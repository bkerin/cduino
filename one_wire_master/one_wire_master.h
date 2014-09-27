
#ifndef ONE_WIRE_MASTER_H
#define ONE_WIRE_MASTER_H

#include "dio.h"

#ifndef ONE_WIRE_MASTER_PIN
#  error ONE_WIRE_MASTER_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
#endif

// Intialize the one wire master interface.  All this does is set up the
// chosen DIO pin.  It starts out set as an input without the internal pull-up
// enabled.  It would probably be possible to use the internal pull-up on
// the AVR microcontroller for short-line communication at least, but the
// datasheet for the part I've used for testing (Maxim DS18B20) calls for a
// much stronger pull-up, so for simplicity the internal pull-up is disabled.
void
owm_init (void);

// Generate a 1-Wire reset.  Return TRUE if a resulting presence pulse is
// detected, or FALSE otherwise.  NOTE: this is logically different than
// the comments for the OWTouchReset() function from Maxim application
// note AN126 indicate, since those seem backwards and confused.  NOTE:
// does not handle alarm presence from DS2404/DS1994.
uint8_t
owm_touch_reset (void);

// Write byte
void
owm_write_byte (uint8_t data);

// Read byte
uint8_t
owm_read_byte (void);

// Fancy simultaneous read/write.  Sort of weird.  See Maxim application
// note AN126. WARNING: FIXME: I haven't tested this.
uint8_t
owm_touch_byte (uint8_t data);

#endif // ONE_WIRE_MASTER_H
