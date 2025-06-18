/**CFile****************************************************************

  FileName    [llb2Cluster.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Non-linear quantification scheduling.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb2Cluster.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"

ABC_NAMESPACE_IMPL_START
 

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Find good static variable ordering.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4FindOrder_rec( Aig_Man_t * pAig, Aig_Obj_t * pObj, Vec_Int_t * vOrder, int * pCounter )
{
    Aig_Obj_t * pFanin0, * pFanin1;
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent( pAig, pObj );
    assert( Llb_ObjBddVar(vOrder, pObj) < 0 );
    if ( Aig_ObjIsCi(pObj) )
    {
        Vec_IntWriteEntry( vOrder, Aig_ObjId(pObj), (*pCounter)++ );
        return;
    }
    // try fanins with higher level first
    pFanin0 = Aig_ObjFanin0(pObj);
    pFanin1 = Aig_ObjFanin1(pObj);
//    if ( pFanin0->Level > pFanin1->Level || (pFanin0->Level == pFanin1->Level && pFanin0->Id < pFanin1->Id) )
    if ( pFanin0->Level > pFanin1->Level )
    {
        Llb_Nonlin4FindOrder_rec( pAig, pFanin0, vOrder, pCounter );
        Llb_Nonlin4FindOrder_rec( pAig, pFanin1, vOrder, pCounter );
    }
    else
    {
        Llb_Nonlin4FindOrder_rec( pAig, pFanin1, vOrder, pCounter );
        Llb_Nonlin4FindOrder_rec( pAig, pFanin0, vOrder, pCounter );
    }
    if ( pObj->fMarkA )
        Vec_IntWriteEntry( vOrder, Aig_ObjId(pObj), (*pCounter)++ );
}

/**Function*************************************************************

  Synopsis    [Find good static variable ordering.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Llb_Nonlin4FindOrder( Aig_Man_t * pAig, int * pCounter )
{
    Vec_Int_t * vNodes = NULL;
    Vec_Int_t * vOrder;
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    // mark nodes to exclude:  AND with low level and CO drivers
    Aig_ManCleanMarkA( pAig );
    Aig_ManForEachNode( pAig, pObj, i )
        if ( Aig_ObjLevel(pObj) > 3 )
            pObj->fMarkA = 1;
    Aig_ManForEachCo( pAig, pObj, i )
        Aig_ObjFanin0(pObj)->fMarkA = 0;

    // collect nodes in the order
    vOrder = Vec_IntStartFull( Aig_ManObjNumMax(pAig) );
    Aig_ManIncrementTravId( pAig );
    Aig_ObjSetTravIdCurrent( pAig, Aig_ManConst1(pAig) );
//    Aig_ManForEachCo( pAig, pObj, i )
    Saig_ManForEachLi( pAig, pObj, i )
    {
        Vec_IntWriteEntry( vOrder, Aig_ObjId(pObj), Counter++ );
        Llb_Nonlin4FindOrder_rec( pAig, Aig_ObjFanin0(pObj), vOrder, &Counter );
    }
    Aig_ManForEachCi( pAig, pObj, i )
        if ( Llb_ObjBddVar(vOrder, pObj) < 0 )
            Vec_IntWriteEntry( vOrder, Aig_ObjId(pObj), Counter++ );
    Aig_ManCleanMarkA( pAig );
    Vec_IntFreeP( &vNodes );
//    assert( Counter == Aig_ManObjNum(pAig) - 1 );

/*
    Saig_ManForEachPi( pAig, pObj, i )
        printf( "pi%d ", Llb_ObjBddVar(vOrder, pObj) );
    printf( "\n" );
    Saig_ManForEachLo( pAig, pObj, i )
        printf( "lo%d ", Llb_ObjBddVar(vOrder, pObj) );
    printf( "\n" );
    Saig_ManForEachPo( pAig, pObj, i )
        printf( "po%d ", Llb_ObjBddVar(vOrder, pObj) );
    printf( "\n" );
    Saig_ManForEachLi( pAig, pObj, i )
        printf( "li%d ", Llb_ObjBddVar(vOrder, pObj) );
    printf( "\n" );
    Aig_ManForEachNode( pAig, pObj, i )
        printf( "n%d ", Llb_ObjBddVar(vOrder, pObj) );
    printf( "\n" );
*/
    if ( pCounter )
        *pCounter = Counter;
    return vOrder;
}

