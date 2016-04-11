
/*********************************************************************
 * INCLUDES
 */

#ifndef NONWK
#include "AF.h"
#endif
#include "hal_adc.h"
#include "hal_flash.h"
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_drivers.h"
#include "OnBoard.h"
#include "OSAL.h"
#include "OSAL_Nv.h"
#include "ZComDef.h"
#include "ZMAC.h"

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void zmain_ext_addr( void );
#if defined ZCL_KEY_ESTABLISH
static void zmain_cert_init( void );
#endif
static void zmain_dev_info( void );
static void zmain_vdd_check( void );

#ifdef LCD_SUPPORTED
static void zmain_lcd_init( void );
#endif

typedef unsigned char uchar;
typedef unsigned int  uint;
typedef unsigned long ulong;
/****************************************************************************
* 名    称: SysPowerMode()
* 功    能: 设置系统工作模式
* 入口参数: mode等于0为PM0 1为PM1 2为PM2 3为PM3             
* 出口参数: 无
****************************************************************************/
void SysPowerMode(uchar mode);
void SysPowerMode(uchar mode) 
{ 
    if(mode < 4) 
    {  
        SLEEPCMD |= mode;    //设置系统睡眠模式 
        PCON = 0x01;         //进入睡眠模式 ,通过中断唤醒
    } 
    else 
        PCON = 0x00;         //通过中断唤醒系统 
}
/****************************************************************************
* 名    称: SysPowerMode()
* 功    能: 初始化休眠定时器,设定后经过指定时间自行唤醒
* 入口参数:             
* 出口参数: 无
****************************************************************************/
void InitSleepTimer(void); 
void InitSleepTimer(void) 
{ 
    ST2 = 0x00; 
    ST1 = 0x00; 
    ST0 = 0x00; 
    EA = 1;     //开中断 
    STIE = 1;   //睡眠定时器中断使能 0： 中断禁止     1： 中断使能
    STIF = 0;   //睡眠定时器中断标志 0： 无中断未决   1： 中断未决
}

/****************************************************************************
* 名    称: Set_ST_Period()
* 功    能: 设置睡眠时间 
* 入口参数: sec 睡眠时间             
* 出口参数: 无
****************************************************************************/
void Set_ST_Period(uint ms_100); 
void Set_ST_Period(uint ms_100) 
{ 
    ulong sleepTimer = 0; 
    
    sleepTimer |= ST0; 
    sleepTimer |= (ulong)ST1 <<  8; 
    sleepTimer |= (ulong)ST2 << 16; 
    sleepTimer += ((ulong)ms_100 * (ulong)32768/10); 
    //sleepTimer += ((ulong)sec * (ulong)32768); 
    ST2 = (uchar)(sleepTimer >> 16); 
    ST1 = (uchar)(sleepTimer >> 8); 
    ST0 = (uchar) sleepTimer; 
}


/*********************************************************************
 * @fn      main
 * @brief   First function called after startup.
 * @return  don't care
 */
int main( void )
{
#include "AT_TOUCH_reset.h"
  AT_TOUCH_RESET_INIT();
  AT_TOUCH_DISABLE();
  P0INP=0xff;
  P1INP=0xff;
  /*
  #define Touch_PIN P1_5
  //set touch chip reset pin to OUTPUT P1_5 and initilized volatage is high
  Touch_PIN =0; //disable
  P1DIR |= BV(5);
  */
  InitSleepTimer();        //初始化休眠定时器
   Set_ST_Period(1);   //设置睡眠时间,睡眠100ms后唤醒系统
   SysPowerMode(2);    //进入睡眠模式PM2
  //recover defalut
  P0INP=0x00;
  P1INP=0x00;
  

  // Turn off interrupts
  osal_int_disable( INTS_ALL );
  // Initialization for board related stuff such as LEDs
  HAL_BOARD_INIT();

  // Make sure supply voltage is high enough to run
  zmain_vdd_check();

  // Initialize board I/O
  InitBoard( OB_COLD );

  // Initialze HAL drivers
  HalDriverInit();

  // Initialize NV System
  osal_nv_init( NULL );

  // Initialize the MAC
  ZMacInit();

  // Determine the extended address
  zmain_ext_addr();

#if defined ZCL_KEY_ESTABLISH
  // Initialize the Certicom certificate information.
  zmain_cert_init();
#endif

  // Initialize basic NV items
  zgInit();

#ifndef NONWK
  // Since the AF isn't a task, call it's initialization routine
  afInit();
#endif

  // Initialize the operating system
  osal_init_system();

  // Allow interrupts
  osal_int_enable( INTS_ALL );

  // Final board initialization
  InitBoard( OB_READY );

  // Display information about this device
  zmain_dev_info();

  /* Display the device info on the LCD */
#ifdef LCD_SUPPORTED
  zmain_lcd_init();
#endif

#ifdef WDT_IN_PM1
  /* If WDT is used, this is a good place to enable it. */
  WatchDogEnable( WDTIMX );
#endif

  osal_start_system(); // No Return from here

  return 0;  // Shouldn't get here.
} // main()

