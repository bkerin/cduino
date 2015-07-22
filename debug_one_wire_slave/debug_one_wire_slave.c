// Implementation of the interface described in debug_one_wire_master.h.

#include <string.h>

#include "one_wire_slave.h"
#include "debug_one_wire_slave.h"
// FIXME: get rid of TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP before
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"

#include <util/crc16.h>

// See context.  This just filters out some 1-wire failures that we retry.

// See context.  This just filters out some 1-wire failures that we retry,
// taking note of unexpected resets.  Do One Wire Operation Note Resets
// Goto Retry On Error it stands for :)
// FIXME: do we really want to just silently retry every type of error?
#define DOWONRGROE(op)                                     \
  do {                                                     \
    ows_result = op;                                       \
    if ( ows_result == OWS_RESULT_GOT_UNEXPECTED_RESET ) { \
      jgur = TRUE;                                         \
    }                                                      \
    if ( ows_result != OWS_RESULT_SUCCESS ) {              \
      goto retry;                                          \
    }                                                      \
  } while ( 0 );

int
dows_init (int (*message_handler)(char const *message))
{
  ows_init (FALSE);

  uint8_t jgur = FALSE;

  for ( ; ; ) {

    ows_result_t ows_result;

    uint8_t cmd;

    DOWONRGROE (ows_wait_for_function_transaction (&cmd, jgur));

    // This is the function command code we expect to get from the master
    // to indicate the start of a "printf" transaction.  Note that the
    // debug_one_wire_master.h must agree to use this value and implement
    // its end of the transaction protocol.
    uint8_t const printf_function_cmd = 0x44;

    if ( cmd != printf_function_cmd ) {
      goto retry;
    }

    // CRC (initial value as specified for _crc16_update() from AVR libc)
    uint16_t crc = 0xffff;

    // Read message length from master
    uint8_t ml;
    DOWONRGROE (ows_read_byte (&ml));
    crc = _crc16_update (crc, ml);


    // Read the message itself from master
    char message_buffer[DOWS_MAX_MESSAGE_LENGTH + 1];
    for ( uint8_t ii = 0 ; ii < ml ; ii++ ) {
      DOWONRGROE (ows_read_byte (((uint8_t *) message_buffer) + ii));
      crc = _crc16_update (crc, (uint8_t) (message_buffer[ii]));
    }

    uint8_t crc_hb, crc_lb;   // CRC High/Low Byte (as sent by master)
    DOWONRGROE (ows_read_byte (&crc_hb));
    DOWONRGROE (ows_read_byte (&crc_lb));
    uint16_t received_crc
      = ((uint16_t) crc_hb << BITS_PER_BYTE) | ((uint16_t) crc_lb);
    assert (crc == received_crc);

    // At this point we become busy handling the message that the master has
    // sent, which might take a while depending on what message_handler does.
    // So we're taking advantage of the send-ones-then-a-zero busy wait
    // approach described in the summary at the top of one_wire_slave.h.

    // The protocol sends the length first, and so doesn't send the
    // terminating NULL byte.  But the message_handler requires it.  So now
    // we add it.
    message_buffer[ml] = '\0';

    // Handle the message by calling the supplied handler function.
    message_handler = message_handler;
    int mhr = message_handler (message_buffer);
    if ( mhr ) {
      return mhr;
    }

    // Once the message is sent we are done being busy.
    ows_unbusy ();

    // Now we're supposed to send back a specific ack byte to indicate that
    // we've relayed the message successfully.
    uint8_t const ack_byte_value = 0x42;

    DOWONRGROE (ows_write_byte (ack_byte_value));

retry:
    continue;

  }

}

int
relay_via_term_io (char const *message)
{
  int cp = printf ("%s", message);
  if ( cp < 0 ) {
    return 1;
  }
  if ( (size_t) cp != strlen (message) ) {
    return 1;
  }

  return 0;
}
