#include <Wire.h>
#include "RTClib.h"
#include <Arduino.h>
#include <SPI.h>
// #include <avr/io.h>
// #include <util/delay.h>
// #include "timer_tools.h"
// // #include "printf_tools.h"

RTC_DS3231 rtc; //OBJETO DO TIPO RTC_DS3231

//DECLARAÇÃO DOS DIAS DA SEMANA
char daysOfTheWeek[7][12] = {"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sábado"};

#define LED1_pin 6
#define LED2_pin 7
#define LED3_pin 8
#define LED4_pin 9
#define LED5_pin 10
#define LED6_pin 11
#define LED7_pin 12

#define S1_pin 2
#define S2_pin 3
// structure to define finite state machine
// tes - time entering state / tis - time in state

typedef struct {
  int state, new_state;
  unsigned long tes, tis;
} fsm_t;

// Input variables
uint8_t s1, prevS1;
uint8_t s2, prevS2;

// Output variables
uint8_t LED_1, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7;

// Finite state machines
fsm_t fsmAdjustDatetime, fsmControl, fsmS1, fsmS2;

unsigned long interval, last_cycle;
unsigned long loop_micros;
unsigned long T,periodo_led, Tmod2, blink, lum;
uint8_t pause, cs, S1_click, S2_click, S2_double, point;

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
  fsmAdjustDatetime.tis = cur_time - fsmAdjustDatetime.tes;
  fsmControl.tis = cur_time - fsmControl.tes;
  fsmS1.tis = cur_time - fsmS1.tes;
  fsmS2.tis = cur_time - fsmS2.tes;
}

void setup()
{
    Wire.begin(); // Wire communication begin
    Wire.setClock(100000);
    Serial.begin(9600); // The baudrate of Serial monitor is set in 9600
    while (!Serial); // Waiting for Serial Monitor
    Serial.println("\nI2C Scanner");

    if(! rtc.begin()) { // SE O RTC NÃO FOR INICIALIZADO, FAZ
    Serial.println("DS3231 não encontrado"); //IMPRIME O TEXTO NO MONITOR SERIAL
    while(1); //SEMPRE ENTRE NO LOOP
    }
    if(rtc.lostPower()){ //SE RTC FOI LIGADO PELA PRIMEIRA VEZ / FICOU SEM ENERGIA / ESGOTOU A BATERIA, FAZ
    Serial.println("DS3231 OK!"); //IMPRIME O TEXTO NO MONITOR SERIAL
    //REMOVA O COMENTÁRIO DE UMA DAS LINHAS ABAIXO PARA INSERIR AS INFORMAÇÕES ATUALIZADAS EM SEU RTC
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //CAPTURA A DATA E HORA EM QUE O SKETCH É COMPILADO
    //rtc.adjust(DateTime(2018, 9, 29, 15, 00, 45)); //(ANO), (MÊS), (DIA), (HORA), (MINUTOS), (SEGUNDOS)
    }
    // delay(100); //INTERVALO DE 100 MILISSEGUNDOS
}

void getCurrentDatetime(){
    DateTime now = rtc.now(); //CHAMADA DE FUNÇÃO
    int dia = now.day();
    int mes = now.month();
    int ano = now.year();
    int diaDaSemana = now.dayOfTheWeek();
    int hora = now.hour();
    int minuto = now.minute();
    int segundo = now.second();

    Serial.print("Data: "); //IMPRIME O TEXTO NO MONITOR SERIAL
    Serial.print(dia, DEC); //IMPRIME NO MONITOR SERIAL O DIA
    Serial.print('/'); //IMPRIME O CARACTERE NO MONITOR SERIAL
    Serial.print(mes, DEC); //IMPRIME NO MONITOR SERIAL O MÊS
    Serial.print('/'); //IMPRIME O CARACTERE NO MONITOR SERIAL
    Serial.print(ano, DEC); //IMPRIME NO MONITOR SERIAL O ANO
    Serial.print(" / Dia: "); //IMPRIME O TEXTO NA SERIAL
    Serial.print(daysOfTheWeek[diaDaSemana]); //IMPRIME NO MONITOR SERIAL O DIA
    Serial.print(" / Horas: "); //IMPRIME O TEXTO NA SERIAL
    Serial.print(hora, DEC); //IMPRIME NO MONITOR SERIAL A HORA
    Serial.print(':'); //IMPRIME O CARACTERE NO MONITOR SERIAL
    Serial.print(minuto, DEC); //IMPRIME NO MONITOR SERIAL OS MINUTOS
    Serial.print(':'); //IMPRIME O CARACTERE NO MONITOR SERIAL
    Serial.print(segundo, DEC); //IMPRIME NO MONITOR SERIAL OS SEGUNDOS
    Serial.println(); //QUEBRA DE LINHA NA SERIAL
}

