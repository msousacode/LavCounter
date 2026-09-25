#pragma once
struct FakeWiFi {void softAP(const char*,const char*){}const char* softAPIP(){return "192.168.4.1";}};inline FakeWiFi WiFi;
