#include <Arduino.h>

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
uint8_t s1, prevs1;
uint8_t s2, prevs2;

// Output variables
uint8_t LED_1, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7;

// Our finite state machines
fsm_t fsm_s1, fsm_s2, fsm_config0, fsm_config1, fsm_config2, fsm_config3, fsm_pc;

unsigned long interval, last_cycle;
unsigned long loop_micros;
bool cc = false;
bool cs = false;
bool cp = false;

// Set new state
void set_state(fsm_t & fsm, int new_state)
{
  if (fsm.state != new_state) {  // if the state changed tis is reset
    fsm.state = new_state;
    fsm.tes = millis();
    fsm.tis = 0;
  }
}

void update_tis()
{
  unsigned long cur_time = millis();   // Just one call to millis()
  fsm_s1.tis = cur_time - fsm_s1.tes;
  fsm_s2.tis = cur_time - fsm_s2.tes;
  fsm_config0.tis = cur_time - fsm_config0.tes;
  fsm_config1.tis = cur_time - fsm_config1.tes;
  fsm_config2.tis = cur_time - fsm_config2.tes;
  fsm_config3.tis = cur_time - fsm_config3.tes;
  fsm_pc.tis = cur_time - fsm_pc.tes;
}

void setup() 
{
  pinMode(LED1_pin, OUTPUT);
  pinMode(LED2_pin, OUTPUT);
  pinMode(LED3_pin, OUTPUT);
  pinMode(LED4_pin, OUTPUT);
  pinMode(LED5_pin, OUTPUT);
  pinMode(LED6_pin, OUTPUT);
  pinMode(LED7_pin, OUTPUT);
  pinMode(S1_pin, INPUT);
  pinMode(S2_pin, INPUT);

  // Start the serial port with 115200 baudrate
  Serial.begin(115200);

  interval = 10;
  set_state(fsm_s1, 0);
  set_state(fsm_s2, 0);    
  set_state(fsm_config0, 0);
  set_state(fsm_config1, 0);
  set_state(fsm_config2, 0);
  set_state(fsm_config3, 0);
  set_state(fsm_pc, 0);
}

