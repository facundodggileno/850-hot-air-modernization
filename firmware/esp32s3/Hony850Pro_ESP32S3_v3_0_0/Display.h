#pragma once
#include "Types.h"
#include "Boot.h"
#include "AutoTune.h"
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
struct Screen {
 Boot::Stage boot;
 uint8_t warning;
 View view;
 Fault fault;
 const SensorSample* sensor;
 int setpoint,air,tuneSet,tuneAir;
 uint8_t duty,cycles,pidPage,pidMenuSelection;
 bool heatRequest,cooldown,preflow,loadedPid;
 uint16_t preflowSeconds;
 Gains gains;
 TuneStatus tuneStatus;
 bool tuneHeating;
};
class Display {
 public:
  bool begin();
  bool ready()const{return ready_;}
  void render(const Screen& s);
 private:
  // PCF8574T P0 RS, P2 E, P4-7 D4-7, P3 backlight. NO R/W.
  // LCD RW fijado fisicamente a GND. No hacer autodeteccion de R/W.
  hd44780_I2Cexp lcd_{0x27, I2Cexp_PCF8574, 0, 2, 4, 5, 6, 7, 3, HIGH};
  bool ready_=false;
  void line(uint8_t r,const char* str);
  void rawLine(char* dst,const SensorSample& s,bool count);
};
