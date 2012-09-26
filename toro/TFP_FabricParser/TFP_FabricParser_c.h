/*
 * TFP_FabricParser_c: P a r s e r  H e a d e r 
 *
 * Generated from: TFP_FabricGrammar.g
 *
 * Terence Parr, Russell Quong, Will Cohen, and Hank Dietz: 1989-2001
 * Parr Research Corporation
 * with Purdue University Electrical Engineering
 * with AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 */

#ifndef TFP_FabricParser_c_h
#define TFP_FabricParser_c_h

#ifndef ANTLR_VERSION
#define ANTLR_VERSION 13333
#endif

#include "AParser.h"


#include <stdio.h>

#include "stdpccts.h"
#include "GenericTokenBuffer.h"

#include "TC_SideName.h"

#include "TFM_FabricModel.h"

#include "TFP_FabricFile.h"
#include "TFP_FabricScanner_c.h"
class TFP_FabricParser_c : public ANTLRParser {
public:
	static  const ANTLRChar *tokenName(int tk);
	enum { SET_SIZE = 62 };
protected:
	static const ANTLRChar *_token_tbl[];
private:

public:

   void syn( ANTLRAbstractToken* /* pToken */, 
ANTLRChar*          pszGroup,
SetWordType*        /* pWordType */,
ANTLRTokenType      tokenType,
int                 /* k */ );

   void SetInterface( TFP_FabricInterface_c* pinterface );
void SetScanner( TFP_FabricScanner_c* pscanner );
void SetFileName( const char* pszFileName );
void SetFabricFile( TFP_FabricFile_c* pfabricFile );
void SetFabricModel( TFM_FabricModel_c* pfabricModel );

private:

   TGS_OrientMode_t FindOrientMode_( ANTLRTokenType tokenType );
TC_SideMode_t FindSideMode_( ANTLRTokenType tokenType );

private:

   TFP_FabricInterface_c* pinterface_;
TFP_FabricScanner_c*   pscanner_;
string                 srFileName_;
TFP_FabricFile_c*      pfabricFile_;
TFM_FabricModel_c*     pfabricModel_;
protected:
	static SetWordType err1[8];
	static SetWordType err2[8];
	static SetWordType err3[8];
	static SetWordType err4[8];
	static SetWordType err5[8];
	static SetWordType err6[8];
	static SetWordType setwd1[62];
	static SetWordType err7[8];
	static SetWordType err8[8];
	static SetWordType err9[8];
	static SetWordType err10[8];
	static SetWordType err11[8];
	static SetWordType err12[8];
	static SetWordType err13[8];
	static SetWordType err14[8];
	static SetWordType setwd2[62];
	static SetWordType err15[8];
	static SetWordType err16[8];
	static SetWordType err17[8];
	static SetWordType err18[8];
	static SetWordType err19[8];
	static SetWordType err20[8];
	static SetWordType err21[8];
	static SetWordType setwd3[62];
	static SetWordType err22[8];
	static SetWordType err23[8];
	static SetWordType err24[8];
	static SetWordType err25[8];
	static SetWordType err26[8];
	static SetWordType err27[8];
	static SetWordType err28[8];
	static SetWordType setwd4[62];
	static SetWordType err29[8];
	static SetWordType err30[8];
	static SetWordType err31[8];
	static SetWordType err32[8];
	static SetWordType ORIENT_VAL_set[8];
	static SetWordType ORIENT_VAL_errset[8];
	static SetWordType err35[8];
	static SetWordType setwd5[62];
	static SetWordType err36[8];
	static SetWordType err37[8];
	static SetWordType err38[8];
	static SetWordType err39[8];
	static SetWordType err40[8];
	static SetWordType err41[8];
	static SetWordType setwd6[62];
	static SetWordType err42[8];
	static SetWordType SIDE_VAL_set[8];
	static SetWordType SIDE_VAL_errset[8];
	static SetWordType err45[8];
	static SetWordType err46[8];
	static SetWordType err47[8];
	static SetWordType setwd7[62];
	static SetWordType err48[8];
	static SetWordType err49[8];
	static SetWordType err50[8];
	static SetWordType err51[8];
	static SetWordType err52[8];
	static SetWordType setwd8[62];
private:
	void zzdflthandlers( int _signal, int *_retsignal );

public:
	TFP_FabricParser_c(ANTLRTokenBuffer *input);
	void start(void);
	void configDef(TFM_Config_c* pconfig);
	void inputOutputList(TFM_InputOutputList_t* pinputOutputList);
	void physicalBlockList(TFM_PhysicalBlockList_t* pphysicalBlockList);
	void switchBoxList(TFM_SwitchBoxList_t* pswitchBoxList);
	void channelList(TFM_ChannelList_t* pchannelList);
	void segmentList(TFM_SegmentList_t* psegmentList);
	void pinList(TFM_PinList_t* ppinList);
	void connectionPattern(TFM_BitPattern_t* pconnectionPattern);
	void sideMapTable(TC_MapTable_c* pmapTable);
	void sideMapIndex(TC_SideIndex_c* psideIndex);
	void regionDef(TGS_Region_c* pregion);
	void lineDef(TGS_Line_c* pline);
	void stringText(string* psrString);
	void floatNum(double* pdouble);
	void expNum(double* pdouble);
	void uintNum(unsigned int* puint);
};

#endif /* TFP_FabricParser_c_h */
