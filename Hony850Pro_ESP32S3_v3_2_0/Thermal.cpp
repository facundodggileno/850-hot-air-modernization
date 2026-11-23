#include "Thermal.h"
#include <math.h>
Thermal::Thermal():max_(Cfg::MAX_SCK,Cfg::MAX_CS,Cfg::MAX_SO),
  lastAt_(0),sampleSequence_(0),lastSamplePeriodMs_(0),readDurationUs_(0),previous1_(NAN),previous2_(NAN),spikeUp_(false),spikeDown_(false){}
void Thermal::tick(uint32_t now){
  // MAX6675 requiere hasta 220ms por conversion. Mantener 250ms
  // entre INICIOS de lectura, sin delay() ni bucles de espera.
  if(lastAt_ && (uint32_t)(now-lastAt_)<Cfg::TC_SAMPLE_MS)return;
  lastSamplePeriodMs_=lastAt_?(uint32_t)(now-lastAt_):0;
  const float dt=lastAt_?(float)(now-lastAt_)/1000.0f:0.25f;
  lastAt_=now;
  const uint32_t startUs=micros();
  const float t=max_.readCelsius();
  readDurationUs_=(uint32_t)(micros()-startUs);
  ++sampleSequence_;
  // El estado anterior de deteccion de picos nunca debe contaminar
  // una lectura invalida o nueva conversion.
  spikeUp_=false;spikeDown_=false;
  const bool good=(t>=HONY_INTERNAL_SENSOR_VALID_MIN_C && t<=HONY_INTERNAL_SENSOR_VALID_MAX_C); // NaN/INF tambien quedan rechazados.
  const float prev=s_.lastGood;
  s_.raw=t;s_.lastAccepted=good;
  if(!good){
    s_.valid=false;s_.consecutive=0;s_.controlled=NAN;s_.rate=0;
    s_.outletEst=NAN;s_.outletControlled=NAN;s_.outletRate=0;
    previous1_=NAN;previous2_=NAN; return;
  }
  if(s_.valid && !isnan(prev) && dt>=0.20f && dt<=0.5f){
    spikeUp_=(t-prev)>HONY_TC_RISE_SPIKE_C;
    spikeDown_=(prev-t)>HONY_TC_DROP_SPIKE_C;
  }
  s_.lastGood=t;s_.lastGoodAt=now;
  s_.outletEst=Cfg::outletEstimate(t);
  if(s_.consecutive<3)s_.consecutive++;
  s_.valid=(s_.consecutive>=3);
  if(isnan(previous1_) || isnan(previous2_))s_.controlled=t;
  else{
    const float lo=previous1_<previous2_?previous1_:previous2_;
    const float hi=previous1_>previous2_?previous1_:previous2_;
    s_.controlled=t<lo?lo:(t>hi?hi:t);
  }
  s_.outletControlled=Cfg::outletEstimate(s_.controlled);
  if(dt>0.05f && !isnan(prev)){
    const float r=(t-prev)/dt;
    s_.rate=0.7f*s_.rate+0.3f*(r < -60?-60:(r>60?60:r));
  }else s_.rate=0;
  if(dt>0.05f && !isnan(prev)){
    const float estimatedPrev=Cfg::outletEstimate(prev);
    const float instantaneous=(s_.outletEst-estimatedPrev)/dt;
    const float limited=instantaneous<-60?-60:(instantaneous>60?60:instantaneous);
    s_.outletRate=0.7f*s_.outletRate+0.3f*limited;
  }else s_.outletRate=0;
  previous2_=previous1_; previous1_=t;
}
bool Thermal::fresh(uint32_t now)const{
  return s_.valid && (uint32_t)(now-s_.lastGoodAt)<=Cfg::TC_STALE_MS;
}
