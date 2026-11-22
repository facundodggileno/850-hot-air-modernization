#include "Hardware.h"
#include <Arduino.h>
#include <driver/gptimer.h>
#include <driver/gpio.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_err.h>

// ESP32-S3, Arduino-ESP32 3.x / ESP-IDF 5.x.
// ALIMENTACION: GPIO 3.3V; drivers relé/speaker pueden tener carga a 5V.
// CONTROL: bomba via fase MOC3023; heater burst ciclos completos MOC3083.
// NO existe sensor de caudal. AIR% es solo una consigna.
// SALIDAS DE POTENCIA BLOQUEADAS POR DEFECTO EN Config.h.
namespace {
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
gptimer_handle_t timerPump=nullptr;
volatile bool timerReady=false, timerFault=false, heartbeatTripped=false;
volatile uint8_t pumpPct=100, heaterPct=0;
volatile bool heatEnabled=false, heatPairSecond=false, heatPairOn=false;
volatile uint32_t lastZcUs=0, hbUs=0;
volatile uint32_t edges=0, rejected=0, gateOn=0, gateOff=0;
volatile uint16_t halfCycleUs=10000, heatAccumulator=0;
enum class Phase : uint8_t {Idle,Wait,Pulse};
volatile Phase phase=Phase::Idle;
uint8_t resetWasWdt=0;
bool watchdogRegistered=false;
inline void IRAM_ATTR pumpGate(bool level){
  gpio_set_level(static_cast<gpio_num_t>(Cfg::PUMP),level?1:0);
}
inline void IRAM_ATTR heaterGate(bool level){
  gpio_set_level(static_cast<gpio_num_t>(Cfg::HEATER),level?1:0);
}
inline void IRAM_ATTR relayGate(bool level){
  gpio_set_level(static_cast<gpio_num_t>(Cfg::RELAY),level?1:0);
}
uint16_t IRAM_ATTR pumpDelay(uint8_t p){
  if(p<20){
    const uint16_t n=Cfg::PUMP_DELAY_20_US+(20-p)*90U;
    return n>Cfg::PUMP_MAX_DELAY_US?Cfg::PUMP_MAX_DELAY_US:n;
  }
  if(p>=95)return Cfg::PUMP_DELAY_95_US;
  return static_cast<uint16_t>(
    Cfg::PUMP_DELAY_20_US -
    ((uint32_t)(Cfg::PUMP_DELAY_20_US-Cfg::PUMP_DELAY_95_US)*(p-20))/75UL);
}
bool IRAM_ATTR onPumpAlarm(gptimer_handle_t timer,
                          const gptimer_alarm_event_data_t* event,void*){
  portENTER_CRITICAL_ISR(&mux);
  if(!Cfg::MAINS_OUTPUTS_ENABLED || !timerReady || timerFault){
    pumpGate(false);phase=Phase::Idle;
  }else if(phase==Phase::Wait && pumpPct>0 && pumpPct<100){
    pumpGate(true);++gateOn;phase=Phase::Pulse;
    gptimer_alarm_config_t alarm={};
    alarm.alarm_count=event->count_value+Cfg::PUMP_GATE_US;
    if(gptimer_set_alarm_action(timer,&alarm)!=ESP_OK){
      pumpGate(false);phase=Phase::Idle;timerFault=true;
      heaterGate(false);relayGate(false);
    }
  }else if(phase==Phase::Pulse){
    pumpGate(false);++gateOff;phase=Phase::Idle;
  }else{
    if(pumpPct<100)pumpGate(false);
    phase=Phase::Idle;
  }
  portEXIT_CRITICAL_ISR(&mux);
  return false;
}
void IRAM_ATTR onZeroCross(){
  const uint32_t now=micros();
  portENTER_CRITICAL_ISR(&mux);
  if(lastZcUs){
    const uint32_t delta=now-lastZcUs;
    if(delta<Cfg::ZC_REJECT_US){
      ++rejected;
      portEXIT_CRITICAL_ISR(&mux);
      return;
    }
    if(delta>=7500UL && delta<=11500UL)
      halfCycleUs=static_cast<uint16_t>((7UL*halfCycleUs+delta)/8UL);
  }
  lastZcUs=now;++edges;
  if(heatEnabled && hbUs && (uint32_t)(now-hbUs)>Cfg::HEARTBEAT_STALE_US){
    heatEnabled=false;heaterPct=0;heatPairOn=false;
    heartbeatTripped=true;heaterGate(false);relayGate(false);
  }
  if(!Cfg::MAINS_OUTPUTS_ENABLED || !heatEnabled || heaterPct==0){
    heatPairSecond=false;heatPairOn=false;heatAccumulator=0;
    heaterGate(false);
  }else if(!heatPairSecond){
    heatAccumulator+=heaterPct;
    heatPairOn=(heatAccumulator>=100);
    if(heatPairOn)heatAccumulator-=100;
    heaterGate(heatPairOn);
    heatPairSecond=true;
  }else{
    heaterGate(heatPairOn);
    heatPairSecond=false;
  }
  if(Cfg::MAINS_OUTPUTS_ENABLED && timerReady && !timerFault){
    if(pumpPct>=100){
      phase=Phase::Idle;
      pumpGate(true);
    }else{
      pumpGate(false);phase=Phase::Idle;
      if(pumpPct>0){
        uint16_t us=pumpDelay(pumpPct);
        const uint16_t maxUs=halfCycleUs>500?halfCycleUs-400:
                              Cfg::PUMP_MAX_DELAY_US;
        if(us>maxUs)us=maxUs;
        if(us<Cfg::PUMP_MIN_DELAY_US)us=Cfg::PUMP_MIN_DELAY_US;
        uint64_t timerNow=0;
        if(gptimer_get_raw_count(timerPump,&timerNow)==ESP_OK){
          gptimer_alarm_config_t alarm={};
          alarm.alarm_count=timerNow+us;
          if(gptimer_set_alarm_action(timerPump,&alarm)==ESP_OK)
            phase=Phase::Wait;
          else timerFault=true;
        }else timerFault=true;
        if(timerFault){
          heaterGate(false);relayGate(false);pumpGate(false);
        }
      }
    }
  }else{
    phase=Phase::Idle;heaterGate(false);relayGate(false);pumpGate(false);
  }
  portEXIT_CRITICAL_ISR(&mux);
}
} // namespace

