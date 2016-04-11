/**************************************************************************************************
  Filename:       AT_uart.c

  Description:    AT command module
  Author:         Xiao Wang
**************************************************************************************************/


#include "ZComDef.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "OSAL_Memory.h"
#include "ZDNwkMgr.h"
#include "ZDProfile.h"
#include "hal_led.h"
#include "zcl_general.h"
#include "BindingTable.h"
#include "AddrMgr.h"
#include "zcl.h"
#include "ZDObject.h"

#include "At_include.h"

const char* Revision = "Private Revision:3.4 \n\rFor Temperature sensor";
byte AT_Uart_TaskID;
const uint8 AT_CMD_EP_ARRAY[]=AT_CMD_EPs;
const uint8 AT_CMD_EPs_Num = sizeof(AT_CMD_EP_ARRAY);
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
  {"ENABLE",  AT_Cmd_ENABLE,  "Enable the AT command AT+ENABLE:<password>"},
#endif
  {"EPENABLE",AT_Cmd_EPENABLE,"Enable/Disable Local Endpoint EPENABLE:<Enable/Disable>,<EP>"},
  {"REPENABLE",AT_Cmd_REPENABLE,"Enable/Disable Remote Endpoint EPENABLE:[<nodeID>],<Enable/Disable>,<EP>"},
  {"EPPRINT" ,AT_Cmd_EPPRINT, "Print Local Endpoints Status"},
  {"REPPRINT",AT_Cmd_REPPRINT,"Print Remote Endpoints Status REPPRIN:<noteID>"},
  {"LONOFF",  AT_Cmd_LONOFF,  "Switch Local Device On/Off LONOFF:[ON/OFF]"},
  {"EN" ,     AT_Cmd_EN,      "Establish Network EN:[<channel>],[<POWER>],[<PANID>]"},
  {"N",       AT_Cmd_N,       "Display Network Information"},
  {"DASSL",   AT_Cmd_DASSL,   "Disassociate Local Device From PAN cmmmand"},
  {"DASSR",   AT_Cmd_DASSR,   "Disassociate Remote Node from PAN AT+DASSR:<address>"},
  {"ESCAN",   AT_Cmd_ESCAN,   "Scan The Energy Of All Channels"},
  {"PANSCAN", AT_Cmd_PANSCAN, "Scan For Active PANs"},
  {"PJOIN",   AT_Cmd_PJOIN,   "Permit joining cmmmand PJOIN:<sec>"},
  {"JPAN",    AT_Cmd_JPAN,    "Join Specific JPAN:[<channel>],[<PANID>],[<EPANID>]"},
  {"JN",      AT_Cmd_JN,      "Join Network"},
  {"NTABLE",  AT_Cmd_NTABLE,  "Display Neighbour Table NTABLE:XX,<address>"},
  {"RTABLE",  AT_Cmd_RTABLE,  "Display Routing Table   RTABLE:XX,<address>"},
  {"IDREQ",   AT_Cmd_IDREQ,   "Request network address (nodeID) IDREQ:[<Address>][,XX]"},
  {"EUIREQ",  AT_Cmd_EUIREQ,  "Request Node's EUI64 EUIREQ:[< Address>,<NodeID>][,XX]"},
  {"NODEDESC",AT_Cmd_NODEDESC,"Request Node's Descriptor NODEDESC:<Address>,<NodeID>"},
  {"POWERDESC",AT_Cmd_POWERDESC,"Request Node's Power Descriptor POWERDESC:<Address>,<NodeID>"},
  {"ACTEPDESC",AT_Cmd_ACTEPDESC,"Request Node's Active Endpoint List ACTEPDESC:<Address>,<NodeID>"},
  {"SIMPLEDESC",AT_Cmd_SIMPLEDESC,"Request Endpoint's Simple Descriptor SIMPLEDESC:<Address>,<NodeID>,<XX>"},
  {"MATCHREQ",AT_Cmd_MATCHREQ,"Find Nodes which Match a Specific Descriptor MATCHREQ:<ProfileID>,<NumInClusters>[,<InClusterList>],<NumOutClusters>[,<OutClusterList>]"},
  {"ANNCE",   AT_Cmd_ANNCE,   "Announce Local Device In The Network"},
  {"ATABLE",  AT_Cmd_ATABLE,  "Display Address Table"},
  {"LBTABLE", AT_Cmd_LBTABLE, "Display Local Binding Table"},
  {"BSET",    AT_Cmd_BSET,    "Set local Binding Table Entry BSET:<type>,<LocalEP>,<ClusterID>,<DstAddress>[,<DstEP>]"},
  {"BCLR",    AT_Cmd_BCLR,    "Clear local Binding Table Entry BCLR:XX[,<ClusterID>]"},
  {"BTABLE",  AT_Cmd_BTABLE,  "Display Binding Table BTABLE:XX,<address>"},
  {"BIND",    AT_Cmd_BIND,    "Create Binding on Remote Device BIND:[<address>],[<SrcAddress>],<SrcEP>,<ClusterID>,[<DstAddress>][,<DstEP>]"},
  {"UNBIND",  AT_Cmd_UNBIND,  "Delete Binding on Remote Device UNBIND:[<address>],[<SrcAddress>],<SrcEP>,<ClusterID>,[<DstAddress>][,<DstEP>]"},
  {"EBIND",   AT_Cmd_EBIND,    "End Device Bind EBIND:<EP>"},
  {"CLEARBIND",AT_Cmd_CLEARBIND,"Clear Local Binding Table"},
  {"READATR", AT_Cmd_READATR, "READATR:<Address>,<EP>,[<SendMode>],<ClusterID>,<AttrID>,...,< AttrID >"},
  {"WRITEATR",AT_Cmd_WRITEATR,"WRITEATR:<Address>,<EP>,[<SendMode>],<ClusterID>,<AttrID>,<DataType>,<Data>"},
  {"DISCOVER",AT_Cmd_DISCOVER,"Discover HA Devices On The HAN DISCOVER:<Cluster ID>[,<option>]"},
  {"CLUSDISC",AT_Cmd_CLUSDISC,"Discover All Supported Clusters on A Remote Device CLUSDISC:[<Node ID>],<EndPoint>"},
  {"ATTRDISC",AT_Cmd_ATTRDISC,"Find Supported Attributs On A Remote Device End Poin ATTRDISC:[<NodeID>],<EP>,<ClusterID>,<AttributeID>,<MaxNumofAttr>"},
  {"IDENTIFY",AT_Cmd_IDENTIFY,"Identify Claster IDENTIFY:[<Address>],<EP>,[<SendMode>],<Time>"},
  {"IDQUERY", AT_Cmd_IDQUERY, "Query If Target Device(s) In Identifying Mode IDQUERY:[<Address>],<EP>,[<SendMode>]"},
  {"RONOFF",  AT_Cmd_RONOFF,  "RONOFF:[<Address>],<EP>,[<SendMode>][,<ON/OFF>]"},
  {"READNV",  AT_Cmd_READNV,  "Read NV READNV:<ID>,<offset>,<lenth>"},
  {"WRITENV", AT_Cmd_WRITENV, "WRITE NV WRITENV:<ID>,<offset>,<hex>"},
  {"INITNV",  AT_Cmd_INITNV,  "Initialize NV INITNV:<ID>,<lenth>"},
  {"STP",     AT_Cmd_STP,     "Set Transmit Power STP:<power level> "},
  {"GTP",     AT_Cmd_GTP,     "Get Transmit Power"},
  {"RSSIREQ", AT_Cmd_RSSIREQ, "RSSI request RSSIREQ:<address>"},
  {"PSEXP",   AT_Cmd_PSEXP,   "Power Saving Experiment PSEXP:<address>,<count>,<interval>"},
  {"SPSEXP",  AT_Cmd_SPSEXP,  "Stop Power Saving Experiment"},
  {"R",       AT_Cmd_R,       "execute command on remote device R:<nodeID>[,SendMode]|<AT command>"},
  {"HELP",    AT_Cmd_HELP,    "all the AT commands:"}, 
  {"RONOFF1", AT_Cmd_RONOFF1, "RONOFF1:<Address>,<EP>,[<SendMode>][,<ON/OFF>]"},
  {"ESCAN1",   AT_Cmd_ESCAN1,   "Scan The Energy Of All Channels"},
  {"TEST",    AT_Cmd_TEST,    "JUST for test and development"}
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
static int8 AT_CmpCmd(AT_CmdUnit* cmdUnit,uint8* str2);
static uint8 AT_get_next_cmdUnit(AT_CmdUnit* cmdUnit,uint8 start_point, uint8* msg);
static uint8 getAT_CMDlength(uint8 *msg);
static uint8 AT_strLen(char * str);
static uint8 AT_get_smap_index(uint8 S_ID);
static uint8 AT_display_pre_cmd(void);


