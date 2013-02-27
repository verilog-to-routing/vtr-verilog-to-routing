######################################################################
#
# Toro VPR interface library
#
# Note: This make file is dependent on the platform-specific ISM 
#       environment variable.  ISM is platform-specific and is used 
#       to determine various build switches and directories.
#
#       This make file currently supports:
#       - setenv ISM linux_x86_64
#       - setenv ISM linux_i686
#       - setenv ISM sun8
#
######################################################################

######################################################################
# Compiler options
######################################################################

pC++linux_x86_64 = /usr/bin/gcc
pC++linux_i686 = /usr/bin/gcc
pC++sun8 = /apps/SUNWspro_6.0-U2/bin/CC
pC++ = $(pC++$(ISM))

PIClinux_x86_64 = -shared
PIClinux_i686 = -shared
PICsun8 = -G -KPIC
PIC = $(PIC$(ISM))

CFLAGSlinux_x86_64 = -Wall -Wpointer-arith -Wcast-qual -static -fPIC -DLINUX_X86_64
CFLAGSlinux_i686 = -Wall -Wpointer-arith -Wcast-qual -static -fPIC -DLINUX_I686
CFLAGSsun8 = +w -xarch=v9 -library=iostream,Cstd -DLP64 -DSUN8
CFLAGS = $(CFLAGS$(ISM)) \
	-DTORO_FABRIC_GRID_VALID_ENABLE \
	-DTORO_FABRIC_CHANNEL_WIDTHS_ENABLE \
	-DTORO_REGION_PLACEMENT_ENABLE \
	-DTORO_RELATIVE_PLACEMENT_ENABLE \
	-DTORO_PREPLACED_PLACEMENT_ENABLE \
	-DTORO_PREROUTED_ROUTE_ENABLE \
	-I. \
	-I../TLO_LogicalObjects \
	-I../TPO_PhysicalObjects \
	-I../TNO_NetObjects \
	-I../TGS_GeometricShapes \
	-I../TGO_GeometricObjects \
	-I../TIO_InputOutputHandlers \
	-I../TC_Common \
	-I../../libpcre/SRC \
	-I../../vpr/SRC/base -I../../vpr/SRC/util -I../../libarchfpga/include

######################################################################
# Linker options
######################################################################

LINKERlinux_x86_64 = /usr/bin/ld
LINKERlinux_i686 = /usr/bin/ld
LINKERsun8 = /usr/ccs/bin/ld
LINKER = $(LINKER$(ISM))

LFLAGSlinux_x86_64 = -shared
LFLAGSlinux_i686 = -shared
LFLAGSsun8 = -G -library=iostream -xarch=v9
LFLAGS = $(LFLAGS$(ISM))

######################################################################
# Program files
######################################################################

SOLlinux_x86_64 = so
SOLlinux_i686 = so
SOLsun8 = so
SOL = $(SOL$(ISM))

LIBSOlinux_x86_64 = ../linux_x86_64/$(PROG).$(SOL)
LIBSOlinux_i686 = ../linux_i686/$(PROG).$(SOL)
LIBSOsun8 = ../sun8/$(PROG).$(SOL)
LIBSO = $(LIBSO$(ISM))

OBJSlinux_x86_64 = linux_x86_64/TCH_RelativePlaceHandler.o \
                   linux_x86_64/TCH_RelativeMacro.o \
                   linux_x86_64/TCH_RelativeNode.o \
                   linux_x86_64/TCH_RelativeBlock.o \
                   linux_x86_64/TCH_RelativeMove.o \
                   linux_x86_64/TCH_RotateMaskKey.o \
                   linux_x86_64/TCH_RotateMaskValue.o \
                   linux_x86_64/TCH_PrePlacedHandler.o \
                   linux_x86_64/TCH_PrePlacedBlock.o \
                   linux_x86_64/TCH_PreRoutedHandler.o \
                   linux_x86_64/TCH_Net.o \
                   linux_x86_64/TCH_RoutePath.o \
                   linux_x86_64/TCH_RouteNode.o \
                   linux_x86_64/TCH_VPR_GraphNode.o \
                   linux_x86_64/TCH_VPR_GraphToRoute.o \
                   linux_x86_64/TCH_VPR_Data.o \
                   linux_x86_64/TCH_StringUtils.o 
