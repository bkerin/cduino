// Set input/output mode and pullup status of digitla IO pins, and set or
// read their values.

#ifndef DIO_PIN_H
#define DIO_PIN_H

#include <inttypes.h>

typedef enum {
  DIO_PIN_DIRECTION_INPUT,
  DIO_PIN_DIRECTION_OUTPUT
} dio_pin_direction_t;

// Initialize pin of port for input or output (as per direction argument).
// If the pin is configured for input, then enable_pullup determines whether
// the internal pullup resistor is enabled.  If configured for ouput,
// initial_value determines the initial value of the pin.
void
dio_pin_initialize (char port, uint8_t pin, dio_pin_direction_t direction, 
  uint8_t enable_pullup, int8_t initial_value);

// Set output pin of port to value.
void
dio_pin_set (char port, uint8_t pin, uint8_t value);

#endif // DIO_PIN_H
