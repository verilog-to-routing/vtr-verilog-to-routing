######################################################################
#
# Toro options store library
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

pC++linux_x86_64 = /usr/bin/gcc
pC++linux_i686 = /usr/bin/gcc
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
	-I../TIO_InputOutputHandlers \
	-I../TC_Common \
	-I../../pcre 

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

OBJSlinux_x86_64 = linux_x86_64/TOS_OptionsStore.o \
                    linux_x86_64/TOS_ControlSwitches.o \
                    linux_x86_64/TOS_RulesSwitches.o \
                    linux_x86_64/TOS_InputOptions.o \
                    linux_x86_64/TOS_OutputOptions.o \
                    linux_x86_64/TOS_MessageOptions.o \
                    linux_x86_64/TOS_ExecuteOptions.o \
                    linux_x86_64/TOS_PackOptions.o \
                    linux_x86_64/TOS_PlaceOptions.o \
                    linux_x86_64/TOS_RouteOptions.o \
                    linux_x86_64/TOS_StringUtils.o
OBJSlinux_i686 = linux_i686/TOS_OptionsStore.o \
                    linux_i686/TOS_ControlSwitches.o \
                    linux_i686/TOS_RulesSwitches.o \
                    linux_i686/TOS_InputOptions.o \
                    linux_i686/TOS_OutputOptions.o \
                    linux_i686/TOS_MessageOptions.o \
                    linux_i686/TOS_ExecuteOptions.o \
                    linux_i686/TOS_PackOptions.o \
                    linux_i686/TOS_PlaceOptions.o \
                    linux_i686/TOS_RouteOptions.o \
                    linux_i686/TOS_StringUtils.o
OBJSsun8 = sun8/TOS_OptionsStore.o \
           sun8/TOS_ControlSwitches.o \
           sun8/TOS_RulesSwitches.o \
           sun8/TOS_InputOptions.o \
           sun8/TOS_OutputOptions.o \
           sun8/TOS_MessageOptions.o \
           sun8/TOS_ExecuteOptions.o \
           sun8/TOS_PackOptions.o \
           sun8/TOS_PlaceOptions.o \
           sun8/TOS_RouteOptions.o \
           sun8/TOS_StringUtils.o
OBJECTS = $(OBJS$(ISP))

PROG = libTOS_OptionsStore
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

.cxx.o:;
.cxx.a:;

######################################################################
# Program rules
######################################################################

ARCMDlinux_x86_64 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDlinux_i686 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDsun8 = $(pC++) -xar -xarch=v9 -library=iostream -o $(ISP)/$(PROG).a $(OBJECTS) ; \
	mv $(ISP)/$(PROG).a $(EXE)
ARCMD = $(ARCMD$(ISP))
$(EXE): $(LIBSO)
	$(ARCMD)

$(OBJECTS): $(@F:.o=.cxx)
	$(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

$(LIBSOlinux_x86_64): $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@
$(LIBSOlinux_i686): $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@
$(LIBSOsun8): $(OBJECTS) Makefile
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

######################################################################
# Library rules
######################################################################

../$(ISP)/libTIO_InputOutputHandlers.a:
	cd ../TIO_InputOutputHandlers ; $(MAKE) $(BUILD_TARGET)

../$(ISP)/libTC_Common.a:
	cd ../TC_Common ; $(MAKE) $(BUILD_TARGET)
