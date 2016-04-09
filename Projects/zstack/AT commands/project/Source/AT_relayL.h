#ifndef AT_RELAYL_H__
#define AT_RELAYL_H__


#define RELAYL_ONOFF_IO_PIN            P0_4
#define RELAYL_SET_PIN_OUTPUT()        P0DIR |= (1<<4)   //set the PIN to output
#define RELAYL_SET_PIN_INPUT()         P0DIR &= ~(1<<4)   //set the PIN to input
#define RELAYL_ON                      1;
#define RELAYL_OFF                     0;

extern void relayL_init(void);
extern void relayL_disable(void);
extern void relayL_enable(void);
extern void relayL_on(void);
extern void relayL_off(void);
#endif