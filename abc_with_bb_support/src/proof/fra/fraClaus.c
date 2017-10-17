/**CFile****************************************************************

  FileName    [fraClaus.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    [Induction with clause strengthening.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraClau.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Clu_Man_t_    Clu_Man_t;
struct Clu_Man_t_
{
    // parameters
    int              nFrames;        // the K of the K-step induction
    int              nPref;          // the number of timeframes to skip  
    int              nClausesMax;    // the max number of 4-clauses to consider
    int              nLutSize;       // the max cut size
    int              nLevels;        // the number of levels for cut computation
    int              nCutsMax;       // the maximum number of cuts to compute at a node
    int              nBatches;       // the number of clause batches to use
    int              fStepUp;        // increase cut size for each batch
    int              fTarget;        // tries to prove the property
    int              fVerbose;       
    int              fVeryVerbose;
    // internal parameters
    int              nSimWords;      // the number of simulation words
    int              nSimWordsPref;  // the number of simulation words in the prefix
    int              nSimFrames;     // the number of frames to simulate 
    int              nBTLimit;       // the largest number of backtracks (0 = infinite)
    // the network 
    Aig_Man_t *      pAig;
    // SAT solvers
    sat_solver *     pSatMain;
    sat_solver *     pSatBmc;
    // CNF for the test solver
    Cnf_Dat_t *      pCnf;
    int              fFail;
    int              fFiltering; 
    int              fNothingNew;
    // clauses
    Vec_Int_t *      vLits;
    Vec_Int_t *      vClauses;
    Vec_Int_t *      vCosts;
    int              nClauses;
    int              nCuts;
    int              nOneHots;
    int              nOneHotsProven;
    // clauses proven
    Vec_Int_t *      vLitsProven;
    Vec_Int_t *      vClausesProven;
    // counter-examples
    Vec_Ptr_t *      vCexes;
    int              nCexes;
    int              nCexesAlloc;
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Runs the SAT solver on the problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausRunBmc( Clu_Man_t * p )
{
    Aig_Obj_t * pObj;
    int Lits[2], nLitsTot, RetValue, i;
    // set the output literals
    nLitsTot = 2 * p->pCnf->nVars;
    pObj = Aig_ManCo(p->pAig, 0);
    for ( i = 0; i < p->nPref + p->nFrames; i++ )
    {
        Lits[0] = i * nLitsTot + toLitCond( p->pCnf->pVarNums[pObj->Id], 0 ); 
        RetValue = sat_solver_solve( p->pSatBmc, Lits, Lits + 1, (ABC_INT64_T)p->nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
        if ( RetValue != l_False )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Runs the SAT solver on the problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausRunSat( Clu_Man_t * p )
{
    Aig_Obj_t * pObj;
    int * pLits;
    int i, RetValue;
    pLits = ABC_ALLOC( int, p->nFrames + 1 );
    // set the output literals
    pObj = Aig_ManCo(p->pAig, 0);
    for ( i = 0; i <= p->nFrames; i++ )
        pLits[i] = i * 2 * p->pCnf->nVars + toLitCond( p->pCnf->pVarNums[pObj->Id], i != p->nFrames ); 
    // try to solve the problem
    RetValue = sat_solver_solve( p->pSatMain, pLits, pLits + p->nFrames + 1, (ABC_INT64_T)p->nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    ABC_FREE( pLits );
    if ( RetValue == l_False )
        return 1;
    // get the counter-example
    assert( RetValue == l_True );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Runs the SAT solver on the problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausRunSat0( Clu_Man_t * p )
{
    Aig_Obj_t * pObj;
    int Lits[2], RetValue;
    pObj = Aig_ManCo(p->pAig, 0);
    Lits[0] = toLitCond( p->pCnf->pVarNums[pObj->Id], 0 ); 
    RetValue = sat_solver_solve( p->pSatMain, Lits, Lits + 1, (ABC_INT64_T)p->nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( RetValue == l_False )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Return combinations appearing in the cut.]

  Description [This procedure is taken from "Hacker's Delight" by H.S.Warren.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void transpose32a( unsigned a[32] ) 
{
    int j, k;
    unsigned long m, t;
    for ( j = 16, m = 0x0000FFFF; j; j >>= 1, m ^= m << j ) 
    {
        for ( k = 0; k < 32; k = ((k | j) + 1) & ~j ) 
        {
            t = (a[k] ^ (a[k|j] >> j)) & m;
            a[k] ^= t;
            a[k|j] ^= (t << j);
        }
    }
}

/**Function*************************************************************

  Synopsis    [Return combinations appearing in the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausProcessClausesCut( Clu_Man_t * p, Fra_Sml_t * pSimMan, Dar_Cut_t * pCut, int * pScores )
{
    unsigned Matrix[32];
    unsigned * pSims[16], uWord;
    int nSeries, i, k, j;
    int nWordsForSim = pSimMan->nWordsTotal - p->nSimWordsPref;
    // compute parameters
    assert( pCut->nLeaves > 1 && pCut->nLeaves < 5 );
    assert( nWordsForSim % 8 == 0 );
    // get parameters
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        pSims[i] = Fra_ObjSim( pSimMan, pCut->pLeaves[i] ) + p->nSimWordsPref;
    // add combinational patterns
    memset( pScores, 0, sizeof(int) * 16 );
    nSeries = nWordsForSim / 8;
    for ( i = 0; i < nSeries; i++ )
    {
        memset( Matrix, 0, sizeof(unsigned) * 32 );
        for ( k = 0; k < 8; k++ )
            for ( j = 0; j < (int)pCut->nLeaves; j++ )
                Matrix[31-(k*4+j)] = pSims[j][i*8+k];
        transpose32a( Matrix );
        for ( k = 0; k < 32; k++ )
            for ( j = 0, uWord = Matrix[k]; j < 8; j++, uWord >>= 4 )
                pScores[uWord & 0xF]++;
    }
    // collect patterns
    uWord = 0;
    for ( i = 0; i < 16; i++ )
        if ( pScores[i] )
            uWord |= (1 << i);
//    Extra_PrintBinary( stdout, &uWord, 16 ); printf( "\n" );
    return (int)uWord;
}

/**Function*************************************************************

  Synopsis    [Return combinations appearing in the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausProcessClausesCut2( Clu_Man_t * p, Fra_Sml_t * pSimMan, Dar_Cut_t * pCut, int * pScores )
{
    unsigned * pSims[16], uWord;
    int iMint, i, k, b;
    int nWordsForSim = pSimMan->nWordsTotal - p->nSimWordsPref;
    // compute parameters
    assert( pCut->nLeaves > 1 && pCut->nLeaves < 5 );
    assert( nWordsForSim % 8 == 0 );
    // get parameters
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        pSims[i] = Fra_ObjSim( pSimMan, pCut->pLeaves[i] ) + p->nSimWordsPref;
    // add combinational patterns
    memset( pScores, 0, sizeof(int) * 16 );
    for ( i = 0; i < nWordsForSim; i++ )
        for ( k = 0; k < 32; k++ )
        {
            iMint = 0;
            for ( b = 0; b < (int)pCut->nLeaves; b++ )
                if ( pSims[b][i] & (1 << k) )
                    iMint |= (1 << b);
            pScores[iMint]++;
        }
    // collect patterns
    uWord = 0;
    for ( i = 0; i < 16; i++ )
        if ( pScores[i] )
            uWord |= (1 << i);
//    Extra_PrintBinary( stdout, &uWord, 16 ); printf( "\n" );
    return (int)uWord;
}

/**Function*************************************************************

  Synopsis    [Return the number of combinations appearing in the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClausProcessClausesCut3( Clu_Man_t * p, Fra_Sml_t * pSimMan, Aig_Cut_t * pCut, int * pScores )
{
    unsigned Matrix[32];
    unsigned * pSims[16], uWord;
    int iMint, i, j, k, b, nMints, nSeries;
    int nWordsForSim = pSimMan->nWordsTotal - p->nSimWordsPref;

    // compute parameters
    assert( pCut->nFanins > 1 && pCut->nFanins < 17 );
    assert( nWordsForSim % 8 == 0 );
    // get parameters
    for ( i = 0; i < (int)pCut->nFanins; i++ )
        pSims[i] = Fra_ObjSim( pSimMan, pCut->pFanins[i] ) + p->nSimWordsPref;
    // add combinational patterns
    nMints = (1 << pCut->nFanins);
    memset( pScores, 0, sizeof(int) * nMints );

    if ( pCut->nLeafMax == 4 )
    {
        // convert the simulation patterns
        nSeries = nWordsForSim / 8;
        for ( i = 0; i < nSeries; i++ )
        {
            memset( Matrix, 0, sizeof(unsigned) * 32 );
            for ( k = 0; k < 8; k++ )
                for ( j = 0; j < (int)pCut->nFanins; j++ )
                    Matrix[31-(k*4+j)] = pSims[j][i*8+k];
            transpose32a( Matrix );
            for ( k = 0; k < 32; k++ )
                for ( j = 0, uWord = Matrix[k]; j < 8; j++, uWord >>= 4 )
                    pScores[uWord & 0xF]++;
        }
    }
    else
    {
        // go through the simulation patterns
        for ( i = 0; i < nWordsForSim; i++ )
            for ( k = 0; k < 32; k++ )
            {
                iMint = 0;
                for ( b = 0; b < (int)pCut->nFanins; b++ )
                    if ( pSims[b][i] & (1 << k) )
                        iMint |= (1 << b);
                pScores[iMint]++;
            }
    }
}


/**Function*************************************************************

  Synopsis    [Returns the cut-off cost.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausSelectClauses( Clu_Man_t * p )
{
    int * pCostCount, nClauCount, Cost, CostMax, i, c;
    assert( Vec_IntSize(p->vClauses) > p->nClausesMax );   
    // count how many implications have each cost
    CostMax = p->nSimWords * 32 + 1;
    pCostCount = ABC_ALLOC( int, CostMax );
    memset( pCostCount, 0, sizeof(int) * CostMax );
    Vec_IntForEachEntry( p->vCosts, Cost, i )
    {
        if ( Cost == -1 )
            continue;
        assert( Cost < CostMax );
        pCostCount[ Cost ]++;
    }
    assert( pCostCount[0] == 0 );
    // select the bound on the cost (above this bound, implication will be included)
    nClauCount = 0;
    for ( c = CostMax - 1; c > 0; c-- )
    {
        assert( pCostCount[c] >= 0 );
        nClauCount += pCostCount[c];
        if ( nClauCount >= p->nClausesMax )
            break;
    }
    // collect implications with the given costs
    nClauCount = 0;
    Vec_IntForEachEntry( p->vCosts, Cost, i )
    {
        if ( Cost >= c && nClauCount < p->nClausesMax )
        {
            nClauCount++;
            continue;
        }
        Vec_IntWriteEntry( p->vCosts, i, -1 );
    }
    ABC_FREE( pCostCount );
    p->nClauses = nClauCount;
if ( p->fVerbose )
printf( "Selected %d clauses. Cost range: [%d < %d < %d]\n", nClauCount, 1, c, CostMax );
    return c;
}


/**Function*************************************************************

  Synopsis    [Processes the clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClausRecordClause( Clu_Man_t * p, Dar_Cut_t * pCut, int iMint, int Cost )
{
    int i;
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        Vec_IntPush( p->vLits, toLitCond( p->pCnf->pVarNums[pCut->pLeaves[i]], (iMint&(1<<i)) ) );
    Vec_IntPush( p->vClauses, Vec_IntSize(p->vLits) );
    Vec_IntPush( p->vCosts, Cost );
}

/**Function*************************************************************

  Synopsis    [Processes the clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClausRecordClause2( Clu_Man_t * p, Aig_Cut_t * pCut, int iMint, int Cost )
{
    int i;
    for ( i = 0; i < (int)pCut->nFanins; i++ )
        Vec_IntPush( p->vLits, toLitCond( p->pCnf->pVarNums[pCut->pFanins[i]], (iMint&(1<<i)) ) );
    Vec_IntPush( p->vClauses, Vec_IntSize(p->vLits) );
    Vec_IntPush( p->vCosts, Cost );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausSmlNodeIsConst( Fra_Sml_t * pSeq, Aig_Obj_t * pObj )
{
    unsigned * pSims;
    int i;
    pSims = Fra_ObjSim(pSeq, pObj->Id);
    for ( i = pSeq->nWordsPref; i < pSeq->nWordsTotal; i++ )
        if ( pSims[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if implications holds.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausSmlNodesAreImp( Fra_Sml_t * pSeq, Aig_Obj_t * pObj1, Aig_Obj_t * pObj2 )
{
    unsigned * pSimL, * pSimR;
    int k;
    pSimL = Fra_ObjSim(pSeq, pObj1->Id);
    pSimR = Fra_ObjSim(pSeq, pObj2->Id);
    for ( k = pSeq->nWordsPref; k < pSeq->nWordsTotal; k++ )
        if ( pSimL[k] & ~pSimR[k] ) // !(Obj1 -> Obj2)
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if implications holds.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausSmlNodesAreImpC( Fra_Sml_t * pSeq, Aig_Obj_t * pObj1, Aig_Obj_t * pObj2 )
{
    unsigned * pSimL, * pSimR;
    int k;
    pSimL = Fra_ObjSim(pSeq, pObj1->Id);
    pSimR = Fra_ObjSim(pSeq, pObj2->Id);
    for ( k = pSeq->nWordsPref; k < pSeq->nWordsTotal; k++ )
        if ( pSimL[k] & pSimR[k] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Processes the clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausCollectLatchClauses( Clu_Man_t * p, Fra_Sml_t * pSeq )
{
    Aig_Obj_t * pObj1, * pObj2;
    unsigned * pSims1, * pSims2;
    int CostMax, i, k, nCountConst, nCountImps;

    nCountConst = nCountImps = 0;
    CostMax = p->nSimWords * 32;
/*
    // add the property
    {
        Aig_Obj_t * pObj;
        int Lits[1];
        pObj = Aig_ManCo( p->pAig, 0 );
        Lits[0] = toLitCond( p->pCnf->pVarNums[pObj->Id], 1 ); 
        Vec_IntPush( p->vLits, Lits[0] );
        Vec_IntPush( p->vClauses, Vec_IntSize(p->vLits) );
        Vec_IntPush( p->vCosts, CostMax );
        nCountConst++;
//        printf( "Added the target property to the set of clauses to be inductively checked.\n" );
    }
*/

    pSeq->nWordsPref = p->nSimWordsPref;
    Aig_ManForEachLoSeq( p->pAig, pObj1, i )
    {
        pSims1 = Fra_ObjSim( pSeq, pObj1->Id );
        if ( Fra_ClausSmlNodeIsConst( pSeq, pObj1 ) )
        {
            Vec_IntPush( p->vLits, toLitCond( p->pCnf->pVarNums[pObj1->Id], 1 ) );
            Vec_IntPush( p->vClauses, Vec_IntSize(p->vLits) );
            Vec_IntPush( p->vCosts, CostMax );
            nCountConst++;
            continue;
        }
        Aig_ManForEachLoSeq( p->pAig, pObj2, k )
        {
            pSims2 = Fra_ObjSim( pSeq, pObj2->Id );
            if ( Fra_ClausSmlNodesAreImp( pSeq, pObj1, pObj2 ) )
            {
                Vec_IntPush( p->vLits, toLitCond( p->pCnf->pVarNums[pObj1->Id], 1 ) );
                Vec_IntPush( p->vLits, toLitCond( p->pCnf->pVarNums[pObj2->Id], 0 ) );
                Vec_IntPush( p->vClauses, Vec_IntSize(p->vLits) );
                Vec_IntPush( p->vCosts, CostMax );
                nCountImps++;
                continue;
            }
            if ( Fra_ClausSmlNodesAreImp( pSeq, pObj2, pObj1 ) )
            {
                Vec_IntPush( p->vLits, toLitCond( p->pCnf->pVarNums[pObj2->Id], 1 ) );
                Vec_IntPush( p->vLits, toLitCond( p->pCnf->pVarNums[pObj1->Id], 0 ) );
                Vec_IntPush( p->vClauses, Vec_IntSize(p->vLits) );
                Vec_IntPush( p->vCosts, CostMax );
                nCountImps++;
                continue;
            }
            if ( Fra_ClausSmlNodesAreImpC( pSeq, pObj1, pObj2 ) )
            {
                Vec_IntPush( p->vLits, toLitCond( p->pCnf->pVarNums[pObj1->Id], 1 ) );
                Vec_IntPush( p->vLits, toLitCond( p->pCnf->pVarNums[pObj2->Id], 1 ) );
                Vec_IntPush( p->vClauses, Vec_IntSize(p->vLits) );
                Vec_IntPush( p->vCosts, CostMax );
                nCountImps++;
                continue;
            }
        }
        if ( nCountConst + nCountImps > p->nClausesMax / 2 )
            break;
    }
    pSeq->nWordsPref = 0;
    if ( p->fVerbose )
    printf( "Collected %d register constants and %d one-hotness implications.\n", nCountConst, nCountImps );
    p->nOneHots = nCountConst + nCountImps;
    p->nOneHotsProven = 0;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Processes the clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausProcessClauses( Clu_Man_t * p, int fRefs )
{
    Aig_MmFixed_t * pMemCuts;
//    Aig_ManCut_t * pManCut;
    Fra_Sml_t * pComb, * pSeq;
    Aig_Obj_t * pObj;
    Dar_Cut_t * pCut;
    int Scores[16], uScores, i, k, j, nCuts = 0;
    abctime clk;

    // simulate the AIG
clk = Abc_Clock();
//    srand( 0xAABBAABB );
    Aig_ManRandom(1);
    pSeq = Fra_SmlSimulateSeq( p->pAig, 0, p->nPref + p->nSimFrames, p->nSimWords/p->nSimFrames, 1 );
    if ( p->fTarget && pSeq->fNonConstOut )
    {
        printf( "Property failed after sequential simulation!\n" );
        Fra_SmlStop( pSeq );
        return 0;
    }
if ( p->fVerbose )
{
ABC_PRT( "Sim-seq", Abc_Clock() - clk );
}


clk = Abc_Clock();
    if ( fRefs )
    {
    Fra_ClausCollectLatchClauses( p, pSeq );
if ( p->fVerbose )
{
ABC_PRT( "Lat-cla", Abc_Clock() - clk );
}
    }


    // generate cuts for all nodes, assign cost, and find best cuts
clk = Abc_Clock();
    pMemCuts = Dar_ManComputeCuts( p->pAig, 10, 0, 1 );
//    pManCut = Aig_ComputeCuts( p->pAig, 10, 4, 0, 1 );
if ( p->fVerbose )
{
ABC_PRT( "Cuts   ", Abc_Clock() - clk );
}

    // collect sequential info for each cut
clk = Abc_Clock();
    Aig_ManForEachNode( p->pAig, pObj, i )
        Dar_ObjForEachCut( pObj, pCut, k )
            if ( pCut->nLeaves > 1 )
            {
                pCut->uTruth = Fra_ClausProcessClausesCut( p, pSeq, pCut, Scores );
//                uScores = Fra_ClausProcessClausesCut2( p, pSeq, pCut, Scores );
//                if ( uScores != pCut->uTruth )
//                {
//                    int x = 0;
//                }
            }
if ( p->fVerbose )
{
ABC_PRT( "Infoseq", Abc_Clock() - clk );
}
    Fra_SmlStop( pSeq );

    // perform combinational simulation
clk = Abc_Clock();
//    srand( 0xAABBAABB );
    Aig_ManRandom(1);
    pComb = Fra_SmlSimulateComb( p->pAig, p->nSimWords + p->nSimWordsPref, 0 );
if ( p->fVerbose )
{
ABC_PRT( "Sim-cmb", Abc_Clock() - clk );
}

    // collect combinational info for each cut
clk = Abc_Clock();
    Aig_ManForEachNode( p->pAig, pObj, i )
        Dar_ObjForEachCut( pObj, pCut, k )
            if ( pCut->nLeaves > 1 )
            {
                nCuts++;
                uScores = Fra_ClausProcessClausesCut( p, pComb, pCut, Scores );
                uScores &= ~pCut->uTruth; pCut->uTruth = 0;
                if ( uScores == 0 )
                    continue;
                // write the clauses
                for ( j = 0; j < (1<<pCut->nLeaves); j++ )
                    if ( uScores & (1 << j) )
                        Fra_ClausRecordClause( p, pCut, j, Scores[j] );

            }
    Fra_SmlStop( pComb );
    Aig_MmFixedStop( pMemCuts, 0 );
//    Aig_ManCutStop( pManCut );
if ( p->fVerbose )
{
ABC_PRT( "Infocmb", Abc_Clock() - clk );
}

    if ( p->fVerbose )
    printf( "Node = %5d. Non-triv cuts = %7d. Clauses = %6d. Clause per cut = %6.2f.\n", 
        Aig_ManNodeNum(p->pAig), nCuts, Vec_IntSize(p->vClauses), 1.0*Vec_IntSize(p->vClauses)/nCuts );

    if ( Vec_IntSize(p->vClauses) > p->nClausesMax )
        Fra_ClausSelectClauses( p );
    else
        p->nClauses = Vec_IntSize( p->vClauses );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Processes the clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausProcessClauses2( Clu_Man_t * p, int fRefs )
{
//    Aig_MmFixed_t * pMemCuts;
    Aig_ManCut_t * pManCut;
    Fra_Sml_t * pComb, * pSeq;
    Aig_Obj_t * pObj;
    Aig_Cut_t * pCut;
    int i, k, j, nCuts = 0;
    abctime clk;
    int ScoresSeq[1<<12], ScoresComb[1<<12];
    assert( p->nLutSize < 13 );

    // simulate the AIG
clk = Abc_Clock();
//    srand( 0xAABBAABB );
    Aig_ManRandom(1);
    pSeq = Fra_SmlSimulateSeq( p->pAig, 0, p->nPref + p->nSimFrames, p->nSimWords/p->nSimFrames, 1  );
    if ( p->fTarget && pSeq->fNonConstOut )
    {
        printf( "Property failed after sequential simulation!\n" );
        Fra_SmlStop( pSeq );
        return 0;
    }
if ( p->fVerbose )
{
//ABC_PRT( "Sim-seq", Abc_Clock() - clk );
}

    // perform combinational simulation
clk = Abc_Clock();
//    srand( 0xAABBAABB );
    Aig_ManRandom(1);
    pComb = Fra_SmlSimulateComb( p->pAig, p->nSimWords + p->nSimWordsPref, 0 );
if ( p->fVerbose )
{
//ABC_PRT( "Sim-cmb", Abc_Clock() - clk );
}


clk = Abc_Clock();
    if ( fRefs )
    {
    Fra_ClausCollectLatchClauses( p, pSeq );
if ( p->fVerbose )
{
//ABC_PRT( "Lat-cla", Abc_Clock() - clk );
}
    }


    // generate cuts for all nodes, assign cost, and find best cuts
clk = Abc_Clock();
//    pMemCuts = Dar_ManComputeCuts( p->pAig, 10, 0, 1 );
    pManCut = Aig_ComputeCuts( p->pAig, p->nCutsMax, p->nLutSize, 0, p->fVerbose );
if ( p->fVerbose )
{
//ABC_PRT( "Cuts   ", Abc_Clock() - clk );
}

    // collect combinational info for each cut
clk = Abc_Clock();
    Aig_ManForEachNode( p->pAig, pObj, i )
    {
        if ( pObj->Level > (unsigned)p->nLevels )
            continue;
        Aig_ObjForEachCut( pManCut, pObj, pCut, k )
            if ( pCut->nFanins > 1 )
            {
                nCuts++;
                Fra_ClausProcessClausesCut3( p, pSeq, pCut, ScoresSeq );
                Fra_ClausProcessClausesCut3( p, pComb, pCut, ScoresComb );
                // write the clauses
                for ( j = 0; j < (1<<pCut->nFanins); j++ )
                    if ( ScoresComb[j] != 0 && ScoresSeq[j] == 0 )
                        Fra_ClausRecordClause2( p, pCut, j, ScoresComb[j] );

            }
    }
    Fra_SmlStop( pSeq );
    Fra_SmlStop( pComb );
    p->nCuts = nCuts;
//    Aig_MmFixedStop( pMemCuts, 0 );
    Aig_ManCutStop( pManCut );
    p->pAig->pManCuts = NULL;

    if ( p->fVerbose )
    {
    printf( "Node = %5d. Cuts = %7d. Clauses = %6d. Clause/cut = %6.2f.\n", 
        Aig_ManNodeNum(p->pAig), nCuts, Vec_IntSize(p->vClauses), 1.0*Vec_IntSize(p->vClauses)/nCuts );
    ABC_PRT( "Processing sim-info to find candidate clauses (unoptimized)", Abc_Clock() - clk );
    }

    // filter out clauses that are contained in the already proven clauses
    assert( p->nClauses == 0 );
    p->nClauses = Vec_IntSize( p->vClauses );
    if ( Vec_IntSize( p->vClausesProven ) > 0 )
    {
        int RetValue, k, Beg;
        int End = -1; // Suppress "might be used uninitialized"
        int * pStart;
        // reset the solver
        if ( p->pSatMain )  sat_solver_delete( p->pSatMain );
        p->pSatMain = (sat_solver *)Cnf_DataWriteIntoSolver( p->pCnf, 1, 0 );
        if ( p->pSatMain == NULL )
        {
            printf( "Error: Main solver is unsat.\n" );
            return -1;
        }  

        // add the proven clauses
        Beg = 0;
        pStart = Vec_IntArray(p->vLitsProven);
        Vec_IntForEachEntry( p->vClausesProven, End, i )
        {
            assert( End - Beg <= p->nLutSize );
            // add the clause to all timeframes
            RetValue = sat_solver_addclause( p->pSatMain, pStart + Beg, pStart + End );
            if ( RetValue == 0 )
            {
                printf( "Error: Solver is UNSAT after adding assumption clauses.\n" );
                return -1;
            }
            Beg = End;
        }
        assert( End == Vec_IntSize(p->vLitsProven) );

        // check the clauses
        Beg = 0;
        pStart = Vec_IntArray(p->vLits);
        Vec_IntForEachEntry( p->vClauses, End, i )
        {
            assert( Vec_IntEntry( p->vCosts, i ) >= 0 );
            assert( End - Beg <= p->nLutSize );
            // check the clause
            for ( k = Beg; k < End; k++ )
                pStart[k] = lit_neg( pStart[k] );
            RetValue = sat_solver_solve( p->pSatMain, pStart + Beg, pStart + End, (ABC_INT64_T)p->nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
            for ( k = Beg; k < End; k++ )
                pStart[k] = lit_neg( pStart[k] );
            // the clause holds
            if ( RetValue == l_False )
            {
                Vec_IntWriteEntry( p->vCosts, i, -1 );
                p->nClauses--;
            }
            Beg = End;
        }
        assert( End == Vec_IntSize(p->vLits) );
        if ( p->fVerbose )
        printf( "Already proved clauses filtered out %d candidate clauses (out of %d).\n", 
            Vec_IntSize(p->vClauses) - p->nClauses, Vec_IntSize(p->vClauses) );
    }

    p->fFiltering = 0;
    if ( p->nClauses > p->nClausesMax )
    {
        Fra_ClausSelectClauses( p );
        p->fFiltering = 1;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Converts AIG into the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausBmcClauses( Clu_Man_t * p )
{
    int * pStart, nLitsTot, RetValue, Beg, End, Counter, i, k, f;
/*
    for ( i = 0; i < Vec_IntSize(p->vLits); i++ )
        printf( "%d ", p->vLits->pArray[i] );
    printf( "\n" );
*/
    // add the clauses
    Counter = 0;
    // skip through the prefix variables
    if ( p->nPref )
    {
        nLitsTot = p->nPref * 2 * p->pCnf->nVars;
        for ( i = 0; i < Vec_IntSize(p->vLits); i++ )
            p->vLits->pArray[i] += nLitsTot;
    }
    // go through the timeframes
    nLitsTot = 2 * p->pCnf->nVars;
    pStart = Vec_IntArray(p->vLits);
    for ( f = 0; f < p->nFrames; f++ )
    {
        Beg = 0;
        Vec_IntForEachEntry( p->vClauses, End, i )
        {
            if ( Vec_IntEntry( p->vCosts, i ) == -1 )
            {
                Beg = End;
                continue;
            }
            assert( Vec_IntEntry( p->vCosts, i ) > 0 );
            assert( End - Beg <= p->nLutSize );

            for ( k = Beg; k < End; k++ )
                pStart[k] = lit_neg( pStart[k] );
            RetValue = sat_solver_solve( p->pSatBmc, pStart + Beg, pStart + End, (ABC_INT64_T)p->nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
            for ( k = Beg; k < End; k++ )
                pStart[k] = lit_neg( pStart[k] );

            if ( RetValue != l_False )
            {
                Beg = End;
                Vec_IntWriteEntry( p->vCosts, i, -1 );
                Counter++;
                continue;
            }
/*
            // add the clause
            RetValue = sat_solver_addclause( p->pSatBmc, pStart + Beg, pStart + End );
    //        assert( RetValue == 1 );
            if ( RetValue == 0 )
            {
                printf( "Error: Solver is UNSAT after adding BMC clauses.\n" );
                return -1;
            }
*/
            Beg = End;

            // simplify the solver
            if ( p->pSatBmc->qtail != p->pSatBmc->qhead )
            {
                RetValue = sat_solver_simplify(p->pSatBmc);
                assert( RetValue != 0 );
                assert( p->pSatBmc->qtail == p->pSatBmc->qhead );
            }
        }
        // increment literals
        for ( i = 0; i < Vec_IntSize(p->vLits); i++ )
            p->vLits->pArray[i] += nLitsTot;
    }

    // return clauses back to normal
    nLitsTot = (p->nPref + p->nFrames) * nLitsTot;
    for ( i = 0; i < Vec_IntSize(p->vLits); i++ )
        p->vLits->pArray[i] -= nLitsTot;
/*
    for ( i = 0; i < Vec_IntSize(p->vLits); i++ )
        printf( "%d ", p->vLits->pArray[i] );
    printf( "\n" );
*/
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Cleans simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClausSimInfoClean( Clu_Man_t * p )
{
    assert( p->pCnf->nVars <= Vec_PtrSize(p->vCexes) );
    Vec_PtrCleanSimInfo( p->vCexes, 0, p->nCexesAlloc/32 );
    p->nCexes = 0;
}

/**Function*************************************************************

  Synopsis    [Reallocs simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClausSimInfoRealloc( Clu_Man_t * p )
{
    assert( p->nCexes == p->nCexesAlloc );
    Vec_PtrReallocSimInfo( p->vCexes );
    Vec_PtrCleanSimInfo( p->vCexes, p->nCexesAlloc/32, 2 * p->nCexesAlloc/32 );
    p->nCexesAlloc *= 2;
}

/**Function*************************************************************

  Synopsis    [Records simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClausSimInfoRecord( Clu_Man_t * p, int * pModel )
{
    int i;
    if ( p->nCexes == p->nCexesAlloc )
        Fra_ClausSimInfoRealloc( p );
    assert( p->nCexes < p->nCexesAlloc );
    for ( i = 0; i < p->pCnf->nVars; i++ )
    {
        if ( pModel[i] == l_True )
        {
            assert( Abc_InfoHasBit( (unsigned *)Vec_PtrEntry(p->vCexes, i), p->nCexes ) == 0 );
            Abc_InfoSetBit( (unsigned *)Vec_PtrEntry(p->vCexes, i), p->nCexes );
        }
    }
    p->nCexes++;
}

/**Function*************************************************************

  Synopsis    [Uses the simulation info.]

  Description [Returns 1 if the simulation info disproved the clause.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausSimInfoCheck( Clu_Man_t * p, int * pLits, int nLits )
{
    unsigned * pSims[16], uWord;
    int nWords, iVar, i, w;
    for ( i = 0; i < nLits; i++ )
    {
        iVar = lit_var(pLits[i]) - p->nFrames * p->pCnf->nVars;
        assert( iVar > 0 && iVar < p->pCnf->nVars );
        pSims[i] = (unsigned *)Vec_PtrEntry( p->vCexes, iVar );
    }
    nWords = p->nCexes / 32;
    for ( w = 0; w < nWords; w++ )
    {
        uWord = ~(unsigned)0;
        for ( i = 0; i < nLits; i++ )
            uWord &= (lit_sign(pLits[i])? pSims[i][w] : ~pSims[i][w]);
        if ( uWord )
            return 1;
    }
    if ( p->nCexes % 32 )
    {
        uWord = ~(unsigned)0;
        for ( i = 0; i < nLits; i++ )
            uWord &= (lit_sign(pLits[i])? pSims[i][w] : ~pSims[i][w]);
        if ( uWord & Abc_InfoMask( p->nCexes % 32 ) )
            return 1;
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    [Converts AIG into the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ClausInductiveClauses( Clu_Man_t * p )
{
//    Aig_Obj_t * pObjLi, * pObjLo;
    int * pStart, nLitsTot, RetValue, Beg, End, Counter, i, k, f, fFlag;//, Lits[2];
    p->fFail = 0;

    // reset the solver
    if ( p->pSatMain )  sat_solver_delete( p->pSatMain );
    p->pSatMain = (sat_solver *)Cnf_DataWriteIntoSolver( p->pCnf, p->nFrames+1, 0 );
    if ( p->pSatMain == NULL )
    {
        printf( "Error: Main solver is unsat.\n" );
        return -1;
    }  
    Fra_ClausSimInfoClean( p );

/*
    // check if the property holds
    if ( Fra_ClausRunSat0( p ) )
        printf( "Property holds without strengthening.\n" );
    else
        printf( "Property does not hold without strengthening.\n" );
*/
/*
    // add constant registers
    Aig_ManForEachLiLoSeq( p->pAig, pObjLi, pObjLo, i )  
        if ( Aig_ObjFanin0(pObjLi) == Aig_ManConst1(p->pAig) )
        {
            for ( k = 0; k < p->nFrames; k++ )
            {
                Lits[0] = k * 2 * p->pCnf->nVars + toLitCond( p->pCnf->pVarNums[pObjLo->Id], Aig_ObjFaninC0(pObjLi) ); 
                RetValue = sat_solver_addclause( p->pSatMain, Lits, Lits + 1 );
                if ( RetValue == 0 )
                {
                    printf( "Error: Solver is UNSAT after adding constant-register clauses.\n" );
                    return -1;
                }
            }
        }
*/


    // add the proven clauses
    nLitsTot = 2 * p->pCnf->nVars;
    pStart = Vec_IntArray(p->vLitsProven);
    for ( f = 0; f < p->nFrames; f++ )
    {
        Beg = 0;
        Vec_IntForEachEntry( p->vClausesProven, End, i )
        {
            assert( End - Beg <= p->nLutSize );
            // add the clause to all timeframes
            RetValue = sat_solver_addclause( p->pSatMain, pStart + Beg, pStart + End );
            if ( RetValue == 0 )
            {
                printf( "Error: Solver is UNSAT after adding assumption clauses.\n" );
                return -1;
            }
            Beg = End;
        }
        // increment literals
        for ( i = 0; i < Vec_IntSize(p->vLitsProven); i++ )
            p->vLitsProven->pArray[i] += nLitsTot;
    }
    // return clauses back to normal
    nLitsTot = (p->nFrames) * nLitsTot;
    for ( i = 0; i < Vec_IntSize(p->vLitsProven); i++ )
        p->vLitsProven->pArray[i] -= nLitsTot;

/*
    // add the proven clauses
    nLitsTot = 2 * p->pCnf->nVars;
    pStart = Vec_IntArray(p->vLitsProven);
    Beg = 0;
    Vec_IntForEachEntry( p->vClausesProven, End, i )
    {
        assert( End - Beg <= p->nLutSize );
        // add the clause to all timeframes
        RetValue = sat_solver_addclause( p->pSatMain, pStart + Beg, pStart + End );
        if ( RetValue == 0 )
        {
            printf( "Error: Solver is UNSAT after adding assumption clauses.\n" );
            return -1;
        }
        Beg = End;
    }
*/
    
    // add the clauses
    nLitsTot = 2 * p->pCnf->nVars;
    pStart = Vec_IntArray(p->vLits);
    for ( f = 0; f < p->nFrames; f++ )
    {
        Beg = 0;
        Vec_IntForEachEntry( p->vClauses, End, i )
        {
            if ( Vec_IntEntry( p->vCosts, i ) == -1 )
            {
                Beg = End;
                continue;
            }
            assert( Vec_IntEntry( p->vCosts, i ) > 0 );
            assert( End - Beg <= p->nLutSize );
            // add the clause to all timeframes
            RetValue = sat_solver_addclause( p->pSatMain, pStart + Beg, pStart + End );
            if ( RetValue == 0 )
            {
                printf( "Error: Solver is UNSAT after adding assumption clauses.\n" );
                return -1;
            }
            Beg = End;
        }
        // increment literals
        for ( i = 0; i < Vec_IntSize(p->vLits); i++ )
            p->vLits->pArray[i] += nLitsTot;
    }

    // simplify the solver
    if ( p->pSatMain->qtail != p->pSatMain->qhead )
    {
        RetValue = sat_solver_simplify(p->pSatMain);
        assert( RetValue != 0 );
        assert( p->pSatMain->qtail == p->pSatMain->qhead );
    }
  
    // check if the property holds
    if ( p->fTarget )
    {
        if ( Fra_ClausRunSat0( p ) )
        {
            if ( p->fVerbose )
            printf( " Property holds.  " );
        }
        else
        {
            if ( p->fVerbose )
            printf( " Property fails.  " );
    //        return -2;
            p->fFail = 1;
        }
    }

/*
    // add the property for the first K frames
    for ( i = 0; i < p->nFrames; i++ )
    {
        Aig_Obj_t * pObj;
        int Lits[2];
        // set the output literals
        pObj = Aig_ManCo(p->pAig, 0);
        Lits[0] = i * nLitsTot + toLitCond( p->pCnf->pVarNums[pObj->Id], 1 ); 
        // add the clause
        RetValue = sat_solver_addclause( p->pSatMain, Lits, Lits + 1 );
//        assert( RetValue == 1 );
        if ( RetValue == 0 )
        {
            printf( "Error: Solver is UNSAT after adding property for the first K frames.\n" );
            return -1;
        }
    }
*/

    // simplify the solver
    if ( p->pSatMain->qtail != p->pSatMain->qhead )
    {
        RetValue = sat_solver_simplify(p->pSatMain);
        assert( RetValue != 0 );
        assert( p->pSatMain->qtail == p->pSatMain->qhead );
    }


    // check the clause in the last timeframe
    Beg = 0;
    Counter = 0;
    Vec_IntForEachEntry( p->vClauses, End, i )
    {
        if ( Vec_IntEntry( p->vCosts, i ) == -1 )
        {
            Beg = End;
            continue;
        }
        assert( Vec_IntEntry( p->vCosts, i ) > 0 );
        assert( End - Beg <= p->nLutSize );

        if ( Fra_ClausSimInfoCheck(p, pStart + Beg, End - Beg) )
        {
            fFlag = 1;
//            printf( "s-" );

            Beg = End;
            Vec_IntWriteEntry( p->vCosts, i, -1 );
            Counter++;
            continue;
        }
        else
        {
            fFlag = 0;
//            printf( "s?" );
        }

        for ( k = Beg; k < End; k++ )
            pStart[k] = lit_neg( pStart[k] );
        RetValue = sat_solver_solve( p->pSatMain, pStart + Beg, pStart + End, (ABC_INT64_T)p->nBTLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
        for ( k = Beg; k < End; k++ )
            pStart[k] = lit_neg( pStart[k] );

        // the problem is not solved
        if ( RetValue != l_False )
        {
//            printf( "S- " );
//            Fra_ClausSimInfoRecord( p, (int*)p->pSatMain->model.ptr + p->nFrames * p->pCnf->nVars );
            Fra_ClausSimInfoRecord( p, (int*)p->pSatMain->model + p->nFrames * p->pCnf->nVars );
//            RetValue = Fra_ClausSimInfoCheck(p, pStart + Beg, End - Beg);
//            assert( RetValue );

            Beg = End;
            Vec_IntWriteEntry( p->vCosts, i, -1 );
            Counter++;
            continue;
        }
//        printf( "S+ " );
//        assert( !fFlag );

/*
        // add the clause
        RetValue = sat_solver_addclause( p->pSatMain, pStart + Beg, pStart + End );
//        assert( RetValue == 1 );
        if ( RetValue == 0 )
        {
            printf( "Error: Solver is UNSAT after adding proved clauses.\n" );
            return -1;
        }
*/
        Beg = End;

        // simplify the solver
        if ( p->pSatMain->qtail != p->pSatMain->qhead )
        {
            RetValue = sat_solver_simplify(p->pSatMain);
            assert( RetValue != 0 );
            assert( p->pSatMain->qtail == p->pSatMain->qhead );
        }
    }

    // return clauses back to normal
    nLitsTot = p->nFrames * nLitsTot;
    for ( i = 0; i < Vec_IntSize(p->vLits); i++ )
        p->vLits->pArray[i] -= nLitsTot;

//    if ( fFail )
//        return -2;
    return Counter;
}



/**Function*************************************************************

  Synopsis    [Converts AIG into the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Clu_Man_t * Fra_ClausAlloc( Aig_Man_t * pAig, int nFrames, int nPref, int nClausesMax, int nLutSize, int nLevels, int nCutsMax, int nBatches, int fStepUp, int fTarget, int fVerbose, int fVeryVerbose )
{
    Clu_Man_t * p;
    p = ABC_ALLOC( Clu_Man_t, 1 );
    memset( p, 0, sizeof(Clu_Man_t) );
    p->pAig          = pAig;
    p->nFrames       = nFrames;
    p->nPref         = nPref;
    p->nClausesMax   = nClausesMax;
    p->nLutSize      = nLutSize;
    p->nLevels       = nLevels;
    p->nCutsMax      = nCutsMax;
    p->nBatches      = nBatches;
    p->fStepUp       = fStepUp;
    p->fTarget       = fTarget;
    p->fVerbose      = fVerbose;
    p->fVeryVerbose  = fVeryVerbose;
    p->nSimWords     = 512;//1024;//64;
    p->nSimFrames    = 32;//8;//32;
    p->nSimWordsPref = p->nPref*p->nSimWords/p->nSimFrames;

    p->vLits         = Vec_IntAlloc( 1<<14 );
    p->vClauses      = Vec_IntAlloc( 1<<12 );
    p->vCosts        = Vec_IntAlloc( 1<<12 );

    p->vLitsProven   = Vec_IntAlloc( 1<<14 );
    p->vClausesProven= Vec_IntAlloc( 1<<12 );

    p->nCexesAlloc   = 1024;
    p->vCexes        = Vec_PtrAllocSimInfo( Aig_ManObjNumMax(p->pAig)+1, p->nCexesAlloc/32 );
    Vec_PtrCleanSimInfo( p->vCexes, 0, p->nCexesAlloc/32 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Converts AIG into the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClausFree( Clu_Man_t * p )
{
    if ( p->vCexes )    Vec_PtrFree( p->vCexes );
    if ( p->vLits )     Vec_IntFree( p->vLits );
    if ( p->vClauses )  Vec_IntFree( p->vClauses );
    if ( p->vLitsProven )     Vec_IntFree( p->vLitsProven );
    if ( p->vClausesProven )  Vec_IntFree( p->vClausesProven );
    if ( p->vCosts )    Vec_IntFree( p->vCosts );
    if ( p->pCnf )      Cnf_DataFree( p->pCnf );
    if ( p->pSatMain )  sat_solver_delete( p->pSatMain );
    if ( p->pSatBmc )   sat_solver_delete( p->pSatBmc );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Converts AIG into the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClausAddToStorage( Clu_Man_t * p )
{
    int * pStart; 
    int Beg, End, Counter, i, k;
    Beg = 0;
    Counter = 0;
    pStart = Vec_IntArray( p->vLits );
    Vec_IntForEachEntry( p->vClauses, End, i )
    {
        if ( Vec_IntEntry( p->vCosts, i ) == -1 )
        {
            Beg = End;
            continue;
        }
        assert( Vec_IntEntry( p->vCosts, i ) > 0 );
        assert( End - Beg <= p->nLutSize );
        for ( k = Beg; k < End; k++ )
            Vec_IntPush( p->vLitsProven, pStart[k] );
        Vec_IntPush( p->vClausesProven, Vec_IntSize(p->vLitsProven) );
        Beg = End;
        Counter++;

        if ( i < p->nOneHots )
            p->nOneHotsProven++;
    }
    if ( p->fVerbose )
    printf( "Added to storage %d proved clauses (including %d one-hot clauses)\n", Counter, p->nOneHotsProven );

    Vec_IntClear( p->vClauses );
    Vec_IntClear( p->vLits );
    Vec_IntClear( p->vCosts );
    p->nClauses = 0;

    p->fNothingNew = (int)(Counter == 0);
}

/**Function*************************************************************

  Synopsis    [Converts AIG into the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClausPrintIndClauses( Clu_Man_t * p )
{
    int Counters[9] = {0};
    int * pStart; 
    int Beg, End, i;
    Beg = 0;
    pStart = Vec_IntArray( p->vLitsProven );
    Vec_IntForEachEntry( p->vClausesProven, End, i )
    {
        if ( End - Beg >= 8 )
            Counters[8]++;
        else
            Counters[End - Beg]++;
//printf( "%d ", End-Beg );
        Beg = End;
    }
    printf( "SUMMARY: Total proved clauses = %d. ", Vec_IntSize(p->vClausesProven) );
    printf( "Clause per lit: " );
    for ( i = 0; i < 8; i++ )
        if ( Counters[i] )
            printf( "%d=%d ", i, Counters[i] );
    if ( Counters[8] )
            printf( ">7=%d ", Counters[8] );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Writes the clauses into an AIGER file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Fra_ClausGetLiteral( Clu_Man_t * p, int * pVar2Id, int Lit )
{
    Aig_Obj_t * pLiteral;
    int NodeId = pVar2Id[ lit_var(Lit) ];
    assert( NodeId >= 0 );
    pLiteral = (Aig_Obj_t *)Aig_ManObj( p->pAig, NodeId )->pData;
    return Aig_NotCond( pLiteral, lit_sign(Lit) );
}

/**Function*************************************************************

  Synopsis    [Writes the clauses into an AIGER file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClausWriteIndClauses( Clu_Man_t * p )
{ 
    extern void Ioa_WriteAiger( Aig_Man_t * pMan, char * pFileName, int fWriteSymbols, int fCompact );
    Aig_Man_t * pNew;
    Aig_Obj_t * pClause, * pLiteral;
    char * pName;
    int * pStart, * pVar2Id; 
    int Beg, End, i, k;
    // create mapping from SAT vars to node IDs
    pVar2Id = ABC_ALLOC( int, p->pCnf->nVars );
    memset( pVar2Id, 0xFF, sizeof(int) * p->pCnf->nVars );
    for ( i = 0; i < Aig_ManObjNumMax(p->pAig); i++ )
        if ( p->pCnf->pVarNums[i] >= 0 )
        {
            assert( p->pCnf->pVarNums[i] < p->pCnf->nVars );
            pVar2Id[ p->pCnf->pVarNums[i] ] = i;
        }
    // start the manager
    pNew = Aig_ManDupWithoutPos( p->pAig );
    // add the clauses
    Beg = 0;
    pStart = Vec_IntArray( p->vLitsProven );
    Vec_IntForEachEntry( p->vClausesProven, End, i )
    {
        pClause = Fra_ClausGetLiteral( p, pVar2Id, pStart[Beg] );
        for ( k = Beg + 1; k < End; k++ )
        {
            pLiteral = Fra_ClausGetLiteral( p, pVar2Id, pStart[k] );
            pClause = Aig_Or( pNew, pClause, pLiteral );
        }
        Aig_ObjCreateCo( pNew, pClause );
        Beg = End;
    }
    ABC_FREE( pVar2Id );
    Aig_ManCleanup( pNew );
    pName = Ioa_FileNameGenericAppend( p->pAig->pName, "_care.aig" );
    printf( "Care one-hotness clauses will be written into file \"%s\".\n", pName );
    Ioa_WriteAiger( pNew, pName, 0, 1 );
    Aig_ManStop( pNew );
}

/**Function*************************************************************

  Synopsis    [Checks if the clause holds using the given simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClausEstimateCoverageOne( Fra_Sml_t * pSim, int * pLits, int nLits, int * pVar2Id, unsigned * pResult )
{
    unsigned * pSims[16];
    int iVar, i, w;
    for ( i = 0; i < nLits; i++ )
    {
        iVar = lit_var(pLits[i]);
        pSims[i] = Fra_ObjSim( pSim, pVar2Id[iVar] );
    }
    for ( w = 0; w < pSim->nWordsTotal; w++ )
    {
        pResult[w] = ~(unsigned)0;
        for ( i = 0; i < nLits; i++ )
            pResult[w] &= (lit_sign(pLits[i])? pSims[i][w] : ~pSims[i][w]);
    }
}

/**Function*************************************************************

  Synopsis    [Estimates the coverage of state space by clauses.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ClausEstimateCoverage( Clu_Man_t * p )
{
    int nCombSimWords = (1<<11);
    Fra_Sml_t * pComb;
    unsigned * pResultTot, * pResultOne;
    int nCovered, Beg, End, i, w;
    int * pStart, * pVar2Id; 
    abctime clk = Abc_Clock();
    // simulate the circuit with nCombSimWords * 32 = 64K patterns
//    srand( 0xAABBAABB );
    Aig_ManRandom(1);
    pComb = Fra_SmlSimulateComb( p->pAig, nCombSimWords, 0 );
    // create mapping from SAT vars to node IDs
    pVar2Id = ABC_ALLOC( int, p->pCnf->nVars );
    memset( pVar2Id, 0, sizeof(int) * p->pCnf->nVars );
    for ( i = 0; i < Aig_ManObjNumMax(p->pAig); i++ )
        if ( p->pCnf->pVarNums[i] >= 0 )
        {
            assert( p->pCnf->pVarNums[i] < p->pCnf->nVars );
            pVar2Id[ p->pCnf->pVarNums[i] ] = i;
        }
    // get storage for one assignment and all assignments
    assert( Aig_ManCoNum(p->pAig) > 2 );
    pResultOne = Fra_ObjSim( pComb, Aig_ManCo(p->pAig, 0)->Id );
    pResultTot = Fra_ObjSim( pComb, Aig_ManCo(p->pAig, 1)->Id );
    // start the OR of don't-cares
    for ( w = 0; w < nCombSimWords; w++ )
        pResultTot[w] = 0;
    // check clauses
    Beg = 0;
    pStart = Vec_IntArray( p->vLitsProven );
    Vec_IntForEachEntry( p->vClausesProven, End, i )
    {
        Fra_ClausEstimateCoverageOne( pComb, pStart + Beg, End-Beg, pVar2Id, pResultOne );
        Beg = End;
        for ( w = 0; w < nCombSimWords; w++ )
            pResultTot[w] |= pResultOne[w];
    }
    // count the total number of patterns contained in the don't-care
    nCovered = 0;
    for ( w = 0; w < nCombSimWords; w++ )
        nCovered += Aig_WordCountOnes( pResultTot[w] );
    Fra_SmlStop( pComb );
    ABC_FREE( pVar2Id );
    // print the result
    printf( "Care states ratio = %f. ", 1.0 * (nCombSimWords * 32 - nCovered) / (nCombSimWords * 32) );
    printf( "(%d out of %d patterns)  ", nCombSimWords * 32 - nCovered, nCombSimWords * 32 );
    ABC_PRT( "Time", Abc_Clock() - clk );
}


/**Function*************************************************************

  Synopsis    [Converts AIG into the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_Claus( Aig_Man_t * pAig, int nFrames, int nPref, int nClausesMax, int nLutSize, int nLevels, int nCutsMax, int nBatches, int fStepUp, int fBmc, int fRefs, int fTarget, int fVerbose, int fVeryVerbose )
{
    Clu_Man_t * p;
    abctime clk, clkTotal = Abc_Clock(), clkInd;
    int b, Iter, Counter, nPrefOld;
    int nClausesBeg = 0;

    // create the manager
    p = Fra_ClausAlloc( pAig, nFrames, nPref, nClausesMax, nLutSize, nLevels, nCutsMax, nBatches, fStepUp, fTarget, fVerbose, fVeryVerbose );
if ( p->fVerbose )
{
    printf( "PARAMETERS: Frames = %d. Pref = %d. Clauses max = %d. Cut size = %d.\n", nFrames, nPref, nClausesMax, nLutSize );
    printf( "Level max = %d. Cuts max = %d. Batches = %d. Increment cut size = %s.\n", nLevels, nCutsMax, nBatches, fStepUp? "yes":"no" );
//ABC_PRT( "Sim-seq", Abc_Clock() - clk );
}

    assert( !p->fTarget || Aig_ManCoNum(pAig) - Aig_ManRegNum(pAig) == 1 );

clk = Abc_Clock();
    // derive CNF
//    if ( p->fTarget )
//        p->pAig->nRegs++;
    p->pCnf = Cnf_DeriveSimple( p->pAig, Aig_ManCoNum(p->pAig) );
//    if ( p->fTarget )
//        p->pAig->nRegs--;
if ( fVerbose )
{
//ABC_PRT( "CNF    ", Abc_Clock() - clk );
}

    // check BMC
clk = Abc_Clock();
    p->pSatBmc = (sat_solver *)Cnf_DataWriteIntoSolver( p->pCnf, p->nPref + p->nFrames, 1 );
    if ( p->pSatBmc == NULL )
    {
        printf( "Error: BMC solver is unsat.\n" );
        Fra_ClausFree( p );
        return 1;
    } 
    if ( p->fTarget && !Fra_ClausRunBmc( p ) )
    {
        printf( "Problem fails the base case after %d frame expansion.\n", p->nPref + p->nFrames );
        Fra_ClausFree( p );
        return 1;
    }
if ( fVerbose )
{
//ABC_PRT( "SAT-bmc", Abc_Clock() - clk );
}

    // start the SAT solver
clk = Abc_Clock();
    p->pSatMain = (sat_solver *)Cnf_DataWriteIntoSolver( p->pCnf, p->nFrames+1, 0 );
    if ( p->pSatMain == NULL )
    {
        printf( "Error: Main solver is unsat.\n" );
        Fra_ClausFree( p );
        return 1;
    } 


    for ( b = 0; b < p->nBatches; b++ )
    {
//        if ( fVerbose )
        printf( "*** BATCH %d:  ", b+1 );
        if ( b && p->nLutSize < 12 && (!p->fFiltering || p->fNothingNew || p->fStepUp) )
            p->nLutSize++;
        printf( "Using %d-cuts.\n", p->nLutSize );

        // try solving without additional clauses
        if ( p->fTarget && Fra_ClausRunSat( p ) )
        {
            printf( "Problem is inductive without strengthening.\n" );
            Fra_ClausFree( p );
            return 1;
        }
        if ( fVerbose )
        {
//        ABC_PRT( "SAT-ind", Abc_Clock() - clk );
        }
 
        // collect the candidate inductive clauses using 4-cuts
        clk = Abc_Clock();
        nPrefOld = p->nPref; p->nPref = 0; p->nSimWordsPref = 0;
    //    Fra_ClausProcessClauses( p, fRefs );
        Fra_ClausProcessClauses2( p, fRefs );
        p->nPref = nPrefOld;
        p->nSimWordsPref = p->nPref*p->nSimWords/p->nSimFrames;
        nClausesBeg = p->nClauses;

    //ABC_PRT( "Clauses", Abc_Clock() - clk );


        // check clauses using BMC
        if ( fBmc ) 
        {
            clk = Abc_Clock();
            Counter = Fra_ClausBmcClauses( p );
            p->nClauses -= Counter;
            if ( fVerbose )
            {
                printf( "BMC disproved %d clauses.  ", Counter );
                ABC_PRT( "Time", Abc_Clock() - clk );
            }
        }


        // prove clauses inductively
        clkInd = clk = Abc_Clock();
        Counter = 1;
        for ( Iter = 0; Counter > 0; Iter++ )
        {
            if ( fVerbose )
            printf( "Iter %3d : Begin = %5d. ", Iter, p->nClauses );
            Counter = Fra_ClausInductiveClauses( p );
            if ( Counter > 0 )
               p->nClauses -= Counter;
            if ( fVerbose )
            {
            printf( "End = %5d. Exs = %5d.  ", p->nClauses, p->nCexes );
    //        printf( "\n" );
            ABC_PRT( "Time", Abc_Clock() - clk );
            }
            clk = Abc_Clock();
        }
        // add proved clauses to storage
        Fra_ClausAddToStorage( p );
        // report the results
        if ( p->fTarget )
        {
            if ( Counter == -1 )
                printf( "Fra_Claus(): Internal error.  " );
            else if ( p->fFail )
                printf( "Property FAILS during refinement.  " );
            else
                printf( "Property HOLDS inductively after strengthening.  " );
            ABC_PRT( "Time  ", Abc_Clock() - clkTotal );
            if ( !p->fFail )
                break;
        }
        else
        {
            printf( "Finished proving inductive clauses. " );
            ABC_PRT( "Time  ", Abc_Clock() - clkTotal );
        }
    }

    // verify the computed interpolant
    Fra_InvariantVerify( pAig, nFrames, p->vClausesProven, p->vLitsProven );
//    printf( "THIS COMMAND IS KNOWN TO HAVE A BUG!\n" );

//    if ( !p->fTarget && p->fVerbose )
    if ( p->fVerbose )
    {
        Fra_ClausPrintIndClauses( p );
        Fra_ClausEstimateCoverage( p );
    }

    if ( !p->fTarget )
    {
        Fra_ClausWriteIndClauses( p );
    }
/*
    // print the statistic into a file
    {
        FILE * pTable;
        assert( p->nBatches == 1 );
        pTable = fopen( "stats.txt", "a+" );
        fprintf( pTable, "%s ",  pAig->pName );
        fprintf( pTable, "%d ",  Aig_ManCiNum(pAig)-Aig_ManRegNum(pAig) );
        fprintf( pTable, "%d ",  Aig_ManCoNum(pAig)-Aig_ManRegNum(pAig) );
        fprintf( pTable, "%d ",  Aig_ManRegNum(pAig) );
        fprintf( pTable, "%d ",  Aig_ManNodeNum(pAig) );
        fprintf( pTable, "%d ",  p->nCuts );
        fprintf( pTable, "%d ",  nClausesBeg );
        fprintf( pTable, "%d ",  p->nClauses );
        fprintf( pTable, "%d ",  Iter );
        fprintf( pTable, "%.2f ", (float)(clkInd-clkTotal)/(float)(CLOCKS_PER_SEC) );
        fprintf( pTable, "%.2f ", (float)(Abc_Clock()-clkInd)/(float)(CLOCKS_PER_SEC) );
        fprintf( pTable, "%.2f ", (float)(Abc_Clock()-clkTotal)/(float)(CLOCKS_PER_SEC) );
        fprintf( pTable, "\n" );
        fclose( pTable );
    }
*/
    // clean the manager
    Fra_ClausFree( p );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

