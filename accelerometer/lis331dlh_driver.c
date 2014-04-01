// Turns out ST Microelectronics already provides an almost-finished interface
// for the LIS331DLH, which is an almost identical device: the WHO_AM_I
// register document for that device isn't documented for the LIS331HH,
// but thats the only difference I've notice in the datasheet (besides the
// different pin out and dynmic ranges and sensitivities of the devices,
// which aren't relevant to this interface).
//
// All we have to do is fill in the in the LIS331DLH_ReadReg() and
// LIS331DLH_WriteReg() functions in lis331dlh_driver.c.  Note that the
// accelerometer_init() function in accelerometer.h must still be called
// first to ensure that SPI is set up correctly, so that our implementation
// of these routines works.

/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
* File Name          : lis331dlh_driver.c
* Author             : MSH Application Team
* Author             : Abhishek Anand	
* Version            : $Revision:$
* Date               : $Date:$
* Description        : LIS331DLH driver file
*                      
* HISTORY:
* Date               |	Modification                    |	Author
* 16/05/2012         |	Initial Revision                |	Abhishek Anand
* 17/05/2012         |  Modified to support multiple drivers in the same program                |	Abhishek Anand

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
#include "lis331dlh_driver.h"
#include "spi.h"

// SPI communication uses PB5, so we rewire our debugging macros to use a
// LED on a different pin.
#ifdef CHKP
#  undef CHKP
#  define CHKP() CHKP_USING(DDRD, DDD2, PORTD, PORTD2, 300.0, 3)
#endif
#ifdef BTRAP
#  undef BTRAP
#  define BTRAP() BTRAP_USING(DDRD, DDD2, PORTD, PORTD2, 100.0)
#endif

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/*******************************************************************************
* Function Name		: LIS331DLH_ReadReg
* Description		: Generic Reading function. It must be fullfilled with either
*			: I2C or SPI reading functions					
* Input			: Register Address
* Output		: Data Read
* Return		: Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/

// SPI-related macros as described in spi.h.
#define MY_SPI_SLAVE_ACCELEROMETER_SELECT_SET_LOW SPI_SS_SET_LOW
#define MY_SPI_SLAVE_ACCELEROMETER_SELECT_SET_HIGH SPI_SS_SET_HIGH

u8_t LIS331DLH_ReadReg(u8_t deviceAddr, u8_t Reg, u8_t* Data) {
  
  //To be completed with either I2c or SPI reading function
  // i.e. *Data = SPI_Mems_Read_Reg( Reg );
  // i.e. if(!I2C_BufferRead(Data, deviceAddr, Reg, 1)) 
  // return MEMS_ERROR;
  // else  
  // return MEMS_SUCCESS;

  // Our implementation of the pseudocode above follows.  Note that
  // accelerometer_init() declared in accelerometer.h must be called to
  // ensure that SPI is initialized before this implementation can be used.

  // In our case the SPI slave device is selected using a particular hardware
  // line that is configured at compile time using macros, so we don't need
  // this argument.
  deviceAddr = deviceAddr;

  MY_SPI_SLAVE_ACCELEROMETER_SELECT_SET_LOW ();
  {
    // NOTE: the datasheet seems to imply that we could read a stream of
    // register values sequentially.  This would be a tiny bit faster since
    // both "directions" of each spi_transfer() call could be used (except
    // for the first byte of the request and last byte of the result).
    // But this interface doesn't seem to do it, so we're sure not going to
    // worry about it.
    spi_transfer (B10000000 | Reg);
    uint8_t dont_care_value = 0x00;
    *Data = spi_transfer (dont_care_value);
  }
  MY_SPI_SLAVE_ACCELEROMETER_SELECT_SET_HIGH ();

  // The SPI interface which we inherited from the Arduino project doesn't
  // provide any possibility for error so far as I know, so we always report
  // success here :)
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name		: LIS331DLH_WriteReg
* Description		: Generic Writing function. It must be fullfilled with either
*			: I2C or SPI writing function
* Input			: Register Address, Data to be written
* Output		: None
* Return		: Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
u8_t LIS331DLH_WriteReg(u8_t deviceAddress, u8_t WriteAddr, u8_t Data) {
  
  //To be completed with either I2c or SPI writing function
  // i.e. SPI_Mems_Write_Reg(Reg, Data);
  // i.e. I2C_ByteWrite(&Data,  deviceAddress,  WriteAddr);  
  //  return MEMS_SUCCESS;
  
  // Our implementation of the pseudocode above follows.  Note that
  // accelerometer_init() declared in accelerometer.h must be called to
  // ensure that SPI is initialized before this implementation can be used.

  // In our case the SPI slave device is selected using a particular hardware
  // line that is configured at compile time using macros, so we don't need
  // this argument.
  deviceAddress = deviceAddress;

  MY_SPI_SLAVE_ACCELEROMETER_SELECT_SET_LOW ();
  {
    // NOTE: looks like they change Reg to WriteAddr between the time the
    // comments were written and the time the prototype was written.
    spi_transfer (WriteAddr);
    spi_transfer (Data);
  }
  MY_SPI_SLAVE_ACCELEROMETER_SELECT_SET_HIGH ();

  // The SPI interface which we inherited from the Arduino project doesn't
  // provide any possibility for error so far as I know, so we always report
  // success here :)
  return MEMS_SUCCESS;
}

/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
* Function Name  : LIS331DLH_GetWHO_AM_I
* Description    : Read identification code from LIS331DLH_WHO_AM_I register
* Input          : char to be filled with the Device identification Value
* Output         : None
* Return         : Status [value of FSS]
*******************************************************************************/
// This probably doesn't work on the LIS331HH, though it would be nice and
// should work fine on the LIS331DHL.  But since LIS331HH is the part I have
// to play with I'm disabling it.
/*
status_t LIS331DLH_GetWHO_AM_I(u8_t* val){
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_WHO_AM_I, val) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}
*/


