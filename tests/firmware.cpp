#include <stdlib.h>
#include <new>
void* operator new(size_t n){void* p=malloc(n);if(!p)abort();return p;}
void operator delete(void* p) noexcept{free(p);}
void operator delete(void* p,size_t) noexcept{free(p);}
#include "../firmware/LavCounter/LavCounter.ino"
#include <assert.h>
void pulseAt(uint64_t at){fakeTime=at;onPulse();serviceFlow();}
int main(){
 lav::Date date;assert(lav::parseDate("2026-09-25T12:00:00",date));lav::encodeRtc(date,Wire.registers);
 setup();assert(rtc.valid());assert(server.routeCount==7);
 // Every registered route, including the HTML, must challenge before touching data.
 for(unsigned i=0;i<server.routeCount;i++){server.code=0;server.routes[i].handler();assert(server.code==401);}
 server.notFound();assert(server.code==401);assert(settings.current.revision==1);
 server.allowed=true;getConfig();assert(server.code==200);JsonDocument d;assert(!deserializeJson(d,server.response));assert(d["fields"].size()==lav::FIELD_COUNT);
 pulseAt(1000000);assert(detector.active);assert(current.config.revision==1);pulseAt(2000000);assert(detector.metrics.flowUs==1000000);
 // Set all configuration values through the production HTTP handler while active.
 JsonDocument body;body["revision"]=1;lav::Config modified=settings.current;for(size_t i=0;i<lav::FIELD_COUNT;i++)modified.v[i]+=lav::FIELDS[i].step;modified.v[lav::TIMEOUT]=10;modified.v[lav::WASH_VOLUME]=12;configJson(body["values"].to<JsonObject>(),modified);serializeJson(body,server.plain);setConfig();if(server.code!=200)fprintf(stderr,"%s\n",server.response.c_str());assert(server.code==200);assert(settings.current.revision==2);assert(detector.config.v[lav::TIMEOUT]==180);
 ConfigStore reboot;assert(reboot.begin());assert(reboot.current.v[lav::TIMEOUT]==10);for(size_t i=0;i<lav::FIELD_COUNT;i++)assert(fabs(reboot.current.v[i]-modified.v[i])<1e-9);
 // Clock changes cannot alter detector duration or original timestamps.
 String started=current.startedAt;server.plain="{\"dateTime\":\"2026-09-24T12:00:00\"}";setRtc();assert(server.code==200);pulseAt(3000000);assert(detector.metrics.durationUs()==2000000);assert(current.startedAt==started.c_str());
 // Exactly at timeout: old event closes once; pulse starts with new config.
 pulseAt(183000000);assert(history.queued()==1);assert(detector.metrics.pulses==1);assert(detector.config.v[lav::TIMEOUT]==10);assert(current.config.revision==2);
 history.tick(fakeTime);assert(history.queued()==0);getSessions();assert(server.code==200);assert(!deserializeJson(d,server.response));assert(d["events"].size()==1);assert(d["events"][0]["clockAdjusted"]==true);assert(d["events"][0]["durationMs"]==2000);
 fakeTime=193000000;serviceFlow();assert(!detector.active);assert(history.queued()==1);serviceFlow();assert(history.queued()==1);
 // Invalid changes do not mutate persisted config; stale tabs receive conflict.
 body["revision"]=2;body["values"]["timeoutSeconds"]=0;serializeJson(body,server.plain);setConfig();assert(server.code==400);assert(settings.current.v[lav::TIMEOUT]==10);
 body["revision"]=1;serializeJson(body,server.plain);setConfig();assert(server.code==409);
 // Missing and failed RTC reads are explicit, while pulse acquisition continues.
 Wire.fail=true;assert(!rtc.poll(fakeTime));assert(!rtc.valid());pulseAt(194000000);assert(detector.active);assert(current.startedAt.isEmpty());Wire.fail=false;
 server.plain="{\"dateTime\":\"2023-02-29T00:00\"}";setRtc();assert(server.code==400);
 // Overflow in the production queue marks acquisition degraded.
 for(unsigned i=0;i<4200;i++){fakeTime+=2000;onPulse();}serviceFlow();assert(detector.metrics.degraded);assert(observedLost>0);
 printf("PASS: production firmware handlers with fake transport/hardware: every route auth, live calibration/reboot, clock adjustment, exact timeout, RTC failure, queue overflow\n");
}







