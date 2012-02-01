#ifndef LCD_H
#define LCD_H

#include <inttypes.h>
#include <stddef.h>

void
lcd_begin (uint8_t cols, uint8_t lines);

void
lcd_init (void);

// Clear the display (its contents are removed).
void
lcd_clear (void);

// Move the cursor to the column 0, line 0 of the display.  Note that by
// default no visual indication of the cursor position is given.
void
lcd_home (void);

// Move the cursor to the given (zero-based) column and row.  Note that by
// default no visual indication of the cursor position is given.
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

void
lcd_createChar(uint8_t, uint8_t[]);

// There are at least a couple potentially useful characters that aren't
// normal ASCII characters that we can output with lcd_write, assuming we
// have an LCD with ROM code A00, which determines the mapping between some
// non-ASCII bytes and corresponding LCD characters.  Unfortunately, this
// implementation probably can't verify the ROM code of a connected LCD,
// since we don't support reading from the LCD.
#define LCD_CHARACTER_RIGHT_ARROW 0x7E
#define LCD_CHARACTER_LEFT_ARROW 0x7F

// Write a single character to the LCD at the current cursor position.
// NOTE: newline characters ('\n') don't do anything useful.
size_t
lcd_write (uint8_t);

// FIXME: sort out char vs. uint8_t nonsense
// Write a fixed string to the LCD at the current cursor position, and
// return the number of characters written.  NOTE: newline characters ('\n')
// don't do anything useful.
size_t
lcd_write_string (const char *buffer);

// Messages for lcd_printf that are longer than this somewhat arbitrary
// lengh will be truncated.  FIXME: what can the LCD actually handle?
// The current limitation comes from the length of string handler used.
#define LCD_PRINTF_MAX_MESSAGE_LENGTH 100

// Print a printf-style formatted string at the current cursor position.
// NOTE: newline characters ('\n') don't do anything useful.
int
lcd_printf (const char *format, ...)
  __attribute__ ((format (printf, 1, 2)));

// Like lcd_printf, but expects a format string that resides in program
// memory space (probably declared using the AVR libc PSTR macro, for example
// 'lcd_printf_P (PSTR ("foo: %d"), some_int)').
int
lcd_printf_P (const char *format, ...)
  __attribute__ ((format (printf, 1, 2)));


#endif // LCD_H
