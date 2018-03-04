/**CFile****************************************************************

  FileName    [sclDnsize.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Standard-cell library representation.]

  Synopsis    [Selective decrease of gate sizes.]

  Author      [Alan Mishchenko, Niklas Een]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 24, 2012.]

  Revision    [$Id: sclDnsize.c,v 1.0 2012/08/24 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sclSize.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Find the array of nodes to be updated.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclFindWindow( Abc_Obj_t * pPivot, Vec_Int_t ** pvNodes, Vec_Int_t ** pvEvals )
{
    Abc_Ntk_t * p = Abc_ObjNtk(pPivot);
    Abc_Obj_t * pObj, * pNext, * pNext2;
    Vec_Int_t * vNodes = *pvNodes;
    Vec_Int_t * vEvals = *pvEvals;
    int i, k;
    assert( Abc_ObjIsNode(pPivot) );
    // collect fanins, node, and fanouts
    Vec_IntClear( vNodes );
    Abc_ObjForEachFanin( pPivot, pNext, i )
//        if ( Abc_ObjIsNode(pNext) && Abc_ObjFaninNum(pNext) > 0 )
        if ( Abc_ObjIsCi(pNext) || Abc_ObjFaninNum(pNext) > 0 )
            Vec_IntPush( vNodes, Abc_ObjId(pNext) );
    Vec_IntPush( vNodes, Abc_ObjId(pPivot) );
    Abc_ObjForEachFanout( pPivot, pNext, i )
        if ( Abc_ObjIsNode(pNext) )
        {
            Vec_IntPush( vNodes, Abc_ObjId(pNext) );
            Abc_ObjForEachFanout( pNext, pNext2, k )
                if ( Abc_ObjIsNode(pNext2) )
                    Vec_IntPush( vNodes, Abc_ObjId(pNext2) );
        }
    Vec_IntUniqify( vNodes );
    // label nodes
    Abc_NtkForEachObjVec( vNodes, p, pObj, i )
    {
        assert( pObj->fMarkB == 0 );
        pObj->fMarkB = 1;
    }
    // collect nodes visible from the critical paths
    Vec_IntClear( vEvals );
    Abc_NtkForEachObjVec( vNodes, p, pObj, i )
        Abc_ObjForEachFanout( pObj, pNext, k )
            if ( !pNext->fMarkB )
            {
                assert( pObj->fMarkB );
                Vec_IntPush( vEvals, Abc_ObjId(pObj) );
                break;
            }
    assert( Vec_IntSize(vEvals) > 0 );
    // label nodes
    Abc_NtkForEachObjVec( vNodes, p, pObj, i )
        pObj->fMarkB = 0;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the node can be improved.]

  Description [Updated the node to have a new gate.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_SclCheckImprovement( SC_Man * p, Abc_Obj_t * pObj, Vec_Int_t * vNodes, Vec_Int_t * vEvals, int Notches, int DelayGap )
{
    Abc_Obj_t * pTemp;
    SC_Cell * pCellOld, * pCellNew;
    float dGain, dGainBest;
    int i, k, gateBest;
    abctime clk;
clk = Abc_Clock();
//    printf( "%d -> %d\n", Vec_IntSize(vNodes), Vec_IntSize(vEvals) );
    // save old gate, timing, fanin load
    pCellOld = Abc_SclObjCell( pObj );
    Abc_SclConeStore( p, vNodes );
    Abc_SclEvalStore( p, vEvals );
    Abc_SclLoadStore( p, pObj );
    // try different gate sizes for this node
    gateBest = -1;
    dGainBest = -DelayGap;
    SC_RingForEachCellRev( pCellOld, pCellNew, i )
    {
        if ( pCellNew->area >= pCellOld->area )
            continue;
        if ( i > Notches )
            break;
        // set new cell
        Abc_SclObjSetCell( pObj, pCellNew );
        Abc_SclUpdateLoad( p, pObj, pCellOld, pCellNew );
        // recompute timing
        Abc_SclTimeCone( p, vNodes );
        // set old cell
        Abc_SclObjSetCell( pObj, pCellOld );
        Abc_SclLoadRestore( p, pObj );
        // evaluate gain
        dGain = Abc_SclEvalPerformLegal( p, vEvals, p->MaxDelay0 );
        if ( dGain == -1 )
            continue;
        // save best gain
        if ( dGainBest < dGain )
        {
            dGainBest = dGain;
            gateBest = pCellNew->Id;
        }
    } 
    // put back old cell and timing
    Abc_SclObjSetCell( pObj, pCellOld );
    Abc_SclConeRestore( p, vNodes );
p->timeSize += Abc_Clock() - clk;
    if ( gateBest >= 0 )
    {
        pCellNew = SC_LibCell( p->pLib, gateBest );
        Abc_SclObjSetCell( pObj, pCellNew );
        p->SumArea += pCellNew->area - pCellOld->area;
//        printf( "%f   %f -> %f\n", pCellNew->area - pCellOld->area, p->SumArea - (pCellNew->area - pCellOld->area), p->SumArea );
//        printf( "%6d  %20s -> %20s  %f -> %f\n", Abc_ObjId(pObj), pCellOld->pName, pCellNew->pName, pCellOld->area, pCellNew->area );
        // mark used nodes with the current trav ID
        Abc_NtkForEachObjVec( vNodes, p->pNtk, pTemp, k )
            Abc_NodeSetTravIdCurrent( pTemp );
        // update load and timing...
        Abc_SclUpdateLoad( p, pObj, pCellOld, pCellNew );
        Abc_SclTimeIncInsert( p, pObj );
        return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Collect nodes by area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCollectNodesByArea( SC_Man * p, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    assert( Vec_QueSize(p->vNodeByGain) == 0 );
    Vec_QueClear( p->vNodeByGain );
    Abc_NtkForEachNode( pNtk, pObj, i )
    if ( Abc_ObjFaninNum(pObj) > 0 )
    {
        Vec_FltWriteEntry( p->vNode2Gain, Abc_ObjId(pObj), Abc_SclObjCell(pObj)->area );
        Vec_QuePush( p->vNodeByGain, Abc_ObjId(pObj) );
    }
}
int Abc_SclCheckOverlap( Abc_Ntk_t * pNtk, Vec_Int_t * vNodes )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObjVec( vNodes, pNtk, pObj, i )
        if ( Abc_NodeIsTravIdCurrent(pObj) )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Print cumulative statistics.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclDnsizePrint( SC_Man * p, int Iter, int nAttempts, int nOverlaps, int nChanges, int fVerbose )
{
    if ( Iter == -1 )
        printf( "Total : " );
    else
        printf( "%5d : ",    Iter );
    printf( "Try =%6d  ",    nAttempts );
    printf( "Over =%6d  ",   nOverlaps );
    printf( "Fail =%6d  ",   nAttempts-nOverlaps-nChanges );
    printf( "Win =%6d  ",    nChanges );
    printf( "A: " );
    printf( "%.2f ",         p->SumArea );
    printf( "(%+5.1f %%)  ", 100.0 * (p->SumArea - p->SumArea0)/ p->SumArea0 );
    printf( "D: " );
    printf( "%.2f ps ",      p->MaxDelay );
    printf( "(%+5.1f %%)  ", 100.0 * (p->MaxDelay - p->MaxDelay0)/ p->MaxDelay0 );
    printf( "%8.2f sec    ", 1.0*(Abc_Clock() - p->timeTotal)/(CLOCKS_PER_SEC) );
    printf( "%c", fVerbose ? '\n' : '\r' );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclDnsizePerformInt( SC_Lib * pLib, Abc_Ntk_t * pNtk, SC_SizePars * pPars )
{
    SC_Man * p;
    Abc_Obj_t * pObj;
    Vec_Int_t * vNodes, * vEvals, * vTryLater; 
    abctime clk, nRuntimeLimit = pPars->TimeOut ? pPars->TimeOut * CLOCKS_PER_SEC + Abc_Clock() : 0;
    int i, k;

    if ( pPars->fVerbose )
    {
        printf( "Parameters: " );
        printf( "Iters =%5d.  ",          pPars->nIters    );
        printf( "UseDept =%2d. ",         pPars->fUseDept  );
        printf( "UseWL =%2d. ",           pPars->fUseWireLoads );
        printf( "Target =%5d ps. ",       pPars->DelayUser );
        printf( "DelayGap =%3d ps. ",     pPars->DelayGap );
        printf( "Timeout =%4d sec",       pPars->TimeOut   );
        printf( "\n" );
    }

    // prepare the manager; collect init stats
    p = Abc_SclManStart( pLib, pNtk, pPars->fUseWireLoads, pPars->fUseDept, pPars->DelayUser, pPars->BuffTreeEst );
    p->timeTotal  = Abc_Clock();
    assert( p->vGatesBest == NULL );
    p->vGatesBest = Vec_IntDup( p->pNtk->vGates );

    // perform upsizing
    vNodes = Vec_IntAlloc( 1000 );
    vEvals = Vec_IntAlloc( 1000 );
    vTryLater = Vec_IntAlloc( 1000 );
    for ( i = 0; i < pPars->nIters; i++ )
    {
        int nRounds = 0;
        int nAttemptAll = 0, nOverlapAll = 0, nChangesAll = 0;
        Abc_NtkCollectNodesByArea( p, pNtk );
        while ( Vec_QueSize(p->vNodeByGain) > 0 )
        {
            int nAttempt = 0, nOverlap = 0, nChanges = 0;
            Vec_IntClear( vTryLater );
            Abc_NtkIncrementTravId( pNtk );
            while ( Vec_QueSize(p->vNodeByGain) > 0 )
            {
                clk = Abc_Clock();
                pObj = Abc_NtkObj( p->pNtk, Vec_QuePop(p->vNodeByGain) );
                Abc_SclFindWindow( pObj, &vNodes, &vEvals );
                p->timeCone += Abc_Clock() - clk;
                if ( Abc_SclCheckOverlap( p->pNtk, vNodes ) )
                    nOverlap++, Vec_IntPush( vTryLater, Abc_ObjId(pObj) );
                else 
                    nChanges += Abc_SclCheckImprovement( p, pObj, vNodes, vEvals, pPars->Notches, pPars->DelayGap );
                nAttempt++;
            }
            Abc_NtkForEachObjVec( vTryLater, pNtk, pObj, k )
                Vec_QuePush( p->vNodeByGain, Abc_ObjId(pObj) );

            clk = Abc_Clock();
            if ( Vec_IntSize(p->vChanged) )
                Abc_SclTimeIncUpdate( p );
            else
                Abc_SclTimeNtkRecompute( p, &p->SumArea, &p->MaxDelay, pPars->fUseDept, pPars->DelayUser );
            p->timeTime += Abc_Clock() - clk;

            p->MaxDelay = Abc_SclReadMaxDelay( p );
            if ( pPars->fUseDept && pPars->DelayUser > 0 && p->MaxDelay < pPars->DelayUser )
                p->MaxDelay = pPars->DelayUser;
            Abc_SclDnsizePrint( p, nRounds++, nAttempt, nOverlap, nChanges, pPars->fVeryVerbose ); 
            nAttemptAll += nAttempt; nOverlapAll += nOverlap; nChangesAll += nChanges;
            if ( nRuntimeLimit && Abc_Clock() > nRuntimeLimit )
                break;
        }
        // recompute
//        Abc_SclTimeNtkRecompute( p, &p->SumArea, &p->MaxDelay, pPars->fUseDept, pPars->DelayUser );
        if ( pPars->fVerbose )
            Abc_SclDnsizePrint( p, -1, nAttemptAll, nOverlapAll, nChangesAll, 1 ); 
        if ( nRuntimeLimit && Abc_Clock() > nRuntimeLimit )
            break;
        if ( nAttemptAll == 0 )
            break;
    }
    Vec_IntFree( vNodes );
    Vec_IntFree( vEvals );
    Vec_IntFree( vTryLater );
    if ( !pPars->fVerbose )
        printf( "                                                                                                                                                  \r" );

    // report runtime
    p->timeTotal = Abc_Clock() - p->timeTotal;
    if ( pPars->fVerbose )
    {
        p->timeOther = p->timeTotal - p->timeCone - p->timeSize - p->timeTime;
        ABC_PRTP( "Runtime: Critical path", p->timeCone,  p->timeTotal );
        ABC_PRTP( "Runtime: Sizing eval  ", p->timeSize,  p->timeTotal );
        ABC_PRTP( "Runtime: Timing update", p->timeTime,  p->timeTotal );
        ABC_PRTP( "Runtime: Other        ", p->timeOther, p->timeTotal );
        ABC_PRTP( "Runtime: TOTAL        ", p->timeTotal, p->timeTotal );
    }
    if ( pPars->fDumpStats )
        Abc_SclDumpStats( p, "stats2.txt", p->timeTotal );
    if ( nRuntimeLimit && Abc_Clock() > nRuntimeLimit )
        printf( "Gate sizing timed out at %d seconds.\n", pPars->TimeOut );

    // save the result and quit
    Abc_SclSclGates2MioGates( pLib, pNtk ); // updates gate pointers
    Abc_SclManFree( p );
//    Abc_NtkCleanMarkAB( pNtk );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_SclDnsizePerform( SC_Lib * pLib, Abc_Ntk_t * pNtk, SC_SizePars * pPars )
{
    Abc_Ntk_t * pNtkNew = pNtk;
    if ( pNtk->nBarBufs2 > 0 )
        pNtkNew = Abc_NtkDupDfsNoBarBufs( pNtk );
    Abc_SclDnsizePerformInt( pLib, pNtkNew, pPars );
    if ( pNtk->nBarBufs2 > 0 )
        Abc_SclTransferGates( pNtk, pNtkNew );
    if ( pNtk->nBarBufs2 > 0 )
        Abc_NtkDelete( pNtkNew );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

