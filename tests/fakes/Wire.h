#pragma once
#include "Arduino.h"
struct FakeWire {
 uint8_t registers[32]={};uint16_t pointer=0;unsigned writes=0;bool address=false,fail=false;size_t remaining=0;
 uint8_t device=0;bool sensorConnected=true,sensorAvailable=true,sensorShort=false,sensorClearFail=false;
 bool sensorL1=false;int failReadReg=-1,failWriteReg=-1;uint16_t sensorMm=2000;uint8_t sensorStatus=11,sensorModelId=0xEE;
 void begin(int,int){}void setTimeOut(int){}
 void beginTransmission(uint8_t target){device=target;address=true;writes=0;}
 size_t write(uint8_t v){if(address){pointer=v;address=false;}else if(device==0x29&&sensorL1&&writes==1)pointer=(pointer<<8)|v;else if(device!=0x29)registers[pointer++]=v;writes++;return 1;}
 size_t write(const uint8_t* p,size_t n){for(size_t i=0;i<n;i++)write(p[i]);return n;}
 int endTransmission(bool=true){return (fail||(device==0x29&&(!sensorConnected||(sensorL1&&writes>2&&(int(pointer)==failWriteReg||(pointer==0x86&&sensorClearFail))))))?1:0;}
 size_t requestFrom(uint8_t target,uint8_t n){device=target;remaining=(fail||(device==0x29&&(!sensorConnected||int(pointer)==failReadReg)))?0:n;if(device==0x29&&sensorShort&&remaining)remaining--;return remaining;}
 int read(){if(!remaining)return -1;remaining--;uint16_t reg=pointer++;if(device==0x29){if(sensorL1){if(reg==0x010F)return 0xEA;if(reg==0x0110)return 0xCC;if(reg==0x0031)return sensorAvailable?0:1;if(reg==0x0089){switch(sensorStatus){case 0:return 9;case 1:return 6;case 2:return 4;case 3:return 8;case 4:return 5;case 5:return 1;case 6:return 9;case 7:return 7;case 9:return 12;case 10:return 18;case 13:return 13;default:return 0;}}if(reg==0x008B)return sensorStatus==6?0:1;uint16_t raw=sensorMm==65535?65535:(uint32_t(sensorMm)*2048+1005)/2011;if(reg==0x0096)return raw>>8;if(reg==0x0097)return raw&255;return 0;}if(reg==0x13)return sensorAvailable?7:0;if(reg==0x14)return sensorStatus<<3;if(reg==0x1E)return sensorMm>>8;if(reg==0x1F)return sensorMm&255;if(reg==0xC0)return sensorModelId;return 0;}return registers[reg];}
};inline FakeWire Wire;
