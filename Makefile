CC = g++
VQM2BLIF_DIR = EXE

all: 
	cd $(VQM2BLIF_DIR) && make

clean:
	rm -f $(EXE)
	cd $(VQM2BLIF_DIR) && make clean
