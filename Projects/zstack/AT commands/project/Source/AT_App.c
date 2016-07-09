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
#include "AddrMgr.h"
#include "OnBoard.h"
#include "ZDObject.h"

#include "zcl.h"
#include "AT_ONOFF_output.h"
#include "AT_include.h"
#include "AT_IR.h"

#include "AT_printf.h"


uint8 AT_App_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // AT_App_Init() is called.

epList_t *removedEPList = NULL;  
AT_App_Cmd_POWER_SAVING_EXP_t AT_App_Cmd_POWER_SAVING_EXP={0,0,0}; 
/*********************************************************************
 * Local VARIABLES
 */
static uint16 UPDATE_timer = AT_UPDATE_TIMEOUT_VALUE;  //update the sensor status after xxx seconds of the device starting

/*********************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************/

uint8 AT_handleEntryEvt(void);
void AT_handleZCL_EP(void);
void AT_App_HandleKeys( uint8 shift, uint8 keys );
static void AT_App_process_Power_Saving_Exp_Evt(void);
static void AT_App_Clean_dead_ED(void);

//initialize this task after the ZCL initialization, I have encounter the mistake that I initialized
//the task before the ZCL. this lead all the zcl layer work innormal.
void AT_App_Init(uint8 task_id ){
  AT_App_TaskID=task_id;
  //ZDO_RegisterForZDOMsg( task_id, Device_annce );
  AT_ZDO_Register(&AT_App_TaskID);
  
  AT_ONOFF_OUTPUT_Register(&AT_App_TaskID);
  
  //register AT command AF layer application
  AT_AF_Register(&AT_App_TaskID);
  //register AT command IR application
  AT_IR_Register(&AT_App_TaskID);
  
  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg(task_id);
  //Initialize the AT ZCL to send AT ZCL command
  AT_ZCL_Init();
  
  
  /*initialise AT_Uart*/
  AT_UartInit(task_id);
  
  /*initialise second serial port*/
   AT_UartInit2 ( task_id );
#if AT_MSG_SEND_MODE
  AT_UartRegisterTaskID( task_id);
#endif
  
#if defined ( HOLD_AUTO_START )
  ZDOInitDevice(0);
#endif
  
  // Register for all key events - This app will handle all key events
  HalKeyConfig (1, NULL);//enable interrupt
  RegisterForKeys( task_id );
  
  NLME_PermitJoiningRequest(0);      //Do not permit joining
  
  osal_set_event(task_id, AT_ENTRY_EVENT); 
  
  //initialize update-to-COOR timer
  if(AT_UPDATE_TIMEOUT_VALUE>0 )
    osal_start_reload_timer( AT_App_TaskID,AT_UPDATE_TIMEOUT_EVT, 1000 ); //reload timer 1 second

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
            case AT_IR_ENDPOINT:
              AT_IR_MessageMSGCB( MSGpkt );
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
            osal_start_timerEx(task_id,AT_DEV_REPORT_EVENT, 100 );
            //clean the dead ED when network OK;
            osal_set_event( AT_App_TaskID,AT_Clean_dead_ED_EVENT); 
            //osal_start_timerEx( AT_App_TaskID,AT_Clean_dead_ED_EVENT,5000);
          }
          else  if (((osal_event_hdr_t *) MSGpkt)->status == DEV_HOLD ||
                  ((osal_event_hdr_t *) MSGpkt)->status == DEV_INIT)
          {
            HalLedSet ( HAL_LED_2, HAL_LED_MODE_FLASH );
          }
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
  else if( events & AT_DEV_REPORT_EVENT ){
    AT_Cmd_ANNCE(0,"\r");//announce in the network
    AT_AF_send_DEV_REPORT();
    return (events ^ AT_DEV_REPORT_EVENT );
  }
  else if( events & AT_Clean_dead_ED_EVENT ){
    AT_App_Clean_dead_ED();
    return (events ^ AT_Clean_dead_ED_EVENT);
  }
  //deal with the update timer event
  if ( events & AT_UPDATE_TIMEOUT_EVT )
  {
    UPDATE_timer--;
    if(UPDATE_timer==0){
      UPDATE_timer= AT_UPDATE_TIMEOUT_VALUE;
      //afStatus_t AT_AF_send_update(uint8 ep,uint16 clusterId,uint16 attrID,uint8 dataType, uint8* data,uint8 status); //status == 0, indicate succeed
      const uint8 ONOFF=0; //the IR ROUTER will always on, send the update just for ZigBee keep alive
      AT_AF_send_update(0x8D,ZCL_CLUSTER_ID_GEN_ON_OFF,
                         ATTRID_ON_OFF,ZCL_DATATYPE_ENUM8,
                         (uint8*) &ONOFF,0);//time up, so send update
    }
    return ( events ^ AT_UPDATE_TIMEOUT_EVT );
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
      AT_Cmd_ANNCE(0,"\r");//announce in the network
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
     HalLedBlink( HAL_LED_2, 4, 50, 250 );
    }else if(keys & HAL_KEY_SW_2){
     HalLedBlink( HAL_LED_2, 4, 50, 250 );
    }
    break;
  case 1: //pressing time during 5 to 10 seconds
    
    if ( keys & HAL_KEY_SW_1 )
    {
      NLME_PermitJoiningRequest(30);//allow join in 30 seconds
      HalLedBlink( HAL_LED_2, 30, 10, 1000 );
    }
    break;
  case 2: //pressing time during 10 to 15 
    AT_Cmd_AT_F(0, "\r");//recover factory setting, so it will search PAN which has the strongest singal and join that PAN
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


