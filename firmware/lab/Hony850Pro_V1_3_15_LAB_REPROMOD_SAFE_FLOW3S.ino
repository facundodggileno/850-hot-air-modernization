/*
  ================================================================
  HONY 850 PRO V1.3.15 LAB SAFE LATCH / REPROMOD / WDT500
  Arduino Nano ATmega328P
  Basado en V1.2.6 YIHUA SAFE COOL68
  ================================================================

  OBJETIVO DE ESTA VERSION (DIAGNOSTICO ANTES DEL FIRMWARE FINAL):
  - AUTO-TEST al arrancar: nunca habilita calefactor durante el test.
  - Comprueba termocupla, cruces por cero, compuertas Timer1 y
    12288 transiciones sinteticas con la misma logica de interlocks real.
  - Ejecuta secuencia de bomba 100 -> 40 -> 100 %, SOLO en frio (<60C).
  - Revisa 100Hz, pulsos de triac, logic interlocks y transiciones conocidas.
  - AIr20..100 permitido en NORMAL y AUTOTUNE, no se bloquea AIR40.
  - NO energiza heater si no termino el test y no se solicita manualmente.
  - Exige >256 B libres de SRAM y <35 pulsos falsos ZC por segundo.
  - Prueba de triac de bomba sin heater; NO verifica caudal fisico.
  - Si algun test falla, calefactor bloqueado hasta reiniciar/reparar.
  - AIR 20..100: modo normal y autotune permitidos tras el autotest;
    AIR 0..19 solo para bomba (no garantiza caudal minimo del heater).
  - Cambio AIR20..100: mantiene HEAT REQUEST y retorna automaticamente
    tras 3 s de aire estabilizandose, incluso AIR40; no aparece HEAT WAIT Axx.
  - RAW siempre visible y usado en protecciones; PID usa mediana de 3 lecturas.
  - Al reanudar caliente 3s de rampa con max 25% de duty; en aire parcial,
    cap proporcional al ajuste AIR (AIR40: duty max 40%). No mide caudal real.
  - Log por Serial de TODOS los motivos que cancelan HEAT REQUEST.
  - Sensor del stand con 35ms de confirmacion: corte electrico inmediato
    en nivel activo, pero NO cancela solicitud por un pulso corto falso.
  - Supervision PERIODICA de gates Timer1 mientras heater trabaja con AIR<100.
  - Si falta un gate o los contadores divergen: corte de calor + bomba 100%.
  - ESTE TEST NO TIENE SENSOR DE FLUJO. Gate OK no prueba caudal real.
  - TC MAX6675 alimentada correctamente a 5V; 3 lecturas validas minimas.
  - No declara FAULT_TC durante las tres conversiones iniciales.
  - Antes del test de bomba exige RAW menor a 58 C por 5 segundos
    (evita disparar por una sola lectura espuria en cooldown).
  - Guarda en SRAM .noinit un guardia anti-bucle para reinicios repetidos
    durante el boot test (no depende exclusivamente de MCUSR del bootloader).
  - El programa NO puede resolver dos fuentes de 5V en paralelo:
    para observar LCD y auto-test, usar fuente INTERNA sin USB conectado.
  - Watchdog HARDWARE AVR de 500ms: reinicia TODO el Nano si loop cuelga.
  - Heartbeat en ZC: corta heater+rele si loop supera 300ms (si ISR corre).
  - LCD re-inicializable por comando L solo en frio/heater apagado.
  - El arranque tras cualquier reset siempre exige encendido MANUAL del heater.
  - TEMP reanuda calor durante cooldown MANUAL con AIR 20..100,
    TC/ZC validos, mango levantado y sin FAULT.
    Mantiene heater OFF mientras verifica preflujo y disparos.
  - No permite rearmar en autotune, pantalla de resultados, fault o boot.
  - NUNCA reanuda automaticamente tras colocar o levantar el mango.
  - Temperatura mostrada: salida estimada MAX6675 * 0.68 (experimental).
  - Protecciones siempre sobre temperatura RAW; nunca sobre 0.68.
  - SAFE inspirado conceptualmente en Repromod PCServiceMOD v5.8:
    memoria termica con decaimiento, RAW intacta, no usar lectura falsa baja.
  - SAFE inspirado en Repromod PCServiceMOD 6001: corte fisico prioritario,
    codigo de causa enclavado y foto del contexto al disparar.
  - Perdida de adquisicion MAX6675 >750ms mientras HEAT solicitado => FAULT.
  - Emergencia: pulsar simultaneamente TEMP + AIR por >=120ms; heater
    se corta al instante, queda FAULT enclavado con bomba 100%.
  - Rearme de FAULT: TEMP presionado LARGO y frio <=68C, con TC/ZC
    validos y mango levantado; nunca rearma solo ni con pulsacion corta.
  - Descensos RAW anormales durante calor: pausa temporal de seguridad
    y PID sin pico de potencia. Ascenso RAW extremo => FAULT enclavado.
  - Estas medidas NO sustituyen proteccion termica ni medida de caudal.

  - PID provisional 1.994 / 0.168 / 1.707; frenado preventivo.
  - Autotune TEMPORALMENTE DESHABILITADO: priorizar Flash/SRAM en el Nano.
  - Conserva PID calibrado, modo OUT/RAW, pruebas de bomba, reflow, WDT.
  - Diagnostico Serial CSV compacto cada 500ms + PASS/FAIL y causa.
  - NO puede detectar caudal real, triac soldado, rele soldado, falla del
    MAX que devuelve valores plausibles, ni todos los fallos de hardware.
  - Para seguridad real hacen falta fusible termico/interlock de caudal.



  HARDWARE:
  D2  <- H11AA1 zero-cross
  D3  -> MOC3023 bomba / BTA12
  D4  -> MOC3083 heater / BTA12-600B
  D5  -> BC337 -> rele Omron
  D6  -> buzzer
  D7  <- stand NC, pull-up externo
  D8/D9/A3 <- encoder TEMP
  A0/A1/A2 <- encoder AIR
  D10/D12/D13 <- MAX6675
  A4/A5 -> LCD 16x2 I2C hd44780_I2Cexp
  ================================================================
*/

#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <max6675.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define PSTR(x) x
#define snprintf_P snprintf
#define strcpy_P strcpy
#endif

// ---------------- PINOUT ----------------
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

// ---------------- LIMITES ----------------
const int TEMP_MIN_C = 80;
const int TEMP_MAX_C = 300;  // °C estimados de salida, RAW interno ~441 C max.
const int TEMP_STEP_C = 5;
const int AIR_MIN_PCT = 0;
const int AIR_MAX_PCT = 100;
const int AIR_STEP_PCT = 5;
// AIR 0 sigue disponible para apagar bomba cuando este frio.
// La calefaccion necesita flujo minimo; base original de la estacion: 20 %.
// ATENCION: no se mide caudal REAL (solo disparos TRIAC), ni se garantiza
// seguridad con bomba mecanicamente bloqueada. Validar fisicamente.
const int AIR_MIN_HEATER_PCT = 20;

const float COOLDOWN_TEMP_C = 80.0f;
const float COOLDOWN_STOP_C = 68.0f;
const float ABS_OVER_TEMP_C = 485.0f; // Corte mas temprano (RAW), NO garantiza temp del bobinado.
const float RUNAWAY_DELTA_C = 40.0f;
const unsigned long RUNAWAY_CONFIRM_MS = 750UL;

const unsigned long NO_RISE_WINDOW_MS = 8000UL; // detectar pronto lectura congelada con potencia alta
const float NO_RISE_MIN_DELTA_C = 3.0f;
const float NO_RISE_MIN_DUTY = 25.0f;
const float NO_RISE_MIN_ERROR_C = 20.0f;

const float STABLE_BAND_C = 10.0f;
const unsigned long STABLE_REQUIRED_MS = 10000UL;
const float CONTROL_LOST_ERROR_C = 40.0f;
const unsigned long CONTROL_LOST_MS = 10000UL;

// Frenado al aproximarse al objetivo. Se aplica solo si la T esta subiendo.
// Punto de partida conservador; medir overshoot antes de modificar.
const float BRAKE_LOOKAHEAD_S = 2.0f;
const float BRAKE_MIN_RATE_CPS = 0.8f;
const float BRAKE_ZONE_C = 90.0f;

// ---------------- TIEMPOS ----------------
const unsigned long BUTTON_LONG_MS = 1200UL;
const unsigned long BUTTON_DEBOUNCE_MS = 35UL;
const unsigned long STAND_DEBOUNCE_MS = 35UL;
const unsigned long TEMP_SAMPLE_MS = 250UL;
const unsigned long PID_SAMPLE_MS = 250UL;
const unsigned long LCD_REFRESH_MS = 150UL;
const unsigned long SERIAL_REFRESH_MS = 500UL;
// Watchdog HW AVR: reset ~500ms si el loop se congela.
// Heartbeat en interrupcion de zero-cross: corta heater/rele si loop >300ms.
// No sustituye un termofusible ni un sensor real de flujo.
const unsigned long HEARTBEAT_TIMEOUT_US = 300000UL;
const unsigned long ZC_STALE_TIMEOUT_US = 150000UL;

// ---------------- RED / BOMBA ----------------
const uint16_t HALF_CYCLE_DEFAULT_US = 10000;
const unsigned long ZC_GLITCH_REJECT_US = 7000UL;
const uint16_t PUMP_GATE_PULSE_US = 180;
const uint16_t PUMP_DELAY_MIN_US = 200;
const uint16_t PUMP_DELAY_MAX_US = 9000;

// Valores de la V1.2.6 base. AIR 100 % usa conduccion continua y no depende de estos.
const uint16_t AIR_DELAY_20_US = 7600;
const uint16_t AIR_DELAY_95_US = 3000;


// RESET FLAGS: leer MCUSR lo antes posible y desactivar watchdog heredado.
// Requiere bootloader compatible con watchdog (Optiboot recomendado).
uint8_t bootResetFlags __attribute__((section(".noinit")));
void earlyResetCapture(void) __attribute__((naked, used, section(".init3")));
void earlyResetCapture(void) {
  bootResetFlags = MCUSR;
  MCUSR = 0;
  wdt_disable();
}

// ---------------- OBJETOS ----------------
hd44780_I2Cexp lcd;
MAX6675 thermocouple(PIN_MAX_SCK, PIN_MAX_CS, PIN_MAX_SO);

