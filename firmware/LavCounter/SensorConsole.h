#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "PresenceSensor.h"
#include "Detection.h"

// Bounded serial input/output; diagnostics share the normal acquisition, never start a second reader.
class SensorConsole {
 char input[48]={},output[384]={};size_t used=0,sent=0,total=0;
 bool overflow=false,streaming=true;uint8_t probe=0;uint64_t nextReport=0;
 bool pending()const{return sent<total;}
 void message(const char* text){if(pending())return;snprintf(output,sizeof(output),"%s\r\n",text);sent=0;total=strlen(output);}
 void report(uint64_t now,const PresenceSensor& sensor,lav::State state,double threshold){
  if(pending())return;
  char age[32];if(sensor.lastSampleUs)snprintf(age,sizeof(age),"%llu",(unsigned long long)((now-sensor.lastSampleUs)/1000));else snprintf(age,sizeof(age),"sem_amostra");
  snprintf(output,sizeof(output),"[%s] id=0x%04X valido=%s bruto_mm=%u status=%u idade_ms=%s limiar_mm=%.0f estado=%s ocupacao=%s motivo=%s saudavel=%s erro=%s\r\n",sensor.modelName(),unsigned(sensor.modelId),sensor.distanceValid?"sim":"nao",unsigned(sensor.distanceMm),unsigned(sensor.rangeStatus),age,threshold,lav::stateName(state),sensor.sampleName(),sensor.presenceReason,sensor.healthy?"sim":"nao",sensor.error.c_str());
  sent=0;total=strlen(output);
 }
public:
 void begin(){streaming=true;nextReport=0;message("Sensor automatico VL53L0X/VL53L1X: 115200 baud. Leituras a cada 1 s; sensor off para silenciar.");}
 void tick(uint64_t now,const PresenceSensor& sensor,lav::State state,double threshold){
  if(pending()){
   int free=Serial.availableForWrite();if(free>0){size_t n=total-sent;if(n>64)n=64;if(n>size_t(free))n=size_t(free);sent+=Serial.write(reinterpret_cast<const uint8_t*>(output+sent),n);}return;
  }
  // One address per loop; no scan across all 127 I2C addresses.
  if(probe){uint8_t address=probe==1?0x29:0x68;Wire.beginTransmission(address);int code=Wire.endTransmission();snprintf(output,sizeof(output),"[I2C] 0x%02X: %s (codigo=%d)\r\n",unsigned(address),code==0?"responde":"sem resposta",code);sent=0;total=strlen(output);probe=probe==1?2:0;return;}
  for(unsigned budget=0;budget<32&&Serial.available()>0;budget++){
   int value=Serial.read();if(value<0)break;char c=char(value);
   if(c=='\r')continue;
   if(c=='\n'){
    input[used]=0;
    if(overflow)message("Comando muito longo. Digite ajuda.");
    else if(strcmp(input,"ajuda")==0||strcmp(input,"help")==0)message("sensor: leitura atual | sensor on: leituras a cada 1 s | sensor off: parar | i2c: consultar 0x29 e 0x68. Testes de presenca podem contar lavagens.");
    else if(strcmp(input,"sensor")==0)report(now,sensor,state,threshold);
    else if(strcmp(input,"sensor on")==0){streaming=true;nextReport=now;message("Leituras ativadas (1 s). sensor off para parar.");}
    else if(strcmp(input,"sensor off")==0){streaming=false;message("Leituras desativadas.");}
    else if(strcmp(input,"i2c")==0){probe=1;message("Consultando SDA=21 / SCL=22; resposta I2C nao confirma o modelo do modulo.");}
    else if(used)message("Comando desconhecido. Digite ajuda.");
    used=0;overflow=false;if(pending()||probe)return;
   }else if(used<sizeof(input)-1&&!overflow)input[used++]=c;else overflow=true;
  }
  if(streaming&&now>=nextReport&&!pending()){report(now,sensor,state,threshold);nextReport=now+1000000;}
 }
};
