######################################################################

MAKEFILE = Makefile
MAKE = /usr/atria/bin/clearmake

######################################################################
# PCCTS options
######################################################################

PCCTS = ../pccts
PCCTSINC = -I$(PCCTS)/h -I$(ISP) -I.
PCCTSBIN = $(PCCTS)/bin.$(ISP)
PCCTSLIB = -L$(PCCTS)/$(ISP) -lh

PCCTSGRAM = TFP_FabricGrammar
PCCTSSCAN = TFP_FabricScanner_c
PCCTSPARSER = TFP_FabricParser_c

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

pC++sun8 = /apps/SUNWspro_6.0-U2/bin/CC
pC++linux24_amd64 = /usr/bin/gcc
pC++ = $(pC++$(ISP))

PICsun8 = -G -KPIC
PIClinux24_amd64 = -shared
PIC = $(PIC$(ISP))

CFLAGSsun8 = +w -xarch=v9 -library=iostream,Cstd -DLP64 -DSUN8
CFLAGSlinux24_amd64 = -W -Wno-deprecated -static -fPIC -DLINUX24_64
CFLAGS = $(CFLAGS$(ISP)) $(PCCTSINC)\
	-I. \
	-I../TFM_FabricModel \
	-I../TFV_FabricView \
	-I../TLO_LogicalObjects \
	-I../TPO_PhysicalObjects \
	-I../TNO_NetObjects \
	-I../TTP_TilePlaneObjects \
	-I../TGS_GeometricShapes \
	-I../TIO_InputOutputHandlers \
	-I../TC_Common \
	-I../pcre \
	-I../pccts/pcctsGeneric -I../pccts/h

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

PROG = libTFP_FabricParser
EXE = ../$(ISP)/$(PROG).a

OBJSsun8	= sun8/TFP_FabricFile.o \
                  sun8/TFP_FabricHandler.o 
OBJSlinux24_amd64	= linux24_amd64/TFP_FabricFile.o \
                          linux24_amd64/TFP_FabricHandler.o

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
	rm -f $(ANTLRGEN) $(DLGGEN)

######################################################################
# PCCTS rules
######################################################################

PCCTSOBJ = $(ISP)/$(PCCTSGRAM).o $(ISP)/$(PCCTSPARSER).o $(ISP)/$(PCCTSSCAN).o
PCCTSSRC = $(ISP)/$(PCCTSGRAM).cpp

$(PCCTSOBJ):	$(ANTLRGEN) $(DLGGEN)

$(ANTLRGEN):	$(PCCTSGRAM).g
	$(ANTLR) $(ANTLRFLAGS) $(PCCTSGRAM).g

$(DLGGEN):	$(PCCTSGRAM).g
	$(DLG) $(DLGFLAGS) $(ISP)/$(DLGFILE)

######################################################################
# Program rules
######################################################################

ARCMDsun8 = $(pC++) -xar -xarch=v9 -library=iostream -o $(ISP)/$(PROG).a $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDlinux24_amd64 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMD = $(ARCMD$(ISP))
$(EXE): $(LIBSO)
	$(ARCMD)

$(PCCTSOBJ):
	$(PURE) $(pC++) -o $@ -c $(ISP)/$(@F:.o=.cpp) $(PIC) $(CFLAGS)
$(OBJECTS): $(@F:.o=.cxx)
	$(PURE) $(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

$(LIBSOsun8): $(PCCTSOBJ) $(OBJECTS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

$(LIBSOlinux24_amd64): $(PCCTSOBJ) $(OBJECTS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

######################################################################
# Library rules
######################################################################

../$(ISP)/libTFM_FabricModel.a:
	cd ../TFM_FabricModel ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTFV_FabricView.a:
	cd ../TFV_FabricView ; $(MAKE) $(BUILD_TARGET)

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
