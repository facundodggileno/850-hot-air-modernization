#include "AutoTune.h"
#include "PidStore.h"
#include <math.h>
AutoTune::AutoTune():status_(TuneStatus::Idle),heating_(true),warmed_(false),
 cycles_(0),highDuty_(60),target_(0),lowPeak_(0),highPeak_(0),pendingLow_(0),
 amplitudeSum_(0),periodSum_(0),began_(0),phaseAt_(0),lastUpper_(0),result_({0,0,0}){}
void AutoTune::begin(float targetOutlet,uint8_t air,uint32_t now){
 target_=targetOutlet;highDuty_=air<HONY_AUTOTUNE_MAX_DUTY?air:HONY_AUTOTUNE_MAX_DUTY;
 status_=TuneStatus::Running;heating_=true;warmed_=false;cycles_=0;
 lowPeak_=1000;highPeak_=-1000;pendingLow_=0;amplitudeSum_=0;periodSum_=0;
 began_=now;phaseAt_=now;lastUpper_=0;
}
void AutoTune::abort(){status_=TuneStatus::Idle;}
uint8_t AutoTune::tick(float outlet,uint32_t now){
 if(status_!=TuneStatus::Running)return 0;
 if((uint32_t)(now-began_)>HONY_AUTOTUNE_TOTAL_TIMEOUT_MS ||
    (uint32_t)(now-phaseAt_)>HONY_AUTOTUNE_PHASE_TIMEOUT_MS){status_=TuneStatus::Timeout;return 0;}
 if(heating_){
   if(outlet<lowPeak_)lowPeak_=outlet;
   if(outlet>=target_+HONY_AUTOTUNE_UPPER_DELTA_C){
     if(warmed_){
       pendingLow_=lowPeak_;
       if(lastUpper_){
         const float period=(float)(now-lastUpper_)/1000.0f;
         if(period<HONY_AUTOTUNE_PERIOD_MIN_S || period>HONY_AUTOTUNE_PERIOD_MAX_S){status_=TuneStatus::Invalid;return 0;}
         periodSum_+=period;
       }
     }
     lastUpper_=now;highPeak_=outlet;heating_=false;phaseAt_=now;
     return 0;
   }
   return highDuty_;
 }
 if(outlet>highPeak_)highPeak_=outlet;
 if(outlet<=target_-HONY_AUTOTUNE_LOWER_DELTA_C){
   if(!warmed_)warmed_=true; // primera oscilacion de calentamiento descartada
   else{
     const float amplitude=(highPeak_-pendingLow_)*0.5f;
     if(amplitude<HONY_AUTOTUNE_AMPLITUDE_MIN_C || amplitude>HONY_AUTOTUNE_AMPLITUDE_MAX_C){status_=TuneStatus::Invalid;return 0;}
     amplitudeSum_+=amplitude;
     ++cycles_;
     if(cycles_>=Cfg::TUNE_CYCLES){
       const float avgAmp=amplitudeSum_/Cfg::TUNE_CYCLES;
       const float period=periodSum_/Cfg::TUNE_CYCLES;
       if(period<HONY_AUTOTUNE_PERIOD_MIN_S || period>HONY_AUTOTUNE_PERIOD_MAX_S){status_=TuneStatus::Invalid;return 0;}
       const float ku=4.0f*((float)highDuty_*0.5f)/(PI*avgAmp);
       result_.kp=ku/2.2f;
       result_.ki=result_.kp/(2.2f*period);
       result_.kd=result_.kp*period/6.3f;
       status_=PidStore::valid(result_)?TuneStatus::Done:TuneStatus::Invalid;
       return 0;
     }
   }
   lowPeak_=outlet;heating_=true;phaseAt_=now;
   return highDuty_;
 }
 return 0;
}
