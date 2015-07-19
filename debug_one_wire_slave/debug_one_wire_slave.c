// Implementation of the interface described in debug_one_wire_master.h.

#include <string.h>

#include "one_wire_slave.h"
#include "debug_one_wire_slave.h"
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
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

int
dows_init (int (*message_handler)(char const *message))
{
  ows_init (FALSE);

  uint8_t jgur = FALSE;

  for ( ; ; ) {

    ows_result_t ows_result;

    uint8_t cmd;

    // FIXME: need to check what's going on inside all these goofy
    // DOWONRGROE() things.

    DOWONRGROE (ows_wait_for_function_transaction (&cmd, jgur));

    // This is the function command code we expect to get from the master
    // to indicate the start of a "printf" transaction.  Note that the
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
      DOWONRGROE (ows_read_byte (((uint8_t *) message_buffer) + ii));
    }

    // The protocol sends the length first, and so doesn't send the
    // terminating NULL byte.  But the message_handler requires it.  So now
    // we add it.
    message_buffer[ml] = '\0';

    PFP ("ml: %hhu, message: %s, cp1\n", ml, message_buffer);

    // Handle the message by calling the supplied handler function.
    message_handler = message_handler;
    int mhr = message_handler (message_buffer);
    if ( mhr ) {
      return mhr;
    }

    // Now we're supposed to send back a specific ack byte to indicate that
    // we've relayed the message successfully.
    uint8_t const ack_byte_value = 0x42;

    //DOWONRGROE (ows_write_byte (ack_byte_value));
    ows_result = ows_write_byte (ack_byte_value);
    if ( ows_result != OWS_RESULT_SUCCESS ) {
      if ( ows_result == OWS_RESULT_GOT_UNEXPECTED_RESET ) {
        // FIXME: WORK POINT: this goes off, even with the busy loop that we
        // added to one_wire_slave.c to try to defuse the problem...  is that
        // fix right?  why couldn't it be?
        PFP ("got unexpected reset\n");
      }
      PHP ();
    }


    // FIXME: get rid of TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP before
    // release
    PTP ();

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
