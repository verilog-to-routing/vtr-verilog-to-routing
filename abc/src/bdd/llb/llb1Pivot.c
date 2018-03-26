/**CFile****************************************************************

  FileName    [llb1Pivot.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Determining pivot variables.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb1Pivot.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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
int Llb_ManTracePaths_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pPivot )
{
    Aig_Obj_t * pFanout;
    int k, iFan = -1;
    if ( Aig_ObjIsTravIdPrevious(p, pObj) )
        return 0;
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return 1;
    if ( Saig_ObjIsLi(p, pObj) )
        return 0;
    if ( Saig_ObjIsPo(p, pObj) )
        return 0;
    if ( pObj == pPivot )
        return 1;
    assert( Aig_ObjIsCand(pObj) );
    Aig_ObjForEachFanout( p, pObj, pFanout, iFan, k )
        if ( !Llb_ManTracePaths_rec( p, pFanout, pPivot ) )
        {
            Aig_ObjSetTravIdPrevious(p, pObj);
            return 0;
        }
    Aig_ObjSetTravIdCurrent(p, pObj);
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Llb_ManTracePaths( Aig_Man_t * p, Aig_Obj_t * pPivot )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    Aig_ManIncrementTravId( p ); // prev = visited with path to LI  (value 0)
    Aig_ManIncrementTravId( p ); // cur  = visited w/o  path to LI  (value 1)
    Saig_ManForEachLo( p, pObj, i )
        Counter += Llb_ManTracePaths_rec( p, pObj, pPivot );
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManTestCuts( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i, Count;
    Aig_ManFanoutStart( p );
    Aig_ManForEachNode( p, pObj, i )
    {
        if ( Aig_ObjRefs(pObj) <= 1 )
            continue;
        Count = Llb_ManTracePaths( p, pObj );
        printf( "Obj =%5d.  Lev =%3d.  Fanout =%5d.  Count = %3d.\n", 
            i, Aig_ObjLevel(pObj), Aig_ObjRefs(pObj), Count );
    }
    Aig_ManFanoutStop( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManLabelLiCones_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    if ( pObj->fMarkB )
        return;
    pObj->fMarkB = 1;
    assert( Aig_ObjIsNode(pObj) );
    Llb_ManLabelLiCones_rec( p, Aig_ObjFanin0(pObj) );
    Llb_ManLabelLiCones_rec( p, Aig_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Determine starting cut-points.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManLabelLiCones( Aig_Man_t * p )
{
    Aig_Obj_t * pObj;
    int i;
    // mark const and PIs
    Aig_ManConst1(p)->fMarkB = 1;
    Aig_ManForEachCi( p, pObj, i )
        pObj->fMarkB = 1;
    // mark cones
    Saig_ManForEachLi( p, pObj, i )
        Llb_ManLabelLiCones_rec( p, Aig_ObjFanin0(pObj) );
}

/**Function*************************************************************

  Synopsis    [Determine starting cut-points.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManMarkInternalPivots( Aig_Man_t * p )
{
    Vec_Ptr_t * vMuxes;
    Aig_Obj_t * pObj;
    int i, Counter = 0;

    // remove refs due to MUXes
    vMuxes = Aig_ManMuxesCollect( p );
    Aig_ManMuxesDeref( p, vMuxes );

    // mark nodes feeding into LIs
    Aig_ManCleanMarkB( p );
    Llb_ManLabelLiCones( p );

    // mark internal nodes
    Aig_ManFanoutStart( p );
    Aig_ManForEachNode( p, pObj, i )
        if ( pObj->fMarkB && pObj->nRefs > 1 )
        {
            if ( Llb_ManTracePaths(p, pObj) > 0 )
                pObj->fMarkA = 1;
            Counter++;
        }
    Aig_ManFanoutStop( p );
//    printf( "TracePath tried = %d.\n", Counter );

    // mark nodes feeding into LIs
    Aig_ManCleanMarkB( p );

    // add refs due to MUXes
    Aig_ManMuxesRef( p, vMuxes );
    Vec_PtrFree( vMuxes );
}

/**Function*************************************************************

  Synopsis    [Determine starting cut-points.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Llb_ManMarkPivotNodes( Aig_Man_t * p, int fUseInternal )
{
    Vec_Int_t * vVar2Obj;
    Aig_Obj_t * pObj;
    int i;
    // mark inputs/outputs
    Aig_ManForEachCi( p, pObj, i )
        pObj->fMarkA = 1;
    Saig_ManForEachLi( p, pObj, i )
        pObj->fMarkA = 1;

    // mark internal pivot nodes
    if ( fUseInternal )
        Llb_ManMarkInternalPivots( p );

    // assign variable numbers
    Aig_ManConst1(p)->fMarkA = 0;
    vVar2Obj = Vec_IntAlloc( 100 );
    Aig_ManForEachCi( p, pObj, i )
        Vec_IntPush( vVar2Obj, Aig_ObjId(pObj) );
    Aig_ManForEachNode( p, pObj, i )
        if ( pObj->fMarkA )
            Vec_IntPush( vVar2Obj, Aig_ObjId(pObj) );
    Saig_ManForEachLi( p, pObj, i )
        Vec_IntPush( vVar2Obj, Aig_ObjId(pObj) );
    return vVar2Obj;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

