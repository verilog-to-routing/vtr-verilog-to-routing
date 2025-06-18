/**CFile****************************************************************

  FileName    [acecUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [CEC for arithmetic circuits.]

  Synopsis    [Various utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: acecUtil.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "acecInt.h"

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
void Gia_PolynCollectXors_rec( Gia_Man_t * pGia, int iObj, Vec_Int_t * vXors )
{
    Gia_Obj_t * pObj = Gia_ManObj( pGia, iObj );
    if ( Gia_ObjIsTravIdCurrent(pGia, pObj) )
        return;
    Gia_ObjSetTravIdCurrent(pGia, pObj);
    if ( !Gia_ObjIsAnd(pObj) || !Gia_ObjIsXor(pObj) || Gia_ObjRefNum(pGia, pObj) > 1 )
        return;
    Gia_PolynCollectXors_rec( pGia, Gia_ObjFaninId0(pObj, iObj), vXors );
    Gia_PolynCollectXors_rec( pGia, Gia_ObjFaninId1(pObj, iObj), vXors );
    Vec_IntPushUnique( vXors, iObj );
}
Vec_Int_t * Gia_PolynCollectLastXor( Gia_Man_t * pGia, int fVerbose )
{
    Vec_Int_t * vXors = Vec_IntAlloc( 100 );
    Gia_Obj_t * pObj = Gia_ManCo( pGia, Gia_ManCoNum(pGia)-1 );
    ABC_FREE( pGia->pRefs );
    Gia_ManCreateRefs( pGia );
    Gia_ManIncrementTravId( pGia );
    Gia_PolynCollectXors_rec( pGia, Gia_ObjFaninId0p(pGia, pObj), vXors );
    Vec_IntReverseOrder( vXors );
    ABC_FREE( pGia->pRefs );
    return vXors;
}
void Gia_PolynAnalyzeXors( Gia_Man_t * pGia, int fVerbose )
{
    int i, iDriver, Count = 0;
    Vec_Int_t * vXors = Vec_IntAlloc( 100 );
    if ( pGia->pMuxes == NULL )
    {
        printf( "AIG does not have XORs extracted.\n" );
        return;
    }
    assert( pGia->pMuxes );
    Gia_ManForEachCoDriverId( pGia, iDriver, i )
    {
        Vec_IntClear( vXors );
        Gia_ManIncrementTravId( pGia );
        Gia_PolynCollectXors_rec( pGia, iDriver, vXors );
        //printf( "%3d : ", i );
        //Vec_IntPrint( vXors );
        printf( "%d=%d  ", i, Vec_IntSize(vXors) );
        Count += Vec_IntSize(vXors);
    }
    printf( "Total = %d.\n", Count );
    Vec_IntFree( vXors );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDupTopMostRange( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Vec_Int_t * vTops = Vec_IntAlloc( 10 );
    int i;
    for ( i = 45; i < 52; i++ )
        Vec_IntPush( vTops, Gia_ObjId( p, Gia_ObjFanin0(Gia_ManCo(p, i)) ) );
    pNew = Gia_ManDupAndConesLimit( p, Vec_IntArray(vTops), Vec_IntSize(vTops), 100 );
    Vec_IntFree( vTops );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

