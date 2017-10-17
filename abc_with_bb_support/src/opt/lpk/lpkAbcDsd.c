/**CFile****************************************************************

  FileName    [lpkAbcDsd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Fast Boolean matching for LUT structures.]

  Synopsis    [LUT-decomposition based on recursive DSD.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: lpkAbcDsd.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

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

  Synopsis    [Cofactors TTs w.r.t. all vars and finds the best var.]

  Description [The best variable is the variable with the minimum 
  sum total of the support sizes of all truth tables. This procedure 
  computes and returns cofactors w.r.t. the best variable.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_FunComputeMinSuppSizeVar( Lpk_Fun_t * p, unsigned ** ppTruths, int nTruths, unsigned ** ppCofs, unsigned uNonDecSupp )
{
    int i, Var, VarBest, nSuppSize0, nSuppSize1;
    int nSuppTotalMin = -1; // Suppress "might be used uninitialized"
    int nSuppTotalCur;
    int nSuppMaxMin = -1; // Suppress "might be used uninitialized"
    int nSuppMaxCur;
    assert( nTruths > 0 );
    VarBest = -1;
    Lpk_SuppForEachVar( p->uSupp, Var )
    {
        if ( (uNonDecSupp & (1 << Var)) == 0 )
            continue;
        nSuppMaxCur = 0;
        nSuppTotalCur = 0;
        for ( i = 0; i < nTruths; i++ )
        {
            if ( nTruths == 1 )
            {
                nSuppSize0 = Kit_WordCountOnes( p->puSupps[2*Var+0] );
                nSuppSize1 = Kit_WordCountOnes( p->puSupps[2*Var+1] );
            }
            else
            {
                Kit_TruthCofactor0New( ppCofs[2*i+0], ppTruths[i], p->nVars, Var );
                Kit_TruthCofactor1New( ppCofs[2*i+1], ppTruths[i], p->nVars, Var );
                nSuppSize0 = Kit_TruthSupportSize( ppCofs[2*i+0], p->nVars );
                nSuppSize1 = Kit_TruthSupportSize( ppCofs[2*i+1], p->nVars );
            }        
            nSuppMaxCur = Abc_MaxInt( nSuppMaxCur, nSuppSize0 );
            nSuppMaxCur = Abc_MaxInt( nSuppMaxCur, nSuppSize1 );
            nSuppTotalCur += nSuppSize0 + nSuppSize1;
        }
        if ( VarBest == -1 || nSuppMaxMin > nSuppMaxCur ||
             (nSuppMaxMin == nSuppMaxCur && nSuppTotalMin > nSuppTotalCur) )
        {
            VarBest = Var;
            nSuppMaxMin = nSuppMaxCur;
            nSuppTotalMin = nSuppTotalCur;
        }
    }
    // recompute cofactors for the best var
    for ( i = 0; i < nTruths; i++ )
    {
        Kit_TruthCofactor0New( ppCofs[2*i+0], ppTruths[i], p->nVars, VarBest );
        Kit_TruthCofactor1New( ppCofs[2*i+1], ppTruths[i], p->nVars, VarBest );
    }
    return VarBest;
}

/**Function*************************************************************

  Synopsis    [Recursively computes decomposable subsets.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Lpk_ComputeBoundSets_rec( Kit_DsdNtk_t * p, int iLit, Vec_Int_t * vSets, int nSizeMax )
{
    unsigned i, iLitFanin, uSupport, uSuppCur;
    Kit_DsdObj_t * pObj;
    // consider the case of simple gate
    pObj = Kit_DsdNtkObj( p, Abc_Lit2Var(iLit) );
    if ( pObj == NULL )
        return (1 << Abc_Lit2Var(iLit));
    if ( pObj->Type == KIT_DSD_AND || pObj->Type == KIT_DSD_XOR )
    {
        unsigned uSupps[16], Limit, s;
        uSupport = 0;
        Kit_DsdObjForEachFanin( p, pObj, iLitFanin, i )
        {
            uSupps[i] = Lpk_ComputeBoundSets_rec( p, iLitFanin, vSets, nSizeMax );
            uSupport |= uSupps[i];
        }
        // create all subsets, except empty and full
        Limit = (1 << pObj->nFans) - 1;
        for ( s = 1; s < Limit; s++ )
        {
            uSuppCur = 0;
            for ( i = 0; i < pObj->nFans; i++ )
                if ( s & (1 << i) )
                    uSuppCur |= uSupps[i];
            if ( Kit_WordCountOnes(uSuppCur) <= nSizeMax )
                Vec_IntPush( vSets, uSuppCur );
        }
        return uSupport;
    }
    assert( pObj->Type == KIT_DSD_PRIME );
    // get the cumulative support of all fanins
    uSupport = 0;
    Kit_DsdObjForEachFanin( p, pObj, iLitFanin, i )
    {
        uSuppCur  = Lpk_ComputeBoundSets_rec( p, iLitFanin, vSets, nSizeMax );
        uSupport |= uSuppCur;
        if ( Kit_WordCountOnes(uSuppCur) <= nSizeMax )
            Vec_IntPush( vSets, uSuppCur );
    }
    return uSupport;
}

/**Function*************************************************************

  Synopsis    [Computes the set of subsets of decomposable variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Lpk_ComputeBoundSets( Kit_DsdNtk_t * p, int nSizeMax )
{
    Vec_Int_t * vSets;
    unsigned uSupport, Entry;
    int Number, i;
    assert( p->nVars <= 16 );
    vSets = Vec_IntAlloc( 100 );
    Vec_IntPush( vSets, 0 );
    if ( Kit_DsdNtkRoot(p)->Type == KIT_DSD_CONST1 )
        return vSets;
    if ( Kit_DsdNtkRoot(p)->Type == KIT_DSD_VAR )
    {
        uSupport = ( 1 << Abc_Lit2Var(Kit_DsdNtkRoot(p)->pFans[0]) );
        if ( Kit_WordCountOnes(uSupport) <= nSizeMax )
            Vec_IntPush( vSets, uSupport );
        return vSets;
    }
    uSupport = Lpk_ComputeBoundSets_rec( p, p->Root, vSets, nSizeMax );
    assert( (uSupport & 0xFFFF0000) == 0 );
    // add the total support of the network
    if ( Kit_WordCountOnes(uSupport) <= nSizeMax )
        Vec_IntPush( vSets, uSupport );
    // set the remaining variables
    Vec_IntForEachEntry( vSets, Number, i )
    {
        Entry = Number;
        Vec_IntWriteEntry( vSets, i, Entry | ((uSupport & ~Entry) << 16) );
    }
    return vSets;
}

/**Function*************************************************************

  Synopsis    [Prints the sets of subsets.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Lpk_PrintSetOne( int uSupport )
{
    unsigned k;
    for ( k = 0; k < 16; k++ )
        if ( uSupport & (1<<k) )
            printf( "%c", 'a'+k );
    printf( " " );
}
/**Function*************************************************************

  Synopsis    [Prints the sets of subsets.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Lpk_PrintSets( Vec_Int_t * vSets )
{
    unsigned uSupport;
    int Number, i;
    printf( "Subsets(%d): ", Vec_IntSize(vSets) );
    Vec_IntForEachEntry( vSets, Number, i )
    {
        uSupport = Number;
        Lpk_PrintSetOne( uSupport );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Merges two bound sets.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Lpk_MergeBoundSets( Vec_Int_t * vSets0, Vec_Int_t * vSets1, int nSizeMax )
{
    Vec_Int_t * vSets;
    int Entry0, Entry1, Entry;
    int i, k;
    vSets = Vec_IntAlloc( 100 );
    Vec_IntForEachEntry( vSets0, Entry0, i )
    Vec_IntForEachEntry( vSets1, Entry1, k )
    {
        Entry = Entry0 | Entry1;
        if ( (Entry & (Entry >> 16)) )
            continue;
        if ( Kit_WordCountOnes(Entry & 0xffff) <= nSizeMax )
            Vec_IntPush( vSets, Entry );
    }
    return vSets;
}

/**Function*************************************************************

  Synopsis    [Performs DSD-based decomposition of the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lpk_FunCompareBoundSets( Lpk_Fun_t * p, Vec_Int_t * vBSets, int nCofDepth, unsigned uNonDecSupp, unsigned uLateArrSupp, Lpk_Res_t * pRes )
{
    int fVerbose = 0;
    unsigned uBoundSet;
    int i, nVarsBS, nVarsRem, Delay, Area;

    // compare the resulting boundsets
    memset( pRes, 0, sizeof(Lpk_Res_t) );
    Vec_IntForEachEntry( vBSets, uBoundSet, i )
    {
        if ( (uBoundSet & 0xFFFF) == 0 ) // skip empty boundset
            continue;
        if ( (uBoundSet & uNonDecSupp) == 0 ) // skip those boundsets that are not in the domain of interest
            continue;
        if ( (uBoundSet & uLateArrSupp) ) // skip those boundsets that are late arriving
            continue;
if ( fVerbose )
{
Lpk_PrintSetOne( uBoundSet & 0xFFFF );
//printf( "\n" );
//Lpk_PrintSetOne( uBoundSet >> 16 );
//printf( "\n" );
}
        assert( (uBoundSet & (uBoundSet >> 16)) == 0 );
        nVarsBS = Kit_WordCountOnes( uBoundSet & 0xFFFF );
        if ( nVarsBS == 1 )
            continue;
        assert( nVarsBS <= (int)p->nLutK - nCofDepth );
        nVarsRem = p->nVars - nVarsBS + 1;
        Area = 1 + Lpk_LutNumLuts( nVarsRem, p->nLutK );
        Delay = 1 + Lpk_SuppDelay( uBoundSet & 0xFFFF, p->pDelays );
if ( fVerbose )
printf( "area = %d limit = %d  delay = %d limit = %d\n", Area, (int)p->nAreaLim, Delay, (int)p->nDelayLim );
        if ( Area > (int)p->nAreaLim || Delay > (int)p->nDelayLim )
            continue;
        if ( pRes->BSVars == 0 || pRes->nSuppSizeL > nVarsRem || (pRes->nSuppSizeL == nVarsRem && pRes->DelayEst > Delay) )
        {
            pRes->nBSVars = nVarsBS;
            pRes->BSVars = (uBoundSet & 0xFFFF);
            pRes->nSuppSizeS = nVarsBS + nCofDepth;
            pRes->nSuppSizeL = nVarsRem;
            pRes->DelayEst = Delay;
            pRes->AreaEst = Area;
        }
    }
if ( fVerbose )
{
if ( pRes->BSVars )
{
printf( "Found bound set " );
Lpk_PrintSetOne( pRes->BSVars );
printf( "\n" );
}
else
printf( "Did not find boundsets.\n" );
printf( "\n" );
}
    if ( pRes->BSVars )
    {
        assert( pRes->DelayEst <= (int)p->nDelayLim );
        assert( pRes->AreaEst <= (int)p->nAreaLim );
    }
}


/**Function*************************************************************

  Synopsis    [Finds late arriving inputs, which cannot be in the bound set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Lpk_DsdLateArriving( Lpk_Fun_t * p )
{
    unsigned i, uLateArrSupp = 0;
    Lpk_SuppForEachVar( p->uSupp, i )
        if ( p->pDelays[i] > (int)p->nDelayLim - 2 )
            uLateArrSupp |= (1 << i);  
    return uLateArrSupp;
}

/**Function*************************************************************

  Synopsis    [Performs DSD-based decomposition of the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lpk_DsdAnalizeOne( Lpk_Fun_t * p, unsigned * ppTruths[5][16], Kit_DsdNtk_t * pNtks[], char pCofVars[], int nCofDepth, Lpk_Res_t * pRes )
{
    int fVerbose = 0;
    Vec_Int_t * pvBSets[4][8];
    unsigned uNonDecSupp, uLateArrSupp;
    int i, k, nNonDecSize, nNonDecSizeMax;
    assert( nCofDepth >= 1 && nCofDepth <= 3 );
    assert( nCofDepth < (int)p->nLutK - 1 );
    assert( p->fSupports );

    // find the support of the largest non-DSD block
    nNonDecSizeMax = 0;
    uNonDecSupp = p->uSupp;
    for ( i = 0; i < (1<<(nCofDepth-1)); i++ )
    {
        nNonDecSize = Kit_DsdNonDsdSizeMax( pNtks[i] );
        if ( nNonDecSizeMax < nNonDecSize )
        {
            nNonDecSizeMax = nNonDecSize;
            uNonDecSupp = Kit_DsdNonDsdSupports( pNtks[i] );
        }
        else if ( nNonDecSizeMax == nNonDecSize )
            uNonDecSupp |= Kit_DsdNonDsdSupports( pNtks[i] );
    }

    // remove those variables that cannot be used because of delay constraints
    // if variables arrival time is more than p->DelayLim - 2, it cannot be used
    uLateArrSupp = Lpk_DsdLateArriving( p );
    if ( (uNonDecSupp & ~uLateArrSupp) == 0 )
    {
        memset( pRes, 0, sizeof(Lpk_Res_t) );
        return 0;
    }

    // find the next cofactoring variable
    pCofVars[nCofDepth-1] = Lpk_FunComputeMinSuppSizeVar( p, ppTruths[nCofDepth-1], 1<<(nCofDepth-1), ppTruths[nCofDepth], uNonDecSupp & ~uLateArrSupp );

    // derive decomposed networks
    for ( i = 0; i < (1<<nCofDepth); i++ )
    {
        if ( pNtks[i] )
            Kit_DsdNtkFree( pNtks[i] );
        pNtks[i] = Kit_DsdDecomposeExpand( ppTruths[nCofDepth][i], p->nVars );
if ( fVerbose )
Kit_DsdPrint( stdout, pNtks[i] );
        pvBSets[nCofDepth][i] = Lpk_ComputeBoundSets( pNtks[i], p->nLutK - nCofDepth ); // try restricting to those in uNonDecSupp!!!
    }

    // derive the set of feasible boundsets
    for ( i = nCofDepth - 1; i >= 0; i-- )
        for ( k = 0; k < (1<<i); k++ )
            pvBSets[i][k] = Lpk_MergeBoundSets( pvBSets[i+1][2*k+0], pvBSets[i+1][2*k+1], p->nLutK - nCofDepth );
    // compare bound-sets
    Lpk_FunCompareBoundSets( p, pvBSets[0][0], nCofDepth, uNonDecSupp, uLateArrSupp, pRes );
    // free the bound sets
    for ( i = nCofDepth; i >= 0; i-- )
        for ( k = 0; k < (1<<i); k++ )
            Vec_IntFree( pvBSets[i][k] );
 
    // copy the cofactoring variables
    if ( pRes->BSVars )
    {
        pRes->nCofVars = nCofDepth;
        for ( i = 0; i < nCofDepth; i++ )
            pRes->pCofVars[i] = pCofVars[i];
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Performs DSD-based decomposition of the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lpk_Res_t * Lpk_DsdAnalize( Lpk_Man_t * pMan, Lpk_Fun_t * p, int nShared )
{ 
    static Lpk_Res_t Res0, * pRes0 = &Res0;
    static Lpk_Res_t Res1, * pRes1 = &Res1;
    static Lpk_Res_t Res2, * pRes2 = &Res2;
    static Lpk_Res_t Res3, * pRes3 = &Res3;
    int fUseBackLooking = 1;
    Lpk_Res_t * pRes = NULL;
    Vec_Int_t * vBSets;
    Kit_DsdNtk_t * pNtks[8] = {NULL};
    char pCofVars[5];
    int i;

    assert( p->nLutK >= 3 );
    assert( nShared >= 0 && nShared <= 3 );
    assert( p->uSupp == Kit_BitMask(p->nVars) );

    // try decomposition without cofactoring
    pNtks[0] = Kit_DsdDecomposeExpand( Lpk_FunTruth( p, 0 ), p->nVars );
    if ( pMan->pPars->fVerbose )
        pMan->nBlocks[ Kit_DsdNonDsdSizeMax(pNtks[0]) ]++;
    vBSets = Lpk_ComputeBoundSets( pNtks[0], p->nLutK );
    Lpk_FunCompareBoundSets( p, vBSets, 0, 0xFFFF, Lpk_DsdLateArriving(p), pRes0 );
    Vec_IntFree( vBSets );

    // check the result
    if ( pRes0->nBSVars == (int)p->nLutK )
        { pRes = pRes0; goto finish; }
    if ( pRes0->nBSVars == (int)p->nLutK - 1 )
        { pRes = pRes0; goto finish; }
    if ( nShared == 0 )
        goto finish;

    // prepare storage
    Kit_TruthCopy( pMan->ppTruths[0][0], Lpk_FunTruth( p, 0 ), p->nVars );

    // cofactor 1 time
    if ( !Lpk_DsdAnalizeOne( p, pMan->ppTruths, pNtks, pCofVars, 1, pRes1 ) )
        goto finish;
    assert( pRes1->nBSVars <= (int)p->nLutK - 1 );
    if ( pRes1->nBSVars == (int)p->nLutK - 1 )
        { pRes = pRes1; goto finish; }
    if ( pRes0->nBSVars == (int)p->nLutK - 2 )
        { pRes = pRes0; goto finish; }
    if ( pRes1->nBSVars == (int)p->nLutK - 2 )
        { pRes = pRes1; goto finish; }
    if ( nShared == 1 )
        goto finish;

    // cofactor 2 times
    if ( p->nLutK >= 4 ) 
    {
        if ( !Lpk_DsdAnalizeOne( p, pMan->ppTruths, pNtks, pCofVars, 2, pRes2 ) )
            goto finish;
        assert( pRes2->nBSVars <= (int)p->nLutK - 2 );
        if ( pRes2->nBSVars == (int)p->nLutK - 2 )
            { pRes = pRes2; goto finish; }
        if ( fUseBackLooking )
        {
            if ( pRes0->nBSVars == (int)p->nLutK - 3 )
                { pRes = pRes0; goto finish; }
            if ( pRes1->nBSVars == (int)p->nLutK - 3 )
                { pRes = pRes1; goto finish; }
        }
        if ( pRes2->nBSVars == (int)p->nLutK - 3 )
            { pRes = pRes2; goto finish; }
        if ( nShared == 2 )
            goto finish;
        assert( nShared == 3 );
    }

    // cofactor 3 times
    if ( p->nLutK >= 5 ) 
    {
        if ( !Lpk_DsdAnalizeOne( p, pMan->ppTruths, pNtks, pCofVars, 3, pRes3 ) )
            goto finish;
        assert( pRes3->nBSVars <= (int)p->nLutK - 3 );
        if ( pRes3->nBSVars == (int)p->nLutK - 3 )
            { pRes = pRes3; goto finish; }
        if ( fUseBackLooking )
        {
            if ( pRes0->nBSVars == (int)p->nLutK - 4 )
                { pRes = pRes0; goto finish; }
            if ( pRes1->nBSVars == (int)p->nLutK - 4 )
                { pRes = pRes1; goto finish; }
            if ( pRes2->nBSVars == (int)p->nLutK - 4 )
                { pRes = pRes2; goto finish; }
        }
        if ( pRes3->nBSVars == (int)p->nLutK - 4 )
            { pRes = pRes3; goto finish; }
    }

finish:
    // free the networks
    for ( i = 0; i < (1<<nShared); i++ )
        if ( pNtks[i] )
            Kit_DsdNtkFree( pNtks[i] );
    // choose the best under these conditions
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Splits the function into two subfunctions using DSD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lpk_Fun_t * Lpk_DsdSplit( Lpk_Man_t * pMan, Lpk_Fun_t * p, char * pCofVars, int nCofVars, unsigned uBoundSet )
{
    Lpk_Fun_t * pNew;
    Kit_DsdNtk_t * pNtkDec;
    int i, k, iVacVar, nCofs;
    // prepare storage
    Kit_TruthCopy( pMan->ppTruths[0][0], Lpk_FunTruth(p, 0), p->nVars );
    // get the vacuous variable
    iVacVar = Kit_WordFindFirstBit( uBoundSet );
    // compute the cofactors
    for ( i = 0; i < nCofVars; i++ )
        for ( k = 0; k < (1<<i); k++ )
        {
            Kit_TruthCofactor0New( pMan->ppTruths[i+1][2*k+0], pMan->ppTruths[i][k], p->nVars, pCofVars[i] );
            Kit_TruthCofactor1New( pMan->ppTruths[i+1][2*k+1], pMan->ppTruths[i][k], p->nVars, pCofVars[i] );
        }
    // decompose each cofactor w.r.t. the bound set
    nCofs = (1<<nCofVars);
    for ( k = 0; k < nCofs; k++ )
    {
        pNtkDec = Kit_DsdDecomposeExpand( pMan->ppTruths[nCofVars][k], p->nVars );
        Kit_DsdTruthPartialTwo( pMan->pDsdMan, pNtkDec, uBoundSet, iVacVar, pMan->ppTruths[nCofVars+1][k], pMan->ppTruths[nCofVars+1][nCofs+k] );
        Kit_DsdNtkFree( pNtkDec );
    }
    // compute the composition/decomposition functions (they will be in pMan->ppTruths[1][0]/pMan->ppTruths[1][1])
    for ( i = nCofVars; i >= 1; i-- )
        for ( k = 0; k < (1<<i); k++ )
            Kit_TruthMuxVar( pMan->ppTruths[i][k], pMan->ppTruths[i+1][2*k+0], pMan->ppTruths[i+1][2*k+1], p->nVars, pCofVars[i-1] );

    // derive the new component (decomposition function)
    pNew = Lpk_FunDup( p, pMan->ppTruths[1][1] );
    // update the old component (composition function)
    Kit_TruthCopy( Lpk_FunTruth(p, 0), pMan->ppTruths[1][0], p->nVars );
    p->uSupp = Kit_TruthSupport( Lpk_FunTruth(p, 0), p->nVars );
    p->pFanins[iVacVar] = pNew->Id;
    p->pDelays[iVacVar] = Lpk_SuppDelay( pNew->uSupp, pNew->pDelays );
    // support minimize both
    p->fSupports = 0;
    Lpk_FunSuppMinimize( p );
    Lpk_FunSuppMinimize( pNew );
    // update delay and area requirements
    pNew->nDelayLim = p->pDelays[iVacVar];
    pNew->nAreaLim = 1;
    p->nAreaLim = p->nAreaLim - 1;
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