/**Function*************************************************************

  Synopsis    [Derives BDDs for the partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_Nonlin4FindPartitions_rec( DdManager * dd, Aig_Obj_t * pObj, Vec_Int_t * vOrder, Vec_Ptr_t * vRoots )
{
    DdNode * bBdd, * bBdd0, * bBdd1, * bPart, * vVar;
    if ( Aig_ObjIsConst1(pObj) )
        return Cudd_ReadOne(dd); 
    if ( Aig_ObjIsCi(pObj) )
        return Cudd_bddIthVar( dd, Llb_ObjBddVar(vOrder, pObj) );
    if ( pObj->pData )
        return (DdNode *)pObj->pData;
    if ( Aig_ObjIsCo(pObj) )
    {
        bBdd0 = Cudd_NotCond( Llb_Nonlin4FindPartitions_rec(dd, Aig_ObjFanin0(pObj), vOrder, vRoots), Aig_ObjFaninC0(pObj) );
        bPart = Cudd_bddXnor( dd, Cudd_bddIthVar( dd, Llb_ObjBddVar(vOrder, pObj) ), bBdd0 );  Cudd_Ref( bPart );
        Vec_PtrPush( vRoots, bPart );
        return NULL;
    }
    bBdd0 = Cudd_NotCond( Llb_Nonlin4FindPartitions_rec(dd, Aig_ObjFanin0(pObj), vOrder, vRoots), Aig_ObjFaninC0(pObj) );
    bBdd1 = Cudd_NotCond( Llb_Nonlin4FindPartitions_rec(dd, Aig_ObjFanin1(pObj), vOrder, vRoots), Aig_ObjFaninC1(pObj) );
    bBdd  = Cudd_bddAnd( dd, bBdd0, bBdd1 );   Cudd_Ref( bBdd );
    if ( Llb_ObjBddVar(vOrder, pObj) >= 0 )
    {
        vVar  = Cudd_bddIthVar( dd, Llb_ObjBddVar(vOrder, pObj) );
        bPart = Cudd_bddXnor( dd, vVar, bBdd );  Cudd_Ref( bPart );
        Vec_PtrPush( vRoots, bPart );
        Cudd_RecursiveDeref( dd, bBdd );
        bBdd = vVar;  Cudd_Ref( vVar );
    }
    pObj->pData = bBdd;
    return bBdd;
}

/**Function*************************************************************

  Synopsis    [Derives BDDs for the partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_Nonlin4FindPartitions( DdManager * dd, Aig_Man_t * pAig, Vec_Int_t * vOrder, int fOutputs )
{
    Vec_Ptr_t * vRoots;
    Aig_Obj_t * pObj;
    int i;
    Aig_ManCleanData( pAig );
    vRoots = Vec_PtrAlloc( 100 );
    if ( fOutputs )
    {
        Saig_ManForEachPo( pAig, pObj, i )
            Llb_Nonlin4FindPartitions_rec( dd, pObj, vOrder, vRoots );
    }
    else
    {
        Saig_ManForEachLi( pAig, pObj, i )
            Llb_Nonlin4FindPartitions_rec( dd, pObj, vOrder, vRoots );
    }
    Aig_ManForEachNode( pAig, pObj, i )
        if ( pObj->pData )
            Cudd_RecursiveDeref( dd, (DdNode *)pObj->pData );
    return vRoots;
}

/**Function*************************************************************

  Synopsis    [Creates quantifiable variables for both types of traversal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Llb_Nonlin4FindVars2Q( DdManager * dd, Aig_Man_t * pAig, Vec_Int_t * vOrder )
{
    Vec_Int_t * vVars2Q;
    Aig_Obj_t * pObj;
    int i;
    vVars2Q = Vec_IntAlloc( 0 );
    Vec_IntFill( vVars2Q, Cudd_ReadSize(dd), 1 );
    Saig_ManForEachLo( pAig, pObj, i )
        Vec_IntWriteEntry( vVars2Q, Llb_ObjBddVar(vOrder, pObj), 0 );
//    Aig_ManForEachCo( pAig, pObj, i )
    Saig_ManForEachLi( pAig, pObj, i )
        Vec_IntWriteEntry( vVars2Q, Llb_ObjBddVar(vOrder, pObj), 0 );
    return vVars2Q;
}

/**Function*************************************************************

  Synopsis    [Creates quantifiable variables for both types of traversal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_Nonlin4CountTerms( DdManager * dd, Aig_Man_t * pAig, Vec_Int_t * vOrder, DdNode * bFunc, int fCo, int fFlop )
{
    DdNode * bSupp;
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    bSupp = Cudd_Support( dd, bFunc );  Cudd_Ref( bSupp );
    if ( !fCo && !fFlop )
    {
        Saig_ManForEachPi( pAig, pObj, i )
            if ( Llb_ObjBddVar(vOrder, pObj) >= 0 )
                Counter += Cudd_bddLeq( dd, bSupp, Cudd_bddIthVar(dd, Llb_ObjBddVar(vOrder, pObj)) );
    }
    else if ( fCo && !fFlop )
    {
        Saig_ManForEachPo( pAig, pObj, i )
            if ( Llb_ObjBddVar(vOrder, pObj) >= 0 )
                Counter += Cudd_bddLeq( dd, bSupp, Cudd_bddIthVar(dd, Llb_ObjBddVar(vOrder, pObj)) );
    }
    else if ( !fCo && fFlop )
    {
        Saig_ManForEachLo( pAig, pObj, i )
            if ( Llb_ObjBddVar(vOrder, pObj) >= 0 )
                Counter += Cudd_bddLeq( dd, bSupp, Cudd_bddIthVar(dd, Llb_ObjBddVar(vOrder, pObj)) );
    }
    else if ( fCo && fFlop )
    {
        Saig_ManForEachLi( pAig, pObj, i )
            if ( Llb_ObjBddVar(vOrder, pObj) >= 0 )
                Counter += Cudd_bddLeq( dd, bSupp, Cudd_bddIthVar(dd, Llb_ObjBddVar(vOrder, pObj)) );
    }
    Cudd_RecursiveDeref( dd, bSupp );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Creates quantifiable variables for both types of traversal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4PrintGroups( DdManager * dd, Aig_Man_t * pAig, Vec_Int_t * vOrder, Vec_Ptr_t * vGroups )
{
    DdNode * bTemp;
    int i, nSuppAll, nSuppPi, nSuppPo, nSuppLi, nSuppLo, nSuppAnd;
    Vec_PtrForEachEntry( DdNode *, vGroups, bTemp, i )
    {
//Extra_bddPrintSupport(dd, bTemp);  printf("\n" );
        nSuppAll = Cudd_SupportSize(dd,bTemp);
        nSuppPi  = Llb_Nonlin4CountTerms(dd, pAig, vOrder, bTemp, 0, 0);
        nSuppPo  = Llb_Nonlin4CountTerms(dd, pAig, vOrder, bTemp, 1, 0);
        nSuppLi  = Llb_Nonlin4CountTerms(dd, pAig, vOrder, bTemp, 0, 1);
        nSuppLo  = Llb_Nonlin4CountTerms(dd, pAig, vOrder, bTemp, 1, 1);
        nSuppAnd = nSuppAll - (nSuppPi+nSuppPo+nSuppLi+nSuppLo);

        if ( Cudd_DagSize(bTemp) <= 10 )
            continue;

        printf( "%4d : bdd =%6d  supp =%3d  ", i, Cudd_DagSize(bTemp), nSuppAll );
        printf( "pi =%3d ",  nSuppPi );
        printf( "po =%3d ",  nSuppPo );
        printf( "lo =%3d ",  nSuppLo );
        printf( "li =%3d ",  nSuppLi );
        printf( "and =%3d",  nSuppAnd );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Creates quantifiable variables for both types of traversal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4PrintSuppProfile( DdManager * dd, Aig_Man_t * pAig, Vec_Int_t * vOrder, Vec_Ptr_t * vGroups )
{
    Aig_Obj_t * pObj;
    int i, * pSupp;
    int nSuppAll = 0, nSuppPi = 0, nSuppPo = 0, nSuppLi = 0, nSuppLo = 0, nSuppAnd = 0;

    pSupp = ABC_CALLOC( int, Cudd_ReadSize(dd) );
    Extra_VectorSupportArray( dd, (DdNode **)Vec_PtrArray(vGroups), Vec_PtrSize(vGroups), pSupp );

    Aig_ManForEachObj( pAig, pObj, i )
    {
        if ( Llb_ObjBddVar(vOrder, pObj) < 0 )
            continue;
        // remove variables that do not participate
        if ( pSupp[Llb_ObjBddVar(vOrder, pObj)] == 0 )
        {
            if ( Aig_ObjIsNode(pObj) )
                Vec_IntWriteEntry( vOrder, Aig_ObjId(pObj), -1 );
            continue;
        }
        nSuppAll++;
        if ( Saig_ObjIsPi(pAig, pObj) )
            nSuppPi++;
        else if ( Saig_ObjIsLo(pAig, pObj) )
            nSuppLo++;
        else if ( Saig_ObjIsPo(pAig, pObj) )
            nSuppPo++;
        else if ( Saig_ObjIsLi(pAig, pObj) )
            nSuppLi++;
        else
            nSuppAnd++;
    }
    ABC_FREE( pSupp );

    printf( "Groups =%3d  ", Vec_PtrSize(vGroups) );
    printf( "Variables: all =%4d ",  nSuppAll );
    printf( "pi =%4d ",  nSuppPi );
    printf( "po =%4d ",  nSuppPo );
    printf( "lo =%4d ",  nSuppLo );
    printf( "li =%4d ",  nSuppLi );
    printf( "and =%4d",  nSuppAnd );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4Cluster( Aig_Man_t * pAig, DdManager ** pdd, Vec_Int_t ** pvOrder, Vec_Ptr_t ** pvGroups, int nBddMax, int fVerbose )
{
    DdManager * dd;
    Vec_Int_t * vOrder, * vVars2Q;
    Vec_Ptr_t * vParts, * vGroups;
    DdNode * bTemp;
    int i, nVarNum;

    // create the BDD manager
    vOrder  = Llb_Nonlin4FindOrder( pAig, &nVarNum );
    dd      = Cudd_Init( nVarNum, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
//    Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );

    vVars2Q = Llb_Nonlin4FindVars2Q( dd, pAig, vOrder );
    vParts  = Llb_Nonlin4FindPartitions( dd, pAig, vOrder, 0 );

    vGroups = Llb_Nonlin4Group( dd, vParts, vVars2Q, nBddMax );
    Vec_IntFree( vVars2Q );

    Vec_PtrForEachEntry( DdNode *, vParts, bTemp, i )
        Cudd_RecursiveDeref( dd, bTemp );
    Vec_PtrFree( vParts );


//    if ( fVerbose )
    Llb_Nonlin4PrintSuppProfile( dd, pAig, vOrder, vGroups );
    if ( fVerbose )
    printf( "Before reordering\n" );
    if ( fVerbose )
    Llb_Nonlin4PrintGroups( dd, pAig, vOrder, vGroups );

//    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 1 );
//    printf( "After reordering\n" );
//    Llb_Nonlin4PrintGroups( dd, pAig, vOrder, vGroups );

    if ( pvOrder )
        *pvOrder = vOrder;
    else
        Vec_IntFree( vOrder );

    if ( pvGroups )
        *pvGroups = vGroups;
    else
    {
        Vec_PtrForEachEntry( DdNode *, vGroups, bTemp, i )
            Cudd_RecursiveDeref( dd, bTemp );
        Vec_PtrFree( vGroups );
    }

    if ( pdd )
        *pdd = dd;
    else
        Extra_StopManager( dd );
//    Cudd_Quit( dd );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

