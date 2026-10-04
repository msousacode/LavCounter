// Private Firebase test wiring, with deliberately unusable fixture credentials.
// The runner excludes CloudPrivate.h, so no real credentials enter native tests.
#define LAVCOUNTER_FIREBASE_TEST_ENABLED 1
#define LAVCOUNTER_FIREBASE_API_KEY "fixture-key"
#define LAVCOUNTER_FIREBASE_CA "fixture-ca"
#define LAVCOUNTER_FIREBASE_REFRESH_TOKEN "fixture-refresh-secret"
#define LAVCOUNTER_FIREBASE_AUTH_UID "fixture-uid"
#define LAVCOUNTER_FIREBASE_PROJECT_NUMBER "957282795652"
#define LAVCOUNTER_FIREBASE_TEST_UNTIL 1791676800ULL
#define LAVCOUNTER_CLOUD_CONFIGURE(c) do { (c).enabled=true; (c).projectId="lavcounter"; (c).testUntil=LAVCOUNTER_FIREBASE_TEST_UNTIL; (c).authUid=LAVCOUNTER_FIREBASE_AUTH_UID; (c).caPem=LAVCOUNTER_FIREBASE_CA; } while(0)
#include <assert.h>
#include <stdio.h>
#include "../firmware/LavCounter/LavCounter.ino"
int main(){
 setup();for(unsigned i=0;i<server.routeCount;i++){server.allowed=false;server.routes[i].handler();assert(server.code==401);}server.allowed=true;
 getCloudSync();JsonDocument d;assert(server.code==200&&!deserializeJson(d,server.response));
 assert(d["configured"]==true&&d["testMode"]==true&&d["testUntil"]==1791676800ULL);
 assert(d["authUid"]=="fixture-uid"&&d["premiumStatus"]=="unknown"&&d["state"]=="premium_unknown");
 assert(strstr(server.response.c_str(),"fixture-refresh-secret")==nullptr&&strstr(server.response.c_str(),"fixture-key")==nullptr);
 lavcloud::AuthToken token;assert(!defaultCloudAuth.acquire(1800000000,false,token)); // native HTTPS stub cannot authorize
 server.plain="{\"testMode\":true}";requestCloudSync();assert(server.code==400);
 server.plain="{\"testUntil\":1999999999}";requestCloudSync();assert(server.code==400);
 server.plain="{}";requestCloudSync();assert(server.code==503); // no internet/token, even with override selected
 getPremiumLink();assert(server.code==503); // no signup/purchase dependency
 getStatus();assert(server.code==200&&!deserializeJson(d,server.response));assert(d["source"]=="local");
 puts("PASS Firebase firmware wiring: selected Auth/test providers, authenticated public diagnostics, no credential disclosure, no override by API, offline fail closed, local operation preserved");
}
