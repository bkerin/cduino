
# This is only required for debugging.
include run_screen.mk

# The presence of an XBee Explorer or XBee Explorer Dongle (or probably
# any other FTDI FT232-based device) on the USB causes the build system
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
# in order to avoid having to flip the switch on the XBee Shield and push
# the reset button every time.  See the notes above the CHKP_PD4() macro
# definition in wireless_sbee_test.c for details.
#
# WARNING: using this will make normal serial Arduino programming impossible
# until the Arduino bootloader is reloaded, see the discussion in the Upload
# Method of generik.mk for details.
#UPLOAD_METHOD = AVRISPmkII

include generic.mk

# Uncomment this to build the test program for bulk testing with
# usb_xbee_test.  See the comments in wireless_xbee_test.c for details.
#CPPFLAGS += -DAUTOMATIC_TESTING_WITH_USB_XBEE_TEST
