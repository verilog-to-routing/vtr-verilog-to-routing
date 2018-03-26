/**CFile****************************************************************

  FileName    [ifDec07.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Performs additional check.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifDec07.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

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
static word Truth7[7][2] = {
    {ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA)},
    {ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC)},
    {ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0)},
    {ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00)},
    {ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000)},
    {ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000)},
    {ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF)}
};

extern void Kit_DsdPrintFromTruth( unsigned * pTruth, int nVars );
extern void Extra_PrintBinary( FILE * pFile, unsigned Sign[], int nBits );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

void If_DecPrintConfig( word z )
{
   unsigned S[1];
   S[0] = (z & 0xffff) | ((z & 0xffff) << 16);
   Extra_PrintBinary( stdout, S, 16 ); 
   printf( " " );
   Kit_DsdPrintFromTruth( S, 4 );
   printf( " " );
   printf( " %d", (int)((z >> 16) & 7) );
   printf( " %d", (int)((z >> 20) & 7) );
   printf( " %d", (int)((z >> 24) & 7) );
   printf( " %d", (int)((z >> 28) & 7) );
   printf( "   " );
   S[0] = ((z >> 32) & 0xffff) | (((z >> 32) & 0xffff) << 16);
   Extra_PrintBinary( stdout, S, 16 );
   printf( " " );
   Kit_DsdPrintFromTruth( S, 4 );
   printf( " " );
   printf( " %d", (int)((z >> 48) & 7) );
   printf( " %d", (int)((z >> 52) & 7) );
   printf( " %d", (int)((z >> 56) & 7) );
   printf( " %d", (int)((z >> 60) & 7) );
   printf( "\n" );
}

// verification for 6-input function
static word If_Dec6ComposeLut4( int t, word f[4] )
{
    int m, v;
    word c, r = 0;
    for ( m = 0; m < 16; m++ )
    {
        if ( !((t >> m) & 1) )
            continue;
        c = ~(word)0;
        for ( v = 0; v < 4; v++ )
            c &= ((m >> v) & 1) ? f[v] : ~f[v];
        r |= c;
    }
    return r;
}
word If_Dec6Truth( word z )
{
    word r, q, f[4];
    int i, v;
    assert( z );
    for ( i = 0; i < 4; i++ )
    {
        v = (z >> (16+(i<<2))) & 7;
        assert( v != 7 );
        if ( v == 6 )
            continue;
        f[i] = Truth6[v];
    }
    q = If_Dec6ComposeLut4( (int)(z & 0xffff), f );
    for ( i = 0; i < 4; i++ )
    {
        v = (z >> (48+(i<<2))) & 7;
        if ( v == 6 )
            continue;
        f[i] = (v == 7) ? q : Truth6[v];
    }
    r = If_Dec6ComposeLut4( (int)((z >> 32) & 0xffff), f );
    return r;
}
void If_Dec6Verify( word t, word z )
{
    word r = If_Dec6Truth( z );
    if ( r != t )
    {
        If_DecPrintConfig( z );
        Kit_DsdPrintFromTruth( (unsigned*)&t, 6 ); printf( "\n" );
//        Kit_DsdPrintFromTruth( (unsigned*)&q, 6 ); printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned*)&r, 6 ); printf( "\n" );
        printf( "Verification failed!\n" );
    }
}
// verification for 7-input function
static void If_Dec7ComposeLut4( int t, word f[4][2], word r[2] )
{
    int m, v;
    word c[2];
    r[0] = r[1] = 0;
    for ( m = 0; m < 16; m++ )
    {
        if ( !((t >> m) & 1) )
            continue;
        c[0] = c[1] = ~(word)0;
        for ( v = 0; v < 4; v++ )
        {
            c[0] &= ((m >> v) & 1) ? f[v][0] : ~f[v][0];
            c[1] &= ((m >> v) & 1) ? f[v][1] : ~f[v][1];
        }
        r[0] |= c[0];
        r[1] |= c[1];
    }
}
void If_Dec7Verify( word t[2], word z )
{
    word f[4][2], r[2];
    int i, v;
    assert( z );
    for ( i = 0; i < 4; i++ )
    {
        v = (z >> (16+(i<<2))) & 7;
        f[i][0] = Truth7[v][0];
        f[i][1] = Truth7[v][1];
    }
    If_Dec7ComposeLut4( (int)(z & 0xffff), f, r );
    f[3][0] = r[0];
    f[3][1] = r[1];
    for ( i = 0; i < 3; i++ )
    {
        v = (z >> (48+(i<<2))) & 7;
        f[i][0] = Truth7[v][0];
        f[i][1] = Truth7[v][1];
    }
    If_Dec7ComposeLut4( (int)((z >> 32) & 0xffff), f, r );
    if ( r[0] != t[0] || r[1] != t[1] )
    {
        If_DecPrintConfig( z );
        Kit_DsdPrintFromTruth( (unsigned*)t, 7 ); printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned*)r, 7 ); printf( "\n" );
        printf( "Verification failed!\n" );
    }
}


// count the number of unique cofactors
static inline int If_Dec6CofCount2( word t )
{
    int i, Mask = 0;
    for ( i = 0; i < 16; i++ )
        Mask |= (1 << ((t >> (i<<2)) & 15));
    return BitCount8[((unsigned char*)&Mask)[0]] + BitCount8[((unsigned char*)&Mask)[1]];
}
// count the number of unique cofactors (up to 3)
static inline int If_Dec7CofCount3( word t[2] )
{
    unsigned char * pTruth = (unsigned char *)t;
    int i, iCof2 = 0;
    for ( i = 1; i < 16; i++ )
    {
        if ( pTruth[i] == pTruth[0] )
            continue;
        if ( iCof2 == 0 )
            iCof2 = i;
        else if ( pTruth[i] != pTruth[iCof2] )
            return 3;
    }
    assert( iCof2 );
    return 2;
}

// permute 6-input function
static inline word If_Dec6SwapAdjacent( word t, int v )
{
    assert( v < 5 );
    return (t & PMasks[v][0]) | ((t & PMasks[v][1]) << (1 << v)) | ((t & PMasks[v][2]) >> (1 << v));
}
static inline word If_Dec6MoveTo( word t, int v, int p, int Pla2Var[6], int Var2Pla[6] )
{
    int iPlace0, iPlace1;
    assert( Var2Pla[v] >= p );
    while ( Var2Pla[v] != p )
    {
        iPlace0 = Var2Pla[v]-1;
        iPlace1 = Var2Pla[v];
        t = If_Dec6SwapAdjacent( t, iPlace0 );
        Var2Pla[Pla2Var[iPlace0]]++;
        Var2Pla[Pla2Var[iPlace1]]--;
        Pla2Var[iPlace0] ^= Pla2Var[iPlace1];
        Pla2Var[iPlace1] ^= Pla2Var[iPlace0];
        Pla2Var[iPlace0] ^= Pla2Var[iPlace1];
    }
    assert( Pla2Var[p] == v );
    return t;
}

// permute 7-input function
static inline void If_Dec7SwapAdjacent( word t[2], int v )
{
    if ( v == 5 )
    {
        unsigned Temp = (t[0] >> 32);
        t[0]  = (t[0] & 0xFFFFFFFF) | ((t[1] & 0xFFFFFFFF) << 32);
        t[1] ^= (t[1] & 0xFFFFFFFF) ^ Temp;
        return;
    }
    assert( v < 5 );
    t[0] = If_Dec6SwapAdjacent( t[0], v );
    t[1] = If_Dec6SwapAdjacent( t[1], v );
}
static inline void If_Dec7MoveTo( word t[2], int v, int p, int Pla2Var[7], int Var2Pla[7] )
{
    int iPlace0, iPlace1;
    assert( Var2Pla[v] >= p );
    while ( Var2Pla[v] != p )
    {
        iPlace0 = Var2Pla[v]-1;
        iPlace1 = Var2Pla[v];
        If_Dec7SwapAdjacent( t, iPlace0 );
        Var2Pla[Pla2Var[iPlace0]]++;
        Var2Pla[Pla2Var[iPlace1]]--;
        Pla2Var[iPlace0] ^= Pla2Var[iPlace1];
        Pla2Var[iPlace1] ^= Pla2Var[iPlace0];
        Pla2Var[iPlace0] ^= Pla2Var[iPlace1];
    }
    assert( Pla2Var[p] == v );
}

// derive binary function
static inline int If_Dec6DeriveCount2( word t, int * pCof0, int * pCof1 )
{
    int i, Mask = 0;
    *pCof0 = (t & 15);
    *pCof1 = (t & 15);
    for ( i = 1; i < 16; i++ )
        if ( *pCof0 != ((t >> (i<<2)) & 15) )
        {
            *pCof1 = ((t >> (i<<2)) & 15);
            Mask |= (1 << i);
        }
    return Mask;
}
static inline int If_Dec7DeriveCount3( word t[2], int * pCof0, int * pCof1 )
{
    unsigned char * pTruth = (unsigned char *)t;
    int i, Mask = 0;
    *pCof0 = pTruth[0];
    *pCof1 = pTruth[0];
    for ( i = 1; i < 16; i++ )
        if ( *pCof0 != pTruth[i] )
        {
            *pCof1 = pTruth[i];
            Mask |= (1 << i);
        }
    return Mask;
}

// derives decomposition
static inline word If_Dec6Cofactor( word t, int iVar, int fCof1 )
{
    assert( iVar >= 0 && iVar < 6 );
    if ( fCof1 )
        return (t & Truth6[iVar]) | ((t & Truth6[iVar]) >> (1<<iVar));
    else
        return (t &~Truth6[iVar]) | ((t &~Truth6[iVar]) << (1<<iVar));
}
static word If_Dec6DeriveDisjoint( word t, int Pla2Var[6], int Var2Pla[6] )
{
    int i, Cof0, Cof1;
    word z = If_Dec6DeriveCount2( t, &Cof0, &Cof1 );
    for ( i = 0; i < 4; i++ )
        z |= (((word)Pla2Var[i+2]) << (16 + 4*i));
    z |= ((word)((Cof1 << 4) | Cof0) << 32);
    z |= ((word)((Cof1 << 4) | Cof0) << 40);
    for ( i = 0; i < 2; i++ )
        z |= (((word)Pla2Var[i]) << (48 + 4*i));
    z |= (((word)7) << (48 + 4*i++));
    assert( i == 3 );
    return z;
}
static word If_Dec6DeriveNonDisjoint( word t, int s, int Pla2Var0[6], int Var2Pla0[6] )
{
    word z, c0, c1;
    int Cof0[2], Cof1[2];
    int Truth0, Truth1, i;
    int Pla2Var[6], Var2Pla[6];
    assert( s >= 2 && s <= 5 );
    for ( i = 0; i < 6; i++ )
    {
        Pla2Var[i] = Pla2Var0[i];
        Var2Pla[i] = Var2Pla0[i];
    }
    for ( i = s; i < 5; i++ )
    {
        t = If_Dec6SwapAdjacent( t, i );
        Var2Pla[Pla2Var[i]]++;
        Var2Pla[Pla2Var[i+1]]--;
        Pla2Var[i] ^= Pla2Var[i+1];
        Pla2Var[i+1] ^= Pla2Var[i];
        Pla2Var[i] ^= Pla2Var[i+1];
    }
    c0 = If_Dec6Cofactor( t, 5, 0 );
    c1 = If_Dec6Cofactor( t, 5, 1 );
    assert( 2 >= If_Dec6CofCount2(c0) );
    assert( 2 >= If_Dec6CofCount2(c1) );
    Truth0 = If_Dec6DeriveCount2( c0, &Cof0[0], &Cof0[1] );
    Truth1 = If_Dec6DeriveCount2( c1, &Cof1[0], &Cof1[1] );
    z = ((Truth1 & 0xFF) << 8) | (Truth0 & 0xFF);
    for ( i = 0; i < 4; i++ )
        z |= (((word)Pla2Var[i+2]) << (16 + 4*i));
    z |= ((word)((Cof0[1] << 4) | Cof0[0]) << 32);
    z |= ((word)((Cof1[1] << 4) | Cof1[0]) << 40);
    for ( i = 0; i < 2; i++ )
        z |= (((word)Pla2Var[i]) << (48 + 4*i));
    z |= (((word)7) << (48 + 4*i++));
    z |= (((word)Pla2Var[5]) << (48 + 4*i++));
    assert( i == 4 );
    return z;
}
static word If_Dec7DeriveDisjoint( word t[2], int Pla2Var[7], int Var2Pla[7] )
{
    int i, Cof0, Cof1;
    word z = If_Dec7DeriveCount3( t, &Cof0, &Cof1 );
    for ( i = 0; i < 4; i++ )
        z |= (((word)Pla2Var[i+3]) << (16 + 4*i));
    z |= ((word)((Cof1 << 8) | Cof0) << 32);
    for ( i = 0; i < 3; i++ )
        z |= (((word)Pla2Var[i]) << (48 + 4*i));
    z |= (((word)7) << (48 + 4*i));
    return z;
}

static inline int If_Dec6CountOnes( word t )
{
    t =    (t & ABC_CONST(0x5555555555555555)) + ((t>> 1) & ABC_CONST(0x5555555555555555));
    t =    (t & ABC_CONST(0x3333333333333333)) + ((t>> 2) & ABC_CONST(0x3333333333333333));
    t =    (t & ABC_CONST(0x0F0F0F0F0F0F0F0F)) + ((t>> 4) & ABC_CONST(0x0F0F0F0F0F0F0F0F));
    t =    (t & ABC_CONST(0x00FF00FF00FF00FF)) + ((t>> 8) & ABC_CONST(0x00FF00FF00FF00FF));
    t =    (t & ABC_CONST(0x0000FFFF0000FFFF)) + ((t>>16) & ABC_CONST(0x0000FFFF0000FFFF));
    return (t & ABC_CONST(0x00000000FFFFFFFF)) +  (t>>32);
}
static inline int If_Dec6HasVar( word t, int v )
{
    return ((t & Truth6[v]) >> (1<<v)) != (t & ~Truth6[v]);
}
static inline int If_Dec7HasVar( word t[2], int v )
{
    assert( v >= 0 && v < 7 );
    if ( v == 6 )
        return t[0] != t[1]; 
    return ((t[0] & Truth6[v]) >> (1<<v)) != (t[0] & ~Truth6[v])
        || ((t[1] & Truth6[v]) >> (1<<v)) != (t[1] & ~Truth6[v]);
}

static inline void If_DecVerifyPerm( int Pla2Var[6], int Var2Pla[6] )
{
    int i;
    for ( i = 0; i < 6; i++ )
        assert( Pla2Var[Var2Pla[i]] == i );
} 
word If_Dec6Perform( word t, int fDerive )
{ 
    word r = 0;
    int i, v, u, x, Count, Pla2Var[6], Var2Pla[6];
    // start arrays
    for ( i = 0; i < 6; i++ )
    {
        assert( If_Dec6HasVar( t, i ) );
        Pla2Var[i] = Var2Pla[i] = i;
    }
    // generate permutations
    i = 0;
    for ( v = 0;   v < 6; v++ )
    for ( u = v+1; u < 6; u++, i++ )
    {
        t = If_Dec6MoveTo( t, v, 0, Pla2Var, Var2Pla );
        t = If_Dec6MoveTo( t, u, 1, Pla2Var, Var2Pla );
//        If_DecVerifyPerm( Pla2Var, Var2Pla );
        Count = If_Dec6CofCount2( t );
        assert( Count > 1 );
        if ( Count == 2 )
            return !fDerive ? 1 : If_Dec6DeriveDisjoint( t, Pla2Var, Var2Pla );
        // check non-disjoint decomposition
        if ( !r && (Count == 3 || Count == 4) )
        {
            for ( x = 0; x < 4; x++ )
            {
                word c0 = If_Dec6Cofactor( t, x+2, 0 );
                word c1 = If_Dec6Cofactor( t, x+2, 1 );
                if ( If_Dec6CofCount2( c0 ) <= 2 && If_Dec6CofCount2( c1 ) <= 2 )
                {
                    r = !fDerive ? 1 : If_Dec6DeriveNonDisjoint( t, x+2, Pla2Var, Var2Pla );
                    break;
                }
            }
        }
    }
    assert( i == 15 );
    return r;
}
word If_Dec7Perform( word t0[2], int fDerive )
{
    word t[2] = {t0[0], t0[1]}; 
    int i, v, u, y, Pla2Var[7], Var2Pla[7];
    // start arrays
    for ( i = 0; i < 7; i++ )
    {
/*
        if ( i < 6 )
            assert( If_Dec6HasVar( t[0], i ) || If_Dec6HasVar( t[1], i ) );
        else
            assert( t[0] != t[1] );
*/
        Pla2Var[i] = Var2Pla[i] = i;
    }
    // generate permutations
    for ( v = 0;   v < 7; v++ )
    for ( u = v+1; u < 7; u++ )
    for ( y = u+1; y < 7; y++ )
    {
        If_Dec7MoveTo( t, v, 0, Pla2Var, Var2Pla );
        If_Dec7MoveTo( t, u, 1, Pla2Var, Var2Pla );
        If_Dec7MoveTo( t, y, 2, Pla2Var, Var2Pla );
//        If_DecVerifyPerm( Pla2Var, Var2Pla );
        if ( If_Dec7CofCount3( t ) == 2 )
        {
            return !fDerive ? 1 : If_Dec7DeriveDisjoint( t, Pla2Var, Var2Pla );
        }
    }
    return 0;
}


