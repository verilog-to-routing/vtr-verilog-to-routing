/**CFile****************************************************************

  FileName    [bmcChain.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Implementation of chain BMC.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcChain.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "aig/gia/giaAig.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Find the first failure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Bmc_ChainFailOneOutput( Gia_Man_t * p, int nFrameMax, int nConfMax, int fVerbose, int fVeryVerbose )
{
    int RetValue;
    Abc_Cex_t * pCex = NULL;
    Aig_Man_t * pAig = Gia_ManToAigSimple( p );
    Saig_ParBmc_t Pars, * pPars = &Pars;
    Saig_ParBmcSetDefaultParams( pPars );
    pPars->nFramesMax = nFrameMax;
    pPars->nConfLimit = nConfMax;
    pPars->fVerbose   = fVeryVerbose;
    RetValue = Saig_ManBmcScalable( pAig, pPars );
    if ( RetValue == 0 ) // SAT
    {
        pCex = pAig->pSeqModel, pAig->pSeqModel = NULL;
        if ( fVeryVerbose )
            Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d.\n", pCex->iPo, p->pName, pCex->iFrame );
    }
    else if ( fVeryVerbose )
        Abc_Print( 1, "No output asserted in %d frames. Resource limit reached.\n", pPars->iFrame+2 );
    Aig_ManStop( pAig );
    return pCex;
}

/**Function*************************************************************

  Synopsis    [Move GIA into the failing state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupWithInit( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
        {
            pObj->Value = Gia_ManAppendCi( pNew );
            pObj->Value = Abc_LitNotCond( pObj->Value, pObj->fMark0 );
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            pObj->Value = Gia_ObjFanin0Copy(pObj);
            pObj->Value = Abc_LitNotCond( pObj->Value, pObj->fMark0 );
            pObj->Value = Gia_ManAppendCo( pNew, pObj->Value );
        }
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}
Gia_Man_t * Gia_ManVerifyCexAndMove( Gia_Man_t * pGia, Abc_Cex_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int RetValue, i, k, iBit = 0;
    Gia_ManCleanMark0(pGia);
    Gia_ManForEachRo( pGia, pObj, i )
        pObj->fMark0 = Abc_InfoHasBit(p->pData, iBit++);
    for ( i = 0; i <= p->iFrame; i++ )
    {
        Gia_ManForEachPi( pGia, pObj, k )
            pObj->fMark0 = Abc_InfoHasBit(p->pData, iBit++);
        Gia_ManForEachAnd( pGia, pObj, k )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                           (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachCo( pGia, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
        if ( i == p->iFrame )
            break;
        Gia_ManForEachRiRo( pGia, pObjRi, pObjRo, k )
            pObjRo->fMark0 = pObjRi->fMark0;
    }
    assert( iBit == p->nBits );
    RetValue = Gia_ManPo(pGia, p->iPo)->fMark0;
    assert( RetValue );
    // set PI/PO values to zero and transfer RO values to RI
    Gia_ManForEachPi( pGia, pObj, k )
        pObj->fMark0 = 0;
    Gia_ManForEachPo( pGia, pObj, k )
        pObj->fMark0 = 0;
    Gia_ManForEachRiRo( pGia, pObjRi, pObjRo, k )
        pObjRi->fMark0 = pObjRo->fMark0;
    // duplicate assuming CI/CO marks are set correctly
    pNew = Gia_ManDupWithInit( pGia );
    Gia_ManCleanMark0(pGia);
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Find what outputs fail in this state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupPosAndPropagateInit( Gia_Man_t * p )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;  int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsPi(p, pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = 0;
        else if ( Gia_ObjIsPo(p, pObj) )
            Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManHashStop( pNew );
    assert( Gia_ManPiNum(p) == Gia_ManPiNum(pNew) );
    assert( Gia_ManPoNum(p) == Gia_ManPoNum(pNew) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}
sat_solver * Gia_ManDeriveSatSolver( Gia_Man_t * p, Vec_Int_t * vSatIds )
{
    sat_solver * pSat;
    Aig_Man_t * pAig = Gia_ManToAigSimple( p );
//    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 0, 0, 0 );
    Cnf_Dat_t * pCnf = Cnf_Derive( pAig, Aig_ManCoNum(pAig) );
    // save SAT vars for the primary inputs
    if ( vSatIds )
    {
        Aig_Obj_t * pObj; int i;
        Vec_IntClear( vSatIds );
        Aig_ManForEachCi( pAig, pObj, i )
            Vec_IntPush( vSatIds, pCnf->pVarNums[ Aig_ObjId(pObj) ] );
        assert( Vec_IntSize(vSatIds) == Gia_ManPiNum(p) );
    }
    Aig_ManStop( pAig );
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    Cnf_DataFree( pCnf );
    assert( p->nRegs == 0 );
    return pSat;
}
Vec_Int_t * Bmc_ChainFindFailedOutputs( Gia_Man_t * p, Vec_Ptr_t * vCexes )
{
    Vec_Int_t * vOutputs;
    Vec_Int_t * vSatIds;
    Gia_Man_t * pInit;
    Gia_Obj_t * pObj;
    sat_solver * pSat;
    int i, j, Lit, status = 0;
    // derive output logic cones
    pInit = Gia_ManDupPosAndPropagateInit( p );
    // derive SAT solver and test
    vSatIds = Vec_IntAlloc( Gia_ManPiNum(p) );
    pSat = Gia_ManDeriveSatSolver( pInit, vSatIds );
    vOutputs = Vec_IntAlloc( 100 );
    Gia_ManForEachPo( pInit, pObj, i )
    {
        if ( Gia_ObjFaninLit0p(pInit, pObj) == 0 )
            continue;
        Lit = Abc_Var2Lit( i+1, 0 );
        status = sat_solver_solve( pSat, &Lit, &Lit + 1, 0, 0, 0, 0 );
        if ( status == l_True )
        {
            // save the index of solved output
            Vec_IntPush( vOutputs, i );
            // create CEX for this output
            if ( vCexes )
            {
                Abc_Cex_t * pCex = Abc_CexAlloc( Gia_ManRegNum(p), Gia_ManPiNum(p), 1 );
                pCex->iFrame = 0;
                pCex->iPo = i;
                for ( j = 0; j < Gia_ManPiNum(p); j++ )
                    if ( sat_solver_var_value( pSat, Vec_IntEntry(vSatIds, j) ) )
                        Abc_InfoSetBit( pCex->pData, Gia_ManRegNum(p) + j );
                Vec_PtrPush( vCexes, pCex );
            }
        }
    }
    Gia_ManStop( pInit );
    sat_solver_delete( pSat );
    Vec_IntFree( vSatIds );
    return vOutputs;
}

/**Function*************************************************************

  Synopsis    [Cleanup AIG by removing COs replacing failed out by const0.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Gia_ObjMakePoConst0( Gia_Man_t * p, Gia_Obj_t * pObj )
{ 
    assert( Gia_ObjIsCo(pObj) );
    pObj->iDiff0 = Gia_ObjId( p, pObj );
    pObj->fCompl0 = 0;
}
int Gia_ManCountNonConst0( Gia_Man_t * p )
{ 
    Gia_Obj_t * pObj;
    int i, Count = 0;
    Gia_ManForEachPo( p, pObj, i )
        Count += (Gia_ObjFaninLit0p(p, pObj) != 0);
    return Count;
}
Gia_Man_t * Bmc_ChainCleanup( Gia_Man_t * p, Vec_Int_t * vOutputs )
{
    int i, iOut;
    Vec_IntForEachEntry( vOutputs, iOut, i )
    {
        Gia_Obj_t * pObj = Gia_ManPo( p, iOut );
        assert( Gia_ObjFaninLit0p(p, pObj) != 0 );
        Gia_ObjMakePoConst0( p, pObj );
        assert( Gia_ObjFaninLit0p(p, pObj) == 0 );
    }
    return Gia_ManCleanup( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bmc_ChainTest( Gia_Man_t * p, int nFrameMax, int nConfMax, int fVerbose, int fVeryVerbose, Vec_Ptr_t ** pvCexes )
{
    int Iter, IterMax = 10000;
    Gia_Man_t * pTemp, * pNew = Gia_ManDup(p);
    Abc_Cex_t * pCex = NULL;
    Vec_Int_t * vOutputs;
    abctime clk2, clk = Abc_Clock(); 
    abctime clkBmc = 0;
    abctime clkMov = 0;
    abctime clkSat = 0;
    abctime clkCln = 0;
    abctime clkSwp = 0;
    int DepthTotal = 0;
    if ( pvCexes )
        *pvCexes = Vec_PtrAlloc( 1000 );
    for ( Iter = 0; Iter < IterMax; Iter++ )
    {
        if ( Gia_ManPoNum(pNew) == 0 )
        {
            if ( fVerbose )
                printf( "Finished all POs.\n" );
            break;
        }
        // run BMC till the first failure
        clk2 = Abc_Clock(); 
        pCex = Bmc_ChainFailOneOutput( pNew, nFrameMax, nConfMax, fVerbose, fVeryVerbose );
        clkBmc += Abc_Clock() - clk2;
        if ( pCex == NULL )
        {
            if ( fVerbose )
                printf( "BMC could not detect a failed output in %d frames with %d conflicts.\n", nFrameMax, nConfMax );
            break;
        }
        assert( !Iter || pCex->iFrame > 0 );
        // move the AIG to the failure state
        clk2 = Abc_Clock(); 
        pNew = Gia_ManVerifyCexAndMove( pTemp = pNew, pCex );
        Gia_ManStop( pTemp );
        DepthTotal += pCex->iFrame;
        clkMov += Abc_Clock() - clk2;
        // find outputs that fail in this state
        clk2 = Abc_Clock(); 
        vOutputs = Bmc_ChainFindFailedOutputs( pNew, pvCexes ? *pvCexes : NULL );
        assert( Vec_IntFind(vOutputs, pCex->iPo) >= 0 );
        // save the counter-example
        if ( pvCexes )
            Vec_PtrPush( *pvCexes, pCex );
        else
            Abc_CexFree( pCex );
        clkSat += Abc_Clock() - clk2;
        // remove them from the AIG 
        clk2 = Abc_Clock(); 
        pNew = Bmc_ChainCleanup( pTemp = pNew, vOutputs );
        Gia_ManStop( pTemp );
        clkCln += Abc_Clock() - clk2;
        // perform sequential cleanup
        clk2 = Abc_Clock(); 
//        pNew = Gia_ManSeqStructSweep( pTemp = pNew, 0, 1, 0 );
//        Gia_ManStop( pTemp );
        clkSwp += Abc_Clock() - clk2;
        // printout
        if ( fVerbose )
        {
            int nNonConst = Gia_ManCountNonConst0(pNew);
            printf( "Iter %4d : ",    Iter+1 );
            printf( "Depth =%5d  ",   DepthTotal );
            printf( "FailPo =%5d  ",  Vec_IntSize(vOutputs) );
            printf( "UndecPo =%5d ",  nNonConst );
            printf( "(%6.2f %%)  ",   100.0 * nNonConst / Gia_ManPoNum(pNew) );
            printf( "AIG =%8d  ",     Gia_ManAndNum(pNew) );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        }
        Vec_IntFree( vOutputs );
    }
    printf( "Completed a CEX chain with %d segments, %d frames, and %d failed POs (out of %d). ", 
        Iter, DepthTotal, Gia_ManPoNum(pNew) - Gia_ManCountNonConst0(pNew), Gia_ManPoNum(pNew) );
    if ( fVerbose )
    {
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Abc_PrintTimeP( 1, "BMC  ", clkBmc, Abc_Clock() - clk );
    Abc_PrintTimeP( 1, "Init ", clkMov, Abc_Clock() - clk );
    Abc_PrintTimeP( 1, "SAT  ", clkSat, Abc_Clock() - clk );
    Abc_PrintTimeP( 1, "Clean", clkCln, Abc_Clock() - clk );
//    Abc_PrintTimeP( 1, "Sweep", clkSwp, Abc_Clock() - clk );
    }
    Gia_ManStop( pNew );
    if ( pvCexes )
        printf( "Total number of CEXes collected = %d.\n", Vec_PtrSize(*pvCexes) );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

