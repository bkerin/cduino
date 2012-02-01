#ifndef LCD_H
#define LCD_H

#include <inttypes.h>
#include <stddef.h>

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

void
lcd_begin (uint8_t cols, uint8_t lines);

void
lcd_init (void);

void
lcd_clear (void);

void
lcd_home (void);

void
lcd_noDisplay (void);

void
lcd_display (void);

void
lcd_noBlink (void);
  
void
lcd_blink(void);

void
lcd_noCursor(void);

void
lcd_cursor(void);

void
lcd_scrollDisplayLeft(void);

void
lcd_scrollDisplayRight(void);

// Set display to expect text that flows from left to right (i.e. the cursor
// moves right after a character is output).  This is the default mode.
void
lcd_left_to_right_mode (void);

// Set display to expect text that flows from right to left (i.e. the cursor
// moves left after a character is output).  This is probably pretty useless
// without wide character support, but who knows.
void
lcd_right_to_left_mode (void);

// Set display to scroll one step for each character output.  Note that
// for routines like lcd_printf() which output a full string all at once,
// the scolling will be pretty instantaneous, so there won't be time to read
// anything that ends up off the screen.  For this to be useful, lcd_write()
// must be used with an existing string in a timed loop.
void
lcd_autoscroll_mode (void);

// Set display to not scroll one step for each character output.  This is
// the default mode.
void
lcd_no_autoscroll_mode (void);

void
lcd_createChar(uint8_t, uint8_t[]);

void
lcd_setCursor(uint8_t, uint8_t); 

size_t
lcd_write(uint8_t);

// Messages for lcd_printf that are longer than this somewhat arbitrary
// lengh will be truncated.
#define LCD_PRINTF_MAX_MESSAGE_LENGTH 100

// Print some characters at the current cursor position.
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

void
lcd_command (uint8_t);


// uh oh, write() was declared virtual and we had some BS using Print::write;
// in the public part of the class.  and the class declaration looked like this:
// class LiquidCrystal : public Print {

#endif // LCD_H
