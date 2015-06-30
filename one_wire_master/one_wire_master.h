// One-wire master interface (software interface -- requires only one IO pin)
//
// Test driver: one_wire_master_test.c    Implementation: one_wire_master.c
//
// If you're new to 1-wire you should first read the entire
// Maxim_DS18B20_datasheet.pdf.  Its hard to use 1-wire without at least a
// rough understanding of how the line signalling and transaction schemes
// work.
//
// This interface features high-level routines that can handle all the
// back-and-forth required to scan the bus or initiate a 1-wire command
// transaction, and also lower-level 1-wire functionality, such as bit-
// and byte-at-a-time communication.  Note that the latter low-level
// functions are typically required to usefully complete a transaction.
// The higher-level routines are presented first in this interface.

#ifndef ONE_WIRE_MASTER_H
#define ONE_WIRE_MASTER_H

#include "dio.h"
#include "one_wire_common.h"

#ifndef OWM_PIN
#  error OWM_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
#endif

// These are the result codes returned by many routines in this interface.
// The ones beginning with OWM_RESULT_ERROR_ probably shouldn't occur if all
// the hardware and software is correct, except perhaps under abnormal noise.
// The ones beginning with just OWM_ might be ok or not, depending on the
// nature of the hardware (e.g. dynamic or fixed slave set).  X macros
// are used here to ensure we stay in sync with the owm_result_as_string()
// function.
#define OWM_RESULT_CODES                                                      \
                                                                              \
  /* Note that this code must be first, so that it ends up with value 0.   */ \
  X (OWM_RESULT_SUCCESS)                                                      \
                                                                              \
  /* The master (that's us) sent a reset pulse, but didn't receive any     */ \
  /* slave response pulse.  This can happen when there's no slave present, */ \
  /* or perhaps if the slaves are all busy, if they are the sort that      */ \
  /* don't always honor reset pulses.  So far as I know official Maxim     */ \
  /* slaves always honor reset pulses, but if you use the one_wire_slave.h */ \
  /* interface to creat your own slaves, you may be tempted to ignore or   */ \
  /* delay your reaction to them at times.                                 */ \
  X (OWM_RESULT_DID_NOT_GET_PRESENCE_PULSE)                                   \
                                                                              \
  /* This occurs when either owm_next() or owm_next_alarmed() fails to     */ \
  /* find another slave, when owm_first_alarmed() fails to find a first    */ \
  /* alarmed slave, or when owm_verify() operates correctly but doesn't    */ \
  /* find the requested slave.  Note that when there are no slaves present */ \
  /* on the bus OWM_RESULT_DID_NOT_GET_PRESENCE_PULSE will result, not     */ \
  /* this.                                                                 */ \
  X (OWM_RESULT_NO_SUCH_SLAVE)                                                \
                                                                              \
  /* Got one values for both a bit and its compliment, in a situation      */ \
  /* where this shouldn't happen (i.e. not during the first bit of an      */ \
  /* owm_first_alarmed() call).  Note that when no slaves are present,     */ \
  /* many  routines in this module return                                  */ \
  /* OWM_RESULT_DID_NOT_GET_PRESENCE_PULSE, not this value.  This result   */ \
  /* could perhaps occur due to a line eror,  or if a slave is             */ \
  /* disconnected during a search.                                         */ \
  X (OWM_RESULT_ERROR_UNEXPECTEDLY_GOT_ONES_FOR_BIT_AND_ITS_COMPLIMENT)       \
                                                                              \
  /* The master (that's us) received a ROM ID with an inconsistent CRC     */ \
  /* value.                                                                */ \
  X (OWM_RESULT_ERROR_GOT_ROM_ID_WITH_INCORRECT_CRC_BYTE)                     \
                                                                              \
  /* A search operation saw what appeared to be a slave with ROM ID byte 0 */ \
  /* with a value of 0.  Well behaved slaves should never have a ROM ID    */ \
  /* with a byte 0 of 0, because this is how a ground-faulted data line    */ \
  /* (or misbhaving slave that's stuck holding the line low) ends up       */ \
  /* making its presence known for the first time.                         */ \
  X (OWM_RESULT_ERROR_GOT_ROM_ID_WITH_BYTE_0_OF_0_PROBABLE_GROUNDED_LINE)     \
                                                                              \
  /* Caller supplied an invalid ROM command argument (one that doesn't     */ \
  /* satisfy OWM_IS_TRANSACTION_INITIATING_ROM_COMMAND()).  This is a      */ \
  /* caller bug.                                                           */ \
  X (OWM_RESULT_ERROR_GOT_INVALID_TRANSACTION_INITIATION_COMMAND)             \
                                                                              \
  /* Caller supplied an invalid function command argument (one that does   */ \
  /* satisfy OWM_IS_ROM_COMMAND()).  This is a caller bug.                 */ \
  X (OWM_RESULT_ERROR_GOT_ROM_COMMAND_INSTEAD_OF_FUNCTION_COMMAND)            \
                                                                              \
  /* Some unknown problem occurred (probably a bug in this module).        */ \
  X (OWM_RESULT_ERROR_UNKNOWN_PROBLEM)

