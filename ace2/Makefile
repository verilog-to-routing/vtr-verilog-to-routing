CC = gcc
CFLAGS = -g -O0 -w
LFLAGS = -g -O0

EXE = ace

ABC_DIR = ../abc_with_bb_support
ABC_LIB = libabc.a
ABC_LIB_PATH = $(ABC_DIR)/$(ABC_LIB)

OBJS = ace.o io_ace.o blif.o cycle.o sim.o bdd.o cube.o depth.o
ACE_HEADERS = ace.h io_ace.h blif.h depth.h cube.h bdd.h

ABC_HEADERS = -I $(ABC_DIR)/src/base/io \
			 -I $(ABC_DIR)/src/base/abc \
			-I $(ABC_DIR)/src/base/main \
			-I $(ABC_DIR)/src/base/cmd \
			-I $(ABC_DIR)/src/misc/vec \
			-I $(ABC_DIR)/src/misc/util \
			-I $(ABC_DIR)/src/misc/st \
			-I $(ABC_DIR)/src/misc/extra \
			-I $(ABC_DIR)/src/misc/ext \
			-I $(ABC_DIR)/src/misc/espresso \
			-I $(ABC_DIR)/src/misc/avl \
			-I $(ABC_DIR)/src/misc/nm \
			-I $(ABC_DIR)/src/aig/aig \
			-I $(ABC_DIR)/src/aig/hop \
			-I $(ABC_DIR)/src/aig/mem \
			-I $(ABC_DIR)/src/aig/tim \
			-I $(ABC_DIR)/src/aig/ssw \
			-I $(ABC_DIR)/src/aig/fra \
			-I $(ABC_DIR)/src/aig/nwk \
			-I $(ABC_DIR)/src/aig/ntl \
			-I $(ABC_DIR)/src/aig/gia \
			-I $(ABC_DIR)/src/aig/dar \
			-I $(ABC_DIR)/src/aig/ioa \
			-I $(ABC_DIR)/src/bdd/cudd \
			-I $(ABC_DIR)/src/bdd/mtr \
			-I $(ABC_DIR)/src/bdd/epd \
			-I $(ABC_DIR)/src/map/if \
			-I $(ABC_DIR)/src/sat/bsat
			
LIBS = -lm -ldl
DEFINES = -DLIN

$(EXE): $(OBJS) Makefile $(ABC_LIB_PATH)
	$(CC) $(OBJS) $(CFLAGS) $(ABC_HEADERS) $(ABC_LIB_PATH) $(LIBS) -o $(EXE)
	
clean:
	rm -f *.o *.exe $(TARGET) $(EXE)
	
%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@ $(ABC_HEADERS) $(DEFINES) $(LIBS)