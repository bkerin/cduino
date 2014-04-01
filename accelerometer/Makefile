# NOTE: this variable definition causes the full floating point version of
# printf (and friends) to be available, which is only required for the test
# driver for this module.  Clients (other modules containing symbolic links to
# the interface and implementation files (currently adc.h and adc.c, or
# stand-alone programs using these files) don't need this definition.
AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm

# This is only really only needed for testing and debugging of this module.
# This goes for the uart and term_io header and source files as well.
include run_screen.mk

include generic.mk
