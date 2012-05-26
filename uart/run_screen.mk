# Make rules to run the screen program with the default 8N1 settings on
# /dev/ttyUSB0, and kick it a  at startup as is often useful to get terminal
# programs started showing something.

# FIXME: it would be nice to somehow deal with the likely case where the user
# wants to run multiple screen sessions connected to different Arduinos,
# but this would require considerable thought and fiddle-faddle to make
# work in a general and comprehensible way.

# The screen session lets you assign names to sessions, which is useful to
# us since we need to kill them later before programming with the bootloader.
SCREEN_SESSION_NAME = cduino_run_screen_mk

.PHONY: run_screen
run_screen: have_screen
	@echo
	@echo About to run screen.
	@echo
	@echo This command does two things:
	@echo
	@echo  '  '\* Starts a screen session with session name
	@echo  '   '  $(SCREEN_SESSION_NAME).  We use a session name so we can
	@echo  '   '  automatically kill the screen session from the writeflash
	@echo  '   '  target of generic.mk, to free the serial line for
	@echo  '   '  bootloader programming.
	@echo
	@echo  '  '\* Feeds the session a return character which often seems to
	@echo  '   '  help the terminal get going :\)
	@echo
	@echo We do not need other options because the UART configuration
	@echo selected for the microcontroll matches the screen defaults in
	@echo our case.
	@echo
	@echo This command looks a bit fragile, if it breaks somehow it is
	@echo possible to use just \'screen $(ACTUAL_ARDUINO_PORT)\'.
	@echo
	@echo Use CNTRL-a \\ \(no control on backslash\) to exit screen.
	@echo
	@echo "Press Enter to continue"
	@read
	@# NOTE: this next command needs a literal Cntrl-M, not a two character
	@# string "^M".  In bash and vi this can be generated using C-v C-m.
	(sleep 0.5 && screen -S $(SCREEN_SESSION_NAME) -X stuff  &) ; \
        screen -S $(SCREEN_SESSION_NAME) $(ACTUAL_ARDUINO_PORT)

.PHONY: have_screen
have_screen:
	[ -n $$(which screen) ] || \
          (echo "the screen program doesn't appear to installed" 2>&1 && false)
