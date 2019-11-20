EXE = vpr

OBJ = main.o util.o read_netlist.o read_arch.o place.o route.o draw.o graphics.o stats.o

SRC = main.c util.c read_netlist.c read_arch.c place.c route.c draw.c graphics.c stats.c

H = util.h pr.h ext.h graphics.h

CC = gcc

LIB = -lX11 -lm

#Flags to be passed to the compiler.  First is for strict warnings, 
#second for interactive debugging and third for optimization. 
 
#FLAGS = -Wall -Wpointer-arith -Wcast-qual -Wstrict-prototypes -O -D__USE_FIXED_PROTOTYPES__ -ansi -pedantic
#FLAGS = -g
FLAGS = -O2

#Useful flags on HP machines.
#FLAGS = -Aa -g
#FLAGS = -Aa +O3

# Architecture types are SPARC, HP, SGI
# Currently the only incompatability is what random number generator
# is called -- depending on the architecture the preprocessor will 
# insert calls to the correct one.
ARCH_TYPE = SPARC

TOT_FLAGS = $(FLAGS) -D$(ARCH_TYPE)

$(EXE): $(OBJ)
	$(CC) $(TOT_FLAGS) -static $(OBJ) -o $(EXE) $(LIB)

# Alternate linker options may be needed on some machines.  Try:
#	$(CC) $(TOT_FLAGS) $(OBJ) -o $(EXE) -L/usr/lib/X11R5 $(LIB)

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

graphics.o: graphics.c graphics.h
	$(CC) -c $(TOT_FLAGS) graphics.c
