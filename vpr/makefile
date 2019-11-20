# This makefile is written for gcc running under Solaris on a SPARCstation.
# To compile VPR on other systems, you may have to change:
# (1) CC to the name of your C compiler.
# (2) LIB_DIR to point at your directory of X11 libraries (libX11.a, etc.)
# (3) On many systems, the -R/usr/openwin/lib option on the LIB line
#     will have to be removed.
# (4) X11_INCLUDE to point at the directory containing "x11/xlib.h" etc.
# (5) ARCH_TYPE should be set to the name of your machine:  SUN, HP or SGI.
# (6) FLAGS should be changed to whatever options turn on maximum optimization
#     in your compiler.

CC = gcc

# On many non-Solaris machines, use LIB_DIR = -L/usr/lib/X11R5

LIB_DIR = -L/usr/openwin/lib

# Sometimes get errors under Solaris if you don't use the -R option
# to (I think) put info about where the shared object libraries are
# right into the binary.  Shouldn't be necessary, but it is on our machines.

LIB = -lX11 -lm -R/usr/openwin/lib

X11_INCLUDE = -I/usr/openwin/include

# Architecture types are SPARC, HP, SGI
# Currently the only incompatability is what random number generator
# is called -- depending on the architecture the preprocessor will 
# insert calls to the correct one.

ARCH_TYPE = SPARC

# Overly strict flags line below.  Lots of useless warnings, but can 
# let you look for redudant declarations. 
# To avoid redundant declarations here I use -D__STDC instead of
# -D__USE_FIXED_PROTOTYPES, but that means some prototypes are missing.

#FLAGS = -Wall -Wpointer-arith -Wcast-qual -Wstrict-prototypes -O -D__STDC__ -ansi -pedantic -Wredundant-decls -Wmissing-prototypes -Wshadow -Wcast-align -D_POSIX_SOURCE

#Flags to be passed to the compiler.  First is for strict warnings, 
#second for interactive debugging and third for optimization. 

#-D_POSIX_SOURCE stops extra declarations from being included in math.h
#and causing -Wshadow to complain about conflicts with y1 in math.h
#(Bessel function 1 of the second kind) 

#FLAGS = -Wall -Wpointer-arith -Wcast-qual -Wstrict-prototypes -O -D__USE_FIXED_PROTOTYPES__ -ansi -pedantic -Wmissing-prototypes -Wshadow -Wcast-align -D_POSIX_SOURCE
#FLAGS = -g
FLAGS = -O2

#Useful flags on HP machines
#FLAGS = -Aa -g
#FLAGS = -Aa +O3

TOT_FLAGS = $(FLAGS) -D$(ARCH_TYPE)

EXE = vpr

OBJ = main.o util.o read_netlist.o read_arch.o place.o route.o draw.o graphics.o stats.o rr_graph.o check_route.o hash.o read_place.o

SRC = main.c util.c read_netlist.c read_arch.c place.c route.c draw.c graphics.c stats.c rr_graph.c check_route.c hash.c read_place.c

H = util.h pr.h ext.h graphics.h read_netlist.h read_arch.h stats.h draw.h place.h route.h rr_graph.h check_route.h hash.h read_place.h


# I haven't been able to make -static work under Solaris.  Use shared
# libraries all the time.

$(EXE): $(OBJ)
	$(CC) $(TOT_FLAGS) $(OBJ) -o $(EXE) $(LIB_DIR) $(LIB)


main.o: main.c $(H)
	$(CC) -c $(TOT_FLAGS) main.c

read_netlist.o: read_netlist.c $(H)
	$(CC) -c $(TOT_FLAGS) read_netlist.c

read_arch.o: read_arch.c $(H)
	$(CC) -c $(TOT_FLAGS) read_arch.c

place.o: place.c $(H)
	$(CC) -c $(TOT_FLAGS) place.c

route.o: route.c $(H)
	$(CC) -c $(TOT_FLAGS) route.c

stats.o: stats.c $(H)
	$(CC) -c $(TOT_FLAGS) stats.c

util.o: util.c $(H)
	$(CC) -c $(TOT_FLAGS) util.c

draw.o: draw.c $(H)
	$(CC) -c $(TOT_FLAGS) draw.c

rr_graph.o: rr_graph.c $(H)
	$(CC) -c $(TOT_FLAGS) rr_graph.c

graphics.o: graphics.c $(H)
	$(CC) -c $(TOT_FLAGS) $(X11_INCLUDE) graphics.c

check_route.o: check_route.c $(H)
	$(CC) -c $(TOT_FLAGS) check_route.c

hash.o: hash.c $(H)
	$(CC) -c $(TOT_FLAGS) hash.c

read_place.o: read_place.c $(H)
	$(CC) -c $(TOT_FLAGS) read_place.c
