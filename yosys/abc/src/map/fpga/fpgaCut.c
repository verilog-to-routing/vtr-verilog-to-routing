/**CFile****************************************************************

  FileName    [fpgaCut.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - August 18, 2004.]

  Revision    [$Id: fpgaCut.c,v 1.3 2004/07/06 04:55:57 alanmi Exp $]

***********************************************************************/
 
#include "fpgaInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Fpga_CutTableStrutct_t Fpga_CutTable_t;
struct Fpga_CutTableStrutct_t
{
    Fpga_Cut_t ** pBins;        // the table used for linear probing
    int           nBins;        // the size of the table
    int *         pCuts;        // the array of cuts currently stored 
    int           nCuts;        // the number of cuts currently stored 
    Fpga_Cut_t ** pArray;       // the temporary array of cuts
    Fpga_Cut_t ** pCuts1;       // the temporary array of cuts
    Fpga_Cut_t ** pCuts2;       // the temporary array of cuts
};

// the largest number of cuts considered
//#define  FPGA_CUTS_MAX_COMPUTE   500
#define  FPGA_CUTS_MAX_COMPUTE   2000
// the largest number of cuts used
//#define  FPGA_CUTS_MAX_USE       200
#define  FPGA_CUTS_MAX_USE       1000

// primes used to compute the hash key
static int s_HashPrimes[10] = { 109, 499, 557, 619, 631, 709, 797, 881, 907, 991 };

