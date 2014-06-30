// Test/demo for the write_random_id_to_eeprom target in generic.mk.

// This module just demonstrates/exercises some functionality from the
// build system.  See the target mentioned above for more details.

#include <avr/eeprom.h>
#include <util/delay.h>
#include <inttypes.h>

#include "term_io.h"

// There are no external hardware requirements other than an arduino and a USB
// cable to connect it to the computer.  It should be possible to run
//
//   make -rR run_screen
//
// or so from the module directory to see it do its thing.

int
main (void)
{
  term_io_init ();

  // Delay a bit at startup to give use time to launch screen and watch the
  // initial values of thigs after reprogramming or changing the ID.
  _delay_ms (3042);

  for ( ; ; ) {

    void const   *id_address = 0;   // ID is at start of EEPROM
    size_t const  id_size    = 8;   // ID is this many bytes long
    uint64_t      id;               // ID itself (to be read)

    eeprom_read_block (&id, id_address, id_size);

    // AVR libc doesn't support 64 bit printf/scanf conversions, so we just
    // do things a byte at a time.
    printf ("ID: ");
    for ( uint8_t ii = 0 ; ii < 8 ; ii++ ) {
      printf ("%" PRIx8, ((uint8_t *) (&id))[ii] );
    }
    printf ("\n");

    // This block is used to verify that writing the first 8 bytes of the
    // flash doesn't change the others: after running this program, then
    // changing the ID with the make target, then restarting this program,
    // the value should still be 42 the first time we hit this point.
    uint8_t *byte_8_offset = (uint8_t *) 8;
    uint8_t byte_offset_8_value = eeprom_read_byte (byte_8_offset);
    printf (
        "Current value of byte at offset 8: %" PRIu8 "\n",
        byte_offset_8_value );
    eeprom_write_byte (byte_8_offset, 42);

    _delay_ms (1042);   // Delay a little bit between repetitions of the ID
  }
}
