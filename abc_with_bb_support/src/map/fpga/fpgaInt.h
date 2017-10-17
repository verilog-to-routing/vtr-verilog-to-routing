/**CFile****************************************************************

  FileName    [fpgaInt.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Technology mapping for variable-size-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - August 18, 2004.]

  Revision    [$Id: fpgaInt.h,v 1.8 2004/09/30 21:18:10 satrajit Exp $]

***********************************************************************/
 
#ifndef ABC__map__fpga__fpgaInt_h
#define ABC__map__fpga__fpgaInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc/extra/extra.h"
#include "fpga.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////
 
// uncomment to have fanouts represented in the mapping graph
//#define FPGA_ALLOCATE_FANOUT  1

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define inline __inline // compatible with MS VS 6.0
#endif

// the maximum number of cut leaves (currently does not work for 7)
#define FPGA_MAX_LEAVES           6   

// the bit masks
#define FPGA_MASK(n)             ((~((unsigned)0)) >> (32-(n)))
#define FPGA_FULL                 (~((unsigned)0))
#define FPGA_NO_VAR               (-9999.0)
#define FPGA_NUM_BYTES(n)         (((n)/16 + (((n)%16) > 0))*16)

// maximum/minimum operators
#define FPGA_MIN(a,b)             (((a) < (b))? (a) : (b))
#define FPGA_MAX(a,b)             (((a) > (b))? (a) : (b))

// the small and large numbers (min/max float are 1.17e-38/3.40e+38)
#define FPGA_FLOAT_LARGE          ((float)1.0e+20)
#define FPGA_FLOAT_SMALL          ((float)1.0e-20)
#define FPGA_INT_LARGE            (10000000)

// the macro to compute the signature
#define FPGA_SEQ_SIGN(p)        (1 << (((ABC_PTRUINT_T)p)%31));

// internal macros to work with cuts
#define Fpga_CutIsComplement(p)  (((int)((ABC_PTRUINT_T)(p) & 01)))
#define Fpga_CutRegular(p)       ((Fpga_Cut_t *)((ABC_PTRUINT_T)(p) & ~01)) 
#define Fpga_CutNot(p)           ((Fpga_Cut_t *)((ABC_PTRUINT_T)(p) ^ 01)) 
#define Fpga_CutNotCond(p,c)     ((Fpga_Cut_t *)((ABC_PTRUINT_T)(p) ^ (c)))

// the cut nodes
#define Fpga_SeqIsComplement( p )      (((int)((ABC_PTRUINT_T) (p) & 01)))
#define Fpga_SeqRegular( p )           ((Fpga_Node_t *)((ABC_PTRUINT_T)(p) & ~015))
#define Fpga_SeqIndex( p )             ((((ABC_PTRUINT_T)(p)) >> 1) & 07)
#define Fpga_SeqIndexCreate( p, Ind )  (((ABC_PTRUINT_T)(p)) | (1 << (((ABC_PTRUINT_T)(Ind)) & 07)))

// internal macros for referencing of nodes
#define Fpga_NodeReadRef(p)      ((Fpga_Regular(p))->nRefs)
#define Fpga_NodeRef(p)          ((Fpga_Regular(p))->nRefs++)

// returns the complemented attribute of the node
#define Fpga_NodeIsSimComplement(p) (Fpga_IsComplement(p)? !(Fpga_Regular(p)->fInv) : (p)->fInv)

// generating random unsigned (#define RAND_MAX 0x7fff)
#define FPGA_RANDOM_UNSIGNED   ((((unsigned)rand()) << 24) ^ (((unsigned)rand()) << 12) ^ ((unsigned)rand()))

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

// the mapping manager
struct Fpga_ManStruct_t_
{
    // the mapping graph
    Fpga_Node_t **      pBins;         // the table of nodes hashed by their children
    int                 nBins;         // the size of the table
    Fpga_Node_t **      pInputs;       // the array of inputs
    int                 nInputs;       // the number of inputs
    Fpga_Node_t **      pOutputs;      // the array of outputs
    int                 nOutputs;      // the number of outputs
    int                 nNodes;        // the total number of nodes
    int                 nLatches;      // the number of latches in the circuit
    Fpga_Node_t *       pConst1;       // the constant 1 node
    Fpga_NodeVec_t *    vNodesAll;     // the nodes by number
    Fpga_NodeVec_t *    vAnds;         // the nodes reachable from COs
    Fpga_NodeVec_t *    vMapping;      // the nodes used in the current mapping

