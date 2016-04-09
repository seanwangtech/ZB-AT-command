
#ifndef AT_ZCL_H
#define AT_ZCL_H

#include "zcl.h"


#define AT_ZCL_ENDPOINT            203

/*********************************************************************
 * Global variable
 */

extern SimpleDescriptionFormat_t AT_ZCL_SimpleDesc;



/*********************************************************************
 * FUNCTIONS
 */

 /*
  * register the basic end point for AT comand zcl
  */
extern void AT_ZCL_Init(void );


#endif