/**CFile****************************************************************

  FileName    [mapperInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperInt.h,v 1.8 2004/09/30 21:18:10 satrajit Exp $]

***********************************************************************/

#ifndef ABC__map__mapper__mapperInt_h
#define ABC__map__mapper__mapperInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "base/main/main.h"
#include "map/mio/mio.h"
#include "mapper.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

// uncomment to have fanouts represented in the mapping graph
//#define MAP_ALLOCATE_FANOUT  1

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

// the bit masks
#define MAP_MASK(n)             ((~((unsigned)0)) >> (32-(n)))
#define MAP_FULL                 (~((unsigned)0))
#define MAP_NO_VAR               (-9999.0)

// maximum/minimum operators
#define MAP_MIN(a,b)             (((a) < (b))? (a) : (b))
#define MAP_MAX(a,b)             (((a) > (b))? (a) : (b))

// the small and large numbers (min/max float are 1.17e-38/3.40e+38)
#define MAP_FLOAT_LARGE          ((float)(FLT_MAX/10))
#define MAP_FLOAT_SMALL          ((float)1.0e-03)

// generating random unsigned (#define RAND_MAX 0x7fff)
#define MAP_RANDOM_UNSIGNED   ((((unsigned)rand()) << 24) ^ (((unsigned)rand()) << 12) ^ ((unsigned)rand()))

// internal macros to work with cuts
#define Map_CutIsComplement(p)  (((int)((ABC_PTRUINT_T) (p) & 01)))
#define Map_CutRegular(p)       ((Map_Cut_t *)((ABC_PTRUINT_T)(p) & ~01)) 
#define Map_CutNot(p)           ((Map_Cut_t *)((ABC_PTRUINT_T)(p) ^ 01)) 
#define Map_CutNotCond(p,c)     ((Map_Cut_t *)((ABC_PTRUINT_T)(p) ^ (c)))

// internal macros for referencing of nodes
#define Map_NodeReadRef(p)      ((Map_Regular(p))->nRefs)
#define Map_NodeRef(p)          ((Map_Regular(p))->nRefs++)

// macros to get hold of the bits in the support info
#define Map_InfoSetVar(p,i)     (p[(i)>>5] |=  (1<<((i) & 31)))
#define Map_InfoRemVar(p,i)     (p[(i)>>5] &= ~(1<<((i) & 31)))
#define Map_InfoFlipVar(p,i)    (p[(i)>>5] ^=  (1<<((i) & 31)))
#define Map_InfoReadVar(p,i)   ((p[(i)>>5]  &  (1<<((i) & 31))) > 0)

// returns the complemented attribute of the node
#define Map_NodeIsSimComplement(p) (Map_IsComplement(p)? !(Map_Regular(p)->fInv) : (p)->fInv)

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// the mapping manager
struct Map_ManStruct_t_
{
    // the mapping graph
    Map_Node_t **       pBins;         // the table of nodes hashed by their children
    int                 nBins;         // the size of the table
    Map_Node_t **       pInputs;       // the array of inputs
    int                 nInputs;       // the number of inputs
    Map_Node_t **       pOutputs;      // the array of outputs
    int                 nOutputs;      // the number of outputs
    int                 nNodes;        // the total number of nodes
    Map_Node_t *        pConst1;       // the constant 1 node
    Map_NodeVec_t *     vMapObjs;      // the array of all nodes
    Map_NodeVec_t *     vMapBufs;      // the array of all nodes
    float *             pNodeDelays;   // the array of node delays

    // info about the original circuit
    char **             ppOutputNames; // the primary output names
    Map_Time_t *        pInputArrivals;// the PI arrival times
    Map_Time_t *        pOutputRequireds;// the PI arrival times

