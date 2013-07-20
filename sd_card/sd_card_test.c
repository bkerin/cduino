// Test/demo for the sd_card.h interface.
//
// This test driver requires an Arduino SD Card/Ethernet shield
// (http://arduino.cc/en/Main/ArduinoEthernetShield) with an SD card that
// supports all the tested features to be connected.
//
// Its 

#include <assert.h>
#include <stdlib.h>

#include <sd_card.h>

#include <term_io.h>

#define SD_CARD_BLOCK_SIZE 512

static void
write_read_42s_at_block_42 (void)
{
  // Fill block 42 with 42s.

  uint32_t bn = 42;
  uint8_t data_block[SD_CARD_BLOCK_SIZE];
  int ii;
  for ( ii = 0 ; ii < SD_CARD_BLOCK_SIZE ; ii++ ) {
    data_block[ii] = 42;
  }
  sd_card_write_block (bn, data_block);

  uint8_t reread_data[SD_CARD_BLOCK_SIZE];
  sd_card_read_block (bn, reread_data);

  for ( ii = 0 ; ii < SD_CARD_BLOCK_SIZE ; ii++ ) {
    assert (reread_data[ii] == 42);
  }
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
      break;
    case SD_CARD_TYPE_SD2:
      printf ("SD2.\n");
      break;
    case SD_CARD_TYPE_SDHC:
      printf ("SDHC.\n");
      break;
    default:
      assert (0);   // Shouldn't be here
      break;
  }

  printf ("Trying sd_card_read_cid()... ");
  cid_t ccid;   // Card CID
  uint8_t return_code = sd_card_read_cid (&ccid);
  if ( return_code ) {
    printf ("returned TRUE, so presumably it worked.\n");
  }
  else {
    printf ("returned FALSE (failed).\n");
    assert (0);
  }

  printf ("Trying sd_card_read_csd()... ");
  csd_t ccsd;   // Card CSD
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

  printf ("Everything worked with %s\n", speed_string);
}

int
main (void)
{
  // FIXME: WORK POINT: adapt the errorCode and errorData methods over from
  // the model, and print some stuff in these tests about how we don't bother
  // to use their output even if we get an error during these tests :)

  // FIXME: add a speed test

  printf ("\n");

  term_io_init ();
  printf ("term_io_init() worked.\n");

  printf ("\n");

  per_speed_tests (SD_CARD_SPI_FULL_SPEED, "SD_CARD_SPI_FULL_SPEED");
  printf ("\n");

  per_speed_tests (SD_CARD_SPI_FULL_SPEED, "SD_CARD_SPI_HALF_SPEED");
  printf ("\n");

  per_speed_tests (SD_CARD_SPI_FULL_SPEED, "SD_CARD_SPI_QUARTER_SPEED");
  printf ("\n");

  printf ("Everything worked!\n"); 
}
