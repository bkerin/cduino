// Interface for debug logging via one_wire_master.h
//
// Test driver: debug_one_wire_master_test.c    Implementation: debug_one_wire_master.c
//
// This module is intended to be used to get debugging output out of a
// device that lacks most communication interfaces.  Only one data wire
// (and ground and power leads) is required.  At the other end of the line
// there should be an Arduino running the debug_one_wire_slave.h interface.
// That Arduino then relays messages out via its USB-to-serial interface.

#ifndef DEBUG_ONE_WIRE_MASTER_H
#define DEBUG_ONE_WIRE_MASTER_H

// This magic value is used to indicate that the debugging line is private
// and the first slave found on the bus should be the target of communication.
#define DOWM_ONLY_SLAVE 0x00

// By default, DOWM_TARGET_SLAVE gets set to DOWM_ONLY_SLAVE.
// If DOWM_TARGET_SLAVE is not DOWM_ONLY_SLAVE, it should be set to
// the 64 bit ID of the slave to be targeted (see the Makefile from the
// one_wire_master module for for examples of the Make options needed to
// do this).  This lets you add a debug logger to a 1-wire network with
// other devices on it.  Note that if your master already uses 1-wire, you
// probably have to use the existing network for debug logging, because the
// current one_wire_master.h interface doesn't support multiple one-wire
// interface instances on different pins (without making copies and using
// an editor to add "_2" to a bunch of stuff :).
#ifndef DOWM_TARGET_SLAVE
#  define DOWM_TARGET_SLAVE DOWM_ONLY_SLAVE
#endif

// Initialize (or reinitialize) the 1-wire network to be used.
void
dowm_init (void);

// This is the maximum message length allowed (odwn_printf() allocated a
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

#endif // DEBUG_ONE_WIRE_MASTER_H
