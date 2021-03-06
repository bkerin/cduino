// Test/demo for the one_wire_master.h interface.
//
// If you're new to 1-wire you should first read the entire
// Maxim_DS18B20_datasheet.pdf.  Its hard to use 1-wire without at least a
// rough understanding of how the line signalling and transaction schemes
// work.
//
// This test program tests many lower-level functions before owm_scan_bus()
// and owm_start_transaction(), which are the ones you probably want to
// use to initiate transactions.
//
// By default, this test program requires exactly one slave to be present:
// a Maxim DS18B20 temperature sensor connected to the chosen one wire master
// pin of the arduino (by default DIO_PIN_DIGITAL_2 -- see the Makefile for
// this module to change this value).  The D18B20 is required to be powered
// externally, not using parasite power.  The DS18B20 must be in its default
// (factory) state.  Normally a 4.7 kohm pull-up resistor must be used on
// the Arduino side of the wire.  See Figure 5 of the DS18B20 datasheet,
// revision 042208.  You might get away with using the internal pull-up
// instead; see near OWM_USE_INTERNAL_PULLUP in one_wire_master.c for details.
//
// Test results are ouput via the term_io.h interface (which is not required
// by the module itself).  If the USB cable used for programming the Arduino
// is still connected, it should be possible to run
//
//   make -rR run_screen
//
// from the module directory to see the test results.
// FIXME: ensure that other modules that use screen mention how to run it
//
// The entire test sequence repeats perpetually.
//
// It's also possible to compile this module differently to test its
// performance with multiple slaves.  See the notes in the Makefile for
// this module for details.

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "dio.h"
#include "ds18b20_commands.h"
#include "one_wire_master.h"
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

// See the definition of this macro in util.h to understand why its here.
WATCHDOG_TIMER_MCUSR_MANTRA

char result_buf[OWM_RESULT_DESCRIPTION_MAX_LENGTH + 1];

#define OWM_CHECK(result)                                        \
  PFP_ASSERT_SUCCESS (result, owm_result_as_string, result_buf);

// The default incarnation of this test program expects a single slave,
// but it can also be compiled and tweaked slightly to test a multi-slave bus.

// The functions that perform DS18B20-specific operations are only used in
// the default single-slave test condition.
#ifdef TEST_CONDITION_SINGLE_SLAVE

#  define DS18B20_FAMILY_CODE UINT8_C (0x28)

// These are properties of the DS18B20 that have nothing to do with the
// 1-wire bus in general.
#  define DS18B20_SCRATCHPAD_SIZE  9
#  define DS18B20_SCRATCHPAD_T_LSB 0
#  define DS18B20_SCRATCHPAD_T_MSB 1

static uint8_t spb[DS18B20_SCRATCHPAD_SIZE];   // DS18B20 Scratchpad Buffer

static uint64_t
ds18b20_init_and_rom_command (void)
{
  // Requies exactly one DS18B20 slave to be present on the bus.  Perform
  // the Initialization (Step 1) and ROM Command (Step 2) steps of the
  // transaction sequence described in the
  // Maxim_DS18B20_datasheet.pdf, page 10, and return the discovered ROM ID
  // code of the slave.  Note that functions that perform this operation
  // in a single call are available in the one_wire_master.h interface,
  // but here we perform them manually as a cross-check.

  // Prompt the slave(s) to respond with a "presence pulse".
  // This corresponds to the "INITIALIZATION" step described in the
  // Maxim_DS18B20_datasheet.pdf, page 10.
  uint8_t slave_presence = owm_touch_reset ();
  PFP_ASSERT (slave_presence);

  // This test program requires that only one slave be present, so we can
  // use the READ ROM command to get the slave's ROM ID.
  uint64_t slave_rid;
  owm_write_byte (OWC_READ_ROM_COMMAND);
  for ( uint8_t ii = 0 ; ii < sizeof (slave_rid) ; ii++ ) {
    ((uint8_t *) (&slave_rid))[ii] = owm_read_byte ();
  }

  // We should have gotten the fixed family code as the first byte of the ID.
  if (((uint8_t *) (&slave_rid))[0] != DS18B20_FAMILY_CODE) {
    PFP (
        "\nUnexpected slave family code '%hhx'\n",
        ((uint8_t *) (&slave_rid))[0] );
  }
  PFP_ASSERT (((uint8_t *) (&slave_rid))[0] == DS18B20_FAMILY_CODE);

  return slave_rid;
}

