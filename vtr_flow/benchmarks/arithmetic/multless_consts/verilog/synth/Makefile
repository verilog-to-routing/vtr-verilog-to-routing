CC=gcc
CXX=g++
CXXFLAGS ?= -O3 -fomit-frame-pointer -Wall -Wextra -pedantic -Werror -Wno-long-long
CFLAGS=$(CXXFLAGS)
LINKFLAGS=

all: chains synth acm libacm.a
#acsel adj graphs

debug: 
	CXXFLAGS=-g $(MAKE) synth

ac1: chains_util.o ac1.o
	$(CXX) $(CXXFLAGS) -o ac1 ac1.o chains_util.o $(LINKFLAGS)

acsel: chains_util.o acsel.o cmdline.o
	$(CXX) $(CXXFLAGS) -o acsel acsel.o chains_util.o cmdline.o $(LINKFLAGS)

chains: chains_util.o chains.o
	$(CXX) $(CXXFLAGS) -o chains chains.o chains_util.o $(LINKFLAGS)

synth.cpp: adder.h chains.h

synth: cmdline.o chains_util.o 21.costs.o ac1.o adder.o synth_main.o adder.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LINKFLAGS)

sd: cmdline.o chains_util.o 21.costs.o ac1.o adder.o sd.o
	$(CXX) $(CXXFLAGS) -o sd cmdline.o chains_util.o ac1.o 21.costs.o adder.o \
        sd.o $(LINKFLAGS)

libsynth.a: synth.o cmdline.o chains_util.o 21.costs.o ac1.o adder.o
	xiar -rv libsynth.a synth.o cmdline.o chains_util.o 21.costs.o ac1.o adder.o
	ranlib libsynth.a

acm: acm-main cmdline.o chains_util.o 21.costs.o synth.o ac1.o adder.o 
	$(CXX) $(CXXFLAGS) -o acm acm.o cmdline.o chains_util.o 21.costs.o ac1.o adder.o $(LINKFLAGS)

acm-main: acm.cpp acm.h
	$(CXX) $(CXXFLAGS) -DACM_MAIN -c -o acm.o acm.cpp

acm-no-main: acm.cpp acm.h
	$(CXX) $(CXXFLAGS) -c -o acm.o acm.cpp

libacm.a: acm-no-main chains_util.o  21.costs.o synth.cpp
	ar -rv libacm.a acm.o chains_util.o 21.costs.o
	ranlib libacm.a

testacm: testacm.cpp libacm.a
	$(CXX) $(CXXFLAGS) -o testacm testacm.cpp libacm.a $(LINKFLAGS)

include Makefile.deps