    // info about the original circuit
    char *              pFileName;     // the file name
    char **             ppOutputNames; // the primary output names
    float *             pInputArrivals;// the PI arrival times

    // mapping parameters
    int                 nVarsMax;      // the max number of variables
    int                 fAreaRecovery; // the flag to use area flow as the first parameter
    int                 fVerbose;      // the verbosiness flag
    int                 fSwitching;    // minimize the switching activity (instead of area)
    int                 fLatchPaths;   // optimize latch paths for delay, other paths for area
    int                 nTravIds;      // the counter of traversal IDs
    float               DelayTarget;   // the target required times

    // support of choice nodes
    int                 nChoiceNodes;  // the number of choice nodes
    int                 nChoices;      // the number of all choices

    int                 nCanons;
    int                 nMatches;
 
    // the supergate library
    Fpga_LutLib_t *     pLutLib;       // the current LUT library

    // the memory managers
    Extra_MmFixed_t *   mmNodes;       // the memory manager for nodes
    Extra_MmFixed_t *   mmCuts;        // the memory manager for cuts

    // resynthesis parameters
    int                 fResynthesis;  // the resynthesis flag
    float               fRequiredGlo;  // the global required times
    float               fRequiredShift;// the shift of the required times
    float               fRequiredStart;// the starting global required times
    float               fRequiredGain; // the reduction in delay
    float               fAreaGlo;      // the total area
    float               fAreaGain;     // the reduction in area
    float               fEpsilon;      // the epsilon used to compare floats
    float               fDelayWindow;  // the delay window for delay-oriented resynthesis
    float               DelayLimit;    // for resynthesis
    float               AreaLimit;     // for resynthesis
    float               TimeLimit;     // for resynthesis

    // runtime statistics
    clock_t             timeToMap;     // time to transfer to the mapping structure
    clock_t             timeCuts;      // time to compute k-feasible cuts
    clock_t             timeTruth;     // time to compute the truth table for each cut
    clock_t             timeMatch;     // time to perform matching for each node
    clock_t             timeRecover;   // time to perform area recovery
    clock_t             timeToNet;     // time to transfer back to the network
    clock_t             timeTotal;     // the total mapping time
    clock_t             time1;         // time to transfer to the mapping structure
    clock_t             time2;         // time to transfer to the mapping structure
};

// the LUT library
struct Fpga_LutLibStruct_t_
{
    char *              pName;         // the name of the LUT library
    int                 LutMax;        // the maximum LUT size 
    int                 fVarPinDelays; // set to 1 if variable pin delays are specified
    float               pLutAreas[FPGA_MAX_LUTSIZE+1]; // the areas of LUTs
    float               pLutDelays[FPGA_MAX_LUTSIZE+1][FPGA_MAX_LUTSIZE+1];// the delays of LUTs
};

// the mapping node
struct Fpga_NodeStruct_t_ 
{
    // general information about the node
    Fpga_Node_t *       pNext;         // the next node in the hash table
    Fpga_Node_t *       pLevel;        // the next node in the linked list by level
    int                 Num;           // the unique number of this node
    int                 NumA;          // the unique number of this node
    int                 Num2;          // the temporary number of this node
    int                 nRefs;         // the number of references (fanouts) of the given node
    unsigned            fMark0 : 1;    // the mark used for traversals
    unsigned            fMark1 : 1;    // the mark used for traversals
    unsigned            fInv   : 1;    // the complemented attribute for the equivalent nodes
    unsigned            Value  : 2;    // the value of the nodes
    unsigned            fUsed  : 1;    // the flag indicating that the node is used in the mapping
    unsigned            fTemp  : 1;    // unused
    unsigned            Level  :11;    // the level of the given node
    unsigned            uData  :14;    // used to mark the fanins, for which resynthesis was tried
    int                 TravId;

    // the successors of this node     
    Fpga_Node_t *       p1;            // the first child
    Fpga_Node_t *       p2;            // the second child
    Fpga_Node_t *       pNextE;        // the next functionally equivalent node
    Fpga_Node_t *       pRepr;         // the representative of the functionally equivalent class

#ifdef FPGA_ALLOCATE_FANOUT
    // representation of node's fanouts
    Fpga_Node_t *       pFanPivot;     // the first fanout of this node
    Fpga_Node_t *       pFanFanin1;    // the next fanout of p1
    Fpga_Node_t *       pFanFanin2;    // the next fanout of p2
//    Fpga_NodeVec_t *    vFanouts;      // the array of fanouts of the gate
#endif

