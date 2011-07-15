
# We can't use the arduino_bl programmer method here; see below error message
# for details.
UPLOAD_METHOD = AVRISPmkII

define UPLOAD_METHOD_ERROR_TEXT =

  This module requires the arduino to be programmed using an AVRISPmkII or
  similar in-system programming device.  The arduino bootloader methos doesn
  not work, presumably because the bootloaded code does not end up running
  the MCUSR reset code quickly enough which results in continual resetting
  after the first reset.  But note that once this method is used, the
  bootloader must be replaced before the normal arduino programming method
  may be used; see http://brittonkerin.com/cduino/manual.html#upload_method
  for details
endef

ifneq ($(UPLOAD_METHOD),AVRISPmkII)
  $(error $(UPLOAD_METHOD_ERROR_TEXT))
endif

include generic.mk
