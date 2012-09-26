/*
 * TAXP_ArchitectureXmlParser_c: P a r s e r  H e a d e r 
 *
 * Generated from: TAXP_ArchitectureXmlGrammar.g
 *
 * Terence Parr, Russell Quong, Will Cohen, and Hank Dietz: 1989-2001
 * Parr Research Corporation
 * with Purdue University Electrical Engineering
 * with AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 */

#ifndef TAXP_ArchitectureXmlParser_c_h
#define TAXP_ArchitectureXmlParser_c_h

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

#include "TAXP_ArchitectureXmlFile.h"
#include "TAXP_ArchitectureXmlScanner_c.h"
class TAXP_ArchitectureXmlParser_c : public ANTLRParser {
public:
	static  const ANTLRChar *tokenName(int tk);
	enum { SET_SIZE = 112 };
protected:
	static const ANTLRChar *_token_tbl[];
private:

public:

   void syn( ANTLRAbstractToken* /* pToken */, 
ANTLRChar*          pszGroup,
SetWordType*        /* pWordType */,
ANTLRTokenType      tokenType,
int                 /* k */ );

   void SetInterface( TAXP_ArchitectureXmlInterface_c* pinterface );
void SetScanner( TAXP_ArchitectureXmlScanner_c* pscanner );
void SetFileName( const char* pszFileName );
void SetArchitectureXmlFile( TAXP_ArchitectureXmlFile_c* parchitectureXmlFile );
void SetArchitectureSpec( TAS_ArchitectureSpec_c* parchitectureSpec );

private:

   TAXP_ArchitectureXmlInterface_c* pinterface_;
TAXP_ArchitectureXmlScanner_c*   pscanner_;
string                           srFileName_;
TAXP_ArchitectureXmlFile_c*      parchitectureXmlFile_;
TAS_ArchitectureSpec_c*          parchitectureSpec_;
protected:
	static SetWordType err1[16];
	static SetWordType err2[16];
	static SetWordType err3[16];
	static SetWordType err4[16];
	static SetWordType setwd1[112];
	static SetWordType err5[16];
	static SetWordType setwd2[112];
	static SetWordType err6[16];
	static SetWordType err7[16];
	static SetWordType setwd3[112];
	static SetWordType err8[16];
	static SetWordType err9[16];
	static SetWordType err10[16];
	static SetWordType err11[16];
	static SetWordType err12[16];
	static SetWordType err13[16];
	static SetWordType err14[16];
	static SetWordType err15[16];
	static SetWordType setwd4[112];
	static SetWordType err16[16];
	static SetWordType err17[16];
	static SetWordType setwd5[112];
	static SetWordType err18[16];
	static SetWordType err19[16];
	static SetWordType setwd6[112];
	static SetWordType err20[16];
	static SetWordType err21[16];
	static SetWordType setwd7[112];
	static SetWordType err22[16];
	static SetWordType setwd8[112];
	static SetWordType err23[16];
	static SetWordType err24[16];
	static SetWordType err25[16];
	static SetWordType err26[16];
	static SetWordType err27[16];
	static SetWordType err28[16];
	static SetWordType err29[16];
	static SetWordType err30[16];
	static SetWordType err31[16];
	static SetWordType setwd9[112];
	static SetWordType setwd10[112];
private:
	void zzdflthandlers( int _signal, int *_retsignal );

public:
	TAXP_ArchitectureXmlParser_c(ANTLRTokenBuffer *input);
	void start(void);
	void layoutDef(TAS_Config_c* pconfig);
	void deviceDef(TAS_Config_c* pconfig);
	void cellList(TAS_CellList_t* pcellList);
	void cellDef(TAS_CellList_t* pcellList);
	void switchBoxList(TAS_SwitchBoxList_t* pswitchBoxList);
	void switchBoxDef(TAS_SwitchBoxList_t* pswitchBoxList);
	void segmentList(TAS_SegmentList_t* psegmentList);
	void segmentDef(TAS_SegmentList_t* psegmentList);
	void complexBlockList(TAS_PhysicalBlockList_t* pphysicalBlockList);
	void pbtypeList(TAS_PhysicalBlockList_t* pphysicalBlockList);
	void pbtypeDef(TAS_PhysicalBlockList_t* pphysicalBlockList);
	void sbList(TAS_BitPattern_t* psbPattern);
	void cbList(TAS_BitPattern_t* pcbPattern);
	void inputPortList(TLO_PortList_t* pportList);
	void outputPortList(TLO_PortList_t* pportList);
	void fcDef(TAS_ConnectionFc_c* pfcIn,TAS_ConnectionFc_c* pfcOut);
	void modeDef(TAS_ModeList_t* pmodeList);
	void interconnectList(TAS_InterconnectList_t* pinterconnectList);
	void interconnectDef(TAS_InterconnectList_t* pinterconnectList);
	void interconnectElem(TAS_Interconnect_c* pinterconnect,TAS_InterconnectMapType_t mapType);
	void portDef(TLO_PortList_t* pportList);
	void portTypeDef(TLO_PortList_t* pportList,TC_TypeMode_t type);
	void timingDelayLists(TAS_TimingDelayLists_c* ptimingDelayLists);
	void delayMatrixDef(TAS_DelayMatrix_t* pdelayMatrix);
	void pinAssignList(TAS_PinAssignPatternType_t* ppinAssignPattern,TAS_PinAssignList_t* ppinAssignList);
	void pinAssignDef(TAS_PinAssignPatternType_t pinAssignPattern,TAS_PinAssignList_t* ppinAssignList);
	void gridAssignList(TAS_GridAssignList_t* pgridAssignList);
	void gridAssignDef(TAS_GridAssignList_t* pgridAssignList);
	void segmentLength(unsigned int* plength);
	void segmentDirType(TAS_SegmentDirType_t* pdirType);
	void switchBoxType(TAS_SwitchBoxType_t* ptype);
	void switchBoxModelType(TAS_SwitchBoxModelType_t* pmodelType);
	void channelWidth(TAS_ChannelWidth_c* pchannelWidthIO,TAS_ChannelWidth_c* pchannelWidthX,TAS_ChannelWidth_c* pchannelWidthY);
	void channelDistrMode(TAS_ChannelDistrMode_t* pmode);
	void connectionBoxInType(TAS_ConnectionBoxType_t* ptype);
	void connectionBoxOutType(TAS_ConnectionBoxType_t* ptype);
	void pinAssignPatternType(TAS_PinAssignPatternType_t* ppatternType);
	void gridAssignDistrMode(TAS_GridAssignDistrMode_t* pdistrMode);
	void sideMode(TC_SideMode_t* pmode);
	void classText(TAS_ClassType_t* ptype);
	void blifModelText(string* psrString,TAS_PhysicalBlockModelType_t* ptype);
	void stringText(string* psrString);
	void floatNum(double* pdouble);
	void expNum(double* pdouble);
	void intNum(int* pint);
	void uintNum(unsigned int* puint);
	void boolType(bool* pbool);
};

#endif /* TAXP_ArchitectureXmlParser_c_h */
