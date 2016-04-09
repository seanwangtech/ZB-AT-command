/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ms.h"
#include "zcl_ha.h"

#include "AT_ZCL_ONOFF_SWITCH.h"


#define AT_ZCL_ONOFF_SWITCH_DEVICE_VERSION     0
#define AT_ZCL_ONOFF_SWITCH_FLAGS              0

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Basic Cluster
const uint8 AT_ZCL_ONOFF_SWITCH_ManufacturerName[] = { 5, 'E','S','S','E','X' };
const uint8 AT_ZCL_ONOFF_SWITCH_ModelId[] = { 6, 'S','W','I','T','C','H' };
const uint8 AT_ZCL_ONOFF_SWITCH_DateCode[] = { 8, '2','0','1','5','0','4','1','0'};
const uint8 AT_ZCL_ONOFF_SWITCH_PowerSource = POWER_SOURCE_BATTERY;
const uint8 AT_ZCL_ONOFF_SWITCH_type =0xFF;   //stand for unknown
uint8 AT_ZCL_ONOFF_SWITCH_LocationDescription[]={ 10, 'L','A','B','0','0','7',' ',' ',' ',' ' };
uint8 AT_ZCL_ONOFF_SWITCH_DeviceEnable = DEVICE_ENABLED;

// Device Temperature Configuration Cluster
int16 AT_ZCL_ONOFF_SWITCH_current=0;

// Identify Cluster
uint16 AT_ZCL_ONOFF_SWITCH_IdentifyTime = 0;


// On/Off Cluster
uint8  AT_ZCL_ONOFF_SWITCH_OnOff = AT_ZCL_GEN_OFF;


// On/Off Switch Cluster Attributes
//0xFF indicates that the value is invalid.
uint8  AT_ZCL_ONOFF_SWITCH_action = 0xFF;

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
CONST zclAttrRec_t AT_ZCL_ONOFF_SWITCH_Attrs[AT_ZCL_ONOFF_SWITCH_MAX_ATTRIBUTES] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  // Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_CHAR_STR,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)AT_ZCL_ONOFF_SWITCH_ManufacturerName  // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)AT_ZCL_ONOFF_SWITCH_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)AT_ZCL_ONOFF_SWITCH_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&AT_ZCL_ONOFF_SWITCH_PowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_LOCATION_DESC,
      ZCL_DATATYPE_CHAR_STR,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)AT_ZCL_ONOFF_SWITCH_LocationDescription
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&AT_ZCL_ONOFF_SWITCH_DeviceEnable
    }
  },

  // *** Identify Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // Attribute record
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&AT_ZCL_ONOFF_SWITCH_IdentifyTime
    }
  },
  
  // *** On/Off Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&AT_ZCL_ONOFF_SWITCH_OnOff
    }
  }, 
  
  
  // *** On/Off Switch Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG,
    { // Attribute record
      ATTRID_ON_OFF_SWITCH_TYPE,
      ZCL_DATATYPE_ENUM8,
      ACCESS_CONTROL_READ,
      (void *)&AT_ZCL_ONOFF_SWITCH_type
    }
  }, 
  
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG,
    { // Attribute record
      ATTRID_ON_OFF_SWITCH_ACTIONS,
      ZCL_DATATYPE_ENUM8,
      ACCESS_CONTROL_READ,
      (void *)&AT_ZCL_ONOFF_SWITCH_action
    }
  }, 
  
  
};



/*********************************************************************
 * SIMPLE DESCRIPTOR
 */

#define AT_ZCL_ONOFF_SWITCH_MAX_INCLUSTERS       2
const cId_t AT_ZCL_ONOFF_SWITCH_InClusterList[AT_ZCL_ONOFF_SWITCH_MAX_INCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY
};

#define AT_ZCL_ONOFF_SWITCH_MAX_OUTCLUSTERS       4
const cId_t AT_ZCL_ONOFF_SWITCH_OutClusterList[AT_ZCL_ONOFF_SWITCH_MAX_OUTCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY,
  ZCL_CLUSTER_ID_GEN_ON_OFF,
  ZCL_CLUSTER_ID_GEN_ON_OFF_SWITCH_CONFIG
};

SimpleDescriptionFormat_t AT_ZCL_ONOFF_SWITCH_SimpleDesc =
{
  AT_ZCL_ONOFF_SWITCH_ENDPOINT,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,                     //  uint16 AppProfId[2];
  ZCL_HA_DEVICEID_TEST_DEVICE,           //  uint16 AppDeviceId[2];
  AT_ZCL_ONOFF_SWITCH_DEVICE_VERSION,            //  int   AppDevVer:4;
  AT_ZCL_ONOFF_SWITCH_FLAGS,                     //  int   AppFlags:4;
  AT_ZCL_ONOFF_SWITCH_MAX_INCLUSTERS,            //  byte  AppNumInClusters;
  (cId_t *)AT_ZCL_ONOFF_SWITCH_InClusterList,    //  byte *pAppInClusterList;
  AT_ZCL_ONOFF_SWITCH_MAX_OUTCLUSTERS,           //  byte  AppNumInClusters;
  (cId_t *)AT_ZCL_ONOFF_SWITCH_OutClusterList    //  byte *pAppInClusterList;
};