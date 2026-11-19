#include "Display.h"
#include "Config.h"
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <math.h>
bool Display::begin(){
 ready_=(lcd_.begin(16,2)==0);
 if(ready_){lcd_.backlight();line(0,"HONY 850 PRO");line(1,"MODULAR 2.0");}
 return ready_;
}
void Display::line(uint8_t r,const char* s){
 if(!ready_)return;
 char b[17];memset(b,' ',16);b[16]=0;
 uint8_t n=strlen(s);if(n>16)n=16;
 memcpy(b,s,n);lcd_.setCursor(0,r);lcd_.print(b);
}
void Display::rawLine(char* b,const SensorSample& s,bool showCount){
 if(isnan(s.raw) || isinf(s.raw)){
   if(showCount)snprintf_P(b,17,PSTR("RAW:ERR %u/3"),(unsigned)s.consecutive);
   else if(isnan(s.lastGood))strcpy_P(b,PSTR("RAW:ERR LG:---"));
   else snprintf_P(b,17,PSTR("RAW:ERR LG:%3d"),(int)s.lastGood);
   return;
 }
 const int q=(int)(s.raw*4.0f+0.5f);
 if(showCount)snprintf_P(b,17,PSTR("RAW:%3d.%02u %u/3"),q/4,(unsigned)(q%4)*25U,(unsigned)s.consecutive);
 else snprintf_P(b,17,PSTR("RAW:%3d.%02u %s"),q/4,(unsigned)(q%4)*25U,s.lastAccepted?"OK":"BAD");
}
void Display::render(const Screen& s){
 if(!ready_)return;
 char a[17],b[17];a[0]=0;b[0]=0;
 if(s.boot!=Boot::Stage::Done){
   if(s.boot==Boot::Stage::Warnings){
     const uint8_t w=s.warning;
     strcpy_P(a,PSTR("BOOT ADVERTENCIA"));
     switch(w){
       case Boot::LCD:strcpy_P(b,PSTR("LCD FALLA"));break;
       case Boot::RAM:strcpy_P(b,PSTR("RAM BAJA"));break;
       case Boot::TC:strcpy_P(b,PSTR("MAX SIN LECTURA"));break;
       case Boot::ZC:strcpy_P(b,PSTR("CRUCE CERO MAL"));break;
       case Boot::WDT:strcpy_P(b,PSTR("RESET WATCHDOG"));break;
       default:strcpy_P(b,PSTR("AVISO"));break;
     }
   }else{
     snprintf_P(a,17,PSTR("BOOT %u/2"),s.boot==Boot::Stage::Sensor?1:2);
     rawLine(b,*s.sensor,true);
   }
 }else if(s.view==View::Fault || s.fault!=Fault::None){
   snprintf_P(a,17,PSTR("FAULT %02u OFF"),(unsigned)s.fault);
   rawLine(b,*s.sensor,false);
 }else if(s.view==View::PidMenu){
   strcpy_P(a,PSTR(" VER PID ACTUAL"));
   strcpy_P(b,PSTR(" HACER AUTOTUNE"));
   if(s.pidMenuSelection==0)a[0]='>';
   else b[0]='>';
 }else if(s.view==View::TuneSetup){
   // Encoder AIR esta a la izquierda: mostrar AIR primero, TEMP despues.
   snprintf_P(a,17,PSTR("AIR:%3d T:%3dC"),s.tuneAir,s.tuneSet);
   strcpy_P(b,PSTR("TEMP=OK AIR=ATR"));
 }else if(s.view==View::Tuning){
   snprintf_P(a,17,PSTR("PID %u/3 %s"),(unsigned)s.cycles,s.tuneHeating?"HEAT":"REST");
   if(s.sensor->valid)snprintf_P(b,17,PSTR("A:%3d T:%3d/%3d"),s.tuneAir,Cfg::corrected(s.sensor->raw),s.tuneSet);
   else rawLine(b,*s.sensor,true);
 }else if(s.view==View::PidView){
   if(s.tuneStatus==TuneStatus::Invalid || s.tuneStatus==TuneStatus::Timeout){
     strcpy_P(a,PSTR("PID TUNE FALLIDO"));
     strcpy_P(b,PSTR("T=SALIR A=ATR"));
   }else{
     const float v=s.pidPage==0?s.gains.kp:(s.pidPage==1?s.gains.ki:s.gains.kd);
     const uint32_t n=(uint32_t)(v*1000+0.5f);
     const char* label=s.pidPage==0?"KP":(s.pidPage==1?"KI":"KD");
     snprintf_P(a,17,PSTR("PID %s %u.%03u"),label,(unsigned)(n/1000),(unsigned)(n%1000));
     strcpy_P(b,s.loadedPid?PSTR("EEPROM T=ATR"):PSTR("DEFECTO T=ATR"));
   }
 }else{
   if(s.sensor->valid)snprintf_P(a,17,PSTR("T:%3d/%3d A%3d"),Cfg::corrected(s.sensor->raw),s.setpoint,s.air);
   else snprintf_P(a,17,PSTR("T:---/%3d A%3d"),s.setpoint,s.air);
   if(s.preflow)snprintf_P(b,17,PSTR("PRE-FLOW %us"),(unsigned)s.preflowSeconds);
   else if(s.heatRequest)snprintf_P(b,17,PSTR("HEAT %3u%%"),(unsigned)s.duty);
   else if(s.cooldown)strcpy_P(b,PSTR("COOLDOWN AIR100"));
   else strcpy_P(b,PSTR("HEAT OFF"));
 }
 line(0,a);line(1,b);
}
