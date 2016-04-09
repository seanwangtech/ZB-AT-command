
/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"

#include "zcl.h"
#include "zcl_ha.h"
#include "AT_uart.h"
#include "AT_ZCL.h" 

#include "onboard.h"

/* HAL */
#include "hal_led.h"
#include "hal_key.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * SIMPLE DESCRIPTOR
 */

#define AT_ZCL_DEVICE_VERSION     0
#define AT_ZCL_FLAGS              0

SimpleDescriptionFormat_t AT_ZCL_SimpleDesc =
{
  AT_ZCL_ENDPOINT,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,                     //  uint16 AppProfId[2];
  ZCL_HA_DEVICEID_TEST_DEVICE,           //  uint16 AppDeviceId[2];
  AT_ZCL_DEVICE_VERSION,            //  int   AppDevVer:4;
  AT_ZCL_FLAGS,                     //  int   AppFlags:4;
  0,                              //  byte  AppNumInClusters;
  NULL,                           //  byte *pAppInClusterList;
  0,                                //  byte  AppNumInClusters;
  NULL                              //  byte *pAppInClusterList;
};

void AT_ZCL_Init(void )
{
  // This app is part of the Home Automation Profile
  zclHA_Init( &AT_ZCL_SimpleDesc );
}







