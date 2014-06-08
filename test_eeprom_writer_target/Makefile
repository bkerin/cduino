# To see this module in action, program the chip ('make -R writeflash' from the
# module subcirectory).  Wait a few seconds for the bootloader to get done
# after the AVR reboots.  Then run 'make -R run_screen' from a terminal to run
# the screen program.  Hit return a few times and you should get a prompt.
# You're now talking to your arduino!

include run_screen.mk

AVRLIBC_PRINTF_LDFLAGS ?= "-Wl,-u,vfprintf -lprintf_flt -lm"

include generic.mk

