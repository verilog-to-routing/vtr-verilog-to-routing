/**CFile****************************************************************

  FileName    [rsbDec6.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Truth-table-based resubstitution.]

  Synopsis    [Implementation of the algorithm.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rsbDec6.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rsbInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Rsb_DecTry0( word c ) 
{
    return (unsigned)(c != 0);
}
static inline unsigned Rsb_DecTry1( word c, word f1 ) 
{
    return (Rsb_DecTry0(c&f1) << 1) | Rsb_DecTry0(c&~f1);
}
static inline unsigned Rsb_DecTry2( word c, word f1, word f2 ) 
{
    return (Rsb_DecTry1(c&f2, f1) << 2) | Rsb_DecTry1(c&~f2, f1);
}
static inline unsigned Rsb_DecTry3( word c, word f1, word f2, word f3 ) 
{
    return (Rsb_DecTry2(c&f3, f1, f2) << 4) | Rsb_DecTry2(c&~f3, f1, f2);
}
static inline unsigned Rsb_DecTry4( word c, word f1, word f2, word f3, word f4 ) 
{
    return (Rsb_DecTry3(c&f4, f1, f2, f3) << 8) | Rsb_DecTry3(c&~f4, f1, f2, f3);
}
static inline unsigned Rsb_DecTry5( word c, word f1, word f2, word f3, word f4, word f5 ) 
{
    return (Rsb_DecTry4(c&f5, f1, f2, f3, f4) << 16) | Rsb_DecTry4(c&~f5, f1, f2, f3, f4);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Rsb_DecTryCex( word * g, int iCexA, int iCexB ) 
{
    return Abc_TtGetBit(g, iCexA) == Abc_TtGetBit(g, iCexB);
}
static inline void Rsb_DecVerifyCex( word * f, word ** g, int nGs, int iCexA, int iCexB ) 
{
    int i;
    // f distinquished it
    if ( Rsb_DecTryCex( f, iCexA, iCexB ) )
        printf( "Verification of CEX has failed: f(A) == f(B)!!!\n" );
    // g do not distinguish it
    for ( i = 0; i < nGs; i++ )
        if ( !Rsb_DecTryCex( g[i], iCexA, iCexB ) )
            printf( "Verification of CEX has failed: g[%d](A) != g[%d](B)!!!\n", i, i );
}
static inline void Rsb_DecRecordCex( word ** g, int nGs, int iCexA, int iCexB, word * pCexes, int nCexes ) 
{
    int i;
    assert( nCexes < 64 );
    for ( i = 0; i < nGs; i++ )
        if ( Rsb_DecTryCex(g[i], iCexA, iCexB) )
            Abc_TtSetBit( pCexes + i, nCexes );
}

/**Function*************************************************************

  Synopsis    [Checks decomposability of f with divisors g.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Rsb_DecCofactor( word ** g, int nGs, int w, int iMint ) 
{
    int i;
    word Cof = ~(word)0;
    for ( i = 0; i < nGs; i++ )
        Cof &=  ((iMint >> i) & 1) ? g[i][w] : ~g[i][w];
    return Cof;
}
unsigned Rsb_DecCheck( int nVars, word * f, word ** g, int nGs, unsigned * pPat, int * pCexA, int * pCexB ) 
{
    word CofA, CofB;
    int nWords = Abc_TtWordNum( nVars );
    int w, k, iMint, iShift = ( 1 << nGs );
    unsigned uMask = (~(unsigned)0) >> (32-iShift);
    unsigned uTotal = 0;
    assert( nGs > 0 && nGs < 5 );
    for ( w = 0; w < nWords; w++ )
    {
        // generate decomposition pattern
        if ( nGs == 1 )
            pPat[w] = Rsb_DecTry2( ~(word)0, g[0][w], f[w] );
        else if ( nGs == 2 )
            pPat[w] = Rsb_DecTry3( ~(word)0, g[0][w], g[1][w], f[w] );
        else if ( nGs == 3 )
            pPat[w] = Rsb_DecTry4( ~(word)0, g[0][w], g[1][w], g[2][w], f[w] );
        else if ( nGs == 4 )
            pPat[w] = Rsb_DecTry5( ~(word)0, g[0][w], g[1][w], g[2][w], g[3][w], f[w] );
        // check if it is consistent
        iMint = Abc_Tt6FirstBit( (pPat[w] >> iShift) & pPat[w] & uMask );
        if ( iMint >= 0 )
        {   // generate a cex
            CofA = Rsb_DecCofactor( g, nGs, w, iMint );
            assert( (~f[w] & CofA) && (f[w] & CofA) );
            *pCexA = w * 64 + Abc_Tt6FirstBit( ~f[w] & CofA );
            *pCexB = w * 64 + Abc_Tt6FirstBit(  f[w] & CofA );
            return 0;
        }
        uTotal |= pPat[w];
        if ( w == 0 )
            continue;
        // check if it is consistent with other patterns seen so far
        iMint = Abc_Tt6FirstBit( (uTotal >> iShift) & uTotal & uMask );
        if ( iMint == -1 )
            continue;
        // find an overlap and generate a cex
        for ( k = 0; k < w; k++ )
        {
            iMint = Abc_Tt6FirstBit( ((pPat[k] | pPat[w]) >> iShift) & (pPat[k] | pPat[w]) & uMask );
            if ( iMint == -1 )
                continue;
            CofA = Rsb_DecCofactor( g, nGs, k, iMint );
            CofB = Rsb_DecCofactor( g, nGs, w, iMint );
            if ( (~f[k] & CofA) && (f[w] & CofB) )
            {
                *pCexA = k * 64 + Abc_Tt6FirstBit( ~f[k] & CofA );
                *pCexB = w * 64 + Abc_Tt6FirstBit(  f[w] & CofB );
            }
            else
            {
                assert( (f[k] & CofA) && (~f[w] & CofB) );
                *pCexA = k * 64 + Abc_Tt6FirstBit(  f[k] & CofA );
                *pCexB = w * 64 + Abc_Tt6FirstBit( ~f[w] & CofB );
            }
            return 0;
        }
    }
    return uTotal;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rsb_DecPrintTable( word * pCexes, int nGs, int nGsAll, Vec_Int_t * vTries ) 
{
    int pCands[16], nCands;
    int i, c, Cand, iStart = 0;
    if ( Vec_IntSize(vTries) == 0 )
        return;

//    printf( "\n" );
    for ( i = 0; i < 4; i++ )
        printf( "    " );
    printf( "  " );
    for ( i = 0; i < nGs; i++ )
        printf( "%d", (i % 100) / 10 );
    printf( "|" );
    for ( ; i < nGsAll; i++ )
        printf( "%d", (i % 100) / 10 );
    printf( "\n" );

    for ( i = 0; i < 4; i++ )
        printf( "    " );
    printf( "  " );
    for ( i = 0; i < nGs; i++ )
        printf( "%d", i % 10 );
    printf( "|" );
    for ( ; i < nGsAll; i++ )
        printf( "%d", i % 10 );
    printf( "\n" );
    printf( "\n" );

    for ( c = 0; iStart < Vec_IntSize(vTries); c++ )
    {
        // collect candidates
        for ( i = 0; i < 4; i++ )
            pCands[i] = -1;
        nCands = 0;
        Vec_IntForEachEntryStart( vTries, Cand, i, iStart )
            if ( Cand == -1 )
            {
                iStart = i + 1;
                break;
            }
            else
                pCands[nCands++] = Cand;
        assert( nCands <= 4 );
        // print them
        for ( i = 0; i < 4; i++ )
            if ( pCands[i] >= 0 )
                printf( "%4d", pCands[i] );
            else
                printf( "    " );
        // print the bit-string
        printf( "  " );
        for ( i = 0; i < nGs; i++ )
            printf( "%c", Abc_TtGetBit( pCexes + i, c ) ? '.' : '+' );
        printf( "|" );
        for ( ; i < nGsAll; i++ )
            printf( "%c", Abc_TtGetBit( pCexes + i, c ) ? '.' : '+' );
        printf( "  %3d\n", c );
    }
    printf( "\n" );

    // write the number of ones
    for ( i = 0; i < 4; i++ )
        printf( "    " );
    printf( "  " );
    for ( i = 0; i < nGs; i++ )
        printf( "%d", Abc_TtCountOnes(pCexes[i]) / 10 );
    printf( "|" );
    for ( ; i < nGsAll; i++ )
        printf( "%d", Abc_TtCountOnes(pCexes[i]) / 10 );
    printf( "\n" );

    for ( i = 0; i < 4; i++ )
        printf( "    " );
    printf( "  " );
    for ( i = 0; i < nGs; i++ )
        printf( "%d", Abc_TtCountOnes(pCexes[i]) % 10 );
    printf( "|" );
    for ( ; i < nGsAll; i++ )
        printf( "%d", Abc_TtCountOnes(pCexes[i]) % 10 );
    printf( "\n" );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Init ]

  Description [Returns the numbers of the decomposition functions and 
  the truth table of a function up to 4 variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rsb_DecInitCexes( int nVars, word * f, word ** g, int nGs, int nGsAll, word * pCexes, Vec_Int_t * vTries ) 
{
    int nWords = Abc_TtWordNum( nVars );
    int ValueB = Abc_TtGetBit( f, 0 );
    int ValueE = Abc_TtGetBit( f, 64*nWords-1 );
    int iCexT0, iCexT1, iCexF0, iCexF1;

    iCexT0 = ValueB  ? 0           : Abc_TtFindFirstBit( f, nVars );
    iCexT1 = ValueE  ? 64*nWords-1 : Abc_TtFindLastBit( f, nVars );

    iCexF0 = !ValueB ? 0           : Abc_TtFindFirstZero( f, nVars );
    iCexF1 = !ValueE ? 64*nWords-1 : Abc_TtFindLastZero( f, nVars );

    assert( !Rsb_DecTryCex( f, iCexT0, iCexF0 ) );
    assert( !Rsb_DecTryCex( f, iCexT0, iCexF1 ) );
    assert( !Rsb_DecTryCex( f, iCexT1, iCexF0 ) );
    assert( !Rsb_DecTryCex( f, iCexT1, iCexF1 ) );

    Rsb_DecRecordCex( g, nGsAll, iCexT0, iCexF0, pCexes, 0 );
    Rsb_DecRecordCex( g, nGsAll, iCexT0, iCexF1, pCexes, 1 );
    Rsb_DecRecordCex( g, nGsAll, iCexT1, iCexF0, pCexes, 2 );
    Rsb_DecRecordCex( g, nGsAll, iCexT1, iCexF1, pCexes, 3 );

    if ( vTries )
    {
    Vec_IntPush( vTries, -1 );
    Vec_IntPush( vTries, -1 );
    Vec_IntPush( vTries, -1 );
    Vec_IntPush( vTries, -1 );
    }
    return 4;
}

/**Function*************************************************************

  Synopsis    [Finds a setset of gs to decompose f.]

  Description [Returns the numbers of the decomposition functions and 
  the truth table of a function up to 4 variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Rsb_DecPerformInt( Rsb_Man_t * p, int nVars, word * f, word ** g, int nGs, int nGsAll, int fFindAll ) 
{
    word * pCexes = Vec_WrdArray(p->vCexes);
    unsigned * pPat = (unsigned *)Vec_IntArray(p->vDecPats);
    /*
    The following filtering hueristic can be used:
    after the first round (if there is more than 5 cexes, 
    remove all the divisors, except fanins of the node
    This should lead to a speadup without sacrifying quality.

    Another idea is to precompute several counter-examples
    like the first and last 0 and 1 mints of the function
    which yields 4 cexes.
    */

    word * pDivs[16];
    unsigned uTruth = 0;
    int i, k, j, n, iCexA, iCexB, nCexes = 0;
    memset( pCexes, 0, sizeof(word) * nGsAll );  
    Vec_IntClear( p->vTries );
    // populate the counter-examples with the three most obvious
