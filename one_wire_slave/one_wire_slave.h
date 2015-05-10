// 1-wire slave interface (software interface -- requires only one IO pin)
//
// Test driver: one_wire_slave_test.c    Implementation: one_wire_slave.c
//
// WARNING: READ THIS ENTIRE INTRODUCTION.
//
// This interface provides a framework for creating slave devices compatible
// with the Maxim 1-wire slave protocol, or close (IMO its often more
// convenient to create almost-compatible devices which aren't quite so
// worried about timing or honoring every request).  Maxim isn't helping
// us create 1-wire slaves, its all based on Maxim Application note AN126
// and mimicking the behavior of the Maxim DS18B20.
//
// If you're new to 1-wire you should first read the entire
// Maxim_DS18B20_datasheet.pdf.  Its hard to use 1-wire without at least a
// rough understanding of how the line signalling and transaction schemes
// work.
//
// This interface provides three types of things:
//
//   * The ows_init() and ows_set_timeout() routines, which set up the
//     hardware and specify the timeout period to use when waiting for
//     1-wire activity (if any).
//
//   * The fairly magical ows_wait_for_function_transaction(), which
//     automatically handles all reset and presence pulse sequences, and
//     ROM commands (steps 1 and 2 of the transaction sequence describe in
//     Maxim_DS18B20_datasheet.pdf, page 10).
//
//   * Bit- and byte-at-a-time functions that you can use to implement your own
//     slave-specific protocol (step 3 of the transaction sequence describe in
//     Maxim_DS18B20_datasheet.pdf, page 10).
//
// Note that ff you only have a network with exactly one slave and
// you control the master as well, you can probably dispense with
// ows_wait_for_function_transaction().
//
// The 1-wire protocol requires fast responses from the slave.  The time
// between a master-request-send edge and the point at which the slave must
// have the line pulled low to send a zero is only 15 us.  The turnaround time
// between a master-write and a subsequent master-read is similarly tight.
// This implementation works down to 10 MHz for me, but not for 8 MHz.
// If you have ISRs that can fire while this code runs, you might find they
// slow things down enough to cause occasional failures.
//
// The implementation has been written faily carefully for speed.  Your code
// that calls into it might not be.  Therefore, it is recommended that you
// arrange for the master to provide some recovery time between reads and
// writes when communicating with slaves implemented using this framework.
// Note that once ows_wait_for_function_transaction() returns (successfully),
// the details of the subsequent transaction protocol are slave-dependent:
// it's prefectly legitimate to require the master to pause for, say,
// 10 us between read/write-bit/byte calls.
//
// According to section 28.3 of the ATMega328P datasheet, 10 MHz operation
// is guaranteed down do 2.7 V Vcc.  12 MHz is safer timing-wise, and
// is guaranteed operation down to 3.06 V Vcc.  If you have a choice I'd
// recommend using 5V and running at 16 MHz.  You'll get better resistance
// to line noise that way as well.
//
// This module uses timer/counter1 to time events on the wire.  If you want
// to use this timer for other things as well it should work, but you'll
// need to reinitialized the timer yourself after using this interface,
// and call ows_init() again before using this interface again.  Of course
// in the meantime, you're slave won't be able to respond to 1-wire events.
//
// This module uses the pin change interrupt flag for the chosen OWS_PIN
// to detect changes on the wire.  Therefore, you can't (easily) use the
// pin change interrupt associated with that pin while using this module,
// since non-naked ISR routines will unexpectedly clear that hardware flag.
// This module doesn't actually run any ISR routines, and therefore never
// enables interrupts itself (so sei() is never called).
//
// FIXME: revisit this advice once we've actually tested wake-up: it might
// for example be required to actually have ISRs enabled, in which case an
// interface function (or maybe two, with one being for clean-up after the
// wake-up) will be required
// If you're on batteries, you'll probably want to put the slave to
// sleep when nothing is happening.  If brown-out detection is enabled,
// the wake-up time will probably be fast enough that the slave will have
// time to respond correctly to a reset pulse from the master.  If not,
// it probably won't, since wake-up times in that situation tend to be 4 ms
// or longer.  The BOD costs some power to operate.  If you want to avoid
// that, I suggest simple declaring that your slave in slightly disobedient
//

#ifndef ONE_WIRE_SLAVE_H
#define ONE_WIRE_SLAVE_H

#include <stdint.h>

#include "dio.h"
#include "one_wire_common.h"
#include "timer1_stopwatch.h"

