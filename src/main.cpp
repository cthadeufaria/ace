#include <Wire.h>
#include "RTClib.h"
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define S1_pin 6
#define S2_pin 7
#define Bup_pin 8
#define Bdown_pin 9
#define Sopen_pin 10
#define Sclose_pin 11
#define M_pin 2 
#define D_pin 3
#define SL_pin 28

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

RTC_DS3231 rtc;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DateTime date;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// tes - time entering state / tis - time in state
typedef struct {
  int state, new_state;
  unsigned long tes, tis;
} fsm_t;

// input
// DateTime(2018, 9, 29, 15, 00, 45); //(ANO), (MÊS), (DIA), (HORA), (MINUTOS), (SEGUNDOS)
uint8_t s1, prevS1, s2, prevS2, k = 0;
DateTime startPeriod, endPeriod, startAuto, endAuto;
uint8_t start, hour;
uint8_t Sopen, Sclose, Bdown, Bup, Bdown_prev, Bup_prev, sensorL, N_an;

// output
uint8_t d, m;

// Finite state machines
fsm_t fsmAdjustDatetime, fsmAdjustVariables, fsmControl, fsmSettings, fsmS1, fsmS2, fsm_manual, fsm_automatic;

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
  fsmAdjustDatetime.tis = cur_time - fsmAdjustDatetime.tes; // ok
  fsmControl.tis = cur_time - fsmControl.tes; // ok
  fsmSettings.tis = cur_time - fsmSettings.tes; // ok
  fsmAdjustVariables.tis = cur_time - fsmAdjustVariables.tes;
  fsmS1.tis = cur_time - fsmS1.tes;
  fsmS2.tis = cur_time - fsmS2.tes;
  fsm_manual.tis = cur_time - fsm_manual.tes;
  fsm_automatic.tis = cur_time - fsm_automatic.tes;
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
    // for(;;);
  }
  else if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    delay(2000);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 10);
    // Display static text

    display.println("SSD1306_SWITCHCAPVCC OLED DISPLAY SET");

    display.display(); 
  }
}

void variables(){
    prevS1 = s1;
    prevS2 = s2;
    Bup_prev = Bup;
    Bdown_prev = Bdown;

    s1 = digitalRead(S1_pin);
    s2 = digitalRead(S2_pin);
    Bup = digitalRead(Bup_pin);
    Bdown = digitalRead(Bdown_pin);    
    Sopen = !digitalRead(Sopen_pin);
    Sclose = !digitalRead(Sclose_pin);
    sensorL = analogRead(SL_pin);
    // sensorL = 0;

    if(m==1) digitalWrite(M_pin, HIGH);
    else digitalWrite(M_pin, LOW);
    if(d==1) digitalWrite(D_pin, HIGH);
    else digitalWrite(D_pin, LOW);    
}