    // the delay information
    float               tRequired;     // the best area flow 
    float               aEstFanouts;   // the fanout estimation
    float               Switching;     // the probability of switching
    int                 LValue;        // the l-value of the node
    short               nLatches1;     // the number of latches on the first edge
    short               nLatches2;     // the number of latches on the second edge

    // cut information
    Fpga_Cut_t *        pCutBest;      // the best mapping
    Fpga_Cut_t *        pCutOld;       // the old mapping
    Fpga_Cut_t *        pCuts;         // mapping choices for the node (elementary comes first)
    Fpga_Cut_t *        pCutsN;        // mapping choices for the node (elementary comes first)

    // misc information  
    char *              pData0;        // temporary storage for the corresponding network node
}; 
  
// the cuts used for matching
struct Fpga_CutStruct_t_  
{
    Fpga_Cut_t *        pOne;          // the father of this cut
    Fpga_Cut_t *        pTwo;          // the mother of this cut
    Fpga_Node_t *       pRoot;         // the root of the cut
    Fpga_Node_t *       ppLeaves[FPGA_MAX_LEAVES+1];   // the leaves of this cut
    float               fLevel;        // the average level of the fanins
    unsigned            uSign;         // signature for quick comparison
    char                fMark;         // the mark to denote visited cut
    char                Phase;         // the mark to denote complemented cut
    char                nLeaves;       // the number of leaves of this cut
    char                nVolume;       // the volume of this cut
    float               tArrival;      // the arrival time 
    float               aFlow;         // the area flow of the cut
    Fpga_Cut_t *        pNext;         // the pointer to the next cut in the list
};

// the vector of nodes
struct Fpga_NodeVecStruct_t_
{
    Fpga_Node_t **      pArray;        // the array of nodes
    int                 nSize;         // the number of entries in the array
    int                 nCap;          // the number of allocated entries
};

// getting hold of the next fanout of the node
#define Fpga_NodeReadNextFanout( pNode, pFanout )                \
    ( ( pFanout == NULL )? NULL :                                \
        ((Fpga_Regular((pFanout)->p1) == (pNode))?               \
             (pFanout)->pFanFanin1 : (pFanout)->pFanFanin2) )

// getting hold of the place where the next fanout will be attached
#define Fpga_NodeReadNextFanoutPlace( pNode, pFanout )           \
    ( (Fpga_Regular((pFanout)->p1) == (pNode))?                  \
         &(pFanout)->pFanFanin1 : &(pFanout)->pFanFanin2 )

// iterator through the fanouts of the node
#define Fpga_NodeForEachFanout( pNode, pFanout )                 \
    for ( pFanout = (pNode)->pFanPivot; pFanout;                 \
          pFanout = Fpga_NodeReadNextFanout(pNode, pFanout) )

// safe iterator through the fanouts of the node
#define Fpga_NodeForEachFanoutSafe( pNode, pFanout, pFanout2 )   \
    for ( pFanout  = (pNode)->pFanPivot,                         \
          pFanout2 = Fpga_NodeReadNextFanout(pNode, pFanout);    \
          pFanout;                                               \
          pFanout  = pFanout2,                                   \
          pFanout2 = Fpga_NodeReadNextFanout(pNode, pFanout) )

static inline int Fpga_FloatMoreThan( Fpga_Man_t * p, float Arg1, float Arg2 ) { return Arg1 > Arg2 + p->fEpsilon; }
static inline int Fpga_FloatLessThan( Fpga_Man_t * p, float Arg1, float Arg2 ) { return Arg1 < Arg2 - p->fEpsilon; }
static inline int Fpga_FloatEqual( Fpga_Man_t * p, float Arg1, float Arg2 )    { return Arg1 > Arg2 - p->fEpsilon && Arg1 < Arg2 + p->fEpsilon; }

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== fpgaCut.c ===============================================================*/
extern void              Fpga_MappingCuts( Fpga_Man_t * p );
extern void              Fpga_MappingCreatePiCuts( Fpga_Man_t * p );
extern int               Fpga_CutCountAll( Fpga_Man_t * pMan );
/*=== fpgaCutUtils.c ===============================================================*/
extern Fpga_Cut_t *      Fpga_CutAlloc( Fpga_Man_t * p );
extern Fpga_Cut_t *      Fpga_CutDup( Fpga_Man_t * p, Fpga_Cut_t * pCutOld );
extern void              Fpga_CutFree( Fpga_Man_t * p, Fpga_Cut_t * pCut );
extern void              Fpga_CutPrint( Fpga_Man_t * p, Fpga_Node_t * pRoot, Fpga_Cut_t * pCut );
extern Fpga_Cut_t *      Fpga_CutCreateSimple( Fpga_Man_t * p, Fpga_Node_t * pNode );
extern float             Fpga_CutGetRootArea( Fpga_Man_t * p, Fpga_Cut_t * pCut );
extern Fpga_Cut_t *      Fpga_CutListAppend( Fpga_Cut_t * pSetAll, Fpga_Cut_t * pSets );
extern void              Fpga_CutListRecycle( Fpga_Man_t * p, Fpga_Cut_t * pSetList, Fpga_Cut_t * pSave );
extern int               Fpga_CutListCount( Fpga_Cut_t * pSets );
extern void              Fpga_CutRemoveFanouts( Fpga_Man_t * p, Fpga_Node_t * pNode, Fpga_Cut_t * pCut );
extern void              Fpga_CutInsertFanouts( Fpga_Man_t * p, Fpga_Node_t * pNode, Fpga_Cut_t * pCut );
extern float             Fpga_CutGetAreaRefed( Fpga_Man_t * pMan, Fpga_Cut_t * pCut );
extern float             Fpga_CutGetAreaDerefed( Fpga_Man_t * pMan, Fpga_Cut_t * pCut );
extern float             Fpga_CutRef( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut, int fFanouts );
extern float             Fpga_CutDeref( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut, int fFanouts );
extern float             Fpga_CutGetAreaFlow( Fpga_Man_t * pMan, Fpga_Cut_t * pCut );
extern void              Fpga_CutGetParameters( Fpga_Man_t * pMan, Fpga_Cut_t * pCut );
/*=== fraigFanout.c =============================================================*/
extern void              Fpga_NodeAddFaninFanout( Fpga_Node_t * pFanin, Fpga_Node_t * pFanout );
extern void              Fpga_NodeRemoveFaninFanout( Fpga_Node_t * pFanin, Fpga_Node_t * pFanoutToRemove );
extern int               Fpga_NodeGetFanoutNum( Fpga_Node_t * pNode );
/*=== fpgaLib.c ============================================================*/
extern Fpga_LutLib_t *   Fpga_LutLibRead( char * FileName, int fVerbose );
extern void              Fpga_LutLibFree( Fpga_LutLib_t * p );
extern void              Fpga_LutLibPrint( Fpga_LutLib_t * pLutLib );
extern int               Fpga_LutLibDelaysAreDiscrete( Fpga_LutLib_t * pLutLib );
/*=== fpgaMatch.c ===============================================================*/
extern int               Fpga_MappingMatches( Fpga_Man_t * p, int fDelayOriented );
extern int               Fpga_MappingMatchesArea( Fpga_Man_t * p );
extern int               Fpga_MappingMatchesSwitch( Fpga_Man_t * p );
/*=== fpgaShow.c =============================================================*/
extern void              Fpga_MappingShow( Fpga_Man_t * pMan, char * pFileName );
extern void              Fpga_MappingShowNodes( Fpga_Man_t * pMan, Fpga_Node_t ** ppRoots, int nRoots, char * pFileName );
/*=== fpgaSwitch.c =============================================================*/
extern float             Fpga_CutGetSwitchDerefed( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut );
extern float             Fpga_CutRefSwitch( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut, int fFanouts );
extern float             Fpga_CutDerefSwitch( Fpga_Man_t * pMan, Fpga_Node_t * pNode, Fpga_Cut_t * pCut, int fFanouts );
extern float             Fpga_MappingGetSwitching( Fpga_Man_t * pMan, Fpga_NodeVec_t * vMapping );
/*=== fpgaTime.c ===============================================================*/
extern float             Fpga_TimeCutComputeArrival( Fpga_Man_t * pMan, Fpga_Cut_t * pCut );
extern float             Fpga_TimeCutComputeArrival_rec( Fpga_Man_t * pMan, Fpga_Cut_t * pCut );
extern float             Fpga_TimeComputeArrivalMax( Fpga_Man_t * p );
extern void              Fpga_TimeComputeRequiredGlobal( Fpga_Man_t * p, int fFirstTime );
extern void              Fpga_TimeComputeRequired( Fpga_Man_t * p, float fRequired );
extern void              Fpga_TimePropagateRequired( Fpga_Man_t * p, Fpga_NodeVec_t * vNodes );
extern void              Fpga_TimePropagateArrival( Fpga_Man_t * p );
/*=== fpgaVec.c =============================================================*/
extern Fpga_NodeVec_t *  Fpga_NodeVecAlloc( int nCap );
extern void              Fpga_NodeVecFree( Fpga_NodeVec_t * p );
extern Fpga_Node_t **    Fpga_NodeVecReadArray( Fpga_NodeVec_t * p );
extern int               Fpga_NodeVecReadSize( Fpga_NodeVec_t * p );
extern void              Fpga_NodeVecGrow( Fpga_NodeVec_t * p, int nCapMin );
extern void              Fpga_NodeVecShrink( Fpga_NodeVec_t * p, int nSizeNew );
extern void              Fpga_NodeVecClear( Fpga_NodeVec_t * p );
extern void              Fpga_NodeVecPush( Fpga_NodeVec_t * p, Fpga_Node_t * Entry );
extern int               Fpga_NodeVecPushUnique( Fpga_NodeVec_t * p, Fpga_Node_t * Entry );
extern Fpga_Node_t *     Fpga_NodeVecPop( Fpga_NodeVec_t * p );
extern void              Fpga_NodeVecWriteEntry( Fpga_NodeVec_t * p, int i, Fpga_Node_t * Entry );
extern Fpga_Node_t *     Fpga_NodeVecReadEntry( Fpga_NodeVec_t * p, int i );
extern void              Fpga_NodeVecSortByLevel( Fpga_NodeVec_t * p );
extern void              Fpga_SortNodesByArrivalTimes( Fpga_NodeVec_t * p );
extern void              Fpga_NodeVecUnion( Fpga_NodeVec_t * p, Fpga_NodeVec_t * p1, Fpga_NodeVec_t * p2 );
extern void              Fpga_NodeVecPushOrder( Fpga_NodeVec_t * vNodes, Fpga_Node_t * pNode, int fIncreasing );
extern void              Fpga_NodeVecReverse( Fpga_NodeVec_t * vNodes );