uint8 AT_UartInit( uint8 taskID )
{
  halUARTCfg_t uartConfig;

  AT_Uart_TaskID = 0;

  /* UART Configuration */
  uartConfig.configured           = TRUE;
  uartConfig.baudRate             = AT_UART_BR;
  uartConfig.flowControl          = FALSE;
  uartConfig.flowControlThreshold = MT_UART_THRESHOLD;
  uartConfig.rx.maxBufSize        = AT_UART_RX_BUFF_MAX;
  uartConfig.tx.maxBufSize        = AT_UART_TX_BUFF_MAX;
  uartConfig.idleTimeout          = MT_UART_IDLE_TIMEOUT;
  uartConfig.intEnable            = TRUE;
  uartConfig.callBackFunc         = AT_UartProcess;
  
  AT_Uart_TaskID = taskID;
  AT_RxBuffer[0]='\r';          //label the buffer's initial status
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
#if AT_re_input_key != '\0'
      else if(ch==AT_re_input_key){
#if AT_ENABLE_PASSWORDS_MODE
        if(AT_enable)
#endif
        {
          AT_tempLen=AT_display_pre_cmd();
          AT_state = AT_DATA_STATE;
        }
      }
#endif
      AT_UART_ECHO();    //very important for cc2530, cause this can avoid uart to die in very harsh envionment.
      break;
      
      
    case AT_HEAD_STATE2:
      if(ch =='T' || ch=='t') AT_state = AT_DATA_STATE;
         else if(ch =='A' || ch=='a') AT_state = AT_HEAD_STATE2;
         else AT_state = AT_HEAD_STATE1;  
      AT_UART_ECHO();     //very important for cc2530, cause this can avoid uart to die in very harsh envionment.
      break;
      
      
    case AT_DATA_STATE:
      if(ch=='\b'||ch=='\x7f'){         //for backspace function, allow user to delete characters
        if(AT_tempLen>0) AT_tempLen--;
          else AT_state = AT_HEAD_STATE2;
      }
      else if(ch =='\r'){       //means end signal <CR>
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
          AT_capitalizeCmd(&cmdUnit);
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
      else {
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
  AT_RESP(errMsg,sizeof("\r\nERROR:xx\r\n"));
}

/*******************************************************************
CALL HalUARTPoll() TO GET A FULL TEXT DISPLAY.
AVOID TEST LOST.
*********************************************************************/


uint16 AT_HalUARTWrite(uint8 port, uint8 *buf, uint16 len){
  uint16 cnt=0;
  if(len>0 && len<AT_UART_TX_BUFF_MAX){   //it takes me a long time to find this bug. if the len is not checked, the system will fail. if len==0, the system will loop here all the time.
    while((cnt=HalUARTWrite(port, buf, len))<len){
      buf+=cnt;
      len-=cnt;
      HalUARTPoll();//wait until the text is sent successfully
                    //when using Z-Stack 2.51a, we have to set the HAL_UART_ISR=1 and HAL_UART_DMA=0 compile flags to enable ISR mode 
                    //maybe the bug of the Z-Stack, the DMA mode does work with this
                    //if using Z-Stack 2.3.0, both the DMA mode and ISR mode are OK
      //while(1);
    }
  }
  return cnt;
}
/*******************************************************
clear all the NV items in AT command system
******************************************************/
void  AT_clear_AT_SYSTEM_NVs(void){
  //dele ZCL status NV 
  osal_nv_delete( AT_NV_ZCL_EP_STATUS_ID,osal_nv_item_len(AT_NV_ZCL_EP_STATUS_ID ) );
  uint8 i;
  for(i=0;i<sizeof(AT_smap);i++){
    osal_nv_delete( AT_smap[i].S_ID+AT_NV_ZCL_EP_STATUS_ID,
                   osal_nv_item_len(AT_smap[i].S_ID+AT_NV_ZCL_EP_STATUS_ID));
  }
  
}
/*******************************************************************
if the endpoint is a cmd endpoint defined in AT_CMD_EPs. return ture, otherwise, return false
*********************************************************************/
uint8 AT_is_CMD_EPs(uint8 value){
  uint8 index;
  for(index=0;index<AT_CMD_EPs_Num;index++){
    if(AT_CMD_EP_ARRAY[index]==value)return true;
  }
  return false;
}
/*******************************************************************
save the endpoint status to NV
*********************************************************************/
uint8 AT_NV_ZCL_saveEPStatus(uint8 offset, uint8* value){
  uint8 state;
  state=osal_nv_item_init( AT_NV_ZCL_EP_STATUS_ID, AT_CMD_EPs_Num, NULL );
  if(state==SUCCESS || state==NV_ITEM_UNINIT  );
  else return state;
  
  return osal_nv_write( AT_NV_ZCL_EP_STATUS_ID, offset,1, value );
}

/*******************************************************************
read the endpoint status from NV
*********************************************************************/
uint8 AT_NV_ZCL_readEPStatus(uint8 offset, uint8* value){
  uint8 state;
  state=osal_nv_item_init( AT_NV_ZCL_EP_STATUS_ID, sizeof(AT_CMD_EP_ARRAY), NULL );
  if(state==SUCCESS || state==NV_ITEM_UNINIT  );
  else return state;
  
  return osal_nv_read( AT_NV_ZCL_EP_STATUS_ID, offset,1, value );
}
/*******************************************************************
get index of the end point in AT_CMD_EP_ARRAY[]
*********************************************************************/
uint8 AT_NV_ZCL_get_index_(uint8 value){
  uint8 index;
  for(index=0;index<sizeof(AT_CMD_EP_ARRAY);index++){
    if(AT_CMD_EP_ARRAY[index]==value)return index;
  }
  return 0xFF;
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
void AT_capitalizeCmd(AT_CmdUnit *cmdUnit){
  //capitalize the characters
  uint8 i;
  for(i=0;i<cmdUnit->unitLen;i++){  
    if(cmdUnit->unit[i]<='z' && cmdUnit->unit[i]>='a') cmdUnit->unit[i] += ('A'-'a');
 }
}
/**************************************************************
sorting the array
****************************************************************/
void AT_sort_arr(uint8 *a, uint8 array_size){
  uint8 i, j, index;
  for (i = 1; i < array_size; ++i)
     {
          index = a[i];
          for (j = i; j > 0 && a[j-1] > index; j--)
               a[j] = a[j-1];

          a[j] = index;
     }

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
uint8 AT_display_pre_cmd(void){
  uint8 i;
  for(i=0;;i++){
    if(AT_RxBuffer[i]== '\r') break;
  }
  AT_DEBUG("AT",2);
  AT_DEBUG(AT_RxBuffer,i);
  return i;
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
    else if(msg[start_point] == '\r'){        //indicate the end of one command
      cmdUnit->symbol =msg[start_point];
      return start_point;
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
  AT_DEBUG( "\n", 1);
  AT_DEBUG( msg, getAT_CMDlength(msg));
  AT_CmdUnit cmdUnit;
  uint16 i;
  start_point = AT_get_next_cmdUnit(&cmdUnit,start_point, msg);
  AT_capitalizeCmd(&cmdUnit);
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
    //deal with all the command in the command array
    for(i=0;i<sizeof(AT_Cmd_Arr)/sizeof(AT_Cmd_Arr[0]);i++){
      if(AT_CmpCmd(&cmdUnit,(uint8*)AT_Cmd_Arr[i].AT_Cmd_str)==0){
#if     AT_DEBUG_INFORMATION_SHOW
        AT_NEXT_LINE();
        printf(AT_Cmd_Arr[i].description_str);
#endif
        //AT_DEBUG(AT_Cmd_Arr[i].description_str,AT_strLen(AT_Cmd_Arr[i].description_str));
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
  }
  else if(cmdUnit.symbol =='&'){
    if(AT_CmpCmd(&cmdUnit,"F")==0){
      AT_DEBUG("\r\nRestore Local Device's Factory Defaults\r\n", sizeof("\r\nRestore Local Device's Factory Defaults\r\n"));
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

  afAddrType_t dstAddr;
  dstAddr.endPoint = endpoint;
  //dstAddr.panId =2016;//0;
  dstAddr.addrMode = sendmode==0 ? (afAddrMode_t)Addr16Bit : (afAddrMode_t) AddrGroup;
  if(cmdUnitArr[0].unitLen==0){
    dstAddr.addr.shortAddr=NLME_GetShortAddr();     
  }else{    
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);             
  }
  
  uint8 status;
  if(cmdUnitArr[3].unitLen==0) 
    status=zclGeneral_SendOnOff_CmdToggle(AT_ZCL_ENDPOINT,&dstAddr,0,1); //stand for without onoff parameter, toggle
  else if(on_off==0) 
    status=zclGeneral_SendOnOff_CmdOff(AT_ZCL_ENDPOINT,&dstAddr,0,1);    //off
  else status=zclGeneral_SendOnOff_CmdOn(AT_ZCL_ENDPOINT,&dstAddr,0,1);  //on
  if(status==ZSUCCESS) AT_OK();
  else AT_ERROR(status);
  
  //ZStatus_t zclGeneral_SendOnOff_CmdOff( uint16 srcEP, afAddrType_t *dstAddr, uint8 disableDefaultRsp, uint8 seqNum );
}


/*****************************************************
  AT+RONOFF1:<Address>,<EP>,<SendMode>[,<ON/OFF>] 
  AT+RONOFF1:,,,[<ON/OFF>]
*********************************************************/

void AT_Cmd_RONOFF1(uint8 start_point, uint8* msg){
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
  printf(Revision);
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
  &F 每 Restore Local Device's Factory Defaults
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
  
  AT_clear_AT_SYSTEM_NVs();
  if(status=zgWriteStartupOptions( ZG_STARTUP_SET, ZCD_STARTOPT_DEFAULT_NETWORK_STATE
                                  |ZCD_STARTOPT_DEFAULT_CONFIG_STATE)==ZSUCCESS){
    AT_OK();
    osal_start_timerEx( AT_Uart_TaskID, AT_RESET_EVENT, 200 ); //set timer ensure OK response from AT command is sent
  }
  else AT_ERROR(status);
}


/*****************************************************
+EPENABLE 每 Enable/Disable Local Endpoint

      AT+EPENABLE:<Enable/Disable>,<EP> 
            <Enable/Disable> - 0 for Disable; 1 for Enable 
            <EP> - 8 bit hexadecimal number Endpoint

Response 
      EPENABLED:<EP> or 
      EPDISABLED:<EP> or 
      UNKNOWNEP 
      OK
*********************************************************/

void AT_Cmd_EPENABLE(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  
  AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr); 
  
  uint8 enable =AT_ChartoInt8(&cmdUnitArr[0]);
  uint8 ep =AT_ChartoInt8(&cmdUnitArr[1]);
  char str[2];
  if(enable!=0) enable=1;
  
  if(enable){
    if( AT_af_get_ep(ep)){
      if(AT_af_register_ep(ep)==afStatus_SUCCESS){ 
        //enable the end point in ZCL layer
        AT_ZCL_EP_ENABLE( enable,ep);
        AT_NV_ZCL_saveEPStatus(AT_NV_ZCL_get_index_(ep),&enable);
      }
        AT_RESP_START();
        AT_RESP("ENABLED:",sizeof("ENABLED:")-1);
        AT_Int8toChar(ep,str);
        AT_RESP(str,2);
        AT_RESP_END();
    }else {
      
      AT_RESP_START();
      AT_RESP("UNKNOWNEP",sizeof("UNKNOWNEP")-1);
      AT_RESP_END();
      return;
    }
  }
  else{
    if(AT_af_get_ep(ep)){
      if(AT_af_remove_ep(ep)==afStatus_SUCCESS){
        //disable the end point in ZCL layer
        AT_ZCL_EP_ENABLE( enable,ep);
        //save to NV
        AT_NV_ZCL_saveEPStatus(AT_NV_ZCL_get_index_(ep),&enable);
      }
      AT_RESP_START();
      AT_RESP("DISABLED:",sizeof("DISABLED:")-1);
      AT_Int8toChar(ep,str);
      AT_RESP(str,2);
      AT_RESP_END();
    }else {
      AT_RESP_START();
      AT_RESP("UNKNOWNEP",sizeof("UNKNOWNEP")-1);
      AT_RESP_END();
      return;
    }
  }
  AT_OK();
}



/*****************************************************
+REPENABLE 每 Enable/Disable Remote Endpoint

      AT+REPENABLE:<nodeID>,<Enable/Disable>,<EP> 
            <Enable/Disable> - 0 for Disable; 1 for Enable 
            <EP> - 8 bit hexadecimal number Endpoint

Response 
      EPENABLED:<EP> or 
      EPDISABLED:<EP> or 
      UNKNOWNEP 
      OK
*********************************************************/
void AT_Cmd_REPENABLE(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[4];
  uint8 i;
  for(i=0;i<4;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":,,\r",cmdUnitArr); 
  
  uint8 enable =AT_ChartoInt8(&cmdUnitArr[1]);
  uint8 ep =AT_ChartoInt8(&cmdUnitArr[2]);
  if(enable!=0) enable=1;
  
  uint8 state;
  AT_AF_Cmd_REPENABLE_req_t buf; 
  buf.hdr.cmd = AT_AF_Cmd_req;
  buf.enable=enable; 
  buf.ep=ep; 
  state =  AT_AF_Cmd_send_simple(cmdUnitArr[0].unitLen ? AT_ChartoInt16(&cmdUnitArr[0]):NLME_GetShortAddr() ,
                      AT_AF_Cmd_REPENABLE_CLUSTERID,
                      sizeof(AT_AF_Cmd_REPENABLE_req_t),&buf);
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
  
}


/*****************************************************
  EPPRINT 每 Print Local Endpoints Status
  Response
        EP 0x01:ENABLED (or DISABLED) 
        EP 0x02:ENABLED (or DISABLED) 
        EP 0x03:ENABLED (or DISABLED) 
        EP 0x04:ENABLED (or DISABLED) 
        EP 0x05:ENABLED (or DISABLED) 
        OK
*********************************************************/
void AT_Cmd_EPPRINT(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr);
  
  uint8 epNum = afNumEndPoints()-1;
  byte* epBuf = (byte*)  osal_mem_alloc(epNum);
  if(epBuf==NULL) AT_ERROR(AT_MEM_ERROR);
  afEndPoints( epBuf, true);
  AT_sort_arr(epBuf,epNum);
  
  int i,j;
  char str[3];
  AT_RESP_START();
  for(i=0,j=0;j<sizeof(AT_CMD_EP_ARRAY);j++){
    AT_NEXT_LINE();
    AT_RESP("EP ",3);
    AT_Int8toChar(AT_CMD_EP_ARRAY[j],str);
    AT_RESP(str,2);
    if(epBuf[i]==AT_CMD_EP_ARRAY[j]){
      AT_RESP(":ENABLED",sizeof(":ENABLED")-1);
      i++;
    }
    else AT_RESP(":DISABLED",sizeof(":DISABLED")-1);
  }
    
  AT_RESP_END();
  AT_OK();
  osal_mem_free(epBuf);  
  //void afEndPoints( byte *epBuf, byte skipZDO );
}

/*****************************************************
REPPRINT:<noteID> 每 Print Remote Endpoints Status
  Response
        <nodeID>,<EndPoint>:<Status>
        XXXX,01:ENABLED (or DISABLED) 
        XXXX,02:ENABLED (or DISABLED) 
        XXXX,03:ENABLED (or DISABLED) 
        XXXX,04:ENABLED (or DISABLED) 
        XXXX,05:ENABLED (or DISABLED) 
        OK
*********************************************************/
void AT_Cmd_REPPRINT(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[2];
  uint8 i;
  for(i=0;i<2;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr);
  
  AT_AF_hdr buff;
  uint8 state; 
  buff.cmd = AT_AF_Cmd_req;
  state =  AT_AF_Cmd_send_simple(AT_ChartoInt16(&cmdUnitArr[0]),
                      AT_AF_Cmd_REPPRINT_CLUSTERID,sizeof(AT_AF_hdr),&buff);
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
  
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

Response +ESCAN1:  11:XX
                   ＃
                  26:XX 
                  OK or ERROR:<errorcode> .
                  XX represents the average energy on the respective channel
*********************************************************/
void AT_Cmd_ESCAN1(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr); 
/*
  zAddrType_t dstAddr; 
  osal_memcpy(dstAddr.addr.extAddr,NLME_GetExtAddr(),16);
  dstAddr.addrMode = (afAddrMode_t)Addr64Bit;
  uint8 state;
  */
  /*
  msg = inMsg->asdu;
  scan.channels = osal_build_uint32( msg, 4 );
  msg += 4;
  scan.duration = *msg++;
  index         = *msg;
  scan.scanType = ZMAC_ACTIVE_SCAN;
  scan.scanApp  = NLME_DISC_SCAN;
  */
  zAddrType_t dstAddr; 
  dstAddr.addr.shortAddr=NLME_GetShortAddr();
  dstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  uint8 state;
  
  state = ZDP_MgmtNwkDiscReq( &dstAddr,
                            0x07FFF800,
                            BEACON_ORDER_120_MSEC,
                            0,
                            0 );
  
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}



/*****************************************************
AT+ESCAN

Response +ESCAN:  11:XX
                   ＃
                  26:XX 
                  OK or ERROR:<errorcode> .
                  XX represents the average energy on the respective channel
*********************************************************/
void AT_Cmd_ESCANCB( NLME_EDScanConfirm_t *EDScanConfirm);
void AT_Cmd_ESCANCB( NLME_EDScanConfirm_t *EDScanConfirm){
  

  
  AT_RESP_START();
  uint8 i;
  for ( i = 0; i < ED_SCAN_MAXCHANNELS; i++ )
  {
     if ( ( (uint32)1 << i ) & EDScanConfirm->scannedChannels ){
       
       printf("%d:%02X",i,EDScanConfirm->energyDetectList[i]);
       if(i<ED_SCAN_MAXCHANNELS-1) AT_NEXT_LINE();
     }
  }
  AT_RESP_END();
  AT_OK();
  NLME_NwkDiscTerm();
  //recover the Energy scan call back funtion to ZDO layer so that ZDO can get the energy scan result
  extern void ZDNwkMgr_EDScanConfirmCB( NLME_EDScanConfirm_t *EDScanConfirm );
  pZDNwkMgr_EDScanConfirmCB = ZDNwkMgr_EDScanConfirmCB;
}

void AT_Cmd_ESCAN(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr);  

#if ZG_BUILD_RTR_TYPE|ZG_BUILD_COORDINATOR_TYPE  
  NLME_ScanFields_t fields;
  fields.channels = 0x07FFF800;
  fields.duration = BEACON_ORDER_1_SECOND;
  fields.scanType = ZMAC_ED_SCAN;
  fields.scanApp =NLME_ED_SCAN;
  uint8 status;
  
  //change the the call back function address of energe scan in NWK_layer. 
  pZDNwkMgr_EDScanConfirmCB = AT_Cmd_ESCANCB;
  
  if((status=NLME_NwkDiscReq2(&fields))==ZSuccess){
    AT_OK();
    AT_DEBUG("\n\rPlease waitting...\n\r",sizeof("\n\rPlease waitting...\n\r")-1);
  }else{
    NLME_NwkDiscTerm();
    AT_ERROR(status);
  }
#else
  AT_ERROR(AT_WRONG_DEV_ERROR );
#endif
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
  ZDO_RegisterForZdoCB(ZDO_NWK_DISCOVERY_CNF_CBID, AT_ZDO_ProcessNWKDISC_CB);
  if((status=NLME_NwkDiscReq2(&fields))==ZSuccess){
    AT_OK();
    AT_DEBUG("\n\rPlease waitting...\n\r",sizeof("\n\rPlease waitting...\n\r")-1);
  } else{
    AT_ERROR(status);
    NLME_NwkDiscTerm();
  }
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
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  
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
/**************************************************************************
Set Transmit Power STP:<power level>

      <power level>: -22 to 19 db


*******************************************************************************/
void AT_Cmd_STP(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[2];
  uint8 i;
  for(i=0;i<2;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr); 
  int8 level= (int8) AT_ChartoInt8(&cmdUnitArr[0]);
  uint8 state;
  if((state=ZMacSetTransmitPower((ZMacTransmitPower_t) level))==ZMacSuccess){
    AT_OK();
  }else{
    AT_ERROR(state);
  }
}
/**************************************************************************
Set Transmit Power

      <power level>: -22 to 19 db


*******************************************************************************/
void AT_Cmd_GTP(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  uint8 i;
  for(i=0;i<1;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  
  int8 level;
  uint8 state;
  if((state=MAC_MlmeGetReq(MAC_PHY_TRANSMIT_POWER_SIGNED,&level))==ZMacSuccess){
    AT_RESP_START();
    printf("GTP:%02d",level);
    AT_RESP_END();
    AT_OK();
  }else{
    AT_ERROR(state);
  }
}
/***********************************************************************************
    RSSI request RSSIREQ:<address>
          //16 bits node id
************************************************************************************/
void AT_Cmd_RSSIREQ(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[2];
  uint8 i;
  for(i=0;i<2;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr); 
  int16 addr=  AT_ChartoInt16(&cmdUnitArr[0]);
  uint8 state;
  AT_AF_hdr buf;
  buf.cmd =AT_AT_PSE_RSSI_req;
  if((state=AT_AF_Cmd_send_simple(addr,AT_AF_POWER_SVING_EXP_CLUSTERID,sizeof(buf), &buf))==ZSuccess){
    AT_OK();
  }else{
    AT_ERROR(state);
  }
}

/****************************************************************************
Power Saving Experiment PSEXP:<address><count><interval>
      address: network address
      count:  uint16 type data
      interval: uint16 type data, time unit: millisecond
*************************************************************************************/
void AT_Cmd_PSEXP(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[4];
  uint8 i;
  for(i=0;i<4;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":,,\r",cmdUnitArr); 
  
  int16 addr=  AT_ChartoInt16(&cmdUnitArr[0]);
  int16 count= AT_ChartoInt16(&cmdUnitArr[1]);
  int16 interval= AT_ChartoInt16(&cmdUnitArr[2]);
  
  uint8 state;
  AT_AF_Cmd_POWER_SAVING_EXP_t buf;
  buf.hdr.cmd =AT_AT_PSE_EXP_req;
  buf.hdr.info =AT_AF_PSE_info_pre;
  buf.count=count;
  buf.interval=interval;
  if((state=AT_AF_Cmd_send_simple(addr,AT_AF_POWER_SVING_EXP_CLUSTERID,sizeof(buf), &buf))==ZSuccess){
    
    AT_App_Cmd_POWER_SAVING_EXP_t pseBuf;
    pseBuf.nwkAddr =addr;
    pseBuf.count = count;
    pseBuf.interval=interval;
    
    if((state=AT_App_Power_saving_exp(&pseBuf))==AT_NO_ERROR){
      AT_OK();
    }else{
      AT_ERROR(state);
    }
  }else{
    AT_ERROR(state);
  }
}


/****************************************************************************
Stop Power Saving Experiment
*************************************************************************************/
void AT_Cmd_SPSEXP(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  uint8 i;
  for(i=0;i<1;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
  AT_App_Power_saving_exp_stop();
  AT_OK();
}

/****************************************************************************
execute command on remote device 
  R:<address>|<AT command>
*************************************************************************************/
void AT_Cmd_R(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  uint8 sendmode;
  uint8 *pCmd;
 #if AT_CMD_PATTERN_CHECK
  //built check pattern
  if(cmdUnitArr[0].symbol!=':')  AT_ERROR(AT_OPERATOR_ERROR);
  
  if(cmdUnitArr[1].symbol==',' && cmdUnitArr[2].symbol=='|') {
    sendmode=AT_ChartoInt8(&cmdUnitArr[1]);
    pCmd = cmdUnitArr[2].unit;
    AT_capitalizeCmd(&cmdUnitArr[2]);
  }else if(cmdUnitArr[1].symbol=='|'){
    sendmode=0;
    pCmd = cmdUnitArr[1].unit;
    AT_capitalizeCmd(&cmdUnitArr[1]);
  }else {
    AT_ERROR(AT_OPERATOR_ERROR);
    return;
  }
  
  //check command header
  if(pCmd[0]=='A' && pCmd[1]=='T'){}
  else{
    AT_ERROR(AT_CMD_ERROR);
    return;
  }
  pCmd+=2;
#endif
  
  uint8 len =0;
  for(i=0;i<AT_UART_RX_BUFF_MAX;i++){
    if(pCmd[i]!='\r') len++;
    else break;
  }
  if(i>=AT_UART_RX_BUFF_MAX) {AT_ERROR(AT_CMD_ERROR); return;}
  
  len++;//to include the end '\r'
  
  //build destination address
  afAddrType_t dstAddr;
  dstAddr.endPoint = AT_AF_ENDPOINT;
  dstAddr.addrMode = sendmode==0 ? (afAddrMode_t)Addr16Bit : (afAddrMode_t) AddrGroup;  
  if(cmdUnitArr[0].unitLen==0){
    dstAddr.addr.shortAddr=NLME_GetShortAddr();     
  }else{    
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);             
  }
  
  //contruct data buff
  AT_AF_Cmd_RCIDDISC_req_t *pBuf =(AT_AF_Cmd_RCIDDISC_req_t *) osal_mem_alloc(sizeof(AT_AF_Cmd_RCIDDISC_req_t)+len);
  if(pBuf==NULL) {
    AT_ERROR(AT_MEM_ERROR);
    return;
  }
  pBuf->hdr.cmd = AT_AF_Cmd_req;
  pBuf->hdr.numItem=len;
  osal_memcpy( pBuf->list, pCmd, len );
  uint8 state;
  if((state=AF_DataRequest( &dstAddr, & AT_AF_epDesc,
                       AT_AF_Cmd_R_CIDDISC_CLUSTERID,
                       sizeof(AT_AF_Cmd_RCIDDISC_req_t)+len,
                       (uint8 *)pBuf,
                       &AT_AF_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS ))==ZSuccess){
                         AT_OK();
                       }
      else{
        AT_ERROR(state);
      }
  
  osal_mem_free(pBuf);
  
}
/***********************************************************************
read non-volatile memory
***************************************************************************/
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
            AT+IDREQ:
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
    if(cmdUnitArr[0].unitLen==0) {
      AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr);
      uint8 state;
      state = ZDP_IEEEAddrReq( NLME_GetShortAddr(), ZDP_ADDR_REQTYPE_SINGLE,
                            0, 0 );
      if(state!=afStatus_SUCCESS) AT_ERROR(state); 
    }else if(cmdUnitArr[1].symbol==',' ){
      
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
    AT_OK();
  
}
/******************************************************************************
Execute Command 
      AT+EUIREQ:< Address>,<NodeID>[,XX]
      AT+EUIREQ:
            Where <Address> is the EUI64, Node ID or address table entry 
            of the node which is to be interrogated about the node with 
            the Node ID specified in <NodeID>. XX is an optional index number. 
            In case an index number is provided, an extended response is 
            requested asking the remote device to list its associated devices (i.e. children).

Prompt 
      AddrResp:<errorcode>[,<NodeID>,<EUI64>] 
         <EUI64> is the Remote node＊s EUI64 and 
        <NodeID> is its Node ID.When an extended response has been requested
                the requested NodeIDs from the associated devices list are listed.

*****************************************************************************/
void AT_Cmd_EUIREQ(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[4];
  uint8 i;
  for(i=0;i<4;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  if(cmdUnitArr[1].symbol=='\r'){
    AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr);
    uint8 state;
    state = ZDP_IEEEAddrReq( NLME_GetShortAddr(), ZDP_ADDR_REQTYPE_SINGLE,
                            0, 0 );
    if(state!=afStatus_SUCCESS) AT_ERROR(state); 
  }else if(cmdUnitArr[2].symbol==',' ){
    AT_PARSE_CMD_PATTERN_ERROR(":,,\r",cmdUnitArr);
    uint8 state;
    state = ZDP_IEEEAddrReq( AT_ChartoInt16(&cmdUnitArr[0]), ZDP_ADDR_REQTYPE_EXTENDED,
                            AT_ChartoInt8(&cmdUnitArr[2]), 0 );
    if(state!=afStatus_SUCCESS) AT_ERROR(state);
    
  }else{
    AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr); 
    uint8 state;
    state = ZDP_IEEEAddrReq( AT_ChartoInt16(&cmdUnitArr[0]), ZDP_ADDR_REQTYPE_SINGLE,
                            0, 0 );
    if(state!=afStatus_SUCCESS) AT_ERROR(state); 
    
  }
  AT_OK();
}

/************************************************************************
+EN 每 Establish Network 
      AT+EN:[<channel>],[<POWER>],[<PANID>] 
      Use on: Coordinator which are not part of a PAN 
      Note: Establishing a PAN can take up to 16 seconds. 
      This command can only be executed if the local node is 
      not part of a PAN already.
**************************************************************************/
void AT_Cmd_EN(uint8 start_point, uint8* msg){
   AT_CmdUnit cmdUnitArr[4];
  uint8 i;
  for(i=0;i<4;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  /*if(cmdUnitArr[0].symbol=='\r'){
    AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
  }else if(cmdUnitArr[1].symbol=='\r'){
    AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr);  
  }else{
    AT_PARSE_CMD_PATTERN_ERROR(":,,\r",cmdUnitArr); 
  }
  */
  
  if(devState==DEV_ZB_COORD){// if Started as Zigbee Coordinator. means: coordinator has started a network, so do nothing, only when a coordinator is not in a PAN, this command is valid
    AT_ERROR(AT_nwkState_ERROR);
    return;
  }
  AT_PARSE_CMD_PATTERN_ERROR(":,,\r",cmdUnitArr);
  uint8 status;
  uint32 channel=1;
  uint16 PANID;
  AT_ChartoIntx(&cmdUnitArr[2],ZDO_UseExtendedPANID, 64);
  if(cmdUnitArr[2].unitLen==0){
     PANID =0xFFFF;
  }else PANID=AT_ChartoInt16(&cmdUnitArr[2]);
  if(cmdUnitArr[0].unitLen==0){
    channel =0x07FFF800;
  }else channel=((uint32) 1)<<AT_ChartoInt8(&cmdUnitArr[0]);
  
  if (ZG_BUILD_COORDINATOR_TYPE )
  {  
    //register the ZDO call back functio to receive the join confirm
    ZDO_RegisterForZdoCB(ZDO_JOIN_CNF_CBID, AT_ZDO_ProcessJOIN_CNF_CB);
    status = NLME_NetworkFormationRequest( PANID, _NIB.extendedPANID, channel,
                                          STARTING_SCAN_DURATION, BEACON_ORDER_NO_BEACONS,
                                          BEACON_ORDER_NO_BEACONS, AT_ChartoInt16(&cmdUnitArr[1]) );
  }
  else{
    AT_ERROR(AT_WRONG_DEV_ERROR);
    
    //cancel the join confirm call back function because of the error
    ZDO_RegisterForZdoCB(ZDO_JOIN_CNF_CBID, AT_ZDO_ProcessJOIN_CNF_CB);
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
//be careful with the coordinator, it cannot join the network again.
  NLME_LeaveReq_t req={
    NULL, //NULL: remove this device. otherwise remove child device
    false,
    false,
    true
  };
  uint8 state;
  if((state=NLME_LeaveReq( &req ))==ZSUCCESS){
    AT_OK();
  }else AT_ERROR(state);
}
/*****************************************************************
+DASSR 每 Disassociate Remote Node from PAN (ZDO) 
      Execute Command AT+DASSR:<address> 
              Where 
              <address> can be a node＊s EUI64, NodeID or address table index 
              Use on All Devices 


      Note
              Use with care when targeting a Coordinator. 
              It will not be able to rejoin the PAN


********************************************************************/
void AT_Cmd_DASSR(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[2];
  uint8 i;
  for(i=0;i<2;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr); 
  

  zAddrType_t addr;
  if(cmdUnitArr[0].unitLen==0){
    AT_ERROR(AT_LACK_PARA);
    return;
  }else if(cmdUnitArr[0].unitLen>10){               
    AT_ChartoIntx(&cmdUnitArr[0],addr.addr.extAddr, 64);  
    addr.addrMode = (afAddrMode_t)Addr64Bit;                  
  }else{                                                         
    addr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);            
    addr.addrMode = (afAddrMode_t)Addr16Bit;       
  }
  uint8 state;
  uint8 nonValidIEEEAddr[8] ={0,0,0,0,0,0,0,0};
  state = ZDP_MgmtLeaveReq( &addr,nonValidIEEEAddr, 
                                   false,
                                   false, 
                                   false );

  if(state==ZSUCCESS){
    AT_OK();
  }else AT_ERROR(state);
}
/****************************************************************
AT+PJOIN:<sec> or AT+PJOIN 
              <sec> - 16 bit hexadecimal number which represents the length 
                      of time in seconds during which the ZigBee coordinator 
                      or router will allow associations.

in my: 0xff for permitting all the time
0x00 for disable the permitting
*****************************************************************/
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
  

  uint8 state;
 // uint8 extPANID[8];
  uint32 channels;
  
  if(ZG_BUILD_JOINING_TYPE){
    AT_ChartoIntx(&cmdUnitArr[2],ZDO_UseExtendedPANID, 64);
    if(cmdUnitArr[1].unitLen==0){
       zgConfigPANID =0xFFFF;
    }else zgConfigPANID=AT_ChartoInt16(&cmdUnitArr[1]);
    if(cmdUnitArr[0].unitLen==0){
      channels =0x07FFF800;
    }else channels=((uint32) 1)<<AT_ChartoInt8(&cmdUnitArr[0]);
    if(cmdUnitArr[2].unitLen==0){
      osal_memcpy( ZDO_UseExtendedPANID, "\0\0\0\0\0\0\0\0", 8);//invalid address enable the device allow all the ExtPANid
    }else AT_ChartoIntx(&cmdUnitArr[2],ZDO_UseExtendedPANID, 64);;
    
    //_NIB.nwkState = NWK_DISC;
    
    //register the ZDO call back functio to receive the join confirm 
    ZDO_RegisterForZdoCB(ZDO_JOIN_CNF_CBID, AT_ZDO_ProcessJOIN_CNF_CB);
    
    //these variable in ZDO layer will control the flow process of ZDO APP, so change it 
    //to reach the needed purpose.
    //similar with state machine
    extern devStartModes_t devStartMode;
    //zdoDiscCounter=1;
    //continueJoining=true;
    //zgDefaultChannelList =channels;
    //zgDefaultStartingScanDuration = BEACON_ORDER_480_MSEC;
    devState=DEV_NWK_DISC;    //change state to DEV_NEW_DISC the flow proces will tend to join or rejoin a network after a discover requset
    devStartMode=MODE_JOIN;   //change start mode will let the ZDO folow the JOIN proces, rather than the REJOIN process
   
    state=NLME_NetworkDiscoveryRequest( channels,BEACON_ORDER_480_MSEC);  //can be substitute by ZDO_StartDevice( byte logicalType, devStartModes_t startMode,
                                                                          //byte beaconOrder, byte superframeOrder );
                                                                          //however, if do so, the zgDefaultChannelList and the zgDefaultStartingScanDuration need to be set
                                                                          //to control channel and scan duration
                                                //using ZDO layer discover call back funtion to keep the ZDO working well, rather than bother it
  
  }else{
    //coordinator can't join to any pan
    //pan coordinator have only address 0x0000
    //it is the network starter
    AT_ERROR(AT_WRONG_DEV_ERROR);
    return;
  }
  if(state==ZSUCCESS){
    AT_OK();
  }else {
    //here I should recover the change of the ZDO variables, such as devState and devStartMode..
    //however, I don't want to change it, because, I want it try its best to join a network.
    
    //cancel the join confirm call back function because of the error 
    
    //ZDO_RegisterForZdoCB(ZDO_JOIN_CNF_CBID, NULL);
    AT_ERROR(state);
  }
  
 
  /*
  if ( (state=NLME_JoinRequest( extPANID,
                 AT_ChartoInt16(&cmdUnitArr[1]),
                 AT_ChartoInt8(&cmdUnitArr[0]),
                 ZDO_Config_Node_Descriptor.CapabilityFlags )) == ZSuccess )
            {
              AT_OK();
            }
  else AT_ERROR(state);
    */
  
  
}
void AT_Cmd_JN(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
  uint8 state;
  
  
  if(ZG_BUILD_JOINING_TYPE){
    //continueJoining=true;
    //zdoDiscCounter=1;
    //ZDO_UpdateNwkStatus( DEV_NWK_DISC );
    devState=DEV_NWK_DISC;
    zgConfigPANID =0xFFFF;    //JOIN ANY EXIT NETWORK
    
    extern devStartModes_t devStartMode;
    devStartMode=MODE_JOIN;   //change start mode will let the ZDO folow the JOIN proces, rather than the REJOIN process
    //register the ZDO call back functio to receive the join confirm 
    ZDO_RegisterForZdoCB(ZDO_JOIN_CNF_CBID, AT_ZDO_ProcessJOIN_CNF_CB);
    if((state=NLME_NetworkDiscoveryRequest( 0x07FFF800,BEACON_ORDER_480_MSEC))==ZSUCCESS){
      AT_OK();
    }else {
      AT_ERROR(state);
      
      //here I should recover the change of the ZDO variables, such as devState and devStartMode..
      //however, I don't want to change it, because, I want it try its best to join a network.
      
      //cancel/retrieve the join confirm call back function because of the error 
      ZDO_RegisterForZdoCB(ZDO_JOIN_CNF_CBID, NULL);
    }
  }
  else{
    //a coordinator cannot join any network,
    //it takes charge of starting a network
    AT_ERROR(AT_WRONG_DEV_ERROR);
  }
}

/*****************************************************************************
+NTABLE 每 Display Neighbour Table
     AT+NTABLE:XX,<address> 
        Where XX is the start index of the remote LQI table and 
        <address> can be the remote node＊s EUI64, Node ID or address table entry.

    This command requests the target node to respond by listing its neighbour table 
    starting from the requested index. Can be used to find the identity of all ZigBee 
    devices in the network including non-Telegesis devices. 
    Prompt (example) 
          NTable:<NodeID>,<errorcode> 
          Length:03 
          No.| Type | EUI | ID | LQI 
          0. | FFD | 000D6F000015896B | BC04 | FF 
          1. | FFD | 000D6F00000B3E77 | 739D | FF 
          2. | FFD | 000D6F00000AAD11 | 75E3 | FF
******************************************************************************/

void AT_Cmd_NTABLE(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr); 

  zAddrType_t dstAddr;
  uint8 startIndex = AT_ChartoInt16(&cmdUnitArr[0]);
  if(cmdUnitArr[1].unitLen==16){               
    AT_ChartoIntx(&cmdUnitArr[1],dstAddr.addr.extAddr, 64);  
    dstAddr.addrMode = (afAddrMode_t)Addr64Bit;                  
  }else{                                                         
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[1]);            
    dstAddr.addrMode = (afAddrMode_t)Addr16Bit;       
  }
  uint8 state;
  state = ZDP_MgmtLqiReq( &dstAddr, startIndex, 0 );
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}


/*****************************************************************************
+RTABLE 每 Display Routing Table
     AT+NTABLE:XX,<address> 
        Where XX is the start index of the remote LQI table and 
        <address> can be the remote node＊s EUI64, Node ID or address table entry.

    This command requests the target node to respond by listing its routing table 
    starting from the requested index. 
    Prompt (example) 
        RTable:<NodeID>,<errorcode> 
        Length:03 
        No.| Dest | Next | Status 
        0. | 1234 | ABCD | 00 
        1. | 4321 | 739D | 00 
        2. | 0000 | 0000 | 03
******************************************************************************/

void AT_Cmd_RTABLE(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr); 

  zAddrType_t dstAddr;
  uint8 startIndex = AT_ChartoInt16(&cmdUnitArr[0]);
  if(cmdUnitArr[1].unitLen==16){               
    AT_ChartoIntx(&cmdUnitArr[1],dstAddr.addr.extAddr, 64);  
    dstAddr.addrMode = (afAddrMode_t)Addr64Bit;                  
  }else{                                                         
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[1]);            
    dstAddr.addrMode = (afAddrMode_t)Addr16Bit;       
  }
  uint8 state;
  state = ZDP_MgmtRtgReq( &dstAddr, startIndex, 0 );
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}

