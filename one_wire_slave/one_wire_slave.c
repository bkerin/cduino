// Implementation of the interface described in one_wire_master.h.

#include <stdlib.h>
#include <string.h>
#include <util/crc16.h>
#include <util/delay.h>

#include "dio.h"
#include "one_wire_slave.h"
#include "timer1_stopwatch.h"
// FIXME: next two lines for debug only:
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

#ifndef OWS_PIN
#  error OWS_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this point)
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

// WARNING: the argument to this macro must be a double expression that the
// compiler knows is constant at compile time.  Pause for exactly ticks ticks.
#define TICK_DELAY(ticks) _delay_us (TICK_TIME_IN_US * ticks)

///////////////////////////////////////////////////////////////////////////////

// Tick delays for various parts of the one-wire protocol, as described in
// Table 2 in Maxim application note AN126.
#define TICK_DELAY_A   6.0
#define TICK_DELAY_B  64.0
#define TICK_DELAY_C  60.0
#define TICK_DELAY_D  10.0
#define TICK_DELAY_E   9.0
#define TICK_DELAY_F  55.0
#define TICK_DELAY_G   0.0
#define TICK_DELAY_H 480.0
#define TICK_DELAY_I  70.0
#define TICK_DELAY_J 410.0

void
ows_init (uint8_t load_eeprom_id)
{
  load_eeprom_id = load_eeprom_id;   // FIXME: out until we use IDs

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

// The DS18B20 datasheet and AN126 both say masters are supposed to send
// 480 us pulse minimum.
#define ST_RESET_PULSE_LENGTH_REQUIRED 231.0

// The DS18B20 datasheet says 15 to 60 us.
#define ST_DELAY_BEFORE_PRESENCE_PULSE 29.0

// The DS18B20 datasheet says 60 to 240 us.  FIXME: do other 1-wire datasheets
// give the same timing numbers?  the DS18B20 is somewhat old, maybe they've
// sorted out new better numbers since then.
#define ST_PRESENCE_PULSE_LENGTH 115.0

// The DS18B20 datasheet says at least 1 us required from master, but the
// actual DS18B20 devices seem even the shortest blip as signaling the start
// of a slot.  So this one-cycle time is sort of a joke, and in fact its best
// to not wait at all so we don't have to worry about the actual timer delay.
#define ST_REQUIRED_READ_SLOT_START_LENGTH (1.0 / 16)

// The total lenght of a slade read slot isn't supposed to be any longer
// than this.
#define ST_SLAVE_READ_SLOT_DURATION 60.0

// This is the time the DS18B20 diagram indicates that it typically waits
// from the time the line is pulled low by the master to when it (the slave)
// samples.
#define ST_SLAVE_READ_SLOT_SAMPLE_TIME 30.0

// This is the time that we wait from when the master pulls the line low to
// indicate the start of a slave write slot to when we set the line ourselves.
// Since AN126 recommends that the master hold the line low for 6 us, we wait
// a little more than that.  Of course Figure 16 of the DS18B20 datasheet
// indicates that the master should keep TINIT "as short as possible".
// I guess 6 us is as short as possible, for some reason :)
#define ST_SLAVE_WRITE_SLOT_SEND_TIME (TICK_DELAY_A + 2.42)

// This is the time to hold the line low when sending a 0 to the master.
// See Figure 1 of Maxim Application Note AN126.  We go with TICK_DELAY_F /
// 2.0 here because it's, well, half way between when we must have the line
// held low and when we must release it.  We could probably measure what
// actual slaves do if necessary...
#define ST_SLAVE_WRITE_ZERO_LINE_HOLD_TIME (TICK_DELAY_E + TICK_DELAY_F / 2.0)

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
#define T1US()    ((double) TIMER1_STOPWATCH_MICROSECONDS ())
#define T1OF()    TIMER1_STOPWATCH_OVERFLOWED ()

// Atomic version of T1US().  We have to use an atomic block to access
// TCNT1 outside the ISR, so we have this macro that does that and sets
// the double variable name argument given it to the elapsed us.
#define AT1US(outvar)                                               \
  do {                                                              \
    uint16_t XxX_tt;                                                \
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE)                              \
    {                                                               \
      XxX_tt = TIMER1_STOPWATCH_TICKS ();                           \
    }                                                               \
    outvar = XxX_tt * TIMER1_STOPWATCH_MICROSECONDS_PER_TIMER_TICK; \
  } while (0)

