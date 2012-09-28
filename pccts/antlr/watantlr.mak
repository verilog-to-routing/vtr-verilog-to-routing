SET=..\support\set
PCCTS_H=..\h

#
#   Watcom
#
CC=wcl386
ANTLR=..\bin\antlr
DLG=..\bin\dlg
CFLAGS= -I. -I$(SET) -I$(PCCTS_H) -DUSER_ZZSYN -DPC
OUT_OBJ = -o
OBJ_EXT = obj
LINK = wcl386

.c.obj :
	$(CC) -c $[* $(CFLAGS)

antlr.exe: antlr.obj scan.obj err.obj bits.obj build.obj fset2.obj &
	fset.obj gen.obj globals.obj hash.obj lex.obj main.obj &
	misc.obj set.obj pred.obj
	$(LINK) -fe=antlr.exe *.obj -k14336
	copy *.exe ..\bin

# *********** Target list of PC machines ***********
#
# Don't worry about the ambiguity messages coming from antlr
# for making antlr.c etc...  [should be 10 of them, I think]
#
antlr.c stdpccts.h parser.dlg tokens.h err.c : antlr.g
	$(ANTLR) antlr.g

antlr.$(OBJ_EXT): antlr.c mode.h tokens.h

scan.$(OBJ_EXT): scan.c mode.h tokens.h

scan.c mode.h: parser.dlg
	$(DLG) -C2 parser.dlg scan.c

set.$(OBJ_EXT): $(SET)\set.c
	$(CC) $(CFLAGS) -c set.$(OBJ_EXT) $(SET)\set.c

#
# ****** These next targets are common to UNIX and PC world ********
#

#clean up all the intermediate files
clean:
	del *.obj

#remove everything in clean plus the PCCTS files generated
scrub:
	del $(PCCTS_GEN) 
	del *.$(OBJ_EXT)
EOF_watantlr.mak
