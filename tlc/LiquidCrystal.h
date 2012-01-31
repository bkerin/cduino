#ifndef LCD_H
#define LCD_H

#include <inttypes.h>
#include "Print.h"

extern "C" {
  #include "dio.h"
}

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
clear (void);

void
home (void);

void
noDisplay (void);

void
display (void);

void
noBlink (void);
  
void
blink();

void
noCursor();

void
cursor();

void
scrollDisplayLeft();

void
scrollDisplayRight();

void
leftToRight();

void
rightToLeft();

void
autoscroll();

void
noAutoscroll();


void
createChar(uint8_t, uint8_t[]);

void
setCursor(uint8_t, uint8_t); 

size_t
write(uint8_t);

// FIXME: sort out char vs. uint8_t nonsense
size_t
write_string (const char *buffer);

// FIXME: was command public in original interface?
void
command (uint8_t);


// uh oh, write() was declared virtual and we had some BS using Print::write;
// in the public part of the class.  and the class declaration looked like this:
// class LiquidCrystal : public Print {

#endif // LCD_H
