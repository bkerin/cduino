// Implementation of the interface described in lcd.h.

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include <inttypes.h>

#include "dio.h"
#include "lcd.h"

// Note, however, that resetting the Arduino doesn't reset the LCD, so
// we can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

// NOTE: many of these command and flags aren't currently used.  They serve
// to illustrate somewhat all the HD44780 functionality that we *don't*
// support :)

// Commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// Flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// Flags for display/cursor on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// Flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// Flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

static uint8_t functionset_flags;
static uint8_t displaycontrol_flags;
static uint8_t entrymodeset_flags;

#define LCD_DISPLAY_LINES 2

// NOTE: resetting the Arduino doesn't necessarily reset the LCD.  So its
// possible to trick yourself about whether a test is working or not while
// developing.  The Makefile for this module contains a silly little target
// the increments a number in the display setting string as one way to test
// the most basic functionality of the LCD.

// This is used to signal the LCD that data is ready on the data pins.
static void
pulse_enable (void) {
  LCD_ENABLE_SET_LOW ();
  _delay_us (1);

  LCD_ENABLE_SET_HIGH ();
  _delay_us (1);

  LCD_ENABLE_SET_LOW ();
  _delay_us (100);   // Commands need > 37us to settle
}

// Write four bits of data to the LCD.  This could be part of a command or
// part of a character of text.
static void
write_4_bits (uint8_t value)
{
  // Re-initializing these pins every time seems a bit weird to me, but
  // the original LiquidCrystal module from Arduino-1.0 does it, so perhaps
  // there is a good reason.  Or perhaps its for pin sharing, which sounds
  // a bit crazed but could work I guess (I haven't investigated it at all).
  LCD_DB4_INIT (DIO_OUTPUT, DIO_DONT_CARE, (value >> 0) & 0x01);
  LCD_DB5_INIT (DIO_OUTPUT, DIO_DONT_CARE, (value >> 1) & 0x01);
  LCD_DB6_INIT (DIO_OUTPUT, DIO_DONT_CARE, (value >> 2) & 0x01);
  LCD_DB7_INIT (DIO_OUTPUT, DIO_DONT_CARE, (value >> 3) & 0x01);

  pulse_enable ();
}

// Send eight bits of data to the LCD.  This could be a command or text data.  
static void
send (uint8_t value, uint8_t mode)
{
  LCD_RS_SET (mode);

  assert (! (functionset_flags & LCD_8BITMODE));
  write_4_bits (value >> 4);
  write_4_bits (value);
}

// Send an eight bit command to the LCD.
static void
command (uint8_t value)
{
  send (value, LOW);
}

void
lcd_init (void)
{
  LCD_RS_INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  LCD_ENABLE_INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  
  functionset_flags = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!  According to the
  // datasheet, we need at least 40ms after power rises above 2.7V before
  // sending commands. And arduino can turn on way befer 4.5V so we'll wait
  // 50 ms.
  _delay_ms (50);
  
  // We pull both RS and R/W low to begin commands.
  LCD_RS_SET_LOW ();
  LCD_ENABLE_SET_LOW ();
 
  // This is done according to the Hitachi HD44780 datasheet figure 24, pg 46.
  // NOTE: Except it seems to me that what was used in LiquidCrystal.cpp in
  // the arduino-1.0 source didn't match what the datasheet required: the
  // datasheet shows only on ~5 ms wait, and doesn't show any wait after the
  // last try.  But this is presumably tried and tested code and seems likely
  // to be a safe deviation from the datasheet anyway, so we'll keep it.
  write_4_bits (0x03);
  _delay_ms (5);
  // Second try
  write_4_bits (0x03);
  _delay_ms (5);
  // Third go!
  write_4_bits (0x03); 
  _delay_us (150);
  // Finally, set to 4-bit interface.
  write_4_bits (0x02); 

  // NOTE: By my reading of the above datasheet, the initialization timing
  // should look like the below code.  But the Arduino library way has
  // presumably been widely tested and works and has some reason behind it.
  //
  //write_4_bits (0x03);
  //_delay_ms (5);
  //
  //write_4_bits (0x03);
  //_delay_us (150);
  //
  //write_4_bits (0x03); 
  //write_4_bits (0x02);

  // NOTE: The order of these next commands may be important at first
  // initialization.  It would be neater to perform them using our wrapper
  // functions, but for all I know there is some reason they need to be
  // performed in single commands.

  // Finally, set # lines, font size, etc.
  command (LCD_FUNCTIONSET | functionset_flags);  

  // Turn the display on with no cursor or blinking cursor.
  displaycontrol_flags = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;  
  command (LCD_DISPLAYCONTROL | displaycontrol_flags);

  // Clear display. 
  lcd_clear ();

  // Initialize to supported text direction (for romance languages).
  entrymodeset_flags = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  command (LCD_ENTRYMODESET | entrymodeset_flags);
}

