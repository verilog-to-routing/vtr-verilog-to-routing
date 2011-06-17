CC = g++

WARN_FLAGS = -Wall 

DEBUG_FLAGS = -g
OPT_FLAGS = -O2
INC_FLAGS = -Iinclude
LIB_FLAGS = rcs

EXE = vqm2blif 

FLAGS = $(DEBUG_FLAGS) $(WARN_FLAGS) $(INC_FLAGS)

SIV_VQMS = $(wildcard ../VQM/*_stratixiv.vqm)
SIV_PROJS = $(basename $(notdir $(SIV_VQMS))) 

OUTS = $(basename $(notdir $(wildcard ../OUT/exp/*/*_stratixiv.blif)))

all: $(EXE)

$(EXE): vqm2blif.cpp vqm2blif.h ../libvqm/libvqm.a ../libvpr/libvpr_6.a
	$(CC) vqm2blif.cpp $(FLAGS) -c
	$(CC) vqm2blif.o -o $(EXE) -L../libvpr -lm -lvpr_6 -L../libvqm -lm -lvqm
	rm -f vqm2blif.o

.cpp.o:%.cpp
	$(CC) $(FLAGS) -c $< -o $@ 

#Run the executable on all stratixiv.vqms in the VQM directory
blifs: $(EXE)
	mkdir -p ../OUT/exp

	for i in $(SIV_PROJS); do \
		mkdir -p ../OUT/exp/$$i; \
		$(EXE) -vqm ../VQM/$$i.vqm -arch ../ARCH/stratixiv_arch.xml -out ../OUT/exp/$$i/$$i.blif; \
	done

#Compare the blifs outputted to the golden results
verify: 
	for i in $(OUTS); do \
		diff -y --suppress-common-lines ../OUT/exp/$$i/$$i.blif ../OUT/golden/$$i/$$i.blif; \
	done

clean:
	rm -f $(EXE) read_arch 
	rm -r -f ../OUT/exp/*
	rm -f *.blif *.echo
test:
	echo $(SIV_PROJS)
	echo $(OUTS)

# Common Typos
clena: clean
