#ifndef LCD_H
#define LCD_H

#include <inttypes.h>
#include <stddef.h>

void
lcd_begin (uint8_t cols, uint8_t lines);

void
lcd_init (void);

void
lcd_clear (void);

void
lcd_home (void);

void
lcd_display_off (void);

void
lcd_display_on (void);

void
lcd_noBlink (void);
  
void
lcd_blink(void);

void
lcd_noCursor(void);

void
lcd_cursor(void);

void
lcd_scroll_left (void);

void
lcd_scroll_right (void);

void
lcd_createChar(uint8_t, uint8_t[]);

void
lcd_setCursor(uint8_t, uint8_t); 

// Write a single character to the LCD at the current cursor position.
// NOTE: newline characters ('\n') don't do anything useful.
size_t
lcd_write (uint8_t);

// Messages for lcd_printf that are longer than this somewhat arbitrary
// lengh will be truncated.
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

// FIXME: sort out char vs. uint8_t nonsense
size_t
lcd_write_string (const char *buffer);

#endif // LCD_H
