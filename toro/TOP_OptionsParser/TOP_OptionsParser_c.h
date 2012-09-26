/*
 * TOP_OptionsParser_c: P a r s e r  H e a d e r 
 *
 * Generated from: TOP_OptionsGrammar.g
 *
 * Terence Parr, Russell Quong, Will Cohen, and Hank Dietz: 1989-2001
 * Parr Research Corporation
 * with Purdue University Electrical Engineering
 * with AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 */

#ifndef TOP_OptionsParser_c_h
#define TOP_OptionsParser_c_h

#ifndef ANTLR_VERSION
#define ANTLR_VERSION 13333
#endif

#include "AParser.h"


#include <stdio.h>

#include "stdpccts.h"
#include "GenericTokenBuffer.h"

#include "TIO_PrintHandler.h"

#include "TOS_OptionsStore.h"

#include "TOP_OptionsFile.h"
#include "TOP_OptionsScanner_c.h"
class TOP_OptionsParser_c : public ANTLRParser {
public:
	static  const ANTLRChar *tokenName(int tk);
	enum { SET_SIZE = 158 };
protected:
	static const ANTLRChar *_token_tbl[];
private:

public:

   void syn( ANTLRAbstractToken* /* pToken */, 
ANTLRChar*          pszGroup,
SetWordType*        /* pWordType */,
ANTLRTokenType      tokenType,
int                 /* k */ );

   void SetScanner( TOP_OptionsScanner_c* pscanner );
void SetFileName( const char* pszFileName );
void SetOptionsFile( TOP_OptionsFile_c* poptionsFile );
void SetOptionsStore( TOS_OptionsStore_c* poptionsStore );

private:

   TOS_InputDataMode_t FindInputDataMode_( ANTLRTokenType tokenType );
int FindOutputLaffMask_( ANTLRTokenType tokenType );
int FindExecuteToolMask_( ANTLRTokenType tokenType );

   bool FindBool_( ANTLRTokenType tokenType );

private:

   TOP_OptionsScanner_c* pscanner_;
string                srFileName_;
TOP_OptionsFile_c*    poptionsFile_;

   TOS_InputOptions_c*   pinputOptions_;
TOS_OutputOptions_c*  poutputOptions_;
TOS_MessageOptions_c* pmessageOptions_;
TOS_ExecuteOptions_c* pexecuteOptions_;
TOS_PackOptions_c*    ppackOptions_;
TOS_PlaceOptions_c*   pplaceOptions_;
TOS_RouteOptions_c*   prouteOptions_;