// support minimization
static inline int If_DecSuppIsMinBase( int Supp )
{
    return (Supp & (Supp+1)) == 0;
}
static inline word If_Dec6TruthShrink( word uTruth, int nVars, int nVarsAll, unsigned Phase )
{
    int i, k, Var = 0;
    assert( nVarsAll <= 6 );
    for ( i = 0; i < nVarsAll; i++ )
        if ( Phase & (1 << i) )
        {
            for ( k = i-1; k >= Var; k-- )
                uTruth = If_Dec6SwapAdjacent( uTruth, k );
            Var++;
        }
    assert( Var == nVars );
    return uTruth;
}
word If_Dec6MinimumBase( word uTruth, int * pSupp, int nVarsAll, int * pnVars )
{
    int v, iVar = 0, uSupp = 0;
    assert( nVarsAll <= 6 );
    for ( v = 0; v < nVarsAll; v++ )
        if ( If_Dec6HasVar( uTruth, v ) )
        {
            uSupp |= (1 << v);
            if ( pSupp )
                pSupp[iVar] = pSupp[v];
            iVar++;
        }
    if ( pnVars )
        *pnVars = iVar;
    if ( If_DecSuppIsMinBase( uSupp ) )
        return uTruth;
    return If_Dec6TruthShrink( uTruth, iVar, nVarsAll, uSupp );
}

