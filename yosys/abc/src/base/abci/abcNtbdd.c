/**CFile****************************************************************

  FileName    [abcNtbdd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Procedures to translate between the BDD and the network.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcNtbdd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "aig/saig/saig.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_CUDD

       int         Abc_NtkBddToMuxesPerformGlo( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew, int Limit, int fReorder, int fUseAdd );
static void        Abc_NtkBddToMuxesPerform( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew );
static Abc_Obj_t * Abc_NodeBddToMuxes( Abc_Obj_t * pNodeOld, Abc_Ntk_t * pNtkNew );
static Abc_Obj_t * Abc_NodeBddToMuxes_rec( DdManager * dd, DdNode * bFunc, Abc_Ntk_t * pNtkNew, st__table * tBdd2Node );
static DdNode *    Abc_NodeGlobalBdds_rec( DdManager * dd, Abc_Obj_t * pNode, int nBddSizeMax, int fDropInternal, ProgressBar * pProgress, int * pCounter, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Constructs the network isomorphic to the given BDD.]

  Description [Assumes that the BDD depends on the variables whose indexes
  correspond to the names in the array (pNamesPi). Otherwise, returns NULL.
  The resulting network comes with one node, whose functionality is
  equal to the given BDD. To decompose this BDD into the network of
  multiplexers use Abc_NtkBddToMuxes(). To decompose this BDD into
  an And-Inverter Graph, use Abc_NtkStrash().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkDeriveFromBdd( void * dd0, void * bFunc, char * pNamePo, Vec_Ptr_t * vNamesPi )
{
    DdManager * dd = (DdManager *)dd0;
    Abc_Ntk_t * pNtk; 
    Vec_Ptr_t * vNamesPiFake = NULL;
    Abc_Obj_t * pNode, * pNodePi, * pNodePo;
    DdNode * bSupp, * bTemp;
    char * pName;
    int i;

    // supply fake names if real names are not given
    if ( pNamePo == NULL )
        pNamePo = "F";
    if ( vNamesPi == NULL )
    {
        vNamesPiFake = Abc_NodeGetFakeNames( dd->size );
        vNamesPi = vNamesPiFake;
    }

    // make sure BDD depends on the variables whose index 
    // does not exceed the size of the array with PI names
    bSupp = Cudd_Support( dd, (DdNode *)bFunc );   Cudd_Ref( bSupp );
    for ( bTemp = bSupp; bTemp != Cudd_ReadOne(dd); bTemp = cuddT(bTemp) )
        if ( (int)Cudd_NodeReadIndex(bTemp) >= Vec_PtrSize(vNamesPi) )
            break;
    Cudd_RecursiveDeref( dd, bSupp );
    if ( bTemp != Cudd_ReadOne(dd) )
        return NULL;

    // start the network
    pNtk = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_BDD, 1 );
    pNtk->pName = Extra_UtilStrsav(pNamePo);
    // make sure the new manager has enough inputs
    Cudd_bddIthVar( (DdManager *)pNtk->pManFunc, Vec_PtrSize(vNamesPi) );
    // add the PIs corresponding to the names
    Vec_PtrForEachEntry( char *, vNamesPi, pName, i )
        Abc_ObjAssignName( Abc_NtkCreatePi(pNtk), pName, NULL );
    // create the node
    pNode = Abc_NtkCreateNode( pNtk );
    pNode->pData = (DdNode *)Cudd_bddTransfer( dd, (DdManager *)pNtk->pManFunc, (DdNode *)bFunc ); Cudd_Ref((DdNode *)pNode->pData);
    Abc_NtkForEachPi( pNtk, pNodePi, i )
        Abc_ObjAddFanin( pNode, pNodePi );
    // create the only PO
    pNodePo = Abc_NtkCreatePo( pNtk );
    Abc_ObjAddFanin( pNodePo, pNode );
    Abc_ObjAssignName( pNodePo, pNamePo, NULL );
    // make the network minimum base
    Abc_NtkMinimumBase( pNtk );
    if ( vNamesPiFake )
        Abc_NodeFreeNames( vNamesPiFake );
    if ( !Abc_NtkCheck( pNtk ) )
        fprintf( stdout, "Abc_NtkDeriveFromBdd(): Network check has failed.\n" );
    return pNtk;
}



/**Function*************************************************************

  Synopsis    [Creates the network isomorphic to the union of local BDDs of the nodes.]

  Description [The nodes of the local BDDs are converted into the network nodes 
  with logic functions equal to the MUX.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkBddToMuxes( Abc_Ntk_t * pNtk, int fGlobal, int Limit, int fUseAdd )
{
    Abc_Ntk_t * pNtkNew;
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_LOGIC, ABC_FUNC_SOP );
    if ( fGlobal ) 
    {
        if ( !Abc_NtkBddToMuxesPerformGlo( pNtk, pNtkNew, Limit, 0, fUseAdd ) )
        {
            Abc_NtkDelete( pNtkNew );
            return NULL;
        }
    }
    else
    {
        Abc_NtkBddToMuxesPerform( pNtk, pNtkNew );
        Abc_NtkFinalize( pNtk, pNtkNew );
    }
    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkBddToMuxes: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Converts the network to MUXes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkBddToMuxesPerform( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pNode, * pNodeNew;
    Vec_Ptr_t * vNodes;
    int i;
    assert( Abc_NtkIsBddLogic(pNtk) );
    // perform conversion in the topological order
    vNodes = Abc_NtkDfs( pNtk, 0 );
    pProgress = Extra_ProgressBarStart( stdout, vNodes->nSize );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // convert one node
        assert( Abc_ObjIsNode(pNode) );
        pNodeNew = Abc_NodeBddToMuxes( pNode, pNtkNew );
        // mark the old node with the new one
        assert( pNode->pCopy == NULL );
        pNode->pCopy = pNodeNew;
    }
    Vec_PtrFree( vNodes );
    Extra_ProgressBarStop( pProgress );
}

/**Function*************************************************************

  Synopsis    [Converts the node to MUXes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeBddToMuxes( Abc_Obj_t * pNodeOld, Abc_Ntk_t * pNtkNew )
{
    DdManager * dd = (DdManager *)pNodeOld->pNtk->pManFunc;
    DdNode * bFunc = (DdNode *)pNodeOld->pData;
    Abc_Obj_t * pFaninOld, * pNodeNew;
    st__table * tBdd2Node;
    int i;
    // create the table mapping BDD nodes into the ABC nodes
    tBdd2Node = st__init_table( st__ptrcmp, st__ptrhash );
    // add the constant and the elementary vars
    Abc_ObjForEachFanin( pNodeOld, pFaninOld, i )
        st__insert( tBdd2Node, (char *)Cudd_bddIthVar(dd, i), (char *)pFaninOld->pCopy );
    // create the new nodes recursively
    pNodeNew = Abc_NodeBddToMuxes_rec( dd, Cudd_Regular(bFunc), pNtkNew, tBdd2Node );
    st__free_table( tBdd2Node );
    if ( Cudd_IsComplement(bFunc) )
        pNodeNew = Abc_NtkCreateNodeInv( pNtkNew, pNodeNew );
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Converts the node to MUXes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NodeBddToMuxes_rec( DdManager * dd, DdNode * bFunc, Abc_Ntk_t * pNtkNew, st__table * tBdd2Node )
{
    Abc_Obj_t * pNodeNew, * pNodeNew0, * pNodeNew1, * pNodeNewC;
    assert( !Cudd_IsComplement(bFunc) );
    //assert( b1 == a1 );
    if ( bFunc == a1 )
        return Abc_NtkCreateNodeConst1(pNtkNew);
    if ( bFunc == a0 )
        return Abc_NtkCreateNodeConst0(pNtkNew);
    if ( st__lookup( tBdd2Node, (char *)bFunc, (char **)&pNodeNew ) )
        return pNodeNew;
    // solve for the children nodes
    pNodeNew0 = Abc_NodeBddToMuxes_rec( dd, Cudd_Regular(cuddE(bFunc)), pNtkNew, tBdd2Node );
    if ( Cudd_IsComplement(cuddE(bFunc)) )
        pNodeNew0 = Abc_NtkCreateNodeInv( pNtkNew, pNodeNew0 );
    pNodeNew1 = Abc_NodeBddToMuxes_rec( dd, cuddT(bFunc), pNtkNew, tBdd2Node );
    if ( ! st__lookup( tBdd2Node, (char *)Cudd_bddIthVar(dd, bFunc->index), (char **)&pNodeNewC ) )
        assert( 0 );
    // create the MUX node
    pNodeNew = Abc_NtkCreateNodeMux( pNtkNew, pNodeNewC, pNodeNew1, pNodeNew0 );
    st__insert( tBdd2Node, (char *)bFunc, (char *)pNodeNew );
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Converts the node to MUXes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkBddToMuxesPerformGlo( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew, int Limit, int fReorder, int fUseAdd )
{
    DdManager * dd;
    Vec_Ptr_t * vAdds = fUseAdd ? Vec_PtrAlloc(100) : NULL;
    Abc_Obj_t * pObj, * pObjNew; int i;
    st__table * tBdd2Node;
    assert( Abc_NtkIsStrash(pNtk) );
    dd = (DdManager *)Abc_NtkBuildGlobalBdds( pNtk, Limit, 1, fReorder, 0, 0 );
    if ( dd == NULL )
    {
        printf( "Construction of global BDDs has failed.\n" );
        return 0;
    }
    //printf( "Shared BDD size = %6d nodes.\n", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );

    tBdd2Node = st__init_table( st__ptrcmp, st__ptrhash );
    Abc_NtkForEachCi( pNtkNew, pObjNew, i )
        st__insert( tBdd2Node, (char *)Cudd_bddIthVar(dd, i), (char *)pObjNew );

    // complement the global functions
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        DdNode * bFunc = (DdNode *)Abc_ObjGlobalBdd(pObj);
        if ( fUseAdd )
        {
            DdNode * aFunc = Cudd_BddToAdd( dd, bFunc );  Cudd_Ref( aFunc );
            pObjNew = Abc_NodeBddToMuxes_rec( dd, aFunc, pNtkNew, tBdd2Node );
            Vec_PtrPush( vAdds, aFunc );
        }
        else
        {
            pObjNew = Abc_NodeBddToMuxes_rec( dd, Cudd_Regular(bFunc), pNtkNew, tBdd2Node );
            if ( Cudd_IsComplement(bFunc) )
                pObjNew = Abc_NtkCreateNodeInv( pNtkNew, pObjNew );
        }
        Abc_ObjAddFanin( pObj->pCopy, pObjNew );
    }

    // cleanup
    st__free_table( tBdd2Node );
    Abc_NtkFreeGlobalBdds( pNtk, 0 );
    if ( vAdds )
    {
        DdNode * aTemp;
        Vec_PtrForEachEntry( DdNode *, vAdds, aTemp, i )
            Cudd_RecursiveDeref( dd, aTemp );
        Vec_PtrFree( vAdds );
    }
    Extra_StopManager( dd );
    Abc_NtkCleanCopy( pNtk );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Derives global BDDs for the COs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abc_NtkBuildGlobalBdds( Abc_Ntk_t * pNtk, int nBddSizeMax, int fDropInternal, int fReorder, int fReverse, int fVerbose )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pObj, * pFanin;
    Vec_Att_t * pAttMan;
    DdManager * dd;
    DdNode * bFunc;
    int i, k, Counter;

    // remove dangling nodes
    Abc_AigCleanup( (Abc_Aig_t *)pNtk->pManFunc );

    // start the manager
    assert( Abc_NtkGlobalBdd(pNtk) == NULL );
    dd = Cudd_Init( Abc_NtkCiNum(pNtk), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    pAttMan = Vec_AttAlloc( Abc_NtkObjNumMax(pNtk) + 1, dd, (void (*)(void*))Extra_StopManager, NULL, (void (*)(void*,void*))Cudd_RecursiveDeref );
    Vec_PtrWriteEntry( pNtk->vAttrs, VEC_ATTR_GLOBAL_BDD, pAttMan );

    // set reordering
    if ( fReorder )
        Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

    // assign the constant node BDD
    pObj = Abc_AigConst1(pNtk);
    if ( Abc_ObjFanoutNum(pObj) > 0 )
    {
        bFunc = dd->one;
        Abc_ObjSetGlobalBdd( pObj, bFunc );   Cudd_Ref( bFunc );
    }
    // set the elementary variables
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > 0 )
        {
            bFunc = fReverse ? dd->vars[Abc_NtkCiNum(pNtk) - 1 - i] : dd->vars[i];
            Abc_ObjSetGlobalBdd( pObj, bFunc );  Cudd_Ref( bFunc );
        }

    // collect the global functions of the COs
    Counter = 0;
    // construct the BDDs
    pProgress = Extra_ProgressBarStart( stdout, Abc_NtkNodeNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        bFunc = Abc_NodeGlobalBdds_rec( dd, Abc_ObjFanin0(pObj), nBddSizeMax, fDropInternal, pProgress, &Counter, fVerbose );
        if ( bFunc == NULL )
        {
            if ( fVerbose )
            printf( "Constructing global BDDs is aborted.\n" );
            Abc_NtkFreeGlobalBdds( pNtk, 0 );
            Cudd_Quit( dd ); 

            // reset references
            Abc_NtkForEachObj( pNtk, pObj, i )
                if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj) )
                    pObj->vFanouts.nSize = 0;
            Abc_NtkForEachObj( pNtk, pObj, i )
                if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
                    Abc_ObjForEachFanin( pObj, pFanin, k )
                        pFanin->vFanouts.nSize++;
            return NULL;
        }
        bFunc = Cudd_NotCond( bFunc, (int)Abc_ObjFaninC0(pObj) );  Cudd_Ref( bFunc ); 
        Abc_ObjSetGlobalBdd( pObj, bFunc );
    }
    Extra_ProgressBarStop( pProgress );

/*
    // derefence the intermediate BDDs
    Abc_NtkForEachNode( pNtk, pObj, i )
        if ( pObj->pCopy ) 
        {
            Cudd_RecursiveDeref( dd, (DdNode *)pObj->pCopy );
            pObj->pCopy = NULL;
        }
*/
/*
    // make sure all nodes are derefed
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        if ( pObj->pCopy != NULL )
            printf( "Abc_NtkBuildGlobalBdds() error: Node %d has BDD assigned\n", pObj->Id );
        if ( pObj->vFanouts.nSize > 0 )
            printf( "Abc_NtkBuildGlobalBdds() error: Node %d has refs assigned\n", pObj->Id );
    }
*/
    // reset references
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBi(pObj) )
            pObj->vFanouts.nSize = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( !Abc_ObjIsBox(pObj) && !Abc_ObjIsBo(pObj) )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                pFanin->vFanouts.nSize++;

    // reorder one more time
    if ( fReorder )
    {
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
//        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
        Cudd_AutodynDisable( dd );
    }
