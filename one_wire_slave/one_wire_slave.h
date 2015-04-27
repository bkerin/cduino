// One-wire slave interface (software interface -- requires only one IO pin)
//
// Test driver: one_wire_slave_test.c    Implementation: one_wire_slave.c
//
// This interface provides a framework for creating slave devices compatible
// with the Maxim one-wire slave protocol, or close (IMO its often more
// convenient to create almost-compatible devices which aren't quite so
// worried about timing or honoring every request).  Maxim isn't helping
// us create 1-wire slaves, its all based on Maxim Application note AN126
// and mimicking the behavior of the Maxim DS18B20.

#ifndef ONE_WIRE_SLAVE_H
#define ONE_WIRE_SLAVE_H

#include "dio.h"
#include "one_wire_common.h"
#include "timer1_stopwatch.h"

// To make slaves work at 8MHz, I had to explicitly lock some variables
// into registers r2, r3, r4, and r5.  For 16MHz things work without this,
// so I don't do it (because its assembly-ish, though admittedly it might
// give better resistance to compiler changes or client ISR use).  This trap
// is designed to ensure that clients realize that this is happening in the
// implementation, since they can't use these registers for other purposes.
#if F_CPU < 16000000
#  ifndef OWS_REGISTER_USE_ACKNOWLEDGED
#    error Because F_CPU < 16000000, OWS_REGISTER_LOCKING_ACKNOWLEDGED required
#  endif
#endif

// I haven't tried this module at frequencies lower than this.  Since there
// are many code sections where we depend on the processor going fast enough
// to get things done quicker that the (fairly speedy) one-wire protocol
// requires, there's a good chance it wouldn't work right at slower speeds.
#if F_CPU < 8000000
# error F_CPU too small
#endif

// Timer1 Stopwatch Frequency Required.  Since we're timing quite short
// pulses, we don't want to deal with a timer that can't even measure
// them accurately.
#define OWS_T1SFR 1000000
#if F_CPU / TIMER1_STOPWATCH_PRESCALER_DIVIDER < OWS_T1SFR
#  error F_CPU / TIMER1_STOPWATCH_PRESCALER_DIVIDER is too small
#endif

// There's probably no real reason we couldn't support F_CPU/timer1
// prescaler combinations that result in non-integer timer1 ticks/us, but
// the implementation doesn't currently do it and a little care would be
// required to convert it to do so at least without using more floating
// point math, and I haven't had the need.
#if CLOCK_CYCLES_PER_MICROSECOND () % TIMER1_STOPWATCH_PRESCALER_DIVIDER != 0
#  error timer1 ticks per microsecond is not an integer
#endif

// Clients probably don't need to use this directly.
#define OWS_TIMER_TICKS_PER_US \
  (CLOCK_CYCLES_PER_MICROSECOND () / TIMER1_STOPWATCH_PRESCALER_DIVIDER)

#ifndef OWS_PIN
#  error OWS_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
#endif

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
// the write_random_id_to_eeprom target of generic.mk would need to change
// as well or a different mechanism used to load the ID into EEPROM (see
// comments above the ows_init() declaration below.
#define OWS_PART_ID_EEPROM_ADDRESS 0

// Return type for functions in this interface which report errors.
typedef enum {
  OWS_ERROR_NONE = 0,
  OWS_ERROR_TIMEOUT,
  OWS_ERROR_GOT_RESET,
  OWS_ERROR_UNEXPECTED_PULSE_LENGTH,
  OWS_ERROR_GOT_INVALID_ROM_COMMAND,
  OWS_ERROR_RESET_DETECTED_AND_HANDLED,
  OWS_ERROR_ROM_ID_MISMATCH,
  OWS_ERROR_NOT_ALARMED,
} ows_error_t;

// Initialize the one-wire slave interface.  This sets up the chosen
// DIO pin, including pin change interrupts and global interrupt enable.
// If use_eeprom_id is true, the first six bytes of the AVR EEPROM are
// read and used together with the OWS_FAMILY_CODE and a CRC to form a
// slave ROM ID, which is loaded into RAM for speedy access.  In this
// case, interrupts will be blocked for a while while the EEPROM is read.
// See the write_random_id_to_eeprom target of generic.mk for a convenient
// way to load unique IDs onto devices (note that that target writes eight
// random bytes, of which only the first six are used by this interface).
// If use_eeprom_id is false, a default device ID with a non-family part
// numberof OWS_DEFAULT_PART_ID is used (note that this arrangement is only
// useful if you intend to use only one of your slaves on the bus).
void
ows_init (uint8_t use_eeprom_id);

