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
// Depending on the USB to provide a common ground didn't work consistently
// for me with my laptop.  I had to add a physical wire connecting the
// Arduino grounds.  This is sort of weird but unlikely to be an issue in
// any real application (where its unlikely that both master and slave will
// even be Arduinos, let alone USB-powered ones).
//
// The slave Arduino should be reset first.  Timeouts are tested first,
// during which the master must be silent (maybe hold it's reset button
// down :).  Then the master should be reset when prompted.
//
// Because the slave needs to respond quickly to requests from the master,
// it can't take the time to provide incremental diagnostic output via
// term_io.h like other module tests do.  For the most part you only get
// output when there's a failure, and should look at the output on the
// master side to verify that it's talking to the slave.
//

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "debug_led.h"  // NOTE: only required for debugging this module
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
  DBL_INIT ();  // NOTE: only required for debugging this module

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
  PFP ("About to start timeout tests.  Ensure that the master is silent\n");
  double const mdtms = 2042;   // Message Display Time in ms FIXME: up for prod
  _delay_ms (mdtms);
  PFP ("Testing ows_wait_for_function_transaction() with minimum timeout... ");
  ows_set_timeout (OWS_MIN_TIMEOUT_US);
  result = ows_wait_for_function_transaction (&fcmd, 0);
  PFP_ASSERT (result = OWS_ERROR_TIMEOUT);
  PFP ("ok.\n");
  PFP ("Testing ows_wait_for_function_transaction() with maximum timeout... ");
  ows_set_timeout (OWS_MAX_TIMEOUT_US);
  result = ows_wait_for_function_transaction (&fcmd, 0);
  PFP_ASSERT (result = OWS_ERROR_TIMEOUT);
  PFP ("ok.\n");

  // NOTE: its also possible to cause many resets in a row, and time them with
  // a stopwatch to verify that the timeouts actually have the approximate
  // duration expected (at least for OWS_MAX_TIMEOUT_US anyway).  I've done
  // this, but I don't think it's worth automating it here.

  PFP ("\n");

  PFP ("Ready to start master-slave tests, reset the master now and look\n");
  PFP ("at it's output to verify correct operation.\n");

  // We're going to perform the remaining tests using the minimum timeout
  // setting, in order to exercise things: if everything works properly,
  // the master should still be able to communicate with us despite
  // regular timeouts and restarts of ows_wait_for_function_transaction()
  // (resulting from all the diagnostic output the the one_wire_master_test.c
  // program does).  In practice OWS_TIMEOUT_NONE could be used if the slave
  // only needs to do things on demand (and doesn't want to sleep), or some
  // value between the minimum OWS_MIN_TIMEOUT_US and OWS_MAX_TIMEOUT_US.
  // Of course, if the slave does anything time-consuming between
  // ows_wait_for_function_transaction() calls, the delay might get to be too
  // much for the master to tolerate without compensating code (it wouldn't
  // get presence pulses in time).
  ows_set_timeout (OWS_MIN_TIMEOUT_US);

  uint8_t jgur = FALSE;   // Gets set TRUE iff we Just Got an Unexpected Reset

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
      PFP_ASSERT_NOT_REACHED ();
    }

    if ( result == OWS_ERROR_GOT_UNEXPECTED_RESET ) {
      // This path gets a little exercise from the test code in
      // one_wire_master, because it starts out by just doing a reset pulse
      // and looking for a presence pulse, then starts over doing a more
      // complete transaction with another reset pulse, which as far as this
      // slave module is concerned constitues an unexpected reset.
      jgur = TRUE;
      continue;
    }
    else {
      jgur = FALSE;
    }

    if ( result != OWS_RESULT_SUCCESS ) {
      continue;
    }

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
        break;

      default:
        PFP_ASSERT_NOT_REACHED ();
        break;

    }

  }

}
