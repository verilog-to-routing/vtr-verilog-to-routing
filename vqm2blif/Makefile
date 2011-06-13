CC = g++

WARN_FLAGS = -Wall 

DEBUG_FLAGS = -g
OPT_FLAGS = -O2
INC_FLAGS = -Iinclude
LIB_FLAGS = rcs

EXE = vqm2blif 

FLAGS = $(DEBUG_FLAGS) $(WARN_FLAGS) $(INC_FLAGS)

all: $(EXE)

$(EXE): vqm2blif.cpp vqm2blif.o ../libvqm/libvqm.a ../libvpr/libvpr_6.a
	$(CC) vqm2blif.cpp $(FLAGS) -c
	$(CC) vqm2blif.o -o $(EXE) -L../libvpr -lm -lvpr_6 -L../libvqm -lm -lvqm
	rm -f vqm2blif.o

.cpp.o:%.cpp
	$(CC) $(FLAGS) -c $< -o $@ 

#Run the executable on all .vqms in the current directory
blifs: $(EXE)
	for i in ../VQM/*_stratix.vqm; do \
		$(EXE) $$i ../ARCH/stratix_arch.xml; \
	done

	for i in ../VQM/*_stratixii.vqm; do \
		$(EXE) $$i ../ARCH/stratixii_arch.xml; \
	done

clean:
	rm -f $(EXE) read_arch 
	rm -f ../OUT/*.blif ../OUT/*.echo *.blif *.echo

# Common Typos
clena: clean
