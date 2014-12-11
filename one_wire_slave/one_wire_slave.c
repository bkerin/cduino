// Implementation of the interface described in one_wire_master.h.

#include <assert.h>
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

// This is intended to help ensure that the master and other slaves are
// behaving correctly.  If defined, it turns a number of points which slaves
// can agreeably handle or return an error from into fatal blinky-traps.
// You probably wouldn't want to use this in production, since it's very
// trigger-happy about rejecting anything weird or pointless from the master.
// See the actual use-point of the SMT() (Strict Mode Trap) macro for details.
// FIXME: disable again for release version
#define STRICT_MODE

// This is like STRICT_MODE, but it causes the trap to indicate the trap
// location in the source with its blink pattern.  Note that it doesn't
// make sense to define both this and STRICT_MODE.
//#define STRICT_MODE_WITH_LOCATION_OUTPUT

#if defined(STRICT_MODE) && defined(STRICT_MODE_WITH_LOCATION_OUTPUT)
#  error STRICT_MODE and STRICT_MODE_WITH_LOCATION_OUTPUT both defined
#endif
#if defined (STRICT_MODE)
#  define SMT() BTRAP ()
#elif defined (STRICT_MODE_WITH_LOCATION_OUTPUT)
// FIXME: this is so messy and huge, and has yet to actually be used
#  define SMT() BASSERT_FEEDING_WDT_SHOW_POINT (FALSE)
#else
#  define SMT()
#endif

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
/*  FIXME: trying a diff value here though first is theoretically fine:
#define ST_SLAVE_WRITE_ZERO_LINE_HOLD_TIME \
    (OWC_TICK_DELAY_E + OWC_TICK_DELAY_F / 2)
*/
#define ST_SLAVE_WRITE_ZERO_LINE_HOLD_TIME \
  (OWC_TICK_DELAY_E * 2)

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

// This ISR keeps track of the length of low pulses.  When we see the end
// of one we consider that we've seen a reset and set a client-visible flag.
ISR (DIO_PIN_CHANGE_INTERRUPT_VECTOR (OWS_PIN))
{
  if ( LINE_IS_HIGH () ) {
    //if ( new_pulse  ) { PFP ("FIXME: debug: pulse stack-up detected"); }
    new_pulse = TRUE;
    pulse_length = TIMER1_STOPWATCH_TICKS ();
    // FIXME: might want to check this too in final code:
    //if ( T1OF () ) { PFP ("FIXME: debug: timer overflow detected"); }
  }
  else {
    // FIXME: we used to clear new_pulse here.  This effectively
    // erases any unhandled pulse from our minds.  But is that good?
    // wait_for_pulse_end() clears it anyway, and not clearing it here
    // extends the time in which we could work on it (admittedly while
    // another negative pulse is already in progress...)
    //new_pulse = FALSE;
    T1RESET ();
  }
}

static uint16_t
wait_for_pulse_end (void)
{
  // Wait for the positive edge that occurs at the end of a negative pulse,
  // then return the negative pulse duration in timer1 ticks.  In fact this
  // waits for a flag variable to be set from a pin change ISR, which seems
  // to be considerably more rebust than pure delta detecton would probably
  // be: see pcint_test.c.disabled in the one_wire_slave module directory
  // for some tests I did to verify how this interrupt works.

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

// Call call, Propagating Errors.  The call argument must be a call to a
// function returning ows_err_t.
#define CPE(call)                      \
  do {                                 \
    ows_error_t XxX_err = call;        \
    if ( XxX_err != OWS_ERROR_NONE ) { \
      return XxX_err;                  \
    }                                  \
  } while ( 0 )

ows_error_t
ows_wait_for_command (uint8_t *command_ptr)
{
  ows_wait_for_reset ();

  CPE (ows_read_byte (command_ptr));

  return OWS_ERROR_NONE;
}

uint8_t bhist[22];
uint8_t bhist_ii = 0;

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
        /*
        PFP (
            "wrong bit value %i at byte %i, bit %i\n",
            (int) bit_value,
            (int) ii, (int) jj );
            */
        return OWS_ERROR_ROM_ID_MISMATCH;
      }
    }
  }

  return OWS_ERROR_NONE;
}

// Transaction State Waiting For (Reset Pulse|ROM Command|Function Command).
#define TSWFRP  0
#define TSWFRC  1
#define TSWFFC  2

