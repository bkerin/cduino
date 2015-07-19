// Implementation of the interface described in debug_one_wire_master.h.

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include "one_wire_master.h"
#include "debug_one_wire_master.h"
// FIXME: remove after debugged:
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"

// FIXME: do we really want to call this interface debgu_?  log_ might be
// more correct given its generality.  The debug_led is sorta unfortunate
// for the same reason, since it could be used for status maybe should also
// be called status_ or so.  Though the led is less likelty to be useful
// for status than this interface is for logging I think.

void
dowm_init (void)
{
  owm_init ();
}

int
dowm_printf (char const *format, ...)
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

#if DOWM_TARGET_SLAVE == DOWM_ONLY_SLAVE

  uint64_t **rom_ids;
  owr = owm_scan_bus ((uint8_t ***) (&rom_ids));
  assert (owr == OWM_RESULT_SUCCESS);

  slave_id = *(rom_ids[0]);
  assert (rom_ids[1] == NULL);   // We were promised a private line

  // This is the function command code we send to the slave so indicate the
  // start of a a "printf" transaction.  Note that the debug_one_wire_slave.h
  // must agree to use this value and implement its end of the transaction
  // protocol.
  uint8_t const printf_function_cmd = 0x44;

  owr = owm_start_transaction (
      OWC_READ_ROM_COMMAND,
      (uint8_t *) &slave_id,
      printf_function_cmd );
  assert (owr == OWM_RESULT_SUCCESS);

  owm_free_rom_ids_list ((uint8_t **) rom_ids);

#else

#  error not tested yet

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

  _delay_us (ibd_us);

  // Now we send the message length as a byte.
  assert (DOWM_MAX_MESSAGE_LENGTH < UINT8_MAX);
  owm_write_byte ((uint8_t) chars_written);

  // Now we send the message itself.
  for ( uint8_t ii = 0 ; ii < chars_written ; ii++ ) {
    _delay_us (ibd_us);
    owm_write_byte (message_buffer[ii]);
  }

  _delay_us (ibd_us);

  double const ms_per_byte = 100.042;
  _delay_ms (DOWM_MAX_MESSAGE_LENGTH * ms_per_byte);

  // FIXME: note that we don't use any CRC.  We really should.

  // Now the slave is supposed to send back an ack byte to indicate that it
  // has relayed the message successfully.
  uint8_t const ack_byte_value = 0x42;
  uint8_t rb = owm_read_byte ();  // Response Byte
  if ( rb == ack_byte_value ) { rb = rb; }  // FIXME: remove this debug junk
  PFP ("got ack byte %hhu\n", rb);
  //assert (rb == ack_byte_value);

  return chars_written;
}

