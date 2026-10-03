#pragma once
#include <ArduinoJson.h>
#include "ConfigStore.h"
struct Event {
 lav::Config config;uint64_t id=0,durationMs=0;
 String startedAt,validatedAt,closedAt;bool clockAdjusted=false,validated=false,closed=false;
 void json(JsonDocument& d)const{
  auto o=d.to<JsonObject>();char idText[24];snprintf(idText,sizeof(idText),"%llu",(unsigned long long)id);
  o["sessionId"]=idText;o["schemaVersion"]=2;o["firmwareVersion"]=lav::VERSION;o["sensorMode"]=lav::SENSOR_MODE;
  o["type"]=closed?"closed":validated?"validated":"candidate";o["validated"]=validated;
  if(startedAt.length())o["startedAt"]=startedAt;else o["startedAt"]=nullptr;
  if(validatedAt.length())o["validatedAt"]=validatedAt;else o["validatedAt"]=nullptr;
  if(closedAt.length())o["closedAt"]=closedAt;else o["closedAt"]=nullptr;
  o["durationMs"]=durationMs;o["clockAdjusted"]=clockAdjusted;o["timeBasis"]="local-rtc";
  o["timeQuality"]=startedAt.isEmpty()||(validated&&validatedAt.isEmpty())||(closed&&closedAt.isEmpty())?"unavailable":clockAdjusted?"adjusted":"rtc-projected";
  configJson(o["config"].to<JsonObject>(),config);
 }
};
