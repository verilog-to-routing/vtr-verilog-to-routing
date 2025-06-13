/**CFile****************************************************************

  FileName    [ivyCutTrav.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyCutTrav.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static unsigned * Ivy_NodeCutElementary( Vec_Int_t * vStore, int nWords, int NodeId );
static void Ivy_NodeComputeVolume( Ivy_Obj_t * pObj, int nNodeLimit, Vec_Ptr_t * vNodes, Vec_Ptr_t * vFront );
static void Ivy_NodeFindCutsMerge( Vec_Ptr_t * vCuts0, Vec_Ptr_t * vCuts1, Vec_Ptr_t * vCuts, int nLeaves, int nWords, Vec_Int_t * vStore );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes cuts for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Store_t * Ivy_NodeFindCutsTravAll( Ivy_Man_t * p, Ivy_Obj_t * pObj, int nLeaves, int nNodeLimit, 
                                      Vec_Ptr_t * vNodes, Vec_Ptr_t * vFront, Vec_Int_t * vStore, Vec_Vec_t * vBitCuts )
{
    static Ivy_Store_t CutStore, * pCutStore = &CutStore;
    Vec_Ptr_t * vCuts, * vCuts0, * vCuts1;
    unsigned * pBitCut;
    Ivy_Obj_t * pLeaf;
    Ivy_Cut_t * pCut;
    int i, k, nWords, nNodes;

    assert( nLeaves <= IVY_CUT_INPUT );

    // find the given number of nodes in the TFI
    Ivy_NodeComputeVolume( pObj, nNodeLimit - 1, vNodes, vFront );
    nNodes = Vec_PtrSize(vNodes);
//    assert( nNodes <= nNodeLimit );

    // make sure vBitCuts has enough room
    Vec_VecExpand( vBitCuts, nNodes-1 );
    Vec_VecClear( vBitCuts );

    // prepare the memory manager
    Vec_IntClear( vStore );
    Vec_IntGrow( vStore, 64000 );

    // set elementary cuts for the leaves
    nWords = Extra_BitWordNum( nNodes );
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pLeaf, i )
    {
        assert( Ivy_ObjTravId(pLeaf) < nNodes );
        // get the new bitcut
        pBitCut = Ivy_NodeCutElementary( vStore, nWords, Ivy_ObjTravId(pLeaf) );
        // set it as the cut of this leaf
        Vec_VecPush( vBitCuts, Ivy_ObjTravId(pLeaf), pBitCut );
    }

    // compute the cuts for each node
    Vec_PtrForEachEntry( Ivy_Obj_t *, vNodes, pLeaf, i )
    {
        // skip the leaves
        vCuts = Vec_VecEntry( vBitCuts, Ivy_ObjTravId(pLeaf) );
        if ( Vec_PtrSize(vCuts) > 0 )
            continue;
        // add elementary cut
        pBitCut = Ivy_NodeCutElementary( vStore, nWords, Ivy_ObjTravId(pLeaf) );
        // set it as the cut of this leaf
        Vec_VecPush( vBitCuts, Ivy_ObjTravId(pLeaf), pBitCut );
        // get the fanin cuts
        vCuts0 = Vec_VecEntry( vBitCuts, Ivy_ObjTravId( Ivy_ObjFanin0(pLeaf) ) );
        vCuts1 = Vec_VecEntry( vBitCuts, Ivy_ObjTravId( Ivy_ObjFanin1(pLeaf) ) );
        assert( Vec_PtrSize(vCuts0) > 0 );
        assert( Vec_PtrSize(vCuts1) > 0 );
        // merge the cuts
        Ivy_NodeFindCutsMerge( vCuts0, vCuts1, vCuts, nLeaves, nWords, vStore );
    }

    // start the structure
    pCutStore->nCuts = 0;
    pCutStore->nCutsMax = IVY_CUT_LIMIT;
    // collect the cuts of the root node
    vCuts = Vec_VecEntry( vBitCuts, Ivy_ObjTravId(pObj) );
    Vec_PtrForEachEntry( unsigned *, vCuts, pBitCut, i )
    {
        pCut = pCutStore->pCuts + pCutStore->nCuts++;
        pCut->nSize = 0;
        pCut->nSizeMax = nLeaves;
        pCut->uHash = 0;
        for ( k = 0; k < nNodes; k++ )
            if ( Extra_TruthHasBit(pBitCut, k) )
                pCut->pArray[ pCut->nSize++ ] = Ivy_ObjId( (Ivy_Obj_t *)Vec_PtrEntry(vNodes, k) );
        assert( pCut->nSize <= nLeaves );
        if ( pCutStore->nCuts == pCutStore->nCutsMax )
            break;
    }

    // clean the travIds
    Vec_PtrForEachEntry( Ivy_Obj_t *, vNodes, pLeaf, i )
        pLeaf->TravId = 0;
    return pCutStore;
}

/**Function*************************************************************

  Synopsis    [Creates elementary bit-cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Ivy_NodeCutElementary( Vec_Int_t * vStore, int nWords, int NodeId )
{
    unsigned * pBitCut;
    pBitCut = Vec_IntFetch( vStore, nWords );
    memset( pBitCut, 0, (size_t)(4 * nWords) );
    Extra_TruthSetBit( pBitCut, NodeId );
    return pBitCut;
}

/**Function*************************************************************

  Synopsis    [Compares the node by level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_CompareNodesByLevel( Ivy_Obj_t ** ppObj1, Ivy_Obj_t ** ppObj2 )
{
    Ivy_Obj_t * pObj1 = *ppObj1;
    Ivy_Obj_t * pObj2 = *ppObj2;
    if ( pObj1->Level < pObj2->Level )
        return -1;
    if ( pObj1->Level > pObj2->Level )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Mark all nodes up to the given depth.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeComputeVolumeTrav1_rec( Ivy_Obj_t * pObj, int Depth )
{
    if ( Ivy_ObjIsCi(pObj) || Depth == 0 )
        return;
    Ivy_NodeComputeVolumeTrav1_rec( Ivy_ObjFanin0(pObj), Depth - 1 );
    Ivy_NodeComputeVolumeTrav1_rec( Ivy_ObjFanin1(pObj), Depth - 1 );
    pObj->fMarkA = 1;
}

/**Function*************************************************************

  Synopsis    [Collect the marked nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeComputeVolumeTrav2_rec( Ivy_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    if ( !pObj->fMarkA )
        return;
    Ivy_NodeComputeVolumeTrav2_rec( Ivy_ObjFanin0(pObj), vNodes );
    Ivy_NodeComputeVolumeTrav2_rec( Ivy_ObjFanin1(pObj), vNodes );
    Vec_PtrPush( vNodes, pObj );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeComputeVolume( Ivy_Obj_t * pObj, int nNodeLimit, Vec_Ptr_t * vNodes, Vec_Ptr_t * vFront )
{
    Ivy_Obj_t * pTemp, * pFanin;
    int i, nNodes;
    // mark nodes up to the given depth
    Ivy_NodeComputeVolumeTrav1_rec( pObj, 6 );
    // collect the marked nodes
    Vec_PtrClear( vFront );
    Ivy_NodeComputeVolumeTrav2_rec( pObj, vFront );
    // find the fanins that are not marked
    Vec_PtrClear( vNodes );
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pTemp, i )
    {
        pFanin = Ivy_ObjFanin0(pTemp);
        if ( !pFanin->fMarkA )
        {
            pFanin->fMarkA = 1;
            Vec_PtrPush( vNodes, pFanin );
        }
        pFanin = Ivy_ObjFanin1(pTemp);
        if ( !pFanin->fMarkA )
        {
            pFanin->fMarkA = 1;
            Vec_PtrPush( vNodes, pFanin );
        }
    }
    // remember the number of nodes in the frontier
    nNodes = Vec_PtrSize( vNodes );
    // add the remaining nodes
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pTemp, i )
        Vec_PtrPush( vNodes, pTemp );
    // unmark the nodes
    Vec_PtrForEachEntry( Ivy_Obj_t *, vNodes, pTemp, i )
    {
        pTemp->fMarkA = 0;
        pTemp->TravId = i;
    }
    // collect the frontier nodes
    Vec_PtrClear( vFront );
    Vec_PtrForEachEntryStop( Ivy_Obj_t *, vNodes, pTemp, i, nNodes )
        Vec_PtrPush( vFront, pTemp );
//    printf( "%d ", Vec_PtrSize(vNodes) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeComputeVolume2( Ivy_Obj_t * pObj, int nNodeLimit, Vec_Ptr_t * vNodes, Vec_Ptr_t * vFront )
{
    Ivy_Obj_t * pLeaf, * pPivot, * pFanin;
    int LevelMax, i;
    assert( Ivy_ObjIsNode(pObj) );
    // clear arrays
    Vec_PtrClear( vNodes );
    Vec_PtrClear( vFront );
    // add the root
    pObj->fMarkA = 1;
    Vec_PtrPush( vNodes, pObj );
    Vec_PtrPush( vFront, pObj );
    // expand node with maximum level
    LevelMax = pObj->Level;
    do {
        // get the node to expand
        pPivot = NULL;
        Vec_PtrForEachEntryReverse( Ivy_Obj_t *, vFront, pLeaf, i )
        {
            if ( (int)pLeaf->Level == LevelMax )
            {
                pPivot = pLeaf;
                break;
            }
        }
        // decrease level if we did not find the node
        if ( pPivot == NULL )
        {
            if ( --LevelMax == 0 )
                break;
            continue;
        }
        // the node to expand is found
        // remove it from frontier
        Vec_PtrRemove( vFront, pPivot );
        // add fanins
        pFanin = Ivy_ObjFanin0(pPivot); 
        if ( !pFanin->fMarkA )
        {
            pFanin->fMarkA = 1;
            Vec_PtrPush( vNodes, pFanin );
            Vec_PtrPush( vFront, pFanin );
        }
        pFanin = Ivy_ObjFanin1(pPivot); 
        if ( pFanin && !pFanin->fMarkA )
        {
            pFanin->fMarkA = 1;
            Vec_PtrPush( vNodes, pFanin );
            Vec_PtrPush( vFront, pFanin );
        }
        // quit if we collected enough nodes
    } while ( Vec_PtrSize(vNodes) < nNodeLimit );

    // sort nodes by level
    Vec_PtrSort( vNodes, (int (*)(const void *, const void *))Ivy_CompareNodesByLevel );
    // make sure the nodes are ordered in the increasing number of levels
    pFanin = (Ivy_Obj_t *)Vec_PtrEntry( vNodes, 0 );
    pPivot = (Ivy_Obj_t *)Vec_PtrEntryLast( vNodes );
    assert( pFanin->Level <= pPivot->Level );

    // clean the marks and remember node numbers in the TravId
    Vec_PtrForEachEntry( Ivy_Obj_t *, vNodes, pFanin, i )
    {
        pFanin->fMarkA = 0;
        pFanin->TravId = i;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Extra_TruthOrWords( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nWords )
{
    int w;
    for ( w = nWords-1; w >= 0; w-- )
        pOut[w] = pIn0[w] | pIn1[w];
}
static inline int Extra_TruthIsImplyWords( unsigned * pIn1, unsigned * pIn2, int nWords )
{
    int w;
    for ( w = nWords-1; w >= 0; w-- )
        if ( pIn1[w] & ~pIn2[w] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Merges two sets of bit-cuts at a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeFindCutsMerge( Vec_Ptr_t * vCuts0, Vec_Ptr_t * vCuts1, Vec_Ptr_t * vCuts, 
                           int nLeaves, int nWords, Vec_Int_t * vStore )
{
    unsigned * pBitCut, * pBitCut0, * pBitCut1, * pBitCutTest;
    int i, k, c, w, Counter;
    // iterate through the cut pairs
    Vec_PtrForEachEntry( unsigned *, vCuts0, pBitCut0, i )
    Vec_PtrForEachEntry( unsigned *, vCuts1, pBitCut1, k )
    {
        // skip infeasible cuts
        Counter = 0;
        for ( w = 0; w < nWords; w++ )
        {
            Counter += Extra_WordCountOnes( pBitCut0[w] | pBitCut1[w] );
            if ( Counter > nLeaves )
                break;
        }
        if ( Counter > nLeaves )
            continue;
        // the new cut is feasible - create it
        pBitCutTest = Vec_IntFetch( vStore, nWords );
        Extra_TruthOrWords( pBitCutTest, pBitCut0, pBitCut1, nWords );
        // filter contained cuts; try to find containing cut
        w = 0;
        Vec_PtrForEachEntry( unsigned *, vCuts, pBitCut, c )
        {
            if ( Extra_TruthIsImplyWords( pBitCut, pBitCutTest, nWords ) )
                break;
            if ( Extra_TruthIsImplyWords( pBitCutTest, pBitCut, nWords ) )
                continue;
            Vec_PtrWriteEntry( vCuts, w++, pBitCut );
        }
        if ( c != Vec_PtrSize(vCuts) )
            continue;
        Vec_PtrShrink( vCuts, w );
        // add the cut
        Vec_PtrPush( vCuts, pBitCutTest );
    }
}
 
/**Function*************************************************************

  Synopsis    [Compute the set of all cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_ManTestCutsTravAll( Ivy_Man_t * p )
{
    Ivy_Store_t * pStore;
    Ivy_Obj_t * pObj;
    Vec_Ptr_t * vNodes, * vFront;
    Vec_Int_t * vStore;
    Vec_Vec_t * vBitCuts;
    int i, nCutsCut, nCutsTotal, nNodeTotal, nNodeOver;
    abctime clk = Abc_Clock();

    vNodes = Vec_PtrAlloc( 100 );
    vFront = Vec_PtrAlloc( 100 );
    vStore = Vec_IntAlloc( 100 );
    vBitCuts = Vec_VecAlloc( 100 );

    nNodeTotal = nNodeOver = 0;
    nCutsTotal = -Ivy_ManNodeNum(p);
    Ivy_ManForEachObj( p, pObj, i )
    {
        if ( !Ivy_ObjIsNode(pObj) )
            continue;
        pStore = Ivy_NodeFindCutsTravAll( p, pObj, 4, 60, vNodes, vFront, vStore, vBitCuts );
        nCutsCut    = pStore->nCuts;
        nCutsTotal += nCutsCut;
        nNodeOver  += (nCutsCut == IVY_CUT_LIMIT);
        nNodeTotal++;
    }
    printf( "Total cuts = %6d. Trivial = %6d.   Nodes = %6d. Satur = %6d.  ", 
        nCutsTotal, Ivy_ManPiNum(p) + Ivy_ManNodeNum(p), nNodeTotal, nNodeOver );
    ABC_PRT( "Time", Abc_Clock() - clk );

    Vec_PtrFree( vNodes );
    Vec_PtrFree( vFront );
    Vec_IntFree( vStore );
    Vec_VecFree( vBitCuts );

}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

