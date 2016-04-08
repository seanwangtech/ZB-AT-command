#ifndef AT_RELAY_H__
#define AT_RELAY_H__


#define RELAY_ONOFF_IO_PIN            P0_6
#define RELAY_SET_PIN_OUTPUT()        P0DIR |= 0x40   //set the PIN to output
#define RELAY_SET_PIN_INPUT()         P0DIR &= ~0x40   //set the PIN to output
#define RELAY_ON                      0;
#define RELAY_OFF                     1;

extern void relay_init(void);
extern void relay_disable(void);
extern void relay_enable(void);
extern void relay_on(void);
extern void relay_off(void);
#endif