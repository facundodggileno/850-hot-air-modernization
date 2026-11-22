#pragma once
#include <Arduino.h>

enum class Fault : uint8_t {
  None=0, Thermocouple=1, OverTemp=2, Runaway=3, NoRise=4,
  ControlLost=5, Watchdog=6, ZeroCross=7, Stall=8, Lcd=9,
  SelfTest=10, PumpGate=11, ResetLoop=12, EStop=13, TcSpike=14
};
enum class View : uint8_t {Normal, PidMenu, TuneSetup, Tuning, PidView, Fault};
enum class ArmStage : uint8_t {Idle, FullFlow, PhaseVerify, Settle, Ready};
struct Gains { float kp, ki, kd; };
struct SensorSample {
  float raw, lastGood, controlled, rate;
  uint32_t lastGoodAt;
  uint8_t consecutive;
  bool valid, lastAccepted;
  SensorSample():raw(NAN),lastGood(NAN),controlled(NAN),rate(0),lastGoodAt(0),
      consecutive(0),valid(false),lastAccepted(false){}
};
