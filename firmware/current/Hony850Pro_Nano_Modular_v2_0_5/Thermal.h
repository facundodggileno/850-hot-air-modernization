#pragma once
#include "Types.h"
#include "Config.h"
#include <max6675.h>

class Thermal {
 public:
  Thermal();
  void tick(uint32_t now);
  bool fresh(uint32_t now)const;
  const SensorSample& sample()const{return s_;}
  bool bigRise()const{return spikeUp_;}
  bool bigDrop()const{return spikeDown_;}
 private:
  MAX6675 max_;
  SensorSample s_;
  uint32_t lastAt_;
  float previous1_,previous2_;
  bool spikeUp_,spikeDown_;
};
