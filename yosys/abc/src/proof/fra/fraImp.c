/**CFile****************************************************************

  FileName    [fraImp.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    [Detecting and proving implications.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraImp.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Counts the number of 1s in each siminfo of each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Fra_SmlCountOnesOne( Fra_Sml_t * p, int Node )
{
    unsigned * pSim;
    int k, Counter = 0;
    pSim = Fra_ObjSim( p, Node );
    for ( k = p->nWordsPref; k < p->nWordsTotal; k++ )
        Counter += Aig_WordCountOnes( pSim[k] );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of 1s in each siminfo of each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int * Fra_SmlCountOnes( Fra_Sml_t * p )
{
    Aig_Obj_t * pObj;
    int i, * pnBits; 
    pnBits = ABC_ALLOC( int, Aig_ManObjNumMax(p->pAig) );  
    memset( pnBits, 0, sizeof(int) * Aig_ManObjNumMax(p->pAig) );
    Aig_ManForEachObj( p->pAig, pObj, i )
        pnBits[i] = Fra_SmlCountOnesOne( p, i );
    return pnBits;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if implications holds.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Sml_NodeCheckImp( Fra_Sml_t * p, int Left, int Right )
{
    unsigned * pSimL, * pSimR;
    int k;
    pSimL = Fra_ObjSim( p, Left );
    pSimR = Fra_ObjSim( p, Right );
    for ( k = p->nWordsPref; k < p->nWordsTotal; k++ )
        if ( pSimL[k] & ~pSimR[k] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Counts the number of 1s in the complement of the implication.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Sml_NodeNotImpWeight( Fra_Sml_t * p, int Left, int Right )
{
    unsigned * pSimL, * pSimR;
    int k, Counter = 0;
    pSimL = Fra_ObjSim( p, Left );
    pSimR = Fra_ObjSim( p, Right );
    for ( k = p->nWordsPref; k < p->nWordsTotal; k++ )
        Counter += Aig_WordCountOnes( pSimL[k] & ~pSimR[k] );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Computes the complement of the implication.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Sml_NodeSaveNotImpPatterns( Fra_Sml_t * p, int Left, int Right, unsigned * pResult )
{
    unsigned * pSimL, * pSimR;
    int k;
    pSimL = Fra_ObjSim( p, Left );
    pSimR = Fra_ObjSim( p, Right );
    for ( k = p->nWordsPref; k < p->nWordsTotal; k++ )
        pResult[k] |= pSimL[k] & ~pSimR[k];
}

/**Function*************************************************************

  Synopsis    [Returns the array of nodes sorted by the number of 1s.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Fra_SmlSortUsingOnes( Fra_Sml_t * p, int fLatchCorr )
{
    Aig_Obj_t * pObj;
    Vec_Ptr_t * vNodes;
    int i, nNodes, nTotal, nBits, * pnNodes, * pnBits, * pMemory;
    assert( p->nWordsTotal > 0 );
    // count 1s in each node's siminfo
    pnBits = Fra_SmlCountOnes( p );
    // count number of nodes having that many 1s
    nNodes = 0;
    nBits = p->nWordsTotal * 32;
    pnNodes = ABC_ALLOC( int, nBits + 1 );
    memset( pnNodes, 0, sizeof(int) * (nBits + 1) );
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        if ( i == 0 ) continue;
        // skip non-PI and non-internal nodes
        if ( fLatchCorr )
        {
            if ( !Aig_ObjIsCi(pObj) )
                continue;
        }
        else
        {
            if ( !Aig_ObjIsNode(pObj) && !Aig_ObjIsCi(pObj) )
                continue;
        }
        // skip nodes participating in the classes
//        if ( Fra_ClassObjRepr(pObj) )
//            continue;
        assert( pnBits[i] <= nBits ); // "<" because of normalized info
        pnNodes[pnBits[i]]++;
        nNodes++;
    }
    // allocate memory for all the nodes
    pMemory = ABC_ALLOC( int, nNodes + nBits + 1 );  
    // markup the memory for each node
    vNodes = Vec_PtrAlloc( nBits + 1 );
    Vec_PtrPush( vNodes, pMemory );
    for ( i = 1; i <= nBits; i++ )
    {
        pMemory += pnNodes[i-1] + 1;
        Vec_PtrPush( vNodes, pMemory );
    }
    // add the nodes
    memset( pnNodes, 0, sizeof(int) * (nBits + 1) );
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        if ( i == 0 ) continue;
        // skip non-PI and non-internal nodes
        if ( fLatchCorr )
        {
            if ( !Aig_ObjIsCi(pObj) )
                continue;
        }
        else
        {
            if ( !Aig_ObjIsNode(pObj) && !Aig_ObjIsCi(pObj) )
                continue;
        }
        // skip nodes participating in the classes
//        if ( Fra_ClassObjRepr(pObj) )
//            continue;
        pMemory = (int *)Vec_PtrEntry( vNodes, pnBits[i] );
        pMemory[ pnNodes[pnBits[i]]++ ] = i;
    }
    // add 0s in the end
    nTotal = 0;
    Vec_PtrForEachEntry( int *, vNodes, pMemory, i )
    {
        pMemory[ pnNodes[i]++ ] = 0;
        nTotal += pnNodes[i];
    }
    assert( nTotal == nNodes + nBits + 1 );
    ABC_FREE( pnNodes );
    ABC_FREE( pnBits );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Returns the array of implications with the highest cost.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Fra_SmlSelectMaxCost( Vec_Int_t * vImps, int * pCosts, int nCostMax, int nImpLimit, int * pCostRange )
{
    Vec_Int_t * vImpsNew;
    int * pCostCount, nImpCount, Imp, i, c;
    assert( Vec_IntSize(vImps) >= nImpLimit );
    // count how many implications have each cost
    pCostCount = ABC_ALLOC( int, nCostMax + 1 );
    memset( pCostCount, 0, sizeof(int) * (nCostMax + 1) );
    for ( i = 0; i < Vec_IntSize(vImps); i++ )
    {
        assert( pCosts[i] <= nCostMax );
        pCostCount[ pCosts[i] ]++;
    }
    assert( pCostCount[0] == 0 );
    // select the bound on the cost (above this bound, implication will be included)
    nImpCount = 0;
    for ( c = nCostMax; c > 0; c-- )
    {
        nImpCount += pCostCount[c];
        if ( nImpCount >= nImpLimit )
            break;
    }
//    printf( "Cost range >= %d.\n", c );
    // collect implications with the given costs
    vImpsNew = Vec_IntAlloc( nImpLimit );
    Vec_IntForEachEntry( vImps, Imp, i )
    {
        if ( pCosts[i] < c )
            continue;
        Vec_IntPush( vImpsNew, Imp );
        if ( Vec_IntSize( vImpsNew ) == nImpLimit )
            break;
    }
    ABC_FREE( pCostCount );
    if ( pCostRange )
        *pCostRange = c;
    return vImpsNew;
}

/**Function*************************************************************

  Synopsis    [Compares two implications using their largest ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sml_CompareMaxId( unsigned short * pImp1, unsigned short * pImp2 )
{
    int Max1 = Abc_MaxInt( pImp1[0], pImp1[1] );
    int Max2 = Abc_MaxInt( pImp2[0], pImp2[1] );
    if ( Max1 < Max2 )
        return -1;
    if ( Max1 > Max2  )
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Derives implication candidates.]

  Description [Implication candidates have the property that 
  (1) they hold using sequential simulation information
  (2) they do not hold using combinational simulation information
  (3) they have as high expressive power as possible (heuristically)
      that is, they are easy to disprove combinationally
      meaning they cover relatively larger sequential subspace.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Fra_ImpDerive( Fra_Man_t * p, int nImpMaxLimit, int nImpUseLimit, int fLatchCorr )
{
    int nSimWords = 64;
    Fra_Sml_t * pSeq, * pComb;
    Vec_Int_t * vImps, * vTemp;
    Vec_Ptr_t * vNodes;
    int * pImpCosts, * pNodesI, * pNodesK;
    int nImpsTotal = 0, nImpsTried = 0, nImpsNonSeq = 0, nImpsComb = 0, nImpsCollected = 0;
    int CostMin = ABC_INFINITY, CostMax = 0;
    int i, k, Imp, CostRange;
    abctime clk = Abc_Clock();
    assert( Aig_ManObjNumMax(p->pManAig) < (1 << 15) );
    assert( nImpMaxLimit > 0 && nImpUseLimit > 0 && nImpUseLimit <= nImpMaxLimit );
    // normalize both managers
    pComb = Fra_SmlSimulateComb( p->pManAig, nSimWords, 0 );
    pSeq = Fra_SmlSimulateSeq( p->pManAig, p->pPars->nFramesP, nSimWords, 1, 1 );
    // get the nodes sorted by the number of 1s
    vNodes = Fra_SmlSortUsingOnes( pSeq, fLatchCorr );
    // count the total number of implications
    for ( k = nSimWords * 32; k > 0; k-- )
    for ( i = k - 1; i > 0; i-- )
    for ( pNodesI = (int *)Vec_PtrEntry( vNodes, i ); *pNodesI; pNodesI++ )
    for ( pNodesK = (int *)Vec_PtrEntry( vNodes, k ); *pNodesK; pNodesK++ )
        nImpsTotal++;

    // compute implications and their costs
    pImpCosts = ABC_ALLOC( int, nImpMaxLimit );
    vImps = Vec_IntAlloc( nImpMaxLimit );
    for ( k = pSeq->nWordsTotal * 32; k > 0; k-- )
        for ( i = k - 1; i > 0; i-- )
        {
            // HERE WE ARE MISSING SOME POTENTIAL IMPLICATIONS (with complement!)

            for ( pNodesI = (int *)Vec_PtrEntry( vNodes, i ); *pNodesI; pNodesI++ )
            for ( pNodesK = (int *)Vec_PtrEntry( vNodes, k ); *pNodesK; pNodesK++ )
            {
                nImpsTried++;
                if ( !Sml_NodeCheckImp(pSeq, *pNodesI, *pNodesK) )
                {
                    nImpsNonSeq++;
                    continue;
                }
                if ( Sml_NodeCheckImp(pComb, *pNodesI, *pNodesK) )
                {
                    nImpsComb++;
                    continue;
                }
                nImpsCollected++;
                Imp = Fra_ImpCreate( *pNodesI, *pNodesK );
                pImpCosts[ Vec_IntSize(vImps) ] = Sml_NodeNotImpWeight(pComb, *pNodesI, *pNodesK);
                CostMin = Abc_MinInt( CostMin, pImpCosts[ Vec_IntSize(vImps) ] );
                CostMax = Abc_MaxInt( CostMax, pImpCosts[ Vec_IntSize(vImps) ] );
                Vec_IntPush( vImps, Imp );
                if ( Vec_IntSize(vImps) == nImpMaxLimit )
                    goto finish;
            } 
        }
finish:
    Fra_SmlStop( pComb );
    Fra_SmlStop( pSeq );

    // select implications with the highest cost
    CostRange = CostMin;
    if ( Vec_IntSize(vImps) > nImpUseLimit )
    {
        vImps = Fra_SmlSelectMaxCost( vTemp = vImps, pImpCosts, nSimWords * 32, nImpUseLimit, &CostRange );
        Vec_IntFree( vTemp );  
    }

    // dealloc
    ABC_FREE( pImpCosts ); 
    {
    void * pTemp = Vec_PtrEntry(vNodes, 0);
    ABC_FREE( pTemp );
    }
    Vec_PtrFree( vNodes );
    // reorder implications topologically
    qsort( (void *)Vec_IntArray(vImps), (size_t)Vec_IntSize(vImps), sizeof(int), 
            (int (*)(const void *, const void *)) Sml_CompareMaxId );
if ( p->pPars->fVerbose )
{
printf( "Implications: All = %d. Try = %d. NonSeq = %d. Comb = %d. Res = %d.\n", 
    nImpsTotal, nImpsTried, nImpsNonSeq, nImpsComb, nImpsCollected );
printf( "Implication weight: Min = %d. Pivot = %d. Max = %d.   ", 
       CostMin, CostRange, CostMax );
ABC_PRT( "Time", Abc_Clock() - clk );
}
    return vImps;
}


// the following three procedures are called to 
// - add implications to the SAT solver
// - check implications using the SAT solver
// - refine implications using after a cex is generated

/**Function*************************************************************

  Synopsis    [Add implication clauses to the SAT solver.]

  Description [Note that implications should be checked in the first frame!]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ImpAddToSolver( Fra_Man_t * p, Vec_Int_t * vImps, int * pSatVarNums )
{
    sat_solver * pSat = p->pSat;
    Aig_Obj_t * pLeft, * pRight;
    Aig_Obj_t * pLeftF, * pRightF;
    int pLits[2], Imp, Left, Right, i, f, status;
    int fComplL, fComplR;
    Vec_IntForEachEntry( vImps, Imp, i )
    {
        // get the corresponding nodes
        pLeft = Aig_ManObj( p->pManAig, Fra_ImpLeft(Imp) );
        pRight = Aig_ManObj( p->pManAig, Fra_ImpRight(Imp) );
        // check if all the nodes are present
        for ( f = 0; f < p->pPars->nFramesK; f++ )
        {
            // map these info fraig
            pLeftF = Fra_ObjFraig( pLeft, f );
            pRightF = Fra_ObjFraig( pRight, f );
            if ( Aig_ObjIsNone(Aig_Regular(pLeftF)) || Aig_ObjIsNone(Aig_Regular(pRightF)) )
            {
                Vec_IntWriteEntry( vImps, i, 0 );
                break;
            }
        } 
        if ( f < p->pPars->nFramesK )
            continue;
        // add constraints in each timeframe
        for ( f = 0; f < p->pPars->nFramesK; f++ )
        {
            // map these info fraig
            pLeftF = Fra_ObjFraig( pLeft, f );
            pRightF = Fra_ObjFraig( pRight, f );
            // get the corresponding SAT numbers
            Left = pSatVarNums[ Aig_Regular(pLeftF)->Id ];
            Right = pSatVarNums[ Aig_Regular(pRightF)->Id ];
            assert( Left > 0  && Left < p->nSatVars );
            assert( Right > 0 && Right < p->nSatVars );
            // get the complemented attributes
            fComplL = pLeft->fPhase ^ Aig_IsComplement(pLeftF);
            fComplR = pRight->fPhase ^ Aig_IsComplement(pRightF);
            // get the constraint
            // L => R      L' v R     (complement = L & R')
            pLits[0] = 2 * Left  + !fComplL;
            pLits[1] = 2 * Right +  fComplR;
            // add constraint to solver
            if ( !sat_solver_addclause( pSat, pLits, pLits + 2 ) )
            {
                sat_solver_delete( pSat );
                p->pSat = NULL;
                return;
            }
        }
    }
    status = sat_solver_simplify(pSat);
    if ( status == 0 )
    {
        sat_solver_delete( pSat );
        p->pSat = NULL;
    }
//    printf( "Total imps = %d. ", Vec_IntSize(vImps) );
    Fra_ImpCompactArray( vImps );
//    printf( "Valid imps = %d. \n", Vec_IntSize(vImps) );
}

/**Function*************************************************************

  Synopsis    [Check implications for the node (if they are present).]

  Description [Returns the new position in the array.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ImpCheckForNode( Fra_Man_t * p, Vec_Int_t * vImps, Aig_Obj_t * pNode, int Pos )
{
    Aig_Obj_t * pLeft, * pRight;
    Aig_Obj_t * pLeftF, * pRightF;
    int i, Imp, Left, Right, Max, RetValue;
    int fComplL, fComplR;
    Vec_IntForEachEntryStart( vImps, Imp, i, Pos )
    {
        if ( Imp == 0 )
            continue;
        Left = Fra_ImpLeft(Imp);
        Right = Fra_ImpRight(Imp);
        Max = Abc_MaxInt( Left, Right );
        assert( Max >= pNode->Id );
        if ( Max > pNode->Id )
            return i;
        // get the corresponding nodes
        pLeft  = Aig_ManObj( p->pManAig, Left );
        pRight = Aig_ManObj( p->pManAig, Right );
        // get the corresponding FRAIG nodes
        pLeftF  = Fra_ObjFraig( pLeft, p->pPars->nFramesK );
        pRightF = Fra_ObjFraig( pRight, p->pPars->nFramesK );
        // get the complemented attributes
        fComplL = pLeft->fPhase ^ Aig_IsComplement(pLeftF);
        fComplR = pRight->fPhase ^ Aig_IsComplement(pRightF);
        // check equality
        if ( Aig_Regular(pLeftF) == Aig_Regular(pRightF) )
        {
            if ( fComplL == fComplR ) // x => x  - always true
                continue;
            assert( fComplL != fComplR );
            // consider 4 possibilities:
            // NOT(1) => 1    or   0 => 1  - always true
            // 1 => NOT(1)    or   1 => 0  - never true
            // NOT(x) => x    or   x       - not always true
            // x => NOT(x)    or   NOT(x)  - not always true
            if ( Aig_ObjIsConst1(Aig_Regular(pLeftF)) && fComplL ) // proved implication
                continue;
            // disproved implication
            p->pCla->fRefinement = 1;
            Vec_IntWriteEntry( vImps, i, 0 );
            continue;
        }
        // check the implication 
        // - if true, a clause is added
        // - if false, a cex is simulated
        // make sure the implication is refined
        RetValue = Fra_NodesAreImp( p, Aig_Regular(pLeftF), Aig_Regular(pRightF), fComplL, fComplR );
        if ( RetValue != 1 )
        {
            p->pCla->fRefinement = 1;
            if ( RetValue == 0 )
                Fra_SmlResimulate( p );
            if ( Vec_IntEntry(vImps, i) != 0 )
                printf( "Fra_ImpCheckForNode(): Implication is not refined!\n" );
            assert( Vec_IntEntry(vImps, i) == 0 );
        }
    }
    return i;
}

/**Function*************************************************************

  Synopsis    [Removes those implications that no longer hold.]

  Description [Returns 1 if refinement has happened.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ImpRefineUsingCex( Fra_Man_t * p, Vec_Int_t * vImps )
{
    Aig_Obj_t * pLeft, * pRight;
    int Imp, i, RetValue = 0;
    Vec_IntForEachEntry( vImps, Imp, i )
    {
        if ( Imp == 0 )
            continue;
        // get the corresponding nodes
        pLeft = Aig_ManObj( p->pManAig, Fra_ImpLeft(Imp) );
        pRight = Aig_ManObj( p->pManAig, Fra_ImpRight(Imp) );
        // check if implication holds using this simulation info
        if ( !Sml_NodeCheckImp(p->pSml, pLeft->Id, pRight->Id) )
        {
            Vec_IntWriteEntry( vImps, i, 0 );
            RetValue = 1;
        }
    }
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Removes empty implications.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ImpCompactArray( Vec_Int_t * vImps )
{
    int i, k, Imp;
    k = 0;
    Vec_IntForEachEntry( vImps, Imp, i )
        if ( Imp )
            Vec_IntWriteEntry( vImps, k++, Imp );
    Vec_IntShrink( vImps, k );
}

/**Function*************************************************************

  Synopsis    [Determines the ratio of the state space by computed implications.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
double Fra_ImpComputeStateSpaceRatio( Fra_Man_t * p )
{
    int nSimWords = 64;
    Fra_Sml_t * pComb;
    unsigned * pResult;
    double Ratio = 0.0;
    int Left, Right, Imp, i;
    if ( p->pCla->vImps == NULL || Vec_IntSize(p->pCla->vImps) == 0 )
        return Ratio;
    // simulate the AIG manager with combinational patterns
    pComb = Fra_SmlSimulateComb( p->pManAig, nSimWords, 0 );
    // go through the implications and collect where they do not hold
    pResult = Fra_ObjSim( pComb, 0 );
    assert( pResult[0] == 0 );
    Vec_IntForEachEntry( p->pCla->vImps, Imp, i )
    {
        Left = Fra_ImpLeft(Imp);
        Right = Fra_ImpRight(Imp);
        Sml_NodeSaveNotImpPatterns( pComb, Left, Right, pResult );
    }
    // count the number of ones in this area
    Ratio = 100.0 * Fra_SmlCountOnesOne( pComb, 0 ) / (32*(pComb->nWordsTotal-pComb->nWordsPref));
    Fra_SmlStop( pComb );
    return Ratio;
}

/**Function*************************************************************

  Synopsis    [Returns the number of failed implications.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_ImpVerifyUsingSimulation( Fra_Man_t * p )
{
    int nFrames = 2000;
    int nSimWords = 8;
    Fra_Sml_t * pSeq;
    char * pfFails;
    int Left, Right, Imp, i, Counter;
    if ( p->pCla->vImps == NULL || Vec_IntSize(p->pCla->vImps) == 0 )
        return 0;
    // simulate the AIG manager with combinational patterns
    pSeq = Fra_SmlSimulateSeq( p->pManAig, p->pPars->nFramesP, nFrames, nSimWords, 1  );
    // go through the implications and check how many of them do not hold
    pfFails = ABC_ALLOC( char, Vec_IntSize(p->pCla->vImps) );
    memset( pfFails, 0, sizeof(char) * Vec_IntSize(p->pCla->vImps) );
    Vec_IntForEachEntry( p->pCla->vImps, Imp, i )
    {
        Left = Fra_ImpLeft(Imp);
        Right = Fra_ImpRight(Imp);
        pfFails[i] = !Sml_NodeCheckImp( pSeq, Left, Right );
    }
    // count how many has failed
    Counter = 0;
    for ( i = 0; i < Vec_IntSize(p->pCla->vImps); i++ )
        Counter += pfFails[i];
    ABC_FREE( pfFails );
    Fra_SmlStop( pSeq );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Record proven implications in the AIG manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fra_ImpRecordInManager( Fra_Man_t * p, Aig_Man_t * pNew )
{
    Aig_Obj_t * pLeft, * pRight, * pMiter;
    int nPosOld, Imp, i;
    if ( p->pCla->vImps == NULL || Vec_IntSize(p->pCla->vImps) == 0 )
        return;
    // go through the implication
    nPosOld = Aig_ManCoNum(pNew);
    Vec_IntForEachEntry( p->pCla->vImps, Imp, i )
    {
        pLeft = Aig_ManObj( p->pManAig, Fra_ImpLeft(Imp) );
        pRight = Aig_ManObj( p->pManAig, Fra_ImpRight(Imp) );
        // record the implication: L' + R
        pMiter = Aig_Or( pNew, 
            Aig_NotCond((Aig_Obj_t *)pLeft->pData, !pLeft->fPhase), 
            Aig_NotCond((Aig_Obj_t *)pRight->pData, pRight->fPhase) ); 
        Aig_ObjCreateCo( pNew, pMiter );
    }
    pNew->nAsserts = Aig_ManCoNum(pNew) - nPosOld;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

