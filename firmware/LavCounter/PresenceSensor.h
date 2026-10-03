#pragma once
#include <VL53L1X.h>
#include "Detection.h"
class PresenceSensor {
 VL53L1X driver;bool initialized=false,seen=false;uint64_t lastPoll=0,lastGood=0,lastInit=0;
public:
 bool healthy=false;uint16_t distanceMm=0;String error="Sensor ainda não lido";
 bool begin(uint64_t now){lastInit=now;driver.setTimeout(50);initialized=driver.init();if(initialized)initialized=driver.setDistanceMode(VL53L1X::Long)&&driver.setMeasurementTimingBudget(50000);if(initialized){driver.startContinuous(100);initialized=driver.last_status==0;}healthy=false;seen=false;error=initialized?"Aguardando leitura":"VL53L1X indisponível";return initialized;}
 bool poll(uint64_t now,lav::Sample& out,double threshold){
  if(now-lastPoll<100000)return false;lastPoll=now;
  if(!initialized){if(now-lastInit>=5000000)begin(now);out=lav::Sample::Unknown;return true;}
  bool available=driver.dataReady();
  if(driver.last_status!=0){healthy=false;error="Falha I2C do VL53L1X";initialized=false;lastInit=now;out=lav::Sample::Unknown;return true;}
  if(available){
   distanceMm=driver.read(false);
   healthy=!driver.timeoutOccurred()&&driver.last_status==0&&driver.ranging_data.range_status==VL53L1X::RangeValid;
   seen=true;lastGood=now;error=healthy?"":"Leitura inválida do VL53L1X";
   out=healthy?(distanceMm<=threshold?lav::Sample::Present:lav::Sample::Absent):lav::Sample::Unknown;return true;
  }
  if(!seen||now-lastGood>=lav::SAMPLE_VALID_US){healthy=false;error="VL53L1X sem amostra recente";out=lav::Sample::Unknown;if(seen&&now-lastGood>=5000000){initialized=false;lastInit=now-5000000;}return true;}
  return false;
 }
};
