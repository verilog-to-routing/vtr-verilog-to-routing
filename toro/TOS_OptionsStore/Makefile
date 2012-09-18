######################################################################

MAKEFILE = Makefile
MAKE = /usr/atria/bin/clearmake

######################################################################
# Compiler options
######################################################################

pC++sun8 = /apps/SUNWspro_6.0-U2/bin/CC
pC++linux24_amd64 = /usr/bin/gcc
pC++ = $(pC++$(ISP))

PICsun8 = -G -KPIC
PIClinux24_amd64 = -shared
PIC = $(PIC$(ISP))

CFLAGSsun8 = +w -xarch=v9 -library=iostream,Cstd -DLP64 -DSUN8
# ??? CFLAGSlinux24_amd64 = -W -Wno-deprecated -static -fPIC -DLINUX24_64
CFLAGSlinux24_amd64 = -Wall -Wpointer-arith -Wcast-qual -Wno-deprecated -static -fPIC -DLINUX24_64
CFLAGS = $(CFLAGS$(ISP)) \
	-I. \
	-I../TIO_InputOutputHandlers \
	-I../TC_Common \
	-I../pcre 

######################################################################
# Linker options
######################################################################

LINKERsun8 = /usr/ccs/bin/ld
LINKERlinux24_amd64 = /usr/bin/ld
LINKER = $(LINKER$(ISP))

LFLAGSsun8 = -G -library=iostream -xarch=v9
LFLAGSlinux24_amd64 = -shared
LFLAGS = $(LFLAGS$(ISP))

######################################################################
# Program files
######################################################################

SOL = $(SOL$(ISP))
SOLsun8 = so
SOLlinux24_amd64 = so

LIBSOsun8 = ../sun8/$(PROG).$(SOL)
LIBSOlinux24_amd64 = ../linux24_amd64/$(PROG).$(SOL)
LIBSO = $(LIBSO$(ISP))

PROG = libTOS_OptionsStore
EXE = ../$(ISP)/$(PROG).a

OBJSsun8	= sun8/TOS_OptionsStore.o \
                  sun8/TOS_ControlSwitches.o \
                  sun8/TOS_RulesSwitches.o \
                  sun8/TOS_InputOptions.o \
                  sun8/TOS_OutputOptions.o \
                  sun8/TOS_MessageOptions.o \
                  sun8/TOS_ExecuteOptions.o \
                  sun8/TOS_PackOptions.o \
                  sun8/TOS_PlaceOptions.o \
                  sun8/TOS_RouteOptions.o \
                  sun8/TOS_StringUtils.o
OBJSlinux24_amd64	= linux24_amd64/TOS_OptionsStore.o \
                          linux24_amd64/TOS_ControlSwitches.o \
                          linux24_amd64/TOS_RulesSwitches.o \
                          linux24_amd64/TOS_InputOptions.o \
                          linux24_amd64/TOS_OutputOptions.o \
                          linux24_amd64/TOS_MessageOptions.o \
                          linux24_amd64/TOS_ExecuteOptions.o \
                          linux24_amd64/TOS_PackOptions.o \
                          linux24_amd64/TOS_PlaceOptions.o \
                          linux24_amd64/TOS_RouteOptions.o \
                          linux24_amd64/TOS_StringUtils.o

OBJECTS = $(OBJS$(ISP))

######################################################################
# Target rules
######################################################################

PURE =
BUILD_TARGET =

.cxx.o:;
.cxx.a:;

all:
	$(MAKE) Debug
Optimized:
	$(MAKE) $(EXE) CFLAGS="-O2 $(CFLAGS)" BUILD_TARGET=Optimized
Debug:
	$(MAKE) $(EXE) CFLAGS="-g -DDEBUG $(CFLAGS)" BUILD_TARGET=Debug
Gprof:
	$(MAKE) $(EXE) CFLAGS="-pg -g -DDEBUG $(CFLAGS)" BUILD_TARGET=Debug
Pure:
	$(MAKE) $(EXE) CFLAGS="-g $(CFLAGS)" PURE=purify
Quantify:
	$(MAKE) $(EXE) CFLAGS="-g $(CFLAGS)" PURE=quantify

clean:
	rm -rf $(ISP)/*
	rm -f *.lis *.lst purify.log core 
	cleartool ls

######################################################################
# Program rules
######################################################################

ARCMDsun8 = $(pC++) -xar -xarch=v9 -library=iostream -o $(ISP)/$(PROG).a $(OBJECTS) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDlinux24_amd64 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMD = $(ARCMD$(ISP))
$(EXE): $(LIBSO)
	$(ARCMD)

$(OBJECTS): $(@F:.o=.cxx)
	$(PURE) $(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

$(LIBSOsun8): $(OBJECTS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

$(LIBSOlinux24_amd64): $(OBJECTS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

######################################################################
# Library rules
######################################################################

../$(ISP)/libTIO_InputOutputHandlers.a:
	cd ../TIO_InputOutputHandlers ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTC_Common.a:
	cd ../TC_Common ; $(MAKE) $(BUILD_TARGET)
