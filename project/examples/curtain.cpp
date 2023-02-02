#include <Wire.h>
#include "RTClib.h"
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define Bup_pin 6
#define Bdown_pin 7
#define Sopen_pin 10
#define Sclose_pin 11
#define M_pin 2 
#define D_pin 3

// structure to define finite state machine
// tes - time entering state / tis - time in state
typedef struct {
  int state, new_state;
  unsigned long tes, tis;
} fsm_t;

unsigned long interval, last_cycle, loop_micros;

uint8_t d, m, start, hour;
uint8_t Sopen, Sclose, Bdown, Bup, Bdown_prev, Bup_prev;
fsm_t fsm_manual, fsm_automatic;

/*
1 motor desligado
2 descer manual
3 descer automatico
4 subir manual
5 subir automatico

sai do modo automatico se qualquer dos botoes for primido ou se chegar a algum dos sensores

*/
void Fmanual(fsm_t &manual){
    switch(manual.state){
        case 0:
            if (true) manual.new_state = 1;
            break;
        case 1:
            if(Bdown<Bdown_prev && !Sclose) manual.new_state=2;
            else if(Bup<Bup_prev && !Sopen) manual.new_state=4;
            break;
        case 2:
            if(manual.tis<500 && Bdown>Bdown_prev) manual.new_state=3;
            else if(Sclose || (manual.tis>500 && Bup>Bup_prev)) manual.new_state=1;
            break;
        case 3:
            if(Sclose || Bdown<Bdown_prev || Bup<Bup_prev) manual.new_state=1;
            break;
        case 4:
            if(manual.tis<500 && Bup>Bup_prev) manual.new_state=5;
            else if(Sopen || (manual.tis>500 && Bup>Bup_prev)) manual.new_state=1;
            break;
        case 5:
            if(Sopen || Bdown<Bdown_prev || Bup<Bup_prev) manual.new_state=1;
            break;
    }
    switch(manual.new_state){
        case 1:
            m=0;
            break;
        case 2:
            d=0;
            m=1;
            break;
        case 4:
            d=1;
            m=1;
            break;
    }
}


/*
1 espera
2 desce auto
3 sobe auto
*/
// void Fautomatico(fsm_t &auto, int hour_cl, int minuto_cl, int hour_op, int minuto_op ){
//     switch(auto.state){
//         case 1:
//         // horas
//             if(hour_cl == hora && minute_cl == minuto && !Sclose) auto.new_state=2;
//             else if(hour_op == hora && minute_op == minuto && !Sopen) auto.new_state=3;
//         //sensor
//             else if(hour_cl >= hora && minute_cl >= minuto && sensorL >= N_an && !Sclose) auto.new_state=2;
//             else if(hour_op <= hora && minute_op <= minuto && sensorL < N_an && !Sopen) auto.new_state=3;            
//             break;
//         case 2:
//             if(Sclose) auto.new_state=1;
//             break;
//         case 3:
//             if(Sopen) auto.new_state=1;
//             break;
//     }
//     switch(auto.new_state){
//         case 1:
//             m=0;
//             break;
//         case 2:
//             d=0;
//             m=1;
//             break;
//         case 3:
//             d=1;
//             m=1;
//             break;
//     }
// }

void variables(){
    
    //s1=DigitalRead(S1_pin);
    //s2=DigitalRead(S2_pin);
    Bup=digitalRead(Bup_pin);
    Bdown=digitalRead(Bdown_pin);
    
    Sopen=!digitalRead(Sopen_pin);
    Sclose=!digitalRead(Sclose_pin);
    //sensorL=analogRead(SL_pin);

    if(m==1) digitalWrite(M_pin, HIGH);
    else digitalWrite(M_pin, LOW);
    if(d==1) digitalWrite(D_pin, HIGH);
    else digitalWrite(D_pin, LOW);    
}

// Set new state
void set_state(fsm_t & fsm, int new_state)
{
  if (fsm.state != new_state) {  // if the state changed tis is reset
    fsm.state = new_state;
    fsm.tes = millis();
    fsm.tis = 0;
  }
}

void update_tis(){
  unsigned long cur_time = millis();   // Just one call to millis()
  //fsm_automatic.tis = cur_time - fsm_automatic.tes;
  fsm_manual.tis = cur_time - fsm_manual.tes;
}

void debug() {
  // DEBUGGING:
  Serial.print("Bup: ");
  Serial.print(Bup);
  Serial.println();
  Serial.print("Bdown: ");    
  Serial.print(Bdown);
  Serial.println();
  Serial.print("m pin: ");    
  Serial.print(m);
  Serial.println();
  Serial.print("d pin: ");    
  Serial.print(d);
  Serial.println();
  Serial.print("S open: ");    
  Serial.print(Sopen);
  Serial.println();
  Serial.print("S close: ");    
  Serial.print(Sclose);
  Serial.println();
  Serial.print("fsm manual: ");    
  Serial.print(fsm_manual.state);
  Serial.println();
  Serial.println();
  Serial.println();
}

void setup() 
{
  Bup=1;
  Bdown=1;
  d=0;
  m=0;

  interval = 10;

  Serial.begin(9600);
  
  pinMode(Bup_pin,INPUT);
  pinMode(Bdown_pin,INPUT);
  pinMode(Sopen_pin,INPUT);
  pinMode(Sclose_pin,INPUT);
  
  pinMode(M_pin,OUTPUT);
  pinMode(D_pin,OUTPUT);

  set_state(fsm_manual, 1);

  Serial.print("fsm_manual.state: ");  
  Serial.print(fsm_manual.state);
  delay(10000);

}

void loop() 
{
  unsigned long now = millis();
    if (now - last_cycle > interval) {
      loop_micros = micros();
      last_cycle = now;
    
    Bup_prev=Bup;
    Bdown_prev=Bdown;
    variables();
    Fmanual(fsm_manual);
    set_state(fsm_manual, fsm_manual.new_state);
    debug();
    }
}