#include "PidControl.h"
#include "Config.h"
PidControl::PidControl():gains_({1.994f,0.168f,1.707f}),integral_(0),lastMs_(0),lastDtMs_(0){}
void PidControl::reset(){integral_=0;lastMs_=0;lastDtMs_=0;}
uint8_t PidControl::update(float outlet,float smoothed,float rate,
                           float targetOutlet,uint8_t air,uint32_t now,
                           bool fastHeat){
  const uint32_t elapsed=lastMs_?(uint32_t)(now-lastMs_):Cfg::PID_MS;
  if(lastMs_ && elapsed<Cfg::PID_MS)return 255; // mantener mando anterior
  lastDtMs_=elapsed;
  lastMs_=now;
  // Tiempo real transcurrido: evita asumir que loop() es exacto.
  // Limitar integracion tras paradas largas para no dar saltos de potencia.
  const float dt=(elapsed>750UL?750UL:elapsed)/1000.0f;
  if(isnan(smoothed))smoothed=outlet;
  const float err=targetOutlet-smoothed;
  // FAST se aplica UNICAMENTE al calentamiento inicial desde App.
  // No supera el duty limitado por AIR. Safety permanece independiente.
  const float derivativeScale=fastHeat?HONY_FAST_HEAT_DERIVATIVE_FACTOR:1.0f;
  const float p=gains_.kp*err;
  const float d=-gains_.kd*rate*derivativeScale;
  float cap=(float)air;
  const float brakeZone=fastHeat?HONY_FAST_HEAT_BRAKE_START_C:90.0f;
  const float leadSeconds=fastHeat?HONY_FAST_HEAT_LOOKAHEAD_S:2.0f;
  if(rate>0.8f && err>0 && err<brakeZone){
    const float projected=err-leadSeconds*rate;
    float brake=projected<=0?0:(projected<=12?15:(projected<=25?30:(projected<=50?55:75)));
    if(brake<cap)cap=brake;
  }
  if(outlet>=targetOutlet || smoothed>=targetOutlet)cap=0;
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
