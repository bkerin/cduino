// Implementation of the interface described in one_wire_master.h.

#include <avr/eeprom.h>
#include <stdlib.h>
#include <string.h>
#include <util/atomic.h>
#include <util/crc16.h>
#include <util/delay.h>

#include "dio.h"
#include "one_wire_common.h"
#include "one_wire_slave.h"
#include "timer1_stopwatch.h"
// FIXME: next two lines for debug only:
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

///////////////////////////////////////////////////////////////////////////////
//
// Line Drive, Sample, and Delay Routines
//
// These macros correspond to the uses of the inp and outp and tickDelay
// functions of Maxim application note AN126.  We use macros to avoid
// function call time overhead, which can be significant: Maxim application
// note AN148 states that the most common programming error in 1-wire
// programmin involves late sampling, which given that some samples occur
// after proscribed waits of only 9 us requires some care, especially at
// slower processor frequencies.

// Release (tri-state) the one wire pin.  Note that this does
// not enable the internal pullup.  See the comments near omw_init()
// in one_wire_master.h.
#define RELEASE_LINE()    \
  DIO_INIT (              \
      OWS_PIN,            \
      DIO_INPUT,          \
      DIO_DISABLE_PULLUP, \
      DIO_DONT_CARE )

// Drive the line of the one wire master pin low.
#define DRIVE_LINE_LOW() \
  DIO_INIT (             \
      OWS_PIN,           \
      DIO_OUTPUT,        \
      DIO_DONT_CARE,     \
      LOW )

#define SAMPLE_LINE() DIO_READ (OWS_PIN)

// We support only standard speed, not overdrive speed, so we make our tick
// 1 us.
#define TICK_TIME_IN_US 1.0

// Timer1 ticks per microsecond
#define T1TPUS 2

static uint8_t rom_id[OWM_ID_BYTE_COUNT];

static void
set_rom_id (uint8_t use_eeprom_id)
{
  // Set up the ROM ID in rom_id, using EEPROM data if use_eeprom_id is
  // TRUE, or the default part ID otherwise.  In either case the matching
  // CRC is added.

  uint8_t const ncb = OWM_ID_BYTE_COUNT - 1;   // Non-CRC Bytes in the ROM ID
  uint8_t const pib = ncb - 1;   // Part ID Bytes in the ROM ID

  if ( use_eeprom_id ) {
    rom_id[0] = OWS_FAMILY_CODE;
    // This probably only really needs to be atomic if the eeprom_* routines
    // are getting used from an ISR somewhere.  But who knows, the user
    // might want to do that.
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
    {
      uint8_t *pidp = rom_id + 1;   // Part ID Ptr (+1 for family code space)
      eeprom_read_block (pidp, OWS_PART_ID_EEPROM_ADDRESS , pib);
    }
  }
  else {
    uint64_t id_as_uint64 = __builtin_bswap64 (UINT64_C (OWS_DEFAULT_PART_ID));
    id_as_uint64 >>= BITS_PER_BYTE;
    ((uint8_t *) (&id_as_uint64))[0] = OWS_FAMILY_CODE;
    for ( uint8_t ii = 0 ; ii < ncb ; ii++ ) {
      rom_id[ii] = ((uint8_t *) &id_as_uint64)[ii];
    }
  }

  uint8_t crc = 0;   // CRC (to be computed)
  for ( uint8_t ii = 0 ; ii < ncb ; ii++ ) {
    crc = _crc_ibutton_update (crc, rom_id[ii]);
  }
  rom_id[ncb] = crc;
}

void
ows_init (uint8_t use_eeprom_id)
{
  set_rom_id (use_eeprom_id);

  timer1_stopwatch_init ();

  RELEASE_LINE ();   // Serves to initialize pin along the way
  DIO_ENABLE_PIN_CHANGE_INTERRUPT (OWS_PIN);

  sei ();
}

