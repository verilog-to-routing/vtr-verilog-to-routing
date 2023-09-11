/**CFile****************************************************************

  FileName    [pdrIncr.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Property driven reachability.]

  Synopsis    [PDR with incremental solving.]

  Author      [Yen-Sheng Ho, Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Feb. 17, 2017.]

  Revision    [$Id: pdrIncr.c$]

***********************************************************************/

#include "pdrInt.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
extern int Pdr_ManBlockCube( Pdr_Man_t * p, Pdr_Set_t * pCube );
extern int Pdr_ManPushClauses( Pdr_Man_t * p );
extern Pdr_Set_t * Pdr_ManReduceClause( Pdr_Man_t * p, int k, Pdr_Set_t * pCube );
extern int Gia_ManToBridgeAbort( FILE * pFile, int Size, unsigned char * pBuffer );
extern int Gia_ManToBridgeResult( FILE * pFile, int Result, Abc_Cex_t * pCex, int iPoProved );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

Vec_Ptr_t * IPdr_ManPushClausesK( Pdr_Man_t * p, int k )
{
    Pdr_Set_t * pTemp, * pCubeK, * pCubeK1;
    Vec_Ptr_t * vArrayK, * vArrayK1;
    int i, j, m, RetValue;

    assert( Vec_VecSize(p->vClauses) == k + 1 );
    vArrayK = Vec_VecEntry( p->vClauses, k );
    Vec_PtrSort( vArrayK, (int (*)(const void *, const void *))Pdr_SetCompare );
    vArrayK1 = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Pdr_Set_t *, vArrayK, pCubeK, j )
    {
        // remove cubes in the same frame that are contained by pCubeK
        Vec_PtrForEachEntryStart( Pdr_Set_t *, vArrayK, pTemp, m, j+1 )
        {
            if ( !Pdr_SetContains( pTemp, pCubeK ) ) // pCubeK contains pTemp
                continue;
            Pdr_SetDeref( pTemp );
            Vec_PtrWriteEntry( vArrayK, m, Vec_PtrEntryLast(vArrayK) );
            Vec_PtrPop(vArrayK);
            m--;
        }

        // check if the clause can be moved to the next frame
        RetValue = Pdr_ManCheckCube( p, k, pCubeK, NULL, 0, 0, 1 );
        assert( RetValue != -1 );
        if ( !RetValue )
            continue;

        {
            Pdr_Set_t * pCubeMin;
            pCubeMin = Pdr_ManReduceClause( p, k, pCubeK );
            if ( pCubeMin != NULL )
            {
                //                Abc_Print( 1, "%d ", pCubeK->nLits - pCubeMin->nLits );
                Pdr_SetDeref( pCubeK );
                pCubeK = pCubeMin;
            }
        }

        // check if the clause subsumes others
        Vec_PtrForEachEntry( Pdr_Set_t *, vArrayK1, pCubeK1, i )
        {
            if ( !Pdr_SetContains( pCubeK1, pCubeK ) ) // pCubeK contains pCubeK1
                continue;
            Pdr_SetDeref( pCubeK1 );
            Vec_PtrWriteEntry( vArrayK1, i, Vec_PtrEntryLast(vArrayK1) );
            Vec_PtrPop(vArrayK1);
            i--;
        }
        // add the last clause
        Vec_PtrPush( vArrayK1, pCubeK );
        Vec_PtrWriteEntry( vArrayK, j, Vec_PtrEntryLast(vArrayK) );
        Vec_PtrPop(vArrayK);
        j--;
    }

    return vArrayK1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void IPdr_ManPrintClauses( Vec_Vec_t * vClauses, int kStart, int nRegs )
{
    Vec_Ptr_t * vArrayK;
    Pdr_Set_t * pCube;
    int i, k, Counter = 0;
    Vec_VecForEachLevelStart( vClauses, vArrayK, k, kStart )
    {
        Vec_PtrSort( vArrayK, (int (*)(const void *, const void *))Pdr_SetCompare );
        Vec_PtrForEachEntry( Pdr_Set_t *, vArrayK, pCube, i )
        {
            Abc_Print( 1, "Frame[%4d]Cube[%4d] = ", k, Counter++ );
            // Pdr_SetPrint( stdout, pCube, nRegs, NULL );  
            ZPdr_SetPrint( pCube );  
            Abc_Print( 1, "\n" ); 
        }
    }
}

