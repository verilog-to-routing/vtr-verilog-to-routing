######################################################################

MAKEFILE = Makefile
MAKE = /usr/atria/bin/clearmake

######################################################################
# Compiler options
######################################################################

pC++sun8 = /apps/SUNWspro_6.0-U2/bin/CC
pC++linux24_amd64 = /usr/bin/g++
pC++ = $(pC++$(ISP))

PICsun8 = -G -KPIC
PIClinux24_amd64 = -shared
PIC = $(PIC$(ISP))

CFLAGSsun8 = +w -xarch=v9 -library=iostream,Cstd -DLP64 -DSUN8
# ??? CFLAGSlinux24_amd64 = -W -Wno-deprecated -static -fPIC -DLINUX24_64
CFLAGSlinux24_amd64 = -Wall -Wpointer-arith -Wcast-qual -Wno-deprecated -static -fPIC -DLINUX24_64
CFLAGS = $(CFLAGS$(ISP)) \
	-I. \
	-I../TM_Master \
	-I../TI_Input \
	-I../TP_Process \
	-I../TO_Output \
	-I../TVPR_Interface \
	-I../TCL_CommandLine \
        -I../TOP_OptionsParser -I../TOP_OptionsParser/$(ISP) \
        -I../TAP_ArchitectureParser -I../TAP_ArchitectureParser/$(ISP) \
        -I../TAXP_ArchitectureXmlParser -I../TAXP_ArchitectureXmlParser/$(ISP) \
        -I../TFP_FabricParser -I../TFP_FabaricParser/$(ISP) \
        -I../TCP_CircuitParser -I../TCP_CircuitParser/$(ISP) \
        -I../TCBP_CircuitBlifParser -I../TCBP_CircuitBlifParser/$(ISP) \
        -I../TOS_OptionsStore \
        -I../TFS_FloorplanStore \
        -I../TAS_ArchitectureSpec \
        -I../TFM_FabricModel \
        -I../TFV_FabricView \
        -I../TCD_CircuitDesign \
        -I../TPO_PhysicalObjects \
        -I../TLO_LogicalObjects \
        -I../TNO_NetObjects \
	-I../TGS_GeometricShapes \
	-I../TIO_InputOutputHandlers \
	-I../TC_Common \
	-I../pccts/pcctsGeneric -I../pccts/h \
	-I../pcre \
	-I../vpr

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

LIBSOsun8 = ../sun8/$(PROG).$(SOL)
LIBSOlinux24_amd64 = ../linux24_amd64/$(PROG).$(SOL)
LIBSO = $(LIBSO$(ISP))

PROG = toro
EXE = ../bin.$(ISP)/$(PROG)

OBJSsun8 = sun8/toro.o 
OBJSlinux24_amd64 = linux24_amd64/toro.o 
OBJECTS = $(OBJS$(ISP))

######################################################################
# Target rules
######################################################################

# ??? .cxx.o:;
# ??? .cxx.a:;

SYSLIBSsun8 = -L../pcre/libs/$(ISP) -lpcre -Bdynamic -lresolv
SYSLIBSlinux24_amd64 = -L../pcre/libs/$(ISP) -lpcre
SYSLIBS = $(SYSLIBS$(ISP))
LIBDEFS = ../$(ISP)/libTM_Master.a \
	../$(ISP)/libTI_Input.a \
	../$(ISP)/libTP_Process.a \
	../$(ISP)/libTO_Output.a \
	../$(ISP)/libTVPR_Interface.a \
	../$(ISP)/libTCL_CommandLine.a \
	../$(ISP)/libTOP_OptionsParser.a \
	../$(ISP)/libTAP_ArchitectureParser.a \
	../$(ISP)/libTAXP_ArchitectureXmlParser.a \
	../$(ISP)/libTFP_FabricParser.a \
	../$(ISP)/libTCP_CircuitParser.a \
	../$(ISP)/libTCBP_CircuitBlifParser.a \
	../$(ISP)/libTOS_OptionsStore.a \
	../$(ISP)/libTFS_FloorplanStore.a \
	../$(ISP)/libTAS_ArchitectureSpec.a \
	../$(ISP)/libTFM_FabricModel.a \
	../$(ISP)/libTFV_FabricView.a \
	../$(ISP)/libTCD_CircuitDesign.a \
	../$(ISP)/libTLO_LogicalObjects.a \
	../$(ISP)/libTPO_PhysicalObjects.a \
	../$(ISP)/libTNO_NetObjects.a \
	../$(ISP)/libTTP_TilePlaneObjects.a \
	../$(ISP)/libTGS_GeometricShapes.a \
	../$(ISP)/libTIO_InputOutputHandlers.a \
	../$(ISP)/libTC_Common.a \
	../pccts/libs/$(ISP)/libpcctsGeneric.a \
	../pccts/libs/$(ISP)/libh.a \
	../vpr/libs/$(ISP)/libvpr.a

