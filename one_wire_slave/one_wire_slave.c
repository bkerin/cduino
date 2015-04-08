// Implementation of the interface described in one_wire_slave.h.

#include <assert.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <string.h>
#include <util/atomic.h>
#include <util/crc16.h>
#include <util/delay.h>

#include "debug_led.h"
#include "dio.h"
#include "one_wire_common.h"
#include "one_wire_slave.h"
#include "timer1_stopwatch.h"
// FIXME: next two lines for debug only:
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

// WARNING: if defined, this creates trap points that deliberately prevent
// the watchdog timer from triggering a reset.
//
// WARNING: its possible that the code added by this define might induce
// hiesenburgs, especially at lower CPU frequencies.
//
// WARNING: these traps are so strict that the tests in one_wire_master_test.h
// will trip them (because they do things like issue two resets in a row
// without issuing a command).
//
// Note that since this uses some stuff from debug_led.h, it may be necessary
// to define DBL_PIN in your Makefile some of the blinky macros from util.h
// for this to work if you aren't using the normal PB5-connected on-board LED.
//
// This is intended both to help ensure that the master and other slaves
// are behaving correctly, and to catch failures in this slave itself.
// If defined, it turns a number of points which slaves can agreeably handle
// or return an error from into fatal blinky-traps, and also adds some code
// to the pin change ISR to detect cases where this slave itself is too slow
// to catch a reset pulse.  You wouldn't want to use this in production,
// since it's very trigger-happy about rejecting anything weird or pointless,
// and could theoretically be triggered by a burst of noise on the line.
// For example, the traps this inserts will trigger if the master sends an
// asynchronous reset when the slave is expecting to read or write a bit,
// which is a perfectly reasonable thing for a production master to decide
// to do (it just shouldn't happen accidently).  See the actual use-points
// of the SMT() (Strict Mode Trap) macro for details.
//#define STRICT_MODE

// WARNING: see the above warnings for STRICT_MODE.
//
// This is like STRICT_MODE, but it causes the trap to indicate the trap
// location in the source with its blink pattern.  Note that it doesn't
// make sense to define both this and STRICT_MODE.  Note also that this
// makes the code huge.
//#define STRICT_MODE_WITH_LOCATION_OUTPUT

#if defined(STRICT_MODE) && defined(STRICT_MODE_WITH_LOCATION_OUTPUT)
#  error STRICT_MODE and STRICT_MODE_WITH_LOCATION_OUTPUT both defined
#endif
#if defined (STRICT_MODE)
#  define SMT() DBL_TRAP ()
#elif defined (STRICT_MODE_WITH_LOCATION_OUTPUT)
#  define SMT() DBL_ASSERT_NOT_REACHED_SHOW_POINT ();
#else
#  define SMT()
#endif

// Aliases for some operations from one_wire_commoh.h (for readability).
#define RELEASE_LINE()    OWC_RELEASE_LINE (OWS_PIN)
#define DRIVE_LINE_LOW()  OWC_DRIVE_LINE_LOW (OWS_PIN)
#define SAMPLE_LINE()     OWC_SAMPLE_LINE (OWS_PIN)
#define TICK_DELAY(ticks) OWC_TICK_DELAY (ticks)

// Timer1 Ticks Per microsecond.
#define T1TPUS \
  (CLOCK_CYCLES_PER_MICROSECOND () / TIMER1_STOPWATCH_PRESCALER_DIVIDER)

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
// of the experimental values.  Some of these got tweaked a tiny bit at
// some point from the pure experimental values to divide evenly by two or
// something, but it shouldn't have been enough to matter.

// The DS18B20 datasheet and AN126 both say masters are supposed to send
// 480 us pulse minimum.
#define ST_RESET_PULSE_LENGTH_REQUIRED 240

// The DS18B20 datasheet says 15 to 60 us.
#define ST_DELAY_BEFORE_PRESENCE_PULSE 28

