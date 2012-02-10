CC = g++
VQM2BLIF_DIR = EXE

#ensure coherence with /EXE/Makefile
EXE = vqm2blif.exe

all: 
	cd $(VQM2BLIF_DIR) && make
	cp ./$(VQM2BLIF_DIR)/$(EXE) ./
clean:
	rm -f $(EXE)
	cd $(VQM2BLIF_DIR) && make clean
