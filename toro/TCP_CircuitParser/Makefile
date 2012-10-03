######################################################################
#
# Toro circuit file parser library
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
# PCCTS options
######################################################################

PCCTS = ../../pccts
PCCTSINC = -I$(PCCTS)/h -I$(ISP) -I.
PCCTSBIN = $(PCCTS)/bin.$(ISP)
PCCTSLIB = -L$(PCCTS)/$(ISP) -lh

PCCTSGRAM = TCP_CircuitGrammar
PCCTSSCAN = TCP_CircuitScanner_c
PCCTSPARSER = TCP_CircuitParser_c

ANTLR = $(PCCTSBIN)/antlr -o $(ISP)
ANTLRFLAGS = -CC -gh

DLG = $(PCCTSBIN)/dlg -o $(ISP)
DLGFLAGS = -CC -cl $(PCCTSSCAN)
DLGFILE = parser.dlg

ANTLRGEN = $(ISP)/$(PCCTSGRAM).cpp $(ISP)/$(PCCTSPARSER).cpp $(ISP)/$(PCCTSPARSER).h \
	$(ISP)/$(DLGFILE) $(ISP)/stdpccts.h $(ISP)/tokens.h
DLGGEN = $(ISP)/$(PCCTSSCAN).cpp $(ISP)/$(PCCTSSCAN).h

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

CFLAGSlinux_x86_64 = -w -static -fPIC -DLINUX_X86_64
CFLAGSlinux_i686 = -w -static -fPIC -DLINUX_I686
CFLAGSsun8 = -w -xarch=v9 -library=iostream,Cstd -DLP64 -DSUN8
CFLAGS = $(CFLAGS$(ISP)) $(PCCTSINC)\
	-I. \
	-I../TCD_CircuitDesign \
	-I../TLO_LogicalObjects \
	-I../TPO_PhysicalObjects \
	-I../TNO_NetObjects \
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

OBJSlinux_x86_64 = linux_x86_64/TCP_CircuitFile.o \
                    linux_x86_64/TCP_CircuitHandler.o
OBJSlinux_i686 = linux_i686/TCP_CircuitFile.o \
                    linux_i686/TCP_CircuitHandler.o
OBJSsun8 = sun8/TCP_CircuitFile.o \
           sun8/TCP_CircuitHandler.o 
OBJECTS = $(OBJS$(ISP))

PROG = libTCP_CircuitParser
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
	rm -f $(ANTLRGEN) $(DLGGEN)

.cxx.o:;
.cxx.a:;

######################################################################
# PCCTS rules
######################################################################

PCCTSOBJ = $(ISP)/$(PCCTSGRAM).o $(ISP)/$(PCCTSPARSER).o $(ISP)/$(PCCTSSCAN).o
PCCTSSRC = $(ISP)/$(PCCTSGRAM).cpp

$(PCCTSOBJ): $(ANTLRGEN) $(DLGGEN)

$(ANTLRGEN): $(PCCTSGRAM).g
	$(ANTLR) $(ANTLRFLAGS) $(PCCTSGRAM).g

$(DLGGEN): $(PCCTSGRAM).g
	$(DLG) $(DLGFLAGS) $(ISP)/$(DLGFILE)

######################################################################
# Program rules
######################################################################

ARCMDlinux_x86_64 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDlinux_i686 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDsun8 = $(pC++) -xar -xarch=v9 -library=iostream -o $(ISP)/$(PROG).a $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMD = $(ARCMD$(ISP))
$(EXE): $(LIBSO)
	$(ARCMD)

$(PCCTSOBJ):
	$(pC++) -o $@ -c $(ISP)/$(@F:.o=.cpp) $(PIC) $(CFLAGS)
$(OBJECTS): $(@F:.o=.cxx)
	$(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

$(LIBSOlinux_x86_64): $(PCCTSOBJ) $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISP)/$(PROG).$(SOL) $@
$(LIBSOlinux_i686): $(PCCTSOBJ) $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISP)/$(PROG).$(SOL) $@
$(LIBSOsun8): $(PCCTSOBJ) $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

######################################################################
# Library rules
######################################################################

../$(ISP)/libTCD_CircuitDesign.a:
	cd ../TCD_CircuitDesign ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTLO_LogicalObjects.a:
	cd ../TLO_LogicalObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTPO_PhysicalObjects.a:
	cd ../TPO_PhysicalObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTNO_NetObjects.a:
	cd ../TNO_NetObjects ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTGS_GeometricShapes.a:
	cd ../TGS_GeometricShapes ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTIO_InputOutputHandlers.a:
	cd ../TIO_InputOutputHandlers ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTC_Common.a:
	cd ../TC_Common ; $(MAKE) $(BUILD_TARGET)
