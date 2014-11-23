// Test/demo for the one_wire_slave.h interface.
//
// This program implements a simple one-wire slave device.  It acts like
// a Maxim DS18B20, but the temperature is always about 42.42 degrees C :)

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "dio.h"
#include "one_wire_slave.h"
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

/*  FIXME: we're leaving this here for the moment to make it easier to
 *  implement the other side of things
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

  // We should have gotten the fixed family code as the first byte of the ID.
  assert (((uint8_t *) (&slave_rid))[0] == DS18B20_FAMILY_CODE);

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

static void
print_slave_id (uint64_t id)
{
   // Output the given slave id as a 64 bit hex number, using printf().

  printf ("0x");
  for ( uint8_t ii = 0 ; ii < sizeof (id) ; ii++ ) {
    printf ("%02" PRIx8, ((uint8_t *) (&id))[ii] );
  }
}
*/

int
main (void)
{
  // This isn't what we're testing exactly, but we need to know if it's
  // working or not to interpret other results.
  term_io_init ();
  PFP ("\n");
  PFP ("\n");
  PFP ("term_io_init() worked.\n");
  PFP ("\n");

  // FIXME: it might be nice to put true in here in the test somehow.
  // But then again the default ID should work so maybe there is no point.
  ows_init (FALSE);   // Initialize the one-wire interface slave end

  // FIXME: devel block to test that pin not broken
  {
    // For testing output:
    /*
    for ( ; ; ) {
      CHKP ();
      DIO_INIT (OWS_PIN, DIO_OUTPUT, DIO_DONT_CARE, HIGH);
      DIO_SET_LOW (OWS_PIN);
      _delay_ms (5000.0);
      DIO_SET_HIGH (OWS_PIN);
      _delay_ms (5000.0);
      DIO_INIT (OWS_PIN, DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
      for ( uint8_t ii = 0 ; ii < 5 ; ii++ ) {
        uint8_t reading = DIO_READ (OWS_PIN);
        PFP ("got reading: %i\n", (int) reading);
        _delay_ms (1000.0);
      }
    }
    */
    // can be used *instead* of the above to test input
    /*
    for ( ; ; ) {
      DIO_INIT (OWS_PIN, DIO_INPUT, DIO_ENABLE_PULLUP, DIO_DONT_CARE);
      for ( uint8_t ii = 0 ; ii < 5 ; ii++ ) {
        uint8_t reading = DIO_READ (OWS_PIN);
        PFP ("got reading: %i\n", (int) reading);
        _delay_ms (1000.0);
      }
    }
    */
  }

  // Because the one-wire protocol doesn't allow us to take a bunch of time
  // out to send things, we accumulate incremental test results in these
  // variables then output everything at once.  Of course, some of the
  // one-wire slave functions might block forever if things aren't working
  // right, in which case more detailed diagnostics might need to be inserted.
  // These variables stand for Got Reset Pulse Sent Presence Pulse and Got
  // Read Rom Command.
  uint8_t grpspp = FALSE, grrc = FALSE;

  PFP ("Trying ows_wait_then_signal_presence()... ");
  ows_wait_then_signal_presence ();
  grpspp = TRUE;
  uint8_t command = ows_read_byte ();
  if ( command == OWS_READ_ROM_COMMAND ) {
    grrc = TRUE;
  }
  else {
    // FIXME: WORK POINT: seem to get zeros here...
    PFP ("uh oh, seemed to get command %x\n", (uint8_t) command);
  }
  PFP ("Test results:\n");
  if ( grpspp ) {
    PFP ("  Got reset pulse (and sent presence pulse)\n");
  }
  if ( grrc ) {
    PFP ("  Got READ_ROM command\n");
  }
  // FIXME: not we need to verify if sending the ROM ID seemed to work from
  // this end.

  /*  FIXME: commented out master-side code follows...
  PFP ("Trying owm_touch_reset()... ");
  uint8_t slave_presence = owm_touch_reset ();
  if ( ! slave_presence ) {
    PFP ("failed: non-true was returned");
    assert (FALSE);
  }
  PFP ("ok, got slave presence pulse.\n");

#ifdef OWM_TEST_CONDITION_SINGLE_SLAVE

  PFP ("Trying DS18B20 initialization... ");
  uint64_t slave_rid = ds18b20_init_and_rom_command ();
  PFP ("ok, succeeded.  Slave ID: ");
  print_slave_id (slave_rid);
  PFP ("\n");


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
  PFP ("Trying owm_read_id()... ");
  uint8_t device_found = owm_read_id ((uint8_t *) &rid);
  if ( (! device_found) || rid != slave_rid ) {
    PFP ("failed: didn't find slave with previously discovered ID");
    assert (FALSE);
  }
  PFP ("ok, found slave with previously discovered ID.\n");

  PFP ("Trying owm_first()... ");
  device_found = owm_first ((uint8_t *) &rid);
  if ( (! device_found) || rid != slave_rid ) {
    PFP ("failed: didn't find slave with previously discovered ID");
    assert (FALSE);
  }
  PFP ("ok, found slave with previously discovered ID.\n");

  // Verify that owm_next() (following the owm_first() call above) returns
  // false, since there is only one device on the bus.
  PFP ("Trying owm_next()... ");
  device_found = owm_next ((uint8_t *) &rid);
  if ( device_found ) {
    PFP ("failed: unexpectedly returned true");
    assert (FALSE);
  }
  PFP ("ok, no next device found.\n");

  // owm_verify() should work with either a single or multiple slaves.
  PFP ("Trying owm_verify() with previously discoved ID... ");
  device_found = owm_verify ((uint8_t *) &rid);
  if ( ! device_found ) {
    PFP ("failed: unexpectedly returned false");
    assert (FALSE);
  }
  PFP ("ok, ID verified.\n");

  PFP ("Starting temperature conversion... ");
  uint8_t const convert_t_command = 0x44;
  owm_write_byte (convert_t_command);
  // The DS18B20 is now supposed to respond with a stream of 0 bits until
  // the conversion completes, after which it's supposed to send 1 bits.
  // Note that this is probably a typical behavior for busy one-wire devices.
  uint8_t conversion_complete = 0;
  while ( ! (conversion_complete = owm_read_bit ()) ) {
    ;
  }
  PFP ("conversion complete.\n");

  // We can now read the device scratchpad memory.  This requires us to first
  // perform the initialization and read rom commands again as described in
  // the DS18B20 datasheet.  The slave ROM code better be the same on second
  // reading :)
  PFP ("Reading DS18B20 scratchpad memory... ");
  uint64_t slave_rid_2nd_reading = ds18b20_init_and_rom_command ();
  assert (slave_rid_2nd_reading == slave_rid);
  ds18b20_get_scratchpad_contents ();
  PFP ("done.\n");

  // Convenient names for the temperature bytes
  uint8_t t_lsb = spb[DS18B20_SCRATCHPAD_T_LSB];
  uint8_t t_msb = spb[DS18B20_SCRATCHPAD_T_MSB];

  uint8_t tin = (t_msb & B10000000);   // Temperature Is Negative

  // Absolute value of temperature (in degrees C) times 2^4.  This is just
  // what the DS18B20 likes to spit out.  See Fig. 2 of the DS18B20 datasheet.
  int16_t atemp_t2tt4 = (((int16_t) t_msb) << BITS_PER_BYTE) | t_lsb;
  if ( tin ) {   // If negative...
    // ...just make it positive (it's 2's compliment)
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

  // NOTE: I think avrlibc doesn't suppor the # printf flag.  No big loss, but
  // it means the blinked-out output might have a different number of digits
  PFP (
      "Temperature reading: %#.6g degrees C (subject to rounding issues).\n",
      (tin ? -1.0 : 1.0) * atemp);

  PFP ("All tests passed (assuming temperature looks sane :).\n");
  PFP ("\n");
  PFP (
      "Will now blink out abs(temperature) forever. Note that due to\n"
      "rounding/formatting issues the number of blinked-out digits or values\n"
      "of those digits might differ slightly from the printf()-generated\n"
      "version.\n" );

  // atemp Times 10000
  uint32_t att10000 = round(atemp * 10000);

  // Blink out the absolute value of the current temperate times 10000
  // (effectively including four decimal places).
  for ( ; ; ) {
    // Feeding the wdt is harmless even when it's not initialized.
    BLINK_OUT_UINT32_FEEDING_WDT (att10000);
  }

#endif

#ifdef OWM_TEST_CONDITION_MULTIPLE_SLAVES

#ifndef OWM_FIRST_SLAVE_ID
#  error OWM_FIRST_SLAVE_ID is not defined
#endif
#ifndef OWM_SECOND_SLAVE_ID
#  error OWM_SECOND_SLAVE_ID is not defined
#endif

  // Account for endianness by swapping the bytes of the literal ID values.
  uint64_t first_device_id
    = __builtin_bswap64 (UINT64_C (OWM_FIRST_SLAVE_ID));
  uint64_t second_device_id
    = __builtin_bswap64 (UINT64_C (OWM_SECOND_SLAVE_ID));

  uint64_t rid;   // ROM ID

  PFP ("Trying owm_first()... ");
  uint8_t device_found = owm_first ((uint8_t *) &rid);
  if ( ! device_found ) {
    PFP ("failed: didn't discover any slave devices");
    assert (FALSE);
  }
  if ( rid != first_device_id ) {
    PFP ("failed: discovered first device ID was ");
    print_slave_id (rid);
    PFP (", not the expected ");
    print_slave_id (first_device_id);
    assert (FALSE);
  }
  PFP ("ok, found 1st device with expected ID ");
  print_slave_id (rid);
  PFP (".\n");

  PFP ("Trying owm_next()... ");
  device_found = owm_next ((uint8_t *) &rid);
  if ( ! device_found ) {
    PFP ("failed: didn't discover a second slave device");
    assert (FALSE);
  }
  if ( rid != second_device_id ) {
    PFP ("failed: discovered second device ID was ");
    print_slave_id (rid);
    PFP (", not the expected ");
    print_slave_id (second_device_id);
    assert (FALSE);
  }
  PFP ("ok, found 2nd device with expected ID ");
  print_slave_id (rid);
  PFP (".\n");

  // Verify that owm_next() (following the owm_first() call above) returns
  // false, since there are only two devices on the bus.
  PFP ("Trying owm_next() again... ");
  device_found = owm_next ((uint8_t *) &rid);
  if ( device_found ) {
    PFP ("failed: unexpectedly returned true");
    assert (FALSE);
  }
  PFP ("ok, no next device found.\n");

  PFP("Trying owm_verify(first_device_id)... ");
  device_found = owm_verify ((uint8_t *) &rid);
  if ( ! device_found ) {
    PFP ("failed: didn't find device");
    assert (FALSE);
  }
  PFP ("ok, found it.\n");

  PFP("Trying owm_verify(second_device_id)... ");
  device_found = owm_verify ((uint8_t *) &rid);
  if ( ! device_found ) {
    PFP ("failed: didn't find device");
    assert (FALSE);
  }
  PFP ("ok, found it.\n");

  PFP ("All tests passed.\n");
  PFP ("\n");

  */
}
