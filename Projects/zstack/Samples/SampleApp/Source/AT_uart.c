/**************************************************************************************************
  Filename:       AT_uart.c

  Description:    AT command module
  Author:         Xiao Wang
**************************************************************************************************/


#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "hal_uart.h"
#include "OSAL_Memory.h"
#include "AF.h"
#include "AT_uart.h"
#include "AT_ONOFF_output.h"
#include "ZDNwkMgr.h"
#include "ZDProfile.h"
#include "hal_led.h"
#include "AT_App.h"


const char* Revision = "Private Revision:1.0";
const uint8 Revision_len = sizeof("Private Revision:1.0");
byte AT_Uart_TaskID;
extern uint8 continueJoining;
extern uint8 zdoDiscCounter;
uint8 AT_state =0;
uint8 AT_tempLen =0;
atOSALSerialData_t  *pAT_Msg;
uint8 AT_RxBuffer[MT_UART_RX_BUFF_MAX];
#if AT_ENABLE_PASSWORDS_MODE
  uint8 AT_enable=0;
  const char* const AT_passwords =  AT_ENABLE_PASSWORDS;
#endif

const AT_Cmd_t AT_Cmd_Arr[]={
#if AT_ENABLE_PASSWORDS_MODE
  {"DISABLE", AT_Cmd_DISABLE, "Disable the AT command"},
#endif
  {"EN" ,     AT_Cmd_EN,      "Establish Network cmmmand"},
  {"DASSL",   AT_Cmd_DASSL,   "Disassociate Local Device From PAN cmmmand"},
  {"PJOIN",   AT_Cmd_PJOIN,   "Permit joining cmmmand"},
  {"JPAN",    AT_Cmd_JPAN,    "Join Specific PAN:ch,PAN,EPAN"},
  {"JN",      AT_Cmd_JN,      "Join Network"},
  {"READATR", AT_Cmd_READATR, "Gets an Attribute From Specified Cluster Server cmmmand"},
  {"WRITEATR",AT_Cmd_WRITEATR,"Sets An Attribute to Specified Cluster Client cmmmand"},
  {"RONOFF",  AT_Cmd_RONOFF,  "Switching Target Devices Between ＆On＊ and ＆Off＊ States cmmmand"},
  {"LONOFF",  AT_Cmd_LONOFF,  "Switch Local Device On/Off"},
  {"ESCAN",   AT_Cmd_ESCAN,   "Scan The Energy Of All Channels\n\rPlease waitting..."},
  {"PANSCAN", AT_Cmd_PANSCAN, "Scan For Active PANs\n\rPlease waitting..."},
  {"N", AT_Cmd_N, "Display Network Information"},
  {"READNV", AT_Cmd_READNV, "Read NV ID,offset,len"},
  {"WRITENV", AT_Cmd_WRITENV, "WRITE NV ID,offset,number"},
  {"INITNV",  AT_Cmd_INITNV, "Initialize NV ID,lenth"},
  {"IDREQ",  AT_Cmd_IDREQ, "Request Node＊s NodeID:Address,XX"},
  {"EUIREQ",  AT_Cmd_EUIREQ, "Request Node＊s EUI64:Address,nodeID,XX"},
  {"TEST", AT_Cmd_TEST, "JUST for test and development"}
};
  
const AT_smap_unit_t AT_smap[] = {
  {0,AT_SMAP_PROTECTED, AT_S_NULL_ID},
  {4,AT_SMAP_READ|AT_SMAP_WRITTEN , AT_S_CHANNEL_MASK_ID},
  {1,AT_SMAP_READ|AT_SMAP_WRITTEN , AT_S_TRANSMIT_POWER_ID},
  {2,AT_SMAP_READ|AT_SMAP_WRITTEN , AT_S_PREFERRED_PANID_ID},
  {8,AT_SMAP_READ|AT_SMAP_WRITTEN , AT_S_PREFERRED_EXT_PANID_ID},
  {8,AT_SMAP_READ , AT_S_LOCAL_EUI_ID},
  {2,AT_SMAP_READ , AT_S_LOCAL_NODEID_ID},
  {8,AT_SMAP_READ , AT_S_PARENT_EUI_ID},
  {2,AT_SMAP_READ , AT_S_PARENT_NODEID_ID},
  {16,AT_SMAP_WRITTEN|AT_SMAP_PROTECTED , AT_S_NET_KEY_ID},
  {16,AT_SMAP_WRITTEN|AT_SMAP_PROTECTED , AT_S_LINK_KEY_ID},
  {2,AT_SMAP_READ|AT_SMAP_WRITTEN|AT_SMAP_PROTECTED , AT_S_MAIN_FUN_ID},
  {8,AT_SMAP_READ|AT_SMAP_WRITTEN|AT_SMAP_PROTECTED , AT_S_USER_NAME_ID},
  {8,AT_SMAP_WRITTEN|AT_SMAP_PROTECTED  , AT_S_PASSWORD_ID},
  {0,AT_SMAP_READ , AT_S_DEVICE_INF_ID},
  {0,AT_SMAP_READ|AT_SMAP_WRITTEN , AT_S_UART_SETUP_ID},
  {0,AT_SMAP_READ|AT_SMAP_WRITTEN , AT_S_MANUFACTURE_CODE_ID},
  {0,AT_SMAP_READ|AT_SMAP_WRITTEN , AT_S_PMW_PRESCALER_VAL_ID},
  {0,AT_SMAP_READ|AT_SMAP_WRITTEN , AT_S_BUTTON_FUN_ID}
};

/*********************************************************************
 * LOCAL FUNCTIONS
 *******************************************************************/
int8 AT_CmpCmd(AT_CmdUnit* cmdUnit,uint8* str2);
uint8 AT_get_next_cmdUnit(AT_CmdUnit* cmdUnit,uint8 start_point, uint8* msg);
uint8 getAT_CMDlength(uint8 *msg);
uint8 AT_strLen(char * str);
uint8 AT_get_smap_index(uint8 S_ID);



uint8 AT_UartInit( uint8 taskID )
{
  halUARTCfg_t uartConfig;

  AT_Uart_TaskID = 0;

  /* UART Configuration */
  uartConfig.configured           = TRUE;
  uartConfig.baudRate             = AT_UART_BR;
  uartConfig.flowControl          = FALSE;
  uartConfig.flowControlThreshold = MT_UART_THRESHOLD;
  uartConfig.rx.maxBufSize        = MT_UART_RX_BUFF_MAX;
  uartConfig.tx.maxBufSize        = MT_UART_TX_BUFF_MAX;
  uartConfig.idleTimeout          = MT_UART_IDLE_TIMEOUT;
  uartConfig.intEnable            = TRUE;
  uartConfig.callBackFunc         = AT_UartProcess;
  
  AT_Uart_TaskID = taskID;
  return HalUARTOpen ( AT_UART_PORT, &uartConfig);
}



