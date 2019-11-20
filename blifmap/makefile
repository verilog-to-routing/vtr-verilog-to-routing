EXE = blifmap

OBJ = main.o util.o map.o read_blif.o

SRC = main.c util.c map.c read_blif.c

H = util.h blifmap.h ext.h

CC = gcc

LIB = -lm

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
	$(CC) $(TOT_FLAGS) $(OBJ) -o $(EXE) $(LIB)

# Alternate linker options may be needed on some machines.  Try:
#       $(CC) $(TOT_FLAGS) $(OBJ) -o $(EXE) -L/usr/lib/X11R5 $(LIB)

main.o: main.c $(H)
	$(CC) -c $(TOT_FLAGS) main.c

read_blif.o: read_blif.c $(H)
	$(CC) -c $(TOT_FLAGS) read_blif.c

map.o:  map.c $(H)
	$(CC) -c $(TOT_FLAGS) map.c

util.o: util.c $(H)
	$(CC) -c $(TOT_FLAGS) util.c
