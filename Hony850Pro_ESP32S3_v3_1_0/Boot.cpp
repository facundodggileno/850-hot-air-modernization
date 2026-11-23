#include "Boot.h"
#include "Config.h"
Boot::Boot():stage_(Stage::Sensor),warnings_(0),stageSince_(0),zcBaseline_(0){}
void Boot::begin(uint32_t now,bool lcdOk,bool ramOk,bool wdt){
 stage_=Stage::Sensor;warnings_=0;stageSince_=now;zcBaseline_=0;
 if(!lcdOk)warnings_|=LCD;
 if(!ramOk)warnings_|=RAM;
 if(wdt)warnings_|=WDT;
}
void Boot::tick(uint32_t now,const SensorSample& t,bool zcOk,Hardware::Counters c){
 switch(stage_){
 case Stage::Sensor:
   if(t.valid || (uint32_t)(now-stageSince_)>=Cfg::BOOT_TC_MS){
     if(!t.valid)warnings_|=TC;
     stage_=Stage::Zc;stageSince_=now;zcBaseline_=c.edges;
   }
   break;
 case Stage::Zc:
   if((uint32_t)(now-stageSince_)>=Cfg::BOOT_ZC_MS){
     const uint32_t n=c.edges-zcBaseline_;
     if(n<HONY_ZC_BOOT_MIN_PULSES || n>HONY_ZC_BOOT_MAX_PULSES || !zcOk)warnings_|=ZC;
     stage_=(warnings_?Stage::Warnings:Stage::Done);stageSince_=now;
   }
   break;
 case Stage::Warnings:
   if((uint32_t)(now-stageSince_)>=Cfg::BOOT_WARN_MS)stage_=Stage::Done;
   break;
 case Stage::Done:break;
 }
}
uint8_t Boot::activeWarning(uint32_t now)const{
 if(stage_!=Stage::Warnings || !warnings_)return 0;
 uint8_t n=0;
 for(uint8_t i=0;i<5;i++)if(warnings_ & (1U<<i))n++;
 if(!n)return 0;
 const uint32_t elapsed=now-stageSince_;
 uint8_t selected=(uint8_t)((elapsed*n)/Cfg::BOOT_WARN_MS);
 if(selected>=n)selected=n-1;
 for(uint8_t i=0;i<5;i++)if(warnings_ & (1U<<i)){
   if(!selected--)return (uint8_t)(1U<<i);
 }
 return 0;
}
