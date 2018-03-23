/**CFile****************************************************************

  FileName    [hopDfs.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic And-Inverter Graph package.]

  Synopsis    [DFS traversal procedures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: hopDfs.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "hop.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ManDfs_rec( Hop_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return;
    Hop_ManDfs_rec( Hop_ObjFanin0(pObj), vNodes );
    Hop_ManDfs_rec( Hop_ObjFanin1(pObj), vNodes );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA(pObj);
    Vec_PtrPush( vNodes, pObj );
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Hop_ManDfs( Hop_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Hop_Obj_t * pObj;
    int i;
    vNodes = Vec_PtrAlloc( Hop_ManNodeNum(p) );
    Hop_ManForEachNode( p, pObj, i )
        Hop_ManDfs_rec( pObj, vNodes );
    Hop_ManForEachNode( p, pObj, i )
        Hop_ObjClearMarkA(pObj);
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Hop_ManDfsNode( Hop_Man_t * p, Hop_Obj_t * pNode )
{
    Vec_Ptr_t * vNodes;
    Hop_Obj_t * pObj;
    int i;
    assert( !Hop_IsComplement(pNode) );
    vNodes = Vec_PtrAlloc( 16 );
    Hop_ManDfs_rec( pNode, vNodes );
    Vec_PtrForEachEntry( Hop_Obj_t *, vNodes, pObj, i )
        Hop_ObjClearMarkA(pObj);
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Computes the max number of levels in the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hop_ManCountLevels( Hop_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Hop_Obj_t * pObj;
    int i, LevelsMax, Level0, Level1;
    // initialize the levels
    Hop_ManConst1(p)->pData = NULL;
    Hop_ManForEachPi( p, pObj, i )
        pObj->pData = NULL;
    // compute levels in a DFS order
    vNodes = Hop_ManDfs( p );
    Vec_PtrForEachEntry( Hop_Obj_t *, vNodes, pObj, i )
    {
        Level0 = (int)(ABC_PTRUINT_T)Hop_ObjFanin0(pObj)->pData;
        Level1 = (int)(ABC_PTRUINT_T)Hop_ObjFanin1(pObj)->pData;
        pObj->pData = (void *)(ABC_PTRUINT_T)(1 + Hop_ObjIsExor(pObj) + Abc_MaxInt(Level0, Level1));
    }
    Vec_PtrFree( vNodes );
    // get levels of the POs
    LevelsMax = 0;
    Hop_ManForEachPo( p, pObj, i )
        LevelsMax = Abc_MaxInt( LevelsMax, (int)(ABC_PTRUINT_T)Hop_ObjFanin0(pObj)->pData );
    return LevelsMax;
}

/**Function*************************************************************

  Synopsis    [Creates correct reference counters at each node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ManCreateRefs( Hop_Man_t * p )
{
    Hop_Obj_t * pObj;
    int i;
    if ( p->fRefCount )
        return;
    p->fRefCount = 1;
    // clear refs
    Hop_ObjClearRef( Hop_ManConst1(p) );
    Hop_ManForEachPi( p, pObj, i )
        Hop_ObjClearRef( pObj );
    Hop_ManForEachNode( p, pObj, i )
        Hop_ObjClearRef( pObj );
    Hop_ManForEachPo( p, pObj, i )
        Hop_ObjClearRef( pObj );
    // set refs
    Hop_ManForEachNode( p, pObj, i )
    {
        Hop_ObjRef( Hop_ObjFanin0(pObj) );
        Hop_ObjRef( Hop_ObjFanin1(pObj) );
    }
    Hop_ManForEachPo( p, pObj, i )
        Hop_ObjRef( Hop_ObjFanin0(pObj) );
}

/**Function*************************************************************

  Synopsis    [Counts the number of AIG nodes rooted at this cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ConeMark_rec( Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return;
    Hop_ConeMark_rec( Hop_ObjFanin0(pObj) );
    Hop_ConeMark_rec( Hop_ObjFanin1(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Counts the number of AIG nodes rooted at this cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ConeCleanAndMark_rec( Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return;
    Hop_ConeCleanAndMark_rec( Hop_ObjFanin0(pObj) );
    Hop_ConeCleanAndMark_rec( Hop_ObjFanin1(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
    pObj->pData = NULL;
}

/**Function*************************************************************

  Synopsis    [Counts the number of AIG nodes rooted at this cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hop_ConeCountAndMark_rec( Hop_Obj_t * pObj )
{
    int Counter;
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return 0;
    Counter = 1 + Hop_ConeCountAndMark_rec( Hop_ObjFanin0(pObj) ) + 
        Hop_ConeCountAndMark_rec( Hop_ObjFanin1(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts the number of AIG nodes rooted at this cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_ConeUnmark_rec( Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || !Hop_ObjIsMarkA(pObj) )
        return;
    Hop_ConeUnmark_rec( Hop_ObjFanin0(pObj) ); 
    Hop_ConeUnmark_rec( Hop_ObjFanin1(pObj) );
    assert( Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjClearMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Counts the number of AIG nodes rooted at this cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hop_DagSize( Hop_Obj_t * pObj )
{
    int Counter;
    Counter = Hop_ConeCountAndMark_rec( Hop_Regular(pObj) );
    Hop_ConeUnmark_rec( Hop_Regular(pObj) );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Counts how many fanout the given node has.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hop_ObjFanoutCount_rec( Hop_Obj_t * pObj, Hop_Obj_t * pPivot )
{
    int Counter;
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return (int)(pObj == pPivot);
    Counter = Hop_ObjFanoutCount_rec( Hop_ObjFanin0(pObj), pPivot ) + 
              Hop_ObjFanoutCount_rec( Hop_ObjFanin1(pObj), pPivot );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
    return Counter;
}
int Hop_ObjFanoutCount( Hop_Obj_t * pObj, Hop_Obj_t * pPivot )
{
    int Counter;
    assert( !Hop_IsComplement(pPivot) );
    Counter = Hop_ObjFanoutCount_rec( Hop_Regular(pObj), pPivot );
    Hop_ConeUnmark_rec( Hop_Regular(pObj) );
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Transfers the AIG from one manager into another.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_Transfer_rec( Hop_Man_t * pDest, Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return;
    Hop_Transfer_rec( pDest, Hop_ObjFanin0(pObj) ); 
    Hop_Transfer_rec( pDest, Hop_ObjFanin1(pObj) );
    pObj->pData = Hop_And( pDest, Hop_ObjChild0Copy(pObj), Hop_ObjChild1Copy(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Transfers the AIG from one manager into another.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Transfer( Hop_Man_t * pSour, Hop_Man_t * pDest, Hop_Obj_t * pRoot, int nVars )
{
    Hop_Obj_t * pObj;
    int i;
    // solve simple cases
    if ( pSour == pDest )
        return pRoot;
    if ( Hop_ObjIsConst1( Hop_Regular(pRoot) ) )
        return Hop_NotCond( Hop_ManConst1(pDest), Hop_IsComplement(pRoot) );
    // set the PI mapping
    Hop_ManForEachPi( pSour, pObj, i )
    {
        if ( i == nVars )
           break;
        pObj->pData = Hop_IthVar(pDest, i);
    }
    // transfer and set markings
    Hop_Transfer_rec( pDest, Hop_Regular(pRoot) );
    // clear the markings
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );
    return Hop_NotCond( (Hop_Obj_t *)Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );
}

/**Function*************************************************************

  Synopsis    [Composes the AIG (pRoot) with the function (pFunc) using PI var (iVar).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_Compose_rec( Hop_Man_t * p, Hop_Obj_t * pObj, Hop_Obj_t * pFunc, Hop_Obj_t * pVar )
{
    assert( !Hop_IsComplement(pObj) );
    if ( Hop_ObjIsMarkA(pObj) )
        return;
    if ( Hop_ObjIsConst1(pObj) || Hop_ObjIsPi(pObj) )
    {
        pObj->pData = pObj == pVar ? pFunc : pObj;
        return;
    }
    Hop_Compose_rec( p, Hop_ObjFanin0(pObj), pFunc, pVar ); 
    Hop_Compose_rec( p, Hop_ObjFanin1(pObj), pFunc, pVar );
    pObj->pData = Hop_And( p, Hop_ObjChild0Copy(pObj), Hop_ObjChild1Copy(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Composes the AIG (pRoot) with the function (pFunc) using PI var (iVar).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Compose( Hop_Man_t * p, Hop_Obj_t * pRoot, Hop_Obj_t * pFunc, int iVar )
{
    // quit if the PI variable is not defined
    if ( iVar >= Hop_ManPiNum(p) )
    {
        printf( "Hop_Compose(): The PI variable %d is not defined.\n", iVar );
        return NULL;
    }
    // recursively perform composition
    Hop_Compose_rec( p, Hop_Regular(pRoot), pFunc, Hop_ManPi(p, iVar) );
    // clear the markings
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );
    return Hop_NotCond( (Hop_Obj_t *)Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );
}

/**Function*************************************************************

  Synopsis    [Complements the AIG (pRoot) with the function (pFunc) using PI var (iVar).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_Complement_rec( Hop_Man_t * p, Hop_Obj_t * pObj, Hop_Obj_t * pVar )
{
    assert( !Hop_IsComplement(pObj) );
    if ( Hop_ObjIsMarkA(pObj) )
        return;
    if ( Hop_ObjIsConst1(pObj) || Hop_ObjIsPi(pObj) )
    {
        pObj->pData = pObj == pVar ? Hop_Not(pObj) : pObj;
        return;
    }
    Hop_Complement_rec( p, Hop_ObjFanin0(pObj), pVar ); 
    Hop_Complement_rec( p, Hop_ObjFanin1(pObj), pVar );
    pObj->pData = Hop_And( p, Hop_ObjChild0Copy(pObj), Hop_ObjChild1Copy(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Complements the AIG (pRoot) with the function (pFunc) using PI var (iVar).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Complement( Hop_Man_t * p, Hop_Obj_t * pRoot, int iVar )
{
    // quit if the PI variable is not defined
    if ( iVar >= Hop_ManPiNum(p) )
    {
        printf( "Hop_Complement(): The PI variable %d is not defined.\n", iVar );
        return NULL;
    }
    // recursively perform composition
    Hop_Complement_rec( p, Hop_Regular(pRoot), Hop_ManPi(p, iVar) );
    // clear the markings
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );
    return Hop_NotCond( (Hop_Obj_t *)Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );
}

/**Function*************************************************************

  Synopsis    [Remaps the AIG (pRoot) to have the given support (uSupp).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_Remap_rec( Hop_Man_t * p, Hop_Obj_t * pObj )
{
    assert( !Hop_IsComplement(pObj) );
    if ( !Hop_ObjIsNode(pObj) || Hop_ObjIsMarkA(pObj) )
        return;
    Hop_Remap_rec( p, Hop_ObjFanin0(pObj) ); 
    Hop_Remap_rec( p, Hop_ObjFanin1(pObj) );
    pObj->pData = Hop_And( p, Hop_ObjChild0Copy(pObj), Hop_ObjChild1Copy(pObj) );
    assert( !Hop_ObjIsMarkA(pObj) ); // loop detection
    Hop_ObjSetMarkA( pObj );
}

/**Function*************************************************************

  Synopsis    [Remaps the AIG (pRoot) to have the given support (uSupp).]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Remap( Hop_Man_t * p, Hop_Obj_t * pRoot, unsigned uSupp, int nVars )
{
    Hop_Obj_t * pObj;
    int i, k;
    // quit if the PI variable is not defined
    if ( nVars > Hop_ManPiNum(p) )
    {
        printf( "Hop_Remap(): The number of variables (%d) is more than the manager size (%d).\n", nVars, Hop_ManPiNum(p) );
        return NULL;
    }
    // return if constant
    if ( Hop_ObjIsConst1( Hop_Regular(pRoot) ) )
        return pRoot;
    if ( uSupp == 0 )
        return Hop_NotCond( Hop_ManConst0(p), Hop_ObjPhaseCompl(pRoot) );
    // set the PI mapping
    k = 0;
    Hop_ManForEachPi( p, pObj, i )
    {
        if ( i == nVars )
           break;
        if ( uSupp & (1 << i) )
            pObj->pData = Hop_IthVar(p, k++);
        else
            pObj->pData = Hop_ManConst0(p);
    }
    assert( k > 0 && k < nVars );
    // recursively perform composition
    Hop_Remap_rec( p, Hop_Regular(pRoot) );
    // clear the markings
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );
    return Hop_NotCond( (Hop_Obj_t *)Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );
}

/**Function*************************************************************

  Synopsis    [Permute the AIG according to the given permutation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_Permute( Hop_Man_t * p, Hop_Obj_t * pRoot, int nRootVars, int * pPermute )
{
    Hop_Obj_t * pObj;
    int i;
    // return if constant
    if ( Hop_ObjIsConst1( Hop_Regular(pRoot) ) )
        return pRoot;
    // create mapping
    Hop_ManForEachPi( p, pObj, i )
    {
        if ( i == nRootVars )
            break;
        assert( pPermute[i] >= 0 && pPermute[i] < Hop_ManPiNum(p) );
        pObj->pData = Hop_IthVar( p, pPermute[i] );
    }
    // recursively perform composition
    Hop_Remap_rec( p, Hop_Regular(pRoot) );
    // clear the markings
    Hop_ConeUnmark_rec( Hop_Regular(pRoot) );
    return Hop_NotCond( (Hop_Obj_t *)Hop_Regular(pRoot)->pData, Hop_IsComplement(pRoot) );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

