// Implementation of the interface described in one_wire_slave.h.

#include <assert.h>
#include <avr/eeprom.h>
#include <avr/power.h>   // FIXME: only needed for devel with timer0
#include <stdint.h>
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
// FIXME: thi sisn't used anywhere in the new code, I think
#define T1TPUS \
  (CLOCK_CYCLES_PER_MICROSECOND () / TIMER1_STOPWATCH_PRESCALER_DIVIDER)

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

// FIXME: I'd like to NOT use UINT8_C everywhere actually.  Perhaps do
// the compiler research to ensure that we don't need to.
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

// More generally, any A-length pulse can probably be arbitrarily short,
// though I haven't checked if it works for writing a 1 from the master
// (FIXME: check that).
#define ST_REQUIRED_A_PULSE_LENGTH (1000000.0 / F_CPU)

// This is the time the DS18B20 diagram indicates that it typically waits
// from the time the line is pulled low by the master to when it (the slave)
// samples when reading a bit.
#define ST_SLAVE_READ_SLOT_SAMPLE_TIME 32

// This is the maximum pulse lengh we will tolerate when looking for the
// pulse the master is supposed to send to start a slave write slot.  We go
// with OWC_TICK_DELAY_E / 2 here because its half whay between what the
// master is supposed to send and the point at which the master is supposed
// to sample the line, and also because the grace time is small enough that
// it won't cause the ST_SLAVE_WRITE_ZERO_TIME-length pulse we might send
// in response to crowd the end of the time slot at all.
#define ST_SLAVE_WRITE_SLOT_START_PULSE_MAX_LENGTH \
  (OWC_TICK_DELAY_A + OWC_TICK_DELAY_E / 2)

// This is the time to hold the line low when sending a 0 to the master.
// See Figure 1 of Maxim_Application_Note_AN126.pdf, page 2.  We subtrack
// E as well as D from F because for all we know we didn't actually get
// arount to taking the line low until E was mostly over.  FIXME: well,
// I guess what we should really be doing is starting to hold the line
// low as soon as we get the initial negative pulse edge when the master
// is goint to read.  But that will require yet another redo of our event
// system *sigh*, it might be fast enough without it.
#define ST_SLAVE_WRITE_ZERO_TIME \
  (OWC_TICK_DELAY_F - OWC_TICK_DELAY_E - OWC_TICK_DELAY_D)

// Thiis an old logic for this number, slightly wrong the fact that F
// overlapped D in the diagram but it worked.  We go with OWC_TICK_DELAY_F /
// 2 here because it's, well, half way between when we must have the line
// held low and when we must release it.  We could probably measure what
// actual slaves do if necessary.  Waiting F/2 has the disadvantage of
// pushing the total low time closer to the length of a presence pulse,
// though its still 20+ ms short of the minimum required time so it should
// be ok, and it gives the master more time to get around to reading the
// line compared to the alternative below.  This makes it slightly harder
// for our code to distinguish the EVZP and EVC events.  FIXME: actually,
// the diagram in AN126 sugggest we
//#define ST_SLAVE_WRITE_ZERO_TIME (OWC_TICK_DELAY_E + OWC_TICK_DELAY_F / 2)

// This also worked super dependably, which isn't too surprising: given the
// timing they both should be fine.  This one gives a margin of E on both
// sides of the anticipated sample point, which makes a lot of sense also
// (compared to the F/2 that we use above).  This keeps the total zero pulse
// length farther from that of a presense pulse, but gives the master less
// time to get around to reading the zero pulse.  It also makes the EVZP
// event easier to distinguish from the EVC event.
//#define ST_SLAVE_WRITE_ZERO_TIME (OWC_TICK_DELAY_E * 2)

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

#define ENY   0   // Event Nothing Yet
#define ER0   1   // Event Read 0
#define ER1   2   // Event Read 1
#define ESW0  3   // Event Started Writing 0
#define ESW1  4   // Event Started Writing 1
#define ELR   5   // Event Line Released
#define EGR   6   // Event Got Reset
#define EGUPL 7   // Event Got Unexpected Pulse Length