template<typename T>
T clampValue(T v, T lo, T hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ---------------- ESTADOS ----------------
enum FaultCode : uint8_t {
  FAULT_NONE = 0,
  FAULT_TC,
  FAULT_OVERTEMP,
  FAULT_RUNAWAY,
  FAULT_NO_RISE,
  FAULT_CONTROL_LOST,
  FAULT_WATCHDOG,
  FAULT_ZEROCROSS,
  FAULT_STALL,
  FAULT_LCD,
  FAULT_SELFTEST,
  FAULT_PUMP_GATE,
  FAULT_RESET_LOOP,
  FAULT_ESTOP,
  FAULT_TC_SPIKE
};

enum UiMode : uint8_t {
  UI_NORMAL = 0,
  UI_FAULT = 4
};

uint8_t faultCode = FAULT_NONE;
uint8_t uiMode = UI_NORMAL;
// Snapshot diagnostico, capturado ANTES del corte y registrado DESPUES.
float safeTripRaw = NAN;
uint8_t safeTripDuty = 0, safeTripAir = 100, safeTripArm = 0;
unsigned long safeTripAtMs = 0;
bool emergencyButtonsHeld = false;
unsigned long safeSensorHoldUntilMs = 0;
uint16_t safeSensorDropCount = 0;

// ---------------- PRINCIPALES ----------------
// Curva empirica confirmada con MAX6675 alimentado: OUT ~= RAW*0.68.
// Modo salida estimada por defecto; RAW accesible solo en frio/heater OFF.
const float OUTLET_SCALE = 0.68f;
bool outletCalMode = true;
int setPointC = 200;
int internalSetpointC() {
  int raw = outletCalMode ? (int)((float)setPointC / OUTLET_SCALE + 0.5f) : setPointC;
  return clampValue(raw, TEMP_MIN_C, 445);
}
int airSetPct = 100;
int lastNonZeroAirPct = 100;

bool heatRequested = false;
bool pumpManualEnabled = true;
bool cooldownActive = false;
bool cooldownCompletedLatch = false;
bool faultCoolingActive = false;
bool standPressed = true; // bloqueo inmediato: entrada en nivel activo O estado confirmado
bool standRawPressed = true;
bool standStablePressed = true;
unsigned long standRawChangeMs = 0;

float tempC = NAN;
float previousTempC = NAN;
float tempRateCps = 0.0f;
float filteredRateCps = 0.0f;
uint8_t brakeDutyCap = 100;
bool tcValid = false;
uint8_t consecutiveValidTcReads = 0;
unsigned long lastValidTcMs = 0;

volatile uint8_t heaterDutyPct = 0;
volatile bool heaterPowerAllowed = false;
volatile uint8_t effectiveAirPctISR = 100; // startup: bomba en marcha hasta leer TC
volatile unsigned long lastControlHeartbeatUs = 0;
volatile bool isrHeartbeatTrip = false;

// ---------------- AUTO-DIAGNOSTICO DEL ARRANQUE ----------------
// Mantiene heater y rele abiertos hasta completar todos los estados.
enum BootStage : uint8_t {
  BT_WAIT_TC = 0, BT_WAIT_COOL, BT_SOFT_STRESS, BT_WAIT_ZC,
  BT_PUMP_FULL, BT_PUMP_PHASE, BT_PUMP_RECOVER, BT_DONE, BT_FAILED
};
uint8_t bootStage = BT_WAIT_TC;
bool selftestPassed = false;
unsigned long bootStageSince = 0;
unsigned long bootZcStartCount = 0;
unsigned long bootZcStartRejected = 0;
unsigned long bootGateStartCount = 0;
unsigned long bootGateEndCount = 0;
uint8_t diagnosticPumpAir = 100;
uint16_t stressFailures = 0;
uint16_t stressCount = 0;
uint32_t stressSeed = 0x850139UL;
const uint16_t STRESS_CASES = 12288U;  // 3 pasadas de 4096 combinaciones.
const uint8_t STRESS_BATCH = 48;
const unsigned long BOOT_TC_TIMEOUT_MS = 12000UL;
const unsigned long BOOT_COOL_STABLE_MS = 5000UL;
const float BOOT_COOL_BELOW_C = 58.0f;  // margen de 2C bajo test de bomba (<60)
unsigned long bootColdStableSince = 0;
// Data RAM .noinit: puede sobrevivir a RESET por USB o watchdog; NO al apagado largo.
// A diferencia de MCUSR, esta guarda puede detectar reinicios aun si Optiboot
// limpia los reset flags antes de ejecutar nuestro sketch.
struct BootGuardRaw { uint16_t marker, inverse; uint8_t testing, resets; };
BootGuardRaw bootGuard __attribute__((section(".noinit")));
const uint16_t BOOT_GUARD_MAGIC = 0x8539U;
const uint8_t BOOT_RESET_LIMIT = 3;
volatile unsigned long pumpGateStartCount = 0;
volatile unsigned long pumpGateEndCount = 0;
volatile unsigned long pumpPhaseSchedules = 0;
bool lcdReady = false;
unsigned long heatArmStartMs = 0;
// Armado NO bloqueante. El heater queda OFF en ambas etapas previas.
// Se exige observar los pulsos de gate antes de permitir calor con AIR<100.
// Al modificar AIR, 1 s preflujo total, 1 s fase a caudal pedido,
// 1 s adicional de estabilizacion. No se energiza heater antes de 3 s.
const unsigned long AIR_CHANGE_HOLD_MS = 3000UL;
const unsigned long PRE_FLOW_MS = 1000UL;
const unsigned long PUMP_GATE_VERIFY_MS = 1000UL;
const unsigned long PUMP_GATE_MONITOR_MS = 500UL;
const unsigned long SAFE_TC_STALE_MS = 750UL;
const unsigned long SAFE_BUTTONS_EMERGENCY_MS = 120UL;
const float SAFE_TC_DROP_GUARD_C = 22.0f;
const float SAFE_TC_RISE_SPIKE_C = 42.0f;
// Referencia TERMICA: memoria superior transitoria con descenso gradual.
// Equivalente conceptual al no reaprender bajadas transitorias del Repromod.
// No es RAW, no participa de las alarmas absolutas.
const float SAFE_THERMAL_REF_DECAY_CPS = 10.0f;
const unsigned long SAFE_TC_DROP_PAUSE_MS = 750UL;
const unsigned long REARM_RAMP_MS = 3000UL;
const uint8_t ARM_IDLE = 0, ARM_FULL_FLOW = 1, ARM_VERIFY_PHASE = 2, ARM_READY = 3, ARM_SETTLE = 4;
uint8_t armStage = ARM_IDLE;
uint8_t armedAirPct = 100;
unsigned long armCycleStartMs = 0;  // nunca se reinicia antes de READY
unsigned long armReadySinceMs = 0;
bool rearmHot = false;
float controlTempC = NAN;          // mediana de 3 muestras para PID, NO para safety
float previousSample1 = NAN;
float previousSample2 = NAN;
float safeThermalReferenceC = NAN;
unsigned long safeThermalRefMs = 0;
// Reutilizamos bootGateStartCount/bootGateEndCount (libres tras BOOT PASS)
// como referencias durante la verificacion y vigilancia de fase.



// ---------------- ZERO CROSS ----------------
volatile unsigned long zcCount = 0;
volatile unsigned long zcRejectedCount = 0;
volatile unsigned long lastZcMicros = 0;
volatile uint16_t measuredHalfCycleUs = HALF_CYCLE_DEFAULT_US;
volatile bool heaterFullCycleOn = false;
volatile bool heaterSecondHalf = false;
volatile uint16_t heaterBurstAccumulator = 0;

// ---------------- PID ----------------
// PID validado por usuario: autotune 100%/0% a 300 C y AIR 100%.
// Empirico; solo en RAM, sin EEPROM ni presets.
const float pidKp = 1.994f;
const float pidKi = 0.168f;
const float pidKd = 1.707f;

float pidIntegral = 0.0f;
float lastPidMeasurement = NAN;
unsigned long lastPidMs = 0;
float pidOutput = 0.0f;

unsigned long stableBandSince = 0;
bool regulationWasStable = false;
unsigned long controlLostSince = 0;
unsigned long noRiseSince = 0;
float noRiseStartTemp = 0.0f;
unsigned long runawaySince = 0;
bool runawayArmed = false;
unsigned long controlGraceUntil = 0;

// ---------------- UI ----------------
unsigned long lastLCD = 0;
unsigned long lastSerial = 0;
int lastTempCLK = HIGH;
int lastAirCLK = HIGH;

const uint8_t BTN_TEMP_IDX = 0;
const uint8_t BTN_AIR_IDX = 1;
const uint8_t buttonPin[2] = { PIN_TEMP_SW, PIN_AIR_SW };
bool buttonStableState[2] = { HIGH, HIGH };
bool buttonRawState[2] = { HIGH, HIGH };
unsigned long buttonLastRawChange[2] = { 0, 0 };
unsigned long buttonPressedSince[2] = { 0, 0 };
bool buttonLongFired[2] = { false, false };

enum ButtonEvent : uint8_t {
  BTN_NONE = 0,
  BTN_SHORT,
  BTN_LONG
};

// ---------------- PROTOTIPOS EXPLICITOS ----------------
// Evitan que el preprocesador de Arduino genere prototipos antes de
// Todos los prototipos usan solo tipos primitivos compatibles con el preprocesador Arduino.
uint8_t updateButton(uint8_t idx);
void setFault(uint8_t code);
void forceHeaterOffPhysical();
void serviceEmergencyButtons();
void serviceBootTest();
bool zeroCrossHealthy();
void armHeatFlow(uint8_t desiredAir);
void serviceHeatArm();
void markSelftestFailure(uint8_t code, const char* detail);
bool heatPermitModel(bool testsOK, bool sensorOK, bool tcFresh,
                     bool stand, bool cooling, bool zcOK, bool requested,
                     bool preflowDone, int air, float rawTemp, uint8_t fault);
uint8_t pumpPlanModel(bool sensorOK, bool fault, bool faultCooling,
                      bool tuning, bool result, bool cooling, bool hot,
                      bool heatPending, bool stand, bool manualPump,
                      bool coolDone, uint8_t requestedAir);


// ---------------- RAM DIAG ----------------
extern int __heap_start, *__brkval;
int freeRam() {
  int v;
  return (int)((intptr_t)&v - (__brkval == 0 ? (intptr_t)&__heap_start : (intptr_t)__brkval));
}

// ---------------- UTILS ----------------
void beep(uint16_t freq, uint16_t durationMs) {
  tone(PIN_SPEAKER, freq, durationMs);
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

uint8_t updateButton(uint8_t idx) {
  bool raw = digitalRead(buttonPin[idx]);
  unsigned long now = millis();

  if (raw != buttonRawState[idx]) {
    buttonRawState[idx] = raw;
    buttonLastRawChange[idx] = now;
  }

  if (now - buttonLastRawChange[idx] < BUTTON_DEBOUNCE_MS) return BTN_NONE;

  if (buttonStableState[idx] != buttonRawState[idx]) {
    buttonStableState[idx] = buttonRawState[idx];

    if (buttonStableState[idx] == LOW) {
      buttonPressedSince[idx] = now;
      buttonLongFired[idx] = false;
    } else {
      if (!buttonLongFired[idx]) return BTN_SHORT;
    }
  }

  if (buttonStableState[idx] == LOW && !buttonLongFired[idx] &&
      now - buttonPressedSince[idx] >= BUTTON_LONG_MS) {
    buttonLongFired[idx] = true;
    return BTN_LONG;
  }

  return BTN_NONE;
}

int8_t readTempEncoderStep() {
  int clk = digitalRead(PIN_TEMP_CLK);
  int8_t step = 0;
  if (clk != lastTempCLK && clk == LOW) {
    step = (digitalRead(PIN_TEMP_DT) != clk) ? 1 : -1;
  }
  lastTempCLK = clk;
  return step;
}

int8_t readAirEncoderStep() {
  int clk = digitalRead(PIN_AIR_CLK);
  int8_t step = 0;
  if (clk != lastAirCLK && clk == LOW) {
    step = (digitalRead(PIN_AIR_DT) != clk) ? 1 : -1;
  }
  lastAirCLK = clk;
  return step;
}

// ---------------- BOMBA / ZERO CROSS ----------------
uint16_t airPercentToDelayUs(uint8_t pct) {
  if (pct == 0) return PUMP_DELAY_MAX_US;

  if (pct < 20) {
    uint32_t d = (uint32_t)AIR_DELAY_20_US + (uint16_t)(20 - pct) * 90U;
    if (d > PUMP_DELAY_MAX_US) d = PUMP_DELAY_MAX_US;
    return (uint16_t)d;
  }

  long posPct = pct - 20;
  if (posPct > 75) posPct = 75;
  long delayUs = AIR_DELAY_20_US + ((long)(AIR_DELAY_95_US - AIR_DELAY_20_US) * posPct) / 75L;
  return (uint16_t)clampValue<long>(delayUs, PUMP_DELAY_MIN_US, PUMP_DELAY_MAX_US);
}

void setupTimer1() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  TCCR1B |= (1 << CS11); // prescaler 8 -> 0.5 us/tick
  TIMSK1 &= ~((1 << OCIE1A) | (1 << OCIE1B));
  sei();
}

ISR(TIMER1_COMPA_vect) {
  if (effectiveAirPctISR > 0) {
    digitalWrite(PIN_PUMP, HIGH);
    OCR1B = (uint16_t)(TCNT1 + (uint16_t)(PUMP_GATE_PULSE_US * 2U));
    // AVR TIFR1 es W1C: usar asignacion, NO |= que borra otras flags.
    TIFR1 = _BV(OCF1B);
    pumpGateStartCount++;
    TIMSK1 |= (1 << OCIE1B);
  }
  TIMSK1 &= ~(1 << OCIE1A);
}

ISR(TIMER1_COMPB_vect) {
  digitalWrite(PIN_PUMP, LOW);
  pumpGateEndCount++;
  TIMSK1 &= ~(1 << OCIE1B);
}

void zeroCrossISR() {
  unsigned long nowUs = micros();

  // Corte de emergencia por interrupcion, INDEPENDIENTE del loop y del I2C.
  // Si la pantalla o Serial se bloquean, no seguimos calentando.
  if (heaterPowerAllowed && lastControlHeartbeatUs != 0 &&
      (unsigned long)(nowUs - lastControlHeartbeatUs) > HEARTBEAT_TIMEOUT_US) {
    heaterPowerAllowed = false;
    heaterDutyPct = 0;
    heaterFullCycleOn = false;
    digitalWrite(PIN_HEATER, LOW);
    digitalWrite(PIN_RELAY, LOW);
    isrHeartbeatTrip = true;  // fallo enclavado al recuperarse el loop
  }

  if (lastZcMicros != 0) {
    unsigned long delta = nowUs - lastZcMicros;
    if (delta < ZC_GLITCH_REJECT_US) {
      zcRejectedCount++;
      return;
    }
    if (delta >= 7500UL && delta <= 11500UL) {
      measuredHalfCycleUs = (uint16_t)((measuredHalfCycleUs * 7UL + delta) / 8UL);
    }
  }

  lastZcMicros = nowUs;
  zcCount++;

  // Heater: el OFF tiene prioridad ABSOLUTA, incluso en segundo semiciclo.
  // Evita que un ciclo anterior vuelva a poner D4 HIGH despues de un fault.
  if (!heaterPowerAllowed || heaterDutyPct == 0) {
    heaterFullCycleOn = false;
    heaterBurstAccumulator = 0;
    heaterSecondHalf = false;
    digitalWrite(PIN_HEATER, LOW);
  } else if (!heaterSecondHalf) {
    heaterBurstAccumulator += heaterDutyPct;
    if (heaterBurstAccumulator >= 100U) {
      heaterBurstAccumulator = (uint16_t)(heaterBurstAccumulator - 100U);
      heaterFullCycleOn = true;
    } else {
      heaterFullCycleOn = false;
    }
    digitalWrite(PIN_HEATER, heaterFullCycleOn ? HIGH : LOW);
    heaterSecondHalf = true;
  } else {
    digitalWrite(PIN_HEATER, heaterFullCycleOn ? HIGH : LOW);
    heaterSecondHalf = false;
  }

  // Bomba.
  uint8_t air = effectiveAirPctISR;
  TIMSK1 &= ~((1 << OCIE1A) | (1 << OCIE1B));
  // Cancela flags de disparos anteriores tras conmutar 40 <-> 100%.
  TIFR1 = _BV(OCF1A) | _BV(OCF1B);

  if (air >= 100) {
    digitalWrite(PIN_PUMP, HIGH);
    return;
  }

  digitalWrite(PIN_PUMP, LOW);

  if (air > 0) {
    uint16_t delayUs = airPercentToDelayUs(air);
    uint16_t maxAllowed = measuredHalfCycleUs > 500 ? measuredHalfCycleUs - 400 : PUMP_DELAY_MAX_US;
    if (delayUs > maxAllowed) delayUs = maxAllowed;
    if (delayUs < PUMP_DELAY_MIN_US) delayUs = PUMP_DELAY_MIN_US;

    TCNT1 = 0;
    OCR1A = (uint16_t)(delayUs * 2U);
    TIFR1 = _BV(OCF1A);
    pumpPhaseSchedules++;
    TIMSK1 |= (1 << OCIE1A);
  }
}

// ---------------- MISMA LOGICA PARA CONTROL Y PRUEBAS ----------------
// Las condiciones de permiso del control real y del test comparten esta
// funcion pura para evitar que el test "pase" con reglas diferentes.
bool heatPermitModel(bool testsOK, bool sensorOK, bool tcFresh,
                     bool stand, bool cooling, bool zcOK, bool requested,
                     bool preflowDone, int air, float rawTemp, uint8_t fault) {
  return testsOK && sensorOK && tcFresh && !stand && !cooling &&
         zcOK && requested && preflowDone && air >= AIR_MIN_HEATER_PCT &&
         rawTemp < ABS_OVER_TEMP_C && fault == FAULT_NONE;
}

uint8_t pumpPlanModel(bool sensorOK, bool fault, bool faultCooling,
                      bool tuning, bool result, bool cooling, bool hot,
                      bool heatPending, bool stand, bool manualPump,
                      bool coolDone, uint8_t requestedAir) {
  if (!sensorOK) return 100;
  if (fault) return faultCooling ? 100 : 0;
  if (tuning || cooling) return 100;
  if (result) return 0;
  if (hot) {
    if (heatPending && !stand) return requestedAir;
    if (coolDone && !heatPending) return 0;
    return 100;
  }
  if (stand) return 0;
  if (heatPending) return requestedAir; // preflujo antes de energizar heater
  return manualPump ? requestedAir : 0;
}

// ---------------- TEMPERATURA ----------------
void serviceTemperature() {
  static unsigned long lastRead = 0;
  unsigned long now = millis();
  if (now - lastRead < TEMP_SAMPLE_MS) return;

  float dt = lastRead == 0 ? 0.25f : (float)(now - lastRead) / 1000.0f;
  lastRead = now;

  previousTempC = tempC;
  const float oldControl = controlTempC;
  float t = thermocouple.readCelsius();
  bool valid = !isnan(t) && t >= 5.0f && t <= 550.0f;

  if (valid) {
    // Deteccion de saltos SOLO con heater solicitado: MAX entrega cada ~220ms.
    // Una bajada aislada puede ser ruido de TC o cambio de flujo; evita
    // que el PID responda con 650W, sin confundirla con un FAULT enclavado.
    if (heatRequested && armStage == ARM_READY && tcValid &&
        !isnan(previousTempC) && dt >= 0.20f && dt <= 0.40f) {
      const float delta = t - previousTempC;
      if (delta < -SAFE_TC_DROP_GUARD_C) {
        safeSensorHoldUntilMs = now + SAFE_TC_DROP_PAUSE_MS;
        ++safeSensorDropCount;
        forceHeaterOffPhysical();
        Serial.print(F("SAFE WARN,TC DROP,RAW=")); Serial.println(t,1);
      } else if (delta > SAFE_TC_RISE_SPIKE_C) {
        // Compatible con ruido grave O calentamiento anormal: no seguir
        // calentando hasta que el usuario revise el problema.
        tempC = t; // snapshot del valor anomalo al disparar SAFE
        setFault(FAULT_TC_SPIKE);
      }
    }
    tempC = t;  // RAW INTACTA para mostrar y detectar sobretemperatura
    // Mantener el valor alto durante un descenso rapido al bajar AIR;
    // permitir que la referencia decaiga 10 C/s, no bloquearla para siempre.
    if (isnan(safeThermalReferenceC) || safeThermalRefMs == 0) {
      safeThermalReferenceC = t;
    } else {
      float refDt = (float)(now - safeThermalRefMs) / 1000.0f;
      if (refDt > 2.0f) refDt = 2.0f;
      safeThermalReferenceC -= SAFE_THERMAL_REF_DECAY_CPS * refDt;
      if (t > safeThermalReferenceC) safeThermalReferenceC = t;
    }
    safeThermalRefMs = now;
    // Mediana de 3 conversiones: una lectura falsa BAJA aislada no debe
    // provocar que el PID pida 100 % de 650W de golpe.
    if (isnan(previousSample1) || isnan(previousSample2)) {
      controlTempC = t;
    } else {
      float lo = previousSample1 < previousSample2 ? previousSample1 : previousSample2;
      float hi = previousSample1 > previousSample2 ? previousSample1 : previousSample2;
      controlTempC = clampValue(t, lo, hi);
    }
    previousSample2 = previousSample1;
    previousSample1 = t;
    lastValidTcMs = now;
    if (consecutiveValidTcReads < 10) consecutiveValidTcReads++;
    tcValid = consecutiveValidTcReads >= 3;
    if (!isnan(oldControl) && dt > 0.05f) tempRateCps = (controlTempC - oldControl) / dt;
    else tempRateCps = 0.0f;
    filteredRateCps = clampValue(0.72f * filteredRateCps + 0.28f * tempRateCps,
                                 -60.0f, 60.0f);
  } else {
    tcValid = false;
    consecutiveValidTcReads = 0;
    filteredRateCps = 0.0f;
    previousSample1 = NAN;
    previousSample2 = NAN;
    controlTempC = NAN;
  }
}

// ---------------- POTENCIA / COOLDOWN ----------------
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
  brakeDutyCap = 100;
  filteredRateCps = 0.0f;
  resetControlSupervision();
}