//    nCexes = Rsb_DecInitCexes( nVars, f, g, nGs, nGsAll, pCexes, vTries );
    // start by checking each function
    p->vFanins->nSize = 1;
    for ( i = 0; i < nGs; i++ )
    {
        if ( pCexes[i] )
            continue;
        pDivs[0] = g[i];  p->vFanins->pArray[0] = i;
        uTruth = Rsb_DecCheck( nVars, f, pDivs, Vec_IntSize(p->vFanins), pPat, &iCexA, &iCexB );
        if ( uTruth )
        {
            if ( fFindAll )
            {
                uTruth = (unsigned)Abc_Tt6Stretch( (word)uTruth, 1 );
                Kit_DsdPrintFromTruth( &uTruth, 1 ); printf( "   " );
                Vec_IntPrint(p->vFanins);
                continue;
            }
            else
                return uTruth;
        }
        if ( nCexes == 64 )
            return 0;
        Rsb_DecVerifyCex( f, pDivs, Vec_IntSize(p->vFanins), iCexA, iCexB );
        Rsb_DecRecordCex( g, nGsAll, iCexA, iCexB, pCexes, nCexes++ );
        if ( !p->fVerbose )
            continue;
        Vec_IntPush( p->vTries, i );
        Vec_IntPush( p->vTries, -1 );
    }
    if ( p->nDecMax == 1 )
        return 0;
    // continue by checking pairs
    p->vFanins->nSize = 2;
    for ( i = 1; i < nGs; i++ )
    for ( k = 0; k < i; k++ )
    {
        if ( pCexes[i] & pCexes[k] )
            continue;
        pDivs[0] = g[i];  p->vFanins->pArray[0] = i;
        pDivs[1] = g[k];  p->vFanins->pArray[1] = k;
        uTruth = Rsb_DecCheck( nVars, f, pDivs, Vec_IntSize(p->vFanins), pPat, &iCexA, &iCexB );
        if ( uTruth )
        {
            if ( fFindAll )
            {
                uTruth = (unsigned)Abc_Tt6Stretch( (word)uTruth, 2 );
                Kit_DsdPrintFromTruth( &uTruth, 2 ); printf( "   " );
                Vec_IntPrint(p->vFanins);
                continue;
            }
            else
                return uTruth;
        }
        if ( nCexes == 64 )
            return 0;
        Rsb_DecVerifyCex( f, pDivs, Vec_IntSize(p->vFanins), iCexA, iCexB );
        Rsb_DecRecordCex( g, nGsAll, iCexA, iCexB, pCexes, nCexes++ );
        if ( !p->fVerbose )
            continue;
        Vec_IntPush( p->vTries, i );
        Vec_IntPush( p->vTries, k );
        Vec_IntPush( p->vTries, -1 );
    }
    if ( p->nDecMax == 2 )
        return 0;
    // continue by checking triples
    p->vFanins->nSize = 3;
    for ( i = 2; i < nGs; i++ )
    for ( k = 1; k < i; k++ )
    for ( j = 0; j < k; j++ )
    {
        if ( pCexes[i] & pCexes[k] & pCexes[j] )
            continue;
        pDivs[0] = g[i];  p->vFanins->pArray[0] = i;
        pDivs[1] = g[k];  p->vFanins->pArray[1] = k;
        pDivs[2] = g[j];  p->vFanins->pArray[2] = j;
        uTruth = Rsb_DecCheck( nVars, f, pDivs, Vec_IntSize(p->vFanins), pPat, &iCexA, &iCexB );
        if ( uTruth )
        {
            if ( fFindAll )
            {
                uTruth = (unsigned)Abc_Tt6Stretch( (word)uTruth, 3 );
                Kit_DsdPrintFromTruth( &uTruth, 3 ); printf( "   " );
                Vec_IntPrint(p->vFanins);
                continue;
            }
            else
                return uTruth;
        }
        if ( nCexes == 64 )
            return 0;
        Rsb_DecVerifyCex( f, pDivs, Vec_IntSize(p->vFanins), iCexA, iCexB );
        Rsb_DecRecordCex( g, nGsAll, iCexA, iCexB, pCexes, nCexes++ );
        if ( !p->fVerbose )
            continue;
        Vec_IntPush( p->vTries, i );
        Vec_IntPush( p->vTries, k );
        Vec_IntPush( p->vTries, j );
        Vec_IntPush( p->vTries, -1 );
    }
    if ( p->nDecMax == 3 )
        return 0;
    // continue by checking quadras
    p->vFanins->nSize = 4;
    for ( i = 3; i < nGs; i++ )
    for ( k = 2; k < i; k++ )
    for ( j = 1; j < k; j++ )
    for ( n = 0; n < j; n++ )
    {
        if ( pCexes[i] & pCexes[k] & pCexes[j] & pCexes[n] )
            continue;
        pDivs[0] = g[i];  p->vFanins->pArray[0] = i;
        pDivs[1] = g[k];  p->vFanins->pArray[1] = k;
        pDivs[2] = g[j];  p->vFanins->pArray[2] = j;
        pDivs[3] = g[n];  p->vFanins->pArray[3] = n;
        uTruth = Rsb_DecCheck( nVars, f, pDivs, Vec_IntSize(p->vFanins), pPat, &iCexA, &iCexB );
        if ( uTruth )
        {
            if ( fFindAll )
            {
                uTruth = (unsigned)Abc_Tt6Stretch( (word)uTruth, 4 );
                Kit_DsdPrintFromTruth( &uTruth, 4 ); printf( "   " );
                Vec_IntPrint(p->vFanins);
                continue;
            }
            else
                return uTruth;
        }
        if ( nCexes == 64 )
            return 0;
        Rsb_DecVerifyCex( f, pDivs, Vec_IntSize(p->vFanins), iCexA, iCexB );
        Rsb_DecRecordCex( g, nGsAll, iCexA, iCexB, pCexes, nCexes++ );
        if ( !p->fVerbose )
            continue;
        Vec_IntPush( p->vTries, i );
        Vec_IntPush( p->vTries, k );
        Vec_IntPush( p->vTries, j );
        Vec_IntPush( p->vTries, n );
        Vec_IntPush( p->vTries, -1 );
    }
