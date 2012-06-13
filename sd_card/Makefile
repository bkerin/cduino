include run_screen.mk

include generic.mk

# These defines specify which pins are used for SPI communication with the
# SD card and selection of the SD card.  The right hand sides of these
# definitions are the initialization/set macros from dio.h.  Note that
# we're really only specifying five pins here, we just have to explicitly
# spell out all the function-like macro names (due to the limitations of
# the C preprocessor).
CPPFLAGS += -DSPI_SS_INIT=DIO_INIT_DIGITAL_10 \
            -DSPI_SS_SET=DIO_SET_DIGITAL_10 \
            -DSPI_SS_SET_HIGH=DIO_SET_DIGITAL_10_HIGH \
            -DSPI_SS_SET_LOW=DIO_SET_DIGITAL_10_LOW \
            \
            -DSPI_MOSI_INIT=DIO_INIT_DIGITAL_11 \
            -DSPI_MOSI_SET=DIO_SET_DIGITAL_11 \
            -DSPI_MOSI_SET_HIGH=DIO_SET_DIGITAL_11_HIGH \
            -DSPI_MOSI_SET_LOW=DIO_SET_DIGITAL_11_LOW \
            \
            -DSPI_MISO_INIT=DIO_INIT_DIGITAL_12 \
            -DSPI_MISO_SET=DIO_SET_DIGITAL_12 \
            -DSPI_MISO_SET_HIGH=DIO_SET_DIGITAL_12_HIGH \
            -DSPI_MISO_SET_LOW=DIO_SET_DIGITAL_12_LOW \
            \
            -DSPI_SCK_INIT=DIO_INIT_DIGITAL_13 \
            -DSPI_SCK_SET=DIO_SET_DIGITAL_13 \
            -DSPI_SCK_SET_HIGH=DIO_SET_DIGITAL_13_HIGH \
            -DSPI_SCK_SET_LOW=DIO_SET_DIGITAL_13_LOW \
            \
            \
            -DCHIP_SELECT_INIT=DIO_INIT_DIGITAL_4 \
            -DCHIP_SELECT_SET=DIO_SET_DIGITAL_4 \
            -DCHIP_SELECT_SET_HIGH=DIO_SET_DIGITAL_13_HIGH \
            -DCHIP_SELECT_SET_LOW=DIO_SET_DIGITAL_13_LOW

# Pull in some macros that make debugging with term_io easier.
CPPFLAGS += -DTERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
