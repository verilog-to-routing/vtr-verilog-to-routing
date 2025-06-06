/**CFile****************************************************************

  FileName    [abcHaig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Implements history AIG for combinational rewriting.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcHaig.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Collects the nodes in the classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkHaigCollectMembers( Hop_Man_t * p )
{
    Vec_Ptr_t * vObjs;
    Hop_Obj_t * pObj;
    int i;
    vObjs = Vec_PtrAlloc( 4098 );
    Vec_PtrForEachEntry( Hop_Obj_t *, p->vObjs, pObj, i )
    {
        if ( pObj->pData == NULL )
            continue;
        pObj->pData = Hop_ObjRepr( pObj );
        Vec_PtrPush( vObjs, pObj );
    }
    return vObjs;
}

/**Function*************************************************************

  Synopsis    [Creates classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Abc_NtkHaigCreateClasses( Vec_Ptr_t * vMembers )
{
    Vec_Ptr_t * vClasses;
    Hop_Obj_t * pObj, * pRepr;
    int i;

    // count classes
    vClasses = Vec_PtrAlloc( 4098 );
    Vec_PtrForEachEntry( Hop_Obj_t *, vMembers, pObj, i )
    {
        pRepr = (Hop_Obj_t *)pObj->pData;
        assert( pRepr->pData == NULL );
        if ( pRepr->fMarkA == 0 ) // new
        {
            pRepr->fMarkA = 1;
            Vec_PtrPush( vClasses, pRepr );
        }
    }

    // set representatives as representatives
    Vec_PtrForEachEntry( Hop_Obj_t *, vClasses, pObj, i )
    {
        pObj->fMarkA = 0;
        pObj->pData = pObj;
    }

    // go through the members and update
    Vec_PtrForEachEntry( Hop_Obj_t *, vMembers, pObj, i )
    {
        pRepr = (Hop_Obj_t *)pObj->pData;
        if ( ((Hop_Obj_t *)pRepr->pData)->Id > pObj->Id )
            pRepr->pData = pObj;
    }

    // change representatives of the class
    Vec_PtrForEachEntry( Hop_Obj_t *, vMembers, pObj, i )
    {
        pRepr = (Hop_Obj_t *)pObj->pData;
        pObj->pData = pRepr->pData;
        assert( ((Hop_Obj_t *)pObj->pData)->Id <= pObj->Id );
    }

    // update classes
    Vec_PtrForEachEntry( Hop_Obj_t *, vClasses, pObj, i )
    {
        pRepr = (Hop_Obj_t *)pObj->pData;
        assert( pRepr->pData == pRepr );
//        pRepr->pData = NULL;
        Vec_PtrWriteEntry( vClasses, i, pRepr );
        Vec_PtrPush( vMembers, pObj );
    }

    Vec_PtrForEachEntry( Hop_Obj_t *, vMembers, pObj, i )
        if ( pObj->pData == pObj )
            pObj->pData = NULL;

/*
    Vec_PtrForEachEntry( Hop_Obj_t *, vMembers, pObj, i )
    {
        printf( "ObjId = %4d : ", pObj->Id );
        if ( pObj->pData == NULL )
        {
            printf( "NULL" );
        }
        else
        {
            printf( "%4d", ((Hop_Obj_t *)pObj->pData)->Id );
            assert( ((Hop_Obj_t *)pObj->pData)->Id <= pObj->Id );
        }
        printf( "\n" );
    }
*/
    return vClasses;
}