void forceHeaterOffPhysical() {
  // Apagado atomico de todo el estado compartido con el ISR.
  noInterrupts();
  heaterPowerAllowed = false;
  heaterDutyPct = 0;
  heaterFullCycleOn = false;
  heaterSecondHalf = false;
  heaterBurstAccumulator = 0;
  interrupts();
  digitalWrite(PIN_HEATER, LOW);
  digitalWrite(PIN_RELAY, LOW);
  pidOutput = 0.0f;
  pidIntegral = 0.0f;
  brakeDutyCap = 100;
}

void armHeatFlow(uint8_t desiredAir) {
  // Solamente llamado tras una accion manual (o al cambiar AIR con heat ON).
  // Nunca habilita relé ni MOC heater directamente.
  forceHeaterOffPhysical();
  armedAirPct = desiredAir;
  armStage = ARM_FULL_FLOW;
  armCycleStartMs = millis();
  heatArmStartMs = armCycleStartMs;
  armReadySinceMs = 0;
  rearmHot = tcValid && tempC >= 120.0f;
  noInterrupts();
  effectiveAirPctISR = 100;
  TIMSK1 &= (uint8_t)~(_BV(OCIE1A) | _BV(OCIE1B));
  TIFR1 = _BV(OCF1A) | _BV(OCF1B);
  interrupts();
  digitalWrite(PIN_PUMP, HIGH);
  resetPID();
}