// The following ST_* (Slave Timing) macros contain timing values that
// actual Maxim DS18B20 devices have been found to use, or values that
// we have derived logically from our understanding of the protocol; see
// one_wire_master.c.probe from the one_wire_master module for the source
// of the experimental values.

// FIXME: clone back to old experimental vals from prenuked now that we
// require a 1 MHz or better timer1

// The DS18B20 datasheet and AN126 both say masters are supposed to send
// 480 us pulse minimum.
#define ST_RESET_PULSE_LENGTH_REQUIRED 240

// The DS18B20 datasheet says 15 to 60 us.
#define ST_DELAY_BEFORE_PRESENCE_PULSE 28

// The DS18B20 datasheet says 60 to 240 us.  FIXME: do other 1-wire datasheets
// give the same timing numbers?  the DS18B20 is somewhat old, maybe they've
// sorted out new better numbers since then.
#define ST_PRESENCE_PULSE_LENGTH 116

// The DS18B20 datasheet says at least 1 us required from master, but the
// actual DS18B20 devices seem even the shortest blip as signaling the start
// of a slot.  So this one-cycle time is sort of a joke, and in fact its best
// to not wait at all so we don't have to worry about the actual timer delay.
#define ST_REQUIRED_READ_SLOT_START_LENGTH (1.0 / 16)

// The total lenght of a slave read slot isn't supposed to be any longer
// than this.
#define ST_SLAVE_READ_SLOT_DURATION 60

// This is the time the DS18B20 diagram indicates that it typically waits
// from the time the line is pulled low by the master to when it (the slave)
// samples.
#define ST_SLAVE_READ_SLOT_SAMPLE_TIME 32

// This is the maximum pulse lengh we will tolerate when looking for the
// pulse the master is supposed to send to start a slave write slot.  We go
// with OWC_TICK_DELAY_E / 2 here because its half whay between what the
// master is supposed to send and the point at which the master is supposed
// to sample the line, and also because the grace time is small enough that
// it won't cause the ST_SLAVE_WRITE_ZERO_LINE_HOLD_TIME-length pulse we
// might send in response to crowd the end of the time slot at all.
#define ST_SLAVE_WRITE_SLOT_START_PULSE_MAX_LENGTH \
  (OWC_TICK_DELAY_A + OWC_TICK_DELAY_E / 2)

// This is the time to hold the line low when sending a 0 to the master.
// See Figure 1 of Maxim Application Note AN126.  We go with OWC_TICK_DELAY_F
// / 2 here because it's, well, half way between when we must have the line
// held low and when we must release it.  We could probably measure what
// actual slaves do if necessary...
#define ST_SLAVE_WRITE_ZERO_LINE_HOLD_TIME \
  (OWC_TICK_DELAY_E + OWC_TICK_DELAY_F / 2)

// This is the longest that this slave implementation ever holds the line
// low itself.  This is relevant because we want to let our interrupt
// handler do all the resetting of the hardware timer that we use to
// detect reset pulses without having to flip the reset detector on and
// off or anything.  This policy avoids timer access atomicity issues
// and generally keeps things simple.  The consequence is that we end up
// requiring reset pulses up to this much longer than the experimentally
// measured value of ST_RESET_PULSE_LENGTH_REQUIRED.  We have to do that
// because the interrupt handler counts interrupts caused when the slave
// itself drives the line low, so the ensuing line-low time ends up counting
// towardes reset pulse time.  Because ST_RESET_PULSE_LENGTH_REQUIRED +
// ST_LONGEST_SLAVE_LOW_PULSE_LENGTH is still considerably less than the 480
// us pulse that well-behaved masters send, this shouldn't be a problem.
// The value of this macro is ST_PRESENCE_PULSE_LENGTH because that's the
// longest the slave should ever have to hold the line low.
#define ST_LONGEST_SLAVE_LOW_PULSE_LENGTH ST_PRESENCE_PULSE_LENGTH

