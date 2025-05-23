/**CFile****************************************************************

  FileName    [mfsMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [The good old minimization with complete don't-cares.]

  Synopsis    [Procedures working with the manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: mfsMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mfsInt.h"

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
Mfs_Man_t * Mfs_ManAlloc( Mfs_Par_t * pPars )
{
    Mfs_Man_t * p;
    // start the manager
    p = ABC_ALLOC( Mfs_Man_t, 1 );
    memset( p, 0, sizeof(Mfs_Man_t) );
    p->pPars     = pPars;
    p->vProjVarsCnf = Vec_IntAlloc( 100 );
    p->vProjVarsSat = Vec_IntAlloc( 100 );
    p->vDivLits  = Vec_IntAlloc( 100 );
    p->nDivWords = Abc_BitWordNum(p->pPars->nWinMax + MFS_FANIN_MAX);
    p->vDivCexes = Vec_PtrAllocSimInfo( p->pPars->nWinMax + MFS_FANIN_MAX + 1, p->nDivWords );
    p->pMan      = Int_ManAlloc();
    p->vMem      = Vec_IntAlloc( 0 );
    p->vLevels   = Vec_VecStart( 32 );
    p->vMfsFanins= Vec_PtrAlloc( 32 );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfs_ManClean( Mfs_Man_t * p )
{
    if ( p->pAigWin )
        Aig_ManStop( p->pAigWin );
    if ( p->pCnf )
        Cnf_DataFree( p->pCnf );
    if ( p->pSat )
        sat_solver_delete( p->pSat );
    if ( p->vRoots )
        Vec_PtrFree( p->vRoots );
    if ( p->vSupp )
        Vec_PtrFree( p->vSupp );
    if ( p->vNodes )
        Vec_PtrFree( p->vNodes );
    if ( p->vDivs )
        Vec_PtrFree( p->vDivs );
    p->pAigWin = NULL;
    p->pCnf    = NULL;
    p->pSat    = NULL;
    p->vRoots  = NULL;
    p->vSupp   = NULL;
    p->vNodes  = NULL;
    p->vDivs   = NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfs_ManPrint( Mfs_Man_t * p )
{
    if ( p->pPars->fResub )
    {
        printf( "Nodes = %d. Try = %d. Resub = %d. Div = %d. SAT calls = %d. Timeouts = %d. MaxDivs = %d.\n",
            p->nTotalNodesBeg, p->nNodesTried, p->nNodesResub, p->nTotalDivs, p->nSatCalls, p->nTimeOuts, p->nMaxDivs );

        printf( "Attempts :   " );
        printf( "Remove %6d out of %6d (%6.2f %%)   ", p->nRemoves, p->nTryRemoves, 100.0*p->nRemoves/Abc_MaxInt(1, p->nTryRemoves) );
        printf( "Resub  %6d out of %6d (%6.2f %%)   ", p->nResubs,  p->nTryResubs,  100.0*p->nResubs /Abc_MaxInt(1, p->nTryResubs)  );
        printf( "\n" );

        printf( "Reduction:   " );
        printf( "Nodes  %6d out of %6d (%6.2f %%)   ", p->nTotalNodesBeg-p->nTotalNodesEnd, p->nTotalNodesBeg, 100.0*(p->nTotalNodesBeg-p->nTotalNodesEnd)/Abc_MaxInt(1, p->nTotalNodesBeg) );
        printf( "Edges  %6d out of %6d (%6.2f %%)   ", p->nTotalEdgesBeg-p->nTotalEdgesEnd, p->nTotalEdgesBeg, 100.0*(p->nTotalEdgesBeg-p->nTotalEdgesEnd)/Abc_MaxInt(1, p->nTotalEdgesBeg) );
        printf( "\n" );

        if (p->pPars->fPower)
            printf( "Power( %5.2f, %4.2f%%) \n",
                 p->TotalSwitchingBeg - p->TotalSwitchingEnd,
                 100.0*(p->TotalSwitchingBeg-p->TotalSwitchingEnd)/p->TotalSwitchingBeg );
        if ( p->pPars->fSwapEdge )
            printf( "Swappable edges = %d. Total edges = %d. Ratio = %5.2f.\n",
                p->nNodesResub, Abc_NtkGetTotalFanins(p->pNtk), 1.00 * p->nNodesResub / Abc_NtkGetTotalFanins(p->pNtk) );
//        printf( "Average ratio of DCs in the resubed nodes = %.2f.\n", 1.0*p->nDcMints/(64 * p->nNodesResub) );
    }
    else
    {
        printf( "Nodes = %d. Try = %d. Total mints = %d. Local DC mints = %d. Ratio = %5.2f.\n", 
            p->nTotalNodesBeg, p->nNodesTried, p->nMintsTotal, p->nMintsTotal-p->nMintsCare, 
            1.0 * (p->nMintsTotal-p->nMintsCare) / p->nMintsTotal );
//        printf( "Average ratio of sequential DCs in the global space = %5.2f.\n", 
//            1.0-(p->dTotalRatios/p->nNodesTried) );
        printf( "Nodes resyn = %d. Ratio = %5.2f.  Total AIG node gain = %d. Timeouts = %d.\n", 
            p->nNodesDec, 1.0 * p->nNodesDec / p->nNodesTried, p->nNodesGained, p->nTimeOuts );
    }

    ABC_PRTP( "Win", p->timeWin            ,  p->timeTotal );
    ABC_PRTP( "Div", p->timeDiv            ,  p->timeTotal );
    ABC_PRTP( "Aig", p->timeAig            ,  p->timeTotal );
    ABC_PRTP( "Gia", p->timeGia            ,  p->timeTotal );
    ABC_PRTP( "Cnf", p->timeCnf            ,  p->timeTotal );
    ABC_PRTP( "Sat", p->timeSat-p->timeInt ,  p->timeTotal );
    ABC_PRTP( "Int", p->timeInt            ,  p->timeTotal );
    ABC_PRTP( "ALL", p->timeTotal          ,  p->timeTotal );

}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mfs_ManStop( Mfs_Man_t * p )
{
    if ( p->pPars->fVerbose )
        Mfs_ManPrint( p );
    if ( p->vTruth )
        Vec_IntFree( p->vTruth );
    if ( p->pManDec )
        Bdc_ManFree( p->pManDec );
    if ( p->pCare )
        Aig_ManStop( p->pCare );
    if ( p->vSuppsInv )
        Vec_VecFree( (Vec_Vec_t *)p->vSuppsInv );
    if ( p->vProbs )
        Vec_IntFree( p->vProbs );
    Mfs_ManClean( p );
    Int_ManFree( p->pMan );
    Vec_IntFree( p->vMem );
    Vec_VecFree( p->vLevels );
    Vec_PtrFree( p->vMfsFanins );
    Vec_IntFree( p->vProjVarsCnf );
    Vec_IntFree( p->vProjVarsSat );
    Vec_IntFree( p->vDivLits );
    Vec_PtrFree( p->vDivCexes );
    ABC_FREE( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