//    Cudd_PrintInfo( dd, stdout );
    return dd;
}

/**Function*************************************************************

  Synopsis    [Derives the global BDD for one AIG node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Abc_NodeGlobalBdds_rec( DdManager * dd, Abc_Obj_t * pNode, int nBddSizeMax, int fDropInternal, ProgressBar * pProgress, int * pCounter, int fVerbose )
{
    DdNode * bFunc, * bFunc0, * bFunc1, * bFuncC;
    int fDetectMuxes = 0;
    assert( !Abc_ObjIsComplement(pNode) );
    if ( Cudd_ReadKeys(dd)-Cudd_ReadDead(dd) > (unsigned)nBddSizeMax )
    {
        Extra_ProgressBarStop( pProgress );
        if ( fVerbose )
        printf( "The number of live nodes reached %d.\n", nBddSizeMax );
        fflush( stdout );
        return NULL;
    }
    // if the result is available return
    if ( Abc_ObjGlobalBdd(pNode) == NULL )
    {
        Abc_Obj_t * pNodeC, * pNode0, * pNode1;
        pNode0 = Abc_ObjFanin0(pNode);
        pNode1 = Abc_ObjFanin1(pNode);
        // check for the special case when it is MUX/EXOR
        if ( fDetectMuxes && 
             Abc_ObjGlobalBdd(pNode0) == NULL && Abc_ObjGlobalBdd(pNode1) == NULL &&
             Abc_ObjIsNode(pNode0) && Abc_ObjFanoutNum(pNode0) == 1 && 
             Abc_ObjIsNode(pNode1) && Abc_ObjFanoutNum(pNode1) == 1 && 
             Abc_NodeIsMuxType(pNode) )
        {
            // deref the fanins
            pNode0->vFanouts.nSize--;
            pNode1->vFanouts.nSize--;
            // recognize the MUX
            pNodeC = Abc_NodeRecognizeMux( pNode, &pNode1, &pNode0 );
            assert( Abc_ObjFanoutNum(pNodeC) > 1 );
            // dereference the control once (the second time it will be derefed when BDDs are computed)
            pNodeC->vFanouts.nSize--;

            // compute the result for all branches
            bFuncC = Abc_NodeGlobalBdds_rec( dd, pNodeC, nBddSizeMax, fDropInternal, pProgress, pCounter, fVerbose ); 
            if ( bFuncC == NULL )
                return NULL;
            Cudd_Ref( bFuncC );
            bFunc0 = Abc_NodeGlobalBdds_rec( dd, Abc_ObjRegular(pNode0), nBddSizeMax, fDropInternal, pProgress, pCounter, fVerbose ); 
            if ( bFunc0 == NULL )
                return NULL;
            Cudd_Ref( bFunc0 );
            bFunc1 = Abc_NodeGlobalBdds_rec( dd, Abc_ObjRegular(pNode1), nBddSizeMax, fDropInternal, pProgress, pCounter, fVerbose ); 
            if ( bFunc1 == NULL )
                return NULL;
            Cudd_Ref( bFunc1 );

            // complement the branch BDDs
            bFunc0 = Cudd_NotCond( bFunc0, (int)Abc_ObjIsComplement(pNode0) );
            bFunc1 = Cudd_NotCond( bFunc1, (int)Abc_ObjIsComplement(pNode1) );
            // get the final result
            bFunc = Cudd_bddIte( dd, bFuncC, bFunc1, bFunc0 );   Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bFunc0 );
            Cudd_RecursiveDeref( dd, bFunc1 );
            Cudd_RecursiveDeref( dd, bFuncC );
            // add the number of used nodes
            (*pCounter) += 3;
        }
        else
        {
            // compute the result for both branches
            bFunc0 = Abc_NodeGlobalBdds_rec( dd, Abc_ObjFanin(pNode,0), nBddSizeMax, fDropInternal, pProgress, pCounter, fVerbose ); 
            if ( bFunc0 == NULL )
                return NULL;
            Cudd_Ref( bFunc0 );
            bFunc1 = Abc_NodeGlobalBdds_rec( dd, Abc_ObjFanin(pNode,1), nBddSizeMax, fDropInternal, pProgress, pCounter, fVerbose ); 
            if ( bFunc1 == NULL )
                return NULL;
            Cudd_Ref( bFunc1 );
            bFunc0 = Cudd_NotCond( bFunc0, (int)Abc_ObjFaninC0(pNode) );
            bFunc1 = Cudd_NotCond( bFunc1, (int)Abc_ObjFaninC1(pNode) );
            // get the final result
            bFunc = Cudd_bddAndLimit( dd, bFunc0, bFunc1, nBddSizeMax );
            if ( bFunc == NULL )
                return NULL;
            Cudd_Ref( bFunc );
            Cudd_RecursiveDeref( dd, bFunc0 );
            Cudd_RecursiveDeref( dd, bFunc1 );
            // add the number of used nodes
            (*pCounter)++;
        }
        // set the result
        assert( Abc_ObjGlobalBdd(pNode) == NULL );
        Abc_ObjSetGlobalBdd( pNode, bFunc );
        // increment the progress bar
        if ( pProgress )
            Extra_ProgressBarUpdate( pProgress, *pCounter, NULL );
    }
    // prepare the return value
    bFunc = (DdNode *)Abc_ObjGlobalBdd(pNode);
    // dereference BDD at the node
    if ( --pNode->vFanouts.nSize == 0 && fDropInternal )
    {
        Cudd_Deref( bFunc );
        Abc_ObjSetGlobalBdd( pNode, NULL );
    }
    return bFunc;
}

/**Function*************************************************************

  Synopsis    [Frees the global BDDs of the network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abc_NtkFreeGlobalBdds( Abc_Ntk_t * pNtk, int fFreeMan ) 
{ 
    return Abc_NtkAttrFree( pNtk, VEC_ATTR_GLOBAL_BDD, fFreeMan ); 
}

/**Function*************************************************************

  Synopsis    [Returns the shared size of global BDDs of the COs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkSizeOfGlobalBdds( Abc_Ntk_t * pNtk ) 
{
    Vec_Ptr_t * vFuncsGlob;
    Abc_Obj_t * pObj;
    int RetValue, i;
    // complement the global functions
    vFuncsGlob = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_PtrPush( vFuncsGlob, Abc_ObjGlobalBdd(pObj) );
    RetValue = Cudd_SharingSize( (DdNode **)Vec_PtrArray(vFuncsGlob), Vec_PtrSize(vFuncsGlob) );
    Vec_PtrFree( vFuncsGlob );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Computes the BDD of the logic cone of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Abc_NtkSpacePercentage( Abc_Obj_t * pNode )
{
    /*
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pNodeR;
    DdManager * dd;
    DdNode * bFunc;
    double Result;
    int i;
    pNodeR = Abc_ObjRegular(pNode);
    assert( Abc_NtkIsStrash(pNodeR->pNtk) );
    Abc_NtkCleanCopy( pNodeR->pNtk );
    // get the CIs in the support of the node
    vNodes = Abc_NtkNodeSupport( pNodeR->pNtk, &pNodeR, 1 );
    // start the manager
    dd = Cudd_Init( Vec_PtrSize(vNodes), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );
    // assign elementary BDDs for the CIs
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)dd->vars[i];
    // build the BDD of the cone
    bFunc = Abc_NodeGlobalBdds_rec( dd, pNodeR, 10000000, 1, NULL, NULL, 1 );  Cudd_Ref( bFunc );
    bFunc = Cudd_NotCond( bFunc, pNode != pNodeR );
    // count minterms
    Result = Cudd_CountMinterm( dd, bFunc, dd->size );
    // get the percentagle
    Result *= 100.0;
    for ( i = 0; i < dd->size; i++ )
        Result /= 2;
    // clean up
    Cudd_Quit( dd );
    Vec_PtrFree( vNodes );
    return Result;
    */
    return 0.0;
}




