#ifndef AT_SWITCH_H__
#define AT_SWITCH_H__


#define SWITCH_ONOFF_IO_PIN            P0_5
#define SWITCH_SET_PIN_INPUT()         P0DIR &= (~0x20)   //set the PIN to output
#define SWITCH_ON                      1
#define SWITCH_OFF                     0

extern void switch_init(void);
extern  uint8 switch_status(void);
#endif