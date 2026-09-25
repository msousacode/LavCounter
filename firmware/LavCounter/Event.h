#pragma once
#include <ArduinoJson.h>
#include "ConfigStore.h"
struct Event {
 lav::Metrics metrics;lav::Config config;uint64_t id=0;
 String startedAt,finishedAt,closedAt;bool clockAdjusted=false;
 void json(JsonDocument& d)const{
  auto o=d.to<JsonObject>();char idText[24];snprintf(idText,sizeof(idText),"%llu",(unsigned long long)id);
  o["id"]=idText;o["schemaVersion"]=1;o["firmwareVersion"]=lav::VERSION;o["classifierVersion"]=1;
  if(startedAt.length())o["startedAt"]=startedAt;else o["startedAt"]=nullptr;
  if(finishedAt.length())o["finishedAt"]=finishedAt;else o["finishedAt"]=nullptr;
  if(closedAt.length())o["closedAt"]=closedAt;else o["closedAt"]=nullptr;
  o["timeBasis"]="local-rtc";o["timeQuality"]=startedAt.isEmpty()||finishedAt.isEmpty()||closedAt.isEmpty()?"unavailable":clockAdjusted?"adjusted":"rtc-projected";
  o["clockAdjusted"]=clockAdjusted;o["acquisitionDegraded"]=metrics.degraded;
  o["pulses"]=metrics.pulses;o["volumeL"]=metrics.liters(config);o["durationMs"]=metrics.durationUs()/1000;
  o["flowMs"]=metrics.flowUs/1000;o["pauseMs"]=metrics.pauseUs/1000;o["maxPauseMs"]=metrics.maxPauseUs/1000;
  o["pauseCount"]=metrics.pauses;o["resumeCount"]=metrics.resumes;
  double rate=metrics.rate(config);if(isfinite(rate))o["avgFlowLpm"]=rate;else o["avgFlowLpm"]=nullptr;
  auto verdict=lav::classify(metrics,config);o["category"]=lav::categoryName(verdict.category);o["score"]=verdict.score;
  static const char* keys[]={"duration","flow","volume","rate","pauses","resumes"};
  static const char* reasons[]={"Duração abaixo da referência","Tempo com fluxo insuficiente","Volume insuficiente","Vazão fora da faixa ou indisponível","Padrão de pausas insuficiente","Padrão de retomadas insuficiente"};
  JsonArray r=o["reasons"].to<JsonArray>();JsonObject evidence=o["evidence"].to<JsonObject>();
  for(size_t i=0;i<6;i++){evidence[keys[i]]=verdict.evidence[i];if(!verdict.evidence[i])r.add(reasons[i]);}
  if(verdict.belowMinimum)r.add("Duração, volume e fluxo abaixo dos três mínimos");
  if(metrics.degraded)r.add("Aquisição degradada: verificar perda de pulsos");
  if(verdict.category==lav::Category::Washing)r.add("Combinação de evidências satisfeita");
  configJson(o["config"].to<JsonObject>(),config);
 }
};
