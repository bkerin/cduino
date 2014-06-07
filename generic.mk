# Make code  to all the different modules.  Some things defined here are
# easy to override in modules, those section titles contain "(Overridable)".

# vim: foldmethod=marker

##### Make Settings and Sanity Measures {{{1

# This is sensible stuff for use but could confuse an experienced Make
# programmer so its out front here.

# Delete files produced by rules the commands of which return non-zero.
.DELETE_ON_ERROR:

# Don't let make parallelize anything.
.NOTPARALLEL:

# Disable old-fashioned suffix rules.
.SUFFIXES:

# Ensure that we can use bashisms.
SHELL = /bin/bash

# This Makefile requires that the -R/--no-builtin-variables and
# -r/--no-builtin-variables options be used.  Implicit rules and default
# variables cause much more trouble and unreadability than they are worth.
ifeq ($(findstring r,$(MAKEFLAGS)),)
  $(error This makefile requires use of the -r/--no-builtin-rules make option)
endif
ifeq ($(findstring R,$(MAKEFLAGS)),)
   $(error This makefile requires use of the -R/--no-builtin-variables make \
           option)
endif

# Avoid default goal confusion by essentially disabling default goals.
PRINT_DEFAULT_GOAL_TRAP_ERROR_MESSAGE := \
  echo ; \
  echo This build system doesn\'t support default goals, because they tend ; \
  echo to cause confusion.  Please explicitly specify a target. ; \
  echo Useful targets:; \
  echo ; \
  echo '  *' some_file.o  --  Compile some_file.c ; \
  echo '  *' writeflash   --  Compile, link, and upload current module test ; \
  echo

.DEFAULT_GOAL = default_goal_trap
.PHONY: default_goal_trap
default_goal_trap:
	@($(PRINT_DEFAULT_GOAL_TRAP_ERROR_MESSAGE) && false) 1>&2

# This function works almost exactly like the builtin shell command, except it
# stops everything with an error if the shell command given as its argument
# returns non-zero when executed.  The other difference is that the output
# is passed through the strip make function (the shell function strips
# only the last trailing newline).  In practice this doesn't matter much
# since the output is usually collapsed by the surroundeing make context
# to the same result produced by strip.  WARNING: don't try to nest calls
# to this function.
SHELL_CHECKED = \
  $(strip \
    $(if $(shell (($1) 1>/tmp/SC_so) || echo 'non-empty'), \
      $(error shell command '$1' failed.  Its stderr should be above \
              somewhere.  Its stdout is available for review in '/tmp/SC_so'), \
      $(shell cat /tmp/SC_so)))


##### Specs for Default Target Part (Overridable) {{{1

# Name for part being targetted, as known by the compiler and the uploader.
COMPILER_MCU ?= atmega328p
PROGRAMMER_MCU ?= m328p

# AVR libc uses this.  Some people might prefer to put it in a header though.
CPU_FREQ_DEFINE ?= -DF_CPU=16000000

# This is used by the replace_bootloader target to determine which
# bootloader .hex file to use.  Different Arduinos need different values:
# my Duemilanove uses ATmegaBOOT_168_atmega328.hex, which my Uno rev.3
# ships with optiboot_atmega328.hex for example.  The default value of
# autoguess tried to make the determination automagically, but this can fail
# (in which case the guessing script will output some diagnostics).
ARDUINO_BOOTLOADER ?= autoguess
#ARDUINO_BOOTLOADER ?= optiboot_atmega328.hex
#ARDUINO_BOOTLOADER ?= ATmegaBOOT_168_atmega328.hex

# Target patters for which we presumably don't need a connected Arduino.
# Clients can augment this variable, but they have to do so before this
# file is processed (i.e. before the include statement that includes it, or
# with a variable assignment command line argument to the make invocation).
# See comments near where the variable is referenced.
VALID_ARDUINOLESS_TARGET_PATTERNS += %.c %.o %.ee.hex %.hex %.out %.out.map


##### Program Name, Constituent Object Files (Overridable) {{{1

# This is the paragraph that determines which files are being built into what.
PROGNAME ?= program_to_upload
OBJS ?= $(patsubst %.c,%.o,$(wildcard *.c)) \
        $(patsubst %.cpp,%.o,$(wildcard *.cpp))
HEADERS ?= $(wildcard *.h)


##### Upload Method (Overridable) {{{1

