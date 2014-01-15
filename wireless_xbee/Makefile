
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
ARDUINO_BOOTLOADER = optiboot_atmega328.hex

# Uncomment this to use an AVRISPmkII for upload instead of the serial port,
# in order to avoid having to flip the switch on the (FIXME: shield ref)
# and push the reset button every time.  See the notes above the CHKP_PD4()
# macro definition in wireless_sbee_test.c for details.
#
# WARNING: using this will make normal serial Arduino programming impossible
# until the Arduino bootloader is reloaded, see the discussion in hte Upload
# Method of generik.mk for details.
#UPLOAD_METHOD = AVRISPmkII

include generic.mk

# Start a screen session connected to the FIXME: part number here XBee
# USB dongle.
.PHONY: dongle_screen
dongle_screen:
	# FIXME: fix the hardwiredness here probably
	screen -S dongle_screen /dev/ttyUSB0
