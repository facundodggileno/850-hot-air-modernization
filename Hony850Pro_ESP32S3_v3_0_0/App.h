#pragma once
#include "Thermal.h"
#include "Hardware.h"
#include "Controls.h"
#include "PidControl.h"
#include "PidStore.h"
#include "AutoTune.h"
#include "Boot.h"
#include "Display.h"
#include "Safety.h"

class App {
 public:
  App();
  void begin();
  void tick();
 private:
  Thermal thermal_;
  Controls controls_;
  PidControl pid_;
  AutoTune tune_;
  Boot boot_;
  Display display_;
  Safety safety_;
  Gains gains_;
  TuneStatus lastTuneResult_;
  bool pidFromEEPROM_,ramOK_;
  Fault fault_;
  View view_;
  int setpoint_,air_,lastAir_,tuneSet_,tuneAir_;
  bool heatRequest_,manualPump_,cooldown_,stand_,bothWasDown_;
  uint8_t duty_,pidPage_,pidMenuSelection_,debugPage_;
  uint32_t diagnosticsAt_,diagAcceptedPerSec_,diagRejectedPerSec_;
  uint32_t diagGateOnPerSec_,diagGateOffPerSec_;
  Hardware::Counters diagnosticsBaseline_;
  bool pidViewFromMenu_;
  bool standTuneOverride_; // Vestigio de V2: V3 nunca habilita calor con stand.
  ArmStage arm_;
  uint32_t armSince_,phaseSince_,gateWindowSince_,
           lastLCD_,lastSerial_,lastPID_,lastTune_,lastFaultAt_;
  Hardware::Counters gateBaseline_;

  void trip(Fault why,uint32_t now);
  void cooldownStart();
  void startFlow(uint32_t now);
  void flowTick(uint32_t now);
  void beginHeat(uint32_t now);
  void beginTune(uint32_t now);
  void finishTune(uint32_t now);
  void handleInput(const InputFrame& e,uint32_t now);
  void controlTick(uint32_t now);
  void displayTick(uint32_t now);
  void serialTick(uint32_t now);
  void diagnosticsTick(uint32_t now);
  void serialCommand(uint32_t now);
  bool allowHeat(uint32_t now) const;
  bool standPermittedForTune() const;
  uint8_t pumpWanted(uint32_t now)const;
  bool canClearFault(uint32_t now)const;
  void clearFault(uint32_t now);
  void chirp(unsigned freq,unsigned ms);
};