   string srActiveCmd_;
protected:
	static SetWordType setwd1[158];
	static SetWordType err1[20];
	static SetWordType err2[20];
	static SetWordType err3[20];
	static SetWordType err4[20];
	static SetWordType err5[20];
	static SetWordType err6[20];
	static SetWordType err7[20];
	static SetWordType setwd2[158];
	static SetWordType err8[20];
	static SetWordType err9[20];
	static SetWordType err10[20];
	static SetWordType err11[20];
	static SetWordType err12[20];
	static SetWordType err13[20];
	static SetWordType err14[20];
	static SetWordType err15[20];
	static SetWordType setwd3[158];
	static SetWordType err16[20];
	static SetWordType err17[20];
	static SetWordType err18[20];
	static SetWordType err19[20];
	static SetWordType err20[20];
	static SetWordType err21[20];
	static SetWordType err22[20];
	static SetWordType err23[20];
	static SetWordType setwd4[158];
	static SetWordType err24[20];
	static SetWordType err25[20];
	static SetWordType err26[20];
	static SetWordType err27[20];
	static SetWordType err28[20];
	static SetWordType err29[20];
	static SetWordType err30[20];
	static SetWordType err31[20];
	static SetWordType setwd5[158];
	static SetWordType err32[20];
	static SetWordType err33[20];
	static SetWordType err34[20];
	static SetWordType err35[20];
	static SetWordType err36[20];
	static SetWordType err37[20];
	static SetWordType err38[20];
	static SetWordType err39[20];
	static SetWordType setwd6[158];
	static SetWordType err40[20];
	static SetWordType err41[20];
	static SetWordType err42[20];
	static SetWordType err43[20];
	static SetWordType err44[20];
	static SetWordType err45[20];
	static SetWordType err46[20];
	static SetWordType err47[20];
	static SetWordType setwd7[158];
	static SetWordType err48[20];
	static SetWordType err49[20];
	static SetWordType err50[20];
	static SetWordType err51[20];
	static SetWordType err52[20];
	static SetWordType err53[20];
	static SetWordType err54[20];
	static SetWordType err55[20];
	static SetWordType setwd8[158];
	static SetWordType err56[20];
	static SetWordType err57[20];
	static SetWordType err58[20];
	static SetWordType err59[20];
	static SetWordType err60[20];
	static SetWordType setwd9[158];
	static SetWordType err61[20];
	static SetWordType err62[20];
	static SetWordType err63[20];
	static SetWordType err64[20];
	static SetWordType err65[20];
	static SetWordType err66[20];
	static SetWordType err67[20];
	static SetWordType err68[20];
	static SetWordType err69[20];
	static SetWordType err70[20];
	static SetWordType setwd10[158];
	static SetWordType err71[20];
	static SetWordType err72[20];
	static SetWordType err73[20];
	static SetWordType err74[20];
	static SetWordType err75[20];
	static SetWordType err76[20];
	static SetWordType err77[20];
	static SetWordType err78[20];
	static SetWordType err79[20];
	static SetWordType err80[20];
	static SetWordType err81[20];
	static SetWordType err82[20];
	static SetWordType setwd11[158];
	static SetWordType err83[20];
	static SetWordType err84[20];
	static SetWordType err85[20];
	static SetWordType err86[20];
	static SetWordType err87[20];
	static SetWordType err88[20];
	static SetWordType err89[20];
	static SetWordType err90[20];
	static SetWordType err91[20];
	static SetWordType err92[20];
	static SetWordType setwd12[158];
	static SetWordType err93[20];
	static SetWordType err94[20];
	static SetWordType err95[20];
	static SetWordType err96[20];
	static SetWordType err97[20];
	static SetWordType err98[20];
	static SetWordType err99[20];
	static SetWordType err100[20];
	static SetWordType err101[20];
	static SetWordType err102[20];
	static SetWordType err103[20];
	static SetWordType err104[20];
	static SetWordType setwd13[158];
	static SetWordType err105[20];
	static SetWordType err106[20];
	static SetWordType err107[20];
	static SetWordType err108[20];
	static SetWordType err109[20];
	static SetWordType err110[20];
	static SetWordType INPUT_DATA_VAL_set[20];
	static SetWordType INPUT_DATA_VAL_errset[20];
	static SetWordType err113[20];
	static SetWordType setwd14[158];
	static SetWordType OUTPUT_LAFF_VAL_set[20];
	static SetWordType OUTPUT_LAFF_VAL_errset[20];
	static SetWordType err116[20];
	static SetWordType err117[20];
	static SetWordType err118[20];
	static SetWordType err119[20];
	static SetWordType setwd15[158];
	static SetWordType err120[20];
	static SetWordType EXECUTE_TOOL_VAL_set[20];
	static SetWordType EXECUTE_TOOL_VAL_errset[20];
	static SetWordType err123[20];
	static SetWordType err124[20];
	static SetWordType setwd16[158];
	static SetWordType err125[20];
	static SetWordType err126[20];
	static SetWordType err127[20];
	static SetWordType err128[20];
	static SetWordType err129[20];
	static SetWordType err130[20];
	static SetWordType setwd17[158];
	static SetWordType err131[20];
	static SetWordType err132[20];
	static SetWordType err133[20];
	static SetWordType err134[20];
	static SetWordType err135[20];
	static SetWordType err136[20];
	static SetWordType err137[20];
	static SetWordType BOOL_set[20];
	static SetWordType BOOL_errset[20];
	static SetWordType setwd18[158];
private:
	void zzdflthandlers( int _signal, int *_retsignal );

public:
	TOP_OptionsParser_c(ANTLRTokenBuffer *input);
	void start(void);
	void inputOptions(void);
	void outputOptions(void);
	void messageOptions(void);
	void executeOptions(void);
	void packOptions(void);
	void placeOptions(void);
	void routeOptions(void);
	void inputDataMode(TOS_InputDataMode_t* pmode);
	void outputLaffMask(int* pmask);
	void rcDelaysExtractMode(TOS_RcDelaysExtractMode_t* pmode);
	void rcDelaysSortMode(TOS_RcDelaysSortMode_t* pmode);
	void rcDelaysNameList(TOS_RcDelaysNameList_t* pnameList);
	void displayNameList(TOS_DisplayNameList_t* pnameList);
	void executeToolMask(int* pmask);
	void packAlgorithmMode(TOS_PackAlgorithmMode_t* pmode);
	void packClusterNetsMode(TOS_PackClusterNetsMode_t* pmode);
	void packAffinityMode(TOS_PackAffinityMode_t* pmode);
	void packCostMode(TOS_PackCostMode_t* pmode);
	void placeAlgorithmMode(TOS_PlaceAlgorithmMode_t* pmode);
	void placeCostMode(TOS_PlaceCostMode_t* pmode);
	void routeAlgorithmMode(TOS_RouteAlgorithmMode_t* pmode);
	void routeAbstractMode(TOS_RouteAbstractMode_t* pmode);
	void routeResourceMode(TOS_RouteResourceMode_t* pmode);
	void routeCostMode(TOS_RouteCostMode_t* pmode);
	void name(TC_Name_c& srName);
	void stringText(string* psrString);
	void floatNum(double* pdouble);
	void intNum(int* pint);
	void uintNum(unsigned int* puint);
	void ulongNum(unsigned long* plong);
	void boolType(bool* pbool);
};

#endif /* TOP_OptionsParser_c_h */
