#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>

namespace lav {
static constexpr const char* VERSION = "1.0.0";
// Public values use seconds, litres, L/min and percent, matching the form.
struct Field { const char* key; const char* label; const char* unit; const char* help; double initial, min, max, step; unsigned group; };
static const Field FIELDS[] = {
 {"pauseSeconds","Reconhecer pausa","s","Ausência de pulsos que separa trechos de fluxo.",3,.1,3600,.1,0},
 {"timeoutSeconds","Encerrar por inatividade","s","Encerra e classifica; não decide sozinho se houve lavagem.",180,1,86400,1,0},
 {"discardDuration","Duração mínima do evento","s","Descarte somente quando os três mínimos não forem atingidos.",5,.1,86400,.1,1},
 {"discardFlow","Tempo mínimo com fluxo","s","Usado em conjunto com duração e volume mínimos.",2,.1,86400,.1,1},
 {"discardVolume","Volume mínimo do evento","L","Usos relevantes continuam registrados como não identificados.",.2,.001,10000,.001,1},
 {"washDuration","Duração de referência","s","Uma das seis evidências de lavagem.",120,1,86400,1,2},
 {"washFlow","Tempo com fluxo para lavagem","s","Evidência obrigatória junto com volume e vazão.",60,1,86400,1,2},
 {"washVolume","Volume para lavagem","L","Evidência obrigatória, sem decidir isoladamente.",10,.001,100000,.001,2},
 {"minRate","Vazão mínima","L/min","Limite inferior durante fluxo.",2,.001,10000,.001,2},
 {"maxRate","Vazão máxima","L/min","Limite superior durante fluxo.",25,.001,10000,.001,2},
 {"minPauses","Quantidade de pausas","pausas","Pausas ajudam a pontuar, mas não são obrigatórias.",1,1,1000,1,2},
 {"maxPause","Maior pausa interna","s","Limite de pausa para essa evidência pontuar.",120,.1,86400,.1,2},
 {"minResumes","Quantidade de retomadas","retomadas","Retomadas ajudam a pontuar, mas não são obrigatórias.",1,1,1000,1,2},
 {"flowPercent","Proporção mínima de fluxo","%","Tempo com fluxo dividido pela duração total.",20,.1,100,.1,2},
 {"score","Pontuação exigida","de 6","4 permite lavagem contínua; 5 ou 6 exigem padrão intermitente.",4,4,6,1,2},
 {"pulsesPerLiter","Calibração do sensor","pulsos/L","Ajuste comparando com volume real conhecido.",450,1,100000,1,3},
 {"filterMs","Intervalo mínimo entre pulsos","ms","Pulsos mais próximos são rejeitados como ruído.",1,.1,100,.1,3},
 {"sampleSeconds","Janela de vazão","s","Intervalo de atualização da vazão instantânea.",1,.1,60,.1,3}
};
constexpr size_t FIELD_COUNT=sizeof(FIELDS)/sizeof(FIELDS[0]);
enum Key { PAUSE,TIMEOUT,DISCARD_DURATION,DISCARD_FLOW,DISCARD_VOLUME,WASH_DURATION,WASH_FLOW,WASH_VOLUME,MIN_RATE,MAX_RATE,MIN_PAUSES,MAX_PAUSE,MIN_RESUMES,FLOW_PERCENT,SCORE,PULSES_PER_LITER,FILTER_MS,SAMPLE_SECONDS };
struct Config {
 uint32_t revision=1; double v[FIELD_COUNT];
 Config(){for(size_t i=0;i<FIELD_COUNT;i++)v[i]=FIELDS[i].initial;}
 uint64_t us(Key k)const{return (uint64_t)llround(v[k]*1000000.0);}
 int invalid()const {
  for(size_t i=0;i<FIELD_COUNT;i++) {
   if(!isfinite(v[i])||v[i]<FIELDS[i].min||v[i]>FIELDS[i].max)return (int)i;
   if((i==MIN_PAUSES||i==MIN_RESUMES||i==SCORE)&&floor(v[i])!=v[i])return (int)i;
  }
  if(v[PAUSE]>=v[TIMEOUT])return TIMEOUT;
  if(v[MIN_RATE]>v[MAX_RATE])return MAX_RATE;
  return -1;
 }
};
struct Metrics {
 uint64_t first=0,last=0,closed=0,pulses=0,flowUs=0,pauseUs=0,maxPauseUs=0;
 uint32_t pauses=0,resumes=0; bool degraded=false;
 uint64_t durationUs()const{return last-first;}
 double liters(const Config& c)const{return pulses/c.v[PULSES_PER_LITER];}
 double rate(const Config& c)const{return flowUs?liters(c)*60000000.0/flowUs:NAN;}
};
enum class Category { Washing, Unidentified, Discarded };
inline const char* categoryName(Category c){return c==Category::Washing?"washing":c==Category::Discarded?"discarded":"unidentified";}
struct Verdict { Category category=Category::Unidentified; bool evidence[6]={}; uint8_t score=0; bool belowMinimum=false; };
inline Verdict classify(const Metrics& m,const Config& c){
 Verdict r;
 r.belowMinimum=m.durationUs()<c.us(DISCARD_DURATION)&&m.flowUs<c.us(DISCARD_FLOW)&&m.liters(c)<c.v[DISCARD_VOLUME];
 double rate=m.rate(c);
 r.evidence[0]=m.durationUs()>=c.us(WASH_DURATION);
 r.evidence[1]=m.flowUs>=c.us(WASH_FLOW);
 r.evidence[2]=m.liters(c)>=c.v[WASH_VOLUME];
 r.evidence[3]=isfinite(rate)&&rate>=c.v[MIN_RATE]&&rate<=c.v[MAX_RATE];
 r.evidence[4]=m.pauses>=c.v[MIN_PAUSES]&&m.maxPauseUs<=c.us(MAX_PAUSE);
 r.evidence[5]=m.resumes>=c.v[MIN_RESUMES]&&m.durationUs()>0&&100.0*m.flowUs/m.durationUs()>=c.v[FLOW_PERCENT];
 for(bool e:r.evidence)if(e)r.score++;
 if(r.belowMinimum)r.category=Category::Discarded;
 else if(!m.degraded&&r.evidence[1]&&r.evidence[2]&&r.evidence[3]&&r.score>=c.v[SCORE])r.category=Category::Washing;
 return r;
}
enum class State { Idle,Flowing,WaitingResume };
class Detector {
public:
 Config config; Metrics metrics; bool active=false;
 void start(uint64_t at,const Config& c){config=c;metrics=Metrics{};metrics.first=metrics.last=at;metrics.pulses=1;active=true;}
 bool expired(uint64_t at)const{return active&&at>=metrics.last&&at-metrics.last>=config.us(TIMEOUT);}
 void pulse(uint64_t at){
  if(at<metrics.last){metrics.degraded=true;return;}
  uint64_t gap=at-metrics.last;
  if(gap>=config.us(PAUSE)){metrics.pauseUs+=gap;metrics.pauses++;metrics.resumes++;if(gap>metrics.maxPauseUs)metrics.maxPauseUs=gap;}
  else metrics.flowUs+=gap;
  metrics.last=at;metrics.pulses++;
 }
 void close(){metrics.closed=metrics.last+config.us(TIMEOUT);active=false;}
 State state(uint64_t now)const{return !active?State::Idle:(now>=metrics.last&&now-metrics.last>=config.us(PAUSE)?State::WaitingResume:State::Flowing);}
};
// Single-producer/single-consumer queue; caller holds the ESP32 spinlock.
template<size_t N> struct PulseQueue {
 uint64_t timestamps[N]={};size_t head=0,tail=0;uint64_t last=0,total=0,rejected=0,lost=0;bool seen=false;
 bool push(uint64_t at,uint64_t filterUs){
  if(seen&&at-last<filterUs){rejected++;return false;}
  last=at;seen=true;total++;size_t next=(head+1)%N;
  if(next==tail){lost++;return false;}timestamps[head]=at;head=next;return true;
 }
 bool pop(uint64_t& at){if(tail==head)return false;at=timestamps[tail];tail=(tail+1)%N;return true;}
};
}
