#include <assert.h>
#include <stdio.h>
#include "../firmware/LavCounter/LavCounter.ino"
void samples(uint16_t mm,unsigned n){VL53L1X::mm=mm;for(unsigned i=0;i<n;i++){fakeTime+=100000;servicePresence();}}
int main(){
 // Check every physical output, both blink phases, and exclusive colors.
 pinValues[5]=-1;
 const lav::State states[]={lav::State::WaitClear,lav::State::Idle,lav::State::Candidate,lav::State::ValidWash};
 const bool healthValues[]={false,true};
 const uint64_t phases[]={0,499999,500000,999999,1000000};
 for(auto state:states){
  for(bool healthy:healthValues)for(uint64_t at:phases){
   updateLeds(state,healthy,at);
   bool lit=(healthy&&state!=lav::State::WaitClear)||at<500000||at>=1000000;
   assert(pinValues[25]==(lit&&(state==lav::State::Idle||state==lav::State::WaitClear)));
   assert(pinValues[18]==(lit&&state==lav::State::Candidate));
   assert(pinValues[19]==(lit&&state==lav::State::ValidWash));
   assert(pinValues[25]+pinValues[18]+pinValues[19]<=1);
  }
 }
 assert(pinValues[5]==-1);
 setup();for(unsigned i=0;i<server.routeCount;i++){server.allowed=false;server.routes[i].handler();assert(server.code==401);}server.allowed=true;server.plain="{\"dateTime\":\"2026-10-01T12:00:00\"}";setRtc();assert(server.code==200);
 lav::Config c;c.v[lav::WASH_SECONDS]=1;c.v[lav::ABSENCE_SECONDS]=1;assert(settings.save(c));
 samples(2000,11);assert(detector.state()==lav::State::Idle);assert(pinValues[25]==1&&pinValues[18]==0&&pinValues[19]==0);
 samples(1000,1);assert(detector.active());assert(pinValues[25]==0&&pinValues[18]==1&&pinValues[19]==0);
 samples(1000,10);assert(detector.validated);assert(history.observed==1);assert(pinValues[25]==0&&pinValues[18]==0&&pinValues[19]==1);
 samples(1000,20);assert(history.observed==1);getStatus();JsonDocument d;assert(!deserializeJson(d,server.response));assert(d["state"]=="VALID_WASH");assert(d["observedWashes"]==1);assert(d["distanceMm"]==1000);assert(d["rateLpm"].isNull());
 JsonDocument body;body["revision"]=settings.current.revision;c.v[lav::WASH_SECONDS]=2;configJson(body["values"].to<JsonObject>(),c);serializeJson(body,server.plain);setConfig();assert(server.code==200);assert(detector.config.v[lav::WASH_SECONDS]==1);
 String started=current.startedAt;server.plain="{\"dateTime\":\"2026-10-02T12:00:00\"}";setRtc();assert(server.code==200);assert(current.startedAt==started.c_str());serializeJson(body,server.plain);setConfig();assert(server.code==409);body["revision"]=settings.current.revision;body["values"]["washMinSeconds"]=0;serializeJson(body,server.plain);setConfig();assert(server.code==400);
 samples(2000,5);VL53L1X::status=VL53L1X::Invalid;samples(2000,1);assert(detector.active());VL53L1X::status=VL53L1X::RangeValid;samples(2000,10);assert(detector.active());samples(2000,1);assert(!detector.active());assert(history.queued()==2);
 fakeTime+=5000000;history.tick(fakeTime);fakeTime+=5000000;history.tick(fakeTime);getSessions();assert(server.code==200);assert(!deserializeJson(d,server.response));assert(d["events"].size()==1);assert(d["events"][0]["validated"]==true);assert(d["events"][0]["clockAdjusted"]==true);
 server.category="washing";getSessions();assert(server.code==400);server.category="";server.cursor="18446744073709551616";getSessions();assert(server.code==400);server.cursor="";
 VL53L1X::available=false;samples(1000,6);assert(!sensor.healthy);getStatus();assert(!deserializeJson(d,server.response));assert(d["distanceMm"].isNull());
 VL53L1X::connected=false;samples(1000,1);assert(!sensor.healthy);VL53L1X::connected=true;VL53L1X::available=true;samples(1000,60);assert(sensor.healthy);
 Wire.fail=true;assert(!rtc.poll(fakeTime));getStatus();assert(!deserializeJson(d,server.response));assert(d["todayWashes"].isNull());
 PresenceSensor boundary;VL53L1X::connected=true;VL53L1X::available=true;assert(boundary.begin(0));lav::Sample sample;
 VL53L1X::mm=1500;assert(boundary.poll(100000,sample,1500));assert(sample==lav::Sample::Present);
 VL53L1X::mm=1501;assert(boundary.poll(200000,sample,1500));assert(sample==lav::Sample::Absent);
 VL53L1X::timedOut=true;assert(boundary.poll(300000,sample,1500));assert(sample==lav::Sample::Unknown);VL53L1X::timedOut=false;
 VL53L1X::connected=false;PresenceSensor absent;assert(!absent.begin(0));assert(absent.poll(100000,sample,1500));assert(sample==lav::Sample::Unknown);
 puts("PASS firmware: authentication, API v2, one count, snapshot, validation/conflict, sensor invalid/stale/disconnect/recovery, RTC failure");
}
