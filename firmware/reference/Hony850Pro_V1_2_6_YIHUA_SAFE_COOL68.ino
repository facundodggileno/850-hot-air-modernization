/*
  ================================================================
  HONY / GORDAK 850 PRO V1.2.6 YIHUA SAFE COOL68
  Firmware final base - Arduino Nano ATmega328P
  ================================================================

  HARDWARE VALIDADO:
  D2  <- H11AA1 zero-cross
  D3  -> MOC3023 bomba / BTA12
  D4  -> MOC3083 heater / BTA16
  D5  -> BC337 -> Omron G5Q-1A4 DC5V
  D6  -> BC337 -> speaker pasivo
  D7  <- microswitch stand NC (pull-up externo 10k a +5V)
  D8  <- TEMP encoder CLK
  D9  <- TEMP encoder DT
  A3  <- TEMP encoder SW
  A0  <- AIR encoder CLK
  A1  <- AIR encoder DT
  A2  <- AIR encoder SW
  D10 -> MAX6675 CS
  D12 <- MAX6675 SO
  D13 -> MAX6675 SCK
  A4/A5 -> LCD HW-61 I2C

  LCD:
  Libreria "hd44780" de Bill Perry.
  Clase hd44780_I2Cexp (autodeteccion del backpack).

  FUNCIONES:
  - TEMP 80..480 C, paso 5 C.
  - AIR 0..100 %, paso 5 %.
  - Heater bloqueado con AIR < 20 %.
  - Bomba obligatoria si T >= 80 C.
  - Stand: heater OFF inmediato; cooldown con bomba 100 % hasta <80 C.
  - Al cruzar <80 C en cooldown, la bomba se apaga automaticamente.
  - Al levantar del stand, si estaba calentando, reanuda TEMP/AIR previos.
  - Boton TEMP corto: Heater ON/OFF / OK en menus.
  - Boton TEMP largo: presets 1/2/3.
  - Boton AIR corto: bomba manual ON/OFF cuando es seguro / BACK en menus.
  - Boton AIR largo: menu de opciones.
  - PID Kp/Ki/Kd.
  - PID autotune ACTUAL o AUTO 3 PUNTOS (AIR 20/50/100).
  - AUTO 3 PUNTOS interpola Kp/Ki/Kd segun AIR.
  - Heater por burst de ciclos completos sincronizado a red.
  - Bomba por phase-angle con Timer1 + H11AA1.
  - EEPROM: PID, AIR calibration, presets, ultimo TEMP/AIR.
  - Thermal runaway.
  - Thermal control fault.
  - Overtemperature absoluta.
  - TC abierta / invalida.
  - FAULT enclavado hasta enfriar por debajo de 80 C y reconocer con TEMP.

  NOTA:
  La calibracion final del AIR y el PID se debe hacer fisicamente en la
  estacion. Los valores de fabrica incluidos son conservadores.
  ================================================================
*/

#include <Wire.h>
#include <EEPROM.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <max6675.h>
#include <avr/interrupt.h>

// ================================================================
// PINOUT
// ================================================================
const uint8_t PIN_ZC       = 2;
const uint8_t PIN_PUMP     = 3;
const uint8_t PIN_HEATER   = 4;
const uint8_t PIN_RELAY    = 5;
const uint8_t PIN_SPEAKER  = 6;
const uint8_t PIN_STAND    = 7;

const uint8_t PIN_TEMP_CLK = 8;
const uint8_t PIN_TEMP_DT  = 9;
const uint8_t PIN_TEMP_SW  = A3;

const uint8_t PIN_AIR_CLK  = A0;
const uint8_t PIN_AIR_DT   = A1;
const uint8_t PIN_AIR_SW   = A2;

const uint8_t PIN_MAX_CS   = 10;
const uint8_t PIN_MAX_SO   = 12;
const uint8_t PIN_MAX_SCK  = 13;

// ================================================================
// LIMITES / SEGURIDAD
// ================================================================
const int TEMP_MIN_C = 80;
const int TEMP_MAX_C = 380;  // TEMPORAL YIHUA: limite conservador
const int TEMP_STEP_C = 5;

const int AIR_MIN_PCT = 0;
const int AIR_MAX_PCT = 100;
const int AIR_STEP_PCT = 5;
const int AIR_MIN_HEATER_PCT = 40;  // TEMPORAL YIHUA: no calentar con poco caudal

const float COOLDOWN_TEMP_C = 80.0f;
const float COOLDOWN_STOP_C = 68.0f;

// Cooldown de FAULT:
// al dispararse el fault, enfria hasta 68 C y queda apagado.
// No se rearma por rebote termico dentro del mismo fault.
const float FAULT_COOL_ON_C  = 80.0f;  // referencia de entrada al cooldown
const float FAULT_COOL_OFF_C = 68.0f;
const float ABS_OVER_TEMP_C = 420.0f;  // TEMPORAL YIHUA
const float RUNAWAY_DELTA_C = 80.0f;

// Runaway relativo debe persistir varias muestras para evitar ruido.
const unsigned long RUNAWAY_CONFIRM_MS = 750UL;

// Thermal Control Fault: no-response
const unsigned long NO_RISE_WINDOW_MS = 20000UL;
const float NO_RISE_MIN_DELTA_C = 8.0f;
const float NO_RISE_MIN_DUTY = 60.0f;
const float NO_RISE_MIN_ERROR_C = 20.0f;

// Thermal Control Fault: perdida de regulacion luego de estabilizar
const float STABLE_BAND_C = 10.0f;
const unsigned long STABLE_REQUIRED_MS = 10000UL;
const float CONTROL_LOST_ERROR_C = 40.0f;
const unsigned long CONTROL_LOST_MS = 10000UL;

// ================================================================
// TIEMPOS UI / CONTROL
// ================================================================
const unsigned long BUTTON_LONG_MS = 1200UL;
const unsigned long BUTTON_DEBOUNCE_MS = 35UL;

const unsigned long TEMP_SAMPLE_MS = 250UL;
const unsigned long PID_SAMPLE_MS = 250UL;
const unsigned long LCD_REFRESH_MS = 150UL;
const unsigned long SERIAL_REFRESH_MS = 1000UL;

const unsigned long EEPROM_SAVE_DELAY_MS = 15000UL;

// ================================================================
// ZERO CROSS / PHASE ANGLE
// ================================================================
// Argentina: 50 Hz -> semiciclo nominal = 10 000 us.
// El firmware mide el periodo y actualiza una estimacion acotada.
const uint16_t HALF_CYCLE_DEFAULT_US = 10000;
const uint16_t PUMP_GATE_PULSE_US = 180;

// Limites fisicos de seguridad del disparo.
const uint16_t PUMP_DELAY_MIN_US = 200;
const uint16_t PUMP_DELAY_MAX_US = 9000;

// ================================================================
// OBJETOS
// ================================================================
hd44780_I2Cexp lcd;
MAX6675 thermocouple(PIN_MAX_SCK, PIN_MAX_CS, PIN_MAX_SO);

// ================================================================
// EEPROM
// ================================================================
struct PresetData {
  int16_t tempC;
  uint8_t airPct;
};

struct PidPointData {
  float kp;
  float ki;
  float kd;
  uint8_t valid;
};

struct ConfigData {
  uint16_t magic;
  uint8_t version;

  int16_t lastTempC;
  uint8_t lastAirPct;

  // PID simple (modo ACTUAL).
  float kp;
  float ki;
  float kd;

  // 0 = PID simple/ACTUAL, 1 = tabla 20/50/100 con interpolacion.
  uint8_t pidMode;
  PidPointData pid20;
  PidPointData pid50;
  PidPointData pid100;

  // Calibracion de phase-angle:
  // delay a 20 % y delay alto (95 %). 100 % usa conduccion completa.
  uint16_t airDelay20us;
  uint16_t airDelay100us;

  PresetData preset[3];

  uint8_t checksum;
};

const uint16_t CONFIG_MAGIC = 0x8501;
const uint8_t CONFIG_VERSION = 3;

ConfigData cfg;

bool configDirty = false;
unsigned long configDirtySince = 0;

// ================================================================
// ESTADOS
// ================================================================
enum FaultCode : uint8_t {
  FAULT_NONE = 0,
  FAULT_TC,
  FAULT_OVERTEMP,
  FAULT_RUNAWAY,
  FAULT_NO_RISE,
  FAULT_CONTROL_LOST
};

enum UiMode : uint8_t {
  UI_NORMAL = 0,
  UI_PRESETS,
  UI_SETTINGS,
  UI_PID_VALUES,
  UI_AUTOTUNE_MODE,
  UI_AUTOTUNE_TEMP,
  UI_AUTOTUNE_RUNNING,
  UI_AUTOTUNE_COOLDOWN,
  UI_SAVE_PRESET,
  UI_AIR_CAL20,
  UI_AIR_CAL100,
  UI_FAULT
};

FaultCode faultCode = FAULT_NONE;
UiMode uiMode = UI_NORMAL;

// ================================================================
// VARIABLES PRINCIPALES
// ================================================================
int setPointC = 120;
int airSetPct = 50;
int lastNonZeroAirPct = 50;

bool pumpManualEnabled = true;
bool heatRequested = false;

// Cooldown forzado: mientras T >= 80 C, bomba = 100%.
// Al bajar de 80 C, la bomba se apaga automaticamente.
bool cooldownActive = false;

// Una vez que un cooldown llega a <=68 C, queda terminado.
// No debe reactivarse por rebote termico mientras heater siga OFF.
bool cooldownCompletedLatch = false;

// Latch de cooldown durante FAULT para evitar serrucho 79/80 C.
bool faultCoolingActive = false;

