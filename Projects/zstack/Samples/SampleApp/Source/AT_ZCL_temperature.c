
/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "AT_uart.h"
#include "AT_ZCL_temperature.h"
#include "AT_ds18b20.h" 

#include "onboard.h"

/* HAL */
#include "hal_led.h"
#include "hal_key.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte AT_ZCL_TEMP_TaskID;

/*********************************************************************
 * LOCAL FUNCTION
 */
static void AT_ZCL_TEMP_IdentifyCB( zclIdentify_t *pCmd );
static void AT_ZCL_TEMP_BasicResetCB( void );
static void AT_ZCL_TEMP_ProcessIdentifyTimeChange( void );
static void AT_ZCL_TEMP_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg);
static void AT_ZCL_TEMP_OnOffCB( uint8 cmd );
static uint8 AT_ZCL_TEMP_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
static void AT_ZCL_TEMP_update(void);

static uint8 AT_ZCL_TEMP_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t AT_ZCL_TEMP_CmdCallbacks =
{ 
  AT_ZCL_TEMP_BasicResetCB,                 // Basic Cluster Reset command
  AT_ZCL_TEMP_IdentifyCB,                   // Identify command   
  NULL,                                     // Identify Query Response command
  AT_ZCL_TEMP_OnOffCB,                      // On/Off cluster command
  NULL,                                     // Level Control Move to Level command
  NULL,                                     // Level Control Move command
  NULL,                                     // Level Control Step command
  NULL,                                     // Group Response commands
  NULL,                                     // Scene Store Request command
  NULL,                                     // Scene Recall Request command
  NULL,                                     // Scene Response command
  NULL,                                     // Alarm (Response) command
  NULL,                                     // RSSI Location commands
  NULL,                                     // RSSI Location Response commands
};

void AT_ZCL_TEMP_Init( byte task_id )
{
  AT_ZCL_TEMP_TaskID = task_id;

  // This app is part of the Home Automation Profile
  zclHA_Init( &AT_ZCL_TEMP_SimpleDesc );

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( AT_ZCL_TEMP_ENDPOINT, &AT_ZCL_TEMP_CmdCallbacks );

  // Register the application's attribute list
  zcl_registerAttrList( AT_ZCL_TEMP_ENDPOINT, AT_ZCL_TEMP_MAX_ATTRIBUTES, AT_ZCL_TEMP_Attrs );

  // Register the Application to receive the unprocessed Foundation command/response messages
  zcl_registerForMsg( AT_ZCL_TEMP_TaskID );

  
  osal_start_timerEx( AT_ZCL_TEMP_TaskID, AT_ZCL_TEMP_TEMP_MEASURE_EVT, 1000 );
  
  
}



/*********************************************************************
 * @fn          zclSample_event_loop
 *
 * @brief       Event Loop Processor for zclGeneral.
 *
 * @param       none
 *
 * @return      none
 */
uint16 AT_ZCL_TEMP_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( AT_ZCL_TEMP_TaskID )) )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZCL_INCOMING_MSG:
          // Incoming ZCL Foundation command/response messages
          AT_ZCL_TEMP_ProcessIncomingMsg( (zclIncomingMsg_t *)MSGpkt );
          break;
          
         default:
          break;
      }
      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & AT_ZCL_TEMP_IDENTIFY_TIMEOUT_EVT )
  {
    if (AT_ZCL_TEMP_IdentifyTime > 0 )
      AT_ZCL_TEMP_IdentifyTime--;
    AT_ZCL_TEMP_ProcessIdentifyTimeChange();

    return ( events ^ AT_ZCL_TEMP_IDENTIFY_TIMEOUT_EVT );
  }
  //measure the temperature
  if ( events & AT_ZCL_TEMP_TEMP_MEASURE_EVT )
  {
    AT_ZCL_TEMP_update();
    osal_start_timerEx( AT_ZCL_TEMP_TaskID, AT_ZCL_TEMP_TEMP_MEASURE_EVT, 1000 );
    return ( events ^ AT_ZCL_TEMP_TEMP_MEASURE_EVT );
  }

  // Discard unknown events
  return 0;
}




/*********************************************************************
 * @fn      AT_ZCL_TEMP_ProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  none
 */
static void AT_ZCL_TEMP_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg)
{
  switch ( pInMsg->zclHdr.commandID )
  {

#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
      AT_ZCL_TEMP_ProcessInReadRspCmd( pInMsg );
      break;
#endif
    case ZCL_CMD_DEFAULT_RSP:
      AT_ZCL_TEMP_ProcessInDefaultRspCmd( pInMsg );
      break;
      
    default:
      break;
  }
  
  if ( pInMsg->attrCmd )
    osal_mem_free( pInMsg->attrCmd );
}






/*********************************************************************
 * @fn      AT_ZCL_TEMP_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to default values.
 *
 * @param   none
 *
 * @return  none
 */
static void AT_ZCL_TEMP_BasicResetCB( void )
{
  // Reset all attributes to default values
}


/*********************************************************************
 * @fn      AT_ZCL_TEMP_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   srcAddr - source address and endpoint of the response message
 * @param   identifyTime - the number of seconds to identify yourself
 *
 * @return  none
 */
static void AT_ZCL_TEMP_IdentifyCB( zclIdentify_t *pCmd )
{
  AT_ZCL_TEMP_IdentifyTime = pCmd->identifyTime;
  AT_ZCL_TEMP_ProcessIdentifyTimeChange();
}

