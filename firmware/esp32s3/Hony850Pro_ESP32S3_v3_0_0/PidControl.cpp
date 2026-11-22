#include "PidControl.h"
#include "Config.h"
PidControl::PidControl():gains_({1.994f,0.168f,1.707f}),integral_(0),lastMs_(0){}
void PidControl::reset(){integral_=0;lastMs_=0;}
uint8_t PidControl::update(float raw,float median,float rate,float targetRaw,uint8_t air,uint32_t now){
  if(lastMs_ && now-lastMs_<Cfg::PID_MS)return 255; // mantener mando anterior
  const float dt=lastMs_?(float)(now-lastMs_)/1000.0f:0.25f;
  lastMs_=now;
  if(isnan(median))median=raw;
  const float err=targetRaw-median;
  const float p=gains_.kp*err, d=-gains_.kd*rate;
  // Limitar potencia cuando el caudal SOLICITADO sea bajo, no implica medir aire.
  float cap=(float)air;
  if(rate>0.8f && err>0 && err<90){
    const float projected=err-2.0f*rate;
    float brake=projected<=0?0:(projected<=12?15:(projected<=25?30:(projected<=50?55:75)));
    if(brake<cap)cap=brake;
  }
  if(raw>=targetRaw || median>=targetRaw)cap=0;
  const float trial=p+integral_+d;
  if(!((err>0 && trial>=cap)||(err<0 && trial<=0))){
    integral_+=gains_.ki*err*dt;
    if(integral_<0)integral_=0;
    if(integral_>100)integral_=100;
  }
  float out=p+integral_+d;
  if(out<0)out=0;
  if(out>cap)out=cap;
  return (uint8_t)(out+0.5f);
}
