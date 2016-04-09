#ifndef AT_ZDO_H
#define AT_ZDO_H


// Display Neighbour Table for AT+NTABLE
// This structure is used to build the Mgmt_Lqi_rsp
typedef struct
{
  uint8  extPanID[Z_EXTADDR_LEN];// Extended Pan ID
  uint8  extAddr[Z_EXTADDR_LEN]; // Extended address
  uint16 nwkAddr;                // Network address
  uint8  devType  :2;            // Device type
  uint8  rxOnIdle :2;            // RxOnWhenIdle
  uint8  relation :4;            // Relationship
  uint8  permit;                 // Permit joining
  uint8  depth;                  // Depth
  uint8  lqi;                    // LQI
} AT_ZDP_MgmtLqiItem_t;

typedef struct{
  uint8  status;
  uint8  neighborLqiEntries;
  uint8  startIndex;
  uint8  neighborLqiCount;
  AT_ZDP_MgmtLqiItem_t list[];
}AT_ZDO_MgmtLqiRsp_t;


void AT_ZDO_Register(uint8 *pTask_id);

void AT_ZDO_ProcessMsgCBs( zdoIncomingMsg_t *inMsg );

// Mgmt_Lqi_rsp - (neighbor table) device type
#define ZDP_MGMT_DT_COORD  0x0
#define ZDP_MGMT_DT_ROUTER 0x1
#define ZDP_MGMT_DT_ENDDEV 0x2

// Mgmt_Lqi_rsp - (neighbor table) relationship
#define ZDP_MGMT_REL_PARENT  0x0
#define ZDP_MGMT_REL_CHILD   0x1
#define ZDP_MGMT_REL_SIBLING 0x2
#define ZDP_MGMT_REL_UNKNOWN 0x3



#endif