/**CFile****************************************************************

  FileName    [ivyRwrAlg.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [Algebraic AIG rewriting.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyRwrAlg.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Ivy_ManFindAlgCut( Ivy_Obj_t * pRoot, Vec_Ptr_t * vFront, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vCone );
static Ivy_Obj_t * Ivy_NodeRewriteAlg( Ivy_Obj_t * pObj, Vec_Ptr_t * vFront, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vCone, Vec_Ptr_t * vSols, int LevelR, int fUseZeroCost );
static int Ivy_NodeCountMffc( Ivy_Obj_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Algebraic AIG rewriting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManRewriteAlg( Ivy_Man_t * p, int fUpdateLevel, int fUseZeroCost )
{
    Vec_Int_t * vRequired;
    Vec_Ptr_t * vFront, * vLeaves, * vCone, * vSol;
    Ivy_Obj_t * pObj, * pResult;
    int i, RetValue, LevelR, nNodesOld;
    int CountUsed, CountUndo;
    vRequired = fUpdateLevel? Ivy_ManRequiredLevels( p ) : NULL;
    vFront    = Vec_PtrAlloc( 100 );
    vLeaves   = Vec_PtrAlloc( 100 );
    vCone     = Vec_PtrAlloc( 100 );
    vSol      = Vec_PtrAlloc( 100 );
    // go through the nodes in the topological order
    CountUsed = CountUndo = 0;
    nNodesOld = Ivy_ManObjIdNext(p);
    Ivy_ManForEachObj( p, pObj, i )
    {
        assert( !Ivy_ObjIsBuf(pObj) );
        if ( i >= nNodesOld )
            break;
        // skip no-nodes and MUX roots
        if ( !Ivy_ObjIsNode(pObj) || Ivy_ObjIsExor(pObj) || Ivy_ObjIsMuxType(pObj) )
            continue;
//        if ( pObj->Id > 297 ) // 296 --- 297 
//            break;
        if ( pObj->Id == 297 )
        {
            int x = 0;
        }
        // get the largest algebraic cut
        RetValue = Ivy_ManFindAlgCut( pObj, vFront, vLeaves, vCone );
        // the case of a trivial tree cut
        if ( RetValue == 1 )
            continue;
        // the case of constant 0 cone
        if ( RetValue == -1 )
        {
            Ivy_ObjReplace( pObj, Ivy_ManConst0(p), 1, 0, 1 ); 
            continue;
        }
        assert( Vec_PtrSize(vLeaves) > 2 );
        // get the required level for this node
        LevelR = vRequired? Vec_IntEntry(vRequired, pObj->Id) : 1000000;
        // create a new cone
        pResult = Ivy_NodeRewriteAlg( pObj, vFront, vLeaves, vCone, vSol, LevelR, fUseZeroCost );
        if ( pResult == NULL || pResult == pObj )
            continue;
        assert( Vec_PtrSize(vSol) == 1 || !Ivy_IsComplement(pResult) );
        if ( Ivy_ObjLevel(Ivy_Regular(pResult)) > LevelR && Ivy_ObjRefs(Ivy_Regular(pResult)) == 0 )
            Ivy_ObjDelete_rec(Ivy_Regular(pResult), 1), CountUndo++;
        else
            Ivy_ObjReplace( pObj, pResult, 1, 0, 1 ), CountUsed++; 
    }
    printf( "Used = %d. Undo = %d.\n", CountUsed, CountUndo );
    Vec_PtrFree( vFront );
    Vec_PtrFree( vCone );
    Vec_PtrFree( vSol );
    if ( vRequired ) Vec_IntFree( vRequired );
    if ( i = Ivy_ManCleanup(p) )
        printf( "Cleanup after rewriting removed %d dangling nodes.\n", i );
    if ( !Ivy_ManCheck(p) )
        printf( "Ivy_ManRewriteAlg(): The check has failed.\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Analizes one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_NodeRewriteAlg( Ivy_Obj_t * pObj, Vec_Ptr_t * vFront, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vCone, Vec_Ptr_t * vSols, int LevelR, int fUseZeroCost )
{
    int fVerbose = 0;
    Ivy_Obj_t * pTemp;
    int k, Counter, nMffc, RetValue;

    if ( fVerbose )
    {
        if ( Ivy_ObjIsExor(pObj) )
            printf( "x " );
        else
            printf( "  " );
    }

/*       
    printf( "%d ", Vec_PtrSize(vFront) );
    printf( "( " );
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pTemp, k )
        printf( "%d ", Ivy_ObjRefs(Ivy_Regular(pTemp)) );
    printf( ")\n" );
*/
    // collect nodes in the cone
    if ( Ivy_ObjIsExor(pObj) )
        Ivy_ManCollectCone( pObj, vFront, vCone );
    else
        Ivy_ManCollectCone( pObj, vLeaves, vCone );

    // deref nodes in the cone
    Vec_PtrForEachEntry( Ivy_Obj_t *, vCone, pTemp, k )
    {
        Ivy_ObjRefsDec( Ivy_ObjFanin0(pTemp) );
        Ivy_ObjRefsDec( Ivy_ObjFanin1(pTemp) );
        pTemp->fMarkB = 1;
    }

    // count the MFFC size
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pTemp, k )
        Ivy_Regular(pTemp)->fMarkA = 1;
    nMffc = Ivy_NodeCountMffc( pObj );
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pTemp, k )
        Ivy_Regular(pTemp)->fMarkA = 0;

    if ( fVerbose )
    {
    Counter = 0;
    Vec_PtrForEachEntry( Ivy_Obj_t *, vCone, pTemp, k )
        Counter += (Ivy_ObjRefs(pTemp) > 0);
    printf( "%5d : Leaves = %2d. Cone = %2d. ConeRef = %2d.   Mffc = %d.   Lev = %d.  LevR = %d.\n", 
        pObj->Id, Vec_PtrSize(vFront), Vec_PtrSize(vCone), Counter-1, nMffc, Ivy_ObjLevel(pObj), LevelR );
    }