/**Function*************************************************************

  Synopsis    [ Check if each cube c_k in frame k satisfies the query
                R_{k-1} && T && !c_k && c_k' (must be UNSAT).
                Return True if all cubes pass the check. ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IPdr_ManCheckClauses( Pdr_Man_t * p )
{
    Pdr_Set_t * pCubeK;
    Vec_Ptr_t * vArrayK;
    int j, k, RetValue, kMax = Vec_PtrSize(p->vSolvers);
    int iStartFrame = 1;
    int counter = 0;

    Vec_VecForEachLevelStartStop( p->vClauses, vArrayK, k, iStartFrame, kMax )
    {
        Vec_PtrForEachEntry( Pdr_Set_t *, vArrayK, pCubeK, j )
        {
            ++counter;
            RetValue = Pdr_ManCheckCube( p, k - 1, pCubeK, NULL, 0, 0, 1 );

            if ( !RetValue ) {
                printf( "Cube[%d][%d] not inductive!\n", k, j );
            }

            if ( RetValue == -1 )
                return -1;
        }
    }

    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * IPdr_ManSaveClauses( Pdr_Man_t * p, int fDropLast )
{
    int i, k;
    Vec_Vec_t * vClausesSaved;
    Pdr_Set_t * pCla;

    if ( Vec_VecSize( p->vClauses ) == 1 )
        return NULL;
    if ( Vec_VecSize( p->vClauses ) == 2 && fDropLast )
        return NULL;

    if ( fDropLast )
        vClausesSaved = Vec_VecStart( Vec_VecSize(p->vClauses)-1 );
    else 
        vClausesSaved = Vec_VecStart( Vec_VecSize(p->vClauses) );

    Vec_VecForEachEntryStartStop( Pdr_Set_t *, p->vClauses, pCla, i, k, 0, Vec_VecSize(vClausesSaved) )
        Vec_VecPush(vClausesSaved, i, Pdr_SetDup(pCla));
 
    return vClausesSaved;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
sat_solver * IPdr_ManSetSolver( Pdr_Man_t * p, int k, int fSetPropOutput )
{
    sat_solver * pSat;
    Vec_Ptr_t * vArrayK;
    Pdr_Set_t * pCube;
    int i, j;

    assert( Vec_PtrSize(p->vSolvers) == k );
    assert( Vec_IntSize(p->vActVars) == k );

    pSat = zsat_solver_new_seed(p->pPars->nRandomSeed);
    pSat = Pdr_ManNewSolver( pSat, p, k, (int)(k == 0) );
    Vec_PtrPush( p->vSolvers, pSat );
    Vec_IntPush( p->vActVars, 0 );

    // set the property output
    if ( fSetPropOutput )
        Pdr_ManSetPropertyOutput( p, k );

    if (k == 0)
        return pSat;

    // add the clauses
    Vec_VecForEachLevelStart( p->vClauses, vArrayK, i, k )
        Vec_PtrForEachEntry( Pdr_Set_t *, vArrayK, pCube, j )
            Pdr_ManSolverAddClause( p, k, pCube );
    return pSat;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IPdr_ManRebuildClauses( Pdr_Man_t * p, Vec_Vec_t * vClauses )
{
    Vec_Ptr_t * vArrayK;
    Pdr_Set_t * pCube;
    int i, j;
    int RetValue = -1;
    int nCubes = 0;

    if ( vClauses == NULL )
        return RetValue;

    assert( Vec_VecSize(vClauses) >= 2 );
    assert( Vec_VecSize(p->vClauses) == 0 ); 
    Vec_VecExpand( p->vClauses, 1 );

    IPdr_ManSetSolver( p, 0, 1 );

    // push clauses from R0 to R1
    Vec_VecForEachLevelStart( vClauses, vArrayK, i, 1 )
    {
        Vec_PtrForEachEntry( Pdr_Set_t *, vArrayK, pCube, j )
        {
            ++nCubes;

            RetValue = Pdr_ManCheckCube( p, 0, pCube, NULL, 0, 0, 1 );
            Vec_IntWriteEntry( p->vActVars, 0, 0 );

            assert( RetValue != -1 );

            if ( RetValue == 0 )
            {
                Abc_Print( 1, "Cube[%d][%d] cannot be pushed from R0 to R1.\n", i, j );
                Pdr_SetDeref( pCube );
                continue;
            }

            Vec_VecPush( p->vClauses, 1, pCube );
        }
    }
    Abc_Print( 1, "RebuildClauses: %d out of %d cubes reused in R1.\n", Vec_PtrSize(Vec_VecEntry(p->vClauses, 1)), nCubes );
    IPdr_ManSetSolver( p, 1, 0 );
    Vec_VecFree( vClauses );

    /*
    for ( i = 1; i < Vec_VecSize( vClauses ) - 1; ++i )
    {
        vArrayK = IPdr_ManPushClausesK( p, i );
        if ( Vec_PtrSize(vArrayK) == 0 )
        {
            Abc_Print( 1, "RebuildClauses: stopped at frame %d.\n", i );
            break;
        }

        Vec_PtrGrow( (Vec_Ptr_t *)(p->vClauses), i + 2 );
        p->vClauses->nSize = i + 2;
        p->vClauses->pArray[i+1] = vArrayK;
        IPdr_ManSetSolver( p, i + 1, 0 );
    }

    Abc_Print( 1, "After rebuild:" );
    Vec_VecForEachLevel( p->vClauses, vArrayK, i )
        Abc_Print( 1, " %d", Vec_PtrSize( vArrayK ) );
    Abc_Print( 1, "\n" );
    */

    return 0;
}

int IPdr_ManRestoreAbsFlops( Pdr_Man_t * p )
{
    Pdr_Set_t * pSet; int i, j, k;   

    Vec_VecForEachEntry( Pdr_Set_t *, p->vClauses, pSet, i, j )
    {
        for ( k = 0; k < pSet->nLits; k++ )
        {
            Vec_IntWriteEntry( p->vAbsFlops, Abc_Lit2Var(pSet->Lits[k]), 1 );
        }
    }
    return 0;
}

