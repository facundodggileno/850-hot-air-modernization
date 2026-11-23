#include "App.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
namespace {
int clampInt(int x,int a,int b){return x<a?a:(x>b?b:x);}
int freeRam(){return (int)ESP.getFreeHeap();}
}
App::App():gains_({2.932f,0.247f,2.510f}),lastTuneResult_(TuneStatus::Idle),pidFromEEPROM_(false),ramOK_(true),
 fault_(Fault::None),view_(View::Normal),setpoint_(250),air_(100),lastAir_(100),
 tuneSet_(250),tuneAir_(100),heatRequest_(false),manualPump_(true),
 cooldown_(false),stand_(true),bothWasDown_(false),duty_(0),pidPage_(0),pidMenuSelection_(0),debugPage_(0),diagnosticsAt_(0),
 diagAcceptedPerSec_(0),diagRejectedPerSec_(0),diagGateOnPerSec_(0),diagGateOffPerSec_(0),
 diagnosticsBaseline_({0,0,0,0}),pidViewFromMenu_(false),standTuneOverride_(false),
 arm_(ArmStage::Idle),armSince_(0),phaseSince_(0),gateWindowSince_(0),
 lastLCD_(0),lastSerial_(0),lastPID_(0),lastTune_(0),lastFaultAt_(0),
 gateBaseline_({0,0,0,0}){}
