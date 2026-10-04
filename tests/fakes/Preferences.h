#pragma once
#include "Arduino.h"
class Preferences {
 bool mock=false;
 String name;
public:
 struct Entry {String key,value;};static inline Entry identity[8];
 static inline String config,legacy;static inline uint64_t next=1;static inline bool fail=false;
 static inline String mockConfig;static inline uint64_t mockNext=1;
 bool begin(const char* ns,bool){name=ns;mock=strcmp(ns,"lavcounterMock")==0;return !fail;}
 bool isKey(const char* key){if(name=="lavIdentity"){for(auto& e:identity)if(e.key==key)return true;return false;}return false;}
 String getString(const char* key,const char* fallback){if(name=="lavIdentity"){for(auto& e:identity)if(e.key==key)return e.value;return fallback;}const auto& value=strcmp(key,"presenceConfig")==0?(mock?mockConfig:config):legacy;return value.length()?value:String(fallback);}
 size_t putString(const char* key,const String& v){if(fail)return 0;if(name=="lavIdentity"){for(auto& e:identity)if(e.key==key||e.key.isEmpty()){e.key=key;e.value=v;return v.length();}return 0;}if(strcmp(key,"presenceConfig")==0)(mock?mockConfig:config)=v;else legacy=v;return v.length();}
 uint64_t getULong64(const char*,uint64_t){return mock?mockNext:next;}
 size_t putULong64(const char*,uint64_t v){if(fail)return 0;(mock?mockNext:next)=v;return sizeof(v);}
};
