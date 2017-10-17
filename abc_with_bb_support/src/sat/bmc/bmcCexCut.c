/**CFile****************************************************************

  FileName    [bmcCexCut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Derives characterization of bad states.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcCexCut.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "aig/gia/giaAig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Generate justifying assignments.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmc_GiaGenerateJust_rec( Gia_Man_t * p, int iFrame, int iObj, Vec_Bit_t * vValues, Vec_Bit_t * vJustis )
{
    Gia_Obj_t * pObj;
    int Shift = Gia_ManObjNum(p) * iFrame;
    if ( iFrame < 0 )
        return 0;
    assert( iFrame >= 0 );
    if ( Vec_BitEntry( vJustis, Shift + iObj ) )
        return 0;
    Vec_BitWriteEntry( vJustis, Shift + iObj, 1 );
    pObj = Gia_ManObj( p, iObj );
    if ( Gia_ObjIsCo(pObj) )
        return Bmc_GiaGenerateJust_rec( p, iFrame, Gia_ObjFaninId0(pObj, iObj), vValues, vJustis );
    if ( Gia_ObjIsCi(pObj) )
        return Bmc_GiaGenerateJust_rec( p, iFrame-1, Gia_ObjId(p, Gia_ObjRoToRi(p, pObj)), vValues, vJustis );
    assert( Gia_ObjIsAnd(pObj) );
    if ( Vec_BitEntry( vValues, Shift + iObj ) )
    {
        Bmc_GiaGenerateJust_rec( p, iFrame, Gia_ObjFaninId0(pObj, iObj), vValues, vJustis );
        Bmc_GiaGenerateJust_rec( p, iFrame, Gia_ObjFaninId1(pObj, iObj), vValues, vJustis );
    }
    else if ( Vec_BitEntry( vValues, Shift + Gia_ObjFaninId0(pObj, iObj) ) == Gia_ObjFaninC0(pObj) )
        Bmc_GiaGenerateJust_rec( p, iFrame, Gia_ObjFaninId0(pObj, iObj), vValues, vJustis );
    else if ( Vec_BitEntry( vValues, Shift + Gia_ObjFaninId1(pObj, iObj) ) == Gia_ObjFaninC1(pObj) )
        Bmc_GiaGenerateJust_rec( p, iFrame, Gia_ObjFaninId1(pObj, iObj), vValues, vJustis );
    else assert( 0 );
    return 0;
}
void Bmc_GiaGenerateJustNonRec( Gia_Man_t * p, int iFrame, Vec_Bit_t * vValues, Vec_Bit_t * vJustis )
{
    Gia_Obj_t * pObj;
    int i, k, Shift = Gia_ManObjNum(p) * iFrame;
    for ( i = iFrame; i >= 0; i--, Shift -= Gia_ManObjNum(p) )
    {
        Gia_ManForEachObjReverse( p, pObj, k )
        {
            if ( k == 0 || Gia_ObjIsPi(p, pObj) )
                continue;
            if ( !Vec_BitEntry( vJustis, Shift + k ) )
                continue;
            if ( Gia_ObjIsAnd(pObj) )
            {
                if ( Vec_BitEntry( vValues, Shift + k ) )
                {
                    Vec_BitWriteEntry( vJustis, Shift + Gia_ObjFaninId0(pObj, k), 1 );
                    Vec_BitWriteEntry( vJustis, Shift + Gia_ObjFaninId1(pObj, k), 1 );
                }
                else if ( Vec_BitEntry( vValues, Shift + Gia_ObjFaninId0(pObj, k) ) == Gia_ObjFaninC0(pObj) )
                    Vec_BitWriteEntry( vJustis, Shift + Gia_ObjFaninId0(pObj, k), 1 );
                else if ( Vec_BitEntry( vValues, Shift + Gia_ObjFaninId1(pObj, k) ) == Gia_ObjFaninC1(pObj) )
                    Vec_BitWriteEntry( vJustis, Shift + Gia_ObjFaninId1(pObj, k), 1 );
                else assert( 0 );
            }
            else if ( Gia_ObjIsCo(pObj) )
                Vec_BitWriteEntry( vJustis, Shift + Gia_ObjFaninId0(pObj, k), 1 );
            else if ( Gia_ObjIsCi(pObj) && i )
                Vec_BitWriteEntry( vJustis, Shift - Gia_ManObjNum(p) + Gia_ObjId(p, Gia_ObjRoToRi(p, pObj)), 1 );
        }
    }
}
void Bmc_GiaGenerateJust( Gia_Man_t * p, Abc_Cex_t * pCex, Vec_Bit_t ** pvValues, Vec_Bit_t ** pvJustis )
{
    Vec_Bit_t * vValues = Vec_BitStart( Gia_ManObjNum(p) * (pCex->iFrame + 1) );
    Vec_Bit_t * vJustis = Vec_BitStart( Gia_ManObjNum(p) * (pCex->iFrame + 1) );
    Gia_Obj_t * pObj;
    int i, k, iBit = 0, fCompl0, fCompl1, fJusti0, fJusti1, Shift;

    Gia_ManCleanMark0(p);
    Gia_ManCleanMark1(p);
    Gia_ManForEachRi( p, pObj, k )
        pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
    for ( Shift = i = 0; i <= pCex->iFrame; i++, Shift += Gia_ManObjNum(p) )
    {
        Gia_ManForEachObj( p, pObj, k )
        {
            if ( Gia_ObjIsAnd(pObj) )
            {
                fCompl0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
                fCompl1 = Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj);
                fJusti0 = Gia_ObjFanin0(pObj)->fMark1;
                fJusti1 = Gia_ObjFanin1(pObj)->fMark1;
                pObj->fMark0 = fCompl0 & fCompl1;
                if ( pObj->fMark0 )
                    pObj->fMark1 = fJusti0 & fJusti1;
                else if ( !fCompl0 && !fCompl1 )
                    pObj->fMark1 = fJusti0 | fJusti1;
                else if ( !fCompl0 )
                    pObj->fMark1 = fJusti0;
                else if ( !fCompl1 )
                    pObj->fMark1 = fJusti1;
                else assert( 0 );
            }
            else if ( Gia_ObjIsCi(pObj) )
            {
                if ( Gia_ObjIsPi(p, pObj) )
                {
                    pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
                    pObj->fMark1 = 1;
                }
                else
                {
                    pObj->fMark0 = Gia_ObjRoToRi(p, pObj)->fMark0;
                    pObj->fMark1 = Gia_ObjRoToRi(p, pObj)->fMark1;
                }
            }
            else if ( Gia_ObjIsCo(pObj) )
            {
                pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1;
            }
            else if ( Gia_ObjIsConst0(pObj) )
                pObj->fMark1 = 1;
            else assert( 0 );
            if ( pObj->fMark0 )
                Vec_BitWriteEntry( vValues, Shift + k, 1 );
            if ( pObj->fMark1 )
                Vec_BitWriteEntry( vJustis, Shift + k, 1 );
        }
    }
    assert( iBit == pCex->nBits );
    Gia_ManCleanMark0(p);
    Gia_ManCleanMark1(p);
    // perform backward traversal to mark just nodes
    pObj = Gia_ManPo( p, pCex->iPo );
    assert( Vec_BitEntry(vJustis, Gia_ManObjNum(p) * pCex->iFrame + Gia_ObjId(p, pObj)) == 0 );
//    Bmc_GiaGenerateJust_rec( p, pCex->iFrame, Gia_ObjId(p, pObj), vValues, vJustis );
    Vec_BitWriteEntry(vJustis, Gia_ManObjNum(p) * pCex->iFrame + Gia_ObjId(p, pObj), 1);
    Bmc_GiaGenerateJustNonRec( p, pCex->iFrame, vValues, vJustis );
    assert( Vec_BitEntry(vJustis, Gia_ManObjNum(p) * pCex->iFrame + Gia_ObjId(p, pObj)) == 1 );

    // return the result
    *pvValues = vValues;
    *pvJustis = vJustis;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Bmc_GiaGenerateGiaOne( Gia_Man_t * p, Abc_Cex_t * pCex, Vec_Bit_t ** pvInits, int iFrBeg, int iFrEnd )
{
    Vec_Bit_t * vValues;
    Vec_Bit_t * vJustis;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int k, Cube = 1, Counter = 0;
    Bmc_GiaGenerateJust( p, pCex, &vValues, &vJustis );
    // collect flop values in frame iFrBeg
    *pvInits = Vec_BitStart( Gia_ManRegNum(p) );
    Gia_ManForEachRo( p, pObj, k )
        if ( Vec_BitEntry(vValues, Gia_ManObjNum(p) * iFrBeg + Gia_ObjId(p, pObj)) )
            Vec_BitWriteEntry( *pvInits, k, 1 );
    // create GIA with justified values in iFrEnd
    pNew = Gia_ManStart( 2 * Gia_ManRegNum(p) + 2 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManForEachRo( p, pObj, k )
    {
        int Literal = Gia_ManAppendCi(pNew);
        if ( !Vec_BitEntry(vJustis, Gia_ManObjNum(p) * iFrEnd + Gia_ObjId(p, pObj)) )
            continue;
        if ( Vec_BitEntry(vValues, Gia_ManObjNum(p) * iFrEnd + Gia_ObjId(p, pObj)) )
            Cube = Gia_ManAppendAnd( pNew, Cube, Literal );
        else
            Cube = Gia_ManAppendAnd( pNew, Cube, Abc_LitNot(Literal) );
        Counter++;
    }
//    printf( "Only %d flops (out of %d) belong to the care set.\n", Counter, Gia_ManRegNum(p) );
    Gia_ManAppendCo( pNew, Cube );
    Vec_BitFree( vValues );
    Vec_BitFree( vJustis );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Generates all frames from G to the last one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Bmc_GiaGenerateGiaAllFrames( Gia_Man_t * p, Abc_Cex_t * pCex, Vec_Bit_t ** pvInits, int iFrBeg, int iFrEnd )
{
    Vec_Bit_t * vInitEnd;
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObjRo, * pObjRi;
    int f, i, k, iBitOld, iBit = 0, fCompl0, fCompl1;

    // skip trough the first iFrEnd frames
    Gia_ManCleanMark0(p);
    Gia_ManForEachRo( p, pObj, k )
        pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
    *pvInits = Vec_BitStart( Gia_ManRegNum(p) );
    for ( i = 0; i < iFrEnd; i++ )
    {
        // remember values in frame iFrBeg
        if ( i == iFrBeg )
            Gia_ManForEachRo( p, pObjRo, k )
                if ( pObjRo->fMark0 )
                    Vec_BitWriteEntry( *pvInits, k, 1 );
        // simulate other values
        Gia_ManForEachPi( p, pObj, k )
            pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
        Gia_ManForEachAnd( p, pObj, k )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                           (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachCo( p, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
            pObjRo->fMark0 = pObjRi->fMark0;
    }
    assert( i == iFrEnd );
    vInitEnd = Vec_BitStart( Gia_ManRegNum(p) );
    Gia_ManForEachRo( p, pObjRo, k )
        if ( pObjRo->fMark0 )
            Vec_BitWriteEntry( vInitEnd, k, 1 );

    // create new AIG manager
    pNew = Gia_ManStart( 10000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManForEachRo( p, pObjRo, k )
        Gia_ManAppendCi(pNew);
    Gia_ManHashStart( pNew );

    Gia_ManConst0(p)->Value = 1;
    Gia_ManForEachPi( p, pObj, k )
        pObj->Value = 1;

    iBitOld = iBit;
    for ( f = iFrEnd; f <= pCex->iFrame; f++ )
    {
        // set up correct init state
        Gia_ManForEachRo( p, pObjRo, k )
            pObjRo->fMark0 = Vec_BitEntry( vInitEnd, k );
        // simulate it for a few frames
        iBit = iBitOld;
        for ( i = iFrEnd; i < f; i++ )
        {
            Gia_ManForEachPi( p, pObj, k )
                pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
            Gia_ManForEachAnd( p, pObj, k )
                pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                               (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
            Gia_ManForEachCo( p, pObj, k )
                pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
                pObjRo->fMark0 = pObjRi->fMark0;
        }
        // start creating values
        Gia_ManForEachRo( p, pObjRo, k )
            pObjRo->Value = Abc_LitNotCond( Gia_Obj2Lit(pNew, Gia_ManPi(pNew, k)), !pObjRo->fMark0 );
        for ( i = f; i <= pCex->iFrame; i++ )
        {
            Gia_ManForEachPi( p, pObj, k )
                pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
            Gia_ManForEachAnd( p, pObj, k )
            {
                fCompl0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
                fCompl1 = Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj);
                pObj->fMark0 = fCompl0 & fCompl1;
                if ( pObj->fMark0 )
                    pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value );
                else if ( !fCompl0 && !fCompl1 )
                    pObj->Value = Gia_ManHashOr( pNew, Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value );
                else if ( !fCompl0 )
                    pObj->Value = Gia_ObjFanin0(pObj)->Value;
                else if ( !fCompl1 )
                    pObj->Value = Gia_ObjFanin1(pObj)->Value;
                else assert( 0 );
                assert( pObj->Value > 0 );
            }
            Gia_ManForEachCo( p, pObj, k )
            {
                pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
                pObj->Value = Gia_ObjFanin0(pObj)->Value;
                assert( pObj->Value > 0 );
            } 
            if ( i == pCex->iFrame )
                break;
            Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
            {
                pObjRo->fMark0 = pObjRi->fMark0;
                pObjRo->Value = pObjRi->Value;
            }
        }
        assert( iBit == pCex->nBits );
        // create PO
        Gia_ManAppendCo( pNew, Gia_ManPo(p, pCex->iPo)->Value );
    }
    Gia_ManHashStop( pNew );
    Vec_BitFree( vInitEnd );

    // cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Generates one frame.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Bmc_GiaGenerateGiaAllOne( Gia_Man_t * p, Abc_Cex_t * pCex, Vec_Bit_t ** pvInits, int iFrBeg, int iFrEnd )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObjRo, * pObjRi;
    int i, k, iBit = 0, fCompl0, fCompl1;

    // skip trough the first iFrEnd frames
    Gia_ManCleanMark0(p);
    Gia_ManForEachRo( p, pObj, k )
        pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
    *pvInits = Vec_BitStart( Gia_ManRegNum(p) );
    for ( i = 0; i < iFrEnd; i++ )
    {
        // remember values in frame iFrBeg
        if ( i == iFrBeg )
            Gia_ManForEachRo( p, pObjRo, k )
                if ( pObjRo->fMark0 )
                    Vec_BitWriteEntry( *pvInits, k, 1 );
        // simulate other values
        Gia_ManForEachPi( p, pObj, k )
            pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
        Gia_ManForEachAnd( p, pObj, k )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                           (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachCo( p, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
            pObjRo->fMark0 = pObjRi->fMark0;
    }
    assert( i == iFrEnd );

    // create new AIG manager
    pNew = Gia_ManStart( 10000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManConst0(p)->Value = 1;
    Gia_ManForEachPi( p, pObj, k )
        pObj->Value = 1;
    Gia_ManForEachRo( p, pObjRo, k )
        pObjRo->Value = Abc_LitNotCond( Gia_ManAppendCi(pNew), !pObjRo->fMark0 );
    Gia_ManHashStart( pNew );
    for ( i = iFrEnd; i <= pCex->iFrame; i++ )
    {
        Gia_ManForEachPi( p, pObj, k )
            pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
        Gia_ManForEachAnd( p, pObj, k )
        {
            fCompl0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            fCompl1 = Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj);
            pObj->fMark0 = fCompl0 & fCompl1;
            if ( pObj->fMark0 )
                pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value );
            else if ( !fCompl0 && !fCompl1 )
                pObj->Value = Gia_ManHashOr( pNew, Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value );
            else if ( !fCompl0 )
                pObj->Value = Gia_ObjFanin0(pObj)->Value;
            else if ( !fCompl1 )
                pObj->Value = Gia_ObjFanin1(pObj)->Value;
            else assert( 0 );
            assert( pObj->Value > 0 );
        }
        Gia_ManForEachCo( p, pObj, k )
        {
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            pObj->Value = Gia_ObjFanin0(pObj)->Value;
            assert( pObj->Value > 0 );
        } 
        if ( i == pCex->iFrame )
            break;
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
        {
            pObjRo->fMark0 = pObjRi->fMark0;
            pObjRo->Value = pObjRi->Value;
        }
    }
    Gia_ManHashStop( pNew );
    assert( iBit == pCex->nBits );

    // create PO
    Gia_ManAppendCo( pNew, Gia_ManPo(p, pCex->iPo)->Value );
    // cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Generate GIA for target bad states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Bmc_GiaTargetStates( Gia_Man_t * p, Abc_Cex_t * pCex, int iFrBeg, int iFrEnd, int fCombOnly, int fUseOne, int fAllFrames, int fVerbose )
{
    Gia_Man_t * pNew, * pTemp;
    Vec_Bit_t * vInitNew;

    if ( iFrBeg < 0 )
        { printf( "Starting frame is less than 0.\n" ); return NULL; }
    if ( iFrEnd < 0 )
        { printf( "Stopping frame is less than 0.\n" ); return NULL; }
    if ( iFrBeg > pCex->iFrame )
        { printf( "Starting frame is more than the last frame of CEX (%d).\n", pCex->iFrame ); return NULL; }
    if ( iFrEnd > pCex->iFrame )
        { printf( "Stopping frame is more than the last frame of CEX (%d).\n", pCex->iFrame ); return NULL; }
    if ( iFrBeg > iFrEnd )
        { printf( "Starting frame (%d) should be less than stopping frame (%d).\n", iFrBeg, iFrEnd ); return NULL; }
    assert( iFrBeg >= 0 && iFrBeg <= pCex->iFrame );
    assert( iFrEnd >= 0 && iFrEnd <= pCex->iFrame );
    assert( iFrBeg < iFrEnd );

    if ( fUseOne )
        pNew = Bmc_GiaGenerateGiaOne( p, pCex, &vInitNew, iFrBeg, iFrEnd );
    else if ( fAllFrames )
        pNew = Bmc_GiaGenerateGiaAllFrames( p, pCex, &vInitNew, iFrBeg, iFrEnd );
    else 
        pNew = Bmc_GiaGenerateGiaAllOne( p, pCex, &vInitNew, iFrBeg, iFrEnd );

    if ( !fCombOnly )
    {
        // create new GIA
        pNew = Gia_ManDupWithNewPo( p, pTemp = pNew ); 
        Gia_ManStop( pTemp );

        // create new initial state
        pNew = Gia_ManDupFlip( pTemp = pNew, Vec_BitArray(vInitNew) );
        Gia_ManStop( pTemp );
    }

    Vec_BitFree( vInitNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Generate AIG for target bad states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Bmc_AigTargetStates( Aig_Man_t * p, Abc_Cex_t * pCex, int iFrBeg, int iFrEnd, int fCombOnly, int fUseOne, int fAllFrames, int fVerbose )
{
    Aig_Man_t * pAig;
    Gia_Man_t * pGia, * pRes;
    pGia = Gia_ManFromAigSimple( p );
    if ( !Gia_ManVerifyCex( pGia, pCex, 0 ) )
    {
        Abc_Print( 1, "Current CEX does not fail AIG \"%s\".\n", p->pName ); 
        Gia_ManStop( pGia );
        return NULL;
    }
    pRes = Bmc_GiaTargetStates( pGia, pCex, iFrBeg, iFrEnd, fCombOnly, fUseOne, fAllFrames, fVerbose );
    Gia_ManStop( pGia );
    pAig = Gia_ManToAigSimple( pRes );
    Gia_ManStop( pRes );
    return pAig;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

