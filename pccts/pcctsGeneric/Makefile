##############################################################################
# Makefile built by imake.pl on Sep 4 2007 at 12:17:35 by Jeff Rudolph
##############################################################################

##############################################################################
# Do not hand edit this Makefile, rather make changes in imake.pl and have
# imake.pl regenerate this file.
##############################################################################

############################################
### Restrict clearmake parallel building ###
############################################
.SIBLINGS_AFFECT_REUSE:
.INCREMENTAL_REPOSITORY_SIBLING: SunWS_cache/CC_state
.NO_DO_FOR_SIBLING: SunWS_cache/@_WARNING SunWS_cache/CC_lockfile
.SIBLING_IGNORED_FOR_REUSE: SunWS_cache/CC_state SunWS_cache/CC_lockfile SunWS_cache/CC_version
.NOTPARALLEL :

MAKEFILE = Makefile
SHELL = /bin/sh

SOL              = $(SOL$(ISP))
SOLsun5          = so
SOLsun7          = so
SOLsun7_32       = so
SOLsun8          = so
SOLsun10         = so
SOLhpux          = sl
SOLhp1100        = sl
SOLhp1100_32     = sl
SOLlinux24       = so
SOLlinux24_amd64   = so
SOLlinux_i686   = so
SOLlinux_x86_64   = so

SHLIBS = 

##############################
# Rogue Wave
##############################

RWLIBsun5 = 
RWLIBsun7 = 
RWLIBsun7_32 = 
RWLIBsun8 = 
RWLIBhpux = -L/vobs/cpp/rw6.1/src/rogue -lrwtool
RWLIBhp1100 = 
RWLIBhp1100_32 = 
RWLIBlinux24 = 
RWLIBlinux24_amd64 = 
RWLIBlinux_i686 = 
RWLIBlinux_x86_64 = 
RWLIB = 

RWLIB = $(RWLIB$(ISP))

RWINCsun5 = 
RWINCsun7 = 
RWINCsun7_32 = 
RWINCsun8 = 
RWINChpux = -I/vobs/cpp/rw6.1/src/rogue
RWINChp1100 = 
RWINChp1100_32 = 
RWINClinux24 = 
RWINClinux24_amd64 = 
RWINClinux_i686 = 
RWINClinux_x86_64 = 
RWINC = $(RWINC$(ISP))

##############################
# Compiler Options
##############################

##############################
# compiler specific data
##############################

pC++hpux = /opt/aCC/bin/aCC
pC++hp1100 = /opt/aCC/bin/aCC
pC++hp1100_32 = /opt/aCC/bin/aCC
pC++sun5 = /apps/SUNWspro_6.0-U2/bin/CC
pC++sun7 = /apps/SUNWspro_6.0-U2/bin/CC
pC++sun7_32 = /apps/SUNWspro_6.0-U2/bin/CC
pC++sun8 = /apps/SUNWspro_6.0-U2/bin/CC
pC++sun10 = /apps/sun/studio/11/SUNWspro/bin/CC
pC++linux24 = /usr/bin/gcc
pC++linux24_amd64 = /usr/bin/gcc
pC++linux_i686 = /usr/bin/gcc
pC++linux_x86_64 = /usr/bin/gcc
pC++ = $(pC++$(ISP))

PIChpux = +Z  -DaCC -w
PIChp1100 = +Z -DaCC -w
PIChp1100_32 = +Z -DaCC -w
PICsun5 = -G -PIC
PICsun7 = -G -KPIC
PICsun7_32 = -G -KPIC
PICsun8 = -G -KPIC
PICsun10 = -G -KPIC
PIClinux24 = -shared
PIClinux24_amd64 = -shared
PIClinux_i686 = -shared
PIClinux_x86_64 = -shared
PIC = $(PIC$(ISP))

##############################
# Linker specific data
##############################

LINKERhpux = /opt/aCC/lbin/ld
LINKERhp1100 = /opt/aCC/lbin/ld
LINKERhp1100_32 = /opt/aCC/lbin/ld
LINKERsun5 = /usr/ccs/bin/ld
LINKERsun7 = /usr/ccs/bin/ld
LINKERsun7_32 = /usr/ccs/bin/ld
LINKERsun8 = /usr/ccs/bin/ld
LINKERsun10 = /usr/ccs/bin/ld
LINKERlinux24 = /usr/bin/ld
LINKERlinux24_amd64 = /usr/bin/ld
LINKERlinux_i686 = /usr/bin/ld
LINKERlinux_x86_64 = /usr/bin/ld
LINKER = $(LINKER$(ISP))
SLINKER = $(LINKER)

