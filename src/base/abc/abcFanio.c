/**CFile****************************************************************

  FileName    [abcFanio.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Various procedures to connect fanins/fanouts.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcFanio.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Vec_IntPushMem( Mem_Step_t * pMemMan, Vec_Int_t * p, int Entry )
{
    if ( p->nSize == p->nCap )
    {
        int * pArray;
        int i;

        if ( p->nSize == 0 )
            p->nCap = 1;
        if ( pMemMan )
            pArray = (int *)Mem_StepEntryFetch( pMemMan, p->nCap * 8 );
        else
            pArray = ABC_ALLOC( int, p->nCap * 2 );
        if ( p->pArray )
        {
            for ( i = 0; i < p->nSize; i++ )
                pArray[i] = p->pArray[i];
            if ( pMemMan )
                Mem_StepEntryRecycle( pMemMan, (char *)p->pArray, p->nCap * 4 );
            else
                ABC_FREE( p->pArray );
        }
        p->nCap *= 2;
        p->pArray = pArray;
    }
    p->pArray[p->nSize++] = Entry;
}

/**Function*************************************************************

  Synopsis    [Creates fanout/fanin relationship between the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjAddFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFanin )
{
    Abc_Obj_t * pFaninR = Abc_ObjRegular(pFanin);
    assert( !Abc_ObjIsComplement(pObj) );
    assert( pObj->pNtk == pFaninR->pNtk );
    assert( pObj->Id >= 0 && pFaninR->Id >= 0 );
    assert( !Abc_ObjIsPi(pObj) && !Abc_ObjIsPo(pFaninR) );    // fanin of PI or fanout of PO
    assert( !Abc_ObjIsCo(pObj) || !Abc_ObjFaninNum(pObj) );  // CO with two fanins
    assert( !Abc_ObjIsNet(pObj) || !Abc_ObjFaninNum(pObj) ); // net with two fanins
    Vec_IntPushMem( pObj->pNtk->pMmStep, &pObj->vFanins,     pFaninR->Id );
    Vec_IntPushMem( pObj->pNtk->pMmStep, &pFaninR->vFanouts, pObj->Id    );
    if ( Abc_ObjIsComplement(pFanin) )
        Abc_ObjSetFaninC( pObj, Abc_ObjFaninNum(pObj)-1 );
}


/**Function*************************************************************

  Synopsis    [Destroys fanout/fanin relationship between the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjDeleteFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFanin )
{
    assert( !Abc_ObjIsComplement(pObj) );
    assert( !Abc_ObjIsComplement(pFanin) );
    assert( pObj->pNtk == pFanin->pNtk );
    assert( pObj->Id >= 0 && pFanin->Id >= 0 );
    if ( !Vec_IntRemove( &pObj->vFanins, pFanin->Id ) )
    {
        printf( "The obj %d is not found among the fanins of obj %d ...\n", pFanin->Id, pObj->Id );
        return;
    }
    if ( !Vec_IntRemove( &pFanin->vFanouts, pObj->Id ) )
    {
        printf( "The obj %d is not found among the fanouts of obj %d ...\n", pObj->Id, pFanin->Id );
        return;
    }
}


/**Function*************************************************************

  Synopsis    [Destroys fanout/fanin relationship between the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjRemoveFanins( Abc_Obj_t * pObj )
{
    Vec_Int_t * vFaninsOld;
    Abc_Obj_t * pFanin;
    int k;
    // remove old fanins
    vFaninsOld = &pObj->vFanins;
    for ( k = vFaninsOld->nSize - 1; k >= 0; k-- )
    {
        pFanin = Abc_NtkObj( pObj->pNtk, vFaninsOld->pArray[k] );
        Abc_ObjDeleteFanin( pObj, pFanin );
    }
    pObj->fCompl0 = 0;
    pObj->fCompl1 = 0;
    assert( vFaninsOld->nSize == 0 );
}

/**Function*************************************************************

  Synopsis    [Replaces a fanin of the node.]

  Description [The node is pObj. An old fanin of this node (pFaninOld) has to be
  replaced by a new fanin (pFaninNew). Assumes that the node and the old fanin 
  are not complemented. The new fanin can be complemented. In this case, the
  polarity of the new fanin will change, compared to the polarity of the old fanin.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjPatchFanin( Abc_Obj_t * pObj, Abc_Obj_t * pFaninOld, Abc_Obj_t * pFaninNew )
{
    Abc_Obj_t * pFaninNewR = Abc_ObjRegular(pFaninNew);
    int iFanin;//, nLats;//, fCompl;
    assert( !Abc_ObjIsComplement(pObj) );
    assert( !Abc_ObjIsComplement(pFaninOld) );
    assert( pFaninOld != pFaninNewR );
//    assert( pObj != pFaninOld );
//    assert( pObj != pFaninNewR );
    assert( pObj->pNtk == pFaninOld->pNtk );
    assert( pObj->pNtk == pFaninNewR->pNtk );
    if ( (iFanin = Vec_IntFind( &pObj->vFanins, pFaninOld->Id )) == -1 )
    {
        printf( "Node %s is not among", Abc_ObjName(pFaninOld) );
        printf( " the fanins of node %s...\n", Abc_ObjName(pObj) );
        return;
    }

    // remember the attributes of the old fanin
//    fCompl = Abc_ObjFaninC(pObj, iFanin);
    // replace the old fanin entry by the new fanin entry (removes attributes)
    Vec_IntWriteEntry( &pObj->vFanins, iFanin, pFaninNewR->Id );
    // set the attributes of the new fanin
//    if ( fCompl ^ Abc_ObjIsComplement(pFaninNew) )
//        Abc_ObjSetFaninC( pObj, iFanin );
    if ( Abc_ObjIsComplement(pFaninNew) )
        Abc_ObjXorFaninC( pObj, iFanin );

//    if ( Abc_NtkIsSeq(pObj->pNtk) && (nLats = Seq_ObjFaninL(pObj, iFanin)) )
//        Seq_ObjSetFaninL( pObj, iFanin, nLats );
    // update the fanout of the fanin
    if ( !Vec_IntRemove( &pFaninOld->vFanouts, pObj->Id ) )
    {
        printf( "Node %s is not among", Abc_ObjName(pObj) );
        printf( " the fanouts of its old fanin %s...\n", Abc_ObjName(pFaninOld) );
//        return;
    }
    Vec_IntPushMem( pObj->pNtk->pMmStep, &pFaninNewR->vFanouts, pObj->Id );
}

/**Function*************************************************************

  Synopsis    [Replaces pObj by iObjNew in the fanin arrays of the fanouts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjPatchFanoutFanin( Abc_Obj_t * pObj, int iObjNew )
{
    Abc_Obj_t * pFanout;
    int i, k, Entry;
    // update fanouts of the node to point to this one
    Abc_ObjForEachFanout( pObj, pFanout, i )
    {
        Vec_IntForEachEntry( &pFanout->vFanins, Entry, k )
            if ( Entry == (int)Abc_ObjId(pObj) )
            {
                Vec_IntWriteEntry( &pFanout->vFanins, k, iObjNew );
                break;
            }
        assert( k < Vec_IntSize(&pFanout->vFanins) );
    }
}

/**Function*************************************************************

  Synopsis    [Inserts one-input node of the type specified between the nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_ObjInsertBetween( Abc_Obj_t * pNodeIn, Abc_Obj_t * pNodeOut, Abc_ObjType_t Type )
{
    Abc_Obj_t * pNodeNew;
    int iFanoutIndex, iFaninIndex;
    // find pNodeOut among the fanouts of pNodeIn
    if ( (iFanoutIndex = Vec_IntFind( &pNodeIn->vFanouts, pNodeOut->Id )) == -1 )
    {
        printf( "Node %s is not among", Abc_ObjName(pNodeOut) );
        printf( " the fanouts of node %s...\n", Abc_ObjName(pNodeIn) );
        return NULL;
    }
    // find pNodeIn among the fanins of pNodeOut
    if ( (iFaninIndex = Vec_IntFind( &pNodeOut->vFanins, pNodeIn->Id )) == -1 )
    {
        printf( "Node %s is not among", Abc_ObjName(pNodeIn) );
        printf( " the fanins of node %s...\n", Abc_ObjName(pNodeOut) );
        return NULL;
    }
    // create the new node
    pNodeNew = Abc_NtkCreateObj( pNodeIn->pNtk, Type );
    // add pNodeIn as fanin and pNodeOut as fanout
    Vec_IntPushMem( pNodeNew->pNtk->pMmStep, &pNodeNew->vFanins,  pNodeIn->Id  );
    Vec_IntPushMem( pNodeNew->pNtk->pMmStep, &pNodeNew->vFanouts, pNodeOut->Id );
    // update the fanout of pNodeIn
    Vec_IntWriteEntry( &pNodeIn->vFanouts, iFanoutIndex, pNodeNew->Id );
    // update the fanin of pNodeOut
    Vec_IntWriteEntry( &pNodeOut->vFanins, iFaninIndex, pNodeNew->Id );
    return pNodeNew;
}

/**Function*************************************************************

  Synopsis    [Transfers fanout from the old node to the new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjTransferFanout( Abc_Obj_t * pNodeFrom, Abc_Obj_t * pNodeTo )
{
    Vec_Ptr_t * vFanouts;
    int nFanoutsOld, i;
    assert( !Abc_ObjIsComplement(pNodeFrom) );
    assert( !Abc_ObjIsComplement(pNodeTo) );
    assert( !Abc_ObjIsPo(pNodeFrom) && !Abc_ObjIsPo(pNodeTo) );
    assert( pNodeFrom->pNtk == pNodeTo->pNtk );
    assert( pNodeFrom != pNodeTo );
    assert( !Abc_ObjIsNode(pNodeFrom) || Abc_ObjFanoutNum(pNodeFrom) > 0 );
    // get the fanouts of the old node
    nFanoutsOld = Abc_ObjFanoutNum(pNodeTo);
    vFanouts = Vec_PtrAlloc( nFanoutsOld );
    Abc_NodeCollectFanouts( pNodeFrom, vFanouts );
    // patch the fanin of each of them
    for ( i = 0; i < vFanouts->nSize; i++ )
        Abc_ObjPatchFanin( (Abc_Obj_t *)vFanouts->pArray[i], pNodeFrom, pNodeTo );
    assert( Abc_ObjFanoutNum(pNodeFrom) == 0 );
    assert( Abc_ObjFanoutNum(pNodeTo) == nFanoutsOld + vFanouts->nSize );
    Vec_PtrFree( vFanouts );
}

/**Function*************************************************************

  Synopsis    [Replaces the node by a new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_ObjReplace( Abc_Obj_t * pNodeOld, Abc_Obj_t * pNodeNew )
{
    assert( !Abc_ObjIsComplement(pNodeOld) );
    assert( !Abc_ObjIsComplement(pNodeNew) );
    assert( pNodeOld->pNtk == pNodeNew->pNtk );
    assert( pNodeOld != pNodeNew );
    assert( Abc_ObjFanoutNum(pNodeOld) > 0 );
    // transfer the fanouts to the old node
    Abc_ObjTransferFanout( pNodeOld, pNodeNew );
    // remove the old node
    Abc_NtkDeleteObj_rec( pNodeOld, 1 );
}

/**Function*************************************************************

  Synopsis    [Returns the index of the fanin in the fanin list of the fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjFanoutFaninNum( Abc_Obj_t * pFanout, Abc_Obj_t * pFanin )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_ObjForEachFanin( pFanout, pObj, i )
        if ( pObj == pFanin )
            return i;
    return -1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

