/**CFile****************************************************************

  FileName    [fsimMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast sequential AIG simulator.]

  Synopsis    [Simulation manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: fsimMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fsimInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates fast simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fsim_ManCreate_rec( Fsim_Man_t * p, Aig_Obj_t * pObj )
{
    int iFan0, iFan1, iTemp;
    assert( !Aig_IsComplement(pObj) );
    if ( pObj->iData )
        return pObj->iData;
    assert( !Aig_ObjIsConst1(pObj) );
    if ( Aig_ObjIsNode(pObj) )
    {
        iFan0 = Fsim_ManCreate_rec( p, Aig_ObjFanin0(pObj) );
        iFan1 = Fsim_ManCreate_rec( p, Aig_ObjFanin1(pObj) );
        assert( iFan0 != iFan1 );
        if ( --p->pRefs[iFan0] == 0 )
            p->nCrossCut--;
        iFan0 = Fsim_Var2Lit( iFan0, Aig_ObjFaninC0(pObj) );
        if ( --p->pRefs[iFan1] == 0 )
            p->nCrossCut--;
        iFan1 = Fsim_Var2Lit( iFan1, Aig_ObjFaninC1(pObj) );
        if ( p->pAig->pEquivs )
            Fsim_ManCreate_rec( p, Aig_ObjEquiv(p->pAig, pObj) );
    }
    else if ( Aig_ObjIsPo(pObj) )
    {
        assert( Aig_ObjRefs(pObj) == 0 );
        iFan0 = Fsim_ManCreate_rec( p, Aig_ObjFanin0(pObj) );
        if ( --p->pRefs[iFan0] == 0 )
            p->nCrossCut--;
        iFan0 = Fsim_Var2Lit( iFan0, Aig_ObjFaninC0(pObj) );
        iFan1 = 0;
    }
    else
    {
        iFan0 = iFan1 = 0;
        Vec_IntPush( p->vCis2Ids, Aig_ObjPioNum(pObj) );
    }
    if ( iFan0 < iFan1 )
        iTemp = iFan0, iFan0 = iFan1, iFan1 = iTemp;
    p->pFans0[p->nObjs] = iFan0;
    p->pFans1[p->nObjs] = iFan1;
    p->pRefs[p->nObjs]  = Aig_ObjRefs(pObj);
    if ( p->pRefs[p->nObjs] )
        if ( p->nCrossCutMax < ++p->nCrossCut ) 
            p->nCrossCutMax = p->nCrossCut;
    return pObj->iData = p->nObjs++;
}

/**Function*************************************************************

  Synopsis    [Creates fast simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fsim_Man_t * Fsim_ManCreate( Aig_Man_t * pAig )
{
    Fsim_Man_t * p;
    Aig_Obj_t * pObj;
    int i, nObjs;
    Aig_ManCleanData( pAig );
    p = (Fsim_Man_t *)ABC_ALLOC( Fsim_Man_t, 1 );
    memset( p, 0, sizeof(Fsim_Man_t) );
    p->pAig = pAig;
    p->nPis = Saig_ManPiNum(pAig);
    p->nPos = Saig_ManPoNum(pAig);
    p->nCis = Aig_ManPiNum(pAig);
    p->nCos = Aig_ManPoNum(pAig);
    p->nNodes = Aig_ManNodeNum(pAig);
    nObjs = p->nCis + p->nCos + p->nNodes + 2;
    p->pFans0 = ABC_ALLOC( int, nObjs );
    p->pFans1 = ABC_ALLOC( int, nObjs );
    p->pRefs  = ABC_ALLOC( int, nObjs );
    p->vCis2Ids = Vec_IntAlloc( Aig_ManPiNum(pAig) );
    // add objects (0=unused; 1=const1)
    p->pFans0[0] = p->pFans1[0] = 0;
    p->pFans0[1] = p->pFans1[1] = 0;
    p->pRefs[0] = 0;
    p->nObjs = 2;
    pObj = Aig_ManConst1( pAig );
    pObj->iData = 1;
    p->pRefs[1] = Aig_ObjRefs(pObj);
    if ( p->pRefs[1] )
        p->nCrossCut = 1;
    Aig_ManForEachPi( pAig, pObj, i )
        if ( Aig_ObjRefs(pObj) == 0 )
            Fsim_ManCreate_rec( p, pObj );
    Aig_ManForEachPo( pAig, pObj, i )
        Fsim_ManCreate_rec( p, pObj );
    assert( Vec_IntSize(p->vCis2Ids) == Aig_ManPiNum(pAig) );
    assert( p->nObjs == nObjs );
    // check references
    assert( p->nCrossCut == 0 );
    Aig_ManForEachObj( pAig, pObj, i )
    {
        assert( p->pRefs[pObj->iData] == 0 );
        p->pRefs[pObj->iData] = Aig_ObjRefs(pObj);
    }
    // collect flop outputs
    p->vLos = Vec_IntAlloc( Aig_ManRegNum(pAig) );
    Saig_ManForEachLo( pAig, pObj, i )
        Vec_IntPush( p->vLos, pObj->iData );
    // collect flop inputs
    p->vLis = Vec_IntAlloc( Aig_ManRegNum(pAig) );
    Saig_ManForEachLi( pAig, pObj, i )
        Vec_IntPush( p->vLis, pObj->iData );
    // determine the frontier size
    p->nFront = 1 + (int)(1.1 * p->nCrossCutMax); 
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes fast simulation manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fsim_ManDelete( Fsim_Man_t * p )
{
    Vec_IntFree( p->vCis2Ids );
    Vec_IntFree( p->vLos );
    Vec_IntFree( p->vLis );
    ABC_FREE( p->pDataAig2 );
    ABC_FREE( p->pDataAig );
    ABC_FREE( p->pFans0 );
    ABC_FREE( p->pFans1 );
    ABC_FREE( p->pRefs );
    ABC_FREE( p->pDataSim );
    ABC_FREE( p->pDataSimCis );
    ABC_FREE( p->pDataSimCos );
    ABC_FREE( p->pData1 );
    ABC_FREE( p->pData2 );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Testing procedure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fsim_ManTest( Aig_Man_t * pAig )
{
    Fsim_Man_t * p;
    p = Fsim_ManCreate( pAig );
    Fsim_ManFront( p, 0 );
    Fsim_ManDelete( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

