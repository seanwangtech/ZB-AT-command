
#ifndef AT_ZCL_DHT_H
#define AT_ZCL_DHT_H

#include "zcl.h"


#define AT_ZCL_DHT_ENDPOINT            6
#define AT_ZCL_DHT_MAX_ATTRIBUTES      14

#define AT_ZCL_GEN_OFF                       0x00
#define AT_ZCL_GEN_ON                        0x01

#define AT_ZCL_DHT_IDENTIFY_TIMEOUT_EVT    0x0001
#define AT_ZCL_DHT_MEASURE_EVT             0x0002
/*********************************************************************
 * Global variable
 */

extern SimpleDescriptionFormat_t AT_ZCL_DHT_SimpleDesc;
extern CONST zclAttrRec_t AT_ZCL_DHT_Attrs[];
extern uint16 AT_ZCL_DHT_IdentifyTime;
extern uint8  AT_ZCL_DHT_OnOff;
//Tempetrue
extern uint16 AT_ZCL_DHT_T_current;
//Relative Humidity
extern uint16 AT_ZCL_DHT_RH_current;



/*********************************************************************
 * FUNCTIONS
 */

 /*
  * Initialization for the task
  */
extern void AT_ZCL_DHT_Init( byte task_id );

/*
 *  Event Process for the task
 */
extern UINT16 AT_ZCL_DHT_event_loop( byte task_id, UINT16 events );

extern void AT_ZCL_DHT_EP_ENABLE( bool isEnable);

#endif