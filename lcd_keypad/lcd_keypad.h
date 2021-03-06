// Interface to the DFRobot DFR0009 LCD and keypad shield.
//
// Test driver: lcd_keypad_test.c    Implementation: lcd_keypad.c
//
// This shield features a Hitachi HD44780 compatible LCD, and five pushbuttons
// (labeled select, left, right, up, and down).  The highest level routine
// in this module is lcd_keypad_set_value(), take a look at that routine's
// comments to get an idea of the overall interface features.

#ifndef LCD_KEYPAD_H
#define LCD_KEYPAD_H

#include "lcd.h"

// The keypad uses a string resistor ladder network connected to this ADC
// pin to detect button presses.
#define LCD_KEYPAD_ADC_PIN 0

// Initialize the LCD and ADC devices.  The ADC reference is always
// initialized to ADC_REFERENCE_AVCC.  After this routine is called,
// routines from lcd.h can be called freely (without calling lcd_init),
// though of course many of the routines in this interface manipulate the LCD
// contents themselves.  The interactions are simple and hopefully obvious.
void
lcd_keypad_init (void);

// The various buttons on the keypad, plus two special related values:
// LCD_KEYPAD_BUTTON_NONE which corresponds to the condition in which no
// button is pressed, and LCD_KAYPAD_BUTTON_INDETERMINATE, which arises
// when the state of buttons appears to have changed during the reading.
typedef enum {
  LCD_KEYPAD_BUTTON_RIGHT = 0,
  LCD_KEYPAD_BUTTON_UP = 1, 
  LCD_KEYPAD_BUTTON_DOWN = 2, 
  LCD_KEYPAD_BUTTON_LEFT = 3, 
  LCD_KEYPAD_BUTTON_SELECT = 4,
  LCD_KEYPAD_BUTTON_NONE = 5,
  LCD_KEYPAD_BUTTON_INDETERMINATE
} lcd_keypad_button_t;

// See description of lcd_keypad_button_name().
#define LCD_KEYPAD_MAX_BUTTON_NAME_LENGTH 13

// Set name to the (null-byte terminated) string name of button
// ("RIGHT", "UP", etc.).  The name array must be able hold at least
// LCD_KEYPAD_MAX_BUTTON_NAME_LENGTH + 1 bytes.
void
lcd_keypad_button_name (lcd_keypad_button_t button, char *name);

// NOTE: this is a fairly low-level routine, lcd_keypad_wait_for_button()
// is probably more useful in most circumstances.
//
// Check to see if a button is currently pressed, and return the
// button, or LCD_KEYPAD_BUTTON_NONE if no button is currently pressed,
// or LCD_KEYPAD_BUTTON_INDETERMINATE if it seems that the buttons are
// changing state.  Due to the way the hardware resistors/ADC input are
// connected, if multiple buttons are pressed simultaneously, the one
// nearest the top of the lcd_keypad_button_t enumeration is returned.
lcd_keypad_button_t
lcd_keypad_check_buttons (void);

// Wait in a busy loop until a button (other than LCD_KEYPAD_BUTTON_NONE)
// is pressed, and return that button.  Note that overlapping button presses
// can't be handled using the voltage-divider-and-ADC approach.  Also,
// since clients will likely want to wait for buttons in a loop, we would
// like this routine to return a single button press event for one actual
// press in this case without the clients having to worry about timing.
// This routine therefore returns when the button is released, not when it
// is first pressed.  This is still a pretty natural-feeling approach given
// the tiny momentary push-buttons involved.  Also note that this routine
// will catch button releases even when the corresponding depression occurs
// before it is called.
lcd_keypad_button_t
lcd_keypad_wait_for_button (void);

// The name of a value to be shown using lcd_keypad_show_value or set using
// lcd_keypad_set_value() should not be longer than this (not including
// the trailing null byte).  Longer value names will be truncated.
#define LCD_KEYPAD_VALUE_NAME_MAX_LENGTH 15

// The printf format string used by lcd_keypad_show_value() and
// lcd_keypad_set_value().  Clients hopefully shouldn't have to care.
#define LCD_KEYPAD_VALUE_DISPLAY_FORMAT "  %-14.6g"

// Display the given named value until a button is released, then return
// the button pushed (leaving the display unchanged).
lcd_keypad_button_t
lcd_keypad_show_value (const char *name, double *value);

// Clear display and prompt user to set the named value.  The value is
// changed with the up and down buttons, and the routine returns when one
// of the other three buttons is pushed, with that other button as the
// return value (and the display left unchanged).  The value is changed by
// step per button push, unless the button is held down, in which case the
// value starts changing rapidly while the button remains down.
lcd_keypad_button_t
lcd_keypad_set_value (const char *name, double *value, double step);

// FIXXME: should have method to get strings and maybe some specialized
// methods to get things like email addresses, IP addresses, etc.

#endif // LCD_KEYPAD_H
