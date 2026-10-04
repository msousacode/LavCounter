#pragma once
#include "Storage.h"
#include "DeviceIdentity.h"
#include "CloudJournal.h"
#include "CloudWorker.h"
namespace lavcloud {
class CloudSync {
 CloudConfig config;DeviceIdentity& identity;EventStorage& history;CloudWorker& worker;CloudJournal journal;
 EventStorage::DurableCursor cursor;CloudJournal::Lookup lookup;JsonDocument snapshot;
 String binding,selectedId,selectedRevision;PremiumGrant grant;
 bool initialized=false,wifi=false,previousWifi=false,inFlight=false,checking=false,scanning=false,looking=false,dirty=true,countKnown=false,stopped=false;
 uint64_t generation=0,nextAttempt=0,nextPremium=0,pending=0,confirmed=0;uint32_t backoff=5;
 String lastCompleted,lastError;const char* activity="idle";
 bool usable()const{return config.valid()&&identity.valid()&&history.mounted()&&initialized;}
 bool authorized()const{return grant.allows(binding,worker.utc());}
 bool startCycle(){
  // Pin the destination only after authenticated premium activation, never while unconfigured/inactive.
  if(!identity.bind(binding,lav::SENSOR_MODE)){lastError=identity.error;stopped=true;return false;}
  if(!history.startDurable(cursor)){lastError="Histórico indisponível para envio.";stopped=true;return false;}scanning=true;looking=false;dirty=false;countKnown=false;pending=confirmed=0;generation=history.generation();activity="syncing";return true;
 }
 void defer(uint64_t now,unsigned retryAfter=0){uint32_t seconds=backoff;if(retryAfter>seconds)seconds=retryAfter;if(seconds>300)seconds=300;nextAttempt=now+(uint64_t)seconds*1000000+(now%1000000);backoff=backoff>=150?300:backoff*2;activity="backoff";}
 bool submit(bool checkOnly){CloudJob job;job.config=config;job.binding=binding;job.checkOnly=checkOnly;
  if(!checkOnly){job.name=documentName(config,identity.clientId.c_str(),identity.deviceId.c_str(),lav::SENSOR_MODE,selectedId.c_str());if(measureJson(snapshot)>MAX_SNAPSHOT||!documentBody(snapshot.as<JsonVariantConst>(),job.name,identity.clientId.c_str(),identity.deviceId.c_str(),selectedRevision,job.body)){lastError="Sessão excede limites ou formato da nuvem.";stopped=true;return false;}activity="syncing";}
  if(!worker.submit(job)){lastError="Worker da nuvem indisponível.";stopped=true;return false;}inFlight=true;checking=checkOnly;return true;
 }
public:
 CloudSync(const CloudConfig& c,DeviceIdentity& i,EventStorage& s,CloudWorker& w):config(c),identity(i),history(s),worker(w){}
 bool begin(){initialized=worker.begin();if(!initialized)lastError="Não foi possível iniciar worker da nuvem.";binding=config.binding(identity.clientId.c_str(),identity.deviceId.c_str(),lav::SENSOR_MODE);journal.setBinding(binding);return initialized;}
 const char* state()const{
  if(!identity.valid()||!initialized||stopped)return "error";
  if(!config.valid())return "not_configured";
  PremiumStatus status=grant.effective(worker.utc());if(!authorized()){if(status==PremiumStatus::Inactive)return "premium_required";if(status==PremiumStatus::Expired)return "premium_expired";return "premium_unknown";}
  if(!wifi)return "offline";return activity;
 }
 int request(uint64_t now){
  if(!usable()||!wifi)return 503;if(!authorized())return 403;if(inFlight||scanning)return 200;
  stopped=false;nextAttempt=now;backoff=5;dirty=true;return 202;
 }
 void completedAt(const String& value){lastCompleted=value;}
 void tick(uint64_t now,bool connected){
  wifi=connected;bool rising=wifi&&!previousWifi;previousWifi=wifi;
  if(history.generation()!=generation){dirty=true;countKnown=false;}
  if(rising){nextPremium=0;nextAttempt=now;dirty=true;stopped=false;grant=PremiumGrant{};}
  CloudResult result;
  if(inFlight&&worker.poll(result)){
   inFlight=false;grant=result.grant;nextPremium=now+60000000;
   if(result.outcome==Outcome::Success){lastError="";backoff=5;nextAttempt=now;
    if(checking){dirty=true;stopped=false;activity=scanning?"syncing":"idle";}
    else if(journal.confirm(selectedId,selectedRevision)){confirmed++;if(pending)pending--;looking=false;activity="syncing";}
    else{lastError=journal.error;scanning=false;looking=false;dirty=true;defer(now);}
   }else{
    lastError=result.error;scanning=false;looking=false;dirty=true;countKnown=false;
    if(result.outcome==Outcome::Permanent||result.outcome==Outcome::Unauthorized){stopped=true;activity="error";}
    else defer(now,result.retryAfter);
   }
  }
  if(inFlight||!usable()||!wifi||stopped||now<nextAttempt)return;
  if(now>=nextPremium||!authorized()){
   if(now>=nextPremium){submit(true);nextPremium=now+60000000;}
   return;
  }
  if(!scanning){if(!dirty){activity="idle";return;}if(!startCycle())return;}
  if(looking){journal.step(lookup);if(!lookup.done)return;
   if(lookup.matched){confirmed++;looking=false;return;}
   pending++;if(!authorized()){scanning=false;looking=false;dirty=true;return;}
   // Keep snapshot immutable until result; journal and storage stay on this thread.
   submit(false);return;
  }
  if(history.durableStep(cursor,snapshot)){
   if(snapshot["validated"]!=true)return;
   if(snapshot["sensorMode"]!=lav::SENSOR_MODE){lastError="Origem incompatível no histórico.";stopped=true;return;}
   selectedId=snapshot["sessionId"].as<String>();selectedRevision=revision(snapshot.as<JsonVariantConst>());journal.start(lookup,selectedId,selectedRevision);looking=true;return;
  }
  if(cursor.done){scanning=false;activity="idle";dirty=history.generation()!=generation;countKnown=!dirty&&!cursor.failed;
   if(cursor.failed){lastError="Histórico degradado; conclusão não confirmada.";stopped=true;countKnown=false;}
   else if(!dirty){lastCompleted="";completionPending=true;}
  }
 }
 bool completionPending=false;
 void describe(JsonDocument& d){
  d["configured"]=config.valid();d["clientId"]=identity.valid()?identity.clientId:String("");d["deviceId"]=identity.deviceId;d["sensorMode"]=lav::SENSOR_MODE;d["wifiConnected"]=wifi;
  d["premiumStatus"]=premiumName(grant.effective(worker.utc()));d["state"]=state();
  d["testMode"]=config.testUntil!=0;d["testUntil"]=config.testUntil;d["authUid"]=config.authUid;
  if(countKnown){d["pendingVersions"]=pending;d["confirmedVersions"]=confirmed;}else{d["pendingVersions"]=nullptr;d["confirmedVersions"]=nullptr;}
  d["cycleConfirmed"]=confirmed;d["cyclePending"]=pending;d["countsComplete"]=countKnown;
  if(lastCompleted.isEmpty())d["lastCompletedAt"]=nullptr;else d["lastCompletedAt"]=lastCompleted;
  d["lastError"]=!identity.error.isEmpty()?identity.error:!journal.error.isEmpty()?journal.error:lastError;d["storageDegraded"]=history.degraded||journal.degraded;d["premiumLinkAvailable"]=!identity.premiumLink(config).isEmpty();
 }
};
}