//    printf( "%d ", nCexes );
    if ( p->nDecMax == 4 )
        return 0;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Verifies 4-input decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rsb_DecPrintFunc( Rsb_Man_t * p, unsigned Truth4, word * f, word ** ppGs, int nGs, int nVarsAll )
{
    int nVars  =  Vec_IntSize(p->vFanins);
    word Copy  =  Truth4;
    word wOn   =  Abc_Tt6Stretch( Copy >> (1 << nVars), nVars );
    word wOnDc = ~Abc_Tt6Stretch( Copy, nVars );
    word wIsop =  Abc_Tt6Isop( wOn, wOnDc, nVars, NULL );
    int i;

    printf( "Offset : " );
    Abc_TtPrintBinary( &Copy, nVars );  //printf( "\n" );
    Copy >>= ((word)1 << nVars);
    printf( "Onset  : " );
    Abc_TtPrintBinary( &Copy, nVars );  //printf( "\n" );
    printf( "Result : " );
    Abc_TtPrintBinary( &wIsop, nVars ); //printf( "\n" );
    Kit_DsdPrintFromTruth( (unsigned *)&wIsop, nVars );  printf( "\n" );

    // print functions
    printf( "Func   : " );
    Abc_TtPrintBinary( f, nVarsAll );   //printf( "\n" );
    Kit_DsdPrintFromTruth( (unsigned *)f, nVarsAll );    printf( "\n" );
    for ( i = 0; i < nGs; i++ )
    {
        printf( "Div%3d : ", i );
        Kit_DsdPrintFromTruth( (unsigned *)ppGs[i], nVarsAll );  printf( "\n" );
    }
    printf( "Solution : " );
    for ( i = 0; i < Vec_IntSize(p->vFanins); i++ )
        printf( "%d ", Vec_IntEntry(p->vFanins, i) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Verifies 4-input decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rsb_DecVerify( Rsb_Man_t * p, int nVars, word * f, word ** g, int nGs, unsigned Truth4, word * pTemp1, word * pTemp2 )
{
    word * pFanins[16];
    int b, m, Num, nSuppSize;
    int nWords = Abc_TtWordNum(nVars);
    Truth4 >>= (1 << Vec_IntSize(p->vFanins));
    Truth4 = (unsigned)Abc_Tt6Stretch( (word)Truth4, Vec_IntSize(p->vFanins) );
//Kit_DsdPrintFromTruth( (unsigned *)&Truth4, Vec_IntSize(p->vFanins) );
//printf( "\n" );
//    nSuppSize = Rsb_Word6SupportSize( Truth4 );
//    assert( nSuppSize == Vec_IntSize(p->vFanins) );
    nSuppSize = Vec_IntSize(p->vFanins);
    // collect the fanins
    Vec_IntForEachEntry( p->vFanins, Num, b )
    {
        assert( Num < nGs );
        pFanins[b] = g[Num];
    }
    // create the or of ands
    Abc_TtClear( pTemp1, nWords );
    for ( m = 0; m < (1<<nSuppSize); m++ )
    {
        if ( ((Truth4 >> m) & 1) == 0 )
            continue;
        Abc_TtFill( pTemp2, nWords );
        for ( b = 0; b < nSuppSize; b++ )
            if ( (m >> b) & 1 )
                Abc_TtAnd( pTemp2, pTemp2, pFanins[b], nWords, 0 );
            else
                Abc_TtSharp( pTemp2, pTemp2, pFanins[b], nWords );
        Abc_TtOr( pTemp1, pTemp1, pTemp2, nWords );
    }
    // check the function
    if ( !Abc_TtEqual( pTemp1, f, nWords ) )
        printf( "Verification failed.\n" );
//    else
//        printf( "Verification passed.\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Finds a setset of gs to decompose f.]

  Description [Returns the numbers of the decomposition functions and 
  the truth table of a function up to 4 variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Rsb_ManPerform( Rsb_Man_t * p, int nVars, word * f, word ** g, int nGs, int nGsAll, int fVerbose0 ) 
{
    word * pCexes = Vec_WrdArray(p->vCexes);
    unsigned * pPat = (unsigned *)Vec_IntArray(p->vDecPats);
    int fVerbose = 0;//(nGs > 40);
    Vec_Int_t * vTries = NULL;
    unsigned uTruth;

    // verify original decomposition
    if ( Vec_IntSize(p->vFaninsOld) && Vec_IntSize(p->vFaninsOld) <= 4 )
    {
        word * pDivs[8];
        int i, Entry, iCexA, iCexB;
        Vec_IntForEachEntry( p->vFaninsOld, Entry, i )
            pDivs[i] = g[Entry];
        uTruth = Rsb_DecCheck( nVars, f, pDivs, Vec_IntSize(p->vFaninsOld), pPat, &iCexA, &iCexB );
//        assert( uTruth != 0 );
        if ( fVerbose )
        {
            printf( "Verified orig decomp with %d vars {", Vec_IntSize(p->vFaninsOld) );
            Vec_IntForEachEntry( p->vFaninsOld, Entry, i )
                printf( " %d", Entry );
            printf( " }\n" );
        }
        if ( uTruth )
        {
//            if ( fVerbose )
//                Rsb_DecPrintFunc( p, uTruth );
        }
        else 
        {
            printf( "Verified orig decomp with %d vars {", Vec_IntSize(p->vFaninsOld) );
            Vec_IntForEachEntry( p->vFaninsOld, Entry, i )
                printf( " %d", Entry );
            printf( " }\n" );
            printf( "Verification FAILED.\n" );
        }
    }
    // start tries
if ( fVerbose )
vTries = Vec_IntAlloc( 100 );
    assert( nGs < nGsAll );
    uTruth = Rsb_DecPerformInt( p, nVars, f, g, nGs, nGsAll, 0 );

    if ( uTruth )
    {
        if ( fVerbose )
        {
            int i, Entry;
            printf( "Found decomp with %d vars {", Vec_IntSize(p->vFanins) );
            Vec_IntForEachEntry( p->vFanins, Entry, i )
                printf( " %d", Entry );
            printf( " }\n" );
//            Rsb_DecPrintFunc( p, uTruth );
//            Rsb_DecVerify( nVars, f, g, nGs, p->vFanins, uTruth, g[nGsAll], g[nGsAll+1] );
        }
    }
    else 
    {
        Vec_IntShrink( p->vFanins, 0 );
        if ( fVerbose )
        printf( "Did not find decomposition with 4 variables.\n" );
    }

if ( fVerbose )
Rsb_DecPrintTable( pCexes, nGs, nGsAll, vTries );
if ( fVerbose )
Vec_IntFree( vTries );
    return uTruth;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rsb_ManPerformResub6( Rsb_Man_t * p, int nVarsAll, word uTruth, Vec_Wrd_t * vDivTruths, word * puTruth0, word * puTruth1, int fVerbose )
{
    word * pGs[200];
    unsigned uTruthRes;
    int i, nVars, nGs = Vec_WrdSize(vDivTruths);
    assert( nGs < 200 );
    for ( i = 0; i < nGs; i++ )
        pGs[i] = Vec_WrdEntryP(vDivTruths,i);
    uTruthRes = Rsb_DecPerformInt( p, nVarsAll, &uTruth, pGs, nGs, nGs, 0 );
    if ( uTruthRes == 0 )
        return 0;

    if ( fVerbose )
        Rsb_DecPrintFunc( p, uTruthRes, &uTruth, pGs, nGs, nVarsAll );
    if ( fVerbose )
        Rsb_DecPrintTable( Vec_WrdArray(p->vCexes), nGs, nGs, p->vTries );

    nVars = Vec_IntSize(p->vFanins);
    *puTruth0 = Abc_Tt6Stretch( uTruthRes,                 nVars );
    *puTruth1 = Abc_Tt6Stretch( uTruthRes >> (1 << nVars), nVars );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rsb_ManPerformResub6Test()
{
    Rsb_Man_t * p;
    Vec_Wrd_t * vDivTruths;
    int RetValue;
    word a = s_Truths6[0];
    word b = s_Truths6[1];
    word c = s_Truths6[2];
    word d = s_Truths6[3];
    word e = s_Truths6[4];
    word f = s_Truths6[5];
    word ab = a & b;
    word cd = c & d;
    word ef = e & f;
    word F = ab | cd | ef;
    word uTruth0, uTruth1;

    vDivTruths = Vec_WrdAlloc( 100 );
    Vec_WrdPush( vDivTruths, a );
    Vec_WrdPush( vDivTruths, b );
    Vec_WrdPush( vDivTruths, c );
    Vec_WrdPush( vDivTruths, d );
    Vec_WrdPush( vDivTruths, e );
    Vec_WrdPush( vDivTruths, f );
    Vec_WrdPush( vDivTruths, ab );
    Vec_WrdPush( vDivTruths, cd );
    Vec_WrdPush( vDivTruths, ef );

    p = Rsb_ManAlloc( 6, 64, 4, 1 );

    RetValue = Rsb_ManPerformResub6( p, 6, F, vDivTruths, &uTruth0, &uTruth1, 1 );

    Rsb_ManFree( p );
    Vec_WrdFree( vDivTruths );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

