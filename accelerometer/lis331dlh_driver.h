// Turns out ST Microelectronics already provides an almost-finished
// interface for us.  See the comments at the start of lis331dlh_driver.c.

/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
* File Name          : lis331dlh_driver.h
* Author             : MSH Application Team
* Author             : Abhishek Anand						
* Version            : $Revision:$
* Date               : $Date:$
* Description        : Descriptor Header for lis331dlh_driver.c driver file
*
* HISTORY:
* Date        | Modification                                | Author
* 16/05/2012  | Initial Revision                            | Abhishek Anand
* 17/05/2012  |  Modified to support multiple drivers in the same program                |	Abhishek Anand

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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LIS331DLH_DRIVER__H
#define __LIS331DLH_DRIVER__H

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

//these could change accordingly with the architecture

#ifndef __ARCHDEP__TYPES
#define __ARCHDEP__TYPES
typedef unsigned char u8_t;
typedef unsigned short int u16_t;
typedef short int i16_t;
typedef short int i8_t;

#endif /*__ARCHDEP__TYPES*/

typedef u8_t LIS331DLH_Axis_t;
typedef u8_t LIS331DLH_IntConf_t;

//define structure
#ifndef __SHARED__TYPES
#define __SHARED__TYPES

typedef enum {
  MEMS_SUCCESS                            =		0x01,
  MEMS_ERROR			          =		0x00	
} status_t;

typedef enum {
  MEMS_ENABLE			          =		0x01,
  MEMS_DISABLE			          =		0x00	
} State_t;

typedef struct {
  i16_t AXIS_X;
  i16_t AXIS_Y;
  i16_t AXIS_Z;
} AxesRaw_t;

#endif /*__SHARED__TYPES*/

typedef enum {  
  LIS331DLH_ODR_50Hz                       =		0x00,
  LIS331DLH_ODR_100Hz		          =		0x01,	
  LIS331DLH_ODR_400Hz		          =		0x02,
  LIS331DLH_ODR_1000Hz		          =		0x03
} LIS331DLH_ODR_t;

typedef enum {
  LIS331DLH_CONTINUOUS_MODE                =		0x00,
  LIS331DLH_SINGLE_MODE 		          =		0x01,
  LIS331DLH_SLEEP_MODE			  =		0x02
} LIS331DLH_Mode_M_t;

typedef enum {
  LIS331DLH_POWER_DOWN                     =		0x00,
  LIS331DLH_NORMAL 			  =		0x01,
  LIS331DLH_LOW_POWER_05		          =		0x02,
  LIS331DLH_LOW_POWER_1 		          =		0x03,
  LIS331DLH_LOW_POWER_2			  =		0x04,
  LIS331DLH_LOW_POWER_5			  =		0x05,
  LIS331DLH_LOW_POWER_10		          =		0x06,
} LIS331DLH_Mode_t;

typedef enum {
  LIS331DLH_HPM_NORMAL_MODE_RES            =             0x00,
  LIS331DLH_HPM_REF_SIGNAL                 =             0x01,
  LIS331DLH_HPM_NORMAL_MODE                =             0x02,
} LIS331DLH_HPFMode_t;

typedef enum {
  LIS331DLH_HPFCF_0                        =             0x00,
  LIS331DLH_HPFCF_1                        =             0x01,
  LIS331DLH_HPFCF_2                        =             0x02,
  LIS331DLH_HPFCF_3                        =             0x03
} LIS331DLH_HPFCutOffFreq_t;

typedef enum {
  LIS331DLH_INT_SOURCE                     =             0x00,
  LIS331DLH_INT_1OR2_SOURCE                =             0x01,
  LIS331DLH_DATA_READY                     =             0x02,
  LIS331DLH_BOOT_RUNNING                   =             0x03
} LIS331DLH_INT_Conf_t;

typedef enum {
  LIS331DLH_SLEEP_TO_WAKE_DIS              =             0x00,
  LIS331DLH_SLEEP_TO_WAKE_ENA              =             0x03,
} LIS331DLH_Sleep_To_Wake_Conf_t;

typedef enum {
  LIS331DLH_FULLSCALE_2                    =             0x00,
  LIS331DLH_FULLSCALE_4                    =             0x01,
  LIS331DLH_FULLSCALE_8                    =             0x03,
} LIS331DLH_Fullscale_t;

typedef enum {
  LIS331DLH_BLE_LSB                        =		0x00,
  LIS331DLH_BLE_MSB                        =		0x01
} LIS331DLH_Endianess_t;

