// Test/demo for the one_wire_slave.h interface.
//
// This program implements a simple 1-wire slave device.  It acts a bit like
// a Maxim DS18B20, but the temperature is always about 42.42 degrees C :)
// There are some other slightly eccentric features of these tests, since
// they are designed to account for the expectations of the test program
// one_wire_master_test.c (from the one_wire_master module).
//
// Physically, the test setup should consist of:
//
//   * one Arduino acting as the master, and set up as described in
//     one_wire_master_test.c, but with the actual DS18B20 removed, and
//
//   * a second Arduino running this test program, connected to the first
//     Arduino via a data line (by default to OWS_PIN DIO_PIN_DIGITAL_2),
//     and a ground line.
//
// Depending on the USB to provide a common ground didn't work for me with
// my laptop.  I had to add a physical wire connecting the Arduino grounds.
// This is sort of weird but unlikely to be an issue in any real application
// (where its unlikely that both master and slave will even be Arduinos,
// let alone USB-powered ones).
//
// The slave Arduino should be reset first, then the master should be reset
// when prompted to kick off the tests.
//
// Because the slave needs to respond quickly to requests from the master,
// it can't take the time to provide incremental diagnostic output via
// term_io.h like other module tests do.  Instead, it will give you some
// nice blinky-LED feedback about the point of failure on the on-board LED
// (connected to PB5, aka DIGITAL_5) , or simple enthusiastic rapid blinking
// if everything works :) See the coments for BASSERT_FEEDING_WDT_SHOW_POINT()
// in util.h for details about the failure feedback.  If everthing works
// here, you might want to take a look at the diagnostic output from the
// master to verify that the final results got to it correctly.
//

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "debug_led.h"  // FIXME: remove when done, I think
#include "dio.h"
#include "ds18b20_commands.h"
#include "one_wire_slave.h"
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

char result_buf[OWS_RESULT_DESCRIPTION_MAX_LENGTH + 1];

#define OWS_CHECK(result)                                        \
  PFP_ASSERT_SUCCESS (result, ows_result_as_string, result_buf);

// FIXME: this should be ows_result_as_string as in owm module
static void
print_ows_error (ows_result_t result)
{
  switch ( result ) {
    case OWS_RESULT_SUCCESS:
      PFP ("OWS_RESULT_SUCCESS");
      break;
    case OWS_ERROR_TIMEOUT:
      PFP ("OWS_ERROR_TIMEOUT");
      break;
    case OWS_ERROR_GOT_UNEXPECTED_RESET:
      PFP ("OWS_ERROR_GOT_UNEXPECTED_RESET");
      break;
    case OWS_ERROR_GOT_INVALID_ROM_COMMAND:
      PFP ("OWS_ERROR_GOT_INVALID_ROM_COMMAND");
      break;
    case OWS_ERROR_ROM_ID_MISMATCH:
      PFP ("OWS_ERROR_ROM_ID_MISMATCH");
      break;
    default:
      assert (FALSE);   // Shouldn't be here
      break;
  }
}

static void
send_fake_ds18b20_scratchpad_contents (void)
{
  // Send the appropriate response to a read scratchpad command from the
  // master, at least as far as the one_wire_master_test.c program cares.
  // Note in particular that one_wire_master_test.c doesn't even care about
  // the CRC that a real DS18B20 would send.  FIXME: maybe we should make
  // the master actually check this since it would be a good idea to show
  // how to do that anyway. (and compute it at this end)

  // Temperature least and most significant bits st the temperature comes
  // out to 42.0 degrees C (see Fig. 2 of Maxim DS18B20 datasheet).
  uint8_t const t_lsb = B10100000;
  uint8_t const t_msb = B00000010;

  OWS_CHECK (ows_write_byte (t_lsb));
  OWS_CHECK (ows_write_byte (t_msb));

  // Remaining Scratchpad Size (after temp. bytes).  This is a property of
  // the DS18B20.
  uint8_t const rss = 7;

  // The test program that supposed to be running on the master doesn't care
  // about anything except the temperature bytes, so we just send 0 for the
  // remaining bytes.
  for ( uint8_t ii = 0 ; ii < rss ; ii++ ) {
    OWS_CHECK (ows_write_byte (0));
  }
}

