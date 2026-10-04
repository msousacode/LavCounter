#pragma once
#include "Arduino.h"
#define FILE_READ 0
#define FILE_APPEND 1
struct FakeDisk {String bytes;bool mounted=true,failOpen=false;size_t budget=1000000,capacity=1000000;};
inline FakeDisk disk,legacyDisk,mockDisk,cloudDisk,cloudMockDisk;
class File {
 FakeDisk* target=nullptr;bool valid=false;size_t pos=0;bool writing=false;
public:
 File()=default;
 File(FakeDisk& d,bool write):target(&d),valid(!d.failOpen),pos(write?d.bytes.length():0),writing(write){}
 operator bool()const{return valid;}
 size_t size()const{return target?target->bytes.length():0;}
 size_t position()const{return pos;}
 bool seek(size_t p){pos=p;return true;}
 bool available()const{return valid&&pos<target->bytes.length();}
 int read(){return available()?(unsigned char)target->bytes[pos++]:-1;}
 size_t print(const String& v){return print(v.c_str());}
 size_t print(const char* v){size_t n=0;while(valid&&writing&&*v&&target->budget){target->bytes+=*v++;target->budget--;n++;pos++;}return n;}
 size_t print(char c){char b[]={c,0};return print(b);}
 void flush(){}void close(){valid=false;}
};
struct FakeSPIFFS {
 bool begin(bool format){if(format)abort();return disk.mounted;}
 FakeDisk& target(const char* path){assertPath(path);return strcmp(path,"/presence-v2.jsonl")==0?disk:strcmp(path,"/presence-mock-v2.jsonl")==0?mockDisk:strcmp(path,"/cloud-v1.jsonl")==0?cloudDisk:strcmp(path,"/cloud-mock-v1.jsonl")==0?cloudMockDisk:legacyDisk;}
 File open(const char* path,int mode){return File(target(path),mode==FILE_APPEND);}
 void assertPath(const char* p){if(strcmp(p,"/presence-v2.jsonl")&&strcmp(p,"/presence-mock-v2.jsonl")&&strcmp(p,"/events-v1.jsonl")&&strcmp(p,"/cloud-v1.jsonl")&&strcmp(p,"/cloud-mock-v1.jsonl"))abort();}
 bool exists(const char* p){return target(p).bytes.length()>0;}
 size_t usedBytes()const{return disk.bytes.length()+legacyDisk.bytes.length()+mockDisk.bytes.length()+cloudDisk.bytes.length()+cloudMockDisk.bytes.length();}
 size_t totalBytes()const{return disk.capacity;}
};
inline FakeSPIFFS SPIFFS;