# Setting this to a non-empty value declares that there is no Arduino
# connected via USB (or if there is it should be ignored), so the normal
# probes for an Arduino on USB are disabled.  This is useful if you're
# trying to program a bare chip using an AVRISPmkII.
NO_USB_ARDUINO_CONNECTION ?=

# This must be one of:
#
#    arduino_bl          Arduino bootloader over USB
#    AVRISPmkII          AVR ISP mkII programmer, overwriting any bootloader
#
# WARNING: the AVRISPmkII options will make your Arduino unprogrammable using
# the normal bootloader method (though you can recover: see the
# replace_bootloader target).
#
# Note that unless NO_USB_ARDUINO_CONNECTION is defined to a non-empty value,
# this build system still requires an Arduino to be connected over USB,
# it just doesn't use it for programming.
UPLOAD_METHOD ?= arduino_bl

# If this is set to true, then if the perl modules required to pulse the DTR
# line of the serial port aren't found or the DTR pulse code fails for some
# reason, it is not considered a fatal error (but the user will then have to
# push the reset button on the arduino immediately after the avrdude
# programming command goes off to program the device).
DTR_PULSE_NOT_REQUIRED ?= false

# Uploader program.
AVRDUDE ?= avrdude

# The programmer type being used as understood by avrdude.
AVRDUDE_ARDUINO_PROGRAMMERID ?= arduino

# Location and characteristics of the Arduino on the USB.  Note that this
# build system usually tries to find an Arduino even if the AVRISPmkII
# method is being used.  If you don't want any of these checks to happen
# see the NO_USB_ARDUINO_CONNECTION variable above.  The values that should
# be used here differ for different recent arduinos: for me at least, the
# Duemilanove needs /dev/ttyUSB0 and 57600 baud, the Uno /dev/ttyUSB0 and
# 115200 baud.  For other setups or setups with multiple Arduinos hooked
# up, different device names might be required.  The special value of
# 'autoguess' can be used to indicate that the build system should try to
# guess which values to use based on the device it finds connected (and
# output diagnostic messages if it can't guess).
ARDUINO_PORT ?= autoguess
#ARDUINO_PORT ?= /dev/ttyACM0
#ARDUINO_PORT ?= /dev/ttyUSB0
ARDUINO_BAUD ?= autoguess
#ARDUINO_BAUD ?= 115200
#ARDUINO_BAUD ?= 57600


##### Compilers, Assemblers, etc. (Overridable) {{{1

# Compilers.  Note that some of these depend for overridability on the -R
# make option that this Makefile requires be used.
CC ?= avr-gcc
CXX ?= avr-g++
OBJCOPY ?= avr-objcopy
OBJDUMP ?= avr-objdump
SIZE ?= avr-size

# These programs could be useful, but I don't use them at the moment.
AVARICE ?=
AVRGDB ?=


##### Fuse Settings (Overridable) {{{1

# Fuse settings to be programmed, in the form of a list of setting for
# individual bits as they are named in for example the Mega328P datasheet.
# For example, setting this to 'BODLEVEL2=1 BODLEVEL1=0 BODLEVEL0=1'
# will enable brown out detection for a typical value of 2.7 V supply
# as described in the datasheet.  Note that lock and fuse bits are always
# written a byte at a time: if any bits of a byte are specified, all should be
# (or else they will take their default values).  Note also that in order
# for anything to be written to a fuse byte, at least one bit of that
# byte must be specified.  Finally, note that the arduino uses a number
# of non-default lock and fuse settings: changing them may break things.
# On the other hand, if you want to use in-system programming to program
# a minimal system at a different clock rate, you'll need to learn about
# fuse settings.  See the uses of this variable for more details.
LOCK_AND_FUSE_SETTINGS ?=


##### printf() Feature Support (Overridable) {{{1

# AVR libc provides minimal, standard, and (mostly) full-featured printf
# implementations, allowing you to trade features for small code size.  See the
# discussion in
# http://www.nongnu.org/avr-libc/user-manual/group__avr__stdio.html for
# details.  The default is to use the AVR libc default printf implementation,
# which supports most normal printf features except floating point support.
# Other possible values which individual modules may use are
# "-Wl,-u,vfprintf -lprintf_min" and "-Wl,-u,vfprintf -lprintf_flt -lm".
AVRLIBC_PRINTF_LDFLAGS ?=


