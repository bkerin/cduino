
# This is only required for debugging this module itself.  If you can talk
# to the master using screen you don't need this module.
include run_screen.mk

AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm

# These setting are only useful when we want to connect two Arduinos
# at once (in order to program/debug one_wire_master_logger.h and
# one_wire_slave_logger.h simultaneously, for example).  In this situation
# we can't depend on build system autodetection, but must instead state
# explicitly which device we want to program.  Our build system then
# requires us to specify all of these options.  Note that there are some
# similar variables in the Makefile for the one_wire_slave_logger module
# which must be uncommented and set correctly as well.  Note also that the
# port device file assigned to a given Arduino can depend on the order in
# which it was plugged in, and on other devices present on USB.  In my case
# I plug in the master Arduino first and it ends up as /dev/ttyACM0. I plug
# the slave Arduino in next and it ends up as /dev/ttyACM1.  When in doubt,
# try plugging and unplugging the Arduinos and doing ls /dev to see which
# files appear/dissapear.  FIXME: perhaps comment out again in release,
# though in this case we're presumably going to be talking to one of our
# one_wire_slave_logger devices, so unlike in one_wire_master module itself
# we may want to leave this in in release, or at least explain that it's
# required.
ARDUINO_PORT = /dev/ttyACM0
ARDUINO_BAUD = 115200
ARDUINO_BOOTLOADER = optiboot_atmega328.hex

include generic.mk

# This specifies which pin is being used for 1-wire communication.  Note that
# this sets a variable from the one_wire_master.h namespace, unfortuantely.
# This is appropriate, because the one_wire_master.h interface doesn't support
# multiple 1-wire networks, so the pin used for debuggging via this interface
# must be the same as the one used for other 1-wire communcation (if any).
CPPFLAGS += -DOWM_PIN=DIO_PIN_DIGITAL_2

# Uncomment and set this to address logging messages to a particular slave
# (as opposed to the default behavior, which assumes the device running the
# one_wire_slave_logger code is the only slave on the bus).  The value here
# is the default used by the one_wire_slave.h and one_wire_slave_logger.h
# interfaces (though to be cute and illustrative, the test program Makefile
# in the one_wire_slave module uses a different value).
#CPPFLAGS += -DDOWM_TARGET_SLAVE=0x4244444422222218
