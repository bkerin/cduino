// Implementation of the interface described in debug_one_wire_master.h.

#include <string.h>

#include "one_wire_slave.h"
#include "debug_one_wire_slave.h"
#include "term_io.h"

// See context.  This just filters out some 1-wire failures that we retry.

// See context.  This just filters out some 1-wire failures that we retry,
// taking note of unexpected resets.  Do One Wire Operation Note Resets
// Goto Retry On Error it stands for :)
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

void
dows_init (int (*message_handler)(char const *message))
{
  ows_init (FALSE);

  uint8_t jgur = FALSE;

  for ( ; ; ) {

    ows_result_t ows_result;

    uint8_t cmd;

    DOWONRGROE (ows_wait_for_function_transaction (&cmd, jgur));

    // This is the function command code we expect to get from the master
    // to indicate the start of a a "printf" transaction.  Note that the
    // debug_one_wire_master.h must agree to use this value and implement
    // its end of the transaction protocol.
    uint8_t const printf_function_cmd = 0x44;

    if ( cmd != printf_function_cmd ) {
      goto retry;
    }

    // Read message length from master
    uint8_t ml;
    DOWONRGROE (ows_read_byte (&ml));

    // Read the message itself from master
    char message_buffer[DOWS_MAX_MESSAGE_LENGTH + 1];
    for ( uint8_t ii = 0 ; ii < ml ; ii++ ) {
      DOWONRGROE (ows_read_byte (message_buffer + ii));
    }

    // Handle the message by calling the supplied handler function.
    int mhr = message_handler (message_buffer);
    if ( mhr ) {
      return mhr;
    }

    // Now we're supposed to send back a specific ack byte to indicate that
    // we've relayed the message successfully.
    uint8_t const ack_byte_value = 0x42;

    DOWONRGROE (ows_write_byte (ack_byte_value));

retry:
    continue;

  }

}

int
relay_via_term_io (char const *message);
{
  static uint8_t tioi = FALSE;   // Term-IO Initialized

  if ( ! tioi ) {
    term_io_init ();
    tioi = TRUE;
  }

  int cp = printf ("%s", message);
  if ( cp == strlen (message) ) {
    return 0;
  }
  else {
    return 1;
  }
}
