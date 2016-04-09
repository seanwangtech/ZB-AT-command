/**************************************************************************************************
  Filename:       AT_App.c

  Description:    AT command module
  Author:         Xiao Wang
**************************************************************************************************/
#include "OSAL.h"
#include "OSAL_NV.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"

#include "OnBoard.h"
#include "ZDObject.h"

#include "zcl.h"
#include "AT_uart.h"
#include "AT_ONOFF_output.h"
#include "AT_AF.h"

#include "AT_ZCL.h" 
#include "AT_ZCL_temperature.h"
#include "AT_ZCL_ONOFF.h"
#include "AT_ZCL_ONOFF_SWITCH.h"

#include "AT_App.h"
#include "AT_ZDO.h"
#include "AT_printf.h"


uint8 AT_App_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // AT_App_Init() is called.

epList_t *removedEPList = NULL;  
/*********************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************/

uint8 AT_handleEntryEvt(void);
void AT_handleZCL_EP(void);


static void AT_ZCL_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg);
static uint8 AT_ZCL_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 AT_ZCL_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
static uint8 AT_ZCL_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );



void AT_App_Init(uint8 task_id ){
  AT_App_TaskID=task_id;
  //ZDO_RegisterForZDOMsg( task_id, Device_annce );
  AT_ZDO_Register(&AT_App_TaskID);
  
  AT_ONOFF_OUTPUT_Register(&AT_App_TaskID);
  
  //register AT command AF layer application
  AT_AF_Register(&AT_App_TaskID);
  
  
  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg(task_id);
  //Initialize the AT ZCL to send AT ZCL command
  AT_ZCL_Init();
  
  
  /*initialise AT_Uart*/
  AT_UartInit(task_id);
  //HalUARTWrite ( 0, "initial very good!", sizeof("initial very good!") );

#if AT_MSG_SEND_MODE
  AT_UartRegisterTaskID( task_id);
#endif
  
  //NLME_PermitJoiningRequest(10);      //permit joining in 16 seconds 
  
  /*test zone*/
  ZDO_RegisterForZDOMsg( task_id, Power_Desc_req );
  ZDO_RegisterForZDOMsg( task_id, Power_Desc_rsp );
  
  
  osal_set_event(task_id, AT_ENTRY_EVENT);
}