// Convenience macros
#define T1RESET() TIMER1_STOPWATCH_RESET ()
//#define T1US()    ((double) TIMER1_STOPWATCH_MICROSECONDS ())
// Hard-coded for our prescaler/F_CPU to not be a double.  This version
// must be used only in ISR, otherwise use AT1US().
#define T1US() (TIMER1_STOPWATCH_TICKS() * 4)
#define T1OF() TIMER1_STOPWATCH_OVERFLOWED ()


// Atomic version of T1US().  We have to use an atomic block to access
// TCNT1 outside the ISR, so we have this macro that does that and sets
// the double variable name argument given it to the elapsed us.
// FIXME: hardcoded for our prescaler/F_CPU case to NOT be a double
#define AT1US(outvar)                     \
  do {                                    \
    uint16_t XxX_tt;                      \
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE)    \
    {                                     \
      XxX_tt = TIMER1_STOPWATCH_TICKS (); \
    }                                     \
    outvar = XxX_tt * 4;                  \
  } while (0)

// Readability macros
#define LINE_IS_HIGH() (  SAMPLE_LINE ())
#define LINE_IS_LOW()  (! SAMPLE_LINE ())

// When the pin change ISR observes a positive edge, it sets new_pulse and
// records the pulse_length in timer1 ticks of the new pulse.
volatile uint8_t new_pulse = FALSE;
volatile uint16_t pulse_length;

// This ISR keeps track of the length of low pulses.  If we see a long enough
// one we consider that we've seen a reset and set a client-visible flag.
ISR (DIO_PIN_CHANGE_INTERRUPT_VECTOR (OWS_PIN))
{
  if ( LINE_IS_HIGH () ) {
    new_pulse = TRUE;
    pulse_length = TIMER1_STOPWATCH_TICKS ();
  }
  else {
    new_pulse = FALSE;
    T1RESET ();
  }
}

static uint16_t
wait_for_pulse_end (void)
{
  // Wait for the positive edge that occurs at the end of a negative pulse,
  // then return the negative pulse duration.  In fact this waits for
  // a flag variable to be set from a pin change ISR, which seems to be
  // considerably more rebust than pure delta detecton would probably be:
  // see pcint_test.c.disabled in the one_wire_slave module directory for
  // some tests I did to verify how this interrupt works.

  uint8_t gnp = FALSE;   // Got New Pulse
  uint16_t npl;          // New Pulse Length
  do {
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
    {
      gnp = new_pulse;
      npl = pulse_length;
      new_pulse = FALSE;
    }
  } while ( ! gnp );

  return npl;
}

// Drive the line low for the time required to indicate presence to the
// master, then call wait_for_pulse_end() to swallow the pulse that this
// causes the ISR to detect.
#define OWS_PRESENCE_PULSE()              \
  do {                                    \
    DRIVE_LINE_LOW ();                    \
    _delay_us (ST_PRESENCE_PULSE_LENGTH); \
    RELEASE_LINE ();                      \
    wait_for_pulse_end ();                \
  } while ( 0 )

void
ows_wait_for_reset (void)
{
  while ( wait_for_pulse_end () < ST_RESET_PULSE_LENGTH_REQUIRED * T1TPUS ) {
    ;
  }
  _delay_us ((double) ST_DELAY_BEFORE_PRESENCE_PULSE);

  OWS_PRESENCE_PULSE ();
}

uint8_t
ows_wait_for_command (void)
{
  uint8_t result;

  while ( ows_read_byte (&result) != OWS_ERROR_NONE ) {
    ;
  }

  return result;
}

// Call call, Propagating Errors.  The call argument must be a call to a
// function returning ows_err_t.
#define CPE(call)                  \
  do {                             \
    ows_error_t err = call;        \
    if ( err != OWS_ERROR_NONE ) { \
      return err;                  \
    }                              \
  } while ( 0 )

// FIXME: I may need to be relocated
//
ows_error_t
ows_read_and_match_rom_id (void)
{
  for ( uint8_t ii = 0 ; ii < OWM_ID_BYTE_COUNT ; ii++ ) {
    for ( uint8_t jj = 0 ; jj < BITS_PER_BYTE ; ii++ ) {
      uint8_t bit_value;
      CPE (ows_read_bit (&bit_value));
      if ( bit_value != ((rom_id[ii]) >> jj) ) {
        return OWS_ERROR_ROM_ID_MISMATCH;
      }
    }
  }

  return OWS_ERROR_NONE;
}

