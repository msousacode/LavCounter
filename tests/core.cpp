#include <assert.h>
#include <stdio.h>
#include "../firmware/LavCounter/Calendar.h"
#include "../firmware/LavCounter/Detection.h"
using namespace lav;
int main(){
 Config c;c.v[WASH_SECONDS]=1;c.v[ABSENCE_SECONDS]=1;Detector d;uint64_t now=0;
 auto feed=[&](Sample s,unsigned n){Transition t;for(unsigned i=0;i<n;i++){t=d.sample(now,s,c);now+=100000;}return t;};
 feed(Sample::Present,30);assert(d.state()==State::WaitClear);
 feed(Sample::Absent,5);feed(Sample::Unknown,1);feed(Sample::Absent,10);assert(d.state()==State::WaitClear);
 feed(Sample::Absent,1);assert(d.state()==State::Idle);
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
 c.v[WASH_SECONDS]=NAN;assert(c.invalid()==WASH_SECONDS);
 Date date;assert(parseDate("2024-02-29T23:59:59",date));uint8_t b[7];encodeRtc(date,b);Date read;assert(decodeRtc(b,0,read));assert(epoch(date)==epoch(read));assert(!decodeRtc(b,128,read));b[3]=0;assert(!decodeRtc(b,0,read));encodeRtc(date,b);b[1]=0x6a;assert(!decodeRtc(b,0,read));assert(!parseDate("2023-02-29T12:00",date));assert(!parseDate("2024-01-01T24:00",date));assert(!parseDate("2024-01-01T12:00bad",date));
 for(int year=2000;year<=2099;year++)for(int month=1;month<=12;month++){Date z;z.year=year;z.month=month;z.day=1;assert(validDate(z));Date round=fromEpoch(epoch(z));assert(round.year==year&&round.month==month&&round.day==1);encodeRtc(z,b);assert(decodeRtc(b,0,read));assert(epoch(z)==epoch(read));}
 parseDate("2024-12-31T23:59:59",date);read=fromEpoch(epoch(date)+1);assert(read.year==2025&&read.month==1&&read.day==1);
 encodeRtc(read,b);b[2]=0x52;assert(decodeRtc(b,0,read)&&read.hour==0);b[2]=0x72;assert(decodeRtc(b,0,read)&&read.hour==12);b[2]=0x61;assert(decodeRtc(b,0,read)&&read.hour==13);

 puts("PASS presence: boot, exact boundaries, single count, interrupted continuity, stale samples, snapshot, LEDs");
}
