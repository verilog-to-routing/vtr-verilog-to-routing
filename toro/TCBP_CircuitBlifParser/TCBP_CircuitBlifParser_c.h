/*
 * TCBP_CircuitBlifParser_c: P a r s e r  H e a d e r 
 *
 * Generated from: TCBP_CircuitBlifGrammar.g
 *
 * Terence Parr, Russell Quong, Will Cohen, and Hank Dietz: 1989-2001
 * Parr Research Corporation
 * with Purdue University Electrical Engineering
 * with AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 */

#ifndef TCBP_CircuitBlifParser_c_h
#define TCBP_CircuitBlifParser_c_h

#ifndef ANTLR_VERSION
#define ANTLR_VERSION 13333
#endif

#include "AParser.h"


#include <stdio.h>

#include "stdpccts.h"
#include "GenericTokenBuffer.h"

#include "TIO_PrintHandler.h"

#include "TCD_CircuitDesign.h"

#include "TCBP_CircuitBlifFile.h"
#include "TCBP_CircuitBlifScanner_c.h"
class TCBP_CircuitBlifParser_c : public ANTLRParser {
public:
	static  const ANTLRChar *tokenName(int tk);
	enum { SET_SIZE = 33 };
protected:
	static const ANTLRChar *_token_tbl[];
private:

public:

   void syn( ANTLRAbstractToken* /* pToken */, 
ANTLRChar*          pszGroup,
SetWordType*        /* pWordType */,
ANTLRTokenType      tokenType,
int                 /* k */ );

   void SetScanner( TCBP_CircuitBlifScanner_c* pscanner );
void SetFileName( const char* pszFileName );
void SetCircuitBlifFile( TCBP_CircuitBlifFile_c* pcircuitBlifFile );
void SetCircuitDesign( TCD_CircuitDesign_c* pcircuitDesign );

private:

   TPO_LatchType_t FindLatchType_( ANTLRTokenType tokenType );
TPO_LatchState_t FindLatchState_( ANTLRTokenType tokenType );

private:

   TCBP_CircuitBlifScanner_c* pscanner_;
string                     srFileName_;
TCBP_CircuitBlifFile_c*    pcircuitBlifFile_;
TCD_CircuitDesign_c*       pcircuitDesign_;

   size_t circuitSubcktCount_;
protected:
	static SetWordType setwd1[33];
	static SetWordType err1[8];
	static SetWordType setwd2[33];
	static SetWordType err2[8];
	static SetWordType err3[8];
	static SetWordType LATCH_TYPE_VAL_set[8];
	static SetWordType LATCH_TYPE_VAL_errset[8];
	static SetWordType LATCH_STATE_VAL_set[8];
	static SetWordType LATCH_STATE_VAL_errset[8];
	static SetWordType err8[8];
	static SetWordType err9[8];
	static SetWordType setwd3[33];
private:
	void zzdflthandlers( int _signal, int *_retsignal );

public:
	TCBP_CircuitBlifParser_c(ANTLRTokenBuffer *input);
	void start(void);
	void topModel(string* psrName,TPO_InstList_t* pinstList,TPO_PortList_t* pportList);
	void cellModel(TLO_CellList_t* pcellList);
	void pinList(TPO_PortList_t* pportList,TC_TypeMode_t type);
	void portList(TLO_PortList_t* pportList,TC_TypeMode_t type);
	void namesElem(TPO_InstList_t* pinstList);
	void latchElem(TPO_InstList_t* pinstList);
	void subcktElem(TPO_InstList_t* pinstList);
	void logicBitsTable(TPO_LogicBitsList_t* plogicBitsList);
	void logicBitsElem(TPO_LogicBits_t* plogicBits);
	void latchType(TPO_LatchType_t* ptype);
	void latchState(TPO_LatchState_t* pstate);
	void stringText(string* psrString);
};

#endif /* TCBP_CircuitBlifParser_c_h */