#ifndef OWS_PIN
#  error OWS_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
#endif

// To make slaves work at 10 MHz, I had to explicitly lock some variables
// into registers r2, r3, r4, and r5.  The tests work without doing this
// at 12 MHz and 16 MHz, but because I don't trust the compiler not to do
// things differently depending on client code shape/size, I've decided
// it's safest to just do it for all frequencies by default.  Because
// this can fail horribly if other code in the same program is trying
// to use these registers, users of this module are required to define
// OWS_REGISTER_USE_ACKNOWLEDGED to ensure that they know what's going on
// (see the Makefile for this module for a line to uncomment to do this).
#ifndef OWS_REGISTER_USE_ACKNOWLEDGED
#  error OWS_REGISTER_LOCKING_ACKNOWLEDGED must be defined by the user
#endif

// This is the lowest frequency at which I've gotten this module to work.
// It didn't work for me at 8 MHz, and the symptoms looked very much like
// the code wasn't getting the line pulled low fast enough to send a zero
// after the master requested a slave write.  See the comments at the top
// of this file.
#if F_CPU < 10000000
# error F_CPU too small
#endif

// Timer1 Stopwatch Frequency Required.  Since we're timing quite short
// pulses, we don't want to deal with a timer that can't even measure
// them accurately.
#define OWS_T1SFR 1000000
#if F_CPU / TIMER1_STOPWATCH_PRESCALER_DIVIDER < OWS_T1SFR
#  error F_CPU / TIMER1_STOPWATCH_PRESCALER_DIVIDER is too small
#endif

// Clients probably don't need to use this directly.
#define OWS_TIMER_TICKS_PER_US \
  (CLOCK_CYCLES_PER_MICROSECOND () / TIMER1_STOPWATCH_PRESCALER_DIVIDER)

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
// the write_random_id_to_eeprom and write_non_random_id_to_eeprom targets
// in generic.mk would need to change as well or a different mechanism
// used to load the ID into EEPROM (see comments above the ows_init()
// declaration below).
#define OWS_PART_ID_EEPROM_ADDRESS 0

// Return type for functions in this interface which report errors.
// FIXME: should use x-macro here as in one_wire_master, and these
// should be results, only some of which are errors (probably only
// GOT_INVALID_ROM_COMMAND, or maybe GOT_UNEXPECTED_RESET as well).
typedef enum {
  OWS_ERROR_NONE = 0,
  OWS_ERROR_TIMEOUT,
  OWS_ERROR_GOT_UNEXPECTED_RESET,
  OWS_ERROR_GOT_INVALID_ROM_COMMAND,
  OWS_ERROR_ROM_ID_MISMATCH,
} ows_error_t;

// Initialize the 1-wire slave interface.  This sets up the chosen DIO
// pin, including pin change interrupts and global interrupt enable.
// If use_eeprom_id is true, the first six bytes of the AVR EEPROM are
// read and used together with the OWS_FAMILY_CODE and a CRC to form a
// slave ROM ID, which is loaded into RAM for speedy access.  In this
// case, interrupts will be blocked for a while while the EEPROM is read.
// See the write_random_id_to_eeprom target of generic.mk for a convenient
// way to load unique IDs onto devices (note that that target writes eight
// random bytes, of which only the first six are used by this interface).
// If use_eeprom_id is false, a default device ID with a non-family part
// numberof OWS_DEFAULT_PART_ID is used (note that this arrangement is
// only useful if you intend to use only one of your slaves on the bus).
// The timeout value is set to OWS_TIMEOUT_NONE (but may be changed using
// osw_set_timeout() before other routines in this interface are called).
void
ows_init (uint8_t use_eeprom_id);

// Value to pass to ows_set_timeout() when timeouts are not to be used.
#define OWS_TIMEOUT_NONE 0

// This is the minimum timout setting that can be passed to ows_set_timeout()
// (besides OWS_TIMEOUT_NONE, which disables timeouts).  Note that the
// ows_set_timeout() funtion implements timeouts of *approximately* this
// length.  At twice the length of a reset pulse (the longest pulse in the
// 1-wire protocol), this value is pretty conservative.  A shorter value
// would probably work, but as you approach the defined lengths of pulses
// in the 1-wire protocol you're asking for trouble.
#define OWS_MIN_TIMEOUT_US (OWC_TICK_DELAY_H * 2)

// Analogous to OWS_MIN_TIMEOUT_US.
#define OWS_MAX_TIMEOUT_US ((uint16_t) (UINT16_MAX / OWS_TIMER_TICKS_PER_US))

