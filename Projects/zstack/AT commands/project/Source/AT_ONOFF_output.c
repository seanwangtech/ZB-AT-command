/**************************************************************************************************
  Filename:       AT_ONOFF_output.c

  Description:    AT command module
  Author:         Xiao Wang
**************************************************************************************************/


#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "AT_ONOFF_output.h"
#include "At_include.h"
#include "hal_led.h"


const cId_t AT_ONOFF_OUTPUT_ClusterList[AT_ONOFF_OUTPUT_MAX_CLUSTERS] =
{
 AT_ONOFF_OUTPUT_Basic_CLUSTERID,                       
 AT_ONOFF_OUTPUT_Power_configuration_CLUSTERID,         
 AT_ONOFF_OUTPUT_Device_temperature_cfg_CLUSTERID,      
 AT_ONOFF_OUTPUT_Identify_CLUSTERID ,                   
 AT_ONOFF_OUTPUT_Group_CLUSTERID    ,                  
 AT_ONOFF_OUTPUT_Scenes_CLUSTERID    ,            
 AT_ONOFF_OUTPUT_On_Off_CLUSTERID ,
};

const SimpleDescriptionFormat_t AT_ONOFF_OUTPUT_SimpleDesc =
{
  AT_ONOFF_OUTPUT_ENDPOINT,              //  int Endpoint;
  AT_ONOFF_OUTPUT_PROFID,                //  uint16 AppProfId[2];
  AT_ONOFF_OUTPUT_DEVICEID,              //  uint16 AppDeviceId[2];
  AT_ONOFF_OUTPUT_DEVICE_VERSION,        //  int   AppDevVer:4;
  AT_ONOFF_OUTPUT_FLAGS,                 //  int   AppFlags:4;
  AT_ONOFF_OUTPUT_MAX_CLUSTERS,                                     //  uint8  AppNumInClusters;
  (cId_t *)AT_ONOFF_OUTPUT_ClusterList,                            //  uint8 *pAppInClusterList;
  AT_ONOFF_OUTPUT_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)AT_ONOFF_OUTPUT_ClusterList   //  uint8 *pAppInClusterList;
};

endPointDesc_t AT_ONOFF_OUTPUT_epDesc;
uint8 AT_ONOFF_OUTPUT_TransID;  // This is the unique message ID (counter)

void AT_ONOFF_OUTPUT_Register(uint8 *task_id){
 // Fill out the endpoint description.
  AT_ONOFF_OUTPUT_epDesc.endPoint = AT_ONOFF_OUTPUT_ENDPOINT;
  AT_ONOFF_OUTPUT_epDesc.task_id = task_id;
  AT_ONOFF_OUTPUT_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&AT_ONOFF_OUTPUT_SimpleDesc;
  AT_ONOFF_OUTPUT_epDesc.latencyReq = noLatencyReqs;
  
  // Register the endpoint description with the AF
  afRegister( &AT_ONOFF_OUTPUT_epDesc );
}


void AT_ONOFF_OUTPUT_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  
  uint8 on_off = (uint8)pkt->cmd.Data[0];
  switch ( pkt->clusterId )
  {
    case AT_ONOFF_OUTPUT_Basic_CLUSTERID:
      break;   
    case AT_ONOFF_OUTPUT_Power_configuration_CLUSTERID:
      break;
    case AT_ONOFF_OUTPUT_Device_temperature_cfg_CLUSTERID:
      break;
    case AT_ONOFF_OUTPUT_Identify_CLUSTERID:
      break;
    case AT_ONOFF_OUTPUT_Group_CLUSTERID :
      break;
    case AT_ONOFF_OUTPUT_Scenes_CLUSTERID :
      break;
      
    case AT_ONOFF_OUTPUT_On_Off_CLUSTERID:
      switch(on_off){
        case 0:
          HalLedSet( HAL_LED_1,HAL_LED_MODE_OFF);
          break;
        case 1:
          HalLedSet( HAL_LED_1,HAL_LED_MODE_ON);
          break;
        case 2:
          HalLedSet( HAL_LED_1,HAL_LED_MODE_TOGGLE);
          break;
          
      }
      break;
  }
}

uint8 AT_AF_error_state;
void AT_ONOFF_OUTPUT_Cmd_RONOFF(afAddrType_t* dstAddr,uint8 on_off){
   if ( (AT_AF_error_state=AF_DataRequest( dstAddr, & AT_ONOFF_OUTPUT_epDesc,
                       AT_ONOFF_OUTPUT_On_Off_CLUSTERID,
                       1,
                       (uint8*)&on_off,
                       &AT_ONOFF_OUTPUT_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS )) == afStatus_SUCCESS )
  {
    AT_OK();
  }
  else
  {
    AT_ERROR(AT_AF_error_state);
  }
}