#pragma once
#include "Arduino.h"
#define HTTP_GET 1
#define HTTP_POST 2
#define BASIC_AUTH 0
#define CONTENT_LENGTH_UNKNOWN -1
class WebServer {
public:
 bool allowed=false;int code=0;String response,plain,category,cursor,limit;
 struct Route {const char* path=nullptr;int method=0;void(*handler)()=nullptr;};Route routes[16];unsigned routeCount=0;void(*notFound)()=nullptr;
 WebServer(int){}
 bool authenticate(const char* u,const char* p){return allowed&&strcmp(u,"lavcounter")==0&&strcmp(p,"lav123")==0;}
 void requestAuthentication(int,const char*){code=401;response="";}
 void sendHeader(const char*,const char*){}
 void send(int c,const char*,const String& b){code=c;response=b;}
 void send_P(int c,const char* mime,const char* b){send(c,mime,String(b));}
 void sendContent(const String& b){response+=b.c_str();}
 void setContentLength(int){}
 String arg(const char* k){return strcmp(k,"plain")==0?plain:strcmp(k,"category")==0?category:strcmp(k,"cursor")==0?cursor:limit;}
 bool hasArg(const char* k){return arg(k).length()!=0;}
 void on(const char* path,int method,void(*handler)()){routes[routeCount++]={path,method,handler};}
 void onNotFound(void(*handler)()){notFound=handler;}
 void begin(){}void handleClient(){}
};
