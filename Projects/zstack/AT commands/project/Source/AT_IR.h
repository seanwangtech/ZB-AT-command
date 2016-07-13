#include "hal_types.h"
#include "AF.h"

#define AT_IR_ENDPOINT 141

//some general clusterID
#define AT_IR_MAX_CLUSTERS                          2
#define AT_IR_CLUSTERID                             0

#define AT_IR_PROFID             0x0008
#define AT_IR_DEVICEID           0x0001
#define AT_IR_DEVICE_VERSION     1
#define AT_IR_FLAGS              0

#define AT_IR_Cmd_send_simple(nwkAddr,CID,len, pBuf)  \
  AT_IR_Cmd_send_simple_(nwkAddr,CID,len, (uint8*)pBuf)

//general cmdID
#define SEND_IR_CMD     0x00
#define UPLOAD_IR_CMD   0x01
#define IR_SEND_SUCCESS 0x00
#define IR_SEND_FAILURE 0xff

//general cluster attri
#define AT_IR_GEN_OFF    0x00
#define AT_IR_GEN_ON     0x01

#define REC_IR_ON     0x01


//data structure for IR
#define AT_IR_Cmd_req                      0x00
#define AT_IR_Cmd_rsp                      0x80
typedef struct{
  uint8 IRversion;
  uint8 IRlength;
  uint8 IRtype;
  uint8 IRaddress[2];
  uint8 IRvalue;
}IRcode;
typedef struct{
  uint8 cmd;
  uint8 cmdIR;
  uint8 status;
  IRcode code;
}AT_IR_t;

void AT_IR_Register(uint8 *task_id);
void AT_IR_MessageMSGCB( afIncomingMSGPacket_t *pkt );
afStatus_t AT_IR_Cmd_send_simple_(uint16 nwkAddr,uint16 CID,uint8 len, uint8 *buff);