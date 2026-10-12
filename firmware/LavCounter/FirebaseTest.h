#pragma once
#include "FirestoreTransport.h"
#include "DeviceIdentity.h"
#include "BuildConfig.h"

namespace lavcloud {
// Provision a dedicated anonymous test user once from the PC, never each boot.
// Its refresh token stays in the ignored private header; no SaaS password or admin key.
class TokenHttp {
public:
 virtual ~TokenHttp()=default;
 virtual HttpReply post(const String& url,const String& form,const char* ca)=0;
};
class EspTokenHttp:public TokenHttp {
public:
 HttpReply post(const String& url,const String& form,const char* ca)override{
  HttpReply out;
#ifdef ARDUINO_ARCH_ESP32
  if(!*ca||time(nullptr)<1700000000||form.length()>8192)return out;
  WiFiClientSecure client;client.setCACert(ca);client.setHandshakeTimeout(8);HTTPClient http;
  http.setConnectTimeout(5000);http.setTimeout(8000);http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);http.setReuse(false);
  if(!http.begin(client,url))return out;
  http.addHeader("Content-Type","application/x-www-form-urlencoded");out.status=http.sendRequest("POST",form);
  if(out.status>0){BoundedResponse sink;int n=http.writeToStream(&sink);out.complete=n>=0&&!sink.overflow;out.body=sink.value;}
  http.end();
#endif
  return out;
 }
};

class FirebaseRefreshAuth:public AuthProvider {
 TokenHttp& http;const char* apiKey;const char* ca;const char* uid;const char* projectNumber;
 String refreshToken;AuthToken cached;
public:
 FirebaseRefreshAuth(TokenHttp& h,const char* key,const char* root,const char* refresh,const char* user,const char* number)
  :http(h),apiKey(key),ca(root),uid(user),projectNumber(number),refreshToken(refresh){}
 bool acquire(uint64_t utc,bool refresh,AuthToken& out)override{
  out=AuthToken{};
  if(utc<1700000000||!*apiKey||!*ca||!*uid||!*projectNumber||refreshToken.isEmpty()||refreshToken.length()>4096)return false;
  if(!refresh&&cached.valid(utc)){out=cached;return true;}
  cached=AuthToken{};
  HttpReply reply=http.post(String("https://securetoken.googleapis.com/v1/token?key=")+encode(apiKey),
    String("grant_type=refresh_token&refresh_token=")+encode(refreshToken.c_str()),ca);
  if(reply.status!=200||!reply.complete||reply.body.length()>MAX_RESPONSE)return false;
  JsonDocument d;if(deserializeJson(d,reply.body))return false;
  // project_id here is Google's project NUMBER, not Firebase's textual project ID.
  if(d["user_id"]!=uid||d["project_id"]!=projectNumber||d["token_type"]!="Bearer"||!d["id_token"].is<const char*>()||!d["refresh_token"].is<const char*>()||!d["expires_in"].is<const char*>())return false;
  const char* ttl=d["expires_in"];uint64_t seconds=0;
  if(!*ttl||strlen(ttl)>5)return false;
  for(const char* p=ttl;*p;p++){if(*p<'0'||*p>'9')return false;seconds=seconds*10+*p-'0';}
  if(seconds<=30||seconds>3600||utc>UINT64_MAX-seconds)return false;
  AuthToken next;next.value=d["id_token"].as<const char*>();next.expiresAt=utc+seconds;
  const char* renewed=d["refresh_token"];if(!*renewed||strlen(renewed)>4096||!next.valid(utc))return false;
  refreshToken=renewed;cached=next;out=cached;return true;
 }
};

// Local permission to attempt uploads; Firestore rules authorize every write.
class FirebaseManagedPremium:public PremiumProvider {
 const char* project;const char* database;const DeviceIdentity& identity;
public:
 FirebaseManagedPremium(const char* p,const char* d,const DeviceIdentity& i):project(p),database(d),identity(i){}
 bool check(const CloudConfig& c,const AuthToken& token,const String& binding,uint64_t utc,PremiumGrant& out)override{
  out=PremiumGrant{};
  if(!c.firebaseManagedPremium||!c.valid()||!token.valid(utc)||strcmp(c.projectId,project)||strcmp(c.databaseId,database)||!identity.valid())return false;
  if(binding!=c.binding(identity.clientId.c_str(),identity.deviceId.c_str(),lav::SENSOR_MODE).c_str())return false;
  out.binding=binding;out.expiresAt=UINT64_MAX;out.status=PremiumStatus::Active;return true;
 }
};

class TemporaryTestPremium:public PremiumProvider {
 const char* project;const char* database;uint64_t until;const DeviceIdentity& identity;
public:
 TemporaryTestPremium(const char* p,const char* d,uint64_t expiry,const DeviceIdentity& i):project(p),database(d),until(expiry),identity(i){}
 bool check(const CloudConfig& c,const AuthToken& token,const String& binding,uint64_t utc,PremiumGrant& out)override{
  out=PremiumGrant{};
  if(!c.valid()||!token.valid(utc)||strcmp(c.projectId,project)||strcmp(c.databaseId,database)||c.testUntil!=until||until<1700000000)return false;
  // The override is restricted to this installation, project, database and sensor mode.
  if(!identity.valid()||binding!=c.binding(identity.clientId.c_str(),identity.deviceId.c_str(),lav::SENSOR_MODE).c_str())return false;
  out.binding=binding;out.expiresAt=until;out.status=utc<until?PremiumStatus::Active:PremiumStatus::Expired;return true;
 }
};
}
