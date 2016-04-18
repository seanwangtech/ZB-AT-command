
#ifndef AT_ZCL_ONOFFL_H
#define AT_ZCL_ONOFFL_H

#include "zcl.h"


#define AT_ZCL_ONOFFL_ENDPOINT            21
#define AT_ZCL_ONOFFL_MAX_ATTRIBUTES      10

#define AT_ZCL_GEN_OFF                       0x00
#define AT_ZCL_GEN_ON                        0x01
#define AT_ZCL_ONOFF_UPDATE_TIMEOUT_VALUE      300      //the unit is seconds. 10 for 10 second£¬300 for 5 mins

#define AT_ZCL_ONOFFL_IDENTIFY_TIMEOUT_EVT    0x0001
#define AT_ZCL_ONOFFL_WDG_TIMEOUT_EVT         0x0002   //for watch dog of switch
#define AT_ZCL_ONOFF_UPDATE_TIMEOUT_EVT      0x0008    //this is for send update to COOR
/*********************************************************************
 * Global variable
 */

extern SimpleDescriptionFormat_t AT_ZCL_ONOFFL_SimpleDesc;
extern CONST zclAttrRec_t AT_ZCL_ONOFFL_Attrs[];
extern uint16 AT_ZCL_ONOFFL_IdentifyTime;
extern uint8  AT_ZCL_ONOFFL_OnOff;


/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void AT_ZCL_ONOFFL_Init( byte task_id );

/*
 *  Event Process for the task
 */
extern UINT16 AT_ZCL_ONOFFL_event_loop( byte task_id, UINT16 events );

extern void AT_ZCL_ONOFFL_EP_ENABLE( bool isEnable);

#endif