LFLAGShpux = -b -Wl,+s -Wl,-a,shared 
LFLAGShp1100 = -b +DA2.0W -Wl,+s -Wl,-a,shared 
LFLAGShp1100_32 = -b +DA2.0N -Wl,+s -Wl,-a,shared 
LFLAGSsun5 = -G
LFLAGSsun7 = -G -library=rwtools7,iostream -xarch=v9
LFLAGSsun7_32 = -G
LFLAGSsun8 = -G -library=rwtools7,iostream -xarch=v9
LFLAGSsun10 = -G -xarch=v9
LFLAGSlinux24 = -shared
LFLAGSlinux24_amd64 = -shared
LFLAGSlinux_i686 = -shared
LFLAGSlinux_x86_64 = -shared
LFLAGS = $(LFLAGS$(ISP))

##########
# CFLAGS #
##########

CFLAGShpux = -DBUILDING_LIB +DAportable -Dhp700 -Dhpux -g
CFLAGShp1100 = -DBUILDING_LIB +DA2.0W -DLP64 -Dhp700 -Dhpux
CFLAGShp1100_32 = -DBUILDING_LIB +DA2.0N -Dhp700 -Dhpux


CFLAGSsun7 = -DBUILDING_LIB +w -xarch=v9 -library=rwtools7,iostream -DLP64
CFLAGSsun7_32 = -DBUILDING_LIB +w -library=rwtools7,iostream
CFLAGSsun8 = -DBUILDING_LIB +w -xarch=v9 -library=rwtools7,iostream -DLP64
CFLAGSsun10 = -DBUILDING_LIB +w -xarch=v9 -library=iostream,Cstd -DLP64 -DSUN10
CFLAGSlinux24 = -W -DBUILDING_LIB
CFLAGSlinux24_amd64 = -W -fPIC -m64 -DBUILDING_LIB
CFLAGSlinux_i686 = -W -fPIC -DBUILDING_LIB
CFLAGSlinux_x86_64 = -W -fPIC -m64 -DBUILDING_LIB
CFLAGSsun5 = -DBUILDING_LIB +w -library=rwtools7,iostream -compat=4 -instances=static -Dsun5 -DSOLARIS -DSUN5 -Dsparcomp -DFUNCPROTO -DXTFUNCPROTO  
CFLAGS = $(CFLAGS$(ISP)) $(USERCCDEFINES) $(RWINC)\
	-I../h 

#######################################################
# Specify memory debugging tool, ie purify, insure etc.
########################################################
PURE =
VENDOR =

BUILD_TARGET =
#######################################################
#                        Targets 
#######################################################

all:
	$(MAKE) Debug
Optimized:
	$(MAKE) $(EXE) CFLAGS="-O2 $(CFLAGS)" BUILD_TARGET=Optimized
Debug:
	$(MAKE) $(EXE) CFLAGS="-g -DDEBUG $(CFLAGS)" BUILD_TARGET=Debug
Pure:
	$(MAKE) $(EXE) CFLAGS="-g $(CFLAGS)" PURE=purify
Quantify:
	$(MAKE) $(EXE) CFLAGS="-g $(CFLAGS)" PURE=quantify
Static:
	$(MAKE) $(EXE) CFLAGS="-g $(CFLAGS)" PIC="" LIBSO="" BUILD_TARGET=Static  GLINKhpux="$(GSLINKHPUX)" GLINKhp1100="$(GSLINKHP1100)" GLINKhp1100_32="$(GSLINKHP1100_32)" GLINKsun5="$(GSLINKSUN5)" GLINKsun7="$(GSLINKSUN7)" GLINKsun7_32="$(GSLINKSUN7_32)" GLINKsun8="$(GSLINKSUN8)"
Shared:
	$(MAKE) $(EXE) CFLAGS="-g $(CFLAGS)" BUILD_TARGET=Shared  GLINKhpux="$(GDLINKHPUX)" GLINKhp1100="$(GDLINKHP1100)" GLINKhp1100_32="$(GDLINKHP1100_32)" GLINKsun5="$(GDLINKSUN5)" GLINKsun7="$(GDLINKSUN7)" GLINKsun7_32="$(GDLINKSUN7_32)" GLINKsun8="$(GDLINKSUN8)"
 GLINKsun10="$(GDLINKSUN10)"

##############################
# program specific files
##############################

LIBSOhpux  = ../hpux/$(PROG).$(SOL)
LIBSOhp1100  = ../hp1100/$(PROG).$(SOL)
LIBSOhp1100_32  = ../hp1100_32/$(PROG).$(SOL)
LIBSOsun5  = ../sun5/$(PROG).$(SOL)
LIBSOsun7  = ../sun7/$(PROG).$(SOL)
LIBSOsun7_32  = ../sun7_32/$(PROG).$(SOL)
LIBSOsun8  = ../sun8/$(PROG).$(SOL)
LIBSOsun10  = ../sun10/$(PROG).$(SOL)
LIBSOlinux24  = ../linux24/$(PROG).$(SOL)
LIBSOlinux24_amd64  = ../linux24_amd64/$(PROG).$(SOL)
LIBSOlinux_i686  = ../linux_i686/$(PROG).$(SOL)
LIBSOlinux_x86_64  = ../linux_x86_64/$(PROG).$(SOL)
LIBSO      = $(LIBSO$(ISP))