// Testea los pulsos reales del Timer1; NO confirma movimiento fisico de aire.
void serviceHeatArm() {
  if (!heatRequested || faultCode != FAULT_NONE || !selftestPassed) {
    armStage = ARM_IDLE;
    return;
  }
  if (!tcValid || standPressed || !zeroCrossHealthy() ||
      (unsigned long)(millis() - lastValidTcMs) > 500UL ||
      tempC >= ABS_OVER_TEMP_C) return;

  const uint8_t air = (uint8_t)airSetPct;
  if (air < AIR_MIN_HEATER_PCT) return;
  if (armedAirPct != air && armStage != ARM_IDLE) {
    armHeatFlow(air);  // cancela el timer y empieza 3s de nuevo al girar
    Serial.println(F("ARM,RESTART_AFTER_AIR_CHANGE"));
    return;
  }
  const unsigned long now = millis();
  const unsigned long elapsed = (unsigned long)(now - armCycleStartMs);
  if (armStage == ARM_FULL_FLOW && elapsed >= PRE_FLOW_MS) {
    if (air >= 100) {
      armStage = ARM_SETTLE;
      Serial.println(F("ARM,SETTLE_AIR100"));
    } else {
      noInterrupts();
      bootGateStartCount = pumpGateStartCount;
      bootGateEndCount = pumpGateEndCount;
      interrupts();
      heatArmStartMs = now;
      armStage = ARM_VERIFY_PHASE;
      Serial.println(F("ARM,VERIFY_TIMER1_PHASE"));
    }
  } else if (armStage == ARM_VERIFY_PHASE &&
             (unsigned long)(now - heatArmStartMs) >= PUMP_GATE_VERIFY_MS) {
    unsigned long starts, ends;
    noInterrupts();
    starts = pumpGateStartCount - bootGateStartCount;
    ends = pumpGateEndCount - bootGateEndCount;
    interrupts();
    Serial.print(F("ARM,GATE_START_END,")); Serial.print(starts);
    Serial.print(','); Serial.println(ends);
    if (starts < 70UL || ends < 70UL ||
        (starts > ends ? starts - ends : ends - starts) > 4UL) {
      setFault(FAULT_PUMP_GATE);
      return;
    }
    armStage = ARM_SETTLE;
    Serial.println(F("ARM,GATES_OK_STABILIZING"));
  } else if (armStage == ARM_SETTLE && elapsed >= AIR_CHANGE_HOLD_MS) {
    armStage = ARM_READY;
    armReadySinceMs = now;
    heatArmStartMs = now; // monitoreo periodico de bomba arranca de cero
    noInterrupts();
    bootGateStartCount = pumpGateStartCount;
    bootGateEndCount = pumpGateEndCount;
    interrupts();
    Serial.println(F("ARM,AUTO_RESUME_AFTER_3SEC"));
  } else if (armStage == ARM_READY && air < 100 &&
             (unsigned long)(now - heatArmStartMs) >= PUMP_GATE_MONITOR_MS) {
    unsigned long starts, ends;
    noInterrupts();
    starts = pumpGateStartCount - bootGateStartCount;
    ends = pumpGateEndCount - bootGateEndCount;
    bootGateStartCount = pumpGateStartCount;
    bootGateEndCount = pumpGateEndCount;
    interrupts();
    heatArmStartMs = now;
    if (starts < 35UL || ends < 35UL ||
        (starts > ends ? starts - ends : ends - starts) > 4UL) {
      Serial.print(F("FAULT PUMP GATES ACTIVE,")); Serial.print(starts);
      Serial.print(','); Serial.println(ends);
      setFault(FAULT_PUMP_GATE);
    }
  }
}

void beginCooldown() {
  armStage = ARM_IDLE;
  heatRequested = false;   // nunca se reanuda automaticamente desde cooldown.
  forceHeaterOffPhysical();
  cooldownActive = true;
  noInterrupts();
  effectiveAirPctISR = 100;
  TIMSK1 = (uint8_t)(TIMSK1 & (uint8_t)~(_BV(OCIE1A) | _BV(OCIE1B)));
  TIFR1 = _BV(OCF1A) | _BV(OCF1B);
  interrupts();
  digitalWrite(PIN_PUMP, HIGH);
  cooldownCompletedLatch = false;
  pumpManualEnabled = false;
}

bool zeroCrossHealthy() {
  noInterrupts();
  unsigned long last = lastZcMicros;
  interrupts();
  return last != 0 && (unsigned long)(micros() - last) <= ZC_STALE_TIMEOUT_US;
}

void setFault(uint8_t code) {
  if (faultCode != FAULT_NONE) return;
  // PRIORIDAD SAFE: primero enclavar y abrir TODAS las salidas.
  // Nunca imprimir por Serial antes de cortar fisicamente D4 y D5.
  faultCode = code;
  safeTripRaw = tempC;
  safeTripDuty = heaterDutyPct;
  safeTripAir = (uint8_t)airSetPct;
  safeTripArm = armStage;
  safeTripAtMs = millis();
  heatRequested = false;
  armStage = ARM_IDLE;
  forceHeaterOffPhysical();
  faultCoolingActive = true;
  // El airflow a 100% no puede depender del detector zero-cross.
  // Al dejar gate del MOC3023 HIGH, la bomba debe poder conducir
  // incluso si D2 dejo de generar interrupciones (con red presente).
  noInterrupts();
  effectiveAirPctISR = 100;
  TIMSK1 = (uint8_t)(TIMSK1 & (uint8_t)~(_BV(OCIE1A) | _BV(OCIE1B)));
  TIFR1 = _BV(OCF1A) | _BV(OCF1B);
  interrupts();
  digitalWrite(PIN_PUMP, HIGH);
  uiMode = UI_FAULT;
  // La emergencia ya tiene el calefactor cortado ANTES del Serial.
  Serial.print(F("SAFE TRIP,CAUSE=")); Serial.print((int)faultCode);
  Serial.print(F(",MS=")); Serial.print(safeTripAtMs);
  Serial.print(F(",RAW=")); Serial.print(safeTripRaw,1);
  Serial.print(F(",DUTY=")); Serial.print(safeTripDuty);
  Serial.print(F(",AIR=")); Serial.print(safeTripAir);
  Serial.print(F(",ARM=")); Serial.println(safeTripArm);
  beep(350, 700);
}

bool heaterCanRun() {
  const bool fresh = tcValid && consecutiveValidTcReads >= 3 &&
        (unsigned long)(millis() - lastValidTcMs) <= 500UL;
  const bool preflow = armStage == ARM_READY;
  // SAFE sensor: un descenso sospechoso bloquea SOLO la potencia;
  // HEAT REQUEST se conserva y reanuda luego de lecturas nuevas.
  const bool sampleGuardPassed = (int32_t)(millis() - safeSensorHoldUntilMs) >= 0;
  if (emergencyButtonsHeld || !sampleGuardPassed) return false;
  return heatPermitModel(selftestPassed, tcValid, fresh, standPressed,
        cooldownActive, zeroCrossHealthy(), heatRequested, preflow,
        airSetPct, tempC, faultCode);
}

