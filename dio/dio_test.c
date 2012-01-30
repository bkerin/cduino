// Exercise the interface described in dio_pin.h.

// vim: foldmethod=marker

#include <assert.h>
#include <util/delay.h>
// FIXME: We neeed this because assert.h doesn't include it.  When that is
// fixed this include could go.
#include <stdlib.h>  

#include "dio.h"

// Signal a checkpoint by blinking the PB5 LED quickly.
void
signal_checkpoint_with_pb5_blinks (void) __attribute__ ((unused));
void
signal_checkpoint_with_pb5_blinks (void)
// {{{1
{ 

  DIO_INIT_PB5 (DIO_OUTPUT, DIO_DONT_CARE, LOW);

  const int blink_count = 3, blinks_per_second = 4;
  const int post_blink_pause_ms = 500;

  for ( int ii = 0 ; ii < blink_count ; ii++ ) {
#define MILLISECONDS_PER_SECOND 1000
    DIO_SET_PB5 (HIGH);
    _delay_ms (MILLISECONDS_PER_SECOND / (blinks_per_second * 2));
    DIO_SET_PB5 (LOW);
    _delay_ms (MILLISECONDS_PER_SECOND / (blinks_per_second * 2));
  }

  _delay_ms (post_blink_pause_ms);
}
// }}}1

// Signal a checkpoint by blinking the PB0 LED quickly.
void
signal_checkpoint_with_pb0_blinks (void) __attribute__ ((unused));
void
signal_checkpoint_with_pb0_blinks (void)
// {{{1
{ 

  DIO_INIT_PB0 (DIO_OUTPUT, DIO_DONT_CARE, LOW);

  const int blink_count = 3, blinks_per_second = 4;
  const int post_blink_pause_ms = 500;

  for ( int ii = 0 ; ii < blink_count ; ii++ ) {
#define MILLISECONDS_PER_SECOND 1000
    DIO_SET_PB0 (HIGH);
    _delay_ms (MILLISECONDS_PER_SECOND / (blinks_per_second * 2));
    DIO_SET_PB0 (LOW);
    _delay_ms (MILLISECONDS_PER_SECOND / (blinks_per_second * 2));
  }

  _delay_ms (post_blink_pause_ms);
}
// }}}1

// Normally we signal checkpoints by blinking the LED attached to PB5,
// but if we're testing the pin for some other purpose we have to use a
// different pin for signalling.
#if (! defined(TEST_CONDITION_PB5_HIGH_PB0_SIGNAL_LED_OTHERS_NC)) && \
    (! defined(TEST_CONDITION_PB5_LOW_PB0_SIGNAL_LED_OTHERS_NC))
#  define SIGNAL_CHECKPOINT signal_checkpoint_with_pb5_blinks
#else
#  define SIGNAL_CHECKPOINT signal_checkpoint_with_pb0_blinks
#endif

