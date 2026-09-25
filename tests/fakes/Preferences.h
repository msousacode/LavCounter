#pragma once
#include "Arduino.h"
class Preferences {
public:
 static inline String config;static inline uint64_t next=1;static inline bool fail=false;
 bool begin(const char*,bool){return !fail;}
 String getString(const char*,const char* fallback){return config.length()?config:String(fallback);}
 size_t putString(const char*,const String& v){if(fail)return 0;config=v;return v.length();}
 uint64_t getULong64(const char*,uint64_t){return next;}
 size_t putULong64(const char*,uint64_t v){if(fail)return 0;next=v;return sizeof(v);}
};
