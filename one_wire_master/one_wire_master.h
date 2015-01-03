// One-wire master interface (software interface -- requires only one IO pin)
//
// Test driver: one_wire_master_test.c    Implementation: one_wire_master.c
//
// If you're new to one-wire you should first read the entire
// Maxim_DS18B20_datasheet.pdf.  Its hard to use one-wire without at
// least a rough understanding of how the line signalling and transaction
// schemes work.
//
// This interface features high-level routines that can handle all the
// back-and-forth required to initiate a one-wire command transaction or
// slave search or verify, and also access to the lower-level one-wire
// functionality, such as bit- and byte-at-a-time communication.  Note that
// the latter low-level functions are typically required to usefully complete
// a transaction.

#ifndef ONE_WIRE_MASTER_H
#define ONE_WIRE_MASTER_H

#include "dio.h"
#include "one_wire_common.h"

#ifndef OWM_PIN
#  error OWM_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
#endif

// These are the result codes returned by many routines in this
// interface.  X macros are used here to ensure we stay in sync with the
// owm_result_as_string() function.
#define OWM_RESULT_CODES                                                      \
                                                                              \
  /* No error.  Note that this code must be first.                         */ \
  X (OWM_ERROR_NONE)                                                          \
                                                                              \
  /* The master (that's us) sent a reset pulse, but didn't receive any     */ \
  /* slave response pulse.  This can happen when there's no slave present, */ \
  /* or perhaps if the slaves are all busy, if they are the sort that      */ \
  /* don't always honor reset pulses :).                                   */ \
  X (OWM_ERROR_DID_NOT_GET_PRESENCE_PULSE)                                    \
                                                                              \
  /* This occurs when either owm_next() or owm_next_alarmed() fails to     */ \
  /* find another slave, or when owm_first_alarmed() fails to find a first */ \
  /* alarmed slave.                                                        */ \
  X (OWM_ERROR_NO_SUCH_SLAVE)                                                 \
                                                                              \
  /* Got one values for both a bit and its compliment, in a situation      */ \
  /* where this shouldn't happen (i.e. not during the first bit of an      */ \
  /* owm_alarm_first() call)                                               */ \
  X (OWM_ERROR_UNEXPECTEDLY_GOT_ONES_FOR_BIT_AND_ITS_COMPLIMENT)              \
                                                                              \
  /* The master (that's us) received a ROM ID with an inconsistent CRC     */ \
  /* value.                                                                */ \
  X (OWM_ERROR_GOT_ROM_ID_WITH_INCORRECT_CRC_BYTE)                            \
                                                                              \
  /* Caller supplied an invalid ROM command argument (one that doesn't     */ \
  /* satisfy OWM_IS_TRANSACTION_INITIATING_ROM_COMMAND()).  This is a      */ \
  /* caller bug.                                                           */ \
  X (OWM_ERROR_GOT_INVALID_TRANSACTION_INITIATION_COMMAND)                    \
                                                                              \
  /* Caller supplied an invalid function command argument (one that does   */ \
  /* satisfy OWM_IS_ROM_COMMAND()).  This is a caller bug.                 */ \
  X (OWM_ERROR_GOT_ROM_COMMAND_INSTEAD_OF_FUNCTION_COMMAND)                   \
                                                                              \
  /* Some unknown problem occurred (probably a bug).                       */ \
  X (OWM_ERROR_UNKNOWN_PROBLEM)

// Return type representing the result of an operation.
typedef enum {
#define X(result_code) result_code,
  OWM_RESULT_CODES
#undef X
} owm_error_t;

#ifdef OWM_BUILD_RESULT_DESCRIPTION_FUNCTION

#  define OWM_RESULT_DESCRIPTION_MAX_LENGTH 81

// Put the string form of result in buf.  The buf argument must point to
// a memory space large enough to hold OWM_RESULT_DESCRIPTION_MAX_LENGTH +
// 1 bytes.  Using this function will make your program quite a bit bigger.
// As a convenience, buf is returned.
char *
owm_result_as_string (owm_error_t result, char *buf);

#endif

// Intialize the one wire master interface.  All this does is set up the
// chosen DIO pin.  It starts out set as an input without the internal pull-up
// enabled.  It would probably be possible to use the internal pull-up on
// the AVR microcontroller for short-line communication at least, but the
// datasheet for the part I've used for testing (Maxim DS18B20) calls for a
// much stronger pull-up, so for simplicity the internal pull-up is disabled.
void
owm_init (void);

