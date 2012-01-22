// Set input/output mode and pullup status of digitla IO pins, and set or
// read their values.

#ifndef DIGITAL_IO_PIN_H
#define DIGITAL_IO_PIN_H

#include <inttypes.h>

// WARNING: Not all the digital IO pins are available for use when using
// an Arduino with the normal bootloader pin configuration.  PD0 and PD1
// are set up for serial communication, and won't work as general digital
// IO pins (at least without disturbing something else).
//
// The pin names we use are the unparenthesized pin labels from the
// ATMega328p datasheet.  The assigned numbers are the digital pin numbers
// printed on (at least some) arduino boards (so they can be used instead if
// the user likes).  For the PB6:7 and PC0:5 pins, my arduino doesn't have
// any specific digital IO-related mark, so these pins have no guaranteed
// value in this enumeration.
typedef enum {
  DIGITAL_IO_PIN_PB0 = 8, 
  DIGITAL_IO_PIN_PB1 = 9,
  DIGITAL_IO_PIN_PB2 = 10, 
  DIGITAL_IO_PIN_PB3 = 11, 
  DIGITAL_IO_PIN_PB4 = 12,
  DIGITAL_IO_PIN_PB5 = 13,
  DIGITAL_IO_PIN_PB6,
  DIGITAL_IO_PIN_PB7,
  DIGITAL_IO_PIN_PC0,
  DIGITAL_IO_PIN_PC1,
  DIGITAL_IO_PIN_PC2,
  DIGITAL_IO_PIN_PC3,
  DIGITAL_IO_PIN_PC4,
  DIGITAL_IO_PIN_PC5,
  DIGITAL_IO_PIN_PD0 = 0,
  DIGITAL_IO_PIN_PD1 = 1, 
  DIGITAL_IO_PIN_PD2 = 2, 
  DIGITAL_IO_PIN_PD3 = 3, 
  DIGITAL_IO_PIN_PD4 = 4, 
  DIGITAL_IO_PIN_PD5 = 5,
  DIGITAL_IO_PIN_PD6 = 6, 
  DIGITAL_IO_PIN_PD7 = 7
} digital_io_pin_name_t;

typedef enum {
  DIGITAL_IO_PIN_DIRECTION_INPUT,
  DIGITAL_IO_PIN_DIRECTION_OUTPUT
} digital_io_pin_direction_t;

// Initialize pin of port for input or output (as per direction argument).
// If the pin is configured for input, then enable_pullup determines whether
// the internal pullup resistor is enabled.  If configured for ouput,
// initial_value determines the initial value of the pin.
void
digital_io_pin_init (
    digital_io_pin_name_t        pin,
    digital_io_pin_direction_t   direction,
    uint8_t                      enable_pullup,
    uint8_t                      initial_value );

// Initialize pin of port for input or output (as per direction argument).
// If the pin is configured for input, then enable_pullup determines whether
// the internal pullup resistor is enabled.  If configured for ouput,
// initial_value determines the initial value of the pin.
void
dio_pin_initialize (char port, uint8_t pin, digital_io_pin_direction_t direction, 
  uint8_t enable_pullup, int8_t initial_value);

// Set output pin of port to value.
void
dio_pin_set (char port, uint8_t pin, uint8_t value);

#endif // DIGITAL_IO_PIN_H
