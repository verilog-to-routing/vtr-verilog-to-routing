######################################################################
#
# Toro - a VPR front-end
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

pC++linux_x86_64 = /usr/bin/g++
pC++linux_i686 = /usr/bin/g++
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
	-I../TTP_TilePlaneObjects \
	-I../TGS_GeometricShapes \
	-I../TIO_InputOutputHandlers \
	-I../TC_Common \
	-I../../pccts/pcctsGeneric -I../../pccts/h \
	-I../../pcre/SRC \
	-I../../vpr/SRC/base -I../../vpr/SRC/util -I../../libarchfpga/include

######################################################################
# Program files
######################################################################

PROG = toro
EXE = ../bin.$(ISP)/$(PROG)

OBJSlinux_x86_64 = linux_x86_64/toro.o 
OBJSlinux_i686 = linux_i686/toro.o 
OBJSsun8 = sun8/toro.o 
OBJECTS = $(OBJS$(ISP))

######################################################################
# Target rules
######################################################################

SYSLIBSlinux_x86_64 = -L../../pcre/$(ISP) -lpcre
SYSLIBSlinux_i686 = -L../../pcre/$(ISP) -lpcre
SYSLIBSsun8 = -L../../pcre/$(ISP) -lpcre -Bdynamic -lresolv
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
	../../pccts/$(ISP)/libpcctsGeneric.a \
	../../pccts/$(ISP)/libh.a \
	../../vpr/$(ISP)/libvpr.a

LINKLIBS = -L../$(ISP) \
        -L../../pccts/$(ISP) -L../../vpr/$(ISP) \
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

BUILD_TARGET =

all:
	make Debug
Optimized:
	make $(EXE) BUILD_TARGET=Optimized
Debug:
	make $(EXE) BUILD_TARGET=Debug
clean:
	rm -rf $(ISP)/*
	rm -f $(ANTLRGEN) $(DLGGEN)

.cxx.o:;
.cxx.a:;

######################################################################
# Program rules
######################################################################

date.h: toro.cxx
	rm -f date.h
	echo "#define COMPILEDATE "\"`date`\""" > date.h

../bin.linux_x86_64/$(PROG): date.h Makefile $(OBJECTS) $(LIBDEFS)
	$(pC++) $(CFLAGS) -o $@ $(OBJECTS) \
	$(LINKLIBS) \
	$(SYSLIBS) 
../bin.linux_i686/$(PROG): date.h Makefile $(OBJECTS) $(LIBDEFS)
	$(pC++) $(CFLAGS) -o $@ $(OBJECTS) \
	$(LINKLIBS) \
	$(SYSLIBS) 
../bin.sun8/$(PROG): date.h Makefile $(OBJECTS) $(LIBDEFS)
	$(pC++) $(CFLAGS) -Bstatic -o $@ $(OBJECTS) \
	$(LINKLIBS) \
	-Bstatic $(SYSLIBS) -Bdynamic

$(OBJECTS): $(@F:.o=.cxx)
	$(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

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