typedef enum {
  LIS331DLH_SPI_4_WIRE                     =             0x00,
  LIS331DLH_SPI_3_WIRE                     =             0x01
} LIS331DLH_SPIMode_t;

typedef enum {
  LIS331DLH_X_ENABLE                       =             0x01,
  LIS331DLH_X_DISABLE                      =             0x00,
  LIS331DLH_Y_ENABLE                       =             0x02,
  LIS331DLH_Y_DISABLE                      =             0x00,
  LIS331DLH_Z_ENABLE                       =             0x04,
  LIS331DLH_Z_DISABLE                      =             0x00    
} LIS331DLH_AXISenable_t;

typedef enum {
  LIS331DLH_UP_SX                          =             0x44,
  LIS331DLH_UP_DX                          =             0x42,
  LIS331DLH_DW_SX                          =             0x41,
  LIS331DLH_DW_DX                          =             0x48,
  LIS331DLH_TOP                            =             0x60,
  LIS331DLH_BOTTOM                         =             0x50
} LIS331DLH_POSITION_6D_t;

typedef enum {
  LIS331DLH_INT_MODE_OR                    =             0x00,
  LIS331DLH_INT_MODE_6D_MOVEMENT           =             0x01,
  LIS331DLH_INT_MODE_AND                   =             0x02,
  LIS331DLH_INT_MODE_6D_POSITION           =             0x03  
} LIS331DLH_IntMode_t;


/* Exported constants --------------------------------------------------------*/

#ifndef __SHARED__CONSTANTS
#define __SHARED__CONSTANTS

#define MEMS_SET                                        0x01
#define MEMS_RESET                                      0x00

#endif /*__SHARED__CONSTANTS*/

#define LIS331DLH_MEMS_I2C_ADDRESS                       0x32

//Register and define

// The LIS331HH doesn't say anything about this register, so I guess
// we have to assume its not supported on it.  Too bad, it'd be a good
// starting point.  So far as I can tell from the LIS331HH datasheet all
// the other registers/bits are identical to those on the LIS331DLH.
//#define LIS331DLH_WHO_AM_I				0x0F  // device identification register

// CONTROL REGISTER 1 
#define LIS331DLH_CTRL_REG1       			0x20
#define LIS331DLH_PM				        BIT(5)
#define LIS331DLH_DR				        BIT(3)
#define LIS331DLH_ZEN					BIT(2)
#define LIS331DLH_YEN					BIT(1)
#define LIS331DLH_XEN					BIT(0)

//CONTROL REGISTER 2 
#define LIS331DLH_CTRL_REG2				0x21
#define LIS331DLH_BOOT                                   BIT(7)
#define LIS331DLH_HPM     				BIT(5)
#define LIS331DLH_FDS     				BIT(4)
#define LIS331DLH_HPEN2					BIT(3)
#define LIS331DLH_HPEN1					BIT(2)
#define LIS331DLH_HPCF					BIT(0)

//CONTROL REGISTER 3 
#define LIS331DLH_CTRL_REG3				0x22
#define LIS331DLH_IHL                                    BIT(7)
#define LIS331DLH_PP_OD					BIT(6)
#define LIS331DLH_LIR2				        BIT(5)
#define LIS331DLH_I2_CFG  				BIT(3)
#define LIS331DLH_LIR1    				BIT(2)
#define LIS331DLH_I1_CFG  				BIT(0)

//CONTROL REGISTER 4
#define LIS331DLH_CTRL_REG4				0x23
#define LIS331DLH_BDU					BIT(7)
#define LIS331DLH_BLE					BIT(6)
#define LIS331DLH_FS					BIT(4)
#define LIS331DLH_ST_SIGN				BIT(3)
#define LIS331DLH_ST       				BIT(1)
#define LIS331DLH_SIM					BIT(0)

//CONTROL REGISTER 5
#define LIS331DLH_CTRL_REG5       			0x24
#define LIS331DLH_TURN_ON                                BIT(0)

#define LIS331DLH_HP_FILTER_RESET			0x25

//REFERENCE/DATA_CAPTURE
#define LIS331DLH_REFERENCE_REG		                0x26
#define LIS331DLH_REF		                	BIT(0)

//STATUS_REG_AXIES 
#define LIS331DLH_STATUS_REG				0x27

//INTERRUPT 1 CONFIGURATION 
#define LIS331DLH_INT1_CFG				0x30