// Si se apoya el mango mientras estaba calentando,
// heatRequested queda true y al levantar se reanuda automaticamente.
bool standPressed = true;
bool lastStandPressed = true;

float tempC = NAN;
float previousTempC = NAN;
bool tcValid = false;

uint8_t invalidTcCount = 0;

// Duty calculado por PID/autotune (0..100).
volatile uint8_t heaterDutyPct = 0;

// Permiso final de potencia.
// ISR de zero-cross usa esta bandera.
volatile bool heaterPowerAllowed = false;

// Aire efectivo que ejecuta la ISR.
// 0 = bomba sin disparo.
volatile uint8_t effectiveAirPctISR = 0;

// ================================================================
// ZERO CROSS / TIMER1
// ================================================================
volatile unsigned long zcCount = 0;
volatile unsigned long lastZcMicros = 0;
volatile uint16_t measuredHalfCycleUs = HALF_CYCLE_DEFAULT_US;

// Heater burst sigma-delta, siempre en ciclos completos.
volatile bool heaterFullCycleOn = false;
volatile bool heaterSecondHalf = false;
volatile uint16_t heaterBurstAccumulator = 0;

// Timer1 pump pulse.
volatile bool pumpGateScheduled = false;

// ================================================================
// PID
// ================================================================
float pidIntegral = 0.0f;
float lastPidMeasurement = NAN;
unsigned long lastPidMs = 0;
float pidOutput = 0.0f;

// Seguimiento estabilidad / fallos.
unsigned long stableBandSince = 0;
bool regulationWasStable = false;

unsigned long controlLostSince = 0;

unsigned long noRiseSince = 0;
float noRiseStartTemp = 0.0f;

unsigned long runawaySince = 0;
bool runawayArmed = false;

unsigned long controlGraceUntil = 0;

// ================================================================
// AUTOTUNE
// ================================================================
enum AutotuneMode : uint8_t {
  AT_CURRENT = 0,
  AT_THREE_POINT = 1
};

AutotuneMode autotuneMode = AT_CURRENT;
int autotuneMenuChoice = 0;
int autotuneSelectedTempC = 300;

bool autotuneHighPhase = true;
float autotuneTargetC = 250.0f;
int autotuneAirPct = 50;
const float AUTOTUNE_HYST_C = 2.0f;

// TEMPORAL YIHUA / bomba de diafragma:
// El heater de ~650 W tiene mucha mas potencia que el conjunto anterior.
// Limites conservadores hasta convertir a brushless y recalibrar.
const uint8_t AUTOTUNE_HIGH_DUTY_PCT = 25;
const uint8_t AUTOTUNE_LOW_DUTY_PCT  = 0;
const uint8_t PID_MAX_DUTY_PCT       = 40;
const float AUTOTUNE_ABORT_MARGIN_C  = 30.0f;

// AUTO 3 PUNTOS: 20%, 50%, 100%.
const uint8_t AUTOTUNE_AIR_POINTS[3] = {40, 70, 100};  // TEMPORAL YIHUA
uint8_t autotuneStage = 0;
bool autotuneNextStagePending = false;

PidPointData tuneResult[3];

// Esta version especial usa 10 ciclos para AUTOTUNE ACTUAL,
// pensado para obtener el PID GENERAL de referencia.
// AUTO 3 PUNTOS sigue usando 4 ciclos por punto.
//
// RAMFIX: no hace falta guardar las 10 muestras completas.
// Para el promedio robusto alcanza con suma, minimo, maximo y cantidad.
// Se conserva exactamente la regla de descartar SOLO un minimo y un maximo.
const uint8_t AUTOTUNE_ACTUAL_CYCLES = 10;
const uint8_t AUTOTUNE_3P_CYCLES = 4;

struct TuneStats {
  float sum;
  float minV;
  float maxV;
  uint8_t count;
};

TuneStats tuneHighStats;
TuneStats tuneLowStats;
TuneStats tunePeriodStats;

float tuneCurrentHighPeak = -1000.0f;
float tuneCurrentLowPeak = 1000.0f;

unsigned long tuneLastUpCrossMs = 0;
unsigned long tuneStartMs = 0;

const unsigned long AUTOTUNE_TIMEOUT_MS = 12UL * 60UL * 1000UL;

// ================================================================
// UI
// ================================================================
int8_t menuIndex = 0;
int8_t presetIndex = 0;

uint16_t airCalTempDelay = 0;
volatile uint16_t pumpDelayOverrideUs = 0;

unsigned long lastLCD = 0;
unsigned long lastSerial = 0;

char transientMessage[17] = "";
unsigned long transientUntil = 0;

// ================================================================
// ENCODERS
// ================================================================
int lastTempCLK = HIGH;
int lastAirCLK = HIGH;

// ================================================================
// BOTONES
// ================================================================
struct ButtonState {
  uint8_t pin;
  bool stableState;
  bool rawState;
  unsigned long lastRawChange;
  unsigned long pressedSince;
  bool longFired;
};

ButtonState btnTemp = { PIN_TEMP_SW, HIGH, HIGH, 0, 0, false };
ButtonState btnAir  = { PIN_AIR_SW,  HIGH, HIGH, 0, 0, false };

enum ButtonEvent : uint8_t {
  BTN_NONE = 0,
  BTN_SHORT,
  BTN_LONG
};