void App::chirp(unsigned f,unsigned d){tone(Cfg::BUZZER,f,d);}
void App::begin(){
 Hardware::begin();controls_.begin();
 // Pantalla con PCF8574T a 3.3V y RW LCD conectado a GND.
 Serial.begin(115200);
 Wire.begin(Cfg::LCD_SDA,Cfg::LCD_SCL,100000);
 bool lcdOK=display_.begin();
 pidFromEEPROM_=PidStore::load(gains_);
 pid_.setGains(gains_);
 stand_=digitalRead(Cfg::STAND)==HIGH;
 ramOK_=freeRam()>=8192;
 const bool wdtBoot=Hardware::resetReason()!=0;
 boot_.begin(millis(),lcdOK,ramOK_,wdtBoot);
 if(wdtBoot){fault_=Fault::Watchdog;view_=View::Fault;}
 safety_.reset(millis());
 Hardware::heatOff();Hardware::pump(100);
 Hardware::watchdogStart();
 Serial.println(F("HONY 850 PRO MODULAR V3.1.0 | ESP32-S3"));
 Serial.print(F("RESET_FLAGS="));Serial.println(Hardware::resetReason(),HEX);
 Serial.print(F("SRAM_FREE="));Serial.println(freeRam());
 Serial.print(F("PID_NVS="));Serial.println(pidFromEEPROM_?1:0);
 Serial.println(F("ms,outlet_est_c,sp,air,pump,duty,heat,arm,fault,zc,boot,ram,tune"));
 chirp(2000,60);
}
void App::trip(Fault why,uint32_t now){
 if(why==Fault::None || fault_!=Fault::None)return;
 // Cortar potencia ANTES de emitir serial, LCD o audio.
 Hardware::heatOff();duty_=0;heatRequest_=false;arm_=ArmStage::Idle;
 standTuneOverride_=false;
 fault_=why;view_=View::Fault;lastFaultAt_=now;
 tune_.abort();
 Hardware::pump(100);
 cooldown_=true;
 Serial.print(F("FAULT,"));Serial.print((unsigned)fault_);
 Serial.print(',');Serial.print(now);
 Serial.print(F(",OUT_EST="));Serial.println(thermal_.sample().outletEst,2);
 chirp(360,300);
}
void App::cooldownStart(){
  Hardware::heatOff();duty_=0;heatRequest_=false;arm_=ArmStage::Idle;
  standTuneOverride_=false;
  tune_.abort();cooldown_=true;manualPump_=false;
 Hardware::pump(100);pid_.reset();
}
void App::startFlow(uint32_t now){
 Hardware::heatOff();duty_=0;
 Hardware::pump(100);
 arm_=ArmStage::FullFlow;armSince_=now;phaseSince_=now;gateWindowSince_=now;
 safety_.reset(now);pid_.reset();
}
bool App::standPermittedForTune() const {
  // En V3 el mango apoyado siempre deshabilita la calefaccion.
  return false;
}
void App::flowTick(uint32_t now){
  if(!heatRequest_ || fault_!=Fault::None || !boot_.done() ||
     (stand_ && !standPermittedForTune())){
   arm_=ArmStage::Idle;return;
 }
 if(!thermal_.fresh(now) || !Hardware::zcHealthy() || air_<Cfg::AIR_MIN_HEAT)return;
 const uint32_t elapsed=now-armSince_;
 if(arm_==ArmStage::FullFlow && elapsed>=1000UL){
   arm_=ArmStage::PhaseVerify;phaseSince_=now;
   gateBaseline_=Hardware::counters();
 }else if(arm_==ArmStage::PhaseVerify && (uint32_t)(now-phaseSince_)>=Cfg::PUMP_GATE_TEST_MS){
   if(air_<100){
     const Hardware::Counters c=Hardware::counters();
     const uint32_t on=c.gateOn-gateBaseline_.gateOn,off=c.gateOff-gateBaseline_.gateOff;
     if(on<70 || off<70 || (on>off?on-off:off-on)>4){trip(Fault::PumpGate,now);return;}
   }
   arm_=ArmStage::Settle;
 }else if(arm_==ArmStage::Settle && elapsed>=Cfg::PREFLOW_MS){
   arm_=ArmStage::Ready;
   gateWindowSince_=now;gateBaseline_=Hardware::counters();
   safety_.reset(now);
 }else if(arm_==ArmStage::Ready && air_<100 && (uint32_t)(now-gateWindowSince_)>=500UL){
   const Hardware::Counters c=Hardware::counters();
   const uint32_t on=c.gateOn-gateBaseline_.gateOn,off=c.gateOff-gateBaseline_.gateOff;
   if(on<35 || off<35 || (on>off?on-off:off-on)>4){trip(Fault::PumpGate,now);return;}
   gateBaseline_=c;gateWindowSince_=now;
 }
}
void App::beginHeat(uint32_t now){
 if(!Cfg::setpointWithinRawLimit(setpoint_)){
   Hardware::heatOff();
   Serial.printf("START_DENIED: %d C fuera de rango calibrado (max %d C de salida)\n",
                 setpoint_,Cfg::MAX_CALIBRATED_OUTLET_C);
   chirp(400,160);return;
 }
 if(!Cfg::MAINS_OUTPUTS_ENABLED || !Hardware::ready() ||
    !boot_.done() || fault_!=Fault::None || !ramOK_ || !display_.ready() ||
    !thermal_.fresh(now) || stand_ || air_<Cfg::AIR_MIN_HEAT ||
    !Hardware::zcHealthy() || thermal_.sample().raw>=Cfg::RAW_HARD_LIMIT){
   chirp(400,160);Serial.println(F("START_DENIED: SENSOR/ZC/STAND/AIR/LCD/RAM/FAULT"));return;
 }
 cooldown_=false;heatRequest_=true;manualPump_=true;
 startFlow(now);chirp(2300,70);
}
void App::beginTune(uint32_t now){
  if(!Cfg::setpointWithinRawLimit(tuneSet_)){
    Hardware::heatOff();
    Serial.printf("AUTOTUNE_DENIED: %d C fuera de rango calibrado (max %d C)\n",
                  tuneSet_,Cfg::MAX_CALIBRATED_OUTLET_C);
    chirp(420,200);return;
  }
  if(!Cfg::MAINS_OUTPUTS_ENABLED || !Hardware::ready() || stand_ ||
     heatRequest_ || !boot_.done() || fault_!=Fault::None || !thermal_.fresh(now) ||
    !display_.ready() || !ramOK_ || !Hardware::zcHealthy() ||
    tuneAir_<Cfg::AIR_MIN_HEAT || thermal_.sample().raw>=Cfg::RAW_HARD_LIMIT){
    chirp(420,200);return;
  }
  setpoint_=tuneSet_;air_=tuneAir_;lastAir_=air_;
  cooldown_=false;heatRequest_=true;manualPump_=true;
  // El unico permiso excepcional de stand se concede en este instante
  // y se revoca en finishTune(), cooldownStart() y trip().
  standTuneOverride_=false;
  tune_.begin((float)setpoint_,(uint8_t)air_,now);
 lastTuneResult_=TuneStatus::Running;
 view_=View::Tuning;startFlow(now);chirp(2100,90);
 Serial.println(F("AUTOTUNE START 3 CICLOS"));
}
void App::finishTune(uint32_t now){
 const TuneStatus result=tune_.status();
 lastTuneResult_=result;
  Hardware::heatOff();heatRequest_=false;arm_=ArmStage::Idle;duty_=0;
  standTuneOverride_=false;
  if(result==TuneStatus::Done){
   const Gains g=tune_.result();
   if(PidStore::save(g)){
     gains_=g;pid_.setGains(gains_);pidFromEEPROM_=true;
     Serial.println(F("AUTOTUNE OK,PID GUARDADO NVS"));
     chirp(2600,150);
   }else Serial.println(F("AUTOTUNE NVS DENIED"));
 }else{
   Serial.print(F("AUTOTUNE FIN,STATUS="));Serial.println((unsigned)result);
   chirp(430,220);
 }
 if(!thermal_.fresh(now) || thermal_.sample().outletEst>=Cfg::OUTLET_COOL_START_C)
   cooldownStart();
 view_=View::PidView;pidPage_=0;pidViewFromMenu_=true;
}
bool App::canClearFault(uint32_t now)const{
 const SensorSample& s=thermal_.sample();
 return thermal_.fresh(now) && s.outletEst<=Cfg::OUTLET_COOL_STOP_C &&
        Hardware::zcHealthy() && !Hardware::stalled();
}
void App::clearFault(uint32_t now){
 if(!canClearFault(now)){chirp(400,150);return;}
  fault_=Fault::None;view_=View::Normal;
  cooldown_=false;heatRequest_=false;arm_=ArmStage::Idle;standTuneOverride_=false;
 Hardware::heatOff();pid_.reset();safety_.reset(now);
 Serial.println(F("FAULT REARM MANUAL / HEAT OFF"));chirp(2200,100);
}
void App::handleInput(const InputFrame& e,uint32_t now){
 stand_=e.standPressed;
  if(stand_ && heatRequest_ && !standPermittedForTune()){
   cooldownStart();
   if(view_==View::Tuning)view_=View::Normal;
  }
 if(e.bothDown){
   Hardware::heatOff();duty_=0;
   if(e.bothLong)trip(Fault::EStop,now);
   bothWasDown_=true;return;
 }
 if(bothWasDown_){
   bothWasDown_=false;
   if(heatRequest_)startFlow(now); // nunca recuperar calor instantaneamente.
   return;
 }
 if(!boot_.done())return;
 // DIAGNOSTICO es de SOLO LECTURA, accesible incluso durante un fallo.
 if(view_==View::Debug){
   if(e.airLong){view_=fault_==Fault::None?View::Normal:View::Fault;return;}
   if(e.airShort || e.tempLong){
     view_=fault_==Fault::None?View::PidMenu:View::Fault;
     return;
   }
   if(e.tempShort){
     debugPage_=(uint8_t)((debugPage_+1)%Cfg::DEBUG_PAGES);
   }else{
     const int8_t step=e.tempStep?e.tempStep:e.airStep;
     if(step){
       int page=(int)debugPage_+(step>0?1:-1);
       if(page<0)page=Cfg::DEBUG_PAGES-1;
       if(page>=Cfg::DEBUG_PAGES)page=0;
       debugPage_=(uint8_t)page;
     }
   }
   return;
 }
 if(view_==View::Fault || fault_!=Fault::None){
   if(e.airLong){debugPage_=0;view_=View::Debug;return;}
   if(e.tempLong)clearFault(now);
   return;
 }
  // En todos los menus: AIR toque = ATRAS; TEMP toque = OK/seleccionar.
  // AIR largo abre el menu desde el modo normal (nunca calor por esa tecla).
  if(view_==View::PidMenu){
   const int8_t nav=e.tempStep?e.tempStep:e.airStep;
   if(nav){
     int next=(int)pidMenuSelection_+(nav>0?1:-1);
     if(next<0)next=2;
     if(next>2)next=0;
     pidMenuSelection_=(uint8_t)next;
   }
   if(e.airShort || e.tempLong){view_=View::Normal;return;}
   if(e.tempShort){
     if(pidMenuSelection_==0){
       pidPage_=0;lastTuneResult_=TuneStatus::Idle;
       pidViewFromMenu_=true;view_=View::PidView;
     }else if(pidMenuSelection_==2){
       debugPage_=0;view_=View::Debug;
     }else{
       if(heatRequest_ || !thermal_.fresh(now)){
         chirp(420,170);
         Serial.println(F("AUTOTUNE_MENU_DENIED: HEAT/SENSOR"));
       }else{
         tuneSet_=setpoint_;
         tuneAir_=air_<Cfg::AIR_MIN_HEAT?Cfg::AIR_MIN_HEAT:air_;
         view_=View::TuneSetup;
       }
     }
   }
   return;
  }
  if(view_==View::PidView){
   // Girar encoder recorre KP, KI, KD. TEMP toque sale (no pasa pagina).
   if(e.tempShort || e.tempLong || e.airShort){
     view_=pidViewFromMenu_?View::PidMenu:View::Normal;
     return;
   }
   if(e.tempStep || e.airStep)pidPage_=(pidPage_+1)%3;
   return;
  }
 if(view_==View::Tuning){
   if(e.tempShort || e.airShort || e.airLong){
     // TEMP = detener la prueba; AIR = volver/cancelar.
     tune_.abort();cooldownStart();view_=View::Normal;
     Serial.println(F("AUTOTUNE CANCELADO"));
   }
   return;
 }
 if(view_==View::TuneSetup){
   if(e.tempStep)tuneSet_=clampInt(tuneSet_+e.tempStep*Cfg::TEMP_STEP,Cfg::TEMP_MIN,Cfg::TEMP_MAX);
   if(e.airStep)tuneAir_=clampInt(tuneAir_+e.airStep*Cfg::AIR_STEP,Cfg::AIR_MIN_HEAT,100);
   if(e.tempShort)beginTune(now);
   if(e.airShort || e.tempLong)view_=View::PidMenu;
   return;
 }
 if(e.tempStep){
   setpoint_=clampInt(setpoint_+e.tempStep*Cfg::TEMP_STEP,Cfg::TEMP_MIN,Cfg::TEMP_MAX);
   if(heatRequest_ && !Cfg::setpointWithinRawLimit(setpoint_)){
     cooldownStart();
     Serial.println(F("COOLDOWN: CONSIGNA FUERA DE CALIBRACION"));
   }
   pid_.reset();safety_.reset(now);
 }
 if(e.airStep){
   const int v=clampInt(air_+e.airStep*Cfg::AIR_STEP,0,100);
   if(v!=air_){
     air_=v;if(air_)lastAir_=air_;
     if(heatRequest_){
       if(air_<Cfg::AIR_MIN_HEAT)cooldownStart();
       else startFlow(now);
     }
   }
 }
 if(e.tempShort){
   if(heatRequest_)cooldownStart();
   else beginHeat(now);
 }
 if(e.airShort){
   if(!heatRequest_ && !(thermal_.fresh(now) && thermal_.sample().outletEst>=Cfg::OUTLET_COOL_START_C)){
     manualPump_=!manualPump_;
     if(manualPump_ && air_==0)air_=lastAir_;
   }
 }
 // Se conserva el atajo anterior: TEMP largo muestra directamente el PID.
 if(e.tempLong && !heatRequest_){
   view_=View::PidView;pidPage_=0;
   lastTuneResult_=TuneStatus::Idle;pidViewFromMenu_=false;
 }
 // AIR largo abre SIEMPRE el menu de dos opciones tras BOOT.
 // Desde el menu se puede VER PID incluso durante calefaccion,
 // pero el autotune sigue sujeto a los interlocks originales.
 if(e.airLong){view_=View::PidMenu;pidMenuSelection_=0;}
}
bool App::allowHeat(uint32_t now)const{
  return Cfg::setpointWithinRawLimit(setpoint_) &&
         Cfg::MAINS_OUTPUTS_ENABLED && Hardware::ready() && boot_.done() && fault_==Fault::None && display_.ready() && ramOK_ &&
        heatRequest_ && arm_==ArmStage::Ready &&
        (!stand_ || standPermittedForTune()) && !bothWasDown_ && air_>=Cfg::AIR_MIN_HEAT &&
        thermal_.fresh(now) && Hardware::zcHealthy() &&
        thermal_.sample().raw<Cfg::RAW_HARD_LIMIT;
}
uint8_t App::pumpWanted(uint32_t now)const{
 const SensorSample& t=thermal_.sample();
 if(!boot_.done() || !thermal_.fresh(now))return 100;
 if(fault_!=Fault::None)return (t.outletEst>Cfg::OUTLET_COOL_STOP_C)?100:0;
 if(cooldown_)return 100;
 if(heatRequest_){
   if(arm_==ArmStage::FullFlow)return 100;
   return (uint8_t)air_;
 }
 if(t.outletEst>=Cfg::OUTLET_COOL_START_C)return 100;
 if(stand_)return 0;
 return manualPump_?(uint8_t)air_:0;
}
void App::controlTick(uint32_t now){
 const SensorSample& t=thermal_.sample();
 // Arranque automatico con histéresis cuando no hay calor ni bomba manual.
 // Una vez iniciado, mantiene 100% hasta alcanzar el umbral de salida estimada.
 if(!cooldown_ && !heatRequest_ && !manualPump_ && fault_==Fault::None &&
    thermal_.fresh(now) && t.outletEst>=Cfg::OUTLET_COOL_START_C){
   cooldown_=true;
 }
 if(cooldown_ && thermal_.fresh(now) && t.outletEst<=Cfg::OUTLET_COOL_STOP_C){
   cooldown_=false;manualPump_=false;chirp(2200,65);
 }
 Hardware::pump(pumpWanted(now));
 if(fault_!=Fault::None || !boot_.done()){
   Hardware::heatOff();duty_=0;return;
 }
 const SafetyInput si={heatRequest_,allowHeat(now),view_==View::Tuning,
   thermal_.fresh(now),Hardware::zcHealthy(),Hardware::stalled(),
   thermal_.bigRise(),thermal_.bigDrop(),t.raw,t.outletEst,(float)setpoint_,duty_};
 const Fault unsafe=safety_.check(si,now);
 if(unsafe!=Fault::None){trip(unsafe,now);return;}
 if(!allowHeat(now)){
   Hardware::heatOff();duty_=0;return;
 }
 if(view_==View::Tuning){
   if(lastTune_ && (uint32_t)(now-lastTune_)<Cfg::PID_MS)return;
   lastTune_=now;
   duty_=tune_.tick(t.outletEst,now);
   if(tune_.status()!=TuneStatus::Running){finishTune(now);return;}
 }else{
   const uint8_t out=pid_.update(t.outletEst,t.outletControlled,t.outletRate,(float)setpoint_,(uint8_t)air_,now);
   if(out!=255)duty_=out;
 }
 Hardware::heatDuty(duty_);
}
void App::diagnosticsTick(uint32_t now){
 // Contadores ZC/gate: representan flancos y ordenes, no flujo fisico.
 if(!diagnosticsAt_){
   diagnosticsAt_=now;
   diagnosticsBaseline_=Hardware::counters();
   return;
 }
 const uint32_t dt=(uint32_t)(now-diagnosticsAt_);
 if(dt<1000UL)return;
 const Hardware::Counters c=Hardware::counters();
 diagAcceptedPerSec_=(uint32_t)(((uint64_t)(c.edges-diagnosticsBaseline_.edges)*1000UL+dt/2UL)/dt);
 diagRejectedPerSec_=(uint32_t)(((uint64_t)(c.rejected-diagnosticsBaseline_.rejected)*1000UL+dt/2UL)/dt);
 diagGateOnPerSec_=(uint32_t)(((uint64_t)(c.gateOn-diagnosticsBaseline_.gateOn)*1000UL+dt/2UL)/dt);
 diagGateOffPerSec_=(uint32_t)(((uint64_t)(c.gateOff-diagnosticsBaseline_.gateOff)*1000UL+dt/2UL)/dt);
 diagnosticsBaseline_=c;
 diagnosticsAt_=now;
}
void App::displayTick(uint32_t now){
 if((uint32_t)(now-lastLCD_)<Cfg::LCD_MS)return;
 lastLCD_=now;
 Screen s;
 s.boot=boot_.stage();s.warning=boot_.activeWarning(now);
 s.view=view_;s.fault=fault_;s.sensor=&thermal_.sample();
 s.setpoint=setpoint_;s.air=air_;s.tuneSet=tuneSet_;s.tuneAir=tuneAir_;
 s.duty=duty_;s.cycles=tune_.cycles();s.pidPage=pidPage_;
 s.pidMenuSelection=pidMenuSelection_;
 s.debugPage=debugPage_;
 s.zcAcceptedPerSec=diagAcceptedPerSec_;
 s.zcRejectedPerSec=diagRejectedPerSec_;
 s.zcGateOnPerSec=diagGateOnPerSec_;
 s.zcGateOffPerSec=diagGateOffPerSec_;
 s.zcPeriodUs=Hardware::zcPeriod();
 s.zcOK=Hardware::zcHealthy();
 s.zcInput=digitalRead(Cfg::ZC)==HIGH;
 s.tcFresh=thermal_.fresh(now);
 s.tcAgeMs=s.sensor->lastGoodAt?(uint32_t)(now-s.sensor->lastGoodAt):0;
 s.pumpCmd=Hardware::pumpCommand();
 s.relayOn=digitalRead(Cfg::RELAY)==HIGH;
 s.standOn=stand_;
 s.arm=(uint8_t)arm_;
 s.freeHeapBytes=ESP.getFreeHeap();
 s.minHeapBytes=ESP.getMinFreeHeap();
 s.resetReason=Hardware::resetReason();
 const Hardware::Counters dbgCounters=Hardware::counters();
 s.zcTotal=dbgCounters.edges;
 s.rejectedTotal=dbgCounters.rejected;
 s.heatRequest=heatRequest_;s.cooldown=cooldown_;
 s.preflow=heatRequest_ && arm_!=ArmStage::Ready;
 const uint32_t elapsed=now-armSince_;
 s.preflowSeconds=s.preflow && elapsed<Cfg::PREFLOW_MS?
       (uint16_t)((Cfg::PREFLOW_MS-elapsed+999)/1000):0;
 s.loadedPid=pidFromEEPROM_;s.gains=gains_;s.tuneStatus=lastTuneResult_;
 s.tuneHeating=tune_.heating();
 display_.render(s);
}
void App::serialTick(uint32_t now){
 if((uint32_t)(now-lastSerial_)<Cfg::SERIAL_MS)return;
 lastSerial_=now;
 // Diagnostico H11AA1: frecuencia real y pulsos descartados en el ultimo intervalo.
 static uint32_t zcPreviousMs=0, zcPreviousEdges=0, zcPreviousRejected=0;
 const Hardware::Counters zc=Hardware::counters();
 const uint32_t zcIntervalMs=zcPreviousMs ? (uint32_t)(now-zcPreviousMs) : Cfg::SERIAL_MS;
 const float zcPulsesPerSec=zcIntervalMs ?
      (float)(zc.edges-zcPreviousEdges)*1000.0f/(float)zcIntervalMs : 0.0f;
 Serial.printf("ZC GPIO4=%d PULSOS/s=%.1f RECHAZADOS=%lu OK=%d\n",
     digitalRead(Cfg::ZC),zcPulsesPerSec,
     (unsigned long)(zc.rejected-zcPreviousRejected),
     Hardware::zcHealthy()?1:0);
 zcPreviousMs=now;zcPreviousEdges=zc.edges;zcPreviousRejected=zc.rejected;
 const SensorSample& s=thermal_.sample();
 Serial.print(now);Serial.print(',');
 if(s.valid)Serial.print(s.outletEst,1);
 else Serial.print(F("NaN"));
 Serial.print(',');Serial.print(setpoint_);Serial.print(',');
 Serial.print(air_);Serial.print(',');Serial.print(Hardware::pumpCommand());Serial.print(',');
 Serial.print(duty_);Serial.print(',');Serial.print(heatRequest_?1:0);Serial.print(',');
 Serial.print((unsigned)arm_);Serial.print(',');Serial.print((unsigned)fault_);Serial.print(',');
 Serial.print(Hardware::zcHealthy()?1:0);Serial.print(',');
 Serial.print((unsigned)boot_.stage());Serial.print(',');
 Serial.print(freeRam());Serial.print(',');Serial.println((unsigned)tune_.cycles());
}
void App::serialCommand(uint32_t now){
 if(!Serial.available())return;
 const char ch=(char)Serial.read();
 if(ch=='?')Serial.println(F("P=PID, T=TEMP_SALIDA_EST, ?=help; sin acciones de potencia por Serial"));
 else if(ch=='P'){
   Serial.print(F("KP="));Serial.print(gains_.kp,4);
   Serial.print(F(" KI="));Serial.print(gains_.ki,4);
   Serial.print(F(" KD="));Serial.print(gains_.kd,4);
   Serial.print(F(" EEPROM="));Serial.println(pidFromEEPROM_?1:0);
 }else if(ch=='T'){
   Serial.print(F("OUT_EST="));Serial.print(thermal_.sample().outletEst,2);
   Serial.print(F(" VALID="));Serial.print(thermal_.sample().valid?1:0);
   Serial.print(F(" COUNT="));Serial.println(thermal_.sample().consecutive);
 }
 (void)now;
}
void App::tick(){
 const uint32_t now=millis();
 InputFrame e=controls_.poll(now);
 // PRIORIDAD 1: cortes inmediatos, antes de LCD, SERIAL, PID y EEPROM.
  if(e.bothDown || fault_!=Fault::None ||
      (e.standPressed && !standPermittedForTune()))Hardware::heatOff();
 thermal_.tick(now);
 diagnosticsTick(now);
 // Cada etapa de boot se recorre siempre, incluso con warning o fault previo.
 boot_.tick(now,thermal_.sample(),Hardware::zcHealthy(),Hardware::counters());
 handleInput(e,now);
 if(heatRequest_)flowTick(now);
 controlTick(now);
 // WDT heartbeat solo despues de revisar interlocks y salidas.
 Hardware::heartbeat();
 serialCommand(now);
 displayTick(now);
 serialTick(now);
 Hardware::watchdogKick(); // Unico punto de alimentacion del watchdog.
}
