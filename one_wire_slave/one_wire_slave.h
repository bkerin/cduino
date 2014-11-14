// One-wire slave interface (software interface -- requires only one IO pin)
//
// Test driver: one_wire_slave_test.h    Implementation: one_wire_slave.c
//
// This interface provides a framework for creating slave devices compatible
// with the Maxim one-wire slave protocol, or close (IMO its often more
// convenient to create almost-compatible devices which aren't quite so
// worried about timing or honoring every request).  Maxim isn't helping
// us create 1-wire slaves, its all based on Maxim Application note AN126
// and mimicking the behavior of the Maxim DS18B20.

#ifndef ONE_WIRE_SLAVE_H
#define ONE_WIRE_SLAVE_H

#include "dio.h"

#ifndef OWS_PIN
#  error OWS_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
#endif

// Intialize the one wire slave interface.  This sets up the chosen DIO
// pin, including pin change interrupts and global interrupt enable.
// FIXME: pull-up?  and documentation for pull-up?
void
ows_init (void);

///////////////////////////////////////////////////////////////////////////////
//
// Individual Bit Functions
//
// These function perform bit-at-a-time operations.  All the fundamental
// timing used in the one-wire protocol is implemented in these functions,
// other functions in this interface are implemented in terms of these.
//

// FIXME: well this should work but its busy... of course we would probably
// like to be able to sleep and wake up on a pin change interrupt.
void
ows_wait_for_reset (void);

// Wait for a reset, and signal our presence when we receive one.
void
ows_present (void);

// Write bit
// FIXME: is this how slaves write bits?
void
ows_write_bit (uint8_t value);

// Read bit
// FIXME: is this how slaves read bits?
uint8_t
ows_read_bit (void);

///////////////////////////////////////////////////////////////////////////////
//
// Device Presense Confirmation/Discovery
//
// These functions allow the presence of particular slaves to be confirmed,
// or the bus searched for all slaves, slave from or not from of a particular
// family, or slaves with an active alarm condition.
//

// When these commands occur after a reset, the slaves interpret them as
// directions to begin participating in various ID search/discovery commands.
// Note that clients don't generally need to use these macros directly.
// See the DS18B20 datasheet and Maxim application note AN187 for details.
#define OWM_READ_ROM_COMMAND   0x33
#define OWM_SEARCH_ROM_COMMAND 0xF0

// One wire ID size in bytes
#define OWM_ID_BYTE_COUNT 8

// FIXME: all these functions need to turn into support for slave end
// of protocol.

// This function requires that exactly one slave be present on the bus.
// If we discover a slave, its ID is written into id_buf (which pust be
// a pointer to OWM_ID_BYTE_COUNT bytes of space) and TRUE is returned.
// If there is not exactly one slave present, the results of this function
// are undefined (later calls to this interface might behave strangely).
uint8_t
ows_read_id (uint8_t *id_buf);

// Find the "first" slave on the one-wire bus (in the sense of the discovery
// order of the one-wire search algorithm described in Maxim application
// note AN187).  If a slave is discovered, its ID is written into id_buf
// (which mucst be a pointer to OWM_ID_BYTE_COUNT bytes of space) and TRUE
// is returned.  If no slave is discovered, FALSE is returned.  Note that
// this resets any search which is already in progress.
uint8_t
ows_first (uint8_t *id_buf);

// Require an immediately preceeding call to ows_first() or ows_next() to
// have occurred.  Find the "next" slave on the one-wire bus (in the sense of
// the discovery order of the one-wire search algorithm described in Maxim
// application note AN187).  This continues a search begun by a previous
// call to ows_first().  If another slave is found, its ID is written into
// id_buf (which must be a pointer to OWM_ID_BYTE_COUNT bytes of space and
// TRUE is returned.  If no additional slave is found, FALSE is returned.
uint8_t
ows_next (uint8_t *id_buf);

// Return true iff device with ID equal to the value in the OWM_ID_BYTE_COUNT
// bytes pointed to by id_buf is present on the bus, or FALSE otherwise.
// Note that unlike ows_read_id(), this function is safe to use when there
// are multiple devices on the bus.  When this function returns, the global
// search state is restored (so for example the next call to ows_next()
// should behave as if the call to this routine never occurred).
uint8_t
ows_verify (uint8_t *id_buf);

// FIXXME: it would be nice to add a filter for alarm search (EC command).
// This command might actually be useful, since it makes it possible to scan
// an entire bus for any devices needing immediate attention.  It wouldn't
// be too hard to add support for this either: I think all that would be
// required would be for the search() function in one_wire_master.c to
// take a alarm_only argument, and then issue an EC command instead of an
// OWM_SEARCH_ROM_COMMAND if that argument was true.  It could be tested
// using the DS18B20 temperature alarm functionality, but setting those
// alarms up is somewhat of a hassle, so we haven't bothered yet.

///////////////////////////////////////////////////////////////////////////////
//
// Byte Write/Read
//

// Write byte
// FIXME: is this how slave does it?
void
ows_write_byte (uint8_t data);

// Read byte
// FIXME: is this how slave does it?
uint8_t
ows_read_byte (void);

// Fancy simultaneous read/write.  Sort of.  I guess, I haven't used it. It's
// supposed to be more efficient.  See Maxim Application Note AN126. WARNING:
// FIXXME: This comes straight from AN126, but I haven't tested it.
// FIXME: is this how slave does it?
uint8_t
ows_touch_byte (uint8_t data);

#endif // ONE_WIRE_SLAVE_H