LIBAAhpux   = ../hpux/$(PROG).aa
LIBAAhp1100   = ../hp1100/$(PROG).aa
LIBAAhp1100_32   = ../hp1100_32/$(PROG).aa
LIBAAsun5   = ../sun5/$(PROG).aa
LIBAAsun7   = ../sun7/$(PROG).aa
LIBAAsun7_32   = ../sun7_32/$(PROG).aa
LIBAAsun8   = ../sun8/$(PROG).aa
LIBAAsun10   = 
LIBAAlinux24 = 
LIBAAlinux24_amd64 = 
LIBAAlinux_i686 = 
LIBAAlinux_x86_64 = 
LIBAA       = $(LIBAA$(ISP))

PROG    = libpcctsGeneric
EXE     = ../$(ISP)/$(PROG).a

OBJShpux	= hpux/GenericToken.o hpux/GenericTokenBuffer.o hpux/NoLeakToken.o 
OBJShp1100	= hp1100/GenericToken.o hp1100/GenericTokenBuffer.o hp1100/NoLeakToken.o 
OBJShp1100_32	= hp1100_32/GenericToken.o hp1100_32/GenericTokenBuffer.o hp1100_32/NoLeakToken.o 
OBJSsun5	= sun5/GenericToken.o sun5/GenericTokenBuffer.o sun5/NoLeakToken.o 
OBJSsun7	= sun7/GenericToken.o sun7/GenericTokenBuffer.o sun7/NoLeakToken.o 
OBJSsun7_32	= sun7_32/GenericToken.o sun7_32/GenericTokenBuffer.o sun7_32/NoLeakToken.o 
OBJSsun8	= sun8/GenericToken.o sun8/GenericTokenBuffer.o sun8/NoLeakToken.o 
OBJSsun10	= sun10/GenericToken.o sun10/GenericTokenBuffer.o sun10/NoLeakToken.o 
OBJSlinux24	= linux24/GenericToken.o linux24/GenericTokenBuffer.o linux24/NoLeakToken.o 
OBJSlinux24_amd64	= linux24_amd64/GenericToken.o linux24_amd64/GenericTokenBuffer.o linux24_amd64/NoLeakToken.o 
OBJSlinux_i686	= linux_i686/GenericToken.o linux_i686/GenericTokenBuffer.o linux_i686/NoLeakToken.o 
OBJSlinux_x86_64	= linux_x86_64/GenericToken.o linux_x86_64/GenericTokenBuffer.o linux_x86_64/NoLeakToken.o 

OBJECTS = $(OBJS$(ISP))