// Start the transaction sequence as described in the Maxim DS18B20
// datasheet, "TRANSACTION SEQUENCE" section.
//
// Arguments:
//
//   rom_cmd          May be OWC_READ_ROM_COMMAND (if there's only one slave on
//                    the bus), OWC_MATCH_ROM_COMMAND, or OWC_SKIP_ROM_COMMAND
//
//   rom_id           For OWC_READ_ROM_COMMAND, this cantains the read ROM ID
//                    on return.  For OWC_MATCH_ROM_COMMAND, it must contain
//                    the ROM ID being addressed.  For OWC_SKIP_ROM_COMMAND it
//                    is unused (and may be NULL)
//
//   function_cmd     The function command to send.  This must not be a ROM
//                    command
//
// Return:
//
//   OWM_ERROR_NONE on succes, or an error code indicating the problem
//   otherwise.  If the arguments are valid, the OWC_MATCH_ROM_COMMAND
//   and OWC_SKIP_ROM_COMMAND flavors of this routine should
//   always succeed.  The OWC_READ_ROM_COMMAND flavor will return
//   OWM_ERROR_GOT_ROM_ID_WITH_INCORRECT_CRC_BYTE when it detects a CRC
//   mismatch on the read ROM ID.
//
// To actually complete the transaction, some slave- and transaction-specific
// back-and-forth using the lower level functions in this interface will
// likely be required.  Note that this routine cannot by itself ensure that
// the slave has received any OWC_MATCH_ROM_COMMAND or OWC_SKIP_ROM_COMMAND
// command correctly, since those don't elicit any response from the slave
// (though they do change its state).  The function_command likely does
// elicit a response, but this routine doesn't read it, so correct receipt
// of that command also cannot be verified by this routine.
owm_error_t
owm_start_transaction (uint8_t rom_cmd, uint8_t *rom_id, uint8_t function_cmd);

///////////////////////////////////////////////////////////////////////////////
//
// Reset and Individual Bit Functions
//
// These function perform reset or bit-at-a-time operations.  All the
// fundamental timing used in the one-wire protocol is implemented in these
// functions, other functions in this interface are implemented in terms
// of these.
//

// Generate a 1-Wire reset.  Return TRUE if a resulting presence
// pulse is detected, or FALSE otherwise.  NOTE: this is logically
// different than the comments for the OWTouchReset() function from
// Maxim_Application_Note_AN126.pdf indicate it uses.  NOTE: does not handle
// alarm presence from DS2404/DS1994.
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

// This function requires that exactly zero or one slaves be present on the
// bus.  If we discover a slave, its ID is written into id_buf (which must
// be a pointer to OWC_ID_SIZE_BYTES bytes of space) and OWM_ERROR_NONE is
// returned.  If no slave responds to our presence pulse, FALSE is returned.
// If there are two or more slaves present, the results of this function
// are undefined (later calls to this interface might behave strangely).
//
// Possible errors:
//
//   * OWM_ERROR_DID_NOT_GET_PRESENCE_PULSE
//   * OWM_ERROR_GOT_ROM_ID_WITH_INCORRECT_CRC_BYTE
//
// The contents of id_buf are unspecified if an error is returned.
//
owm_error_t
owm_read_id (uint8_t *id_buf);

// Find the "first" slave on the one-wire bus (in the sense of the discovery
// order of the one-wire search algorithm described in Maxim application
// note AN187).  If a slave is discovered, its ID is written into id_buf
// (which mucst be a pointer to OWC_ID_SIZE_BYTES bytes of space) and
// OWM_ERROR_NONE is returned, otherwise a non-zero error code is returned.
// Note that this resets any search which is already in progress.
owm_error_t
owm_first (uint8_t *id_buf);

// Require an immediately preceeding call to owm_first() or owm_next() to
// have occurred.  Find the "next" slave on the one-wire bus (in the sense of
// the discovery order of the one-wire search algorithm described in Maxim
// application note AN187).  This continues a search begun by a previous
// call to owm_first().  If another slave is found, its ID is written into
// id_buf (which must be a pointer to OWC_ID_SIZE_BYTES bytes of space and
// TRUE is returned.  If no additional slave is found, FALSE is returned.
uint8_t
owm_next (uint8_t *id_buf);

// Return true iff device with ID equal to the value in the OWC_ID_SIZE_BYTES
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
