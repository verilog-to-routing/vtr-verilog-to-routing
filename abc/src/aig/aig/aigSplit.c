/**CFile****************************************************************

  FileName    [aigSplit.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Splits the property output cone into a set of cofactor properties.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigSplit.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"
#include "aig/saig/saig.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#ifdef ABC_USE_CUDD

/**Function*************************************************************

  Synopsis    [Converts the node to MUXes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_NodeBddToMuxes_rec( DdManager * dd, DdNode * bFunc, Aig_Man_t * pNew, st__table * tBdd2Node )
{
    Aig_Obj_t * pNode, * pNode0, * pNode1, * pNodeC;
    assert( !Cudd_IsComplement(bFunc) );
    if ( st__lookup( tBdd2Node, (char *)bFunc, (char **)&pNode ) )
        return pNode;
    // solve for the children nodes
    pNode0 = Aig_NodeBddToMuxes_rec( dd, Cudd_Regular(cuddE(bFunc)), pNew, tBdd2Node );
    pNode0 = Aig_NotCond( pNode0, Cudd_IsComplement(cuddE(bFunc)) );
    pNode1 = Aig_NodeBddToMuxes_rec( dd, cuddT(bFunc), pNew, tBdd2Node );
    if ( ! st__lookup( tBdd2Node, (char *)Cudd_bddIthVar(dd, bFunc->index), (char **)&pNodeC ) )
        assert( 0 );
    // create the MUX node
    pNode = Aig_Mux( pNew, pNodeC, pNode1, pNode0 );
    st__insert( tBdd2Node, (char *)bFunc, (char *)pNode );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Derives AIG for the BDDs of the cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManConvertBddsToAigs( Aig_Man_t * p, DdManager * dd, Vec_Ptr_t * vCofs )
{
    DdNode * bFunc;
    st__table * tBdd2Node;
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    int i;
    Aig_ManCleanData( p );
    // generate AIG for BDD
    pNew = Aig_ManStart( Aig_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    Aig_ManForEachCi( p, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    // create the table mapping BDD nodes into the ABC nodes
    tBdd2Node = st__init_table( st__ptrcmp, st__ptrhash );
    // add the constant and the elementary vars
    st__insert( tBdd2Node, (char *)Cudd_ReadOne(dd), (char *)Aig_ManConst1(pNew) );
    Aig_ManForEachCi( p, pObj, i )
        st__insert( tBdd2Node, (char *)Cudd_bddIthVar(dd, i), (char *)pObj->pData );
    // build primary outputs for the cofactors
    Vec_PtrForEachEntry( DdNode *, vCofs, bFunc, i )
    {
        if ( bFunc == Cudd_ReadLogicZero(dd) )
            continue;
        pObj = Aig_NodeBddToMuxes_rec( dd, Cudd_Regular(bFunc), pNew, tBdd2Node );
        pObj = Aig_NotCond( pObj, Cudd_IsComplement(bFunc) );
        Aig_ObjCreateCo( pNew, pObj );
    }
    st__free_table( tBdd2Node );

    // duplicate the rest of the AIG
    // add the POs
    Aig_ManForEachCo( p, pObj, i )
    {
        if ( i == 0 )
            continue;
        Aig_ManDupSimpleDfs_rec( pNew, p, Aig_ObjFanin0(pObj) );
        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    }
    Aig_ManCleanup( pNew );
    Aig_ManSetRegNum( pNew, Aig_ManRegNum(p) );
    // check the resulting network
    if ( !Aig_ManCheck(pNew) )
        printf( "Aig_ManConvertBddsToAigs(): The check has failed.\n" );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Returns the array of constraint candidates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Aig_ManBuildPoBdd_rec( Aig_Man_t * p, Aig_Obj_t * pObj, DdManager * dd )
{
    DdNode * bBdd0, * bBdd1;
    if ( pObj->pData != NULL )
        return (DdNode *)pObj->pData;
    assert( Aig_ObjIsNode(pObj) );
    bBdd0 = Aig_ManBuildPoBdd_rec( p, Aig_ObjFanin0(pObj), dd ); 
    bBdd1 = Aig_ManBuildPoBdd_rec( p, Aig_ObjFanin1(pObj), dd ); 
    bBdd0 = Cudd_NotCond( bBdd0, Aig_ObjFaninC0(pObj) );
    bBdd1 = Cudd_NotCond( bBdd1, Aig_ObjFaninC1(pObj) );
    pObj->pData = Cudd_bddAnd( dd, bBdd0, bBdd1 );  Cudd_Ref( (DdNode *)pObj->pData );
    return (DdNode *)pObj->pData;
}

/**Function*************************************************************

  Synopsis    [Derive BDDs for the cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManCofactorBdds( Aig_Man_t * p, Vec_Ptr_t * vSubset, DdManager * dd, DdNode * bFunc )
{
    Vec_Ptr_t * vCofs;
    DdNode * bCube, * bTemp, * bCof, ** pbVars;
    int i;
    vCofs = Vec_PtrAlloc( 100 );
    pbVars = (DdNode **)Vec_PtrArray(vSubset);
    for ( i = 0; i < (1 << Vec_PtrSize(vSubset)); i++ )
    {
        bCube = Extra_bddBitsToCube( dd, i, Vec_PtrSize(vSubset), pbVars, 1 );  Cudd_Ref( bCube );
        bCof  = Cudd_Cofactor( dd, bFunc, bCube );        Cudd_Ref( bCof );
        bCof  = Cudd_bddAnd( dd, bTemp = bCof, bCube );   Cudd_Ref( bCof );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
        Vec_PtrPush( vCofs, bCof );
    }
    return vCofs;
}

/**Function*************************************************************

  Synopsis    [Construct BDDs for the primary output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Aig_ManBuildPoBdd( Aig_Man_t * p, DdNode ** pbFunc )
{
    DdManager * dd;
    Aig_Obj_t * pObj;
    int i;
    assert( Saig_ManPoNum(p) == 1 );
    Aig_ManCleanData( p );
    dd = Cudd_Init( Aig_ManCiNum(p), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );
    pObj = Aig_ManConst1(p);
    pObj->pData = Cudd_ReadOne(dd);  Cudd_Ref( (DdNode *)pObj->pData );
    Aig_ManForEachCi( p, pObj, i )
    {
        pObj->pData = Cudd_bddIthVar(dd, i);  Cudd_Ref( (DdNode *)pObj->pData );
    }
    pObj = Aig_ManCo( p, 0 );
    *pbFunc = Aig_ManBuildPoBdd_rec( p, Aig_ObjFanin0(pObj), dd );  Cudd_Ref( *pbFunc );
    *pbFunc = Cudd_NotCond( *pbFunc, Aig_ObjFaninC0(pObj) );
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( pObj->pData )
            Cudd_RecursiveDeref( dd, (DdNode *)pObj->pData );
    }
    Cudd_ReduceHeap( dd,  CUDD_REORDER_SYMM_SIFT, 1 );
    return dd;
}

/**Function*************************************************************

  Synopsis    [Randomly selects a random subset of inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManVecRandSubset( Vec_Ptr_t * vVec, int nVars )
{
    Vec_Ptr_t * vRes;
    void * pEntry;
    unsigned Rand; 
    vRes = Vec_PtrDup(vVec);
    while ( Vec_PtrSize(vRes) > nVars )
    {
        Rand   = Aig_ManRandom( 0 );
        pEntry = Vec_PtrEntry( vRes, Rand % Vec_PtrSize(vRes) );
        Vec_PtrRemove( vRes, pEntry );
    }
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManSplit( Aig_Man_t * p, int nVars, int fVerbose )
{
    Aig_Man_t * pRes;
    Aig_Obj_t * pNode;
    DdNode * bFunc;
    DdManager * dd;
    Vec_Ptr_t * vSupp, * vSubs, * vCofs;
    int i;
    abctime clk = Abc_Clock();
    if ( Saig_ManPoNum(p) != 1 )
    {
        printf( "Currently works only for one primary output.\n" );
        return NULL;
    }
    if ( nVars < 1 )
    {
        printf( "The number of cofactoring variables should be a positive number.\n" );
        return NULL;
    }
    if ( nVars > 16 )
    {
        printf( "The number of cofactoring variables should be less than 17.\n" );
        return NULL;
    }
    vSupp = Aig_Support( p, Aig_ObjFanin0(Aig_ManCo(p,0)) );
    if ( Vec_PtrSize(vSupp) == 0 )
    {
        printf( "Property output function is a constant.\n" );
        Vec_PtrFree( vSupp );
        return NULL;
    }
    dd    = Aig_ManBuildPoBdd( p, &bFunc ); // bFunc is referenced
    if ( fVerbose )
        printf( "Support =%5d.  BDD size =%6d.  ", Vec_PtrSize(vSupp), Cudd_DagSize(bFunc) );
    vSubs = Aig_ManVecRandSubset( vSupp, nVars );
    // replace nodes by their BDD variables
    Vec_PtrForEachEntry( Aig_Obj_t *, vSubs, pNode, i )
        Vec_PtrWriteEntry( vSubs, i, pNode->pData );
    // derive cofactors and functions
    vCofs = Aig_ManCofactorBdds( p, vSubs, dd, bFunc );
    pRes  = Aig_ManConvertBddsToAigs( p, dd, vCofs );
    Vec_PtrFree( vSupp );
    Vec_PtrFree( vSubs );
    if ( fVerbose )
        printf( "Created %d cofactors (out of %d).  ", Saig_ManPoNum(pRes), Vec_PtrSize(vCofs) );
    if ( fVerbose )
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    // dereference
    Cudd_RecursiveDeref( dd, bFunc );
    Vec_PtrForEachEntry( DdNode *, vCofs, bFunc, i )
        Cudd_RecursiveDeref( dd, bFunc );
    Vec_PtrFree( vCofs );
    Extra_StopManager( dd );
    return pRes;
}

#else

Aig_Man_t * Aig_ManSplit( Aig_Man_t * p, int nVars, int fVerbose )
{
    return NULL;
}

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

