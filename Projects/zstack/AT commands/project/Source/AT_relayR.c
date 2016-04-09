#include"iocc2530.h"
#include"OnBoard.h"
#include "AT_relayR.h" 


void relayR_on(void);
void relayR_off(void);

void relayR_init(void){
  RELAYR_ONOFF_IO_PIN = RELAYR_OFF;
  RELAYR_SET_PIN_INPUT();
}

extern void relayR_disable(void){
  RELAYR_ONOFF_IO_PIN = RELAYR_OFF;
  RELAYR_SET_PIN_INPUT();
}
extern void relayR_enable(void){
  RELAYR_ONOFF_IO_PIN = RELAYR_OFF;
  RELAYR_SET_PIN_OUTPUT();
}
void relayR_on(void){
  RELAYR_ONOFF_IO_PIN=RELAYR_ON;
}
void relayR_off(void){
  RELAYR_ONOFF_IO_PIN=RELAYR_OFF;
}