/**Function*************************************************************

  Synopsis    [Experiment with BDD-based representation of implications.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkBddImplicationTest()
{
    DdManager * dd;
    DdNode * bImp, * bSum, * bTemp;
    int nVars = 200;
    int nImps = 200;
    int i;
    abctime clk;
clk = Abc_Clock();
    dd = Cudd_Init( nVars, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_AutodynEnable( dd, CUDD_REORDER_SIFT );
    bSum = b0;   Cudd_Ref( bSum );
    for ( i = 0; i < nImps; i++ )
    {
        printf( "." );
        bImp = Cudd_bddAnd( dd, dd->vars[rand()%nVars], dd->vars[rand()%nVars] );  Cudd_Ref( bImp );
        bSum = Cudd_bddOr( dd, bTemp = bSum, bImp );     Cudd_Ref( bSum );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bImp );
    }
    printf( "The BDD before = %d.\n", Cudd_DagSize(bSum) );
    Cudd_ReduceHeap( dd, CUDD_REORDER_SIFT, 1 );
    printf( "The BDD after  = %d.\n", Cudd_DagSize(bSum) );
ABC_PRT( "Time", Abc_Clock() - clk );
    Cudd_RecursiveDeref( dd, bSum );
    Cudd_Quit( dd );
}

#else

double Abc_NtkSpacePercentage( Abc_Obj_t * pNode ) { return 0.0; }
Abc_Ntk_t * Abc_NtkBddToMuxes( Abc_Ntk_t * pNtk, int fGlobal, int Limit, int fUseAdd ) { return NULL; }


#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

