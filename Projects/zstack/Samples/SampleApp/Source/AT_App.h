#ifndef AT_APP_H
#define AT_APP_H


#define AT_ENTRY_EVENT 0x4000
#define AT_RESET_EVENT 0x2000


void AT_App_Init(uint8 task_id );
uint16 AT_App_ProcessEvent( uint8 task_id, uint16 events );

#endif