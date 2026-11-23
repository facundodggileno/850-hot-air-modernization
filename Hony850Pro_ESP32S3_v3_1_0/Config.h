#pragma once
#include <Arduino.h>
#include <math.h>
#if !defined(ARDUINO_ARCH_ESP32) || !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Este firmware requiere ESP32-S3 y Arduino-ESP32 3.x."
#endif
// ============================================================================
// HONY 850 PRO ESP32-S3 V3.2.0 - CONFIGURACION PARA EL USUARIO
// EDITAR SOLO ESTE BLOQUE PARA CAMBIAR LIMITES/HABILITACIONES.
// AVISO: la temperatura de salida (~) es ESTIMADA a partir de la sonda interna.
// Los ensayos externos validan aprox. hasta 300 C, NO hasta 480 C.
// La validez electrica y termica NO se garantiza cambiando constantes.
// ============================================================================

// 1. SALIDAS FISICAS (0=APAGADA, 1=HABILITADA)
// Nota: MAINS=1 habilita CONJUNTAMENTE el relé y el MOC del calefactor.
// No activarlo hasta validar corte fisico, control de potencia y aislamiento.
#ifndef HONY_ENABLE_MAINS_OUTPUTS
#define HONY_ENABLE_MAINS_OUTPUTS 0
#endif
// Bomba 220V por MOC3023: ensayos de aire habilitados (modo ya probado).
#ifndef HONY_ENABLE_PUMP_OUTPUT
#define HONY_ENABLE_PUMP_OUTPUT 1
#endif

// 2. LIMITES DEL SELECTOR Y TEMPERATURA (C)
#define HONY_USER_MAX_TEMP_C               480
#define HONY_CALIBRATED_MAX_OUTLET_C        300  // Maximo respaldado por termocupla externa
#define HONY_INTERNAL_HARD_C              595.0f // Corte de la sonda interna (proteccion)
#define HONY_INTERNAL_HEADROOM_C          35.0f  // Margen hasta el corte interno
#define HONY_INTERNAL_SENSOR_VALID_MAX_C 700.0f  // Lecturas superiores se invalidan
#define HONY_INTERNAL_SENSOR_VALID_MIN_C   5.0f
// IMPORTANTE: para alcanzar 480 C de salida estimada no basta con tocar USER_MAX.
// La calibracion, la resistencia, mango y protecciones requieren verificacion real.

// 3. SOPORTE, BOMBA, ENFRIAMIENTO
#define HONY_AUTO_START_ON_LIFT              1    // Levantar tras detectar stand = iniciar
#define HONY_MIN_AIR_TO_RUN                 20    // AIR 0..19 apaga calor y orden normal de bomba
#define HONY_COOL_START_OUTLET_C           79.0f // Bomba forzada en estado caliente
#define HONY_COOL_STOP_OUTLET_C            73.0f // Bomba puede parar tras enfriar
#define HONY_STAND_DEBOUNCE_CONFIG_MS       35UL // Inmediatez al apoyar, debounce al sacar
#define HONY_PRE_FLOW_TIME_MS             3000UL // Calor solo tras este preflujo
#define HONY_PUMP_GATE_TEST_TIME_MS       1000UL
#define HONY_PUMP_GATE_TEST_MIN_EVENTS        70  // Comando MOC ON/OFF esperado en prueba 1s
#define HONY_PUMP_GATE_MONITOR_MIN_EVENTS     35  // Comando MOC ON/OFF esperado en vigilancia 0,5s
#define HONY_PUMP_GATE_MAX_IMBALANCE           4  // Diferencia entre pulsos ON y OFF

// 4. CANALES PERSISTENTES NVS
#define HONY_CHANNEL_COUNT                   5   // CH1..CH5
#define HONY_AIR_SAVE_HOLD_MS           10000UL // Pulsacion AIR sostenida, sin menu intermedio
#define HONY_CHANNEL_SAVE_FEEDBACK_MS     1800UL

// 5. PROTECCIONES TERMICAS (fuera del menu)
#define HONY_TC_RISE_SPIKE_C               42.0f // Pico interno en una muestra
#define HONY_TC_DROP_SPIKE_C               22.0f
#define HONY_TUNE_OVERSHOOT_C              20.0f // Exceso de salida estimada en autotune
#define HONY_OVERSHOOT_C                   25.0f // Exceso sostenido de salida estimada
#define HONY_OVERSHOOT_CONFIRM_MS          750UL
#define HONY_GRACE_AFTER_START_MS         5000UL
#define HONY_NO_RISE_DUTY_MIN               25
#define HONY_NO_RISE_ERROR_C              14.0f
#define HONY_NO_RISE_WINDOW_MS            8000UL
#define HONY_NO_RISE_MIN_DELTA_C           2.0f
#define HONY_STABLE_ERROR_BAND_C           7.0f
#define HONY_STABLE_PERIOD_MS            10000UL
#define HONY_CONTROL_LOST_ERROR_C         28.0f
#define HONY_CONTROL_LOST_PERIOD_MS      10000UL