/***********************************************************************
+NODEDESC 每 Request Node＊s Descriptor (ZDO)
      AT+NODEDESC:<Address>,<NodeID> Where 
            <Address> is the EUI64, NodeID or Address table entry of the node
                      which is to be interrogated about the node with the NodeID 
                      specified in <NodeID>. 
            Sends a unicast to obtain the specified device＊s node descriptor.


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
***************************************************************************/
void AT_Cmd_NODEDESC(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr); 

  zAddrType_t dstAddr;
  //uint8 startIndex = AT_ChartoInt16(&cmdUnitArr[2]);
  if(cmdUnitArr[0].unitLen>10){               
    AT_ChartoIntx(&cmdUnitArr[0],dstAddr.addr.extAddr, 64);  
    dstAddr.addrMode = (afAddrMode_t)Addr64Bit;                  
  }else{                                                         
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);            
    dstAddr.addrMode = (afAddrMode_t)Addr16Bit;       
  }
  uint8 state;
  state = ZDP_NodeDescReq(&dstAddr,AT_ChartoInt16(&cmdUnitArr[1]), 0);
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}

/***********************************************************************
+POWERDESC 每 Request Node＊s Power Descriptor (ZDO) 
    AT+POWERDESC:<Address>,<NodeID> Where 
          <Address> is the EUI64, Node ID or Address table entry of the node 
                    which is to be interrogated about the node with the Node 
                    ID specified in <NodeID>. 
          Sends a unicast to obtain the specified device＊s power descriptor.

    Prompt 
          PowerDesc:<NodeID>,<errorcode>[,<PowerDescriptor>]
***************************************************************************/
void AT_Cmd_POWERDESC(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr); 

  zAddrType_t dstAddr;
  //uint8 startIndex = AT_ChartoInt16(&cmdUnitArr[2]);
  if(cmdUnitArr[0].unitLen>10){               
    AT_ChartoIntx(&cmdUnitArr[0],dstAddr.addr.extAddr, 64);  
    dstAddr.addrMode = (afAddrMode_t)Addr64Bit;                  
  }else{                                                         
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);            
    dstAddr.addrMode = (afAddrMode_t)Addr16Bit;       
  }
  uint8 state;
  state = ZDP_PowerDescReq(&dstAddr,AT_ChartoInt16(&cmdUnitArr[1]), 0);
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}