// Return type representing the result of an operation.
typedef enum {
#define X(result_code) result_code,
  OWM_RESULT_CODES
#undef X
} owm_result_t;

#ifdef OWM_BUILD_RESULT_DESCRIPTION_FUNCTION

#  define OWM_RESULT_DESCRIPTION_MAX_LENGTH 81

// Put the string form of result in buf.  The buf argument must point to
// a memory space large enough to hold OWM_RESULT_DESCRIPTION_MAX_LENGTH +
// 1 bytes.  Using this function will make your program quite a bit bigger.
// As a convenience, buf is returned.
char *
owm_result_as_string (owm_result_t result, char *buf);

#endif

// Intialize the one wire master interface.  All this does is set up the
// chosen DIO pin.  It starts out set as an input without the internal pull-up
// enabled.  It would probably be possible to use the internal pull-up on
// the AVR microcontroller for short-line communication at least, but the
// datasheet for the part I've used for testing (Maxim DS18B20) calls for a
// much stronger pull-up, so for simplicity the internal pull-up is disabled.
void
owm_init (void);

// Attempt to find all slaves present on the bus and store their ROM IDs in
// a newly allocated NULL-terminated list.  If at least one slave is found
// and no errors occur during the scan, then OWM_RESULT_SUCCESS is returned
// and *rom_ids is set to the first element of the new list, otherwise a
// non-zero result code is returned and no new memory is allocated.
owm_result_t
owm_scan_bus (uint8_t ***rom_ids_ptr);

// Free the memory allocated by a previous call to owm_scan_bus().
// If rom_ids is NULL nothing is freed (becuase there's nothing to free).
void
owm_free_rom_ids_list (uint8_t **rom_ids);

// Start the transaction sequence as described in the
// Maxim_DS18B20_datasheet.pdf page 10, "TRANSACTION SEQUENCE" section.
// This routine performs steps 1, 2, and the first half of 3 from this
// sequence (the function-specific communication required to complete the
// transaction is not performed).
//
// Arguments:
//
//   rom_cmd          May be OWC_READ_ROM_COMMAND (if there's only one slave on
//                    the bus), OWC_MATCH_ROM_COMMAND, or OWC_SKIP_ROM_COMMAND
//
//   rom_id           For OWC_READ_ROM_COMMAND, this contains the read ROM ID
//                    on return.  For OWC_MATCH_ROM_COMMAND, it must contain
//                    the ROM ID being addressed.  For OWC_SKIP_ROM_COMMAND it
//                    is unused (and may be NULL)
//
//   function_cmd     The function command to send.  This must not be a ROM
//                    command
//
// Return:
//
//   OWM_RESULT_SUCCESS on succes, or a non-zero result code otherwise.
//   indicating the problem otherwise.
//
// To actually complete the transaction, some slave- and transaction-specific
// back-and-forth using the lower level functions in this interface will
// likely be required.  Note that this routine cannot by itself ensure that
// the slave has received any OWC_MATCH_ROM_COMMAND or OWC_SKIP_ROM_COMMAND
// command correctly, since those don't elicit any response from the slave
// (though they do change its state).  The function_command likely does
// elicit a response, but this routine doesn't read it, so correct receipt
// of that command also cannot be verified by this routine.
owm_result_t
owm_start_transaction (uint8_t rom_cmd, uint8_t *rom_id, uint8_t function_cmd);

