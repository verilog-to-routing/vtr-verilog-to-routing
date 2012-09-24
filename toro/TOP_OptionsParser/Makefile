######################################################################
#
# Toro options file parser library
#
# Note: This make file is dependent on the platform-specific ISP 
#       environment variable.  ISP is platform-specific and is used 
#       to determine various build switches and directories.
#
#       This make file currently supports:
#       - setenv ISP linux24_amd64
#       - setenv ISP sun8
#
######################################################################

######################################################################
# PCCTS options
######################################################################

PCCTS = ../pccts
PCCTSINC = -I$(PCCTS)/h -I$(ISP) -I.
PCCTSBIN = $(PCCTS)/bin.$(ISP)
PCCTSLIB = -L$(PCCTS)/$(ISP) -lh

PCCTSGRAM = TOP_OptionsGrammar
PCCTSSCAN = TOP_OptionsScanner_c
PCCTSPARSER = TOP_OptionsParser_c

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

pC++linux24_amd64 = /usr/bin/gcc
pC++sun8 = /apps/SUNWspro_6.0-U2/bin/CC
pC++ = $(pC++$(ISP))

PIClinux24_amd64 = -shared
PICsun8 = -G -KPIC
PIC = $(PIC$(ISP))

CFLAGSlinux24_amd64 = -w -Wno-deprecated -static -fPIC -DLINUX24_64
CFLAGSsun8 = -w -xarch=v9 -library=iostream,Cstd -DLP64 -DSUN8
CFLAGS = $(CFLAGS$(ISP)) $(PCCTSINC)\
	-I. \
	-I../TOS_OptionsStore \
	-I../TIO_InputOutputHandlers \
	-I../TC_Common \
	-I../pcre \
	-I../pccts/pcctsGeneric -I../pccts/h

######################################################################
# Linker options
######################################################################

LINKERlinux24_amd64 = /usr/bin/ld
LINKERsun8 = /usr/ccs/bin/ld
LINKER = $(LINKER$(ISP))

LFLAGSlinux24_amd64 = -shared
LFLAGSsun8 = -G -library=iostream -xarch=v9
LFLAGS = $(LFLAGS$(ISP))

######################################################################
# Program files
######################################################################

SOLlinux24_amd64 = so
SOLsun8 = so
SOL = $(SOL$(ISP))

LIBSOlinux24_amd64 = ../linux24_amd64/$(PROG).$(SOL)
LIBSOsun8 = ../sun8/$(PROG).$(SOL)
LIBSO = $(LIBSO$(ISP))

OBJSlinux24_amd64 = linux24_amd64/TOP_OptionsFile.o \
                    linux24_amd64/TOP_OptionsHandler.o
OBJSsun8 = sun8/TOP_OptionsFile.o \
           sun8/TOP_OptionsHandler.o 
OBJECTS = $(OBJS$(ISP))

PROG = libTOP_OptionsParser
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

ARCMDlinux24_amd64 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) $(PCCTSOBJ) ; \
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

$(LIBSOlinux24_amd64): $(PCCTSOBJ) $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISP)/$(PROG).$(SOL) $@
$(LIBSOsun8): $(PCCTSOBJ) $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) $(PCCTSOBJ) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

######################################################################
# Library rules
######################################################################

../$(ISP)/libTOS_OptionsStore.a:
	cd ../TOS_OptionsStore ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTIO_InputOutputHandlers.a:
	cd ../TIO_InputOutputHandlers ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTC_Common.a:
	cd ../TC_Common ; $(MAKE) $(BUILD_TARGET)
