#include <assert.h>
#include <stdio.h>
#include "../firmware/LavCounter/DeviceIdentity.h"
#include "../firmware/LavCounter/CloudAuth.h"
#include "../firmware/LavCounter/CloudJournal.h"
#include "../firmware/LavCounter/Storage.h"
#include "../firmware/LavCounter/CloudSync.h"
#include "../firmware/LavCounter/CloudNetwork.h"
using namespace lavcloud;
void randomA(uint8_t* b,size_t n){for(size_t i=0;i<n;i++)b[i]=(uint8_t)i;}
void randomB(uint8_t* b,size_t n){for(size_t i=0;i<n;i++)b[i]=(uint8_t)(i+30);}
void resetIdentity(){for(auto& e:Preferences::identity)e=Preferences::Entry{};Preferences::fail=false;}
struct FakeAuth:AuthProvider {
 bool available=true,expired=false,refreshAllowed=true;unsigned calls=0,refreshes=0;
 bool acquire(uint64_t utc,bool refresh,AuthToken& out)override{calls++;if(refresh)refreshes++;if(!available||(refresh&&!refreshAllowed))return false;out.value="secret-test-token";out.expiresAt=expired?utc:utc+3600;return true;}
};
struct FakePremium:PremiumProvider {
 bool available=true,mismatch=false;PremiumStatus status=PremiumStatus::Active;uint64_t ttl=600;unsigned calls=0;
 bool check(const CloudConfig&,const AuthToken&,const String& bind,uint64_t utc,PremiumGrant& out)override{calls++;out.status=status;out.binding=mismatch?String("other-client"):bind;out.expiresAt=utc+ttl;return available;}
};
struct FakeHttp:HttpBackend {
 int code=200;bool complete=true,invalid=false,first401=false;unsigned calls=0;String lastName,lastBody;unsigned retryAfter=0;
 bool guardStage=false;int remoteStage=0;String remoteRevision;
 HttpReply patch(const String& url,const String& body,const AuthToken& token,const char* ca)override{
  assert(!strncmp(url.c_str(),"https://firestore.googleapis.com/v1/",36));assert(token.value=="secret-test-token");assert(*ca);calls++;lastBody=body;JsonDocument d;assert(!deserializeJson(d,body));lastName=d["name"].as<String>();
  HttpReply out;out.status=first401&&calls==1?401:code;
  if(guardStage){const char* stageText=d["fields"]["syncStage"]["integerValue"]|"0";int stage=atoi(stageText);String rev=d["fields"]["syncRevision"]["stringValue"].as<String>();if(stage<remoteStage||(stage==remoteStage&&rev!=remoteRevision.c_str()))out.status=403;else{remoteStage=stage;remoteRevision=rev;}}
  out.complete=complete;out.retryAfter=retryAfter;out.body=invalid?String("{}"):body;return out;
 }
};
struct ManualWorker:CloudWorker {
 FirestoreTransport& transport;CloudJob job;bool occupied=false,ready=false,beginOk=true,submitOk=true;unsigned submits=0;uint64_t clock=1800000000;CloudResult result;
 explicit ManualWorker(FirestoreTransport& t):transport(t){}
 bool begin()override{return beginOk;}uint64_t utc()const override{return clock;}
 bool submit(const CloudJob& j)override{if(occupied||!submitOk)return false;job=j;occupied=true;submits++;return true;}
 bool poll(CloudResult& r)override{if(!ready)return false;r=result;ready=false;occupied=false;return true;}
 void finish(){assert(occupied);result=transport.execute(job.config,job.binding,clock,job.name,job.body,job.checkOnly);ready=true;}
};
int main(){
 CloudConfig config;assert(!config.valid());config.enabled=true;config.projectId="demo-project";config.premiumUrl="https://saas.example/premium";config.caPem="test-only";assert(config.valid());config.projectId="../other";assert(!config.valid());config.projectId="demo-project";
 const char* bad[]={"http://saas.example","https://","https://user:secret@saas.example","https://saas.example/?token=secret","https://saas.example/#x","https://saas.example/\n"};for(auto url:bad)assert(!httpsUrl(url));assert(!segment(".."));assert(encode("a b&/")=="a%20b%26%2F");
 const uint8_t mac[]={1,2,3,4,5,6};resetIdentity();DeviceIdentity a;assert(a.begin(mac,randomA));assert(uuid(a.clientId.c_str()));assert(a.deviceId=="esp32-010203040506");String first=a.clientId;
 DeviceIdentity reboot;assert(reboot.begin(mac,randomB));assert(reboot.clientId==first.c_str());assert(Preferences::legacy.isEmpty());assert(a.bind(config.binding(a.clientId.c_str(),a.deviceId.c_str(),"physical"),"physical"));assert(a.bind(config.binding(a.clientId.c_str(),a.deviceId.c_str(),"mock"),"mock"));assert(!a.bind("different-client","physical"));
 config.signupUrl="https://saas.example/cadastro";String link=a.premiumLink(config);assert(link== (String(config.signupUrl)+"?clientId="+first+"&deviceId=esp32-010203040506").c_str());
 resetIdentity();DeviceIdentity b;assert(b.begin(mac,randomB));assert(b.clientId!=first.c_str());Preferences::identity[0].value="corrupt";DeviceIdentity corrupt;assert(!corrupt.begin(mac,randomA));assert(Preferences::identity[0].value=="corrupt");resetIdentity();Preferences::fail=true;DeviceIdentity failed;assert(!failed.begin(mac,randomA));assert(failed.premiumLink(config).isEmpty());resetIdentity();
 UnavailableAuth auth;AuthToken token;assert(!auth.acquire(1800000000,false,token));assert(!token.valid(1800000000));token.value="fake-token";token.expiresAt=1800000100;assert(token.valid(1800000000));assert(!token.valid(1800000100));
 UnavailablePremium premium;PremiumGrant grant;assert(!premium.check(config,token,"binding",1800000000,grant));assert(!grant.allows("binding",1800000000));grant.status=PremiumStatus::Active;grant.binding="binding";grant.expiresAt=1800000100;assert(grant.allows("binding",1800000000));assert(!grant.allows("other-client",1800000000));assert(grant.effective(1800000100)==PremiumStatus::Expired);grant.status=PremiumStatus::Inactive;assert(!grant.allows("binding",1800000000));
 puts("PASS cloud identity/config/auth/premium: UUID stability, isolated bindings, fail closed, URL, expiry");
 assert(digest("abc")=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");assert(digest("")=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
 JsonDocument x,y;deserializeJson(x,"{\"b\":2,\"a\":{\"z\":null,\"c\":true}}");deserializeJson(y,"{\"a\":{\"c\":true,\"z\":null},\"b\":2}");assert(revision(x.as<JsonVariantConst>())==revision(y.as<JsonVariantConst>()).c_str());
 EventStorage store("/presence-v2.jsonl");assert(store.begin(nullptr));Event event;event.id=9007199254740993ULL;event.validated=true;assert(store.enqueue(event));EventStorage::DurableCursor cur;assert(store.startDurable(cur));JsonDocument snapshot;while(!cur.done)assert(!store.durableStep(cur,snapshot));store.tick(5000000);assert(store.generation()==1);
 assert(store.startDurable(cur));event.closed=true;event.closedAt="2026-10-03T12:00:00";assert(store.enqueue(event));store.tick(10000000);unsigned emitted=0;while(!cur.done)if(store.durableStep(cur,snapshot)){emitted++;assert(snapshot["type"]=="validated");}assert(emitted==1);String before=revision(snapshot.as<JsonVariantConst>());
 CloudJournal journal("/cloud-v1.jsonl");journal.setBinding("test-binding");assert(journal.confirm("9007199254740993",before));CloudJournal::Lookup lookup;journal.start(lookup,"9007199254740993",before);while(!lookup.done)journal.step(lookup);assert(lookup.matched);
 assert(store.startDurable(cur));while(!cur.done)store.durableStep(cur,snapshot);assert(snapshot["type"]=="closed");String after=revision(snapshot.as<JsonVariantConst>());assert(after!=before.c_str());journal.start(lookup,"9007199254740993",after);while(!lookup.done)journal.step(lookup);assert(!lookup.matched);
 cloudDisk.budget=10;assert(!journal.confirm("9007199254740993",after));cloudDisk.budget=100000;assert(journal.confirm("9007199254740993",after));journal.start(lookup,"9007199254740993",after);while(!lookup.done)journal.step(lookup);assert(!lookup.matched&&journal.degraded);
 cloudDisk=FakeDisk{};assert(journal.confirm("9007199254740993",after));CloudJournal restarted("/cloud-v1.jsonl");restarted.setBinding("test-binding");restarted.start(lookup,"9007199254740993",after);while(!lookup.done)restarted.step(lookup);assert(lookup.matched);cloudDisk.failOpen=true;restarted.start(lookup,"9007199254740993",after);assert(lookup.done&&lookup.failed&&!lookup.matched);cloudDisk.failOpen=false;
 EventStorage boot("/presence-v2.jsonl");assert(boot.begin(nullptr));event.id++;event.closed=false;event.closedAt="";store.enqueue(event);store.tick(15000000);EventStorage interrupted("/presence-v2.jsonl");assert(interrupted.begin(nullptr));interrupted.startDurable(cur);while(!cur.done)interrupted.durableStep(cur,snapshot);assert(snapshot["interrupted"]==true);
 puts("PASS cloud durable source and journal: SHA vectors, canonical snapshots, RAM exclusion, captured boundary, close revision, reboot, corruption");
 disk.bytes+="{\"schemaVersion\":2";EventStorage truncated("/presence-v2.jsonl");truncated.begin(nullptr);truncated.startDurable(cur);while(!cur.done)truncated.durableStep(cur,snapshot);assert(cur.failed&&truncated.degraded);disk=FakeDisk{};
 FakeAuth fakeAuth;FakePremium fakePremium;FakeHttp http;FirestoreTransport transport(fakeAuth,fakePremium,http);
 config.projectId="demo-project";config.ssid="shop-wifi";config.wifiPassword="private";
 String name=documentName(config,"client-1","esp32-010203040506","physical","9007199254740993");assert(!name.isEmpty());assert(documentName(config,"../client","device","physical","1").isEmpty());assert(documentName(config,"client","device","physical","18446744073709551616").isEmpty());
 snapshot["durationMs"]=9007199254740993ULL;snapshot["startedAt"]=nullptr;snapshot["type"]="closed";snapshot["interrupted"]=false;String body;assert(documentBody(snapshot.as<JsonVariantConst>(),name,"client-1","esp32-010203040506",after,body));JsonDocument request;deserializeJson(request,body);assert(request["fields"]["durationMs"]["integerValue"]=="9007199254740993");assert(request["fields"]["startedAt"]["nullValue"].isNull());assert(request["fields"]["config"]["mapValue"]["fields"]["revision"]["integerValue"]=="1");assert(request["fields"]["syncStage"]["integerValue"]=="3");
 auto run=[&](){return transport.execute(config,"binding",1800000000,name,body,false);};assert(run().outcome==Outcome::Success);
 fakeAuth.available=false;unsigned beforeAuthFailure=http.calls;assert(run().outcome==Outcome::Transient&&http.calls==beforeAuthFailure);fakeAuth.available=true;
 http.code=401;http.first401=true;http.calls=0;http.code=200;assert(run().outcome==Outcome::Success&&http.calls==2&&fakeAuth.refreshes==1);http.calls=0;fakeAuth.refreshAllowed=false;assert(run().outcome==Outcome::Unauthorized);fakeAuth.refreshAllowed=true;http.first401=false;
 const int codes[]={400,403,429,500};for(int code:codes){http.code=code;http.retryAfter=120;auto result=run();assert(result.outcome==(code==400?Outcome::Permanent:code==403?Outcome::Unauthorized:Outcome::Transient));if(code==429)assert(result.retryAfter==120);}
 http.code=200;http.complete=false;assert(run().outcome==Outcome::Transient);http.complete=true;http.invalid=true;assert(run().outcome==Outcome::Transient);http.invalid=false;
 unsigned calls=http.calls;fakePremium.status=PremiumStatus::Inactive;assert(run().outcome==Outcome::PremiumBlocked&&http.calls==calls);fakePremium.status=PremiumStatus::Unknown;assert(run().outcome==Outcome::PremiumBlocked);fakePremium.status=PremiumStatus::Expired;assert(run().outcome==Outcome::PremiumBlocked);fakePremium.status=PremiumStatus::Active;fakePremium.mismatch=true;assert(run().outcome==Outcome::PremiumBlocked);fakePremium.mismatch=false;fakePremium.ttl=0;assert(run().outcome==Outcome::PremiumBlocked);fakePremium.ttl=600;fakeAuth.expired=true;assert(run().outcome==Outcome::Unauthorized);fakeAuth.expired=false;config.caPem="";assert(run().outcome==Outcome::Transient);config.caPem="test-only";assert(transport.execute(config,"binding",0,name,body,false).outcome==Outcome::Transient);
 // Server contract rejects delayed old validation after terminal close.
 http.guardStage=true;http.remoteStage=0;assert(run().outcome==Outcome::Success);String closedBody=body;deserializeJson(request,body);request["fields"]["syncStage"]["integerValue"]="1";request["fields"]["syncRevision"]["stringValue"]="old";body="";serializeJson(request,body);assert(run().outcome==Outcome::Unauthorized);body=closedBody;http.guardStage=false;
 CloudNetwork network;WiFi=FakeWiFi{};network.begin(config);assert(WiFi.modeValue==WIFI_AP_STA&&WiFi.aps==1);network.tick(0);unsigned tries=WiFi.begins;network.tick(1);assert(WiFi.begins==tries);WiFi.statusValue=WL_CONNECTED;assert(network.tick(1000000)&&network.connected());assert(!network.tick(2000000));WiFi.statusValue=0;network.tick(3000000);assert(!network.connected());CloudConfig disabled;network.begin(disabled);assert(WiFi.modeValue==WIFI_AP&&!network.connected());
 puts("PASS transport/network: exact Firestore types, idempotent paths, auth refresh, status errors, TLS/time fail closed, AP+STA");
 resetIdentity();DeviceIdentity unit;assert(unit.begin(mac,randomA));disk=FakeDisk{};cloudDisk=FakeDisk{};EventStorage sessions("/presence-v2.jsonl");assert(sessions.begin(nullptr));event=Event{};event.id=1;event.validated=true;sessions.enqueue(event);sessions.tick(5000000);
 ManualWorker worker(transport);CloudSync sync(config,unit,sessions,worker);assert(sync.begin());uint64_t now=0;
 auto tick=[&](){now+=1000;sync.tick(now,true);};
 fakePremium.status=PremiumStatus::Inactive;tick();assert(worker.occupied&&worker.job.checkOnly);worker.finish();tick();assert(!strcmp(sync.state(),"premium_required"));assert(sync.request(now)==403);calls=http.calls;for(unsigned i=0;i<100;i++)tick();assert(http.calls==calls);Preferences bindingCheck;bindingCheck.begin("lavIdentity",false);assert(!bindingCheck.isKey("realBinding"));
 fakePremium.status=PremiumStatus::Active;now=61000000;tick();worker.finish();tick();for(unsigned i=0;i<100&&!worker.occupied;i++)tick();assert(worker.occupied&&!worker.job.checkOnly);assert(sync.request(now)==200);
 // While remote work is stalled, persistence continues; captured validation stays immutable.
 String captured=worker.job.body;event.closed=true;event.closedAt="2026-10-03T13:00:00";sessions.enqueue(event);sessions.tick(70000000);assert(sessions.queued()==0);for(unsigned i=0;i<100;i++)tick();assert(worker.job.body==captured.c_str());worker.finish();tick();
 for(unsigned i=0;i<1000;i++){if(worker.occupied&&!worker.ready)worker.finish();tick();}JsonDocument state;sync.describe(state);assert(state["pendingVersions"]==0&&state["confirmedVersions"]==1);deserializeJson(request,http.lastBody);assert(request["fields"]["type"]["stringValue"]=="closed");assert(sync.completionPending);
 // Fresh coordinator + markers must not reupload the already-confirmed closed snapshot.
 EventStorage rebootSessions("/presence-v2.jsonl");rebootSessions.begin(nullptr);ManualWorker worker2(transport);CloudSync rebootSync(config,unit,rebootSessions,worker2);rebootSync.begin();calls=http.calls;
 for(unsigned i=0;i<1000;i++){rebootSync.tick(++now,true);if(worker2.occupied&&!worker2.ready)worker2.finish();}assert(http.calls==calls);
 // New session, server success but no local marker: reboot safely resends it.
 event.id=2;event.closed=true;sessions.enqueue(event);sessions.tick(80000000);for(unsigned i=0;i<1000&&!worker.occupied;i++)tick();assert(worker.occupied);worker.finish();cloudDisk.budget=0;tick();assert(!cloudDisk.bytes.isEmpty());cloudDisk.budget=100000;
 EventStorage rebootAgain("/presence-v2.jsonl");rebootAgain.begin(nullptr);ManualWorker worker3(transport);CloudSync resumed(config,unit,rebootAgain,worker3);resumed.begin();calls=http.calls;
 for(unsigned i=0;i<1500;i++){resumed.tick(++now,true);if(worker3.occupied&&!worker3.ready)worker3.finish();}assert(http.calls==calls+1);resumed.describe(state);assert(state["confirmedVersions"]==2);
 fakePremium.status=PremiumStatus::Inactive;now+=61000000;resumed.tick(now,true);worker3.finish();resumed.tick(++now,true);assert(resumed.request(now)==403);assert(!strcmp(resumed.state(),"premium_required"));
 ManualWorker unavailable(transport);unavailable.beginOk=false;CloudSync noWorker(config,unit,sessions,unavailable);assert(!noWorker.begin());assert(!strcmp(noWorker.state(),"error"));
 ManualWorker full(transport);full.submitOk=false;CloudSync noSubmit(config,unit,sessions,full);assert(noSubmit.begin());noSubmit.tick(1,true);assert(!strcmp(noSubmit.state(),"error"));
 puts("PASS coordinator: premium gate, activation backlog, serialized worker, close during upload, reboot markers, marker failure recovery, revocation");
}
