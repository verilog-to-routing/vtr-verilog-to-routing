/**CFile****************************************************************

  FileName    [bmcCexTools.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [CEX analysis and optimization toolbox.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcCexTools.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline word Bmc_CexBitMask( int iBit )                                    { assert( iBit < 64 ); return ~(((word)1) << iBit); }
static inline void Bmc_CexCopySim( Vec_Wrd_t * vSims, int iObjTo, int iObjFrom ) { Vec_WrdWriteEntry( vSims, iObjTo, iObjFrom );     }
static inline void Bmc_CexAndSim( Vec_Wrd_t * vSims, int iObjTo, int i, int j )  { Vec_WrdWriteEntry( vSims, iObjTo, Vec_WrdEntry(vSims, i) & Vec_WrdEntry(vSims, j) ); }
static inline void Bmc_CexOrSim( Vec_Wrd_t * vSims, int iObjTo, int i, int j )   { Vec_WrdWriteEntry( vSims, iObjTo, Vec_WrdEntry(vSims, i) | Vec_WrdEntry(vSims, j) ); }
static inline int  Bmc_CexSim( Vec_Wrd_t * vSims, int iObj, int i )              { return (Vec_WrdEntry(vSims, iObj) >> i) & 1;      }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmc_CexBitCount( Abc_Cex_t * p, int nInputs )
{
    int k, Counter = 0, Counter2 = 0;
    if ( p == NULL )
    {
        printf( "The counter example is NULL.\n" );
        return -1;
    }
    for ( k = 0; k < p->nBits; k++ )
    {
        Counter += Abc_InfoHasBit(p->pData, k);
        if ( (k - p->nRegs) % p->nPis < nInputs )
            Counter2 += Abc_InfoHasBit(p->pData, k);
    }
    return Counter2;
}
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_CexDumpStats( Gia_Man_t * p, Abc_Cex_t * pCex, Abc_Cex_t * pCexCare, Abc_Cex_t * pCexEss, Abc_Cex_t * pCexMin, abctime clk )
{
    int nInputs    = Gia_ManPiNum(p);
    int nBitsTotal = (pCex->iFrame + 1) * nInputs;
    int nBitsCare  = Bmc_CexBitCount(pCexCare, nInputs);
    int nBitsDC    = nBitsTotal - nBitsCare;
    int nBitsEss   = Bmc_CexBitCount(pCexEss, nInputs);
    int nBitsOpt   = nBitsCare - nBitsEss;
    int nBitsMin   = Bmc_CexBitCount(pCexMin, nInputs);

    FILE * pTable  = fopen( "cex/stats.txt", "a+" );
    fprintf( pTable, "%s ", p->pName );
    fprintf( pTable, "%d ", nInputs );
    fprintf( pTable, "%d ", Gia_ManRegNum(p) );
    fprintf( pTable, "%d ", pCex->iFrame + 1 );
    fprintf( pTable, "%d ", nBitsTotal );
    fprintf( pTable, "%.2f ", 100.0 * nBitsDC  / nBitsTotal );
    fprintf( pTable, "%.2f ", 100.0 * nBitsOpt / nBitsTotal );
    fprintf( pTable, "%.2f ", 100.0 * nBitsEss / nBitsTotal );
    fprintf( pTable, "%.2f ", 100.0 * nBitsMin / nBitsTotal );
    fprintf( pTable, "%.2f ", 1.0*clk/(CLOCKS_PER_SEC) );
    fprintf( pTable, "\n" );
    fclose( pTable );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_CexDumpAogStats( Gia_Man_t * p, abctime clk )
{
    FILE * pTable  = fopen( "cex/aig_stats.txt", "a+" );
    fprintf( pTable, "%s ", p->pName );
    fprintf( pTable, "%d ", Gia_ManPiNum(p) );
    fprintf( pTable, "%d ", Gia_ManAndNum(p) );
    fprintf( pTable, "%d ", Gia_ManLevelNum(p) );
    fprintf( pTable, "%.2f ", 1.0*clk/(CLOCKS_PER_SEC) );
    fprintf( pTable, "\n" );
    fclose( pTable );
}

/**Function*************************************************************

  Synopsis    [Performs initialized unrolling till the last frame.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Bmc_CexPerformUnrolling( Gia_Man_t * p, Abc_Cex_t * pCex )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObjRo, * pObjRi;
    int i, k;
    assert( Gia_ManRegNum(p) > 0 );
    pNew = Gia_ManStart( (pCex->iFrame + 1) * Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachRi( p, pObj, k )
        pObj->Value = 0;
    Gia_ManHashAlloc( pNew );
    for ( i = 0; i <= pCex->iFrame; i++ )
    {
        Gia_ManForEachPi( p, pObj, k )
            pObj->Value = Gia_ManAppendCi( pNew );
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
            pObjRo->Value = pObjRi->Value;
        Gia_ManForEachAnd( p, pObj, k )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachCo( p, pObj, k )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
    }
    Gia_ManHashStop( pNew );
    pObj = Gia_ManPo(p, pCex->iPo);
    Gia_ManAppendCo( pNew, pObj->Value );
    // cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
void Bmc_CexPerformUnrollingTest( Gia_Man_t * p, Abc_Cex_t * pCex )
{
    Gia_Man_t * pNew;
    abctime clk = Abc_Clock();
    pNew = Bmc_CexPerformUnrolling( p, pCex );
    Gia_ManPrintStats( pNew, NULL );
    Gia_AigerWrite( pNew, "unroll.aig", 0, 0, 0 );
//Bmc_CexDumpAogStats( pNew, Abc_Clock() - clk );
    Gia_ManStop( pNew );
    printf( "CE-induced network is written into file \"unroll.aig\".\n" );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}


/**Function*************************************************************

  Synopsis    [Computes CE-induced network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Bmc_CexBuildNetwork( Gia_Man_t * p, Abc_Cex_t * pCex )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObjRo, * pObjRi;
    int fCompl0, fCompl1;
    int i, k, iBit = pCex->nRegs;
    // start the manager
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "unate" );
    // set const0
    Gia_ManConst0(p)->fMark0 = 0;
    Gia_ManConst0(p)->fMark1 = 1;
    Gia_ManConst0(p)->Value  = ~0;
    // set init state
    Gia_ManForEachRi( p, pObj, k )
    {
        pObj->fMark0 = 0;
        pObj->fMark1 = 1;
        pObj->Value  = ~0;
    }
    Gia_ManHashAlloc( pNew );
    for ( i = 0; i <= pCex->iFrame; i++ )
    {
        //  primary inputs
        Gia_ManForEachPi( p, pObj, k )
        {
            pObj->fMark0 = Abc_InfoHasBit( pCex->pData, iBit++ );
            pObj->fMark1 = 0;
            pObj->Value  = Gia_ManAppendCi(pNew);
        }
        // transfer 
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
        {
            pObjRo->fMark0 = pObjRi->fMark0;
            pObjRo->fMark1 = pObjRi->fMark1;
            pObjRo->Value  = pObjRi->Value;
        }
        // internal nodes
        Gia_ManForEachAnd( p, pObj, k )
        {
            fCompl0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            fCompl1 = Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj);
            pObj->fMark0 = fCompl0 & fCompl1;
            if ( pObj->fMark0 )
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1 & Gia_ObjFanin1(pObj)->fMark1;
            else if ( !fCompl0 && !fCompl1 )
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1 | Gia_ObjFanin1(pObj)->fMark1;
            else if ( !fCompl0 )
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1;
            else if ( !fCompl1 )
                pObj->fMark1 = Gia_ObjFanin1(pObj)->fMark1;
            else assert( 0 );
            pObj->Value = ~0;
            if ( pObj->fMark1 )
                continue;
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
        // combinational outputs
        Gia_ManForEachCo( p, pObj, k )
        {
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1;
            pObj->Value  = Gia_ObjFanin0(pObj)->Value;
        }
    }
    Gia_ManHashStop( pNew );
    assert( iBit == pCex->nBits );
    // create primary output
    pObj = Gia_ManPo(p, pCex->iPo);
    assert( pObj->fMark0 == 1 );
    assert( pObj->fMark1 == 0 );
    assert( pObj->Value > 0 );
    Gia_ManAppendCo( pNew, pObj->Value );
    // cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
void Bmc_CexBuildNetworkTest( Gia_Man_t * p, Abc_Cex_t * pCex )
{
    Gia_Man_t * pNew;
    abctime clk = Abc_Clock();
    pNew = Bmc_CexBuildNetwork( p, pCex );
    Gia_ManPrintStats( pNew, NULL );
    Gia_AigerWrite( pNew, "unate.aig", 0, 0, 0 );
//Bmc_CexDumpAogStats( pNew, Abc_Clock() - clk );
    Gia_ManStop( pNew );
    printf( "CE-induced network is written into file \"unate.aig\".\n" );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}


/**Function*************************************************************

  Synopsis    [Prints one counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_CexPrint( Abc_Cex_t * pCex, int nRealPis, int fVerbose )
{
    int i, k, Count, iBit = pCex->nRegs;
    Abc_CexPrintStatsInputs( pCex, nRealPis );
    if ( !fVerbose )
        return;

    for ( i = 0; i <= pCex->iFrame; i++ )
    {
        Count = 0;
        printf( "%3d : ", i );
        for ( k = 0; k < nRealPis; k++ )
        {
            Count += Abc_InfoHasBit(pCex->pData, iBit);
            printf( "%d", Abc_InfoHasBit(pCex->pData, iBit++) );
        }
        printf( " %3d ", Count );
        Count = 0;
        for (      ; k < pCex->nPis; k++ )
        {
            Count += Abc_InfoHasBit(pCex->pData, iBit);
            printf( "%d", Abc_InfoHasBit(pCex->pData, iBit++) );
        }
        printf( " %3d\n", Count );
    }
    assert( iBit == pCex->nBits );
}

/**Function*************************************************************

  Synopsis    [Verifies the care set of the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmc_CexVerify( Gia_Man_t * p, Abc_Cex_t * pCex, Abc_Cex_t * pCexCare )
{
    Gia_Obj_t * pObj;
    int i, k;
//    assert( pCex->nRegs > 0 );
//    assert( pCexCare->nRegs == 0 );
    Gia_ObjTerSimSet0( Gia_ManConst0(p) );
    Gia_ManForEachRi( p, pObj, k )
        Gia_ObjTerSimSet0( pObj );
    for ( i = 0; i <= pCex->iFrame; i++ )
    {
        Gia_ManForEachPi( p, pObj, k )
        {
            if ( !Abc_InfoHasBit( pCexCare->pData, pCexCare->nRegs + i * pCexCare->nPis + k ) )
                Gia_ObjTerSimSetX( pObj );
            else if ( Abc_InfoHasBit( pCex->pData, pCex->nRegs + i * pCex->nPis + k ) )
                Gia_ObjTerSimSet1( pObj );
            else
                Gia_ObjTerSimSet0( pObj );
        }
        Gia_ManForEachRo( p, pObj, k )
            Gia_ObjTerSimRo( p, pObj );
        Gia_ManForEachAnd( p, pObj, k )
            Gia_ObjTerSimAnd( pObj );
        Gia_ManForEachCo( p, pObj, k )
            Gia_ObjTerSimCo( pObj );
    }
    pObj = Gia_ManPo( p, pCex->iPo );
    return Gia_ObjTerSimGet1(pObj);
}

/**Function*************************************************************

  Synopsis    [Verifies the care set of the counter-example for an arbitrary PO.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmc_CexVerifyAnyPo( Gia_Man_t * p, Abc_Cex_t * pCex, Abc_Cex_t * pCexCare )
{
    Gia_Obj_t * pObj;
    int i, k;
//    assert( pCex->nRegs > 0 );
//    assert( pCexCare->nRegs == 0 );
    Gia_ObjTerSimSet0( Gia_ManConst0(p) );
    Gia_ManForEachRi( p, pObj, k )
        Gia_ObjTerSimSet0( pObj );
    for ( i = 0; i <= pCex->iFrame; i++ )
    {
        Gia_ManForEachPi( p, pObj, k )
        {
            if ( !Abc_InfoHasBit( pCexCare->pData, pCexCare->nRegs + i * pCexCare->nPis + k ) )
                Gia_ObjTerSimSetX( pObj );
            else if ( Abc_InfoHasBit( pCex->pData, pCex->nRegs + i * pCex->nPis + k ) )
                Gia_ObjTerSimSet1( pObj );
            else
                Gia_ObjTerSimSet0( pObj );
        }
        Gia_ManForEachRo( p, pObj, k )
            Gia_ObjTerSimRo( p, pObj );
        Gia_ManForEachAnd( p, pObj, k )
            Gia_ObjTerSimAnd( pObj );
        Gia_ManForEachCo( p, pObj, k )
            Gia_ObjTerSimCo( pObj );
    }
    for (i = 0; i < Gia_ManPoNum(p) - Gia_ManConstrNum(p); i++)
    {
        pObj = Gia_ManPo( p, i );
        if (Gia_ObjTerSimGet1(pObj))
            return i;
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Computes internal states of the CEX.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Bmc_CexInnerStates( Gia_Man_t * p, Abc_Cex_t * pCex, Abc_Cex_t ** ppCexImpl, int fVerbose )
{
    Abc_Cex_t * pNew, * pNew2;
    Gia_Obj_t * pObj, * pObjRo, * pObjRi;
    int fCompl0, fCompl1;
    int i, k, iBit = 0;
    assert( pCex->nRegs > 0 );
    // start the counter-example
    pNew = Abc_CexAlloc( 0, Gia_ManCiNum(p), pCex->iFrame + 1 );
    pNew->iFrame = pCex->iFrame;
    pNew->iPo    = pCex->iPo;
    // start the counter-example
    pNew2 = Abc_CexAlloc( 0, Gia_ManCiNum(p), pCex->iFrame + 1 );
    pNew2->iFrame = pCex->iFrame;
    pNew2->iPo    = pCex->iPo;
    // set initial state
    Gia_ManCleanMark01(p);
    // set const0
    Gia_ManConst0(p)->fMark0 = 0;
    Gia_ManConst0(p)->fMark1 = 1;
    // set init state
    Gia_ManForEachRi( p, pObjRi, k )
    {
        pObjRi->fMark0 = 0;
        pObjRi->fMark1 = 1;
    }
    iBit = pCex->nRegs;
    for ( i = 0; i <= pCex->iFrame; i++ )
    {
        Gia_ManForEachPi( p, pObj, k )
            pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
        {
            pObjRo->fMark0 = pObjRi->fMark0;
            pObjRo->fMark1 = pObjRi->fMark1;
        }
        Gia_ManForEachCi( p, pObj, k )
        {
            if ( pObj->fMark0 )
                Abc_InfoSetBit( pNew->pData, pNew->nPis * i + k );
            if ( pObj->fMark1 )
                Abc_InfoSetBit( pNew2->pData, pNew2->nPis * i + k );
        }
        Gia_ManForEachAnd( p, pObj, k )
        {
            fCompl0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            fCompl1 = Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj);
            pObj->fMark0 = fCompl0 & fCompl1;
            if ( pObj->fMark0 )
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1 & Gia_ObjFanin1(pObj)->fMark1;
            else if ( !fCompl0 && !fCompl1 )
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1 | Gia_ObjFanin1(pObj)->fMark1;
            else if ( !fCompl0 )
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1;
            else if ( !fCompl1 )
                pObj->fMark1 = Gia_ObjFanin1(pObj)->fMark1;
            else assert( 0 );
        }
        Gia_ManForEachCo( p, pObj, k )
        {
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1;
        }

/*
        // print input/state/output
        printf( "%3d : ", i );
        Gia_ManForEachPi( p, pObj, k )
            printf( "%d", pObj->fMark0 );
        printf( " " );
        Gia_ManForEachRo( p, pObj, k )
            printf( "%d", pObj->fMark0 );
        printf( " " );
        Gia_ManForEachPo( p, pObj, k )
            printf( "%d", pObj->fMark0 );
        printf( "\n" );
*/
    }
    assert( iBit == pCex->nBits );
    assert( Gia_ManPo(p, pCex->iPo)->fMark0 == 1 );

    printf( "Inner states: " );
    Bmc_CexPrint( pNew, Gia_ManPiNum(p), fVerbose );
    printf( "Implications: " );
    Bmc_CexPrint( pNew2, Gia_ManPiNum(p), fVerbose );
    if ( ppCexImpl )
        *ppCexImpl = pNew2;
    else
        Abc_CexFree( pNew2 );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Computes care bits of the CEX.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_CexCareBits_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    int fCompl0, fCompl1;
    if ( Gia_ObjIsConst0(pObj) )
        return;
    if ( pObj->fMark1 )
        return;
    pObj->fMark1 = 1;
    if ( Gia_ObjIsCi(pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    fCompl0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
    fCompl1 = Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj);
    if ( pObj->fMark0 )
    {
        assert( fCompl0 == 1 && fCompl1 == 1 );
        Bmc_CexCareBits_rec( p, Gia_ObjFanin0(pObj) );
        Bmc_CexCareBits_rec( p, Gia_ObjFanin1(pObj) );
    }
    else
    {
        assert( fCompl0 == 0 || fCompl1 == 0 );
        if ( fCompl0 == 0 )
            Bmc_CexCareBits_rec( p, Gia_ObjFanin0(pObj) );
        if ( fCompl1 == 0 )
            Bmc_CexCareBits_rec( p, Gia_ObjFanin1(pObj) );
    }
}
void Bmc_CexCareBits2_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    int fCompl0, fCompl1;
    if ( Gia_ObjIsConst0(pObj) )
        return;
    if ( pObj->fMark1 )
        return;
    pObj->fMark1 = 1;
    if ( Gia_ObjIsCi(pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    fCompl0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
    fCompl1 = Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj);
    if ( pObj->fMark0 )
    {
        assert( fCompl0 == 1 && fCompl1 == 1 );
        Bmc_CexCareBits2_rec( p, Gia_ObjFanin0(pObj) );
        Bmc_CexCareBits2_rec( p, Gia_ObjFanin1(pObj) );
    }
    else
    {
        assert( fCompl0 == 0 || fCompl1 == 0 );
        if ( fCompl0 == 0 )
            Bmc_CexCareBits2_rec( p, Gia_ObjFanin0(pObj) );
        /**/ 
        else 
        /**/ 
        if ( fCompl1 == 0 )
            Bmc_CexCareBits2_rec( p, Gia_ObjFanin1(pObj) );
    }
}
Abc_Cex_t * Bmc_CexCareBits( Gia_Man_t * p, Abc_Cex_t * pCexState, Abc_Cex_t * pCexImpl, Abc_Cex_t * pCexEss, int fFindAll, int fVerbose )
{
    Abc_Cex_t * pNew;
    Gia_Obj_t * pObj;
    int fCompl0, fCompl1;
    int i, k, iBit;
    assert( pCexState->nRegs == 0 );
    // start the counter-example
    pNew = Abc_CexAlloc( 0, Gia_ManCiNum(p), pCexState->iFrame + 1 );
    pNew->iFrame = pCexState->iFrame;
    pNew->iPo    = pCexState->iPo;
    // set initial state
    Gia_ManCleanMark01(p);
    // set const0
    Gia_ManConst0(p)->fMark0 = 0;
    Gia_ManConst0(p)->fMark1 = 1;
    for ( i = pCexState->iFrame; i >= 0; i-- )
    {
        // set correct values
        iBit = pCexState->nPis * i;
        Gia_ManForEachCi( p, pObj, k )
        {
            pObj->fMark0 = Abc_InfoHasBit(pCexState->pData, iBit+k);
            pObj->fMark1 = 0;
            if ( pCexImpl )
                pObj->fMark1 |= Abc_InfoHasBit(pCexImpl->pData, iBit+k);
            if ( pCexEss )
                pObj->fMark1 |= Abc_InfoHasBit(pCexEss->pData, iBit+k);
        }
        Gia_ManForEachAnd( p, pObj, k )
        {
            fCompl0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            fCompl1 = Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj);
            pObj->fMark0 = fCompl0 & fCompl1;
            if ( pObj->fMark0 )
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1 & Gia_ObjFanin1(pObj)->fMark1;
            else if ( !fCompl0 && !fCompl1 )
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1 | Gia_ObjFanin1(pObj)->fMark1;
            else if ( !fCompl0 )
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1;
            else if ( !fCompl1 )
                pObj->fMark1 = Gia_ObjFanin1(pObj)->fMark1;
            else assert( 0 );
        }
        Gia_ManForEachCo( p, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
        // move values from COs to CIs
        if ( i == pCexState->iFrame )
        {
            assert( Gia_ManPo(p, pCexState->iPo)->fMark0 == 1 );
            if ( fFindAll )
                Bmc_CexCareBits_rec( p, Gia_ObjFanin0(Gia_ManPo(p, pCexState->iPo)) );
            else
                Bmc_CexCareBits2_rec( p, Gia_ObjFanin0(Gia_ManPo(p, pCexState->iPo)) );
        }
        else
        {
            Gia_ManForEachRi( p, pObj, k )
                if ( Abc_InfoHasBit(pNew->pData, pNew->nPis * (i+1) + Gia_ManPiNum(p) + k) )
                {
                     if ( fFindAll )
                         Bmc_CexCareBits_rec( p, Gia_ObjFanin0(pObj) );
                     else
                         Bmc_CexCareBits2_rec( p, Gia_ObjFanin0(pObj) );
                }
        }
        // save the results
        Gia_ManForEachCi( p, pObj, k )
            if ( pObj->fMark1 )
            {
                pObj->fMark1 = 0;
                if ( pCexImpl == NULL || !Abc_InfoHasBit(pCexImpl->pData, pNew->nPis * i + k) )
                    Abc_InfoSetBit(pNew->pData, pNew->nPis * i + k);
            }
    }
    if ( pCexEss )
        printf( "Minimized:    " );
    else
        printf( "Care bits:    " );
    Bmc_CexPrint( pNew, Gia_ManPiNum(p), fVerbose );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Simulates one bit to check whether it is essential.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Bmc_CexEssentialBitOne( Gia_Man_t * p, Abc_Cex_t * pCexState, int iBit, Abc_Cex_t * pCexPrev, int * pfEqual )
{
    Abc_Cex_t * pNew;
    Gia_Obj_t * pObj;
    int i, k, fCompl0, fCompl1;
    assert( pCexState->nRegs == 0 );
    assert( iBit < pCexState->nBits );
    if ( pfEqual )
        *pfEqual = 0;
    // start the counter-example
    pNew = Abc_CexAllocFull( 0, Gia_ManCiNum(p), pCexState->iFrame + 1 );
    pNew->iFrame = pCexState->iFrame;
    pNew->iPo    = pCexState->iPo;
    // clean bit
    Abc_InfoXorBit( pNew->pData, iBit );
    // simulate the remaining frames
    Gia_ManConst0(p)->fMark0 = 0;
    Gia_ManConst0(p)->fMark1 = 1;
    for ( i = iBit / pCexState->nPis; i <= pCexState->iFrame; i++ )
    {
        Gia_ManForEachCi( p, pObj, k )
        {
            pObj->fMark0 = Abc_InfoHasBit( pCexState->pData, i * pCexState->nPis + k );
            pObj->fMark1 = Abc_InfoHasBit( pNew->pData,      i * pCexState->nPis + k );
        }
        Gia_ManForEachAnd( p, pObj, k )
        {
            fCompl0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            fCompl1 = Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj);
            pObj->fMark0 = fCompl0 & fCompl1;
            if ( pObj->fMark0 )
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1 & Gia_ObjFanin1(pObj)->fMark1;
            else if ( !fCompl0 && !fCompl1 )
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1 | Gia_ObjFanin1(pObj)->fMark1;
            else if ( !fCompl0 )
                pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1;
            else if ( !fCompl1 )
                pObj->fMark1 = Gia_ObjFanin1(pObj)->fMark1;
            else assert( 0 );
        }
        Gia_ManForEachCo( p, pObj, k )
        {
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            pObj->fMark1 = Gia_ObjFanin0(pObj)->fMark1;
        }
        if ( i < pCexState->iFrame )
        {
            int fChanges  = 0;
            int fEqual    = (pCexPrev != NULL);
            int iBitShift = (i + 1) * pCexState->nPis + Gia_ManPiNum(p);
            Gia_ManForEachRi( p, pObj, k )
            {
                if ( fEqual && pCexPrev && (int)pObj->fMark1 != Abc_InfoHasBit(pCexPrev->pData, iBitShift + k) )
                    fEqual = 0;
                if ( !pObj->fMark1 )
                {
                    fChanges = 1;
                    Abc_InfoXorBit( pNew->pData, iBitShift + k );
                }
            }
            if ( !fChanges || fEqual )
            {
                if ( pfEqual )
                    *pfEqual = fEqual;
                Abc_CexFree( pNew );
                return NULL;
            }
        }
/*
        if ( i == 20 )
        {
            printf( "Frame %d : ", i );
            Gia_ManForEachRi( p, pObj, k )
                printf( "%d", pObj->fMark1 );
            printf( "\n" );
        }
*/
    }
    return pNew;
}
void Bmc_CexEssentialBitTest( Gia_Man_t * p, Abc_Cex_t * pCexState )
{
    Abc_Cex_t * pNew;
    int b;
    for ( b = 0; b < pCexState->nBits; b++ )
    {
        if ( b % 100 )
            continue;
        pNew = Bmc_CexEssentialBitOne( p, pCexState, b, NULL, NULL );
        Bmc_CexPrint( pNew, Gia_ManPiNum(p), 0 );

        if ( Gia_ManPo(p, pCexState->iPo)->fMark1 )
            printf( "Not essential\n" );
        else
            printf( "Essential\n" );

        Abc_CexFree( pNew );
    }
}

