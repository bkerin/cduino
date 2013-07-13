# This can be uncommented to disable the error trap that warns users that
# we haven't tested all the possible SPI bit orders and modes.
CPPFLAGS = -DUNTESTEDNESS_ACKNOWLEDGED

include generic.mk

# Specify which pins are to be used for SPI communication.  NOTE: the pin
# used for SPI slave selection does NOT have to be the SS pin, and its
# possible (probably necessary actually) to use multiple output pins for
# slave selection when multiple SPI slaves are in use.  See the comments
# near the first references to these variables in spi.h for details.
CPPFLAGS += -DSPI_SLAVE_1_SELECT_INIT=SPI_SS_INIT \
            -DSPI_SLAVE_1_SELECT_SET_LOW=SPI_SS_SET_LOW \
            -DSPI_SLAVE_1_SELECT_SET_HIGH=SPI_SS_SET_HIGH \
            -DSPI_SCK_INIT=DIO_INIT_DIGITAL_13 \
            -DSPI_MOSI_INIT=DIO_INIT_DIGITAL_11