static void
ds18b20_get_scratchpad_contents (void)
{
  // Send the command that causes the DS18B20 to send the scratchpad contents,
  // then read the result and store it in spb.  This routine must follow a
  // ds18b20_init_and_rom_command() call.

  owm_write_byte (DS18B20_COMMANDS_READ_SCRATCHPAD_COMMAND);
  for ( uint8_t ii = 0 ; ii < DS18B20_SCRATCHPAD_SIZE ; ii++ ) {
    spb[ii] = owm_read_byte ();
  }
}

#endif

static void
print_slave_id (uint64_t id)
{
   // Output the given slave id as a 64 bit hex number, using PFP().

  PFP ("0x");
  for ( uint8_t ii = 0 ; ii < sizeof (id) ; ii++ ) {
    PFP ("%02" PRIx8, ((uint8_t *) (&id))[ii] );
  }
}

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

  owm_init ();   // Initialize the 1-wire interface master end

  PFP ("Trying owm_touch_reset()... ");
  uint8_t slave_presence = owm_touch_reset ();
  PFP_ASSERT (slave_presence);
  PFP ("ok, got slave presence pulse.\n");

#ifdef TEST_CONDITION_SINGLE_SLAVE

  // All the tests are re-run forever.  This can be convenient when checking
  // for effects of line lenght, etc. that don't crop up every time.
  for ( ; ; ) {

    PFP ("Trying DS18B20 initialization... ");
    uint64_t slave_rid = ds18b20_init_and_rom_command ();
    PFP ("ok, succeeded.  Slave ID: ");
    print_slave_id (slave_rid);
    PFP ("\n");

    uint64_t rid;   // ROM ID (of slave)

    // We can use owm_read_id() because we know we have exactly one slave.
    PFP ("Trying owm_read_id()... ");
    OWM_CHECK (owm_read_id ((uint8_t *) &rid));  // One-wire Master Result
    PFP_ASSERT (rid == slave_rid);   // Should be the same as last time
    PFP ("ok, found slave with previously discovered ID.\n");

    PFP ("Trying owm_first()... ");
    OWM_CHECK (owm_first ((uint8_t *) &rid));
    PFP_ASSERT (rid == slave_rid);   // Should be the same as last time
    PFP ("ok, found slave with previously discovered ID.\n");

    // Verify that owm_next() (following the owm_first() call above) returns
    // OWM_RESULT_NO_SUCH_SLAVE, since there is only one slave on the bus.
    PFP ("Trying owm_next()... ");
    owm_result_t result = owm_next ((uint8_t *) &rid);
    if ( result != OWM_RESULT_NO_SUCH_SLAVE ) {
      PFP (
          "failed: returned %s instead of OWM_RESULT_NO_SUCH_SLAVE\n",
          owm_result_as_string (result, result_buf) );
      PFP_ASSERT_NOT_REACHED ();
    }
    PFP ("ok, no next slave found.\n");

    // The normal test arrangement doesn't feature any alarmed slaves.  Its kind
    // of a pain to program the alarm condition on real DS18B20 devices and
    // I haven't done so.  The one_wire_slave_test.c program contains a line
    // that can be uncommented to cause and slave created using that interface
    // to consider itself alarmed, however.
    PFP ("Trying owm_first_alarmed()... ");
    result = owm_first_alarmed ((uint8_t *) &rid);
    if ( result == OWM_RESULT_NO_SUCH_SLAVE ) {
      PFP ("no alarmed slaves found (usually ok, see source).\n");
    }
    else if ( result == OWM_RESULT_SUCCESS ) {
      PFP ("found alarmed slave (ID: ");
      print_slave_id (slave_rid);
      PFP (").\n");
    }
    else {
      PFP (
          "failed: returned %s instead of OWM_RESULT_NO_SUCH_SLAVE or "
          "OWM_RESULT_SUCCESS\n",
          owm_result_as_string (result, result_buf) );
      PFP_ASSERT_NOT_REACHED ();
    }
    if ( result == OWM_RESULT_SUCCESS ) {
      // If owm_first_alarmed() found an alarmed slave, its reasonable to look
      // for another one, which we try here.
      PFP ("Trying own_next_alarmed()... ");
      result = owm_next_alarmed ((uint8_t *) &rid);
      if ( result != OWM_RESULT_NO_SUCH_SLAVE ) {
        PFP (
            "failed: returned %s instead of OWM_RESULT_NO_SUCH_SLAVE\n",
            owm_result_as_string (result, result_buf) );
        PFP_ASSERT_NOT_REACHED ();
      }
      PFP ("ok, no next alarmed slave found.\n");
    }

    // owm_verify() should work with either a single or multiple slaves.
    PFP ("Trying owm_verify() with previously discoved ID... ");
    OWM_CHECK (owm_verify ((uint8_t *) &rid));
    PFP ("ok, ID verified.\n");

    // Verify that owm_verify() works as expected when given a nonexistend RID.
    PFP ("Trying owm_verify() with nonexistent ID... ");
    uint64_t nerid = rid + 42;   // NonExistent RID
    result = owm_verify ((uint8_t *) &nerid);
    if ( result != OWM_RESULT_NO_SUCH_SLAVE ) {
      PFP (
          "failed: returned %s instead of OWM_RESULT_NO_SUCH_SLAVE\n",
          owm_result_as_string (result, result_buf) );
      PFP_ASSERT_NOT_REACHED ();
    }
    PFP ("ok, no such slave found.\n");

    PFP ("Trying owm_scan_bus()... ");
    uint64_t **rom_ids;
    OWM_CHECK (owm_scan_bus ((uint8_t ***) (&rom_ids)));
    PFP_ASSERT (*(rom_ids[0]) == rid);
    PFP_ASSERT (rom_ids[1] == NULL);
    PFP ("ok.\n");

    // Repeatedly doing owm_scan_bus() and owm_free_rom_ids_list() has also
    // been tried to check for leaks.
    PFP ("Trying owm_free_rom_ids_list()... ");
    owm_free_rom_ids_list ((uint8_t **) rom_ids);
    PFP ("seemed to work.\n");

    PFP ("Starting temperature conversion... ");
    // NOTE: the DS18B20 doesn't seem to require an addressing command before
    // the "Convert T" command here, which is contrary to its own datasheet.
    // I guess if there's only one slave on the bus it works ok, but its sort of
    // weird, so we go ahead and do the addressing again.
    uint64_t slave_rid_2nd_reading = ds18b20_init_and_rom_command ();
    PFP_ASSERT (slave_rid_2nd_reading == slave_rid);
    owm_write_byte (DS18B20_COMMANDS_CONVERT_T_COMMAND);
    // The DS18B20 is now supposed to respond with a stream of 0 bits until
    // the conversion completes, after which it's supposed to send 1 bits.
    // Note that this is probably a typical behavior for busy 1-wire slaves.
    uint8_t conversion_complete = 0;
    while ( ! (conversion_complete = owm_read_bit ()) ) {
      ;
    }
    PFP ("conversion complete.\n");

    // Now we try the conversion command a few more times using different
    // flavors of the highest-level, generic transaction initiation routine.
    // Of course, after all these conversions the temperature should
    // still come out the same.  If it doesn't, that could indicate that
    // owm_start_transaction() isn't working right.

    PFP ("Trying owm_start_transaction() with READ_ROM... ");
    uint64_t slave_rid_3rd_reading;
    OWM_CHECK (
        owm_start_transaction (
          OWC_READ_ROM_COMMAND,
          (uint8_t *) &slave_rid_3rd_reading,
          DS18B20_COMMANDS_CONVERT_T_COMMAND ) );
    PFP_ASSERT (slave_rid_3rd_reading == slave_rid);
    conversion_complete = 0;
    while ( ! (conversion_complete = owm_read_bit ()) ) {
      ;
    }
    PFP ("seemed ok.\n");

    PFP ("Trying owm_start_transaction() with MATCH_ROM... ");
    OWM_CHECK (
        owm_start_transaction (
          OWC_MATCH_ROM_COMMAND,
          (uint8_t *) &slave_rid,
          DS18B20_COMMANDS_CONVERT_T_COMMAND ) );
    conversion_complete = 0;
    while ( ! (conversion_complete = owm_read_bit ()) ) {
      ;
    }
    PFP ("seemed ok.\n");

    PFP ("Trying owm_start_transaction() with SKIP_ROM... ");
    OWM_CHECK (
        owm_start_transaction (
          OWC_SKIP_ROM_COMMAND,
          NULL,
          DS18B20_COMMANDS_CONVERT_T_COMMAND ) );
    conversion_complete = 0;
    while ( ! (conversion_complete = owm_read_bit ()) ) {
      ;
    }
    PFP ("seemed ok.\n");

    // We can now read the slave scratchpad memory.  This requires us to first
    // perform the initialization and read rom commands again as described
    // in the DS18B20 datasheet.  The slave ROM code better be the same on
    // second reading :)
    PFP ("Reading DS18B20 scratchpad memory... ");
    uint64_t slave_rid_4th_reading = ds18b20_init_and_rom_command ();
    PFP_ASSERT (slave_rid_4th_reading == slave_rid);
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

    // Uncomment some of this to test the the 2's compliment features
    // themselves, ignoring the real sensor output (but assuming we make it
    // to this point :).  Table 1 of the DS18B20 datasheet has a number of
    // example values.
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

    double const dbi = 2.42;   // Delay between iterations
    _delay_ms (dbi);
  }

