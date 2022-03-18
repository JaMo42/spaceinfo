CXX=g++
CXXFLAGS=-std=c++20 -Wall -Wextra -pedantic
LDFLAGS=-lncurses
VGFLAGS=--track-origins=yes

ifeq ($(DEBUG),1)
	CXXFLAGS += -O0 -g
else
	CXXFLAGS += -O3 -march=native -mtune=native
endif

all: spaceinfo

build/space_info.o: source/space_info.cc source/space_info.hh source/stdafx.hh
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/display.o: source/display.cc source/display.hh source/stdafx.hh
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/select.o: source/select.cc source/select.hh source/stdafx.hh
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/options.o: source/options.cc source/options.hh
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/main.o: source/main.cc source/display.hh source/space_info.hh source/stdafx.hh
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/input.o: source/input.cc source/input.hh source/stdafx.hh
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/help.o: source/nc-help/help.c
	$(CXX) $(CXXFLAGS) -c -o $@ $<

spaceinfo: build/space_info.o build/display.o build/select.o build/options.o \
           build/main.o build/input.o build/help.o
	$(CXX) -o $@ $^ $(LDFLAGS)

vg: spaceinfo
	valgrind $(VGFLAGS) ./spaceinfo $(VG_ARGS)

vgclean:
	rm -f vgcore.*

clean: vgclean
	rm -f spaceinfo build/main.o build/display.o build/space_info.o

.PHONY: all vg vgclean clean
