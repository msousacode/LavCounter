#include <assert.h>
#include <stdio.h>
#include "../firmware/LavCounter/PresenceSensor.h"
int main(){
 lav::Sample sample;
 const bool models[]={false,true};const uint16_t boundaries[]={1499,1500,1501,0,65535},rawValues[]={0,100,1500,2600,65535};
 const int reads[]={0x31,0x89,0x0B,0x08,0xD8},writes[]={0x0B,0x08,0x4D,0x47,0x54,0x86};
 for(bool l1:models){
  Wire=FakeWire{};Wire.sensorL1=l1;Wire.sensorStatus=l1?0:11;PresenceSensor sensor;assert(sensor.begin(0));uint64_t now=0;
  for(uint16_t mm:boundaries){
   Wire.sensorMm=mm;assert(sensor.poll(now+=100000,sample,1500));
   assert(sample==(mm==0||mm==65535?lav::Sample::Unknown:mm<=1500?lav::Sample::Present:lav::Sample::Absent));
   assert(sensor.distanceValid==(mm!=0&&mm!=65535));
  }
  Wire.sensorMm=1200;assert(sensor.poll(now+=100000,sample,1000)&&sample==lav::Sample::Absent);
  assert(sensor.poll(now+=100000,sample,1200)&&sample==lav::Sample::Present);
  for(unsigned status=0;status<(l1?256:16);status++){
   Wire.sensorStatus=status;Wire.sensorMm=1000;assert(sensor.poll(now+=100000,sample,1500));
   assert(sample==(status==(l1?0:11)?lav::Sample::Present:l1&&status==2?lav::Sample::Absent:lav::Sample::Unknown));
  }
 }
 Wire=FakeWire{};Wire.sensorL1=true;Wire.sensorStatus=2;PresenceSensor sensor;assert(sensor.begin(0));uint64_t now=0;
 for(uint16_t mm:rawValues){
  Wire.sensorMm=mm;assert(sensor.poll(now+=100000,sample,1500)&&sample==lav::Sample::Absent);
  assert(sensor.healthy&&!sensor.distanceValid&&sensor.error.isEmpty());assert(strcmp(sensor.presenceReason,"signal_insufficient")==0);
 }
 VL53L1X::timedOut=true;assert(sensor.poll(now+=100000,sample,1500)&&sample==lav::Sample::Unknown);
 assert(!sensor.healthy&&strcmp(sensor.presenceReason,"timeout")==0);
 assert(sensor.poll(now+=100000,sample,1500)&&sample==lav::Sample::Absent);
 Wire.sensorAvailable=false;
 for(unsigned i=0;i<4;i++)assert(!sensor.poll(now+=100000,sample,1500));
 assert(sensor.poll(now+=100000,sample,1500)&&sample==lav::Sample::Unknown);
 assert(!sensor.healthy&&!sensor.distanceValid&&strcmp(sensor.sampleName(),"unknown")==0);
 Wire.sensorAvailable=true;assert(sensor.poll(now+=100000,sample,1500)&&sample==lav::Sample::Absent);
 // Every acquisition phase must fail closed, including calibration/DSS writes
 // that precede a successful interrupt clear in the uninstrumented driver.
 for(int reg:reads){
  PresenceSensor failed;assert(failed.begin(0));Wire.failReadReg=reg;
  assert(failed.poll(100000,sample,1500)&&sample==lav::Sample::Unknown&&!failed.healthy);Wire.failReadReg=-1;
 }
 for(int reg:writes){
  PresenceSensor failed;assert(failed.begin(0));Wire.failWriteReg=reg;
  assert(failed.poll(100000,sample,1500)&&sample==lav::Sample::Unknown&&!failed.healthy);Wire.failWriteReg=-1;
 }
 for(unsigned fault=0;fault<3;fault++){
  PresenceSensor failed;assert(failed.begin(0));Wire.sensorShort=fault==0;Wire.sensorClearFail=fault==1;Wire.sensorConnected=fault!=2;
  assert(failed.poll(100000,sample,1500)&&sample==lav::Sample::Unknown&&!failed.healthy);
  Wire.sensorShort=false;Wire.sensorClearFail=false;Wire.sensorConnected=true;
  assert(failed.poll(5100000,sample,1500)&&sample==lav::Sample::Unknown);
  assert(failed.poll(5200000,sample,1500)&&sample==lav::Sample::Absent);
 }
 puts("PASS physical occupancy: model statuses, inclusive thresholds, status 2, checked I2C/calibration/DSS/clear, timeout, expiry, recovery");
}
