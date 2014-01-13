
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
  assert (WX_MCOSL < UINT8_MAX); 
    
  char response[WX_MCOSL];

  uint8_t sentinel = get_line (WX_MCOSL, response);
  assert (sentinel);
  
  // FIXME: should propagate
  assert (! strcmp (response, "OK\r"));

  return TRUE;
}

static uint8_t
at_command (char *command, char *output)
{
  // Require the XBee module to be in AT command mode (see
  // enter_at_command_mode()).  Execute the given AT command, and return
  // its output.  The command should ommit the "AT" prefix and "\r" postfix:
  // this routine will add them.  The trailing "\r" that is returned is
  // removed from output.  Return true on success.

  UART_PUT_BYTE ('A');
  UART_PUT_BYTE ('T');

  for ( uint8_t ii = 0 ; ii < strlen (command) ; ii++ ) {
    UART_PUT_BYTE (command[ii]);
  }

  UART_PUT_BYTE ('\r');
 
  uint8_t sentinel = get_line (WX_MCOSL, output);
  assert (sentinel);  // FIXME: propagate

  return TRUE;
}

static uint8_t
exit_at_command_mode (void)
{
  // Require the XBee module to be in AT command mode (see
  // enter_at_command_mode()).  Leave AT command mode.
  
  char response[WX_MCOSL];

  uint8_t sentinel = at_command ("CN", response);
  assert (sentinel);   // FIXME: propagate
  
  assert (! strcmp (response, "OK\r"));  // FIXME: propagate

  return TRUE;
}

uint8_t
wx_com (char *command, char *output)
{

  uint8_t sentinel = enter_at_command_mode ();
  assert (sentinel);   // FIXME: propagate

  sentinel = at_command (command, output);
  assert (sentinel);   // FIXME: propagate

  sentinel = exit_at_command_mode ();
  assert (sentinel);   // FIXME: propagate

  return TRUE;
}
