#ifndef AT_APP_H
#define AT_APP_H


#define AT_ENTRY_EVENT            0x4000
#define AT_RESET_EVENT            0x2000
#define AT_POWER_SAVING_EXP_EVENT 0x1000
#define AT_ENTRY_DELAY_EVENT      0x0800
#define AT_ATF_DELAY_EVENT        0x0400
#define AT_DEV_REPORT_EVENT       0x0200
typedef struct{
  uint16 nwkAddr;
  uint16 count;
  uint16 interval;
} AT_App_Cmd_POWER_SAVING_EXP_t;

/************************************************************
*global variables
*/
extern uint8 AT_Is_Network_connect;

/********************************************************
* functions
*/
void AT_App_Init(uint8 task_id );
uint16 AT_App_ProcessEvent( uint8 task_id, uint16 events );

extern afStatus_t AT_af_remove_ep(uint8 EndPoint);
extern afStatus_t AT_af_register_ep(uint8 EndPoint);
extern epList_t* AT_af_get_ep(uint8 EndPoint);
extern uint8 AT_af_ep_num( void );
extern void AT_af_ep_list( uint8 len, uint8 *list );
extern uint8 AT_App_Power_saving_exp(AT_App_Cmd_POWER_SAVING_EXP_t* pBuf);
extern void AT_App_Power_saving_exp_stop(void );



#endif