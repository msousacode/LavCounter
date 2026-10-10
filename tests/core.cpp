#include <assert.h>
#include <stdio.h>
#include "../firmware/LavCounter/Calendar.h"
#include "../firmware/LavCounter/Detection.h"
using namespace lav;
int main(){
 Config c;c.v[WASH_SECONDS]=1;c.v[ABSENCE_SECONDS]=1;Detector d;uint64_t now=0;
 auto feed=[&](Sample s,unsigned n){Transition t;for(unsigned i=0;i<n;i++){t=d.sample(now,s,c);now+=100000;}return t;};
 assert(d.state()==State::Idle);
 feed(Sample::Unknown,30);assert(d.state()==State::Idle&&!d.active()&&!d.validated);
 Detector restarted;assert(restarted.state()==State::Idle);
 assert(restarted.sample(0,Sample::Present,c).started);assert(restarted.state()==State::Candidate&&!restarted.validated);
 feed(Sample::Absent,5);assert(d.state()==State::Idle);
 assert(feed(Sample::Present,1).started);feed(Sample::Present,9);assert(!d.validated);
 assert(feed(Sample::Present,1).validated);assert(d.state()==State::ValidWash);
 assert(!feed(Sample::Present,100).validated);feed(Sample::Absent,5);feed(Sample::Present,20);assert(d.validated);
 feed(Sample::Absent,10);assert(d.active());assert(feed(Sample::Absent,1).closed);assert(!d.active());
 assert(feed(Sample::Present,1).started);feed(Sample::Present,5);feed(Sample::Absent,1);feed(Sample::Present,10);assert(!d.validated);
 feed(Sample::Unknown,1);feed(Sample::Present,10);assert(!d.validated);feed(Sample::Present,1);assert(d.validated);
 Detector gap;now=0;for(int i=0;i<11;i++)gap.sample(i*100000,Sample::Absent,c);
 gap.sample(1100000,Sample::Present,c);gap.sample(5000000,Sample::Present,c);assert(!gap.validated);
 c.v[WASH_SECONDS]=2;gap.sample(5100000,Sample::Present,c);assert(gap.config.v[WASH_SECONDS]==1);
 assert(ledColor(State::Candidate)[0]=='y');assert(ledOn(State::ValidWash,true,500000));assert(!ledOn(State::ValidWash,false,500000));
 Detector cancelled;Config shortConfig;shortConfig.v[WASH_SECONDS]=2;shortConfig.v[ABSENCE_SECONDS]=1;
 assert(cancelled.sample(0,Sample::Present,shortConfig).started);
 for(unsigned i=1;i<=10;i++)cancelled.sample(i*100000,Sample::Absent,shortConfig);assert(cancelled.active());
 auto cancelledResult=cancelled.sample(1100000,Sample::Absent,shortConfig);assert(cancelledResult.closed&&!cancelled.validated&&!cancelled.active());
 // Empty bay for a long uptime must not credit any time toward presence.
 Detector normal;Config defaults;uint64_t base=86400000000ULL;
 for(unsigned i=0;i<200;i++){
  auto empty=normal.sample(base+i*100000,Sample::Absent,defaults);
  assert(normal.state()==State::Idle&&!empty.started&&!empty.validated);
 }
 base+=20000000;
 auto arrival=normal.sample(base,Sample::Present,defaults);
 assert(arrival.started&&!arrival.validated&&normal.state()==State::Candidate);
 for(unsigned i=1;i<1200;i++){
  auto waiting=normal.sample(base+i*100000,Sample::Present,defaults);
  assert(normal.state()==State::Candidate&&!waiting.validated);
 }
 assert(normal.sample(base+120000000,Sample::Present,defaults).validated);
 for(unsigned i=1;i<=100;i++){
  auto leaving=normal.sample(base+120000000+i*100000,Sample::Absent,defaults);
  assert(!leaving.validated&&!leaving.closed&&normal.state()==State::ValidWash);
 }
 assert(normal.sample(base+130100000,Sample::Absent,defaults).closed);
 auto nextArrival=normal.sample(base+130200000,Sample::Present,defaults);
 assert(nextArrival.started&&!nextArrival.validated&&normal.state()==State::Candidate);
 puts("PASS no red-to-green shortcut: long empty uptime, full default 120s candidate, 10s absence, next candidate");
 c.v[WASH_SECONDS]=NAN;assert(c.invalid()==WASH_SECONDS);
 Date date;assert(parseDate("2024-02-29T23:59:59",date));uint8_t b[7];encodeRtc(date,b);Date read;assert(decodeRtc(b,0,read));assert(epoch(date)==epoch(read));assert(!decodeRtc(b,128,read));b[3]=0;assert(!decodeRtc(b,0,read));encodeRtc(date,b);b[1]=0x6a;assert(!decodeRtc(b,0,read));assert(!parseDate("2023-02-29T12:00",date));assert(!parseDate("2024-01-01T24:00",date));assert(!parseDate("2024-01-01T12:00bad",date));
 for(int year=2000;year<=2099;year++)for(int month=1;month<=12;month++){Date z;z.year=year;z.month=month;z.day=1;assert(validDate(z));Date round=fromEpoch(epoch(z));assert(round.year==year&&round.month==month&&round.day==1);encodeRtc(z,b);assert(decodeRtc(b,0,read));assert(epoch(z)==epoch(read));}
 parseDate("2024-12-31T23:59:59",date);read=fromEpoch(epoch(date)+1);assert(read.year==2025&&read.month==1&&read.day==1);
 encodeRtc(read,b);b[2]=0x52;assert(decodeRtc(b,0,read)&&read.hour==0);b[2]=0x72;assert(decodeRtc(b,0,read)&&read.hour==12);b[2]=0x61;assert(decodeRtc(b,0,read)&&read.hour==13);

 puts("PASS presence: boot, exact boundaries, single count, interrupted continuity, stale samples, snapshot, LEDs");
}