//INTERRUPT 2 CONFIGURATION 
#define LIS331DLH_INT2_CFG				0x34
#define LIS331DLH_ANDOR                                  BIT(7)
#define LIS331DLH_INT_6D                                 BIT(6)

//INT REGISTERS 
#define LIS331DLH_INT1_THS                               0x32
#define LIS331DLH_INT1_DURATION                          0x33
#define LIS331DLH_INT2_THS                               0x36
#define LIS331DLH_INT2_DURATION                          0x37

//INTERRUPT 1 SOURCE REGISTER 
#define LIS331DLH_INT1_SRC                               0x31
#define LIS331DLH_INT2_SRC			        0x35

//INT_CFG  bit mask
#define LIS331DLH_INT_AND                                0x80
#define LIS331DLH_INT_OR                                 0x00
#define LIS331DLH_INT_ZHIE_ENABLE                        0x20
#define LIS331DLH_INT_ZHIE_DISABLE                       0x00
#define LIS331DLH_INT_ZLIE_ENABLE                        0x10
#define LIS331DLH_INT_ZLIE_DISABLE                       0x00
#define LIS331DLH_INT_YHIE_ENABLE                        0x08
#define LIS331DLH_INT_YHIE_DISABLE                       0x00
#define LIS331DLH_INT_YLIE_ENABLE                        0x04
#define LIS331DLH_INT_YLIE_DISABLE                       0x00
#define LIS331DLH_INT_XHIE_ENABLE                        0x02
#define LIS331DLH_INT_XHIE_DISABLE                       0x00
#define LIS331DLH_INT_XLIE_ENABLE                        0x01
#define LIS331DLH_INT_XLIE_DISABLE                       0x00

//INT_SRC  bit mask
#define LIS331DLH_INT_SRC_IA                             0x40
#define LIS331DLH_INT_SRC_ZH                             0x20
#define LIS331DLH_INT_SRC_ZL                             0x10
#define LIS331DLH_INT_SRC_YH                             0x08
#define LIS331DLH_INT_SRC_YL                             0x04
#define LIS331DLH_INT_SRC_XH                             0x02
#define LIS331DLH_INT_SRC_XL                             0x01

//OUTPUT REGISTER
#define LIS331DLH_OUT_X_L                                0x28
#define LIS331DLH_OUT_X_H                                0x29
#define LIS331DLH_OUT_Y_L			        0x2A
#define LIS331DLH_OUT_Y_H		                0x2B
#define LIS331DLH_OUT_Z_L			        0x2C
#define LIS331DLH_OUT_Z_H		                0x2D

//STATUS REGISTER bit mask
#define LIS331DLH_STATUS_REG_ZYXOR                       0x80    // 1	:	new data set has over written the previous one
						                // 0	:	no overrun has occurred (default)	
#define LIS331DLH_STATUS_REG_ZOR                         0x40    // 0	:	no overrun has occurred (default)
							        // 1	:	new Z-axis data has over written the previous one
#define LIS331DLH_STATUS_REG_YOR                         0x20    // 0	:	no overrun has occurred (default)
						        	// 1	:	new Y-axis data has over written the previous one
#define LIS331DLH_STATUS_REG_XOR                         0x10    // 0	:	no overrun has occurred (default)
							        // 1	:	new X-axis data has over written the previous one
#define LIS331DLH_STATUS_REG_ZYXDA                       0x08    // 0	:	a new set of data is not yet avvious one
                                                                // 1	:	a new set of data is available 
#define LIS331DLH_STATUS_REG_ZDA                         0x04    // 0	:	a new data for the Z-Axis is not availvious one
                                                                // 1	:	a new data for the Z-Axis is available
#define LIS331DLH_STATUS_REG_YDA                         0x02    // 0	:	a new data for the Y-Axis is not available
                                                                // 1	:	a new data for the Y-Axis is available
#define LIS331DLH_STATUS_REG_XDA                         0x01    // 0	:	a new data for the X-Axis is not available
                                                                // 1	:	a new data for the X-Axis is available
#define LIS331DLH_DATAREADY_BIT                          LIS331DLH_STATUS_REG_ZYXDA



/* Exported macro ------------------------------------------------------------*/

#ifndef __SHARED__MACROS

#define __SHARED__MACROS
#define ValBit(VAR,Place)         (VAR & (1<<Place))
#define BIT(x) ( (x) )