#endif

#ifdef TEST_CONDITION_MULTIPLE_SLAVES

#ifndef FIRST_SLAVE_ID
#  error FIRST_SLAVE_ID is not defined
#endif
#ifndef SECOND_SLAVE_ID
#  error SECOND_SLAVE_ID is not defined
#endif

  // Account for endianness by swapping the bytes of the literal ID values.
  uint64_t first_slave_id
    = __builtin_bswap64 (UINT64_C (FIRST_SLAVE_ID));
  uint64_t second_slave_id
    = __builtin_bswap64 (UINT64_C (SECOND_SLAVE_ID));

  uint64_t rid;   // ROM ID

  PFP ("Trying owm_first()... ");
  OWM_CHECK (owm_first ((uint8_t *) &rid));  // One-wire Master Result
  if ( rid != first_slave_id ) {
    PFP ("failed: discovered first slave ID was ");
    print_slave_id (rid);
    PFP (", not the expected ");
    print_slave_id (first_slave_id);
    PFP ("\n");
    PFP_ASSERT_NOT_REACHED ();
  }
  PFP ("ok, found 1st slave with expected ID ");
  print_slave_id (rid);
  PFP (".\n");

  PFP ("Trying owm_next()... ");
  OWM_CHECK (owm_next ((uint8_t *) &rid));
  if ( rid != second_slave_id ) {
    PFP ("failed: discovered second slave ID was ");
    print_slave_id (rid);
    PFP (", not the expected ");
    print_slave_id (second_slave_id);
    PFP_ASSERT_NOT_REACHED ();
  }
  PFP ("ok, found 2nd slave with expected ID ");
  print_slave_id (rid);
  PFP (".\n");

  // Verify that owm_next() (following the owm_first() call above) returns
  // false, since there are only two slaves on the bus.
  PFP ("Trying owm_next() again... ");
  owm_result_t result = owm_next ((uint8_t *) &rid);
  if ( result != OWM_RESULT_NO_SUCH_SLAVE ) {
    PFP (
        "failed: returned %s instead of OWM_RESULT_NO_SUCH_SLAVE\n",
        owm_result_as_string (result, result_buf) );
    PFP_ASSERT_NOT_REACHED ();
  }
  PFP ("ok, no next slave found.\n");

  PFP("Trying owm_verify(first_slave_id)... ");
  OWM_CHECK (owm_verify ((uint8_t *) &first_slave_id));
  PFP ("ok, found it.\n");

  PFP("Trying owm_verify(second_slave_id)... ");
  OWM_CHECK (owm_verify ((uint8_t *) &second_slave_id));
  PFP ("ok, found it.\n");

  PFP ("All tests passed.\n");
  PFP ("\n");

#endif

}
