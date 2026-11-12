/*
  HONY 850 PRO - PUMP ISOLATED DIAGNOSTIC V1
  Arduino Nano ATmega328P (Optiboot)

  Purpose: distinguish software gate commands from real blower movement.
  Heater control D4 and safety relay D5 remain LOW at all times.
  IMPORTANT: PHYSICALLY DISCONNECT THE HEATER AC SUPPLY before running.
  LOW signals are not protection from a shorted triac or incorrect wiring.

  Inputs: zero-cross D2 (RISING), Serial 115200, 50 Hz supply.
  Outputs: pump MOC3023 gate D3; heater D4 OFF; relay D5 OFF.
  Automatically cycles AIR100, AIR70, AIR40, AIR100, AIR40, AIR100,
  6 seconds each. Prints ZC and Timer1 gate counts per 500 ms.

  IMPORTANT: 'cmdAIR' is a request, NOT measured real airflow.
  'D3pin' is a 5V logic reading, NOT triac current or AC motor voltage.
*/

#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

const uint8_t PIN_ZC = 2;
const uint8_t PIN_PUMP = 3;
const uint8_t PIN_HEATER = 4;
const uint8_t PIN_RELAY = 5;

const uint16_t HALF_CYCLE_US = 10000;
const uint16_t GATE_PULSE_US = 180;
const uint32_t ZC_REJECT_US = 7000UL;
const uint32_t ZC_STALE_US = 150000UL;
const uint32_t STAGE_MS = 6000UL;

const uint8_t airSequence[] = {100,70,40,100,40,100};
const uint8_t sequenceCount = sizeof(airSequence)/sizeof(airSequence[0]);

volatile uint8_t currentAir = 100;
volatile uint32_t zeroCrossCount = 0;
volatile uint32_t zcRejectedCount = 0;
volatile uint32_t gateStartCount = 0;
volatile uint32_t gateEndCount = 0;
volatile uint32_t lastZcUs = 0;
volatile uint16_t measuredHalfCycleUs = HALF_CYCLE_US;

uint8_t seqIndex = 0;
uint32_t stageStartMs = 0;
uint32_t lastLogMs = 0;
uint32_t lastLoggedZC = 0;
uint32_t lastLoggedGates = 0;

uint16_t airDelayUs(uint8_t air) {
  if (air >= 95) return 3000;
  if (air <= 20) return 7600;
  return (uint16_t)(7600L - ((int32_t)(air - 20) * 4600L) / 75L);
}

void forceHeatOff() {
  // No writes HIGH to either heater output anywhere in this diagnostic.
  digitalWrite(PIN_HEATER, LOW);
  digitalWrite(PIN_RELAY, LOW);
}

ISR(TIMER1_COMPA_vect) {
  if (currentAir > 0 && currentAir < 100) {
    digitalWrite(PIN_PUMP, HIGH);
    OCR1B = (uint16_t)(TCNT1 + GATE_PULSE_US * 2U);
    TIFR1 = _BV(OCF1B);
    TIMSK1 |= _BV(OCIE1B);
    gateStartCount++;
  }
  TIMSK1 &= (uint8_t)~_BV(OCIE1A);
}

ISR(TIMER1_COMPB_vect) {
  if (currentAir < 100) digitalWrite(PIN_PUMP, LOW);
  gateEndCount++;
  TIMSK1 &= (uint8_t)~_BV(OCIE1B);
}

void zeroCross() {
  uint32_t nowUs = micros();
  if (lastZcUs) {
    uint32_t delta = nowUs - lastZcUs;
    if (delta < ZC_REJECT_US) { zcRejectedCount++; return; }
    if (delta >= 7500UL && delta <= 11500UL)
      measuredHalfCycleUs = (uint16_t)((measuredHalfCycleUs * 7UL + delta) / 8UL);
  }
  lastZcUs = nowUs;
  zeroCrossCount++;

  // Clear pending gate events atomically inside ISR.
  TIMSK1 &= (uint8_t)~(_BV(OCIE1A) | _BV(OCIE1B));
  TIFR1 = _BV(OCF1A) | _BV(OCF1B);
  if (currentAir == 100) {
    digitalWrite(PIN_PUMP, HIGH);
  } else {
    digitalWrite(PIN_PUMP, LOW);
    if (currentAir > 0) {
      uint16_t d = airDelayUs(currentAir);
      const uint16_t maxD = measuredHalfCycleUs > 500U ? measuredHalfCycleUs - 400U : 9000U;
      if (d > maxD) d = maxD;
      TCNT1 = 0;
      OCR1A = d * 2U;
      TIFR1 = _BV(OCF1A);
      TIMSK1 |= _BV(OCIE1A);
    }
  }
}