// If time_us isn't OWS_TIMEOUT_NONE, then any call into this interface
// that waits for a 1-wire event will timeout and return OWS_ERROR_TIMEOUT
// after approximately this many microseconds without any change in the line
// state.  Note that events not addressed to this slave will still prevent
// a timeout from occurring, so if your master continually talks to other
// slaves you'll never get a timeout.  If it isn't OWS_TIMEOUT_NONE, the
// timeout_t1t argument must be in [OWS_MIN_TIMEOUT_US, OWS_MAX_TIMEOUT_US].
// This isn't intended to support short timeouts, it just gives a simple
// way to do other things from the main thread occasionally.  If you really
// need microsecond response from the slave for other purposes than 1-wire,
// give your slave its own slave processor :).
void
ows_set_timeout (uint16_t time_us);

// Wait for the initiation of a function command transaction intended
// for our ROM ID (and possibly others as well if a OWS_SKIP_ROM_COMMAND
// was sent), and return the function command itself in *command_ptr.
// See Maxim_DS18B20_datasheet.pdf page 10, "TRANSACTION SEQUENCE" section.
// In the meantime, automagically respond to resets and participate in any
// slave searches (i.e. respond to any incoming OWC_SEARCH_ROM_COMMAND or
// OWC_ALARM_SEARCH_COMMAND commands).
//
// The jgur (Just Got Unexpected Reset) argument should be FALSE unless
// you just got an unexpected reset (see below).
//
// On success OWS_ERROR_NONE is returned.  Otherwise, one of the following
// errors is returned:
//
//   * OWS_ERROR_TIMEOUT if a timeout occurred (see ows_set_timeout() above).
//
//   * OWS_ERROR_GOT_UNEXPECTED_RESET if we get a reset pulse from the
//     master when we where expecting something else.  In this case, it
//     probably makes sense to immediately call this function again with
//     the jgur argument TRUE.
//
//   * OWS_ERROR_GOT_INVALID_ROM_COMMAND if we get a command that doesn't
//     satisfy OWC_IS_ROM_COMMAND(command) from one_wire_common.h.  This could
//     result from a misbehaving master, or from data corruption on the line.
//
ows_error_t
ows_wait_for_function_transaction (uint8_t *command_ptr, uint8_t jgur);

// Write a single bit with value bit_value (when the master requests it).
// Use ows_write_byte() instead for byte-at-a-time messages.  Returns
// OWS_ERROR_TIMEOUT or OWS_ERROR_GOT_UNEXPECTED_RESET on error, or
// OWS_ERROR_NONE otherwise.  READ THE ENTIRE TEXT AT THE TOP OF THIS FILE.
ows_error_t
ows_write_bit (uint8_t bit_value);

// Read a single bit into *bit_value_ptr (when the master sends it).
// Use ows_read_byte() instead for byte-at-a-time messages.  Returns
// OWS_ERROR_TIMEOUT or OWS_ERROR_GOT_UNEXPECTED_RESET on error, or
// OWS_ERROR_NONE otherwise.  READ THE ENTIRE TEXT AT THE TOP OF THIS FILE.
ows_error_t
ows_read_bit (uint8_t *bit_value_ptr);

// Write a byte with value byte_value (when the master requests it).
// Returns OWS_ERROR_TIMEOUT or OWS_ERROR_GOT_UNEXPECTED_RESET on error,
// or OWS_ERROR_NONE otherwise.  READ THE ENTIRE TEXT AT THE TOP OF THIS FILE.
ows_error_t
ows_write_byte (uint8_t byte_value);

// Reads a byte into *byte_value_ptr (when the master requests it).
// Returns OWS_ERROR_TIMEOUT or OWS_ERROR_GOT_UNEXPECTED_RESET on error,
// or OWS_ERROR_NONE otherwise.  READ THE ENTIRE TEXT AT THE TOP OF THIS FILE.
ows_error_t
ows_read_byte (uint8_t *byte_value_ptr);

// If set to a non-zero value, this flag indicates an alarm condition.
// Clients of this interface can ascribe particular meanings to particular
// non-zero values if desired.  Slaves with an alarm condition will respond
// to any OWC_ALARM_SEARCH_COMMAND issued by thier master (this process is
// started automagically in ows_wait_for_function_command()).
extern uint8_t ows_alarm;

#endif // ONE_WIRE_SLAVE_H
