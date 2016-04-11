
#ifndef AT_ZCL_H
#define AT_ZCL_H

#include "zcl.h"
#include "zcl_general.h"
#include "AT_AF.h"

#define AT_ZCL_ENDPOINT            203

typedef void (*AT_ZCL_EP_ENABLE_CB_t) (bool isEnable);
typedef struct AT_ZCL_EP_ENABLE_CB_tag{
  struct AT_ZCL_EP_ENABLE_CB_tag *next;
  uint8 ep;
  AT_ZCL_EP_ENABLE_CB_t CB;
} AT_ZCL_EP_ENABLE_CBs_t;

/*********************************************************************
 * Global variable
 */
extern SimpleDescriptionFormat_t AT_ZCL_SimpleDesc;



/*********************************************************************
 * FUNCTIONS
 */

 /*
  * register the basic end point for AT comand zcl
  */
extern void AT_ZCL_Init(void );

extern void AT_ZCL_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg);

extern bool AT_ZCL_EP_ENABLE( bool isEnable,uint8 endPoint);

extern bool AT_ZCL_EP_ENABLE_Register( uint8 endPoint,AT_ZCL_EP_ENABLE_CB_t CB);


#endif