void
lcd_clear (void)
{
  command (LCD_CLEARDISPLAY);  // Clear display, set cursor position to zero.
  _delay_us (2000);   // This command takes a long time.
}

void
lcd_home (void)
{
  // Set cursor position to zero and undo any scrolling that is in effect.
  command (LCD_RETURNHOME);  
  _delay_us (2000);  // This command takes a long time.
}

void
lcd_set_cursor_position (uint8_t col, uint8_t row)
{
  // If given an invalid row number, display on last line.
  if ( row >= LCD_DISPLAY_LINES ) {
    row = LCD_DISPLAY_LINES - 1;    // We count rows starting from 0.
  }

  // Positions of the beginnings of rows in LCD DRAM.
  const int const row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  
  command (LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void
lcd_display_off (void)
{
  displaycontrol_flags &= ~LCD_DISPLAYON;
  command (LCD_DISPLAYCONTROL | displaycontrol_flags);
}

void
lcd_display_on (void)
{
  displaycontrol_flags |= LCD_DISPLAYON;
  command (LCD_DISPLAYCONTROL | displaycontrol_flags);
}

void
lcd_blinking_cursor_off (void)
{
  displaycontrol_flags &= ~LCD_BLINKON;
  command (LCD_DISPLAYCONTROL | displaycontrol_flags);
}

void
lcd_blinking_cursor_on (void)
{
  displaycontrol_flags |= LCD_BLINKON;
  command (LCD_DISPLAYCONTROL | displaycontrol_flags);
}


void
lcd_underline_cursor_off (void)
{
  displaycontrol_flags &= ~LCD_CURSORON;
  command (LCD_DISPLAYCONTROL | displaycontrol_flags);
}

void
lcd_underline_cursor_on (void)
{
  displaycontrol_flags |= LCD_CURSORON;
  command (LCD_DISPLAYCONTROL | displaycontrol_flags);
}

void
lcd_scroll_left (void) {
  command (LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void
lcd_scroll_right (void) {
  command (LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

size_t
lcd_write (char value)
{
  send ((uint8_t) value, HIGH);
  return 1;   // Assume success
}

int
lcd_printf (const char *format, ...)
{
  char message_buffer[LCD_MAX_MESSAGE_LENGTH + 1];

  va_list ap;
  va_start (ap, format);
  int chars_written
    = vsnprintf (message_buffer, LCD_MAX_MESSAGE_LENGTH, format, ap);
  va_end (ap);

  lcd_write_string (message_buffer);

  return chars_written;
}

int
lcd_printf_P (const char *format, ...)
{
  char message_buffer[LCD_MAX_MESSAGE_LENGTH + 1];

  va_list ap;
  va_start (ap, format);
  int chars_written
    = vsnprintf_P (message_buffer, LCD_MAX_MESSAGE_LENGTH, format, ap);
  va_end (ap);

  lcd_write_string (message_buffer);

  return chars_written;
}

size_t
lcd_write_string (const char *buffer)
{
  size_t size = strlen (buffer);
  size_t n = 0;
  while ( size-- ) {
    n += lcd_write ((uint8_t) (*buffer++));
  }

  return n;
}

