
include run_screen.mk

# Defining this variable will cause the term_io module to be used for
# diagnostic output from timer1_stopwatch_test.c.
TIMER0_STOPWATCH_DEBUG = defined
ifdef TIMER0_STOPWATCH_DEBUG
  CPPFLAGS += -DTIMER0_STOPWATCH_DEBUG
  AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm
endif

include generic.mk