static inline void If_Dec7TruthShrink( word uTruth[2], int nVars, int nVarsAll, unsigned Phase )
{
    int i, k, Var = 0;
    assert( nVarsAll <= 7 );
    for ( i = 0; i < nVarsAll; i++ )
        if ( Phase & (1 << i) )
        {
            for ( k = i-1; k >= Var; k-- )
                If_Dec7SwapAdjacent( uTruth, k );
            Var++;
        }
    assert( Var == nVars );
}
void If_Dec7MinimumBase( word uTruth[2], int * pSupp, int nVarsAll, int * pnVars )
{
    int v, iVar = 0, uSupp = 0;
    assert( nVarsAll <= 7 );
    for ( v = 0; v < nVarsAll; v++ )
        if ( If_Dec7HasVar( uTruth, v ) )
        {
            uSupp |= (1 << v);
            if ( pSupp )
                pSupp[iVar] = pSupp[v];
            iVar++;
        }
    if ( pnVars )
        *pnVars = iVar;
    if ( If_DecSuppIsMinBase( uSupp ) )
        return;
    If_Dec7TruthShrink( uTruth, iVar, nVarsAll, uSupp );
}



// check for MUX decomposition
static inline int If_Dec6SuppSize( word t )
{
    int v, Count = 0;
    for ( v = 0; v < 6; v++ )
        if ( If_Dec6Cofactor(t, v, 0) != If_Dec6Cofactor(t, v, 1) )
            Count++;
    return Count;
}
static inline int If_Dec6CheckMux( word t )
{
    int v;
    for ( v = 0; v < 6; v++ )
        if ( If_Dec6SuppSize(If_Dec6Cofactor(t, v, 0)) < 5 &&
             If_Dec6SuppSize(If_Dec6Cofactor(t, v, 1)) < 5 )
             return v;
    return -1;
}

