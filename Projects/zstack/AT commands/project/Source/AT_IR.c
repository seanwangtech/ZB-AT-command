#include "AT_IR.h"
#include "AF.h"
#include "hal_uart.h"
#include "AT_include.h"
//void AT_IR_MessageMSGCB( afIncomingMSGPacket_t *pkt );
void AT_IR_CB(afIncomingMSGPacket_t *pkt);
void AT_IR_req(afIncomingMSGPacket_t *pkt);
void AT_IR_rsp(afIncomingMSGPacket_t *pkt );

const cId_t AT_IR_ClusterList[AT_IR_MAX_CLUSTERS] ={
 AT_IR_CLUSTERID
};
const SimpleDescriptionFormat_t AT_IR_SimpleDesc =
{
  AT_IR_ENDPOINT,              //  int Endpoint;
  AT_IR_PROFID,                //  uint16 AppProfId[2];
  AT_IR_DEVICEID,              //  uint16 AppDeviceId[2];
  AT_IR_DEVICE_VERSION,        //  int   AppDevVer:4;
  AT_IR_FLAGS,                 //  int   AppFlags:4;
  AT_IR_MAX_CLUSTERS,                                     //  uint8  AppNumInClusters;
  (cId_t *)AT_IR_ClusterList,                            //  uint8 *pAppInClusterList;
  AT_IR_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)AT_IR_ClusterList   //  uint8 *pAppInClusterList;
};

endPointDesc_t AT_IR_epDesc;
uint8 AT_IR_TransID;  // This is the unique message ID (counter)
void AT_IR_Register(uint8 *task_id){
 // Fill out the endpoint description.
  AT_IR_epDesc.endPoint = AT_IR_ENDPOINT;
  AT_IR_epDesc.task_id = task_id;
  AT_IR_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&AT_IR_SimpleDesc;
  AT_IR_epDesc.latencyReq = noLatencyReqs;
  
  // Register the endpoint description with the AF
  afRegister( &AT_IR_epDesc );
  
}
void AT_IR_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  
  switch ( pkt->clusterId )
  {
    case AT_IR_CLUSTERID:
      AT_IR_CB(pkt);
      break;
     
  }
}

afStatus_t AT_IR_Cmd_send_simple_(uint16 nwkAddr,uint16 CID,uint8 len, uint8 *buff){
  
  afAddrType_t dstAddr;
  dstAddr.endPoint = AT_IR_ENDPOINT;
  dstAddr.addr.shortAddr =nwkAddr;
  dstAddr.addrMode = (afAddrMode_t) Addr16Bit;
  
  return AF_DataRequest( &dstAddr, & AT_IR_epDesc,
                       CID,
                       len,
                       buff,
                       &AT_IR_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS );
}
void AT_IR_CB(afIncomingMSGPacket_t *pkt){
AT_IR_t *hdr = (AT_IR_t*)pkt->cmd.Data;
  switch (hdr->cmd){
  case AT_IR_Cmd_req:
    AT_IR_req(pkt);
    break;
  case AT_IR_Cmd_rsp:
    AT_IR_rsp(pkt);
    break;
  } 
}
void AT_IR_req(afIncomingMSGPacket_t *pkt  ){
  AT_IR_t *hdr = (AT_IR_t*)pkt->cmd.Data;
  //数据发送格式：head,type,length,data,tail
  //数据发送例子：EA,01,06,000000012345,EB
  uint8 code[3];
  uint8 nwk=0x0000;
     //收到来自协调器的红外数据,在UART1打印
     hdr->cmd=AT_IR_Cmd_rsp;
     //uint8 status;
     code[0]=hdr->code.IRhead;
     code[1]=hdr->cmdIR;
     code[2]=hdr->code.IRlength;
     HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&code),3);
     uint16 num;
    // HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&(hdr->code.IRdata)),hdr->code.IRlength);
     num=HalUARTWrite(HAL_UART_PORT_1,(uint8*)(hdr->code.IRdata),hdr->code.IRlength);
     HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&(hdr->code.IRtail)),1);
     //将发送成功与否的状态返回给协调器
     if(num){
     hdr->status=(uint8)IR_SEND_SUCCESS;
     AT_IR_Cmd_send_simple(nwk,AT_IR_CLUSTERID,sizeof(AT_IR_t), hdr);
     }
     else{
     hdr->status=(uint8)IR_SEND_FAILURE;
     AT_IR_Cmd_send_simple(nwk,AT_IR_CLUSTERID,sizeof(AT_IR_t), hdr);
     }
} 

void AT_IR_rsp(afIncomingMSGPacket_t *pkt ){
  AT_IR_t *hdr = (AT_IR_t*)pkt->cmd.Data;
  //协调器串口0打印收到的状态信息
  AT_RESP_START();
  printf("IR:%04X,%02X,%02X,%02X",
            pkt->srcAddr.addr.shortAddr,
            hdr->IR_EP,
            hdr->cmdIR,
            hdr->status);
  AT_RESP_END();
}
