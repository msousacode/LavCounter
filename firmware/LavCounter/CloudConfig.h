#pragma once
#include <Arduino.h>

namespace lavcloud {
inline bool segment(const char* s){
 if(!s||!*s||strlen(s)>128||!strcmp(s,".")||!strcmp(s,".."))return false;
 for(;*s;s++)if(!((*s>='a'&&*s<='z')||(*s>='A'&&*s<='Z')||(*s>='0'&&*s<='9')||*s=='-'||*s=='_'))return false;
 return true;
}
// Configuration URLs must not carry credentials, query parameters or fragments.
inline bool httpsUrl(const char* s){
 if(!s||strncmp(s,"https://",8)||strlen(s)>512)return false;
 const char* host=s+8;const char* end=strchr(host,'/');if(!end)end=host+strlen(host);
 if(end==host)return false;
 for(const char* p=host;p<end;p++)if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='.'||*p=='-'||*p==':'))return false;
 for(const char* p=s;*p;p++)if((unsigned char)*p<=32||*p=='@'||*p=='?'||*p=='#'||*p=='\\')return false;
 return true;
}
inline String encode(const char* s){String out;const char* hex="0123456789ABCDEF";for(;*s;s++){unsigned char c=*s;if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~')out+=(char)c;else{out+='%';out+=hex[c>>4];out+=hex[c&15];}}return out;}
struct CloudConfig {
 bool enabled=false;
 const char* ssid="";const char* wifiPassword="";const char* projectId="";const char* databaseId="(default)";
 const char* signupUrl="";const char* premiumUrl="";const char* caPem="";
 // Explicit, expiring integration override. Zero preserves production premium checks.
 uint64_t testUntil=0;const char* authUid="";bool firebaseManagedPremium=false;
 bool valid()const{return enabled&&segment(projectId)&&(!strcmp(databaseId,"(default)")||segment(databaseId))&&(httpsUrl(premiumUrl)||((firebaseManagedPremium||testUntil>=1700000000)&&segment(authUid)))&&*caPem;}
 bool stationConfigured()const{return *ssid&&strlen(ssid)<=32&&strlen(wifiPassword)<=63;}
 String binding(const char* client,const char* device,const char* mode)const{return String(projectId)+"/"+databaseId+"/"+client+"/"+device+"/"+mode;}
};
}
#if __has_include("CloudPrivate.h") && !defined(LAVCOUNTER_CLOUD_IGNORE_PRIVATE)
#include "CloudPrivate.h"
#endif
namespace lavcloud {
inline CloudConfig defaultConfig(){CloudConfig c;
#ifdef LAVCOUNTER_CLOUD_CONFIGURE
 LAVCOUNTER_CLOUD_CONFIGURE(c);
#endif
#if defined(LAVCOUNTER_FIREBASE_TEST_ENABLED) && LAVCOUNTER_FIREBASE_TEST_ENABLED && defined(LAVCOUNTER_FIREBASE_REFRESH_TOKEN)
 c.firebaseManagedPremium=true;c.testUntil=0;
#endif
 return c;}
}
