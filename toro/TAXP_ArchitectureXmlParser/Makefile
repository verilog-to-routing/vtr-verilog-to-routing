######################################################################
#
# Toro architecture XML file parser library
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
# PCCTS options
######################################################################

PCCTS = ../../pccts
PCCTSINC = -I$(PCCTS)/h -I$(ISM) -I.
PCCTSBIN = $(PCCTS)/bin.$(ISM)
PCCTSLIB = -L$(PCCTS)/$(ISM) -lh

PCCTSGRAM = TAXP_ArchitectureXmlGrammar
PCCTSSCAN = TAXP_ArchitectureXmlScanner_c
PCCTSPARSER = TAXP_ArchitectureXmlParser_c

ANTLR = $(PCCTSBIN)/antlr -o $(ISM)
ANTLRFLAGS = -CC -gh

DLG = $(PCCTSBIN)/dlg -o $(ISM)
DLGFLAGS = -CC -cl $(PCCTSSCAN)
DLGFILE = parser.dlg

ANTLRGEN = $(ISM)/$(PCCTSGRAM).cpp $(ISM)/$(PCCTSPARSER).cpp $(ISM)/$(PCCTSPARSER).h \
	$(ISM)/$(DLGFILE) $(ISM)/stdpccts.h $(ISM)/tokens.h
DLGGEN = $(ISM)/$(PCCTSSCAN).cpp $(ISM)/$(PCCTSSCAN).h

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

CFLAGSlinux_x86_64 = -w -static -fPIC -DLINUX_X86_64
CFLAGSlinux_i686 = -w -static -fPIC -DLINUX_I686
CFLAGSsun8 = -w -xarch=v9 -library=iostream,Cstd -DLP64 -DSUN8
CFLAGS = $(CFLAGS$(ISM)) $(PCCTSINC)\
	-I. \
	-I../TAS_ArchitectureSpec \
	-I../TLO_LogicalObjects \
	-I../TPO_PhysicalObjects \
	-I../TGS_GeometricShapes \
	-I../TIO_InputOutputHandlers \
	-I../TC_Common \
	-I../../pcre/SRC \
	-I../../pccts/pcctsGeneric -I../../pccts/h

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

PROG = libTAXP_ArchitectureXmlParser
EXE = ../$(ISM)/$(PROG).a

ARCMDlinux_x86_64 = $(LINKER) -r -o $(ISM)/$(PROG).a $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISM)/$(PROG).a $(EXE)
ARCMDlinux_i686 = $(LINKER) -r -o $(ISM)/$(PROG).a $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISM)/$(PROG).a $(EXE)
ARCMDsun8 = $(pC++) -xar -xarch=v9 -library=iostream -o $(ISM)/$(PROG).a $(OBJECTS) $(PCCTSOBJ) ; \
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

OBJSlinux_x86_64 = linux_x86_64/TAXP_ArchitectureXmlFile.o \
                    linux_x86_64/TAXP_ArchitectureXmlHandler.o
OBJSlinux_i686 = linux_i686/TAXP_ArchitectureXmlFile.o \
                    linux_i686/TAXP_ArchitectureXmlHandler.o
OBJSsun8 = sun8/TAXP_ArchitectureXmlFile.o \
           sun8/TAXP_ArchitectureXmlHandler.o 
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
	rm -f $(ANTLRGEN) $(DLGGEN)

.cxx.o:;
.cxx.a:;

######################################################################
# PCCTS rules
######################################################################

PCCTSOBJ = $(ISM)/$(PCCTSGRAM).o $(ISM)/$(PCCTSPARSER).o $(ISM)/$(PCCTSSCAN).o
PCCTSSRC = $(ISM)/$(PCCTSGRAM).cpp

$(PCCTSOBJ): $(ANTLRGEN) $(DLGGEN)

$(ANTLRGEN): $(PCCTSGRAM).g
	$(ANTLR) $(ANTLRFLAGS) $(PCCTSGRAM).g

$(DLGGEN): $(PCCTSGRAM).g
	$(DLG) $(DLGFLAGS) $(ISM)/$(DLGFILE)

######################################################################
# Program rules
######################################################################

$(EXE): $(LIBSO)
	$(ARCMD)

$(LIBSOlinux_x86_64): $(PCCTSOBJ) $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISM)/$(PROG).$(SOL) $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISM)/$(PROG).$(SOL) $@
$(LIBSOlinux_i686): $(PCCTSOBJ) $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISM)/$(PROG).$(SOL) $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISM)/$(PROG).$(SOL) $@
$(LIBSOsun8): $(PCCTSOBJ) $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISM)/$(PROG).$(SOL) $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISM)/$(PROG).$(SOL) $@

$(PCCTSOBJ):
	$(pC++) -o $@ -c $(ISM)/$(@F:.o=.cpp) $(PIC) $(CFLAGS)

$(OBJECTS): $(ISM)/%.o:%.cxx
	$(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

######################################################################
# Library rules
######################################################################

../$(ISM)/libTAS_ArchitectureSpec.a:
	cd ../TAS_ArchitectureSpec ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTLO_LogicalObjects.a:
	cd ../TLO_LogicalObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTPO_PhysicalObjects.a:
	cd ../TPO_PhysicalObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTGS_GeometricShapes.a:
	cd ../TGS_GeometricShapes ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTIO_InputOutputHandlers.a:
	cd ../TIO_InputOutputHandlers ; $(MAKE) $(BUILD_TARGET)

../$(ISM)/libTC_Common.a:
	cd ../TC_Common ; $(MAKE) $(BUILD_TARGET)