##### Debugging Macro CPP Flag (Overridable) {{{1

# Its often convenient to compile code slightly differently when debugging
# will be done, this variable gets put in CPPFLAGS so you can set it to
# something like '-DDEBUG' then say '#ifdef DEBUG' in the code to do logging
# and such.
CPP_DEBUG_DEFINE_FLAGS ?=


##### Computed File Names and Settings {{{1

# Magical files that one doesn't see in non-microcontroller GCC development.
TRG = $(PROGNAME).out
DUMPTRG = $(PROGNAME).s
HEXROMTRG = $(PROGNAME).hex
HEXTRG = $(HEXROMTRG) $(PROGNAME).ee.hex
LSTFILES := $(patsubst %.o,%.c,$(OBJS))
GENASMFILES := $(patsubst %.o,%.s,$(OBJS))

# Automatic Determination of Arduino Parameters (Augmentable) {{{2

# In order to support building and linking of files when an Arduino is not
# connected, we avoid doing any of the autodetection work if we our goals
# clearly do not require communication with the Arduino.  Note that this
# is conservative in the sense that if the user tries to build something
# that isn't explicitly included in our list of things that don't really
# need a connection, the autodetection code will go off and they will get
# the detection failure messages.
ARDUINOLESS_TARGET_WARNING_TEXT :=                                             \
  If you should not need to be connected to an Arduino for the taget you are   \
  are building, consider adding a pattern to the                               \
  VALID_ARDUINOLESS_TARGET_PATTERNS Make variable.                             \
                                                                               \
  If you are using an AVRISPmkII (UPLOAD_METHOD = AVRISPmkII), note that the   \
  autodetection code normally still requires the Arduino to be connected to    \
  the computer by USB.  If you do not have a USB connection to the Arduino,    \
  you must define the NO_USB_ARDUINO_CONNECTION make variable to a non-empty   \
  value.  Note that the Arduino hardware autodetection should work even if     \
  there is no Arduino bootloader on the chip.
ifneq ($(filter-out $(VALID_ARDUINOLESS_TARGET_PATTERNS),$(MAKECMDGOALS)),)

  ifeq ($(NO_USB_ARDUINO_CONNECTION), )

    ifeq ($(ARDUINO_PORT),autoguess)
      ACTUAL_ARDUINO_PORT := \
        $(call SHELL_CHECKED,./guess_arduino_attribute.perl --device)
      ifeq ($(ACTUAL_ARDUINO_PORT),)
        $(warning $(ARDUINOLESS_TARGET_WARNING_TEXT))
        $(info )
        $(error could not guess ARDUINO_PORT, see messages above)
      endif
    else
      ACTUAL_ARDUINO_PORT := $(ARDUINO_PORT)
    endif

    ifeq ($(ARDUINO_BAUD),autoguess)
      ACTUAL_ARDUINO_BAUD := \
        $(call SHELL_CHECKED,./guess_arduino_attribute.perl --baud)
      ifeq ($(ACTUAL_ARDUINO_BAUD),)
        $(warning $(ARDUINOLESS_TARGET_WARNING_TEXT))
        $(info )
        $(error could not guess ARDUINO_BAUD, see messages above)
      endif
    else
      ACTUAL_ARDUINO_BAUD := $(ARDUINO_BAUD)
    endif

    ifeq ($(ARDUINO_BOOTLOADER),autoguess)
      ACTUAL_ARDUINO_BOOTLOADER := \
        $(call SHELL_CHECKED,./guess_arduino_attribute.perl --bootloader)
      ifeq ($(ACTUAL_ARDUINO_BOOTLOADER),)
        $(warning $(ARDUINOLESS_TARGET_WARNING_TEXT))
        $(info )
        $(error could not guess ARDUINO_BOOTLOADER, see messages above)
      endif
    else
      ACTUAL_ARDUINO_BOOTLOADER := $(ARDUINO_BOOTLOADER)
    endif

  else

    ifeq ($(UPLOAD_METHOD),arduino_bl)
      $(error The NO_USB_ARDUINO_CONNECTION Make variable is non-empty, but \
              UPLOAD_METHOD is arduino_bl and at least one of the current   \
              make goals does not match any pattern in                      \
              VALID_ARDUINOLESS_TARGET_PATTERNS )
    endif

    ifneq ($(filter replace_bootloader,$(MAKECMDGOALS)),)
      $(error The NO_USB_ARDUINO_CONNECTION Make variable is non-empty, but   \
              replace_bootloader  has been requested as a make goal.  The     \
              replace_bootloader target needs to be able to find an Arduino   \
              on USB to determine which bootloader image is required for your \
              hardware. )
    endif

  endif

