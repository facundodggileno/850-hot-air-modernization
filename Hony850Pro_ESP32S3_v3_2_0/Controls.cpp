#include "Controls.h"

Controls::Controls():
 btn_{{Cfg::TEMP_SW,true,true,false,false,0,0},
      {Cfg::AIR_SW,true,true,false,false,0,0}},
 oldTempClk_(HIGH),oldAirClk_(HIGH),standRaw_(true),standStable_(true),
 standChanged_(0),bothAt_(0){}

void Controls::begin(){
 const uint8_t pins[]={Cfg::TEMP_CLK,Cfg::TEMP_DT,Cfg::TEMP_SW,
                       Cfg::AIR_CLK,Cfg::AIR_DT,Cfg::AIR_SW};
 for(uint8_t i=0;i<sizeof(pins);i++)pinMode(pins[i],INPUT_PULLUP);
 oldTempClk_=digitalRead(Cfg::TEMP_CLK);
 oldAirClk_=digitalRead(Cfg::AIR_CLK);
 standRaw_=digitalRead(Cfg::STAND)==HIGH;
 standStable_=standRaw_;
 for(uint8_t i=0;i<2;i++){
   btn_[i].raw=digitalRead(btn_[i].pin);
   btn_[i].stable=btn_[i].raw;
 }
}

void Controls::pollButton(Button& b,uint32_t now,bool& shortEvent,
                          bool& longEvent,bool& hold10Event,bool isAir){
 const bool r=digitalRead(b.pin);
 if(r!=b.raw){b.raw=r;b.changed=now;}
 if((uint32_t)(now-b.changed)<Cfg::BUTTON_DEBOUNCE_MS)return;
 if(b.stable!=r){
   b.stable=r;
   if(!r){
     b.pressed=now;
     b.longSent=false;
     b.holdSent=false;
   }else{
     if(isAir){
       // Para AIR, esperar la liberacion antes de enviar AIR largo:
       // si se mantiene 10s no debe abrir el menu previamente a guardar.
       if(!b.holdSent){
         if((uint32_t)(now-b.pressed)>=Cfg::BUTTON_LONG_MS)
           longEvent=true;
         else shortEvent=true;
       }
     }else if(!b.longSent){
       shortEvent=true;
     }
   }
 }
 if(!b.stable){
   if(isAir){
     if(!b.holdSent &&
        (uint32_t)(now-b.pressed)>=Cfg::AIR_SAVE_HOLD_MS){
       b.holdSent=true;
       b.longSent=true;
       hold10Event=true;
     }
   }else if(!b.longSent &&
            (uint32_t)(now-b.pressed)>=Cfg::BUTTON_LONG_MS){
     b.longSent=true;longEvent=true;
   }
 }
}

InputFrame Controls::poll(uint32_t now){
 InputFrame e={};
 const uint8_t t=digitalRead(Cfg::TEMP_CLK);
 const uint8_t a=digitalRead(Cfg::AIR_CLK);
 if(t!=oldTempClk_ && t==LOW)
   e.tempStep=(digitalRead(Cfg::TEMP_DT)!=t)?1:-1;
 if(a!=oldAirClk_ && a==LOW)
   e.airStep=(digitalRead(Cfg::AIR_DT)!=a)?1:-1;
 oldTempClk_=t;oldAirClk_=a;
 bool tempHoldDummy=false;
 pollButton(btn_[0],now,e.tempShort,e.tempLong,tempHoldDummy,false);
 pollButton(btn_[1],now,e.airShort,e.airLong,e.airHold10,true);
 e.bothDown=(digitalRead(Cfg::TEMP_SW)==LOW && digitalRead(Cfg::AIR_SW)==LOW);
 if(e.bothDown){
   if(!bothAt_)bothAt_=now?now:1;
   e.bothLong=(uint32_t)(now-bothAt_)>=Cfg::ESTOP_HOLD_MS;
   e.tempShort=e.airShort=e.tempLong=e.airLong=e.airHold10=false;
 }else bothAt_=0;
 const bool sr=digitalRead(Cfg::STAND)==HIGH;
 if(sr!=standRaw_){standRaw_=sr;standChanged_=now;}
 if((uint32_t)(now-standChanged_)>=Cfg::STAND_DEBOUNCE_MS)
   standStable_=standRaw_;
 e.standRaw=standRaw_;
 e.standPressed=standRaw_||standStable_; // respuesta inmediata al apoyar
 return e;
}