// ================================================================
// UTILIDADES
// ================================================================
template<typename T>
T clampValue(T v, T lo, T hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void beep(uint16_t freq, uint16_t durationMs) {
  tone(PIN_SPEAKER, freq, durationMs);
}

void showMessage(const char* msg, unsigned long durationMs = 1200UL) {
  strncpy(transientMessage, msg, 16);
  transientMessage[16] = '\0';
  transientUntil = millis() + durationMs;
}

void printLine(uint8_t row, const char* text) {
  char b[17];
  memset(b, ' ', 16);
  b[16] = '\0';

  size_t n = strlen(text);
  if (n > 16) n = 16;
  memcpy(b, text, n);

  lcd.setCursor(0, row);
  lcd.print(b);
}

const char* faultText(FaultCode f) {
  switch (f) {
    case FAULT_TC:           return "TC ERROR";
    case FAULT_OVERTEMP:     return "OVER TEMP";
    case FAULT_RUNAWAY:      return "THERM RUNAWAY";
    case FAULT_NO_RISE:      return "NO HEAT RISE";
    case FAULT_CONTROL_LOST: return "CONTROL FAULT";
    default:                 return "NO FAULT";
  }
}

// ================================================================
// EEPROM
// ================================================================
uint8_t calcChecksum(const ConfigData& c) {
  const uint8_t* p = reinterpret_cast<const uint8_t*>(&c);
  uint8_t x = 0x5A;

  // No incluir el ultimo byte (checksum).
  for (size_t i = 0; i < sizeof(ConfigData) - 1; i++) {
    x ^= p[i];
    x = (uint8_t)((x << 1) | (x >> 7));
  }
  return x;
}

void loadDefaults() {
  memset(&cfg, 0, sizeof(cfg));

  cfg.magic = CONFIG_MAGIC;
  cfg.version = CONFIG_VERSION;

  cfg.lastTempC = 300;
  cfg.lastAirPct = 50;

  // Valores conservadores para primer arranque.
  // El autotune debe reemplazarlos con los valores reales de la estacion.
  cfg.kp = 1.00f;
  cfg.ki = 0.02f;
  cfg.kd = 4.00f;

  cfg.pidMode = 0;

  cfg.pid20.kp = cfg.kp;  cfg.pid20.ki = cfg.ki;  cfg.pid20.kd = cfg.kd;  cfg.pid20.valid = 0;
  cfg.pid50.kp = cfg.kp;  cfg.pid50.ki = cfg.ki;  cfg.pid50.kd = cfg.kd;  cfg.pid50.valid = 0;
  cfg.pid100.kp = cfg.kp; cfg.pid100.ki = cfg.ki; cfg.pid100.kd = cfg.kd; cfg.pid100.valid = 0;

  // Phase-angle inicial conservador.
  // Se calibra fisicamente desde el menu.
  cfg.airDelay20us = 7600;
  cfg.airDelay100us = 3000;

  cfg.preset[0].tempC = 250;
  cfg.preset[0].airPct = 40;

  cfg.preset[1].tempC = 350;
  cfg.preset[1].airPct = 60;

  cfg.preset[2].tempC = 450;
  cfg.preset[2].airPct = 80;

  cfg.checksum = calcChecksum(cfg);
}

void saveConfigNow() {
  cfg.magic = CONFIG_MAGIC;
  cfg.version = CONFIG_VERSION;
  cfg.lastTempC = setPointC;
  cfg.lastAirPct = (uint8_t)airSetPct;
  cfg.checksum = calcChecksum(cfg);

  EEPROM.put(0, cfg);

  configDirty = false;
}

void markConfigDirty() {
  configDirty = true;
  configDirtySince = millis();
}

void serviceDeferredEEPROM() {
  if (!configDirty) return;

  if (millis() - configDirtySince >= EEPROM_SAVE_DELAY_MS) {
    saveConfigNow();
  }
}

void loadConfig() {
  EEPROM.get(0, cfg);

  bool valid =
    cfg.magic == CONFIG_MAGIC &&
    cfg.version == CONFIG_VERSION &&
    cfg.checksum == calcChecksum(cfg);

  if (!valid) {
    loadDefaults();
    EEPROM.put(0, cfg);
  }

  setPointC = clampValue((int)cfg.lastTempC, TEMP_MIN_C, TEMP_MAX_C);
  airSetPct = clampValue((int)cfg.lastAirPct, AIR_MIN_PCT, AIR_MAX_PCT);

  // Redondeo a pasos.
  setPointC = ((setPointC + TEMP_STEP_C / 2) / TEMP_STEP_C) * TEMP_STEP_C;
  airSetPct = ((airSetPct + AIR_STEP_PCT / 2) / AIR_STEP_PCT) * AIR_STEP_PCT;

  if (airSetPct > 0) lastNonZeroAirPct = airSetPct;
}

// ================================================================
// BOTONES
// ================================================================
ButtonEvent updateButton(ButtonState& b) {
  bool raw = digitalRead(b.pin);
  unsigned long now = millis();

  if (raw != b.rawState) {
    b.rawState = raw;
    b.lastRawChange = now;
  }

  if (now - b.lastRawChange < BUTTON_DEBOUNCE_MS) {
    return BTN_NONE;
  }

  if (b.stableState != b.rawState) {
    b.stableState = b.rawState;

    if (b.stableState == LOW) {
      b.pressedSince = now;
      b.longFired = false;
    } else {
      if (!b.longFired) {
        return BTN_SHORT;
      }
    }
  }

  if (b.stableState == LOW && !b.longFired) {
    if (now - b.pressedSince >= BUTTON_LONG_MS) {
      b.longFired = true;
      return BTN_LONG;
    }
  }

  return BTN_NONE;
}

// ================================================================
// ENCODERS
// ================================================================
int8_t readTempEncoderStep() {
  int clk = digitalRead(PIN_TEMP_CLK);
  int8_t step = 0;

  if (clk != lastTempCLK && clk == LOW) {
    step = (digitalRead(PIN_TEMP_DT) != clk) ? +1 : -1;
  }

  lastTempCLK = clk;
  return step;
}

int8_t readAirEncoderStep() {
  int clk = digitalRead(PIN_AIR_CLK);
  int8_t step = 0;

  if (clk != lastAirCLK && clk == LOW) {
    step = (digitalRead(PIN_AIR_DT) != clk) ? +1 : -1;
  }

  lastAirCLK = clk;
  return step;
}

// ================================================================
// PUMP PHASE ANGLE
// ================================================================
uint16_t airPercentToDelayUs(uint8_t pct) {
  if (pct == 0) return PUMP_DELAY_MAX_US;

  // 5..20 %: extrapolacion suave hasta el punto calibrado de 20 %.
  if (pct < 20) {
    uint16_t d20 = cfg.airDelay20us;
    uint16_t extra = (uint16_t)(20 - pct) * 90U;
    uint32_t d = (uint32_t)d20 + extra;
    if (d > PUMP_DELAY_MAX_US) d = PUMP_DELAY_MAX_US;
    return (uint16_t)d;
  }

  // Interpolacion lineal 20..95 %.
  // 100 % no usa phase-angle: usa conduccion completa.
  long spanPct = 75;
  long posPct = pct - 20;
  if (posPct > spanPct) posPct = spanPct;

  long d20 = cfg.airDelay20us;
  long d95 = cfg.airDelay100us;

  long delayUs = d20 + ((d95 - d20) * posPct) / spanPct;

  return (uint16_t)clampValue<long>(
    delayUs,
    PUMP_DELAY_MIN_US,
    PUMP_DELAY_MAX_US
  );
}

void setupTimer1() {
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // Prescaler 8: 16 MHz / 8 = 2 MHz -> tick 0.5 us.
  TCCR1B |= (1 << CS11);

  TIMSK1 &= ~((1 << OCIE1A) | (1 << OCIE1B));

  sei();
}

ISR(TIMER1_COMPA_vect) {
  if (effectiveAirPctISR > 0) {
    digitalWrite(PIN_PUMP, HIGH);

    uint16_t pulseTicks = (uint16_t)(PUMP_GATE_PULSE_US * 2U);
    OCR1B = (uint16_t)(TCNT1 + pulseTicks);
    TIFR1 |= (1 << OCF1B);
    TIMSK1 |= (1 << OCIE1B);
  }

  TIMSK1 &= ~(1 << OCIE1A);
}

ISR(TIMER1_COMPB_vect) {
  digitalWrite(PIN_PUMP, LOW);
  TIMSK1 &= ~(1 << OCIE1B);
}

// ================================================================
// ZERO CROSS
// ================================================================
void zeroCrossISR() {
  unsigned long nowUs = micros();

  if (lastZcMicros != 0) {
    unsigned long delta = nowUs - lastZcMicros;

    // Aceptar 45..65 Hz aproximadamente.
    if (delta >= 7500UL && delta <= 11500UL) {
      measuredHalfCycleUs =
        (uint16_t)((measuredHalfCycleUs * 7UL + delta) / 8UL);
    }
  }

  lastZcMicros = nowUs;
  zcCount++;

  // ------------------------------------------------------------
  // HEATER: ciclos COMPLETOS AC con distribucion sigma-delta.
  // Cambiamos decision solo cada 2 semiciclos.
  // ------------------------------------------------------------
  if (!heaterSecondHalf) {
    if (heaterPowerAllowed && heaterDutyPct > 0) {
      heaterBurstAccumulator += heaterDutyPct;

      if (heaterBurstAccumulator >= 100U) {
        heaterBurstAccumulator -= 100U;
        heaterFullCycleOn = true;
      } else {
        heaterFullCycleOn = false;
      }
    } else {
      heaterFullCycleOn = false;
      heaterBurstAccumulator = 0;
    }

    digitalWrite(PIN_HEATER, heaterFullCycleOn ? HIGH : LOW);
    heaterSecondHalf = true;

  } else {
    // Mantener exactamente el mismo estado para el segundo semiciclo.
    digitalWrite(PIN_HEATER, heaterFullCycleOn ? HIGH : LOW);
    heaterSecondHalf = false;
  }

  // ------------------------------------------------------------
  // BOMBA:
  // 100% = MOC3023 encendido continuamente => potencia completa real.
  // 1..95% = phase-angle sincronizado al zero-cross.
  // ------------------------------------------------------------
  uint8_t air = effectiveAirPctISR;

  TIMSK1 &= ~((1 << OCIE1A) | (1 << OCIE1B));

  if (air >= 100 && pumpDelayOverrideUs == 0) {
    // Igual al firmware de diagnostico: D3 HIGH continuo.
    digitalWrite(PIN_PUMP, HIGH);
    return;
  }

  digitalWrite(PIN_PUMP, LOW);

  if (air > 0) {
    uint16_t overrideDelay = pumpDelayOverrideUs;
    uint16_t delayUs = overrideDelay > 0 ? overrideDelay : airPercentToDelayUs(air);

    // No disparar demasiado cerca del proximo zero-cross medido.
    uint16_t maxAllowed =
      (measuredHalfCycleUs > 500) ?
      (measuredHalfCycleUs - 400) :
      PUMP_DELAY_MAX_US;

    if (delayUs > maxAllowed) delayUs = maxAllowed;
    if (delayUs < PUMP_DELAY_MIN_US) delayUs = PUMP_DELAY_MIN_US;

    TCNT1 = 0;
    OCR1A = (uint16_t)(delayUs * 2U);
    TIFR1 |= (1 << OCF1A);
    TIMSK1 |= (1 << OCIE1A);
  }
}

// ================================================================
// TEMPERATURA
// ================================================================
void serviceTemperature() {
  static unsigned long lastRead = 0;
  unsigned long now = millis();

  if (now - lastRead < TEMP_SAMPLE_MS) return;
  lastRead = now;

  previousTempC = tempC;
  float t = thermocouple.readCelsius();

  bool valid = !isnan(t) && t >= -10.0f && t <= 550.0f;

  if (valid) {
    tempC = t;
    tcValid = true;
    invalidTcCount = 0;
  } else {
    tcValid = false;
    if (invalidTcCount < 255) invalidTcCount++;

    // Solo convertir en fault enclavado si se estaba usando calor,
    // si la estacion estaba caliente, o si ya venia leyendo mal.
    if (heatRequested ||
        (!isnan(tempC) && tempC >= COOLDOWN_TEMP_C)) {
      // fault se procesa abajo fuera de esta funcion.
    }
  }
}

// ================================================================
// ESTADO / POTENCIA
// ================================================================
bool heaterCanRun() {
  if (faultCode != FAULT_NONE) return false;
  if (!tcValid) return false;
  if (standPressed) return false;
  if (!heatRequested) return false;
  if (airSetPct < AIR_MIN_HEATER_PCT) return false;

  return true;
}

uint8_t computeEffectiveAirPct() {
  // FAULT cooldown enclavado:
  // - al entrar en fault: bomba 100%.
  // - sigue enfriando hasta <=68 C.
  // - una vez llegado a <=68 C queda OFF y NO vuelve a arrancar
  //   por rebote termico mientras siga siendo el mismo FAULT.
  // - si TC es invalida antes de completar el cooldown, mantener 100%.
  if (faultCode != FAULT_NONE) {
    if (!tcValid) {
      return faultCoolingActive ? 100 : 0;
    }

    if (faultCoolingActive && tempC <= FAULT_COOL_OFF_C) {
      faultCoolingActive = false;
    }

    return faultCoolingActive ? 100 : 0;
  }

  // Autotune activo usa exactamente el caudal del punto que se esta midiendo.
  if (uiMode == UI_AUTOTUNE_RUNNING) {
    return (uint8_t)max(autotuneAirPct, AIR_MIN_HEATER_PCT);
  }

  // Cooldown forzado: una vez iniciado, bomba 100% hasta <=68 C.
  // NO se apaga al cruzar 80 C.
  if (cooldownActive || uiMode == UI_AUTOTUNE_COOLDOWN) {
    if (!tcValid) return 100;
    return 100;
  }

  if (tcValid && tempC >= COOLDOWN_TEMP_C) {
    // Si heater esta activo usamos el valor del usuario.
    if (heaterCanRun()) {
      return (uint8_t)max(airSetPct, AIR_MIN_HEATER_PCT);
    }

    // Si ya completo un cooldown a <=68 C, un rebote termico posterior
    // NO debe volver a encender la bomba por si solo.
    if (cooldownCompletedLatch && !heatRequested) {
      return 0;
    }

    // Si aun no se completo un cooldown seguro, mantener enfriamiento.
    return 100;
  }

  // Stand frio: bomba apagada.
  if (standPressed) {
    return 0;
  }

  // Heater requiere minimo 20 %.
  if (heaterCanRun()) {
    return (uint8_t)max(airSetPct, AIR_MIN_HEATER_PCT);
  }

  // Bomba manual en frio.
  if (pumpManualEnabled && airSetPct > 0) {
    return (uint8_t)airSetPct;
  }

  return 0;
}


void beginCooldown() {
  cooldownActive = true;
  cooldownCompletedLatch = false;

  // Durante un cooldown iniciado por el firmware la bomba queda bajo
  // control automatico hasta llegar a <=68 C.
  pumpManualEnabled = false;
}

void forceHeaterOffPhysical() {
  heaterPowerAllowed = false;
  heaterDutyPct = 0;
  digitalWrite(PIN_HEATER, LOW);
  digitalWrite(PIN_RELAY, LOW);

  pidOutput = 0.0f;
  pidIntegral = 0.0f;
}

void setFault(FaultCode code) {
  if (faultCode != FAULT_NONE) return;

  faultCode = code;
  heatRequested = false;

  forceHeaterOffPhysical();

  // Iniciar cooldown de FAULT inmediatamente.
  faultCoolingActive = true;
  effectiveAirPctISR = 100;

  uiMode = UI_FAULT;

  beep(350, 700);
}

// ================================================================
// PID
// ================================================================
void resetControlSupervision() {
  stableBandSince = 0;
  regulationWasStable = false;
  controlLostSince = 0;

  noRiseSince = 0;
  noRiseStartTemp = tempC;

  runawaySince = 0;
  runawayArmed = false;

  controlGraceUntil = millis() + 5000UL;
}

void resetPID() {
  pidIntegral = 0.0f;
  lastPidMeasurement = tempC;
  lastPidMs = millis();
  pidOutput = 0.0f;

  resetControlSupervision();
}


bool pidTableValid() {
  return cfg.pid20.valid && cfg.pid50.valid && cfg.pid100.valid;
}

void getActivePid(float &kp, float &ki, float &kd) {
  if (cfg.pidMode != 1 || !pidTableValid()) {
    kp = cfg.kp;
    ki = cfg.ki;
    kd = cfg.kd;
    return;
  }

  int air = clampValue(airSetPct, 20, 100);

  const PidPointData* a;
  const PidPointData* b;
  float x;

  if (air <= 50) {
    a = &cfg.pid20;
    b = &cfg.pid50;
    x = (air - 20) / 30.0f;
  } else {
    a = &cfg.pid50;
    b = &cfg.pid100;
    x = (air - 50) / 50.0f;
  }

  x = clampValue(x, 0.0f, 1.0f);

  kp = a->kp + x * (b->kp - a->kp);
  ki = a->ki + x * (b->ki - a->ki);
  kd = a->kd + x * (b->kd - a->kd);
}

void servicePID() {
  if (uiMode == UI_AUTOTUNE_RUNNING) return;

  unsigned long now = millis();

  if (now - lastPidMs < PID_SAMPLE_MS) return;

  float dt = (lastPidMs == 0) ? 0.25f : (now - lastPidMs) / 1000.0f;
  lastPidMs = now;
  if (!heaterCanRun() || !tcValid) {
    pidOutput = 0.0f;
    heaterDutyPct = 0;
    heaterPowerAllowed = false;
    digitalWrite(PIN_RELAY, LOW);
    return;
  }

  float error = (float)setPointC - tempC;

  float kpNow, kiNow, kdNow;
  getActivePid(kpNow, kiNow, kdNow);

  // Proporcional.
  float p = kpNow * error;

  // Integral con anti-windup.
  pidIntegral += kiNow * error * dt;
  pidIntegral = clampValue(pidIntegral, 0.0f, 100.0f);

  // Derivada sobre medicion para evitar derivative kick al mover setpoint.
  float d = 0.0f;

  if (!isnan(lastPidMeasurement) && dt > 0.01f) {
    d = -kdNow * ((tempC - lastPidMeasurement) / dt);
  }

  lastPidMeasurement = tempC;

  float out = p + pidIntegral + d;
  out = clampValue(out, 0.0f, (float)PID_MAX_DUTY_PCT);

  pidOutput = out;

  heaterDutyPct = (uint8_t)(out + 0.5f);
  heaterPowerAllowed = true;

  digitalWrite(PIN_RELAY, HIGH);
}

// ================================================================
// THERMAL SUPERVISION
// ================================================================
void serviceThermalSafety() {
  unsigned long now = millis();

  // TC.
  if (!tcValid) {
    if (heatRequested ||
        (!isnan(tempC) && tempC >= COOLDOWN_TEMP_C)) {
      setFault(FAULT_TC);
    }
    return;
  }

  // Limite absoluto.
  if (tempC >= ABS_OVER_TEMP_C) {
    setFault(FAULT_OVERTEMP);
    return;
  }

  // Durante PID AUTOTUNE NO se aplica el runaway relativo SP+80 C.
  // El autotune necesita provocar oscilacion y overshoot para medir Ku/Pu.
  // Siguen activas siempre:
  //   - termocupla valida
  //   - OVER TEMP absoluto a 500 C (evaluado arriba)
  //
  // Fuera de autotune, el thermal runaway relativo sigue funcionando
  // exactamente como proteccion normal.
  if (uiMode == UI_AUTOTUNE_RUNNING) {
    runawaySince = 0;
    runawayArmed = false;
    return;
  }

  float activeTarget = (float)setPointC;
  bool thermalControlActive = heaterCanRun();

  // THERMAL RUNAWAY relativo en uso normal.
  // Se arma solo despues de que la temperatura estuvo razonablemente
  // cerca/por debajo del target actual. Esto evita un falso runaway
  // si el usuario baja el setpoint mientras el mango sigue caliente.
  if (thermalControlActive && tempC <= activeTarget + 20.0f) {
    runawayArmed = true;
  }

  if (thermalControlActive &&
      runawayArmed &&
      now >= controlGraceUntil &&
      tempC >= activeTarget + RUNAWAY_DELTA_C) {

    if (runawaySince == 0) runawaySince = now;

    if (now - runawaySince >= RUNAWAY_CONFIRM_MS) {
      setFault(FAULT_RUNAWAY);
      return;
    }
  } else {
    runawaySince = 0;
  }

  if (!heaterCanRun()) {
    noRiseSince = 0;
    controlLostSince = 0;
    stableBandSince = 0;
    return;
  }

  if (now < controlGraceUntil) return;

  float error = (float)setPointC - tempC;
  float absError = fabs(error);

  // ------------------------------------------------------------
  // THERMAL CONTROL FAULT A:
  // mucha potencia + lejos del objetivo + casi no aumenta temperatura.
  // ------------------------------------------------------------
  if (pidOutput >= NO_RISE_MIN_DUTY &&
      error >= NO_RISE_MIN_ERROR_C) {

    if (noRiseSince == 0) {
      noRiseSince = now;
      noRiseStartTemp = tempC;
    }

    if (now - noRiseSince >= NO_RISE_WINDOW_MS) {
      float rise = tempC - noRiseStartTemp;

      if (rise < NO_RISE_MIN_DELTA_C) {
        setFault(FAULT_NO_RISE);
        return;
      }

      // Ventana movil: progreso suficiente, volver a observar.
      noRiseSince = now;
      noRiseStartTemp = tempC;
    }

  } else {
    noRiseSince = 0;
  }

  // ------------------------------------------------------------
  // Detectar que alguna vez se estabilizo.
  // ------------------------------------------------------------
  if (absError <= STABLE_BAND_C) {
    if (stableBandSince == 0) stableBandSince = now;

    if (now - stableBandSince >= STABLE_REQUIRED_MS) {
      regulationWasStable = true;
    }
  } else {
    stableBandSince = 0;
  }

  // ------------------------------------------------------------
  // THERMAL CONTROL FAULT B:
  // ya estaba estable y luego se aleja demasiado por tiempo sostenido.
  // ------------------------------------------------------------
  if (regulationWasStable && absError >= CONTROL_LOST_ERROR_C) {
    if (controlLostSince == 0) controlLostSince = now;

    if (now - controlLostSince >= CONTROL_LOST_MS) {
      setFault(FAULT_CONTROL_LOST);
      return;
    }
  } else {
    controlLostSince = 0;
  }
}

// ================================================================
// AUTOTUNE
// ================================================================
void resetTuneStats(TuneStats &s) {
  s.sum = 0.0f;
  s.minV = 1000000.0f;
  s.maxV = -1000000.0f;
  s.count = 0;
}

void addTuneSample(TuneStats &s, float v) {
  s.sum += v;
  if (v < s.minV) s.minV = v;
  if (v > s.maxV) s.maxV = v;
  if (s.count < 255) s.count++;
}

float averageTuneStats(const TuneStats &s) {
  if (s.count == 0) return 0.0f;

  // AUTO 3 PUNTOS (4 ciclos): promedio normal.
  if (s.count < 5) {
    return s.sum / (float)s.count;
  }

  // Autotune largo: elimina SOLO un minimo y un maximo.
  return (s.sum - s.minV - s.maxV) / (float)(s.count - 2);
}

void resetAutotuneData() {
  resetTuneStats(tuneHighStats);
  resetTuneStats(tuneLowStats);
  resetTuneStats(tunePeriodStats);

  tuneCurrentHighPeak = -1000.0f;
  tuneCurrentLowPeak = 1000.0f;

  tuneLastUpCrossMs = 0;
  tuneStartMs = millis();

  autotuneHighPhase = true;
}

uint8_t getAutotuneCycleTarget() {
  return (autotuneMode == AT_CURRENT)
    ? AUTOTUNE_ACTUAL_CYCLES
    : AUTOTUNE_3P_CYCLES;
}

bool calculateAutotunePid(PidPointData &out) {
  // En el autotune largo (10 ciclos), este promedio elimina solo
  // un extremo alto y uno bajo; conserva la primera oscilacion completa.
  // En AUTO 3 PUNTOS (4 ciclos), usa promedio normal.
  float avgHigh = averageTuneStats(tuneHighStats);
  float avgLow = averageTuneStats(tuneLowStats);
  float avgPeriod = averageTuneStats(tunePeriodStats);

  float amplitude = (avgHigh - avgLow) * 0.5f;
  const float relayAmplitude = (AUTOTUNE_HIGH_DUTY_PCT - AUTOTUNE_LOW_DUTY_PCT) * 0.5f;

  if (amplitude < 1.0f ||
      amplitude > 100.0f ||
      avgPeriod < 1.0f ||
      avgPeriod > 120.0f) {
    return false;
  }

  float Ku = (4.0f * relayAmplitude) / (PI * amplitude);
  float Pu = avgPeriod;

  // Tyreus-Luyben PID: conservador para sistemas termicos con inercia.
  float newKp = Ku / 2.2f;
  float Ti = 2.2f * Pu;
  float Td = Pu / 6.3f;
  float newKi = newKp / Ti;
  float newKd = newKp * Td;

  if (newKp <= 0.0f || newKp > 50.0f ||
      newKi < 0.0f || newKi > 10.0f ||
      newKd < 0.0f || newKd > 500.0f) {
    return false;
  }

  out.kp = newKp;
  out.ki = newKi;
  out.kd = newKd;
  out.valid = 1;
  return true;
}

void prepareAutotuneRun(int airPct, int targetC) {
  autotuneAirPct = clampValue(airPct, AIR_MIN_HEATER_PCT, AIR_MAX_PCT);
  autotuneTargetC = (float)clampValue(targetC, 150, TEMP_MAX_C);

  heatRequested = false;
  cooldownActive = false;
  pumpManualEnabled = false;

  resetPID();
  resetAutotuneData();

  uiMode = UI_AUTOTUNE_RUNNING;

  digitalWrite(PIN_RELAY, HIGH);
  heaterPowerAllowed = true;
  heaterDutyPct = AUTOTUNE_HIGH_DUTY_PCT;

  controlGraceUntil = millis() + 5000UL;

  beep(2200, 120);
}

void startAutotuneStage() {
  if (autotuneMode == AT_THREE_POINT) {
    int air = AUTOTUNE_AIR_POINTS[autotuneStage];
    prepareAutotuneRun(air, autotuneSelectedTempC);
  } else {
    prepareAutotuneRun(airSetPct, autotuneSelectedTempC);
  }
}

void startAutotuneFromMenu() {
  if (!tcValid) {
    showMessage("TC INVALIDA");
    return;
  }

  if (autotuneMode == AT_CURRENT &&
      airSetPct < AIR_MIN_HEATER_PCT) {
    showMessage("AIR MIN 20%");
    return;
  }

  autotuneSelectedTempC =
    clampValue(autotuneSelectedTempC, 150, TEMP_MAX_C);

  autotuneStage = 0;
  autotuneNextStagePending = false;

  for (uint8_t i = 0; i < 3; i++) {
    tuneResult[i].kp = 0.0f;
    tuneResult[i].ki = 0.0f;
    tuneResult[i].kd = 0.0f;
    tuneResult[i].valid = 0;
  }

  startAutotuneStage();
}

void abortAutotune(const char* msg) {
  heaterDutyPct = 0;
  heaterPowerAllowed = false;
  digitalWrite(PIN_RELAY, LOW);

  autotuneNextStagePending = false;

  if (!tcValid || tempC >= COOLDOWN_TEMP_C) {
    beginCooldown();
  }

  uiMode = UI_SETTINGS;
  showMessage(msg, 1800);

  resetPID();
}

void finishAutotuneCycle() {
  PidPointData result;

  if (!calculateAutotunePid(result)) {
    abortAutotune("TUNE INVALIDO");
    return;
  }

  heaterDutyPct = 0;
  heaterPowerAllowed = false;
  digitalWrite(PIN_RELAY, LOW);

  if (autotuneMode == AT_CURRENT) {
    // ACTUAL: PID simple para el caudal actual.
    cfg.kp = result.kp;
    cfg.ki = result.ki;
    cfg.kd = result.kd;
    cfg.pidMode = 0;

    saveConfigNow();

    beginCooldown();

    menuIndex = 0;
    uiMode = UI_PID_VALUES;

    showMessage("PID ACTUAL OK", 1800);
    beep(3000, 180);

    resetPID();
    return;
  }

  // AUTO 3 PUNTOS.
  tuneResult[autotuneStage] = result;

  if (autotuneStage < 2) {
    autotuneStage++;
    autotuneNextStagePending = true;

    beginCooldown();
    uiMode = UI_AUTOTUNE_COOLDOWN;

    showMessage("COOL PROX PID", 1600);
    beep(1800, 100);
    resetPID();
    return;
  }

  // Los 3 puntos fueron medidos correctamente.
  cfg.pid20 = tuneResult[0];
  cfg.pid50 = tuneResult[1];
  cfg.pid100 = tuneResult[2];
  cfg.pidMode = 1;

  // Guardar tambien el punto de 50% como fallback del PID simple.
  cfg.kp = cfg.pid50.kp;
  cfg.ki = cfg.pid50.ki;
  cfg.kd = cfg.pid50.kd;

  saveConfigNow();

  autotuneNextStagePending = false;
  beginCooldown();

  menuIndex = 0;
  uiMode = UI_PID_VALUES;

  showMessage("PID 3P GUARDADO", 1800);
  beep(3200, 200);

  resetPID();
}

void serviceAutotuneCooldown() {
  if (uiMode != UI_AUTOTUNE_COOLDOWN) return;

  // Mientras no podamos confiar en la TC, mantener el cooldown.
  if (!tcValid) return;

  if (tempC > COOLDOWN_STOP_C) return;

  // Ya esta por debajo de 80 C: la bomba puede apagarse.
  cooldownActive = false;
  pumpManualEnabled = false;

  if (!autotuneNextStagePending) {
    uiMode = UI_SETTINGS;
    return;
  }

  autotuneNextStagePending = false;
  startAutotuneStage();
}

void serviceCooldown() {
  // AUTO 3 PUNTOS usa su propio cooldown entre etapas.
  if (uiMode == UI_AUTOTUNE_COOLDOWN) {
    serviceAutotuneCooldown();
    return;
  }

  if (!cooldownActive) return;
  if (!tcValid) return;

  // El cooldown iniciado por el firmware NO termina al cruzar 80 C.
  // Sigue forzando la bomba hasta llegar a 68 C o menos.
  if (tempC <= COOLDOWN_STOP_C) {
    cooldownActive = false;
    cooldownCompletedLatch = true;
    pumpManualEnabled = false;

    showMessage("COOLDOWN OK", 900);
  }
}

void serviceAutotune() {
  if (uiMode != UI_AUTOTUNE_RUNNING) return;

  unsigned long now = millis();

  if (now - tuneStartMs > AUTOTUNE_TIMEOUT_MS) {
    abortAutotune("TUNE TIMEOUT");
    return;
  }

  if (!tcValid) {
    setFault(FAULT_TC);
    return;
  }

  // Proteccion especifica del autotune para el heater Yihua temporal.
  // Si la inercia supera demasiado el objetivo, cortar y enfriar en lugar
  // de permitir que el metodo relay siga excitando el calefactor.
  if (tempC >= autotuneTargetC + AUTOTUNE_ABORT_MARGIN_C) {
    abortAutotune("TUNE OVER TEMP");
    return;
  }

  // Safety absoluto y runaway se procesa en serviceThermalSafety().

  if (autotuneHighPhase) {
    heaterPowerAllowed = true;
    heaterDutyPct = AUTOTUNE_HIGH_DUTY_PCT;
    digitalWrite(PIN_RELAY, HIGH);

    if (tempC < tuneCurrentLowPeak) tuneCurrentLowPeak = tempC;

    // Cruce ascendente.
    if (tempC >= autotuneTargetC + AUTOTUNE_HYST_C) {
      unsigned long upNow = now;

      // Ignorar SOLO el minimo inicial desde ambiente: no es una oscilacion completa.
      // La primera oscilacion completa alrededor del setpoint SI se conserva.
      if (tuneHighStats.count > 0 && tuneLowStats.count < getAutotuneCycleTarget()) {
        addTuneSample(tuneLowStats, tuneCurrentLowPeak);
      }

      if (tuneLastUpCrossMs != 0 && tunePeriodStats.count < getAutotuneCycleTarget()) {
        addTuneSample(
          tunePeriodStats,
          (upNow - tuneLastUpCrossMs) / 1000.0f
        );
      }

      tuneLastUpCrossMs = upNow;

      autotuneHighPhase = false;
      tuneCurrentHighPeak = tempC;

      heaterDutyPct = 0;
    }

  } else {
    heaterPowerAllowed = true;
    heaterDutyPct = AUTOTUNE_LOW_DUTY_PCT;
    digitalWrite(PIN_RELAY, HIGH);

    if (tempC > tuneCurrentHighPeak) tuneCurrentHighPeak = tempC;

    // Cruce descendente.
    if (tempC <= autotuneTargetC - AUTOTUNE_HYST_C) {
      if (tuneHighStats.count < getAutotuneCycleTarget()) {
        addTuneSample(tuneHighStats, tuneCurrentHighPeak);
      }

      autotuneHighPhase = true;
      tuneCurrentLowPeak = tempC;

      uint8_t targetCycles = getAutotuneCycleTarget();
      uint8_t minLowPeriod = targetCycles - 1;

      if (tuneHighStats.count >= targetCycles &&
          tuneLowStats.count >= minLowPeriod &&
          tunePeriodStats.count >= minLowPeriod) {
        finishAutotuneCycle();
        return;
      }

      heaterDutyPct = AUTOTUNE_HIGH_DUTY_PCT;
    }
  }
}

// ================================================================
// STAND
// ================================================================
void serviceStand() {
  standPressed = (digitalRead(PIN_STAND) == HIGH);

  // Durante PID AUTOTUNE el stand se ignora intencionalmente.
  // Esto permite calibrar con el mango apoyado.
  // Fuera del autotune, el stand conserva toda su funcion de seguridad normal.
  if (uiMode == UI_AUTOTUNE_RUNNING || uiMode == UI_AUTOTUNE_COOLDOWN) {
    lastStandPressed = standPressed;
    return;
  }

  if (standPressed != lastStandPressed) {
    if (standPressed) {
      // Heater OFF fisico inmediato.
      heaterPowerAllowed = false;
      heaterDutyPct = 0;
      digitalWrite(PIN_HEATER, LOW);
      digitalWrite(PIN_RELAY, LOW);

      // heatRequested NO se borra:
      // al levantar se reanuda automaticamente.
      if (!tcValid || tempC >= COOLDOWN_TEMP_C) {
        beginCooldown();
      }

      showMessage("COOLDOWN");
      beep(900, 90);

      resetPID();

    } else {
      // Se levanto el mango.
      // Si habia una solicitud de calor previa, cancelar cooldown y reanudar.
      if (heatRequested && faultCode == FAULT_NONE) {
        cooldownActive = false;
      }

      showMessage("MANGO ACTIVO");
      beep(2200, 70);

      // Si heatRequested seguia true, heaterCanRun() lo reanuda.
      resetPID();
    }

    lastStandPressed = standPressed;
  }
}

// ================================================================
// INPUT NORMAL
// ================================================================
void changeSetPoint(int8_t step) {
  if (step == 0) return;

  int v = setPointC + step * TEMP_STEP_C;
  v = clampValue(v, TEMP_MIN_C, TEMP_MAX_C);

  if (v != setPointC) {
    setPointC = v;
    markConfigDirty();
    resetPID();
  }
}

void changeAirSet(int8_t step) {
  if (step == 0) return;

  int v = airSetPct + step * AIR_STEP_PCT;
  v = clampValue(v, AIR_MIN_PCT, AIR_MAX_PCT);

  // Con heater solicitado no permitir caer por debajo del minimo.
  if (heatRequested && v < AIR_MIN_HEATER_PCT) {
    v = AIR_MIN_HEATER_PCT;
    showMessage("AIR MIN 20%");
  }

  if (v != airSetPct) {
    airSetPct = v;

    if (airSetPct > 0) {
      lastNonZeroAirPct = airSetPct;
      pumpManualEnabled = true;
    } else if (!heatRequested && tcValid && tempC < COOLDOWN_TEMP_C) {
      pumpManualEnabled = false;
    }

    markConfigDirty();
    resetControlSupervision();
  }
}

void toggleHeaterRequest() {
  if (faultCode != FAULT_NONE) return;

  if (heatRequested) {
    heatRequested = false;
    forceHeaterOffPhysical();

    if (!tcValid || tempC >= COOLDOWN_TEMP_C) {
      beginCooldown();
    }

    showMessage("HEATER OFF");
    beep(1000, 90);
    return;
  }

  if (!tcValid) {
    showMessage("TC INVALIDA");
    beep(400, 250);
    return;
  }

  if (standPressed) {
    showMessage("MANGO EN BASE");
    beep(450, 220);
    return;
  }

  if (airSetPct < AIR_MIN_HEATER_PCT) {
    showMessage("AIR MIN 20%");
    beep(450, 220);
    return;
  }

  heatRequested = true;
  pumpManualEnabled = true;
  cooldownCompletedLatch = false;

  resetPID();

  showMessage("HEATER ON");
  beep(2500, 80);
}

void togglePumpManual() {
  // Si el usuario intenta apagar la bomba estando caliente,
  // iniciar un cooldown automatico hasta <=68 C.
  if (tcValid && tempC >= COOLDOWN_TEMP_C) {
    beginCooldown();
    showMessage("COOLDOWN ->68");
    beep(450, 180);
    return;
  }

  // Nunca permitir apagarla con heater solicitado.
  if (heatRequested) {
    pumpManualEnabled = true;
    showMessage("HEATER ACTIVO");
    beep(450, 180);
    return;
  }

  pumpManualEnabled = !pumpManualEnabled;

  if (pumpManualEnabled && airSetPct == 0) {
    airSetPct = max(lastNonZeroAirPct, AIR_MIN_HEATER_PCT);
  }

  showMessage(pumpManualEnabled ? "BOMBA ON" : "BOMBA OFF");
}

// ================================================================
// UI / MENUS
// ================================================================
const uint8_t SETTINGS_COUNT = 6;

const char* settingsLabel(uint8_t idx) {
  switch (idx) {
    case 0: return "PID AUTOTUNE";
    case 1: return "VER PID";
    case 2: return "AIR CAL 20%";
    case 3: return "AIR FULL TEST";
    case 4: return "GUARDAR PRESET";
    default: return "SALIR";
  }
}

void enterSettings() {
  heatRequested = false;
  forceHeaterOffPhysical();

  if (!tcValid || tempC >= COOLDOWN_TEMP_C) {
    beginCooldown();
  }

  menuIndex = 0;
  uiMode = UI_SETTINGS;
  showMessage("MENU");
}

void enterPresets() {
  presetIndex = 0;
  uiMode = UI_PRESETS;
}

void loadSelectedPreset() {
  PresetData& p = cfg.preset[presetIndex];

  setPointC = clampValue((int)p.tempC, TEMP_MIN_C, TEMP_MAX_C);
  airSetPct = clampValue((int)p.airPct, AIR_MIN_PCT, AIR_MAX_PCT);

  if (airSetPct > 0) lastNonZeroAirPct = airSetPct;

  markConfigDirty();
  resetPID();

  uiMode = UI_NORMAL;
  showMessage("PRESET CARGADO");
  beep(2400, 90);
}

void saveSelectedPreset() {
  cfg.preset[presetIndex].tempC = setPointC;
  cfg.preset[presetIndex].airPct = (uint8_t)airSetPct;

  saveConfigNow();

  uiMode = UI_SETTINGS;
  showMessage("PRESET GUARDADO");
  beep(2800, 100);
}

void serviceUiInputs() {
  int8_t tempStep = readTempEncoderStep();
  int8_t airStep = readAirEncoderStep();

  ButtonEvent tempEvt = updateButton(btnTemp);
  ButtonEvent airEvt = updateButton(btnAir);

  // ------------------------------------------------------------
  // FAULT
  // ------------------------------------------------------------
  if (uiMode == UI_FAULT) {
    if (tempEvt == BTN_SHORT) {
      if (tcValid && tempC <= FAULT_COOL_OFF_C) {
        faultCode = FAULT_NONE;
        faultCoolingActive = false;
        uiMode = UI_NORMAL;
        resetPID();

        showMessage("FAULT RESET");
        beep(2000, 100);
      } else {
        showMessage("ENFRIAR A 68C");
      }
    }
    return;
  }

  // ------------------------------------------------------------
  // AUTOTUNE RUNNING
  // AIR corto = abortar.
  // ------------------------------------------------------------
  if (uiMode == UI_AUTOTUNE_RUNNING) {
    if (airEvt == BTN_SHORT || airEvt == BTN_LONG) {
      abortAutotune("TUNE ABORTADO");
    }
    return;
  }

  // ------------------------------------------------------------
  // NORMAL
  // ------------------------------------------------------------
  if (uiMode == UI_NORMAL) {
    changeSetPoint(tempStep);
    changeAirSet(airStep);

    if (tempEvt == BTN_SHORT) {
      toggleHeaterRequest();
    } else if (tempEvt == BTN_LONG) {
      enterPresets();
    }

    if (airEvt == BTN_SHORT) {
      togglePumpManual();
    } else if (airEvt == BTN_LONG) {
      enterSettings();
    }

    return;
  }

  // ------------------------------------------------------------
  // PRESETS
  // ------------------------------------------------------------
  if (uiMode == UI_PRESETS) {
    if (tempStep != 0 || airStep != 0) {
      int s = (tempStep != 0) ? tempStep : airStep;
      presetIndex += s;

      if (presetIndex < 0) presetIndex = 2;
      if (presetIndex > 2) presetIndex = 0;
    }

    if (tempEvt == BTN_SHORT) {
      loadSelectedPreset();
    }

    if (airEvt == BTN_SHORT) {
      uiMode = UI_NORMAL;
    }

    return;
  }

  // ------------------------------------------------------------
  // SETTINGS
  // ------------------------------------------------------------
  if (uiMode == UI_SETTINGS) {
    if (tempStep != 0 || airStep != 0) {
      int s = (tempStep != 0) ? tempStep : airStep;
      menuIndex += s;

      if (menuIndex < 0) menuIndex = SETTINGS_COUNT - 1;
      if (menuIndex >= SETTINGS_COUNT) menuIndex = 0;
    }

    if (airEvt == BTN_SHORT) {
      uiMode = UI_NORMAL;
      return;
    }

    if (tempEvt == BTN_SHORT) {
      switch (menuIndex) {
        case 0:
          autotuneMenuChoice = 0;
          uiMode = UI_AUTOTUNE_MODE;
          break;

        case 1:
          menuIndex = 0;
          uiMode = UI_PID_VALUES;
          break;

        case 2:
          if (tcValid && tempC >= COOLDOWN_TEMP_C) {
            showMessage("ENFRIAR <80C");
          } else {
            airCalTempDelay = cfg.airDelay20us;
            uiMode = UI_AIR_CAL20;
          }
          break;

        case 3:
          if (tcValid && tempC >= COOLDOWN_TEMP_C) {
            showMessage("ENFRIAR <80C");
          } else {
            pumpDelayOverrideUs = 0;
            uiMode = UI_AIR_CAL100;
          }
          break;

        case 4:
          presetIndex = 0;
          uiMode = UI_SAVE_PRESET;
          break;

        default:
          uiMode = UI_NORMAL;
          break;
      }
    }

    return;
  }

  // ------------------------------------------------------------
  // PID VALUES
  // Rotacion alterna Kp/Ki/Kd solo para visualizar.
  // ------------------------------------------------------------
  if (uiMode == UI_PID_VALUES) {
    if (tempStep != 0 || airStep != 0) {
      int s = (tempStep != 0) ? tempStep : airStep;
      menuIndex += s;
      if (menuIndex < 0) menuIndex = 2;
      if (menuIndex > 2) menuIndex = 0;
    }

    if (airEvt == BTN_SHORT || tempEvt == BTN_SHORT) {
      uiMode = UI_SETTINGS;
    }

    return;
  }

  // ------------------------------------------------------------
  // PID AUTOTUNE: elegir ACTUAL o AUTO 3 PUNTOS
  // ------------------------------------------------------------
  if (uiMode == UI_AUTOTUNE_MODE) {
    if (tempStep != 0 || airStep != 0) {
      autotuneMenuChoice = 1 - autotuneMenuChoice;
    }

    if (tempEvt == BTN_SHORT) {
      autotuneMode =
        (autotuneMenuChoice == 0) ? AT_CURRENT : AT_THREE_POINT;

      autotuneSelectedTempC =
        clampValue(setPointC, 150, 400);

      uiMode = UI_AUTOTUNE_TEMP;
    }

    if (airEvt == BTN_SHORT) {
      uiMode = UI_SETTINGS;
    }

    return;
  }

  // ------------------------------------------------------------
  // PID AUTOTUNE: elegir temperatura de calibracion
  // ------------------------------------------------------------
  if (uiMode == UI_AUTOTUNE_TEMP) {
    int8_t s = (tempStep != 0) ? tempStep : airStep;

    if (s != 0) {
      autotuneSelectedTempC += s * TEMP_STEP_C;
      autotuneSelectedTempC =
        clampValue(autotuneSelectedTempC, 150, TEMP_MAX_C);
    }

    if (tempEvt == BTN_SHORT) {
      startAutotuneFromMenu();
    }

    if (airEvt == BTN_SHORT) {
      uiMode = UI_AUTOTUNE_MODE;
    }

    return;
  }

  // ------------------------------------------------------------
  // AUTO 3 PUNTOS: cooldown entre mediciones.
  // AIR/BACK cancela los puntos restantes, pero conserva cooldown.
  // ------------------------------------------------------------
  if (uiMode == UI_AUTOTUNE_COOLDOWN) {
    if (airEvt == BTN_SHORT || airEvt == BTN_LONG) {
      autotuneNextStagePending = false;
      uiMode = UI_SETTINGS;
      beginCooldown();
      showMessage("AUTO3 ABORTADO");
    }

    return;
  }

  // ------------------------------------------------------------
  // SAVE PRESET
  // ------------------------------------------------------------
  if (uiMode == UI_SAVE_PRESET) {
    if (tempStep != 0 || airStep != 0) {
      int s = (tempStep != 0) ? tempStep : airStep;
      presetIndex += s;
      if (presetIndex < 0) presetIndex = 2;
      if (presetIndex > 2) presetIndex = 0;
    }

    if (tempEvt == BTN_SHORT) {
      saveSelectedPreset();
    }

    if (airEvt == BTN_SHORT) {
      uiMode = UI_SETTINGS;
    }

    return;
  }

  // ------------------------------------------------------------
  // AIR CAL 20
  // Ajusta delay en pasos de 100 us y hace funcionar la bomba al 20 %.
  // ------------------------------------------------------------
  if (uiMode == UI_AIR_CAL20) {
    if (tempStep != 0 || airStep != 0) {
      int s = (tempStep != 0) ? tempStep : airStep;
      long d = (long)airCalTempDelay + (long)s * 100L;

      d = clampValue<long>(d, 1000L, 9000L);
      if (d <= cfg.airDelay100us + 500) {
        d = cfg.airDelay100us + 500;
      }

      airCalTempDelay = (uint16_t)d;
    }

    // Sobrescribir AIR efectivo y delay mientras se calibra.
    effectiveAirPctISR = 20;
    pumpDelayOverrideUs = airCalTempDelay;

    if (tempEvt == BTN_SHORT) {
      noInterrupts();
      cfg.airDelay20us = airCalTempDelay;
      interrupts();
      saveConfigNow();

      uiMode = UI_SETTINGS;
      showMessage("AIR20 GUARDADO");
    }

    if (airEvt == BTN_SHORT) {
      uiMode = UI_SETTINGS;
    }

    return;
  }

  // ------------------------------------------------------------
  // AIR FULL TEST
  // 100% real = conduccion completa; no necesita calibracion.
  // TEMP/OK o AIR/BACK vuelven al menu.
  // ------------------------------------------------------------
  if (uiMode == UI_AIR_CAL100) {
    pumpDelayOverrideUs = 0;
    effectiveAirPctISR = 100;

    if (tempEvt == BTN_SHORT || airEvt == BTN_SHORT) {
      uiMode = UI_SETTINGS;
      showMessage("AIR FULL OK");
    }

    return;
  }
}

// ================================================================
// LCD
// ================================================================
void formatFloat1(float value, char* out, uint8_t width) {
  dtostrf(value, width, 2, out);
}

void updateLCD() {
  unsigned long now = millis();

  if (now - lastLCD < LCD_REFRESH_MS) return;
  lastLCD = now;

  char l1[17];
  char l2[17];

  // Mensaje transitorio conserva la primera linea normal
  // y reemplaza brevemente la segunda.
  bool transient = now < transientUntil;

  // ------------------------------------------------------------
  // FAULT
  // ------------------------------------------------------------
  if (uiMode == UI_FAULT) {
    printLine(0, faultText(faultCode));

    if (tcValid) {
      snprintf(l2, sizeof(l2), "T:%3d COOL68", (int)tempC);
    } else {
      snprintf(l2, sizeof(l2), "AIR100 TC ERR");
    }

    printLine(1, l2);
    return;
  }

  // ------------------------------------------------------------
  // AUTOTUNE
  // ------------------------------------------------------------
  if (uiMode == UI_AUTOTUNE_RUNNING) {
    snprintf(
      l1,
      sizeof(l1),
      "T:%3d AT:%3d",
      tcValid ? (int)tempC : -1,
      (int)autotuneTargetC
    );

    if (autotuneMode == AT_THREE_POINT) {
      snprintf(
        l2,
        sizeof(l2),
        "3P%d/3 A%3d C%d",
        autotuneStage + 1,
        autotuneAirPct,
        tuneHighStats.count
      );
    } else {
      snprintf(
        l2,
        sizeof(l2),
        "ACT A%3d C%d/%d",
        autotuneAirPct,
        tuneHighStats.count,
        getAutotuneCycleTarget()
      );
    }

    printLine(0, l1);
    printLine(1, l2);
    return;
  }

  if (uiMode == UI_AUTOTUNE_COOLDOWN) {
    int nextAir = AUTOTUNE_AIR_POINTS[autotuneStage];

    snprintf(l1, sizeof(l1), "PID3 COOL <68C");
    snprintf(
      l2,
      sizeof(l2),
      "T:%3d NEXT:%3d%%",
      tcValid ? (int)tempC : -1,
      nextAir
    );

    printLine(0, l1);
    printLine(1, l2);
    return;
  }

  // ------------------------------------------------------------
  // PRESETS
  // ------------------------------------------------------------
  if (uiMode == UI_PRESETS) {
    PresetData& p = cfg.preset[presetIndex];

    snprintf(
      l1,
      sizeof(l1),
      "HEATER %d",
      presetIndex + 1
    );

    snprintf(
      l2,
      sizeof(l2),
      "%3dC AIR:%3d%%",
      p.tempC,
      p.airPct
    );

    printLine(0, l1);
    printLine(1, l2);
    return;
  }

  // ------------------------------------------------------------
  // SETTINGS
  // ------------------------------------------------------------
  if (uiMode == UI_SETTINGS) {
    snprintf(l1, sizeof(l1), "MENU %d/%d", menuIndex + 1, SETTINGS_COUNT);
    strncpy(l2, settingsLabel(menuIndex), 16);
    l2[16] = '\0';

    printLine(0, l1);
    printLine(1, l2);
    return;
  }

  // ------------------------------------------------------------
  // PID VALUES
  // ------------------------------------------------------------
  if (uiMode == UI_PID_VALUES) {
    if (cooldownActive) {
      printLine(
        0,
        (cfg.pidMode == 1 && pidTableValid()) ? "PID 3P GUARDADO" : "PID ACTUAL OK"
      );

      snprintf(
        l2,
        sizeof(l2),
        "COOL T:%3d ->68",
        tcValid ? (int)tempC : -1
      );

      printLine(1, l2);
      return;
    }

    char val[12];
    float kpNow, kiNow, kdNow;
    getActivePid(kpNow, kiNow, kdNow);

    if (menuIndex == 0) {
      formatFloat1(kpNow, val, 6);
      snprintf(l1, sizeof(l1), "Kp %s", val);
    } else if (menuIndex == 1) {
      formatFloat1(kiNow, val, 6);
      snprintf(l1, sizeof(l1), "Ki %s", val);
    } else {
      formatFloat1(kdNow, val, 6);
      snprintf(l1, sizeof(l1), "Kd %s", val);
    }

    snprintf(
      l2,
      sizeof(l2),
      "%s A:%3d%%",
      (cfg.pidMode == 1 && pidTableValid()) ? "TABLE" : "SINGLE",
      airSetPct
    );

    printLine(0, l1);
    printLine(1, l2);
    return;
  }

  // ------------------------------------------------------------
  // PID AUTOTUNE: modo
  // ------------------------------------------------------------
  if (uiMode == UI_AUTOTUNE_MODE) {
    printLine(0, "PID AUTOTUNE");
    printLine(
      1,
      autotuneMenuChoice == 0 ? "> ACTUAL" : "> AUTO 3 PUNTOS"
    );
    return;
  }

  // ------------------------------------------------------------
  // PID AUTOTUNE: temperatura
  // ------------------------------------------------------------
  if (uiMode == UI_AUTOTUNE_TEMP) {
    snprintf(l1, sizeof(l1), "PID TEMP:%3dC", autotuneSelectedTempC);
    printLine(0, l1);
    printLine(1, "OK=START AIR=BACK");
    return;
  }

  // ------------------------------------------------------------
  // SAVE PRESET
  // ------------------------------------------------------------
  if (uiMode == UI_SAVE_PRESET) {
    snprintf(l1, sizeof(l1), "GUARDAR P%d?", presetIndex + 1);
    snprintf(l2, sizeof(l2), "%3dC %3d%%", setPointC, airSetPct);

    printLine(0, l1);
    printLine(1, l2);
    return;
  }

  // ------------------------------------------------------------
  // AIR CAL
  // ------------------------------------------------------------
  if (uiMode == UI_AIR_CAL20) {
    snprintf(l1, sizeof(l1), "AIR CAL 20%%");
    snprintf(l2, sizeof(l2), "DELAY:%4uus", airCalTempDelay);

    printLine(0, l1);
    printLine(1, l2);
    return;
  }

  if (uiMode == UI_AIR_CAL100) {
    printLine(0, "AIR FULL TEST");
    printLine(1, "100% FULL POWER");
    return;
  }

  // ------------------------------------------------------------
  // NORMAL
  // ------------------------------------------------------------
  if (tcValid) {
    snprintf(
      l1,
      sizeof(l1),
      "T:%3d/%3d A:%3d",
      (int)tempC,
      setPointC,
      airSetPct
    );
  } else {
    snprintf(
      l1,
      sizeof(l1),
      "T:ERR/%3d A:%3d",
      setPointC,
      airSetPct
    );
  }

  if (transient) {
    strncpy(l2, transientMessage, 16);
    l2[16] = '\0';

  } else if (standPressed) {
    if (tcValid && tempC >= COOLDOWN_TEMP_C) {
      snprintf(l2, sizeof(l2), "STAND COOL %3dC", (int)tempC);
    } else {
      snprintf(l2, sizeof(l2), "STAND READY");
    }

  } else if (heaterCanRun()) {
    int duty = (int)heaterDutyPct;

    if (abs(setPointC - (int)tempC) <= 10) {
      snprintf(l2, sizeof(l2), "PID H:%3d%% RUN", duty);
    } else {
      snprintf(l2, sizeof(l2), "HEAT H:%3d%%", duty);
    }

  } else if (tcValid && tempC >= COOLDOWN_TEMP_C) {
    snprintf(l2, sizeof(l2), "COOLING AIR100");

  } else {
    snprintf(
      l2,
      sizeof(l2),
      "READY P:%s",
      effectiveAirPctISR > 0 ? "ON" : "OFF"
    );
  }

  printLine(0, l1);
  printLine(1, l2);
}

// ================================================================
// SERIAL DIAGNOSTICO
// ================================================================
void updateSerial() {
  unsigned long now = millis();

  if (now - lastSerial < SERIAL_REFRESH_MS) return;
  lastSerial = now;

  Serial.print(F("T="));
  if (tcValid) Serial.print(tempC, 1);
  else Serial.print(F("ERR"));

  Serial.print(F(" SP="));
  Serial.print(setPointC);

  Serial.print(F(" AIRset="));
  Serial.print(airSetPct);

  Serial.print(F(" AIReff="));
  Serial.print((int)effectiveAirPctISR);

  Serial.print(F(" Hreq="));
  Serial.print(heatRequested ? 1 : 0);

  Serial.print(F(" Hduty="));
  Serial.print((int)heaterDutyPct);

  Serial.print(F(" PID="));
  Serial.print(pidOutput, 1);

  Serial.print(F(" Stand="));
  Serial.print(standPressed ? 1 : 0);

  Serial.print(F(" ZC="));
  noInterrupts();
  unsigned long z = zcCount;
  uint16_t hc = measuredHalfCycleUs;
  interrupts();

  Serial.print(z);
  Serial.print(F(" HalfUs="));
  Serial.print(hc);

  Serial.print(F(" Fault="));
  Serial.println((int)faultCode);
}

// ================================================================
// SETUP
// ================================================================
void setup() {
  // Salidas en estado seguro ANTES de cualquier otra cosa.
  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_HEATER, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_SPEAKER, OUTPUT);

  digitalWrite(PIN_PUMP, LOW);
  digitalWrite(PIN_HEATER, LOW);
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_SPEAKER, LOW);

  pinMode(PIN_ZC, INPUT);

  // Pull-up externo 10k a +5V.
  pinMode(PIN_STAND, INPUT);

  pinMode(PIN_TEMP_CLK, INPUT_PULLUP);
  pinMode(PIN_TEMP_DT, INPUT_PULLUP);
  pinMode(PIN_TEMP_SW, INPUT_PULLUP);

  pinMode(PIN_AIR_CLK, INPUT_PULLUP);
  pinMode(PIN_AIR_DT, INPUT_PULLUP);
  pinMode(PIN_AIR_SW, INPUT_PULLUP);

  Serial.begin(115200);

  loadConfig();

  // LCD.
  int lcdStatus = lcd.begin(16, 2);
  lcd.backlight();

  if (lcdStatus) {
    // Potencia siempre segura aunque LCD falle.
    forceHeaterOffPhysical();
  }

  printLine(0, "HONY 850 PRO");
  printLine(1, "FIRMWARE V1.0");

  // Encoders.
  lastTempCLK = digitalRead(PIN_TEMP_CLK);
  lastAirCLK = digitalRead(PIN_AIR_CLK);

  // Stand.
  standPressed = (digitalRead(PIN_STAND) == HIGH);
  lastStandPressed = standPressed;

  // Timer bomba.
  setupTimer1();

  // Zero cross.
  attachInterrupt(
    digitalPinToInterrupt(PIN_ZC),
    zeroCrossISR,
    RISING
  );

  delay(1000);

  // Primera lectura MAX6675.
  tempC = thermocouple.readCelsius();
  tcValid = !isnan(tempC) && tempC >= -10.0f && tempC <= 550.0f;

  previousTempC = tempC;

  resetPID();

  beep(2000, 80);

  showMessage("READY", 900);
}

