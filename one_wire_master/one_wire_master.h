
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

///////////////////////////////////////////////////////////////////////////////
//
// Individual Bit Functions
//
// These function perform bit-at-a-time operations.  All the fundamental
// timing used in the one-wire protocol are implemented in these functions,
// other functions in this interface are implemented in terms of these.
//

// Generate a 1-Wire reset.  Return TRUE if a resulting presence pulse is
// detected, or FALSE otherwise.  NOTE: this is logically different than
// the comments for the OWTouchReset() function from Maxim application
// note AN126 indicate, since those seem backwards and confused.  NOTE:
// does not handle alarm presence from DS2404/DS1994.
uint8_t
owm_touch_reset (void);

// Write bit
void
owm_write_bit (uint8_t value);

// Read bit
uint8_t
owm_read_bit (void);

///////////////////////////////////////////////////////////////////////////////
//
// Device Presense Confirmation/Discovery
//
// These functions allow the presence of particular slaves to be confirmed,
// or the bus searched for all slaves.
//

// The slaves interpret these bytes as directions to begin participating
// in various ID search/discovery commands.  See the DS18B20 datasheet and
// Maxim application note AN187 for details.
// FIXME: these need OWM_ prefix
#define READ_ROM_COMMAND   0x33
#define SEARCH_ROM_COMMAND 0xF0

// One wire ID size in bytes
#define OWM_ID_BYTE_COUNT 8

// This function requires that exactly one slave be present on the bus.
// It returns the discovered ID of that device in id_buf (which must be a
// pointer to OWM_ID_BYTE_COUNT bytes of space).  Returns TRUE on success.
// If there is not exactly one device present, FALSE will probably be
// returned.  But don't do that.  FIXME: why not?  make sure that works
uint8_t
owm_read_id (uint8_t *id_buf);

// Find the "first" device on the one-wire bus (in the sense of the
// discovery order of the one-wire search algorithm described in Maxim
// application note AN187).  Note that this resets any search which is
// already in progress.  Returns TRUE if a device is found and writes its
// ID into the OWM_ID_BYTE_COUNT bytes pointed to by id_buf, or FALSE if
// no device is found.
uint8_t
owm_first (uint8_t *id_buf);

// Require an immediately preceeding call to owm_first() or owm_next()
// to have occurred.  Find the "next" device on the one-wire bus (in the
// sense of the discovery order of the one-wire search algorithm described
// in Maxim application note AN187).  This continues a search begun by a
// previous call to owm_first().  Returns TRUE if another device is found
// and writes its ID into the OWM_ID_BYTE_COUNT bytes pointed to by id_buf,
// or FALSE if no additional device is found.
uint8_t
owm_next (uint8_t *id_buf);

// Return true iff device with ID equal to the value in the OWM_ID_BYTE_COUNT
// bytes pointed to by id_buf is present on the bus, or FALSE otherwise.
// Note that unlike owm_read_id(), this function is safe to use when there
// are multiple devices on the bus.
uint8_t
owm_verify (uint8_t *id_buf);

///////////////////////////////////////////////////////////////////////////////
//
// Byte Write/Read
//

// Write byte
void
owm_write_byte (uint8_t data);

// Read byte
uint8_t
owm_read_byte (void);

// Fancy simultaneous read/write.  Sort of.  I guess, I haven't used it. See
// Maxim application note AN126. WARNING: FIXME: I haven't tested this.
uint8_t
owm_touch_byte (uint8_t data);

#endif // ONE_WIRE_MASTER_H
