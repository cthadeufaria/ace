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
fsm_t fsm_s1, fsm_s2, fsm_config0, fsm_config1, fsm_config2, fsm_config3, fsm_pc, fsm_end ,fsm_control, fsm_led1, fsm_led2, fsm_led3, fsm_led4, fsm_led5, fsm_led6, fsm_led7;

unsigned long interval, last_cycle;
unsigned long loop_micros;
unsigned long T,periodo_led, Tmod2, blink;
//cc-> change configuration
//cs-> configuration setting
//S2 click or double click
uint8_t cc, cs,S1_click,S2_click, S2_double;
/*bool cc = false; 
bool cs = false;
bool cp = false;*/

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
  fsm_s1.tis = cur_time - fsm_s1.tes;
  fsm_s2.tis = cur_time - fsm_s2.tes;
  fsm_config0.tis = cur_time - fsm_config0.tes;
  fsm_config1.tis = cur_time - fsm_config1.tes;
  fsm_config2.tis = cur_time - fsm_config2.tes;
  fsm_config3.tis = cur_time - fsm_config3.tes;
  fsm_pc.tis = cur_time - fsm_pc.tes;
  fsm_end.tis = cur_time - fsm_end.tes;
  fsm_control.tis= cur_time - fsm_control.tes;
  fsm_led1.tis= cur_time - fsm_led1.tes;
}

void evolve_S1(fsm_t &fsm){
// Calculate next state for the 1st state machine
  switch (fsm.state)
    {
    case 0:
      if(s1 < prevs1)
        fsm.new_state = 1;
      break;

    case 1:
      if(s1 > prevs1){
        fsm.new_state = 2;
        S1_click=1;
      }
      else if(fsm.tis>=3000){
        fsm.new_state = 3;
        cs = 1;
      }
      break;

    case 2:
      if(S1_click==0)
        fsm.new_state = 0;
      break;
    
    case 3:
      if(s1 < prevs1)
        fsm.new_state = 4;
      break;
    
    case 4:
      if(fsm.tis >= 3000){
        fsm.new_state = 0;
        fsm_control.new_state=0;
        cs = 0;
      }
      else if(s1 > prevs1){
        fsm.new_state = 5;
        cc=1;
      }
      break;
    
    case 5:
      if(cc==0)
        fsm.new_state = 0;
      break;
  }
}

void evolve_S2(fsm_t &fsm){
  // Calculate next state for the 2nd state machine
  switch (fsm.state)
    {
    case 0:
      if(s2 < prevs2)
        fsm.new_state = 1;
      break;

    case 1:
      if(fsm.tis>=500){
        fsm.new_state = 0;
        S2_click = 1;
      }
      else if(s2 > prevs2){
        fsm.new_state = 2;
        S2_double = 1;
      }
      break;

    case 2:
      if(S2_double==0)
        fsm.new_state = 0;
      break;
  }
}

// led 1 pisca no estado 0
// led 2 pisca no estado 1
// led 3 pisca no estado 2
void evolve_config0(fsm_t &fsm){
 // Calculate next state for the 3rd state machine
  switch (fsm.state){
    case 0:
    if(cc==1){
      fsm.new_state=1;
      cc=0;
    }
    break;
    case 1:
    if(cc==1){
      fsm.new_state=2;
      cc=0;
    }
    break;
    case 2:
    if(cc==1){
      fsm.new_state=0;
      cc=0;
    }
    break;
  }
}

// led 7 acende com o tempo imposto
void evolve_config1(fsm_t &fsm){
// Calculate next state for the 4th state machine
  switch(fsm.state){
    case 0:
      if(fsm_config0.state == 0 && cs==1 && S2_click==1){
        fsm.new_state=1;
        S2_click=0;
      }
      break;
    case 1:
      if(fsm_config0.state == 0 && cs==1 && S2_click==1){
        fsm.new_state=2;
        S2_click=0;
      }
      break;
    case 2:
      if(fsm_config0.state == 0 && cs==1 && S2_click==1){
        fsm.new_state=3;
        S2_click=0;
      }
      break;
    case 3:
      if(fsm_config0.state == 0 && cs==1 && S2_click==1){
        fsm.new_state=0;
        S2_click=0;
      }
      break;
  }
}

