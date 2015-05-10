// This header describes fundamental characteristics and operations of
// the 1-wire protocol.  These values are (necessarily) identical in the
// one_wire_master and one_wire_slave modules, so they get their own header.

#include <util/delay.h>

#ifndef ONE_WIRE_COMMON_H
#define ONE_WIRE_COMMON_H

// Tick delays for various parts of the 1-wire protocol, as described in
// Table 2 on Maxim_Application_Note_AN126.pdf, page 3.
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


///////////////////////////////////////////////////////////////////////////////
//
// Line Drive, Sample, and Delay Routines
//
// These macros correspond to the uses of the inp and outp and
// tickDelay functions of Maxim_Application_Note_AN126.pdf.  We use
// macros to avoid function call time overhead, which can be significant:
// Maxim_Application_Note_AN148.pdf states that the most common programming
// error in 1-wire programmin involves late sampling, which given that some
// samples occur after proscribed waits of only 9 us requires some care,
// especially at slower processor frequencies.

// Release (tri-state) pin.  Note that this does not enable the internal
// pullup.  See the commends near owm_init() in one_wire_master.h.
#define OWC_RELEASE_LINE(pin) \
  DIO_INIT (pin, DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE)

// Drive pin low.
#define OWC_DRIVE_LINE_LOW(pin) \
  DIO_INIT (pin, DIO_OUTPUT, DIO_DONT_CARE, LOW)

#define OWC_SAMPLE_LINE(pin) DIO_READ (pin)

// We support only standard speed, not overdrive speed, so we make our tick
// 1 us.
#define OWC_TICK_TIME_IN_US 1.0

// WARNING: the argument to this macro must be a valid constant double
// or constant integer expression that the compiler knows is constant at
// compile time.  Pause for exactly ticks ticks.
#define OWC_TICK_DELAY(ticks) _delay_us (OWC_TICK_TIME_IN_US * ticks)

///////////////////////////////////////////////////////////////////////////////


// The ROM ID present in all slave devices consists of a one byte family code
// (shared by all parts of a given type), a six byte ID unique to each part,
// and an 8 bit CRC computer from the other seven bytes.
#define OWC_ID_SIZE_BYTES 8

// These are the standard ROM ID search and addressing commands common to
// all 1-wire devices, see the DS18B20 datasheet "ROM COMMANDS" section.
#define OWC_SEARCH_ROM_COMMAND   0xF0
#define OWC_READ_ROM_COMMAND     0x33
#define OWC_MATCH_ROM_COMMAND    0x55
#define OWC_SKIP_ROM_COMMAND     0xCC
#define OWC_ALARM_SEARCH_COMMAND 0xEC

// ROM commands perform 1-wire search and addressing operations and are
// effectively part of the 1-wire protocol, as opposed to other commands
// which particular slave types may define to do particular things.
#define OWC_IS_ROM_COMMAND(command) \
  ( command ==   OWC_SEARCH_ROM_COMMAND || \
    command ==     OWC_READ_ROM_COMMAND || \
    command ==    OWC_MATCH_ROM_COMMAND || \
    command ==     OWC_SKIP_ROM_COMMAND || \
    command == OWC_ALARM_SEARCH_COMMAND    )

// These ROM commands are valid ways to start a transaction (see
// DS18B20_datasheed.pdf "TRANSACTION SEQUENCE" section).
#define OWC_IS_TRANSACTION_INITIATING_ROM_COMMAND(command) \
  ( command ==  OWC_READ_ROM_COMMAND || \
    command == OWC_MATCH_ROM_COMMAND || \
    command ==  OWC_SKIP_ROM_COMMAND    )

#endif  // ONE_WIRE_COMMON_H
