
CC   := gcc
CXX  := g++ 
LD   := g++
CP   := cp

PROG := abc

MODULES := src/base/abc src/base/abci src/base/cmd \
	src/base/io src/base/main src/base/ver \
	src/aig/ivy src/aig/hop src/aig/rwt src/aig/deco \
	src/aig/mem src/aig/dar src/aig/fra src/aig/cnf \
	src/aig/csw src/aig/ec src/aig/aig src/aig/kit \
	src/aig/bdc \
	src/bdd/cudd src/bdd/dsd src/bdd/epd src/bdd/mtr \
	src/bdd/parse src/bdd/reo src/bdd/cas \
	src/map/fpga src/map/mapper src/map/mio \
	src/map/super src/map/if \
	src/misc/extra src/misc/mvc src/misc/st src/misc/util \
	src/misc/espresso src/misc/nm src/misc/vec \
	src/misc/hash \
	src/opt/cut src/opt/dec src/opt/fxu src/opt/rwr \
	src/opt/sim src/opt/ret src/opt/res src/opt/lpk \
	src/sat/bsat src/sat/csat src/sat/msat src/sat/fraig \
	src/phys/place 	

default: $(PROG)

#OPTFLAGS  := -DNDEBUG -O3
OPTFLAGS  := -g -O

CFLAGS   += -w $(OPTFLAGS) $(patsubst %, -I%, $(MODULES)) 
CFLAGS   += -MD -MP
CXXFLAGS += $(CFLAGS) 

#LIBS := -ldl -rdynamic -ltermcap
LIBS := -ldl -rdynamic

SRC  := 
GARBAGE := core core.* *.stackdump ./tags $(PROG)

.PHONY: tags clean docs

include $(patsubst %, %/module.make, $(MODULES))

OBJ := \
	$(patsubst %.cc, %.o, $(filter %.cc, $(SRC))) \
	$(patsubst %.c, %.o,  $(filter %.c, $(SRC)))  \
	$(patsubst %.y, %.o,  $(filter %.y, $(SRC))) 

DEP := $(OBJ:.o=.d)

# implicit rules
#  Jason Luu Oct 26: Not necessary due to -MP -MD
#%.d: %.c
#	./depends.sh $(CC) `dirname $*.c` $(CFLAGS) $*.c > $@
#
#%.d: %.cc
#	./depends.sh $(CXX) `dirname $*.cc` $(CXXFLAGS) $(CFLAGS) $*.cc > $@
#
#-include $(DEP)

# Actual targets

depend: $(DEP)

clean: 
	rm -rf $(PROG) $(OBJ) $(GARBAGE) $(OBJ:.o=.d) 

tags:
	ctags -R .

$(PROG): $(OBJ) lib$(PROG).a
	$(LD) -o $@ $^ $(LIBS)

lib$(PROG).a: $(OBJ)
	ar rv $@ $?
	ranlib $@

docs:
	doxygen doxygen.conf