// Readability macros
#define LINE_IS_HIGH() (  SAMPLE_LINE ())
#define LINE_IS_LOW()  (! SAMPLE_LINE ())

volatile uint8_t got_reset = FALSE;

// This ISR keeps track of the length of low pulses.  If we see a long enough
// one we consider that we've seen a reset and set a client-visible flag.
ISR (DIO_PIN_CHANGE_INTERRUPT_VECTOR (OWS_PIN))
{
  if ( LINE_IS_LOW () ) {
    T1RESET ();
  }
  else {
    // See comments near the definition of ST_LONGEST_SLAVE_LOW_PULSE
    if ( ( T1US ()
           > ST_RESET_PULSE_LENGTH_REQUIRED +
             ST_LONGEST_SLAVE_LOW_PULSE_LENGTH )
         ||
         T1OF () ) {
      got_reset = TRUE;
      // FIXME: should we reset here as well so can figure out when to start
      // presence pulse?  In which cast I guess we want to always T1RESET(),
      // and the LINE_IS_LOW() branch above is actually empty...
      T1RESET ();
    }
  }
}

/*
void
ows_wait_for_reset (void)
{
  do {
    while ( LINE_IS_HIGH () ) { ; }
    T1RESET ();
    while ( LINE_IS_LOW () ) { ; }
  } while ( T1US () < ST_RESET_PULSE_LENGTH_REQUIRED );
}
*/

// FIXME: should move elsewhere?  Or maybe change to version with the timing
// padding out front?
static void
ows_send_presence_pulse (void)
{
  // The ISR resets the timer for us at the end of the reset pulse, so we
  // can use it to ensure that we've delayed long enough before sending the
  // presence pulse.
  /*
  double tsrp;   // Time Since Reset Pulse

  do {
    AT1US (tsrp);
  } while ( tsrp < ST_DELAY_BEFORE_PRESENCE_PULSE );
  */

  DRIVE_LINE_LOW ();
  _delay_us (ST_PRESENCE_PULSE_LENGTH);
  RELEASE_LINE ();
}

// FIXME: comment me
#define OWS_PRESENCE_PULSE()              \
  do {                                    \
    DRIVE_LINE_LOW ();                    \
    _delay_us (ST_PRESENCE_PULSE_LENGTH); \
    RELEASE_LINE ();                      \
  } while ( 0 )

#define AT1TICKS(outvar)                  \
  do {                                    \
    ATOMIC_BLOCK (ATOMIC_RESTORESTATE)    \
    {                                     \
      outvar = TIMER1_STOPWATCH_TICKS (); \
    }                                     \
  } while ( 0 )

void
ows_wait_for_reset (void)
{
  while ( ! got_reset ) { ; }
  uint16_t at1_ticks;
  do {
    AT1TICKS (at1_ticks);
  } while ( at1_ticks < 7 );
  OWS_PRESENCE_PULSE ();
  got_reset = FALSE;
}

// Time between edge drop and read on master end is 70 us, so we need to
// determine what timing to use here.
//#define RESET_IS_RECENT(et) (et < 60.0)


// FIXME: hard-wired for our us/tick here, which depends on the
// timer1_stopwatch.h not being subject to tweaked prescaler values, and
// our FCPU being as expected.
#define RESET_IS_RECENT(eticks) (eticks < 15)

ows_error_t
ows_handle_any_reset (void)
{
  ows_error_t result;

  if ( got_reset ) {
    uint16_t at1_ticks;
    do {
      AT1TICKS (at1_ticks);
    } while ( at1_ticks < 7);
    if ( RESET_IS_RECENT (at1_ticks) ) {
      AT1TICKS (at1_ticks);
      ows_send_presence_pulse ();
      result = OWS_ERROR_RESET_DETECTED_AND_HANDLED;
    }
    else {
      PFP ("DEBUG: missed reset pulse\n");
      result = OWS_ERROR_MISSED_RESET_DETECTED;
    }
  }
  else {
    result = OWS_ERROR_NONE;
  }

  got_reset = FALSE;

  return result;
}

// Wait for a reset.  When we receive one, send a presence pulse and then
// clear got_reset.
void
ows_wait_then_signal_presence (void)
{
  ows_wait_for_reset ();
  _delay_us (ST_DELAY_BEFORE_PRESENCE_PULSE);
  DRIVE_LINE_LOW ();
  _delay_us (ST_PRESENCE_PULSE_LENGTH);
  RELEASE_LINE ();
  got_reset = FALSE;
}

