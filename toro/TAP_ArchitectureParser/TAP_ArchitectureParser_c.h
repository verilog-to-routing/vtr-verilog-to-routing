/*
 * TAP_ArchitectureParser_c: P a r s e r  H e a d e r 
 *
 * Generated from: TAP_ArchitectureGrammar.g
 *
 * Terence Parr, Russell Quong, Will Cohen, and Hank Dietz: 1989-2001
 * Parr Research Corporation
 * with Purdue University Electrical Engineering
 * with AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 */

#ifndef TAP_ArchitectureParser_c_h
#define TAP_ArchitectureParser_c_h

#ifndef ANTLR_VERSION
#define ANTLR_VERSION 13333
#endif

#include "AParser.h"


#include <stdio.h>

#include "stdpccts.h"
#include "GenericTokenBuffer.h"

#include "TC_StringUtils.h"

#include "TIO_PrintHandler.h"

#include "TAS_ArchitectureSpec.h"

#include "TAP_ArchitectureFile.h"
#include "TAP_ArchitectureScanner_c.h"
class TAP_ArchitectureParser_c : public ANTLRParser {
public:
	static  const ANTLRChar *tokenName(int tk);
	enum { SET_SIZE = 127 };
protected:
	static const ANTLRChar *_token_tbl[];
private:

public:

   void syn( ANTLRAbstractToken* /* pToken */, 
ANTLRChar*          pszGroup,
SetWordType*        /* pWordType */,
ANTLRTokenType      tokenType,
int                 /* k */ );

   void SetInterface( TAP_ArchitectureInterface_c* pinterface );
void SetScanner( TAP_ArchitectureScanner_c* pscanner );
void SetFileName( const char* pszFileName );
void SetArchitectureFile( TAP_ArchitectureFile_c* parchitectureFile );
void SetArchitectureSpec( TAS_ArchitectureSpec_c* parchitectureSpec );

private:

   TAS_ClassType_t FindClassType_( ANTLRTokenType tokenType );
TAS_ArraySizeMode_t FindArraySizeMode_( ANTLRTokenType tokenType );
TAS_SwitchBoxType_t FindSwitchBoxType_( ANTLRTokenType tokenType );
TAS_SwitchBoxModelType_t FindSwitchBoxModelType_( ANTLRTokenType tokenType );
TAS_SegmentDirType_t FindSegmentDirType_( ANTLRTokenType tokenType );
TAS_PinAssignPatternType_t FindPinAssignPatternType_( ANTLRTokenType tokenType );
TAS_GridAssignDistrMode_t FindGridAssignDistrMode_( ANTLRTokenType tokenType );
TAS_GridAssignOrientMode_t FindGridAssignOrientMode_( ANTLRTokenType tokenType );
TAS_InterconnectMapType_t FindInterconnectMapType_( ANTLRTokenType tokenType );
TC_SideMode_t FindSideMode_( ANTLRTokenType tokenType );
TC_TypeMode_t FindTypeMode_( ANTLRTokenType tokenType );
bool FindBool_( ANTLRTokenType tokenType );

private:

