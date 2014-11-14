include run_screen.mk

AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm

include generic.mk

# FIXME: copied things rather then link, all these test programs should
# get moved into the one_wire_slave project anyway

# The one_wire_master.h interface requires that we define ONE_WIRE_MASTER_PIN
# at compile time: in this case we choose to use DIO_PIN_DIGITAL_2
CPPFLAGS += -DOWM_PIN=DIO_PIN_DIGITAL_2

# Exactly one of the following OWM_TEST_CONDITION_* macros must be defined.

CPPFLAGS += -DOWM_TEST_CONDITION_SINGLE_SLAVE

# FIXME: we made this tighter to allow for more timing accuracy.  Thes tests
# don't cover this timing, ticks twice per us at arduino freq (16 MHz)
CPPFLAGS += -DTIMER1_STOPWATCH_PRESCALER_DIVIDER=8