void loop() 
{
    // To measure the time between loop() calls
    // unsigned long last_loop_micros = loop_micros; 
    
    // Do this only every "interval" miliseconds 
    // It helps to clear the switches bounce effect
    unsigned long now = millis();
    if (now - last_cycle > interval) {
      loop_micros = micros();
      last_cycle = now;
      
      // Read the inputs / inverse logic: buttom pressed equals zero
      prevs1 = s1;
      prevs2 = s2;
      s1 = digitalRead(S1_pin);
      s2 = digitalRead(S2_pin);

      // FSM processing

      // Update tis for all state machines
      update_tis();

      // Calculate next state for the 1st state machine
      if (fsm_s1.state == 0 && s1 < prevs1){
        fsm_s1.new_state = 1;
      } else if(fsm_s1.state == 1 && s1 > prevs1) {
        fsm_s1.new_state = 2;
      } else if(fsm_s1.state == 2 /* insert condition to finish resetting operation */) {
        fsm_s1.new_state = 0;
      } else if (fsm_s1.state == 1 && fsm_s1.tis >= 3000){
        fsm_s1.new_state = 3;
        cs = true;
      } else if (fsm_s1.state == 3 && s1 < prevs1){
        fsm_s1.state = 4;
        cc = true;
      } else if (fsm_s1.state == 4 && s1 > prevs1){
        fsm_s1.new_state = 5;
      } else if (fsm_s1.state == 4 && fsm_s1.tis >= 3000){
        fsm_s1.new_state = 0;
        cs = false;
      } else if (fsm_s1.state == 5 && cc == false){
        fsm_s1.new_state = 3;
      } 
      
      // Calculate next state for the 2nd state machine
      if (fsm_s2.state == 0 && s2 < prevs2){
        fsm_s2.new_state = 1;
      } else if(fsm_s2.state == 1 && fsm_s2.tis > 500) {
        fsm_s2.new_state = 0;
        cp = true;
      } else if(fsm_s2.state == 1 && s2 < prevs2) {
        fsm_s2.new_state = 2;
      } else if(fsm_s2.state == 2 && s2 > prevs2) {
        fsm_s2.new_state = 0;
      }

      // Calculate next state for the 3rd state machine
      if (fsm_config0.state == 0 && cc == 1){
        fsm_config0.new_state = 1;
        cc = 0;
      } else if (fsm_config0.state == 1 && cc == 1){
        fsm_config0.new_state = 2;
        cc = 0;
      } else if (fsm_config0.state == 2 && cc == 1){
        fsm_config0.new_state = 3;
        cc = 0;
      } else if (fsm_config0.state == 3 && cc == 1){
        fsm_config0.new_state = 0;
        cc = 0;
      }

      // Calculate next state for the 4th state machine
      if (fsm_config1.state == 0 && fsm_config0.state == 1 && cs == 1 && s2 < prevs2){
        fsm_config1.new_state = 1;
      } else if (fsm_config1.state == 1 && fsm_config0.state == 1 && cs == 1 && s2 < prevs2){
        fsm_config1.new_state = 2;
      } else if (fsm_config1.state == 2 && fsm_config0.state == 1 && cs == 1 && s2 < prevs2){
        fsm_config1.new_state = 3;
      } else if (fsm_config1.state == 3 && fsm_config0.state == 1 && cs == 1 && s2 < prevs2){
        fsm_config1.new_state = 0;
      }

      // Calculate next state for the 5th state machine
      if (fsm_config2.state == 0 && fsm_config0.state == 2 && cs == 1 && s2 < prevs2){
        fsm_config2.new_state = 1;
      } else if (fsm_config2.state == 1 && fsm_config0.state == 2 && cs == 1 && s2 < prevs2){
        fsm_config2.new_state = 2;
      } else if (fsm_config2.state == 2 && fsm_config0.state == 2 && cs == 1 && s2 < prevs2){
        fsm_config2.new_state = 0;
      }

      // Calculate next state for the 6th state machine
      if (fsm_config3.state == 0 && fsm_config0.state == 3 && cs == 1 && s2 < prevs2){
        fsm_config2.new_state = 1;
      } else if (fsm_config2.state == 1 && fsm_config0.state == 3 && cs == 1 && s2 < prevs2){
        fsm_config2.new_state = 0;
      }

      // Calculate next state for the 7th state machine
      if (fsm_pc.state == 0 && cp == 1){
        fsm_pc.new_state = 1;
        cp = 0;
      } else if (fsm_pc.state == 1 && cp == 1){
        fsm_pc.new_state = 0;
        cp = 0;
      }

      // Update the states
      set_state(fsm_s1, fsm_s1.new_state);
      set_state(fsm_s2, fsm_s2.new_state);
      set_state(fsm_config0, fsm_config0.new_state);
      set_state(fsm_config1, fsm_config1.new_state);
      set_state(fsm_config2, fsm_config2.new_state);
      set_state(fsm_config3, fsm_config3.new_state);
      set_state(fsm_pc, fsm_pc.new_state);

      // NOTHING UPDATED FROM HERE //
      // Actions set by the current state of the first state machine
      if (fsm1.state == 0){
        LED_1 = 0;
      } else if (fsm1.state == 1){
        LED_1 = 1;
      } else if (fsm1.state == 2){
        LED_1 = 0;
      }

      // A more compact way
      // LED_1 = (fsm1.state == 1);
      // LED_1 = (state == 1)||(state ==2);  if LED1 must be set in states 1 and 2
      
      // Actions set by the current state of the second state machine
      if (fsm2.state == 0){
        LED_2 = 0;
      } else if (fsm2.state == 1){
        LED_2 = 1;
      }

      // Set the outputs
      digitalWrite(LED1_pin, LED_1);
      digitalWrite(LED2_pin, LED_2);
      digitalWrite(LED3_pin, LED_3);
      digitalWrite(LED4_pin, LED_4);
      digitalWrite(LED5_pin, LED_5);
      digitalWrite(LED6_pin, LED_6);
      digitalWrite(LED7_pin, LED_7);

      // Debug using the serial port
      // Print buttons
      Serial.print("S1: ");
      Serial.print(s1);
      Serial.print(" S2: ");
      Serial.print(s2);

      // Print states for each fsm
      Serial.print(" fsm_s1.state: ");
      Serial.print(fsm_s1.state);
      Serial.print(" fsm_s2.state: ");
      Serial.print(fsm_s2.state);
      Serial.print(" fsm_config0.state: ");
      Serial.print(fsm_config0.state);
      Serial.print(" fsm_config1.state: ");
      Serial.print(fsm_config1.state);
      Serial.print(" fsm_config2.state: ");
      Serial.print(fsm_config2.state);
      Serial.print(" fsm_config3.state: ");
      Serial.print(fsm_config3.state);
      Serial.print(" fsm_pc.state: ");
      Serial.print(fsm_pc.state);

      // Print LEDs
      Serial.print(" LED_1: ");
      Serial.print(LED_1);
      Serial.print(" LED_2: ");
      Serial.print(LED_2);
      Serial.print(" LED_3: ");
      Serial.print(LED_3);
      Serial.print(" LED_4: ");
      Serial.print(LED_4);
      Serial.print(" LED_5: ");
      Serial.print(LED_5);
      Serial.print(" LED_6: ");
      Serial.print(LED_6);
      Serial.print(" LED_7: ");
      Serial.print(LED_7);

      // Print loop number
      Serial.print(" loop: ");
      Serial.println(micros() - loop_micros);
    }
    
}
