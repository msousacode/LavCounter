#pragma once
#include <assert.h>
#include "Wire.h"
class VL53L1X {
public:
 enum DistanceMode {Long};enum RangeStatus {RangeValid=0,Invalid=4};
 struct RangingData {uint16_t range_mm=0;RangeStatus range_status=Invalid;} ranging_data;
 uint8_t last_status=0;
 static inline bool timedOut=false;
 void setTimeout(int){}
 bool init(){last_status=Wire.sensorConnected?0:1;return Wire.sensorConnected&&Wire.sensorL1;}
 bool setDistanceMode(DistanceMode){return true;}
 bool setMeasurementTimingBudget(int){return true;}
 void startContinuous(int){}
 uint16_t read(bool blocking){assert(!blocking);last_status=Wire.sensorConnected&&!Wire.sensorClearFail?0:1;ranging_data.range_mm=Wire.sensorMm;ranging_data.range_status=RangeStatus(Wire.sensorStatus);return Wire.sensorMm;}
 bool timeoutOccurred(){bool result=timedOut;timedOut=false;return result;}
};
