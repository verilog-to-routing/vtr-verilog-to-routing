/**CFile****************************************************************

  FileName    [llb2Sweep.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Non-linear quantification scheduling.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb2Sweep.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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
void Llb_Nonlin4SweepOrder_rec( Aig_Man_t * pAig, Aig_Obj_t * pObj, Vec_Int_t * vOrder, int * pCounter, int fSaveAll )
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
        Llb_Nonlin4SweepOrder_rec( pAig, pFanin0, vOrder, pCounter, fSaveAll );
        Llb_Nonlin4SweepOrder_rec( pAig, pFanin1, vOrder, pCounter, fSaveAll );
    }
    else
    {
        Llb_Nonlin4SweepOrder_rec( pAig, pFanin1, vOrder, pCounter, fSaveAll );
        Llb_Nonlin4SweepOrder_rec( pAig, pFanin0, vOrder, pCounter, fSaveAll );
    }
    if ( fSaveAll || pObj->fMarkA )
        Vec_IntWriteEntry( vOrder, Aig_ObjId(pObj), (*pCounter)++ );
}

/**Function*************************************************************

  Synopsis    [Find good static variable ordering.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Llb_Nonlin4SweepOrder( Aig_Man_t * pAig, int * pCounter, int fSaveAll )
{
    Vec_Int_t * vOrder;
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    // collect nodes in the order
    vOrder = Vec_IntStartFull( Aig_ManObjNumMax(pAig) );
    Aig_ManIncrementTravId( pAig );
    Aig_ObjSetTravIdCurrent( pAig, Aig_ManConst1(pAig) );
    Aig_ManForEachCo( pAig, pObj, i )
    {
        Vec_IntWriteEntry( vOrder, Aig_ObjId(pObj), Counter++ );
        Llb_Nonlin4SweepOrder_rec( pAig, Aig_ObjFanin0(pObj), vOrder, &Counter, fSaveAll );
    }
    Aig_ManForEachCi( pAig, pObj, i )
        if ( Llb_ObjBddVar(vOrder, pObj) < 0 )
            Vec_IntWriteEntry( vOrder, Aig_ObjId(pObj), Counter++ );
//    assert( Counter == Aig_ManObjNum(pAig) - 1 ); // no dangling nodes
    if ( pCounter )
        *pCounter = Counter - Aig_ManCiNum(pAig) - Aig_ManCoNum(pAig);
    return vOrder;
}


/**Function*************************************************************

  Synopsis    [Performs BDD sweep on the netlist.]

  Description [Returns AIG with internal cut points labeled with fMarkA.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb4_Nonlin4SweepCutpoints( Aig_Man_t * pAig, Vec_Int_t * vOrder, int nBddLimit, int fVerbose )
{
    DdManager * dd;
    DdNode * bFunc0, * bFunc1, * bFunc;
    Aig_Obj_t * pObj;
    int i, Counter = 0, Counter1 = 0;
    dd = Cudd_Init( Aig_ManObjNumMax(pAig), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // assign elementary variables
    Aig_ManCleanData( pAig );
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Cudd_bddIthVar( dd, Llb_ObjBddVar(vOrder, pObj) );
    // sweep internal nodes
    Aig_ManForEachNode( pAig, pObj, i )
    {
/*
        if ( pObj->nRefs >= 4 )
        {
            bFunc = Cudd_bddIthVar( dd, Llb_ObjBddVar(vOrder, pObj) );  Cudd_Ref( bFunc );
            pObj->pData = bFunc;
            Counter1++;
            continue;
        }
*/
        bFunc0 = Cudd_NotCond( (DdNode *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
        bFunc1 = Cudd_NotCond( (DdNode *)Aig_ObjFanin1(pObj)->pData, Aig_ObjFaninC1(pObj) );
        bFunc  = Cudd_bddAnd( dd, bFunc0, bFunc1 );  Cudd_Ref( bFunc );
        if ( Cudd_DagSize(bFunc) > nBddLimit )
        {
//            if ( fVerbose )
//                printf( "Node %5d : Beg =%5d. ", i, Cudd_DagSize(bFunc) );

            // add cutpoint at a larger one
            Cudd_RecursiveDeref( dd, bFunc );
            if ( Cudd_DagSize(bFunc0) >= Cudd_DagSize(bFunc1) )
            {
                Cudd_RecursiveDeref( dd, (DdNode *)Aig_ObjFanin0(pObj)->pData );
                bFunc = Cudd_bddIthVar( dd, Llb_ObjBddVar(vOrder, Aig_ObjFanin0(pObj)) );
                Aig_ObjFanin0(pObj)->pData = bFunc;  Cudd_Ref( bFunc );
                Aig_ObjFanin0(pObj)->fMarkA = 1;

//                if ( fVerbose )
//                    printf( "Ref =%3d  ", Aig_ObjFanin0(pObj)->nRefs );
            }
            else
            {
                Cudd_RecursiveDeref( dd, (DdNode *)Aig_ObjFanin1(pObj)->pData );
                bFunc = Cudd_bddIthVar( dd, Llb_ObjBddVar(vOrder, Aig_ObjFanin1(pObj)) );
                Aig_ObjFanin1(pObj)->pData = bFunc;  Cudd_Ref( bFunc );
                Aig_ObjFanin1(pObj)->fMarkA = 1;

//                if ( fVerbose )
//                    printf( "Ref =%3d  ", Aig_ObjFanin1(pObj)->nRefs );
            }
            // perform new operation
            bFunc0 = Cudd_NotCond( (DdNode *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
            bFunc1 = Cudd_NotCond( (DdNode *)Aig_ObjFanin1(pObj)->pData, Aig_ObjFaninC1(pObj) );
            bFunc  = Cudd_bddAnd( dd, bFunc0, bFunc1 );  Cudd_Ref( bFunc );
//            assert( Cudd_DagSize(bFunc) <= nBddLimit );

//            if ( fVerbose )
//                printf( "End =%5d.\n", Cudd_DagSize(bFunc) );
            Counter++;
        }
        pObj->pData = bFunc;
//printf( "%d ", Cudd_DagSize(bFunc) );
    }
//printf( "\n" );
    // clean up
    Aig_ManForEachNode( pAig, pObj, i )
        Cudd_RecursiveDeref( dd, (DdNode *)pObj->pData );
    Extra_StopManager( dd );
//    Aig_ManCleanMarkA( pAig );
    if ( fVerbose )
    printf( "Added %d cut points.  Used %d high fanout points.\n", Counter, Counter1 );
    return Counter + Counter1;
}

/**Function*************************************************************

  Synopsis    [Derives BDDs for the partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_Nonlin4SweepPartitions_rec( DdManager * dd, Aig_Obj_t * pObj, Vec_Int_t * vOrder, Vec_Ptr_t * vRoots )
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
        bBdd0 = Cudd_NotCond( Llb_Nonlin4SweepPartitions_rec(dd, Aig_ObjFanin0(pObj), vOrder, vRoots), Aig_ObjFaninC0(pObj) );
        bPart = Cudd_bddXnor( dd, Cudd_bddIthVar( dd, Llb_ObjBddVar(vOrder, pObj) ), bBdd0 );  Cudd_Ref( bPart );
        Vec_PtrPush( vRoots, bPart );
        return NULL;
    }
    bBdd0 = Cudd_NotCond( Llb_Nonlin4SweepPartitions_rec(dd, Aig_ObjFanin0(pObj), vOrder, vRoots), Aig_ObjFaninC0(pObj) );
    bBdd1 = Cudd_NotCond( Llb_Nonlin4SweepPartitions_rec(dd, Aig_ObjFanin1(pObj), vOrder, vRoots), Aig_ObjFaninC1(pObj) );
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
Vec_Ptr_t * Llb_Nonlin4SweepPartitions( DdManager * dd, Aig_Man_t * pAig, Vec_Int_t * vOrder, int fTransition )
{
    Vec_Ptr_t * vRoots;
    Aig_Obj_t * pObj;
    int i;
    Aig_ManCleanData( pAig );
    vRoots = Vec_PtrAlloc( 100 );
    if ( fTransition )
    {
        Saig_ManForEachLi( pAig, pObj, i )
            Llb_Nonlin4SweepPartitions_rec( dd, pObj, vOrder, vRoots );
    }
    else
    {
        Saig_ManForEachPo( pAig, pObj, i )
            Llb_Nonlin4SweepPartitions_rec( dd, pObj, vOrder, vRoots );
    }
    Aig_ManForEachNode( pAig, pObj, i )
        if ( pObj->pData )
            Cudd_RecursiveDeref( dd, (DdNode *)pObj->pData );
    return vRoots;
}

/**Function*************************************************************

  Synopsis    [Get bad state monitor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb4_Nonlin4SweepBadMonitor( Aig_Man_t * pAig, Vec_Int_t * vOrder, DdManager * dd )
{
    Aig_Obj_t * pObj;
    DdNode * bRes, * bVar, * bTemp;
    int i;
    abctime TimeStop;
    TimeStop = dd->TimeStop;  dd->TimeStop = 0;
    bRes = Cudd_ReadOne( dd );   Cudd_Ref( bRes );
    Saig_ManForEachPo( pAig, pObj, i )
    {
        bVar = Cudd_bddIthVar( dd, Llb_ObjBddVar(vOrder, pObj) );
        bRes = Cudd_bddAnd( dd, bTemp = bRes, Cudd_Not(bVar) );  Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bRes );
    dd->TimeStop = TimeStop;
    return Cudd_Not(bRes);
}

/**Function*************************************************************

  Synopsis    [Creates quantifiable variables for both types of traversal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Llb_Nonlin4SweepVars2Q( Aig_Man_t * pAig, Vec_Int_t * vOrder, int fAddLis )
{
    Vec_Int_t * vVars2Q;
    Aig_Obj_t * pObj;
    int i;
    vVars2Q = Vec_IntAlloc( 0 );
    Vec_IntFill( vVars2Q, Aig_ManObjNumMax(pAig), 1 );
    // add flop outputs
    Saig_ManForEachLo( pAig, pObj, i )
        Vec_IntWriteEntry( vVars2Q, Llb_ObjBddVar(vOrder, pObj), 0 );
    // add flop inputs
    if ( fAddLis )
    Saig_ManForEachLi( pAig, pObj, i )
        Vec_IntWriteEntry( vVars2Q, Llb_ObjBddVar(vOrder, pObj), 0 );
    return vVars2Q;
}

/**Function*************************************************************

  Synopsis    [Multiply every partition by the cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4SweepDeref( DdManager * dd, Vec_Ptr_t * vParts )
{
    DdNode * bFunc;
    int i;
    Vec_PtrForEachEntry( DdNode *, vParts, bFunc, i )
        Cudd_RecursiveDeref( dd, bFunc );
    Vec_PtrFree( vParts );
}

/**Function*************************************************************

  Synopsis    [Multiply every partition by the cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4SweepPrint( Vec_Ptr_t * vFuncs )
{
    DdNode * bFunc;
    int i;
    printf( "(%d) ", Vec_PtrSize(vFuncs) );
    Vec_PtrForEachEntry( DdNode *, vFuncs, bFunc, i )
        printf( "%d ", Cudd_DagSize(bFunc) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Computes bad states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Llb4_Nonlin4SweepBadStates( Aig_Man_t * pAig, Vec_Int_t * vOrder, int nVars )
{
    DdManager * dd;
    Vec_Ptr_t * vParts;
    Vec_Int_t * vVars2Q;
    DdNode * bMonitor, * bImage;
    // get quantifiable variables
    vVars2Q = Llb_Nonlin4SweepVars2Q( pAig, vOrder, 0 );
    // start BDD manager and create partitions 
    dd = Cudd_Init( nVars, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    vParts = Llb_Nonlin4SweepPartitions( dd, pAig, vOrder, 0 );
//printf( "Outputs: " );
//Llb_Nonlin4SweepPrint( vParts );
    // compute image of the partitions
    bMonitor = Llb4_Nonlin4SweepBadMonitor( pAig, vOrder, dd );  Cudd_Ref( bMonitor );
    Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );
    bImage = Llb_Nonlin4Image( dd, vParts, bMonitor, vVars2Q );  Cudd_Ref( bImage );
    Cudd_RecursiveDeref( dd, bMonitor );
    Llb_Nonlin4SweepDeref( dd, vParts );
    Vec_IntFree( vVars2Q );
    // save image and return
    dd->bFunc = bImage;
    return dd;
}

/**Function*************************************************************

  Synopsis    [Computes clusters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Llb4_Nonlin4SweepGroups( Aig_Man_t * pAig, Vec_Int_t * vOrder, int nVars, Vec_Ptr_t ** pvGroups, int nBddLimitClp, int fVerbose )
{
    DdManager * dd;
    Vec_Ptr_t * vParts;
    Vec_Int_t * vVars2Q;
    // get quantifiable variables
    vVars2Q = Llb_Nonlin4SweepVars2Q( pAig, vOrder, 1 );
    // start BDD manager and create partitions 
    dd = Cudd_Init( nVars, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    vParts = Llb_Nonlin4SweepPartitions( dd, pAig, vOrder, 1 );
//printf( "Transitions: " );
//Llb_Nonlin4SweepPrint( vParts );
    // compute image of the partitions

    Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );
    *pvGroups = Llb_Nonlin4Group( dd, vParts, vVars2Q, nBddLimitClp );
    Llb_Nonlin4SweepDeref( dd, vParts );
//    *pvGroups = vParts;

if ( fVerbose )
{
printf( "Groups: " );
Llb_Nonlin4SweepPrint( *pvGroups );
}

    Vec_IntFree( vVars2Q );
    return dd;
}


/**Function*************************************************************

  Synopsis    [Creates quantifiable variables for both types of traversal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_Nonlin4SweepPrintSuppProfile( DdManager * dd, Aig_Man_t * pAig, Vec_Int_t * vOrder, Vec_Ptr_t * vGroups, int fVerbose )
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

    if ( fVerbose )
    {
    printf( "Groups =%3d  ", Vec_PtrSize(vGroups) );
    printf( "Variables: all =%4d ",  nSuppAll );
    printf( "pi =%4d ",  nSuppPi );
    printf( "po =%4d ",  nSuppPo );
    printf( "lo =%4d ",  nSuppLo );
    printf( "li =%4d ",  nSuppLi );
    printf( "and =%4d",  nSuppAnd );
    printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Performs BDD sweep on the netlist.]

  Description [Returns BDD manager, ordering, clusters, and bad states 
  inside dd->bFunc.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb4_Nonlin4Sweep( Aig_Man_t * pAig, int nSweepMax, int nClusterMax, DdManager ** pdd, Vec_Int_t ** pvOrder, Vec_Ptr_t ** pvGroups, int fVerbose )
{
    DdManager * ddBad, * ddWork;
    Vec_Ptr_t * vGroups;
    Vec_Int_t * vOrder;
    int Counter, nCutPoints;

    // get the original ordering
    Aig_ManCleanMarkA( pAig );
    vOrder = Llb_Nonlin4SweepOrder( pAig, &Counter, 1 );
    assert( Counter == Aig_ManNodeNum(pAig) );
    // mark the nodes
    nCutPoints = Llb4_Nonlin4SweepCutpoints( pAig, vOrder, nSweepMax, fVerbose );
    Vec_IntFree( vOrder );
    // get better ordering
    vOrder = Llb_Nonlin4SweepOrder( pAig, &Counter, 0 );
    assert( Counter == nCutPoints );
    Aig_ManCleanMarkA( pAig );
    // compute the BAD states
    ddBad = Llb4_Nonlin4SweepBadStates( pAig, vOrder, nCutPoints + Aig_ManCiNum(pAig) + Aig_ManCoNum(pAig) );
    // compute the clusters
    ddWork = Llb4_Nonlin4SweepGroups( pAig, vOrder, nCutPoints + Aig_ManCiNum(pAig) + Aig_ManCoNum(pAig), &vGroups, nClusterMax, fVerbose );
    // transfer the result from the Bad manager
//printf( "Bad before = %d.\n", Cudd_DagSize(ddBad->bFunc) );
    ddWork->bFunc = Cudd_bddTransfer( ddBad, ddWork, ddBad->bFunc );   Cudd_Ref( ddWork->bFunc );
    Cudd_RecursiveDeref( ddBad, ddBad->bFunc );  ddBad->bFunc = NULL;
    Extra_StopManager( ddBad );
    // update ordering to exclude quantified variables
//printf( "Bad after = %d.\n", Cudd_DagSize(ddWork->bFunc) );

    Llb_Nonlin4SweepPrintSuppProfile( ddWork, pAig, vOrder, vGroups, fVerbose );

    // return the result
    *pdd = ddWork;
    *pvOrder = vOrder;
    *pvGroups = vGroups;
}

/**Function*************************************************************

  Synopsis    [Performs BDD sweep on the netlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb4_Nonlin4SweepExperiment( Aig_Man_t * pAig )
{
    DdManager * dd;
    Vec_Int_t * vOrder;
    Vec_Ptr_t * vGroups;
    Llb4_Nonlin4Sweep( pAig, 100, 500, &dd, &vOrder, &vGroups, 1 );

    Llb_Nonlin4SweepDeref( dd, vGroups );

    Cudd_RecursiveDeref( dd, dd->bFunc );
    Extra_StopManager( dd );
    Vec_IntFree( vOrder );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

