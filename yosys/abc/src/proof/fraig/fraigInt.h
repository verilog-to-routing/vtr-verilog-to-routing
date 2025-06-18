/**CFile****************************************************************

  FileName    [fraigInt.h]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Internal declarations of the FRAIG package.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - October 1, 2004]

  Revision    [$Id: fraigInt.h,v 1.15 2005/07/08 01:01:31 alanmi Exp $]

***********************************************************************/

#ifndef ABC__sat__fraig__fraigInt_h
#define ABC__sat__fraig__fraigInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/util/abc_global.h"
#include "fraig.h"
#include "sat/msat/msat.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////
 
/*
    The AIG node policy:
    - Each node has its main number (pNode->Num)
      This is the number of this node in the array of all nodes and its SAT variable number
    - The PI nodes are stored along with other nodes
      Additionally, PI nodes have a PI number, by which they are stored in the PI node array
    - The constant node is has number 0 and is also stored in the array
*/

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////
 
// enable this macro to support the fanouts
#define FRAIG_ENABLE_FANOUTS
#define FRAIG_PATTERNS_RANDOM   2048   // should not be less than 128 and more than 32768 (2^15)
#define FRAIG_PATTERNS_DYNAMIC  2048   // should not be less than 256 and more than 32768 (2^15)
#define FRAIG_MAX_PRIMES        1024   // the maximum number of primes used for hashing

// this parameter determines when simulation info is extended
// it will be extended when the free storage in the dynamic simulation
// info is less or equal to this number of words (FRAIG_WORDS_STORE)
// this is done because if the free storage for dynamic simulation info 
// is not sufficient, computation becomes inefficient 
#define FRAIG_WORDS_STORE           5   

// the bit masks
#define FRAIG_MASK(n)             ((~((unsigned)0)) >> (32-(n)))
#define FRAIG_FULL                 (~((unsigned)0))
#define FRAIG_NUM_WORDS(n)         (((n)>>5) + (((n)&31) > 0))

// generating random unsigned (#define RAND_MAX 0x7fff)
//#define FRAIG_RANDOM_UNSIGNED   ((((unsigned)rand()) << 24) ^ (((unsigned)rand()) << 12) ^ ((unsigned)rand()))
#define FRAIG_RANDOM_UNSIGNED  Aig_ManRandom(0)

// macros to get hold of the bits in a bit string
#define Fraig_BitStringSetBit(p,i)  ((p)[(i)>>5] |= (1<<((i) & 31)))
#define Fraig_BitStringXorBit(p,i)  ((p)[(i)>>5] ^= (1<<((i) & 31)))
#define Fraig_BitStringHasBit(p,i) (((p)[(i)>>5]  & (1<<((i) & 31))) > 0)

// macros to get hold of the bits in the support info
//#define Fraig_NodeSetVarStr(p,i)      (Fraig_Regular(p)->pSuppStr[((i)%FRAIG_SUPP_SIGN)>>5] |= (1<<(((i)%FRAIG_SUPP_SIGN) & 31)))
//#define Fraig_NodeHasVarStr(p,i)     ((Fraig_Regular(p)->pSuppStr[((i)%FRAIG_SUPP_SIGN)>>5]  & (1<<(((i)%FRAIG_SUPP_SIGN) & 31))) > 0)
#define Fraig_NodeSetVarStr(p,i)     Fraig_BitStringSetBit(Fraig_Regular(p)->pSuppStr,i)
#define Fraig_NodeHasVarStr(p,i)     Fraig_BitStringHasBit(Fraig_Regular(p)->pSuppStr,i)

// copied from "extra.h" for stand-aloneness
#define Fraig_PrintTime(a,t)      printf( "%s = ", (a) ); printf( "%6.2f sec\n", (float)(t)/(float)(CLOCKS_PER_SEC) )

#define Fraig_HashKey2(a,b,TSIZE) (((ABC_PTRUINT_T)(a) + (ABC_PTRUINT_T)(b) * 12582917) % TSIZE)
//#define Fraig_HashKey2(a,b,TSIZE) (( ((unsigned)(a)->Num * 19) ^ ((unsigned)(b)->Num * 1999) ) % TSIZE)
//#define Fraig_HashKey2(a,b,TSIZE) ( ((unsigned)((a)->Num + (b)->Num) * ((a)->Num + (b)->Num + 1) / 2) % TSIZE)
// the other two hash functions give bad distribution of hash chain lengths (not clear why)

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Fraig_MemFixed_t_    Fraig_MemFixed_t;    

// the mapping manager
struct Fraig_ManStruct_t_
{
    // the AIG nodes
    Fraig_NodeVec_t *     vInputs;       // the array of primary inputs
    Fraig_NodeVec_t *     vNodes;        // the array of all nodes, including primary inputs
    Fraig_NodeVec_t *     vOutputs;      // the array of primary outputs (some internal nodes)
    Fraig_Node_t *        pConst1;       // the pointer to the constant node (vNodes->pArray[0])

    // info about the original circuit
    char **               ppInputNames;  // the primary input names
    char **               ppOutputNames; // the primary output names

    // various hash-tables
    Fraig_HashTable_t *   pTableS;       // hashing by structure
    Fraig_HashTable_t *   pTableF;       // hashing by simulation info
    Fraig_HashTable_t *   pTableF0;      // hashing by simulation info (sparse functions)

    // parameters
    int                   nWordsRand;    // the number of words of random simulation info
    int                   nWordsDyna;    // the number of words of dynamic simulation info
    int                   nBTLimit;      // the max number of backtracks to perform
    int                   nSeconds;      // the runtime limit for the miter proof
    int                   fFuncRed;      // performs only one level hashing
    int                   fFeedBack;     // enables solver feedback
    int                   fDist1Pats;    // enables solver feedback
    int                   fDoSparse;     // performs equiv tests for sparse functions 
    int                   fChoicing;     // enables recording structural choices
    int                   fTryProve;     // tries to solve the final miter
    int                   fVerbose;      // the verbosiness flag
    int                   fVerboseP;     // the verbosiness flag
    ABC_INT64_T                nInspLimit;    // the inspection limit

    int                   nTravIds;      // the traversal counter
    int                   nTravIds2;     // the traversal counter

    // info related to the solver feedback
    int                   iWordStart;    // the first word to use for simulation
    int                   iWordPerm;     // the number of words stored permanently
    int                   iPatsPerm;     // the number of patterns stored permanently
    Fraig_NodeVec_t *     vCones;        // the temporary array of internal variables
    Msat_IntVec_t *       vPatsReal;     // the array of real pattern numbers
    unsigned *            pSimsReal;     // used for simulation patterns
    unsigned *            pSimsDiff;     // used for simulation patterns
    unsigned *            pSimsTemp;     // used for simulation patterns

    // the support information
    int                   nSuppWords;
    unsigned **           pSuppS;
    unsigned **           pSuppF;

    // the memory managers
    Fraig_MemFixed_t *    mmNodes;       // the memory manager for nodes
    Fraig_MemFixed_t *    mmSims;        // the memory manager for simulation info

    // solving the SAT problem
    Msat_Solver_t *       pSat;          // the SAT solver
    Msat_IntVec_t *       vProj;         // the temporary array of projection vars 
    int                   nSatNums;      // the counter of SAT variables
    int *                 pModel;        // the assignment, which satisfies the miter
    // these arrays belong to the solver
    Msat_IntVec_t *       vVarsInt;      // the temporary array of variables
    Msat_ClauseVec_t *    vAdjacents;    // the temporary storage for connectivity
    Msat_IntVec_t *       vVarsUsed;     // the array marking vars appearing in the cone

    // various statistic variables
    int                   nSatCalls;     // the number of times equivalence checking was called
    int                   nSatProof;     // the number of times a proof was found
    int                   nSatCounter;   // the number of times a counter example was found
    int                   nSatFails;     // the number of times the SAT solver failed to complete due to resource limit or prediction
    int                   nSatFailsReal; // the number of times the SAT solver failed to complete due to resource limit

