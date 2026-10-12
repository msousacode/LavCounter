#include <assert.h>
#include <stdio.h>
#include "../firmware/LavCounter/Calendar.h"
#include "../firmware/LavCounter/Detection.h"
using namespace lav;
int main(){
 Config c;c.v[WASH_SECONDS]=1;c.v[ABSENCE_SECONDS]=1;c.v[CANDIDATE_ABSENCE_SECONDS]=1;Detector d;uint64_t now=0;
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
 Detector cancelled;Config shortConfig;shortConfig.v[WASH_SECONDS]=2;shortConfig.v[ABSENCE_SECONDS]=1;shortConfig.v[CANDIDATE_ABSENCE_SECONDS]=1;
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
 for(unsigned i=1;i<=300;i++){
  auto leaving=normal.sample(base+120000000+i*100000,Sample::Absent,defaults);
  assert(!leaving.validated&&!leaving.closed&&normal.state()==State::ValidWash);
 }
 assert(normal.sample(base+150100000,Sample::Absent,defaults).closed);
 auto nextArrival=normal.sample(base+150200000,Sample::Present,defaults);
 assert(nextArrival.started&&!nextArrival.validated&&normal.state()==State::Candidate);
 puts("PASS no red-to-green shortcut: long empty uptime, full default 120s candidate, 30s absence, next candidate");
 Config split;split.v[CANDIDATE_ABSENCE_SECONDS]=3;split.v[ABSENCE_SECONDS]=30;split.v[WASH_SECONDS]=60;Detector splitDetector;
 assert(splitDetector.sample(0,Sample::Present,split).started);
 for(unsigned i=1;i<=30;i++)assert(!splitDetector.sample(i*100000,Sample::Absent,split).closed);
 assert(splitDetector.sample(3100000,Sample::Absent,split).closed&&!splitDetector.validated);
 assert(splitDetector.sample(3200000,Sample::Present,split).started);
 for(unsigned i=1;i<=600;i++)splitDetector.sample(3200000+i*100000,Sample::Present,split);
 assert(splitDetector.validated);Config changed=split;changed.v[ABSENCE_SECONDS]=120;
 for(unsigned i=1;i<=300;i++)assert(!splitDetector.sample(63200000+i*100000,Sample::Absent,changed).closed);
 assert(splitDetector.sample(93300000,Sample::Absent,changed).closed&&splitDetector.validated);
 // A VL53L1X status-7 pause must not repeatedly erase confirmed exit time.
 Config pauseConfig;pauseConfig.v[WASH_SECONDS]=1;pauseConfig.v[ABSENCE_SECONDS]=1;
 auto readyWash=[&](){Detector result;for(unsigned i=0;i<=10;i++)result.sample(i*100000,Sample::Present,pauseConfig);assert(result.validated);for(unsigned i=11;i<=15;i++)result.sample(i*100000,Sample::Absent,pauseConfig);return result;};
 Detector optical=readyWash();
 for(unsigned i=16;i<=25;i++){assert(!optical.sample(i*100000,Sample::Unknown,pauseConfig,true).closed);assert(optical.state()==State::ValidWash);}
 optical.sample(2600000,Sample::Absent,pauseConfig);
 for(unsigned i=27;i<=31;i++)assert(!optical.sample(i*100000,Sample::Absent,pauseConfig).closed);
 assert(optical.sample(3200000,Sample::Absent,pauseConfig).closed);
 Detector noisy=readyWash();uint64_t noisyTime=1500000;
 for(unsigned i=0;i<6;i++){assert(!noisy.sample(noisyTime+=100000,Sample::Unknown,pauseConfig,true).closed);assert(!noisy.sample(noisyTime+=100000,Sample::Absent,pauseConfig).closed);assert(noisy.sample(noisyTime+=100000,Sample::Absent,pauseConfig).closed==(i==5));}
 Detector returned=readyWash();returned.sample(1600000,Sample::Unknown,pauseConfig,true);returned.sample(1700000,Sample::Present,pauseConfig);
 for(unsigned i=18;i<=27;i++)assert(!returned.sample(i*100000,Sample::Absent,pauseConfig).closed);
 assert(returned.sample(2800000,Sample::Absent,pauseConfig).closed);
 Detector transport=readyWash();transport.sample(1600000,Sample::Unknown,pauseConfig,false);
 for(unsigned i=17;i<=26;i++)assert(!transport.sample(i*100000,Sample::Absent,pauseConfig).closed);
 assert(transport.sample(2700000,Sample::Absent,pauseConfig).closed);
 Detector staleExit=readyWash();assert(!staleExit.sample(3000000,Sample::Absent,pauseConfig).closed);
 for(unsigned i=31;i<=39;i++)assert(!staleExit.sample(i*100000,Sample::Absent,pauseConfig).closed);
 assert(staleExit.sample(4000000,Sample::Absent,pauseConfig).closed);
 Detector opticalCandidate;opticalCandidate.sample(0,Sample::Present,pauseConfig);for(unsigned i=1;i<=9;i++)opticalCandidate.sample(i*100000,Sample::Present,pauseConfig);opticalCandidate.sample(1000000,Sample::Unknown,pauseConfig,true);for(unsigned i=11;i<=20;i++)assert(!opticalCandidate.sample(i*100000,Sample::Present,pauseConfig).validated);assert(opticalCandidate.sample(2100000,Sample::Present,pauseConfig).validated);
 puts("PASS validated exit: optical pause excludes unknown time, return/transport/stale reset, candidate still requires continuous presence");
 Config restricted;
 for(size_t i=0;i<FIELD_COUNT;i++)for(size_t j=0;j<FIELDS[i].choiceCount;j++){restricted.v[i]=FIELDS[i].choices[j].value;assert(restricted.invalid()==-1);}
 restricted=Config{};restricted.v[PRESENCE_MM]=401;assert(restricted.invalid()==PRESENCE_MM);
 restricted=Config{};restricted.v[WASH_SECONDS]=90;assert(restricted.invalid()==WASH_SECONDS);
 restricted=Config{};restricted.v[ABSENCE_SECONDS]=31;assert(restricted.invalid()==ABSENCE_SECONDS);
 restricted=Config{};restricted.v[CANDIDATE_ABSENCE_SECONDS]=4;assert(restricted.invalid()==CANDIDATE_ABSENCE_SECONDS);
 puts("PASS separate candidate/validated absence, exact boundaries, session snapshot and fixed configuration choices");
 c.v[WASH_SECONDS]=NAN;c.v[PRESENCE_MM]=1500;assert(c.invalid()==WASH_SECONDS);
 Date date;assert(parseDate("2024-02-29T23:59:59",date));uint8_t b[7];encodeRtc(date,b);Date read;assert(decodeRtc(b,0,read));assert(epoch(date)==epoch(read));assert(!decodeRtc(b,128,read));b[3]=0;assert(!decodeRtc(b,0,read));encodeRtc(date,b);b[1]=0x6a;assert(!decodeRtc(b,0,read));assert(!parseDate("2023-02-29T12:00",date));assert(!parseDate("2024-01-01T24:00",date));assert(!parseDate("2024-01-01T12:00bad",date));
 for(int year=2000;year<=2099;year++)for(int month=1;month<=12;month++){Date z;z.year=year;z.month=month;z.day=1;assert(validDate(z));Date round=fromEpoch(epoch(z));assert(round.year==year&&round.month==month&&round.day==1);encodeRtc(z,b);assert(decodeRtc(b,0,read));assert(epoch(z)==epoch(read));}
 parseDate("2024-12-31T23:59:59",date);read=fromEpoch(epoch(date)+1);assert(read.year==2025&&read.month==1&&read.day==1);
 encodeRtc(read,b);b[2]=0x52;assert(decodeRtc(b,0,read)&&read.hour==0);b[2]=0x72;assert(decodeRtc(b,0,read)&&read.hour==12);b[2]=0x61;assert(decodeRtc(b,0,read)&&read.hour==13);

 puts("PASS presence: boot, exact boundaries, single count, interrupted continuity, stale samples, snapshot, LEDs");
}

