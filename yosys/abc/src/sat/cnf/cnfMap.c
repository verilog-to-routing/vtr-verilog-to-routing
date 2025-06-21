/**CFile****************************************************************

  FileName    [cnfMap.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG-to-CNF conversion.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: cnfMap.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cnf.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes area flow of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_CutAssignAreaFlow( Cnf_Man_t * p, Dar_Cut_t * pCut, int * pAreaFlows )
{
    Aig_Obj_t * pLeaf;
    int i;
    pCut->Value = 0;
//    pCut->uSign = 100 * Cnf_CutSopCost( p, pCut );
    pCut->uSign = 10 * Cnf_CutSopCost( p, pCut );
    Dar_CutForEachLeaf( p->pManAig, pCut, pLeaf, i )
    {
        pCut->Value += pLeaf->nRefs;
        if ( !Aig_ObjIsNode(pLeaf) )
            continue;
        assert( pLeaf->nRefs > 0 );
        pCut->uSign += pAreaFlows[pLeaf->Id] / (pLeaf->nRefs? pLeaf->nRefs : 1);
    }
}

/**Function*************************************************************

  Synopsis    [Computes area flow of the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_CutSuperAreaFlow( Vec_Ptr_t * vSuper, int * pAreaFlows )
{
    Aig_Obj_t * pLeaf;
    int i, nAreaFlow;
    nAreaFlow = 100 * (Vec_PtrSize(vSuper) + 1);
    Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pLeaf, i )
    {
        pLeaf = Aig_Regular(pLeaf);
        if ( !Aig_ObjIsNode(pLeaf) )
            continue;
        assert( pLeaf->nRefs > 0 );
        nAreaFlow += pAreaFlows[pLeaf->Id] / (pLeaf->nRefs? pLeaf->nRefs : 1);
    }
    return nAreaFlow;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_DeriveMapping( Cnf_Man_t * p )
{
    Vec_Ptr_t * vSuper;
    Aig_Obj_t * pObj;
    Dar_Cut_t * pCut, * pCutBest;
    int i, k, AreaFlow, * pAreaFlows;
    // allocate area flows
    pAreaFlows = ABC_ALLOC( int, Aig_ManObjNumMax(p->pManAig) );
    memset( pAreaFlows, 0, sizeof(int) * Aig_ManObjNumMax(p->pManAig) );
    // visit the nodes in the topological order and update their best cuts
    vSuper = Vec_PtrAlloc( 100 );
    Aig_ManForEachNode( p->pManAig, pObj, i )
    {
        // go through the cuts
        pCutBest = NULL;
        Dar_ObjForEachCut( pObj, pCut, k )
        {
            pCut->fBest = 0;
            if ( k == 0 )
                continue;
            Cnf_CutAssignAreaFlow( p, pCut, pAreaFlows );
            if ( pCutBest == NULL || pCutBest->uSign > pCut->uSign || 
                (pCutBest->uSign == pCut->uSign && pCutBest->Value < pCut->Value) )
                 pCutBest = pCut;
        }
        // check the big cut
//        Aig_ObjCollectSuper( pObj, vSuper );
        // get the area flow of this cut
//        AreaFlow = Cnf_CutSuperAreaFlow( vSuper, pAreaFlows );
        AreaFlow = ABC_INFINITY;
        if ( AreaFlow >= (int)pCutBest->uSign )
        {
            pAreaFlows[pObj->Id] = pCutBest->uSign;
            pCutBest->fBest = 1;
        }
        else
        {
            pAreaFlows[pObj->Id] = AreaFlow;
            pObj->fMarkB = 1; // mark the special node
        }
    }
    Vec_PtrFree( vSuper );
    ABC_FREE( pAreaFlows );

/*
    // compute the area of mapping
    AreaFlow = 0;
    Aig_ManForEachCo( p->pManAig, pObj, i )
        AreaFlow += Dar_ObjBestCut(Aig_ObjFanin0(pObj))->uSign / 100 / Aig_ObjFanin0(pObj)->nRefs;
    printf( "Area of the network = %d.\n", AreaFlow );
*/
}



#if 0

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
void Aig_CutDeref( Aig_Man_t * p, Dar_Cut_t * pCut )
{ 
    Aig_Obj_t * pLeaf;
    int i;
    Dar_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        assert( pLeaf->nRefs > 0 );
        if ( --pLeaf->nRefs > 0 || !Aig_ObjIsAnd(pLeaf) )
            continue;
        Aig_CutDeref( p, Aig_ObjBestCut(pLeaf) );
    }
}

