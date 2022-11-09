#include <Arduino.h>

#define Led1_pin 6
#define Led2_pin 7

#define S1_pin 2
#define S2_pin 3

uint8_t S1, S1_prev, S2, S2_prev;
uint8_t Led1,Led2;

int me1=0,me2=0, mes2=0, toogle=0;
void setup() {
  // put your setup code here, to run once:
  pinMode(Led1_pin,OUTPUT);
  pinMode(Led2_pin,OUTPUT);
  pinMode(S1_pin, INPUT);
  pinMode(S2_pin, INPUT);

  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  S1_prev=S1;
  S2_prev=S2;
  S1= digitalRead(S1_pin);
  S2= digitalRead(S2_pin);

  switch (me1)
  {
  case 0:
    if(!S1) me1=1;
    break;
  case 1:
    if(S1) me1=0;
    else{
      delay(2000);
      me1=2;
    }
    break;
  case 2:
    if(S1) me1=0;
    else{
      delay(2000);
      me1=1;
    }
    break; 
  }
  switch (mes2)
  {
  case 0:
    if(S2_prev>S2){
      mes2=1;
      toogle=1;
    }
    break;
  
  case 1:
    if(S2_prev<S2) mes2=0;
    break;
  }
//maquina de estados led2
  switch (me2)
  {
  case 0:
    if(toogle==1){ 
      me2=1;
      toogle=0;
    }
    break;
  case 1:
    if(toogle==1) me2=0;
    else{
      delay(800);
      me2=2;
    }
    break;
  case 2:
    if(toogle==1) me2=0;
    else{
      delay(800);
      me2=1;
    }
    break; 
  }


  if(me1==1) digitalWrite(Led1_pin,HIGH);
  else digitalWrite(Led1_pin,LOW);
  if(me2==1) digitalWrite(Led2_pin,HIGH);
  else digitalWrite(Led2_pin,LOW);
}