    // mapping parameters
    int                 nVarsMax;      // the max number of variables
    int                 fAreaRecovery; // the flag to enable area recovery
    int                 fVerbose;      // the verbosiness flag
    int                 fMappingMode;  // set to 1 when doing area
    float               fRequiredGlo;  // the global required times
    float               fEpsilon;      // the epsilon used to compare floats
    float               AreaBase;      // the area after delay-oriented mapping
    float               AreaFinal;     // the area after delay-oriented mapping
    int                 nIterations;   // How many matching passes to do
    int                 fObeyFanoutLimits;// Should mapper try to obey fanout limits or not
    float               DelayTarget;   // the required times set by the user
    int                 nTravIds;      // the traversal counter
    int                 fSwitching;    // use switching activity
    int                 fSkipFanout;   // skip large gates when mapping high-fanout nodes
    int                 fUseProfile;   // use standard-cell profile

    // the supergate library
    Map_SuperLib_t *    pSuperLib;     // the current supergate library
    unsigned            uTruths[6][2]; // the elementary truth tables
    unsigned            uTruthsLarge[10][32]; // the elementary truth tables
    int                 nCounts[32];   // the counter of minterms
    int                 nCountsBest[32];// the counter of minterms
    Map_NodeVec_t *     vVisited;      // the visited cuts during cut computation

    // the memory managers
    Extra_MmFixed_t *   mmNodes;       // the memory manager for nodes
    Extra_MmFixed_t *   mmCuts;        // the memory manager for cuts

    // precomputed N-canonical forms
    unsigned short *    uCanons;       // N-canonical forms
    char **             uPhases;       // N-canonical phases
    char *              pCounters;     // counters of phases

    // various statistical variables
    int                 nChoiceNodes;  // the number of choice nodes
    int                 nChoices;      // the number of all choices
    int                 nCanons;       // the number of times N-canonical form was computed
    int                 nMatches;      // the number of times supergate matching was performed
    int                 nPhases;       // the number of phases considered during matching
    int                 nFanoutViolations;  // the number of nodes in mapped circuit violating fanout

    // runtime statistics
    abctime             timeToMap;     // time to transfer to the mapping structure
    abctime             timeCuts;      // time to compute k-feasible cuts
    abctime             timeTruth;     // time to compute the truth table for each cut
    abctime             timeMatch;     // time to perform matching for each node
    abctime             timeArea;      // time to recover area after delay oriented mapping
    abctime             timeSweep;     // time to perform technology dependent sweep
    abctime             timeToNet;     // time to transfer back to the network
    abctime             timeTotal;     // the total mapping time
    abctime             time1;         // time to transfer to the mapping structure
    abctime             time2;         // time to transfer to the mapping structure
    abctime             time3;         // time to transfer to the mapping structure
};

// the supergate library
struct Map_SuperLibStruct_t_
{
    // general info
    char *              pName;         // the name of the supergate library
    Mio_Library_t *     pGenlib;       // the generic library

    // other info
    int                 nVarsMax;      // the max number of variables
    int                 nSupersAll;    // the total number of supergates
    int                 nSupersReal;   // the total number of supergates
    int                 nLines;        // the total number of lines in the supergate file
    int                 fVerbose;      // the verbosity flag

    // hash tables
    Map_Super_t **      ppSupers;      // the array of supergates
    Map_HashTable_t *   tTableC;       // the table mapping N-canonical forms into supergates
    Map_HashTable_t *   tTable;        // the table mapping truth tables into supergates

    // data structures for N-canonical form computation
    unsigned            uTruths[6][2]; // the elementary truth tables
    unsigned            uMask[2];      // the mask for the truth table

    // the invertor
    Mio_Gate_t *        pGateInv;      // the pointer to the intertor gate
    Map_Time_t          tDelayInv;     // the delay of the inverter
    float               AreaInv;       // the area of the inverter
    float               AreaBuf;       // the area of the buffer
    Map_Super_t *       pSuperInv;     // the supergate representing the inverter

    // the memory manager for the internal table
    Extra_MmFixed_t *   mmSupers;      // the mamory manager for supergates
    Extra_MmFixed_t *   mmEntries;     // the memory manager for the entries
    Extra_MmFlex_t *    mmForms;       // the memory manager for formulas
};

// the mapping node
struct Map_NodeStruct_t_ 
{
    // general information about the node
    Map_Man_t *         p;             // the mapping manager
    Map_Node_t *        pNext;         // the next node in the hash table
    int                 Num;           // the unique number of this node
    int                 TravId;        // the traversal ID (use to avoid cleaning marks)
    int                 nRefs;         // the number of references (fanouts) of the given node
    unsigned            fMark0 : 1;    // the mark used for traversals
    unsigned            fMark1 : 1;    // the mark used for traversals
    unsigned            fUsed  : 1;    // the mark to mark the node or its fanins
    unsigned            fInv   : 1;    // the complemented attribute for the equivalent nodes
    unsigned            fInvert: 1;    // the flag to denote the use of interter
    unsigned            Level  :16;    // the level of the given node
    unsigned            NumTemp:10;    // the level of the given node
    int                 nRefAct[3];    // estimated fanout for current covering phase, neg and pos and sum
    float               nRefEst[3];    // actual fanout for previous covering phase, neg and pos and sum
    float               Switching;     // the probability of switching

    // connectivity
    Map_Node_t *        p1;            // the first child
    Map_Node_t *        p2;            // the second child
    Map_Node_t *        pNextE;        // the next functionally equivalent node
    Map_Node_t *        pRepr;         // the representative of the functionally equivalent class

#ifdef MAP_ALLOCATE_FANOUT
    // representation of node's fanouts
    Map_Node_t *        pFanPivot;     // the first fanout of this node
    Map_Node_t *        pFanFanin1;    // the next fanout of p1
    Map_Node_t *        pFanFanin2;    // the next fanout of p2
//    Map_NodeVec_t *     vFanouts;      // the array of fanouts of the gate
#endif

    // the delay information
    Map_Time_t          tArrival[2];   // the best arrival time of the neg (0) and pos (1) phases
    Map_Time_t          tRequired[2];  // the required time of the neg (0) and pos (1) phases

    // misc information  
    Map_Cut_t *         pCutBest[2];   // the best mapping for neg and pos phase
    Map_Cut_t *         pCuts;         // mapping choices for the node (elementary comes first)
    char *              pData0;        // temporary storage for the corresponding network node
    char *              pData1;        // temporary storage for the corresponding network node
}; 

// the match of the cut
struct Map_MatchStruct_t_  
{
    // information used for matching
    Map_Super_t *       pSupers;     
    unsigned            uPhase;  
    // information about the best selected match
    unsigned            uPhaseBest;    // the best phase (the EXOR of match's phase and gate's phase)
    Map_Super_t *       pSuperBest;    // the best supergate matched
    // the parameters of the match
    Map_Time_t          tArrive;       // the arrival time of this match
    float               AreaFlow;      // the area flow or area of this match
};
  
// the cuts used for matching
struct Map_CutStruct_t_  
{
    Map_Cut_t *         pNext;         // the pointer to the next cut in the list
    Map_Cut_t *         pOne;          // the father of this cut
    Map_Cut_t *         pTwo;          // the mother of this cut
    Map_Node_t *        ppLeaves[6];   // the leaves of this cut
    unsigned            uTruth;        // truth table for five-input cuts
    char                nLeaves;       // the number of leaves
    char                nVolume;       // the volume of this cut
    char                fMark;         // the mark to denote visited cut
    char                Phase;         // the mark to denote complemented cut
    Map_Match_t         M[2];          // the matches for positive/negative phase
};

// the supergate internally represented
struct Map_SuperStruct_t_  
{
    int                 Num;           // the ID of the supergate
    unsigned            fSuper  :  1;  // the flag to distinquish a real super from a fake one
    unsigned            fExclude:  1;  // the flag if set causes gate to be excluded from being used for mapping
    unsigned            nFanins :  3;  // the number of inputs
    unsigned            nGates  :  3;  // the number of gates inside this supergate
    unsigned            nFanLimit: 4;  // the max number of fanout count
    unsigned            nSupers : 16;  // the number of supergates in the list
    unsigned            nPhases :  4;  // the number of phases for matching with canonical form
    unsigned char       uPhases[4];    // the maximum of 4 phases for matching with canonical form
    int                 nUsed;         // the number of times the supergate is used
    Map_Super_t *       pFanins[6];    // the fanins of the gate
    Mio_Gate_t *        pRoot;         // the root gate
    unsigned            uTruth[2];     // the truth table
    Map_Time_t          tDelaysR[6];   // the pin-to-pin delay constraints for the rise of the output
    Map_Time_t          tDelaysF[6];   // the pin-to-pin delay constraints for the rise of the output
    Map_Time_t          tDelayMax;     // the maximum delay
    float               Area;          // the area
    char *              pFormula;      // the symbolic formula
    Map_Super_t *       pNext;         // the pointer to the next super in the list
};

// the vector of nodes
struct Map_NodeVecStruct_t_
{
    Map_Node_t **       pArray;        // the array of nodes
    int                 nSize;         // the number of entries in the array
    int                 nCap;          // the number of allocated entries
};

// the hash table 
struct Map_HashTableStruct_t_
{
    Map_HashEntry_t **  pBins;         // the table bins
    int                 nBins;         // the size of the table
    int                 nEntries;      // the total number of entries in the table
    Extra_MmFixed_t *   mmMan;         // the memory manager for entries
};

// the entry in the hash table
struct Map_HashEntryStruct_t_
{
    unsigned            uTruth[2];     // the truth table for 6-var function
    unsigned            uPhase;        // the phase to tranform it into the canonical form
    Map_Super_t *       pGates;        // the linked list of matching supergates
    Map_HashEntry_t *   pNext;         // the next entry in the hash table
};

// getting hold of the next fanout of the node
#define Map_NodeReadNextFanout( pNode, pFanout )                \
    ( ( pFanout == NULL )? NULL :                               \
        ((Map_Regular((pFanout)->p1) == (pNode))?               \
             (pFanout)->pFanFanin1 : (pFanout)->pFanFanin2) )

// getting hold of the place where the next fanout will be attached
#define Map_NodeReadNextFanoutPlace( pNode, pFanout )           \
    ( (Map_Regular((pFanout)->p1) == (pNode))?                  \
         &(pFanout)->pFanFanin1 : &(pFanout)->pFanFanin2 )

// iterator through the fanouts of the node
#define Map_NodeForEachFanout( pNode, pFanout )                 \
    for ( pFanout = (pNode)->pFanPivot; pFanout;                \
          pFanout = Map_NodeReadNextFanout(pNode, pFanout) )

// safe iterator through the fanouts of the node
#define Map_NodeForEachFanoutSafe( pNode, pFanout, pFanout2 )   \
    for ( pFanout  = (pNode)->pFanPivot,                        \
          pFanout2 = Map_NodeReadNextFanout(pNode, pFanout);    \
          pFanout;                                              \
          pFanout  = pFanout2,                                  \
          pFanout2 = Map_NodeReadNextFanout(pNode, pFanout) )

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== mapperCanon.c =============================================================*/
/*=== mapperCut.c ===============================================================*/
extern void              Map_MappingCuts( Map_Man_t * p );
/*=== mapperCutUtils.c ===============================================================*/
extern Map_Cut_t *       Map_CutAlloc( Map_Man_t * p );
extern void              Map_CutFree( Map_Man_t * p, Map_Cut_t * pCut );
extern void              Map_CutPrint( Map_Man_t * p, Map_Node_t * pRoot, Map_Cut_t * pCut, int fPhase );
extern float             Map_CutGetRootArea( Map_Cut_t * pCut, int fPhase );
extern int               Map_CutGetLeafPhase( Map_Cut_t * pCut, int fPhase, int iLeaf );
extern int               Map_NodeGetLeafPhase( Map_Node_t * pNode, int fPhase, int iLeaf );
extern Map_Cut_t *       Map_CutListAppend( Map_Cut_t * pSetAll, Map_Cut_t * pSets );
extern void              Map_CutListRecycle( Map_Man_t * p, Map_Cut_t * pSetList, Map_Cut_t * pSave );
extern int               Map_CutListCount( Map_Cut_t * pSets );
extern void              Map_CutRemoveFanouts( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase );
extern void              Map_CutInsertFanouts( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase );
/*=== mapperFanout.c =============================================================*/
extern void              Map_NodeAddFaninFanout( Map_Node_t * pFanin, Map_Node_t * pFanout );
extern void              Map_NodeRemoveFaninFanout( Map_Node_t * pFanin, Map_Node_t * pFanoutToRemove );
extern int               Map_NodeGetFanoutNum( Map_Node_t * pNode );
/*=== mapperLib.c ============================================================*/
extern Map_SuperLib_t *  Map_SuperLibCreate( Mio_Library_t * pGenlib, Vec_Str_t * vStr, char * pFileName, char * pExcludeFile, int  fAlgorithm, int  fVerbose );
extern void              Map_SuperLibFree( Map_SuperLib_t * p );
/*=== mapperMatch.c ===============================================================*/
extern int               Map_MappingMatches( Map_Man_t * p );
/*=== mapperRefs.c =============================================================*/
extern void              Map_MappingEstimateRefsInit( Map_Man_t * p );
extern void              Map_MappingEstimateRefs( Map_Man_t * p );
extern float             Map_CutGetAreaFlow( Map_Cut_t * pCut, int fPhase );
extern float             Map_CutGetAreaRefed( Map_Cut_t * pCut, int fPhase );
extern float             Map_CutGetAreaDerefed( Map_Cut_t * pCut, int fPhase );
extern float             Map_CutRef( Map_Cut_t * pCut, int fPhase, int fProfile );
extern float             Map_CutDeref( Map_Cut_t * pCut, int fPhase, int fProfile );
extern void              Map_MappingSetRefs( Map_Man_t * pMan );
extern float             Map_MappingGetArea( Map_Man_t * pMan );
/*=== mapperSwitch.c =============================================================*/
extern float             Map_SwitchCutGetDerefed( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase );
extern float             Map_SwitchCutRef( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase );
extern float             Map_SwitchCutDeref( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase );
extern float             Map_MappingGetSwitching( Map_Man_t * pMan );
/*=== mapperTree.c ===============================================================*/
extern int               Map_LibraryDeriveGateInfo( Map_SuperLib_t * pLib, st__table * tExcludeGate );
extern int               Map_LibraryReadFileTreeStr( Map_SuperLib_t * pLib, Mio_Library_t * pGenlib, Vec_Str_t * vStr, char * pFileName );
extern int               Map_LibraryReadTree( Map_SuperLib_t * pLib, Mio_Library_t * pGenlib, char * pFileName, char * pExcludeFile );
extern void              Map_LibraryPrintTree( Map_SuperLib_t * pLib );
/*=== mapperSuper.c ===============================================================*/
extern int               Map_LibraryRead( Map_SuperLib_t * p, char * pFileName );
extern void              Map_LibraryPrintSupergate( Map_Super_t * pGate );
/*=== mapperTable.c ============================================================*/
extern Map_HashTable_t * Map_SuperTableCreate( Map_SuperLib_t * pLib );
extern void              Map_SuperTableFree( Map_HashTable_t * p );
extern int               Map_SuperTableInsertC( Map_HashTable_t * pLib, unsigned uTruthC[], Map_Super_t * pGate );
extern int               Map_SuperTableInsert( Map_HashTable_t * pLib, unsigned uTruth[], Map_Super_t * pGate, unsigned uPhase );
extern Map_Super_t *     Map_SuperTableLookup( Map_HashTable_t * p, unsigned uTruth[], unsigned * puPhase );
extern void              Map_SuperTableSortSupergates( Map_HashTable_t * p, int nSupersMax );
extern void              Map_SuperTableSortSupergatesByDelay( Map_HashTable_t * p, int nSupersMax );
/*=== mapperTime.c =============================================================*/
extern float             Map_TimeCutComputeArrival( Map_Node_t * pNode, Map_Cut_t * pCut, int fPhase, float tWorstCaseLimit );
extern float             Map_TimeComputeArrivalMax( Map_Man_t * p );
extern void              Map_TimeComputeRequiredGlobal( Map_Man_t * p );
/*=== mapperTruth.c ===============================================================*/
extern void              Map_MappingTruths( Map_Man_t * pMan );
extern int               Map_TruthsCutDontCare( Map_Man_t * pMan, Map_Cut_t * pCut, unsigned * uTruthDc );
extern int               Map_TruthCountOnes( unsigned * uTruth, int nLeaves );
extern int               Map_TruthDetectTwoFirst( unsigned * uTruth, int nLeaves );
/*=== mapperUtils.c ===============================================================*/
extern Map_NodeVec_t *   Map_MappingDfs( Map_Man_t * pMan, int fCollectEquiv );
extern int               Map_MappingCountLevels( Map_Man_t * pMan );
extern void              Map_MappingUnmark( Map_Man_t * pMan );
extern void              Map_MappingMark_rec( Map_Node_t * pNode );
extern void              Map_MappingUnmark_rec( Map_Node_t * pNode );
extern void              Map_MappingPrintOutputArrivals( Map_Man_t * p );
extern void              Map_MappingSetupMask( unsigned uMask[], int nVarsMax );
extern int               Map_MappingNodeIsViolator( Map_Node_t * pNode, Map_Cut_t * pCut, int fPosPol );
extern float             Map_MappingGetAreaFlow( Map_Man_t * p );
extern void              Map_MappingSortByLevel( Map_Man_t * pMan, Map_NodeVec_t * vNodes );
extern int               Map_MappingCountDoubles( Map_Man_t * pMan, Map_NodeVec_t * vNodes );
extern void              Map_MappingExpandTruth( unsigned uTruth[2], int nVars );
extern float             Map_MappingPrintSwitching( Map_Man_t * pMan );
extern void              Map_MappingSetPlacementInfo( Map_Man_t * p );
extern float             Map_MappingPrintWirelength( Map_Man_t * p );
extern void              Map_MappingWireReport( Map_Man_t * p );
extern float             Map_MappingComputeDelayWithFanouts( Map_Man_t * p );
extern int               Map_MappingGetMaxLevel( Map_Man_t * pMan );
extern void              Map_MappingSetChoiceLevels( Map_Man_t * pMan );
extern void              Map_MappingReportChoices( Map_Man_t * pMan );
/*=== mapperVec.c =============================================================*/
extern Map_NodeVec_t *   Map_NodeVecAlloc( int nCap );
extern void              Map_NodeVecFree( Map_NodeVec_t * p );
extern Map_NodeVec_t *   Map_NodeVecDup( Map_NodeVec_t * p );
extern Map_Node_t **     Map_NodeVecReadArray( Map_NodeVec_t * p );
extern int               Map_NodeVecReadSize( Map_NodeVec_t * p );
extern void              Map_NodeVecGrow( Map_NodeVec_t * p, int nCapMin );
extern void              Map_NodeVecShrink( Map_NodeVec_t * p, int nSizeNew );
extern void              Map_NodeVecClear( Map_NodeVec_t * p );
extern void              Map_NodeVecPush( Map_NodeVec_t * p, Map_Node_t * Entry );
extern int               Map_NodeVecPushUnique( Map_NodeVec_t * p, Map_Node_t * Entry );
extern Map_Node_t *      Map_NodeVecPop( Map_NodeVec_t * p );
extern void              Map_NodeVecRemove( Map_NodeVec_t * p, Map_Node_t * Entry );
extern void              Map_NodeVecWriteEntry( Map_NodeVec_t * p, int i, Map_Node_t * Entry );
extern Map_Node_t *      Map_NodeVecReadEntry( Map_NodeVec_t * p, int i );
extern void              Map_NodeVecSortByLevel( Map_NodeVec_t * p );



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