/***********************************************************************
+ACTEPDESC 每 Request Node＊s Active Endpoint List (ZDO) 
        AT+ACTEPDESC:<Address>,<NodeID> Where 
                  <Address> is the EUI64, NodeID or Address table entry of the node 
                            which is to be interrogated about the node with the NodeID 
                            specified in <NodeID>. 
                  It then sends a unicast to obtain the specified device＊s active endpoint list.


Prompt 
        ActEpDesc:<NodeID>,<errorcode>[,XX,＃]
***************************************************************************/
void AT_Cmd_ACTEPDESC(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr); 

  zAddrType_t dstAddr;
  //uint8 startIndex = AT_ChartoInt16(&cmdUnitArr[2]);
  if(cmdUnitArr[0].unitLen>10){               
    AT_ChartoIntx(&cmdUnitArr[0],dstAddr.addr.extAddr, 64);  
    dstAddr.addrMode = (afAddrMode_t)Addr64Bit;                  
  }else{                                                         
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);            
    dstAddr.addrMode = (afAddrMode_t)Addr16Bit;       
  }
  uint8 state;
  state = ZDP_ActiveEPReq(&dstAddr,AT_ChartoInt16(&cmdUnitArr[1]), 0);
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}


/*************************************************************

+SIMPLEDESC 每 Request Endpoint＊s Simple Descriptor (ZDO) 
    AT+SIMPLEDESC:<Address>,<NodeID>,<XX> Where 
                  <Address> is the EUI64, NodeID or Address table entry of the node 
                            which is to be interrogated about the node with the NodeID 
                            specified in <NodeID> and 
                  XX is the number of the endpoint, which simple descriptor is to be read.
                  XX should be hexadecimal number. It then sends a unicast to obtain the 
                  specified device＊s active endpoint list.

Prompt 
    SimpleDesc:<NodeID>,<errorcode>   
    EP:XX 
    ProfileID:XXXX 
    DeviceID:XXXXvXX 
    InCluster:<Cluster List> 
    OutCluster:<Cluster List>
*************************************************************/
void AT_Cmd_SIMPLEDESC(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[4];
  uint8 i;
  for(i=0;i<4;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":,,\r",cmdUnitArr); 

  zAddrType_t dstAddr;
  uint8 XX = AT_ChartoInt16(&cmdUnitArr[2]);
  if(cmdUnitArr[0].unitLen>10){               
    AT_ChartoIntx(&cmdUnitArr[0],dstAddr.addr.extAddr, 64);  
    dstAddr.addrMode = (afAddrMode_t)Addr64Bit;                  
  }else{                                                         
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);            
    dstAddr.addrMode = (afAddrMode_t)Addr16Bit;       
  }
  uint8 state;
  state = ZDP_SimpleDescReq( &dstAddr, AT_ChartoInt16(&cmdUnitArr[1]), XX, 0 );
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}