// The DS18B20 datasheet says 60 to 240 us.  It would be bad for us to
// choose 240 us, since that would be as long as the minimum we require
// for reset pulses, so we wouldn't be able to distinguish our own presence
// pulse from a new reset pulse.
#define ST_PRESENCE_PULSE_LENGTH 116

// The DS18B20 datasheet says at least 1 us required from master, but the
// actual DS18B20 devices seem even the shortest blip as signaling the start
// of a slot.  So this one-cycle time is sort of a joke, and in fact its best
// to not wait at all so we don't have to worry about the actual timer delay.
#define ST_REQUIRED_READ_SLOT_START_LENGTH (1000000.0 / F_CPU)

// This is the time the DS18B20 diagram indicates that it typically waits
// from the time the line is pulled low by the master to when it (the slave)
// samples when reading a bit.
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
// See Figure 1 of Maxim_Application_Note_AN126.pdf, page 2.  We go with
// OWC_TICK_DELAY_F / 2 here because it's, well, half way between when
// we must have the line held low and when we must release it.  We could
// probably measure what actual slaves do if necessary.  Waiting F/2 has
// the disadvantage of pushing the total low time closer to the length of
// a presence pulse, though its still 20+ ms short of the minimum required
// time so it should be ok, and it gives the master more time to get around
// to reading the line compared to the alternative below.
#define ST_SLAVE_WRITE_ZERO_LINE_HOLD_TIME \
    (OWC_TICK_DELAY_E + OWC_TICK_DELAY_F / 2)
// This also worked super dependably, which isn't too surprising: given the
// timing they both should be fine.  This one gives a margin of E on both
// sides of the anticipated sample point, which makes a lot of sense also
// (compared to the F/2 that we use above).  This keeps the total zero pulse
// length farther from that of a presense pulse, but gives the master less
// time to get around to reading the zero pulse.
/*
#define ST_SLAVE_WRITE_ZERO_LINE_HOLD_TIME \
  (OWC_TICK_DELAY_E * 2)
*/

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

// Readability macros
#define LINE_IS_HIGH() (  SAMPLE_LINE ())
#define LINE_IS_LOW()  (! SAMPLE_LINE ())

#define SW0B  0   // State Writing 0 Bit
#define SW1B  1   // State Writing 1 Bit
#define SRB   2   // State Reading Bit
#define SATRL 3   // State About To Release Line
#define SWFR  4   // State Waiting For Reset


volatile uint8_t state;

#define ENY   0   // Event Nothing Yet
#define ER0   1   // Event Read 0
#define ER1   2   // Event Read 1
#define ESW0  3   // Event Started Writing 0
#define ESW1  4   // Event Started Writing 1
#define ELR   5   // Event Line Released
#define EGR   6   // Event Got Reset
#define EGUPL 7   // Event Got Unexpected Pulse Length
volatile uint8_t event;

// When the pin change ISR observes any change, it sets new_line_activity.
// When it observes a positive edge, it sets new_pulse and records the
// pulse_length in timer1 ticks of the new pulse.
volatile uint8_t new_line_activity = FALSE;
volatile uint8_t new_pulse = FALSE;
volatile uint16_t pulse_length;

volatile uint8_t zero_queued = FALSE;

