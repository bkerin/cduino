
#ifndef ONE_WIRE_MASTER_H
#define ONE_WIRE_MASTER_H

#include "dio.h"

// This interface actually supports multiple instances (partly just because
// it's a convenient way to store the port/pin details being used, partly
// because there's enough hardware to support multiple instances and that
// might actually be useful).
// FIXME: do we maye want to just use a typedef between OneWireMaster and
// dio_pin_t here?
typedef struct {
  dio_pin_t pin;
} OneWireMaster;

OneWireMaster *
owm_new (dio_pin_t pin);

// Generate a 1-Wire reset.  Return 1 if no presence detect was found,
// or 0 otherwise.  (NOTE: Does not handle alarm presence from DS2404/DS1994)
uint8_t
owm_touch_reset (OneWireMaster *owm);

// Write 1-Wire data byte
void
owm_write_byte (OneWireMaster *owm, uint8_t data);

// Read 1-Wire data byte and return it
uint8_t
owm_read_byte (OneWireMaster *owm);

// FIXME: do we like this name?
// Write a 1-Wire data byte and return the sampled result
uint8_t
owm_touch_byte (OneWireMaster *owm, uint8_t data);

#endif // ONE_WIRE_MASTER_H
