######################################################################
#
# Toro I-P-O output object library
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
	-I. \
	-I../TCL_CommandLine \
	-I../TOS_OptionsStore \
	-I../TFS_FloorplanStore \
	-I../TAS_ArchitectureSpec \
	-I../TFM_FabricModel \
	-I../TFV_FabricView \
	-I../TCD_CircuitDesign \
	-I../TLO_LogicalObjects \
	-I../TPO_PhysicalObjects \
	-I../TNO_NetObjects \
	-I../TTP_TilePlaneObjects \
	-I../TGS_GeometricShapes \
	-I../TGO_GeometricObjects \
	-I../TIO_InputOutputHandlers \
	-I../TC_Common \
	-I../../libcommon_c/pcre/SRC

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

PROG = libTO_Output
EXE = ../$(ISM)/$(PROG).a

ARCMDlinux_x86_64 = $(LINKER) -r -o $(ISM)/$(PROG).a $(OBJECTS) ; \
	mv $(ISM)/$(PROG).a $(EXE)
ARCMDlinux_i686 = $(LINKER) -r -o $(ISM)/$(PROG).a $(OBJECTS) ; \
	mv $(ISM)/$(PROG).a $(EXE)
ARCMDsun8 = $(pC++) -xar -xarch=v9 -library=iostream -o $(ISM)/$(PROG).a $(OBJECTS) ; \
	mv $(ISM)/$(PROG).a $(EXE)
ARCMD = $(ARCMD$(ISM))

SOLlinux_x86_64 = so
SOLlinux_i686 = so
SOLsun8 = so
SOL = $(SOL$(ISM))

LIBSOlinux_x86_64 = ../linux_x86_64/$(PROG).$(SOL)
LIBSOlinux_i686 = ../linux_i686/$(PROG).$(SOL)
LIBSOsun8 = ../sun8/$(PROG).$(SOL)
LIBSO = $(LIBSO$(ISM))

OBJSlinux_x86_64 = linux_x86_64/TO_Output.o \
                    linux_x86_64/TO_OutputWriteLaff.o \
                    linux_x86_64/TO_OutputWriteOptions.o \
                    linux_x86_64/TO_OutputWriteXml.o \
                    linux_x86_64/TO_OutputWriteBlif.o \
                    linux_x86_64/TO_OutputWriteArchitecture.o \
                    linux_x86_64/TO_OutputWriteFabric.o \
                    linux_x86_64/TO_OutputWriteCircuit.o \
                    linux_x86_64/TO_OutputEmailMetrics.o
OBJSlinux_i686 = linux_i686/TO_Output.o \
                    linux_i686/TO_OutputWriteLaff.o \
                    linux_i686/TO_OutputWriteOptions.o \
                    linux_i686/TO_OutputWriteXml.o \
                    linux_i686/TO_OutputWriteBlif.o \
                    linux_i686/TO_OutputWriteArchitecture.o \
                    linux_i686/TO_OutputWriteFabric.o \
                    linux_i686/TO_OutputWriteCircuit.o \
                    linux_i686/TO_OutputEmailMetrics.o
OBJSsun8 = sun8/TO_Output.o \
           sun8/TO_OutputWriteOptions.o \
           sun8/TO_OutputWriteXml.o \
           sun8/TO_OutputWriteBlif.o \
           sun8/TO_OutputWriteArchitecture.o \
           sun8/TO_OutputWriteFabric.o \
           sun8/TO_OutputWriteCircuit.o \
           sun8/TO_OutputWriteLaff.o \
           sun8/TO_OutputEmailMetrics.o
OBJECTS = $(OBJS$(ISM))

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

../$(ISM)/libTCL_CommandLine.a:
	cd ../TCL_CommandLine ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTOS_OptionsStore.a:
	cd ../TOS_OptionsStore ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTFS_FloorplanStore.a:
	cd ../TFS_FloorplanStore ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTAS_ArchitectureSpec.a:
	cd ../TAS_ArchitectureSpec ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTFM_FabricModel.a:
	cd ../TFM_FabricModel ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTFV_FabricView.a:
	cd ../TFV_FabricView ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTCD_CircuitDesign.a:
	cd ../TCD_CircuitDesign ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTLO_LogicalObjects.a:
	cd ../TLO_LogicalObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTPO_PhysicalObjects.a:
	cd ../TPO_PhysicalObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTNO_NetObjects.a:
	cd ../TNO_NetObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTTPT_TilePlaneObjects.a:
	cd ../TTP_TilePlaneObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTGS_GeometricShapes.a:
	cd ../TGS_GeometricShapes ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTGO_GeometricObjects.a:
	cd ../TGO_GeometricObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTIO_InputOutputHandlers.a:
	cd ../TIO_InputOutputHandlers ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTC_Common.a:
	cd ../TC_Common ; $(MAKE) $(BUILD_TARGET)
