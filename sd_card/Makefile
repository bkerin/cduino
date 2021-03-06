
# This is only required for debugging.
include run_screen.mk

include generic.mk

# Specify the pin which is to be used for SD card SPI slave selection.
# Note that we don't use the normal SS pin for slave selection here (though
# it will still be initialized as an output as required for correct ATMega
# hardware SPI master mode operation), because we're targeting the Arduino
# SD Card/Ethernet shield, which uses a different pin for control of the
# SD card controller SPI slave.
CPPFLAGS += -DSD_CARD_SPI_SLAVE_SELECT_PIN=DIO_PIN_DIGITAL_4

# Uncommenting this will cause this module to use the timer0 hardware to
# detect timeout failures when communicating with the SD card.  Otherwise,
# timeouts are detected by counting iterations transferring bytes on the
# SPI bus.  Using the timer0 hardware might be slightly safer, since it will
# still work in the case where SPI communication slows to a crawl (due to
# contention for the SPI bus perhaps).  However, if SPI communication locks up
# entirely, the timer still won't get checked and a lockup will result.  So in
# general I think it's not worth using the timer0 hardware for this purpose.
#CPPFLAGS += -DSD_CARD_USE_TIMER0_FOR_TIMEOUTS

# If this is uncommented, we build a function which fetches a text description
# of an SD card error.  However, this inevitably burns about 1k of program
# memory for the strings (whether the function is called or not), so we
# provide this option so it can be disabled easily.
CPPFLAGS += -DSD_CARD_BUILD_ERROR_DESCRIPTION_FUNCTION
