#include"iocc2530.h"
#include"OnBoard.h"
#include "AT_relayL.h" 


void relayL_on(void);
void relayL_off(void);

void relayL_init(void){
  RELAYL_ONOFF_IO_PIN = RELAYL_OFF;
  RELAYL_SET_PIN_INPUT();
}

extern void relayL_disable(void){
  RELAYL_ONOFF_IO_PIN = RELAYL_OFF;
  RELAYL_SET_PIN_INPUT();
}
extern void relayL_enable(void){
  RELAYL_ONOFF_IO_PIN = RELAYL_OFF;
  RELAYL_SET_PIN_OUTPUT();
}
void relayL_on(void){
  RELAYL_ONOFF_IO_PIN=RELAYL_ON;
}
void relayL_off(void){
  RELAYL_ONOFF_IO_PIN=RELAYL_OFF;
}

