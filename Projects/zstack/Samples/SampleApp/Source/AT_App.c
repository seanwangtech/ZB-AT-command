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


#include "AT_uart.h"
#include "AT_ONOFF_output.h"

#include "AT_App.h"


uint8 AT_App_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // AT_App_Init() is called.
/*********************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************/
void AT_ProcessMsgCBs( zdoIncomingMsg_t *inMsg );
uint8 AT_handleEntryEvt(void);
void AT_ProcessNoderDescReq(zdoIncomingMsg_t *inMsg );



void AT_App_Init(uint8 task_id ){
  AT_App_TaskID=task_id;
  //ZDO_RegisterForZDOMsg( task_id, Device_annce );
  ZDO_RegisterForZDOMsg( task_id, IEEE_addr_rsp );
  ZDO_RegisterForZDOMsg( task_id, NWK_addr_rsp );
  ZDO_RegisterForZDOMsg( task_id,  Node_Desc_rsp );  
  AT_ONOFF_OUTPUT_Register(&AT_App_TaskID);
  
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
            default:
              break;
          }
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
            AT_ProcessMsgCBs( (zdoIncomingMsg_t *)MSGpkt );
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



/*********************************************************************
 * @fn          ZDO_ProcessPowerDescReq
 *
 * @brief       This function processes and responds to the
 *              Node_Power_req message.
 *
 * @param       inMsg  - incoming request
 *
 * @return      none
 */


void AT_ProcessPowerDescReq( zdoIncomingMsg_t *inMsg );
void AT_ProcessPowerDescReq( zdoIncomingMsg_t *inMsg )
{
 /* uint16 aoi = BUILD_UINT16( inMsg->asdu[0], inMsg->asdu[1] );
  NodePowerDescriptorFormat_t *desc = NULL;

  if ( aoi == ZDAppNwkAddr.addr.shortAddr )
  {
    desc = &ZDO_Config_Power_Descriptor;
  }

  if ( desc != NULL )
  {
    ZDP_PowerDescMsg( inMsg, aoi, desc );
  }
  else
  {
    ZDP_GenericRsp( inMsg->TransSeq, &(inMsg->srcAddr),
              ZDP_INVALID_REQTYPE, aoi, Power_Desc_rsp, inMsg->SecurityUse );
  }*/
}
void AT_ProcessPowerDescRsp( zdoIncomingMsg_t *inMsg );
void AT_ProcessPowerDescRsp( zdoIncomingMsg_t *inMsg )
{
  ZDO_PowerRsp_t NPRsp;
  ZDO_ParsePowerDescRsp(inMsg,&NPRsp);
  uint8 powerlevel = (uint8) NPRsp.pwrDesc.CurrentPowerSourceLevel;
  AT_ERROR(powerlevel);
 
}


void AT_ProcessMsgCBs( zdoIncomingMsg_t *inMsg ){
  switch ( inMsg->clusterID )
  {
    case NWK_addr_rsp:
    case IEEE_addr_rsp:
      {
        ZDO_NwkIEEEAddrResp_t *pAddrRsp;
        pAddrRsp = ZDO_ParseAddrRsp( inMsg );
        if ( pAddrRsp )
        {
          if ( pAddrRsp->status == ZSuccess )
          {
            uint8 i;
            char str[16];
            AT_RESP_START();
            
            AT_Int16toChar(pAddrRsp->nwkAddr,str);
            AT_RESP(str,4);
            AT_RESP(",",1);
            AT_EUI64toChar(pAddrRsp->extAddr,str);
            AT_RESP(str,16);
            for( i=0;i<pAddrRsp->numAssocDevs;i++){
              AT_NEXT_LINE();
              AT_Int16toChar(pAddrRsp->devList[i],str);
              AT_RESP(str,4);
            }
            AT_RESP_END();
            AT_OK();
          }
          osal_mem_free( pAddrRsp );
        }
      }
      break;
    case Node_Desc_rsp:
      AT_ProcessNoderDescReq(inMsg );
      break;
    case Power_Desc_req:
      AT_ProcessPowerDescReq(inMsg );
      break;
    case Power_Desc_rsp:
      AT_ProcessPowerDescRsp(inMsg );
      break;
    default:
      break;
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


void AT_ProcessNoderDescReq(zdoIncomingMsg_t *inMsg ){
  
}