endif

# }}}2


##### Build Settings and Flags (Augmentable) {{{1

HEXFORMAT := ihex

OPTLEVEL := s

# Clients can add their own CPPFLAGS.  Note that order sometimes matters
# for compiler flags, so it could matter whether they do so before or after
# the include statement that includes this file.
CPPFLAGS += $(CPP_DEBUG_DEFINE_FLAGS) $(CPU_FREQ_DEFINE) -I.

# See comments near CPPFLAGS, above.
CFLAGS += -std=gnu99 -fshort-enums -mmcu=$(COMPILER_MCU) -O$(OPTLEVEL) \
          -Werror -Wall -Wextra -Winline -Wmissing-prototypes \
          -Wredundant-decls -Winit-self -Wstrict-prototypes

# There are a number of C compiler flags that the C++ compiler doesn't like, or
# that the standard arduino libs dont satisfy.
NONCXXFLAGS = -std=gnu99 \
              -Wimplicit-int \
              -Winline \
              -Wmissing-prototypes \
              -Wold-style-declaration \
              -Wredundant-decls \
              -Wstrict-prototypes \

# Support building C++ files.  Currently this has mainly been used to ease
# the translation of Arduino modules away from C++, but maybe some people
# would like to use our interfaces from C++ for some reason.  See also the
# comments near CPPFLAGS, above.
CXXFLAGS += $(filter-out $(NONCXXFLAGS), $(CFLAGS))

# WARNING: I don't think I've actually exercised the assembly parts of this
# build system myself at all.
ASMFLAGS := -I. -mmcu=$(COMPILER_MCU)-x assembler-with-cpp \
            -Wa,-gstabs,-ahlms=$(firstword $(<:.S=.lst) $(<.s=.lst))

LDFLAGS := -mmcu=$(COMPILER_MCU) $(AVRLIBC_PRINTF_LDFLAGS) -lm \
           -Wl,-Map,$(TRG).map

ifeq ($(UPLOAD_METHOD), AVRISPmkII)
  # This flag shows up somewhere in the arduino build files.  But it seems to
  # cause trouble by making the built files different so things don't work when
  # the upload method is changed.  And it makes the uploads take longer.  And
  # it doesn't seem necessary.  And I originally thought it might be useful to
  # avoid nuking the bootloader when using AVRISPmkII programming, but it turns
  # out its impossible to prevent that.  So Its not enabled at the moment.
  # FIXME: so get rid of this garbage

  #LDFLAGS += -Wl,--section-start=.text=0x7800
endif


##### Rules {{{1

########## Interface Targets {{{2

# These are targets that users are likely to want to invoke directly (as
# arguments to make).

PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING := \
  echo "" ; \
  echo "Couldn't pulse DTR or upload failed.  Some possible reasons:" ; \
  echo "" ; \
  echo "  * The Device::SerialPort perl module isn't installed, see the " ; \
  echo "    diagnostic output above for more details." ; \
  echo "" ; \
  echo "  * You don't have write permission for the Arduino device file." ; \
  echo "    If you have a 'Permission denied' message above that refers to" ; \
  echo "    a file in /dev (perhaps /dev/ACM0 or /dev/USB0) this is likely" ; \
  echo "    the problem.  Make sure the Arduino is plugged in, and then" ; \
  echo "    take a look at the mentioned file with 'ls -l' and see if" ; \
  echo "    there is a group you can add yourself to to get write" ; \
  echo "    permission (on debian I had to add myself to the 'dialout'" ; \
  echo "    group).  Note that for group membership to take effect for you" ; \
  echo "    you may need to restart your system (or restart the Gnome" ; \
  echo "    Display Manager or somethimg) and login again.  You can see" ; \
  echo "    what groups you're in with the 'groups' command." ; \
  echo "" ; \
  echo "  * Your Arduino program is itself using the serial port, which" ; \
  echo "    prevents the programmer from working on my Arduino Uno rev. 3" ; \
  echo "    at leats.  Make sure that you do not have a screen session" ; \
  echo "    connected to the arduino, for example.  If you get a message" ; \
  echo "    like \"Couldn't open /dev/ttyACM0: Device or resource busy\"" ; \
  echo "    this is a particularly likely explanation." ; \
  echo "" ; \
  echo "  * You might need to press reset immediately after avrdude starts" ; \
  echo "    as required (with my Duemilanove anyway) if the above serial" ; \
  echo "    DTR pulse is not doing it (the DTR pulse does not seem to work" ; \
  echo "    if the arduino has been running for a while without being" ; \
  echo "    programmed, or if the program running on the arduino is using" ; \
  echo "    the serial line itself)." ; \
  echo "" ; \
  echo "  * The bootloader has been nuked (by programming with the" ; \
  echo "    AVRISPmkII for example).  See the replace_bootloader target in" ; \
  echo "    generic.mk." ; \
  echo ""

