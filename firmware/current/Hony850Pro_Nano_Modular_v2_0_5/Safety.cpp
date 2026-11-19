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
 if(i.tcFresh && i.raw>=Cfg::RAW_HARD_LIMIT)return Fault::OverTemp;
 if(!i.heatRequest)return Fault::None;
 if(!i.tcFresh)return Fault::Thermocouple;
 if(i.spikeUp || i.spikeDown)return Fault::TcSpike;
 if(!i.zcOK)return Fault::ZeroCross;
 if(!i.active){
   runawaySince_=0;noRiseSince_=0;inBandSince_=0;lostSince_=0;
   return Fault::None;
 }
 const float err=i.targetRaw-i.raw;
 // El autotune no tiene gracia termica: un sobrepaso grave corta ya.
 if(i.tuning && i.raw>=i.targetRaw+30.0f)return Fault::Runaway;
 const float limit=35.0f;
 if(i.raw>=i.targetRaw+limit && (int32_t)(now-graceTo_)>=0){
   if(!runawaySince_)runawaySince_=now;
   if((uint32_t)(now-runawaySince_)>=750UL)return Fault::Runaway;
 }else runawaySince_=0;
 if(i.tuning)return Fault::None;
 if((int32_t)(now-graceTo_)<0)return Fault::None;
 if(i.duty>=25 && err>=20){
   if(!noRiseSince_){noRiseSince_=now;noRiseStart_=i.raw;}
   if((uint32_t)(now-noRiseSince_)>=8000UL){
     if(i.raw-noRiseStart_<3)return Fault::NoRise;
     noRiseSince_=now;noRiseStart_=i.raw;
   }
 }else noRiseSince_=0;
 if(fabs(err)<=10){
   if(!inBandSince_)inBandSince_=now;
   if((uint32_t)(now-inBandSince_)>=10000UL)stable_=true;
 }else inBandSince_=0;
 if(stable_ && fabs(err)>=40){
   if(!lostSince_)lostSince_=now;
   if((uint32_t)(now-lostSince_)>=10000UL)return Fault::ControlLost;
 }else lostSince_=0;
 return Fault::None;
}
