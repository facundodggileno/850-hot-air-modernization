#pragma once
#include "Types.h"
class PidControl {
 public:
  PidControl();
  void reset();
  void setGains(const Gains& g){gains_=g;reset();}
  Gains gains()const{return gains_;}
  uint32_t lastIntervalMs()const{return lastDtMs_;}
  uint8_t update(float outlet,float smoothed,float rate,float targetOutlet,uint8_t air,uint32_t now);
 private:
  Gains gains_;
  float integral_;
  uint32_t lastMs_,lastDtMs_;
};
