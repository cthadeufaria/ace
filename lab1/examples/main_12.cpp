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
fsm_t fsm_s1, fsm_s2, fsm_config0, fsm_config1, fsm_config2, fsm_config3, fsm_pause, fsm_end ,fsm_control, fsm_led1, fsm_led2, fsm_led3, fsm_led4, fsm_led5, fsm_led6, fsm_led7;

unsigned long interval, last_cycle;
unsigned long loop_micros;
unsigned long T,periodo_led, Tmod2, blink, lum;
//S1_click-> change configuration
//cs-> configuration setting
//S2 click or double click
uint8_t pause, cs, S1_click, S2_click, S2_double, point;
/*bool S1_click = false; 
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
  fsm_pause.tis = cur_time - fsm_pause.tes;
  fsm_end.tis = cur_time - fsm_end.tes;
  fsm_control.tis= cur_time - fsm_control.tes;
  fsm_led1.tis= cur_time - fsm_led1.tes;
  fsm_led2.tis= cur_time - fsm_led2.tes;
  fsm_led3.tis= cur_time - fsm_led3.tes;
  fsm_led4.tis= cur_time - fsm_led4.tes;
  fsm_led5.tis= cur_time - fsm_led5.tes;
  fsm_led6.tis= cur_time - fsm_led6.tes;
  fsm_led7.tis= cur_time - fsm_led7.tes;
}

void evolve_S1(fsm_t &fsm, fsm_t &control, fsm_t &L7){
// Calculate next state for the 1st state machine
  switch (fsm.state)
    {
    case 0:
      if(s1 < prevs1)
        fsm.new_state = 1;
      break;

    case 1:
      if(s1 > prevs1){
        fsm.new_state=2;
        control.new_state=1;        
      }
      else if(fsm.tis>=3000){
        fsm.new_state = 3;
        cs = 1;
        control.new_state=0;
      }
      break;
  
    case 2:
      if(point==1)
        fsm.new_state = 0;
      break;
    
    case 3:
      if(s1 < prevs1)
        fsm.new_state = 4;
      break;
    
    case 4:
      if(fsm.tis >= 3000){
        fsm.new_state = 0;
        cs = 0;
        L7.new_state=0;
      }
      else if(s1 > prevs1){
        fsm.new_state = 5;
        S1_click=1;
      }
      break;
    
    case 5:
      if(S1_click==0)
        fsm.new_state = 3;
      break;
  }
}

void evolve_S2(fsm_t &fsm){
  // Calculate next state for the 2nd state machine
  switch (fsm.state)
    {
    case 0:
      if(s2 < prevs2){
        fsm.new_state = 1;
        // LED_5 = 1;
      }
      break;

    case 1:
      if(fsm.tis >= 500 || cs==0){
        fsm.new_state = 2;
        S2_click = 1;
        // LED_2 = 1;
      }
      else if(s2 < prevs2){
        fsm.new_state = 3;
        S2_double = 1;
        // LED_6 = 1;
      }
      break;

    case 2:
      if(S2_click==0)
        fsm.new_state = 0;
      break;

    case 3:
      if(S2_double==0)
        fsm.new_state = 0;
      break;
  }
}

// led 1 pisca no estado 0
// led 2 pisca no estado 1
// led 3 pisca no estado 2
void evolve_config0(fsm_t & fsm){
 // Calculate next state for the 3rd state machine
  switch (fsm.state){
    case 0:
    if(cs==1 && S1_click==1){
      fsm.new_state=1;
      S1_click=0;
    }
    break;

    case 1:
    if(cs==1 && S1_click==1){
      fsm.new_state=2;
      S1_click=0;
    }
    break;

    case 2:
    if(cs==1 && S1_click==1){
      fsm.new_state=0;
      S1_click=0;
    }
    break;
  }
}

// led 7 acende com o tempo imposto
void evolve_config1(fsm_t &fsm){
// Calculate next state for the 4th state machine
  switch(fsm.state){
    case 0:
      // m == 0
      if(fsm_config0.state == 0 && cs==1 && S2_click==1){
        fsm.new_state=1;
        S2_click=0;
        // LED_2 = 1;
      }
      break;

    case 1:
      // m == 0
      if(fsm_config0.state == 0 && cs==1 && S2_click==1){
        fsm.new_state=2;
        S2_click=0;
        // LED_5 = 1;
      }
      break;

    case 2:
      // m == 0
      if(fsm_config0.state == 0 && cs==1 && S2_click==1){
        fsm.new_state=3;
        S2_click=0;
        // LED_6 = 1;
      }
      break;

    case 3:
      // m == 0
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
      // m == 1
      if(fsm_config0.state == 1 && cs==1 && S2_click==1){
        fsm.new_state=1;
        S2_click=0;
      }
      break;
    case 1:
      // m == 1
      if(fsm_config0.state == 1 && cs==1 && S2_click==1){
        fsm.new_state=2;
        S2_click=0;
      }
      break;
    case 2:
      // m == 1
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
      // m == 2
      if(fsm_config0.state == 2 && cs==1 && S2_click==1){
        fsm.new_state=1;
        S2_click=0;
      }
      break;
    case 1:
      // m == 2
      if(fsm_config0.state == 2 && cs==1 && S2_click==1){
        fsm.new_state=0;
        S2_click=0;
      }
      break;
  }
}

// quando no estado 1 o leds atuals piscam
void evolve_pause(fsm_t &fsm, fsm_t &control){
 // Calculate next state for the 7th state machine
  switch(fsm.state){
    case 0:
      if(cs==0 && S2_click==1 && control.state!= 0 && control.state!=7){
        fsm.new_state=1;
        S2_click=0;
        Tmod2= Tmod2 - fsm_led1.tis;
        periodo_led= T - control.tis;
        lum= control.tis;
      }
      break;
    case 1:
      if(cs==0 && S2_click==1){
        fsm.new_state=0;
        S2_click=0;
        control.new_state= control.state - 10;
      }
      else if(cs==1){
        fsm.new_state=0;
        S2_click=0;
      }
      break;
  }
}

//0 led 7 on
//1 leds 1-6 piscam
void evolve_end(fsm_t &fsm, fsm_t &control){
  switch (fsm.state){
    case 0:
      if(control.state==7 && cs==0)//control state final
        fsm.new_state=1;
      break;
    case 1:
      if(fsm_config3.state==0 && cs==0)
        fsm.new_state=2;
      else if(fsm_config3.state==1&& cs==0)
        fsm.new_state=3;
      break;
    case 2:
      if(control.state <=1 || cs==1)
        fsm.new_state=0;
      break;
    case 3:
      if(fsm.tis>=blink && cs==0)
        fsm.new_state=4;
      else if(control.state<=1 || cs==1)
        fsm.new_state=0;
      break;
    case 4:
      if(fsm.tis>=blink && cs==0)
        fsm.new_state=3;
      else if(control.state<=1 || cs==1)
        fsm.new_state=0;
      break;
  }
}

//0 espera
//1 inicia e cada estado desliga um led
//7 fim
void evolve_control(fsm_t &fsm, fsm_t &ss1){
  switch(fsm.state){
    case 0:
      if(ss1.state==2){
        fsm.new_state=1;
        periodo_led=T;
        lum=0;
      }
      break;

    case 1:
      if(fsm.tis >= periodo_led){
        fsm.new_state=2;
        periodo_led=T;
        lum=0;
      }
      else if(pause){
        fsm.new_state=11;
      }
      break;

    case 2:
      if(fsm.tis >= periodo_led){
        fsm.new_state=3;
        periodo_led=T;
        lum=0;
      }
      else if(pause){
        fsm.new_state=12;
      }
      break;

    case 3:
      if(fsm.tis >= periodo_led){
        fsm.new_state=4;
        periodo_led=T;
        lum=0;
      }
      else if(pause){
        fsm.new_state=13;
      }
      break;
    
    case 4:
      if(fsm.tis >= periodo_led){
        fsm.new_state=5;
        periodo_led=T;
        lum=0;
      }
      else if(pause){
        fsm.new_state=14;
      }
      break;
      
      case 5:
      if(fsm.tis >= periodo_led){
        fsm.new_state=6;
        periodo_led=T;
        lum=0;
      }
      else if(pause){
        fsm.new_state=15;
      }
      break;
    
    case 6:
      if(fsm.tis >= periodo_led){
        fsm.new_state=7;
        periodo_led=T;
        lum=0;
      }
      else if(pause){
        fsm.new_state=16;
      }
      break;

    case 7:
      if(ss1.state==2){
        fsm.new_state=1;
        periodo_led=T;
      }
      break;
  } 
}

//0 espera
//1 decisao do modo
//2 modo 1
//3-5 modo 2
//6... modo 3
void evolve_led1(fsm_t &fsm, fsm_t &control, fsm_t &c0){
  switch(fsm.state){
    case 0:
      if(control.state==1 && cs==0)
        fsm.new_state=1;
      else if((control.state==11 && pause)||(cs==1 && c0.state==0)){
        fsm.new_state=7;
      }      
      break;
    
    case 1:
      if(fsm_config2.state==0)
        fsm.new_state=2;
      else if(fsm_config2.state==1 && Tmod2 > 0)
        fsm.new_state=3;
      else if(fsm_config2.state==1 && Tmod2 == 0)
        fsm.new_state=4;
      else if(fsm_config2.state==2)
        fsm.new_state=6;
      break;

    case 2:
      if(cs==1||pause||control.state==2)
        fsm.new_state=0;
      break;
    
    case 3:
      if(cs==1||pause||control.state==2)
        fsm.new_state=0;
      else if(fsm.tis>=Tmod2){
        fsm.new_state=4;
        Tmod2=0;
      }
      break;
    
    case 4:
      if(cs==1||pause||control.state==2)
        fsm.new_state=0;
      else if(fsm.tis>=blink)
        fsm.new_state=5;
      break;
    
    case 5:
      if(cs==1||pause||control.state==2)
        fsm.new_state=0;
      else if(fsm.tis>=blink)
        fsm.new_state=4;
      break;

    case 6:
      if(cs==1||pause||control.state==2)
        fsm.new_state=0;
      break;
    
    case 7:
    //  && (pause || (fsm_config0.state==1 && cs==1))
      if(fsm.tis >= blink){
        fsm.new_state=8;
      }      
      else if((!pause && cs==0) || (cs==1 && c0.state!=0))
        fsm.new_state=0;
      break;

    case 8:
      if(fsm.tis>=blink)
        fsm.new_state=7;
      else if((!pause && cs==0) || (cs==1 && c0.state!=0))
        fsm.new_state=0;
      break;
  }
}

void evolve_led2(fsm_t &fsm, fsm_t &control, fsm_t &c0){
  switch(fsm.state){
    case 0:
      if(control.state==1 && cs==0)
        fsm.new_state=1;
      else if((control.state==12 && pause)){
        fsm.new_state=7;
      }
      else if(cs==1 && c0.state == 1){
        fsm.new_state=9;
      }     
      break;
    
    case 1:
      if(fsm_config2.state==0 && control.state==2)
        fsm.new_state=2;
      else if(fsm_config2.state==1 && Tmod2 > 0 && control.state==2)
        fsm.new_state=3;
      else if(fsm_config2.state==1 && Tmod2 == 0 && control.state==2)
        fsm.new_state=4;
      else if(fsm_config2.state==2 && control.state==2)
        fsm.new_state=6;
      break;

    case 2:
      if(cs==1||pause||control.state==3)
        fsm.new_state=0;
      break;
    
    case 3:
      if(cs==1||pause||control.state==3)
        fsm.new_state=0;
      else if(fsm.tis>=Tmod2){
        fsm.new_state=4;
        Tmod2=0;
      }
      break;
    
    case 4:
      if(cs==1||pause||control.state==3)
        fsm.new_state=0;
      else if(fsm.tis>=blink)
        fsm.new_state=5;
      break;
    
    case 5:
      if(cs==1||pause||control.state==3)
        fsm.new_state=0;
      else if(fsm.tis>=blink)
        fsm.new_state=4;
      break;

    case 6:
      if(cs==1||pause||control.state==3)
        fsm.new_state=0;
      break;
    
    case 7:
    //  && (pause || (fsm_config0.state==1 && cs==1))
      if(fsm.tis >= blink){
        fsm.new_state=8;
      }      
      else if(!pause && cs==0)
        fsm.new_state=1;
      break;

    case 8:
      if(fsm.tis>=blink)
        fsm.new_state=7;
      else if(!pause && cs==0)
        fsm.new_state=1;
      break;
    
    case 9:
      if(fsm.tis >= blink){
        fsm.new_state=10;
      }      
      else if(cs==1 && c0.state!=1)
        fsm.new_state=0;
      break;

    case 10:
      if(fsm.tis>=blink)
        fsm.new_state=9;
      else if(cs==1 && c0.state!=1)
        fsm.new_state=0;
      break;
  }
}

void evolve_led3(fsm_t &fsm, fsm_t &control, fsm_t &c0){
  switch(fsm.state){
    case 0:
      if(control.state==1 && cs==0)
        fsm.new_state=1;
      else if(control.state==13 && pause){
        fsm.new_state=7;
      }
      else if(cs==1 && c0.state==2){
        fsm.new_state=9;
      }
      break;
    
    case 1:
      if(fsm_config2.state==0 && control.state==3)
        fsm.new_state=2;
      else if(fsm_config2.state==1 && Tmod2 > 0 && control.state==3)
        fsm.new_state=3;
      else if(fsm_config2.state==1 && Tmod2 == 0 && control.state==3)
        fsm.new_state=4;
      else if(fsm_config2.state==2 && control.state==3)
        fsm.new_state=6;
      break;

    case 2:
      if(cs==1||pause||control.state==4)
        fsm.new_state=0;
      break;
    
    case 3:
      if(cs==1||pause||control.state==4)
        fsm.new_state=0;
      else if(fsm.tis>=Tmod2){
        fsm.new_state=4;
        Tmod2=0;
      }
      break;
    
    case 4:
      if(cs==1||pause||control.state==4)
        fsm.new_state=0;
      else if(fsm.tis>=blink)
        fsm.new_state=5;
      break;
    
    case 5:
      if(cs==1||pause||control.state==4)
        fsm.new_state=0;
      else if(fsm.tis>=blink)
        fsm.new_state=4;
      break;

    case 6:
      if(cs==1||pause||control.state==4)
        fsm.new_state=0;
      break;
    
    case 7:
    //  && (pause || (fsm_config0.state==1 && cs==1))
      if(fsm.tis >= blink){
        fsm.new_state=8;
      }      
      else if(!pause && cs==0)
        fsm.new_state=1;
      break;

    case 8:
      if(fsm.tis>=blink)
        fsm.new_state=7;
      else if(!pause && cs==0)
        fsm.new_state=1;
      break;
    
    case 9:
      if(fsm.tis >= blink){
        fsm.new_state=10;
      }      
      else if(cs==1 && c0.state!=2)
        fsm.new_state=0;
      break;

    case 10:
      if(fsm.tis>=blink)
        fsm.new_state=9;
      else if(cs==1 && c0.state!=2)
        fsm.new_state=0;
      break;
  }
}

void evolve_led4(fsm_t &fsm, fsm_t &control){
  switch(fsm.state){
    case 0:
      if(control.state==1 && cs==0)
        fsm.new_state=1;
      else if(control.state==14 && pause){
        fsm.new_state=7;
      }      
      break;
    
    case 1:
      if(fsm_config2.state==0 && control.state==4)
        fsm.new_state=2;
      else if(fsm_config2.state==1 && Tmod2 > 0 && control.state==4)
        fsm.new_state=3;
      else if(fsm_config2.state==1 && Tmod2 == 0 && control.state==4)
        fsm.new_state=4;
      else if(fsm_config2.state==2 && control.state==4)
        fsm.new_state=6;
      break;

    case 2:
      if(cs==1||pause||control.state==5)
        fsm.new_state=0;
      break;
    
    case 3:
      if(cs==1||pause||control.state==5)
        fsm.new_state=0;
      else if(fsm.tis>=Tmod2){
        fsm.new_state=4;
        Tmod2=0;
      }
      break;
    
    case 4:
      if(cs==1||pause||control.state==5)
        fsm.new_state=0;
      else if(fsm.tis>=blink)
        fsm.new_state=5;
      break;
    
    case 5:
      if(cs==1||pause||control.state==5)
        fsm.new_state=0;
      else if(fsm.tis>=blink)
        fsm.new_state=4;
      break;

    case 6:
      if(cs==1||pause||control.state==5)
        fsm.new_state=0;
      break;
    
    case 7:
    //  && (pause || (fsm_config0.state==1 && cs==1))
      if(fsm.tis >= blink){
        fsm.new_state=8;
      }      
      else if(!pause && cs==0)
        fsm.new_state=1;
      break;

    case 8:
      if(fsm.tis>=blink)
        fsm.new_state=7;
      else if(!pause && cs==0)
        fsm.new_state=1;
      break;
  }
}

void evolve_led5(fsm_t &fsm, fsm_t &control){
  switch(fsm.state){
    case 0:
      if(control.state==1 && cs==0)
        fsm.new_state=1;
      else if(control.state==15 && pause){
        fsm.new_state=7;
      }      
      break;
    
    case 1:
      if(fsm_config2.state==0 && control.state==5)
        fsm.new_state=2;
      else if(fsm_config2.state==1 && Tmod2 > 0 && control.state==5)
        fsm.new_state=3;
      else if(fsm_config2.state==1 && Tmod2 == 0 && control.state==5)
        fsm.new_state=4;
      else if(fsm_config2.state==2 && control.state==5)
        fsm.new_state=6;
      break;

    case 2:
      if(cs==1||pause||control.state==6)
        fsm.new_state=0;
      break;
    
    case 3:
      if(cs==1||pause||control.state==6)
        fsm.new_state=0;
      else if(fsm.tis>=Tmod2){
        fsm.new_state=4;
        Tmod2=0;
      }
      break;
    
    case 4:
      if(cs==1||pause||control.state==6)
        fsm.new_state=0;
      else if(fsm.tis>=blink)
        fsm.new_state=5;
      break;
    
    case 5:
      if(cs==1||pause||control.state==6)
        fsm.new_state=0;
      else if(fsm.tis>=blink)
        fsm.new_state=4;
      break;

    case 6:
      if(cs==1||pause||control.state==6)
        fsm.new_state=0;
      break;
    
    case 7:
    //  && (pause || (fsm_config0.state==1 && cs==1))
      if(fsm.tis >= blink){
        fsm.new_state=8;
      }      
      else if(!pause && cs==0)
        fsm.new_state=1;
      break;

    case 8:
      if(fsm.tis>=blink)
        fsm.new_state=7;
      else if(!pause && cs==0)
        fsm.new_state=1;
      break;
  }
}

void evolve_led6(fsm_t &fsm, fsm_t &control){
  switch(fsm.state){
    case 0:
      if(control.state==1 && cs==0)
        fsm.new_state=1;
      else if(control.state==16 && pause){
        fsm.new_state=7;
      }      
      break;
    
    case 1:
      if(fsm_config2.state==0 && control.state==6)
        fsm.new_state=2;
      else if(fsm_config2.state==1 && Tmod2 > 0 && control.state==6)
        fsm.new_state=3;
      else if(fsm_config2.state==1 && Tmod2 == 0 && control.state==6)
        fsm.new_state=4;
      else if(fsm_config2.state==2 && control.state==6)
        fsm.new_state=6;
      break;

    case 2:
      if(cs==1||pause||control.state==7)
        fsm.new_state=0;
      break;
    
    case 3:
      if(cs==1||pause||control.state==7)
        fsm.new_state=0;
      else if(fsm.tis>=Tmod2){
        fsm.new_state=4;
        Tmod2=0;
      }
      break;
    
    case 4:
      if(cs==1||pause||control.state==7)
        fsm.new_state=0;
      else if(fsm.tis>=blink)
        fsm.new_state=5;
      break;
    
    case 5:
      if(cs==1||pause||control.state==7)
        fsm.new_state=0;
      else if(fsm.tis>=blink)
        fsm.new_state=4;
      break;

    case 6:
      if(cs==1||pause||control.state==7)
        fsm.new_state=0;
      break;
    
    case 7:
    //  && (pause || (fsm_config0.state==1 && cs==1))
      if(fsm.tis >= blink){
        fsm.new_state=8;
      }      
      else if(!pause && cs==0)
        fsm.new_state=1;
      break;

    case 8:
      if(fsm.tis>=blink)
        fsm.new_state=7;
      else if(!pause && cs==0)
        fsm.new_state=1;
      break;
  }
}

void evolve_led7(fsm_t &fsm, fsm_t &c0, fsm_t &c1, fsm_t &c2, fsm_t &c3){
  switch (fsm.state)
  {
  case 0:
    if(cs==1){
      if(fsm_config0.state==0)
        fsm.new_state=1;
      else if(fsm_config0.state==1)
        fsm.new_state=3;
      else if(fsm_config0.state==2)
        fsm.new_state=9;
    } 
    break;
//m=1
  case 1:
    if(fsm.tis>=T && fsm_config0.state==0 && cs==1)
      fsm.new_state=2;
    else if(cs==1 && fsm_config0.state!=0)
      fsm.new_state=0;
    break;
  
  case 2:
    if(cs==1 && (fsm_config0.state!=0 || (c1.state!=c1.new_state)))
      fsm.new_state=0;
    break;
  //m=2
  case 3:
    if(fsm_config2.state==0 && fsm_config0.state==1 && cs==1)
      fsm.new_state=4;
    else if(fsm_config2.state==1 && fsm_config0.state==1 && cs==1){
      fsm.new_state=5;
      Tmod2=T/2;
    }
    else if(fsm_config2.state==2 && fsm_config0.state==1 && cs==1)
      fsm.new_state=8;
    else if(cs==1 && fsm_config0.state!=1)
      fsm.new_state=0;
    break;
  
  case 4:
    if(fsm.tis>=T && fsm_config0.state==1 && cs==1)
      fsm.new_state=5;
    else if(cs==1 && fsm_config0.state!=1)
      fsm.new_state=0;
    break;

  case 5:
    if(fsm_config2.state!=2 && fsm_config0.state==1 && cs==1)
      fsm.new_state=3;
    else if(fsm_config0.state!=1 && cs==1)
      fsm.new_state=0;
    break;

  case 6:
    if(fsm.tis>=Tmod2 && fsm_config0.state==1 && cs==1)
      fsm.new_state=7;
    else if(fsm_config2.state!=1 && fsm_config0.state==1 && cs==1)
      fsm.new_state=3;
    else if(cs==1 && fsm_config0.state!=1)
      fsm.new_state=0;
    break;
  case 7:
    if(fsm.tis>=blink && fsm_config0.state==1 && cs==1){
      Tmod2=Tmod2-fsm.tis;
      fsm.new_state=8;
    }
    else if(Tmod2<=0 && fsm_config0.state==1 && cs==1)
      fsm.new_state=9;
    else if(fsm_config2.state!=1 && fsm_config0.state==1 && cs==1)
      fsm.new_state=3;
    else if(cs==1 && fsm_config0.state!=1)
      fsm.new_state=0;
    break;

  case 8:
    if(fsm.tis>=blink && fsm_config0.state==1 && cs==1){
      Tmod2=Tmod2-fsm.tis;
      fsm.new_state=7;
    }
    else if(Tmod2<=0 && fsm_config0.state==1 && cs==1)
      fsm.new_state=9;
    else if(fsm_config2.state!=1 && fsm_config0.state==1 && cs==1)
      fsm.new_state=3;
    else if(cs==1 && fsm_config0.state!=1)
      fsm.new_state=0;
    break;
  
  case 9:
    if(fsm_config2.state!=2 && fsm_config0.state==1 && cs==1)
      fsm.new_state=3;
    else if(fsm_config0.state!=1 && cs==1)
      fsm.new_state=0;
    break;

  case 10:
    if(fsm.tis>=T && fsm_config0.state==1 && cs==1)
      fsm.new_state=11;
    else if(cs==1 && fsm_config0.state!=1)
      fsm.new_state=0;
    break;
  
  case 11:
    if(fsm_config2.state!=2 && fsm_config0.state==1 && cs==1)
      fsm.new_state=3;
    else if(fsm_config0.state!=1 && cs==1)
      fsm.new_state=0;
    break;
//m=3
  case 12:
    if(fsm_config3.state==0 && fsm_config0.state==1 && cs==1)
      fsm.new_state=13;
    else if(fsm_config3.state==1&& fsm_config0.state==1 && cs==1)
      fsm.new_state=15;
    else if(cs==1 && fsm_config0.state!=1)
      fsm.new_state=0;
    break;

  case 13:
    if(fsm_config3.state==1&& fsm_config0.state==2 && cs==1)
      fsm.new_state=13;
    else if(cs==1 && fsm_config0.state!=2)
      fsm.new_state=0;
    break;

  case 14:
    if(fsm.tis>= blink/2 && fsm_config0.state==2 && cs==1)
      fsm.new_state=15;
    else if(fsm_config3.state==0 && fsm_config0.state==2 && cs==1)
      fsm.new_state=13;
    else if(fsm_config0.state!=2 && cs==1)
      fsm.new_state=0;
    break;

  case 15:
    if(fsm.tis>= blink/2 && fsm_config0.state==2 && cs==1)
      fsm.new_state=14;
    else if(fsm_config3.state==0 && fsm_config0.state==2 && cs==1)
      fsm.new_state=13;
    else if(fsm_config0.state!=2 && cs==1)
      fsm.new_state=0;
    break;

  }
}


void act_config1(fsm_t &fsm){
  switch (fsm.state)
  {
  case 0:
    T=2000;
    // LED_2 = 1;
    break;  
  case 1:
    T=4000;
    // LED_5 = 1;
    break;  
  case 2:
    T=8000;
    // LED_6 = 1;
    break;  
  case 3:
    T=1000;
    break;  
  }
}



void act_pause(fsm_t &fsm){
  if(fsm.state==1) pause=true;
  else pause=false;
}

void act_control(fsm_t &fsm){
  point=fsm.state;
}

void act_leds(){
    //led1
    if(fsm_end.state == 3 || fsm_led1.state == 2 || fsm_led1.state == 3 || fsm_led1.state == 5 || fsm_led1.state == 8){
      digitalWrite(LED1_pin,HIGH);
    }
    else if (fsm_led1.state == 6)
    {
        LED_1=map(fsm_control.tis+lum, 0, T, 255, 0);
      analogWrite(LED1_pin,LED_1);
    }
    else
      digitalWrite(LED1_pin,LOW);
  
    //led2
        if(fsm_end.state == 3 || fsm_led2.state == 1 || fsm_led2.state == 2 || fsm_led2.state == 3 || fsm_led2.state == 5 || fsm_led2.state == 8|| fsm_led2.state == 10){
      digitalWrite(LED2_pin,HIGH);
    }
    else if (fsm_led2.state == 6)
    {
        LED_2=map(fsm_control.tis+lum, 0, T, 255, 0);
      analogWrite(LED2_pin,LED_2);
    }
    else
      digitalWrite(LED2_pin,LOW);
    
    //led3
        if(fsm_end.state == 3 || fsm_led3.state == 1 || fsm_led3.state == 2 || fsm_led3.state == 3 || fsm_led3.state == 5 || fsm_led3.state == 8 || fsm_led3.state == 10){
      digitalWrite(LED3_pin,HIGH);
    }
    else if (fsm_led3.state == 6)
    {
        LED_3=map(fsm_control.tis+lum, 0, T, 255, 0);
      analogWrite(LED3_pin,LED_3);
    }
    else
      digitalWrite(LED3_pin,LOW);
    
  
    //led4
        if(fsm_end.state == 3 || fsm_led4.state == 1 || fsm_led4.state == 2 || fsm_led4.state == 3 || fsm_led4.state == 5 || fsm_led4.state == 8){
      digitalWrite(LED4_pin,HIGH);
    }
    else if (fsm_led4.state == 6)
    {
        LED_4=map(fsm_control.tis+lum, 0, T, 255, 0);
      analogWrite(LED4_pin,LED_4);
    }
    else
      digitalWrite(LED4_pin,LOW);
    
    //led5
        if(fsm_end.state == 3 || fsm_led5.state == 1 || fsm_led5.state == 2 || fsm_led5.state == 3 || fsm_led5.state == 5 || fsm_led5.state == 8){
      digitalWrite(LED5_pin,HIGH);
    }
    else if (fsm_led5.state == 6)
    {
        LED_5=map(fsm_control.tis+lum, 0, T, 255, 0);
      analogWrite(LED5_pin,LED_5);
    }
    else
      digitalWrite(LED5_pin,LOW);
    
    //led6
        if(fsm_end.state == 3 || fsm_led6.state == 1 || fsm_led6.state == 2 || fsm_led6.state == 3 || fsm_led6.state == 5 || fsm_led6.state == 8){
      digitalWrite(LED6_pin,HIGH);
    }
    else if (fsm_led6.state == 6)
    {
        LED_6=map(fsm_control.tis+lum, 0, T, 255, 0);
      analogWrite(LED6_pin,LED_6);
    }
    else
      digitalWrite(LED6_pin,LOW);
  
    //led7
    if(fsm_end.state == 2|| fsm_led7.state == 1 || fsm_led7.state == 4 || fsm_led7.state == 5 || fsm_led7.state == 8 || fsm_led7.state == 10|| fsm_led7.state == 13){
      digitalWrite(LED7_pin,HIGH);
    }
    else if (fsm_led7.state == 10)
    {
        LED_7=map(fsm_control.tis+lum, 0, T, 255, 0);
      analogWrite(LED7_pin,LED_7);
    }
    else
      digitalWrite(LED7_pin,LOW);
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
  point=0;
  S1_click=0;
  S2_click=0;
  S2_double=0;
  periodo_led=2000;
  blink=200;
  interval = 10;
  set_state(fsm_s1, 0);
  set_state(fsm_s2, 0);    
  set_state(fsm_config0, 0);
  set_state(fsm_config1, 0);
  set_state(fsm_config2, 0);
  set_state(fsm_config3, 0);
  set_state(fsm_pause, 0);
  set_state(fsm_end, 0);
  set_state(fsm_control, 0);
  set_state(fsm_led1, 0);
  set_state(fsm_led2, 0);
  set_state(fsm_led3, 0);
  set_state(fsm_led4, 0);
  set_state(fsm_led5, 0);
  set_state(fsm_led6, 0);
  set_state(fsm_led7, 0);  
}

void loop() 
{
    // To measure the time between loop() calls
    // unsigned long last_loop_micros = loop_micros; 
    
    // Do this only every "interval" miliseconds 
    // It helps to clear the switches bounce effect
    /*
    unsigned long now = millis();
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

      evolve_S1(fsm_s1, fsm_control, fsm_led7);
      evolve_S2(fsm_s2);
      evolve_config0(fsm_config0);
      evolve_config1(fsm_config1);
      evolve_config2(fsm_config2);
      evolve_config3(fsm_config3);
      evolve_pause(fsm_pause, fsm_control);
      evolve_end(fsm_end, fsm_control);
      evolve_control(fsm_control, fsm_s1);
      evolve_led1(fsm_led1, fsm_control, fsm_config0);
      evolve_led2(fsm_led2, fsm_control, fsm_config0);
      evolve_led3(fsm_led3, fsm_control, fsm_config0);
      evolve_led4(fsm_led4, fsm_control);
      evolve_led5(fsm_led5, fsm_control);
      evolve_led6(fsm_led6, fsm_control);
      evolve_led7(fsm_led7, fsm_config0, fsm_config1, fsm_config2, fsm_config3);
      

      // Update the states
      set_state(fsm_s1, fsm_s1.new_state);
      set_state(fsm_s2, fsm_s2.new_state);
      set_state(fsm_config0, fsm_config0.new_state);
      set_state(fsm_config1, fsm_config1.new_state);
      set_state(fsm_config2, fsm_config2.new_state);
      set_state(fsm_config3, fsm_config3.new_state);
      set_state(fsm_pause, fsm_pause.new_state);
      set_state(fsm_end, fsm_end.new_state);
      set_state(fsm_control, fsm_control.new_state);
      set_state(fsm_led1,fsm_led1.new_state);
      set_state(fsm_led2,fsm_led2.new_state);
      set_state(fsm_led3,fsm_led3.new_state);
      set_state(fsm_led4,fsm_led4.new_state);
      set_state(fsm_led5,fsm_led5.new_state);
      set_state(fsm_led6,fsm_led6.new_state);
      set_state(fsm_led7,fsm_led7.new_state);      
      
      //actions of the states
      act_config1(fsm_config1);
      act_pause(fsm_pause);
      act_control(fsm_control);
      act_leds();
    
      // DEBUGGING 
      if(LED_2 == 1){
        digitalWrite(LED2_pin, HIGH);
      }
      if(LED_5 == 1){
        digitalWrite(LED5_pin, HIGH);
      }
      if(LED_6 == 1){
        digitalWrite(LED6_pin, HIGH);
      }
      Serial.print("fsm_s1.state: ");
      Serial.print(fsm_s1.state);
      Serial.print("S1: ");
      Serial.print(s1);

      // digitalWrite(LED1_pin, HIGH);
      // digitalWrite(LED2_pin, HIGH);
      // digitalWrite(LED3_pin, HIGH);
      // digitalWrite(LED4_pin, HIGH);
      // digitalWrite(LED5_pin, HIGH);
      // digitalWrite(LED6_pin, HIGH);
      // digitalWrite(LED7_pin, HIGH);



    /*
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
      Serial.print(" m: ");
      Serial.print(m);
      Serial.print(" fsm_config1.state: ");
      Serial.print(fsm_config1.state);
      Serial.print(" fsm_config2.state: ");
      Serial.print(fsm_config2.state);
      Serial.print("fsm_config3.state: ");
      Serial.print(end);
      Serial.print(" pause: ");
      Serial.print(pause);

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
    */   
}