
# This is only required for debugging.
include run_screen.mk

# The presence of the (FIXME: dongle name here) causes the build system
# to think it might be an Arduino, so it requires us to specify which
# Arduino we want to program.  These are the likely settings for Arduino
# Uno Rev.3 boards at least, though the ARDUINO_PORT could be different.
# Try commenting out these lines and rebuilding whatever target you're
# working on: the build system should print out some hints about what to
# try (unplugging and replugging the Arduino to see which file in /dev
# dissapears/reapears is a good way to start :).
ARDUINO_PORT = /dev/ttyACM0
ARDUINO_BAUD = 115200
ARDUINO_BOOTLOADER=optiboot_atmega328.hex

include generic.mk

# Start a screen session connected to the FIXME: part number here XBee
# USB dongle.
.PHONY: dongle_screen
dongle_screen:
	# FIXME: fix the hardwiredness here probably
	screen -S dongle_screen /dev/ttyUSB0
