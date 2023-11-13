/**CFile****************************************************************

  FileName    [abcDsd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Technology dependent sweep.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcDsd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "proof/fraig/fraig.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void           Abc_NtkFraigSweepUsingExdc( Fraig_Man_t * pMan, Abc_Ntk_t * pNtk );
static stmm_table *   Abc_NtkFraigEquiv( Abc_Ntk_t * pNtk, int fUseInv, int fVerbose, int fVeryVerbose );
static void           Abc_NtkFraigTransform( Abc_Ntk_t * pNtk, stmm_table * tEquiv, int fUseInv, int fVerbose );
static void           Abc_NtkFraigMergeClassMapped( Abc_Ntk_t * pNtk, Abc_Obj_t * pChain, int fUseInv, int fVerbose );
static void           Abc_NtkFraigMergeClass( Abc_Ntk_t * pNtk, Abc_Obj_t * pChain, int fUseInv, int fVerbose );
static int            Abc_NodeDroppingCost( Abc_Obj_t * pNode );

static int            Abc_NtkReduceNodes( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes );
static void           Abc_NodeSweep( Abc_Obj_t * pNode, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sweping functionally equivalence nodes.]

  Description [Removes gates with equivalent functionality. Works for 
  both technology-independent and mapped networks. If the flag is set, 
  allows adding inverters at the gate outputs.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkFraigSweep( Abc_Ntk_t * pNtk, int fUseInv, int fExdc, int fVerbose, int fVeryVerbose )
{
    Fraig_Params_t Params;
    Abc_Ntk_t * pNtkAig;
    Fraig_Man_t * pMan;
    stmm_table * tEquiv;
    Abc_Obj_t * pObj;
    int i, fUseTrick;

    assert( !Abc_NtkIsStrash(pNtk) );

    // save gate assignments
    fUseTrick = 0;
    if ( Abc_NtkIsMappedLogic(pNtk) )
    {
        fUseTrick = 1;
        Abc_NtkForEachNode( pNtk, pObj, i )
            pObj->pNext = (Abc_Obj_t *)pObj->pData;
    }
    // derive the AIG
    pNtkAig = Abc_NtkStrash( pNtk, 0, 1, 0 );
    // reconstruct gate assignments
    if ( fUseTrick )
    {
//        extern void * Abc_FrameReadLibGen(); 
        Hop_ManStop( (Hop_Man_t *)pNtk->pManFunc );
        pNtk->pManFunc = Abc_FrameReadLibGen();
        pNtk->ntkFunc = ABC_FUNC_MAP;
        Abc_NtkForEachNode( pNtk, pObj, i )
            pObj->pData = pObj->pNext, pObj->pNext = NULL;
    }

    // perform fraiging of the AIG
    Fraig_ParamsSetDefault( &Params );
    Params.fInternal = 1;
    pMan = (Fraig_Man_t *)Abc_NtkToFraig( pNtkAig, &Params, 0, 0 );   
    // cannot use EXDC with FRAIG because it can create classes of equivalent FRAIG nodes
    // with representative nodes that do not correspond to the nodes with the current network

    // update FRAIG using EXDC
    if ( fExdc )
    {
        if ( pNtk->pExdc == NULL )
            printf( "Warning: Networks has no EXDC.\n" );
        else
            Abc_NtkFraigSweepUsingExdc( pMan, pNtk );
    }
    // assign levels to the nodes of the network
    Abc_NtkLevel( pNtk );

    // collect the classes of equivalent nets
    tEquiv = Abc_NtkFraigEquiv( pNtk, fUseInv, fVerbose, fVeryVerbose );

    // transform the network into the equivalent one
    Abc_NtkFraigTransform( pNtk, tEquiv, fUseInv, fVerbose );
    stmm_free_table( tEquiv );

    // free the manager
    Fraig_ManFree( pMan );
    Abc_NtkDelete( pNtkAig );

    // cleanup the dangling nodes
    if ( Abc_NtkHasMapping(pNtk) )
        Abc_NtkCleanup( pNtk, fVerbose );
    else
        Abc_NtkSweep( pNtk, fVerbose );

    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkFraigSweep: The network check has failed.\n" );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Sweep the network using EXDC.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFraigSweepUsingExdc( Fraig_Man_t * pMan, Abc_Ntk_t * pNtk )
{
    Fraig_Node_t * gNodeExdc, * gNode, * gNodeRes;
    Abc_Obj_t * pNode, * pNodeAig;
    int i;
    extern Fraig_Node_t * Abc_NtkToFraigExdc( Fraig_Man_t * pMan, Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkExdc );

    assert( pNtk->pExdc );
    // derive FRAIG node representing don't-cares in the EXDC network
    gNodeExdc = Abc_NtkToFraigExdc( pMan, pNtk, pNtk->pExdc );
    // update the node pointers
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        // skip the constant input nodes
        if ( Abc_ObjFaninNum(pNode) == 0 )
            continue;
        // get the strashed node
        pNodeAig = pNode->pCopy;
        // skip the dangling nodes
        if ( pNodeAig == NULL )
            continue;
        // get the FRAIG node
        gNode = Fraig_NotCond( Abc_ObjRegular(pNodeAig)->pCopy, (int)Abc_ObjIsComplement(pNodeAig) );
        // perform ANDing with EXDC
        gNodeRes = Fraig_NodeAnd( pMan, gNode, Fraig_Not(gNodeExdc) );
        // write the node back
        Abc_ObjRegular(pNodeAig)->pCopy = (Abc_Obj_t *)Fraig_NotCond( gNodeRes, (int)Abc_ObjIsComplement(pNodeAig) );
    }
}

/**Function*************************************************************

  Synopsis    [Collects equivalence classes of node in the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
stmm_table * Abc_NtkFraigEquiv( Abc_Ntk_t * pNtk, int fUseInv, int fVerbose, int fVeryVerbose )
{
    Abc_Obj_t * pList, * pNode, * pNodeAig;
    Fraig_Node_t * gNode;
    Abc_Obj_t ** ppSlot;
    stmm_table * tStrash2Net;
    stmm_table * tResult;
    stmm_generator * gen;
    int c, Counter;

    // create mapping of strashed nodes into the corresponding network nodes
    tStrash2Net = stmm_init_table(stmm_ptrcmp,stmm_ptrhash);
    Abc_NtkForEachNode( pNtk, pNode, c )
    {
        // skip the constant input nodes
        if ( Abc_ObjFaninNum(pNode) == 0 )
            continue;
        // get the strashed node
        pNodeAig = pNode->pCopy;
        // skip the dangling nodes
        if ( pNodeAig == NULL )
            continue;
        // skip the nodes that fanout into COs
        if ( Abc_NodeFindCoFanout(pNode) )
            continue;
        // get the FRAIG node
        gNode = Fraig_NotCond( Abc_ObjRegular(pNodeAig)->pCopy, (int)Abc_ObjIsComplement(pNodeAig) );
        if ( !stmm_find_or_add( tStrash2Net, (char *)Fraig_Regular(gNode), (char ***)&ppSlot ) )
            *ppSlot = NULL;
        // add the node to the list
        pNode->pNext = *ppSlot;
        *ppSlot = pNode;
        // mark the node if it is complemented
        pNode->fPhase = Fraig_IsComplement(gNode);
    }

    // print the classes
    c = 0;
    Counter = 0;
    tResult = stmm_init_table(stmm_ptrcmp,stmm_ptrhash);
    stmm_foreach_item( tStrash2Net, gen, (char **)&gNode, (char **)&pList )
    {
        // skip the trival classes
        if ( pList == NULL || pList->pNext == NULL )
            continue;
        // add the non-trival class
        stmm_insert( tResult, (char *)pList, NULL );
        // count nodes in the non-trival classes
        for ( pNode = pList; pNode; pNode = pNode->pNext )
            Counter++;

        if ( fVeryVerbose )
        {
            printf( "Class %2d : {", c );
            for ( pNode = pList; pNode; pNode = pNode->pNext )
            {
                pNode->pCopy = NULL;
                printf( " %s", Abc_ObjName(pNode) );
                printf( "(%c)", pNode->fPhase? '-' : '+' );
                printf( "(%d)", pNode->Level );
            }
            printf( " }\n" );
            c++;
        }
    }
    if ( fVerbose || fVeryVerbose )
    {
        printf( "Sweeping stats for network \"%s\":\n", pNtk->pName );
        printf( "Internal nodes = %d. Different functions (up to compl) = %d.\n", Abc_NtkNodeNum(pNtk), stmm_count(tStrash2Net) );
        printf( "Non-trivial classes = %d. Nodes in non-trivial classes = %d.\n", stmm_count(tResult), Counter );
    }
    stmm_free_table( tStrash2Net );
    return tResult;
}


/**Function*************************************************************

  Synopsis    [Transforms the network using the equivalence relation on nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFraigTransform( Abc_Ntk_t * pNtk, stmm_table * tEquiv, int fUseInv, int fVerbose )
{
    stmm_generator * gen;
    Abc_Obj_t * pList;
    if ( stmm_count(tEquiv) == 0 )
        return;
    // merge nodes in the classes
    if ( Abc_NtkHasMapping( pNtk ) )
    {
        Abc_NtkDelayTrace( pNtk, NULL, NULL, 0 );
        stmm_foreach_item( tEquiv, gen, (char **)&pList, NULL )
            Abc_NtkFraigMergeClassMapped( pNtk, pList, fUseInv, fVerbose );
    }
    else 
    {
        stmm_foreach_item( tEquiv, gen, (char **)&pList, NULL )
            Abc_NtkFraigMergeClass( pNtk, pList, fUseInv, fVerbose );
    }
}


/**Function*************************************************************

  Synopsis    [Transforms the list of one-phase equivalent nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFraigMergeClassMapped( Abc_Ntk_t * pNtk, Abc_Obj_t * pChain, int fUseInv, int fVerbose )
{
    Abc_Obj_t * pListDir, * pListInv;
    Abc_Obj_t * pNodeMin, * pNode, * pNext;
    float Arrival1, Arrival2;

    assert( pChain );
    assert( pChain->pNext );

    // divide the nodes into two parts: 
    // those that need the invertor and those that don't need
    pListDir = pListInv = NULL;
    for ( pNode = pChain, pNext = pChain->pNext; 
          pNode; 
          pNode = pNext, pNext = pNode? pNode->pNext : NULL )
    {
        // check to which class the node belongs
        if ( pNode->fPhase == 1 )
        {
            pNode->pNext = pListDir;
            pListDir = pNode;
        }
        else
        {
            pNode->pNext = pListInv;
            pListInv = pNode;
        }
    }

    // find the node with the smallest number of logic levels
    pNodeMin = pListDir;
    for ( pNode = pListDir; pNode; pNode = pNode->pNext )
    {
        Arrival1 = Abc_NodeReadArrivalWorst(pNodeMin);
        Arrival2 = Abc_NodeReadArrivalWorst(pNode   );
//        assert( Abc_ObjIsCi(pNodeMin) || Arrival1 > 0 );
//        assert( Abc_ObjIsCi(pNode)    || Arrival2 > 0 );
        if (  Arrival1 > Arrival2 ||
              (Arrival1 == Arrival2 && pNodeMin->Level >  pNode->Level) ||
              (Arrival1 == Arrival2 && pNodeMin->Level == pNode->Level &&
              Abc_NodeDroppingCost(pNodeMin) < Abc_NodeDroppingCost(pNode))  )
            pNodeMin = pNode;
    }

    // move the fanouts of the direct nodes
    for ( pNode = pListDir; pNode; pNode = pNode->pNext )
        if ( pNode != pNodeMin )
            Abc_ObjTransferFanout( pNode, pNodeMin );

    // find the node with the smallest number of logic levels
    pNodeMin = pListInv;
    for ( pNode = pListInv; pNode; pNode = pNode->pNext )
    {
        Arrival1 = Abc_NodeReadArrivalWorst(pNodeMin);
        Arrival2 = Abc_NodeReadArrivalWorst(pNode   );
//        assert( Abc_ObjIsCi(pNodeMin) || Arrival1 > 0 );
//        assert( Abc_ObjIsCi(pNode)    || Arrival2 > 0 );
        if (  Arrival1 > Arrival2 ||
              (Arrival1 == Arrival2 && pNodeMin->Level >  pNode->Level) ||
              (Arrival1 == Arrival2 && pNodeMin->Level == pNode->Level &&
              Abc_NodeDroppingCost(pNodeMin) < Abc_NodeDroppingCost(pNode))  )
            pNodeMin = pNode;
    }

    // move the fanouts of the direct nodes
    for ( pNode = pListInv; pNode; pNode = pNode->pNext )
        if ( pNode != pNodeMin )
            Abc_ObjTransferFanout( pNode, pNodeMin );
}

/**Function*************************************************************

  Synopsis    [Process one equivalence class of nodes.]

  Description [This function does not remove the nodes. It only switches 
  around the connections.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkFraigMergeClass( Abc_Ntk_t * pNtk, Abc_Obj_t * pChain, int fUseInv, int fVerbose )
{
    Abc_Obj_t * pListDir, * pListInv;
    Abc_Obj_t * pNodeMin, * pNodeMinInv;
    Abc_Obj_t * pNode, * pNext;

    assert( pChain );
    assert( pChain->pNext );

    // find the node with the smallest number of logic levels
    pNodeMin = pChain;
    for ( pNode = pChain->pNext; pNode; pNode = pNode->pNext )
        if (  pNodeMin->Level >  pNode->Level || 
            ( pNodeMin->Level == pNode->Level && 
              Abc_NodeDroppingCost(pNodeMin) < Abc_NodeDroppingCost(pNode) ) )
            pNodeMin = pNode;

    // divide the nodes into two parts: 
    // those that need the invertor and those that don't need
    pListDir = pListInv = NULL;
    for ( pNode = pChain, pNext = pChain->pNext; 
          pNode; 
          pNode = pNext, pNext = pNode? pNode->pNext : NULL )
    {
        if ( pNode == pNodeMin )
            continue;
        // check to which class the node belongs
        if ( pNodeMin->fPhase == pNode->fPhase )
        {
            pNode->pNext = pListDir;
            pListDir = pNode;
        }
        else
        {
            pNode->pNext = pListInv;
            pListInv = pNode;
        }
    }

    // move the fanouts of the direct nodes
    for ( pNode = pListDir; pNode; pNode = pNode->pNext )
        Abc_ObjTransferFanout( pNode, pNodeMin );

    // skip if there are no inverted nodes
    if ( pListInv == NULL )
        return;

    // add the invertor
    pNodeMinInv = Abc_NtkCreateNodeInv( pNtk, pNodeMin );
   
    // move the fanouts of the inverted nodes
    for ( pNode = pListInv; pNode; pNode = pNode->pNext )
        Abc_ObjTransferFanout( pNode, pNodeMinInv );
}


/**Function*************************************************************

  Synopsis    [Returns the number of literals saved if this node becomes useless.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeDroppingCost( Abc_Obj_t * pNode )
{ 
    return 1;
}





/**Function*************************************************************

  Synopsis    [Removes dangling nodes.]

  Description [Returns the number of nodes removed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCleanup( Abc_Ntk_t * pNtk, int fVerbose )
{
    Vec_Ptr_t * vNodes;
    int Counter;
    assert( Abc_NtkIsLogic(pNtk) );
    // mark the nodes reachable from the POs
    vNodes = Abc_NtkDfs( pNtk, 0 );
    Counter = Abc_NtkReduceNodes( pNtk, vNodes );
    if ( fVerbose )
        printf( "Cleanup removed %d dangling nodes.\n", Counter );
    Vec_PtrFree( vNodes );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Removes dangling nodes.]

  Description [Returns the number of nodes removed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCleanupNodes( Abc_Ntk_t * pNtk, Vec_Ptr_t * vRoots, int fVerbose )
{
    Vec_Ptr_t * vNodes, * vStarts;
    Abc_Obj_t * pObj;
    int i, Counter;
    assert( Abc_NtkIsLogic(pNtk) );
    // collect starting nodes into one array
    vStarts = Vec_PtrAlloc( 1000 );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_PtrPush( vStarts, pObj );
    Vec_PtrForEachEntry( Abc_Obj_t *, vRoots, pObj, i )
        if ( pObj )
            Vec_PtrPush( vStarts, pObj );
    // mark the nodes reachable from the POs
    vNodes = Abc_NtkDfsNodes( pNtk, (Abc_Obj_t **)Vec_PtrArray(vStarts), Vec_PtrSize(vStarts) );
    Vec_PtrFree( vStarts );
    Counter = Abc_NtkReduceNodes( pNtk, vNodes );
    if ( fVerbose )
        printf( "Cleanup removed %d dangling nodes.\n", Counter );
    Vec_PtrFree( vNodes );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Preserves the nodes collected in the array.]

  Description [Returns the number of nodes removed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkReduceNodes( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pNode;
    int i, Counter;
    assert( Abc_NtkIsLogic(pNtk) );
    // mark the nodes reachable from the POs
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
        pNode->fMarkA = 1;
    // remove the non-marked nodes
    Counter = 0;
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( pNode->fMarkA == 0 )
        {
            Abc_NtkDeleteObj( pNode );
            Counter++;
        }
    // unmark the remaining nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
        pNode->fMarkA = 0;
    // check
    if ( !Abc_NtkCheck( pNtk ) )
        printf( "Abc_NtkCleanup: The network check has failed.\n" );
    return Counter;
}




#ifdef ABC_USE_CUDD

/**Function*************************************************************

  Synopsis    [Replaces the local function by its cofactor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NodeConstantInput( Abc_Obj_t * pNode, Abc_Obj_t * pFanin, int fConst0 )
{
    DdManager * dd = (DdManager *)pNode->pNtk->pManFunc;
    DdNode * bVar, * bTemp;
    int iFanin;
    assert( Abc_NtkIsBddLogic(pNode->pNtk) ); 
    if ( (iFanin = Vec_IntFind( &pNode->vFanins, pFanin->Id )) == -1 )
    {
        printf( "Node %s should be among", Abc_ObjName(pFanin) );
        printf( " the fanins of node %s...\n", Abc_ObjName(pNode) );
        return;
    }
    bVar = Cudd_NotCond( Cudd_bddIthVar(dd, iFanin), fConst0 );
    pNode->pData = Cudd_Cofactor( dd, bTemp = (DdNode *)pNode->pData, bVar );   Cudd_Ref( (DdNode *)pNode->pData );
    Cudd_RecursiveDeref( dd, bTemp );
}

/**Function*************************************************************

  Synopsis    [Tranditional sweep of the network.]

  Description [Propagates constant and single-input node, removes dangling nodes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkSweep( Abc_Ntk_t * pNtk, int fVerbose )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pNode, * pFanout, * pDriver;
    int i, nNodesOld;
    assert( Abc_NtkIsLogic(pNtk) ); 
    // convert network to BDD representation
    if ( !Abc_NtkToBdd(pNtk) )
    {
        fprintf( stdout, "Converting to BDD has failed.\n" );
        return 1;
    }
    // perform cleanup
    nNodesOld = Abc_NtkNodeNum(pNtk);
    Abc_NtkCleanup( pNtk, 0 );
    // prepare nodes for sweeping
    //Abc_NtkRemoveDupFanins(pNtk);
    Abc_NtkMinimumBase(pNtk);
    // collect sweepable nodes
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NtkForEachNode( pNtk, pNode, i )
        if ( Abc_ObjFaninNum(pNode) < 2 )
            Vec_PtrPush( vNodes, pNode );
    // sweep the nodes
    while ( Vec_PtrSize(vNodes) > 0 )
    {
        // get any sweepable node
        pNode = (Abc_Obj_t *)Vec_PtrPop(vNodes);
        if ( !Abc_ObjIsNode(pNode) )
            continue;
        // get any non-CO fanout of this node
        pFanout = Abc_NodeFindNonCoFanout(pNode);
        if ( pFanout == NULL )
            continue;
        assert( Abc_ObjIsNode(pFanout) );
        // transform the function of the fanout
        if ( Abc_ObjFaninNum(pNode) == 0 )
            Abc_NodeConstantInput( pFanout, pNode, Abc_NodeIsConst0(pNode) );
        else 
        {
            assert( Abc_ObjFaninNum(pNode) == 1 );
            pDriver = Abc_ObjFanin0(pNode);
            if ( Abc_NodeIsInv(pNode) )
                Abc_NodeComplementInput( pFanout, pNode );
            Abc_ObjPatchFanin( pFanout, pNode, pDriver );
        }
        //Abc_NodeRemoveDupFanins( pFanout );
        Abc_NodeMinimumBase( pFanout );
        // check if the fanout should be added
        if ( Abc_ObjFaninNum(pFanout) < 2 )
            Vec_PtrPush( vNodes, pFanout );
        // check if the node has other fanouts
        if ( Abc_ObjFanoutNum(pNode) > 0 )
            Vec_PtrPush( vNodes, pNode );
        else
            Abc_NtkDeleteObj_rec( pNode, 1 );
    }
    Vec_PtrFree( vNodes );
    // sweep a node into its CO fanout if all of this is true:
    // (a) this node is a single-input node
    // (b) the driver of the node has only one fanout (this node)
    // (c) the driver is a node
    Abc_NtkForEachCo( pNtk, pFanout, i )
    {
        pNode = Abc_ObjFanin0(pFanout);
        if ( Abc_ObjFaninNum(pNode) != 1 )
            continue;
        pDriver = Abc_ObjFanin0(pNode);
        if ( !(Abc_ObjFanoutNum(pDriver) == 1 && Abc_ObjIsNode(pDriver)) )
            continue;
        // trasform this CO
        if ( Abc_NodeIsInv(pNode) )
            pDriver->pData = Cudd_Not(pDriver->pData);
        Abc_ObjPatchFanin( pFanout, pNode, pDriver );
    }
    // perform cleanup
    Abc_NtkCleanup( pNtk, 0 );
    // report
    if ( fVerbose )
        printf( "Sweep removed %d nodes.\n", nNodesOld - Abc_NtkNodeNum(pNtk) );
    return nNodesOld - Abc_NtkNodeNum(pNtk);
}


#else

int Abc_NtkSweep( Abc_Ntk_t * pNtk, int fVerbose ) { return 1; }

#endif


/**Function*************************************************************

  Synopsis    [Removes all objects whose trav ID is not current.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeRemoveNonCurrentObjects( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int Counter, i;
    int fVerbose = 0;

    // report on the nodes to be deleted
    if ( fVerbose )
    {
        printf( "These nodes will be deleted: \n" );
        Abc_NtkForEachObj( pNtk, pObj, i )
            if ( !Abc_NodeIsTravIdCurrent( pObj ) )
            {
                printf( "    " );
                Abc_ObjPrint( stdout, pObj );
            }
    }
    
    // delete the nodes    
    Counter = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_NodeIsTravIdCurrent( pObj ) )
        {
            Abc_NtkDeleteObj( pObj );
            Counter++;
        }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Check if the fanin of this latch is a constant.]

  Description [Returns 0/1 if constant; -1 if not a constant.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSetTravId_rec( Abc_Obj_t * pObj )
{
    Abc_NodeSetTravIdCurrent(pObj);
    if ( Abc_ObjFaninNum(pObj) == 0 )
        return;
    assert( Abc_ObjFaninNum(pObj) == 1 );
    Abc_NtkSetTravId_rec( Abc_ObjFanin0(pObj) );    
}

/**Function*************************************************************

  Synopsis    [Check if the fanin of this latch is a constant.]

  Description [Returns 0/1 if constant; -1 if not a constant.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCheckConstant_rec( Abc_Obj_t * pObj )
{
    if ( Abc_ObjFaninNum(pObj) == 0 )
    {
        if ( !Abc_ObjIsNode(pObj) )
            return -1;
        if ( Abc_NodeIsConst0(pObj) )
            return 0;
        if ( Abc_NodeIsConst1(pObj) )
            return 1;
        assert( 0 );
        return -1;
    }
    if ( Abc_ObjIsLatch(pObj) || Abc_ObjFaninNum(pObj) > 1 )
        return -1;
    if ( !Abc_ObjIsNode(pObj) || Abc_NodeIsBuf(pObj) )
        return Abc_NtkCheckConstant_rec( Abc_ObjFanin0(pObj) );
    if ( Abc_NodeIsInv(pObj) )
    {
        int RetValue = Abc_NtkCheckConstant_rec( Abc_ObjFanin0(pObj) );
        if ( RetValue == 0 )
            return 1;
        if ( RetValue == 1 )
            return 0;
        return RetValue;
    }
    assert( 0 );
    return -1;
}

/**Function*************************************************************

  Synopsis    [Removes redundant latches.]

  Description [The redundant latches are of two types:
  - Latches fed by a constant which matches the init value of the latch.
  - Latches fed by a constant which does not match the init value of the latch
  can be all replaced by one latch.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkLatchSweep( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pFanin, * pLatch, * pLatchPivot = NULL;
    int Counter, RetValue, i;
    Counter = 0;
    // go through the latches
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        // check if the latch has constant input
        RetValue = Abc_NtkCheckConstant_rec( Abc_ObjFanin0(pLatch) );
        if ( RetValue == -1 )
            continue;
        // found a latch with constant fanin
        if ( (RetValue == 1 && Abc_LatchIsInit0(pLatch)) ||
             (RetValue == 0 && Abc_LatchIsInit1(pLatch)) )
        {
            // fanin constant differs from the latch init value
            if ( pLatchPivot == NULL )
            {
                pLatchPivot = pLatch;
                continue;
            }
            if ( Abc_LatchInit(pLatch) != Abc_LatchInit(pLatchPivot) ) // add inverter
                pFanin = Abc_NtkCreateNodeInv( pNtk, Abc_ObjFanout0(pLatchPivot) );
            else
                pFanin = Abc_ObjFanout0(pLatchPivot);
        }
        else
            pFanin = Abc_ObjFanin0(Abc_ObjFanin0(pLatch));
        // replace latch
        Abc_ObjTransferFanout( Abc_ObjFanout0(pLatch), pFanin );
        // delete the extra nodes
        Abc_NtkDeleteObj_rec( Abc_ObjFanout0(pLatch), 0 );
        Counter++;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Replaces autonumnous logic by free inputs.]

  Description [Assumes that non-autonomous logic is marked with
  the current ID.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkReplaceAutonomousLogic( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode, * pFanin;
    Vec_Ptr_t * vNodes;
    int i, k, Counter;
    // collect the nodes that feed into the reachable logic
    vNodes = Vec_PtrAlloc( 100 );
    Abc_NtkForEachObj( pNtk, pNode, i )
    {
        // skip non-visited fanins
        if ( !Abc_NodeIsTravIdCurrent(pNode) )
            continue;
        // look for non-visited fanins
        Abc_ObjForEachFanin( pNode, pFanin, k )
        {
            // skip visited fanins
            if ( Abc_NodeIsTravIdCurrent(pFanin) )
                continue;
            // skip constants and latches fed by constants
            if ( Abc_NtkCheckConstant_rec(pFanin) != -1 ||
                 (Abc_ObjIsBo(pFanin) && Abc_NtkCheckConstant_rec(Abc_ObjFanin0(Abc_ObjFanin0(pFanin))) != -1) )
            {
                Abc_NtkSetTravId_rec( pFanin );
                continue;
            }
            assert( !Abc_ObjIsLatch(pFanin) );
            Vec_PtrPush( vNodes, pFanin );
        }
    }
    Vec_PtrUniqify( vNodes, (int (*)(const void *, const void *))Abc_ObjPointerCompare );
    // replace these nodes by the PIs
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        pFanin = Abc_NtkCreatePi(pNtk);
        Abc_ObjAssignName( pFanin, Abc_ObjName(pFanin), NULL );
        Abc_NodeSetTravIdCurrent( pFanin );
        Abc_ObjTransferFanout( pNode, pFanin );
    }
    Counter = Vec_PtrSize(vNodes);
    Vec_PtrFree( vNodes );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Sequential cleanup.]

  Description [Performs three tasks:
  - Removes logic that does not feed into POs.
  - Removes latches driven by constant values equal to the initial state.
  - Replaces the autonomous components by additional PI variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCleanupSeq( Abc_Ntk_t * pNtk, int fLatchSweep, int fAutoSweep, int fVerbose )
{
    Vec_Ptr_t * vNodes;
    int Counter;
    assert( Abc_NtkIsLogic(pNtk) );
    // mark the nodes reachable from the POs
    vNodes = Abc_NtkDfsSeq( pNtk );
    Vec_PtrFree( vNodes );
    // remove the non-marked nodes
    Counter = Abc_NodeRemoveNonCurrentObjects( pNtk );
    if ( fVerbose )
        printf( "Cleanup removed %4d dangling objects.\n", Counter );
    // check if some of the latches can be removed
    if ( fLatchSweep )
    {
        Counter = Abc_NtkLatchSweep( pNtk );
        if ( fVerbose )
            printf( "Cleanup removed %4d redundant latches.\n", Counter );
    }
    // detect the autonomous components
    if ( fAutoSweep )
    {
        vNodes = Abc_NtkDfsSeqReverse( pNtk );
        Vec_PtrFree( vNodes );
        // replace them by PIs
        Counter = Abc_NtkReplaceAutonomousLogic( pNtk );
        if ( fVerbose )
            printf( "Cleanup added   %4d additional PIs.\n", Counter );
        // remove the non-marked nodes
        Counter = Abc_NodeRemoveNonCurrentObjects( pNtk );
        if ( fVerbose )
            printf( "Cleanup removed %4d autonomous objects.\n", Counter );
    }
    // check
    if ( !Abc_NtkCheck( pNtk ) )
        printf( "Abc_NtkCleanupSeq: The network check has failed.\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Sweep to remove buffers and inverters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkSweepBufsInvs( Abc_Ntk_t * pNtk, int fVerbose )
{
    Hop_Man_t * pMan;
    Abc_Obj_t * pObj, * pFanin;
    int i, k, fChanges = 1, Counter = 0;
    assert( Abc_NtkIsLogic(pNtk) ); 
    // convert network to BDD representation
    if ( !Abc_NtkToAig(pNtk) )
    {
        fprintf( stdout, "Converting to SOP has failed.\n" );
        return 1;
    }
    // get AIG manager
    pMan = (Hop_Man_t *)pNtk->pManFunc;
    // label selected nodes
    Abc_NtkIncrementTravId( pNtk );
    // iterate till no improvement
    while ( fChanges )
    {
        fChanges = 0;
        Abc_NtkForEachObj( pNtk, pObj, i )
        {
            Abc_ObjForEachFanin( pObj, pFanin, k )
            {
                // do not eliminate marked fanins
                if ( Abc_NodeIsTravIdCurrent(pFanin) )
                    continue;
                // do not eliminate constant nodes
                if ( !Abc_ObjIsNode(pFanin) || Abc_ObjFaninNum(pFanin) != 1 )
                    continue;
                // do not eliminate inverters into COs
                if ( Abc_ObjIsCo(pObj) && Abc_NodeIsInv(pFanin) )
                    continue;
                // do not eliminate buffers connecting PIs and POs 
//                if ( Abc_ObjIsCo(pObj) && Abc_ObjIsCi(Abc_ObjFanin0(pFanin)) )
//                    continue;
                fChanges = 1;
                Counter++;
                // update function of the node
                if ( Abc_NodeIsInv(pFanin) )
                    pObj->pData = Hop_Compose( pMan, (Hop_Obj_t *)pObj->pData, Hop_Not(Hop_IthVar(pMan, k)), k );
                // update the fanin
                Abc_ObjPatchFanin( pObj, pFanin, Abc_ObjFanin0(pFanin) );
                if ( Abc_ObjFanoutNum(pFanin) == 0 )
                    Abc_NtkDeleteObj(pFanin);            
            }
        }
    }
    if ( fVerbose )
        printf( "Removed %d single input nodes.\n", Counter );
    return Counter;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