    int                   nSatCallsImp;  // the number of times equivalence checking was called
    int                   nSatProofImp;  // the number of times a proof was found
    int                   nSatCounterImp;// the number of times a counter example was found
    int                   nSatFailsImp;  // the number of times the SAT solver failed to complete

    int                   nSatZeros;     // the number of times the simulation vector is zero
    int                   nSatSupps;     // the number of times the support info was useful
    int                   nRefErrors;    // the number of ref counting errors
    int                   nImplies;      // the number of implication cases
    int                   nSatImpls;     // the number of implication SAT calls
    int                   nVarsClauses;  // the number of variables with clauses
    int                   nSimplifies0;
    int                   nSimplifies1;
    int                   nImplies0;
    int                   nImplies1;

    // runtime statistics
    abctime               timeToAig;     // time to transfer to the mapping structure
    abctime               timeSims;      // time to compute k-feasible cuts
    abctime               timeTrav;      // time to traverse the network
    abctime               timeFeed;      // time for solver feedback (recording and resimulating)
    abctime               timeImply;     // time to analyze implications
    abctime               timeSat;       // time to compute the truth table for each cut
    abctime               timeToNet;     // time to transfer back to the network
    abctime               timeTotal;     // the total mapping time
    abctime               time1;         // time to perform one task
    abctime               time2;         // time to perform another task
    abctime               time3;         // time to perform another task
    abctime               time4;         // time to perform another task
};

// the mapping node
struct Fraig_NodeStruct_t_ 
{
    // various numbers associated with the node
    int                   Num;           // the unique number (SAT var number) of this node 
    int                   NumPi;         // if the node is a PI, this is its variable number
    int                   Level;         // the level of the node
    int                   nRefs;         // the number of references of the node
    int                   TravId;        // the traversal ID (use to avoid cleaning marks)
    int                   TravId2;       // the traversal ID (use to avoid cleaning marks)

    // general information about the node
    unsigned              fInv     :  1; // the mark to show that simulation info is complemented
    unsigned              fNodePo  :  1; // the mark used for primary outputs
    unsigned              fClauses :  1; // the clauses for this node are loaded
    unsigned              fMark0   :  1; // the mark used for traversals
    unsigned              fMark1   :  1; // the mark used for traversals
    unsigned              fMark2   :  1; // the mark used for traversals
    unsigned              fMark3   :  1; // the mark used for traversals
    unsigned              fFeedUse :  1; // the presence of the variable in the feedback
    unsigned              fFeedVal :  1; // the value of the variable in the feedback
    unsigned              fFailTfo :  1; // the node is in the TFO of the failed SAT run
    unsigned              nFanouts :  2; // the indicator of fanouts (none, one, or many)
    unsigned              nOnes    : 20; // the number of 1's in the random sim info
 
    // the children of the node
    Fraig_Node_t *        p1;            // the first child
    Fraig_Node_t *        p2;            // the second child
    Fraig_NodeVec_t *     vFanins;       // the fanins of the supergate rooted at this node
//    Fraig_NodeVec_t *     vFanouts;      // the fanouts of the supergate rooted at this node

    // various linked lists
    Fraig_Node_t *        pNextS;        // the next node in the structural hash table
    Fraig_Node_t *        pNextF;        // the next node in the functional (simulation) hash table
    Fraig_Node_t *        pNextD;        // the next node in the list of nodes based on dynamic simulation
    Fraig_Node_t *        pNextE;        // the next structural choice (functionally-equivalent node)
    Fraig_Node_t *        pRepr;         // the canonical functional representative of the node

    // simulation data
    unsigned              uHashR;        // the hash value for random information
    unsigned              uHashD;        // the hash value for dynamic information 
    unsigned *            puSimR;        // the simulation information (random)
    unsigned *            puSimD;        // the simulation information (dynamic)

    // misc information  
    Fraig_Node_t *        pData0;        // temporary storage for the corresponding network node
    Fraig_Node_t *        pData1;        // temporary storage for the corresponding network node

#ifdef FRAIG_ENABLE_FANOUTS
    // representation of node's fanouts
    Fraig_Node_t *        pFanPivot;     // the first fanout of this node
    Fraig_Node_t *        pFanFanin1;    // the next fanout of p1
    Fraig_Node_t *        pFanFanin2;    // the next fanout of p2
#endif
}; 