/*
static uint8_t read_bits[16] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
static uint8_t crb = 0;
*/

ows_error_t
ows_read_bit (uint8_t *data_bit_ptr)
{
  // You might think we would want to require the line to stay low for a
  // while, rather than going the first time we see anything.  But so far
  // as I can tell the DS18B20 doesn't do this for master read slots: it
  // considers one to have started whenever you blip the line low even for
  // one instruction.
  while ( LINE_IS_HIGH () ) {
    // We could use ST_REQUIRED_READ_SLOT_START_LENGTH here after we detect
    // a blip but it's pointless: see comments near that macro.
    // FIXME: shouldn't we spend all our idle time polling got_reset, so we
    // can't get stuck forever in case of a horrible timing incident?
    ;
  }

  _delay_us (ST_SLAVE_READ_SLOT_SAMPLE_TIME);

  *data_bit_ptr = SAMPLE_LINE ();

  // FIXME: debug stuff
  /*
  read_bits[crb++] = *data_bit_ptr;
  if ( crb == 15 ) {
    PFP ("read bits: ");
    for ( uint8_t ii = 0 ; ii < 16 ; ii++ ) {
      PFP ("%i ", (int) (read_bits[ii]));
    }
  }
  */

  // FIXME: This is slightly suspect: it finishes out the time slot without
  // regard for the master going fast or slow, and the masteris supposed to
  // signal the next bit.  Perhaps we should call ourselves done as soon as we
  // find the line to be be high instead, that way if the master goes faster
  // we follow along?  I think its right actually, but we should check after
  // this time that the master has actually released the line ever somehow
  _delay_us (ST_SLAVE_READ_SLOT_DURATION - ST_SLAVE_READ_SLOT_SAMPLE_TIME);

  //while ( LINE_IS_LOW () ) {
  //  ;
  //}

  // Before we report that all is well, we check that the master did in fact
  // release the line at the end of the slot as expected.
  return (LINE_IS_HIGH () ? OWS_ERROR_NONE : OWS_ERROR_LINE_UNEXPECTEDLY_LOW);
}

ows_error_t
ows_write_bit (uint8_t data_bit)
{
  // Figure 1 of Maxim Application Note AN 126 shows that the master should
  // start a read slot by pulling the line low for 6 us, then sample the
  // line after an addional 9 us.  The slave transmits a one by leaving the
  // bus high at that point, and a zero by pulling it low.  In either case,
  // the bus is supposed to be released again by the end of the time slot F
  // (55) us later.

  while ( LINE_IS_HIGH () ) {
    ;
  }

  // FIXME: questionable delay, questionable policy of insisting that line
  // must be clear when we start to send...

  _delay_us (ST_SLAVE_WRITE_SLOT_SEND_TIME);

  if ( ! SAMPLE_LINE () ) {
    return OWS_ERROR_LINE_UNEXPECTEDLY_LOW;
  }

  if ( ! data_bit ) {
    DRIVE_LINE_LOW ();
  }

  _delay_us (ST_SLAVE_WRITE_ZERO_LINE_HOLD_TIME);

  RELEASE_LINE ();

  if ( ! SAMPLE_LINE () ) {
    return OWS_ERROR_LINE_UNEXPECTEDLY_LOW;
  }

  return OWS_ERROR_NONE;
}

ows_error_t
ows_read_byte (uint8_t *data_byte_ptr)
{
  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ ) {

    (*data_byte_ptr) >>= 1;  // Shift the result to get ready for the next bit
    uint8_t bit_value;
    ows_error_t err = ows_read_bit (&bit_value);
    if ( err ) { return err; }
    err = err;  // FIXME: handle errors here
    // If result is one, then set MS bit
    if ( bit_value ) {
      (*data_byte_ptr) |= B10000000;
    }

    // If we see a reset pulse, we just return immediately.  We can go a bit
    // slot without taking so much time that we might be ignoring a reset
    // pulse for too long, but perhaps not eight of them.  If we get a reset
    // pulse return early as promised.  FIXME: this can probably never happen
    // now that we propagate error right away when doin gows_read_bit()
    if ( got_reset ) {
      return OWS_ERROR_GOT_RESET_PULSE;
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
    ows_error_t err = ows_write_bit (data_byte & B00000001);
    if ( err ) {
      return err;
    }
    data_byte >>= 1;   // Shift to get to next bit
  }

  return OWS_ERROR_NONE;
}

