/**CFile****************************************************************

  FileName    [mpmTruth.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Configurable technology mapper.]

  Synopsis    [Truth table manipulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 1, 2013.]

  Revision    [$Id: mpmTruth.c,v 1.00 2013/06/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mpmInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

//#define MPM_TRY_NEW

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Unifies variable order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Mpm_TruthStretch( word * pTruth, Mpm_Cut_t * pCut, Mpm_Cut_t * pCut0, int nLimit )
{
    int i, k;
    for ( i = (int)pCut->nLeaves - 1, k = (int)pCut0->nLeaves - 1; i >= 0 && k >= 0; i-- )
    {
        if ( pCut0->pLeaves[k] < pCut->pLeaves[i] )
            continue;
        assert( pCut0->pLeaves[k] == pCut->pLeaves[i] );
        if ( k < i )
            Abc_TtSwapVars( pTruth, nLimit, k, i );
        k--;
    }
}

/**Function*************************************************************

  Synopsis    [Performs truth table support minimization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Mpm_CutTruthMinimize6( Mpm_Man_t * p, Mpm_Cut_t * pCut )
{
    unsigned uSupport;
    int i, k, nSuppSize;
    // compute the support of the cut's function
    word t = *Mpm_CutTruth( p, Abc_Lit2Var(pCut->iFunc) );
    uSupport = Abc_Tt6SupportAndSize( t, Mpm_CutLeafNum(pCut), &nSuppSize );
    if ( nSuppSize == Mpm_CutLeafNum(pCut) )
        return 0;
    p->nSmallSupp += (int)(nSuppSize < 2);
    // update leaves and signature
    for ( i = k = 0; i < Mpm_CutLeafNum(pCut); i++ )
    {
        if ( ((uSupport >> i) & 1) )
        {
            if ( k < i )
            {
                pCut->pLeaves[k] = pCut->pLeaves[i];
                Abc_TtSwapVars( &t, p->nLutSize, k, i );
            }
            k++;
        }
    }
    assert( k == nSuppSize );
    pCut->nLeaves = nSuppSize;
    assert( nSuppSize == Abc_TtSupportSize(&t, 6) );
    // save the result
    pCut->iFunc = Abc_Var2Lit( Vec_MemHashInsert(p->vTtMem, &t), Abc_LitIsCompl(pCut->iFunc) );
    return 1;
}
static inline int Mpm_CutTruthMinimize7( Mpm_Man_t * p, Mpm_Cut_t * pCut )
{
    unsigned uSupport;
    int i, k, nSuppSize;
    // compute the support of the cut's function
    word * pTruth = Mpm_CutTruth( p, Abc_Lit2Var(pCut->iFunc) );
    uSupport = Abc_TtSupportAndSize( pTruth, Mpm_CutLeafNum(pCut), &nSuppSize );
    if ( nSuppSize == Mpm_CutLeafNum(pCut) )
        return 0;
    p->nSmallSupp += (int)(nSuppSize < 2);
    // update leaves and signature
    Abc_TtCopy( p->Truth, pTruth, p->nTruWords, 0 );
    for ( i = k = 0; i < Mpm_CutLeafNum(pCut); i++ )
    {
        if ( ((uSupport >> i) & 1) )
        {
            if ( k < i )
            {
                pCut->pLeaves[k] = pCut->pLeaves[i];
                Abc_TtSwapVars( p->Truth, p->nLutSize, k, i );
            }
            k++;
        }
    }
    assert( k == nSuppSize );
    assert( nSuppSize == Abc_TtSupportSize(p->Truth, Mpm_CutLeafNum(pCut)) );
    pCut->nLeaves = nSuppSize;
    // save the result
    pCut->iFunc = Abc_Var2Lit( Vec_MemHashInsert(p->vTtMem, p->Truth), Abc_LitIsCompl(pCut->iFunc) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs truth table computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Mpm_CutComputeTruth6( Mpm_Man_t * p, Mpm_Cut_t * pCut, Mpm_Cut_t * pCut0, Mpm_Cut_t * pCut1, Mpm_Cut_t * pCutC, int fCompl0, int fCompl1, int fComplC, int Type )
{
    word * pTruth0 = Mpm_CutTruth( p, Abc_Lit2Var(pCut0->iFunc) );
    word * pTruth1 = Mpm_CutTruth( p, Abc_Lit2Var(pCut1->iFunc) );
    word * pTruthC = NULL;
    word t0 = (fCompl0 ^ pCut0->fCompl ^ Abc_LitIsCompl(pCut0->iFunc)) ? ~*pTruth0 : *pTruth0;
    word t1 = (fCompl1 ^ pCut1->fCompl ^ Abc_LitIsCompl(pCut1->iFunc)) ? ~*pTruth1 : *pTruth1;
    word tC = 0, t = 0;
    Mpm_TruthStretch( &t0, pCut, pCut0, p->nLutSize );
    Mpm_TruthStretch( &t1, pCut, pCut1, p->nLutSize );
    if ( pCutC )
    {
        pTruthC = Mpm_CutTruth( p, Abc_Lit2Var(pCutC->iFunc) );
        tC = (fComplC ^ pCutC->fCompl ^ Abc_LitIsCompl(pCutC->iFunc)) ? ~*pTruthC : *pTruthC;
        Mpm_TruthStretch( &tC, pCut, pCutC, p->nLutSize );
    }
    assert( p->nLutSize <= 6 );
    if ( Type == 1 )
        t = t0 & t1;
    else if ( Type == 2 )
        t = t0 ^ t1;
    else if ( Type == 3 )
        t = (tC & t1) | (~tC & t0);
    else assert( 0 );
    // save the result
    if ( t & 1 )
    {
        t = ~t;
        pCut->iFunc = Abc_Var2Lit( Vec_MemHashInsert( p->vTtMem, &t ), 1 );
    }
    else
        pCut->iFunc = Abc_Var2Lit( Vec_MemHashInsert( p->vTtMem, &t ), 0 );
    if ( p->pPars->fCutMin )
        return Mpm_CutTruthMinimize6( p, pCut );
    return 1;
}
static inline int Mpm_CutComputeTruth7( Mpm_Man_t * p, Mpm_Cut_t * pCut, Mpm_Cut_t * pCut0, Mpm_Cut_t * pCut1, Mpm_Cut_t * pCutC, int fCompl0, int fCompl1, int fComplC, int Type )
{
    word * pTruth0 = Mpm_CutTruth( p, Abc_Lit2Var(pCut0->iFunc) );
    word * pTruth1 = Mpm_CutTruth( p, Abc_Lit2Var(pCut1->iFunc) );
    word * pTruthC = NULL;
    Abc_TtCopy( p->Truth0, pTruth0, p->nTruWords, fCompl0 ^ pCut0->fCompl ^ Abc_LitIsCompl(pCut0->iFunc) );
    Abc_TtCopy( p->Truth1, pTruth1, p->nTruWords, fCompl1 ^ pCut1->fCompl ^ Abc_LitIsCompl(pCut1->iFunc) );
    Mpm_TruthStretch( p->Truth0, pCut, pCut0, p->nLutSize );
    Mpm_TruthStretch( p->Truth1, pCut, pCut1, p->nLutSize );
    if ( pCutC )
    {
        pTruthC = Mpm_CutTruth( p, Abc_Lit2Var(pCutC->iFunc) );
        Abc_TtCopy( p->TruthC, pTruthC, p->nTruWords, fComplC ^ pCutC->fCompl ^ Abc_LitIsCompl(pCutC->iFunc) );
        Mpm_TruthStretch( p->TruthC, pCut, pCutC, p->nLutSize );
    }
    if ( Type == 1 )
        Abc_TtAnd( p->Truth, p->Truth0, p->Truth1, p->nTruWords, 0 );
    else if ( Type == 2 )
        Abc_TtXor( p->Truth, p->Truth0, p->Truth1, p->nTruWords, 0 );
    else if ( Type == 3 )
        Abc_TtMux( p->Truth, p->TruthC, p->Truth1, p->Truth0, p->nTruWords );
    else assert( 0 );
    // save the result
    if ( p->Truth[0] & 1 )
    {
        Abc_TtNot( p->Truth, p->nTruWords );
        pCut->iFunc = Abc_Var2Lit( Vec_MemHashInsert( p->vTtMem, p->Truth ), 1 );
    }
    else
        pCut->iFunc = Abc_Var2Lit( Vec_MemHashInsert( p->vTtMem, p->Truth ), 0 );
    if ( p->pPars->fCutMin )
        return Mpm_CutTruthMinimize7( p, pCut );
    return 1;
}
int Mpm_CutComputeTruth( Mpm_Man_t * p, Mpm_Cut_t * pCut, Mpm_Cut_t * pCut0, Mpm_Cut_t * pCut1, Mpm_Cut_t * pCutC, int fCompl0, int fCompl1, int fComplC, int Type )
{
    int RetValue;
    if ( p->nLutSize <= 6 )
        RetValue = Mpm_CutComputeTruth6( p, pCut, pCut0, pCut1, pCutC, fCompl0, fCompl1, fComplC, Type );
    else
        RetValue = Mpm_CutComputeTruth7( p, pCut, pCut0, pCut1, pCutC, fCompl0, fCompl1, fComplC, Type );
#ifdef MPM_TRY_NEW
    {
        extern unsigned Abc_TtCanonicize( word * pTruth, int nVars, char * pCanonPerm );
        char pCanonPerm[16];
        memcpy( p->Truth0, p->Truth, sizeof(word) * p->nTruWords );
        Abc_TtCanonicize( p->Truth0, pCut->nLimit, pCanonPerm );
    }
#endif
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

