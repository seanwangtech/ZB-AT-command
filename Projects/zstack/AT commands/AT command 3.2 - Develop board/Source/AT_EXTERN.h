/**********************************************************************
AT_EXTERN.h
this file record the special global function that is defined in Z-Stack different
layer. Because some AT commands may need too special behavior and these behaviors
is always hard to implented in application layer. So I define them in different 
Z-Stack layers. All these funcitons are labeled "AT_EXTERN"

***************************************************************************/

#ifndef AT_EXTERN_H
#define AT_EXTERN_H


/********************************************************
label: AT_EXTERN

uint8 AT_EXTERN_CID_list(uint8 endPoint, uint16 *list);

@zcl.c in ZCL profile layer.
********************************************************/
extern uint8 AT_EXTERN_CID_list(uint8 endPoint, uint16 *list);


#endif