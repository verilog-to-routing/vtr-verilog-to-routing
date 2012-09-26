#ifndef tokens_h
#define tokens_h
/* tokens.h -- List of labelled tokens and stuff
 *
 * Generated from: TCBP_CircuitBlifGrammar.g
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
	MODEL=11,
	INPUTS=12,
	OUTPUTS=13,
	CLOCKS=14,
	NAMES=15,
	LATCH=16,
	SUBCKT=17,
	END=18,
	IGNORED=19,
	TYPE_FALLING_EDGE=20,
	TYPE_RISING_EDGE=21,
	TYPE_ACTIVE_HIGH=22,
	TYPE_ACTIVE_LOW=23,
	TYPE_ASYNCHRONOUS=24,
	STATE_TRUE=25,
	STATE_FALSE=26,
	STATE_DONT_CARE=27,
	STATE_UNKNOWN=28,
	BIT_STRING=31,
	BIT_PAIR=32,
	DLGminToken=0,
	DLGmaxToken=9999};

#endif
