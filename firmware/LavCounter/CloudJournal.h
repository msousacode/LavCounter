#pragma once
#include <SPIFFS.h>
#include "CloudDigest.h"
#include "BuildConfig.h"
namespace lavcloud {
class CloudJournal {
 const char* path;String binding;
public:
 String error;bool degraded=false;
 explicit CloudJournal(const char* p="/cloud-v1.jsonl"):path(p){}
 void setBinding(const String& b){binding=b;}
 struct Lookup {File file;size_t boundary=0;String row,id,wanted;bool over=false,done=false,matched=false,failed=false;};
 void start(Lookup& c,const String& id,const String& rev){c.file.close();c.row="";c.id=id;c.wanted=rev;c.over=false;c.done=false;c.matched=false;c.failed=false;c.file=SPIFFS.open(path,FILE_READ);c.boundary=c.file?c.file.size():0;if(!c.file){c.done=true;if(SPIFFS.exists(path)){c.failed=true;error="Falha ao ler confirmações.";}}}
 void step(Lookup& c){
  if(c.done)return;unsigned budget=256;
  while(budget--&&c.file.position()<c.boundary){int ch=c.file.read();if(ch<0){c.failed=true;break;}if(ch!='\n'){if(c.row.length()<1024)c.row+=(char)ch;else c.over=true;continue;}
   JsonDocument d;bool ok=!c.over&&!deserializeJson(d,c.row)&&d["version"]==1&&d["binding"]==binding.c_str()&&d["sessionId"].is<const char*>()&&d["revision"].is<const char*>()&&d["checksum"].is<const char*>();
   if(ok){String expected=d["checksum"].as<String>();d.remove("checksum");ok=revision(d.as<JsonVariantConst>())==expected.c_str();}
   if(!ok){degraded=true;c.failed=true;}else if(d["sessionId"]==c.id.c_str())c.matched=d["revision"]==c.wanted.c_str();
   c.row="";c.over=false;
  }
  if(c.file.position()>=c.boundary||!c.file.available()||c.failed&&c.file.position()<c.boundary&&c.row.isEmpty()){
   if(!c.row.isEmpty()||c.over){degraded=true;c.failed=true;}c.file.close();c.done=true;if(c.failed){c.matched=false;error="Confirmações degradadas; reenvio conservador.";}
  }
 }
 bool confirm(const String& id,const String& rev){
  JsonDocument d;d["version"]=1;d["binding"]=binding;d["sessionId"]=id;d["revision"]=rev;d["checksum"]=revision(d.as<JsonVariantConst>());String row;serializeJson(d,row);
  if(SPIFFS.usedBytes()>=SPIFFS.totalBytes()||SPIFFS.totalBytes()-SPIFFS.usedBytes()<row.length()+512){error="Sem espaço para confirmar envio.";return false;}
  File check=SPIFFS.open(path,FILE_READ);bool separator=false;if(check&&check.size()){check.seek(check.size()-1);separator=check.read()!='\n';}check.close();
  File f=SPIFFS.open(path,FILE_APPEND);if(!f){error="Falha ao abrir confirmações.";return false;}
  if(separator&&f.print('\n')!=1){f.close();error="Falha ao separar confirmação incompleta.";return false;}
  size_t n=f.print(row),end=n==row.length()?f.print('\n'):0;f.flush();f.close();
  if(n!=row.length()||end!=1){degraded=true;error="Confirmação local incompleta.";return false;}error="";return true;
 }
};
}
