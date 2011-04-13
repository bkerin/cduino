# Make code common to all the different modules.  Some things defined here are
# easy to override in modules, those section titles contain "(Overridable)".

# vim: set foldmethod=marker

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
   $(error This makefile requires use of the -R/--no-builtin-variables option)
endif


##### Specs for Default Target Part (Overridable) {{{1

# Name for part being targetted, as known by the compiler and the uploader.
COMPILER_MCU ?= atmega328p
PROGRAMMER_MCU ?= m328p

# AVR libc uses this.  Some people might prefer to put it in a header though.
CPU_FREQ_DEFINE ?= -DF_CPU=16000000


##### Program Name, Constituent Object Files (Overridable) {{{1

# This is the paragraph that determines which files are being built into what.
PROGNAME ?= generic_program_name
OBJS ?= $(patsubst %.c,%.o,$(wildcard *.c))
HEADERS ?= $(wildcard *.h)


##### Upload Method (Overridable) {{{1

# This must be one of:
#
#    arduino_bl          Arduino bootloader over USB
#    AVRISPmkII          AVR ISP mkII programmer, preserving target bootloader
#    AVRISPmkII_no_bl    AVR ISP mkII programmer, overwriting any bootloader
#
# WARNING: the last options will make your Arduino unprogrammable using the
# normal bootloader method (though you can recover: see the replace_bootloader
# target).
#
# Note also that the offset used with with AVRISPmkII method is sizable and may
# fail with the small-memory AVR chips.  When migrating to really small chips
# AVRISPmkII_no_bl is the way to go in the end.
UPLOAD_METHOD ?= arduino_bl


##### Programs for Building and Uploading (Overridable) {{{1

# Probaly the only reason to override these is if you have your own builds
# somewhere special, or need to use something other than /dev/ttyUSB0.

# Compiler.  Note that some of these depend for overridability on the -R make
# option that this Makefile requires be used.
CC ?= avr-gcc
OBJCOPY ?= avr-objcopy
OBJDUMP ?= avr-objdump
SIZE ?= avr-size

# Uploader
AVRDUDE ?= avrdude
AVRDUDE_PROGRAMMERID ?= arduino
AVRDUDE_BAUD ?= 57600
AVRDUDE_PORT ?= /dev/ttyUSB0

# These programs could be useful, but I don't use them at the moment.
AVARICE =
AVRGDB =

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


##### Build Settings {{{1

HEXFORMAT := ihex

OPTLEVEL := s

CPPFLAGS +=
INC :=
CFLAGS += -I. $(INC) -std=gnu99 -gstabs $(CPU_FREQ_DEFINE) \
          -mmcu=$(COMPILER_MCU) -O$(OPTLEVEL) $(CTUNING) -Wall -Wextra \
          -Wimplicit-int -Wold-style-declaration -Wredundant-decls \
          -Wstrict-prototypes -Wmissing-prototypes

ASMFLAGS := -I. $(INC) -mmcu=$(COMPILER_MCU)-x assembler-with-cpp \
            -Wa,-gstabs,-ahlms=$(firstword $(<:.S=.lst) $(<.s=.lst))

# NORE: The arduino-0021/hardware/arduino/bootloaders/atmega/Makefile also
# seems to end up  putting this in LDFLAGS:
#      -Wl,--section-start=.text=0x7800
# Is it needed?  Is it needed only for bootloaders?
LDFLAGS := -mmcu=$(COMPILER_MCU) \
           -Wl,-Map,$(TRG).map

ifeq ($(UPLOAD_METHOD), AVRISPmkII)
  # The AVR ISP mkII doesn't know not to nuke bootloader unless we tell it.
  LDFLAGS += -Wl,--section-start=.text=0x7800
endif


##### Build Rules {{{1

# FIXME: I'd like to put a ':=' at the end up this line for consistency but
# there is a make bug in 3.81 and probably earlier that causes the define to
# fail if this is done, so we hav enothing and this default to recursively
# expanded.  FIXME: no worky, but replace_bootloader does, so probably the
# fuses need programmed
define DO_AVRISPMKII_UPLOAD
	# FIXME: not sure what the heck the below (from long knight) was about
	# FIXME: Since this command eventually makes the chip unprogrammable
	# (at least with the settings currently in use), shouldn't we do the
	# flash programming in the command below first?  It seems to work, I
	# suspect because the chip doesn't get rebooted in between, but still...
	# FIXME: -P usb ?!?  needa number?  Or not use one to cause avrdude
	# to search for usb automatically (but then do we need
	# binaries_suid_root_stamp?)
	$(POSSIBLE_FUSE_PROGRAMMING_COMMAND)
	# FIXME: WORK POINT: resetting fuse bits here doesn't change problem
	$(AVRDUDE) -c avrispmkII -p $(PROGRAMMER_MCU) -P usb \
                   `lock_and_fuse_bits_to_avrdude_options.perl -- \
                      m328p \
                      BLB12=1 BLB11=1 BLB02=1 BLB01=1 LB2=1 LB1=1 \
                      BODLEVEL2=1 BODLEVEL1=0 BODLEVEL0=1 \
                      RSTDISBL=1 DWEN=1 SPIEN=0 WDTON=1 \
                      EESAVE=1 BOOTSZ1=0 BOOTSZ0=1 BOOTRST=0 \
                      CKDIV8=1 CKOUT=1 SUT1=1 SUT0=1 \
                      CKSEL3=1 CKSEL2=1 CKSEL1=1 CKSEL0=1` \
                   -U flash:w:$(HEXROMTRG)
