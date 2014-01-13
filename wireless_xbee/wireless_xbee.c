
#include <assert.h>
#include <stdlib.h>   // FIXXME: probably only needed for broken assert header
#include <string.h>
#include <util/delay.h>

#include "uart.h"
#include "wireless_xbee.h"
#include "util.h"

void
wx_init (void)
{
  uart_init ();
}

static char
get_char (void)
{
  UART_WAIT_FOR_BYTE ();
  assert (! UART_RX_ERROR ());   // FIXME: should propagate somehow
  return UART_GET_BYTE ();
}

// Maximum length of response expected for commands, including the trailing
// carriage return ('\r').  Note that some function automatically add a
// NUL byte to results, and so may require an additional character of storage.
// FIXME: we should trim this down to what is actually needed
#define WX_MAX_COMMAND_RESPONSE_LENGTH 42

static uint8_t
get_line (uint8_t bufsize, char *buf)
{
  // Get up to bufsize - 1 character from the serial port, or until a
  // carriage return ('\r') character is recieved, then add a terminating
  // NUL byte and return TRUE to indicate success.

  uint8_t ii;   // Index

  for ( ii = 0 ; ii < bufsize - 1 ; ii++ ) {
    buf[ii] = get_char ();
    if ( buf[ii] == '\r' ) {
      ii++;
      break;
    }
  } 

  buf[ii] = '\0';

  return TRUE;
}

#define WX_MCRL 42

static uint8_t
enter_at_command_mode (void)
{
  // Send the sequence which initiates AT command mode, returning true on
  // success.  After this function returns, the XBee will remain in command
  // mode for up to 10 seconds (or until exit_at_command_mode() is called.

  // This magic sequence should send us into AT command mode
  float const dwmms = 1142;   // Delay With Margin (in ms) -- AT requires 1 s
  _delay_ms (dwmms);
  UART_PUT_BYTE ('+');
  UART_PUT_BYTE ('+');
  UART_PUT_BYTE ('+');
  _delay_ms (dwmms);
  
  // This probably goes without saying, but we have to assert it somewhere
  assert (WX_MAX_COMMAND_RESPONSE_LENGTH < UINT8_MAX); 
    
  char response[WX_MAX_COMMAND_RESPONSE_LENGTH + 1];

  uint8_t sentinel = get_line (WX_MAX_COMMAND_RESPONSE_LENGTH + 1, response);
  assert (sentinel);
  
  // FIXME: should propagate
  assert (! strcmp (response, "OK\r"));

  return TRUE;
}

uint8_t
wx_com (char *command, char *output)
{
  // FIXME: have to do the auto-addition still
  // Enter command mode, execute the given AT command with an "AT" prefix
  // and "\r" postfix implicitly added (e.g. "BD9600" becomes "ATBD9600"),
  // place the command output in output, stip the trailing carriage return
  // ("\r") from output, and finally return TRUE if all that succeeded.

  /*

  // This magic sequence should send us into AT command mode
  float const dwmms = 1142;   // Delay With Margin (in ms) -- AT requires 1 s
  _delay_ms (dwmms);
  UART_PUT_BYTE ('+');
  UART_PUT_BYTE ('+');
  UART_PUT_BYTE ('+');
  _delay_ms (dwmms);
  
  assert (WX_MAX_COMMAND_RESPONSE_LENGTH < UINT8_MAX); 
    
  char response[WX_MAX_COMMAND_RESPONSE_LENGTH + 1];

  uint8_t sentinel = get_line (WX_MAX_COMMAND_RESPONSE_LENGTH + 1, response);
  assert (sentinel);
  
  // FIXME: should propagate
  assert (! strcmp (response, "OK\r"));

  */
  
  uint8_t sentinel = enter_at_command_mode ();

  uint8_t ii;   // Index

  for ( ii = 0 ; ii < strlen (command) ; ii++ ) {
    UART_PUT_BYTE (command[ii]);
  }

  sentinel = get_line (42, output);
  assert (sentinel);
  
  /*
  ii = 0;

  do {
    UART_WAIT_FOR_BYTE ();
    // FIXME: needs timeouts/error checking/prop
    assert (! UART_RX_ERROR ());
    output[ii] = UART_GET_BYTE ();
  } while ( output[ii++] != '\r' );
  */


  return TRUE;  // FIXME: Meaning command worked I guess
}
