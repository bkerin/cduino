/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
* File Name          : example_main.c
* Author             : MSH Application Team
* Author             : Fabio Tota
* Revision           : $Revision: 1.5 $
* Date               : $Date: 25/08/2011 12:19:08 $
* Description        : EKSTM32 main file
* HISTORY:
* Date        | Modification                                | Author
* 25/08/2011  | Initial Revision                            | Fabio Tota
* 17/05/2012  | Uses LIS331DLH driver suitable for use in a program with other MEMS drivers            | Abhishek Anand

********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
*
*******************************************************************************/


/* Includes ------------------------------------------------------------------*/
//include files for MKI109V1 board 
#include "stm32f10x.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "i2c_mems.h"
#include "hw_config.h"
#include "usb_pwr.h"
#include "led.h"
#include "button.h"
#include "adc_mems.h"
#include "string.h"
#include <stdio.h>

//include MEMS driver
#include "lis331dlh_driver.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t response;
uint8_t USBbuffer[64];

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

//define for example1 or example2
#define __EXAMPLE1__H 
//#define __EXAMPLE2__H 


/*******************************************************************************
* Function Name  : main.
* Description    : Main routine.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
int main(void)
{
  uint8_t buffer[26]; 
  uint8_t position=0, old_position=0;
  AxesRaw_t data;
  i8_t val=0;
  uint8_t len = 0;

  //Initialize your hardware here
  
  //function for MKI109V1 board 
  InitHardware();
  I2C_MEMS_Init();
  
  EKSTM32_LEDOff(LED1);
  EKSTM32_LEDOff(LED2);
  EKSTM32_LEDOff(LED3);    
  
  //wait until the USB is ready (MKI109V1 board)
  while(bDeviceState != CONFIGURED);
  EKSTM32_LEDOn(LED3); 
  
  //Inizialize MEMS Sensor
  //set ODR (turn ON device)
  response = LIS331DLH_SetODR( LIS331DLH_ODR_100Hz);
  if(response==1){
	len = sprintf((char*)buffer,"\n\rSET_ODR_OK\n\r");
	USB_SIL_Write(EP1_IN, buffer, len);
	SetEPTxValid(ENDP1);
  } 
  
  //set PowerMode 
  response = LIS331DLH_SetMode( LIS331DLH_NORMAL);
  if(response==1){
	len = sprintf((char*)buffer,"SET_MODE_OK\n\r");
	USB_SIL_Write(EP1_IN, buffer, len);
	SetEPTxValid(ENDP1);
	
  }
  
  //set Fullscale
  response = LIS331DLH_SetFullScale( LIS331DLH_FULLSCALE_2);
  if(response==1){        
	len = sprintf((char*)buffer,"SET_FULLSCALE_OK\n\r");
    USB_SIL_Write(EP1_IN, buffer, len);
    SetEPTxValid(ENDP1);
  } 
  
  //set axis Enable
  response = LIS331DLH_SetAxis( LIS331DLH_X_ENABLE | LIS331DLH_Y_ENABLE |  LIS331DLH_Z_ENABLE);
  if(response==1){
	len = sprintf((char*)buffer,"SET_AXIS_OK\n\r");
	USB_SIL_Write(EP1_IN, buffer, len);
	SetEPTxValid(ENDP1);
  } 
 
/******Example 1******/ 
#ifdef __EXAMPLE1__H 
  while(1){
  //get Acceleration Raw data  
  response = LIS331DLH_GetAccAxesRaw(&data);
  if(response==1){
    //print data values
    //function for MKI109V1 board 
    EKSTM32_LEDToggle(LED1);
    len = sprintf((char*)buffer, "X=%6d Y=%6d Z=%6d\r\n", data.AXIS_X, data.AXIS_Y, data.AXIS_Z);
    USB_SIL_Write(EP1_IN, buffer, len);
    SetEPTxValid(ENDP1);  
  }
 }
#endif /* __EXAMPLE1__H  */ 
 

 /******Example 2******/
#ifdef __EXAMPLE2__H
 //Inizialize MEMS Sensor
 //set Interrupt Threshold 
 response = LIS331DLH_SetInt1Threshold(20);
 if(response==1){
   len = sprintf((char*)buffer,"SET_THRESHOLD_OK\n\r");
   USB_SIL_Write(EP1_IN, buffer, len);
   SetEPTxValid(ENDP1);
 }
 //set Interrupt configuration (all enabled)
 response = LIS331DLH_SetInt1Configuration(  LIS331DLH_INT_ZHIE_ENABLE |  LIS331DLH_INT_ZLIE_ENABLE |
										   LIS331DLH_INT_YHIE_ENABLE |  LIS331DLH_INT_YLIE_ENABLE |
											 LIS331DLH_INT_XHIE_ENABLE |  LIS331DLH_INT_XLIE_ENABLE ); 
 if(response==1){
   len = sprintf((char*)buffer,"SET_INT_CONF_OK \n\r");
   USB_SIL_Write(EP1_IN, buffer, len);
   SetEPTxValid(ENDP1);
 }
 //set Interrupt Mode
 response = LIS331DLH_SetInt1Mode( LIS331DLH_INT_MODE_6D_POSITION);
 if(response==1){
   len = sprintf((char*)buffer,"SET_INT_MODE\n\r");
   USB_SIL_Write(EP1_IN, buffer, len);
   SetEPTxValid(ENDP1);
 }
 
 while(1) {
   //get 6D Position
   response = LIS331DLH_Get6DPositionInt1(&position);
   if((response==1) && (old_position!=position)){
	 EKSTM32_LEDToggle(LED1);
	 switch (position){
	 case  LIS331DLH_UP_SX:
	   len = sprintf((char*)buffer,"\n\rposition = UP_SX\n\r");   
	   break;
	 case  LIS331DLH_UP_DX:
	   len = sprintf((char*)buffer,"\n\rposition = UP_DX\n\r");
	   break;
	 case  LIS331DLH_DW_SX:
	   len = sprintf((char*)buffer,"\n\rposition = DW_SX\n\r");   
	   break;              
	 case  LIS331DLH_DW_DX:   
	   len = sprintf((char*)buffer,"\n\rposition = DW_DX\n\r");   
	   break; 
	 case  LIS331DLH_TOP:     
	   len = sprintf((char*)buffer,"\n\rposition = TOP\n\r");   
	   break; 
	 case  LIS331DLH_BOTTOM:  
	   len = sprintf((char*)buffer,"\n\rposition = BOTTOM\n\r");   
	   break; 
	 default:      
	   len = sprintf((char*)buffer,"\n\rposition = unknown\n\r");   
	   break;	 
	 }
	 
	 //function for MKI109V1 board   
	 USB_SIL_Write(EP1_IN, buffer, len);
	 SetEPTxValid(ENDP1);  
	 old_position = position;
   }
 }
#endif /*__EXAMPLE2__H */ 
 
 
} // end main


//function for MKI109V1 board 
#ifdef USE_FULL_ASSERT
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert_param error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert_param error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

/******************* (C) COPYRIGHT 2012 STMicroelectronics *****END OF FILE****/