// This ISR keeps track of the length of low pulses.  When we see the end
// of one we consider that we've seen a reset and set a client-visible flag.
ISR (DIO_PIN_CHANGE_INTERRUPT_VECTOR (OWS_PIN))
{
  if ( LINE_IS_HIGH () ) {


    uint16_t pl;   // Pulse Length
    if ( ! TIMER1_STOPWATCH_OVERFLOWED () ) {
      pl = TIMER1_STOPWATCH_TICKS ();   // Pulse Length
    }
    else {
      // If we've had overflow, its a really long pulse, so set to max.
      pl = UINT16_MAX;
    }

    // Any pulse longer than the minimum required reset produces an EGR event.
    if ( pl > OUR_RESET_PULSE_LENGTH_REQUIRED * T1TPUS ) {
      event = EGR;
      return;
    }

    switch ( state ) {
      case SW0B:
        if ( pl < ST_SLAVE_WRITE_SLOT_START_PULSE_MAX_LENGTH * T1TPUS ) {
          // Right here we have the tightest timing window in the 1-wire
          // protocol.  For the slave to write a 0, we have to ensure
          // that the line is low at most 9 us after the end of the pulse
          // the master sends to direct us to write a bit.  See Fig. 1 of
          // Maxim_Application_Note_AN126.pdf.  We therefore drive the line
          // low here immediately, rather than letting the main thread do it.
          // FIXME: this presumably triggers another interrupt after this
          // one is returned for the new line change, but thats fine as all
          // it will do is reset the timer, but verify this.
          DRIVE_LINE_LOW ();
          event = ESW0;
        }
        else {
          event = EGUPL;
        }
        return;
      case SW1B:
        if ( pl < ST_SLAVE_WRITE_SLOT_START_PULSE_MAX_LENGTH * T1TPUS ) {
          event = ESW1;
        }
        else {
          event = EGUPL;
        }
        return;
      case SRB:
        if ( pl < ST_SLAVE_READ_SLOT_SAMPLE_TIME * T1TPUS ) {
          event = ER1;
        }
        // Note that D is the margin since we expect the master to go high
        // again after C.
        else if ( pl < ( OWC_TICK_DELAY_C + OWC_TICK_DELAY_D ) * T1TPUS ) {
          event = ER0;
        }
        else {
          event = EGUPL;
        }
        return;
      case SATRL:
        // We don't try to mask off the interrupt that occurs when we releate
        // the line ourselves, we just ignore it.  Note that if the master
        // happens to be sending a reset pulse when we release the line,
        // it won't go high until the end of that reset pulse, and we'll
        // end up detecting the reset pulse above and not ending up here.
        event = ELR;
        return;
      case SWFR:
        // Pulses long enough to be resets were caught above, so if we're
        // here it's an unexpected pulse length (which in this context
        // probably just means another slave talking).
        event = EGUPL;
        return;
      default:
        PFP_ASSERT_NOT_REACHED ();   // FIXME: debug, do something different...
        break;
    }

  }
  else {

    TIMER1_STOPWATCH_RESET ();

  }
}

uint8_t wfec = 0;
uint8_t eh[UINT8_MAX] = { 0 };

static void
peh (void)
{
  printf ("wfec: %lu\n", (unsigned long) wfec);
  for ( uint8_t ii = 0 ; ii < wfec ; ii++ ) {
    printf ("eh[%lu]: %lu\n", (unsigned long) ii, (unsigned long) eh[ii]);
  }
}

static uint8_t
wait_for_event (void)
{
  uint8_t ne = ENY;
  while ( ne == ENY ) {
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
    {
      if ( event != ENY ) {
        //printf ("event: %lu\n", (long unsigned) event);
        ne = event;
        event = ENY;
        //printf ("ne: %lu\n", (long unsigned) ne);
      }
    }
  }

  //DBL_TRAP ();

  eh[wfec++] = ne;
  if ( wfec > 66 ) {
      printf ("wfec: %lu\n", (unsigned long) wfec);
      for ( uint8_t ii = 0 ; ii < 8 ; ii++ ) {
        printf ("eh[%lu]: %lu\n", (unsigned long) ii, (unsigned long) eh[ii]);
      }
      assert (FALSE);   // Shouldnt be here FIXME: correct?
  }

  return ne;
}

// Convert microseconds (as an integer) to timer1 ticks (rounded down).
#define US2T1T(us) \
  (MICROSECONDS_TO_CLOCK_CYCLES(us) / TIMER1_STOPWATCH_PRESCALER_DIVIDER)

//uint16_t ows_timeout_us = OWS_NO_TIMEOUT;

