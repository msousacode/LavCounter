#pragma once
#define WIFI_AP 1
#define WIFI_AP_STA 3
#define WL_CONNECTED 3
struct FakeWiFi {int modeValue=0,statusValue=0;unsigned begins=0,aps=0;void persistent(bool){}void disconnect(bool,bool){statusValue=0;}void mode(int v){modeValue=v;}void softAP(const char*,const char*){aps++;}void begin(const char*,const char*){begins++;}void setAutoReconnect(bool){}int status(){return statusValue;}const char* softAPIP(){return "192.168.4.1";}};inline FakeWiFi WiFi;