uint16 AT_App_ProcessEvent( uint8 task_id, uint16 events ){
  
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( AT_App_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        // Received when a messages is received (OTA) for this endpoint
        case AF_INCOMING_MSG_CMD:
          switch(MSGpkt->endPoint)
          {
            case AT_ONOFF_OUTPUT_ENDPOINT:
              AT_ONOFF_OUTPUT_MessageMSGCB( MSGpkt );
              break;
            case AT_AF_ENDPOINT:
              AT_AF_MessageMSGCB( MSGpkt );
              break;
              
            default:
              break;
          }
          break;
          
        case ZCL_INCOMING_MSG:
          // Incoming ZCL Foundation command/response messages
          AT_ZCL_ProcessIncomingMsg( (zclIncomingMsg_t *)MSGpkt );
          break;
          
          
        // Received whenever the device changes state in the network
        case ZDO_STATE_CHANGE:
          break;
#if AT_MSG_SEND_MODE
        case AT_CMD_MSG:
          //HalUARTWrite ( 0, " received \n", sizeof(" received \n") );
          AT_HandleCMD(((atOSALSerialData_t*) MSGpkt)->msg);
         break;
#endif
        case ZDO_CB_MSG:
            AT_ZDO_ProcessMsgCBs( (zdoIncomingMsg_t *)MSGpkt );
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next - if one is available
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( AT_App_TaskID );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  else if( events & AT_ENTRY_EVENT){
    AT_handleZCL_EP();
    AT_handleEntryEvt();
    //zgWriteStartupOptions( ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );
    //zgWriteStartupOptions( ZG_STARTUP_CLEAR, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );
    return (events ^ AT_ENTRY_EVENT);
  }else if( events & AT_RESET_EVENT ){
    SystemReset(); 
  }

  // Discard unknown events
  return 0;
}


void AT_handleZCL_EP(void){
  const uint8 AT_CMD_EP_ARRAY[]=AT_CMD_EPs;
  uint8 i;
  uint8 enable;
  for(i=0;i<sizeof(AT_CMD_EP_ARRAY);i++){
    //read ZCL NV
    AT_NV_ZCL_readEPStatus(i,&enable);
    //if the NV indicate the enpoint is not enable, remove it
    if(enable == 1) {
      
      //enable the ZCL layer
      AT_ZCL_EP_ENABLE( 1,AT_CMD_EP_ARRAY[i]);
    }else{
      AT_af_remove_ep(AT_CMD_EP_ARRAY[i]);
      //enable the ZCL layer
      AT_ZCL_EP_ENABLE( 0,AT_CMD_EP_ARRAY[i]);
    }
  }
}

uint8 AT_handleEntryEvt(void){
  
  //zgWriteStartupOptions( ZG_STARTUP_CLEAR, ZCD_STARTOPT_DEFAULT_NETWORK_STATE );
  uint8 status;
  uint8 startupOptions = 0;

  status = osal_nv_read( ZCD_NV_STARTUP_OPTION,
                0,
                sizeof( startupOptions ),
                &startupOptions );

  if ( status == ZSUCCESS )
  {
    
    if(startupOptions & ZCD_STARTOPT_DEFAULT_NETWORK_STATE){
      
      startupOptions &= (!ZCD_STARTOPT_DEFAULT_NETWORK_STATE);
      
      status = osal_nv_write( ZCD_NV_STARTUP_OPTION,
                 0,
                 sizeof( startupOptions ),
                 &startupOptions );
     }
   }
   return status;
}




/*******************************************************
the reverse proces of afRegister() function
********************************************************/
afStatus_t AT_af_remove_ep(uint8 EndPoint){
  
  
  epList_t *epSearchpre;
  epList_t *epSearchcur;

  //endPoint range:(1-240)
  if(EndPoint<1 || EndPoint>240 ) return ( afStatus_INVALID_PARAMETER);
  
  // Start at the beginning
  epSearchcur = epList;
  epSearchpre = NULL;

  // Look through the list until the end
  while ( epSearchcur )
  {
    // Is there a match?
    if ( epSearchcur->epDesc->endPoint == EndPoint )
    {
      //the enpoint is found
      //delete the epSearchcur
      if(epSearchpre==NULL)//check whether the aim is the first one
          epList = epSearchcur->nextDesc;
      else epSearchpre->nextDesc = epSearchcur->nextDesc;
      
      epSearchcur->nextDesc=removedEPList;
      removedEPList = epSearchcur;
      //osal_mem_free(epSearchcur);
      return afStatus_SUCCESS;
    }
    else
      
      epSearchpre = epSearchcur;
      epSearchcur = epSearchcur->nextDesc;  // Next entry
  }
  return ( afStatus_INVALID_PARAMETER);
}

/*******************************************************
the afRegister() function for register an endpoint with provided endpoint
********************************************************/
afStatus_t AT_af_register_ep(uint8 EndPoint){
  
  
  epList_t *epSearchpre;
  epList_t *epSearchcur;
  //endPoint range:(1-240)
  if(EndPoint<1 || EndPoint>240 ) return ( afStatus_INVALID_PARAMETER);
  
  // Start at the beginning
  epSearchcur = removedEPList;
  epSearchpre = NULL;
  
  // Look through the list until the end
  while ( epSearchcur )
  {
    // Is there a match?
    if ( epSearchcur->epDesc->endPoint == EndPoint )
    {
      //the enpoint is found
      //delete the epSearchcur
      if(epSearchpre==NULL)//check whether the aim is the first one
          removedEPList = epSearchcur->nextDesc;
      else epSearchpre->nextDesc = epSearchcur->nextDesc;
      
      epSearchcur->nextDesc=epList;
      epList = epSearchcur;
      
      //osal_mem_free(epSearchcur);
      return afStatus_SUCCESS;
    }
    else
      
      epSearchpre = epSearchcur;
      epSearchcur = epSearchcur->nextDesc;  // Next entry
  }
  return ( afStatus_INVALID_PARAMETER);
}

epList_t* AT_af_get_ep(uint8 EndPoint){
  
  epList_t *epSearch;
  // Start at the beginning
  epSearch = removedEPList;
  // Look through the list until the end
  while ( epSearch )
  {
    // Is there a match?
    if ( epSearch->epDesc->endPoint == EndPoint )
    {
      return epSearch;
    }
    else
      epSearch = epSearch->nextDesc;  // Next entry
  }
  epSearch = epList;
  // Look through the list until the end
  while ( epSearch )
  {
    // Is there a match?
    if ( epSearch->epDesc->endPoint == EndPoint )
    {
      return epSearch;
    }
    else
      epSearch = epSearch->nextDesc;  // Next entry
  }

  return false;
}



uint8 AT_af_ep_num( void ){
  epList_t *epSearch;
  uint8 cnt=0;
  // Start at the beginning
  epSearch = removedEPList;
  // Look through the list until the end
  while ( epSearch )
  {
      epSearch = epSearch->nextDesc;  // Next entry
      cnt++;
  }
  epSearch = epList;
  // Look through the list until the end
  while ( epSearch )
  {
      epSearch = epSearch->nextDesc;  // Next entry
      cnt++;
  }

  return cnt;
  
}

void AT_af_ep_list( uint8 len, uint8 *list ){
  epList_t *epSearch;
  uint8 cnt=0;
  // Start at the beginning
  epSearch = removedEPList;
  // Look through the list until the end
  while ( epSearch )
  {
      epSearch = epSearch->nextDesc;  // Next entry
      list[cnt] = epSearch->epDesc->endPoint;
      cnt++;
  }
  epSearch = epList;
  // Look through the list until the end
  while ( epSearch )
  {
      epSearch = epSearch->nextDesc;  // Next entry
      list[cnt] = epSearch->epDesc->endPoint;
      cnt++;
  }
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
static void AT_ZCL_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg)
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
 * @fn      AT_ZCL_TEMP_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 AT_ZCL_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;
   
  // Device is notified of the Default Response command.
  (void)pInMsg;
  
  return TRUE; 
}


/*********************************************************************
 * @fn      AT_ZCL_ONOFF_EP_ENABLE
 *
 * @brief   Enable/Disable ZCL endpoint
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  if operation success, return ture. otherwise return false.
 */
bool AT_ZCL_EP_ENABLE( bool isEnable,uint8 endPoint){
  switch(endPoint){
  case AT_ZCL_TEMP_ENDPOINT:
    AT_ZCL_TEMP_EP_ENABLE( isEnable);
    return true;
    break;
    
  case AT_ZCL_ONOFF_ENDPOINT:
    AT_ZCL_ONOFF_EP_ENABLE( isEnable);
    return true;
    break;
    
  case AT_ZCL_ONOFF_SWITCH_ENDPOINT:
    AT_ZCL_ONOFF_SWITCH_EP_ENABLE( isEnable);
    return true;
    break;
  }
  return false;
}
