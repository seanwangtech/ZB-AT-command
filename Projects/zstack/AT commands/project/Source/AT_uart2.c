/**************************************************************************************************
  Filename:       AT_uart.c

  Description:    For second uart port
  Author:         Xiao Wang
**************************************************************************************************/

#include "At_include.h"
#include "AT_uart2.h"
#include "AT_IR.h"

uint8 AT_UartInit2( uint8 taskID )
{
  (void) taskID;
  halUARTCfg_t uartConfig;


  /* UART Configuration */
  uartConfig.configured           = TRUE;
  uartConfig.baudRate             = AT_UART1_BR;
  uartConfig.flowControl          = FALSE;
  uartConfig.flowControlThreshold = MT_UART_THRESHOLD;
  uartConfig.rx.maxBufSize        = 64;               //actually it is useless for cc2530
  uartConfig.tx.maxBufSize        = 64;               //actually it is useless for cc2530
  uartConfig.idleTimeout          = MT_UART_IDLE_TIMEOUT;
  uartConfig.intEnable            = TRUE;
  uartConfig.callBackFunc         = NULL;//AT_UartProcess2;
  return HalUARTOpen ( HAL_UART_PORT_1, &uartConfig);
}

//void AT_UartProcess2( uint8 port, uint8 event ){
 //uint8 code[3];
  //(void) event;
  //uint8 status;
//  AT_IR_t buff; 
//  uint16 nwk=0x0000;
//  if(Hal_UART_RxBufLen( port )){
//   uint8 size=HalUARTRead(port,(uint8*)&code,5); 
//   if(size==3){
//   buff.cmd=AT_IR_Cmd_req;
//   buff.cmdIR=UPLOAD_IR_CMD;
//   //串口读取到红外模块学习的数据
//   buff.code.IRversion=0x01;
//   buff.code.IRlength=0x24;
//   buff.code.IRtype=0x01;
//   buff.code.IRaddress[0]=code[0];
//   buff.code.IRaddress[1]=code[1];
//   buff.code.IRvalue=code[2];
//   AT_IR_Cmd_send_simple(nwk,AT_IR_CLUSTERID,sizeof(AT_IR_t),&buff);
//   }
// }
//}