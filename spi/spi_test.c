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
//
// 

#include <assert.h>
#include <stdlib.h>   // FIXME: remove this once assert.h is fixed (new avrlibc)
#include <util/delay.h>

#include "spi.h"

int
main (void)
{
  spi_init ();

  spi_set_bit_order (SPI_BIT_ORDER_MSB_FIRST);
  
  spi_set_data_mode (SPI_DATA_MODE_0);
  
  // We're going to use a look over the clock divider settings, so here we
  // verify that the interface gives them the the endpoint values we expect.
  assert (SPI_CLOCK_DIVIDER_DIV4 == 0x00);
  assert (SPI_CLOCK_DIVIDER_DIV32 == 0x06);
 
  uint8_t cds = 0;   // Clock Divider Setting
  for ( cds = 0x00 ; cds <= 0x06 ; cds++ ) {
    spi_set_clock_divider (cds); 
    int ii;
    for ( ii = 0 ; ii <= 4 ; ii++ ) {
      SPI_SS_LOW ();
      uint8_t const channel_six_address = 0x05;   // From AD5206 datasheet
      spi_transfer (channel_six_address);
      spi_transfer (ii * 255 / 4);
      SPI_SS_HIGH ();
      double const seconds_per_step = 5.0;
      _delay_ms (1000.0 * seconds_per_step);
    }
  }  
}
