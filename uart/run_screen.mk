# Make rules to run the screen program with the default 8N1 settings on
# /dev/ttyUSB0, and kick it a  at startup as is often useful to get terminal
# programs started showing something.

# FIXXME: it would be nice to somehow deal with the likely case where the user
# wants to run multiple screen sessions connected to different Arduinos,
# but this would require considerable thought and fiddle-faddle to make
# work in a general and comprehensible way.

# The screen session lets you assign names to sessions, which is useful to
# us since we need to kill them later before programming with the bootloader.
SCREEN_SESSION_NAME = cduino_run_screen_mk

# FIXME: might be nice to do something clever if the Arduino isn't hooked
# up over USB.
.PHONY: run_screen
run_screen: have_screen not_no_usb_arduino_connection
	@echo
	@echo About to run screen.  This command does two things:
	@echo
	@echo  '  '\* Starts a screen session with session name
	@echo  '   '  $(SCREEN_SESSION_NAME).  The writeflash target of
	@echo  '   '  generic.mk will automatically kill this screen session,
	@echo  '   '  so that programming will work \(easing the
	@echo  '   '  edit-compile-debug cycle\).
	@echo
	@echo  '  '\* Feeds the session a return character which often seems to
	@echo  '   '  help the terminal get going :\)
	@echo
	@echo We do not need other options because the UART configuration
	@echo selected for the microcontroll matches the screen defaults in
	@echo our case.
	@echo
	@echo You can just do \'screen $(ACTUAL_ARDUINO_PORT)\' instead of
	@echo using this target if it doesn\'t work for you.
	@echo
	@echo Use CNTRL-a \\ \(no control on backslash\) to exit screen.
	@echo
	@echo Press Enter to continue
	@read
	@# NOTE: this next command needs a literal Cntrl-M, not a two character
	@# string "^M".  In bash and vi this can be generated using C-v C-m.
	(sleep 0.5 && screen -S $(SCREEN_SESSION_NAME) -X stuff  &) ; \
        screen -S $(SCREEN_SESSION_NAME) $(ACTUAL_ARDUINO_PORT)

.PHONY: have_screen
have_screen:
	[ -n $$(which screen) ] || \
          (echo "the screen program doesn't appear to installed" 2>&1 && false)

# If the NO_USB_ARDUINO_CONNECTION is non-empty, we won't have the port
# definition we need for the run_screen target to work.
.PHONY: not_no_usb_arduino_connection
not_no_usb_arduino_connection:
	[ -z "$(NO_USB_ARDUINO_CONNECTION)" ] || \
          (echo "NO_USB_ARDUINO_CONNECTION is non-empty" 2>&1 && false)