// Value to pass to ows_set_timeout() when timeouts are not to be used.
#define OWS_NO_TIMEOUT 0

// This is the minimum timout setting that can be passed to ows_set_timeout()
// (besides OWS_NO_TIMEOUT, which disables timeouts).  Note that the
// ows_set_timeout() funtion implements timeouts of *approximately*
// this lengh.  At twice the length of a reset pulse (the longest pulse
// in the 1-wire protocol), this value is hopefully pretty conservative.
// A shorter value would probably work, but as you approach the defined
// lengths of pulses in the 1-wire protocol you're asking for trouble.
#define OWS_MIN_TIMEOUT_US (OWC_TICK_DELAY_H * 2)

// Analogous to OWS_MIN_TIMEOUT_US.
#define OWS_MAX_TIMEOUT_US ((uint16_t) (UINT16_MAX / OWS_TIMER_TICKS_PER_US))

// FIXME: change all uses of term one-wire to "1-wire" to match Maxim docs.

// If time_us isn't OWS_NO_TIMEOUT, then any calls in this interface that
// waits for 1-wire events will timeout and return OWS_ERROR_TIMEOUT after
// approximately this many microseconds without any activity on the line.
// Note that events not addressed to this slave will still prevent a
// timeout from occurring, so if your master continually talks to other
// slaves you'll never get a timeout.  If it isn't OWS_NO_TIMEOUT, the
// timeout_t1t argument must be in [OWS_MIN_TIMEOUT_US, OWS_MAX_TIMEOUT_US].
// This isn't intended to support short timeouts, it just gives a simple
// way to do other things from the main thread occasionally.  If you really
// need microsecond response from the slave for other purposes than 1-wire,
// give it its own slave processor :).
void
ows_set_timeout (uint16_t time_us);

// Wait for the initiation of a function command transaction intended
// for our ROM ID (and possibly others as well if a OWS_SKIP_ROM_COMMAND
// was sent), and return the function command itself in *command_ptr.
// See Maxim_DS18B20_datasheet.pdf page 10, "TRANSACTION SEQUENCE" section.
// In the meantime, automagically respond to resets and participate in any
// slave searches (i.e. respond to any incoming OWC_SEARCH_ROM_COMMAND or
// OWC_ALARM_SEARCH_COMMAND commands).  An error is generated if a pulse
// with an abnormal lenght (i.e. normal bit communication or reset lengh)
// is encountered.  An error is *not* generated when a potential transaction
// is aborted by the master by a new reset pulse.  This policy allows this
// routine to automagically handle all ROM SEARCH, READ ROM and the like,
// but also permits the somewhat weird case in which the master sends a
// command that is useless except for transaction initiation (a MATCH_ROM
// or SKIP_ROM), then doesn't actually follow it with a function command.
// An actual DS18B20 appears to tolerate this misbehavior as well, however.
// On the other hand we *do not* tolerate function commands that aren't
// preceeded by a ROM command.  The DS18B20 seems to tolerate them (when its
// the only device on the bus), but its completely contrary to the proscribed
// behavior for masters to do that, and makes the implementation of this
// routine harder.  Use the lower-level portion of this interface if you
// know you have only one slave and don't want to bother with addressing.
// Masters that fail to send an entire byte after issuing a reset pulse
// will cause this routine to hang until they get around to doing that
// (or issuing another reset pulse).
//
// FIXME: perhaps for consistency we should be ignoring short pulses in this routine as well, since that's what ows_wait_for_reset() currently does.
//
// If timeout is not zero, this routine will return OWS_ERROR_TIMEOUT iff:
//
//   * it's waiting for a reset pulse, and
//
//   * there is no recent line activity that hasn't been handled yet, and
//
//   * the line stays high for more than timeout_us microseconds +/- one
//     timer1 timer tick and a small amount of code overhead.
//
// Pending (negative) pulse ends in particular are always processed as
// pulses, rather than triggering a timeout.
//
// If not zero, the timeout value is required to be no greater than
// OWS_MAX_TIMEOUT_US.  This limit is required by the implementation in
// order to ensure that timer1 overflow doesn't cause a timeout to be missed.
//
// Note that a timeout won't happen if other slaves are talking continually,
// or if the master hangs this slave by stopping mid-byte or otherwise
// misbehaving.  This timeout implementation is intended as a simple
// way to integrate this routine into a loop in a slave that wants to do
// other things (including perhaps going to sleep if there's no activity,
// thereby gauranteeing that it won't be a truly well-behaved one-wire
// slave, but who cares).  If you need guaranteed latency regardless of
// FIXME: so how exactly will these timeouts interact with sleep?
// arbitrary network activity, perhaps you should add another processor to
// your design, or reconsider your use of one-wire for communication with
// the single processor, or run an RTOS instead.
// FIXME: update docs now timeout is a global setting
ows_error_t
ows_wait_for_function_transaction (uint8_t *command_ptr);

