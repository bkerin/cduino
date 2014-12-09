// Test/demo for the one_wire_master.h interface.
//
// Note that this test program tests some lower-level functions earlier
// than owm_start_transaction(), which is the one you probably want to use.
//
// By default, this test program requires exactly one slave to be present:
// a Maxim DS18B20 temperature sensor connected to the chosen one wire master
// pin of the arduino (by default DIO_PIN_DIGITAL_2 -- see the Makefile for
// this module to change this value).  The D18B20 is required to be powered
// externally, not using parasite power.  The DS18B20 must be in its default
// (factory) state.  A 4.7 kohm pull-up resistor must be used on the Arduino
// side of the wire.  See Figure 5 of the DS18B20 datasheet, revision 042208.
//
// Test results are ouput via the term_io.h interface (which is not required
// by the module itself).  If the USB cable used for programming the Arduino
// is still connected, it should be possible to run
//
//   make -rR run_screen
//
// from the module directory to see the test results.
//
// If everything works correctly, the Arduino should also blink out the
// absolute value of the sensed temperature in degrees Celcius multiplied
// by 10000 on the on-board led (connected to PB5).  Single quick blinks
// are zeros, slower series of blinks are other digits.
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

// FIXME: this big ugly function should turn into a table or something,
// maybe with X macros or some other macro setup
static void
print_owm_error (owm_error_t err)
{
  switch ( err ) {
    case OWM_ERROR_NONE:
      PFP ("OWM_ERROR_NONE");
      break;
    case OWM_ERROR_GOT_INVALID_TRANSACTION_INITIATION_COMMAND:
      PFP ("OWM_ERROR_GOT_INVALID_TRANSACTION_INITIATION_COMMAND");
      break;
    case OWM_ERROR_GOT_ROM_COMMAND_INSTEAD_OF_FUNCTION_COMMAND:
      PFP ("OWM_ERROR_GOT_ROM_COMMAND_INSTEAD_OF_FUNCTION_COMMAND");
      break;
    case OWM_ERROR_DID_NOT_GET_PRESENCE_PULSE:
      PFP ("OWM_ERROR_DID_NOT_GET_PRESENCE_PULSE");
      break;
    case OWM_ERROR_GOT_ROM_ID_WITH_INCORRECT_CRC_BYTE:
      PFP ("OWM_ERROR_GOT_ROM_ID_WITH_INCORRECT_CRC_BYTE");
      break;
  }
}

// FIXME: this could conceivably be genericized into util.h as well, maybe
// taking a constant-to-string lookup table or so.
#define PFP_ASSERT_NO_OWM_ERROR(err) \
  pfp_assert_no_owm_error ((err), __FILE__, __LINE__, __func__)
static void
pfp_assert_no_owm_error (
    owm_error_t err,
    char const *file,
    int line,
    char const *func )
{
  if ( UNLIKELY (err) ) {
    PFP ("\n");
    PFP ("%s:%i: %s: OWM error: ", file, line, func);
    print_owm_error (err);
    PFP ("\n");
    assert (FALSE);
  }
}


