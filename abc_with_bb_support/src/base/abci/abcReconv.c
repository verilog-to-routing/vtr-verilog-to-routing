/**CFile****************************************************************

  FileName    [abcReconv.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Computation of reconvergence-driven cuts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcReconv.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Abc_ManCut_t_
{
    // user specified parameters
    int              nNodeSizeMax;  // the limit on the size of the supernode
    int              nConeSizeMax;  // the limit on the size of the containing cone
    int              nNodeFanStop;  // the limit on the size of the supernode
    int              nConeFanStop;  // the limit on the size of the containing cone
    // internal parameters
    Vec_Ptr_t *      vNodeLeaves;   // fanins of the collapsed node (the cut)
    Vec_Ptr_t *      vConeLeaves;   // fanins of the containing cone
    Vec_Ptr_t *      vVisited;      // the visited nodes
    Vec_Vec_t *      vLevels;       // the data structure to compute TFO nodes
    Vec_Ptr_t *      vNodesTfo;     // the nodes in the TFO of the cut
};

static int   Abc_NodeBuildCutLevelOne_int( Vec_Ptr_t * vVisited, Vec_Ptr_t * vLeaves, int nSizeLimit, int nFaninLimit );
static int   Abc_NodeBuildCutLevelTwo_int( Vec_Ptr_t * vVisited, Vec_Ptr_t * vLeaves, int nFaninLimit );
static void  Abc_NodeConeMarkCollect_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vVisited );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Unmarks the TFI cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_NodesMark( Vec_Ptr_t * vVisited )
{
    Abc_Obj_t * pNode;
    int i;
    Vec_PtrForEachEntry( Abc_Obj_t *, vVisited, pNode, i )
        pNode->fMarkA = 1;
}

/**Function*************************************************************

  Synopsis    [Unmarks the TFI cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_NodesUnmark( Vec_Ptr_t * vVisited )
{
    Abc_Obj_t * pNode;
    int i;
    Vec_PtrForEachEntry( Abc_Obj_t *, vVisited, pNode, i )
        pNode->fMarkA = 0;
}

/**Function*************************************************************

  Synopsis    [Unmarks the TFI cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_NodesUnmarkB( Vec_Ptr_t * vVisited )
{
    Abc_Obj_t * pNode;
    int i;
    Vec_PtrForEachEntry( Abc_Obj_t *, vVisited, pNode, i )
        pNode->fMarkB = 0;
}

/**Function*************************************************************

  Synopsis    [Evaluate the cost of removing the node from the set of leaves.]

  Description [Returns the number of new leaves that will be brought in.
  Returns large number if the node cannot be removed from the set of leaves.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_NodeGetLeafCostOne( Abc_Obj_t * pNode, int nFaninLimit )
{
    int Cost;
    // make sure the node is in the construction zone
    assert( pNode->fMarkB == 1 );  
    // cannot expand over the PI node
    if ( Abc_ObjIsCi(pNode) )
        return 999;
    // get the cost of the cone
    Cost = (!Abc_ObjFanin0(pNode)->fMarkB) + (!Abc_ObjFanin1(pNode)->fMarkB);
    // always accept if the number of leaves does not increase
    if ( Cost < 2 )
        return Cost;
    // skip nodes with many fanouts
    if ( Abc_ObjFanoutNum(pNode) > nFaninLimit )
        return 999;
    // return the number of nodes that will be on the leaves if this node is removed
    return Cost;
}

/**Function*************************************************************

  Synopsis    [Evaluate the cost of removing the node from the set of leaves.]

  Description [Returns the number of new leaves that will be brought in.
  Returns large number if the node cannot be removed from the set of leaves.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_NodeGetLeafCostTwo( Abc_Obj_t * pNode, int nFaninLimit, 
    Abc_Obj_t ** ppLeafToAdd, Abc_Obj_t ** pNodeToMark1, Abc_Obj_t ** pNodeToMark2 )
{
    Abc_Obj_t * pFanin0, * pFanin1, * pTemp;
    Abc_Obj_t * pGrand, * pGrandToAdd;
    // make sure the node is in the construction zone
    assert( pNode->fMarkB == 1 );  
    // cannot expand over the PI node
    if ( Abc_ObjIsCi(pNode) )
        return 999;
    // skip nodes with many fanouts
//    if ( Abc_ObjFanoutNum(pNode) > nFaninLimit )
//        return 999;
    // get the children
    pFanin0 = Abc_ObjFanin0(pNode);
    pFanin1 = Abc_ObjFanin1(pNode);
    assert( !pFanin0->fMarkB && !pFanin1->fMarkB );
    // count the number of unique grandchildren that will be included
    // return infinite cost if this number if more than 1
    if ( Abc_ObjIsCi(pFanin0) && Abc_ObjIsCi(pFanin1) )
        return 999;
    // consider the special case when a non-CI fanin can be dropped
    if ( !Abc_ObjIsCi(pFanin0) && Abc_ObjFanin0(pFanin0)->fMarkB && Abc_ObjFanin1(pFanin0)->fMarkB )
    {
        *ppLeafToAdd  = pFanin1;
        *pNodeToMark1 = pFanin0;
        *pNodeToMark2 = NULL;
        return 1;
    }
    if ( !Abc_ObjIsCi(pFanin1) && Abc_ObjFanin0(pFanin1)->fMarkB && Abc_ObjFanin1(pFanin1)->fMarkB )
    {
        *ppLeafToAdd  = pFanin0;
        *pNodeToMark1 = pFanin1;
        *pNodeToMark2 = NULL;
        return 1;
    }

    // make the first node CI if any
    if ( Abc_ObjIsCi(pFanin1) )
        pTemp = pFanin0, pFanin0 = pFanin1, pFanin1 = pTemp;
    // consider the first node
    pGrandToAdd = NULL;
    if ( Abc_ObjIsCi(pFanin0) )
    {
        *pNodeToMark1 = NULL;
        pGrandToAdd = pFanin0;
    }
    else
    {
        *pNodeToMark1 = pFanin0;
        pGrand = Abc_ObjFanin0(pFanin0);
        if ( !pGrand->fMarkB )
        {
            if ( pGrandToAdd && pGrandToAdd != pGrand )
                return 999;
            pGrandToAdd = pGrand;
        }
        pGrand = Abc_ObjFanin1(pFanin0);
        if ( !pGrand->fMarkB )
        {
            if ( pGrandToAdd && pGrandToAdd != pGrand )
                return 999;
            pGrandToAdd = pGrand;
        }
    }
    // consider the second node
    *pNodeToMark2 = pFanin1;
    pGrand = Abc_ObjFanin0(pFanin1);
    if ( !pGrand->fMarkB )
    {
        if ( pGrandToAdd && pGrandToAdd != pGrand )
            return 999;
        pGrandToAdd = pGrand;
    }
    pGrand = Abc_ObjFanin1(pFanin1);
    if ( !pGrand->fMarkB )
    {
        if ( pGrandToAdd && pGrandToAdd != pGrand )
            return 999;
        pGrandToAdd = pGrand;
    }
    assert( pGrandToAdd != NULL );
    *ppLeafToAdd = pGrandToAdd;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Finds a fanin-limited, reconvergence-driven cut for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NodeFindCut( Abc_ManCut_t * p, Abc_Obj_t * pRoot, int fContain )
{
    Abc_Obj_t * pNode;
    int i;

    assert( !Abc_ObjIsComplement(pRoot) );
    assert( Abc_ObjIsNode(pRoot) );

    // start the visited nodes and mark them
    Vec_PtrClear( p->vVisited );
    Vec_PtrPush( p->vVisited, pRoot );
    Vec_PtrPush( p->vVisited, Abc_ObjFanin0(pRoot) );
    Vec_PtrPush( p->vVisited, Abc_ObjFanin1(pRoot) );
    pRoot->fMarkB = 1;
    Abc_ObjFanin0(pRoot)->fMarkB = 1;
    Abc_ObjFanin1(pRoot)->fMarkB = 1;

    // start the cut 
    Vec_PtrClear( p->vNodeLeaves );
    Vec_PtrPush( p->vNodeLeaves, Abc_ObjFanin0(pRoot) );
    Vec_PtrPush( p->vNodeLeaves, Abc_ObjFanin1(pRoot) );

    // compute the cut
    while ( Abc_NodeBuildCutLevelOne_int( p->vVisited, p->vNodeLeaves, p->nNodeSizeMax, p->nNodeFanStop ) );
    assert( Vec_PtrSize(p->vNodeLeaves) <= p->nNodeSizeMax );

    // return if containing cut is not requested
    if ( !fContain )
    {
        // unmark both fMarkA and fMarkB in tbe TFI
        Abc_NodesUnmarkB( p->vVisited );
        return p->vNodeLeaves;
    }

//printf( "\n\n\n" );
    // compute the containing cut
    assert( p->nNodeSizeMax < p->nConeSizeMax );
    // copy the current boundary
    Vec_PtrClear( p->vConeLeaves );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vNodeLeaves, pNode, i )
        Vec_PtrPush( p->vConeLeaves, pNode );
    // compute the containing cut
    while ( Abc_NodeBuildCutLevelOne_int( p->vVisited, p->vConeLeaves, p->nConeSizeMax, p->nConeFanStop ) );
    assert( Vec_PtrSize(p->vConeLeaves) <= p->nConeSizeMax );
    // unmark TFI using fMarkA and fMarkB
    Abc_NodesUnmarkB( p->vVisited );
    return p->vNodeLeaves;
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
int Abc_NodeBuildCutLevelOne_int( Vec_Ptr_t * vVisited, Vec_Ptr_t * vLeaves, int nSizeLimit, int nFaninLimit )
{
    Abc_Obj_t * pNode, * pFaninBest, * pNext;
    int CostBest, CostCur, i;
    // find the best fanin
    CostBest   = 100;
    pFaninBest = NULL;
//printf( "Evaluating fanins of the cut:\n" );
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pNode, i )
    {
        CostCur = Abc_NodeGetLeafCostOne( pNode, nFaninLimit );
//printf( "    Fanin %s has cost %d.\n", Abc_ObjName(pNode), CostCur );
//        if ( CostBest > CostCur ) // performance improvement: expand the variable with the smallest level
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
//        return Abc_NodeBuildCutLevelTwo_int( vVisited, vLeaves, nFaninLimit );

    assert( CostBest < 3 );
    if ( vLeaves->nSize - 1 + CostBest > nSizeLimit )
        return 0;
//        return Abc_NodeBuildCutLevelTwo_int( vVisited, vLeaves, nFaninLimit );

    assert( Abc_ObjIsNode(pFaninBest) );
    // remove the node from the array
    Vec_PtrRemove( vLeaves, pFaninBest );
//printf( "Removing fanin %s.\n", Abc_ObjName(pFaninBest) );

    // add the left child to the fanins
    pNext = Abc_ObjFanin0(pFaninBest);
    if ( !pNext->fMarkB )
    {
//printf( "Adding fanin %s.\n", Abc_ObjName(pNext) );
        pNext->fMarkB = 1;
        Vec_PtrPush( vLeaves, pNext );
        Vec_PtrPush( vVisited, pNext );
    }
    // add the right child to the fanins
    pNext = Abc_ObjFanin1(pFaninBest);
    if ( !pNext->fMarkB )
    {
//printf( "Adding fanin %s.\n", Abc_ObjName(pNext) );
        pNext->fMarkB = 1;
        Vec_PtrPush( vLeaves, pNext );
        Vec_PtrPush( vVisited, pNext );
    }
    assert( vLeaves->nSize <= nSizeLimit );
    // keep doing this
    return 1;
}

/**Function*************************************************************

  Synopsis    [Builds reconvergence-driven cut by changing one leaf at a time.]

  Description [This procedure looks at the current leaves and tries to change 
  one leaf at a time in such a way that the cut grows as little as possible.
  In evaluating the fanins, this procedure looks across two levels of fanins
  (this is why it is called a two-level construction procedure).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeBuildCutLevelTwo_int( Vec_Ptr_t * vVisited, Vec_Ptr_t * vLeaves, int nFaninLimit )
{
    Abc_Obj_t * pNode = NULL, * pLeafToAdd, * pNodeToMark1, * pNodeToMark2;
    int CostCur = 0, i;
    // find the best fanin
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pNode, i )
    {
        CostCur = Abc_NodeGetLeafCostTwo( pNode, nFaninLimit, &pLeafToAdd, &pNodeToMark1, &pNodeToMark2 );
        if ( CostCur < 2 )
            break;
    }
    if ( CostCur > 2 )
        return 0;
    // remove the node from the array
    Vec_PtrRemove( vLeaves, pNode );
    // add the node to the leaves
    if ( pLeafToAdd )
    {
        assert( !pLeafToAdd->fMarkB );
        pLeafToAdd->fMarkB = 1;
        Vec_PtrPush( vLeaves, pLeafToAdd );
        Vec_PtrPush( vVisited, pLeafToAdd );
    }
    // mark the other nodes
    if ( pNodeToMark1 )
    {
        assert( !pNodeToMark1->fMarkB );
        pNodeToMark1->fMarkB = 1;
        Vec_PtrPush( vVisited, pNodeToMark1 );
    }
    if ( pNodeToMark2 )
    {
        assert( !pNodeToMark2->fMarkB );
        pNodeToMark2->fMarkB = 1;
        Vec_PtrPush( vVisited, pNodeToMark2 );
    }
    // keep doing this
    return 1;
}


/**Function*************************************************************

  Synopsis    [Get the nodes contained in the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeConeCollect( Abc_Obj_t ** ppRoots, int nRoots, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vVisited, int fIncludeFanins )
{
    Abc_Obj_t * pTemp;
    int i;
    // mark the fanins of the cone
    Abc_NodesMark( vLeaves );
    // collect the nodes in the DFS order
    Vec_PtrClear( vVisited );
    // add the fanins
    if ( fIncludeFanins )
        Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pTemp, i )
            Vec_PtrPush( vVisited, pTemp );
    // add other nodes
    for ( i = 0; i < nRoots; i++ )
        Abc_NodeConeMarkCollect_rec( ppRoots[i], vVisited );
    // unmark both sets
    Abc_NodesUnmark( vLeaves );
    Abc_NodesUnmark( vVisited );
}

/**Function*************************************************************

  Synopsis    [Marks the TFI cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeConeMarkCollect_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vVisited )
{
    if ( pNode->fMarkA == 1 )
        return;
    // visit transitive fanin 
    if ( Abc_ObjIsNode(pNode) )
    {
        Abc_NodeConeMarkCollect_rec( Abc_ObjFanin0(pNode), vVisited );
        Abc_NodeConeMarkCollect_rec( Abc_ObjFanin1(pNode), vVisited );
    }
    assert( pNode->fMarkA == 0 );
    pNode->fMarkA = 1;
    Vec_PtrPush( vVisited, pNode );
}

#ifdef ABC_USE_CUDD

/**Function*************************************************************

  Synopsis    [Returns BDD representing the logic function of the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NodeConeBdd( DdManager * dd, DdNode ** pbVars, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vVisited )
{
    Abc_Obj_t * pNode;
    DdNode * bFunc0, * bFunc1, * bFunc = NULL;
    int i;
    // get the nodes in the cut without fanins in the DFS order
    Abc_NodeConeCollect( &pRoot, 1, vLeaves, vVisited, 0 );
    // set the elementary BDDs
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pNode, i )
        pNode->pCopy = (Abc_Obj_t *)pbVars[i];
    // compute the BDDs for the collected nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vVisited, pNode, i )
    {
        assert( !Abc_ObjIsPi(pNode) );
        bFunc0 = Cudd_NotCond( Abc_ObjFanin0(pNode)->pCopy, (int)Abc_ObjFaninC0(pNode) );
        bFunc1 = Cudd_NotCond( Abc_ObjFanin1(pNode)->pCopy, (int)Abc_ObjFaninC1(pNode) );
        bFunc  = Cudd_bddAnd( dd, bFunc0, bFunc1 );    Cudd_Ref( bFunc );
        pNode->pCopy = (Abc_Obj_t *)bFunc;
    }
    assert(bFunc);
    Cudd_Ref( bFunc );
    // dereference the intermediate ones
    Vec_PtrForEachEntry( Abc_Obj_t *, vVisited, pNode, i )
        Cudd_RecursiveDeref( dd, (DdNode *)pNode->pCopy );
    Cudd_Deref( bFunc );
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Returns BDD representing the transition relation of the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NodeConeDcs( DdManager * dd, DdNode ** pbVarsX, DdNode ** pbVarsY, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vRoots, Vec_Ptr_t * vVisited )
{
    DdNode * bFunc0, * bFunc1, * bFunc, * bTrans, * bTemp, * bCube, * bResult;
    Abc_Obj_t * pNode;
    int i;
    // get the nodes in the cut without fanins in the DFS order
    Abc_NodeConeCollect( (Abc_Obj_t **)vRoots->pArray, vRoots->nSize, vLeaves, vVisited, 0 );
    // set the elementary BDDs
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pNode, i )
        pNode->pCopy = (Abc_Obj_t *)pbVarsX[i];
    // compute the BDDs for the collected nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vVisited, pNode, i )
    {
        bFunc0 = Cudd_NotCond( Abc_ObjFanin0(pNode)->pCopy, (int)Abc_ObjFaninC0(pNode) );
        bFunc1 = Cudd_NotCond( Abc_ObjFanin1(pNode)->pCopy, (int)Abc_ObjFaninC1(pNode) );
        bFunc  = Cudd_bddAnd( dd, bFunc0, bFunc1 );    Cudd_Ref( bFunc );
        pNode->pCopy = (Abc_Obj_t *)bFunc;
    }
    // compute the transition relation of the cone
    bTrans = b1;    Cudd_Ref( bTrans );
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pNode, i )
    {
        bFunc = Cudd_bddXnor( dd, (DdNode *)pNode->pCopy, pbVarsY[i] );  Cudd_Ref( bFunc );
		bTrans = Cudd_bddAnd( dd, bTemp = bTrans, bFunc );               Cudd_Ref( bTrans );
		Cudd_RecursiveDeref( dd, bTemp );
		Cudd_RecursiveDeref( dd, bFunc );
    }
    // dereference the intermediate ones
    Vec_PtrForEachEntry( Abc_Obj_t *, vVisited, pNode, i )
        Cudd_RecursiveDeref( dd, (DdNode *)pNode->pCopy );
    // compute don't-cares
    bCube = Extra_bddComputeRangeCube( dd, vRoots->nSize, vRoots->nSize + vLeaves->nSize );  Cudd_Ref( bCube );
    bResult = Cudd_bddExistAbstract( dd, bTrans, bCube );                Cudd_Ref( bResult );
    bResult = Cudd_Not( bResult );
	Cudd_RecursiveDeref( dd, bCube );
	Cudd_RecursiveDeref( dd, bTrans );
    Cudd_Deref( bResult );
    return bResult;
}
 
#endif

/**Function*************************************************************

  Synopsis    [Starts the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_ManCut_t * Abc_NtkManCutStart( int nNodeSizeMax, int nConeSizeMax, int nNodeFanStop, int nConeFanStop )
{
    Abc_ManCut_t * p;
    p = ABC_ALLOC( Abc_ManCut_t, 1 );
    memset( p, 0, sizeof(Abc_ManCut_t) );
    p->vNodeLeaves  = Vec_PtrAlloc( 100 );
    p->vConeLeaves  = Vec_PtrAlloc( 100 );
    p->vVisited     = Vec_PtrAlloc( 100 );
    p->vLevels      = Vec_VecAlloc( 100 );
    p->vNodesTfo    = Vec_PtrAlloc( 100 );
    p->nNodeSizeMax = nNodeSizeMax;
    p->nConeSizeMax = nConeSizeMax;
    p->nNodeFanStop = nNodeFanStop;
    p->nConeFanStop = nConeFanStop;
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkManCutStop( Abc_ManCut_t * p )
{
    Vec_PtrFree( p->vNodeLeaves );
    Vec_PtrFree( p->vConeLeaves );
    Vec_PtrFree( p->vVisited    );
    Vec_VecFree( p->vLevels );
    Vec_PtrFree( p->vNodesTfo );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns the leaves of the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkManCutReadCutLarge( Abc_ManCut_t * p )
{
    return p->vConeLeaves;
}

/**Function*************************************************************

  Synopsis    [Returns the leaves of the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkManCutReadCutSmall( Abc_ManCut_t * p )
{
    return p->vNodeLeaves;
}

/**Function*************************************************************

  Synopsis    [Returns the leaves of the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkManCutReadVisited( Abc_ManCut_t * p )
{
    return p->vVisited;
}



/**Function*************************************************************

  Synopsis    [Collects the TFO of the cut in the topological order.]

  Description [TFO of the cut is defined as a set of nodes, for which the cut
  is a cut, that is, every path from the collected nodes to the CIs goes through 
  a node in the cut. The nodes are collected if their level does not exceed
  the given number (LevelMax). The nodes are returned in the topological order.
  If the root node is given, its MFFC is marked, so that the collected nodes
  do not contain any nodes in the MFFC.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NodeCollectTfoCands( Abc_ManCut_t * p, Abc_Obj_t * pRoot, Vec_Ptr_t * vLeaves, int LevelMax )
{
    Abc_Ntk_t * pNtk = pRoot->pNtk;
    Vec_Ptr_t * vVec;
    Abc_Obj_t * pNode, * pFanout;
    int i, k, v, LevelMin;
    assert( Abc_NtkIsStrash(pNtk) );

    // assuming that the structure is clean
    Vec_VecForEachLevel( p->vLevels, vVec, i )
        assert( vVec->nSize == 0 );

    // put fanins into the structure while labeling them
    Abc_NtkIncrementTravId( pNtk );
    LevelMin = -1;
    Vec_PtrForEachEntry( Abc_Obj_t *, vLeaves, pNode, i )
    {
        if ( pNode->Level > (unsigned)LevelMax )
            continue;
        Abc_NodeSetTravIdCurrent( pNode );
        Vec_VecPush( p->vLevels, pNode->Level, pNode );
        if ( LevelMin < (int)pNode->Level )
            LevelMin = pNode->Level;
    }
    assert( LevelMin >= 0 );

    // mark MFFC 
    if ( pRoot )
        Abc_NodeMffcLabelAig( pRoot );

    // go through the levels up
    Vec_PtrClear( p->vNodesTfo );
    Vec_VecForEachEntryStart( Abc_Obj_t *, p->vLevels, pNode, i, k, LevelMin )
    {
        if ( i > LevelMax )
            break;
        // if the node is not marked, it is not a fanin
        if ( !Abc_NodeIsTravIdCurrent(pNode) )
        {
            // check if it belongs to the TFO
            if ( !Abc_NodeIsTravIdCurrent(Abc_ObjFanin0(pNode)) || 
                 !Abc_NodeIsTravIdCurrent(Abc_ObjFanin1(pNode)) )
                 continue;
            // save the node in the TFO and label it
            Vec_PtrPush( p->vNodesTfo, pNode );
            Abc_NodeSetTravIdCurrent( pNode );
        }
        // go through the fanouts and add them to the structure if they meet the conditions
        Abc_ObjForEachFanout( pNode, pFanout, v )
        {
            // skip if fanout is a CO or its level exceeds
            if ( Abc_ObjIsCo(pFanout) || pFanout->Level > (unsigned)LevelMax )
                continue;
            // skip if it is already added or if it is in MFFC
            if ( Abc_NodeIsTravIdCurrent(pFanout) )
                continue;
            // add it to the structure but do not mark it (until tested later)
            Vec_VecPushUnique( p->vLevels, pFanout->Level, pFanout );
        }
    }

    // clear the levelized structure
    Vec_VecForEachLevelStart( p->vLevels, vVec, i, LevelMin )
    {
        if ( i > LevelMax )
            break;
        Vec_PtrClear( vVec );
    }
    return p->vNodesTfo;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