void Fmanual(fsm_t &manual, fsm_t &control){
  if(manual.state > 0 && control.state > 1) manual.new_state = 0;
  switch(manual.state){
      case 0:
          if (control.state==0 || control.state==1) manual.new_state=1;
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

void Fautomatico (fsm_t &automatic, fsm_t &control, DateTime &open, DateTime &close, DateTime &dt) {
  int hora = dt.hour();
  int minuto = dt.minute();

  if(automatic.state > 0 && (control.state<2 || control.state>3)) automatic.new_state = 0;
  switch(automatic.state){
      case 0:
        if (control.state==2 || control.state==3) automatic.new_state=1;
        break;
      case 1:
      // horas
          if((close.hour() == hora && close.minute() == minuto) && !Sclose) automatic.new_state=2;
          else if((open.hour() == hora && open.minute() == minuto) && !Sopen) automatic.new_state=3;
      //sensor
          else if((close.hour() < hora) || (close.hour() == hora && close.minute() <= minuto) && (sensorL > N_an) && !Sclose) automatic.new_state=2;
          else if((open.hour() < hora) || (open.hour() == hora && open.minute() <= minuto) && (sensorL < N_an) && !Sopen) automatic.new_state=3;            
          break;
      case 2:
          if(Sclose) automatic.new_state=1;
          break;
      case 3:
          if(Sopen) automatic.new_state=1;
          break;
  }
  switch(automatic.new_state){
      case 1:
          m=0;
          break;
      case 2: // down
          d=0;
          m=1;
          break;
      case 3: // up
          d=1;
          m=1;
          break;
  }
}

void actOLED(DateTime &d, int c, int l, int k) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(c, l);

  // Display static text
  if (k == 0) {
    if (fsmControl.state == 0 || fsmControl.state == 1) {
    display.println("(Manual) Now: ");
    Serial.println("(Manual) Now: ");
    }
    else if (fsmControl.state == 2 || fsmControl.state == 3) {
    display.println("(Automatic) Now: ");
    Serial.println("(Automatic) Now: ");
    }
  }
  else if (k == 1) {
    display.println("Adjust datetime now: ");
    Serial.println("Adjust datetime now: ");
  }
  else if (k == 2) {
    if (fsmAdjustVariables.state == 1) {
      display.println("Auto close time: ");
      Serial.println("Auto close time: ");
    }
    else if (fsmAdjustVariables.state == 0) {
      display.println("Auto open time: ");
      Serial.println("Auto open time: ");
    }
  }
  else if (k == 3) {
    if (fsmAdjustVariables.state == 1) {
      display.println("Auto period end: ");
      Serial.println("Auto period end: ");
    }
    else if (fsmAdjustVariables.state == 0) {
      display.println("Auto period beggin: ");
      Serial.println("Auto period beggin: ");
    }
  }

  display.print(d.day(), DEC); 
  display.print('/');
  display.print(d.month(), DEC);
  display.print('/');
  display.println(d.year(), DEC);
  display.println(daysOfTheWeek[d.dayOfTheWeek()]);
  display.print(d.hour(), DEC);
  display.print(':');
  display.print(d.minute(), DEC);
  display.print(':');
  display.println(d.second(), DEC);

  Serial.print(d.day(), DEC); 
  Serial.print('/');
  Serial.print(d.month(), DEC);
  Serial.print('/');
  Serial.println(d.year(), DEC);
  Serial.println(daysOfTheWeek[d.dayOfTheWeek()]);
  Serial.print(d.hour(), DEC);
  Serial.print(':');
  Serial.print(d.minute(), DEC);
  Serial.print(':');
  Serial.println(d.second(), DEC);

  display.display();
}

void setup()
{
  Wire.begin();
  Wire.setClock(100000);     

  pinMode(S1_pin, INPUT);
  pinMode(S2_pin, INPUT);
  pinMode(Bup_pin, INPUT);
  pinMode(Bdown_pin, INPUT);
  pinMode(Sopen_pin, INPUT_PULLUP);
  pinMode(Sclose_pin, INPUT_PULLUP);
  pinMode(SL_pin, INPUT);
  pinMode(M_pin, OUTPUT);
  pinMode(D_pin, OUTPUT);

  interval = 10;
  Bup = 1;
  Bdown = 1;
  d = 0;
  m = 0;
  N_an = 70;

  startPeriod = DateTime(2022, 2, 2, 10, 0, 0); 
  endPeriod = DateTime(2022, 2, 2, 11, 0, 0); 
  startAuto = DateTime(2022, 2, 2, 10, 5, 0); 
  endAuto = DateTime(2022, 2, 2, 10, 6, 0);

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
  set_state(fsm_manual, 0);
  set_state(fsm_automatic, 0);

  Serial.print("fsm_manual.state: ");  
  Serial.print(fsm_manual.state);

  delay(3000);
}

void getCurrentDatetime(DateTime &now){
    now = rtc.now();
    int dia = now.day();
    int mes = now.month();
    int ano = now.year();
    int diaDaSemana = now.dayOfTheWeek();
    int hora = now.hour();
    int minuto = now.minute();
    int segundo = now.second();
}

void updateControl (fsm_t & fsm) {
  switch (fsm.state)
  {
  case 0:
    if (s2 < prevS2) {
      fsm.new_state = 1;
    }
    break;
  case 1:
    if (fsm.tis >= 3000 && (fsm.tis - fsmS2.tis <= 10)) {
      fsm.new_state = 2;
    }
    else if (fsm.tis - fsmS2.tis > 10) {
      fsm.new_state = 0;
    }
    break;
  case 2:
    if (s2 < prevS2) {
      fsm.new_state = 3;
    }
    break;
  case 3:
    if (fsm.tis >= 3000 && (fsm.tis - fsmS2.tis <= 10)) {
      fsm.new_state = 4;
    }
    else if (fsm.tis - fsmS2.tis > 10) {
      fsm.new_state = 2;
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
      fsm.new_state = 4;
    }
    else if (fsm.tis - fsmS1.tis > 10) {
      fsm.new_state = 2;
    }
    break;
  case 4:
    if (s1 < prevS1) {
      fsm.new_state = 5;
    }
    break;
  case 5:
    if (fsm.tis >= 3000 && (fsm.tis - fsmS1.tis <= 10)) {
      fsm.new_state = 0;
    }
    else if (fsm.tis - fsmS1.tis > 10) {
      fsm.new_state = 4;
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

void updateAdjustVariables (fsm_t & fsm) {
  switch (fsm.state)
  {
  case 0:
    if (fsmControl.new_state == 4 && fsmControl.state == 5 && fsmControl.tis >= 500) {
      fsm.new_state = 1;
    }
    break;
  case 1:
    if (fsmControl.new_state == 4 && fsmControl.state == 5 && fsmControl.tis >= 500) {
      fsm.new_state = 0;
    }
    break;
  }
}

void adjustDatetime(fsm_t &fsm, DateTime &date) {
    int dia=0, hora=0, minuto=0, segundo=0, operation=1; 
    TimeSpan ts;
    DateTime dt, now;

    if (fsmSettings.state == 0 || fsmSettings.state == 1) {
      now = rtc.now();
    }
    else if ((fsmSettings.state == 2 || fsmSettings.state == 3)) {
      if (fsmAdjustVariables.state == 1) {
        now = endAuto;
      }
      if (fsmAdjustVariables.state == 0) {
        now = startAuto;
      }
    }
    else if ((fsmSettings.state == 4 || fsmSettings.state == 5)) {
      if (fsmAdjustVariables.state == 1) {
        now = endPeriod;
      }
      if (fsmAdjustVariables.state == 0) {
        now = startPeriod;
      }
    }
    
    switch (fsm.state)
    {
    case 0:
      if (fsmSettings.new_state < fsmSettings.state) {
        dia = 365;
      }
      break;
    case 1:
      if (fsmSettings.new_state < fsmSettings.state) {
        dia = 30;
      }
      break;
    case 2:
      if (fsmSettings.new_state < fsmSettings.state) {
        dia = 1;
      }
      break;
    case 3:
      if (fsmSettings.new_state < fsmSettings.state) {
        hora = 1;
      }
      break;
    case 4:
      if (fsmSettings.new_state < fsmSettings.state) {
        minuto = 1;
      }
      break;
    case 5:
      if (fsmSettings.new_state < fsmSettings.state) {
        segundo = 1;
      }
      break;
    }

    if (fsmSettings.new_state < fsmSettings.state && fsmSettings.tis >= 500) {
      operation = -1;
    }

    ts = TimeSpan(dia, hora, minuto, segundo);

    if (operation == 1) {
      dt = now + ts;
    }
    else if (operation == -1) {
      dt = now - ts;
    }

    if (fsmSettings.state == 0 || fsmSettings.state == 1) {
      rtc.adjust(dt);
    }
    else if ((fsmSettings.state == 2 || fsmSettings.state == 3)) {
      if (fsmAdjustVariables.state == 1) {
        endAuto = dt;
      }
      if (fsmAdjustVariables.state == 0) {
        startAuto = dt;
      }
    }
    else if ((fsmSettings.state == 4 || fsmSettings.state == 5)) {
      if (fsmAdjustVariables.state == 1) {
        endPeriod = dt;
      }
      if (fsmAdjustVariables.state == 0) {
        startPeriod = dt;
      }
    }

    date = dt;
}

void debug() {
  // DEBUGGING:startAuto, endAuto, date
  Serial.print("fsm automatic: ");
  Serial.println(fsm_automatic.state);
  Serial.print("fsm manual: ");
  Serial.println(fsm_manual.state);
  Serial.print("startAuto.minute: ");
  Serial.println(startAuto.minute());
  Serial.print("endAuto.minute: ");
  Serial.println(endAuto.minute());
  Serial.print("date.minute: ");
  Serial.println(date.minute());
  Serial.print("N_an: ");
  Serial.println(N_an);
  Serial.print("sensorL: ");    
  Serial.println(sensorL);
  Serial.print("Bdown: ");    
  Serial.println(Bdown);
  Serial.print("Bup: ");    
  Serial.println(Bup);
  Serial.print("Sclose: ");    
  Serial.println(Sclose);
  Serial.print("Sopen: ");    
  Serial.println(Sopen);
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
      k = 0;

      variables();

      update_tis();

      getCurrentDatetime(date);

      updateControl(fsmControl);

      Fmanual(fsm_manual, fsmControl);
      Fautomatico(fsm_automatic, fsmControl, startAuto, endAuto, date);
      
      if (fsmControl.state == 4 || fsmControl.state == 5) {
        updateSettings(fsmSettings);
        updateAdjustDatetime(fsmAdjustDatetime);
        if (fsmSettings.state == 0 || fsmSettings.state == 1) {
            k = 1;
            adjustDatetime(fsmAdjustDatetime, date);
            // updateAdjustDatetime(fsmAdjustDatetime);
        } else if (fsmSettings.state == 2 || fsmSettings.state == 3) {
            k = 2;
            // adjustVariables(fsmAdjustVariables);
            adjustDatetime(fsmAdjustDatetime, date);
            updateAdjustVariables(fsmAdjustVariables);
        } else if (fsmSettings.state == 4 || fsmSettings.state == 5) {
            k = 3;
            // adjustVariables(fsmAdjustVariables);
            adjustDatetime(fsmAdjustDatetime, date);
            updateAdjustVariables(fsmAdjustVariables);
        }
      }

      actOLED(date, 0, 0, k);

      set_state(fsmS1, s1);
      set_state(fsmS2, s2);
      set_state(fsmAdjustDatetime, fsmAdjustDatetime.new_state);
      set_state(fsmAdjustVariables, fsmAdjustVariables.new_state);
      set_state(fsmControl, fsmControl.new_state);
      set_state(fsmSettings, fsmSettings.new_state);
      set_state(fsm_manual, fsm_manual.new_state);
      set_state(fsm_automatic, fsm_automatic.new_state);

      debug();
  }
}