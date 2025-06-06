/**CFile****************************************************************

  FileName    [bmcInse.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcInse.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"
#include "aig/gia/giaAig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline void   Gia_ParTestAlloc( Gia_Man_t * p, int nWords ) { assert( !p->pData ); p->pData = (unsigned *)ABC_ALLOC(word, 2*nWords*Gia_ManObjNum(p)); p->iData = nWords; }
static inline void   Gia_ParTestFree( Gia_Man_t * p )              { ABC_FREE( p->pData ); p->iData = 0;             }
static inline word * Gia_ParTestObj( Gia_Man_t * p, int Id )       { return (word *)p->pData + Id*(p->iData << 1);   }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManInseInit( Gia_Man_t * p, Vec_Int_t * vInit )
{
    Gia_Obj_t * pObj;
    word * pData1, * pData0;
    int i, k;
    Gia_ManForEachRi( p, pObj, k )
    {
        pData0 = Gia_ParTestObj( p, Gia_ObjId(p, pObj) );
        pData1 = pData0 + p->iData;
        if ( Vec_IntEntry(vInit, k) == 0 ) // 0
            for ( i = 0; i < p->iData; i++ )
                pData0[i] = ~(word)0, pData1[i] = 0;
        else if ( Vec_IntEntry(vInit, k) == 1 ) // 1
            for ( i = 0; i < p->iData; i++ )
                pData0[i] = 0, pData1[i] = ~(word)0;
        else // if ( Vec_IntEntry(vInit, k) > 1 ) // X
            for ( i = 0; i < p->iData; i++ )
                pData0[i] = pData1[i] = 0;
    }
}
void Gia_ManInseSimulateObj( Gia_Man_t * p, int Id )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, Id );
    word * pData0, * pDataA0, * pDataB0;
    word * pData1, * pDataA1, * pDataB1;
    int i;
    if ( Gia_ObjIsAnd(pObj) )
    {
        pData0  = Gia_ParTestObj( p, Id );
        pData1  = pData0 + p->iData;
        if ( Gia_ObjFaninC0(pObj) )
        {
            pDataA1 = Gia_ParTestObj( p, Gia_ObjFaninId0(pObj, Id) );
            pDataA0 = pDataA1 + p->iData;
            if ( Gia_ObjFaninC1(pObj) )
            {
                pDataB1 = Gia_ParTestObj( p, Gia_ObjFaninId1(pObj, Id) );
                pDataB0 = pDataB1 + p->iData;
            }
            else 
            {
                pDataB0 = Gia_ParTestObj( p, Gia_ObjFaninId1(pObj, Id) );
                pDataB1 = pDataB0 + p->iData;
            }
        }
        else 
        {
            pDataA0 = Gia_ParTestObj( p, Gia_ObjFaninId0(pObj, Id) );
            pDataA1 = pDataA0 + p->iData;
            if ( Gia_ObjFaninC1(pObj) )
            {
                pDataB1 = Gia_ParTestObj( p, Gia_ObjFaninId1(pObj, Id) );
                pDataB0 = pDataB1 + p->iData;
            }
            else 
            {
                pDataB0 = Gia_ParTestObj( p, Gia_ObjFaninId1(pObj, Id) );
                pDataB1 = pDataB0 + p->iData;
            }
        }
        for ( i = 0; i < p->iData; i++ )
            pData0[i] = pDataA0[i] | pDataB0[i], pData1[i] = pDataA1[i] & pDataB1[i];
    }
    else if ( Gia_ObjIsCo(pObj) )
    {
        pData0  = Gia_ParTestObj( p, Id );
        pData1  = pData0 + p->iData;
        if ( Gia_ObjFaninC0(pObj) )
        {
            pDataA1 = Gia_ParTestObj( p, Gia_ObjFaninId0(pObj, Id) );
            pDataA0 = pDataA1 + p->iData;
        }
        else 
        {
            pDataA0 = Gia_ParTestObj( p, Gia_ObjFaninId0(pObj, Id) );
            pDataA1 = pDataA0 + p->iData;
        }
        for ( i = 0; i < p->iData; i++ )
            pData0[i] = pDataA0[i], pData1[i] = pDataA1[i];
    }
    else if ( Gia_ObjIsCi(pObj) )
    {
        if ( Gia_ObjIsPi(p, pObj) )
        {
            pData0  = Gia_ParTestObj( p, Id );
            pData1  = pData0 + p->iData;
            for ( i = 0; i < p->iData; i++ )
                pData0[i] = Gia_ManRandomW(0), pData1[i] = ~pData0[i];
        }
        else
        {
            int Id2 = Gia_ObjId(p, Gia_ObjRoToRi(p, pObj));
            pData0  = Gia_ParTestObj( p, Id );
            pData1  = pData0 + p->iData;
            pDataA0 = Gia_ParTestObj( p, Id2 );
            pDataA1 = pDataA0 + p->iData;
            for ( i = 0; i < p->iData; i++ )
                pData0[i] = pDataA0[i], pData1[i] = pDataA1[i];
        }
    }
    else if ( Gia_ObjIsConst0(pObj) )
    {
        pData0  = Gia_ParTestObj( p, Id );
        pData1  = pData0 + p->iData;
        for ( i = 0; i < p->iData; i++ )
            pData0[i] = ~(word)0, pData1[i] = 0;
    }
    else assert( 0 );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManInseHighestScore( Gia_Man_t * p, int * pCost )
{
    Gia_Obj_t * pObj;
    word * pData0, * pData1;
    int * pCounts, CountBest;
    int i, k, b, nPats, iPat;
    nPats = 64 * p->iData;
    pCounts = ABC_CALLOC( int, nPats );
    Gia_ManForEachRi( p, pObj, k )
    {
        pData0 = Gia_ParTestObj( p, Gia_ObjId(p, pObj) );
        pData1 = pData0 + p->iData;
        for ( i = 0; i < p->iData; i++ )
            for ( b = 0; b < 64; b++ )
                pCounts[64*i+b] += (((pData0[i] >> b) & 1) || ((pData1[i] >> b) & 1)); // binary
    }
    iPat = 0;
    CountBest = pCounts[0]; 
    for ( k = 1; k < nPats; k++ )
        if ( CountBest < pCounts[k] )
            CountBest = pCounts[k], iPat = k;
    *pCost = Gia_ManRegNum(p) - CountBest; // ternary
    ABC_FREE( pCounts );
    return iPat;
}
void Gia_ManInseFindStarting( Gia_Man_t * p, int iPat, Vec_Int_t * vInit, Vec_Int_t * vInputs )
{
    Gia_Obj_t * pObj;
    word * pData0, * pData1;
    int i, k;
    Vec_IntClear( vInit );
    Gia_ManForEachRi( p, pObj, k )
    {
        pData0 = Gia_ParTestObj( p, Gia_ObjId(p, pObj) );
        pData1 = pData0 + p->iData;
        for ( i = 0; i < p->iData; i++ )
            assert( (pData0[i] & pData1[i]) == 0 );
        if ( Abc_InfoHasBit( (unsigned *)pData0, iPat ) )
            Vec_IntPush( vInit, 0 );
        else if ( Abc_InfoHasBit( (unsigned *)pData1, iPat ) )
            Vec_IntPush( vInit, 1 );
        else 
            Vec_IntPush( vInit, 2 );
    }
    Gia_ManForEachPi( p, pObj, k )
    {
        pData0 = Gia_ParTestObj( p, Gia_ObjId(p, pObj) );
        pData1 = pData0 + p->iData;
        for ( i = 0; i < p->iData; i++ )
            assert( (pData0[i] & pData1[i]) == 0 );
        if ( Abc_InfoHasBit( (unsigned *)pData0, iPat ) )
            Vec_IntPush( vInputs, 0 );
        else if ( Abc_InfoHasBit( (unsigned *)pData1, iPat ) )
            Vec_IntPush( vInputs, 1 );
        else 
            Vec_IntPush( vInputs, 2 );
    }
}
Vec_Int_t * Gia_ManInseSimulate( Gia_Man_t * p, Vec_Int_t * vInit0, Vec_Int_t * vInputs, Vec_Int_t * vInit )
{
    Vec_Int_t * vRes;
    Gia_Obj_t * pObj, * pObjRo, * pObjRi;
    int nFrames = Vec_IntSize(vInputs) / Gia_ManPiNum(p);
    int i, f, iBit = 0;
    assert( Vec_IntSize(vInputs) % Gia_ManPiNum(p) == 0 );
    assert( Vec_IntSize(vInit0) == Gia_ManRegNum(p) );
    assert( Vec_IntSize(vInit) == Gia_ManRegNum(p) );
    Gia_ManConst0(p)->fMark0 = 0;
    Gia_ManForEachRi( p, pObj, i )
        pObj->fMark0 = Vec_IntEntry(vInit0, i);
    for ( f = 0; f < nFrames; f++ )
    {
        Gia_ManForEachPi( p, pObj, i )
            pObj->fMark0 = Vec_IntEntry(vInputs, iBit++);
        Gia_ManForEachAnd( p, pObj, i )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                           (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachRi( p, pObj, i )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, i )
            pObjRo->fMark0 = pObjRi->fMark0;
    }
    assert( iBit == Vec_IntSize(vInputs) );
    vRes = Vec_IntAlloc( Gia_ManRegNum(p) );
    Gia_ManForEachRo( p, pObj, i )
        assert( Vec_IntEntry(vInit, i) == 2 || Vec_IntEntry(vInit, i) == (int)pObj->fMark0 );
    Gia_ManForEachRo( p, pObj, i )
        Vec_IntPush( vRes, pObj->fMark0 | (Vec_IntEntry(vInit, i) != 2 ? 4 : 0) );
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = 0;
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManInsePerform( Gia_Man_t * p, Vec_Int_t * vInit0, int nFrames, int nWords, int fVerbose )
{
    Vec_Int_t * vRes, * vInit, * vInputs;
    Gia_Obj_t * pObj;
    int i, f, iPat, Cost, Cost0;
    abctime clk, clkTotal = Abc_Clock();
    Gia_ManRandomW( 1 );
    if ( fVerbose )
        printf( "Running with %d frames, %d words, and %sgiven init state.\n", nFrames, nWords, vInit0 ? "":"no " );
    vInit = Vec_IntAlloc(0); Vec_IntFill( vInit, Gia_ManRegNum(p), 2 );
    vInputs = Vec_IntStart( Gia_ManPiNum(p) * nFrames );
    Gia_ParTestAlloc( p, nWords );
    Gia_ManInseInit( p, vInit );
    Cost0 = 0;
    Vec_IntForEachEntry( vInit, iPat, i )
        Cost0 += ((iPat >> 1) & 1);
    if ( fVerbose )
        printf( "Frame =%6d : Values =%6d (out of %6d)\n", 0, Cost0, Cost0 );
    for ( f = 0; f < nFrames; f++ )
    {
        clk = Abc_Clock();
        Gia_ManForEachObj( p, pObj, i )
            Gia_ManInseSimulateObj( p, i );
        iPat = Gia_ManInseHighestScore( p, &Cost );
        Gia_ManInseFindStarting( p, iPat, vInit, vInputs );
        Gia_ManInseInit( p, vInit );
        if ( fVerbose )
            printf( "Frame =%6d : Values =%6d (out of %6d)   ", f+1, Cost, Cost0 );
        if ( fVerbose )
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    Gia_ParTestFree( p );
    vRes = Gia_ManInseSimulate( p, vInit0, vInputs, vInit );
    Vec_IntFreeP( &vInit );
    Vec_IntFreeP( &vInputs );
    printf( "After %d frames, found a sequence to produce %d x-values (out of %d).  ", f, Cost, Gia_ManRegNum(p) );
    Abc_PrintTime( 1, "Total runtime", Abc_Clock() - clkTotal );
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_ManInseTest( Gia_Man_t * p, Vec_Int_t * vInit0, int nFrames, int nWords, int nTimeOut, int fSim, int fVerbose )
{
    Vec_Int_t * vRes, * vInit;
    vInit = Vec_IntAlloc(0); Vec_IntFill( vInit, Gia_ManRegNum(p), 0 );
    vRes = Gia_ManInsePerform( p, vInit, nFrames, nWords, fVerbose );
    if ( vInit != vInit0 )
        Vec_IntFree( vInit );
    return vRes;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

