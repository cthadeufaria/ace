#include <Wire.h>
#include "RTClib.h"
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define S1_pin 6
#define S2_pin 7
// #define S3_pin 8
// #define S4_pin 9

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
uint8_t s1, prevS1, s2, prevS2, k = 0;
DateTime startPeriod = DateTime(0, 8, 0, 0), endPeriod = DateTime(0, 19, 0, 0); 
DateTime startAuto = DateTime(0, 8, 0, 0), endAuto = DateTime(0, 19, 0, 0);

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

void actOLED(DateTime &d, int c, int l, int k) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(c, l);

  // Display static text
  if (k == 0) {
    display.println("Updated datetime now: ");
    Serial.println("Updated datetime now: ");
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

// void adjustVariables (fsm_t & fsm) {
//   switch (fsm.state)
//   {
//   case 0:
//     if (fsmSettings.new_state == 2 && fsmSettings.state == 3) {
//       startPeriod += 1;
//       if (startPeriod >= 24) {
//         startPeriod = 0;
//       } 
//     }
//     break;
//   case 1:
//     if (fsmSettings.new_state == 2 && fsmSettings.state == 3) {
//       endPeriod += 1;
//       if (endPeriod >= 24) {
//         endPeriod = 0;
//       } 
//     }
//     break;
//   }
// }

void debug() {
  // DEBUGGING:
  Serial.print("startPeriod.year()");
  Serial.print(startPeriod.year());
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
      k = 0;

      prevS1 = s1;
      prevS2 = s2;
      s1 = digitalRead(S1_pin);
      s2 = digitalRead(S2_pin);

      update_tis();

      getCurrentDatetime(date);

      updateControl(fsmControl);

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

      debug();
  }
}