int IPdr_ManRestoreClauses( Pdr_Man_t * p, Vec_Vec_t * vClauses, Vec_Int_t * vMap )
{
    int i;

    assert(vClauses);

    Vec_VecFree(p->vClauses);
    p->vClauses = vClauses;

    // remap clause literals using mapping (old flop -> new flop) found in array vMap
    if ( vMap )
    {
        Pdr_Set_t * pSet; int j, k;   
        Vec_VecForEachEntry( Pdr_Set_t *, vClauses, pSet, i, j )
            for ( k = 0; k < pSet->nLits; k++ )
                pSet->Lits[k] = Abc_Lit2LitV( Vec_IntArray(vMap), pSet->Lits[k] );
    }

    for ( i = 0; i < Vec_VecSize(p->vClauses); ++i )
        IPdr_ManSetSolver( p, i, i < Vec_VecSize( p->vClauses ) - 1 );

    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IPdr_ManSolveInt( Pdr_Man_t * p, int fCheckClauses, int fPushClauses )
{
    int fPrintClauses = 0;
    Pdr_Set_t * pCube = NULL;
    Aig_Obj_t * pObj;
    Abc_Cex_t * pCexNew;
    int iFrame, RetValue = -1;
    int nOutDigits = Abc_Base10Log( Saig_ManPoNum(p->pAig) );
    abctime clkStart = Abc_Clock(), clkOne = 0;
    p->timeToStop = p->pPars->nTimeOut ? p->pPars->nTimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0;
    // assert( Vec_PtrSize(p->vSolvers) == 0 );
    // in the multi-output mode, mark trivial POs (those fed by const0) as solved 
    if ( p->pPars->fSolveAll )
        Saig_ManForEachPo( p->pAig, pObj, iFrame )
            if ( Aig_ObjChild0(pObj) == Aig_ManConst0(p->pAig) )
            {
                Vec_IntWriteEntry( p->pPars->vOutMap, iFrame, 1 ); // unsat
                p->pPars->nProveOuts++;
                if ( p->pPars->fUseBridge )
                    Gia_ManToBridgeResult( stdout, 1, NULL, iFrame );
            }
    // create the first timeframe
    p->pPars->timeLastSolved = Abc_Clock();

    if ( Vec_VecSize(p->vClauses) == 0 )
        Pdr_ManCreateSolver( p, (iFrame = 0) );
    else {
        iFrame = Vec_VecSize(p->vClauses) - 1;

        if ( fCheckClauses )
        {
            if ( p->pPars->fVerbose )
               Abc_Print( 1, "IPDR: Checking the reloaded length-%d trace...", iFrame + 1 ) ;
            IPdr_ManCheckClauses( p );
            if ( p->pPars->fVerbose )
               Abc_Print( 1, " Passed!\n" ) ;
        }

        if ( fPushClauses )
        {
            p->iUseFrame = Abc_MaxInt(iFrame, 1);

            if ( p->pPars->fVerbose ) 
            {
                Abc_Print( 1, "IPDR: Pushing the reloaded clauses. Before:\n" );
                Pdr_ManPrintProgress( p, 1, Abc_Clock() - clkStart );
            }

            RetValue = Pdr_ManPushClauses( p );

            if ( p->pPars->fVerbose ) 
            {
                Abc_Print( 1, "IPDR: Finished pushing. After:\n" );
                Pdr_ManPrintProgress( p, 1, Abc_Clock() - clkStart );
            }

            if ( RetValue ) 
            {
                Pdr_ManReportInvariant( p );
                Pdr_ManVerifyInvariant( p );
                return 1;
            }
        }

        if ( p->pPars->fUseAbs && p->vAbsFlops == NULL && iFrame >= 1 )
        {
            assert( p->vAbsFlops == NULL );
            p->vAbsFlops  = Vec_IntStart( Saig_ManRegNum(p->pAig) );
            p->vMapFf2Ppi = Vec_IntStartFull( Saig_ManRegNum(p->pAig) );
            p->vMapPpi2Ff = Vec_IntAlloc( 100 );
            
            IPdr_ManRestoreAbsFlops( p );
        }
 
    }
    while ( 1 )
    {
        int fRefined = 0;
        if ( p->pPars->fUseAbs && p->vAbsFlops == NULL && iFrame == 1 )
        {
//            int i, Prio;
            assert( p->vAbsFlops == NULL );
            p->vAbsFlops  = Vec_IntStart( Saig_ManRegNum(p->pAig) );
            p->vMapFf2Ppi = Vec_IntStartFull( Saig_ManRegNum(p->pAig) );
            p->vMapPpi2Ff = Vec_IntAlloc( 100 );
//            Vec_IntForEachEntry( p->vPrio, Prio, i )
//                if ( Prio >> p->nPrioShift  )
//                    Vec_IntWriteEntry( p->vAbsFlops, i, 1 );
        }
        //if ( p->pPars->fUseAbs && p->vAbsFlops )
        //    printf( "Starting frame %d with %d (%d) flops.\n", iFrame, Vec_IntCountPositive(p->vAbsFlops), Vec_IntCountPositive(p->vPrio) );
        p->nFrames = iFrame;
        assert( iFrame == Vec_PtrSize(p->vSolvers)-1 );
        p->iUseFrame = Abc_MaxInt(iFrame, 1);
        Saig_ManForEachPo( p->pAig, pObj, p->iOutCur )
        {
            // skip disproved outputs
            if ( p->vCexes && Vec_PtrEntry(p->vCexes, p->iOutCur) )
                continue;
            // skip output whose time has run out
            if ( p->pTime4Outs && p->pTime4Outs[p->iOutCur] == 0 )
                continue;
            // check if the output is trivially solved
            if ( Aig_ObjChild0(pObj) == Aig_ManConst0(p->pAig) )
                continue;
            // check if the output is trivially solved
            if ( Aig_ObjChild0(pObj) == Aig_ManConst1(p->pAig) )
            {
                if ( !p->pPars->fSolveAll )
                {
                    pCexNew = Abc_CexMakeTriv( Aig_ManRegNum(p->pAig), Saig_ManPiNum(p->pAig), Saig_ManPoNum(p->pAig), iFrame*Saig_ManPoNum(p->pAig)+p->iOutCur );
                    p->pAig->pSeqModel = pCexNew;
                    return 0; // SAT
                }
                pCexNew = (p->pPars->fUseBridge || p->pPars->fStoreCex) ? Abc_CexMakeTriv( Aig_ManRegNum(p->pAig), Saig_ManPiNum(p->pAig), Saig_ManPoNum(p->pAig), iFrame*Saig_ManPoNum(p->pAig)+p->iOutCur ) : (Abc_Cex_t *)(ABC_PTRINT_T)1;
                p->pPars->nFailOuts++;
                if ( p->pPars->vOutMap ) Vec_IntWriteEntry( p->pPars->vOutMap, p->iOutCur, 0 );
                if ( !p->pPars->fNotVerbose )
                Abc_Print( 1, "Output %*d was trivially asserted in frame %2d (solved %*d out of %*d outputs).\n",
                    nOutDigits, p->iOutCur, iFrame, nOutDigits, p->pPars->nFailOuts, nOutDigits, Saig_ManPoNum(p->pAig) );
                assert( Vec_PtrEntry(p->vCexes, p->iOutCur) == NULL );
                if ( p->pPars->fUseBridge )
                    Gia_ManToBridgeResult( stdout, 0, pCexNew, pCexNew->iPo );
                Vec_PtrWriteEntry( p->vCexes, p->iOutCur, pCexNew );
                if ( p->pPars->pFuncOnFail && p->pPars->pFuncOnFail(p->iOutCur, p->pPars->fStoreCex ? (Abc_Cex_t *)Vec_PtrEntry(p->vCexes, p->iOutCur) : NULL) )
                {
                    if ( p->pPars->fVerbose )
                        Pdr_ManPrintProgress( p, 1, Abc_Clock() - clkStart );
                    if ( !p->pPars->fSilent )
                        Abc_Print( 1, "Quitting due to callback on fail in frame %d.\n", iFrame );
                    p->pPars->iFrame = iFrame;
                    return -1;
                }
                if ( p->pPars->nFailOuts + p->pPars->nDropOuts == Saig_ManPoNum(p->pAig) )
                    return p->pPars->nFailOuts ? 0 : -1; // SAT or UNDEC
                p->pPars->timeLastSolved = Abc_Clock();
                continue;
            }
            // try to solve this output
            if ( p->pTime4Outs )
            {
                assert( p->pTime4Outs[p->iOutCur] > 0 );
                clkOne = Abc_Clock();
                p->timeToStopOne = p->pTime4Outs[p->iOutCur] + Abc_Clock();
            }
            while ( 1 )
            {
                if ( p->pPars->nTimeOutGap && p->pPars->timeLastSolved && Abc_Clock() > p->pPars->timeLastSolved + p->pPars->nTimeOutGap * CLOCKS_PER_SEC )
                {
                    if ( p->pPars->fVerbose )
                        Pdr_ManPrintProgress( p, 1, Abc_Clock() - clkStart );
                    if ( !p->pPars->fSilent )
                        Abc_Print( 1, "Reached gap timeout (%d seconds) in frame %d.\n",  p->pPars->nTimeOutGap, iFrame );
                    p->pPars->iFrame = iFrame;
                    return -1;
                }
                RetValue = Pdr_ManCheckCube( p, iFrame, NULL, &pCube, p->pPars->nConfLimit, 0, 1 );
                if ( RetValue == 1 )
                    break;
                if ( RetValue == -1 )
                {
                    if ( p->pPars->fVerbose )
                        Pdr_ManPrintProgress( p, 1, Abc_Clock() - clkStart );
                    if ( p->timeToStop && Abc_Clock() > p->timeToStop )
                        Abc_Print( 1, "Reached timeout (%d seconds) in frame %d.\n",  p->pPars->nTimeOut, iFrame );
                    else if ( p->pPars->nTimeOutGap && p->pPars->timeLastSolved && Abc_Clock() > p->pPars->timeLastSolved + p->pPars->nTimeOutGap * CLOCKS_PER_SEC )
                        Abc_Print( 1, "Reached gap timeout (%d seconds) in frame %d.\n",  p->pPars->nTimeOutGap, iFrame );
                    else if ( p->timeToStopOne && Abc_Clock() > p->timeToStopOne )
                    {
                        Pdr_QueueClean( p );
                        pCube = NULL;
                        break; // keep solving
                    }
                    else if ( p->pPars->nConfLimit )
                        Abc_Print( 1, "Reached conflict limit (%d) in frame %d.\n",  p->pPars->nConfLimit, iFrame );
                    else if ( p->pPars->fVerbose )
                        Abc_Print( 1, "Computation cancelled by the callback in frame %d.\n", iFrame );
                    p->pPars->iFrame = iFrame;
                    return -1;
                }
                if ( RetValue == 0 )
                {
                    RetValue = Pdr_ManBlockCube( p, pCube );
                    if ( RetValue == -1 )
                    {
                        if ( p->pPars->fVerbose )
                            Pdr_ManPrintProgress( p, 1, Abc_Clock() - clkStart );
                        if ( p->timeToStop && Abc_Clock() > p->timeToStop )
                            Abc_Print( 1, "Reached timeout (%d seconds) in frame %d.\n",  p->pPars->nTimeOut, iFrame );
                        else if ( p->pPars->nTimeOutGap && p->pPars->timeLastSolved && Abc_Clock() > p->pPars->timeLastSolved + p->pPars->nTimeOutGap * CLOCKS_PER_SEC )
                            Abc_Print( 1, "Reached gap timeout (%d seconds) in frame %d.\n",  p->pPars->nTimeOutGap, iFrame );
                        else if ( p->timeToStopOne && Abc_Clock() > p->timeToStopOne )
                        {
                            Pdr_QueueClean( p );
                            pCube = NULL;
                            break; // keep solving
                        }
                        else if ( p->pPars->nConfLimit )
                            Abc_Print( 1, "Reached conflict limit (%d) in frame %d.\n",  p->pPars->nConfLimit, iFrame );
                        else if ( p->pPars->fVerbose )
                            Abc_Print( 1, "Computation cancelled by the callback in frame %d.\n", iFrame );
                        p->pPars->iFrame = iFrame;
                        return -1;
                    }
                    if ( RetValue == 0 )
                    {
                        if ( fPrintClauses )
                        {
                            Abc_Print( 1, "*** Clauses after frame %d:\n", iFrame );
                            Pdr_ManPrintClauses( p, 0 );
                        }
                        if ( p->pPars->fVerbose && !p->pPars->fUseAbs )
                            Pdr_ManPrintProgress( p, !p->pPars->fSolveAll, Abc_Clock() - clkStart );
                        p->pPars->iFrame = iFrame;
                        if ( !p->pPars->fSolveAll )
                        {
                            abctime clk = Abc_Clock();
                            Abc_Cex_t * pCex = Pdr_ManDeriveCexAbs(p);
                            p->tAbs += Abc_Clock() - clk;
                            if ( pCex == NULL )
                            {
                                assert( p->pPars->fUseAbs );
                                Pdr_QueueClean( p );
                                pCube = NULL;
                                fRefined = 1;
                                break; // keep solving
                            }
                            p->pAig->pSeqModel = pCex;

                            if ( p->pPars->fVerbose && p->pPars->fUseAbs )
                                Pdr_ManPrintProgress( p, !p->pPars->fSolveAll, Abc_Clock() - clkStart );
                            return 0; // SAT
                        }
                        p->pPars->nFailOuts++;
                        pCexNew = (p->pPars->fUseBridge || p->pPars->fStoreCex) ? Pdr_ManDeriveCex(p) : (Abc_Cex_t *)(ABC_PTRINT_T)1;
                        if ( p->pPars->vOutMap ) Vec_IntWriteEntry( p->pPars->vOutMap, p->iOutCur, 0 );
                        assert( Vec_PtrEntry(p->vCexes, p->iOutCur) == NULL );
                        if ( p->pPars->fUseBridge )
                            Gia_ManToBridgeResult( stdout, 0, pCexNew, pCexNew->iPo );
                        Vec_PtrWriteEntry( p->vCexes, p->iOutCur, pCexNew );
                        if ( p->pPars->pFuncOnFail && p->pPars->pFuncOnFail(p->iOutCur, p->pPars->fStoreCex ? (Abc_Cex_t *)Vec_PtrEntry(p->vCexes, p->iOutCur) : NULL) )
                        {
                            if ( p->pPars->fVerbose )
                                Pdr_ManPrintProgress( p, 1, Abc_Clock() - clkStart );
                            if ( !p->pPars->fSilent )
                                Abc_Print( 1, "Quitting due to callback on fail in frame %d.\n", iFrame );
                            p->pPars->iFrame = iFrame;
                            return -1;
                        }
                        if ( !p->pPars->fNotVerbose )
                            Abc_Print( 1, "Output %*d was asserted in frame %2d (%2d) (solved %*d out of %*d outputs).\n",
                                nOutDigits, p->iOutCur, iFrame, iFrame, nOutDigits, p->pPars->nFailOuts, nOutDigits, Saig_ManPoNum(p->pAig) );
                        if ( p->pPars->nFailOuts == Saig_ManPoNum(p->pAig) )
                            return 0; // all SAT
                        Pdr_QueueClean( p );
                        pCube = NULL;
                        break; // keep solving
                    }
                    if ( p->pPars->fVerbose )
                        Pdr_ManPrintProgress( p, 0, Abc_Clock() - clkStart );
                }
            }
            if ( fRefined )
                break;
            if ( p->pTime4Outs )
            {
                abctime timeSince = Abc_Clock() - clkOne;
                assert( p->pTime4Outs[p->iOutCur] > 0 );
                p->pTime4Outs[p->iOutCur] = (p->pTime4Outs[p->iOutCur] > timeSince) ? p->pTime4Outs[p->iOutCur] - timeSince : 0;
                if ( p->pTime4Outs[p->iOutCur] == 0 && Vec_PtrEntry(p->vCexes, p->iOutCur) == NULL ) // undecided
                {
                    p->pPars->nDropOuts++;
                    if ( p->pPars->vOutMap ) 
                        Vec_IntWriteEntry( p->pPars->vOutMap, p->iOutCur, -1 );
                    if ( !p->pPars->fNotVerbose ) 
                        Abc_Print( 1, "Timing out on output %*d in frame %d.\n", nOutDigits, p->iOutCur, iFrame );
                }
                p->timeToStopOne = 0;
            }
        }
        /*
        if ( p->pPars->fUseAbs && p->vAbsFlops && !fRefined )
        {
            int i, Used;
            Vec_IntForEachEntry( p->vAbsFlops, Used, i )
                if ( Used && (Vec_IntEntry(p->vPrio, i) >> p->nPrioShift) == 0 )
                    Vec_IntWriteEntry( p->vAbsFlops, i, 0 );
        }
        */
        if ( p->pPars->fUseAbs && p->vAbsFlops && !fRefined )
        {
            Pdr_Set_t * pSet;
            int i, j, k;
            Vec_IntFill( p->vAbsFlops, Vec_IntSize( p->vAbsFlops ), 0 );
            Vec_VecForEachEntry( Pdr_Set_t *, p->vClauses, pSet, i, j )
                for ( k = 0; k < pSet->nLits; k++ )
                    Vec_IntWriteEntry( p->vAbsFlops, Abc_Lit2Var(pSet->Lits[k]), 1 );
        }

        if ( p->pPars->fVerbose )
            Pdr_ManPrintProgress( p, !fRefined, Abc_Clock() - clkStart );
        if ( fRefined )
            continue;
        //if ( p->pPars->fUseAbs && p->vAbsFlops )
        //    printf( "Finished frame %d with %d (%d) flops.\n", iFrame, Vec_IntCountPositive(p->vAbsFlops), Vec_IntCountPositive(p->vPrio) );
        // open a new timeframe
        p->nQueLim = p->pPars->nRestLimit;
        assert( pCube == NULL );
        Pdr_ManSetPropertyOutput( p, iFrame );
        Pdr_ManCreateSolver( p, ++iFrame );
        if ( fPrintClauses )
        {
            Abc_Print( 1, "*** Clauses after frame %d:\n", iFrame );
            Pdr_ManPrintClauses( p, 0 );
        }
        // push clauses into this timeframe
        RetValue = Pdr_ManPushClauses( p );
        if ( RetValue == -1 )
        {
            if ( p->pPars->fVerbose )
                Pdr_ManPrintProgress( p, 1, Abc_Clock() - clkStart );
            if ( !p->pPars->fSilent )
            {
                if ( p->timeToStop && Abc_Clock() > p->timeToStop )
                    Abc_Print( 1, "Reached timeout (%d seconds) in frame %d.\n",  p->pPars->nTimeOut, iFrame );
                else
                    Abc_Print( 1, "Reached conflict limit (%d) in frame.\n",  p->pPars->nConfLimit, iFrame );
            }
            p->pPars->iFrame = iFrame;
            return -1;
        }
        if ( RetValue )
        {
            if ( p->pPars->fVerbose )
                Pdr_ManPrintProgress( p, 1, Abc_Clock() - clkStart );
            if ( !p->pPars->fSilent )
                Pdr_ManReportInvariant( p );
            if ( !p->pPars->fSilent )
                Pdr_ManVerifyInvariant( p );
            p->pPars->iFrame = iFrame;
            // count the number of UNSAT outputs
            p->pPars->nProveOuts = Saig_ManPoNum(p->pAig) - p->pPars->nFailOuts - p->pPars->nDropOuts;
            // convert previously 'unknown' into 'unsat'
            if ( p->pPars->vOutMap )
                for ( iFrame = 0; iFrame < Saig_ManPoNum(p->pAig); iFrame++ )
                    if ( Vec_IntEntry(p->pPars->vOutMap, iFrame) == -2 ) // unknown
                    {
                        Vec_IntWriteEntry( p->pPars->vOutMap, iFrame, 1 ); // unsat
                        if ( p->pPars->fUseBridge )
                            Gia_ManToBridgeResult( stdout, 1, NULL, iFrame );
                    }
            if ( p->pPars->nProveOuts == Saig_ManPoNum(p->pAig) )
                return 1; // UNSAT
            if ( p->pPars->nFailOuts > 0 )
                return 0; // SAT
            return -1;
        }
        if ( p->pPars->fVerbose )
            Pdr_ManPrintProgress( p, 0, Abc_Clock() - clkStart );

        // check termination
        if ( p->pPars->pFuncStop && p->pPars->pFuncStop(p->pPars->RunId) )
        {
            p->pPars->iFrame = iFrame;
            return -1;
        }
        if ( p->timeToStop && Abc_Clock() > p->timeToStop )
        {
            if ( fPrintClauses )
            {
                Abc_Print( 1, "*** Clauses after frame %d:\n", iFrame );
                Pdr_ManPrintClauses( p, 0 );
            }
            if ( p->pPars->fVerbose )
                Pdr_ManPrintProgress( p, 1, Abc_Clock() - clkStart );
            if ( !p->pPars->fSilent )
                Abc_Print( 1, "Reached timeout (%d seconds) in frame %d.\n",  p->pPars->nTimeOut, iFrame );
            p->pPars->iFrame = iFrame;
            return -1;
        }
        if ( p->pPars->nTimeOutGap && p->pPars->timeLastSolved && Abc_Clock() > p->pPars->timeLastSolved + p->pPars->nTimeOutGap * CLOCKS_PER_SEC )
        {
            if ( fPrintClauses )
            {
                Abc_Print( 1, "*** Clauses after frame %d:\n", iFrame );
                Pdr_ManPrintClauses( p, 0 );
            }
            if ( p->pPars->fVerbose )
                Pdr_ManPrintProgress( p, 1, Abc_Clock() - clkStart );
            if ( !p->pPars->fSilent )
                Abc_Print( 1, "Reached gap timeout (%d seconds) in frame %d.\n",  p->pPars->nTimeOutGap, iFrame );
            p->pPars->iFrame = iFrame;
            return -1;
        }
        if ( p->pPars->nFrameMax && iFrame >= p->pPars->nFrameMax )
        {
            if ( p->pPars->fVerbose )
                Pdr_ManPrintProgress( p, 1, Abc_Clock() - clkStart );
            if ( !p->pPars->fSilent )
                Abc_Print( 1, "Reached limit on the number of timeframes (%d).\n", p->pPars->nFrameMax );
            p->pPars->iFrame = iFrame;
            return -1;
        }
    }
    assert( 0 );
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int IPdr_ManSolve( Aig_Man_t * pAig, Pdr_Par_t * pPars )
{
    Pdr_Man_t * p;
    int k, RetValue;
    Vec_Vec_t * vClausesSaved;

    abctime clk = Abc_Clock();
    if ( pPars->nTimeOutOne && !pPars->fSolveAll )
        pPars->nTimeOutOne = 0;
    if ( pPars->nTimeOutOne && pPars->nTimeOut == 0 )
        pPars->nTimeOut = pPars->nTimeOutOne * Saig_ManPoNum(pAig) / 1000 + (int)((pPars->nTimeOutOne * Saig_ManPoNum(pAig) % 1000) > 0);
    if ( pPars->fVerbose )
    {
//    Abc_Print( 1, "Running PDR by Niklas Een (aka IC3 by Aaron Bradley) with these parameters:\n" );
        Abc_Print( 1, "VarMax = %d. FrameMax = %d. QueMax = %d. TimeMax = %d. ",
            pPars->nRecycle,
            pPars->nFrameMax,
            pPars->nRestLimit,
            pPars->nTimeOut );
        Abc_Print( 1, "MonoCNF = %s. SkipGen = %s. SolveAll = %s.\n",
            pPars->fMonoCnf ?     "yes" : "no",
            pPars->fSkipGeneral ? "yes" : "no",
            pPars->fSolveAll ?    "yes" : "no" );
    }
    ABC_FREE( pAig->pSeqModel );


    p = Pdr_ManStart( pAig, pPars, NULL );
    while ( 1 ) {
        RetValue = IPdr_ManSolveInt( p, 1, 0 );

        if ( RetValue == -1 && pPars->iFrame == pPars->nFrameMax) {
            vClausesSaved = IPdr_ManSaveClauses( p, 1 );

            Pdr_ManStop( p );

            p = Pdr_ManStart( pAig, pPars, NULL );
            IPdr_ManRestoreClauses( p, vClausesSaved, NULL );

            pPars->nFrameMax = pPars->nFrameMax << 1;

            continue;
        }

        if ( RetValue == 0 )
            assert( pAig->pSeqModel != NULL || p->vCexes != NULL );
        if ( p->vCexes )
        {
            assert( p->pAig->vSeqModelVec == NULL );
            p->pAig->vSeqModelVec = p->vCexes;
            p->vCexes = NULL;
        }
        if ( p->pPars->fDumpInv )
        {
            char * pFileName = pPars->pInvFileName ? pPars->pInvFileName : Extra_FileNameGenericAppend(p->pAig->pName, "_inv.pla");
                Abc_FrameSetInv( Pdr_ManDeriveInfinityClauses( p, RetValue!=1 ) );
                Pdr_ManDumpClauses( p, pFileName, RetValue==1 );
        }
        else if ( RetValue == 1 )
            Abc_FrameSetInv( Pdr_ManDeriveInfinityClauses( p, RetValue!=1 ) );

        p->tTotal += Abc_Clock() - clk;
        Pdr_ManStop( p );

        break;
    }
    
    
    pPars->iFrame--;
    // convert all -2 (unknown) entries into -1 (undec)
    if ( pPars->vOutMap )
        for ( k = 0; k < Saig_ManPoNum(pAig); k++ )
            if ( Vec_IntEntry(pPars->vOutMap, k) == -2 ) // unknown
                Vec_IntWriteEntry( pPars->vOutMap, k, -1 ); // undec
    if ( pPars->fUseBridge )
        Gia_ManToBridgeAbort( stdout, 7, (unsigned char *)"timeout" );
    return RetValue;
}

int IPdr_ManCheckCombUnsat( Pdr_Man_t * p )
{
    int iFrame, RetValue = -1;

    Pdr_ManCreateSolver( p, (iFrame = 0) );
    Pdr_ManCreateSolver( p, (iFrame = 1) );

    p->nFrames = iFrame;
    p->iUseFrame = Abc_MaxInt(iFrame, 1);

    RetValue = Pdr_ManCheckCube( p, iFrame, NULL, NULL, p->pPars->nConfLimit, 0, 1 );
    return RetValue;
}

int IPdr_ManCheckCubeReduce( Pdr_Man_t * p, Vec_Ptr_t * vClauses, Pdr_Set_t * pCube, int nConfLimit )
{ 
    sat_solver * pSat;
    Vec_Int_t * vLits, * vLitsA;
    int Lit, RetValue = l_True;
    int i;
    Pdr_Set_t * pCla;
    int iActVar = 0;
    abctime clk = Abc_Clock();

    pSat = Pdr_ManSolver( p, 1 );

    if ( pCube == NULL ) // solve the property
    {
        Lit = Abc_Var2Lit( Pdr_ObjSatVar(p, 1, 2, Aig_ManCo(p->pAig, p->iOutCur)), 0 ); // pos literal (property fails)
        RetValue = sat_solver_addclause( pSat, &Lit, &Lit+1 );
        assert( RetValue == 1 );

        vLitsA = Vec_IntStart( Vec_PtrSize( vClauses ) );
        iActVar = Pdr_ManFreeVar( p, 1 );
        for ( i = 1; i < Vec_PtrSize( vClauses ); ++i )
            Pdr_ManFreeVar( p, 1 );
        Vec_PtrForEachEntry( Pdr_Set_t *, vClauses, pCla, i )
        {
            vLits = Pdr_ManCubeToLits( p, 1, pCla, 1, 0 );
            Lit = Abc_Var2Lit( iActVar + i, 1 );
            Vec_IntPush( vLits, Lit );

            RetValue = sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits) );
            assert( RetValue == 1 );
            Vec_IntWriteEntry( vLitsA, i, Abc_Var2Lit( iActVar + i, 0 )  );
        }
        sat_solver_compress( pSat );

        // solve 
        RetValue = sat_solver_solve( pSat, Vec_IntArray(vLitsA), Vec_IntArray(vLitsA) + Vec_IntSize(vLitsA), nConfLimit, 0, 0, 0 );
        Vec_IntFree( vLitsA );

        if ( RetValue == l_Undef )
            return -1;
    }

    assert( RetValue != l_Undef );
    if ( RetValue == l_False ) // UNSAT
    {
        int ncorelits, *pcorelits;
        Vec_Ptr_t * vTemp = NULL;
        Vec_Bit_t * vMark = NULL;

        ncorelits = sat_solver_final(pSat, &pcorelits);
        Abc_Print( 1, "UNSAT at the last frame. nCores = %d (out of %d).", ncorelits, Vec_PtrSize( vClauses ) );
        Abc_PrintTime( 1, "    Time", Abc_Clock() - clk );

        vTemp = Vec_PtrDup( vClauses );
        vMark = Vec_BitStart( Vec_PtrSize( vClauses) );
        Vec_PtrClear( vClauses );
        for ( i = 0; i < ncorelits; ++i )
        {
            //Abc_Print( 1, "Core[%d] = lit(%d) = var(%d) = %d-th set\n", i, pcorelits[i], Abc_Lit2Var(pcorelits[i]), Abc_Lit2Var(pcorelits[i]) - iActVar );
            Vec_BitWriteEntry( vMark, Abc_Lit2Var( pcorelits[i] ) - iActVar, 1 );
        }
        
        Vec_PtrForEachEntry( Pdr_Set_t *, vTemp, pCla, i )
        {
            if ( Vec_BitEntry( vMark, i ) )
            { 
                Vec_PtrPush( vClauses, pCla );
                continue;
            }
            Pdr_SetDeref( pCla );
        }

        Vec_PtrFree( vTemp );
        Vec_BitFree( vMark );
        RetValue = 1;
    }
    else // SAT
    {
        Abc_Print( 1, "SAT at the last frame." );
        Abc_PrintTime( 1, "    Time", Abc_Clock() - clk );
        RetValue = 0;
    }

    return RetValue;
}

