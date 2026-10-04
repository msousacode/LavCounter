#pragma once
#include <Preferences.h>
#include "CloudConfig.h"
#ifdef ARDUINO_ARCH_ESP32
#include <esp_mac.h>
#include <esp_random.h>
#include <bootloader_random.h>
#endif
namespace lavcloud {
inline bool uuid(const char* s){if(strlen(s)!=36)return false;for(unsigned i=0;i<36;i++){if(i==8||i==13||i==18||i==23){if(s[i]!='-')return false;}else if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return false;}return s[14]=='4'&&(s[19]=='8'||s[19]=='9'||s[19]=='a'||s[19]=='b');}
class DeviceIdentity {
 Preferences prefs;bool ready_=false;
public:
 String clientId,deviceId,error;
 bool begin(const uint8_t mac[6],void(*randomBytes)(uint8_t*,size_t)){
  ready_=false;error="";char device[19];snprintf(device,sizeof(device),"esp32-%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);deviceId=device;
  if(!prefs.begin("lavIdentity",false)){error="Identidade NVS indisponível.";return false;}
  bool exists=prefs.isKey("clientId");String saved=prefs.getString("clientId","");
  if(exists){if(!uuid(saved.c_str())){error="Identidade inválida; reprovisionamento necessário.";return false;}clientId=saved;}
  else{uint8_t b[16];randomBytes(b,16);b[6]=(b[6]&15)|64;b[8]=(b[8]&63)|128;char text[37];snprintf(text,sizeof(text),"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],b[10],b[11],b[12],b[13],b[14],b[15]);
   if(prefs.putString("clientId",text)!=36||prefs.getString("clientId","")!=text){error="Falha ao persistir identidade.";return false;}clientId=text;
  }ready_=true;return true;
 }
 bool begin(){
#ifdef ARDUINO_ARCH_ESP32
  uint8_t mac[6];if(esp_efuse_mac_get_default(mac)!=ESP_OK){error="MAC de fábrica indisponível.";return false;}
  // Called before Wi-Fi/ADC initialization: explicitly enable the entropy source.
  bootloader_random_enable();bool ok=begin(mac,[](uint8_t* b,size_t n){esp_fill_random(b,n);});bootloader_random_disable();return ok;
#else
  const uint8_t mac[]={0x12,0x34,0x56,0x78,0x9a,0xbc};return begin(mac,[](uint8_t* b,size_t n){for(size_t i=0;i<n;i++)b[i]=(uint8_t)(i*17+3);});
#endif
 }
 bool valid()const{return ready_;}
 bool bind(const String& value,const char* mode){
  if(!ready_)return false;const char* key=!strcmp(mode,"mock")?"mockBinding":"realBinding";
  if(prefs.isKey(key)){if(prefs.getString(key,"")!=value.c_str()){error="Vínculo incompatível; reprovisionamento necessário.";return false;}return true;}
  if(prefs.putString(key,value)!=value.length()||prefs.getString(key,"")!=value.c_str()){error="Falha ao persistir vínculo.";return false;}return true;
 }
 String premiumLink(const CloudConfig& c)const{if(!valid()||!httpsUrl(c.signupUrl))return "";return String(c.signupUrl)+"?clientId="+encode(clientId.c_str())+"&deviceId="+encode(deviceId.c_str());}
};
}
