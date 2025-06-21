/**CFile****************************************************************

  FileName    [resWin.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resynthesis package.]

  Synopsis    [Windowing algorithm.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 15, 2007.]

  Revision    [$Id: resWin.c,v 1.00 2007/01/15 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "resInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Res_Win_t * Res_WinAlloc()
{
    Res_Win_t * p;
    // start the manager
    p = ABC_ALLOC( Res_Win_t, 1 );
    memset( p, 0, sizeof(Res_Win_t) );
    // set internal parameters
    p->nFanoutLimit = 10;
    p->nLevTfiMinus =  3;
    // allocate storage
    p->vRoots    = Vec_PtrAlloc( 256 );
    p->vLeaves   = Vec_PtrAlloc( 256 );
    p->vBranches = Vec_PtrAlloc( 256 );
    p->vNodes    = Vec_PtrAlloc( 256 );
    p->vDivs     = Vec_PtrAlloc( 256 );
    p->vMatrix   = Vec_VecStart( 128 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_WinFree( Res_Win_t * p )
{
    Vec_PtrFree( p->vRoots  );
    Vec_PtrFree( p->vLeaves );
    Vec_PtrFree( p->vBranches );
    Vec_PtrFree( p->vNodes  );
    Vec_PtrFree( p->vDivs   );
    Vec_VecFree( p->vMatrix );
    ABC_FREE( p );
}



/**Function*************************************************************

  Synopsis    [Collect the limited TFI cone of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_WinCollectLeavesAndNodes( Res_Win_t * p )
{
    Vec_Ptr_t * vFront;
    Abc_Obj_t * pObj, * pTemp;
    int i, k, m;

    assert( p->nWinTfiMax > 0 );
    assert( Vec_VecSize(p->vMatrix) > p->nWinTfiMax );

    // start matrix with the node
    Vec_VecClear( p->vMatrix );
    Vec_VecPush( p->vMatrix, 0, p->pNode );
    Abc_NtkIncrementTravId( p->pNode->pNtk );
    Abc_NodeSetTravIdCurrent( p->pNode );

    // collect the leaves (nodes pTemp such that "p->pNode->Level - pTemp->Level > p->nWinTfiMax")
    Vec_PtrClear( p->vLeaves );
    Vec_VecForEachLevelStartStop( p->vMatrix, vFront, i, 0, p->nWinTfiMax+1 )
    {
        Vec_PtrForEachEntry( Abc_Obj_t *, vFront, pObj, k )
        {
            Abc_ObjForEachFanin( pObj, pTemp, m )
            {
                if ( Abc_NodeIsTravIdCurrent( pTemp ) )
                    continue;
                Abc_NodeSetTravIdCurrent( pTemp );
                if ( Abc_ObjIsCi(pTemp) || (int)(p->pNode->Level - pTemp->Level) > p->nWinTfiMax )                    
                    Vec_PtrPush( p->vLeaves, pTemp );
                else
                    Vec_VecPush( p->vMatrix, p->pNode->Level - pTemp->Level, pTemp );
            }
        }
    }
    if ( Vec_PtrSize(p->vLeaves) == 0 )
        return 0;

    // collect the nodes in the reverse order
    Vec_PtrClear( p->vNodes );
    Vec_VecForEachLevelReverseStartStop( p->vMatrix, vFront, i, p->nWinTfiMax+1, 0 )
    {
        Vec_PtrForEachEntry( Abc_Obj_t *, vFront, pObj, k )
            Vec_PtrPush( p->vNodes, pObj );
        Vec_PtrClear( vFront );
    }

    // get the lowest leaf level
    p->nLevLeafMin = ABC_INFINITY;
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pObj, k )
        p->nLevLeafMin = Abc_MinInt( p->nLevLeafMin, (int)pObj->Level );

    // set minimum traversal level
    p->nLevTravMin = Abc_MaxInt( ((int)p->pNode->Level) - p->nWinTfiMax - p->nLevTfiMinus, p->nLevLeafMin );
    assert( p->nLevTravMin >= 0 );
    return 1;
}



/**Function*************************************************************

  Synopsis    [Returns 1 if the node should be a root.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Res_WinComputeRootsCheck( Abc_Obj_t * pNode, int nLevelMax, int nFanoutLimit )
{
    Abc_Obj_t * pFanout;
    int i;
    // the node is the root if one of the following is true:
    // (1) the node has more than fanouts than the limit
    if ( Abc_ObjFanoutNum(pNode) > nFanoutLimit )
        return 1;
    // (2) the node has CO fanouts
    // (3) the node has fanouts above the cutoff level
    Abc_ObjForEachFanout( pNode, pFanout, i )
        if ( Abc_ObjIsCo(pFanout) || (int)pFanout->Level > nLevelMax )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Recursively collects the root candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_WinComputeRoots_rec( Abc_Obj_t * pNode, int nLevelMax, int nFanoutLimit, Vec_Ptr_t * vRoots )
{
    Abc_Obj_t * pFanout;
    int i;
    assert( Abc_ObjIsNode(pNode) );
    if ( Abc_NodeIsTravIdCurrent(pNode) )
        return;
    Abc_NodeSetTravIdCurrent( pNode );
    // check if the node should be the root
    if ( Res_WinComputeRootsCheck( pNode, nLevelMax, nFanoutLimit ) )
        Vec_PtrPush( vRoots, pNode );
    else // if not, explore its fanouts
        Abc_ObjForEachFanout( pNode, pFanout, i )
            Res_WinComputeRoots_rec( pFanout, nLevelMax, nFanoutLimit, vRoots );
}

/**Function*************************************************************

  Synopsis    [Recursively collects the root candidates.]

  Description [Returns 1 if the only root is this node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_WinComputeRoots( Res_Win_t * p )
{
    Vec_PtrClear( p->vRoots );
    Abc_NtkIncrementTravId( p->pNode->pNtk );
    Res_WinComputeRoots_rec( p->pNode, p->pNode->Level + p->nWinTfoMax, p->nFanoutLimit, p->vRoots );
    assert( Vec_PtrSize(p->vRoots) > 0 );
    if ( Vec_PtrSize(p->vRoots) == 1 && Vec_PtrEntry(p->vRoots, 0) == p->pNode )
        return 0;
    return 1;
}



/**Function*************************************************************

  Synopsis    [Marks the paths from the roots to the leaves.]

  Description [Returns 1 if the the node can reach a leaf.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_WinMarkPaths_rec( Abc_Obj_t * pNode, Abc_Obj_t * pPivot, int nLevelMin )
{
    Abc_Obj_t * pFanin;
    int i, RetValue;
    // skip visited nodes
    if ( Abc_NodeIsTravIdCurrent(pNode) )
        return 1;
    if ( Abc_NodeIsTravIdPrevious(pNode) )
        return 0;
    // assume that the node does not have access to the leaves
    Abc_NodeSetTravIdPrevious( pNode );
    // skip nodes below the given level
    if ( pNode == pPivot || (int)pNode->Level <= nLevelMin )
        return 0;
    assert( Abc_ObjIsNode(pNode) );
    // check if the fanins have access to the leaves
    RetValue = 0;
    Abc_ObjForEachFanin( pNode, pFanin, i )
        RetValue |= Res_WinMarkPaths_rec( pFanin, pPivot, nLevelMin );
    // relabel the node if it has access to the leaves
    if ( RetValue )
        Abc_NodeSetTravIdCurrent( pNode );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Marks the paths from the roots to the leaves.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_WinMarkPaths( Res_Win_t * p )
{
    Abc_Obj_t * pObj;
    int i;
    // mark the leaves
    Abc_NtkIncrementTravId( p->pNode->pNtk );
    Abc_NtkIncrementTravId( p->pNode->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pObj, i )
        Abc_NodeSetTravIdCurrent( pObj );
    // traverse from the roots and mark the nodes that can reach leaves
    // the nodes that do not reach leaves have previous trav ID
    // the nodes that reach leaves have current trav ID
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vRoots, pObj, i )
        Res_WinMarkPaths_rec( pObj, p->pNode, p->nLevTravMin );
}




/**Function*************************************************************

  Synopsis    [Recursively collects the roots.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_WinFinalizeRoots_rec( Abc_Obj_t * pObj, Vec_Ptr_t * vRoots )
{
    Abc_Obj_t * pFanout;
    int i;
    assert( Abc_ObjIsNode(pObj) );
    assert( Abc_NodeIsTravIdCurrent(pObj) );
    // check if the node has all fanouts marked
    Abc_ObjForEachFanout( pObj, pFanout, i )
        if ( !Abc_NodeIsTravIdCurrent(pFanout) )
            break;
    // if some of the fanouts are unmarked, add the node to the roots
    if ( i < Abc_ObjFanoutNum(pObj) ) 
        Vec_PtrPushUnique( vRoots, pObj );
    else  // otherwise, call recursively
        Abc_ObjForEachFanout( pObj, pFanout, i )
            Res_WinFinalizeRoots_rec( pFanout, vRoots );
}

/**Function*************************************************************

  Synopsis    [Finalizes the roots of the window.]

  Description [Roots of the window are the nodes that have at least
  one fanout that it not in the TFO of the leaves.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_WinFinalizeRoots( Res_Win_t * p )
{
    assert( !Abc_NodeIsTravIdCurrent(p->pNode) );
    // mark the node with the old traversal ID
    Abc_NodeSetTravIdCurrent( p->pNode ); 
    // recollect the roots
    Vec_PtrClear( p->vRoots );
    Res_WinFinalizeRoots_rec( p->pNode, p->vRoots );
    assert( Vec_PtrSize(p->vRoots) > 0 );
    if ( Vec_PtrSize(p->vRoots) == 1 && Vec_PtrEntry(p->vRoots, 0) == p->pNode )
        return 0;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Recursively adds missing nodes and leaves.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_WinAddMissing_rec( Res_Win_t * p, Abc_Obj_t * pObj, int nLevTravMin )
{
    Abc_Obj_t * pFanin;
    int i;
    // skip the already collected leaves, nodes, and branches
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return;
    // if this is not an internal node - make it a new branch
    if ( !Abc_NodeIsTravIdPrevious(pObj) )
    {
        assert( Vec_PtrFind(p->vLeaves, pObj) == -1 );
        Abc_NodeSetTravIdCurrent( pObj );
        Vec_PtrPush( p->vBranches, pObj );
        return;
    }
    assert( Abc_ObjIsNode(pObj) ); // if this is a CI, then the window is incorrect!
    Abc_NodeSetTravIdCurrent( pObj );
    // visit the fanins of the node
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Res_WinAddMissing_rec( p, pFanin, nLevTravMin );
    // collect the node
    Vec_PtrPush( p->vNodes, pObj );
}

/**Function*************************************************************

  Synopsis    [Adds to the window nodes and leaves in the TFI of the roots.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_WinAddMissing( Res_Win_t * p )
{
    Abc_Obj_t * pObj;
    int i;
    // mark the leaves
    Abc_NtkIncrementTravId( p->pNode->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pObj, i )
        Abc_NodeSetTravIdCurrent( pObj );        
    // mark the already collected nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vNodes, pObj, i )
        Abc_NodeSetTravIdCurrent( pObj );        
    // explore from the roots
    Vec_PtrClear( p->vBranches );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vRoots, pObj, i )
        Res_WinAddMissing_rec( p, pObj, p->nLevTravMin );
}



 
/**Function*************************************************************

  Synopsis    [Returns 1 if the window is trivial (without TFO).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_WinIsTrivial( Res_Win_t * p )
{
    return Vec_PtrSize(p->vRoots) == 1 && Vec_PtrEntry(p->vRoots, 0) == p->pNode;
}

/**Function*************************************************************

  Synopsis    [Computes the window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_WinCompute( Abc_Obj_t * pNode, int nWinTfiMax, int nWinTfoMax, Res_Win_t * p )
{
    assert( Abc_ObjIsNode(pNode) );
    assert( nWinTfiMax > 0 && nWinTfiMax < 10 );
    assert( nWinTfoMax >= 0 && nWinTfoMax < 10 );

    // initialize the window
    p->pNode      = pNode;
    p->nWinTfiMax = nWinTfiMax;
    p->nWinTfoMax = nWinTfoMax;

    Vec_PtrClear( p->vBranches );
    Vec_PtrClear( p->vDivs );
    Vec_PtrClear( p->vRoots );
    Vec_PtrPush( p->vRoots, pNode );

    // compute the leaves
    if ( !Res_WinCollectLeavesAndNodes( p ) )
        return 0;

    // compute the candidate roots
    if ( p->nWinTfoMax > 0 && Res_WinComputeRoots(p) )
    {
        // mark the paths from the roots to the leaves
        Res_WinMarkPaths( p );
        // refine the roots and add branches and missing nodes
        if ( Res_WinFinalizeRoots( p ) )
            Res_WinAddMissing( p );
    }

    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

