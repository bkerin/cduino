// Implementation of the interface described in wireless_xbee.h.

// We're using assert() to handle errors, we can't let clients turn it off.
#ifdef NDEBUG
#  error The HANDLE_ERRORS() macro in this file requires assert()
#endif
#include <assert.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>   // FIXXME: probably only needed for broken assert header
#include <string.h>
#include <util/delay.h>

#include "uart.h"
#include "wireless_xbee.h"
#include "util.h"

// For development: signal a check point by blinking something attached to PD4
#define CHKP_PD4() CHKP_USING (DDRD, DDD4, PORTD, PORTD4, 300.0, 3)

void
wx_init (void)
{
  uart_init ();
}

// Define a HANDLE_ERRORS macro that either asserts the given condition,
// or simple returns depending on a compile-time setting.  We guarantee
// that this macro will evaluate its argument only once, so its safe to
// use around a function that has other effects.
#ifdef WX_ASSERT_SUCCESS
#  define HANDLE_ERRORS(condition) assert (condition)
#else
#  define HANDLE_ERRORS(condition) \
  do { \
    if ( UNLIKELY (! (condition)) ) { \
      return FALSE; \
    } \
  } while ( 0 )
#endif

static char
get_char (void)
{
  UART_WAIT_FOR_BYTE ();
  HANDLE_ERRORS (! UART_RX_ERROR ());
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

uint8_t
wx_enter_at_command_mode (void)
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
  HANDLE_ERRORS (sentinel);
  
  HANDLE_ERRORS (! strcmp (response, "OK\r"));

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
 
  HANDLE_ERRORS (get_line (WX_MCOSL, output));

  // Verify that the output string ends with '\r'
  uint8_t osl = strnlen (output, WX_MCOSL);   // Output String Length
  HANDLE_ERRORS (output[osl - 1] == '\r');

  output[osl - 1] = '\0';   // Snip the trailing "\r" as promised

  return TRUE;
}

uint8_t
wx_exit_at_command_mode (void)
{
  // Require the XBee module to be in AT command mode (see
  // enter_at_command_mode()).  Leave AT command mode.

  char response[WX_MCOSL];

  HANDLE_ERRORS (at_command ("CN", response));

  HANDLE_ERRORS (! strcmp (response, "OK"));

  return TRUE;
}

static uint8_t
at_command_expect_ok (char const *command)
{
  // Like at_command(), but simply checks that the result is "OK\r" and
  // returns TRUE iff it is.

  put_command (command);

  HANDLE_ERRORS (get_char () == 'O');
  HANDLE_ERRORS (get_char () == 'K');
  HANDLE_ERRORS (get_char () == '\r');

  return TRUE;
}

uint8_t
wx_com (char *command, char *output)
{
  HANDLE_ERRORS (wx_enter_at_command_mode ());

  HANDLE_ERRORS (at_command (command, output));

  HANDLE_ERRORS (wx_exit_at_command_mode ());

  return TRUE;
}

uint8_t
wx_com_expect_ok (char const *command)
{
  HANDLE_ERRORS (wx_enter_at_command_mode ());

  HANDLE_ERRORS (at_command_expect_ok (command));

  HANDLE_ERRORS (wx_exit_at_command_mode ());
  
  return TRUE;
}

uint8_t
wx_ensure_network_id_set_to (uint16_t id)
{
  char buf[WX_MCOSL];   // Buffer for command/output string storage

  HANDLE_ERRORS (wx_enter_at_command_mode ());
   
  uint8_t cp = sprintf_P (buf, PSTR ("ID"));   // Chars Printed
  HANDLE_ERRORS (cp == 2);  // sprintf_P gives a return value, so we check it

  HANDLE_ERRORS (at_command (buf, buf));

  int const base_16 = 16;   // Base to use to convert retrieved string
  char *endptr;   //  Pointer to be set to end of converted string
  long int eidv = strtol (buf, &endptr, base_16);   // Existing ID value
  if ( *buf != '\0' && *endptr == '\0' ) {
    if ( eidv == id ) {
      return TRUE;   // ID is already set as requested, so we're done
    }
  }
  else {
    HANDLE_ERRORS (0);   // Didn't get a convertible string back from command
  }

  // Print the id into a command string (in the form expected by
  // at_command_expect_ok()).  The argument itself must be upper case,
  // without any leading "0x" or "0X".
  uint8_t const escsl = 6;   // Expected Setting Command String Length
  cp = sprintf_P (buf, PSTR ("ID%.4" PRIX16), id);
  HANDLE_ERRORS (cp == escsl);

  HANDLE_ERRORS (at_command_expect_ok (buf));

  HANDLE_ERRORS (at_command_expect_ok ("WR"));

  HANDLE_ERRORS (wx_exit_at_command_mode ());

  return TRUE;
}

uint8_t
wx_ensure_channel_set_to (uint8_t channel)
{
  // The channel argument must fall in the valid range.
  HANDLE_ERRORS (channel >= 0x0b);
  HANDLE_ERRORS (channel <= 0x1a);
  
  char buf[WX_MCOSL];   // Buffer for command/output string storage

  HANDLE_ERRORS (wx_enter_at_command_mode ());
   
  uint8_t cp = sprintf_P (buf, PSTR ("CH"));   // Chars Printed
  HANDLE_ERRORS (cp == 2);  // sprintf_P gives a return value, so we check it

  HANDLE_ERRORS (at_command (buf, buf));

  int const base_16 = 16;   // Base to use to convert retrieved string
  char *endptr;   //  Pointer to be set to end of converted string
  long int echv = strtol (buf, &endptr, base_16);   // Existing CH value
  if ( *buf != '\0' && *endptr == '\0' ) {
    if ( echv == channel ) {
      return TRUE;   // CH is already set as requested, so we're done
    }
  }
  else {
    HANDLE_ERRORS (0);   // Didn't get a convertible string back from command
  }

  // Print the channel into a command string (in the form expected by
  // at_command_expect_ok()).  The argument itself must be upper case,
  // without any leading "0x" or "0X".
  uint8_t const escsl = 6;   // Expected Setting Command String Length
  cp = sprintf_P (buf, PSTR ("CH%.4" PRIX16), channel);
  HANDLE_ERRORS (cp == escsl);

  HANDLE_ERRORS (at_command_expect_ok (buf));

  HANDLE_ERRORS (at_command_expect_ok ("WR"));

  HANDLE_ERRORS (wx_exit_at_command_mode ());

  return TRUE;
}

uint8_t
wx_restore_defaults (void)
{
  HANDLE_ERRORS (wx_enter_at_command_mode ());

  HANDLE_ERRORS (at_command_expect_ok ("RE"));

  HANDLE_ERRORS (at_command_expect_ok ("WR"));
  
  HANDLE_ERRORS (wx_exit_at_command_mode ());

  return TRUE;
}
