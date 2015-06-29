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

# Because some variables are being locked into particular registers in
# one_wire_slave.c, we have to ensure that the rest of program (i.e. code in
# other source files) knows about this.  These options provide the required
# voodoo (see the GCC manual "Defining Global Register Variables" section
# for details).
CFLAGS += -ffixed-r2 -ffixed-r3 -ffixed-r4 -ffixed-r5

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

# The one_wire_slave.h interface requires this to be defined at compile time.
CPPFLAGS += -DOWS_PIN=DIO_PIN_DIGITAL_2

# This module requires us to use a prescaler setting st we get at least
# 1 us resolution on the timer.  This setting works for the full range of
# CPU speeds tested (10 - 16 MHz).
CPPFLAGS += -DTIMER1_STOPWATCH_PRESCALER_DIVIDER=8

# This is the Maxim family code for the DS18B20.  Since our test/demo
# program emulates a limited DS18B20, we use this family code :) Actually,
# we mainly use it to show how to set your own family code.
CPPFLAGS += -DOWS_FAMILY_CODE=0x28

# If this is uncommented, we build a function which fetches a text description
# of a ows_result_t.  This inevitably burns some program memory for the
# strings (whether the function is called or not), so we provide this option
# so it can be disabled easily.
CPPFLAGS += -DOWS_BUILD_RESULT_DESCRIPTION_FUNCTION

# Ensure that exactly one test condition is defined.
# FIXME: remove this if we don't actually end up using multiple test conditions
# for this slave
ifneq ($(words $(filter -DOWS_TEST_CONDITION_%,$(CPPFLAGS))),1)
  $(error Exactly one OWS_TEST_CONDITION_* must be defined)
endif
