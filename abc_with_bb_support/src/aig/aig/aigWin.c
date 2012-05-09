/**CFile****************************************************************

  FileName    [aigWin.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Window computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigWin.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Evaluate the cost of removing the node from the set of leaves.]

  Description [Returns the number of new leaves that will be brought in.
  Returns large number if the node cannot be removed from the set of leaves.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Aig_NodeGetLeafCostOne( Aig_Obj_t * pNode, int nFanoutLimit )
{
    int Cost;
    // make sure the node is in the construction zone
    assert( pNode->fMarkA );  
    // cannot expand over the PI node
    if ( Aig_ObjIsPi(pNode) )
        return 999;
    // get the cost of the cone
    Cost = (!Aig_ObjFanin0(pNode)->fMarkA) + (!Aig_ObjFanin1(pNode)->fMarkA);
    // always accept if the number of leaves does not increase
    if ( Cost < 2 )
        return Cost;
    // skip nodes with many fanouts
    if ( (int)pNode->nRefs > nFanoutLimit )
        return 999;
    // return the number of nodes that will be on the leaves if this node is removed
    return Cost;
}

/**Function*************************************************************

  Synopsis    [Builds reconvergence-driven cut by changing one leaf at a time.]

  Description [This procedure looks at the current leaves and tries to change 
  one leaf at a time in such a way that the cut grows as little as possible.
  In evaluating the fanins, this procedure looks only at their immediate 
  predecessors (this is why it is called a one-level construction procedure).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManFindCut_int( Vec_Ptr_t * vFront, Vec_Ptr_t * vVisited, int nSizeLimit, int nFanoutLimit )
{
    Aig_Obj_t * pNode, * pFaninBest, * pNext;
    int CostBest, CostCur, i;
    // find the best fanin
    CostBest   = 100;
    pFaninBest = NULL;
//printf( "Evaluating fanins of the cut:\n" );
    Vec_PtrForEachEntry( vFront, pNode, i )
    {
        CostCur = Aig_NodeGetLeafCostOne( pNode, nFanoutLimit );
//printf( "    Fanin %s has cost %d.\n", Aig_ObjName(pNode), CostCur );
        if ( CostBest > CostCur ||
            (CostBest == CostCur && pNode->Level > pFaninBest->Level) )
        {
            CostBest   = CostCur;
            pFaninBest = pNode;
        }
        if ( CostBest == 0 )
            break;
    }
    if ( pFaninBest == NULL )
        return 0;
    assert( CostBest < 3 );
    if ( Vec_PtrSize(vFront) - 1 + CostBest > nSizeLimit )
        return 0;
    assert( Aig_ObjIsNode(pFaninBest) );
    // remove the node from the array
    Vec_PtrRemove( vFront, pFaninBest );
//printf( "Removing fanin %s.\n", Aig_ObjName(pFaninBest) );

    // add the left child to the fanins
    pNext = Aig_ObjFanin0(pFaninBest);
    if ( !pNext->fMarkA )
    {
//printf( "Adding fanin %s.\n", Aig_ObjName(pNext) );
        pNext->fMarkA = 1;
        Vec_PtrPush( vFront, pNext );
        Vec_PtrPush( vVisited, pNext );
    }
    // add the right child to the fanins
    pNext = Aig_ObjFanin1(pFaninBest);
    if ( !pNext->fMarkA )
    {
//printf( "Adding fanin %s.\n", Aig_ObjName(pNext) );
        pNext->fMarkA = 1;
        Vec_PtrPush( vFront, pNext );
        Vec_PtrPush( vVisited, pNext );
    }
    assert( Vec_PtrSize(vFront) <= nSizeLimit );
    // keep doing this
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes one sequential cut of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManFindCut( Aig_Obj_t * pRoot, Vec_Ptr_t * vFront, Vec_Ptr_t * vVisited, int nSizeLimit, int nFanoutLimit )
{
    Aig_Obj_t * pNode;
    int i;

    assert( !Aig_IsComplement(pRoot) );
    assert( Aig_ObjIsNode(pRoot) );
    assert( Aig_ObjChild0(pRoot) );
    assert( Aig_ObjChild1(pRoot) );

    // start the cut 
    Vec_PtrClear( vFront );
    Vec_PtrPush( vFront, Aig_ObjFanin0(pRoot) );
    Vec_PtrPush( vFront, Aig_ObjFanin1(pRoot) );

    // start the visited nodes
    Vec_PtrClear( vVisited );
    Vec_PtrPush( vVisited, pRoot );
    Vec_PtrPush( vVisited, Aig_ObjFanin0(pRoot) );
    Vec_PtrPush( vVisited, Aig_ObjFanin1(pRoot) );

    // mark these nodes
    assert( !pRoot->fMarkA );
    assert( !Aig_ObjFanin0(pRoot)->fMarkA );
    assert( !Aig_ObjFanin1(pRoot)->fMarkA );
    pRoot->fMarkA = 1;
    Aig_ObjFanin0(pRoot)->fMarkA = 1;
    Aig_ObjFanin1(pRoot)->fMarkA = 1;

    // compute the cut
    while ( Aig_ManFindCut_int( vFront, vVisited, nSizeLimit, nFanoutLimit ) );
    assert( Vec_PtrSize(vFront) <= nSizeLimit );

    // clean the visit markings
    Vec_PtrForEachEntry( vVisited, pNode, i )
        pNode->fMarkA = 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


