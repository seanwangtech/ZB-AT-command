/**************************************************************************************************
  Filename:       AT_ZDO.c

  Description:    AT command module
  Author:         Xiao Wang
**************************************************************************************************/


#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"
#include "ZDObject.h"

#include "AT_ZDO.h"
#include "AT_App.h"
#include "AT_printf.h"

#include "AT_uart.h"
#include "hal_led.h"

/*******************************local functions************************/

void AT_ZDO_ProcessNoderDescReq(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessPowerDescReq( zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessPowerDescRsp( zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessLqiRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessRtgRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessNwkDiscRsp(zdoIncomingMsg_t *inMsg );




void AT_ZDO_Register(uint8 *pTask_id){
  uint8 task_id = *pTask_id;
  ZDO_RegisterForZDOMsg( task_id, IEEE_addr_rsp );
  ZDO_RegisterForZDOMsg( task_id, NWK_addr_rsp );
  //ZDO_RegisterForZDOMsg( task_id,  Node_Desc_rsp ); 
  ZDO_RegisterForZDOMsg( task_id,  Mgmt_Lqi_rsp ); 
  ZDO_RegisterForZDOMsg( task_id,  Mgmt_Rtg_rsp ); 
  ZDO_RegisterForZDOMsg( task_id,  Mgmt_NWK_Disc_rsp ); 
}


void AT_ZDO_ProcessMsgCBs( zdoIncomingMsg_t *inMsg ){
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
          }
          osal_mem_free( pAddrRsp );
        }
      }
      break;
    case Node_Desc_rsp:
      AT_ZDO_ProcessNoderDescReq(inMsg );
      break;
    case Power_Desc_req:
      AT_ZDO_ProcessPowerDescReq(inMsg );
      break;
    case Power_Desc_rsp:
      AT_ZDO_ProcessPowerDescRsp(inMsg );
      break;
    case Mgmt_Lqi_rsp:
      AT_ZDO_ProcessLqiRsp(inMsg );
      break;
    case Mgmt_Rtg_rsp:
      AT_ZDO_ProcessRtgRsp(inMsg );
      break;
    case Mgmt_NWK_Disc_rsp:
      AT_ZDO_ProcessNwkDiscRsp(inMsg );
      break;
    default:
      break;
  }
}

