#pragma once
#include "Arduino.h"
class Preferences {
public:
 static inline String config,legacy;static inline uint64_t next=1;static inline bool fail=false;
 bool begin(const char*,bool){return !fail;}
 String getString(const char* key,const char* fallback){const auto& value=strcmp(key,"presenceConfig")==0?config:legacy;return value.length()?value:String(fallback);}
 size_t putString(const char* key,const String& v){if(fail)return 0;if(strcmp(key,"presenceConfig")==0)config=v;else legacy=v;return v.length();}
 uint64_t getULong64(const char*,uint64_t){return next;}
 size_t putULong64(const char*,uint64_t v){if(fail)return 0;next=v;return sizeof(v);}
};
