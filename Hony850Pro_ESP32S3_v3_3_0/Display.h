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
 uint8_t channel;
 bool channelSaveNotice,channelSaveOK;
 uint16_t preflowSeconds;
 Gains gains;
 TuneStatus tuneStatus;
 bool tuneHeating;
 // Diagnostico en LCD: contadores de sincronizacion y estado interno.
 uint8_t debugPage,pumpCmd,arm,resetReason;
 uint32_t zcAcceptedPerSec,zcRejectedPerSec,zcPeriodUs;
 uint32_t zcGateOnPerSec,zcGateOffPerSec,tcAgeMs;
 uint32_t freeHeapBytes,minHeapBytes,zcTotal,rejectedTotal;
 uint32_t tcPeriodMs,tcReadUs,pidPeriodMs;
 bool zcOK,zcInput,tcFresh,relayOn,standOn;
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
  // Cache de 16 caracteres por fila. No depende del pin R/W ni de lectura de LCD.
  char lastPrinted_[2][17]={{0},{0}};
  bool rowValid_[2]={false,false};
  void line(uint8_t r,const char* str);
  void tempLine(char* dst,const SensorSample& s,bool count);
};