/*=== fpgaUtils.c ===============================================================*/
extern Fpga_NodeVec_t *  Fpga_MappingDfs( Fpga_Man_t * pMan, int fCollectEquiv );
extern Fpga_NodeVec_t *  Fpga_MappingDfsNodes( Fpga_Man_t * pMan, Fpga_Node_t ** ppNodes, int nNodes, int fEquiv );
extern int               Fpga_CountLevels( Fpga_Man_t * pMan );
extern float             Fpga_MappingGetAreaFlow( Fpga_Man_t * p );
extern float             Fpga_MappingArea( Fpga_Man_t * pMan );
extern float             Fpga_MappingAreaTrav( Fpga_Man_t * pMan );
extern float             Fpga_MappingSetRefsAndArea( Fpga_Man_t * pMan );
extern void              Fpga_MappingPrintOutputArrivals( Fpga_Man_t * p );
extern void              Fpga_MappingSetupTruthTables( unsigned uTruths[][2] );
extern void              Fpga_MappingSetupMask( unsigned uMask[], int nVarsMax );
extern void              Fpga_MappingSortByLevel( Fpga_Man_t * pMan, Fpga_NodeVec_t * vNodes, int fIncreasing );
extern Fpga_NodeVec_t *  Fpga_DfsLim( Fpga_Man_t * pMan, Fpga_Node_t * pNode, int nLevels );
extern Fpga_NodeVec_t *  Fpga_MappingLevelize( Fpga_Man_t * pMan, Fpga_NodeVec_t * vNodes );
extern int               Fpga_MappingMaxLevel( Fpga_Man_t * pMan );
extern void              Fpga_ManReportChoices( Fpga_Man_t * pMan );
extern void              Fpga_MappingSetChoiceLevels( Fpga_Man_t * pMan );



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
