#pragma once
#include "Types.h"

namespace PidStore {
// Mismo layout/version que la V1.4.x: permite recuperar PID de EEPROM previa.
struct __attribute__((packed)) Record {
  uint16_t magic;
  uint8_t version;
  float kp,ki,kd;
  uint8_t checksum;
};
bool load(Gains& gains);
bool save(const Gains& gains);
bool valid(const Gains& gains);
}