uint8_t computeEffectiveAirPct() {
  // Diagnostico activo: JAMAS calor; pruebas de bomba solamente en frio.
  if (!selftestPassed) {
    if (!tcValid || tempC > 60.0f || bootStage == BT_WAIT_COOL) return 100;
    return diagnosticPumpAir;
  }

  if (faultCode != FAULT_NONE && tcValid && faultCoolingActive &&
      tempC <= COOLDOWN_STOP_C) faultCoolingActive = false;
  // Heater OFF durante preflujo: bomba al 100% en cada reinicio/retoma.
  if (armStage == ARM_FULL_FLOW && heatRequested) return 100;
  return pumpPlanModel(tcValid, faultCode != FAULT_NONE, faultCoolingActive,
      false, false, cooldownActive,
      tcValid && tempC >= COOLDOWN_TEMP_C, heatRequested,
      standPressed, pumpManualEnabled, cooldownCompletedLatch,
      (uint8_t)airSetPct);
}

void serviceCooldown() {
  if (!cooldownActive || !tcValid) return;
  if (tempC <= COOLDOWN_STOP_C) {
    cooldownActive = false;
    cooldownCompletedLatch = true;
    pumpManualEnabled = false;
    beep(2200, 80);
  }
}

// ---------------- AUTOMATIC STARTUP STRESS TEST ----------------
// SOLO prueba la logica y pulsos de control. Nunca energiza heater.
void markSelftestFailure(uint8_t code, const char* detail) {
  if (bootStage == BT_FAILED) return;
  bootStage = BT_FAILED;
  selftestPassed = false;
  diagnosticPumpAir = 100;
  forceHeaterOffPhysical();
  Serial.print(F("BOOT FAIL,"));
#ifdef __AVR__
  Serial.println((const __FlashStringHelper*)detail);
#else
  Serial.println(detail);
#endif
  setFault(code);
}

void bootNextStage(uint8_t stage, const char* label) {
  bootStage = stage;
  bootStageSince = millis();
  if (stage == BT_WAIT_COOL) bootColdStableSince = 0;
  Serial.print(F("BOOT STAGE,"));
#ifdef __AVR__
  Serial.println((const __FlashStringHelper*)label);
#else
  Serial.println(label);
#endif
}

void oneStressCase() {
  // PRNG determinista: permite reproducir la misma secuencia en cada boot.
  stressSeed ^= stressSeed << 13;
  stressSeed ^= stressSeed >> 17;
  stressSeed ^= stressSeed << 5;
  const uint32_t r = stressSeed;
  const bool valid = (r & 1UL) != 0;
  const bool fresh = (r & 2UL) != 0;
  const bool stand = (r & 4UL) != 0;
  const bool cool = (r & 8UL) != 0;
  const bool zc = (r & 16UL) != 0;
  const bool heat = (r & 32UL) != 0;
  const bool preflow = (r & 64UL) != 0;
  const bool fault = (r & 128UL) != 0;
  const int air = (r & 256UL) ? 100 : 40;
  const float temp = (r & 512UL) ? 490.0f : 300.0f;
  const bool expected = valid && fresh && !stand && !cool && zc &&
         heat && preflow && !fault && air >= AIR_MIN_HEATER_PCT &&
         temp < ABS_OVER_TEMP_C;
  const bool actual = heatPermitModel(true,valid,fresh,stand,cool,zc,
                   heat,preflow,air,temp,fault ? FAULT_TC : FAULT_NONE);
  if (actual != expected) stressFailures++;

  // TRANSICION ESPECIFICA DEL BUG: cooldown + AIR40 + SP300 + PLAY.
  const bool hot = (r & 1024UL) != 0;
  const bool manual = (r & 2048UL) != 0;
  const bool complete = (r & 4096UL) != 0;
  const bool tuning = (r & 8192UL) != 0;
  const bool result = (r & 16384UL) != 0;
  const uint8_t pump = pumpPlanModel(valid,fault,hot,tuning,result,cool,                   hot,heat,stand,manual,complete,(uint8_t)air);
  if (!valid && pump != 100) stressFailures++;
  if (valid && !fault && cool && pump != 100) stressFailures++;
  if (pump > 100) stressFailures++;
  const uint16_t delayUs = airPercentToDelayUs((uint8_t)air);
  if (delayUs < PUMP_DELAY_MIN_US || delayUs > PUMP_DELAY_MAX_US) stressFailures++;
  stressCount++;
}

bool fixedTransitionAssertions() {
  // AIR40 DURANTE cooldown => heater OFF y pump 100.
  if (heatPermitModel(true,true,true,false,true,true,true,true,40,440.0f,FAULT_NONE)) return false;
  if (pumpPlanModel(true,false,false,false,false,true,true,true,false,true,false,40) != 100) return false;
  // Al CANCELAR cooldown sin preflujo completo => heater OFF; bomba puede pasar a AIR40.
  if (heatPermitModel(true,true,true,false,false,true,true,false,40,440.0f,FAULT_NONE)) return false;
  if (pumpPlanModel(true,false,false,false,false,false,true,true,false,true,false,40) != 40) return false;
  // Con preflujo + gate OK (equivalente ARM_READY), AIR40 SI debe permitir heater.
  if (!heatPermitModel(true,true,true,false,false,true,true,true,40,440.0f,FAULT_NONE)) return false;
  if (heatPermitModel(true,true,true,false,false,true,true,true,0,440.0f,FAULT_NONE)) return false;
  if (!heatPermitModel(true,true,true,false,false,true,true,true,20,440.0f,FAULT_NONE)) return false;
  if (!heatPermitModel(true,true,true,false,false,true,true,true,100,440.0f,FAULT_NONE)) return false;
  // Condiciones de seguridad NO dependen de AIR.
  if (heatPermitModel(true,false,true,false,false,true,true,true,100,300.0f,FAULT_NONE)) return false;
  if (heatPermitModel(true,true,true,true,false,true,true,true,40,300.0f,FAULT_NONE)) return false;
  if (heatPermitModel(true,true,true,false,false,false,true,true,40,300.0f,FAULT_NONE)) return false;
  if (heatPermitModel(true,true,true,false,false,true,true,true,40,490.0f,FAULT_NONE)) return false;
  return true;
}

void serviceBootTest() {
  if (bootStage == BT_DONE || bootStage == BT_FAILED) return;
  if (faultCode != FAULT_NONE && faultCode != FAULT_WATCHDOG) {
    markSelftestFailure(FAULT_SELFTEST,PSTR("FAULT DURING TEST"));
    return;
  }
  // Principio fail-safe: heater NUNCA se activa durante un autodiagnostico.
  heatRequested = false;
  forceHeaterOffPhysical();

  const unsigned long now = millis();
  if (bootStage == BT_WAIT_TC) {
    diagnosticPumpAir = 100;
    // Tres lecturas MAX6675 validas. NO hay FAULT_TC durante esa ventana;
    // el mismo autotest usa un timeout de 12 segundos para el sensor.
    if (tcValid) {
      bootNextStage(BT_WAIT_COOL,PSTR("COOL <58 5SEC"));
    } else if ((unsigned long)(now - bootStageSince) > BOOT_TC_TIMEOUT_MS) {
      markSelftestFailure(FAULT_TC,PSTR("MAX NO 3READS"));
    }
    return;
  }
  if (bootStage == BT_WAIT_COOL) {
    diagnosticPumpAir = 100;
    if (!tcValid) {
      bootColdStableSince = 0;
      if ((unsigned long)(now - lastValidTcMs) > 3000UL)
        markSelftestFailure(FAULT_TC,PSTR("MAX LOST 3SEC"));
      return;
    }
    // Una muestra espuria que caiga de 70 a 53 C NO habilita AIR40.
    if (tempC <= BOOT_COOL_BELOW_C) {
      if (bootColdStableSince == 0) bootColdStableSince = now;
      if ((unsigned long)(now - bootColdStableSince) >= BOOT_COOL_STABLE_MS)
        bootNextStage(BT_SOFT_STRESS,PSTR("MODEL 3x4096"));
    } else bootColdStableSince = 0;
    return;
  }
  if (bootStage == BT_SOFT_STRESS) {
    // Nano: reservar margen para stack, bibliotecas e interrupciones.
    // La medicion es aproximada; validar SRAM real al compilar en Arduino IDE.
    if (freeRam() < 256) {
      markSelftestFailure(FAULT_SELFTEST,PSTR("SRAM FREE BELOW 256 B"));
      return;
    }
    if (!fixedTransitionAssertions()) {
      markSelftestFailure(FAULT_SELFTEST,PSTR("FIXED COOLDOWN AIR40 TRANSITION"));
      return;
    }
    for (uint8_t i=0; i<STRESS_BATCH && stressCount<STRESS_CASES; i++) oneStressCase();
    if (stressFailures) { markSelftestFailure(FAULT_SELFTEST,PSTR("MODEL INVARIANT")); return; }
    if (stressCount == STRESS_CASES) {
      Serial.print(F("MODEL PASS,CASES=")); Serial.println(stressCount);
      noInterrupts();
      bootZcStartCount = zcCount;
      bootZcStartRejected = zcRejectedCount;
      interrupts();
      bootNextStage(BT_WAIT_ZC,PSTR("ZERO CROSS COUNT"));
    }
    return;
  }
  if (bootStage == BT_WAIT_ZC) {
    if ((unsigned long)(now - bootStageSince) < 1000UL) return;
    noInterrupts();
    unsigned long z = zcCount;
    unsigned long glitches = zcRejectedCount;
    interrupts();
    const unsigned long edges = z - bootZcStartCount;
    Serial.print(F("ZC COUNT/GLITCH PER SEC="));
    Serial.print(edges); Serial.print('/');
    Serial.println(glitches - bootZcStartRejected);
    if (edges < 85 || edges > 115 || (glitches - bootZcStartRejected) > 35UL || !zeroCrossHealthy()) {
      markSelftestFailure(FAULT_ZEROCROSS,PSTR("ZERO CROSS NOT ~100Hz"));
      return;
    }
    diagnosticPumpAir = 100;
    bootNextStage(BT_PUMP_FULL,PSTR("PUMP 100 PRETEST"));
    return;
  }
  if (bootStage == BT_PUMP_FULL) {
    if (!tcValid || tempC >= 60.0f) { bootNextStage(BT_WAIT_COOL,PSTR("TEMP HOT DURING TEST")); return; }
    if ((unsigned long)(now - bootStageSince) < 650UL) return;
    noInterrupts();
    bootGateStartCount = pumpGateStartCount;
    bootGateEndCount = pumpGateEndCount;
    interrupts();
    diagnosticPumpAir = 40;
    bootNextStage(BT_PUMP_PHASE,PSTR("PUMP 40 PHASE ONLY"));
    return;
  }
  if (bootStage == BT_PUMP_PHASE) {
    if (!tcValid || tempC >= 60.0f) {
      diagnosticPumpAir=100;
      bootNextStage(BT_WAIT_COOL,PSTR("TEMP HOT PHASE ABORT"));
      return;
    }
    if ((unsigned long)(now - bootStageSince) < 1250UL) return;
    noInterrupts();
    const unsigned long starts = pumpGateStartCount - bootGateStartCount;
    const unsigned long ends = pumpGateEndCount - bootGateEndCount;
    interrupts();
    diagnosticPumpAir = 100;
    Serial.print(F("PUMP40 GATE START/END="));
    Serial.print(starts); Serial.print('/'); Serial.println(ends);
    if (starts < 75 || ends < 75 || starts > 160 || ends > 160) {
      markSelftestFailure(FAULT_PUMP_GATE,PSTR("TIMER1 PUMP40 PULSE COUNT"));
      return;
    }
    bootNextStage(BT_PUMP_RECOVER,PSTR("PUMP 100 RECOVER"));
    return;
  }
  if (bootStage == BT_PUMP_RECOVER) {
    if (!tcValid || tempC >= 60.0f) {
      diagnosticPumpAir=100;
      bootNextStage(BT_WAIT_COOL,PSTR("HOT RECOVERY ABORT"));
      return;
    }
    if ((unsigned long)(now - bootStageSince) < 500UL) return;
    noInterrupts();
    const unsigned long a = pumpGateStartCount;
    const unsigned long b = pumpGateEndCount;
    interrupts();
    if (a > b + 2 || b > a + 2) {
      markSelftestFailure(FAULT_PUMP_GATE,PSTR("UNBALANCED GATE AFTER AIR100"));
      return;
    }
    diagnosticPumpAir = 100;
    bootStage=BT_DONE;
    selftestPassed=true;
    bootGuard.testing = 0; // evita falsos positivos al reiniciar tras PASS
    bootGuard.resets = 0;
    Serial.print(F("BOOT PASS,CASES=")); Serial.print(stressCount);
    Serial.print(F(",RAM=")); Serial.println(freeRam());
    beep(2500, 120);
    return;
  }
}

