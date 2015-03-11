include generic.mk

# By default this module uses pin DIO_PIN_DIGITAL_13 (aka DIO_PIN_PB5),
# since the on-board LED on the Arduino is connected to that pin.  But this
# is problematic when other things (e.g. SPI) need to use that pin.
# Setting DBL_PIN to a different pin name from dio.h will cause it to use
# a different pin instead (e.g. DIO_PIN_PD2).  This should be done before
# debug_led.h is included anywhere (so here is the perfect place :).
#CPPFLAGS += -DDBL_PIN=DIO_PIN_PD2

# If DBL_FEED_WDT is defined, many routines in this module will deliberately
# foil the watchdog timer (WDT) by explicitly feeding the watchdog.  This
# makes them more useful for debugging, but of course not so good for the WDT.
#CPPFLAGS += -DDBL_FEED_WDT

# If this is defined, namespace-polluting short-form aliases for some of
# the macros in this interface are defined.
#CPPFLAGS += -DDBL_POLLUTE_NAMESPACE

# If this is defined, the test program will enable a short watchdog timer
# timeout, and the blinky output will therefore not work correctly unless
# DBL_FEED_WDT is defined.
#CPPFLAGS += -DENABLE_WDT

# Ease case here represents a different feature of this module's interface.
# Its not practical to test them all in the same program in this case, since
# most of them cause the program to loop forever.  These don't represent
# any different in the hardware, only in the interface module to test.
CPPFLAGS += -DTEST_CASE_MULTIBLINK
#CPPFLAGS += -DTEST_CASE_CHKP
#CPPFLAGS += -DTEST_CASE_TRAP
#CPPFLAGS += -DTEST_CASE_ASSERT
#CPPFLAGS += -DTEST_CASE_DISPLAY_UINT32
#CPPFLAGS += -DTEST_CASE_ASSERT_SHOW_POINT

# Ensure that exactly one test case is defined.
ifneq ($(words $(filter -DTEST_CASE_%,$(CPPFLAGS))),1)
  $(error Exactly one TEST_CASE_* must be defined)
endif