/*********************************************************************
 * @fn      zmain_vdd_check
 * @brief   Check if the Vdd is OK to run the processor.
 * @return  Return if Vdd is ok; otherwise, flash LED, then reset
 *********************************************************************/
static void zmain_vdd_check( void )
{
  uint8 cnt = 16;
  
  do {
    while (!HalAdcCheckVdd(VDD_MIN_RUN));
  } while (--cnt);
}

/**************************************************************************************************
 * @fn          zmain_ext_addr
 *
 * @brief       Execute a prioritized search for a valid extended address and write the results
 *              into the OSAL NV system for use by the system. Temporary address not saved to NV.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
static void zmain_ext_addr(void)
{
  uint8 nullAddr[Z_EXTADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8 writeNV = TRUE;

  // First check whether a non-erased extended address exists in the OSAL NV.
  if ((SUCCESS != osal_nv_item_init(ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL))  ||
      (SUCCESS != osal_nv_read(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, aExtendedAddress)) ||
      (osal_memcmp(aExtendedAddress, nullAddr, Z_EXTADDR_LEN)))
  {
    // Attempt to read the extended address from the location on the lock bits page
    // where the programming tools know to reserve it.
    HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IEEE_OSET, aExtendedAddress, Z_EXTADDR_LEN);

    if (osal_memcmp(aExtendedAddress, nullAddr, Z_EXTADDR_LEN))
    {
      // Attempt to read the extended address from the designated location in the Info Page.
      if (!osal_memcmp((uint8 *)(P_INFOPAGE+HAL_INFOP_IEEE_OSET), nullAddr, Z_EXTADDR_LEN))
      {
        osal_memcpy(aExtendedAddress, (uint8 *)(P_INFOPAGE+HAL_INFOP_IEEE_OSET), Z_EXTADDR_LEN);
      }
      else  // No valid extended address was found.
      {
        uint8 idx;
        
#if !defined ( NV_RESTORE )
        writeNV = FALSE;  // Make this a temporary IEEE address
#endif

        /* Attempt to create a sufficiently random extended address for expediency.
         * Note: this is only valid/legal in a test environment and
         *       must never be used for a commercial product.
         */
        for (idx = 0; idx < (Z_EXTADDR_LEN - 2);)
        {
          uint16 randy = osal_rand();
          aExtendedAddress[idx++] = LO_UINT16(randy);
          aExtendedAddress[idx++] = HI_UINT16(randy);
        }
        // Next-to-MSB identifies ZigBee devicetype.
#if ZG_BUILD_COORDINATOR_TYPE && !ZG_BUILD_JOINING_TYPE
        aExtendedAddress[idx++] = 0x10;
#elif ZG_BUILD_RTRONLY_TYPE
        aExtendedAddress[idx++] = 0x20;
#else
        aExtendedAddress[idx++] = 0x30;
#endif
        // MSB has historical signficance.
        aExtendedAddress[idx] = 0xF8;
      }
    }

    if (writeNV)
    {
      (void)osal_nv_write(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, aExtendedAddress);
    }
  }

  // Set the MAC PIB extended address according to results from above.
  (void)ZMacSetReq(MAC_EXTENDED_ADDRESS, aExtendedAddress);
}

