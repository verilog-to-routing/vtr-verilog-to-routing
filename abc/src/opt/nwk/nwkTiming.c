/**CFile****************************************************************

  FileName    [nwkTiming.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Logic network representation.]

  Synopsis    [Manipulation of timing information.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwkTiming.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#include "nwk.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Cleans timing information for all nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManCleanTiming( Nwk_Man_t * pNtk )
{
    Nwk_Obj_t * pObj;
    int i;
    Nwk_ManForEachObj( pNtk, pObj, i )
    {
        pObj->tArrival = pObj->tSlack = 0.0;
        pObj->tRequired = TIM_ETERNITY;
    }
}

/**Function*************************************************************

  Synopsis    [Sorts the pins in the decreasing order of delays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManDelayTraceSortPins( Nwk_Obj_t * pNode, int * pPinPerm, float * pPinDelays )
{
    Nwk_Obj_t * pFanin;
    int i, j, best_i, temp;
    // start the trivial permutation and collect pin delays
    Nwk_ObjForEachFanin( pNode, pFanin, i )
    {
        pPinPerm[i] = i;
        pPinDelays[i] = Nwk_ObjArrival(pFanin);
    }
    // selection sort the pins in the decreasible order of delays
    // this order will match the increasing order of LUT input pins
    for ( i = 0; i < Nwk_ObjFaninNum(pNode)-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < Nwk_ObjFaninNum(pNode); j++ )
            if ( pPinDelays[pPinPerm[j]] > pPinDelays[pPinPerm[best_i]] )
                best_i = j;
        if ( best_i == i )
            continue;
        temp = pPinPerm[i]; 
        pPinPerm[i] = pPinPerm[best_i]; 
        pPinPerm[best_i] = temp;
    }
    // verify
    assert( Nwk_ObjFaninNum(pNode) == 0 || pPinPerm[0] < Nwk_ObjFaninNum(pNode) );
    for ( i = 1; i < Nwk_ObjFaninNum(pNode); i++ )
    {
        assert( pPinPerm[i] < Nwk_ObjFaninNum(pNode) );
        assert( pPinDelays[pPinPerm[i-1]] >= pPinDelays[pPinPerm[i]] );
    }
}

/**Function*************************************************************

  Synopsis    [Sorts the pins in the decreasing order of delays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManWhereIsPin( Nwk_Obj_t * pFanout, Nwk_Obj_t * pFanin, int * pPinPerm )
{
    int i;
    for ( i = 0; i < Nwk_ObjFaninNum(pFanout); i++ )
        if ( Nwk_ObjFanin(pFanout, pPinPerm[i]) == pFanin )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Computes the arrival times for the given object.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Nwk_NodeComputeArrival( Nwk_Obj_t * pObj, int fUseSorting )
{
    If_LibLut_t * pLutLib = pObj->pMan->pLutLib;
    int pPinPerm[32];
    float pPinDelays[32];
    Nwk_Obj_t * pFanin;
    float tArrival, * pDelays;
    int k;
    assert( Nwk_ObjIsNode(pObj) || Nwk_ObjIsCi(pObj) || Nwk_ObjIsCo(pObj) );
    if ( Nwk_ObjIsCi(pObj) )
        return Nwk_ObjArrival(pObj);
    if ( Nwk_ObjIsCo(pObj) )
        return Nwk_ObjArrival( Nwk_ObjFanin0(pObj) );
    tArrival = -TIM_ETERNITY;
    if ( pLutLib == NULL )
    {
        Nwk_ObjForEachFanin( pObj, pFanin, k )
            if ( tArrival < Nwk_ObjArrival(pFanin) + 1.0 )
                tArrival = Nwk_ObjArrival(pFanin) + 1.0;
    }
    else if ( !pLutLib->fVarPinDelays )
    {
        pDelays = pLutLib->pLutDelays[Nwk_ObjFaninNum(pObj)];
        Nwk_ObjForEachFanin( pObj, pFanin, k )
            if ( tArrival < Nwk_ObjArrival(pFanin) + pDelays[0] )
                tArrival = Nwk_ObjArrival(pFanin) + pDelays[0];
    }
    else
    {
        pDelays = pLutLib->pLutDelays[Nwk_ObjFaninNum(pObj)];
        if ( fUseSorting )
        {
            Nwk_ManDelayTraceSortPins( pObj, pPinPerm, pPinDelays );
            Nwk_ObjForEachFanin( pObj, pFanin, k ) 
                if ( tArrival < Nwk_ObjArrival(Nwk_ObjFanin(pObj,pPinPerm[k])) + pDelays[k] )
                    tArrival = Nwk_ObjArrival(Nwk_ObjFanin(pObj,pPinPerm[k])) + pDelays[k];
        }
        else
        {
            Nwk_ObjForEachFanin( pObj, pFanin, k )
                if ( tArrival < Nwk_ObjArrival(pFanin) + pDelays[k] )
                    tArrival = Nwk_ObjArrival(pFanin) + pDelays[k];
        }
    }
    if ( Nwk_ObjFaninNum(pObj) == 0 )
        tArrival = 0.0;
    return tArrival;
}

/**Function*************************************************************

  Synopsis    [Computes the required times for the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Nwk_NodeComputeRequired( Nwk_Obj_t * pObj, int fUseSorting )
{
    If_LibLut_t * pLutLib = pObj->pMan->pLutLib;
    int pPinPerm[32];
    float pPinDelays[32];
    Nwk_Obj_t * pFanout;
    float tRequired, tDelay, * pDelays;
    int k, iFanin;
    assert( Nwk_ObjIsNode(pObj) || Nwk_ObjIsCi(pObj) || Nwk_ObjIsCo(pObj) );
    if ( Nwk_ObjIsCo(pObj) )
        return Nwk_ObjRequired(pObj);
    tRequired = TIM_ETERNITY;
    if ( pLutLib == NULL )
    {
        Nwk_ObjForEachFanout( pObj, pFanout, k )
        {
            tDelay = Nwk_ObjIsCo(pFanout)? 0.0 : 1.0;
            if ( tRequired > Nwk_ObjRequired(pFanout) - tDelay )
                tRequired = Nwk_ObjRequired(pFanout) - tDelay;
        }
    }
    else if ( !pLutLib->fVarPinDelays )
    {
        Nwk_ObjForEachFanout( pObj, pFanout, k )
        {
            pDelays = pLutLib->pLutDelays[Nwk_ObjFaninNum(pFanout)];
            tDelay = Nwk_ObjIsCo(pFanout)? 0.0 : pDelays[0];
            if ( tRequired > Nwk_ObjRequired(pFanout) - tDelay )
                tRequired = Nwk_ObjRequired(pFanout) - tDelay;
        }
    }
    else
    {
        if ( fUseSorting )
        {
            Nwk_ObjForEachFanout( pObj, pFanout, k ) 
            {
                pDelays = pLutLib->pLutDelays[Nwk_ObjFaninNum(pFanout)];
                Nwk_ManDelayTraceSortPins( pFanout, pPinPerm, pPinDelays );
                iFanin = Nwk_ManWhereIsPin( pFanout, pObj, pPinPerm );
                assert( Nwk_ObjFanin(pFanout,pPinPerm[iFanin]) == pObj );
                tDelay = Nwk_ObjIsCo(pFanout)? 0.0 : pDelays[iFanin];
                if ( tRequired > Nwk_ObjRequired(pFanout) - tDelay )
                    tRequired = Nwk_ObjRequired(pFanout) - tDelay;
            }
        }
        else
        {
            Nwk_ObjForEachFanout( pObj, pFanout, k )
            {
                pDelays = pLutLib->pLutDelays[Nwk_ObjFaninNum(pFanout)];
                iFanin = Nwk_ObjFindFanin( pFanout, pObj );
                assert( Nwk_ObjFanin(pFanout,iFanin) == pObj );
                tDelay = Nwk_ObjIsCo(pFanout)? 0.0 : pDelays[iFanin];
                if ( tRequired > Nwk_ObjRequired(pFanout) - tDelay )
                    tRequired = Nwk_ObjRequired(pFanout) - tDelay;
            }
        }
    }
    return tRequired;
}

/**Function*************************************************************

  Synopsis    [Propagates the required times through the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Nwk_NodePropagateRequired( Nwk_Obj_t * pObj, int fUseSorting )
{
    If_LibLut_t * pLutLib = pObj->pMan->pLutLib;
    int pPinPerm[32];
    float pPinDelays[32];
    Nwk_Obj_t * pFanin;
    float tRequired = 0.0; // Suppress "might be used uninitialized"
    float * pDelays;
    int k;
    assert( Nwk_ObjIsNode(pObj) );
    if ( pLutLib == NULL )
    {
        tRequired = Nwk_ObjRequired(pObj) - (float)1.0;
        Nwk_ObjForEachFanin( pObj, pFanin, k )
            if ( Nwk_ObjRequired(pFanin) > tRequired )
                Nwk_ObjSetRequired( pFanin, tRequired );
    }
    else if ( !pLutLib->fVarPinDelays )
    {
        pDelays = pLutLib->pLutDelays[Nwk_ObjFaninNum(pObj)];
        tRequired = Nwk_ObjRequired(pObj) - pDelays[0];
        Nwk_ObjForEachFanin( pObj, pFanin, k )
            if ( Nwk_ObjRequired(pFanin) > tRequired )
                Nwk_ObjSetRequired( pFanin, tRequired );
    }
    else 
    {
        pDelays = pLutLib->pLutDelays[Nwk_ObjFaninNum(pObj)];
        if ( fUseSorting )
        {
            Nwk_ManDelayTraceSortPins( pObj, pPinPerm, pPinDelays );
            Nwk_ObjForEachFanin( pObj, pFanin, k )
            {
                tRequired = Nwk_ObjRequired(pObj) - pDelays[k];
                if ( Nwk_ObjRequired(Nwk_ObjFanin(pObj,pPinPerm[k])) > tRequired )
                    Nwk_ObjSetRequired( Nwk_ObjFanin(pObj,pPinPerm[k]), tRequired );
            }
        }
        else
        {
            Nwk_ObjForEachFanin( pObj, pFanin, k )
            {
                tRequired = Nwk_ObjRequired(pObj) - pDelays[k];
                if ( Nwk_ObjRequired(pFanin) > tRequired )
                    Nwk_ObjSetRequired( pFanin, tRequired );
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
float Nwk_ManDelayTraceLut( Nwk_Man_t * pNtk )
{
    Vec_Ptr_t * vObjs;
    int fUseSorting = 1;
    If_LibLut_t * pLutLib = pNtk->pLutLib;
    Vec_Ptr_t * vNodes;
    Nwk_Obj_t * pObj;
    float tArrival, tRequired, tSlack;
    int i;

    // get the library
    if ( pLutLib && pLutLib->LutMax < Nwk_ManGetFaninMax(pNtk) )
    {
        printf( "The max LUT size (%d) is less than the max fanin count (%d).\n", 
            pLutLib->LutMax, Nwk_ManGetFaninMax(pNtk) );
        return -TIM_ETERNITY;
    }

    // compute the reverse order of all objects
    vNodes = Nwk_ManDfsReverse( pNtk );

    // initialize the arrival times
    Nwk_ManCleanTiming( pNtk );

    // propagate arrival times
    if ( pNtk->pManTime )
        Tim_ManIncrementTravId( pNtk->pManTime );
//    Nwk_ManForEachObj( pNtk, pObj, i )
    vObjs = Nwk_ManDfs( pNtk );
    Vec_PtrForEachEntry( Nwk_Obj_t *, vObjs, pObj, i )
    {
        tArrival = Nwk_NodeComputeArrival( pObj, fUseSorting );
        if ( Nwk_ObjIsCi(pObj) && pNtk->pManTime )
            tArrival = Tim_ManGetCiArrival( pNtk->pManTime, pObj->PioId );
        if ( Nwk_ObjIsCo(pObj) && pNtk->pManTime )
            Tim_ManSetCoArrival( pNtk->pManTime, pObj->PioId, tArrival );
        Nwk_ObjSetArrival( pObj, tArrival );
    }
    Vec_PtrFree( vObjs );

    // get the latest arrival times
    tArrival = -TIM_ETERNITY;
    Nwk_ManForEachPo( pNtk, pObj, i )
        if ( tArrival < Nwk_ObjArrival(pObj) )
            tArrival = Nwk_ObjArrival(pObj);

    // initialize the required times
    if ( pNtk->pManTime )
    {
        Tim_ManIncrementTravId( pNtk->pManTime );
        Tim_ManInitPoRequiredAll( pNtk->pManTime, tArrival );
    }
    else
    {
        Nwk_ManForEachCo( pNtk, pObj, i )
            Nwk_ObjSetRequired( pObj, tArrival );
    }

    // propagate the required times
    Vec_PtrForEachEntry( Nwk_Obj_t *, vNodes, pObj, i )
    {
        if ( Nwk_ObjIsNode(pObj) )
        {
            Nwk_NodePropagateRequired( pObj, fUseSorting );
        }
        else if ( Nwk_ObjIsCi(pObj) )
        {
            if ( pNtk->pManTime )
                Tim_ManSetCiRequired( pNtk->pManTime, pObj->PioId, Nwk_ObjRequired(pObj) );
        }
        else if ( Nwk_ObjIsCo(pObj) )
        {
            if ( pNtk->pManTime )
            {
                tRequired = Tim_ManGetCoRequired( pNtk->pManTime, pObj->PioId );
                Nwk_ObjSetRequired( pObj, tRequired );
            }
            if ( Nwk_ObjRequired(Nwk_ObjFanin0(pObj)) > Nwk_ObjRequired(pObj) )
                Nwk_ObjSetRequired( Nwk_ObjFanin0(pObj), Nwk_ObjRequired(pObj) );
        }

        // set slack for this object
        tSlack = Nwk_ObjRequired(pObj) - Nwk_ObjArrival(pObj);
        assert( tSlack + 0.01 > 0.0 );
        Nwk_ObjSetSlack( pObj, tSlack < 0.0 ? 0.0 : tSlack );
    }
    Vec_PtrFree( vNodes );
    return tArrival;
}

/**Function*************************************************************

  Synopsis    [Computes the arrival times for the given node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManVerifyTiming(  Nwk_Man_t * pNtk )
{
    Nwk_Obj_t * pObj;
    float tArrival, tRequired;
    int i;
    Nwk_ManForEachObj( pNtk, pObj, i )
    {
        if ( Nwk_ObjIsCi(pObj) && Nwk_ObjFanoutNum(pObj) == 0 )
            continue;
        tArrival = Nwk_NodeComputeArrival( pObj, 1 );
        tRequired = Nwk_NodeComputeRequired( pObj, 1 );
        if ( !Nwk_ManTimeEqual( tArrival, Nwk_ObjArrival(pObj), (float)0.01 ) )
            printf( "Nwk_ManVerifyTiming(): Object %d has different arrival time (%.2f) from computed (%.2f).\n", 
                pObj->Id, Nwk_ObjArrival(pObj), tArrival );
        if ( !Nwk_ManTimeEqual( tRequired, Nwk_ObjRequired(pObj), (float)0.01 ) )
            printf( "Nwk_ManVerifyTiming(): Object %d has different required time (%.2f) from computed (%.2f).\n", 
                pObj->Id, Nwk_ObjRequired(pObj), tRequired );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prints the delay trace for the given network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManDelayTracePrint( Nwk_Man_t * pNtk )
{
    If_LibLut_t * pLutLib = pNtk->pLutLib;
    Nwk_Obj_t * pNode;
    int i, Nodes, * pCounters;
    float tArrival, tDelta, nSteps, Num;
    // get the library
    if ( pLutLib && pLutLib->LutMax < Nwk_ManGetFaninMax(pNtk) )
    {
        printf( "The max LUT size (%d) is less than the max fanin count (%d).\n", 
            pLutLib->LutMax, Nwk_ManGetFaninMax(pNtk) );
        return;
    }
    // decide how many steps
    nSteps = pLutLib ? 20 : Nwk_ManLevelMax(pNtk);
    pCounters = ABC_ALLOC( int, nSteps + 1 );
    memset( pCounters, 0, sizeof(int)*(nSteps + 1) );
    // perform delay trace
    tArrival = Nwk_ManDelayTraceLut( pNtk );
    tDelta = tArrival / nSteps;
    // count how many nodes have slack in the corresponding intervals
    Nwk_ManForEachNode( pNtk, pNode, i )
    {
        if ( Nwk_ObjFaninNum(pNode) == 0 )
            continue;
        Num = Nwk_ObjSlack(pNode) / tDelta;
        if ( Num > nSteps )
            continue;
        assert( Num >=0 && Num <= nSteps );
        pCounters[(int)Num]++;
    }
    // print the results
    printf( "Max delay = %6.2f. Delay trace using %s model:\n", tArrival, pLutLib? "LUT library" : "unit-delay" );
    Nodes = 0;
    for ( i = 0; i < nSteps; i++ )
    {
        Nodes += pCounters[i];
        printf( "%3d %s : %5d  (%6.2f %%)\n", pLutLib? 5*(i+1) : i+1, 
            pLutLib? "%":"lev", Nodes, 100.0*Nodes/Nwk_ManNodeNum(pNtk) );
    }
    ABC_FREE( pCounters );
}


/**Function*************************************************************

  Synopsis    [Inserts node into the queue of nodes sorted by level.]

  Description [The inserted node should not go before the current position 
  given by iCurrent. If the arrival times are computed, the nodes are sorted
  in the increasing order of levels. If the required times are computed, 
  the nodes are sorted in the decreasing order of levels.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_NodeUpdateAddToQueue( Vec_Ptr_t * vQueue, Nwk_Obj_t * pObj, int iCurrent, int fArrival )
{
    Nwk_Obj_t * pTemp1, * pTemp2;
    int i;
    Vec_PtrPush( vQueue, pObj );
    for ( i = Vec_PtrSize(vQueue) - 1; i > iCurrent + 1; i-- )
    {
        pTemp1 = (Nwk_Obj_t *)vQueue->pArray[i];
        pTemp2 = (Nwk_Obj_t *)vQueue->pArray[i-1];
        if ( fArrival )
        {
            if ( Nwk_ObjLevel(pTemp2) <= Nwk_ObjLevel(pTemp1) )
                break;
        }
        else
        {
            if ( Nwk_ObjLevel(pTemp2) >= Nwk_ObjLevel(pTemp1) )
                break;
        }
        vQueue->pArray[i-1] = pTemp1;
        vQueue->pArray[i]   = pTemp2;
    }
    // verification
    for ( i = iCurrent + 1; i < Vec_PtrSize(vQueue) - 1; i++ )
    {
        pTemp1 = (Nwk_Obj_t *)vQueue->pArray[i];
        pTemp2 = (Nwk_Obj_t *)vQueue->pArray[i+1];
        if ( fArrival )
            assert( Nwk_ObjLevel(pTemp1) <= Nwk_ObjLevel(pTemp2) );
        else
            assert( Nwk_ObjLevel(pTemp1) >= Nwk_ObjLevel(pTemp2) );
    }
}

/**Function*************************************************************

  Synopsis    [Incrementally updates arrival times of the node.]

  Description [Supports variable-pin delay model and white-boxes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_NodeUpdateArrival( Nwk_Obj_t * pObj )
{
    Tim_Man_t * pManTime = pObj->pMan->pManTime;
    Vec_Ptr_t * vQueue = pObj->pMan->vTemp;
    Nwk_Obj_t * pTemp;
    Nwk_Obj_t * pNext = NULL; // Suppress "might be used uninitialized"
    float tArrival;
    int iCur, k, iBox, iTerm1, nTerms;
    assert( Nwk_ObjIsNode(pObj) );
    // verify the arrival time
    tArrival = Nwk_NodeComputeArrival( pObj, 1 );
    assert( Nwk_ManTimeLess( tArrival, Nwk_ObjRequired(pObj), (float)0.01 ) );
    // initialize the queue with the node
    Vec_PtrClear( vQueue );
    Vec_PtrPush( vQueue, pObj );
    pObj->MarkA = 1;
    // process objects
    if ( pManTime )
        Tim_ManIncrementTravId( pManTime );
    Vec_PtrForEachEntry( Nwk_Obj_t *, vQueue, pTemp, iCur )
    {
        pTemp->MarkA = 0;
        tArrival = Nwk_NodeComputeArrival( pTemp, 1 );
        if ( Nwk_ObjIsCi(pTemp) && pManTime )
            tArrival = Tim_ManGetCiArrival( pManTime, pTemp->PioId );
        if ( Nwk_ManTimeEqual( tArrival, Nwk_ObjArrival(pTemp), (float)0.01 ) )
            continue;
        Nwk_ObjSetArrival( pTemp, tArrival );
        // add the fanouts to the queue
        if ( Nwk_ObjIsCo(pTemp) )
        {
            if ( pManTime )
            {
                iBox = Tim_ManBoxForCo( pManTime, pTemp->PioId );
                if ( iBox >= 0 ) // this CO is an input of the box
                {
                    // it may happen that a box-input (CO) was already marked as visited
                    // when some other box-input of the same box was visited - here we undo this
                    if ( Tim_ManIsCoTravIdCurrent( pManTime, pTemp->PioId ) )
                        Tim_ManSetPreviousTravIdBoxInputs( pManTime, iBox );
                    Tim_ManSetCoArrival( pManTime, pTemp->PioId, tArrival );
                    Tim_ManSetCurrentTravIdBoxInputs( pManTime, iBox );
                    iTerm1 = Tim_ManBoxOutputFirst( pManTime, iBox );
                    nTerms = Tim_ManBoxOutputNum( pManTime, iBox );
                    for ( k = 0; k < nTerms; k++ )
                    {
                        pNext = Nwk_ManCi(pNext->pMan, iTerm1 + k);
                        if ( pNext->MarkA )
                            continue;
                        Nwk_NodeUpdateAddToQueue( vQueue, pNext, iCur, 1 );
                        pNext->MarkA = 1;
                    }
                }
            }
        }
        else
        {
            Nwk_ObjForEachFanout( pTemp, pNext, k )
            {
                if ( pNext->MarkA )
                    continue;
                Nwk_NodeUpdateAddToQueue( vQueue, pNext, iCur, 1 );
                pNext->MarkA = 1;
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Incrementally updates required times of the node.]

  Description [Supports variable-pin delay model and white-boxes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_NodeUpdateRequired( Nwk_Obj_t * pObj )
{
    Tim_Man_t * pManTime = pObj->pMan->pManTime;
    Vec_Ptr_t * vQueue = pObj->pMan->vTemp;
    Nwk_Obj_t * pTemp;
    Nwk_Obj_t * pNext = NULL; // Suppress "might be used uninitialized"
    float tRequired;
    int iCur, k, iBox, iTerm1, nTerms;
    assert( Nwk_ObjIsNode(pObj) );
    // make sure the node's required time remained the same
    tRequired = Nwk_NodeComputeRequired( pObj, 1 );
    assert( Nwk_ManTimeEqual( tRequired, Nwk_ObjRequired(pObj), (float)0.01 ) );
    // initialize the queue with the node's faninsa and the old node's fanins
    Vec_PtrClear( vQueue );
    Nwk_ObjForEachFanin( pObj, pNext, k )
    {
        if ( pNext->MarkA )
            continue;
        Nwk_NodeUpdateAddToQueue( vQueue, pNext, -1, 0 );
        pNext->MarkA = 1;
    }
    // process objects
    if ( pManTime )
        Tim_ManIncrementTravId( pManTime );
    Vec_PtrForEachEntry( Nwk_Obj_t *, vQueue, pTemp, iCur )
    {
        pTemp->MarkA = 0;
        tRequired = Nwk_NodeComputeRequired( pTemp, 1 );
        if ( Nwk_ObjIsCo(pTemp) && pManTime )
            tRequired = Tim_ManGetCoRequired( pManTime, pTemp->PioId );
        if ( Nwk_ManTimeEqual( tRequired, Nwk_ObjRequired(pTemp), (float)0.01 ) )
            continue;
        Nwk_ObjSetRequired( pTemp, tRequired );
        // add the fanins to the queue
        if ( Nwk_ObjIsCi(pTemp) )
        {
            if ( pManTime )
            {
                iBox = Tim_ManBoxForCi( pManTime, pTemp->PioId );
                if ( iBox >= 0 ) // this CI is an output of the box
                {
                    // it may happen that a box-output (CI) was already marked as visited
                    // when some other box-output of the same box was visited - here we undo this
                    if ( Tim_ManIsCiTravIdCurrent( pManTime, pTemp->PioId ) )
                        Tim_ManSetPreviousTravIdBoxOutputs( pManTime, iBox );
                    Tim_ManSetCiRequired( pManTime, pTemp->PioId, tRequired );
                    Tim_ManSetCurrentTravIdBoxOutputs( pManTime, iBox );
                    iTerm1 = Tim_ManBoxInputFirst( pManTime, iBox );
                    nTerms = Tim_ManBoxInputNum( pManTime, iBox );
                    for ( k = 0; k < nTerms; k++ )
                    {
                        pNext = Nwk_ManCo(pNext->pMan, iTerm1 + k);
                        if ( pNext->MarkA )
                            continue;
                        Nwk_NodeUpdateAddToQueue( vQueue, pNext, iCur, 0 );
                        pNext->MarkA = 1;
                    }
                }
            }
        }
        else
        {
            Nwk_ObjForEachFanin( pTemp, pNext, k )
            {
                if ( pNext->MarkA )
                    continue;
                Nwk_NodeUpdateAddToQueue( vQueue, pNext, iCur, 0 );
                pNext->MarkA = 1;
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Computes the level of the node using its fanin levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ObjLevelNew( Nwk_Obj_t * pObj )
{
    Tim_Man_t * pManTime = pObj->pMan->pManTime;
    Nwk_Obj_t * pFanin;
    int i, iBox, iTerm1, nTerms, Level = 0;
    if ( Nwk_ObjIsCi(pObj) || Nwk_ObjIsLatch(pObj) )
    {
        if ( pManTime )
        {
            iBox = Tim_ManBoxForCi( pManTime, pObj->PioId );
            if ( iBox >= 0 ) // this CI is an output of the box
            {
                iTerm1 = Tim_ManBoxInputFirst( pManTime, iBox );
                nTerms = Tim_ManBoxInputNum( pManTime, iBox );
                for ( i = 0; i < nTerms; i++ )
                {
                    pFanin = Nwk_ManCo(pObj->pMan, iTerm1 + i);
                    Level = Abc_MaxInt( Level, Nwk_ObjLevel(pFanin) );
                }
                Level++;
            }
        }
        return Level;
    }
    assert( Nwk_ObjIsNode(pObj) || Nwk_ObjIsCo(pObj) );
    Nwk_ObjForEachFanin( pObj, pFanin, i )
        Level = Abc_MaxInt( Level, Nwk_ObjLevel(pFanin) );
    return Level + (Nwk_ObjIsNode(pObj) && Nwk_ObjFaninNum(pObj) > 0);
}

/**Function*************************************************************

  Synopsis    [Incrementally updates level of the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManUpdateLevel( Nwk_Obj_t * pObj )
{
    Tim_Man_t * pManTime = pObj->pMan->pManTime;
    Vec_Ptr_t * vQueue = pObj->pMan->vTemp;
    Nwk_Obj_t * pTemp;
    Nwk_Obj_t * pNext = NULL; // Suppress "might be used uninitialized"
    int LevelNew, iCur, k, iBox, iTerm1, nTerms;
    assert( Nwk_ObjIsNode(pObj) );
    // initialize the queue with the node
    Vec_PtrClear( vQueue );
    Vec_PtrPush( vQueue, pObj );
    pObj->MarkA = 1;
    // process objects
    Vec_PtrForEachEntry( Nwk_Obj_t *, vQueue, pTemp, iCur )
    {
        pTemp->MarkA = 0;
        LevelNew = Nwk_ObjLevelNew( pTemp );
        if ( LevelNew == Nwk_ObjLevel(pTemp) )
            continue;
        Nwk_ObjSetLevel( pTemp, LevelNew );
        // add the fanouts to the queue
        if ( Nwk_ObjIsCo(pTemp) )
        {
            if ( pManTime )
            {
                iBox = Tim_ManBoxForCo( pManTime, pTemp->PioId );
                if ( iBox >= 0 ) // this is not a true PO
                {
                    Tim_ManSetCurrentTravIdBoxInputs( pManTime, iBox );
                    iTerm1 = Tim_ManBoxOutputFirst( pManTime, iBox );
                    nTerms = Tim_ManBoxOutputNum( pManTime, iBox );
                    for ( k = 0; k < nTerms; k++ )
                    {
                        pNext = Nwk_ManCi(pNext->pMan, iTerm1 + k);
                        if ( pNext->MarkA )
                            continue;
                        Nwk_NodeUpdateAddToQueue( vQueue, pNext, iCur, 1 );
                        pNext->MarkA = 1;
                    }
                }
            }
        }
        else
        {
            Nwk_ObjForEachFanout( pTemp, pNext, k )
            {
                if ( pNext->MarkA )
                    continue;
                Nwk_NodeUpdateAddToQueue( vQueue, pNext, iCur, 1 );
                pNext->MarkA = 1;
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Computes the level of the node using its fanin levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nwk_ManVerifyLevel( Nwk_Man_t * pNtk )
{
    Nwk_Obj_t * pObj;
    int LevelNew, i;
    Nwk_ManForEachObj( pNtk, pObj, i )
    {
        assert( pObj->MarkA == 0 );
        LevelNew = Nwk_ObjLevelNew( pObj );
        if ( Nwk_ObjLevel(pObj) != LevelNew )
        {
            printf( "Object %6d: Mismatch betweeh levels: Actual = %d. Correct = %d.\n", 
                i, Nwk_ObjLevel(pObj), LevelNew );
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Replaces the node and incrementally updates levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManUpdate( Nwk_Obj_t * pObj, Nwk_Obj_t * pObjNew, Vec_Vec_t * vLevels )
{
    assert( pObj->pMan == pObjNew->pMan );
    assert( pObj != pObjNew );
    assert( Nwk_ObjFanoutNum(pObj) > 0 );
    assert( Nwk_ObjIsNode(pObj) && !Nwk_ObjIsCo(pObjNew) );
    // transfer fanouts to the old node
    Nwk_ObjTransferFanout( pObj, pObjNew );
    // transfer the timing information
    // (this is needed because updating level happens if the level has changed;
    // when we set the old level, it will be recomputed by the level updating
    // procedure, which will update level of other nodes if there is a difference)
    pObjNew->Level = pObj->Level;
    pObjNew->tArrival = pObj->tArrival;
    pObjNew->tRequired = pObj->tRequired;
    // update required times of the old fanins
    pObj->tRequired = TIM_ETERNITY;
    Nwk_NodeUpdateRequired( pObj );
    // remove the old node
    Nwk_ManDeleteNode_rec( pObj );
    // update the information of the new node
    Nwk_ManUpdateLevel( pObjNew );
    Nwk_NodeUpdateArrival( pObjNew );
    Nwk_NodeUpdateRequired( pObjNew );
//Nwk_ManVerifyTiming( pObjNew->pMan );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

