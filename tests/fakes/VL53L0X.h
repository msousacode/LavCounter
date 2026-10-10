#ifndef VL53L0X_h
#define VL53L0X_h
#include "Wire.h"
class VL53L0X {
public:
 enum {RESULT_INTERRUPT_STATUS=0x13,RESULT_RANGE_STATUS=0x14,SYSTEM_INTERRUPT_CLEAR=0x0B,IDENTIFICATION_MODEL_ID=0xC0};
 enum RangeStatus {RangeValid=11,Invalid=4};
 static inline bool &connected=Wire.sensorConnected,&available=Wire.sensorAvailable;
 static inline bool timedOut=false;
 static inline uint16_t &mm=Wire.sensorMm;
 static inline uint8_t &status=Wire.sensorStatus;
 static inline uint8_t &modelId=Wire.sensorModelId;
 uint8_t last_status=0;
 uint8_t getAddress(){return 0x29;}
 void setTimeout(int){}
 bool init(){last_status=connected?0:1;return connected&&modelId==0xEE;}
 uint8_t readReg(uint8_t){last_status=connected?0:1;return modelId;}
 bool setMeasurementTimingBudget(int){return true;}
 void startContinuous(int){}
 void writeReg(uint8_t,uint8_t){last_status=connected&&!Wire.sensorClearFail?0:1;}
 bool timeoutOccurred(){bool result=timedOut;timedOut=false;return result;}
};
#endif
