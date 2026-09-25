#pragma once
#include "Arduino.h"
struct FakeWire {
 uint8_t registers[32]={};uint8_t pointer=0;bool address=false,fail=false;size_t remaining=0;
 void begin(int,int){}void setTimeOut(int){}
 void beginTransmission(uint8_t){address=true;}
 size_t write(uint8_t v){if(address){pointer=v;address=false;}else registers[pointer++]=v;return 1;}
 size_t write(const uint8_t* p,size_t n){for(size_t i=0;i<n;i++)write(p[i]);return n;}
 int endTransmission(bool=true){return fail?1:0;}
 size_t requestFrom(uint8_t,uint8_t n){remaining=fail?0:n;return remaining;}
 int read(){if(!remaining)return -1;remaining--;return registers[pointer++];}
};inline FakeWire Wire;
