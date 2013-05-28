SET=..\support\set
PCCTS_H=..\h

#
#   Watcom
#
CC=wcl386
ANTLR=..\bin\antlr
DLG=..\bin\dlg
CFLAGS= -I. -I$(SET) -I$(PCCTS_H) -DUSER_ZZSYN -DPC
LIBS=
OBJ_EXT = obj
LINK = wcl386

.c.obj :
	$(CC) -c $[* $(CFLAGS)

dlg.exe : dlg_p.obj dlg_a.obj main.obj err.obj set.obj support.obj &
	output.obj relabel.obj automata.obj
	$(LINK) -fe=dlg.exe *.obj -k14336
	copy *.exe ..\bin

SRC = dlg_p.c dlg_a.c main.c err.c $(SET)\set.c support.c output.c &
	relabel.c automata.c

dlg_p.c parser.dlg err.c tokens.h : dlg_p.g
	$(ANTLR) dlg_p.g

dlg_a.c mode.h : parser.dlg
	$(DLG) -C2 parser.dlg dlg_a.c

dlg_p.$(OBJ_EXT) : dlg_p.c dlg.h tokens.h mode.h
	$(CC) $(CFLAGS) -c dlg_p.c

dlg_a.$(OBJ_EXT) : dlg_a.c dlg.h tokens.h mode.h
	$(CC) $(CFLAGS) -c dlg_a.c

main.$(OBJ_EXT) : main.c dlg.h
	$(CC) $(CFLAGS) -c main.c

set.$(OBJ_EXT) : $(SET)\set.c
	$(CC) -c $(CFLAGS) $(SET)\set.c

#clean up all the intermediate files
clean:
	del *.$(OBJ_EXT)
EOF_watdlg.mak
