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
#include "AT_ZCL_ONOFFL.h"
#include "AT_relayL.h" 

#include "onboard.h"

/* HAL */
#include "hal_led.h"
#include "hal_key.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte AT_ZCL_ONOFFL_TaskID;

/*********************************************************************
 * Local VARIABLES
 */
static uint16 UPDATE_timer = AT_ZCL_ONOFF_UPDATE_TIMEOUT_VALUE;  //update the sensor status after xxx seconds of the device starting
/*********************************************************************
 * LOCAL FUNCTION
 */
static void AT_ZCL_ONOFFL_IdentifyCB( zclIdentify_t *pCmd );
static void AT_ZCL_ONOFFL_BasicResetCB( void );
static void AT_ZCL_ONOFFL_ProcessIdentifyTimeChange( void );

//ninglvfeihong Modified for Socket
void AT_ZCL_ONOFFL_OnOffCB( uint8 cmd );
static void AT_ZCL_ONOFFL_EP_ENABLE( bool isEnable);

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t AT_ZCL_ONOFFL_GEN_CmdCallbacks =
{ 
  AT_ZCL_ONOFFL_BasicResetCB,                 // Basic Cluster Reset command
  AT_ZCL_ONOFFL_IdentifyCB,                   // Identify command   
  NULL,                                      // Identify Query Response command
  AT_ZCL_ONOFFL_OnOffCB,                      // On/Off cluster command
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



void AT_ZCL_ONOFFL_Init( byte task_id )
{
  AT_ZCL_ONOFFL_TaskID = task_id;

  // This app is part of the Home Automation Profile
  zclHA_Init( &AT_ZCL_ONOFFL_SimpleDesc );

  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( AT_ZCL_ONOFFL_ENDPOINT, &AT_ZCL_ONOFFL_GEN_CmdCallbacks );
  
  // Register the application's attribute list
  zcl_registerAttrList( AT_ZCL_ONOFFL_ENDPOINT, AT_ZCL_ONOFFL_MAX_ATTRIBUTES, AT_ZCL_ONOFFL_Attrs );
  
  //register for AT command system enable/disable call back function
  AT_ZCL_EP_ENABLE_Register(  AT_ZCL_ONOFFL_ENDPOINT,AT_ZCL_ONOFFL_EP_ENABLE);
  
  //initialize the ONOFF device such as: a relay
  relayL_init();
  
  
  //initialize update-to-COOR timer
  if(AT_ZCL_ONOFF_UPDATE_TIMEOUT_VALUE>0 )
    osal_start_reload_timer( AT_ZCL_ONOFFL_TaskID,AT_ZCL_ONOFF_UPDATE_TIMEOUT_EVT, 1000 ); //reload timer 1 second
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
uint16 AT_ZCL_ONOFFL_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( AT_ZCL_ONOFFL_TaskID )) )
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

  if ( events & AT_ZCL_ONOFFL_IDENTIFY_TIMEOUT_EVT )
  {
    if (AT_ZCL_ONOFFL_IdentifyTime > 0 )
      AT_ZCL_ONOFFL_IdentifyTime--;
    AT_ZCL_ONOFFL_ProcessIdentifyTimeChange();

    return ( events ^ AT_ZCL_ONOFFL_IDENTIFY_TIMEOUT_EVT );
  }
  //deal with the update timer event
  if ( events & AT_ZCL_ONOFF_UPDATE_TIMEOUT_EVT )
  {
    UPDATE_timer--;
    if(UPDATE_timer==0){
      UPDATE_timer= AT_ZCL_ONOFF_UPDATE_TIMEOUT_VALUE;
      //send update of switch status
      /** On/Off Cluster Attributes ***
      {
        ZCL_CLUSTER_ID_GEN_ON_OFF,
        { // Attribute record
          ATTRID_ON_OFF,
          ZCL_DATATYPE_UINT8,
          ACCESS_CONTROL_READ,
          (void *)&AT_ZCL_ONOFF_OnOff
        }
      }, */
      //afStatus_t AT_AF_send_update(uint8 ep,uint16 clusterId,uint16 attrID,uint8 dataType, uint8* data,uint8 status); //status == 0, indicate succeed
      AT_AF_send_update(AT_ZCL_ONOFFL_ENDPOINT,ZCL_CLUSTER_ID_GEN_ON_OFF,
                         ATTRID_ON_OFF,ZCL_DATATYPE_ENUM8,
                         (uint8*) &AT_ZCL_ONOFFL_OnOff,0);//time up, so send update
    }
    return ( events ^ AT_ZCL_ONOFF_UPDATE_TIMEOUT_EVT );
  }
  
  // Discard unknown events
  return 0;
}









/*********************************************************************
 * @fn      AT_ZCL_ONOFFL_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to default values.
 *
 * @param   none
 *
 * @return  none
 */