// check for MUX decomposition
static inline void If_Dec7Cofactor( word t[2], int iVar, int fCof1, word r[2] )
{
    assert( iVar >= 0 && iVar < 7 );
    if ( iVar == 6 )
    {
        if ( fCof1 )
            r[0] = r[1] = t[1];
        else
            r[0] = r[1] = t[0];
    }
    else
    {
        if ( fCof1 )
        {
            r[0] = (t[0] & Truth6[iVar]) | ((t[0] & Truth6[iVar]) >> (1<<iVar));
            r[1] = (t[1] & Truth6[iVar]) | ((t[1] & Truth6[iVar]) >> (1<<iVar));
        }
        else
        {
            r[0] = (t[0] &~Truth6[iVar]) | ((t[0] &~Truth6[iVar]) << (1<<iVar));
            r[1] = (t[1] &~Truth6[iVar]) | ((t[1] &~Truth6[iVar]) << (1<<iVar));
        }
    }
}
static inline int If_Dec7SuppSize( word t[2] )
{
    word c0[2], c1[2];
    int v, Count = 0;
    for ( v = 0; v < 7; v++ )
    {
        If_Dec7Cofactor( t, v, 0, c0 );
        If_Dec7Cofactor( t, v, 1, c1 );
        if ( c0[0] != c1[0] || c0[1] != c1[1] )
            Count++;
    }
    return Count;
}
static inline int If_Dec7CheckMux( word t[2] )
{
    word c0[2], c1[2];
    int v;
    for ( v = 0; v < 7; v++ )
    {
        If_Dec7Cofactor( t, v, 0, c0 );
        If_Dec7Cofactor( t, v, 1, c1 );
        if ( If_Dec7SuppSize(c0) < 5 && If_Dec7SuppSize(c1) < 5 )
            return v;
    }
    return -1;
}

