// Test/demo for the dc_motor.h interface.

// Assumptions:
//
//      - Arduino Motor Shield Rev. 3 plugged into Arduino
//
//      - Small 5V DC motor(s) connected to one (or both) shield outputs
//
//      - In my experience most small motors will turn when the Arduino is
//        powered from USB, but for larger motors it may be necessary
//        to power either the Arduino or the motor shield with a
//        wall wart supply.  Read the material regarding power at
//        http://arduino.cc/en/Main/ArduinoMotorShieldR3.
//
// This program will slowly ramp the motor speeds up and down (including
// putting them in reverse).  The motors are run in opposite directions.
// The load is measured at each speed and printed to the terminal (use
// 'make run_screen' from the shell to view it).  Loading the motor shaft
// (say, by pinching it slightly between thumb and forefinger... carefully)
// should cause the load current to rise.  Note that at lower speed setting
// and voltages, many motors won't get going, so there may be several steps
// where the motors don't move.

#include <util/delay.h>

#include "dc_motor.h"
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

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

  // Ramp motor speeds up and down continually, measuring load at each step.
  // To be cute, motors A and B will be run in opposite directions :)
  {
    int8_t spd = 0;          // Speed setting (for motor A)
    int8_t ss = 20;          // Step size (in % duty cycle)
    int8_t ssgn = 1;         // Step sign
    double tps = 2000.0;     // Time Per Step (in ms)
    int8_t max_spd = 100;    // Maximum speed setting
    int8_t min_spd = -100;   // Minimum speed setting (full speed reverse)

    for ( ; ; ) {

      dc_motor_set_speed (DC_MOTOR_CHANNEL_A, spd);
      PFP ("Set motor A speed to %hhi\n", spd);
      dc_motor_set_speed (DC_MOTOR_CHANNEL_B, -spd);
      PFP ("Set motor B speed to %hhi\n", -spd);

      // Run motors at this speed (in opposite directions) for tps
      // milliseconds, measuring load current half way through.
      _delay_ms (tps / 2.0);
      int mapa = 1000;   // Milliamps Per Amp
      PFP (
          "DC motor A load current: %i mA\n", 
          (int) (mapa * dc_motor_load_current (DC_MOTOR_CHANNEL_A)) );
      PFP (
          "DC motor B load current: %i mA\n", 
          (int) (mapa * dc_motor_load_current (DC_MOTOR_CHANNEL_B)) );
      _delay_ms (tps / 2.0);

      PFP ("\n");

      // Calculate motor speed to set on next iteration
      spd += ssgn * ss;
      if ( spd > max_spd ) {
        spd = max_spd;
        ssgn = -1;
      }
      else if ( spd < min_spd ) {
        spd = min_spd;
        ssgn = 1;
      }
    }
  }
}
