######################################################################
#
# Toro - a VPR front-end
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

pC++linux_x86_64 = /usr/bin/g++
pC++linux_i686 = /usr/bin/g++
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
	-I../TM_Master \
	-I../TI_Input \
	-I../TP_Process \
	-I../TO_Output \
	-I../TVPR_Interface \
	-I../TCL_CommandLine \
        -I../TOP_OptionsParser -I../TOP_OptionsParser/$(ISM) \
        -I../TAP_ArchitectureParser -I../TAP_ArchitectureParser/$(ISM) \
        -I../TAXP_ArchitectureXmlParser -I../TAXP_ArchitectureXmlParser/$(ISM) \
        -I../TFP_FabricParser -I../TFP_FabaricParser/$(ISM) \
        -I../TCP_CircuitParser -I../TCP_CircuitParser/$(ISM) \
        -I../TCBP_CircuitBlifParser -I../TCBP_CircuitBlifParser/$(ISM) \
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
EXE = ../bin.$(ISM)/$(PROG)

OBJSlinux_x86_64 = linux_x86_64/toro.o 
OBJSlinux_i686 = linux_i686/toro.o 
OBJSsun8 = sun8/toro.o 
OBJECTS = $(OBJS$(ISM))

######################################################################
# Target rules
######################################################################

LINKLIBS = -L../$(ISM) \
        -L../../pccts/$(ISM) -L../../vpr/$(ISM) \
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

SYSLIBSlinux_x86_64 = -L../../pcre/$(ISM) -lpcre
SYSLIBSlinux_i686 = -L../../pcre/$(ISM) -lpcre
SYSLIBSsun8 = -L../../pcre/$(ISM) -lpcre -Bdynamic -lresolv
SYSLIBS = $(SYSLIBS$(ISM))
LIBDEFS = ../$(ISM)/libTM_Master.a \
	../$(ISM)/libTI_Input.a \
	../$(ISM)/libTP_Process.a \
	../$(ISM)/libTO_Output.a \
	../$(ISM)/libTVPR_Interface.a \
	../$(ISM)/libTCL_CommandLine.a \
	../$(ISM)/libTOP_OptionsParser.a \
	../$(ISM)/libTAP_ArchitectureParser.a \
	../$(ISM)/libTAXP_ArchitectureXmlParser.a \
	../$(ISM)/libTFP_FabricParser.a \
	../$(ISM)/libTCP_CircuitParser.a \
	../$(ISM)/libTCBP_CircuitBlifParser.a \
	../$(ISM)/libTOS_OptionsStore.a \
	../$(ISM)/libTFS_FloorplanStore.a \
	../$(ISM)/libTAS_ArchitectureSpec.a \
	../$(ISM)/libTFM_FabricModel.a \
	../$(ISM)/libTFV_FabricView.a \
	../$(ISM)/libTCD_CircuitDesign.a \
	../$(ISM)/libTLO_LogicalObjects.a \
	../$(ISM)/libTPO_PhysicalObjects.a \
	../$(ISM)/libTNO_NetObjects.a \
	../$(ISM)/libTTP_TilePlaneObjects.a \
	../$(ISM)/libTGS_GeometricShapes.a \
	../$(ISM)/libTIO_InputOutputHandlers.a \
	../$(ISM)/libTC_Common.a \
	../../pccts/$(ISM)/libpcctsGeneric.a \
	../../pccts/$(ISM)/libh.a \
	../../vpr/$(ISM)/libvpr.a

.PHONY: $(LIBDEFS)

BUILD_TARGET =

all:
	make Debug
Optimized:
	make $(EXE) BUILD_TARGET=Optimized
Debug:
	make $(EXE) BUILD_TARGET=Debug
clean:
	rm -rf $(ISM)/*
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
	$(pC++) $(CFLAGS) -o $@ $(OBJECTS) $(LINKLIBS) $(SYSLIBS) 
../bin.linux_i686/$(PROG): date.h Makefile $(OBJECTS) $(LIBDEFS)
	$(pC++) $(CFLAGS) -o $@ $(OBJECTS) $(LINKLIBS) $(SYSLIBS) 
../bin.sun8/$(PROG): date.h Makefile $(OBJECTS) $(LIBDEFS)
	$(pC++) $(CFLAGS) -Bstatic -o $@ $(OBJECTS) $(LINKLIBS) -Bstatic $(SYSLIBS) -Bdynamic

$(OBJECTS): $(ISM)/%.o:%.cxx
	$(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

######################################################################
# Library rules
######################################################################

../$(ISM)/libTM_Master.a:
	cd ../TM_Master && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTI_Input.a:
	cd ../TI_Input && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTP_Process.a:
	cd ../TP_Process && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTO_Output.a:
	cd ../TO_Output && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTVPR_Interface.a:
	cd ../TVPR_Interface && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTCL_CommandLine.a:
	cd ../TCL_CommandLine && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTOP_OptionsParser.a:
	cd ../TOP_OptionsParser && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTAP_ArchitectureParser.a:
	cd ../TAP_ArchitectureParser && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTAXP_ArchitectureXmlParser.a:
	cd ../TAXP_ArchitectureXmlParser && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTFP_FabricParser.a:
	cd ../TFP_FabricParser && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTCP_CircuitParser.a:
	cd ../TCP_CircuitParser && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTCBP_CircuitBlifParser.a:
	cd ../TCBP_CircuitBlifParser && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTOS_OptionsStore.a:
	cd ../TOS_OptionsStore && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTFS_FloorplanStore.a:
	cd ../TFS_FloorplanStore && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTAS_ArchitectureSpec.a:
	cd ../TAS_ArchitectureSpec && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTFM_FabricModel.a:
	cd ../TFM_FabricModel && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTFV_FabricView.a:
	cd ../TFV_FabricView && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTCD_CircuitDesign.a:
	cd ../TCD_CircuitDesign && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTLO_LogicalObjects.a:
	cd ../TLO_LogicalObjects && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTPO_PhysicalObjects.a:
	cd ../TPO_PhysicalObjects && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTNO_NetObjects.a:
	cd ../TNO_NetObjects && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTTP_TilePlaneObjects.a:
	cd ../TTP_TilePlaneObjects && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTGS_GeometricShapes.a:
	cd ../TGS_GeometricShapes && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTIO_InputOutputHandlers.a:
	cd ../TIO_InputOutputHandlers && $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTC_Common.a:
	cd ../TC_Common && $(MAKE) $(BUILD_TARGET)
