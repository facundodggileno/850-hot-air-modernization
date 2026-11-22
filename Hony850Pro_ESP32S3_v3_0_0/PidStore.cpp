#include "PidStore.h"
#include <Preferences.h>
namespace {
uint8_t crc(const PidStore::Record& r) {
  const uint8_t* p=reinterpret_cast<const uint8_t*>(&r);
  uint8_t c=0xA7;
  for(size_t i=0;i<sizeof(r)-1;++i) {
    c^=p[i];c=static_cast<uint8_t>((c<<1)|(c>>7));
  }
  return c;
}
}
namespace PidStore {
static_assert(sizeof(Record)==16,"NVS PID layout inesperado");
bool valid(const Gains& g) {
 return g.kp>=0.05f && g.kp<=30 && g.ki>=0.0001f &&
        g.ki<=5 && g.kd>=0 && g.kd<=150;
}
bool load(Gains& g) {
 Preferences store;
 if(!store.begin("hony850",true))return false;
 Record r={};
 const bool ok=store.getBytesLength("pid")==sizeof(r) &&
   store.getBytes("pid",&r,sizeof(r))==sizeof(r);
 store.end();
 if(!ok||r.magic!=0x8504||r.version!=1||r.checksum!=crc(r))return false;
 const Gains candidate={r.kp,r.ki,r.kd};
 if(!valid(candidate))return false;
 g=candidate;return true;
}
bool save(const Gains& g) {
 if(!valid(g))return false;
 Record r={0x8504,1,g.kp,g.ki,g.kd,0};
 r.checksum=crc(r);
 Preferences store;
 if(!store.begin("hony850",false))return false;
 const bool ok=store.putBytes("pid",&r,sizeof(r))==sizeof(r);
 store.end();
 return ok;
}
}