/***************************************************************************************************
 *
 * @brief   | Head  |   Data   |  End  |   FCS  |
 *          |   2   |  0-Len   |   1   |    1   |
 *          |   AT  |    ?     |  <CR> | verify |
 *
 *          Parses the data and determine either is SPI or just simply serial data
 *          then send the data to correct place (MT or APP)
 *
 * @param   port     - UART port
 *          event    - Event that causes the callback
 *
 *
 * @return  None
 ***************************************************************************************************/
void AT_UartProcess( uint8 port, uint8 event ){
  uint8 ch;
  (void) event;
#if AT_ENABLE_PASSWORDS_MODE
  uint8 start_point=0;
  AT_CmdUnit cmdUnit;
#endif
  while(Hal_UART_RxBufLen( port )){
    HalUARTRead(port,&ch,1);
    
    switch(AT_state){
    case AT_HEAD_STATE1:
      if(ch =='A' || ch=='a') AT_state = AT_HEAD_STATE2;
      
      AT_UART_ECHO();    //very important for cc2530, cause this can avoid uart to die in very harsh envionment.
      break;
      
    case AT_HEAD_STATE2:
      if(ch =='T' || ch=='t') AT_state = AT_DATA_STATE;
         else if(ch =='A' || ch=='a') AT_state = AT_HEAD_STATE2;
         else AT_state = AT_HEAD_STATE1;  
      AT_UART_ECHO();     //very important for cc2530, cause this can avoid uart to die in very harsh envionment.
      break;
      
    case AT_DATA_STATE:
      if(ch =='\r'){       //means end signal <CR>
        AT_RxBuffer[AT_tempLen++]='\r';
#if AT_ENABLE_PASSWORDS_MODE
        if (AT_enable){
#endif
          
#if AT_FCS_VERIFY
          AT_state = AT_FCS_STATE ;
#else
          goto LABEL1;
#endif
#if AT_ENABLE_PASSWORDS_MODE
        }else{
          start_point=AT_get_next_cmdUnit(&cmdUnit,start_point, AT_RxBuffer);
          if(cmdUnit.symbol=='+' && AT_CmpCmd(&cmdUnit,"ENABLE")==0){
            start_point=AT_get_next_cmdUnit(&cmdUnit,start_point, AT_RxBuffer);
            if(cmdUnit.symbol==':' && AT_CmpCmd(&cmdUnit,(uint8 *)AT_passwords)==0){
              AT_enable=1;
              AT_tempLen=0;
              AT_state = AT_HEAD_STATE1;
              AT_OK();
              return;
            }
          }
          AT_tempLen=0;
          AT_state = AT_HEAD_STATE1;
          
        }
#endif
      }
      else{
         /* if the buffer is not full, read the data */
        if(AT_tempLen <MT_UART_RX_BUFF_MAX-1) AT_RxBuffer[AT_tempLen++] = ch;
        else{
          
          /*excessive data error. so,clear RXUART buffer , re-start to receive the command*/
          AT_state = AT_HEAD_STATE1;
          AT_tempLen=0;
          while(Hal_UART_RxBufLen(port)){
            HalUARTRead(port,&ch,1);
          }
        }
      }
      
      AT_UART_ECHO();   //very important for cc2530, cause this can avoid uart to die in very harsh envionment.
      

      break;
    case AT_FCS_STATE:
#if AT_FCS_VERIFY
      /* Make sure it's correct. ch is the FCS token*/
      if ((AT_UartCalcFCS (AT_RxBuffer, AT_tempLen) == ch)  ){
#else
      if (1){
LABEL1:
#endif
#if AT_MSG_SEND_MODE
        pAT_Msg = (atOSALSerialData_t *)osal_msg_allocate( sizeof ( atOSALSerialData_t )+AT_tempLen);//it is a big bug to calculat wrong memory size. it lead all system unstable for only one byte wrong. (It is diffult to debug for such a fault).
        pAT_Msg->hdr.event = AT_CMD_MSG;
        pAT_Msg->msg = (uint8*)(pAT_Msg+1);
        while(AT_tempLen){
          AT_tempLen--;
          pAT_Msg->msg[AT_tempLen] = AT_RxBuffer[AT_tempLen];
        }
        osal_msg_send( AT_Uart_TaskID, (byte *)pAT_Msg );
#else
        AT_tempLen=0;
        AT_HandleCMD(AT_RxBuffer);
#endif
      }
       else{
         AT_tempLen=0;
         AT_ERROR(AT_FCS_ERROR); //very important for cc2530, cause this can avoid uart to die in very harsh envionment.
      }
      /* Reset the state, send or discard the buffers at this point */
      AT_state = AT_HEAD_STATE1; 
      break;
    } 
  }
}






byte AT_UartCalcFCS( uint8 *msg_ptr, uint8 len )
{
  byte x;
  byte xorResult;

  xorResult = 0;

  for ( x = 0; x < len; x++, msg_ptr++ )
    xorResult = xorResult ^ *msg_ptr;

  return ( xorResult );
}

void AT_UARTWriteErrMsg(uint8 error_code){
  uint8* errMsg1 = "\r\nERROR:xx\r\n";
  uint8 errMsg[sizeof("\r\nERROR:xx\r\n")];
  uint8 i=0;
  for(;i<sizeof(errMsg);i++){
    errMsg[i] = errMsg1[i];
  }
  errMsg[sizeof("\r\nERROR:")-1] = error_code/16 < 10 ? error_code/16+'0' : error_code/16-10+'A';
  errMsg[sizeof("\r\nERROR:x")-1] = error_code%16 < 10 ? error_code%16+'0' : error_code%16-10+'A';
  HalUARTWrite(  AT_UART_PORT,errMsg,sizeof("\r\nERROR:xx\r\n"));
}


/*****************************

AT command handler!!!


**********************************/
uint8 getAT_CMDlength(uint8 *msg){
  int i =0;
  for(;;i++){
    if(msg[i]=='\r')break;
  }
  return i;
}

void AT_EUI64toChar(uint8* EUI64,char* str){
  int i;
  for(i=0;i<8;i++){
    str[15-2*i] = EUI64[i]%16 < 10 ? EUI64[i]%16+'0' : EUI64[i]%16-10+'A';
    str[14-2*i] = EUI64[i]/16 < 10 ? EUI64[i]/16+'0' : EUI64[i]/16-10+'A';
  }
}

void AT_Int16toChar(uint16 integer16,char* str){
  uint16 n= integer16>>8;
    str[1] = n%16 < 10 ? n%16+'0' : n%16-10+'A';
    str[0] = n/16 < 10 ? n/16+'0' : n/16-10+'A';
    n=integer16&0xff;
    str[3] = n%16 < 10 ? n%16+'0' : n%16-10+'A';
    str[2] = n/16 < 10 ? n/16+'0' : n/16-10+'A';
}

void AT_Int8toChar(uint8 n,char* str){
    str[1] = n%16 < 10 ? n%16+'0' : n%16-10+'A';
    str[0] = n/16 < 10 ? n/16+'0' : n/16-10+'A';
}
uint8 _AT_ChartoInt(uint8 n){
  if(n<='9'&&n>='0') return n-'0';
  if(n<='f'&&n>='a') return n-'a'+10;
  if(n<='F'&&n>='A') return n-'A'+10;
  return 0xff;
}
uint8 AT_ChartoInt8(AT_CmdUnit *cmdUnit){
  uint8 result=0;
  if(cmdUnit->unitLen>0) result |= _AT_ChartoInt(cmdUnit->unit[cmdUnit->unitLen-1]);
  if(cmdUnit->unitLen>1) result |= _AT_ChartoInt(cmdUnit->unit[cmdUnit->unitLen-2])<<(1*4);
  return result;
}
uint16 AT_ChartoInt16(AT_CmdUnit *cmdUnit){
  uint16 result=0;
  if(cmdUnit->unitLen>0) result |= _AT_ChartoInt(cmdUnit->unit[cmdUnit->unitLen-1]);
  if(cmdUnit->unitLen>1) result |=((uint16) _AT_ChartoInt(cmdUnit->unit[cmdUnit->unitLen-2]))<<(1*4);
  if(cmdUnit->unitLen>2) result |=((uint16) _AT_ChartoInt(cmdUnit->unit[cmdUnit->unitLen-3]))<<(2*4);
  if(cmdUnit->unitLen>3) result |=((uint16) _AT_ChartoInt(cmdUnit->unit[cmdUnit->unitLen-4]))<<(3*4);
  return result;
}
void AT_ChartoIntx(AT_CmdUnit *cmdUnit,uint8 *pHex, uint8 x){
  uint8 len=x/8;
  uint8 i;
  for(i=0;i<len;i++){
    pHex[i]=0;
  }
  for(i=0;i<len;i++){
    if(cmdUnit->unitLen>2*i) pHex[i] |= _AT_ChartoInt(cmdUnit->unit[cmdUnit->unitLen-1-2*i]);
    else break;
    if(cmdUnit->unitLen>2*i+1) pHex[i] |= _AT_ChartoInt(cmdUnit->unit[cmdUnit->unitLen-2-2*i])<<(1*4);
    else break;
  }
  
}
uint8 AT_get_smap_index(uint8 S_ID){
    uint8 index;          //for check record
    for(index=1;index<sizeof(AT_smap)/sizeof(AT_smap_unit_t);index++){
      if(AT_smap[index].S_ID==S_ID) return index;
    }
    return 0x00;
}

uint8 AT_get_next_cmdUnit(AT_CmdUnit* cmdUnit,uint8 start_point, uint8* msg){
  
  cmdUnit->unitLen=0;
  for(;;start_point++){
    if(msg[start_point] == ' '||msg[start_point] == '\0'){
      continue;
    }
    else if((msg[start_point]<='z' && msg[start_point]>='a') || 
       (msg[start_point]<='Z' && msg[start_point]>='A') || 
       (msg[start_point]<='9' && msg[start_point]>='0')){
       cmdUnit->symbol ='\0';                                  //indicate no operator
       break;
    }
    else {
      cmdUnit->symbol =msg[start_point];
      start_point++;
      break;
    }
  }
  
  for(;;start_point++){
    if(msg[start_point] == ' '|| msg[start_point] == '\0'){
      continue;
    }
    else while((msg[start_point]<='z' && msg[start_point]>='a') || 
       (msg[start_point]<='Z' && msg[start_point]>='A') || 
       (msg[start_point]<='9' && msg[start_point]>='0') )   {
       if(cmdUnit->unitLen==0) cmdUnit->unit = &msg[start_point];
       cmdUnit->unitLen++;
       start_point++;
    }
    return start_point;
  }
}

/*
uint8 CmpStr(uint8* str1,uint8* str2);
uint8 CmpStr(uint8* str1,uint8* str2){
  int i;
  for(i=0;str1[i]!='\0' && str2[i]!='\0';i++){
    if(str1[i]!=str2[i]) return str1[i]-str2[i];
  }
  return str1[i]-str2[i];
}
*/
int8 AT_CmpCmd(AT_CmdUnit* cmdUnit,uint8* str2){
  int i;
  for(i=0;i<cmdUnit->unitLen;i++){
    if(cmdUnit->unit[i]!=str2[i]) return cmdUnit->unit[i]-str2[i];
  }
  return 0-str2[cmdUnit->unitLen];
}

uint8 AT_strLen(char * str){
  uint8 i;
  for(i=0;i<255;i++){
    if(str[i]=='\0') break;
  }
  return i;
}

void AT_HandleCMD(uint8 *msg){
  uint8 start_point=0;
  AT_DEBUG( msg, getAT_CMDlength(msg));
  AT_CmdUnit cmdUnit;
  uint16 i;
  start_point = AT_get_next_cmdUnit(&cmdUnit,start_point, msg);
  if(cmdUnit.symbol =='\r'){                        //means there is no any followed operator or cmmand
       AT_OK();
  }
  else if(cmdUnit.symbol =='\0'){                   ////indicate no operator
    if(AT_CmpCmd(&cmdUnit,"I")==0){
      //AT_DEBUG("\r\nDisplay Product Identification Information command\r\n", sizeof("\r\nDisplay Product Identification Information command\r\n"));
      AT_Cmd_ATI(start_point,msg);
    }else if(AT_CmpCmd(&cmdUnit,"Z")==0){
      AT_DEBUG("\r\nSoftware Reset\r\n", sizeof("\r\nSoftware Reset\r\n"));
      AT_Cmd_ATZ(start_point,msg);
    }else if(cmdUnit.unit[0]=='S'){
      start_point=1;
      AT_DEBUG("\r\nS-Register Access\r\n", sizeof("\r\nSoftware Reset\r\n"));
      AT_Cmd_ATS(start_point,msg);
    }
    else AT_ERROR(AT_LACK_OPERATOR);
  }
  else if(cmdUnit.symbol =='+'){  
    
    for(i=0;i<sizeof(AT_Cmd_Arr)/sizeof(AT_Cmd_Arr[0]);i++){
      if(AT_CmpCmd(&cmdUnit,(uint8*)AT_Cmd_Arr[i].AT_Cmd_str)==0){
        AT_DEBUG("\r\n",2);
        AT_DEBUG(AT_Cmd_Arr[i].description_str,AT_strLen(AT_Cmd_Arr[i].description_str));
        AT_DEBUG("\r\n",2);
        AT_Cmd_Arr[i].AT_CmdFn(start_point,msg);
        break;
      }
    }
    if(sizeof(AT_Cmd_Arr)/sizeof(AT_Cmd_Arr[0])==i){
      if(AT_CmpCmd(&cmdUnit,"")==0){
        AT_ERROR(AT_LACK_CMD);
      }else{
        AT_ERROR(AT_CMD_ERROR);
      }
    }
    /*
    if(AT_CmpCmd(&cmdUnit,"EN")==0) {
      AT_DEBUG("\r\nEstablish Network cmmmand\r\n", sizeof("\r\nEstablish Network cmmmand\r\n"));
    }else if(AT_CmpCmd(&cmdUnit,"DASSL")==0){
      AT_DEBUG("\r\nDisassociate Local Device From PAN cmmmand\r\n", sizeof("\r\nDisassociate Local Device From PAN cmmmand\r\n"));
    }else if(AT_CmpCmd(&cmdUnit,"PJOIN")==0){
      AT_DEBUG("\r\nPermit joining cmmmand\r\n", sizeof("\r\nPermit joining cmmmand\r\n"));
    }else if(AT_CmpCmd(&cmdUnit,"READATR")==0){
      AT_DEBUG("\r\nGets an Attribute From Specified Cluster Server cmmmand\r\n", sizeof("\r\nGets an Attribute From Specified Cluster Server cmmmand\r\n"));
    }else if(AT_CmpCmd(&cmdUnit,"WRITEATR")==0){
      AT_DEBUG("\r\nSets An Attribute to Specified Cluster Client cmmmand\r\n", sizeof("\r\nSets An Attribute to Specified Cluster Client cmmmand\r\n"));
    }else if(AT_CmpCmd(&cmdUnit,"RONOFF")==0){
      AT_DEBUG("\r\nSwitching Target Devices Between ＆On＊ and ＆Off＊ States cmmmand\r\n", sizeof("\r\nSwitching Target Devices Between ＆On＊ and ＆Off＊ States cmmmand\r\n"));
      AT_Cmd_RONOFF(start_point,msg);
    }else if(AT_CmpCmd(&cmdUnit,"LONOFF")==0){
      AT_Cmd_LONOFF(start_point,msg);
    }else if(AT_CmpCmd(&cmdUnit,"ESCAN")==0){
      AT_DEBUG("\r\nScan The Energy Of All Channels\r\n", sizeof("\r\nScan The Energy Of All Channels\r\n"));
      AT_DEBUG("\r\nPlease waitting...\r\n", sizeof("\r\nPlease waitting...\r\n"));
      AT_Cmd_ESCAN(start_point,msg);
    }else if(AT_CmpCmd(&cmdUnit,"PANSCAN")==0){
      AT_DEBUG("\r\nScan For Active PANs\r\n", sizeof("\r\nScan For Active PANs\r\n"));
      AT_DEBUG("\r\nPlease waitting...\r\n", sizeof("\r\nPlease waitting...\r\n"));
      AT_Cmd_PANSCAN(start_point,msg);
    }else if(AT_CmpCmd(&cmdUnit,"")==0){
      AT_ERROR(AT_LACK_CMD);
    }else{
      AT_ERROR(AT_CMD_ERROR);
    }*/
  }
  else if(cmdUnit.symbol =='&'){
    if(AT_CmpCmd(&cmdUnit,"F")==0){
      AT_DEBUG("\r\nRestore Local Device＊s Factory Defaults\r\n", sizeof("\r\nRestore Local Device＊s Factory Defaults\r\n"));
      AT_Cmd_AT_F(start_point,msg);
    }
    else AT_ERROR(AT_LACK_OPERATOR);
  }
  else{
    AT_ERROR(AT_OPERATOR_ERROR);
  }
}

#if AT_CMD_PATTERN_CHECK
uint8 AT_pattern_check(char* pattern, AT_CmdUnit cmdUnitArr[]);
uint8 AT_pattern_check(char* pattern, AT_CmdUnit cmdUnitArr[]){
  uint8 i=0; 
  for(;pattern[i+1]!='\0';i++) {
    if(pattern[i] != cmdUnitArr[i].symbol){
      if(cmdUnitArr[i].symbol =='\0') return AT_LACK_OPERATOR;
      else if(cmdUnitArr[i].symbol =='\r')return AT_LACK_PARA;
      else return AT_OPERATOR_ERROR;
    }
  }
  if(pattern[i]=='\r')
    if(cmdUnitArr[i].symbol!='\r') return AT_EXCESSIVE_PARA;
  else{
    if(pattern[i] != cmdUnitArr[i].symbol){
      if(cmdUnitArr[i].symbol =='\0') return AT_LACK_OPERATOR;
      else if(cmdUnitArr[i].symbol =='\r')return AT_LACK_PARA;
      else return AT_OPERATOR_ERROR;
    }
  }
  return AT_NO_ERROR;
}
#endif

/*****************************************************
  AT+RONOFF:<Address>,<EP>,<SendMode>[,<ON/OFF>] 
  AT+RONOFF:,,,[<ON/OFF>]
*********************************************************/

void AT_Cmd_RONOFF(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[5];
  uint8 i;
  for(i=0;i<5;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  
  AT_PARSE_CMD_PATTERN_ERROR(":,,,\r",cmdUnitArr); 
  
  uint8 endpoint=AT_ChartoInt8(&cmdUnitArr[1]);
  uint8 sendmode=AT_ChartoInt8(&cmdUnitArr[2]);
  uint8 on_off =AT_ChartoInt8(&cmdUnitArr[3]);
  if(cmdUnitArr[3].unitLen==0) on_off=2;      //stand for without onoff parameter
  else if(on_off==0) on_off=0;        //off
  else on_off=1;                      //on
  afAddrType_t dstAddr;
  dstAddr.endPoint = endpoint;
  dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);
  //dstAddr.panId =2016;//0;
  dstAddr.addrMode = sendmode==0 ? (afAddrMode_t)Addr16Bit : (afAddrMode_t) AddrGroup;
  switch(endpoint)
  {
    case AT_ONOFF_OUTPUT_ENDPOINT:
      AT_ONOFF_OUTPUT_Cmd_RONOFF(&dstAddr,on_off);
      break;
    default:
      break;
    
  }
  
 
  
}


/*****************************************************
  I 每 Display Product Identification Information
  Response 
           University of Essex <DeviceName>
           <Firmware Revision>
           <EUI64>
           OK 

           <DeviceName> is the device type 
           <Firmware Revision> is the firmware revision 
           <EUI64> is the device＊s IEEE 802.15.4 identifier
*********************************************************/

void AT_Cmd_ATI(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
  
  //uint8 pValue[8]={1,2,3,4,5,6,7,8};
  uint8 str[17];
  uint8 version[2];
  AT_EUI64toChar(NLME_GetExtAddr(),(char*)str);
  AT_Int8toChar(_NIB.nwkProtocolVersion,(char*)version);
  
  

  //ZDO_Config_Node_Descriptor.LogicalType
  AT_RESP_START();
  AT_RESP("University of Essex",sizeof("University of Essex"));
  AT_NEXT_LINE();
#if ( ZG_BUILD_COORDINATOR_TYPE )
    AT_RESP("COORDINATOR",sizeof("COORDINATOR"));
    //#define DEVICE_LOGICAL_TYPE ZG_DEVICETYPE_COORDINATOR
#elif ( ZG_BUILD_RTR_TYPE )
    AT_RESP("ROUTER",sizeof("ROUTER"));
    //#define DEVICE_LOGICAL_TYPE ZG_DEVICETYPE_ROUTER
#elif ( ZG_BUILD_ENDDEVICE_TYPE )
    AT_RESP("ENDDEVICE",sizeof("ENDDEVICE"));
    // Must be an end device
    //#define DEVICE_LOGICAL_TYPE ZG_DEVICETYPE_ENDDEVICE
#else
    AT_RESP("<unknow error>",sizeof("<unknow error>"));
    //#error ZSTACK_DEVICE_BUILD must be defined as something!
#endif
  AT_NEXT_LINE();
  AT_RESP(Revision,Revision_len);
  AT_NEXT_LINE();
  AT_RESP("EUI64:",sizeof("EUI64:"));
  AT_RESP(str,16);
  AT_RESP_END();
  
  AT_OK();
}

/*****************************************************
  Z 每 Software Reset
  Response 
           Response
           OK
*********************************************************/
void AT_Cmd_ATZ(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
  AT_OK();
  
  osal_start_timerEx( AT_Uart_TaskID, AT_RESET_EVENT, 50 ); //set timer ensure OK response from AT command is sent
}

/*****************************************************
  &F 每 Restore Local Device＊s Factory Defaults
  Response 
           Response
           OK
            Module performs a factory reset. 
            All non-volatile S Registers are updated with their factory defaults 
            and the node leaves the currently joined network
*********************************************************/
void AT_Cmd_AT_F(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  uint8 status;
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
  if(status=zgWriteStartupOptions( ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE)==ZSUCCESS){
    AT_OK();
    osal_start_timerEx( AT_Uart_TaskID, AT_RESET_EVENT, 200 ); //set timer ensure OK response from AT command is sent
  }
  else AT_ERROR(status);
}


/*****************************************************
  S 每 S-Register Access
  ATSXX?
          Response
          <data> 
          OK 
          or ERROR:<errorcode>

ATSXX=<data>
            Response
            OK 
            or ERROR:<errorcode>

Notes:
      Some S-Registers require a password for write access.
      The default password is ※password§. 
      Some S-Registers are read-only 
      and will return an error if you are trying to write to them
FORMART: ATSXX=<DATA>:PASSWORD OR ATSXX?:PASSWORD
*********************************************************/
void AT_Cmd_ATS(uint8 start_point, uint8* msg){          
  uint8 state;
  AT_CmdUnit registerID;
  start_point = AT_get_next_cmdUnit(&registerID,start_point, msg);
  if(registerID.symbol=='\0'){
    uint8 id = AT_ChartoInt8(&registerID);
    uint8 index=AT_get_smap_index(id);          //for check record
    if(AT_smap[index].Privilege &AT_SMAP_PROTECTED){
      AT_CmdUnit cmdUnitArr[3];
      
      uint8 i;
      for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);
        uint8 pw_index = AT_get_smap_index(AT_S_PASSWORD_ID);
        state=osal_nv_item_init( AT_S_PASSWORD_ID+AT_S_NV_OFFSET, AT_smap[pw_index].len, NULL );
         if(state==SUCCESS || state==NV_ITEM_UNINIT){
           if(cmdUnitArr[0].symbol=='?'){ AT_PARSE_CMD_PATTERN_ERROR("?:\r",cmdUnitArr);}
           else {AT_PARSE_CMD_PATTERN_ERROR("=:\r",cmdUnitArr);}
           uint8 *str = osal_mem_alloc(AT_smap[pw_index].len+1);
           str[AT_smap[pw_index].len]='\0';
           if ((state=osal_nv_read(AT_S_PASSWORD_ID+AT_S_NV_OFFSET, 0, AT_smap[pw_index].len, str))==SUCCESS){
              if(AT_CmpCmd( &cmdUnitArr[1],str)==0){
                  osal_mem_free(str);
                
                //code for protected register
                 if((state=osal_nv_item_init( id+AT_S_NV_OFFSET, AT_smap[index].len, NULL ))==SUCCESS){
                   uint8* str;
                   if(cmdUnitArr[0].symbol=='?'){
                     // for read
                      if((AT_smap[index].Privilege & AT_SMAP_READ)==0) {
                        AT_ERROR(AT_FATAL_ERROR);
                        return;
                      }
                      str = osal_mem_alloc(AT_smap[index].len);
                      if ((state=osal_nv_read(id+AT_S_NV_OFFSET, 0, AT_smap[index].len, str))==SUCCESS){
                        //read successfully
                        char str2[2];
                        AT_RESP_START();             
                        for(i=0;i<AT_smap[index].len;i++) {
                          AT_Int8toChar(str[AT_smap[index].len-1-i],str2);
                          AT_RESP(str2,2);
                        }
                        AT_RESP_END();
                        AT_OK();
                      }
                      else AT_ERROR(state); 
                     
                   }
                   else {
                      //for writing
                      if((AT_smap[index].Privilege & AT_SMAP_WRITTEN)==0) {AT_ERROR(AT_FATAL_ERROR);return;};
                      if(id==AT_S_PASSWORD_ID){
                        if ((state=osal_nv_write(id+AT_S_NV_OFFSET, 0, AT_smap[index].len, cmdUnitArr[0].unit))==SUCCESS
                              && AT_smap[index].len==cmdUnitArr[0].unitLen){
                          //write successfully
                          AT_OK();
                          return;
                        }
                        else {AT_ERROR(state);return;}
                        
                      } 
                      str = osal_mem_alloc(AT_smap[index].len);
                      uint8 *str2 = osal_mem_alloc(2*AT_smap[index].len);
                      for(i=0;i<AT_smap[index].len*2;i++) {
                        if(i<cmdUnitArr[0].unitLen){
                          str2[AT_smap[index].len*2-1-i] =cmdUnitArr[0].unit[cmdUnitArr[0].unitLen-1-i];
                        }
                        else str2[AT_smap[index].len*2-1-i]='0';
                      }
                      AT_CmdUnit temp;
                      temp.unitLen=2;
                      for(i=0;i<AT_smap[index].len;i++){
                        temp.unit = str2+2*i;
                        str[AT_smap[index].len-1-i] = AT_ChartoInt8(&temp);
                      }
                      osal_mem_free(str2);
                      if ((state=osal_nv_write(id+AT_S_NV_OFFSET, 0, AT_smap[index].len, str))==SUCCESS){
                        //write successfully
                        AT_OK();
                      }
                      else AT_ERROR(state);
                     
                   }
                   osal_mem_free(str);
                 }else AT_ERROR(state);
                
                
              }
              else {
                osal_mem_free(str);
                AT_ERROR(AT_PASSWORD_ERROR);
              }
           }else AT_ERROR(state);
         }else AT_ERROR(state);
        
    }
    else{
      AT_CmdUnit cmdUnitArr[2];
      uint8 i;
      for(i=0;i<2;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);
          state=osal_nv_item_init( id+AT_S_NV_OFFSET, AT_smap[index].len, NULL );
          if(state==SUCCESS || state==NV_ITEM_UNINIT){
            uint8* str;
            if(cmdUnitArr[0].symbol=='?'){
              if((AT_smap[index].Privilege & AT_SMAP_READ)==0) {AT_ERROR(AT_FATAL_ERROR);return;};
              AT_PARSE_CMD_PATTERN_ERROR("?\r",cmdUnitArr);
              str = osal_mem_alloc(AT_smap[index].len);
              if ((state=osal_nv_read(id+AT_S_NV_OFFSET, 0, AT_smap[index].len, str))==SUCCESS){
                //read successfully
                char str2[2];
                AT_RESP_START();             
                for(i=0;i<AT_smap[index].len;i++) {
                  AT_Int8toChar(str[AT_smap[index].len-1-i],str2);
                  AT_RESP(str2,2);
                }
                AT_RESP_END();
                AT_OK();
              }
              else AT_ERROR(state);
              osal_mem_free(str);
            }else{
              if((AT_smap[index].Privilege & AT_SMAP_WRITTEN)==0) {AT_ERROR(AT_FATAL_ERROR);return;};  //check privilege
              AT_PARSE_CMD_PATTERN_ERROR("=\r",cmdUnitArr);
              str = osal_mem_alloc(AT_smap[index].len);
              uint8 *str2 = osal_mem_alloc(2*AT_smap[index].len);
              for(i=0;i<AT_smap[index].len*2;i++) {
                if(i<cmdUnitArr[0].unitLen){
                  str2[AT_smap[index].len*2-1-i] =cmdUnitArr[0].unit[cmdUnitArr[0].unitLen-1-i];
                }
                else str2[AT_smap[index].len*2-1-i]='0';
              }
              AT_CmdUnit temp;
              temp.unitLen=2;
              for(i=0;i<AT_smap[index].len;i++){
                temp.unit = str2+2*i;
                str[AT_smap[index].len-1-i] = AT_ChartoInt8(&temp);
              }
              osal_mem_free(str2);
              if ((state=osal_nv_write(id+AT_S_NV_OFFSET, 0, AT_smap[index].len, str))==SUCCESS){
                //write successfully
                AT_OK();
              }
              else AT_ERROR(state);
              osal_mem_free(str);
            }
          }else AT_ERROR(state);
    }
        
  }
  else AT_ERROR(AT_CMD_ERROR);
}