static int bit_count[256] = {
  0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

#define FPGA_COUNT_ONES(u) (bit_count[(u)&255]+bit_count[((u)>>8)&255]+bit_count[((u)>>16)&255]+bit_count[(u)>>24])

static Fpga_Cut_t *      Fpga_CutCompute( Fpga_Man_t * p, Fpga_CutTable_t * pTable, Fpga_Node_t * pNode );
static void              Fpga_CutFilter( Fpga_Man_t * p, Fpga_Node_t * pNode );
static Fpga_Cut_t *      Fpga_CutMergeLists( Fpga_Man_t * p, Fpga_CutTable_t * pTable, Fpga_Cut_t * pList1, Fpga_Cut_t * pList2, int fComp1, int fComp2, int fPivot1, int fPivot2 );
static int               Fpga_CutMergeTwo( Fpga_Cut_t * pCut1, Fpga_Cut_t * pCut2, Fpga_Node_t * ppNodes[], int nNodesMax );
static Fpga_Cut_t *      Fpga_CutUnionLists( Fpga_Cut_t * pList1, Fpga_Cut_t * pList2 );
static int               Fpga_CutBelongsToList( Fpga_Cut_t * pList, Fpga_Node_t * ppNodes[], int nNodes );
extern Fpga_Cut_t *      Fpga_CutAlloc( Fpga_Man_t * p );
extern int               Fpga_CutCountAll( Fpga_Man_t * pMan );

static void              Fpga_CutListPrint( Fpga_Man_t * pMan, Fpga_Node_t * pRoot );
static void              Fpga_CutListPrint2( Fpga_Man_t * pMan, Fpga_Node_t * pRoot );
static void              Fpga_CutPrint_( Fpga_Man_t * pMan, Fpga_Cut_t * pCut, Fpga_Node_t * pRoot );

static Fpga_CutTable_t * Fpga_CutTableStart( Fpga_Man_t * pMan );
static void              Fpga_CutTableStop( Fpga_CutTable_t * p );
static unsigned          Fpga_CutTableHash( Fpga_Node_t * ppNodes[], int nNodes );
static int               Fpga_CutTableLookup( Fpga_CutTable_t * p, Fpga_Node_t * ppNodes[], int nNodes );
static Fpga_Cut_t *      Fpga_CutTableConsider( Fpga_Man_t * pMan, Fpga_CutTable_t * p, Fpga_Node_t * ppNodes[], int nNodes );
static void              Fpga_CutTableRestart( Fpga_CutTable_t * p );

static int               Fpga_CutSortCutsCompare( Fpga_Cut_t ** pC1, Fpga_Cut_t ** pC2 );
static Fpga_Cut_t *      Fpga_CutSortCuts( Fpga_Man_t * pMan, Fpga_CutTable_t * p, Fpga_Cut_t * pList );
static int               Fpga_CutList2Array( Fpga_Cut_t ** pArray, Fpga_Cut_t * pList );
static Fpga_Cut_t *      Fpga_CutArray2List( Fpga_Cut_t ** pArray, int nCuts );


// iterator through all the cuts of the list
#define Fpga_ListForEachCut( pList, pCut )                \
    for ( pCut = pList;                                   \
          pCut;                                           \
          pCut = pCut->pNext )
#define Fpga_ListForEachCutSafe( pList, pCut, pCut2 )     \
    for ( pCut = pList,                                   \
          pCut2 = pCut? pCut->pNext: NULL;                \
          pCut;                                           \
          pCut = pCut2,                                   \
          pCut2 = pCut? pCut->pNext: NULL )


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes the cuts for each node in the object graph.]

  Description [The cuts are computed in one sweep over the mapping graph. 
  First, the elementary cuts, which include the node itself, are assigned 
  to the PI nodes. The internal nodes are considered in the DFS order.
  Each node is two-input AND-gate. So to compute the cuts at a node, we
  need to merge the sets of cuts of its two predecessors. The merged set
  contains only unique cuts with the number of inputs equal to k or less.
  Finally, the elementary cut, composed of the node itself, is added to
  the set of cuts for the node.
  
  This procedure is pretty fast for 5-feasible cuts, but it dramatically
  slows down on some "dense" networks when computing 6-feasible cuts.
  The problem is that there are too many cuts in this case. We should
  think how to heuristically trim the number of cuts in such cases, 
  to have reasonable runtime.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingCuts( Fpga_Man_t * p )
{
    ProgressBar * pProgress;
    Fpga_CutTable_t * pTable;
    Fpga_Node_t * pNode;
    int nCuts, nNodes, i;
    clock_t clk = clock();

    // set the elementary cuts for the PI variables
    assert( p->nVarsMax > 1 && p->nVarsMax < 11 );
    Fpga_MappingCreatePiCuts( p );

    // compute the cuts for the internal nodes
    nNodes = p->vAnds->nSize;
    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    pTable = Fpga_CutTableStart( p );
    for ( i = 0; i < nNodes; i++ )
    {
        Extra_ProgressBarUpdate( pProgress, i, "Cuts ..." );
        pNode = p->vAnds->pArray[i];
        if ( !Fpga_NodeIsAnd( pNode ) )
            continue;
        Fpga_CutCompute( p, pTable, pNode );
    }
    Extra_ProgressBarStop( pProgress );
    Fpga_CutTableStop( pTable );

    // report the stats
    if ( p->fVerbose )
    {
        nCuts = Fpga_CutCountAll(p);
        printf( "Nodes = %6d. Total %d-cuts = %d. Cuts per node = %.1f. ", 
               p->nNodes, p->nVarsMax, nCuts, ((float)nCuts)/p->nNodes );
        ABC_PRT( "Time", clock() - clk );
    }

    // print the cuts for the first primary output
//    Fpga_CutListPrint( p, Fpga_Regular(p->pOutputs[0]) );
}

/**Function*************************************************************

  Synopsis    [Performs technology mapping for variable-size-LUTs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_MappingCreatePiCuts( Fpga_Man_t * p )
{
    Fpga_Cut_t * pCut;
    int i;

    // set the elementary cuts for the PI variables
    for ( i = 0; i < p->nInputs; i++ )
    {
        pCut = Fpga_CutAlloc( p );
        pCut->nLeaves = 1;
        pCut->ppLeaves[0] = p->pInputs[i];
        pCut->uSign = (1 << (i%31));
        p->pInputs[i]->pCuts   = pCut;
        p->pInputs[i]->pCutBest = pCut;
        // set the input arrival times
//        p->pInputs[i]->pCut[1]->tArrival = p->pInputArrivals[i];
    }
}

/**Function*************************************************************

  Synopsis    [Computes the cuts for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_CutCompute( Fpga_Man_t * p, Fpga_CutTable_t * pTable, Fpga_Node_t * pNode )
{
    Fpga_Node_t * pTemp;
    Fpga_Cut_t * pList, * pList1, * pList2;
    Fpga_Cut_t * pCut;
    int fTree = 0;
    int fPivot1 = fTree && (Fpga_NodeReadRef(pNode->p1)>2);
    int fPivot2 = fTree && (Fpga_NodeReadRef(pNode->p2)>2);

    // if the cuts are computed return them
    if ( pNode->pCuts )
        return pNode->pCuts;

    // compute the cuts for the children
    pList1 = Fpga_Regular(pNode->p1)->pCuts;
    pList2 = Fpga_Regular(pNode->p2)->pCuts;
    // merge the lists
    pList = Fpga_CutMergeLists( p, pTable, pList1, pList2, 
        Fpga_IsComplement(pNode->p1), Fpga_IsComplement(pNode->p2),
        fPivot1, fPivot2 );
    // if there are functionally equivalent nodes, union them with this list
    assert( pList );
    // only add to the list of cuts if the node is a representative one
    if ( pNode->pRepr == NULL )
    {
        for ( pTemp = pNode->pNextE; pTemp; pTemp = pTemp->pNextE )
        {
            assert( pTemp->pCuts );
            pList = Fpga_CutUnionLists( pList, pTemp->pCuts );
            assert( pTemp->pCuts );
            pList = Fpga_CutSortCuts( p, pTable, pList );
        }
    }
    // add the new cut
    pCut = Fpga_CutAlloc( p );
    pCut->nLeaves = 1;
    pCut->ppLeaves[0] = pNode;
    pCut->uSign = (1 << (pNode->Num%31));
    pCut->fLevel = (float)pCut->ppLeaves[0]->Level;
    // append (it is important that the elementary cut is appended first)
    pCut->pNext = pList;
    // set at the node
    pNode->pCuts = pCut;
    // remove the dominated cuts
//    Fpga_CutFilter( p, pNode );
    // set the phase correctly
    if ( pNode->pRepr && Fpga_NodeComparePhase(pNode, pNode->pRepr) )
    {
        Fpga_ListForEachCut( pNode->pCuts, pCut )
            pCut->Phase = 1;
    }


/*
    { 
        Fpga_Cut_t * pPrev;
        int i, Counter = 0;
        for ( pCut = pNode->pCuts->pNext, pPrev = pNode->pCuts; pCut; pCut = pCut->pNext )
        {
            for ( i = 0; i < pCut->nLeaves; i++ )
                if ( pCut->ppLeaves[i]->Level >= pNode->Level )
                    break;
            if ( i != pCut->nLeaves )
                pPrev->pNext = pCut->pNext;
            else 
                pPrev = pCut; 
        }
    }
    { 
        int i, Counter = 0;;
        for ( pCut = pNode->pCuts->pNext; pCut; pCut = pCut->pNext )
            for ( i = 0; i < pCut->nLeaves; i++ )
                Counter += (pCut->ppLeaves[i]->Level >= pNode->Level);
        if ( Counter )
            printf( " %d", Counter );
    }
*/

    return pCut;
}