void updateControl (fsm_t & fsm) {
  switch (fsm.state)
  {
  case 0:
    if (s2 < prevS2 && fsmS2.tis >= 3000) {
      fsm.new_state = 3;
    }
    break;
  }
}

void updateAdjustDatetime(fsm_t & fsm, fsm_t & fsmControl){
  if (fsmControl.state == 3) {
    switch (fsm.state)
    {
    case 0:
      if (s2 < prevS2) {
        fsm.new_state = 1;
      }
      break;
    case 1:
      if (s2 < prevS2) {
        fsm.new_state = 2;
      }
      break;
    case 2:
      if (s2 < prevS2) {
        fsm.new_state = 3;
      }
      break;
    case 3:
      if (s2 < prevS2) {
        fsm.new_state = 4;
      }
      break;
    case 4:
      if (s2 < prevS2) {
        fsm.new_state = 5;
      }
      break;
    case 5:
      if (s2 < prevS2) {
        fsm.new_state = 6;
      }
      break;  
    }
  }
}

void adjustDatetime(fsm_t & fsm){
    DateTime now = rtc.now();
    int dia = now.day();
    int mes = now.month();
    int ano = now.year();
    int hora = now.hour();
    int minuto = now.minute();
    int segundo = now.second();

    switch (fsm.state)
    {
    case 0:
      if (s1 < prevS1) {
        ano += 1;
      }
    case 1:
      if (s1 < prevS1) {
        mes += 1;
      }
    case 2:
      if (s1 < prevS1) {
        dia += 1;
      }
    case 3:
      if (s1 < prevS1) {
        hora += 1;
      }
    case 4:
      if (s1 < prevS1) {
        minuto += 1;
      }
    case 5:
      if (s1 < prevS1) {
        segundo += 1;
      }
    }

    rtc.adjust(DateTime(ano, mes, dia, hora, minuto, segundo));
}

void loop()
{
    byte error, address; //variable for error and I2C address
    int nDevices;

    Serial.println("Scanning...");

    // Read the inputs / inverse logic: buttom pressed equals zero
    prevS1 = s1;
    prevS2 = s2;
    s1 = digitalRead(S1_pin);
    s2 = digitalRead(S2_pin);

    nDevices = 0;
    for (address = 1; address < 127; address++ )
    {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
        Serial.print("I2C device found at address 0x");
        if (address < 16)
        Serial.print("0");
        Serial.print(address, HEX);
        Serial.println("  !");
        nDevices++;
    }
    else if (error == 4)
    {
        Serial.print("Unknown error at address 0x");
        if (address < 16)
        Serial.print("0");
        Serial.println(address, HEX);
    }
    }
    if (nDevices == 0)
    Serial.println("No I2C devices found\n");
    else
    Serial.println("done\n");

    getCurrentDatetime();

    if (fsmControl.state == 3) {
        adjustDatetime(fsmAdjustDatetime);
    };

    // delay(5000); // wait 5 seconds

    update_tis();

    updateAdjustDatetime(fsmAdjustDatetime, fsmControl);
    updateControl(fsmControl);

    set_state(fsmS1, fsmS1.new_state);
    set_state(fsmS2, fsmS2.new_state);
    set_state(fsmAdjustDatetime, fsmAdjustDatetime.new_state);
    set_state(fsmControl, fsmControl.new_state);


    // DEBUGGING:
    Serial.print("fsmControl.state: ");    
    Serial.print(fsmControl.state);
    Serial.println();
    Serial.print("fsmControl.new_state: ");    
    Serial.print(fsmControl.new_state);
    Serial.println();
    Serial.print("s1: ");
    Serial.print(s1);
    Serial.println();
    Serial.print("s2: ");
    Serial.print(s2);
    Serial.println();
    Serial.print("prevS1: ");
    Serial.print(prevS1);
    Serial.println();
    Serial.print("prevS2: ");
    Serial.print(prevS2);
    Serial.println();
    Serial.println();
    Serial.print("fsmAdjustDatetime.state: ");    
    Serial.print(fsmAdjustDatetime.state);
    Serial.println();

}