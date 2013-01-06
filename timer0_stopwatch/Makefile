AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm

include generic.mk
include run_screen.mk

# This can be used to change the overflow counter or readout types at
# compile time; see the comments in timer0_stopwatch.h.
#CPPFLAGS += -DTIMER0_STOPWATCH_OCT=uint64_t \
#            -DTIMER0_STOPWATCH_RT=uint64_t
