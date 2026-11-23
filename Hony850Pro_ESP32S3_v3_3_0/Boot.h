#pragma once
#include "Types.h"
#include "Hardware.h"
class Boot {
 public:
  enum class Stage:uint8_t {Sensor,Zc,Warnings,Done};
  enum Warn:uint8_t {LCD=1,RAM=2,TC=4,ZC=8,WDT=16};
  Boot();
  void begin(uint32_t now,bool lcdOk,bool ramOk,bool watchdogReset);
  void tick(uint32_t now,const SensorSample& sample,bool zcOk,Hardware::Counters ct);
  Stage stage()const{return stage_;}
  bool done()const{return stage_==Stage::Done;}
  uint8_t warnings()const{return warnings_;}
  uint8_t activeWarning(uint32_t now)const;
 private:
  Stage stage_;
  uint8_t warnings_;
  uint32_t stageSince_,zcBaseline_;
};