/*****************************************************
+LONOFF:[<ON/OFF>] 每 Set On or Off Status for the Local Node (For Local LED )
<ON/OFF> - A Boolean type to choose transmission mode,
          0 每 means Off; 
          1 每 means On. 
          If this field is left blank, the command will be a toggle command.

  Response 
           Response OK or ERROR:<errorcode>
*********************************************************/
void AT_Cmd_LONOFF(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[2];
  uint8 i;
  for(i=0;i<2;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr); 
  
  if(cmdUnitArr[0].unitLen==0) HalLedSet( HAL_LED_1,HAL_LED_MODE_TOGGLE);
  else{
    uint8 onoff;
    onoff = AT_ChartoInt8(&cmdUnitArr[0]);
    if(onoff)HalLedSet( HAL_LED_1,HAL_LED_MODE_ON);
    else HalLedSet( HAL_LED_1,HAL_LED_MODE_OFF);
  }
  AT_OK();
}


/*****************************************************
AT+ESCAN

Response +ESCAN:  11:XX
                   ＃
                  26:XX 
                  OK or ERROR:<errorcode> .
                  XX represents the average energy on the respective channel
*********************************************************/
uint8 AT_Cmd_SCAN_signal=0;
void AT_Cmd_ESCAN(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr);  
 
  NLME_ScanFields_t fields;
  fields.channels = 0x07FFF800;
  fields.duration = BEACON_ORDER_1_SECOND;
  fields.scanType = ZMAC_ED_SCAN;
  fields.scanApp = NLME_ED_SCAN;
  uint8 status;
  if(status=NLME_NwkDiscReq2(&fields)==ZSuccess){
    AT_Cmd_SCAN_signal=1;
    HalUARTSuspend();
  }else{
    AT_ERROR(status);
  }
}


