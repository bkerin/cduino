// One-wire master interface (software interface -- requires only one IO pin)
//
// Test driver: one_wire_master_test.h    Implementation: one_wire_master.c
//
// If you're new to one-wire you should first read the entire Maxim DS18B20
// temperature sensor datasheet FIXME: link it.  Its hard to use one-wire
// without at least a rough understanding of how the line signalling and
// transaction schemes work.
//
// This interface features high-level routines that can handle all the
// back-and-forth required to initiate a one-wire command transaction or
// slave search or verify, and also access to the lower-level one-wire
// functionality, such as bit- and byte-at-a-time communication.  Note that
// the latter low-level functions are typically required to usefully complete
// a transaction.  FIXME: actually provide the transaction-level interface.

#ifndef ONE_WIRE_MASTER_H
#define ONE_WIRE_MASTER_H

#include "dio.h"
#include "one_wire_common.h"

#ifndef OWM_PIN
#  error OWM_PIN not defined (it must be explicitly set to one of \
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
// Reset and Individual Bit Functions
//
// These function perform reset or bit-at-a-time operations.  All the
// fundamental timing used in the one-wire protocol is implemented in these
// functions, other functions in this interface are implemented in terms
// of these.
//

// Generate a 1-Wire reset.  Return TRUE if a resulting presence pulse is
// detected, or FALSE otherwise.  NOTE: this is logically different than
// the comments for the OWTouchReset() function from Maxim application
// note AN126 indicate, since those seem backwards and confused.  FIXME:
// I don't think its wrong relative to their code, so the question is why
// do they do it that way?  Does it have to do with the search?  NOTE:
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
// or the bus searched for all slaves.  FIXXME: searches for slaves from
// particular families or with active alarm conditions aren't supported yet.
//

// One wire ID size in bytes
#define OWM_ID_BYTE_COUNT 8

// This function requires that exactly one slave be present on the bus.
// If we discover a slave, its ID is written into id_buf (which pust be
// a pointer to OWM_ID_BYTE_COUNT bytes of space) and TRUE is returned.
// If there is not exactly one slave present, the results of this function
// are undefined (later calls to this interface might behave strangely).
uint8_t
owm_read_id (uint8_t *id_buf);

// Find the "first" slave on the one-wire bus (in the sense of the discovery
// order of the one-wire search algorithm described in Maxim application
// note AN187).  If a slave is discovered, its ID is written into id_buf
// (which mucst be a pointer to OWM_ID_BYTE_COUNT bytes of space) and TRUE
// is returned.  If no slave is discovered, FALSE is returned.  Note that
// this resets any search which is already in progress.
uint8_t
owm_first (uint8_t *id_buf);

// Require an immediately preceeding call to owm_first() or owm_next() to
// have occurred.  Find the "next" slave on the one-wire bus (in the sense of
// the discovery order of the one-wire search algorithm described in Maxim
// application note AN187).  This continues a search begun by a previous
// call to owm_first().  If another slave is found, its ID is written into
// id_buf (which must be a pointer to OWM_ID_BYTE_COUNT bytes of space and
// TRUE is returned.  If no additional slave is found, FALSE is returned.
uint8_t
owm_next (uint8_t *id_buf);

// Return true iff device with ID equal to the value in the OWM_ID_BYTE_COUNT
// bytes pointed to by id_buf is present on the bus, or FALSE otherwise.
// Note that unlike owm_read_id(), this function is safe to use when there
// are multiple devices on the bus.  When this function returns, the global
// search state is restored (so for example the next call to owm_next()
// should behave as if the call to this routine never occurred).
uint8_t
owm_verify (uint8_t *id_buf);

// Like owm_first(), but only finds slaves with an active alarm condition.
uint8_t
owm_first_alarmed (uint8_t *id_buf);

// Like owm_next(), but only finds slaves with an active alarm condition.
uint8_t
owm_next_alarmed (uint8_t *id_buf);

// FIXXME: it would be nice to add a filter for alarm search (EC command).
// This command might actually be useful, since it makes it possible to scan
// an entire bus for any devices needing immediate attention.  It wouldn't
// be too hard to add support for this either: I think all that would be
// required would be for the search() function in one_wire_master.c to
// take a alarm_only argument, and then issue an EC command instead of an
// OWC_SEARCH_ROM_COMMAND if that argument was true.  It could be tested
// using the DS18B20 temperature alarm functionality, but setting those
// alarms up is somewhat of a hassle, so we haven't bothered yet.

///////////////////////////////////////////////////////////////////////////////
//
// Byte Write/Read
//

// Write byte.  The LSB is written first.
void
owm_write_byte (uint8_t data);

// Read byte.  The LSB is read first.
uint8_t
owm_read_byte (void);

// Fancy simultaneous read/write.  Sort of.  I guess, I haven't used it. It's
// supposed to be more efficient.  See Maxim Application Note AN126. WARNING:
// FIXXME: This comes straight from AN126, but I haven't tested it.
uint8_t
owm_touch_byte (uint8_t data);

#endif // ONE_WIRE_MASTER_H
