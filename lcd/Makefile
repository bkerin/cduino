
# Support reasonably full-featured printf-style functionality for
# lcd_printf().  Commenting this out will give significantly smaller code
# but no floating point output support, see the comments about this variable
# in generic.mk for details.
AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm

include generic.mk

# These defines specify which pins are used for communication with the LCD.
# The right hand sides of these definitions are the initialization/set
# macros from dio.h.
CPPFLAGS += -DLCD_RS_INIT=DIO_INIT_DIGITAL_8 \
            -DLCD_RS_SET=DIO_SET_DIGITAL_8 \
            -DLCD_RS_SET_HIGH=DIO_SET_DIGITAL_8_HIGH \
            -DLCD_RS_SET_LOW=DIO_SET_DIGITAL_8_LOW \
             \
            -DLCD_ENABLE_INIT=DIO_INIT_DIGITAL_9 \
            -DLCD_ENABLE_SET_HIGH=DIO_SET_DIGITAL_9_HIGH \
            -DLCD_ENABLE_SET_LOW=DIO_SET_DIGITAL_9_LOW \
             \
            -DLCD_DATA0_INIT=DIO_INIT_DIGITAL_4 \
            -DLCD_DATA0_SET=DIO_SET_DIGITAL_4 \
             \
            -DLCD_DATA1_INIT=DIO_INIT_DIGITAL_5 \
            -DLCD_DATA1_SET=DIO_SET_DIGITAL_5 \
             \
            -DLCD_DATA2_INIT=DIO_INIT_DIGITAL_6 \
            -DLCD_DATA2_SET=DIO_SET_DIGITAL_6 \
             \
            -DLCD_DATA3_INIT=DIO_INIT_DIGITAL_7 \
            -DLCD_DATA3_SET=DIO_SET_DIGITAL_7

CXX := avr-g++

NONCXXFLAGS = -std=gnu99 \
              -Wimplicit-int \
              -Wmissing-prototypes \
              -Wold-style-declaration \
              -Wstrict-prototypes \

CXXFLAGS = $(filter-out $(NONCXXFLAGS), $(CFLAGS))

OBJS += $(patsubst %.cpp,%.o,$(wildcard *.cpp))

$(TRG): $(OBJS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# WARNING: THIS NEXT TARGET PLUS VERSION CONTROL AND ROUNDING IS A BIG FAT
# CONFUSING FALSE FAIL WAITING TO HAPPEN.
#
# This silly little target just increments a number in the test text we
# print, to let us feel sure that what we are seeing on the LCD is actually
# the new version (since the LCD doesn't necessarilly get reset with the
# rest of the arduino).
wfovn:
	perl -p -i.bak -e 'm/42.(\d\d)/;' \
                       -e '$$mvn = $$1;' \
                       -e '$$nmvn = $$mvn + 1;' \
                       -e 's/42\.$$mvn/42.$$nmvn/;' \
                       main.c ; \
                       $(MAKE) writeflash