// find the best MUX decomposition
int If_Dec6PickBestMux( word t, word Cofs[2] )
{
    int v, vBest = -1, Count0, Count1, CountBest = 1000;
    for ( v = 0; v < 6; v++ )
    {
        Count0 = If_Dec6SuppSize( If_Dec6Cofactor(t, v, 0) );
        Count1 = If_Dec6SuppSize( If_Dec6Cofactor(t, v, 1) );
        if ( Count0 < 5 && Count1 < 5 && CountBest > Count0 + Count1 )
        {
            CountBest = Count0 + Count1;
            vBest = v;
            Cofs[0] = If_Dec6Cofactor(t, v, 0);
            Cofs[1] = If_Dec6Cofactor(t, v, 1);
        }
    }
    return vBest;
}
int If_Dec7PickBestMux( word t[2], word c0r[2], word c1r[2] )
{
    word c0[2], c1[2];
    int v, vBest = -1, Count0, Count1, CountBest = 1000;
    for ( v = 0; v < 7; v++ )
    {
        If_Dec7Cofactor( t, v, 0, c0 );
        If_Dec7Cofactor( t, v, 1, c1 );
        Count0 = If_Dec7SuppSize(c0);
        Count1 = If_Dec7SuppSize(c1);
        if ( Count0 < 5 && Count1 < 5 && CountBest > Count0 + Count1 )
        {
            CountBest = Count0 + Count1;
            vBest = v;
            c0r[0] = c0[0]; c0r[1] = c0[1];
            c1r[0] = c1[0]; c1r[1] = c1[1];
        }
    }
    return vBest;
}



