# Make code common to all the different modules.  Some things defined here are
# easy to override in modules, those section titles contain "(Overridable)".

# vim: foldmethod=marker

##### Make Settings {{{1

# This is sensible stuff for use but could confuse an experience Make
# programmer so its out front here.

# Delete files produced by rules the commands of which return non-zero.
.DELETE_ON_ERROR:
# Disable all suffix rules.
.SUFFIXES:

# This Makefile requires that the -R/--no-builtin-variables option be used.
# Implicit rules and default variables cause much more trouble thatn they are
# worth (yes variables do too, not just rules: for example, when using the
# fill-in-undefined-variables-with-defaults design pattern that we're using
# here, default values for variables can screw things up).
HAVE_NO_BUILTIN_VARIABLES_OPTION := $(shell echo $(MAKEFLAGS) | grep -e R)
ifndef HAVE_NO_BUILTIN_VARIABLES_OPTION
   $(error This makefile requires use of the -R/--no-builtin-variables make \
           option)
endif


##### Specs for Default Target Part (Overridable) {{{1

# Name for part being targetted, as known by the compiler and the uploader.
COMPILER_MCU ?= atmega328p
PROGRAMMER_MCU ?= m328p

# AVR libc uses this.  Some people might prefer to put it in a header though.
CPU_FREQ_DEFINE ?= -DF_CPU=16000000


##### Program Name, Constituent Object Files (Overridable) {{{1

# This is the paragraph that determines which files are being built into what.
PROGNAME ?= program_to_upload
OBJS ?= $(patsubst %.c,%.o,$(wildcard *.c))
HEADERS ?= $(wildcard *.h)

echo_pn:
	echo $(PROGNAME)


##### Upload Method (Overridable) {{{1

# This must be one of:
#
#    arduino_bl          Arduino bootloader over USB
#    AVRISPmkII          AVR ISP mkII programmer, overwriting any bootloader
#
# WARNING: the AVRISPmkII options will make your Arduino unprogrammable using
# the normal bootloader method (though you can recover: see the
# replace_bootloader target).
UPLOAD_METHOD ?= arduino_bl

# If this is set to true, then if the perl modules required to pulse the DTR
# line of the serial port aren't found or the DTR pulse code fails for some
# reason, it is not considered a fatal error (but the user will then have to
# push the reset button on the arduino immediately after the avrdude
# programming command goes off to program the device).
DTR_PULSE_NOT_REQUIRED ?= false

# Uploader program.
AVRDUDE ?= avrdude

# Upload program parameters for when the arduino_bl UPLOAD_METHOD is used.
AVRDUDE_ARDUINO_PROGRAMMERID ?= arduino
AVRDUDE_ARDUINO_PORT ?= /dev/ttyUSB0
AVRDUDE_ARDUINO_BAUD ?= 57600


##### Compilers, Assemblers, etc. (Overridable) {{{1

# Probaly the only reason to override these is if you have your own builds
# somewhere special, or need to use something other than /dev/ttyUSB0.

# Compiler.  Note that some of these depend for overridability on the -R make
# option that this Makefile requires be used.
CC ?= avr-gcc
OBJCOPY ?= avr-objcopy
OBJDUMP ?= avr-objdump
SIZE ?= avr-size

# These programs could be useful, but I don't use them at the moment.
AVARICE ?=
AVRGDB ?=


##### Fuse Settings (Overridable) {{{1

# WARNING: UNTESTED.  Fuse settings to be programmed, in the form of a list of
# setting for individual bits as they are named in for example the Mega328P
# datasheet.  For example, setting this to 'BODLEVEL2=1 BODLEVEL1=0
# BODLEVEL0=1' will enable brown out detection for a typical value of 2.7 V
# supply as described in the datasheet.  Note that lock and fuse bits are
# always written a byte at a time: if any bits of a byte are specified, all
# should be (or else they will take their default values).  Note also that the
# arduino uses a number of non-default lock and fuse settings; changing them
# may break things.  On the other hand, if you want to use in-system
# programming to program a minimal system at a different clock rate, you'll
# need to learn about fuse settings.  See the uses of this variable for more
# details.
LOCK_AND_FUSE_SETTINGS ?=


