#include <assert.h>
#include <stdio.h>
#include "../firmware/LavCounter/Storage.h"
int main(){
 legacyDisk.bytes="preserved-old-history";Preferences::legacy="legacy-config";ConfigStore cfg;assert(cfg.begin());assert(cfg.current.v[lav::WASH_SECONDS]==120);auto c=cfg.current;c.v[lav::WASH_SECONDS]=60;assert(cfg.save(c));ConfigStore reboot;assert(reboot.begin());assert(reboot.current.v[lav::WASH_SECONDS]==60);assert(reboot.current.v[lav::CANDIDATE_ABSENCE_SECONDS]==5);assert(Preferences::legacy=="legacy-config");
 Preferences::fail=true;c.v[lav::WASH_SECONDS]=120;assert(!cfg.save(c));assert(cfg.current.v[lav::WASH_SECONDS]==60);Preferences::fail=false;
 JsonDocument configInput,errors;configJson(configInput.to<JsonObject>(),cfg.current);configInput["washMinSeconds"]=1.5;lav::Config parsed;assert(!parseConfig(configInput.as<JsonVariantConst>(),parsed,errors.to<JsonObject>()));
 String savedConfig=Preferences::config;
 Preferences::config="{\"revision\":7,\"presenceMaxMm\":1000,\"washMinSeconds\":90,\"absenceMinSeconds\":10}";
 String originalLegacy=Preferences::config;ConfigStore adapted;assert(adapted.begin());assert(adapted.current.revision==7&&adapted.current.v[lav::PRESENCE_MM]==1000&&adapted.current.v[lav::WASH_SECONDS]==120&&adapted.current.v[lav::ABSENCE_SECONDS]==30&&adapted.current.v[lav::CANDIDATE_ABSENCE_SECONDS]==5);assert(!adapted.error.isEmpty()&&Preferences::config==originalLegacy.c_str());
 assert(adapted.save(adapted.current));ConfigStore adaptedReboot;assert(adaptedReboot.begin()&&adaptedReboot.error.isEmpty()&&adaptedReboot.current.revision==8);
 JsonDocument restricted;configJson(restricted.to<JsonObject>(),adaptedReboot.current);restricted["candidateAbsenceSeconds"]=4;assert(!parseConfig(restricted.as<JsonVariantConst>(),parsed,errors.to<JsonObject>()));restricted.remove("candidateAbsenceSeconds");assert(!parseConfig(restricted.as<JsonVariantConst>(),parsed,errors.to<JsonObject>()));
 Preferences::config="{\"presenceMaxMm\":true,\"washMinSeconds\":120,\"absenceMinSeconds\":10}";ConfigStore corruptLegacy;assert(!corruptLegacy.begin());Preferences::config=savedConfig;
 puts("PASS configuration: legacy adaptation preserves allowed values/NVS, explicit save/reboot, missing and unsupported choices rejected");
 EventStorage store;assert(store.begin(nullptr));Event e;e.id=1;e.validated=true;e.startedAt="2026-10-01T12:00:00";e.validatedAt="2026-10-01T12:02:00";
 assert(store.enqueue(e));assert(store.enqueue(e));assert(store.observed==1);assert(store.queued()==1);assert(store.totals(false,"2026-10-01").total==0);assert(store.totals(true,"2026-10-01").total==1);store.tick(5000000);assert(store.queued()==0);
 assert(store.totals(false,"2026-10-01").today==1);assert(store.totals(false,"2026-10-02").today==0);
 e.closed=true;e.closedAt="2026-10-01T12:03:00";e.durationMs=180000;assert(store.enqueue(e));store.tick(10000000);assert(store.totals(false,"2026-10-01").total==1);
 EventStorage restarted;assert(restarted.begin(nullptr));assert(restarted.observed==1);
 bool more;unsigned rows=0;restarted.page(0,20,[&](JsonDocument& d){rows++;assert(d["closedAt"]=="2026-10-01T12:03:00");assert(d["interrupted"]==false);assert(d["volumeL"].isNull());},more);assert(rows==1&&!more);
 e.id=2;e.closed=false;e.closedAt="";e.validatedAt="";store.enqueue(e);disk.budget=20;store.tick(15000000);assert(store.queued()==1);disk.budget=100000;store.tick(20000000);assert(store.queued()==0);assert(store.totals(false,"").undated==1);
 EventStorage interrupted;assert(interrupted.begin(nullptr));rows=0;interrupted.page(1,1,[&](JsonDocument& d){rows++;assert(d["interrupted"]==true);assert(d["closedAt"].isNull());},more);assert(rows==1);
 // Complete JSON but missing newline, then retry: must consolidate one ID.
 e.id=3;JsonDocument data;e.json(data);String serialized;serializeJson(data,serialized);store.enqueue(e);disk.budget=serialized.length();store.tick(25000000);assert(store.queued()==1);disk.budget=100000;store.tick(30000000);assert(store.totals(false,"").total==3);
 disk.capacity=disk.bytes.length()+1;e.id=4;store.enqueue(e);store.tick(35000000);assert(store.queued()==1);
 for(unsigned i=5;i<14;i++){e.id=i;store.enqueue(e);}assert(store.queued()==8);assert(store.lostEvents==2);
 assert(legacyDisk.bytes=="preserved-old-history");assert(cfg.ensureIdAbove(100));assert(cfg.reserveId()==101);disk.mounted=false;EventStorage missing;assert(!missing.begin(nullptr));assert(!missing.mounted());
 // Multiple pages and IDs beyond JavaScript's exact integer range.
 disk=FakeDisk{};EventStorage pages;assert(pages.begin(nullptr));
 for(unsigned i=0;i<66;i++){Event row;row.id=9007199254740993ULL+i;row.closed=true;row.validated=true;assert(pages.enqueue(row));pages.tick((i+1)*5000000ULL);}
 uint64_t cursor=0;unsigned total=0;do{cursor=pages.page(cursor,20,[&](JsonDocument& d){assert(d["sessionId"].is<const char*>());total++;},more);}while(more);assert(total==66);assert(cursor==9007199254741058ULL);
 EventStorage pageBoot;assert(pageBoot.begin(nullptr));assert(pageBoot.observed==66);
 puts("PASS storage: config isolation/reboot/failure, validation+close, dates, partial writes, retry dedup, boot interruption, queue and capacity");
}

