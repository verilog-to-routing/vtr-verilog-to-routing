/**CFile****************************************************************

  FileName    [dauCanon.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    [Canonical form computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dauCanon.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"
#include "misc/util/utilTruth.h"
#include "misc/vec/vecMem.h"
#include "bool/lucky/lucky.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static word s_CMasks6[5] = {
    ABC_CONST(0x1111111111111111),
    ABC_CONST(0x0303030303030303),
    ABC_CONST(0x000F000F000F000F),
    ABC_CONST(0x000000FF000000FF),
    ABC_CONST(0x000000000000FFFF)
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks equality of pairs of cofactors w.r.t. adjacent variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtCheckEqual2VarCofs( word * pTruth, int nWords, int iVar, int Num1, int Num2 )
{
    assert( Num1 < Num2 && Num2 < 4 );
    if ( nWords == 1 )
        return ((pTruth[0] >> (Num2 * (1 << iVar))) & s_CMasks6[iVar]) == ((pTruth[0] >> (Num1 * (1 << iVar))) & s_CMasks6[iVar]);
	if ( iVar <= 4 )
	{
		int w, shift = (1 << iVar);
		for ( w = 0; w < nWords; w++ )
            if ( ((pTruth[w] >> Num2 * shift) & s_CMasks6[iVar]) != ((pTruth[w] >> Num1 * shift) & s_CMasks6[iVar]) )
                return 0;
        return 1;
	}
	if ( iVar == 5 )
	{
        unsigned * pTruthU = (unsigned *)pTruth;
        unsigned * pLimitU = (unsigned *)(pTruth + nWords);
        assert( nWords >= 2 );
		for ( ; pTruthU < pLimitU; pTruthU += 4 )
            if ( pTruthU[Num2] != pTruthU[Num1] )
                return 0;
        return 1;
	}
	// if ( iVar > 5 )
	{
        word * pLimit = pTruth + nWords;
		int i, iStep = Abc_TtWordNum(iVar);
        assert( nWords >= 4 );
		for ( ; pTruth < pLimit; pTruth += 4*iStep )
			for ( i = 0; i < iStep; i++ )
                if ( pTruth[i+Num2*iStep] != pTruth[i+Num1*iStep] )
                    return 0;
        return 1;
	}	
}

/**Function*************************************************************

  Synopsis    [Compares pairs of cofactors w.r.t. adjacent variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtCompare2VarCofs( word * pTruth, int nWords, int iVar, int Num1, int Num2 )
{
    assert( Num1 < Num2 && Num2 < 4 );
    if ( nWords == 1 )
    {
        word Cof1 = (pTruth[0] >> (Num1 * (1 << iVar))) & s_CMasks6[iVar];
        word Cof2 = (pTruth[0] >> (Num2 * (1 << iVar))) & s_CMasks6[iVar];
        if ( Cof1 != Cof2 )
            return Cof1 < Cof2 ? -1 : 1;
        return 0;
    }
	if ( iVar <= 4 )
	{
        word Cof1, Cof2;
		int w, shift = (1 << iVar);
		for ( w = 0; w < nWords; w++ )
        {
            Cof1 = (pTruth[w] >> Num1 * shift) & s_CMasks6[iVar];
            Cof2 = (pTruth[w] >> Num2 * shift) & s_CMasks6[iVar];
            if ( Cof1 != Cof2 )
                return Cof1 < Cof2 ? -1 : 1;
        }
        return 0;
	}
	if ( iVar == 5 )
	{
        unsigned * pTruthU = (unsigned *)pTruth;
        unsigned * pLimitU = (unsigned *)(pTruth + nWords);
        assert( nWords >= 2 );
		for ( ; pTruthU < pLimitU; pTruthU += 4 )
            if ( pTruthU[Num1] != pTruthU[Num2] )
                return pTruthU[Num1] < pTruthU[Num2] ? -1 : 1;
        return 0;
	}
	// if ( iVar > 5 )
	{
        word * pLimit = pTruth + nWords;
		int i, iStep = Abc_TtWordNum(iVar);
        int Offset1 = Num1*iStep;
        int Offset2 = Num2*iStep;
        assert( nWords >= 4 );
		for ( ; pTruth < pLimit; pTruth += 4*iStep )
			for ( i = 0; i < iStep; i++ )
                if ( pTruth[i + Offset1] != pTruth[i + Offset2] )
                    return pTruth[i + Offset1] < pTruth[i + Offset2] ? -1 : 1;
        return 0;
	}	
}
static inline int Abc_TtCompare2VarCofsRev( word * pTruth, int nWords, int iVar, int Num1, int Num2 )
{
    assert( Num1 < Num2 && Num2 < 4 );
    if ( nWords == 1 )
    {
        word Cof1 = (pTruth[0] >> (Num1 * (1 << iVar))) & s_CMasks6[iVar];
        word Cof2 = (pTruth[0] >> (Num2 * (1 << iVar))) & s_CMasks6[iVar];
        if ( Cof1 != Cof2 )
            return Cof1 < Cof2 ? -1 : 1;
        return 0;
    }
	if ( iVar <= 4 )
	{
        word Cof1, Cof2;
		int w, shift = (1 << iVar);
		for ( w = nWords - 1; w >= 0; w-- )
        {
            Cof1 = (pTruth[w] >> Num1 * shift) & s_CMasks6[iVar];
            Cof2 = (pTruth[w] >> Num2 * shift) & s_CMasks6[iVar];
            if ( Cof1 != Cof2 )
                return Cof1 < Cof2 ? -1 : 1;
        }
        return 0;
	}
	if ( iVar == 5 )
	{
        unsigned * pTruthU = (unsigned *)pTruth;
        unsigned * pLimitU = (unsigned *)(pTruth + nWords);
        assert( nWords >= 2 );
		for ( pLimitU -= 4; pLimitU >= pTruthU; pLimitU -= 4 )
            if ( pLimitU[Num1] != pLimitU[Num2] )
                return pLimitU[Num1] < pLimitU[Num2] ? -1 : 1;
        return 0;
	}
	// if ( iVar > 5 )
	{
        word * pLimit = pTruth + nWords;
		int i, iStep = Abc_TtWordNum(iVar);
        int Offset1 = Num1*iStep;
        int Offset2 = Num2*iStep;
        assert( nWords >= 4 );
		for ( pLimit -= 4*iStep; pLimit >= pTruth; pLimit -= 4*iStep )
			for ( i = iStep - 1; i >= 0; i-- )
                if ( pLimit[i + Offset1] != pLimit[i + Offset2] )
                    return pLimit[i + Offset1] < pLimit[i + Offset2] ? -1 : 1;
        return 0;
	}	
}

/**Function*************************************************************

  Synopsis    [Minterm counting in all cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtCountOnesInTruth( word * pTruth, int nVars )
{   
    int nWords = Abc_TtWordNum( nVars );
    int k, Counter = 0;
    for ( k = 0; k < nWords; k++ )
        if ( pTruth[k] )
            Counter += Abc_TtCountOnes( pTruth[k] );
    return Counter;
}
static inline void Abc_TtCountOnesInCofs( word * pTruth, int nVars, int * pStore )
{
    word Temp;
    int i, k, Counter, nWords;
    if ( nVars <= 6 )
    {
        for ( i = 0; i < nVars; i++ )
            pStore[i] = Abc_TtCountOnes( pTruth[0] & s_Truths6Neg[i] );
        return;
    }
    assert( nVars > 6 );
    nWords = Abc_TtWordNum( nVars );
    memset( pStore, 0, sizeof(int) * nVars );
    for ( k = 0; k < nWords; k++ )
    {
        // count 1's for the first six variables
        for ( i = 0; i < 6; i++ )
            if ( (Temp = (pTruth[k] & s_Truths6Neg[i]) | ((pTruth[k+1] & s_Truths6Neg[i]) << (1 << i))) )
                pStore[i] += Abc_TtCountOnes( Temp );
        // count 1's for all other variables
        if ( pTruth[k] )
        {
            Counter = Abc_TtCountOnes( pTruth[k] );
            for ( i = 6; i < nVars; i++ )
                if ( (k & (1 << (i-6))) == 0 )
                    pStore[i] += Counter;
        }
        k++;
        // count 1's for all other variables
        if ( pTruth[k] )
        {
            Counter = Abc_TtCountOnes( pTruth[k] );
            for ( i = 6; i < nVars; i++ )
                if ( (k & (1 << (i-6))) == 0 )
                    pStore[i] += Counter;
        }
    } 
}

/**Function*************************************************************

  Synopsis    [Minterm counting in all cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_TtCountOnesInCofsSlow( word * pTruth, int nVars, int * pStore )
{
    static int bit_count[256] = {
      0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
      1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
      1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
      2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
      1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
      2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
      2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
      3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
    };
    int i, k, nBytes;
    unsigned char * pTruthC = (unsigned char *)pTruth;
    nBytes = 8 * Abc_TtWordNum( nVars );
    memset( pStore, 0, sizeof(int) * nVars );
    for ( k = 0; k < nBytes; k++ )
    {
        pStore[0] += bit_count[ pTruthC[k] & 0x55 ];
        pStore[1] += bit_count[ pTruthC[k] & 0x33 ];
        pStore[2] += bit_count[ pTruthC[k] & 0x0F ];
        for ( i = 3; i < nVars; i++ )
            if ( (k & (1 << (i-3))) == 0 )
                pStore[i] += bit_count[pTruthC[k]];
    }
}

/**Function*************************************************************

  Synopsis    [Minterm counting in all cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_TtCountOnesInCofsFast6_rec( word Truth, int iVar, int nBytes, int * pStore )
{
    static int bit_count[256] = {
      0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
      1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
      1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
      2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
      1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
      2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
      2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
      3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
    };
    int nMints0, nMints1;
    if ( Truth == 0 )
        return 0;
    if ( ~Truth == 0 )
    {
        int i;
        for ( i = 0; i <= iVar; i++ )
            pStore[i] += nBytes * 4;
        return nBytes * 8;
    }
    if ( nBytes == 1 )
    {
        assert( iVar == 2 );
        pStore[0] += bit_count[ Truth & 0x55 ];
        pStore[1] += bit_count[ Truth & 0x33 ];
        pStore[2] += bit_count[ Truth & 0x0F ];
        return bit_count[ Truth & 0xFF ];
    }
    nMints0 = Abc_TtCountOnesInCofsFast6_rec( Abc_Tt6Cofactor0(Truth, iVar), iVar - 1, nBytes/2, pStore );
    nMints1 = Abc_TtCountOnesInCofsFast6_rec( Abc_Tt6Cofactor1(Truth, iVar), iVar - 1, nBytes/2, pStore );
    pStore[iVar] += nMints0;
    return nMints0 + nMints1;
}

int Abc_TtCountOnesInCofsFast_rec( word * pTruth, int iVar, int nWords, int * pStore )
{
    int nMints0, nMints1;
    if ( nWords == 1 )
    {
        assert( iVar == 5 );
        return Abc_TtCountOnesInCofsFast6_rec( pTruth[0], iVar, 8, pStore );
    }
    assert( nWords > 1 );
    assert( iVar > 5 );
    if ( pTruth[0] & 1 )
    {
        if ( Abc_TtIsConst1( pTruth, nWords ) )
        {
            int i;
            for ( i = 0; i <= iVar; i++ )
                pStore[i] += nWords * 32;
            return nWords * 64;
        }
    }
    else 
    {
        if ( Abc_TtIsConst0( pTruth, nWords ) )
            return 0;
    }
    nMints0 = Abc_TtCountOnesInCofsFast_rec( pTruth,            iVar - 1, nWords/2, pStore );
    nMints1 = Abc_TtCountOnesInCofsFast_rec( pTruth + nWords/2, iVar - 1, nWords/2, pStore );
    pStore[iVar] += nMints0;
    return nMints0 + nMints1;
}
int Abc_TtCountOnesInCofsFast( word * pTruth, int nVars, int * pStore )
{
    memset( pStore, 0, sizeof(int) * nVars );
    assert( nVars >= 3 );
    if ( nVars <= 6 )
        return Abc_TtCountOnesInCofsFast6_rec( pTruth[0], nVars - 1, Abc_TtByteNum( nVars ), pStore );
    else
        return Abc_TtCountOnesInCofsFast_rec( pTruth, nVars - 1, Abc_TtWordNum( nVars ), pStore );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Abc_TtSemiCanonicize( word * pTruth, int nVars, char * pCanonPerm, int * pStoreOut )
{
    int fOldSwap = 0;
    int pStoreIn[17];
    int * pStore = pStoreOut ? pStoreOut : pStoreIn;
    int i, nOnes, nWords = Abc_TtWordNum( nVars );
    unsigned uCanonPhase = 0;
    assert( nVars <= 16 );
    for ( i = 0; i < nVars; i++ )
        pCanonPerm[i] = i;
    // normalize polarity    
    nOnes = Abc_TtCountOnesInTruth( pTruth, nVars );
    if ( nOnes > nWords * 32 )
    {
        Abc_TtNot( pTruth, nWords );
        nOnes = nWords*64 - nOnes;
        uCanonPhase |= (1 << nVars);
    }
    // normalize phase
    Abc_TtCountOnesInCofs( pTruth, nVars, pStore );
    pStore[nVars] = nOnes;
    for ( i = 0; i < nVars; i++ )
    {
        if ( pStore[i] >= nOnes - pStore[i] )
            continue;
        Abc_TtFlip( pTruth, nWords, i );
        uCanonPhase |= (1 << i);
        pStore[i] = nOnes - pStore[i]; 
    }
    // normalize permutation
    if ( fOldSwap )
    {
        int fChange;
        do {
            fChange = 0;
            for ( i = 0; i < nVars-1; i++ )
            {
                if ( pStore[i] <= pStore[i+1] )
    //            if ( pStore[i] >= pStore[i+1] )
                    continue;
                ABC_SWAP( int, pCanonPerm[i], pCanonPerm[i+1] );
                ABC_SWAP( int, pStore[i], pStore[i+1] );
                if ( ((uCanonPhase >> i) & 1) != ((uCanonPhase >> (i+1)) & 1) )
                {
                    uCanonPhase ^= (1 << i);
                    uCanonPhase ^= (1 << (i+1));
                }
                Abc_TtSwapAdjacent( pTruth, nWords, i );            
                fChange = 1;
    //            nSwaps++;
            }
        } 
        while ( fChange );
    }
    else
    {
        int k, BestK;
        for ( i = 0; i < nVars - 1; i++ )
        {
            BestK = i + 1;
            for ( k = i + 2; k < nVars; k++ )
                if ( pStore[BestK] > pStore[k] )
    //            if ( pStore[BestK] < pStore[k] )
                    BestK = k;
            if ( pStore[i] <= pStore[BestK] )
    //        if ( pStore[i] >= pStore[BestK] )
                continue;
            ABC_SWAP( int, pCanonPerm[i], pCanonPerm[BestK] );
            ABC_SWAP( int, pStore[i], pStore[BestK] );
            if ( ((uCanonPhase >> i) & 1) != ((uCanonPhase >> BestK) & 1) )
            {
                uCanonPhase ^= (1 << i);
                uCanonPhase ^= (1 << BestK);
            }
            Abc_TtSwapVars( pTruth, nVars, i, BestK );
    //        nSwaps++;
        }
    }
    return uCanonPhase;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_TtCofactorTest10( word * pTruth, int nVars, int N )
{
    static word pCopy1[1024];
    static word pCopy2[1024];
    int nWords = Abc_TtWordNum( nVars );
    int i;
    for ( i = 0; i < nVars - 1; i++ )
    {
//        Kit_DsdPrintFromTruth( pTruth, nVars ); printf( "\n" );
        Abc_TtCopy( pCopy1, pTruth, nWords, 0 );
        Abc_TtSwapAdjacent( pCopy1, nWords, i );
//        Kit_DsdPrintFromTruth( pCopy1, nVars ); printf( "\n" );
        Abc_TtCopy( pCopy2, pTruth, nWords, 0 );
        Abc_TtSwapVars( pCopy2, nVars, i, i+1 );
//        Kit_DsdPrintFromTruth( pCopy2, nVars ); printf( "\n" );
        assert( Abc_TtEqual( pCopy1, pCopy2, nWords ) );
    }
}

/**Function*************************************************************

  Synopsis    [Naive evaluation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_Tt6CofactorPermNaive( word * pTruth, int i, int fSwapOnly )
{
    if ( fSwapOnly )
    {
        word Copy = Abc_Tt6SwapAdjacent( pTruth[0], i );
        if ( pTruth[0] > Copy )
        {
            pTruth[0] = Copy;
            return 4;
        }
        return 0;
    }
    {
        word Copy = pTruth[0];
        word Best = pTruth[0];
        int Config = 0;
        // PXY
        // 001
        Copy = Abc_Tt6Flip( Copy, i );
        if ( Best > Copy )
            Best = Copy, Config = 1;
        // PXY
        // 011
        Copy = Abc_Tt6Flip( Copy, i+1 );
        if ( Best > Copy )
            Best = Copy, Config = 3;
        // PXY
        // 010
        Copy = Abc_Tt6Flip( Copy, i );
        if ( Best > Copy )
            Best = Copy, Config = 2;
        // PXY
        // 110
        Copy = Abc_Tt6SwapAdjacent( Copy, i );
        if ( Best > Copy )
            Best = Copy, Config = 6;
        // PXY
        // 111
        Copy = Abc_Tt6Flip( Copy, i+1 );
        if ( Best > Copy )
            Best = Copy, Config = 7;
        // PXY
        // 101
        Copy = Abc_Tt6Flip( Copy, i );
        if ( Best > Copy )
            Best = Copy, Config = 5;
        // PXY
        // 100
        Copy = Abc_Tt6Flip( Copy, i+1 );
        if ( Best > Copy )
            Best = Copy, Config = 4;
        // PXY
        // 000
        Copy = Abc_Tt6SwapAdjacent( Copy, i );
        assert( Copy == pTruth[0] );
        assert( Best <= pTruth[0] );
        pTruth[0] = Best;
        return Config;
    }
}
int Abc_TtCofactorPermNaive( word * pTruth, int i, int nWords, int fSwapOnly )
{
    if ( fSwapOnly )
    {
        static word pCopy[1024];
        Abc_TtCopy( pCopy, pTruth, nWords, 0 );
        Abc_TtSwapAdjacent( pCopy, nWords, i );
        if ( Abc_TtCompareRev(pTruth, pCopy, nWords) == 1 )
        {
            Abc_TtCopy( pTruth, pCopy, nWords, 0 );
            return 4;
        }
        return 0;
    }
    {
        static word pCopy[1024];
        static word pBest[1024];
        int Config = 0;
        // save two copies
        Abc_TtCopy( pCopy, pTruth, nWords, 0 );
        Abc_TtCopy( pBest, pTruth, nWords, 0 );
        // PXY
        // 001
        Abc_TtFlip( pCopy, nWords, i );
        if ( Abc_TtCompareRev(pBest, pCopy, nWords) == 1 )
            Abc_TtCopy( pBest, pCopy, nWords, 0 ), Config = 1;
        // PXY
        // 011
        Abc_TtFlip( pCopy, nWords, i+1 );
        if ( Abc_TtCompareRev(pBest, pCopy, nWords) == 1 )
            Abc_TtCopy( pBest, pCopy, nWords, 0 ), Config = 3;
        // PXY
        // 010
        Abc_TtFlip( pCopy, nWords, i );
        if ( Abc_TtCompareRev(pBest, pCopy, nWords) == 1 )
            Abc_TtCopy( pBest, pCopy, nWords, 0 ), Config = 2;
        // PXY
        // 110
        Abc_TtSwapAdjacent( pCopy, nWords, i );
        if ( Abc_TtCompareRev(pBest, pCopy, nWords) == 1 )
            Abc_TtCopy( pBest, pCopy, nWords, 0 ), Config = 6;
        // PXY
        // 111
        Abc_TtFlip( pCopy, nWords, i+1 );
        if ( Abc_TtCompareRev(pBest, pCopy, nWords) == 1 )
            Abc_TtCopy( pBest, pCopy, nWords, 0 ), Config = 7;
        // PXY
        // 101
        Abc_TtFlip( pCopy, nWords, i );
        if ( Abc_TtCompareRev(pBest, pCopy, nWords) == 1 )
            Abc_TtCopy( pBest, pCopy, nWords, 0 ), Config = 5;
        // PXY
        // 100
        Abc_TtFlip( pCopy, nWords, i+1 );
        if ( Abc_TtCompareRev(pBest, pCopy, nWords) == 1 )
            Abc_TtCopy( pBest, pCopy, nWords, 0 ), Config = 4;
        // PXY
        // 000
        Abc_TtSwapAdjacent( pCopy, nWords, i );
        assert( Abc_TtEqual( pTruth, pCopy, nWords ) );
        if ( Config == 0 )
            return 0;
        assert( Abc_TtCompareRev(pTruth, pBest, nWords) == 1 );
        Abc_TtCopy( pTruth, pBest, nWords, 0 );
        return Config;
    }
}


/**Function*************************************************************

  Synopsis    [Smart evaluation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_TtCofactorPermConfig( word * pTruth, int i, int nWords, int fSwapOnly, int fNaive )
{
    if ( nWords == 1 )
        return Abc_Tt6CofactorPermNaive( pTruth, i, fSwapOnly );
    if ( fNaive )
        return Abc_TtCofactorPermNaive( pTruth, i, nWords, fSwapOnly );
    if ( fSwapOnly )
    {
        if ( Abc_TtCompare2VarCofsRev( pTruth, nWords, i, 1, 2 ) < 0 ) // Cof1 < Cof2
        {
            Abc_TtSwapAdjacent( pTruth, nWords, i );
            return 4;
        }
        return 0;
    }
    {  
        int fComp01, fComp02, fComp03, fComp12, fComp13, fComp23, Config = 0;
        fComp01 = Abc_TtCompare2VarCofsRev( pTruth, nWords, i, 0, 1 );
        fComp23 = Abc_TtCompare2VarCofsRev( pTruth, nWords, i, 2, 3 );
        if ( fComp23 >= 0 ) // Cof2 >= Cof3 
        {
            if ( fComp01 >= 0 ) // Cof0 >= Cof1
            {
                fComp13 = Abc_TtCompare2VarCofsRev( pTruth, nWords, i, 1, 3 );
                if ( fComp13 < 0 ) // Cof1 < Cof3 
                    Abc_TtFlip( pTruth, nWords, i + 1 ), Config = 2;
                else if ( fComp13 == 0 ) // Cof1 == Cof3 
                {
                    fComp02 = Abc_TtCompare2VarCofsRev( pTruth, nWords, i, 0, 2 );
                    if ( fComp02 < 0 )
                        Abc_TtFlip( pTruth, nWords, i + 1 ), Config = 2;
                }
                // else   Cof1 > Cof3 -- do nothing
            }
            else // Cof0 < Cof1
            {
                fComp03 = Abc_TtCompare2VarCofsRev( pTruth, nWords, i, 0, 3 );
                if ( fComp03 < 0 ) // Cof0 < Cof3
                {
                    Abc_TtFlip( pTruth, nWords, i );
                    Abc_TtFlip( pTruth, nWords, i + 1 ), Config = 3;
                }
                else //  Cof0 >= Cof3
                {
                    if ( fComp23 == 0 ) // can flip Cof0 and Cof1
                        Abc_TtFlip( pTruth, nWords, i ), Config = 1;
                }
            }
        }
        else // Cof2 < Cof3 
        {
            if ( fComp01 >= 0 ) // Cof0 >= Cof1
            {
                fComp12 = Abc_TtCompare2VarCofsRev( pTruth, nWords, i, 1, 2 );
                if ( fComp12 > 0 ) // Cof1 > Cof2 
                    Abc_TtFlip( pTruth, nWords, i ), Config = 1;
                else if ( fComp12 == 0 ) // Cof1 == Cof2 
                {
                    Abc_TtFlip( pTruth, nWords, i );
                    Abc_TtFlip( pTruth, nWords, i + 1 ), Config = 3;
                }
                else // Cof1 < Cof2
                {
                    Abc_TtFlip( pTruth, nWords, i + 1 ), Config = 2;
                    if ( fComp01 == 0 )
                        Abc_TtFlip( pTruth, nWords, i ), Config ^= 1;
                }
            }
            else // Cof0 < Cof1
            {
                fComp02 = Abc_TtCompare2VarCofsRev( pTruth, nWords, i, 0, 2 );
                if ( fComp02 == -1 ) // Cof0 < Cof2 
                {
                    Abc_TtFlip( pTruth, nWords, i );
                    Abc_TtFlip( pTruth, nWords, i + 1 ), Config = 3;
                }
                else if ( fComp02 == 0 ) // Cof0 == Cof2
                {
                    fComp13 = Abc_TtCompare2VarCofsRev( pTruth, nWords, i, 1, 3 );
                    if ( fComp13 >= 0 ) // Cof1 >= Cof3 
                        Abc_TtFlip( pTruth, nWords, i ), Config = 1;
                    else // Cof1 < Cof3 
                    {
                        Abc_TtFlip( pTruth, nWords, i );
                        Abc_TtFlip( pTruth, nWords, i + 1 ), Config = 3;
                    }
                }
                else // Cof0 > Cof2
                    Abc_TtFlip( pTruth, nWords, i ), Config = 1;
            }
        }
        // perform final swap if needed
        fComp12 = Abc_TtCompare2VarCofsRev( pTruth, nWords, i, 1, 2 );
        if ( fComp12 < 0 ) // Cof1 < Cof2
            Abc_TtSwapAdjacent( pTruth, nWords, i ), Config ^= 4;
        return Config;
    }
}
int Abc_TtCofactorPerm( word * pTruth, int i, int nWords, int fSwapOnly, char * pCanonPerm, unsigned * puCanonPhase, int fNaive )
{
    if ( fSwapOnly )
    {
        int Config = Abc_TtCofactorPermConfig( pTruth, i, nWords, 1, 0 );
        if ( Config )
        {
            if ( ((*puCanonPhase >> i) & 1) != ((*puCanonPhase >> (i+1)) & 1) )
            {
                *puCanonPhase ^= (1 << i);
                *puCanonPhase ^= (1 << (i+1));
            }
            ABC_SWAP( int, pCanonPerm[i], pCanonPerm[i+1] );
        }
        return Config;
    }
    {
        static word pCopy1[1024];
        int Config;
        Abc_TtCopy( pCopy1, pTruth, nWords, 0 );
        Config = Abc_TtCofactorPermConfig( pTruth, i, nWords, 0, fNaive );
        if ( Config == 0 )
            return 0;
        if ( Abc_TtCompareRev(pTruth, pCopy1, nWords) == 1 ) // made it worse
        {
            Abc_TtCopy( pTruth, pCopy1, nWords, 0 );
            return 0;
        }
        // improved
        if ( Config & 1 )
            *puCanonPhase ^= (1 << i);
        if ( Config & 2 )
            *puCanonPhase ^= (1 << (i+1));
        if ( Config & 4 )
        {
            if ( ((*puCanonPhase >> i) & 1) != ((*puCanonPhase >> (i+1)) & 1) )
            {
                *puCanonPhase ^= (1 << i);
                *puCanonPhase ^= (1 << (i+1));
            }
            ABC_SWAP( int, pCanonPerm[i], pCanonPerm[i+1] );
        }
        return Config;
    }
}

/**Function*************************************************************

  Synopsis    [Semi-canonical form computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
//#define CANON_VERIFY
unsigned Abc_TtCanonicize( word * pTruth, int nVars, char * pCanonPerm )
{
    int pStoreIn[17];
    unsigned uCanonPhase;
    int i, k, nWords = Abc_TtWordNum( nVars );
    int fNaive = 1;

#ifdef CANON_VERIFY
    char pCanonPermCopy[16];
    static word pCopy1[1024];
    static word pCopy2[1024];
    Abc_TtCopy( pCopy1, pTruth, nWords, 0 );
#endif

    uCanonPhase = Abc_TtSemiCanonicize( pTruth, nVars, pCanonPerm, pStoreIn );
    for ( k = 0; k < 5; k++ )
    {
        int fChanges = 0;
        for ( i = nVars - 2; i >= 0; i-- )
            if ( pStoreIn[i] == pStoreIn[i+1] )
                fChanges |= Abc_TtCofactorPerm( pTruth, i, nWords, pStoreIn[i] != pStoreIn[nVars]/2, pCanonPerm, &uCanonPhase, fNaive );
        if ( !fChanges )
            break;
        fChanges = 0;
        for ( i = 1; i < nVars - 1; i++ )
            if ( pStoreIn[i] == pStoreIn[i+1] )
                fChanges |= Abc_TtCofactorPerm( pTruth, i, nWords, pStoreIn[i] != pStoreIn[nVars]/2, pCanonPerm, &uCanonPhase, fNaive );
        if ( !fChanges )
            break;
    }

#ifdef CANON_VERIFY
    Abc_TtCopy( pCopy2, pTruth, nWords, 0 );
    memcpy( pCanonPermCopy, pCanonPerm, sizeof(char) * nVars );
    Abc_TtImplementNpnConfig( pCopy2, nVars, pCanonPermCopy, uCanonPhase );
    if ( !Abc_TtEqual( pCopy1, pCopy2, nWords ) )
        printf( "Canonical form verification failed!\n" );
#endif
/*
    if ( !Abc_TtEqual( pCopy1, pCopy2, nWords ) )
    {
        Kit_DsdPrintFromTruth( pCopy1, nVars ); printf( "\n" );
        Kit_DsdPrintFromTruth( pCopy2, nVars ); printf( "\n" );
        i = 0;
    }
*/
    return uCanonPhase;
}