// led 7 apresenta o modo imposto
void evolve_config2(fsm_t &fsm){
// Calculate next state for the 5th state machine
  switch(fsm.state){
    case 0:
      if(fsm_config0.state == 1 && cs==1 && S2_click==1){
        fsm.new_state=1;
        S2_click=0;
      }
      break;
    case 1:
      if(fsm_config0.state == 1 && cs==1 && S2_click==1){
        fsm.new_state=2;
        S2_click=0;
      }
      break;
    case 2:
      if(fsm_config0.state == 1 && cs==1 && S2_click==1){
        fsm.new_state=0;
        S2_click=0;
      }
      break;
  }
}

// led 7 apresenta o final imposto
void evolve_config3(fsm_t &fsm){
// Calculate next state for the 6th state machine
  switch(fsm.state){
    case 0:
      if(fsm_config0.state == 2 && cs==1 && S2_click==1){
        fsm.new_state=1;
        S2_click=0;
      }
      break;
    case 1:
      if(fsm_config0.state == 2 && cs==1 && S2_click==1){
        fsm.new_state=0;
        S2_click=0;
      }
      break;
  }
}

// quando no estado 1 o leds atuals piscam
void evolve_pc(fsm_t &fsm){
 // Calculate next state for the 7th state machine
  switch(fsm.state){
    case 0:
      if(cs==0 && S2_click==1){
        fsm.new_state=1;
        S2_click=0;
        Tmod2= Tmod2 - fsm_led1.tis;
      }
      break;
    case 1:
      if(cs==0 && S2_click==1){
        fsm.new_state=0;
        S2_click=0;
        periodo_led= T + fsm.tis;
      }
      break;
  }
}

//0 led 7 on
//1 leds 1-6 piscam
void evolve_end(fsm_t &fsm){
  switch (fsm.state){
    case 0:
      if(fsm_control.state==7)
        fsm.new_state=1;
      break;
    case 1:
      if(fsm_config3.state==0)
        fsm.new_state=2;
      else if(fsm_config3.state==1)
        fsm.new_state=3;
      break;
    case 2:
      if(fsm_control.state <=1)
        fsm.new_state=0;
      break;
    case 3:
      if(fsm.tis>=100)
        fsm.new_state=4;
      else if(fsm_control.state<=1)
        fsm.new_state=0;
      break;
    case 4:
      if(fsm.tis>=100)
        fsm.new_state=3;
      else if(fsm_control.state<=1)
        fsm.new_state=0;
      break;
  }
}

//0 espera
//1 inicia e cada estado desliga um led
//7 fim
//falta a pausa
void evolve_control(fsm_t &fsm){
  switch(fsm.state){
    case 0:
      if(S1_click==1){
        fsm.new_state=1;
        periodo_led=T;
        S1_click=0;
      }
      break;
    case 1:
      if(fsm.tis >= periodo_led && fsm_pc.state==0){
        fsm.new_state=2;
        periodo_led=T;
      }
      else if(fsm_pc.state==1){
        fsm.new_state=11;
      }
      break;
    case 2:
      if(S1_click==1){
        fsm.new_state=1;
        periodo_led=T;
        S1_click=0;
      }
      break;
    default:
      if(fsm.state>10)
       {if(fsm_pc.state==0)
        fsm.new_state=fsm.state-10;
        }break;
  } 
}