#endif /*__SHARED__MACROS*/

/* Exported functions --------------------------------------------------------*/

//Sensor Configuration Functions
status_t LIS331DLH_GetWHO_AM_I(u8_t* val);
status_t LIS331DLH_SetODR(LIS331DLH_ODR_t dr);
status_t LIS331DLH_SetMode(LIS331DLH_Mode_t pm);
status_t LIS331DLH_SetAxis(LIS331DLH_Axis_t axis);
status_t LIS331DLH_SetFullScale(LIS331DLH_Fullscale_t fs);
status_t LIS331DLH_SetBDU(State_t bdu);
status_t LIS331DLH_SetBLE(LIS331DLH_Endianess_t ble);
status_t LIS331DLH_SetSelfTest(State_t st);
status_t LIS331DLH_SetSelfTestSign(State_t st_sign);
status_t LIS331DLH_TurnONEnable(LIS331DLH_Sleep_To_Wake_Conf_t stw);
status_t LIS331DLH_SetBOOT(State_t boot);
status_t LIS331DLH_SetFDS(State_t fds);
status_t LIS331DLH_SetSPI34Wire(LIS331DLH_SPIMode_t sim);

//Filtering Functions
status_t LIS331DLH_SetHPFMode(LIS331DLH_HPFMode_t hpm);
status_t LIS331DLH_SetHPFCutOFF(LIS331DLH_HPFCutOffFreq_t hpf);
status_t LIS331DLH_SetFilterDataSel(State_t state);
status_t LIS331DLH_SetReference(i8_t ref);

//Interrupt Functions
status_t LIS331DLH_SetIntHighLow(State_t hil);
status_t LIS331DLH_SetIntPPOD(State_t pp_od);
status_t LIS331DLH_SetInt1DataSign(LIS331DLH_INT_Conf_t i_cfg);
status_t LIS331DLH_SetInt2DataSign(LIS331DLH_INT_Conf_t i_cfg);
status_t LIS331DLH_SetInt1HPEnable(State_t stat);
status_t LIS331DLH_SetInt2HPEnable(State_t stat);
status_t LIS331DLH_Int1LatchEnable(State_t latch);
status_t LIS331DLH_Int2LatchEnable(State_t latch);
status_t LIS331DLH_ResetInt1Latch(void);
status_t LIS331DLH_ResetInt2Latch(void);
status_t LIS331DLH_SetInt1Configuration(LIS331DLH_IntConf_t ic);
status_t LIS331DLH_SetInt2Configuration(LIS331DLH_IntConf_t ic);
status_t LIS331DLH_SetInt1Threshold(u8_t ths);
status_t LIS331DLH_SetInt2Threshold(u8_t ths);
status_t LIS331DLH_SetInt1Duration(u8_t id);
status_t LIS331DLH_SetInt2Duration(u8_t id);
status_t LIS331DLH_SetInt1Mode(LIS331DLH_IntMode_t int_mode);
status_t LIS331DLH_SetInt2Mode(LIS331DLH_IntMode_t int_mode);
status_t LIS331DLH_GetInt1Src(u8_t* val);
status_t LIS331DLH_GetInt2Src(u8_t* val);
status_t LIS331DLH_GetInt1SrcBit(u8_t statusBIT, u8_t* val);
status_t LIS331DLH_GetInt2SrcBit(u8_t statusBIT, u8_t* val); 

//Other Reading Functions
status_t LIS331DLH_GetStatusReg(u8_t* val);
status_t LIS331DLH_GetStatusBit(u8_t statusBIT, u8_t* val);
status_t LIS331DLH_GetAccAxesRaw(AxesRaw_t* buff);
status_t LIS331DLH_Get6DPositionInt1(u8_t* val);
status_t LIS331DLH_Get6DPositionInt2(u8_t* val);

//Generic
// i.e. u8_t LIS331DLH_ReadReg(u8_t deviceAddr, u8_t Reg, u8_t* Data);
// i.e. u8_t LIS331DLH_WriteReg(u8_t deviceAddress, u8_t WriteAddr, u8_t Data); 
u8_t LIS331DLH_ReadReg(u8_t deviceAddr, u8_t Reg, u8_t* Data);
u8_t LIS331DLH_WriteReg(u8_t deviceAddress, u8_t WriteAddr, u8_t Data);

#endif /*__LIS331DLH_H */

/******************* (C) COPYRIGHT 2012 STMicroelectronics *****END OF FILE****/



