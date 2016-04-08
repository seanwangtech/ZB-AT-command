#ifndef AT_AF_H
#define AT_AF_H

#define AT_AF_ENDPOINT           204

#define AT_AF_PROFID             0x0007
#define AT_AF_DEVICEID           0x0002
#define AT_AF_DEVICE_VERSION     0
#define AT_AF_FLAGS              0

#define AT_AF_UNKNOWNEP          2
#define AT_AF_enable             1
#define AT_AF_disable            0

#define AT_AF_MAX_CLUSTERS                 7
#define AT_AF_Cmd_REPPRINT_CLUSTERID       0
#define AT_AF_Cmd_REPENABLE_CLUSTERID      1
#define AT_AF_Cmd_HA_DISC_CLUSTERID        2
#define AT_AF_Cmd_HA_CIDDISC_CLUSTERID     3
#define AT_AF_Cmd_R_CIDDISC_CLUSTERID      4
#define AT_AF_TEST_KEY_CLUSTERID           5
#define AT_AF_POWER_SVING_EXP_CLUSTERID    6
#define AT_AF_GROUP_ID                     0x8000


#define AT_AF_NOTFOUND_ERROR              0x40
#define AT_AF_MEM_ERROR                   0x41

#define AT_AF_ClusterList_  {                   \
  AT_AF_Cmd_REPPRINT_CLUSTERID,                 \
  AT_AF_Cmd_REPENABLE_CLUSTERID,                \
  AT_AF_Cmd_HA_DISC_CLUSTERID,                  \
  AT_AF_Cmd_HA_CIDDISC_CLUSTERID,               \
  AT_AF_Cmd_R_CIDDISC_CLUSTERID,                \
  AT_AF_POWER_SVING_EXP_CLUSTERID               \
}


#define AT_AF_Cmd_send_simple(nwkAddr,CID,len, pBuf)  \
  AT_AF_Cmd_send_simple_(nwkAddr,CID,len, (uint8*)pBuf)


#define AT_AF_Cmd_req                      0x00
#define AT_AF_Cmd_rsp                      0x80
typedef struct{
  uint8 cmd;
  union {
    uint8 numItem;
    uint8 ep;
    uint8 info;
  };
} AT_AF_hdr;

typedef struct{
  uint8 ep;
  uint8 status;
} AT_AF_Cmd_REP_rec_t;

typedef struct{
  AT_AF_hdr hdr;
  AT_AF_Cmd_REP_rec_t item[];
}AT_AF_Cmd_REP_t;

//the structure for REPPRINT command
typedef struct{
  AT_AF_hdr hdr;
  uint8 status[];
}AT_AF_Cmd_REPPRINT_rsp_t;


//the structure for REPENABLE command
typedef struct{
  AT_AF_hdr hdr;
  uint8 enable;
  uint8 ep;
}AT_AF_Cmd_REPENABLE_req_t;

typedef struct{
  AT_AF_hdr hdr;
  AT_AF_Cmd_REP_rec_t item;
}AT_AF_Cmd_REPENABLE_rsp_t;

//the structure for HA_DISC command
typedef struct{
  AT_AF_hdr hdr;
  uint16 CID;
  uint8 option;
}AT_AF_Cmd_HA_DISC_req_t;

typedef struct{
  uint8 ep;
  uint8 status;
}AT_AF_Cmd_HA_DISC_item_t;

typedef struct{
  AT_AF_hdr hdr;
  AT_AF_Cmd_HA_DISC_item_t item[];
}AT_AF_Cmd_HA_DISC_rsp_t;

//the structure for HA_CIDDISC command
typedef struct{
  AT_AF_hdr hdr;
  uint8 status;
  uint8 serverNum;
  uint8 clientNum;
  uint16 list[];      //int the front of the list is the server number  and followind is client
} AT_AF_Cmd_HA_CIDDISC_rsp_t;


//the structure for R_CIDDISC command
typedef struct{
  AT_AF_hdr hdr;
  uint8 list[];
} AT_AF_Cmd_RCIDDISC_req_t;

//the necessary structure for POWER SVING experiment structure
#define AT_AT_PSE_RSSI_req 0x01
#define AT_AT_PSE_RSSI_rsp AT_AF_Cmd_rsp|AT_AT_PSE_RSSI_req
#define AT_AT_PSE_EXP_req 0x02
#define AT_AT_PSE_EXP_rsp AT_AF_Cmd_rsp|AT_AT_PSE_EXP_req

#define AT_AF_PSE_info_pre   0x01
#define AT_AF_PSE_info_start 0x02
#define AT_AF_PSE_info_ing   0x03
#define AT_AF_PSE_info_end   0x04
typedef struct{
  AT_AF_hdr hdr;
  uint16 count;
  uint16 interval;
}AT_AF_Cmd_POWER_SAVING_EXP_t;

typedef struct{
  AT_AF_hdr hdr;
  uint8 lqi;
  int8 rssi;
  uint8 correlation;
}AT_AF_Cmd_POWER_SAVING_rssi_t;


//global variable
extern endPointDesc_t AT_AF_epDesc;
extern uint8 AT_AF_TransID;  // This is the unique message ID (counter)


void AT_AF_Register(uint8 *task_id);

void AT_AF_MessageMSGCB( afIncomingMSGPacket_t *pkt );

afStatus_t AT_AF_Cmd_send_simple_(uint16 nwkAddr,uint16 CID,uint8 len, uint8 *buff);










#endif