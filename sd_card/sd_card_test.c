// Test/demo for the sd_card.h interface.
//
// This test driver requires an Arduino SD Card/Ethernet shield
// (http://arduino.cc/en/Main/ArduinoEthernetShield) with an SD card that
// supports all the tested features to be connected.  The author has tested
// things only with the Rev. 3 version of the above shield and an SDHC type
// SD card (as opposed to SD1 or SD2 type).
//
// WARNING: despite being ubiquitous, many SD cards are utter junk.
// They lack any underlying wear leveling for the flash memory and are
// horribly intolerant of asynchronous shutdown (power cuts).  If you're
// doing anything remotely serious you must invest in an "industrial"
// SD card.  I've used the Apacer AP-MSD04GCS4P-1TM with good results.

#include <assert.h>
#include <stdlib.h>

#include "sd_card.h"
#include "term_io.h"

#define SD_CARD_BLOCK_SIZE 512

static void
write_read_42s_at_block_42 (void)
{
  // Fill block 42 with 42s, then read them back out.

  uint32_t bn = 42;
  uint8_t data_block[SD_CARD_BLOCK_SIZE];
  int ii;
  for ( ii = 0 ; ii < SD_CARD_BLOCK_SIZE ; ii++ ) {
    data_block[ii] = 42;
  }
  uint8_t return_code = sd_card_write_block (bn, data_block);
  assert (return_code);

  uint8_t reread_data[SD_CARD_BLOCK_SIZE];
  return_code = sd_card_read_block (bn, reread_data);
  assert (return_code);

  for ( ii = 0 ; ii < SD_CARD_BLOCK_SIZE ; ii++ ) {
    assert (reread_data[ii] == 42);
  }
}

static void
speed_test_1000_blocks (void)
{
  // Write and then read back in 1000 blocks, to give an idea of speed.

  uint8_t data_block[SD_CARD_BLOCK_SIZE];
  for ( int ii = 0 ; ii < SD_CARD_BLOCK_SIZE ; ii++ ) {
    data_block[ii] = 42;
  }

  printf ("Speed test: writing 1000 blocks... ");
  for ( uint32_t ii = 0 ; ii < 1000 ; ii++ ) {
    uint8_t return_code = sd_card_write_block (ii + 1, data_block);
    assert (return_code);
  }
  printf ("done.\n");

  printf ("Speed test: reading 1000 blocks... ");
  for ( uint32_t ii = 0 ; ii < 1000 ; ii++ ) {
    uint8_t return_code = sd_card_read_block (ii + 1, data_block);
    assert (return_code);
    // Here we double check that we're getting back the correct values,
    // which makes the speed test take slightly longer, but its not going
    // to be much compared to the read itself at hight F_CPU at least.
    for ( int ii = 0 ; ii < SD_CARD_BLOCK_SIZE ; ii++ ) {
      assert (data_block[ii] == 42);
    }
  }
  printf ("done.\n");
}

static void
per_speed_tests (sd_card_spi_speed_t speed, char const *speed_string)
{
  // Perform the various tests that we try for each speed setting.

  printf ("Trying sd_card_init (%s)... ", speed_string);
  sd_card_init (speed);
  printf ("ok.\n");

  printf ("Trying sd_card_size ()... ");
  uint32_t card_size = sd_card_size ();
  if ( card_size != 0 ) {
    printf ("ok, card_size: %lu\n", card_size);
  }
  else {
    printf ("failed.\n");
    assert (0);
  }

  printf ("Trying sd_card_type()... ");
  sd_card_type_t card_type = sd_card_type ();
  printf ("got card type ");
  switch ( card_type ) {
    case SD_CARD_TYPE_INDETERMINATE:
      printf ("indeterminate.\n");
      break;
    case SD_CARD_TYPE_SD1:
      printf ("SD1.\n");
      printf (
          "SD1 type cards haven't been tested.  Disable this warning  and try\n"
          "it :)  Other tests that don't work for this card type might also\n"
          "need to be disabled.\n" );
      assert (0);   // SD1 type cards haven't been tested
      break;
    case SD_CARD_TYPE_SD2:
      printf ("SD2.\n");
      printf (
          "SD2 type cards haven't been tested.  Disable this warning  and try\n"
          "it :)  Other tests that don't work for this card type might also\n"
          "need to be disabled.\n" );
      assert (0);   // SD2 type cards haven't been tested
      break;
    case SD_CARD_TYPE_SDHC:
      printf ("SDHC.\n");
      break;
    default:
      assert (0);   // Shouldn't be here
      break;
  }

  printf ("Trying sd_card_read_cid()... ");
  sd_card_cid_t ccid;   // Card CID
  uint8_t return_code = sd_card_read_cid (&ccid);
  if ( return_code ) {
    printf ("returned TRUE, so presumably it worked.\n");
  }
  else {
    printf ("returned FALSE (failed).\n");
    assert (0);
  }

  printf ("Trying sd_card_read_csd()... ");
  sd_card_csd_t ccsd;   // Card CSD
  return_code = sd_card_read_csd (&ccsd);
  if ( return_code ) {
    printf ("returned TRUE, so presumably it worked.\n");
  }
  else {
    printf ("returned FALSE (failed).\n");
    assert (0);
  }

  printf ("Trying writing/reading... ");
  write_read_42s_at_block_42 ();
  printf ("ok.\n");

  printf ("Trying sd_card_single_block_erase_supported()... ");
  uint8_t result = sd_card_single_block_erase_supported ();
  if ( result ) {
    printf ("ok, it's supported.\n");
    printf ("Trying sd_card_erase_blocks (42, 43)... ");
    return_code = sd_card_erase_blocks (42, 43);
    if ( return_code ) {
      printf ("ok.\n");
    }
    else {
      printf ("failed.\n");
      assert (0);
    }
  }
  else {
    printf ("it's not supported.\n");
    assert (0);
  }

  speed_test_1000_blocks ();

  printf ("Everything worked with %s\n", speed_string);
}

int
main (void)
{
  // This isn't what we're testing exactly, but we need to know if its
  // working or not to interpret other results.
  term_io_init ();
  printf ("\n");
  printf ("\n");
  printf ("term_io_init() worked.\n");
  printf ("\n");

  printf (
      "NOTE: these tests don't bother to call sd_card_last_error() when\n"
      "things go wrong.  You might be able to get information about the\n"
      "nature of a failure by doing that.\n" );
  printf ("\n");

  per_speed_tests (SD_CARD_SPI_FULL_SPEED, "SD_CARD_SPI_FULL_SPEED");
  printf ("\n");

  per_speed_tests (SD_CARD_SPI_FULL_SPEED, "SD_CARD_SPI_HALF_SPEED");
  printf ("\n");

  per_speed_tests (SD_CARD_SPI_FULL_SPEED, "SD_CARD_SPI_QUARTER_SPEED");
  printf ("\n");

  printf ("Everything worked!\n"); 
  printf ("\n");
}
