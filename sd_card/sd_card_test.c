// Test/demo for the sd_card.h interface.
//
// This test driver requires an Arduino SD Card/Ethernet shield
// (http://arduino.cc/en/Main/ArduinoEthernetShield) with an SD card that
// supports all the tested features to be connected.  The author has tested
// things only with the Rev. 3 version of the above shield and an SDHC type
// SD card (as opposed to SD1 or SD2 type).
//
// Diagnostic output is produced on an attached terminal using the term_io.h
// interface.
//
// WARNING: despite being ubiquitous, many SD cards are utter junk.
// They lack any underlying wear leveling for the flash memory and are
// horribly intolerant of asynchronous shutdown (power cuts).  If you're
// doing anything remotely serious you must invest in an "industrial"
// SD card.  I've used the Apacer AP-MSD04GCS4P-1TM with good results.

#include <assert.h>
#include <avr/pgmspace.h>
// FIXME: do we need stdlib here once we have the new avr libc which has
// the fixed assert.h?  I doubt it but it needs checked..
#include <stdlib.h>

#include "sd_card.h"
#include "term_io.h"

// This test program can actually run the ATMega328P out of RAM (at least
// I think that's whats going on), so we need to store our output strings
// in program space to have enough memory.  This macro makes that a little
// more graceful.
#ifndef __GNUC__
#  error GNU C is required by the comma-swallowing macro
#endif
#define PFP(format, ...) printf_P (PSTR (format), ## __VA_ARGS__)

#ifndef SD_CARD_BUILD_ERROR_DESCRIPTION_FUNCTION
#  error This test program requires SD_CARD_BUILD_ERROR_DESCRIPTION_FUNCTION
#endif

static void
check_maybe_print_possible_failure_message (uint8_t code)
{
  // Check that code is 0, if it isn't, print a message describing the error
  // returned by sd_card_last_error(), followed by a newline.

  if ( ! code ) {
    sd_card_error_t last_error = sd_card_last_error ();
    char err_buf[SD_CARD_ERROR_DESCRIPTION_MAX_LENGTH + 1];
    sd_card_error_description (last_error, err_buf);
    PFP ("failed: %s\n", err_buf);
  }
}

static void
test_write_read (void)
{
  // Fill block 42 with 42s, then read them back out.

  uint32_t bn = 42;
  uint8_t data_block[SD_CARD_BLOCK_SIZE];
  uint16_t ii;
  for ( ii = 0 ; ii < SD_CARD_BLOCK_SIZE ; ii++ ) {
    data_block[ii] = 42;
  }

  PFP ("Trying sd_card_write_block()... ");
  uint8_t return_code = sd_card_write_block (bn, data_block);
  check_maybe_print_possible_failure_message (return_code);
  assert (return_code);
  PFP ("ok.\n");

  uint8_t reread_data[SD_CARD_BLOCK_SIZE];
  PFP ("Trying sd_card_read_block()... ");
  return_code = sd_card_read_block (bn, reread_data);
  check_maybe_print_possible_failure_message (return_code);
  assert (return_code);
  for ( ii = 0 ; ii < SD_CARD_BLOCK_SIZE ; ii++ ) {
    if ( reread_data[ii] != 42 ) {
      PFP ("failed: didn't read expected value");
      assert (0);
    }
  }
  PFP ("ok.\n");
 
  // Re-zero the reread data buffer to give the next tests a better chance
  // of catching problems.
  for ( ii = 0 ; ii < SD_CARD_BLOCK_SIZE ; ii++ ) {
    reread_data[ii] = 0;
  }

  PFP ("Trying sd_card_write_partial_block()... ");
  uint16_t const pbbc = 42;   // Partial block byte count
  return_code = sd_card_write_partial_block (bn, pbbc, data_block);
  check_maybe_print_possible_failure_message (return_code);
  assert (return_code);
  PFP ("ok.\n");

  PFP ("Trying sd_card_read_partial_block()... ");
  return_code = sd_card_read_partial_block (bn, pbbc, reread_data);
  check_maybe_print_possible_failure_message (return_code);
  assert (return_code);
  for ( ii = 0 ; ii < pbbc ; ii++ ) {
    if ( reread_data[ii] != 42 ) {
      PFP ("failed: didn't read expected value");
      assert (0);
    }
  }
  PFP ("ok.\n");
}

