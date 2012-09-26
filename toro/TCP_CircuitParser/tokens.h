#ifndef tokens_h
#define tokens_h
/* tokens.h -- List of labelled tokens and stuff
 *
 * Generated from: TCP_CircuitGrammar.g
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * ANTLR Version 1.33MR33
 */
enum ANTLRTokenType {
	END_OF_FILE=1,
	CLOSE_QUOTE=2,
	STRING=3,
	UNCLOSED_STRING=4,
	CPP_COMMENT=6,
	BLOCK_COMMENT=7,
	NEW_LINE=8,
	OPEN_QUOTE=9,
	EQUAL=10,
	CIRCUIT=11,
	IO=12,
	PB=13,
	SB=14,
	PORT=15,
	INST=16,
	CELL=17,
	MASTER=18,
	NET=19,
	PIN=20,
	TYPE=21,
	ROUTABLE=22,
	STATUS=23,
	PACK=24,
	PLACE=25,
	CHANNEL=26,
	RELATIVE=27,
	GROUTE=28,
	ROUTE=29,
	SWITCHBOX=30,
	PLACE_FLOAT=31,
	PLACE_FIXED=32,
	PLACE_PLACED=33,
	NET_OPEN=35,
	NET_GROUTED=36,
	NET_ROUTED=37,
	LEFT=39,
	RIGHT=40,
	LOWER=41,
	UPPER=42,
	BOTTOM=43,
	TOP=44,
	R=46,
	T=47,
	INPUT=48,
	OUTPUT=49,
	SIGNAL=50,
	CLOCK=51,
	POWER=52,
	GLOBAL=53,
	BOOL_TRUE=55,
	BOOL_FALSE=56,
	BIT_CHAR=58,
	NEG_INT=59,
	POS_INT=60,
	FLOAT=61,
	EXP=62,
	REGION=66,
	HIER=68,
	DLGminToken=0,
	DLGmaxToken=9999};

#endif
