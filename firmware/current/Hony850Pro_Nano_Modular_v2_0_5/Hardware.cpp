#include "Hardware.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>

namespace {
volatile uint8_t pumpPct=100, heaterPct=0;
volatile bool heatEnabled=false, heartbeatTripped=false;
volatile uint32_t hbUs=0, lastZcUs=0, edgeCount=0, rejectCount=0, gateStarts=0, gateEnds=0;
volatile uint16_t halfCycleUs=10000, heatAccumulator=0;
volatile bool heatPairSecond=false, heatPairOn=false;

uint16_t pumpDelay(uint8_t p) {
  if(p<20) return (uint16_t)min((uint16_t)Cfg::PUMP_MAX_DELAY_US,
                               (uint16_t)(Cfg::PUMP_DELAY_20_US+(20-p)*90U));
  if(p>=95) return Cfg::PUMP_DELAY_95_US;
  return (uint16_t)(Cfg::PUMP_DELAY_20_US-((uint32_t)(Cfg::PUMP_DELAY_20_US-Cfg::PUMP_DELAY_95_US)*(p-20))/75UL);
}
void zcISR() {
  const uint32_t now=micros();
  if(heatEnabled && hbUs && (uint32_t)(now-hbUs)>Cfg::HEARTBEAT_STALE_US){
    heatEnabled=false; heaterPct=0; heatPairOn=false; heartbeatTripped=true;
    digitalWrite(Cfg::HEATER,LOW); digitalWrite(Cfg::RELAY,LOW);
  }
  if(lastZcUs){
    const uint32_t delta=now-lastZcUs;
    if(delta<Cfg::ZC_REJECT_US){rejectCount++; return;}
    if(delta>=7500UL && delta<=11500UL)
      halfCycleUs=(uint16_t)((7UL*halfCycleUs+delta)/8UL);
  }
  lastZcUs=now; edgeCount++;
  if(!heatEnabled || heaterPct==0){
    heatPairSecond=false; heatPairOn=false; heatAccumulator=0;
    digitalWrite(Cfg::HEATER,LOW);
  }else if(!heatPairSecond){
    heatAccumulator+=(uint16_t)heaterPct;
    heatPairOn=(heatAccumulator>=100);
    if(heatPairOn) heatAccumulator-=100;
    digitalWrite(Cfg::HEATER,heatPairOn?HIGH:LOW);
    heatPairSecond=true;
  }else{
    digitalWrite(Cfg::HEATER,heatPairOn?HIGH:LOW);
    heatPairSecond=false;
  }
  TIMSK1 &= (uint8_t)~(_BV(OCIE1A)|_BV(OCIE1B));
  TIFR1=_BV(OCF1A)|_BV(OCF1B); // W1C, NO '|='
  if(pumpPct>=100){digitalWrite(Cfg::PUMP,HIGH); return;}
  digitalWrite(Cfg::PUMP,LOW);
  if(pumpPct>0){
    uint16_t us=pumpDelay(pumpPct);
    const uint16_t maxUs=halfCycleUs>500?halfCycleUs-400:Cfg::PUMP_MAX_DELAY_US;
    if(us>maxUs)us=maxUs;
    if(us<Cfg::PUMP_MIN_DELAY_US)us=Cfg::PUMP_MIN_DELAY_US;
    TCNT1=0; OCR1A=us*2U; TIFR1=_BV(OCF1A);
    TIMSK1 |= _BV(OCIE1A);
  }
}
}

ISR(TIMER1_COMPA_vect){
  if(pumpPct>0 && pumpPct<100){
    digitalWrite(Cfg::PUMP,HIGH); gateStarts++;
    OCR1B=(uint16_t)(TCNT1+Cfg::PUMP_GATE_US*2U);
    TIFR1=_BV(OCF1B); TIMSK1 |= _BV(OCIE1B);
  }
  TIMSK1 &= (uint8_t)~_BV(OCIE1A);
}
ISR(TIMER1_COMPB_vect){
  digitalWrite(Cfg::PUMP,LOW); gateEnds++;
  TIMSK1 &= (uint8_t)~_BV(OCIE1B);
}

// Captura la causa del reset ANTES del boot del runtime AVR.
#ifndef HONY_HOST_TEST
uint8_t savedResetCause __attribute__((section(".noinit")));
void earlyWdtDisable(void) __attribute__((naked, used, section(".init3")));
void earlyWdtDisable(void){savedResetCause=MCUSR; MCUSR=0; wdt_disable();}
#else
uint8_t savedResetCause=0;
#endif

namespace Hardware {
void begin(){
  // Escribir niveles seguros ANTES de activar la direccion de salida.
  digitalWrite(Cfg::HEATER,LOW); digitalWrite(Cfg::RELAY,LOW);
  digitalWrite(Cfg::PUMP,HIGH);
  pinMode(Cfg::HEATER,OUTPUT); pinMode(Cfg::RELAY,OUTPUT);
  pinMode(Cfg::PUMP,OUTPUT); pinMode(Cfg::BUZZER,OUTPUT);
  digitalWrite(Cfg::BUZZER,LOW);
  pinMode(Cfg::ZC,INPUT); pinMode(Cfg::STAND,INPUT);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
    TCCR1A=0; TCCR1B=_BV(CS11); // Timer1 /8, 0.5 us por tick a 16 MHz.
    TCNT1=0; TIMSK1 &= (uint8_t)~(_BV(OCIE1A)|_BV(OCIE1B));
    heaterPct=0; heatEnabled=false; pumpPct=100;
  }
  attachInterrupt(digitalPinToInterrupt(Cfg::ZC),zcISR,RISING);
  heartbeat();
}
void heartbeat(){ATOMIC_BLOCK(ATOMIC_RESTORESTATE){hbUs=micros();}}
void heatOff(){
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
    heatEnabled=false; heaterPct=0; heatPairOn=false;
    heatPairSecond=false; heatAccumulator=0;
  }
  digitalWrite(Cfg::HEATER,LOW); digitalWrite(Cfg::RELAY,LOW);
}
void heatDuty(uint8_t pct){
  if(pct>100)pct=100;
  // El rele queda habilitado durante control activo, incluso con PID=0,
  // para evitar conmutarlo cientos de veces durante la regulacion.
  digitalWrite(Cfg::RELAY,HIGH);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){heaterPct=pct; heatEnabled=(pct>0);}
  if(pct==0)digitalWrite(Cfg::HEATER,LOW);
}
void pump(uint8_t p){
  if(p>100)p=100;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
    pumpPct=p;
    if(p==100){TIMSK1 &= (uint8_t)~(_BV(OCIE1A)|_BV(OCIE1B));}
  }
  if(p==100)digitalWrite(Cfg::PUMP,HIGH);
  else if(p==0)digitalWrite(Cfg::PUMP,LOW);
}
uint8_t pumpCommand(){return pumpPct;}
uint8_t dutyCommand(){return heaterPct;}
uint8_t resetReason(){return savedResetCause;}
bool stalled(){return heartbeatTripped;}
bool zcHealthy(){
  uint32_t stamp;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){stamp=lastZcUs;}
  return stamp!=0 && (uint32_t)(micros()-stamp)<=Cfg::ZC_STALE_US;
}
Counters counters(){
  Counters c;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
    c.edges=edgeCount;c.rejected=rejectCount;c.gateOn=gateStarts;c.gateOff=gateEnds;
  }
  return c;
}
void watchdogStart(){wdt_reset();wdt_enable(WDTO_500MS);}
void watchdogKick(){wdt_reset();}
}