OBJSlinux_i686 = linux_i686/TCH_RelativePlaceHandler.o \
                 linux_i686/TCH_RelativeMacro.o \
                 linux_i686/TCH_RelativeNode.o \
                 linux_i686/TCH_RelativeBlock.o \
                 linux_i686/TCH_RelativeMove.o \
                 linux_i686/TCH_RotateMaskKey.o \
                 linux_i686/TCH_RotateMaskValue.o \
                 linux_i686/TCH_PrePlacedHandler.o \
                 linux_i686/TCH_PrePlacedBlock.o \
                 linux_i686/TCH_PreRoutedHandler.o \
                 linux_i686/TCH_Net.o \
                 linux_i686/TCH_RoutePath.o \
                 linux_i686/TCH_RouteNode.o \
                 linux_i686/TCH_VPR_GraphNode.o \
                 linux_i686/TCH_VPR_GraphToRoute.o \
                 linux_i686/TCH_VPR_Data.o \
                 linux_i686/TCH_StringUtils.o 
OBJSsun8 = sun8/TCH_RelativePlaceHandler.o \
           sun8/TCH_RelativeMacro.o \
           sun8/TCH_RelativeNode.o \
           sun8/TCH_RelativeBlock.o \
           sun8/TCH_RelativeMove.o \
           sun8/TCH_RotateMaskKey.o \
           sun8/TCH_RotateMaskValue.o \
           sun8/TCH_PrePlacedHandler.o \
           sun8/TCH_PrePlacedBlock.o \
           sun8/TCH_PreRoutedHandler.o \
           sun8/TCH_Net.o \
           sun8/TCH_RoutePath.o \
           sun8/TCH_RouteNode.o \
           sun8/TCH_VPR_GraphNode.o \
           sun8/TCH_VPR_GraphToRoute.o \
           sun8/TCH_VPR_Data.o \
           sun8/TCH_StringUtils.o 
OBJECTS = $(OBJS$(ISM))

PROG = libTCH_CircuitHandlers
EXE = ../$(ISM)/$(PROG).a

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
	rm -rf $(ISM)/*

.cxx.o:;
.cxx.a:;

######################################################################
# Program rules
######################################################################

ARCMDlinux_x86_64 = $(LINKER) -r -o $(ISM)/$(PROG).a $(OBJECTS) ; \
	mv $(ISM)/$(PROG).a $(EXE)
ARCMDlinux_i686 = $(LINKER) -r -o $(ISM)/$(PROG).a $(OBJECTS) ; \
	mv $(ISM)/$(PROG).a $(EXE)
ARCMDsun8 = $(pC++) -xar -xarch=v9 -library=iostream -o $(ISM)/$(PROG).a $(OBJECTS) ; \
	mv $(ISM)/$(PROG).a $(EXE)
ARCMD = $(ARCMD$(ISM))
$(EXE): $(LIBSO)
	$(ARCMD)

$(LIBSOlinux_x86_64): $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISM)/$(PROG).$(SOL) $(OBJECTS) ; \
	mv $(ISM)/$(PROG).$(SOL) $@
$(LIBSOlinux_i686): $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISM)/$(PROG).$(SOL) $(OBJECTS) ; \
	mv $(ISM)/$(PROG).$(SOL) $@
$(LIBSOsun8): $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISM)/$(PROG).$(SOL) $(OBJECTS) ; \
	mv $(ISM)/$(PROG).$(SOL) $@

$(OBJECTS): $(ISM)/%.o:%.cxx
	$(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

######################################################################
# Library rules
######################################################################

../$(ISM)/libTLO_LogicalObjects.a:
	cd ../TLO_LogicalObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTPO_PhysicalObjects.a:
	cd ../TPO_PhysicalObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTNO_NetObjects.a:
	cd ../TNO_NetObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTGS_GeometricShapes.a:
	cd ../TGS_GeometricShapes ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTGO_GeometricObjects.a:
	cd ../TGO_GeometricObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTIO_InputOutputHandlers.a:
	cd ../TIO_InputOutputHandlers ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTC_Common.a:
	cd ../TC_Common ; $(MAKE) $(BUILD_TARGET)
