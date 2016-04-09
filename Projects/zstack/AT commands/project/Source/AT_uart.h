#ifndef AT_UART_H
#define AT_UART_H


#include "Onboard.h"
#include "OSAL.h"
#include "MT.h"




/*****************************************
                 config
****************************************/
#define AT_UART_PORT        HAL_UART_PORT_0
//#define AT_UART_BR HAL_UART_BR_9600
//#define AT_UART_BR HAL_UART_BR_19200
//#define AT_UART_BR HAL_UART_BR_38400
//#define AT_UART_BR HAL_UART_BR_57600
#define AT_UART_BR HAL_UART_BR_115200

#define AT_UART_RX_BUFF_MAX     128
#define AT_UART_TX_BUFF_MAX     128

#define AT_FCS_VERIFY              FALSE          //for being convenient for Serial Portdebugging 
#define AT_CMD_PATTERN_CHECK       TRUE
#define AT_DEBUG_INFORMATION_SHOW  TRUE
#define AT_MSG_SEND_MODE           FALSE          //if sent at Msg send mode, the message will sent to registered task to deal. otherwise, the at command will run automatic without registration. (NB: if no need for change cmd msg or the node with a small memory, it is a best choice to set it to false.)
#define AT_ENABLE_PASSWORDS_MODE   FALSE         //special function for Yitian Zhang
#define AT_ENABLE_PASSWORDS        "Essex"
#define AT_UART_ECHO()             //AT_RESP( "", 1)
#define AT_re_input_key            '\r'       //if defiene this as '\0', the reinput function will be removied. otherwise, press corresponding key will reinput the last command

#define AT_CMD_HELP_DESC_OMIT        3     //omit displaying last several command which is keep for interal use.
#define AT_CMD_HELP_DESC_OFFSET     14      //define the offset characters of the help command when display.

#define AT_CMD_EPs                  {0x01,0x04,0x05,0x06,0x07}    //whitch will display when perform print the end point command, sorted end point
extern const uint8 AT_CMD_EPs_Num;

/********************************************************
                  Error message defination
*******************************************************/
#define AT_NO_ERROR            0x00
#define AT_FCS_ERROR           0x01
#define AT_OPERATOR_ERROR      0x11
#define AT_LACK_OPERATOR       0x12
#define AT_CMD_ERROR           0x21
#define AT_LACK_CMD            0x22
#define AT_WRONG_CMD           0x23
#define AT_PARA_ERROR          0x24
#define AT_LACK_PARA           0x25
#define AT_EXCESSIVE_PARA      0x26

#define AT_FATAL_ERROR         0x01
#define AT_WRONG_DEV_ERROR     0x03
#define AT_WRONG_PASSWORD      0X04
#define AT_PASSWORD_ERROR      0x05
#define AT_MEM_ERROR           0x06
#define AT_nwkState_ERROR      0x07
#define AT_isActive_ERROR      0x08

#define AT_ERROR(x)   AT_UARTWriteErrMsg(x)
#define AT_OK()   AT_RESP( "\r\nOK\r\n", sizeof("\r\nOK\r\n"))
#define AT_RESP(str,len)  AT_HalUARTWrite(  AT_UART_PORT,(uint8*) str, len)
#define AT_RESP_START()   AT_RESP("\r\n", 2)
#define AT_RESP_END()     AT_RESP("\r\n", 2)
#define AT_NEXT_LINE()    AT_RESP("\n\r", 2)

#if     AT_DEBUG_INFORMATION_SHOW
#define AT_DEBUG(str,len) AT_RESP((uint8*) str, len)
#else   
#define AT_DEBUG(str,len)
#endif


#if AT_CMD_PATTERN_CHECK
#define AT_PARSE_CMD_PATTERN_ERROR(x,y)                    \
  {uint8 err;                                               \
  err=AT_pattern_check(x,y);                               \
    if(err!=0) {AT_ERROR(err);return; } }
#else
#define AT_PARSE_CMD_PATTERN_ERROR(x,y)
#endif

#if AT_CMD_PATTERN_CHECK
#define AT_PARSE_CMD_PATTERN_ERROR_1(x,y)                    \
  {if(x[0] != y->symbol) {AT_ERROR(AT_OPERATOR_ERROR);return;  }}
#else
#define AT_PARSE_CMD_PATTERN_ERROR_1(x,y)
#endif



