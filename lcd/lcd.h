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

void
lcd_leftToRight(void);

void
lcd_rightToLeft(void);

void
lcd_autoscroll(void);

void
lcd_noAutoscroll(void);

void
lcd_createChar(uint8_t, uint8_t[]);

void
lcd_setCursor(uint8_t, uint8_t); 

size_t
lcd_write(uint8_t);

// Messages for lcd_printf that are longer than this somewhat arbitrary
// lengh will be truncated.
#define LCD_PRINTF_MAX_MESSAGE_LENGTH 100

int
lcd_printf (const char *format, ...)
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
