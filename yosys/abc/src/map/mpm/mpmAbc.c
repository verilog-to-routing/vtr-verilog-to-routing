/**CFile****************************************************************

  FileName    [mpmAbc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Configurable technology mapper.]

  Synopsis    [Interface with ABC data structures.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 1, 2013.]

  Revision    [$Id: mpmAbc.c,v 1.00 2013/06/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig/gia/gia.h"
#include "mpmInt.h"

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
void Mig_ManCreateChoices( Mig_Man_t * pMig, Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    assert( Mig_ManObjNum(pMig) == Gia_ManObjNum(p) );
    assert( Vec_IntSize(&pMig->vSibls) == 0 );
    Vec_IntFill( &pMig->vSibls, Gia_ManObjNum(p), 0 );
    Gia_ManMarkFanoutDrivers( p );
    Gia_ManForEachObj( p, pObj, i )
    {
        Gia_ObjSetPhase( p, pObj );
        assert( Abc_Lit2Var(pObj->Value) == i );
        Mig_ObjSetPhase( Mig_ManObj(pMig, i), pObj->fPhase );
        if ( Gia_ObjSibl(p, i) && pObj->fMark0 )
        {
            Gia_Obj_t * pSibl, * pPrev;
            for ( pPrev = pObj, pSibl = Gia_ObjSiblObj(p, i); pSibl; pPrev = pSibl, pSibl = Gia_ObjSiblObj(p, Gia_ObjId(p, pSibl)) )
                Mig_ObjSetSiblId( Mig_ManObj(pMig, Abc_Lit2Var(pPrev->Value)), Abc_Lit2Var(pSibl->Value) );
            pMig->nChoices++;
        }
    }
    Gia_ManCleanMark0( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Mig_ObjFanin0Copy( Gia_Obj_t * pObj ) { return Abc_LitNotCond( Gia_ObjFanin0(pObj)->Value, Gia_ObjFaninC0(pObj) );     }
static inline int Mig_ObjFanin1Copy( Gia_Obj_t * pObj ) { return Abc_LitNotCond( Gia_ObjFanin1(pObj)->Value, Gia_ObjFaninC1(pObj) );     }
Mig_Man_t * Mig_ManCreate( void * pGia )
{
    Gia_Man_t * p = (Gia_Man_t *)pGia;
    Mig_Man_t * pNew;
    Gia_Obj_t * pObj;
    int i;
    pNew = Mig_ManStart();
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsMuxId(p, i) )
            pObj->Value = Mig_ManAppendMux( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj), Gia_ObjFanin2Copy(p, pObj) );
        else if ( Gia_ObjIsXor(pObj) )
            pObj->Value = Mig_ManAppendXor( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Mig_ManAppendAnd( pNew, Mig_ObjFanin0Copy(pObj), Mig_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Mig_ManAppendCi( pNew );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Mig_ManAppendCo( pNew, Mig_ObjFanin0Copy(pObj) );
        else assert( 0 );
    }
    Mig_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    if ( Gia_ManHasChoices(p) )
        Mig_ManCreateChoices( pNew, p );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Recursively derives the local AIG for the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Mpm_CutDataInt( Mpm_Cut_t * pCut )                    { return pCut->hNext;  }
static inline void     Mpm_CutSetDataInt( Mpm_Cut_t * pCut, int Data )       { pCut->hNext = Data;  }
int Mpm_ManNodeIfToGia_rec( Gia_Man_t * pNew, Mpm_Man_t * pMan, Mig_Obj_t * pObj, Vec_Ptr_t * vVisited, int fHash )
{
    Mig_Obj_t * pTemp;
    Mpm_Cut_t * pCut;
    int iFunc, iFunc0, iFunc1, iFunc2 = 0;
    assert( fHash == 0 );
    // get the best cut
    pCut = Mpm_ObjCutBestP( pMan, pObj );
    // if the cut is visited, return the result
    if ( Mpm_CutDataInt(pCut) )
        return Mpm_CutDataInt(pCut);
    // mark the node as visited
    Vec_PtrPush( vVisited, pCut );
    // insert the worst case
    Mpm_CutSetDataInt( pCut, ~0 );
    // skip in case of primary input
    if ( Mig_ObjIsCi(pObj) )
        return Mpm_CutDataInt(pCut);
    // compute the functions of the children
    for ( pTemp = pObj; pTemp; pTemp = Mig_ObjSibl(pTemp) )
    {
        iFunc0 = Mpm_ManNodeIfToGia_rec( pNew, pMan, Mig_ObjFanin0(pTemp), vVisited, fHash );
        if ( iFunc0 == ~0 )
            continue;
        iFunc1 = Mpm_ManNodeIfToGia_rec( pNew, pMan, Mig_ObjFanin1(pTemp), vVisited, fHash );
        if ( iFunc1 == ~0 )
            continue;
        if ( Mig_ObjIsNode3(pTemp) )
        {
            iFunc2 = Mpm_ManNodeIfToGia_rec( pNew, pMan, Mig_ObjFanin2(pTemp), vVisited, fHash );
            if ( iFunc2 == ~0 )
                continue;
            iFunc2 = Abc_LitNotCond(iFunc2, Mig_ObjFaninC2(pTemp));
        }
        iFunc0 = Abc_LitNotCond(iFunc0, Mig_ObjFaninC0(pTemp));
        iFunc1 = Abc_LitNotCond(iFunc1, Mig_ObjFaninC1(pTemp));
        // both branches are solved
        if ( fHash )
        {
            if ( Mig_ObjIsMux(pTemp) )
                iFunc = Gia_ManHashMux( pNew, iFunc2, iFunc1, iFunc0 );
            else if ( Mig_ObjIsXor(pTemp) )
                iFunc = Gia_ManHashXor( pNew, iFunc0, iFunc1 );
            else 
                iFunc = Gia_ManHashAnd( pNew, iFunc0, iFunc1 ); 
        }
        else
        {
            if ( Mig_ObjIsMux(pTemp) )
                iFunc = Gia_ManAppendMux( pNew, iFunc2, iFunc1, iFunc0 );
            else if ( Mig_ObjIsXor(pTemp) )
                iFunc = Gia_ManAppendXor( pNew, iFunc0, iFunc1 );
            else 
                iFunc = Gia_ManAppendAnd( pNew, iFunc0, iFunc1 ); 
        }
        if ( Mig_ObjPhase(pTemp) != Mig_ObjPhase(pObj) )
            iFunc = Abc_LitNot(iFunc);
        Mpm_CutSetDataInt( pCut, iFunc );
        break;
    }
    return Mpm_CutDataInt(pCut);
}
int Mpm_ManNodeIfToGia( Gia_Man_t * pNew, Mpm_Man_t * pMan, Mig_Obj_t * pObj, Vec_Int_t * vLeaves, int fHash )
{
    Mpm_Cut_t * pCut;
    Mig_Obj_t * pFanin;
    int i, iRes;
    // get the best cut
    pCut = Mpm_ObjCutBestP( pMan, pObj );
    assert( pCut->nLeaves > 1 );
    // set the leaf variables
    Mpm_CutForEachLeaf( pMan->pMig, pCut, pFanin, i )
        Mpm_CutSetDataInt( Mpm_ObjCutBestP(pMan, pFanin), Vec_IntEntry(vLeaves, i) );
    // recursively compute the function while collecting visited cuts
    Vec_PtrClear( pMan->vTemp );
    iRes = Mpm_ManNodeIfToGia_rec( pNew, pMan, pObj, pMan->vTemp, fHash ); 
    if ( iRes == ~0 )
    {
        Abc_Print( -1, "Mpm_ManNodeIfToGia(): Computing local AIG has failed.\n" );
        return ~0;
    }
    // clean the cuts
    Mpm_CutForEachLeaf( pMan->pMig, pCut, pFanin, i )
        Mpm_CutSetDataInt( Mpm_ObjCutBestP(pMan, pFanin), 0 );
    Vec_PtrForEachEntry( Mpm_Cut_t *, pMan->vTemp, pCut, i )
        Mpm_CutSetDataInt( pCut, 0 );
    return iRes;
}
void * Mpm_ManFromIfLogic( Mpm_Man_t * pMan )
{
    Gia_Man_t * pNew;
    Mpm_Cut_t * pCutBest;
    Mig_Obj_t * pObj, * pFanin;
    Vec_Int_t * vMapping, * vMapping2, * vPacking = NULL;
    Vec_Int_t * vLeaves, * vLeaves2, * vCover;
    word uTruth, * pTruth = &uTruth;
    int i, k, Entry, iLitNew = 0;
//    assert( !pMan->pPars->fDeriveLuts || pMan->pPars->fTruth );
    // start mapping and packing
    vMapping  = Vec_IntStart( Mig_ManObjNum(pMan->pMig) );
    vMapping2 = Vec_IntStart( 1 );
    if ( 0 ) // pMan->pPars->fDeriveLuts && pMan->pPars->pLutStruct )
    {
        vPacking = Vec_IntAlloc( 1000 );
        Vec_IntPush( vPacking, 0 );
    }
    // create new manager
    pNew = Gia_ManStart( Mig_ManObjNum(pMan->pMig) );
    // iterate through nodes used in the mapping
    vCover   = Vec_IntAlloc( 1 << 16 );
    vLeaves  = Vec_IntAlloc( 16 );
    vLeaves2 = Vec_IntAlloc( 16 );
    Mig_ManCleanCopy( pMan->pMig );
    Mig_ManForEachObj( pMan->pMig, pObj )
    {
        if ( !Mpm_ObjMapRef(pMan, pObj) && !Mig_ObjIsTerm(pObj) )
            continue;
        if ( Mig_ObjIsNode(pObj) )
        {
            // collect leaves of the best cut
            Vec_IntClear( vLeaves );
            pCutBest = Mpm_ObjCutBestP( pMan, pObj );
            Mpm_CutForEachLeaf( pMan->pMig, pCutBest, pFanin, k )
                Vec_IntPush( vLeaves, Mig_ObjCopy(pFanin) );
            if ( pMan->pPars->fDeriveLuts && (pMan->pPars->fUseTruth || pMan->pPars->fUseDsd) )
            {
                extern int Gia_ManFromIfLogicNode( void * p, Gia_Man_t * pNew, int iObj, Vec_Int_t * vLeaves, Vec_Int_t * vLeavesTemp, 
                    word * pRes, char * pStr, Vec_Int_t * vCover, Vec_Int_t * vMapping, Vec_Int_t * vMapping2, Vec_Int_t * vPacking, int fCheck75, int fCheck44e );
                if ( pMan->pPars->fUseTruth )
                    pTruth = Mpm_CutTruth(pMan, Abc_Lit2Var(pCutBest->iFunc));
                else
                    uTruth = Mpm_CutTruthFromDsd( pMan, pCutBest, Abc_Lit2Var(pCutBest->iFunc) );
//                Kit_DsdPrintFromTruth( pTruth, Vec_IntSize(vLeaves) );  printf( "\n" );
                // perform decomposition of the cut
                iLitNew = Gia_ManFromIfLogicNode( NULL, pNew, Mig_ObjId(pObj), vLeaves, vLeaves2, pTruth, NULL, vCover, vMapping, vMapping2, vPacking, 0, 0 );
                iLitNew = Abc_LitNotCond( iLitNew, pCutBest->fCompl ^ Abc_LitIsCompl(pCutBest->iFunc) );
            }
            else
            {
                // perform one of the two types of mapping: with and without structures
                iLitNew = Mpm_ManNodeIfToGia( pNew, pMan, pObj, vLeaves, 0 );
                // write mapping
                Vec_IntSetEntry( vMapping, Abc_Lit2Var(iLitNew), Vec_IntSize(vMapping2) );
                Vec_IntPush( vMapping2, Vec_IntSize(vLeaves) );
                Vec_IntForEachEntry( vLeaves, Entry, k )
                    assert( Abc_Lit2Var(Entry) < Abc_Lit2Var(iLitNew) );
                Vec_IntForEachEntry( vLeaves, Entry, k )
                    Vec_IntPush( vMapping2, Abc_Lit2Var(Entry)  );
                Vec_IntPush( vMapping2, Abc_Lit2Var(iLitNew) );
            }
        }
        else if ( Mig_ObjIsCi(pObj) )
            iLitNew = Gia_ManAppendCi(pNew);
        else if ( Mig_ObjIsCo(pObj) )
            iLitNew = Gia_ManAppendCo( pNew, Abc_LitNotCond(Mig_ObjCopy(Mig_ObjFanin0(pObj)), Mig_ObjFaninC0(pObj)) );
        else if ( Mig_ObjIsConst0(pObj) )
        {
            iLitNew = 0;
            // create const LUT
            Vec_IntWriteEntry( vMapping, 0, Vec_IntSize(vMapping2) );
            Vec_IntPush( vMapping2, 0 );
            Vec_IntPush( vMapping2, 0 );
        }
        else assert( 0 );
        Mig_ObjSetCopy( pObj, iLitNew );
    }
    Vec_IntFree( vCover );
    Vec_IntFree( vLeaves );
    Vec_IntFree( vLeaves2 );
//    printf( "Mapping array size:  IfMan = %d. Gia = %d. Increase = %.2f\n", 
//        Mig_ManObjNum(pMan), Gia_ManObjNum(pNew), 1.0 * Gia_ManObjNum(pNew) / Mig_ManObjNum(pMan) );
    // finish mapping 
    if ( Vec_IntSize(vMapping) > Gia_ManObjNum(pNew) )
        Vec_IntShrink( vMapping, Gia_ManObjNum(pNew) );
    else
        Vec_IntFillExtra( vMapping, Gia_ManObjNum(pNew), 0 );
    assert( Vec_IntSize(vMapping) == Gia_ManObjNum(pNew) );
    Vec_IntForEachEntry( vMapping, Entry, i )
        if ( Entry > 0 )
            Vec_IntAddToEntry( vMapping, i, Gia_ManObjNum(pNew) );
    Vec_IntAppend( vMapping, vMapping2 );
    Vec_IntFree( vMapping2 );
    // attach mapping and packing
    assert( pNew->vMapping == NULL );
    assert( pNew->vPacking == NULL );
    pNew->vMapping = vMapping;
    pNew->vPacking = vPacking;
    // verify that COs have mapping
    {
        Gia_Obj_t * pObj;
        Gia_ManForEachCo( pNew, pObj, i )
           assert( !Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) || Gia_ObjIsLut(pNew, Gia_ObjFaninId0p(pNew, pObj)) );
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
void Mig_ManTest2( Gia_Man_t * pGia )
{
    extern int Gia_ManSuppSizeTest( Gia_Man_t * p );
    Mig_Man_t * p;
    Gia_ManSuppSizeTest( pGia );
    p = Mig_ManCreate( pGia );
    Mig_ManSuppSizeTest( p );
    Mig_ManStop( p );
}
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