/*******************************************************
          State for interpreting AT Command Frame
********************************************************/
#define AT_HEAD_STATE1      0x00
#define AT_HEAD_STATE2      0x01
#define AT_DATA_STATE       0x02
#define AT_END_STATE        0x03
#define AT_FCS_STATE        0x04


/********************NV ID defination ***************/
#define AT_NV_ZCL_EP_STATUS_ID      0x0501

/********************S-Register defination ***************/
#define AT_S_NV_OFFSET              0X0401
#define AT_S_CHANNEL_MASK_ID        0x00
#define AT_S_NULL_ID                0xff
#define AT_S_TRANSMIT_POWER_ID      0x01
#define AT_S_PREFERRED_PANID_ID     0x02
#define AT_S_PREFERRED_EXT_PANID_ID 0x03
#define AT_S_LOCAL_EUI_ID           0x04
#define AT_S_LOCAL_NODEID_ID        0x05
#define AT_S_PARENT_EUI_ID          0x06
#define AT_S_PARENT_NODEID_ID       0x07
#define AT_S_NET_KEY_ID             0x08
#define AT_S_LINK_KEY_ID            0x09
#define AT_S_MAIN_FUN_ID            0x0A
#define AT_S_USER_NAME_ID           0x0B
#define AT_S_PASSWORD_ID            0x0C
#define AT_S_DEVICE_INF_ID          0x0D
#define AT_S_UART_SETUP_ID          0x12
#define AT_S_MANUFACTURE_CODE_ID    0x60
#define AT_S_PMW_PRESCALER_VAL_ID   0x62
#define AT_S_BUTTON_FUN_ID          0x63


#define AT_SMAP_PROTECTED        0x80
#define AT_SMAP_READ             0x02
#define AT_SMAP_WRITTEN          0x01


typedef struct{
  uint8 len;
  uint8 Privilege;        //write read or protected
  uint8 S_ID;
} AT_smap_unit_t;


/******************************************************
              message event defination
***************************************************/
#define AT_CMD_MSG          CMD_SERIAL_MSG

typedef void (*AT_CmdFn_t)(uint8 start_point, uint8* msg);
typedef struct{
  char *AT_Cmd_str;
  AT_CmdFn_t  AT_CmdFn;
  char *description_str;
} AT_Cmd_t;



typedef struct
{
  osal_event_hdr_t  hdr;
  uint8             *msg;
} atOSALSerialData_t;

typedef union{
  struct{
  uint8 symbol;
  uint8 unitLen;
  uint8* unit;
  };
  struct{
    uint8 signal;
    uint8 len;
    uint16 test;
  };
} AT_CmdUnit;


uint8 AT_UartInit ( uint8 taskID );
void AT_UartRegisterTaskID( uint8 taskID );
void AT_UartProcess( uint8 port, uint8 event );
byte AT_UartCalcFCS( uint8 *msg_ptr, uint8 len );
void AT_HandleCMD(uint8 *msg);
void AT_UARTWriteErrMsg(uint8 error_code);
uint16 AT_HalUARTWrite(uint8 port, uint8 *buf, uint16 len);
uint8 AT_is_CMD_EPs(uint8 value );
uint8 AT_NV_ZCL_saveEPStatus(uint8 offset, uint8* value);
uint8 AT_NV_ZCL_readEPStatus(uint8 offset, uint8* value);
uint8 AT_NV_ZCL_get_index_(uint8 value);
void  AT_clear_AT_SYSTEM_NVs(void);

/**********for parsing  command (TOOLS)*****************/
uint8 _AT_ChartoInt(uint8 n);
uint8 AT_ChartoInt8(AT_CmdUnit *cmdUnit);
uint16 AT_ChartoInt16(AT_CmdUnit *cmdUnit);
void AT_ChartoIntx(AT_CmdUnit *cmdUnit,uint8 *pHex, uint8 x);
void AT_EUI64toChar(uint8* EUI64,char* str);
void AT_Int16toChar(uint16 integer16,char* str);
void AT_Int8toChar(uint8 n,char* str);
void AT_capitalizeCmd(AT_CmdUnit *cmdUnit);
void AT_sort_arr(uint8 *a, uint8 array_size);




#if AT_ENABLE_PASSWORDS_MODE
  void AT_Cmd_DISABLE(uint8 start_point, uint8* msg);
  void AT_Cmd_ENABLE(uint8 start_point, uint8* msg);