namespace Hardware {
void begin(){
  const esp_reset_reason_t reason=esp_reset_reason();
  resetWasWdt=(reason==ESP_RST_TASK_WDT || reason==ESP_RST_INT_WDT ||
               reason==ESP_RST_WDT)?1:0;
  digitalWrite(Cfg::HEATER,LOW);digitalWrite(Cfg::RELAY,LOW);
  digitalWrite(Cfg::PUMP,LOW);digitalWrite(Cfg::BUZZER,LOW);
  pinMode(Cfg::HEATER,OUTPUT);pinMode(Cfg::RELAY,OUTPUT);
  pinMode(Cfg::PUMP,OUTPUT);pinMode(Cfg::BUZZER,OUTPUT);
  pinMode(Cfg::ZC,INPUT);pinMode(Cfg::STAND,INPUT);
  pumpGate(false);heaterGate(false);relayGate(false);
  pumpPct=100;heaterPct=0;heatEnabled=false;heartbeatTripped=false;
  gptimer_config_t conf={};
  conf.clk_src=GPTIMER_CLK_SRC_DEFAULT;
  conf.direction=GPTIMER_COUNT_UP;
  conf.resolution_hz=1000000;
  esp_err_t status=gptimer_new_timer(&conf,&timerPump);
  if(status==ESP_OK){
    gptimer_event_callbacks_t cbs={};
    cbs.on_alarm=onPumpAlarm;
    status=gptimer_register_event_callbacks(timerPump,&cbs,nullptr);
  }
  if(status==ESP_OK)status=gptimer_enable(timerPump);
  if(status==ESP_OK)status=gptimer_start(timerPump);
  timerReady=(status==ESP_OK);
  timerFault=!timerReady;
  if(timerReady)attachInterrupt(digitalPinToInterrupt(Cfg::ZC),onZeroCross,RISING);
  // Bomba al 100% durante el arranque solo si la build autoriza potencia.
  if(timerReady && Cfg::MAINS_OUTPUTS_ENABLED)pumpGate(true);
  heartbeat();
}
bool ready(){return timerReady && !timerFault;}
void heartbeat(){
  portENTER_CRITICAL(&mux);hbUs=micros();portEXIT_CRITICAL(&mux);
}
void heatOff(){
  portENTER_CRITICAL(&mux);
  heatEnabled=false;heaterPct=0;heatPairOn=false;
  heatPairSecond=false;heatAccumulator=0;
  heaterGate(false);relayGate(false);
  portEXIT_CRITICAL(&mux);
}
void heatDuty(uint8_t pct){
  if(!Cfg::MAINS_OUTPUTS_ENABLED || !ready()){heatOff();return;}
  if(pct>100)pct=100;
  portENTER_CRITICAL(&mux);
  // Relé habilitado durante control activo, aun si duty=0.
  relayGate(true);
  heaterPct=pct;heatEnabled=(pct>0);
  if(pct==0)heaterGate(false);
  portEXIT_CRITICAL(&mux);
}
void pump(uint8_t percent){
  if(percent>100)percent=100;
  portENTER_CRITICAL(&mux);
  pumpPct=percent;
  if(!Cfg::MAINS_OUTPUTS_ENABLED || !ready()){
    pumpGate(false);phase=Phase::Idle;
  }else if(percent==100){
    pumpGate(true);phase=Phase::Idle;
  }else if(percent==0){
    pumpGate(false);phase=Phase::Idle;
  }
  portEXIT_CRITICAL(&mux);
}
uint8_t pumpCommand(){return pumpPct;}
uint8_t dutyCommand(){return heaterPct;}
uint8_t resetReason(){return resetWasWdt;}
bool stalled(){return heartbeatTripped||timerFault;}
bool zcHealthy(){
  uint32_t stamp;
  portENTER_CRITICAL(&mux);stamp=lastZcUs;portEXIT_CRITICAL(&mux);
  return stamp && (uint32_t)(micros()-stamp)<=Cfg::ZC_STALE_US;
}
Counters counters(){
  Counters c;
  portENTER_CRITICAL(&mux);
  c.edges=edges;c.rejected=rejected;c.gateOn=gateOn;c.gateOff=gateOff;
  portEXIT_CRITICAL(&mux);
  return c;
}
void watchdogStart(){
  if(watchdogRegistered)return;
  esp_task_wdt_config_t cfg={};
  cfg.timeout_ms=2000;cfg.idle_core_mask=0;cfg.trigger_panic=true;
  esp_err_t status=esp_task_wdt_init(&cfg);
  if(status==ESP_ERR_INVALID_STATE)
    status=esp_task_wdt_reconfigure(&cfg);
  if(status==ESP_OK || status==ESP_ERR_INVALID_STATE){
    if(esp_task_wdt_add(nullptr)==ESP_OK ||
       esp_task_wdt_status(nullptr)==ESP_OK)
      watchdogRegistered=true;
  }
}
void watchdogKick(){if(watchdogRegistered)esp_task_wdt_reset();}
} // namespace Hardware
