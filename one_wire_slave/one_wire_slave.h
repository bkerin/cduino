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
#include "one_wire_common.h"

#ifndef OWS_PIN
#  error OWS_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
#endif

// We have our own namespace-prefixed names for the common one-wire commands.
// You probably won't want to use any of these directly when using this
// interface.
#define OWS_NULL_COMMAND         OWC_NULL_COMMAND
#define OWS_SLEEP_COMMAND        OWC_SLEEP_COMMAND
#define OWS_SEARCH_ROM_COMMAND   OWC_SEARCH_ROM_COMMAND
#define OWS_READ_ROM_COMMAND     OWC_READ_ROM_COMMAND
#define OWS_MATCH_ROM_COMMAND    OWC_MATCH_ROM_COMMAND
#define OWS_SKIP_ROM_COMMAND     OWC_SKIP_ROM_COMMAND
#define OWS_ALARM_SEARCH_COMMAND OWC_ALARM_SEARCH_COMMAND

// The first byte of the ROM ID is supposed to be a family code that is
// constant for all devices in a given "family".  If it hasn't been defined
// already, we assign a default.
#ifndef OWS_FAMILY_CODE
#  define OWS_FAMILY_CODE 0x42
#endif

// The second through seventh bytes of the ROM ID are supposed to be unique
// to each part.  There's support in ows_init() for loading a unique code
// from EEPROM, but in case clients don't want to deal with setting that
// up we have this default value.  The bytes given in this value are bytes
// two through seven, reading from left to right.  Watch out for the endian
// switch if you even try to compare ROM IDs as uint64_t numbers.
#define OWS_DEFAULT_PART_ID 0x444444222222

// If the use_eeprom_id argument to ows_init() is TRUE, this is the
// location where the six byte part ID is looked up in EEPROM, otherwise
// it is ignored.  A different address could be used here, but then the
// write_random_id_to_eeprom target of generic.mk would need to change
// as well or a different mechanism used to load the ID into EEPROM (see
// comments above the ows_init() declaration below.
#define OWS_PART_ID_EEPROM_ADDRESS 0

// Return type for function in this interface which can encounter errors.
// FIXME: figure out which of thest we end up using
typedef enum {
  OWS_ERROR_NONE = 0,
  OWS_ERROR_UNEXPECTED_PULSE_LENGTH,
  OWS_ERROR_LINE_UNEXPECTEDLY_LOW,
  OWS_ERROR_GOT_RESET_PULSE,
  OWS_ERROR_RESET_DETECTED_AND_HANDLED,
  OWS_ERROR_MISSED_RESET_DETECTED,
  OWS_ERROR_ROM_ID_MISMATCH,
} ows_error_t;

// Initialize the one-wire slave interface.  This sets up the chosen
// DIO pin, including pin change interrupts and global interrupt enable.
// If use_eeprom_id FIXME: make the EEPROM offset a tunable constant is
// true, the first six bytes of the AVR EEPROM are read and used together
// with the OWS_FAMILY_CODE and a CRC to form a slave ROM ID, which is
// loaded into RAM for speedy access.  See the write_random_id_to_eeprom
// target of generic.mk for a convenient way to load unique IDs onto devices
// (note that that target writes eight random bytes, of which only the first
// six are used by this interface).  If use_eeprom_id is false, a default
// device ID with a non-family part numberof OWS_DEFAULT_PART_ID is used
// (note that this arrangement is only useful if you intend to use only
// one of your slaves on the bus).
void
ows_init (uint8_t use_eeprom_id);

///////////////////////////////////////////////////////////////////////////////
//
// Reset Support and Individual Bit Functions
//
// FIXME: fix this out-of-date description
// This stuff supports reset and bit-at-a-time operations.  All the
// fundamental timing used in the one-wire protocol is implemented here,
// other functions in this interface are implemented in terms of these.
//
// The master could decide at any time to abort whatever operation is going
// on and and send us a reset pulse.

// Wait for a function command transaction intended for our ROM ID (and
// possibly others as well if a OWS_SKIP_ROM_COMMAND was sent), and return
// the function command itself.  In the meantime, automagically participate
// in any slave searches (i.e. respond to any incoming OWS_SEARCH_ROM_COMMAND
// or OWS_ALARM_SEARCH_COMMAND commands).  Any errors (funny-lengh pulses,
// aborted searches, etc.) are silently ignored.
uint8_t
ows_wait_for_function_command (void);

