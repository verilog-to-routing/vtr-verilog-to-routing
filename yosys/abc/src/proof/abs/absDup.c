/**CFile****************************************************************

  FileName    [absDup.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Abstraction package.]

  Synopsis    [Duplication procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: absDup.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#include "abs.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Duplicates the AIG manager recursively.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupAbsFlops_rec( Gia_Man_t * pNew, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupAbsFlops_rec( pNew, Gia_ObjFanin0(pObj) );
    Gia_ManDupAbsFlops_rec( pNew, Gia_ObjFanin1(pObj) );
    pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Extractes a flop-level abstraction given a flop map.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupAbsFlops( Gia_Man_t * p, Vec_Int_t * vFlopClasses )
{ 
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, nFlops = 0;
    Gia_ManFillValue( p );
    // start the new manager
    pNew = Gia_ManStart( 5000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // create PIs
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    // create additional PIs
    Gia_ManForEachRo( p, pObj, i )
        if ( !Vec_IntEntry(vFlopClasses, i) )
            pObj->Value = Gia_ManAppendCi(pNew);
    // create ROs
    Gia_ManForEachRo( p, pObj, i )
        if ( Vec_IntEntry(vFlopClasses, i) )
            pObj->Value = Gia_ManAppendCi(pNew);
    // create POs
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachPo( p, pObj, i )
    {
        Gia_ManDupAbsFlops_rec( pNew, Gia_ObjFanin0(pObj) );
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    // create RIs
    Gia_ManForEachRi( p, pObj, i )
        if ( Vec_IntEntry(vFlopClasses, i) )
        {
            Gia_ManDupAbsFlops_rec( pNew, Gia_ObjFanin0(pObj) );
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
            nFlops++;
        }
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, nFlops );
    // clean up
    pNew = Gia_ManSeqCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Returns the array of neighbors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_GlaCollectAssigned( Gia_Man_t * p, Vec_Int_t * vGateClasses )
{
    Vec_Int_t * vAssigned;
    Gia_Obj_t * pObj;
    int i, Entry;
    vAssigned = Vec_IntAlloc( 1000 );
    Vec_IntForEachEntry( vGateClasses, Entry, i )
    {
        if ( Entry == 0 )
            continue;
        assert( Entry > 0 );
        pObj = Gia_ManObj( p, i );
        Vec_IntPush( vAssigned, Gia_ObjId(p, pObj) );
        if ( Gia_ObjIsAnd(pObj) )
        {
            Vec_IntPush( vAssigned, Gia_ObjFaninId0p(p, pObj) );
            Vec_IntPush( vAssigned, Gia_ObjFaninId1p(p, pObj) );
        }
        else if ( Gia_ObjIsRo(p, pObj) )
            Vec_IntPush( vAssigned, Gia_ObjFaninId0p(p, Gia_ObjRoToRi(p, pObj)) );
        else assert( Gia_ObjIsConst0(pObj) );
    }
    Vec_IntUniqify( vAssigned );
    return vAssigned;
}

/**Function*************************************************************

  Synopsis    [Collects PIs and PPIs of the abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManGlaCollect( Gia_Man_t * p, Vec_Int_t * vGateClasses, Vec_Int_t ** pvPis, Vec_Int_t ** pvPPis, Vec_Int_t ** pvFlops, Vec_Int_t ** pvNodes )
{ 
    Vec_Int_t * vAssigned;
    Gia_Obj_t * pObj;
    int i;
    assert( Gia_ManPoNum(p) == 1 );
    assert( Vec_IntSize(vGateClasses) == Gia_ManObjNum(p) );
    // create included objects and their fanins
    vAssigned = Gia_GlaCollectAssigned( p, vGateClasses );
    // create additional arrays
    if ( pvPis )   *pvPis   = Vec_IntAlloc( 100 );
    if ( pvPPis )  *pvPPis  = Vec_IntAlloc( 100 );
    if ( pvFlops ) *pvFlops = Vec_IntAlloc( 100 );
    if ( pvNodes ) *pvNodes = Vec_IntAlloc( 1000 );
    Gia_ManForEachObjVec( vAssigned, p, pObj, i )
    {
        if ( Gia_ObjIsPi(p, pObj) )
            { if ( pvPis )   Vec_IntPush( *pvPis, Gia_ObjId(p,pObj) );   }
        else if ( !Vec_IntEntry(vGateClasses, Gia_ObjId(p,pObj)) )
            { if ( pvPPis )  Vec_IntPush( *pvPPis, Gia_ObjId(p,pObj) );  }
        else if ( Gia_ObjIsRo(p, pObj) )
            { if ( pvFlops ) Vec_IntPush( *pvFlops, Gia_ObjId(p,pObj) ); }
        else if ( Gia_ObjIsAnd(pObj) )
            { if ( pvNodes ) Vec_IntPush( *pvNodes, Gia_ObjId(p,pObj) ); }
        else assert( Gia_ObjIsConst0(pObj) );
    }
    Vec_IntFree( vAssigned );
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG manager recursively.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDupAbsGates_rec( Gia_Man_t * pNew, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupAbsGates_rec( pNew, Gia_ObjFanin0(pObj) );
    Gia_ManDupAbsGates_rec( pNew, Gia_ObjFanin1(pObj) );
    pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Extractes a gate-level abstraction given a gate map.]

  Description [The array contains 1 for those objects (const, RO, AND)
  that are included in the abstraction; 0, otherwise.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupAbsGates( Gia_Man_t * p, Vec_Int_t * vGateClasses )
{ 
    Vec_Int_t * vPis, * vPPis, * vFlops, * vNodes;
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pCopy;
    int i;//, nFlops = 0;
    assert( Gia_ManPoNum(p) == 1 );
    assert( Vec_IntSize(vGateClasses) == Gia_ManObjNum(p) );

    // create additional arrays
    Gia_ManGlaCollect( p, vGateClasses, &vPis, &vPPis, &vFlops, &vNodes );

    // start the new manager
    pNew = Gia_ManStart( 5000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    // create constant
    Gia_ManFillValue( p );
    Gia_ManConst0(p)->Value = 0;
    // create PIs
    Gia_ManForEachObjVec( vPis, p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    // create additional PIs
    Gia_ManForEachObjVec( vPPis, p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    // create ROs
    Gia_ManForEachObjVec( vFlops, p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    // create internal nodes
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
//        Gia_ManDupAbsGates_rec( pNew, pObj );
    // create PO
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    // create RIs
    Gia_ManForEachObjVec( vFlops, p, pObj, i )
        Gia_ObjRoToRi(p, pObj)->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(Gia_ObjRoToRi(p, pObj)) );
    Gia_ManSetRegNum( pNew, Vec_IntSize(vFlops) );
    // clean up
    pNew = Gia_ManSeqCleanup( pTemp = pNew );
    // transfer copy values: (p -> pTemp -> pNew) => (p -> pNew)
    if ( Gia_ManObjNum(pTemp) != Gia_ManObjNum(pNew) )
    {
//        printf( "Gia_ManDupAbsGates() Internal error: object mismatch.\n" );
        Gia_ManForEachObj( p, pObj, i )
        {
            if ( !~pObj->Value )
                continue;
            assert( !Abc_LitIsCompl(pObj->Value) );
            pCopy = Gia_ObjCopy( pTemp, pObj );
            if ( !~pCopy->Value )
            {
                Vec_IntWriteEntry( vGateClasses, i, 0 );
                pObj->Value = ~0;
                continue;
            }
            assert( !Abc_LitIsCompl(pCopy->Value) );
            pObj->Value = pCopy->Value;
        }
    }
    Gia_ManStop( pTemp );

    Vec_IntFree( vPis );
    Vec_IntFree( vPPis );
    Vec_IntFree( vFlops );
    Vec_IntFree( vNodes );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Prints stats for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintFlopClasses( Gia_Man_t * p )
{
    int Counter0, Counter1;
    if ( p->vFlopClasses == NULL )
        return;
    if ( Vec_IntSize(p->vFlopClasses) != Gia_ManRegNum(p) )
    {
        printf( "Gia_ManPrintFlopClasses(): The number of flop map entries differs from the number of flops.\n" );
        return;
    }
    Counter0 = Vec_IntCountEntry( p->vFlopClasses, 0 );
    Counter1 = Vec_IntCountEntry( p->vFlopClasses, 1 );
    printf( "Flop-level abstraction:  Excluded FFs = %d  Included FFs = %d  (%.2f %%) ", 
        Counter0, Counter1, 100.0*Counter1/(Counter0 + Counter1 + 1) );
    if ( Counter0 + Counter1 < Gia_ManRegNum(p) )
        printf( "and there are other FF classes..." );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints stats for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintGateClasses( Gia_Man_t * p )
{
    Vec_Int_t * vPis, * vPPis, * vFlops, * vNodes;
    int nTotal;
    if ( p->vGateClasses == NULL )
        return;
    if ( Vec_IntSize(p->vGateClasses) != Gia_ManObjNum(p) )
    {
        printf( "Gia_ManPrintGateClasses(): The number of flop map entries differs from the number of flops.\n" );
        return;
    }
    // create additional arrays
    Gia_ManGlaCollect( p, p->vGateClasses, &vPis, &vPPis, &vFlops, &vNodes );
    nTotal = 1 + Vec_IntSize(vFlops) + Vec_IntSize(vNodes);
    printf( "Gate-level abstraction:  PI = %d  PPI = %d  FF = %d (%.2f %%)  AND = %d (%.2f %%)  Obj = %d (%.2f %%)\n", 
        Vec_IntSize(vPis), Vec_IntSize(vPPis), 
        Vec_IntSize(vFlops), 100.0*Vec_IntSize(vFlops)/(Gia_ManRegNum(p)+1), 
        Vec_IntSize(vNodes), 100.0*Vec_IntSize(vNodes)/(Gia_ManAndNum(p)+1), 
        nTotal,              100.0*nTotal             /(Gia_ManRegNum(p)+Gia_ManAndNum(p)+1) );
    Vec_IntFree( vPis );
    Vec_IntFree( vPPis );
    Vec_IntFree( vFlops );
    Vec_IntFree( vNodes );
}

/**Function*************************************************************

  Synopsis    [Prints stats for the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManPrintObjClasses( Gia_Man_t * p )
{
    Vec_Int_t * vSeens;  // objects seen so far
    Vec_Int_t * vAbs = p->vObjClasses;
    int i, k, Entry, iStart, iStop = -1, nFrames;
    int nObjBits, nObjMask, iObj, iFrame, nWords;
    unsigned * pInfo;
    int * pCountAll, * pCountUni;
    if ( vAbs == NULL )
        return;
    nFrames = Vec_IntEntry( vAbs, 0 );
    assert( Vec_IntEntry(vAbs, nFrames+1) == Vec_IntSize(vAbs) );
    pCountAll = ABC_ALLOC( int, nFrames + 1 );
    pCountUni = ABC_ALLOC( int, nFrames + 1 );
    // start storage for seen objects
    nWords = Abc_BitWordNum( nFrames );
    vSeens = Vec_IntStart( Gia_ManObjNum(p) * nWords );
    // get the bitmasks
    nObjBits = Abc_Base2Log( Gia_ManObjNum(p) );
    nObjMask = (1 << nObjBits) - 1;
    assert( Gia_ManObjNum(p) <= nObjMask );
    // print info about frames
    printf( "Frame   Core   F0   F1   F2   F3 ...\n" );
    for ( i = 0; i < nFrames; i++ )
    {
        iStart = Vec_IntEntry( vAbs, i+1 );
        iStop  = Vec_IntEntry( vAbs, i+2 );
        memset( pCountAll, 0, sizeof(int) * (nFrames + 1) );
        memset( pCountUni, 0, sizeof(int) * (nFrames + 1) );
        Vec_IntForEachEntryStartStop( vAbs, Entry, k, iStart, iStop )
        {
            iObj   = (Entry &  nObjMask);
            iFrame = (Entry >> nObjBits);
            pInfo  = (unsigned *)Vec_IntEntryP( vSeens, nWords * iObj );
            if ( Abc_InfoHasBit(pInfo, iFrame) == 0 )
            {
                Abc_InfoSetBit( pInfo, iFrame );
                pCountUni[iFrame+1]++;
                pCountUni[0]++;
            }
            pCountAll[iFrame+1]++;
            pCountAll[0]++;
        }
        assert( pCountAll[0] == (iStop - iStart) );
//        printf( "%5d%5d  ", pCountAll[0], pCountUni[0] ); 
        printf( "%3d :", i );
        printf( "%7d", pCountAll[0] ); 
        if ( i >= 10 )
        {
            for ( k = 0; k < 4; k++ )
                printf( "%5d", pCountAll[k+1] ); 
            printf( "  ..." );
            for ( k = i-4; k <= i; k++ )
                printf( "%5d", pCountAll[k+1] ); 
        }
        else
        {
            for ( k = 0; k <= i; k++ )
                if ( k <= i )
                    printf( "%5d", pCountAll[k+1] ); 
        }
//        for ( k = 0; k < nFrames; k++ )
//            if ( k <= i )
//                printf( "%5d", pCountAll[k+1] ); 
        printf( "\n" );
    }
    assert( iStop == Vec_IntSize(vAbs) );
    Vec_IntFree( vSeens );
    ABC_FREE( pCountAll );
    ABC_FREE( pCountUni );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