/**Function*************************************************************

  Synopsis    [Computes area of the first level.]

  Description [The cut need to be derefed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_CutRef( Aig_Man_t * p, Dar_Cut_t * pCut )
{
    Aig_Obj_t * pLeaf;
    int i, Area = pCut->Value;
    Dar_CutForEachLeaf( p, pCut, pLeaf, i )
    {
        assert( pLeaf->nRefs >= 0 );
        if ( pLeaf->nRefs++ > 0 || !Aig_ObjIsAnd(pLeaf) )
            continue;
        Area += Aig_CutRef( p, Aig_ObjBestCut(pLeaf) );
    }
    return Area;
}

/**Function*************************************************************

  Synopsis    [Computes exact area of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_CutArea( Aig_Man_t * p, Dar_Cut_t * pCut )
{
    int Area;
    Area = Aig_CutRef( p, pCut );
    Aig_CutDeref( p, pCut );
    return Area;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the second cut is better.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cnf_CutCompare( Dar_Cut_t * pC0, Dar_Cut_t * pC1 )
{
    if ( pC0->Area < pC1->Area - 0.0001 )
        return -1;
    if ( pC0->Area > pC1->Area + 0.0001 ) // smaller area flow is better
        return 1;
//    if ( pC0->NoRefs < pC1->NoRefs )
//        return -1;
//    if ( pC0->NoRefs > pC1->NoRefs ) // fewer non-referenced fanins is better
//        return 1;
//    if ( pC0->FanRefs / pC0->nLeaves > pC1->FanRefs / pC1->nLeaves )
//        return -1;
//    if ( pC0->FanRefs / pC0->nLeaves < pC1->FanRefs / pC1->nLeaves )
//        return 1; // larger average fanin ref-counter is better
//    return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns the cut with the smallest area flow.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dar_Cut_t * Cnf_ObjFindBestCut( Aig_Obj_t * pObj )
{
    Dar_Cut_t * pCut, * pCutBest;
    int i;
    pCutBest = NULL;
    Dar_ObjForEachCut( pObj, pCut, i )
        if ( pCutBest == NULL || Cnf_CutCompare(pCutBest, pCut) == 1 )
            pCutBest = pCut;
    return pCutBest;
}

/**Function*************************************************************

  Synopsis    [Computes area flow of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cnf_CutAssignArea( Cnf_Man_t * p, Dar_Cut_t * pCut )
{
    Aig_Obj_t * pLeaf;
    int i;
    pCut->Area    = (float)pCut->Cost;
    pCut->NoRefs  = 0;
    pCut->FanRefs = 0;
    Dar_CutForEachLeaf( p->pManAig, pCut, pLeaf, i )
    {
        if ( !Aig_ObjIsNode(pLeaf) )
            continue;
        if ( pLeaf->nRefs == 0 )
        {
            pCut->Area += Aig_ObjBestCut(pLeaf)->Cost;
            pCut->NoRefs++;
        }
        else
        {
            if ( pCut->FanRefs + pLeaf->nRefs > 15 )
                pCut->FanRefs = 15;
            else
                pCut->FanRefs += pLeaf->nRefs;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Performs one round of "area recovery" using exact local area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cnf_ManMapForCnf( Cnf_Man_t * p )
{
    Aig_Obj_t * pObj;
    Dar_Cut_t * pCut, * pCutBest;
    int i, k;
    // visit the nodes in the topological order and update their best cuts
    Aig_ManForEachNode( p->pManAig, pObj, i )
    {
        // find the old best cut
        pCutBest = Aig_ObjBestCut(pObj);
        Dar_ObjClearBestCut(pCutBest);
        // if the node is used, dereference its cut
        if ( pObj->nRefs )
            Aig_CutDeref( p->pManAig, pCutBest );

        // evaluate the cuts of this node
        Dar_ObjForEachCut( pObj, pCut, k )
//            Cnf_CutAssignAreaFlow( p, pCut );
            pCut->Area = (float)Cnf_CutArea( p->pManAig, pCut );

        // find the new best cut
        pCutBest = Cnf_ObjFindBestCut(pObj);
        Dar_ObjSetBestCut( pCutBest );
        // if the node is used, reference its cut
        if ( pObj->nRefs )
            Aig_CutRef( p->pManAig, pCutBest );
    }
    return 1;
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

