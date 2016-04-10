
/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ms.h"
#include "zcl_ha.h"
#include "AT_ZCL.h"
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
 * Local VARIABLES
 */
static uint16 UPDATE_timer = AT_ZCL_TEMP_UPDATE_TIMEOUT_VALUE;  //update the temperature after xxx seconds of the device starting
/*********************************************************************
 * LOCAL FUNCTION
 */
static void AT_ZCL_TEMP_IdentifyCB( zclIdentify_t *pCmd );
static void AT_ZCL_TEMP_BasicResetCB( void );
static void AT_ZCL_TEMP_ProcessIdentifyTimeChange( void );


//ninglvfeihong Modified for TEMP
void AT_ZCL_TEMP_OnOffCB( uint8 cmd );
static void AT_ZCL_TEMP_update(void);
static void AT_ZCL_TEMP_MS_PlaceHolder( void );
static void AT_ZCL_TEMP_EP_ENABLE( bool isEnable);

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t AT_ZCL_TEMP_GEN_CmdCallbacks =
{ 
  AT_ZCL_TEMP_BasicResetCB,                 // Basic Cluster Reset command
  AT_ZCL_TEMP_IdentifyCB,                   // Identify command   
  NULL,                                    // Identify Query Response command
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
/*********************************************************************
 * ZCL Measurement and Sensing Profile Callback table
 */
static zclMS_AppCallbacks_t AT_ZCL_TEMP_MS_CmdCallbacks=
{
  AT_ZCL_TEMP_MS_PlaceHolder,                                     //// Place Holder
};


void AT_ZCL_TEMP_Init( byte task_id )
{
  AT_ZCL_TEMP_TaskID = task_id;

  // This app is part of the Home Automation Profile
  zclHA_Init( &AT_ZCL_TEMP_SimpleDesc );

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( AT_ZCL_TEMP_ENDPOINT, &AT_ZCL_TEMP_GEN_CmdCallbacks );
  
  // Register the ZCL Measurement and Sensing Cluster Library callback functions
  zclMS_RegisterCmdCallbacks( AT_ZCL_TEMP_ENDPOINT, &AT_ZCL_TEMP_MS_CmdCallbacks );

  //register for AT command system enable/disable call back function
  AT_ZCL_EP_ENABLE_Register(  AT_ZCL_TEMP_ENDPOINT,AT_ZCL_TEMP_EP_ENABLE);
  
  // Register the application's attribute list
  zcl_registerAttrList( AT_ZCL_TEMP_ENDPOINT, AT_ZCL_TEMP_MAX_ATTRIBUTES, AT_ZCL_TEMP_Attrs );
  
  if(AT_ZCL_TEMP_UPDATE_TIMEOUT_VALUE>0 )
    osal_start_reload_timer( AT_ZCL_TEMP_TaskID,AT_ZCL_TEMP_UPDATE_TIMEOUT_EVT, 1000 ); //reload timer 1 second
  
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
  //deal with the update timer event
  if ( events & AT_ZCL_TEMP_UPDATE_TIMEOUT_EVT )
  {
    UPDATE_timer--;
    if(UPDATE_timer==0){
      UPDATE_timer= AT_ZCL_TEMP_UPDATE_TIMEOUT_VALUE;
      AT_AF_send_update(AT_ZCL_TEMP_ENDPOINT, AT_ZCL_TEMP_current,0); //time up, so send update
    }
    return ( events ^ AT_ZCL_TEMP_UPDATE_TIMEOUT_EVT );
  }

  // Discard unknown events
  return 0;
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
  AT_ZCL_TEMP_OnOffCB(COMMAND_OFF);
  
  zclIdentify_t identifyCmd={NULL,0};
  AT_ZCL_TEMP_IdentifyCB( &identifyCmd );
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
    HalLedBlink ( HAL_LED_1, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    if ( AT_ZCL_TEMP_OnOff )
      HalLedSet ( HAL_LED_1, HAL_LED_MODE_ON );
    else
      HalLedSet ( HAL_LED_1, HAL_LED_MODE_OFF );
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
  static uint16 TEMP_previous =0;
  uint16 temp = ReadDs18B20();
  AT_ZCL_TEMP_current =(int16)((*(int16*) & temp)*6.25);
  //update the temperature information to COOR when temperature variation is more than 0.2 C
  if((TEMP_previous>AT_ZCL_TEMP_current ? TEMP_previous-AT_ZCL_TEMP_current:AT_ZCL_TEMP_current-TEMP_previous)>20){
    //if temperature variation greater than 0.2 C, then update the temperature and reset the UPDATE_timer
    TEMP_previous=AT_ZCL_TEMP_current;
    UPDATE_timer=AT_ZCL_TEMP_UPDATE_TIMEOUT_VALUE;              //reset UPDATE_timer
    AT_AF_send_update(AT_ZCL_TEMP_ENDPOINT, AT_ZCL_TEMP_current,0); //update
  }
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
//ninglvfeihong Modified for Temperature end point
void AT_ZCL_TEMP_OnOffCB( uint8 cmd )
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

  // In this sample app, we use LED3 to simulate the Light
  if (AT_ZCL_TEMP_OnOff == TEMP_ON ){
    HalLedSet( HAL_LED_1, HAL_LED_MODE_ON );
    osal_start_timerEx( AT_ZCL_TEMP_TaskID, AT_ZCL_TEMP_TEMP_MEASURE_EVT, 10 );
  }else{
    HalLedSet( HAL_LED_1, HAL_LED_MODE_OFF );
    osal_stop_timerEx( AT_ZCL_TEMP_TaskID, AT_ZCL_TEMP_TEMP_MEASURE_EVT );
    AT_ZCL_TEMP_current=0x8000;   //0x8000 indicates that the temperature measurement is invalid.
  }
}

/******************************************************
 * @fn      AT_ZCL_TEMP_MS_PlaceHolder
 *
 * @brief   Process  Measurement and Sensing profile call Callback function
********************************************************/

static void AT_ZCL_TEMP_MS_PlaceHolder( void){
}


/******************************************************
 * @fn      AT_ZCL_TEMP_EP_ENABLE
 *
 * @brief   Process  Measurement and Sensing profile call Callback function
********************************************************/
static void AT_ZCL_TEMP_EP_ENABLE( bool isEnable){
  if(isEnable) {
    AT_ZCL_TEMP_OnOffCB(COMMAND_ON);
  }
  else AT_ZCL_TEMP_BasicResetCB( );
}