##### printf() Feature Support (Overridable) {{{1

# AVR libc provides minimal, standard, and (mostly) full-featured printf
# implementations, allowing you to trade features for small code size.  See the
# discussion in
# http://www.nongnu.org/avr-libc/user-manual/group__avr__stdio.html for
# details.  The default is to use the AVR libc default printf implementation,
# which supports most normal printf features except floating point support.
# Other possible values which individual modules may use are "-Wl,-u,vfprintf
# -lprintf_min" and "-Wl,-u,vfprintf -lprintf_flt -lm".
AVRLIBC_PRINTF_LDFLAGS ?=


##### Debugging Macro CPP Flag (Overridable) {{{1

# Its often convenient to have a way to compile code when debugging will
# be done, this variable gets put in CPPFLAGS st you can say things like
# '#ifdef DEBUG' in the code to do logging and such.  For example one might
# assign it a value of '-DDEBUG'.
CPP_DEBUG_DEFINE_FLAGS ?=


##### Computed File Names {{{1

CFILES := $(patsubst %.o,%.c,$(OBJS))
LDLIBS :=

# Magical files that one doesn't see in non-microcontroller gcc development.
TRG = $(PROGNAME).out
DUMPTRG = $(PROGNAME).s
HEXROMTRG = $(PROGNAME).hex
HEXTRG = $(HEXROMTRG) $(PROGNAME).ee.hex
LSTFILES := $(patsubst %.o,%.c,$(OBJS))
GENASMFILES := $(patsubst %.o,%.s,$(OBJS))


##### Build Settings and Flags {{{1

HEXFORMAT := ihex

OPTLEVEL := s

CPPFLAGS += $(CPP_DEBUG_DEFINE_FLAGS)
INC :=
# FIXME: -I and $(INC) should be in CPPFLAGS, do and test.
CFLAGS += -I. $(INC) -std=gnu99 -gstabs $(CPU_FREQ_DEFINE) \
          -mmcu=$(COMPILER_MCU) -O$(OPTLEVEL) $(CTUNING) -Wall -Wextra \
          -Wimplicit-int -Wold-style-declaration -Wredundant-decls \
          -Wstrict-prototypes -Wmissing-prototypes

# FIXME: -I and $(INC) should be in CPPFLAGS? do and test.
ASMFLAGS := -I. $(INC) -mmcu=$(COMPILER_MCU)-x assembler-with-cpp \
            -Wa,-gstabs,-ahlms=$(firstword $(<:.S=.lst) $(<.s=.lst))

LDFLAGS := -mmcu=$(COMPILER_MCU) $(AVRLIBC_PRINTF_LDFLAGS) -Wl,-Map,$(TRG).map

ifeq ($(UPLOAD_METHOD), AVRISPmkII)
  # This flag shows up somewhere in the arduino build files.  But it seems to
  # cause trouble by making the built files different so things don't work when
  # the upload method is changed.  And it makes the uploads take longer.  And
  # it doesn't seem necessary.  And I originally thought it might be useful to
  # avoid nuking the bootloader when using AVRISPmkII programming, but it turns
  # out its impossible to prevent that.  So Its not enabled at the moment.

  #LDFLAGS += -Wl,--section-start=.text=0x7800
endif


##### Rules {{{1

########## Interface Targets {{{2

# These are targets that users are likely to want to invoke directly (as
# arguments to make).

.PHONY: writeflash
writeflash: LOCK_AND_FUSE_AVRDUDE_OPTIONS := \
  $(shell ./lock_and_fuse_bits_to_avrdude_options.perl \
            $(PROGRAMMER_MCU) $(LOCK_AND_FUSE_SETTINGS))
