#pragma once
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <esp_timer.h>
#include "CloudConfig.h"
namespace lavcloud {
class CloudNetwork {
 String ssid,password;uint64_t nextAttempt=0,attemptStarted=0;uint32_t delaySeconds=5;bool previous=false,reconnect=false;
 bool save(const String& name,const String& secret){
  Preferences prefs;if(!prefs.begin("lavWifi",false))return false;
  JsonDocument d;d["ssid"]=name;d["password"]=secret;String record;serializeJson(d,record);
  bool ok=prefs.putString("network",record)==record.length();prefs.end();return ok;
 }
public:
 String error;
 static bool valid(const String& name,const String& secret){return name.length()>0&&name.length()<=32&&secret.length()<=63&&(secret.isEmpty()||secret.length()>=8)&&strlen(name.c_str())==name.length()&&strlen(secret.c_str())==secret.length();}
 void begin(const CloudConfig& c){
  ssid=c.ssid;password=c.wifiPassword;
  Preferences prefs;if(prefs.begin("lavWifi",false)){
   String record=prefs.getString("network","");prefs.end();
   if(!record.isEmpty()){JsonDocument d;if(record.length()>512||deserializeJson(d,record)||!d["ssid"].is<String>()||!d["password"].is<String>()||!valid(d["ssid"].as<String>(),d["password"].as<String>()))error="Rede salva inválida. Configure o Wi-Fi novamente.";
    else{ssid=d["ssid"].as<String>();password=d["password"].as<String>();}}
  }else error="Não foi possível ler a rede salva.";
  WiFi.persistent(false);WiFi.mode(WIFI_AP_STA);WiFi.softAP("LavCounter","lavcounter123");WiFi.setAutoReconnect(false);
  if(configured()){WiFi.begin(ssid.c_str(),password.c_str());attemptStarted=esp_timer_get_time();nextAttempt=attemptStarted+30000000;}
 }
 bool configured()const{return !ssid.isEmpty();}
 bool connected()const{return configured()&&!reconnect&&WiFi.status()==WL_CONNECTED;}
 bool configure(const String& name,const String& secret,uint64_t now){
  if(!valid(name,secret)){error="Informe rede de até 32 bytes e senha de 8 a 63 caracteres (ou vazia para rede aberta).";return false;}
  if(!save(name,secret)){error="Não foi possível salvar o Wi-Fi. Tente novamente.";return false;}
  ssid=name;password=secret;error="";previous=false;reconnect=true;delaySeconds=5;attemptStarted=now;nextAttempt=now+500000;return true;
 }
 void describe(JsonDocument& d){d["wifiConfigured"]=configured();d["wifiConnected"]=connected();d["wifiSsid"]=ssid;d["wifiState"]=connected()?"connected":!configured()?"not_configured":error.isEmpty()?"connecting":"error";d["wifiError"]=error;}
 bool tick(uint64_t now){bool current=connected();bool rising=current&&!previous;previous=current;if(current){error="";attemptStarted=now;delaySeconds=5;nextAttempt=now+5000000;return rising;}
  if(configured()&&now>=nextAttempt){
   if(reconnect){WiFi.disconnect(false,false);reconnect=false;}
   if(attemptStarted&&now-attemptStarted>=30000000)error="Sem conexão com a rede. Confira nome, senha e sinal do Wi-Fi de 2,4 GHz.";
   WiFi.begin(ssid.c_str(),password.c_str());nextAttempt=now+(uint64_t)(delaySeconds<30?30:delaySeconds)*1000000;delaySeconds=delaySeconds>=150?300:delaySeconds*2;
  }return false;
 }
};
}