// We grade (negative) pulses that we observe in the pin change ISR by lenght
// and call them events.  This is convenient because it lets us test them
// atomically, while the actualy 16-bit timer value would require atomic
// blocks everywhere.
#define EVNY 0   // Event Nothing Yet
#define EVA  1   // Event A-length pulse
#define EVC  2   // Event C-length pulse
#define EVZP 3   // Event Zero Pulse-length pulse (probably we sent it)
#define EVR  4   // Event Reset-length pulse (we don't require a full 480 us)
#define EVPP 5   // Event Presence pulse-length pulse (probably we sent it)
#define EVWP 6   // Event Weird-length pulse
#define ETO  7   // Event TimeOut (this one is detected in a different ISR)
// FIXME: initialize elsewhere if at all
volatile uint8_t event = EVNY;

// FIXME: the globals should probably be re-initialized by the _init routine

uint8_t wfec = 0;
uint8_t eh[UINT8_MAX] = { 0 };

static void
peh (uint8_t mec)
{
  printf ("wfec: %lu\n", (unsigned long) wfec);
  for ( uint8_t ii = 0 ; ii < (mec > 0 ? mec : wfec) ; ii++ ) {
    printf ("eh[%lu]: %lu\n", (unsigned long) ii, (unsigned long) eh[ii]);
  }
}

#define ARM_TIMEOUT_ISR()    do { TIMSK1 |= _BV (OCIE1A);    } while ( 0 )
#define DISARM_TIMEOUT_ISR() do { TIMSK1 &= ~(_BV (OCIE1A)); } while ( 0 )

// FIXME: review globals for use
uint8_t cbit;    // Current Bit
uint8_t cbyte;   // Current Byte
uint8_t cbi;     // Current Bit Index (bit offset in cbyte)

// FIXME: well maybe we don't need an actual ISR for this.  We can just check
// for the compare match flag while waiting for an event Would make waiting
// for event a tiny bit slower every time, but shouldn't be any slower worst
// case than when the timeout ISR fires, and we wouldn't have to twiddle
// the ISR enable everywhere (no more ARM/DISARM).  We only needed OCR1A
// to avoid missing the case where the timer ran past the exact value of
// interest between checks, and since the flag records the fact I don't
// see any need for the interrupt.  Flag would need manual clearing of course.

// This ISR exists only to keep track of timeout events.  This ISR is
// supposed to get armed immediately before we start waiting for something
// to happen on the 1-wire bus.
ISR (TIMER1_COMPA_vect)
{
  PFP_ASSERT_NOT_REACHED ();   // ISRs are out

  // We only want to queue a timeout event if we aren't already in the proc

  // This ISR is disarmed and the corresponding interrupt flag cleared in
  // the pulse-timing pin change ISR when it records a pulse end.  Therefore,
  // ths ISR should only fire when no other even has been set yet.  Therefore,
  // we don't need to test even before setting it.
  // FIXME: therefore, remove these again after things are working and verify
  // no breakage.
  if ( event == EVNY ) {
    event = ETO;
  }

  // For debugging purposes, we collapse sequences of many timeouts into
  // just one event report.
  if ( eh[wfec - 1] != ETO ) {
    eh[wfec++] = event;   // FIXME: this is debug
  }

  // This is a one-shot ISR that we reset when we start a new timeout counter,
  // so we reset it here.  Note that we don't have to clear the output compare
  // interrupt flag itself, since it will happen automagically because we're
  // in the corresponding ISR.
  DISARM_TIMEOUT_ISR ();
}

// FIXME: I think OWS_NO_TIMEOUT should turn into OWS_TIMEOUT_NONE

// Timeout, in timer1 ticks.  FIXME: this should be reset in _init, and
// not initialized here ( to save a little flash).
uint16_t timeout_t1t = OWS_NO_TIMEOUT;

