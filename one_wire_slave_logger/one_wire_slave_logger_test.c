// Test/demo for the one_wire_slave_logger.h interface.
//
// This program implements a 1-wire slave relay that copies messages
// received over 1-wire out using term_io.h.  Note that by supplying a
// different function pointer argument to owsl_init() messages can easily
// be relayed to a different interface or device.
//
// Physically, the test setup should consist of:
//
//   * one Arduino acting as the master, and set up as described in
//     one_wire_master_logger_test.c
//
//   * a second Arduino running this test program, connected to the first
//     Arduino via a data line (by default to OWS_PIN DIO_PIN_DIGITAL_2),
//     and a ground line
//
//   * a computer connected by USB to this second slave Arduino
//
// It should then be possible to run
//
//   make -rR run_screen
//
// from this module directory to view the test output and log messages from
// the master.
//
// Depending on the USB to provide a common ground didn't work consistently
// for me with my laptop.  I had to add a physical wire connecting the
// Arduino grounds.  This is sort of weird but unlikely to be an issue in
// any real application (where its unlikely that both master and slave will
// even be Arduinos, let alone USB-powered ones).
//
// The slave Arduino should be reset first so its ready to receive messages
// from the master.
//

#include "one_wire_slave_logger.h"
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

// See the definition of this macro in util.h to understand why its here.
WATCHDOG_TIMER_MCUSR_MANTRA

// FIXME: we wont need this, better to tell user to just go run the
// one_wire_slave tests if they get any error return out of init I think.
#define OWS_CHECK(result)                                        \
  PFP_ASSERT_SUCCESS (result, ows_result_as_string, result_buf);

int
main (void)
{
  // This isn't what we're testing exactly, but we need to know if it's
  // working or not to interpret other results.

  // NOTE: in this case we're using term_io both for the output of this test
  // program, and for message handling via the relay_via_term_io callback.
  // This call would therefore be required for other similar (non-test)
  // programs which use this callback to handle messages.
  term_io_init ();
  PFP ("\n");
  PFP ("\n");
  PFP ("term_io_init() worked.\n");
  PFP ("\n");

  PFP ("Trying owsl_init()... ");
  PFP ("\n");
  // Initialize the interface.  Note that in this case
  owsl_init (owsl_relay_via_term_io);
  // Use this if you want to use an ID that you've loaded into EEPROM:
  //ows_init (TRUE);   // Initialize the 1-wire interface slave end
  // FIXME: ultimately we probably want _init and the listener function to
  // be seperate.  As it is owsl_init() doesn't return so we dont see this:
  PFP ("ok, it returned, we should be relaying messages now...\n");

}
