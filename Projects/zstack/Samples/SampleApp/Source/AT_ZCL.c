
/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"

#include "zcl.h"
#include "zcl_ha.h"

#include "At_include.h"

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



/****************************functons**********************/

void AT_ZCL_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg);
static uint8 AT_ZCL_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 AT_ZCL_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg);
static uint8 AT_ZCL_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );

void AT_ZCL_Init(void )
{
  // This app is part of the Home Automation Profile
  zclHA_Init( &AT_ZCL_SimpleDesc );
}




/*********************************************************************
 * @fn      AT_ZCL_ProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  none
 */
void AT_ZCL_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg)
{
  switch ( pInMsg->zclHdr.commandID )
  {

#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
      AT_ZCL_ProcessInReadRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_WRITE    
    case ZCL_CMD_WRITE_RSP:
      AT_ZCL_ProcessInWriteRspCmd( pInMsg );
      break;
#endif
    case ZCL_CMD_DEFAULT_RSP:
      AT_ZCL_ProcessInDefaultRspCmd( pInMsg );
      break;
      
    default:
      AT_ZCL_ProcessInDefaultRspCmd( pInMsg );
      break;
  }
  
  if ( pInMsg->attrCmd )
    osal_mem_free( pInMsg->attrCmd );
}


#ifdef ZCL_READ
/*********************************************************************
 * @fn      AT_ZCL_ProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 AT_ZCL_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;

  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  
  char str[17];
  AT_RESP_START();
  for (i = 0; i < readRspCmd->numAttr; i++)
  {
        AT_RESP("RESPATR:",8);
        //node id
        if(pInMsg->srcAddr.addrMode==(afAddrMode_t)Addr16Bit){
          AT_Int16toChar(pInMsg->srcAddr.addr.shortAddr,str);
          AT_RESP(str,4);
        }else{
          AT_EUI64toChar(pInMsg->srcAddr.addr.extAddr,str);
          AT_RESP(str,16);
        }
        AT_RESP(",",1);
        
        //End ponit
        AT_Int8toChar(pInMsg->srcAddr.endPoint,str);
        AT_RESP(str,2);
        AT_RESP(",",1);
        
        //Cluster ID
        AT_Int16toChar(pInMsg->clusterId,str);
        AT_RESP(str,4);
        AT_RESP(",",1);
        
        //Attribute ID
        AT_Int16toChar(readRspCmd->attrList[i].attrID,str);
        AT_RESP(str,4);
        AT_RESP(",",1);
        
        //status
        AT_Int8toChar(readRspCmd->attrList[i].status,str);
        AT_RESP(str,2);
        AT_RESP(",",1);

        //data type
        AT_Int8toChar(readRspCmd->attrList[i].dataType,str);
        AT_RESP(str,2);
        AT_RESP(",",1);
        
        //ATTR VALUE
        if(readRspCmd->attrList[i].dataType == ZCL_DATATYPE_CHAR_STR){
          AT_RESP(readRspCmd->attrList[i].data +1,readRspCmd->attrList[i].data[0]);        
        }else if(readRspCmd->attrList[i].dataType == ZCL_DATATYPE_DATA16||
                 readRspCmd->attrList[i].dataType == ZCL_DATATYPE_UINT16||
                 readRspCmd->attrList[i].dataType == ZCL_DATATYPE_INT16){
          AT_Int16toChar(*((uint16*)readRspCmd->attrList[i].data),str);
          AT_RESP(str,4);
        }else{
          
          AT_Int8toChar((uint8)readRspCmd->attrList[i].data[0],str);
          AT_RESP(str,2);
        }
        
        if(i<readRspCmd->numAttr-1) AT_NEXT_LINE();
  }
  AT_RESP_END(); 

  return TRUE; 
}
#endif // ZCL_READ



#ifdef ZCL_WRITE
/*********************************************************************
 * @fn      AT_ZCL_ProcessInWriteRspCmd
 *
 * @brief   Process the "Profile" Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 AT_ZCL_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;
  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  
  char str[17];
  AT_RESP_START();
  for (i = 0; i < writeRspCmd->numAttr; i++)
  {
         AT_RESP("WRITEATR:",9);
         
        //node id
        if(pInMsg->srcAddr.addrMode==(afAddrMode_t)Addr16Bit){
          AT_Int16toChar(pInMsg->srcAddr.addr.shortAddr,str);
          AT_RESP(str,4);
        }else{
          AT_EUI64toChar(pInMsg->srcAddr.addr.extAddr,str);
          AT_RESP(str,16);
        }
        AT_RESP(",",1);
        
        //End ponit
        AT_Int8toChar(pInMsg->srcAddr.endPoint,str);
        AT_RESP(str,2);
        AT_RESP(",",1);
        
        //Cluster ID
        AT_Int16toChar(pInMsg->clusterId,str);
        AT_RESP(str,4);
        AT_RESP(",",1);
        
        //Attribute ID
        AT_Int16toChar(writeRspCmd->attrList[i].attrID,str);
        AT_RESP(str,4);
        AT_RESP(",",1);
        
        //status
        AT_Int8toChar(writeRspCmd->attrList[i].status,str);
        AT_RESP(str,2);
        AT_RESP(",",1);
        
        if(i < writeRspCmd->numAttr-1) AT_NEXT_LINE();
  }
  
  AT_RESP_END(); 

  return TRUE; 
}
#endif // ZCL_WRITE



/*********************************************************************
 * @fn      AT_ZCL_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none

    DFTREP:<NodeID>,<EP>,<ClusterID>,<CMD>,<Status> 
          <NodeID> - represents the address of the device which send back response. 
          <EP> is the endpoint where is the response from. 
          <ClusterID> - shows the cluster which the command belong to. 
          <CMD> - is command ID which the default response responds to. 
          <Status> - indicate if the command is implemented successfully or not.

 */
static uint8 AT_ZCL_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;
   
  // Device is notified of the Default Response command.
  //(void)pInMsg;
  zclDefaultRspCmd_t* pRsp = (zclDefaultRspCmd_t*)pInMsg->attrCmd;
  AT_RESP_START();
  if(pInMsg->srcAddr.addrMode == (afAddrMode_t)Addr16Bit){
    printf("DFTREP:%04X,%02X,%04X,%02X,%02X",
           pInMsg->srcAddr.addr.shortAddr,
           pInMsg->srcAddr.endPoint,
           pInMsg->clusterId,
           pRsp->commandID,
           pRsp->statusCode);
  }else{
    printf("DFTREP:UNKNOWN,%02X,%04X,%02X,%02X",
           pInMsg->srcAddr.endPoint,
           pInMsg->clusterId,
           pRsp->commandID,
           pRsp->statusCode);
  }
  AT_RESP_END();
  return TRUE; 
}