/*****************************************************
AT+PANSCAN

          Response +PANSCAN:<channel>,<PANID>,<EPANID>,XX,b
          OK or ERROR:<errorcode> . 

          <channel> represents the channel,
          <PANID> the PAN ID, 
          <EPANID> the extended PAN ID, The node gives a list of all PANs found. 
          XX the ZigBee stack profile (00 = Custom, 01 = ZigBee, 02 = ZigBee PRO) 
          b indicates whether the network is allowing additional nodes to join (1 = joining permitted).
*********************************************************/
void AT_Cmd_PANSCAN(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr);  
  
  NLME_ScanFields_t fields;
  fields.channels = 0x07FFF800;
  fields.duration = BEACON_ORDER_480_MSEC;
  fields.scanType = ZMAC_ACTIVE_SCAN;
  fields.scanApp = NLME_DISC_SCAN;
  uint8 status;
  if(status=NLME_NwkDiscReq2(&fields)==ZSuccess){
    AT_Cmd_SCAN_signal=1;
    HalUARTSuspend();
  } else{
    AT_ERROR(status);
  }
}
void AT_Cmd_ESCANCB( ZDNwkMgr_EDScanConfirm_t *pEDScanConfirm);
void AT_Cmd_ESCANCB( ZDNwkMgr_EDScanConfirm_t *pEDScanConfirm){
  HalUARTResume();
  
  AT_RESP_START();
  char str[20];
  uint8 i;
  for ( i = 0; i < ED_SCAN_MAXCHANNELS; i++ )
  {
     if ( ( (uint32)1 << i ) & pEDScanConfirm->scannedChannels ){
       
       AT_Int8toChar(i,str);
       AT_RESP(str,2);
       AT_RESP(":",1);
       AT_Int8toChar(pEDScanConfirm->energyDetectList[i],str);
       AT_RESP(str,2);
       AT_NEXT_LINE();
     }
  }
  AT_RESP_END();
  
  AT_OK();
  NLME_NwkDiscTerm();
  AT_Cmd_SCAN_signal=0;
}

