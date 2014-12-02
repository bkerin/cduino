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

  PFP ("Trying ows_init()... ");
  // Initialize the interface, using the OWS_DEFAULT_PART_ID
  ows_init (FALSE);
  // Use this if you want to use an ID that you've loaded into EEPROM:
  //ows_init (TRUE);   // Initialize the one-wire interface slave end
  PFP ("ok, it returned.\n");

  // FIXME: This command loop part is having some problems, though I think
  // it mostlyworks
  PFP ("Ready to honor a READ_ROM_COMMAND... ");
  uint8_t command = ows_wait_for_command ();
  assert (command == OWS_READ_ROM_COMMAND);
  ows_error_t err = ows_write_rom_id ();
  PFP ("cp-1\n");
  assert (err == OWS_ERROR_NONE);

  /*
  for ( uint8_t ii = 0 ; ii < 8 ; ii++ ) {
    uint8_t fake_byte = 0x28;
    ows_write_byte (fake_byte);
  }
  */

  PFP ("cp0\n");


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
