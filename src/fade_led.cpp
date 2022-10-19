#include <Arduino.h>
#include "hardware/pwm.h"

#define LED1_pin 6
#define LED2_pin 7

#define S1_pin 2
#define S2_pin 3

int brightness = 0;
int fadeAmount = 1;

// int main() {
 
//     // Tell GPIO 0 and 1 they are allocated to the PWM
//     gpio_set_function(0, GPIO_FUNC_PWM);
//     gpio_set_function(1, GPIO_FUNC_PWM);
 
//     // Find out which PWM slice is connected to GPIO 0 (it's slice 0)
//     uint slice_num = pwm_gpio_to_slice_num(0);
 
//     // Set period of 4 cycles (0 to 3 inclusive)
//     pwm_set_wrap(slice_num, 3);
//     // Set channel A output high for one cycle before dropping
//     pwm_set_chan_level(slice_num, PWM_CHAN_A, 1);
//     // Set initial B output high for three cycles before dropping
//     pwm_set_chan_level(slice_num, PWM_CHAN_B, 3);
//     // Set the PWM running
//     pwm_set_enabled(slice_num, true);
 
//     // Note we could also use pwm_set_gpio_level(gpio, x) which looks up the
//     // correct slice and channel for a given GPIO.
// }

typedef struct {
  int state, new_state;

  // tes - time entering state
  // tis - time in state
  unsigned long tes, tis;
} fsm_t;

// Input variables
uint8_t S1, prevS1;
uint8_t S2, prevS2;

// Output variables
uint8_t LED_1, LED_2;

// Our finite state machines
fsm_t fsm1, fsm2;

unsigned long interval, last_cycle;
unsigned long loop_micros;

// Set new state
void set_state(fsm_t& fsm, int new_state)
{
  if (fsm.state != new_state) {  // if the state chnanged tis is reset
    fsm.state = new_state;
    fsm.tes = millis();
    fsm.tis = 0;
  }
}


void setup() 
{
  pinMode(LED1_pin, OUTPUT);
  pinMode(LED2_pin, OUTPUT);
  pinMode(S1_pin, INPUT);
  pinMode(S2_pin, INPUT);

  // Start the serial port with 115200 baudrate
  Serial.begin(115200);

  interval = 10;
  set_state(fsm1, 0);
  set_state(fsm2, 0);    
}

void loop() 
{
    // To measure the time between loop() calls
    //unsigned long last_loop_micros = loop_micros; 
    
    // Do this only every "interval" miliseconds 
    // It helps to clear the switches bounce effect
    unsigned long now = millis();
    if (now - last_cycle > interval) {
      loop_micros = micros();
      last_cycle = now;
      
      // Read the inputs
      prevS1 = S1;
      prevS2 = S2;
      S1 = !digitalRead(S1_pin);
      S2 = !digitalRead(S2_pin);

      // FSM processing

      // Update tis for all state machines
      unsigned long cur_time = millis();   // Just one call to millis()
      fsm1.tis = cur_time - fsm1.tes;
      fsm2.tis = cur_time - fsm2.tes; 

      // Calculate next state for the first state machine
      if (fsm1.state == 0 && S1){
        fsm1.new_state = 1;
      } else if(fsm1.state == 1 && !S1) {
        fsm1.new_state = 0;
      } /* else if (fsm1.state == 1 && fsm1.tis > 1000){
        fsm1.new_state = 2;
      } else if (fsm1.state == 2 && fsm1.tis > 1000){
        fsm1.new_state = 1;
      } else if (fsm1.state == 2 && !S1){
        fsm1.new_state = 0;
      } */

      // Calculate next state for the second state machine
      /*if (fsm2.state == 0 && S2 && !prevS2){
        fsm2.new_state = 1;
      } else if (fsm2.state == 1 && S2 && !prevS2){
        fsm2.new_state = 0;
      }*/

      // Update the states
      set_state(fsm1, fsm1.new_state);
      set_state(fsm2, fsm2.new_state);

      // Actions set by the current state of the first state machine
      if (fsm1.state == 0){
        LED_1 = 0;
      } else if (fsm1.state == 1){
        LED_1 = 1;
      } /* else if (fsm1.state == 2){
        LED_1 = 0;
      } */

      // A more compact way
      // LED_1 = (fsm1.state == 1);
      // LED_1 = (state == 1)||(state ==2);  if LED1 must be set in states 1 and 2
      
      // Actions set by the current state of the second state machine
      // LED_2 = (fsm2.state == 0);

      // Set the outputs
      analogWrite(LED1_pin, brightness);
      digitalWrite(LED2_pin, LED_2);

      if (LED_1 == 1){
        // change the brightness for next time through the loop:
        brightness = brightness + fadeAmount;

        // reverse the direction of the fading at the ends of the fade:
        if (brightness == 0 || brightness == 255) {
          fadeAmount = -fadeAmount ;
        }
      } else if (LED_1 == 0){
        brightness = 0;
      }

      // Debug using the serial port
      Serial.print("S1: ");
      Serial.print(S1);

      Serial.print(" S2: ");
      Serial.print(S2);

      Serial.print(" fsm1.state: ");
      Serial.print(fsm1.state);

      Serial.print(" LED_1: ");
      Serial.print(LED_1);

      Serial.print(" LED_2: ");
      Serial.print(LED_2);

      Serial.print(" loop: ");
      Serial.println(micros() - loop_micros);
    }
    
}