void AT_ZDO_ProcessNoderDescReq(zdoIncomingMsg_t *inMsg ){
  
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


void AT_ZDO_ProcessPowerDescReq( zdoIncomingMsg_t *inMsg )
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
void AT_ZDO_ProcessPowerDescRsp( zdoIncomingMsg_t *inMsg )
{
  ZDO_PowerRsp_t NPRsp;
  ZDO_ParsePowerDescRsp(inMsg,&NPRsp);
  uint8 powerlevel = (uint8) NPRsp.pwrDesc.CurrentPowerSourceLevel;
  AT_ERROR(powerlevel);
 
}

/**********************************************************

 Display Neighbour Table for AT+NTABLE

for example:
NTable:<NodeID>,<errorcode> 
Length:03 
No.| Type | EUI | ID | LQI 
0. | FFD | 000D6F000015896B | BC04 | FF 
1. | FFD | 000D6F00000B3E77 | 739D | FF 
2. | FFD | 000D6F00000AAD11 | 75E3 | FF

********************************************************/
void AT_ZDO_ProcessLqiRsp(zdoIncomingMsg_t *inMsg ){
  
  AT_ZDO_MgmtLqiRsp_t *pRsp = (AT_ZDO_MgmtLqiRsp_t *)inMsg->asdu;
  //pRsp = ZDO_ParseMgmtLqiRsp( inMsg );
  
  AT_RESP_START();
  //NTable:<NodeID>,<errorcode> 
  if(inMsg->srcAddr.addrMode==(afAddrMode_t)Addr16Bit){
    printf("NTABLE:%04X,%02X\n\r",(uint16)inMsg->srcAddr.addr.shortAddr,
           pRsp->status);
  }
  //Length:XX
  printf("Length:%02X\n\r",pRsp->neighborLqiCount);   
  //No.| Type | Relation | EUI | ID | LQI
 if(pRsp->neighborLqiCount!=0) printf("No.|  Type  |Relation|       EUI        |  ID  | LQI\n\r");
  
  uint8 i;
  for(i=0;i<pRsp->neighborLqiCount;i++){
    uint16 *ext = (uint16*) pRsp->list[i].extAddr;
    //print No.
    printf("\n\r%X. | ",i+pRsp->startIndex);
    //print type
    if(pRsp->list[i].devType==ZDP_MGMT_DT_COORD) printf("%-7s| ","COORD");
    else if(pRsp->list[i].devType==ZDP_MGMT_DT_ROUTER)printf("%-7s| ","ROUTER");
    else if(pRsp->list[i].devType==ZDP_MGMT_DT_ENDDEV)printf("%-7s| ","ENDDEV");
    else printf("%-s| ","UNKNOWN");
    //print relation
    if(pRsp->list[i].relation == ZDP_MGMT_REL_PARENT) printf("%-7s| ","PARENT");
    else if(pRsp->list[i].relation == ZDP_MGMT_REL_CHILD) printf("%-7s| ","CHILD");
    else if(pRsp->list[i].relation == ZDP_MGMT_REL_SIBLING) printf("%-7s| ","SIBLING");
    else printf("%-7s| ","UNKNOWN");
    //print EUI ID LQI    
    printf("%04X%04X%04X%04X | %04X | %02X",
           ext[3],ext[2],ext[1],ext[0],pRsp->list[i].nwkAddr,
           pRsp->list[i].lqi);
    //print permit join
    
  }
  AT_RESP_END();    
  
  /* in ZDP_MgmtLqiRsp() function in ZDProfile.c
  therefor, we know that:
  devType, rxOnIdle, relation, are all packed into 1 byte
  reason 1:
  
    // DEVICETYPE
    *pBuf = list->devType;

    // RXONIDLE
    *pBuf |= (uint8)(list->rxOnIdle << 2);

    // RELATIONSHIP
    *pBuf++ |= (uint8)(list->relation << 4);
  
  reason 2:
  // This structure is used to build the Mgmt_Lqi_rsp
typedef struct
  {
    uint16 panID;                  // PAN Id
    uint8  extPanID[Z_EXTADDR_LEN];// Extended Pan ID
    uint8  extAddr[Z_EXTADDR_LEN]; // Extended address
    uint16 nwkAddr;                // Network address
    uint8  devType;                // Device type
    uint8  rxOnIdle;               // RxOnWhenIdle
    uint8  relation;               // Relationship
    uint8  permit;                 // Permit joining
    uint8  depth;                  // Depth
    uint8  lqi;                    // LQI
  } ZDP_MgmtLqiItem_t;
  // devType, rxOnIdle, relation, are all packed into 1 byte: 18-2=16.
  */
}

/**********************************************************

 Display Neighbour Table for AT+RABLE
    Prompt (example) 
        RTable:<NodeID>,<errorcode> 
        Length:03 
        No.| Dest | Next | Status 
        0. | 1234 | ABCD | 00 
        1. | 4321 | 739D | 00 
        2. | 0000 | 0000 | 03

********************************************************/
void AT_ZDO_ProcessRtgRsp(zdoIncomingMsg_t *inMsg ){
  ZDO_MgmtRtgRsp_t* pRsp = (ZDO_MgmtRtgRsp_t*)ZDO_ParseMgmtRtgRsp(inMsg );
  
  AT_RESP_START();
  
  //NTable:<NodeID>,<errorcode> 
  if(inMsg->srcAddr.addrMode==(afAddrMode_t)Addr16Bit){
    printf("NTABLE:%04X,%02X\n\r",(uint16)inMsg->srcAddr.addr.shortAddr,
           pRsp->status);
  }
  //Length:XX
  printf("Length:%02X\n\r",pRsp->rtgCount); 
  
  //No.| Dest | Next | Status 
 if(pRsp->rtgCount!=0) printf("No.| Dest | Next | Status\n\r");
  
  uint8 i;
  for(i=0;i<pRsp->rtgListCount;i++){
    //print No.| Dest | Next | Status
    printf("\n\r%X. | %04X | %04X | %02X",
           i+pRsp->startIndex,
           pRsp->list[i].dstAddress,
           pRsp->list[i].nextHopAddress,
           pRsp->list[i].status);
  }
  AT_RESP_END();
  osal_mem_free( pRsp );
}


void AT_ZDO_ProcessNwkDiscRsp(zdoIncomingMsg_t *inMsg ){
  ZDO_MgmNwkDiscRsp_t * pRsp=ZDO_ParseMgmNwkDiscRsp( inMsg );
  AT_RESP_START();
/*
  uint8  status;
  uint8  networkCount;
  uint8  startIndex;
  uint8  networkListCount;
  
  
  uint8 extendedPANID[Z_EXTADDR_LEN];   // The extended PAN ID
  uint16 PANId;            // The network PAN ID
  uint8   logicalChannel;  // Network's channel
  uint8   stackProfile;    // Network's profile
  uint8   version;         // Network's Zigbee version
  uint8   beaconOrder;     // Beacon Order
  uint8   superFrameOrder; 
  uint8   permitJoining;   // PermitJoining. 1 or 0
  */
  uint8 i;
  for(i=0;i<pRsp->networkListCount;i++){
    printf("%d:%d",pRsp->list[i].logicalChannel,pRsp->list[i].permitJoining);
  }
  
  AT_RESP_END();
  
  osal_mem_free( pRsp );
}