void AT_Cmd_PANSCANCB(uint8 ResultCount,networkDesc_t *NetworkList );
void AT_Cmd_PANSCANCB(uint8 ResultCount,networkDesc_t *NetworkList ){
  HalUARTResume();
  networkDesc_t *pNwkDesc = NetworkList;
  
  AT_RESP_START();
  char str[20];
  uint8 i;
  for ( i = 0; i < ResultCount && pNwkDesc->chosenRouterLinkQuality>0; i++, pNwkDesc = pNwkDesc->nextDesc ){
      AT_Int8toChar(pNwkDesc->logicalChannel,str);
      AT_RESP(str,2);
      AT_RESP(",",1);
      AT_Int16toChar(pNwkDesc->panId,str);
      AT_RESP(str,4);
      AT_RESP(",",1);
      AT_EUI64toChar(pNwkDesc->extendedPANID,str);
      AT_RESP(str,16);
      AT_RESP(",",1);
      AT_Int8toChar(pNwkDesc->stackProfile,str);
      AT_RESP(str,2);
      AT_RESP(",",1);
      AT_NEXT_LINE();
  }    
  AT_RESP_END();
  
  AT_OK();
  NLME_NwkDiscTerm();
  AT_Cmd_SCAN_signal=0;
}


/************************************************************
+N 每 Display Network Information
Response
    +N=<devicetype>,<channel>,<power>,<PANID>,<EPANID> 
    or +N=NoPAN
    followed by OK 

    <devicetype> represents the node＊s functionality in the PAN (FFD,COO,ZED,SED,MED) 
    <power> the node＊s output power in dBm 
    <channel> the IEEE 802.15.4 radio channel (11-26) 
    <PANID> the node＊s PAN ID 
    <EPANID> the node＊s extended PAN ID.
*********************************************************/
void AT_Cmd_N(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr);
  /* zAddrType_t dst;
  dst.addr.shortAddr=0;
  dst.addrMode =(afAddrMode_t)Addr16Bit;
  ZDP_PowerDescReq(&dst,0,0);
  */  
  //ZDO_Config_Power_Descriptor.AvailablePowerSources = NODEAVAILPWR_RECHARGE;
  //ZDO_Config_Power_Descriptor.CurrentPowerSource = NODEAVAILPWR_RECHARGE;
  //ZDO_Config_Power_Descriptor.CurrentPowerSourceLevel = NODEPOWER_LEVEL_66;
  //ZDConfig_UpdateNodeDescriptor();
  //ZDConfig_UpdatePowerDescriptor();
  // ZDO_Config_Power_Descriptor.CurrentPowerSourceLevel
  char str[20];
   AT_RESP_START();
   AT_RESP("=",1);
  switch (ZDO_Config_Node_Descriptor.LogicalType){
  case NODETYPE_COORDINATOR:
    AT_RESP("COO",3);
    break;
  case NODETYPE_ROUTER:
    AT_RESP("ROUTER",6);
    break;
  case NODETYPE_DEVICE:
    AT_RESP("ZED",3);
    break;
  }
  AT_RESP(",",1);
  AT_Int8toChar(_NIB.nwkLogicalChannel,str);
  AT_RESP(str,2);
  AT_RESP(",",1);
  switch (ZDO_Config_Power_Descriptor.CurrentPowerSourceLevel){
  case NODEPOWER_LEVEL_CRITICAL:
    AT_RESP("Critical",8);
    break;
  case NODEPOWER_LEVEL_33:
    AT_RESP("33%",3);
    break;
  case NODEPOWER_LEVEL_66:
    AT_RESP("66%",3);
    break;
  case NODEPOWER_LEVEL_100:
    AT_RESP("100%",4);
    break;
  }
  AT_RESP(",",1);
  AT_Int16toChar(_NIB.nwkPanId,str);
  AT_RESP(str,4);
  AT_RESP(",",1);
  AT_EUI64toChar(_NIB.extendedPANID,str);
  AT_RESP(str,16);
  AT_RESP_END(); 
  AT_OK();
}



