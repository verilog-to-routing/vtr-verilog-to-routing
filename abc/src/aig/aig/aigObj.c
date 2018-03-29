/**CFile****************************************************************

  FileName    [aigObj.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Adding/removing objects.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigObj.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates primary input.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_ObjCreateCi( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    pObj = Aig_ManFetchMemory( p );
    pObj->Type = AIG_OBJ_CI;
    Vec_PtrPush( p->vCis, pObj );
    p->nObjs[AIG_OBJ_CI]++;
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Creates primary output with the given driver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_ObjCreateCo( Aig_Man_t * p, Aig_Obj_t * pDriver )
{
    Aig_Obj_t * pObj;
    pObj = Aig_ManFetchMemory( p );
    pObj->Type = AIG_OBJ_CO;
    Vec_PtrPush( p->vCos, pObj );
    Aig_ObjConnect( p, pObj, pDriver, NULL );
    p->nObjs[AIG_OBJ_CO]++;
    return pObj;
}


/**Function*************************************************************

  Synopsis    [Create the new node assuming it does not exist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_ObjCreate( Aig_Man_t * p, Aig_Obj_t * pGhost )
{
    Aig_Obj_t * pObj;
    assert( !Aig_IsComplement(pGhost) );
    assert( Aig_ObjIsHash(pGhost) );
//    assert( pGhost == &p->Ghost );
    // get memory for the new object
    pObj = Aig_ManFetchMemory( p );
    pObj->Type = pGhost->Type;
    // add connections
    Aig_ObjConnect( p, pObj, pGhost->pFanin0, pGhost->pFanin1 );
    // update node counters of the manager
    p->nObjs[Aig_ObjType(pObj)]++;
    assert( pObj->pData == NULL );
    // create the power counter
    if ( p->vProbs )
    {
        float Prob0 = Abc_Int2Float( Vec_IntEntry( p->vProbs, Aig_ObjFaninId0(pObj) ) );
        float Prob1 = Abc_Int2Float( Vec_IntEntry( p->vProbs, Aig_ObjFaninId1(pObj) ) );
        Prob0 = Aig_ObjFaninC0(pObj)? 1.0 - Prob0 : Prob0;
        Prob1 = Aig_ObjFaninC1(pObj)? 1.0 - Prob1 : Prob1;
        Vec_IntSetEntry( p->vProbs, pObj->Id, Abc_Float2Int(Prob0 * Prob1) );
    }
    return pObj;
}
 
/**Function*************************************************************

  Synopsis    [Connect the object to the fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjConnect( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFan0, Aig_Obj_t * pFan1 )
{
    assert( !Aig_IsComplement(pObj) );
    assert( !Aig_ObjIsCi(pObj) );
    // add the first fanin
    pObj->pFanin0 = pFan0;
    pObj->pFanin1 = pFan1;
    // increment references of the fanins and add their fanouts
    if ( pFan0 != NULL )
    {
        assert( Aig_ObjFanin0(pObj)->Type > 0 );
        Aig_ObjRef( Aig_ObjFanin0(pObj) );
        if ( p->pFanData )
            Aig_ObjAddFanout( p, Aig_ObjFanin0(pObj), pObj );
    }
    if ( pFan1 != NULL )
    {
        assert( Aig_ObjFanin1(pObj)->Type > 0 );
        Aig_ObjRef( Aig_ObjFanin1(pObj) );
        if ( p->pFanData )
            Aig_ObjAddFanout( p, Aig_ObjFanin1(pObj), pObj );
    }
    // set level and phase
    pObj->Level = Aig_ObjLevelNew( pObj );
    pObj->fPhase = Aig_ObjPhaseReal(pFan0) & Aig_ObjPhaseReal(pFan1);
    // add the node to the structural hash table
    if ( p->pTable && Aig_ObjIsHash(pObj) )
        Aig_TableInsert( p, pObj );
    // add the node to the dynamically updated topological order
//    if ( p->pOrderData && Aig_ObjIsNode(pObj) )
//        Aig_ObjOrderInsert( p, pObj->Id );
    assert( !Aig_ObjIsNode(pObj) || pObj->Level > 0 );
}

/**Function*************************************************************

  Synopsis    [Disconnects the object from the fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjDisconnect( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    assert( !Aig_IsComplement(pObj) );
    // remove connections
    if ( pObj->pFanin0 != NULL )
    {
        if ( p->pFanData )
            Aig_ObjRemoveFanout( p, Aig_ObjFanin0(pObj), pObj );
        Aig_ObjDeref(Aig_ObjFanin0(pObj));
    }
    if ( pObj->pFanin1 != NULL )
    {
        if ( p->pFanData )
            Aig_ObjRemoveFanout( p, Aig_ObjFanin1(pObj), pObj );
        Aig_ObjDeref(Aig_ObjFanin1(pObj));
    }
    // remove the node from the structural hash table
    if ( p->pTable && Aig_ObjIsHash(pObj) )
        Aig_TableDelete( p, pObj );
    // add the first fanin
    pObj->pFanin0 = NULL;
    pObj->pFanin1 = NULL;
    // remove the node from the dynamically updated topological order
//    if ( p->pOrderData && Aig_ObjIsNode(pObj) )
//        Aig_ObjOrderRemove( p, pObj->Id );
}

/**Function*************************************************************

  Synopsis    [Deletes the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjDelete( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    assert( !Aig_IsComplement(pObj) );
    assert( !Aig_ObjIsTerm(pObj) );
    assert( Aig_ObjRefs(pObj) == 0 );
    if ( p->pFanData && Aig_ObjIsBuf(pObj) )
        Vec_PtrRemove( p->vBufs, pObj );
    p->nObjs[pObj->Type]--;
    Vec_PtrWriteEntry( p->vObjs, pObj->Id, NULL );
    Aig_ManRecycleMemory( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Deletes the MFFC of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjDelete_rec( Aig_Man_t * p, Aig_Obj_t * pObj, int fFreeTop )
{
    Aig_Obj_t * pFanin0, * pFanin1;
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsConst1(pObj) || Aig_ObjIsCi(pObj) )
        return;
    assert( !Aig_ObjIsCo(pObj) );
    pFanin0 = Aig_ObjFanin0(pObj);
    pFanin1 = Aig_ObjFanin1(pObj);
    Aig_ObjDisconnect( p, pObj );
    if ( fFreeTop )
        Aig_ObjDelete( p, pObj );
    if ( pFanin0 && !Aig_ObjIsNone(pFanin0) && Aig_ObjRefs(pFanin0) == 0 )
        Aig_ObjDelete_rec( p, pFanin0, 1 );
    if ( pFanin1 && !Aig_ObjIsNone(pFanin1) && Aig_ObjRefs(pFanin1) == 0 )
        Aig_ObjDelete_rec( p, pFanin1, 1 );
}

/**Function*************************************************************

  Synopsis    [Deletes the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjDeletePo( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    assert( Aig_ObjIsCo(pObj) );
    Aig_ObjDeref(Aig_ObjFanin0(pObj));
    pObj->pFanin0 = NULL;
    p->nObjs[pObj->Type]--;
    Vec_PtrWriteEntry( p->vObjs, pObj->Id, NULL );
    Aig_ManRecycleMemory( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Replaces the first fanin of the node by the new fanin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjPatchFanin0( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFaninNew )
{
    Aig_Obj_t * pFaninOld;
    assert( !Aig_IsComplement(pObj) );
    assert( Aig_ObjIsCo(pObj) );
    pFaninOld = Aig_ObjFanin0(pObj);
    // decrement ref and remove fanout
    if ( p->pFanData )
        Aig_ObjRemoveFanout( p, pFaninOld, pObj );
    Aig_ObjDeref( pFaninOld );
    // update the fanin
    pObj->pFanin0 = pFaninNew;
    pObj->Level = Aig_ObjLevelNew( pObj );
    pObj->fPhase = Aig_ObjPhaseReal(pObj->pFanin0);
    // increment ref and add fanout
    if ( p->pFanData )
        Aig_ObjAddFanout( p, Aig_ObjFanin0(pObj), pObj );
    Aig_ObjRef( Aig_ObjFanin0(pObj) );
    // get rid of old fanin
    if ( !Aig_ObjIsCi(pFaninOld) && !Aig_ObjIsConst1(pFaninOld) && Aig_ObjRefs(pFaninOld) == 0 )
        Aig_ObjDelete_rec( p, pFaninOld, 1 );
}

/**Function*************************************************************

  Synopsis    [Verbose printing of the AIG node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjPrint( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    int fShowFanouts = 0;
    Aig_Obj_t * pTemp;
    if ( pObj == NULL )
    {
        printf( "Object is NULL." );
        return;
    }
    if ( Aig_IsComplement(pObj) )
    {
        printf( "Compl " );
        pObj = Aig_Not(pObj);
    }
    assert( !Aig_IsComplement(pObj) );
    printf( "Node %4d : ", Aig_ObjId(pObj) );
    if ( Aig_ObjIsConst1(pObj) )
        printf( "constant 1" );
    else if ( Aig_ObjIsCi(pObj) )
        printf( "PI" );
    else if ( Aig_ObjIsCo(pObj) )
        printf( "PO( %4d%s )", Aig_ObjFanin0(pObj)->Id, (Aig_ObjFaninC0(pObj)? "\'" : " ") );
    else if ( Aig_ObjIsBuf(pObj) )
        printf( "BUF( %d%s )", Aig_ObjFanin0(pObj)->Id, (Aig_ObjFaninC0(pObj)? "\'" : " ") );
    else
        printf( "AND( %4d%s, %4d%s )", 
            Aig_ObjFanin0(pObj)->Id, (Aig_ObjFaninC0(pObj)? "\'" : " "), 
            Aig_ObjFanin1(pObj)->Id, (Aig_ObjFaninC1(pObj)? "\'" : " ") );
    printf( " (refs = %3d)", Aig_ObjRefs(pObj) );
    if ( fShowFanouts && p->pFanData )
    {
        Aig_Obj_t * pFanout;
        int i;
        int iFan = -1; // Suppress "might be used uninitialized"
        printf( "\nFanouts:\n" );
        Aig_ObjForEachFanout( p, pObj, pFanout, iFan, i )
        {
            printf( "    " );
            printf( "Node %4d : ", Aig_ObjId(pFanout) );
            if ( Aig_ObjIsCo(pFanout) )
                printf( "PO( %4d%s )", Aig_ObjFanin0(pFanout)->Id, (Aig_ObjFaninC0(pFanout)? "\'" : " ") );
            else if ( Aig_ObjIsBuf(pFanout) )
                printf( "BUF( %d%s )", Aig_ObjFanin0(pFanout)->Id, (Aig_ObjFaninC0(pFanout)? "\'" : " ") );
            else
                printf( "AND( %4d%s, %4d%s )", 
                    Aig_ObjFanin0(pFanout)->Id, (Aig_ObjFaninC0(pFanout)? "\'" : " "), 
                    Aig_ObjFanin1(pFanout)->Id, (Aig_ObjFaninC1(pFanout)? "\'" : " ") );
            printf( "\n" );
        }
        return;
    }
    // there are choices
    if ( p->pEquivs && p->pEquivs[pObj->Id] )
    {
        // print equivalence class
        printf( "  { %4d ", pObj->Id );
        for ( pTemp = p->pEquivs[pObj->Id]; pTemp; pTemp = p->pEquivs[pTemp->Id] )
            printf( " %4d%s", pTemp->Id, (pTemp->fPhase != pObj->fPhase)? "\'" : " " );
        printf( " }" );
        return;
    }
    // this is a secondary node
    if ( p->pReprs && p->pReprs[pObj->Id] )
        printf( "  class of %d", pObj->Id );
}

/**Function*************************************************************

  Synopsis    [Replaces node with a buffer fanin by a node without them.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_NodeFixBufferFanins( Aig_Man_t * p, Aig_Obj_t * pObj, int fUpdateLevel )
{
    Aig_Obj_t * pFanReal0, * pFanReal1, * pResult = NULL;
    p->nBufFixes++;
    if ( Aig_ObjIsCo(pObj) )
    {
        assert( Aig_ObjIsBuf(Aig_ObjFanin0(pObj)) );
        pFanReal0 = Aig_ObjReal_rec( Aig_ObjChild0(pObj) );
        assert( Aig_ObjPhaseReal(Aig_ObjChild0(pObj)) == Aig_ObjPhaseReal(pFanReal0) );
        Aig_ObjPatchFanin0( p, pObj, pFanReal0 );
        return;
    }
    assert( Aig_ObjIsNode(pObj) );
    assert( Aig_ObjIsBuf(Aig_ObjFanin0(pObj)) || Aig_ObjIsBuf(Aig_ObjFanin1(pObj)) );
    // get the real fanins
    pFanReal0 = Aig_ObjReal_rec( Aig_ObjChild0(pObj) );
    pFanReal1 = Aig_ObjReal_rec( Aig_ObjChild1(pObj) );
    // get the new node
    if ( Aig_ObjIsNode(pObj) )
        pResult = Aig_Oper( p, pFanReal0, pFanReal1, Aig_ObjType(pObj) );
//    else if ( Aig_ObjIsLatch(pObj) )
//        pResult = Aig_Latch( p, pFanReal0, Aig_ObjInit(pObj) );
    else 
        assert( 0 );
    // replace the node with buffer by the node without buffer
    Aig_ObjReplace( p, pObj, pResult, fUpdateLevel );
}

/**Function*************************************************************

  Synopsis    [Returns the number of dangling nodes removed.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
int Aig_ManPropagateBuffers( Aig_Man_t * p, int fUpdateLevel )
{
    Aig_Obj_t * pObj;
    int nSteps;
    assert( p->pFanData );
    for ( nSteps = 0; Vec_PtrSize(p->vBufs) > 0; nSteps++ )
    {
        // get the node with a buffer fanin
        for ( pObj = (Aig_Obj_t *)Vec_PtrEntryLast(p->vBufs); Aig_ObjIsBuf(pObj); pObj = Aig_ObjFanout0(p, pObj) );
        // replace this node by a node without buffer
        Aig_NodeFixBufferFanins( p, pObj, fUpdateLevel );
        // stop if a cycle occured
        if ( nSteps > 1000000 )
        {
            printf( "Error: A cycle is encountered while propagating buffers.\n" );
            break;
        }
    }
    return nSteps;
}

/**Function*************************************************************

  Synopsis    [Replaces one object by another.]

  Description [The new object (pObjNew) should be used instead of the old 
  object (pObjOld). If the new object is complemented or used, the buffer 
  is added and the new object remains in the manager; otherwise, the new
  object is deleted.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ObjReplace( Aig_Man_t * p, Aig_Obj_t * pObjOld, Aig_Obj_t * pObjNew, int fUpdateLevel )
{
    Aig_Obj_t * pObjNewR = Aig_Regular(pObjNew);
    // the object to be replaced cannot be complemented
    assert( !Aig_IsComplement(pObjOld) );
    // the object to be replaced cannot be a terminal
    assert( !Aig_ObjIsCi(pObjOld) && !Aig_ObjIsCo(pObjOld) );
    // the object to be used cannot be a buffer or a PO
    assert( !Aig_ObjIsBuf(pObjNewR) && !Aig_ObjIsCo(pObjNewR) );
    // the object cannot be the same
    assert( pObjOld != pObjNewR );
    // make sure object is not pointing to itself
    assert( pObjOld != Aig_ObjFanin0(pObjNewR) );
    assert( pObjOld != Aig_ObjFanin1(pObjNewR) );
    if ( pObjOld == Aig_ObjFanin0(pObjNewR) || pObjOld == Aig_ObjFanin1(pObjNewR) )
    {
        printf( "Aig_ObjReplace(): Internal error!\n" );
        exit(1);
    }
    // recursively delete the old node - but leave the object there
    pObjNewR->nRefs++;
    Aig_ObjDelete_rec( p, pObjOld, 0 );
    pObjNewR->nRefs--;
    // if the new object is complemented or already used, create a buffer
    p->nObjs[pObjOld->Type]--;
    if ( Aig_IsComplement(pObjNew) || Aig_ObjRefs(pObjNew) > 0 || !Aig_ObjIsNode(pObjNew) )
    {
        pObjOld->Type = AIG_OBJ_BUF;
        Aig_ObjConnect( p, pObjOld, pObjNew, NULL );
        p->nBufReplaces++;
    }
    else
    {
        Aig_Obj_t * pFanin0 = pObjNew->pFanin0;
        Aig_Obj_t * pFanin1 = pObjNew->pFanin1;
        int LevelOld = pObjOld->Level;
        pObjOld->Type = pObjNew->Type;
        Aig_ObjDisconnect( p, pObjNew );
        Aig_ObjConnect( p, pObjOld, pFanin0, pFanin1 );
        // delete the new object
        Aig_ObjDelete( p, pObjNew );
        // update levels
        if ( p->pFanData )
        {
            pObjOld->Level = LevelOld;
            Aig_ManUpdateLevel( p, pObjOld );
        }
        if ( fUpdateLevel )
        {
            Aig_ObjClearReverseLevel( p, pObjOld );
            Aig_ManUpdateReverseLevel( p, pObjOld );
        }
    }
    p->nObjs[pObjOld->Type]++;
    // store buffers if fanout is allocated
    if ( p->pFanData && Aig_ObjIsBuf(pObjOld) )
    {
        Vec_PtrPush( p->vBufs, pObjOld );
        p->nBufMax = Abc_MaxInt( p->nBufMax, Vec_PtrSize(p->vBufs) );
        Aig_ManPropagateBuffers( p, fUpdateLevel );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