/**Function*************************************************************

  Synopsis    [Semi-canonical form computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Abc_TtCanonicizePhaseVar6( word * pTruth, int nVars, int v )
{
    int w, nWords = Abc_TtWordNum( nVars );
    int s, nStep = 1 << (v-6);
    assert( v >= 6 );
    for ( w = nWords - 1, s = nWords - nStep; w > 0; w-- )
    {
        if ( pTruth[w-nStep] == pTruth[w] )
        {
            if ( w == s ) { w = s - nStep; s = w - nStep; }
            continue;
        }
        if ( pTruth[w-nStep] > pTruth[w] )
            return -1;
        for ( ; w > 0; w-- )
        {
            ABC_SWAP( word, pTruth[w-nStep], pTruth[w] );
            if ( w == s ) { w = s - nStep; s = w - nStep; }
        }
        assert( w == -1 );
        return 1;
    }
    return 0;
}
static inline int Abc_TtCanonicizePhaseVar5( word * pTruth, int nVars, int v )
{
    int w, nWords = Abc_TtWordNum( nVars );
    int Shift = 1 << v;
    word Mask = s_Truths6[v];
    assert( v < 6 );
    for ( w = nWords - 1; w >= 0; w-- )
    {
        if ( ((pTruth[w] << Shift) & Mask) == (pTruth[w] & Mask) )
            continue;
        if ( ((pTruth[w] << Shift) & Mask) > (pTruth[w] & Mask) )
            return -1;
//        Extra_PrintHex( stdout, (unsigned *)pTruth, nVars ); printf("\n" );
        for ( ; w >= 0; w-- )
            pTruth[w] = ((pTruth[w] << Shift) & Mask) | ((pTruth[w] & Mask) >> Shift);
//        Extra_PrintHex( stdout, (unsigned *)pTruth, nVars ); printf( " changed %d", v ), printf("\n" );
        return 1;
    }
    return 0;
}
unsigned Abc_TtCanonicizePhase( word * pTruth, int nVars )
{
    unsigned uCanonPhase = 0;
    int v, nWords = Abc_TtWordNum( nVars );
//    static int Counter = 0;
//    Counter++;

#ifdef CANON_VERIFY
    static word pCopy1[1024];
    static word pCopy2[1024];
    Abc_TtCopy( pCopy1, pTruth, nWords, 0 );
#endif

    if ( (pTruth[nWords-1] >> 63) & 1 )
    {
        Abc_TtNot( pTruth, nWords );
        uCanonPhase ^= (1 << nVars);
    }

//    while ( 1 )
//    {
//        unsigned uCanonPhase2 = uCanonPhase;
        for ( v = nVars - 1; v >= 6; v-- )
            if ( Abc_TtCanonicizePhaseVar6( pTruth, nVars, v ) == 1 )
                uCanonPhase ^= 1 << v;
        for ( ; v >= 0; v-- )
            if ( Abc_TtCanonicizePhaseVar5( pTruth, nVars, v ) == 1 )
                uCanonPhase ^= 1 << v;
//        if ( uCanonPhase2 == uCanonPhase )
//            break;
//    }

//    for ( v = 5; v >= 0; v-- )
//        assert( Abc_TtCanonicizePhaseVar5( pTruth, nVars, v ) != 1 );

#ifdef CANON_VERIFY
    Abc_TtCopy( pCopy2, pTruth, nWords, 0 );
    Abc_TtImplementNpnConfig( pCopy2, nVars, NULL, uCanonPhase );
    if ( !Abc_TtEqual( pCopy1, pCopy2, nWords ) )
        printf( "Canonical form verification failed!\n" );
#endif
    return uCanonPhase;
}


/**Function*************************************************************

  Synopsis    [Hierarchical semi-canonical form computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#define TT_NUM_TABLES 5

struct Abc_TtMan_t_
{
    Vec_Mem_t *   vTtMem[TT_NUM_TABLES];   // truth table memory and hash tables
    Vec_Int_t **  vRepres;                 // pointers to the representatives from the last hierarchical level
};

Vec_Int_t ** Abc_TtRepresStart() {
    Vec_Int_t ** vRepres = ABC_ALLOC(Vec_Int_t *, TT_NUM_TABLES - 1);
    int i;
    // create a list of pointers for each level of the hierarchy
    for (i = 0; i < (TT_NUM_TABLES - 1); i++) {
        vRepres[i] = Vec_IntAlloc(1);
    }
    return vRepres;
}

void Abc_TtRepresStop(Vec_Int_t ** vRepres) {
    int i;
    for (i = 0; i < (TT_NUM_TABLES - 1); i++) {
        Vec_IntFree(vRepres[i]);
    }
    ABC_FREE( vRepres );
}

Abc_TtMan_t * Abc_TtManStart( int nVars )
{
    Abc_TtMan_t * p = ABC_CALLOC( Abc_TtMan_t, 1 );
    int i, nWords = Abc_TtWordNum( nVars );
    for ( i = 0; i < TT_NUM_TABLES; i++ )
    {
        p->vTtMem[i] = Vec_MemAlloc( nWords, 12 );
        Vec_MemHashAlloc( p->vTtMem[i], 10000 );
    }
    p->vRepres = Abc_TtRepresStart();
    return p;
}
void Abc_TtManStop( Abc_TtMan_t * p )
{
    int i;
    for ( i = 0; i < TT_NUM_TABLES; i++ )
    {
        Vec_MemHashFree( p->vTtMem[i] );
        Vec_MemFreeP( &p->vTtMem[i] );
    }
    Abc_TtRepresStop(p->vRepres);
    ABC_FREE( p );
}
int Abc_TtManNumClasses( Abc_TtMan_t * p )
{
    return Vec_MemEntryNum( p->vTtMem[TT_NUM_TABLES-1] );
}

unsigned Abc_TtCanonicizeHie( Abc_TtMan_t * p, word * pTruthInit, int nVars, char * pCanonPerm, int fExact )
{
    int fNaive = 1;
    int pStore[17];
    static word pTruth[1024];
    unsigned uCanonPhase = 0;
    int nOnes, nWords = Abc_TtWordNum( nVars );
    int i, k, truthId;
    int * pSpot;
    int vTruthId[TT_NUM_TABLES-1];
    int fLevelFound;
    word * pRepTruth;
    assert( nVars <= 16 );

    Abc_TtCopy( pTruth, pTruthInit, nWords, 0 );

    for ( i = 0; i < nVars; i++ )
        pCanonPerm[i] = i;

    // normalize polarity    
    nOnes = Abc_TtCountOnesInTruth( pTruth, nVars );
    if ( nOnes > nWords * 32 )
    {
        Abc_TtNot( pTruth, nWords );
        nOnes = nWords*64 - nOnes;
        uCanonPhase |= (1 << nVars);
    }
    // check cache
    pSpot = Vec_MemHashLookup( p->vTtMem[0], pTruth );
    if ( *pSpot != -1 ) {
        fLevelFound = 0;
        goto end_repres;
    }
    vTruthId[0] = Vec_MemHashInsert( p->vTtMem[0], pTruth );

    // normalize phase
    Abc_TtCountOnesInCofs( pTruth, nVars, pStore );
    pStore[nVars] = nOnes;
    for ( i = 0; i < nVars; i++ )
    {
        if ( pStore[i] >= nOnes - pStore[i] )
            continue;
        Abc_TtFlip( pTruth, nWords, i );
        uCanonPhase |= (1 << i);
        pStore[i] = nOnes - pStore[i]; 
    }
    // check cache
    pSpot = Vec_MemHashLookup( p->vTtMem[1], pTruth );
    if ( *pSpot != -1 ) {
        fLevelFound = 1;
        goto end_repres;
    }
    vTruthId[1] = Vec_MemHashInsert( p->vTtMem[1], pTruth );

    // normalize permutation
    {
        int k, BestK;
        for ( i = 0; i < nVars - 1; i++ )
        {
            BestK = i + 1;
            for ( k = i + 2; k < nVars; k++ )
                if ( pStore[BestK] > pStore[k] )
                    BestK = k;
            if ( pStore[i] <= pStore[BestK] )
                continue;
            ABC_SWAP( int, pCanonPerm[i], pCanonPerm[BestK] );
            ABC_SWAP( int, pStore[i], pStore[BestK] );
            if ( ((uCanonPhase >> i) & 1) != ((uCanonPhase >> BestK) & 1) )
            {
                uCanonPhase ^= (1 << i);
                uCanonPhase ^= (1 << BestK);
            }
            Abc_TtSwapVars( pTruth, nVars, i, BestK );
        }
    }
    // check cache
    pSpot = Vec_MemHashLookup( p->vTtMem[2], pTruth );
    if ( *pSpot != -1 ) {
        fLevelFound = 2;
        goto end_repres;
    }
    vTruthId[2] = Vec_MemHashInsert( p->vTtMem[2], pTruth );

    // iterate TT permutations for tied variables
    for ( k = 0; k < 5; k++ )
    {
        int fChanges = 0;
        for ( i = nVars - 2; i >= 0; i-- )
            if ( pStore[i] == pStore[i+1] )
                fChanges |= Abc_TtCofactorPerm( pTruth, i, nWords, pStore[i] != pStore[nVars]/2, pCanonPerm, &uCanonPhase, fNaive );
        if ( !fChanges )
            break;
        fChanges = 0;
        for ( i = 1; i < nVars - 1; i++ )
            if ( pStore[i] == pStore[i+1] )
                fChanges |= Abc_TtCofactorPerm( pTruth, i, nWords, pStore[i] != pStore[nVars]/2, pCanonPerm, &uCanonPhase, fNaive );
        if ( !fChanges )
            break;
    }
    // check cache
    pSpot = Vec_MemHashLookup( p->vTtMem[3], pTruth );
    if ( *pSpot != -1 ) {
        fLevelFound = 3;
        goto end_repres;
    }
    vTruthId[3] = Vec_MemHashInsert( p->vTtMem[3], pTruth );

    // perform exact NPN using groups
    if ( fExact ) {
        extern void simpleMinimalGroups(word* x, word* pAux, word* minimal, int* pGroups, int nGroups, permInfo** pis, int nVars, int fFlipOutput, int fFlipInput);
        word pAuxWord[1024], pAuxWord1[1024];
        int pGroups[16];
        int nGroups = 0;
        permInfo * pis[17];
        // get groups
        pGroups[0] = 0;
        for (i = 0; i < nVars - 1; i++) {
            if (pStore[i] == pStore[i + 1]) {
                pGroups[nGroups]++;
            } else {
                pGroups[nGroups]++;
                nGroups++;
                pGroups[nGroups] = 0;
            }
        }
        pGroups[nGroups]++;
        nGroups++;

        // compute permInfo from 0 to nVars  (incl.)
        for (i = 0; i <= nVars; i++) {
            pis[i] = setPermInfoPtr(i);
        }

        // do the exact matching
        if (nOnes == nWords * 32) /* balanced output */
            simpleMinimalGroups(pTruth, pAuxWord, pAuxWord1, pGroups, nGroups, pis, nVars, 1, 1);
        else if (pStore[0] != pStore[1] && pStore[0] == (nOnes - pStore[0])) /* balanced singleton input */
            simpleMinimalGroups(pTruth, pAuxWord, pAuxWord1, pGroups, nGroups, pis, nVars, 0, 1);
        else
            simpleMinimalGroups(pTruth, pAuxWord, pAuxWord1, pGroups, nGroups, pis, nVars, 0, 0);

        // cleanup
        for (i = 0; i <= nVars; i++) {
            freePermInfoPtr(pis[i]);
        }
    }
    // check cache
    pSpot = Vec_MemHashLookup( p->vTtMem[4], pTruth );
    fLevelFound = 4;
    if ( *pSpot != -1 ) {
        goto end_repres;
    }
    *pSpot = Vec_MemHashInsert( p->vTtMem[4], pTruth );

end_repres:
    // return the class representative
    if(fLevelFound < (TT_NUM_TABLES - 1))
        truthId = Vec_IntEntry(p->vRepres[fLevelFound], *pSpot);
    else truthId = *pSpot;
    for(i = 0; i < fLevelFound; i++)
        Vec_IntSetEntry(p->vRepres[i], vTruthId[i], truthId);
    pRepTruth = Vec_MemReadEntry(p->vTtMem[TT_NUM_TABLES-1], truthId);
    Abc_TtCopy( pTruthInit, pRepTruth, nWords, 0 );

    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

