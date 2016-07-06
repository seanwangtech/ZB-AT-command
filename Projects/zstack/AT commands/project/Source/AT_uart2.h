#ifndef AT_UART2_H
#define AT_UART2_H

/****************************
* configuration for second serial port
******************************/
#define AT_UART1_BR HAL_UART_BR_9600
//#define AT_UART1_BR HAL_UART_BR_19200
//#define AT_UART1_BR HAL_UART_BR_38400
//#define AT_UART1_BR HAL_UART_BR_57600
//#define AT_UART1_BR HAL_UART_BR_115200



/************Global function****************/
uint8 AT_UartInit2 ( uint8 taskID );
void AT_UartProcess2( uint8 port, uint8 event );



#endif