static void
speed_test_1000_blocks (void)
{
  // Write and then read back in 1000 blocks, to give an idea of speed.

  uint8_t data_block[SD_CARD_BLOCK_SIZE];
  for ( int ii = 0 ; ii < SD_CARD_BLOCK_SIZE ; ii++ ) {
    data_block[ii] = 42;
  }

  PFP ("Speed test: writing 1000 blocks... ");
  for ( uint32_t ii = 0 ; ii < 1000 ; ii++ ) {
    uint8_t return_code = sd_card_write_block (ii + 1, data_block);
    check_maybe_print_possible_failure_message (return_code);
    assert (return_code);
  }
  PFP ("done.\n");

  PFP ("Speed test: reading 1000 blocks... ");
  for ( uint32_t ii = 0 ; ii < 1000 ; ii++ ) {
    uint8_t return_code = sd_card_read_block (ii + 1, data_block);
    check_maybe_print_possible_failure_message (return_code);
    assert (return_code);
    // Here we double check that we're getting back the correct values,
    // which makes the speed test take slightly longer, but its not going
    // to be much compared to the read itself at hight F_CPU at least.
    for ( int ii = 0 ; ii < SD_CARD_BLOCK_SIZE ; ii++ ) {
      if ( data_block[ii] != 42 ) {
        PFP ("failed: didn't read expected value");
        assert (0);
      }
    }
  }
  PFP ("done.\n");
}

static void
per_speed_tests (sd_card_spi_speed_t speed, char const *speed_string)
{
  // Perform the various tests that we try for each speed setting.

  PFP ("Trying sd_card_init (%s)... ", speed_string);
  uint8_t return_code = sd_card_init (speed);
  check_maybe_print_possible_failure_message (return_code);
  assert (return_code);
  PFP ("ok.\n");

  PFP ("Trying sd_card_size ()... ");
  uint32_t card_size = sd_card_size ();
  if ( card_size == 0 ) {
    check_maybe_print_possible_failure_message (return_code);
    assert (0);
  }
  else {
    PFP ("ok, card_size: %lu\n", card_size);
  }

  PFP ("Trying sd_card_type()... ");
  sd_card_type_t card_type = sd_card_type ();
  PFP ("got card type ");
  switch ( card_type ) {
    case SD_CARD_TYPE_INDETERMINATE:
      PFP ("indeterminate.\n");
      break;
    case SD_CARD_TYPE_SD1:
      PFP ("SD1.\n");
      PFP ( 
          "SD1 type cards haven't been tested (only SDHC cards have).\n"
          "Disable this trap and try it :)  Other tests that don't work\n"
          "for this card type might also need to be disabled.\n" );
      assert (0);   // SD1 type cards haven't been tested
      break;
    case SD_CARD_TYPE_SD2:
      PFP ("SD2.\n");
      PFP (
          "SD2 type cards haven't been tested (only SDHC cards have).\n"
          "Disable this trap and try it :)  Other tests that don't work\n"
          "for this card type might also need to be disabled.\n" );
      assert (0);   // SD2 type cards haven't been tested
      break;
    case SD_CARD_TYPE_SDHC:
      PFP ("SDHC.\n");
      break;
    default:
      assert (0);   // Shouldn't be here
      break;
  }

  PFP ("Trying sd_card_read_cid()... ");
  sd_card_cid_t ccid;   // Card CID
  return_code = sd_card_read_cid (&ccid);
  if ( return_code ) {
    PFP ("returned TRUE, so presumably it worked.\n");
  }
  else {
    check_maybe_print_possible_failure_message (return_code);
    assert (0);
  }

  PFP ("Trying sd_card_read_csd()... ");
  sd_card_csd_t ccsd;   // Card CSD
  return_code = sd_card_read_csd (&ccsd);
  if ( return_code ) {
    PFP ("returned TRUE, so presumably it worked.\n");
  }
  else {
    check_maybe_print_possible_failure_message (return_code);
    assert (0);
  }

  test_write_read ();

  PFP ("Trying sd_card_single_block_erase_supported()... ");
  uint8_t result = sd_card_single_block_erase_supported ();
  if ( result ) {
    PFP ("ok, it's supported.\n");
    PFP ("Trying sd_card_erase_blocks (42, 43)... ");
    return_code = sd_card_erase_blocks (42, 43);
    if ( return_code ) {
      PFP ("ok.\n");
    }
    else {
      check_maybe_print_possible_failure_message (return_code);
      assert (0);
    }
  }
  else {
    PFP ("it's not supported.\n");
    assert (0);
  }

  speed_test_1000_blocks ();

  PFP ("Everything worked with %s\n", speed_string);
}

int
main (void)
{
  // This isn't what we're testing exactly, but we need to know if its
  // working or not to interpret other results.
  term_io_init ();
  PFP ("\n");
  PFP ("\n");
  PFP ("term_io_init() worked.\n");
  PFP ("\n");

  PFP (
      "NOTE: some tests don't bother to call sd_card_last_error() when\n"
      "things go wrong.  You might be able to get information about the\n"
      "nature of a failure by doing that.\n" );
  PFP ("\n");

  per_speed_tests (SD_CARD_SPI_SPEED_FULL, "SD_CARD_SPI_SPEED_FULL");
  PFP ("\n");

  per_speed_tests (SD_CARD_SPI_SPEED_HALF, "SD_CARD_SPI_SPEED_HALF");
  PFP ("\n");

  per_speed_tests (SD_CARD_SPI_SPEED_QUARTER, "SD_CARD_SPI_SPEED_QUARTER");
  PFP ("\n");

  PFP ("Everything worked!\n"); 
  PFP ("\n");
}
