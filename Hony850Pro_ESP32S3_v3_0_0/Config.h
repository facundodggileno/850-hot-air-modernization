#pragma once
#include <Arduino.h>
#include <math.h>
#if !defined(ARDUINO_ARCH_ESP32) || !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Este firmware requiere ESP32-S3 y Arduino-ESP32 3.x."
#endif
#ifndef HONY_ENABLE_MAINS_OUTPUTS
#define HONY_ENABLE_MAINS_OUTPUTS 0
#endif
// Ensayo: permite SOLO la bomba, sin habilitar calefactor ni relé.
#ifndef HONY_ENABLE_PUMP_OUTPUT
#define HONY_ENABLE_PUMP_OUTPUT 1
#endif

// Hony 850 Pro V3.0.0: GPIO ESP32-S3. Solo bomba habilitada para ensayo; heater y rele OFF por defecto.
namespace Cfg {
constexpr uint8_t ZC=4, PUMP=5, HEATER=6, RELAY=7, BUZZER=8, STAND=9;
constexpr uint8_t TEMP_CLK=10, TEMP_DT=11, TEMP_SW=12;
constexpr uint8_t AIR_CLK=13, AIR_DT=14, AIR_SW=15;
constexpr uint8_t MAX_CS=16, MAX_SO=17, MAX_SCK=18;
constexpr uint8_t LCD_SDA=1, LCD_SCL=2;
constexpr uint8_t TC_EXTERNAL_CS=21, TC_PREHEATER_CS=39;
constexpr bool MAINS_OUTPUTS_ENABLED = (HONY_ENABLE_MAINS_OUTPUTS != 0);
constexpr bool PUMP_OUTPUTS_ENABLED = (HONY_ENABLE_PUMP_OUTPUT != 0) || MAINS_OUTPUTS_ENABLED;

// SALIDA ESTIMADA: curva empirica de termocupla interna -> aire en boquilla.
// Datos medidos con sonda externa, principalmente AIR 100%.
// NO es medicion continua de una sonda de salida. Hasta 300C externos validada.
struct CalPoint {float internalC, outletC;};
constexpr CalPoint OUTPUT_CURVE[] = {
 {40.0f,40.0f},  // Aproximacion para temperatura ambiente
 {80.0f,60.0f}, {150.0f,104.0f},
 {230.0f,157.0f}, {300.0f,196.0f},
 {350.0f,239.0f}, {380.0f,256.0f},
 {420.0f,285.0f}, {440.0f,300.0f}
};
constexpr int TEMP_MIN=80, TEMP_MAX=480, TEMP_STEP=5;
constexpr int MAX_CALIBRATED_OUTLET_C=300;
constexpr uint8_t AIR_MIN_HEAT=20, AIR_STEP=5;
constexpr float RAW_HARD_LIMIT=595.0f; // Corte interno fisico conservado.
constexpr float INTERNAL_HARD_MARGIN=35.0f;
inline float outletEstimate(float internalC) {
 if(!isfinite(internalC))return NAN;
 if(internalC<=40.0f)return internalC;
 const size_t n=sizeof(OUTPUT_CURVE)/sizeof(OUTPUT_CURVE[0]);
 for(size_t i=1;i<n;i++){
   if(internalC<=OUTPUT_CURVE[i].internalC){
     const CalPoint &lo=OUTPUT_CURVE[i-1],&hi=OUTPUT_CURVE[i];
     return lo.outletC+(internalC-lo.internalC)*
            (hi.outletC-lo.outletC)/(hi.internalC-lo.internalC);
   }
 }
 // Por encima de 300C de salida: extrapolacion NO validada.
 return 300.0f+(internalC-440.0f)*0.68f;
}
inline float internalTargetForOutlet(float outletC){
 if(!isfinite(outletC))return NAN;
 if(outletC<=40.0f)return outletC;
 const size_t n=sizeof(OUTPUT_CURVE)/sizeof(OUTPUT_CURVE[0]);
 for(size_t i=1;i<n;i++){
   if(outletC<=OUTPUT_CURVE[i].outletC){
     const CalPoint &lo=OUTPUT_CURVE[i-1],&hi=OUTPUT_CURVE[i];
     return lo.internalC+(outletC-lo.outletC)*
            (hi.internalC-lo.internalC)/(hi.outletC-lo.outletC);
   }
 }
 return 440.0f+(outletC-300.0f)/0.68f;
}
inline bool setpointWithinRawLimit(int outletC){
 // Prohibido calentar fuera de los valores contrastados con sonda externa.
 if(outletC>MAX_CALIBRATED_OUTLET_C || outletC<TEMP_MIN)return false;
 const float internalC=internalTargetForOutlet((float)outletC);
 return isfinite(internalC) && internalC<(RAW_HARD_LIMIT-INTERNAL_HARD_MARGIN);
}
// Temperaturas de cooldown equivalentes a los umbrales antiguos 110/100
// del sensor interno, ahora expresadas en C de salida ESTIMADOS.
constexpr float OUTLET_COOL_START_C=79.0f, OUTLET_COOL_STOP_C=73.0f;
constexpr uint32_t TC_SAMPLE_MS=250, TC_STALE_MS=750;
constexpr uint32_t PID_MS=250, LCD_MS=200, SERIAL_MS=1000;
constexpr uint32_t PREFLOW_MS=3000, PUMP_GATE_TEST_MS=1000;
constexpr uint32_t BOOT_TC_MS=4000, BOOT_ZC_MS=1000, BOOT_WARN_MS=4000;
constexpr uint32_t ZC_STALE_US=150000, HEARTBEAT_STALE_US=300000;
constexpr uint32_t ZC_REJECT_US=7000;
constexpr uint16_t PUMP_GATE_US=180, PUMP_MIN_DELAY_US=200;
constexpr uint16_t PUMP_DELAY_20_US=7600, PUMP_DELAY_95_US=3000;
constexpr uint16_t PUMP_MAX_DELAY_US=9000;
constexpr uint32_t BUTTON_LONG_MS=1200, BUTTON_DEBOUNCE_MS=35;
constexpr uint32_t STAND_DEBOUNCE_MS=35, ESTOP_HOLD_MS=120;
constexpr uint8_t TUNE_CYCLES=3, DEBUG_PAGES=8;

}
