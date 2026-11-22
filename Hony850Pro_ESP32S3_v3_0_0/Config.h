#pragma once
#include <Arduino.h>
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

constexpr float CORRECTION=0.68f;     // C indicados = MAX RAW * 0.68 (calibracion empirica)
constexpr int TEMP_MIN=80, TEMP_MAX=360, TEMP_STEP=5;
constexpr uint8_t AIR_MIN_HEAT=20, AIR_STEP=5;
constexpr float RAW_HARD_LIMIT=595.0f; // No confundir con temperatura de boquilla.
// Cooldown: encender/reforzar al llegar a 110 RAW, detener en 100 RAW (68 C corregidos).
constexpr float RAW_COOL_START=110.0f, RAW_COOL_STOP=100.0f;
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
constexpr uint8_t TUNE_CYCLES=3;
inline float rawTarget(int correctedC) {return (float)correctedC/CORRECTION;}
inline int corrected(float raw) {return (int)(raw*CORRECTION+0.5f);}
}