// FIXME: this is another candidate to go in term_io.  or at least get docs
#define PFP_ASSERT(cond) \
  pfp_assert ((cond), __FILE__, __LINE__, __func__, #cond)

static void
pfp_assert (
    uint8_t cond,
    char const *file,
    int line,
    char const *func,
    char const *cond_string )
{
  if ( UNLIKELY (! cond) ) {
    PFP ("\n%s:%i: %s: Assertion `%s' failed.\n",
        file, line, func, cond_string );
    assert (FALSE);
  }
}

// The default incarnation of this test program expects a single slave,
// but it can also be compiled and tweaked slightly to test a multi-slave bus.

// The functions that perform DS18B20-specific operations are only used in
// the default single-slave test condition.
#ifdef OWM_TEST_CONDITION_SINGLE_SLAVE

#  define DS18B20_FAMILY_CODE UINT8_C (0x28)

// These are properties of the DS18B20 that have nothing to do with the
// one-wire bus in general.
#  define DS18B20_SCRATCHPAD_SIZE  9
#  define DS18B20_SCRATCHPAD_T_LSB 0
#  define DS18B20_SCRATCHPAD_T_MSB 1

static uint8_t spb[DS18B20_SCRATCHPAD_SIZE];   // DS18B20 Scratchpad Buffer

static uint64_t
ds18b20_init_and_rom_command (void)
{
  // Requies exactly one DS18B20 slave to be present on the bus.  Perform the
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
  PFP_ASSERT (slave_presence);

  // This test program requires that only one slave be present, so we can
  // use the READ ROM command to get the slave's ROM ID.
  uint64_t slave_rid;
  owm_write_byte (OWC_READ_ROM_COMMAND);
  for ( uint8_t ii = 0 ; ii < sizeof (slave_rid) ; ii++ ) {
    ((uint8_t *) (&slave_rid))[ii] = owm_read_byte ();
  }

  // We should have gotten the fixed family code as the first byte of the ID.
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
   // Output the given slave id as a 64 bit hex number, using printf().

  printf ("0x");
  for ( uint8_t ii = 0 ; ii < sizeof (id) ; ii++ ) {
    printf ("%02" PRIx8, ((uint8_t *) (&id))[ii] );
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

  owm_init ();   // Initialize the one-wire interface master end

  PFP ("Trying owm_touch_reset()... ");
  uint8_t slave_presence = owm_touch_reset ();
  if ( ! slave_presence ) {
    PFP ("failed: non-true was returned");
    PFP_ASSERT (FALSE);
  }
  PFP ("ok, got slave presence pulse.\n");

#ifdef OWM_TEST_CONDITION_SINGLE_SLAVE

  PFP ("Trying DS18B20 initialization... ");
  uint64_t slave_rid = ds18b20_init_and_rom_command ();
  PFP ("ok, succeeded.  Slave ID: ");
  print_slave_id (slave_rid);
  PFP ("\n");

  uint64_t rid;   // ROM ID (of slave)

  // We can use owm_read_id() because we know we have exactly one slave.
  PFP ("Trying owm_read_id()... ");
  uint8_t slave_found = owm_read_id ((uint8_t *) &rid);
  if ( (! slave_found) ) {
    PFP ("failed: no slave found");
    PFP_ASSERT (FALSE);
  }
  if ( rid != slave_rid ) {
    PFP ("failed: discovered slave ID is different");
    PFP_ASSERT (FALSE);
  }
  PFP ("ok, found slave with previously discovered ID.\n");

  PFP ("Trying owm_first()... ");
  slave_found = owm_first ((uint8_t *) &rid);
  if ( (! slave_found) ) {
    PFP ("failed: no slave found");
    PFP_ASSERT (FALSE);
  }
  if ( rid != slave_rid ) {
    PFP ("failed: discovered slave ID is different");
    PFP_ASSERT (FALSE);
  }
  PFP ("ok, found slave with previously discovered ID.\n");

  // Verify that owm_next() (following the owm_first() call above) returns
  // false, since there is only one slave on the bus.
  PFP ("Trying owm_next()... ");
  slave_found = owm_next ((uint8_t *) &rid);
  if ( slave_found ) {
    PFP ("failed: unexpectedly returned true");
    PFP_ASSERT (FALSE);
  }
  PFP ("ok, no next slave found.\n");

  // The normal test arrangement doesn't feature any alarmed slaves.  Its kind
  // of a pain to program the alarm condition on real DS18B20 devices and
  // I haven't done so.  The one_wire_slave_test.c program contains a line
  // that can be uncommented to cause and slave created using that interface
  // to consider itself alarmed, however.
  PFP ("Trying owm_first_alarmed()... ");
  slave_found = owm_first_alarmed ((uint8_t *) &rid);
  if ( ! slave_found ) {
    PFP ("no alarmed slaves found (usually ok, see source).\n");
  }
  else {
    PFP ("found alarmed slave (ID: ");
    print_slave_id (slave_rid);
    PFP (").\n");
  }

  // owm_verify() should work with either a single or multiple slaves.
  PFP ("Trying owm_verify() with previously discoved ID... ");
  slave_found = owm_verify ((uint8_t *) &rid);
  if ( ! slave_found ) {
    PFP ("failed: unexpectedly returned false");
    PFP_ASSERT (FALSE);
  }
  PFP ("ok, ID verified.\n");

  PFP ("Starting temperature conversion... ");
  // FIXME: the master should probably get some sort of
  // owm_start_command_transaction() or so which would take SKIP_ROM,
  // READ_ROM, or MATCH_ROM (and for this last one an ID), this would wrap
  // up the entire command-transaction initiation process as used by for
  // example the DS18B20.
  // NOTE: the DS18B20 doesn't seem to require an addressing command before
  // the "Convert T" command here, which is contrary to its own datasheet.
  // I guess if there's only one slave on the bus it works ok, but its sort of
  // weird, so we go ahead and do the addressing again.
  uint64_t slave_rid_2nd_reading = ds18b20_init_and_rom_command ();
  PFP_ASSERT (slave_rid_2nd_reading == slave_rid);
  owm_write_byte (DS18B20_COMMANDS_CONVERT_T_COMMAND);
  // The DS18B20 is now supposed to respond with a stream of 0 bits until
  // the conversion completes, after which it's supposed to send 1 bits.
  // Note that this is probably a typical behavior for busy one-wire slaves.
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

  // FIXME: well assert is a pretty lousy way to detect how tests fail,
  // might want to conditionally print something or so...

  PFP ("Trying owm_start_transaction() with READ_ROM... ");
  uint64_t slave_rid_3rd_reading;
  owm_error_t err = owm_start_transaction (
      OWC_READ_ROM_COMMAND,
      (uint8_t *) &slave_rid_3rd_reading,
      DS18B20_COMMANDS_CONVERT_T_COMMAND );
  PFP_ASSERT_NO_OWM_ERROR (err);
  PFP_ASSERT (err == OWM_ERROR_NONE);
  PFP_ASSERT (slave_rid_3rd_reading = slave_rid);
  conversion_complete = 0;
  while ( ! (conversion_complete = owm_read_bit ()) ) {
    ;
  }
  PFP ("seemed ok.\n");

  PFP ("Trying owm_start_transaction() with MATCH_ROM... ");
  err = owm_start_transaction (
      OWC_MATCH_ROM_COMMAND,
      (uint8_t *) &slave_rid,
      DS18B20_COMMANDS_CONVERT_T_COMMAND );
  PFP_ASSERT (err == OWM_ERROR_NONE);
  conversion_complete = 0;
  while ( ! (conversion_complete = owm_read_bit ()) ) {
    ;
  }
  PFP ("seemed ok.\n");

  PFP ("Trying owm_start_transaction() with SKIP_ROM... ");
  err = owm_start_transaction (
      OWC_SKIP_ROM_COMMAND,
      NULL,
      DS18B20_COMMANDS_CONVERT_T_COMMAND );
  PFP_ASSERT (err == OWM_ERROR_NONE);
  conversion_complete = 0;
  while ( ! (conversion_complete = owm_read_bit ()) ) {
    ;
  }
  PFP ("seemed ok.\n");

  // We can now read the slave scratchpad memory.  This requires us to first
  // perform the initialization and read rom commands again as described in
  // the DS18B20 datasheet.  The slave ROM code better be the same on second
  // reading :)
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
  uint64_t first_slave_id
    = __builtin_bswap64 (UINT64_C (OWM_FIRST_SLAVE_ID));
  uint64_t second_slave_id
    = __builtin_bswap64 (UINT64_C (OWM_SECOND_SLAVE_ID));

  uint64_t rid;   // ROM ID

  PFP ("Trying owm_first()... ");
  uint8_t slave_found = owm_first ((uint8_t *) &rid);
  if ( ! slave_found ) {
    PFP ("failed: didn't discover any slaves");
    PFP_ASSERT (FALSE);
  }
  if ( rid != first_slave_id ) {
    PFP ("failed: discovered first slave ID was ");
    print_slave_id (rid);
    PFP (", not the expected ");
    print_slave_id (first_slave_id);
    PFP_ASSERT (FALSE);
  }
  PFP ("ok, found 1st slave with expected ID ");
  print_slave_id (rid);
  PFP (".\n");

  PFP ("Trying owm_next()... ");
  slave_found = owm_next ((uint8_t *) &rid);
  if ( ! slave_found ) {
    PFP ("failed: didn't discover a second slave");
    PFP_ASSERT (FALSE);
  }
  if ( rid != second_slave_id ) {
    PFP ("failed: discovered second slave ID was ");
    print_slave_id (rid);
    PFP (", not the expected ");
    print_slave_id (second_slave_id);
    PFP_ASSERT (FALSE);
  }
  PFP ("ok, found 2nd slave with expected ID ");
  print_slave_id (rid);
  PFP (".\n");

  // Verify that owm_next() (following the owm_first() call above) returns
  // false, since there are only two slaves on the bus.
  PFP ("Trying owm_next() again... ");
  slave_found = owm_next ((uint8_t *) &rid);
  if ( slave_found ) {
    PFP ("failed: unexpectedly returned true");
    PFP_ASSERT (FALSE);
  }
  PFP ("ok, no next slave found.\n");

  PFP("Trying owm_verify(first_slave_id)... ");
  slave_found = owm_verify ((uint8_t *) &rid);
  if ( ! slave_found ) {
    PFP ("failed: didn't find slave");
    PFP_ASSERT (FALSE);
  }
  PFP ("ok, found it.\n");

  PFP("Trying owm_verify(second_slave_id)... ");
  slave_found = owm_verify ((uint8_t *) &rid);
  if ( ! slave_found ) {
    PFP ("failed: didn't find slave");
    PFP_ASSERT (FALSE);
  }
  PFP ("ok, found it.\n");

  PFP ("All tests passed.\n");
  PFP ("\n");

#endif

}
