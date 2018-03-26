/**CFile****************************************************************

  FileName    [mio.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mio.h,v 1.6 2004/08/09 22:16:31 satrajit Exp $]

***********************************************************************/

#ifndef ABC__map__mio__mio_h
#define ABC__map__mio__mio_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef enum { MIO_PHASE_UNKNOWN, MIO_PHASE_INV, MIO_PHASE_NONINV } Mio_PinPhase_t;

typedef struct  Mio_LibraryStruct_t_      Mio_Library_t;
typedef struct  Mio_GateStruct_t_         Mio_Gate_t;
typedef struct  Mio_PinStruct_t_          Mio_Pin_t;

typedef struct Mio_Cell_t_ Mio_Cell_t; 
struct Mio_Cell_t_
{
    char *          pName;          // name
    unsigned        Id       : 28;  // gate ID
    unsigned        nFanins  :  4;  // gate fanins
    float           Area;           // area
    word            uTruth;         // truth table
    float           Delays[6];      // delay
};

typedef struct Mio_Cell2_t_ Mio_Cell2_t; 
struct Mio_Cell2_t_
{
    char *          pName;          // name
    Vec_Int_t *     vExpr;          // expression
    unsigned        Id       : 26;  // gate ID
    unsigned        Type     :  2;  // gate type
    unsigned        nFanins  :  4;  // gate fanins
    float           AreaF;          // area
    word            AreaW;          // area
    word            uTruth;         // truth table
    int             iDelayAve;      // average delay
    int             iDelays[6];     // delay
    void *          pMioGate;       // gate pointer
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////
 
#define Mio_LibraryForEachGate( Lib, Gate )                   \
    for ( Gate = Mio_LibraryReadGates(Lib);                   \
          Gate;                                               \
          Gate = Mio_GateReadNext(Gate) )
#define Mio_LibraryForEachGateSafe( Lib, Gate, Gate2 )        \
    for ( Gate = Mio_LibraryReadGates(Lib),                   \
          Gate2 = (Gate? Mio_GateReadNext(Gate): NULL);       \
          Gate;                                               \
          Gate = Gate2,                                       \
          Gate2 = (Gate? Mio_GateReadNext(Gate): NULL) )

#define Mio_GateForEachPin( Gate, Pin )                       \
    for ( Pin = Mio_GateReadPins(Gate);                       \
          Pin;                                                \
          Pin = Mio_PinReadNext(Pin) )
#define Mio_GateForEachPinSafe( Gate, Pin, Pin2 )             \
    for ( Pin = Mio_GateReadPins(Gate),                       \
          Pin2 = (Pin? Mio_PinReadNext(Pin): NULL);           \
          Pin;                                                \
          Pin = Pin2,                                         \
          Pin2 = (Pin? Mio_PinReadNext(Pin): NULL) )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== mio.c =============================================================*/
extern void              Mio_UpdateGenlib( Mio_Library_t * pLib );
extern int               Mio_UpdateGenlib2( Vec_Str_t * vStr, Vec_Str_t * vStr2, char * pFileName, int fVerbose );
/*=== mioApi.c =============================================================*/
extern char *            Mio_LibraryReadName       ( Mio_Library_t * pLib );
extern int               Mio_LibraryReadGateNum    ( Mio_Library_t * pLib );
extern Mio_Gate_t *      Mio_LibraryReadGates      ( Mio_Library_t * pLib );
extern Mio_Gate_t **     Mio_LibraryReadGateArray  ( Mio_Library_t * pLib );
extern Mio_Gate_t *      Mio_LibraryReadGateById   ( Mio_Library_t * pLib, int iD );
extern Mio_Gate_t *      Mio_LibraryReadGateByName ( Mio_Library_t * pLib, char * pName, char * pOutName );
extern char *            Mio_LibraryReadSopByName  ( Mio_Library_t * pLib, char * pName );    
extern Mio_Gate_t *      Mio_LibraryReadGateByTruth( Mio_Library_t * pLib, word t );
extern Mio_Gate_t *      Mio_LibraryReadConst0     ( Mio_Library_t * pLib );
extern Mio_Gate_t *      Mio_LibraryReadConst1     ( Mio_Library_t * pLib );
extern Mio_Gate_t *      Mio_LibraryReadNand2      ( Mio_Library_t * pLib );
extern Mio_Gate_t *      Mio_LibraryReadAnd2       ( Mio_Library_t * pLib );
extern Mio_Gate_t *      Mio_LibraryReadNor2       ( Mio_Library_t * pLib );
extern Mio_Gate_t *      Mio_LibraryReadOr2        ( Mio_Library_t * pLib );
extern Mio_Gate_t *      Mio_LibraryReadBuf        ( Mio_Library_t * pLib );
extern Mio_Gate_t *      Mio_LibraryReadInv        ( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayInvRise( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayInvFall( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayInvMax( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayNand2Rise( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayNand2Fall( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayNand2Max( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayAnd2Max( Mio_Library_t * pLib );
extern float             Mio_LibraryReadDelayAigNode( Mio_Library_t * pLib );
extern float             Mio_LibraryReadAreaInv    ( Mio_Library_t * pLib );
extern float             Mio_LibraryReadAreaBuf    ( Mio_Library_t * pLib );
extern float             Mio_LibraryReadAreaNand2  ( Mio_Library_t * pLib );
extern int               Mio_LibraryReadGateNameMax( Mio_Library_t * pLib );
extern void              Mio_LibrarySetName        ( Mio_Library_t * pLib, char * pName );
extern char *            Mio_GateReadName          ( Mio_Gate_t * pGate );      
extern char *            Mio_GateReadOutName       ( Mio_Gate_t * pGate );      
extern double            Mio_GateReadArea          ( Mio_Gate_t * pGate );      
extern char *            Mio_GateReadForm          ( Mio_Gate_t * pGate );      
extern Mio_Pin_t *       Mio_GateReadPins          ( Mio_Gate_t * pGate );      
extern Mio_Library_t *   Mio_GateReadLib           ( Mio_Gate_t * pGate );      
extern Mio_Gate_t *      Mio_GateReadNext          ( Mio_Gate_t * pGate );      
extern Mio_Gate_t *      Mio_GateReadTwin          ( Mio_Gate_t * pGate );      
extern int               Mio_GateReadPinNum        ( Mio_Gate_t * pGate );      
extern double            Mio_GateReadDelayMax      ( Mio_Gate_t * pGate );      
extern char *            Mio_GateReadSop           ( Mio_Gate_t * pGate );      
extern Vec_Int_t *       Mio_GateReadExpr          ( Mio_Gate_t * pGate );      
extern word              Mio_GateReadTruth         ( Mio_Gate_t * pGate );
extern word *            Mio_GateReadTruthP        ( Mio_Gate_t * pGate );
extern int               Mio_GateReadValue         ( Mio_Gate_t * pGate );
extern int               Mio_GateReadCell          ( Mio_Gate_t * pGate );
extern int               Mio_GateReadProfile       ( Mio_Gate_t * pGate );
extern int               Mio_GateReadProfile2      ( Mio_Gate_t * pGate );
extern char *            Mio_GateReadPinName       ( Mio_Gate_t * pGate, int iPin );
extern float             Mio_GateReadPinDelay      ( Mio_Gate_t * pGate, int iPin );
extern void              Mio_GateSetValue          ( Mio_Gate_t * pGate, int Value );
extern void              Mio_GateSetCell           ( Mio_Gate_t * pGate, int Cell );
extern void              Mio_GateSetProfile        ( Mio_Gate_t * pGate, int Prof );
extern void              Mio_GateSetProfile2       ( Mio_Gate_t * pGate, int Prof );
extern void              Mio_GateIncProfile2       ( Mio_Gate_t * pGate );
extern void              Mio_GateDecProfile2       ( Mio_Gate_t * pGate );
extern void              Mio_GateAddToProfile      ( Mio_Gate_t * pGate, int Prof );
extern void              Mio_GateAddToProfile2     ( Mio_Gate_t * pGate, int Prof );
extern int               Mio_GateIsInv             ( Mio_Gate_t * pGate );
extern char *            Mio_PinReadName           ( Mio_Pin_t * pPin );  
extern Mio_PinPhase_t    Mio_PinReadPhase          ( Mio_Pin_t * pPin );  
extern double            Mio_PinReadInputLoad      ( Mio_Pin_t * pPin );  
extern double            Mio_PinReadMaxLoad        ( Mio_Pin_t * pPin );  
extern double            Mio_PinReadDelayBlockRise ( Mio_Pin_t * pPin );  
extern double            Mio_PinReadDelayFanoutRise( Mio_Pin_t * pPin );  
extern double            Mio_PinReadDelayBlockFall ( Mio_Pin_t * pPin );  
extern double            Mio_PinReadDelayFanoutFall( Mio_Pin_t * pPin );  
extern double            Mio_PinReadDelayBlockMax  ( Mio_Pin_t * pPin );  
extern Mio_Pin_t *       Mio_PinReadNext           ( Mio_Pin_t * pPin );  
/*=== mioRead.c =============================================================*/
extern char *            Mio_ReadFile( char * FileName, int fAddEnd );
extern Mio_Library_t *   Mio_LibraryRead( char * FileName, char * pBuffer, char * ExcludeFile, int fVerbose );
extern int               Mio_LibraryReadExclude( char * ExcludeFile, st__table * tExcludeGate );
/*=== mioFunc.c =============================================================*/
extern int               Mio_LibraryParseFormulas( Mio_Library_t * pLib );
/*=== mioParse.c =============================================================*/
extern Vec_Int_t *       Mio_ParseFormula( char * pFormInit, char ** ppVarNames, int nVars );
extern Vec_Wrd_t *       Mio_ParseFormulaTruth( char * pFormInit, char ** ppVarNames, int nVars );
extern int               Mio_ParseCheckFormula( Mio_Gate_t * pGate, char * pForm );
/*=== mioSop.c =============================================================*/
extern char *            Mio_LibDeriveSop( int nVars, Vec_Int_t * vExpr, Vec_Str_t * vStr );
/*=== mioUtils.c =============================================================*/
extern void              Mio_LibraryDelete( Mio_Library_t * pLib );
extern void              Mio_GateDelete( Mio_Gate_t * pGate );
extern void              Mio_PinDelete( Mio_Pin_t * pPin );
extern Mio_Pin_t *       Mio_PinDup( Mio_Pin_t * pPin );
extern void              Mio_WriteLibrary( FILE * pFile, Mio_Library_t * pLib, int fPrintSops, int fShort, int fSelected );
extern Mio_Gate_t **     Mio_CollectRoots( Mio_Library_t * pLib, int nInputs, float tDelay, int fSkipInv, int * pnGates, int fVerbose );
extern Mio_Cell_t *      Mio_CollectRootsNew( Mio_Library_t * pLib, int nInputs, int * pnGates, int fVerbose );
extern Mio_Cell_t *      Mio_CollectRootsNewDefault( int nInputs, int * pnGates, int fVerbose );
extern Mio_Cell2_t *     Mio_CollectRootsNewDefault2( int nInputs, int * pnGates, int fVerbose );
extern int               Mio_CollectRootsNewDefault3( int nInputs, Vec_Ptr_t ** pvNames, Vec_Wrd_t ** pvTruths );
extern word              Mio_DeriveTruthTable6( Mio_Gate_t * pGate );
extern void              Mio_DeriveTruthTable( Mio_Gate_t * pGate, unsigned uTruthsIn[][2], int nSigns, int nInputs, unsigned uTruthRes[] );
extern void              Mio_DeriveGateDelays( Mio_Gate_t * pGate, 
                            float ** ptPinDelays, int nPins, int nInputs, float tDelayZero, 
                            float * ptDelaysRes, float * ptPinDelayMax );
extern Mio_Gate_t *      Mio_GateCreatePseudo( int nInputs );
extern void              Mio_LibraryShiftDelay( Mio_Library_t * pLib, double Shift );
extern void              Mio_LibraryMultiArea( Mio_Library_t * pLib, double Multi );
extern void              Mio_LibraryMultiDelay( Mio_Library_t * pLib, double Multi );
extern void              Mio_LibraryTransferDelays( Mio_Library_t * pLibD, Mio_Library_t * pLibS );
extern void              Mio_LibraryTransferCellIds();
extern void              Mio_LibraryReadProfile( FILE * pFile, Mio_Library_t * pLib );
extern void              Mio_LibraryWriteProfile( FILE * pFile, Mio_Library_t * pLib );
extern void              Mio_LibraryTransferProfile( Mio_Library_t * pLibDst, Mio_Library_t * pLibSrc );
extern void              Mio_LibraryTransferProfile2( Mio_Library_t * pLibDst, Mio_Library_t * pLibSrc );
extern int               Mio_LibraryHasProfile( Mio_Library_t * pLib );
extern void              Mio_LibraryCleanProfile2( Mio_Library_t * pLib );
extern void              Mio_LibraryShortNames( Mio_Library_t * pLib );

extern void              Mio_LibraryMatchesStop( Mio_Library_t * pLib );
extern void              Mio_LibraryMatchesStart( Mio_Library_t * pLib, int fPinFilter, int fPinPerm, int fPinQuick );
extern void              Mio_LibraryMatchesFetch( Mio_Library_t * pLib, Vec_Mem_t ** pvTtMem, Vec_Wec_t ** pvTt2Match, Mio_Cell2_t ** ppCells, int * pnCells, int fPinFilter, int fPinPerm, int fPinQuick );

extern void              Mio_LibraryMatches2Stop( Mio_Library_t * pLib );
extern void              Mio_LibraryMatches2Start( Mio_Library_t * pLib );
extern void              Mio_LibraryMatches2Fetch( Mio_Library_t * pLib, Vec_Ptr_t ** pvNames, Vec_Wrd_t ** pvTruths, Vec_Int_t ** pvTt2Match4, Vec_Int_t ** pvConfigs, Vec_Mem_t * pvTtMem2[3], Vec_Int_t * pvTt2Match2[3] );

/*=== sclUtil.c =========================================================*/
extern Mio_Library_t *   Abc_SclDeriveGenlibSimple( void * pScl );
extern Mio_Library_t *   Abc_SclDeriveGenlib( void * pScl, void * pMio, float Slew, float Gain, int nGatesMin, int fVerbose );
extern int               Abc_SclHasDelayInfo( void * pScl );


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