int
main (void)
{
  DBL_INIT ();  // FIXME: remove when done, I think

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
  //ows_init (TRUE);   // Initialize the 1-wire interface slave end
  PFP ("ok, it returned.\n");

  // The one_wire_master_test.c program does a search for alarmed slaves.
  // Uncomment this line to cause this slave to consider itself alarmed :)
  ows_alarm = 42;

  uint8_t fcmd;         // Function Command
  ows_result_t result;   // Storage for function "results" (exit codes anyway)

  PFP ("\n");

  // FIXME: timeout tests disabled for now
  /*
  PFP ("About to start timeout tests.  Ensure that the master is silent\n");
  double const mdtms = 2042;   // Message Display Time in ms FIXME: up for prod
  _delay_ms (mdtms);
  PFP ("Testing ows_wait_for_function_transaction() with minimum timeout... ");
  timeout = 1;   // Minimum setting
  result = ows_wait_for_function_transaction (&fcmd, timeout);
  PFP_ASSERT (result = OWS_ERROR_TIMEOUT);
  PFP ("ok.\n");
  PFP ("Testing ows_wait_for_function_transaction() with maximum timeout... ");
  timeout = OWS_MAX_TIMEOUT_US;
  result = ows_wait_for_function_transaction (&fcmd, timeout);
  PFP_ASSERT (result = OWS_ERROR_TIMEOUT);
  PFP ("ok.\n");
  */

  PFP ("\n");

  PFP ("Ready to start master-slave tests, reset the master now\n");

  // We're going to perform the remaining tests using the minimum timeout
  // setting, in order to exercise things: if everything works properly,
  // the master should still be able to communicate with us despite
  // regular timeouts and restarts of ows_wait_for_function_transaction().
  // In practice OWS_TIMEOUT_NONE could be used if the slave only needs to
  // do things on demand (and doesn't want to sleep), or some value between
  // the minimum (1) and OWS_MAX_TIMEOUT_US.  Of course, if the slave does
  // anything time-consuming between ows_wait_for_function_transaction()
  // calls, the delay might get to be to much for the master to tolerate
  // without compensating code (it wouldn't get presence pulses in time).
  // FIXME: so maybe actually set it to the minimum.
  // FIXME: do the min and max timeout values still make sense?
  //ows_set_timeout (32767);
  ows_set_timeout (OWS_TIMEOUT_NONE);

  // FIXME: debug
  uint32_t lc = 0;   // FIXME: for testing timeout time correctness only

  uint8_t jgur = FALSE;

  for ( ; ; ) {

    result = ows_wait_for_function_transaction (&fcmd, jgur);

    if ( result != OWS_RESULT_SUCCESS                       &&
         result != OWS_ERROR_TIMEOUT &&
         result != OWS_ERROR_GOT_UNEXPECTED_RESET ) {
      // For diagnostic purposes we do this.  Normally printing something
      // out at this point might take too much time that could otherwise be
      // spent eating the error and waiting for the line to sort itself out :)
      PFP ("\n");
      PFP ("Unexpected ows_wait_for_function_transaction() result: ");
      print_ows_error (result);
      PFP ("\n");
      continue;
    }

    if ( result == OWS_ERROR_GOT_UNEXPECTED_RESET ) {
      // FIXME: because of the text that the slave outputs between things,
      // this path isn't really tested.
      jgur = TRUE;
      continue;
    }

    jgur = FALSE;

    if ( result != OWS_RESULT_SUCCESS ) {
      continue;
    }

    //printf ("got function command %hhx\n", fcmd);

    switch ( fcmd ) {

      case DS18B20_COMMANDS_CONVERT_T_COMMAND:
        // Because we're just making up a number, we convert instantly, so we
        // can immediately send the one that the DS18B20 sends when it's done
        // converting :) See the comment below near the next reference to the
        // DS18B20_CONVERT_T_COMMAND below for a longer discussion about this.
        ows_write_bit (1);
        break;

      case DS18B20_COMMANDS_READ_SCRATCHPAD_COMMAND:
        send_fake_ds18b20_scratchpad_contents ();
        // FIXME: Maybe it doesn't make sense for this version to blink,
        // since it makes it essentially throws an annoying gotcha into it if
        // its being used as a demo program?  In which case the text at the
        // top of this file would need to change.  At the moment it doesn't
        // blink in case we're twiddling the master, that can sometimes make
        // life easier.
        break;

      default:
        // FIXME: this should maybe still be here but we shouldn't end up
        // in switch if we got a timeout above.
        // FIXME: should this be assert FALSE?
        DBL_TRAP ();
        assert (FALSE);
        break;

    }

    lc++;  // FIXME: debug
    if ( lc == 1000 ) { printf ("got 1000 timeouts\n"); }  // FIXME: debug

  }

  // Made it through, so start rapid blinking as promised!
  //DBL_TRAP ();
}
