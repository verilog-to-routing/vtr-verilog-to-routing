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

LFLAGSlinux24_amd64 = -shared
LFLAGSsun8 = -G -library=iostream -xarch=v9
LFLAGS = $(LFLAGS$(ISP))

##############################
# program specific files
##############################

SOL = $(SOL$(ISP))
SOLlinux24_amd64 = so
SOLsun8 = so

LIBSOlinux24_amd64 = ../linux24_amd64/$(PROG).$(SOL)
LIBSOsun8 = ../sun8/$(PROG).$(SOL)
LIBSO = $(LIBSO$(ISP))

LIBAAlinux24_amd64 = 
LIBAAsun8 = ../sun8/$(PROG).aa
LIBAA = $(LIBAA$(ISP))

OBJSlinux24_amd64 = linux24_amd64/AParser.o linux24_amd64/ASTBase.o linux24_amd64/ATokenBuffer.o linux24_amd64/DLexerBase.o linux24_amd64/PCCTSAST.o 
OBJSsun8 = sun8/AParser.o sun8/ASTBase.o sun8/ATokenBuffer.o sun8/DLexerBase.o sun8/PCCTSAST.o 
OBJECTS = $(OBJS$(ISP))

PROG = libh
EXE = ../$(ISP)/$(PROG).a

##############################
# targets
##############################

BUILD_TARGET =

all:
	$(MAKE) Debug
Optimized:
	$(MAKE) $(EXE) CFLAGS="-O2 $(CFLAGS)" BUILD_TARGET=Optimized
Debug:
	$(MAKE) $(EXE) CFLAGS="-g -DDEBUG $(CFLAGS)" BUILD_TARGET=Debug
clean:
	rm -rf $(ISP)/*

##############################
# target rules
##############################

.cxx.o:;
.cxx.a:;

##############################
# program rules
##############################

SYSLIBSlinux24_amd64 = 
SYSLIBSsun8 = 
SYSLIBS = $(SYSLIBS$(ISP))

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
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) $(SYSLIBS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@
$(LIBSOsun8): $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) -Bstatic $(SYSLIBS) -Bdynamic; \
	mv $(ISP)/$(PROG).$(SOL) $@

