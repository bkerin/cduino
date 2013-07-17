# vim: foldmethod=marker

# To test some pins we have to use an in-system programmer (see comment
# elsewhere in this file).
#UPLOAD_METHOD = AVRISPmkII

include generic.mk

# When using the in-system programming (ISP), we can't run all tests on
# pins PB3:5 without disconnecting the programmer, which is inconvenient.
# So we skip the tests on these pins when using ISP.  So we can't test all
# pins on any single run of the test program, oh well.
CPPFLAGS += -DTEST_ISP_PINS

# Uncomment this to enable testing of the PD0 and PD1 pin interface
# macros.  They won't work if the standard Arduino bootloader is being
# used, since Arduino puts them in serial mode.  See also the comment near
# "-DTEST_ISP_PINS".
#CPPFLAGS += -DTEST_SERIAL_PINS

# Ensure that we aren't trying to test pins that won't work with the
# current programmer method (at least without disconnecting the ISP).
ifneq (,$(findstring -DTEST_ISP_PINS, $(CPPFLAGS)))
  ifeq ($(UPLOAD_METHOD),AVRISPmkII)
  $(error Cannot test pins PB3:5 with these settings)
  endif
endif
ifneq (,$(findstring -DTEST_SERIAL_PINS, $(CPPFLAGS)))
  ifeq ($(UPLOAD_METHOD),arduino_bl)
    $(error Cannot test pins PD0 and PD1 with these settings)
  endif
endif

# See comments near UNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED in dio_test.c.
#CPPFLAGS += -DUNDERSTAND_PB6_PB7_PC6_MACROS_UNTESTED

# See comments near UNDERSTAND_PB5_PD0_PD1_WEAK_LOW_470_KOHM_CONDITION_UNTESTED
# in dio_test.c.
#CPPFLAGS += -DUNDERSTAND_PB5_PD0_PD1_WEAK_LOW_470_KOHM_CONDITIONS_UNTESTED

# The hardware of the different versions is slightly different, so we define
# an appropriate preprocessor symbol to make sure our test code expects
# the right thing.  see the notes near the uses of the macros defined in
# this conditional for details.
ARDUINO_MODEL := $(shell ./guess_arduino_attribute.perl --model)
ifeq ($(ARDUINO_MODEL),Uno Rev3)
  CPPFLAGS += -DARDUINO_MODEL_IS_UNO_REV3
else ifeq ($(ARDUINO_MODEL),Duemilanove)
  CPPFLAGS += -DARDUINO_MODEL_IS_DUEMILANOVE
else
  $(error Unknown (or poory detected) Arduino model '$(ARDUINO_MODEL)')
endif

# Each condition here represents a different hardware setup capable of
# exercising different uses of the interface (e.g. input, output, pullups).


CPPFLAGS += -DTEST_CONDITION_ALL_PINS_NC


#CPPFLAGS += -DTEST_CONDITION_PB0_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB1_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB2_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB3_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB4_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB5_HIGH_PB0_SIGNAL_LED_OTHERS_NC

#CPPFLAGS += -DTEST_CONDITION_PC0_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC1_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC2_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC3_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC4_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC5_HIGH_OTHERS_NC

#CPPFLAGS += -DTEST_CONDITION_PD0_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD1_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD2_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD3_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD4_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD5_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD6_HIGH_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD7_HIGH_OTHERS_NC


#CPPFLAGS += -DTEST_CONDITION_PB0_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB1_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB2_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB3_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB4_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB5_LOW_PB0_SIGNAL_LED_OTHERS_NC

#CPPFLAGS += -DTEST_CONDITION_PC0_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC1_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC2_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC3_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC4_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC5_LOW_OTHERS_NC

#CPPFLAGS += -DTEST_CONDITION_PD0_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD1_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD2_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD3_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD4_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD5_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD6_LOW_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD7_LOW_OTHERS_NC


