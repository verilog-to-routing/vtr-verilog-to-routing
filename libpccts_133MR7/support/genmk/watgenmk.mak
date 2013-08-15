SET=..\..\support\set
PCCTS_H=..\..\h

#
#   Watcom
#
CC=wcl386
ANTLR=..\..\bin\antlr
DLG=..\..\bin\dlg
CFLAGS= -I. -I$(SET) -I$(PCCTS_H) -DUSER_ZZSYN -DPC
OUT_OBJ = -o
OBJ_EXT = obj
LINK = wcl386

.c.obj :
	$(CC) -c $[* $(CFLAGS)

genmk.exe: genmk.obj
	$(LINK) -fe=genmk.exe *.obj -k14336
	copy *.exe ..\..\bin

#clean up all the intermediate files
clean:
	del *.obj

#remove everything in clean plus the PCCTS files generated
scrub:
	del *.$(OBJ_EXT)
EOF_watgenmk.mak
cat << \EOF_makefile | sed 's/^>//' > makefile
SRC=genmk.c
OBJ=genmk.o
# Define PC if you use a PC OS (changes directory symbol and object file extension)
# see pccts/h/config.h
#CFLAGS=-I../../h -DPC
CFLAGS=-I../../h
CC=cc
BAG=../../bin/bag

genmk: $(OBJ) $(SRC) ../../h/config.h
	$(CC) -o genmk $(OBJ)

clean:
	rm -rf core *.o

scrub:
	rm -rf genmk core *.o

shar:
	shar genmk.c makefile > genmk.shar

archive:
	$(BAG) genmk.c watgenmk.mak makefile > genmk.bag
EOF_makefile
