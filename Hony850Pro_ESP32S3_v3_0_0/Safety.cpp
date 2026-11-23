#include "Safety.h"
#include "Config.h"
#include <math.h>
Safety::Safety():graceTo_(0),runawaySince_(0),noRiseSince_(0),inBandSince_(0),
 lostSince_(0),noRiseStart_(0),stable_(false){}
void Safety::reset(uint32_t now){
 graceTo_=now+5000UL;runawaySince_=0;noRiseSince_=0;
 inBandSince_=0;lostSince_=0;noRiseStart_=0;stable_=false;
}
Fault Safety::check(const SafetyInput& i,uint32_t now){
 if(i.stalled)return Fault::Stall;
 // Internal sensor enforces the independent high-temperature protection.
 if(i.tcFresh && i.internalRaw>=Cfg::RAW_HARD_LIMIT)return Fault::OverTemp;
 if(!i.heatRequest)return Fault::None;
 if(!i.tcFresh)return Fault::Thermocouple;
 if(i.spikeUp || i.spikeDown)return Fault::TcSpike;
 if(!i.zcOK)return Fault::ZeroCross;
 if(!i.active){
   runawaySince_=0;noRiseSince_=0;inBandSince_=0;lostSince_=0;
   return Fault::None;
 }
 const float err=i.targetC-i.temperatureC;
 // El autotune no tiene gracia termica: un sobrepaso grave corta ya.
 if(i.tuning && i.temperatureC>=i.targetC+20.0f)return Fault::Runaway;
 const float limit=25.0f;
 if(i.temperatureC>=i.targetC+limit && (int32_t)(now-graceTo_)>=0){
   if(!runawaySince_)runawaySince_=now;
   if((uint32_t)(now-runawaySince_)>=750UL)return Fault::Runaway;
 }else runawaySince_=0;
 if(i.tuning)return Fault::None;
 if((int32_t)(now-graceTo_)<0)return Fault::None;
 if(i.duty>=25 && err>=14){
   if(!noRiseSince_){noRiseSince_=now;noRiseStart_=i.temperatureC;}
   if((uint32_t)(now-noRiseSince_)>=8000UL){
     if(i.temperatureC-noRiseStart_<2.0f)return Fault::NoRise;
     noRiseSince_=now;noRiseStart_=i.temperatureC;
   }
 }else noRiseSince_=0;
 if(fabs(err)<=7){
   if(!inBandSince_)inBandSince_=now;
   if((uint32_t)(now-inBandSince_)>=10000UL)stable_=true;
 }else inBandSince_=0;
 if(stable_ && fabs(err)>=28){
   if(!lostSince_)lostSince_=now;
   if((uint32_t)(now-lostSince_)>=10000UL)return Fault::ControlLost;
 }else lostSince_=0;
 return Fault::None;
}
