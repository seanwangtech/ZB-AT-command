#include"iocc2530.h"
#include"OnBoard.h"
#include "AT_switch2.h" 

void switch2_init(void){
  SWITCH_SET_PIN_INPUT2();
}
uint8 switch2_status(void){
  if(SWITCH_ONOFF_IO_PIN2 == SWITCH_ON){
    return 0x00;
  }else if(SWITCH_ONOFF_IO_PIN2 == SWITCH_OFF){
    return 0x01;
  }else{
    return 0x02;
  }
}