// This is the flattened version that's under development.
ows_error_t
ows_wait_for_function_transaction_2 (uint8_t *command_ptr, uint8_t jgur);

// Wait for a reset pulse, and respond with a presence pulse, then try
// to read a single byte from the master and return it.  An error is
// generated if any unexpected line behavior is encountered (abnormal
// pulse lengths, unexpected mid-byte resets, etc.  Any additional reset
// pulses that occur during this read attempt are also responded to with
// presence pulses (and OWS_ERROR_RESET_DETECTED_AND_HANDLED is returned).
// Any errors that occur while trying to read the byte effectively cause
// a new wait for a reset pulse to begin.  You should probably just use
// ows_wait_for_function_transaction() instead of this.
ows_error_t
ows_wait_for_command (uint8_t *command_ptr);

// Block until a reset pulse from the master is seen, then produce a
// corresponding presence pulse and return.  If timeout_us is not zero,
// this routine will return OWS_ERROR_TIMEOUT if the line stays high for
// at least timeout_us microseconds.  Pulses short of the length required
// for a reset pulse are silently ignored.
// FIXME: fix docs now that timeout time is a global
ows_error_t
ows_wait_for_reset (void);

// Write bit (which should be 0 or 1).
ows_error_t
ows_write_bit (uint8_t data_bit);

// Read bit, setting data_bit_ptr to 0 or 1.
ows_error_t
ows_read_bit (uint8_t *data_bit_ptr);


///////////////////////////////////////////////////////////////////////////////
//
// Byte Write/Read
//

// Write up to eight bits in a row.  The least significant bit is written
// first.  Any error that can occur in the underlying ows_write_bit()
// routine is immediately propagated and returned by this routine.
ows_error_t
ows_write_byte (uint8_t data_byte);

// Read up to eight bits in a row.  The least significant bit is read first.
// Any error that can occur in the underlying ows_read_bit() routine is
// immediately propagated and returned by this routine.
ows_error_t
ows_read_byte (uint8_t *data_byte_ptr);

#endif // ONE_WIRE_SLAVE_H


///////////////////////////////////////////////////////////////////////////////
//
// Device Presense Confirmation/Discovery Support
//
// These routines support slave participation in master-initiated device
// discovery.  Note that ows_wait_for_function_transaction() can handle
// this stuff automagically.
//

// This is the appropriate response to a OWC_READ_ROM_COMMAND.
ows_error_t
ows_write_id (void);

// Answer a (just received) OWC_SEARCH_ROM_COMMAND by engaging in the search
// process described in Maxim_Application_Note_AN187.pdf.
ows_error_t
ows_answer_search (void);

// If set to a non-zero value, this flag indicates an alarm condition.
// Clients of this interface can ascribe particular meanings to particular
// non-zero values if desired.  Slaves with an alarm condition should respond
// to any OWC_ALARM_SEARCH_COMMAND issued by thier master (and will do so
// automagically in ows_wait_for_function_command()).
extern uint8_t ows_alarm;

// Like ows_answer_search(), but only participates if ows_alarm is non-zero.
// This is the correct reaction to an OWC_ALARM_SEARCH_COMMAND.  If ows_alarm
// is zero, this macro evaluates to OWS_ERROR_NOT_ALARMED.
#define OWS_MAYBE_ANSWER_ALARM_SEARCH() \
  (ows_alarm ? ows_answer_search () : OWS_ERROR_NOT_ALARMED)

// Respond to a (just received) OWC_MATCH_ROM_COMMAND by reading up to
// OWC_ID_SIZE_BYTES bytes, one bit at a time, and matching the bits to our
// ROM ID.  Return OWS_ERROR_ROM_ID_MISMATCH as soon as we see a non-matching
// bit, or OWS_ERROR_NONE if all bits match (i.e. the master is talking
// to us).  Note that it isn't really an error if the ROM doesn't match,
// it just means the master doesn't want to talk to us.  This routine can
// also return other errors propagated from ows_read_bit().
ows_error_t
ows_read_and_match_id (void);
