/**CFile****************************************************************

  FileName    [lpkMap.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast Boolean matching for LUT structures.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: lpkMap.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

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

  Synopsis    [Transforms the decomposition graph into the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * Lpk_MapPrimeInternal( If_Man_t * pIfMan, Kit_Graph_t * pGraph )
{
    Kit_Node_t * pNode = NULL; // Suppress "might be used uninitialized"
    If_Obj_t * pAnd0, * pAnd1;
    int i;
    // check for constant function
    if ( Kit_GraphIsConst(pGraph) )
        return If_ManConst1(pIfMan);
    // check for a literal
    if ( Kit_GraphIsVar(pGraph) )
        return (If_Obj_t *)Kit_GraphVar(pGraph)->pFunc;
    // build the AIG nodes corresponding to the AND gates of the graph
    Kit_GraphForEachNode( pGraph, pNode, i )
    {
        pAnd0 = (If_Obj_t *)Kit_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc; 
        pAnd1 = (If_Obj_t *)Kit_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc; 
        pNode->pFunc = If_ManCreateAnd( pIfMan, 
            If_NotCond( If_Regular(pAnd0), If_IsComplement(pAnd0) ^ pNode->eEdge0.fCompl ), 
            If_NotCond( If_Regular(pAnd1), If_IsComplement(pAnd1) ^ pNode->eEdge1.fCompl ) );
    }
    return (If_Obj_t *)pNode->pFunc;
}

/**Function*************************************************************

  Synopsis    [Strashes one logic node using its SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * Lpk_MapPrime( Lpk_Man_t * p, unsigned * pTruth, int nVars, If_Obj_t ** ppLeaves )
{
    Kit_Graph_t * pGraph;
    Kit_Node_t * pNode;
    If_Obj_t * pRes;
    int i;
    // derive the factored form
    pGraph = Kit_TruthToGraph( pTruth, nVars, p->vCover );
    if ( pGraph == NULL )
        return NULL;
    // collect the fanins
    Kit_GraphForEachLeaf( pGraph, pNode, i )
        pNode->pFunc = ppLeaves[i];
    // perform strashing
    pRes = Lpk_MapPrimeInternal( p->pIfMan, pGraph );
    pRes = If_NotCond( pRes, Kit_GraphIsComplement(pGraph) );
    Kit_GraphFree( pGraph );
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Prepares the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_Obj_t * Lpk_MapTree_rec( Lpk_Man_t * p, Kit_DsdNtk_t * pNtk, If_Obj_t ** ppLeaves, int iLit, If_Obj_t * pResult )
{
    Kit_DsdObj_t * pObj;
    If_Obj_t * pObjNew = NULL, * pObjNew2 = NULL, * pFansNew[16];
    unsigned i, iLitFanin;

    assert( iLit >= 0 );

    // consider the case of a gate
    pObj = Kit_DsdNtkObj( pNtk, Abc_Lit2Var(iLit) );
    if ( pObj == NULL )
    {
        pObjNew = ppLeaves[Abc_Lit2Var(iLit)];
        return If_NotCond( pObjNew, Abc_LitIsCompl(iLit) );
    }
    if ( pObj->Type == KIT_DSD_CONST1 )
    {
        return If_NotCond( If_ManConst1(p->pIfMan), Abc_LitIsCompl(iLit) );
    }
    if ( pObj->Type == KIT_DSD_VAR )
    {
        pObjNew = ppLeaves[Abc_Lit2Var(pObj->pFans[0])];
        return If_NotCond( pObjNew, Abc_LitIsCompl(iLit) ^ Abc_LitIsCompl(pObj->pFans[0]) );
    }
    if ( pObj->Type == KIT_DSD_AND )
    {
        assert( pObj->nFans == 2 );
        pFansNew[0] = Lpk_MapTree_rec( p, pNtk, ppLeaves, pObj->pFans[0], NULL );
        pFansNew[1] = pResult? pResult : Lpk_MapTree_rec( p, pNtk, ppLeaves, pObj->pFans[1], NULL );
        if ( pFansNew[0] == NULL || pFansNew[1] == NULL )
            return NULL;
        pObjNew = If_ManCreateAnd( p->pIfMan, pFansNew[0], pFansNew[1] ); 
        return If_NotCond( pObjNew, Abc_LitIsCompl(iLit) );
    }
    if ( pObj->Type == KIT_DSD_XOR )
    {
        int fCompl = Abc_LitIsCompl(iLit);
        assert( pObj->nFans == 2 );
        pFansNew[0] = Lpk_MapTree_rec( p, pNtk, ppLeaves, pObj->pFans[0], NULL );
        pFansNew[1] = pResult? pResult : Lpk_MapTree_rec( p, pNtk, ppLeaves, pObj->pFans[1], NULL );
        if ( pFansNew[0] == NULL || pFansNew[1] == NULL )
            return NULL;
        fCompl ^= If_IsComplement(pFansNew[0]) ^ If_IsComplement(pFansNew[1]);
        pObjNew = If_ManCreateXor( p->pIfMan, If_Regular(pFansNew[0]), If_Regular(pFansNew[1]) );
        return If_NotCond( pObjNew, fCompl );
    }
    assert( pObj->Type == KIT_DSD_PRIME );
    p->nBlocks[pObj->nFans]++;

    // solve for the inputs
    Kit_DsdObjForEachFanin( pNtk, pObj, iLitFanin, i )
    {
        if ( i == 0 )
            pFansNew[i] = pResult? pResult : Lpk_MapTree_rec( p, pNtk, ppLeaves, iLitFanin, NULL );
        else
            pFansNew[i] = Lpk_MapTree_rec( p, pNtk, ppLeaves, iLitFanin, NULL );
        if ( pFansNew[i] == NULL )
            return NULL;
    }
/* 
    if ( !p->fCofactoring && p->pPars->nVarsShared > 0 && (int)pObj->nFans > p->pPars->nLutSize )
    {
        pObjNew = Lpk_MapTreeMulti( p, Kit_DsdObjTruth(pObj), pObj->nFans, pFansNew );
        return If_NotCond( pObjNew, Abc_LitIsCompl(iLit) );
    }
*/
/*
    if ( (int)pObj->nFans > p->pPars->nLutSize )
    {
        pObjNew2 = Lpk_MapTreeMux_rec( p, Kit_DsdObjTruth(pObj), pObj->nFans, pFansNew );
//        if ( pObjNew2 )
//            return If_NotCond( pObjNew2, Abc_LitIsCompl(iLit) );
    }
*/

    // find best cofactoring variable
    if ( p->pPars->nVarsShared > 0 && (int)pObj->nFans > p->pPars->nLutSize )
    {
        pObjNew2 = Lpk_MapSuppRedDec_rec( p, Kit_DsdObjTruth(pObj), pObj->nFans, pFansNew );
        if ( pObjNew2 )
            return If_NotCond( pObjNew2, Abc_LitIsCompl(iLit) );
    }

    pObjNew = Lpk_MapPrime( p, Kit_DsdObjTruth(pObj), pObj->nFans, pFansNew );

    // add choice
    if ( pObjNew && pObjNew2 )
    {
        If_ObjSetChoice( If_Regular(pObjNew), If_Regular(pObjNew2) );
        If_ManCreateChoice( p->pIfMan, If_Regular(pObjNew) );
    }
    return If_NotCond( pObjNew, Abc_LitIsCompl(iLit) );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

