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
  uint32_t sequence()const{return sampleSequence_;}
  uint32_t samplePeriodMs()const{return lastSamplePeriodMs_;}
  uint32_t readDurationUs()const{return readDurationUs_;}
 private:
  MAX6675 max_;
  SensorSample s_;
  uint32_t lastAt_,lastReadCompletedAtMs_,sampleSequence_,lastSamplePeriodMs_,readDurationUs_;
  float previous1_,previous2_;
  bool spikeUp_,spikeDown_;
};