/*************************************************************

+MATCHREQ 每 Find Nodes which Match a Specific Descriptor (ZDO) 
    AT+MATCHREQ:<ProfileID>,<NumInClusters>[,<InClusterList>],<NumOutClusters>[,<OutClusterList>] Where 
                <ProfileID> Required profile ID of the device being searched for followed by
                            a specification of required input and output clusters. If a remote
                             node has a matching ProfileID and matches at least one of the specified 
                            clusters it will respond to this broadcast listing the matching endpoint(s). 

                <NumInClusters> and <NumOutClusters> must be 2 hexadecimal digits

Prompt 
    MatchDesc:<NodeID>,<errorcode>,XX,＃
*************************************************************/
void AT_Cmd_MATCHREQ(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  uint8 i;
  uint16 ProfileID;
  uint8 NumInClusters;
  uint8 NumOutClusters;
  uint16 *InClusterList;
  uint16 *OutClusterList;
  //ProfileID
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR_1(":",cmdUnitArr);
  ProfileID= AT_ChartoInt16(&cmdUnitArr[0]);
  
  //NumInClusters
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR_1(",",cmdUnitArr);
  NumInClusters = AT_ChartoInt8(&cmdUnitArr[0]);
  
  //InClusterList
  InClusterList = (uint16*)osal_mem_alloc(NumInClusters*2);
  if(InClusterList==NULL) {
    AT_ERROR(AT_MEM_ERROR);
    return;
  }
  for(i=0;i<NumInClusters;i++){
    start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
    AT_PARSE_CMD_PATTERN_ERROR_1(",",cmdUnitArr);
    InClusterList[i]= AT_ChartoInt16(&cmdUnitArr[0]);
  }
  

  
  //NumOutClusters
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR_1(",",cmdUnitArr);
  NumOutClusters = AT_ChartoInt8(&cmdUnitArr[0]);
  
  //OutClusterList
  OutClusterList = (uint16*)osal_mem_alloc(NumOutClusters*2);
  if(OutClusterList==NULL) {
    AT_ERROR(AT_MEM_ERROR);
    return;
  }
  for(i=0;i<NumOutClusters;i++){
    start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
    AT_PARSE_CMD_PATTERN_ERROR_1(",",cmdUnitArr);
    OutClusterList[i]= AT_ChartoInt16(&cmdUnitArr[0]);
  }
  
  //chaeck the end of the command
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR_1("\r",cmdUnitArr);
  
    //build broadcast address
  zAddrType_t broad_addr={
    {0xffff},                       //addr
    (afAddrMode_t)AddrBroadcast,              //addr mode
  };

  uint8 state;
  state = ZDP_MatchDescReq( &broad_addr,0xffff,ProfileID,
                           NumInClusters ,InClusterList, 
                           NumOutClusters,OutClusterList,0);
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
  
  osal_mem_free(OutClusterList);
  osal_mem_free(InClusterList);
  
  /*
  
  ZDP_MatchDescReq( zAddrType_t *dstAddr, uint16 nwkAddr,
                                uint16 ProfileID,
                                byte NumInClusters, uint16 *InClusterList,
                                byte NumOutClusters, uint16 *OutClusterList,
                                byte SecurityEnable );*/
}


