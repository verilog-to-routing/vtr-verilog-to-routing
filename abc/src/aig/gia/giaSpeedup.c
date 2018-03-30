/**CFile****************************************************************

  FileName    [gia.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: gia.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "map/if/if.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sorts the pins in the decreasing order of delays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_LutDelayTraceSortPins( Gia_Man_t * p, int iObj, int * pPinPerm, float * pPinDelays )
{
    int iFanin, i, j, best_i, temp;
    assert( Gia_ObjIsLut(p, iObj) );
    // start the trivial permutation and collect pin delays
    Gia_LutForEachFanin( p, iObj, iFanin, i )
    {
        pPinPerm[i] = i;
        pPinDelays[i] = Gia_ObjTimeArrival(p, iFanin);
    }
    // selection sort the pins in the decreasible order of delays
    // this order will match the increasing order of LUT input pins
    for ( i = 0; i < Gia_ObjLutSize(p, iObj)-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < Gia_ObjLutSize(p, iObj); j++ )
            if ( pPinDelays[pPinPerm[j]] > pPinDelays[pPinPerm[best_i]] )
                best_i = j;
        if ( best_i == i )
            continue;
        temp = pPinPerm[i]; 
        pPinPerm[i] = pPinPerm[best_i]; 
        pPinPerm[best_i] = temp;
    }
    // verify
    assert( Gia_ObjLutSize(p, iObj) == 0 || pPinPerm[0] < Gia_ObjLutSize(p, iObj) );
    for ( i = 1; i < Gia_ObjLutSize(p, iObj); i++ )
    {
        assert( pPinPerm[i] < Gia_ObjLutSize(p, iObj) );
        assert( pPinDelays[pPinPerm[i-1]] >= pPinDelays[pPinPerm[i]] );
    }
}

/**Function*************************************************************

  Synopsis    [Sorts the pins in the decreasing order of delays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_LutWhereIsPin( Gia_Man_t * p, int iFanout, int iFanin, int * pPinPerm )
{
    int i;
    for ( i = 0; i < Gia_ObjLutSize(p, iFanout); i++ )
        if ( Gia_ObjLutFanin(p, iFanout, pPinPerm[i]) == iFanin )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Computes the arrival times for the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Gia_ObjComputeArrival( Gia_Man_t * p, int iObj, int fUseSorting )
{
    If_LibLut_t * pLutLib = (If_LibLut_t *)p->pLutLib;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    int k, iFanin, pPinPerm[32];
    float pPinDelays[32];
    float tArrival, * pDelays;
    if ( Gia_ObjIsCi(pObj) )
        return Gia_ObjTimeArrival(p, iObj);
    if ( Gia_ObjIsCo(pObj) )
        return Gia_ObjTimeArrival(p, Gia_ObjFaninId0p(p, pObj) );
    assert( Gia_ObjIsLut(p, iObj) );
    tArrival = -TIM_ETERNITY;
    if ( pLutLib == NULL )
    {
        Gia_LutForEachFanin( p, iObj, iFanin, k )
            if ( tArrival < Gia_ObjTimeArrival(p, iFanin) + 1.0 )
                tArrival = Gia_ObjTimeArrival(p, iFanin) + 1.0;
    }
    else if ( !pLutLib->fVarPinDelays )
    {
        pDelays = pLutLib->pLutDelays[Gia_ObjLutSize(p, iObj)];
        Gia_LutForEachFanin( p, iObj, iFanin, k )
            if ( tArrival < Gia_ObjTimeArrival(p, iFanin) + pDelays[0] )
                tArrival = Gia_ObjTimeArrival(p, iFanin) + pDelays[0];
    }
    else
    {
        pDelays = pLutLib->pLutDelays[Gia_ObjLutSize(p, iObj)];
        if ( fUseSorting )
        {
            Gia_LutDelayTraceSortPins( p, iObj, pPinPerm, pPinDelays );
            Gia_LutForEachFanin( p, iObj, iFanin, k ) 
                if ( tArrival < Gia_ObjTimeArrival( p, Gia_ObjLutFanin(p,iObj,pPinPerm[k])) + pDelays[k] )
                    tArrival = Gia_ObjTimeArrival( p, Gia_ObjLutFanin(p,iObj,pPinPerm[k])) + pDelays[k];
        }
        else
        {
            Gia_LutForEachFanin( p, iObj, iFanin, k )
                if ( tArrival < Gia_ObjTimeArrival(p, iFanin) + pDelays[k] )
                    tArrival = Gia_ObjTimeArrival(p, iFanin) + pDelays[k];
        }
    }
    if ( Gia_ObjLutSize(p, iObj) == 0 )
        tArrival = 0.0;
    return tArrival;
}


/**Function*************************************************************

  Synopsis    [Propagates the required times through the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Gia_ObjPropagateRequired( Gia_Man_t * p, int iObj, int fUseSorting )
{
    If_LibLut_t * pLutLib = (If_LibLut_t *)p->pLutLib;
    int k, iFanin, pPinPerm[32];
    float pPinDelays[32];
    float tRequired = 0.0; // Suppress "might be used uninitialized"
    float * pDelays;
    assert( Gia_ObjIsLut(p, iObj) );
    if ( pLutLib == NULL )
    {
        tRequired = Gia_ObjTimeRequired( p, iObj) - (float)1.0;
        Gia_LutForEachFanin( p, iObj, iFanin, k )
            if ( Gia_ObjTimeRequired(p, iFanin) > tRequired )
                Gia_ObjSetTimeRequired( p, iFanin, tRequired );
    }
    else if ( !pLutLib->fVarPinDelays )
    {
        pDelays = pLutLib->pLutDelays[Gia_ObjLutSize(p, iObj)];
        tRequired = Gia_ObjTimeRequired(p, iObj) - pDelays[0];
        Gia_LutForEachFanin( p, iObj, iFanin, k )
            if ( Gia_ObjTimeRequired(p, iFanin) > tRequired )
                Gia_ObjSetTimeRequired( p, iFanin, tRequired );
    }
    else 
    {
        pDelays = pLutLib->pLutDelays[Gia_ObjLutSize(p, iObj)];
        if ( fUseSorting )
        {
            Gia_LutDelayTraceSortPins( p, iObj, pPinPerm, pPinDelays );
            Gia_LutForEachFanin( p, iObj, iFanin, k )
            {
                tRequired = Gia_ObjTimeRequired( p, iObj) - pDelays[k];
                if ( Gia_ObjTimeRequired( p, Gia_ObjLutFanin(p, iObj,pPinPerm[k])) > tRequired )
                    Gia_ObjSetTimeRequired( p,  Gia_ObjLutFanin(p, iObj,pPinPerm[k]), tRequired );
            }
        }
        else
        {
            Gia_LutForEachFanin( p, iObj, iFanin, k )
            {
                tRequired = Gia_ObjTimeRequired(p, iObj) - pDelays[k];
                if ( Gia_ObjTimeRequired(p, iFanin) > tRequired )
                    Gia_ObjSetTimeRequired( p,  iFanin, tRequired );
            }
        }
    }
    return tRequired;
}

/**Function*************************************************************

  Synopsis    [Computes the delay trace of the given network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Gia_ManDelayTraceLut( Gia_Man_t * p )
{
    int fUseSorting = 1;
    If_LibLut_t * pLutLib = (If_LibLut_t *)p->pLutLib;
    Vec_Int_t * vObjs;
    Gia_Obj_t * pObj;
    float tArrival, tArrivalCur, tRequired, tSlack;
    int i, iObj;

    // get the library
    if ( pLutLib && pLutLib->LutMax < Gia_ManLutSizeMax(p) )
    {
        printf( "The max LUT size (%d) is less than the max fanin count (%d).\n", 
            pLutLib->LutMax, Gia_ManLutSizeMax(p) );
        return -TIM_ETERNITY;
    }

    // initialize the arrival times
    Gia_ManTimeStart( p );

    // propagate arrival times
    if ( p->pManTime )
        Tim_ManIncrementTravId( (Tim_Man_t *)p->pManTime );
    Gia_ManForEachObj( p, pObj, i )
    {
        if ( !Gia_ObjIsCi(pObj) && !Gia_ObjIsCo(pObj) && !Gia_ObjIsLut(p, i) )
            continue;
        tArrival = Gia_ObjComputeArrival( p, i, fUseSorting );
        if ( Gia_ObjIsCi(pObj) && p->pManTime )
        {
            tArrival = Tim_ManGetCiArrival( (Tim_Man_t *)p->pManTime, Gia_ObjCioId(pObj) );
//printf( "%.3f  ", tArrival );
        }
        if ( Gia_ObjIsCo(pObj) && p->pManTime )
            Tim_ManSetCoArrival( (Tim_Man_t *)p->pManTime, Gia_ObjCioId(pObj), tArrival );
        Gia_ObjSetTimeArrival( p, i, tArrival );
    }

    // get the latest arrival times
    tArrival = -TIM_ETERNITY;
    Gia_ManForEachCo( p, pObj, i )
    {
        tArrivalCur = Gia_ObjTimeArrivalObj( p, Gia_ObjFanin0(pObj) );
        Gia_ObjSetTimeArrival( p, Gia_ObjId(p,pObj), tArrivalCur );
        if ( tArrival < tArrivalCur )
            tArrival = tArrivalCur;
    }

    // initialize the required times
    if ( p->pManTime )
    {
        Tim_ManIncrementTravId( (Tim_Man_t *)p->pManTime );
        Tim_ManInitPoRequiredAll( (Tim_Man_t *)p->pManTime, tArrival );
    }
    else
    {
        Gia_ManForEachCo( p, pObj, i )
            Gia_ObjSetTimeRequiredObj( p, pObj, tArrival );
    }

    // propagate the required times
    vObjs = Gia_ManOrderReverse( p );
    Vec_IntForEachEntry( vObjs, iObj, i )
    {
        pObj = Gia_ManObj(p, iObj);
        if ( Gia_ObjIsLut(p, iObj) )
        {
            Gia_ObjPropagateRequired( p, iObj, fUseSorting );
        }
        else if ( Gia_ObjIsCi(pObj) )
        {
            if ( p->pManTime )
                Tim_ManSetCiRequired( (Tim_Man_t *)p->pManTime, Gia_ObjCioId(pObj), Gia_ObjTimeRequired(p, iObj) );
        }
        else if ( Gia_ObjIsCo(pObj) )
        {
            if ( p->pManTime )
            {
                tRequired = Tim_ManGetCoRequired( (Tim_Man_t *)p->pManTime, Gia_ObjCioId(pObj) );
                Gia_ObjSetTimeRequired( p, iObj, tRequired );
            }
            if ( Gia_ObjTimeRequired(p, Gia_ObjFaninId0p(p, pObj)) > Gia_ObjTimeRequired(p, iObj) )
                Gia_ObjSetTimeRequired(p, Gia_ObjFaninId0p(p, pObj), Gia_ObjTimeRequired(p, iObj) );
        }

        // set slack for this object
        tSlack = Gia_ObjTimeRequired(p, iObj) - Gia_ObjTimeArrival(p, iObj);
        assert( tSlack + 0.01 > 0.0 );
        Gia_ObjSetTimeSlack( p, iObj, tSlack < 0.0 ? 0.0 : tSlack );
    }
    Vec_IntFree( vObjs );
    return tArrival;
}

#if 0

/**Function*************************************************************

  Synopsis    [Computes the required times for the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Gia_ObjComputeRequired( Gia_Man_t * p, int iObj, int fUseSorting )
{
    If_LibLut_t * pLutLib = p->pLutLib;
    int pPinPerm[32];
    float pPinDelays[32];
    Gia_Obj_t * pFanout;
    float tRequired, tDelay, * pDelays;
    int k, iFanin;
    if ( Gia_ObjIsCo(iObj) )
        return Gia_ObjTimeRequired( p, iObj);
    tRequired = TIM_ETERNITY;
    if ( pLutLib == NULL )
    {
        Gia_ObjForEachFanout( iObj, pFanout, k )
        {
            tDelay = Gia_ObjIsCo(pFanout)? 0.0 : 1.0;
            if ( tRequired > Gia_ObjTimeRequired( p, pFanout) - tDelay )
                tRequired = Gia_ObjTimeRequired( p, pFanout) - tDelay;
        }
    }
    else if ( !pLutLib->fVarPinDelays )
    {
        Gia_ObjForEachFanout( iObj, pFanout, k )
        {
            pDelays = pLutLib->pLutDelays[Gia_ObjLutSize(p, pFanout)];
            tDelay = Gia_ObjIsCo(pFanout)? 0.0 : pDelays[0];
            if ( tRequired > Gia_ObjTimeRequired( p, pFanout) - tDelay )
                tRequired = Gia_ObjTimeRequired( p, pFanout) - tDelay;
        }
    }
    else
    {
        if ( fUseSorting )
        {
            Gia_ObjForEachFanout( iObj, pFanout, k ) 
            {
                pDelays = pLutLib->pLutDelays[Gia_ObjLutSize(p, pFanout)];
                Gia_LutDelayTraceSortPins( p, pFanout, pPinPerm, pPinDelays );
                iFanin = Gia_LutWhereIsPin( p, pFanout, iObj, pPinPerm );
                assert( Gia_ObjLutFanin( p, pFanout, pPinPerm[iFanin]) == iObj );
                tDelay = Gia_ObjIsCo(pFanout)? 0.0 : pDelays[iFanin];
                if ( tRequired > Gia_ObjTimeRequired( p, pFanout) - tDelay )
                    tRequired = Gia_ObjTimeRequired( p, pFanout) - tDelay;
            }
        }
        else
        {
            Gia_ObjForEachFanout( iObj, pFanout, k )
            {
                pDelays = pLutLib->pLutDelays[Gia_ObjLutSize(p, pFanout)];
                iFanin = Gia_ObjFindFanin( p, pFanout, iObj );
                assert( Gia_ObjLutFanin(p, pFanout, iFanin) == iObj );
                tDelay = Gia_ObjIsCo(pFanout)? 0.0 : pDelays[iFanin];
                if ( tRequired > Gia_ObjTimeRequired( p, pFanout) - tDelay )
                    tRequired = Gia_ObjTimeRequired( p, pFanout) - tDelay;
            }
        }
    }
    return tRequired;
}

/**Function*************************************************************

  Synopsis    [Computes the arrival times for the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_LutVerifyTiming(  Gia_Man_t * p )
{
    int iObj;
    float tArrival, tRequired;
    int i;
    Gia_LutForEachObj( p, iObj, i )
    {
        if ( Gia_ObjIsCi(iObj) && Gia_ObjFanoutNum(iObj) == 0 )
            continue;
        tArrival = Gia_ObjComputeArrival( p, iObj, 1 );
        tRequired = Gia_ObjComputeRequired( p, iObj, 1 );
        if ( !Gia_LutTimeEqual( tArrival, Gia_ObjTimeArrival( p, iObj), (float)0.01 ) )
            printf( "Gia_LutVerifyTiming(): Object %d has different arrival time (%.2f) from computed (%.2f).\n", 
                iObj->Id, Gia_ObjTimeArrival( p, iObj), tArrival );
        if ( !Gia_LutTimeEqual( tRequired, Gia_ObjTimeRequired( p, iObj), (float)0.01 ) )
            printf( "Gia_LutVerifyTiming(): Object %d has different required time (%.2f) from computed (%.2f).\n", 
                iObj->Id, Gia_ObjTimeRequired( p, iObj), tRequired );
    }
    return 1;
}

#endif

/**Function*************************************************************

  Synopsis    [Prints the delay trace for the given network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Gia_ManDelayTraceLutPrint( Gia_Man_t * p, int fVerbose )
{
    If_LibLut_t * pLutLib = (If_LibLut_t *)p->pLutLib;
    int i, Nodes, * pCounters;
    float tArrival, tDelta, nSteps, Num;
    // get the library
    if ( pLutLib && pLutLib->LutMax < Gia_ManLutSizeMax(p) )
    {
        printf( "The max LUT size (%d) is less than the max fanin count (%d).\n", 
            pLutLib->LutMax, Gia_ManLutSizeMax(p) );
        return -ABC_INFINITY;
    }
    // decide how many steps
    nSteps = pLutLib ? 20 : Gia_ManLutLevel(p, NULL);
    pCounters = ABC_ALLOC( int, nSteps + 1 );
    memset( pCounters, 0, sizeof(int)*(nSteps + 1) );
    // perform delay trace
    tArrival = Gia_ManDelayTraceLut( p );
    tDelta   = tArrival / nSteps;
    // count how many nodes have slack in the corresponding intervals
    Gia_ManForEachLut( p, i )
    {
        if ( Gia_ObjLutSize(p, i) == 0 )
            continue;
        Num = Gia_ObjTimeSlack(p, i) / tDelta;
        if ( Num > nSteps )
            continue;
        assert( Num >=0 && Num <= nSteps );
        pCounters[(int)Num]++;
    }
    // print the results    
    if ( fVerbose )
    {
        printf( "Max delay = %6.2f. Delay trace using %s model:\n", tArrival, pLutLib? "LUT library" : "unit-delay" );
        Nodes = 0;
        for ( i = 0; i < nSteps; i++ )
        {
            Nodes += pCounters[i];
            printf( "%3d %s : %5d  (%6.2f %%)\n", pLutLib? 5*(i+1) : i+1, 
                pLutLib? "%":"lev", Nodes, 100.0*Nodes/Gia_ManLutNum(p) );
        }
    }
    ABC_FREE( pCounters );
    Gia_ManTimeStop( p );
    return tArrival;
}

/**Function*************************************************************

  Synopsis    [Determines timing-critical edges of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Gia_LutDelayTraceTCEdges( Gia_Man_t * p, int iObj, float tDelta )
{
    If_LibLut_t * pLutLib = (If_LibLut_t *)p->pLutLib;
    int pPinPerm[32];
    float pPinDelays[32];
    float tRequired, * pDelays;
    unsigned uResult = 0;
    int k, iFanin;
    tRequired = Gia_ObjTimeRequired( p, iObj );
    if ( pLutLib == NULL )
    {
        Gia_LutForEachFanin( p, iObj, iFanin, k )
            if ( tRequired < Gia_ObjTimeArrival(p, iFanin) + 1.0 + tDelta )
                uResult |= (1 << k);
    }
    else if ( !pLutLib->fVarPinDelays )
    {
        pDelays = pLutLib->pLutDelays[Gia_ObjLutSize(p, iObj)];
        Gia_LutForEachFanin( p, iObj, iFanin, k )
            if ( tRequired < Gia_ObjTimeArrival(p, iFanin) + pDelays[0] + tDelta )
                uResult |= (1 << k);
    }
    else
    {
        pDelays = pLutLib->pLutDelays[Gia_ObjLutSize(p, iObj)];
        Gia_LutDelayTraceSortPins( p, iObj, pPinPerm, pPinDelays );
        Gia_LutForEachFanin( p, iObj, iFanin, k )
            if ( tRequired < Gia_ObjTimeArrival( p, Gia_ObjLutFanin(p, iObj,pPinPerm[k])) + pDelays[k] + tDelta )
                uResult |= (1 << pPinPerm[k]);
    }
    return uResult;
}

/**Function*************************************************************

  Synopsis    [Adds strashed nodes for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSpeedupObj_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return 1;
    Gia_ObjSetTravIdCurrent( p, pObj );
    if ( Gia_ObjIsCi(pObj) )
        return 0;
    assert( Gia_ObjIsAnd(pObj) );
    if ( !Gia_ManSpeedupObj_rec( p, Gia_ObjFanin0(pObj), vNodes ) )
        return 0;
    if ( !Gia_ManSpeedupObj_rec( p, Gia_ObjFanin1(pObj), vNodes ) )
        return 0;
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds strashed nodes for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManSpeedupObj( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vLeaves, Vec_Int_t * vTimes )
{
    Vec_Int_t * vNodes;
    Gia_Obj_t * pTemp = NULL;
    int pCofs[32], nCofs, nSkip, i, k, iResult, iObj;
    // mark the leaves
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent( p, Gia_ManConst0(p) );
    Gia_ManForEachObjVec( vLeaves, p, pTemp, i )
        Gia_ObjSetTravIdCurrent( p, pTemp );
    // collect the AIG nodes
    vNodes = Vec_IntAlloc( 100 );
    if ( !Gia_ManSpeedupObj_rec( p, pObj, vNodes ) )
    {
        printf( "Bad node!!!\n" );
        Vec_IntFree( vNodes );
        return;
    }
    // derive cofactors
    nCofs = (1 << Vec_IntSize(vTimes));
    for ( i = 0; i < nCofs; i++ )
    {
        Gia_ManForEachObjVec( vLeaves, p, pTemp, k )
            pTemp->Value = Abc_Var2Lit( Gia_ObjId(p, pTemp), 0 );
        Gia_ManForEachObjVec( vTimes, p, pTemp, k )
            pTemp->Value = ((i & (1<<k)) != 0);
        Gia_ManForEachObjVec( vNodes, p, pTemp, k )
            pTemp->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pTemp), Gia_ObjFanin1Copy(pTemp) );
        pCofs[i] = pTemp->Value;
    }
    Vec_IntFree( vNodes );
    // collect the resulting tree
    Gia_ManForEachObjVec( vTimes, p, pTemp, k )
        for ( nSkip = (1<<k), i = 0; i < nCofs; i += 2*nSkip )
            pCofs[i] = Gia_ManHashMux( pNew, Gia_ObjToLit(p,pTemp), pCofs[i+nSkip], pCofs[i] );
    // create choice node  (pObj is repr and ppCofs[0] is new)
    iObj    = Gia_ObjId( p, pObj );
    iResult = Abc_Lit2Var( pCofs[0] );
    if ( iResult <= iObj )
        return;
    Gia_ObjSetRepr( pNew, iResult, iObj );
    Gia_ObjSetNext( pNew, iResult, Gia_ObjNext(pNew, iObj) );
    Gia_ObjSetNext( pNew, iObj, iResult );
}

/**Function*************************************************************

  Synopsis    [Adds choices to speed up the network by the given percentage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManSpeedup( Gia_Man_t * p, int Percentage, int Degree, int fVerbose, int fVeryVerbose )
{
    Gia_Man_t * pNew, * pTemp;
    Vec_Int_t * vTimeCries, * vTimeFanins;
    int iObj, iFanin, iFanin2, nNodesNew;
    float tDelta, tArrival;
    int i, k, k2, Counter, CounterRes, nTimeCris;
    int fUseLutLib = (p->pLutLib != NULL);
    void * pTempTim = NULL;
    unsigned * puTCEdges;
    assert( Gia_ManHasMapping(p) );
    if ( !fUseLutLib && p->pManTime )
    {
        pTempTim = p->pManTime;
        p->pManTime = Tim_ManDup( (Tim_Man_t *)pTempTim, 1 );
    }
    // perform delay trace
    tArrival = Gia_ManDelayTraceLut( p );
    tDelta = fUseLutLib ? tArrival*Percentage/100.0 : 1.0;
    if ( fVerbose )
    {
        printf( "Max delay = %.2f. Delta = %.2f. ", tArrival, tDelta );
        printf( "Using %s model. ", fUseLutLib ? "LUT library" : "unit-delay" );
        if ( fUseLutLib )
            printf( "Percentage = %d. ", Percentage );
        printf( "\n" );
    }
    // mark the timing critical nodes and edges
    puTCEdges = ABC_CALLOC( unsigned, Gia_ManObjNum(p) );
    Gia_ManForEachLut( p, iObj )
    {
        if ( Gia_ObjTimeSlack(p, iObj) >= tDelta )
            continue;
        puTCEdges[iObj] = Gia_LutDelayTraceTCEdges( p, iObj, tDelta );
    }
    if ( fVerbose )
    {
        Counter = CounterRes = 0;
        Gia_ManForEachLut( p, iObj )
        {
            Gia_LutForEachFanin( p, iObj, iFanin, k )
                if ( !Gia_ObjIsCi(Gia_ManObj(p, iFanin)) && Gia_ObjTimeSlack(p, iFanin) < tDelta )
                    Counter++;
            CounterRes += Gia_WordCountOnes( puTCEdges[iObj] );
        }
        printf( "Edges: Total = %7d. 0-slack = %7d. Critical = %7d. Ratio = %4.2f\n", 
            Gia_ManLutFaninCount(p), Counter, CounterRes, Counter? 1.0*CounterRes/Counter : 0.0 );
    }

    // start the resulting network
    pNew = Gia_ManDup( p );
    Gia_ManHashStart( pNew );
    nNodesNew = 1000 + 3 * Gia_ManObjNum(pNew);
    pNew->pNexts = ABC_CALLOC( int, nNodesNew );
    pNew->pReprs = ABC_CALLOC( Gia_Rpr_t, nNodesNew );
    for ( i = 0; i < nNodesNew; i++ )
        Gia_ObjSetRepr( pNew, i, GIA_VOID );

    // collect nodes to be used for resynthesis
    Counter = CounterRes = 0;
    vTimeCries = Vec_IntAlloc( 16 );
    vTimeFanins = Vec_IntAlloc( 16 );
    Gia_ManForEachLut( p, iObj )
    {
        if ( Gia_ObjTimeSlack(p, iObj) >= tDelta )
            continue;
        // count the number of non-PI timing-critical nodes
        nTimeCris = 0;
        Gia_LutForEachFanin( p, iObj, iFanin, k )
            if ( !Gia_ObjIsCi(Gia_ManObj(p, iFanin)) && (puTCEdges[iObj] & (1<<k)) )
                nTimeCris++;
        if ( !fVeryVerbose && nTimeCris == 0 )
            continue;
        Counter++;
        // count the total number of timing critical second-generation nodes
        Vec_IntClear( vTimeCries );
        if ( nTimeCris )
        {
            Gia_LutForEachFanin( p, iObj, iFanin, k )
                if ( !Gia_ObjIsCi(Gia_ManObj(p, iFanin)) && (puTCEdges[iObj] & (1<<k)) )
                    Gia_LutForEachFanin( p, iFanin, iFanin2, k2 )
                        if ( puTCEdges[iFanin] & (1<<k2) )
                            Vec_IntPushUnique( vTimeCries, iFanin2 );
        }
//        if ( !fVeryVerbose && (Vec_IntSize(vTimeCries) == 0 || Vec_IntSize(vTimeCries) > Degree) )
        if ( (Vec_IntSize(vTimeCries) == 0 || Vec_IntSize(vTimeCries) > Degree) )
            continue;
        CounterRes++;
        // collect second generation nodes
        Vec_IntClear( vTimeFanins );
        Gia_LutForEachFanin( p, iObj, iFanin, k )
        {
            if ( Gia_ObjIsCi(Gia_ManObj(p, iFanin)) )
                Vec_IntPushUnique( vTimeFanins, iFanin );
            else
                Gia_LutForEachFanin( p, iFanin, iFanin2, k2 )
                    Vec_IntPushUnique( vTimeFanins, iFanin2 );                    
        }
        // print the results
        if ( fVeryVerbose )
        {
        printf( "%5d Node %5d : %d %2d %2d  ", Counter, iObj, 
            nTimeCris, Vec_IntSize(vTimeCries), Vec_IntSize(vTimeFanins) );
        Gia_LutForEachFanin( p, iObj, iFanin, k )
            printf( "%d(%.2f)%s ", iFanin, Gia_ObjTimeSlack(p, iFanin), (puTCEdges[iObj] & (1<<k))? "*":"" );
        printf( "\n" );
        }
        // add the node to choices
        if ( Vec_IntSize(vTimeCries) == 0 || Vec_IntSize(vTimeCries) > Degree )
            continue;
        // order the fanins in the increasing order of criticalily
        if ( Vec_IntSize(vTimeCries) > 1 )
        {
            iFanin = Vec_IntEntry( vTimeCries, 0 );
            iFanin2 = Vec_IntEntry( vTimeCries, 1 );
            if ( Gia_ObjTimeSlack(p, iFanin) < Gia_ObjTimeSlack(p, iFanin2) )
            {
                Vec_IntWriteEntry( vTimeCries, 0, iFanin2 );
                Vec_IntWriteEntry( vTimeCries, 1, iFanin );
            }
        }
        if ( Vec_IntSize(vTimeCries) > 2 )
        {
            iFanin = Vec_IntEntry( vTimeCries, 1 );
            iFanin2 = Vec_IntEntry( vTimeCries, 2 );
            if ( Gia_ObjTimeSlack(p, iFanin) < Gia_ObjTimeSlack(p, iFanin2) )
            {
                Vec_IntWriteEntry( vTimeCries, 1, iFanin2 );
                Vec_IntWriteEntry( vTimeCries, 2, iFanin );
            }
            iFanin = Vec_IntEntry( vTimeCries, 0 );
            iFanin2 = Vec_IntEntry( vTimeCries, 1 );
            if ( Gia_ObjTimeSlack(p, iFanin) < Gia_ObjTimeSlack(p, iFanin2) )
            {
                Vec_IntWriteEntry( vTimeCries, 0, iFanin2 );
                Vec_IntWriteEntry( vTimeCries, 1, iFanin );
            }
        }
        // add choice
        Gia_ManSpeedupObj( pNew, p, Gia_ManObj(p,iObj), vTimeFanins, vTimeCries );
        // quit if the number of nodes is large
        if ( Gia_ManObjNum(pNew) > nNodesNew - 100 )
        {
            printf( "Speedup stopped adding choices because there was too many to add.\n" );
            break;
        }
    }
    Gia_ManTimeStop( p );
    Vec_IntFree( vTimeCries );
    Vec_IntFree( vTimeFanins );
    ABC_FREE( puTCEdges );
    if ( fVerbose )
        printf( "Nodes: Total = %7d. 0-slack = %7d. Workable = %7d. Ratio = %4.2f\n", 
        Gia_ManLutNum(p), Counter, CounterRes, Counter? 1.0*CounterRes/Counter : 0.0 ); 
    if ( pTempTim )
    {
        Tim_ManStop( (Tim_Man_t *)p->pManTime );
        p->pManTime = pTempTim;
    }
    // derive AIG with choices
//Gia_ManPrintStats( pNew, 0 );
    pTemp = Gia_ManEquivToChoices( pNew, 1 );
    Gia_ManStop( pNew );
//Gia_ManPrintStats( pTemp, 0 );
//    pNew = Gia_ManDupOrderDfsChoices( pTemp );
//    Gia_ManStop( pTemp );
//Gia_ManPrintStats( pNew, 0 );
//    return pNew;
    return pTemp;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

