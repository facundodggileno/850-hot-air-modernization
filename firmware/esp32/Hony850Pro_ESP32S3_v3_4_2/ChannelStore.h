#pragma once
#include "Config.h"
#include <stdint.h>

// Tres presets de temperatura de SALIDA ESTIMADA y comando AIR (0..100).
// Un canal solo se sobrescribe con AIR sostenido 10s.
// El indice seleccionado se guarda al cambiar de canal.
class ChannelStore {
public:
  struct Preset {uint16_t tempC;uint8_t air;};
  ChannelStore();
  void begin();
  uint8_t selectedIndex()const{return selected_;}
  Preset selectedPreset()const{return presets_[selected_];}
  bool select(uint8_t index);
  bool saveCurrent(int tempC,int air);
  bool presetsLoadedFromNvs()const{return loaded_;}
private:
  struct __attribute__((packed)) Blob {
    uint16_t magic;
    uint8_t version;
    uint8_t count;
    Preset presets[Cfg::CHANNELS];
    uint8_t checksum;
  };
  Preset presets_[Cfg::CHANNELS];
  uint8_t selected_;
  bool loaded_;
  void defaults();
  static uint8_t checksum(const Blob& b);
  static bool valid(const Preset& p);
};
