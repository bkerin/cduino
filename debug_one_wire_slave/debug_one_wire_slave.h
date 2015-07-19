// Interface for debug logging via one_wire_slave.h
//
// Test driver: debug_one_wire_slave_test.c    Implementation: debug_one_wire_slave.c
//
// This module is intended to be used to receive debugging output from of
// a device that lacks most communication interfaces.  At the other end of
// the line there should be an Arduino running the debug_one_wire_master.h
// interface.  Only one data wire (and ground and power leads is required at
// that end.  This module is intended to run on an Arduino or other hardware
// that can do something useful with the messages received (e.g. echo them
// to the serial-to-USB device).

#ifndef DEBUG_ONE_WIRE_SLAVE_H
#define DEBUG_ONE_WIRE_SLAVE_H

// See the notes in the Makefile for this module for details about why we
// require variable from the OWS_* namespace to be set here.
#ifndef OWS_PIN
#  error OWS_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
#endif

// This is the maximum message length that the master can send as a single
// message.  This should match or exceed the DOWM_MAX_MESSAGE_LENGTH used
// in debug_one_wire_mater.h.  It cannot be defined to be greater than or
// equal to UINT8_MAX.
#ifndef DOWS_MAX_MESSAGE_LENGTH
#  define DOWS_MAX_MESSAGE_LENGTH 242
#endif

// Initialize the module, and start waiting for messages.
// The *message_handler should handle the given NULL-byte-terminated
// message as appropriate (save it, relay it, whatever), then return
// 0 on success or non-zero otherwise.  This function only returns if
// message_handler returns non-zero, in which case it returns the value
// returned by message_handler.  Other 1-wire errors are silently ignored
// and and the operation that caused them retried.  FIXME: its somewhat
// goofy to have an _init function returning, either eat all errors or add
// another function that can return errors.
int
dows_init (int (*message_handler)(char const *message));

// This is an example of a useful message_handler that can be passed
// to dows_init.  This handler just relays the given NULL-byte-terminated
// message via printf() as set up by term_io.h interface.  Clients must ensure
// that the term_io_init() function is called before this function runs.
int
relay_via_term_io (char const *message);

#endif // DEBUG_ONE_WIRE_SLAVE_H