// the vector of nodes
struct Fraig_NodeVecStruct_t_
{
    int                   nCap;          // the number of allocated entries
    int                   nSize;         // the number of entries in the array
    Fraig_Node_t **       pArray;        // the array of nodes
};

// the hash table 
struct Fraig_HashTableStruct_t_
{
    Fraig_Node_t **       pBins;         // the table bins
    int                   nBins;         // the size of the table
    int                   nEntries;      // the total number of entries in the table
};

// getting hold of the next fanout of the node
#define Fraig_NodeReadNextFanout( pNode, pFanout )                \
    ( ( pFanout == NULL )? NULL :                                 \
        ((Fraig_Regular((pFanout)->p1) == (pNode))?               \
             (pFanout)->pFanFanin1 : (pFanout)->pFanFanin2) )
// getting hold of the place where the next fanout will be attached
#define Fraig_NodeReadNextFanoutPlace( pNode, pFanout )           \
    ( (Fraig_Regular((pFanout)->p1) == (pNode))?                  \
         &(pFanout)->pFanFanin1 : &(pFanout)->pFanFanin2 )
// iterator through the fanouts of the node
#define Fraig_NodeForEachFanout( pNode, pFanout )                 \
    for ( pFanout = (pNode)->pFanPivot; pFanout;                  \
          pFanout = Fraig_NodeReadNextFanout(pNode, pFanout) )
// safe iterator through the fanouts of the node
#define Fraig_NodeForEachFanoutSafe( pNode, pFanout, pFanout2 )   \
    for ( pFanout  = (pNode)->pFanPivot,                          \
          pFanout2 = Fraig_NodeReadNextFanout(pNode, pFanout);    \
          pFanout;                                                \
          pFanout  = pFanout2,                                    \
          pFanout2 = Fraig_NodeReadNextFanout(pNode, pFanout) )

// iterators through the entries in the linked lists of nodes
// the list of nodes in the structural hash table
#define Fraig_TableBinForEachEntryS( pBin, pEnt )                 \
    for ( pEnt = pBin;                                            \
          pEnt;                                                   \
          pEnt = pEnt->pNextS )
#define Fraig_TableBinForEachEntrySafeS( pBin, pEnt, pEnt2 )      \
    for ( pEnt = pBin,                                            \
          pEnt2 = pEnt? pEnt->pNextS: NULL;                       \
          pEnt;                                                   \
          pEnt = pEnt2,                                           \
          pEnt2 = pEnt? pEnt->pNextS: NULL )
// the list of nodes in the functional (simulation) hash table
#define Fraig_TableBinForEachEntryF( pBin, pEnt )                 \
    for ( pEnt = pBin;                                            \
          pEnt;                                                   \
          pEnt = pEnt->pNextF )
#define Fraig_TableBinForEachEntrySafeF( pBin, pEnt, pEnt2 )      \
    for ( pEnt = pBin,                                            \
          pEnt2 = pEnt? pEnt->pNextF: NULL;                       \
          pEnt;                                                   \
          pEnt = pEnt2,                                           \
          pEnt2 = pEnt? pEnt->pNextF: NULL )
// the list of nodes with the same simulation and different functionality
#define Fraig_TableBinForEachEntryD( pBin, pEnt )                 \
    for ( pEnt = pBin;                                            \
          pEnt;                                                   \
          pEnt = pEnt->pNextD )
#define Fraig_TableBinForEachEntrySafeD( pBin, pEnt, pEnt2 )      \
    for ( pEnt = pBin,                                            \
          pEnt2 = pEnt? pEnt->pNextD: NULL;                       \
          pEnt;                                                   \
          pEnt = pEnt2,                                           \
          pEnt2 = pEnt? pEnt->pNextD: NULL )
// the list of nodes with the same functionality 
#define Fraig_TableBinForEachEntryE( pBin, pEnt )                 \
    for ( pEnt = pBin;                                            \
          pEnt;                                                   \
          pEnt = pEnt->pNextE )