/**Function*************************************************************

  Synopsis    [Counts how many data members have non-trivial fanout.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkHaigCountFans( Hop_Man_t * p )
{
    Hop_Obj_t * pObj;
    int i, Counter = 0;
    Vec_PtrForEachEntry( Hop_Obj_t *, p->vObjs, pObj, i )
    {
        if ( pObj->pData == NULL )
            continue;
        if ( Hop_ObjRefs(pObj) > 0 )
            Counter++;
    }
    printf( "The number of class members with fanouts = %5d.\n", Counter );
    return Counter;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Hop_Obj_t * Hop_ObjReprHop( Hop_Obj_t * pObj )
{
    Hop_Obj_t * pRepr;
    assert( pObj->pNext != NULL );
    if ( pObj->pData == NULL )
        return pObj->pNext;
    pRepr = (Hop_Obj_t *)pObj->pData;
    assert( pRepr->pData == pRepr );
    return Hop_NotCond( pRepr->pNext, pObj->fPhase ^ pRepr->fPhase );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Hop_Obj_t * Hop_ObjChild0Hop( Hop_Obj_t * pObj ) { return Hop_NotCond( Hop_ObjReprHop(Hop_ObjFanin0(pObj)), Hop_ObjFaninC0(pObj) ); }
static inline Hop_Obj_t * Hop_ObjChild1Hop( Hop_Obj_t * pObj ) { return Hop_NotCond( Hop_ObjReprHop(Hop_ObjFanin1(pObj)), Hop_ObjFaninC1(pObj) ); }

/**Function*************************************************************

  Synopsis    [Stops history AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Man_t * Abc_NtkHaigReconstruct( Hop_Man_t * p )
{ 
    Hop_Man_t * pNew;
    Hop_Obj_t * pObj;
    int i, Counter = 0;
    Vec_PtrForEachEntry( Hop_Obj_t *, p->vObjs, pObj, i )
        pObj->pNext = NULL;
    // start the HOP package
    pNew = Hop_ManStart();
    pNew->vObjs = Vec_PtrAlloc( p->nCreated );
    Vec_PtrPush( pNew->vObjs, Hop_ManConst1(pNew) );
    // map the constant node
    Hop_ManConst1(p)->pNext = Hop_ManConst1(pNew);
    // map the CIs
    Hop_ManForEachPi( p, pObj, i )
        pObj->pNext = Hop_ObjCreatePi(pNew);
    // map the internal nodes
    Vec_PtrForEachEntry( Hop_Obj_t *, p->vObjs, pObj, i )
    {
        if ( !Hop_ObjIsNode(pObj) )
            continue;
        pObj->pNext = Hop_And( pNew, Hop_ObjChild0Hop(pObj), Hop_ObjChild1Hop(pObj) );
//        assert( !Hop_IsComplement(pObj->pNext) );
        if ( Hop_ManConst1(pNew) == Hop_Regular(pObj->pNext) )
            Counter++;
        if ( pObj->pData ) // member of the class
            Hop_Regular(pObj->pNext)->pData = Hop_Regular(((Hop_Obj_t *)pObj->pData)->pNext);
    }
//    printf( " Counter = %d.\n", Counter );
    // transfer the POs
    Hop_ManForEachPo( p, pObj, i )
        Hop_ObjCreatePo( pNew, Hop_ObjChild0Hop(pObj) );
    // check the new manager
    if ( !Hop_ManCheck(pNew) )
    {
        printf( "Abc_NtkHaigReconstruct: Check for History AIG has failed.\n" );
        Hop_ManStop(pNew);
        return NULL;
    }
    return pNew;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if pOld is in the TFI of pNew.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkHaigCheckTfi_rec( Abc_Obj_t * pNode, Abc_Obj_t * pOld )
{
    if ( pNode == NULL )
        return 0;
    if ( pNode == pOld )
        return 1;
    // check the trivial cases
    if ( Abc_ObjIsCi(pNode) )
        return 0;
    assert( Abc_ObjIsNode(pNode) );
    // if this node is already visited, skip
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return 0;
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pNode );
    // check the children
    if ( Abc_NtkHaigCheckTfi_rec( Abc_ObjFanin0(pNode), pOld ) )
        return 1;
    if ( Abc_NtkHaigCheckTfi_rec( Abc_ObjFanin1(pNode), pOld ) )
        return 1;
    // check equivalent nodes
    return Abc_NtkHaigCheckTfi_rec( (Abc_Obj_t *)pNode->pData, pOld );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pOld is in the TFI of pNew.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkHaigCheckTfi( Abc_Ntk_t * pNtk, Abc_Obj_t * pOld, Abc_Obj_t * pNew )
{
    assert( !Abc_ObjIsComplement(pOld) );
    assert( !Abc_ObjIsComplement(pNew) );
    Abc_NtkIncrementTravId(pNtk);
    return Abc_NtkHaigCheckTfi_rec( pNew, pOld );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Abc_Obj_t * Hop_ObjChild0Next( Hop_Obj_t * pObj ) { return Abc_ObjNotCond( (Abc_Obj_t *)Hop_ObjFanin0(pObj)->pNext, Hop_ObjFaninC0(pObj) );  }
static inline Abc_Obj_t * Hop_ObjChild1Next( Hop_Obj_t * pObj ) { return Abc_ObjNotCond( (Abc_Obj_t *)Hop_ObjFanin1(pObj)->pNext, Hop_ObjFaninC1(pObj) );  }

/**Function*************************************************************

  Synopsis    [Stops history AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkHaigRecreateAig( Abc_Ntk_t * pNtk, Hop_Man_t * p )
{
    Abc_Ntk_t * pNtkAig;
    Abc_Obj_t * pObjOld, * pObjAbcThis, * pObjAbcRepr;
    Hop_Obj_t * pObj;
    int i;
    assert( p->nCreated == Vec_PtrSize(p->vObjs) );

    // start the new network
    pNtkAig = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );

    // transfer new nodes to the PIs of HOP
    Hop_ManConst1(p)->pNext = (Hop_Obj_t *)Abc_AigConst1( pNtkAig );
    Hop_ManForEachPi( p, pObj, i )
        pObj->pNext = (Hop_Obj_t *)Abc_NtkCi( pNtkAig, i );

    // construct new nodes
    Vec_PtrForEachEntry( Hop_Obj_t *, p->vObjs, pObj, i )
    {
        if ( !Hop_ObjIsNode(pObj) )
            continue;
        pObj->pNext = (Hop_Obj_t *)Abc_AigAnd( (Abc_Aig_t *)pNtkAig->pManFunc, Hop_ObjChild0Next(pObj), Hop_ObjChild1Next(pObj) );
        assert( !Hop_IsComplement(pObj->pNext) );
    }

    // set the COs
    Abc_NtkForEachCo( pNtk, pObjOld, i )
        Abc_ObjAddFanin( pObjOld->pCopy, Hop_ObjChild0Next(Hop_ManPo(p,i)) );

    // construct choice nodes
    Vec_PtrForEachEntry( Hop_Obj_t *, p->vObjs, pObj, i )
    {
        // skip the node without choices
        if ( pObj->pData == NULL )
            continue;
        // skip the representative of the class
        if ( pObj->pData == pObj )
            continue;
        // do not create choices for constant 1 and PIs
        if ( !Hop_ObjIsNode((Hop_Obj_t *)pObj->pData) )
            continue;
        // get the corresponding new nodes
        pObjAbcThis = (Abc_Obj_t *)pObj->pNext;
        pObjAbcRepr = (Abc_Obj_t *)((Hop_Obj_t *)pObj->pData)->pNext;
        // the new node cannot be already in the class
        assert( pObjAbcThis->pData == NULL );
        // the new node cannot have fanouts
        assert( Abc_ObjFanoutNum(pObjAbcThis) == 0 );
        // these should be different nodes
        assert( pObjAbcRepr != pObjAbcThis );
        // do not create choices if there is a path from pObjAbcThis to pObjAbcRepr
        if ( !Abc_NtkHaigCheckTfi( pNtkAig, pObjAbcRepr, pObjAbcThis ) )
        {
            // find the last node in the class
            while ( pObjAbcRepr->pData )
                pObjAbcRepr = (Abc_Obj_t *)pObjAbcRepr->pData;
            // add the new node at the end of the list
            pObjAbcRepr->pData = pObjAbcThis;
        }
    }

    // finish the new network
//    Abc_NtkFinalize( pNtk, pNtkAig );
//    Abc_AigCleanup( pNtkAig->pManFunc );
    // check correctness of the network
    if ( !Abc_NtkCheck( pNtkAig ) )
    {
        printf( "Abc_NtkHaigUse: The network check has failed.\n" );
        Abc_NtkDelete( pNtkAig );
        return NULL;
    }
    return pNtkAig;
}

/**Function*************************************************************

  Synopsis    [Resets representatives.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkHaigResetReprsOld( Hop_Man_t * pMan )
{
    Vec_Ptr_t * vMembers, * vClasses;

    // collect members of the classes and make them point to reprs
    vMembers = Abc_NtkHaigCollectMembers( pMan );
    printf( "Collected %6d class members.\n", Vec_PtrSize(vMembers) );

    // create classes
    vClasses = Abc_NtkHaigCreateClasses( vMembers );
    printf( "Collected %6d classes. (Ave = %5.2f)\n", Vec_PtrSize(vClasses), 
        (float)(Vec_PtrSize(vMembers))/Vec_PtrSize(vClasses) );

    Vec_PtrFree( vMembers );
    Vec_PtrFree( vClasses );
}

/**Function*************************************************************

  Synopsis    [Resets representatives.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkHaigResetReprs( Hop_Man_t * p )
{
    Hop_Obj_t * pObj, * pRepr;
    int i, nClasses, nMembers, nFanouts, nNormals;
    // clear self-classes
    Vec_PtrForEachEntry( Hop_Obj_t *, p->vObjs, pObj, i )
    {
        // fix the strange situation of double-loop
        pRepr = (Hop_Obj_t *)pObj->pData;
        if ( pRepr && pRepr->pData == pObj )
            pRepr->pData = pRepr;
        // remove self-loops
        if ( pObj->pData == pObj )
            pObj->pData = NULL;
    }
    // set representatives
    Vec_PtrForEachEntry( Hop_Obj_t *, p->vObjs, pObj, i )
    {
        if ( pObj->pData == NULL )
            continue;
        // get representative of the node
        pRepr = Hop_ObjRepr( pObj );
        pRepr->pData = pRepr;
        // set the representative
        pObj->pData = pRepr;
    }
    // make each class point to the smallest topological order
    Vec_PtrForEachEntry( Hop_Obj_t *, p->vObjs, pObj, i )
    {
        if ( pObj->pData == NULL )
            continue;
        pRepr = Hop_ObjRepr( pObj );
        if ( pRepr->Id > pObj->Id )
        {
            pRepr->pData = pObj;
            pObj->pData = pObj;
        }
        else
            pObj->pData = pRepr;
    }
    // count classes, members, and fanouts - and verify
    nMembers = nClasses = nFanouts = nNormals = 0;
    Vec_PtrForEachEntry( Hop_Obj_t *, p->vObjs, pObj, i )
    {
        if ( pObj->pData == NULL )
            continue;
        // count members
        nMembers++;
        // count the classes and fanouts
        if ( pObj->pData == pObj )
            nClasses++;
        else if ( Hop_ObjRefs(pObj) > 0 )
            nFanouts++;
        else
            nNormals++;
        // compare representatives
        pRepr = Hop_ObjRepr( pObj );
        assert( pObj->pData == pRepr );
        assert( pRepr->Id <= pObj->Id );
    }
//    printf( "Nodes = %7d.  Member = %7d.  Classes = %6d.  Fanouts = %6d.  Normals = %6d.\n", 
//        Hop_ManNodeNum(p), nMembers, nClasses, nFanouts, nNormals );
    return nFanouts;
}

/**Function*************************************************************

  Synopsis    [Transform HOP manager into the one without loops.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkHopRemoveLoops( Abc_Ntk_t * pNtk, Hop_Man_t * pMan )
{
    Abc_Ntk_t * pNtkAig;
    Hop_Man_t * pManTemp;

    // iteratively reconstruct the HOP manager to create choice nodes
    while ( Abc_NtkHaigResetReprs( pMan ) )
    {
        pMan = Abc_NtkHaigReconstruct( pManTemp = pMan );
        Hop_ManStop( pManTemp );
    }

    // traverse in the topological order and create new AIG
    pNtkAig = Abc_NtkHaigRecreateAig( pNtk, pMan );
    Hop_ManStop( pMan );
    return pNtkAig;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

