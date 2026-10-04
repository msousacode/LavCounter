#pragma once
#include <ArduinoJson.h>
#include "CloudAuth.h"
#ifdef ARDUINO_ARCH_ESP32
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <esp_timer.h>
#endif
namespace lavcloud {
constexpr size_t MAX_SNAPSHOT=4096,MAX_REQUEST=12288,MAX_RESPONSE=16384;
inline bool firestoreValue(JsonVariantConst in,JsonObject out,unsigned depth=0){
 if(depth>8)return false;
 if(in.isNull())out["nullValue"]=nullptr;
 else if(in.is<bool>())out["booleanValue"]=in.as<bool>();
 else if(in.is<const char*>())out["stringValue"]=in.as<const char*>();
 else if(in.is<int64_t>()){char b[24];snprintf(b,sizeof(b),"%lld",(long long)in.as<int64_t>());out["integerValue"]=b;}
 else if(in.is<uint64_t>()){uint64_t n=in.as<uint64_t>();if(n>INT64_MAX)return false;char b[24];snprintf(b,sizeof(b),"%llu",(unsigned long long)n);out["integerValue"]=b;}
 else if(in.is<double>()){double d=in.as<double>();if(!isfinite(d))return false;out["doubleValue"]=d;}
 else if(in.is<JsonObjectConst>()){JsonObject fields=out["mapValue"]["fields"].to<JsonObject>();for(JsonPairConst p:in.as<JsonObjectConst>())if(!firestoreValue(p.value(),fields[p.key()].to<JsonObject>(),depth+1))return false;}
 else if(in.is<JsonArrayConst>()){auto a=out["arrayValue"]["values"].to<JsonArray>();for(JsonVariantConst item:in.as<JsonArrayConst>())if(!firestoreValue(item,a.add<JsonObject>(),depth+1))return false;}
 else return false;return true;
}
inline String documentName(const CloudConfig& c,const char* client,const char* device,const char* mode,const char* id){
 if(!c.valid()||!segment(client)||!segment(device)||(!strcmp(mode,"physical")?false:strcmp(mode,"mock")!=0))return "";
 if(!id||!*id||strlen(id)>20)return "";uint64_t n=0;for(const char* p=id;*p;p++){if(*p<'0'||*p>'9'||n>(UINT64_MAX-(*p-'0'))/10)return "";n=n*10+*p-'0';}if(!n)return "";
 return String("projects/")+c.projectId+"/databases/"+c.databaseId+"/documents/clients/"+client+"/devices/"+device+"/modes/"+mode+"/sessions/"+id;
}
inline bool documentBody(JsonVariantConst snapshot,const String& name,const char* client,const char* device,const String& rev,String& out){
 if(!snapshot.is<JsonObjectConst>()||snapshot["validated"]!=true||name.isEmpty())return false;
 JsonDocument d;d["name"]=name;auto fields=d["fields"].to<JsonObject>();
 for(JsonPairConst p:snapshot.as<JsonObjectConst>())if(!firestoreValue(p.value(),fields[p.key()].to<JsonObject>()))return false;
 fields["clientId"]["stringValue"]=client;fields["deviceId"]["stringValue"]=device;fields["syncRevision"]["stringValue"]=rev;fields["cloudSchemaVersion"]["integerValue"]="1";
 // Server rules must enforce nondecreasing stage, including delayed old requests.
 fields["syncStage"]["integerValue"]=snapshot["type"]=="closed"?"3":snapshot["interrupted"]==true?"2":"1";
 if(d.overflowed()||measureJson(d)>MAX_REQUEST)return false;out="";serializeJson(d,out);return !out.isEmpty();
}
struct HttpReply {int status=0;String body;bool complete=false;unsigned retryAfter=0;};
class HttpBackend {public:virtual ~HttpBackend()=default;virtual HttpReply patch(const String& url,const String& body,const AuthToken& token,const char* ca)=0;};
#ifdef ARDUINO_ARCH_ESP32
class BoundedResponse:public Stream {
public:
 String value;bool overflow=false;
 int available()override{return 0;}int read()override{return -1;}int peek()override{return -1;}void flush()override{}
 size_t write(uint8_t c)override{if(value.length()>=MAX_RESPONSE){overflow=true;return 0;}return value.concat((char)c)?1:0;}
 size_t write(const uint8_t* b,size_t n)override{for(size_t i=0;i<n;i++)if(!write(b[i]))return i;return n;}
};
class EspHttpBackend:public HttpBackend {
public:
 HttpReply patch(const String& url,const String& body,const AuthToken& token,const char* ca)override{
  HttpReply out;if(!*ca||time(nullptr)<1700000000)return out;
  WiFiClientSecure client;client.setCACert(ca);client.setHandshakeTimeout(8);HTTPClient http;
  http.setConnectTimeout(5000);http.setTimeout(8000);http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);http.setReuse(false);
  if(!http.begin(client,url))return out;
  const char* headers[]={"Retry-After"};http.collectHeaders(headers,1);http.addHeader("Authorization",String("Bearer ")+token.value);http.addHeader("Content-Type","application/json");
  out.status=http.sendRequest("PATCH",body);
  String retry=http.header("Retry-After");unsigned seconds=0;bool numeric=retry.length()>0;for(char ch:retry){if(ch<'0'||ch>'9'){numeric=false;break;}if(seconds>300){seconds=300;break;}seconds=seconds*10+ch-'0';}if(numeric)out.retryAfter=seconds>300?300:seconds;
  if(out.status>0){BoundedResponse sink;int n=http.writeToStream(&sink);out.complete=n>=0&&!sink.overflow;out.body=sink.value;}http.end();return out;
 }
};
#else
class EspHttpBackend:public HttpBackend {public:HttpReply patch(const String&,const String&,const AuthToken&,const char*)override{return {};}};
#endif
enum class Outcome {Success,Transient,Permanent,Unauthorized,PremiumBlocked};
struct CloudResult {Outcome outcome=Outcome::Transient;PremiumGrant grant;unsigned retryAfter=0;const char* error="Falha de conexão com a nuvem.";bool checkOnly=false;};
class FirestoreTransport {
 AuthProvider* auth;PremiumProvider* premium;HttpBackend& http;
 uint64_t currentUtc(uint64_t fallback)const{
#ifdef ARDUINO_ARCH_ESP32
  return (uint64_t)time(nullptr);
#else
  return fallback;
#endif
 }
public:
 FirestoreTransport(AuthProvider& a,PremiumProvider& p,HttpBackend& h):auth(&a),premium(&p),http(h){}
 void providers(AuthProvider& a,PremiumProvider& p){auth=&a;premium=&p;}
 CloudResult execute(const CloudConfig& c,const String& binding,uint64_t utc,const String& name,const String& body,bool checkOnly){
  CloudResult r;r.checkOnly=checkOnly;if(!c.valid()||utc<1700000000){r.error="Configuração TLS ou hora UTC indisponível.";return r;}
  for(unsigned attempt=0;attempt<2;attempt++){
   utc=currentUtc(utc);
   AuthToken token;
   if(!auth->acquire(utc,attempt==1,token)){r.outcome=attempt==1?Outcome::Unauthorized:Outcome::Transient;r.error="Autenticação da nuvem indisponível.";return r;}
   if(!token.valid(utc)){r.outcome=Outcome::Unauthorized;r.error="Token da nuvem sem validade confirmada.";return r;}
   if(!premium->check(c,token,binding,utc,r.grant)){r.grant=PremiumGrant{};r.outcome=Outcome::PremiumBlocked;r.error="Não foi possível confirmar o premium.";return r;}
   utc=currentUtc(utc);
   if(!token.valid(utc)||!r.grant.allows(binding,utc)){r.outcome=Outcome::PremiumBlocked;r.error="Premium ou vínculo sem autorização válida.";return r;}
   if(checkOnly){r.outcome=Outcome::Success;r.error="";return r;}
   HttpReply reply=http.patch(String("https://firestore.googleapis.com/v1/")+name,body,token,c.caPem);r.retryAfter=reply.retryAfter;
   if(reply.status==401&&attempt==0)continue;
   if(reply.status==401||reply.status==403){r.outcome=Outcome::Unauthorized;r.error="A nuvem recusou a autorização.";return r;}
   if(reply.status==429||reply.status>=500||reply.status<=0||!reply.complete){r.error="Envio não confirmado; aguardando nova tentativa.";return r;}
   if(reply.status>=200&&reply.status<300){
    JsonDocument d;if(reply.body.length()<=MAX_RESPONSE&&!deserializeJson(d,reply.body)&&d["name"]==name.c_str()){
     JsonDocument request;if(!deserializeJson(request,body)&&d["fields"]["syncRevision"]["stringValue"]==request["fields"]["syncRevision"]["stringValue"]){r.outcome=Outcome::Success;r.error="";return r;}
    }r.error="Resposta da nuvem inválida ou incompleta.";return r;
   }
   r.outcome=Outcome::Permanent;r.error="A nuvem recusou o formato da sessão.";return r;
  }return r;
 }
};
}