// This ISR keeps track of the length of low pulses.  When we see the end of
// one we consider that we've seen something interesting, and if a timeout
// event hasn't already occurred we record a pulse event.
ISR (DIO_PIN_CHANGE_INTERRUPT_VECTOR (OWS_PIN))
{
  PFP_ASSERT_NOT_REACHED ();   // ISRs are out

  if ( LINE_IS_HIGH () ) {

    // If we've already got an unhandled timeout event, we don't want to
    // stomp on it here.
    if ( event == ETO ) {
      return;
    }

    uint16_t pl;   // Pulse Length
    if ( LIKELY(! TIMER1_STOPWATCH_OVERFLOWED ()) ) {
      pl = TIMER1_STOPWATCH_TICKS ();
    }
    else {
      // If we've had overflow, its a really long pulse, so set to max.
      pl = UINT16_MAX;
    }

    // Now we grade the event by length.  We look for shorter pulses first,
    // to ensure that they are handled as fast as possible.

    // FIXME: probably we should just put the event size distinction points
    // midway between the values given in Maxim AN126.  Thus we might not
    // need the probed timings (ST_*) at all.  In fact the required reset
    // pulse length we discovered might be explained by the maxim slaves
    // using this policy.

    // Event ZP Timing Margin (in us).  See comments for ectm_us below.
    // The danger of a high margin here is that one of these could get
    // mistaken for an A pulse event.
    uint8_t const ezptm_us = 11;

    // Event C Timing Margin (in us).  Pulses measured as this much shorter
    // than OWC_TICK_DELAY_C will still be counted as C pulse events.
    // This choice of margin puts us about 10 us from the length of our
    // own zero pulse.  FIXME: in reality distinguishing C pulses and zero
    // pulses is probably pointless and problematic and we should just not
    // do it, but instead respond to an event that could be either in the
    // appropriate way given our state.
    uint8_t const ectm_us = 15;  // Timing Margin (in us)

    if ( LIKELY (pl < (ST_SLAVE_WRITE_ZERO_TIME - ezptm_us) * T1TPUS) ) {
      event = EVA;
    }

    else if ( pl < (OWC_TICK_DELAY_C - ectm_us) * T1TPUS ) {
      event = EVZP;
    }

    else if ( pl < ST_PRESENCE_PULSE_LENGTH * T1TPUS ) {
      // FIXME: we could put a check for weirdly long C pulses here, and use
      // our EVWP event.  They probably mean something screwy is going on.
      // but they would slow things down and the timing after the last bit
      // in a mster write and before a following slave write is also tight
      // and this contributes.
      event = EVC;
    }

    else if ( pl < OUR_RESET_PULSE_LENGTH_REQUIRED * T1TPUS ) {
      event = EVPP;
    }

    else {
      event = EVR;
    }

    // FIXME: : well, if this next line is in, we get wrong answers
    // at 16MHz even.  At 8 MHz, even if it isn't, we see all f's on the
    // master, indicating that we're never fast enough to send a zero.
    // I think the thing to do is rig the test slave up for feedback via
    // the XBee and write a faster state machine :)
    //eh[wfec++] = event;   // FIXME: this is debug
    DISARM_TIMEOUT_ISR ();
    // Clear any timeout interrupt that might have occurred during
    // this ISR.  Note that writing a logic one to TOV1 actually
    // *clears* it, and we don't have to use a read-modify-write
    // cycle to write the one (i.e. no "|=" required).  See
    // http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_intbits.
    TIFR1 = _BV (OCF1A);

  }
  else {

    // Note that this won't cause an immediate compare match even if
    // timeout_t1t is OWS_NO_TIMEOUT (which is 0), because writing TCNT1
    // (via TIMER1_STOPWATCH_RESET() blocks any compare match that occurs
    // in the next timer clock cycle (see atmega datasheet section 15.7.2
    // (FIXME: make this a ref and link the processor datasheet).
    OCR1A = timeout_t1t;
    TIMER1_STOPWATCH_RESET ();

  }
}

