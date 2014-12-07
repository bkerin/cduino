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
// in util.h for details about the failure feedback.  If everthing works
// here, you might want to take a look at the diagnostic output from the
// master to verify that the final results got to it correctly.
//

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "dio.h"
#include "ds18b20_commands.h"
#include "one_wire_slave.h"
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

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
// wrapper to the function to automatically add the __FILE__ and __LINE__
// arguments.
#define BASSERT_SPE(cond) \
  bassert_feeding_wdt_show_point ((cond), __FILE__, __LINE__)

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

  ows_error_t err;

  err = ows_write_byte (t_lsb);
  BASSERT_SPE (err == OWS_ERROR_NONE);
  err = ows_write_byte (t_msb);
  BASSERT_SPE (err == OWS_ERROR_NONE);

  // Remaining Scratchpad Size (after temp. bytes).  This is a property of
  // the DS18B20.
  uint8_t const rss = 7;

  // The test program that supposed to be running on the master doesn't care
  // about anything except the temperature bytes, so we just send 0 for the
  // remaining bytes.
  for ( uint8_t ii = 0 ; ii < rss ; ii++ ) {
    err = ows_write_byte (0);
    BASSERT_SPE (err == OWS_ERROR_NONE);
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

  PFP ("Trying ows_init()... ");
  // Initialize the interface, using the OWS_DEFAULT_PART_ID
  ows_init (FALSE);
  // Use this if you want to use an ID that you've loaded into EEPROM:
  //ows_init (TRUE);   // Initialize the one-wire interface slave end
  PFP ("ok, it returned.\n");

  PFP ("Ready to start tests, reset the master now\n");

  for ( ; ; ) {

    uint8_t fcmd = ows_wait_for_function_command ();

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
        // since it makes it essentially throws an annoying gotcha into it
        // if its being used as a demo program?  In which case the text at
        // the top of this file would need to change
        BTRAP ();
        break;

      default:
        BASSERT_SPE (0);   // Shouldn't be here unless the master screwed up
        break;

    }

    // FIXME: we promised above to end with rapid blinking, so this dispatch
    // versin should do that as well somehow, or we shouldn't promise it
  }

  uint8_t command = ows_wait_for_command ();
  BASSERT_SPE (command == OWC_READ_ROM_COMMAND);
  ows_error_t err = ows_write_rom_id ();
  BASSERT_SPE (err == OWS_ERROR_NONE);
  command = ows_wait_for_command ();
  BASSERT_SPE (command == OWC_READ_ROM_COMMAND);
  err = ows_write_rom_id ();
  BASSERT_SPE (err == OWS_ERROR_NONE);

  // Next we expect a SEARCH_ROM command from the master (because it calls
  // owm_first()).
  command = ows_wait_for_command ();
  BASSERT_SPE (command == OWC_SEARCH_ROM_COMMAND);
  err = ows_answer_search ();
  BASSERT_SPE (err == OWS_ERROR_NONE);

  // Next we expect another SEARCH_ROM command from the master (because it
  // calls owm_next()).
  command = ows_wait_for_command ();
  BASSERT_SPE (command == OWC_SEARCH_ROM_COMMAND);
  err = ows_answer_search ();
  BASSERT_SPE (err == OWS_ERROR_NONE);

  // Next we expect a verify command, then a convert temperature command from
  // the master (the verify is required as part of the transaction sequence
  // (see the DS18B20 datasheet "Transaction Sequence" section).
  command = ows_wait_for_command ();
  BASSERT_SPE (command == DS18B20_COMMANDS_CONVERT_T_COMMAND);
  // A real Maxim DS18B20 would take some time here to digitize the
  // temperature measurement.  If it wasn't using parasite power, it would
  // during that time respond to read bit commands from the master by
  // sending zeros.  Fortunately we're just making up a number, which is
  // instantaneous :) But seriously, this shows a weak point in our slave
  // implementation: we aren't set up to send zeros and do anything else
  // useful at the same time.  In theory the ISR in one_wire_slave.c could be
  // programmed to automatically send zeros when requested (perhaps with an
  // additional timer compare match interrupt or something to time the zero
  // pulse being sent), but I think its not worth dealing with this.  If you
  // really want DS18B20 behavior use a real one, otherwise just specify
  // your own requirement for masters that want to interrogate your device.
  // For example: "after issuing a convert_t_command, the master shall wait
  // 0.42 ms for the conversion to complete before beginning any additional
  // transaction sequence with the same slave".  Another possible strategy
  // is to have your slave continually make conversions on its own (possibly
  // at a cost in responsiveness to reset pulses, see the comments near the
  // ows_wait_for_reset() declaration in one_wire_slave.h), in which case
  // the results of recent measurements should be immediately available
  // on request, and can be returned as part of the same transaction.
  // Or you could try using a real-time OS instead.  Or you could just add
  // one more microcontroller and have it communicate the recent results
  // of it's measurements to the processor using this module on IO pins,
  // so they are always immediately available :)
  ows_write_bit (1);

  // FIXME: WORK POINT: master does another ds18b20_init_and_rom_command in
  // here, wonder if that's actually required?

  // Next we expect a READ_SCRATCHPAD transaction sequence, in this case
  // initiated with a READ_ROM command.
  command = ows_wait_for_command ();
  BASSERT_SPE (command == OWC_READ_ROM_COMMAND);
  err = ows_write_rom_id ();
  BASSERT_SPE (err == OWS_ERROR_NONE);
  command = ows_wait_for_command ();
  BASSERT_SPE (command == DS18B20_COMMANDS_READ_SCRATCHPAD_COMMAND);
  send_fake_ds18b20_scratchpad_contents ();

  // Made it through, so start rapid blinking as promised!
  BTRAP ();
}
