// Test/demo for the dc_motor.h interface.

#include <util/delay.h>

#include "dc_motor.h"
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

// FIXME: document what this test requires/does in the usual spots

int
main (void)
{
  // This isn't what we're testing exactly, but we need to know if its
  // working or not to interpret other results.
  term_io_init ();
  PFP ("\n");
  PFP ("\n");
  PFP ("term_io_init() worked.\n");
  PFP ("\n");

  dc_motor_init ();
  PFP ("Finished dc_motor_init().\n");

  PFP ("\n");

  // Ramp motor speed up and down continually, measuring load at each step
  {
    int8_t spd = 0;          // Speed setting
    int8_t ss = 20;          // Step size (in % duty cycle)
    int8_t ssgn = 1;         // Step sign
    double tps = 2000.0;     // Time Per Step (in ms)
    int8_t max_spd = 100;    // Maximum speed setting
    int8_t min_spd = -100;   // Minimum speed setting (full speed reverse)

    for ( ; ; ) {

      //dc_motor_set_speed (DC_MOTOR_CHANNEL_B, ss * sign);
      dc_motor_set_speed (DC_MOTOR_CHANNEL_B, spd);
      PFP ("Set motor speed to %hhi\n", spd);

      // Run motor at this speed for tps milliseconds, measuring load current
      // half way through.
      _delay_ms (tps / 2.0);
      int mapa = 1000;   // Milliamps Per Amp
      PFP (
          "dc motor load current: %i mA\n", 
          (int) (mapa * dc_motor_load_current (DC_MOTOR_CHANNEL_B)) );
      _delay_ms (tps / 2.0);

      PFP ("\n");

      // Calculate motor speed to set on next iteration
      spd += ssgn * ss;
      if ( spd > max_spd ) {
        spd = max_spd - ss;
        ssgn = -1;
      }
      if ( spd < min_spd ) {
        spd = min_spd + ss;
        ssgn = 1;
      }
    }
  }
}
