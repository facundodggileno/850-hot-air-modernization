#pragma once
#include "Config.h"
#include <stdint.h>

// Hardware de tiempo real: unico dueno de los pines de potencia y Timer1.
// Todo corte de calor pasa por heatOff(). No hay escritura directa desde el PID/UI.
namespace Hardware {
struct Counters {uint32_t edges, rejected, gateOn, gateOff;};
void begin();
void heartbeat();
void heatOff();
void heatDuty(uint8_t duty);    // duty 0..100; rele ON solo si duty > 0
void pump(uint8_t percent);    // 100=gate HIGH continua, independiente de cruces
uint8_t pumpCommand();
uint8_t dutyCommand();
uint8_t resetReason();
bool stalled();
bool zcHealthy();
Counters counters();
void watchdogStart();
void watchdogKick();
}