///////////////////////////////////////////////////////////////////////////////
//
// Reset and Individual Bit Functions
//
// These function perform reset or bit-at-a-time operations.  All the
// fundamental timing used in the 1-wire protocol is implemented in these
// functions, other functions in this interface are implemented in terms
// of these.
//

// Generate a 1-Wire reset, and then listen for a presence pulse.  Return
// TRUE if a presence pulse is detected, or FALSE otherwise.  NOTE: this
// is logically different than the comments for the OWTouchReset() function
// from Maxim_Application_Note_AN126.pdf indicate it uses.  NOTE: does not
// handle alarm presence from DS2404/DS1994.
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
// Byte Write/Read
//

// Write byte.  The LSB is written first.
void
owm_write_byte (uint8_t data);

// Read byte.  The LSB is read first.
uint8_t
owm_read_byte (void);

// Fancy simultaneous read/write.  Sort of.  I guess, I
// haven't used it. It's supposed to be more efficient.  See
// Maxim_Application_Note_AN126.pdf. WARNING: FIXXME: This comes straight
// from AN126, but I haven't tested it.
uint8_t
owm_touch_byte (uint8_t data);


///////////////////////////////////////////////////////////////////////////////
//
// Device Presense Confirmation/Discovery
//
// These functions allow the presence of particular slaves to be confirmed,
// or the bus searched for all slaves or all alarmed slaves.  You might be
// able to to use owm_scan_bus() instead of these lower-level functions.
//

// This function requires that exactly zero or one slaves be present on the
// bus.  If we discover a slave, its ID is written into id_buf (which must
// be a pointer to OWC_ID_SIZE_BYTES bytes of space) and OWM_RESULT_SUCCESS
// is returned, otherwise a non-zero result code is returned.  If there are
// two or more slaves present, the results of this function are undefined
// (later calls to this interface might behave strangely).
owm_result_t
owm_read_id (uint8_t *id_buf);

// Find the "first" slave on the 1-wire bus (in the sense of
// the discovery order of the 1-wire search algorithm described in
// Maxim_Application_Note_AN187.pdf).  If a slave is discovered, its ID is
// written into id_buf (which mucst be a pointer to OWC_ID_SIZE_BYTES bytes
// of space) and OWM_RESULT_SUCCESS is returned, otherwise a non-zero error
// code is returned.  Note that this resets any search which is already
// in progress.
owm_result_t
owm_first (uint8_t *id_buf);

// Require an immediately preceeding call to owm_first() or owm_next()
// to have occurred.  Find the "next" slave on the 1-wire bus (in the
// sense of the discovery order of the 1-wire search algorithm described
// in Maxim_Application_Note_AN187.pdf).  This continues a search begun by
// a previous call to owm_first().  If another slave is found, its ID is
// written into id_buf (which must be a pointer to OWC_ID_SIZE_BYTES bytes
// of space) and OWM_RESULT_SUCCESS is returned, otherwise a non-zero result
// code is returned.  If the end of the list of slaves has been reached, the
// non-zero result code will be OWM_RESULT_NO_SUCH_SLAVE.  Additional calls
// to this routine may wrap the search back to the start of the slave list,
// but this behavior is not guaranteed.
owm_result_t
owm_next (uint8_t *id_buf);

// Return OWM_RESULT_SUCCESS iff device with ID equal to the value in
// the OWC_ID_SIZE_BYTES bytes pointed to by id_buf is confirmed to be
// present on the bus, or a non-zero result code otherwise.  Note that
// unlike owm_read_id(), this function is safe to use when there are
// multiple devices on the bus.  When this function returns, the global
// search state is restored (so for example the next call to owm_next()
// should behave as if the call to this routine never occurred).
owm_result_t
owm_verify (uint8_t const *id_buf);

// Like owm_first(), but only finds slaves with an active alarm condition.
owm_result_t
owm_first_alarmed (uint8_t *id_buf);

// Like owm_next(), but only finds slaves with an active alarm condition.
owm_result_t
owm_next_alarmed (uint8_t *id_buf);

#endif // ONE_WIRE_MASTER_H
