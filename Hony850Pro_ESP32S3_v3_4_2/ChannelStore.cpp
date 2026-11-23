#include "ChannelStore.h"
#include <Preferences.h>
#include <stddef.h>
#include <string.h>
namespace {
constexpr uint16_t kMagic=0x8505;
constexpr uint8_t kVersion=1;
constexpr const char* kNamespace="hony850";
constexpr const char* kProfilesKey="chset3"; // 3 canales: nuevo tamaño/version de datos
constexpr const char* kLegacyProfilesKey="chset1"; // Presets V3.2 (5 canales)
constexpr const char* kSelectedKey="chsel1";
uint8_t crcProfile(const uint8_t* bytes,size_t len){
  uint8_t c=0xA5;
  for(size_t i=0;i<len;i++){
    c^=bytes[i];
    c=(uint8_t)((c<<1)|(c>>7));
  }
  return c;
}
}
ChannelStore::ChannelStore():selected_(0),loaded_(false){defaults();}
void ChannelStore::defaults(){
  const Preset standard[3]={{250,100},{180,60},{230,80}};
  for(uint8_t i=0;i<Cfg::CHANNELS;i++)
    presets_[i]=standard[i%3];
}
uint8_t ChannelStore::checksum(const Blob& b){
  return crcProfile(reinterpret_cast<const uint8_t*>(&b),
                    offsetof(Blob,checksum));
}
bool ChannelStore::valid(const Preset& p){
  return p.tempC>=Cfg::TEMP_MIN && p.tempC<=Cfg::TEMP_MAX &&
         p.air<=100;
}
void ChannelStore::begin(){
  defaults();
  selected_=0;loaded_=false;
  Preferences prefs;
  if(!prefs.begin(kNamespace,false))return;
  Blob blob={};
  if(prefs.getBytesLength(kProfilesKey)==sizeof(blob) &&
     prefs.getBytes(kProfilesKey,&blob,sizeof(blob))==sizeof(blob) &&
     blob.magic==kMagic && blob.version==kVersion &&
     blob.count==Cfg::CHANNELS && blob.checksum==checksum(blob)){
    bool allValid=true;
    for(uint8_t i=0;i<Cfg::CHANNELS;i++)
      if(!valid(blob.presets[i])){allValid=false;break;}
    if(allValid){
      for(uint8_t i=0;i<Cfg::CHANNELS;i++)presets_[i]=blob.presets[i];
      loaded_=true;
    }
  }
  if(!loaded_){
    // Migracion transparente del formato antiguo de cinco canales.
    // Se conservan solamente CH1, CH2 y CH3; los otros no se borran.
    struct __attribute__((packed)) LegacyBlob {
      uint16_t magic; uint8_t version,count;
      Preset presets[5];
      uint8_t checksum;
    };
    LegacyBlob legacy={};
    if(prefs.getBytesLength(kLegacyProfilesKey)==sizeof(legacy) &&
       prefs.getBytes(kLegacyProfilesKey,&legacy,sizeof(legacy))==sizeof(legacy) &&
       legacy.magic==kMagic && legacy.version==kVersion &&
       legacy.count==5 &&
       legacy.checksum==crcProfile(
          reinterpret_cast<const uint8_t*>(&legacy),
          offsetof(LegacyBlob,checksum))){
      bool validFirstThree=true;
      for(uint8_t i=0;i<Cfg::CHANNELS;i++)
        if(!valid(legacy.presets[i])){validFirstThree=false;break;}
      if(validFirstThree){
        for(uint8_t i=0;i<Cfg::CHANNELS;i++)
          presets_[i]=legacy.presets[i];
        loaded_=true;
        // Persistir en formato nuevo, sin sobrescribir el backup anterior.
        blob.magic=kMagic;blob.version=kVersion;blob.count=Cfg::CHANNELS;
        for(uint8_t i=0;i<Cfg::CHANNELS;i++)blob.presets[i]=presets_[i];
        blob.checksum=checksum(blob);
        prefs.putBytes(kProfilesKey,&blob,sizeof(blob));
      }
    }
  }
  const uint8_t saved=prefs.getUChar(kSelectedKey,0);
  selected_=(saved<Cfg::CHANNELS)?saved:0;
  if(saved>=Cfg::CHANNELS)prefs.putUChar(kSelectedKey,0);
  prefs.end();
}
bool ChannelStore::select(uint8_t index){
  if(index>=Cfg::CHANNELS)return false;
  if(selected_==index)return true;
  selected_=index;
  Preferences prefs;
  if(!prefs.begin(kNamespace,false))return false;
  const bool ok=prefs.putUChar(kSelectedKey,selected_)==1;
  prefs.end();
  return ok;
}
bool ChannelStore::saveCurrent(int tempC,int air){
  if(tempC<Cfg::TEMP_MIN || tempC>Cfg::TEMP_MAX ||
     air<0 || air>100)return false;
  const Preset next={(uint16_t)tempC,(uint8_t)air};
  if(!valid(next))return false;
  Blob blob={};
  blob.magic=kMagic;blob.version=kVersion;blob.count=Cfg::CHANNELS;
  for(uint8_t i=0;i<Cfg::CHANNELS;i++)blob.presets[i]=presets_[i];
  blob.presets[selected_]=next;
  blob.checksum=checksum(blob);
  Preferences prefs;
  if(!prefs.begin(kNamespace,false))return false;
  const bool ok=prefs.putBytes(kProfilesKey,&blob,sizeof(blob))==sizeof(blob);
  prefs.end();
  if(ok)presets_[selected_]=next;
  return ok;
}