/**Function*************************************************************

  Synopsis    [Computes essential bits of the CEX.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Bmc_CexEssentialBits( Gia_Man_t * p, Abc_Cex_t * pCexState, Abc_Cex_t * pCexCare, int fVerbose )
{
    Abc_Cex_t * pNew, * pTemp, * pPrev = NULL;
    int b, fEqual = 0, fPrevStatus = 0;
//    abctime clk = Abc_Clock();
    assert( pCexState->nBits == pCexCare->nBits );
    // start the counter-example
    pNew = Abc_CexAlloc( 0, Gia_ManCiNum(p), pCexState->iFrame + 1 );
    pNew->iFrame = pCexState->iFrame;
    pNew->iPo    = pCexState->iPo;
    // iterate through care-bits
    for ( b = 0; b < pCexState->nBits; b++ )
    {
        // skip don't-care bits
        if ( !Abc_InfoHasBit(pCexCare->pData, b) )
            continue;

        // skip state bits
        if ( b % pCexCare->nPis >= Gia_ManPiNum(p) )
        {
            Abc_InfoSetBit( pNew->pData, b );
            continue;
        }

        // check if this is an essential bit
        pTemp = Bmc_CexEssentialBitOne( p, pCexState, b, pPrev, &fEqual );
//        pTemp = Bmc_CexEssentialBitOne( p, pCexState, b, NULL, &fEqual );
        if ( pTemp == NULL )
        {
            if ( fEqual && fPrevStatus )
                Abc_InfoSetBit( pNew->pData, b );
            continue;
        }
//        Bmc_CexPrint( pTemp, Gia_ManPiNum(p), fVerbose );
        Abc_CexFree( pPrev );
        pPrev = pTemp;

        // record essential bit
        fPrevStatus = !Gia_ManPo(p, pCexState->iPo)->fMark1;
        if ( !Gia_ManPo(p, pCexState->iPo)->fMark1 )
            Abc_InfoSetBit( pNew->pData, b );
    }
    Abc_CexFreeP( &pPrev );
//    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    printf( "Essentials:   " );
    Bmc_CexPrint( pNew, Gia_ManPiNum(p), fVerbose );
    return pNew;
}
 

/**Function*************************************************************

  Synopsis    [Computes essential bits of the CEX.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bmc_CexTest( Gia_Man_t * p, Abc_Cex_t * pCex, int fVerbose )
{
    abctime clk = Abc_Clock();
    Abc_Cex_t * pCexImpl   = NULL;
    Abc_Cex_t * pCexStates = Bmc_CexInnerStates( p, pCex, &pCexImpl, fVerbose );
    Abc_Cex_t * pCexCare   = Bmc_CexCareBits( p, pCexStates, pCexImpl, NULL, 1, fVerbose );
    Abc_Cex_t * pCexEss, * pCexMin;

    if ( !Bmc_CexVerify( p, pCex, pCexCare ) )
        printf( "Counter-example care-set verification has failed.\n" );

    pCexEss = Bmc_CexEssentialBits( p, pCexStates, pCexCare, fVerbose );
    pCexMin = Bmc_CexCareBits( p, pCexStates, pCexImpl, pCexEss, 0, fVerbose );

    if ( !Bmc_CexVerify( p, pCex, pCexMin ) )
        printf( "Counter-example min-set verification has failed.\n" );

//    Bmc_CexDumpStats( p, pCex, pCexCare, pCexEss, pCexMin, Abc_Clock() - clk );

    Abc_CexFreeP( &pCexStates );
    Abc_CexFreeP( &pCexImpl );
    Abc_CexFreeP( &pCexCare );
    Abc_CexFreeP( &pCexEss );
    Abc_CexFreeP( &pCexMin );

    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
//    Bmc_CexBuildNetworkTest( p, pCex );
//    Bmc_CexPerformUnrollingTest( p, pCex );
}



/**Function*************************************************************

  Synopsis    [Count the number of care bits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCountCareBits( Gia_Man_t * p, Vec_Wec_t * vPats )
{
    Gia_Obj_t * pObj;
    int i, k, fCompl0, fCompl1;
    word Counter = 0;
    Vec_Int_t * vPat;
    Vec_WecForEachLevel( vPats, vPat, i )
    {
        int Count = 0;
        assert( Vec_IntSize(vPat) == Gia_ManCiNum(p) );

        // propagate forward
        Gia_ManConst0(p)->fMark0 = 0;
        Gia_ManConst0(p)->fMark1 = 0;
        Gia_ManForEachCi( p, pObj, k )
        {
            pObj->fMark0 = Vec_IntEntry(vPat, k);
            pObj->fMark1 = 0;
        }
        Gia_ManForEachAnd( p, pObj, k )
        {
            fCompl0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            fCompl1 = Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj);
            pObj->fMark0 = fCompl0 & fCompl1;
            pObj->fMark1 = 0;
        }
        Gia_ManForEachCo( p, pObj, k )
        {
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            Gia_ObjFanin0(pObj)->fMark1 = 1;
        }
        // propagate backward
        Gia_ManForEachAndReverse( p, pObj, k )
        {
            if ( !pObj->fMark1 )
                continue;
            if ( pObj->fMark0 == 0 )
            {
                fCompl0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
                fCompl1 = Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj);
                assert( fCompl0 == 0 || fCompl1 == 0 );
                if ( fCompl1 == 0 )
                    Gia_ObjFanin1(pObj)->fMark1 = 1;
                else if ( fCompl0 == 0 )
                    Gia_ObjFanin0(pObj)->fMark1 = 1;
                
            }
            else
            {
                Gia_ObjFanin0(pObj)->fMark1 = 1;
                Gia_ObjFanin1(pObj)->fMark1 = 1;
            }
        }
        Gia_ManForEachAnd( p, pObj, k )
            Count += pObj->fMark1;
        Counter += Count;

        //printf( "%3d = %8d\n", i, Count );
    }
    Counter /= Vec_WecSize(vPats);
    printf( "Stats over %d patterns: Average care-nodes = %d (%6.2f %%)\n", Vec_WecSize(vPats), (int)Counter, 100.0*(int)Counter/Gia_ManAndNum(p) );
}

unsigned char * Mnist_ReadImages1_()
{
    int Size = 60000 * 28 * 28 + 16;
    unsigned char * pData = (unsigned char *)malloc( Size );
    FILE * pFile = fopen( "train-images.idx3-ubyte", "rb" );
    int RetValue = fread( pData, 1, Size, pFile );
    assert( RetValue == Size );
    fclose( pFile );
    return pData;
}
Vec_Wec_t * Mnist_ReadImages_( int nPats )
{
    Vec_Wec_t * vPats = Vec_WecStart( nPats );
    unsigned char * pL1 = Mnist_ReadImages1_();
    int i, k, x;
    for ( i = 0; i < nPats; i++ )
        for ( x = 0; x < 784; x++ )
            for ( k = 0; k < 8; k++ )
                Vec_WecPush( vPats, i, (pL1[16 + i * 784 + x] >> k) & 1 );
    free( pL1 );
    return vPats;
}
void Gia_ManCountCareBitsTest( Gia_Man_t * p )
{
    Vec_Wec_t * vPats = Mnist_ReadImages_( 100 );
    Gia_ManCountCareBits( p, vPats );
    Vec_WecFree( vPats );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

