/**CFile****************************************************************

  FileName    [llb2Image.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Computes image using partitioned structure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb2Image.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Vec_Ptr_t * Llb_ManCutNodes( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper );
extern Vec_Ptr_t * Llb_ManCutRange( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes supports of the partitions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ImgSupports( Aig_Man_t * p, Vec_Ptr_t * vDdMans, Vec_Int_t * vStart, Vec_Int_t * vStop, int fAddPis, int fVerbose )
{
    Vec_Ptr_t * vSupps;
    Vec_Int_t * vOne;
    Aig_Obj_t * pObj;
    DdManager * dd;
    DdNode * bSupp, * bTemp;
    int i, Entry, nSize;
    nSize  = Cudd_ReadSize( (DdManager *)Vec_PtrEntry( vDdMans, 0 ) );
    vSupps = Vec_PtrAlloc( 100 );
    // create initial
    vOne = Vec_IntStart( nSize );
    Vec_IntForEachEntry( vStart, Entry, i )
        Vec_IntWriteEntry( vOne, Entry, 1 );
    Vec_PtrPush( vSupps, vOne );
    // create intermediate 
    Vec_PtrForEachEntry( DdManager *, vDdMans, dd, i )
    {
        vOne  = Vec_IntStart( nSize );
        bSupp = Cudd_Support( dd, dd->bFunc );  Cudd_Ref( bSupp );
        for ( bTemp = bSupp; bTemp != Cudd_ReadOne(dd); bTemp = cuddT(bTemp) )
            Vec_IntWriteEntry( vOne, bTemp->index, 1 );
        Cudd_RecursiveDeref( dd, bSupp );
        Vec_PtrPush( vSupps, vOne );
    }
    // create final
    vOne = Vec_IntStart( nSize );
    Vec_IntForEachEntry( vStop, Entry, i )
        Vec_IntWriteEntry( vOne, Entry, 1 );
    if ( fAddPis )
        Saig_ManForEachPi( p, pObj, i )
            Vec_IntWriteEntry( vOne, Aig_ObjId(pObj), 1 );
    Vec_PtrPush( vSupps, vOne );

    // print supports
    assert( nSize == Aig_ManObjNumMax(p) );
    if ( !fVerbose )
        return vSupps;
    Aig_ManForEachObj( p, pObj, i )
    {
        int k, Counter = 0;
        Vec_PtrForEachEntry( Vec_Int_t *, vSupps, vOne, k )
            Counter += Vec_IntEntry(vOne, i);
        if ( Counter == 0 ) 
            continue;
        printf( "Obj = %4d : ", i );
        if ( Saig_ObjIsPi(p,pObj) )
            printf( "pi  " );
        else if ( Saig_ObjIsLo(p,pObj) )
            printf( "lo  " );
        else if ( Saig_ObjIsLi(p,pObj) )
            printf( "li  " );
        else if ( Aig_ObjIsNode(pObj) )
            printf( "and " );
        Vec_PtrForEachEntry( Vec_Int_t *, vSupps, vOne, k )
            printf( "%d", Vec_IntEntry(vOne, i) );
        printf( "\n" );
    }
    return vSupps;
}

/**Function*************************************************************

  Synopsis    [Computes quantification schedule.]

  Description [Input array contains supports: 0=starting, ... intermediate...
  N-1=final. Output arrays contain immediately quantifiable vars (vQuant0)
  and vars that should be quantified after conjunction (vQuant1).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ImgSchedule( Vec_Ptr_t * vSupps, Vec_Ptr_t ** pvQuant0, Vec_Ptr_t ** pvQuant1, int fVerbose )
{
    Vec_Int_t * vOne;
    int nVarsAll, Counter, iSupp = -1, Entry, i, k;
    // start quantification arrays
    *pvQuant0 = Vec_PtrAlloc( Vec_PtrSize(vSupps) );
    *pvQuant1 = Vec_PtrAlloc( Vec_PtrSize(vSupps) );
    Vec_PtrForEachEntry( Vec_Int_t *, vSupps, vOne, k )
    {
        Vec_PtrPush( *pvQuant0, Vec_IntAlloc(16) );
        Vec_PtrPush( *pvQuant1, Vec_IntAlloc(16) );
    }
    // count how many times each var appears
    nVarsAll = Vec_IntSize( (Vec_Int_t *)Vec_PtrEntry(vSupps, 0) );
    for ( i = 0; i < nVarsAll; i++ )
    {
        Counter = 0;
        Vec_PtrForEachEntry( Vec_Int_t *, vSupps, vOne, k )
            if ( Vec_IntEntry(vOne, i) )
            {
                iSupp = k;
                Counter++;
            }
        if ( Counter == 0 )
            continue;
        if ( Counter == 1 )
            Vec_IntPush( (Vec_Int_t *)Vec_PtrEntry(*pvQuant0, iSupp), i );
        else // if ( Counter > 1 )
            Vec_IntPush( (Vec_Int_t *)Vec_PtrEntry(*pvQuant1, iSupp), i );
    }

    if ( fVerbose )
    for ( i = 0; i < Vec_PtrSize(vSupps); i++ )
    {
        printf( "%2d : Quant0 = ", i );
        Vec_IntForEachEntry( (Vec_Int_t *)Vec_PtrEntry(*pvQuant0, i), Entry, k )
            printf( "%d ", Entry );
        printf( "\n" );
    }

    if ( fVerbose )
    for ( i = 0; i < Vec_PtrSize(vSupps); i++ )
    {
        printf( "%2d : Quant1 = ", i );
        Vec_IntForEachEntry( (Vec_Int_t *)Vec_PtrEntry(*pvQuant1, i), Entry, k )
            printf( "%d ", Entry );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    [Computes one partition in a separate BDD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdManager * Llb_ImgPartition( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper, abctime TimeTarget )
{
    Vec_Ptr_t * vNodes, * vRange;
    Aig_Obj_t * pObj;
    DdManager * dd;
    DdNode * bBdd0, * bBdd1, * bProd, * bRes, * bTemp;
    int i;

    dd = Cudd_Init( Aig_ManObjNumMax(p), 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    Cudd_AutodynEnable( dd,  CUDD_REORDER_SYMM_SIFT );
    dd->TimeStop = TimeTarget;

    Vec_PtrForEachEntry( Aig_Obj_t *, vLower, pObj, i )
        pObj->pData = Cudd_bddIthVar( dd, Aig_ObjId(pObj) );

    vNodes = Llb_ManCutNodes( p, vLower, vUpper );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        bBdd0 = Cudd_NotCond( (DdNode *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj) );
        bBdd1 = Cudd_NotCond( (DdNode *)Aig_ObjFanin1(pObj)->pData, Aig_ObjFaninC1(pObj) );
//        pObj->pData = Cudd_bddAnd( dd, bBdd0, bBdd1 );   Cudd_Ref( (DdNode *)pObj->pData );
//        pObj->pData = Extra_bddAndTime( dd, bBdd0, bBdd1, TimeTarget );  
        pObj->pData = Cudd_bddAnd( dd, bBdd0, bBdd1 );  
        if ( pObj->pData == NULL )
        {
            Cudd_Quit( dd );
            Vec_PtrFree( vNodes );
            return NULL;
        }
        Cudd_Ref( (DdNode *)pObj->pData );
    }

    vRange = Llb_ManCutRange( p, vLower, vUpper );
    bRes   = Cudd_ReadOne(dd);   Cudd_Ref( bRes );
    Vec_PtrForEachEntry( Aig_Obj_t *, vRange, pObj, i )
    {
        assert( Aig_ObjIsNode(pObj) );
        bProd = Cudd_bddXnor( dd, Cudd_bddIthVar(dd, Aig_ObjId(pObj)), (DdNode *)pObj->pData );   Cudd_Ref( bProd );
//        bRes  = Cudd_bddAnd( dd, bTemp = bRes, bProd ); Cudd_Ref( bRes );
//        bRes  = Extra_bddAndTime( dd, bTemp = bRes, bProd, TimeTarget );  
        bRes  = Cudd_bddAnd( dd, bTemp = bRes, bProd );  
        if ( bRes == NULL )
        {
            Cudd_Quit( dd );
            Vec_PtrFree( vRange );
            Vec_PtrFree( vNodes );
            return NULL;
        }        
        Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bProd );
    }
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        Cudd_RecursiveDeref( dd, (DdNode *)pObj->pData );

    Vec_PtrFree( vRange );
    Vec_PtrFree( vNodes );
    Cudd_AutodynDisable( dd );
//    Cudd_RecursiveDeref( dd, bRes );
//    Extra_StopManager( dd );
    dd->bFunc = bRes;
    dd->TimeStop = 0;
    return dd;
}

/**Function*************************************************************

  Synopsis    [Derives positive cube composed of nodes IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_ImgComputeCube( Aig_Man_t * pAig, Vec_Int_t * vNodeIds, DdManager * dd )
{
    DdNode * bProd, * bTemp;
    Aig_Obj_t * pObj;
    int i;
    abctime TimeStop;
    TimeStop = dd->TimeStop; dd->TimeStop = 0;
    bProd = Cudd_ReadOne(dd);   Cudd_Ref( bProd );
    Aig_ManForEachObjVec( vNodeIds, pAig, pObj, i )
    {
        bProd  = Cudd_bddAnd( dd, bTemp = bProd, Cudd_bddIthVar(dd, Aig_ObjId(pObj)) ); Cudd_Ref( bProd );
        Cudd_RecursiveDeref( dd, bTemp );
    }
    Cudd_Deref( bProd );
    dd->TimeStop = TimeStop;
    return bProd;
} 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ImgQuantifyFirst( Aig_Man_t * pAig, Vec_Ptr_t * vDdMans, Vec_Ptr_t * vQuant0, int fVerbose )
{
    DdManager * dd;
    DdNode * bProd, * bRes, * bTemp;
    int i;
    abctime clk = Abc_Clock();
    Vec_PtrForEachEntry( DdManager *, vDdMans, dd, i )
    {
        // remember unquantified ones
        assert( dd->bFunc2 == NULL );
        dd->bFunc2 = dd->bFunc;   Cudd_Ref( dd->bFunc2 );

        Cudd_AutodynEnable( dd, CUDD_REORDER_SYMM_SIFT );

        bRes = dd->bFunc;
        if ( fVerbose )
            Abc_Print( 1, "Part %2d : Init =%5d. ", i, Cudd_DagSize(bRes) );
        bProd = Llb_ImgComputeCube( pAig, (Vec_Int_t *)Vec_PtrEntry(vQuant0, i+1), dd );   Cudd_Ref( bProd );
        bRes  = Cudd_bddExistAbstract( dd, bTemp = bRes, bProd );                          Cudd_Ref( bRes );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bProd );
        dd->bFunc = bRes;

        Cudd_AutodynDisable( dd );

        if ( fVerbose )
            Abc_Print( 1, "Quant =%5d. ", Cudd_DagSize(bRes) );
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
        if ( fVerbose ) 
            Abc_Print( 1, "Reo = %5d. ", Cudd_DagSize(bRes) );
        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
        if ( fVerbose ) 
            Abc_Print( 1, "Reo = %5d.  ", Cudd_DagSize(bRes) );
        if ( fVerbose ) 
            Abc_Print( 1, "Supp = %3d.  ", Cudd_SupportSize(dd, bRes) );
        if ( fVerbose ) 
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );

    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ImgQuantifyReset( Vec_Ptr_t * vDdMans )
{
    DdManager * dd;
    int i;
    Vec_PtrForEachEntry( DdManager *, vDdMans, dd, i )
    {
        assert( dd->bFunc2 != NULL );
        Cudd_RecursiveDeref( dd, dd->bFunc );
        dd->bFunc = dd->bFunc2;
        dd->bFunc2 = NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Computes image of the initial set of states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Llb_ImgComputeImage( Aig_Man_t * pAig, Vec_Ptr_t * vDdMans, DdManager * dd, DdNode * bInit, 
    Vec_Ptr_t * vQuant0, Vec_Ptr_t * vQuant1, Vec_Int_t * vDriRefs, 
    abctime TimeTarget, int fBackward, int fReorder, int fVerbose )
{
//    int fCheckSupport = 0;
    DdManager * ddPart;
    DdNode * bImage, * bGroup, * bCube, * bTemp;
    int i;
    abctime clk, clk0 = Abc_Clock();

    bImage = bInit;  Cudd_Ref( bImage );
    if ( fBackward )
    {
        // change polarity
        bCube  = Llb_DriverPhaseCube( pAig, vDriRefs, dd );                Cudd_Ref( bCube );
        bImage = Extra_bddChangePolarity( dd, bTemp = bImage, bCube );     Cudd_Ref( bImage );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
    }
    else
    {
        // quantify unique vriables
        bCube  = Llb_ImgComputeCube( pAig, (Vec_Int_t *)Vec_PtrEntry(vQuant0, 0), dd ); Cudd_Ref( bCube );
        bImage = Cudd_bddExistAbstract( dd, bTemp = bImage, bCube );                    
        if ( bImage == NULL )
        {
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCube );
            return NULL;
        }
        Cudd_Ref( bImage );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
    }
    // perform image computation
    Vec_PtrForEachEntry( DdManager *, vDdMans, ddPart, i )
    {
        clk = Abc_Clock();
if ( fVerbose )
printf( "   %2d : ", i );
        // transfer the BDD from the group manager to the main manager
        bGroup = Cudd_bddTransfer( ddPart, dd, ddPart->bFunc );                           
        if ( bGroup == NULL )
            return NULL;
        Cudd_Ref( bGroup );
if ( fVerbose )
printf( "Pt0 =%6d. Pt1 =%6d. ", Cudd_DagSize(ddPart->bFunc), Cudd_DagSize(bGroup) );
        // perform partial product
        bCube  = Llb_ImgComputeCube( pAig, (Vec_Int_t *)Vec_PtrEntry(vQuant1, i+1), dd ); Cudd_Ref( bCube );
//        bImage = Cudd_bddAndAbstract( dd, bTemp = bImage, bGroup, bCube );                
//        bImage = Extra_bddAndAbstractTime( dd, bTemp = bImage, bGroup, bCube, TimeTarget );  
        bImage = Cudd_bddAndAbstract( dd, bTemp = bImage, bGroup, bCube );  
        if ( bImage == NULL )
        {
            Cudd_RecursiveDeref( dd, bTemp );
            Cudd_RecursiveDeref( dd, bCube );
            Cudd_RecursiveDeref( dd, bGroup );
            return NULL;
        }
        Cudd_Ref( bImage );

if ( fVerbose )
printf( "Im0 =%6d. Im1 =%6d. ", Cudd_DagSize(bTemp), Cudd_DagSize(bImage) );
//printf("\n"); Extra_bddPrintSupport(dd, bImage); printf("\n");
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
        Cudd_RecursiveDeref( dd, bGroup );

//        Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
//        Abc_Print( 1, "Reo =%6d.  ", Cudd_DagSize(bImage) );

if ( fVerbose )
printf( "Supp =%3d. ", Cudd_SupportSize(dd, bImage) );
if ( fVerbose )
Abc_PrintTime( 1, "T", Abc_Clock() - clk );
    }

    if ( !fBackward )
    {
        // change polarity
        bCube  = Llb_DriverPhaseCube( pAig, vDriRefs, dd );                Cudd_Ref( bCube );
        bImage = Extra_bddChangePolarity( dd, bTemp = bImage, bCube );     Cudd_Ref( bImage );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
    }
    else
    {
        // quantify unique vriables
        bCube  = Llb_ImgComputeCube( pAig, (Vec_Int_t *)Vec_PtrEntry(vQuant0, 0), dd ); Cudd_Ref( bCube );
        bImage = Cudd_bddExistAbstract( dd, bTemp = bImage, bCube );                    Cudd_Ref( bImage );
        Cudd_RecursiveDeref( dd, bTemp );
        Cudd_RecursiveDeref( dd, bCube );
    }

    if ( fReorder )
    {
    if ( fVerbose )
    Abc_Print( 1, "        Reordering... Before =%5d. ", Cudd_DagSize(bImage) );
    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
    if ( fVerbose )
    Abc_Print( 1, "After =%5d. ", Cudd_DagSize(bImage) );
//    Cudd_ReduceHeap( dd, CUDD_REORDER_SYMM_SIFT, 100 );
//    Abc_Print( 1, "After =%5d.  ", Cudd_DagSize(bImage) );
    if ( fVerbose )
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk0 );
//    Abc_Print( 1, "\n" );
    }

    Cudd_Deref( bImage );
    return bImage;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

