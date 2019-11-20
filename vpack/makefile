# This makefile is written for gcc running under Solaris on a SPARCstation.
# To compile VPR on other systems, you may have to change:
# (1) CC to the name of your C compiler.
# (2) ARCH_TYPE should be set to the name of your machine:  SUN, HP or SGI.
# (3) FLAGS should be changed to whatever options turn on maximum optimization
#     in your compiler.

CC = gcc

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

#Useful flags on HP machines.
#FLAGS = -Aa -g
#FLAGS = -Aa +O3



EXE = vpack

OBJ = main.o util.o ff_pack.o read_blif.o cluster.o output_clustering.o

SRC = main.c util.c ff_pack.c read_blif.c cluster.c output_clustering.c

H = util.h vpack.h ext.h read_blif.h cluster.h ff_pack.h output_clustering.h

LIB = -lm

TOT_FLAGS = $(FLAGS) -D$(ARCH_TYPE)


$(EXE): $(OBJ)
	$(CC) $(TOT_FLAGS) $(OBJ) -o $(EXE) $(LIB)

main.o: main.c $(H)
	$(CC) -c $(TOT_FLAGS) main.c

read_blif.o: read_blif.c $(H)
	$(CC) -c $(TOT_FLAGS) read_blif.c

ff_pack.o:  ff_pack.c $(H)
	$(CC) -c $(TOT_FLAGS) ff_pack.c

util.o: util.c $(H)
	$(CC) -c $(TOT_FLAGS) util.c

cluster.o: cluster.c $(H)
	$(CC) -c $(TOT_FLAGS) cluster.c

output_clustering.o: output_clustering.c $(H)
	$(CC) -c $(TOT_FLAGS) output_clustering.c