void selectAir(uint8_t air) {
  noInterrupts();
  currentAir = air;
  TIMSK1 &= (uint8_t)~(_BV(OCIE1A) | _BV(OCIE1B));
  TIFR1 = _BV(OCF1A) | _BV(OCF1B);
  interrupts();
  // In full mode, keep MOC LED continuously active independently of ZC.
  digitalWrite(PIN_PUMP, air == 100 ? HIGH : LOW);
  forceHeatOff();
}

void setup() {
  // Force low before switching outputs to avoid transient heater ON.
  digitalWrite(PIN_HEATER, LOW);
  digitalWrite(PIN_RELAY, LOW);
  pinMode(PIN_HEATER, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, HIGH);
  pinMode(PIN_ZC, INPUT);

  Serial.begin(115200);
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = _BV(CS11);   // prescaler 8, half-microsecond ticks @16MHz
  TCNT1 = 0;
  TIMSK1 &= (uint8_t)~(_BV(OCIE1A) | _BV(OCIE1B));
  interrupts();
  attachInterrupt(digitalPinToInterrupt(PIN_ZC), zeroCross, RISING);
  seqIndex = 0;
  stageStartMs = millis();
  lastLogMs = millis();
  selectAir(airSequence[seqIndex]);
  wdt_enable(WDTO_500MS);
  Serial.println(F("PUMP DIAG V1 - HEATER OUTPUTS FORCED OFF"));
  Serial.println(F("DISCONNECT HEATER MAINS PHYSICALLY; NO FLOW SENSOR"));
  Serial.println(F("ms,stage,cmdAIR,D3pin,zcDelta500ms,gateDelta500ms,gateStarts,gateEnds,zcReject,zcHealthy"));
}

void loop() {
  forceHeatOff();
  const uint32_t now = millis();
  if ((uint32_t)(now - stageStartMs) >= STAGE_MS) {
    seqIndex = (seqIndex+1) % sequenceCount;
    stageStartMs = now;
    selectAir(airSequence[seqIndex]);
    Serial.print(F("STAGE,END=>")); Serial.println(airSequence[seqIndex]);
  }

  // If ZC stops while in phase mode, drive pump gate HIGH as a fallback.
  // This is only a logic drive; it cannot restore missing mains power.
  uint32_t lastZc;
  noInterrupts(); lastZc = lastZcUs; interrupts();
  if (lastZc == 0 || (uint32_t)(micros() - lastZc) > ZC_STALE_US) {
    digitalWrite(PIN_PUMP, HIGH);
  }

  if ((uint32_t)(now - lastLogMs) >= 500UL) {
    uint32_t z, st, en, rej;
    noInterrupts();
    z=zeroCrossCount; st=gateStartCount; en=gateEndCount; rej=zcRejectedCount;
    lastZc=lastZcUs;
    interrupts();
    Serial.print(now); Serial.print(',');
    Serial.print(seqIndex); Serial.print(',');
    Serial.print((int)airSequence[seqIndex]); Serial.print(',');
    Serial.print(digitalRead(PIN_PUMP)); Serial.print(',');
    Serial.print(z-lastLoggedZC); Serial.print(',');
    Serial.print(st-lastLoggedGates); Serial.print(',');
    Serial.print(st); Serial.print(','); Serial.print(en); Serial.print(',');
    Serial.print(rej); Serial.print(',');
    Serial.println(lastZc && (uint32_t)(micros()-lastZc) <= ZC_STALE_US ? 1 : 0);
    lastLoggedZC=z;
    lastLoggedGates=st;
    lastLogMs=now;
  }
  wdt_reset();
}