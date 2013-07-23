
include run_screen.mk

include generic.mk

# Specify the pin which is to be used for SD card SPI slave selection.
# Note that we don't use the normal SS pin for slave selection here (though
# it will still be initialized as an output as required for correct ATMega
# hardware SPI master mode operation), because we're targeting the Arduino
# SD Card/Ethernet shield, which uses a different pin for control of the
# SD card controller SPI slave.
CPPFLAGS += -DSD_CARD_SPI_SLAVE_SELECT_PIN=DIO_PIN_DIGITAL_4

# Pull in some macros that make debugging with term_io easier.
CPPFLAGS += -DTERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