# These *_WEAK_LOW_470_KOHM_* test conditions mean that the pin in question is
# tied to ground via a 470 kohm resistor.  The point of this is to test that
# the pin ends up high when the internal pullup is enabled, but low otherwise.
# The 470 kohm value seems large, but the ATMega328p datasheet claims a
# maximum input leakage current of 1 uA at 5V when an input pin is high,
# which according to V=IR suggests an input impediance of 5 Mohm or more.
# Using a large value guarantees that when the pull-up is enabled, the pin
# voltage will be high enough to be read as high.

#CPPFLAGS += -DTEST_CONDITION_PB0_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB1_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB2_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB3_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB4_WEAK_LOW_470_KOHM_OTHERS_NC
# NOTE: it isn't possible to test PB5_WEAK_LOW with arduino hardware, see
# comments near UNDERSTAND_PB5_PD0_PD1_WEAK_LOW_470_KOHM_CONDITIONS_UNTESTED.

#CPPFLAGS += -DTEST_CONDITION_PC0_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC1_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC2_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC3_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC4_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC5_WEAK_LOW_470_KOHM_OTHERS_NC

# NOTE: it isn't possible to test PD0_WEAK_LOW or
# PD1_WEAK_LOW with arduino hardware, see comments near
# UNDERSTAND_PB5_PD0_PD1_WEAK_LOW_470_KOHM_CONDITIONS_UNTESTED.
#CPPFLAGS += -DTEST_CONDITION_PD2_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD3_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD4_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD5_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD6_WEAK_LOW_470_KOHM_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD7_WEAK_LOW_470_KOHM_OTHERS_NC


#CPPFLAGS += -DTEST_CONDITION_PB0_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB1_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB2_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB3_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB4_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PB5_LED_OTHERS_NC

#CPPFLAGS += -DTEST_CONDITION_PC0_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC1_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC2_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC3_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC4_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PC5_LED_OTHERS_NC

#CPPFLAGS += -DTEST_CONDITION_PD0_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD1_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD2_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD3_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD4_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD5_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD6_LED_OTHERS_NC
#CPPFLAGS += -DTEST_CONDITION_PD7_LED_OTHERS_NC


# Ensure that the macros declaring in-system programming (ISP) pins to be
# ok to test are defined if the TEST_CONDITION_* that will be using those
# pins is defined.
ifeq (,$(findstring -DTEST_ISP_PINS,$(CPPFLAGS)))
  ifneq (,$(findstring -DTEST_CONDITION_PB3_,$(CPPFLAGS)))
    $(error Cannot test pin PB3 when TEST_ISP_PINS is not defined)
  endif
  ifneq (,$(findstring -DTEST_CONDITION_PB4_,$(CPPFLAGS)))
    $(error Cannot test pin PB4 when TEST_ISP_PINS is not defined)
  endif
  ifneq (,$(findstring -DTEST_CONDITION_PB5_,$(CPPFLAGS)))
    $(error Cannot test pin PB5 when TEST_ISP_PINS is not defined)
  endif
endif

# Ensure that the macros declaring serial pins to be ok to test are defined
# if the TEST_CONDITION_* that will be using those pins is defined.
ifeq (,$(findstring -DTEST_SERIAL_PINS,$(CPPFLAGS)))
  ifneq (,$(findstring -DTEST_CONDITION_PD0_,$(CPPFLAGS)))
    $(error Cannot test pin PD0 when TEST_SERIAL_PINS is not defined)
  endif
  ifneq (,$(findstring -DTEST_CONDITION_PD1_,$(CPPFLAGS)))
    $(error Cannot test pin PD1 when TEST_SERIAL_PINS is not defined)
  endif
endif

# Ensure that exactly one test condition is defined.
ifneq ($(words $(filter -DTEST_CONDITION_%,$(CPPFLAGS))),1)
  $(error Exactly one TEST_CONDITION_* must be defined)
endif
