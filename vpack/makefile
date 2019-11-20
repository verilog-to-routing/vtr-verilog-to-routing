# This makefile is written for gcc running under Solaris on a SPARCstation.
# To compile VPR on other systems, you may have to change:
# (1) CC to the name of your C compiler.
# (2) FLAGS should be changed to whatever options turn on maximum optimization
#     in your compiler.

CC = gcc

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
 
WARN_FLAGS = -Wall -Wpointer-arith -Wcast-qual -Wstrict-prototypes -O -D__USE_FIXED_PROTOTYPES__ -ansi -pedantic -Wmissing-prototypes -Wshadow -Wcast-align -D_POSIX_SOURCE

DEBUG_FLAGS = -g
OPT_FLAGS = -O3

FLAGS = $(OPT_FLAGS)

#Useful flags on HP machines.
#DEBUG_FLAGS = -Aa -g
#OPT_FLAGS = -Aa +O3

EXE = vpack

OBJ = main.o util.o ff_pack.o read_blif.o cluster.o output_clustering.o

SRC = main.c util.c ff_pack.c read_blif.c cluster.c output_clustering.c

H = util.h vpack.h globals.h read_blif.h cluster.h ff_pack.h output_clustering.h

LIB = -lm

# Add purify in front of CC below to run purify on the code.

$(EXE): $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o $(EXE) $(LIB)

main.o: main.c $(H)
	$(CC) -c $(FLAGS) main.c

read_blif.o: read_blif.c $(H)
	$(CC) -c $(FLAGS) read_blif.c

ff_pack.o:  ff_pack.c $(H)
	$(CC) -c $(FLAGS) ff_pack.c

util.o: util.c $(H)
	$(CC) -c $(FLAGS) util.c

cluster.o: cluster.c $(H)
	$(CC) -c $(FLAGS) cluster.c

output_clustering.o: output_clustering.c $(H)
	$(CC) -c $(FLAGS) output_clustering.c
