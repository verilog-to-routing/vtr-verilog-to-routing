######################################################################
#
# PCCTS include library
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

SOLlinux24_amd64 = so
SOLsun8 = so
SOL = $(SOL$(ISP))

##############################
# compiler specific data
##############################

pC++linux24_amd64 = /usr/bin/gcc
pC++sun8 = /apps/SUNWspro_6.0-U2/bin/CC
pC++ = $(pC++$(ISP))

PIClinux24_amd64 = -shared
PICsun8 = -G -KPIC
PIC = $(PIC$(ISP))

CFLAGSlinux24_amd64 = -W -fPIC -m64
CFLAGSsun8 = +w -xarch=v9 -library=iostream -DLP64
CFLAGS = $(CFLAGS$(ISP)) $(USERCCDEFINES)

##############################
# linker specific data
##############################

LINKERlinux24_amd64 = /usr/bin/ld
LINKERsun8 = /usr/ccs/bin/ld
LINKER = $(LINKER$(ISP))
SLINKER = $(LINKER)

LFLAGSlinux24_amd64 = -shared
LFLAGSsun8 = -G -library=iostream -xarch=v9
LFLAGS = $(LFLAGS$(ISP))

#######################################################
# targets
#######################################################

BUILD_TARGET =

all:
	make Debug
Optimized:
	make $(EXE) CFLAGS="-O2 $(CFLAGS)" BUILD_TARGET=Optimized
Debug:
	make $(EXE) CFLAGS="-g -DDEBUG $(CFLAGS)" BUILD_TARGET=Debug
Static:
	make $(EXE) CFLAGS="-g $(CFLAGS)" PIC="" LIBSO="" BUILD_TARGET=Static GLINKsun8="$(GSLINKSUN8)"
Shared:
	make $(EXE) CFLAGS="-g $(CFLAGS)" BUILD_TARGET=Shared GLINKsun8="$(GDLINKSUN8)"
clean:
	rm -rf $(ISP)/*

##############################
# program specific files
##############################

LIBSOlinux24_amd64 = ../linux24_amd64/$(PROG).$(SOL)
LIBSOsun8 = ../sun8/$(PROG).$(SOL)
LIBSO = $(LIBSO$(ISP))

OBJSlinux24_amd64 = linux24_amd64/GenericToken.o linux24_amd64/GenericTokenBuffer.o linux24_amd64/NoLeakToken.o 
OBJSsun8 = sun8/GenericToken.o sun8/GenericTokenBuffer.o sun8/NoLeakToken.o 
OBJECTS = $(OBJS$(ISP))

PROG = libh
EXE = ../$(ISP)/$(PROG).a

##############################
# target rules
##############################

.cxx.o:;
.cxx.a:;

SYSLIBSlinux24_amd64 = 
SYSLIBSsun8 = 
SYSLIBS = $(SYSLIBS$(ISP))

##############################
# program rules
##############################

ARCMDlinux24_amd64 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) ;\
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDsun8 = $(pC++) -xar -xarch=v9 -library=iostream -o $(ISP)/$(PROG).a $(OBJECTS) ;\
	mv $(ISP)/$(PROG).a $(EXE)
ARCMD = $(ARCMD$(ISP))
$(EXE): $(LIBSO)
	$(ARCMD)

$(OBJECTS): $(@F:.o=.cxx)
	$(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

$(LIBSOlinux24_amd64): $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS)  \
	$(SYSLIBS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@
$(LIBSOsun8): $(OBJECTS) $(MWBUILDLIBS) $(MWPBLIBS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS)  \
	-Bstatic $(GDLINKSUN_8)\
	$(TIDASLNK) $(INSLIBS) $(SYSLIBS) -Bdynamic; \
	mv $(ISP)/$(PROG).$(SOL) $@
