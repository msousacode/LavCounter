#pragma once
#include "Arduino.h"
#define FILE_READ 0
#define FILE_APPEND 1
struct FakeDisk {String bytes;bool mounted=true,failOpen=false;size_t budget=1000000,capacity=1000000;};
inline FakeDisk disk;
class File {
 bool valid=false;size_t pos=0;bool writing=false;
public:
 File()=default;
 File(bool write):valid(!disk.failOpen),pos(write?disk.bytes.length():0),writing(write){}
 operator bool()const{return valid;}
 size_t size()const{return disk.bytes.length();}
 size_t position()const{return pos;}
 bool seek(size_t p){pos=p;return true;}
 bool available()const{return valid&&pos<disk.bytes.length();}
 int read(){return available()?(unsigned char)disk.bytes[pos++]:-1;}
 size_t print(const String& v){return print(v.c_str());}
 size_t print(const char* v){size_t n=0;while(valid&&writing&&*v&&disk.budget){disk.bytes+=*v++;disk.budget--;n++;pos++;}return n;}
 size_t print(char c){char b[]={c,0};return print(b);}
 void flush(){}void close(){valid=false;}
};
struct FakeSPIFFS {
 bool begin(bool format){if(format)abort();return disk.mounted;}
 File open(const char*,int mode){return File(mode==FILE_APPEND);}
 size_t usedBytes()const{return disk.bytes.length();}
 size_t totalBytes()const{return disk.capacity;}
};
inline FakeSPIFFS SPIFFS;