/*************************************************************

+ANNCE 每 Announce Local Device In The Network (ZDO)
    AT+ANNCE Send a ZigBee device announcement. 
             Broadcast announcing the local node on the network.

*************************************************************/
void AT_Cmd_ANNCE(uint8 start_point, uint8* msg){
  
  AT_CmdUnit cmdUnitArr[1];
  uint8 i;
  for(i=0;i<1;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr);
  
  uint8 state;
  state = ZDP_DeviceAnnce( NLME_GetShortAddr(), NLME_GetExtAddr(), _NIB.CapabilityFlags,0 );
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
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
/*********************************************************************
+ATABLE 每 Display Address Table 
          AT+ATABLE
Response 
        No. | Active | ID | EUI 
        00 | N | 0000 |000D6F0000012345 
        (＃) 
         OK
***********************************************************************/

void AT_Cmd_ATABLE(uint8 start_point, uint8* msg){AT_CmdUnit cmdUnitArr[1];
  uint8 i;
  for(i=0;i<1;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
  
  AddrMgrEntry_t entry;
  AT_RESP_START();
  printf("No. | user |  ID  | EUI ");
  for(entry.index=0;entry.index< NWK_MAX_ADDRESSES;entry.index++){
    if(AddrMgrEntryGet( &entry )){
      AT_NEXT_LINE();
      uint16* ext= (uint16*) entry.extAddr;
      printf("%02X. |  %02X  | %04X | %04X%04X%04X%04X",
             entry.index, entry.user,entry.nwkAddr,
             ext[3],ext[2],ext[1],ext[0]);
    }
    
  }
  AT_RESP_END();
  AT_OK();
}

/*****************************************************************************
+LBTABLE 每 Display Local Binding Table
        AT+LBTABLE 
              Use on All Devices The binding table is cleared by a reset
Response 
              No. | Type | Active | LocalEP | ClusterID | Addr | RemEP 
              10. | Ucast | No | 01 | DEAD | 1234567887654321 | 01 
              11. | MTO | No | 01 | DEAD | E012345678876543 | 88 
              12. | Mcast | No | 01 | DEAD | CDAB 
              13. | Unused 
              14. | Unused 
              15. | Unused 
              16. | Unused 
              17. | Unused 
              18. | Unused 
              19. | Unused
******************************************************************************/

void AT_Cmd_LBTABLE(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  uint8 i;
  for(i=0;i<1;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
  
  AT_RESP_START();
  uint8 j;
  uint16 maxEntries,usedEntries;
  bindCapacity( &maxEntries,&usedEntries );
  printf("maxEntries %d, maxCID %d", maxEntries,gMAX_BINDING_CLUSTER_IDS);
  AT_NEXT_LINE();
  printf("No. |  Type  | Active | LocalEP | ClusterIDs |      Address     | RemEP");
  char* type;
  zAddrType_t addr;
  char address[17];
  char dstEP[5];
  //j = bindAddClusterIdToList(BindingTable,2);
  uint16 cnt=0;
  for(i=0;i<maxEntries;i++){
    if(BindingTable[i].srcEP !=0xff){ //valid check
      if(BindingTable[i].dstGroupMode==1) {
        type="Mcast";
        sprintf(address,"%04X",BindingTable[i].dstIdx);
        dstEP[0]='\0';
      }
      else{
        bindingAddrMgsHelperConvert(BindingTable[i].dstIdx, &addr );
        if(addr.addrMode== Addr64Bit){
          type="Ucast";
          AT_EUI64toChar(addr.addr.extAddr,address);
          address[16]='\0';
          sprintf(dstEP,"| %02X",BindingTable[i].dstEP);
        }else{
          type="??";
          sprintf(address,"???????");
          sprintf(dstEP,"| %02X",BindingTable[i].dstEP);
        }
      }
      if(BindingTable[i].numClusterIds==0) {
          AT_NEXT_LINE();
          printf("%02X. | %-6s |  ????  |   %02X    |   unused   | %-16s %s",
                 cnt,type,BindingTable[i].srcEP,address, dstEP);
      }
      for(j=0;j<gMAX_BINDING_CLUSTER_IDS;j++){
        
        AT_NEXT_LINE();
        if(j<BindingTable[i].numClusterIds){
          printf("%02X. | %-6s |  ????  |   %02X    |    %04X    | %-16s %s",
                 cnt,type,BindingTable[i].srcEP,BindingTable[i].clusterIdList[j],address, dstEP);
        }else{
          printf("%02X. | %-6s |  ????  |   %02X    |   unused   | %-16s %s",
                 cnt,type,BindingTable[i].srcEP,address, dstEP);
        }
      }
    }else{
      AT_NEXT_LINE();
      printf("%d. | Unused", cnt);
    }
    cnt++;
  }
  AT_RESP_END();
  AT_OK();
}


/*****************************************************************************
+BSET 每 Set local Binding Table Entry 
      AT+BSET:<type>,<LocalEP>,<ClusterID>,<DstAddress>[,<DstEP>]  Where: 
            <Type> is the type of binding: 
                    1= Unicast Binding with EUI64 and remote EP specified 
                    2= Many to one Binding with EUI64 and remote EP Specified 
                    3= Multicast Binding with Multicast ID Specified 
            <LocalEP> is the local endpoint 
            <ClusterID> is the t cluster ID, Address is either the EUI64 of the 
                    target device, or a multicast ID 
            <DstEP> the remote endpoint which is not specified in case of a multicast 
                    binding. 

            The new binding is created in the next available free binding table entry.
******************************************************************************/
void AT_Cmd_BSET(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[6];
  uint8 i;
  for(i=0;i<6;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":,,,,\r",cmdUnitArr); 
  
  uint8 type    = AT_ChartoInt8(&cmdUnitArr[0]);
  uint8 localEP = AT_ChartoInt8(&cmdUnitArr[1]);
  uint8 dstEP   = AT_ChartoInt8(&cmdUnitArr[4]);
  uint16 CID    = AT_ChartoInt16(&cmdUnitArr[2]);
  
  zAddrType_t dstAddr;
  

  if(type==1 || type==2) {
    /*if(cmdUnitArr[3].unitLen!=16) {
      AT_ERROR(AT_PARA_ERROR);
      return;
    }*/
    dstAddr.addrMode =Addr64Bit;
    AT_ChartoIntx(&cmdUnitArr[3],dstAddr.addr.extAddr, 64);
  }
  else if(type==3) {
    dstAddr.addrMode =AddrGroup;
    dstAddr.addr.shortAddr = AT_ChartoInt16(&cmdUnitArr[3]);
  }
  else {
    AT_ERROR(AT_PARA_ERROR);
    return;
  }
  
  if(NULL==bindAddEntry( localEP,&dstAddr,dstEP, 1,&CID )){
       AT_ERROR(AT_FATAL_ERROR);            
  }else{
       AT_OK();
       // up data the NwkAddr
       if ( dstAddr.addrMode == Addr64Bit ){
         uint16 nwkAddr=0;
         if ( APSME_LookupNwkAddr( dstAddr.addr.extAddr, &nwkAddr ) == FALSE ){
           ZDP_NwkAddrReq( dstAddr.addr.extAddr, ZDP_ADDR_REQTYPE_SINGLE, 0, 0 );
         }
       }
       BindWriteNV();
  }  
}


/*****************************************************************************
+BCLR 每 Clear local Binding Table Entry
      AT+BCLR:XX [,<ClusterID>]
          Where 
          XX is the entry number of the binding table which is to be cleared.
          <clsusterID> is the cluster ID to be removed. without this parameter,will
              the whole entry.

******************************************************************************/
void AT_Cmd_BCLR(uint8 start_point, uint8* msg){
   AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  uint8 XX    = AT_ChartoInt8(&cmdUnitArr[0]);
  if(cmdUnitArr[1].symbol ==','){
    uint16 CID = AT_ChartoInt16(&cmdUnitArr[1]);
    AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr);
    if(bindRemoveClusterIdFromList( BindingTable+XX,CID )){//true if at least 1 cluster ID is left in the list after the delete, 
                                                           //false if cluster list is empty. If the cluster list is empty itshould be deleted by the calling function.
      AT_OK();
    }else{
      if(bindRemoveEntry( BindingTable+XX )){
        AT_OK();
        BindWriteNV();
      }else{
        AT_ERROR(AT_FATAL_ERROR); 
      }
    }
  }else{
    AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr);
    if(bindRemoveEntry( BindingTable+XX )){
      AT_OK();
      BindWriteNV();
    }else{
      AT_ERROR(AT_FATAL_ERROR); 
    }
  }
}



/*****************************************************************************
+BTABLE 每 Display Binding Table (ZDO)
      AT+BTABLE:XX,<address> 
            Where 
            XX is the start index of the remote binding table and 
            <address> can be the remote node＊s EUI64, NodeID or address/binding table entry.

      AT+BTABLE:00,0000 
      SEQ:01 
      OK 
      BTable:0000,00 
      Length:03 
      No. | SrcAddr | SrcEP | ClusterID | DstAddr | DstEP 
      00. | 000D6F000059474E | 01 | DEAD |1234567887654321 | 12 
      01. | 000D6F000059474E | 01 | DEAD |E012345678876543 | E0 
      02. | 000D6F000059474E | 01 | DEAD | ABCD 
      ACK:01


******************************************************************************/
void AT_Cmd_BTABLE(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr); 

  zAddrType_t dstAddr;
  uint8 startIndex = AT_ChartoInt16(&cmdUnitArr[0]);
  if(cmdUnitArr[1].unitLen==16){               
    AT_ChartoIntx(&cmdUnitArr[1],dstAddr.addr.extAddr, 64);  
    dstAddr.addrMode = (afAddrMode_t)Addr64Bit;                  
  }else{                                                         
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[1]);            
    dstAddr.addrMode = (afAddrMode_t)Addr16Bit;       
  }
  uint8 state;
  state = ZDP_MgmtBindReq( &dstAddr, startIndex, 0 );
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
  
}

/*****************************************************************************
+BIND 每 Create Binding on Remote Device (ZDO) 
    AT+BIND:<address>,[<SrcAddress>],<SrcEP>,<ClusterID>,[<DstAddress>][,<DstEP>] 
          Create Binding on a remote device with 

          <address> the target Node＊s EUI64, Node ID, or Address Table entry 

          [<SrcAddress>] The EUI64 of the Source 

          <SrcEP> The source Endpoint <ClusterID> The Cluster ID on the source Device 

          <DstAddress> The EUI64 or 16-bit multicast ID

          <DstEP> Only in Mode 2: The destination endpoint



Prompt 
          Bind:<NodeID>,<status>

          In case of an error an status other than 00 will be displayed 
          <NodeID> is the Remote node＊s Node ID.


******************************************************************************/
void AT_Cmd_BIND(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[7];
  uint8 i;
  for(i=0;i<7;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  
  uint8 dstEP;
  uint8 srcEP   = AT_ChartoInt8(&cmdUnitArr[2]);
  uint16 CID    = AT_ChartoInt16(&cmdUnitArr[3]);
  zAddrType_t dstAddr;
  zAddrType_t addr;
  uint8 srcAddr[8];
  
  if(cmdUnitArr[5].symbol==','){
    AT_PARSE_CMD_PATTERN_ERROR(":,,,,,\r",cmdUnitArr); 
    dstEP   = AT_ChartoInt8(&cmdUnitArr[5]);
  }else{
    AT_PARSE_CMD_PATTERN_ERROR(":,,,,\r",cmdUnitArr);
    dstEP =0;
  }
  

  //<address>,<SrcAddress>,<SrcEP>,<ClusterID>,<DstAddress>[,<DstEP>]
  if(cmdUnitArr[1].unitLen==0){//if the user din't type this parameter, it will filled in by self address;
    osal_memcpy(srcAddr,NLME_GetExtAddr(),8);
  }else{
    AT_ChartoIntx(&cmdUnitArr[1],srcAddr, 64); 
  }
  
  if(cmdUnitArr[0].unitLen==0){
    addr.addr.shortAddr=NLME_GetShortAddr();              
    addr.addrMode = (afAddrMode_t)Addr16Bit; 
  }else if(cmdUnitArr[0].unitLen>10){               
    AT_ChartoIntx(&cmdUnitArr[0],addr.addr.extAddr, 64);  
    addr.addrMode = (afAddrMode_t)Addr64Bit;                  
  }else{                                                         
    addr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);            
    addr.addrMode = (afAddrMode_t)Addr16Bit;       
  }
  
  if(cmdUnitArr[4].unitLen==0){//if the user din't type this parameter, it will filled in by self address;
    osal_memcpy(dstAddr.addr.extAddr,NLME_GetExtAddr(),8);
    dstAddr.addrMode = (afAddrMode_t)Addr64Bit;
  }else if(cmdUnitArr[4].unitLen>10){ 
    AT_ChartoIntx(&cmdUnitArr[4],dstAddr.addr.extAddr, 64);  
    dstAddr.addrMode = (afAddrMode_t)Addr64Bit;                  
  }else{                                                         
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[4]);            
    dstAddr.addrMode = (afAddrMode_t)AddrGroup;       
  }
  uint8 state;
  state = ZDP_BindReq( &addr, srcAddr, srcEP, CID, &dstAddr, dstEP,0 );
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}
/*****************************************************************************
+UNBIND 每 Delete Binding on Remote Device 
    AT+UNBIND:<address>,[<SrcAddress>],<SrcEP>,<ClusterID>,[<DstAddress>][,<DstEP>] 
          Create Binding on a remote device with 

          <address> the target Node＊s EUI64, Node ID, or Address Table entry 

          [<SrcAddress>] The EUI64 of the Source 

          <SrcEP> The source Endpoint <ClusterID> The Cluster ID on the source Device 

          <DstAddress> The EUI64 or 16-bit multicast ID

          <DstEP> Only in Mode 2: The destination endpoint



Prompt 
          Unbind:<NodeID>,<status>

          In case of an error an status other than 00 will be displayed 
          <NodeID> is the Remote node＊s Node ID.


******************************************************************************/
void AT_Cmd_UNBIND(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[7];
  uint8 i;
  for(i=0;i<7;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  
  uint8 dstEP;
  uint8 srcEP   = AT_ChartoInt8(&cmdUnitArr[2]);
  uint16 CID    = AT_ChartoInt16(&cmdUnitArr[3]);
  zAddrType_t dstAddr;
  zAddrType_t addr;
  uint8 srcAddr[8];
  
  if(cmdUnitArr[5].symbol==','){
    AT_PARSE_CMD_PATTERN_ERROR(":,,,,,\r",cmdUnitArr); 
    dstEP   = AT_ChartoInt8(&cmdUnitArr[5]);
  }else{
    AT_PARSE_CMD_PATTERN_ERROR(":,,,,\r",cmdUnitArr);
    dstEP =0;
  }
  

  //<address>,<SrcAddress>,<SrcEP>,<ClusterID>,<DstAddress>[,<DstEP>]
  if(cmdUnitArr[1].unitLen==0){//if the user din't type this parameter, it will filled in by self address;
    osal_memcpy(srcAddr,NLME_GetExtAddr(),8);
  }else{
    AT_ChartoIntx(&cmdUnitArr[1],srcAddr, 64); 
  }
  
  if(cmdUnitArr[0].unitLen==0){//if the user din't type this parameter, it will filled in by self address;
    addr.addr.shortAddr=NLME_GetShortAddr();              
    addr.addrMode = (afAddrMode_t)Addr16Bit; 
  }else if(cmdUnitArr[0].unitLen>10){               
    AT_ChartoIntx(&cmdUnitArr[0],addr.addr.extAddr, 64);  
    addr.addrMode = (afAddrMode_t)Addr64Bit;                  
  }else{                                                         
    addr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);            
    addr.addrMode = (afAddrMode_t)Addr16Bit;       
  }
  
  if(cmdUnitArr[4].unitLen==0){//if the user din't type this parameter, it will filled in by self address;
    osal_memcpy(dstAddr.addr.extAddr,NLME_GetExtAddr(),8);
    dstAddr.addrMode = (afAddrMode_t)Addr64Bit;
  }else if(cmdUnitArr[4].unitLen>10){ 
    AT_ChartoIntx(&cmdUnitArr[4],dstAddr.addr.extAddr, 64);  
    dstAddr.addrMode = (afAddrMode_t)Addr64Bit;                  
  }else{                                                         
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[4]);            
    dstAddr.addrMode = (afAddrMode_t)AddrGroup;       
  }
  uint8 state;
  state = ZDP_UnbindReq( &addr, srcAddr, srcEP, CID, &dstAddr, dstEP,0 );
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}

/***********************************************************
+EBIND 每 End Device Bind
      AT+EBIND:<EP> 
      <EP> - Local Endpoint which will initiate end device binding.

Response
          BIND:<NodeID>,<Status> 
          OK
*************************************************************/

