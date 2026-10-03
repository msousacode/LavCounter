#pragma once
#include <Arduino.h>
#include "Detection.h"
class MockPresenceSensor {
 lav::Sample selected=lav::Sample::Absent;
 uint64_t lastPoll=0;
public:
 bool healthy=true;uint16_t distanceMm=0;String error;
 bool begin(uint64_t now){lastPoll=now;selected=lav::Sample::Absent;healthy=true;error="";return true;}
 const char* sampleName()const{return selected==lav::Sample::Present?"present":selected==lav::Sample::Absent?"absent":"fault";}
 bool select(const char* value){
  if(!value)return false;
  if(!strcmp(value,"present"))selected=lav::Sample::Present;
  else if(!strcmp(value,"absent"))selected=lav::Sample::Absent;
  else if(!strcmp(value,"fault"))selected=lav::Sample::Unknown;
  else return false;
  healthy=selected!=lav::Sample::Unknown;error=healthy?"":"Falha simulada do sensor";return true;
 }
 bool poll(uint64_t now,lav::Sample& out,double){
  if(now-lastPoll<100000)return false;
  lastPoll=now;out=selected;return true;
 }
};
