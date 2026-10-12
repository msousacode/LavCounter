#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
namespace lav {
static constexpr const char* VERSION="2.0.0";
static constexpr uint64_t SAMPLE_VALID_US=500000;
struct Choice {double value;const char* label;};
static const Choice DISTANCES[]={{400,"0,4 m (40 cm)"},{1000,"1 m"},{1500,"1,5 m"}};
static const Choice WASH_TIMES[]={{60,"1 min"},{120,"2 min"},{180,"3 min"},{240,"4 min"},{300,"5 min"}};
static const Choice CLOSE_TIMES[]={{30,"0,5 min (30 segundos)"},{60,"1 min"},{120,"2 min"}};
static const Choice INVALID_TIMES[]={{3,"3 s"},{5,"5 s"},{30,"30 s"},{60,"1 min"}};
struct Field {const char* key;const char* label;const char* unit;const char* help;double initial,min,max,step;unsigned group;const Choice* choices;size_t choiceCount;};
static const Field FIELDS[]={
 {"presenceMaxMm","Distância máxima de presença","m","Distância válida até este limite indica presença.",1500,400,1500,1,0,DISTANCES,3},
 {"washMinSeconds","Permanência mínima para lavagem","min","Presença contínua no amarelo até validar uma lavagem.",120,60,300,1,0,WASH_TIMES,5},
 {"absenceMinSeconds","Ausência mínima para encerrar lavagem","min","No verde, ausência contínua encerra a sessão e volta ao vermelho.",30,30,120,1,0,CLOSE_TIMES,3},
 {"candidateAbsenceSeconds","Ausência mínima para invalidar a lavagem","s","No amarelo, ausência contínua encerra sem contabilizar e volta ao vermelho.",5,3,60,1,0,INVALID_TIMES,4}
};
constexpr size_t FIELD_COUNT=sizeof(FIELDS)/sizeof(FIELDS[0]);
enum Key {PRESENCE_MM,WASH_SECONDS,ABSENCE_SECONDS,CANDIDATE_ABSENCE_SECONDS};
inline bool allowed(size_t key,double value){for(size_t j=0;j<FIELDS[key].choiceCount;j++)if(value==FIELDS[key].choices[j].value)return true;return false;}
struct Config {
 uint32_t revision=1;double v[FIELD_COUNT];
 Config(){for(size_t i=0;i<FIELD_COUNT;i++)v[i]=FIELDS[i].initial;}
 uint64_t us(Key k)const{return (uint64_t)(v[k]*1000000.0);}
 int invalid()const{for(size_t i=0;i<FIELD_COUNT;i++)if(!allowed(i,v[i]))return (int)i;return -1;}
};
enum class Sample {Unknown,Present,Absent};
enum class State {WaitClear,Idle,Candidate,ValidWash};
inline const char* stateName(State s){return s==State::WaitClear?"WAIT_CLEAR":s==State::Idle?"IDLE":s==State::Candidate?"CANDIDATE":"VALID_WASH";}
struct Transition {bool started=false,validated=false,closed=false;};
class Detector {
 State state_=State::Idle;Sample previous=Sample::Unknown;
 uint64_t since=0,last=0,exitAbsence=0;bool seen=false;
public:
 Config config;uint64_t startedUs=0,validatedUs=0,closedUs=0;bool validated=false;
 bool active()const{return state_==State::Candidate||state_==State::ValidWash;}
 State state()const{return state_;}
 Transition sample(uint64_t now,Sample value,const Config& next,bool pauseExitOnOpticalError=false){
  Transition t;
  bool gap=seen&&(now<last||now-last>SAMPLE_VALID_US);
  if(gap){previous=Sample::Unknown;exitAbsence=0;}
  if(value==Sample::Present)exitAbsence=0;
  // Credit only intervals bracketed by confirmed absence samples. A permitted
  // optical error can pause the validated session's exit, never prove absence.
  if(state_==State::ValidWash&&value==Sample::Absent&&previous==Sample::Absent)exitAbsence+=now-last;
  seen=true;last=now;
  if(value==Sample::Unknown){if(state_!=State::ValidWash||!pauseExitOnOpticalError)exitAbsence=0;previous=value;return t;}
  if(value!=previous){since=now;previous=value;}
  if(state_==State::WaitClear){if(value==Sample::Absent&&now-since>=next.us(ABSENCE_SECONDS)){state_=State::Idle;previous=Sample::Unknown;}return t;}
  if(state_==State::Idle){if(value==Sample::Present){config=next;startedUs=now;validatedUs=closedUs=0;validated=false;exitAbsence=0;state_=State::Candidate;since=now;t.started=true;}return t;}
  Key absenceKey=state_==State::Candidate?CANDIDATE_ABSENCE_SECONDS:ABSENCE_SECONDS;
  uint64_t absentUs=state_==State::ValidWash?exitAbsence:now-since;
  if(value==Sample::Absent&&absentUs>=config.us(absenceKey)){closedUs=now;state_=State::Idle;previous=Sample::Unknown;exitAbsence=0;t.closed=true;}
  else if(state_==State::Candidate&&value==Sample::Present&&!validated&&now-since>=config.us(WASH_SECONDS)){validated=true;validatedUs=now;state_=State::ValidWash;t.validated=true;}
  return t;
 }
};
inline const char* ledColor(State s){return s==State::ValidWash?"green":s==State::Candidate?"yellow":"red";}
inline bool ledOn(State s,bool healthy,uint64_t now){return (s!=State::WaitClear&&healthy)||(now/500000)%2==0;}
}
