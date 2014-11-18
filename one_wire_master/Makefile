include run_screen.mk

AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm

include generic.mk

# FIXME: USE OWM_ prefix for test conditions?  dio doesn't do it...

# The one_wire_master.h interface requires that we define ONE_WIRE_MASTER_PIN
# at compile time: in this case we choose to use DIO_PIN_DIGITAL_2
CPPFLAGS += -DOWM_PIN=DIO_PIN_DIGITAL_2

# Exactly one of the following OWM_TEST_CONDITION_* macros must be defined.

CPPFLAGS += -DOWM_TEST_CONDITION_SINGLE_SLAVE

# Uncomment these to test a multi-slave bus configuration.
#CPPFLAGS += -DOWM_TEST_CONDITION_MULTIPLE_SLAVES

# For the multiple-slave tests to work, you must determine your actual
# slave IDs beforehand and fill them in here.  The test output for the
# OWM_TEST_CONDITION_SINGLE_SLAVE code outputs the ID of the slave it finds
# -- it can be run multiple times with different slaves devices to find
# all their IDs.  Note the IDs must also be assigned correctly to first and
# second positions with respect to the discovery order of the one-wire bus
# search algorithm.  It might be easiest to try both orders :) The individual
# bytes of these IDs should be given in the order they occur in the ID.
# The test program takes care of swapping them to account for endianness.
#CPPFLAGS += -DOWM_FIRST_SLAVE_ID=0x28542a8e060000b4
#CPPFLAGS += -DOWM_SECOND_SLAVE_ID=0x289ae53005000015

# Ensure that exactly one test condition is defined.
ifneq ($(words $(filter -DOWM_TEST_CONDITION_%,$(CPPFLAGS))),1)
  $(error Exactly one OWM_TEST_CONDITION_* must be defined)
endif

# This is only required by the one_wire_master.c.probe program, which isn't
# normally required by users.
#CPPFLAGS += -DTIMER1_STOPWATCH_PRESCALER_DIVIDER=8
