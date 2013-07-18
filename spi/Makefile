# This can be uncommented to disable the error trap that warns users that
# we haven't tested all the possible SPI bit orders and modes.
CPPFLAGS = -DSPI_UNTESTEDNESS_ACKNOWLEDGED

include generic.mk

# Specify the pin which will be used for SPI slave selection.  NOTE: the
# pin used for SPI slave selection does NOT have to be the SS pin, and its
# possible (probably necessary actually) to use multiple output pins for
# slave selection when multiple SPI slaves are in use.  See the comments
# near the top of spi.h for details.
CPPFLAGS += \
  -D'MY_SS_INIT_HIGH()'='SPI_SS_INIT(DIO_OUTPUT, DIO_DONT_CARE, HIGH)' \
  -D'MY_SPI_SLAVE_1_SELECT_INIT()'='MY_SS_INIT_HIGH()' \
  -DMY_SPI_SLAVE_1_SELECT_SET_LOW=SPI_SS_SET_LOW \
  -DMY_SPI_SLAVE_1_SELECT_SET_HIGH=SPI_SS_SET_HIGH