void AT_Cmd_INITNV(uint8 start_point, uint8* msg){
   AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<4;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  
  AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr); 
  uint8 state;
  uint16 id=AT_ChartoInt16(&cmdUnitArr[0]);
  uint16 len =AT_ChartoInt16(&cmdUnitArr[1]);
  state=osal_nv_item_init( id, len, NULL );
  if(state==SUCCESS || state==NV_ITEM_UNINIT  ){
    AT_OK();
  }
  else AT_ERROR(state);
  
}
void AT_Cmd_READNV(uint8 start_point, uint8* msg){
   AT_CmdUnit cmdUnitArr[4];
  uint8 i;
  for(i=0;i<4;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  
  AT_PARSE_CMD_PATTERN_ERROR(":,,\r",cmdUnitArr); 
  char str[2];
  uint8 state;
  uint16 id=AT_ChartoInt16(&cmdUnitArr[0]);
  uint16 offset =AT_ChartoInt16(&cmdUnitArr[1]);
  uint16 len =AT_ChartoInt16(&cmdUnitArr[2]);
  uint8 *buf = osal_msg_allocate( len);
  if((state=osal_nv_read(id, offset, len, buf ))==SUCCESS){
      uint8 i;  
      AT_RESP_START();
      for(i=0;i<len;i++){
        AT_Int8toChar(buf[i],str); 
        AT_RESP(str,2);
        AT_RESP(" ",1);
      }
      AT_RESP_END(); 
      AT_OK();
  }else AT_ERROR(state);
  osal_msg_deallocate( buf );
}
void AT_Cmd_WRITENV(uint8 start_point, uint8* msg){
   AT_CmdUnit cmdUnitArr[4];
  uint8 i;
  for(i=0;i<4;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  
  AT_PARSE_CMD_PATTERN_ERROR(":,,\r",cmdUnitArr); 
  uint16 id=AT_ChartoInt16(&cmdUnitArr[0]);
  uint16 offset =AT_ChartoInt16(&cmdUnitArr[1]);
  uint8 data =AT_ChartoInt8(&cmdUnitArr[2]);
  uint8 state;
  if((state=osal_nv_write( id, offset,1, &data ))==SUCCESS) AT_OK();
  else AT_ERROR(state);
  
  
}
/****************************************************
+IDREQ 每 Request Node＊s NodeID (ZDO) 
  Execute Command 
            AT+IDREQ:<Address>[,XX]
  Response OK or ERROR:<errorcode>
  Prompt AddrResp:
    <errorcode>[,<NodeID>,<EUI64>] [nn. <NodeID>]

    Where <Address> can be a node＊s EUI64, or address table entry 
    and XX is an optional index number.
    Where an index number is provided, an extended response is requested 
    asking the remote device to list its associated devices (ie children). 
    It then sends a broadcast to obtain the specified Device＊s NodeID 
    and optionally also elements of its associated devices list.

***************************************************/
void AT_Cmd_IDREQ(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
 
    uint8 ext[8];
    AT_ChartoIntx(&cmdUnitArr[0],ext, 64);
    if(cmdUnitArr[1].symbol==',' ){
      
      AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr);
      uint8 state;
      state = ZDP_NwkAddrReq( ext, ZDP_ADDR_REQTYPE_EXTENDED,
                                AT_ChartoInt8(&cmdUnitArr[1]), 0 );
      if(state!=afStatus_SUCCESS) AT_ERROR(state);
    }else{
      AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr);
      uint8 state;
      state = ZDP_NwkAddrReq( ext, ZDP_ADDR_REQTYPE_SINGLE,
                            0, 0 );
      if(state!=afStatus_SUCCESS) AT_ERROR(state);
   
    }
  
}
void AT_Cmd_EUIREQ(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[4];
  uint8 i;
  for(i=0;i<4;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  if(cmdUnitArr[2].symbol==',' ){
    AT_PARSE_CMD_PATTERN_ERROR(":,,\r",cmdUnitArr);
    uint8 state;
    state = ZDP_IEEEAddrReq( AT_ChartoInt16(&cmdUnitArr[1]), ZDP_ADDR_REQTYPE_EXTENDED,
                            AT_ChartoInt8(&cmdUnitArr[2]), 0 );
    if(state!=afStatus_SUCCESS) AT_ERROR(state);
    
  }else{
    AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr); 
    uint8 state;
    state = ZDP_IEEEAddrReq( AT_ChartoInt16(&cmdUnitArr[1]), ZDP_ADDR_REQTYPE_SINGLE,
                            0, 0 );
    if(state!=afStatus_SUCCESS) AT_ERROR(state); 
    
  }
}


