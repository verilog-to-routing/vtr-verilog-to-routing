######################################################################
#
# Toro architecture spec library
#
# Note: This make file is dependent on the platform-specific ISP 
#       environment variable.  ISP is platform-specific and is used 
#       to determine various build switches and directories.
#
#       This make file currently supports:
#       - setenv ISP linux_x86_64
#       - setenv ISP linux_i686
#       - setenv ISP sun8
#
######################################################################

######################################################################
# Compiler options
######################################################################

pC++linux_x86_64 = /usr/bin/gcc
pC++linux_i686 = /usr/bin/gcc
pC++sun8 = /apps/SUNWspro_6.0-U2/bin/CC
pC++ = $(pC++$(ISP))

PIClinux_x86_64 = -shared
PIClinux_i686 = -shared
PICsun8 = -G -KPIC
PIC = $(PIC$(ISP))

CFLAGSlinux_x86_64 = -Wall -Wpointer-arith -Wcast-qual -static -fPIC -DLINUX_X86_64
CFLAGSlinux_i686 = -Wall -Wpointer-arith -Wcast-qual -static -fPIC -DLINUX_I686
CFLAGSsun8 = +w -xarch=v9 -library=iostream,Cstd -DLP64 -DSUN8
CFLAGS = $(CFLAGS$(ISP)) \
	-I. \
	-I../TLO_LogicalObjects \
	-I../TPO_PhysicalObjects \
	-I../TGS_GeometricShapes \
	-I../TIO_InputOutputHandlers \
	-I../TC_Common \
	-I../../pcre/SRC

######################################################################
# Linker options
######################################################################

LINKERlinux_x86_64 = /usr/bin/ld
LINKERlinux_i686 = /usr/bin/ld
LINKERsun8 = /usr/ccs/bin/ld
LINKER = $(LINKER$(ISP))

LFLAGSlinux_x86_64 = -shared
LFLAGSlinux_i686 = -shared
LFLAGSsun8 = -G -library=iostream -xarch=v9
LFLAGS = $(LFLAGS$(ISP))

######################################################################
# Program files
######################################################################

SOLlinux_x86_64 = so
SOLlinux_i686 = so
SOLsun8 = so
SOL = $(SOL$(ISP))

LIBSOlinux_x86_64 = ../linux_x86_64/$(PROG).$(SOL)
LIBSOlinux_i686 = ../linux_i686/$(PROG).$(SOL)
LIBSOsun8 = ../sun8/$(PROG).$(SOL)
LIBSO = $(LIBSO$(ISP))

OBJSlinux_x86_64 = linux_x86_64/TAS_ArchitectureSpec.o \
                    linux_x86_64/TAS_Config.o \
                    linux_x86_64/TAS_PhysicalBlock.o \
                    linux_x86_64/TAS_Cell.o \
                    linux_x86_64/TAS_SwitchBox.o \
                    linux_x86_64/TAS_Segment.o \
                    linux_x86_64/TAS_Mode.o \
                    linux_x86_64/TAS_ConnectionFc.o \
                    linux_x86_64/TAS_Interconnect.o \
                    linux_x86_64/TAS_PinAssign.o \
                    linux_x86_64/TAS_GridAssign.o \
                    linux_x86_64/TAS_TimingDelay.o \
                    linux_x86_64/TAS_TimingDelayLists.o \
                    linux_x86_64/TAS_ChannelWidth.o \
                    linux_x86_64/TAS_StringUtils.o
OBJSlinux_i686 = linux_i686/TAS_ArchitectureSpec.o \
                    linux_i686/TAS_Config.o \
                    linux_i686/TAS_PhysicalBlock.o \
                    linux_i686/TAS_Cell.o \
                    linux_i686/TAS_SwitchBox.o \
                    linux_i686/TAS_Segment.o \
                    linux_i686/TAS_Mode.o \
                    linux_i686/TAS_ConnectionFc.o \
                    linux_i686/TAS_Interconnect.o \
                    linux_i686/TAS_PinAssign.o \
                    linux_i686/TAS_GridAssign.o \
                    linux_i686/TAS_TimingDelay.o \
                    linux_i686/TAS_TimingDelayLists.o \
                    linux_i686/TAS_ChannelWidth.o \
                    linux_i686/TAS_StringUtils.o
OBJSsun8 = sun8/TAS_ArchitectureSpec.o \
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
OBJECTS = $(OBJS$(ISP))

PROG = libTAS_ArchitectureSpec
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

ARCMDlinux_x86_64 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDlinux_i686 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDsun8 = $(pC++) -xar -xarch=v9 -library=iostream -o $(ISP)/$(PROG).a $(OBJECTS) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMD = $(ARCMD$(ISP))
$(EXE): $(LIBSO)
	$(ARCMD)

$(OBJECTS): $(@F:.o=.cxx)
	$(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

$(LIBSOlinux_x86_64): $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@
$(LIBSOlinux_i686): $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@
$(LIBSOsun8): $(OBJECTS) Makefile
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
