/**CFile****************************************************************

  FileName    [int2Util.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Various utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 1, 2013.]

  Revision    [$Id: int2Util.c,v 1.00 2013/12/01 00:00:00 alanmi Exp $]

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
Vec_Int_t * Int2_ManComputeCoPres( Vec_Int_t * vSop, int nRegs )
{
    Vec_Int_t * vCoPres, * vMap;
    vCoPres = Vec_IntAlloc( 100 );
    if ( vSop == NULL )
        Vec_IntPush( vCoPres, 0 );
    else
    {
        int i, k, Limit;
        vMap = Vec_IntStart( nRegs );
        Vec_IntForEachEntryStart( vSop, Limit, i, 1 )
        {
            for ( k = 0; k < Limit; k++ )
            {
                i++;
                assert( Vec_IntEntry(vSop, i + k) < 2 * nRegs );
                Vec_IntWriteEntry( vMap, Abc_Lit2Var(Vec_IntEntry(vSop, i + k)), 1 );
            }
        }
        Vec_IntForEachEntry( vMap, Limit, i )
            if ( Limit )
                Vec_IntPush( vCoPres, i+1 );
        Vec_IntFree( vMap );
    }
    return vCoPres;
}

void Int2_ManCollectInternal_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes )
{
    if ( Gia_ObjIsTravIdCurrent(p, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(p, pObj);
    if ( Gia_ObjIsCi(pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Int2_ManCollectInternal_rec( p, Gia_ObjFanin0(pObj), vNodes );
    Int2_ManCollectInternal_rec( p, Gia_ObjFanin1(pObj), vNodes );
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );
}
Vec_Int_t * Int2_ManCollectInternal( Gia_Man_t * p, Vec_Int_t * vCoPres )
{
    Vec_Int_t * vNodes;
    Gia_Obj_t * pObj;
    int i, Entry;
    Gia_ManIncrementTravId( p );
    Gia_ObjSetTravIdCurrent(p, Gia_ManConst0(p));
    Gia_ManForEachCi( p, pObj, i )
        Gia_ObjSetTravIdCurrent(p, pObj);
    vNodes = Vec_IntAlloc( 1000 );
    Vec_IntForEachEntry( vCoPres, Entry, i )
        Int2_ManCollectInternal_rec( p, Gia_ObjFanin0(Gia_ManCo(p, Entry)), vNodes );
    return vNodes;
}
Gia_Man_t * Int2_ManProbToGia( Gia_Man_t * p, Vec_Int_t * vSop )
{
    Vec_Int_t * vCoPres, * vNodes;
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i, k, Entry, Limit;
    int Lit, Cube, Sop;
    assert( Gia_ManPoNum(p) == 1 );
    // collect COs and ANDs
    vCoPres = Int2_ManComputeCoPres( vSop, Gia_ManRegNum(p) );
    vNodes = Int2_ManCollectInternal( p, vCoPres );
    // create new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManHashAlloc( pNew );
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Vec_IntForEachEntry( vCoPres, Entry, i )
    {
        pObj = Gia_ManCo(p, Entry);
        pObj->Value = Gia_ObjFanin0Copy( pObj );
    }
    // create additional cubes
    Sop = 0;
    Vec_IntForEachEntryStart( vSop, Limit, i, 1 )
    {
        Cube = 1;
        for ( k = 0; k < Limit; k++ )
        {
            i++;
            Lit = Vec_IntEntry( vSop, i + k );
            pObj = Gia_ManRi( p, Abc_Lit2Var(Lit) );
            Cube = Gia_ManHashAnd( pNew, Cube, Abc_LitNotCond(pObj->Value, Abc_LitIsCompl(Lit)) );
        }
        Sop = Gia_ManHashOr( pNew, Sop, Cube );
    }
    Gia_ManAppendCo( pNew, Sop );
    Gia_ManHashStop( pNew );
    // cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

