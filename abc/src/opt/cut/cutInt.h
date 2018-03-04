/**CFile****************************************************************

  FileName    [cutInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [K-feasible cut computation package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cutInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__opt__cut__cutInt_h
#define ABC__opt__cut__cutInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "misc/extra/extra.h"
#include "misc/vec/vec.h"
#include "cut.h"
#include "cutList.h"

ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Cut_HashTableStruct_t_ Cut_HashTable_t;

struct Cut_ManStruct_t_
{ 
    // user preferences
    Cut_Params_t *     pParams;          // computation parameters
    Vec_Int_t *        vFanCounts;       // the array of fanout counters
    Vec_Int_t *        vNodeAttrs;       // node attributes (1 = global; 0 = local)
    // storage for cuts
    Vec_Ptr_t *        vCutsNew;         // new cuts by node ID
    Vec_Ptr_t *        vCutsOld;         // old cuts by node ID
    Vec_Ptr_t *        vCutsTemp;        // temp cuts for cutset nodes by cutset node number
    // memory management
    Extra_MmFixed_t *  pMmCuts;
    int                EntrySize;
    int                nTruthWords;
    // temporary variables
    Cut_Cut_t *        pReady;
    Vec_Ptr_t *        vTemp;
    int                fCompl0;
    int                fCompl1;
    int                fSimul;
    int                nNodeCuts;
    Cut_Cut_t *        pStore0[2];
    Cut_Cut_t *        pStore1[2];
    Cut_Cut_t *        pCompareOld;
    Cut_Cut_t *        pCompareNew;
    unsigned *         puTemp[4];
    // record of the cut computation
    Vec_Int_t *        vNodeCuts;        // the number of cuts for each node
    Vec_Int_t *        vNodeStarts;      // the number of the starting cut of each node
    Vec_Int_t *        vCutPairs;        // the pairs of parent cuts for each cut
    // minimum delay mapping with the given cuts
    Vec_Ptr_t *        vCutsMax;
    Vec_Int_t *        vDelays;
    Vec_Int_t *        vDelays2;
    int                nDelayMin;
    // statistics
    int                nCutsCur;
    int                nCutsAlloc;
    int                nCutsDealloc;
    int                nCutsPeak;
    int                nCutsTriv;
    int                nCutsFilter;
    int                nCutsLimit;
    int                nNodes;
    int                nNodesDag;
    int                nNodesNoCuts;
    // runtime
    abctime            timeMerge;
    abctime            timeUnion;
    abctime            timeTruth;
    abctime            timeFilter;
    abctime            timeHash;
    abctime            timeMap;
};

// iterator through all the cuts of the list
#define Cut_ListForEachCut( pList, pCut )                 \
    for ( pCut = pList;                                   \
          pCut;                                           \
          pCut = pCut->pNext )
#define Cut_ListForEachCutStop( pList, pCut, pStop )      \
    for ( pCut = pList;                                   \
          pCut != pStop;                                  \
          pCut = pCut->pNext )
#define Cut_ListForEachCutSafe( pList, pCut, pCut2 )      \
    for ( pCut = pList,                                   \
          pCut2 = pCut? pCut->pNext: NULL;                \
          pCut;                                           \
          pCut = pCut2,                                   \
          pCut2 = pCut? pCut->pNext: NULL )

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// computes signature of the node
static inline unsigned Cut_NodeSign( int Node )        { return (1 << (Node % 31));                        }
static inline int      Cut_TruthWords( int nVarsMax )  { return nVarsMax <= 5 ? 1 : (1 << (nVarsMax - 5)); }

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== cutCut.c ==========================================================*/
extern Cut_Cut_t *         Cut_CutAlloc( Cut_Man_t * p );
extern void                Cut_CutRecycle( Cut_Man_t * p, Cut_Cut_t * pCut );
extern int                 Cut_CutCompare( Cut_Cut_t * pCut1, Cut_Cut_t * pCut2 );
extern Cut_Cut_t *         Cut_CutDupList( Cut_Man_t * p, Cut_Cut_t * pList );
extern void                Cut_CutRecycleList( Cut_Man_t * p, Cut_Cut_t * pList );
extern Cut_Cut_t *         Cut_CutMergeLists( Cut_Cut_t * pList1, Cut_Cut_t * pList2 ); 
extern void                Cut_CutNumberList( Cut_Cut_t * pList );
extern Cut_Cut_t *         Cut_CutCreateTriv( Cut_Man_t * p, int Node );
extern void                Cut_CutPrintMerge( Cut_Cut_t * pCut, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1 );
/*=== cutMerge.c ==========================================================*/
extern Cut_Cut_t *         Cut_CutMergeTwo( Cut_Man_t * p, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1 );
/*=== cutNode.c ==========================================================*/
extern void                Cut_NodeDoComputeCuts( Cut_Man_t * p, Cut_List_t * pSuper, int Node, int fCompl0, int fCompl1, Cut_Cut_t * pList0, Cut_Cut_t * pList1, int fTriv, int TreeCode ); 
extern int                 Cut_CutListVerify( Cut_Cut_t * pList );
/*=== cutTable.c ==========================================================*/
extern Cut_HashTable_t *   Cut_TableStart( int Size );
extern void                Cut_TableStop( Cut_HashTable_t * pTable );
extern int                 Cut_TableLookup( Cut_HashTable_t * pTable, Cut_Cut_t * pCut, int fStore );
extern void                Cut_TableClear( Cut_HashTable_t * pTable );
extern int                 Cut_TableReadTime( Cut_HashTable_t * pTable );
/*=== cutTruth.c ==========================================================*/
extern void                Cut_TruthComputeOld( Cut_Cut_t * pCut, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1, int fCompl0, int fCompl1 );
extern void                Cut_TruthCompute( Cut_Man_t * p, Cut_Cut_t * pCut, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1, int fCompl0, int fCompl1 );



ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