/*******************************************************************************
* Function Name  : LIS331DLH_SetODR
* Description    : Sets LIS331DLH Accelerometer Output Data Rate 
* Input          : Output Data Rate
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetODR(LIS331DLH_ODR_t dr){
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG1, &value) )
    return MEMS_ERROR;
  
  value &= 0xE7;
  value |= dr<<LIS331DLH_DR;
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG1, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetMode
* Description    : Sets LIS331DLH Accelerometer Operating Mode
* Input          : Modality (LIS331DLH_LOW_POWER, LIS331DLH_NORMAL, LIS331DLH_POWER_DOWN...)
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetMode(LIS331DLH_Mode_t pm) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG1, &value) )
    return MEMS_ERROR;
  
  value &= 0x1F;
  value |= (pm<<LIS331DLH_PM);   
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG1, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}

/*******************************************************************************
* Function Name  : LIS331DLH_SetAxis
* Description    : Enable/Disable LIS331DLH Axis
* Input          : LIS331DLH_X_ENABLE/LIS331DLH_X_DISABLE | LIS331DLH_Y_ENABLE/LIS331DLH_Y_DISABLE
                   | LIS331DLH_Z_ENABLE/LIS331DLH_Z_DISABLE
* Output         : None
* Note           : You MUST use all input variable in the argument, as example
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetAxis(LIS331DLH_Axis_t axis) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG1, &value) )
    return MEMS_ERROR;
  
  value &= 0xF8;
  value |= (0x07 & axis);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG1, value) )
    return MEMS_ERROR;   
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetFullScale
* Description    : Sets the LIS331DLH FullScale
* Input          : LIS331DLH_FULLSCALE_2/LIS331DLH_FULLSCALE_4/LIS331DLH_FULLSCALE_8
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetFullScale(LIS331DLH_Fullscale_t fs) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG4, &value) )
    return MEMS_ERROR;
  
  value &= 0xCF;	
  value |= (fs<<LIS331DLH_FS);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG4, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}

// Analogous to LIS331DLH_SetFullScale().
status_t LIS331HH_SetFullScale(LIS331HH_Fullscale_t fs) {
  // Values of type LIS331HH_Fullscale_t can be used where values
  // of type LIS331DLH_Fullscale_t are called for, so we can use
  // LIS331DLH_SetFullScale() to implement this function.
  return LIS331DLH_SetFullScale (fs);
}

/*******************************************************************************
* Function Name  : LIS331DLH_SetBDU
* Description    : Enable/Disable Block Data Update Functionality
* Input          : MEMS_ENABLE/MEMS_DISABLE
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetBDU(State_t bdu) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG4, &value) )
    return MEMS_ERROR;
  
  value &= 0x7F;
  value |= (bdu<<LIS331DLH_BDU);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG4, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetBLE
* Description    : Set Endianess (MSB/LSB)
* Input          : LIS331DLH_BLE_LSB / LIS331DLH_BLE_MSB
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetBLE(LIS331DLH_Endianess_t ble) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG4, &value) )
    return MEMS_ERROR;
  
  value &= 0xBF;	
  value |= (ble<<LIS331DLH_BLE);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG4, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetFDS
* Description    : Set Filter Data Selection
* Input          : MEMS_ENABLE/MEMS_DISABLE
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetFDS(State_t fds) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG2, &value) )
    return MEMS_ERROR;
  
  value &= 0xEF;	
  value |= (fds<<LIS331DLH_FDS);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG2, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetBOOT
* Description    : Rebot memory content
* Input          : MEMS_ENABLE/MEMS_DISABLE
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetBOOT(State_t boot) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG2, &value) )
    return MEMS_ERROR;
  
  value &= 0x7F;	
  value |= (boot<<LIS331DLH_BOOT);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG2, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetSelfTest
* Description    : Set Self Test Modality
* Input          : MEMS_DISABLE/MEMS_ENABLE
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetSelfTest(State_t st) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG4, &value) )
    return MEMS_ERROR;
  
  value &= 0xFD;
  value |= (st<<LIS331DLH_ST);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG4, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetSelfTestSign
* Description    : Set Self Test Sign (Disable = st_plus, Enable = st_minus)
* Input          : MEMS_DISABLE/MEMS_ENABLE
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetSelfTestSign(State_t st_sign) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG4, &value) )
    return MEMS_ERROR;
  
  value &= 0xF7;
  value |= (st_sign<<LIS331DLH_ST_SIGN);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG4, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetIntHighLow
* Description    : Set Interrupt active state (Disable = active high, Enable = active low)
* Input          : MEMS_DISABLE/MEMS_ENABLE
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetIntHighLow(State_t ihl) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG3, &value) )
    return MEMS_ERROR;
  
  value &= 0x7F;
  value |= (ihl<<LIS331DLH_IHL);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG3, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetIntPPOD
* Description    : Set Interrupt Push-Pull/OpenDrain Pad (Disable = Push-Pull, Enable = OpenDrain)
* Input          : MEMS_DISABLE/MEMS_ENABLE
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetIntPPOD(State_t pp_od) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG3, &value) )
    return MEMS_ERROR;
  
  value &= 0xBF;
  value |= (pp_od<<LIS331DLH_PP_OD);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG3, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetInt1DataSign
* Description    : Set Data signal Interrupt 1 pad
* Input          : Modality by LIS331DLH_INT_Conf_t Typedef 
                  (LIS331DLH_INT_SOURCE, LIS331DLH_INT_1OR2_SOURCE, LIS331DLH_DATA_READY, LIS331DLH_BOOT_RUNNING)
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetInt1DataSign(LIS331DLH_INT_Conf_t i_cfg) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG3, &value) )
    return MEMS_ERROR;
  
  value &= 0xFC;
  value |= (i_cfg<<LIS331DLH_I1_CFG);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG3, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetInt2DataSign
* Description    : Set Data signal Interrupt 2 pad
* Input          : Modality by LIS331DLH_INT_Conf_t Typedef 
                  (LIS331DLH_INT_SOURCE, LIS331DLH_INT_1OR2_SOURCE, LIS331DLH_DATA_READY, LIS331DLH_BOOT_RUNNING)
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetInt2DataSign(LIS331DLH_INT_Conf_t i_cfg) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG3, &value) )
    return MEMS_ERROR;
  
  value &= 0xE7;
  value |= (i_cfg<<LIS331DLH_I2_CFG);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG3, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetSPI34Wire
* Description    : Set SPI mode 
* Input          : Modality by LIS331DLH_SPIMode_t Typedef (LIS331DLH_SPI_4_WIRE, LIS331DLH_SPI_3_WIRE)
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetSPI34Wire(LIS331DLH_SPIMode_t sim) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG4, &value) )
    return MEMS_ERROR;
  
  value &= 0xFE;
  value |= (sim<<LIS331DLH_SIM);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG4, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_TurnONEnable
* Description    : TurnON Mode selection for sleep to wake function
* Input          : LIS331DLH_SLEEP_TO_WAKE_DIS/LIS331DLH_SLEEP_TO_WAKE_ENA
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_TurnONEnable(LIS331DLH_Sleep_To_Wake_Conf_t stw) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG5, &value) )
    return MEMS_ERROR;
  
  value &= 0x00;
  value |= (stw<<LIS331DLH_TURN_ON);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG5, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_HPFilterReset
* Description    : Reading register for reset the content of internal HP filter
* Input          : None
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/

// This function didn't have a prototype in lis331dlh_driver.h, so I'm
// commenting it out.  Given its signature, I think the default signature
// thingy C assumes of undeclared functions would probably work, so it may
// have been working despite the lack of a prototype.  But it does something
// weird that I don't care about anyway so I'm playing it safe and taking
// it out.
/*
status_t LIS331DLH_HPFilterReset(void) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_HP_FILTER_RESET, &value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}
*/


