/**CFile****************************************************************

  FileName    [abcUnreach.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Computes unreachable states for small benchmarks.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcUnreach.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

static DdNode *    Abc_NtkTransitionRelation( DdManager * dd, Abc_Ntk_t * pNtk, int fVerbose );
static DdNode *    Abc_NtkInitStateAndVarMap( DdManager * dd, Abc_Ntk_t * pNtk, int fVerbose );
static DdNode *    Abc_NtkComputeUnreachable( DdManager * dd, Abc_Ntk_t * pNtk, DdNode * bRelation, DdNode * bInitial, int fVerbose );
static Abc_Ntk_t * Abc_NtkConstructExdc     ( DdManager * dd, Abc_Ntk_t * pNtk, DdNode * bUnreach );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Extracts sequential DCs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkExtractSequentialDcs( Abc_Ntk_t * pNtk, int fVerbose )
{
    int fReorder = 1;
    DdManager * dd;
    DdNode * bRelation, * bInitial, * bUnreach;

    // remove EXDC network if present
    if ( pNtk->pExdc )
    {
        Abc_NtkDelete( pNtk->pExdc );
        pNtk->pExdc = NULL; 
    }

    // compute the global BDDs of the latches
    dd = (DdManager *)Abc_NtkBuildGlobalBdds( pNtk, 10000000, 1, 1, fVerbose );    
    if ( dd == NULL )
        return 0;
    if ( fVerbose )
        printf( "Shared BDD size = %6d nodes.\n", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );

    // create the transition relation (dereferenced global BDDs)
    bRelation = Abc_NtkTransitionRelation( dd, pNtk, fVerbose );              Cudd_Ref( bRelation );
    // create the initial state and the variable map
    bInitial  = Abc_NtkInitStateAndVarMap( dd, pNtk, fVerbose );              Cudd_Ref( bInitial );
    // compute the unreachable states
    bUnreach  = Abc_NtkComputeUnreachable( dd, pNtk, bRelation, bInitial, fVerbose );   Cudd_Ref( bUnreach );
    Cudd_RecursiveDeref( dd, bRelation );
    Cudd_RecursiveDeref( dd, bInitial );

    // reorder and disable reordering
    if ( fReorder )
    {
        if ( fVerbose )
            fprintf( stdout, "BDD nodes in the unreachable states before reordering %d.\n", Cudd_DagSize(bUnreach) );
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
        Cudd_AutodynDisable( dd );
        if ( fVerbose )
            fprintf( stdout, "BDD nodes in the unreachable states after reordering %d.\n", Cudd_DagSize(bUnreach) );
    }

    // allocate ZDD variables
    Cudd_zddVarsFromBddVars( dd, 2 );
    // create the EXDC network representing the unreachable states
    if ( pNtk->pExdc )
        Abc_NtkDelete( pNtk->pExdc );
    pNtk->pExdc = Abc_NtkConstructExdc( dd, pNtk, bUnreach );
    Cudd_RecursiveDeref( dd, bUnreach );
    Extra_StopManager( dd );
//    pNtk->pManGlob = NULL;

    // make sure that everything is okay
    if ( pNtk->pExdc && !Abc_NtkCheck( pNtk->pExdc ) )
    {
        printf( "Abc_NtkExtractSequentialDcs: The network check has failed.\n" );
        Abc_NtkDelete( pNtk->pExdc );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes the transition relation of the network.]

  Description [Assumes that the global BDDs are computed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkTransitionRelation( DdManager * dd, Abc_Ntk_t * pNtk, int fVerbose )
{
    DdNode * bRel, * bTemp, * bProd, * bVar, * bInputs;
    Abc_Obj_t * pNode;
    int fReorder = 1;
    int i;

    // extand the BDD manager to represent NS variables
    assert( dd->size == Abc_NtkCiNum(pNtk) );
    Cudd_bddIthVar( dd, Abc_NtkCiNum(pNtk) + Abc_NtkLatchNum(pNtk) - 1 );

    // enable reordering
    if ( fReorder )
        Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
    else
        Cudd_AutodynDisable( dd );

    // compute the transition relation
    bRel = b1;   Cudd_Ref( bRel );
    Abc_NtkForEachLatch( pNtk, pNode, i )
    {
        bVar  = Cudd_bddIthVar( dd, Abc_NtkCiNum(pNtk) + i );
//        bProd = Cudd_bddXnor( dd, bVar, pNtk->vFuncsGlob->pArray[i] );  Cudd_Ref( bProd );
        bProd = Cudd_bddXnor( dd, bVar, (DdNode *)Abc_ObjGlobalBdd(Abc_ObjFanin0(pNode)) );  Cudd_Ref( bProd );
        bRel  = Cudd_bddAnd( dd, bTemp = bRel, bProd );                 Cudd_Ref( bRel );
        Cudd_RecursiveDeref( dd, bTemp ); 
        Cudd_RecursiveDeref( dd, bProd ); 
    }
    // free the global BDDs
//    Abc_NtkFreeGlobalBdds( pNtk );
    Abc_NtkFreeGlobalBdds( pNtk, 0 );

    // quantify the PI variables
    bInputs = Extra_bddComputeRangeCube( dd, 0, Abc_NtkPiNum(pNtk) );    Cudd_Ref( bInputs );
    bRel    = Cudd_bddExistAbstract( dd, bTemp = bRel, bInputs );    Cudd_Ref( bRel );
    Cudd_RecursiveDeref( dd, bTemp ); 
    Cudd_RecursiveDeref( dd, bInputs ); 

    // reorder and disable reordering
    if ( fReorder )
    {
        if ( fVerbose )
            fprintf( stdout, "BDD nodes in the transition relation before reordering %d.\n", Cudd_DagSize(bRel) );
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
        Cudd_AutodynDisable( dd );
        if ( fVerbose )
            fprintf( stdout, "BDD nodes in the transition relation after reordering %d.\n", Cudd_DagSize(bRel) );
    }
    Cudd_Deref( bRel );
    return bRel;
}

/**Function*************************************************************

  Synopsis    [Computes the initial state and sets up the variable map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkInitStateAndVarMap( DdManager * dd, Abc_Ntk_t * pNtk, int fVerbose )
{
    DdNode ** pbVarsX, ** pbVarsY;
    DdNode * bTemp, * bProd, * bVar;
    Abc_Obj_t * pLatch;
    int i;

    // set the variable mapping for Cudd_bddVarMap()
    pbVarsX = ABC_ALLOC( DdNode *, dd->size );
    pbVarsY = ABC_ALLOC( DdNode *, dd->size );
    bProd = b1;         Cudd_Ref( bProd );
    Abc_NtkForEachLatch( pNtk, pLatch, i )
    {
        pbVarsX[i] = dd->vars[ Abc_NtkPiNum(pNtk) + i ];
        pbVarsY[i] = dd->vars[ Abc_NtkCiNum(pNtk) + i ];
        // get the initial value of the latch
        bVar  = Cudd_NotCond( pbVarsX[i], !Abc_LatchIsInit1(pLatch) );
        bProd = Cudd_bddAnd( dd, bTemp = bProd, bVar );      Cudd_Ref( bProd );
        Cudd_RecursiveDeref( dd, bTemp ); 
    }
    Cudd_SetVarMap( dd, pbVarsX, pbVarsY, Abc_NtkLatchNum(pNtk) );
    ABC_FREE( pbVarsX );
    ABC_FREE( pbVarsY );

    Cudd_Deref( bProd );
    return bProd;
}

/**Function*************************************************************

  Synopsis    [Computes the set of unreachable states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NtkComputeUnreachable( DdManager * dd, Abc_Ntk_t * pNtk, DdNode * bTrans, DdNode * bInitial, int fVerbose )
{
    DdNode * bRelation, * bReached, * bCubeCs;
    DdNode * bCurrent, * bNext, * bTemp;
    int nIters, nMints;

    // perform reachability analisys
    bCurrent  = bInitial;   Cudd_Ref( bCurrent );
    bReached  = bInitial;   Cudd_Ref( bReached );
    bRelation = bTrans;     Cudd_Ref( bRelation );
    bCubeCs   = Extra_bddComputeRangeCube( dd, Abc_NtkPiNum(pNtk), Abc_NtkCiNum(pNtk) );    Cudd_Ref( bCubeCs );
    for ( nIters = 1; ; nIters++ )
    {
        // compute the next states
        bNext = Cudd_bddAndAbstract( dd, bRelation, bCurrent, bCubeCs );  Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bCurrent );
        // remap these states into the current state vars
        bNext = Cudd_bddVarMap( dd, bTemp = bNext );                      Cudd_Ref( bNext );
        Cudd_RecursiveDeref( dd, bTemp );
        // check if there are any new states
        if ( Cudd_bddLeq( dd, bNext, bReached ) )
            break;
        // get the new states
        bCurrent = Cudd_bddAnd( dd, bNext, Cudd_Not(bReached) );          Cudd_Ref( bCurrent );
        // minimize the new states with the reached states
//        bCurrent = Cudd_bddConstrain( dd, bTemp = bCurrent, Cudd_Not(bReached) ); Cudd_Ref( bCurrent );
//        Cudd_RecursiveDeref( dd, bTemp );
        // add to the reached states
        bReached = Cudd_bddOr( dd, bTemp = bReached, bNext );             Cudd_Ref( bReached );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bNext );
        // minimize the transition relation
//        bRelation = Cudd_bddConstrain( dd, bTemp = bRelation, Cudd_Not(bReached) ); Cudd_Ref( bRelation );
//        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_RecursiveDeref( dd, bRelation );
    Cudd_RecursiveDeref( dd, bCubeCs );
    Cudd_RecursiveDeref( dd, bNext );
    // report the stats
    if ( fVerbose )
    {
        nMints = (int)Cudd_CountMinterm(dd, bReached, Abc_NtkLatchNum(pNtk) );
        fprintf( stdout, "Reachability analysis completed in %d iterations.\n", nIters );
        fprintf( stdout, "The number of minterms in the reachable state set = %d. (%6.2f %%)\n", nMints, 100.0*nMints/(1<<Abc_NtkLatchNum(pNtk)) );
    }
//ABC_PRB( dd, bReached );
    Cudd_Deref( bReached );
    return Cudd_Not( bReached );
}

/**Function*************************************************************

  Synopsis    [Creates the EXDC network.]

  Description [The set of unreachable states depends on CS variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkConstructExdc( DdManager * dd, Abc_Ntk_t * pNtk, DdNode * bUnreach )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pNode, * pNodeNew;
    int * pPermute;
    int i;

    // start the new network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_BDD, 1 );
    pNtkNew->pName = Extra_UtilStrsav( "exdc" );
    pNtkNew->pSpec = NULL;

    // create PIs corresponding to LOs
    Abc_NtkForEachLatchOutput( pNtk, pNode, i )
        Abc_ObjAssignName( pNode->pCopy = Abc_NtkCreatePi(pNtkNew), Abc_ObjName(pNode), NULL );
    // cannot ADD POs here because pLatch->pCopy point to the PIs

    // create a new node
    pNodeNew = Abc_NtkCreateNode(pNtkNew);
    // add the fanins corresponding to latch outputs
    Abc_NtkForEachLatchOutput( pNtk, pNode, i )
        Abc_ObjAddFanin( pNodeNew, pNode->pCopy );

    // create the logic function
    pPermute = ABC_ALLOC( int, dd->size );
    for ( i = 0; i < dd->size; i++ )
        pPermute[i] = -1;
    Abc_NtkForEachLatch( pNtk, pNode, i )
        pPermute[Abc_NtkPiNum(pNtk) + i] = i;
    // remap the functions
    pNodeNew->pData = Extra_TransferPermute( dd, (DdManager *)pNtkNew->pManFunc, bUnreach, pPermute );   Cudd_Ref( (DdNode *)pNodeNew->pData );
    ABC_FREE( pPermute );
    Abc_NodeMinimumBase( pNodeNew );

    // for each CO, create PO (skip POs equal to CIs because of name conflict)
    Abc_NtkForEachPo( pNtk, pNode, i )
        if ( !Abc_ObjIsCi(Abc_ObjFanin0(pNode)) )
            Abc_ObjAssignName( pNode->pCopy = Abc_NtkCreatePo(pNtkNew), Abc_ObjName(pNode), NULL );
    Abc_NtkForEachLatchInput( pNtk, pNode, i )
        Abc_ObjAssignName( pNode->pCopy = Abc_NtkCreatePo(pNtkNew), Abc_ObjName(pNode), NULL );

    // link to the POs of the network 
    Abc_NtkForEachPo( pNtk, pNode, i )
        if ( !Abc_ObjIsCi(Abc_ObjFanin0(pNode)) )
            Abc_ObjAddFanin( pNode->pCopy, pNodeNew );
    Abc_NtkForEachLatchInput( pNtk, pNode, i )
        Abc_ObjAddFanin( pNode->pCopy, pNodeNew );

    // remove the extra nodes
    Abc_AigCleanup( (Abc_Aig_t *)pNtkNew->pManFunc );

    // fix the problem with complemented and duplicated CO edges
    Abc_NtkLogicMakeSimpleCos( pNtkNew, 0 );

    // transform the network to the SOP representation
    if ( !Abc_NtkBddToSop( pNtkNew, -1, ABC_INFINITY ) )
    {
        printf( "Abc_NtkConstructExdc(): Converting to SOPs has failed.\n" );
        return NULL;
    }
    return pNtkNew;
//    return NULL;
}

#else

int Abc_NtkExtractSequentialDcs( Abc_Ntk_t * pNtk, int fVerbose ) { return 1; }

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

