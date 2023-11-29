/**CFile****************************************************************

  FileName    [sfmCore.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [Core procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sfmCore.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sfmInt.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Setup parameter structure.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_ParSetDefault( Sfm_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Sfm_Par_t) );
    pPars->nTfoLevMax   =    2;  // the maximum fanout levels
    pPars->nFanoutMax   =   30;  // the maximum number of fanouts
    pPars->nDepthMax    =   20;  // the maximum depth to try  
    pPars->nWinSizeMax  =  300;  // the maximum window size
    pPars->nGrowthLevel =    0;  // the maximum allowed growth in level
    pPars->nBTLimit     = 5000;  // the maximum number of conflicts in one SAT run
    pPars->fRrOnly      =    0;  // perform redundancy removal
    pPars->fArea        =    0;  // performs optimization for area
    pPars->fMoreEffort  =    0;  // performs high-affort minimization
    pPars->fAllBoxes    =    0;  // enable preserving all boxes
    pPars->fVerbose     =    0;  // enable basic stats
    pPars->fVeryVerbose =    0;  // enable detailed stats
}

/**Function*************************************************************

  Synopsis    [Prints statistics.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_NtkPrintStats( Sfm_Ntk_t * p )
{
    p->timeOther = p->timeTotal - p->timeWin - p->timeDiv - p->timeCnf - p->timeSat;
    printf( "Nodes = %d. Try = %d. Resub = %d. Div = %d (ave = %d). SAT calls = %d. Timeouts = %d. MaxDivs = %d.\n",
        Sfm_NtkNodeNum(p), p->nNodesTried, p->nRemoves + p->nResubs, p->nTotalDivs, p->nTotalDivs/Abc_MaxInt(1, p->nNodesTried), p->nSatCalls, p->nTimeOuts, p->nMaxDivs );

    printf( "Attempts :   " );
    printf( "Remove %6d out of %6d (%6.2f %%)   ", p->nRemoves, p->nTryRemoves, 100.0*p->nRemoves/Abc_MaxInt(1, p->nTryRemoves) );
    printf( "Resub  %6d out of %6d (%6.2f %%)   ", p->nResubs,  p->nTryResubs,  100.0*p->nResubs /Abc_MaxInt(1, p->nTryResubs)  );
    if ( p->pPars->fUseDcs )
    printf( "Improves %6d out of %6d (%6.2f %%)   ", p->nImproves,p->nTryImproves,100.0*p->nImproves/Abc_MaxInt(1, p->nTryImproves));
    printf( "\n" );

    printf( "Reduction:   " );
    printf( "Nodes  %6d out of %6d (%6.2f %%)   ", p->nTotalNodesBeg-p->nTotalNodesEnd, p->nTotalNodesBeg, 100.0*(p->nTotalNodesBeg-p->nTotalNodesEnd)/Abc_MaxInt(1, p->nTotalNodesBeg) );
    printf( "Edges  %6d out of %6d (%6.2f %%)   ", p->nTotalEdgesBeg-p->nTotalEdgesEnd, p->nTotalEdgesBeg, 100.0*(p->nTotalEdgesBeg-p->nTotalEdgesEnd)/Abc_MaxInt(1, p->nTotalEdgesBeg) );
    printf( "\n" );

    ABC_PRTP( "Win", p->timeWin  ,  p->timeTotal );
    ABC_PRTP( "Div", p->timeDiv  ,  p->timeTotal );
    ABC_PRTP( "Cnf", p->timeCnf  ,  p->timeTotal );
    ABC_PRTP( "Sat", p->timeSat  ,  p->timeTotal );
    ABC_PRTP( "Oth", p->timeOther,  p->timeTotal );
    ABC_PRTP( "ALL", p->timeTotal,  p->timeTotal );
//    ABC_PRTP( "   ", p->time1    ,  p->timeTotal );
}

/**Function*************************************************************

  Synopsis    [Performs resubstitution for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_NodeResubSolve( Sfm_Ntk_t * p, int iNode, int f, int fRemoveOnly )
{
    int fSkipUpdate  = 0;
    int fVeryVerbose = 0;//p->pPars->fVeryVerbose && Vec_IntSize(p->vDivs) < 200;// || pNode->Id == 556;
    int i, iFanin, iVar = -1;
    int iFaninRem = -1, iFaninSkip = -1;
    int nFanins = Sfm_ObjFaninNum(p, iNode); 
    word uTruth, uSign, uMask;
    abctime clk;
    assert( Sfm_ObjIsNode(p, iNode) );
    assert( f >= 0 && f < Sfm_ObjFaninNum(p, iNode) );
    p->nTryRemoves++;
    // report init stats
    if ( p->pPars->fVeryVerbose )
        printf( "%5d : Lev =%3d. Leaf =%3d.  Node =%3d.  Div=%3d.  Fanin =%4d (%d/%d). MFFC = %d\n", 
            iNode, Sfm_ObjLevel(p, iNode), 0, Vec_IntSize(p->vNodes), Vec_IntSize(p->vDivs), 
            Sfm_ObjFanin(p, iNode, f), f, Sfm_ObjFaninNum(p, iNode), Sfm_ObjMffcSize(p, Sfm_ObjFanin(p, iNode, f)) );
    // clean simulation info
    p->nCexes = 0;
    Vec_WrdFill( p->vDivCexes, Vec_IntSize(p->vDivs), 0 );
    // try removing the critical fanin
    Vec_IntClear( p->vDivIds );
    Sfm_ObjForEachFanin( p, iNode, iFanin, i )
        if ( i != f )
            Vec_IntPush( p->vDivIds, Sfm_ObjSatVar(p, iFanin) );
        else
            iFaninRem = iFanin;
    assert( iFaninRem != -1 );
    // find fanin to skip 
    if ( Sfm_ObjIsFixed(p, iFaninRem) && Sfm_ObjFaninNum(p, iFaninRem) == 1 )
        iFaninSkip = Sfm_ObjFanin(p, iFaninRem, 0);
clk = Abc_Clock();
    uTruth = Sfm_ComputeInterpolant( p );
p->timeSat += Abc_Clock() - clk;
    // analyze outcomes
    if ( uTruth == SFM_SAT_UNDEC )
    {
        p->nTimeOuts++;
        return 0;
    }
    if ( uTruth != SFM_SAT_SAT )
        goto finish;
    if ( fRemoveOnly || p->pPars->fRrOnly || Vec_IntSize(p->vDivs) == 0 )
        return 0;

    p->nTryResubs++;
    if ( fVeryVerbose )
    {
        for ( i = 0; i < 9; i++ )
            printf( " " );
        for ( i = 0; i < Vec_IntSize(p->vDivs); i++ )
            printf( "%d", i % 10 );
        printf( "\n" );
    }
    while ( 1 ) 
    {
        if ( fVeryVerbose )
        {
            printf( "%3d: %3d ", p->nCexes, iVar );
            Vec_WrdForEachEntry( p->vDivCexes, uSign, i )
                printf( "%d", Abc_InfoHasBit((unsigned *)&uSign, p->nCexes-1) );
            printf( "\n" );
        }
        // find the next divisor to try
        uMask = (~(word)0) >> (64 - p->nCexes);
        Vec_WrdForEachEntry( p->vDivCexes, uSign, iVar )
            if ( uSign == uMask && Vec_IntEntry(p->vDivs, iVar) != iFaninSkip )
                break;
        if ( iVar == Vec_IntSize(p->vDivs) )
            return 0;
        assert( Vec_IntEntry(p->vDivs, iVar) != iFaninSkip );
        // try replacing the critical fanin
        Vec_IntPush( p->vDivIds, Sfm_ObjSatVar(p, Vec_IntEntry(p->vDivs, iVar)) );
clk = Abc_Clock();
        uTruth = Sfm_ComputeInterpolant( p );
p->timeSat += Abc_Clock() - clk;
        // analyze outcomes
        if ( uTruth == SFM_SAT_UNDEC )
        {
            p->nTimeOuts++;
            return 0;
        }
        if ( uTruth != SFM_SAT_SAT )
            goto finish;
        if ( p->nCexes == 64 )
            return 0;
        // remove the last variable
        Vec_IntPop( p->vDivIds );
    }
finish:
    if ( p->pPars->fVeryVerbose )
    {
        if ( iVar == -1 )
            printf( "Node %d: Fanin %d (%d) can be removed.  ", iNode, f, Sfm_ObjFanin(p, iNode, f) );
        else
            printf( "Node %d: Fanin %d (%d) can be replaced by divisor %d (%d).   ", 
            iNode, f, Sfm_ObjFanin(p, iNode, f), iVar, Vec_IntEntry(p->vDivs, iVar) );
        Kit_DsdPrintFromTruth( (unsigned *)&uTruth, Vec_IntSize(p->vDivIds) ); printf( "\n" );
    }
    if ( iVar == -1 )
        p->nRemoves++;
    else
        p->nResubs++;
    if ( fSkipUpdate )
        return 0;
    // update the network
    Sfm_NtkUpdate( p, iNode, f, (iVar == -1 ? iVar : Vec_IntEntry(p->vDivs, iVar)), uTruth, p->pTruth );
    // the number of fanins cannot increase
    assert( nFanins >= Sfm_ObjFaninNum(p, iNode) );
    //printf( "Modifying node %d with %d fanins (resulting in %d fanins).  ", iNode, nFanins, Sfm_ObjFaninNum(p, iNode) );
    //Abc_TtPrintHexRev( stdout, p->pTruth, Sfm_ObjFaninNum(p, iNode) );
    //printf( "\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs resubstitution for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_NodeResubOne( Sfm_Ntk_t * p, int iNode )
{
    int fSkipUpdate  = 0;
    int i, iFanin;
    word uTruth;
    abctime clk;
    assert( Sfm_ObjIsNode(p, iNode) );
    p->nTryImproves++;
    // report init stats
    if ( p->pPars->fVeryVerbose )
        printf( "%5d : Lev =%3d. Leaf =%3d.  Node =%3d.  Div=%3d.  Fanins = %d.  MFFC = %d\n", 
            iNode, Sfm_ObjLevel(p, iNode), 0, Vec_IntSize(p->vNodes), Vec_IntSize(p->vDivs), 
            Sfm_ObjFaninNum(p, iNode), Sfm_ObjMffcSize(p, iNode) );
    // collect fanins
    Vec_IntClear( p->vDivIds );
    Sfm_ObjForEachFanin( p, iNode, iFanin, i )
        Vec_IntPush( p->vDivIds, Sfm_ObjSatVar(p, iFanin) );
clk = Abc_Clock();
    uTruth = Sfm_ComputeInterpolant2( p );
p->timeSat += Abc_Clock() - clk;
    // analyze outcomes
    if ( uTruth == SFM_SAT_UNDEC )
    {
        p->nTimeOuts++;
        return 0;
    }
    assert( uTruth != SFM_SAT_SAT );
    if ( uTruth == Vec_WrdEntry(p->vTruths, iNode) )
        return 0;
    else
    {
        word uTruth0  =  Vec_WrdEntry(p->vTruths, iNode);
        //word uTruth0N = ~uTruth0;
        //word uTruthN  = ~uTruth;
        int Old  = Kit_TruthLitNum((unsigned*)&uTruth0,  Sfm_ObjFaninNum(p, iNode), p->vCover);
        //int OldN = Kit_TruthLitNum((unsigned*)&uTruth0N, Sfm_ObjFaninNum(p, iNode), p->vCover);
        int New  = Kit_TruthLitNum((unsigned*)&uTruth,   Sfm_ObjFaninNum(p, iNode), p->vCover);
        //int NewN = Kit_TruthLitNum((unsigned*)&uTruthN,  Sfm_ObjFaninNum(p, iNode), p->vCover);
        //if ( Abc_MinInt(New, NewN) > Abc_MinInt(Old, OldN) )
        if ( New > Old )
            return 0;
    }
    p->nImproves++;
    if ( fSkipUpdate )
        return 0;
    // update truth table
    Vec_WrdWriteEntry( p->vTruths, iNode, uTruth );
    Sfm_TruthToCnf( uTruth, NULL, Sfm_ObjFaninNum(p, iNode), p->vCover, (Vec_Str_t *)Vec_WecEntry(p->vCnfs, iNode) );
    return 1;
}
int Sfm_NodeResub( Sfm_Ntk_t * p, int iNode )
{
    int i, iFanin;
    p->nNodesTried++;
    // prepare SAT solver
    if ( !Sfm_NtkCreateWindow( p, iNode, p->pPars->fVeryVerbose ) )
        return 0;
    if ( !Sfm_NtkWindowToSolver( p ) )
        return 0;
    // try replacing area critical fanins
    Sfm_ObjForEachFanin( p, iNode, iFanin, i )
        if ( Sfm_ObjIsNode(p, iFanin) && Sfm_ObjFanoutNum(p, iFanin) == 1 )
        {
            if ( Sfm_NodeResubSolve( p, iNode, i, 0 ) )
                return 1;
        }
    // try removing redundant edges
    if ( !p->pPars->fArea )
    Sfm_ObjForEachFanin( p, iNode, iFanin, i )
        if ( !(Sfm_ObjIsNode(p, iFanin) && Sfm_ObjFanoutNum(p, iFanin) == 1) )
        {
            if ( Sfm_NodeResubSolve( p, iNode, i, 1 ) )
                return 1;
        }
    // try simplifying local functions
    if ( p->pPars->fUseDcs && Sfm_ObjFaninNum(p, iNode) <= 6 )
        if ( Sfm_NodeResubOne( p, iNode ) )
            return 1;
/*
    // try replacing area critical fanins while adding two new fanins
    if ( Sfm_ObjFaninNum(p, iNode) < p->nFaninMax )
        Abc_ObjForEachFanin( pNode, pFanin, i )
            if ( !Abc_ObjIsCi(pFanin) && Abc_ObjFanoutNum(pFanin) == 1 )
            {
                if ( Abc_NtkMfsSolveSatResub2( p, pNode, i, -1 ) )
                    return 1;
            }
*/
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_NtkPrint( Sfm_Ntk_t * p )
{
    int i;
    for ( i = 0; i < p->nObjs; i++ )
    {
        Vec_Int_t * vArray = Vec_WecEntry( &p->vFanins, i );
        printf( "Obj %3d : ", i );
        printf( "Fixed %d  ", Vec_StrEntry(p->vFixed, i) );
        printf( "Empty %d  ", Vec_StrEntry(p->vEmpty, i) );
        printf( "Truth  " );
        Extra_PrintHex( stdout, (unsigned *)Vec_WrdEntryP(p->vTruths, i), Vec_IntSize(vArray) );
        printf( "  " );
        Vec_IntPrint( vArray );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_NtkPerform( Sfm_Ntk_t * p, Sfm_Par_t * pPars )
{
    int i, k, Counter = 0, CounterLarge = 0;
    //Sfm_NtkPrint( p );
    p->timeTotal = Abc_Clock();
    if ( pPars->fVerbose )
    {
        int nFixed = p->vFixed ? Vec_StrSum(p->vFixed) : 0;
        int nEmpty = p->vEmpty ? Vec_StrSum(p->vEmpty) : 0;
        printf( "Performing MFS with %d PIs, %d POs, %d nodes (%d flexible, %d fixed, %d empty).\n", 
            p->nPis, p->nPos, p->nNodes, p->nNodes-nFixed, nFixed, nEmpty );
    }
    p->pPars = pPars;
    Sfm_NtkPrepare( p );
//    Sfm_ComputeInterpolantCheck( p );
//    return 0;
    p->nTotalNodesBeg = Vec_WecSizeUsedLimits( &p->vFanins, Sfm_NtkPiNum(p), Vec_WecSize(&p->vFanins) - Sfm_NtkPoNum(p) );
    p->nTotalEdgesBeg = Vec_WecSizeSize(&p->vFanins) - Sfm_NtkPoNum(p);
    Sfm_NtkForEachNode( p, i )
    {
        if ( Sfm_ObjIsFixed( p, i ) )
            continue;
        if ( p->pPars->nDepthMax && Sfm_ObjLevel(p, i) > p->pPars->nDepthMax )
            continue;
        //if ( Sfm_ObjFaninNum(p, i) < 2 )
        //    continue;
        if ( Sfm_ObjFaninNum(p, i) > SFM_SUPP_MAX )
        {
            CounterLarge++;
            continue;
        }
        for ( k = 0; Sfm_NodeResub(p, i); k++ )
        {
//            Counter++;
//            break;
        }
        Counter += (k > 0);
        if ( pPars->nNodesMax && Counter >= pPars->nNodesMax )
            break;
    }
    p->nTotalNodesEnd = Vec_WecSizeUsedLimits( &p->vFanins, Sfm_NtkPiNum(p), Vec_WecSize(&p->vFanins) - Sfm_NtkPoNum(p) );
    p->nTotalEdgesEnd = Vec_WecSizeSize(&p->vFanins) - Sfm_NtkPoNum(p);
    p->timeTotal = Abc_Clock() - p->timeTotal;
    if ( pPars->fVerbose && CounterLarge )
        printf( "MFS skipped %d (out of %d) nodes with more than %d fanins.\n", CounterLarge, p->nNodes, SFM_SUPP_MAX );
    if ( pPars->fVerbose )
        Sfm_NtkPrintStats( p );
    //Sfm_NtkPrint( p );
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

