/**CFile****************************************************************

  FileName    [dauNonDsd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dauNonDsd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"
#include "misc/util/utilTruth.h"
#include "misc/extra/extra.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    [Checks decomposability with given variable set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_DecCheckSetTop5( word * p, int nVars, int nVarsF, int nVarsB, int nVarsS, int uMaskS, int * pSched, word * pDec, word * pComp )
{
    word Cof[2][64], Value;
    word MaskFF = (((word)1) << (1 << nVarsF)) - 1;
    int ShiftF = 6 - nVarsF, MaskF = (1 << ShiftF) - 1;
    int pVarsS[16], pVarsB[16];
    int nMints  = (1 << nVarsB);
    int nMintsB = (1 <<(nVarsB-nVarsS));
    int nMintsS = (1 << nVarsS);
    int s, b, v, m, Mint, MintB, MintS;
    assert( nVars == nVarsB + nVarsF );
    assert( nVars <= 16 );
    assert( nVarsS <= 6 );
    assert( nVarsF >= 1 && nVarsF <= 5 );
    // collect bound/shared variables
    for ( s = b = v = 0; v < nVarsB; v++ )
        if ( (uMaskS >> v) & 1 )
            pVarsB[v] = -1, pVarsS[v] = s++;
        else
            pVarsS[v] = -1, pVarsB[v] = b++;
    assert( s == nVarsS );
    assert( b == nVarsB-nVarsS );
    // clean minterm storage
    for ( s = 0; s < nMintsS; s++ )
        Cof[0][s] = Cof[1][s] = ~(word)0;
    // iterate through bound set minters
    for ( MintS = MintB = Mint = m = 0; m < nMints; m++ )
    {
        // find minterm value
        Value = (p[Mint>>ShiftF] >> ((Mint&MaskF)<<nVarsF)) & MaskFF;
        // check if this cof already appeared 
        if ( !~Cof[0][MintS] || Cof[0][MintS] == Value )
            Cof[0][MintS] = Value;
        else if ( !~Cof[1][MintS] || Cof[1][MintS] == Value )
        {
            Cof[1][MintS] = Value;
            if ( pDec ) 
            {
                int iMintB = MintS * nMintsB + MintB;
                pDec[iMintB>>6] |= (((word)1)<<(iMintB & 63));     
            }
        }
        else
            return 0;            
        // find next minterm
        v = pSched[m];
        Mint ^= (1 << v);
        if ( (uMaskS >> v) & 1 ) // shared variable
            MintS ^= (1 << pVarsS[v]);
        else
            MintB ^= (1 << pVarsB[v]);
    }
    // create composition function
    if ( pComp )
    {
        for ( s = 0; s < nMintsS; s++ )
        {
            pComp[s>>ShiftF] |= (Cof[0][s] << ((s&MaskF) << nVarsF));
            if ( ~Cof[1][s] ) 
                pComp[(s+nMintsS)>>ShiftF] |= (Cof[1][s] << (((s+nMintsS)&MaskF) << nVarsF));
            else
                pComp[(s+nMintsS)>>ShiftF] |= (Cof[0][s] << (((s+nMintsS)&MaskF) << nVarsF));
        }
        if ( nVarsF + nVarsS + 1 < 6 )
            pComp[0] = Abc_Tt6Stretch( pComp[0], nVarsF + nVarsS + 1 );
    }
    if ( pDec && nVarsB < 6 )
        pDec[0] = Abc_Tt6Stretch( pDec[0], nVarsB );
    return 1;
}
int Dau_DecCheckSetTop6( word * p, int nVars, int nVarsF, int nVarsB, int nVarsS, int uMaskS, int * pSched, word * pDec, word * pComp )
{
    word * Cof[2][64];
    int nWordsF = Abc_TtWordNum(nVarsF); 
    int pVarsS[16], pVarsB[16];
    int nMints  = (1 << nVarsB);
    int nMintsB = (1 <<(nVarsB-nVarsS));
    int nMintsS = (1 << nVarsS);
    int s, b, v, m, Mint, MintB, MintS;
    assert( nVars == nVarsB + nVarsF );
    assert( nVars <= 16 );
    assert( nVarsS <= 6 );
    assert( nVarsF >= 6 );
    // collect bound/shared variables
    for ( s = b = v = 0; v < nVarsB; v++ )
        if ( (uMaskS >> v) & 1 )
            pVarsB[v] = -1, pVarsS[v] = s++;
        else
            pVarsS[v] = -1, pVarsB[v] = b++;
    assert( s == nVarsS );
    assert( b == nVarsB-nVarsS );
    // clean minterm storage
    for ( s = 0; s < nMintsS; s++ )
        Cof[0][s] = Cof[1][s] = NULL;
    // iterate through bound set minters
    for ( MintS = MintB = Mint = m = 0; m < nMints; m++ )
    {
        // check if this cof already appeared 
        if ( !Cof[0][MintS] || !memcmp(Cof[0][MintS], p + Mint * nWordsF, sizeof(word) * nWordsF) )
            Cof[0][MintS] = p + Mint * nWordsF;
        else if ( !Cof[1][MintS] || !memcmp(Cof[1][MintS], p + Mint * nWordsF, sizeof(word) * nWordsF) )
        {
            Cof[1][MintS] = p + Mint * nWordsF;
            if ( pDec ) 
            {
                int iMintB = MintS * nMintsB + MintB;
                pDec[iMintB>>6] |= (((word)1)<<(iMintB & 63));     
            }
        }
        else
            return 0;            
        // find next minterm
        v = pSched[m];
        Mint ^= (1 << v);
        if ( (uMaskS >> v) & 1 ) // shared variable
            MintS ^= (1 << pVarsS[v]);
        else
            MintB ^= (1 << pVarsB[v]);
    }
    // create composition function
    if ( pComp )
    {
        for ( s = 0; s < nMintsS; s++ )
        {
            memcpy( pComp + s * nWordsF, Cof[0][s], sizeof(word) * nWordsF );
            if ( Cof[1][s] ) 
                memcpy( pComp + (s+nMintsS) * nWordsF, Cof[1][s], sizeof(word) * nWordsF );
            else
                memcpy( pComp + (s+nMintsS) * nWordsF, Cof[0][s], sizeof(word) * nWordsF );
        }
    }
    if ( pDec && nVarsB < 6 )
        pDec[0] = Abc_Tt6Stretch( pDec[0], nVarsB );
    return 1;
}
static inline int Dau_DecCheckSetTop( word * p, int nVars, int nVarsF, int nVarsB, int nVarsS, int uMaskS, int * pSched, word * pDec, word * pComp )
{
    if ( nVarsF < 6 )
        return Dau_DecCheckSetTop5( p, nVars, nVarsF, nVarsB, nVarsS, uMaskS, pSched, pDec, pComp );
    else
        return Dau_DecCheckSetTop6( p, nVars, nVarsF, nVarsB, nVarsS, uMaskS, pSched, pDec, pComp );
}

/**Function*************************************************************

  Synopsis    [Checks decomposability with given BS variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Dau_DecGetMinterm( word * p, int g, int nVarsS, int uMaskAll )
{
    int m, c, v;
    for ( m = c = v = 0; v < nVarsS; v++ )
        if ( !((uMaskAll >> v) & 1) ) // not shared bound set variable
        {
            if ( (g >> v) & 1 )
                m |= (1 << c);
            c++;
        }
    assert( c >= 2 );
    p[m>>6] |= (((word)1)<<(m & 63));                               
}
static inline int Dau_DecCheckSet5( word * p, int nVars, int nVarsF, int uMaskAll, int uMaskValue, word * pCof0, word * pCof1, word * pDec )
{
    int fFound0 = 0, fFound1 = 0;
    int g, gMax = (1 << (nVars - nVarsF));
    int Shift = 6 - nVarsF, Mask = (1 << Shift) - 1;
    word Mask2 = (((word)1) << (1 << nVarsF)) - 1;
    word Cof0 = 0, Cof1 = 0, Value;
    assert( nVarsF >= 1 && nVarsF <= 5 );
    if ( pDec ) *pDec = 0;
    for ( g = 0; g < gMax; g++ )
        if ( (g & uMaskAll) == uMaskValue ) // this minterm g matches shared variable minterm uMaskValue
        {
            Value = (p[g>>Shift] >> ((g&Mask)<<nVarsF)) & Mask2;
            if ( !fFound0 )
                Cof0 = Value, fFound0 = 1;
            else if ( Cof0 == Value )
                continue;
            else if ( !fFound1 )
            {
                Cof1 = Value, fFound1 = 1;
                if ( pDec ) Dau_DecGetMinterm( pDec, g, nVars-nVarsF, uMaskAll );
            }
            else if ( Cof1 == Value )
            {
                if ( pDec ) Dau_DecGetMinterm( pDec, g, nVars-nVarsF, uMaskAll );
                continue;
            }
            else
                return 0;            
        }
    if ( pCof0 )
    {
        assert( fFound0 );
        Cof1 = fFound1 ? Cof1 : Cof0;
        *pCof0 = Abc_Tt6Stretch( Cof0, nVarsF );
        *pCof1 = Abc_Tt6Stretch( Cof1, nVarsF );
    }
    return 1;
}
static inline int Dau_DecCheckSet6( word * p, int nVars, int nVarsF, int uMaskAll, int uMaskValue, word * pCof0, word * pCof1, word * pDec )
{
    int fFound0 = 0, fFound1 = 0;
    int g, gMax = (1 << (nVars - nVarsF));
    int nWords = Abc_TtWordNum(nVarsF);
    word * Cof0 = NULL, * Cof1 = NULL;
    assert( nVarsF >= 6 && nVarsF <= nVars - 2 );
    if ( pDec ) *pDec = 0;
    for ( g = 0; g < gMax; g++ )
        if ( (g & uMaskAll) == uMaskValue )
        {
            if ( !fFound0 )
                Cof0 = p + g * nWords, fFound0 = 1;
            else if ( !memcmp(Cof0, p + g * nWords, sizeof(word) * nWords) )
                continue;
            else if ( !fFound1 )
            {
                Cof1 = p + g * nWords, fFound1 = 1;
                if ( pDec ) Dau_DecGetMinterm( pDec, g, nVars-nVarsF, uMaskAll );
            }
            else if ( !memcmp(Cof1, p + g * nWords, sizeof(word) * nWords) )
            {
                if ( pDec ) Dau_DecGetMinterm( pDec, g, nVars-nVarsF, uMaskAll );
                continue;
            }
            else
                return 0;            
        }
    if ( pCof0 )
    {
        assert( fFound0 );
        Cof1 = fFound1 ? Cof1 : Cof0;
        memcpy( pCof0, Cof0, sizeof(word) * nWords );
        memcpy( pCof1, Cof1, sizeof(word) * nWords );
    }
    return 1;
}
static inline int Dau_DecCheckSetAny( word * p, int nVars, int nVarsF, int uMaskAll, int uMaskValue, word * pCof0, word * pCof1, word * pDec )
{
    assert( nVarsF >= 1 && nVarsF <= nVars - 2 );
    if ( nVarsF < 6 )
        return Dau_DecCheckSet5( p, nVars, nVarsF, uMaskAll, uMaskValue, pCof0, pCof1, pDec );
    else
        return Dau_DecCheckSet6( p, nVars, nVarsF, uMaskAll, uMaskValue, pCof0, pCof1, pDec );
}
int Dau_DecCheckSetTopOld( word * p, int nVars, int nVarsF, int nVarsB, int nVarsS, int maskS, word ** pCof0, word ** pCof1, word ** pDec )
{
    int i, pVarsS[16];
    int v, m, mMax = (1 << nVarsS), uMaskValue;
    assert( nVars >= 3 && nVars <= 16 );
    assert( nVars == nVarsF + nVarsB );
    assert( nVarsF >= 1 && nVarsF <= nVars - 2 );
    assert( nVarsB >= 2 && nVarsB <= nVars - 1 );
    assert( nVarsS >= 0 && nVarsS <= nVarsB - 2 );
    if ( nVarsS == 0 )
        return Dau_DecCheckSetAny( p, nVars, nVarsF, 0, 0, pCof0? pCof0[0] : 0, pCof1? pCof1[0] : 0, pDec? pDec[0] : 0 );
    // collect shared variables
    assert( maskS > 0 && maskS < (1 << nVarsB) );
    for ( i = 0, v = 0; v < nVarsB; v++ )
        if ( (maskS >> v) & 1 )
            pVarsS[i++] = v;
    assert( i == nVarsS );
    // go through shared set minterms
    for ( m = 0; m < mMax; m++ )
    {
        // generate share set mask
        uMaskValue = 0;
        for ( v = 0; v < nVarsS; v++ )
            if ( (m >> v) & 1 )
                uMaskValue |= (1 << pVarsS[v]);
        assert( (maskS & uMaskValue) == uMaskValue );
        // check decomposition
        if ( !Dau_DecCheckSetAny( p, nVars, nVarsF, maskS, uMaskValue, pCof0? pCof0[m] : 0, pCof1? pCof1[m] : 0, pDec? pDec[m] : 0 ) )
            return 0;
    }
   return 1;
}
 

/**Function*************************************************************

  Synopsis    [Variable sets.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Dau_DecCreateSet( int * pVarsB, int sizeB, int maskS )
{
    unsigned uSet = 0; int v;
    for ( v = 0; v < sizeB; v++ )
    {
        uSet |= (1 <<  (pVarsB[v] << 1));
        if ( (maskS >> v) & 1 )
        uSet |= (1 << ((pVarsB[v] << 1)+1));
    }
    return uSet;
}
static inline int Dau_DecSetHas01( unsigned Mask )
{
    return (Mask & ((~Mask) >> 1) & 0x55555555);
}
static inline int Dau_DecSetIsContained( Vec_Int_t * vSets, unsigned New )
// Old=abcD contains New=abcDE
// Old=abcD contains New=abCD
{
    unsigned Old;
    int i, Entry;
    Vec_IntForEachEntry( vSets, Entry, i )
    {
        Old = (unsigned)Entry;
        if ( (Old & ~New) == 0 && !Dau_DecSetHas01(~Old & New))
            return 1;
    }
    return 0;
}
void Dau_DecSortSet( unsigned set, int nVars, int * pnUnique, int * pnShared, int * pnFree )
{
    int v;
    int nUnique = 0, nShared = 0, nFree = 0;
    for ( v = 0; v < nVars; v++ )
    {
        int Value = ((set >> (v << 1)) & 3);
        if ( Value == 1 )
            nUnique++;
        else if ( Value == 3 )
            nShared++;
        else if ( Value == 0 )
            nFree++;
        else assert( 0 );
    }
    *pnUnique = nUnique;
    *pnShared = nShared;
    *pnFree   = nFree;
}
void Dau_DecPrintSet( unsigned set, int nVars, int fNewLine )
{
    int v, Counter = 0;
    int nUnique = 0, nShared = 0, nFree = 0;
    Dau_DecSortSet( set, nVars, &nUnique, &nShared, &nFree );
    printf( "S =%2d  D =%2d  C =%2d   ", nShared, nUnique+nShared, nShared+nFree+1 );

    printf( "x=" );
    for ( v = 0; v < nVars; v++ )
    {
        int Value = ((set >> (v << 1)) & 3);
        if ( Value == 1 )
            printf( "%c", 'a' + v ), Counter++;
        else if ( Value == 3 )
            printf( "%c", 'A' + v ), Counter++;
        else assert( Value == 0 );
    }
    printf( " y=x" );
    for ( v = 0; v < nVars; v++ )
    {
        int Value = ((set >> (v << 1)) & 3);
        if ( Value == 0 )
            printf( "%c", 'a' + v ), Counter++;
        else if ( Value == 3 )
            printf( "%c", 'A' + v ), Counter++;
    }
    for ( ; Counter < 15; Counter++ )
        printf( " " );
    if ( fNewLine )
        printf( "\n" );
}
unsigned Dau_DecReadSet( char * pStr )
{
    unsigned uSet = 0; int v;
    for ( v = 0; pStr[v]; v++ )
    {
        if ( pStr[v] >= 'a' && pStr[v] <= 'z' )
            uSet |= (1 << ((pStr[v] - 'a') << 1));
        else if ( pStr[v] >= 'A' && pStr[v] <= 'Z' )
            uSet |= (1 << ((pStr[v] - 'a') << 1)) | (1 << (((pStr[v] - 'a') << 1)+1));
        else break;
    }
    return uSet;
}
void Dau_DecPrintSets( Vec_Int_t * vSets, int nVars )
{
    int i, Entry;
    printf( "The %d-variable set family contains %d sets:\n", nVars, Vec_IntSize(vSets) );
    Vec_IntForEachEntry( vSets, Entry, i )
        Dau_DecPrintSet( (unsigned)Entry, nVars, 1 );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Find decomposable bound-sets of the given function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_DecMoveFreeToLSB( word * p, int nVars, int * V2P, int * P2V, int maskB, int sizeB )
{
    int v, c = 0;
    for ( v = 0; v < nVars; v++ )
        if ( !((maskB >> v) & 1) )
            Abc_TtMoveVar( p, nVars, V2P, P2V, v, c++ );
    assert( c == nVars - sizeB );
}
Vec_Int_t * Dau_DecFindSets_int( word * pInit, int nVars, int * pSched[16] )
{
    Vec_Int_t * vSets = Vec_IntAlloc( 32 );
    int V2P[16], P2V[16], pVarsB[16];
    int Limit = (1 << nVars);
    int c, v, sizeB, sizeS, maskB, maskS;
    unsigned setMixed;
    word p[1<<10]; 
    memcpy( p, pInit, sizeof(word) * Abc_TtWordNum(nVars) );
    for ( v = 0; v < nVars; v++ )
        assert( Abc_TtHasVar( p, nVars, v ) );
    // initialize permutation
    for ( v = 0; v < nVars; v++ )
        V2P[v] = P2V[v] = v;
    // iterate through bound sets of each size in increasing order
    for ( sizeB = 2; sizeB < nVars; sizeB++ ) // bound set size
    for ( maskB = 0; maskB < Limit; maskB++ ) // bound set
    if ( Abc_TtBitCount16(maskB) == sizeB )
    {
        // permute variables to have bound set on top
        Dau_DecMoveFreeToLSB( p, nVars, V2P, P2V, maskB, sizeB );
        // collect bound set vars on levels nVars-sizeB to nVars-1
        for ( c = 0; c < sizeB; c++ )
            pVarsB[c] = P2V[nVars-sizeB+c];
        // check disjoint
//        if ( Dau_DecCheckSetTopOld(p, nVars, nVars-sizeB, sizeB, 0, 0, NULL, NULL, NULL) )
        if ( Dau_DecCheckSetTop(p, nVars, nVars-sizeB, sizeB, 0, 0, pSched[sizeB], NULL, NULL) )
        {
            Vec_IntPush( vSets, Dau_DecCreateSet(pVarsB, sizeB, 0) );
            continue;
        }
        if ( sizeB == 2 )
            continue;
        // iterate through shared sets of each size in the increasing order
        for ( sizeS = 1; sizeS <= sizeB - 2; sizeS++ )   // shared set size
        if ( sizeS <= 3 )
//        sizeS = 1; 
        for ( maskS = 0; maskS < (1 << sizeB); maskS++ ) // shared set
        if ( Abc_TtBitCount16(maskS) == sizeS )
        {
            setMixed = Dau_DecCreateSet( pVarsB, sizeB, maskS );
//            printf( "Considering %10d ", setMixed );
//            Dau_DecPrintSet( setMixed, nVars );
            // check if it exists
            if ( Dau_DecSetIsContained(vSets, setMixed) )
                continue;
            // check if it can be added
//            if ( Dau_DecCheckSetTopOld(p, nVars, nVars-sizeB, sizeB, sizeS, maskS, NULL, NULL, NULL) )
            if ( Dau_DecCheckSetTop(p, nVars, nVars-sizeB, sizeB, sizeS, maskS, pSched[sizeB], NULL, NULL) )
                Vec_IntPush( vSets, setMixed );
        }
    }
    return vSets;
}
Vec_Int_t * Dau_DecFindSets( word * pInit, int nVars )
{
    Vec_Int_t * vSets;
    int v, * pSched[16] = {NULL};
    for ( v = 2; v < nVars; v++ )
        pSched[v] = Extra_GreyCodeSchedule( v );
    vSets = Dau_DecFindSets_int( pInit, nVars, pSched );
    for ( v = 2; v < nVars; v++ )
        ABC_FREE( pSched[v] );
    return vSets;
}
void Dau_DecFindSetsTest2()
{
    Vec_Int_t * vSets;
    word a0 = (~s_Truths6[1] & s_Truths6[2]) | (s_Truths6[1] & s_Truths6[3]);
    word a1 = (~s_Truths6[1] & s_Truths6[4]) | (s_Truths6[1] & s_Truths6[5]);
    word t  = (~s_Truths6[0] & a0)           | (s_Truths6[0] & a1); 
//    word t = ABC_CONST(0x7EFFFFFFFFFFFF7E); // and(gam1,gam2)
//    word t = ABC_CONST(0xB0F0BBFFB0F0BAFE); // some funct
//    word t = ABC_CONST(0x2B0228022B022802); // 5-var non-dec0x0F7700000F770000
//    word t = ABC_CONST(0x0F7700000F770000); // (!<(ab)cd>e)
//    word t = ABC_CONST(0x7F00000000000000); // (!(abc)def)
    int nVars = 5;

    vSets   = Dau_DecFindSets( &t, nVars );
    Dau_DecPrintSets( vSets, nVars );
    Vec_IntFree( vSets );
}


/**Function*************************************************************

  Synopsis    [Replaces variables in the string.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_DecVarReplace( char * pStr, int * pPerm, int nVars )
{
    int v;
    for ( v = 0; pStr[v]; v++ )
        if ( pStr[v] >= 'a' && pStr[v] <= 'z' )
        {
            assert( pStr[v] - 'a' < nVars );
            pStr[v] = 'a' + pPerm[pStr[v] - 'a'];
        }
}

/**Function*************************************************************

  Synopsis    [Decomposes with the given bound-set.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_DecDecomposeSet( word * pInit, int nVars, unsigned uSet, word * pComp, word * pDec, int * pPermC, int * pPermD, int * pnVarsC, int * pnVarsD, int * pnVarsS )
{
    word p[1<<13], Cof[64], Cof0[64], Cof1[64], Decs[64];
    word * pCof0[64], * pCof1[64], * pDecs[64], MintC, MintD;
    int V2P[16], P2V[16], pVarsU[16], pVarsS[16], pVarsF[16];
    int nVarsU = 0, nVarsS = 0, nVarsF = 0;
    int nWords = Abc_TtWordNum(nVars);
    int v, d, c, Status, nDecs;
    assert( nVars <= 16 );
    for ( v = 0; v < nVars; v++ )
        V2P[v] = P2V[v] = v;
    memcpy( p, pInit, sizeof(word) * nWords );
    // sort variables
    for ( v = 0; v < nVars; v++ )
    {
        int Value = (uSet >> (v<<1)) & 3;
        if ( Value == 0 )
            pVarsF[nVarsF++] = v;
        else if ( Value == 1 )
            pVarsU[nVarsU++] = v;
        else if ( Value == 3 )
            pVarsS[nVarsS++] = v;
        else assert(0);
    }
    assert( nVarsS >= 0 && nVarsS <= 6 );
    assert( nVarsF + nVarsS + 1 <= 6 );
    assert( nVarsU + nVarsS <= 6 );
    // init space for decomposition functions
    nDecs = (1 << nVarsS);
    for ( d = 0; d < nDecs; d++ )
    {
        pCof0[d] = Cof0 + d;
        pCof1[d] = Cof1 + d;
        pDecs[d] = Decs + d;
    }
    // permute variables
    c = 0;
    for ( v = 0; v < nVarsF; v++ )
       Abc_TtMoveVar( p, nVars, V2P, P2V, pVarsF[v], c++ );
    for ( v = 0; v < nVarsS; v++ )
       Abc_TtMoveVar( p, nVars, V2P, P2V, pVarsS[v], c++ );
    for ( v = 0; v < nVarsU; v++ )
       Abc_TtMoveVar( p, nVars, V2P, P2V, pVarsU[v], c++ );
    assert( c == nVars );
    // check decomposition
    Status = Dau_DecCheckSetTopOld( p, nVars, nVarsF, nVarsS+nVarsU, nVarsS, Abc_InfoMask(nVarsS), pCof0, pCof1, pDecs );
    if ( !Status )
        return 0;
    // compute cofactors
    assert( nVarsF + nVarsS < 6 );
    for ( d = 0; d < nDecs; d++ )
    {
        Cof[d] = (pCof1[d][0] & s_Truths6[nVarsF + nVarsS]) | (pCof0[d][0] & ~s_Truths6[nVarsF + nVarsS]);
        pDecs[d][0] = Abc_Tt6Stretch( pDecs[d][0], nVarsU );
    }
    // compute the resulting functions
    pComp[0] = 0;
    pDec[0] = 0;
    for ( d = 0; d < nDecs; d++ )
    {
        // compute minterms for composition/decomposition function
        MintC = MintD = ~((word)0);
        for ( v = 0; v < nVarsS; v++ )
        {
            MintC &= ((d >> v) & 1) ? s_Truths6[nVarsF+v] : ~s_Truths6[nVarsF+v];
            MintD &= ((d >> v) & 1) ? s_Truths6[nVarsU+v] : ~s_Truths6[nVarsU+v];
        }
        // derive functions
        pComp[0] |= MintC & Cof[d];
        pDec[0] |= MintD & pDecs[d][0];
    }
    // derive variable permutations
    if ( pPermC )
    {
        for ( v = 0; v < nVarsF; v++ )
            pPermC[v] = pVarsF[v];
        for ( v = 0; v < nVarsS; v++ )
            pPermC[nVarsF+v] = pVarsS[v];
        pPermC[nVarsF + nVarsS] = nVars;
    }
    if ( pPermD )
    {
        for ( v = 0; v < nVarsU; v++ )
            pPermD[v] = pVarsU[v];
        for ( v = 0; v < nVarsS; v++ )
            pPermD[nVarsU+v] = pVarsS[v];
    }
    if ( pnVarsC )
        *pnVarsC = nVarsF + nVarsS + 1;
    if ( pnVarsD )
        *pnVarsD = nVarsU + nVarsS;
    if ( pnVarsS )
        *pnVarsS = nVarsS;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Testing procedures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_DecVerify( word * pInit, int nVars, char * pDsdC, char * pDsdD )
{
    word pC[1<<13], pD[1<<13], pRes[1<<13]; // max = 16
    int nWordsC = Abc_TtWordNum(nVars+1);
    int nWordsD = Abc_TtWordNum(nVars);
    assert( nVars < 16 );
    memcpy( pC, Dau_DsdToTruth(pDsdC, nVars+1), sizeof(word) * nWordsC );
    memcpy( pD, Dau_DsdToTruth(pDsdD, nVars), sizeof(word) * nWordsD );
    if ( nVars >= 6 )
    {
        assert( nWordsD >= 1 );
        assert( nWordsC > 1 );
        Abc_TtMux( pRes, pD, pC + nWordsD, pC, nWordsD );
    }
    else
    {
        word pC0 = Abc_Tt6Stretch( pC[0], nVars );
        word pC1 = Abc_Tt6Stretch( (pC[0] >> (1 << nVars)), nVars );
        Abc_TtMux( pRes, pD, &pC1, &pC0, nWordsD );
    }
    if ( !Abc_TtEqual(pInit, pRes, nWordsD) )
        printf( "      Verification failed" );
//    else
//        printf( "      Verification successful" );
    printf( "\n" );
    return 1;
}
int Dau_DecPerform6( word * p, int nVars, unsigned uSet )
{
    word tComp = 0, tDec = 0, tDec0, tComp0, tComp1, FuncC, FuncD;
    char pDsdC[1000], pDsdD[1000];
    int pPermC[16], pPermD[16];
    int nVarsC, nVarsD, nVarsS, nVarsU, nVarsF, nPairs;
    int i, m, v, status, ResC, ResD, Counter = 0;
    status = Dau_DecDecomposeSet( p, nVars, uSet, &tComp, &tDec0, pPermC, pPermD, &nVarsC, &nVarsD, &nVarsS );
    if ( !status )
    {
        printf( "  Decomposition does not exist\n" );
        return 0;
    }
    nVarsU = nVarsD - nVarsS;
    nVarsF = nVarsC - nVarsS - 1;
    tComp0 = Abc_Tt6Cofactor0( tComp, nVarsF + nVarsS );
    tComp1 = Abc_Tt6Cofactor1( tComp, nVarsF + nVarsS );
    nPairs = 1 << (1 << nVarsS); 
    for ( i = 0; i < nPairs; i++ )
    {
        if ( i & 1 )
            continue;
        // create miterms with this polarity
        FuncC = FuncD = 0;
        for ( m = 0; m < (1 << nVarsS); m++ )
        {
            word MintC, MintD;
            if ( !((i >> m) & 1) )
                continue;
            MintC = MintD = ~(word)0;
            for ( v = 0; v < nVarsS; v++ )
            {
                MintC &= ((m >> v) & 1) ? s_Truths6[nVarsF+v] : ~s_Truths6[nVarsF+v];
                MintD &= ((m >> v) & 1) ? s_Truths6[nVarsU+v] : ~s_Truths6[nVarsU+v];
            }
            FuncC |= MintC;
            FuncD |= MintD;
        }
        // uncomplement given variables
        tComp = (~s_Truths6[nVarsF + nVarsS] & ((tComp0 & ~FuncC) | (tComp1 & FuncC))) | (s_Truths6[nVarsF + nVarsS] & ((tComp1 & ~FuncC) | (tComp0 & FuncC)));
        tDec = tDec0 ^ FuncD;
        // decompose
        ResC = Dau_DsdDecompose( &tComp, nVarsC, 0, 1, pDsdC );
        ResD = Dau_DsdDecompose( &tDec, nVarsD, 0, 1, pDsdD );
        // replace variables
        Dau_DecVarReplace( pDsdD, pPermD, nVarsD );
        Dau_DecVarReplace( pDsdC, pPermC, nVarsC );
        // report
//        printf( "  " );
        printf( "%3d : ", Counter++ );
        printf( "%24s  ", pDsdD );
        printf( "%24s  ", pDsdC );
        Dau_DecVerify( p, nVars, pDsdC, pDsdD );
    }
    return 1;
}

int Dau_DecPerform( word * pInit, int nVars, unsigned uSet )
{
    word p[1<<10], pDec[1<<10], pComp[1<<10]; // at most 2^10 words
    char pDsdC[5000], pDsdD[5000];  // at most 2^12 hex digits
    int nVarsU, nVarsS, nVarsF, nVarsC = 0, nVarsD = 0;
    int V2P[16], P2V[16], pPermC[16], pPermD[16], * pSched;
    int v, i, status, ResC, ResD;
    int nWords = Abc_TtWordNum(nVars);
    assert( nVars <= 16 );
    // backup the function
    memcpy( p, pInit, sizeof(word) * nWords );
    // get variable numbers
    Dau_DecSortSet( uSet, nVars, &nVarsU, &nVarsS, &nVarsF );
    // permute function and order variables
    for ( v = 0; v < nVars; v++ )
        V2P[v] = P2V[v] = v;
    for ( i = v = 0; v < nVars; v++ )
        if ( ((uSet >> (v<<1)) & 3) == 0 ) // free first
           Abc_TtMoveVar( p, nVars, V2P, P2V, v, i++ ), pPermC[nVarsC++] = v;
    for ( v = 0; v < nVars; v++ )
        if ( ((uSet >> (v<<1)) & 3) == 3 ) // share second
           Abc_TtMoveVar( p, nVars, V2P, P2V, v, i++ ), pPermC[nVarsC++] = v;
    pPermC[nVarsC++] = nVars;
    for ( v = 0; v < nVars; v++ )
        if ( ((uSet >> (v<<1)) & 3) == 1 ) // unique last
           Abc_TtMoveVar( p, nVars, V2P, P2V, v, i++ ), pPermD[nVarsD++] = v;
    for ( v = 0; v < nVarsS; v++ )
        pPermD[nVarsD++] = pPermC[nVarsF+v];
    assert( nVarsD == nVarsU + nVarsS );
    assert( nVarsC == nVarsF + nVarsS + 1 );
    assert( i == nVars );
    // decompose
    pSched = Extra_GreyCodeSchedule( nVarsU + nVarsS );
    memset( pDec, 0, sizeof(word) * Abc_TtWordNum(nVarsD) );
    memset( pComp, 0, sizeof(word) * Abc_TtWordNum(nVarsC) );
    status = Dau_DecCheckSetTop( p, nVars, nVarsF, nVarsU + nVarsS, nVarsS, nVarsS ? Abc_InfoMask(nVarsS) : 0, pSched, pDec, pComp );
    ABC_FREE( pSched );
    if ( !status )
    {
        printf( "  Decomposition does not exist\n" );
        return 0;
    }
//    Dau_DsdPrintFromTruth( stdout, pC, nVars+1 ); //printf( "\n" );
//    Dau_DsdPrintFromTruth( stdout, pD, nVars ); //printf( "\n" );
//    Kit_DsdPrintFromTruth( (unsigned *)pComp, 6 ); printf( "\n" );
//    Kit_DsdPrintFromTruth( (unsigned *)pDec, 6 );  printf( "\n" );
    // decompose
    ResC = Dau_DsdDecompose( pComp, nVarsC, 0, 1, pDsdC );
    ResD = Dau_DsdDecompose( pDec, nVarsD, 0, 1, pDsdD );
    // replace variables
    Dau_DecVarReplace( pDsdD, pPermD, nVarsD );
    Dau_DecVarReplace( pDsdC, pPermC, nVarsC );
    // report
    printf( "     " );
    printf( "%3d : ", 0 );
    printf( "%24s  ", pDsdD );
    printf( "%24s  ", pDsdC );
    Dau_DecVerify( pInit, nVars, pDsdC, pDsdD );
    return 1;
}
void Dau_DecTrySets( word * pInit, int nVars, int fVerbose )
{
    Vec_Int_t * vSets;
    int i, Entry;
    assert( nVars <= 16 );
    vSets = Dau_DecFindSets( pInit, nVars );
    if ( !fVerbose )
    {
        Vec_IntFree( vSets );
        return;
    }
    Dau_DsdPrintFromTruth( pInit, nVars ); 
    printf( "This %d-variable function has %d decomposable variable sets:\n", nVars, Vec_IntSize(vSets) );
    Vec_IntForEachEntry( vSets, Entry, i )
    {
        unsigned uSet = (unsigned)Entry;
        printf( "Set %4d : ", i );
        if ( nVars > 6 )
        {
            Dau_DecPrintSet( uSet, nVars, 0 );
            Dau_DecPerform( pInit, nVars, uSet );
        }
        else
        {
            Dau_DecPrintSet( uSet, nVars, 1 );
            Dau_DecPerform6( pInit, nVars, uSet );
        }
    }
    Vec_IntFree( vSets );
//    printf( "\n" );
}

void Dau_DecFindSetsTest3()
{
    word a0 = (~s_Truths6[1] & s_Truths6[2]) | (s_Truths6[1] & s_Truths6[3]);
    word a1 = (~s_Truths6[1] & s_Truths6[4]) | (s_Truths6[1] & s_Truths6[5]);
    word t  = (~s_Truths6[0] & a0)           | (s_Truths6[0] & a1); 
//    word t = ABC_CONST(0x0F7700000F770000); // (!<(ab)cd>e)
    int nVars = 6;
    char * pStr = "Bcd";
//    char * pStr = "Abcd";
//    char * pStr = "ab";
    unsigned uSet = Dau_DecReadSet( pStr );
    Dau_DecPerform6( &t, nVars, uSet );
}

void Dau_DecFindSetsTest()
{
    int nVars = 6;
//    word a0 = (~s_Truths6[1] & s_Truths6[2]) | (s_Truths6[1] & s_Truths6[3]);
//    word a1 = (~s_Truths6[1] & s_Truths6[4]) | (s_Truths6[1] & s_Truths6[5]);
//    word t  = (~s_Truths6[0] & a0)           | (s_Truths6[0] & a1); 
//    word t = ABC_CONST(0x7EFFFFFFFFFFFF7E); // and(gam1,gam2)
//    word t = ABC_CONST(0xB0F0BBFFB0F0BAFE); // some funct
//    word t = ABC_CONST(0x00000000901FFFFF); // some funct
    word t = ABC_CONST(0x000030F00D0D3FFF); // some funct
//    word t = ABC_CONST(0x00000000690006FF); // some funct
//    word t = ABC_CONST(0x7000F80007FF0FFF); // some funct
//    word t = ABC_CONST(0x4133CB334133CB33); // some funct 5 var
//    word t = ABC_CONST(0x2B0228022B022802); // 5-var non-dec0x0F7700000F770000
//    word t = ABC_CONST(0x0F7700000F770000); // (!<(ab)cd>e)
//    word t = ABC_CONST(0x7F00000000000000); // (!(abc)def)
    Dau_DecTrySets( &t, nVars, 1 );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