clean:
	rm -rf $(ISP)/*
	rm -f *.lis *.lst purify.log core 
	cleartool ls

##############################
# target rules
##############################

.cxx.o:;
.cxx.a:;
SYSLIBShpux = 
SYSLIBShp1100 = 
SYSLIBShp1100_32 = 
SYSLIBSsun5 = -Bdynamic -lresolv
SYSLIBSsun7 = 
SYSLIBSsun7_32 = 
SYSLIBSsun8 = 
SYSLIBSsun10 = -L/vobs/cpp/pcre/sun10 -lpcre 
SYSLIBSlinux24 = 
SYSLIBSlinux24_amd64 = 
SYSLIBSlinux_i686 = 
SYSLIBSlinux_x86_64 = 
SYSLIBS = $(SYSLIBS$(ISP))
LIBDEFS = ../$(ISP)/libh.a 

LINKLIBS = -L../$(ISP) -lh  $(RWLIB)

##############################
# library dependency rules
##############################

../$(ISP)/libh.a:
	cd ../h ; $(MAKE) $(BUILD_TARGET)

##############################
# program rules
##############################

ARCMDhpux = /usr/ccs/bin/ar r $(EXE) $(OBJECTS) ; $(LINKER) -r -o ../$(ISP)/$(PROG).ao $(ISP)/*.o
ARCMDhp1100 = /usr/ccs/bin/ar r $(EXE) $(OBJECTS) ;$(LINKER) -r -o ../$(ISP)/$(PROG).ao $(ISP)/*.o
ARCMDhp1100_32 = /usr/ccs/bin/ar r $(EXE) $(OBJECTS) ;$(LINKER) -r -o ../$(ISP)/$(PROG).ao $(ISP)/*.o
ARCMDsun5 = $(pC++) -xar -o $(EXE) $(OBJECTS) 
ARCMDsun7 = $(pC++) -xar -xarch=v9 -library=rwtools7,iostream -o $(ISP)/$(PROG).a $(OBJECTS) ;\
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDsun7_32 = $(pC++) -xar -o $(ISP)/$(PROG).a $(OBJECTS) ;\
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDsun8 = $(pC++) -xar -xarch=v9 -library=rwtools7,iostream -o $(ISP)/$(PROG).a $(OBJECTS) ;\
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDsun10 = $(pC++) -xar -xarch=v9 -o $(ISP)/$(PROG).a $(OBJECTS) ;\
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDlinux24 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) ;\
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDlinux24_amd64 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) ;\
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDlinux_i686 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) ;\
	mv $(ISP)/$(PROG).a $(EXE)
ARCMDlinux_x86_64 = $(LINKER) -r -o $(ISP)/$(PROG).a $(OBJECTS) ;\
	mv $(ISP)/$(PROG).a $(EXE)
ARCMD = $(ARCMD$(ISP))
$(EXE): $(LIBSO)
	$(ARCMD)

$(OBJECTS): $(@F:.o=.cxx)
	$(PURE) $(pC++) -o $@ -c $(@F:.o=.cxx) $(PIC) $(CFLAGS)

$(LIBSOhpux):  $(OBJECTS) $(MWBUILDLIBS) $(MWPBLIBS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $@ $(OBJECTS)  \
	$(MWPBLIBS) $(MWAOLIBS)\
	$(GDLINKHPUX) $(TIDASLNK) $(INSLIBS) $(RWLIB) $(SYSLIBS)

$(LIBSOhp1100):  $(OBJECTS) $(MWBUILDLIBS) $(MWPBLIBS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $@ $(OBJECTS)  \
	$(MWPBLIBS) $(MWAOLIBS)\
	$(GDLINKHP1100) $(TIDASLNK) $(INSLIBS) $(RWLIB) $(SYSLIBS)

$(LIBSOhp1100_32):  $(OBJECTS) $(MWBUILDLIBS) $(MWPBLIBS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $@ $(OBJECTS)  \
	$(MWPBLIBS) $(MWAOLIBS)\
	$(GDLINKHP1100_32) $(TIDASLNK) $(INSLIBS) $(RWLIB) $(SYSLIBS)

$(LIBSOsun5):  $(OBJECTS) $(MWBUILDLIBS) $(MWPBLIBS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $@ $(OBJECTS)  \
	-Bstatic $(MWLINKLIBS) $(MWPBLIBS) $(GDLINKSUN5)\
	$(TIDASLNK) $(INSLIBS) $(RWLIB) $(SYSLIBS) -Bdynamic

$(LIBSOsun7):  $(OBJECTS) $(MWBUILDLIBS) $(MWPBLIBS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS)  \
	-Bstatic $(MWLINKLIBS) $(MWPBLIBS) $(GDLINKSUN7) \
	$(TIDASLNK) $(INSLIBS) $(RWLIB) $(SYSLIBS) -Bdynamic; \
	mv $(ISP)/$(PROG).$(SOL) $@

$(LIBSOsun7_32):  $(OBJECTS) $(MWBUILDLIBS) $(MWPBLIBS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS)  \
	-Bstatic $(MWLINKLIBS) $(MWPBLIBS) $(GDLINKSUN7_32)\
	$(TIDASLNK) $(INSLIBS) $(RWLIB) $(SYSLIBS) -Bdynamic; \
	mv $(ISP)/$(PROG).$(SOL) $@

$(LIBSOsun8):  $(OBJECTS) $(MWBUILDLIBS) $(MWPBLIBS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS)  \
	-Bstatic $(MWLINKLIBS) $(MWPBLIBS) $(GDLINKSUN_8)\
	$(TIDASLNK) $(INSLIBS) $(RWLIB) $(SYSLIBS) -Bdynamic; \
	mv $(ISP)/$(PROG).$(SOL) $@

$(LIBSOsun10):  $(OBJECTS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS)  \
	-Bstatic $(SYSLIBS) -Bdynamic; \
	mv $(ISP)/$(PROG).$(SOL) $@

$(LIBSOlinux24):  $(OBJECTS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS)  \
	$(SYSLIBS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

$(LIBSOlinux24_amd64):  $(OBJECTS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS)  \
	$(SYSLIBS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

$(LIBSOlinux_i686):  $(OBJECTS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS)  \
	$(SYSLIBS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

$(LIBSOlinux_x86_64):  $(OBJECTS) $(MAKEFILE)
	$(pC++) $(LFLAGS) -o $(ISP)/$(PROG).$(SOL) $(OBJECTS)  \
	$(SYSLIBS) ; \
	mv $(ISP)/$(PROG).$(SOL) $@