static void AT_ZCL_ONOFFL_BasicResetCB( void )
{
  // Reset all attributes to default values
  AT_ZCL_ONOFFL_OnOffCB( COMMAND_OFF );
  
  
  zclIdentify_t identifyCmd={NULL,0};
  AT_ZCL_ONOFFL_IdentifyCB( &identifyCmd );
}


/*********************************************************************
 * @fn      AT_ZCL_ONOFFL_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   srcAddr - source address and endpoint of the response message
 * @param   identifyTime - the number of seconds to identify yourself
 *
 * @return  none
 */
static void AT_ZCL_ONOFFL_IdentifyCB( zclIdentify_t *pCmd )
{
  AT_ZCL_ONOFFL_IdentifyTime = pCmd->identifyTime;
  AT_ZCL_ONOFFL_ProcessIdentifyTimeChange();
}

/*********************************************************************
 * @fn      AT_ZCL_ONOFFL_ProcessIdentifyTimeChange
 *
 * @brief   Called to process any change to the IdentifyTime attribute.
 *
 * @param   none
 *
 * @return  none
 */
static void AT_ZCL_ONOFFL_ProcessIdentifyTimeChange( void )
{
  if ( AT_ZCL_ONOFFL_IdentifyTime > 0 )
  {
    osal_start_timerEx( AT_ZCL_ONOFFL_TaskID, AT_ZCL_ONOFFL_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LEDL, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    if ( AT_ZCL_ONOFFL_OnOff )
      HalLedSet ( HAL_LEDL, HAL_LED_MODE_ON );
    else
      HalLedSet ( HAL_LEDL, HAL_LED_MODE_OFF );
    osal_stop_timerEx( AT_ZCL_ONOFFL_TaskID, AT_ZCL_ONOFFL_IDENTIFY_TIMEOUT_EVT );
  }
}



/*********************************************************************
 * @fn      AT_ZCL_ONOFFL_OnOffCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an On/Off Command for this application.
 *
 * @param   cmd - COMMAND_ON, COMMAND_OFF or COMMAND_TOGGLE
 *
 * @return  none
 */

//ninglvfeihong Modified for Socket
void AT_ZCL_ONOFFL_OnOffCB( uint8 cmd )
{
  // Turn on the light
  if ( cmd == COMMAND_ON )
    AT_ZCL_ONOFFL_OnOff = AT_ZCL_GEN_ON;

  // Turn off the light
  else if ( cmd == COMMAND_OFF )
    AT_ZCL_ONOFFL_OnOff = AT_ZCL_GEN_OFF;

  // Toggle the light
  else
  {
    if ( AT_ZCL_ONOFFL_OnOff == AT_ZCL_GEN_OFF )
      AT_ZCL_ONOFFL_OnOff = AT_ZCL_GEN_ON;
    else
      AT_ZCL_ONOFFL_OnOff = AT_ZCL_GEN_OFF;
  }

  // In this sample app, we use LED4 to simulate the Light
  if ( AT_ZCL_ONOFFL_OnOff == AT_ZCL_GEN_ON ){
    HalLedSet( HAL_LEDL, HAL_LED_MODE_ON );
    relayL_on();
  }
  else{
    HalLedSet( HAL_LEDL, HAL_LED_MODE_OFF );
    relayL_off();
  }
  
      //sound the speaker
      HalLedBlink( HAL_LED_3, 1,30,50  ); 
      
      //send update of switch status
      /** On/Off Cluster Attributes ***
      {
        ZCL_CLUSTER_ID_GEN_ON_OFF,
        { // Attribute record
          ATTRID_ON_OFF,
          ZCL_DATATYPE_UINT8,
          ACCESS_CONTROL_READ,
          (void *)&AT_ZCL_ONOFF_OnOff
        }
      }, */
      //afStatus_t AT_AF_send_update(uint8 ep,uint16 clusterId,uint16 attrID,uint8 dataType, uint8* data,uint8 status); //status == 0, indicate succeed
      AT_AF_send_update(AT_ZCL_ONOFFL_ENDPOINT,ZCL_CLUSTER_ID_GEN_ON_OFF,
                         ATTRID_ON_OFF,ZCL_DATATYPE_ENUM8,
                         (uint8*) &AT_ZCL_ONOFFL_OnOff,0); //send update
  
}


/******************************************************
 * @fn      AT_ZCL_ONOFFL_EP_ENABLE
 *
 * @brief   Process  Measurement and Sensing profile call Callback function
********************************************************/
static void AT_ZCL_ONOFFL_EP_ENABLE( bool isEnable){
  if(isEnable) {
    relayL_enable();
    AT_ZCL_ONOFFL_OnOffCB(COMMAND_OFF);
  }
  else {
    relayL_disable();
    AT_ZCL_ONOFFL_BasicResetCB( );
  }
}
