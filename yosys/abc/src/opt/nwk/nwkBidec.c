/**CFile****************************************************************

  FileName    [nwkBidec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Logic network representation.]

  Synopsis    [Bi-decomposition of local functions.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwkBidec.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "nwk.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static inline int  Extra_TruthWordNum( int nVars )  { return nVars <= 5 ? 1 : (1 << (nVars - 5)); }
static inline void Extra_TruthNot( unsigned * pOut, unsigned * pIn, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = ~pIn[w];
}
static inline void Extra_TruthOr( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn0[w] | pIn1[w];
}
static inline void Extra_TruthSharp( unsigned * pOut, unsigned * pIn0, unsigned * pIn1, int nVars )
{
    int w;
    for ( w = Extra_TruthWordNum(nVars)-1; w >= 0; w-- )
        pOut[w] = pIn0[w] & ~pIn1[w];
}

static inline Hop_Obj_t * Bdc_FunCopyHop( Bdc_Fun_t * pObj )  { return Hop_NotCond( (Hop_Obj_t *)Bdc_FuncCopy(Bdc_Regular(pObj)), Bdc_IsComplement(pObj) );  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Resynthesizes nodes using bi-decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Nwk_NodeIfNodeResyn( Bdc_Man_t * p, Hop_Man_t * pHop, Hop_Obj_t * pRoot, int nVars, Vec_Int_t * vTruth, unsigned * puCare, float dProb )
{
    unsigned * pTruth;
    Bdc_Fun_t * pFunc;
    int nNodes, i;
    assert( nVars <= 16 );
    // derive truth table
    pTruth = Hop_ManConvertAigToTruth( pHop, Hop_Regular(pRoot), nVars, vTruth, 0 );
    if ( Hop_IsComplement(pRoot) )
        for ( i = Abc_TruthWordNum(nVars)-1; i >= 0; i-- )
            pTruth[i] = ~pTruth[i];
    // perform power-aware decomposition
    if ( dProb >= 0.0 )
    {
        float Prob = (float)2.0 * dProb * (1.0 - dProb);
        assert( Prob >= 0.0 && Prob <= 0.5 );
        if ( Prob >= 0.4 )
        {
            Extra_TruthNot( puCare, puCare, nVars );
            if ( dProb > 0.5 ) // more 1s than 0s
                Extra_TruthOr( pTruth, pTruth, puCare, nVars );
            else
                Extra_TruthSharp( pTruth, pTruth, puCare, nVars );
            Extra_TruthNot( puCare, puCare, nVars );
            // decompose truth table
            Bdc_ManDecompose( p, pTruth, NULL, nVars, NULL, 1000 );
        }
        else
        {
            // decompose truth table
            Bdc_ManDecompose( p, pTruth, puCare, nVars, NULL, 1000 );
        }
    }
    else
    {
        // decompose truth table
        Bdc_ManDecompose( p, pTruth, puCare, nVars, NULL, 1000 );
    }
    // convert back into HOP
    Bdc_FuncSetCopy( Bdc_ManFunc( p, 0 ), Hop_ManConst1( pHop ) );
    for ( i = 0; i < nVars; i++ )
        Bdc_FuncSetCopy( Bdc_ManFunc( p, i+1 ), Hop_ManPi( pHop, i ) );
    nNodes = Bdc_ManNodeNum(p);
    for ( i = nVars + 1; i < nNodes; i++ )
    {
        pFunc = Bdc_ManFunc( p, i );
        Bdc_FuncSetCopy( pFunc, Hop_And( pHop, Bdc_FunCopyHop(Bdc_FuncFanin0(pFunc)), Bdc_FunCopyHop(Bdc_FuncFanin1(pFunc)) ) );
    }
    return Bdc_FunCopyHop( Bdc_ManRoot(p) );
}

/**Function*************************************************************

  Synopsis    [Resynthesizes nodes using bi-decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nwk_ManBidecResyn( Nwk_Man_t * pNtk, int fVerbose )
{
    Bdc_Par_t Pars = {0}, * pPars = &Pars;
    Bdc_Man_t * p;
    Nwk_Obj_t * pObj;
    Vec_Int_t * vTruth;
    int i, nGainTotal = 0, nNodes1, nNodes2;
    abctime clk = Abc_Clock();
    pPars->nVarsMax = Nwk_ManGetFaninMax( pNtk );
    pPars->fVerbose = fVerbose;
    if ( pPars->nVarsMax < 2 )
    {
        printf( "Resynthesis is not performed for networks whose nodes are less than 2 inputs.\n" );
        return;
    }
    if ( pPars->nVarsMax > 15 )
    {
        if ( fVerbose )
        printf( "Resynthesis is not performed for nodes with more than 15 inputs.\n" );
        pPars->nVarsMax = 15;
    }
    vTruth = Vec_IntAlloc( 0 );
    p = Bdc_ManAlloc( pPars );
    Nwk_ManForEachNode( pNtk, pObj, i )
    {
        if ( Nwk_ObjFaninNum(pObj) > 15 )
            continue;
        nNodes1 = Hop_DagSize(pObj->pFunc);
        pObj->pFunc = Nwk_NodeIfNodeResyn( p, pNtk->pManHop, pObj->pFunc, Nwk_ObjFaninNum(pObj), vTruth, NULL, -1.0 );
        nNodes2 = Hop_DagSize(pObj->pFunc);
        nGainTotal += nNodes1 - nNodes2;
    }
    Bdc_ManFree( p );
    Vec_IntFree( vTruth );
    if ( fVerbose )
    {
    printf( "Total gain in AIG nodes = %d.  ", nGainTotal );
    ABC_PRT( "Total runtime", Abc_Clock() - clk );
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

