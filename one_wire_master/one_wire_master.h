
#ifndef ONE_WIRE_MASTER_H
#define ONE_WIRE_MASTER_H

// This interface actually supports multiple instances (partly just because
// it's a convenient way to store the port/pin details being used, partly
// because there's enough hardware to support multiple instances and that
// might actually be useful).
typedef struct {
   int porrrt;
   int pin;
   int reg;
   int blah;
} OneWireMaster;

int
owm_touch_reset (OneWireMaster *owm);

// FIXME: narrow datatypes where possible

// FIXME: move descriptions of interface functions into this header
void
owm_write_byte (OneWireMaster *owm, int data);

int
owm_read_byte (OneWireMaster *owm);

// FIXME: do we like this name?
int
owm_touch_byte (OneWireMaster *owm, int data);

#endif // ONE_WIRE_MASTER_H
