#pragma once
#include "BuildConfig.h"
#include <VL53L0X.h>
#include <VL53L1X.h>
#include "Detection.h"

class PresenceSensor {
 VL53L0X driver;VL53L1X driver1;
 bool isL1=false,initialized=false,seen=false,l1Calibrated=false;
 uint64_t lastPoll=0,lastGood=0,lastInit=0;
 bool readRegisters(uint16_t reg,uint8_t* data,uint8_t count,bool wide=false){
  Wire.beginTransmission(0x29);if(wide)Wire.write(uint8_t(reg>>8));Wire.write(uint8_t(reg));
  if(Wire.endTransmission(false)!=0||Wire.requestFrom(uint8_t(0x29),count)!=count)return false;
  for(uint8_t i=0;i<count;i++){int value=Wire.read();if(value<0)return false;data[i]=uint8_t(value);}return true;
 }
 bool writeRegisters(uint16_t reg,uint16_t value,bool word=false){
  Wire.beginTransmission(0x29);Wire.write(uint8_t(reg>>8));Wire.write(uint8_t(reg));
  if(word)Wire.write(uint8_t(value>>8));Wire.write(uint8_t(value));return Wire.endTransmission()==0;
 }
 static uint16_t word(const uint8_t* p){return (uint16_t(p[0])<<8)|p[1];}
 // Pololu 1.3.1 low-power calibration, DSS and gain, with checked transactions.
 // read(false) in that driver can mask a read error with a successful clear.
 bool prepareL1(const uint8_t* result){
  if(!l1Calibrated){
   uint8_t vhv,loops,phase;
   if(!readRegisters(0x000B,&vhv,1,true)||!readRegisters(0x0008,&loops,1,true)||!readRegisters(0x00D8,&phase,1,true))return false;
   if(!writeRegisters(0x000B,vhv&0x7F)||!writeRegisters(0x0008,(loops&3)+12)||!writeRegisters(0x004D,1)||!writeRegisters(0x0047,phase))return false;
   l1Calibrated=true;
  }
  uint16_t spads=word(result+3);uint32_t rate=uint32_t(word(result+15))+word(result+7),required=0x8000;
  if(rate>65535)rate=65535;
  if(spads){rate=(rate<<16)/spads;if(rate){required=(uint32_t(0x0A00)<<16)/rate;if(required>65535)required=65535;}}
  return writeRegisters(0x0054,uint16_t(required),true);
 }
 static uint8_t l1Status(uint8_t raw,uint8_t stream){
  switch(raw){case 1:case 2:case 3:case 17:return 5;case 13:return 13;case 18:return 10;case 5:return 4;case 4:return 2;case 6:return 1;case 7:return 7;case 12:return 9;case 8:return 3;case 9:return stream?0:6;default:return 255;}
 }
 void unknown(const char* reason,const char* message){healthy=false;distanceValid=false;presenceSample=lav::Sample::Unknown;presenceReason=reason;error=message;}
 bool i2cFailure(uint64_t now,lav::Sample& out){unknown("i2c_error","Falha I2C do sensor ToF");initialized=false;lastInit=now;out=presenceSample;return true;}
 bool accept(uint64_t now,double threshold,bool timedOut,lav::Sample& out){
  seen=true;lastGood=now;lastSampleUs=now;
  if(timedOut)unknown("timeout","Timeout do sensor ToF");
  else{
   distanceValid=rangeStatus==(isL1?0:11)&&distanceMm>0&&distanceMm!=65535;
   bool noTarget=isL1&&rangeStatus==2;healthy=distanceValid||noTarget;
   if(!healthy)unknown("invalid_range",isL1?"Leitura invalida do VL53L1X":"Leitura invalida do VL53L0X");
   else{presenceSample=noTarget?lav::Sample::Absent:distanceMm<=threshold?lav::Sample::Present:lav::Sample::Absent;presenceReason=noTarget?"signal_insufficient":presenceSample==lav::Sample::Present?"within_threshold":"beyond_threshold";error="";}
  }
  out=presenceSample;return true;
 }
public:
 bool healthy=false,distanceValid=false;uint16_t distanceMm=0,modelId=0;
 lav::Sample presenceSample=lav::Sample::Unknown;const char* presenceReason="initializing";
 String error="Sensor ainda não lido";uint8_t rangeStatus=255;uint64_t lastSampleUs=0;
 const char* sampleName()const{return presenceSample==lav::Sample::Present?"present":presenceSample==lav::Sample::Absent?"absent":"unknown";}
 const char* modelName()const{return modelId==0xEACC?"VL53L1X":modelId==0xEE?"VL53L0X":"ToF nao identificado";}
 bool begin(uint64_t now){
  lastInit=now;lastSampleUs=0;rangeStatus=255;modelId=0;seen=false;initialized=false;isL1=false;l1Calibrated=false;unknown("initializing","Aguardando leitura");
  uint8_t id[2]={};bool idRead=readRegisters(0xC0,id,1);modelId=id[0];
  if(!idRead){unknown("i2c_error","Falha I2C ao ler ID do sensor ToF");return false;}
  if(modelId!=0xEE){
   if(!readRegisters(0x010F,id,2,true)){unknown("i2c_error","Falha I2C ao identificar VL53L1X");return false;}
   uint16_t l1Id=word(id);if(l1Id!=0xEACC){unknown("initializing","Sensor nao reconhecido: esperado VL53L0X ou VL53L1X");return false;}
   modelId=l1Id;isL1=true;
  }
  if(isL1){driver1=VL53L1X();driver1.setTimeout(50);initialized=driver1.init();if(initialized)initialized=driver1.setDistanceMode(VL53L1X::Long)&&driver1.setMeasurementTimingBudget(50000);if(initialized){driver1.startContinuous(100);initialized=driver1.last_status==0;}driver1.timeoutOccurred();}
  else{driver.setTimeout(50);initialized=driver.init();if(initialized)initialized=driver.setMeasurementTimingBudget(50000);if(initialized){driver.startContinuous(100);initialized=driver.last_status==0;}driver.timeoutOccurred();}
  if(!initialized)error="Inicializacao/calibracao do sensor ToF falhou";return initialized;
 }
 bool poll(uint64_t now,lav::Sample& out,double threshold){
  if(now-lastPoll<100000)return false;lastPoll=now;
  if(!initialized){if(now-lastInit>=5000000)begin(now);out=lav::Sample::Unknown;return true;}
  if(isL1){
   uint8_t ready;if(!readRegisters(0x0031,&ready,1,true))return i2cFailure(now,out);
   if(!(ready&1)){
    uint8_t result[17];if(!readRegisters(0x0089,result,sizeof(result),true))return i2cFailure(now,out);
    rangeStatus=l1Status(result[0],result[2]);uint16_t rawMm=word(result+13);distanceMm=rawMm==65535?rawMm:(uint32_t(rawMm)*2011+1024)/2048;
    if(!prepareL1(result)||!writeRegisters(0x0086,1))return i2cFailure(now,out);
    return accept(now,threshold,driver1.timeoutOccurred(),out);
   }
  }else{
   uint8_t ready;if(!readRegisters(VL53L0X::RESULT_INTERRUPT_STATUS,&ready,1))return i2cFailure(now,out);
   if(ready&7){
    uint8_t result[12];if(!readRegisters(VL53L0X::RESULT_RANGE_STATUS,result,sizeof(result)))return i2cFailure(now,out);
    distanceMm=word(result+10);rangeStatus=(result[0]&0x78)>>3;
    driver.writeReg(VL53L0X::SYSTEM_INTERRUPT_CLEAR,1);if(driver.last_status!=0)return i2cFailure(now,out);
    return accept(now,threshold,driver.timeoutOccurred(),out);
   }
  }
  if(!seen||now-lastGood>=lav::SAMPLE_VALID_US){unknown("stale","Sensor ToF sem amostra recente");out=presenceSample;if(now-lastInit>=5000000&&(!seen||now-lastGood>=5000000)){initialized=false;lastInit=now-5000000;}return true;}
  return false;
 }
};