/*

// Global search state
static uint8_t rom_id[OWM_ID_BYTE_COUNT];   // Current ROM device ID
static uint8_t last_discrep;                // Bit position of last discrepancy
static uint8_t last_family_discrep;
static uint8_t last_device_flag;            // True iff we got last slave
static uint8_t crc8;

// Length of slave ROM IDs, in bits
#define ID_BIT_COUNT 64

// This many bits of each slave ROM ID form a so-called family code.
#define FAMILY_ID_BIT_COUNT 8

// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE : device found, ROM number in rom_id buffer
//        FALSE : device not found, end of search
//
static uint8_t
search (void)
{
  uint8_t id_bit_number;
  uint8_t last_zero, rom_byte_number, search_result;
  uint8_t id_bit, cmp_id_bit;
  uint8_t rom_byte_mask, search_direction;

  // Initialize for search
  id_bit_number = 1;
  last_zero = 0;
  rom_byte_number = 0;
  rom_byte_mask = 1;
  search_result = FALSE;
  crc8 = 0;

  // If the last call was not the last one
  if ( !last_device_flag )
  {
    // 1-Wire reset
    if ( !owm_touch_reset () ) {
      // Reset the search
      last_discrep = 0;
      last_device_flag = FALSE;
      last_family_discrep = 0;
      return FALSE;
    }
    // Issue the search command
    owm_write_byte (OWM_SEARCH_ROM_COMMAND);   // Issue the search command
    // Loop to do the search
    do {
      // Read a bit and its complement
      id_bit = owm_read_bit ();
      cmp_id_bit = owm_read_bit ();
      // Check for no devices on 1-wire
      if ( (id_bit == 1) && (cmp_id_bit == 1) ) {
        break;
      }
      else {
        // All devices coupled have 0 or 1
        if ( id_bit != cmp_id_bit ) {
           search_direction = id_bit;   // Bit write value for search
        }
        else {
          // If this discrepancy is before the Last Discrepancy
          // on a previous next then pick the same as last time
          if ( id_bit_number < last_discrep ) {
            search_direction = ((rom_id[rom_byte_number] & rom_byte_mask) > 0);
          }
          else {
            // If equal to last pick 1, otherwire pick 0
            search_direction = (id_bit_number == last_discrep);
          }
          // If 0 was picked then record its position in LastZero
          if ( search_direction == 0 ) {
            last_zero = id_bit_number;
            // Check for Last discrepancy in family
            if ( last_zero <= FAMILY_ID_BIT_COUNT ) {
              last_family_discrep = last_zero;
            }
          }
        }
        // Set or clear the bit in the ROM byte rom_byte_number with mask
        // rom_byte_mask
        if (search_direction == 1) {
          rom_id[rom_byte_number] |= rom_byte_mask;
        }
        else {
          rom_id[rom_byte_number] &= ~rom_byte_mask;
        }
        // Serial number search direction write bit
        owm_write_bit (search_direction);
        // Increment the byte counter id_bit_number and shift the mask
        // rom_byte_mask
        id_bit_number++;
        rom_byte_mask <<= 1;
        // If the mask is 0 then go to new SerialNum byte rom_byte_number
        // and reset mask
        if (rom_byte_mask == 0) {
             // Incrementally update CRC
             crc8 = _crc_ibutton_update (crc8, rom_id[rom_byte_number]);
             rom_byte_number++;
             rom_byte_mask = 1;
        }
      }
    }
    while ( rom_byte_number < OWM_ID_BYTE_COUNT );

    // If the search was successful...
    if ( ! ((id_bit_number <= ID_BIT_COUNT) || (crc8 != 0)) ) {
      last_discrep = last_zero;
      // If this was the last device...
      if ( last_discrep == 0 ) {
         last_device_flag = TRUE;
      }
      search_result = TRUE;
    }
  }

  // If no device found, then reset counters so next 'search' will be like
  // a first
  if ( (! search_result) || (! (rom_id[0])) ) {
    last_discrep = 0;
    last_device_flag = FALSE;
    last_family_discrep = 0;
    search_result = FALSE;
  }

  return search_result;
}

static uint8_t
first (void)
{
  // Reset the search state
  last_discrep = 0;
  last_device_flag = FALSE;
  last_family_discrep = 0;

  return search();
}


// Find the 'next' devices on the 1-Wire bus
// Return TRUE : device found, ROM number in rom_id buffer
//        FALSE : device not found, end of search
//
static uint8_t
next (void)
{
  return search();
}

// Verify that the device with the ROM number in rom_id buffer is present.
// Return TRUE : device verified present
//        FALSE : device not present
//
static uint8_t
verify (void)
{
  unsigned char rom_backup[OWM_ID_BYTE_COUNT];
  uint8_t result;
  uint8_t ld_backup, ldf_backup, lfd_backup;

  // Keep a backup copy of the current state
  for ( uint8_t ii = 0 ; ii < OWM_ID_BYTE_COUNT ; ii++ ) {
     rom_backup[ii] = rom_id[ii];
  }
  ld_backup = last_discrep;
  ldf_backup = last_device_flag;
  lfd_backup = last_family_discrep;

  // Set globals st the next search will look for the device with id in rom_id
  last_discrep = ID_BIT_COUNT;
  last_device_flag = FALSE;

  if ( search() ) {
     // Check if same device found
     result = TRUE;
     for ( uint8_t ii = 0 ; ii < OWM_ID_BYTE_COUNT ; ii++)
     {
        if ( rom_backup[ii] != rom_id[ii] )
        {
            result = FALSE;
            break;
        }
     }
  }
  else {
    result = FALSE;
  }

  // Restore the search state
  for ( uint8_t ii = 0 ; ii < OWM_ID_BYTE_COUNT ; ii++ ) {
     rom_id[ii] = rom_backup[ii];
  }
  last_discrep = ld_backup;
  last_device_flag = ldf_backup;
  last_family_discrep = lfd_backup;

  return result;
}

uint8_t
owm_first (uint8_t *id_buf)
{
  uint8_t result = first ();

  if ( result ) {
    memcpy (id_buf, rom_id, OWM_ID_BYTE_COUNT);
  }

  return result;
}

uint8_t
owm_next (uint8_t *id_buf)
{
  uint8_t result = next ();
  if ( result ) {
    memcpy (id_buf, rom_id, OWM_ID_BYTE_COUNT);
  }

  return result;
}

uint8_t
owm_verify (uint8_t *id_buf)
{
  memcpy (rom_id, id_buf, OWM_ID_BYTE_COUNT);
  uint8_t result = verify ();
  return result;
}

void
owm_write_byte (uint8_t data)
{
  // Loop to write each bit in the byte, LS-bit first
  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ )
  {
    owm_write_bit (data & B00000001);
    data >>= 1;   // Shift to get to next bit
  }
}

// Like the other functions, these come from Maxim Application Note AN187.
// But they didn't seem to work right for me, I think just because their
// interaction with owm_first()/owm_next() is weird.  And they seem sort
// of pointless: surely clients can just remember things by family for
// themsleves after the initial scan if the need to?  I guess it could
// make things a tiny bit faster in the presence of hot-plug devices or
// something but I have difficulty imagining caring.
//void
//owm_target_setup (uint8_t family_code)
//{
//  rom_id[0] = family_code;
//  for ( uint8_t ii = 1; ii < FAMILY_ID_BIT_COUNT ; ii++ ) {
//    rom_id[ii] = 0;
//  }
//  last_discrep = ID_BIT_COUNT;
//  last_family_discrep = 0;
//  last_device_flag = FALSE;
//}
//
//void
//owm_skip_setup (void)
//{
//  last_discrep = last_family_discrep;
//  last_family_discrep = 0;
//
//  // If there are no devices or other families left...
//  if ( last_discrep == 0 ) {
//     last_device_flag = TRUE;
//  }
//}

uint8_t
owm_read_byte (void)
{
  uint8_t result = 0;
  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ ) {
    result >>= 1;  // Shift the result to get ready for the next bit
    // If result is one, then set MS bit
    if ( owm_read_bit () ) {
      result |= B10000000;
    }
  }

  return result;
}

uint8_t
owm_touch_byte (uint8_t data)
{
  uint8_t result = 0;
  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ ) {
    // Shift the result to get it ready for the next bit
    result >>= 1;
    // If sending a '1' then read a bit, otherwise write a '0'
    if ( data & B00000001 ) {
      if ( owm_read_bit () ) {
        result |= B10000000;
      }
    }
    else {
      owm_write_bit (0);
    }
    // Shift the data byte for the next bit
    data >>= 1;
  }
  return result;
}

*/
