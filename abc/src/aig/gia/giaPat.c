/**CFile****************************************************************

  FileName    [gia.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: gia.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int   Sat_ObjXValue( Gia_Obj_t * pObj )          { return (pObj->fMark1 << 1) | pObj->fMark0;             }
static inline void  Sat_ObjSetXValue( Gia_Obj_t * pObj, int v) { pObj->fMark0 = (v & 1); pObj->fMark1 = ((v >> 1) & 1); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collects nodes in the cone and initialized them to x.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_SatCollectCone_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vVisit )
{
    if ( Sat_ObjXValue(pObj) == GIA_UND )
        return;
    if ( Gia_ObjIsAnd(pObj) )
    {
        Gia_SatCollectCone_rec( p, Gia_ObjFanin0(pObj), vVisit );
        Gia_SatCollectCone_rec( p, Gia_ObjFanin1(pObj), vVisit );
    }
    assert( Sat_ObjXValue(pObj) == 0 );
    Sat_ObjSetXValue( pObj, GIA_UND );
    Vec_IntPush( vVisit, Gia_ObjId(p, pObj) );
}

/**Function*************************************************************

  Synopsis    [Collects nodes in the cone and initialized them to x.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_SatCollectCone( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vVisit )
{
    assert( !Gia_IsComplement(pObj) );
    assert( !Gia_ObjIsConst0(pObj) );
    assert( Sat_ObjXValue(pObj) == 0 );
    Vec_IntClear( vVisit );
    Gia_SatCollectCone_rec( p, pObj, vVisit );
}

/**Function*************************************************************

  Synopsis    [Checks if the counter-examples asserts the output.]

  Description [Assumes that fMark0 and fMark1 are clean. Leaves them clean.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_SatVerifyPattern( Gia_Man_t * p, Gia_Obj_t * pRoot, Vec_Int_t * vCex, Vec_Int_t * vVisit )
{
    Gia_Obj_t * pObj;
    int i, Entry, Value, Value0, Value1;
    assert( Gia_ObjIsCo(pRoot) );
    assert( !Gia_ObjIsConst0(Gia_ObjFanin0(pRoot)) );
    // collect nodes and initialized them to x
    Gia_SatCollectCone( p, Gia_ObjFanin0(pRoot), vVisit );
    // set binary values to nodes in the counter-example
    Vec_IntForEachEntry( vCex, Entry, i )
//        Sat_ObjSetXValue( Gia_ManObj(p, Abc_Lit2Var(Entry)), Abc_LitIsCompl(Entry)? GIA_ZER : GIA_ONE );
        Sat_ObjSetXValue( Gia_ManCi(p, Abc_Lit2Var(Entry)), Abc_LitIsCompl(Entry)? GIA_ZER : GIA_ONE );
    // simulate
    Gia_ManForEachObjVec( vVisit, p, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
            continue;
        assert( Gia_ObjIsAnd(pObj) );
        Value0 = Sat_ObjXValue( Gia_ObjFanin0(pObj) );
        Value1 = Sat_ObjXValue( Gia_ObjFanin1(pObj) );
        Value = Gia_XsimAndCond( Value0, Gia_ObjFaninC0(pObj), Value1, Gia_ObjFaninC1(pObj) );
        Sat_ObjSetXValue( pObj, Value );
    }
    Value = Sat_ObjXValue( Gia_ObjFanin0(pRoot) );
    Value = Gia_XsimNotCond( Value, Gia_ObjFaninC0(pRoot) );
    if ( Value != GIA_ONE )
        printf( "Gia_SatVerifyPattern(): Verification FAILED.\n" );
//    else
//        printf( "Gia_SatVerifyPattern(): Verification succeeded.\n" );
//    assert( Value == GIA_ONE );
    // clean the nodes
    Gia_ManForEachObjVec( vVisit, p, pObj, i )
        Sat_ObjSetXValue( pObj, 0 );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

