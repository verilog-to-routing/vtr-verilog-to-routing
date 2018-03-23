/**CFile****************************************************************

  FileName    [giaTis.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Technology independent synthesis.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaTis.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives GIA with MUXes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManTisDupMuxes( Gia_Man_t * p )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj, * pFan0, * pFan1, * pFanC;
    int i;
    assert( p->pMuxes == NULL );
    ABC_FREE( p->pRefs );
    Gia_ManCreateRefs( p ); 
    // start the new manager
    pNew = Gia_ManStart( 5000 );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->pMuxes = ABC_CALLOC( unsigned, pNew->nObjsAlloc );
    // create constant
    Gia_ManConst0(p)->Value = 0;
    // create PIs
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    // create internal nodes
    Gia_ManHashStart( pNew );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !Gia_ObjIsMuxType(pObj) || (Gia_ObjRefNum(p, Gia_ObjFanin0(pObj)) > 1 && Gia_ObjRefNum(p, Gia_ObjFanin1(pObj)) > 1) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
            pObj->Value = Gia_ManHashXorReal( pNew, Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan0)), Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan1)) );
        else
        {
            pFanC = Gia_ObjRecognizeMux( pObj, &pFan1, &pFan0 );
            pObj->Value = Gia_ManHashMuxReal( pNew, Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFanC)), Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan1)), Gia_ObjLitCopy(p, Gia_ObjToLit(p, pFan0)) );
        }
    }
    Gia_ManHashStop( pNew );
    // create ROs
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    // perform cleanup
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTisCollectMffc_rec( Gia_Man_t * p, int Id, Vec_Int_t * vMffc, Vec_Int_t * vLeaves )
{
    Gia_Obj_t * pObj;
    if ( Gia_ObjIsTravIdCurrentId(p, Id) )
        return;
    Gia_ObjSetTravIdCurrentId(p, Id);
    if ( Gia_ObjRefNumId(p, Id) > 1 )
    {
        Vec_IntPush( vLeaves, Id );
        return;
    }
    pObj = Gia_ManObj( p, Id );
    if ( Gia_ObjIsCi(pObj) )
    {
        Vec_IntPush( vLeaves, Id );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManTisCollectMffc_rec( p, Gia_ObjFaninId0(pObj, Id), vMffc, vLeaves );
    Gia_ManTisCollectMffc_rec( p, Gia_ObjFaninId1(pObj, Id), vMffc, vLeaves );
    if ( Gia_ObjIsMuxId(p, Id) )
        Gia_ManTisCollectMffc_rec( p, Gia_ObjFaninId2(p, Id), vMffc, vLeaves );
    Vec_IntPush( vMffc, Id );
}
void Gia_ManTisCollectMffc( Gia_Man_t * p, int Id, Vec_Int_t * vMffc, Vec_Int_t * vLeaves )
{
    Gia_Obj_t * pObj = Gia_ManObj( p, Id );
    assert( Gia_ObjIsAnd(pObj) );
    Vec_IntClear( vMffc );
    Vec_IntClear( vLeaves );
    Gia_ManIncrementTravId( p );
    Gia_ManTisCollectMffc_rec( p, Gia_ObjFaninId0(pObj, Id), vMffc, vLeaves );
    Gia_ManTisCollectMffc_rec( p, Gia_ObjFaninId1(pObj, Id), vMffc, vLeaves );
    if ( Gia_ObjIsMuxId(p, Id) )
        Gia_ManTisCollectMffc_rec( p, Gia_ObjFaninId2(p, Id), vMffc, vLeaves );
    Vec_IntPush( vMffc, Id );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTisPrintMffc( Gia_Man_t * p, int Id, Vec_Int_t * vMffc, Vec_Int_t * vLeaves )
{
    Gia_Obj_t * pObj;
    int i;
    printf( "MFFC %d has %d nodes and %d leaves:\n", Id, Vec_IntSize(vMffc), Vec_IntSize(vLeaves) );
    Gia_ManForEachObjVecReverse( vMffc, p, pObj, i )
    {
        printf( "Node %2d : ", Vec_IntSize(vMffc) - 1 - i );
        Gia_ObjPrint( p, pObj );
    }
    Gia_ManForEachObjVec( vLeaves, p, pObj, i )
    {
        printf( "Leaf %2d : ", i );
        Gia_ObjPrint( p, pObj );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManTisTest( Gia_Man_t * pInit )
{
    Gia_Man_t * p;
    Gia_Obj_t * pObj;
    Vec_Int_t * vMffc, * vLeaves;
    int i;
    vMffc = Vec_IntAlloc( 10 );
    vLeaves = Vec_IntAlloc( 10 );
    p = Gia_ManTisDupMuxes( pInit );
    Gia_ManCreateRefs( p );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Gia_ObjRefNumId(p, i) == 1 )
            continue;
        Gia_ManTisCollectMffc( p, i, vMffc, vLeaves );
        Gia_ManTisPrintMffc( p, i, vMffc, vLeaves );
    }
    Gia_ManForEachCo( p, pObj, i )
    {
        if ( Gia_ObjRefNumId(p, Gia_ObjFaninId0p(p, pObj)) > 1 )
            continue;
        Gia_ManTisCollectMffc( p, Gia_ObjFaninId0p(p, pObj), vMffc, vLeaves );
        Gia_ManTisPrintMffc( p, Gia_ObjFaninId0p(p, pObj), vMffc, vLeaves );
    }
    Gia_ManStop( p );
    Vec_IntFree( vMffc );
    Vec_IntFree( vLeaves );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

