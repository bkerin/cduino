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

int
main (void)
{
  BTRAP ();

  term_io_init ();
  printf ("term_io_init() completed.\n");

  accelerometer_init ();

  for ( ; ; ) {
    AxesRaw_t aclr;   // Accelerometer Readings
    status_t result = LIS331DLH_GetAccAxesRaw (&aclr);
    BASSERT (result = MEMS_SUCCESS);

    printf (
        "Ax: %" PRIi16 "Ay: %" PRIi16 "Az: %" PRIi16 "\n",
        aclr.AXIS_X, aclr.AXIS_Y, aclr.AXIS_Z );

    float tbr = 100.0;   // Time Between Readings (in milliseconds)
    _delay_ms (tbr);
  }
}