int IPdr_ManReduceClauses( Pdr_Man_t * p, Vec_Vec_t * vClauses )
{
    int iFrame, RetValue = -1;
    Vec_Ptr_t * vLast = NULL;

    Pdr_ManCreateSolver( p, (iFrame = 0) );
    Pdr_ManCreateSolver( p, (iFrame = 1) );

    p->nFrames = iFrame;
    p->iUseFrame = Abc_MaxInt(iFrame, 1);

    vLast = Vec_VecEntry( vClauses, Vec_VecSize( vClauses ) - 1 );
    RetValue = IPdr_ManCheckCubeReduce( p, vLast, NULL, p->pPars->nConfLimit );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDarIPdr ( Abc_Ntk_t * pNtk, Pdr_Par_t * pPars )
{
    int RetValue = -1;
    abctime clk = Abc_Clock();
    Aig_Man_t * pMan;
    pMan = Abc_NtkToDar( pNtk, 0, 1 );

    RetValue = IPdr_ManSolve( pMan, pPars );
 
    if ( RetValue == 1 )
        Abc_Print( 1, "Property proved.  " );
    else 
    {
        if ( RetValue == 0 )
        {
            if ( pMan->pSeqModel == NULL )
                Abc_Print( 1, "Counter-example is not available.\n" );
            else
            {
                Abc_Print( 1, "Output %d of miter \"%s\" was asserted in frame %d.  ", pMan->pSeqModel->iPo, pNtk->pName, pMan->pSeqModel->iFrame );
                if ( !Saig_ManVerifyCex( pMan, pMan->pSeqModel ) )
                    Abc_Print( 1, "Counter-example verification has FAILED.\n" );
            }
        }
        else if ( RetValue == -1 )
            Abc_Print( 1, "Property UNDECIDED.  " );
        else
            assert( 0 );
    }
    ABC_PRT( "Time", Abc_Clock() - clk );


    ABC_FREE( pNtk->pSeqModel );
    pNtk->pSeqModel = pMan->pSeqModel;
    pMan->pSeqModel = NULL;
    if ( pNtk->vSeqModelVec )
        Vec_PtrFreeFree( pNtk->vSeqModelVec );
    pNtk->vSeqModelVec = pMan->vSeqModelVec;
    pMan->vSeqModelVec = NULL;
    Aig_ManStop( pMan );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