#endif
void AT_Cmd_ATI(uint8 start_point, uint8* msg);
void AT_Cmd_EPENABLE(uint8 start_point, uint8* msg);
void AT_Cmd_REPENABLE(uint8 start_point, uint8* msg);
void AT_Cmd_EPPRINT(uint8 start_point, uint8* msg);
void AT_Cmd_REPPRINT(uint8 start_point, uint8* msg);
void AT_Cmd_EN(uint8 start_point, uint8* msg);
void AT_Cmd_ESCAN(uint8 start_point, uint8* msg);
void AT_Cmd_PANSCAN(uint8 start_point, uint8* msg);
void AT_Cmd_DASSL(uint8 start_point, uint8* msg);
void AT_Cmd_DASSR(uint8 start_point, uint8* msg);
void AT_Cmd_PJOIN(uint8 start_point, uint8* msg);
void AT_Cmd_JPAN(uint8 start_point, uint8* msg);
void AT_Cmd_JN(uint8 start_point, uint8* msg);
void AT_Cmd_NTABLE(uint8 start_point, uint8* msg);
void AT_Cmd_RTABLE(uint8 start_point, uint8* msg);
void AT_Cmd_NODEDESC(uint8 start_point, uint8* msg);
void AT_Cmd_POWERDESC(uint8 start_point, uint8* msg);
void AT_Cmd_ACTEPDESC(uint8 start_point, uint8* msg);
void AT_Cmd_SIMPLEDESC(uint8 start_point, uint8* msg);
void AT_Cmd_MATCHREQ(uint8 start_point, uint8* msg);
void AT_Cmd_ANNCE(uint8 start_point, uint8* msg);
void AT_Cmd_ATABLE(uint8 start_point, uint8* msg);
void AT_Cmd_LBTABLE(uint8 start_point, uint8* msg);
void AT_Cmd_BSET(uint8 start_point, uint8* msg);
void AT_Cmd_BCLR(uint8 start_point, uint8* msg);
void AT_Cmd_BTABLE(uint8 start_point, uint8* msg);
void AT_Cmd_BIND(uint8 start_point, uint8* msg);
void AT_Cmd_UNBIND(uint8 start_point, uint8* msg);
void AT_Cmd_EBIND(uint8 start_point, uint8* msg);
void AT_Cmd_CLEARBIND(uint8 start_point, uint8* msg);
void AT_Cmd_READATR(uint8 start_point, uint8* msg);
void AT_Cmd_WRITEATR(uint8 start_point, uint8* msg);
void AT_Cmd_DISCOVER(uint8 start_point, uint8* msg);
void AT_Cmd_CLUSDISC(uint8 start_point, uint8* msg);
void AT_Cmd_ATTRDISC(uint8 start_point, uint8* msg);
void AT_Cmd_IDENTIFY(uint8 start_point, uint8* msg);
void AT_Cmd_IDQUERY(uint8 start_point, uint8* msg);
void AT_Cmd_RONOFF(uint8 start_point, uint8* msg);
void AT_Cmd_RONOFF1(uint8 start_point, uint8* msg);
void AT_Cmd_LONOFF(uint8 start_point, uint8* msg);
void AT_Cmd_ATZ(uint8 start_point, uint8* msg);
void AT_Cmd_AT_F(uint8 start_point, uint8* msg);
void AT_Cmd_ATS(uint8 start_point, uint8* msg);
void AT_Cmd_ESCAN1(uint8 start_point, uint8* msg);
void AT_Cmd_N(uint8 start_point, uint8* msg);
void AT_Cmd_READNV(uint8 start_point, uint8* msg);
void AT_Cmd_WRITENV(uint8 start_point, uint8* msg);
void AT_Cmd_INITNV(uint8 start_point, uint8* msg);
void AT_Cmd_STP(uint8 start_point, uint8* msg);
void AT_Cmd_GTP(uint8 start_point, uint8* msg);
void AT_Cmd_RSSIREQ(uint8 start_point, uint8* msg);
void AT_Cmd_PSEXP(uint8 start_point, uint8* msg);
void AT_Cmd_SPSEXP(uint8 start_point, uint8* msg);
void AT_Cmd_R(uint8 start_point, uint8* msg);
void AT_Cmd_IDREQ(uint8 start_point, uint8* msg);
void AT_Cmd_EUIREQ(uint8 start_point, uint8* msg);
void AT_Cmd_HELP(uint8 start_point, uint8* msg);
void AT_Cmd_TEST(uint8 start_point, uint8* msg);

#endif