/**Function*************************************************************

  Synopsis    [Checks decomposability ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// count the number of unique cofactors
static inline word If_Dec5CofCount2( word t, int x, int y, int * Pla2Var, word t0, int fDerive )
{
    int m, i, Mask;
    assert( x >= 0 && x < 4 );
    assert( y >= 0 && y < 4 );
    for ( m = 0; m < 4; m++ )
    {
        for ( Mask = i = 0; i < 16; i++ )
            if ( ((i >> x) & 1) == ((m >> 0) & 1) && ((i >> y) & 1) == ((m >> 1) & 1) )
                Mask |= (1 << ((t >> (i<<1)) & 3));
        if ( BitCount8[Mask & 0xF] > 2 )
            return 0;
    }
//    Kit_DsdPrintFromTruth( (unsigned *)&t, 5 ); printf( "\n" );
    if ( !fDerive )
        return 1;
    else
    {
        int fHas2, fHas3;
        // composition function C depends on {x, y, Out, v[0]}
        // decomposed function D depends on {x, y, z1, z2}
        word F[4] = { 0, ABC_CONST(0x5555555555555555), ABC_CONST(0xAAAAAAAAAAAAAAAA), ~((word)0) };
        word C2[4], D2[4] = {0}, C1[2], D1[2], C, D, z;
        int v, zz1 = -1, zz2 = -1;
        // find two variables
        for ( v = 0; v < 4; v++ )
            if ( v != x && v != y )
                {  zz1 = v; break; }
        for ( v = 1; v < 4; v++ )
            if ( v != x && v != y && v != zz1 )
                {  zz2 = v; break; }
        assert( zz1 != -1 && zz2 != -1 );
        // find the cofactors
        for ( m = 0; m < 4; m++ )
        {
            // for each cofactor, derive C2 and D2
            for ( Mask = i = 0; i < 16; i++ )
                if ( ((i >> x) & 1) == ((m >> 0) & 1) && ((i >> y) & 1) == ((m >> 1) & 1) )
                    Mask |= (1 << ((t >> (i<<1)) & 3));
            // find the values
            if ( BitCount8[Mask & 0xF] == 1 )
            {
                C2[m] = F[Abc_Tt6FirstBit( Mask )];
                D2[m] = ~(word)0;
            }
            else if ( BitCount8[Mask & 0xF] == 2 )
            {
                int Bit0 = Abc_Tt6FirstBit( Mask );
                int Bit1 = Abc_Tt6FirstBit( Mask ^ (((word)1)<<Bit0) );
                C2[m] = (F[Bit1] & Truth6[1]) | (F[Bit0] & ~Truth6[1]);
                // find Bit1 appears
                for ( Mask = i = 0; i < 16; i++ )
                    if ( ((i >> x) & 1) == ((m >> 0) & 1) && ((i >> y) & 1) == ((m >> 1) & 1) )
                        if ( Bit1 == ((t >> (i<<1)) & 3) )
                            D2[m] |= (((word)1) << ( (((i >> zz2) & 1) << 1) | ((i >> zz1) & 1) ));
            }
            else assert( 0 );
            D2[m] = Abc_Tt6Stretch( D2[m], 2 );
        }

        // combine
        C1[0] = (C2[1] & Truth6[2]) | (C2[0] & ~Truth6[2]);
        C1[1] = (C2[3] & Truth6[2]) | (C2[2] & ~Truth6[2]);
        C     = (C1[1] & Truth6[3]) | (C1[0] & ~Truth6[3]);

        // combine
        D1[0] = (D2[1] & Truth6[2]) | (D2[0] & ~Truth6[2]);
        D1[1] = (D2[3] & Truth6[2]) | (D2[2] & ~Truth6[2]);
        D     = (D1[1] & Truth6[3]) | (D1[0] & ~Truth6[3]);

//        printf( "\n" );
//        Kit_DsdPrintFromTruth( (unsigned *)&C, 5 ); printf("\n");
//        Kit_DsdPrintFromTruth( (unsigned *)&D, 5 ); printf("\n");

        // create configuration
        // find one
        fHas2 = Abc_TtHasVar(&D, 5, 2);
        fHas3 = Abc_TtHasVar(&D, 5, 3);
        if ( fHas2 && fHas3 )
        {
            z = D & 0xFFFF;
            z |= (((word)Pla2Var[zz1+1]) << (16 + 4*0));
            z |= (((word)Pla2Var[zz2+1]) << (16 + 4*1));
            z |= (((word)Pla2Var[x+1])   << (16 + 4*2));
            z |= (((word)Pla2Var[y+1])   << (16 + 4*3));
        }
        else if ( fHas2 && !fHas3 )
        {
            z = D & 0xFFFF;
            z |= (((word)Pla2Var[zz1+1]) << (16 + 4*0));
            z |= (((word)Pla2Var[zz2+1]) << (16 + 4*1));
            z |= (((word)Pla2Var[x+1])   << (16 + 4*2));
            z |= (((word)6)              << (16 + 4*3));
        }
        else if ( !fHas2 && fHas3 )
        {
            Abc_TtSwapVars( &D, 5, 2, 3 );
            z = D & 0xFFFF;
            z |= (((word)Pla2Var[zz1+1]) << (16 + 4*0));
            z |= (((word)Pla2Var[zz2+1]) << (16 + 4*1));
            z |= (((word)Pla2Var[y+1])   << (16 + 4*2));
            z |= (((word)6)              << (16 + 4*3));
        }
        else 
        {
            z = D & 0xFFFF;
            z |= (((word)Pla2Var[zz1+1]) << (16 + 4*0));
            z |= (((word)Pla2Var[zz2+1]) << (16 + 4*1));
            z |= (((word)6)              << (16 + 4*2));
            z |= (((word)6)              << (16 + 4*3));
        }

        // second one
        fHas2 = Abc_TtHasVar(&C, 5, 2);
        fHas3 = Abc_TtHasVar(&C, 5, 3);
        if ( fHas2 && fHas3 )
        {
            z |= ((C & 0xFFFF) << 32);
            z |= (((word)Pla2Var[0])     << (48 + 4*0));
            z |= (((word)7)              << (48 + 4*1));
            z |= (((word)Pla2Var[x+1])   << (48 + 4*2));
            z |= (((word)Pla2Var[y+1])   << (48 + 4*3));
        }
        else if ( fHas2 && !fHas3 )
        {
            z |= ((C & 0xFFFF) << 32);
            z |= (((word)Pla2Var[0])     << (48 + 4*0));
            z |= (((word)7)              << (48 + 4*1));
            z |= (((word)Pla2Var[x+1])   << (48 + 4*2));
            z |= (((word)6)              << (48 + 4*3));
        }
        else if ( !fHas2 && fHas3 )
        {
            Abc_TtSwapVars( &C, 5, 2, 3 );
            z |= ((C & 0xFFFF) << 32);
            z |= (((word)Pla2Var[0])     << (48 + 4*0));
            z |= (((word)7)              << (48 + 4*1));
            z |= (((word)Pla2Var[y+1])   << (48 + 4*2));
            z |= (((word)6)              << (48 + 4*3));
        }
        else 
        {
            z |= ((C & 0xFFFF) << 32);
            z |= (((word)Pla2Var[0])     << (48 + 4*0));
            z |= (((word)7)              << (48 + 4*1));
            z |= (((word)6)              << (48 + 4*2));
            z |= (((word)6)              << (48 + 4*3));
        }

        // verify
        {
            word t1 = If_Dec6Truth( z );
            if ( t1 != t0 )
            {
                printf( "\n" );
                Kit_DsdPrintFromTruth( (unsigned *)&C2[0], 5 ); printf("\n");
                Kit_DsdPrintFromTruth( (unsigned *)&C2[1], 5 ); printf("\n");
                Kit_DsdPrintFromTruth( (unsigned *)&C2[2], 5 ); printf("\n");
                Kit_DsdPrintFromTruth( (unsigned *)&C2[3], 5 ); printf("\n");

                printf( "\n" );
                Kit_DsdPrintFromTruth( (unsigned *)&D2[0], 5 ); printf("\n");
                Kit_DsdPrintFromTruth( (unsigned *)&D2[1], 5 ); printf("\n");
                Kit_DsdPrintFromTruth( (unsigned *)&D2[2], 5 ); printf("\n");
                Kit_DsdPrintFromTruth( (unsigned *)&D2[3], 5 ); printf("\n");

                printf( "\n" );
                Kit_DsdPrintFromTruth( (unsigned *)&C, 5 ); printf("\n");
                Kit_DsdPrintFromTruth( (unsigned *)&D, 5 ); printf("\n");

                printf( "\n" );
                Kit_DsdPrintFromTruth( (unsigned *)&t1, 5 ); printf("\n");
                Kit_DsdPrintFromTruth( (unsigned *)&t0, 5 ); printf("\n");
            }
            assert( t1 == t0 );
        }
        return z;
    }
}
word If_Dec5Perform( word t, int fDerive )
{
    int Pla2Var[7], Var2Pla[7];
    int i, j, v;
    word t0 = t;
/*
    word c0, c1, c00, c01, c10, c11;
    for ( i = 0; i < 5; i++ )
    {
        c0 = If_Dec6Cofactor( t, i, 0 );
        c1 = If_Dec6Cofactor( t, i, 1 );
        if ( c0 == 0 )
            return 1;
        if ( ~c0 == 0 )
            return 1;
        if ( c1 == 0 )
            return 1;
        if ( ~c1 == 0 )
            return 1;
       if ( c0 == ~c1 )
            return 1;
    }
    for ( i = 0; i < 4; i++ )
    {
        c0 = If_Dec6Cofactor( t, i, 0 );
        c1 = If_Dec6Cofactor( t, i, 1 );
        for ( j = i + 1; j < 5; j++ )
        {
            c00 = If_Dec6Cofactor( c0, j, 0 );
            c01 = If_Dec6Cofactor( c0, j, 1 );
            c10 = If_Dec6Cofactor( c1, j, 0 );
            c11 = If_Dec6Cofactor( c1, j, 1 );
            if ( c00 == c01 && c00 == c10 )
                return 1;
            if ( c11 == c01 && c11 == c10 )
                return 1;
            if ( c11 == c00 && c11 == c01 )
                return 1;
            if ( c11 == c00 && c11 == c10 )
                return 1;
            if ( c00 == c11 && c01 == c10 )
                return 1;
        }
    }
*/
    // start arrays
    for ( i = 0; i < 7; i++ )
        Pla2Var[i] = Var2Pla[i] = i;
    // generate permutations
    for ( v = 0; v < 5; v++ )
    {
        t = If_Dec6MoveTo( t, v, 0, Pla2Var, Var2Pla );
        If_DecVerifyPerm( Pla2Var, Var2Pla );
        for ( i = 0; i < 4; i++ )
            for ( j = i + 1; j < 4; j++ )
            {
                word z = If_Dec5CofCount2( t, i, j, Pla2Var, t0, fDerive );
                if ( z )
                {
/*
                    if ( v == 0 && i == 1 && j == 2 )
                    {
                          Kit_DsdPrintFromTruth( (unsigned *)&t, 5 ); printf( "\n" );
                    }
*/
                    return z;
                }
            }
    }

