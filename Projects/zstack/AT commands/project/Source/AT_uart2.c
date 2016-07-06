/**************************************************************************************************
  Filename:       AT_uart.c

  Description:    For second uart port
  Author:         Xiao Wang
**************************************************************************************************/

#include "At_include.h"
#include "AT_uart2.h"


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
  uartConfig.callBackFunc         = AT_UartProcess2;
  return HalUARTOpen ( HAL_UART_PORT_1, &uartConfig);
}

void AT_UartProcess2( uint8 port, uint8 event ){
  uint8 ch;
  (void) event;
  while(Hal_UART_RxBufLen( port )){
    HalUARTRead(port,&ch,1); 
    HalUARTWrite(HAL_UART_PORT_1, "\n\rI received a letter:'",sizeof("\n\rI received a letter:'"));
    HalUARTWrite(HAL_UART_PORT_1, &ch, 1);
    HalUARTWrite(HAL_UART_PORT_1, "'\n\r", 3);
  }
}