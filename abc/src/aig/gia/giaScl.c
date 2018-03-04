/**CFile****************************************************************

  FileName    [giaScl.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Sequential cleanup.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaScl.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

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

  Synopsis    [Marks unreachable internal nodes and returns their number.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManCombMarkUsed_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( pObj == NULL )
        return 0;
    if ( !pObj->fMark0 )
        return 0;
    pObj->fMark0 = 0;
    assert( Gia_ObjIsAnd(pObj) );
    assert( !Gia_ObjIsBuf(pObj) );
    return 1 + Gia_ManCombMarkUsed_rec( p, Gia_ObjFanin0(pObj) )
             + Gia_ManCombMarkUsed_rec( p, Gia_ObjFanin1(pObj) )
             + (p->pNexts ? Gia_ManCombMarkUsed_rec( p, Gia_ObjNextObj(p, Gia_ObjId(p, pObj)) ) : 0)
             + (p->pSibls ? Gia_ManCombMarkUsed_rec( p, Gia_ObjSiblObj(p, Gia_ObjId(p, pObj)) ) : 0)
             + (p->pMuxes ? Gia_ManCombMarkUsed_rec( p, Gia_ObjFanin2(p, pObj) ) : 0);
}
int Gia_ManCombMarkUsed( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, nNodes = 0;
    Gia_ManForEachObj( p, pObj, i )
        pObj->fMark0 = Gia_ObjIsAnd(pObj) && !Gia_ObjIsBuf(pObj);
    Gia_ManForEachBuf( p, pObj, i )
        nNodes += Gia_ManCombMarkUsed_rec( p, Gia_ObjFanin0(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        nNodes += Gia_ManCombMarkUsed_rec( p, Gia_ObjFanin0(pObj) );
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Performs combinational cleanup.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManCleanup( Gia_Man_t * p )
{
    Gia_ManCombMarkUsed( p );
    return Gia_ManDupMarked( p );
}

/**Function*************************************************************

  Synopsis    [Skip the first outputs during cleanup.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManCleanupOutputs( Gia_Man_t * p, int nOutputs )
{
    Gia_Obj_t * pObj;
    int i;
    assert( Gia_ManRegNum(p) == 0 );
    assert( nOutputs < Gia_ManCoNum(p) );
    Gia_ManCombMarkUsed( p );
    Gia_ManForEachCo( p, pObj, i )
        if ( i < nOutputs )
            pObj->fMark0 = 1;
        else
            break;
    return Gia_ManDupMarked( p );
}


/**Function*************************************************************

  Synopsis    [Marks CIs/COs/ANDs unreachable from POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSeqMarkUsed_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vRoots )
{
    if ( !pObj->fMark0 )
        return 0;
    pObj->fMark0 = 0;
    if ( Gia_ObjIsCo(pObj) )
        return Gia_ManSeqMarkUsed_rec( p, Gia_ObjFanin0(pObj), vRoots );
    if ( Gia_ObjIsRo(p, pObj) )
    {
        Vec_IntPush( vRoots, Gia_ObjId(p, Gia_ObjRoToRi(p, pObj)) );
        return 0;
    }
    assert( Gia_ObjIsAnd(pObj) );
    return 1 + Gia_ManSeqMarkUsed_rec( p, Gia_ObjFanin0(pObj), vRoots )
             + Gia_ManSeqMarkUsed_rec( p, Gia_ObjFanin1(pObj), vRoots );
}

/**Function*************************************************************

  Synopsis    [Marks CIs/COs/ANDs unreachable from POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSeqMarkUsed( Gia_Man_t * p )
{
    Vec_Int_t * vRoots;
    Gia_Obj_t * pObj;
    int i, nNodes = 0;
    Gia_ManSetMark0( p );
    Gia_ManConst0(p)->fMark0 = 0;
    Gia_ManForEachPi( p, pObj, i )
        pObj->fMark0 = 0;
    vRoots = Gia_ManCollectPoIds( p );
    Gia_ManForEachObjVec( vRoots, p, pObj, i )
        nNodes += Gia_ManSeqMarkUsed_rec( p, pObj, vRoots );
    Vec_IntFree( vRoots );
    return nNodes;
}

/**Function*************************************************************

  Synopsis    [Performs sequential cleanup.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManSeqCleanup( Gia_Man_t * p )
{
    Gia_ManSeqMarkUsed( p );
    return Gia_ManDupMarked( p );
}

/**Function*************************************************************

  Synopsis    [Find representatives due to identical fanins.]

  Description [Returns the old manager if there is no changes.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManReduceEquiv( Gia_Man_t * p, int fVerbose )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObjRi, * pObjRo;
    unsigned * pCi2Lit, * pMaps;
    int i, iLit, nFanins = 1, Counter0 = 0, Counter = 0;
    Gia_ManForEachRi( p, pObjRi, i )
        Gia_ObjFanin0(pObjRi)->Value = 0;
    Gia_ManForEachRi( p, pObjRi, i )
        if ( Gia_ObjFanin0(pObjRi)->Value == 0 )
            Gia_ObjFanin0(pObjRi)->Value = 2*nFanins++;
    pCi2Lit = ABC_FALLOC( unsigned, Gia_ManCiNum(p) );
    pMaps   = ABC_FALLOC( unsigned, 2 * nFanins );
    Gia_ManForEachRiRo( p, pObjRi, pObjRo, i )
    {
        iLit = Gia_ObjFanin0Copy( pObjRi );
        if ( Gia_ObjFaninId0p(p, pObjRi) == 0 && Gia_ObjFaninC0(pObjRi) == 0 ) // const 0 
            pCi2Lit[Gia_ManPiNum(p)+i] = 0, Counter0++;
        else if ( ~pMaps[iLit] ) // in this case, ID(pObj) > ID(pRepr) 
            pCi2Lit[Gia_ManPiNum(p)+i] = pMaps[iLit], Counter++; 
        else
            pMaps[iLit] = Abc_Var2Lit( Gia_ObjId(p, pObjRo), 0 );
    }
/*
    Gia_ManForEachCi( p, pObjRo, i )
    {
        if ( ~pCi2Lit[i] )
        {
            Gia_Obj_t * pObj0 = Gia_ObjRoToRi(p, pObjRo);
            Gia_Obj_t * pObj1 = Gia_ObjRoToRi(p, Gia_ManObj(p, pCi2Lit[i]));
            Gia_Obj_t * pFan0 = Gia_ObjChild0( p, Gia_ObjRoToRi(p, pObjRo) );
            Gia_Obj_t * pFan1 = Gia_ObjChild0( p, Gia_ObjRoToRi(p, Gia_ManObj(p, pCi2Lit[i])) );
            assert( pFan0 == pFan1 );
        }
    }
*/
//    if ( fVerbose )
//        printf( "ReduceEquiv detected %d constant regs and %d equivalent regs.\n", Counter0, Counter );
    ABC_FREE( pMaps );
    if ( Counter0 || Counter )
        pNew = Gia_ManDupDfsCiMap( p, (int *)pCi2Lit, NULL );
    else
        pNew = p;
    ABC_FREE( pCi2Lit );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Performs sequential cleanup.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManSeqStructSweep( Gia_Man_t * p, int fConst, int fEquiv, int fVerbose )
{
    Gia_Man_t * pTemp;
    if ( Gia_ManRegNum(p) == 0 )
        return Gia_ManCleanup( p );
    if ( fVerbose )
        printf( "Performing sequential cleanup.\n" );
    p = Gia_ManSeqCleanup( pTemp = p );
    if ( fVerbose )
        Gia_ManReportImprovement( pTemp, p );
    if ( fConst && Gia_ManRegNum(p) )
    {
        p = Gia_ManReduceConst( pTemp = p, fVerbose );
        if ( fVerbose )
            Gia_ManReportImprovement( pTemp, p );
        Gia_ManStop( pTemp );
    }
    if ( fVerbose && fEquiv )
        printf( "Merging combinationally equivalent flops.\n" );
    if ( fEquiv )
    while ( 1 )
    {
        p = Gia_ManSeqCleanup( pTemp = p );
        if ( fVerbose )
            Gia_ManReportImprovement( pTemp, p );
        Gia_ManStop( pTemp );
        if ( Gia_ManRegNum(p) == 0 )
            break;
        p = Gia_ManReduceEquiv( pTemp = p, fVerbose );
        if ( p == pTemp )
            break;
        Gia_ManStop( pTemp );
    }
    return p;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

