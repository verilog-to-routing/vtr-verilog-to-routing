/**CFile****************************************************************

  FileName    [pdrMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Property driven reachability.]

  Synopsis    [Manager procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 20, 2010.]

  Revision    [$Id: pdrMan.c,v 1.00 2010/11/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "pdrInt.h"
#include "sat/bmc/bmc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Structural analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Pdr_ManDeriveFlopPriorities3( Gia_Man_t * p, int fMuxCtrls )
{
    int fDiscount = 0;
    Vec_Wec_t * vLevels;
    Vec_Int_t * vRes, * vLevel, * vCosts;
    Gia_Obj_t * pObj, * pCtrl, * pData0, * pData1; 
    int i, k, Entry, MaxEntry = 0;
    Gia_ManCreateRefs(p);
    // discount references
    if ( fDiscount )
    {
        Gia_ManForEachAnd( p, pObj, i )
        {
            if ( !Gia_ObjIsMuxType(pObj) )
                continue;
            pCtrl  = Gia_Regular(Gia_ObjRecognizeMux(pObj, &pData1, &pData0));
            pData0 = Gia_Regular(pData0);
            pData1 = Gia_Regular(pData1);
            p->pRefs[Gia_ObjId(p, pCtrl)]--;
            if ( pData0 == pData1 )
                p->pRefs[Gia_ObjId(p, pData0)]--;
        }
    }
    // create flop costs
    vCosts = Vec_IntAlloc( Gia_ManRegNum(p) );
    Gia_ManForEachRo( p, pObj, i )
    {
        Vec_IntPush( vCosts, Gia_ObjRefNum(p, pObj) );
        MaxEntry = Abc_MaxInt( MaxEntry, Gia_ObjRefNum(p, pObj) );
        //printf( "%d(%d) ", i, Gia_ObjRefNum(p, pObj) );
    }
    //printf( "\n" );
    MaxEntry++;
    // add costs due to MUX inputs
    if ( fMuxCtrls )
    {
        int fVerbose = 0;
        Vec_Bit_t * vCtrls = Vec_BitStart( Gia_ManObjNum(p) );
        Vec_Bit_t * vDatas = Vec_BitStart( Gia_ManObjNum(p) );
        Gia_Obj_t * pCtrl, * pData1, * pData0; 
        int nCtrls = 0, nDatas = 0, nBoth = 0;
        Gia_ManForEachAnd( p, pObj, i )
        {
            if ( !Gia_ObjIsMuxType(pObj) )
                continue;
            pCtrl  = Gia_ObjRecognizeMux( pObj, &pData1, &pData0 );
            pCtrl  = Gia_Regular(pCtrl);
            pData1 = Gia_Regular(pData1);
            pData0 = Gia_Regular(pData0);
            Vec_BitWriteEntry( vCtrls, Gia_ObjId(p, pCtrl), 1 );
            Vec_BitWriteEntry( vDatas, Gia_ObjId(p, pData1), 1 );
            Vec_BitWriteEntry( vDatas, Gia_ObjId(p, pData0), 1 );
        }
        Gia_ManForEachRo( p, pObj, i )
            if ( Vec_BitEntry(vCtrls, Gia_ObjId(p, pObj)) )
                Vec_IntAddToEntry( vCosts, i, MaxEntry );
            //else if ( Vec_BitEntry(vDatas, Gia_ObjId(p, pObj)) )
            //    Vec_IntAddToEntry( vCosts, i,   MaxEntry );
        MaxEntry = 2*MaxEntry + 1;
        // print out
        if ( fVerbose )
        {
            Gia_ManForEachRo( p, pObj, i )
            {
                if ( Vec_BitEntry(vCtrls, Gia_ObjId(p, pObj)) )
                    nCtrls++;
                if ( Vec_BitEntry(vDatas, Gia_ObjId(p, pObj)) )
                    nDatas++;
                if ( Vec_BitEntry(vCtrls, Gia_ObjId(p, pObj)) && Vec_BitEntry(vDatas, Gia_ObjId(p, pObj)) )
                    nBoth++;
            }
            printf( "%10s : Flops = %5d.  Ctrls = %5d.  Datas = %5d.  Both = %5d.\n", Gia_ManName(p), Gia_ManRegNum(p), nCtrls, nDatas, nBoth );
        }
        Vec_BitFree( vCtrls );
        Vec_BitFree( vDatas );
    }
    // create levelized structure
    vLevels = Vec_WecStart( MaxEntry );
    Vec_IntForEachEntry( vCosts, Entry, i )
        Vec_WecPush( vLevels, Entry, i );
    // collect in this order
    MaxEntry = 0;
    vRes = Vec_IntStart( Gia_ManRegNum(p) );
    Vec_WecForEachLevel( vLevels, vLevel, i )
        Vec_IntForEachEntry( vLevel, Entry, k )
            Vec_IntWriteEntry( vRes, Entry, MaxEntry++ );
        //printf( "%d ", Gia_ObjRefNum(p, Gia_ManCi(p, Gia_ManPiNum(p)+Entry)) );
    //printf( "\n" );
    assert( MaxEntry == Gia_ManRegNum(p) );
    Vec_WecFree( vLevels );
    Vec_IntFree( vCosts );
    ABC_FREE( p->pRefs );
//Vec_IntPrint( vRes );
    return vRes;
}

/**Function*************************************************************

  Synopsis    [Structural analysis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Pdr_ManDeriveFlopPriorities2( Gia_Man_t * p, int fMuxCtrls )
{
    int fDiscount = 0;
    Vec_Int_t * vRes = NULL;
    Gia_Obj_t * pObj, * pCtrl, * pData0, * pData1; 
    int MaxEntry = 0;
    int i, * pPerm;
    // create flop costs
    Vec_Int_t * vCosts = Vec_IntStart( Gia_ManRegNum(p) );
    Gia_ManCreateRefs(p);
    // discount references
    if ( fDiscount )
    {
        Gia_ManForEachAnd( p, pObj, i )
        {
            if ( !Gia_ObjIsMuxType(pObj) )
                continue;
            pCtrl  = Gia_Regular(Gia_ObjRecognizeMux(pObj, &pData1, &pData0));
            pData0 = Gia_Regular(pData0);
            pData1 = Gia_Regular(pData1);
            p->pRefs[Gia_ObjId(p, pCtrl)]--;
            if ( pData0 == pData1 )
                p->pRefs[Gia_ObjId(p, pData0)]--;
        }
    }
    Gia_ManForEachRo( p, pObj, i )
    {
        Vec_IntWriteEntry( vCosts, i, Gia_ObjRefNum(p, pObj) );
        MaxEntry = Abc_MaxInt( MaxEntry, Gia_ObjRefNum(p, pObj) );
    }
    MaxEntry++;
    ABC_FREE( p->pRefs );
    // add costs due to MUX inputs
    if ( fMuxCtrls )
    {
        int fVerbose = 0;
        Vec_Bit_t * vCtrls = Vec_BitStart( Gia_ManObjNum(p) );
        Vec_Bit_t * vDatas = Vec_BitStart( Gia_ManObjNum(p) );
        Gia_Obj_t * pCtrl, * pData1, * pData0; 
        int nCtrls = 0, nDatas = 0, nBoth = 0;
        Gia_ManForEachAnd( p, pObj, i )
        {
            if ( !Gia_ObjIsMuxType(pObj) )
                continue;
            pCtrl  = Gia_ObjRecognizeMux( pObj, &pData1, &pData0 );
            pCtrl  = Gia_Regular(pCtrl);
            pData1 = Gia_Regular(pData1);
            pData0 = Gia_Regular(pData0);
            Vec_BitWriteEntry( vCtrls, Gia_ObjId(p, pCtrl), 1 );
            Vec_BitWriteEntry( vDatas, Gia_ObjId(p, pData1), 1 );
            Vec_BitWriteEntry( vDatas, Gia_ObjId(p, pData0), 1 );
        }
        Gia_ManForEachRo( p, pObj, i )
            if ( Vec_BitEntry(vCtrls, Gia_ObjId(p, pObj)) )
                Vec_IntAddToEntry( vCosts, i, MaxEntry );
            //else if ( Vec_BitEntry(vDatas, Gia_ObjId(p, pObj)) )
            //    Vec_IntAddToEntry( vCosts, i,   MaxEntry );
        // print out
        if ( fVerbose )
        {
            Gia_ManForEachRo( p, pObj, i )
            {
                if ( Vec_BitEntry(vCtrls, Gia_ObjId(p, pObj)) )
                    nCtrls++;
                if ( Vec_BitEntry(vDatas, Gia_ObjId(p, pObj)) )
                    nDatas++;
                if ( Vec_BitEntry(vCtrls, Gia_ObjId(p, pObj)) && Vec_BitEntry(vDatas, Gia_ObjId(p, pObj)) )
                    nBoth++;
            }
            printf( "%10s : Flops = %5d.  Ctrls = %5d.  Datas = %5d.  Both = %5d.\n", Gia_ManName(p), Gia_ManRegNum(p), nCtrls, nDatas, nBoth );
        }
        Vec_BitFree( vCtrls );
        Vec_BitFree( vDatas );
    }
    // create ordering based on costs
    pPerm = Abc_MergeSortCost( Vec_IntArray(vCosts), Vec_IntSize(vCosts) );
    vRes = Vec_IntAllocArray( pPerm, Vec_IntSize(vCosts) );
    Vec_IntFree( vCosts );
    vCosts = Vec_IntInvert( vRes, -1 );
    Vec_IntFree( vRes );
//Vec_IntPrint( vCosts );
    return vCosts;
}

/**Function*************************************************************

  Synopsis    [Creates manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Pdr_Man_t * Pdr_ManStart( Aig_Man_t * pAig, Pdr_Par_t * pPars, Vec_Int_t * vPrioInit )
{
    Pdr_Man_t * p;
    p = ABC_CALLOC( Pdr_Man_t, 1 );
    p->pPars    = pPars;
    p->pAig     = pAig;
    p->pGia     = (pPars->fFlopPrio || p->pPars->fNewXSim || p->pPars->fUseAbs) ? Gia_ManFromAigSimple(pAig) : NULL;
    p->vSolvers = Vec_PtrAlloc( 0 );
    p->vClauses = Vec_VecAlloc( 0 );
    p->pQueue   = NULL;
    p->pOrder   = ABC_ALLOC( int, Aig_ManRegNum(pAig) );
    p->vActVars = Vec_IntAlloc( 256 );
    if ( !p->pPars->fMonoCnf )
        p->vVLits   = Vec_WecStart( 1+Abc_MaxInt(1, Aig_ManLevels(pAig)) );
    // internal use
    p->nPrioShift = Abc_Base2Log(Aig_ManRegNum(pAig));
    if ( vPrioInit )
        p->vPrio = vPrioInit;
    else if ( pPars->fFlopPrio )
        p->vPrio = Pdr_ManDeriveFlopPriorities2(p->pGia, 1);
//    else if ( p->pPars->fNewXSim )
//        p->vPrio = Vec_IntStartNatural( Aig_ManRegNum(pAig) );
    else 
        p->vPrio = Vec_IntStart( Aig_ManRegNum(pAig) );
    p->vLits    = Vec_IntAlloc( 100 );  // array of literals
    p->vCiObjs  = Vec_IntAlloc( 100 );  // cone leaves
    p->vCoObjs  = Vec_IntAlloc( 100 );  // cone roots
    p->vCiVals  = Vec_IntAlloc( 100 );  // cone leaf values
    p->vCoVals  = Vec_IntAlloc( 100 );  // cone root values
    p->vNodes   = Vec_IntAlloc( 100 );  // cone nodes
    p->vUndo    = Vec_IntAlloc( 100 );  // cone undos
    p->vVisits  = Vec_IntAlloc( 100 );  // intermediate
    p->vCi2Rem  = Vec_IntAlloc( 100 );  // CIs to be removed
    p->vRes     = Vec_IntAlloc( 100 );  // final result
    p->pCnfMan  = Cnf_ManStart();
    // ternary simulation
    p->pTxs3    = pPars->fNewXSim ? Txs3_ManStart( p, pAig, p->vPrio ) : NULL;
    // additional AIG data-members
    if ( pAig->pFanData == NULL )
        Aig_ManFanoutStart( pAig );
    if ( pAig->pTerSimData == NULL )
        pAig->pTerSimData = ABC_CALLOC( unsigned, 1 + (Aig_ManObjNumMax(pAig) / 16) );
    // time spent on each outputs
    if ( pPars->nTimeOutOne )
    {
        int i;
        p->pTime4Outs = ABC_ALLOC( abctime, Saig_ManPoNum(pAig) );
        for ( i = 0; i < Saig_ManPoNum(pAig); i++ )
            p->pTime4Outs[i] = pPars->nTimeOutOne * CLOCKS_PER_SEC / 1000 + 1;
    }
    if ( pPars->fSolveAll )
    {
        p->vCexes = Vec_PtrStart( Saig_ManPoNum(p->pAig) );
        p->pPars->vOutMap = Vec_IntAlloc( Saig_ManPoNum(pAig) );
        Vec_IntFill( p->pPars->vOutMap, Saig_ManPoNum(pAig), -2 );
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pdr_ManStop( Pdr_Man_t * p )
{
    Pdr_Set_t * pCla;
    sat_solver * pSat;
    int i, k;
    Gia_ManStopP( &p->pGia );
    Aig_ManCleanMarkAB( p->pAig );
    if ( p->pPars->fVerbose ) 
    {
        Abc_Print( 1, "Block =%5d  Oblig =%6d  Clause =%6d  Call =%6d (sat=%.1f%%)  Cex =%4d  Start =%4d\n", 
            p->nBlocks, p->nObligs, p->nCubes, p->nCalls, 100.0 * p->nCallsS / p->nCalls, p->nCexesTotal, p->nStarts );
        ABC_PRTP( "SAT solving", p->tSat,       p->tTotal );
        ABC_PRTP( "  unsat    ", p->tSatUnsat,  p->tTotal );
        ABC_PRTP( "  sat      ", p->tSatSat,    p->tTotal );
        ABC_PRTP( "Generalize ", p->tGeneral,   p->tTotal );
        ABC_PRTP( "Push clause", p->tPush,      p->tTotal );
        ABC_PRTP( "Ternary sim", p->tTsim,      p->tTotal );
        ABC_PRTP( "Containment", p->tContain,   p->tTotal );
        ABC_PRTP( "CNF compute", p->tCnf,       p->tTotal );
        ABC_PRTP( "Refinement ", p->tAbs,       p->tTotal );
        ABC_PRTP( "TOTAL      ", p->tTotal,     p->tTotal );
        fflush( stdout );
    }
//    Abc_Print( 1, "SS =%6d. SU =%6d. US =%6d. UU =%6d.\n", p->nCasesSS, p->nCasesSU, p->nCasesUS, p->nCasesUU );
    Vec_PtrForEachEntry( sat_solver *, p->vSolvers, pSat, i )
        sat_solver_delete( pSat );
    Vec_PtrFree( p->vSolvers );
    Vec_VecForEachEntry( Pdr_Set_t *, p->vClauses, pCla, i, k )
        Pdr_SetDeref( pCla );
    Vec_VecFree( p->vClauses );
    Pdr_QueueStop( p );
    ABC_FREE( p->pOrder );
    Vec_IntFree( p->vActVars );
    // static CNF
    Cnf_DataFree( p->pCnf1 );
    Vec_IntFreeP( &p->vVar2Reg );
    // dynamic CNF
    Cnf_DataFree( p->pCnf2 );
    if ( p->pvId2Vars )
    for ( i = 0; i < Aig_ManObjNumMax(p->pAig); i++ )
        ABC_FREE( p->pvId2Vars[i].pArray );
    ABC_FREE( p->pvId2Vars );
//    Vec_VecFreeP( (Vec_Vec_t **)&p->vVar2Ids );
    for ( i = 0; i < Vec_PtrSize(&p->vVar2Ids); i++ )
        Vec_IntFree( (Vec_Int_t *)Vec_PtrEntry(&p->vVar2Ids, i) );
    ABC_FREE( p->vVar2Ids.pArray );
    Vec_WecFreeP( &p->vVLits );
    // CNF manager
    Cnf_ManStop( p->pCnfMan );
    Vec_IntFreeP( &p->vAbsFlops );
    Vec_IntFreeP( &p->vMapFf2Ppi );
    Vec_IntFreeP( &p->vMapPpi2Ff );
    // terminary simulation
    if ( p->pPars->fNewXSim )
        Txs3_ManStop( p->pTxs3 );
    // internal use
    Vec_IntFreeP( &p->vPrio   );  // priority flops
    Vec_IntFree( p->vLits     );  // array of literals
    Vec_IntFree( p->vCiObjs   );  // cone leaves
    Vec_IntFree( p->vCoObjs   );  // cone roots
    Vec_IntFree( p->vCiVals   );  // cone leaf values
    Vec_IntFree( p->vCoVals   );  // cone root values
    Vec_IntFree( p->vNodes    );  // cone nodes
    Vec_IntFree( p->vUndo     );  // cone undos
    Vec_IntFree( p->vVisits   );  // intermediate
    Vec_IntFree( p->vCi2Rem   );  // CIs to be removed
    Vec_IntFree( p->vRes      );  // final result
    Vec_PtrFreeP( &p->vInfCubes );
    ABC_FREE( p->pTime4Outs );
    if ( p->vCexes )
        Vec_PtrFreeFree( p->vCexes );
    // additional AIG data-members
    if ( p->pAig->pFanData != NULL )
        Aig_ManFanoutStop( p->pAig );
    if ( p->pAig->pTerSimData != NULL )
        ABC_FREE( p->pAig->pTerSimData );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Derives counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Pdr_ManDeriveCex( Pdr_Man_t * p )
{
    Abc_Cex_t * pCex;
    Pdr_Obl_t * pObl;
    int i, f, Lit, nFrames = 0;
    // count the number of frames
    for ( pObl = p->pQueue; pObl; pObl = pObl->pNext )
        nFrames++;
    // create the counter-example
    pCex = Abc_CexAlloc( Aig_ManRegNum(p->pAig), Saig_ManPiNum(p->pAig), nFrames );
    pCex->iPo    = p->iOutCur;
    pCex->iFrame = nFrames-1;
    for ( pObl = p->pQueue, f = 0; pObl; pObl = pObl->pNext, f++ )
        for ( i = pObl->pState->nLits; i < pObl->pState->nTotal; i++ )
        {
            Lit = pObl->pState->Lits[i];
            if ( Abc_LitIsCompl(Lit) )
                continue;
            if ( Abc_Lit2Var(Lit) >= pCex->nPis ) // allows PPI literals to be thrown away
                continue;
            assert( Abc_Lit2Var(Lit) < pCex->nPis );
            Abc_InfoSetBit( pCex->pData, pCex->nRegs + f * pCex->nPis + Abc_Lit2Var(Lit) );
        }
    assert( f == nFrames );
    if ( !Saig_ManVerifyCex(p->pAig, pCex) )
        printf( "CEX for output %d is not valid.\n", p->iOutCur );
    return pCex;
}

/**Function*************************************************************

  Synopsis    [Derives counter-example when abstraction is used.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Pdr_ManDeriveCexAbs( Pdr_Man_t * p )
{
    extern Gia_Man_t * Gia_ManDupAbs( Gia_Man_t * p, Vec_Int_t * vMapPpi2Ff, Vec_Int_t * vMapFf2Ppi );

    Gia_Man_t * pAbs;
    Abc_Cex_t * pCex, * pCexCare;
    Pdr_Obl_t * pObl;
    int i, f, Lit, Flop, nFrames = 0;
    int nPis = Saig_ManPiNum(p->pAig);
    int nFfRefined = 0;
    if ( !p->pPars->fUseAbs || !p->vMapPpi2Ff )
        return Pdr_ManDeriveCex(p);
    // restore previous map
    Vec_IntForEachEntry( p->vMapPpi2Ff, Flop, i )
    {
        assert( Vec_IntEntry( p->vMapFf2Ppi, Flop ) == i );
        Vec_IntWriteEntry( p->vMapFf2Ppi, Flop, -1 );
    }
    Vec_IntClear( p->vMapPpi2Ff );
    // count the number of frames
    for ( pObl = p->pQueue; pObl; pObl = pObl->pNext )
    {
        for ( i = pObl->pState->nLits; i < pObl->pState->nTotal; i++ )
        {
            Lit = pObl->pState->Lits[i]; 
            if ( Abc_Lit2Var(Lit) < nPis ) // PI literal
                continue;
            Flop = Abc_Lit2Var(Lit) - nPis;
            if ( Vec_IntEntry(p->vMapFf2Ppi, Flop) >= 0 ) // already used PPI literal
                continue;
            Vec_IntWriteEntry( p->vMapFf2Ppi, Flop, Vec_IntSize(p->vMapPpi2Ff) );
            Vec_IntPush( p->vMapPpi2Ff, Flop );
        }
        nFrames++;
    }
    if ( Vec_IntSize(p->vMapPpi2Ff) == 0 ) // no PPIs -- this is a real CEX
        return Pdr_ManDeriveCex(p);
    if ( p->pPars->fUseSimpleRef )
    {
        // rely on ternary simulation to perform refinement
        Vec_IntForEachEntry( p->vMapPpi2Ff, Flop, i )
        {
            assert( Vec_IntEntry(p->vAbsFlops, Flop) == 0 );
            Vec_IntWriteEntry( p->vAbsFlops, Flop, 1 );
            nFfRefined++;
        }
    }
    else
    {
        // create the counter-example
        pCex = Abc_CexAlloc( Aig_ManRegNum(p->pAig) - Vec_IntSize(p->vMapPpi2Ff), Saig_ManPiNum(p->pAig) + Vec_IntSize(p->vMapPpi2Ff), nFrames );
        pCex->iPo    = p->iOutCur;
        pCex->iFrame = nFrames-1;
        for ( pObl = p->pQueue, f = 0; pObl; pObl = pObl->pNext, f++ )
            for ( i = pObl->pState->nLits; i < pObl->pState->nTotal; i++ )
            {
                Lit = pObl->pState->Lits[i];
                if ( Abc_LitIsCompl(Lit) )
                    continue;
                if ( Abc_Lit2Var(Lit) < nPis ) // PI literal
                    Abc_InfoSetBit( pCex->pData, pCex->nRegs + f * pCex->nPis + Abc_Lit2Var(Lit) );
                else
                {
                    int iPPI = nPis + Vec_IntEntry(p->vMapFf2Ppi, Abc_Lit2Var(Lit) - nPis);
                    assert( iPPI < pCex->nPis );
                    Abc_InfoSetBit( pCex->pData, pCex->nRegs + f * pCex->nPis + iPPI );
                }
            }
        assert( f == nFrames );
        // perform CEX minimization
        pAbs = Gia_ManDupAbs( p->pGia, p->vMapPpi2Ff, p->vMapFf2Ppi );
        pCexCare = Bmc_CexCareMinimizeAig( pAbs, nPis, pCex, 1, 0, 0 );
        Gia_ManStop( pAbs );
        assert( pCexCare->nPis == pCex->nPis );
        Abc_CexFree( pCex );
        // detect care PPIs
        for ( f = 0; f < nFrames; f++ )
        {
            for ( i = nPis; i < pCexCare->nPis; i++ )
                if ( Abc_InfoHasBit(pCexCare->pData, pCexCare->nRegs + pCexCare->nPis * f + i) )
                {
                    if ( Vec_IntEntry(p->vAbsFlops, Vec_IntEntry(p->vMapPpi2Ff, i-nPis)) == 0 ) // currently abstracted
                        Vec_IntWriteEntry( p->vAbsFlops, Vec_IntEntry(p->vMapPpi2Ff, i-nPis), 1 ), nFfRefined++;
                }
        }
        Abc_CexFree( pCexCare );
        if ( nFfRefined == 0 ) // no refinement -- this is a real CEX
            return Pdr_ManDeriveCex(p);
    }
    //printf( "CEX-based refinement refined %d flops.\n", nFfRefined );
    p->nCexesTotal++;
    p->nCexes++;
    return NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

