// Test/demo for the spi.h interface.
//
// This test driver requires an Analog Devices AD5206 connected as follows:
//
//   * A6 pin (of AD5206) connected to +5V
//   * B6 of AD5206 connected to ground
//   * /CS to digital pin 10  (SS pin)
//   * SDI to digital pin 11 (MOSI pin)
//   * CLK - to digital pin 13 (SCK pin)
//
// This program starts with the wiper pin W6 connected to ground (SPI
// data input 0x00).  It then move the wiper 1/4 of the way up the scale
// every 5 seconds.  If things are working correctly, this will produce a
// voltage output sequence of ~0V, ~1/4 Vcc, ~1/2 Vcc, ~3/4 Vcc, and ~Vcc
// at wiper pin W6.  It then repeats this sequence using all the different
// clock divider frequencies (of which there are a total of 7).

#include <assert.h>
#include <stdlib.h>   // FIXME: remove this once assert.h is fixed (new avrlibc)
#include <util/delay.h>

#include "spi.h"

#if ! (defined (MY_SPI_SLAVE_1_SELECT_INIT) && \
       defined (MY_SPI_SLAVE_1_SELECT_SET_LOW) && \
       defined (MY_SPI_SLAVE_1_SELECT_SET_HIGH))
#  error The macros which specify which pin should be used for SPI slave \
         selection are not set.  Please see the example in the Makefile \
         in the spi module directory.
#endif

// WARNING: This module not fully tested.  These tests test output with
// SPI_BIT_ORDER_MSB_FIRST and SPI_DATA_MODE_0 with all SPI_CLOCK_DIVIDER_*
// settings.  The sd_card.h interface exercises input.  The other data
// orders and modes are only trivially different and should work fine,
// but I have not personally tried them. Remove this warning trap and try it!

int
main (void)
{
  MY_SPI_SLAVE_1_SELECT_INIT (); 

  spi_init ();

  spi_set_data_order (SPI_DATA_ORDER_MSB_FIRST);
  
  spi_set_data_mode (SPI_DATA_MODE_0);
  
  // We're going to use a loop over the clock divider settings, so here we
  // verify that the interface gives them the the endpoint values we expect.
  assert (SPI_CLOCK_DIVIDER_DIV4 == 0x00);
  assert (SPI_CLOCK_DIVIDER_DIV32 == 0x06);
 
  // Smallest, Largest Clock divider Setting
  uint8_t scs = SPI_CLOCK_DIVIDER_DIV4, lcs = SPI_CLOCK_DIVIDER_DIV32;

  // For each clock divider setting...
  for ( uint8_t cds = scs ; cds <= lcs ; cds++ ) {

    spi_set_clock_divider (cds); 

    // Number of different resistor setting we test
    int const test_steps = 5;

    // For each different resistance setting we want to test...
    for ( int ii = 0 ; ii < test_steps ; ii++ ) {
      MY_SPI_SLAVE_1_SELECT_SET_LOW ();
      uint8_t const channel_six_address = 0x05;   // From AD5206 datasheet
      spi_transfer (channel_six_address);
      spi_transfer (ii * 255 / 4);
      MY_SPI_SLAVE_1_SELECT_SET_HIGH ();
      double const seconds_per_step = 5.0;
      _delay_ms (1000.0 * seconds_per_step);
    }
  }  
}