writeflash: $(HEXTRG) avrdude_version_check
ifeq ($(UPLOAD_METHOD), arduino_bl)
  writeflash:
	$(PROBABLY_PULSE_DTR)
	$(AVRDUDE) -c $(AVRDUDE_ARDUINO_PROGRAMMERID)   \
                   -p $(PROGRAMMER_MCU) \
                   -P $(AVRDUDE_ARDUINO_PORT) \
                   -b $(AVRDUDE_ARDUINO_BAUD) \
                   -U flash:w:$(HEXROMTRG) \
                   $(LOCK_AND_FUSE_AVRDUDE_OPTIONS) || \
        ( $(PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING) ; false ) 1>&2
        # FIXME: sometimes the chip doesn't seem to reset after programming.
        # And sometimes the pulse to program doesn't work for unknown reasons
        # as noted elsewhere.  WHY?!?!  For now we pulse DTR again here which
        # seems to wake it up after programming which it sometimes needs.
	$(PROBABLY_PULSE_DTR)
else ifeq ($(UPLOAD_METHOD), AVRISPmkII)
  writeflash: binaries_suid_root_stamp
	$(AVRDUDE) -c avrispmkII \
                   -p $(PROGRAMMER_MCU) -P usb \
                   -U flash:w:$(HEXROMTRG) \
                   $(LOCK_AND_FUSE_AVRDUDE_OPTIONS)
else
  $(error invalid UPLOAD_METHOD value '$(UPLOAD_METHOD)')
endif

# This target is special: it uses an AVR ISPmkII and  the bootloaded image that
# comes in the arduino download package and some magic goop copied here from
# arduino-0021/hardware/arduino/bootloaders/atmega/Makefile to replace the
# bootloader and program the fuses as required for bootloading to work.  This
# is useful if we've managed to nuke the bootloader some way or other.
replace_bootloader: ATmegaBOOT_168_atmega328.hex binaries_suid_root_stamp
	# This serial port reset may be uneeded these days.
	$(PROBABLY_PULSE_DTR)
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
        ( $(PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING) ; false ) 1>&2
	$(AVRDUDE) -c avrispmkII -p $(PROGRAMMER_MCU) -P usb \
                   -U flash:w:$< \
                   `./lock_and_fuse_bits_to_avrdude_options.perl -- \
                      $(PROGRAMMER_MCU) \
                      BLB12=0 BLB11=0 BLB02=1 BLB01=1 LB2=1 LB1=1`

COMPILE = $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(COMPILE)

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

# Arduino Duemilanove and newer allow you to pulse the DTR line to trigger a
# reset, after which the bootloaded takes over so a new sketch can be uploaded
# without having to putsh the reset button on the board.  In theory.  In
# practice it doesn't always work for me with this code at least.  And I dont'
# know how it interacts with serial line use exactly.  In case the user doesn't
# have the perl module we use for this and/or doesn't want to deal with it,
# they can set the DTR_PULSE_NOT_REQUIRED make variable to true.
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
  my $$port = Device::SerialPort->new("/dev/ttyUSB0") \
      or die "Cannot open /dev/ttyUSB0: $$!\n"; \
  $$port->pulse_dtr_on(100);' || \
  [ '$(DTR_PULSE_NOT_REQUIRED)' = true ]

# This target is just for testing out the PROBABLY_PULSE_DTR code.
test_probably_pulse_dtr:
	$(PROBABLY_PULSE_DTR)

PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING := \
  echo "Upload failed.  You might need to press reset immediately after" ; \
  echo "avrdude starts as required (with my Duemilanove anyway) if the" ; \
  echo "above serial DTR pulse isn't doing it (the DTR pulse doesn't seem" ; \
  echo "to work if the arduino has been running for a while without being" ; \
  echo "programmed, or if the program running on the arduino is using the" ; \
  echo "serial line itself).  Or maybe the bootloader has been nuked (by" ; \
  echo "programming with the AVRISPmkII for example)?" ; \
  echo ""

$(TRG): $(OBJS) $(LDLIBS)
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