void AT_Cmd_EN(uint8 start_point, uint8* msg){
   AT_CmdUnit cmdUnitArr[4];
  uint8 i;
  for(i=0;i<4;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  
  AT_PARSE_CMD_PATTERN_ERROR(":,,\r",cmdUnitArr); 
  
  uint8 status;
  uint32 channel=1;
  
  if (1|| ZG_BUILD_COORDINATOR_TYPE && NODETYPE_COORDINATOR )
  {
    status = NLME_NetworkFormationRequest( AT_ChartoInt16(&cmdUnitArr[2]), _NIB.extendedPANID, channel<<AT_ChartoInt8(&cmdUnitArr[0]),
                                          STARTING_SCAN_DURATION, BEACON_ORDER_NO_BEACONS,
                                          BEACON_ORDER_NO_BEACONS, false );
  }
  else{
    AT_ERROR(AT_WRONG_DEV_ERROR);
    return;
  }
  if(status!=ZSUCCESS){
    AT_ERROR(status);
    return;
  }
  AT_OK();
}
void AT_Cmd_DASSL(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr);  
  
  /*typedef struct
    {
    uint8* extAddr;
    uint8 removeChildren;
    uint8 rejoin;
    uint8 silent;
    } NLME_LeaveReq_t;
  */
  NLME_LeaveReq_t req={
    aExtendedAddress,
    false,
    false,
    true
  };
  uint8 state;
  if((state=NLME_LeaveReq( &req ))==ZSUCCESS){
    AT_OK();
  }else AT_ERROR(state);
  
  
}
void AT_Cmd_PJOIN(uint8 start_point, uint8* msg){
   AT_CmdUnit cmdUnitArr[2];
  uint8 i;
  for(i=0;i<2;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr); 
/*  
  zAddrType_t addr={
    {_NIB.nwkDevAddress},
    (afAddrMode_t)Addr16Bit
  };
*/  
  uint8 state;
  if(cmdUnitArr[0].unitLen==0){
    //if((state=ZDP_MgmtPermitJoinReq(&addr,60,false,0))==ZSUCCESS){ 
    if((state=NLME_PermitJoiningRequest(60))==ZSUCCESS){
      AT_OK();
    }else AT_ERROR(state);
  }
  else{
    uint16 duration = AT_ChartoInt16(&cmdUnitArr[0]);
    //if((state=ZDP_MgmtPermitJoinReq(&addr,duration,false,0))==ZSUCCESS){
    if((state=NLME_PermitJoiningRequest(duration))==ZSUCCESS){
      AT_OK();
    }else AT_ERROR(state);
  }
  
}

