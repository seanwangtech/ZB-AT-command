#ifndef AT_TOUCH_reset_H__
#define AT_TOUCH_reset_H__

#define AT_TOUCH_RESET_PIN    P1_5
#define AT_TOUCH_RESET_INIT() { AT_TOUCH_RESET_PIN=0; P1DIR |= (1<<5);}
#define AT_TOUCH_DISABLE()    AT_TOUCH_RESET_PIN=0
#define AT_TOUCH_ENABLE()    AT_TOUCH_RESET_PIN=1

extern void switch2_init(void);
extern  uint8 switch2_status(void);
#endif