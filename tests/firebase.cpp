#include <assert.h>
#include <stdio.h>
#include "../firmware/LavCounter/FirebaseTest.h"
using namespace lavcloud;
struct FakeTokenHttp:TokenHttp {
 unsigned calls=0;int status=200;bool complete=true;String response;
 HttpReply post(const String& url,const String& form,const char* ca)override{
  calls++;assert(url=="https://securetoken.googleapis.com/v1/token?key=test-key");assert(*ca);
  if(calls==1)assert(form=="grant_type=refresh_token&refresh_token=seed%2B%2F%3D");
  else assert(form=="grant_type=refresh_token&refresh_token=rotated");
  HttpReply r;r.status=status;r.complete=complete;r.body=response;return r;
 }
 void valid(){response="{\"user_id\":\"test-uid\",\"project_id\":\"957282795652\",\"token_type\":\"Bearer\",\"expires_in\":\"3600\",\"id_token\":\"id-token\",\"refresh_token\":\"rotated\"}";}
};
int main(){
 const uint64_t now=1800000000;FakeTokenHttp http;http.valid();
 FirebaseRefreshAuth auth(http,"test-key","test-ca","seed+/=","test-uid","957282795652");AuthToken token;
 assert(!auth.acquire(0,false,token)&&http.calls==0);
 assert(auth.acquire(now,false,token)&&http.calls==1&&token.expiresAt==now+3600);
 assert(auth.acquire(now+3500,false,token)&&http.calls==1); // cached until expiry margin
 assert(auth.acquire(now+3570,false,token)&&http.calls==2); // renew at the 30s margin
 assert(auth.acquire(now+3571,true,token)&&http.calls==3); // force refresh on 401
 http.status=403;assert(!auth.acquire(now+3572,true,token)&&token.value.isEmpty());
 http.status=200;http.complete=false;assert(!auth.acquire(now+3572,false,token));http.complete=true;
 const char* invalid[]={"{}","{\"user_id\":\"other-uid\",\"project_id\":\"957282795652\"}",
  "{\"user_id\":\"test-uid\",\"project_id\":\"other-project\"}"};
 for(auto body:invalid){http.response=body;assert(!auth.acquire(now+3572,false,token));}
 const char* fields[]={"user_id","project_id"};
 for(auto field:fields){
  JsonDocument d;http.valid();deserializeJson(d,http.response);d[field]="other";serializeJson(d,http.response);
  assert(!auth.acquire(now+3572,false,token));
 }
 const char* ttls[]={"0","30","3601","-1","1.5","garbage","999999999"};
 for(auto ttl:ttls){
  JsonDocument d;http.valid();deserializeJson(d,http.response);d["expires_in"]=ttl;serializeJson(d,http.response);
  assert(!auth.acquire(now+3572,false,token));
 }
 http.valid();assert(auth.acquire(now+3572,false,token));
 JsonDocument d;deserializeJson(d,http.response);d["refresh_token"]="";serializeJson(d,http.response);assert(!auth.acquire(now+3572,true,token));
 http.valid();deserializeJson(d,http.response);d["id_token"]="";serializeJson(d,http.response);assert(!auth.acquire(now+3572,true,token));
 http.valid();deserializeJson(d,http.response);d["token_type"]="other";serializeJson(d,http.response);assert(!auth.acquire(now+3572,true,token));
 http.valid();deserializeJson(d,http.response);d["expires_in"]=3600;serializeJson(d,http.response);assert(!auth.acquire(now+3572,true,token));
 puts("PASS Firebase Auth: TLS/time/config gates, form escaping, cached token, expiry, forced refresh, rotation, UID/project validation, malformed/failed responses");
 DeviceIdentity identity;assert(identity.begin());CloudConfig c;c.enabled=true;c.projectId="lavcounter";c.caPem="test-ca";c.authUid="test-uid";
 assert(!c.valid());c.testUntil=now+100;assert(c.valid());
 TemporaryTestPremium premium("lavcounter","(default)",now+100,identity);PremiumGrant grant;
 String binding=c.binding(identity.clientId.c_str(),identity.deviceId.c_str(),lav::SENSOR_MODE);
 token.value="valid-token";token.expiresAt=now+3600;
 assert(premium.check(c,token,binding,now,grant)&&grant.allows(binding,now));
 assert(!premium.check(c,token,"lavcounter/(default)/other-client/device/mock",now,grant));
 c.projectId="other-project";assert(!premium.check(c,token,binding,now,grant));c.projectId="lavcounter";
 c.databaseId="other-db";assert(!premium.check(c,token,binding,now,grant));c.databaseId="(default)";
 c.testUntil=now+101;assert(!premium.check(c,token,binding,now,grant));c.testUntil=now+100;
 assert(premium.check(c,token,binding,now+100,grant)&&grant.status==PremiumStatus::Expired&&!grant.allows(binding,now+100));
 token.expiresAt=now;assert(!premium.check(c,token,binding,now,grant));
 c.testUntil=0;assert(!c.valid()&&!premium.check(c,token,binding,now,grant));
 puts("PASS temporary test grant: explicit opt-in, real token required, exact device/mode/project/database binding, fixed expiry, production default denied");
}
