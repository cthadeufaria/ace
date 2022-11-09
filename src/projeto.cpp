#include <Arduino.h>

#define LED1_pin 6
#define S1_pin 2

// structure to define finite state machine
// tes - time entering state / tis - time in state
typedef struct {
  int state, new_state;
  unsigned long tes, tis;
} fsm_t;

// Input variables
uint8_t button, prev_button;

// Output variables
uint8_t motor_1, motor_2;

// Our finite state machines
fsm_t fsm_1;

// Declaring main variables
unsigned long interval, last_cycle;
unsigned long loop_micros;
uint8_t var;

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
  fsm_1.tis = cur_time - fsm_1.tes;
}

void setup() 
{
  pinMode(LED1_pin, OUTPUT);
  pinMode(S1_pin, INPUT);

  // Start the serial port with 115200 baudrate
  Serial.begin(115200);

  // Start variables
  interval = 10;
  var = 0;
  set_state(fsm_1, 0);
}

void act(fsm_t&fsm, int var){
    // do something
}

void loop() 
{
    // To measure the time between loop() calls
    unsigned long last_loop_micros = loop_micros; 
    
    // Do this only every "interval" miliseconds 
    // It helps to clear the switches bounce effect
    unsigned long now = millis();
    if (now - last_cycle > interval) {
      loop_micros = micros();
      last_cycle = now;
      
      
      
      // Read the inputs / inverse logic: buttom pressed equals zero
      prev_button = button;
      button = digitalRead(S1_pin);

      // FSM processing

      // Update tis for all state machines
      update_tis();      

      // Update the states
      set_state(fsm_1, fsm_1.new_state);

      //actions of the states
      act(fsm_1, 1);
    
      // DEBUGGING
    /*
      // Debug using the serial port
      // Print buttons
      Serial.print("S1: ");
      Serial.print(s1);

      // Print loop number
      Serial.print(" loop: ");
      Serial.println(micros() - loop_micros);
    */   
}