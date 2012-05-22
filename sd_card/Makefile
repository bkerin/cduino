include run_screen.mk

include generic.mk

# These defines specify which pins are used for SPI communication.
# The right hand sides of these definitions are the initialization/set
# macros from dio.h.  Note that we're really only specifying four pins here,
# we just have to explicitly spell out all the function-like macro names
# (due to the limitations of the C preprocessor).
#
# FIXME: possibly don't use generic DIO to control these for hardware
# SPI anyway...
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
            -DLCD_RS_INIT=DIO_INIT_DIGITAL_8 \
            -DLCD_RS_SET=DIO_SET_DIGITAL_8 \
            -DLCD_RS_SET_HIGH=DIO_SET_DIGITAL_8_HIGH \
            -DLCD_RS_SET_LOW=DIO_SET_DIGITAL_8_LOW \
            \
            -DLCD_ENABLE_INIT=DIO_INIT_DIGITAL_9 \
            -DLCD_ENABLE_SET_HIGH=DIO_SET_DIGITAL_9_HIGH \
            -DLCD_ENABLE_SET_LOW=DIO_SET_DIGITAL_9_LOW \
            \
            -DLCD_DB4_INIT=DIO_INIT_DIGITAL_4 \
            \
            -DLCD_DB5_INIT=DIO_INIT_DIGITAL_5 \
            \
            -DLCD_DB6_INIT=DIO_INIT_DIGITAL_6 \
            \
            -DLCD_DB7_INIT=DIO_INIT_DIGITAL_7