/*
    // start arrays
    for ( i = 0; i < 7; i++ )
        Pla2Var[i] = Var2Pla[i] = i;

    t = t0;
    for ( v = 0; v < 5; v++ )
    {
        int x, y;

        t = If_Dec6MoveTo( t, v, 0, Pla2Var, Var2Pla );
        If_DecVerifyPerm( Pla2Var, Var2Pla );

        for ( i = 0; i < 16; i++ )
            printf( "%d ", ((t >> (i<<1)) & 3) );
        printf( "\n" );

        for ( x = 0; x < 4; x++ )
        for ( y = x + 1; y < 4; y++ )
        {
            for ( i = 0; i < 16; i++ )
                if ( !((i >> x) & 1) && !((i >> y) & 1) )
                    printf( "%d ", ((t >> (i<<1)) & 3) );
            printf( "\n" );

            for ( i = 0; i < 16; i++ )
                if ( ((i >> x) & 1) && !((i >> y) & 1) )
                    printf( "%d ", ((t >> (i<<1)) & 3) );
            printf( "\n" );

            for ( i = 0; i < 16; i++ )
                if ( !((i >> x) & 1) && ((i >> y) & 1) )
                    printf( "%d ", ((t >> (i<<1)) & 3) );
            printf( "\n" );

            for ( i = 0; i < 16; i++ )
                if ( ((i >> x) & 1) && ((i >> y) & 1) )
                    printf( "%d ", ((t >> (i<<1)) & 3) );
            printf( "\n" );
            printf( "\n" );
        }
    }
*/

