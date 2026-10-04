#pragma once
#include "FirestoreTransport.h"
#ifdef ARDUINO_ARCH_ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#endif
namespace lavcloud {
struct CloudJob {CloudConfig config;String binding,name,body;bool checkOnly=false;};
class CloudWorker {public:virtual ~CloudWorker()=default;virtual bool begin()=0;virtual bool submit(const CloudJob&)=0;virtual bool poll(CloudResult&)=0;virtual uint64_t utc()const=0;};
class EspCloudWorker:public CloudWorker {
 FirestoreTransport& transport;
#ifdef ARDUINO_ARCH_ESP32
 QueueHandle_t input=nullptr,output=nullptr;TaskHandle_t task=nullptr;
 // Queues carry signals only: Strings never undergo FreeRTOS byte-copy ownership transfer.
 CloudJob job;CloudResult result;
 static void run(void* self){auto& w=*static_cast<EspCloudWorker*>(self);uint8_t signal;
  for(;;){if(xQueueReceive(w.input,&signal,portMAX_DELAY)==pdTRUE){
   if(time(nullptr)<1700000000){configTime(0,0,"pool.ntp.org","time.google.com");uint64_t start=esp_timer_get_time();while(time(nullptr)<1700000000&&esp_timer_get_time()-start<10000000)vTaskDelay(pdMS_TO_TICKS(100));}
   w.result=w.transport.execute(w.job.config,w.job.binding,w.utc(),w.job.name,w.job.body,w.job.checkOnly);xQueueSend(w.output,&signal,portMAX_DELAY);
  }}
 }
#endif
 bool busy=false;
public:
 explicit EspCloudWorker(FirestoreTransport& t):transport(t){}
 bool begin()override{
#ifdef ARDUINO_ARCH_ESP32
  input=xQueueCreate(1,sizeof(uint8_t));output=xQueueCreate(1,sizeof(uint8_t));if(!input||!output){if(input)vQueueDelete(input);if(output)vQueueDelete(output);input=output=nullptr;return false;}
  if(xTaskCreate(run,"cloud-sync",24576,this,1,&task)!=pdPASS){vQueueDelete(input);vQueueDelete(output);input=output=nullptr;return false;}return true;
#else
  return true;
#endif
 }
 bool submit(const CloudJob& value)override{
#ifdef ARDUINO_ARCH_ESP32
  if(!task||busy)return false;job=value;uint8_t signal=1;busy=true;if(xQueueSend(input,&signal,0)!=pdTRUE){busy=false;return false;}return true;
#else
  (void)value;return false;
#endif
 }
 bool poll(CloudResult& value)override{
#ifdef ARDUINO_ARCH_ESP32
  uint8_t signal;if(busy&&xQueueReceive(output,&signal,0)==pdTRUE){value=result;busy=false;return true;}
#else
  (void)value;
#endif
  return false;
 }
 uint64_t utc()const override{
#ifdef ARDUINO_ARCH_ESP32
  return (uint64_t)time(nullptr);
#else
  return 0;
#endif
 }
};
}
