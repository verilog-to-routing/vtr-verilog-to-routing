CC = g++
AR = ar

WARN_FLAGS = -Wall 

DEBUG_FLAGS = -g
OPT_FLAGS = -O2
INC_FLAGS = -I./
LIB_FLAGS = rcs

FLAGS = $(DEBUG_FLAGS) $(WARN_FLAGS) $(INC_FLAGS) -Wno-write-strings

SRC = vqm_common.c vqm_dll.cpp
GEN = vqm_parser.tab.c lex.yy.c vqm_parser.tab.h
H = vqm_common.h vqm_dll.h vqm_stratix.h

PARSER1 = $(if $(strip $(shell which flex)), flex, \
	$(error flex not found.  ODIN requires flex to compile.))
PARSER2 = $(if $(strip $(shell which bison)), bison, \
	$(error bison not found.  ODIN requires bison to compile.))

all: libvqm.a

libvqm.a: $(SRC) vqm_parser.l vqm_parser.y $(H)
	$(PARSER1) vqm_parser.l
	$(PARSER2) -d vqm_parser.y
	$(CC) $(FLAGS) -c *.c *.cpp
	$(AR) $(LIB_FLAGS) $@ *.o

clean:
	rm -f *.o *.a $(GEN)

