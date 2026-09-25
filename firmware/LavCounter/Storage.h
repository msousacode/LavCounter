#pragma once
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "Event.h"
// JSONL storage derived from the original Storage module. Never auto-formats.
class EventStorage {
 static constexpr const char* PATH="/events-v1.jsonl";
 static constexpr size_t MAX_LINE=8192;
 uint64_t lastId=0;bool ready=false;
 String pending[8];uint8_t pendingCount=0;uint64_t pendingIds[8]={};uint64_t lastAttempt=0;
 void (*service)()=nullptr;
 bool line(File& f,String& out){
  out="";bool oversized=false,terminated=false;
  while(f.available()){
   char c=(char)f.read();if(c=='\n'){terminated=true;break;}
   if(out.length()<MAX_LINE)out+=c;else oversized=true;
   if((out.length()%256)==0&&service)service();
  }
  if(oversized||!terminated){degraded=true;return false;}return !out.isEmpty();
 }
 bool append(const String& value,uint64_t expectedId){
  if(!ready){error="Armazenamento indisponível. Histórico não foi apagado.";return false;}
  if(SPIFFS.totalBytes()-SPIFFS.usedBytes()<value.length()+512){error="Armazenamento cheio. Exporte o histórico e faça manutenção.";return false;}
  bool separator=false,completeWithoutNewline=false;File check=SPIFFS.open(PATH,FILE_READ);
  if(check&&check.size()){
   check.seek(check.size()-1);separator=check.read()!='\n';
   if(separator){
    size_t start=check.size()>MAX_LINE?check.size()-MAX_LINE:0;check.seek(start);String tail;
    while(check.available()){char c=(char)check.read();if(c=='\n')tail="";else tail+=c;}
    JsonDocument previous;
    if(!deserializeJson(previous,tail)&&previous["id"].is<const char*>())completeWithoutNewline=strtoull(previous["id"].as<const char*>(),nullptr,10)==expectedId;
   }
  }check.close();
  File f=SPIFFS.open(PATH,FILE_APPEND);if(!f){error="Falha ao abrir histórico.";return false;}
  size_t prefix=separator?f.print('\n'):0;
  if(completeWithoutNewline){f.flush();f.close();if(prefix==1)return true;error="Falha ao concluir registro pendente.";return false;}
  size_t bytes=(!separator||prefix==1)?f.print(value):0;
  size_t newline=bytes==value.length()?f.print('\n'):0;f.flush();f.close();
  if((separator&&prefix!=1)||bytes!=value.length()||newline!=1){error="Gravação incompleta; evento aguarda nova tentativa.";degraded=true;return false;}
  return true;
 }
public:
 String error;bool degraded=false;uint32_t lostEvents=0;
 bool begin(void (*callback)()){
  service=callback;ready=SPIFFS.begin(false);
  if(!ready){error="SPIFFS não montado. Inicialize a partição explicitamente; não houve formatação automática.";return false;}
  File f=SPIFFS.open(PATH,FILE_READ);String row;
  while(f&&f.available()){
   if(line(f,row)){JsonDocument d;if(deserializeJson(d,row)||!d["id"].is<const char*>()){degraded=true;continue;}
    uint64_t id=strtoull(d["id"].as<const char*>(),nullptr,10);if(id>lastId)lastId=id;}
   if(service)service();
  }f.close();return true;
 }
 void enqueue(const Event& e){
  if(e.id==0){lostEvents++;error="Evento sem ID persistente; verifique a memória de configuração.";return;}
  if(pendingCount==8){lostEvents++;error="Fila de gravação cheia; há eventos não persistidos.";return;}
  JsonDocument d;e.json(d);serializeJson(d,pending[pendingCount]);pendingIds[pendingCount]=e.id;pendingCount++;
 }
 void tick(uint64_t now){
  if(!pendingCount||now-lastAttempt<5000000)return;lastAttempt=now;
  if(pendingIds[0]>lastId){if(!append(pending[0],pendingIds[0]))return;lastId=pendingIds[0];}
  for(size_t i=1;i<pendingCount;i++){pending[i-1]=pending[i];pendingIds[i-1]=pendingIds[i];}
  pending[--pendingCount]="";error="";
 }
 unsigned queued()const{return pendingCount;}
 bool mounted()const{return ready;}
 size_t used()const{return ready?SPIFFS.usedBytes():0;}
 size_t capacity()const{return ready?SPIFFS.totalBytes():0;}
 template<class Emit> uint32_t page(uint32_t cursor,unsigned limit,const String& category,Emit emit,bool& more){
  more=false;if(!ready)return 0;File f=SPIFFS.open(PATH,FILE_READ);if(!f)return 0;
  if(cursor>f.size()){f.close();return 0;}
  if(cursor){f.seek(cursor-1);if(f.read()!='\n'){f.close();return 0;}}else f.seek(0);
  unsigned count=0;String row;uint64_t previousId=0;
  while(f.available()){
   bool complete=line(f,row);if(service)service();
   if(!complete)continue;JsonDocument d;
   if(deserializeJson(d,row)||!d["id"].is<const char*>()||!d["category"].is<const char*>()){degraded=true;continue;}
   uint64_t id=strtoull(d["id"].as<const char*>(),nullptr,10);
   if(id==previousId){degraded=true;continue;}previousId=id;
   if(category.length()&&category!=d["category"].as<const char*>())continue;
   emit(row);if(++count==limit)break;
  }
  uint32_t next=f.position();more=f.available();f.close();return next;
 }
};
