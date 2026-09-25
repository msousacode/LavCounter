#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "Calendar.h"
class RtcClock {
 uint64_t anchorUs=0;int64_t anchorEpoch=0;bool good=false;
 bool readRegisters(uint8_t reg,uint8_t* data,size_t len){
  Wire.beginTransmission(0x68);Wire.write(reg);if(Wire.endTransmission(false)!=0)return false;
  if(Wire.requestFrom((uint8_t)0x68,(uint8_t)len)!=len)return false;
  for(size_t i=0;i<len;i++)data[i]=Wire.read();return true;
 }
public:
 uint32_t revision=0;String error="RTC ainda não lido";
 bool poll(uint64_t now){
  uint8_t b[7],status;lav::Date d;
  if(!readRegisters(0,b,7)||!readRegisters(0x0f,&status,1)||!lav::decodeRtc(b,status,d)){good=false;error="RTC indisponível ou sem horário válido. Ajuste o relógio.";return false;}
  int64_t e=lav::epoch(d);
  if(good){int64_t expected=anchorEpoch+(int64_t)(now-anchorUs)/1000000;if(llabs(e-expected)>2)revision++;}
  anchorUs=now;anchorEpoch=e;good=true;error="";return true;
 }
 bool valid()const{return good;}
 String at(uint64_t when)const{
  if(!good)return "";
  int64_t seconds=(int64_t(when)-int64_t(anchorUs))/1000000;
  int64_t e=anchorEpoch+seconds;if(e<946684800LL||e>4102444799LL)return "";
  char b[24];lav::formatDate(e,b,sizeof(b));return String(b);
 }
 bool set(const lav::Date& d,uint64_t now){
  if(!lav::validDate(d))return false;uint8_t b[7];lav::encodeRtc(d,b);
  Wire.beginTransmission(0x68);Wire.write(0);Wire.write(b,7);
  if(Wire.endTransmission()!=0){good=false;error="Falha ao gravar RTC.";return false;}
  uint8_t status;if(!readRegisters(0x0f,&status,1)){good=false;error="Falha ao verificar RTC.";return false;}
  Wire.beginTransmission(0x68);Wire.write(0x0f);Wire.write(status&0x7f);
  if(Wire.endTransmission()!=0){good=false;error="Falha ao validar RTC.";return false;}
  good=false;revision++;return poll(now);
 }
};
