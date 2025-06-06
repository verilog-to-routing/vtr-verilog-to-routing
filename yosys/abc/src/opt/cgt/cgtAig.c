/**CFile****************************************************************

  FileName    [cgtAig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Clock gating package.]

  Synopsis    [Creates AIG to compute clock-gating.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cgtAig.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cgtInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes transitive fanout cone of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ManDetectCandidates_rec( Aig_Man_t * pAig, Vec_Int_t * vUseful, Aig_Obj_t * pObj, int nLevelMax, Vec_Ptr_t * vCands )
{
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(pAig, pObj);
    if ( Aig_ObjIsNode(pObj) )
    {
        Cgt_ManDetectCandidates_rec( pAig, vUseful, Aig_ObjFanin0(pObj), nLevelMax, vCands );
        Cgt_ManDetectCandidates_rec( pAig, vUseful, Aig_ObjFanin1(pObj), nLevelMax, vCands );
    }
    if ( Aig_ObjLevel(pObj) <= nLevelMax && (vUseful == NULL || Vec_IntEntry(vUseful, Aig_ObjId(pObj))) )
        Vec_PtrPush( vCands, pObj );
}

/**Function*************************************************************

  Synopsis    [Computes transitive fanout cone of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ManDetectCandidates( Aig_Man_t * pAig, Vec_Int_t * vUseful, Aig_Obj_t * pObj, int nLevelMax, Vec_Ptr_t * vCands )
{
    Vec_PtrClear( vCands );
    if ( !Aig_ObjIsNode(pObj) )
        return;
    Aig_ManIncrementTravId( pAig );
    Cgt_ManDetectCandidates_rec( pAig, vUseful, pObj, nLevelMax, vCands );
}

/**Function*************************************************************

  Synopsis    [Computes transitive fanout cone of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ManDetectFanout_rec( Aig_Man_t * pAig, Aig_Obj_t * pObj, int nOdcMax, Vec_Ptr_t * vFanout )
{
    Aig_Obj_t * pFanout;
    int f, iFanout = -1;
    if ( Aig_ObjIsCo(pObj) || Aig_ObjLevel(pObj) > nOdcMax )
        return;
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(pAig, pObj);
    Vec_PtrPush( vFanout, pObj );
    Aig_ObjForEachFanout( pAig, pObj, pFanout, iFanout, f )
        Cgt_ManDetectFanout_rec( pAig, pFanout, nOdcMax, vFanout );
}

/**Function*************************************************************

  Synopsis    [Computes transitive fanout cone of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ManDetectFanout( Aig_Man_t * pAig, Aig_Obj_t * pObj, int nOdcMax, Vec_Ptr_t * vFanout )
{
    Aig_Obj_t * pFanout;
    int i, k, f, iFanout = -1;
    // collect visited nodes
    Vec_PtrClear( vFanout );
    Aig_ManIncrementTravId( pAig );
    Cgt_ManDetectFanout_rec( pAig, pObj, nOdcMax, vFanout );
    // remove those nodes whose fanout is included
    k = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vFanout, pObj, i )
    {
        // go through the fanouts of this node
        Aig_ObjForEachFanout( pAig, pObj, pFanout, iFanout, f )
            if ( !Aig_ObjIsTravIdCurrent(pAig, pFanout) )
                break;
        if ( f == Aig_ObjRefs(pObj) ) // all fanouts are included
            continue;
        Vec_PtrWriteEntry( vFanout, k++, pObj );
    }
    Vec_PtrShrink( vFanout, k );
    Vec_PtrSort( vFanout, (int (*)(const void *, const void *))Aig_ObjCompareIdIncrease );
    assert( Vec_PtrSize(vFanout) > 0 );
}

/**Function*************************************************************

  Synopsis    [Computes visited nodes in the topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ManCollectVisited_rec( Aig_Man_t * pAig, Aig_Obj_t * pObj, Vec_Ptr_t * vVisited )
{
    if ( Aig_ObjIsCi(pObj) )
        return;
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(pAig, pObj);
    assert( Aig_ObjIsNode(pObj) );
    Cgt_ManCollectVisited_rec( pAig, Aig_ObjFanin0(pObj), vVisited );
    Cgt_ManCollectVisited_rec( pAig, Aig_ObjFanin1(pObj), vVisited );
    Vec_PtrPush( vVisited, pObj );
}

/**Function*************************************************************

  Synopsis    [Computes visited nodes in the topological order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ManCollectVisited( Aig_Man_t * pAig, Vec_Ptr_t * vFanout, Vec_Ptr_t * vVisited )
{
    Aig_Obj_t * pObj;
    int i;
    Vec_PtrClear( vVisited );
    Aig_ManIncrementTravId( pAig );
    Vec_PtrForEachEntry( Aig_Obj_t *, vFanout, pObj, i )
        Cgt_ManCollectVisited_rec( pAig, pObj, vVisited );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Aig_Obj_t *  Aig_ObjChild0CopyVec( Vec_Ptr_t * vCopy, Aig_Obj_t * pObj )  
{ return Aig_NotCond((Aig_Obj_t *)Vec_PtrEntry(vCopy, Aig_ObjFaninId0(pObj)), Aig_ObjFaninC0(pObj));  }
static inline Aig_Obj_t *  Aig_ObjChild1CopyVec( Vec_Ptr_t * vCopy, Aig_Obj_t * pObj )  
{ return Aig_NotCond((Aig_Obj_t *)Vec_PtrEntry(vCopy, Aig_ObjFaninId1(pObj)), Aig_ObjFaninC1(pObj));  }

/**Function*************************************************************

  Synopsis    [Derives miter for clock-gating.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Cgt_ManConstructCareCondition( Cgt_Man_t * p, Aig_Man_t * pNew, Aig_Obj_t * pObjLo, Vec_Ptr_t * vCopy0, Vec_Ptr_t * vCopy1 )
{
    Aig_Obj_t * pMiter, * pObj, * pTemp;
    int i;
    assert( Aig_ObjIsCi(pObjLo) );
    // detect nodes and their cone
    Cgt_ManDetectFanout( p->pAig, pObjLo, p->pPars->nOdcMax, p->vFanout );
    Cgt_ManCollectVisited( p->pAig, p->vFanout, p->vVisited );
    // add new variables if the observability condition depends on PI variables
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vVisited, pObj, i )
    {
        assert( Aig_ObjIsNode(pObj) );
        if ( Saig_ObjIsPi(p->pAig, Aig_ObjFanin0(pObj)) && Vec_PtrEntry(vCopy0, Aig_ObjFaninId0(pObj)) == NULL )
        {
            pTemp = Aig_ObjCreateCi( pNew );
            Vec_PtrWriteEntry( vCopy0, Aig_ObjFaninId0(pObj), pTemp );
            Vec_PtrWriteEntry( vCopy1, Aig_ObjFaninId0(pObj), pTemp );
        }
        if ( Saig_ObjIsPi(p->pAig, Aig_ObjFanin1(pObj)) && Vec_PtrEntry(vCopy0, Aig_ObjFaninId1(pObj)) == NULL )
        {
            pTemp = Aig_ObjCreateCi( pNew );
            Vec_PtrWriteEntry( vCopy0, Aig_ObjFaninId1(pObj), pTemp );
            Vec_PtrWriteEntry( vCopy1, Aig_ObjFaninId1(pObj), pTemp );
        }
    }
    // construct AIGs for the nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vVisited, pObj, i )
    {
        pTemp = Aig_And( pNew, Aig_ObjChild0CopyVec(vCopy0, pObj), Aig_ObjChild1CopyVec(vCopy0, pObj) );
        Vec_PtrWriteEntry( vCopy0, Aig_ObjId(pObj), pTemp );
        pTemp = Aig_And( pNew, Aig_ObjChild0CopyVec(vCopy1, pObj), Aig_ObjChild1CopyVec(vCopy1, pObj) );
        Vec_PtrWriteEntry( vCopy1, Aig_ObjId(pObj), pTemp );
    }
    // construct the care miter
    pMiter = Aig_ManConst0( pNew );
    Vec_PtrForEachEntry( Aig_Obj_t *, p->vFanout, pObj, i )
    {
        pTemp = Aig_Exor( pNew, (Aig_Obj_t *)Vec_PtrEntry(vCopy0, Aig_ObjId(pObj)), (Aig_Obj_t *)Vec_PtrEntry(vCopy1, Aig_ObjId(pObj)) );
        pMiter = Aig_Or( pNew, pMiter, pTemp );
    }
    return pMiter;
}

/**Function*************************************************************

  Synopsis    [Derives AIG for clock-gating.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Cgt_ManDeriveAigForGating( Cgt_Man_t * p )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo, * pCare, * pMiter;
    Vec_Ptr_t * vCopy0, * vCopy1;
    int i;
    assert( Aig_ManRegNum(p->pAig) );
    pNew = Aig_ManStart( Aig_ManObjNumMax(p->pAig) );
    pNew->pName = Abc_UtilStrsav( "CG_miter" );
    // build the first frame
    Aig_ManConst1(p->pAig)->pData = Aig_ManConst1(pNew);
    Aig_ManForEachCi( p->pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    Aig_ManForEachNode( p->pAig, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
//    Saig_ManForEachPo( p->pAig, pObj, i )
//        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    if ( p->pPars->nOdcMax > 0 )
    {
        // create storage for observability conditions
        vCopy0 = Vec_PtrStart( Aig_ManObjNumMax(p->pAig) );
        vCopy1 = Vec_PtrStart( Aig_ManObjNumMax(p->pAig) );
        // initialize register outputs
        Saig_ManForEachLiLo( p->pAig, pObjLi, pObjLo, i )
        {
            Vec_PtrWriteEntry( vCopy0, Aig_ObjId(pObjLo), Aig_ObjChild0Copy(pObjLi) );
            Vec_PtrWriteEntry( vCopy1, Aig_ObjId(pObjLo), Aig_ObjChild0Copy(pObjLi) );
        }
        // compute observability condition for each latch output
        Saig_ManForEachLiLo( p->pAig, pObjLi, pObjLo, i )
        {
            // set the constants
            Vec_PtrWriteEntry( vCopy0, Aig_ObjId(pObjLo), Aig_ManConst0(pNew) );
            Vec_PtrWriteEntry( vCopy1, Aig_ObjId(pObjLo), Aig_ManConst1(pNew) );
            // compute condition
            pCare = Cgt_ManConstructCareCondition( p, pNew, pObjLo, vCopy0, vCopy1 );
            // restore the values
            Vec_PtrWriteEntry( vCopy0, Aig_ObjId(pObjLo), Aig_ObjChild0Copy(pObjLi) );
            Vec_PtrWriteEntry( vCopy1, Aig_ObjId(pObjLo), Aig_ObjChild0Copy(pObjLi) );
            // compute the miter
            pMiter = Aig_Exor( pNew, (Aig_Obj_t *)pObjLo->pData, Aig_ObjChild0Copy(pObjLi) );
            pMiter = Aig_And( pNew, pMiter, pCare );
            pObjLi->pData = Aig_ObjCreateCo( pNew, pMiter );
        }
        Vec_PtrFree( vCopy0 );
        Vec_PtrFree( vCopy1 );
    }
    else
    {
        // construct clock-gating miters for each register input
        Saig_ManForEachLiLo( p->pAig, pObjLi, pObjLo, i )
        {
            pMiter = Aig_Exor( pNew, (Aig_Obj_t *)pObjLo->pData, Aig_ObjChild0Copy(pObjLi) );
            pObjLi->pData = Aig_ObjCreateCo( pNew, pMiter );
        }
    }
    Aig_ManCleanup( pNew );
    Aig_ManSetCioIds( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Adds relevant constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Cgt_ManConstructCare_rec( Aig_Man_t * pCare, Aig_Obj_t * pObj, Aig_Man_t * pNew )
{
    Aig_Obj_t * pObj0, * pObj1;
    if ( Aig_ObjIsTravIdCurrent( pCare, pObj ) )
        return (Aig_Obj_t *)pObj->pData;
    Aig_ObjSetTravIdCurrent( pCare, pObj );
    if ( Aig_ObjIsCi(pObj) )
        return (Aig_Obj_t *)(pObj->pData = NULL);
    pObj0 = Cgt_ManConstructCare_rec( pCare, Aig_ObjFanin0(pObj), pNew );
    if ( pObj0 == NULL )
        return (Aig_Obj_t *)(pObj->pData = NULL);
    pObj1 = Cgt_ManConstructCare_rec( pCare, Aig_ObjFanin1(pObj), pNew );
    if ( pObj1 == NULL )
        return (Aig_Obj_t *)(pObj->pData = NULL);
    pObj0 = Aig_NotCond( pObj0, Aig_ObjFaninC0(pObj) );
    pObj1 = Aig_NotCond( pObj1, Aig_ObjFaninC1(pObj) );
    return (Aig_Obj_t *)(pObj->pData = Aig_And( pNew, pObj0, pObj1 ));
}

/**Function*************************************************************

  Synopsis    [Builds constraints belonging to the given partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cgt_ManConstructCare( Aig_Man_t * pNew, Aig_Man_t * pCare, Vec_Vec_t * vSuppsInv, Vec_Ptr_t * vLeaves )
{
    Vec_Int_t * vOuts;
    Aig_Obj_t * pLeaf, * pPi, * pPo, * pObjAig;
    int i, k, iOut;
    // go through the PIs of the partition
    // label the corresponding PIs of the care set
    Aig_ManIncrementTravId( pCare );
    Vec_PtrForEachEntry( Aig_Obj_t *, vLeaves, pLeaf, i )
    {
        pPi = Aig_ManCi( pCare, Aig_ObjCioId(pLeaf) );
        Aig_ObjSetTravIdCurrent( pCare, pPi );
        pPi->pData = pLeaf->pData;
    }
    // construct the constraints
    Vec_PtrForEachEntry( Aig_Obj_t *, vLeaves, pLeaf, i )
    {
        vOuts = Vec_VecEntryInt( vSuppsInv, Aig_ObjCioId(pLeaf) );
        Vec_IntForEachEntry( vOuts, iOut, k )
        {
            pPo = Aig_ManCo( pCare, iOut );
            if ( Aig_ObjIsTravIdCurrent( pCare, pPo ) )
                continue;
            Aig_ObjSetTravIdCurrent( pCare, pPo );
            if ( Aig_ObjFanin0(pPo) == Aig_ManConst1(pCare) )
                continue;
            pObjAig = Cgt_ManConstructCare_rec( pCare, Aig_ObjFanin0(pPo), pNew );
            if ( pObjAig == NULL )
                continue;
            pObjAig = Aig_NotCond( pObjAig, Aig_ObjFaninC0(pPo) );
            Aig_ObjCreateCo( pNew, pObjAig );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG recursively.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Cgt_ManDupPartition_rec( Aig_Man_t * pNew, Aig_Man_t * pAig, Aig_Obj_t * pObj, Vec_Ptr_t * vLeaves )
{
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return (Aig_Obj_t *)pObj->pData;
    Aig_ObjSetTravIdCurrent(pAig, pObj);
    if ( Aig_ObjIsCi(pObj) )
    {
        pObj->pData = Aig_ObjCreateCi( pNew );
        Vec_PtrPush( vLeaves, pObj );
        return (Aig_Obj_t *)pObj->pData;
    }
    Cgt_ManDupPartition_rec( pNew, pAig, Aig_ObjFanin0(pObj), vLeaves );
    Cgt_ManDupPartition_rec( pNew, pAig, Aig_ObjFanin1(pObj), vLeaves );
    return (Aig_Obj_t *)(pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) ));
}

/**Function*************************************************************

  Synopsis    [Duplicates register outputs starting from the given one.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Cgt_ManDupPartition( Aig_Man_t * pFrame, int nVarsMin, int nFlopsMin, int iStart, Aig_Man_t * pCare, Vec_Vec_t * vSuppsInv, int * pnOutputs )
{
    Vec_Ptr_t * vRoots, * vLeaves, * vPos;
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    int i;
    assert( Aig_ManRegNum(pFrame) == 0 );
    vRoots = Vec_PtrAlloc( 100 );
    vLeaves = Vec_PtrAlloc( 100 ); 
    vPos = Vec_PtrAlloc( 100 );
    pNew = Aig_ManStart( nVarsMin );
    pNew->pName = Abc_UtilStrsav( "partition" );
    Aig_ManIncrementTravId( pFrame );
    Aig_ManConst1(pFrame)->pData = Aig_ManConst1(pNew);
    Aig_ObjSetTravIdCurrent( pFrame, Aig_ManConst1(pFrame) );
    for ( i = iStart; i < iStart + nFlopsMin && i < Aig_ManCoNum(pFrame); i++ )
    {
        pObj = Aig_ManCo( pFrame, i );
        Cgt_ManDupPartition_rec( pNew, pFrame, Aig_ObjFanin0(pObj), vLeaves );
        Vec_PtrPush( vRoots, Aig_ObjChild0Copy(pObj) );
        Vec_PtrPush( vPos, pObj );
    }
    for ( ; Aig_ManObjNum(pNew) < nVarsMin && i < Aig_ManCoNum(pFrame); i++ )
    {
        pObj = Aig_ManCo( pFrame, i );
        Cgt_ManDupPartition_rec( pNew, pFrame, Aig_ObjFanin0(pObj), vLeaves );
        Vec_PtrPush( vRoots, Aig_ObjChild0Copy(pObj) );
        Vec_PtrPush( vPos, pObj );
    }
    assert( nFlopsMin >= Vec_PtrSize(vRoots) || Vec_PtrSize(vRoots) >= nFlopsMin );
    // create constaints
    if ( pCare )
        Cgt_ManConstructCare( pNew, pCare, vSuppsInv, vLeaves );
    // create POs
    Vec_PtrForEachEntry( Aig_Obj_t *, vPos, pObj, i )
        pObj->pData = (Aig_Obj_t *)Aig_ObjCreateCo( pNew, (Aig_Obj_t *)Vec_PtrEntry(vRoots, i) );
    if ( pnOutputs != NULL )
        *pnOutputs = Vec_PtrSize( vPos );
    Vec_PtrFree( vRoots );
    Vec_PtrFree( vLeaves );
    Vec_PtrFree( vPos );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Implements one clock-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Cgt_ManBuildClockGate( Aig_Man_t * pNew, Vec_Ptr_t * vGates )
{
    Aig_Obj_t * pGate, * pTotal;
    int i;
    assert( Vec_PtrSize(vGates) > 0 );
    pTotal = Aig_ManConst0(pNew);
    Vec_PtrForEachEntry( Aig_Obj_t *, vGates, pGate, i )
    {
        if ( Aig_Regular(pGate)->pNext )
            pGate = Aig_NotCond( Aig_Regular(pGate)->pNext, Aig_IsComplement(pGate) );
        else
            pGate = Aig_NotCond( (Aig_Obj_t *)Aig_Regular(pGate)->pData, Aig_IsComplement(pGate) );
        pTotal = Aig_Or( pNew, pTotal, pGate );
    }
    return pTotal;
}

/**Function*************************************************************

  Synopsis    [Derives AIG after clock-gating.]

  Description [The array contains, for each flop, its gate if present.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Cgt_ManDeriveGatedAig( Aig_Man_t * pAig, Vec_Vec_t * vGates, int fReduce, int * pnUsedNodes )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj, * pObjNew, * pObjLi, * pObjLo, * pGateNew;
    Vec_Ptr_t * vOne;
    int i, k;
    Aig_ManCleanNext( pAig );
    // label nodes
    Vec_VecForEachEntry( Aig_Obj_t *, vGates, pObj, i, k )
    {
        if ( Aig_IsComplement(pObj) )
            Aig_Regular(pObj)->fMarkB = 1;
        else
            Aig_Regular(pObj)->fMarkA = 1;
    }
    // construct AIG
    assert( Aig_ManRegNum(pAig) );
    pNew = Aig_ManStart( Aig_ManObjNumMax(pAig) );
    pNew->pName = Abc_UtilStrsav( pAig->pName );
    pNew->pSpec = Abc_UtilStrsav( pAig->pSpec );
    Aig_ManConst1(pAig)->pData = Aig_ManConst1(pNew);
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    if ( fReduce )
    {
        Aig_ManForEachNode( pAig, pObj, i )
        {
            assert( !(pObj->fMarkA && pObj->fMarkB) );
            pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
            if ( pObj->fMarkA )
            {
                pObj->pNext = (Aig_Obj_t *)pObj->pData;
                pObj->pData = Aig_ManConst0(pNew);
            }
            else if ( pObj->fMarkB )
            {
                pObj->pNext = (Aig_Obj_t *)pObj->pData;
                pObj->pData = Aig_ManConst1(pNew);
            }
        }
    }
    else
    {
        Aig_ManForEachNode( pAig, pObj, i )
            pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    }
    if ( pnUsedNodes != NULL )
        *pnUsedNodes = Aig_ManNodeNum(pNew);
    Saig_ManForEachPo( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    Saig_ManForEachLiLo( pAig, pObjLi, pObjLo, i )
    {
        vOne = Vec_VecEntry( vGates, i );
        if ( Vec_PtrSize(vOne) == 0 )
            pObjNew = Aig_ObjChild0Copy(pObjLi);
        else
        {
//            pGateNew = Aig_NotCond( Aig_Regular(pGate)->pData, Aig_IsComplement(pGate) );
            pGateNew = Cgt_ManBuildClockGate( pNew, vOne );
            pObjNew = Aig_Mux( pNew, pGateNew, (Aig_Obj_t *)pObjLo->pData, Aig_ObjChild0Copy(pObjLi) );
        }
        pObjLi->pData = Aig_ObjCreateCo( pNew, pObjNew );
    }
    Aig_ManCleanup( pNew );
    Aig_ManSetRegNum( pNew, Aig_ManRegNum(pAig) );
    // unlabel nodes
    Aig_ManCleanMarkAB( pAig );
    Aig_ManCleanNext( pAig );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

