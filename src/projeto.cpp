#include <Arduino.h>

#define ledPin 6
#define s1Pin 2
#define sensor1Pin 9
#define motor1Pin1 14
#define motor1Pin2 15
#define motor1Enable 15

// structure to define finite state machine
// tes - time entering state / tis - time in state
typedef struct {
  int state, new_state;
  unsigned long tes, tis;
} fsm_t;

// Input variables
uint8_t s1, prevS1, sensor1;

// Output variables
uint8_t motor1act1, motor1act2, motor1enable;

// Our finite state machines
fsm_t fsm1;

// Declaring main variables
unsigned long interval, last_cycle;
unsigned long loop_micros;
uint8_t var, speed1, speed2;

// Set new state
void setState(fsm_t & fsm, int new_state){
  if (fsm.state != new_state) {  // if the state changed tis is reset
    fsm.state = new_state;
    fsm.tes = millis();
    fsm.tis = 0;
  }
}

void updateTis(){
  unsigned long cur_time = millis();   // Just one call to millis()
  fsm1.tis = cur_time - fsm1.tes;
}

void setup(){
  pinMode(ledPin, OUTPUT);
  pinMode(s1Pin, INPUT);
  pinMode(sensor1Pin, INPUT);

  // Start the serial port with 115200 baudrate
  Serial.begin(115200);

  // Start variables
  interval = 10;
  var = 0;
  setState(fsm1, 0);
}

void actOnMotors(fsm_t & fsm, int var){
  // Function to act on dc motors.
  // speed must be an integer between 0 and 255
  digitalWrite(motor1act1, speed1);
  digitalWrite(motor1act2, speed2);
  analogWrite(motor1enable, speed2);
}

void readInputs(){
  sensor1 = analogRead(sensor1Pin);
}

void loop(){
    // To measure the time between loop() calls
    unsigned long last_loop_micros = loop_micros; 
    
    // Do this only every "interval" miliseconds 
    // It helps to clear the switches bounce effect
    unsigned long now = millis();
    if (now - last_cycle > interval) {
      loop_micros = micros();
      last_cycle = now;
      
      // Read the inputs / inverse logic: buttom pressed equals zero
      prevS1 = s1;
      s1 = digitalRead(s1Pin);

      // FSM processing

      // Update tis for all state machines
      updateTis();      

      // Update the states
      setState(fsm1, fsm1.new_state);

      //actions of the states
      actOnMotors(fsm1, 1);
    
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
}