   TAP_ArchitectureInterface_c* pinterface_;
TAP_ArchitectureScanner_c*   pscanner_;
string                       srFileName_;
TAP_ArchitectureFile_c*      parchitectureFile_;
TAS_ArchitectureSpec_c*      parchitectureSpec_;
protected:
	static SetWordType err1[16];
	static SetWordType ARRAY_SIZE_MODE_VAL_set[16];
	static SetWordType ARRAY_SIZE_MODE_VAL_errset[16];
	static SetWordType err4[16];
	static SetWordType err5[16];
	static SetWordType err6[16];
	static SetWordType SB_MODEL_TYPE_VAL_set[16];
	static SetWordType SB_MODEL_TYPE_VAL_errset[16];
	static SetWordType err9[16];
	static SetWordType err10[16];
	static SetWordType setwd1[127];
	static SetWordType err11[16];
	static SetWordType err12[16];
	static SetWordType SEGMENT_DIR_TYPE_VAL_set[16];
	static SetWordType SEGMENT_DIR_TYPE_VAL_errset[16];
	static SetWordType err15[16];
	static SetWordType err16[16];
	static SetWordType err17[16];
	static SetWordType err18[16];
	static SetWordType err19[16];
	static SetWordType setwd2[127];
	static SetWordType err20[16];
	static SetWordType err21[16];
	static SetWordType err22[16];
	static SetWordType err23[16];
	static SetWordType err24[16];
	static SetWordType err25[16];
	static SetWordType err26[16];
	static SetWordType setwd3[127];
	static SetWordType err27[16];
	static SetWordType err28[16];
	static SetWordType MAP_TYPE_VAL_set[16];
	static SetWordType MAP_TYPE_VAL_errset[16];
	static SetWordType err31[16];
	static SetWordType err32[16];
	static SetWordType err33[16];
	static SetWordType SB_TYPE_VAL_set[16];
	static SetWordType SB_TYPE_VAL_errset[16];
	static SetWordType err36[16];
	static SetWordType err37[16];
	static SetWordType err38[16];
	static SetWordType setwd4[127];
	static SetWordType err39[16];
	static SetWordType err40[16];
	static SetWordType err41[16];
	static SetWordType err42[16];
	static SetWordType err43[16];
	static SetWordType err44[16];
	static SetWordType err45[16];
	static SetWordType setwd5[127];
	static SetWordType err46[16];
	static SetWordType err47[16];
	static SetWordType err48[16];
	static SetWordType err49[16];
	static SetWordType err50[16];
	static SetWordType err51[16];
	static SetWordType setwd6[127];
	static SetWordType err52[16];
	static SetWordType CLASS_TYPE_VAL_set[16];
	static SetWordType CLASS_TYPE_VAL_errset[16];
	static SetWordType TYPE_VAL_set[16];
	static SetWordType TYPE_VAL_errset[16];
	static SetWordType err57[16];
	static SetWordType err58[16];
	static SetWordType err59[16];
	static SetWordType BOOL_VAL_set[16];
	static SetWordType BOOL_VAL_errset[16];
	static SetWordType setwd7[127];
	static SetWordType err62[16];
	static SetWordType err63[16];
	static SetWordType err64[16];
	static SetWordType err65[16];
	static SetWordType err66[16];
	static SetWordType setwd8[127];
	static SetWordType err67[16];
	static SetWordType err68[16];
	static SetWordType err69[16];
	static SetWordType err70[16];
	static SetWordType err71[16];
	static SetWordType err72[16];
	static SetWordType err73[16];
	static SetWordType setwd9[127];
	static SetWordType err74[16];
	static SetWordType err75[16];
	static SetWordType err76[16];
	static SetWordType err77[16];
	static SetWordType err78[16];
	static SetWordType err79[16];
	static SetWordType setwd10[127];
	static SetWordType err80[16];
	static SetWordType err81[16];
	static SetWordType err82[16];
	static SetWordType PIN_PATTERN_TYPE_VAL_set[16];
	static SetWordType PIN_PATTERN_TYPE_VAL_errset[16];
	static SetWordType err85[16];
	static SetWordType SIDE_VAL_set[16];
	static SetWordType SIDE_VAL_errset[16];
	static SetWordType setwd11[127];
	static SetWordType err88[16];
	static SetWordType GRID_DISTR_MODE_VAL_set[16];
	static SetWordType GRID_DISTR_MODE_VAL_errset[16];
	static SetWordType err91[16];
	static SetWordType GRID_ORIENT_MODE_VAL_set[16];
	static SetWordType GRID_ORIENT_MODE_VAL_errset[16];
	static SetWordType err94[16];
	static SetWordType err95[16];
	static SetWordType err96[16];
	static SetWordType setwd12[127];
	static SetWordType err97[16];
	static SetWordType err98[16];
	static SetWordType setwd13[127];
	static SetWordType err99[16];
	static SetWordType err100[16];
	static SetWordType err101[16];
	static SetWordType err102[16];
	static SetWordType err103[16];
	static SetWordType err104[16];
	static SetWordType err105[16];
	static SetWordType err106[16];
	static SetWordType err107[16];
	static SetWordType setwd14[127];
private:
	void zzdflthandlers( int _signal, int *_retsignal );

public:
	TAP_ArchitectureParser_c(ANTLRTokenBuffer *input);
	void start(void);
	void configDef(TAS_Config_c* pconfig);
	void inputOutputList(TAS_InputOutputList_t* pinputOutputList);
	void physicalBlockList(TAS_PhysicalBlockList_t* pphysicalBlockList);
	void modeList(TAS_ModeList_t* pmodeList);
	void switchBoxList(TAS_SwitchBoxList_t* pswitchBoxList);
	void segmentList(TAS_SegmentList_t* psegmentList);
	void cellList(TAS_CellList_t* pcellList);
	void fcDef(TAS_ConnectionFc_c* pfc);
	void modeNameList(TAS_ModeNameList_t* pmodeNameList);
	void pinList(TLO_PortList_t* pportList);
	void timingDelayLists(TAS_TimingDelayLists_c* ptimingDelayLists);
	void delayMatrixDef(TAS_DelayMatrix_t* pdelayMatrix);
	void cellModelText(string* psrString,TAS_PhysicalBlockModelType_t* ptype);
	void pinAssignList(TAS_PinAssignPatternType_t* ppinAssignPattern,TAS_PinAssignList_t* ppinAssignList);
	void gridAssignList(TAS_GridAssignList_t* pgridAssignList);
	void mapSideTable(unsigned int Fs,TAS_MapTable_t* pmapTable);
	void mapSideIndex(TC_SideIndex_c* psideIndex);
	void segmentLength(unsigned int * plength);
	void floatDims(TC_FloatDims_t* pfloatDims);
	void originPoint(TGS_Point_c* poriginPoint);
	void stringText(string* psrString);
	void floatNum(double* pdouble);
	void expNum(double* pdouble);
	void intNum(int* pint);
	void uintNum(unsigned int* puint);
	void boolType(bool* pbool);
};

#endif /* TAP_ArchitectureParser_c_h */
