
# We can't use the arduino_bl programmer method here; see below error message
# for details.  Because using the AVRISPmkII (in-system programming)
# programming method overwrites the bootloaded and requires a special command
# to restore it, this line must be uncommented by the user.
#UPLOAD_METHOD = AVRISPmkII

# FIXME: if/when an updated "official" arduino bootloader image gets released
# that fixes this we should supply it instead of the current 0022 version one
define UPLOAD_METHOD_ERROR_TEXT =

  This module requires the arduino to be programmed using an AVRISPmkII or
  similar in-system programming device.  The reason is that the stock
  bootloader that comes with many arduinos does not correctly clear the
  watchdog reset flag at the start of the boot, resulting in a continual reset
  loop.  It may be that modern (Uno and later) arduinos have fixed this
  problem, but I'm not sure.  The commented-out UPLOAD_METHOD assignment in the
  Makefile for this module can be used to specify that a connected AVRISPmkII
  programmer should be used.  But note that once this method is used, the
  bootloader must be replaced before the normal arduino programming method may
  be used; see http://brittonkerin.com/cduino/manual.html#upload_method for
  details.

  Note that if the AVR does end up in a situation where the WDT reset bit is
  set and the chip is resetting continually, resetting the chip with the reset
  button will not be enough to make the chip programmable.  It will be
  necessary to power cycle the AVR board to make the chip programmable (because
  power down resets are detected by the AVR and result in the WDT reset flag
  being cleared.  Of course, depending on the program it may take good timing
  to do this before the reset loop starts again...
endef

ifneq ($(UPLOAD_METHOD),AVRISPmkII)
  $(error $(UPLOAD_METHOD_ERROR_TEXT))
endif

include generic.mk
