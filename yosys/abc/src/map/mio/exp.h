/**CFile****************************************************************

  FileName    [exp.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Boolean expression.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: exp.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__map__mio__exp_h
#define ABC__map__mio__exp_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define EXP_CONST0 -1
#define EXP_CONST1 -2

static inline Vec_Int_t * Exp_Const0()
{
    Vec_Int_t * vExp;
    vExp = Vec_IntAlloc( 1 );
    Vec_IntPush( vExp, EXP_CONST0 );
    return vExp;
}
static inline Vec_Int_t * Exp_Const1()
{
    Vec_Int_t * vExp;
    vExp = Vec_IntAlloc( 1 );
    Vec_IntPush( vExp, EXP_CONST1 );
    return vExp;
}
static inline int Exp_IsConst( Vec_Int_t * p )
{
    return Vec_IntEntry(p,0) == EXP_CONST0 || Vec_IntEntry(p,0) == EXP_CONST1;
}
static inline int Exp_IsConst0( Vec_Int_t * p )
{
    return Vec_IntEntry(p,0) == EXP_CONST0;
}
static inline int Exp_IsConst1( Vec_Int_t * p )
{
    return Vec_IntEntry(p,0) == EXP_CONST1;
}
static inline Vec_Int_t * Exp_Var( int iVar )
{
    Vec_Int_t * vExp;
    vExp = Vec_IntAlloc( 1 );
    Vec_IntPush( vExp, 2 * iVar );
    return vExp;
}
static inline int Exp_LitShift( int nVars, int Lit, int Shift )
{
    if ( Lit < 2 * nVars )
        return Lit;
    return Lit + 2 * Shift;
}
static inline int Exp_IsLit( Vec_Int_t * p )
{
    return Vec_IntSize(p) == 1 && !Exp_IsConst(p);
}
static inline int Exp_NodeNum( Vec_Int_t * p )
{
    return Vec_IntSize(p)/2;
}
static inline Vec_Int_t * Exp_Not( Vec_Int_t * p )
{
    Vec_IntWriteEntry( p, 0, Vec_IntEntry(p,0) ^ 1 );
    return p;
}
static inline void Exp_PrintLit( int nVars, int Lit )
{
    if ( Lit == EXP_CONST0 )
        Abc_Print( 1, "Const0" );
    else if ( Lit == EXP_CONST1 )
        Abc_Print( 1, "Const1" );
    else if ( Lit < 2 * nVars )
        Abc_Print( 1, "%s%c", (Lit&1) ? "!" : " ", 'a' + Lit/2 );
    else
        Abc_Print( 1, "%s%d", (Lit&1) ? "!" : " ", Lit/2 );
}
static inline void Exp_Print( int nVars, Vec_Int_t * p )
{
    int i;
    for ( i = 0; i < Exp_NodeNum(p); i++ )
    {
        Abc_Print( 1, "%2d = ", nVars + i );
        Exp_PrintLit( nVars, Vec_IntEntry(p, 2*i+0) );
        Abc_Print( 1, " & " );
        Exp_PrintLit( nVars, Vec_IntEntry(p, 2*i+1) );
        Abc_Print( 1, "\n" );
    }
    Abc_Print( 1, " F = " );
    Exp_PrintLit( nVars, Vec_IntEntryLast(p) );
    Abc_Print( 1, "\n" );
}
static inline Vec_Int_t * Exp_Reverse( Vec_Int_t * p )
{
    Vec_IntReverseOrder( p );
    return p;
}
static inline void Exp_PrintReverse( int nVars, Vec_Int_t * p )
{
    Exp_Reverse( p );
    Exp_Print( nVars, p );
    Exp_Reverse( p );
}
static inline Vec_Int_t * Exp_And( int * pMan, int nVars, Vec_Int_t * p0, Vec_Int_t * p1, int fCompl0, int fCompl1 )
{
    int i, Len0 = Vec_IntSize(p0), Len1 = Vec_IntSize(p1);
    Vec_Int_t * r = Vec_IntAlloc( Len0 + Len1 + 1 );
    assert( (Len0 & 1) && (Len1 & 1) );
    Vec_IntPush( r, 2 * (nVars + Len0/2 + Len1/2) );
    Vec_IntPush( r, Exp_LitShift( nVars, Vec_IntEntry(p0, 0) ^ fCompl0, Len1/2 ) );
    Vec_IntPush( r, Vec_IntEntry(p1, 0) ^ fCompl1 );
    for ( i = 1; i < Len0; i++ )
        Vec_IntPush( r, Exp_LitShift( nVars, Vec_IntEntry(p0, i), Len1/2 ) );
    for ( i = 1; i < Len1; i++ )
        Vec_IntPush( r, Vec_IntEntry(p1, i) );
    assert( Vec_IntSize(r) == Len0 + Len1 + 1 );
    return r;
}
static inline Vec_Int_t * Exp_Or( int * pMan, int nVars, Vec_Int_t * p0, Vec_Int_t * p1 )
{
    return Exp_Not( Exp_And(pMan, nVars, p0, p1, 1, 1) );
}
static inline Vec_Int_t * Exp_Xor( int * pMan, int nVars, Vec_Int_t * p0, Vec_Int_t * p1 )
{
    int i, Len0 = Vec_IntSize(p0), Len1 = Vec_IntSize(p1);
    Vec_Int_t * r = Vec_IntAlloc( Len0 + Len1 + 5 );
    assert( (Len0 & 1) && (Len1 & 1) );
    Vec_IntPush( r, 2 * (nVars + Len0/2 + Len1/2 + 2)     );
    Vec_IntPush( r, 2 * (nVars + Len0/2 + Len1/2 + 1) + 1 );
    Vec_IntPush( r, 2 * (nVars + Len0/2 + Len1/2 + 0) + 1 );
    Vec_IntPush( r, Exp_LitShift( nVars, Vec_IntEntry(p0, 0) ^ 1, Len1/2 ) );
    Vec_IntPush( r, Vec_IntEntry(p1, 0) );
    Vec_IntPush( r, Exp_LitShift( nVars, Vec_IntEntry(p0, 0), Len1/2 ) );
    Vec_IntPush( r, Vec_IntEntry(p1, 0) ^ 1 );
    for ( i = 1; i < Len0; i++ )
        Vec_IntPush( r, Exp_LitShift( nVars, Vec_IntEntry(p0, i), Len1/2 ) );
    for ( i = 1; i < Len1; i++ )
        Vec_IntPush( r, Vec_IntEntry(p1, i) );
    assert( Vec_IntSize(r) == Len0 + Len1 + 5 );
    return Exp_Not( r );
}
static inline word Exp_Truth6Lit( int nVars, int Lit, word * puFanins, word * puNodes )
{
    if ( Lit == EXP_CONST0 )
        return 0;
    if ( Lit == EXP_CONST1 )
        return ~(word)0;
    if ( Lit < 2 * nVars )
        return  (Lit&1) ? ~puFanins[Lit/2] : puFanins[Lit/2];
    return (Lit&1) ? ~puNodes[Lit/2-nVars] : puNodes[Lit/2-nVars];
}
static inline word Exp_Truth6( int nVars, Vec_Int_t * p, word * puFanins )
{
    static word Truth6[6] = {
        ABC_CONST(0xAAAAAAAAAAAAAAAA),
        ABC_CONST(0xCCCCCCCCCCCCCCCC),
        ABC_CONST(0xF0F0F0F0F0F0F0F0),
        ABC_CONST(0xFF00FF00FF00FF00),
        ABC_CONST(0xFFFF0000FFFF0000),
        ABC_CONST(0xFFFFFFFF00000000)
    };
    word * puNodes, Res;
    int i;
    if ( puFanins == NULL )
        puFanins = (word *)Truth6;
    puNodes = ABC_CALLOC( word, Exp_NodeNum(p) );
    for ( i = 0; i < Exp_NodeNum(p); i++ )
        puNodes[i] = Exp_Truth6Lit( nVars, Vec_IntEntry(p, 2*i+0), puFanins, puNodes ) & 
                     Exp_Truth6Lit( nVars, Vec_IntEntry(p, 2*i+1), puFanins, puNodes );
    Res = Exp_Truth6Lit( nVars, Vec_IntEntryLast(p), puFanins, puNodes );
    ABC_FREE( puNodes );
    return Res;
}
static inline void Exp_Truth8( int nVars, Vec_Int_t * p, word ** puFanins, word * puRes )
{
    word Truth8[8][4] = {
        { ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA) },
        { ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC) },
        { ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0) },
        { ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00) },
        { ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000) },
        { ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000) },
        { ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF) },
        { ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF) }
    };
    word * puFaninsInt[8], * pStore, * pThis = NULL;
    int i, k, iRoot = Vec_IntEntryLast(p);
    if ( puFanins == NULL )
    {
        puFanins = puFaninsInt;
        for ( k = 0; k < 8; k++ )
            puFanins[k] = Truth8[k];
    }
    if ( Exp_NodeNum(p) == 0 )
    {
        assert( iRoot < 2 * nVars );
        if ( iRoot == EXP_CONST0 || iRoot == EXP_CONST1 )
            for ( k = 0; k < 4; k++ )
                puRes[k] = iRoot == EXP_CONST0 ? 0 : ~(word)0;
        else
            for ( k = 0; k < 4; k++ )
                puRes[k] = Abc_LitIsCompl(iRoot) ? ~puFanins[Abc_Lit2Var(iRoot)][k] : puFanins[Abc_Lit2Var(iRoot)][k];
        return;
    }
    pStore = ABC_CALLOC( word, 4 * Exp_NodeNum(p) );
    for ( i = 0; i < Exp_NodeNum(p); i++ )
    {
        int iVar0 = Abc_Lit2Var( Vec_IntEntry(p, 2*i+0) );
        int iVar1 = Abc_Lit2Var( Vec_IntEntry(p, 2*i+1) );
        int fCompl0 = Abc_LitIsCompl( Vec_IntEntry(p, 2*i+0) );
        int fCompl1 = Abc_LitIsCompl( Vec_IntEntry(p, 2*i+1) );
        word * pIn0 = iVar0 < nVars ? puFanins[iVar0] : pStore + 4 * (iVar0 - nVars);
        word * pIn1 = iVar1 < nVars ? puFanins[iVar1] : pStore + 4 * (iVar1 - nVars);
        pThis = pStore + 4 * i;
        if ( fCompl0 && fCompl1 )
            for ( k = 0; k < 4; k++ )
                pThis[k] = ~pIn0[k] & ~pIn1[k];
        else if ( fCompl0 && !fCompl1 )
            for ( k = 0; k < 4; k++ )
                pThis[k] = ~pIn0[k] &  pIn1[k];
        else if ( !fCompl0 && fCompl1 )
            for ( k = 0; k < 4; k++ )
                pThis[k] =  pIn0[k] & ~pIn1[k];
        else //if ( !fCompl0 && !fCompl1 )
            for ( k = 0; k < 4; k++ )
                pThis[k] =  pIn0[k] &  pIn1[k];
    }
    assert( Abc_Lit2Var(iRoot) - nVars == i - 1 );
    for ( k = 0; k < 4; k++ )
        puRes[k] = Abc_LitIsCompl(iRoot) ? ~pThis[k] : pThis[k];
    ABC_FREE( pStore );
}
static inline void Exp_TruthLit( int nVars, int Lit, word ** puFanins, word ** puNodes, word * pRes, int nWords )
{
    int w;
    if ( Lit == EXP_CONST0 )
        for ( w = 0; w < nWords; w++ )
            pRes[w] = 0;
    else if ( Lit == EXP_CONST1 )
        for ( w = 0; w < nWords; w++ )
            pRes[w] = ~(word)0;
    else if ( Lit < 2 * nVars )
        for ( w = 0; w < nWords; w++ )
            pRes[w] = (Lit&1) ? ~puFanins[Lit/2][w] : puFanins[Lit/2][w];
    else
        for ( w = 0; w < nWords; w++ )
            pRes[w] = (Lit&1) ? ~puNodes[Lit/2-nVars][w] : puNodes[Lit/2-nVars][w];
}
static inline void Exp_Truth( int nVars, Vec_Int_t * p, word * pRes )
{
    static word Truth6[6] = {
        ABC_CONST(0xAAAAAAAAAAAAAAAA),
        ABC_CONST(0xCCCCCCCCCCCCCCCC),
        ABC_CONST(0xF0F0F0F0F0F0F0F0),
        ABC_CONST(0xFF00FF00FF00FF00),
        ABC_CONST(0xFFFF0000FFFF0000),
        ABC_CONST(0xFFFFFFFF00000000)
    };
    word ** puFanins, ** puNodes, * pTemp0, * pTemp1;
    int i, w, nWords = (nVars <= 6 ? 1 : 1 << (nVars-6));
    // create elementary variables
    puFanins = ABC_ALLOC( word *, nVars );
    for ( i = 0; i < nVars; i++ )
        puFanins[i] = ABC_ALLOC( word, nWords );
    // assign elementary truth tables
    for ( i = 0; i < nVars; i++ )
        if ( i < 6 )
            for ( w = 0; w < nWords; w++ )
                puFanins[i][w] = Truth6[i];
        else
            for ( w = 0; w < nWords; w++ )
                puFanins[i][w] = (w & (1 << (i-6))) ? ~(word)0 : 0;
    // create intermediate nodes
    puNodes = ABC_ALLOC( word *, Exp_NodeNum(p) );
    for ( i = 0; i < Exp_NodeNum(p); i++ )
        puNodes[i] = ABC_ALLOC( word, nWords );
    // evaluate the expression
    pTemp0 = ABC_ALLOC( word, nWords );
    pTemp1 = ABC_ALLOC( word, nWords );
    for ( i = 0; i < Exp_NodeNum(p); i++ )
    {
        Exp_TruthLit( nVars, Vec_IntEntry(p, 2*i+0), puFanins, puNodes, pTemp0, nWords );
        Exp_TruthLit( nVars, Vec_IntEntry(p, 2*i+1), puFanins, puNodes, pTemp1, nWords );
        for ( w = 0; w < nWords; w++ )
            puNodes[i][w] = pTemp0[w] & pTemp1[w];
    }
    ABC_FREE( pTemp0 );
    ABC_FREE( pTemp1 );
    // copy the final result
    Exp_TruthLit( nVars, Vec_IntEntryLast(p), puFanins, puNodes, pRes, nWords );
    // cleanup
    for ( i = 0; i < nVars; i++ )
        ABC_FREE( puFanins[i] );
    ABC_FREE( puFanins );
    for ( i = 0; i < Exp_NodeNum(p); i++ )
        ABC_FREE( puNodes[i] );
    ABC_FREE( puNodes );
}

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_END


#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

