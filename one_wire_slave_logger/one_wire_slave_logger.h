// Interface for debug logging via one_wire_slave.h
//
// Test driver: one_wire_slave_logger_test.c    Implementation: one_wire_slave_logger.c
//
// This module is intended to be used to receive log messages from a device
// that uses the one_wire_master_logger.h interface.  Only one data wire (and
// ground and power leads) is required at that end.  This module is intended
// to run on an Arduino or other hardware that can do something useful with
// the messages received (e.g. echo them to the serial-to-USB device).

#ifndef ONE_WIRE_SLAVE_LOGGER_H
#define ONE_WIRE_SLAVE_LOGGER_H

// See the notes in the Makefile for this module for details about why we
// require variable from the OWS_* namespace to be set here.
#ifndef OWS_PIN
#  error OWS_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
#endif

// This is the maximum message length that the master can send as a single
// message.  This should match or exceed the DOWM_MAX_MESSAGE_LENGTH used
// in one_wire_master_logger.h.  It cannot be defined to be greater than or
// equal to UINT8_MAX.
#ifndef OWSL_MAX_MESSAGE_LENGTH
#  define OWSL_MAX_MESSAGE_LENGTH 242
#endif

// The owsl_init() command can return these error codes in addition to
// those defined in ows_result_t.  These errors should only occur in the
// event of data corruption on the line.  The values are high enough not
// to intersect with the numeric values defined in ows_result_t :)
#define OWSL_RESULT_ERROR_INVALID_FUNCTION_CMD 142
#define OWSL_RESULT_ERROR_CRC_MISMATCH         143

// Initialize (or reinitialize) the module, and start waiting for messages.
// The *message_handler should handle the given NULL-byte-terminated message
// as appropriate (save it, relay it, whatever), then return 0 on success or
// negative value otherwise.  This function only returns on error, in which
// case it returns the (negative) value returned by *message_handler (if
// it failed), or one of the ows_result_t codes if there's a 1-wire error,
// or one of the OWSL_RESULT_ERROR_* values otherwise.  Unexpected 1-wire
// resets do not result in an error, but are retried.
int
owsl_init (int (*message_handler)(char const *message));

// This is an example of a useful message_handler that can be passed to
// owsl_init().  This handler just relays the given NULL-byte-terminated
// message via printf() as set up by term_io.h interface.  Clients must ensure
// that the term_io_init() function is called before this function runs.
int
owsl_relay_via_term_io (char const *message);

#endif // ONE_WIRE_SLAVE_LOGGER_H