// ---------------- PID NORMAL ----------------
void servicePID() {
  unsigned long now = millis();
  if (now - lastPidMs < PID_SAMPLE_MS) return;

  float dt = lastPidMs == 0 ? 0.25f : (float)(now - lastPidMs) / 1000.0f;
  lastPidMs = now;

  if (!heaterCanRun() || !tcValid) {
    pidOutput = 0.0f;
    heaterDutyPct = 0;
    heaterPowerAllowed = false;
    digitalWrite(PIN_RELAY, LOW);
    brakeDutyCap = 100;
    return;
  }

  const float rawTarget = (float)internalSetpointC();
  // El PID utiliza mediana filtrada; safety sigue usando RAW directa.
  float measured = isnan(controlTempC) ? tempC : controlTempC;
  // SAFE referencia: ante salto BAJO aislado, no pedir 650W por una lectura.
  if (!isnan(safeThermalReferenceC) && safeThermalReferenceC > measured)
    measured = safeThermalReferenceC;
  const float error = rawTarget - measured;
  const float p = pidKp * error;
  const float d = -pidKd * filteredRateCps;

  // Control de aproximacion: no limita la potencia durante mantenimiento.
  // Si la temperatura sube, proyecta la lectura 2 segundos hacia adelante.
  // El corte preventivo intenta reducir la energia depositada antes del pico.
  float cap = 100.0f;
  if (filteredRateCps >= BRAKE_MIN_RATE_CPS &&
      error > 0.0f && error <= BRAKE_ZONE_C) {
    float futureError = error - filteredRateCps * BRAKE_LOOKAHEAD_S;
    if (futureError <= 0.0f) cap = 0.0f;
    else if (futureError <= 12.0f) cap = 15.0f;
    else if (futureError <= 25.0f) cap = 30.0f;
    else if (futureError <= 50.0f) cap = 55.0f;
    else if (futureError <= 80.0f) cap = 75.0f;
  }

  // Un AIR40 a 650W no tiene el mismo balance termico que AIR100.
  // Limite de potencia proporcional al caudal pedido (no verifica caudal real).
  // Tras un rearme caliente aplica una rampa mas conservadora durante 3 s.
  float flowLimit = (float)airSetPct;
  cap = cap < flowLimit ? cap : flowLimit;
  if (rearmHot && (unsigned long)(now - armReadySinceMs) < REARM_RAMP_MS)
    cap = cap < 25.0f ? cap : 25.0f;
  // El termino derivativo nunca debe aplicar MAS calor si ya se alcanzo SP.
  // El filtro nunca puede ocultar que RAW ya alcanzo el objetivo.
  if (tempC >= rawTarget || measured >= rawTarget) cap = 0.0f;
  // Frenar antes si ya casi alcanzo el objetivo y la T sube rapido.
  if (tempC >= rawTarget - 5.0f && filteredRateCps >= 1.0f)
    cap = 0.0f;

  // Anti-windup: no acumular integral cuando el mando ya esta saturado.
  const float tentative = p + pidIntegral + d;
  bool atUpperLimit = (error > 0.0f && tentative >= cap);
  bool atLowerLimit = (error < 0.0f && tentative <= 0.0f);
  if (!atUpperLimit && !atLowerLimit) {
    pidIntegral += pidKi * error * dt;
    pidIntegral = clampValue(pidIntegral, 0.0f, 100.0f);
  }

  const float out = clampValue(p + pidIntegral + d, 0.0f, cap);
  brakeDutyCap = (uint8_t)(cap + 0.5f);
  pidOutput = out;
  heaterDutyPct = (uint8_t)(out + 0.5f);
  heaterPowerAllowed = true;
  digitalWrite(PIN_RELAY, HIGH);
}

void serviceThermalSafety() {
  if (faultCode != FAULT_NONE) return;
  // Arranque: el MAX6675 aun no tuvo tiempo de entregar 3 lecturas.
  // La salida del heater sigue FISICAMENTE OFF por serviceBootTest();
  // solo el autotest puede declarar error de TC tras su timeout.
  // Conservamos sobretemperatura absoluta incluso durante el boot.
  if (!selftestPassed) {
    if (isrHeartbeatTrip) setFault(FAULT_STALL);
    else if (tcValid && tempC >= ABS_OVER_TEMP_C) setFault(FAULT_OVERTEMP);
    return;
  }
  unsigned long now = millis();

  // Protecciones adicionales antes de cualquier calculo PID.
  if (isrHeartbeatTrip) {
    setFault(FAULT_STALL);
    return;
  }
  if (heatRequested && !standPressed && !cooldownActive) {
    if ((unsigned long)(now - lastValidTcMs) > SAFE_TC_STALE_MS) {
      setFault(FAULT_TC); // perdida adquisicion => SAFE enclavado (Repromod)
      return;
    }
    if (!zeroCrossHealthy()) {
      setFault(FAULT_ZEROCROSS);
      return;
    }
  }

  if (!tcValid) {
    if (heatRequested || (!isnan(tempC) && tempC >= COOLDOWN_TEMP_C)) {
      setFault(FAULT_TC);
    }
    return;
  }

  if (tempC >= ABS_OVER_TEMP_C) {
    setFault(FAULT_OVERTEMP);
    return;
  }

  bool active = heaterCanRun();
  const float rawTarget = (float)internalSetpointC();
  float error = rawTarget - tempC;
  float absError = fabs(error);

  if (active && tempC <= rawTarget + 20.0f) runawayArmed = true;

  if (active && runawayArmed && now >= controlGraceUntil && tempC >= rawTarget + RUNAWAY_DELTA_C) {
    if (runawaySince == 0) runawaySince = now;
    if (now - runawaySince >= RUNAWAY_CONFIRM_MS) {
      setFault(FAULT_RUNAWAY);
      return;
    }
  } else {
    runawaySince = 0;
  }

  if (!active) {
    noRiseSince = 0;
    controlLostSince = 0;
    stableBandSince = 0;
    return;
  }

  if (now < controlGraceUntil) return;

  if (pidOutput >= NO_RISE_MIN_DUTY && error >= NO_RISE_MIN_ERROR_C) {
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
      noRiseSince = now;
      noRiseStartTemp = tempC;
    }
  } else {
    noRiseSince = 0;
  }

  if (absError <= STABLE_BAND_C) {
    if (stableBandSince == 0) stableBandSince = now;
    if (now - stableBandSince >= STABLE_REQUIRED_MS) regulationWasStable = true;
  } else {
    stableBandSince = 0;
  }

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

// ---------------- STAND ----------------
// Un falso pulso en D7 NO debe borrar heatRequested ni exigir otro PLAY.
// Ante HIGH se corta D4/D5 de inmediato; si HIGH persiste 35 ms se trata
// como mango apoyado y se cancela el calor (interlock real conservado).
void serviceStand() {
  const bool raw = digitalRead(PIN_STAND) == HIGH;
  const unsigned long now = millis();
  if (raw != standRawPressed) {
    standRawPressed = raw;
    standRawChangeMs = now;
    if (raw && heatRequested) {
      // Para transitorio falso, preservar la solicitud de heater pero
      // forzar preflujo/verificacion antes de cualquier rearme.
      armHeatFlow((uint8_t)airSetPct);
      Serial.println(F("STAND RAW HIGH,HEAT PAUSE (NOT CANCEL)"));
    }
  }
  standPressed = standRawPressed || standStablePressed;
  if (standRawPressed != standStablePressed &&
      (unsigned long)(now - standRawChangeMs) >= STAND_DEBOUNCE_MS) {
    standStablePressed = standRawPressed;
    standPressed = standRawPressed || standStablePressed;
    if (standStablePressed) {
      // Stand REAL: nunca reactivar el calefactor automaticamente.
      if (heatRequested) Serial.println(F("HEAT CANCEL,STAND CONFIRMED"));
      if (heatRequested || !tcValid || tempC >= COOLDOWN_TEMP_C) beginCooldown();
      else forceHeaterOffPhysical();
      resetPID();
    } else {
      Serial.println(F("STAND RELEASED,NO AUTO START"));
      resetPID();
      // Si hubo un pulso corto mientras el usuario tenia HEAT ON,
      // armHeatFlow() ya aseguro que no vuelva el calor inmediatamente.
    }
  }
}

