#pragma once
#include "Types.h"
struct SafetyInput {
  bool heatRequest,active,tuning,tcFresh,zcOK,stalled,spikeUp,spikeDown;
  float internalRaw,temperatureC,targetC;
  uint8_t duty;
};
class Safety {
 public:
  Safety();
  void reset(uint32_t now);
  Fault check(const SafetyInput& i,uint32_t now);
 private:
  uint32_t graceTo_,runawaySince_,noRiseSince_,inBandSince_,lostSince_;
  float noRiseStart_;
  bool stable_;
};
