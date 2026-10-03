#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
class String {
 char* value=nullptr;size_t length_=0;
public:
 String(const char* s=""){*this=s;}
 String(const String& s){*this=s.c_str();}
 String(unsigned int n){char b[32];snprintf(b,sizeof(b),"%u",n);*this=b;}
 String(double n,unsigned int decimals){char b[80];snprintf(b,sizeof(b),"%.*f",(int)decimals,n);*this=b;}
 ~String(){free(value);}
 String& operator=(const char* s){if(!s)s="";size_t n=strlen(s);char* p=(char*)malloc(n+1);memcpy(p,s,n+1);free(value);value=p;length_=n;return *this;}
 String& operator=(const String& s){return *this=s.c_str();}
 const char* c_str()const{return value?value:"";}
 size_t length()const{return length_;}
 bool isEmpty()const{return !length_;}
 bool concat(const char* s){size_t n=strlen(s);value=(char*)realloc(value,length_+n+1);memcpy(value+length_,s,n+1);length_+=n;return true;}
 String& operator+=(char c){char b[]={c,0};concat(b);return *this;}
 String& operator+=(const char* s){concat(s);return *this;}
 bool operator!=(const char* s)const{return strcmp(c_str(),s)!=0;}
 bool operator==(const char* s)const{return strcmp(c_str(),s)==0;}
 char operator[](size_t i)const{return value[i];}
 const char* begin()const{return c_str();}const char* end()const{return c_str()+length_;}
};
inline String operator+(const String& a,const char* b){String s=a;s+=b;return s;}
inline String operator+(const String& a,const String& b){return a+b.c_str();}
#define IRAM_ATTR
#define PROGMEM
#define INPUT_PULLUP 0
#define OUTPUT 1
#define LOW 0
#define HIGH 1
#define FALLING 2
#define portMUX_INITIALIZER_UNLOCKED 0
using portMUX_TYPE=int;
inline void portENTER_CRITICAL(portMUX_TYPE*){} inline void portEXIT_CRITICAL(portMUX_TYPE*){}
inline void portENTER_CRITICAL_ISR(portMUX_TYPE*){} inline void portEXIT_CRITICAL_ISR(portMUX_TYPE*){}
inline int pinValues[40]={};inline void pinMode(int,int){}inline void digitalWrite(int p,int v){pinValues[p]=v;}inline int digitalPinToInterrupt(int p){return p;}
inline void attachInterrupt(int,void(*)(),int){}inline void delay(int){}
struct FakeSerial {void begin(int){} template<class... A>void printf(const char*,A...){} template<class T>void print(T){}template<class T>void println(T){}};
inline FakeSerial Serial;
