#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <esp_timer.h>
#include "Detection.h"
#include "ConfigStore.h"
#include "RtcClock.h"
#include "Storage.h"
#include "PresenceSensor.h"
#include "WebPage.h"
// Active-high indicators: GPIO -> LED with built-in resistor -> GND.
constexpr int LED_RED_PIN=25;
constexpr int LED_YELLOW_PIN=18;
constexpr int LED_GREEN_PIN=19;
void updateLeds(lav::State state,bool healthy,uint64_t now){
 bool on=lav::ledOn(state,healthy,now);
 digitalWrite(LED_RED_PIN,on&&(state==lav::State::Idle||state==lav::State::WaitClear));
 digitalWrite(LED_YELLOW_PIN,on&&state==lav::State::Candidate);
 digitalWrite(LED_GREEN_PIN,on&&state==lav::State::ValidWash);
}
WebServer server(80);ConfigStore settings;RtcClock rtc;EventStorage history;lav::Detector detector;PresenceSensor sensor;
Event current;uint32_t sessionClockRevision=0;uint64_t lastRtcPoll=0;bool ready=false,servicing=false;
void servicePresence(){
 if(!ready||servicing)return;servicing=true;
 uint64_t now=esp_timer_get_time();lav::Sample sample;
 const auto& c=detector.active()?detector.config:settings.current;
 if(sensor.poll(now,sample,c.v[lav::PRESENCE_MM])){
  auto change=detector.sample(now,sample,settings.current);
  if(change.started){current=Event{};current.id=settings.reserveId();current.config=detector.config;current.startedAt=rtc.at(now);sessionClockRevision=rtc.revision;}
  if(change.validated||change.closed){current.validated=detector.validated;current.durationMs=(now-detector.startedUs)/1000;current.clockAdjusted=rtc.revision!=sessionClockRevision;
   if(change.validated)current.validatedAt=rtc.at(now);
   if(change.closed){current.closed=true;current.closedAt=rtc.at(now);}
   history.enqueue(current);
  }
 }
 updateLeds(detector.state(),sensor.healthy,now);
 servicing=false;
}
bool authenticated(){if(server.authenticate("lavcounter","lav123"))return true;server.requestAuthentication(BASIC_AUTH,"LavCounter");return false;}
void sendJson(int code,JsonDocument& d){String body;serializeJson(d,body);server.sendHeader("Cache-Control","no-store");server.send(code,"application/json; charset=utf-8",body);}
void errorResponse(int code,const char* message){JsonDocument d;d["error"]=message;sendJson(code,d);}
void getStatus(){
 if(!authenticated())return;servicePresence();JsonDocument d;
 String date=rtc.at(esp_timer_get_time());auto saved=history.totals(false,date.c_str());auto all=history.totals(true,date.c_str());auto state=detector.state();
 d["schemaVersion"]=2;d["product"]="LavCounter";d["firmwareVersion"]=lav::VERSION;d["source"]="local";
 d["state"]=lav::stateName(state);d["sensorHealthy"]=sensor.healthy;d["sensorError"]=sensor.error;
 if(sensor.healthy)d["distanceMm"]=sensor.distanceMm;else d["distanceMm"]=nullptr;
 d["ledColor"]=lav::ledColor(state);d["ledBlinking"]=!sensor.healthy||state==lav::State::WaitClear;
 d["observedWashes"]=history.observed;d["persistedWashes"]=saved.total;
 if(rtc.valid())d["todayWashes"]=all.today;else d["todayWashes"]=nullptr;
 d["undatedWashes"]=all.undated;d["rtcValid"]=rtc.valid();d["rtcError"]=rtc.error;d["configError"]=settings.error;
 d["storageError"]=history.error;d["storageDegraded"]=history.degraded;d["queuedEvents"]=history.queued();d["lostEvents"]=history.lostEvents;
 d["storageUsed"]=history.used();d["storageCapacity"]=history.capacity();d["configRevision"]=settings.current.revision;
 if(detector.active()){JsonDocument e;Event live=current;live.durationMs=(esp_timer_get_time()-detector.startedUs)/1000;live.json(e);d["session"].set(e);}else d["session"]=nullptr;
 sendJson(200,d);
}
void getConfig(){if(!authenticated())return;JsonDocument d;settings.describe(d);sendJson(200,d);}
void setConfig(){
 if(!authenticated())return;servicePresence();JsonDocument input,result;
 if(server.arg("plain").length()>4096||deserializeJson(input,server.arg("plain"))){errorResponse(400,"Configuração inválida.");return;}
 if(!input["revision"].is<uint32_t>()||input["revision"].as<uint32_t>()!=settings.current.revision){errorResponse(409,"A configuração mudou. Recarregue os valores antes de salvar.");return;}
 lav::Config next;if(!parseConfig(input["values"],next,result["errors"].to<JsonObject>())){sendJson(400,result);return;}
 if(!settings.save(next)){errorResponse(500,settings.error.c_str());return;}settings.describe(result);sendJson(200,result);
}
void getRtc(){if(!authenticated())return;JsonDocument d;d["valid"]=rtc.valid();d["dateTime"]=rtc.at(esp_timer_get_time());d["error"]=rtc.error;sendJson(200,d);}
void setRtc(){
 if(!authenticated())return;servicePresence();JsonDocument d;lav::Date date;
 if(server.arg("plain").length()>256||deserializeJson(d,server.arg("plain"))||!d["dateTime"].is<const char*>()||!lav::parseDate(d["dateTime"],date)){errorResponse(400,"Data ou hora inválida (2000–2099).");return;}
 if(!rtc.set(date,esp_timer_get_time())){errorResponse(503,rtc.error.c_str());return;}getRtc();
}
bool unsignedArg(const String& value,uint64_t& number){if(value.isEmpty())return false;uint64_t n=0;for(char c:value){if(c<'0'||c>'9'||n>(UINT64_MAX-(c-'0'))/10)return false;n=n*10+c-'0';}number=n;return true;}
void getSessions(){
 if(!authenticated())return;uint64_t limit=20,cursor=0;
 if(server.hasArg("category")){errorResponse(400,"Filtro de categoria removido no contrato v2.");return;}
 if((server.hasArg("limit")&&!unsignedArg(server.arg("limit"),limit))||!limit||limit>50||(server.hasArg("cursor")&&!unsignedArg(server.arg("cursor"),cursor))){errorResponse(400,"Paginação inválida (1 a 50 sessões).");return;}
 if(!history.mounted()){errorResponse(503,history.error.c_str());return;}
 server.sendHeader("Cache-Control","no-store");server.setContentLength(CONTENT_LENGTH_UNKNOWN);server.send(200,"application/json; charset=utf-8","");server.sendContent("{\"schemaVersion\":2,\"events\":[");
 bool first=true,more=false;uint64_t next=history.page(cursor,(unsigned)limit,[&](JsonDocument& d){if(!first)server.sendContent(",");first=false;String row;serializeJson(d,row);server.sendContent(row);servicePresence();},more);
 char b[24];snprintf(b,sizeof(b),"%llu",(unsigned long long)next);
 server.sendContent(String("],\"nextCursor\":\"")+b+"\",\"hasMore\":"+(more?"true":"false")+",\"degraded\":"+(history.degraded?"true":"false")+"}");server.sendContent("");
}
void setup(){
 Serial.begin(115200);settings.begin();Wire.begin(21,22);Wire.setTimeOut(20);rtc.poll(esp_timer_get_time());
 const int pins[]={LED_RED_PIN,LED_YELLOW_PIN,LED_GREEN_PIN};
 for(int p:pins){digitalWrite(p,LOW);pinMode(p,OUTPUT);}
 updateLeds(detector.state(),sensor.healthy,esp_timer_get_time());
 pinMode(26,OUTPUT);digitalWrite(26,HIGH);
 history.begin(nullptr);settings.ensureIdAbove(history.maxId());sensor.begin(esp_timer_get_time());ready=true;
 WiFi.softAP("LavCounter","lavcounter123");MDNS.begin("lavcounter");
 server.on("/",HTTP_GET,[]{if(authenticated()){server.sendHeader("Cache-Control","no-store");server.send_P(200,"text/html; charset=utf-8",WEB_PAGE);}});
 server.on("/api/status",HTTP_GET,getStatus);server.on("/api/config",HTTP_GET,getConfig);server.on("/api/config",HTTP_POST,setConfig);
 server.on("/api/rtc",HTTP_GET,getRtc);server.on("/api/rtc",HTTP_POST,setRtc);server.on("/api/sessions",HTTP_GET,getSessions);
 server.onNotFound([]{if(authenticated())errorResponse(404,"Recurso não encontrado.");});server.begin();history.setService(servicePresence);
}
void loop(){servicePresence();uint64_t now=esp_timer_get_time();if(now-lastRtcPoll>=1000000){rtc.poll(now);lastRtcPoll=now;}history.tick(now);servicePresence();server.handleClient();servicePresence();delay(1);}
