/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"

#include "AT_ZCL_temperature.h"


#define AT_ZCL_TEMP_DEVICE_VERSION     0
#define AT_ZCL_TEMP_FLAGS              0

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Basic Cluster
const uint8 AT_ZCL_TEMP_ManufacturerName[] = { 5, 'E','S','S','E','X' };
const uint8 AT_ZCL_TEMP_ModelId[] = { 7, 'D','S','1','8','B','2','0' };
const uint8 AT_ZCL_TEMP_DateCode[] = { 8, '2','0','1','5','0','4','0','7'};
const uint8 AT_ZCL_TEMP_PowerSource = POWER_SOURCE_BATTERY;
uint8 AT_ZCL_TEMP_DeviceEnable = DEVICE_ENABLED;

// Device Temperature Configuration Cluster
uint16 AT_ZCL_TEMP_current=0;

// Identify Cluster
uint16 AT_ZCL_TEMP_IdentifyTime = 0;


// On/Off Cluster
uint8  AT_ZCL_TEMP_OnOff = TEMP_OFF;

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */
CONST zclAttrRec_t AT_ZCL_TEMP_Attrs[AT_ZCL_TEMP_MAX_ATTRIBUTES] =
{
  // *** General Basic Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_BASIC,             // Cluster IDs - defined in the foundation (ie. zcl.h)
    {  // Attribute record
      ATTRID_BASIC_MANUFACTURER_NAME,            // Attribute ID - Found in Cluster Library header (ie. zcl_general.h)
      ZCL_DATATYPE_CHAR_STR,                 // Data Type - found in zcl.h
      ACCESS_CONTROL_READ,                // Variable access control - found in zcl.h
      (void *)AT_ZCL_TEMP_ManufacturerName  // Pointer to attribute variable
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_MODEL_ID,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)AT_ZCL_TEMP_ModelId
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DATE_CODE,
      ZCL_DATATYPE_CHAR_STR,
      ACCESS_CONTROL_READ,
      (void *)AT_ZCL_TEMP_DateCode
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_POWER_SOURCE,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&AT_ZCL_TEMP_PowerSource
    }
  },
  {
    ZCL_CLUSTER_ID_GEN_BASIC,
    { // Attribute record
      ATTRID_BASIC_DEVICE_ENABLED,
      ZCL_DATATYPE_BOOLEAN,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&AT_ZCL_TEMP_DeviceEnable
    }
  },
  
   // *** Device Temperature Configuration Cluster ***
  {
    ZCL_CLUSTER_ID_GEN_DEVICE_TEMP_CONFIG,
    { // Attribute record
      ATTRID_DEV_TEMP_CURRENT,
      ZCL_DATATYPE_DATA16,
      ACCESS_CONTROL_READ,
      (void *)&AT_ZCL_TEMP_current
    }
  },

  // *** Identify Cluster Attribute ***
  {
    ZCL_CLUSTER_ID_GEN_IDENTIFY,
    { // Attribute record
      ATTRID_IDENTIFY_TIME,
      ZCL_DATATYPE_UINT16,
      (ACCESS_CONTROL_READ | ACCESS_CONTROL_WRITE),
      (void *)&AT_ZCL_TEMP_IdentifyTime
    }
  },
  
  // *** On/Off Cluster Attributes ***
  {
    ZCL_CLUSTER_ID_GEN_ON_OFF,
    { // Attribute record
      ATTRID_ON_OFF,
      ZCL_DATATYPE_UINT8,
      ACCESS_CONTROL_READ,
      (void *)&AT_ZCL_TEMP_OnOff
    }
  }
};



/*********************************************************************
 * SIMPLE DESCRIPTOR
 */

#define AT_ZCL_TEMP_MAX_INCLUSTERS       2
const cId_t AT_ZCL_TEMP_InClusterList[AT_ZCL_TEMP_MAX_INCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_IDENTIFY
};

#define AT_ZCL_TEMP_MAX_OUTCLUSTERS       2
const cId_t AT_ZCL_TEMP_OutClusterList[AT_ZCL_TEMP_MAX_OUTCLUSTERS] =
{
  ZCL_CLUSTER_ID_GEN_BASIC,
  ZCL_CLUSTER_ID_GEN_DEVICE_TEMP_CONFIG
};

SimpleDescriptionFormat_t AT_ZCL_TEMP_SimpleDesc =
{
  AT_ZCL_TEMP_ENDPOINT,                  //  int Endpoint;
  ZCL_HA_PROFILE_ID,                     //  uint16 AppProfId[2];
  ZCL_HA_DEVICEID_TEST_DEVICE,           //  uint16 AppDeviceId[2];
  AT_ZCL_TEMP_DEVICE_VERSION,            //  int   AppDevVer:4;
  AT_ZCL_TEMP_FLAGS,                     //  int   AppFlags:4;
  AT_ZCL_TEMP_MAX_INCLUSTERS,            //  byte  AppNumInClusters;
  (cId_t *)AT_ZCL_TEMP_InClusterList,    //  byte *pAppInClusterList;
  AT_ZCL_TEMP_MAX_OUTCLUSTERS,           //  byte  AppNumInClusters;
  (cId_t *)AT_ZCL_TEMP_OutClusterList    //  byte *pAppInClusterList;
};