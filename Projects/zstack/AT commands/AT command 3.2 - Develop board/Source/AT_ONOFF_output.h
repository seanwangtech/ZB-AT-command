#ifndef AT_ONOFF_OUTPUT_H
#define AT_ONOFF_OUTPUT_H

#define AT_ONOFF_OUTPUT_ENDPOINT           201

#define AT_ONOFF_OUTPUT_PROFID             0x0007
#define AT_ONOFF_OUTPUT_DEVICEID           0x0002
#define AT_ONOFF_OUTPUT_DEVICE_VERSION     0
#define AT_ONOFF_OUTPUT_FLAGS              0

#define AT_ONOFF_OUTPUT_MAX_CLUSTERS                 7
#define AT_ONOFF_OUTPUT_Basic_CLUSTERID                       0
#define AT_ONOFF_OUTPUT_Power_configuration_CLUSTERID         1
#define AT_ONOFF_OUTPUT_Device_temperature_cfg_CLUSTERID      2
#define AT_ONOFF_OUTPUT_Identify_CLUSTERID                    3
#define AT_ONOFF_OUTPUT_Group_CLUSTERID                       4
#define AT_ONOFF_OUTPUT_Scenes_CLUSTERID                      5
#define AT_ONOFF_OUTPUT_On_Off_CLUSTERID                      6


void AT_ONOFF_OUTPUT_Register(uint8 *task_id);

void AT_ONOFF_OUTPUT_MessageMSGCB( afIncomingMSGPacket_t *pkt );

void AT_ONOFF_OUTPUT_Cmd_RONOFF(afAddrType_t* dstAddr,uint8 on_off);










#endif