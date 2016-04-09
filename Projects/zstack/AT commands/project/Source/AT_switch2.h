#ifndef AT_SWITCH2_H__
#define AT_SWITCH2_H__


#define SWITCH_ONOFF_IO_PIN2            P0_6
#define SWITCH_SET_PIN_INPUT2()         P0DIR &= (~0x40)   //set the PIN to INput
#define SWITCH_ON                      0
#define SWITCH_OFF                     1

extern void switch2_init(void);
extern  uint8 switch2_status(void);
#endif