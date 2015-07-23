// Test/demo for the one_wire_master_logger.h interface.
//
// This program implements a 1-wire master that sends messages out over a
// 1-wire bus.  These message can then be picked up and handled by a second
// Arduino providing a one_wire_slave_logger.h interface.
//
// Physically, the test setup should consist of:
//
//   * one Arduino running this test program acting as the master.  To see
//     debugging output from this Arduino, it may be connected to a computer
//     via USB, though of course in a real system if you can do that you
//     probably wouldn't need this module...
//
//   * a second Arduino running the one_wire_slave_logger_test.c test program,
//     connected to the first Arduino via a data line (by default to OWS_PIN
//     DIO_PIN_DIGITAL_2), and a ground line
//
//   * a computer connected by USB to this second slave Arduino
//
// It should then be possible to run
//
//   make -rR run_screen
//
// from the module directory for the second Arduino to view the messages
// messages from the master.
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

#include "one_wire_master_logger.h"
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

// See the definition of this macro in util.h to understand why its here.
WATCHDOG_TIMER_MCUSR_MANTRA

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

  PFP ("git description: " EXPAND_AND_STRINGIFY (VERSION_CONTROL_COMMIT) "\n");
  PFP ("git description: " EXPAND_AND_STRINGIFY (GIT_DESCRIPTION) "\n");

  PFP ("Trying dowm_init()... ");
  // Initialize the interface, using the OWS_DEFAULT_PART_ID
  dowm_init ();
  // Use this if you want to use an ID that you've loaded into EEPROM:
  //ows_init (TRUE);   // Initialize the 1-wire interface slave end
  PFP ("ok, it returned.\n");
  PFP ("\n");

  uint32_t tmn = 1;   // Test Message Number
  for ( ; ; ) {
    PFP ("About to send message \"Message %" PRIu32 "\"...", tmn);
    dowm_printf ("Message %" PRIu32 "\n", tmn);
    PFP (" sent and acknowledge received.\n");
    tmn++;
  }
}
