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

#include "AT_printf.h"
#include "OSAL_PwrMgr.h"


uint8 AT_App_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // AT_App_Init() is called.

epList_t *removedEPList = NULL;  
AT_App_Cmd_POWER_SAVING_EXP_t AT_App_Cmd_POWER_SAVING_EXP={0,0,0};
/*********************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************/

uint8 AT_handleEntryEvt(void);
void AT_handleZCL_EP(void);
void AT_App_HandleKeys( uint8 shift, uint8 keys );
static void AT_App_process_Power_Saving_Exp_Evt(void);

//initialize this task after the ZCL initialization, I have encounter the mistake that I initialized
//the task before the ZCL. this lead all the zcl layer work innormal.
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
  
#if defined ( HOLD_AUTO_START )
  ZDOInitDevice(0);
#endif
  
  // Register for all key events - This app will handle all key events
  HalKeyConfig (1, NULL);//enable interrupt
  RegisterForKeys( task_id );
  
  NLME_PermitJoiningRequest(0);      //disable permit joining
  
  osal_set_event(task_id, AT_ENTRY_EVENT);
  
  //ninglvfeihong very inmportant for Switch. because the device have to enter power saving mode once power on
  osal_pwrmgr_device( PWRMGR_BATTERY );
  

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
          // If the device has started up, notify the application
          if (((osal_event_hdr_t *) MSGpkt)->status == DEV_END_DEVICE ||
              ((osal_event_hdr_t *) MSGpkt)->status == DEV_ROUTER ||
              ((osal_event_hdr_t *) MSGpkt)->status == DEV_ZB_COORD )
          {
            HalLedSet (HAL_LED_2, HAL_LED_MODE_ON);
          }
          else  if (((osal_event_hdr_t *) MSGpkt)->status == DEV_HOLD ||
                  ((osal_event_hdr_t *) MSGpkt)->status == DEV_INIT)
          {
            HalLedSet ( HAL_LED_2, HAL_LED_MODE_FLASH );
          }
#if AT_MSG_SEND_MODE
        case AT_CMD_MSG:
          //HalUARTWrite ( 0, " received \n", sizeof(" received \n") );
          AT_HandleCMD(((atOSALSerialData_t*) MSGpkt)->msg);
         break;
#endif
        case ZDO_CB_MSG:
            AT_ZDO_ProcessMsgCBs( (zdoIncomingMsg_t *)MSGpkt );
          break;
          
        // Received when a key is pressed
        case KEY_CHANGE:
          AT_App_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
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
    return (events ^ AT_ENTRY_EVENT);
  }
  else if( events & AT_POWER_SAVING_EXP_EVENT){
    AT_App_process_Power_Saving_Exp_Evt();
    return (events ^ AT_POWER_SAVING_EXP_EVENT);
  }else if( events & AT_RESET_EVENT ){
    SystemReset(); 
  }

  // Discard unknown events
  return 0;
}

/*********************************************************
when the device just starts, this function will remove end point from AF layer, according 
to NV record.
***************************************************************/
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
  
  //ninglvfeihong
  AT_Cmd_EPENABLE(0, ":1,1\r");//force to enable switch endpoint,because this is middle switch endpoint
  AT_Cmd_EPENABLE(0, ":1,11\r");//force to enable switch endpoint,because this is left switch endpoint
  AT_Cmd_EPENABLE(0, ":1,21\r");//force to enable switch endpoint,because this is righ switch endpoint
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
    
    if(startupOptions & (ZCD_STARTOPT_DEFAULT_NETWORK_STATE|ZCD_STARTOPT_DEFAULT_CONFIG_STATE)  ){
      
      startupOptions &= (~(ZCD_STARTOPT_DEFAULT_NETWORK_STATE|ZCD_STARTOPT_DEFAULT_CONFIG_STATE));
      status = osal_nv_write( ZCD_NV_STARTUP_OPTION,
                 0,
                 sizeof( startupOptions ),
                 &startupOptions );
     }
   }
   return status;
}

