/**CFile****************************************************************

  FileName    [ifSeq.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Sequential mapping.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifSeq.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern int s_MappingTime;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Prepares for sequential mapping by linking the latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManPrepareMappingSeq( If_Man_t * p )
{
    If_Obj_t * pObjLi, * pObjLo;
    int i;

    // link the latch outputs (CIs) directly to the drivers of latch inputs (COs)
    for ( i = 0; i < p->pPars->nLatches; i++ )
    {
        pObjLi = If_ManLi( p, i );
        pObjLo = If_ManLo( p, i );
        pObjLo->pFanin0 = If_ObjFanin0( pObjLi );
        pObjLo->fCompl0 = If_ObjFaninC0( pObjLi );
    }
}

/**Function*************************************************************

  Synopsis    [Collects latches in the topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []
 
***********************************************************************/
void If_ManCollectLatches_rec( If_Obj_t * pObj, Vec_Ptr_t * vLatches )
{
    if ( !If_ObjIsLatch(pObj) )
        return;
    if ( pObj->fMark )
        return;
    pObj->fMark = 1;
    If_ManCollectLatches_rec( pObj->pFanin0, vLatches );
    Vec_PtrPush( vLatches, pObj );
}

/**Function*************************************************************

  Synopsis    [Collects latches in the topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * If_ManCollectLatches( If_Man_t * p )
{
    Vec_Ptr_t * vLatches;
    If_Obj_t * pObj;
    int i;
    // collect latches 
    vLatches = Vec_PtrAlloc( p->pPars->nLatches );
    If_ManForEachLatchOutput( p, pObj, i )
        If_ManCollectLatches_rec( pObj, vLatches );
    // clean marks
    Vec_PtrForEachEntry( vLatches, pObj, i )
        pObj->fMark = 0;
    assert( Vec_PtrSize(vLatches) == p->pPars->nLatches );
    return vLatches;
}

/**Function*************************************************************

  Synopsis    [Performs one pass of l-value computation over all nodes.]

  Description [Experimentally it was found that checking POs changes
  is not enough to detect the convergence of l-values in the network.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManPerformMappingRoundSeq( If_Man_t * p, int nIter )
{
    If_Obj_t * pObj;
    int i, clk = clock();
    int fVeryVerbose = 0;
    int fChange = 0;

    // map the internal nodes
    p->nCutsMerged = 0;
    If_ManForEachNode( p, pObj, i )
    {
        If_ObjPerformMappingAnd( p, pObj, 0, 0 );
        if ( pObj->fRepr )
            If_ObjPerformMappingChoice( p, pObj, 0, 0 );
    }

    // postprocess the mapping
//printf( "Itereation %d: \n", nIter );
    If_ManForEachNode( p, pObj, i )
    {
        // update the LValues stored separately
        if ( If_ObjLValue(pObj) < If_ObjCutBest(pObj)->Delay - p->fEpsilon )
        {
            If_ObjSetLValue( pObj, If_ObjCutBest(pObj)->Delay );
            fChange = 1;
        }
//printf( "%d ", (int)If_ObjLValue(pObj) );
        // reset the visit counters
        assert( pObj->nVisits == 0 );
        pObj->nVisits = pObj->nVisitsCopy;
    }
//printf( "\n" );

    // propagate LValues over the registers
    Vec_PtrForEachEntry( p->vLatchOrder, pObj, i )
    {
        If_ObjSetLValue( pObj, If_ObjLValue(If_ObjFanin0(pObj)) - p->Period );
        If_ObjSetArrTime( pObj, If_ObjLValue(pObj) );
    }

    // compute area and delay
    if ( fVeryVerbose )
    {
        p->RequiredGlo = If_ManDelayMax( p, 1 );
        p->AreaGlo = If_ManScanMapping(p);
        printf( "S%d:  Fi = %6.2f. Del = %6.2f. Area = %8.2f. Cuts = %8d. ", 
             nIter, (float)p->Period, p->RequiredGlo, p->AreaGlo, p->nCutsMerged );
        PRT( "T", clock() - clk );
    }
    return fChange;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if retiming with this clock period is feasible.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManBinarySearchPeriod( If_Man_t * p )
{
    If_Obj_t * pObj;
    int i, c, fConverged;
    int fResetRefs = 0;

    p->nAttempts++;

    // reset initial LValues (PIs to 0; others to -inf)
    If_ManForEachObj( p, pObj, i )
    {
        if ( If_ObjIsPi(pObj) || If_ObjIsConst1(pObj) )
        {
            If_ObjSetLValue( pObj, (float)0.0 );
            If_ObjSetArrTime( pObj, (float)0.0 );
        }
        else
        {
            If_ObjSetLValue( pObj, (float)-IF_INFINITY );
            If_ObjSetArrTime( pObj, (float)-IF_INFINITY );
        }
        // undo any previous mapping, except for CIs
        if ( If_ObjIsAnd(pObj) )
            If_ObjCutBest(pObj)->nLeaves = 0;
    }

    // update all values iteratively
    fConverged = 0;
    for ( c = 1; c <= p->nMaxIters; c++ )
    {
        if ( !If_ManPerformMappingRoundSeq( p, c ) )
        {
            p->RequiredGlo = If_ManDelayMax( p, 1 );
            fConverged = 1;
            break;
        }
        p->RequiredGlo = If_ManDelayMax( p, 1 );
//printf( "Global = %d \n", (int)p->RequiredGlo );
        if ( p->RequiredGlo > p->Period + p->fEpsilon )
            break; 
    }

    // report the results
    if ( p->pPars->fVerbose )
    {
        p->AreaGlo = If_ManScanMapping(p);
        printf( "Attempt = %2d.  Iters = %3d.  Area = %10.2f.  Fi = %6.2f.  ", p->nAttempts, c, p->AreaGlo, (float)p->Period );
        if ( fConverged )
            printf( "  Feasible" );
        else if ( c > p->nMaxIters )
            printf( "Infeasible (timeout)" );
        else
            printf( "Infeasible" );
        printf( "\n" );
    }
    return fConverged;
}


/**Function*************************************************************

  Synopsis    [Performs binary search for the optimal clock period.]

  Description [Assumes that FiMin is infeasible while FiMax is feasible.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManBinarySearch_rec( If_Man_t * p, int FiMin, int FiMax )
{
    assert( FiMin < FiMax );
    if ( FiMin + 1 == FiMax )
        return FiMax;
    // compute the median
    p->Period = FiMin + (FiMax - FiMin)/2;
    if ( If_ManBinarySearchPeriod( p ) )
        return If_ManBinarySearch_rec( p, FiMin, p->Period ); // Median is feasible
    else 
        return If_ManBinarySearch_rec( p, p->Period, FiMax ); // Median is infeasible
}

/**Function*************************************************************

  Synopsis    [Performs sequential mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_ManPerformMappingSeqPost( If_Man_t * p )
{
    If_Obj_t * pObjLi, * pObjLo, * pObj;
    int i;

    // link the latch outputs (CIs) directly to the drivers of latch inputs (COs)
    for ( i = 0; i < p->pPars->nLatches; i++ )
    {
        pObjLi = If_ManLi( p, i );
        pObjLo = If_ManLo( p, i );
//        printf( "%3d : %2d -> %2d   \n", i, 
//            (int)If_ObjLValue(If_ObjFanin0(pObjLo)), (int)If_ObjLValue(pObjLo) );
    }

    // set arrival times
    assert( p->pPars->pTimesArr != NULL );
    If_ManForEachLatchOutput( p, pObjLo, i )
        p->pPars->pTimesArr[i] = If_ObjLValue(pObjLo);

    // set the required times
    assert( p->pPars->pTimesReq == NULL );
    p->pPars->pTimesReq = ALLOC( float, If_ManCoNum(p) );
    If_ManForEachPo( p, pObj, i )
    {
        p->pPars->pTimesReq[i] = p->RequiredGlo2;
//        printf( "Out %3d : %2d   \n", i, (int)p->pPars->pTimesReq[i] );
    }
    If_ManForEachLatchInput( p, pObjLi, i )
    {
        p->pPars->pTimesReq[i] = If_ObjLValue(If_ObjFanin0(pObjLi));
//        printf( "Out %3d : %2d   \n", i, (int)p->pPars->pTimesReq[i] );
    }

    // undo previous mapping
    If_ManForEachObj( p, pObj, i )
        if ( If_ObjIsAnd(pObj) )
            If_ObjCutBest(pObj)->nLeaves = 0;

    // map again combinationally
    p->pPars->fSeqMap = 0;
    If_ManPerformMappingComb( p );
    p->pPars->fSeqMap = 1;
}

/**Function*************************************************************

  Synopsis    [Performs sequential mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_ManPerformMappingSeq( If_Man_t * p )
{
    int clkTotal = clock();
    int PeriodBest;

    p->SortMode = 0;

    // perform combinational mapping to get the upper bound on the clock period
    If_ManPerformMappingRound( p, 1, 0, 0, NULL );
    p->RequiredGlo = If_ManDelayMax( p, 0 );
    p->RequiredGlo2 = p->RequiredGlo;

    // set direct linking of latches with their inputs
    If_ManPrepareMappingSeq( p );

    // collect latches
    p->vLatchOrder = If_ManCollectLatches( p );

    // set parameters
    p->nCutsUsed = p->pPars->nCutsMax;
    p->nAttempts = 0;
    p->nMaxIters = 50;
    p->Period    = (int)p->RequiredGlo;

    // make sure the clock period works
    if ( !If_ManBinarySearchPeriod( p ) )
    {
        printf( "If_ManPerformMappingSeq(): The upper bound on the clock period cannot be computed.\n" );
        return 0;
    }

    // perform binary search
    PeriodBest = If_ManBinarySearch_rec( p, 0, p->Period );

    // recompute the best l-values
    if ( p->Period != PeriodBest )
    {
        p->Period = PeriodBest;
        if ( !If_ManBinarySearchPeriod( p ) )
        {
            printf( "If_ManPerformMappingSeq(): The final clock period cannot be confirmed.\n" );
            return 0;
        }
    }
    if ( p->pPars->fVerbose )
    {
/*
        {
            FILE * pTable;
            pTable = fopen( "iscas/stats_new.txt", "a+" );
//            fprintf( pTable, "%s ",  pNtk->pName );
            fprintf( pTable, "%d ", p->Period );
    //        fprintf( pTable, "%.2f ", (float)(s_MappingMem)/(float)(1<<20) );
//            fprintf( pTable, "%.2f", (float)(s_MappingTime)/(float)(CLOCKS_PER_SEC) );
//            fprintf( pTable, "\n" );
            fclose( pTable );
        }
*/
        printf( "The best clock period is %3d.  ", p->Period );
        PRT( "Sequential time", clock() - clkTotal );
    }
    p->RequiredGlo = (float)PeriodBest;

    // postprocess it using combinational mapping
    If_ManPerformMappingSeqPost( p );
    s_MappingTime = clock() - clkTotal;
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


