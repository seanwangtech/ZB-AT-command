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
#include "AT_ONOFF_output.h"
#include "AT_include.h"
#include "AT_ZCL_temperature.h"
#include "AT_ZCL_ONOFF.h"
#include "AT_ZCL_ONOFF_SWITCH.h"


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
 * @fn      AT_ZCL_EP_ENABLE
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
