// This header describes one-wire characteristics that aren't specific
// to any particular slave device, and also some constants used in to
// implement extra features that this library provides.  These values are
// (necessarily) identical in the one_wire_master and one_wire_slave modules,
// so they get their own header.

#ifndef ONE_WIRE_COMMON_H
#define ONE_WIRE_COMMON_H

// Tick delays for various parts of the one-wire protocol, as described in
// Table 2 in Maxim application note AN126 FIXME: link it
#define OWC_TICK_DELAY_A   6
#define OWC_TICK_DELAY_B  64
#define OWC_TICK_DELAY_C  60
#define OWC_TICK_DELAY_D  10
#define OWC_TICK_DELAY_E   9
#define OWC_TICK_DELAY_F  55
#define OWC_TICK_DELAY_G   0
#define OWC_TICK_DELAY_H 480
#define OWC_TICK_DELAY_I  70
#define OWC_TICK_DELAY_J 410

#define OWC_ID_SIZE_BYTES 1

// This was isn't a real command: the slave implementation assumes that no
// actual command has this value.
#define OWC_NULL_COMMAND 0x00

// FIXME: either figure out which number might be good to use for this,
// or make it configurable or something, so we're not intruding into the
// slave command space?  Then again, that only matters when the entire bus
// is being addressed (SKIP_ROM).
// This command is used to implement an extra feature provided by
// the one_wire_master and one_wire_slave interfaces in this library:
// sleep-on-request.  See the sleep functions in those interfaces for details.
#define OWC_SLEEP_COMMAND 0x77

// These are the standard ROM ID search and addressing commands common to
// all one-wire devices, see the DS18B20 datasheet "ROM COMMANDS" section.
#define OWC_SEARCH_ROM_COMMAND   0xF0
#define OWC_READ_ROM_COMMAND     0x33
#define OWC_MATCH_ROM_COMMAND    0x55
#define OWC_SKIP_ROM_COMMAND     0xCC
#define OWC_ALARM_SEARCH_COMMAND 0xEC

// ROM commands perform one-wire search and addressing operations and are
// effectively part of the one-wire protocol, as opposed to other commands
// which particular slave types may define to do particular things.
#define OWC_IS_ROM_COMMAND(command) \
  ( command ==   OWC_SEARCH_ROM_COMMAND || \
    command ==     OWC_READ_ROM_COMMAND || \
    command ==    OWC_MATCH_ROM_COMMAND || \
    command ==     OWC_SKIP_ROM_COMMAND || \
    command == OWC_ALARM_SEARCH_COMMAND    )

#endif  // ONE_WIRE_COMMON_H
