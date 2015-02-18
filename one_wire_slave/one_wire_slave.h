// One-wire slave interface (software interface -- requires only one IO pin)
//
// Test driver: one_wire_slave_test.c    Implementation: one_wire_slave.c
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
#include "timer1_stopwatch.h"

// Timer1 Stopwatch Frequency Required.  Since we're timing quite short
// pulses, we don't want to deal with a timer that can't even measure
// them accurately.
#define OWS_T1SFR 1000000

#if F_CPU / TIMER1_STOPWATCH_PRESCALER_DIVIDER < OWS_T1SFR
#  error F_CPU / TIMER1_STOPWATCH_PRESCALER_DIVIDER is too small
#endif

#ifndef OWS_PIN
#  error OWS_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
#endif

// The first byte of the ROM ID is supposed to be a family code that is
// constant for all devices in a given "family".  If it hasn't been defined
// already, we assign a default.
#ifndef OWS_FAMILY_CODE
#  define OWS_FAMILY_CODE 0x42
#endif

// Using 0x00 as a family code value is not allowed, because the master
// considers it an error if it sees it, which is sensible because that's
// what a data-line-to-ground fault (or slave stuck grounding the line)
// ends up looking like during a search (and the search is the first point
// where this fault condition is detectable).
#if OWS_FAMILY_CODE == 0x00
#  error invalid OWS_FAMILY_CODE setting of 0x00
#endif

// The second through seventh bytes of the ROM ID are supposed to be unique
// to each part.  There's support in ows_init() for loading a unique code
// from EEPROM, but in case clients don't want to deal with setting that
// up we have this default value.  The bytes given in this value are bytes
// two through seven, reading from left to right.  Watch out for the endian
// switch if you even try to compare ROM IDs as uint64_t numbers.
#define OWS_DEFAULT_PART_ID 0x444444222222

// If the use_eeprom_id argument to ows_init() is TRUE, this is the location
// where the six byte part ID is looked up in EEPROM, otherwise it is ignored.
// A different address (integer offset value) could be used here, but then
// the write_random_id_to_eeprom target of generic.mk would need to change
// as well or a different mechanism used to load the ID into EEPROM (see
// comments above the ows_init() declaration below.
#define OWS_PART_ID_EEPROM_ADDRESS 0

// Return type for functions in this interface which report errors.
typedef enum {
  OWS_ERROR_NONE = 0,
  OWS_ERROR_UNEXPECTED_PULSE_LENGTH,
  OWS_ERROR_DID_NOT_GET_ROM_COMMAND,
  OWS_ERROR_RESET_DETECTED_AND_HANDLED,
  OWS_ERROR_ROM_ID_MISMATCH,
  OWS_ERROR_NOT_ALARMED,
} ows_error_t;

// Initialize the one-wire slave interface.  This sets up the chosen
// DIO pin, including pin change interrupts and global interrupt enable.
// If use_eeprom_id is true, the first six bytes of the AVR EEPROM are
// read and used together with the OWS_FAMILY_CODE and a CRC to form a
// slave ROM ID, which is loaded into RAM for speedy access.  In this
// case, interrupts will be blocked for a while while the EEPROM is read.
// See the write_random_id_to_eeprom target of generic.mk for a convenient
// way to load unique IDs onto devices (note that that target writes eight
// random bytes, of which only the first six are used by this interface).
// If use_eeprom_id is false, a default device ID with a non-family part
// numberof OWS_DEFAULT_PART_ID is used (note that this arrangement is only
// useful if you intend to use only one of your slaves on the bus).
void
ows_init (uint8_t use_eeprom_id);

