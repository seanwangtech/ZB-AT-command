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

#include "AT_AF.h"
#include "AT_App.h"

#include "AT_uart.h"
#include "hal_led.h"




const cId_t AT_AF_ClusterList[AT_AF_MAX_CLUSTERS] =
{
  AT_AF_Cmd_REPPRINT_CLUSTERID,
  AT_AF_Cmd_REPENABLE_CLUSTERID
};

const SimpleDescriptionFormat_t AT_AF_SimpleDesc =
{
  AT_AF_ENDPOINT,              //  int Endpoint;
  AT_AF_PROFID,                //  uint16 AppProfId[2];
  AT_AF_DEVICEID,              //  uint16 AppDeviceId[2];
  AT_AF_DEVICE_VERSION,        //  int   AppDevVer:4;
  AT_AF_FLAGS,                 //  int   AppFlags:4;
  AT_AF_MAX_CLUSTERS,                                     //  uint8  AppNumInClusters;
  (cId_t *)AT_AF_ClusterList,                            //  uint8 *pAppInClusterList;
  AT_AF_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)AT_AF_ClusterList   //  uint8 *pAppInClusterList;
};

endPointDesc_t AT_AF_epDesc;
uint8 AT_AF_TransID;  // This is the unique message ID (counter)

/****************Local function*********************/
void AT_AF_Cmd_REPPRINT_CB(afIncomingMSGPacket_t *pkt );
void AT_AF_Cmd_REPPRINT_req(afIncomingMSGPacket_t *pkt  );
void AT_AF_Cmd_REPPRINT_rsp(afIncomingMSGPacket_t *pkt );
void AT_AF_Cmd_REPENABLE_CB(afIncomingMSGPacket_t *pkt );
void AT_AF_Cmd_REPENABLE_req(afIncomingMSGPacket_t *pkt  );
void AT_AF_Cmd_REPENABLE_rsp(afIncomingMSGPacket_t *pkt );


void AT_AF_Register(uint8 *task_id){
 // Fill out the endpoint description.
  AT_AF_epDesc.endPoint = AT_AF_ENDPOINT;
  AT_AF_epDesc.task_id = task_id;
  AT_AF_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&AT_AF_SimpleDesc;
  AT_AF_epDesc.latencyReq = noLatencyReqs;
  
  // Register the endpoint description with the AF
  afRegister( &AT_AF_epDesc );
}


void AT_AF_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  
  switch ( pkt->clusterId )
  {
    case AT_AF_Cmd_REPPRINT_CLUSTERID:
      AT_AF_Cmd_REPPRINT_CB(pkt);
      break;
    case AT_AF_Cmd_REPENABLE_CLUSTERID:
      AT_AF_Cmd_REPENABLE_CB(pkt);
      break;
  }
}


afStatus_t AT_AF_Cmd_send_simple_(uint16 nwkAddr,uint16 CID,uint8 len, uint8 *buff){
  
  afAddrType_t dstAddr;
  dstAddr.endPoint = AT_AF_ENDPOINT;
  dstAddr.addr.shortAddr =nwkAddr;
  dstAddr.addrMode = (afAddrMode_t) Addr16Bit;
  
  return AF_DataRequest( &dstAddr, & AT_AF_epDesc,
                       CID,
                       len,
                       buff,
                       &AT_AF_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS );
}



void AT_AF_Cmd_REPPRINT_req(afIncomingMSGPacket_t *pkt ){
  const uint8 AT_CMD_EP_ARRAY[] = AT_CMD_EPs;
  uint8 epNum = afNumEndPoints()-1;
  byte* epBuf = (byte*)  osal_mem_alloc(epNum);
  if(epBuf==NULL) return;
  afEndPoints( epBuf, true);
  AT_sort_arr(epBuf,epNum);
  
  //the epStatus store the search result
  uint8 pbuf_temp[sizeof(AT_CMD_EP_ARRAY)+sizeof(AT_AF_hdr)];
  AT_AF_Cmd_REPPRINT_rsp_t* pbuf = (AT_AF_Cmd_REPPRINT_rsp_t*)pbuf_temp;
  pbuf->hdr.cmd=AT_AF_Cmd_rsp;
  
  int i,j;
  for(i=0,j=0;j<sizeof(AT_CMD_EP_ARRAY);j++){
    if(epBuf[i]==AT_CMD_EP_ARRAY[j]){
      pbuf->status[j] = AT_AF_enable;
      i++;
    }
    else pbuf->status[j] = AT_AF_disable;
  }
  osal_mem_free(epBuf);
  AF_DataRequest( & (pkt->srcAddr), & AT_AF_epDesc,
                       AT_AF_Cmd_REPPRINT_CLUSTERID,
                       sizeof(AT_CMD_EP_ARRAY)+sizeof(AT_AF_hdr),
                       (uint8*)pbuf,
                       &AT_AF_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS );
  
}
void AT_AF_Cmd_REPPRINT_rsp(afIncomingMSGPacket_t *pkt ){
  uint8* epStatus = ((AT_AF_Cmd_REPPRINT_rsp_t*) pkt->cmd.Data)->status;
  const uint8 AT_CMD_EP_ARRAY[] = AT_CMD_EPs;
  
  uint8 i;
  char str[4];
  AT_RESP_START();
  AT_RESP("<nodeID>,<EndPoint>:<Status>",
          sizeof("<nodeID>,<EndPoint>:<Status>")-1);
  for(i=0;i<sizeof(AT_CMD_EP_ARRAY);i++){
    AT_NEXT_LINE();
    
    //display nodeID
    if(pkt->srcAddr.addrMode==(afAddrMode_t)Addr16Bit){
      AT_Int16toChar(pkt->srcAddr.addr.shortAddr,str);
      AT_RESP(str,4);
    }else AT_RESP("UNKNOWN",sizeof("UNKNOWN")-1);
    AT_RESP(",",1);
    
    //print end point
    AT_Int8toChar(AT_CMD_EP_ARRAY[i],str);
    AT_RESP(str,2);
    
    //display status 
    if(epStatus[i]==AT_AF_enable){
      AT_RESP(":ENABLED",sizeof(":ENABLED")-1);
    }
    else AT_RESP(":DISABLED",sizeof(":DISABLED")-1);
  }
  AT_RESP_END();
}