// ================================================================
// LOOP
// ================================================================
void loop() {
  serviceTemperature();

  serviceStand();

  serviceUiInputs();

  serviceAutotune();

  // Cooldown forzado: al bajar de 80 C apaga automaticamente la bomba.
  // En AUTO 3 PUNTOS tambien lanza el siguiente punto cuando corresponde.
  serviceCooldown();

  // Actualizar PRIMERO el aire efectivo. Asi, al pedir heater,
  // la bomba ya queda programada antes de habilitar potencia termica.
  // Los modos AIR CAL escriben la variable directamente.
  if (uiMode != UI_AIR_CAL20 && uiMode != UI_AIR_CAL100) {
    pumpDelayOverrideUs = 0;
    effectiveAirPctISR = computeEffectiveAirPct();
  }

  // Safety se evalua antes del PID.
  serviceThermalSafety();

  // Si safety no genero fault, PID puede actualizar potencia.
  if (faultCode == FAULT_NONE) {
    servicePID();
  }

  // Si el heater no puede funcionar, relay fisico abierto,
  // salvo que el autotune este ejecutandose.
  if (uiMode != UI_AUTOTUNE_RUNNING && !heaterCanRun()) {
    heaterPowerAllowed = false;
    heaterDutyPct = 0;
    digitalWrite(PIN_HEATER, LOW);
    digitalWrite(PIN_RELAY, LOW);
  }

  serviceDeferredEEPROM();

  updateLCD();
  updateSerial();
}