ows_error_t
ows_wait_for_function_command (uint8_t *command_ptr)
{
  // FIXME: maybe we don't actually need OWC_NULL_COMMAND for anything now

  uint8_t ts = TSWFRP;                // Transaction State
  ows_error_t err = OWS_ERROR_NONE;   // Storage for most recent error

  for ( ; ; ) {
    switch ( ts ) {
      case TSWFRP:
        ows_wait_for_reset ();
        ts = TSWFRC;
        break;
      case TSWFRC:
        err = ows_read_byte (command_ptr);
        if ( err == OWS_ERROR_RESET_DETECTED_AND_HANDLED ) {
          // State doesn't change
          break;
        }
        else if ( err != OWS_ERROR_NONE ) {
          return err;
        }
        // Could just return this byte to do what real DS18B20 does?
        if ( ! OWC_IS_ROM_COMMAND (*command_ptr) ) {
          return OWS_ERROR_DID_NOT_GET_ROM_COMMAND;
        }
        if ( OWC_IS_TRANSACTION_INITIATING_ROM_COMMAND (*command_ptr) ) {
          switch ( *command_ptr ) {
            case OWC_READ_ROM_COMMAND:
              err = ows_write_rom_id ();
              break;
            case OWC_MATCH_ROM_COMMAND:
              err = ows_read_and_match_rom_id ();
              break;
            case OWC_SKIP_ROM_COMMAND:
              // FIXME: WORK POINT: master says it doesn't get a presence
              // pulse when it tries this... perhaps the problem has to
              // do with the 1 that we send immediately after a convert?
              // only a maybe
              err = OWS_ERROR_NONE;
              break;
            default:
              assert (FALSE);  // Shouldn't be here
              break;
          }
          switch ( err ) {
            case OWS_ERROR_NONE:
              ts = TSWFFC;
              break;
            case OWS_ERROR_RESET_DETECTED_AND_HANDLED:
              ts = TSWFRC;
              break;
            case OWS_ERROR_ROM_ID_MISMATCH:
              ts = TSWFRP;
              break;
            default:
              return err;
              break;
          }
        }
        else {
          switch ( *command_ptr ) {
            case OWC_SEARCH_ROM_COMMAND:
              err = ows_answer_search ();
              break;
            case OWC_ALARM_SEARCH_COMMAND:
              err = OWS_MAYBE_ANSWER_ALARM_SEARCH ();
              if ( err != OWS_ERROR_NONE && err != OWS_ERROR_NOT_ALARMED ) {
                return err;
              }
              break;
            default:
              assert (FALSE);  // Shouldn't be here
              break;
          }
          switch ( err ) {
            case OWS_ERROR_NONE:
            case OWS_ERROR_NOT_ALARMED:
              ts = TSWFRP;
              break;
            case OWS_ERROR_RESET_DETECTED_AND_HANDLED:
              ts = TSWFRC;
              break;
            default:
              return err;
              break;
          }
        }
        break;
      case TSWFFC:
        err = ows_read_byte (command_ptr);
        switch ( err ) {
          case OWS_ERROR_RESET_DETECTED_AND_HANDLED:
            ts = TSWFRC;
            break;
          default:
            return err;   // Note that err could be OWS_ERROR_NONE here
            break;
        }
        break;
    }
  }

}

ows_error_t
ows_read_bit (uint8_t *data_bit_ptr)
{
  uint16_t pl = wait_for_pulse_end ();

  if ( pl < ST_SLAVE_READ_SLOT_SAMPLE_TIME * T1TPUS ) {
    *data_bit_ptr = 1;
  }
  // FIXME: use the time quantity symbols here, assuming this is right:
  // this is required to be less than tick delay C + D, D is the margin
  // since we expect the master to go high again after C
  else if ( pl < (60+10) * T1TPUS ) {
    *data_bit_ptr = 0;
  }
  else if ( pl > ST_RESET_PULSE_LENGTH_REQUIRED * T1TPUS ) {
    _delay_us (ST_DELAY_BEFORE_PRESENCE_PULSE);
    OWS_PRESENCE_PULSE ();
    return OWS_ERROR_RESET_DETECTED_AND_HANDLED;
  }
  else {
    // FIXME: so where don't we use SMT?  any errors other than
    // ROM_ID_MISMATCH or NOT_ALARMED (which for that matter aren't really
    // errors)?
    SMT ();   // Because weird pulse lengths shouldn't happen
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
    SMT ();   // Because we shouldn't get reset when master asked us to write
    _delay_us (ST_DELAY_BEFORE_PRESENCE_PULSE);
    OWS_PRESENCE_PULSE ();
    return OWS_ERROR_RESET_DETECTED_AND_HANDLED;
  }
  else {
    SMT ();   // Because weird pulse lengths shouldn't happen
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

// FIXME: note that the actual bit storage order in rom_id is wonky, it
// should be documented somewhere, I believe is least significant byte first,
// but then within bytes it most significant bit first.  Also, we could be
// interpreting bits backwards from the real Maxim slave and not know it...

// Evaluate to the value of Bit Number bn (0-indexed) of rom_id.
#define ID_BIT(bn) \
  ((rom_id[bn / BITS_PER_BYTE]) >> (bn % BITS_PER_BYTE) & B00000001)

ows_error_t
ows_answer_search (void)
{
  /*   FIXME: debug goop
  for ( uint8_t ii = 0 ; ii < OWM_ID_BYTE_COUNT ; ii++ ) {
    uint8_t byte_val = rom_id[ii];
    for ( uint8_t jj = 0 ; jj < BITS_PER_BYTE ; jj++ ) {
      uint8_t bit_val = byte_val & (B10000000 >> jj) ? 1 : 0;
      PFP ("%i", (int) bit_val);
    }
  }
  PFP ("\n");
  */
  for ( uint8_t ii = 0 ; ii < OWM_ID_BYTE_COUNT ; ii++ ) {
    uint8_t byte_val = rom_id[ii];
    for ( uint8_t jj = 0 ; jj < BITS_PER_BYTE ; jj++ ) {
      uint8_t bit_val = byte_val & (B00000001 << jj) ? 1 : 0;
      CPE (ows_write_bit (bit_val));
      CPE (ows_write_bit (! bit_val));
      uint8_t master_bit_val;
      CPE (ows_read_bit (&master_bit_val));
      // This is actually reasonably likely, but if its true we have lots
      // of time, whereas if its not we have to keep up with the master for
      // potentially all the remaining bits in the ID, so we want that path
      // to be fast.
      //if ( UNLIKELY (bit_val != master_bit_val) ) {
      if ( bit_val != master_bit_val ) {
        return OWS_ERROR_NONE;
      }
    }
  }
  /*
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
  */

  return OWS_ERROR_NONE;
}

uint8_t ows_alarm = 0;