endef


# FIXME: one of these is needed to keep the bootloader update
# method working: -Map in LDFLAGS or maybe --section-start=blah that also ends
# up in LDFLAGS, see arduino-0021/hardware/arduino/bootloaders/atmega/Makefile
# when LDSECTION gets mentioned.  Or maybe it was just that reset wasn't
# getting pushed... There is also the DA versus D8 in HFUSE issue which
# I think means a 1024 word boot program (DA) or 2048 boot program (D8).
.PHONY: writeflash
ifeq ($(UPLOAD_METHOD), arduino_bl)
  writeflash: $(HEXTRG)
	$(PULSE_DTR)
	$(AVRDUDE) -F -c $(AVRDUDE_PROGRAMMERID)   \
                   -p $(PROGRAMMER_MCU) -P $(AVRDUDE_PORT) -b $(AVRDUDE_BAUD) \
                   -U flash:w:$(HEXROMTRG) || \
        ( $(PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING) ; false ) 1>&2
else ifeq ($(UPLOAD_METHOD), AVRISPmkII)
  writeflash: $(HEXTRG) binaries_suid_root_stamp
	$(DO_AVRISPMKII_UPLOAD)
else ifeq ($(UPLOAD_METHOD), AVRISPmkII_no_bl)
  writeflash: $(HEXTRG) binaries_suid_root_stamp
	$(DO_AVRISPMKII_UPLOAD)
else
  $(error invalid UPLOAD_METHOD value '$(UPLOAD_METHOD)')
endif

# A bunch of stuff only works when suid root, this verifies the permissions.
# We're not trying to use avarice or avr-gdb at the moment, but if we were they
# would probably need this to talk over libusb as well.
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

# Arduino Duemilanove and newer allow you to pulse the DTR line to trigger a
# reset, after which the bootloaded takes over so a new sketch can be uploaded
# without having to putsh the reset button on the board.  In theory.  In
# practice it doesn't seem to work well for me with this code at least.  And I
# dont' know how it interacts with serial line use exactly.
PULSE_DTR := perl -e ' \
  use Device::SerialPort; \
  my $$port = Device::SerialPort->new("/dev/ttyUSB0") \
      or die "Cannot open /dev/ttyUSB0: $$!\n"; \
  $$port->pulse_dtr_on(100); '

PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING := \
  echo "Upload failed.  Might need to press reset immediately after" ; \
  echo "starting upload as required (with my Duimilanove anyway)" ; \
  echo "if the above serial DTR pulse isn't doint it (the DTR pulse" ; \
  echo "doesn't seem to work if the arduino has been running for a" ; \
  echo "while without being programmed)." ; \
  echo ""

# This target is special: it uses an AVR ISPmkII and  the bootloaded image that
# comes in the arduino download package and some magic goop copied here from
# arduino-0021/hardware/arduino/bootloaders/atmega/Makefile to replace the
# bootloader and program the fuses as required for bootloading to work.  This
# is useful if we've managed to nuke the bootloader some way or other.
replace_bootloader: ATmegaBOOT_168_atmega328.hex binaries_suid_root_stamp
	# This serial port reset may be uneeded these days.
	$(PULSE_DTR)
	# FIXME: WORK POINT: verify that this command still works with gen cmd
	$(AVRDUDE) -c avrispmkII -p $(PROGRAMMER_MCU) -P usb \
                   -e -u \
                   `lock_and_fuse_bits_to_avrdude_options.perl -- \
                      m328p \
                      BLB12=1 BLB11=1 BLB02=1 BLB01=1 LB2=1 LB1=1 \
                      BODLEVEL2=1 BODLEVEL1=0 BODLEVEL0=1 \
                      RSTDISBL=1 DWEN=1 SPIEN=0 WDTON=1 \
                      EESAVE=1 BOOTSZ1=0 BOOTSZ0=1 BOOTRST=0 \
                      CKDIV8=1 CKOUT=1 SUT1=1 SUT0=1 \
                      CKSEL3=1 CKSEL2=1 CKSEL1=1 CKSEL0=1` || \
        ( $(PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING) ; false ) 1>&2
	# FIXME: would be nice to reprogram lock using out program, but its
	# all or nothing...
	$(AVRDUDE) -c avrispmkII -p $(PROGRAMMER_MCU) -P usb \
                   -U flash:w:$< -U lock:w:0x0f:m

$(TRG): $(OBJS) $(LDLIBS)
	$(CC) $(LDFLAGS) -o $(TRG) $^

%.hex: %.out
	$(OBJCOPY) -j .text -j .data -O $(HEXFORMAT) $< $@

%.ee.hex: %.out
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 \
                   -O $(HEXFORMAT) $< $@

# General build-too-much strategy.
$(OBJS): $(HEADERS) Makefile generic.mk

COMPILE = $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(COMPILE)

%.s: %.c
	$(CC) -S $(CPPFLAGS) $(CFLAGS) $< -o $@

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
