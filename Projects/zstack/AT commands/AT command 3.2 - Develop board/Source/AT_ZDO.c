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


#include "At_include.h"
#include "hal_led.h"

/*******************************local functions************************/

void AT_ZDO_ProcessLqiRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessRtgRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessNwkDiscRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessMgmtBindRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessBindRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessUnbindRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessEDbindRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessMgmtLeaveRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessNodeDescRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessPowerDescRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessSimpleDescRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessActEPRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessMatchDescRsp(zdoIncomingMsg_t *inMsg );
void AT_ZDO_ProcessAnnceRsp(zdoIncomingMsg_t *inMsg );
void* AT_ZDO_ProcessNWKDISC_CB(void *param);

void AT_ZDO_Register(uint8 *pTask_id){
  uint8 task_id = *pTask_id;
  ZDO_RegisterForZDOMsg( task_id, IEEE_addr_rsp );
  ZDO_RegisterForZDOMsg( task_id, NWK_addr_rsp );
  //ZDO_RegisterForZDOMsg( task_id,  Node_Desc_rsp ); 
  ZDO_RegisterForZDOMsg( task_id,  Mgmt_Lqi_rsp ); 
  ZDO_RegisterForZDOMsg( task_id,  Mgmt_Rtg_rsp ); 
  ZDO_RegisterForZDOMsg( task_id,  Mgmt_NWK_Disc_rsp );
  ZDO_RegisterForZDOMsg( task_id,  Mgmt_Bind_rsp ); 
  ZDO_RegisterForZDOMsg( task_id,  Bind_rsp );
  ZDO_RegisterForZDOMsg( task_id,  Unbind_rsp );
  ZDO_RegisterForZDOMsg( task_id,  End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( task_id,  Mgmt_Leave_rsp );
  ZDO_RegisterForZDOMsg( task_id,  Node_Desc_rsp );
  ZDO_RegisterForZDOMsg( task_id,  Power_Desc_rsp );
  ZDO_RegisterForZDOMsg( task_id,  Simple_Desc_rsp );
  ZDO_RegisterForZDOMsg( task_id,  Active_EP_rsp );
  ZDO_RegisterForZDOMsg( task_id,  Match_Desc_rsp );
  ZDO_RegisterForZDOMsg( task_id,  Device_annce );
  ZDO_RegisterForZdoCB(ZDO_LEAVE_CNF_CBID, AT_ZDO_ProcessLEAVE_CNF_CB);
  
  //if req.silent = TRUE,get ZDO_LeaveCnf message
  //if req.silent = FALSE,get ZDO_LeaveInd
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
    case Mgmt_Lqi_rsp:
      AT_ZDO_ProcessLqiRsp(inMsg );
      break;
    case Mgmt_Rtg_rsp:
      AT_ZDO_ProcessRtgRsp(inMsg );
      break;
    case Mgmt_NWK_Disc_rsp:
      AT_ZDO_ProcessNwkDiscRsp(inMsg );
      break;
    case Mgmt_Bind_rsp:
      AT_ZDO_ProcessMgmtBindRsp(inMsg );
      break;
    case Bind_rsp:
      AT_ZDO_ProcessBindRsp(inMsg );
      break;
    case Unbind_rsp:
      AT_ZDO_ProcessUnbindRsp(inMsg );
      break;
    case End_Device_Bind_rsp:
      AT_ZDO_ProcessEDbindRsp(inMsg );
      break;
    case Mgmt_Leave_rsp:
      AT_ZDO_ProcessMgmtLeaveRsp(inMsg );
      break;
    case Node_Desc_rsp:
      AT_ZDO_ProcessNodeDescRsp(inMsg );
      break;
    case Power_Desc_rsp:
      AT_ZDO_ProcessPowerDescRsp(inMsg );
      break;
    case Simple_Desc_rsp:
      AT_ZDO_ProcessSimpleDescRsp(inMsg );
      break;
    case Active_EP_rsp:
      AT_ZDO_ProcessActEPRsp(inMsg );
      break;
    case Match_Desc_rsp:
      AT_ZDO_ProcessMatchDescRsp(inMsg );
      break;
    case Device_annce:
      AT_ZDO_ProcessAnnceRsp(inMsg );
      break;
    default:
      break;
  }
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

/**********************************************************
+BTABLE 每 Display Binding Table (ZDO)
      BTable:0000,00 
      Length:03 
      No. | SrcAddr | SrcEP | ClusterID | DstAddr | DstEP 
      00. | 000D6F000059474E | 01 | DEAD |1234567887654321 | 12 
      01. | 000D6F000059474E | 01 | DEAD |E012345678876543 | E0 
      02. | 000D6F000059474E | 01 | DEAD | ABCD 
      ACK:01
****************************************************************/

void AT_ZDO_ProcessMgmtBindRsp(zdoIncomingMsg_t *inMsg ){
  ZDO_MgmtBindRsp_t * pRsp = ZDO_ParseMgmtBindRsp(inMsg );

  uint8 i;
  char dstAddr[17];
  char dstEP[5];
  zAddrType_t* addr;
  AT_RESP_START();
  printf("Length:%02X",pRsp->bindingListCount);
  AT_NEXT_LINE();
  if(pRsp->bindingListCount)
    printf("No. |     SrcAddr      | SrcEP | ClusterID |     DstAddr      | DstEP");
  for(i=0;i<pRsp->bindingListCount;i++){
    addr = &pRsp->list[i].dstAddr;
    AT_NEXT_LINE();
    if(addr->addrMode== (afAddrMode_t)Addr64Bit){
      AT_EUI64toChar(addr->addr.extAddr,dstAddr);
      sprintf(dstEP,"| %02X",pRsp->list[i].dstEP);
      dstAddr[16]='\0';
    }else{
      sprintf(dstAddr,"%04X",addr->addr.shortAddr);
      dstEP[0]='\0';
    }
    uint16 *srcAddr= (uint16*) pRsp->list[i].srcAddr;    
    printf("%02X. | %04X%04X%04X%04X |  %02X   |   %04X    | %s %s ",
           i+pRsp->startIndex,srcAddr[3],srcAddr[2],srcAddr[1],srcAddr[0],
           pRsp->list[i].srcEP, pRsp->list[i].clusterID, dstAddr,dstEP);
    
  }
  AT_RESP_END();
}

/**********************************************************
+BIND 每 Create Binding on Remote Device (ZDO) 
Prompt 
          Bind:<NodeID>,<status>

          In case of an error an status other than 00 will be displayed 
          <NodeID> is the Remote node＊s Node ID.

****************************************************************/
void AT_ZDO_ProcessBindRsp(zdoIncomingMsg_t *inMsg ){
  uint8 status = ZDO_ParseBindRsp(inMsg);
  AT_RESP_START();
  if(inMsg->srcAddr.addrMode== (afAddrMode_t)Addr16Bit){
    printf("Bind:%04X,%02X",inMsg->srcAddr.addr.shortAddr, status);
  }else{
    printf("Bind:UNKNOWN,%02X", status);
  }
  AT_RESP_END();
    
}
/**********************************************************
+UNBIND 每 Create Binding on Remote Device (ZDO) 
Prompt 
          Unbind:<NodeID>,<status>

          In case of an error an status other than 00 will be displayed 
          <NodeID> is the Remote node＊s Node ID.

****************************************************************/
void AT_ZDO_ProcessUnbindRsp(zdoIncomingMsg_t *inMsg ){
  uint8 status = ZDO_ParseBindRsp(inMsg);
  AT_RESP_START();
  if(inMsg->srcAddr.addrMode== (afAddrMode_t)Addr16Bit){
    printf("Unbind:%04X,%02X",inMsg->srcAddr.addr.shortAddr, status);
  }else{
    printf("Unbind:UNKNOWN,%02X", status);
  }
  AT_RESP_END();
    
}

/**********************************************************
+EBIND 每 End Device Bind 

Response
          EBIND:<NodeID>,<Status> 
          OK

****************************************************************/
void AT_ZDO_ProcessEDbindRsp(zdoIncomingMsg_t *inMsg ){
  uint8 status = ZDO_ParseBindRsp(inMsg);
  AT_RESP_START();
  if(inMsg->srcAddr.addrMode== (afAddrMode_t)Addr16Bit){
    printf("Ebind:%04X,%02X",inMsg->srcAddr.addr.shortAddr, status);
  }else{
    printf("Ebind:UNKNOWN,%02X", status);
  }
  AT_RESP_END();
    
}


/**********************************************************
+DASSR 每 Disassociate Remote Node from PAN

    Prompt 
          LeftPAN

****************************************************************/
void AT_ZDO_ProcessMgmtLeaveRsp(zdoIncomingMsg_t *inMsg ){
  uint8 status = ZDO_ParseMgmtLeaveRsp(inMsg);
  AT_RESP_START();
  if(status==ZSUCCESS){
    printf("LeftPAN");
  }else{
    printf("failed:%02X",status);
  }
  AT_RESP_END();
}


/**************************************************************
+PANSCAN 每 Scan For Active PANs
    +PANSCAN:<channel>,<PANID>,<EPANID>,XX,b

      <channel> represents the channel, 
      <PANID> the PAN ID, 
      <EPANID> the extended PAN ID, The node gives a list of all PANs found. 
      XX the ZigBee stack profile (00 = Custom, 01 = ZigBee, 02 = ZigBee PRO)

****************************************************************/

void* AT_ZDO_ProcessNWKDISC_CB(void *param){
  uint8 i;
  networkDesc_t  * NetworkList, *pNwkDesc;
  uint8 ResultCount=0;
  uint8 count=0; //record the lik quality >0 networks
  
  pNwkDesc = NetworkList= nwk_getNwkDescList();
  
    // Count the number of nwk descriptors in the list
    while (pNwkDesc)
    {
      ResultCount++;
      if(pNwkDesc->chosenRouterLinkQuality>0)count++;
      pNwkDesc = pNwkDesc->nextDesc;
    }
    pNwkDesc =  NetworkList;
  
    AT_RESP_START();
    printf("%d result(s)\n\r",count);
    pNwkDesc = NetworkList;
    if(count>0){
      //print:Channel | PANID | EPANID | StackProfile | LQI | perimit
      printf("channel | PANID | EPANID | StackProfile | LQI | perimit");
    }
    for ( i = 0; i < ResultCount ; i++, pNwkDesc = pNwkDesc->nextDesc ){
      if(pNwkDesc->chosenRouterLinkQuality>0){
        uint16 *ext = (uint16*)pNwkDesc->extendedPANID;
        uint8 permit;
        if ( pNwkDesc->chosenRouter != INVALID_NODE_ADDR )
        {
          permit = 1;//TRUE;                         // Permit Joining
        }
        else
        {
          permit = 0;//FALSE;
        }
        AT_NEXT_LINE();
        printf("%02X,%04X,%04X%04X%04X%04X,%02X,%02X,%X",
               pNwkDesc->logicalChannel,
               pNwkDesc->panId,
               ext[3],ext[2],ext[1],ext[0],
               pNwkDesc->stackProfile,
               pNwkDesc->chosenRouterLinkQuality,
               permit);
      } 
    }
  AT_RESP_END();
  NLME_NwkDiscTerm();
  ZDO_RegisterForZdoCB(ZDO_NWK_DISCOVERY_CNF_CBID, NULL);//cancel the registation in the AT command and implement the default ZDO network call back
  return NULL;
}

/**************************************************************
Join network response
    JPAN:<channel>,<PANID>,<EPANID>

****************************************************************/
void* AT_ZDO_ProcessJOIN_CNF_CB(void *param){
    zdoJoinCnf_t *joinCnf = (zdoJoinCnf_t*) param;
    

    AT_RESP_START();
    if(joinCnf->status==SUCCESS){
      uint16* ext = (uint16*)_NIB.extendedPANID;
      //cancel the call, because the command will finished in this function 
      ZDO_RegisterForZdoCB(ZDO_JOIN_CNF_CBID,NULL);
      printf("JPAN:%02X,%04X,%04X%04X%04X%04X",_NIB.nwkLogicalChannel,_NIB.nwkPanId,ext[3],ext[2],ext[1],ext[0]);
    }else{
      AT_ERROR(joinCnf->status);
      //I set the device auto start (HOLD_AUTO_START), so, when the device start, it controlled by ZDO
      //although, I change the devState and devStartMode (variables in ZDAPP) to control the flow process
      //the ZDO layer may still commit newwork discover when the device is end device.
      //therefor The MAC_BAD_STATE appears, due to doulbe call of the NLME_NetworkDiscoveryRequest();
      //one by me, one by ZDO.
      //I don't want controll all the thing by myself, so I using auto start mode.
      AT_DEBUG("MAC_BAD_STATE ZDO Trying...",sizeof("MAC_BAD_STATE ZDO Trying..."));
    }
    AT_RESP_END();
    return NULL;
}

/**************************************************************
+DASSL 每 Disassociate Local Device From PAN
    LeftPAN

****************************************************************/
extern void* AT_ZDO_ProcessLEAVE_CNF_CB(void *param){

    AT_RESP_START();
    printf("LeftPAN");
    AT_RESP_END();
    return NULL;
}


/**************************************************************
+NODEDESC 每 Request Node＊s Descriptor
    Prompt (example) 
          NodeDesc:<NodeID>,<errorcode> 
          Type:FFD 
          ComplexDesc:No 
          UserDesc:No 
          APSFlags:00  
          FreqBand:40 
          MacCap:8E 
          ManufCode:1010 
          MaxBufSize:52 
          MaxInSize:0080 
          SrvMask:0000 
          MaxOutSize:0080 
          DescCap:00
****************************************************************/
void AT_ZDO_ProcessNodeDescRsp(zdoIncomingMsg_t *inMsg ){
  ZDO_NodeDescRsp_t Rsp;
  ZDO_ParseNodeDescRsp(inMsg, &Rsp );
 /* // Node Logical Types
#define NODETYPE_COORDINATOR    0x00
#define NODETYPE_ROUTER         0x01
#define NODETYPE_DEVICE         0x02
  
    NodeDescriptorFormat_t nodeDesc;
  uint8 LogicalType:3;*
  uint8 ComplexDescAvail:1;  /AF_V1_SUPPORT - reserved bit. 
  uint8 UserDescAvail:1;     /AF_V1_SUPPORT - reserved bit. 
  uint8 Reserved:3;
  uint8 APSFlags:3;
  uint8 FrequencyBand:5;
  uint8 CapabilityFlags;
  uint8 ManufacturerCode[2];
  uint8 MaxBufferSize;
  uint8 MaxInTransferSize[2];
  uint16 ServerMask;
  uint8 MaxOutTransferSize[2];
  uint8 DescriptorCapability;
  */
  AT_RESP_START();
  
  printf("NodeDesc:%04X,%02X",Rsp.nwkAddr,Rsp.status);
  if(Rsp.status==SUCCESS){
    
    AT_NEXT_LINE();
    printf("Type:%s",NODETYPE_COORDINATOR==Rsp.nodeDesc.LogicalType ? "COOR" :
             NODETYPE_ROUTER==Rsp.nodeDesc.LogicalType ? "ROUTER" : "ENDDEV");
    AT_NEXT_LINE();
    printf("ComplexDesc:%s" , Rsp.nodeDesc.ComplexDescAvail ? "Yes" : "No");
    AT_NEXT_LINE();
    printf("UserDesc:%s" , Rsp.nodeDesc.UserDescAvail ? "Yes" : "No");
    AT_NEXT_LINE();
    printf("APSFlags:%02X",Rsp.nodeDesc.APSFlags);  
    AT_NEXT_LINE();
    printf("FreqBand:%02X",Rsp.nodeDesc.FrequencyBand); 
    AT_NEXT_LINE();
    printf("MacCap:%02x",Rsp.nodeDesc.CapabilityFlags); 
    AT_NEXT_LINE();
    printf("ManufCode:%04X",*(uint16*)Rsp.nodeDesc.ManufacturerCode); 
    AT_NEXT_LINE();
    printf("MaxBufSize:%02X",Rsp.nodeDesc.MaxBufferSize); 
    AT_NEXT_LINE();
    printf("MaxInSize:%04X",*(uint16*)Rsp.nodeDesc.MaxInTransferSize); 
    AT_NEXT_LINE();
    printf("SrvMask:%02X",Rsp.nodeDesc.ServerMask); 
    AT_NEXT_LINE();
    printf("MaxOutSize:%04X",*(uint16*)Rsp.nodeDesc.MaxOutTransferSize); 
    AT_NEXT_LINE();
    printf("DescCap:%02X",Rsp.nodeDesc.DescriptorCapability);
  }
  
  AT_RESP_END();
}

/**************************************************************
+POWERDESC 每 Request Node＊s Power Descriptor

Prompt 
      PowerDesc:<NodeID>,<errorcode>[,<PowerDescriptor>] 
            In case of an error an errorcode other than 00 will be displayed  
                    and the prompt will end after the errorcode 

            <NodeID> is the Remote node＊s Node ID. In addition the 

            <PowerDescriptor> is displayed as a 16 bit hexadecimal number as 
                    described in section 2.3.2.4. of ZigBee Pro Specification.

****************************************************************/
void AT_ZDO_ProcessPowerDescRsp(zdoIncomingMsg_t *inMsg ){
  ZDO_PowerRsp_t Rsp;
  ZDO_ParsePowerDescRsp( inMsg, &Rsp );
  
  AT_RESP_START();
  
  printf("PowerDesc:%04X,%02X",Rsp.nwkAddr,Rsp.status);
  if(Rsp.status==SUCCESS){
    AT_NEXT_LINE();
    printf("PowerMode:%02X",Rsp.pwrDesc.PowerMode);
    AT_NEXT_LINE();
    printf("AvailablePowerSources:%02X",Rsp.pwrDesc.AvailablePowerSources);
    AT_NEXT_LINE();
    printf("CurrentPowerSource:%02X",Rsp.pwrDesc.CurrentPowerSource);
    AT_NEXT_LINE();
    printf("CurrentPowerSourceLevel:%02X",Rsp.pwrDesc.CurrentPowerSourceLevel);
    
  }
  AT_RESP_END();
}

/**************************************************************
Prompt 
      SimpleDesc:<NodeID>,<errorcode> 
      EP:XX 
      ProfileID:XXXX 
      DeviceID:XXXXvXX 
      InCluster:<Cluster List> 
      OutCluster:<Cluster List> 

      In case of an error, an errorcode other than 00 will be displayed and
      the prompt will end after the errorcode. <NodeID> is th

****************************************************************/
void AT_ZDO_ProcessSimpleDescRsp(zdoIncomingMsg_t *inMsg ){
   ZDO_SimpleDescRsp_t Rsp;
  ZDO_ParseSimpleDescRsp(inMsg, &Rsp );
  /*
  uint8  status;
  uint16 nwkAddr;   // Network address of interest
  SimpleDescriptionFormat_t simpleDesc;
  
  uint8          EndPoint;
  uint16         AppProfId;
  uint16         AppDeviceId;
  uint8          AppDevVer:4;
  uint8          Reserved:4;             // AF_V1_SUPPORT uses for AppFlags:4.
  uint8          AppNumInClusters;
  cId_t         *pAppInClusterList;
  uint8          AppNumOutClusters;
  cId_t         *pAppOutClusterList;
  */
  AT_RESP_START();
  
  printf("SimpleDesc:%04X,%02X",Rsp.nwkAddr,Rsp.status);
  if(Rsp.status==SUCCESS){
    AT_NEXT_LINE();
    printf("EP:%02X",Rsp.simpleDesc.EndPoint);
    AT_NEXT_LINE();
    printf("ProfileID:%04X",Rsp.simpleDesc.AppProfId);
    AT_NEXT_LINE();
    printf("DeviceID:%04Xv%02X",Rsp.simpleDesc.AppDeviceId,Rsp.simpleDesc.AppDevVer);
    AT_NEXT_LINE();
    uint8 i;
    printf("InCluster: ");
    for(i=0;i<Rsp.simpleDesc.AppNumInClusters;i++){
        if(i) printf("           %04X",Rsp.simpleDesc.pAppInClusterList[i]);
        else printf("%04X",Rsp.simpleDesc.pAppInClusterList[i]);
        AT_NEXT_LINE();
    }
    
    printf("OutCluster: ");
    for(i=0;i<Rsp.simpleDesc.AppNumOutClusters;i++){
        if(i) printf("            %04X",Rsp.simpleDesc.pAppOutClusterList[i]); 
        else printf("%04X",Rsp.simpleDesc.pAppOutClusterList[i]);
        if(i<Rsp.simpleDesc.AppNumOutClusters-1) AT_NEXT_LINE();
    }
  }
  
  AT_RESP_END();
}

/**************************************************************
Prompt 
    ActEpDesc:<NodeID>,<errorcode>[,XX,＃] 
          <NodeID> is the Remote node＊s NodeID. In addition all active endpoints 
                    are listed as 8-bit hexadecimal numbers separated by commas. 
                    In case of an error an errorcode other than 00 will be displayed
                    and the prompt will end after the errorcode

****************************************************************/
void AT_ZDO_ProcessActEPRsp(zdoIncomingMsg_t *inMsg ){
  ZDO_ActiveEndpointRsp_t * pRsp = ZDO_ParseEPListRsp(inMsg );
  
  AT_RESP_START();
  printf("ActEpDesc:%04X,%02X",pRsp->nwkAddr,pRsp->status);
  if(pRsp->status==SUCCESS){
    uint8 i;
    for(i=0;i<pRsp->cnt;i++){
      printf(",%02X",pRsp->epList[i]);
    }
  }
  AT_RESP_END();
  osal_mem_free(pRsp);
}

/**************************************************************
+MATCHREQ 每 Find Nodes which Match a Specific Descriptor
Prompt 
      MatchDesc:<NodeID>,<errorcode>,XX,＃ 

      In case of an error an errorcode other than 00 will be displayed 
      and the prompt will end after the errorcode.

****************************************************************/
void AT_ZDO_ProcessMatchDescRsp(zdoIncomingMsg_t *inMsg ){
  ZDO_ActiveEndpointRsp_t * pRsp = ZDO_ParseEPListRsp(inMsg );
  
  AT_RESP_START();
  printf("MatchDesc:%04X,%02X",pRsp->nwkAddr,pRsp->status);
  if(pRsp->status==SUCCESS){
    uint8 i;
    for(i=0;i<pRsp->cnt;i++){
      printf(",%02X",pRsp->epList[i]);
    }
  }
  AT_RESP_END();
  osal_mem_free(pRsp);
}

/**************************************************************
+ANNCE 每 Announce Local Device In The Network
    Prompt 
          FFD:<EUI64>,<NodeID> 
          The prompt above will be displayed on all nodes which can hear the announcement.
****************************************************************/
void AT_ZDO_ProcessAnnceRsp(zdoIncomingMsg_t *inMsg ){
  ZDO_DeviceAnnce_t Rsp;
  ZDO_ParseDeviceAnnce( inMsg,&Rsp );

  uint16* ext= (uint16*) Rsp.extAddr;
  AT_RESP_START();
  printf("%s:%04X%04X%04X%04X,%04X",
         (Rsp.capabilities&0x03) == CAPINFO_ALTPANCOORD ? "COORD" :
         (Rsp.capabilities&0x03) == CAPINFO_DEVICETYPE_FFD ? "FFD" : "RFD"
         ,ext[3],ext[2],ext[1],ext[0],Rsp.nwkAddr);
  AT_RESP_END();
  
    /*
  // CapabilityFlags Bitmap values
#define CAPINFO_ALTPANCOORD           0x01
#define CAPINFO_DEVICETYPE_FFD        0x02
#define CAPINFO_DEVICETYPE_RFD        0x00
#define CAPINFO_POWER_AC              0x04
#define CAPINFO_RCVR_ON_IDLE          0x08
#define CAPINFO_SECURITY_CAPABLE      0x40
#define CAPINFO_ALLOC_ADDR            0x80*/
}