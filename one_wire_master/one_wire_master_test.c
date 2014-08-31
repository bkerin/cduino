//
// FIXME: describe seperate power assumption
//
// This program assumes that the DS18B20 EEPROM configuration is in the
// default (factory) state: FIXME; say what that is here


#include <assert.h>
#include <stdlib.h>

#include "dio.h"
#include "one_wire_master.h"
#include "util.h"

#define ONE_WIRE_PIN DIO_PIN_DIGITAL_2

#define DS18B20_SCRATCHPAD_SIZE  9
#define DS18B20_SCRATCHPAD_T_LSB 0
#define DS18B20_SCRATCHPAD_T_MSB 1

static uint8_t spc[DS18B20_SCRATCHPAD_SIZE];   // DS18B20 Scratchpad Contents

static uint64_t
init_and_rom_command (OneWireMaster *owm)
{
  // Requies exactly on DS18B20 device to be present on the bus.  Perform the
  // Initialization (Step 1) and ROM Command (Step 2) steps of the transaction
  // sequence described in the DS18B20 datasheet, and return the discovered
  // ROM code of the slave.

  // Prompt the slave(s) to respond with a "presence pulse".  This corresponds
  // to the "INITIALIZATION" step (Step 1) described in the DS18B20 datasheet.
  // FIXME: would be nice to have datasheet available on web and linked to
  // by the docs...
  uint8_t slave_presence = owm_touch_reset (owm);
  assert (slave_presence);

  // This test program requires that only one slave is present, so we can
  // use the READ ROM command to get the slave's address.
  uint64_t slave_rom;
  uint8_t const read_rom_command = 0xF0;
  owm_write_byte (owm, read_rom_command);
  for ( uint8_t ii = 0 ; ii < sizeof (slave_rom) ; ii++ ) {
    (&slave_rom)[ii] = owm_read_byte (owm);
  }

  return slave_rom;
}

static void
get_scratchpad_contents (OneWireMaster *owm)
{
  // Send the command that causes the DS18B20 to send the scratchpad contents,
  // then read the result and store it in spc.  This routine must follow an
  // init_and_rom_command() call.

  uint8_t const read_scratchpad_command = 0xBE;
  owm_write_byte (owm, read_scratchpad_command);
  for ( uint8_t ii = 0 ; ii < DS18B20_SCRATCHPAD_SIZE ; ii++ ) {
    spc[ii] = owm_read_byte (owm);
  }
}

int
main (void)
{
  dio_pin_t owp = { ONE_WIRE_PIN };
  OneWireMaster *owm = owm_new (owp);

  uint64_t slave_rom = init_and_rom_command (owm);

  uint8_t const convert_t_command = 0x44;
  owm_write_byte (owm, convert_t_command);

  // The DS18B20 is now supposed to respond with a stream of 0 bits until the
  // conversion completes, after which it's supposed to send 1 bits.  So we
  // could do this bit-by-bit if our API exposed the bit-by-bit interface.
  // But it shouldn't hurt to read a few extra ones.
  uint8_t conversion_complete = 0;
  while ( ! (conversion_complete = owm_read_byte (owm)) ) {
    ;
  }

  // We can now read the device scratchpad memory.  This requires us to first
  // perform the initialization and read rom commands again as described in
  // the DS18B20 datasheet.  The slave ROM code better be the same on second
  // reading :)
  uint64_t slave_rom_2nd_reading = init_and_rom_command (owm);
  assert (slave_rom == slave_rom_2nd_reading);

  get_scratchpad_contents (owm);

  // Convenient names for the temperature bytes
  uint8_t t_lsb = spc[DS18B20_SCRATCHPAD_T_LSB];
  uint8_t t_msb = spc[DS18B20_SCRATCHPAD_T_MSB];

  int16_t temp = (((int16_t) t_msb) << BITS_PER_BYTE) | t_lsb;

  // Need to report the temp somehow.
  temp = temp;
}
