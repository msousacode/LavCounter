#define LAVCOUNTER_SENSOR_MOCK 1
#include <assert.h>
#include <stdio.h>
#include "../firmware/LavCounter/LavCounter.ino"
#ifdef VL53L1X_h
#error Mock must not include physical sensor driver
#endif
void ticks(unsigned n){for(unsigned i=0;i<n;i++){fakeTime+=100000;servicePresence();history.tick(fakeTime);}}
void command(const char* value){JsonDocument d;d["sample"]=value;serializeJson(d,server.plain);setMockSensor();assert(server.code==200);}
int main(){
 ConfigStore physical;assert(physical.begin("lavcounter"));lav::Config p;p.v[lav::WASH_SECONDS]=15;assert(physical.save(p));assert(physical.reserveId()==1);
 const String originalConfig=Preferences::config;
 // A legacy physical record without sensorMode must survive mock operations.
 disk.bytes="{\"schemaVersion\":2,\"sessionId\":\"1\",\"type\":\"validated\",\"validated\":true}\n";
 const String originalDisk=disk.bytes;
 setup();assert(settings.current.v[lav::WASH_SECONDS]==120);
 for(unsigned i=0;i<server.routeCount;i++){server.allowed=false;server.routes[i].handler();assert(server.code==401);}server.allowed=true;
 JsonDocument cloudState;getCloudSync();assert(server.code==200&&!deserializeJson(cloudState,server.response));assert(cloudState["sensorMode"]=="mock"&&cloudState["state"]=="not_configured");assert(lavcloud::uuid(cloudState["clientId"]|""));server.plain="{}";requestCloudSync();assert(server.code==503);getPremiumLink();assert(server.code==503);
 lav::Config c;c.v[lav::WASH_SECONDS]=1;c.v[lav::ABSENCE_SECONDS]=1;assert(settings.save(c));
 assert(detector.state()==lav::State::WaitClear);ticks(10);assert(!detector.active());ticks(1);assert(detector.state()==lav::State::Idle);
 const char* invalid[]={"{}","[]","null","{\"sample\":1}","{\"sample\":\"other\"}","{\"sample\":\"present\",\"extra\":1}","{"};
 for(auto body:invalid){server.plain=body;setMockSensor();assert(server.code==400);assert(!strcmp(sensor.sampleName(),"absent"));}
 command("present");ticks(1);assert(detector.active());assert(pinValues[18]==1);auto id=current.id;
 ticks(5);command("fault");ticks(1);assert(!sensor.healthy);assert(!detector.validated);
 command("present");ticks(10);assert(!detector.validated);command("present");ticks(1);assert(detector.validated&&current.id==id);assert(history.observed==1&&pinValues[19]==1);
 ticks(20);assert(history.observed==1);command("fault");ticks(1);assert(detector.validated&&detector.active());getStatus();JsonDocument d;assert(!deserializeJson(d,server.response));assert(d["ledBlinking"]==true&&d["sensorHealthy"]==false);assert(d["source"]=="local"&&d["sensorMode"]=="mock"&&d["mockSample"]=="fault");assert(d["distanceMm"].isNull());assert(d["session"]["sensorMode"]=="mock");
 command("absent");ticks(5);command("present");ticks(1);assert(current.id==id&&detector.validated);
 command("absent");ticks(10);assert(detector.active());ticks(1);assert(!detector.active()&&pinValues[25]==1);
 getSessions();assert(!deserializeJson(d,server.response));assert(d["events"].size()==1&&d["events"][0]["type"]=="closed"&&d["events"][0]["sensorMode"]=="mock");
 // Real RTC failures remain visible and never prevent a simulated wash.
 Wire.fail=true;rtc.poll(fakeTime);command("present");ticks(11);assert(history.observed==2);getStatus();assert(!deserializeJson(d,server.response));assert(d["todayWashes"].isNull());assert(d["undatedWashes"]==2);
 mockDisk.failOpen=true;command("absent");ticks(11);assert(history.queued()==1&&!history.error.isEmpty());mockDisk.failOpen=false;ticks(50);assert(history.queued()==0);
 EventStorage reopened;assert(reopened.begin(nullptr));assert(reopened.observed==2);assert(reopened.totals(false,nullptr).total==2);
 bool more=false;unsigned rows=0;auto cursor=reopened.page(0,1,[&](JsonDocument& e){rows++;assert(e["sensorMode"]=="mock");},more);assert(rows==1&&more);reopened.page(cursor,1,[&](JsonDocument&){rows++;},more);assert(rows==2&&!more);
 EventStorage real("/presence-v2.jsonl");assert(real.begin(nullptr));assert(real.observed==1);real.page(0,20,[&](JsonDocument& e){assert(e["sensorMode"]=="physical");},more);
 assert(disk.bytes==originalDisk.c_str()&&Preferences::config==originalConfig.c_str()&&Preferences::next==2);
 ConfigStore reload;assert(reload.begin());assert(reload.current.v[lav::WASH_SECONDS]==1);assert(reload.reserveId()==3);
 detector=lav::Detector{};sensor.begin(fakeTime);ticks(10);assert(detector.state()==lav::State::WaitClear);ticks(1);assert(detector.state()==lav::State::Idle);assert(!strcmp(sensor.sampleName(),"absent"));
 puts("PASS mock firmware: timed cycle, fault, auth, API, RTC, isolation, retry, pagination, restart");
}
