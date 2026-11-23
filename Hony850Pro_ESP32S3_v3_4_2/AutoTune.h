#pragma once
#include "Types.h"
#include "Config.h"

enum class TuneStatus:uint8_t {Idle,Running,Done,Invalid,Timeout};
class AutoTune {
 public:
  AutoTune();
  void begin(float targetOutlet,uint8_t air,uint32_t now);
  void abort();
  uint8_t tick(float outlet,uint32_t now); // duty 0..65, run only when interlocks OK
  TuneStatus status()const{return status_;}
  uint8_t cycles()const{return cycles_;}
  Gains result()const{return result_;}
  bool heating()const{return heating_;}
 private:
  TuneStatus status_;
  bool heating_,warmed_;
  uint8_t cycles_,highDuty_;
  float target_,lowPeak_,highPeak_,pendingLow_,amplitudeSum_,periodSum_;
  uint32_t began_,phaseAt_,lastUpper_;
  Gains result_;
};
