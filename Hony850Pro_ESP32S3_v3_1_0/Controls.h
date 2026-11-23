#pragma once
#include "Config.h"
struct InputFrame {
  int8_t tempStep,airStep;
  bool tempShort,tempLong,airShort,airLong,airHold10;
  bool bothDown,bothLong;
  bool standRaw,standPressed;
};
class Controls {
 public:
  Controls();
  void begin();
  InputFrame poll(uint32_t now);
 private:
  struct Button {uint8_t pin;bool raw,stable,longSent,holdSent;uint32_t changed,pressed;};
  Button btn_[2];
  uint8_t oldTempClk_,oldAirClk_;
  bool standRaw_,standStable_;
  uint32_t standChanged_,bothAt_;
  void pollButton(Button &b,uint32_t now,bool &shortEvent,bool &longEvent,
                  bool &hold10Event,bool isAir);
};