PRINT_AVRISPMKII_PROGRAMMING_FAILED_MESSAGE := \
  echo ; \
  echo Programming failed.  Is the AVRISPmkII hooked up?  Does the Arduino ; \
  echo have power?  The AVRISPmkII does not power the Arduino, but you can ; \
  echo just plug in the USB cable to power it and still use the AVRISPmkII ; \
  echo for programming. ; \
  echo

.PHONY: writeflash
writeflash: LOCK_AND_FUSE_AVRDUDE_OPTIONS := \
  $(call SHELL_CHECKED,./lock_and_fuse_bits_to_avrdude_options.perl \
                       $(PROGRAMMER_MCU) $(LOCK_AND_FUSE_SETTINGS))
writeflash: $(HEXTRG) avrdude_version_check
ifeq ($(UPLOAD_METHOD), arduino_bl)
  writeflash:
	# First kill any screen session started from run_screen.mk.
	screen -S $(SCREEN_SESSION_NAME) -X kill || true
	# Give screen time to die, once I still had programming fail w/o this.
	sleep 0.42 || sleep 1
	$(PROBABLY_PULSE_DTR) || \
          ($(PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING) && false) 1>&2
	$(AVRDUDE) -c $(AVRDUDE_ARDUINO_PROGRAMMERID)   \
                   -p $(PROGRAMMER_MCU) \
                   -P $(ACTUAL_ARDUINO_PORT) \
                   -b $(ACTUAL_ARDUINO_BAUD) \
                   -U flash:w:$(HEXROMTRG) \
                   $(LOCK_AND_FUSE_AVRDUDE_OPTIONS) || \
        ( $(PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING) ; false ) 1>&2
        # Sometimes the chip doesn't seem to reset after programming.
        # Pulsing the DTR again here often seems to help wake it up.  NOTE:
        # this probably sort of races serial line use on the arduino itself,
        # but it should pretty much always win :)
	$(PROBABLY_PULSE_DTR) || \
          ($(PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING) && false) 1>&2
else ifeq ($(UPLOAD_METHOD), AVRISPmkII)
  writeflash: binaries_suid_root_stamp
	$(AVRDUDE) -c avrispmkII \
                   -p $(PROGRAMMER_MCU) -P usb \
                   -U flash:w:$(HEXROMTRG) \
                   $(LOCK_AND_FUSE_AVRDUDE_OPTIONS) || \
          ( $(PRINT_AVRISPMKII_PROGRAMMING_FAILED_MESSAGE) && \
            false ) 1>&2
else
  $(error invalid UPLOAD_METHOD value '$(UPLOAD_METHOD)')
endif

# Support for writing a random (hopefully unique) 8 byte signature from
# /dev/random at the start of the EEPROM memory.  This isn't rolled into
# writeflash like the lock and fuse settings are because its probably
# something we want to do only once and not change all the time.  Note that
# other memory types (program flash, lock and fuse bits, etc.) are unaffected
# by this target, nor does the writeflash target affect the EEPROM area.
# This target only affects the first 8 bytes of the EEPROM, the rest is
# unchanged (FIXME: is that true?).
RIF = /tmp/generic.mk.random_id_file   # Random Id File
.PHONY: new_random_id
new_random_id:
	dd if=/dev/random of=$(RIF) bs=1 count=8
.PHONY: write_random_id_to_eeprom
write_random_id_to_eeprom: DUMP_RANDOM_ID = \
  od --format=x8 $(RIF) | cut -f 2 -d' ' | head -n 1
