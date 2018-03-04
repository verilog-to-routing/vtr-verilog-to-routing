/**CFile****************************************************************

  FileName    [llb1Group.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Initial partition computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb1Group.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"

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
Llb_Grp_t * Llb_ManGroupAlloc( Llb_Man_t * pMan )
{
    Llb_Grp_t * p;
    p = ABC_CALLOC( Llb_Grp_t, 1 );
    p->pMan   = pMan;
    p->vIns   = Vec_PtrAlloc( 8 );
    p->vOuts  = Vec_PtrAlloc( 8 );
    p->Id     = Vec_PtrSize( pMan->vGroups );
    Vec_PtrPush( pMan->vGroups, p );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManGroupStop( Llb_Grp_t * p )
{
    if ( p == NULL )
        return;
    Vec_PtrWriteEntry( p->pMan->vGroups, p->Id, NULL );
    Vec_PtrFreeP( &p->vIns );
    Vec_PtrFreeP( &p->vOuts );
    Vec_PtrFreeP( &p->vNodes );
    ABC_FREE( p );
}

 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManGroupCollect_rec( Aig_Man_t * pAig, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(pAig, pObj);
    if ( Aig_ObjIsConst1(pObj) )
        return;
    if ( Aig_ObjIsCo(pObj) )
    {
        Llb_ManGroupCollect_rec( pAig, Aig_ObjFanin0(pObj), vNodes );
        return;
    }
    assert( Aig_ObjIsAnd(pObj) );
    Llb_ManGroupCollect_rec( pAig, Aig_ObjFanin0(pObj), vNodes );
    Llb_ManGroupCollect_rec( pAig, Aig_ObjFanin1(pObj), vNodes );
    Vec_PtrPush( vNodes, pObj );
}
 
/**Function*************************************************************

  Synopsis    [Collects the support of MFFC.]

  Description [Returns the number of internal nodes in the MFFC.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Llb_ManGroupCollect( Llb_Grp_t * pGroup )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    vNodes = Vec_PtrAlloc( 100 );
    Aig_ManIncrementTravId( pGroup->pMan->pAig );
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vIns, pObj, i )
        Aig_ObjSetTravIdCurrent( pGroup->pMan->pAig, pObj );
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vOuts, pObj, i )
        Aig_ObjSetTravIdPrevious( pGroup->pMan->pAig, pObj );
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vOuts, pObj, i )
        Llb_ManGroupCollect_rec( pGroup->pMan->pAig, pObj, vNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManGroupCreate_rec( Aig_Man_t * pAig, Aig_Obj_t * pObj, Vec_Ptr_t * vSupp )
{ 
    if ( Aig_ObjIsTravIdCurrent(pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(pAig, pObj);
    if ( Aig_ObjIsConst1(pObj) )
        return;
    if ( pObj->fMarkA )
    {
        Vec_PtrPush( vSupp, pObj );
        return;
    }
    assert( Aig_ObjIsAnd(pObj) );
    Llb_ManGroupCreate_rec( pAig, Aig_ObjFanin0(pObj), vSupp );
    Llb_ManGroupCreate_rec( pAig, Aig_ObjFanin1(pObj), vSupp );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Llb_Grp_t * Llb_ManGroupCreate( Llb_Man_t * pMan, Aig_Obj_t * pObj )
{
    Llb_Grp_t * p;
    assert( pObj->fMarkA == 1 );
    // derive group
    p = Llb_ManGroupAlloc( pMan );
    Vec_PtrPush( p->vOuts, pObj );
    Aig_ManIncrementTravId( pMan->pAig );
    if ( Aig_ObjIsCo(pObj) )
        Llb_ManGroupCreate_rec( pMan->pAig, Aig_ObjFanin0(pObj), p->vIns );
    else
    {
        Llb_ManGroupCreate_rec( pMan->pAig, Aig_ObjFanin0(pObj), p->vIns );
        Llb_ManGroupCreate_rec( pMan->pAig, Aig_ObjFanin1(pObj), p->vIns );
    }
    // derive internal objects
    assert( p->vNodes == NULL );
    p->vNodes = Llb_ManGroupCollect( p );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Llb_Grp_t * Llb_ManGroupCreateFirst( Llb_Man_t * pMan )
{
    Llb_Grp_t * p;
    Aig_Obj_t * pObj;
    int i;
    p = Llb_ManGroupAlloc( pMan );
    Saig_ManForEachLo( pMan->pAig, pObj, i )
        Vec_PtrPush( p->vOuts, pObj );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Llb_Grp_t * Llb_ManGroupCreateLast( Llb_Man_t * pMan )
{
    Llb_Grp_t * p;
    Aig_Obj_t * pObj;
    int i;
    p = Llb_ManGroupAlloc( pMan );
    Saig_ManForEachLi( pMan->pAig, pObj, i )
        Vec_PtrPush( p->vIns, pObj );
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Llb_Grp_t * Llb_ManGroupsCombine( Llb_Grp_t * p1, Llb_Grp_t * p2 )
{
    Llb_Grp_t * p;
    Aig_Obj_t * pObj;
    int i;
    p = Llb_ManGroupAlloc( p1->pMan );
    // create inputs
    Vec_PtrForEachEntry( Aig_Obj_t *, p1->vIns, pObj, i )
        Vec_PtrPush( p->vIns, pObj );
    Vec_PtrForEachEntry( Aig_Obj_t *, p2->vIns, pObj, i )
        Vec_PtrPushUnique( p->vIns, pObj );
    // create outputs
    Vec_PtrForEachEntry( Aig_Obj_t *, p1->vOuts, pObj, i )
        Vec_PtrPush( p->vOuts, pObj );
    Vec_PtrForEachEntry( Aig_Obj_t *, p2->vOuts, pObj, i )
        Vec_PtrPushUnique( p->vOuts, pObj );

    // derive internal objects
    assert( p->vNodes == NULL );
    p->vNodes = Llb_ManGroupCollect( p );
    return p;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManGroupMarkNodes_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    if ( Aig_ObjIsTravIdPrevious(p, pObj) )
    {
        Aig_ObjSetTravIdCurrent(p, pObj);
        return;
    }
    Aig_ObjSetTravIdCurrent(p, pObj);
    assert( Aig_ObjIsNode(pObj) );
    Llb_ManGroupMarkNodes_rec( p, Aig_ObjFanin0(pObj) );
    Llb_ManGroupMarkNodes_rec( p, Aig_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Creates group from two cuts derived by the flow computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Llb_Grp_t * Llb_ManGroupCreateFromCuts( Llb_Man_t * pMan, Vec_Int_t * vCut1, Vec_Int_t * vCut2 )
{
    Llb_Grp_t * p;
    Aig_Obj_t * pObj;
    int i;
    p = Llb_ManGroupAlloc( pMan );

    // mark Cut1
    Aig_ManIncrementTravId( pMan->pAig );
    Aig_ManForEachObjVec( vCut1, pMan->pAig, pObj, i )
        Aig_ObjSetTravIdCurrent( pMan->pAig, pObj );
    // collect unmarked Cut2
    Aig_ManForEachObjVec( vCut2, pMan->pAig, pObj, i )
        if ( !Aig_ObjIsTravIdCurrent( pMan->pAig, pObj ) )
            Vec_PtrPush( p->vOuts, pObj );

    // mark nodes reachable from Cut2
    Aig_ManIncrementTravId( pMan->pAig );
    Aig_ManForEachObjVec( vCut2, pMan->pAig, pObj, i )
        Llb_ManGroupMarkNodes_rec( pMan->pAig, pObj );
    // collect marked Cut1
    Aig_ManForEachObjVec( vCut1, pMan->pAig, pObj, i )
        if ( Aig_ObjIsTravIdCurrent( pMan->pAig, pObj ) )
            Vec_PtrPush( p->vIns, pObj );

    // derive internal objects
    assert( p->vNodes == NULL );
    p->vNodes = Llb_ManGroupCollect( p );
    return p;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManPrepareGroups( Llb_Man_t * pMan )
{
    Aig_Obj_t * pObj;
    int i;
    assert( pMan->vGroups == NULL );
    pMan->vGroups = Vec_PtrAlloc( 1000 );
    Llb_ManGroupCreateFirst( pMan );
    Aig_ManForEachNode( pMan->pAig, pObj, i )
    {
        if ( pObj->fMarkA )
            Llb_ManGroupCreate( pMan, pObj );
    }
    Saig_ManForEachLi( pMan->pAig, pObj, i )
    {
        if ( pObj->fMarkA )
            Llb_ManGroupCreate( pMan, pObj );
    }
    Llb_ManGroupCreateLast( pMan );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManPrintSpan( Llb_Man_t * p )
{
    Llb_Grp_t * pGroup;
    Aig_Obj_t * pVar;
    int i, k, Span = 0, SpanMax = 0;
    Vec_PtrForEachEntry( Llb_Grp_t *, p->vGroups, pGroup, i )
    {
        Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vIns, pVar, k )
            if ( Vec_IntEntry(p->vVarBegs, pVar->Id) == i )
                Span++;
        Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vOuts, pVar, k )
            if ( Vec_IntEntry(p->vVarBegs, pVar->Id) == i )
                Span++;

        SpanMax = Abc_MaxInt( SpanMax, Span );
printf( "%d ", Span );

        Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vIns, pVar, k )
            if ( Vec_IntEntry(p->vVarEnds, pVar->Id) == i )
                Span--;
        Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vOuts, pVar, k )
            if ( Vec_IntEntry(p->vVarEnds, pVar->Id) == i )
                Span--;
    }
printf( "\n" );
printf( "Max = %d\n", SpanMax );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManGroupHasVar( Llb_Man_t * p, int iGroup, int iVar )
{
    Llb_Grp_t * pGroup = (Llb_Grp_t *)Vec_PtrEntry( p->vGroups, iGroup );
    Aig_Obj_t * pObj;
    int i;
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vIns, pObj, i )
        if ( pObj->Id == iVar )
            return 1;
    Vec_PtrForEachEntry( Aig_Obj_t *, pGroup->vOuts, pObj, i )
        if ( pObj->Id == iVar )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManPrintHisto( Llb_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, k;
    Aig_ManForEachObj( p->pAig, pObj, i )
    {
        if ( Vec_IntEntry(p->vObj2Var, i) < 0 )
            continue;
        printf( "%3d :", i );
        for ( k = 0; k < Vec_IntEntry(p->vVarBegs, i); k++ )
            printf( " " );
        for (      ; k <= Vec_IntEntry(p->vVarEnds, i); k++ )
            printf( "%c", Llb_ManGroupHasVar(p, k, i)? '*':'-' );
        printf( "\n" );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

