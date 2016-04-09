#ifndef AT_APP_H
#define AT_APP_H


#define AT_ENTRY_EVENT 0x4000
#define AT_RESET_EVENT 0x2000


void AT_App_Init(uint8 task_id );
uint16 AT_App_ProcessEvent( uint8 task_id, uint16 events );

extern afStatus_t AT_af_remove_ep(uint8 EndPoint);
extern afStatus_t AT_af_register_ep(uint8 EndPoint);
extern bool AT_af_exist_ep(uint8 EndPoint);

extern bool AT_ZCL_EP_ENABLE( bool isEnable,uint8 endPoint);


#endif