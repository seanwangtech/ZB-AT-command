#ifndef AT_IR_H
#define AT_IR_H

/*
在修改AT_IR这一块的代码时，有两个c语言知识点，
1.
#ifndef AT_IR_H
#define AT_IR_H
...
#endif
这个宏定义可以解决一个文件同时多次包含同一个头文件而导致冲突的问题，定义头文件时记得加上
2.
在结构体中如果定义了动态数组，如data[],需要先调用内存分配函数分配存储空间，而不能直接赋值
typedef struct{
  AT_AF_hdr hdr;
  uint8 status[];
}AT_AF_Cmd_REPPRINT_rsp_t;
上面这个结构体就会根据收到的消息动态分配内存，然后给结构体相应元素赋值。
*/

#include "hal_types.h"
#include "AF.h"

#define AT_IR_ENDPOINT 140

//some general clusterID
#define AT_IR_MAX_CLUSTERS                          2
#define AT_IR_CLUSTERID                             0

#define AT_IR_PROFID             0x0008
#define AT_IR_DEVICEID           0x0001
#define AT_IR_DEVICE_VERSION     1
#define AT_IR_FLAGS              0

#define AT_IR_Cmd_send_simple(nwkAddr,CID,len, pBuf)  \
  AT_IR_Cmd_send_simple_(nwkAddr,CID,len, (uint8*)pBuf)

//general cmdIR
#define IRMONITOR_CMD     0x01
//#define UPLOAD_IR_CMD   0x01
#define IRKEY_CMD         0x02
#define IRDELETE_CMD      0x03

//statusIR
#define IR_SEND_SUCCESS 0x00
#define IR_SEND_FAILURE 0xff


//general cmd req,rsp
#define AT_IR_Cmd_req                      0x00
#define AT_IR_Cmd_rsp                      0x80
typedef struct{
  uint8 IRhead;
  uint8 IRtail;
  uint8 IRlength;
  //uint8 IRtype;
  //uint8 IRaddress[2];
  uint8 IRkey[4];
  uint8 IRdata[128];
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

#endif