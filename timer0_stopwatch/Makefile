AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm

include generic.mk
include run_screen.mk

# This can be used to set the overflow counter type.  It should be an unsigned
# integer type.  Wider types will allow the stopwatch to run longer before
# wrapping around, but involve more overhead per reading.
CPPFLAGS += -DTIMER0_STOPWATCH_OCT=uint32_t
