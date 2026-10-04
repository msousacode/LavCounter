#pragma once
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "Event.h"
// Journal is ordered by increasing reserved session ID. Two records per session.
class EventStorage {
 const char* PATH;
 static constexpr size_t MAX_LINE=4096;
 bool ready=false,retrying=false;String pending[8];unsigned count=0;uint64_t lastAttempt=0,bootMax=0,lastObserved=0;uint64_t generation_=0;
 void (*service)()=nullptr;
 bool line(File& f,String& out){out="";bool over=false;while(f.available()){int c=f.read();if(c=='\n')return !over&&!out.isEmpty();if(out.length()<MAX_LINE)out+=(char)c;else over=true;if(service&&out.length()%128==0)service();}if(out.length())degraded=true;return false;}
 static uint64_t idOf(JsonDocument& d){const char* s=d["sessionId"]|"";if(!*s)return 0;uint64_t id=0;for(;*s;s++){if(*s<'0'||*s>'9'||id>(UINT64_MAX-(*s-'0'))/10)return 0;id=id*10+(*s-'0');}return id;}
 bool parse(const String& row,JsonDocument& d){if(deserializeJson(d,row)||d["schemaVersion"]!=2||!idOf(d)||!d["validated"].is<bool>()){degraded=true;return false;}const char* type=d["type"]|"";if(strcmp(type,"validated")&&strcmp(type,"closed")){degraded=true;return false;}return true;}
 template<class Visit> void records(bool withPending,Visit visit){
  File f=SPIFFS.open(PATH,FILE_READ);if(!f&&SPIFFS.exists(PATH)){degraded=true;error="Falha ao ler histórico.";}String row;while(f&&f.available()){if(line(f,row)){JsonDocument d;if(parse(row,d))visit(d);}else degraded=true;if(service)service();}f.close();
  // Snapshot prevents newly queued entries changing this traversal.
  unsigned n=withPending?count:0;for(unsigned i=0;i<n;i++){JsonDocument d;if(parse(pending[i],d))visit(d);}
 }
 bool append(const String& value){
  if(!ready){error="Armazenamento indisponível.";return false;}
  if(SPIFFS.usedBytes()>=SPIFFS.totalBytes()||SPIFFS.totalBytes()-SPIFFS.usedBytes()<value.length()+512){error="Armazenamento cheio; preserve e exporte os dados.";return false;}
  bool separator=false;File check=SPIFFS.open(PATH,FILE_READ);if(check&&check.size()){check.seek(check.size()-1);separator=check.read()!='\n';}check.close();
  File f=SPIFFS.open(PATH,FILE_APPEND);if(!f){error="Falha ao abrir histórico.";return false;}
  if(separator&&f.print('\n')!=1){f.close();error="Falha ao separar registro incompleto.";return false;}
  size_t bytes=f.print(value);size_t newline=bytes==value.length()?f.print('\n'):0;f.flush();f.close();
  if(bytes!=value.length()||newline!=1){degraded=true;error="Gravação incompleta; registro pendente.";return false;}return true;
 }
public:
 explicit EventStorage(const char* path=lav::JOURNAL_PATH):PATH(path){}
 String error;bool degraded=false;uint32_t lostEvents=0;uint64_t observed=0;
 struct Totals {uint64_t total=0,today=0,undated=0;};
 bool begin(void (*callback)()){
  service=callback;ready=SPIFFS.begin(false);if(!ready){error="SPIFFS indisponível; nenhuma formatação foi realizada.";return false;}
  scan(false,[&](JsonDocument& d){uint64_t id=idOf(d);if(id>bootMax)bootMax=id;if(d["validated"]==true)observed++;});lastObserved=bootMax;return true;
 }
 void setService(void (*callback)()){service=callback;}
 uint64_t maxId()const{return bootMax;}
 bool enqueue(const Event& e){
  if(!e.closed&&e.validated&&(!e.id||e.id>lastObserved)){observed++;lastObserved=e.id;}
  if(!e.id||count==8){lostEvents++;error="Registro não persistido: fila cheia ou ID indisponível.";return false;}
  JsonDocument d;e.json(d);String row;serializeJson(d,row);
  for(unsigned i=0;i<count;i++)if(pending[i]==row.c_str())return true;
  pending[count++]=row;return true;
 }
 void tick(uint64_t now){
  if(!count||(retrying&&now-lastAttempt<5000000))return;lastAttempt=now;
  if(!append(pending[0])){retrying=true;return;}retrying=false;
  for(unsigned i=1;i<count;i++)pending[i-1]=pending[i];pending[--count]="";error="";generation_++;
 }
 unsigned queued()const{return count;}bool mounted()const{return ready;}
 size_t used()const{return ready?SPIFFS.usedBytes():0;}size_t capacity()const{return ready?SPIFFS.totalBytes():0;}
 uint64_t generation()const{return generation_;}
 // At most 256 bytes of file input per step, captured durable boundary, no RAM queue.
 // Caller owns cursor exclusively on the main loop; cloud worker never accesses files.
 struct DurableCursor {File file;size_t boundary=0;String row;JsonDocument merged;uint64_t id=0;bool closed=false,over=false,done=false,failed=false;};
 bool startDurable(DurableCursor& c){
  c.file.close();c.row="";c.merged.clear();c.id=0;c.closed=false;c.over=false;c.done=false;c.failed=false;c.boundary=0;
  if(!ready){c.done=c.failed=true;return false;}c.file=SPIFFS.open(PATH,FILE_READ);
  if(!c.file){c.done=true;c.failed=SPIFFS.exists(PATH);if(c.failed){degraded=true;error="Falha ao ler histórico.";}return !c.failed;}
  c.boundary=c.file.size();return true;
 }
 // returns true only when one consolidated session is available in out.
 bool durableStep(DurableCursor& c,JsonDocument& out){
  if(c.done)return false;
  auto emit=[&](){out.set(c.merged);if(out["sensorMode"].isNull())out["sensorMode"]="physical";out["interrupted"]=!c.closed&&c.id<=bootMax;};
  unsigned budget=256;
  while(budget--&&c.file.position()<c.boundary){
   int ch=c.file.read();if(ch<0){c.failed=true;degraded=true;break;}
   if(ch!='\n'){if(c.row.length()<MAX_LINE)c.row+=(char)ch;else c.over=true;continue;}
   JsonDocument d;bool valid=!c.over&&!c.row.isEmpty()&&parse(c.row,d);c.row="";c.over=false;
   if(!valid){degraded=true;c.failed=true;continue;}
   uint64_t id=idOf(d);if(id<c.id){degraded=true;c.failed=true;continue;}
   bool closed=d["type"]=="closed";
   if(id!=c.id){bool have=c.id!=0;if(have)emit();c.id=id;c.closed=closed;c.merged.set(d);if(service)service();if(have)return true;}
   else if(closed&&!c.closed){c.closed=true;c.merged.set(d);}
  }
  if(service)service();
  if(c.file.position()>=c.boundary||!c.file.available()){
   if(!c.row.isEmpty()||c.over){degraded=true;c.failed=true;}c.file.close();c.done=true;if(c.id){emit();c.id=0;return true;}
  }return false;
 }
 // Constant memory streaming consolidation; late/out-of-order journal entries are rejected.
 template<class Visit> void scan(bool withPending,Visit visit){
  JsonDocument merged;uint64_t currentId=0;bool closed=false;
  auto emit=[&](){if(!currentId)return;if(merged["sensorMode"].isNull())merged["sensorMode"]="physical";merged["interrupted"]=!closed&&currentId<=bootMax;visit(merged);};
  records(withPending,[&](JsonDocument& d){uint64_t id=idOf(d);if(id<currentId){degraded=true;return;}bool nextClosed=d["type"]=="closed";
   if(id!=currentId){emit();currentId=id;closed=nextClosed;merged.set(d);}
   else if(nextClosed&&!closed){merged.set(d);closed=true;}
  });emit();
 }
 Totals totals(bool includePending,const char* date){Totals t;if(!ready&&!includePending)return t;scan(includePending,[&](JsonDocument& d){if(d["validated"]!=true)return;t.total++;const char* when=d["validatedAt"]|"";if(!*when)t.undated++;else if(date&&strlen(date)>=10&&!strncmp(when,date,10))t.today++;});return t;}
 template<class Emit> uint64_t page(uint64_t after,unsigned limit,Emit emit,bool& more){unsigned n=0;uint64_t next=after;more=false;scan(true,[&](JsonDocument& d){uint64_t id=idOf(d);if(id<=after)return;if(n>=limit){more=true;return;}emit(d);next=id;n++;});return next;}
};
