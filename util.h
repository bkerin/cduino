// Generally useful stuff for AVR programming.

#ifndef MYLIB_H
#define MYLIB_H

#define HIGH 0x1
#define LOW  0x0

// FIXME: what about putting a blink blink routine in here for use as
// checkpoint, also maybe a trap point routine?

// WARNING: of course some contexts might understand things differently...
#define TRUE  0x1
#define FALSE 0x0

#define CLOCK_CYCLES_PER_MICROSECOND() (F_CPU / 1000000L)
#define CLOCK_CYCLES_TO_MICROSECONDS(a) (((a) * 1000L) / (F_CPU / 1000L))
#define MICROSECONDS_TO_CLOCK_CYCLES(a) (((a) * (F_CPU / 1000L)) / 1000L)

#endif // MYLIB_H
