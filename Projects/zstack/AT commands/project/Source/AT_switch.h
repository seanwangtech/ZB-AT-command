#ifndef AT_SWITCH_H__
#define AT_SWITCH_H__


#define SWITCH_ONOFF_IO_PIN            P0_4
#define SWITCH_PIN_BV                  (1<<4)
#define SWITCH_SET_PIN_INPUT()         P0DIR &= (~SWITCH_PIN_BV)   //set the PIN to INput
#define SWITCH_PIN_INP                 P0INP                        //for setting to tri-state
#define SWITCH_ON                      1
#define SWITCH_OFF                     0

extern void switch_init(void);
extern  uint8 switch_status(void);
#endif