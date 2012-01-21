
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

