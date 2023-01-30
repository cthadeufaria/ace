#include <Wire.h>
#include "RTClib.h"
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define S1_pin 2
#define S2_pin 3

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 32

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

RTC_DS3231 rtc;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

char daysOfTheWeek[7][12] = {"Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sábado"};

// tes - time entering state / tis - time in state
typedef struct {
  int state, new_state;
  unsigned long tes, tis;
} fsm_t;

// input
uint8_t s1, prevS1;
uint8_t s2, prevS2;
uint8_t startPeriod = 8;
uint8_t endPeriod = 19;

// output
uint8_t LED_1, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7;

// Finite state machines
fsm_t fsmAdjustDatetime, fsmAdjustVariables, fsmControl, fsmSettings, fsmS1, fsmS2;

// variables
unsigned long interval, last_cycle, loop_micros;

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
  fsmSettings.tis = cur_time - fsmSettings.tes;
  fsmAdjustVariables.tis = cur_time - fsmAdjustVariables.tes;
  fsmS1.tis = cur_time - fsmS1.tes;
  fsmS2.tis = cur_time - fsmS2.tes;
}

void getRTC() {
  if(! rtc.begin()) {
    Serial.println("DS3231 não encontrado");
    // while(1);
    }

  if(rtc.begin()){
    Serial.println("DS3231 OK!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  delay(2500);
}

void getI2C() 
{
    byte error, address;
    int nDevices;

    nDevices = 0;

    while (!Serial) {
      Serial.println("\nI2C Scanner");
    }

    Serial.println("Scanning...");

    for (address = 1; address < 127; address++ ) {
      // The i2c_scanner uses the return value of
      // the Write.endTransmisstion to see if
      // a device did acknowledge to the address.
      Wire.beginTransmission(address);
      error = Wire.endTransmission();

      if (error == 0) {
        Serial.print("I2C device found at address 0x");
        if (address < 16)
        Serial.print("0");
        Serial.print(address, HEX);
        Serial.print("   ");
        Serial.println(address, DEC);
        nDevices++;
      }
      else if (error == 4) {
        Serial.print("Unknown error at address 0x");
        if (address < 16)
        Serial.print("0");
        Serial.println(address, HEX);
      }
    }

    if (nDevices == 0) {
      Serial.println("No I2C devices found\n");
    }
    else {
      Serial.println("done");
      Serial.print ("Found ");
      Serial.print (nDevices, DEC);
      Serial.println (" device(s).");
    }

    delay(2500);
}

void setupOLED() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("Hello, world!");
  display.display(); 
}

void setup()
{
  Wire.begin();
  Wire.setClock(100000);     

  pinMode(S1_pin, INPUT);
  pinMode(S2_pin, INPUT);

  interval = 10;

  Serial.begin(9600);

  getI2C();
  getRTC();
  setupOLED();

  set_state(fsmS1, 1);
  set_state(fsmS2, 1);
  set_state(fsmAdjustDatetime, 0);
  set_state(fsmControl, 0);
  set_state(fsmSettings, 0);
  set_state(fsmAdjustVariables, 0);

  delay(10000);
}

void getCurrentDatetime(){
    DateTime now = rtc.now();
    int dia = now.day();
    int mes = now.month();
    int ano = now.year();
    int diaDaSemana = now.dayOfTheWeek();
    int hora = now.hour();
    int minuto = now.minute();
    int segundo = now.second();

    Serial.print("Data: ");
    Serial.print(dia, DEC); 
    Serial.print('/');
    Serial.print(mes, DEC);
    Serial.print('/');
    Serial.print(ano, DEC);
    Serial.print(" / Dia: ");
    Serial.print(daysOfTheWeek[diaDaSemana]);
    Serial.print(" / Horas: ");
    Serial.print(hora, DEC);
    Serial.print(':');
    Serial.print(minuto, DEC);
    Serial.print(':');
    Serial.print(segundo, DEC);
    Serial.println();
}

void updateControl (fsm_t & fsm) {
  switch (fsm.state)
  {
  case 0:
    if (s2 < prevS2) {
      fsm.new_state = 3;
    }
    break;
  case 3:
    if (fsm.tis >= 3000 && (fsm.tis - fsmS2.tis <= 10)) {
      fsm.new_state = 4;
    }
    else if (fsm.tis - fsmS2.tis > 10) {
      fsm.new_state = 0;
    }
    break;
  case 4:
    if (s2 < prevS2) {
      fsm.new_state = 5;
    }
    break;
  case 5:
    if (fsm.tis >= 3000 && (fsm.tis - fsmS2.tis <= 10)) {
      fsm.new_state = 0;
    }
    else if (fsm.tis - fsmS2.tis > 10) {
      fsm.new_state = 4;
    }
    break;
  }
}

void updateSettings (fsm_t & fsm) {
  switch (fsm.state)
  {
  case 0:
    if (s1 < prevS1) {
      fsm.new_state = 1;
    }
    break;
  case 1:
    if (fsm.tis >= 3000 && (fsm.tis - fsmS1.tis <= 10)) {
      fsm.new_state = 2;
    }
    else if (fsm.tis - fsmS1.tis > 10) {
      fsm.new_state = 0;
    }
    break;
  case 2:
    if (s1 < prevS1) {
      fsm.new_state = 3;
    }
    break;
  case 3:
    if (fsm.tis >= 3000 && (fsm.tis - fsmS1.tis <= 10)) {
      fsm.new_state = 0;
    }
    else if (fsm.tis - fsmS1.tis > 10) {
      fsm.new_state = 2;
    }
    break;
  }
}

void updateAdjustDatetime(fsm_t & fsm) {
  switch (fsm.state)
  {
  case 0:
    if (fsmControl.new_state == 4 && fsmControl.state == 5) {
      fsm.new_state = 1;
    }
    break;
  case 1:
    if (fsmControl.new_state == 4 && fsmControl.state == 5) {
      fsm.new_state = 2;
    }
    break;
  case 2:
    if (fsmControl.new_state == 4 && fsmControl.state == 5) {
      fsm.new_state = 3;
    }
    break;
  case 3:
    if (fsmControl.new_state == 4 && fsmControl.state == 5) {
      fsm.new_state = 4;
    }
    break;
  case 4:
    if (fsmControl.new_state == 4 && fsmControl.state == 5) {
      fsm.new_state = 5;
    }
    break;
  case 5:
    if (fsmControl.new_state == 4 && fsmControl.state == 5) {
      fsm.new_state = 0;
    }
    break;  
  }
}

void updateAdjustVariables(fsm_t & fsm) {
  switch (fsm.state)
  {
  case 0:
    if (fsmControl.new_state == 4 && fsmControl.state == 5) {
      fsm.new_state = 1;
    }
    break;
  case 1:
    if (fsmControl.new_state == 4 && fsmControl.state == 5) {
      fsm.new_state = 0;
    }
    break;
  }
}

void adjustDatetime(fsm_t & fsm) {
    int dia=0, hora=0, minuto=0, segundo=0, operation=1; 
    TimeSpan ts;
    DateTime dt;
    DateTime now = rtc.now();
    
    switch (fsm.state)
    {
    case 0:
      if (fsmSettings.new_state == 0 && fsmSettings.state == 1) {
        dia = 365;
      }
      break;
    case 1:
      if (fsmSettings.new_state == 0 && fsmSettings.state == 1) {
        dia = 30;
      }
      break;
    case 2:
      if (fsmSettings.new_state == 0 && fsmSettings.state == 1) {
        dia = 1;
      }
      break;
    case 3:
      if (fsmSettings.new_state == 0 && fsmSettings.state == 1) {
        hora = 1;
      }
      break;
    case 4:
      if (fsmSettings.new_state == 0 && fsmSettings.state == 1) {
        minuto = 1;
      }
      break;
    case 5:
      if (fsmSettings.new_state == 0 && fsmSettings.state == 1) {
        segundo = 1;
      }
      break;
    }

    if (fsmSettings.new_state == 0 && fsmSettings.state == 1 && fsmSettings.tis >= 500) {
      operation = -1;
    }

    ts = TimeSpan(dia, hora, minuto, segundo);
    if (operation == 1) {
      dt = now + ts;
    }
    else if (operation == -1) {
      dt = now - ts;
    }
    rtc.adjust(dt);
}

void adjustVariables (fsm_t & fsm) {
  switch (fsm.state)
  {
  case 0:
    if (fsmSettings.new_state == 2 && fsmSettings.state == 3) {
      startPeriod += 1;
      if (startPeriod >= 24) {
        startPeriod = 0;
      } 
    }
    break;
  case 1:
    if (fsmSettings.new_state == 2 && fsmSettings.state == 3) {
      endPeriod += 1;
      if (endPeriod >= 24) {
        endPeriod = 0;
      } 
    }
    break;
  }
}

void debug() {
  // DEBUGGING:
  Serial.print("startPeriod: ");    
  Serial.print(startPeriod);
  Serial.println();
  Serial.print("startPeriod: ");    
  Serial.print(endPeriod);
  Serial.println();
  Serial.print("Serial.available(): ");    
  Serial.print(Serial.available());
  Serial.println();
  Serial.print("fsmControl.state: ");    
  Serial.print(fsmControl.state);
  Serial.println();
  Serial.print("fsmSettings.state: ");    
  Serial.print(fsmSettings.state);
  Serial.println();
  Serial.print("fsmAdjustDatetime.state: ");    
  Serial.print(fsmAdjustDatetime.state);
  Serial.println();
  Serial.println();
  Serial.println();
}

void loop()
{
    unsigned long now = millis();
    if (now - last_cycle > interval) {
      loop_micros = micros();
      last_cycle = now;

      prevS1 = s1;
      prevS2 = s2;
      s1 = digitalRead(S1_pin);
      s2 = digitalRead(S2_pin);

      update_tis();

      getCurrentDatetime();

      updateControl(fsmControl);

      if (fsmControl.state == 4 || fsmControl.state == 5) {
        updateSettings(fsmSettings);
        if (fsmSettings.state == 0 || fsmSettings.state == 1) {
            adjustDatetime(fsmAdjustDatetime);
            updateAdjustDatetime(fsmAdjustDatetime);
        } else if (fsmSettings.state == 2 || fsmSettings.state == 3) {
            adjustVariables(fsmAdjustVariables);
            updateAdjustVariables(fsmAdjustVariables);
        }
      }

      // actOLED();

      set_state(fsmS1, s1);
      set_state(fsmS2, s2);
      set_state(fsmAdjustDatetime, fsmAdjustDatetime.new_state);
      set_state(fsmAdjustVariables, fsmAdjustVariables.new_state);
      set_state(fsmControl, fsmControl.new_state);
      set_state(fsmSettings, fsmSettings.new_state);

      debug();
  }
}