// Wait for a reset pulse, respond with a presence pulse, then try to
// read a single byte from the master and return it.  Any additional reset
// pulses that occur during this read attempt are also responded to with
// presence pulses.  Any errors that occur while trying to read the byte
// effectively cause a new wait for a reset pulse to begin.
uint8_t
ows_wait_for_command (void);

// ROM commands perform one-wire search and addressing operations and are
// effectively part of the one-wire protocol, as opposed to other commands
// which particular slave types may define to do slave-type-specific things.
#define OWS_IS_ROM_COMMAND(command) \
  ( command == OWS_SEARCH_ROM_COMMAND   || \
    command == OWS_READ_ROM_COMMAND     || \
    command == OWS_MATCH_ROM_COMMAND    || \
    command == OWS_SKIP_ROM_COMMAND     || \
    command == OWS_ALARM_SEARCH_COMMAND    )

// Block until a reset pulse from the master is seen, then produce a
// corresponding presence pulse and return.
void
ows_wait_for_reset (void);

// Write bit (which should be 0 or 1).
ows_error_t
ows_write_bit (uint8_t data_bit);

// Read bit, setting data_bit_ptr to 0 or 1.
ows_error_t
ows_read_bit (uint8_t *data_bit_ptr);

///////////////////////////////////////////////////////////////////////////////
//
// Device Presense Confirmation/Discovery
//
// These functions allow the presence of particular slaves to be confirmed,
// or the bus searched for all slaves, slave from or not from of a particular
// family, or slaves with an active alarm condition.
//

// One wire ID size in bytes
// FIXME: consolidate with constante from owm?  Or rename to OWS_ prefix
// FIXME: might be nice to use _SIZE for byte sizes, everywhere.  What do
// we do elsewhere?
// FIXME: maybe the common header should be one_wire_common.h, instead of
// one_wire_common_commands.h, so it could include this and be decently named
#define OWM_ID_BYTE_COUNT 8

// FIXME: all these functions need to turn into support for slave end
// of protocol.

// This function requires that exactly one slave be present on the bus.
// If we discover a slave, its ID is written into id_buf (which pust be
// a pointer to OWM_ID_BYTE_COUNT bytes of space) and TRUE is returned.
// If there is not exactly one slave present, the results of this function
// are undefined (later calls to this interface might behave strangely).

// This is the appropriate response to a OWS_READ_ROM_COMMAND.
// FIXME: consistify ID, id, rom_id names externally at least
ows_error_t
ows_write_rom_id (void);

// Answer a (just received) OWS_SEARCH_ROM_COMMAND by engaging in the search
// process described in Maxim Application Note AN187.
ows_error_t
ows_answer_search (void);

// Respond to a (just received) OWS_MATCH_ROM_COMMAND by reading up to
// OWM_ID_BYTE_COUNT bytes, one bit at a time, and matching the bits to
// our ROM ID.  Return OWS_ERROR_ROM_ID_MISMATCH as soon as we see a
// non-matching bit, or OWS_ERROR_NONE if all bits match (i.e. the master
// is talking to us).  Note that it isn't really an error if the ROM doesn't
// match, it just means the master doesn't want to talk to us.  This routine
// can also return other errors propagated from ows_read_bit().  FIXME:
// I guess all the routines should mention possible error return values,
// or else they should be described once in some central place.
ows_error_t
ows_read_and_match_rom_id (void);

// FIXME: it would be nice to add a filter for alarm search (EC command).
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

// Write up to eight bits in a row.  The least significant bit is written
// first.  Any error that can occur in the underlying ows_write_bit()
// routine is immediately propagated and returned by this routine.
ows_error_t
ows_write_byte (uint8_t data_byte);

// Read up to eight bits in a row.  The least significant bit is read first.
// Any error that can occur in the underlying ows_read_bit() routine is
// immediately propagated and returned by this routine.
ows_error_t
ows_read_byte (uint8_t *data_byte_ptr);

// Fancy simultaneous read/write.  Sort of.  I guess, I haven't used it. It's
// supposed to be more efficient.  See Maxim Application Note AN126. WARNING:
// FIXXME: This comes straight from AN126, but I haven't tested it.
// FIXME: is this how slave does it?
uint8_t
ows_touch_byte (uint8_t data);

#endif // ONE_WIRE_SLAVE_H
