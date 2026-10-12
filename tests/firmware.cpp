#include <assert.h>
#include <stdio.h>
#include "../firmware/LavCounter/LavCounter.ino"
void samples(uint16_t mm,unsigned n){VL53L0X::mm=mm;for(unsigned i=0;i<n;i++){fakeTime+=100000;servicePresence();}}
int main(){
 // Check every physical output, both blink phases, and exclusive colors.
 pinValues[5]=-1;pinValues[23]=-1;
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
 assert(pinValues[5]==-1&&pinValues[23]==-1);
 const String oldSimulation="{\"sessionId\":\"99\",\"validated\":true,\"sensorMode\":\"mock\"}\n";
 mockDisk.bytes=oldSimulation;Preferences::mockConfig="old-simulation-settings";
 setup();assert(detector.state()==lav::State::Idle&&!detector.active());assert(history.observed==0);for(unsigned i=0;i<server.routeCount;i++){server.allowed=false;server.routes[i].handler();assert(server.code==401);}server.allowed=true;
 getCloudSync();JsonDocument cloudState;assert(server.code==200&&!deserializeJson(cloudState,server.response));assert(cloudState["state"]=="not_configured"&&cloudState["premiumStatus"]=="unknown");assert(cloudState["sensorMode"]=="physical");assert(lavcloud::uuid(cloudState["clientId"]|""));server.plain="{}";requestCloudSync();assert(server.code==503);getPremiumLink();assert(server.code==503);cloudConfig.signupUrl="https://saas.example/cadastro";getPremiumLink();assert(server.code==200&&!deserializeJson(cloudState,server.response));assert(cloudState["url"]==identity.premiumLink(cloudConfig).c_str());server.plain="{\"premium\":true}";requestCloudSync();assert(server.code==400);
 server.plain="{\"dateTime\":\"2026-10-01T12:00:00\"}";setRtc();assert(server.code==200);
 lav::Config c;c.v[lav::WASH_SECONDS]=60;c.v[lav::ABSENCE_SECONDS]=30;c.v[lav::CANDIDATE_ABSENCE_SECONDS]=3;assert(settings.save(c));
 samples(2000,11);assert(detector.state()==lav::State::Idle);assert(pinValues[25]==1&&pinValues[18]==0&&pinValues[19]==0);
 samples(1000,1);assert(detector.active());assert(pinValues[25]==0&&pinValues[18]==1&&pinValues[19]==0);
 samples(1000,600);assert(detector.validated);assert(history.observed==1);assert(pinValues[25]==0&&pinValues[18]==0&&pinValues[19]==1);
 samples(1000,20);assert(history.observed==1);getStatus();JsonDocument d;assert(!deserializeJson(d,server.response));assert(d["state"]=="VALID_WASH");assert(d["observedWashes"]==1);assert(d["distanceMm"]==1000);assert(d["rateLpm"].isNull());
 JsonDocument body;body["revision"]=settings.current.revision;c.v[lav::WASH_SECONDS]=120;configJson(body["values"].to<JsonObject>(),c);serializeJson(body,server.plain);setConfig();assert(server.code==200);assert(detector.config.v[lav::WASH_SECONDS]==60);
 String started=current.startedAt;server.plain="{\"dateTime\":\"2026-10-02T12:00:00\"}";setRtc();assert(server.code==200);assert(current.startedAt==started.c_str());serializeJson(body,server.plain);setConfig();assert(server.code==409);body["revision"]=settings.current.revision;body["values"]["washMinSeconds"]=0;serializeJson(body,server.plain);setConfig();assert(server.code==400);
 samples(2000,5);VL53L0X::status=VL53L0X::Invalid;samples(2000,1);assert(detector.active());VL53L0X::status=VL53L0X::RangeValid;samples(2000,300);assert(detector.active());samples(2000,1);assert(!detector.active());assert(history.queued()==2);
 fakeTime+=5000000;history.tick(fakeTime);fakeTime+=5000000;history.tick(fakeTime);getSessions();assert(server.code==200);assert(!deserializeJson(d,server.response));assert(d["events"].size()==1);assert(d["events"][0]["validated"]==true);assert(d["events"][0]["clockAdjusted"]==true);
 server.category="washing";getSessions();assert(server.code==400);server.category="";server.cursor="18446744073709551616";getSessions();assert(server.code==400);server.cursor="";
 VL53L0X::available=false;samples(1000,6);assert(!sensor.healthy);getStatus();assert(!deserializeJson(d,server.response));assert(d["distanceMm"].isNull());
 VL53L0X::connected=false;samples(1000,1);assert(!sensor.healthy);VL53L0X::connected=true;VL53L0X::available=true;samples(1000,60);assert(sensor.healthy);
 Wire.fail=true;assert(!rtc.poll(fakeTime));getStatus();assert(!deserializeJson(d,server.response));assert(d["todayWashes"].isNull());Wire.fail=false;
 PresenceSensor boundary;VL53L0X::connected=true;VL53L0X::available=true;assert(boundary.begin(0));lav::Sample sample;
 VL53L0X::mm=1500;assert(boundary.poll(100000,sample,1500));assert(sample==lav::Sample::Present);
 VL53L0X::mm=1501;assert(boundary.poll(200000,sample,1500));assert(sample==lav::Sample::Absent);
 VL53L0X::timedOut=true;assert(boundary.poll(300000,sample,1500));assert(sample==lav::Sample::Unknown);VL53L0X::timedOut=false;
 for(uint8_t status=0;status<16;status++){VL53L0X::status=status;assert(boundary.poll(400000+status*100000,sample,1500));assert(sample==(status==11?lav::Sample::Absent:lav::Sample::Unknown));}VL53L0X::status=VL53L0X::RangeValid;
 VL53L0X::mm=65535;assert(boundary.poll(2000000,sample,1500));assert(sample==lav::Sample::Unknown);
 VL53L0X::mm=0;assert(boundary.poll(2100000,sample,1500));assert(sample==lav::Sample::Unknown);
 Wire.sensorShort=true;assert(boundary.poll(2200000,sample,1500));assert(sample==lav::Sample::Unknown&&!boundary.healthy);Wire.sensorShort=false;
 assert(boundary.begin(2300000));Wire.sensorClearFail=true;assert(boundary.poll(2400000,sample,1500));assert(sample==lav::Sample::Unknown&&!boundary.healthy);Wire.sensorClearFail=false;
 PresenceSensor noData;VL53L0X::available=false;assert(noData.begin(0));assert(noData.poll(5000000,sample,1500));assert(sample==lav::Sample::Unknown);VL53L0X::available=true;VL53L0X::mm=1501;assert(noData.poll(5100000,sample,1500));assert(noData.poll(5200000,sample,1500));assert(sample==lav::Sample::Absent);
 VL53L0X::connected=false;PresenceSensor absent;assert(!absent.begin(0));assert(absent.error=="Falha I2C ao ler ID do sensor ToF");assert(absent.poll(100000,sample,1500));assert(sample==lav::Sample::Unknown);
 VL53L0X::connected=true;VL53L0X::modelId=0xEA;PresenceSensor wrongModel;assert(!wrongModel.begin(0));assert(wrongModel.modelId==0xEA&&wrongModel.error=="Sensor nao reconhecido: esperado VL53L0X ou VL53L1X");VL53L0X::modelId=0xEE;
 Wire.sensorL1=true;Wire.sensorStatus=0;Wire.sensorMm=1500;PresenceSensor l1;assert(l1.begin(0));assert(l1.modelId==0xEACC&&strcmp(l1.modelName(),"VL53L1X")==0);
 assert(l1.poll(100000,sample,1500)&&sample==lav::Sample::Present);Wire.sensorMm=1501;assert(l1.poll(200000,sample,1500)&&sample==lav::Sample::Absent);
 Wire.sensorStatus=4;assert(l1.poll(300000,sample,1500)&&sample==lav::Sample::Unknown);Wire.sensorStatus=0;
 VL53L1X::timedOut=true;assert(l1.poll(400000,sample,1500)&&sample==lav::Sample::Unknown);
 Wire.sensorShort=true;assert(l1.poll(500000,sample,1500)&&sample==lav::Sample::Unknown&&!l1.healthy);Wire.sensorShort=false;
 assert(l1.begin(600000));Wire.sensorClearFail=true;assert(l1.poll(700000,sample,1500)&&sample==lav::Sample::Unknown);Wire.sensorClearFail=false;
 assert(l1.begin(800000));Wire.sensorAvailable=false;assert(l1.poll(5800000,sample,1500)&&sample==lav::Sample::Unknown);Wire.sensorAvailable=true;l1.poll(5900000,sample,1500);assert(l1.poll(6000000,sample,1500)&&sample==lav::Sample::Absent);
 Wire.sensorConnected=false;assert(l1.poll(6100000,sample,1500)&&!l1.healthy);Wire.sensorConnected=true;l1.poll(11100000,sample,1500);assert(l1.poll(11200000,sample,1500)&&l1.healthy);
 Wire.sensorL1=false;Wire.sensorStatus=11;
 puts("PASS firmware: authentication, API v2, one count, snapshot, validation/conflict, sensor invalid/stale/disconnect/recovery, RTC failure");
 getStatus();assert(!deserializeJson(d,server.response));assert(d["sensorMode"]=="physical");
 for(JsonPairConst item:d.as<JsonObjectConst>())assert(strcmp(item.key().c_str(),"mockSample")!=0);
 for(unsigned i=0;i<server.routeCount;i++)assert(strcmp(server.routes[i].path,"/api/mock-sensor")!=0);
 server.notFound();assert(server.code==404);
 // Console stays bounded with long input and a blocked USB receiver; uses cached acquisition.
 VL53L0X::connected=true;VL53L0X::mm=1000;PresenceSensor consoleSensor;assert(consoleSensor.begin(0));assert(consoleSensor.poll(100000,sample,1500));
 assert(consoleSensor.rangeStatus==11&&consoleSensor.lastSampleUs==100000);
 SensorConsole console;Serial=FakeSerial{};console.begin();
 auto runConsole=[&](const char* command,uint64_t now){Serial.input=command;Serial.cursor=0;Serial.output="";for(unsigned i=0;i<40;i++)console.tick(now,consoleSensor,lav::State::Candidate,1500);};
 runConsole("",0);assert(strstr(Serial.output.c_str(),"[VL53L0X]"));
 runConsole("sensor\r\n",1000000);assert(strstr(Serial.output.c_str(),"bruto_mm=1000")&&strstr(Serial.output.c_str(),"status=11")&&strstr(Serial.output.c_str(),"idade_ms=900"));
 runConsole("sensor on\n",1000000);assert(strstr(Serial.output.c_str(),"Leituras ativadas")&&strstr(Serial.output.c_str(),"[VL53L0X]"));
 runConsole("",1500000);assert(Serial.output.isEmpty());runConsole("",2000000);assert(strstr(Serial.output.c_str(),"[VL53L0X]"));
 runConsole("sensor off\n",2000000);assert(strstr(Serial.output.c_str(),"desativadas"));runConsole("",3000000);assert(Serial.output.isEmpty());
 runConsole("i2c\n",3000000);assert(strstr(Serial.output.c_str(),"0x29: responde")&&strstr(Serial.output.c_str(),"0x68: responde"));
 VL53L0X::connected=false;runConsole("i2c\n",3000000);assert(strstr(Serial.output.c_str(),"0x29: sem resposta"));VL53L0X::connected=true;
 runConsole("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\nsensor\n",3000000);assert(strstr(Serial.output.c_str(),"muito longo")&&strstr(Serial.output.c_str(),"bruto_mm=1000"));
 Serial.txFree=0;runConsole("sensor\n",3000000);assert(Serial.output.isEmpty());Serial.txFree=1;runConsole("",3000000);assert(Serial.output.length()==40);Serial.txFree=64;runConsole("",3000000);assert(Serial.output.length()>0);
 puts("PASS sensor console: cached range/status/age, streaming cadence, stop, CRLF, bounded input/output, I2C diagnostics");
 assert(mockDisk.bytes==oldSimulation.c_str()&&Preferences::mockConfig=="old-simulation-settings");
 // Exercise the actual sketch with a VL53L1X, including operational absence.
 Wire.sensorL1=true;Wire.sensorStatus=0;Wire.sensorMm=1500;
 sensor=PresenceSensor{};assert(sensor.begin(fakeTime));detector=lav::Detector{};
 c.v[lav::WASH_SECONDS]=60;c.v[lav::ABSENCE_SECONDS]=30;c.v[lav::CANDIDATE_ABSENCE_SECONDS]=3;assert(settings.save(c));
 uint64_t countBefore=history.observed;
 samples(1500,1);assert(detector.state()==lav::State::Candidate);auto sessionId=current.id;
 samples(1500,600);assert(detector.validated&&history.observed==countBefore+1);
 assert(pinValues[19]==1&&pinValues[25]==0);
 getStatus();assert(!deserializeJson(d,server.response));assert(d["presenceSample"]=="present"&&d["distanceValid"]==true&&d["sensorRangeStatus"]==0);
 Wire.sensorStatus=2;samples(2600,1);
 getStatus();assert(!deserializeJson(d,server.response));assert(d["presenceSample"]=="absent"&&d["presenceReason"]=="signal_insufficient"&&d["distanceMm"].isNull()&&d["sensorHealthy"]==true&&d["distanceValid"]==false);
 Wire.sensorStatus=0;samples(2600,4);Wire.sensorStatus=7;samples(2201,1);assert(detector.active()&&!sensor.healthy&&sensor.presenceSample==lav::Sample::Unknown);
 Wire.sensorStatus=2;samples(2600,296);assert(detector.active()&&pinValues[19]==1);
 samples(2600,1);assert(!detector.active()&&current.id==sessionId&&history.observed==countBefore+1&&pinValues[25]==1&&pinValues[19]==0);
 Wire.sensorStatus=0;samples(1000,1);assert(detector.active()&&current.id!=sessionId);
 Wire.sensorStatus=7;samples(1000,1);getStatus();assert(!deserializeJson(d,server.response));assert(d["presenceSample"]=="unknown"&&d["distanceMm"].isNull()&&d["sensorHealthy"]==false);
 Wire.sensorStatus=2;VL53L1X::timedOut=true;samples(1000,1);getStatus();assert(!deserializeJson(d,server.response));assert(d["presenceReason"]=="timeout");
 VL53L1X::timedOut=false;samples(1000,1);Wire.sensorAvailable=false;samples(1000,5);getStatus();assert(!deserializeJson(d,server.response));assert(d["presenceReason"]=="stale");
 Wire.sensorAvailable=true;Wire.failReadReg=0x89;samples(1000,1);getStatus();assert(!deserializeJson(d,server.response));assert(d["presenceReason"]=="i2c_error");Wire.failReadReg=-1;
 server.allowed=false;getStatus();assert(server.code==401);server.allowed=true;
 PresenceSensor diagnostic;assert(diagnostic.begin(0));assert(diagnostic.poll(100000,sample,1500));
 SensorConsole automatic;Serial=FakeSerial{};automatic.begin();for(unsigned i=0;i<40;i++)automatic.tick(100000,diagnostic,lav::State::Idle,1500);
 assert(strstr(Serial.output.c_str(),"ocupacao=absent")&&strstr(Serial.output.c_str(),"valido=nao")&&strstr(Serial.output.c_str(),"saudavel=sim"));
 Wire.sensorStatus=7;assert(diagnostic.poll(200000,sample,1500));Serial.output="";Serial.input="sensor\n";Serial.cursor=0;for(unsigned i=0;i<40;i++)automatic.tick(200000,diagnostic,lav::State::Candidate,1500);
 assert(strstr(Serial.output.c_str(),"ocupacao=unknown")&&strstr(Serial.output.c_str(),"saudavel=nao"));
 // Silencing diagnostics never disables acquisition or its state transitions.
 Serial.input="sensor off\n";Serial.cursor=0;for(unsigned i=0;i<40;i++)automatic.tick(300000,diagnostic,lav::State::Candidate,1500);
 assert(sensor.begin(fakeTime));Wire.sensorStatus=2;samples(2600,31);assert(!detector.active()&&history.observed==countBefore+1);
 fakeTime+=5000000;history.tick(fakeTime);fakeTime+=5000000;history.tick(fakeTime);fakeTime+=5000000;history.tick(fakeTime);
 getSessions();assert(!deserializeJson(d,server.response));unsigned matches=0;
 for(JsonObjectConst event:d["events"].as<JsonArrayConst>())if(strtoull(event["sessionId"]|"0",nullptr,10)==sessionId){matches++;assert(event["validated"]==true&&event["type"]=="closed"&&event["interrupted"]==false);}assert(matches==1);
 JsonDocument rejected;rejected["revision"]=settings.current.revision;configJson(rejected["values"].to<JsonObject>(),settings.current);rejected["values"]["presenceMaxMm"]=600;serializeJson(rejected,server.plain);setConfig();assert(server.code==400);
 rejected["values"]["presenceMaxMm"]=1500;rejected["values"].remove("candidateAbsenceSeconds");serializeJson(rejected,server.plain);setConfig();assert(server.code==400);
 getConfig();assert(!deserializeJson(d,server.response));assert(d["fields"].size()==4&&d["fields"][0]["options"].size()==3&&d["fields"][1]["options"].size()==5&&d["fields"][2]["options"].size()==3&&d["fields"][3]["options"].size()==4);
 puts("PASS VL53L1X sketch: operational absence, one count, consolidated same closing ID, next ID, LEDs, authenticated diagnostics, automatic console, sensor off isolation and fixed configuration API");
}


