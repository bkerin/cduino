
include run_screen.mk

# Defining this variable at compile time will cause the term_io module
# to be used from timer1_stopwatch_test.c (and possibly elsewhere) for
# debug logging.
#TIMER1_STOPWATCH_DEBUG = defined
ifdef TIMER1_STOPWATCH_DEBUG
  CPPFLAGS += -DTIMER1_STOPWATCH_DEBUG
  AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm
endif

include generic.mk
