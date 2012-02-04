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

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1 
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

static uint8_t _displayfunction;
static uint8_t _displaycontrol;
static uint8_t _displaymode;
static uint8_t _numlines, _currline;

/************ low level data pushing command  **********/
static void
pulseEnable (void) {
  LCD_ENABLE_SET_LOW ();
  _delay_us (1);

  LCD_ENABLE_SET_HIGH ();
  _delay_us (1);

  LCD_ENABLE_SET_LOW ();
  _delay_us (100);   // commands need > 37us to settle
}

/************ low level data pushing command **********/
static void
write4bits (uint8_t value)
{
  LCD_DATA0_INIT (DIO_OUTPUT, DIO_DONT_CARE, (value >> 0) & 0x01);
  LCD_DATA1_INIT (DIO_OUTPUT, DIO_DONT_CARE, (value >> 1) & 0x01);
  LCD_DATA2_INIT (DIO_OUTPUT, DIO_DONT_CARE, (value >> 2) & 0x01);
  LCD_DATA3_INIT (DIO_OUTPUT, DIO_DONT_CARE, (value >> 3) & 0x01);

  pulseEnable();
}

/************ low level data pushing command **********/
static void
send (uint8_t value, uint8_t mode)
{
  LCD_RS_SET (mode);

  assert (! (_displayfunction & LCD_8BITMODE));
  write4bits(value >> 4);
  write4bits(value);
}

/*********** mid level command, for sending cmds */
static void
command (uint8_t value)
{
  send (value, LOW);
}

/*********** mid level command, for sending data*/
size_t
lcd_write (uint8_t value)
{
  send (value, HIGH);
  return 1;   // Assume success
}

int
lcd_printf (const char *format, ...)
{
  char message_buffer[LCD_PRINTF_MAX_MESSAGE_LENGTH + 1];

  va_list ap;
  va_start (ap, format);
  int chars_written
    = vsnprintf (message_buffer, LCD_PRINTF_MAX_MESSAGE_LENGTH, format, ap);
  va_end (ap);

  lcd_write_string (message_buffer);

  return chars_written;
}

int
lcd_printf_P (const char *format, ...)
{
  char message_buffer[LCD_PRINTF_MAX_MESSAGE_LENGTH + 1];

  va_list ap;
  va_start (ap, format);
  int chars_written
    = vsnprintf_P (message_buffer, LCD_PRINTF_MAX_MESSAGE_LENGTH, format, ap);
  va_end (ap);

  lcd_write_string (message_buffer);

  return chars_written;
}

size_t
lcd_write_string (const char *buffer)
{
  // FIXME: sort out casting mess
  size_t size = strlen ((const char *) buffer);
  size_t n = 0;
  while (size--) {
    n += lcd_write(*buffer++);
  }
  return n;
}

void
lcd_init(void)
{
  LCD_RS_INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  LCD_ENABLE_INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  
  _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  
  lcd_begin(16, 1);  
}

void
lcd_begin (uint8_t cols, uint8_t lines) {
  cols = cols;   // Keep compiler happy.
  if (lines > 1) {
    _displayfunction |= LCD_2LINE;
  }
  _numlines = lines;
  _currline = 0;

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!  According to the
  // datasheet, we need at least 40ms after power rises above 2.7V before
  // sending commands. And arduino can turn on way befer 4.5V so we'll wait 50
  // ms.
  _delay_ms (50);
  
  // We pull both RS and R/W low to begin commands.
  LCD_RS_SET_LOW ();
  LCD_ENABLE_SET_LOW ();
  
  //put the LCD into 4 bit mode
  assert (! (_displayfunction & LCD_8BITMODE));

  // This is done according to the hitachi HD44780 datasheet figure 24, pg 46.
  // We start in 8bit mode, then try to set 4 bit mode.
  write4bits (0x03);
  _delay_us (4500);
  // Second try
  write4bits (0x03);
  _delay_us (4500);
  // Third go!
  write4bits (0x03); 
  _delay_us (150);
  // Finally, set to 4-bit interface
  write4bits (0x02); 

  // Finally, set # lines, font size, etc.
  command (LCD_FUNCTIONSET | _displayfunction);  

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;  
  lcd_display_on ();

  // Clear display. 
  lcd_clear ();

  // Initialize to default text direction (for romance languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  command (LCD_ENTRYMODESET | _displaymode);
}

/********** high level commands, for the user! */
void lcd_clear(void)
{
  command (LCD_CLEARDISPLAY);  // Clear display, set cursor position to zero.
  _delay_us (2000);   // This command takes a long time.
}

void
lcd_home (void)
{
  command (LCD_RETURNHOME);  // set cursor position to zero
  _delay_us (2000);  // This command takes a long time.
}

void
lcd_set_cursor_position (uint8_t col, uint8_t row)
{
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if ( row >= _numlines ) {
    row = _numlines-1;    // We count rows starting from 0.
  }
  
  command (LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void
lcd_display_off (void)
{
  _displaycontrol &= ~LCD_DISPLAYON;
  command (LCD_DISPLAYCONTROL | _displaycontrol);
}

void
lcd_display_on (void)
{
  _displaycontrol |= LCD_DISPLAYON;
  command (LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void
lcd_underline_cursor_off (void)
{
  _displaycontrol &= ~LCD_CURSORON;
  command (LCD_DISPLAYCONTROL | _displaycontrol);
}
void
lcd_underline_cursor_on (void)
{
  _displaycontrol |= LCD_CURSORON;
  command (LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void
lcd_blinking_cursor_off (void)
{
  _displaycontrol &= ~LCD_BLINKON;
  command (LCD_DISPLAYCONTROL | _displaycontrol);
}
void
lcd_blinking_cursor_on (void)
{
  _displaycontrol |= LCD_BLINKON;
  command (LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void
lcd_scroll_left (void) {
  command (LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void
lcd_scroll_right (void) {
  command (LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}