uint8_t
ows_wait_for_function_command (void)
{
  uint8_t tfu = FALSE;  // Transaction For Us (our ROM ID, or all slaves)
  uint8_t command = OWC_NULL_COMMAND;
  ows_error_t err;   // For return codes

  do {

    while ( command == OWC_NULL_COMMAND ) {
      // This command *should* be a ROM command of some sort, even if its
      // only a OWS_SKIP_ROM_COMMAND, since at this point we don't have a
      // target slave for a real comand, however...
      command = ows_wait_for_command ();
      // it might not be, and some Maxim slave devices might still work in
      // some situation.  For example, the DS18B20 seems to honor "Convert
      // T" (0x44) commands that come after a Search ROM command, at least
      // when there's only one slave on the bus.  Yet the DS18B20 datasheet
      // clearly states in the "TRANSACTION SEQUENCE" section that a new
      // transaction must be started after Search ROM (and for good reason,
      // since this command doesn't address any particular slave).  So if
      // we get a non-ROM command here, we have two choices:
      //
      //   1.  Do what the DS18B20 apparently does, and treat it as valid
      //       (and return it from this function).
      //
      //   2.  Treat it as an error, and silently eat it (FIXME: or propagate
      //       the error, if we decide we want to propagate errors from
      //       this function).
      //
      // I'm going with option 2 for the moment, but this is a sufficiently
      // screwy issue that perhaps a client-visible option controlling or
      // at least indicating this behavior is warranted.
      if ( ! OWC_IS_ROM_COMMAND (command) ) {
        command = OWC_NULL_COMMAND;
      }
    }

    switch ( command ) {

      case OWC_SEARCH_ROM_COMMAND:
        // FIXME: we should perhaps just propagate errors everywhere, all
        // the way to the top?  At the moment we eat them here
        ows_answer_search ();
        command = OWC_NULL_COMMAND;
        break;

      case OWC_ALARM_SEARCH_COMMAND:
        OWS_MAYBE_ANSWER_ALARM_SEARCH ();
        command = OWC_NULL_COMMAND;
        break;

      case OWC_READ_ROM_COMMAND:
        // FIXME: again, swallowing errors is questionable policy
        err = ows_write_rom_id ();
        if ( err == OWS_ERROR_NONE ) {
          tfu = TRUE;
        }
        else {
          command = OWC_NULL_COMMAND;
        }
        break;

      case OWC_MATCH_ROM_COMMAND:
        err = ows_read_and_match_rom_id ();
        if ( err == OWS_ERROR_NONE ) {
          tfu = TRUE;
        }
        else {
          command = OWC_NULL_COMMAND;
        }
        // FIXME: other errors during matching are swallowed...
        break;

      case OWC_SKIP_ROM_COMMAND:
        tfu = TRUE;
        break;

    }

    if ( tfu ) {
      command = ows_wait_for_command ();
      // The master might be a weirdo and decide to send another ROM command
      // instead of a function command, in which case we need to keep going.
      if ( ! OWC_IS_ROM_COMMAND (command) ) {
        return command;
      }
      else {
        tfu = FALSE;
      }
    }

  } while ( TRUE );

}

ows_error_t
ows_read_bit (uint8_t *data_bit_ptr)
{
  uint16_t pl = wait_for_pulse_end ();

  if ( pl < ST_SLAVE_READ_SLOT_SAMPLE_TIME * T1TPUS ) {
    *data_bit_ptr = 1;
  }
  // this is required to be less than tick delay C + D, D is the margin
  else if ( pl < (60+10) * T1TPUS ) {
    *data_bit_ptr = 0;
  }
  else if ( pl > ST_RESET_PULSE_LENGTH_REQUIRED * T1TPUS ) {
    _delay_us (ST_DELAY_BEFORE_PRESENCE_PULSE);
    OWS_PRESENCE_PULSE ();
    return OWS_ERROR_RESET_DETECTED_AND_HANDLED;
  }
  else {
    return OWS_ERROR_UNEXPECTED_PULSE_LENGTH;
  }

  return OWS_ERROR_NONE;
}

