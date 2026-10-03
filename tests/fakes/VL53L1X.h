#pragma once
#include "Arduino.h"
class VL53L1X {
public:
 enum DistanceMode {Long};enum RangeStatus {RangeValid,Invalid};
 static inline bool connected=true,available=true,timedOut=false;static inline uint16_t mm=2000;static inline RangeStatus status=RangeValid;
 struct {RangeStatus range_status=RangeValid;} ranging_data;uint8_t last_status=0;
 void setTimeout(int){}bool init(){last_status=connected?0:1;return connected;}bool setDistanceMode(DistanceMode){return true;}bool setMeasurementTimingBudget(int){return true;}void startContinuous(int){}
 bool dataReady(){last_status=connected?0:1;return available;}uint16_t read(bool){ranging_data.range_status=status;return mm;}bool timeoutOccurred(){return timedOut;}
};

