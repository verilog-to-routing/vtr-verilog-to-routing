/**CFile****************************************************************

  FileName    [lpkAbcDec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast Boolean matching for LUT structures.]

  Synopsis    [The new core procedure.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: lpkAbcDec.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/
 
#include "lpkInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Implements the function.]

  Description [Returns the node implementing this function.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Lpk_ImplementFun( Lpk_Man_t * pMan, Abc_Ntk_t * pNtk, Vec_Ptr_t * vLeaves, Lpk_Fun_t * p )
{
    extern Hop_Obj_t * Kit_TruthToHop( Hop_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory );
    unsigned * pTruth;
    Abc_Obj_t * pObjNew;
    int i;
    if ( p->fMark )
        pMan->nMuxes++;
    else
        pMan->nDsds++;
    // create the new node
    pObjNew = Abc_NtkCreateNode( pNtk );
    for ( i = 0; i < (int)p->nVars; i++ )
        Abc_ObjAddFanin( pObjNew, Abc_ObjRegular((Abc_Obj_t *)Vec_PtrEntry(vLeaves, p->pFanins[i])) );
    Abc_ObjSetLevel( pObjNew, Abc_ObjLevelNew(pObjNew) );
    // assign the node's function
    pTruth = Lpk_FunTruth(p, 0);
    if ( p->nVars == 0 )
    {
        pObjNew->pData = Hop_NotCond( Hop_ManConst1((Hop_Man_t *)pNtk->pManFunc), !(pTruth[0] & 1) );
        return pObjNew;
    }
    if ( p->nVars == 1 )
    {
        pObjNew->pData = Hop_NotCond( Hop_ManPi((Hop_Man_t *)pNtk->pManFunc, 0), (pTruth[0] & 1) );
        return pObjNew;
    }
    // create the logic function
    pObjNew->pData = Kit_TruthToHop( (Hop_Man_t *)pNtk->pManFunc, pTruth, p->nVars, NULL );
    return pObjNew;
}

/**Function*************************************************************

  Synopsis    [Implements the function.]

  Description [Returns the node implementing this function.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Lpk_Implement_rec( Lpk_Man_t * pMan, Abc_Ntk_t * pNtk, Vec_Ptr_t * vLeaves, Lpk_Fun_t * pFun )
{
    Abc_Obj_t * pFanin, * pRes;
    int i;
    // prepare the leaves of the function
    for ( i = 0; i < (int)pFun->nVars; i++ )
    {
        pFanin = (Abc_Obj_t *)Vec_PtrEntry( vLeaves, pFun->pFanins[i] );
        if ( !Abc_ObjIsComplement(pFanin) )
            Lpk_Implement_rec( pMan, pNtk, vLeaves, (Lpk_Fun_t *)pFanin );
        pFanin = (Abc_Obj_t *)Vec_PtrEntry( vLeaves, pFun->pFanins[i] );
        assert( Abc_ObjIsComplement(pFanin) );
    }
    // construct the function
    pRes = Lpk_ImplementFun( pMan, pNtk, vLeaves, pFun );
    // replace the function
    Vec_PtrWriteEntry( vLeaves, pFun->Id, Abc_ObjNot(pRes) );
    Lpk_FunFree( pFun );
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Implements the function.]

  Description [Returns the node implementing this function.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Lpk_Implement( Lpk_Man_t * pMan, Abc_Ntk_t * pNtk, Vec_Ptr_t * vLeaves, int nLeavesOld )
{
    Abc_Obj_t * pFanin, * pRes;
    int i;
    assert( nLeavesOld < Vec_PtrSize(vLeaves) );
    // mark implemented nodes
    Vec_PtrForEachEntryStop( Abc_Obj_t *, vLeaves, pFanin, i, nLeavesOld )
        Vec_PtrWriteEntry( vLeaves, i, Abc_ObjNot(pFanin) );
    // recursively construct starting from the first entry
    pRes = Lpk_Implement_rec( pMan, pNtk, vLeaves, (Lpk_Fun_t *)Vec_PtrEntry( vLeaves, nLeavesOld ) );
    Vec_PtrShrink( vLeaves, nLeavesOld );
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Decomposes the function using recursive MUX decomposition.]

  Description [Returns the ID of the top-most decomposition node 
  implementing this function, or 0 if there is no decomposition satisfying
  the constraints on area and delay.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_Decompose_rec( Lpk_Man_t * pMan, Lpk_Fun_t * p )
{
    Lpk_Res_t * pResMux, * pResDsd;
    Lpk_Fun_t * p2;
    abctime clk;

    // is only called for non-trivial blocks
    assert( p->nLutK >= 3 && p->nLutK <= 6 );
    assert( p->nVars > p->nLutK );
    // skip if area bound is exceeded
    if ( Lpk_LutNumLuts(p->nVars, p->nLutK) > (int)p->nAreaLim )
        return 0;
    // skip if delay bound is exceeded
    if ( Lpk_SuppDelay(p->uSupp, p->pDelays) > (int)p->nDelayLim )
        return 0;

    // compute supports if needed
    if ( !p->fSupports )
        Lpk_FunComputeCofSupps( p );

    // check DSD decomposition
clk = Abc_Clock();
    pResDsd = Lpk_DsdAnalize( pMan, p, pMan->pPars->nVarsShared );
pMan->timeEvalDsdAn += Abc_Clock() - clk;
    if ( pResDsd && (pResDsd->nBSVars == (int)p->nLutK || pResDsd->nBSVars == (int)p->nLutK - 1) && 
          pResDsd->AreaEst <= (int)p->nAreaLim && pResDsd->DelayEst <= (int)p->nDelayLim )
    {
clk = Abc_Clock();
        p2 = Lpk_DsdSplit( pMan, p, pResDsd->pCofVars, pResDsd->nCofVars, pResDsd->BSVars );
pMan->timeEvalDsdSp += Abc_Clock() - clk;
        assert( p2->nVars <= (int)p->nLutK );
        if ( p->nVars > p->nLutK && !Lpk_Decompose_rec( pMan, p ) )
            return 0;
        return 1;
    }

    // check MUX decomposition
clk = Abc_Clock();
    pResMux = Lpk_MuxAnalize( pMan, p );
pMan->timeEvalMuxAn += Abc_Clock() - clk;
//    pResMux = NULL;
    assert( !pResMux || (pResMux->DelayEst <= (int)p->nDelayLim && pResMux->AreaEst <= (int)p->nAreaLim) );
    // accept MUX decomposition if it is "good"
    if ( pResMux && pResMux->nSuppSizeS <= (int)p->nLutK && pResMux->nSuppSizeL <= (int)p->nLutK )
        pResDsd = NULL;
    else if ( pResMux && pResDsd )
    {
        // compare two decompositions
        if ( pResMux->AreaEst < pResDsd->AreaEst || 
            (pResMux->AreaEst == pResDsd->AreaEst && pResMux->nSuppSizeL < pResDsd->nSuppSizeL) || 
            (pResMux->AreaEst == pResDsd->AreaEst && pResMux->nSuppSizeL == pResDsd->nSuppSizeL && pResMux->DelayEst < pResDsd->DelayEst) )
            pResDsd = NULL;
        else
            pResMux = NULL;
    }
    assert( pResMux == NULL || pResDsd == NULL );
    if ( pResMux )
    {
clk = Abc_Clock();
        p2 = Lpk_MuxSplit( pMan, p, pResMux->Variable, pResMux->Polarity );
pMan->timeEvalMuxSp += Abc_Clock() - clk;
        if ( p2->nVars > p->nLutK && !Lpk_Decompose_rec( pMan, p2 ) )
            return 0;
        if ( p->nVars > p->nLutK && !Lpk_Decompose_rec( pMan, p ) )
            return 0;
        return 1;
    }
    if ( pResDsd )
    {
clk = Abc_Clock();
        p2 = Lpk_DsdSplit( pMan, p, pResDsd->pCofVars, pResDsd->nCofVars, pResDsd->BSVars );
pMan->timeEvalDsdSp += Abc_Clock() - clk;
        assert( p2->nVars <= (int)p->nLutK );
        if ( p->nVars > p->nLutK && !Lpk_Decompose_rec( pMan, p ) )
            return 0;
        return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Removes decomposed nodes from the array of fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lpk_DecomposeClean( Vec_Ptr_t * vLeaves, int nLeavesOld )
{
    Lpk_Fun_t * pFunc;
    int i;
    Vec_PtrForEachEntryStart( Lpk_Fun_t *, vLeaves, pFunc, i, nLeavesOld )
        Lpk_FunFree( pFunc );
    Vec_PtrShrink( vLeaves, nLeavesOld );
}

/**Function*************************************************************

  Synopsis    [Decomposes the function using recursive MUX decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Lpk_Decompose( Lpk_Man_t * p, Abc_Ntk_t * pNtk, Vec_Ptr_t * vLeaves, unsigned * pTruth, unsigned * puSupps, int nLutK, int AreaLim, int DelayLim )
{
    Lpk_Fun_t * pFun;
    Abc_Obj_t * pObjNew = NULL;
    int nLeaves = Vec_PtrSize( vLeaves );
    pFun = Lpk_FunCreate( pNtk, vLeaves, pTruth, nLutK, AreaLim, DelayLim );
    if ( puSupps[0] || puSupps[1] )
    {
/*
        int i;
        Lpk_FunComputeCofSupps( pFun );
        for ( i = 0; i < nLeaves; i++ )
        {
            assert( pFun->puSupps[2*i+0] == puSupps[2*i+0] );
            assert( pFun->puSupps[2*i+1] == puSupps[2*i+1] );
        }
*/
        memcpy( pFun->puSupps, puSupps, sizeof(unsigned) * 2 * nLeaves );
        pFun->fSupports = 1;
    }
    Lpk_FunSuppMinimize( pFun );
    if ( pFun->nVars <= pFun->nLutK )
        pObjNew = Lpk_ImplementFun( p, pNtk, vLeaves, pFun );
    else if ( Lpk_Decompose_rec(p, pFun) )
        pObjNew = Lpk_Implement( p, pNtk, vLeaves, nLeaves );
    Lpk_DecomposeClean( vLeaves, nLeaves );
    return pObjNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