/**Function*************************************************************

  Synopsis    [Filter the cuts using dominance.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutFilter( Fpga_Man_t * p, Fpga_Node_t * pNode )
{ 
    Fpga_Cut_t * pTemp, * pPrev, * pCut, * pCut2;
    int i, k, Counter;

    Counter = 0;
    pPrev = pNode->pCuts;
    Fpga_ListForEachCutSafe( pNode->pCuts->pNext, pCut, pCut2 )
    {
        // go through all the previous cuts up to pCut
        for ( pTemp = pNode->pCuts->pNext; pTemp != pCut; pTemp = pTemp->pNext )
        {
            // check if every node in pTemp is contained in pCut
            for ( i = 0; i < pTemp->nLeaves; i++ )
            {
                for ( k = 0; k < pCut->nLeaves; k++ )
                    if ( pTemp->ppLeaves[i] == pCut->ppLeaves[k] )
                        break;
                if ( k == pCut->nLeaves ) // node i in pTemp is not contained in pCut
                    break;
            }
            if ( i == pTemp->nLeaves ) // every node in pTemp is contained in pCut
            {
                Counter++;
                break;
            }
        }
        if ( pTemp != pCut ) // pTemp contain pCut
        {
            pPrev->pNext = pCut->pNext;  // skip pCut
            // recycle pCut
            Fpga_CutFree( p, pCut );
        }
        else 
            pPrev = pCut; 
    }
//  printf( "Dominated = %3d. \n", Counter );
}


/**Function*************************************************************

  Synopsis    [Merges two lists of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_CutMergeLists( Fpga_Man_t * p, Fpga_CutTable_t * pTable, 
    Fpga_Cut_t * pList1, Fpga_Cut_t * pList2, int fComp1, int fComp2, int fPivot1, int fPivot2 )
{
    Fpga_Node_t * ppNodes[FPGA_MAX_LEAVES];
    Fpga_Cut_t * pListNew, ** ppListNew, * pLists[FPGA_MAX_LEAVES+1] = { NULL };
    Fpga_Cut_t * pCut, * pPrev, * pTemp1, * pTemp2;
    int nNodes, Counter, i;
    Fpga_Cut_t ** ppArray1, ** ppArray2, ** ppArray3;
    int nCuts1, nCuts2, nCuts3, k, fComp3;

    ppArray1 = pTable->pCuts1;
    ppArray2 = pTable->pCuts2;
    nCuts1 = Fpga_CutList2Array( ppArray1, pList1 );
    nCuts2 = Fpga_CutList2Array( ppArray2, pList2 );
    if ( fPivot1 )
        nCuts1 = 1;
    if ( fPivot2 )
        nCuts2 = 1;
    // swap the lists based on their length
    if ( nCuts1 > nCuts2 )
    {
         ppArray3 = ppArray1;
         ppArray1 = ppArray2;
         ppArray2 = ppArray3;

         nCuts3 = nCuts1;
         nCuts1 = nCuts2;
         nCuts2 = nCuts3;

         fComp3 = fComp1;
         fComp1 = fComp2;
         fComp2 = fComp3;
    }
    // pList1 is shorter or equal length compared to pList2
 
    // prepare the manager for the cut computation
    Fpga_CutTableRestart( pTable );
    // go through the cut pairs
    Counter = 0;
//    for ( pTemp1 = pList1; pTemp1; pTemp1 = fPivot1? NULL: pTemp1->pNext )
//        for ( pTemp2 = pList2; pTemp2; pTemp2 = fPivot2? NULL: pTemp2->pNext )
    for ( i = 0; i < nCuts1; i++ )
    {
        for ( k = 0; k <= i; k++ )
        {
            pTemp1 = ppArray1[i];
            pTemp2 = ppArray2[k];

            if ( pTemp1->nLeaves == p->nVarsMax && pTemp2->nLeaves == p->nVarsMax )
            {
                if ( pTemp1->ppLeaves[0] != pTemp2->ppLeaves[0] )
                    continue;
                if ( pTemp1->ppLeaves[1] != pTemp2->ppLeaves[1] )
                    continue;
            }

            // check if k-feasible cut exists
            nNodes = Fpga_CutMergeTwo( pTemp1, pTemp2, ppNodes, p->nVarsMax );
            if ( nNodes == 0 )
                continue;
            // consider the cut for possible addition to the set of new cuts
            pCut = Fpga_CutTableConsider( p, pTable, ppNodes, nNodes );
            if ( pCut == NULL )
                continue;
            // add data to the cut
            pCut->pOne = Fpga_CutNotCond( pTemp1, fComp1 );
            pCut->pTwo = Fpga_CutNotCond( pTemp2, fComp2 );
            // create the signature
            pCut->uSign = pTemp1->uSign | pTemp2->uSign;
            // add it to the corresponding list
            pCut->pNext = pLists[(int)pCut->nLeaves];
            pLists[(int)pCut->nLeaves] = pCut;
            // count this cut and quit if limit is reached
            Counter++;
            if ( Counter == FPGA_CUTS_MAX_COMPUTE )
                goto QUITS;
        }
        for ( k = 0; k < i; k++ )
        {
            pTemp1 = ppArray1[k];
            pTemp2 = ppArray2[i];

            if ( pTemp1->nLeaves == p->nVarsMax && pTemp2->nLeaves == p->nVarsMax )
            {
                if ( pTemp1->ppLeaves[0] != pTemp2->ppLeaves[0] )
                    continue;
                if ( pTemp1->ppLeaves[1] != pTemp2->ppLeaves[1] )
                    continue;
            }


            // check if k-feasible cut exists
            nNodes = Fpga_CutMergeTwo( pTemp1, pTemp2, ppNodes, p->nVarsMax );
            if ( nNodes == 0 )
                continue;
            // consider the cut for possible addition to the set of new cuts
            pCut = Fpga_CutTableConsider( p, pTable, ppNodes, nNodes );
            if ( pCut == NULL )
                continue;
            // add data to the cut
            pCut->pOne = Fpga_CutNotCond( pTemp1, fComp1 );
            pCut->pTwo = Fpga_CutNotCond( pTemp2, fComp2 );
            // create the signature
            pCut->uSign = pTemp1->uSign | pTemp2->uSign;
            // add it to the corresponding list
            pCut->pNext = pLists[(int)pCut->nLeaves];
            pLists[(int)pCut->nLeaves] = pCut;
            // count this cut and quit if limit is reached
            Counter++;
            if ( Counter == FPGA_CUTS_MAX_COMPUTE )
                goto QUITS;
        }
    }
    // consider the rest of them
    for ( i = nCuts1; i < nCuts2; i++ )
        for ( k = 0; k < nCuts1; k++ )
        {
            pTemp1 = ppArray1[k];
            pTemp2 = ppArray2[i];

            if ( pTemp1->nLeaves == p->nVarsMax && pTemp2->nLeaves == p->nVarsMax )
            {
                if ( pTemp1->ppLeaves[0] != pTemp2->ppLeaves[0] )
                    continue;
                if ( pTemp1->ppLeaves[1] != pTemp2->ppLeaves[1] )
                    continue;
                if ( pTemp1->ppLeaves[2] != pTemp2->ppLeaves[2] )
                    continue;
            }


            // check if k-feasible cut exists
            nNodes = Fpga_CutMergeTwo( pTemp1, pTemp2, ppNodes, p->nVarsMax );
            if ( nNodes == 0 )
                continue;
            // consider the cut for possible addition to the set of new cuts
            pCut = Fpga_CutTableConsider( p, pTable, ppNodes, nNodes );
            if ( pCut == NULL )
                continue;
            // add data to the cut
            pCut->pOne = Fpga_CutNotCond( pTemp1, fComp1 );
            pCut->pTwo = Fpga_CutNotCond( pTemp2, fComp2 );
            // create the signature
            pCut->uSign = pTemp1->uSign | pTemp2->uSign;
            // add it to the corresponding list
            pCut->pNext = pLists[(int)pCut->nLeaves];
            pLists[(int)pCut->nLeaves] = pCut;
            // count this cut and quit if limit is reached
            Counter++;
            if ( Counter == FPGA_CUTS_MAX_COMPUTE )
                goto QUITS;
        }
QUITS :
    // combine all the lists into one
    pListNew  = NULL;
    ppListNew = &pListNew;
    for ( i = 1; i <= p->nVarsMax; i++ )
    {
        if ( pLists[i] == NULL )
            continue;
        // find the last entry
        for ( pPrev = pLists[i], pCut = pPrev->pNext; pCut; 
            pPrev = pCut, pCut = pCut->pNext );
        // connect these lists
        *ppListNew = pLists[i];
        ppListNew  = &pPrev->pNext;
    }
    *ppListNew = NULL;
    // sort the cuts by arrival times and use only the first FPGA_CUTS_MAX_USE
    pListNew = Fpga_CutSortCuts( p, pTable, pListNew );
    return pListNew;
}


/**Function*************************************************************

  Synopsis    [Merges two lists of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_CutMergeLists2( Fpga_Man_t * p, Fpga_CutTable_t * pTable, 
    Fpga_Cut_t * pList1, Fpga_Cut_t * pList2, int fComp1, int fComp2, int fPivot1, int fPivot2 )
{
    Fpga_Node_t * ppNodes[FPGA_MAX_LEAVES];
    Fpga_Cut_t * pListNew, ** ppListNew, * pLists[FPGA_MAX_LEAVES+1] = { NULL };
    Fpga_Cut_t * pCut, * pPrev, * pTemp1, * pTemp2;
    int nNodes, Counter, i;

    // prepare the manager for the cut computation
    Fpga_CutTableRestart( pTable );
    // go through the cut pairs
    Counter = 0;
    for ( pTemp1 = pList1; pTemp1; pTemp1 = fPivot1? NULL: pTemp1->pNext )
        for ( pTemp2 = pList2; pTemp2; pTemp2 = fPivot2? NULL: pTemp2->pNext )
        {
            // check if k-feasible cut exists
            nNodes = Fpga_CutMergeTwo( pTemp1, pTemp2, ppNodes, p->nVarsMax );
            if ( nNodes == 0 )
                continue;
            // consider the cut for possible addition to the set of new cuts
            pCut = Fpga_CutTableConsider( p, pTable, ppNodes, nNodes );
            if ( pCut == NULL )
                continue;
            // add data to the cut
            pCut->pOne = Fpga_CutNotCond( pTemp1, fComp1 );
            pCut->pTwo = Fpga_CutNotCond( pTemp2, fComp2 );
            // add it to the corresponding list
            pCut->pNext = pLists[(int)pCut->nLeaves];
            pLists[(int)pCut->nLeaves] = pCut;
            // count this cut and quit if limit is reached
            Counter++;
            if ( Counter == FPGA_CUTS_MAX_COMPUTE )
                goto QUITS;
        }
QUITS :
    // combine all the lists into one
    pListNew  = NULL;
    ppListNew = &pListNew;
    for ( i = 1; i <= p->nVarsMax; i++ )
    {
        if ( pLists[i] == NULL )
            continue;
        // find the last entry
        for ( pPrev = pLists[i], pCut = pPrev->pNext; pCut; 
            pPrev = pCut, pCut = pCut->pNext );
        // connect these lists
        *ppListNew = pLists[i];
        ppListNew  = &pPrev->pNext;
    }
    *ppListNew = NULL;
    // sort the cuts by arrival times and use only the first FPGA_CUTS_MAX_USE
    pListNew = Fpga_CutSortCuts( p, pTable, pListNew );
    return pListNew;
}

/**Function*************************************************************

  Synopsis    [Merges two cuts.]

  Description [Returns the number of nodes in the resulting cut, or 0 if the
  cut is infeasible. Returns the resulting nodes in the array ppNodes[].]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CutMergeTwo( Fpga_Cut_t * pCut1, Fpga_Cut_t * pCut2, Fpga_Node_t * ppNodes[], int nNodesMax )
{
    Fpga_Node_t * pNodeTemp;
    int nTotal, i, k, min, Counter;
    unsigned uSign;

    // use quick prefiltering
    uSign = pCut1->uSign | pCut2->uSign;
    Counter = FPGA_COUNT_ONES(uSign);
    if ( Counter > nNodesMax )
        return 0;
/*
    // check the special case when at least of the cuts is the largest
    if ( pCut1->nLeaves == nNodesMax )
    {
        if ( pCut2->nLeaves == nNodesMax )
        {
            // return 0 if the cuts are different
            for ( i = 0; i < nNodesMax; i++ )
                if ( pCut1->ppLeaves[i] != pCut2->ppLeaves[i] )
                    return 0;
            // return nNodesMax if they are the same
            for ( i = 0; i < nNodesMax; i++ )
                ppNodes[i] = pCut1->ppLeaves[i];
            return nNodesMax;
        }
        else if ( pCut2->nLeaves == nNodesMax - 1 ) 
        {
            // return 0 if the cuts are different
            fMismatch = 0;
            for ( i = 0; i < nNodesMax; i++ )
                if ( pCut1->ppLeaves[i] != pCut2->ppLeaves[i - fMismatch] )
                {
                    if ( fMismatch == 1 )
                        return 0;
                    fMismatch = 1;
                }
            // return nNodesMax if they are the same
            for ( i = 0; i < nNodesMax; i++ )
                ppNodes[i] = pCut1->ppLeaves[i];
            return nNodesMax;
        }
    }
    else if ( pCut1->nLeaves == nNodesMax - 1 && pCut2->nLeaves == nNodesMax )
    {
        // return 0 if the cuts are different
        fMismatch = 0;
        for ( i = 0; i < nNodesMax; i++ )
            if ( pCut1->ppLeaves[i - fMismatch] != pCut2->ppLeaves[i] )
            {
                if ( fMismatch == 1 )
                    return 0;
                fMismatch = 1;
            }
        // return nNodesMax if they are the same
        for ( i = 0; i < nNodesMax; i++ )
            ppNodes[i] = pCut2->ppLeaves[i];
        return nNodesMax;
    }
*/
    // count the number of unique entries in pCut2
    nTotal = pCut1->nLeaves;
    for ( i = 0; i < pCut2->nLeaves; i++ )
    {
        // try to find this entry among the leaves of pCut1
        for ( k = 0; k < pCut1->nLeaves; k++ )
            if ( pCut2->ppLeaves[i] == pCut1->ppLeaves[k] )
                break;
        if ( k < pCut1->nLeaves ) // found
            continue;
        // we found a new entry to add
        if ( nTotal == nNodesMax )
            return 0;
        ppNodes[nTotal++] = pCut2->ppLeaves[i];
    }
    // we know that the feasible cut exists

    // add the starting entries
    for ( k = 0; k < pCut1->nLeaves; k++ )
        ppNodes[k] = pCut1->ppLeaves[k];

    // selection-sort the entries
    for ( i = 0; i < nTotal - 1; i++ )
    {
        min = i;
        for ( k = i+1; k < nTotal; k++ )
//            if ( ppNodes[k] < ppNodes[min] ) // reported bug fix (non-determinism!)
            if ( ppNodes[k]->Num < ppNodes[min]->Num )
                min = k;
        pNodeTemp    = ppNodes[i];
        ppNodes[i]   = ppNodes[min];
        ppNodes[min] = pNodeTemp;
    }

    return nTotal;
}