//    Kit_DsdPrintFromTruth( (unsigned *)&t, 5 ); printf( "\n" );
    return 0;
}

word If_Dec5PerformEx()
{
    word z;
    // find one
    z = (word)(0x17ac & 0xFFFF);
    z |= (((word)3) << (16 + 4*0));
    z |= (((word)4) << (16 + 4*1));
    z |= (((word)1) << (16 + 4*2));
    z |= (((word)2) << (16 + 4*3));
    // second one
    z |= (((word)(0x179a & 0xFFFF)) << 32);
    z |= (((word)0) << (48 + 4*0));
    z |= (((word)7) << (48 + 4*1));
    z |= (((word)1) << (48 + 4*2));
    z |= (((word)2) << (48 + 4*3));
    return z;
}
void If_Dec5PerformTest()
{
    word z, t, t1;
//    s = If_Dec5PerformEx();
//    t = If_Dec6Truth( s );
    t = ABC_CONST(0xB0F3B0FFB0F3B0FF);

    Kit_DsdPrintFromTruth( (unsigned *)&t, 5 ); printf("\n");

    z = If_Dec5Perform( t, 1 );
    t1 = If_Dec6Truth( z );
    assert( t == t1 );
}


/**Function*************************************************************

  Synopsis    [Performs additional check.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word If_CutPerformDerive07( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr )
{
    if ( nLeaves < 5 )
        return 1;
    if ( nLeaves == 5 )
    {
        word z, t = ((word)pTruth[0] << 32) | (word)pTruth[0];
        z = If_Dec5Perform( t, 1 );
        If_Dec6Verify( t, z );
        return z;
    }
    if ( nLeaves == 6 )
    {
        word z, t = ((word *)pTruth)[0];
        z = If_Dec6Perform( t, 1 );
        If_Dec6Verify( t, z );
        return z;
    }
    if ( nLeaves == 7 )
    {
        word z, t[2];
        t[0] = ((word *)pTruth)[0];
        t[1] = ((word *)pTruth)[1];
        z = If_Dec7Perform( t, 1 );
        If_Dec7Verify( t, z );
        return z;
    }
    assert( 0 );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs additional check.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutPerformCheck07( If_Man_t * p, unsigned * pTruth, int nVars, int nLeaves, char * pStr )
{
    int fDerive = 0;
    int v;
    // skip non-support minimal
    for ( v = 0; v < nLeaves; v++ )
        if ( !Abc_TtHasVar( (word *)pTruth, nVars, v ) )
            return 0;
    // check
    if ( nLeaves < 5 )
        return 1;
    if ( nLeaves == 5 )
    {
        word z, t = ((word)pTruth[0] << 32) | (word)pTruth[0];
        z = If_Dec5Perform( t, fDerive );
        if ( fDerive && z )
            If_Dec6Verify( t, z );
        return z != 0;
    }
    if ( nLeaves == 6 )
    {
        word z, t = ((word *)pTruth)[0];
        z = If_Dec6Perform( t, fDerive );
        if ( fDerive && z )
        {
//            If_DecPrintConfig( z );
            If_Dec6Verify( t, z );
        }
//        if ( z == 0 )
//            Extra_PrintHex(stdout, (unsigned *)&t, 6), printf( "  " ), Kit_DsdPrintFromTruth( (unsigned *)&t, 6 ), printf( "\n" );
        return z != 0;
    }
    if ( nLeaves == 7 )
    {
        word z, t[2];
        t[0] = ((word *)pTruth)[0];
        t[1] = ((word *)pTruth)[1];
//        if ( If_Dec7CheckMux(t) >= 0 )
//            return 1;
        z = If_Dec7Perform( t, fDerive );
        if ( fDerive && z )
            If_Dec7Verify( t, z );
        return z != 0;
    }
    assert( 0 );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

