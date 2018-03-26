/**CFile****************************************************************

  FileName    [abcMulti.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Procedures which transform an AIG into multi-input AND-graph.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcMulti.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_CUDD

static void        Abc_NtkMultiInt( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew );
static Abc_Obj_t * Abc_NtkMulti_rec( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNodeOld );

static DdNode *    Abc_NtkMultiDeriveBdd_rec( DdManager * dd, Abc_Obj_t * pNodeOld, Vec_Ptr_t * vFanins );
static DdNode *    Abc_NtkMultiDeriveBdd( DdManager * dd, Abc_Obj_t * pNodeOld, Vec_Ptr_t * vFaninsOld );

static void        Abc_NtkMultiSetBounds( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax );
static void        Abc_NtkMultiSetBoundsCnf( Abc_Ntk_t * pNtk );
static void        Abc_NtkMultiSetBoundsMulti( Abc_Ntk_t * pNtk, int nThresh );
static void        Abc_NtkMultiSetBoundsSimple( Abc_Ntk_t * pNtk );
static void        Abc_NtkMultiSetBoundsFactor( Abc_Ntk_t * pNtk );
static void        Abc_NtkMultiCone( Abc_Obj_t * pNode, Vec_Ptr_t * vCone );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Transforms the AIG into nodes.]

  Description [Threhold is the max number of nodes duplicated at a node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkMulti( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax, int fCnf, int fMulti, int fSimple, int fFactor )
{
    Abc_Ntk_t * pNtkNew;

    assert( Abc_NtkIsStrash(pNtk) );
    assert( nThresh >= 0 );
    assert( nFaninMax > 1 );

    // print a warning about choice nodes
    if ( Abc_NtkGetChoiceNum( pNtk ) )
        printf( "Warning: The choice nodes in the AIG are removed by renoding.\n" );

    // define the boundary
    if ( fCnf )
        Abc_NtkMultiSetBoundsCnf( pNtk );
    else if ( fMulti )
        Abc_NtkMultiSetBoundsMulti( pNtk, nThresh );
    else if ( fSimple )
        Abc_NtkMultiSetBoundsSimple( pNtk );
    else if ( fFactor )
        Abc_NtkMultiSetBoundsFactor( pNtk );
    else
        Abc_NtkMultiSetBounds( pNtk, nThresh, nFaninMax );

    // perform renoding for this boundary
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_BDD );
    Abc_NtkMultiInt( pNtk, pNtkNew );
    Abc_NtkFinalize( pNtk, pNtkNew );

    // make the network minimum base
    Abc_NtkMinimumBase( pNtkNew );

    // fix the problem with complemented and duplicated CO edges
    Abc_NtkLogicMakeSimpleCos( pNtkNew, 0 );

    // report the number of CNF objects
    if ( fCnf )
    {
//        int nClauses = Abc_NtkGetClauseNum(pNtkNew) + 2*Abc_NtkPoNum(pNtkNew) + 2*Abc_NtkLatchNum(pNtkNew);
//        printf( "CNF variables = %d. CNF clauses = %d.\n",  Abc_NtkNodeNum(pNtkNew), nClauses );
    }
//printf( "Maximum fanin = %d.\n", Abc_NtkGetFaninMax(pNtkNew) );

    if ( pNtk->pExdc )
        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkMulti: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Transforms the AIG into nodes.]

  Description [Threhold is the max number of nodes duplicated at a node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMultiInt( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pNode, * pConst1, * pNodeNew;
    int i;

    // set the constant node
    pConst1 = Abc_AigConst1(pNtk);
    if ( Abc_ObjFanoutNum(pConst1) > 0 )
    {
        pNodeNew = Abc_NtkCreateNode( pNtkNew );  
        pNodeNew->pData = Cudd_ReadOne( (DdManager *)pNtkNew->pManFunc );   Cudd_Ref( (DdNode *)pNodeNew->pData );
        pConst1->pCopy = pNodeNew;
    }

    // perform renoding for POs
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        if ( Abc_ObjIsCi(Abc_ObjFanin0(pNode)) )
            continue;
        Abc_NtkMulti_rec( pNtkNew, Abc_ObjFanin0(pNode) );
    }
    Extra_ProgressBarStop( pProgress );

    // clean the boundaries and data field in the old network
    Abc_NtkForEachObj( pNtk, pNode, i )
    {
        pNode->fMarkA = 0;
        pNode->pData = NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Find the best multi-input node rooted at the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkMulti_rec( Abc_Ntk_t * pNtkNew, Abc_Obj_t * pNodeOld )
{
    Vec_Ptr_t * vCone;
    Abc_Obj_t * pNodeNew;
    int i;

    assert( !Abc_ObjIsComplement(pNodeOld) );
    // return if the result if known
    if ( pNodeOld->pCopy )
        return pNodeOld->pCopy;
    assert( Abc_ObjIsNode(pNodeOld) );
    assert( !Abc_AigNodeIsConst(pNodeOld) );
    assert( pNodeOld->fMarkA );

//printf( "%d ", Abc_NodeMffcSizeSupp(pNodeOld) );

    // collect the renoding cone
    vCone = Vec_PtrAlloc( 10 );
    Abc_NtkMultiCone( pNodeOld, vCone );

    // create a new node 
    pNodeNew = Abc_NtkCreateNode( pNtkNew ); 
    for ( i = 0; i < vCone->nSize; i++ )
        Abc_ObjAddFanin( pNodeNew, Abc_NtkMulti_rec(pNtkNew, (Abc_Obj_t *)vCone->pArray[i]) );

    // derive the function of this node
    pNodeNew->pData = Abc_NtkMultiDeriveBdd( (DdManager *)pNtkNew->pManFunc, pNodeOld, vCone );    
    Cudd_Ref( (DdNode *)pNodeNew->pData );
    Vec_PtrFree( vCone );

    // remember the node
    pNodeOld->pCopy = pNodeNew;
    return pNodeOld->pCopy;
}


/**Function*************************************************************

  Synopsis    [Derives the local BDD of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkMultiDeriveBdd( DdManager * dd, Abc_Obj_t * pNodeOld, Vec_Ptr_t * vFaninsOld )
{
    Abc_Obj_t * pFaninOld;
    DdNode * bFunc;
    int i;
    assert( !Abc_AigNodeIsConst(pNodeOld) );
    assert( Abc_ObjIsNode(pNodeOld) );
    // set the elementary BDD variables for the input nodes
    for ( i = 0; i < vFaninsOld->nSize; i++ )
    {
        pFaninOld = (Abc_Obj_t *)vFaninsOld->pArray[i];
        pFaninOld->pData = Cudd_bddIthVar( dd, i );    Cudd_Ref( (DdNode *)pFaninOld->pData );
        pFaninOld->fMarkC = 1;
    }
    // call the recursive BDD computation
    bFunc = Abc_NtkMultiDeriveBdd_rec( dd, pNodeOld, vFaninsOld );  Cudd_Ref( bFunc );
    // dereference the intermediate nodes
    for ( i = 0; i < vFaninsOld->nSize; i++ )
    {
        pFaninOld = (Abc_Obj_t *)vFaninsOld->pArray[i];
        Cudd_RecursiveDeref( dd, (DdNode *)pFaninOld->pData );
        pFaninOld->fMarkC = 0;
    }
    Cudd_Deref( bFunc );
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Derives the local BDD of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkMultiDeriveBdd_rec( DdManager * dd, Abc_Obj_t * pNode, Vec_Ptr_t * vFanins )
{
    DdNode * bFunc, * bFunc0, * bFunc1;
    assert( !Abc_ObjIsComplement(pNode) );
    // if the result is available return
    if ( pNode->fMarkC )
    {
        assert( pNode->pData ); // network has a cycle
        return (DdNode *)pNode->pData;
    }
    // mark the node as visited
    pNode->fMarkC = 1;
    Vec_PtrPush( vFanins, pNode );
    // compute the result for both branches
    bFunc0 = Abc_NtkMultiDeriveBdd_rec( dd, Abc_ObjFanin(pNode,0), vFanins ); Cudd_Ref( bFunc0 );
    bFunc1 = Abc_NtkMultiDeriveBdd_rec( dd, Abc_ObjFanin(pNode,1), vFanins ); Cudd_Ref( bFunc1 );
    bFunc0 = Cudd_NotCond( bFunc0, (long)Abc_ObjFaninC0(pNode) );
    bFunc1 = Cudd_NotCond( bFunc1, (long)Abc_ObjFaninC1(pNode) );
    // get the final result
    bFunc = Cudd_bddAnd( dd, bFunc0, bFunc1 );   Cudd_Ref( bFunc );
    Cudd_RecursiveDeref( dd, bFunc0 );
    Cudd_RecursiveDeref( dd, bFunc1 );
    // set the result
    pNode->pData = bFunc;
    assert( pNode->pData );
    return bFunc;
}



/**Function*************************************************************

  Synopsis    [Limits the cones to be no more than the given size.]

  Description [Returns 1 if the last cone was limited. Returns 0 if no changes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMultiLimit_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vCone, int nFaninMax, int fCanStop, int fFirst )
{
    int nNodes0, nNodes1;
    assert( !Abc_ObjIsComplement(pNode) );
    // check if the node should be added to the fanins
    if ( !fFirst && (pNode->fMarkA || !Abc_ObjIsNode(pNode)) )
    {
        Vec_PtrPushUnique( vCone, pNode );
        return 0;
    }
    // if we cannot stop in this branch, collect all nodes
    if ( !fCanStop )
    {
        Abc_NtkMultiLimit_rec( Abc_ObjFanin(pNode,0), vCone, nFaninMax, 0, 0 );
        Abc_NtkMultiLimit_rec( Abc_ObjFanin(pNode,1), vCone, nFaninMax, 0, 0 );
        return 0;
    }
    // if we can stop, try the left branch first, and return if we stopped
    assert( vCone->nSize == 0 );
    if ( Abc_NtkMultiLimit_rec( Abc_ObjFanin(pNode,0), vCone, nFaninMax, 1, 0 ) )
        return 1;
    // save the number of nodes in the left branch and call for the right branch
    nNodes0 = vCone->nSize;
    assert( nNodes0 <= nFaninMax );
    Abc_NtkMultiLimit_rec( Abc_ObjFanin(pNode,1), vCone, nFaninMax, 0, 0 );
    // check the number of nodes
    if ( vCone->nSize <= nFaninMax )
        return 0;
    // the number of nodes exceeds the limit

    // get the number of nodes in the right branch
    vCone->nSize = 0;
    Abc_NtkMultiLimit_rec( Abc_ObjFanin(pNode,1), vCone, nFaninMax, 0, 0 );
    // if this number exceeds the limit, solve the problem for this branch
    if ( vCone->nSize > nFaninMax )
    {
        int RetValue;
        vCone->nSize = 0;
        RetValue = Abc_NtkMultiLimit_rec( Abc_ObjFanin(pNode,1), vCone, nFaninMax, 1, 0 );
        assert( RetValue == 1 );
        return 1;
    }

    nNodes1 = vCone->nSize; 
    assert( nNodes1 <= nFaninMax );
    if ( nNodes0 >= nNodes1 )
    { // the left branch is larger - cut it
        assert( Abc_ObjFanin(pNode,0)->fMarkA == 0 );
        Abc_ObjFanin(pNode,0)->fMarkA = 1;
    }
    else
    { // the right branch is larger - cut it
        assert( Abc_ObjFanin(pNode,1)->fMarkA == 0 );
        Abc_ObjFanin(pNode,1)->fMarkA = 1;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Limits the cones to be no more than the given size.]

  Description [Returns 1 if the last cone was limited. Returns 0 if no changes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMultiLimit( Abc_Obj_t * pNode, Vec_Ptr_t * vCone, int nFaninMax )
{
    vCone->nSize = 0;
    return Abc_NtkMultiLimit_rec( pNode, vCone, nFaninMax, 1, 1 );
}

/**Function*************************************************************

  Synopsis    [Sets the expansion boundary for multi-input nodes.]

  Description [The boundary includes the set of PIs and all nodes such that 
  when expanding over the node we duplicate no more than nThresh nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMultiSetBounds( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax )
{
    Vec_Ptr_t * vCone = Vec_PtrAlloc(10);
    Abc_Obj_t * pNode;
    int i, nFanouts, nConeSize;

    // make sure the mark is not set
    Abc_NtkForEachObj( pNtk, pNode, i )
        assert( pNode->fMarkA == 0 );

    // mark the nodes where expansion stops using pNode->fMarkA
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        // skip PI/PO nodes
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // mark the nodes with multiple fanouts
        nFanouts = Abc_ObjFanoutNum(pNode);
        nConeSize = Abc_NodeMffcSize(pNode);
        if ( (nFanouts - 1) * nConeSize > nThresh )
            pNode->fMarkA = 1;
    }

    // mark the PO drivers
    Abc_NtkForEachCo( pNtk, pNode, i )
        Abc_ObjFanin0(pNode)->fMarkA = 1;

    // make sure the fanin limit is met
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        // skip PI/PO nodes
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        if ( pNode->fMarkA == 0 )
            continue;
        // continue cutting branches until it meets the fanin limit
        while ( Abc_NtkMultiLimit(pNode, vCone, nFaninMax) );
        assert( vCone->nSize <= nFaninMax );  
    }
    Vec_PtrFree(vCone);
/*
    // make sure the fanin limit is met
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        // skip PI/PO nodes
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        if ( pNode->fMarkA == 0 )
            continue;
        Abc_NtkMultiCone( pNode, vCone );
        assert( vCone->nSize <= nFaninMax );    
    }
*/
}

