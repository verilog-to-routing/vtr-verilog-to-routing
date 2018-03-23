/**CFile****************************************************************

  FileName    [lpkAbcMux.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast Boolean matching for LUT structures.]

  Synopsis    [LUT-decomposition based on recursive MUX decomposition.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: lpkAbcMux.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

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

  Synopsis    [Checks the possibility of MUX decomposition.]

  Description [Returns the best variable to use for MUX decomposition.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lpk_Res_t * Lpk_MuxAnalize( Lpk_Man_t * pMan, Lpk_Fun_t * p )
{
    static Lpk_Res_t Res, * pRes = &Res;
    int nSuppSize0, nSuppSize1, nSuppSizeS, nSuppSizeL;
    int Var, Area, Polarity, Delay, Delay0, Delay1, DelayA, DelayB;
    memset( pRes, 0, sizeof(Lpk_Res_t) );
    assert( p->uSupp == Kit_BitMask(p->nVars) );
    assert( p->fSupports );
    // derive the delay and area after MUX-decomp with each var - and find the best var
    pRes->Variable = -1;
    Lpk_SuppForEachVar( p->uSupp, Var )
    {
        nSuppSize0 = Kit_WordCountOnes(p->puSupps[2*Var+0]);
        nSuppSize1 = Kit_WordCountOnes(p->puSupps[2*Var+1]);
        assert( nSuppSize0 < (int)p->nVars );
        assert( nSuppSize1 < (int)p->nVars );
        if ( nSuppSize0 < 1 || nSuppSize1 < 1 )
            continue;
//printf( "%d %d    ", nSuppSize0, nSuppSize1 );
        if ( nSuppSize0 <= (int)p->nLutK - 2 && nSuppSize1 <= (int)p->nLutK - 2 )
        {
            // include cof var into 0-block
            DelayA = Lpk_SuppDelay( p->puSupps[2*Var+0] | (1<<Var), p->pDelays );
            DelayB = Lpk_SuppDelay( p->puSupps[2*Var+1]           , p->pDelays );
            Delay0 = Abc_MaxInt( DelayA, DelayB + 1 );
            // include cof var into 1-block
            DelayA = Lpk_SuppDelay( p->puSupps[2*Var+1] | (1<<Var), p->pDelays );
            DelayB = Lpk_SuppDelay( p->puSupps[2*Var+0]           , p->pDelays );
            Delay1 = Abc_MaxInt( DelayA, DelayB + 1 );
            // get the best delay
            Delay = Abc_MinInt( Delay0, Delay1 );
            Area = 2;
            Polarity = (int)(Delay == Delay1);
        }
        else if ( nSuppSize0 <= (int)p->nLutK - 2 )
        {
            DelayA = Lpk_SuppDelay( p->puSupps[2*Var+0] | (1<<Var), p->pDelays );
            DelayB = Lpk_SuppDelay( p->puSupps[2*Var+1]           , p->pDelays );
            Delay = Abc_MaxInt( DelayA, DelayB + 1 );
            Area = 1 + Lpk_LutNumLuts( nSuppSize1, p->nLutK );
            Polarity = 0;
        }
        else if ( nSuppSize1 <= (int)p->nLutK - 2 )
        {
            DelayA = Lpk_SuppDelay( p->puSupps[2*Var+1] | (1<<Var), p->pDelays );
            DelayB = Lpk_SuppDelay( p->puSupps[2*Var+0]           , p->pDelays );
            Delay = Abc_MaxInt( DelayA, DelayB + 1 );
            Area = 1 + Lpk_LutNumLuts( nSuppSize0, p->nLutK );
            Polarity = 1;
        }
        else if ( nSuppSize0 <= (int)p->nLutK )
        {
            DelayA = Lpk_SuppDelay( p->puSupps[2*Var+1] | (1<<Var), p->pDelays );
            DelayB = Lpk_SuppDelay( p->puSupps[2*Var+0]           , p->pDelays );
            Delay = Abc_MaxInt( DelayA, DelayB + 1 );
            Area = 1 + Lpk_LutNumLuts( nSuppSize1+2, p->nLutK );
            Polarity = 1;
        }
        else if ( nSuppSize1 <= (int)p->nLutK )
        {
            DelayA = Lpk_SuppDelay( p->puSupps[2*Var+0] | (1<<Var), p->pDelays );
            DelayB = Lpk_SuppDelay( p->puSupps[2*Var+1]           , p->pDelays );
            Delay = Abc_MaxInt( DelayA, DelayB + 1 );
            Area = 1 + Lpk_LutNumLuts( nSuppSize0+2, p->nLutK );
            Polarity = 0;
        }
        else
        {
            // include cof var into 0-block
            DelayA = Lpk_SuppDelay( p->puSupps[2*Var+0] | (1<<Var), p->pDelays );
            DelayB = Lpk_SuppDelay( p->puSupps[2*Var+1]           , p->pDelays );
            Delay0 = Abc_MaxInt( DelayA, DelayB + 1 );
            // include cof var into 1-block
            DelayA = Lpk_SuppDelay( p->puSupps[2*Var+1] | (1<<Var), p->pDelays );
            DelayB = Lpk_SuppDelay( p->puSupps[2*Var+0]           , p->pDelays );
            Delay1 = Abc_MaxInt( DelayA, DelayB + 1 );
            // get the best delay
            Delay = Abc_MinInt( Delay0, Delay1 );
            if ( Delay == Delay0 )
                Area = Lpk_LutNumLuts( nSuppSize0+2, p->nLutK ) + Lpk_LutNumLuts( nSuppSize1, p->nLutK );
            else
                Area = Lpk_LutNumLuts( nSuppSize1+2, p->nLutK ) + Lpk_LutNumLuts( nSuppSize0, p->nLutK );
            Polarity = (int)(Delay == Delay1);
        }
        // find the best variable
        if ( Delay > (int)p->nDelayLim )
            continue;
        if ( Area > (int)p->nAreaLim )
            continue;
        nSuppSizeS = Abc_MinInt( nSuppSize0 + 2 *!Polarity, nSuppSize1 + 2 * Polarity );
        nSuppSizeL = Abc_MaxInt( nSuppSize0 + 2 *!Polarity, nSuppSize1 + 2 * Polarity );
        if ( nSuppSizeL > (int)p->nVars )
            continue;
        if ( pRes->Variable == -1 || pRes->AreaEst > Area || 
            (pRes->AreaEst == Area && pRes->nSuppSizeS + pRes->nSuppSizeL > nSuppSizeS + nSuppSizeL) || 
            (pRes->AreaEst == Area && pRes->nSuppSizeS + pRes->nSuppSizeL == nSuppSizeS + nSuppSizeL && pRes->DelayEst > Delay) )
        {
            pRes->Variable = Var;
            pRes->Polarity = Polarity;
            pRes->AreaEst  = Area;
            pRes->DelayEst = Delay;
            pRes->nSuppSizeS = nSuppSizeS;
            pRes->nSuppSizeL = nSuppSizeL;
        }
    }
    return pRes->Variable == -1 ? NULL : pRes;
}

/**Function*************************************************************

  Synopsis    [Transforms the function decomposed by the MUX decomposition.]

  Description [Returns the best variable to use for MUX decomposition.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lpk_Fun_t * Lpk_MuxSplit( Lpk_Man_t * pMan, Lpk_Fun_t * p, int Var, int Pol )
{
    Lpk_Fun_t * pNew;
    unsigned * pTruth  = Lpk_FunTruth( p, 0 );
    unsigned * pTruth0 = Lpk_FunTruth( p, 1 );
    unsigned * pTruth1 = Lpk_FunTruth( p, 2 );
//    unsigned uSupp;
    int iVarVac; 
    assert( Var >= 0 && Var < (int)p->nVars );
    assert( p->nAreaLim >= 2 );
    assert( p->uSupp == Kit_BitMask(p->nVars) );
    Kit_TruthCofactor0New( pTruth0, pTruth, p->nVars, Var );
    Kit_TruthCofactor1New( pTruth1, pTruth, p->nVars, Var );
/*
uSupp = Kit_TruthSupport( pTruth, p->nVars );
Extra_PrintBinary( stdout, &uSupp, 16 ); printf( "\n" );
uSupp = Kit_TruthSupport( pTruth0, p->nVars );
Extra_PrintBinary( stdout, &uSupp, 16 ); printf( "\n" );
uSupp = Kit_TruthSupport( pTruth1, p->nVars );
Extra_PrintBinary( stdout, &uSupp, 16 ); printf( "\n\n" );
*/
    // derive the new component
    pNew = Lpk_FunDup( p, Pol ? pTruth0 : pTruth1 );
    // update the support of the old component
    p->uSupp  = Kit_TruthSupport( Pol ? pTruth1 : pTruth0, p->nVars );
    p->uSupp |= (1 << Var);
    // update the truth table of the old component
    iVarVac = Kit_WordFindFirstBit( ~p->uSupp );
    assert( iVarVac < (int)p->nVars );
    p->uSupp |= (1 << iVarVac);
    Kit_TruthIthVar( pTruth, p->nVars, iVarVac );
    if ( Pol )
        Kit_TruthMuxVar( pTruth, pTruth, pTruth1, p->nVars, Var );
    else
        Kit_TruthMuxVar( pTruth, pTruth0, pTruth, p->nVars, Var );
    assert( p->uSupp == Kit_TruthSupport(pTruth, p->nVars) );
    // set the decomposed variable
    p->pFanins[iVarVac] = pNew->Id;
    p->pDelays[iVarVac] = p->nDelayLim - 1;
    // support minimize both
    p->fSupports = 0;
    Lpk_FunSuppMinimize( p );
    Lpk_FunSuppMinimize( pNew );
    // update delay and area requirements
    pNew->nDelayLim = p->nDelayLim - 1;
    if ( pNew->nVars <= pNew->nLutK )
    {
        pNew->nAreaLim = 1;
        p->nAreaLim = p->nAreaLim - 1;
    }
    else if ( p->nVars <= p->nLutK )
    {
        pNew->nAreaLim = p->nAreaLim - 1;
        p->nAreaLim = 1;
    }
    else if ( p->nVars < pNew->nVars )
    {
        pNew->nAreaLim = p->nAreaLim / 2 + p->nAreaLim % 2;
        p->nAreaLim = p->nAreaLim / 2 - p->nAreaLim % 2;
    }
    else // if ( pNew->nVars < p->nVars )
    {
        pNew->nAreaLim = p->nAreaLim / 2 - p->nAreaLim % 2;
        p->nAreaLim = p->nAreaLim / 2 + p->nAreaLim % 2;
    }
    pNew->fMark = 1;
    return pNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

