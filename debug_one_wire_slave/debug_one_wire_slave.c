// Implementation of the interface described in debug_one_wire_master.h.

#include <string.h>

#include "one_wire_slave.h"
#include "debug_one_wire_slave.h"
// This next line may be useful for debugging:
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"

#include <util/crc16.h>

// Call call, Propagating Most Failures.  Unexpected resets aren't
// propagated, but instead end up arming a jgur argument for the next
// ows_wait_for_function_transaction() call.  This macro may only be used
// from within a particular function. The call argument must be a call to
// a function returning ows_result_t.
#define CPMF(call)                                      \
  do {                                                  \
    ows_result_t XxX_err = call;                        \
    if ( XxX_err == OWS_RESULT_GOT_UNEXPECTED_RESET ) { \
      goto arm_jgur;                                    \
    }                                                   \
    if ( UNLIKELY (XxX_err != OWS_RESULT_SUCCESS) ) {   \
      return XxX_err;                                   \
    }                                                   \
  } while ( 0 )

int
dows_init (int (*message_handler)(char const *message))
{
  ows_init (FALSE);

  uint8_t jgur = FALSE;

  for ( ; ; ) {

    uint8_t cmd;

    CPMF (ows_wait_for_function_transaction (&cmd, jgur));

    // This is the function command code we expect to get from the master
    // to indicate the start of a "printf" transaction.  Note that the
    // debug_one_wire_master.h must agree to use this value and implement
    // its end of the transaction protocol.
    uint8_t const printf_function_cmd = 0x44;

    if ( cmd != printf_function_cmd ) {
      return DOWS_RESULT_ERROR_INVALID_FUNCTION_CMD;
    }

    // CRC (initial value as specified for _crc16_update() from AVR libc)
    uint16_t crc = 0xffff;

    // Read message length from master
    uint8_t ml;
    CPMF (ows_read_byte (&ml));
    crc = _crc16_update (crc, ml);

    // Read the message itself from master
    char message_buffer[DOWS_MAX_MESSAGE_LENGTH + 1];
    for ( uint8_t ii = 0 ; ii < ml ; ii++ ) {
      CPMF (ows_read_byte (((uint8_t *) message_buffer) + ii));
      crc = _crc16_update (crc, (uint8_t) (message_buffer[ii]));
    }

    uint8_t crc_hb, crc_lb;   // CRC High/Low Byte (as sent by master)
    CPMF (ows_read_byte (&crc_hb));
    CPMF (ows_read_byte (&crc_lb));
    uint16_t received_crc
      = ((uint16_t) crc_hb << BITS_PER_BYTE) | ((uint16_t) crc_lb);
    if ( crc != received_crc ) {
      return DOWS_RESULT_ERROR_CRC_MISMATCH;
    }

    // At this point we become busy handling the message that the master has
    // sent, which might take a while depending on what message_handler does.
    // So we're taking advantage of the send-ones-then-a-zero busy wait
    // approach described in the summary at the top of one_wire_slave.h.

    // The protocol sends the length first, and so doesn't send the
    // terminating NULL byte.  But the message_handler requires it.  So now
    // we add it.
    message_buffer[ml] = '\0';

    // Handle the message by calling the supplied handler.
    message_handler = message_handler;
    int mhr = message_handler (message_buffer);
    if ( mhr != 0 ) {
      return mhr;
    }

    // Once the message is sent we are done being busy.
    ows_unbusy ();

    // Now we're supposed to send back this specific ack byte to indicate
    // that we've relayed the message successfully.
    uint8_t const ack_byte_value = 0x42;

    CPMF (ows_write_byte (ack_byte_value));

    jgur = FALSE;
    continue;

arm_jgur:
    jgur = TRUE;
  }


}

int
dows_relay_via_term_io (char const *message)
{
  int cp = printf ("%s", message);
  if ( cp < 0 ) {
    return -1;
  }
  if ( (size_t) cp != strlen (message) ) {
    return -1;
  }

  return 0;
}
