
#include <util/delay.h>

#include "dio.h"
#include "one_wire_master.h"

///////////////////////////////////////////////////////////////////////////////
//
// Line Drive and Sample Functions
//
// These routines correspond to the uses of the inp and outp functions of
// Maxim application note AN126.  These work on a per-instance basis.
//

static void
one_wire_release_line (OneWireMaster *owi)
{
  owi = owi;
}

static void
one_wire_drive_line_low (OneWireMaster *owi)
{
  owi = owi;
}

static uint8_t
one_wire_sample_line (OneWireMaster *owi)
{
  owi = owi;

  return 42;   // FIXME: fill in
}

// Tick delays for various parts of the standard speed one-wire protocol,
// as described in Table 2 in Maxim application note AN126.
#define TICK_DELAY_A   6
#define TICK_DELAY_B  64
#define TICK_DELAY_C  60
#define TICK_DELAY_D  10
#define TICK_DELAY_E   9
#define TICK_DELAY_F  55
#define TICK_DELAY_G   0
#define TICK_DELAY_H 480
#define TICK_DELAY_I  70
#define TICK_DELAY_J 410

// Implementation of the delay function described in Maxim application
// note AN126.  Pause for exactly 'ticks' number of 0.25 us ticks.
static void
tickDelay (int ticks)
{
  float const tick_time_in_us = 0.25;
  _delay_us (tick_time_in_us * ticks);
} // Implementation is platform specific

//---------------------------------------------------------------------------
// Generate a 1-Wire reset, return 1 if no presence detect was found,
// return 0 otherwise.
// (NOTE: Does not handle alarm presence from DS2404/DS1994)
//
int
owm_touch_reset (OneWireMaster *owm)
{
  int result;
  tickDelay (TICK_DELAY_G);
  one_wire_drive_line_low (owm);
  tickDelay (TICK_DELAY_H);
  one_wire_release_line (owm);
  tickDelay (TICK_DELAY_I);
  // Look for presence pulse from slave
  result = one_wire_sample_line (owm);

  tickDelay (TICK_DELAY_J); // Complete the reset sequence recovery
  return result; // Return sample presence pulse result
}

//---------------------------------------------------------------------------
// Send a 1-Wire write bit. Provide 10us recovery time.
//
static void
OWWriteBit (OneWireMaster *owm, int bit)
{
  if (bit)
  {
    // Write '1' bit
    one_wire_drive_line_low (owm);
    tickDelay (TICK_DELAY_A);
    one_wire_release_line (owm);
    tickDelay (TICK_DELAY_B); // Complete the time slot and 10us recovery
  }
  else
  {
    // Write '0' bit
    one_wire_drive_line_low (owm);
    tickDelay (TICK_DELAY_C);
    one_wire_release_line (owm);
    tickDelay (TICK_DELAY_D);
  }
}

//---------------------------------------------------------------------------
// Read a bit from the 1-Wire bus and return it. Provide 10us recovery time.
//
static int
OWReadBit (OneWireMaster *owm)
{
  int result;
  one_wire_drive_line_low (owm);
  tickDelay(TICK_DELAY_A);
  one_wire_release_line (owm);
  tickDelay(TICK_DELAY_E);
  result = one_wire_sample_line (owm);   // Sample the bit value from the slave
  tickDelay(TICK_DELAY_F); // Complete the time slot and 10us recovery
  return result;
}

//---------------------------------------------------------------------------
// Write 1-Wire data byte
//
void
owm_write_byte (OneWireMaster *owm, int data)
{
  int loop;
  // Loop to write each bit in the byte, LS-bit first
  for (loop = 0; loop < 8; loop++)
  {
    OWWriteBit (owm, data & 0x01);
    // shift the data byte for the next bit
    data >>= 1;
  }
}

//---------------------------------------------------------------------------
// Read 1-Wire data byte and return it
//
int owm_read_byte (OneWireMaster *owm)
{
  int loop, result=0;
  for (loop = 0; loop < 8; loop++)
  {
    // shift the result to get it ready for the next bit
    result >>= 1;
    // if result is one, then set MS bit
    if ( OWReadBit (owm) ) {
      result |= 0x80;
    }
  }
  return result;
}

//---------------------------------------------------------------------------
// Write a 1-Wire data byte and return the sampled result.
//
int
owm_touch_byte (OneWireMaster *owm, int data)
{
  int loop, result=0;
  for (loop = 0; loop < 8; loop++)
  {
    // shift the result to get it ready for the next bit
    result >>= 1;
    // If sending a '1' then read a bit else write a '0'
    if ( data & 0x01 ) {
      if ( OWReadBit (owm) ) {
        result |= 0x80;
      }
    }
    else {
      OWWriteBit (owm, 0);
    }
    // shift the data byte for the next bit
    data >>= 1;
  }
  return result;
}