// Drive the line low for the time required to indicate a value of zero
// when writing a bit, then call wait_for_pulse_end() to swallow the pulse
// that this causes the ISR to detect.
#define OWS_ZERO_PULSE()                            \
  do {                                              \
    DRIVE_LINE_LOW ();                              \
    _delay_us (ST_SLAVE_WRITE_ZERO_LINE_HOLD_TIME); \
    RELEASE_LINE ();                                \
    wait_for_pulse_end ();                          \
  } while ( 0 )

ows_error_t
ows_write_bit (uint8_t data_bit)
{
  // Figure 1 of Maxim Application Note AN 126 shows that the master should
  // start a read slot by pulling the line low for 6 us, then sample the
  // line after an addional 9 us.  The slave transmits a one by leaving the
  // bus high at that point, and a zero by pulling it low.  In either case,
  // the bus is supposed to be released again by the end of the time slot F
  // (55) us later.

  uint16_t pl = wait_for_pulse_end ();

  if ( pl < ST_SLAVE_WRITE_SLOT_START_PULSE_MAX_LENGTH * T1TPUS ) {
    if ( ! data_bit ) {
      OWS_ZERO_PULSE ();
    }
  }
  else if ( pl > ST_RESET_PULSE_LENGTH_REQUIRED * T1TPUS ) {
    _delay_us (ST_DELAY_BEFORE_PRESENCE_PULSE);
    OWS_PRESENCE_PULSE ();
    return OWS_ERROR_RESET_DETECTED_AND_HANDLED;
  }
  else {
    return OWS_ERROR_UNEXPECTED_PULSE_LENGTH;
  }

  return OWS_ERROR_NONE;
}

ows_error_t
ows_read_byte (uint8_t *data_byte_ptr)
{
  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ ) {

    (*data_byte_ptr) >>= 1;  // Shift the result to get ready for the next bit

    // Read the next bit
    uint8_t bit_value;
    CPE (ows_read_bit (&bit_value));

    // If result is one, then set (current) MS bit of the result byte
    if ( bit_value ) {
      (*data_byte_ptr) |= B10000000;
    }
  }

  return OWS_ERROR_NONE;
}

ows_error_t
ows_write_byte (uint8_t data_byte)
{
  // Loop to write each bit in the byte, LS-bit first
  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ )
  {
    CPE (ows_write_bit (data_byte & B00000001));
    data_byte >>= 1;   // Shift to get to next bit
  }

  return OWS_ERROR_NONE;
}

ows_error_t
ows_write_rom_id (void)
{
  for ( uint8_t ii = 0 ; ii < OWM_ID_BYTE_COUNT ; ii++ ) {
    CPE (ows_write_byte (rom_id[ii]));
  }

  return OWS_ERROR_NONE;
}

// Evaluate to the value of Bit Number bn (0-indexed) of rom_id.
#define ID_BIT(bn) \
  ((rom_id[bn / OWM_ID_BYTE_COUNT]) >> (bn % BITS_PER_BYTE) & B00000001)

ows_error_t
ows_answer_search (void)
{
  for ( uint8_t ii = 0 ; ii < OWM_ID_BYTE_COUNT * BITS_PER_BYTE ; ii++ ) {
    uint8_t bv = ID_BIT (ii);
    CPE (ows_write_bit (bv));
    CPE (ows_write_bit (! bv));
    uint8_t mbv;
    CPE (ows_read_bit (&mbv));
    if ( bv != mbv ) {
      return OWS_ERROR_NONE;
    }
  }

  return OWS_ERROR_NONE;
}

uint8_t ows_alarm = 0;
