#include "../firmware/LavCounter/Detection.h"
#include "../firmware/LavCounter/Calendar.h"
#include <assert.h>
#include <stdio.h>

using namespace lav;
int main(){
 Config c;assert(c.invalid()==-1);
 for(size_t i=0;i<FIELD_COUNT;i++){Config x=c;x.v[i]=NAN;assert(x.invalid()>=0);x=c;x.v[i]=FIELDS[i].min-1;assert(x.invalid()>=0);x=c;x.v[i]=FIELDS[i].max+1;assert(x.invalid()>=0);}
 Config x=c;x.v[PAUSE]=x.v[TIMEOUT];assert(x.invalid()==TIMEOUT);x=c;x.v[MIN_RATE]=30;assert(x.invalid()==MAX_RATE);x=c;x.v[SCORE]=4.5;assert(x.invalid()==SCORE);
 Metrics m;m.pulses=225;assert(fabs(m.liters(c)-.5)<1e-12);assert(isnan(m.rate(c)));
 Detector d;d.start(1000000,c);d.pulse(2000000);d.pulse(3000000);d.pulse(93000000);d.pulse(94000000);
 assert(d.metrics.pulses==5&&d.metrics.flowUs==3000000&&d.metrics.pauseUs==90000000);assert(d.metrics.durationUs()==d.metrics.flowUs+d.metrics.pauseUs);assert(d.metrics.pauses==1&&d.metrics.resumes==1);
 assert(d.state(97000000)==State::WaitingResume);assert(!d.expired(273999999));assert(d.expired(274000000));d.close();assert(d.metrics.closed==274000000&&d.metrics.last==94000000);
 d.start(0,c);assert(d.active);d.pulse(c.us(PAUSE)-1);assert(d.metrics.pauses==0);d.pulse(d.metrics.last+c.us(PAUSE));assert(d.metrics.pauses==1);
 d.start(4294967295ULL*1000,c);d.pulse(d.metrics.last+1000000);assert(d.metrics.flowUs==1000000);
 Config edited=c;edited.v[PULSES_PER_LITER]=900;edited.v[TIMEOUT]=10;assert(d.config.v[TIMEOUT]==180);d.start(0,edited);d.pulse(1000000);assert(d.expired(11000000));
 m=Metrics{};m.last=180000000;m.flowUs=180000000;m.pulses=13500;assert(classify(m,c).category==Category::Washing);
 m=Metrics{};m.last=300000000;m.flowUs=10000000;m.pulses=450;assert(classify(m,c).category==Category::Unidentified);
 m=Metrics{};m.pulses=1;assert(classify(m,c).category==Category::Discarded);
 m=Metrics{};m.last=180000000;m.flowUs=90000000;m.pauseUs=90000000;m.maxPauseUs=90000000;m.pulses=6750;m.pauses=m.resumes=1;assert(classify(m,c).category==Category::Washing);
 m.degraded=true;assert(classify(m,c).category==Category::Unidentified);m.degraded=false;
 // Every evidence changes independently at its documented threshold.
 Metrics base;base.last=c.us(WASH_DURATION);base.flowUs=c.us(WASH_FLOW);base.pulses=4500;base.pauses=1;base.resumes=1;base.maxPauseUs=c.us(MAX_PAUSE);
 assert(classify(base,c).evidence[0]);base.last--;assert(!classify(base,c).evidence[0]);base.last++;
 assert(classify(base,c).evidence[1]);base.flowUs--;assert(!classify(base,c).evidence[1]);base.flowUs++;
 assert(classify(base,c).evidence[2]);base.pulses--;assert(!classify(base,c).evidence[2]);base.pulses++;
 assert(classify(base,c).evidence[4]);base.maxPauseUs++;assert(!classify(base,c).evidence[4]);base.maxPauseUs--;
 base.flowUs=24000000;assert(classify(base,c).evidence[5]);base.flowUs--;assert(!classify(base,c).evidence[5]);
 Key discardKeys[]={DISCARD_DURATION,DISCARD_FLOW,DISCARD_VOLUME}; for(auto key:discardKeys){Metrics z;z.pulses=1;if(key==DISCARD_DURATION)z.last=c.us(key);if(key==DISCARD_FLOW){z.flowUs=c.us(key);z.last=z.flowUs;}if(key==DISCARD_VOLUME)z.pulses=90;assert(classify(z,c).category!=Category::Discarded);}
 Metrics rate;rate.last=rate.flowUs=60000000;rate.pulses=900;assert(classify(rate,c).evidence[3]);rate.pulses=899;assert(!classify(rate,c).evidence[3]);rate.pulses=11250;assert(classify(rate,c).evidence[3]);rate.pulses++;assert(!classify(rate,c).evidence[3]);
 PulseQueue<4> q;assert(q.push(1000,1000));assert(!q.push(1999,1000));assert(q.push(2000,1000));assert(q.push(3000,1000));assert(!q.push(4000,1000));assert(q.lost==1&&q.rejected==1);uint64_t at;assert(q.pop(at)&&at==1000);assert(q.pop(at)&&at==2000);assert(q.pop(at)&&at==3000);assert(!q.pop(at));
 Date date;assert(parseDate("2024-02-29T23:59:59",date));uint8_t b[7];encodeRtc(date,b);Date read;assert(decodeRtc(b,0,read));assert(epoch(date)==epoch(read));assert(!decodeRtc(b,128,read));b[3]=0;assert(!decodeRtc(b,0,read));encodeRtc(date,b);b[1]=0x6a;assert(!decodeRtc(b,0,read));assert(!parseDate("2023-02-29T12:00",date));assert(!parseDate("2024-01-01T24:00",date));assert(!parseDate("2024-01-01T12:00bad",date));
 for(int year=2000;year<=2099;year++)for(int month=1;month<=12;month++){Date z;z.year=year;z.month=month;z.day=1;assert(validDate(z));Date round=fromEpoch(epoch(z));assert(round.year==year&&round.month==month&&round.day==1);encodeRtc(z,b);assert(decodeRtc(b,0,read));assert(epoch(z)==epoch(read));}
 parseDate("2024-12-31T23:59:59",date);read=fromEpoch(epoch(date)+1);assert(read.year==2025&&read.month==1&&read.day==1);
 encodeRtc(read,b);b[2]=0x52;assert(decodeRtc(b,0,read)&&read.hour==0);b[2]=0x72;assert(decodeRtc(b,0,read)&&read.hour==12);b[2]=0x61;assert(decodeRtc(b,0,read)&&read.hour==13);
 printf("PASS: config limits, sessions/boundaries/rollover, calibration, six-evidence classification, noise/overflow, RTC/calendar (1200 months)\n");
}



