#ifndef AT_RELAYR_H__
#define AT_RELAYR_H__


#define RELAYR_ONOFF_IO_PIN            P0_0
#define RELAYR_SET_PIN_OUTPUT()        P0DIR |= 0x01   //set the PIN to output
#define RELAYR_SET_PIN_INPUT()         P0DIR &= ~0x01   //set the PIN to input
#define RELAYR_ON                      1;
#define RELAYR_OFF                     0;

extern void relayR_init(void);
extern void relayR_disable(void);
extern void relayR_enable(void);
extern void relayR_on(void);
extern void relayR_off(void);
#endif