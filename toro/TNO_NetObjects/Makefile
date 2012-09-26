######################################################################
#
# Toro net objects library
#
# Note: This make file is dependent on the platform-specific ISP 
#       environment variable.  ISP is platform-specific and is used 
#       to determine various build switches and directories.
#
#       This make file currently supports:
#       - setenv ISP linux24_amd64
#       - setenv ISP sun8
#
######################################################################

######################################################################
# Compiler options
######################################################################

pC++linux24_amd64 = /usr/bin/gcc
pC++sun8 = /apps/SUNWspro_6.0-U2/bin/CC
pC++ = $(pC++$(ISP))

PIClinux24_amd64 = -shared
PICsun8 = -G -KPIC
PIC = $(PIC$(ISP))

CFLAGSlinux24_amd64 = -Wall -Wpointer-arith -Wcast-qual -static -fPIC -DLINUX24_64
CFLAGSsun8 = +w -xarch=v9 -library=iostream,Cstd -DLP64 -DSUN8
CFLAGS = $(CFLAGS$(ISP)) \
	-I. \
	-I../TGS_GeometricShapes \
	-I../TIO_InputOutputHandlers \
	-I../TC_Common \
	-I../pcre 

######################################################################
# Linker options
######################################################################

LINKERlinux24_amd64 = /usr/bin/ld
LINKERsun8 = /usr/ccs/bin/ld
LINKER = $(LINKER$(ISP))

LFLAGSlinux24_amd64 = -shared
LFLAGSsun8 = -G -library=iostream -xarch=v9
LFLAGS = $(LFLAGS$(ISP))

######################################################################
# Program files
######################################################################

SOLlinux24_amd64 = so
SOLsun8 = so
SOL = $(SOL$(ISP))

LIBSOlinux24_amd64 = ../linux24_amd64/$(PROG).$(SOL)
LIBSOsun8 = ../sun8/$(PROG).$(SOL)
LIBSO = $(LIBSO$(ISP))

OBJSlinux24_amd64 = linux24_amd64/TNO_NetList.o \
                    linux24_amd64/TNO_Net.o \
                    linux24_amd64/TNO_Route.o \
                    linux24_amd64/TNO_InstPin.o \
                    linux24_amd64/TNO_SwitchBox.o \
                    linux24_amd64/TNO_StringUtils.o
OBJSsun8 = sun8/TNO_NetList.o \
           sun8/TNO_Net.o \
           sun8/TNO_Route.o \
           sun8/TNO_InstPin.o \
           sun8/TNO_SwitchBox.o \
           sun8/TNO_StringUtils.o
OBJECTS = $(OBJS$(ISP))

PROG = libTNO_NetObjects
EXE = ../$(ISP)/$(PROG).a

######################################################################
# Target rules
######################################################################

BUILD_TARGET =

all:
	make Debug
Optimized:
	make $(EXE) CFLAGS="-O2 $(CFLAGS)" BUILD_TARGET=Optimized
Debug:
	make $(EXE) CFLAGS="-g -DDEBUG $(CFLAGS)" BUILD_TARGET=Debug
Gprof:
	make $(EXE) CFLAGS="-pg -g -DDEBUG $(CFLAGS)" BUILD_TARGET=Debug
clean:
	rm -rf $(ISP)/*

.cxx.o:;
.cxx.a:;

######################################################################
# Program rules
######################################################################

ARCMDlinux24_amd64 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDsun8 = $(pC++) -xar -xarch=v9 -library=iostream -o $(ISP)/$(PROG).a $(OBJECTS) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMD = $(ARCMD$(ISP))
$(EXE): $(LIBSO)
	$(ARCMD)

$(OBJECTS): $(@F:.o=.cxx)
	$(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

$(LIBSOlinux24_amd64): $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@
$(LIBSOsun8): $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

######################################################################
# Library rules
######################################################################

../$(ISP)/libTGS_GeometricShapes.a:
	cd ../TGS_GeometricShapes ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTIO_InputOutputHandlers.a:
	cd ../TIO_InputOutputHandlers ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTC_Common.a:
	cd ../TC_Common ; $(MAKE) $(BUILD_TARGET)
