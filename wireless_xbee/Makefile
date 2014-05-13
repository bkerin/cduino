
# Unlike most modules, debugging using the Arduino serial port, a USB cable,
# and the screen program is not supported.  This is because the XBee needs
# to use the serial port to talk to the arduino, and because the XBee itself
# provides a good way to get data out of the Arduino: see the usb_xbee program
# in this directory (or its documentation: try 'pod2text usb_xbee | more').
#include run_screen.mk

# The presence of an XBee Explorer or XBee Explorer Dongle (or probably
# any other FTDI FT232-based device) on the USB causes the build system
# to think it might be an Arduino, so it requires us to specify which
# Arduino we want to program.  These are the likely settings for Arduino
# Uno Rev.3 boards at least, though the ARDUINO_PORT could be different.
# Try commenting out these lines and rebuilding whatever target you're
# working on: the build system should print out some hints about what to
# try (unplugging and replugging the Arduino to see which file in /dev
# dissapears/reapears is a good way to start :).  Annoyingly, this problem
# breaks the rebuilds that the toplevel Makefile release targets do, so
# you have to make sure there is no Explorer plugged in when you do that.
# FIXME: we should put some sort of check in this makefile so we get a good
# diagnostic when this happens.
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

# Clients can optionally declare which Arduino pins are connected to the
# XBee SLEEP_RQ and RESET lines to enable some macros which use them.
# See wireless_xbee.h for details.  The actual setting here are random
# examples (unfortunately there is no DIO_PIN_PD42 :).
#CPPFLAGS += -DWX_RESET_CONTROL_PIN=DIO_PIN_PD6
#CPPFLAGS += -DWX_SLEEP_RQ_PIN=DIO_PIN_PB1

# Uncomment this to build the test program for bulk testing with
# usb_xbee_test.  See the comments in wireless_xbee_test.c for details.
#CPPFLAGS += -DAUTOMATIC_TESTING_WITH_USB_XBEE_TEST

# This is really just a note-to-self about one of the most useful usb_xbee
# invocations: it just listenes for incoming frame lines and displays them.
# Assuming the hard-wired device file used here is actually right (try
# 'pod2text uxb_xbee | more' for details).
start_frame_listener:
	./usb_xbee --device=/dev/ttyUSB0 --framed-lines-mode