void AT_AF_Cmd_REPPRINT_CB(afIncomingMSGPacket_t *pkt ){
  
  AT_AF_hdr *hdr = (AT_AF_hdr*)pkt->cmd.Data;
  switch (hdr->cmd){
  case AT_AF_Cmd_req:
    AT_AF_Cmd_REPPRINT_req(pkt);
    break;
  case AT_AF_Cmd_rsp:
    AT_AF_Cmd_REPPRINT_rsp(pkt);
    break;
  }   
}


/*************************************************************
    deal with the requst from other devices for REPENABLE, 
    and send a response
*************************************************************/
void AT_AF_Cmd_REPENABLE_req(afIncomingMSGPacket_t *pkt  ){
  AT_AF_Cmd_REPENABLE_rsp_t rsp;
  
  AT_AF_Cmd_REPENABLE_req_t *req = (AT_AF_Cmd_REPENABLE_req_t*)pkt->cmd.Data;
  uint8 enable =req->enable;
  uint8 ep     =req->ep;
  
  rsp.hdr.cmd=AT_AF_Cmd_rsp;
  rsp.item.ep=req->ep;
  
  if(enable){
    if( AT_af_exist_ep(ep)){
      if(AT_af_register_ep(ep)==afStatus_SUCCESS){ 
        //enable the end point in ZCL layer
        AT_ZCL_EP_ENABLE( enable,ep);
        AT_NV_ZCL_saveEPStatus(AT_NV_ZCL_get_index_(ep),&enable);
      }
      rsp.item.status=AT_AF_enable;
    }else {
      rsp.item.status = AT_AF_UNKNOWNEP;
    }
  }
  else{
    if(AT_af_exist_ep(ep)){
      if(AT_af_remove_ep(ep)==afStatus_SUCCESS){
        //disable the end point in ZCL layer
        AT_ZCL_EP_ENABLE( enable,ep);
        //save to NV
        AT_NV_ZCL_saveEPStatus(AT_NV_ZCL_get_index_(ep),&enable);
      }
      rsp.item.status = AT_AF_disable;
    }else {
      rsp.item.status = AT_AF_UNKNOWNEP;
    }
  }
  
  AF_DataRequest( & (pkt->srcAddr), & AT_AF_epDesc,
                       AT_AF_Cmd_REPENABLE_CLUSTERID,
                       sizeof(AT_AF_Cmd_REPENABLE_rsp_t),
                       (uint8*)&rsp,
                       &AT_AF_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS );
  
}

/*************************************************************
deal with the response from other devices
*************************************************************/
void AT_AF_Cmd_REPENABLE_rsp(afIncomingMSGPacket_t *pkt ){
  AT_AF_Cmd_REPENABLE_rsp_t *rsp = (AT_AF_Cmd_REPENABLE_rsp_t*)pkt->cmd.Data;
  uint8 status =rsp->item.status;
  uint8 ep     =rsp->item.ep;
  char str[4];
  AT_RESP_START();
       
  switch (status){
  case AT_AF_enable:
    AT_RESP("ENABLED:",sizeof("ENABLED:")-1);
    //print nodeID
    if(pkt->srcAddr.addrMode==(afAddrMode_t)Addr16Bit){
      AT_Int16toChar(pkt->srcAddr.addr.shortAddr,str);
      AT_RESP(str,4);
    }else AT_RESP("UNKNOWN",sizeof("UNKNOWN")-1);
    AT_RESP(",",1); 
    //print end point
    AT_Int8toChar(ep,str);
    AT_RESP(str,2);
    break;
  case AT_AF_disable:
    AT_RESP("DISABLED:",sizeof("DISABLED:")-1);
    //print nodeID
    if(pkt->srcAddr.addrMode==(afAddrMode_t)Addr16Bit){
      AT_Int16toChar(pkt->srcAddr.addr.shortAddr,str);
      AT_RESP(str,4);
    }else AT_RESP("UNKNOWN",sizeof("UNKNOWN")-1);
    AT_RESP(",",1); 
    //print end point
    AT_Int8toChar(ep,str);
    AT_RESP(str,2);
    break;
  case AT_AF_UNKNOWNEP:
    if(pkt->srcAddr.addrMode==(afAddrMode_t)Addr16Bit){
      AT_Int16toChar(pkt->srcAddr.addr.shortAddr,str);
      AT_RESP(str,4);
    }else AT_RESP("UNKNOWN",sizeof("UNKNOWN")-1);
    AT_RESP(",",1); 
    //print end point
    AT_RESP("UNKNOWNEP",sizeof("UNKNOWNEP")-1);
    break;
  }
  AT_RESP_END();
}
/**************************************************************
deal with AT_AF_Cmd_REPENABLE_CLUSTERID messages.
*************************************************************/
void AT_AF_Cmd_REPENABLE_CB(afIncomingMSGPacket_t *pkt ){
  AT_AF_Cmd_REPENABLE_req_t *req = (AT_AF_Cmd_REPENABLE_req_t *)pkt->cmd.Data;
  if(req->hdr.cmd==AT_AF_Cmd_req){
    AT_AF_Cmd_REPENABLE_req(pkt);
  }else{
    AT_AF_Cmd_REPENABLE_rsp(pkt);
  }
}