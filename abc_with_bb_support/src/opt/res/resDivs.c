/**CFile****************************************************************

  FileName    [resDivs.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resynthesis package.]

  Synopsis    [Collect divisors for the given window.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 15, 2007.]

  Revision    [$Id: resDivs.c,v 1.00 2007/01/15 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "resInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Res_WinMarkTfi( Res_Win_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Adds candidate divisors of the node to its window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_WinDivisors( Res_Win_t * p, int nLevDivMax )
{
    Abc_Obj_t * pObj, * pFanout, * pFanin;
    int k, f, m;

    // set the maximum level of the divisors
    p->nLevDivMax = nLevDivMax;

    // mark the TFI with the current trav ID
    Abc_NtkIncrementTravId( p->pNode->pNtk );
    Res_WinMarkTfi( p );

    // mark with the current trav ID those nodes that should not be divisors:
    // (1) the node and its TFO
    // (2) the MFFC of the node
    // (3) the node's fanins (these are treated as a special case)
    Abc_NtkIncrementTravId( p->pNode->pNtk );
    Res_WinSweepLeafTfo_rec( p->pNode, p->nLevDivMax );
    Res_WinVisitMffc( p->pNode );
    Abc_ObjForEachFanin( p->pNode, pObj, k )
        Abc_NodeSetTravIdCurrent( pObj );

    // at this point the nodes are marked with two trav IDs:
    // nodes to be collected as divisors are marked with previous trav ID
    // nodes to be avoided as divisors are marked with current trav ID

    // start collecting the divisors
    Vec_PtrClear( p->vDivs );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pObj, k )
    {
        assert( (int)pObj->Level >= p->nLevLeafMin ); 
        if ( !Abc_NodeIsTravIdPrevious(pObj) )
            continue;
        if ( (int)pObj->Level > p->nLevDivMax )
            continue;
        Vec_PtrPush( p->vDivs, pObj );
    }
    // add the internal nodes to the data structure
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vNodes, pObj, k )
    {
        if ( !Abc_NodeIsTravIdPrevious(pObj) )
            continue;
        if ( (int)pObj->Level > p->nLevDivMax )
            continue;
        Vec_PtrPush( p->vDivs, pObj );
    }

    // explore the fanouts of already collected divisors
    p->nDivsPlus = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vDivs, pObj, k )
    {
        // consider fanouts of this node
        Abc_ObjForEachFanout( pObj, pFanout, f )
        {
            // stop if there are too many fanouts
            if ( f > 20 )
                break;
            // skip nodes that are already added
            if ( Abc_NodeIsTravIdPrevious(pFanout) )
                continue;
            // skip nodes in the TFO or in the MFFC of node
            if ( Abc_NodeIsTravIdCurrent(pFanout) )
                continue;
            // skip COs
            if ( !Abc_ObjIsNode(pFanout) ) 
                continue;
            // skip nodes with large level
            if ( (int)pFanout->Level > p->nLevDivMax )
                continue;
            // skip nodes whose fanins are not divisors
            Abc_ObjForEachFanin( pFanout, pFanin, m )
                if ( !Abc_NodeIsTravIdPrevious(pFanin) )
                    break;
            if ( m < Abc_ObjFaninNum(pFanout) )
                continue;
            // add the node to the divisors
            Vec_PtrPush( p->vDivs, pFanout );
            Vec_PtrPush( p->vNodes, pFanout );
            Abc_NodeSetTravIdPrevious( pFanout );
            p->nDivsPlus++;
        }
    }
/*
    printf( "Node level = %d.  ", Abc_ObjLevel(p->pNode) );
    Vec_PtrForEachEntryStart( Abc_Obj_t *, p->vDivs, pObj, k, Vec_PtrSize(p->vDivs)-p->nDivsPlus )
        printf( "%d ", Abc_ObjLevel(pObj) );
    printf( "\n" );
*/
//printf( "%d ", p->nDivsPlus );
}

/**Function*************************************************************

  Synopsis    [Marks the TFI cone of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_WinMarkTfi_rec( Res_Win_t * p, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin;
    int i;
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    assert( Abc_ObjIsNode(pObj) );
    // visit the fanins of the node
    Abc_ObjForEachFanin( pObj, pFanin, i )
        Res_WinMarkTfi_rec( p, pFanin );
}

/**Function*************************************************************

  Synopsis    [Marks the TFI cone of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_WinMarkTfi( Res_Win_t * p )
{
    Abc_Obj_t * pObj;
    int i;
    // mark the leaves
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pObj, i )
        Abc_NodeSetTravIdCurrent( pObj );
    // start from the node
    Res_WinMarkTfi_rec( p, p->pNode );
}

/**Function*************************************************************

  Synopsis    [Marks the TFO of the collected nodes up to the given level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_WinSweepLeafTfo_rec( Abc_Obj_t * pObj, int nLevelLimit )
{
    Abc_Obj_t * pFanout;
    int i;
    if ( Abc_ObjIsCo(pObj) || (int)pObj->Level > nLevelLimit )
        return;
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    Abc_ObjForEachFanout( pObj, pFanout, i )
        Res_WinSweepLeafTfo_rec( pFanout, nLevelLimit );
}

/**Function*************************************************************

  Synopsis    [Dereferences the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_NodeDeref_rec( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    int i, Counter = 1;
    if ( Abc_ObjIsCi(pNode) )
        return 0;
    Abc_NodeSetTravIdCurrent( pNode );
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        assert( pFanin->vFanouts.nSize > 0 );
        if ( --pFanin->vFanouts.nSize == 0 )
            Counter += Res_NodeDeref_rec( pFanin );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [References the node's MFFC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_NodeRef_rec( Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanin;
    int i, Counter = 1;
    if ( Abc_ObjIsCi(pNode) )
        return 0;
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        if ( pFanin->vFanouts.nSize++ == 0 )
            Counter += Res_NodeRef_rec( pFanin );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Labels MFFC of the node with the current trav ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_WinVisitMffc( Abc_Obj_t * pNode )
{
    int Count1, Count2;
    assert( Abc_ObjIsNode(pNode) );
    // dereference the node (mark with the current trav ID)
    Count1 = Res_NodeDeref_rec( pNode );
    // reference it back
    Count2 = Res_NodeRef_rec( pNode );
    assert( Count1 == Count2 );
    return Count1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

