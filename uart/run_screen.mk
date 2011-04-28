# Make rules to run the screen program with the default 8N1 settings on
# /dev/ttyUSB0, and kick it a  at startup as is often useful to get terminal
# programs started showing something.

.PHONY: run_screen
run_screen: have_screen
	@echo
	@echo About to run screen.
	@echo
	@echo The fragile command used in this Makefile does nothing besides
	@echo feed a return to screen so a prompt shows up without the user
	@echo having to push enter.  It can be replaced with a simple
	@echo \'screen /dev/ttyUSB0\'.
	@echo
	@echo This screen invocation works without options because the UART
	@echo configuration selected for the microcontroller matches the screen
	@echo defaults in our case.
	@echo
	@echo "Use C-a \ (no control on backslash) to exit screen";
	@echo
	@echo "Press Enter to continue"
	@read
	@# NOTE: this next command needs a literal Cntrl-M, not a two character
	@# string "^M".  In bash and vi this can be generated using C-v C-m.
	(sleep 0.5 && screen -X stuff  &) ; screen -S arduino /dev/ttyUSB0

.PHONY: have_screen
have_screen:
	[ -n $$(which screen) ] || \
          (echo "the screen program doesn't appear to installed" 2>&1 && false)
