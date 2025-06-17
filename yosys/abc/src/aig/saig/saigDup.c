/**CFile****************************************************************

  FileName    [saigDup.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Various duplication procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigDup.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Duplicates while ORing the POs of sequential circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManDupOrpos( Aig_Man_t * pAig )
{
    Aig_Man_t * pAigNew;
    Aig_Obj_t * pObj, * pMiter;
    int i;
    if ( pAig->nConstrs > 0 )
    {
        printf( "The AIG manager should have no constraints.\n" );
        return NULL;
    }
    // start the new manager
    pAigNew = Aig_ManStart( Aig_ManNodeNum(pAig) );
    pAigNew->pName = Abc_UtilStrsav( pAig->pName );
    pAigNew->nConstrs = pAig->nConstrs;
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pAigNew );
    // create variables for PIs
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pAigNew );
    // add internal nodes of this frame
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->pData = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // create PO of the circuit
    pMiter = Aig_ManConst0( pAigNew );
    Saig_ManForEachPo( pAig, pObj, i )
        pMiter = Aig_Or( pAigNew, pMiter, Aig_ObjChild0Copy(pObj) );
    Aig_ObjCreateCo( pAigNew, pMiter );
    // transfer to register outputs
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );
    Aig_ManCleanup( pAigNew );
    Aig_ManSetRegNum( pAigNew, Aig_ManRegNum(pAig) );
    return pAigNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates while ORing the POs of sequential circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManCreateEquivMiter( Aig_Man_t * pAig, Vec_Int_t * vPairs, int fAddOuts )
{
    Aig_Man_t * pAigNew;
    Aig_Obj_t * pObj, * pObj2, * pMiter;
    int i;
    if ( pAig->nConstrs > 0 )
    {
        printf( "The AIG manager should have no constraints.\n" );
        return NULL;
    }
    // start the new manager
    pAigNew = Aig_ManStart( Aig_ManNodeNum(pAig) );
    pAigNew->pName = Abc_UtilStrsav( pAig->pName );
    pAigNew->nConstrs = pAig->nConstrs;
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pAigNew );
    // create variables for PIs
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pAigNew );
    // add internal nodes of this frame
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->pData = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // create POs
    assert( Vec_IntSize(vPairs) % 2 == 0 );
    Aig_ManForEachObjVec( vPairs, pAig, pObj, i )
    {
        pObj2  = Aig_ManObj( pAig, Vec_IntEntry(vPairs, ++i) );
        pMiter = Aig_Exor( pAigNew, (Aig_Obj_t *)pObj->pData, (Aig_Obj_t *)pObj2->pData );
        pMiter = Aig_NotCond( pMiter, pObj->fPhase ^ pObj2->fPhase );
        Aig_ObjCreateCo( pAigNew, pMiter );
    }
    // transfer to register outputs
    if ( fAddOuts )
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );
    Aig_ManCleanup( pAigNew );
    if ( fAddOuts )
    Aig_ManSetRegNum( pAigNew, Aig_ManRegNum(pAig) );
    return pAigNew;
}

/**Function*************************************************************

  Synopsis    [Trims the model by removing PIs without fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManTrimPis( Aig_Man_t * p )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    int i, fAllPisHaveNoRefs;
    // check the refs of PIs    
    fAllPisHaveNoRefs = 1;
    Saig_ManForEachPi( p, pObj, i )
        if ( pObj->nRefs )
            fAllPisHaveNoRefs = 0;
    // start the new manager
    pNew = Aig_ManStart( Aig_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->nConstrs = p->nConstrs;
    // start mapping of the CI numbers
    pNew->vCiNumsOrig = Vec_IntAlloc( Aig_ManCiNum(p) );
    // map const and primary inputs
    Aig_ManCleanData( p );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    Aig_ManForEachCi( p, pObj, i )
        if ( fAllPisHaveNoRefs || pObj->nRefs || Saig_ObjIsLo(p, pObj) )
        {
            pObj->pData = Aig_ObjCreateCi( pNew );
            Vec_IntPush( pNew->vCiNumsOrig, Vec_IntEntry(p->vCiNumsOrig, i) );
        }
    Aig_ManForEachNode( p, pObj, i )
        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    Aig_ManForEachCo( p, pObj, i )
        pObj->pData = Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    Aig_ManSetRegNum( pNew, Aig_ManRegNum(p) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG manager recursively.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Saig_ManAbstractionDfs_rec( Aig_Man_t * pNew, Aig_Obj_t * pObj )
{
    if ( pObj->pData )
        return (Aig_Obj_t *)pObj->pData;
    Saig_ManAbstractionDfs_rec( pNew, Aig_ObjFanin0(pObj) );
    Saig_ManAbstractionDfs_rec( pNew, Aig_ObjFanin1(pObj) );
    return (Aig_Obj_t *)(pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) ));
}

/**Function*************************************************************

  Synopsis    [Performs abstraction of the AIG to preserve the included flops.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManDupAbstraction( Aig_Man_t * p, Vec_Int_t * vFlops )
{ 
    Aig_Man_t * pNew;//, * pTemp;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i, Entry;
    Aig_ManCleanData( p );
    // start the new manager
    pNew = Aig_ManStart( 5000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    // map the constant node
    Aig_ManConst1(p)->pData = Aig_ManConst1( pNew );
    // label included flops
    Vec_IntForEachEntry( vFlops, Entry, i )
    {
        pObjLi = Saig_ManLi( p, Entry );
        assert( pObjLi->fMarkA == 0 );
        pObjLi->fMarkA = 1;
        pObjLo = Saig_ManLo( p, Entry );
        assert( pObjLo->fMarkA == 0 );
        pObjLo->fMarkA = 1;
    }
    // create variables for PIs
    assert( p->vCiNumsOrig == NULL );
    pNew->vCiNumsOrig = Vec_IntAlloc( Aig_ManCiNum(p) );
    Aig_ManForEachCi( p, pObj, i )
        if ( !pObj->fMarkA )
        {
            pObj->pData = Aig_ObjCreateCi( pNew );
            Vec_IntPush( pNew->vCiNumsOrig, i );
        }
    // create variables for LOs
    Aig_ManForEachCi( p, pObj, i )
        if ( pObj->fMarkA )
        {
            pObj->fMarkA = 0;
            pObj->pData = Aig_ObjCreateCi( pNew );
            Vec_IntPush( pNew->vCiNumsOrig, i );
        }
    // add internal nodes 
//    Aig_ManForEachNode( p, pObj, i )
//        pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // create POs
    Saig_ManForEachPo( p, pObj, i )
    {
        Saig_ManAbstractionDfs_rec( pNew, Aig_ObjFanin0(pObj) );
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    }
    // create LIs
    Aig_ManForEachCo( p, pObj, i )
        if ( pObj->fMarkA )
        {
            pObj->fMarkA = 0;
            Saig_ManAbstractionDfs_rec( pNew, Aig_ObjFanin0(pObj) );
            Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
        }
    Aig_ManSetRegNum( pNew, Vec_IntSize(vFlops) );
    Aig_ManSeqCleanup( pNew );
    // remove PIs without fanout
//    pNew = Saig_ManTrimPis( pTemp = pNew );
//    Aig_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Resimulates the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManVerifyCex( Aig_Man_t * pAig, Abc_Cex_t * p )
{
    Aig_Obj_t * pObj, * pObjRi, * pObjRo;
    int RetValue, i, k, iBit = 0;
    Aig_ManCleanMarkB(pAig);
    Aig_ManConst1(pAig)->fMarkB = 1;
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->fMarkB = Abc_InfoHasBit(p->pData, iBit++);
    for ( i = 0; i <= p->iFrame; i++ )
    {
        Saig_ManForEachPi( pAig, pObj, k )
            pObj->fMarkB = Abc_InfoHasBit(p->pData, iBit++);
        Aig_ManForEachNode( pAig, pObj, k )
            pObj->fMarkB = (Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj)) & 
                           (Aig_ObjFanin1(pObj)->fMarkB ^ Aig_ObjFaninC1(pObj));
        Aig_ManForEachCo( pAig, pObj, k )
            pObj->fMarkB = Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj);
        if ( i == p->iFrame )
            break;
        Saig_ManForEachLiLo( pAig, pObjRi, pObjRo, k )
            pObjRo->fMarkB = pObjRi->fMarkB;
    }
    assert( iBit == p->nBits );
    RetValue = Aig_ManCo(pAig, p->iPo)->fMarkB;
    Aig_ManCleanMarkB(pAig);
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Resimulates the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManVerifyCexNoClear( Aig_Man_t * pAig, Abc_Cex_t * p )
{
    Aig_Obj_t * pObj, * pObjRi, * pObjRo;
    int RetValue, i, k, iBit = 0;
    Aig_ManCleanMarkB(pAig);
    Aig_ManConst1(pAig)->fMarkB = 1;
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->fMarkB = Abc_InfoHasBit(p->pData, iBit++);
    for ( i = 0; i <= p->iFrame; i++ )
    {
        Saig_ManForEachPi( pAig, pObj, k )
            pObj->fMarkB = Abc_InfoHasBit(p->pData, iBit++);
        Aig_ManForEachNode( pAig, pObj, k )
            pObj->fMarkB = (Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj)) & 
                           (Aig_ObjFanin1(pObj)->fMarkB ^ Aig_ObjFaninC1(pObj));
        Aig_ManForEachCo( pAig, pObj, k )
            pObj->fMarkB = Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj);
        if ( i == p->iFrame )
            break;
        Saig_ManForEachLiLo( pAig, pObjRi, pObjRo, k )
            pObjRo->fMarkB = pObjRi->fMarkB;
    }
    assert( iBit == p->nBits );
    RetValue = Aig_ManCo(pAig, p->iPo)->fMarkB;
    //Aig_ManCleanMarkB(pAig);
    return RetValue;
}
Vec_Int_t * Saig_ManReturnFailingState( Aig_Man_t * pAig, Abc_Cex_t * p, int fNextOne )
{
    Aig_Obj_t * pObj;
    Vec_Int_t * vState;
    int i, RetValue = Saig_ManVerifyCexNoClear( pAig, p );
    if ( RetValue == 0 )
    {
        Aig_ManCleanMarkB(pAig);
        printf( "CEX does fail the given sequential miter.\n" );
        return NULL;
    }
    vState = Vec_IntAlloc( Aig_ManRegNum(pAig) );
    if ( fNextOne )
    {
        Saig_ManForEachLi( pAig, pObj, i )
            Vec_IntPush( vState, pObj->fMarkB );
    }
    else
    {
        Saig_ManForEachLo( pAig, pObj, i )
            Vec_IntPush( vState, pObj->fMarkB );
    }
    Aig_ManCleanMarkB(pAig);
    return vState;
}

/**Function*************************************************************

  Synopsis    [Resimulates the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Saig_ManExtendCex( Aig_Man_t * pAig, Abc_Cex_t * p )
{
    Abc_Cex_t * pNew;
    Aig_Obj_t * pObj, * pObjRi, * pObjRo;
    int RetValue, i, k, iBit = 0;
    // create new counter-example
    pNew = Abc_CexAlloc( 0, Aig_ManCiNum(pAig), p->iFrame + 1 );
    pNew->iPo = p->iPo;
    pNew->iFrame = p->iFrame;
    // simulate the AIG
    Aig_ManCleanMarkB(pAig);
    Aig_ManConst1(pAig)->fMarkB = 1;
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->fMarkB = Abc_InfoHasBit(p->pData, iBit++);
    for ( i = 0; i <= p->iFrame; i++ )
    {
        Saig_ManForEachPi( pAig, pObj, k )
            pObj->fMarkB = Abc_InfoHasBit(p->pData, iBit++);
        ///////// write PI+LO values ////////////
        Aig_ManForEachCi( pAig, pObj, k )
            if ( pObj->fMarkB )
                Abc_InfoSetBit(pNew->pData, Aig_ManCiNum(pAig)*i + k);
        /////////////////////////////////////////
        Aig_ManForEachNode( pAig, pObj, k )
            pObj->fMarkB = (Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj)) & 
                           (Aig_ObjFanin1(pObj)->fMarkB ^ Aig_ObjFaninC1(pObj));
        Aig_ManForEachCo( pAig, pObj, k )
            pObj->fMarkB = Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj);
        if ( i == p->iFrame )
            break;
        Saig_ManForEachLiLo( pAig, pObjRi, pObjRo, k )
            pObjRo->fMarkB = pObjRi->fMarkB;
    }
    assert( iBit == p->nBits );
    RetValue = Aig_ManCo(pAig, p->iPo)->fMarkB;
    Aig_ManCleanMarkB(pAig);
    if ( RetValue == 0 )
        printf( "Saig_ManExtendCex(): The counter-example is invalid!!!\n" );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Resimulates the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManFindFailedPoCex( Aig_Man_t * pAig, Abc_Cex_t * p )
{
    Aig_Obj_t * pObj, * pObjRi, * pObjRo;
    int RetValue, i, k, iBit = 0;
    Aig_ManCleanMarkB(pAig);
    Aig_ManConst1(pAig)->fMarkB = 1;
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->fMarkB = Abc_InfoHasBit(p->pData, iBit++);
    for ( i = 0; i <= p->iFrame; i++ )
    {
        Saig_ManForEachPi( pAig, pObj, k )
            pObj->fMarkB = Abc_InfoHasBit(p->pData, iBit++);
        Aig_ManForEachNode( pAig, pObj, k )
            pObj->fMarkB = (Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj)) & 
                           (Aig_ObjFanin1(pObj)->fMarkB ^ Aig_ObjFaninC1(pObj));
        Aig_ManForEachCo( pAig, pObj, k )
            pObj->fMarkB = Aig_ObjFanin0(pObj)->fMarkB ^ Aig_ObjFaninC0(pObj);
        if ( i == p->iFrame )
            break;
        Saig_ManForEachLiLo( pAig, pObjRi, pObjRo, k )
            pObjRo->fMarkB = pObjRi->fMarkB;
    }
    assert( iBit == p->nBits );
    // remember the number of failed output
    RetValue = -1;
    Saig_ManForEachPo( pAig, pObj, i )
        if ( pObj->fMarkB )
        {
            RetValue = i;
            break;
        }
    Aig_ManCleanMarkB(pAig);
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Duplicates while ORing the POs of sequential circuit.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManDupWithPhase( Aig_Man_t * pAig, Vec_Int_t * vInit )
{
    Aig_Man_t * pAigNew;
    Aig_Obj_t * pObj;
    int i;
    assert( Aig_ManRegNum(pAig) <= Vec_IntSize(vInit) );
    // start the new manager
    pAigNew = Aig_ManStart( Aig_ManNodeNum(pAig) );
    pAigNew->pName = Abc_UtilStrsav( pAig->pName );
    pAigNew->nConstrs = pAig->nConstrs;
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pAigNew );
    // create variables for PIs
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pAigNew );
    // update the flop variables
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->pData = Aig_NotCond( (Aig_Obj_t *)pObj->pData, Vec_IntEntry(vInit, i) );
    // add internal nodes of this frame
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->pData = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // transfer to register outputs
    Saig_ManForEachPo( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );
    // update the flop variables
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_NotCond(Aig_ObjChild0Copy(pObj), Vec_IntEntry(vInit, i)) );
    // finalize
    Aig_ManCleanup( pAigNew );
    Aig_ManSetRegNum( pAigNew, Aig_ManRegNum(pAig) );
    return pAigNew;
}

/**Function*************************************************************

  Synopsis    [Copy an AIG structure related to the selected POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManDupCones_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vNodes, Vec_Ptr_t * vRoots )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ObjIsNode(pObj) )
    {
        Saig_ManDupCones_rec( p, Aig_ObjFanin0(pObj), vLeaves, vNodes, vRoots );
        Saig_ManDupCones_rec( p, Aig_ObjFanin1(pObj), vLeaves, vNodes, vRoots );
        Vec_PtrPush( vNodes, pObj );
    }
    else if ( Aig_ObjIsCo(pObj) )
        Saig_ManDupCones_rec( p, Aig_ObjFanin0(pObj), vLeaves, vNodes, vRoots );
    else if ( Saig_ObjIsLo(p, pObj) )
        Vec_PtrPush( vRoots, Saig_ObjLoToLi(p, pObj) );
    else if ( Saig_ObjIsPi(p, pObj) )
        Vec_PtrPush( vLeaves, pObj );
    else assert( 0 );
}
Aig_Man_t * Saig_ManDupCones( Aig_Man_t * pAig, int * pPos, int nPos )
{
    Aig_Man_t * pAigNew;
    Vec_Ptr_t * vLeaves, * vNodes, * vRoots;
    Aig_Obj_t * pObj;
    int i;

    // collect initial POs
    vLeaves = Vec_PtrAlloc( 100 );
    vNodes = Vec_PtrAlloc( 100 );
    vRoots = Vec_PtrAlloc( 100 );
    for ( i = 0; i < nPos; i++ )
        Vec_PtrPush( vRoots, Aig_ManCo(pAig, pPos[i]) );

    // mark internal nodes
    Aig_ManIncrementTravId( pAig );
    Aig_ObjSetTravIdCurrent( pAig, Aig_ManConst1(pAig) );
    Vec_PtrForEachEntry( Aig_Obj_t *, vRoots, pObj, i )
        Saig_ManDupCones_rec( pAig, pObj, vLeaves, vNodes, vRoots );

    // start the new manager
    pAigNew = Aig_ManStart( Vec_PtrSize(vNodes) );
    pAigNew->pName = Abc_UtilStrsav( pAig->pName );
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pAigNew );
    // create PIs
    Vec_PtrForEachEntry( Aig_Obj_t *, vLeaves, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pAigNew );
    // create LOs
    Vec_PtrForEachEntryStart( Aig_Obj_t *, vRoots, pObj, i, nPos )
        Saig_ObjLiToLo(pAig, pObj)->pData = Aig_ObjCreateCi( pAigNew );
    // create internal nodes
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        pObj->pData = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // create COs
    Vec_PtrForEachEntry( Aig_Obj_t *, vRoots, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );
    // finalize
    Aig_ManSetRegNum( pAigNew, Vec_PtrSize(vRoots)-nPos );
    Vec_PtrFree( vLeaves );
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vRoots );
    return pAigNew;

}

#ifndef ABC_USE_CUDD
int Aig_ManVerifyUsingBdds( Aig_Man_t * pInit, Saig_ParBbr_t * pPars ) { return 0; }
void Bbr_ManSetDefaultParams( Saig_ParBbr_t * p ) {}
#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

