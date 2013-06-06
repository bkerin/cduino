// Interface to an 16x2 character HD44780 compatible display.
//
// Test driver: lcd_test.c    Implementation: lcd.c
//
// This interface always uses four bit control.  Only ASCII characters in
// left-to-right text mode are supported.

#ifndef LCD_H
#define LCD_H

#include <inttypes.h>
#include <stddef.h>

// We require clients to set some macros at compile time to specify which pins
// are being used to control the LCD.  The Makefile in the dio module direcory
// shows one way to do this.
#if ! (defined (LCD_RS_INIT) && \
       defined (LCD_RS_SET) && \
       defined (LCD_RS_SET_HIGH) && \
       defined (LCD_RS_SET_LOW) && \
       \
       defined (LCD_ENABLE_INIT) && \
       defined (LCD_ENABLE_SET_HIGH) && \
       defined (LCD_ENABLE_SET_LOW) && \
       \
       defined (LCD_DB4_INIT) && \
       \
       defined (LCD_DB5_INIT) && \
       \
       defined (LCD_DB6_INIT) && \
       \
       defined (LCD_DB7_INIT))
#  error The macros which specify which pins the LCD should use are not set. \
         Please see the example in the Makefile in the dio module directory.
#endif

// Initialize display.  This routine takes about 50 milliseconds (to ensure
// that the input voltage has risen sufficiently for corret display operation,
// in case we are called near power-on).  Note also that some macros must
// be defined at compile-time to control which IO pins the LCD will use
// (see comments above).  The display is cleared and the cursor set to home
// (row 0 column 0).
void
lcd_init (void);

// Clear the display (the underlying content of the LCD is removed).
void
lcd_clear (void);

// Move the cursor to the column 0, line 0 of the display, and remove any
// scrolling that is in effect.  Note that by default no visual indication
// of the cursor position is given.
void
lcd_home (void);

// Move the cursor to the given (zero-based) column and row.  Note that by
// default no visual indication of the cursor position is given.  FIXME:
// this is subject to current scolling I think, verify and document.
void
lcd_set_cursor_position (uint8_t column, uint8_t row); 

// Turn the display off/on (leaving its contents intact but not displayed).
void
lcd_display_off (void);
void
lcd_display_on (void);

// Turn different styles of cursor marks off/on.  By default cursors are off.
void
lcd_blinking_cursor_off (void);
void
lcd_blinking_cursor_on (void);
void
lcd_underline_cursor_off (void);
void
lcd_underline_cursor_on (void);

// Scroll the display window left/right (leaving the contents of undisplayed
// areas unchanged).
void
lcd_scroll_left (void);
void
lcd_scroll_right (void);

// There are at least a couple potentially useful characters that aren't
// normal ASCII characters that we can output with lcd_write, assuming we
// have an LCD with ROM code A00, which determines the mapping between some
// non-ASCII bytes and corresponding LCD characters.  Unfortunately, this
// implementation probably can't verify the ROM code of a connected LCD,
// since we don't support reading from the LCD.
#define LCD_CHARACTER_RIGHT_ARROW 0x7E
#define LCD_CHARACTER_LEFT_ARROW 0x7F

// Messages that are longer than this will probably be truncated.  The HD4470
// spec guarantees only 80 eight bit characters of RAM.  I'm not sure
// if you can put them all on one line or not, so we allow half of that.
// There may be some even tighter limitation of which I'm not aware.
#define LCD_MAX_MESSAGE_LENGTH 40

// Write a single character to the LCD at the current cursor position.
// NOTE: newline characters ('\n') don't do anything useful.  This function
// always returns 1 to indicate that 1 character has been written.  Hopefully.
size_t
lcd_write (char character);

// Write a fixed string to the LCD at the current cursor position, and
// return the number of characters written.  NOTE: newline characters ('\n')
// don't do anything useful.
size_t
lcd_write_string (const char *buffer);

// Print a printf-style formatted string at the current cursor position.
// NOTE: newline characters ('\n') don't do anything useful.
int
lcd_printf (const char *format, ...)
  __attribute__ ((format (printf, 1, 2)));

// Like lcd_printf, but expects a format string that resides in program
// memory space (probably declared using the AVR libc PSTR macro, for example
// 'lcd_printf_P (PSTR ("foo: %d"), some_int)').  This saves some RAM,
// but note that the expanded version of format still gets stored in RAM
// before being sent to the LCD.
int
lcd_printf_P (const char *format, ...)
  __attribute__ ((format (printf, 1, 2)));


#endif // LCD_H