// FIXME: I think OWS_NO_TIMEOUT should turn into OWS_TIMEOUT_NONE
// Timout setting in timer1 ticks, or OWS_NO_TIMEOUT
uint16_t ows_timeout_t1t = OWS_NO_TIMEOUT;

void
ows_set_timeout (uint16_t timeout_us)
{
  ows_timeout_t1t = US2T1T (timeout_us);
  PFP ("ows_timeout_t1t: %lu\n", (long unsigned) ows_timeout_t1t);
}

// Transaction State Waiting For (Reset Pulse|ROM Command|Function Command).
#define TSWFRP  0
#define TSWFRC  1
#define TSWFFC  2

#define WAIT_FOR_EVENT() do { ; } while ( event == ENY );

#define PRESENCE_PULSE_SEQUENCE()               \
  do {                                          \
    _delay_us (ST_DELAY_BEFORE_PRESENCE_PULSE); \
    DRIVE_LINE_LOW ();                          \
    _delay_us (ST_PRESENCE_PULSE_LENGTH);       \
  } while ( 0 )

// FIXME: finish and use
//#define COMPLETE_ZERO_PULSE() \

uint8_t ns;   // Next State (one to go to after end of a sent zero pulse)

uint8_t cb;   // Current Byte

uint8_t cri;   // Current ROM Byte Index
uint8_t cbi;   // Current Bit Index

// FIXME: WORK POINT: this is part of a new attempt to flatten everything to
// use a single state machine and fewer function calls.  I think its got to be
// all the function stack frame push-pop that's causing things to be too slow.

#define SWFR 0   // State Waiting For Reset
#define SSPP 1   // State Sent Presence Pulse
#define SRRC 2   // State Reading ROM Command
#define SWID 3   // State Writing ID

ows_error_t
ows_wait_for_function_transaction_2 (uint8_t *command_ptr)
{
  for ( ; ; ) {
    WAIT_FOR_EVENT ();
    // FIXME: could handle presence pulses up front, but it makes service
    // of the tight-timing cases a tiny bit slower...
    switch ( state ) {
      case SWFR:
        // This is handled below, since it can occurr from any state.
        break;
      case SSPP:
        switch ( event ) {
          case EPP:
            state = SRRC;
            break;
          default:
            assert (0);
            break;
        }
        break;
      case SRRC:
        switch ( event ) {
          case ER0:
            cb |= B00000001 << (cbi++);
            break;
          case ER1:
            cb &= ~(B00000001 << (cbi++));
            break;
          default:
            assert (0);
            break;
        }
        if ( cbi >= BITS_PER_BYTE ) {
          assert (cbi == BITS_PER_BYTE);   // FIXME: remove after debug
          cri = 0;
          cbi = 0;
          // FIXME: need to not assume READ_ROM here
          state = SWID;
        }
        break;
      case SWID:
        if ( ! (rom_id[cri] & (0x01 << (cbi++))) ) {
          _delay_us (ST_SLAVE_WRITE_ZERO_LINE_HOLD_TIME);
          RELEASE_LINE ();
          if ( cbi >= BITS_PER_BYTE ) {
            assert (cbi == BITS_PER_BYTE);  // FIXME: remove after debug
            cri++;
          }
          if ( cri >= OWC_ID_SIZE_BYTES ) {
            assert (cri == OWC_ID_SIZE_BYTES);
            ns = SRFC;
            state = SSZP;
          }
          else {
            ns = SWID;
            state = SSZP;
          }
        }
        else {
          if ( cbi >= BITS_PER_BYTE ) {
            assert (cbi == BITS_PER_BYTE);  // FIXME: remove after debug
            cri++;
          }
          if ( cri >= OWC_ID_SIZE_BYTES ) {
            assert (cri == OWC_ID_SIZE_BYTES);
            state = SRFC;
          }
          else {
            // We're still writing the ID, so the state doesn't change
            ;
          }
        }
        break;
      case SSZP:
        if ( EZP ) {
          state = ns;
        }
        break;
      case SRFC:
        PFP ("woo ha!\n");
        assert (0);

    }

    if ( event == ERP ) {
      PRESENCE_PULSE_SEQUENCE ();
      state = SSPP;
    }

  }

}

