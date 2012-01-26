// Exercise the interface described in dio_pin.h.

// vim: foldmethod=marker

#include <assert.h>
#include <util/delay.h>
// FIXME: We neeed this because assert.h doesn't include it.  When that is
// fixed this include could go.
#include <stdlib.h>  

#include "dio_pin.h"

#ifndef HIGH
#  define HIGH 1
#endif

#ifndef LOW
#  define LOW 0
#endif

// Blink the PB5 LED quickly.
static void
quick_pb5_blink (void)
{
  const int blink_count = 6, blinks_per_second = 4;

  for ( int ii = 0 ; ii < blink_count ; ii++ ) {
#define MILLISECONDS_PER_SECOND 1000
    DIO_SET_PB5 (1);
    _delay_ms (MILLISECONDS_PER_SECOND / (blinks_per_second * 2));
    DIO_SET_PB5 (0);
    _delay_ms (MILLISECONDS_PER_SECOND / (blinks_per_second * 2));
  }
}

int
main (void)
{
  // To keep things simple, we just use assert and blink on-board PB5 LED
  // a bit when the right stuff happens, or expect the user to check the
  // blinking LEDs to verify that the right thing is happening.  These tests
  // therefore require some human attention to switches to connect the outputs
  // to the expected rails at the expected times, and to monitor LED output.
  // Closing all the top level folds will and reading the comments above
  // them give a step-by-step description of what should be done and expected.
  
  uint8_t value;   // Holds value set to or read from a pin.
  const double milliseconds_per_second = 1000;

  // For this test, we assume all the pins have nothing connected externally.
  // We then set the pins for input, with pullups enabled, and read all
  // their values.  They should all read as high.
  // {{{1

  DIO_INIT_PB0 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PB0 ();
  assert (value);
  
  DIO_INIT_PB1 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PB1 ();
  assert (value);

  DIO_INIT_PB2 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PB2 ();
  assert (value);

  DIO_INIT_PB3 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PB3 ();
  assert (value);

  DIO_INIT_PB4 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PB4 ();
  assert (value);

  // NOTE: on the Arduino, PB5 is pulled towards ground via one or two 1
  // kohm resistors in parallel and a LED.  This is a stronger pull than
  // that exerted by the internal pull-up resistor, and so we expect to read
  // a low value from this pin even with the pull-up enabled.
  DIO_INIT_PB5 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PB5 ();
  assert (!value);

  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PB6 and PB7 control macros are identical in form to the other \
   macros in this interface, but have not been tested.  The Makefile for this \
   module includes a line which may be uncommented to override this failure.
#endif

  DIO_INIT_PC0 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PC0 ();
  assert (value);
  
  DIO_INIT_PC1 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PC1 ();
  assert (value);
  
  DIO_INIT_PC2 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PC2 ();
  assert (value);
  
  DIO_INIT_PC3 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PC3 ();
  assert (value);
  
  DIO_INIT_PC4 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PC4 ();
  assert (value);
  
  DIO_INIT_PC5 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PC5 ();
  assert (value);
  
  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PC6 control macros are identical in form to the other macros in \
   this interface, but have not been tested.  The Makefile for this module \
   includes a line which may be uncommented to override this failure.
#endif
  
  // These we can test only if the Arduino isn't hogging the serial line.
#ifndef NO_TEST_SERIAL_PINS
  //assert (0);
  DIO_INIT_PD0 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD0 ();
  assert (value);
  
  DIO_INIT_PD1 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD1 ();
  assert (value);
#endif
  
  DIO_INIT_PD2 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD2 ();
  assert (value);
  
  DIO_INIT_PD3 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD3 ();
  assert (value);
  
  DIO_INIT_PD4 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD4 ();
  assert (value);
  
  DIO_INIT_PD5 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD5 ();
  assert (value);

  DIO_INIT_PD6 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD6 ();
  assert (value);

  DIO_INIT_PD7 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD7 ();
  assert (value);
  
  // Signal we got through the test.
  DIO_INIT_PB5 (DIO_OUTPUT, DIO_DONT_CARE, 0);
  quick_pb5_blink ();

  // }}}1

  // For this test, we expect all pins except PB5 (which is held somewhat
  // low by its resistor-LED combination anyway) to be held low, and test
  // the operation of the pins both with the pull-ups still on.
  // {{{1
  
  // Delay for a second to give the user time to close a switch connecting
  // all pins to ground.
  _delay_ms (milliseconds_per_second);

  value = DIO_READ_PB0 ();
  assert (!value);
  
  value = DIO_READ_PB1 ();
  assert (!value);

  value = DIO_READ_PB2 ();
  assert (!value);

  value = DIO_READ_PB3 ();
  assert (!value);

  value = DIO_READ_PB4 ();
  assert (!value);

  value = DIO_READ_PB5 ();
  assert (!value);

  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PB6 and PB7 control macros are identical in form to the other \
   macros in this interface, but have not been tested.  The Makefile for this \
   module includes a line which may be uncommented to override this failure.
#endif

  value = DIO_READ_PC0 ();
  assert (!value);
  
  value = DIO_READ_PC1 ();
  assert (!value);
  
  value = DIO_READ_PC2 ();
  assert (!value);
  
  value = DIO_READ_PC3 ();
  assert (!value);
  
  value = DIO_READ_PC4 ();
  assert (!value);
  
  value = DIO_READ_PC5 ();
  assert (!value);
  
  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PC6 control macros are identical in form to the other macros in \
   this interface, but have not been tested.  The Makefile for this module \
   includes a line which may be uncommented to override this failure.
#endif
  
  // These we can test only if the Arduino isn't hogging the serial line.
#ifndef NO_TEST_SERIAL_PINS
  value = DIO_READ_PD0 ();
  assert (!value);
  
  value = DIO_READ_PD1 ();
  assert (!value);
#endif
  
  value = DIO_READ_PD2 ();
  assert (!value);
  
  value = DIO_READ_PD3 ();
  assert (!value);
  
  value = DIO_READ_PD4 ();
  assert (!value);
  
  value = DIO_READ_PD5 ();
  assert (!value);

  value = DIO_READ_PD6 ();
  assert (!value);

  value = DIO_READ_PD7 ();
  assert (!value);
  
  // Signal we got through the test.
  DIO_INIT_PB5 (DIO_OUTPUT, DIO_DONT_CARE, 0);
  quick_pb5_blink ();

  // }}}1

  // For this test, we disable all internal pull-up resistors, and expect
  // all pins except PB5 (which is held somewhat low by its resistor-LED
  // combination anyway) to be held low.
  // {{{1
  
  // Delay for a second between tests.
  _delay_ms (milliseconds_per_second);

  DIO_INIT_PB0 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PB0 ();
  assert (!value);
  
  DIO_INIT_PB1 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PB1 ();
  assert (!value);

  DIO_INIT_PB2 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PB2 ();
  assert (!value);

  DIO_INIT_PB3 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PB3 ();
  assert (!value);

  DIO_INIT_PB4 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PB4 ();
  assert (!value);

  DIO_INIT_PB5 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PB5 ();
  assert (!value);

  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PB6 and PB7 control macros are identical in form to the other \
   macros in this interface, but have not been tested.  The Makefile for this \
   module includes a line which may be uncommented to override this failure.
#endif

  DIO_INIT_PC0 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PC0 ();
  assert (!value);
  
  DIO_INIT_PC1 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PC1 ();
  assert (!value);
  
  DIO_INIT_PC2 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PC2 ();
  assert (!value);
  
  DIO_INIT_PC3 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PC3 ();
  assert (!value);
  
  DIO_INIT_PC4 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PC4 ();
  assert (!value);
  
  DIO_INIT_PC5 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PC5 ();
  assert (!value);
  
  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PC6 control macros are identical in form to the other macros in \
   this interface, but have not been tested.  The Makefile for this module \
   includes a line which may be uncommented to override this failure.
#endif
  
  // These we can test only if the Arduino isn't hogging the serial line.
#ifndef NO_TEST_SERIAL_PINS
  DIO_INIT_PD0 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD0 ();
  assert (!value);
  
  DIO_INIT_PD1 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD1 ();
  assert (!value);
#endif
  
  DIO_INIT_PD2 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD2 ();
  assert (!value);
  
  DIO_INIT_PD3 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD3 ();
  assert (!value);
  
  DIO_INIT_PD4 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD4 ();
  assert (!value);
  
  DIO_INIT_PD5 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD5 ();
  assert (!value);

  DIO_INIT_PD6 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD6 ();
  assert (!value);

  DIO_INIT_PD7 (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  value = DIO_READ_PD7 ();
  assert (!value);
  
  // Signal we got through the test.
  DIO_INIT_PB5 (DIO_OUTPUT, DIO_DONT_CARE, 0);
  quick_pb5_blink ();

  // }}}1
  
  // For this test, leave all internal pull-up resistors disabled, and expect
  // all pins to be held high.
  // {{{1
  
  // Delay for a second to give the user time to close a switch connecting
  // all pins to high.
  _delay_ms (milliseconds_per_second);

  value = DIO_READ_PB0 ();
  assert (value);
  
  value = DIO_READ_PB1 ();
  assert (value);

  value = DIO_READ_PB2 ();
  assert (value);

  value = DIO_READ_PB3 ();
  assert (value);

  value = DIO_READ_PB4 ();
  assert (value);

  value = DIO_READ_PB5 ();
  assert (value);

  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PB6 and PB7 control macros are identical in form to the other \
   macros in this interface, but have not been tested.  The Makefile for this \
   module includes a line which may be uncommented to override this failure.
#endif

  value = DIO_READ_PC0 ();
  assert (value);
  
  value = DIO_READ_PC1 ();
  assert (value);
  
  value = DIO_READ_PC2 ();
  assert (value);
  
  value = DIO_READ_PC3 ();
  assert (value);
  
  value = DIO_READ_PC4 ();
  assert (value);
  
  value = DIO_READ_PC5 ();
  assert (value);
  
  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PC6 control macros are identical in form to the other macros in \
   this interface, but have not been tested.  The Makefile for this module \
   includes a line which may be uncommented to override this failure.
#endif
  
  // These we can test only if the Arduino isn't hogging the serial line.
#ifndef NO_TEST_SERIAL_PINS
  value = DIO_READ_PD0 ();
  assert (value);
  
  value = DIO_READ_PD1 ();
  assert (value);
#endif
  
  value = DIO_READ_PD2 ();
  assert (value);
  
  value = DIO_READ_PD3 ();
  assert (value);
  
  value = DIO_READ_PD4 ();
  assert (value);
  
  value = DIO_READ_PD5 ();
  assert (value);

  value = DIO_READ_PD6 ();
  assert (value);

  value = DIO_READ_PD7 ();
  assert (value);
  
  // Signal we got through the test.
  DIO_INIT_PB5 (DIO_OUTPUT, DIO_DONT_CARE, 0);
  quick_pb5_blink ();

  // }}}1

  // When blinking LEDs to test output function, we use these timing values.
#define OUTPUT_TEST_POST_INIT_WAIT_SECONDS 5.0
#define OUTPUT_TEST_TOGGLE_SECONDS 60.0
  // This can't be changed without changing other things to match.
#define OUTPUT_TEST_BLINK_ON_TIME 0.5
  
  // For this test, we configure all pins as outputs, with initial_value HIGH.
  // Then we wait OUTPUT_TEST_POST_INIT_WAIT_SECONDS.  Then we toggle the
  // value off and on for OUTPUT_TEST_TOGGLE_SECONDS seconds.  This test
  // depends on a human user to monitor one or more connected LEDs to verify
  // correct operation.
  // {{{1
  
  // Delay for a second to give the user time to close switches used for
  // testing pins as inputs.
  _delay_ms (milliseconds_per_second);

  DIO_INIT_PB0 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PB1 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PB2 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PB3 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PB4 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PB5 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);

  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PB6 and PB7 control macros are identical in form to the other \
   macros in this interface, but have not been tested.  The Makefile for this \
   module includes a line which may be uncommented to override this failure.
#endif

  DIO_INIT_PC0 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PC1 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PC2 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PC3 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PC4 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PC5 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);

  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PC6 control macros are identical in form to the other macros in \
   this interface, but have not been tested.  The Makefile for this module \
   includes a line which may be uncommented to override this failure.
