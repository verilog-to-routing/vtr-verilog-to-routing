/**CFile****************************************************************

  FileName    [mapper.h]
 
  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapper.h,v 1.11 2005/02/28 05:34:26 alanmi Exp $]

***********************************************************************/

#ifndef ABC__map__mapper__mapper_h
#define ABC__map__mapper__mapper_h


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

typedef struct Map_ManStruct_t_         Map_Man_t;
typedef struct Map_NodeStruct_t_        Map_Node_t;
typedef struct Map_NodeVecStruct_t_     Map_NodeVec_t;
typedef struct Map_CutStruct_t_         Map_Cut_t;
typedef struct Map_MatchStruct_t_       Map_Match_t;
typedef struct Map_SuperStruct_t_       Map_Super_t;
typedef struct Map_SuperLibStruct_t_    Map_SuperLib_t;
typedef struct Map_HashTableStruct_t_   Map_HashTable_t;
typedef struct Map_HashEntryStruct_t_   Map_HashEntry_t;
typedef struct Map_TimeStruct_t_        Map_Time_t;

// the pair of rise/fall time parameters
struct Map_TimeStruct_t_
{
    float              Rise;
    float              Fall;
    float              Worst;
};

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////
 
#define Map_IsComplement(p)    (((int)((ABC_PTRUINT_T) (p) & 01)))
#define Map_Regular(p)         ((Map_Node_t *)((ABC_PTRUINT_T)(p) & ~01)) 
#define Map_Not(p)             ((Map_Node_t *)((ABC_PTRUINT_T)(p) ^ 01)) 
#define Map_NotCond(p,c)       ((Map_Node_t *)((ABC_PTRUINT_T)(p) ^ (c)))

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== mapperCreate.c =============================================================*/
extern Map_Man_t *     Map_ManCreate( int nInputs, int nOutputs, int fVerbose );
extern Map_Node_t *    Map_NodeCreate( Map_Man_t * p, Map_Node_t * p1, Map_Node_t * p2 );
extern void            Map_ManCreateNodeDelays( Map_Man_t * p, int LogFan );
extern void            Map_ManFree( Map_Man_t * pMan );
extern void            Map_ManPrintTimeStats( Map_Man_t * p );
extern void            Map_ManPrintStatsToFile( char * pName, float Area, float Delay, abctime Time );
extern int             Map_ManReadInputNum( Map_Man_t * p );
extern int             Map_ManReadOutputNum( Map_Man_t * p );
extern int             Map_ManReadBufNum( Map_Man_t * p );
extern Map_Node_t **   Map_ManReadInputs ( Map_Man_t * p );
extern Map_Node_t **   Map_ManReadOutputs( Map_Man_t * p );
extern Map_Node_t **   Map_ManReadBufs( Map_Man_t * p );
extern Map_Node_t *    Map_ManReadBufDriver( Map_Man_t * p, int i );
extern Map_Node_t *    Map_ManReadConst1 ( Map_Man_t * p );
extern Map_Time_t *    Map_ManReadInputArrivals( Map_Man_t * p );
extern Mio_Library_t * Map_ManReadGenLib ( Map_Man_t * p );
extern int             Map_ManReadVerbose( Map_Man_t * p );
extern float           Map_ManReadAreaFinal( Map_Man_t * p );
extern float           Map_ManReadRequiredGlo( Map_Man_t * p );
extern void            Map_ManSetOutputNames( Map_Man_t * p, char ** ppNames );
extern void            Map_ManSetAreaRecovery( Map_Man_t * p, int fAreaRecovery );
extern void            Map_ManSetDelayTarget( Map_Man_t * p, float DelayTarget );
extern void            Map_ManSetInputArrivals( Map_Man_t * p, Map_Time_t * pArrivals );
extern void            Map_ManSetOutputRequireds( Map_Man_t * p, Map_Time_t * pArrivals );
extern void            Map_ManSetObeyFanoutLimits( Map_Man_t * p, int  fObeyFanoutLimits );              
extern void            Map_ManSetNumIterations( Map_Man_t * p, int nNumIterations );
extern int             Map_ManReadPass( Map_Man_t * p );
extern void            Map_ManSetPass( Map_Man_t * p, int nPass );
extern int             Map_ManReadFanoutViolations( Map_Man_t * p );
extern void            Map_ManSetFanoutViolations( Map_Man_t * p, int nVio );
extern void            Map_ManSetChoiceNodeNum( Map_Man_t * p, int nChoiceNodes );
extern void            Map_ManSetChoiceNum( Map_Man_t * p, int nChoices );
extern void            Map_ManSetVerbose( Map_Man_t * p, int fVerbose );
extern void            Map_ManSetSwitching( Map_Man_t * p, int fSwitching );
extern void            Map_ManSetSkipFanout( Map_Man_t * p, int fSkipFanout );
extern void            Map_ManSetUseProfile( Map_Man_t * p );
extern void            Map_ManCreateAigIds( Map_Man_t * p, int nObjs );   

extern Map_Man_t *     Map_NodeReadMan( Map_Node_t * p );
extern char *          Map_NodeReadData( Map_Node_t * p, int fPhase );
extern int             Map_NodeReadNum( Map_Node_t * p );
extern int             Map_NodeReadLevel( Map_Node_t * p );
extern int             Map_NodeReadAigId( Map_Node_t * p );
extern Map_Cut_t *     Map_NodeReadCuts( Map_Node_t * p );
extern Map_Cut_t *     Map_NodeReadCutBest( Map_Node_t * p, int fPhase );
extern Map_Node_t *    Map_NodeReadOne( Map_Node_t * p );
extern Map_Node_t *    Map_NodeReadTwo( Map_Node_t * p );
extern void            Map_NodeSetData( Map_Node_t * p, int fPhase, char * pData );
extern void            Map_NodeSetNextE( Map_Node_t * p, Map_Node_t * pNextE );
extern void            Map_NodeSetRepr( Map_Node_t * p, Map_Node_t * pRepr );
extern void            Map_NodeSetSwitching( Map_Node_t * p, float Switching );
extern void            Map_NodeSetAigId( Map_Node_t * p, int Id );

