#pragma once
#include <ArduinoJson.h>
#include <Arduino.h>
namespace lavcloud {
// SHA-256, FIPS 180-4. Same implementation on host and device for journal integrity.
class Sha256 {
 uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
 uint8_t block[64];size_t used=0;uint64_t bytes=0;
 static uint32_t r(uint32_t x,unsigned n){return (x>>n)|(x<<(32-n));}
 void compress(){
  static const uint32_t k[64]={0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
  uint32_t w[64];for(unsigned i=0;i<16;i++)w[i]=((uint32_t)block[4*i]<<24)|((uint32_t)block[4*i+1]<<16)|((uint32_t)block[4*i+2]<<8)|block[4*i+3];
  for(unsigned i=16;i<64;i++)w[i]=w[i-16]+(r(w[i-15],7)^r(w[i-15],18)^(w[i-15]>>3))+w[i-7]+(r(w[i-2],17)^r(w[i-2],19)^(w[i-2]>>10));
  uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],v=h[7];
  for(unsigned i=0;i<64;i++){uint32_t t1=v+(r(e,6)^r(e,11)^r(e,25))+((e&f)^(~e&g))+k[i]+w[i];uint32_t t2=(r(a,2)^r(a,13)^r(a,22))+((a&b)^(a&c)^(b&c));v=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;}
  h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=v;
 }
public:
 void add(uint8_t c){block[used++]=c;bytes++;if(used==64){compress();used=0;}}
 void add(const char* s){for(;*s;s++)add((uint8_t)*s);}
 String finish(){uint64_t bits=bytes*8;add((uint8_t)128);while(used!=56)add((uint8_t)0);for(int i=7;i>=0;i--)add((uint8_t)(bits>>(8*i)));char out[65];for(unsigned i=0;i<8;i++)snprintf(out+8*i,9,"%08lx",(unsigned long)h[i]);return out;}
};
inline String digest(const String& text){Sha256 s;s.add(text.c_str());return s.finish();}
// Object keys sorted recursively; arrays retain order; primitives use JSON encoding.
inline void canonical(JsonVariantConst value,String& out){
 if(value.is<JsonObjectConst>()){
  out+='{';const char* previous=nullptr;bool first=true;auto object=value.as<JsonObjectConst>();
  for(size_t i=0;i<object.size();i++){const char* next=nullptr;for(JsonPairConst p:object){const char* key=p.key().c_str();if((!previous||strcmp(key,previous)>0)&&(!next||strcmp(key,next)<0))next=key;}
   if(!next)break;if(!first)out+=',';first=false;JsonDocument key;key.set(next);String encoded;serializeJson(key,encoded);out+=encoded.c_str();out+=':';canonical(object[next],out);previous=next;
  }out+='}';
 }else if(value.is<JsonArrayConst>()){out+='[';bool first=true;for(JsonVariantConst item:value.as<JsonArrayConst>()){if(!first)out+=',';first=false;canonical(item,out);}out+=']';}
 else {String encoded;serializeJson(value,encoded);out+=encoded.c_str();}
}
inline String revision(JsonVariantConst snapshot){String out;canonical(snapshot,out);return digest(out);}
}
