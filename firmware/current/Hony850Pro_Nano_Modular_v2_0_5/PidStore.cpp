#include "PidStore.h"
#include <EEPROM.h>
namespace {
uint8_t crc(const PidStore::Record& r){
  const uint8_t* p=(const uint8_t*)&r;
  uint8_t c=0xA7;
  for(uint8_t i=0;i<sizeof(r)-1;i++){
    c^=p[i]; c=(uint8_t)((c<<1)|(c>>7));
  }
  return c;
}
}
namespace PidStore {
static_assert(sizeof(Record)==16,"EEPROM PID: layout inesperado");
bool valid(const Gains& g){
 return g.kp>=0.05f && g.kp<=30 && g.ki>=0.0001f && g.ki<=5 && g.kd>=0 && g.kd<=150;
}
bool load(Gains& g){
 Record r; EEPROM.get(0,r);
 if(r.magic!=0x8504 || r.version!=1 || r.checksum!=crc(r))return false;
 Gains c={r.kp,r.ki,r.kd}; if(!valid(c))return false;
 g=c;return true;
}
bool save(const Gains& g){
 if(!valid(g))return false;
 Record r={0x8504,1,g.kp,g.ki,g.kd,0};r.checksum=crc(r);
 EEPROM.put(0,r);return true;
}
}