#endif
  
  // These we can test only if the Arduino isn't hogging the serial line.
#ifndef NO_TEST_SERIAL_PINS
  //assert (0);
  DIO_INIT_PD0 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PD1 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
#endif
  
  DIO_INIT_PD2 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PD3 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PD4 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PD5 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PD6 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  DIO_INIT_PD7 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);

  _delay_ms (OUTPUT_TEST_POST_INIT_WAIT_SECONDS * milliseconds_per_second);

  for ( int ii = 0 ; ii < OUTPUT_TEST_TOGGLE_SECONDS ; ii++ ) {
    DIO_SET_PB0 (LOW);
    DIO_SET_PB1 (LOW);
    DIO_SET_PB2 (LOW);
    DIO_SET_PB3 (LOW);
    DIO_SET_PB4 (LOW);
    DIO_SET_PB5 (LOW);
    // Its a sin in my book to distribute untested code without clearly
    // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PB6 and PB7 control macros are identical in form to the other \
   macros in this interface, but have not been tested.  The Makefile for this \
   module includes a line which may be uncommented to override this failure.
#endif

    DIO_SET_PC0 (LOW);
    DIO_SET_PC1 (LOW);
    DIO_SET_PC2 (LOW);
    DIO_SET_PC3 (LOW);
    DIO_SET_PC4 (LOW);
    DIO_SET_PC5 (LOW);
    // Its a sin in my book to distribute untested code without clearly
    // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PC6 control macros are identical in form to the other macros in \
   this interface, but have not been tested.  The Makefile for this module \
   includes a line which may be uncommented to override this failure.
#endif
  
  // These we can test only if the Arduino isn't hogging the serial line.
#ifndef NO_TEST_SERIAL_PINS
    DIO_SET_PD0 (LOW);
    DIO_SET_PD1 (LOW);
#endif
    DIO_SET_PD2 (LOW);
    DIO_SET_PD3 (LOW);
    DIO_SET_PD4 (LOW);
    DIO_SET_PD5 (LOW);
    DIO_SET_PD6 (LOW);
    DIO_SET_PD7 (LOW);
    
    _delay_ms (OUTPUT_TEST_BLINK_ON_TIME);  
    
    DIO_SET_PB0 (HIGH);
    DIO_SET_PB1 (HIGH);
    DIO_SET_PB2 (HIGH);
    DIO_SET_PB3 (HIGH);
    DIO_SET_PB4 (HIGH);
    DIO_SET_PB5 (HIGH);
    // Its a sin in my book to distribute untested code without clearly
    // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PB6 and PB7 control macros are identical in form to the other \
   macros in this interface, but have not been tested.  The Makefile for this \
   module includes a line which may be uncommented to override this failure.
#endif

    DIO_SET_PC0 (HIGH);
    DIO_SET_PC1 (HIGH);
    DIO_SET_PC2 (HIGH);
    DIO_SET_PC3 (HIGH);
    DIO_SET_PC4 (HIGH);
    DIO_SET_PC5 (HIGH);
    // Its a sin in my book to distribute untested code without clearly
    // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PC6 control macros are identical in form to the other macros in \
   this interface, but have not been tested.  The Makefile for this module \
   includes a line which may be uncommented to override this failure.
#endif
  
  // These we can test only if the Arduino isn't hogging the serial line.
#ifndef NO_TEST_SERIAL_PINS
    DIO_SET_PD0 (HIGH);
    DIO_SET_PD1 (HIGH);
#endif
    DIO_SET_PD2 (HIGH);
    DIO_SET_PD3 (HIGH);
    DIO_SET_PD4 (HIGH);
    DIO_SET_PD5 (HIGH);
    DIO_SET_PD6 (HIGH);
    DIO_SET_PD7 (HIGH);
  
    _delay_ms (OUTPUT_TEST_BLINK_ON_TIME);  
  }

  // Signal we finished the test.
  DIO_INIT_PB5 (DIO_OUTPUT, DIO_DONT_CARE, 0);
  quick_pb5_blink ();

  // }}}1
  
  // For this test, we configure all pins as outputs, with initial_value LOW.
  // Then we wait OUTPUT_TEST_POST_INIT_WAIT_SECONDS.  Then we toggle the
  // value off and on for OUTPUT_TEST_TOGGLE_SECONDS seconds.  This test
  // depends on a human user to monitor one or more connected LEDs to verify
  // correct operation.
  // {{{1
  
  // Delay for a second to give the user time to close switches used for
  // testing pins as inputs.
  _delay_ms (milliseconds_per_second);

  DIO_INIT_PB0 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PB1 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PB2 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PB3 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PB4 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PB5 (DIO_OUTPUT, DIO_DONT_CARE, LOW);

  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PB6 and PB7 control macros are identical in form to the other \
   macros in this interface, but have not been tested.  The Makefile for this \
   module includes a line which may be uncommented to override this failure.
