/**CFile****************************************************************

  FileName    [bmcCexDepth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [CEX depth minimization.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcCexDepth.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Abc_Cex_t * Bmc_CexInnerStates( Gia_Man_t * p, Abc_Cex_t * pCex, Abc_Cex_t ** ppCexImpl, int fVerbose );
extern Abc_Cex_t * Bmc_CexCareBits( Gia_Man_t * p, Abc_Cex_t * pCexState, Abc_Cex_t * pCexImpl, Abc_Cex_t * pCexEss, int fFindAll, int fVerbose );
extern int Bmc_CexVerify( Gia_Man_t * p, Abc_Cex_t * pCex, Abc_Cex_t * pCexCare );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs targe enlargement of the given size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Bmc_CexTargetEnlarge( Gia_Man_t * p, int nFrames )
{
    Gia_Man_t * pNew, * pOne;
    Gia_Obj_t * pObj, * pObjRo;
    int i, k;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    for ( k = 0; k < nFrames; k++ )
        Gia_ManForEachPi( p, pObj, i )
            Gia_ManAppendCi( pNew );
    Gia_ManForEachRo( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    for ( k = 0; k < nFrames; k++ )
    {
        Gia_ManForEachPi( p, pObj, i )
            pObj->Value = Gia_ManCiLit( pNew, (nFrames - 1 - k) * Gia_ManPiNum(p) + i );
        Gia_ManForEachAnd( p, pObj, i )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachRi( p, pObj, i )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
        Gia_ManForEachRiRo( p, pObj, pObjRo, i )
            pObjRo->Value  = pObj->Value;
    }
    pObj = Gia_ManPo( p, 0 );
    pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pOne = pNew );
    Gia_ManStop( pOne );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Create target with quantified inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Bmc_CexTarget( Gia_Man_t * p, int nFrames )
{
    Gia_Man_t * pNew, * pTemp;
    int i, Limit = nFrames * Gia_ManPiNum(p);
    pNew = Bmc_CexTargetEnlarge( p, nFrames );
    for ( i = 0; i < Limit; i++ )
    {
        printf( "%3d : ", i );
        if ( i % Gia_ManPiNum(p) == 0 )
            Gia_ManPrintStats( pNew, NULL );
        pNew = Gia_ManDupExist( pTemp = pNew, i );
        Gia_ManStop( pTemp );
    }
    Gia_ManPrintStats( pNew, NULL );
    pNew = Gia_ManDupLastPis( pTemp = pNew, Gia_ManRegNum(p) );
    Gia_ManStop( pTemp );  
    Gia_ManPrintStats( pNew, NULL );
    pTemp = Gia_ManDupAppendCones( p, &pNew, 1, 1 );
    Gia_ManStop( pNew );
    Gia_AigerWrite( pTemp, "miter3.aig", 0, 0, 0 );
    return pTemp;
}

/**Function*************************************************************

  Synopsis    [Computes CE-induced network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Bmc_CexBuildNetwork2( Gia_Man_t * p, Abc_Cex_t * pCex, int fStart )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObjRo, * pObjRi;
    int fCompl0, fCompl1;
    int i, k, iBit;
    assert( pCex->nRegs == 0 );
    assert( pCex->nPis == Gia_ManCiNum(p) );
    assert( fStart <= pCex->iFrame );
    // start the manager
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "unate" );
    // set const0
    Gia_ManConst0(p)->fMark0 = 0; // value
    Gia_ManConst0(p)->fMark1 = 1; // care
    Gia_ManConst0(p)->Value  = ~0;
    // set init state
    Gia_ManForEachRi( p, pObj, k )
    {
        pObj->fMark0 = Abc_InfoHasBit( pCex->pData, pCex->nRegs + fStart * Gia_ManCiNum(p) + Gia_ManPiNum(p) + k );
        pObj->fMark1 = 0;
        pObj->Value  = Abc_LitNotCond( Gia_ManAppendCi(pNew), !pObj->fMark0 );
    }
    Gia_ManHashAlloc( pNew );
    iBit = pCex->nRegs + fStart * Gia_ManCiNum(p);
    for ( i = fStart; i <= pCex->iFrame; i++ )
    {
        //  primary inputs
        Gia_ManForEachPi( p, pObj, k )
        {
            pObj->fMark0 = Abc_InfoHasBit( pCex->pData, iBit++ );
            pObj->fMark1 = 1;
            pObj->Value  = ~0;
        }
        iBit += Gia_ManRegNum(p);
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
Gia_Man_t * Bmc_CexBuildNetwork2_( Gia_Man_t * p, Abc_Cex_t * pCex, int fStart )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pObjRo, * pObjRi;
    int fCompl0, fCompl1;
    int i, k, iBit;
    assert( pCex->nRegs == 0 );
    assert( pCex->nPis == Gia_ManCiNum(p) );
    assert( fStart <= pCex->iFrame );
    // start the manager
    pNew = Gia_ManStart( 1000 );
    pNew->pName = Abc_UtilStrsav( "unate" );
    // set const0
    Gia_ManConst0(p)->fMark0 = 0; // value
    Gia_ManConst0(p)->Value  = 1;
    // set init state
    Gia_ManForEachRi( p, pObj, k )
    {
        pObj->fMark0 = Abc_InfoHasBit( pCex->pData, pCex->nRegs + fStart * Gia_ManCiNum(p) + Gia_ManPiNum(p) + k );
        pObj->Value  = Abc_LitNotCond( Gia_ManAppendCi(pNew), !pObj->fMark0 );
    }
    Gia_ManHashAlloc( pNew );
    iBit = pCex->nRegs + fStart * Gia_ManCiNum(p);
    for ( i = fStart; i <= pCex->iFrame; i++ )
    {
        //  primary inputs
        Gia_ManForEachPi( p, pObj, k )
        {
            pObj->fMark0 = Abc_InfoHasBit( pCex->pData, iBit++ );
            pObj->Value  = 1;
        }
        iBit += Gia_ManRegNum(p);
        // transfer 
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
        {
            pObjRo->fMark0 = pObjRi->fMark0;
            pObjRo->Value  = pObjRi->Value;
        }
        // internal nodes
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
        }
        // combinational outputs
        Gia_ManForEachCo( p, pObj, k )
        {
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
            pObj->Value  = Gia_ObjFanin0(pObj)->Value;
        }
    }
    Gia_ManHashStop( pNew );
    assert( iBit == pCex->nBits );
    // create primary output
    pObj = Gia_ManPo(p, pCex->iPo);
    assert( pObj->fMark0 == 1 );
    assert( pObj->Value > 0 );
    Gia_ManAppendCo( pNew, pObj->Value );
    // cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

Gia_Man_t * Bmc_CexBuildNetwork2Test( Gia_Man_t * p, Abc_Cex_t * pCex, int nFramesMax )
{
    Gia_Man_t * pNew, * pTemp;
    Vec_Ptr_t * vCones;
    abctime clk = Abc_Clock();
    int i;
    nFramesMax = Abc_MinInt( nFramesMax, pCex->iFrame );
    printf( "Processing CEX in frame %d (max frames %d).\n", pCex->iFrame, nFramesMax );
    vCones = Vec_PtrAlloc( nFramesMax );
    for ( i = pCex->iFrame; i > pCex->iFrame - nFramesMax; i-- )
    {
        printf( "Frame %5d : ", i );
        pNew = Bmc_CexBuildNetwork2_( p, pCex, i );
        Gia_ManPrintStats( pNew, NULL );
        Vec_PtrPush( vCones, pNew );
    }
    pNew = Gia_ManDupAppendCones( p, (Gia_Man_t **)Vec_PtrArray(vCones), Vec_PtrSize(vCones), 1 );
    Gia_AigerWrite( pNew, "miter2.aig", 0, 0, 0 );
//Bmc_CexDumpAogStats( pNew, Abc_Clock() - clk );
    Vec_PtrForEachEntry( Gia_Man_t *, vCones, pTemp, i )
        Gia_ManStop( pTemp );
    Vec_PtrFree( vCones );
    printf( "GIA with additional properties is written into \"miter2.aig\".\n" );
//    printf( "CE-induced network is written into file \"unate.aig\".\n" );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
//    Gia_ManStop( pNew );
    return pNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Bmc_CexDepthTest( Gia_Man_t * p, Abc_Cex_t * pCex, int nFrames, int fVerbose )
{
    Gia_Man_t * pNew;
    abctime clk = Abc_Clock();
    Abc_Cex_t * pCexImpl   = NULL;
    Abc_Cex_t * pCexStates = Bmc_CexInnerStates( p, pCex, &pCexImpl, fVerbose );
    Abc_Cex_t * pCexCare   = Bmc_CexCareBits( p, pCexStates, pCexImpl, NULL, 1, fVerbose );
//    Abc_Cex_t * pCexEss, * pCexMin;

    if ( !Bmc_CexVerify( p, pCex, pCexCare ) )
        printf( "Counter-example care-set verification has failed.\n" );

//    pCexEss = Bmc_CexEssentialBits( p, pCexStates, pCexCare, fVerbose );
//    pCexMin = Bmc_CexCareBits( p, pCexStates, pCexImpl, pCexEss, 0, fVerbose );

//    if ( !Bmc_CexVerify( p, pCex, pCexMin ) )
//        printf( "Counter-example min-set verification has failed.\n" );

//    Bmc_CexDumpStats( p, pCex, pCexCare, pCexEss, pCexMin, Abc_Clock() - clk );

    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    pNew = Bmc_CexBuildNetwork2Test( p, pCexStates, nFrames );
//    Bmc_CexPerformUnrollingTest( p, pCex );

    Abc_CexFreeP( &pCexStates );
    Abc_CexFreeP( &pCexImpl );
    Abc_CexFreeP( &pCexCare );
//    Abc_CexFreeP( &pCexEss );
//    Abc_CexFreeP( &pCexMin );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

