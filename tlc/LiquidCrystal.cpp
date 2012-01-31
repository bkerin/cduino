#include "LiquidCrystal.h"

#include <assert.h>
#include <avr/delay.h>
#include <string.h>
#include <inttypes.h>

extern "C" {
  #include "dio.h"
}

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

static uint8_t _displayfunction;
static uint8_t _displaycontrol;
static uint8_t _displaymode;
static uint8_t _initialized;
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

/*********** mid level command, for sending data/cmds */
void
command (uint8_t value)
{
  send (value, LOW);
}

/*********** mid level command, for sending data/cmds */
size_t
write (uint8_t value)
{
  send (value, HIGH);
  return 1;   // Assume success
}

size_t
write_string (const char *buffer)
{
  // FIXME: sort out casting mess
  size_t size = strlen ((const char *) buffer);
  size_t n = 0;
  while (size--) {
    n += write(*buffer++);
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
  if (lines > 1) {
    _displayfunction |= LCD_2LINE;
  }
  _numlines = lines;
  _currline = 0;

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!  According to the
  // datasheet, we need at least 40ms after power rises above 2.7V before
  // sending commands. And arduino can turn on way befer 4.5V so we'll wait 50
  // ms.
  _delay_ms (50); //delayMicroseconds(50000); 
  
  // Now we pull both RS and R/W low to begin commands
  LCD_RS_SET_LOW ();//dio_pin_set ('B', 0, 0);
  LCD_ENABLE_SET_LOW ();//dio_pin_set ('B', 1, 0);
  
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
  command(LCD_FUNCTIONSET | _displayfunction);  

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;  
  display();

  // Clear display. 
  clear();

  // Initialize to default text direction (for romance languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);
}

/********** high level commands, for the user! */
void clear(void)
{
  command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
  _delay_us (2000);//delayMicroseconds(2000);  // this command takes a long time!
}

void home(void)
{
  command(LCD_RETURNHOME);  // set cursor position to zero
  _delay_us (2000);//delayMicroseconds(2000);  // this command takes a long time!
}

void
setCursor (uint8_t col, uint8_t row)
{
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if ( row >= _numlines ) {
    row = _numlines-1;    // we count rows starting w/0
  }
  
  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void
noDisplay (void) {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void
display(void) {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void
noCursor(void) {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void
cursor(void) {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void
noBlink (void) {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void
blink (void) {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void
scrollDisplayLeft (void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void
scrollDisplayRight (void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void
leftToRight (void) {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void
rightToLeft (void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void
autoscroll (void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void
noAutoscroll (void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void
createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) {
    write(charmap[i]);
  }
}