/***************************************
+JPAN 每 Join Specific PAN Execute Command 
    AT+JPAN:[<channel>],[<PANID>],[<EPANID>]

**************************************************/
void AT_Cmd_JPAN(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[4];
  uint8 i;
  for(i=0;i<4;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":,,\r",cmdUnitArr); 
  

  
  //_NIB.nwkState = NWK_DISC;
  zdoDiscCounter=3;
  continueJoining=false;
  devState=DEV_NWK_DISC;
  uint8 state;
 // uint8 extPANID[8];
  uint32 channels;
  AT_ChartoIntx(&cmdUnitArr[2],ZDO_UseExtendedPANID, 64);
  if(cmdUnitArr[1].unitLen==0){
     zgConfigPANID =0xFFFF;
  }else zgConfigPANID=AT_ChartoInt16(&cmdUnitArr[1]);
  if(cmdUnitArr[0].unitLen==0){
    channels =0x07FFF800;
  }else channels=((uint32) 1)<<AT_ChartoInt8(&cmdUnitArr[0]);
  if((state=NLME_NetworkDiscoveryRequest( channels,BEACON_ORDER_480_MSEC))==ZSUCCESS){
    AT_OK();
  }else AT_ERROR(state);
  
  /*if ( (state=NLME_JoinRequest( extPANID,
                 AT_ChartoInt16(&cmdUnitArr[1]),
                 AT_ChartoInt8(&cmdUnitArr[0]),
                 ZDO_Config_Node_Descriptor.CapabilityFlags )) == ZSuccess )
            {
              AT_OK();
            }
  else AT_ERROR(state);*/
  
  
}
void AT_Cmd_JN(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
  continueJoining=true;
  zdoDiscCounter=1;
  devState=DEV_NWK_DISC;
  zgConfigPANID =0xFFFF;    //JOIN ANY EXIT NETWORK
  uint8 state;
  if((state=NLME_NetworkDiscoveryRequest( 0x07FFF800,BEACON_ORDER_480_MSEC))==ZSUCCESS){
    AT_OK();
  }else AT_ERROR(state);
}

void AT_Cmd_READATR(uint8 start_point, uint8* msg){
}
void AT_Cmd_WRITEATR(uint8 start_point, uint8* msg){
}



#if AT_ENABLE_PASSWORDS_MODE
void AT_Cmd_DISABLE(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[2];
  uint8 i;
  for(i=0;i<2;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr); 
  
  if(AT_CmpCmd(cmdUnitArr,(uint8 *)AT_passwords)==0){
    AT_enable=0; 
    AT_OK();
  }else{
    AT_ERROR(AT_WRONG_PASSWORD);
  }
}
#endif

void AT_Cmd_TEST(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
 
  AT_RESP_START();
  char str[20];
  AT_Int8toChar(_NIB.nwkState,str);
  AT_RESP(str,2);
  AT_RESP_END(); 
  AT_OK();
  /*
  typedef enum
{
  NWK_INIT,
  NWK_JOINING_ORPHAN,
  NWK_DISC,
  NWK_JOINING,
  NWK_ENDDEVICE,
  PAN_CHNL_SELECTION,
  PAN_CHNL_VERIFY,
  PAN_STARTING,
  NWK_ROUTER,
  NWK_REJOINING
} nwk_states_t;*/
  
 /* // Capability Information Field Bitmap values
#define CAPINFO_ALTPANCOORD           0x01
#define CAPINFO_DEVICETYPE_FFD        0x02
#define CAPINFO_DEVICETYPE_RFD        0x00
#define CAPINFO_POWER_AC              0x04
#define CAPINFO_RCVR_ON_IDLE          0x08
#define CAPINFO_SECURITY_CAPABLE      0x40
#define CAPINFO_ALLOC_ADDR            0x80
  */
  

  
}