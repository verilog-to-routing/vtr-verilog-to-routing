/**CFile****************************************************************

  FileName    [ifDec10.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Performs additional check.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifDec10.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"
#include "misc/extra/extra.h"
#include "bool/kit/kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// the bit count for the first 256 integer numbers
static int BitCount8[256] = {
    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};
// variable swapping code
static word PMasks[5][3] = {
    { ABC_CONST(0x9999999999999999), ABC_CONST(0x2222222222222222), ABC_CONST(0x4444444444444444) },
    { ABC_CONST(0xC3C3C3C3C3C3C3C3), ABC_CONST(0x0C0C0C0C0C0C0C0C), ABC_CONST(0x3030303030303030) },
    { ABC_CONST(0xF00FF00FF00FF00F), ABC_CONST(0x00F000F000F000F0), ABC_CONST(0x0F000F000F000F00) },
    { ABC_CONST(0xFF0000FFFF0000FF), ABC_CONST(0x0000FF000000FF00), ABC_CONST(0x00FF000000FF0000) },
    { ABC_CONST(0xFFFF00000000FFFF), ABC_CONST(0x00000000FFFF0000), ABC_CONST(0x0000FFFF00000000) }
};
// elementary truth tables
static word Truth6[6] = {
    ABC_CONST(0xAAAAAAAAAAAAAAAA),
    ABC_CONST(0xCCCCCCCCCCCCCCCC),
    ABC_CONST(0xF0F0F0F0F0F0F0F0),
    ABC_CONST(0xFF00FF00FF00FF00),
    ABC_CONST(0xFFFF0000FFFF0000),
    ABC_CONST(0xFFFFFFFF00000000)
};
static word Truth10[10][16] = {
    {ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA)},
    {ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC)},
    {ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0)},
    {ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00)},
    {ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000)},
    {ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000)},
    {ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF)},
    {ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF)},
    {ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF)},
    {ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF)}
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

static inline int If_Dec10WordNum( int nVars )
{
    return nVars <= 6 ? 1 : 1 << (nVars-6);
}
static void If_Dec10PrintConfigOne( unsigned z )
{
    unsigned t;
    t = (z & 0xffff) | ((z & 0xffff) << 16);
    Extra_PrintBinary( stdout, &t, 16 ); 
    printf( " " );
    Kit_DsdPrintFromTruth( &t, 4 );
    printf( " " );
    printf( " %d", (z >> 16) & 7 );
    printf( " %d", (z >> 20) & 7 );
    printf( " %d", (z >> 24) & 7 );
    printf( " %d", (z >> 28) & 7 );
    printf( "\n" );
}
void If_Dec10PrintConfig( unsigned * pZ )
{
    while ( *pZ )
        If_Dec10PrintConfigOne( *pZ++ );
}
static inline void If_Dec10ComposeLut4( int t, word ** pF, word * pR, int nVars )
{
    word pC[16];
    int m, w, v, nWords;
    assert( nVars <= 10 );
    nWords = If_Dec10WordNum( nVars );
    for ( w = 0; w < nWords; w++ )
        pR[w] = 0;
    for ( m = 0; m < 16; m++ )
    {
        if ( !((t >> m) & 1) )
            continue;
        for ( w = 0; w < nWords; w++ )
            pC[w] = ~(word)0;
        for ( v = 0; v < 4; v++ )
            for ( w = 0; w < nWords; w++ )
                pC[w] &= ((m >> v) & 1) ? pF[v][w] : ~pF[v][w];
        for ( w = 0; w < nWords; w++ )
            pR[w] |= pC[w];
    }
}
void If_Dec10Verify( word * pF, int nVars, unsigned * pZ )
{
    word pN[16][16], * pG[4];
    int i, w, v, k, nWords;
    unsigned z;
    nWords = If_Dec10WordNum( nVars );
    for ( k = 0; k < nVars; k++ )
        for ( w = 0; w < nWords; w++ )
            pN[k][w] = Truth10[k][w];
    for ( i = 0; (z = pZ[i]); i++, k++ )
    {
        for ( v = 0; v < 4; v++ )
            pG[v] = pN[ (z >> (16+(v<<2))) & 7 ];
        If_Dec10ComposeLut4( (int)(z & 0xffff), pG, pN[k], nVars );
    }
    k--;
    for ( w = 0; w < nWords; w++ )
        if ( pN[k][w] != pF[w] )
        {
            If_Dec10PrintConfig( pZ );
            Kit_DsdPrintFromTruth( (unsigned*)pF, nVars ); printf( "\n" );
            Kit_DsdPrintFromTruth( (unsigned*)pN[k], nVars ); printf( "\n" );
            printf( "Verification failed!\n" );
            break;
        }
}


// count the number of unique cofactors
static inline int If_Dec10CofCount2( word * pF, int nVars )
{
    int nShift = (1 << (nVars - 4));
    word Mask  = (((word)1) << nShift) - 1;
    word iCof0 = pF[0] & Mask;
    word iCof1 = iCof0, iCof;
    int i;
    assert( nVars > 6 && nVars <= 10 );
    if ( nVars == 10 )
        Mask = ~(word)0;
    for ( i = 1; i < 16; i++ )
    {
        iCof = (pF[(i * nShift) / 64] >> ((i * nShift) & 63)) & Mask;
        if ( iCof == iCof0 )
            continue;
        if ( iCof1 == iCof0 )
            iCof1 = iCof;
        else if ( iCof != iCof1 )
            return 3;
    }
    return 2;
}
static inline int If_Dec10CofCount( word * pF, int nVars )
{
    int nShift = (1 << (nVars - 4));
    word Mask  = (((word)1) << nShift) - 1;
    word iCofs[16], iCof;
    int i, c, nCofs = 1;
    if ( nVars == 10 )
        Mask = ~(word)0;
    iCofs[0] = pF[0] & Mask;
    for ( i = 1; i < 16; i++ )
    {
        iCof = (pF[(i * nShift) / 64] >> ((i * nShift) & 63)) & Mask;
        for ( c = 0; c < nCofs; c++ )
            if ( iCof == iCofs[c] )
                break;
        if ( c == nCofs )
            iCofs[nCofs++] = iCof;
    }
    return nCofs;
}


// variable permutation for large functions
static inline void If_Dec10Copy( word * pOut, word * pIn, int nVars )
{
    int w, nWords = If_Dec10WordNum( nVars );
    for ( w = 0; w < nWords; w++ )
        pOut[w] = pIn[w];
}
static inline void If_Dec10SwapAdjacent( word * pOut, word * pIn, int iVar, int nVars )
{
    int i, k, nWords = If_Dec10WordNum( nVars );
    assert( iVar < nVars - 1 );
    if ( iVar < 5 )
    {
        int Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
            pOut[i] = (pIn[i] & PMasks[iVar][0]) | ((pIn[i] & PMasks[iVar][1]) << Shift) | ((pIn[i] & PMasks[iVar][2]) >> Shift);
    }
    else if ( iVar > 5 )
    {
        int Step = (1 << (iVar - 6));
        for ( k = 0; k < nWords; k += 4*Step )
        {
            for ( i = 0; i < Step; i++ )
                pOut[i] = pIn[i];
            for ( i = 0; i < Step; i++ )
                pOut[Step+i] = pIn[2*Step+i];
            for ( i = 0; i < Step; i++ )
                pOut[2*Step+i] = pIn[Step+i];
            for ( i = 0; i < Step; i++ )
                pOut[3*Step+i] = pIn[3*Step+i];
            pIn  += 4*Step;
            pOut += 4*Step;
        }
    }
    else // if ( iVar == 5 )
    {
        for ( i = 0; i < nWords; i += 2 )
        {
            pOut[i]   = (pIn[i]   & ABC_CONST(0x00000000FFFFFFFF)) | ((pIn[i+1] & ABC_CONST(0x00000000FFFFFFFF)) << 32);
            pOut[i+1] = (pIn[i+1] & ABC_CONST(0xFFFFFFFF00000000)) | ((pIn[i]   & ABC_CONST(0xFFFFFFFF00000000)) >> 32);
        }
    }
}
static inline void If_Dec10MoveTo( word * pF, int nVars, int v, int p, int Pla2Var[], int Var2Pla[] )
{
    word pG[16], * pIn = pF, * pOut = pG, * pTemp;
    int iPlace0, iPlace1, Count = 0;
    assert( Var2Pla[v] <= p );
    while ( Var2Pla[v] != p )
    {
        iPlace0 = Var2Pla[v];
        iPlace1 = Var2Pla[v]+1;
        If_Dec10SwapAdjacent( pOut, pIn, iPlace0, nVars );
        pTemp = pIn; pIn = pOut, pOut = pTemp;
        Var2Pla[Pla2Var[iPlace0]]++;
        Var2Pla[Pla2Var[iPlace1]]--;
        Pla2Var[iPlace0] ^= Pla2Var[iPlace1];
        Pla2Var[iPlace1] ^= Pla2Var[iPlace0];
        Pla2Var[iPlace0] ^= Pla2Var[iPlace1];
        Count++;
    }
    if ( Count & 1 )
        If_Dec10Copy( pF, pIn, nVars );
    assert( Pla2Var[p] == v );
}
/*
// derive binary function
static inline int If_Dec10DeriveCount2( word * pF, word * pRes, int nVars )
{
    int nShift = (1 << (nVars - 4));
    word Mask  = (((word)1) << nShift) - 1;
    int i, MaskDec = 0;
	word iCof0 = pF[0] & Mask;
	word iCof1 = pF[0] & Mask;
    word iCof, * pCof0, * pCof1;
    if ( nVars == 10 )
        Mask = ~(word)0;
    for ( i = 1; i < 16; i++ )
    {
        iCof = (pF[(i * nShift) / 64] >> ((i * nShift) & 63)) & Mask;
        if ( *pCof0 != iCof )
		{
			*pCof1 = iCof;
			MaskDec |= (1 << i);
		}
    }
    *pRes = ((iCof1 << nShift) | iCof0);
	return MaskDec;
}
static inline word If_DecTruthStretch( word t, int nVars )
{
    assert( nVars > 1 );
    if ( nVars == 1 )
        nVars++, t = (t & 0x3) | ((t & 0x3) << 2);
    if ( nVars == 2 )
        nVars++, t = (t & 0xF) | ((t & 0xF) << 4);
    if ( nVars == 3 )
        nVars++, t = (t & 0xFF) | ((t & 0xFF) << 8);
    if ( nVars == 4 )
        nVars++, t = (t & 0xFFFF) | ((t & 0xFFFF) << 16);
    if ( nVars == 5 )
        nVars++, t = (t & 0xFFFFFFFF) | ((t & 0xFFFFFFFF) << 32);
    assert( nVars >= 6 );
}
*/