/*********************************************************************
 * @fn      AT_ZCL_TEMP_ProcessIdentifyTimeChange
 *
 * @brief   Called to process any change to the IdentifyTime attribute.
 *
 * @param   none
 *
 * @return  none
 */
static void AT_ZCL_TEMP_ProcessIdentifyTimeChange( void )
{
  if ( AT_ZCL_TEMP_IdentifyTime > 0 )
  {
    osal_start_timerEx( AT_ZCL_TEMP_TaskID, AT_ZCL_TEMP_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_2, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    if ( AT_ZCL_TEMP_OnOff )
      HalLedSet ( HAL_LED_2, HAL_LED_MODE_ON );
    else
      HalLedSet ( HAL_LED_2, HAL_LED_MODE_OFF );
    osal_stop_timerEx( AT_ZCL_TEMP_TaskID, AT_ZCL_TEMP_IDENTIFY_TIMEOUT_EVT );
  }
}


/*********************************************************************
 * @fn      void AT_ZCL_TEMP_update
 *
 * @brief   updata the Device Temperature Configuration Cluster every second
 *
 * @param   none
 *
 * @return  none
 *
************************************************************/
static void AT_ZCL_TEMP_update(void){
  
  AT_ZCL_TEMP_current=ReadDs18B20();
  
}



/*********************************************************************
 * @fn      AT_ZCL_TEMP_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 AT_ZCL_TEMP_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;
   
  // Device is notified of the Default Response command.
  (void)pInMsg;
  
  return TRUE; 
}

/*********************************************************************
 * @fn      AT_ZCL_TEMP_OnOffCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an On/Off Command for this application.
 *
 * @param   cmd - COMMAND_ON, COMMAND_OFF or COMMAND_TOGGLE
 *
 * @return  none
 */
static void AT_ZCL_TEMP_OnOffCB( uint8 cmd )
{
  // Turn on the light
  if ( cmd == COMMAND_ON )
    AT_ZCL_TEMP_OnOff = TEMP_ON;

  // Turn off the light
  else if ( cmd == COMMAND_OFF )
    AT_ZCL_TEMP_OnOff = TEMP_OFF;

  // Toggle the light
  else
  {
    if ( AT_ZCL_TEMP_OnOff == TEMP_OFF )
      AT_ZCL_TEMP_OnOff = TEMP_ON;
    else
      AT_ZCL_TEMP_OnOff = TEMP_OFF;
  }

  // In this sample app, we use LED4 to simulate the Light
  if ( AT_ZCL_TEMP_OnOff == TEMP_ON )
    HalLedSet( HAL_LED_2, HAL_LED_MODE_ON );
  else
    HalLedSet( HAL_LED_2, HAL_LED_MODE_OFF );
}


#ifdef ZCL_READ
/*********************************************************************
 * @fn      AT_ZCL_TEMP_ProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 AT_ZCL_TEMP_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;

  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  
  char str[17];
  AT_RESP_START();
  for (i = 0; i < readRspCmd->numAttr; i++)
  {
        AT_RESP("RESPATR:",9);
        //node id
        if(pInMsg->srcAddr.addrMode==(afAddrMode_t)Addr16Bit){
          AT_Int16toChar(pInMsg->srcAddr.addr.shortAddr,str);
          AT_RESP(str,4);
        }else{
          AT_EUI64toChar(pInMsg->srcAddr.addr.extAddr,str);
          AT_RESP(str,16);
        }
        AT_RESP(",",1);
        
        //End ponit
        AT_Int8toChar(pInMsg->srcAddr.endPoint,str);
        AT_RESP(str,2);
        AT_RESP(",",1);
        
        //Cluster ID
        AT_Int16toChar(pInMsg->clusterId,str);
        AT_RESP(str,4);
        AT_RESP(",",1);
        
        //Attribute ID
        AT_Int16toChar(readRspCmd->attrList[i].attrID,str);
        AT_RESP(str,4);
        AT_RESP(",",1);
        
        //status
        AT_Int8toChar(readRspCmd->attrList[i].status,str);
        AT_RESP(str,2);
        AT_RESP(",",1);

        //data type
        AT_Int8toChar(readRspCmd->attrList[i].dataType,str);
        AT_RESP(str,2);
        AT_RESP(",",1);
        
        //ATTR VALUE
        if(readRspCmd->attrList[i].dataType == ZCL_DATATYPE_CHAR_STR){
          AT_RESP(readRspCmd->attrList[i].data +1,readRspCmd->attrList[i].data[0]);        
        }else if(readRspCmd->attrList[i].dataType == ZCL_DATATYPE_DATA16||
                 readRspCmd->attrList[i].dataType == ZCL_DATATYPE_UINT16||
                 readRspCmd->attrList[i].dataType == ZCL_DATATYPE_INT16){
          AT_Int16toChar(*((uint16*)readRspCmd->attrList[i].data),str);
          AT_RESP(str,4);
        }else{
          
          AT_Int8toChar((uint8)readRspCmd->attrList[i].data[0],str);
          AT_RESP(str,2);
        }
        
        if(i<readRspCmd->numAttr-1) AT_NEXT_LINE();
  }
  AT_RESP_END(); 
  AT_OK();

  return TRUE; 
}
#endif // ZCL_READ



