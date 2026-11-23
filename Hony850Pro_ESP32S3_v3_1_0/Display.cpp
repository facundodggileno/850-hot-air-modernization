#include "Display.h"
#include "Config.h"
#include <stdio.h>
#include <string.h>

#include <math.h>
bool Display::begin(){
 ready_=(lcd_.begin(16,2)==0);
 if(ready_){lcd_.backlight();line(0,"HONY 850 PRO S3");line(1,"TEMP SALIDA EST");}
 return ready_;
}
void Display::line(uint8_t r,const char* s){
 if(!ready_)return;
 char b[17];memset(b,' ',16);b[16]=0;
 uint8_t n=strlen(s);if(n>16)n=16;
 memcpy(b,s,n);lcd_.setCursor(0,r);lcd_.print(b);
}
void Display::tempLine(char* b,const SensorSample& s,bool count){
 // La salida es una ESTIMACION calibrada, no una sonda fisica de boquilla.
 if(!s.valid || !isfinite(s.outletEst)){
   if(count)snprintf(b,17,"SAL:ERR %u/3",(unsigned)s.consecutive);
   else strcpy(b,"SAL:ERR TC");
   return;
 }
 if(count)snprintf(b,17,"SAL~%3dC %u/3",
                   (int)lroundf(s.outletEst),(unsigned)s.consecutive);
 else snprintf(b,17,"SAL~%3dC %s",
               (int)lroundf(s.outletEst),s.lastAccepted?"OK":"BAD");
}
void Display::render(const Screen& s){
 if(!ready_)return;
 char a[17],b[17];a[0]=0;b[0]=0;
 if(s.view==View::Debug && s.boot==Boot::Stage::Done){
  switch(s.debugPage){
  case 0:
    if(s.sensor->valid && isfinite(s.sensor->outletEst))
      snprintf(a,17,"EST~%5.1fC",s.sensor->outletEst);
    else strcpy(a,"EST~SIN DATO");
    if(isfinite(s.sensor->raw))
      snprintf(b,17,"RAW %5.1fC",s.sensor->raw);
    else strcpy(b,"RAW ERROR");
    break;
  case 1:
    snprintf(a,17,"ZC%4lu/s P%5lu",
             (unsigned long)s.zcAcceptedPerSec,
             (unsigned long)s.zcPeriodUs);
    snprintf(b,17,"REJ:%4lu G4:%u",
             (unsigned long)s.zcRejectedPerSec,s.zcInput?1:0);
    break;
  case 2:
    snprintf(a,17,"TC:%s N:%u/3",
             s.tcFresh?"OK":"ERR",(unsigned)s.sensor->consecutive);
    snprintf(b,17,"AGE:%lums %s",(unsigned long)s.tcAgeMs,
             s.sensor->lastAccepted?"LECT OK":"LECT ERR");
    break;
  case 3:
    snprintf(a,17,"PUMP:%3u H:%3u",
             (unsigned)s.pumpCmd,(unsigned)s.duty);
    snprintf(b,17,"REL:%u ST:%u ARM:%u",
             s.relayOn?1:0,s.standOn?1:0,(unsigned)s.arm);
    break;
  case 4:
    snprintf(a,17,"GATE ON:%4lu/s",(unsigned long)s.zcGateOnPerSec);
    snprintf(b,17,"GATE OFF:%3lu/s",(unsigned long)s.zcGateOffPerSec);
    break;
  case 5:
    snprintf(a,17,"FAULT:%u BOOT:%u",
             (unsigned)s.fault,(unsigned)s.boot);
    snprintf(b,17,"ZC:%s RST:%u",
             s.zcOK?"SI":"NO",(unsigned)s.resetReason);
    break;
  case 6:
    snprintf(a,17,"ZC TOTAL:%lu",(unsigned long)s.zcTotal);
    snprintf(b,17,"REJ TOTAL:%lu",(unsigned long)s.rejectedTotal);
    break;
  default:
    snprintf(a,17,"HEAP:%5luKB",
             (unsigned long)(s.freeHeapBytes/1024UL));
    snprintf(b,17,"MIN:%6luKB",
             (unsigned long)(s.minHeapBytes/1024UL));
    break;
  }
 }else if(s.boot!=Boot::Stage::Done){
   if(s.boot==Boot::Stage::Warnings){
     const uint8_t w=s.warning;
     strcpy(a,"BOOT ADVERTENCIA");
     switch(w){
       case Boot::LCD:strcpy(b,"LCD FALLA");break;
       case Boot::RAM:strcpy(b,"RAM BAJA");break;
       case Boot::TC:strcpy(b,"MAX SIN LECTURA");break;
       case Boot::ZC:strcpy(b,"CRUCE CERO MAL");break;
       case Boot::WDT:strcpy(b,"RESET WATCHDOG");break;
       default:strcpy(b,"AVISO");break;
     }
   }else{
     snprintf(a,17,"BOOT %u/2",s.boot==Boot::Stage::Sensor?1:2);
     tempLine(b,*s.sensor,true);
   }
 }else if(s.view==View::Fault || s.fault!=Fault::None){
   snprintf(a,17,"FAULT %02u OFF",(unsigned)s.fault);
   tempLine(b,*s.sensor,false);
 }else if(s.view==View::PidMenu){
   if(s.pidMenuSelection<2){
     strcpy(a," VER PID ACTUAL");strcpy(b," HACER AUTOTUNE");
     if(s.pidMenuSelection==0)a[0]='>';else b[0]='>';
   }else{
     strcpy(a," HACER AUTOTUNE");strcpy(b,">DIAGNOSTICO");
   }
 }else if(s.view==View::TuneSetup){
   // Encoder AIR esta a la izquierda: mostrar AIR primero, TEMP despues.
   snprintf(a,17,"AIR:%3d T:%3dC",s.tuneAir,s.tuneSet);
   strcpy(b,"TEMP=OK AIR=ATR");
 }else if(s.view==View::Tuning){
   snprintf(a,17,"PID %u/3 %s",(unsigned)s.cycles,s.tuneHeating?"HEAT":"REST");
   if(s.sensor->valid)snprintf(b,17,"A:%3d T:%3d/%3d",s.tuneAir,(int)lroundf(s.sensor->outletEst),s.tuneSet);
   else tempLine(b,*s.sensor,true);
 }else if(s.view==View::PidView){
   if(s.tuneStatus==TuneStatus::Invalid || s.tuneStatus==TuneStatus::Timeout){
     strcpy(a,"PID TUNE FALLIDO");
     strcpy(b,"T=SALIR A=ATR");
   }else{
     const float v=s.pidPage==0?s.gains.kp:(s.pidPage==1?s.gains.ki:s.gains.kd);
     const uint32_t n=(uint32_t)(v*1000+0.5f);
     const char* label=s.pidPage==0?"KP":(s.pidPage==1?"KI":"KD");
     snprintf(a,17,"PID %s %u.%03u",label,(unsigned)(n/1000),(unsigned)(n%1000));
     strcpy(b,s.loadedPid?"NVS T=ATR":"DEFECTO T=ATR");
   }
 }else{
   if(s.sensor->valid)snprintf(a,17,"S~:%3d/%3d A%3d",(int)lroundf(s.sensor->outletEst),s.setpoint,s.air);
   else snprintf(a,17,"S~:---/%3d A%3d",s.setpoint,s.air);
   if(s.preflow)snprintf(b,17,"PRE-FLOW %us",(unsigned)s.preflowSeconds);
   else if(s.heatRequest)snprintf(b,17,"HEAT %3u%%",(unsigned)s.duty);
   else if(s.cooldown)strcpy(b,"COOLDOWN AIR100");
   else strcpy(b,"HEAT OFF");
 }
 line(0,a);line(1,b);
}