void AT_Cmd_EBIND(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[2];
  uint8 i;
  for(i=0;i<2;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr);
  
  uint8 ep = AT_ChartoInt8(&cmdUnitArr[0]);
   //build self address
  zAddrType_t addr={
    {0x0000},                                     //addr 
    (afAddrMode_t) Addr16Bit,                     //addr mode
  };
  
  endPointDesc_t * epDesc = afFindEndPointDesc( ep );
  if(epDesc){
    uint8 state;
    state = ZDP_EndDeviceBindReq( &addr,
                              NLME_GetShortAddr(),//struange but check the sorce code, this the only way. or it will return afStatus_INVALID_PARAMETER error
                              ep,
                              epDesc->simpleDesc->AppProfId,
                              epDesc->simpleDesc->AppNumInClusters,
                              epDesc->simpleDesc->pAppInClusterList,
                              epDesc->simpleDesc->AppNumOutClusters, 
                              epDesc->simpleDesc->pAppOutClusterList,
                              0 );
    if(state==SUCCESS){
      AT_OK();
    }else{
      AT_ERROR(state);
    }
  }else{
    AT_ERROR(AT_FATAL_ERROR);
  }
}
/*****************************************************
+CLEARBIND 每 Clear Local Binding Table
      AT+CLEARBIND

***********************************************************/

void AT_Cmd_CLEARBIND(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  uint8 i;
  for(i=0;i<1;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr);
  
  InitBindingTable( );
  BindWriteNV();
  AT_OK();
}

/*****************************************************
AT+READATR:<Address>,<EP>,<SendMode>,<ClusterID>,<AttrID>,＃< AttrID >
          <Address> - 16 bit hexadecimal number.
                       The node ID of a remote device if the command is 
                       sent directly to a node or a group ID if the command 
                       is sent to a group.

          <EP> - 8 bit hexadecimal number, endpoint of a remote device. 
                    Valid end point is 0x01 to 0xF0. 

          <SendMode> - A Boolean type to choose transmission mode, 
                    0 每 means sending command directly; 
                    1 每 means sending command to a group 

          <ClusterID> 16 bit hexadecimal number which represents the cluster ID

          <AttrID> - 16 bit hexadecimal number which represents the attribute ID
                      according to the ZigBee Home Automation specification.support
                      5 attributes per AT command

RESPATTR:<NodeID>,<EP>,<ClusterID>,<AttrID>,<Status>,<dataType>,<AttrInfo>
          <NodeID> - 16 bit hexadecimal number. It is the source Node ID 
                     of response. 

          <EP> - 8 bit hexadecimal number, the source endpoint of the response. 

          <ClusterID> - cluster ID, 16 bit hexadecimal number

          <AttrID>: attribute ID 16 bit hexadecimal number 

          <Status> - 8 bit hexadecimal number which indicates the result of
                      the requested operation. 

          <dataType> - 8 bit hexadecimal number which indicates the type of the data
          
          <AttrInfo> - hexadecimal number of char string (size depends on 
                      the attribute requested). <AttrInfo> shall only be 
                      valid if <Status> = 0x00. If <Status> indicates error, 
                      <AttrInfo> is not returned.

example:
AT+READATR:0,4,0,0,6,5,4

*******************************************************/
void AT_Cmd_READATR(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[10];
  uint8 i;
  uint8 parameterN=1;
  char pattern[11];
  for(i=0;i<10;i++){
    start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
    if(cmdUnitArr[i].symbol == ',') parameterN++;
  }
  
#if AT_CMD_PATTERN_CHECK
  //built check pattern
  pattern[0]=':';
  for(i=1;i<5;i++) pattern[i]=',';
  for(i=5;i<parameterN;i++) pattern[i]=',';
  pattern[i]='\r';
  pattern[i+1]='\0';
#endif
  
  //check pattern
  AT_PARSE_CMD_PATTERN_ERROR(pattern,cmdUnitArr);
  
  
  uint8 endpoint=AT_ChartoInt8(&cmdUnitArr[1]);
  uint8 sendmode=AT_ChartoInt8(&cmdUnitArr[2]);
  
  //build destination address
  afAddrType_t dstAddr;
  dstAddr.endPoint = endpoint;
  dstAddr.addrMode = sendmode==0 ? (afAddrMode_t)Addr16Bit : (afAddrMode_t) AddrGroup;
  if(cmdUnitArr[0].unitLen==0){
    dstAddr.addr.shortAddr=NLME_GetShortAddr();     
  }else{    
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);             
  }
  
  //build ZCL readCmd
  zclReadCmd_t* readCmd = (zclReadCmd_t*) osal_mem_alloc(sizeof(int)+2*(parameterN-4));  //sizeof(int) but not 1. to avoid memory alignment error
  readCmd->numAttr=parameterN-4;
  for(i=0;i<parameterN-4;i++)
      readCmd->attrID[i] = AT_ChartoInt16(&cmdUnitArr[4+i]);
  
  //send zcl read
  uint8 state;
  state = zcl_SendRead( AT_ZCL_ENDPOINT, &dstAddr,
                               AT_ChartoInt16(&cmdUnitArr[3]), readCmd,
                               ZCL_FRAME_CLIENT_SERVER_DIR, 0, 1);
  osal_mem_free(readCmd);
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
  
}
/****************************************************************************
AT+WRITEATR:<Address>,<EP>,<SendMode>,<ClusterID>,<AttrID>,<DataType>,<Data>
          <Address> - 16 bit hexadecimal number.
                       The node ID of a remote device if the command is 
                       sent directly to a node or a group ID if the command 
                       is sent to a group.

          <EP> - 8 bit hexadecimal number, endpoint of a remote device. 
                    Valid end point is 0x01 to 0xF0. 

          <SendMode> - A Boolean type to choose transmission mode, 
                    0 每 means sending command directly; 
                    1 每 means sending command to a group 

          <ClusterID> 16 bit hexadecimal number which represents attribute ID

          <AttrID> - 16 bit hexadecimal number which represents the attribute ID
                      according to the ZigBee Home Automation specification.
          <DataType> - 8 bit hexadecimal number that represents the type of the 
                      data accepted by this Attribute (please check HA specification)

          <AttrValue> - If attribute value has an integer type this field shall contain 
                        hexadecimal representation in big-endian format. If attribute 
                        value has a string type this field contains sequence of characters.

Response
    WRITEATTR:<NodeID>,<EP>,<Cluster>,<AttrID>,<Status> 
          <NodeID> - 16 bit hexadecimal number. It is the source Node ID of response.

          <EP> - 8 bit hexadecimal number, the source endpoint of the response. 

          <ClusterID> - cluster ID, 16 bit hexadecimal number, see section 2.2 

          <AttrID>: attribute ID 16 bit hexadecimal number 

          <Status> - 8 bit hexadecimal number which indicates the result of 
                    the requested operation.If < Status > is not 00, it will be an errorcode

******************************************************************************/
void AT_Cmd_WRITEATR(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[8];
  uint8 i;
  for(i=0;i<8;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  AT_PARSE_CMD_PATTERN_ERROR(":,,,,,,\r",cmdUnitArr);
  
  uint8 endpoint=AT_ChartoInt8(&cmdUnitArr[1]);
  uint8 sendmode=AT_ChartoInt8(&cmdUnitArr[2]);
  uint8 dataType=AT_ChartoInt8(&cmdUnitArr[5]);
  //build destination address
  afAddrType_t dstAddr;
  dstAddr.endPoint = endpoint;
  dstAddr.addrMode = sendmode==0 ? (afAddrMode_t)Addr16Bit : (afAddrMode_t) AddrGroup;  
  if(cmdUnitArr[0].unitLen==0){
    dstAddr.addr.shortAddr=NLME_GetShortAddr();     
  }else{    
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);             
  }
  
  //build ZCL writeCmd
  uint8 buf_writeCmd[sizeof(zclWriteRec_t)+sizeof(int)];
  zclWriteCmd_t* writeCmd = (zclWriteCmd_t*) buf_writeCmd;
  writeCmd->numAttr=1;
  writeCmd->attrList[0].attrID = AT_ChartoInt16(&cmdUnitArr[4]);
  writeCmd->attrList[0].dataType = dataType;
  
  if(dataType==ZCL_DATATYPE_CHAR_STR){
    cmdUnitArr[6].unit--;
    *(cmdUnitArr[6].unit)=cmdUnitArr[6].unitLen;
    writeCmd->attrList[0].attrData=cmdUnitArr[6].unit;
  }else if(dataType == ZCL_DATATYPE_DATA16||
           dataType == ZCL_DATATYPE_UINT16||
           dataType == ZCL_DATATYPE_INT16){
     *(uint16*)cmdUnitArr[6].unit = AT_ChartoInt16(&cmdUnitArr[6]);
     writeCmd->attrList[0].attrData=cmdUnitArr[6].unit; 
  }else{
     *(uint8*)cmdUnitArr[6].unit = AT_ChartoInt8(&cmdUnitArr[6]);
     writeCmd->attrList[0].attrData=cmdUnitArr[6].unit;
  }
  
  //send zcl write
  uint8 state;
  state = zcl_SendWrite( AT_ZCL_ENDPOINT, &dstAddr,
                               AT_ChartoInt16(&cmdUnitArr[3]), writeCmd,
                               ZCL_FRAME_CLIENT_SERVER_DIR, 0, 1);
  
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}
/******************************************************************************
+DISCOVER 每 Discover HA Devices On The HAN
    AT+DISCOVER:<Cluster ID>[,<option>] 
            <Cluster ID> - 16 bit hexadecimal cluster ID (please see section 2.2).
            The Five-In-One device can search for HA devices based on a specified cluster ID.
            <option> 1- display enabled device and disabled device
                     0- dispaly only enabled device

Prompt: 
    DEV:<NodeID>,<EndPoint>,<enable/disable>
    Carry out the ZigBee Service Discovery to 
    find ZigBee HA devices that support the given match criteria.
*****************************************************************************/

void AT_Cmd_DISCOVER(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  
  AT_AF_Cmd_HA_DISC_req_t buff;
  uint8 state; 
  buff.hdr.cmd = AT_AF_Cmd_req;
  buff.CID = AT_ChartoInt16(&cmdUnitArr[0]);
  
  if(cmdUnitArr[1].symbol == ','){
    AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr); 
    if(AT_ChartoInt8(&cmdUnitArr[1])) buff.option=1;
    else buff.option=0;
  }else{
    
    AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr); 
    buff.option=0;
  } 
  
 /* 
  //there is a problem that the end devices can't receive the group cast msg. 
  //the snifer indicate the broadcast address is 0xfffd which will not broadcast
  //to RxonIdle device. So some end device can't receive the msg. 
  //but the broad cast can do this. becsue the broad cast address is 0xffff 
  //which means to all device.
  //So, I will think about this after a period of time.
  
  //build group address  
  afAddrType_t AT_AF_group_addr={
    {AT_AF_GROUP_ID},                       //addr
    (afAddrMode_t)afAddrGroup,              //addr mode
    AT_AF_ENDPOINT,                         //end point
    NULL                                    //PAN ID
  };*/
  //build broadcast address
  afAddrType_t AT_AF_broad_addr={
    {0xffff},                       //addr
    (afAddrMode_t)AddrBroadcast,              //addr mode
    AT_AF_ENDPOINT,                         //end point
    NULL                                    //PAN ID
  };
  
  //build self address
  afAddrType_t AT_AF_self_addr={
    {NLME_GetShortAddr()},                       //addr
    (afAddrMode_t) Addr16Bit,              //addr mode
    AT_AF_ENDPOINT,                         //end point
    NULL                                    //PAN ID
  };
  
  
  //send data over air
  state = AF_DataRequest( &AT_AF_broad_addr, &AT_AF_epDesc,
                       AT_AF_Cmd_HA_DISC_CLUSTERID,
                       sizeof(buff),
                       (uint8*)&buff,
                       &AT_AF_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS );
  if(state!=afStatus_SUCCESS) {
    AT_ERROR(state);
    return;
  } 
  
  //send data to self
  state = AF_DataRequest( &AT_AF_self_addr, &AT_AF_epDesc,
                       AT_AF_Cmd_HA_DISC_CLUSTERID,
                       sizeof(buff),
                       (uint8*)&buff,
                       &AT_AF_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS );
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK(); 
}

