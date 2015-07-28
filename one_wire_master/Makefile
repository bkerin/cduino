include run_screen.mk

# It's convenient to have both a master and a slave connected at the
# same time, so we can use our master and slave implementations together.
# To support that, we have to use a different port for the second Arduino,
# and not depend on autodetection.  Our build system then requires us to
# specify all of these options.

# These setting are only useful when we want to connect two Arduinos at
# once (in order to test both a 1-wire master and a slave, for example).
# In this situation we can't depend on build system autodetection, but
# must instead state explicitly which device we want to program.  Our build
# system then requires us to specify all of these options.  Note that there
# are some similar variables in the Makefile for the one_wire_slave module
# which must be uncommented and set correctly as well.  Note also that the
# port device file assigned to a given Arduino can depend on the order in
# which it was plugged in, and on other devices present on USB.  In my case
# I plug in the master Arduino first and it ends up as /dev/ttyACM0. I plug
# the slave Arduino in next and it ends up as /dev/ttyACM1.  When in doubt,
# try plugging and unplugging the Arduinos and doing ls /dev to see which
# files appear/dissapear.
#ARDUINO_PORT = /dev/ttyACM0
#ARDUINO_BAUD = 115200
#ARDUINO_BOOTLOADER = optiboot_atmega328.hex

AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm

include generic.mk

# The one_wire_master.h interface requires this to be defined at compile time.
CPPFLAGS += -DOWM_PIN=DIO_PIN_DIGITAL_2

# If this is uncommented, we build a function which fetches a text description
# of a owm_result_t.  This inevitably burns some program memory for the
# strings (whether the function is called or not), so we provide this option
# so it can be disabled easily.
CPPFLAGS += -DOWM_BUILD_RESULT_DESCRIPTION_FUNCTION

# If this is uncommented, the internal pull-up resistor on the IO pin
# will be used, saving you an external 4.7 kohm resistor.  There are
# significant trade-offs associated with using this: see the comments near
# OWM_USE_INTERNAL_PULLUP in one_wire_master.c.
#CPPFLAGS += -DOWM_USE_INTERNAL_PULLUP

# Exactly one of the following TEST_CONDITION_* macros must be defined.

CPPFLAGS += -DTEST_CONDITION_SINGLE_SLAVE
#CPPFLAGS += -DTEST_CONDITION_MULTIPLE_SLAVES

# For the multiple-slave tests to work, you must determine your actual
# slave IDs beforehand and fill them in here.  The test output for the
# TEST_CONDITION_SINGLE_SLAVE code outputs the ID of the slave it finds --
# it can be run multiple times with different slaves devices to find all
# their IDs.  Note the IDs must also be assigned correctly to first and
# second positions with respect to the discovery order of the 1-wire bus
# search algorithm.  It might be easiest to try both orders :) The individual
# bytes of these IDs should be given in the order they occur in the ID.
# The test program takes care of swapping them to account for endianness.
#CPPFLAGS += -DFIRST_SLAVE_ID=0x28542a8e060000b4
#CPPFLAGS += -DSECOND_SLAVE_ID=0x289ae53005000015

# When testing a slave implemented using the one_wire_slave module together
# other slaves, it may be useful to have one of the slave IDs set to
# the ROM ID used by the Makefile for the test program in that module.
# Note that this is slightly different than the default ROM ID used by the
# one_wire_slave.h interface.
#CPPFLAGS += -DFIRST_SLAVE_ID=0x28444444222222bb
#CPPFLAGS += -DSECOND_SLAVE_ID=0x28542a8e060000b4

# Ensure that exactly one test condition is defined.
ifneq ($(words $(filter -DTEST_CONDITION_%,$(CPPFLAGS))),1)
  $(error Exactly one TEST_CONDITION_* must be defined)
endif

# This is only required by the one_wire_master.c.probe program, which isn't
# normally required by users (or up-to-date anymore).
#CPPFLAGS += -DTIMER1_STOPWATCH_PRESCALER_DIVIDER=8
