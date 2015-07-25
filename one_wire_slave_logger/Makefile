# vim:foldmethod=marker

# This module is designed to handle log messages, and printing them out to
# a terminal connected to the slave arduino is a good thing to do with them,
# so we likely want this.
include run_screen.mk

# It's convenient to have both a master and a slave connected at the same
# time, so we can test our one_wire_master_logger and one_wire_slave_logger
# implementations together.  To support that, we have to use a different
# port for the second Arduino, and not depend on autodetection.  Our build
# system then requires us to specify all of these options.  Note that there
# are some similar variables in the Makefile for the one_wire_master_logger
# module which must be uncommented and set correctly as well.  Note also that
# the port device file assigned to a given Arduino can depend on the order
# in which it was plugged in, and on other devices present on USB.  In my
# case I plug in the master Arduino first and it ends up as /dev/ttyACM0. I
# plug the slave Arduino in next and it ends up as /dev/ttyACM1.  When in
# doubt, try plugging and unplugging the Arduinos and doing ls /dev to see
# which files appear/dissapear.
ARDUINO_PORT = /dev/ttyACM1
ARDUINO_BAUD = 115200
ARDUINO_BOOTLOADER = optiboot_atmega328.hex

include generic.mk

# The one_wire_slave.h interface requires us to explicitly acknowledge that
# register locking is being used and weird CFLAGS are required.
#CPPFLAGS += -DOWS_REGISTER_LOCKING_ACKNOWLEDGED
CFLAGS += -ffixed-r2 -ffixed-r3 -ffixed-r4 -ffixed-r5

# The one_wire_slave module requires a sufficiently fast timer1.
CPPFLAGS += -DTIMER1_STOPWATCH_PRESCALER_DIVIDER=8

# This specifies which pin is being used for 1-wire communication.
# Note that this sets a variable from the one_wire_slave.h namespace.
# This is appropriate, because the one_wire_slave.h interface doesn't support
# slaves being on multiple 1-wire networks simultaneously, so the pin used
# for debuggging via this interface must be the same as the one used for
# other 1-wire communcation (if any).
CPPFLAGS += -DOWS_PIN=DIO_PIN_DIGITAL_2
