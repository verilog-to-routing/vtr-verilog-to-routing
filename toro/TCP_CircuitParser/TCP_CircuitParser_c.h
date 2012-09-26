/*
 * TCP_CircuitParser_c: P a r s e r  H e a d e r 
 *
 * Generated from: TCP_CircuitGrammar.g
 *
 * Terence Parr, Russell Quong, Will Cohen, and Hank Dietz: 1989-2001
 * Parr Research Corporation
 * with Purdue University Electrical Engineering
 * with AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 */

#ifndef TCP_CircuitParser_c_h
#define TCP_CircuitParser_c_h

#ifndef ANTLR_VERSION
#define ANTLR_VERSION 13333
#endif

#include "AParser.h"


#include <stdio.h>

#include "stdpccts.h"
#include "GenericTokenBuffer.h"

#include "TC_SideName.h"
#include "TC_StringUtils.h"

#include "TCD_CircuitDesign.h"

#include "TCP_CircuitFile.h"
#include "TCP_CircuitScanner_c.h"
class TCP_CircuitParser_c : public ANTLRParser {
public:
	static  const ANTLRChar *tokenName(int tk);
	enum { SET_SIZE = 69 };
protected:
	static const ANTLRChar *_token_tbl[];
private:

public:

   void syn( ANTLRAbstractToken* /* pToken */, 
ANTLRChar*          pszGroup,
SetWordType*        /* pWordType */,
ANTLRTokenType      tokenType,
int                 /* k */ );

   void SetScanner( TCP_CircuitScanner_c* pscanner );
void SetFileName( const char* pszFileName );
void SetCircuitFile( TCP_CircuitFile_c* pcircuitFile );
void SetCircuitDesign( TCD_CircuitDesign_c* pcircuitDesign );

private:

   TPO_StatusMode_t FindPlaceStatusMode_( ANTLRTokenType tokenType );
TNO_StatusMode_t FindNetStatusMode_( ANTLRTokenType tokenType );
TC_SideMode_t FindSideMode_( ANTLRTokenType tokenType );
TC_TypeMode_t FindTypeMode_( ANTLRTokenType tokenType );
bool FindBool_( ANTLRTokenType tokenType );

private:

   TCP_CircuitScanner_c*   pscanner_;
string                  srFileName_;
TCP_CircuitFile_c*      pcircuitFile_;
TCD_CircuitDesign_c*    pcircuitDesign_;
protected:
	static SetWordType err1[12];
	static SetWordType err2[12];
	static SetWordType err3[12];
	static SetWordType err4[12];
	static SetWordType PLACE_STATUS_VAL_set[12];
	static SetWordType PLACE_STATUS_VAL_errset[12];
	static SetWordType err7[12];
	static SetWordType err8[12];
	static SetWordType setwd1[69];
	static SetWordType err9[12];
	static SetWordType err10[12];
	static SetWordType err11[12];
	static SetWordType err12[12];
	static SetWordType err13[12];
	static SetWordType err14[12];
	static SetWordType err15[12];
	static SetWordType setwd2[69];
	static SetWordType err16[12];
	static SetWordType TYPE_VAL_set[12];
	static SetWordType TYPE_VAL_errset[12];
	static SetWordType err19[12];
	static SetWordType BOOL_VAL_set[12];
	static SetWordType BOOL_VAL_errset[12];
	static SetWordType err22[12];
	static SetWordType NET_STATUS_VAL_set[12];
	static SetWordType NET_STATUS_VAL_errset[12];
	static SetWordType err25[12];
	static SetWordType err26[12];
	static SetWordType setwd3[69];
	static SetWordType SIDE_VAL_set[12];
	static SetWordType SIDE_VAL_errset[12];
	static SetWordType setwd4[69];
	static SetWordType err29[12];
	static SetWordType err30[12];
	static SetWordType err31[12];
	static SetWordType err32[12];
	static SetWordType err33[12];
	static SetWordType setwd5[69];
private:
	void zzdflthandlers( int _signal, int *_retsignal );

public:
	TCP_CircuitParser_c(ANTLRTokenBuffer *input);
	void start(void);
	void blockList(TPO_InstList_t* pblockList);
	void portList(TPO_PortList_t* pportList);
	void instList(TPO_InstList_t* pinstList);
	void netList(TNO_NetList_c* pnetList);
	void pinDef(TPO_Pin_t* ppin);
	void instPinList(TNO_InstPinList_t* pinstPinList);
	void instPinDef(TNO_InstPin_c* pinstPin);
	void globalRouteList(TNO_GlobalRouteList_t* pglobalRouteList);
	void routeList(TNO_RouteList_t* prouteList);
	void switchBoxDef(TNO_SwitchBox_c* pswitchBox);
	void channelDef(TNO_Channel_t* pchannel);
	void hierMapList(TPO_HierMapList_t* phierMapList);
	void relativeList(TPO_RelativeList_t* prelativeList);
	void sideIndex(TC_SideIndex_c* psideIndex);
	void regionList(TGS_RegionList_t* pregionList);
	void regionDef(TGS_Region_c* pregion);
	void cellSourceText(string* psrString,TPO_InstSource_t* psource);
	void stringText(string* psrString);
	void floatNum(double* pdouble);
	void expNum(double* pdouble);
	void uintNum(unsigned int* puint);
};

#endif /* TCP_CircuitParser_c_h */
