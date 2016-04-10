
#ifndef AT_ZCL_TEMPERATURE_H
#define AT_ZCL_TEMPERATURE_H

#include "zcl.h"


#define AT_ZCL_TEMP_ENDPOINT            4
#define AT_ZCL_TEMP_MAX_ATTRIBUTES      12

#define TEMP_OFF                       0x00
#define TEMP_ON                        0x01

#define AT_ZCL_TEMP_UPDATE_TIMEOUT_VALUE    300      //the unit is seconds. 10 for 10 second£¬300 for 10 mins

#define AT_ZCL_TEMP_IDENTIFY_TIMEOUT_EVT    0x0001
#define AT_ZCL_TEMP_TEMP_MEASURE_EVT        0x0002
#define AT_ZCL_TEMP_UPDATE_TIMEOUT_EVT      0x0004
/*********************************************************************
 * Global variable
 */

extern SimpleDescriptionFormat_t AT_ZCL_TEMP_SimpleDesc;
extern CONST zclAttrRec_t AT_ZCL_TEMP_Attrs[];
extern int16 AT_ZCL_TEMP_current;
extern uint16 AT_ZCL_TEMP_IdentifyTime;
extern uint8  AT_ZCL_TEMP_OnOff;


/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void AT_ZCL_TEMP_Init( byte task_id );

/*
 *  Event Process for the task
 */
extern UINT16 AT_ZCL_TEMP_event_loop( byte task_id, UINT16 events );
extern void AT_ZCL_TEMP_EP_ENABLE( bool isEnable);

#endif