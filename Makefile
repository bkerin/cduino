# My simple makefile that can build some libraries in subdirectories, and
# different programs by editing a few lines.

##### Specs for part being targetted #####

# Name for part being targetted, as known by the compiler and the uploader.
COMPILER_MCU := atmega328p
PROGRAMMER_MCU=m328p

CPU_FREQ_DEFINE := -DF_CPU=16000000


##### Project to be built, and constituent object files and libraries #####

# This is the paragraph that determines which files are being built into what.
PROJNAME := uart_test
OBJS := stdiodemo.o uart.o
HEADERS := $(wildcard *.h)

# Example of what to set for a hypothetical different project with two source
# files PC_Comm.o and Demonstrator.o.
#PROJNAME := pc_comm
#OBJS := PC_Comm.o Demonstrator.o


# Example of what to set for a hypothetical different project with two source
# files PC_Comm.o and Demonstrator.o.
#PROJNAME := pc_comm
#OBJS := PC_Comm.o Demonstrator.o

##### Programs used for building and uploading #####

# Compiler
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size

# Uploader
AVRDUDE = avrdude


##### Computed files and directories #####

CFILES := $(patsubst %.o,%.c,$(OBJS))
LDLIBS :=

# Magical files that one doesn't see in non-uc gcc development.
TRG = $(PROJNAME).out
DUMPTRG = $(PROJNAME).s
HEXROMTRG = $(PROJNAME).hex
HEXTRG = $(HEXROMTRG) $(PROJNAME).ee.hex
LSTFILES := $(patsubst %.o,%.c,$(OBJS))
GENASMFILES := $(patsubst %.o,%.s,$(OBJS))


##### Build and upload program settings #####

AVRDUDE_PROGRAMMERID := arduino
AVRDUDE_BAUD := 57600
AVRDUDE_PORT := /dev/ttyUSB0

HEXFORMAT := ihex

OPTLEVEL := s

CPPFLAGS +=
INC :=
CFLAGS += -I. $(INC) -std=gnu99 -gstabs $(CPU_FREQ_DEFINE) \
          -mmcu=$(COMPILER_MCU) -O$(OPTLEVEL) $(CTUNING) -Wall -Wextra -Wimplicit-int -Wold-style-declaration -Wredundant-decls \
          -Wstrict-prototypes -Wmissing-prototypes

ASMFLAGS := -I. $(INC) -mmcu=$(COMPILER_MCU)-x assembler-with-cpp \
            -Wa,-gstabs,-ahlms=$(firstword $(<:.S=.lst) $(<.s=.lst))

# NORE: The arduino-0021/hardware/arduino/bootloaders/atmega/Makefile also
# ends up  putting this in LDFLAGS:
#      -Wl,--section-start=.text=0x7800
# Is it needed?  Is it needed only for bootloaders?
LDFLAGS := -Wl,-Map,$(TRG).map \
           -mmcu=$(COMPILER_MCU)

##### Build rules #####

# Disable all suffix rules.
.SUFFIXES:

# A bunch of stuff only works when suid root, this verifies the permissions.
# I'm not trying to use avarice or avr-gdb at the moment, but if I was they
# would probably need this to talk over libusb as well.
AVARICE := $(shell which $(AVARICE))
AVRGDB := $(shell which $(AVRGDB))
binaries_suid_root_stamp: $(shell which $(AVRDUDE)) $(AVARICE_BIN) $(AVRGDB_BIN)
	ls -l $$(which $(AVRDUDE)) | grep --quiet -- '-rwsr-xr-x'
	#ls -l $(AVARICE_BIN) | grep --quiet -- '-rwsr-xr-x'
	#ls -l $(AVRGDB_BIN)  | grep --quiet -- '-rwsr-xr-x'
	touch $@
	@echo good the binaries which need to be suid are

PULSE_DTR = perl -e ' \
  use Device::SerialPort; \
  my $$port = Device::SerialPort->new("/dev/ttyUSB0") \
      or die "Cannot open /dev/ttyUSB0: $$!\n"; \
  $$port->pulse_dtr_on(100); '

PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING := \
  echo "Upload failed.  Might need to press reset immediately after" ; \
  echo "starting upload as required (with my Duimilanove anyway)" ; \
  echo "if the above serial DTR pulse isn't doint it (the DTR pulse" ; \
  echo "doesn't seem to work if the arduino has been running for a" ; \
  echo "while without being programmed).  Or perhaps having the" ; \
  echo "avrispmkII and the Arduino plugged in at the same time is" ; \
  echo "causing problems?  Maybe you need to unplug the arduino for" ; \
  echo "a while after replacing the bootloader?" ; \
  echo ""

# This target is special: it uses the bootloaded image that comes in the
# arduino download package and some magic goop copied here from
# arduino-0021/hardware/arduino/bootloaders/atmega/Makefile to replace the
# bootloader
# and program the fuses as required for bootloading to work.  This is useful if
# we've managed to nuke the bootloader some way or other.
# HFUSE = 1101 1010
replace_bootloader: HFUSE := DA
# LFUSE = 1111 1111
replace_bootloader: LFUSE := FF
# EFUSE = 0000 0101
replace_bootloader: EFUSE := 05
replace_bootloader: ATmegaBOOT_168_atmega328.hex binaries_suid_root_stamp
	# This serial port reset may be uneeded these days.
	$(PULSE_DTR)
	$(AVRDUDE) -c avrispmkII -p $(PROGRAMMER_MCU) -P usb \
                   -e -u -U lock:w:0x3f:m \
                   -U efuse:w:0x$(EFUSE):m \
                   -U hfuse:w:0x$(HFUSE):m \
                   -U lfuse:w:0x$(LFUSE):m || \
        ( $(PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING) ; false ) 1>&2
	$(AVRDUDE) -c avrispmkII -p $(PROGRAMMER_MCU) -P usb \
                   -U flash:w:$< -U lock:w:0x0f:m

# FIXME: WORK POINT: one of these is needed to keep the bootloader update
# method working: -Map in LDFLAGS or maybe --section-start=blah that also ends
# up in LDFLAGS, see arduino-0021/hardware/arduino/bootloaders/atmega/Makefile
# when LDSECTION gets mentioned.  Or maybe it was just that reset wasn't
# getting pushed... There is also the DA versus D8 in HFUSE issue which
# I think means a 1024 word boot program (DA) or 2048 boot program (D8).
.PHONY: writeflash
writeflash: $(HEXTRG)
	$(PULSE_DTR)
	$(AVRDUDE) -F -c $(AVRDUDE_PROGRAMMERID)   \
                   -p $(PROGRAMMER_MCU) -P $(AVRDUDE_PORT) -b $(AVRDUDE_BAUD) \
                   -U flash:w:$(HEXROMTRG) || \
        ( $(PRINT_ARDUINO_DTR_TOGGLE_WEIRDNESS_WARNING) ; false ) 1>&2

$(TRG): $(OBJS) $(LDLIBS)
	$(CC) $(LDFLAGS) -o $(TRG) $^

%.hex: %.out
	$(OBJCOPY) -j .text -j .data -O $(HEXFORMAT) $< $@

%.ee.hex: %.out
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 \
                   -O $(HEXFORMAT) $< $@

COMPILE = $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(COMPILE)

%.s: %.c
	$(CC) -S $(CPPFLAGS) $(CFLAGS) $< -o $@

# Clean everything imaginable.
.PHONY: clean
clean:
	rm -rf $(TRG) $(TRG).map $(DUMPTRG) $(PROJNAME).out $(PROJNAME).out.map
	rm -rf $(OBJS)
	rm -rf $(LST) $(GDBINITFILE)
	rm -rf $(GENASMFILES)
	rm -rf $(HEXTRG)
	rm -rf libavr_hacked.a
	rm -rf avrlib_hacked/*.o
	rm -rf *.deps
	rm -rf binaries_suid_root_stamp