/*******************************************************************************
* Function Name  : LIS331DLH_SetReference
* Description    : Sets Reference register acceleration value as a reference for HP filter
* Input          : Value of reference acceleration value (0-255)
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetReference(i8_t ref) {
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_REFERENCE_REG, ref) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetHPFMode
* Description    : Set High Pass Filter Modality
* Input          : LIS331DLH_HPM_NORMAL_MODE_RES/LIS331DLH_HPM_REF_SIGNAL/LIS331DLH_HPM_NORMAL_MODE
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetHPFMode(LIS331DLH_HPFMode_t hpm) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG2, &value) )
    return MEMS_ERROR;
  
  value &= 0x9F;
  value |= (hpm<<LIS331DLH_HPM);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG2, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetHPFCutOFF
* Description    : Set High Pass CUT OFF Freq
* Input          : LIS331DLH_HPFCF [0,3]
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetHPFCutOFF(LIS331DLH_HPFCutOffFreq_t hpf) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG2, &value) )
    return MEMS_ERROR;
  
  value &= 0xFC;
  value |= (hpf<<LIS331DLH_HPCF);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG2, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
  
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetInt2HPEnable
* Description    : Set Interrupt2 hp filter enable/disable
* Input          : MEMS_ENABLE/MEMS_DISABLE
* example        : LIS331DLH_SetInt2HPEnable(MEMS_ENABLE)
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetInt2HPEnable(State_t stat) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG2, &value) )
    return MEMS_ERROR;
  
  value &= 0xF7;
  value |= stat<<LIS331DLH_HPEN2 ;
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG2, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}     


/*******************************************************************************
* Function Name  : LIS331DLH_SetInt1HPEnable
* Description    : Set Interrupt1 hp filter enable/disable
* Input          : MEMS_ENABLE/MEMS_DISABLE
* example        : LIS331DLH_SetInt1HPEnable(MEMS_ENABLE)
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetInt1HPEnable(State_t stat) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG2, &value) )
    return MEMS_ERROR;
  
  value &= 0xFB;
  value |= stat<<LIS331DLH_HPEN1 ;
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG2, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}  


/*******************************************************************************
* Function Name  : LIS331DLH_Int1LatchEnable
* Description    : Enable Interrupt 1 Latching function
* Input          : ENABLE/DISABLE
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_Int1LatchEnable(State_t latch) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG3, &value) )
    return MEMS_ERROR;
  
  value &= 0xFB;
  value |= latch<<LIS331DLH_LIR1;
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG3, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_Int2LatchEnable
* Description    : Enable Interrupt 2 Latching function
* Input          : ENABLE/DISABLE
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_Int2LatchEnable(State_t latch) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG3, &value) )
    return MEMS_ERROR;
  
  value &= 0xDF;
  value |= latch<<LIS331DLH_LIR2;
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG3, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_ResetInt1Latch
* Description    : Reset Interrupt 1 Latching function
* Input          : None
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_ResetInt1Latch(void) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT1_SRC, &value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_ResetInt2Latch
* Description    : Reset Interrupt 2 Latching function
* Input          : None
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_ResetInt2Latch(void) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT2_SRC, &value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetInt1Configuration
* Description    : Interrupt 1 Configuration (without 6D_INT)
* Input          : LIS331DLH_INT_AND/OR | LIS331DLH_INT_ZHIE_ENABLE/DISABLE | LIS331DLH_INT_ZLIE_ENABLE/DISABLE...
* Output         : None
* Note           : You MUST use ALL input variable in the argument, as in example above
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetInt1Configuration(LIS331DLH_IntConf_t ic) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT1_CFG, &value) )
    return MEMS_ERROR;
  
  value &= 0x40; 
  value |= ic;
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT1_CFG, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetInt2Configuration
* Description    : Interrupt 2 Configuration (without 6D_INT)
* Input          : LIS331DLH_INT_AND/OR | LIS331DLH_INT_ZHIE_ENABLE/DISABLE | LIS331DLH_INT_ZLIE_ENABLE/DISABLE...
* Output         : None
* Note           : You MUST use all input variable in the argument, as example
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetInt2Configuration(LIS331DLH_IntConf_t ic) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT2_CFG, &value) )
    return MEMS_ERROR;
  
  value &= 0x40; 
  value |= ic;
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT2_CFG, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetInt1Mode
* Description    : Interrupt 1 Configuration mode (OR, 6D Movement, AND, 6D Position)
* Input          : LIS331DLH_INT_MODE_OR, LIS331DLH_INT_MODE_6D_MOVEMENT, LIS331DLH_INT_MODE_AND, LIS331DLH_INT_MODE_6D_POSITION
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetInt1Mode(LIS331DLH_IntMode_t int_mode) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT1_CFG, &value) )
    return MEMS_ERROR;
  
  value &= 0x3F; 
  value |= (int_mode<<LIS331DLH_INT_6D);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT1_CFG, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetInt2Mode
* Description    : Interrupt 2 Configuration mode (OR, 6D Movement, AND, 6D Position)
* Input          : LIS331DLH_INT_MODE_OR, LIS331DLH_INT_MODE_6D_MOVEMENT, LIS331DLH_INT_MODE_AND, LIS331DLH_INT_MODE_6D_POSITION
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetInt2Mode(LIS331DLH_IntMode_t int_mode) {
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT2_CFG, &value) )
    return MEMS_ERROR;
  
  value &= 0x3F; 
  value |= (int_mode<<LIS331DLH_INT_6D);
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT2_CFG, value) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_Get6DPositionInt1
* Description    : 6D Interrupt 1 Position Detect
* Input          : Byte to be filled with LIS331DLH_POSITION_6D_t Typedef
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_Get6DPositionInt1(u8_t* val){
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT1_SRC, &value) )
    return MEMS_ERROR;
  
  value &= 0x7F;
  
  switch (value){
  case LIS331DLH_UP_SX:   
    *val = LIS331DLH_UP_SX;    
    break;
  case LIS331DLH_UP_DX:   
    *val = LIS331DLH_UP_DX;    
    break;
  case LIS331DLH_DW_SX:   
    *val = LIS331DLH_DW_SX;    
    break;
  case LIS331DLH_DW_DX:   
    *val = LIS331DLH_DW_DX;    
    break;
  case LIS331DLH_TOP:     
    *val = LIS331DLH_TOP;      
    break;
  case LIS331DLH_BOTTOM:  
    *val = LIS331DLH_BOTTOM;  
    break;
  }
  
  return MEMS_SUCCESS;  
}


/*******************************************************************************
* Function Name  : LIS331DLH_Get6DPositionInt2
* Description    : 6D Interrupt 2 Position Detect
* Input          : Byte to be filled with LIS331DLH_POSITION_6D_t Typedef
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_Get6DPositionInt2(u8_t* val){
  u8_t value;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT2_SRC, &value) )
    return MEMS_ERROR;
  
  value &= 0x7F;
  
  switch (value){
  case LIS331DLH_UP_SX:   
    *val = LIS331DLH_UP_SX;    
    break;
  case LIS331DLH_UP_DX:   
    *val = LIS331DLH_UP_DX;    
    break;
  case LIS331DLH_DW_SX:   
    *val = LIS331DLH_DW_SX;    
    break;
  case LIS331DLH_DW_DX:   
    *val = LIS331DLH_DW_DX;    
    break;
  case LIS331DLH_TOP:     
    *val = LIS331DLH_TOP;      
    break;
  case LIS331DLH_BOTTOM:  
    *val = LIS331DLH_BOTTOM;   
    break;
  }
  
  return MEMS_SUCCESS;  
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetInt1Threshold
* Description    : Sets Interrupt 1 Threshold
* Input          : Threshold = [0,127]
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetInt1Threshold(u8_t ths) {
  if (ths > 127)
    return MEMS_ERROR;
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT1_THS, ths) )
    return MEMS_ERROR;    
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetInt1Duration
* Description    : Sets Interrupt 1 Duration
* Input          : Duration = [0,127]
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetInt1Duration(u8_t id) {  
  if (id > 127)
    return MEMS_ERROR;
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT1_DURATION, id) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetInt2Threshold
* Description    : Sets Interrupt 2 Threshold
* Input          : Threshold = [0,127]
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetInt2Threshold(u8_t ths) {
  if (ths > 127)
    return MEMS_ERROR;
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT2_THS, ths) )
    return MEMS_ERROR;    
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_SetInt2Duration
* Description    : Sets Interrupt 2 Duration
* Input          : Duration = [0,127]
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_SetInt2Duration(u8_t id) {  
  if (id > 127)
    return MEMS_ERROR;
  
  if( !LIS331DLH_WriteReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT2_DURATION, id) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_GetStatusReg
* Description    : Read the status register
* Input          : char to empty by Status Reg Value
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_GetStatusReg(u8_t* val) {
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_STATUS_REG, val) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;  
}

/*******************************************************************************
* Function Name  : LIS331DLH_GetStatusBIT
* Description    : Read the status register BIT
* Input          : LIS331DLH_STATUS_REG_ZYXOR, LIS331DLH_STATUS_REG_ZOR, LIS331DLH_STATUS_REG_YOR, LIS331DLH_STATUS_REG_XOR,
                   LIS331DLH_STATUS_REG_ZYXDA, LIS331DLH_STATUS_REG_ZDA, LIS331DLH_STATUS_REG_YDA, LIS331DLH_STATUS_REG_XDA, 
                   LIS331DLH_DATAREADY_BIT
* Output         : status register BIT
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_GetStatusBit(u8_t statusBIT, u8_t *val) {
  u8_t value;  
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_STATUS_REG, &value) )
    return MEMS_ERROR;
  
  switch (statusBIT){
  case LIS331DLH_STATUS_REG_ZYXOR:     
    if(value &= LIS331DLH_STATUS_REG_ZYXOR){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }  
  case LIS331DLH_STATUS_REG_ZOR:       
    if(value &= LIS331DLH_STATUS_REG_ZOR){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }  
  case LIS331DLH_STATUS_REG_YOR:       
    if(value &= LIS331DLH_STATUS_REG_YOR){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }                                 
  case LIS331DLH_STATUS_REG_XOR:       
    if(value &= LIS331DLH_STATUS_REG_XOR){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }  
  case LIS331DLH_STATUS_REG_ZYXDA:     
    if(value &= LIS331DLH_STATUS_REG_ZYXDA){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }   
  case LIS331DLH_STATUS_REG_ZDA:       
    if(value &= LIS331DLH_STATUS_REG_ZDA){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }   
  case LIS331DLH_STATUS_REG_YDA:       
    if(value &= LIS331DLH_STATUS_REG_YDA){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }   
  case LIS331DLH_STATUS_REG_XDA:       
    if(value &= LIS331DLH_STATUS_REG_XDA){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }                                      
  }
  
  return MEMS_ERROR;
}


/*******************************************************************************
* Function Name  : LIS331DLH_GetAccAxesRaw
* Description    : Read the Acceleration Values Output Registers
* Input          : buffer to empity by AccAxesRaw_t Typedef
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_GetAccAxesRaw(AxesRaw_t* buff) {
  i16_t value;
  u8_t *valueL = (u8_t *)(&value);
  u8_t *valueH = ((u8_t *)(&value)+1);
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_OUT_X_L, valueL) )
    return MEMS_ERROR;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_OUT_X_H, valueH) )
    return MEMS_ERROR;
  
  buff->AXIS_X = value/16;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_OUT_Y_L, valueL) )
    return MEMS_ERROR;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_OUT_Y_H, valueH) )
    return MEMS_ERROR;
  
  buff->AXIS_Y = value/16;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_OUT_Z_L, valueL) )
    return MEMS_ERROR;
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_OUT_Z_H, valueH) )
    return MEMS_ERROR;
  
  buff->AXIS_Z = value/16;
  
  return MEMS_SUCCESS;  
}


/*******************************************************************************
* Function Name  : LIS331DLH_GetInt1Src
* Description    : Reset Interrupt 1 Latching function
* Input          : buffer to empty by Int1 Source Value
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_GetInt1Src(u8_t* val) {  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT1_SRC, val) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_GetInt2Src
* Description    : Reset Interrupt 2 Latching function
* Input          : buffer to empty by Int2 Source Value
* Output         : None
* Return         : Status [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_GetInt2Src(u8_t* val) {  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT2_SRC, val) )
    return MEMS_ERROR;
  
  return MEMS_SUCCESS;
}


/*******************************************************************************
* Function Name  : LIS331DLH_GetInt1SrcBit
* Description    : Reset Interrupt 1 Latching function
* Input          : LIS331DLH_INT1_SRC_IA, LIS331DLH_INT1_SRC_ZH, LIS331DLH_INT1_SRC_ZL .....
* Output         : None
* Return         : Status of BIT [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_GetInt1SrcBit(u8_t statusBIT, u8_t *val) {
  u8_t value;  
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT1_SRC, &value) )
    return MEMS_ERROR;
  
  if(statusBIT == LIS331DLH_INT_SRC_IA){
    if(value &= LIS331DLH_INT_SRC_IA){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }    
  }
  
  if(statusBIT == LIS331DLH_INT_SRC_ZH){
    if(value &= LIS331DLH_INT_SRC_ZH){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }   
  }
  
  if(statusBIT == LIS331DLH_INT_SRC_ZL){
    if(value &= LIS331DLH_INT_SRC_ZL){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }   
  }
  
  if(statusBIT == LIS331DLH_INT_SRC_YH){
    if(value &= LIS331DLH_INT_SRC_YH){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }   
  }
  
  if(statusBIT == LIS331DLH_INT_SRC_YL){
    if(value &= LIS331DLH_INT_SRC_YL){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }  
  }
  
  if(statusBIT == LIS331DLH_INT_SRC_XH){
    if(value &= LIS331DLH_INT_SRC_XH){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }   
  }
  
  if(statusBIT == LIS331DLH_INT_SRC_XL){
    if(value &= LIS331DLH_INT_SRC_XL){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }    
  } 
  return MEMS_ERROR;
}


/*******************************************************************************
* Function Name  : LIS331DLH_GetInt2SrcBit
* Description    : Reset Interrupt 2 Latching function
* Input          : LIS331DLH_INT_SRC_IA, LIS331DLH_INT_SRC_ZH, LIS331DLH_INT_SRC_ZL .....
* Output         : None
* Return         : Status of BIT [MEMS_ERROR, MEMS_SUCCESS]
*******************************************************************************/
status_t LIS331DLH_GetInt2SrcBit(u8_t statusBIT, u8_t *val) {
  u8_t value;  
  
  if( !LIS331DLH_ReadReg(LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_INT2_SRC, &value) )
    return MEMS_ERROR;
  
  if(statusBIT == LIS331DLH_INT_SRC_IA){
    if(value &= LIS331DLH_INT_SRC_IA){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }   
  }
  
  if(statusBIT == LIS331DLH_INT_SRC_ZH){
    if(value &= LIS331DLH_INT_SRC_ZH){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }  
  }
  
  if(statusBIT == LIS331DLH_INT_SRC_ZL){
    if(value &= LIS331DLH_INT_SRC_ZL){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }  
  }
  
  if(statusBIT == LIS331DLH_INT_SRC_YH){
    if(value &= LIS331DLH_INT_SRC_YH){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }    
  }
  
  if(statusBIT == LIS331DLH_INT_SRC_YL){
    if(value &= LIS331DLH_INT_SRC_YL){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }   
  }
  
  if(statusBIT == LIS331DLH_INT_SRC_XH){
    if(value &= LIS331DLH_INT_SRC_XH){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }  
  }
  
  if(statusBIT == LIS331DLH_INT_SRC_XL){
    if(value &= LIS331DLH_INT_SRC_XL){     
      *val = MEMS_SET;
      return MEMS_SUCCESS;
    }
    else{  
      *val = MEMS_RESET;
      return MEMS_SUCCESS;
    }    
  } 
  return MEMS_ERROR;
}
/******************* (C) COPYRIGHT 2012 STMicroelectronics *****END OF FILE****/
