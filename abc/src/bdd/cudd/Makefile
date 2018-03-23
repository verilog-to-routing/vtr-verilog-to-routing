# $Id$
#
#    Cudd - DD package
#---------------------------
.SUFFIXES: .o .c .u

CC	= gcc
RANLIB	= ranlib
PURE	=
# Define EXE as .exe for MS-DOS and derivatives.
EXE	=
#EXE	= .exe

MFLAG	=
ICFLAGS	= -g
XCFLAGS	= -DDD_STATS
CFLAGS	= $(ICFLAGS) $(MFLAG) $(XCFLAGS)
#DDDEBUG	= -DDD_DEBUG -DDD_CACHE_PROFILE -DDD_VERBOSE -DDD_UNIQUE_PROFILE
DDDEBUG	=

LINTFLAGS = -u -n -DDD_STATS -DDD_CACHE_PROFILE -DDD_VERBOSE -DDD_DEBUG -DDD_UNIQUE_PROFILE

# this is to create the lint library
LINTSWITCH = -o

WHERE	= ..

INCLUDE = $(WHERE)/include

LIBS	= ./libcudd.a $(WHERE)/mtr/libmtr.a \
	$(WHERE)/st/libst.a $(WHERE)/util/libutil.a $(WHERE)/epd/libepd.a

MNEMLIB =

BLIBS	= -kL. -klcudd -kL$(WHERE)/mtr -klmtr \
	-kL$(WHERE)/st -klst -kL$(WHERE)/util -klutil -kL$(WHERE)/epd -klepd

LINTLIBS = ./llib-lcudd.ln $(WHERE)/mtr/llib-lmtr.ln \
	$(WHERE)/st/llib-lst.ln $(WHERE)/util/llib-lutil.ln \
	$(WHERE)/epd/llib-lepd.ln

LDFLAGS	=

# files for the package
P	= cudd
PSRC	= cuddAPI.c cuddAddAbs.c cuddAddApply.c cuddAddFind.c cuddAddIte.c \
	  cuddAddInv.c cuddAddNeg.c cuddAddWalsh.c cuddAndAbs.c \
	  cuddAnneal.c cuddApa.c cuddApprox.c cuddBddAbs.c cuddBddCorr.c \
	  cuddBddIte.c cuddBridge.c cuddCache.c cuddCheck.c cuddClip.c \
	  cuddCof.c cuddCompose.c cuddDecomp.c cuddEssent.c \
	  cuddExact.c cuddExport.c cuddGenCof.c cuddGenetic.c \
	  cuddGroup.c cuddHarwell.c cuddInit.c cuddInteract.c \
	  cuddLCache.c cuddLevelQ.c \
	  cuddLinear.c cuddLiteral.c cuddMatMult.c cuddPriority.c \
	  cuddRead.c cuddRef.c cuddReorder.c cuddSat.c cuddSign.c \
	  cuddSolve.c cuddSplit.c cuddSubsetHB.c cuddSubsetSP.c cuddSymmetry.c \
	  cuddTable.c cuddUtil.c cuddWindow.c cuddZddCount.c cuddZddFuncs.c \
	  cuddZddGroup.c cuddZddIsop.c cuddZddLin.c cuddZddMisc.c \
	  cuddZddPort.c cuddZddReord.c cuddZddSetop.c cuddZddSymm.c \
	  cuddZddUtil.c
PHDR    = cudd.h cuddInt.h
POBJ	= $(PSRC:.c=.o)
PUBJ	= $(PSRC:.c=.u)
TARGET	= test$(P)$(EXE)
TARGETu = test$(P)-u

# files for the test program
SRC	= test$(P).c
OBJ	= $(SRC:.c=.o)
UBJ	= $(SRC:.c=.u)

#------------------------------------------------------

lib$(P).a: $(POBJ)
	ar rv $@ $?
	$(RANLIB) $@

.c.o: $(PSRC) $(PHDR)
	$(CC) -c  $< -I$(INCLUDE) $(CFLAGS) $(DDDEBUG) 

optimize_dec: lib$(P).b

lib$(P).b: $(PUBJ)
	ar rv $@ $?
	$(RANLIB) $@

.c.u: $(PSRC) $(PHDR)
	cc -j $< -I$(INCLUDE) $(XCFLAGS)

# if the header files change, recompile
$(POBJ): $(PHDR)
$(PUBJ): $(PHDR)
$(OBJ): $(PHDR)
$(UBJ): $(PHDR)

$(TARGET): $(SRC) $(OBJ) $(HDR) $(LIBS) $(MNEMLIB)
	$(PURE) $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LIBS) $(MNEMLIB) -lm

# optimize (DECstations and Alphas only: uses u-code)
$(TARGETu): $(SRC) $(UBJ) $(HDR) $(LIBS:.a=.b)
	$(CC) -O3 -Olimit 1000 $(XCFLAGS) $(LDFLAGS) -o $@ $(UBJ) $(BLIBS) -lm

lint: llib-l$(P).ln

llib-l$(P).ln: $(PSRC) $(PHDR)
	lint $(LINTFLAGS) $(LINTSWITCH)$(P) -I$(INCLUDE) $(PSRC)

lintpgm: lint
	lint $(LINTFLAGS) -I$(INCLUDE) $(SRC) $(LINTLIBS)

tags: $(PSRC) $(PHDR)
	ctags $(PSRC) $(PHDR)

all: lib$(P).a lib$(P).b llib-l$(P).ln tags

programs: $(TARGET) $(TARGETu) lintpgm

clean:
	rm -f *.o *.u mon.out gmon.out *.pixie *.Addrs *.Counts mnem.* \
	.pure core *.warnings

distclean: clean
	rm -f $(TARGET) $(TARGETu) lib*.a lib$(P).b llib-l$(P).ln \
	*.bak *~ tags .gdb_history *.qv *.qx
