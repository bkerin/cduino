// Test/demo for the accelerometer.h interface.

#include <inttypes.h>

#include "accelerometer.h"
#include "term_io.h"
#include "util.h"

// This test program assumes that an LIS331HH is correctly connected to
// the Arduino via SPI as described in the datasheet for the device.
//
// Since the LIS331H is a 3.3V device, a 5V-3.3V a level shifter is required.
// The Texas instruments SN74LVC245AN is one option.
//
// The LIS331HH comes in a no-lead LGA16 package, which makes
// prototyping hard.  I like the breakout assembly service available
// from www.proto-advantage.com, in particular for the LIS331HH see
// http://www.proto-advantage.com/store/product_info.php?products_id=2200123

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

// Buffered Readings Count.  This program makes a series of readings in a
// burst, then reports what we read laer.  This prevents the serial port
// delay from affecting the read mechanics (e.g. by causing overruns to
// show up in the status register).
#define BRC 142

// Storage for X/Y/Z acceleration readings
int16_t ax[BRC], ay[BRC], az[BRC];

int
main (void)
{
  term_io_init ();
  printf ("term_io_init() completed.\n");

  accelerometer_init ();
  printf ("accelerometer_init() completed.\n");

  //status_t sent_value = LIS331DLH_SetMode (LIS331DLH_NORMAL);
  //BASSERT (sent_value);
  
  uint8_t ctrl1_value;
  LIS331DLH_ReadReg (
      LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG1, &ctrl1_value);
  printf ("CTRL_REG1: %x\n", ctrl1_value);

  LIS331DLH_SetMode(LIS331DLH_NORMAL);

  LIS331HH_SetFullScale (LIS331HH_FULLSCALE_24);                    

  for ( ; ; ) {

    for ( uint8_t ii = 0 ; ii < BRC ; ) {

      uint8_t status_reg_contents;
      LIS331DLH_GetStatusReg (&status_reg_contents);

      if ( ! (status_reg_contents & LIS331DLH_DATAREADY_BIT) ) {
        continue;
      }
      // BTRAP ();
          
      AxesRaw_t aclr;   // Accelerometer Readings
      aclr.AXIS_X = 42;
      aclr.AXIS_Y = 42;
      aclr.AXIS_Z = 42;
      status_t result = LIS331DLH_GetAccAxesRaw (&aclr);
      BASSERT (result = MEMS_SUCCESS);
      ax[ii] = aclr.AXIS_X;
      ay[ii] = aclr.AXIS_Y;
      az[ii] = aclr.AXIS_Z;
  
      ii++;
    }

    for ( uint8_t ii = 0 ; ii < BRC ; ii++ ) {
      printf (
          "Ax: %3i  Ay: %3i  Az: %3i\n",
          (int) ax[ii], (int) ay[ii], (int) az[ii] );
      // The AVR libc version I have seems to get confused when we try to use
      // field widths with the type macros PRIi16 etc. so we can't use this:
      //printf (
      //    "Ax: %3 " PRIi16 "Ay: %3 " PRIi16 "Az: %3 " PRIi16 "\n",
      //    (int) ax[ii], (int) ay[ii], (int) az[ii] );
    }

    // FIXME: we don't use a delay loop at the moment just frantically read
    // untel the device signals us it has a new value.
    //float tbr = 1000.0;   // Time Between Readings (in milliseconds)
    //_delay_ms (tbr);
  }
}
