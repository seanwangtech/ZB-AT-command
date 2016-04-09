
#ifndef AT_ZCL_ONOFF_SWITCH2_H
#define AT_ZCL_ONOFF_SWITCH2_H

#include "zcl.h"


#define AT_ZCL_ONOFF_SWITCH2_ENDPOINT            7
#define AT_ZCL_ONOFF_SWITCH2_MAX_ATTRIBUTES      12

#define AT_ZCL_GEN_OFF                       0x00
#define AT_ZCL_GEN_ON                        0x01

#define AT_ZCL_ONOFF_SWITCH2_IDENTIFY_TIMEOUT_EVT    0x0001
#define AT_ZCL_ONOFF_SWITCH2_UPDATE_EVT              0x0002
/*********************************************************************
 * Global variable
 */

extern SimpleDescriptionFormat_t AT_ZCL_ONOFF_SWITCH2_SimpleDesc;
extern CONST zclAttrRec_t AT_ZCL_ONOFF_SWITCH2_Attrs[];
extern uint16 AT_ZCL_ONOFF_SWITCH2_IdentifyTime;
extern uint8  AT_ZCL_ONOFF_SWITCH2_action;
extern uint8  AT_ZCL_ONOFF_SWITCH2_OnOff;


/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void AT_ZCL_ONOFF_SWITCH2_Init( byte task_id );

/*
 *  Event Process for the task
 */
extern UINT16 AT_ZCL_ONOFF_SWITCH2_event_loop( byte task_id, UINT16 events );

extern void AT_ZCL_ONOFF_SWITCH2_EP_ENABLE( bool isEnable);
#endif