extern int             Map_NodeIsConst( Map_Node_t * p );
extern int             Map_NodeIsVar( Map_Node_t * p );
extern int             Map_NodeIsBuf( Map_Node_t * p );
extern int             Map_NodeIsAnd( Map_Node_t * p );
extern int             Map_NodeComparePhase( Map_Node_t * p1, Map_Node_t * p2 );

extern Map_Super_t *   Map_CutReadSuperBest( Map_Cut_t * p, int fPhase );
extern Map_Super_t *   Map_CutReadSuper0( Map_Cut_t * p );
extern Map_Super_t *   Map_CutReadSuper1( Map_Cut_t * p );
extern int             Map_CutReadLeavesNum( Map_Cut_t * p );
extern Map_Node_t **   Map_CutReadLeaves( Map_Cut_t * p );
extern unsigned        Map_CutReadPhaseBest( Map_Cut_t * p, int fPhase );
extern unsigned        Map_CutReadPhase0( Map_Cut_t * p );
extern unsigned        Map_CutReadPhase1( Map_Cut_t * p );
extern Map_Cut_t *     Map_CutReadNext( Map_Cut_t * p );

extern char *          Map_SuperReadFormula( Map_Super_t * p );
extern Mio_Gate_t *    Map_SuperReadRoot( Map_Super_t * p );
extern int             Map_SuperReadNum( Map_Super_t * p );
extern Map_Super_t **  Map_SuperReadFanins( Map_Super_t * p );
extern int             Map_SuperReadFaninNum( Map_Super_t * p );
extern Map_Super_t *   Map_SuperReadNext( Map_Super_t * p );
extern int             Map_SuperReadNumPhases( Map_Super_t * p );
extern unsigned char * Map_SuperReadPhases( Map_Super_t * p );
extern int             Map_SuperReadFanoutLimit( Map_Super_t * p );

extern Mio_Library_t * Map_SuperLibReadGenLib( Map_SuperLib_t * p );
extern float           Map_SuperLibReadAreaInv( Map_SuperLib_t * p );
extern Map_Time_t      Map_SuperLibReadDelayInv( Map_SuperLib_t * p );
extern int             Map_SuperLibReadVarsMax( Map_SuperLib_t * p );

extern Map_Node_t *    Map_NodeAnd( Map_Man_t * p, Map_Node_t * p1, Map_Node_t * p2 );
extern Map_Node_t *    Map_NodeBuf( Map_Man_t * p, Map_Node_t * p1 );
extern void            Map_NodeSetChoice( Map_Man_t * pMan, Map_Node_t * pNodeOld, Map_Node_t * pNodeNew );

/*=== resmCanon.c =============================================================*/
extern int             Map_CanonComputeSlow( unsigned uTruths[][2], int nVarsMax, int nVarsReal, unsigned uTruth[], unsigned char * puPhases, unsigned uTruthRes[] );
extern int             Map_CanonComputeFast( Map_Man_t * p, int nVarsMax, int nVarsReal, unsigned uTruth[], unsigned char * puPhases, unsigned uTruthRes[] );
/*=== mapperCut.c =============================================================*/
extern Map_Cut_t *     Map_CutAlloc( Map_Man_t * p );
/*=== mapperCutUtils.c =============================================================*/
extern void            Map_CutCreateFromNode( Map_Man_t * p, Map_Super_t * pSuper, int iRoot, unsigned uPhaseRoot, 
                           int * pLeaves, int nLeaves, unsigned uPhaseLeaves );
extern Vec_Ptr_t *     Map_CutInternalNodes( Map_Node_t * pObj, Map_Cut_t * pCut );
/*=== mapperCore.c =============================================================*/
extern int             Map_Mapping( Map_Man_t * p );
/*=== mapperLib.c =============================================================*/
extern int             Map_SuperLibDeriveFromGenlib( Mio_Library_t * pLib, int fVerbose );
extern void            Map_SuperLibFree( Map_SuperLib_t * p );
/*=== mapperMntk.c =============================================================*/
//extern Mntk_Man_t *    Map_ConvertMappingToMntk( Map_Man_t * pMan );
/*=== mapperSuper.c =============================================================*/
extern char *          Map_LibraryReadFormulaStep( char * pFormula, char * pStrings[], int * pnStrings );
/*=== mapperSweep.c =============================================================*/
extern void            Map_NetworkSweep( Abc_Ntk_t * pNet );
/*=== mapperTable.c =============================================================*/
extern Map_Super_t *   Map_SuperTableLookupC( Map_SuperLib_t * pLib, unsigned uTruth[] );
/*=== mapperTime.c =============================================================*/
/*=== mapperUtil.c =============================================================*/
extern int             Map_ManCheckConsistency( Map_Man_t * p );
extern st__table *      Map_CreateTableGate2Super( Map_Man_t * p );
extern void            Map_ManCleanData( Map_Man_t * p );
extern void            Map_MappingSetupTruthTables( unsigned uTruths[][2] );
extern void            Map_MappingSetupTruthTablesLarge( unsigned uTruths[][32] );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
