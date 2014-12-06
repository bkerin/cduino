// This header contains command code byte values that aren't specific to
// any particular slave device, or are used in to implement extra features
// that this library provides (e.g. OWCC_SLEEP_COMMAND).  These values are
// (necessarily) identical in the one_wire_master and one_wire_slave modules,
// so they get their own header.

#ifndef ONE_WIRE_COMMON_COMMANDS_H
#define ONE_WIRE_COMMON_COMMANDS_H

// This was isn't a real command: the slave implementation assumes that no
// actual command has this value.
#define OWCC_NULL_COMMAND         0x00

// FIXME: either figure out which number might be good to use for this,
// or make it configurable or something, so we're not intruding into the
// slave command space?  Then again, that only matters when the entire bus
// is being addressed (SKIP_ROM).
// This command is used to implement an extra feature provided by
// the one_wire_master and one_wire_slave interfaces in this library:
// sleep-on-request.  See the sleep functions in those interfaces for details.
#define OWCC_SLEEP_COMMAND        0x77

// These are the standard ROM ID search and addressing commands common to
// all one-wire devices, see the DS18B20 datasheet "ROM COMMANDS" section.
#define OWCC_SEARCH_ROM_COMMAND   0xF0
#define OWCC_READ_ROM_COMMAND     0x33
#define OWCC_MATCH_ROM_COMMAND    0x55
#define OWCC_SKIP_ROM_COMMAND     0xCC
#define OWCC_ALARM_SEARCH_COMMAND 0xEC

#endif  // ONE_WIRE_COMMON_COMMANDS_H
