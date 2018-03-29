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
#include <math.h>

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

  Synopsis    [Compares Cof0 and Cof1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
static inline int Abc_TtCompare1VarCofs( word * pTruth, int nWords, int iVar )
{
    if ( nWords == 1 )
    {
        word Cof0 = pTruth[0] & s_Truths6Neg[iVar];
        word Cof1 = (pTruth[0] >> (1 << iVar)) & s_Truths6Neg[iVar];
        if ( Cof0 != Cof1 )
            return Cof0 < Cof1 ? -1 : 1;
        return 0;
    }
	if ( iVar <= 5 )
	{
        word Cof0, Cof1;
		int w, shift = (1 << iVar);
		for ( w = 0; w < nWords; w++ )
        {
            Cof0 = pTruth[w] & s_Truths6Neg[iVar];
            Cof1 = (pTruth[w] >> shift) & s_Truths6Neg[iVar];
            if ( Cof0 != Cof1 )
                return Cof0 < Cof1 ? -1 : 1;
        }
        return 0;
	}
	// if ( iVar > 5 )
	{
        word * pLimit = pTruth + nWords;
		int i, iStep = Abc_TtWordNum(iVar);
        assert( nWords >= 2 );
		for ( ; pTruth < pLimit; pTruth += 2*iStep )
			for ( i = 0; i < iStep; i++ )
                if ( pTruth[i] != pTruth[i + iStep] )
                    return pTruth[i] < pTruth[i + iStep] ? -1 : 1;
        return 0;
	}	
}
static inline int Abc_TtCompare1VarCofsRev( word * pTruth, int nWords, int iVar )
{
    if ( nWords == 1 )
    {
        word Cof0 = pTruth[0] & s_Truths6Neg[iVar];
        word Cof1 = (pTruth[0] >> (1 << iVar)) & s_Truths6Neg[iVar];
        if ( Cof0 != Cof1 )
            return Cof0 < Cof1 ? -1 : 1;
        return 0;
    }
	if ( iVar <= 5 )
	{
        word Cof0, Cof1;
		int w, shift = (1 << iVar);
		for ( w = nWords - 1; w >= 0; w-- )
        {
            Cof0 = pTruth[w] & s_Truths6Neg[iVar];
            Cof1 = (pTruth[w] >> shift) & s_Truths6Neg[iVar];
            if ( Cof0 != Cof1 )
                return Cof0 < Cof1 ? -1 : 1;
        }
        return 0;
	}
	// if ( iVar > 5 )
	{
        word * pLimit = pTruth + nWords;
		int i, iStep = Abc_TtWordNum(iVar);
        assert( nWords >= 2 );
		for ( pLimit -= 2*iStep; pLimit >= pTruth; pLimit -= 2*iStep )
			for ( i = iStep - 1; i >= 0; i-- )
                if ( pLimit[i] != pLimit[i + iStep] )
                    return pLimit[i] < pLimit[i + iStep] ? -1 : 1;
        return 0;
	}	
}
*/

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
#define DO_SMALL_TRUTHTABLE 0

static inline void Abc_TtNormalizeSmallTruth(word * pTruth, int nVars)
{
#if DO_SMALL_TRUTHTABLE
	if (nVars < 6)
		*pTruth &= (1ULL << (1 << nVars)) - 1;
#endif
}

static inline int Abc_TtCountOnesInTruth( word * pTruth, int nVars )
{   
    int nWords = Abc_TtWordNum( nVars );
    int k, Counter = 0;
    Abc_TtNormalizeSmallTruth(pTruth, nVars);
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
        Abc_TtNormalizeSmallTruth(pTruth, nVars);
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
#define TT_MAX_LEVELS 5

struct Abc_TtHieMan_t_
{
	int  nLastLevel, nWords;
	Vec_Mem_t *  vTtMem[TT_MAX_LEVELS];    // truth table memory and hash tables
	Vec_Int_t *  vRepres[TT_MAX_LEVELS];   // pointers to the representatives from the last hierarchical level
	int         vTruthId[TT_MAX_LEVELS];
};

Abc_TtHieMan_t * Abc_TtHieManStart(int nVars, int nLevels)
{
	Abc_TtHieMan_t * p = NULL;
	int i;
	if (nLevels > TT_MAX_LEVELS) return p;
	p = ABC_CALLOC(Abc_TtHieMan_t, 1);
	p->nLastLevel = nLevels - 1;
	p->nWords = Abc_TtWordNum(nVars);
	for (i = 0; i < nLevels; i++)
	{
		p->vTtMem[i] = Vec_MemAlloc(p->nWords, 12);
		Vec_MemHashAlloc(p->vTtMem[i], 10000);
		p->vRepres[i] = Vec_IntAlloc(1);
	}
	return p;
}

void Abc_TtHieManStop(Abc_TtHieMan_t * p)
{
	int i;
	for (i = 0; i <= p->nLastLevel; i++)
	{
		Vec_MemHashFree(p->vTtMem[i]);
		Vec_MemFreeP(&p->vTtMem[i]);
		Vec_IntFree(p->vRepres[i]);
	}
	ABC_FREE(p);
}

int Abc_TtHieRetrieveOrInsert(Abc_TtHieMan_t * p, int level, word * pTruth, word * pResult)
{
	int i, iSpot, truthId;
	word * pRepTruth;
	if (level < 0) level += p->nLastLevel + 1;
	if (level < 0 || level > p->nLastLevel) return -1;
	iSpot = *Vec_MemHashLookup(p->vTtMem[level], pTruth);
	if (iSpot == -1) {
		p->vTruthId[level] = Vec_MemHashInsert(p->vTtMem[level], pTruth);
		if (level < p->nLastLevel) return 0;
		iSpot = p->vTruthId[level];
	}
	// return the class representative
	if (level < p->nLastLevel)
		truthId = Vec_IntEntry(p->vRepres[level], iSpot);
	else
		truthId = iSpot;
	for (i = 0; i < level; i++)
		Vec_IntSetEntry(p->vRepres[i], p->vTruthId[i], truthId);

	pRepTruth = Vec_MemReadEntry(p->vTtMem[p->nLastLevel], truthId);
	if (level < p->nLastLevel) {
		Abc_TtCopy(pResult, pRepTruth, p->nWords, 0);
		return 1;
	}
	assert(Abc_TtEqual(pTruth, pRepTruth, p->nWords));
	if (pTruth != pResult)
		Abc_TtCopy(pResult, pRepTruth, p->nWords, 0);
	return 0;
}

unsigned Abc_TtCanonicizeHie( Abc_TtHieMan_t * p, word * pTruthInit, int nVars, char * pCanonPerm, int fExact )
{
    int fNaive = 1;
    int pStore[17];
    static word pTruth[1024];
    unsigned uCanonPhase = 0;
    int nOnes, nWords = Abc_TtWordNum( nVars );
    int i, k;
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
    if (Abc_TtHieRetrieveOrInsert(p, 0, pTruth, pTruthInit) > 0) return 0;

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
	if (Abc_TtHieRetrieveOrInsert(p, 1, pTruth, pTruthInit) > 0) return 0;

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
	if (Abc_TtHieRetrieveOrInsert(p, 2, pTruth, pTruthInit) > 0) return 0;

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
	if (Abc_TtHieRetrieveOrInsert(p, 3, pTruth, pTruthInit) > 0) return 0;

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
    // update cache
	Abc_TtHieRetrieveOrInsert(p, 4, pTruth, pTruthInit);
    return 0;
}


/**Function*************************************************************

Synopsis    [Adaptive exact/semi-canonical form computation.]

Description []

SideEffects []

SeeAlso     []

***********************************************************************/

typedef struct TiedGroup_
{
	char iStart;                         // index of Abc_TgMan_t::pPerm
	char nGVars;                         // the number of variables in the group
	char fPhased;                        // if the phases of the variables are determined
} TiedGroup;

typedef struct Abc_TgMan_t_
{
	word *pTruth;
	int nVars;                          // the number of variables
	int nGVars;                         // the number of variables in groups ( symmetric variables purged )
	int nGroups;                        // the number of tied groups
	unsigned uPhase;                    // phase of each variable and the function
	char pPerm[16];                     // permutation of variables, symmetric variables purged, for grouping
	char pPermT[16];                    // permutation of variables, symmetric variables expanded, actual transformation for pTruth
	char pPermTRev[16];                 // reverse permutation of pPermT
	signed char pPermDir[16];			// for generating the next permutation
	TiedGroup pGroup[16];               // tied groups
	// symemtric group attributes
	char symPhase[16];                  // phase type of symemtric groups
	signed char symLink[17];            // singly linked list, indicate the variables in symemtric groups
} Abc_TgMan_t;

#if !defined(NDEBUG) && !defined(CANON_VERIFY)
#define CANON_VERIFY
#endif
/**Function*************************************************************

Synopsis    [Utilities.]

Description []

SideEffects []

SeeAlso     []

***********************************************************************/

// Johnson¨CTrotter algorithm
static int Abc_NextPermSwapC(char * pData, signed char * pDir, int size)
{
	int i, j, k = -1;
	for (i = 0; i < size; i++)
	{
		j = i + pDir[i];
		if (j >= 0 && j < size && pData[i] > pData[j] && (k < 0 || pData[i] > pData[k]))
			k = i;
	}
	if (k < 0) k = 0;

	for (i = 0; i < size; i++)
		if (pData[i] > pData[k])
			pDir[i] = -pDir[i];

	j = k + pDir[k];
	return j < k ? j : k;
}


typedef unsigned(*TtCanonicizeFunc)(Abc_TtHieMan_t * p, word * pTruth, int nVars, char * pCanonPerm, int flag);
unsigned Abc_TtCanonicizeWrap(TtCanonicizeFunc func, Abc_TtHieMan_t * p, word * pTruth, int nVars, char * pCanonPerm, int flag)
{
	int nWords = Abc_TtWordNum(nVars);
	unsigned uCanonPhase1, uCanonPhase2;
	char pCanonPerm2[16];
	static word pTruth2[1024];

	if (Abc_TtCountOnesInTruth(pTruth, nVars) != (1 << (nVars - 1)))
		return func(p, pTruth, nVars, pCanonPerm, flag);
	Abc_TtCopy(pTruth2, pTruth, nWords, 1);
	Abc_TtNormalizeSmallTruth(pTruth2, nVars);
	uCanonPhase1 = func(p, pTruth, nVars, pCanonPerm, flag);
	uCanonPhase2 = func(p, pTruth2, nVars, pCanonPerm2, flag);
	if (Abc_TtCompareRev(pTruth, pTruth2, nWords) <= 0)
		return uCanonPhase1;
	Abc_TtCopy(pTruth, pTruth2, nWords, 0);
	memcpy(pCanonPerm, pCanonPerm2, nVars);
	return uCanonPhase2;
}

word gpVerCopy[1024];
static int Abc_TtCannonVerify(word* pTruth, int nVars, char * pCanonPerm, unsigned uCanonPhase)
{
#ifdef CANON_VERIFY
	int nWords = Abc_TtWordNum(nVars);
	char pCanonPermCopy[16];
	static word pCopy2[1024];
	Abc_TtCopy(pCopy2, pTruth, nWords, 0);
	memcpy(pCanonPermCopy, pCanonPerm, sizeof(char) * nVars);
	Abc_TtImplementNpnConfig(pCopy2, nVars, pCanonPermCopy, uCanonPhase);
	Abc_TtNormalizeSmallTruth(pCopy2, nVars);
	return Abc_TtEqual(gpVerCopy, pCopy2, nWords);
#else
	return 1;
#endif
}

/**Function*************************************************************

Synopsis    [Tied group management.]

Description []

SideEffects []

SeeAlso     []

***********************************************************************/

static void Abc_TginitMan(Abc_TgMan_t * pMan, word * pTruth, int nVars)
{
	int i;
	pMan->pTruth = pTruth;
	pMan->nVars = pMan->nGVars = nVars;
	pMan->uPhase = 0;
	for (i = 0; i < nVars; i++)
	{
		pMan->pPerm[i] = i;
		pMan->pPermT[i] = i;
		pMan->pPermTRev[i] = i;
		pMan->symPhase[i] = 1;
	}
}

static inline void Abc_TgManCopy(Abc_TgMan_t* pDst, word* pDstTruth, Abc_TgMan_t* pSrc)
{
	*pDst = *pSrc;
	Abc_TtCopy(pDstTruth, pSrc->pTruth, Abc_TtWordNum(pSrc->nVars), 0);
	pDst->pTruth = pDstTruth;
}

static inline int Abc_TgCannonVerify(Abc_TgMan_t* pMan)
{
	return Abc_TtCannonVerify(pMan->pTruth, pMan->nVars, pMan->pPermT, pMan->uPhase);
}

void Abc_TgExpendSymmetry(Abc_TgMan_t * pMan, char * pPerm, char * pDest);
static void CheckConfig(Abc_TgMan_t * pMan)
{
#ifndef NDEBUG
	int i;
	char pPermE[16];
	Abc_TgExpendSymmetry(pMan, pMan->pPerm, pPermE);
	for (i = 0; i < pMan->nVars; i++)
	{
		assert(pPermE[i] == pMan->pPermT[i]);
		assert(pMan->pPermTRev[pMan->pPermT[i]] == i);
	}
	assert(Abc_TgCannonVerify(pMan));
#endif
}

/**Function*************************************************************

Synopsis    [Truthtable manipulation.]

Description []

SideEffects []

SeeAlso     []

***********************************************************************/

static inline void Abc_TgFlipVar(Abc_TgMan_t* pMan, int iVar)
{
	int nWords = Abc_TtWordNum(pMan->nVars);
	int ivp = pMan->pPermTRev[iVar];
	Abc_TtFlip(pMan->pTruth, nWords, ivp);
	pMan->uPhase ^= 1 << ivp;
}

static inline void Abc_TgFlipSymGroupByVar(Abc_TgMan_t* pMan, int iVar)
{
	for (; iVar >= 0; iVar = pMan->symLink[iVar])
		if (pMan->symPhase[iVar])
			Abc_TgFlipVar(pMan, iVar);
}

static inline void Abc_TgFlipSymGroup(Abc_TgMan_t* pMan, int idx)
{
	Abc_TgFlipSymGroupByVar(pMan, pMan->pPerm[idx]);
}

static inline void Abc_TgClearSymGroupPhase(Abc_TgMan_t* pMan, int iVar)
{
	for (; iVar >= 0; iVar = pMan->symLink[iVar])
		pMan->symPhase[iVar] = 0;
}

static void Abc_TgImplementPerm(Abc_TgMan_t* pMan, const char *pPermDest)
{
	int i, nVars = pMan->nVars;
	char *pPerm = pMan->pPermT;
	char *pRev = pMan->pPermTRev;
	unsigned uPhase = pMan->uPhase & (1 << nVars);

	for (i = 0; i < nVars; i++)
		pRev[pPerm[i]] = i;
	for (i = 0; i < nVars; i++)
		pPerm[i] = pRev[pPermDest[i]];
	for (i = 0; i < nVars; i++)
		pRev[pPerm[i]] = i;

	Abc_TtImplementNpnConfig(pMan->pTruth, nVars, pRev, 0);
	Abc_TtNormalizeSmallTruth(pMan->pTruth, nVars);

	for (i = 0; i < nVars; i++)
	{
		if (pMan->uPhase & (1 << pPerm[i]))
			uPhase |= (1 << i);
		pPerm[i] = pPermDest[i];
		pRev[pPerm[i]] = i;
	}
	pMan->uPhase = uPhase;
}

static void Abc_TgSwapAdjacentSymGroups(Abc_TgMan_t* pMan, int idx)
{
	int iVar, jVar, ix;
	char pPermNew[16];
	assert(idx < pMan->nGVars - 1);
	iVar = pMan->pPerm[idx];
	jVar = pMan->pPerm[idx + 1];
	pMan->pPerm[idx] = jVar;
	pMan->pPerm[idx + 1] = iVar;
	ABC_SWAP(char, pMan->pPermDir[idx], pMan->pPermDir[idx + 1]);
	if (pMan->symLink[iVar] >= 0 || pMan->symLink[jVar] >= 0)
	{
		Abc_TgExpendSymmetry(pMan, pMan->pPerm, pPermNew);
		Abc_TgImplementPerm(pMan, pPermNew);
		return;
	}
	// plain variable swap
	ix = pMan->pPermTRev[iVar];
	assert(pMan->pPermT[ix] == iVar && pMan->pPermT[ix + 1] == jVar);
	Abc_TtSwapAdjacent(pMan->pTruth, Abc_TtWordNum(pMan->nVars), ix);
	pMan->pPermT[ix] = jVar;
	pMan->pPermT[ix + 1] = iVar;
	pMan->pPermTRev[iVar] = ix + 1;
	pMan->pPermTRev[jVar] = ix;
	if (((pMan->uPhase >> ix) & 1) != ((pMan->uPhase >> (ix + 1)) & 1))
		pMan->uPhase ^= 1 << ix | 1 << (ix + 1);
	assert(Abc_TgCannonVerify(pMan));
}

/**Function*************************************************************

Synopsis    [semmetry of two variables.]

Description []

SideEffects []

SeeAlso     []

***********************************************************************/

static word pSymCopy[1024];

static int Abc_TtIsSymmetric(word * pTruth, int nVars, int iVar, int jVar, int fPhase)
{
	int rv;
	int nWords = Abc_TtWordNum(nVars);
	Abc_TtCopy(pSymCopy, pTruth, nWords, 0);
	Abc_TtSwapVars(pSymCopy, nVars, iVar, jVar);
	rv = Abc_TtEqual(pTruth, pSymCopy, nWords) * 2;
	if (!fPhase) return rv;
	Abc_TtFlip(pSymCopy, nWords, iVar);
	Abc_TtFlip(pSymCopy, nWords, jVar);
	return rv + Abc_TtEqual(pTruth, pSymCopy, nWords);
}

static int Abc_TtIsSymmetricHigh(Abc_TgMan_t * pMan, int iVar, int jVar, int fPhase)
{
	int rv, iv, jv, n;
	int nWords = Abc_TtWordNum(pMan->nVars);
	Abc_TtCopy(pSymCopy, pMan->pTruth, nWords, 0);
	for (n = 0, iv = iVar, jv = jVar; iv >= 0 && jv >= 0; iv = pMan->symLink[iv], jv = pMan->symLink[jv], n++)
		Abc_TtSwapVars(pSymCopy, pMan->nVars, iv, jv);
	assert(iv < 0 && jv < 0);	// two symmetric groups must have the same size 
	rv = Abc_TtEqual(pMan->pTruth, pSymCopy, nWords) * 2;
	if (!fPhase) return rv;
	for (iv = iVar, jv = jVar; iv >= 0 && jv >= 0; iv = pMan->symLink[iv], jv = pMan->symLink[jv])
	{
		if (pMan->symPhase[iv]) Abc_TtFlip(pSymCopy, nWords, iv);
		if (pMan->symPhase[jv]) Abc_TtFlip(pSymCopy, nWords, jv);
	}
	return rv + Abc_TtEqual(pMan->pTruth, pSymCopy, nWords);
}

/**Function*************************************************************

Synopsis    [Create groups by cofactor signatures]

Description [Similar to Abc_TtSemiCanonicize. 
             Use stable insertion sort to keep the order of the variables in the groups.
			 Defer permutation. ]

SideEffects []

SeeAlso     []

***********************************************************************/

static void Abc_TgCreateGroups(Abc_TgMan_t * pMan)
{
	int pStore[17];
	int i, j, nOnes;
	int nVars = pMan->nVars, nWords = Abc_TtWordNum(nVars);
	TiedGroup * pGrp = pMan->pGroup;
	assert(nVars <= 16);
	// normalize polarity    
	nOnes = Abc_TtCountOnesInTruth(pMan->pTruth, nVars);
	if (nOnes > (1 << (nVars - 1)))
	{
		Abc_TtNot(pMan->pTruth, nWords);
		nOnes = (1 << nVars) - nOnes;
		pMan->uPhase |= (1 << nVars);
	}
	// normalize phase
	Abc_TtCountOnesInCofs(pMan->pTruth, nVars, pStore);
	pStore[nVars] = nOnes;
	for (i = 0; i < nVars; i++)
	{
		if (pStore[i] >= nOnes - pStore[i])
			continue;
		Abc_TtFlip(pMan->pTruth, nWords, i);
		pMan->uPhase |= (1 << i);
		pStore[i] = nOnes - pStore[i];
	}

	// sort variables
	for (i = 1; i < nVars; i++)
	{
		int a = pStore[i]; char aa = pMan->pPerm[i];
		for (j = i; j > 0 && pStore[j - 1] > a; j--)
			pStore[j] = pStore[j - 1], pMan->pPerm[j] = pMan->pPerm[j - 1];
		pStore[j] = a; pMan->pPerm[j] = aa;
	}
	// group variables
//	Abc_SortIdxC(pStore, pMan->pPerm, nVars);
	pGrp[0].iStart = 0;
	pGrp[0].fPhased = pStore[0] * 2 != nOnes;
	for (i = j = 1; i < nVars; i++)
	{
		if (pStore[i] == pStore[i - 1]) continue;
		pGrp[j].iStart = i;
		pGrp[j].fPhased = pStore[i] * 2 != nOnes;
		pGrp[j - 1].nGVars = i - pGrp[j - 1].iStart;
		j++;
	}
	pGrp[j - 1].nGVars = i - pGrp[j - 1].iStart;
	pMan->nGroups = j;
}

/**Function*************************************************************

Synopsis    [Group symmetric variables.]

Description []

SideEffects []

SeeAlso     []

***********************************************************************/

static int Abc_TgGroupSymmetry(Abc_TgMan_t * pMan, TiedGroup * pGrp, int doHigh)
{
	int i, j, iVar, jVar, nsym = 0;
	int fDone[16], scnt[16], stype[16];
	signed char *symLink = pMan->symLink;
//	char * symPhase = pMan->symPhase;
	int nGVars = pGrp->nGVars;
	char * pVars = pMan->pPerm + pGrp->iStart;
	int modified, order = 0;

	for (i = 0; i < nGVars; i++)
		fDone[i] = 0, scnt[i] = 1;

	do {
		modified = 0;
		for (i = 0; i < nGVars - 1; i++)
		{
			iVar = pVars[i];
			if (iVar < 0 || fDone[i]) continue;
//			if (!pGrp->fPhased && !Abc_TtHasVar(pMan->pTruth, pMan->nVars, iVar)) continue;
			// Mark symmetric variables/groups
			for (j = i + 1; j < nGVars; j++)
			{
				jVar = pVars[j];
				if (jVar < 0 || scnt[j] != scnt[i])	// || pMan->symPhase[jVar] != pMan->symPhase[iVar])
					stype[j] = 0;
				else if (scnt[j] == 1)
					stype[j] = Abc_TtIsSymmetric(pMan->pTruth, pMan->nVars, iVar, jVar, !pGrp->fPhased);
				else
					stype[j] = Abc_TtIsSymmetricHigh(pMan, iVar, jVar, !pGrp->fPhased);
			}
			fDone[i] = 1;
			// Merge symmetric groups
			for (j = i + 1; j < nGVars; j++)
			{
				int ii;
				jVar = pVars[j];
				switch (stype[j])
				{
				case 1:			// E-Symmetry
					Abc_TgFlipSymGroupByVar(pMan, jVar);
					// fallthrough
				case 2:			// NE-Symmetry
					pMan->symPhase[iVar] += pMan->symPhase[jVar];
					break;
				case 3:			// multiform Symmetry
					Abc_TgClearSymGroupPhase(pMan, jVar);
					break;
				default:		// case 0: No Symmetry
					continue;
				}

				for (ii = iVar; symLink[ii] >= 0; ii = symLink[ii])
					;
				symLink[ii] = jVar;
				pVars[j] = -1;
				scnt[i] += scnt[j];
				modified = 1;
				fDone[i] = 0;
				nsym++;
			}
		}
//		if (++order > 3) printf("%d", order);
	} while (doHigh && modified);

	return nsym;
}

static void Abc_TgPurgeSymmetry(Abc_TgMan_t * pMan, int doHigh)
{
	int i, j, k, sum = 0, nVars = pMan->nVars;
	signed char *symLink = pMan->symLink;
	char gcnt[16] = { 0 };
	char * pPerm = pMan->pPerm;

	for (i = 0; i <= nVars; i++)
		symLink[i] = -1;

	// purge unsupported variables
	if (!pMan->pGroup[0].fPhased)
	{
		int iVar = pMan->nVars;
		for (j = 0; j < pMan->pGroup[0].nGVars; j++)
		{
			int jVar = pPerm[j];
			assert(jVar >= 0);
			if (!Abc_TtHasVar(pMan->pTruth, nVars, jVar))
			{
				symLink[jVar] = symLink[iVar];
				symLink[iVar] = jVar;
				pPerm[j] = -1;
				gcnt[0]++;
			}
		}
	}

	for (k = 0; k < pMan->nGroups; k++)
		gcnt[k] += Abc_TgGroupSymmetry(pMan, pMan->pGroup + k, doHigh);

	for (i = 0; i < nVars && pPerm[i] >= 0; i++)
		;
	for (j = i + 1; ; i++, j++)
	{
		while (j < nVars && pPerm[j] < 0) j++;
		if (j >= nVars) break;
		pPerm[i] = pPerm[j];
	}
	for (k = 0; k < pMan->nGroups; k++)
	{
		pMan->pGroup[k].nGVars -= gcnt[k];
		pMan->pGroup[k].iStart -= sum;
		sum += gcnt[k];
	}
	if (pMan->pGroup[0].nGVars == 0)
	{
		pMan->nGroups--;
		memmove(pMan->pGroup, pMan->pGroup + 1, sizeof(TiedGroup) * pMan->nGroups);
		assert(pMan->pGroup[0].iStart == 0);
	}
	pMan->nGVars -= sum;
}

void Abc_TgExpendSymmetry(Abc_TgMan_t * pMan, char * pPerm, char * pDest)
{
	int i = 0, j, k;
	for (j = 0; j < pMan->nGVars; j++)
		for (k = pPerm[j]; k >= 0; k = pMan->symLink[k])
			pDest[i++] = k;
	for (k = pMan->symLink[pMan->nVars]; k >= 0; k = pMan->symLink[k])
		pDest[i++] = k;
	assert(i == pMan->nVars);
}


/**Function*************************************************************

Synopsis    [Semi-canonical form computation.]

Description [simple config enumeration]

SideEffects []

SeeAlso     []

***********************************************************************/
static int Abc_TgSymGroupPerm(Abc_TgMan_t* pMan, int idx, TiedGroup* pTGrp)
{
	word* pTruth = pMan->pTruth;
	static word pCopy[1024];
	static word pBest[1024];
	int Config = 0;
	int nWords = Abc_TtWordNum(pMan->nVars);
	Abc_TgMan_t tgManCopy, tgManBest;
	int fSwapOnly = pTGrp->fPhased;

	CheckConfig(pMan);
	if (fSwapOnly)
	{
		Abc_TgManCopy(&tgManCopy, pCopy, pMan);
		Abc_TgSwapAdjacentSymGroups(&tgManCopy, idx);
		CheckConfig(&tgManCopy);
		if (Abc_TtCompareRev(pTruth, pCopy, nWords) < 0)
		{
			Abc_TgManCopy(pMan, pTruth, &tgManCopy);
			return 4;
		}
		return 0;
	}

	// save two copies
	Abc_TgManCopy(&tgManCopy, pCopy, pMan);
	Abc_TgManCopy(&tgManBest, pBest, pMan);
	// PXY
	// 001
	Abc_TgFlipSymGroup(&tgManCopy, idx);
	CheckConfig(&tgManCopy);
	if (Abc_TtCompareRev(pBest, pCopy, nWords) == 1)
		Abc_TgManCopy(&tgManBest, pBest, &tgManCopy), Config = 1;
	// PXY
	// 011
	Abc_TgFlipSymGroup(&tgManCopy, idx + 1);
	CheckConfig(&tgManCopy);
	if (Abc_TtCompareRev(pBest, pCopy, nWords) == 1)
		Abc_TgManCopy(&tgManBest, pBest, &tgManCopy), Config = 3;
	// PXY
	// 010
	Abc_TgFlipSymGroup(&tgManCopy, idx);
	CheckConfig(&tgManCopy);
	if (Abc_TtCompareRev(pBest, pCopy, nWords) == 1)
		Abc_TgManCopy(&tgManBest, pBest, &tgManCopy), Config = 2;
	// PXY
	// 110
	Abc_TgSwapAdjacentSymGroups(&tgManCopy, idx);
	CheckConfig(&tgManCopy);
	if (Abc_TtCompareRev(pBest, pCopy, nWords) == 1)
		Abc_TgManCopy(&tgManBest, pBest, &tgManCopy), Config = 6;
	// PXY
	// 111
	Abc_TgFlipSymGroup(&tgManCopy, idx + 1);
	CheckConfig(&tgManCopy);
	if (Abc_TtCompareRev(pBest, pCopy, nWords) == 1)
		Abc_TgManCopy(&tgManBest, pBest, &tgManCopy), Config = 7;
	// PXY
	// 101
	Abc_TgFlipSymGroup(&tgManCopy, idx);
	CheckConfig(&tgManCopy);
	if (Abc_TtCompareRev(pBest, pCopy, nWords) == 1)
		Abc_TgManCopy(&tgManBest, pBest, &tgManCopy), Config = 5;
	// PXY
	// 100
	Abc_TgFlipSymGroup(&tgManCopy, idx + 1);
	CheckConfig(&tgManCopy);
	if (Abc_TtCompareRev(pBest, pCopy, nWords) == 1)
		Abc_TgManCopy(&tgManBest, pBest, &tgManCopy), Config = 4;
	// PXY
	// 000
	Abc_TgSwapAdjacentSymGroups(&tgManCopy, idx);
	CheckConfig(&tgManCopy);
	assert(Abc_TtEqual(pTruth, pCopy, nWords));
	if (Config == 0)
		return 0;
	assert(Abc_TtCompareRev(pTruth, pBest, nWords) == 1);
	Abc_TgManCopy(pMan, pTruth, &tgManBest);
	return Config;
}

static int Abc_TgPermPhase(Abc_TgMan_t* pMan, int iVar)
{
	static word pCopy[1024];
	int nWords = Abc_TtWordNum(pMan->nVars);
	int ivp = pMan->pPermTRev[iVar];
	Abc_TtCopy(pCopy, pMan->pTruth, nWords, 0);
	Abc_TtFlip(pCopy, nWords, ivp);
	if (Abc_TtCompareRev(pMan->pTruth, pCopy, nWords) == 1)
	{
		Abc_TtCopy(pMan->pTruth, pCopy, nWords, 0);
		pMan->uPhase ^= 1 << ivp;
		return 16;
	}
	return 0;
}

static void Abc_TgSimpleEnumeration(Abc_TgMan_t * pMan)
{
	int i, j, k;
	int pGid[16];

	for (k = j = 0; j < pMan->nGroups; j++)
		for (i = 0; i < pMan->pGroup[j].nGVars; i++, k++)
			pGid[k] = j;
	assert(k == pMan->nGVars);

	for (k = 0; k < 5; k++)
	{
		int fChanges = 0;
		for (i = pMan->nGVars - 2; i >= 0; i--)
			if (pGid[i] == pGid[i + 1])
				fChanges |= Abc_TgSymGroupPerm(pMan, i, pMan->pGroup + pGid[i]);
		for (i = 1; i < pMan->nGVars - 1; i++)
			if (pGid[i] == pGid[i + 1])
				fChanges |= Abc_TgSymGroupPerm(pMan, i, pMan->pGroup + pGid[i]);

		for (i = pMan->nVars - 1; i >= 0; i--)
			if (pMan->symPhase[i])
				fChanges |= Abc_TgPermPhase(pMan, i);
		for (i = 1; i < pMan->nVars; i++)
			if (pMan->symPhase[i])
				fChanges |= Abc_TgPermPhase(pMan, i);
		if (!fChanges) break;
	}
	assert(Abc_TgCannonVerify(pMan));
}

/**Function*************************************************************

Synopsis    [Exact canonical form computation.]

Description [full config enumeration]

SideEffects []

SeeAlso     []

***********************************************************************/
// enumeration time = exp((cost-27.12)*0.59)
static int Abc_TgEnumerationCost(Abc_TgMan_t * pMan)
{
	int cSym = 0;
	double cPerm = 0.0;
	TiedGroup * pGrp = 0;
	int i, j, n;
	if (pMan->nGroups == 0) return 0;

	for (i = 0; i < pMan->nGroups; i++)
	{
		pGrp = pMan->pGroup + i;
		n = pGrp->nGVars;
		if (n > 1)
			cPerm += 0.92 + log(n) / 2 + n * (log(n) - 1);
	}
	if (pMan->pGroup->fPhased)
		n = 0;
	else
	{
		char * pVars = pMan->pPerm;
		n = pMan->pGroup->nGVars;
		for (i = 0; i < n; i++)
			for (j = pVars[i]; j >= 0; j = pMan->symLink[j])
				cSym++;
	}
	// coefficients computed by linear regression
	return pMan->nVars + n * 1.09 + cPerm * 1.65 + 0.5; 
//	return (rv > 60 ? 100000000 : 0) + n * 1000000 + cSym * 10000 + cPerm * 100 + 0.5;
}

static int Abc_TgIsInitPerm(char * pData, signed char * pDir, int size)
{
	int i;
	if (pDir[0] != -1) return 0;
	for (i = 1; i < size; i++)
		if (pDir[i] != -1 || pData[i] < pData[i - 1])
			return 0;
	return 1;
}

static void Abc_TgFirstPermutation(Abc_TgMan_t * pMan)
{
	int i;
	for (i = 0; i < pMan->nGVars; i++)
		pMan->pPermDir[i] = -1;
#ifndef NDEBUG
	for (i = 0; i < pMan->nGroups; i++)
	{
		TiedGroup * pGrp = pMan->pGroup + i;
		int nGvars = pGrp->nGVars;
		char * pVars = pMan->pPerm + pGrp->iStart;
		signed char * pDirs = pMan->pPermDir + pGrp->iStart;
		assert(Abc_TgIsInitPerm(pVars, pDirs, nGvars));
	}
#endif
}

static int Abc_TgNextPermutation(Abc_TgMan_t * pMan)
{
	int i, j, nGvars;
	TiedGroup * pGrp;
	char * pVars;
	signed char * pDirs;
	for (i = 0; i < pMan->nGroups; i++)
	{
		pGrp = pMan->pGroup + i;
		nGvars = pGrp->nGVars;
		if (nGvars == 1) continue;
		pVars = pMan->pPerm + pGrp->iStart;
		pDirs = pMan->pPermDir + pGrp->iStart;
		j = Abc_NextPermSwapC(pVars, pDirs, nGvars);
		if (j >= 0)
		{
			Abc_TgSwapAdjacentSymGroups(pMan, j + pGrp->iStart);
			return 1;
		}
		Abc_TgSwapAdjacentSymGroups(pMan, pGrp->iStart);
		assert(Abc_TgIsInitPerm(pVars, pDirs, nGvars));
	}
	return 0;
}

static inline unsigned grayCode(unsigned a) { return a ^ (a >> 1); }

static int grayFlip(unsigned a, int n)
{
	unsigned d = grayCode(a) ^ grayCode(a + 1);
	int i;
	for (i = 0; i < n; i++)
		if (d == 1U << i) return i;
	assert(0);
    return -1;
}

static inline void Abc_TgSaveBest(Abc_TgMan_t * pMan, Abc_TgMan_t * pBest)
{
	if (Abc_TtCompare(pBest->pTruth, pMan->pTruth, Abc_TtWordNum(pMan->nVars)) == 1)
		Abc_TgManCopy(pBest, pBest->pTruth, pMan);
}

static void Abc_TgPhaseEnumeration(Abc_TgMan_t * pMan, Abc_TgMan_t * pBest)
{
	char pFGrps[16];
	TiedGroup * pGrp = pMan->pGroup;
	int i, j, n = pGrp->nGVars;

	Abc_TgSaveBest(pMan, pBest);
	if (pGrp->fPhased) return;

	// sort by symPhase
	for (i = 0; i < n; i++)
	{
		char iv = pMan->pPerm[i];		
		for (j = i; j > 0 && pMan->symPhase[pFGrps[j-1]] > pMan->symPhase[iv]; j--)
			pFGrps[j] = pFGrps[j - 1];
		pFGrps[j] = iv;
	}

	for (i = 0; i < (1 << n) - 1; i++)
	{
		Abc_TgFlipSymGroupByVar(pMan, pFGrps[grayFlip(i, n)]);
		Abc_TgSaveBest(pMan, pBest);
	}
}

static void Abc_TgFullEnumeration(Abc_TgMan_t * pWork, Abc_TgMan_t * pBest)
{
//	static word pCopy[1024];
//	Abc_TgMan_t tgManCopy;
//	Abc_TgManCopy(&tgManCopy, pCopy, pMan);

	Abc_TgFirstPermutation(pWork);
	do Abc_TgPhaseEnumeration(pWork, pBest);
	while (Abc_TgNextPermutation(pWork));
	pBest->uPhase |= 1U << 30;
}

unsigned Abc_TtCanonicizeAda(Abc_TtHieMan_t * p, word * pTruth, int nVars, char * pCanonPerm, int iThres)
{
	int nWords = Abc_TtWordNum(nVars);
	unsigned fExac = 0, fHash = 1U << 29;
	static word pCopy[1024];
	Abc_TgMan_t tgMan, tgManCopy;
	int iCost;
	const int MaxCost = 84;  // maximun posible cost for function with 16 inputs 
	const int doHigh = iThres / 100, iEnumThres = iThres % 100;

#ifdef CANON_VERIFY
	Abc_TtCopy(gpVerCopy, pTruth, nWords, 0);
#endif

	assert(nVars <= 16);
	if (p && Abc_TtHieRetrieveOrInsert(p, -5, pTruth, pTruth) > 0) return fHash;
	Abc_TginitMan(&tgMan, pTruth, nVars);
	Abc_TgCreateGroups(&tgMan);
	if (p && Abc_TtHieRetrieveOrInsert(p, -4, pTruth, pTruth) > 0) return fHash;
	Abc_TgPurgeSymmetry(&tgMan, doHigh);

	Abc_TgExpendSymmetry(&tgMan, tgMan.pPerm, pCanonPerm);
	Abc_TgImplementPerm(&tgMan, pCanonPerm);
	assert(Abc_TgCannonVerify(&tgMan));

	if (p == NULL) {
		if (iEnumThres > MaxCost || Abc_TgEnumerationCost(&tgMan) < iEnumThres) {
			Abc_TgManCopy(&tgManCopy, pCopy, &tgMan);
			Abc_TgFullEnumeration(&tgManCopy, &tgMan);
		}
		else
			Abc_TgSimpleEnumeration(&tgMan);
	}
	else {
		iCost = Abc_TgEnumerationCost(&tgMan);
		if (iCost < iEnumThres) fExac = 1U << 30;
		if (Abc_TtHieRetrieveOrInsert(p, -3, pTruth, pTruth) > 0) return fHash + fExac;
		Abc_TgManCopy(&tgManCopy, pCopy, &tgMan);
		Abc_TgSimpleEnumeration(&tgMan);
		if (Abc_TtHieRetrieveOrInsert(p, -2, pTruth, pTruth) > 0) return fHash + fExac;
		if (fExac) {
			Abc_TgManCopy(&tgMan, pTruth, &tgManCopy);
			Abc_TgFullEnumeration(&tgManCopy, &tgMan);
		}
		Abc_TtHieRetrieveOrInsert(p, -1, pTruth, pTruth);
	}
	memcpy(pCanonPerm, tgMan.pPermT, sizeof(char) * nVars);

#ifdef CANON_VERIFY
	if (!Abc_TgCannonVerify(&tgMan))
		printf("Canonical form verification failed!\n");
#endif
	return tgMan.uPhase;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

