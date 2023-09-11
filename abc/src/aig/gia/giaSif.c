/**CFile****************************************************************

  FileName    [giaSif.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Sequential mapping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaSif.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSifDupNode_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( Gia_ObjUpdateTravIdCurrent(p, pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManSifDupNode_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManSifDupNode_rec( pNew, p, Gia_ObjFanin1(pObj) );
    pObj->Value = Gia_ManAppendAnd2( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
} 
void Gia_ManSifDupNode( Gia_Man_t * pNew, Gia_Man_t * p, int iObj, Vec_Int_t * vCopy )
{
    int k, iFan;
    Gia_Obj_t * pObj = Gia_ManObj(p, iObj); 
    Gia_ManIncrementTravId( p );
    Gia_LutForEachFanin( p, iObj, iFan, k )
    {
        assert( Vec_IntEntry(vCopy, iFan) >= 0 );
        Gia_ManObj(p, iFan)->Value = Vec_IntEntry(vCopy, iFan);
        Gia_ObjUpdateTravIdCurrentId(p, iFan);
    }
    Gia_ManSifDupNode_rec( pNew, p, pObj );
    Vec_IntWriteEntry( vCopy, iObj, pObj->Value );
}
Vec_Int_t * Gia_ManSifInitNeg( Gia_Man_t * p, Vec_Int_t * vMoves, Vec_Int_t * vRegs )
{
    Vec_Int_t * vRes = Vec_IntAlloc( Vec_IntSize(vRegs) );
    Gia_Obj_t * pObj; int i, iObj;
    Gia_Man_t * pNew = Gia_ManStart( 1000 ), * pTemp;
    Vec_Int_t * vCopy = Vec_IntStartFull( Gia_ManObjNum(p) );
    Vec_IntWriteEntry( vCopy, 0, 0 );
    Gia_ManForEachRo( p, pObj, i )
        Vec_IntWriteEntry( vCopy, Gia_ObjId(p, pObj), Gia_ManAppendCi(pNew) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Vec_IntForEachEntry( vMoves, iObj, i )
        Gia_ManSifDupNode( pNew, p, iObj, vCopy );
    Vec_IntForEachEntry( vRegs, iObj, i )
    {
        int iLit = Vec_IntEntry( vCopy, iObj );
        assert( iLit >= 0 );
        Gia_ManAppendCo( pNew, iLit );
    }
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    Gia_ManSetPhase( pNew );
    Gia_ManForEachPo( pNew, pObj, i )
        Vec_IntPush( vRes, pObj->fPhase );
    Gia_ManStop( pNew );
    Vec_IntFree( vCopy );
    assert( Vec_IntSize(vRes) == Vec_IntSize(vRegs) );
    return vRes;
}
Vec_Int_t * Gia_ManSifInitPos( Gia_Man_t * p, Vec_Int_t * vMoves, Vec_Int_t * vRegs )
{
    extern int * Abc_NtkSolveGiaMiter( Gia_Man_t * p );
    int i, iObj, iLitAnd = 1, * pResult = NULL;
    Gia_Obj_t * pObj; Vec_Int_t * vRes = NULL;
    Gia_Man_t * pNew = Gia_ManStart( 1000 ), * pTemp;
    Vec_Int_t * vCopy = Vec_IntStartFull( Gia_ManObjNum(p) );
    Vec_IntWriteEntry( vCopy, 0, 0 );
    Vec_IntForEachEntry( vRegs, iObj, i )
        Vec_IntWriteEntry( vCopy, iObj, Gia_ManAppendCi(pNew) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Vec_IntForEachEntry( vMoves, iObj, i )
        Gia_ManSifDupNode( pNew, p, iObj, vCopy );
    Gia_ManForEachRi( p, pObj, i )
    {
        int iFan = Gia_ObjFaninId0p(p, pObj);
        int iLit = Vec_IntEntry(vCopy, iFan);
        if ( iLit == -1 )
            continue;
        iLit = Abc_LitNotCond( iLit, Gia_ObjFaninC0(pObj) );
        iLitAnd = Gia_ManAppendAnd2( pNew, iLitAnd, Abc_LitNot(iLit) );
    }
    Gia_ManAppendCo( pNew, iLitAnd );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    pResult = Abc_NtkSolveGiaMiter( pNew );
    if ( pResult )
    {
        vRes = Vec_IntAllocArray( pResult, Vec_IntSize(vRegs) );
        Gia_ManSetPhasePattern( pNew, vRes );
        assert( Gia_ManPo(pNew, 0)->fPhase == 1 );
    }
    else
    {
        vRes = Vec_IntStart( Vec_IntSize(vRegs) );
        printf( "***!!!*** The SAT problem has no solution. Using all-0 initial state. ***!!!***\n" );
    }
    Gia_ManStop( pNew );
    Vec_IntFree( vCopy );
    assert( Vec_IntSize(vRes) == Vec_IntSize(vRegs) );
    return vRes;
}
Gia_Man_t * Gia_ManSifDerive( Gia_Man_t * p, Vec_Int_t * vCounts, int fVerbose )
{
    Gia_Man_t * pNew   = NULL; Gia_Obj_t * pObj;
    Vec_Int_t * vCopy  = Vec_IntStartFull( Gia_ManObjNum(p) );    
    Vec_Int_t * vCopy2 = Vec_IntStartFull( Gia_ManObjNum(p) );    
    Vec_Int_t * vLuts[3], * vRos[3], * vRegs[2], * vInits[2], * vTemp;
    int i, k, Id, iFan;
    for ( i = 0; i < 3; i++ )
    {
        vLuts[i] = Vec_IntAlloc(100);
        vRos[i]  = Vec_IntAlloc(100);
        if ( i == 2 ) break;
        vRegs[i] = Vec_IntAlloc(100);
    }
    Gia_ManForEachLut( p, i )
        if ( Vec_IntEntry(vCounts, i) == 1 )
            Vec_IntPush( vLuts[0], i );
        else if ( Vec_IntEntry(vCounts, i) == -1 )
            Vec_IntPush( vLuts[1], i );
        else if ( Vec_IntEntry(vCounts, i) == 0 )
            Vec_IntPush( vLuts[2], i );
        else assert( 0 );
    assert( Vec_IntSize(vLuts[0]) || Vec_IntSize(vLuts[1]) );
    if ( Vec_IntSize(vLuts[0]) )
    {
        Gia_ManIncrementTravId( p );
        Vec_IntForEachEntry( vLuts[0], Id, i )
            Gia_ObjSetTravIdCurrentId(p, Id);
        Gia_ManForEachRo( p, pObj, i )
            if ( Gia_ObjIsTravIdCurrent(p, Gia_ObjFanin0(Gia_ObjRoToRi(p, pObj))) )
                Vec_IntPush( vRos[0], Gia_ObjId(p, pObj) );
        assert( !Vec_IntSize(vLuts[0]) == !Vec_IntSize(vRos[0]) );
    }
    if ( Vec_IntSize(vLuts[1]) )
    {
        Gia_ManIncrementTravId( p );
        Vec_IntForEachEntry( vLuts[1], Id, i )
            Gia_LutForEachFanin( p, Id, iFan, k )
                Gia_ObjSetTravIdCurrentId(p, iFan);
        Gia_ManForEachRo( p, pObj, i )
            if ( Gia_ObjIsTravIdCurrent(p, pObj) )
                Vec_IntPush( vRos[1], Gia_ObjId(p, pObj) );
        assert( !Vec_IntSize(vLuts[1]) == !Vec_IntSize(vRos[1]) );
    }
    Gia_ManIncrementTravId( p );
    for ( k = 0; k < 2; k++ )
        Vec_IntForEachEntry( vRos[k], Id, i )
            Gia_ObjSetTravIdCurrentId(p, Id);
    Gia_ManForEachRo( p, pObj, i )
        if ( !Gia_ObjIsTravIdCurrent(p, pObj) )
            Vec_IntPush( vRos[2], Gia_ObjId(p, pObj) );

    Gia_ManIncrementTravId( p );
    Vec_IntForEachEntry( vLuts[0], Id, i )
        Gia_ObjSetTravIdCurrentId(p, Id);
    Vec_IntForEachEntry( vLuts[0], Id, i )
        Gia_LutForEachFanin( p, Id, iFan, k )
            if ( !Gia_ObjUpdateTravIdCurrentId(p, iFan) )
                Vec_IntPush( vRegs[0], iFan );
    Vec_IntSort( vRegs[0], 0 );
    assert( Vec_IntCountDuplicates(vRegs[1]) == 0 );
    assert( !Vec_IntSize(vLuts[0]) == !Vec_IntSize(vRegs[0]) );

    Gia_ManIncrementTravId( p );
    Vec_IntForEachEntry( vLuts[0], Id, i )
        Gia_LutForEachFanin( p, Id, iFan, k )
            Gia_ObjSetTravIdCurrentId(p, iFan);
    Vec_IntForEachEntry( vLuts[2], Id, i )
        Gia_LutForEachFanin( p, Id, iFan, k )
            Gia_ObjSetTravIdCurrentId(p, iFan);
    Gia_ManForEachCo( p, pObj, i )
        Gia_ObjSetTravIdCurrentId(p, Gia_ObjFaninId0p(p, pObj));
    Vec_IntForEachEntry( vRos[1], Id, i )
        if ( Gia_ObjIsTravIdCurrentId(p, Id) )
            Vec_IntPush( vRegs[1], Id );
    Vec_IntForEachEntry( vLuts[1], Id, i )
        if ( Gia_ObjIsTravIdCurrentId(p, Id) )
            Vec_IntPush( vRegs[1], Id );
    Vec_IntSort( vRegs[1], 0 );
    assert( Vec_IntCountDuplicates(vRegs[1]) == 0 );
    assert( !Vec_IntSize(vLuts[1]) == !Vec_IntSize(vRegs[1]) );

    vInits[0] = Vec_IntSize(vLuts[0]) ? Gia_ManSifInitPos( p, vLuts[0], vRegs[0] ) : Vec_IntAlloc(0);
    vInits[1] = Vec_IntSize(vLuts[1]) ? Gia_ManSifInitNeg( p, vLuts[1], vRegs[1] ) : Vec_IntAlloc(0);

    if ( fVerbose )
    {
        printf( "Flops : %5d %5d %5d\n", Vec_IntSize(vRos[0]),  Vec_IntSize(vRos[1]),  Vec_IntSize(vRos[2]) );
        printf( "LUTs  : %5d %5d %5d\n", Vec_IntSize(vLuts[0]), Vec_IntSize(vLuts[1]), Vec_IntSize(vLuts[2]) );
        printf( "Spots : %5d %5d %5d\n", Vec_IntSize(vRegs[0]), Vec_IntSize(vRegs[1]), 0 );
    }

    pNew = Gia_ManStart( Gia_ManObjNum(p) + Vec_IntSize(vRegs[0]) + Vec_IntSize(vRegs[1]) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );

    Vec_IntWriteEntry( vCopy, 0, 0 );
    Gia_ManForEachPi( p, pObj, i )
        Vec_IntWriteEntry( vCopy, Gia_ObjId(p, pObj), Gia_ManAppendCi(pNew) );
    Vec_IntForEachEntry( vRos[2], Id, i )
        Vec_IntWriteEntry( vCopy, Id, Gia_ManAppendCi(pNew) );
    Vec_IntForEachEntry( vRegs[1], Id, i )
        Vec_IntWriteEntry( vCopy, Id, Abc_LitNotCond(Gia_ManAppendCi(pNew), Vec_IntEntry(vInits[1], i)) );

    vTemp = Vec_IntAlloc(100);
    Vec_IntForEachEntry( vRegs[0], Id, i )
        Vec_IntPush( vTemp, Vec_IntEntry(vCopy, Id) );
    Vec_IntForEachEntry( vRegs[0], Id, i )
        Vec_IntWriteEntry( vCopy, Id, Abc_LitNotCond(Gia_ManAppendCi(pNew), Vec_IntEntry(vInits[0], i)) );
    Vec_IntForEachEntry( vLuts[0], Id, i )
        Gia_ManSifDupNode( pNew, p, Id, vCopy );
    Vec_IntForEachEntry( vRegs[0], Id, i )
        Vec_IntWriteEntry( vCopy, Id, Vec_IntEntry(vTemp, i) );
    Vec_IntFree( vTemp );

    Gia_ManForEachRoToRiVec( vRos[0], p, pObj, i )
        Vec_IntWriteEntry( vCopy, Vec_IntEntry(vRos[0], i), Abc_LitNotCond(Vec_IntEntry(vCopy, Gia_ObjFaninId0p(p, pObj)), Gia_ObjFaninC0(pObj)) );
    Vec_IntForEachEntry( vLuts[2], Id, i )
        Gia_ManSifDupNode( pNew, p, Id, vCopy );

    Gia_ManForEachRoToRiVec( vRos[1], p, pObj, i )
        Vec_IntWriteEntry( vCopy2, Vec_IntEntry(vRos[1], i), Abc_LitNotCond(Vec_IntEntry(vCopy, Gia_ObjFaninId0p(p, pObj)), Gia_ObjFaninC0(pObj)) );
    Vec_IntForEachEntry( vLuts[1], Id, i )
        Gia_ManSifDupNode( pNew, p, Id, vCopy2 );

    Gia_ManForEachPo( p, pObj, i )
        Gia_ManAppendCo( pNew, Abc_LitNotCond(Vec_IntEntry(vCopy, Gia_ObjFaninId0p(p, pObj)), Gia_ObjFaninC0(pObj)) );
    Gia_ManForEachRoToRiVec( vRos[2], p, pObj, i )
        Gia_ManAppendCo( pNew, Abc_LitNotCond(Vec_IntEntry(vCopy, Gia_ObjFaninId0p(p, pObj)), Gia_ObjFaninC0(pObj)) );
    Vec_IntForEachEntry( vRegs[1], Id, i )
        Gia_ManAppendCo( pNew, Abc_LitNotCond(Vec_IntEntry(vCopy2, Id), Vec_IntEntry(vInits[1], i)) );
    Vec_IntForEachEntry( vRegs[0], Id, i )
        Gia_ManAppendCo( pNew, Abc_LitNotCond(Vec_IntEntry(vCopy, Id), Vec_IntEntry(vInits[0], i)) );

    Gia_ManSetRegNum( pNew, Vec_IntSize(vRos[2]) + Vec_IntSize(vRegs[0]) + Vec_IntSize(vRegs[1]) );

    for ( i = 0; i < 3; i++ )
    {
        Vec_IntFreeP( &vLuts[i] );
        Vec_IntFreeP( &vRos[i] );
        if ( i == 2 ) break;
        Vec_IntFreeP( &vRegs[i] );
        Vec_IntFreeP( &vInits[i] );
    }
    Vec_IntFree( vCopy );
    Vec_IntFree( vCopy2 );
    return pNew;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSifArea_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vCuts, int nSize )
{
    int i, * pCut, Area = 1;
    if ( Gia_ObjUpdateTravIdCurrent(p, pObj) )
        return 0;
    if ( !Gia_ObjIsAnd(pObj) )
        return 0;
    pCut = Vec_IntEntryP( vCuts, Gia_ObjId(p, pObj)*nSize );
    for ( i = 1; i <= pCut[0]; i++ )
        Area += Gia_ManSifArea_rec( p, Gia_ManObj(p, pCut[i] >> 8), vCuts, nSize );
    return Area;
}
int Gia_ManSifArea( Gia_Man_t * p, Vec_Int_t * vCuts, int nSize )
{
    Gia_Obj_t * pObj; int i, nArea = 0;
    Gia_ManIncrementTravId( p );
    Gia_ManForEachCo( p, pObj, i )
        nArea += Gia_ManSifArea_rec( p, Gia_ObjFanin0(pObj), vCuts, nSize );
    return nArea;
}
int Gia_ManSifDelay_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vCuts, Vec_Int_t * vTimes, int nSize )
{
    int i, * pCut, Delay, nFails = 0;
    if ( Gia_ObjUpdateTravIdCurrent(p, pObj) )
        return 0;
    if ( !Gia_ObjIsAnd(pObj) )
        return 0;
    pCut = Vec_IntEntryP( vCuts, Gia_ObjId(p, pObj)*nSize );
    Delay = -ABC_INFINITY-10000;
    for ( i = 1; i <= pCut[0]; i++ )
    {
        nFails += Gia_ManSifDelay_rec( p, Gia_ManObj(p, pCut[i] >> 8), vCuts, vTimes, nSize );
        Delay = Abc_MaxInt( Delay, Vec_IntEntry(vTimes, pCut[i] >> 8) );
    }
    Delay += 1;
    return nFails + (int)(Delay > Vec_IntEntry(vTimes, Gia_ObjId(p, pObj)));
}
int Gia_ManSifDelay( Gia_Man_t * p, Vec_Int_t * vCuts, Vec_Int_t * vTimes, int nSize )
{
    Gia_Obj_t * pObj; int i, nFails = 0;
    Gia_ManIncrementTravId( p );
    Gia_ManForEachCo( p, pObj, i )
        nFails += Gia_ManSifDelay_rec( p, Gia_ObjFanin0(pObj), vCuts, vTimes, nSize );
    return nFails;
}
static inline int Gia_ManSifTimeToCount( int Value, int Period )
{
    return (Period*0xFFFF + Value)/Period + ((Period*0xFFFF + Value)%Period != 0) - 0x10000;
}
Vec_Int_t * Gia_ManSifTimesToCounts( Gia_Man_t * p, Vec_Int_t * vTimes, int Period )
{
    int i, Times;
    Vec_Int_t * vCounts = Vec_IntStart( Gia_ManObjNum(p) ); 
    Vec_IntFillExtra( vTimes, Gia_ManObjNum(p), 0 );
    Vec_IntForEachEntry( vTimes, Times, i )
        if ( Gia_ObjIsLut(p, i) )
            Vec_IntWriteEntry( vCounts, i, Gia_ManSifTimeToCount(Times, Period) );
    return vCounts;
}
Gia_Man_t * Gia_ManSifTransform( Gia_Man_t * p, Vec_Int_t * vCuts, Vec_Int_t * vTimes, int nLutSize, int Period, int fVerbose )
{
    Gia_Man_t * pNew = NULL; Vec_Int_t * vCounts = NULL;
    if ( fVerbose )
        printf( "Current area = %d.  Period = %d.  ", Gia_ManSifArea(p, vCuts, nLutSize+1), Period );
    if ( fVerbose )
        printf( "Delay checking failed for %d cuts.\n", Gia_ManSifDelay( p, vCuts, vTimes, nLutSize+1 ) );
    vCounts   = Gia_ManSifTimesToCounts( p, vTimes, Period );
    pNew      = Gia_ManSifDerive( p, vCounts, fVerbose );
    Vec_IntFreeP( &vCounts );
    return pNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ManSifCutMerge( int * pCut, int * pCut1, int * pCut2, int nSize )
{
    int * pBeg  = pCut+1;
    int * pBeg1 = pCut1+1;
    int * pBeg2 = pCut2+1;
    int * pEnd1 = pBeg1 + pCut1[0];
    int * pEnd2 = pBeg2 + pCut2[0];
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( pBeg == pCut+nSize )
        {
            pCut[0] = -1;
            return;
        }
        if ( *pBeg1 == *pBeg2 )
            *pBeg++ = *pBeg1++, pBeg2++;
        else if ( *pBeg1 < *pBeg2 )
            *pBeg++ = *pBeg1++;
        else 
            *pBeg++ = *pBeg2++;
    }
    while ( pBeg1 < pEnd1 )
    {
        if ( pBeg == pCut+nSize )
        {
            pCut[0] = -1;
            return;
        }
        *pBeg++ = *pBeg1++;
    }
    while ( pBeg2 < pEnd2 )
    {
        if ( pBeg == pCut+nSize )
        {
            pCut[0] = -1;
            return;
        }
        *pBeg++ = *pBeg2++;
    }
    pCut[0] = pBeg-(pCut+1);
    assert( pCut[0] < nSize );
}
static inline int Gia_ManSifCutChoice( Gia_Man_t * p, int Level, int iObj, int iSibl, Vec_Int_t * vCuts, Vec_Int_t * vTimes, int nSize )
{
    int * pCut  = Vec_IntEntryP( vCuts, iObj*nSize );
    int * pCut2 = Vec_IntEntryP( vCuts, iSibl*nSize );
    int Level2 = Vec_IntEntry( vTimes, iSibl ); int i;
    assert( iObj > iSibl );
    if ( Level < Level2 || (Level == Level2 && pCut[0] <= pCut2[0]) )
        return Level;
    for ( i = 0; i <= pCut2[0]; i++ )
        pCut[i] = pCut2[i];
    return Level2;
}
static inline int Gia_ManSifCutOne( Gia_Man_t * p, int iObj, Vec_Int_t * vCuts, Vec_Int_t * vTimes, int nSize )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    int iFan0 = Gia_ObjFaninId0(pObj, iObj);
    int iFan1 = Gia_ObjFaninId1(pObj, iObj);
    int Cut0[2] = { 1, iFan0 << 8 };
    int Cut1[2] = { 1, iFan1 << 8 };
    int * pCut  = Vec_IntEntryP( vCuts, iObj*nSize );
    int * pCut0 = Vec_IntEntryP( vCuts, iFan0*nSize );
    int * pCut1 = Vec_IntEntryP( vCuts, iFan1*nSize );
    int Level_ = Vec_IntEntry( vTimes, iObj );
    int Level0 = Vec_IntEntry( vTimes, iFan0 );
    int Level1 = Vec_IntEntry( vTimes, iFan1 );
    int Level  = -ABC_INFINITY, i;
    assert( pCut0[0] > 0 && pCut1[0] > 0 );
    if ( Level0 == Level1 )
        Gia_ManSifCutMerge( pCut, pCut0, pCut1, nSize );
    else if ( Level0 > Level1 )
        Gia_ManSifCutMerge( pCut, pCut0, Cut1, nSize );
    else //if ( Level0 < Level1 )
        Gia_ManSifCutMerge( pCut, pCut1, Cut0, nSize );
    if ( pCut[0] == -1 )
    {
        pCut[0] = 2;
        pCut[1] = iFan0 << 8;
        pCut[2] = iFan1 << 8;
    }
    for ( i = 1; i <= pCut[0]; i++ )
        Level = Abc_MaxInt( Level, Vec_IntEntry(vTimes, pCut[i] >> 8) );
    Level++;
    if ( Gia_ObjSibl(p, iObj) )
        Level = Gia_ManSifCutChoice( p, Level, iObj, Gia_ObjSibl(p, iObj), vCuts, vTimes, nSize );
    assert( pCut[0] > 0 && pCut[0] < nSize );
    Vec_IntUpdateEntry( vTimes, iObj, Level );
    return Level > Level_;
}
int Gia_ManSifCheckIter( Gia_Man_t * p, Vec_Int_t * vCuts, Vec_Int_t * vTimes, int nLutSize, int Period )
{
    int i, fChange = 0, nSize = nLutSize+1;
    Gia_Obj_t * pObj, * pObjRi, * pObjRo; 
    Gia_ManForEachAnd( p, pObj, i )
        fChange |= Gia_ManSifCutOne( p, i, vCuts, vTimes, nSize );
    Gia_ManForEachCo( p, pObj, i )
        Vec_IntWriteEntry( vTimes, Gia_ObjId(p, pObj), Vec_IntEntry(vTimes, Gia_ObjFaninId0p(p, pObj)) );
    Gia_ManForEachRiRo( p, pObjRi, pObjRo, i )
    {
        int TimeNew = Vec_IntEntry(vTimes, Gia_ObjId(p, pObjRi)) - Period;
        TimeNew = Abc_MaxInt( TimeNew, Vec_IntEntry(vTimes, Gia_ObjId(p, pObjRo)) );
        Vec_IntWriteEntry( vTimes, Gia_ObjId(p, pObjRo), TimeNew );
    }
    return fChange;
}
int Gia_ManSifCheckPeriod( Gia_Man_t * p, Vec_Int_t * vCuts, Vec_Int_t * vTimes, int nLutSize, int Period, int * pIters )
{
    Gia_Obj_t * pObj; int i, Id, nSize = nLutSize+1;
    assert( Gia_ManRegNum(p) > 0 );
    Gia_ManForEachCiId( p, Id, i )
        Vec_IntWriteEntry( vCuts, Id*nSize, 1 );
    Gia_ManForEachCiId( p, Id, i )
        Vec_IntWriteEntry( vCuts, Id*nSize+1, Id << 8 );
    Vec_IntFill( vTimes, Gia_ManObjNum(p), -Period );
    Vec_IntWriteEntry( vTimes, 0, 0 );
    Gia_ManForEachPi( p, pObj, i )
        Vec_IntWriteEntry( vTimes, Gia_ObjId(p, pObj), 0 );
    for ( *pIters = 0; *pIters < 100; (*pIters)++ )
    {
        if ( !Gia_ManSifCheckIter(p, vCuts, vTimes, nLutSize, Period) )
            return 1;
        Gia_ManForEachPo( p, pObj, i )
            if ( Vec_IntEntry(vTimes, Gia_ObjId(p, pObj)) > Period )
                return 0;
        Gia_ManForEachObj( p, pObj, i )
            if ( Vec_IntEntry(vTimes, Gia_ObjId(p, pObj)) > 2*Period )
                return 0;
    }
    return 0;
}
int Gia_ManSifMapComb( Gia_Man_t * p, Vec_Int_t * vCuts, Vec_Int_t * vTimes, int nLutSize )
{
    Gia_Obj_t * pObj; int i, Id, Res = 0, nSize = nLutSize+1; 
    Vec_IntFill( vTimes, Gia_ManObjNum(p), 0 );
    Gia_ManForEachCiId( p, Id, i )
        Vec_IntWriteEntry( vCuts, Id*nSize, 1 );
    Gia_ManForEachCiId( p, Id, i )
        Vec_IntWriteEntry( vCuts, Id*nSize+1, Id << 8 );
    Gia_ManForEachAnd( p, pObj, i )
        Gia_ManSifCutOne( p, i, vCuts, vTimes, nSize );
    Gia_ManForEachCo( p, pObj, i )
        Res = Abc_MaxInt( Res, Vec_IntEntry(vTimes, Gia_ObjFaninId0p(p, pObj)) );
    return Res;
}
void Gia_ManSifPrintTimes( Gia_Man_t * p, Vec_Int_t * vTimes, int Period )
{
    int i, Value, Pos[256] = {0}, Neg[256] = {0};
    Gia_ManForEachLut( p, i )
    {        
        Value = Gia_ManSifTimeToCount( Vec_IntEntry(vTimes, i), Period );
        Value = Abc_MinInt( Value,  255 );
        Value = Abc_MaxInt( Value, -255 );
        if ( Value >= 0 )
            Pos[Value]++;
        else
            Neg[-Value]++;
    }
    printf( "Statistics: " );
    for ( i = 255; i > 0; i-- )
        if ( Neg[i] )
            printf( " -%d=%d", i, Neg[i] );
    for ( i = 0; i < 256; i++ )
        if ( Pos[i] )
            printf( " %d=%d", i, Pos[i] );
    printf( "\n" );
}
int Gia_ManSifDeriveMapping_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vCuts, int nSize )
{
    int i, * pCut, Area = 1;
    if ( !Gia_ObjIsAnd(pObj) )
        return 0;
    if ( Gia_ObjUpdateTravIdCurrent(p, pObj) )
        return 0;
    pCut = Vec_IntEntryP( vCuts, Gia_ObjId(p, pObj)*nSize );
    for ( i = 1; i <= pCut[0]; i++ )
        Area += Gia_ManSifDeriveMapping_rec( p, Gia_ManObj(p, pCut[i] >> 8), vCuts, nSize );
    Vec_IntWriteEntry( p->vMapping, Gia_ObjId(p, pObj), Vec_IntSize(p->vMapping) );
    Vec_IntPush( p->vMapping, pCut[0] );
    for ( i = 1; i <= pCut[0]; i++ )
    {
        Gia_Obj_t * pObj = Gia_ManObj(p, pCut[i] >> 8);
        assert( !Gia_ObjIsAnd(pObj) || Gia_ObjIsLut(p, pCut[i] >> 8) );
        Vec_IntPush( p->vMapping, pCut[i] >> 8 );
    }
    Vec_IntPush( p->vMapping, -1 );
    return Area;
}
int Gia_ManSifDeriveMapping( Gia_Man_t * p, Vec_Int_t * vCuts, Vec_Int_t * vTimes, int nLutSize, int Period, int fVerbose )
{
    Gia_Obj_t * pObj; int i, nArea = 0;
    if ( p->vMapping != NULL )
    {
        printf( "Removing available combinational mapping.\n" );
        Vec_IntFreeP( &p->vMapping );
    }
    assert( p->vMapping == NULL );
    p->vMapping = Vec_IntStart( Gia_ManObjNum(p) );
    Gia_ManIncrementTravId( p );
    Gia_ManForEachCo( p, pObj, i )
        nArea += Gia_ManSifDeriveMapping_rec( p, Gia_ObjFanin0(pObj), vCuts, nLutSize+1 );
    return nArea;
}
Gia_Man_t * Gia_ManSifPerform( Gia_Man_t * p, int nLutSize, int fEvalOnly, int fVerbose )
{
    Gia_Man_t * pNew = NULL;
    int nIters, Status, nSize = nLutSize+1; // (2+1+nSize)*4=40 bytes/node
    abctime clk = Abc_Clock(); 
    Vec_Int_t * vCuts  = Vec_IntStart( Gia_ManObjNum(p) * nSize );
    Vec_Int_t * vTimes = Vec_IntAlloc( Gia_ManObjNum(p) );
    int Lower = 0;
    int Upper = Gia_ManSifMapComb( p, vCuts, vTimes, nLutSize );
    int CombD = Upper;
    if ( fVerbose && Gia_ManRegNum(p) )
    printf( "Clock period %2d is %s\n", Lower, 0 ? "Yes" : "No " );
    if ( fVerbose && Gia_ManRegNum(p) )
    printf( "Clock period %2d is %s\n", Upper, 1 ? "Yes" : "No " );
    while ( Gia_ManRegNum(p) > 0 && Upper - Lower > 1 )
    {
        int Middle = (Upper + Lower) / 2;
        int Status = Gia_ManSifCheckPeriod( p, vCuts, vTimes, nLutSize, Middle, &nIters );
        if ( Status )
            Upper = Middle;
        else
            Lower = Middle;
        if ( fVerbose )
        printf( "Clock period %2d is %s after %d iterations\n", Middle, Status ? "Yes" : "No ", nIters );
    }
    if ( fVerbose )
    printf( "Best  period = <<%d>> (%.2f %%)  ", Upper, (float)(100.0*(CombD-Upper)/CombD) );
    if ( fVerbose )
    printf( "LUT size = %d   ", nLutSize );
    if ( fVerbose )
    printf( "Memory usage = %.2f MB   ", 4.0*(2+1+nSize)*Gia_ManObjNum(p)/(1 << 20) );
    if ( fVerbose )
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( Upper == CombD )
    {
        Vec_IntFree( vCuts );
        Vec_IntFree( vTimes );
        printf( "Combinational delay (%d) cannot be improved.\n", CombD );
        return Gia_ManDup( p );
    }
    Status = Gia_ManSifCheckPeriod( p, vCuts, vTimes, nLutSize, Upper, &nIters );
    assert( Status );
    Status = Gia_ManSifDeriveMapping( p, vCuts, vTimes, nLutSize, Upper, fVerbose );
    if ( fEvalOnly )
    {
        printf( "Optimized level %2d  (%6.2f %% less than comb level %2d).  LUT size = %d.  Area estimate = %d.\n", 
            Upper, (float)(100.0*(CombD-Upper)/CombD), CombD, nLutSize, Gia_ManSifArea(p, vCuts, nLutSize+1) );     
        printf( "The command is invoked in the evaluation mode. Retiming is not performed.\n" );
    }
    else
        pNew = Gia_ManSifTransform( p, vCuts, vTimes, nLutSize, Upper, fVerbose );
    Vec_IntFree( vCuts );
    Vec_IntFree( vTimes );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

