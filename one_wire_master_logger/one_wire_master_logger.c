// Implementation of the interface described in one_wire_master_logger.h.

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <util/crc16.h>

#include "one_wire_master.h"
#include "one_wire_master_logger.h"
// This next line may be useful for debugging:
//#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"

void
owml_init (void)
{
  owm_init ();
}

// FIXME: error propagationin this module hasn't been considered

int
owml_printf (char const *format, ...)
{
  char message_buffer[DOWM_MAX_MESSAGE_LENGTH + 1];

  va_list ap;
  va_start (ap, format);
  int chars_written
    = vsnprintf (message_buffer, DOWM_MAX_MESSAGE_LENGTH, format, ap);
  va_end (ap);
  assert (chars_written >= 0);

  owm_result_t owr;   // 1-Wire Result (function return code storage)

  uint64_t slave_id;   // Slave we're going to talk to

  // This is the function command code we send to the slave so indicate the
  // start of a a "printf" transaction.  Note that the one_wire_slave_logger.h
  // must agree to use this value and implement its end of the transaction
  // protocol.
  uint8_t const printf_function_cmd = 0x44;

#if DOWM_TARGET_SLAVE == DOWM_ONLY_SLAVE

  uint64_t **rom_ids;
  owr = owm_scan_bus ((uint8_t ***) (&rom_ids));
  assert (owr == OWM_RESULT_SUCCESS);

  slave_id = *(rom_ids[0]);
  assert (rom_ids[1] == NULL);   // We were promised a private line

  owr = owm_start_transaction (
      OWC_READ_ROM_COMMAND,
      (uint8_t *) &slave_id,
      printf_function_cmd );
  assert (owr == OWM_RESULT_SUCCESS);

  owm_free_rom_ids_list ((uint8_t **) rom_ids);

#else

  slave_id = __builtin_bswap64 (UINT64_C (DOWM_TARGET_SLAVE));

  owr = owm_start_transaction (
      OWC_MATCH_ROM_COMMAND,
      (uint8_t *) &slave_id,
      printf_function_cmd );
  assert (owr == OWM_RESULT_SUCCESS);

#endif

  // To be nice to the slave, we provide a little bit of interbyte delay as
  // per the recommendation in one_wire_slave.h.
  double const ibd_us = 10.0;

  // CRC (initial value as specified for _crc16_update() from AVR libc)
  uint16_t crc = 0xffff;

  // First part is the message length as a byte.
  assert (DOWM_MAX_MESSAGE_LENGTH < UINT8_MAX);
  crc = _crc16_update (crc, chars_written);
  _delay_us (ibd_us);
  owm_write_byte ((uint8_t) chars_written);

  // Next part is the message itself.
  for ( uint8_t ii = 0 ; ii < chars_written ; ii++ ) {
    _delay_us (ibd_us);
    crc = _crc16_update (crc, message_buffer[ii]);
    owm_write_byte (message_buffer[ii]);
  }

  // Finally we send the CRC, high byte first.
  _delay_us (ibd_us);
  owm_write_byte (HIGH_BYTE (crc));
  _delay_us (ibd_us);
  owm_write_byte (LOW_BYTE (crc));

  _delay_us (ibd_us);

  // Wait for the slave to send the zero it sends when it's done handling
  // the message.
  while ( owm_read_bit () ) {
    ;
  }

  // Now the slave is supposed to send back a particular ack byte to indicate
  // that it has relayed the message successfully.
  uint8_t const ack_byte_value = 0x42;
  _delay_us (ibd_us);
  uint8_t rb = owm_read_byte ();  // Response Byte
  // FIXME: should maybe be returning rather than asserting
  assert (rb == ack_byte_value);

  return chars_written;
}
