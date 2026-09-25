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
#include "WebPage.h"

constexpr int FLOW_PIN=27;
WebServer server(80);
ConfigStore settings;
RtcClock rtc;
EventStorage history;
lav::Detector detector;
lav::PulseQueue<4096> pulses;
portMUX_TYPE pulseMux=portMUX_INITIALIZER_UNLOCKED;
Event current;
uint64_t accepted=0,rejected=0,lastAccepted=0,observedLost=0,lastRtcPoll=0,rateAt=0,ratePulses=0;
uint32_t sessionClockRevision=0;
bool hasAccepted=false,servicing=false,ready=false;
double currentRate=0;

void IRAM_ATTR onPulse(){
 uint64_t now=(uint64_t)esp_timer_get_time();
 portENTER_CRITICAL_ISR(&pulseMux);
 // Keep the complete ISR in IRAM; do not call a flash-resident template while
 // SPIFFS/NVS writes temporarily disable the flash cache.
 pulses.total++;size_t next=(pulses.head+1)%4096;
 if(next==pulses.tail)pulses.lost++;
 else {pulses.timestamps[pulses.head]=now;pulses.head=next;}
 portEXIT_CRITICAL_ISR(&pulseMux);
}
void finishEvent(){
 detector.close();current.metrics=detector.metrics;current.closedAt=rtc.at(detector.metrics.closed);
 current.clockAdjusted=rtc.revision!=sessionClockRevision;
 history.enqueue(current);
 Serial.printf("LavCounter: evento encerrado, categoria %s\n",lav::categoryName(lav::classify(current.metrics,current.config).category));
}
void serviceFlow(){
 if(!ready||servicing)return;servicing=true;
 uint64_t lost;portENTER_CRITICAL(&pulseMux);lost=pulses.lost;portEXIT_CRITICAL(&pulseMux);
 bool degraded=lost!=observedLost;observedLost=lost;
 if(degraded&&detector.active)detector.metrics.degraded=true;
 for(size_t count=0;count<4096;count++){
  uint64_t at;portENTER_CRITICAL(&pulseMux);bool got=pulses.pop(at);portEXIT_CRITICAL(&pulseMux);if(!got)break;
  if(detector.expired(at))finishEvent();
  const lav::Config& c=detector.active?detector.config:settings.current;
  if(hasAccepted&&at-lastAccepted<(uint64_t)llround(c.v[lav::FILTER_MS]*1000)){rejected++;continue;}
  hasAccepted=true;lastAccepted=at;accepted++;
  if(!detector.active){
   detector.start(at,settings.current);current=Event{};current.id=settings.reserveId();current.config=detector.config;
   current.startedAt=rtc.at(at);sessionClockRevision=rtc.revision;rateAt=at;ratePulses=accepted;
  }else detector.pulse(at);
  if(degraded)detector.metrics.degraded=true;
  current.finishedAt=rtc.at(at);
 }
 uint64_t now=esp_timer_get_time();
 // Never close beyond an unprocessed timestamp if the queue remains busy.
 portENTER_CRITICAL(&pulseMux);bool empty=pulses.head==pulses.tail;portEXIT_CRITICAL(&pulseMux);
 if(empty&&detector.expired(now))finishEvent();
 const lav::Config& c=detector.active?detector.config:settings.current;
 if(now-rateAt>=c.us(lav::SAMPLE_SECONDS)){
  currentRate=(accepted-ratePulses)/c.v[lav::PULSES_PER_LITER]*60000000.0/(now-rateAt);rateAt=now;ratePulses=accepted;
 }
 servicing=false;
}
bool authenticated(){
 if(server.authenticate("lavcounter","lav123"))return true;
 server.requestAuthentication(BASIC_AUTH,"LavCounter");return false;
}
void sendJson(int code,JsonDocument& d){String body;serializeJson(d,body);server.sendHeader("Cache-Control","no-store");server.send(code,"application/json; charset=utf-8",body);}
void errorResponse(int code,const char* message){JsonDocument d;d["error"]=message;sendJson(code,d);}
void getStatus(){
 if(!authenticated())return;serviceFlow();JsonDocument d;
 auto state=detector.state(esp_timer_get_time());d["product"]="LavCounter";d["firmwareVersion"]=lav::VERSION;
 d["state"]=state==lav::State::Idle?"IDLE":state==lav::State::Flowing?"FLOWING":"WAITING_RESUME";
 d["rateLpm"]=currentRate;d["acceptedPulses"]=accepted;d["rejectedPulses"]=rejected;d["lostPulses"]=observedLost;
 d["rtcValid"]=rtc.valid();d["rtcError"]=rtc.error;d["configError"]=settings.error;
 d["storageError"]=history.error;d["storageDegraded"]=history.degraded;d["queuedEvents"]=history.queued();d["lostEvents"]=history.lostEvents;
 d["storageUsed"]=history.used();d["storageCapacity"]=history.capacity();d["configRevision"]=settings.current.revision;
 if(detector.active){d["volumeL"]=detector.metrics.liters(detector.config);d["durationMs"]=detector.metrics.durationUs()/1000;d["flowMs"]=detector.metrics.flowUs/1000;d["pauseCount"]=detector.metrics.pauses;d["resumeCount"]=detector.metrics.resumes;d["sessionConfigRevision"]=detector.config.revision;}
 sendJson(200,d);
}
void getConfig(){if(!authenticated())return;JsonDocument d;settings.describe(d);sendJson(200,d);}
void setConfig(){
 if(!authenticated())return;serviceFlow();JsonDocument input,result;
 if(server.arg("plain").length()>4096||deserializeJson(input,server.arg("plain"))){errorResponse(400,"Configuração inválida.");return;}
 if(!input["revision"].is<uint32_t>()||input["revision"].as<uint32_t>()!=settings.current.revision){errorResponse(409,"A configuração mudou. Recarregue os valores antes de salvar.");return;}
 lav::Config next;
 if(!parseConfig(input["values"],next,result["errors"].to<JsonObject>())){sendJson(400,result);return;}
 if(!settings.save(next)){errorResponse(500,settings.error.c_str());return;}
 settings.describe(result);sendJson(200,result);
}
void getRtc(){if(!authenticated())return;JsonDocument d;d["valid"]=rtc.valid();d["dateTime"]=rtc.at(esp_timer_get_time());d["error"]=rtc.error;sendJson(200,d);}
void setRtc(){
 if(!authenticated())return;serviceFlow();JsonDocument d;lav::Date date;
 if(server.arg("plain").length()>256||deserializeJson(d,server.arg("plain"))||!d["dateTime"].is<const char*>()||!lav::parseDate(d["dateTime"],date)){errorResponse(400,"Data ou hora inválida (2000–2099).");return;}
 if(!rtc.set(date,esp_timer_get_time())){errorResponse(503,rtc.error.c_str());return;}getRtc();
}
bool unsignedArg(const String& value,uint32_t& number){
 if(value.isEmpty())return false;uint64_t n=0;for(char c:value){if(c<'0'||c>'9')return false;n=n*10+c-'0';if(n>UINT32_MAX)return false;}number=(uint32_t)n;return true;
}
void getSessions(){
 if(!authenticated())return;
 uint32_t limit=20,cursor=0;
 if((server.hasArg("limit")&&!unsignedArg(server.arg("limit"),limit))||!limit||limit>50||(server.hasArg("cursor")&&!unsignedArg(server.arg("cursor"),cursor))){errorResponse(400,"Paginação inválida (1 a 50 eventos).");return;}
 String category=server.arg("category");if(category.length()&&category!="washing"&&category!="unidentified"&&category!="discarded"){errorResponse(400,"Categoria inválida.");return;}
 if(!history.mounted()){errorResponse(503,history.error.c_str());return;}
 server.sendHeader("Cache-Control","no-store");server.setContentLength(CONTENT_LENGTH_UNKNOWN);server.send(200,"application/json; charset=utf-8","");server.sendContent("{\"events\":[");
 bool first=true,more=false;
 uint32_t next=history.page(cursor,limit,category,[&](const String& row){if(!first)server.sendContent(",");first=false;server.sendContent(row);serviceFlow();},more);
 server.sendContent(String("],\"nextCursor\":")+next+",\"hasMore\":"+(more?"true":"false")+",\"degraded\":"+(history.degraded?"true":"false")+"}");server.sendContent("");
}
void setup(){
 Serial.begin(115200);Serial.printf("LavCounter %s\n",lav::VERSION);
 settings.begin();Wire.begin(21,22);Wire.setTimeOut(20);rtc.poll(esp_timer_get_time());
 pinMode(FLOW_PIN,INPUT_PULLUP);ready=true;attachInterrupt(digitalPinToInterrupt(FLOW_PIN),onPulse,FALLING);
 history.begin(serviceFlow);
 const int ledPins[]={5,18,19};for(int p:ledPins){pinMode(p,OUTPUT);digitalWrite(p,LOW);}pinMode(26,OUTPUT);digitalWrite(26,HIGH);
 WiFi.softAP("LavCounter","lavcounter123");MDNS.begin("lavcounter");
 server.on("/",HTTP_GET,[]{if(authenticated()){server.sendHeader("Cache-Control","no-store");server.send_P(200,"text/html; charset=utf-8",WEB_PAGE);}});
 server.on("/api/status",HTTP_GET,getStatus);server.on("/api/config",HTTP_GET,getConfig);server.on("/api/config",HTTP_POST,setConfig);
 server.on("/api/rtc",HTTP_GET,getRtc);server.on("/api/rtc",HTTP_POST,setRtc);server.on("/api/sessions",HTTP_GET,getSessions);
 server.onNotFound([]{if(authenticated())errorResponse(404,"Recurso não encontrado.");});server.begin();
 Serial.print("LavCounter: acesso web em http://");Serial.println(WiFi.softAPIP());
}
void loop(){
 serviceFlow();uint64_t now=esp_timer_get_time();
 if(now-lastRtcPoll>=1000000){rtc.poll(now);lastRtcPoll=now;}
 history.tick(now);serviceFlow();server.handleClient();serviceFlow();
 auto state=detector.state(esp_timer_get_time());digitalWrite(5,state==lav::State::Idle);digitalWrite(18,state==lav::State::WaitingResume);digitalWrite(19,state==lav::State::Flowing);
 delay(1);
}