// support minimization
static inline int If_DecSuppIsMinBase( int Supp )
{
    return (Supp & (Supp+1)) == 0;
}
static inline int If_Dec10HasVar( word * t, int nVars, int iVar )
{
    int nWords = If_Dec10WordNum( nVars );
    assert( iVar < nVars );
    if ( iVar < 6 )
    {
        int i, Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
            if ( (t[i] & ~Truth6[iVar]) != ((t[i] & Truth6[iVar]) >> Shift) )
                return 1;
        return 0;
    }
    else
    {
        int i, k, Step = (1 << (iVar - 6));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
                if ( t[i] != t[Step+i] )
                    return 1;
            t += 2*Step;
        }
        return 0;
    }
}
static inline int If_Dec10Support( word * t, int nVars )
{
    int v, Supp = 0;
    for ( v = 0; v < nVars; v++ )
        if ( If_Dec10HasVar( t, nVars, v ) )
            Supp |= (1 << v);
    return Supp;
}

// checks
void If_Dec10Cofactors( word * pF, int nVars, int iVar, word * pCof0, word * pCof1 )
{
    int nWords = If_Dec10WordNum( nVars );
    assert( iVar < nVars );
    if ( iVar < 6 )
    {
        int i, Shift = (1 << iVar);
        for ( i = 0; i < nWords; i++ )
        {
            pCof0[i] = (pF[i] & ~Truth6[iVar]) | ((pF[i] & ~Truth6[iVar]) << Shift);
            pCof1[i] = (pF[i] &  Truth6[iVar]) | ((pF[i] &  Truth6[iVar]) >> Shift);
        }
        return;
    }
    else
    {
        int i, k, Step = (1 << (iVar - 6));
        for ( k = 0; k < nWords; k += 2*Step )
        {
            for ( i = 0; i < Step; i++ )
            {
                pCof0[i] = pCof0[Step+i] = pF[i];
                pCof1[i] = pCof1[Step+i] = pF[Step+i];
            }
            pF    += 2*Step;
            pCof0 += 2*Step;
            pCof1 += 2*Step;
        }
        return;
    }
}
static inline int If_Dec10Count16( int Num16 )
{
    assert( Num16 < (1<<16)-1 );
    return BitCount8[Num16 & 255] + BitCount8[(Num16 >> 8) & 255];
}
static inline void If_DecVerifyPerm( int Pla2Var[10], int Var2Pla[10], int nVars )
{
    int i;
    for ( i = 0; i < nVars; i++ )
        assert( Pla2Var[Var2Pla[i]] == i );
} 
int If_Dec10Perform( word * pF, int nVars, int fDerive )
{ 
//    static int Cnt = 0;
    word pCof0[16], pCof1[16];
    int Pla2Var[10], Var2Pla[10], Count[210], Masks[210];
    int i, i0,i1,i2,i3, v, x;
    assert( nVars >= 6 && nVars <= 10 );
    // start arrays
    for ( i = 0; i < nVars; i++ )
    {
        assert( If_Dec10HasVar( pF, nVars, i ) );
        Pla2Var[i] = Var2Pla[i] = i;
    }
/*
    Cnt++;
//if ( Cnt == 108 )
{
printf( "%d\n", Cnt );
//Extra_PrintHex( stdout, (unsigned *)pF, nVars );
//printf( "\n" ); 
Kit_DsdPrintFromTruth( (unsigned *)pF, nVars );
printf( "\n" );
printf( "\n" );
}
*/
    // generate permutations
    v = 0;
    for ( i0 = 0;    i0 < nVars; i0++ )
    for ( i1 = i0+1; i1 < nVars; i1++ )
    for ( i2 = i1+1; i2 < nVars; i2++ )
    for ( i3 = i2+1; i3 < nVars; i3++, v++ )
    {
        If_Dec10MoveTo( pF, nVars, i0, nVars-1, Pla2Var, Var2Pla );
        If_Dec10MoveTo( pF, nVars, i1, nVars-2, Pla2Var, Var2Pla );
        If_Dec10MoveTo( pF, nVars, i2, nVars-3, Pla2Var, Var2Pla );
        If_Dec10MoveTo( pF, nVars, i3, nVars-4, Pla2Var, Var2Pla );
        If_DecVerifyPerm( Pla2Var, Var2Pla, nVars );
        Count[v] = If_Dec10CofCount( pF, nVars );
        Masks[v] = (1 << i0) | (1 << i1) | (1 << i2) | (1 << i3);
        assert( Count[v] > 1 );
//printf( "%d ", Count[v] );
        if ( Count[v] == 2 || Count[v] > 5 )
            continue;
        for ( x = 0; x < 4; x++ )
        {
            If_Dec10Cofactors( pF, nVars, nVars-1-x, pCof0, pCof1 );
            if ( If_Dec10CofCount2(pCof0, nVars) <= 2 && If_Dec10CofCount2(pCof1, nVars) <= 2 )
            {
                Count[v] = -Count[v];
                break;
            }
        }
    }
//printf( "\n" );
    assert( v <= 210 );
    // check if there are compatible bound sets
    for ( i0 = 0; i0 < v; i0++ )
    for ( i1 = i0+1; i1 < v; i1++ )
    {
        if ( If_Dec10Count16(Masks[i0] & Masks[i1]) > 10 - nVars )
            continue;
        if ( nVars == 10 )
        {
            if ( Count[i0] == 2 && Count[i1] == 2 )
                return 1;
        }
        else if ( nVars == 9 )
        {
            if ( (Count[i0] == 2 && Count[i1] == 2) || 
                 (Count[i0] == 2 && Count[i1] <  0) || 
                 (Count[i0] <  0 && Count[i1] == 2) )
                return 1;
        }
        else
        {
            if ( (Count[i0] == 2 && Count[i1] == 2) || 
                 (Count[i0] == 2 && Count[i1] <  0) || 
                 (Count[i0] <  0 && Count[i1] == 2) || 
                 (Count[i0] <  0 && Count[i1] <  0) )
                return 1;
        }
    }
//    printf( "not found\n" );
    return 0;
}


/**Function*************************************************************

  Synopsis    [Performs additional check.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutPerformCheck10( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr )
{
	int nSupp, fDerive = 0;
    word pF[16];
    if ( nLeaves <= 6 )
        return 1;
    If_Dec10Copy( pF, (word *)pTruth, nVars );
    nSupp = If_Dec10Support( pF, nLeaves );
    if ( !nSupp || !If_DecSuppIsMinBase(nSupp) )
        return 0;
    if ( If_Dec10Perform( pF, nLeaves, fDerive ) )
    {
//        printf( "1" );
        return 1;
//        If_Dec10Verify( t, nLeaves, NULL );
    }
//    printf( "0" );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