/*
    printf( "Leaves:" );
    Vec_PtrForEachEntry( Ivy_Obj_t *, vLeaves, pTemp, k )
        printf( " %d%s", Ivy_Regular(pTemp)->Id, Ivy_IsComplement(pTemp)? "\'" : "" );
    printf( "\n" );
    printf( "Cone:\n" );
    Vec_PtrForEachEntry( Ivy_Obj_t *, vCone, pTemp, k )
        printf( " %5d = %d%s %d%s\n", pTemp->Id,
            Ivy_ObjFaninId0(pTemp), Ivy_ObjFaninC0(pTemp)? "\'" : "",
            Ivy_ObjFaninId1(pTemp), Ivy_ObjFaninC1(pTemp)? "\'" : "" );
*/

    RetValue = Ivy_MultiPlus( vLeaves, vCone, Ivy_ObjType(pObj), nMffc + fUseZeroCost, vSols );

    // ref nodes in the cone
    Vec_PtrForEachEntry( Ivy_Obj_t *, vCone, pTemp, k )
    {
        Ivy_ObjRefsInc( Ivy_ObjFanin0(pTemp) );
        Ivy_ObjRefsInc( Ivy_ObjFanin1(pTemp) );
        pTemp->fMarkA = 0;
        pTemp->fMarkB = 0;
    }

    if ( !RetValue )
        return NULL;

    if ( Vec_PtrSize( vSols ) == 1 )
        return Vec_PtrEntry( vSols, 0 );
    return Ivy_NodeBalanceBuildSuper( vSols, Ivy_ObjType(pObj), 1 );
}

