#include <assert.h>
#include <stdlib.h>

#include <Sd2Card.h>

#ifdef __cplusplus
  extern "C" {
#   include <term_io.h>
  }
#endif

const int chipSelect = 4;

#define SD_CARD_BLOCK_SIZE 512

int
main (void)
{
  term_io_init ();

  sd_card_init (SPI_HALF_SPEED);

  uint32_t card_size = sd_card_size ();

  printf ("card_size: %lu\n", card_size);

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

  printf ("it worked!\n"); 
}