# ID Report String
write_random_id_to_eeprom: IRS = \
  "Wrote randomly generated 8 byte ID 0x"`$(DUMP_RANDOM_ID)` \
  "to start of chip EEPROM"
write_random_id_to_eeprom: avrdude_version_check new_random_id
ifeq ($(UPLOAD_METHOD), arduino_bl)
  write_random_id_to_eeprom:
	false # FIXME: ill in
else ifeq ($(UPLOAD_METHOD), AVRISPmkII)
  write_random_id_to_eeprom: binaries_suid_root_stamp
	$(AVRDUDE) -c avrispmkII \
                   -p $(PROGRAMMER_MCU) -P usb \
                   -U eeprom:w:$(RIF)
	@echo $(IRS)
else
  $(error invalid UPLOAD_METHOD value '$(UPLOAD_METHOD)')
endif


# This target is special: it uses an AVR ISPmkII and the bootloaded image
# that comes in the arduino download package and some magic goop copied here
# from arduino-1.0/hardware/arduino/bootloaders/atmega/Makefile to replace
# the bootloader and program the fuses as required for bootloading to work.
# The EEPROM is also erased.  The intent is to put the chip back into a
# natural Arduino-ish state :) This is useful if we've managed to nuke the
# bootloader some way or other.
#
# FIXME: ATmegaBOOT_168_atmega328.hex seems unchanged in latest distribution,
# but we should autotrack FIXME: by settin (SUT1, SUT0) to (1, 1), we
# would seem to be stomping a reserved state.  Why not just leave in (1, 0)
# (meaning slowly rising power)?
replace_bootloader: $(ACTUAL_ARDUINO_BOOTLOADER)  binaries_suid_root_stamp
	$(AVRDUDE) -c avrispmkII -p $(PROGRAMMER_MCU) -P usb \
                   -e -u \
                   `./lock_and_fuse_bits_to_avrdude_options.perl -- \
                      $(PROGRAMMER_MCU) \
                      BLB12=1 BLB11=1 BLB02=1 BLB01=1 LB2=1 LB1=1 \
                      BODLEVEL2=1 BODLEVEL1=0 BODLEVEL0=1 \
                      RSTDISBL=1 DWEN=1 SPIEN=0 WDTON=1 \
                      EESAVE=1 BOOTSZ1=0 BOOTSZ0=1 BOOTRST=0 \
                      CKDIV8=1 CKOUT=1 SUT1=1 SUT0=1 \
                      CKSEL3=1 CKSEL2=1 CKSEL1=1 CKSEL0=1` || \
          ( $(PRINT_AVRISPMKII_PROGRAMMING_FAILED_MESSAGE) && \
            false ) 1>&2
	$(AVRDUDE) -c avrispmkII -p $(PROGRAMMER_MCU) -P usb \
                   -U flash:w:$< \
                   `./lock_and_fuse_bits_to_avrdude_options.perl -- \
                      $(PROGRAMMER_MCU) \
                      BLB12=0 BLB11=0 BLB02=1 BLB01=1 LB2=1 LB1=1`

COMPILE_C = $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# FIXME: would we rather use a static pattern rule here?  They are generally
# cleaner and result in much better error messages in some circumstances,
# but are also less well understood.  This would take some thought as to
# which sorts of errors are more likely and how confusing the results are
# likely to be.
%.o: %.c
	$(COMPILE_C)

COMPILE_CXX = $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# FIXME: see above fixme about static pattern rules.
%.o: %.cpp
	$(COMPILE_CXX)

# Clean everything imaginable.
.PHONY: clean
clean:
	rm -rf $(TRG) $(TRG).map $(DUMPTRG) $(PROGNAME).out $(PROGNAME).out.map
	rm -rf $(OBJS)
	rm -rf $(LST) $(GDBINITFILE)
	rm -rf $(GENASMFILES)
	rm -rf $(HEXTRG)
	rm -rf *.deps
	rm -rf binaries_suid_root_stamp

########## Supporting Targets (Implementation) {{{2

dependency_checks: avrdude_version_check \
                   avrgcc_check \
                   avrlibc_check

.PHONY: avrdude_version_check
avrdude_version_check: VERSION_CHECKER := \
  perl -ne ' \
    m/version (\d+\.\d+(?:\d+))/ and \
     ($$1 >= 5.10 \
       or die "avrdude version 5.10 or later required (found version $$1)"); #'
