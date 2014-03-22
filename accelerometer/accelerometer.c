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
  // The defaults that spi_init() uses will work for this device, so we don't
  // need any more calls to configure SPI FIXME: if we can use a higher data
  // rate than 4 MHz, we might want to do that by default?
}