LINKLIBS = -L../$(ISP) \
        -L../pccts/libs/$(ISP) -L../vpr/libs/$(ISP) \
	-lTM_Master \
	-lTI_Input \
	-lTP_Process \
	-lTO_Output \
	-lTVPR_Interface \
	-lTCL_CommandLine \
	-lTOP_OptionsParser \
	-lTAP_ArchitectureParser \
	-lTAXP_ArchitectureXmlParser \
	-lTFP_FabricParser \
	-lTCP_CircuitParser \
	-lTCBP_CircuitBlifParser \
	-lTOS_OptionsStore \
	-lTFS_FloorplanStore \
	-lTAS_ArchitectureSpec \
	-lTFM_FabricModel \
	-lTFV_FabricView \
	-lTCD_CircuitDesign \
	-lTLO_LogicalObjects \
	-lTPO_PhysicalObjects \
	-lTNO_NetObjects \
	-lTTP_TilePlaneObjects \
	-lTGS_GeometricShapes \
	-lTIO_InputOutputHandlers \
	-lTC_Common \
	-lpcctsGeneric \
	-lh \
	-lvpr

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
	$(MAKE) $(EXE) CFLAGS="-pg -g -DDEBUG $(CFLAGS)" BUILD_TARGET=Gprof
Pure:
	$(MAKE) $(EXE) CFLAGS="-g $(CFLAGS)" PURE=purify
Quantify:
	$(MAKE) $(EXE) CFLAGS="-g $(CFLAGS)" PURE=quantify

clean:
	rm -rf $(ISP)/*
	rm -f *.lis *.lst purify.log core 
	rm -f $(ANTLRGEN) $(DLGGEN)

######################################################################
# Program rules
######################################################################

date.h: toro.cxx
	rm -f date.h
	echo "#define COMPILEDATE "\"`date`\""" > date.h

../bin.sun8/$(PROG): date.h $(MAKEFILE) $(OBJECTS) $(LIBDEFS)
	$(PURE) $(pC++) $(CFLAGS) -Bstatic -o $@ $(OBJECTS) \
	$(LINKLIBS) \
	-Bstatic $(SYSLIBS) -Bdynamic

../bin.linux24_amd64/$(PROG): date.h $(MAKEFILE) $(OBJECTS) $(LIBDEFS)
	$(PURE) $(pC++) $(CFLAGS) -o $@ $(OBJECTS) \
	$(LINKLIBS) \
	$(SYSLIBS) 

$(OBJECTS): $(@F:.o=.cxx)
	$(PURE) $(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

$(LIBSOsun8): $(OBJECTS) $(MWBUILDLIBS) $(MWPBLIBS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) \
	-Bstatic $(SYSLIBS) -Bdynamic ; \
	mv $(ISP)/$(PROG).$(SOL) $@

$(LIBSOlinux24_amd64): $(OBJECTS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) \
	$(SYSLIBS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

######################################################################
# Library rules
######################################################################

../$(ISP)/libTM_Master.a:
	cd ../TM_Master ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTI_Input.a:
	cd ../TI_Input ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTP_Process.a:
	cd ../TP_Process ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTO_Output.a:
	cd ../TO_Output ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTVPR_Interface.a:
	cd ../TVPR_Interface ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTCL_CommandLine.a:
	cd ../TCL_CommandLine ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTOP_OptionsParser.a:
	cd ../TOP_OptionsParser ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTAP_ArchitectureParser.a:
	cd ../TAP_ArchitectureParser ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTAXP_ArchitectureXmlParser.a:
	cd ../TAXP_ArchitectureXmlParser ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTFP_FabricParser.a:
	cd ../TFP_FabricParser ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTCP_CircuitParser.a:
	cd ../TCP_CircuitParser ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTCBP_CircuitBlifParser.a:
	cd ../TCBP_CircuitBlifParser ; $(MAKE) $(BUILD_TARGET)

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

../$(ISP)/libTTP_TilePlaneObjects.a:
	cd ../TTP_TilePlaneObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTGS_GeometricShapes.a:
	cd ../TGS_GeometricShapes ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTIO_InputOutputHandlers.a:
	cd ../TIO_InputOutputHandlers ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTC_Common.a:
	cd ../TC_Common ; $(MAKE) $(BUILD_TARGET)
