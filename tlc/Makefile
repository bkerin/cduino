
include generic.mk

CXX := avr-g++

NONCXXFLAGS = -std=gnu99 \
              -Wimplicit-int \
              -Wmissing-prototypes \
              -Wold-style-declaration \
              -Wstrict-prototypes \

CXXFLAGS = $(filter-out $(NONCXXFLAGS), $(CFLAGS))

OBJS += $(patsubst %.cpp,%.o,$(wildcard *.cpp))

$(TRG): $(OBJS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# This silly little target just increments a number in the test text we
# print, to let us feel sure that what we are seeing on the LCD is actually
# the new version (since the LCD doesn't necessarilly get reset with the
# rest of the arduino).
wfovn:
	perl -p -i.bak -e 'm/42.(\d\d)/;' \
                       -e '$$mvn = $$1;' \
                       -e '$$nmvn = $$mvn + 1;' \
                       -e 's/42\.$$mvn/42.$$nmvn/;' \
                       main.cpp ; \
                       $(MAKE) writeflash
