#pragma once
#include "Config.h"
#include <stdint.h>

// Hardware de tiempo real: GPIO de potencia y GPTimer ESP32-S3.
// Todo corte de calor pasa por heatOff(). No hay escritura directa desde el PID/UI.
namespace Hardware {
struct Counters {uint32_t edges, rejected, gateOn, gateOff;};
void begin();
bool ready(); // GPTimer configurado, sin fallo
void heartbeat();
void heatOff();
void heatDuty(uint8_t duty);    // duty 0..100; rele ON solo si duty > 0
void pump(uint8_t percent);    // 100=gate HIGH continua, independiente de cruces
uint8_t pumpCommand();
uint8_t dutyCommand();
uint8_t resetReason();
bool stalled();
bool zcHealthy();
uint32_t zcPeriod(); // ultimo intervalo ENTRE flancos aceptados, en microsegundos
Counters counters();
void watchdogStart();
void watchdogKick();
}
