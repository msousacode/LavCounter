#pragma once
#include "Arduino.h"
class Preferences {
 bool mock=false;
public:
 static inline String config,legacy;static inline uint64_t next=1;static inline bool fail=false;
 static inline String mockConfig;static inline uint64_t mockNext=1;
 bool begin(const char* name,bool){mock=strcmp(name,"lavcounterMock")==0;return !fail;}
 String getString(const char* key,const char* fallback){const auto& value=strcmp(key,"presenceConfig")==0?(mock?mockConfig:config):legacy;return value.length()?value:String(fallback);}
 size_t putString(const char* key,const String& v){if(fail)return 0;if(strcmp(key,"presenceConfig")==0)(mock?mockConfig:config)=v;else legacy=v;return v.length();}
 uint64_t getULong64(const char*,uint64_t){return mock?mockNext:next;}
 size_t putULong64(const char*,uint64_t v){if(fail)return 0;(mock?mockNext:next)=v;return sizeof(v);}
};
