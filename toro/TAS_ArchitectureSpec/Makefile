#####################################################################

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
	-I../TLO_LogicalObjects \
	-I../TPO_PhysicalObjects \
	-I../TGS_GeometricShapes \
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

PROG = libTAS_ArchitectureSpec
EXE = ../$(ISP)/$(PROG).a

OBJSsun8	= sun8/TAS_ArchitectureSpec.o \
                  sun8/TAS_Config.o \
                  sun8/TAS_PhysicalBlock.o \
                  sun8/TAS_Cell.o \
                  sun8/TAS_SwitchBox.o \
                  sun8/TAS_Segment.o \
                  sun8/TAS_Mode.o \
                  sun8/TAS_ConnectionFc.o \
                  sun8/TAS_Interconnect.o \
                  sun8/TAS_PinAssign.o \
                  sun8/TAS_GridAssign.o \
                  sun8/TAS_TimingDelay.o \
                  sun8/TAS_TimingDelayLists.o \
                  sun8/TAS_ChannelWidth.o \
                  sun8/TAS_StringUtils.o
OBJSlinux24_amd64	= linux24_amd64/TAS_ArchitectureSpec.o \
                          linux24_amd64/TAS_Config.o \
                          linux24_amd64/TAS_PhysicalBlock.o \
                          linux24_amd64/TAS_Cell.o \
                          linux24_amd64/TAS_SwitchBox.o \
                          linux24_amd64/TAS_Segment.o \
                          linux24_amd64/TAS_Mode.o \
                          linux24_amd64/TAS_ConnectionFc.o \
                          linux24_amd64/TAS_Interconnect.o \
                          linux24_amd64/TAS_PinAssign.o \
                          linux24_amd64/TAS_GridAssign.o \
                          linux24_amd64/TAS_TimingDelay.o \
                          linux24_amd64/TAS_TimingDelayLists.o \
                          linux24_amd64/TAS_ChannelWidth.o \
                          linux24_amd64/TAS_StringUtils.o

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

../$(ISP)/libTLO_LogicalObjects.a:
	cd ../TLO_LogicalObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTPO_PhysicalObjects.a:
	cd ../TPO_PhysicalObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTGS_GeometricShapes.a:
	cd ../TGS_GeometricShapes ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTIO_InputOutputHandlers.a:
	cd ../TIO_InputOutputHandlers ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTC_Common.a:
	cd ../TC_Common ; $(MAKE) $(BUILD_TARGET)
