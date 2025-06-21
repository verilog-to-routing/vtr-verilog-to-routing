/**CFile****************************************************************

  FileName    [nwkSpeedup.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Netlist representation.]

  Synopsis    [Global delay optimization using structural choices.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwkSpeedup.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Adds strashed nodes for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManSpeedupNode_rec( Aig_Man_t * pAig, Aig_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    if ( Aig_ObjIsTravIdCurrent(pAig, pNode) )
        return 1;
    if ( Aig_ObjIsCi(pNode) )
        return 0;
    assert( Aig_ObjIsNode(pNode) );
    Aig_ObjSetTravIdCurrent( pAig, pNode );
    if ( !Aig_ManSpeedupNode_rec( pAig, Aig_ObjFanin0(pNode), vNodes ) )
        return 0;
    if ( !Aig_ManSpeedupNode_rec( pAig, Aig_ObjFanin1(pNode), vNodes ) )
        return 0;
    Vec_PtrPush( vNodes, pNode );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds strashed nodes for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManSpeedupNode( Nwk_Man_t * pNtk, Aig_Man_t * pAig, Nwk_Obj_t * pNode, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vTimes )
{
    Vec_Ptr_t * vNodes;
    Nwk_Obj_t * pObj, * pObj2;
    Aig_Obj_t * ppCofs[32], * pAnd, * pTemp;
    int nCofs, i, k, nSkip;

    // quit of regulars are the same
    Vec_PtrForEachEntry( Nwk_Obj_t *, vLeaves, pObj, i )
    Vec_PtrForEachEntry( Nwk_Obj_t *, vLeaves, pObj2, k )
        if ( i != k && Aig_Regular((Aig_Obj_t *)pObj->pCopy) == Aig_Regular((Aig_Obj_t *)pObj2->pCopy) )
        {
//            printf( "Identical after structural hashing!!!\n" );
            return;
        }

    // collect the AIG nodes
    vNodes = Vec_PtrAlloc( 100 );
    Aig_ManIncrementTravId( pAig );
    Aig_ObjSetTravIdCurrent( pAig, Aig_ManConst1(pAig) );
    Vec_PtrForEachEntry( Nwk_Obj_t *, vLeaves, pObj, i )
    {
        pAnd = (Aig_Obj_t *)pObj->pCopy;
        Aig_ObjSetTravIdCurrent( pAig, Aig_Regular(pAnd) );
    }
    // traverse from the root node
    pAnd = (Aig_Obj_t *)pNode->pCopy;
    if ( !Aig_ManSpeedupNode_rec( pAig, Aig_Regular(pAnd), vNodes ) )
    {
//        printf( "Bad node!!!\n" );
        Vec_PtrFree( vNodes );
        return;
    }

    // derive cofactors
    nCofs = (1 << Vec_PtrSize(vTimes));
    for ( i = 0; i < nCofs; i++ )
    {
        Vec_PtrForEachEntry( Nwk_Obj_t *, vLeaves, pObj, k )
        {
            pAnd = (Aig_Obj_t *)pObj->pCopy;
            Aig_Regular(pAnd)->pData = Aig_Regular(pAnd);
        }
        Vec_PtrForEachEntry( Nwk_Obj_t *, vTimes, pObj, k )
        {
            pAnd = (Aig_Obj_t *)pObj->pCopy;
            Aig_Regular(pAnd)->pData = Aig_NotCond( Aig_ManConst1(pAig), ((i & (1<<k)) == 0) );
        }
        Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pTemp, k )
            pTemp->pData = Aig_And( pAig, Aig_ObjChild0Copy(pTemp), Aig_ObjChild1Copy(pTemp) );
        // save the result
        pAnd = (Aig_Obj_t *)pNode->pCopy;
        ppCofs[i] = Aig_NotCond( (Aig_Obj_t *)Aig_Regular(pAnd)->pData, Aig_IsComplement(pAnd) );
    }
    Vec_PtrFree( vNodes );

//Nwk_ObjAddFanin( Nwk_ManCreatePo(pAig), ppCofs[0] );
//Nwk_ObjAddFanin( Nwk_ManCreatePo(pAig), ppCofs[1] );

    // collect the resulting tree
    Vec_PtrForEachEntry( Nwk_Obj_t *, vTimes, pObj, k )
        for ( nSkip = (1<<k), i = 0; i < nCofs; i += 2*nSkip )
        {
            pAnd = (Aig_Obj_t *)pObj->pCopy;
            ppCofs[i] = Aig_Mux( pAig, Aig_Regular(pAnd), ppCofs[i+nSkip], ppCofs[i] );
        }
//Nwk_ObjAddFanin( Nwk_ManCreatePo(pAig), ppCofs[0] );

    // create choice node
    pAnd  = Aig_Regular((Aig_Obj_t *)pNode->pCopy); // repr
    pTemp = Aig_Regular(ppCofs[0]);    // new
    if ( Aig_ObjEquiv(pAig, pAnd) == NULL && Aig_ObjEquiv(pAig, pTemp) == NULL && !Aig_ObjCheckTfi(pAig, pTemp, pAnd) )
        pAig->pEquivs[pAnd->Id] = pTemp;
}

/**Function*************************************************************

  Synopsis    [Determines timing-critical edges of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Nwk_ManDelayTraceTCEdges( Nwk_Man_t * pNtk, Nwk_Obj_t * pNode, float tDelta, int fUseLutLib )
{
    int pPinPerm[32];
    float pPinDelays[32];
    If_LibLut_t * pLutLib = fUseLutLib? pNtk->pLutLib : NULL;
    Nwk_Obj_t * pFanin;
    unsigned uResult = 0;
    float tRequired, * pDelays;
    int k;
    tRequired = Nwk_ObjRequired(pNode);
    if ( pLutLib == NULL )
    {
        Nwk_ObjForEachFanin( pNode, pFanin, k )
            if ( tRequired < Nwk_ObjArrival(pFanin) + 1.0 + tDelta )
                uResult |= (1 << k);
    }
    else if ( !pLutLib->fVarPinDelays )
    {
        pDelays = pLutLib->pLutDelays[Nwk_ObjFaninNum(pNode)];
        Nwk_ObjForEachFanin( pNode, pFanin, k )
            if ( tRequired < Nwk_ObjArrival(pFanin) + pDelays[0] + tDelta )
                uResult |= (1 << k);
    }
    else
    {
        pDelays = pLutLib->pLutDelays[Nwk_ObjFaninNum(pNode)];
        Nwk_ManDelayTraceSortPins( pNode, pPinPerm, pPinDelays );
        Nwk_ObjForEachFanin( pNode, pFanin, k )
            if ( tRequired < Nwk_ObjArrival(Nwk_ObjFanin(pNode,pPinPerm[k])) + pDelays[k] + tDelta )
                uResult |= (1 << pPinPerm[k]);
    }
    return uResult;
}

/**Function*************************************************************

  Synopsis    [Adds choices to speed up the network by the given percentage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Nwk_ManSpeedup( Nwk_Man_t * pNtk, int fUseLutLib, int Percentage, int Degree, int fVerbose, int fVeryVerbose )
{
    Aig_Man_t * pAig, * pTemp;
    Vec_Ptr_t * vTimeCries, * vTimeFanins;
    Nwk_Obj_t * pNode, * pFanin, * pFanin2;
    Aig_Obj_t * pAnd;
    If_LibLut_t * pTempLib = pNtk->pLutLib;
    Tim_Man_t * pTempTim = NULL; 
    float tDelta, tArrival;
    int i, k, k2, Counter, CounterRes, nTimeCris;
    unsigned * puTCEdges;
    // perform delay trace
    if ( !fUseLutLib )
    {
        pNtk->pLutLib = NULL;
        if ( pNtk->pManTime )
        {
            pTempTim = pNtk->pManTime;
            pNtk->pManTime = Tim_ManDup( pTempTim, 1 );
        }
    }
    tArrival = Nwk_ManDelayTraceLut( pNtk );
    tDelta = fUseLutLib ? tArrival*Percentage/100.0 : 1.0;
    if ( fVerbose )
    {
        printf( "Max delay = %.2f. Delta = %.2f. ", tArrival, tDelta );
        printf( "Using %s model. ", fUseLutLib? "LUT library" : "unit-delay" );
        if ( fUseLutLib )
            printf( "Percentage = %d. ", Percentage );
        printf( "\n" );
    }
    // mark the timing critical nodes and edges
    puTCEdges = ABC_ALLOC( unsigned, Nwk_ManObjNumMax(pNtk) );
    memset( puTCEdges, 0, sizeof(unsigned) * Nwk_ManObjNumMax(pNtk) );
    Nwk_ManForEachNode( pNtk, pNode, i )
    {
        if ( Nwk_ObjSlack(pNode) >= tDelta )
            continue;
        puTCEdges[pNode->Id] = Nwk_ManDelayTraceTCEdges( pNtk, pNode, tDelta, fUseLutLib );
    }
    if ( fVerbose )
    {
        Counter = CounterRes = 0;
        Nwk_ManForEachNode( pNtk, pNode, i )
        {
            Nwk_ObjForEachFanin( pNode, pFanin, k )
                if ( !Nwk_ObjIsCi(pFanin) && Nwk_ObjSlack(pFanin) < tDelta )
                    Counter++;
            CounterRes += Aig_WordCountOnes( puTCEdges[pNode->Id] );
        }
        printf( "Edges: Total = %7d. 0-slack = %7d. Critical = %7d. Ratio = %4.2f\n", 
            Nwk_ManGetTotalFanins(pNtk), Counter, CounterRes, Counter? 1.0*CounterRes/Counter : 0.0 );
    }
    // start the resulting network
    pAig = Nwk_ManStrash( pNtk );
    pAig->pEquivs = ABC_ALLOC( Aig_Obj_t *, 3 * Aig_ManObjNumMax(pAig) );
    memset( pAig->pEquivs, 0, sizeof(Aig_Obj_t *) * 3 * Aig_ManObjNumMax(pAig) );

    // collect nodes to be used for resynthesis
    Counter = CounterRes = 0;
    vTimeCries = Vec_PtrAlloc( 16 );
    vTimeFanins = Vec_PtrAlloc( 16 );
    Nwk_ManForEachNode( pNtk, pNode, i )
    {
        if ( Nwk_ObjSlack(pNode) >= tDelta )
            continue;
        // count the number of non-PI timing-critical nodes
        nTimeCris = 0;
        Nwk_ObjForEachFanin( pNode, pFanin, k )
            if ( !Nwk_ObjIsCi(pFanin) && (puTCEdges[pNode->Id] & (1<<k)) )
                nTimeCris++;
        if ( !fVeryVerbose && nTimeCris == 0 )
            continue;
        Counter++;
        // count the total number of timing critical second-generation nodes
        Vec_PtrClear( vTimeCries );
        if ( nTimeCris )
        {
            Nwk_ObjForEachFanin( pNode, pFanin, k )
                if ( !Nwk_ObjIsCi(pFanin) && (puTCEdges[pNode->Id] & (1<<k)) )
                    Nwk_ObjForEachFanin( pFanin, pFanin2, k2 )
                        if ( puTCEdges[pFanin->Id] & (1<<k2) )
                            Vec_PtrPushUnique( vTimeCries, pFanin2 );
        }
//        if ( !fVeryVerbose && (Vec_PtrSize(vTimeCries) == 0 || Vec_PtrSize(vTimeCries) > Degree) )
        if ( (Vec_PtrSize(vTimeCries) == 0 || Vec_PtrSize(vTimeCries) > Degree) )
            continue;
        CounterRes++;
        // collect second generation nodes
        Vec_PtrClear( vTimeFanins );
        Nwk_ObjForEachFanin( pNode, pFanin, k )
        {
            if ( Nwk_ObjIsCi(pFanin) )
                Vec_PtrPushUnique( vTimeFanins, pFanin );
            else
                Nwk_ObjForEachFanin( pFanin, pFanin2, k2 )
                    Vec_PtrPushUnique( vTimeFanins, pFanin2 );                    
        }
        // print the results
        if ( fVeryVerbose )
        {
        printf( "%5d Node %5d : %d %2d %2d  ", Counter, pNode->Id, 
            nTimeCris, Vec_PtrSize(vTimeCries), Vec_PtrSize(vTimeFanins) );
        Nwk_ObjForEachFanin( pNode, pFanin, k )
            printf( "%d(%.2f)%s ", pFanin->Id, Nwk_ObjSlack(pFanin), (puTCEdges[pNode->Id] & (1<<k))? "*":"" );
        printf( "\n" );
        }
        // add the node to choices
        if ( Vec_PtrSize(vTimeCries) == 0 || Vec_PtrSize(vTimeCries) > Degree )
            continue;
        // order the fanins in the increasing order of criticalily
        if ( Vec_PtrSize(vTimeCries) > 1 )
        {
            pFanin = (Nwk_Obj_t *)Vec_PtrEntry( vTimeCries, 0 );
            pFanin2 = (Nwk_Obj_t *)Vec_PtrEntry( vTimeCries, 1 );
            if ( Nwk_ObjSlack(pFanin) < Nwk_ObjSlack(pFanin2) )
            {
                Vec_PtrWriteEntry( vTimeCries, 0, pFanin2 );
                Vec_PtrWriteEntry( vTimeCries, 1, pFanin );
            }
        }
        if ( Vec_PtrSize(vTimeCries) > 2 )
        {
            pFanin = (Nwk_Obj_t *)Vec_PtrEntry( vTimeCries, 1 );
            pFanin2 = (Nwk_Obj_t *)Vec_PtrEntry( vTimeCries, 2 );
            if ( Nwk_ObjSlack(pFanin) < Nwk_ObjSlack(pFanin2) )
            {
                Vec_PtrWriteEntry( vTimeCries, 1, pFanin2 );
                Vec_PtrWriteEntry( vTimeCries, 2, pFanin );
            }
            pFanin = (Nwk_Obj_t *)Vec_PtrEntry( vTimeCries, 0 );
            pFanin2 = (Nwk_Obj_t *)Vec_PtrEntry( vTimeCries, 1 );
            if ( Nwk_ObjSlack(pFanin) < Nwk_ObjSlack(pFanin2) )
            {
                Vec_PtrWriteEntry( vTimeCries, 0, pFanin2 );
                Vec_PtrWriteEntry( vTimeCries, 1, pFanin );
            }
        }
        // add choice
        Aig_ManSpeedupNode( pNtk, pAig, pNode, vTimeFanins, vTimeCries );
    }
    Vec_PtrFree( vTimeCries );
    Vec_PtrFree( vTimeFanins );
    ABC_FREE( puTCEdges );
    if ( fVerbose )
        printf( "Nodes: Total = %7d. 0-slack = %7d. Workable = %7d. Ratio = %4.2f\n", 
        Nwk_ManNodeNum(pNtk), Counter, CounterRes, Counter? 1.0*CounterRes/Counter : 0.0 ); 

    // remove invalid choice nodes
    Aig_ManForEachNode( pAig, pAnd, i )
        if ( Aig_ObjEquiv(pAig, pAnd) )
        {
            if ( Aig_ObjRefs(Aig_ObjEquiv(pAig, pAnd)) > 0 )
                pAig->pEquivs[pAnd->Id] = NULL;
        }

    // put back the library
    if ( !fUseLutLib )
        pNtk->pLutLib = pTempLib;
    if ( pTempTim )
    {
        Tim_ManStop( pNtk->pManTime );
        pNtk->pManTime = pTempTim;
    }

    // reconstruct the network
    pAig = Aig_ManDupDfs( pTemp = pAig );
    Aig_ManStop( pTemp );
    // reset levels
    Aig_ManChoiceLevel( pAig );
    return pAig;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

