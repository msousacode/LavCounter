#include <stdlib.h>
#include <new>
void* operator new(size_t n){void* p=malloc(n);if(!p)abort();return p;}
void operator delete(void* p) noexcept{free(p);}
void operator delete(void* p,size_t) noexcept{free(p);}
#include "../firmware/LavCounter/Storage.h"
#include <assert.h>
int main(){
 ConfigStore s;assert(s.begin());lav::Config c=s.current;c.v[lav::TIMEOUT]=90;assert(s.save(c));assert(s.current.revision==2);
 ConfigStore reboot;assert(reboot.begin());assert(reboot.current.v[lav::TIMEOUT]==90);
 auto original=reboot.current;Preferences::fail=true;c.v[lav::TIMEOUT]=60;assert(!reboot.save(c));assert(reboot.current.v[lav::TIMEOUT]==90);Preferences::fail=false;
 uint64_t first=reboot.reserveId();ConfigStore boot2;assert(boot2.begin());assert(boot2.reserveId()==first+1);
 JsonDocument invalid,errors;configJson(invalid.to<JsonObject>(),original);invalid["timeoutSeconds"]=0;assert(!parseConfig(invalid.as<JsonVariantConst>(),c,errors.to<JsonObject>()));
 Event e;e.id=first;e.config=original;e.metrics.first=0;e.metrics.last=1000000;e.metrics.flowUs=1000000;e.metrics.pulses=225;e.startedAt="2026-09-25T12:00:00";e.finishedAt="2026-09-25T12:00:01";e.closedAt="2026-09-25T12:01:31";
 JsonDocument record;e.json(record);assert(record["volumeL"].as<double>()==.5);assert(record["config"]["revision"].as<unsigned>()==2);assert(record["timeQuality"]=="rtc-projected");
 EventStorage h;assert(h.begin(nullptr));h.enqueue(e);h.tick(5000000);assert(h.queued()==0);String complete=disk.bytes;
 bool more=false;unsigned count=0;h.page(0,50,"",[&](const String& row){JsonDocument d;assert(!deserializeJson(d,row));assert(d["pulses"]==225);count++;},more);assert(count==1&&!more);
 EventStorage rebootHistory;assert(rebootHistory.begin(nullptr));rebootHistory.enqueue(e);rebootHistory.tick(5000000);assert(disk.bytes.length()==complete.length());
 // Partial record survives, retry becomes a separate complete JSONL record.
 e.id=10;disk.budget=30;h.enqueue(e);h.tick(10000000);assert(h.queued()==1&&!h.error.isEmpty());disk.budget=1000000;h.tick(15000000);assert(h.queued()==0);count=0;h.page(0,50,"",[&](const String&){count++;},more);assert(count==2&&h.degraded);
 // Full medium preserves prior bytes and keeps pending data.
 e.id=11;h.enqueue(e);auto length=disk.bytes.length();disk.capacity=length;h.tick(20000000);assert(h.queued()==1&&disk.bytes.length()==length);disk.capacity=1000000;h.tick(25000000);assert(h.queued()==0);
 // >50 records, explicit pagination and category filtering.
 for(unsigned i=12;i<75;i++){e.id=i;h.enqueue(e);h.tick((uint64_t)i*5000000);}
 count=0;uint32_t cursor=h.page(0,50,"unidentified",[&](const String&){count++;},more);assert(count==50&&more);unsigned remainder=0;h.page(cursor,50,"unidentified",[&](const String&){remainder++;},more);assert(remainder==16&&!more);
 // Truncated tail must not make earlier rows unreadable.
 disk.bytes+="{bad";EventStorage degraded;assert(degraded.begin(nullptr));assert(degraded.degraded);count=0;degraded.page(0,50,"",[&](const String&){count++;},more);assert(count==50);
 disk.mounted=false;EventStorage absent;assert(!absent.begin(nullptr));assert(!absent.error.isEmpty());
 printf("PASS: real ConfigStore/Event/Storage with fake NVS and SPIFFS: persistence, atomic failure, IDs, JSON, partial writes, full disk, restart, 66 records/pagination\n");
}
