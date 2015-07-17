// Implementation of the interface described in one_wire_slave.h.

#include <assert.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <util/atomic.h>
#include <util/crc16.h>
#include <util/delay.h>

#include "dio.h"
#include "one_wire_common.h"
#include "one_wire_slave.h"
#include "timer1_stopwatch.h"
#include "util.h"

// To get things to work at 10MHz I had to lock these variables into registers
// (see http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_regbind).
// There are many gotchas associated with doing this.  See the "Defining
// Global Register Variables" section of the GCC manual for more details.
// These definition have to come at the start of this file.
//
// I believe doing this is ok, because:
//
//   http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_regbind
//   https://gcc.gnu.org/wiki/avr-gcc#Call-Saved_Registers
//
// See the comments above the reference to OWS_REGISTER_LOCKING_ACKNOWLEDGED
// in one_wire_slave.h for details.
//
register uint8_t ls      asm("r2");   // Line State as of last reading, 1 or 0
register uint8_t cbitv   asm("r3");   // Current Bit Value
register uint8_t cbytevu asm("r4");   // Current Byte Value, Usually
register uint8_t cbiti   asm("r5");   // Current Bit Index

#ifdef OWS_BUILD_RESULT_DESCRIPTION_FUNCTION

char *
ows_result_as_string (ows_result_t result, char *buf)
{
  switch ( result ) {

#  define X(result_code)                                                  \
    case result_code:                                                     \
      assert (strlen (#result_code) < OWS_RESULT_DESCRIPTION_MAX_LENGTH); \
      strcpy_P (buf, PSTR (#result_code));                                \
      break;
    OWS_RESULT_CODES
#  undef X

    default:
      assert (FALSE);   // Shouldn't be here
      break;
  }

  return buf;
}

#endif

// Aliases for some operations from one_wire_commoh.h (for readability).
#define RELEASE_LINE()    OWC_RELEASE_LINE (OWS_PIN)
#define DRIVE_LINE_LOW()  OWC_DRIVE_LINE_LOW (OWS_PIN)
#define SAMPLE_LINE()     OWC_SAMPLE_LINE (OWS_PIN)
#define TICK_DELAY(ticks) OWC_TICK_DELAY (ticks)

// Convert an integer number of microseconds to timer1 ticks for the F_CPU and
// timer1 prescaler divider settings in use.  Integer math is used throughout,
// so if the us argument is small significant truncation error may result.
#define US2T1T(us) \
  (us * CLOCK_CYCLES_PER_MICROSECOND () / TIMER1_STOPWATCH_PRESCALER_DIVIDER)

static uint8_t rom_id[OWC_ID_SIZE_BYTES];

static void
set_rom_id (uint8_t use_eeprom_id)
{
  // Load the ROM ID into rom_id, using EEPROM data if use_eeprom_id is
  // TRUE, or the default part ID otherwise.  In either case the matching
  // CRC is added.

  uint8_t const ncb = OWC_ID_SIZE_BYTES - 1;   // Non-CRC Bytes in the ROM ID
  uint8_t const pib = ncb - 1;                 // Part ID Bytes in the ROM ID

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

#define TCCR1A_DEFAULT_VALUE UINT8_C (0x00)

void
ows_init (uint8_t use_eeprom_id)
{
  set_rom_id (use_eeprom_id);

  timer1_stopwatch_init ();
  // In addition to the functionality implied by timer1_stopwatch_init(),
  // we're going to use the output compare unit to generate timeout
  // interrupts.  Here we verify that TCCR1A still has the default value
  // specified for it in timer1_stopwatch.h, so we don't accidently twiddle
  // any output pins.
  assert (TCCR1A == TCCR1A_DEFAULT_VALUE);

  RELEASE_LINE ();   // Serves to initialize pin along the way
  DIO_ENABLE_PIN_CHANGE_INTERRUPT_FLAG_ONLY (OWS_PIN);

  // Note that since we're not going to actually use an ISR but instead
  // monitor the pin change interrupt flag, we don't need to actually enable
  // interrupts here (i.e. no sei() call required).

  ows_set_timeout (OWS_TIMEOUT_NONE);
}

// The following ST_* (Slave Timing) macros contain timing values that
// actual Maxim DS18B20 devices have been found to use, or values that
// we have derived logically from our understanding of the protocol; see
// one_wire_master.c.probe from the one_wire_master module for the (probably
// out-of-date) source of the experimental values.  Some of these got tweaked
// a tiny bit at some point from the pure experimental values to divide
// evenly by two or something, but it shouldn't have been enough to matter.

// The DS18B20 datasheet and AN126 both say masters are supposed to send
// 480 us pulse minimum.
#define ST_RESET_PULSE_LENGTH_REQUIRED 240

// The DS18B20 datasheet says 15 to 60 us.
#define ST_DELAY_BEFORE_PRESENCE_PULSE 28

// The DS18B20 datasheet says 60 to 240 us.  It would be bad for us to choose
// 240 us, since that would be as long as the minimum we require for reset
// pulses, so we wouldn't be able to distinguish our own presence pulse
// from a reset pulse that the master might decide to send simultaneously.
#define ST_PRESENCE_PULSE_LENGTH 116

// This is the longest that this slave implementation ever holds the line
// low itself.  This is relevant because we want to let our interrupt handler
// do all the resetting of the hardware timer that we use to detect reset
// pulses without having to flip the reset detector on and off or anything.
// This policy avoids timer access atomicity issues and generally keeps
// things simple.  The consequence is that we end up requiring reset
// pulses up to this much longer than the experimentally measured value of
// ST_RESET_PULSE_LENGTH_REQUIRED.  We have to do that because the interrupt
// handler counts interrupts caused when the slave itself drives the line
// low, so that time ends up counting towards reset pulse time.  Because
// ST_RESET_PULSE_LENGTH_REQUIRED + ST_LONGEST_SLAVE_LOW_PULSE_LENGTH is
// still considerably less than the 480 us pulse that well-behaved masters
// are required to send, this shouldn't be a problem.  The value of this
// macro is ST_PRESENCE_PULSE_LENGTH because that's the longest the slave
// should ever have to hold the line low.
#define ST_LONGEST_SLAVE_LOW_PULSE_LENGTH ST_PRESENCE_PULSE_LENGTH

// This is the reset pulse lengh this slave implementation requires.
// See the comments for the addends in the value for details.
#define OUR_RESET_PULSE_LENGTH_REQUIRED \
  (ST_RESET_PULSE_LENGTH_REQUIRED + ST_LONGEST_SLAVE_LOW_PULSE_LENGTH)

// The exact factoring used here could be a bit different.  For example
// we don't necessarily need a different state for each of the commands,
// they could just be done directly following the read.  Note that there
// is no Doing Skip ROM Command becaus ein that case there' snothing at
// all to do, so we go straight to SRFC.
#define SWFR  0   // State Waiting For Reset
#define SPPP  1   // State Pre-Presense Pulse
#define SRRC  2   // State Reading ROM Command
#define SDSRC 3   // State Doing Search ROM Command
#define SDRRC 4   // State Doing Read ROM Command
#define SDMRC 5   // State Doing Match ROM Command
#define SDASC 6   // State Doing Alarm Search (ROM) Command
#define SRFC  7   // State Reading Function Command
volatile uint8_t state;

// Timeout, in timer1 ticks.
uint16_t timeout_t1t;

void
ows_set_timeout (uint16_t time_us)
{
  // Insist on the interface requirements.
  if ( time_us != OWS_TIMEOUT_NONE ) {
    assert (time_us >= OWS_MIN_TIMEOUT_US);
    assert (time_us <= OWS_MAX_TIMEOUT_US);
  }

  timeout_t1t = time_us * OWS_TIMER_TICKS_PER_US;
}

uint16_t tr;       // Timer Reading (most recent)

// Read Bit Sample Time.  Time from negative edge to slave sample point
// when reading a bit, in microseconds.  We do like the master and give
// OWC_TICK_DELAY_E from the time when the master is supposed to have the
// line set.
#define RBST_TICK_DELAY (OWC_TICK_DELAY_A + OWC_TICK_DELAY_E)

// Zero Pulse Time in Ticks.  Here we do what Figure 1 of
// Maxim_Application_Note_AN126.pdf proscribes, even though it seems stupid.
// We might as well do it, since we can't change the fact that the master
// holds the line low almost to the end of the time slot when writing a
// zero anyway, giving us the same tight timing issue when transitioning
// between a master-write-0 and a master-read-0 that we get here between
// master-read-0 operations.
#define ZPTT \
  (OWC_TICK_DELAY_A + OWC_TICK_DELAY_E + OWC_TICK_DELAY_F - OWC_TICK_DELAY_D)

// Clear Pin Change Flag and Reset Timer1
#define CPCFRT1()                                  \
  do {                                             \
    DIO_CLEAR_PIN_CHANGE_INTERRUPT_FLAG (OWS_PIN); \
    TIMER1_STOPWATCH_CLEAR_OVERFLOW_FLAG ();       \
    TIMER1_STOPWATCH_FAST_RESET ();                \
  } while ( 0 )

// Check For Reset, in other words check if timer reading taken at the end
// of the last negative pulse was large enough that it was a reset.
#define CFR() (tr >= US2T1T (OUR_RESET_PULSE_LENGTH_REQUIRED))

static ows_result_t
wfpcoto (void)
{
  // Wait For Pin Change Or Time Out.  If a change is detected record the
  // timer reading and return OWS_RESULT_SUCCESS, if a timeout is detected
  // return OWS_RESULT_TIMEOUT.

  while ( TRUE ) {
    if ( LIKELY (DIO_PIN_CHANGE_INTERRUPT_FLAG (OWS_PIN)) ) {
      ls = SAMPLE_LINE ();
      // Note: we could in in theory check for resets right here since
      // callers of this routine always have to do it.  Then we could
      // dispense with tr.  But there aren't any real savings and I don't
      // want to risk making the optimizer do something different.
      DIO_CLEAR_PIN_CHANGE_INTERRUPT_FLAG (OWS_PIN);
      tr = TIMER1_STOPWATCH_TICKS ();
      // If we've had timer overflow, treat it as a really long pulse.
      if ( UNLIKELY (TIMER1_STOPWATCH_OVERFLOWED ()) ) {
        tr = UINT16_MAX;
      }
      TIMER1_STOPWATCH_CLEAR_OVERFLOW_FLAG ();
      TIMER1_STOPWATCH_FAST_RESET ();   // do we want fast reset or normal?
      return OWS_RESULT_SUCCESS;
    }
    else if ( UNLIKELY (TIMER1_STOPWATCH_TICKS () >= timeout_t1t) ) {
      if ( timeout_t1t != OWS_TIMEOUT_NONE ) {
        return OWS_RESULT_TIMEOUT;
      }
    }
  }
}

// Call call, Propagating Errors.  The call argument must be a call to a
// function returning ows_result_t.
#define CPE(call)                                     \
  do {                                                \
    ows_result_t XxX_err = call;                      \
    if ( UNLIKELY (XxX_err != OWS_RESULT_SUCCESS) ) { \
      return XxX_err;                                 \
    }                                                 \
  } while ( 0 )

static ows_result_t
read_bit (void)
{
  while ( TRUE ) {
    CPE (wfpcoto ());
    if ( ls ) {
      if ( UNLIKELY (CFR ()) ) {
        return OWS_RESULT_GOT_UNEXPECTED_RESET;
      }
    }
    else {
      // According to measurements made with timer0, even at 16MHz
      // F_CPU, it takes about 4 us to get from the point where wfpcoto()
      // first detects a change to here.  In this case it doesn't matter
      // since the low pulses sent by the master are a full 60 us, but
      // it's a nice illustration of how tight the timing ends up being.
      // Its the time between the end of a master-write-0 and subsequent
      // master-read-0, or a between the end of one master-read-0 and
      // a subsequent master-read-0 where the timing crunch really hits.
      // I haven't measure that directly, but presumably its similarly tight,
      // hence the sensitivity to register variable use.
      TICK_DELAY (RBST_TICK_DELAY);
      cbitv = SAMPLE_LINE ();
      CPCFRT1 ();
      return OWS_RESULT_SUCCESS;
    }
  }
}

static ows_result_t
write_bit (void)
{
  while ( TRUE ) {
    CPE (wfpcoto ());
    if ( UNLIKELY (ls) ) {
      if ( UNLIKELY (CFR ()) ) {
        return OWS_RESULT_GOT_UNEXPECTED_RESET;
      }
    }
    else {
      if ( LIKELY (! cbitv) ) {
        DRIVE_LINE_LOW ();
        TICK_DELAY (ZPTT);
        RELEASE_LINE ();
        CPCFRT1 ();
      }
      return OWS_RESULT_SUCCESS;
    }
  }
}

static ows_result_t
read_byte (void)
{
  cbytevu = 0;
  for ( cbiti = 0 ; cbiti < BITS_PER_BYTE ; cbiti++ ) {
    CPE (read_bit ());
    cbytevu |= (cbitv << cbiti);
  }

  return OWS_RESULT_SUCCESS;
}

static ows_result_t
write_byte (void)
{
  for ( cbiti = 0 ; cbiti < BITS_PER_BYTE ; cbiti++ ) {
    cbitv = cbytevu & (B00000001 << cbiti);
    CPE (write_bit ());
  }

  return OWS_RESULT_SUCCESS;
}

ows_result_t
ows_write_bit (uint8_t bit_value)
{
  cbitv = bit_value;
  CPE (write_bit ());

  return OWS_RESULT_SUCCESS;
}

// FIXME: needs tested, though its trivially diff from its inner fctn
ows_result_t
ows_read_bit (uint8_t *bit_value_ptr)
{
  CPE (read_bit ());
  *bit_value_ptr = cbitv;

  return OWS_RESULT_SUCCESS;
}

ows_result_t
ows_write_byte (uint8_t byte_value)
{
  cbytevu = byte_value;
  CPE (write_byte ());

  return OWS_RESULT_SUCCESS;
}

// FIXME: needs tested, though its trivially diff from its inner fctn
ows_result_t
ows_read_byte (uint8_t *byte_value_ptr)
{
  CPE (read_byte ());
  *byte_value_ptr = cbytevu;

  return OWS_RESULT_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////

static ows_result_t
read_and_match_rom_id (void)
{
  // NOTE: we're using cbytevu as an index variable here (it's register
  // locked and we don't want to completely change its mostly-right name :).
  for ( cbytevu = 0 ; cbytevu < OWC_ID_SIZE_BYTES ; cbytevu++ ) {
    for ( cbiti = 0 ; cbiti < BITS_PER_BYTE ; cbiti++ ) {
      CPE (read_bit ());
      if ( cbitv != ((rom_id[cbytevu]) >> cbiti) ) {
        return OWS_RESULT_ROM_ID_MISMATCH;
      }
    }
  }

  return OWS_RESULT_SUCCESS;
}

// Evaluate to the value of Bit Number bn (0-indexed) of rom_id.
#define ROM_ID_BIT(bn) \
  (((rom_id[bn / BITS_PER_BYTE]) >> (bn % BITS_PER_BYTE)) & B00000001)

// This is exposed in the interface.  Clients are supplsed to set it to
// non-zero to indicate an alarm condition.
uint8_t ows_alarm = 0;

static ows_result_t
answer_search (void)
{
  // ROM ID Size, in bits
  uint8_t const rids_bits = OWC_ID_SIZE_BYTES * BITS_PER_BYTE;
  for ( cbiti = 0 ; cbiti < rids_bits ; cbiti++ ) {
    // NOTE: we're using cbytevu as storage for the current ROM ID bit value
    // (it's register locked so we want to use it, but we don't want to
    // completely change its mostly-right name :).
    cbytevu = ROM_ID_BIT (cbiti);   // Current ROM ID Bit Value
    cbitv = cbytevu;
    CPE (write_bit ());
    cbitv = ! cbitv;
    CPE (write_bit ());
    CPE (read_bit ());
    if ( UNLIKELY (cbitv != cbytevu) ) {
      // Mismatches are successes -- see comment above in this function.
      return OWS_RESULT_SUCCESS;
    }
  }

  return OWS_RESULT_SUCCESS;
}

// This is the proper response to a reset pulse.  It includes the delay and
// the presence pulse, and consumer the resulting presence pulse event,
// or else goes to unexpected_event if something other that presence
// pulse occurs.
#define PRESENCE_PULSE_SEQUENCE()               \
  do {                                          \
    _delay_us (ST_DELAY_BEFORE_PRESENCE_PULSE); \
    DRIVE_LINE_LOW ();                          \
    _delay_us (ST_PRESENCE_PULSE_LENGTH);       \
    RELEASE_LINE ();                            \
  } while ( 0 )

ows_result_t
ows_wait_for_function_transaction (uint8_t *command_ptr, uint8_t jgur)
{
  uint8_t state = (jgur ? SPPP : SWFR);

  CPCFRT1 ();

  while ( TRUE ) {
    switch ( state ) {
      case SWFR:
        {
          CPE (wfpcoto ());
          if ( ls ) {
            if ( CFR () ) {
              state = SPPP;
            }
          }
          break;
        }
      case SPPP:
        PRESENCE_PULSE_SEQUENCE ();
        CPCFRT1 ();
        state = SRRC;
        break;
      case SRRC:
        {
          CPE (read_byte ());
          switch ( cbytevu ) {
            case OWC_SEARCH_ROM_COMMAND:
              state = SDSRC;
              break;
            case OWC_READ_ROM_COMMAND:
              state = SDRRC;
              break;
            case OWC_MATCH_ROM_COMMAND:
              state = SDMRC;
              break;
            case OWC_SKIP_ROM_COMMAND:
              state = SRFC;
              break;
            case OWC_ALARM_SEARCH_COMMAND:
              state = SDASC;
              break;
            default:
              return OWS_RESULT_ERROR_GOT_INVALID_ROM_COMMAND;
              break;
          }
          break;
        }
      case SDSRC:
        CPE (answer_search ());
        state = SWFR;
        break;
      case SDRRC:
        for ( uint8_t ii = 0 ; ii < OWC_ID_SIZE_BYTES ; ii++ ) {
          cbytevu = rom_id[ii];
          CPE (write_byte ());
        }
        state = SRFC;
        break;
      case SDMRC:
        {
          ows_result_t result = read_and_match_rom_id ();
          if ( result == OWS_RESULT_ROM_ID_MISMATCH ) {
            state = SWFR;
          }
          else {
            // Note that both success and non-mismatch results end up here.
            return result;
          }
          break;
        }
      case SDASC:
        if ( UNLIKELY (! ows_alarm) ) {
          return OWS_RESULT_SUCCESS;
        }
        CPE (answer_search ());
        state = SWFR;
        break;
      case SRFC:
        {
          CPE (read_byte ());
          *command_ptr = cbytevu;
          return OWS_RESULT_SUCCESS;
          break;
        }
      default:
        assert (FALSE);  // Shouldn't be here
        break;
    }
  }
}
