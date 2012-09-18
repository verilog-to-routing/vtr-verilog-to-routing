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
# CFLAGSlinux24_amd64 = -W -Wno-deprecated -static -fPIC -DLINUX24_64
CFLAGSlinux24_amd64 = -Wall -Wpointer-arith -Wcast-qual -Wno-deprecated -static -fPIC -DLINUX24_64
CFLAGS = $(CFLAGS$(ISP)) \
	-I. \
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
	-I../TIO_InputOutputHandlers \
	-I../TC_Common \
	-I../pcre \
        -I../vpr/SRC/base -I../vpr/SRC/util -I../libarchfpga/include

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

PROG = libTVPR_Interface
EXE = ../$(ISP)/$(PROG).a

OBJSsun8	= sun8/TVPR_Interface.o \
                  sun8/TVPR_OptionsStore.o \
                  sun8/TVPR_ArchitectureSpec.o \
                  sun8/TVPR_FabricModel.o \
                  sun8/TVPR_CircuitDesign.o
OBJSlinux24_amd64	= linux24_amd64/TVPR_Interface.o \
                          linux24_amd64/TVPR_OptionsStore.o \
                          linux24_amd64/TVPR_ArchitectureSpec.o \
                          linux24_amd64/TVPR_FabricModel.o \
                          linux24_amd64/TVPR_CircuitDesign.o

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

../$(ISP)/libTOS_OptionsStore.a:
	cd ../TOS_OptionsStore ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTFS_FloorplanStore.a:
	cd ../TFS_FloorplanStore ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTAS_ArchitectureSpec.a:
	cd ../TAS_ArchitectureSpec ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTFM_FabricModel.a:
	cd ../TFM_FabricModel ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTFV_FabricView.a:
	cd ../TFV_FabricView ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTCD_CircuitDesign.a:
	cd ../TCD_CircuitDesign ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTLO_LogicalObjects.a:
	cd ../TLO_LogicalObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTPO_PhysicalObjects.a:
	cd ../TPO_PhysicalObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTNO_NetObjects.a:
	cd ../TNO_NetObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTTPT_TilePlaneObjects.a:
	cd ../TTP_TilePlaneObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTGS_GeometricShapes.a:
	cd ../TGS_GeometricShapes ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTIO_InputOutputHandlers.a:
	cd ../TIO_InputOutputHandlers ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTC_Common.a:
	cd ../TC_Common ; $(MAKE) $(BUILD_TARGET)
