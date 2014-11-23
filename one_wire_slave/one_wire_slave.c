// Implementation of the interface described in one_wire_master.h.

#include <stdlib.h>
#include <string.h>
#include <util/crc16.h>
#include <util/delay.h>

#include "dio.h"
#include "one_wire_slave.h"
#include "timer1_stopwatch.h"
#include "util.h"

#ifndef OWS_PIN
#  error OWS_PIN not defined (it must be explicitly set to one of \
         the DIO_PIN_* tuple macros before this header is included)
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
  RELEASE_LINE ();
}

// The following ST_* (Slave Timing) macros contain timing values that actual
// Maxim DS18B20 devices have been found to use; see one_wire_slave.c.probe
// from the one_wire_master module for the source of these values.

// DS18B20 datasheet and AN126 both say masters are supposed to send 480
// us pulse minimum.
#define ST_RESET_PULSE_LENGTH_REQUIRED 231.0

// DS18B20 datasheet says 15 to 60 us.
#define ST_DELAY_BEFORE_PRESENCE_PULSE 29.0

// DS18B20 datasheet says 60 to 240 us.   FIXME: do
// other 1-wire datasheets give the same timing numbers?  the DS18B20 is
// somewhat old, maybe they've sorted out new better numbers since then.
#define ST_PRESENCE_PULSE_LENGTH 115.0

// DS18B20 datasheet says at least 1 us required from master, but the actual
// DS18B20 devices seem even the shortest blip as signaling the start of
// a slot.  So this one-cycle time is sort of a joke, and in fact its best
// to not wait at all so we don't have to worry about the actual timer delay.
#define ST_REQUIRED_READ_SLOT_START_LENGTH (1.0 / 16)

// Convenience macros
#define T1RESET() TIMER1_STOPWATCH_RESET ()
#define T1US() ((double) TIMER1_STOPWATCH_MICROSECONDS ())

// Readability macros
#define LINE_IS_HIGH() (SAMPLE_LINE ())
#define LINE_IS_LOW() (! SAMPLE_LINE ())

void
ows_wait_for_reset (void)
{
  do {
    while ( LINE_IS_HIGH () ) { ; }
    T1RESET ();
    while ( LINE_IS_LOW () ) { ; }
  } while ( T1US () < ST_RESET_PULSE_LENGTH_REQUIRED );
}



// Wait for a reset, and signal our presence when we receive one.
void
ows_wait_then_signal_presence (void)
{
  ows_wait_for_reset ();
  _delay_us (ST_DELAY_BEFORE_PRESENCE_PULSE);
  DRIVE_LINE_LOW ();
  _delay_us (ST_PRESENCE_PULSE_LENGTH);
  RELEASE_LINE ();
}

// FIXME: not sure whether we need event system or can just check for long
// reset pulses everywhere that we're getting pulses.
/*
typedef enum {
  OWS_EVENT_RESET_PULSE,
  OWS_EVENT_FALLING_EDGE,
} ows_event_t;

ows_event_t
ows_wait_for_event (uint8_t line_is_high)
{
  if ( line_is_high ) {
    T1RESET ();
    while ( LINE_IS_HIGH () ) { ; }
    return OWS_EVENT_FALLING_EDGE;
  }
  else {
    while ( LINE_IS_LOW () ) { ; }
    return OWS_EVENT_RISING_EDGE;
  }
}
*/

/*
static uint8_t volatile owsps = HIGH;   // One-Wire Slave Pin State

ISR (OWS_PIN_CHANGE_INTERRUPT_VECT)
{
  owsps = SAMPLE_LINE ();
}
*/

// FIXME: WORK POINT: well I think the next func would work for monitoring
// the length of low pulses in a procedural sort of way, but I dunno about
// that approach generally.  The trouble is I'm not sure I see a way to
// do it without forcing clients to keep track of time themselves, which
// is undesirable.  I guess you could have every call into the API check
// for a reset pulse and return a sentinel if it happens, but boy is it
// ugly and painful to keep track of the timing.  I think we need to read
// how the search algorithm works before deciding what kind of event loop or
// whatever is best to use on the slave.  I think slaves need a general even
// system for sure though, since for example at the moment our master-side
// test ends up sending two resets in a row, which this slave doesn't end
// up honoring.  The slave should generate events when the bit read gets
// done, but keep tracking the cumulative low time and eventually generate
// a reset even when that becomes appropriate... as long as it hasn't been
// instructed not to do so by a search command or something...

static void
wait_watch_for_reset (double ttw, double *alt)
{
  // Busy wait for Time To Wait us, while keeping track of any Accumulated
  // Low Time on the line.  Note that if the line is known to be high when
  // this function is call, *alt should be zero.

  double lpst = -1.0;   // Low Pulse Start Time
  T1RESET ();
  uint8_t ols = HIGH;   // Old Line State
  while ( T1US () < ttw ) {
    uint8_t nls = SAMPLE_LINE ();
    if ( nls == HIGH ) {
      *alt = 0.0;
      lpst = -1.0;
    }
    else if ( nls == LOW && ols == HIGH ) {
      lpst = T1US ();
    }
    nls = ols;
  }

  if ( lpst > 0.0 ) {
    *alt += T1US ();
  }
}

