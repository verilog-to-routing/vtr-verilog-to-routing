/**CFile****************************************************************

  FileName    [wlcWin.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Parses several flavors of word-level Verilog.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 22, 2014.]

  Revision    [$Id: wlcWin.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "wlc.h"
#include "base/abc/abc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Collect arithmetic nodes.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Wlc_ObjIsArithm( Wlc_Obj_t * pObj )
{
    return pObj->Type == WLC_OBJ_CONST       || 
           pObj->Type == WLC_OBJ_BUF         || pObj->Type == WLC_OBJ_BIT_NOT     ||
           pObj->Type == WLC_OBJ_BIT_ZEROPAD || pObj->Type == WLC_OBJ_BIT_SIGNEXT ||
//           pObj->Type == WLC_OBJ_BIT_SELECT  || pObj->Type == WLC_OBJ_BIT_CONCAT  ||
           pObj->Type == WLC_OBJ_ARI_ADD     || pObj->Type == WLC_OBJ_ARI_SUB     || 
           pObj->Type == WLC_OBJ_ARI_MULTI   || pObj->Type == WLC_OBJ_ARI_MINUS;
}
int Wlc_ObjIsArithmReal( Wlc_Obj_t * pObj )
{
    return pObj->Type == WLC_OBJ_BIT_NOT     ||
           pObj->Type == WLC_OBJ_ARI_ADD     || pObj->Type == WLC_OBJ_ARI_SUB     || 
           pObj->Type == WLC_OBJ_ARI_MULTI   || pObj->Type == WLC_OBJ_ARI_MINUS;
}
int Wlc_ManCountArithmReal( Wlc_Ntk_t * p, Vec_Int_t * vNodes )
{
    Wlc_Obj_t * pObj; 
    int i, Counter = 0;
    Wlc_NtkForEachObjVec( vNodes, p, pObj, i )
        Counter += Wlc_ObjIsArithmReal( pObj );
    return Counter;
}
int Wlc_ObjHasArithm_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )
{
    if ( pObj->Type == WLC_OBJ_CONST )
        return 0;
    if ( pObj->Type == WLC_OBJ_BUF         || pObj->Type == WLC_OBJ_BIT_NOT ||
         pObj->Type == WLC_OBJ_BIT_ZEROPAD || pObj->Type == WLC_OBJ_BIT_SIGNEXT )
         return Wlc_ObjHasArithm_rec( p, Wlc_ObjFanin0(p, pObj) );
    return pObj->Type == WLC_OBJ_ARI_ADD   || pObj->Type == WLC_OBJ_ARI_SUB || 
           pObj->Type == WLC_OBJ_ARI_MULTI || pObj->Type == WLC_OBJ_ARI_MINUS;
}
int Wlc_ObjHasArithmFanins( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )
{
    Wlc_Obj_t * pFanin;  int i;
    assert( !Wlc_ObjHasArithm_rec(p, pObj) );
    Wlc_ObjForEachFaninObj( p, pObj, pFanin, i )
        if ( Wlc_ObjHasArithm_rec(p, pFanin) )
            return 1;
    return 0;
}
void Wlc_WinCompute_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Vec_Int_t * vLeaves, Vec_Int_t * vNodes )
{
    Wlc_Obj_t * pFanin;  int i;
    if ( pObj->Mark )
        return;
    pObj->Mark = 1;
    if ( !Wlc_ObjIsArithm(pObj) )
    {
        Vec_IntPush( vLeaves, Wlc_ObjId(p, pObj) );
        return;
    }
    Wlc_ObjForEachFaninObj( p, pObj, pFanin, i )
        Wlc_WinCompute_rec( p, pFanin, vLeaves, vNodes );
    Vec_IntPush( vNodes, Wlc_ObjId(p, pObj) );
}
void Wlc_WinCleanMark_rec( Wlc_Ntk_t * p, Wlc_Obj_t * pObj )
{
    Wlc_Obj_t * pFanin;  int i;
    if ( !pObj->Mark )
        return;
    pObj->Mark = 0;
    Wlc_ObjForEachFaninObj( p, pObj, pFanin, i )
        Wlc_WinCleanMark_rec( p, pFanin );
}
void Wlc_WinCompute( Wlc_Ntk_t * p, Wlc_Obj_t * pObj, Vec_Int_t * vLeaves, Vec_Int_t * vNodes )
{
    Vec_IntClear( vLeaves );
    Vec_IntClear( vNodes );
    if ( Wlc_ObjHasArithm_rec(p, pObj) )
    {
        Wlc_WinCompute_rec( p, pObj, vLeaves, vNodes );
        Wlc_WinCleanMark_rec( p, pObj );
    }
    else if ( Wlc_ObjHasArithmFanins(p, pObj) )
    {
        Wlc_Obj_t * pFanin;  int i;
        Wlc_ObjForEachFaninObj( p, pObj, pFanin, i )
            if ( Wlc_ObjHasArithm_rec(p, pFanin) )
                Wlc_WinCompute_rec( p, pFanin, vLeaves, vNodes );
        Wlc_ObjForEachFaninObj( p, pObj, pFanin, i )
            if ( Wlc_ObjHasArithm_rec(p, pFanin) )
                Wlc_WinCleanMark_rec( p, pFanin );
    }
    else assert( 0 );
}
void Wlc_WinProfileArith( Wlc_Ntk_t * p )
{
    Vec_Int_t * vLeaves = Vec_IntAlloc( 1000 );
    Vec_Int_t * vNodes  = Vec_IntAlloc( 1000 );
    Wlc_Obj_t * pObj; int i, Count = 0;
    Wlc_NtkForEachObj( p, pObj, i )
        pObj->Mark = 0;
    Wlc_NtkForEachObj( p, pObj, i )
        if ( Wlc_ObjHasArithm_rec(p, pObj) ? Wlc_ObjIsCo(pObj) : Wlc_ObjHasArithmFanins(p, pObj) )
        {
            Wlc_WinCompute( p, pObj, vLeaves, vNodes ); 
            if ( Wlc_ManCountArithmReal(p, vNodes) < 2 )
                continue;

            printf( "Arithmetic cone of node %d (%s):\n", Wlc_ObjId(p, pObj), Wlc_ObjName(p, Wlc_ObjId(p, pObj)) );
            Wlc_NtkPrintNode( p, pObj );
            Vec_IntReverseOrder( vNodes );
            Wlc_NtkPrintNodeArray( p, vNodes );
            printf( "\n" );
            Count++;
        }
    Wlc_NtkForEachObj( p, pObj, i )
        assert( pObj->Mark == 0 );
    printf( "Finished printing %d arithmetic cones.\n", Count );
    Vec_IntFree( vLeaves );
    Vec_IntFree( vNodes );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