/********************************************************
Handles all key events for this device.
*******************************************************/
void AT_App_HandleKeys( uint8 shift, uint8 keys ){
  switch (shift){
  case 0: //pressing time less than 5 seconds
    if ( keys & HAL_KEY_SW_1 )
    {
      
      
      afAddrType_t dstAddr;
      dstAddr.endPoint = 1;
      //dstAddr.panId =2016;//0;
      dstAddr.addrMode =(afAddrMode_t)Addr16Bit;
      dstAddr.addr.shortAddr=NLME_GetShortAddr();     
      uint8 status;
      //this is allow the socket is locked by parent to prevent child from playing it
      status=zclGeneral_SendOnOff_CmdToggle(AT_ZCL_ENDPOINT,&dstAddr,0,1); //stand for without onoff parameter, toggle
      if(status==ZSUCCESS){
      }else{
        //execute when the node isn't in the PAN
        extern void AT_ZCL_ONOFF_OnOffCB( uint8 cmd );
        AT_ZCL_ONOFF_OnOffCB(2);//toggle switch when the network isn't connected
       
      }
    }
    if( keys & HAL_KEY_SWL )
    {
      
      
      afAddrType_t dstAddr;
      dstAddr.endPoint = 0x11;
      //dstAddr.panId =2016;//0;
      dstAddr.addrMode =(afAddrMode_t)Addr16Bit;
      dstAddr.addr.shortAddr=NLME_GetShortAddr();     
      uint8 status;
      //this is allow the socket is locked by parent to prevent child from playing it
      status=zclGeneral_SendOnOff_CmdToggle(AT_ZCL_ENDPOINT,&dstAddr,0,1); //stand for without onoff parameter, toggle
      if(status==ZSUCCESS){
      }else{
        //execute when the node isn't in the PAN
        extern void AT_ZCL_ONOFFL_OnOffCB( uint8 cmd );
        AT_ZCL_ONOFFL_OnOffCB(2);//toggle switch when the network isn't connected
       
      }
    }
    if( keys & HAL_KEY_SWR )
    {
      
      
      afAddrType_t dstAddr;
      dstAddr.endPoint = 0x21;
      //dstAddr.panId =2016;//0;
      dstAddr.addrMode =(afAddrMode_t)Addr16Bit;
      dstAddr.addr.shortAddr=NLME_GetShortAddr();     
      uint8 status;
      //this is allow the socket is locked by parent to prevent child from playing it
      status=zclGeneral_SendOnOff_CmdToggle(AT_ZCL_ENDPOINT,&dstAddr,0,1); //stand for without onoff parameter, toggle
      if(status==ZSUCCESS){
      }else{
        //execute when the node isn't in the PAN
        extern void AT_ZCL_ONOFFR_OnOffCB( uint8 cmd );
        AT_ZCL_ONOFFR_OnOffCB(2);//toggle switch when the network isn't connected
       
      }
    }
    break;
  case 1: //pressing time during 5 to 10 seconds
    
   if ( keys & HAL_KEY_SW_1 )
    {
      AT_Cmd_ANNCE(0,"\r");//announce in the network
      NLME_PermitJoiningRequest(30);//allow join in 30 seconds
      //build broadcast address
      afAddrType_t AT_AF_broad_addr={
        {AT_AF_GROUP_ID},                       //addr
        (afAddrMode_t)AddrGroup,              //addr mode
        AT_AF_ENDPOINT,                         //end point
        NULL                                    //PAN ID
      };
      AF_DataRequest( &AT_AF_broad_addr, &AT_AF_epDesc,
                         AT_AF_TEST_KEY_CLUSTERID,
                         0,
                         0,
                         &AT_AF_TransID,
                         AF_DISCV_ROUTE,
                         AF_DEFAULT_RADIUS );
      
    }
    break;
  case 2: //pressing time during 10 to 15 
    if ( keys & HAL_KEY_SW_1 )
    {
      AT_Cmd_AT_F(0, "\r");//recover factory setting, so it will search PAN which has the strongest singal and join that PAN
    }
    break;
  default:
    break;
  }
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

/********************************************************************
give all endpoint, indcluding deleted endpoint by AT_af_remove_ep;
*************************************************************************/
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
for power saving experiment command 
Power Saving Experiment PSEXP:<address><count><interval>
*************************************************************************/
uint8 AT_App_Power_saving_exp(AT_App_Cmd_POWER_SAVING_EXP_t* pBuf){
  //Error check
  if(AT_App_Cmd_POWER_SAVING_EXP.count!=0) return AT_isActive_ERROR;
  if(pBuf->count==0 || pBuf->interval==0) return AT_PARA_ERROR;  
  
  
  AT_App_Cmd_POWER_SAVING_EXP.count= pBuf->count;
  AT_App_Cmd_POWER_SAVING_EXP.nwkAddr= pBuf->nwkAddr;
  AT_App_Cmd_POWER_SAVING_EXP.interval= pBuf->interval;
  
  //start timer to start send task
  osal_start_timerEx( AT_App_TaskID, AT_POWER_SAVING_EXP_EVENT, 500 );  
  return AT_NO_ERROR;
}


static void AT_App_process_Power_Saving_Exp_Evt(){
  AT_AF_hdr buf;
  buf.cmd =AT_AT_PSE_EXP_req;
  if(AT_App_Cmd_POWER_SAVING_EXP.count==0) {
    AT_App_Cmd_POWER_SAVING_EXP.interval=0;//not necessay but for robust;
    //send end information
    buf.info =AT_AF_PSE_info_end;
    AT_AF_Cmd_send_simple(AT_App_Cmd_POWER_SAVING_EXP.nwkAddr,AT_AF_POWER_SVING_EXP_CLUSTERID,sizeof(buf), &buf);
    
    HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );
    AT_RESP_START();
    printf("Power Saving Experiment finished");
    AT_RESP_END();
    return;
  }
  HalLedSet ( HAL_LED_1, HAL_LED_MODE_TOGGLE );
  buf.info =AT_AF_PSE_info_ing;
  AT_App_Cmd_POWER_SAVING_EXP.count--;
  AT_AF_Cmd_send_simple(AT_App_Cmd_POWER_SAVING_EXP.nwkAddr,AT_AF_POWER_SVING_EXP_CLUSTERID,sizeof(buf), &buf);
  osal_start_timerEx( AT_App_TaskID, AT_POWER_SAVING_EXP_EVENT, AT_App_Cmd_POWER_SAVING_EXP.interval );
}
/*********************************************************************
for power saving experiment command 
stop experiment
*************************************************************************/
void AT_App_Power_saving_exp_stop(void ){
  AT_App_Cmd_POWER_SAVING_EXP.count=0;
}