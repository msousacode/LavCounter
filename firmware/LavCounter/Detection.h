#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
namespace lav {
static constexpr const char* VERSION="2.0.0";
static constexpr uint64_t SAMPLE_VALID_US=500000;
struct Field {const char* key;const char* label;const char* unit;const char* help;double initial,min,max,step;unsigned group;};
static const Field FIELDS[]={
 {"presenceMaxMm","Distância máxima de presença","mm","Calibre com o veículo e o box vazio.",1500,100,4000,1,0},
 {"washMinSeconds","Permanência mínima para lavagem","s","Exige presença contínua; uma lavagem por sessão.",120,1,86400,1,0},
 {"absenceMinSeconds","Ausência mínima para encerrar","s","Ausências menores mantêm a mesma sessão.",10,1,3600,1,0}
};
constexpr size_t FIELD_COUNT=sizeof(FIELDS)/sizeof(FIELDS[0]);
enum Key {PRESENCE_MM,WASH_SECONDS,ABSENCE_SECONDS};
struct Config {
 uint32_t revision=1;double v[FIELD_COUNT];
 Config(){for(size_t i=0;i<FIELD_COUNT;i++)v[i]=FIELDS[i].initial;}
 uint64_t us(Key k)const{return (uint64_t)(v[k]*1000000.0);}
 int invalid()const{for(size_t i=0;i<FIELD_COUNT;i++)if(!isfinite(v[i])||floor(v[i])!=v[i]||v[i]<FIELDS[i].min||v[i]>FIELDS[i].max)return (int)i;return -1;}
};
enum class Sample {Unknown,Present,Absent};
enum class State {WaitClear,Idle,Candidate,ValidWash};
inline const char* stateName(State s){return s==State::WaitClear?"WAIT_CLEAR":s==State::Idle?"IDLE":s==State::Candidate?"CANDIDATE":"VALID_WASH";}
struct Transition {bool started=false,validated=false,closed=false;};
class Detector {
 State state_=State::WaitClear;Sample previous=Sample::Unknown;
 uint64_t since=0,last=0;bool seen=false;
public:
 Config config;uint64_t startedUs=0,validatedUs=0,closedUs=0;bool validated=false;
 bool active()const{return state_==State::Candidate||state_==State::ValidWash;}
 State state()const{return state_;}
 Transition sample(uint64_t now,Sample value,const Config& next){
  Transition t;
  if(seen&&(now<last||now-last>SAMPLE_VALID_US))previous=Sample::Unknown;
  seen=true;last=now;
  if(value==Sample::Unknown){previous=value;return t;}
  if(value!=previous){since=now;previous=value;}
  if(state_==State::WaitClear){if(value==Sample::Absent&&now-since>=next.us(ABSENCE_SECONDS)){state_=State::Idle;previous=Sample::Unknown;}return t;}
  if(state_==State::Idle){if(value==Sample::Present){config=next;startedUs=now;validatedUs=closedUs=0;validated=false;state_=State::Candidate;since=now;t.started=true;}return t;}
  if(value==Sample::Absent&&now-since>=config.us(ABSENCE_SECONDS)){closedUs=now;state_=State::Idle;previous=Sample::Unknown;t.closed=true;}
  else if(value==Sample::Present&&!validated&&now-since>=config.us(WASH_SECONDS)){validated=true;validatedUs=now;state_=State::ValidWash;t.validated=true;}
  return t;
 }
};
inline const char* ledColor(State s){return s==State::ValidWash?"green":s==State::Candidate?"yellow":"red";}
inline bool ledOn(State s,bool healthy,uint64_t now){return (s!=State::WaitClear&&healthy)||(now/500000)%2==0;}
}