/**Function*************************************************************

  Synopsis    [Computes the union of the two lists of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_CutUnionLists( Fpga_Cut_t * pList1, Fpga_Cut_t * pList2 )
{
    Fpga_Cut_t * pTemp, * pRoot;
    // find the last cut in the first list
    pRoot = pList1;
    Fpga_ListForEachCut( pList1, pTemp )
        pRoot = pTemp;
    // attach the non-trival part of the second cut to the end of the first
    assert( pRoot->pNext == NULL );
    pRoot->pNext = pList2->pNext;   
    pList2->pNext = NULL;
    return pList1;
}


/**Function*************************************************************

  Synopsis    [Checks whether the given cut belongs to the list.]

  Description [This procedure takes most of the runtime in the cut 
  computation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CutBelongsToList( Fpga_Cut_t * pList, Fpga_Node_t * ppNodes[], int nNodes )
{
    Fpga_Cut_t * pTemp;
    int i;
    for ( pTemp = pList; pTemp; pTemp = pTemp->pNext )
    {
        for ( i = 0; i < nNodes; i++ )
            if ( pTemp->ppLeaves[i] != ppNodes[i] )
                break;
        if ( i == nNodes )
            return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Counts all the cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CutCountAll( Fpga_Man_t * pMan )
{
    Fpga_Node_t * pNode;
    Fpga_Cut_t * pCut;
    int i, nCuts;
    // go through all the nodes in the unique table of the manager
    nCuts = 0;
    for ( i = 0; i < pMan->nBins; i++ )
        for ( pNode = pMan->pBins[i]; pNode; pNode = pNode->pNext )
            for ( pCut = pNode->pCuts; pCut; pCut = pCut->pNext )
                if ( pCut->nLeaves > 1 ) // skip the elementary cuts
                {
//                    Fpga_CutVolume( pCut );
                    nCuts++;
                }
    return nCuts;
}


/**Function*************************************************************

  Synopsis    [Clean the signatures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutsCleanSign( Fpga_Man_t * pMan )
{
    Fpga_Node_t * pNode;
    Fpga_Cut_t * pCut;
    int i;
    for ( i = 0; i < pMan->nBins; i++ )
        for ( pNode = pMan->pBins[i]; pNode; pNode = pNode->pNext )
            for ( pCut = pNode->pCuts; pCut; pCut = pCut->pNext )
                pCut->uSign = 0;
}

/**Function*************************************************************

  Synopsis    [Clean the signatures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutsCleanRoot( Fpga_Man_t * pMan )
{
    Fpga_Node_t * pNode;
    Fpga_Cut_t * pCut;
    int i;
    for ( i = 0; i < pMan->nBins; i++ )
        for ( pNode = pMan->pBins[i]; pNode; pNode = pNode->pNext )
            for ( pCut = pNode->pCuts; pCut; pCut = pCut->pNext )
                pCut->pRoot = NULL;
}



/**Function*************************************************************

  Synopsis    [Prints the cuts in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutListPrint( Fpga_Man_t * pMan, Fpga_Node_t * pRoot )
{
    Fpga_Cut_t * pTemp;
    int Counter;
    for ( Counter = 0, pTemp = pRoot->pCuts; pTemp; pTemp = pTemp->pNext, Counter++ )
    {
        printf( "%2d : ", Counter + 1 );
        Fpga_CutPrint_( pMan, pTemp, pRoot );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the cuts in the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutListPrint2( Fpga_Man_t * pMan, Fpga_Node_t * pRoot )
{
    Fpga_Cut_t * pTemp;
    int Counter;
    for ( Counter = 0, pTemp = pRoot->pCuts; pTemp; pTemp = pTemp->pNext, Counter++ )
    {
        printf( "%2d : ", Counter + 1 );
        Fpga_CutPrint_( pMan, pTemp, pRoot );
    }
}

/**Function*************************************************************

  Synopsis    [Prints the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutPrint_( Fpga_Man_t * pMan, Fpga_Cut_t * pCut, Fpga_Node_t * pRoot )
{
    int i;
    printf( "(%3d)  {", pRoot->Num );
    for ( i = 0; i < pMan->nVarsMax; i++ )
        if ( pCut->ppLeaves[i] )
            printf( " %3d", pCut->ppLeaves[i]->Num );
    printf( " }\n" );
}








/**Function*************************************************************

  Synopsis    [Starts the hash table to canonicize cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_CutTable_t * Fpga_CutTableStart( Fpga_Man_t * pMan )
{
    Fpga_CutTable_t * p;
    // allocate the table
    p = ABC_ALLOC( Fpga_CutTable_t, 1 );
    memset( p, 0, sizeof(Fpga_CutTable_t) );
    p->nBins = Abc_PrimeCudd( 10 * FPGA_CUTS_MAX_COMPUTE );
    p->pBins = ABC_ALLOC( Fpga_Cut_t *, p->nBins );
    memset( p->pBins, 0, sizeof(Fpga_Cut_t *) * p->nBins );
    p->pCuts = ABC_ALLOC( int, 2 * FPGA_CUTS_MAX_COMPUTE );
    p->pArray = ABC_ALLOC( Fpga_Cut_t *, 2 * FPGA_CUTS_MAX_COMPUTE );
    p->pCuts1 = ABC_ALLOC( Fpga_Cut_t *, 2 * FPGA_CUTS_MAX_COMPUTE );
    p->pCuts2 = ABC_ALLOC( Fpga_Cut_t *, 2 * FPGA_CUTS_MAX_COMPUTE );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutTableStop( Fpga_CutTable_t * p )
{
    ABC_FREE( p->pCuts1 );
    ABC_FREE( p->pCuts2 );
    ABC_FREE( p->pArray );
    ABC_FREE( p->pBins );
    ABC_FREE( p->pCuts );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Computes the hash value of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Fpga_CutTableHash( Fpga_Node_t * ppNodes[], int nNodes )
{
    unsigned uRes;
    int i;
    uRes = 0;
    for ( i = 0; i < nNodes; i++ )
        uRes += s_HashPrimes[i] * ppNodes[i]->Num;
    return uRes;
}

/**Function*************************************************************

  Synopsis    [Looks up the table for the available cut.]

  Description [Returns -1 if the same cut is found. Returns the index
  of the cell where the cut should be added, if it does not exist.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CutTableLookup( Fpga_CutTable_t * p, Fpga_Node_t * ppNodes[], int nNodes )
{
    Fpga_Cut_t * pCut;
    unsigned Key;
    int b, i;

    Key = Fpga_CutTableHash(ppNodes, nNodes) % p->nBins; 
    for ( b = Key; p->pBins[b]; b = (b+1) % p->nBins )
    {
        pCut = p->pBins[b];
        if ( pCut->nLeaves != nNodes )
            continue;
        for ( i = 0; i < nNodes; i++ )
            if ( pCut->ppLeaves[i] != ppNodes[i] )
                break;
        if ( i == nNodes )
            return -1;
    }
    return b;
}


/**Function*************************************************************

  Synopsis    [Starts the hash table to canonicize cuts.]

  Description [Considers addition of the cut to the hash table.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_CutTableConsider( Fpga_Man_t * pMan, Fpga_CutTable_t * p, Fpga_Node_t * ppNodes[], int nNodes )
{
    Fpga_Cut_t * pCut;
    int Place, i;
    // check the cut
    Place = Fpga_CutTableLookup( p, ppNodes, nNodes );
    if ( Place == -1 )
        return NULL;
    assert( nNodes > 0 );
    // create the new cut
    pCut = Fpga_CutAlloc( pMan );
    pCut->nLeaves = nNodes;
    pCut->fLevel = 0.0;
    for ( i = 0; i < nNodes; i++ )
    {
        pCut->ppLeaves[i] = ppNodes[i];
        pCut->fLevel += ppNodes[i]->Level;
    }
    pCut->fLevel /= nNodes;
    // add the cut to the table
    assert( p->pBins[Place] == NULL );
    p->pBins[Place] = pCut;
    // add the cut to the new list
    p->pCuts[ p->nCuts++ ] = Place;
    return pCut;
}

/**Function*************************************************************

  Synopsis    [Prepares the table to be used with other cuts.]

  Description [Restarts the table by cleaning the info about cuts stored
  when the previous node was considered.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_CutTableRestart( Fpga_CutTable_t * p )
{
    int i;
    for ( i = 0; i < p->nCuts; i++ )
    {
        assert( p->pBins[ p->pCuts[i] ] );
        p->pBins[ p->pCuts[i] ] = NULL;
    }
    p->nCuts = 0;
}



/**Function*************************************************************

  Synopsis    [Compares the cuts by the number of leaves and then by delay.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CutSortCutsCompare( Fpga_Cut_t ** pC1, Fpga_Cut_t ** pC2 )
{
    if ( (*pC1)->nLeaves < (*pC2)->nLeaves )
        return -1;
    if ( (*pC1)->nLeaves > (*pC2)->nLeaves )
        return 1;
/*
    if ( (*pC1)->fLevel > (*pC2)->fLevel )
        return -1;
    if ( (*pC1)->fLevel < (*pC2)->fLevel )
        return 1;
*/
    return 0;
}

