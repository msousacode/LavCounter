#pragma once
#include <stdint.h>
inline uint64_t fakeTime=0;
inline int64_t esp_timer_get_time(){return fakeTime;}
