/**CFile****************************************************************

  FileName    [giaAbs.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Counter-example-guided abstraction refinement.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaAbs.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#include "gia.h"

#include "sat/bsat/satSolver.h"
#include "sat/cnf/cnf.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Resimulates the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManVerifyCex( Gia_Man_t * pAig, Abc_Cex_t * p, int fDualOut )
{
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int RetValue, i, k, iBit = 0;
    Gia_ManCleanMark0(pAig);
    Gia_ManForEachRo( pAig, pObj, i )
        pObj->fMark0 = Abc_InfoHasBit(p->pData, iBit++);
    for ( i = 0; i <= p->iFrame; i++ )
    {
        Gia_ManForEachPi( pAig, pObj, k )
            pObj->fMark0 = Abc_InfoHasBit(p->pData, iBit++);
        Gia_ManForEachAnd( pAig, pObj, k )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                           (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachCo( pAig, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
        if ( i == p->iFrame )
            break;
        Gia_ManForEachRiRo( pAig, pObjRi, pObjRo, k )
        {
            pObjRo->fMark0 = pObjRi->fMark0;
        }
    }
    assert( iBit == p->nBits );
    if ( fDualOut )
        RetValue = Gia_ManPo(pAig, 2*p->iPo)->fMark0 ^ Gia_ManPo(pAig, 2*p->iPo+1)->fMark0;
    else
        RetValue = Gia_ManPo(pAig, p->iPo)->fMark0;
    Gia_ManCleanMark0(pAig);
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Resimulates the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManFindFailedPoCex( Gia_Man_t * pAig, Abc_Cex_t * p, int nOutputs )
{
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int RetValue, i, k, iBit = 0;
    assert( Gia_ManPiNum(pAig) == p->nPis );
    Gia_ManCleanMark0(pAig);
    Gia_ManForEachRo( pAig, pObj, i )
        pObj->fMark0 = Abc_InfoHasBit(p->pData, iBit++);
    iBit = p->nRegs;
    for ( i = 0; i <= p->iFrame; i++ )
    {
        Gia_ManForEachPi( pAig, pObj, k )
            pObj->fMark0 = Abc_InfoHasBit(p->pData, iBit++);
        Gia_ManForEachAnd( pAig, pObj, k )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                           (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachCo( pAig, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
        Gia_ManForEachRiRo( pAig, pObjRi, pObjRo, k )
            pObjRo->fMark0 = pObjRi->fMark0;
    }
    assert( iBit == p->nBits );
    // figure out the number of failed output
    RetValue = -1;
//    for ( i = Gia_ManPoNum(pAig) - 1; i >= nOutputs; i-- )
    for ( i = nOutputs; i < Gia_ManPoNum(pAig); i++ )
    {
        if ( Gia_ManPo(pAig, i)->fMark0 )
        {
            RetValue = i;
            break;
        }
    }
    Gia_ManCleanMark0(pAig);
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Determines the failed PO when its exact frame is not known.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSetFailedPoCex( Gia_Man_t * pAig, Abc_Cex_t * p )
{
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int i, k, iBit = 0;
    assert( Gia_ManPiNum(pAig) == p->nPis );
    Gia_ManCleanMark0(pAig);
    p->iPo = -1;
//    Gia_ManForEachRo( pAig, pObj, i )
//       pObj->fMark0 = Abc_InfoHasBit(p->pData, iBit++);
    iBit = p->nRegs;
    for ( i = 0; i <= p->iFrame; i++ )
    {
        Gia_ManForEachPi( pAig, pObj, k )
            pObj->fMark0 = Abc_InfoHasBit(p->pData, iBit++);
        Gia_ManForEachAnd( pAig, pObj, k )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & 
                           (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachCo( pAig, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
        Gia_ManForEachRiRo( pAig, pObjRi, pObjRo, k )
            pObjRo->fMark0 = pObjRi->fMark0;
        // check the POs
        Gia_ManForEachPo( pAig, pObj, k )
        {
            if ( !pObj->fMark0 )
                continue;
            p->iPo = k;
            p->iFrame = i;
            p->nBits = iBit;
            break;
        }
    }
    Gia_ManCleanMark0(pAig);
    return p->iPo;
}


/**Function*************************************************************

  Synopsis    [Starts the process of returning values for internal nodes.]

  Description [Should be called when pCex is available, before probing 
  any object for its value using Gia_ManCounterExampleValueLookup().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCounterExampleValueStart( Gia_Man_t * pGia, Abc_Cex_t * pCex )
{
    Gia_Obj_t * pObj, * pObjRi, * pObjRo;
    int Val0, Val1, nObjs, i, k, iBit = 0;
    assert( Gia_ManRegNum(pGia) > 0 ); // makes sense only for sequential AIGs
    assert( pGia->pData2 == NULL );    // if this fail, there may be a memory leak
    // allocate memory to store simulation bits for internal nodes
    pGia->pData2 = ABC_CALLOC( unsigned, Abc_BitWordNum( (pCex->iFrame + 1) * Gia_ManObjNum(pGia) ) );
    // the register values in the counter-example should be zero
    Gia_ManForEachRo( pGia, pObj, k )
        assert( Abc_InfoHasBit(pCex->pData, iBit++) == 0 );
    // iterate through the timeframes
    nObjs = Gia_ManObjNum(pGia);
    for ( i = 0; i <= pCex->iFrame; i++ )
    {
        // no need to set constant-0 node
        // set primary inputs according to the counter-example
        Gia_ManForEachPi( pGia, pObj, k )
            if ( Abc_InfoHasBit(pCex->pData, iBit++) )
                Abc_InfoSetBit( (unsigned *)pGia->pData2, nObjs * i + Gia_ObjId(pGia, pObj) );
        // compute values for each node
        Gia_ManForEachAnd( pGia, pObj, k )
        {
            Val0 = Abc_InfoHasBit( (unsigned *)pGia->pData2, nObjs * i + Gia_ObjFaninId0p(pGia, pObj) );
            Val1 = Abc_InfoHasBit( (unsigned *)pGia->pData2, nObjs * i + Gia_ObjFaninId1p(pGia, pObj) );
            if ( (Val0 ^ Gia_ObjFaninC0(pObj)) & (Val1 ^ Gia_ObjFaninC1(pObj)) )
                Abc_InfoSetBit( (unsigned *)pGia->pData2, nObjs * i + Gia_ObjId(pGia, pObj) );
        }
        // derive values for combinational outputs
        Gia_ManForEachCo( pGia, pObj, k )
        {
            Val0 = Abc_InfoHasBit( (unsigned *)pGia->pData2, nObjs * i + Gia_ObjFaninId0p(pGia, pObj) );
            if ( Val0 ^ Gia_ObjFaninC0(pObj) )
                Abc_InfoSetBit( (unsigned *)pGia->pData2, nObjs * i + Gia_ObjId(pGia, pObj) );
        }
        if ( i == pCex->iFrame )
            continue;
        // transfer values to the register output of the next frame
        Gia_ManForEachRiRo( pGia, pObjRi, pObjRo, k )
            if ( Abc_InfoHasBit( (unsigned *)pGia->pData2, nObjs * i + Gia_ObjId(pGia, pObjRi) ) )
                Abc_InfoSetBit( (unsigned *)pGia->pData2, nObjs * (i+1) + Gia_ObjId(pGia, pObjRo) );
    }
    assert( iBit == pCex->nBits );
    // check that the counter-example is correct, that is, the corresponding output is asserted
    assert( Abc_InfoHasBit( (unsigned *)pGia->pData2, nObjs * pCex->iFrame + Gia_ObjId(pGia, Gia_ManCo(pGia, pCex->iPo)) ) );
}

/**Function*************************************************************

  Synopsis    [Stops the process of returning values for internal nodes.]

  Description [Should be called when probing is no longer needed]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCounterExampleValueStop( Gia_Man_t * pGia )
{
    assert( pGia->pData2 != NULL );    // if this fail, we try to call this procedure more than once
    ABC_FREE( pGia->pData2 );
    pGia->pData2 = NULL;
}

/**Function*************************************************************

  Synopsis    [Returns the value of the given object in the given timeframe.]

  Description [Should be called to probe the value of an object with 
  the given ID (iFrame is a 0-based number of a time frame - should not 
  exceed the number of timeframes in the original counter-example).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCounterExampleValueLookup(  Gia_Man_t * pGia, int Id, int iFrame )
{
    assert( Id >= 0 && Id < Gia_ManObjNum(pGia) );
    return Abc_InfoHasBit( (unsigned *)pGia->pData2, Gia_ManObjNum(pGia) * iFrame + Id );
}

/**Function*************************************************************

  Synopsis    [Procedure to test the above code.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManCounterExampleValueTest( Gia_Man_t * pGia, Abc_Cex_t * pCex )
{
    Gia_Obj_t * pObj = Gia_ManObj( pGia, Gia_ManObjNum(pGia)/2 );
    int iFrame = Abc_MaxInt( 0, pCex->iFrame - 1 );
    printf( "\nUsing counter-example, which asserts output %d in frame %d.\n", pCex->iPo, pCex->iFrame );
    Gia_ManCounterExampleValueStart( pGia, pCex );
    printf( "Value of object %d in frame %d is %d.\n", Gia_ObjId(pGia, pObj), iFrame,
        Gia_ManCounterExampleValueLookup(pGia, Gia_ObjId(pGia, pObj), iFrame) );
    Gia_ManCounterExampleValueStop( pGia );
}


/**Function*************************************************************

  Synopsis    [Returns CEX containing PI+CS values for each timeframe.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Gia_ManCexExtendToIncludeCurrentStates( Gia_Man_t * p, Abc_Cex_t * pCex )
{
    Abc_Cex_t * pNew;
    Gia_Obj_t * pObj, * pObjRo, * pObjRi;
    int i, k, iBit = 0;
    assert( pCex->nRegs > 0 );
    // start the counter-example
    pNew = Abc_CexAlloc( 0, Gia_ManCiNum(p), pCex->iFrame + 1 );
    pNew->iFrame = pCex->iFrame;
    pNew->iPo    = pCex->iPo;
    // set const0
    Gia_ManConst0(p)->fMark0 = 0;
    // set init state
    Gia_ManForEachRi( p, pObjRi, k )
        pObjRi->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
    assert( iBit == pCex->nRegs );
    for ( i = 0; i <= pCex->iFrame; i++ )
    {
        Gia_ManForEachPi( p, pObj, k )
            pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
            pObjRo->fMark0 = pObjRi->fMark0;
        Gia_ManForEachCi( p, pObj, k )
            if ( pObj->fMark0 )
                Abc_InfoSetBit( pNew->pData, pNew->nPis * i + k );
        Gia_ManForEachAnd( p, pObj, k )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachCo( p, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
    }
    assert( iBit == pCex->nBits );
    assert( Gia_ManPo(p, pCex->iPo)->fMark0 == 1 );
    Gia_ManCleanMark0(p);
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Returns CEX containing all object valuess for each timeframe.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Gia_ManCexExtendToIncludeAllObjects( Gia_Man_t * p, Abc_Cex_t * pCex )
{
    Abc_Cex_t * pNew;
    Gia_Obj_t * pObj, * pObjRo, * pObjRi;
    int i, k, iBit = 0;
    assert( pCex->nRegs > 0 );
    // start the counter-example
    pNew = Abc_CexAlloc( 0, Gia_ManObjNum(p), pCex->iFrame + 1 );
    pNew->iFrame = pCex->iFrame;
    pNew->iPo    = pCex->iPo;
    // set const0
    Gia_ManConst0(p)->fMark0 = 0;
    // set init state
    Gia_ManForEachRi( p, pObjRi, k )
        pObjRi->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
    assert( iBit == pCex->nRegs );
    for ( i = 0; i <= pCex->iFrame; i++ )
    {
        Gia_ManForEachPi( p, pObj, k )
            pObj->fMark0 = Abc_InfoHasBit(pCex->pData, iBit++);
        Gia_ManForEachRiRo( p, pObjRi, pObjRo, k )
            pObjRo->fMark0 = pObjRi->fMark0;
        Gia_ManForEachObj( p, pObj, k )
            if ( pObj->fMark0 )
                Abc_InfoSetBit( pNew->pData, pNew->nPis * i + k );
        Gia_ManForEachAnd( p, pObj, k )
            pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        Gia_ManForEachCo( p, pObj, k )
            pObj->fMark0 = Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj);
    }
    assert( iBit == pCex->nBits );
    assert( Gia_ManPo(p, pCex->iPo)->fMark0 == 1 );
    Gia_ManCleanMark0(p);
    return pNew;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManFramesForCexMin( Gia_Man_t * p, int nFrames )
{
    Gia_Man_t * pFrames, * pTemp;
    Gia_Obj_t * pObj; int i, f;
    assert( Gia_ManPoNum(p) == 1 );
    pFrames = Gia_ManStart( Gia_ManObjNum(p) );
    pFrames->pName = Abc_UtilStrsav( p->pName );
    pFrames->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pFrames );
    Gia_ManConst0(p)->Value = 0;
    for ( f = 0; f < nFrames; f++ )
    {
        Gia_ManForEachRo( p, pObj, i )
            pObj->Value = f ? Gia_ObjRoToRi( p, pObj )->Value : 0;
        Gia_ManForEachPi( p, pObj, i )
            pObj->Value = Gia_ManAppendCi( pFrames );
        Gia_ManForEachAnd( p, pObj, i )
            pObj->Value = Gia_ManHashAnd( pFrames, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachRi( p, pObj, i )
            pObj->Value = Gia_ObjFanin0Copy(pObj);
    }
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManAppendCo( pFrames, Gia_ObjFanin0Copy(pObj) );
    pFrames = Gia_ManCleanup( pTemp = pFrames );
    //printf( "Before cleanup = %d nodes. After cleanup = %d nodes.\n", 
    //    Gia_ManAndNum(pTemp), Gia_ManAndNum(pFrames) );
    Gia_ManStop( pTemp );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManMinCex( Gia_Man_t * p, Abc_Cex_t * pCex )
{
    abctime clk = Abc_Clock();
    int n, i, iFirstVar, iLit, status, Counter = 0;//, Id;
    Vec_Int_t * vLits;
    sat_solver * pSat;
    Cnf_Dat_t * pCnf;
    int nFinal, * pFinal;
    Abc_Cex_t * pCexCare;
    Gia_Man_t * pFrames;

    // CEX minimization
    clk = Abc_Clock();
    pCexCare = Bmc_CexCareMinimizeAig( p, Gia_ManPiNum(p), pCex, 1, 1, 1 );
    for ( i = pCexCare->nRegs; i < pCexCare->nBits; i++ )
        Counter += Abc_InfoHasBit(pCexCare->pData, i);
    Abc_CexFree( pCexCare );
    printf( "Care bits = %d. ", Counter );
    Abc_PrintTime( 1, "CEX minimization", Abc_Clock() - clk );

    // SAT instance
    clk = Abc_Clock();
    pFrames = Gia_ManFramesForCexMin( p, pCex->iFrame + 1 );
    pCnf = (Cnf_Dat_t*)Mf_ManGenerateCnf( pFrames, 8, 0, 0, 0, 0 );
    iFirstVar = pCnf->nVars - (pCex->iFrame+1) * pCex->nPis;
    pSat = (sat_solver*)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    iLit = Abc_Var2Lit( 1, 1 );
    status = sat_solver_addclause( pSat, &iLit, &iLit + 1 );
    assert( status );
    // create literals
    vLits = Vec_IntAlloc( 100 );
    for ( i = pCex->nRegs; i < pCex->nBits; i++ )
        Vec_IntPush( vLits, Abc_Var2Lit(iFirstVar + i - pCex->nRegs, !Abc_InfoHasBit(pCex->pData, i)) );
    Abc_PrintTime( 1, "SAT solver", Abc_Clock() - clk );

    for ( n = 0; n < 2; n++ )
    {
        if ( n ) Vec_IntReverseOrder( vLits );

        // SAT-based minimization
        clk = Abc_Clock();
        status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntLimit(vLits), 0, 0, 0, 0 );
        nFinal = sat_solver_final( pSat, &pFinal );
        printf( "Status %d.  Selected %d assumptions out of %d.  ", status, nFinal, Vec_IntSize(vLits) );
        Abc_PrintTime( 1, "Analyze_final", Abc_Clock() - clk );

        // SAT-based minimization
        clk = Abc_Clock();
        nFinal = sat_solver_minimize_assumptions( pSat, Vec_IntArray(vLits), Vec_IntSize(vLits), 0 );
        printf( "Status %d.  Selected %d assumptions out of %d.  ", status, nFinal, Vec_IntSize(vLits) );
        Abc_PrintTime( 1, "LEXUNSAT", Abc_Clock() - clk );
    }

    // cleanup
    Vec_IntFree( vLits );
    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    Gia_ManStop( pFrames );
}


Abc_Cex_t * Bmc_CexCareDeriveCex( Abc_Cex_t * pCex, int iFirstVar, int * pLits, int nLits )
{
    Abc_Cex_t * pCexMin; int i;
    pCexMin = Abc_CexAlloc( pCex->nRegs, pCex->nPis, pCex->iFrame + 1 );
    pCexMin->iPo    = pCex->iPo;
    pCexMin->iFrame = pCex->iFrame;
    for ( i = 0; i < nLits; i++ )
    {
        int PiNum = Abc_Lit2Var(pLits[i]) - iFirstVar;
        assert( PiNum >= 0 && PiNum < pCex->nBits - pCex->nRegs );
        Abc_InfoSetBit( pCexMin->pData, pCexMin->nRegs + PiNum );
    }
    return pCexMin;
}
Abc_Cex_t * Bmc_CexCareSatBasedMinimizeAig( Gia_Man_t * p, Abc_Cex_t * pCex, int fHighEffort, int fVerbose )
{
    abctime clk = Abc_Clock();
    int n, i, iFirstVar, iLit, status;
    Vec_Int_t * vLits = NULL, * vTemp;
    sat_solver * pSat;
    Cnf_Dat_t * pCnf;
    int nFinal, * pFinal;
    Abc_Cex_t * pCexBest = NULL; 
    int CountBest = 0;
    Gia_Man_t * pFrames;

    // CEX minimization
    clk = Abc_Clock();
    pCexBest = Bmc_CexCareMinimizeAig( p, Gia_ManPiNum(p), pCex, 1, 1, fVerbose );
    for ( i = pCexBest->nRegs; i < pCexBest->nBits; i++ )
        CountBest += Abc_InfoHasBit(pCexBest->pData, i);
    if ( fVerbose )
    {
        printf( "Care bits = %d. ", CountBest );
        Abc_PrintTime( 1, "Non-SAT-based CEX minimization", Abc_Clock() - clk );
    }

    // SAT instance
    clk = Abc_Clock();
    pFrames = Gia_ManFramesForCexMin( p, pCex->iFrame + 1 );
    pCnf = (Cnf_Dat_t*)Mf_ManGenerateCnf( pFrames, 8, 0, 0, 0, 0 );
    iFirstVar = pCnf->nVars - (pCex->iFrame+1) * pCex->nPis;
    pSat = (sat_solver*)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    iLit = Abc_Var2Lit( 1, 1 );
    status = sat_solver_addclause( pSat, &iLit, &iLit + 1 );
    assert( status );
    // create literals
    vTemp = Vec_IntAlloc( 100 );
    for ( i = pCex->nRegs; i < pCex->nBits; i++ )
        Vec_IntPush( vTemp, Abc_Var2Lit(iFirstVar + i - pCex->nRegs, !Abc_InfoHasBit(pCex->pData, i)) );
    if ( fVerbose )
    Abc_PrintTime( 1, "Constructing SAT solver", Abc_Clock() - clk );

    for ( n = 0; n < 2; n++ )
    {
        Vec_IntFreeP( &vLits );

        vLits = Vec_IntDup( vTemp );
        if ( n ) Vec_IntReverseOrder( vLits );

        // SAT-based minimization
        clk = Abc_Clock();
        status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntLimit(vLits), 0, 0, 0, 0 );
        nFinal = sat_solver_final( pSat, &pFinal );
        if ( fVerbose )
        {
            printf( "Status %s   Selected %5d assumptions out of %5d.  ", status == l_False ? "OK ":"BUG", nFinal, Vec_IntSize(vLits) );
            Abc_PrintTime( 1, "Analyze_final", Abc_Clock() - clk );
        }
        if ( CountBest > nFinal )
        {
            CountBest = nFinal;
            ABC_FREE( pCexBest );
            pCexBest = Bmc_CexCareDeriveCex( pCex, iFirstVar, pFinal, nFinal );
        }
        if ( !fHighEffort )
            continue;

        // SAT-based minimization
        clk = Abc_Clock();
        nFinal = sat_solver_minimize_assumptions( pSat, Vec_IntArray(vLits), Vec_IntSize(vLits), 0 );
        if ( fVerbose )
        {
            printf( "Status %s   Selected %5d assumptions out of %5d.  ", status == l_False ? "OK ":"BUG", nFinal, Vec_IntSize(vLits) );
            Abc_PrintTime( 1, "LEXUNSAT     ", Abc_Clock() - clk );
        }
        if ( CountBest > nFinal )
        {
            CountBest = nFinal;
            ABC_FREE( pCexBest );
            pCexBest = Bmc_CexCareDeriveCex( pCex, iFirstVar, Vec_IntArray(vLits), nFinal );
        }
    }
    if ( fVerbose )
    {
        printf( "Final    :    " );
        Bmc_CexPrint( pCexBest, pCexBest->nPis, 0 );
    }
    // cleanup
    Vec_IntFreeP( &vLits );
    Vec_IntFreeP( &vTemp );
    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    Gia_ManStop( pFrames );
    return pCexBest;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

