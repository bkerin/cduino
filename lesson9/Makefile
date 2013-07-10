# First, connect a 10 kohm potentiometer between the arduino 5V supply and
# ground.  Connect the potentiometer tap to pin PC0 (ADC0) Next, program
# the chip ('make -rR writeflash').  Wait a few seconds for the bootloader
# to get done after the AVR reboots.  Then run 'make -rR run_screen' from
# a terminal to run the screen program.  Hit return a few times and you
# should see a list of potentiometer voltage readings start to scroll past.
# Twiddle the potentiometer dial and the voltage reading should follow.

AVRLIBC_PRINTF_LDFLAGS = -Wl,-u,vfprintf -lprintf_flt -lm

include run_screen.mk

include generic.mk