//0 espera
//1 decisao do modo
//2 modo 1
//3-5 modo 2
//6... modo 3
void evolve_led1(fsm_t &fsm){
  if((fsm_pc.state==1 and fsm_control.state==11) || (cs==1 && fsm_config0.state==0)){
    fsm.new_state=10;
  }
  switch (fsm.state){
    case 0:
      if(fsm_control.state==1)
        fsm.new_state=1;
      break;
    case 1:
      if(fsm_config2.state==0)//modo 1
        fsm.new_state=2;
      else if(fsm_config2.state==1){//modo 2
        fsm.new_state=3;
        Tmod2=T/2;  
      }
      else if(fsm_config2.state==2)//modo 3
        fsm.new_state=6;
      break;
    //mod1
    case 2:
      if(fsm_control.state==2)
        fsm.new_state=0;
      break;
    //mod2
    case 3:
      if(fsm.tis>= Tmod2)
        fsm.new_state=4;
      break;
    case 4:
      if(fsm.tis>=blink)
        fsm.new_state=5;
      else if(fsm_control.state==2)
        fsm.new_state=0;
      break;
    case 5:
      if(fsm.tis>=blink)
        fsm.new_state=4;
      else if(fsm_control.state==2)
        fsm.new_state=0;
      break;
    //mod3
    case 6:
        if(fsm_control.state==2)
            fsm.new_state=0;
        break;
    //pausa e cs com modo 1
    case 10:
      if(fsm.tis>=blink)
        fsm.new_state=11;
      else if((fsm_pc.state=0) && (cs==0|| fsm_config0.state!=0))
        fsm.new_state=0;
      break;
    case 11:
      if(fsm.tis>=blink)
        fsm.new_state=10;
      else if((fsm_pc.state=0) && (cs==0|| fsm_config0.state!=0))
        fsm.new_state=0;
      break;
  }
}

void act_config1(fsm_t &fsm){
  switch (fsm.state)
  {
  case 0:
    T=2000;
    break;  
  case 1:
    T=4000;
    break;  
  case 2:
    T=8000;
    break;  
  case 3:
    T=1000;
    break;  
  }
}

void act_leds(){
    //led1
    if(fsm_led1.state==6){
      LED_1=map(fsm_control.tis, 0, T, 255,0);
    }
    else if(fsm_led1.state==2 || fsm_led1.state==3 || fsm_led1.state==5 || fsm_led1.state==10){
        digitalWrite(LED1_pin, HIGH);
    }
    else digitalWrite(LED1_pin,LOW);
    //led2
    //led3
    //led4
    //led5
    //led6
    //led7
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
  
  s1=1;
  s2=1;
  cc=0;
  cs=0;
  S1_click=0;
  S2_click=0;
  S2_double=0;

  T=2000;
  blink=100;
  interval = 10;
  set_state(fsm_s1, 0);
//  set_state(fsm_s2, 0);    
  set_state(fsm_config0, 0);
  set_state(fsm_config1, 0);
  set_state(fsm_config2, 0);
  set_state(fsm_config3, 0);
  set_state(fsm_pc, 0);
  set_state(fsm_end, 0);
  set_state(fsm_control, 0);
  set_state(fsm_led1, 0);  
}

void loop() 
{
    // To measure the time between loop() calls
    // unsigned long last_loop_micros = loop_micros; 
    
    // Do this only every "interval" miliseconds 
    // It helps to clear the switches bounce effect
    /*unsigned long now = millis();
    if (now - last_cycle > interval) {
      loop_micros = micros();
      last_cycle = now;
    */  
      // Read the inputs / inverse logic: buttom pressed equals zero
      prevs1 = s1;
      prevs2 = s2;
      s1 = digitalRead(S1_pin);
      s2 = digitalRead(S2_pin);

      // FSM processing

      // Update tis for all state machines
      update_tis();

      evolve_S1(fsm_s1);
  //    evolve_S2(fsm_s2);
      evolve_config0(fsm_config0);
      evolve_config1(fsm_config1);
      evolve_config2(fsm_config2);
      evolve_config3(fsm_config3);
      evolve_pc(fsm_pc);
      evolve_end(fsm_end);
      evolve_control(fsm_control);
      evolve_led1(fsm_led1);
      

      // Update the states
      set_state(fsm_s1, fsm_s1.new_state);
    //  set_state(fsm_s2, fsm_s2.new_state);
      set_state(fsm_config0, fsm_config0.new_state);
      set_state(fsm_config1, fsm_config1.new_state);
      set_state(fsm_config2, fsm_config2.new_state);
      set_state(fsm_config3, fsm_config3.new_state);
      set_state(fsm_pc, fsm_pc.new_state);
      set_state(fsm_end, fsm_end.new_state);
      set_state(fsm_control, fsm_control.new_state);
      set_state(fsm_led1,fsm_led1.new_state);
      
      //actions of the states
      act_config1(fsm_config1);
      act_leds();

/*
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
      Serial.println(micros() - loop_micros);*/
    }