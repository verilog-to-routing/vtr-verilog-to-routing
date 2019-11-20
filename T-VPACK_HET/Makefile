# This makefile is written for gcc running under Solaris on a SPARCstation.
# To compile T-VPack on other systems, you may have to change:
# (1) CC to the name of your C compiler.
# (2) OPT_FLAGS should be changed to whatever options turn on maximum 
#     optimization in your compiler.

CC = gcc
#CC = purify gcc
#CC = g++


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
OPT_FLAGS = -O2

FLAGS = $(DEBUG_FLAGS)
#FLAGS = $(OPT_FLAGS)
#FLAGS = $(WARN_FLAGS)

SRC_DIR = SRC

EXE = t-vpack.exe

OBJ = \
OBJ/main.o \
OBJ/util.o \
OBJ/ff_pack.o \
OBJ/read_blif.o \
OBJ/cluster.o \
OBJ/output_clustering.o \
OBJ/path_length.o \
OBJ/heapsort.o

SRC = SRC/main.c SRC/util.c SRC/ff_pack.c SRC/read_blif.c SRC/cluster.c SRC/output_clustering.c SRC/path_length.c SRC/heapsort.c

H = SRC/util.h SRC/vpack.h SRC/globals.h SRC/read_blif.h SRC/cluster.h SRC/ff_pack.h SRC/output_clustering.h SRC/path_length.h SRC/heapsort.h

LIB = -lm

# Add purify in front of CC below to run purify on the code.

$(EXE): $(OBJ)
	$(CC) $(FLAGS) $(OBJ) -o $(EXE) $(LIB)

OBJ/%.o:$(SRC_DIR)/%.c $(H)
	$(CC) $(FLAGS) -c $< -o $@

clean :
	rm -f t-vpack.exe 
	rm -f OBJ/*.o
