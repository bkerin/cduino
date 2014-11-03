
include run_screen.mk

# Defining this variable will cause the term_io module to be used for
# diagnostic output from timer1_stopwatch_test.c.
TIMER1_STOPWATCH_DEBUG = defined
ifdef TIMER1_STOPWATCH_DEBUG
  CPPFLAGS += -DTIMER1_STOPWATCH_DEBUG
  AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm
endif

# WARNING: the tests in timer1_stopwatch_test.c don't all work correctly for
# small divider settings.  Uncomment this to change the prescalre divider
# setting to be used.  Note that the prescalre is shared with timer0.
# Possible values are 1, 8, 64 (the default) 256, and 1024.
#CPPFLAGS += -DTIMER1_STOPWATCH_PRESCALER_DIVIDER=256

include generic.mk