// Wait for the initiation of a function command transaction intended
// for our ROM ID (and possibly others as well if a OWS_SKIP_ROM_COMMAND
// was sent), and return the function command itself in *command_ptr.
// See Maxim_DS18B20_datasheet.pdf page 10, "TRANSACTION SEQUENCE" section.
// In the meantime, automagically respond to resets and participate in any
// slave searches (i.e. respond to any incoming OWC_SEARCH_ROM_COMMAND
// or OWC_ALARM_SEARCH_COMMAND commands).  An error is generated if
// any unexpected line behavior is encountered (abnormal pulse lengths,
// unexpected mid-byte resets, etc.).  An error is *not* generated when
// a potential transaction is aborted by the master at the end of one of
// the steps listed in the "TRANSACTION SEQUENCE" section of the datasheet.
// This policy allows this routine to automagically handle all ROM SEARCH,
// READ ROM and the like, but also permits the somewhat weird case in
// which the master sends a command that is useless except for transaction
// initiation (a MATCH_ROM or SKIP_ROM), then doesn't actually follow it
// with a function command.  An actual DS18B20 appears to tolerate this
// misbehavior as well, however.  On the other hand we *do not* tolerate
// function commands that aren't preceeded by a ROM command.  The DS18B20
// seems to tolerate this (when its the only device on the bus), but its
// completely contrary to the proscribed behavior for masters to do that,
// and makes the implementation of this routine harder.  Use the lower-lever
// portion of this interface if you know you have only one slave and don't
// want to bother with addressing.  Masters that fail to send an entire
// byte after issuing a reset pulse will cause this routine to hang until
// they get around to doing that (or issuing another reset pulse).
ows_error_t
ows_wait_for_function_transaction (uint8_t *command_ptr);

// Wait for a reset pulse, respond with a presence pulse, then try to read
// a single byte from the master and return it.  An error is generated if
// any unexpected line behavior is encountered (abnormal pulse lengths,
// unexpected mid-byte resets, etc.  Any additional reset pulses that occur
// during this read attempt are also responded to with presence pulses
// (and OWS_ERROR_RESET_DETECTED_AND_HANDLED is returned).  Any errors that
// occur while trying to read the byte effectively cause a new wait for a
// reset pulse to begin.
ows_error_t
ows_wait_for_command (uint8_t *command_ptr);

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

#endif // ONE_WIRE_SLAVE_H


///////////////////////////////////////////////////////////////////////////////
//
// Device Presense Confirmation/Discovery Support
//
// These routines support slave participation in master-initiated device
// discovery.  Note that ows_wait_for_function_transaction() can handle
// this stuff automagically.
//

// This is the appropriate response to a OWC_READ_ROM_COMMAND.
ows_error_t
ows_write_id (void);

// Answer a (just received) OWC_SEARCH_ROM_COMMAND by engaging in the search
// process described in Maxim_Application_Note_AN187.pdf.
ows_error_t
ows_answer_search (void);

// If set to a non-zero value, this flag indicates an alarm condition.
// Clients of this interface can ascribe particular meanings to particular
// non-zero values if desired.  Slaves with an alarm condition should respond
// to any OWC_ALARM_SEARCH_COMMAND issued by thier master (and will do so
// automagically in ows_wait_for_function_command()).
extern uint8_t ows_alarm;

// Like ows_answer_search(), but only participates if ows_alarm is non-zero.
// This is the correct reaction to an OWC_ALARM_SEARCH_COMMAND.  If ows_alarm
// is zero, this macro evaluates to OWS_ERROR_NOT_ALARMED.
#define OWS_MAYBE_ANSWER_ALARM_SEARCH() \
  (ows_alarm ? ows_answer_search () : OWS_ERROR_NOT_ALARMED)

// Respond to a (just received) OWC_MATCH_ROM_COMMAND by reading up to
// OWC_ID_SIZE_BYTES bytes, one bit at a time, and matching the bits to our
// ROM ID.  Return OWS_ERROR_ROM_ID_MISMATCH as soon as we see a non-matching
// bit, or OWS_ERROR_NONE if all bits match (i.e. the master is talking
// to us).  Note that it isn't really an error if the ROM doesn't match,
// it just means the master doesn't want to talk to us.  This routine can
// also return other errors propagated from ows_read_bit().
ows_error_t
ows_read_and_match_id (void);


