
#include <stdlib.h>
#include <util/delay.h>

#include "dio.h"
#include "one_wire_master.h"

///////////////////////////////////////////////////////////////////////////////
//
// Line Drive, Sample, and Delay Routines
//
// These macros correspond to the uses of the inp and outp and tickDelay
// functions of Maxim application note AN126.  Our versions work on a
// per-instance basis but are otherwise the same.  We use macros to avoid
// function call time overhead, which can be significant: Maxim application
// note AN148 states that the most common programming error in 1-wire
// programmin involves late sampling, which given that some samples occur
// after proscribed waits of only 9 us requires some care, especially at
// slower processor frequencies.
//

// Reinterpret One Wire Master Instance pointer owmi as a dio_pin_t Pointer.
#define OADP(owm) ((dio_pin_t *) owm)

// FIXME: if all this crap works CPDA should probably be provided in dio.h,
// together with the mention of the example here

// Canned Pin Description Arguments (just a convenience tuple)
#define CPDA(owm)                    \
  _SFR_IO8 ((OADP (owm))->dir_reg),  \
  (OADP (owm))->dir_bit,             \
  _SFR_IO8 ((OADP (owm))->port_reg), \
  (OADP (owm))->port_bit,            \
  _SFR_IO8 ((OADP (owm))->pin_reg),  \
  (OADP (owm))->pin_bit              \

// Release the line of the give OneWireMaster Instance.
#define RELEASE_LINE(owm) \
  DIO_INIT (              \
      CPDA(owm),          \
      DIO_INPUT,          \
      DIO_ENABLE_PULLUP,  \
      DIO_DONT_CARE )

static void
one_wire_release_line (OneWireMaster *owm)
{
  dio_pin_t *pin = (dio_pin_t *) owm;
  DIO_INIT_NA (
      _SFR_IO8 (pin->dir_reg),
      pin->dir_bit,
      _SFR_IO8 (pin->port_reg),
      pin->port_bit,
      _SFR_IO8 (pin->pin_reg),
      pin->pin_bit,
      DIO_INPUT,
      DIO_ENABLE_PULLUP,
      DIO_DONT_CARE );
}

// Drive the line of the given OneWireMaster Instance low.
#define DRIVE_LINE_LOW(owm) \
  DIO_INIT_NA (             \
      CPDA(owm),            \
      DIO_OUTPUT,           \
      DIO_DONT_CARE,        \
      LOW )

static void
one_wire_drive_line_low (OneWireMaster *owm)
{
  dio_pin_t *pin = (dio_pin_t *) owm;
  DIO_INIT_NA (
      _SFR_IO8 (pin->dir_reg),
      pin->dir_bit,
      _SFR_IO8 (pin->port_reg),
      pin->port_bit,
      _SFR_IO8 (pin->pin_reg),
      pin->pin_bit,
      DIO_OUTPUT,
      DIO_DONT_CARE,
      LOW );
}

#define SAMPLE_LINE(owm) DIO_READ_NA (CPDA(owm))

static uint8_t
one_wire_sample_line (OneWireMaster *owm)
{
  dio_pin_t *pin = (dio_pin_t *) owm;

  return
    DIO_READ_NA (
        _SFR_IO8 (pin->dir_reg),
        pin->dir_bit,
        _SFR_IO8 (pin->port_reg),
        pin->port_bit,
        _SFR_IO8 (pin->pin_reg),
        pin->pin_bit );
}

// We support only stand speed, not overdrive speed, so we make our tick 1 us.
#define TICK_TIME_IN_US 1.0

#define TICK_DELAY(ticks) _delay_us (TICK_TIME_IN_US * ticks)

// Pause for exactly ticks ticks.
static void
tickDelay (int ticks)
{
  _delay_us (TICK_TIME_IN_US * ticks);
}

///////////////////////////////////////////////////////////////////////////////

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

OneWireMaster *
owm_new (dio_pin_t pin)
{
  OneWireMaster *new_instance = malloc (sizeof (OneWireMaster));
  new_instance->pin = pin;

  return new_instance;
}

uint8_t
owm_touch_reset (OneWireMaster *owm)
{
  tickDelay (TICK_DELAY_G);
  one_wire_drive_line_low (owm);
  tickDelay (TICK_DELAY_H);
  // FIXME: WORK POINT: here is start converting from fctn to macro operation
  //one_wire_release_line (owm);
  RELEASE_LINE (owm);
  tickDelay (TICK_DELAY_I);
  // Look for presence pulse from slave
  uint8_t result = one_wire_sample_line (owm);
  tickDelay (TICK_DELAY_J); // Complete the reset sequence recovery

  return result; // Return sample presence pulse result
}

static void
OWWriteBit (OneWireMaster *owm, uint8_t bit)
{
  // Send a 1-Wire write bit. Provide 10us recovery time.

  if ( bit ) {
    // Write '1' bit
    one_wire_drive_line_low (owm);
    tickDelay (TICK_DELAY_A);
    one_wire_release_line (owm);
    tickDelay (TICK_DELAY_B); // Complete the time slot and 10us recovery
  }
  else {
    // Write '0' bit
    one_wire_drive_line_low (owm);
    tickDelay (TICK_DELAY_C);
    one_wire_release_line (owm);
    tickDelay (TICK_DELAY_D);
  }
}

static uint8_t
OWReadBit (OneWireMaster *owm)
{
  // Read a bit from the 1-Wire bus and return it. Provide 10us recovery time.

  one_wire_drive_line_low (owm);
  tickDelay(TICK_DELAY_A);
  one_wire_release_line (owm);
  tickDelay(TICK_DELAY_E);
  uint8_t result = one_wire_sample_line (owm);   // Sample bit value from slave
  tickDelay(TICK_DELAY_F); // Complete the time slot and 10us recovery

  return result;
}

void
owm_write_byte (OneWireMaster *owm, uint8_t data)
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

uint8_t
owm_read_byte (OneWireMaster *owm)
{
  uint8_t loop, result = 0;
  for ( loop = 0; loop < 8; loop++ ) {
    // Shift the result to get it ready for the next bit
    result >>= 1;
    // If result is one, then set MS bit
    if ( OWReadBit (owm) ) {
      result |= 0x80;
    }
  }

  return result;
}

uint8_t
owm_touch_byte (OneWireMaster *owm, uint8_t data)
{
  uint8_t loop, result = 0;
  for ( loop = 0; loop < 8; loop++ ) {
    // Shift the result to get it ready for the next bit
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
    // Shift the data byte for the next bit
    data >>= 1;
  }
  return result;
}