avrdude_version_check:
	avrdude -? 2>&1 | $(VERSION_CHECKER)

.PHONY: avrgcc_check
avrgcc_check:
	@which fweg || \
        ( \
          echo -e "\navr-gcc binary '$(CC)' not found\n" 1>&2 ; \
          echo -e "Perhaps the CC make variable is set wrong, or perhaps no" ; \
          echo -e "AVR GCC cross compiler is installed?\n" \
          && false \
        )

.PHONY: avrlibc_check
avrlibc_check:
	@echo I do not know offhand a reliable way to check for avrlibc
	@echo availability...

# A bunch of stuff only works when suid root, this verifies the permissions.
# We are not trying to use avarice or avr-gdb at the moment, but if we were
# they would probably need this to talk over libusb as well.
binaries_suid_root_stamp: $(shell which $(AVRDUDE)) $(AVARICE) $(AVRGDB)
	ls -l $$(which $(AVRDUDE)) | grep --quiet -- '^-rws' || \
          ( \
            echo -e \\nError: $(AVRDUDE) binary is not suid root\\n 1>&2 && \
            false )
#	ls -l $$(which $(AVARICE)) | grep --quiet -- ' -rws' || \
#          ( \
#            echo -e \\nError: $(AVARICE) binary is not suid root\\n 1>&2 && \
#            false )
#	ls -l $$(which $(AVRGDB)) | grep --quiet -- ' -rws' || \
#          ( \
#            echo -e \\nError: $(AVRGDB) binary is not suid root\\n 1>&2 && \
#            false )
	touch $@
	@echo -e \\ngood the binaries which need to be suid are\\n

# In case the user is confused about quoting.
ifeq ($(DTR_PULSE_NOT_REQUIRED),"true")
  $(error "value of DTR_PULSE_NOT_REQUIRED shouldn't have quotes around it.")
endif
ifeq ($(DTR_PULSE_NOT_REQUIRED),'true')
  $(error "value of DTR_PULSE_NOT_REQUIRED shouldn't have quotes around it.")
endif

# Arduino Duemilanove and newer allow you to pulse the DTR line to
# trigger a reset, after which the bootloaded takes over so a new sketch
# can be uploaded without having to putsh the reset button on the board.
# In theory.  In practice it doesn't always work for me with this code at
# least.  And I don't know how it interacts with serial line use exactly.
# One the duemilanove it seems to sort of work, with Uno rev. 3 avrdude
# seems to always fail.  In case the user doesn't have the perl module
# we use for this and/or doesn't want to deal with it, they can set the
# DTR_PULSE_NOT_REQUIRED make variable to true.
PROBABLY_PULSE_DTR := perl -e ' \
  eval "require Device::SerialPort;" or ( \
    ($$@ =~ m/^Can.t locate Device\/SerialPort/ or die $$@) and \
    ("$(DTR_PULSE_NOT_REQUIRED)" eq "true" and exit(0)) or ( \
      print \
        $$@. \
        "\nDevice::SerialPort perl module not found.  You will need to set". \
        "\nDTR_PULSE_NOT_REQUIRED make variable to \"true\" and manually". \
        "\npress the reset button on the arduino immediately after the". \
        "\navrdude programming command goes off to successfully program the". \
        "\nchip.\n\n" \
      and exit(1) ) ); \
  my $$port = Device::SerialPort->new("$(ACTUAL_ARDUINO_PORT)") \
      or die "Cannot open $(ACTUAL_ARDUINO_PORT): $$!\n"; \
  $$port->pulse_dtr_on(100);' || \
  [ '$(DTR_PULSE_NOT_REQUIRED)' = true ]

# This target is just for testing out the PROBABLY_PULSE_DTR code.
test_probably_pulse_dtr:
	$(PROBABLY_PULSE_DTR) || \
          ($(PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING) && false) 1>&2

$(TRG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TRG) $^

%.hex: %.out
	$(OBJCOPY) -j .text -j .data -O $(HEXFORMAT) $< $@

%.ee.hex: %.out
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 \
                   -O $(HEXFORMAT) $< $@

# General build-too-much strategy.
$(OBJS): $(HEADERS) Makefile generic.mk

# WARNING: I don't think I've ever exercised this target.
%.s: %.c
	$(CC) -S $(CPPFLAGS) $(CFLAGS) $< -o $@

