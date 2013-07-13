# This can be uncommented to disable the error trap that warns users that
# we haven't tested all the possible SPI bit orders and modes.
CPPFLAGS = -DUNTESTEDNESS_ACKNOWLEDGED

include generic.mk

# Specify which pins are to be used for SPI communication.  See the comments
# near the first references to these variables in spi.h for details.
CPPFLAGS += -DSPI_SS_INIT=DIO_INIT_DIGITAL_10 \
            -DSPI_SS_SET_LOW=DIO_SET_DIGITAL_LOW \
            -DSPI_SS_SET_HIGH=DIO_SET_DIGITAL_10_HIGH \
            -DSPI_SCK_INIT=DIO_INIT_DIGITAL_13 \
            -DSPI_MOSI_INIT=DIO_INIT_DIGITAL_11