// ---------------- INPUT ----------------
void toggleHeater() {
  if (faultCode != FAULT_NONE || uiMode != UI_NORMAL || !selftestPassed) return;

  if (heatRequested) {
    Serial.println(F("HEAT CANCEL,TEMP BUTTON"));
    heatRequested = false;
    armStage = ARM_IDLE;
    forceHeaterOffPhysical();
    if (!tcValid || tempC >= COOLDOWN_TEMP_C) beginCooldown();
    beep(1000, 90);
    return;
  }

  // La secuencia COOLDOWN -> AIR40 -> PLAY ES un caso de prueba valido.
  // Solo se niega calor con condiciones fisicamente inseguras, NO AIR40.
  const bool fresh = tcValid && consecutiveValidTcReads >= 3 &&
        (unsigned long)(millis() - lastValidTcMs) <= 500UL;
  if (!fresh || standPressed || airSetPct < AIR_MIN_HEATER_PCT ||
      !zeroCrossHealthy() || isnan(tempC) || tempC >= ABS_OVER_TEMP_C) {
    beep(450, 220);
    Serial.println(F("START DENIED: TC/STAND/AIR<20/ZC/OVERTEMP"));
    return;
  }

  const bool resumedFromCooldown = cooldownActive;
  // Evitar perdida de flujo en toda transicion: no cortar bomba de cooldown
  // hasta que armHeatFlow la fuerce a 100% y rearme temporizadores.
  armHeatFlow((uint8_t)airSetPct);
  cooldownActive = false;
  heatRequested = true;
  pumpManualEnabled = true;
  cooldownCompletedLatch = false;
  if (resumedFromCooldown)
    Serial.println(F("COOL RESUME, SAFE REFLOW, AIR VARIABLE"));
  else Serial.println(F("HEAT REQUEST, SAFE REFLOW, AIR VARIABLE"));
  beep(2500, 80);
}

void togglePump() {
  if (uiMode != UI_NORMAL) return;
  if (tcValid && tempC >= COOLDOWN_TEMP_C) {
    beginCooldown();
    beep(450, 180);
    return;
  }
  if (heatRequested) {
    pumpManualEnabled = true;
    beep(450, 180);
    return;
  }
  pumpManualEnabled = !pumpManualEnabled;
  if (pumpManualEnabled && airSetPct == 0) airSetPct = max(lastNonZeroAirPct, AIR_MIN_HEATER_PCT);
}

void serviceUiInputs() {
  if (!selftestPassed) return;
  int8_t tStep = readTempEncoderStep();
  int8_t aStep = readAirEncoderStep();
  uint8_t tEvt = updateButton(BTN_TEMP_IDX);
  uint8_t aEvt = updateButton(BTN_AIR_IDX);

  if (uiMode == UI_FAULT) {
    const bool safeToRearm = selftestPassed && tcValid &&
       (unsigned long)(millis()-lastValidTcMs) <= SAFE_TC_STALE_MS &&
       tempC <= COOLDOWN_STOP_C && !standPressed &&
       zeroCrossHealthy() && !emergencyButtonsHeld;
    if (tEvt == BTN_LONG && safeToRearm &&
        faultCode != FAULT_LCD && faultCode != FAULT_WATCHDOG &&
        faultCode != FAULT_RESET_LOOP && faultCode != FAULT_SELFTEST) {
      Serial.print(F("SAFE REARM,MANUAL,PREV=")); Serial.println((int)faultCode);
      faultCode = FAULT_NONE;
      faultCoolingActive = false;
      cooldownActive = false;
      uiMode = UI_NORMAL;
      heatRequested = false; // explicitamente NO reactivar calor al rearmar
      armStage = ARM_IDLE;
      resetPID();
      beep(2200, 100);
    }
    return;
  }

  // NORMAL
  if (tStep != 0) {
    setPointC = clampValue(setPointC + tStep * TEMP_STEP_C, TEMP_MIN_C, TEMP_MAX_C);
    resetPID();
  }

  if (aStep != 0) {
    const int oldAir = airSetPct;
    const int v = clampValue(airSetPct + aStep * AIR_STEP_PCT, AIR_MIN_PCT, AIR_MAX_PCT);
    airSetPct = v;
    if (airSetPct > 0) {
      lastNonZeroAirPct = airSetPct;
      pumpManualEnabled = true;
    }
    if (heatRequested && v != oldAir) {
      if (v < AIR_MIN_HEATER_PCT) {
        // AIR 0..19 se puede seleccionar, pero HEAT se cancela y enfria.
        beginCooldown();
        Serial.println(F("AIR BELOW SAFE MIN, HEATER OFF, COOLDOWN"));
      } else {
        // IMPORTANTE: no bajar heatRequested a false durante el cambio.
        // Es una pausa de control de la bomba, no un nuevo STOP del usuario.
        armHeatFlow((uint8_t)v); // calor OFF durante nueva prueba Timer1
        Serial.println(F("AIR CHANGE, HEAT REQUEST KEPT, AUTO RESUME"));
      }
    }
  }

  if (tEvt == BTN_SHORT) {
    Serial.println(F("INPUT,TEMP BUTTON SHORT"));
    toggleHeater();
  }
  if (aEvt == BTN_SHORT) togglePump();

  // Cambiar estimado OUT/RAW solo en frio, sin calefactor ni cooldown.
  if (tEvt == BTN_LONG) {
    if (!heatRequested && !cooldownActive && tcValid && tempC <= COOLDOWN_STOP_C) {
      outletCalMode = !outletCalMode;
      setPointC = outletCalMode ? 200 : 295;
      resetPID();
      Serial.println(outletCalMode ? F("MODE OUT *0.68") : F("MODE RAW"));
    } else beep(450, 180);
    return;
  }

  // AIR largo: reservado para una futura version con autotune.
  if (aEvt == BTN_LONG) beep(450, 150);
}

// ---------------- LCD ----------------
void updateLCD() {
  if (!lcdReady) return;
  unsigned long now = millis();
  if (now - lastLCD < LCD_REFRESH_MS) return;
  lastLCD = now;

  char top[17];
  char bottom[17];
  if (!selftestPassed && bootStage != BT_FAILED) {
    snprintf_P(top,sizeof(top),PSTR("BOOT %u/7"),(unsigned)bootStage+1);
    if (bootStage == BT_WAIT_COOL) snprintf_P(bottom,sizeof(bottom),PSTR("COOL RAW %3dC"),tcValid?(int)tempC:-1);
    else if (bootStage == BT_SOFT_STRESS) snprintf_P(bottom,sizeof(bottom),PSTR("TEST %5u"),stressCount);
    else if (bootStage == BT_PUMP_PHASE) strcpy_P(bottom,PSTR("PUMP 40 TEST"));
    else strcpy_P(bottom,PSTR("HEATER OFF"));
  } else if (uiMode == UI_FAULT || faultCode != FAULT_NONE) {
    snprintf_P(top,sizeof(top),PSTR("FAULT #%02u"),faultCode);
    snprintf_P(bottom,sizeof(bottom),PSTR("RAW:%3d SAFE"),tcValid?(int)tempC:-1);
  } else {
    const int t = tcValid ? (int)(tempC * (outletCalMode ? OUTLET_SCALE : 1.0f) + 0.5f) : -1;
    snprintf_P(top,sizeof(top),PSTR("%c:%3d/%3d A%3d"), outletCalMode?'E':'R',t,setPointC,airSetPct);
    if (heatRequested && armStage != ARM_READY) {
      const unsigned long elapsed = millis() - armCycleStartMs;
      const unsigned int secs = (elapsed >= AIR_CHANGE_HOLD_MS) ? 0U :
           (unsigned int)((AIR_CHANGE_HOLD_MS - elapsed + 999UL) / 1000UL);
      snprintf_P(bottom,sizeof(bottom),PSTR("AIR STABLE %us"),secs);
    }
    else if (heatRequested)
      snprintf_P(bottom,sizeof(bottom),PSTR("H:%3u%% CAP:%3u"),heaterDutyPct,brakeDutyCap);
    else if (cooldownActive) snprintf_P(bottom,sizeof(bottom),PSTR("H:%3u%% COOL"),heaterDutyPct);
    else snprintf_P(bottom,sizeof(bottom),PSTR("H:%3u%% OFF"),heaterDutyPct);
  }
  printLine(0,top);
  printLine(1,bottom);
}

void updateSerial() {
  unsigned long now = millis();
  if (now - lastSerial < SERIAL_REFRESH_MS) return;
  lastSerial = now;

  unsigned long starts, ends;
  noInterrupts();
  starts = pumpGateStartCount;
  ends = pumpGateEndCount;
  interrupts();

  // Compact CSV: evite largas rutinas Serial en el loop con WDT de 500ms.
  Serial.print(now); Serial.print(',');
  Serial.print(tcValid ? tempC : NAN,1); Serial.print(',');
  Serial.print(setPointC); Serial.print(',');
  Serial.print(airSetPct); Serial.print(',');
  Serial.print((int)effectiveAirPctISR); Serial.print(',');
  Serial.print((int)heaterDutyPct); Serial.print(',');
  Serial.print(heatRequested?1:0); Serial.print(',');
  Serial.print((int)armStage); Serial.print(',');
  Serial.print((int)faultCode); Serial.print(',');
  Serial.print(zeroCrossHealthy()?1:0); Serial.print(',');
  Serial.print((int)bootStage); Serial.print(',');
  Serial.print(freeRam()); Serial.print(',');
  Serial.print(starts); Serial.print(',');
  Serial.print(ends); Serial.print(',');
  Serial.print((int)isrHeartbeatTrip); Serial.print(',');
  Serial.print(tcValid ? controlTempC : NAN,1); Serial.print(',');
  Serial.print((int)brakeDutyCap); Serial.print(',');
  Serial.print(safeSensorDropCount); Serial.print(',');
  Serial.println(safeThermalReferenceC,1);
}