/**Function*************************************************************

  Synopsis    [Sets the expansion boundary for conversion into CNF.]

  Description [The boundary includes the set of PIs, the roots of MUXes,
  the nodes with multiple fanouts and the nodes with complemented outputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMultiSetBoundsCnf( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i, nMuxes;

    // make sure the mark is not set
    Abc_NtkForEachObj( pNtk, pNode, i )
        assert( pNode->fMarkA == 0 );

    // mark the nodes where expansion stops using pNode->fMarkA
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        // skip PI/PO nodes
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // mark the nodes with multiple fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1 )
            pNode->fMarkA = 1;
        // mark the nodes that are roots of MUXes
        if ( Abc_NodeIsMuxType( pNode ) )
        {
            pNode->fMarkA = 1;
            Abc_ObjFanin0( Abc_ObjFanin0(pNode) )->fMarkA = 1;
            Abc_ObjFanin0( Abc_ObjFanin1(pNode) )->fMarkA = 1;
            Abc_ObjFanin1( Abc_ObjFanin0(pNode) )->fMarkA = 1;
            Abc_ObjFanin1( Abc_ObjFanin1(pNode) )->fMarkA = 1;
        }
        else  // mark the complemented edges
        {
            if ( Abc_ObjFaninC0(pNode) )
                Abc_ObjFanin0(pNode)->fMarkA = 1;
            if ( Abc_ObjFaninC1(pNode) )
                Abc_ObjFanin1(pNode)->fMarkA = 1;
        }
    }

    // mark the PO drivers
    Abc_NtkForEachCo( pNtk, pNode, i )
        Abc_ObjFanin0(pNode)->fMarkA = 1;

    // count the number of MUXes
    nMuxes = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        // skip PI/PO nodes
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        if ( Abc_NodeIsMuxType(pNode) && 
            Abc_ObjFanin0(pNode)->fMarkA == 0 &&
            Abc_ObjFanin1(pNode)->fMarkA == 0 )
            nMuxes++;
    }
//    printf( "The number of MUXes detected = %d (%5.2f %% of logic).\n", nMuxes, 300.0*nMuxes/Abc_NtkNodeNum(pNtk) );
} 
 
/**Function*************************************************************

  Synopsis    [Sets the expansion boundary for conversion into multi-input AND graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMultiSetBoundsMulti( Abc_Ntk_t * pNtk, int nThresh )
{
    Abc_Obj_t * pNode;
    int i, nFanouts, nConeSize;

    // make sure the mark is not set
    Abc_NtkForEachObj( pNtk, pNode, i )
        assert( pNode->fMarkA == 0 );

    // mark the nodes where expansion stops using pNode->fMarkA
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        // skip PI/PO nodes
//        if ( Abc_NodeIsConst(pNode) )
//            continue;
        // mark the nodes with multiple fanouts
//        if ( Abc_ObjFanoutNum(pNode) > 1 )
//            pNode->fMarkA = 1;
        // mark the nodes with multiple fanouts
        nFanouts = Abc_ObjFanoutNum(pNode);
        nConeSize = Abc_NodeMffcSizeStop(pNode);
        if ( (nFanouts - 1) * nConeSize > nThresh )
            pNode->fMarkA = 1;
        // mark the children if they are pointed by the complemented edges
        if ( Abc_ObjFaninC0(pNode) )
            Abc_ObjFanin0(pNode)->fMarkA = 1;
        if ( Abc_ObjFaninC1(pNode) )
            Abc_ObjFanin1(pNode)->fMarkA = 1;
    }

    // mark the PO drivers
    Abc_NtkForEachCo( pNtk, pNode, i )
        Abc_ObjFanin0(pNode)->fMarkA = 1;
}

/**Function*************************************************************

  Synopsis    [Sets a simple boundary.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMultiSetBoundsSimple( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i;
    // make sure the mark is not set
    Abc_NtkForEachObj( pNtk, pNode, i )
        assert( pNode->fMarkA == 0 );
    // mark the nodes where expansion stops using pNode->fMarkA
    Abc_NtkForEachNode( pNtk, pNode, i )
        pNode->fMarkA = 1;
}

/**Function*************************************************************

  Synopsis    [Sets a factor-cut boundary.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMultiSetBoundsFactor( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode;
    int i;
    // make sure the mark is not set
    Abc_NtkForEachObj( pNtk, pNode, i )
        assert( pNode->fMarkA == 0 );
    // mark the nodes where expansion stops using pNode->fMarkA
    Abc_NtkForEachNode( pNtk, pNode, i )
        pNode->fMarkA = (pNode->vFanouts.nSize > 1 && !Abc_NodeIsMuxControlType(pNode));
    // mark the PO drivers
    Abc_NtkForEachCo( pNtk, pNode, i )
        Abc_ObjFanin0(pNode)->fMarkA = 1;
}

/**Function*************************************************************

  Synopsis    [Collects the fanins of a large node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMultiCone_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vCone )
{
    assert( !Abc_ObjIsComplement(pNode) );
    if ( pNode->fMarkA || !Abc_ObjIsNode(pNode) )
    {
        Vec_PtrPushUnique( vCone, pNode );
        return;
    }
    Abc_NtkMultiCone_rec( Abc_ObjFanin(pNode,0), vCone );
    Abc_NtkMultiCone_rec( Abc_ObjFanin(pNode,1), vCone );
}

/**Function*************************************************************

  Synopsis    [Collects the fanins of a large node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkMultiCone( Abc_Obj_t * pNode, Vec_Ptr_t * vCone )
{
    assert( !Abc_ObjIsComplement(pNode) );
    assert( Abc_ObjIsNode(pNode) );
    vCone->nSize = 0;
    Abc_NtkMultiCone_rec( Abc_ObjFanin(pNode,0), vCone );
    Abc_NtkMultiCone_rec( Abc_ObjFanin(pNode,1), vCone );
}

#else

Abc_Ntk_t * Abc_NtkMulti( Abc_Ntk_t * pNtk, int nThresh, int nFaninMax, int fCnf, int fMulti, int fSimple, int fFactor ) { return NULL; }

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

