
# Support reasonably full-featured printf-style functionality for
# lcd_printf().  Commenting this out will give significantly smaller code
# but no floating point output support, see the comments about this variable
# in generic.mk for details.
AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm

include generic.mk

# These defines specify which pins are used for communication with the LCD.
# The right hand sides of these definitions are the initialization/set
# macros from dio.h.  Note that we're really only specifying six pins here,
# we just have to explicitly spell out all the function-like macro names
# (due to the limitations of the C preprocessor).
CPPFLAGS += -DLCD_RS_INIT=DIO_INIT_DIGITAL_8 \
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

# WARNING: THIS NEXT TARGET PLUS VERSION CONTROL AND ROUNDING IS A BIG FAT
# CONFUSING FALSE FAIL WAITING TO HAPPEN.
#
# This silly little target just increments a number in the test text we
# print, to let us feel sure that what we are seeing on the LCD is actually
# the new version (since the LCD doesn't necessarilly get reset with the
# rest of the arduino).
wfovn:
	test -w lcd_test.c && \
        perl -p -i.bak -e 'm/42.(\d\d)/;' \
                       -e '$$mvn = $$1;' \
                       -e '$$nmvn = $$mvn + 1;' \
                       -e 's/42\.$$mvn/42.$$nmvn/;' \
                       lcd_test.c && \
                       $(MAKE) writeflash
