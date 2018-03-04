/**CFile****************************************************************

  FileName    [int2Refine.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Various utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 1, 2013.]

  Revision    [$Id: int2Refine.c,v 1.00 2013/12/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "int2Int.h"

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
void Int2_ManJustify_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vSelect )
{ 
    if ( pObj->fMark1 )
        return;
    pObj->fMark1 = 1;
    if ( Gia_ObjIsPi(p, pObj) )
        return;
    if ( Gia_ObjIsCo(pObj) )
    {
        Vec_IntPush( vSelect, Gia_ObjCioId(pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    if ( pObj->Value == 1 )
    {
        if ( Gia_ObjFanin0(pObj)->Value < ABC_INFINITY )
            Int2_ManJustify_rec( p, Gia_ObjFanin0(pObj), vSelect );
        if ( Gia_ObjFanin1(pObj)->Value < ABC_INFINITY )
            Int2_ManJustify_rec( p, Gia_ObjFanin1(pObj), vSelect );
        return;
    }
    if ( (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) == 0 && (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj)) == 0 )
    {
        if ( Gia_ObjFanin0(pObj)->fMark0 <= Gia_ObjFanin1(pObj)->fMark0 ) // choice
        {
            if ( Gia_ObjFanin0(pObj)->Value < ABC_INFINITY )
                Int2_ManJustify_rec( p, Gia_ObjFanin0(pObj), vSelect );
        }
        else
        {
            if ( Gia_ObjFanin1(pObj)->Value < ABC_INFINITY )
                Int2_ManJustify_rec( p, Gia_ObjFanin1(pObj), vSelect );
        }
    }
    else if ( (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) == 0 )
    {
        if ( Gia_ObjFanin0(pObj)->Value < ABC_INFINITY )
            Int2_ManJustify_rec( p, Gia_ObjFanin0(pObj), vSelect );
    }
    else if ( (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj)) == 0 )
    {
        if ( Gia_ObjFanin1(pObj)->Value < ABC_INFINITY )
            Int2_ManJustify_rec( p, Gia_ObjFanin1(pObj), vSelect );
    }
    else assert( 0 );
 }

/**Function*************************************************************

  Synopsis    [Computes the reduced set of flop variables.]

  Description [Given is a single-output seq AIG manager and an assignment 
  of its CIs.  Returned is a subset of flops that justifies the output.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Int2_ManRefineCube( Gia_Man_t * p, Vec_Int_t * vAssign, Vec_Int_t * vPrio )
{
    Vec_Int_t * vSubset;
    Gia_Obj_t * pObj;
    int i;
    // set values and prios
    assert( Gia_ManRegNum(p) > 0 );
    assert( Vec_IntSize(vAssign) == Vec_IntSize(vPrio) );
    Gia_ManConst0(p)->fMark0 = 0;
    Gia_ManConst0(p)->fMark1 = 0;
    Gia_ManConst0(p)->Value = ABC_INFINITY;
    Gia_ManForEachCi( p, pObj, i )
    {
        pObj->fMark0 = Vec_IntEntry(vAssign, i);
        pObj->fMark1 = 0;
        pObj->Value  = Vec_IntEntry(vPrio, i);        
    }
    Gia_ManForEachAnd( p, pObj, i )
    {
        pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) & (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj));
        pObj->fMark1 = 0;
        if ( pObj->fMark0 == 1 )
            pObj->Value = Abc_MaxInt( Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value );
        else if ( (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) == 0 && (Gia_ObjFanin1(pObj)->fMark0 ^ Gia_ObjFaninC1(pObj)) == 0 )
            pObj->Value = Abc_MinInt( Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value ); // choice
        else if ( (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj)) == 0 )
            pObj->Value = Gia_ObjFanin0(pObj)->Value;
        else 
            pObj->Value = Gia_ObjFanin1(pObj)->Value;
    }
    pObj = Gia_ManPo( p, 0 );
    pObj->fMark0 = (Gia_ObjFanin0(pObj)->fMark0 ^ Gia_ObjFaninC0(pObj));
    pObj->fMark1 = 0;
    pObj->Value  = Gia_ObjFanin0(pObj)->Value;
    assert( pObj->fMark0 == 1 );
    assert( pObj->Value < ABC_INFINITY );
    // select subset
    vSubset = Vec_IntAlloc( 100 );
    Int2_ManJustify_rec( p, Gia_ObjFanin0(pObj), vSubset );
    return vSubset;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