/**Function*************************************************************

  Synopsis    [Comparison for node pointers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_NodeCountMffc_rec( Ivy_Obj_t * pNode )
{
    if ( Ivy_ObjRefs(pNode) > 0 || Ivy_ObjIsCi(pNode) || pNode->fMarkA )
        return 0;
    assert( pNode->fMarkB );
    pNode->fMarkA = 1;
//    printf( "%d ", pNode->Id );
    if ( Ivy_ObjIsBuf(pNode) )
        return Ivy_NodeCountMffc_rec( Ivy_ObjFanin0(pNode) );
    return 1 + Ivy_NodeCountMffc_rec( Ivy_ObjFanin0(pNode) ) + Ivy_NodeCountMffc_rec( Ivy_ObjFanin1(pNode) );
}

/**Function*************************************************************

  Synopsis    [Comparison for node pointers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_NodeCountMffc( Ivy_Obj_t * pNode )
{
    assert( pNode->fMarkB );
    return 1 + Ivy_NodeCountMffc_rec( Ivy_ObjFanin0(pNode) ) + Ivy_NodeCountMffc_rec( Ivy_ObjFanin1(pNode) );
}

/**Function*************************************************************

  Synopsis    [Comparison for node pointers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManFindAlgCutCompare( Ivy_Obj_t ** pp1, Ivy_Obj_t ** pp2 )
{
    if ( *pp1 < *pp2 )
        return -1;
    if ( *pp1 > *pp2 )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Computing one algebraic cut.]

  Description [Returns 1 if the tree-leaves of this node where traversed 
  and found to have no external references (and have not been collected). 
  Returns 0 if the tree-leaves have external references and are collected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManFindAlgCut_rec( Ivy_Obj_t * pObj, Ivy_Type_t Type, Vec_Ptr_t * vFront, Vec_Ptr_t * vCone )
{
    int RetValue0, RetValue1;
    Ivy_Obj_t * pObjR = Ivy_Regular(pObj);
    assert( !Ivy_ObjIsBuf(pObjR) );
    assert( Type != IVY_EXOR || !Ivy_IsComplement(pObj) );

    // make sure the node is not visited twice in different polarities
    if ( Ivy_IsComplement(pObj) )
    { // if complemented, mark B
        if ( pObjR->fMarkA )
            return -1;
        pObjR->fMarkB = 1;
    }
    else
    { // if non-complicated, mark A
        if ( pObjR->fMarkB )
            return -1;
        pObjR->fMarkA = 1;
    }
    Vec_PtrPush( vCone, pObjR );

    // if the node is the end of the tree, return
    if ( Ivy_IsComplement(pObj) || Ivy_ObjType(pObj) != Type )
    {
        if ( Ivy_ObjRefs(pObjR) == 1 )
            return 1;
        assert( Ivy_ObjRefs(pObjR) > 1 );
        Vec_PtrPush( vFront, pObj );
        return 0;
    }

    // branch on the node
    assert( !Ivy_IsComplement(pObj) );
    assert( Ivy_ObjIsNode(pObj) );
    // what if buffer has more than one fanout???
    RetValue0 = Ivy_ManFindAlgCut_rec( Ivy_ObjReal( Ivy_ObjChild0(pObj) ), Type, vFront, vCone );
    RetValue1 = Ivy_ManFindAlgCut_rec( Ivy_ObjReal( Ivy_ObjChild1(pObj) ), Type, vFront, vCone );
    if ( RetValue0 == -1 || RetValue1 == -1 )
        return -1;

    // the case when both have no external references
    if ( RetValue0 && RetValue1 )
    {
        if ( Ivy_ObjRefs(pObj) == 1 )
            return 1;
        assert( Ivy_ObjRefs(pObj) > 1 );
        Vec_PtrPush( vFront, pObj );
        return 0;
    }
    // the case when one of them has external references
    if ( RetValue0 )
        Vec_PtrPush( vFront, Ivy_ObjReal( Ivy_ObjChild0(pObj) ) );
    if ( RetValue1 )
        Vec_PtrPush( vFront, Ivy_ObjReal( Ivy_ObjChild1(pObj) ) );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Computing one algebraic cut.]

  Description [Algebraic cut stops when we hit (a) CI, (b) complemented edge,
  (c) boundary of different gates. Returns 1 if this is a pure tree.
  Returns -1 if the contant 0 is detected. Return 0 if the array can be used.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_ManFindAlgCut( Ivy_Obj_t * pRoot, Vec_Ptr_t * vFront, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vCone )
{
    Ivy_Obj_t * pObj, * pPrev;
    int RetValue, i;
    assert( !Ivy_IsComplement(pRoot) );
    assert( Ivy_ObjIsNode(pRoot) );
    // clear the frontier and collect the nodes
    Vec_PtrClear( vCone );
    Vec_PtrClear( vFront );
    Vec_PtrClear( vLeaves );
    RetValue = Ivy_ManFindAlgCut_rec( pRoot, Ivy_ObjType(pRoot), vFront, vCone );
    // clean the marks
    Vec_PtrForEachEntry( Ivy_Obj_t *, vCone, pObj, i )
        pObj->fMarkA = pObj->fMarkB = 0;
    // quit if the same node is found in both polarities
    if ( RetValue == -1 )
        return -1;
    // return if the node is the root of a tree
    if ( RetValue == 1 )
        return 1;
    // return if the cut is composed of two nodes
    if ( Vec_PtrSize(vFront) <= 2 )
        return 1;
    // sort the entries in increasing order
    Vec_PtrSort( vFront, (int (*)(void))Ivy_ManFindAlgCutCompare );
    // remove duplicates from vFront and save the nodes in vLeaves
    pPrev = Vec_PtrEntry(vFront, 0);
    Vec_PtrPush( vLeaves, pPrev );
    Vec_PtrForEachEntryStart( Ivy_Obj_t *, vFront, pObj, i, 1 )
    {
        // compare current entry and the previous entry
        if ( pObj == pPrev )
        {
            if ( Ivy_ObjIsExor(pRoot) ) // A <+> A = 0
            {
                // vLeaves are no longer structural support of pRoot!!!
                Vec_PtrPop(vLeaves);  
                pPrev = Vec_PtrSize(vLeaves) == 0 ? NULL : Vec_PtrEntryLast(vLeaves);
            }
            continue;
        }
        if ( pObj == Ivy_Not(pPrev) )
        {
            assert( Ivy_ObjIsAnd(pRoot) );
            return -1;
        }
        pPrev = pObj;
        Vec_PtrPush( vLeaves, pObj );
    }
    if ( Vec_PtrSize(vLeaves) == 0 )
        return -1;
    if ( Vec_PtrSize(vLeaves) <= 2 )
        return 1;
    return 0;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

