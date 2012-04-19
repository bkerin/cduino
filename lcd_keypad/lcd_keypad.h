// FIXME: not finished.
//
// Interface to the DFRobot DFR0009 LCD and keypad shield.  This shield
// features a Hitachi HD44780 compatible LCD, and five pushbuttons (labeled
// select, left, right, up, and down).  The highest level routine in this
// module is lcd_keypad_set_parameter(), take a look at that routine's
// comments to get an idea of the overall interface features.

#ifndef LCD_KEYPAD_H
#define LCD_KEYPAD_H

// Initialized the LCD and ADC devices.
void
lcd_keypad_init (void);

// FIXME: not finished.
// FIXME: this routine get referrred to in the header comment, it needs
// to continue existing or that comment changed.
//
// Prompt user to set parameter name, which begins with initial value.
// The value is changed with the up and down buttons, and the routine
// returns when the select button is pushed.
void
lcd_keypad_set_parameter (const char *name, double *value);

#endif // LCD_KEYPAD_H
