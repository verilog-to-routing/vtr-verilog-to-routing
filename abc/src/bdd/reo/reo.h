/**CFile****************************************************************

  FileName    [reo.h]

  PackageName [REO: A specialized DD reordering engine.]

  Synopsis    [External and internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 15, 2002.]

  Revision    [$Id: reo.h,v 1.0 2002/15/10 03:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__bdd__reo__reo_h
#define ABC__bdd__reo__reo_h


#include <stdio.h>
#include <stdlib.h>
#include "bdd/extrab/extraBdd.h"

////////////////////////////////////////////////////////////////////////
///                     MACRO DEFINITIONS                            ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


// reordering parameters
#define REO_REORDER_LIMIT      1.15  // determines the quality/runtime trade-off
#define REO_QUAL_PAR              3  // the quality [1 = simple lower bound, 2 = strict, larger = heuristic]
// internal parameters
#define REO_CONST_LEVEL       30000  // the number of the constant level
#define REO_TOPREF_UNDEF      30000  // the undefined top reference
#define REO_CHUNK_SIZE         5000  // the number of units allocated at one time
#define REO_COST_EPSILON  0.0000001  // difference in cost large enough so that it counted as an error
#define REO_HIGH_VALUE     10000000  // a large value used to initialize some variables
// interface parameters
#define REO_ENABLE                1  // the value of the enable flag
#define REO_DISABLE               0  // the value of the disable flag

// the types of minimization currently supported
typedef enum {
	REO_MINIMIZE_NODES,
	REO_MINIMIZE_WIDTH,   // may not work for BDDs with complemented edges
	REO_MINIMIZE_APL
} reo_min_type;

////////////////////////////////////////////////////////////////////////
///                      DATA STRUCTURES                             ///
////////////////////////////////////////////////////////////////////////

typedef struct _reo_unit     reo_unit;    // the unit representing one DD node during reordering
typedef struct _reo_plane    reo_plane;   // the set of nodes on one level
typedef struct _reo_hash     reo_hash;    // the entry in the hash table
typedef struct _reo_man      reo_man;     // the reordering manager
typedef struct _reo_test     reo_test;    // 

struct _reo_unit
{
	short       lev;             // the level of this node at the beginning
	short       TopRef;          // the top level from which this node is refed (used to update BDD width)
	short       TopRefNew;       // the new top level from which this node is refed (used to update BDD width)
	short       n;               // the number of incoming edges (similar to ref count in the BDD)
	int         Sign;            // the signature

	reo_unit *  pE;              // the pointer to the "else" branch
	reo_unit *  pT;              // the pointer to the "then" branch
	reo_unit *  Next;            // the link to the next one in the list
	double      Weight;          // the probability of traversing this node
};

struct _reo_plane
{
	int         fSifted;         // to mark the sifted variables
	int         statsNodes;      // the number of nodes in the current level
	int         statsWidth;      // the width on the current level
	double      statsApl;        // the sum of node probabilities on this level
	double      statsCost;       // the current cost is stored here
	double      statsCostAbove;  // the current cost is stored here
	double      statsCostBelow;  // the current cost is stored here

	reo_unit *  pHead;           // the pointer to the beginning of the unit list
};

struct _reo_hash
{
	int         Sign;            // signature of the current cache operation
	reo_unit *  Arg1;            // the first argument
	reo_unit *  Arg2;            // the second argument
	reo_unit *  Arg3;            // the third argument
};

struct _reo_man
{
	// these paramaters can be set by the API functions
	int         fMinWidth;       // the flag to enable reordering for minimum width
	int         fMinApl;         // the flag to enable reordering for minimum APL
	int         fVerbose;        // the verbosity level
	int         fVerify;         // the flag toggling verification
	int         fRemapUp;        // the flag to enable remapping   
	int         nIters;          // the number of interations of sifting to perform

	// parameters given by the user when reordering is called
	DdManager * dd;              // the CUDD BDD manager
	int *       pOrder;          // the resulting variable order will be returned here

	// derived parameters
	int         fThisIsAdd;      // this flag is one if the function is the ADD 
	int *       pSupp;           // the support of the given function
	int         nSuppAlloc;      // the max allowed number of support variables
	int         nSupp;           // the number of support variables
	int *       pOrderInt;       // the array storing the internal variable permutation
	double *    pVarCosts;       // other arrays
	int *       pLevelOrder;     // other arrays
	reo_unit ** pWidthCofs;      // temporary storage for cofactors used during reordering for width

	// parameters related to cost
	int         nNodesBeg;
	int         nNodesCur;
	int         nNodesEnd;
	int         nWidthCur;
	int         nWidthBeg;
	int         nWidthEnd;
	double      nAplCur;
	double      nAplBeg;
	double      nAplEnd;

	// mapping of the function into planes and back
	int *       pMapToPlanes;    // the mapping of var indexes into plane levels
	int *       pMapToDdVarsOrig;// the mapping of plane levels into the original indexes
	int *       pMapToDdVarsFinal;// the mapping of plane levels into the final indexes

	// the planes table
	reo_plane * pPlanes;
	int         nPlanes;
	reo_unit ** pTops;
	int         nTops;
	int         nTopsAlloc;

	// the hash table
	reo_hash *  HTable;           // the table itself
	int         nTableSize;       // the size of the hash table
	int         Signature;        // the signature counter

	// the referenced node list
	int         nNodesMaxAlloc;   // this parameters determins how much memory is allocated
	DdNode **   pRefNodes;
	int         nRefNodes;
	int         nRefNodesAlloc;

	// unit memory management
	reo_unit *  pUnitFreeList;
	reo_unit ** pMemChunks;
	int         nMemChunks;
	int         nMemChunksAlloc;
	int         nUnitsUsed;

	// statistic variables
	int         HashSuccess;
	int         HashFailure;
	int         nSwaps;            // the number of swaps
	int         nNISwaps;          // the number of swaps without interaction
};

// used to manipulate units
#define Unit_Regular(u)     ((reo_unit *)((ABC_PTRUINT_T)(u) & ~01))
#define Unit_Not(u)         ((reo_unit *)((ABC_PTRUINT_T)(u) ^ 01))
#define Unit_NotCond(u,c)   ((reo_unit *)((ABC_PTRUINT_T)(u) ^ (c)))
#define Unit_IsConstant(u)  ((int)((u)->lev == REO_CONST_LEVEL))

////////////////////////////////////////////////////////////////////////
///                   FUNCTION DECLARATIONS                          ///
////////////////////////////////////////////////////////////////////////

// ======================= reoApi.c ========================================
extern reo_man *  Extra_ReorderInit( int nDdVarsMax, int nNodesMax );
extern void       Extra_ReorderQuit( reo_man * p );
extern void       Extra_ReorderSetMinimizationType( reo_man * p, reo_min_type fMinType );
extern void       Extra_ReorderSetRemapping( reo_man * p, int fRemapUp );
extern void       Extra_ReorderSetIterations( reo_man * p, int nIters );
extern void       Extra_ReorderSetVerbosity( reo_man * p, int fVerbose );
extern void       Extra_ReorderSetVerification( reo_man * p, int fVerify );
extern DdNode *   Extra_Reorder( reo_man * p, DdManager * dd, DdNode * Func, int * pOrder );
extern void       Extra_ReorderArray( reo_man * p, DdManager * dd, DdNode * Funcs[], DdNode * FuncsRes[], int nFuncs, int * pOrder );
// ======================= reoCore.c =======================================
extern void       reoReorderArray( reo_man * p, DdManager * dd, DdNode * Funcs[], DdNode * FuncsRes[], int nFuncs, int * pOrder );
extern void       reoResizeStructures( reo_man * p, int nDdVarsMax, int nNodesMax, int nFuncs );
// ======================= reoProfile.c ======================================
extern void       reoProfileNodesStart( reo_man * p );
extern void       reoProfileAplStart( reo_man * p );
extern void       reoProfileWidthStart( reo_man * p );
extern void       reoProfileWidthStart2( reo_man * p );
extern void       reoProfileAplPrint( reo_man * p );
extern void       reoProfileNodesPrint( reo_man * p );
extern void       reoProfileWidthPrint( reo_man * p );
extern void       reoProfileWidthVerifyLevel( reo_plane * pPlane, int Level );
// ======================= reoSift.c =======================================
extern void       reoReorderSift( reo_man * p );
// ======================= reoSwap.c =======================================
extern double     reoReorderSwapAdjacentVars( reo_man * p, int Level, int fMovingUp );
// ======================= reoTransfer.c ===================================
extern reo_unit * reoTransferNodesToUnits_rec( reo_man * p, DdNode * F );
extern DdNode *   reoTransferUnitsToNodes_rec( reo_man * p, reo_unit * pUnit );
// ======================= reoUnits.c ======================================
extern reo_unit * reoUnitsGetNextUnit(reo_man * p );
extern void       reoUnitsRecycleUnit( reo_man * p, reo_unit * pUnit );
extern void       reoUnitsRecycleUnitList( reo_man * p, reo_plane * pPlane );
extern void       reoUnitsAddUnitToPlane( reo_plane * pPlane, reo_unit * pUnit );
extern void       reoUnitsStopDispenser( reo_man * p );
// ======================= reoTest.c =======================================
extern void       Extra_ReorderTest( DdManager * dd, DdNode * Func );
extern DdNode *   Extra_ReorderCudd( DdManager * dd, DdNode * aFunc, int pPermuteReo[] );
extern int        Extra_bddReorderTest( DdManager * dd, DdNode * bF ); 
extern int        Extra_addReorderTest( DdManager * dd, DdNode * aF ); 



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                         END OF FILE                              ///
////////////////////////////////////////////////////////////////////////
