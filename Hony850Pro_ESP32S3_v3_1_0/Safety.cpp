#include "Safety.h"
#include "Config.h"
#include <math.h>
Safety::Safety():graceTo_(0),runawaySince_(0),noRiseSince_(0),inBandSince_(0),
 lostSince_(0),noRiseStart_(0),stable_(false){}
void Safety::reset(uint32_t now){
 graceTo_=now+HONY_GRACE_AFTER_START_MS;runawaySince_=0;noRiseSince_=0;
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
 if(i.tuning && i.temperatureC>=i.targetC+HONY_TUNE_OVERSHOOT_C)return Fault::Runaway;
 const float limit=HONY_OVERSHOOT_C;
 if(i.temperatureC>=i.targetC+limit && (int32_t)(now-graceTo_)>=0){
   if(!runawaySince_)runawaySince_=now;
   if((uint32_t)(now-runawaySince_)>=HONY_OVERSHOOT_CONFIRM_MS)return Fault::Runaway;
 }else runawaySince_=0;
 if(i.tuning)return Fault::None;
 if((int32_t)(now-graceTo_)<0)return Fault::None;
 if(i.duty>=HONY_NO_RISE_DUTY_MIN && err>=HONY_NO_RISE_ERROR_C){
   if(!noRiseSince_){noRiseSince_=now;noRiseStart_=i.temperatureC;}
   if((uint32_t)(now-noRiseSince_)>=HONY_NO_RISE_WINDOW_MS){
     if(i.temperatureC-noRiseStart_<HONY_NO_RISE_MIN_DELTA_C)return Fault::NoRise;
     noRiseSince_=now;noRiseStart_=i.temperatureC;
   }
 }else noRiseSince_=0;
 if(fabs(err)<=HONY_STABLE_ERROR_BAND_C){
   if(!inBandSince_)inBandSince_=now;
   if((uint32_t)(now-inBandSince_)>=HONY_STABLE_PERIOD_MS)stable_=true;
 }else inBandSince_=0;
 if(stable_ && fabs(err)>=HONY_CONTROL_LOST_ERROR_C){
   if(!lostSince_)lostSince_=now;
   if((uint32_t)(now-lostSince_)>=HONY_CONTROL_LOST_PERIOD_MS)return Fault::ControlLost;
 }else lostSince_=0;
 return Fault::None;
}
