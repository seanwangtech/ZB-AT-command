
#ifndef AT_ZCL_ONOFF_SWITCH_H
#define AT_ZCL_ONOFF_SWITCH_H

#include "zcl.h"


#define AT_ZCL_ONOFF_SWITCH_ENDPOINT            5
#define AT_ZCL_ONOFF_SWITCH_MAX_ATTRIBUTES      12

#define AT_ZCL_GEN_OFF                       0x00
#define AT_ZCL_GEN_ON                        0x01

#define AT_ZCL_ONOFF_SWITCH_IDENTIFY_TIMEOUT_EVT    0x0001
#define AT_ZCL_ONOFF_SWITCH_UPDATE_EVT              0x0002
/*********************************************************************
 * Global variable
 */

extern SimpleDescriptionFormat_t AT_ZCL_ONOFF_SWITCH_SimpleDesc;
extern CONST zclAttrRec_t AT_ZCL_ONOFF_SWITCH_Attrs[];
extern uint16 AT_ZCL_ONOFF_SWITCH_IdentifyTime;
extern uint8  AT_ZCL_ONOFF_SWITCH_action;
extern uint8  AT_ZCL_ONOFF_SWITCH_OnOff;


/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void AT_ZCL_ONOFF_SWITCH_Init( byte task_id );

/*
 *  Event Process for the task
 */
extern UINT16 AT_ZCL_ONOFF_SWITCH_event_loop( byte task_id, UINT16 events );

extern void AT_ZCL_ONOFF_SWITCH_EP_ENABLE( bool isEnable);
#endif