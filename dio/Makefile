include generic.mk

# Uncomment this to disable testing of the PD0 and PD1 pin interface macros.
# They won't work if the standard Arduino bootloader is being used, since
# Arduino puts them in serial mode.
CPPFLAGS += -DNO_TEST_SERIAL_PINS

# See above comments near 'CPPFLAGS +='.
ifeq (,$(findstring -DNO_TEST_SERIAL_PINS, $(CPPFLAGS)))
  ifeq ($(UPLOAD_METHOD),arduino_bl)
    $(error Cannot test pins PD0 and PD1 with these settings)
  endif
endif

# The user must uncomment this line to acknowledge that the PB6, PB7, and PC6
# macros have never been tested by the test program (because they are used by
# the Arduino for the external oscillator crystal connections and reset pin).
CPPFLAGS += -DUNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED
