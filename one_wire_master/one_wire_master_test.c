// Test/demo for the one_wire_master.h interface.
//
// By default, this test program requires exactly one slave to be present:
// a Maxim DS18B20 temperature sensor connected to the chosen one wire master
// pin of the arduino (by default DIO_PIN_DIGITAL_2 -- see the Makefile for
// this module to change this value).  The D18B20 is required to be powered
// externally, not using parasite power.  The DS18B20 must be in its default
// (factory) state.  A 4.7 kohm pull-up resistor must be used on the Arduino
// side of the wire.  See Figure 5 of the DS18B20 datasheet, revision 042208.
//
// If everything works correctly, the arduino should blink out the absolute
// value of the sensed temperature in degrees Celcius multiplied by 10000
// on the on-board led (connected to PB5).  Single quick blinks are zeros,
// slower series of blinks are other digits.

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "dio.h"
#include "one_wire_master.h"
#include "util.h"

// The default incarnation of this test program expects a single slave,
// but it can also be compiled and tweaked slightly to test a multi-slave bus.
#ifndef OWM_TEST_CONDITION_MULTIPLE_SLAVES

// These are properties of the DS18B20 that have nothing to do with the
// one-wire bus in general.
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
  // ROM code of the slave.  Note that functions that perform this operation
  // in a single call are available in the one_wire_master.h interface,
  // but here we perform them manually as a cross-check.

  // Prompt the slave(s) to respond with a "presence pulse".  This corresponds
  // to the "INITIALIZATION" step (Step 1) described in the DS18B20 datasheet.
  // FIXME: would be nice to have datasheet available on web and linked to
  // by the docs...
  uint8_t slave_presence = owm_touch_reset ();
  assert (slave_presence);

  // This test program requires that only one slave be present, so we can
  // use the READ ROM command to get the slave's ROM ID.
  uint64_t slave_rid;
  owm_write_byte (OWM_READ_ROM_COMMAND);
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

#endif

int
main (void)
{

#ifndef OWM_TEST_CONDITION_MULTIPLE_SLAVES

  owm_init ();   // Initialize the one-wire interface master end

  uint64_t slave_rid = ds18b20_init_and_rom_command ();

  uint8_t const convert_t_command = 0x44;
  owm_write_byte (convert_t_command);

  // The DS18B20 is now supposed to respond with a stream of 0 bits until the
  // conversion completes, after which it's supposed to send 1 bits.  So we
  // could do this bit-by-bit if our API exposed the bit-by-bit interface.
  // FIXME: which it now does.  So should we do it that way?
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

  // Uncomment this to repeatedly blink out the (decimal) byte values of
  // the ROM ID for the device instead of continuing the normal test program.
  //for ( ; ; ) {
  //  for ( uint8_t ii = 0 ; ii < sizeof (uint64_t) ; ii++ ) {
  //    BLINK_OUT_UINT32_FEEDING_WDT (((uint8_t *) slave_id)[ii]);
  //  }
  //  _delay_ms (4.2);
  //}

  uint64_t rid;   // ROM ID

  // We can use owm_read_id() because we know we have exactly one slave.
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

  // owm_verify() should work with either a single or multiple slaves.
  device_found = owm_verify ((uint8_t *) &rid);
  assert (device_found);
  assert (rid == slave_rid);

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
  // features themselves, ignoring the real sensor output (but assuming
  // we make it to this point :).  Table 1 of the DS18B20 datasheet has a
  // number of example values.
  //
  //atemp_t2tt4 = INT16_C (0xFF5E);    // Means -10.125 (blinks out 101250)
  //atemp_t2tt4 = (~atemp_t2tt4) + 1;  // Knowns to be negative so abs it
  //
  //atemp_t2tt4 = INT16_C (0x0191);    // Means 25.0625 (blinks out 250625)

  // Absolute value of temperature, in degrees C.  These factors of 2.0 are
  // due to the meaning the DS18B20 assigns to the individual field bits.
  double atemp = atemp_t2tt4 / (2.0 * 2.0 * 2.0 * 2.0);

  // atemp Times 10000
  uint32_t att10000 = round(atemp * 10000);

  // Blink out the absolute value of the current temperate times 10000
  // (effectively including four decimal places).
  for ( ; ; ) {
    // I think feeding the wdt is harmless even when its not initialized.
    BLINK_OUT_UINT32_FEEDING_WDT (att10000);
  }

#else // OWM_TEST_CONDITION_MULTIPLE_SLAVES is defined

  // Must put in the real value of the second device ID here.  There is a
  // commented-out block in the default single-slave version of this test
  // program above that can be used to determine the ROM ID of a slave
  // (FIXXME: in decimal bytes, unfortunately).
  uint64_t first_device_id = 0x4242424242424242;
  uint64_t second_device_id = 0x4242424242424242;

  // FIXME: remove these once we know our device IDs so can comment out
  // the asserts.
  first_device_id = first_device_id;
  second_device_id = second_device_id;

  uint64_t rid;   // ROM ID

  uint8_t device_found = owm_first ((uint8_t *) &rid);
  assert (device_found);
  //assert (rid == first_device_id);

  //BTRAP ();
  device_found = owm_next ((uint8_t *) &rid);
  assert (device_found);
  BTRAP ();
  //assert (rid == second_device_id);

#endif

}