/*********************************************************************
for clean the dead ED after a few time of start up.
to some extent, 
*************************************************************************/
static void AT_App_Clean_dead_ED(void){
  static uint16 times =0xffff;
  if(times==0xffff){
    //first broad cast a discover message and the TTL is 1 to test linkLQI
    //propare for clean dead end device
    //help check whether the device polling the parent device in a period time
    afAddrType_t AT_AF_broad_addr={
      {0xffff},                       //addr
      (afAddrMode_t)AddrBroadcast,              //addr mode
      AT_AF_ENDPOINT,                         //end point
      NULL                                    //PAN ID
    };
    AT_AF_hdr hdr;
    hdr.cmd = AT_AF_Cmd_req;
    AF_DataRequest( &AT_AF_broad_addr, &AT_AF_epDesc,
                         AT_AF_DISC_ASSO_CLUSTERID,
                         sizeof(AT_AF_hdr),
                         (uint8*) &hdr,
                         &AT_AF_TransID,
                         AF_DISCV_ROUTE,
                         AF_DEFAULT_RADIUS );
    osal_start_timerEx( AT_App_TaskID,AT_Clean_dead_ED_EVENT,60000);//start clean program in 1 mins
    times =0;
  }else{
    times++;
    if(times==AT_ED_DEAD_Period) {
      // AssocReset();
      //.devType==ZDP_MGMT_DT_ENDDEV)
      associated_devices_t *aDevice;
      // Get the number of associated items
      uint8 aItems = (uint8) AssocCount( PARENT, CHILD_FFD_RX_IDLE );
      uint8 i=0;
      for(;i<aItems;i++){
        // get associated device
        aDevice = AssocFindDevice(i);
        if(AssocIsRFChild( aDevice->shortAddr ) && aDevice->linkInfo.rxLqi <2){
            // set extented address
            AddrMgrEntry_t  nwkEntry;
            nwkEntry.user    = ADDRMGR_USER_DEFAULT;
            nwkEntry.nwkAddr = aDevice->shortAddr;
            if ( AddrMgrEntryLookupNwk( &nwkEntry ) == TRUE )
            {
              AssocRemove( nwkEntry.extAddr );
              //when a device removed, the index will shifted by decrease 1
              //as well as the aItems also decrease 1
              i--;
              aItems--;
            }
        }
      }
    }else{
      osal_start_timerEx( AT_App_TaskID,AT_Clean_dead_ED_EVENT,60000);
    }
  }
}