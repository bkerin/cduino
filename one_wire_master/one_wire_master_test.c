//
// FIXME: describe seperate power assumption
//
// This program assumes that the DS18B20 EEPROM configuration is in the
// default (factory) state: FIXME; say what that is here


#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "dio.h"
#include "one_wire_master.h"
#include "util.h"

// By default this test program expects to find exactly one slave on the
// one-wire bus, but there is some alternate code included for testing with
// more slaves.
//#define OWM_TEST_CONDITION_SINGLE_SLAVE

#define DS18B20_SCRATCHPAD_SIZE  9
#define DS18B20_SCRATCHPAD_T_LSB 0
#define DS18B20_SCRATCHPAD_T_MSB 1

static uint8_t spb[DS18B20_SCRATCHPAD_SIZE];   // DS18B20 Scratchpad Buffer

static uint64_t
ds18b20_init_and_rom_command (void)
{
  // Requies exactly one DS18B20 device to be present on the bus.  Perform the
  // Initialization (Step 1) and ROM Command (Step 2) steps of the transaction
  // sequence described in the DS18B20 datasheet, and return the discovered
  // ROM code of the slave.

  // Prompt the slave(s) to respond with a "presence pulse".  This corresponds
  // to the "INITIALIZATION" step (Step 1) described in the DS18B20 datasheet.
  // FIXME: would be nice to have datasheet available on web and linked to
  // by the docs...
  uint8_t slave_presence = owm_touch_reset ();
  assert (slave_presence);

  // This test program requires that only one slave be present, so we can
  // use the READ ROM command to get the slave's ROM ID.
  uint64_t slave_rid;
  uint8_t const read_rom_command = 0x33;
  owm_write_byte (read_rom_command);
  for ( uint8_t ii = 0 ; ii < sizeof (slave_rid) ; ii++ ) {
    ((uint8_t *) (&slave_rid))[ii] = owm_read_byte ();
  }

  return slave_rid;
}

static void
ds18b20_get_scratchpad_contents (void)
{
  // Send the command that causes the DS18B20 to send the scratchpad contents,
  // then read the result and store it in spb.  This routine must follow an
  // ds18b20_init_and_rom_command() call.

  uint8_t const read_scratchpad_command = 0xBE;
  owm_write_byte (read_scratchpad_command);
  for ( uint8_t ii = 0 ; ii < DS18B20_SCRATCHPAD_SIZE ; ii++ ) {
    spb[ii] = owm_read_byte ();
  }
}

int
main (void)
{
  owm_init ();   // Initialize the one-wire interface master end

  uint64_t slave_rid = ds18b20_init_and_rom_command ();

  uint8_t const convert_t_command = 0x44;
  owm_write_byte (convert_t_command);

  // The DS18B20 is now supposed to respond with a stream of 0 bits until the
  // conversion completes, after which it's supposed to send 1 bits.  So we
  // could do this bit-by-bit if our API exposed the bit-by-bit interface.
  // But it shouldn't hurt to read a few extra ones.
  uint8_t conversion_complete = 0;
  while ( ! (conversion_complete = owm_read_byte ()) ) {
    ;
  }

  // We can now read the device scratchpad memory.  This requires us to first
  // perform the initialization and read rom commands again as described in
  // the DS18B20 datasheet.  The slave ROM code better be the same on second
  // reading :)
  uint64_t slave_rid_2nd_reading = ds18b20_init_and_rom_command ();
  assert (slave_rid_2nd_reading == slave_rid);
  ds18b20_get_scratchpad_contents ();

#ifndef OWM_TEST_CONDITION_MULTIPLE_SLAVE

  uint64_t rid;   // ROM ID

  uint8_t device_found = owm_read_id ((uint8_t *) &rid);
  assert (device_found);
  assert (rid == slave_rid);

  device_found = owm_first ((uint8_t *) &rid);
  assert (device_found);
  assert (rid == slave_rid);

  // Verify that owm_next() (following the owm_first() call above) returns
  // false, since there is only one device on the bus.
  device_found = owm_next ((uint8_t *) &rid);
  assert (! device_found);

  // FIXME: WORK POINT: worked to here fine I think

  // owm_verify() should work with either a single or multiple slaves.
  device_found = owm_verify ((uint8_t *) &rid);
  assert (device_found);
  assert (rid == slave_rid);

#else

  uint64_t rid;   // ROM ID

  device_found = owm_next ((uint8_t *) &rid);
  assert (device_found);
  // Must put in the real value of the second device ID here
  uint64_t const second_device_id = 0x4242424242424242;
  assert (rid == second_device_id);

#endif

  // Convenient names for the temperature bytes
  uint8_t t_lsb = spb[DS18B20_SCRATCHPAD_T_LSB];
  uint8_t t_msb = spb[DS18B20_SCRATCHPAD_T_MSB];

  // Absolute value of temperature (in degrees C) times 2^4.  This is just
  // what the DS18B20 likes to spit out.  See Fig. 2 of the DS18B20 datasheet.
  int16_t atemp_t2tt4 = (((int16_t) t_msb) << BITS_PER_BYTE) | t_lsb;
  if ( t_msb & B10000000 ) {   // If negative...
    // ...just make it positive (its 2's compliment)
    atemp_t2tt4 = (~atemp_t2tt4) + 1;
  }

  // Uncomment some of this to test the the 2's compliment and blinky-output
  // features themselves, ignoring the real sensor output.  Table 1 of the
  // DS18B20 datasheet has a number of example values.
  //
  //atemp_t2tt4 = INT16_C (0xFF5E);    // Means -10.125 (blinks out 101250)
  //atemp_t2tt4 = (~atemp_t2tt4) + 1;  // Knowns to be negative so abs it
  //
  //atemp_t2tt4 = INT16_C (0x0191);    // Means 25.0625 (blinks out 250625)

  // Absolute value of temperature, in degrees C
  double atemp = atemp_t2tt4 / (2.0 * 2.0 * 2.0 * 2.0);

  // atemp Times 10000
  uint32_t att10000 = round(atemp * 10000);

  // Blink out the absolute value of the current temperate times 10000
  // (effectively including four decimal places).
  for ( ; ; ) {
    // I think feeding the wdt is harmless even when its not initialized.
    BLINK_OUT_UINT32_FEEDING_WDT (att10000);
  }
}