#if defined ZCL_KEY_ESTABLISH
/**************************************************************************************************
 * @fn          zmain_cert_init
 *
 * @brief       Initialize the Certicom certificate information.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
static void zmain_cert_init(void)
{
  uint8 certData[ZCL_KE_IMPLICIT_CERTIFICATE_LEN];
  uint8 nullData[ZCL_KE_IMPLICIT_CERTIFICATE_LEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };

  (void)osal_nv_item_init(ZCD_NV_IMPLICIT_CERTIFICATE, ZCL_KE_IMPLICIT_CERTIFICATE_LEN, NULL);
  (void)osal_nv_item_init(ZCD_NV_DEVICE_PRIVATE_KEY, ZCL_KE_DEVICE_PRIVATE_KEY_LEN, NULL);

  // First check whether non-null certificate data exists in the OSAL NV. To save on code space,
  // just use the ZCD_NV_CA_PUBLIC_KEY as the bellwether for all three.
  if ((SUCCESS != osal_nv_item_init(ZCD_NV_CA_PUBLIC_KEY, ZCL_KE_CA_PUBLIC_KEY_LEN, NULL))    ||
      (SUCCESS != osal_nv_read(ZCD_NV_CA_PUBLIC_KEY, 0, ZCL_KE_CA_PUBLIC_KEY_LEN, certData))  ||
      (osal_memcmp(certData, nullData, ZCL_KE_CA_PUBLIC_KEY_LEN)))
  {
    // Attempt to read the certificate data from its corresponding location on the lock bits page.
    HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_CA_PUBLIC_KEY_OSET, certData,
                                         ZCL_KE_CA_PUBLIC_KEY_LEN);
    // If the certificate data is not NULL, use it to update the corresponding NV items.
    if (!osal_memcmp(certData, nullData, ZCL_KE_CA_PUBLIC_KEY_LEN))
    {
      (void)osal_nv_write(ZCD_NV_CA_PUBLIC_KEY, 0, ZCL_KE_CA_PUBLIC_KEY_LEN, certData);
      HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IMPLICIT_CERT_OSET, certData,
                                           ZCL_KE_IMPLICIT_CERTIFICATE_LEN);
      (void)osal_nv_write(ZCD_NV_IMPLICIT_CERTIFICATE, 0,
                          ZCL_KE_IMPLICIT_CERTIFICATE_LEN, certData);
      HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_DEV_PRIVATE_KEY_OSET, certData,
                                        ZCL_KE_DEVICE_PRIVATE_KEY_LEN);
      (void)osal_nv_write(ZCD_NV_DEVICE_PRIVATE_KEY, 0, ZCL_KE_DEVICE_PRIVATE_KEY_LEN, certData);
    }
  }
}
#endif

/**************************************************************************************************
 * @fn          zmain_dev_info
 *
 * @brief       This displays the IEEE (MSB to LSB) on the LCD.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
static void zmain_dev_info(void)
{
#ifdef LCD_SUPPORTED
  uint8 i;
  uint8 *xad;
  uint8 lcd_buf[Z_EXTADDR_LEN*2+1];

  // Display the extended address.
  xad = aExtendedAddress + Z_EXTADDR_LEN - 1;

  for (i = 0; i < Z_EXTADDR_LEN*2; xad--)
  {
    uint8 ch;
    ch = (*xad >> 4) & 0x0F;
    lcd_buf[i++] = ch + (( ch < 10 ) ? '0' : '7');
    ch = *xad & 0x0F;
    lcd_buf[i++] = ch + (( ch < 10 ) ? '0' : '7');
  }
  lcd_buf[Z_EXTADDR_LEN*2] = '\0';
  HalLcdWriteString( "IEEE: ", HAL_LCD_LINE_1 );
  HalLcdWriteString( (char*)lcd_buf, HAL_LCD_LINE_2 );
#endif
}

#ifdef LCD_SUPPORTED
/*********************************************************************
 * @fn      zmain_lcd_init
 * @brief   Initialize LCD at start up.
 * @return  none
 *********************************************************************/
static void zmain_lcd_init ( void )
{
#ifdef SERIAL_DEBUG_SUPPORTED
  {
    HalLcdWriteString( "TexasInstruments", HAL_LCD_LINE_1 );

#if defined( MT_MAC_FUNC )
#if defined( ZDO_COORDINATOR )
      HalLcdWriteString( "MAC-MT Coord", HAL_LCD_LINE_2 );
#else
      HalLcdWriteString( "MAC-MT Device", HAL_LCD_LINE_2 );
#endif // ZDO
#elif defined( MT_NWK_FUNC )
#if defined( ZDO_COORDINATOR )
      HalLcdWriteString( "NWK Coordinator", HAL_LCD_LINE_2 );
#else
      HalLcdWriteString( "NWK Device", HAL_LCD_LINE_2 );
#endif // ZDO
#endif // MT_FUNC
  }
#endif // SERIAL_DEBUG_SUPPORTED
}
#endif

/*********************************************************************
*********************************************************************/