#endif

  DIO_INIT_PC0 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PC1 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PC2 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PC3 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PC4 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PC5 (DIO_OUTPUT, DIO_DONT_CARE, LOW);

  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PC6 control macros are identical in form to the other macros in \
   this interface, but have not been tested.  The Makefile for this module \
   includes a line which may be uncommented to override this failure.
#endif
  
  // These we can test only if the Arduino isn't hogging the serial line.
#ifndef NO_TEST_SERIAL_PINS
  //assert (0);
  DIO_INIT_PD0 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PD1 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
#endif
  
  DIO_INIT_PD2 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PD3 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PD4 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PD5 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PD6 (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  DIO_INIT_PD7 (DIO_OUTPUT, DIO_DONT_CARE, LOW);

  _delay_ms (OUTPUT_TEST_POST_INIT_WAIT_SECONDS * milliseconds_per_second);

  for ( int ii = 0 ; ii < OUTPUT_TEST_TOGGLE_SECONDS ; ii++ ) {
    DIO_SET_PB0 (HIGH);
    DIO_SET_PB1 (HIGH);
    DIO_SET_PB2 (HIGH);
    DIO_SET_PB3 (HIGH);
    DIO_SET_PB4 (HIGH);
    DIO_SET_PB5 (HIGH);
    // Its a sin in my book to distribute untested code without clearly
    // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PB6 and PB7 control macros are identical in form to the other \
   macros in this interface, but have not been tested.  The Makefile for this \
   module includes a line which may be uncommented to override this failure.
#endif

    DIO_SET_PC0 (HIGH);
    DIO_SET_PC1 (HIGH);
    DIO_SET_PC2 (HIGH);
    DIO_SET_PC3 (HIGH);
    DIO_SET_PC4 (HIGH);
    DIO_SET_PC5 (HIGH);
    // Its a sin in my book to distribute untested code without clearly
    // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PC6 control macros are identical in form to the other macros in \
   this interface, but have not been tested.  The Makefile for this module \
   includes a line which may be uncommented to override this failure.
#endif
  
  // These we can test only if the Arduino isn't hogging the serial line.
#ifndef NO_TEST_SERIAL_PINS
    DIO_SET_PD0 (HIGH);
    DIO_SET_PD1 (HIGH);
#endif
    DIO_SET_PD2 (HIGH);
    DIO_SET_PD3 (HIGH);
    DIO_SET_PD4 (HIGH);
    DIO_SET_PD5 (HIGH);
    DIO_SET_PD6 (HIGH);
    DIO_SET_PD7 (HIGH);
    
    _delay_ms (OUTPUT_TEST_BLINK_ON_TIME);  
    
    DIO_SET_PB0 (LOW);
    DIO_SET_PB1 (LOW);
    DIO_SET_PB2 (LOW);
    DIO_SET_PB3 (LOW);
    DIO_SET_PB4 (LOW);
    DIO_SET_PB5 (LOW);
    // Its a sin in my book to distribute untested code without clearly
    // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PB6 and PB7 control macros are identical in form to the other \
   macros in this interface, but have not been tested.  The Makefile for this \
   module includes a line which may be uncommented to override this failure.
#endif

    DIO_SET_PC0 (LOW);
    DIO_SET_PC1 (LOW);
    DIO_SET_PC2 (LOW);
    DIO_SET_PC3 (LOW);
    DIO_SET_PC4 (LOW);
    DIO_SET_PC5 (LOW);
    // Its a sin in my book to distribute untested code without clearly
    // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PC6 control macros are identical in form to the other macros in \
   this interface, but have not been tested.  The Makefile for this module \
   includes a line which may be uncommented to override this failure.
#endif
  
  // These we can test only if the Arduino isn't hogging the serial line.
#ifndef NO_TEST_SERIAL_PINS
    DIO_SET_PD0 (LOW);
    DIO_SET_PD1 (LOW);
#endif
    DIO_SET_PD2 (LOW);
    DIO_SET_PD3 (LOW);
    DIO_SET_PD4 (LOW);
    DIO_SET_PD5 (LOW);
    DIO_SET_PD6 (LOW);
    DIO_SET_PD7 (LOW);
  
    _delay_ms (OUTPUT_TEST_BLINK_ON_TIME);  
  }

  // Signal we finished the test.
  DIO_INIT_PB5 (DIO_OUTPUT, DIO_DONT_CARE, 0);
  quick_pb5_blink ();

  // }}}1
}
