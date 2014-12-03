// Test/demo for the one_wire_slave.h interface.
//
// This program implements a simple one-wire slave device.  It acts a bit
// like a Maxim DS18B20, but the temperature is always about 42.42 degrees
// C :) There are some other slightly eccentric features of these tests,
// since they are designed to account for the expectations of the test
// program one_wire_master_test.c (from the one_wire_master module).
//
// Physically, the test setup should consist of:
//
//   * one Arduino acting as the master, and set up as described in
//     one_wire_master_test.c, but with the actual DS18B20 removed, and
//
//   * a second Arduino running this test program, connected to the first
//     Arduino via a single wire (by default to OWS_PIN DIO_PIN_DIGITAL_2),
//     and
//
//   * A common ground provided through the USB system, i.e. both Arduinos
//     should be plugged into the same USB hub :)
//
// The slave Arduino should be reset first, then the master should be reset
// to kick off the tests.
//
// Because the slave needs to respond quickly to requests from the master,
// it can't take the time to provide incremental diagnostic output via
// term_io.h like other module tests do.  Instead, it will give you some
// nice blinky-LED feedback about the point of failure on the on-board LED
// (connected to PB5, aka DIGITAL_5) , or simple enthusiastic rapid blinking
// if everything works :) See the coments for BASSERT_FEEDING_WDT_SHOW_POINT()
// in util.h for details.
//

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

// This is like BASERT_FEEDING_WDT_SHOW_POINT() from util.h, but it doesn't
// wast a huge blob of code space every use.  FIXME: it would be nice if
// util supplied an efficient version like this, but it would have to stop
// being a header-only module.
static void
bassert_feeding_wdt_show_point (uint8_t condition, char const *file, int line)
{
  if ( UNLIKELY (! (condition)) ) {
    for ( ; ; ) {
      size_t XxX_fnl = strlen (file);
      BLINK_OUT_UINT32_FEEDING_WDT (XxX_fnl);
      BLINK_OUT_UINT32_FEEDING_WDT (line);
    }
  }
}

// Blinky-assert, Showing Point Efficiently.  This does a blinky-assert
// showing the failure point in a code-space efficient way.  We need this
// wrapper to the function to conveniently add the correct __FILE__ and
// __LINE__ arguments.
#define BASSERT_SPE(cond) \
  bassert_feeding_wdt_show_point ((cond), __FILE__, __LINE__)

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

  PFP ("Trying ows_init()... ");
  // Initialize the interface, using the OWS_DEFAULT_PART_ID
  ows_init (FALSE);
  // Use this if you want to use an ID that you've loaded into EEPROM:
  //ows_init (TRUE);   // Initialize the one-wire interface slave end
  PFP ("ok, it returned.\n");

  PFP ("Ready to honor two READ_ROM_COMMANDs... ");
  uint8_t command = ows_wait_for_command ();
  BASSERT_SPE (command == OWS_READ_ROM_COMMAND);
  ows_error_t err = ows_write_rom_id ();
  BASSERT_SPE (err == OWS_ERROR_NONE);
  command = ows_wait_for_command ();
  BASSERT_SPE (command == OWS_READ_ROM_COMMAND);
  err = ows_write_rom_id ();
  BASSERT_SPE (err == OWS_ERROR_NONE);
  PFP ("ok, got them (and sent IDs in response).");

  // Next we expect a SEARCH_ROM command from the master (because it calls
  // owm_first()).
  command = ows_wait_for_command ();
  BASSERT_SPE (command == OWS_SEARCH_ROM_COMMAND);
  // FIXME: WORK POINT: test me
  BTRAP ();
  err = ows_answer_search ();
  BASSERT_SPE (err == OWS_ERROR_NONE);

  // Next we expect another SEARCH_ROM command from the master (because it
  // calls owm_next()).
  command = ows_wait_for_command ();
  BASSERT_SPE (command == OWS_SEARCH_ROM_COMMAND);
  err = ows_answer_search ();
  BASSERT_SPE (err == OWS_ERROR_NONE);

  // FIXME: next the master does owm_verify()...

  /*
  // Because the one-wire protocol doesn't allow us to take a bunch of time
  // out to send things, we accumulate incremental test results in these
  // variables then output everything at once.  Of course, some of the
  // one-wire slave functions might block forever if things aren't working
  // right, in which case more detailed diagnostics might need to be inserted.
  // These variables stand for Got Reset Pulse Sent Presence Pulse, Handled
  // Extra Reset Pulse, and Got Read Rom Command.
  uint8_t grpspp = FALSE, herp = FALSE, grrc = FALSE;

  PFP ("Trying wait-for-reset, presence-pulse, command sequence... ");

  ows_wait_for_reset ();

  grpspp = TRUE;

  uint8_t command;

  for ( ; ; ) {

    ows_error_t err = ows_read_byte (&command);

    if ( err == OWS_ERROR_NONE ) {
      break;
    }
    else if ( err == OWS_ERROR_UNEXPECTED_PULSE_LENGTH ) {
      PFP ("upl!\n");
    }
    else {
      // It so happens that our code in one_wire_master/one_wire_master_test.c
      // takes advantage of the ability of the DS18B20 to correctly handle
      // an additional reset request, and sends us one here.  In general,
      // masters might send reset requests at any time, and its nice to
      // honor them if possible, so here we test our capacity to do so a bit.
      if ( err == OWS_ERROR_RESET_DETECTED_AND_HANDLED ) {
        herp = TRUE;
      }
    }
  }

  if ( command == OWS_READ_ROM_COMMAND ) {
    grrc = TRUE;
  }
  PFP ("  Results:\n");
  if ( grpspp ) {
    PFP ("    Got reset pulse (and sent presence pulse)\n");
  }
  if ( herp ) {
    PFP ("    Got extra reset pulse (and sent another presence pulse)\n");
  }
  if ( grrc ) {
    PFP ("    Got READ_ROM command\n");
  }
  PFP ("Got byte %" PRIx8 "\n", command);
  */

}
