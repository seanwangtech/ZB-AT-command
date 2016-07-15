
#include "zcl.h"

//#define AT_ZCL_IR_ENDPOINT               141
#define AT_ZCL_IR_MAX_ATTRIBUTES         8

#define AT_ZCL_GEN_OFF                       0x00
#define AT_ZCL_GEN_ON                        0x01

#define AT_ZCL_IR_IDENTIFY_TIMEOUT_EVT    0x0001
/*********************************************************************
 * Global variable
 */

extern SimpleDescriptionFormat_t AT_ZCL_IR_SimpleDesc;
extern CONST zclAttrRec_t AT_ZCL_IR_Attrs[];
extern uint16 AT_ZCL_IR_IdentifyTime;
extern uint8  AT_ZCL_IR_OnOff;

/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void AT_ZCL_IR_Init( byte task_id );

/*
 *  Event Process for the task
 */
extern UINT16 AT_ZCL_IR_event_loop( byte task_id, UINT16 events );

extern void AT_ZCL_IR_EP_ENABLE( bool isEnable);