/**************************************************************************
+CLUSDISC 每 Find All Supported Clusters On A Remote Device End Point
      AT+CLUSDISC:[<Node ID>],<EndPoint> 
            <Node ID> - 16 bit hexadecimal number. The network address of the target device. 
            <EndPoint> - 8 bit hexadecimal number. The end point of the target device.

DISCCLUS:<Node ID>,<EndPoint>,<Status>        //different with telegesis
Prompt (will show, if <Status> is 00) 
            SERVER:<ClusterID>,<ClusterID>,<ClusterID> 
            CLIENT:<ClusterID>,<ClusterID>,< ClusterI
*****************************************************************************/
void AT_Cmd_CLUSDISC(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[3];
  uint8 i;
  for(i=0;i<3;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  AT_PARSE_CMD_PATTERN_ERROR(":,\r",cmdUnitArr);
  
  uint8 endpoint=AT_ChartoInt8(&cmdUnitArr[1]);
  uint16 addr;
  
  if(cmdUnitArr[0].unitLen==0){
    addr=NLME_GetShortAddr();              
   // addr.addrMode = (afAddrMode_t)Addr16Bit; 
  }else{                                                         
    addr =AT_ChartoInt16(&cmdUnitArr[0]);            
    //addr.addrMode = (afAddrMode_t)Addr16Bit;       
  }
  
  AT_AF_hdr buff;
  buff.cmd=AT_AF_Cmd_req;
  buff.ep = endpoint;
  uint8 state;
  state =  AT_AF_Cmd_send_simple(addr,
                      AT_AF_Cmd_HA_CIDDISC_CLUSTERID,sizeof(buff),&buff);
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}

/**************************************************************************
+ATTRDISC - Find Supported Attributs On A Remote Device End Point Execute Command 
      AT+ATTRDISC:<NodeID>,<EP>,<ClusterID>,<AttributeID>,<MaxNumofAttr> 
            <NodeID> - 16 bit hexadecimal number represents target device＊s network address 
            <EP> - 8 bit hexadecimal number represents target device＊s end point 
            <ClusterID> - 16 bit hexadecimal number 
            <AttributeID> -16 bit hexadecimal number the (discover will start from this attribute) 
            <MaxNumofAttr> - 2 decimal number represent the number of attributes that required to be discovered, e.g: 01, 10

        DISCATTR:<Complete code> 
        CLUS:<ClusterID>,ATTR:<AttributeID>,TYPE:<DataType> 
        Note: <Complete code> represents if all attributes support by this cluster have been discovered. 
        00 每 Completed discovery 01 每 Uncompleted discovery
*****************************************************************************/
void AT_Cmd_ATTRDISC(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[6];
  uint8 i;
  for(i=0;i<6;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  AT_PARSE_CMD_PATTERN_ERROR(":,,,,\r",cmdUnitArr);
  
  uint8 endpoint=AT_ChartoInt8(&cmdUnitArr[1]);
  uint8 CID=AT_ChartoInt16(&cmdUnitArr[2]);
  
  //build destination address
  afAddrType_t dstAddr;
  dstAddr.endPoint = endpoint;
  dstAddr.addrMode = (afAddrMode_t)Addr16Bit;
  if(cmdUnitArr[0].unitLen==0){
    dstAddr.addr.shortAddr=NLME_GetShortAddr();     
  }else{    
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);             
  }
  
  zclDiscoverCmd_t discCmd;
  discCmd.startAttr =AT_ChartoInt8(&cmdUnitArr[3]);
  discCmd.maxAttrIDs =AT_ChartoInt8(&cmdUnitArr[4]);
  
  
  uint8 state;
  state = zcl_SendDiscoverCmd( AT_ZCL_ENDPOINT, &dstAddr,
                            CID, &discCmd,
                            ZCL_FRAME_CLIENT_SERVER_DIR,0, 0 );
  
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}


/**************************************************************************
AT+IDENTIFY:[<Address>],<EP>,<SendMode>,<Time>
            <Address> - 16 bit hexadecimal number. It shall be Node ID of a remote device 
                        if the command is sent directly to a node 
                         or it shall be a group ID if the command is sent to a group.

            <EP> - 8 bit hexadecimal number represent the Endpoint of the target

            <SendMode> - A Boolean type to choose transmission mode, 
                          0 每 means sending command directly; 
                          1 每 means sending command to a group 

            <Time> - 16 bit hexadecimal number represents the identification time
*******************************************************************************/
void AT_Cmd_IDENTIFY(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[5];
  uint8 i;
  for(i=0;i<5;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  AT_PARSE_CMD_PATTERN_ERROR(":,,,\r",cmdUnitArr);
  
  uint8 endpoint=AT_ChartoInt8(&cmdUnitArr[1]);
  uint8 sendmode=AT_ChartoInt8(&cmdUnitArr[2]);
  
  //build destination address
  afAddrType_t dstAddr;
  dstAddr.endPoint = endpoint;
  dstAddr.addrMode = sendmode==0 ? (afAddrMode_t)Addr16Bit : (afAddrMode_t) AddrGroup;
  if(cmdUnitArr[0].unitLen==0){
    dstAddr.addr.shortAddr=NLME_GetShortAddr();     
  }else{    
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);             
  }
  
  uint8 state;
  state = zclGeneral_SendIdentify( AT_ZCL_ENDPOINT,&dstAddr,AT_ChartoInt16(&cmdUnitArr[3]),0, 0 );
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
  
}

/**************************************************************************
+IDQUERY 每 Query If Target Device(s) In Identifying Mode 
      AT+IDQUERY:<Address>,<EP>,<SendMode> 
            <Address> - 16 bit hexadecimal number.It shall be Node ID of 
                        a remote device if the command is sent directly to a 
                        node or it shall be a group ID if the command is sent to a group. 
            <EP> - 8 bit hexadecimal number represent the Endpoint of the target 
            <SendMode> - A Boolean type to choose transmission mode, 
                          0 每 means sending command directly; 
                          1 每 means sending command to a group


Response 
      OK 
      IDQUERYRSP:<NodeId>,<EP>,<TimeOut> 
      or ERROR<errorcode> 
            <NodeID> - 16 bit hexadecimal number, source of the response. It should 
                       be the same with the target＊s node ID. 
            <EP> - 8 bit hexadecimal number represent the Endpoint of the target
            <TimeOut> - 16 bit hexadecimal number, represents the length of time, 
                        in seconds, that the device will continue to identify itself.
*****************************************************************************/
void AT_Cmd_IDQUERY(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[4];
  uint8 i;
  for(i=0;i<4;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg); 
  AT_PARSE_CMD_PATTERN_ERROR(":,,\r",cmdUnitArr);
  
  uint8 endpoint=AT_ChartoInt8(&cmdUnitArr[1]);
  uint8 sendmode=AT_ChartoInt8(&cmdUnitArr[2]);
  
  //build destination address
  afAddrType_t dstAddr;
  dstAddr.endPoint = endpoint;
  dstAddr.addrMode = sendmode==0 ? (afAddrMode_t)Addr16Bit : (afAddrMode_t) AddrGroup;
  if(cmdUnitArr[0].unitLen==0){
    dstAddr.addr.shortAddr=NLME_GetShortAddr();     
  }else{    
    dstAddr.addr.shortAddr =AT_ChartoInt16(&cmdUnitArr[0]);             
  }
  uint8 state;
  state = zclGeneral_SendIdentifyQuery( AT_ZCL_ENDPOINT,&dstAddr,0, 0 );
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
}

/*******************************************************
Disable the AT commands
      AT+DISABLE

*********************************************************/

#if AT_ENABLE_PASSWORDS_MODE
void AT_Cmd_DISABLE(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  uint8 i;
  for(i=0;i<1;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
  
  AT_enable=0; 
  AT_OK();
}
#endif


/*******************************************************
enable the AT commands
      AT+ENABLE:<password>
*********************************************************/
#if AT_ENABLE_PASSWORDS_MODE
void AT_Cmd_ENABLE(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[2];
  uint8 i;
  for(i=0;i<2;i++)start_point = AT_get_next_cmdUnit(&cmdUnitArr[i],start_point, msg);  
  AT_PARSE_CMD_PATTERN_ERROR(":\r",cmdUnitArr); 
  
  AT_DEBUG("\n\r", 2);
  AT_DEBUG("AT command has already been enabled",sizeof("AT command has already been enabled")-1);
  if(AT_CmpCmd(cmdUnitArr,(uint8 *)AT_passwords)==0){
    //AT_enable=1; 
    AT_OK();
  }else{
    AT_ERROR(AT_WRONG_PASSWORD);
  }
}
#endif

/**************************************************************************
AT+HELP
           display the available commands
*******************************************************************************/
void AT_Cmd_HELP(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
  
  uint8 i;
  AT_RESP_START();
  AT_RESP("ATI",3);
  for(i=0;i<AT_CMD_HELP_DESC_OFFSET;i++) AT_RESP(".",1);
  AT_RESP("Display Product Identification Information",sizeof("Display Product Identification Information")-1);
  AT_NEXT_LINE();
  
  AT_RESP("ATZ",3);
  for(i=0;i<AT_CMD_HELP_DESC_OFFSET;i++) AT_RESP(".",1);
  AT_RESP("Software Reset",sizeof("Software Reset")-1);
  AT_NEXT_LINE();
  
  AT_RESP("AT&F",4);
  for(i=0;i<AT_CMD_HELP_DESC_OFFSET-1;i++) AT_RESP(".",1);
  AT_RESP("Restore Local Device's Factory Defaults",sizeof("Restore Local Device's Factory Defaults")-1);
  AT_NEXT_LINE();
  
  AT_RESP("ATS",3);
  for(i=0;i<AT_CMD_HELP_DESC_OFFSET;i++) AT_RESP(".",1);
  AT_RESP("S-Register Access ATSXX? / ATSXX=<data>",sizeof("S-Register Access ATSXX? / ATSXX=<data>")-1);
  
  for(i=0;i<sizeof(AT_Cmd_Arr)/sizeof(AT_Cmd_Arr[0]) - AT_CMD_HELP_DESC_OMIT;i++){
    uint8 j;
    AT_NEXT_LINE();
    AT_RESP("AT+",3);
    AT_RESP(AT_Cmd_Arr[i].AT_Cmd_str,AT_strLen(AT_Cmd_Arr[i].AT_Cmd_str));
    for(j=0;j<AT_CMD_HELP_DESC_OFFSET-AT_strLen(AT_Cmd_Arr[i].AT_Cmd_str);j++) AT_RESP(".",1);
    printf(AT_Cmd_Arr[i].description_str);
  }
  AT_RESP_END();
  AT_OK(); 
  
}

void AT_Cmd_TEST(uint8 start_point, uint8* msg){
  AT_CmdUnit cmdUnitArr[1];
  start_point = AT_get_next_cmdUnit(&cmdUnitArr[0],start_point, msg);
  AT_PARSE_CMD_PATTERN_ERROR("\r",cmdUnitArr); 
  
  
  afAddrType_t dstAddr;
  dstAddr.endPoint = 4;
  dstAddr.addr.shortAddr =0;
  //dstAddr.panId =2016;//0;
  dstAddr.addrMode = (afAddrMode_t)Addr16Bit;  
  
  zclReadCmd_t* readCmd = (zclReadCmd_t*) osal_mem_alloc(1+2*3 );
  readCmd->numAttr=3;
  readCmd->attrID[0] = ATTRID_BASIC_MANUFACTURER_NAME;
  readCmd->attrID[1] = ATTRID_BASIC_MODEL_ID;
  readCmd->attrID[2] = ATTRID_BASIC_DATE_CODE;
 
  uint8 state;
  state = zcl_SendRead( 4, &dstAddr,
                                ZCL_CLUSTER_ID_GEN_BASIC, readCmd,
                               ZCL_FRAME_CLIENT_SERVER_DIR, 0, 1);
  osal_mem_free(readCmd);
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  /*
  uint16 buf[]={ATTRID_DEV_TEMP_CURRENT};
  uint8 state;
  state = zcl_SendCommand( 4, &dstAddr, ZCL_CLUSTER_ID_GEN_DEVICE_TEMP_CONFIG, ZCL_CMD_READ, FALSE,
                              ZCL_FRAME_CLIENT_SERVER_DIR,0, 0, 0, 2, (uint8*) buf );
  if(state!=afStatus_SUCCESS) AT_ERROR(state);
  else AT_OK();
  */
  
/* 
  AT_RESP_START();
  char str[20];
  AT_Int8toChar(_NIB.nwkState,str);
  AT_RESP(str,2);
  AT_RESP_END(); 
  AT_OK();
  
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