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

#define AT_AF_MAX_CLUSTERS                 2
#define AT_AF_Cmd_REPPRINT_CLUSTERID       0
#define AT_AF_Cmd_REPENABLE_CLUSTERID      1

#define AT_AF_Cmd_send_simple(nwkAddr,CID,len, pBuf)  \
  AT_AF_Cmd_send_simple_(nwkAddr,CID,len, (uint8*)pBuf)


#define AT_AF_Cmd_req                      0
#define AT_AF_Cmd_rsp                      1
typedef struct{
  uint8 cmd;
  uint8 numItem;
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

void AT_AF_Register(uint8 *task_id);

void AT_AF_MessageMSGCB( afIncomingMSGPacket_t *pkt );

afStatus_t AT_AF_Cmd_send_simple_(uint16 nwkAddr,uint16 CID,uint8 len, uint8 *buff);










#endif