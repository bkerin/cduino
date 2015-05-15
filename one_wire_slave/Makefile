# vim:foldmethod=marker

include run_screen.mk

# It's convenient to have both a master and a slave connected at the
# same time, so we can use our master and slave implementations together.
# To support that, we have to use a different port for the second Arduino,
# and not depend on autodetection.  Our build system then requires us to
# specify all of these options.  Note that the port device file assigned to
# a given Arduino can depend on the order in which it was plugged in, and on
# other devices present on USB.  When in doubt, try plugging and unplugging
# the Arduinos and doing ls /dev to see which files appear/dissapear.
ARDUINO_PORT = /dev/ttyACM1
ARDUINO_BAUD = 115200
ARDUINO_BOOTLOADER=optiboot_atmega328.hex

# This line must be uncommented to ensure that people using this module
# know that some variables are being locked into user registers.  See the
# reference to OWS_REGISTER_USE_ACKNOWLEDGED in one_wire_slave.h for details.
# FIXME: comment this out again for the public release:
CPPFLAGS += -DOWS_REGISTER_USE_ACKNOWLEDGED

# Uncomment for testing with my particular non-arduino slave breadboard build
ENABLE_NON_ARDUINO_OVERRIDES = defined

##### Orverrides for Non-Arduino Slave {{{1

# Some of the setting in this section are tailored to my specific non-arduino
# test breadboard setup (e.g. DBL_PIN).

ifdef ENABLE_NON_ARDUINO_OVERRIDES

  #CPU_FREQ_DEFINE = -DF_CPU=16000000
  #CPU_FREQ_DEFINE = -DF_CPU=12000000
  CPU_FREQ_DEFINE = -DF_CPU=10000000
  #CPU_FREQ_DEFINE = -DF_CPU=8000000

  # No clock divider, external 0.4-20MHz MHz ceramic resonator clock, and
  # most conservative suggested power rise assumptions to be used without
  # brown-out detection enable.  Note that for ceramic resonators, we use
  # CKSEL3:1 settings that in one datasheet table are described as being for
  # low-power crystal oscillators.  But explanatory text elsewhere indicates
  # that this setting is to be used for resonators as well.
  LOCK_AND_FUSE_SETTINGS = \
     CKDIV8=1 CKSEL3=0 CKSEL2=1 CKSEL1=1 CKSEL0=1 SUT1=0 SUT0=0

  UPLOAD_METHOD = AVRISPmkII

  # Let the build system know that it doesn't need to look out for us by
  # insisting on finding a USB connection to something (FTDI or Uno Rev.3
  # or whatever).
  NO_USB_ARDUINO_CONNECTION = defined_nonempty

endif

# }}}1

AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm

include generic.mk

# Pin to use for debug LED output.
ifndef ENABLE_NON_ARDUINO_OVERRIDES
  # FIXME: this will be the default probably once blinky debug has its
  # own module.  Assuming it seems smart to have it have a default.
  CPPFLAGS += -DDBL_PIN=DIO_PIN_PB5
else
  CPPFLAGS += -DDBL_PIN=DIO_PIN_PD4
endif


# FIXME: USE OWS_ prefix for test conditions?  dio doesn't do it...

# The one_wire_slave.h interface requires this to be defined at compile time.
CPPFLAGS += -DOWS_PIN=DIO_PIN_DIGITAL_2

# This module requires us to use a prescaler setting st we get at least
# 1 us resolution on the timer.  This setting works for 16 MHz (gives a
# 2 MHz timer1).  If the cpu were at 1MHz, would need to use a prescaler
# setting of 1.  FIXME: 1 MHz cpu might not work anyway because of slowness
# running other code, this needs tested/reasoned about a bit.
CPPFLAGS += -DTIMER1_STOPWATCH_PRESCALER_DIVIDER=8

# This is the Maxim family code for the DS18B20.  Since our test/demo
# program emulates a limited DS18B20, we use this family code :) Actually,
# we mainly use it to show how to set your own family code.
CPPFLAGS += -DOWS_FAMILY_CODE=0x28

# Exactly one of the following OWM_TEST_CONDITION_* macros must be defined.

# FIXME: this isnt thw OWM anymore, so remove or change
CPPFLAGS += -DOWM_TEST_CONDITION_SINGLE_SLAVE

# Uncomment these to test a multi-slave bus configuration.
# FIXME: this isnt thw OWM anymore, so remove or change
#CPPFLAGS += -DOWM_TEST_CONDITION_MULTIPLE_SLAVES

# For the multiple-slave tests to work, you must determine your actual
# slave IDs beforehand and fill them in here.  The test output for the
# OWM_TEST_CONDITION_SINGLE_SLAVE code outputs the ID of the slave it finds
# -- it can be run multiple times with different slaves devices to find all
# their IDs.  Note the IDs must also be assigned correctly to first and
# second positions with respect to the discovery order of the 1-wire bus
# search algorithm.  It might be easiest to try both orders :) The individual
# bytes of these IDs should be given in the order they occur in the ID.
# The test program takes care of swapping them to account for endianness.
# FIXME: this isnt thw OWM anymore, so remove or change
#CPPFLAGS += -DOWM_FIRST_SLAVE_ID=0x28542a8e060000b4
# FIXME: this isnt thw OWM anymore, so remove or change
#CPPFLAGS += -DOWM_SECOND_SLAVE_ID=0x289ae53005000015

# If this is uncommented, we build a function which fetches a text description
# of a ows_result_t.  This inevitably burns some program memory for the
# strings (whether the function is called or not), so we provide this option
# so it can be disabled easily.
CPPFLAGS += -DOWS_BUILD_RESULT_DESCRIPTION_FUNCTION

# Ensure that exactly one test condition is defined.
# FIXME: this isnt thw OWM anymore, so remove or change
ifneq ($(words $(filter -DOWM_TEST_CONDITION_%,$(CPPFLAGS))),1)
  $(error Exactly one OWM_TEST_CONDITION_* must be defined)
endif
