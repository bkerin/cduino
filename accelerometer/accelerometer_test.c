// Test/demo for the accelerometer.h interface.

#include <float.h>
#include <inttypes.h>
#include <math.h>

#include "accelerometer.h"
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

// This test program assumes that an LIS331HH is correctly connected to
// the Arduino via SPI as described in the datasheet for the device.
//
// Since the LIS331H is a 3.3V device, a 5V-3.3V a level shifter is required
// to use it with an Arduino.  I used the Sparkfun BOB-12009.
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

// Buffered Readings Count.  This program makes a series of readings in
// a burst, then reports what we read laer.  This approach prevents the
// serial port delay associated with outputting the data from interfering
// with the accelerometer reading mechanics (e.g. by causing overruns to
// show up in the status register).
#define BRC 142

// Storage for X/Y/Z acceleration readings
int16_t ax[BRC], ay[BRC], az[BRC];

int
main (void)
{
  term_io_init ();
  PFP ("term_io_init() completed.\n");

  accelerometer_init ();
  PFP ("accelerometer_init() completed.\n");

  // After power-up, control register 1 should consist of its default
  // value (0x07) or'ed with the bits that indicate normal mode (0x20).
  // NOTE that since the accelerometer isn't reset when the ATmega328P is,
  // other settings (performed below) may persist accross resets.  To reset
  // the accelerometer, power down the board.  If you want to see the startup
  // values in screen, hold the reset button down after powering up the board
  // until you have the screen setting up and running (make -rR run_screen),
  // then release the reset button.
  uint8_t ctrl1_value;
  LIS331DLH_ReadReg (
      LIS331DLH_MEMS_I2C_ADDRESS, LIS331DLH_CTRL_REG1, &ctrl1_value);
  PFP ("CTRL_REG1 value: %x\n", ctrl1_value);

  accelerometer_power_down ();
  PFP ("accelerometer_power_down() completed.\n");

  // In theory a delay could be inserted here, and the actual power
  // consumption of the device in power-down state measured.  I haven't
  // actually done this.

  accelerometer_power_up ();
  PFP ("accelerometer_power_up() completed.\n");

  accelerometer_set_fullscale (ACCELEROMETER_FULLSCALE_TYPE_6G);
  PFP ("Fullscale set to 24 gravities.\n");

  accelerometer_set_data_rate (ACCELEROMETER_DATA_RATE_1000HZ);
  PFP ("Date rate set to 1000 Hz.\n");

  // FIXME: this is a work in progress.  In theory we can make gravity go
  // away using the high-pass filter.  In practice I'm not yet clear on the
  // exact mantra required.
  /*
  // I believe this means: "use the filter for the device readings (I think
  // you use different mantra to apply the filter to interrupt generation).
  LIS331DLH_SetFDS (MEMS_ENABLE);

  // Required mantra for turning the filter on generally.
  LIS331DLH_SetHPFMode(LIS331DLH_HPM_REF_SIGNAL);

  // This ends up meaning 2.5 Hz when a 1000Hz data rate is being used;
  // see Table 23 of the LIS331HH datasheet.
  LIS331DLH_SetHPFCutOFF(LIS331DLH_HPFCF_3);

  // It's totally unclear from the datasheed what the REFERENCE register is
  // for or how it's interpreted.  It definitely has an effect, but I can't
  // tell what it's doing.
  LIS331DLH_SetReference (42);
  */

  for ( ; ; ) {

    for ( uint8_t ii = 0 ; ii < BRC ; ii++ ) {
      accelerometer_get_accel (&(ax[ii]), &(ay[ii]), &(az[ii]));
    }

    // Find the average and peak acceleration (in an arbitrary direction).
    float total_ax = 0, total_ay = 0, total_az = 0;
    float peak_abs_a = -FLT_MAX;
    int16_t peak_ax = INT16_MIN, peak_ay = INT16_MIN, peak_az = INT16_MIN;
    for ( uint8_t ii = 0 ; ii < BRC ; ii++ ) {
      total_ax += ax[ii];
      total_ay += ay[ii];
      total_az += az[ii];
      float cur_a
        = sqrt (pow(ax[ii], 2.0) + pow (ay[ii], 2.0) + pow (az[ii], 2.0));
      if ( cur_a > peak_abs_a ) {
        peak_ax = ax[ii];
        peak_ay = ay[ii];
        peak_az = az[ii];
      }
    }
    float mean_ax = total_ax / BRC;
    float mean_ay = total_ay / BRC;
    float mean_az = total_az / BRC;

    PFP (
        "Recent-time peak acceleration: Ax: %3i  Ay: %3i  Az: %3i\n",
        (int) peak_ax, (int) peak_ay, (int) peak_az );
    PFP (
        "Recent-time mean acceleration: Ax: %3i  Ay: %3i  Az: %3i\n",
        (int) mean_ax, (int) mean_ay, (int) mean_az );
    PFP ("\n");
  }
}