ows_error_t
ows_wait_for_function_transaction (uint8_t *command_ptr)
{
  uint8_t ts = TSWFRP;                // Transaction State
  ows_error_t err = OWS_ERROR_NONE;   // Storage for most recent error

  for ( ; ; ) {
    switch ( ts ) {
      case TSWFRP:
        err = ows_wait_for_reset ();
        if ( err != OWS_ERROR_NONE ) {
          return err;
        }
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
        // ProbablyDontFIXXME: We might be able to just return this byte to
        // do what real DS18B20 does with respect to commands that don't
        // include any addressing (i.e. no Step 2 of the the "TRANSACTION
        // SEQUENCE" described in the DS18B20 datasheet).  I haven't thought
        // it through fully though, and the interface currently says that
        // we return an error in this case.
        if ( ! OWC_IS_ROM_COMMAND (*command_ptr) ) {
          SMT ();
          return OWS_ERROR_DID_NOT_GET_ROM_COMMAND;
        }
        if ( OWC_IS_TRANSACTION_INITIATING_ROM_COMMAND (*command_ptr) ) {
          switch ( *command_ptr ) {
            case OWC_READ_ROM_COMMAND:
              err = ows_write_id ();
              break;
            case OWC_MATCH_ROM_COMMAND:
              err = ows_read_and_match_id ();
              break;
            case OWC_SKIP_ROM_COMMAND:
              err = OWS_ERROR_NONE;
              break;
            default:
              //DBL_TRAP ();   // FIXME: debug
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
                // In strict mode, asynchronous resets in particular aren't
                // acceptable here.  To actually hit this code, it would be
                // necessary to remove the SMT() call from one of the bit
                // commands -- but it might be worth doing this if things
                // aren't working.  Note that we don't have SMT() traps in
                // place for all higher-level failure points.
                SMT ();
              }
              break;
            default:
              //DBL_TRAP ();   // FIXME: debug
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

// Call call, Propagating Errors.  The call argument must be a call to a
// function returning ows_error_t.
#define CPE(call)                      \
  do {                                 \
    ows_error_t XxX_err = call;        \
    if ( XxX_err != OWS_ERROR_NONE ) { \
      return XxX_err;                  \
    }                                  \
  } while ( 0 )

// Handle a (presumably just received) reset pulse, by delaying a short time,
// then sending a presence pulse, swallowing the pulse end that results
// hopefully from the presence pulse itself, and repeating the procedure
// until a pulse shorter than a reset pulse is received.  By verifying
// that the presence pulse is short enough, we're handling the situation
// where the master sends a series of reset pulses in a row without paying
// attention to our answering presence pulse).
// FIXME: maybe we dont want the OWS prefix, since this is private?
// FIXME: should this be a function?
// FIXME: test that reset trains actually work (needs test from master side)
//
// FIXME: the fact that this gets called from ows_wait_for_reset() probably
// breaks the promises that routine makes since we call wait_for_pulse_end()
// in a loop without counting time.  Its probably simpler to just fix
// the innermost wait loop to handle timeouts, and would give a better
// interfact too
#define OWS_HANDLE_RESET_PULSE()                          \
  do {                                                    \
    _delay_us (ST_DELAY_BEFORE_PRESENCE_PULSE);           \
    DRIVE_LINE_LOW ();                                    \
    _delay_us (ST_PRESENCE_PULSE_LENGTH);                 \
    RELEASE_LINE ();                                      \
  } while ( wait_for_pulse_end ()                         \
            >= OUR_RESET_PULSE_LENGTH_REQUIRED * T1TPUS );

#define SET_STATE(sv) ATOMIC_BLOCK (ATOMIC_RESTORESTATE) { state = sv; }

// FIXME: public or private?
static ows_error_t
ows_handle_reset_pulse (void)
{
  while ( TRUE ) {

    _delay_us (ST_DELAY_BEFORE_PRESENCE_PULSE);
    DRIVE_LINE_LOW ();
    _delay_us (ST_PRESENCE_PULSE_LENGTH);
    SET_STATE (SATRL);
    RELEASE_LINE ();

    uint8_t ne = wait_for_event ();

    switch ( ne ) {
      case ELR:
        return OWS_ERROR_NONE;
      case EGR:
        // We got another reset (so we're going to go around again).
        break;
      default:
        // FIXME: maybe propagate an error from here.  This is almost in
        // the should-never-happen category.  If we got a reset pulse, the
        // only kind of 1-wire activity that should be able to stomp on our
        // presence pulse is another reset pulse.  Of course, there might
        // be some other badly-behaved slave on the network (for example one
        // that talks while we're pausing before sending our presence pulse),
        // so probably this should be an error expressing that.
        PFP_ASSERT_NOT_REACHED ();
        assert (FALSE);
        break;
    }
  }
}

ows_error_t
ows_wait_for_reset (void)
{
  while ( TRUE ) {
    if ( wait_for_event () == EGR ) {
      return ows_handle_reset_pulse ();
    }
  }
}

// Drive the line low for the time required to indicate a value of zero
// when writing a bit, then call wait_for_pulse_end() to swallow the pulse
// that this causes the ISR to detect.  FIXME: this has the same problem
// as OWS_PRESENCE_PULSE() used to: it might end up eating a reset pulse.
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

  // ProbabablyDontFIXXME: our general strategy of waiting for the end of a
  // pulse and considering the length of the detected pulse to necessarily
  // constitute a deliberate signal from the master is potentially less
  // robust than if we somehow polled continuously and threw out everything
  // that didn't fall at the particular sample point of interest.  In the
  // presense of noise that manages to jerk the line high despite the master
  // trying to hold it low, our strategy will fail for any premature noise,
  // not just noise that happens to fall at the sample point.  Come to
  // think of it, I guess we could wait for pulses in a loop until we get an
  // elapsed time that puts us at what we consider the optimal sample point
  // for a given operation.  But if we find ourselves having to do that,
  // it raises the question of what we should really consider to count
  // as a reset pulse.  I don't have a good sense for how common noise
  // of sufficient power to cause false pulse ends is in practice.  Also,
  // while studying the behavior of real Maxim slaves, I found cases where
  // they considered *any* low pulse (even the shortest I could generate
  // with function-call-free hard-wired code) to count as a low pulse,
  // despite the spec for masters calling for some specific (small) number
  // of us of delay.  In other words, the official slaves didn't appear to
  // be filtering anything in terms of false low pulses, which should be
  // less firmly pulled low than the line is when we're pulling it low.
  // This in turn suggests that spurious pull-up/pull-down events aren't
  // much of a problem in practice, which probably means that the pulse
  // lengh measurement strategy being used here is fine.

  if ( event != ENY ) {
    peh ();
    printf ("event: %hhu\n", event);
    assert (0);
  }

  // FIXME: if data_bit was guaranteed to be zero or one, and we required
  // SW0B and SW1B to have values 0 and 1 respectively, we could save some
  // branch here.
  if ( ! data_bit ) {
    SET_STATE (SW0B);
  }
  else {
    SET_STATE (SW1B);
  }

  PFP_ASSERT (event == ENY);

  uint8_t le = wait_for_event ();   // Latest Event

  switch ( le ) {
    case ESW0:
      _delay_us (ST_SLAVE_WRITE_ZERO_LINE_HOLD_TIME);
      SET_STATE (SATRL);
      RELEASE_LINE ();
      le = wait_for_event ();
      if ( le == EGR ) {
        ows_error_t err = ows_handle_reset_pulse ();
        if ( err == OWS_ERROR_NONE ) {
          return OWS_ERROR_RESET_DETECTED_AND_HANDLED;
        }
      }
      DBL_ASSERT (le == ELR);
      break;
    case ESW1:
      break;  // Nothing to do: the rest of the time slot is empty
    case EGR:
      {
        ows_error_t err = ows_handle_reset_pulse ();
        if ( err == OWS_ERROR_NONE ) {
          return OWS_ERROR_RESET_DETECTED_AND_HANDLED;
        }
        else {
          return err;   // FIXME: make sure propagate makes sense here
        }
        break;
      }
    case EGUPL:
      return OWS_ERROR_UNEXPECTED_PULSE_LENGTH;
      break;
    default:
      peh ();
      PFP_ASSERT_NOT_REACHED ();
      break;
  }

  return OWS_ERROR_NONE;
}

ows_error_t
ows_read_bit (uint8_t *data_bit_ptr)
{
  SET_STATE (SRB);

  uint8_t le = wait_for_event ();   // Latest Event

  //if ( le == ELR ) { PFP_ASSERT_NOT_REACHED (); }

  //PFP_ASSERT_NOT_REACHED ();

  switch ( le ) {
    case ER0:
      *data_bit_ptr = 0;
      break;
    case ER1:
      *data_bit_ptr = 1;
      break;
    case EGR:
      {
        ows_error_t err = ows_handle_reset_pulse ();
        if ( err == OWS_ERROR_NONE ) {
          return OWS_ERROR_RESET_DETECTED_AND_HANDLED;
        }
        break;
      }
    case EGUPL:
      return OWS_ERROR_UNEXPECTED_PULSE_LENGTH;
      break;
    default:
      peh ();
      assert (FALSE);   // Shouldnt be here FIXME: correct?
      break;
  }

  return OWS_ERROR_NONE;
}

ows_error_t
ows_write_byte (uint8_t data_byte)
{
  // FIXME: remove debug
  //printf ("byte to write: %lu\n", (long unsigned) data_byte);
  //assert (0);

  // Loop to write each bit in the byte, LS bit first
  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ )
  {
    CPE (ows_write_bit (data_byte & B00000001));
    data_byte >>= 1;   // Shift to get to next bit
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
ows_write_id (void)
{
  for ( uint8_t ii = 0 ; ii < OWC_ID_SIZE_BYTES ; ii++ ) {
    CPE (ows_write_byte (rom_id[ii]));
  }

  return OWS_ERROR_NONE;
}

// Evaluate to the value of Bit Number bn (0-indexed) of rom_id.
#define ROM_ID_BIT(bn) \
  (((rom_id[bn / BITS_PER_BYTE]) >> (bn % BITS_PER_BYTE)) & B00000001)

ows_error_t
ows_answer_search (void)
{
  for ( uint8_t ii = 0 ; ii < OWC_ID_SIZE_BYTES * BITS_PER_BYTE ; ii++ ) {
    uint8_t bv = ROM_ID_BIT (ii);   // Bit Value
    CPE (ows_write_bit (bv));
    CPE (ows_write_bit (! bv));
    uint8_t mbv;
    CPE (ows_read_bit (&mbv));
    // This is actually reasonably likely, but if its true we have lots
    // of time, whereas if its not we have to keep up with the master for
    // potentially all the remaining bits in the ID, so we want that path
    // to be fast.
    if ( UNLIKELY (bv != mbv) ) {
      // Note that we don't return OWS_ERROR_ROM_ID_MISMATCH here.  This is
      // a bit of a judgement call, but since this function is supposed to
      // be used to respond to a SEARCH_ROM command, and its not really
      // an error when we drop out of such a search, and clients of this
      // interface shouldn't have to care one way or the other, we don't
      // report mismatches as errors.
      return OWS_ERROR_NONE;
    }
  }

  return OWS_ERROR_NONE;
}

uint8_t ows_alarm = 0;

ows_error_t
ows_read_and_match_id (void)
{
  for ( uint8_t ii = 0 ; ii < OWC_ID_SIZE_BYTES ; ii++ ) {
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