#define Fraig_TableBinForEachEntrySafeE( pBin, pEnt, pEnt2 )      \
    for ( pEnt = pBin,                                            \
          pEnt2 = pEnt? pEnt->pNextE: NULL;                       \
          pEnt;                                                   \
          pEnt = pEnt2,                                           \
          pEnt2 = pEnt? pEnt->pNextE: NULL )

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

// random number generator imported from another package
extern unsigned            Aig_ManRandom( int fReset );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== fraigCanon.c =============================================================*/
extern Fraig_Node_t *      Fraig_NodeAndCanon( Fraig_Man_t * pMan, Fraig_Node_t * p1, Fraig_Node_t * p2 );
/*=== fraigFanout.c =============================================================*/
extern void                Fraig_NodeAddFaninFanout( Fraig_Node_t * pFanin, Fraig_Node_t * pFanout );
extern void                Fraig_NodeRemoveFaninFanout( Fraig_Node_t * pFanin, Fraig_Node_t * pFanoutToRemove );
extern int                 Fraig_NodeGetFanoutNum( Fraig_Node_t * pNode );
/*=== fraigFeed.c =============================================================*/
extern void                Fraig_FeedBackInit( Fraig_Man_t * p );
extern void                Fraig_FeedBack( Fraig_Man_t * p, int * pModel, Msat_IntVec_t * vVars, Fraig_Node_t * pOld, Fraig_Node_t * pNew );
extern void                Fraig_FeedBackTest( Fraig_Man_t * p );
extern int                 Fraig_FeedBackCompress( Fraig_Man_t * p );
extern int *               Fraig_ManAllocCounterExample( Fraig_Man_t * p );
extern int *               Fraig_ManSaveCounterExample( Fraig_Man_t * p, Fraig_Node_t * pNode );
/*=== fraigMan.c =============================================================*/
extern void                Fraig_ManCreateSolver( Fraig_Man_t * p );
/*=== fraigMem.c =============================================================*/
extern Fraig_MemFixed_t *  Fraig_MemFixedStart( int nEntrySize );
extern void                Fraig_MemFixedStop( Fraig_MemFixed_t * p, int fVerbose );
extern char *              Fraig_MemFixedEntryFetch( Fraig_MemFixed_t * p );
extern void                Fraig_MemFixedEntryRecycle( Fraig_MemFixed_t * p, char * pEntry );
extern void                Fraig_MemFixedRestart( Fraig_MemFixed_t * p );
extern int                 Fraig_MemFixedReadMemUsage( Fraig_MemFixed_t * p );
/*=== fraigNode.c =============================================================*/
extern Fraig_Node_t *      Fraig_NodeCreateConst( Fraig_Man_t * p );
extern Fraig_Node_t *      Fraig_NodeCreatePi( Fraig_Man_t * p );
extern Fraig_Node_t *      Fraig_NodeCreate( Fraig_Man_t * p, Fraig_Node_t * p1, Fraig_Node_t * p2 );
extern void                Fraig_NodeSimulate( Fraig_Node_t * pNode, int iWordStart, int iWordStop, int fUseRand );
/*=== fraigPrime.c =============================================================*/
extern int                 s_FraigPrimes[FRAIG_MAX_PRIMES];
/*=== fraigSat.c ===============================================================*/
extern int                 Fraig_NodeIsImplification( Fraig_Man_t * p, Fraig_Node_t * pOld, Fraig_Node_t * pNew, int nBTLimit );
/*=== fraigTable.c =============================================================*/
extern Fraig_HashTable_t * Fraig_HashTableCreate( int nSize );
extern void                Fraig_HashTableFree( Fraig_HashTable_t * p );
extern int                 Fraig_HashTableLookupS( Fraig_Man_t * pMan, Fraig_Node_t * p1, Fraig_Node_t * p2, Fraig_Node_t ** ppNodeRes );
extern Fraig_Node_t *      Fraig_HashTableLookupF( Fraig_Man_t * pMan, Fraig_Node_t * pNode );
extern Fraig_Node_t *      Fraig_HashTableLookupF0( Fraig_Man_t * pMan, Fraig_Node_t * pNode );
extern void                Fraig_HashTableInsertF0( Fraig_Man_t * pMan, Fraig_Node_t * pNode );
extern int                 Fraig_CompareSimInfo( Fraig_Node_t * pNode1, Fraig_Node_t * pNode2, int iWordLast, int fUseRand );
extern int                 Fraig_CompareSimInfoUnderMask( Fraig_Node_t * pNode1, Fraig_Node_t * pNode2, int iWordLast, int fUseRand, unsigned * puMask );
extern int                 Fraig_FindFirstDiff( Fraig_Node_t * pNode1, Fraig_Node_t * pNode2, int fCompl, int iWordLast, int fUseRand );
extern void                Fraig_CollectXors( Fraig_Node_t * pNode1, Fraig_Node_t * pNode2, int iWordLast, int fUseRand, unsigned * puMask );
extern void                Fraig_TablePrintStatsS( Fraig_Man_t * pMan );
extern void                Fraig_TablePrintStatsF( Fraig_Man_t * pMan );
extern void                Fraig_TablePrintStatsF0( Fraig_Man_t * pMan );
extern int                 Fraig_TableRehashF0( Fraig_Man_t * pMan, int fLinkEquiv );
/*=== fraigUtil.c ===============================================================*/
extern int                 Fraig_NodeCountPis( Msat_IntVec_t * vVars, int nVarsPi );
extern int                 Fraig_NodeCountSuppVars( Fraig_Man_t * p, Fraig_Node_t * pNode, int fSuppStr );
extern int                 Fraig_NodesCompareSupps( Fraig_Man_t * p, Fraig_Node_t * pOld, Fraig_Node_t * pNew );
extern int                 Fraig_NodeAndSimpleCase_rec( Fraig_Node_t * pOld, Fraig_Node_t * pNew );
extern int                 Fraig_NodeIsExorType( Fraig_Node_t * pNode );
extern void                Fraig_ManSelectBestChoice( Fraig_Man_t * p );
extern int                 Fraig_BitStringCountOnes( unsigned * pString, int nWords );
extern void                Fraig_PrintBinary( FILE * pFile, unsigned * pSign, int nBits );
extern int                 Fraig_NodeIsExorType( Fraig_Node_t * pNode );
extern int                 Fraig_NodeIsExor( Fraig_Node_t * pNode );
extern int                 Fraig_NodeIsMuxType( Fraig_Node_t * pNode );
extern Fraig_Node_t *      Fraig_NodeRecognizeMux( Fraig_Node_t * pNode, Fraig_Node_t ** ppNodeT, Fraig_Node_t ** ppNodeE );
extern int                 Fraig_ManCountExors( Fraig_Man_t * pMan );
extern int                 Fraig_ManCountMuxes( Fraig_Man_t * pMan );
extern int                 Fraig_NodeSimsContained( Fraig_Man_t * pMan, Fraig_Node_t * pNode1, Fraig_Node_t * pNode2 );
extern int                 Fraig_NodeIsInSupergate( Fraig_Node_t * pOld, Fraig_Node_t * pNew );
extern Fraig_NodeVec_t *   Fraig_CollectSupergate( Fraig_Node_t * pNode, int fStopAtMux );
extern int                 Fraig_CountPis( Fraig_Man_t * p, Msat_IntVec_t * vVarNums );
extern void                Fraig_ManIncrementTravId( Fraig_Man_t * pMan );
extern void                Fraig_NodeSetTravIdCurrent( Fraig_Man_t * pMan, Fraig_Node_t * pNode );
extern int                 Fraig_NodeIsTravIdCurrent( Fraig_Man_t * pMan, Fraig_Node_t * pNode );
extern int                 Fraig_NodeIsTravIdPrevious( Fraig_Man_t * pMan, Fraig_Node_t * pNode );
/*=== fraigVec.c ===============================================================*/
extern void                Fraig_NodeVecSortByRefCount( Fraig_NodeVec_t * p );



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
