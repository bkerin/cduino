// Interface for logging via one_wire_master.h
//
// Test driver: one_wire_master_logger_test.c    Implementation: one_wire_master_logger.c
//
// This module is intended to be used to get log message out of a
// 1-wire device.  It's probably most useful for devices that lacks most
// communication interfaces.  Only one data wire (and ground and power
// leads) is required.  At the other end of the line there should be an
// Arduino running the one_wire_slave_logger.h interface.  That Arduino
// then relays or stores messages somehow (via its USB-to-serial interface
// using term_io.h for example).

#ifndef ONE_WIRE_MASTER_LOGGER_H
#define ONE_WIRE_MASTER_LOGGER_H

// See the notes in the Makefile for this module for details about why we
// require variable from the OWM_* namespace to be set here.
#ifndef OWM_PIN
#  error OWM_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
#endif

// This magic value is used to indicate that the 1-wire line is private
// and the first slave found on the bus should be the target of communication.
#define DOWM_ONLY_SLAVE 0x00

// By default, DOWM_TARGET_SLAVE gets set to DOWM_ONLY_SLAVE.  If
// DOWM_TARGET_SLAVE is not DOWM_ONLY_SLAVE, it should be set to the 64 bit
// ID of the slave to be targeted (see the Makefile from the one_wire_master
// module for for examples of the Make options needed to do this).  This lets
// you add a logger to a 1-wire network with other devices on it.  Note that
// if your master already uses 1-wire, you probably have to use the existing
// network for logging, because the current one_wire_master.h interface
// doesn't support multiple 1-wire interface instances on different pins
// (without making copies and using an editor to add "_2" to a bunch of
// stuff :).
#ifndef DOWM_TARGET_SLAVE
#  define DOWM_TARGET_SLAVE DOWM_ONLY_SLAVE
#endif

// Initialize (or reinitialize) the 1-wire network to be used.
void
dowm_init (void);

// This is the maximum message length allowed (odwn_printf() allocates a
// buffer about this size, in addition to the RAM the format string uses...).
// This cannot be defined to be greater than or equal to UINT8_MAX.
#ifndef DOWM_MAX_MESSAGE_LENGTH
#  define DOWM_MAX_MESSAGE_LENGTH 242
#endif

// Print (send) a message, and wait for the slave to return an acknowledgement
// that the message has been successfully relayed.  On success the number of
// character printed is returned, on error an assertion violation is produced
// or a negative value is returned.  FIXME: maybe do one or the other?
int
dowm_printf (char const *format, ...)
  __attribute__ ((format (printf, 1, 2)));   // For printf format warnings

// FIXME: WORK POINT: in process of renaming still have to change all prefixes in this module and everything in debug_one_wire_slave

#endif // ONE_WIRE_MASTER_LOGGER_H
