#include"iocc2530.h"
#include"OnBoard.h"
#include "AT_relay.h" 


void relay_on(void);
void relay_off(void);

void relay_init(void){
  RELAY_ONOFF_IO_PIN = RELAY_OFF;
  RELAY_SET_PIN_INPUT();
}

extern void relay_disable(void){
  RELAY_ONOFF_IO_PIN = RELAY_OFF;
  RELAY_SET_PIN_INPUT();
}
extern void relay_enable(void){
  RELAY_ONOFF_IO_PIN = RELAY_OFF;
  RELAY_SET_PIN_OUTPUT();
}
void relay_on(void){
  RELAY_ONOFF_IO_PIN=RELAY_ON;
}
void relay_off(void){
  RELAY_ONOFF_IO_PIN=RELAY_OFF;
}

