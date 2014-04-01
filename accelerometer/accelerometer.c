// Implementation of the interface described in accelerometer.h.

#include "accelerometer.h"
#include "lis331dlh_driver.h"
#include "spi.h"

// SPI-related initialization macro as described in spi.h.
#define MY_SPI_SLAVE_ACCELEROMETER_SELECT_INIT() \
  SPI_SS_INIT(DIO_OUTPUT, DIO_DONT_CARE, HIGH)
// Note that since all the actual SPI use other than initialization
// takes place in the lis331dlh_driver.c file (kindly provided by ST
// Microelectronics) the MY_SPI_SLAVE_ACCELEROMETER_SELECT_SET_LOW/HIGH
// macros that spi.h implies should exist are defined there.  Letting this
// interface be responsible for SPI initialization keeps the changes
// required in the provided driver file to a minimum (for easy tracking of
// any upstream changes).

#define MY_SPI_SLAVE_ACCELEROMETER_SELECT_SET_LOW SPI_SS_SET_LOW
#define MY_SPI_SLAVE_ACCELEROMETER_SELECT_SET_HIGH SPI_SS_SET_HIGH

void
accelerometer_init (void)
{
  MY_SPI_SLAVE_ACCELEROMETER_SELECT_INIT ();
  spi_init ();
  spi_set_data_mode (SPI_DATA_MODE_3);

  accelerometer_power_up ();
}

void
accelerometer_power_down (void)
{
  LIS331DLH_SetMode(LIS331DLH_POWER_DOWN);
}

void
accelerometer_power_up (void)
{
  LIS331DLH_SetMode(LIS331DLH_NORMAL);
}

void
accelerometer_set_fullscale (accelerometer_fullscale_t fs)
{
  LIS331HH_SetFullScale (fs);                    
}

void
accelerometer_set_data_rate (accelerometer_data_rate_t dr)
{
  LIS331DLH_SetODR (dr);
}

void
accelerometer_get_accel (int16_t *ax, int16_t *ay, int16_t *az)
{
  AxesRaw_t aclr;   // Accelerometer Readings
  LIS331DLH_GetAccAxesRaw (&aclr);
  *ax = aclr.AXIS_X;
  *ay = aclr.AXIS_Y;
  *az = aclr.AXIS_Z;
}
