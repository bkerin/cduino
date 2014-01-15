
#include <assert.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>   // FIXXME: probably only needed for broken assert header
#include <string.h>
#include <util/delay.h>

#include "uart.h"
#include "wireless_xbee.h"
#include "util.h"

// FIXME: for final testing we don't need this hack to allow motor shield
// use here at any rate (in _test.c only).
#define CHKP_PD4() CHKP_USING (DDRD, DDD4, PORTD, PORTD4, 300.0, 3)

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
    
  // NOTE: this seems like more RAM than we need, but clients are eventually
  // going to need it anyway to read the (longer) responses from real query
  // commands.
  char response[WX_MCOSL];

  uint8_t sentinel = get_line (WX_MCOSL, response);
  assert (sentinel);
  
  // FIXME: should propagate
  assert (! strcmp (response, "OK\r"));

  return TRUE;
}

static void
put_command (char const *command)
{
  // Put a command out on the serial port.  The "AT" prefix and "\r" postfix
  // are automatically added to the supplied command.  This resulting bytes
  // are sent.  This routine doesn't wait for a response.

  UART_PUT_BYTE ('A');
  UART_PUT_BYTE ('T');

  uint8_t csl = strlen (command);   // Command String Length
  for ( uint8_t ii = 0 ; ii < csl ; ii++ ) {
    UART_PUT_BYTE (command[ii]);
  }

  UART_PUT_BYTE ('\r');
}

static uint8_t
at_command (char *command, char *output)
{
  // Require the XBee module to be in AT command mode (see
  // enter_at_command_mode()).  Execute the given AT command, and return its
  // output.  The command should ommit the "AT" prefix and "\r" postfix: this
  // routine will add them.  The trailing "\r" that is returned is removed
  // from output.  Return true on success.  Both command and output must be
  // pointers to at least WX_MCOSL bytes of storage.  Its ok to pass the same
  // pointer for both command and output, in which case the command string
  // is overwritten with the command output (saving a few bytes of RAM).

  put_command (command);
 
  uint8_t sentinel = get_line (WX_MCOSL, output);
  assert (sentinel);  // FIXME: propagate

  // Verify that the output string ends with '\r'
  uint8_t osl = strnlen (output, WX_MCOSL);   // Output String Length
  assert (output[osl - 1] == '\r');   // FIXME: propagate

  output[osl - 1] = '\0';   // Snip the trailing "\r" as promised

  return TRUE;
}

static uint8_t
at_command_expect_ok (char const *command)
{
  // Like at_command(), but simply checks that the result is "OK\r" and
  // returns TRUE iff it is.

  put_command (command);

  // FIXME: propagate
  assert (get_char () == 'O');
  assert (get_char () == 'K');
  assert (get_char () == '\r');

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
  
  assert (! strcmp (response, "OK"));  // FIXME: propagate

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

uint8_t
wx_com_expect_ok (char const *command)
{
  uint8_t sentinel = enter_at_command_mode ();
  assert (sentinel);   // FIXME: propagate

  sentinel = at_command_expect_ok (command);
  assert (sentinel);   // FIXME: propagate

  sentinel = exit_at_command_mode ();
  assert (sentinel);   // FIXME: propagate
  
  return TRUE;
}

uint8_t
wx_ensure_network_id_set_to (uint16_t id)
{
  char buf[WX_MCOSL];   // Buffer for command/output string storage
  uint8_t tmp;          // For various things (char counts, sentinels, etc.)

  tmp = enter_at_command_mode (); 
  assert (tmp);   // FIXME: propagate
   
  tmp = sprintf_P (buf, PSTR ("ID"));
  assert (tmp == 2);  // sprintf_P gives a return value, so we check it

  tmp = at_command (buf, buf);
  assert (tmp);   // FIXME: propagate

  int const base_16 = 16;   // Base to use to convert retrieved string
  char *endptr;   //  Pointer to be set to end of converted string
  long int eidv = strtol (buf, &endptr, base_16);   // Existing ID value
  if ( *buf != '\0' && *endptr == '\0' ) {
    if ( eidv == id ) {
      return TRUE;   // ID is already set as requested, so we're done
    }
  }
  else {
    // FIXME: propagate
    assert (0);   // Didn't get a convertible string back from command
  }

  // Print the id into a command string (in the form expected by
  // at_command_expect_ok()).  The argument itself must be upper case,
  // without any leading "0x" or "0X".
  uint8_t const escsl = 6;   // Expected Setting Command String Length
  uint8_t cp = sprintf_P (buf, PSTR ("ID%.4" PRIX16), id);
  assert (cp == escsl);   // FIXME: propagate

  tmp = at_command_expect_ok (buf);
  assert (tmp);   // FIXME: propagate

  tmp = at_command_expect_ok ("WR");
  assert (tmp);   // FIXME: propagate

  tmp = exit_at_command_mode ();
  assert (tmp);

  return TRUE;
}

uint8_t
wx_ensure_channel_set_to (uint8_t channel)
{
  // The channel argument must fall in the valid range.  FIXME: propagate?
  assert (channel >= 0x0b);
  assert (channel <= 0x1a);

  char buf[WX_MCOSL];   // Buffer for command/output string storage
  uint8_t tmp;          // For various things (char counts, sentinels, etc.)

  tmp = enter_at_command_mode (); 
  assert (tmp);   // FIXME: propagate
   
  tmp = sprintf_P (buf, PSTR ("CH"));
  assert (tmp == 2);  // sprintf_P gives a return value, so we check it

  tmp = at_command (buf, buf);
  assert (tmp);   // FIXME: propagate

  int const base_16 = 16;   // Base to use to convert retrieved string
  char *endptr;   //  Pointer to be set to end of converted string
  long int echv = strtol (buf, &endptr, base_16);   // Existing CH value
  if ( *buf != '\0' && *endptr == '\0' ) {
    if ( echv == channel ) {
      return TRUE;   // CH is already set as requested, so we're done
    }
  }
  else {
    // FIXME: propagate
    assert (0);   // Didn't get a convertible string back from command
  }

  // Print the channel into a command string (in the form expected by
  // at_command_expect_ok()).  The argument itself must be upper case,
  // without any leading "0x" or "0X".
  uint8_t const escsl = 6;   // Expected Setting Command String Length
  uint8_t cp = sprintf_P (buf, PSTR ("CH%.4" PRIX16), channel);
  assert (cp == escsl);   // FIXME: propagate

  tmp = at_command_expect_ok (buf);
  assert (tmp);   // FIXME: propagate

  tmp = at_command_expect_ok ("WR");
  assert (tmp);   // FIXME: propagate

  tmp = exit_at_command_mode ();
  assert (tmp);

  return TRUE;
}

uint8_t
wx_restore_defaults (void)
{
  uint8_t sentinel;

  sentinel = enter_at_command_mode (); 
  assert (sentinel);   // FIXME: propagate

  sentinel = at_command_expect_ok ("RE");
  assert (sentinel);   // FIXME: propagate

  sentinel = at_command_expect_ok ("WR");
  assert (sentinel);   // FIXME: propagate
  
  sentinel = exit_at_command_mode (); 
  assert (sentinel);   // FIXME: propagate

  return TRUE;
}
