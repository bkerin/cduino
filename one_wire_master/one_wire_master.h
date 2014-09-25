
#ifndef ONE_WIRE_MASTER_H
#define ONE_WIRE_MASTER_H

#include "dio.h"

#ifndef ONE_WIRE_MASTER_PIN
#  error ONE_WIRE_MASTER_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
#endif

// Intialize the one wire master interface.  All this does is set up the
// chosen DIO pin.  It starts out tri-stated (line released).
void
owm_init (void);

// Generate a 1-Wire reset.  Return 1 if a presence pulse was detected,
// or 0 otherwise.  NOTE: this is logically different than the comments for
// the OWTouchReset() function from Maxim application note AN126 indicate,
// since those seem backwards and confused.  NOTE: does not handle alarm
// presence from DS2404/DS1994.
uint8_t
owm_touch_reset (void);

// Write 1-Wire data byte
void
owm_write_byte (uint8_t data);

// Read 1-Wire data byte and return it
uint8_t
owm_read_byte (void);

// FIXME: do we like this name?
// Write a 1-Wire data byte and return the sampled result
uint8_t
owm_touch_byte (uint8_t data);

#endif // ONE_WIRE_MASTER_H