// 5.b AUTOTUNE Y VALIDACION DE PID
#define HONY_AUTOTUNE_TOTAL_TIMEOUT_MS   900000UL
#define HONY_AUTOTUNE_PHASE_TIMEOUT_MS   180000UL
#define HONY_AUTOTUNE_UPPER_DELTA_C         3.0f
#define HONY_AUTOTUNE_LOWER_DELTA_C         3.0f
#define HONY_AUTOTUNE_AMPLITUDE_MIN_C       1.0f
#define HONY_AUTOTUNE_AMPLITUDE_MAX_C     110.0f
#define HONY_AUTOTUNE_PERIOD_MIN_S          1.0f
#define HONY_AUTOTUNE_PERIOD_MAX_S        180.0f
#define HONY_AUTOTUNE_MAX_DUTY              65
#define HONY_PID_KP_MIN                    0.05f
#define HONY_PID_KP_MAX                   30.0f
#define HONY_PID_KI_MIN                  0.0001f
#define HONY_PID_KI_MAX                    5.0f
#define HONY_PID_KD_MIN                    0.0f
#define HONY_PID_KD_MAX                  150.0f
// Heap libre minimo para permitir calefaccion en el ESP32-S3.
#define HONY_MIN_HEAP_FOR_HEAT_BYTES      8192UL

// 6. SINCRONISMO Y WATCHDOG
#define HONY_ZC_PERIOD_MIN_US             8000UL // Acepta ~10ms entre cruces en red 50Hz
#define HONY_ZC_PERIOD_MAX_US            12000UL
#define HONY_ZC_BOOT_MIN_PULSES             80   // Ventana 1s; esp. ~100 pulsos
#define HONY_ZC_BOOT_MAX_PULSES            120
#define HONY_ZC_STALE_TIMEOUT_US        150000UL
#define HONY_ZC_REJECT_WINDOW_US         7000UL
#define HONY_HEARTBEAT_TIMEOUT_US      300000UL
#define HONY_WATCHDOG_TIMEOUT_MS          2000UL
#define HONY_ESTOP_HOLD_TIME_MS            120UL // Ambos encoders, E-STOP

// 7. TIEMPOS DE MUESTREO / PANTALLA. Modificar con cuidado.
#define HONY_SAMPLE_TC_MS                  250UL
#define HONY_SAMPLE_STALE_MS               750UL
#define HONY_SAMPLE_PID_MS                 250UL
#define HONY_UPDATE_LCD_MS                 200UL
#define HONY_UPDATE_SERIAL_MS             1000UL

#if HONY_CHANNEL_COUNT < 1 || HONY_CHANNEL_COUNT > 9
#error "Cantidad de canales entre 1 y 9 (LCD1602)."
#endif
#if HONY_MIN_AIR_TO_RUN < 1 || HONY_MIN_AIR_TO_RUN > 100
#error "HONY_MIN_AIR_TO_RUN debe estar entre 1 y 100."
#endif
// ============================================================================

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
constexpr int TEMP_MIN=80, TEMP_MAX=HONY_USER_MAX_TEMP_C, TEMP_STEP=5;
constexpr int MAX_CALIBRATED_OUTLET_C=HONY_CALIBRATED_MAX_OUTLET_C;
constexpr uint8_t AIR_MIN_HEAT=HONY_MIN_AIR_TO_RUN, AIR_STEP=5;
constexpr float RAW_HARD_LIMIT=HONY_INTERNAL_HARD_C; // Corte critico en el sensor interno.
constexpr float INTERNAL_HARD_MARGIN=HONY_INTERNAL_HEADROOM_C;
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
constexpr float OUTLET_COOL_START_C=HONY_COOL_START_OUTLET_C, OUTLET_COOL_STOP_C=HONY_COOL_STOP_OUTLET_C;
constexpr uint32_t TC_SAMPLE_MS=HONY_SAMPLE_TC_MS, TC_STALE_MS=HONY_SAMPLE_STALE_MS;
constexpr uint32_t PID_MS=HONY_SAMPLE_PID_MS, LCD_MS=HONY_UPDATE_LCD_MS, SERIAL_MS=HONY_UPDATE_SERIAL_MS;
constexpr uint32_t PREFLOW_MS=HONY_PRE_FLOW_TIME_MS, PUMP_GATE_TEST_MS=HONY_PUMP_GATE_TEST_TIME_MS;
constexpr uint32_t BOOT_TC_MS=4000, BOOT_ZC_MS=1000, BOOT_WARN_MS=4000;
constexpr uint32_t ZC_STALE_US=HONY_ZC_STALE_TIMEOUT_US, HEARTBEAT_STALE_US=HONY_HEARTBEAT_TIMEOUT_US;
constexpr uint32_t ZC_REJECT_US=HONY_ZC_REJECT_WINDOW_US;
constexpr uint16_t PUMP_GATE_US=180, PUMP_MIN_DELAY_US=200;
constexpr uint16_t PUMP_DELAY_20_US=7600, PUMP_DELAY_95_US=3000;
constexpr uint16_t PUMP_MAX_DELAY_US=9000;
constexpr uint32_t BUTTON_LONG_MS=1200, BUTTON_DEBOUNCE_MS=35;
constexpr uint32_t STAND_DEBOUNCE_MS=HONY_STAND_DEBOUNCE_CONFIG_MS, ESTOP_HOLD_MS=HONY_ESTOP_HOLD_TIME_MS;
constexpr uint8_t TUNE_CYCLES=3, DEBUG_PAGES=8, CHANNELS=HONY_CHANNEL_COUNT;
constexpr uint32_t AIR_SAVE_HOLD_MS=HONY_AIR_SAVE_HOLD_MS;
constexpr uint32_t CHANNEL_SAVED_MS=HONY_CHANNEL_SAVE_FEEDBACK_MS;
constexpr bool AUTO_START_ON_LIFT=(HONY_AUTO_START_ON_LIFT!=0);

}