int
main (void)
{
  // Its a sin in my book to distribute untested code without clearly
  // acknowledging the fact.
#ifndef UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
#  error The PB6, PB7, and PC6 control macros are identical in form to the \
   other macros in this interface, but have not been tested.  The Makefile \
   for this module includes a line which may be uncommented to override \
   this failure.
#endif

// We're going to be a bit careless with out lack of namespace prefixes
// here so we have this check.
#ifdef INIT
#  error uh oh, INIT is already defined
#endif
#ifdef SET
#  error uh oh, SET is already defined
#endif
#ifdef READ
#  error uh oh, READ is already defined
#endif

  // The time we allow to give the pull-up resistors a chance to get their
  // pins pulled high.  Note that in practice they shouldn't need a full this
  // long to settle, at least with the anticipated capacitances connected
  // to them.  But the chip runs fast enough that not waiting at all can be
  // to fast.
  const double settling_time_ms __attribute__ ((unused)) = 10;

  uint8_t value = 0;   // Holds value set to or read from a pin.
  value = value;   // Prevent compiler from whining

#ifdef TEST_CONDITION_ALL_PINS_NC

  // For this test, we assume all the pins have nothing connected externally.
  // We then set the pins for input with pullups enabled, wait a few
  // milliseconds for the pins to settle high, and read all their values.
  // They should all read as high.
  // {{{1

  DIO_INIT_PB0 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PB1 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PB2 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
#  ifdef TEST_ISP_PINS
  DIO_INIT_PB3 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PB4 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PB5 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
#  endif
  DIO_INIT_PC0 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PC1 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PC2 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PC3 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PC4 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PC5 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
#  ifdef TEST_SERIAL_PINS
  DIO_INIT_PD0 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PD1 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
#  endif
  DIO_INIT_PD2 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PD3 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PD4 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PD5 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PD6 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
  DIO_INIT_PD7 (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);

  _delay_ms (settling_time_ms);

  value = DIO_READ_PB0 ();
  assert (value);

  value = DIO_READ_PB1 ();
  assert (value);

  value = DIO_READ_PB2 ();
  assert (value);

#  ifdef TEST_ISP_PINS

  value = DIO_READ_PB3 ();
  assert (value);

  value = DIO_READ_PB4 ();
  assert (value);

  // NOTE: on the Arduino, PB5 is pulled towards ground via one or two 1 kohm
  // resistors in parallel and a LED.  This is a stronger pull than that
  // exerted by the internal pull-up resistor (which is at least 20 kohm),
  // and so we would expect to read a low value from this pin even with the
  // pull-up enabled.
  value = DIO_READ_PB5 ();
  assert (!value);

#  endif // TEST_ISP_PINS

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

#  ifdef TEST_SERIAL_PINS
  value = DIO_READ_PD0 ();
  assert (value);
  
  value = DIO_READ_PD1 ();
  assert (value);
#  endif
  
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
  
  SIGNAL_CHECKPOINT ();
  return 0;   // Success;

// }}}1

#endif // TEST_CONDITION_ALL_PINS_NC

  // Macros for pin INIT/SET/READ dependinf on which TEST_CONDITION_* is
  // in effect.
// {{{1

#if defined(TEST_CONDITION_PB0_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PB0_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PB0_LED_OTHERS_NC)
#  define INIT DIO_INIT_PB0 
#  define SET DIO_SET_PB0 
#  define READ DIO_READ_PB0 
#elif defined(TEST_CONDITION_PB1_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PB1_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PB1_LED_OTHERS_NC)
#  define INIT DIO_INIT_PB1 
#  define SET DIO_SET_PB1 
#  define READ DIO_READ_PB1 
#elif defined(TEST_CONDITION_PB2_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PB2_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PB2_LED_OTHERS_NC)
#  define INIT DIO_INIT_PB2 
#  define SET DIO_SET_PB2 
#  define READ DIO_READ_PB2 
#elif defined(TEST_CONDITION_PB3_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PB3_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PB3_LED_OTHERS_NC)
#  define INIT DIO_INIT_PB3 
#  define SET DIO_SET_PB3 
#  define READ DIO_READ_PB3 
#elif defined(TEST_CONDITION_PB4_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PB4_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PB4_LED_OTHERS_NC)
#  define INIT DIO_INIT_PB4 
#  define SET DIO_SET_PB4 
#  define READ DIO_READ_PB4 
#elif defined(TEST_CONDITION_PB5_HIGH_PB0_SIGNAL_LED_OTHERS_NC) || \
      defined(TEST_CONDITION_PB5_LOW_PB0_SIGNAL_LED_OTHERS_NC) || \
      defined(TEST_CONDITION_PB5_LED_OTHERS_NC)
#  define INIT DIO_INIT_PB5 
#  define SET DIO_SET_PB5 
#  define READ DIO_READ_PB5 

#elif defined(TEST_CONDITION_PC0_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PC0_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PC0_LED_OTHERS_NC)
#  define INIT DIO_INIT_PC0 
#  define SET DIO_SET_PC0 
#  define READ DIO_READ_PC0 
#elif defined(TEST_CONDITION_PC1_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PC1_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PC1_LED_OTHERS_NC)
#  define INIT DIO_INIT_PC1 
#  define SET DIO_SET_PC1 
#  define READ DIO_READ_PC1 
#elif defined(TEST_CONDITION_PC2_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PC2_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PC2_LED_OTHERS_NC)
#  define INIT DIO_INIT_PC2 
#  define SET DIO_SET_PC2 
#  define READ DIO_READ_PC2 
#elif defined(TEST_CONDITION_PC3_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PC3_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PC3_LED_OTHERS_NC)
#  define INIT DIO_INIT_PC3 
#  define SET DIO_SET_PC3 
#  define READ DIO_READ_PC3 
#elif defined(TEST_CONDITION_PC4_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PC4_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PC4_LED_OTHERS_NC)
#  define INIT DIO_INIT_PC4 
#  define SET DIO_SET_PC4 
#  define READ DIO_READ_PC4 
#elif defined(TEST_CONDITION_PC5_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PC5_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PC5_LED_OTHERS_NC)
#  define INIT DIO_INIT_PC5 
#  define SET DIO_SET_PC5 
#  define READ DIO_READ_PC5 

#elif defined(TEST_CONDITION_PD0_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PD0_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PD0_LED_OTHERS_NC)
#  define INIT DIO_INIT_PD0 
#  define SET DIO_SET_PD0 
#  define READ DIO_READ_PD0 
#elif defined(TEST_CONDITION_PD1_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PD1_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PD1_LED_OTHERS_NC)
#  define INIT DIO_INIT_PD1 
#  define SET DIO_SET_PD1 
#  define READ DIO_READ_PD1 
#elif defined(TEST_CONDITION_PD2_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PD2_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PD2_LED_OTHERS_NC)
#  define INIT DIO_INIT_PD2 
#  define SET DIO_SET_PD2 
#  define READ DIO_READ_PD2 
#elif defined(TEST_CONDITION_PD3_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PD3_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PD3_LED_OTHERS_NC)
#  define INIT DIO_INIT_PD3 
#  define SET DIO_SET_PD3 
#  define READ DIO_READ_PD3 
#elif defined(TEST_CONDITION_PD4_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PD4_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PD4_LED_OTHERS_NC)
#  define INIT DIO_INIT_PD4 
#  define SET DIO_SET_PD4 
#  define READ DIO_READ_PD4 
#elif defined(TEST_CONDITION_PD5_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PD5_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PD5_LED_OTHERS_NC)
#  define INIT DIO_INIT_PD5 
#  define SET DIO_SET_PD5 
#  define READ DIO_READ_PD5 
#elif defined(TEST_CONDITION_PD6_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PD6_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PD6_LED_OTHERS_NC)
#  define INIT DIO_INIT_PD6 
#  define SET DIO_SET_PD6 
#  define READ DIO_READ_PD6 
#elif defined(TEST_CONDITION_PD7_HIGH_OTHERS_NC) || \
      defined(TEST_CONDITION_PD7_LOW_OTHERS_NC) || \
      defined(TEST_CONDITION_PD7_LED_OTHERS_NC)
#  define INIT DIO_INIT_PD7 
#  define SET DIO_SET_PD7 
#  define READ DIO_READ_PD7 
#endif

// }}}1

  // Macros describing particular condition of pind being tested, depending
  // on which TEST_CONDITION_* is in effect.
  // {{{1
#if defined(TEST_CONDITION_PB0_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PB1_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PB2_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PB3_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PB4_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PB5_HIGH_PB0_SIGNAL_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PC0_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PC1_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PC2_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PC3_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PC4_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PC5_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PD0_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PD1_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PD2_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PD3_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PD4_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PD5_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PD6_HIGH_OTHERS_NC) || \
    defined(TEST_CONDITION_PD7_HIGH_OTHERS_NC)
#  define INPUT_HIGH
#elif defined(TEST_CONDITION_PB0_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PB1_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PB2_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PB3_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PB4_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PB5_LOW_PB0_SIGNAL_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PC0_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PC1_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PC2_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PC3_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PC4_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PC5_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PD0_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PD1_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PD2_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PD3_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PD4_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PD5_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PD6_LOW_OTHERS_NC) || \
    defined(TEST_CONDITION_PD7_LOW_OTHERS_NC)
#  define INPUT_LOW
#elif defined(TEST_CONDITION_PB0_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PB1_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PB2_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PB3_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PB4_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PB5_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PC0_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PC1_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PC2_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PC3_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PC4_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PC5_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PD0_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PD1_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PD2_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PD3_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PD4_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PD5_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PD6_LED_OTHERS_NC) || \
    defined(TEST_CONDITION_PD7_LED_OTHERS_NC)
#  define OUTPUT_LED 
#endif
  // }}}1

  // Test particular pins as inputs or outputs (depending ultimately on
  // which TEST_CONDITION_* is in effect).
  // {{{1

#if defined(INPUT_HIGH) || defined(INPUT_LOW)

  // NOTE: these tests depend on the pin in question being connected as
  // indicated by the test condition name.

  // Test pin as input with pull-up enabled.
  INIT (DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE); 
  _delay_ms (settling_time_ms);

  value = READ ();
#  if defined(INPUT_HIGH)
  assert (value == HIGH);
#  elif defined(INPUT_LOW)
  assert (value == LOW);
#  else
#    error Should not be here
#  endif

  // Test pin again as input but this time with pull-up disabled.
  INIT (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE); 
  _delay_ms (settling_time_ms);
  
  value = READ ();
#  if defined(INPUT_HIGH)
  assert (value == HIGH);
#  elif defined(INPUT_LOW)
  assert (value == LOW);
#  else
#    error Should not be here
#  endif

#elif defined(OUTPUT_LED)

  // NOTE: these tests depend on a LED being connected in the manner indicated
  // by the test condition name and also no careful human observation of
  // the LED during the test.

  const float second_in_ms = 1000.0;

  // Initialize pin off, then turn on for one second.
  INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  _delay_ms (second_in_ms);
  SET (HIGH);
  _delay_ms (second_in_ms);
  SET (LOW);
  _delay_ms (second_in_ms);

  // Now initialize pin on, then turn off for one second.
  INIT (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  _delay_ms (second_in_ms);
  SET (LOW);
  _delay_ms (second_in_ms);
  SET (HIGH);
  _delay_ms (second_in_ms);
  
#elif defined(TEST_CONDITION_ALL_PINS_NC)
#else
#  error Should not be here
#endif

  SIGNAL_CHECKPOINT ();
  return 0;   // Success (assuming observed LED behaved correctly).

  // }}}1
}
