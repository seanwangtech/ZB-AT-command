#include"iocc2530.h"
#include"OnBoard.h"
#include "AT_switch.h" 

void switch_init(void){
  SWITCH_SET_PIN_INPUT();
}
uint8 switch_status(void){
  if(SWITCH_ONOFF_IO_PIN == SWITCH_ON){
    return 0x00;
  }else if(SWITCH_ONOFF_IO_PIN == SWITCH_OFF){
    return 0x01;
  }else{
    return 0x02;
  }
}