// ---------------- DIAGNOSTICO SERIAL ----------------
// Prueba deliberada del watchdog SOLO en condiciones frias y heater apagado.
// Hacerla con TODA LA SECCION DE RED DESCONECTADA (solo USB / 5 V).
void serviceSerialCommands() {
  if (Serial.available() <= 0) return;
  char c = (char)Serial.read();
  if (c=='?') {
    Serial.println(F("CMD: ? help | W WDT inject (cold/USB) | L LCD reinit (cold) | R rerun test (cold)"));
    return;
  }
  if (c != 'W' && c != 'L' && c != 'R') return;
  if (heatRequested || !tcValid || tempC>=60.0f || cooldownActive) {
    Serial.println(F("CMD DENIED: HEAT OFF + TC RAW<60 + NO COOLDOWN REQUIRED"));
    return;
  }
  forceHeaterOffPhysical();
  if (c=='W') {
    Serial.println(F("WDT INJECT: Nano completo RESET ~500ms, no solo LCD"));
    Serial.flush();
    while (true) { }  // provoque WDT; solo ejecutar SIN 220V
  }
  if (c=='L') {
    Serial.println(F("LCD REINIT: heater locked during I2C"));
    selftestPassed=false;
    diagnosticPumpAir=100;
    int rc=lcd.begin(16,2);
    if (rc==0) { lcd.backlight(); lcdReady=true; Serial.println(F("LCD REINIT OK")); }
    else { lcdReady=false; setFault(FAULT_LCD); Serial.println(F("LCD REINIT FAILED")); }
    // Retest de todo al recuperar LCD; NO rearmar heater automaticamente.
  }
  if (c=='R' || c=='L') {
    if (faultCode != FAULT_NONE) { Serial.println(F("RETEST BLOCKED: fault latched; repair/restart")); return; }
    stressCount=0; stressFailures=0; stressSeed=0x850139UL;
    bootGuard.resets=0; bootGuard.testing=1;
    heatRequested=false; selftestPassed=false; bootStage=BT_WAIT_TC;
    bootStageSince=millis(); diagnosticPumpAir=100;
    Serial.println(F("RETEST START, HEATER OFF"));
  }
}

// ---------------- REPROMOD-STYLE EMERGENCY ----------------
// BTN1 + BTN2 simultaneos. El corte es inmediato sin esperar debounce;
// si los dos siguen presionados 120ms, registra un FAULT enclavado.
void serviceEmergencyButtons() {
  static unsigned long bothSinceMs = 0;
  static bool wasBoth = false;
  const bool both = digitalRead(PIN_TEMP_SW) == LOW &&
                    digitalRead(PIN_AIR_SW) == LOW;
  emergencyButtonsHeld = both;
  if (both) {
    // No depende de SPI, PID, UI, LCD ni operaciones en Serial.
    forceHeaterOffPhysical();
    if (!wasBoth) bothSinceMs = millis();
    if (faultCode == FAULT_NONE &&
        (unsigned long)(millis() - bothSinceMs) >= SAFE_BUTTONS_EMERGENCY_MS)
      setFault(FAULT_ESTOP);
  } else if (wasBoth && heatRequested && faultCode == FAULT_NONE) {
    // Si fue un toque accidental, no restaurar calor instantaneamente.
    armHeatFlow((uint8_t)airSetPct);
  }
  wasBoth = both;
}

// ---------------- SETUP / LOOP ----------------
void setup() {
  pinMode(PIN_PUMP, OUTPUT);
  pinMode(PIN_HEATER, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_SPEAKER, OUTPUT);
  digitalWrite(PIN_PUMP, HIGH); // arranque fail-safe: aire al maximo
  digitalWrite(PIN_HEATER, LOW);
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_SPEAKER, LOW);

  pinMode(PIN_ZC, INPUT);
  pinMode(PIN_STAND, INPUT);
  pinMode(PIN_TEMP_CLK, INPUT_PULLUP);
  pinMode(PIN_TEMP_DT, INPUT_PULLUP);
  pinMode(PIN_TEMP_SW, INPUT_PULLUP);
  pinMode(PIN_AIR_CLK, INPUT_PULLUP);
  pinMode(PIN_AIR_DT, INPUT_PULLUP);
  pinMode(PIN_AIR_SW, INPUT_PULLUP);

  Serial.begin(115200);

  int lcdStatus = lcd.begin(16, 2);
  lcdReady = (lcdStatus == 0);
  if (lcdReady) lcd.backlight();
  if (lcdStatus) {
    // Nunca habilitar calentamiento si falla la inicializacion de pantalla.
    faultCode = FAULT_LCD;
    uiMode = UI_FAULT;
    faultCoolingActive = true;
    forceHeaterOffPhysical();
  }

  if (lcdReady) {
    printLine(0, "HONY 850 LAB");
    printLine(1, "V1.3.15 LAB");
  }

  lastTempCLK = digitalRead(PIN_TEMP_CLK);
  lastAirCLK = digitalRead(PIN_AIR_CLK);
  standPressed = (digitalRead(PIN_STAND) == HIGH);
  standRawPressed = standPressed;
  standStablePressed = standPressed;
  standRawChangeMs = millis();

  setupTimer1();
  // Durante arranque garantizar flujo en lugar de dejar el triac de bomba apagado.
  effectiveAirPctISR = 100;
  attachInterrupt(digitalPinToInterrupt(PIN_ZC), zeroCrossISR, RISING);

  delay(500);
  tempC = thermocouple.readCelsius();
  // Primera lectura solo informativa; exigir otras 3 en serviceTemperature().
  tcValid = false;
  consecutiveValidTcReads = 0;
  previousTempC = tempC;
  resetPID();

  // Contar reinicios durante autodiagnostico. El bootloader puede borrar MCUSR;
  // esta guarda SRAM detecta bucles de reset durante la prueba, incluso USB/DTR.
  const bool guardOK = bootGuard.marker == BOOT_GUARD_MAGIC &&
                       bootGuard.inverse == (uint16_t)~BOOT_GUARD_MAGIC;
  if (!guardOK) {
    bootGuard.marker = BOOT_GUARD_MAGIC;
    bootGuard.inverse = (uint16_t)~BOOT_GUARD_MAGIC;
    bootGuard.testing = 0;
    bootGuard.resets = 0;
  }
  if (bootGuard.testing && bootGuard.resets < 255U) bootGuard.resets++;
  else if (!bootGuard.testing) bootGuard.resets = 0;
  bootGuard.testing = 1;
  // Tras 3 resets continuos DURANTE el autotest, bloquear nuevo intento.
  if (bootGuard.resets >= BOOT_RESET_LIMIT) {
    faultCode = FAULT_RESET_LOOP; // bucle de reset, no asumimos causa watchdog
    uiMode = UI_FAULT;
    faultCoolingActive = true;
    bootStage = BT_FAILED;
    selftestPassed = false;
    diagnosticPumpAir = 100;
  }

  // Al reiniciar por watchdog NO volver a armar el heater: fault enclavado.
  if (bootResetFlags & _BV(WDRF)) {
    // Si el watchdog se activo durante la prueba AIR40, NO reintentar
    // automaticamente la misma secuencia en un boot-loop.
    faultCode = FAULT_WATCHDOG;
    uiMode = UI_FAULT;
    faultCoolingActive = true;
    bootStage = BT_FAILED;
    selftestPassed = false;
    diagnosticPumpAir = 100;
  }
  // No enclavar TC_ERROR durante la ventana inicial de 3 conversiones.
  // Aun sin TC validada el heater esta OFF y la bomba se mantiene encendida.
  // Si la temperatura de la primera lectura es elevada, enfriar preventivamente.
  if (!isnan(tempC) && tempC >= COOLDOWN_TEMP_C && faultCode == FAULT_NONE) beginCooldown();
  if (bootStage != BT_FAILED) bootStage=BT_WAIT_TC;
  bootStageSince=millis();
  diagnosticPumpAir=100;
  effectiveAirPctISR = computeEffectiveAirPct();
  // Heartbeat inicial antes de armar el watchdog.
  // IMPORTANTE: si se necesita ZC con la red y se observa por Serial, usar
  // aislador USB adecuado, o trabajar sin USB y observar el LCD. No alimentar
  // 5V interno + VBUS USB en paralelo sin verificar proteccion de fuente.
  unsigned long hb = micros();
  noInterrupts();
  lastControlHeartbeatUs = hb;
  interrupts();
  wdt_reset();
  wdt_enable(WDTO_500MS);
  Serial.print(F("BOOT RESET_FLAGS=0x")); Serial.println(bootResetFlags, HEX);
  Serial.print(F("BOOT RESET_LOOP_GUARD=")); Serial.println(bootGuard.resets);
  Serial.println(F("V1.3.15 SAFE LATCH + AIR 3SEC; WDT500"));
  Serial.println(F("ms,rawC,spUI,airSet,airActual,duty,heat,arm,fault,zcOK,boot,ram,gateOn,gateOff,stall,pidTemp,cap,tcDrop,safeRef"));

  if (bootStage == BT_FAILED) Serial.println(F("BOOT LATCH: WDT/RESET LOOP; NO AUTO-RETEST. REMOVE USB AND POWER CYCLE AFTER CHECK"));
  else Serial.println(F("BOOT AUTO-TEST START: heater gated OFF"));
  Serial.print(F("BOOT RAM="));
  Serial.println(freeRam());
  beep(2000, 80);
}

void loop() {
  serviceEmergencyButtons(); // primer servicio, prioridad sobre UI
  serviceTemperature();
  serviceStand();
  serviceBootTest();
  serviceSerialCommands();
  if (!emergencyButtonsHeld) serviceUiInputs();
  serviceCooldown();

  effectiveAirPctISR = computeEffectiveAirPct();
  serviceHeatArm();
  // Si arm cambia el estado, actualizar salida antes del PID.
  effectiveAirPctISR = computeEffectiveAirPct();

  serviceThermalSafety();
  if (faultCode == FAULT_NONE) servicePID();

  if (!heaterCanRun()) {
    heaterPowerAllowed = false;
    heaterDutyPct = 0;
    digitalWrite(PIN_HEATER, LOW);
    digitalWrite(PIN_RELAY, LOW);
  }

  // Refrescar heartbeat DESPUES de temperatura, seguridad y PID, pero ANTES
  // de I2C/Serial para detectar tambien cuelgues dentro de esas rutinas.
  unsigned long hb = micros();
  noInterrupts();
  lastControlHeartbeatUs = hb;
  interrupts();

  updateLCD();
  updateSerial();
  // Unico punto donde alimentamos el watchdog: loop completo y sin bloqueo.
  wdt_reset();
}