static uint8_t
wait_for_event (void)
{
  uint8_t ne = EVNY;
  while ( ne == EVNY ) {
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
    {
      if ( event != EVNY ) {
        //printf ("event: %lu\n", (long unsigned) event);
        ne = event;
        event = EVNY;
        //printf ("ne: %lu\n", (long unsigned) ne);
      }
    }
  }

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

void
ows_set_timeout (uint16_t time_us)
{
  // Insist on the interface requirements.
  if ( time_us != OWS_NO_TIMEOUT ) {
    assert (time_us >= OWS_MIN_TIMEOUT_US);
    assert (time_us <= OWS_MAX_TIMEOUT_US);
  }

  timeout_t1t = time_us * OWS_TIMER_TICKS_PER_US;
}


// Wait for the next event (low-high transition on the line or timeout).
// Note that event must be cleared by setting it to EVNY before this is used.
// The correct time to clear event is as soon as possible, which means as soon
// as the event type has been used to determine what we're going to do next.
// Unfortunately this rule can be a little confusing to interpret thanks to
// the goto and procedural macros we use, but with a little thought it's
// not hard to figure out when we're done with the information in event.
// The event variable effectively forms a queue of length one.  Events fill
// it, and as soon as we're decided how to react to an even we can empty it.
// There's no point in a longer queue, since we can't afford to fall behind
// anyway: some 1-wire events require an almost immediate reaction from
// the slave.
#define WAIT_FOR_EVENT()                   \
  do {                                     \
    if ( timeout_t1t != OWS_NO_TIMEOUT ) { \
      ATOMIC_BLOCK (ATOMIC_RESTORESTATE)   \
      {                                    \
        OCR1A = TCNT1 + timeout_t1t;       \
        ARM_TIMEOUT_ISR ();                \
      }                                    \
    }                                      \
    do {                                   \
      ;                                    \
    } while ( event == EVNY );             \
  } while ( 0 )

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

// Read a single bit into cbit.
#define READ_BIT()             \
  do {                         \
    WAIT_FOR_EVENT ();         \
    switch ( event ) {         \
      case EVA:                \
        event = ENVY;          \
        cbit = 1;              \
        break;                 \
      case EVC:                \
        event = EVNY;          \
        cbit = 0;              \
      default:                 \
        goto unexpected_event; \
    }                          \
  } while ( 0 )

// Read a single byte into cbyte.  Note that cbit is never set.
#define READ_BYTE()                         \
  do {                                      \
    cbyte = 0;                              \
    cbi = 0;                                \
    while ( cbi < BITS_PER_BYTE ) {         \
      WAIT_FOR_EVENT ();                    \
      switch ( event ) {                    \
        case EVA:                           \
          event = EVNY;                     \
          cbyte |= B00000001 << (cbi++);    \
          break;                            \
        case EVC:                           \
          event = EVNY;                     \
          cbyte &= ~(B00000001 << (cbi++)); \
          break;                            \
        default:                            \
          goto unexpected_event;            \
      }                                     \
    }                                       \
  } while ( 0 )

// FIXME: WRITE_BIT is a little bit untested at the moment
// Write a single bit with value bit_value.  Note that cbit is not use here.
#define WRITE_BIT(bit_value)                \
  do {                                      \
    WAIT_FOR_EVENT ();                      \
    if ( event != EVA ) {                   \
      goto unexpected_event;                \
    }                                       \
    event = EVNY;                           \
    if ( ! (bit_value) ) {                  \
      DRIVE_LINE_LOW ();                    \
      _delay_us (ST_SLAVE_WRITE_ZERO_TIME); \
      RELEASE_LINE ();                      \
      WAIT_FOR_EVENT ();                    \
      if ( event != EVZP ) {                \
        goto unexpected_event;              \
      }                                     \
      event = EVNY;                         \
    }                                       \
  } while ( 0 )

#define WRITE_BYTE()                              \
  do {                                            \
    cbi = 0;                                      \
    while ( cbi < BITS_PER_BYTE ) {               \
      WAIT_FOR_EVENT ();                          \
      if ( event != EVA ) {                       \
        goto unexpected_event;                    \
      }                                           \
      event = EVNY;                               \
      if ( ! (cbyte & (B00000001 << (cbi++))) ) { \
        DRIVE_LINE_LOW ();                        \
        _delay_us (ST_SLAVE_WRITE_ZERO_TIME);     \
        RELEASE_LINE ();                          \
        WAIT_FOR_EVENT ();                        \
        if ( event != EVZP ) {                    \
          goto unexpected_event;                  \
        }                                         \
        event = EVNY;                             \
      }                                           \
    }                                             \
  } while ( 0 )

// Write cbyte.
/*
#define WRITE_BYTE()                              \
  do {                                            \
    cbi = 0;                                      \
    while ( cbi < BITS_PER_BYTE ) {               \
      WRITE_BIT (cbyte & (B00000001 << (cbi++))); \
    }                                             \
  } while ( 0 )
  */



// Transaction State Waiting For (Reset Pulse|ROM Command|Function Command).
#define TSWFRP  0
#define TSWFRC  1
#define TSWFFC  2

///////////////////////////////////////////////////////////////////////////////
//
// Latest Version that doesn't use actual ISRs
//

// To get things to work at 8MHz I had to lock these variables into registers
// (see http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_regbind).
// It works without this trick at 16MHz, so I don't do it there, because
// it's assemblyish and weird, but it would probably make for (perhaps just
// slightly) better interrupt resistance at 16 MHz also.
#if F_CPU >= 16000000
uint8_t ls;       // Line State as of last reading, 1 or 0
uint8_t cbitv;    // Current Bit Value
uint8_t cbytev;   // Current Byte Value
uint8_t cbiti;    // Curren Bit Index (of cbytev)
#else
register uint8_t ls     asm("r2");   // Line State as of last reading, 1 or 0
register uint8_t cbitv  asm("r3");   // Current Bit Value
register uint8_t cbytev asm("r4");   // Current Byte Value
register uint8_t cbiti  asm("r5");   // Curren Bit Index (of cbytev)
#endif


uint16_t tr;       // Timer Reading (most recent)

// Read Bit Sample Time.  Time from negative edge to slave sample point
// when reading a bit, in microseconds.  We do like the master and give
// OWC_TICK_DELAY_E from the time when the master is supposed to have the
// line set.
#define RBST_US (OWC_TICK_DELAY_A + OWC_TICK_DELAY_E)

// Zero Pulse Time.  Here we do what Figure 1 of
// Maxim_Application_Note_AN126.pdf proscribes, even though it seems stupid.
// We might as well do it, since we can't change the fact that the master
// holds the line low almost to the end of the time slot when writing a
// zero anyway, giving us the same tight timing issue when transitioning
// between a master-write-0 and a master-read-0 that we get here between
// master-read-0 operations.
#define ZPT_US \
  (OWC_TICK_DELAY_A + OWC_TICK_DELAY_E + OWC_TICK_DELAY_F - OWC_TICK_DELAY_D)

// Clear Pin Change Flag and Reset Timer1
#define CPCFRT1()                                                             \
  do {                                                                       \
    DIO_CLEAR_PIN_CHANGE_INTERRUPT_FLAG (OWS_PIN);                           \
    TIMER1_STOPWATCH_FAST_RESET ();   /* do we want fast reset or normal? */ \
  } while ( 0 )

// FIXME: update for whatever value we settle on for timeout requirements,
// given that we ignore parts of pulses during certain operations.
#define CFR() (tr >= US2T1T (OUR_RESET_PULSE_LENGTH_REQUIRED))

// For tracking sample history
// FIXME: remove all the shist stuff sometime
/*
uint8_t shist[250] = {0};   // FIXME: im debug
uint8_t shp = 0;   // FIXME: im debug
static void
psh (void)
{
  for ( uint8_t ii = 0 ; ii < shp ; ii++ ) {
    printf ("shist[%hhu]: %hhu\n", ii, shist[ii]);
  }
}
*/

// FIXME: maybe this should just be a macro, the assembler does generate
// actual calls for it...
static ows_error_t
wfpcoto (void)
{
  while ( TRUE ) {
    if ( LIKELY (DIO_PIN_CHANGE_INTERRUPT_FLAG (OWS_PIN)) ) {
      TCNT0 = 0;   // FIXME: devel: measure time to second edge
      ls = SAMPLE_LINE ();
      //shist[shp++] = ls;
      // FIXME: perhaps we should detect resets right here, since clients
      // always have to do it I think.  Fctn name have to change again :)
      DIO_CLEAR_PIN_CHANGE_INTERRUPT_FLAG (OWS_PIN);
      tr = TIMER1_STOPWATCH_TICKS ();
      // FIXME: do we want fast reset or normal?  if fast we can't honor TOV1,
      // FIXME: currently we don't honor TOV1 anyway.
      TIMER1_STOPWATCH_FAST_RESET ();   // do we want fast reset or normal?
      return OWS_ERROR_NONE;
    }
    else if ( UNLIKELY (TCNT1 >= timeout_t1t) ) {
      if ( timeout_t1t != OWS_NO_TIMEOUT ) {
        return OWS_ERROR_TIMEOUT;
      }
    }
  }
}

uint8_t t0r;   // timer0 reading // FIXME: for debug

// Call call, Propagating Errors.  The call argument must be a call to a
// function returning ows_error_t.
#define CPE(call)                                 \
  do {                                            \
    ows_error_t XxX_err = call;                   \
    if ( UNLIKELY (XxX_err != OWS_ERROR_NONE) ) { \
      return XxX_err;                             \
    }                                             \
  } while ( 0 )

static ows_error_t
read_bit (void)
{
  while ( TRUE ) {
    CPE (wfpcoto ());
    if ( ls ) {
      if ( UNLIKELY (CFR ()) ) {
        return OWS_ERROR_GOT_RESET;
      }
    }
    else {
      //t0r = TCNT0;  // FIXME: im debug
      //printf ("t0r: %hhu\n", t0r); PHP ();

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
      _delay_us (RBST_US);
      cbitv = SAMPLE_LINE ();
      //shist[shp++] = cbitv;  // FIXME: im debug
      CPCFRT1 ();
      return OWS_ERROR_NONE;
    }
  }
}

static ows_error_t
write_bit (void)
{
  while ( TRUE ) {
    CPE (wfpcoto ());
    if ( UNLIKELY (ls) ) {
      if ( UNLIKELY (CFR ()) ) {
        return OWS_ERROR_GOT_RESET;
      }
    }
    else {
      if ( LIKELY (! cbitv) ) {
        DRIVE_LINE_LOW ();
        _delay_us (ZPT_US);
        RELEASE_LINE ();
        CPCFRT1 ();
      }
      return OWS_ERROR_NONE;
    }
  }
}


static ows_error_t
read_byte (void)
{
  cbytev = 0;
  for ( cbiti = 0 ; cbiti < BITS_PER_BYTE ; cbiti++ ) {
    CPE (read_bit ());
    cbytev |= (cbitv << cbiti);
  }

  return OWS_ERROR_NONE;
}

static ows_error_t
write_byte (void)
{
  for ( cbiti = 0 ; cbiti < BITS_PER_BYTE ; cbiti++ ) {
    cbitv = cbytev & (B00000001 << cbiti);
    CPE (write_bit ());
  }

  return OWS_ERROR_NONE;
}

// FIXME: this might be faster if it didn't take an arg and instead used
// the global, then it becomes a can of worms if we want to expose this
// function and require user to use the global, or make it a macro that
// does that implicitly, who knows.
ows_error_t
ows_new_write_bit (uint8_t bit_value)
{
  cbitv = bit_value;
  CPE (write_bit ());

  return OWS_ERROR_NONE;
}

ows_error_t
ows_new_read_bit (uint8_t *bit_value_ptr)
{
  CPE (read_bit ());
  *bit_value_ptr = cbitv;

  return OWS_ERROR_NONE;
}

ows_error_t
ows_new_write_byte (uint8_t byte_value)
{
  cbytev = byte_value;
  CPE (write_byte ());

  return OWS_ERROR_NONE;
}

ows_error_t
ows_new_read_byte (uint8_t *byte_value_ptr)
{
  CPE (read_byte ());
  *byte_value_ptr = cbytev;

  return OWS_ERROR_NONE;
}


///////////////////////////////////////////////////////////////////////////////

static ows_error_t
read_and_match_rom_id (void)
{
  for ( uint8_t ii = 0 ; ii < OWC_ID_SIZE_BYTES ; ii++ ) {
    for ( cbiti = 0 ; cbiti < BITS_PER_BYTE ; cbiti++ ) {
      CPE (read_bit ());
      if ( cbitv != ((rom_id[ii]) >> cbiti) ) {
        return OWS_ERROR_ROM_ID_MISMATCH;
      }
    }
  }

  return OWS_ERROR_NONE;
}

// Evaluate to the value of Bit Number bn (0-indexed) of rom_id.
#define ROM_ID_BIT(bn) \
  (((rom_id[bn / BITS_PER_BYTE]) >> (bn % BITS_PER_BYTE)) & B00000001)

static ows_error_t
answer_search (void)
{
  // ROM ID Size, in bits
  uint8_t const rids_bits = OWC_ID_SIZE_BYTES * BITS_PER_BYTE;
  // FIXME: use inner loop var
  for ( uint8_t ii = 0 ; ii < rids_bits ; ii++ ) {
    uint8_t cridbv = ROM_ID_BIT (ii);   // Current ROM ID Bit Value
    cbitv = cridbv;
    CPE (write_bit ());
    cbitv = ! cbitv;
    CPE (write_bit ());
    CPE (read_bit ());
    if ( UNLIKELY (cbitv != cridbv) ) {
      // Mismatches aren't error -- see comment above in this function..
      return OWS_ERROR_NONE;
    }
  }

  return OWS_ERROR_NONE;
}


static void
setup_timer0 (void)
{
  // FIXME: im a debugging function

  power_timer0_enable ();   // Ensure timer0 not shut down to save power
  TCCR0A = 0;
  TCCR0B = 0;

  // Clock / 8 for prescaler setting, giving two ticks per us at 16 MHz.
  TCCR0B &= ~(_BV (CS02) | _BV (CS00));
  TCCR0B |= _BV (CS01);
}

// FIXME: I think should add a compile-time option OWS_FILTER_LENGH or
// something that would treat sufficiently short pulses (less than, say,
// 1 us) as noise instead of registering them.  Or maybe just an option.
// The DS18B20 seems to treat even the shortest pulses that I can produce
// using an IO pin as real, but it might have some sort of filter short
// of that that comes into play.  Perhaps just using the edge detection is
// accomplishing something similar, but since well-behaved masters should
// never produce 1 us pulses anyway, I don't see why we shouldn't play it
// safe and filter them.

ows_error_t
ows_wait_for_function_transaction_2 (uint8_t *command_ptr, uint8_t jgur)
{
  *command_ptr = 42;   // FIXME: for devel compiler silencing

  // FIXME: devel block.  Verifies that timer0 measure time well
  setup_timer0 ();
  TCNT0 = 0;
  _delay_us (RBST_US);

  /*
  for ( uint8_t ii = 0 ; ii < 20 ; ii++ ) {
    shist[ii] = 42;
  }
  */

  uint8_t state = (jgur ? SPPP : SWFR);
  //uint8_t state = SWFR; jgur = jgur;  // FIXME:  unused debug

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
          switch ( cbytev ) {
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
              return OWS_ERROR_GOT_INVALID_ROM_COMMAND;
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
          cbytev = rom_id[ii];
          CPE (write_byte ());
        }
        state = SRFC;
        break;
      case SDMRC:
        {
          ows_error_t error = read_and_match_rom_id ();
          if ( error == OWS_ERROR_ROM_ID_MISMATCH ) {
            state = SWFR;
          }
          else {
            // Note that bother success and non-mismatch results end up here.
            return error;
          }
          break;
        }
      case SDASC:
        if ( UNLIKELY (! ows_alarm) ) {
          return OWS_ERROR_NONE;
        }
        CPE (answer_search ());
        state = SWFR;
        break;
      case SRFC:
        {
          CPE (read_byte ());
          *command_ptr = cbytev;
          return OWS_ERROR_NONE;
          break;
        }
      default:
        PFP_ASSERT_NOT_REACHED ();   // FIXME im debug change probly to assert0
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
          return OWS_ERROR_GOT_INVALID_ROM_COMMAND;
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
    _delay_us (ST_SLAVE_WRITE_ZERO_TIME); \
    RELEASE_LINE ();                                \
    wait_for_pulse_end ();                          \
  } while ( 0 )

ows_error_t
ows_write_bit (uint8_t data_bit)
{
  // FIXME: these are essentially gutted but here for reference and to maybe
  // fill in later.

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
    peh (0);
    printf ("event: %hhu\n", event);
    assert (0);
  }

  // FIXME: if data_bit was guaranteed to be zero or one, and we required
  // SW0B and SW1B to have values 0 and 1 respectively, we could save some
  // branch here.
  if ( ! data_bit ) {
    ;
  }
  else {
    ;
  }

  PFP_ASSERT (event == ENY);

  uint8_t le = wait_for_event ();   // Latest Event

  switch ( le ) {
    case ESW0:
      _delay_us (ST_SLAVE_WRITE_ZERO_TIME);
      RELEASE_LINE ();
      le = wait_for_event ();
      if ( le == EGR ) {
        ows_error_t err = ows_handle_reset_pulse ();
        if ( err == OWS_ERROR_NONE ) {
          return OWS_ERROR_RESET_DETECTED_AND_HANDLED;
        }
      }
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
      peh (0);
      PFP_ASSERT_NOT_REACHED ();
      break;
  }

  return OWS_ERROR_NONE;
}

ows_error_t
ows_read_bit (uint8_t *data_bit_ptr)
{
  // FIXME: maybe not in use now, maybe reactivate later

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
      peh (0);
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

ows_error_t
ows_answer_search (void)
{
  // FIXME: use cbiti instead of ii
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

