#include "ChannelStore.h"
#include <Preferences.h>
#include <stddef.h>
#include <string.h>
namespace {
constexpr uint16_t kMagic=0x8505;
constexpr uint8_t kVersion=1;
constexpr const char* kNamespace="hony850";
constexpr const char* kProfilesKey="chset1";
constexpr const char* kSelectedKey="chsel1";
}
ChannelStore::ChannelStore():selected_(0),loaded_(false){defaults();}
void ChannelStore::defaults(){
  const Preset standard[5]={{250,100},{180,60},{230,80},{280,100},{300,100}};
  for(uint8_t i=0;i<Cfg::CHANNELS;i++)
    presets_[i]=standard[i%5];
}
uint8_t ChannelStore::checksum(const Blob& b){
  const uint8_t *bytes=reinterpret_cast<const uint8_t*>(&b);
  uint8_t c=0xA5;
  for(size_t i=0;i<offsetof(Blob,checksum);i++){
    c^=bytes[i];
    c=(uint8_t)((c<<1)|(c>>7));
  }
  return c;
}
bool ChannelStore::valid(const Preset& p){
  return p.tempC>=Cfg::TEMP_MIN && p.tempC<=Cfg::TEMP_MAX && p.air<=100;
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
  uint8_t saved=prefs.getUChar(kSelectedKey,0);
  if(saved<Cfg::CHANNELS)selected_=saved;
  prefs.end();
}
bool ChannelStore::select(uint8_t index){
  if(index>=Cfg::CHANNELS)return false;
  if(selected_==index)return true;
  selected_=index; // elegir en RAM aun si la flash es inaccesible
  Preferences prefs;
  if(!prefs.begin(kNamespace,false))return false;
  bool ok=prefs.putUChar(kSelectedKey,selected_)==1;
  prefs.end();
  return ok;
}
bool ChannelStore::saveCurrent(int tempC,int air){
  const Preset next={(uint16_t)tempC,(uint8_t)air};
  if(tempC<Cfg::TEMP_MIN || tempC>Cfg::TEMP_MAX ||
     air<0 || air>100 || !valid(next))return false;
  Blob blob={};
  blob.magic=kMagic;blob.version=kVersion;blob.count=Cfg::CHANNELS;
  // Preparar copia completa: las modificaciones no se activan en RAM
  // hasta que Preferences confirme que fueron persistidas.
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
