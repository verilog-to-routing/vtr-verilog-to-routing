CC = g++
VQM2BLIF_DIR = EXE

export BUILD_TYPE = release
export OPTIMIZATION_LEVEL_FOR_RELEASE_BUILD = -O2


#ensure coherence with /EXE/Makefile
EXE = vqm2blif.exe

all: 
	cd $(VQM2BLIF_DIR) && make
	cp -f ./$(VQM2BLIF_DIR)/$(EXE) ./
clean:
	rm -f $(EXE)
	cd $(VQM2BLIF_DIR) && make clean