uint8_t
ows_read_bit (void)
{
  // Slave Read Slot Duration
  double const srsd = 60.0;

  // Slave Read Slot Sample Time.  This is the time the DS18B20 diagram
  // indicates that it typically waits from the time the line is pulled low
  // to when it samples.
  double const srsst = 30.0;

  WAIT_FOR_LINE_LOW (BIG_WAIT);
  WAIT_FOR_LINE_HIGH (



  // You might think we would want to require the line to stay low for a
  // while, rather than going the first time we see anything.  But so far
  // as I can tell the DS18B20 doesn't do this for master read slots: it
  // considers one to have started whenever you blip the line low even for
  // one instruction.
  while ( LINE_IS_HIGH () ) {
    // We could use ST_REQUIRED_READ_SLOT_START_LENGTH here after we detect
    // a blip but it's pointless: see comments near that macro.
    ;
  }
  T1RESET ();

  double low_time = 0.0;
  wait_count_low_time ();

  while ( LINE_IS_LOW () ) {
    if ( T1US () >= srsst ) { result = 0; }
  }
  while ( T1US () < srsst ) { result = 1; }


  _delay_us (srsd - srsst);

  return result;
}

void
ows_write_bit (uint8_t value)
{
  // Figure 1 of Maxim Application Note AN 126 shows that the master should
  // start a read slot by pulling the line low for 6 us, then sample the
  // line after an addional 9 us.  The slave transmits a one by leaving the
  // bus high at that point, and a zero by pulling it low.  In either case,
  // the bus is supposed to be released again by the end of the time slot F
  // (55) us later.

  // Slave Write Slot Send Time.  This is the time that we wait from when
  // the master pulls the line low to indicate the start of a slave write
  // slot to when we set the line ourselves.  Since AN126 recommends that
  // the master hold the line low for 6 us, we wait a little more than that.
  // Of course Figure 16 of the DS18B20 datasheet indicates that the master
  // should keep TINIT "as short as possible".  I guess 6 us is as short as
  // possible, for some reason :)
  double const swsst = TICK_DELAY_A + 2.42;

  // Slave Write Line Hold Time.  Time to hold the line low when sending a
  // 0 to the master.  See Figure 1 of Maxim Application Note AN126.  We go
  // with TICK_DELAY_F / 2.0 here because it's, well, half way between when
  // we must have the line held low and when we must release it.
  double const swlht = (TICK_DELAY_E + TICK_DELAY_F / 2.0);

  while ( SAMPLE_LINE () ) {
    ;
  }

  _delay_us (swsst);

  if ( ! value ) {
    DRIVE_LINE_LOW ();
  }

  _delay_us (swlht);

  RELEASE_LINE ();
}

uint8_t
ows_read_byte (void)
{
  uint8_t result = 0;

  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ ) {
    result >>= 1;  // Shift the result to get ready for the next bit
    // If result is one, then set MS bit
    if ( ows_read_bit () ) {
      result |= B10000000;
    }
  }

  return result;
}

void
ows_write_byte (uint8_t data)
{
  // Loop to write each bit in the byte, LS-bit first
  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ )
  {
    ows_write_bit (data & B00000001);
    data >>= 1;   // Shift to get to next bit
  }
}

/*

uint8_t
owm_touch_reset (void)
{
  TICK_DELAY (TICK_DELAY_G);
  DRIVE_LINE_LOW ();
  TICK_DELAY (TICK_DELAY_H);
  RELEASE_LINE ();
  TICK_DELAY (TICK_DELAY_I);
  // Look for presence pulse from slave
  uint8_t result = ! SAMPLE_LINE ();
  TICK_DELAY (TICK_DELAY_J); // Complete the reset sequence recovery

  return result; // Return sample presence pulse result
}

void
owm_write_bit (uint8_t value)
{
  // Send a 1-Wire write bit. Provide 10us recovery time.

  if ( value ) {
    // Write '1' bit
    DRIVE_LINE_LOW ();
    TICK_DELAY (TICK_DELAY_A);
    RELEASE_LINE ();
    TICK_DELAY (TICK_DELAY_B); // Complete the time slot and 10us recovery
  }
  else {
    // Write '0' bit
    DRIVE_LINE_LOW ();
    TICK_DELAY (TICK_DELAY_C);
    RELEASE_LINE ();
    TICK_DELAY (TICK_DELAY_D);
  }
}

uint8_t
owm_read_bit (void)
{
  // Read a bit from the 1-Wire bus and return it. Provide 10us recovery time.

  DRIVE_LINE_LOW ();
  TICK_DELAY (TICK_DELAY_A);
  RELEASE_LINE ();
  TICK_DELAY (TICK_DELAY_E);
  uint8_t result = SAMPLE_LINE ();   // Sample bit value from slave
  TICK_DELAY (TICK_DELAY_F); // Complete the time slot and 10us recovery

  return result;
}

uint8_t
owm_read_id (uint8_t *id_buf)
{
  uint8_t slave_presence = owm_touch_reset ();
  if ( ! slave_presence ) {
    return FALSE;
  }

  uint8_t const read_rom_command = OWM_READ_ROM_COMMAND;
  owm_write_byte (read_rom_command);
  for ( uint8_t ii = 0 ; ii < OWM_ID_BYTE_COUNT ; ii++ ) {
    id_buf[ii] = owm_read_byte ();
  }

  return TRUE;
}

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
