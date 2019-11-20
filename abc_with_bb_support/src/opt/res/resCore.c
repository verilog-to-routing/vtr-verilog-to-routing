/**CFile****************************************************************

  FileName    [resCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resynthesis package.]

  Synopsis    [Top-level resynthesis procedure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 15, 2007.]

  Revision    [$Id: resCore.c,v 1.00 2007/01/15 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "resInt.h"
#include "kit.h"
#include "satStore.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Res_Man_t_ Res_Man_t;
struct Res_Man_t_
{
    // general parameters
    Res_Par_t *   pPars;
    // specialized manager
    Res_Win_t *   pWin;          // windowing manager
    Abc_Ntk_t *   pAig;          // the strashed window
    Res_Sim_t *   pSim;          // simulation manager
    Sto_Man_t *   pCnf;          // the CNF of the SAT problem
    Int_Man_t *   pMan;          // interpolation manager;
    Vec_Int_t *   vMem;          // memory for intermediate SOPs
    Vec_Vec_t *   vResubs;       // resubstitution candidates of the AIG
    Vec_Vec_t *   vResubsW;      // resubstitution candidates of the window
    Vec_Vec_t *   vLevels;       // levelized structure for updating
    // statistics
    int           nWins;         // the number of windows tried
    int           nWinNodes;     // the total number of window nodes
    int           nDivNodes;     // the total number of divisors
    int           nWinsTriv;     // the total number of trivial windows
    int           nWinsUsed;     // the total number of useful windows (with at least one candidate)
    int           nConstsUsed;   // the total number of constant nodes under ODC
    int           nCandSets;     // the total number of candidates
    int           nProvedSets;   // the total number of proved groups
    int           nSimEmpty;     // the empty simulation info
    int           nTotalNets;    // the total number of nets
    int           nTotalNodes;   // the total number of nodess
    int           nTotalNets2;   // the total number of nets
    int           nTotalNodes2;  // the total number of nodess
    // runtime
    int           timeWin;       // windowing
    int           timeDiv;       // divisors
    int           timeAig;       // strashing
    int           timeSim;       // simulation
    int           timeCand;      // resubstitution candidates
    int           timeSatTotal;  // SAT solving total 
    int           timeSatSat;    // SAT solving (sat calls)
    int           timeSatUnsat;  // SAT solving (unsat calls)
    int           timeSatSim;    // SAT solving (simulation)
    int           timeInt;       // interpolation 
    int           timeUpd;       // updating  
    int           timeTotal;     // total runtime
};

extern Hop_Obj_t * Kit_GraphToHop( Hop_Man_t * pMan, Kit_Graph_t * pGraph );

extern int s_ResynTime;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocate resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Res_Man_t * Res_ManAlloc( Res_Par_t * pPars )
{
    Res_Man_t * p;
    p = ALLOC( Res_Man_t, 1 );
    memset( p, 0, sizeof(Res_Man_t) );
    assert( pPars->nWindow > 0 && pPars->nWindow < 100 );
    assert( pPars->nCands > 0 && pPars->nCands < 100 );
    p->pPars = pPars;
    p->pWin = Res_WinAlloc();
    p->pSim = Res_SimAlloc( pPars->nSimWords );
    p->pMan = Int_ManAlloc( 512 );
    p->vMem = Vec_IntAlloc( 0 );
    p->vResubs  = Vec_VecStart( pPars->nCands );
    p->vResubsW = Vec_VecStart( pPars->nCands );
    p->vLevels  = Vec_VecStart( 32 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocate resynthesis manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_ManFree( Res_Man_t * p )
{
    if ( p->pPars->fVerbose )
    {
        printf( "Reduction in nodes = %5d. (%.2f %%) ", 
            p->nTotalNodes-p->nTotalNodes2, 
            100.0*(p->nTotalNodes-p->nTotalNodes2)/p->nTotalNodes );
        printf( "Reduction in edges = %5d. (%.2f %%) ", 
            p->nTotalNets-p->nTotalNets2, 
            100.0*(p->nTotalNets-p->nTotalNets2)/p->nTotalNets );
        printf( "\n" );

        printf( "Winds = %d. ", p->nWins );
        printf( "Nodes = %d. (Ave = %5.1f)  ", p->nWinNodes, 1.0*p->nWinNodes/p->nWins );
        printf( "Divs = %d. (Ave = %5.1f)  ",  p->nDivNodes, 1.0*p->nDivNodes/p->nWins );
        printf( "\n" );
        printf( "WinsTriv = %d. ", p->nWinsTriv );
        printf( "SimsEmpt = %d. ", p->nSimEmpty );
        printf( "Const = %d. ", p->nConstsUsed );
        printf( "WindUsed = %d. ", p->nWinsUsed );
        printf( "Cands = %d. ", p->nCandSets );
        printf( "Proved = %d.", p->nProvedSets );
        printf( "\n" );

        PRTP( "Windowing  ", p->timeWin,      p->timeTotal );
        PRTP( "Divisors   ", p->timeDiv,      p->timeTotal );
        PRTP( "Strashing  ", p->timeAig,      p->timeTotal );
        PRTP( "Simulation ", p->timeSim,      p->timeTotal );
        PRTP( "Candidates ", p->timeCand,     p->timeTotal );
        PRTP( "SAT solver ", p->timeSatTotal, p->timeTotal );
        PRTP( "    sat    ", p->timeSatSat,   p->timeTotal );
        PRTP( "    unsat  ", p->timeSatUnsat, p->timeTotal );
        PRTP( "    simul  ", p->timeSatSim,   p->timeTotal );
        PRTP( "Interpol   ", p->timeInt,      p->timeTotal );
        PRTP( "Undating   ", p->timeUpd,      p->timeTotal );
        PRTP( "TOTAL      ", p->timeTotal,    p->timeTotal );
    }
    Res_WinFree( p->pWin );
    if ( p->pAig ) Abc_NtkDelete( p->pAig );
    Res_SimFree( p->pSim );
    if ( p->pCnf ) Sto_ManFree( p->pCnf );
    Int_ManFree( p->pMan );
    Vec_IntFree( p->vMem );
    Vec_VecFree( p->vResubs );
    Vec_VecFree( p->vResubsW );
    Vec_VecFree( p->vLevels );
    free( p );
}

/**Function*************************************************************

  Synopsis    [Incrementally updates level of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res_UpdateNetwork( Abc_Obj_t * pObj, Vec_Ptr_t * vFanins, Hop_Obj_t * pFunc, Vec_Vec_t * vLevels )
{
    Abc_Obj_t * pObjNew, * pFanin;
    int k;
    // create the new node
    pObjNew = Abc_NtkCreateNode( pObj->pNtk );
    pObjNew->pData = pFunc;
    Vec_PtrForEachEntry( vFanins, pFanin, k )
        Abc_ObjAddFanin( pObjNew, pFanin );
    // replace the old node by the new node
    // update the level of the node
    Abc_NtkUpdate( pObj, pObjNew, vLevels );
}

/**Function*************************************************************

  Synopsis    [Entrace into the resynthesis package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkResynthesize( Abc_Ntk_t * pNtk, Res_Par_t * pPars )
{
    ProgressBar * pProgress;
    Res_Man_t * p;
    Abc_Obj_t * pObj;
    Hop_Obj_t * pFunc;
    Kit_Graph_t * pGraph;
    Vec_Ptr_t * vFanins;
    unsigned * puTruth;
    int i, k, RetValue, nNodesOld, nFanins, nFaninsMax;
    int clk, clkTotal = clock();

    // start the manager
    p = Res_ManAlloc( pPars );
    p->nTotalNets = Abc_NtkGetTotalFanins(pNtk);
    p->nTotalNodes = Abc_NtkNodeNum(pNtk);
    nFaninsMax = Abc_NtkGetFaninMax(pNtk);

    // perform the network sweep
    Abc_NtkSweep( pNtk, 0 );

    // convert into the AIG
    if ( !Abc_NtkToAig(pNtk) )
    {
        fprintf( stdout, "Converting to BDD has failed.\n" );
        Res_ManFree( p );
        return 0;
    }
    assert( Abc_NtkHasAig(pNtk) );

    // set the number of levels
    Abc_NtkLevel( pNtk );
    Abc_NtkStartReverseLevels( pNtk, pPars->nGrowthLevel );

    // try resynthesizing nodes in the topological order
    nNodesOld = Abc_NtkObjNumMax(pNtk);
    pProgress = Extra_ProgressBarStart( stdout, nNodesOld );
    Abc_NtkForEachObj( pNtk, pObj, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        if ( !Abc_ObjIsNode(pObj) )
            continue;
        if ( pObj->Id > nNodesOld )
            break;

        // create the window for this node
clk = clock();
        RetValue = Res_WinCompute( pObj, p->pPars->nWindow/10, p->pPars->nWindow%10, p->pWin );
p->timeWin += clock() - clk;
        if ( !RetValue )
            continue;
        p->nWinsTriv += Res_WinIsTrivial( p->pWin );

        if ( p->pPars->fVeryVerbose )
        {
            printf( "%5d (lev=%2d) : ", pObj->Id, pObj->Level );
            printf( "Win = %3d/%3d/%4d/%3d   ", 
                Vec_PtrSize(p->pWin->vLeaves), 
                Vec_PtrSize(p->pWin->vBranches),
                Vec_PtrSize(p->pWin->vNodes), 
                Vec_PtrSize(p->pWin->vRoots) );
        }

        // collect the divisors
clk = clock();
        Res_WinDivisors( p->pWin, Abc_ObjRequiredLevel(pObj) - 1 );
p->timeDiv += clock() - clk;

        p->nWins++;
        p->nWinNodes += Vec_PtrSize(p->pWin->vNodes);
        p->nDivNodes += Vec_PtrSize( p->pWin->vDivs);

        if ( p->pPars->fVeryVerbose )
        {
            printf( "D = %3d ", Vec_PtrSize(p->pWin->vDivs) );
            printf( "D+ = %3d ", p->pWin->nDivsPlus );
        }

        // create the AIG for the window
clk = clock();
        if ( p->pAig ) Abc_NtkDelete( p->pAig );
        p->pAig = Res_WndStrash( p->pWin );
p->timeAig += clock() - clk;

        if ( p->pPars->fVeryVerbose )
        {
            printf( "AIG = %4d ", Abc_NtkNodeNum(p->pAig) );
            printf( "\n" );
        }
 
        // prepare simulation info
clk = clock();
        RetValue = Res_SimPrepare( p->pSim, p->pAig, Vec_PtrSize(p->pWin->vLeaves), 0 ); //p->pPars->fVerbose );
p->timeSim += clock() - clk;
        if ( !RetValue )
        {
            p->nSimEmpty++;
            continue;
        }

        // consider the case of constant node
        if ( p->pSim->fConst0 || p->pSim->fConst1 )
        {
            p->nConstsUsed++;

            pFunc = p->pSim->fConst1? Hop_ManConst1(pNtk->pManFunc) : Hop_ManConst0(pNtk->pManFunc);
            vFanins = Vec_VecEntry( p->vResubsW, 0 );
            Vec_PtrClear( vFanins );
            Res_UpdateNetwork( pObj, vFanins, pFunc, p->vLevels );
            continue;
        }

//        printf( " " );

        // find resub candidates for the node
clk = clock();
        if ( p->pPars->fArea )
            RetValue = Res_FilterCandidates( p->pWin, p->pAig, p->pSim, p->vResubs, p->vResubsW, nFaninsMax, 1 );
        else
            RetValue = Res_FilterCandidates( p->pWin, p->pAig, p->pSim, p->vResubs, p->vResubsW, nFaninsMax, 0 );
p->timeCand += clock() - clk;
        p->nCandSets += RetValue;
        if ( RetValue == 0 )
            continue;

//        printf( "%d(%d) ", Vec_PtrSize(p->pWin->vDivs), RetValue );

        p->nWinsUsed++;

        // iterate through candidate resubstitutions
        Vec_VecForEachLevel( p->vResubs, vFanins, k )
        {
            if ( Vec_PtrSize(vFanins) == 0 )
                break;

            // solve the SAT problem and get clauses
clk = clock();
            if ( p->pCnf ) Sto_ManFree( p->pCnf );
            p->pCnf = Res_SatProveUnsat( p->pAig, vFanins );
            if ( p->pCnf == NULL )
            {
p->timeSatSat += clock() - clk;
//                printf( " Sat\n" );
//                printf( "-" );
                continue;
            }
p->timeSatUnsat += clock() - clk;
//            printf( "+" );

            p->nProvedSets++;
//            printf( " Unsat\n" );
//            continue;
//            printf( "Proved %d.\n", k );

            // write it into a file
//            Sto_ManDumpClauses( p->pCnf, "trace.cnf" );

            // interpolate the problem if it was UNSAT
clk = clock();
            nFanins = Int_ManInterpolate( p->pMan, p->pCnf, 0, &puTruth );
p->timeInt += clock() - clk;
            if ( nFanins != Vec_PtrSize(vFanins) - 2 )
                continue;
            assert( puTruth );
//            Extra_PrintBinary( stdout, puTruth, 1 << nFanins );  printf( "\n" );

            // transform interpolant into the AIG
            pGraph = Kit_TruthToGraph( puTruth, nFanins, p->vMem );

            // derive the AIG for the decomposition tree
            pFunc = Kit_GraphToHop( pNtk->pManFunc, pGraph );
            Kit_GraphFree( pGraph );

            // update the network
clk = clock();
            Res_UpdateNetwork( pObj, Vec_VecEntry(p->vResubsW, k), pFunc, p->vLevels );
p->timeUpd += clock() - clk;
            break;
        }
//        printf( "\n" );
    }
    Extra_ProgressBarStop( pProgress );
    Abc_NtkStopReverseLevels( pNtk );

p->timeSatSim += p->pSim->timeSat;
p->timeSatTotal = p->timeSatSat + p->timeSatUnsat + p->timeSatSim;

    p->nTotalNets2 = Abc_NtkGetTotalFanins(pNtk);
    p->nTotalNodes2 = Abc_NtkNodeNum(pNtk);

    // quit resubstitution manager
p->timeTotal = clock() - clkTotal;
    Res_ManFree( p );

s_ResynTime += clock() - clkTotal;
    // check the resulting network
    if ( !Abc_NtkCheck( pNtk ) )
    {
        fprintf( stdout, "Abc_NtkResynthesize(): Network check has failed.\n" );
        return 0;
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