/**Function*************************************************************

  Synopsis    [Sorts the cuts by average arrival time.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_CutSortCuts( Fpga_Man_t * pMan, Fpga_CutTable_t * p, Fpga_Cut_t * pList )
{
    Fpga_Cut_t * pListNew;
    int nCuts, i;
    // move the cuts from the list into the array
    nCuts = Fpga_CutList2Array( p->pCuts1, pList );
    assert( nCuts <= FPGA_CUTS_MAX_COMPUTE );
    // sort the cuts
    qsort( (void *)p->pCuts1, (size_t)nCuts, sizeof(void *), 
            (int (*)(const void *, const void *)) Fpga_CutSortCutsCompare );
    // move them back into the list
    if ( nCuts > FPGA_CUTS_MAX_USE - 1 )
    {
//        printf( "*" );
        // free the remaining cuts
        for ( i = FPGA_CUTS_MAX_USE - 1; i < nCuts; i++ )
            Extra_MmFixedEntryRecycle( pMan->mmCuts, (char *)p->pCuts1[i] );
        // update the number of cuts
        nCuts = FPGA_CUTS_MAX_USE - 1;
    }
    pListNew = Fpga_CutArray2List( p->pCuts1, nCuts );
    return pListNew;
}

/**Function*************************************************************

  Synopsis    [Moves the nodes from the list into the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_CutList2Array( Fpga_Cut_t ** pArray, Fpga_Cut_t * pList )
{
    int i;
    for ( i = 0; pList; pList = pList->pNext, i++ )
        pArray[i] = pList;
    return i;
}

/**Function*************************************************************

  Synopsis    [Moves the nodes from the array into the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Cut_t * Fpga_CutArray2List( Fpga_Cut_t ** pArray, int nCuts )
{
    Fpga_Cut_t * pListNew, ** ppListNew;
    int i;
    pListNew  = NULL;
    ppListNew = &pListNew;
    for ( i = 0; i < nCuts; i++ )
    {
        // connect these lists
        *ppListNew = pArray[i];
        ppListNew  = &pArray[i]->pNext;
//printf( " %d(%.2f)", pArray[i]->nLeaves, pArray[i]->fLevel );
    }
//printf( "\n" );

    *ppListNew = NULL;
    return pListNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

