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

// The one-wire protocol says that the device ID consists of an 8-bit family
// code (which is the same for all devices of a given type, followed by a
// unique 48-bit part code, followed by an 8 bit CRC of the previous bits).
#define OWS_PART_ID_SIZE_BITS 48

// The number of CRC bits in the ROM ID.
#define OWS_PART_ID_CRC_SIZE_BITS 8

// Offset of the family code in bits from the LSB of the ROM ID.
#define OWS_FAMILY_CODE_OFFSET \
  (OWS_PART_ID_SIZE_BITS + OWS_PART_ID_CRC_SIZE_BITS)

// If a family code hasn't been specified already, assign a default.
#ifndef OWS_FAMILY_CODE
#  define OWS_FAMILY_CODE 0x42
#endif

// Default ID (excluding the trailing CRC), in case the user doesn't want
// to bother providing one.  Note that we put the bytes in this literal in
// the order we want them to go in the ID.  To compare this to a literal
// uint64_t value you need to swap the bytes of the literal (or just don't
// do that).  The last byte is 0x00 for now, it gets filled in with the
// CRC value at initialization-time.
#define OWS_DEFAULT_ID \
  ((((uint64_t) OWS_FAMILY_CODE) << OWS_FAMILY_CODE_OFFSET) & 0x42424242424200)

// Return type for function in this interface which can encounter errors.
// FIXME: figure out which of thest we end up using
typedef enum {
  OWS_ERROR_NONE = 0,
  OWS_ERROR_UNEXPECTED_PULSE_LENGTH,
  OWS_ERROR_LINE_UNEXPECTEDLY_LOW,
  OWS_ERROR_GOT_RESET_PULSE,
  OWS_ERROR_RESET_DETECTED_AND_HANDLED,
  OWS_ERROR_MISSED_RESET_DETECTED
} ows_error_t;

// Initialize the one-wire slave interface.  This sets up the chosen DIO
// pin, including pin change interrupts and global interrupt enable (see
// comments near the got_reset declaration below).  If load_eeprom_id
// FIXME: make the EEPROM offset a tunable constant
// is true, the first 64 bits of the AVR EEPROM are read and used to
// form a slave ID, which is loaded into RAM for speedy access.  See the
// write_random_id_to_eeprom target of generic.mk for a convenient way to
// load unique IDs onto devices.  If load_eeprom_id is false, a default
// device ID of OWS_DEFAULT_ID is used (note that this arrangement is only
// useful if you intend to use only one of your slaves on the bus).
void
ows_init (uint8_t load_eeprom_id);

///////////////////////////////////////////////////////////////////////////////
//
// Reset Support and Individual Bit Functions
//
// This stuff supports reset and bit-at-a-time operations.  All the
// fundamental timing used in the one-wire protocol is implemented here,
// other functions in this interface are implemented in terms of these.
//
// The master could decide at any time to abort whatever operation is going
// on and and send us a reset pulse.

// Wait for a reset pulse, respond with a presence pulse, then read a
// single byte from the master and return it.  This ignores any short or
// weird-length pulses on the line without any complaints.
uint8_t
ows_wait_for_command (void);

// Block until a reset pulse from the master is seen, then produce a
// corresponding presence pulse and return.
void
ows_wait_for_reset (void);

// Write bit
ows_error_t
ows_write_bit (uint8_t data_bit);

// Read bit
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

// When these commands occur after a reset, the slaves interpret them as
// directions to begin participating in various ID search/discovery commands.
// Note that clients don't generally need to use these macros directly.
// See the DS18B20 datasheet and Maxim application note AN187 for details.
// FIXME: consolidate with constante from owm?
#define OWS_READ_ROM_COMMAND   0x33
#define OWS_SEARCH_ROM_COMMAND 0xF0

// One wire ID size in bytes
// FIXME: consolidate with constante from owm?  Or rename to OWS_ prefix
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

// Write up to eight bits in a row, checking for got_reset after each bit.
// The LSB of data is written first.  If got_reset becomes true, return
// immediately (the returned value will be invalid).  This early return
// is done to allow slave implementations using this interface to react
// to reset pulses in time.
ows_error_t
ows_write_byte (uint8_t data_byte);

// Read byte
// FIXME: is this how slave does it?

// Read up to eight bits in a row, checking for got_reset after each bit.
// The LSB bit of data is read first.  If got_reset becomes true, return
// immediately (the returned value will be invalid).  This early return
// is done to allow slave implementations using this interface to react
// to reset pulses in time.
ows_error_t
ows_read_byte (uint8_t *data_byte_ptr);

///////////////////////////////////////////////////////////////////////////////
//
// Device Presense Confirmation/Discovery
//
// These functions can be used to allow the slave to participate in bus
// searches or device checks.
//

// When these commands occur after a reset, the slaves should interpret
// them as directions to begin participating in various ID search/discovery
// commands.  Note that clients don't generally need to use these macros
// directly.  See the DS18B20 datasheet and Maxim application note AN187
// for details.
#define OWS_READ_ROM_COMMAND   0x33
#define OWS_SEARCH_ROM_COMMAND 0xF0


// Fancy simultaneous read/write.  Sort of.  I guess, I haven't used it. It's
// supposed to be more efficient.  See Maxim Application Note AN126. WARNING:
// FIXXME: This comes straight from AN126, but I haven't tested it.
// FIXME: is this how slave does it?
uint8_t
ows_touch_byte (uint8_t data);

#endif // ONE_WIRE_SLAVE_H
