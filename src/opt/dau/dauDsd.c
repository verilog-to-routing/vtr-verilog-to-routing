/**CFile****************************************************************

  FileName    [dauDsd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    [Disjoint-support decomposition.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dauDsd.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/* 
    This code performs truth-table-based decomposition for 6-variable functions.
    Representation of operations:
    ! = not; 
    (ab) = a and b;  
    [ab] = a xor b;  
    <abc> = ITE( a, b, c )
    FUNCTION{abc} = FUNCTION( a, b, c )
*/


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Elementary truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word ** Dau_DsdTtElems()
{
    static word TtElems[DAU_MAX_VAR+1][DAU_MAX_WORD], * pTtElems[DAU_MAX_VAR+1] = {NULL};
    if ( pTtElems[0] == NULL )
    {
        int v;
        for ( v = 0; v <= DAU_MAX_VAR; v++ )
            pTtElems[v] = TtElems[v];
        Abc_TtElemInit( pTtElems, DAU_MAX_VAR );
    }
    return pTtElems;
}

/**Function*************************************************************

  Synopsis    [DSD formula manipulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Dau_DsdComputeMatches( char * p )
{
    static int pMatches[DAU_MAX_STR];
    int pNested[DAU_MAX_VAR];
    int v, nNested = 0;
    for ( v = 0; p[v]; v++ )
    {
        pMatches[v] = 0;
        if ( p[v] == '(' || p[v] == '[' || p[v] == '<' || p[v] == '{' )
            pNested[nNested++] = v;
        else if ( p[v] == ')' || p[v] == ']' || p[v] == '>' || p[v] == '}' )
            pMatches[pNested[--nNested]] = v;
        assert( nNested < DAU_MAX_VAR );
    }
    assert( nNested == 0 );
    return pMatches;
}

/**Function*************************************************************

  Synopsis    [Generate random permutation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_DsdFindVarNum( char * pDsd )
{
    int vMax = 0;
    pDsd--;
    while ( *++pDsd )
        if ( *pDsd >= 'a' && *pDsd <= 'z' )
            vMax = Abc_MaxInt( vMax, *pDsd - 'a' );
    return vMax + 1;
}
void Dau_DsdGenRandPerm( int * pPerm, int nVars )
{
    int v, vNew;
    for ( v = 0; v < nVars; v++ )
        pPerm[v] = v;
    for ( v = 0; v < nVars; v++ )
    {
        vNew = rand() % nVars;
        ABC_SWAP( int, pPerm[v], pPerm[vNew] );
    }
}
void Dau_DsdPermute( char * pDsd )
{
    int pPerm[16];
    int nVars = Dau_DsdFindVarNum( pDsd );
    Dau_DsdGenRandPerm( pPerm, nVars );
    pDsd--;
    while ( *++pDsd )
        if ( *pDsd >= 'a' && *pDsd < 'a' + nVars )
            *pDsd = 'a' + pPerm[*pDsd - 'a'];
}

/**Function*************************************************************

  Synopsis    [Normalize the ordering of components.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Dau_DsdNormalizeCopy( char * pDest, char * pSour, int * pMarks, int i )
{
    int s;
    for ( s = pMarks[i]; s < pMarks[i+1]; s++ )
        *pDest++ = pSour[s];
    return pDest;
}
int Dau_DsdNormalizeCompare( char * pStr, int * pMarks, int i, int j )
{
    char * pStr1 = pStr + pMarks[i];
    char * pStr2 = pStr + pMarks[j];
    char * pLimit1 = pStr + pMarks[i+1];
    char * pLimit2 = pStr + pMarks[j+1];
    for ( ; pStr1 < pLimit1 && pStr2 < pLimit2; pStr1++, pStr2++ )
    {
        if ( !(*pStr1 >= 'a' &&  *pStr1 <= 'z') )
        {
            pStr2--;
            continue;
        }
        if ( !(*pStr2 >= 'a' &&  *pStr2 <= 'z') )
        {
            pStr1--;
            continue;
        }
        if ( *pStr1 < *pStr2 )
            return -1;
        if ( *pStr1 > *pStr2 )
            return 1;
    }
    assert( pStr1 < pLimit1 || pStr2 < pLimit2 );
    if ( pStr1 == pLimit1 )
        return -1;
    if ( pStr2 == pLimit2 )
        return 1;
    assert( 0 );
    return 0;
}
int * Dau_DsdNormalizePerm( char * pStr, int * pMarks, int nMarks )
{
    static int pPerm[DAU_MAX_VAR];
    int i, k;
    for ( i = 0; i < nMarks; i++ )
        pPerm[i] = i;
    for ( i = 0; i < nMarks; i++ )
    {
        int iBest = i;
        for ( k = i + 1; k < nMarks; k++ )
            if ( Dau_DsdNormalizeCompare( pStr, pMarks, pPerm[iBest], pPerm[k] ) == 1 )
                iBest = k;
        ABC_SWAP( int, pPerm[i], pPerm[iBest] );
    }
    return pPerm;
}
void Dau_DsdNormalize_rec( char * pStr, char ** p, int * pMatches )
{
    static char pBuffer[DAU_MAX_STR];
    if ( **p == '!' )
        (*p)++;
    while ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
        (*p)++;
    if ( **p == '<' )
    {
        char * q = pStr + pMatches[*p - pStr];
        if ( *(q+1) == '{' )
            *p = q+1;
    }
    if ( **p >= 'a' && **p <= 'z' ) // var
        return;
    if ( **p == '(' || **p == '[' ) // and/or/xor
    {
        char * pStore, * pOld = *p + 1;
        char * q = pStr + pMatches[ *p - pStr ];
        int i, * pPerm, nMarks = 0, pMarks[DAU_MAX_VAR+1];
        assert( *q == **p + 1 + (**p != '(') );
        for ( (*p)++; *p < q; (*p)++ )
        {
            pMarks[nMarks++] = *p - pStr;
            Dau_DsdNormalize_rec( pStr, p, pMatches );
        }
        pMarks[nMarks] = *p - pStr;
        assert( *p == q );
        // add to buffer in good order
        pPerm = Dau_DsdNormalizePerm( pStr, pMarks, nMarks );
        // copy to the buffer
        pStore = pBuffer;
        for ( i = 0; i < nMarks; i++ )
            pStore = Dau_DsdNormalizeCopy( pStore, pStr, pMarks, pPerm[i] );
        assert( pStore - pBuffer == *p - pOld );
        memcpy( pOld, pBuffer, pStore - pBuffer );
        return;
    }
    if ( **p == '<' || **p == '{' ) // mux
    {
        char * q = pStr + pMatches[ *p - pStr ];
        assert( *q == **p + 1 + (**p != '(') );
        if ( (**p == '<') && (*(q+1) == '{') )
        {
            *p = q+1;
            Dau_DsdNormalize_rec( pStr, p, pMatches );
            return;
        }
        for ( (*p)++; *p < q; (*p)++ )
            Dau_DsdNormalize_rec( pStr, p, pMatches );
        assert( *p == q );
        return;
    }
    assert( 0 );
}
void Dau_DsdNormalize( char * pDsd )
{
    if ( pDsd[1] != 0 )
        Dau_DsdNormalize_rec( pDsd, &pDsd, Dau_DsdComputeMatches(pDsd) );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_DsdCountAnds_rec( char * pStr, char ** p, int * pMatches )
{
    if ( **p == '!' )
        (*p)++;
    while ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
        (*p)++;
    if ( **p == '<' )
    {
        char * q = pStr + pMatches[*p - pStr];
        if ( *(q+1) == '{' )
            *p = q+1;
    }
    if ( **p >= 'a' && **p <= 'z' ) // var
        return 0;
    if ( **p == '(' || **p == '[' ) // and/or/xor
    {
        int Counter = 0, AddOn = (**p == '(')? 1 : 3;
        char * q = pStr + pMatches[ *p - pStr ];
        assert( *q == **p + 1 + (**p != '(') );
        for ( (*p)++; *p < q; (*p)++ )
            Counter += AddOn + Dau_DsdCountAnds_rec( pStr, p, pMatches );
        assert( *p == q );
        return Counter - AddOn;
    }
    if ( **p == '<' || **p == '{' ) // mux
    {
        int Counter = 3;
        char * q = pStr + pMatches[ *p - pStr ];
        assert( *q == **p + 1 + (**p != '(') );
        for ( (*p)++; *p < q; (*p)++ )
            Counter += Dau_DsdCountAnds_rec( pStr, p, pMatches );
        assert( *p == q );
        return Counter;
    }
    assert( 0 );
    return 0;
}
int Dau_DsdCountAnds( char * pDsd )
{
    if ( pDsd[1] == 0 )
        return 0;
    return Dau_DsdCountAnds_rec( pDsd, &pDsd, Dau_DsdComputeMatches(pDsd) );
}

/**Function*************************************************************

  Synopsis    [Computes truth table for the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Dau_Dsd6TruthCompose_rec( word Func, word * pFanins, int nVars )
{
    word t0, t1;
    if ( Func == 0 )
        return 0;
    if ( Func == ~(word)0 )
        return ~(word)0;
    assert( nVars > 0 );
    if ( --nVars == 0 )
    {
        assert( Func == s_Truths6[0] || Func == s_Truths6Neg[0] );
        return (Func == s_Truths6[0]) ? pFanins[0] : ~pFanins[0];
    }
    if ( !Abc_Tt6HasVar(Func, nVars) )
        return Dau_Dsd6TruthCompose_rec( Func, pFanins, nVars );
    t0 = Dau_Dsd6TruthCompose_rec( Abc_Tt6Cofactor0(Func, nVars), pFanins, nVars );
    t1 = Dau_Dsd6TruthCompose_rec( Abc_Tt6Cofactor1(Func, nVars), pFanins, nVars );
    return (~pFanins[nVars] & t0) | (pFanins[nVars] & t1);
}
word Dau_Dsd6ToTruth_rec( char * pStr, char ** p, int * pMatches, word * pTruths )
{
    int fCompl = 0;
    if ( **p == '!' )
        (*p)++, fCompl = 1;
    if ( **p >= 'a' && **p <= 'f' ) // var
    {
        assert( **p - 'a' >= 0 && **p - 'a' < 6 );
        return fCompl ? ~pTruths[**p - 'a'] : pTruths[**p - 'a'];
    }
    if ( **p == '(' ) // and/or
    {
        char * q = pStr + pMatches[ *p - pStr ];
        word Res = ~(word)0;
        assert( **p == '(' && *q == ')' );
        for ( (*p)++; *p < q; (*p)++ )
            Res &= Dau_Dsd6ToTruth_rec( pStr, p, pMatches, pTruths );
        assert( *p == q );
        return fCompl ? ~Res : Res;
    }
    if ( **p == '[' ) // xor
    {
        char * q = pStr + pMatches[ *p - pStr ];
        word Res = 0;
        assert( **p == '[' && *q == ']' );
        for ( (*p)++; *p < q; (*p)++ )
            Res ^= Dau_Dsd6ToTruth_rec( pStr, p, pMatches, pTruths );
        assert( *p == q );
        return fCompl ? ~Res : Res;
    }
    if ( **p == '<' ) // mux
    {
        int nVars = 0;
        word Temp[3], * pTemp = Temp, Res;
        word Fanins[6], * pTruths2;
        char * pOld = *p;
        char * q = pStr + pMatches[ *p - pStr ];
        // read fanins
        if ( *(q+1) == '{' )
        {
            char * q2;
            *p = q+1;
            q2 = pStr + pMatches[ *p - pStr ];
            assert( **p == '{' && *q2 == '}' );
            for ( nVars = 0, (*p)++; *p < q2; (*p)++, nVars++ )
                Fanins[nVars] = Dau_Dsd6ToTruth_rec( pStr, p, pMatches, pTruths );
            assert( *p == q2 );
            pTruths2 = Fanins;
        }
        else
            pTruths2 = pTruths;
        // read MUX
        *p = pOld;
        q = pStr + pMatches[ *p - pStr ];
        assert( **p == '<' && *q == '>' );
        // verify internal variables
        if ( nVars )
            for ( ; pOld < q; pOld++ )
                if ( *pOld >= 'a' && *pOld <= 'z' )
                    assert( *pOld - 'a' < nVars );
        // derive MAX components
        for ( (*p)++; *p < q; (*p)++ )
            *pTemp++ = Dau_Dsd6ToTruth_rec( pStr, p, pMatches, pTruths2 );
        assert( pTemp == Temp + 3 );
        assert( *p == q );
        if ( *(q+1) == '{' ) // and/or
        {
            char * q = pStr + pMatches[ ++(*p) - pStr ];
            assert( **p == '{' && *q == '}' );
            *p = q;
        }
        Res = (Temp[0] & Temp[1]) | (~Temp[0] & Temp[2]);
        return fCompl ? ~Res : Res;
    }
    if ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
    {
        word Func, Fanins[6], Res;
        char * q;
        int i, nVars = Abc_TtReadHex( &Func, *p );
        *p += Abc_TtHexDigitNum( nVars );
        q = pStr + pMatches[ *p - pStr ];
        assert( **p == '{' && *q == '}' );
        for ( i = 0, (*p)++; *p < q; (*p)++, i++ )
            Fanins[i] = Dau_Dsd6ToTruth_rec( pStr, p, pMatches, pTruths );
        assert( i == nVars );
        assert( *p == q );
        Res = Dau_Dsd6TruthCompose_rec( Func, Fanins, nVars );
        return fCompl ? ~Res : Res;
    }
    assert( 0 );
    return 0;
}
word Dau_Dsd6ToTruth( char * p )
{
    word Res;
    if ( *p == '0' && *(p+1) == 0 )
        Res = 0;
    else if ( *p == '1' && *(p+1) == 0 )
        Res = ~(word)0;
    else
        Res = Dau_Dsd6ToTruth_rec( p, &p, Dau_DsdComputeMatches(p), s_Truths6 );
    assert( *++p == 0 );
    return Res;
}

/**Function*************************************************************

  Synopsis    [Computes truth table for the DSD formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_DsdTruth6Compose_rec( word Func, word pFanins[DAU_MAX_VAR][DAU_MAX_WORD], word * pRes, int nVars, int nWordsR )
{
    if ( Func == 0 )
    {
        Abc_TtConst0( pRes, nWordsR );
        return;
    }
    if ( Func == ~(word)0 )
    {
        Abc_TtConst1( pRes, nWordsR );
        return;
    }
    assert( nVars > 0 );
    if ( --nVars == 0 )
    {
        assert( Func == s_Truths6[0] || Func == s_Truths6Neg[0] );
        Abc_TtCopy( pRes, pFanins[0], nWordsR, Func == s_Truths6Neg[0] );
        return;
    }
    if ( !Abc_Tt6HasVar(Func, nVars) )
    {
        Dau_DsdTruth6Compose_rec( Func, pFanins, pRes, nVars, nWordsR );
        return;
    }
    {
        word pTtTemp[2][DAU_MAX_WORD];
        Dau_DsdTruth6Compose_rec( Abc_Tt6Cofactor0(Func, nVars), pFanins, pTtTemp[0], nVars, nWordsR );
        Dau_DsdTruth6Compose_rec( Abc_Tt6Cofactor1(Func, nVars), pFanins, pTtTemp[1], nVars, nWordsR );
        Abc_TtMux( pRes, pFanins[nVars], pTtTemp[1], pTtTemp[0], nWordsR );
        return;
    }
}
void Dau_DsdTruthCompose_rec( word * pFunc, word pFanins[DAU_MAX_VAR][DAU_MAX_WORD], word * pRes, int nVars, int nWordsR )
{
    int nWordsF;
    if ( nVars <= 6 )
    {
        Dau_DsdTruth6Compose_rec( pFunc[0], pFanins, pRes, nVars, nWordsR );
        return;
    }
    nWordsF = Abc_TtWordNum( nVars );
    assert( nWordsF > 1 );
    if ( Abc_TtIsConst0(pFunc, nWordsF) )
    {
        Abc_TtConst0( pRes, nWordsR );
        return;
    }
    if ( Abc_TtIsConst1(pFunc, nWordsF) )
    {
        Abc_TtConst1( pRes, nWordsR );
        return;
    }
    if ( !Abc_TtHasVar( pFunc, nVars, nVars-1 ) )
    {
        Dau_DsdTruthCompose_rec( pFunc, pFanins, pRes, nVars-1, nWordsR );
        return;
    }
    {
        word pTtTemp[2][DAU_MAX_WORD];
        nVars--;
        Dau_DsdTruthCompose_rec( pFunc,             pFanins, pTtTemp[0], nVars, nWordsR );
        Dau_DsdTruthCompose_rec( pFunc + nWordsF/2, pFanins, pTtTemp[1], nVars, nWordsR );
        Abc_TtMux( pRes, pFanins[nVars], pTtTemp[1], pTtTemp[0], nWordsR );
        return;
    }
}
void Dau_DsdToTruth_rec( char * pStr, char ** p, int * pMatches, word ** pTtElems, word * pRes, int nVars )
{
    int nWords = Abc_TtWordNum( nVars );
    int fCompl = 0;
    if ( **p == '!' )
        (*p)++, fCompl = 1;
    if ( **p >= 'a' && **p <= 'z' ) // var
    {
        assert( **p - 'a' >= 0 && **p - 'a' < nVars );
        Abc_TtCopy( pRes, pTtElems[**p - 'a'], nWords, fCompl );
        return;
    }
    if ( **p == '(' ) // and/or
    {
        char * q = pStr + pMatches[ *p - pStr ];
        word pTtTemp[DAU_MAX_WORD];
        assert( **p == '(' && *q == ')' );
        Abc_TtConst1( pRes, nWords );
        for ( (*p)++; *p < q; (*p)++ )
        {
            Dau_DsdToTruth_rec( pStr, p, pMatches, pTtElems, pTtTemp, nVars );
            Abc_TtAnd( pRes, pRes, pTtTemp, nWords, 0 );
        }
        assert( *p == q );
        if ( fCompl ) Abc_TtNot( pRes, nWords );
        return;
    }
    if ( **p == '[' ) // xor
    {
        char * q = pStr + pMatches[ *p - pStr ];
        word pTtTemp[DAU_MAX_WORD];
        assert( **p == '[' && *q == ']' );
        Abc_TtConst0( pRes, nWords );
        for ( (*p)++; *p < q; (*p)++ )
        {
            Dau_DsdToTruth_rec( pStr, p, pMatches, pTtElems, pTtTemp, nVars );
            Abc_TtXor( pRes, pRes, pTtTemp, nWords, 0 );
        }
        assert( *p == q );
        if ( fCompl ) Abc_TtNot( pRes, nWords );
        return;
    }
    if ( **p == '<' ) // mux
    {
        char * q = pStr + pMatches[ *p - pStr ];
        word pTtTemp[3][DAU_MAX_WORD];
        int i;
        assert( **p == '<' && *q == '>' );
        for ( i = 0, (*p)++; *p < q; (*p)++, i++ )
            Dau_DsdToTruth_rec( pStr, p, pMatches, pTtElems, pTtTemp[i], nVars );
        assert( i == 3 );
        Abc_TtMux( pRes, pTtTemp[0], pTtTemp[1], pTtTemp[2], nWords );
        assert( *p == q );
        if ( fCompl ) Abc_TtNot( pRes, nWords );
        return;
    }
    if ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
    {
        word pFanins[DAU_MAX_VAR][DAU_MAX_WORD], pFunc[DAU_MAX_WORD];
        char * q;
        int i, nVarsF = Abc_TtReadHex( pFunc, *p );
        *p += Abc_TtHexDigitNum( nVarsF );
        q = pStr + pMatches[ *p - pStr ];
        assert( **p == '{' && *q == '}' );
        for ( i = 0, (*p)++; *p < q; (*p)++, i++ )
            Dau_DsdToTruth_rec( pStr, p, pMatches, pTtElems, pFanins[i], nVars );
        assert( i == nVarsF );
        assert( *p == q );
        Dau_DsdTruthCompose_rec( pFunc, pFanins, pRes, nVarsF, nWords );
        if ( fCompl ) Abc_TtNot( pRes, nWords );
        return;
    }
    assert( 0 );
}
word * Dau_DsdToTruth( char * p, int nVars )
{
    int nWords = Abc_TtWordNum( nVars );
    word ** pTtElems = Dau_DsdTtElems();
    word * pRes = pTtElems[DAU_MAX_VAR];
    assert( nVars <= DAU_MAX_VAR );
    if ( Dau_DsdIsConst0(p) )
        Abc_TtConst0( pRes, nWords );
    else if ( Dau_DsdIsConst1(p) )
        Abc_TtConst1( pRes, nWords );
    else
        Dau_DsdToTruth_rec( p, &p, Dau_DsdComputeMatches(p), pTtElems, pRes, nVars );
    assert( *++p == 0 );
    return pRes;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_DsdTest2()
{
//    char * p = Abc_UtilStrsav( "!(ab!(de[cf]))" );
//    char * p = Abc_UtilStrsav( "!(a![d<ecf>]b)" );
//    word t = Dau_Dsd6ToTruth( p );
}



/**Function*************************************************************

  Synopsis    [Performs DSD.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dau_DsdPerformReplace( char * pBuffer, int PosStart, int Pos, int Symb, char * pNext )
{
    static char pTemp[DAU_MAX_STR];
    char * pCur = pTemp;
    int i, k, RetValue;
    for ( i = PosStart; i < Pos; i++ )
        if ( pBuffer[i] != Symb )
            *pCur++ = pBuffer[i];
        else
            for ( k = 0; pNext[k]; k++ )
                *pCur++ = pNext[k];
    RetValue = PosStart + (pCur - pTemp);
    for ( i = PosStart; i < RetValue; i++ )
        pBuffer[i] = pTemp[i-PosStart];
    return RetValue;
}
int Dau_DsdPerform_rec( word t, char * pBuffer, int Pos, int * pVars, int nVars )
{
    char pNest[10];
    word Cof0[6], Cof1[6], Cof[4];
    int pVarsNew[6], nVarsNew, PosStart;
    int v, u, vBest, CountBest;
    assert( Pos < DAU_MAX_STR );
    // perform support minimization
    nVarsNew = 0;
    for ( v = 0; v < nVars; v++ )
        if ( Abc_Tt6HasVar( t, pVars[v] ) )
            pVarsNew[ nVarsNew++ ] = pVars[v];
    assert( nVarsNew > 0 );
    // special case when function is a var
    if ( nVarsNew == 1 )
    {
        if ( t == s_Truths6[ pVarsNew[0] ] )
        {
            pBuffer[Pos++] = 'a' +  pVarsNew[0];
            return Pos;
        }
        if ( t == ~s_Truths6[ pVarsNew[0] ] )
        {
            pBuffer[Pos++] = '!';
            pBuffer[Pos++] = 'a' +  pVarsNew[0];
            return Pos;
        }
        assert( 0 );
        return Pos;
    }
    // decompose on the output side
    for ( v = 0; v < nVarsNew; v++ )
    {
        Cof0[v] = Abc_Tt6Cofactor0( t, pVarsNew[v] );
        Cof1[v] = Abc_Tt6Cofactor1( t, pVarsNew[v] );
        assert( Cof0[v] != Cof1[v] );
        if ( Cof0[v] == 0 ) // ax
        {
            pBuffer[Pos++] = '(';
            pBuffer[Pos++] = 'a' + pVarsNew[v];
            Pos = Dau_DsdPerform_rec( Cof1[v], pBuffer, Pos, pVarsNew, nVarsNew );
            pBuffer[Pos++] = ')';
            return Pos;
        }
        if ( Cof0[v] == ~(word)0 ) // !(ax)
        {
            pBuffer[Pos++] = '!';
            pBuffer[Pos++] = '(';
            pBuffer[Pos++] = 'a' + pVarsNew[v];
            Pos = Dau_DsdPerform_rec( ~Cof1[v], pBuffer, Pos, pVarsNew, nVarsNew );
            pBuffer[Pos++] = ')';
            return Pos;
        }
        if ( Cof1[v] == 0 ) // !ax
        {
            pBuffer[Pos++] = '(';
            pBuffer[Pos++] = '!';
            pBuffer[Pos++] = 'a' + pVarsNew[v];
            Pos = Dau_DsdPerform_rec( Cof0[v], pBuffer, Pos, pVarsNew, nVarsNew );
            pBuffer[Pos++] = ')';
            return Pos;
        }
        if ( Cof1[v] == ~(word)0 ) // !(!ax)
        {
            pBuffer[Pos++] = '!';
            pBuffer[Pos++] = '(';
            pBuffer[Pos++] = '!';
            pBuffer[Pos++] = 'a' + pVarsNew[v];
            Pos = Dau_DsdPerform_rec( ~Cof0[v], pBuffer, Pos, pVarsNew, nVarsNew );
            pBuffer[Pos++] = ')';
            return Pos;
        }
        if ( Cof0[v] == ~Cof1[v] ) // a^x
        {
            pBuffer[Pos++] = '[';
            pBuffer[Pos++] = 'a' + pVarsNew[v];
            Pos = Dau_DsdPerform_rec( Cof0[v], pBuffer, Pos, pVarsNew, nVarsNew );
            pBuffer[Pos++] = ']';
            return Pos;
        }
    }
    // decompose on the input side
    for ( v = 0; v < nVarsNew; v++ )
    for ( u = v+1; u < nVarsNew; u++ )
    {
        Cof[0] = Abc_Tt6Cofactor0( Cof0[v], pVarsNew[u] );
        Cof[1] = Abc_Tt6Cofactor1( Cof0[v], pVarsNew[u] );
        Cof[2] = Abc_Tt6Cofactor0( Cof1[v], pVarsNew[u] );
        Cof[3] = Abc_Tt6Cofactor1( Cof1[v], pVarsNew[u] );
        if ( Cof[0] == Cof[1] && Cof[0] == Cof[2] ) // vu
        {
            PosStart = Pos;
            sprintf( pNest, "(%c%c)", 'a' + pVarsNew[v], 'a' + pVarsNew[u] );
            Pos = Dau_DsdPerform_rec( (s_Truths6[pVarsNew[u]] & Cof[3]) | (~s_Truths6[pVarsNew[u]] & Cof[0]), pBuffer, Pos, pVarsNew, nVarsNew );
            Pos = Dau_DsdPerformReplace( pBuffer, PosStart, Pos, 'a' + pVarsNew[u], pNest );
            return Pos;
        }
        if ( Cof[0] == Cof[1] && Cof[0] == Cof[3] ) // v!u
        {
            PosStart = Pos;
            sprintf( pNest, "(%c!%c)", 'a' + pVarsNew[v], 'a' + pVarsNew[u] );
            Pos = Dau_DsdPerform_rec( (s_Truths6[pVarsNew[u]] & Cof[2]) | (~s_Truths6[pVarsNew[u]] & Cof[0]), pBuffer, Pos, pVarsNew, nVarsNew );
            Pos = Dau_DsdPerformReplace( pBuffer, PosStart, Pos, 'a' + pVarsNew[u], pNest );
            return Pos;
        }
        if ( Cof[0] == Cof[2] && Cof[0] == Cof[3] ) // !vu
        {
            PosStart = Pos;
            sprintf( pNest, "(!%c%c)", 'a' + pVarsNew[v], 'a' + pVarsNew[u] );
            Pos = Dau_DsdPerform_rec( (s_Truths6[pVarsNew[u]] & Cof[1]) | (~s_Truths6[pVarsNew[u]] & Cof[0]), pBuffer, Pos, pVarsNew, nVarsNew );
            Pos = Dau_DsdPerformReplace( pBuffer, PosStart, Pos, 'a' + pVarsNew[u], pNest );
            return Pos;
        }
        if ( Cof[1] == Cof[2] && Cof[1] == Cof[3] ) // !v!u
        {
            PosStart = Pos;
            sprintf( pNest, "(!%c!%c)", 'a' + pVarsNew[v], 'a' + pVarsNew[u] );
            Pos = Dau_DsdPerform_rec( (s_Truths6[pVarsNew[u]] & Cof[0]) | (~s_Truths6[pVarsNew[u]] & Cof[1]), pBuffer, Pos, pVarsNew, nVarsNew );
            Pos = Dau_DsdPerformReplace( pBuffer, PosStart, Pos, 'a' + pVarsNew[u], pNest );
            return Pos;
        }
        if ( Cof[0] == Cof[3] && Cof[1] == Cof[2] ) // v+u
        {
            PosStart = Pos;
            sprintf( pNest, "[%c%c]", 'a' + pVarsNew[v], 'a' + pVarsNew[u] );
            Pos = Dau_DsdPerform_rec( (s_Truths6[pVarsNew[u]] & Cof[1]) | (~s_Truths6[pVarsNew[u]] & Cof[0]), pBuffer, Pos, pVarsNew, nVarsNew );
            Pos = Dau_DsdPerformReplace( pBuffer, PosStart, Pos, 'a' + pVarsNew[u], pNest );
            return Pos;
        }
    } 
    // find best variable for MUX decomposition
    vBest = -1;
    CountBest = 10;
    for ( v = 0; v < nVarsNew; v++ )
    {
        int CountCur = 0;
        for ( u = 0; u < nVarsNew; u++ )
            if ( u != v && Abc_Tt6HasVar(Cof0[v], pVarsNew[u]) && Abc_Tt6HasVar(Cof1[v], pVarsNew[u]) )
                CountCur++;
        if ( CountBest > CountCur )
        {
            CountBest = CountCur;
            vBest = v;
        }
        if ( CountCur == 0 )
            break;
    }
    // perform MUX decomposition
    pBuffer[Pos++] = '<';
    pBuffer[Pos++] = 'a' + pVarsNew[vBest];
    Pos = Dau_DsdPerform_rec( Cof1[vBest], pBuffer, Pos, pVarsNew, nVarsNew );
    Pos = Dau_DsdPerform_rec( Cof0[vBest], pBuffer, Pos, pVarsNew, nVarsNew );
    pBuffer[Pos++] = '>';
    return Pos;
}
char * Dau_DsdPerform( word t )
{
    static char pBuffer[DAU_MAX_STR];
    int pVarsNew[6] = {0, 1, 2, 3, 4, 5};
    int Pos = 0;
    if ( t == 0 )
        pBuffer[Pos++] = '0';
    else if ( t == ~(word)0 )
        pBuffer[Pos++] = '1';
    else
        Pos = Dau_DsdPerform_rec( t, pBuffer, Pos, pVarsNew, 6 );
    pBuffer[Pos++] = 0;
//    printf( "%d ", strlen(pBuffer) );
//    printf( "%s ->", pBuffer );
    Dau_DsdRemoveBraces( pBuffer, Dau_DsdComputeMatches(pBuffer) );
//    printf( " %s\n", pBuffer );
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_DsdTest3()
{
//    word t = s_Truths6[0] & s_Truths6[1] & s_Truths6[2];
//    word t = ~s_Truths6[0] | (s_Truths6[1] ^ ~s_Truths6[2]);
//    word t = (s_Truths6[1] & s_Truths6[2]) | (s_Truths6[0] & s_Truths6[3]);
//    word t = (~s_Truths6[1] & ~s_Truths6[2]) | (s_Truths6[0] ^ s_Truths6[3]);
//    word t = ((~s_Truths6[1] & ~s_Truths6[2]) | (s_Truths6[0] ^ s_Truths6[3])) ^ s_Truths6[5];
//    word t = ((s_Truths6[1] & ~s_Truths6[2]) ^ (s_Truths6[0] & s_Truths6[3])) & s_Truths6[5];
//    word t = (~(~s_Truths6[0] & ~s_Truths6[4]) & s_Truths6[2]) | (~s_Truths6[1] & ~s_Truths6[0] & ~s_Truths6[4]);
//    word t = 0x0000000000005F3F;
//    word t = 0xF3F5030503050305;
//    word t = (s_Truths6[0] & s_Truths6[1] & (s_Truths6[2] ^ s_Truths6[4])) | (~s_Truths6[0] & ~s_Truths6[1] & ~(s_Truths6[2] ^ s_Truths6[4]));
//    word t = 0x05050500f5f5f5f3;
    word t = ABC_CONST(0x9ef7a8d9c7193a0f);
    char * p = Dau_DsdPerform( t );
    word t2 = Dau_Dsd6ToTruth( p );
    if ( t != t2 )
        printf( "Verification failed.\n" );
}




/**Function*************************************************************

  Synopsis    [Find the best cofactoring variable.]

  Description [Return -2 if non-DSD; -1 if full DSD; otherwise,
  returns cofactoring variables i (i >= 0).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_DsdCheck1Step( void * p, word * pTruth, int nVarsInit, int * pVarLevels )
{
    word pCofTemp[DAU_MAX_WORD];
    int pVarPrios[DAU_MAX_VAR];
    int nWords = Abc_TtWordNum(nVarsInit);
    int nSizeNonDec, nSizeNonDec0, nSizeNonDec1;
    int i, vBest = -2, nSumCofsBest = ABC_INFINITY, nSumCofs;
    nSizeNonDec = Dau_DsdDecompose( pTruth, nVarsInit, 0, 0, NULL );
    if ( nSizeNonDec == 0 )
        return -1;
    assert( nSizeNonDec > 0 );
    // find variable priority
    for ( i = 0; i < nVarsInit; i++ )
        pVarPrios[i] = i;
    if ( pVarLevels )
    {
        extern int Dau_DsdLevelVar( void * pMan, int iVar );
        int pVarLevels[DAU_MAX_VAR];
        for ( i = 0; i < nVarsInit; i++ )
            pVarLevels[i] = -Dau_DsdLevelVar( p, i );
//        for ( i = 0; i < nVarsInit; i++ )
//            printf( "%d ", -pVarLevels[i] );
//        printf( "\n" );
        Vec_IntSelectSortCost2( pVarPrios, nVarsInit, pVarLevels );
//        for ( i = 0; i < nVarsInit; i++ )
//            printf( "%d ", pVarPrios[i] );
//        printf( "\n\n" );
    }
    for ( i = 0; i < nVarsInit; i++ )
    {
        assert( pVarPrios[i] >= 0 && pVarPrios[i] < nVarsInit );
        // try first cofactor
        Abc_TtCofactor0p( pCofTemp, pTruth, nWords, pVarPrios[i] );
        nSumCofs = Abc_TtSupportSize( pCofTemp, nVarsInit );
        nSizeNonDec0 = Dau_DsdDecompose( pCofTemp, nVarsInit, 0, 0, NULL );
        // try second cofactor
        Abc_TtCofactor1p( pCofTemp, pTruth, nWords, pVarPrios[i] );
        nSumCofs += Abc_TtSupportSize( pCofTemp, nVarsInit );
        nSizeNonDec1 = Dau_DsdDecompose( pCofTemp, nVarsInit, 0, 0, NULL );
        // compare cofactors
        if ( nSizeNonDec0 || nSizeNonDec1 )
            continue;
        if ( nSumCofsBest > nSumCofs )
        {
            vBest = pVarPrios[i];
            nSumCofsBest = nSumCofs;
        }
    }
    return vBest;
}

/**Function*************************************************************

  Synopsis    [Data-structure to store DSD information.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
typedef struct Dau_Dsd_t_ Dau_Dsd_t;
struct Dau_Dsd_t_
{
    int      nVarsInit;            // the initial number of variables
    int      nVarsUsed;            // the current number of variables
    int      nPos;                 // writing position
    int      nSizeNonDec;          // size of the largest non-decomposable block
    int      nConsts;              // the number of constant decompositions
    int      uConstMask;           // constant decomposition mask
    int      fSplitPrime;          // represent prime function as 1-step DSD
    int      fWriteTruth;          // writing truth table as a hex string
    int *    pVarLevels;           // variable levels
    char     pVarDefs[32][8];      // variable definitions
    char     Cache[32][32];        // variable cache
    char     pOutput[DAU_MAX_STR]; // output stream
};

static abctime s_Times[3] = {0};

/**Function*************************************************************

  Synopsis    [Manipulation of DSD data-structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Dau_DsdInitialize( Dau_Dsd_t * p, int nVarsInit )
{
    int i, v, u;
    assert( nVarsInit >= 0 && nVarsInit <= 16 );
    p->nVarsInit   = nVarsInit;
    p->nVarsUsed   = nVarsInit;
    p->nPos        = 0;
    p->nSizeNonDec = 0;
    p->nConsts     = 0;
    p->uConstMask  = 0;
    for ( i = 0; i < nVarsInit; i++ )
        p->pVarDefs[i][0] = 'a' + i, p->pVarDefs[i][1] = 0;
    for ( v = 0; v < nVarsInit; v++ )
    for ( u = 0; u < nVarsInit; u++ )
        p->Cache[v][u] = 0;

}
static inline void Dau_DsdWriteString( Dau_Dsd_t * p, char * pStr )
{
    while ( *pStr )
        p->pOutput[ p->nPos++ ] = *pStr++;
}
static inline void Dau_DsdWriteVar( Dau_Dsd_t * p, int iVar, int fInv )
{
    char * pStr;
    if ( fInv )
        p->pOutput[ p->nPos++ ] = '!';
    for ( pStr = p->pVarDefs[iVar]; *pStr; pStr++ )
        if ( *pStr >= 'a' + p->nVarsInit && *pStr < 'a' + p->nVarsUsed )
            Dau_DsdWriteVar( p, *pStr - 'a', 0 );
        else
            p->pOutput[ p->nPos++ ] = *pStr;
}
int Dau_DsdLevelVar( void * pMan, int iVar )
{
    Dau_Dsd_t * p = (Dau_Dsd_t *)pMan;
    char * pStr;
    int LevelMax = 0, Level;
    for ( pStr = p->pVarDefs[iVar]; *pStr; pStr++ )
    {
        if ( *pStr >= 'a' + p->nVarsInit && *pStr < 'a' + p->nVarsUsed )
            Level = 1 + Dau_DsdLevelVar( p, *pStr - 'a' );
        else
            Level = p->pVarLevels[*pStr - 'a'];
        LevelMax = Abc_MaxInt( LevelMax, Level );
    }
    return LevelMax;
}
static inline void Dau_DsdTranslate( Dau_Dsd_t * p, int * pVars, int nVars, char * pStr )
{
    for ( ; *pStr; pStr++ )
        if ( *pStr >= 'a' && *pStr < 'a' + nVars )
            Dau_DsdWriteVar( p, pVars[*pStr - 'a'], 0 );
        else
            p->pOutput[ p->nPos++ ] = *pStr;
}
static inline int Dau_DsdWritePrime( Dau_Dsd_t * p, word * pTruth, int * pVars, int nVars )
{
    int v, RetValue = 2;
    assert( nVars > 2 );
    if ( p->fSplitPrime )
    {
        word pCofTemp[DAU_MAX_WORD];
        int nWords = Abc_TtWordNum(nVars);
        int vBest = Dau_DsdCheck1Step( p, pTruth, nVars, p->pVarLevels );
        assert( vBest != -1 );
        if ( vBest == -2 ) // non-dec
            p->nPos += Abc_TtWriteHexRev( p->pOutput + p->nPos, pTruth, nVars );
        else 
        {
            char pRes[DAU_MAX_STR];
            int nNonDecSize;
            // compose the result
            Dau_DsdWriteString( p, "<" );
            Dau_DsdWriteVar( p, vBest, 0 );
            // split decomposition
            Abc_TtCofactor1p( pCofTemp, pTruth, nWords, vBest );
            nNonDecSize = Dau_DsdDecompose( pCofTemp, nVars, 0, p->fWriteTruth, pRes );
            assert( nNonDecSize == 0 );
            Dau_DsdWriteString( p, pRes );
            // split decomposition
            Abc_TtCofactor0p( pCofTemp, pTruth, nWords, vBest );
            nNonDecSize = Dau_DsdDecompose( pCofTemp, nVars, 0, p->fWriteTruth, pRes );
            assert( nNonDecSize == 0 );
            Dau_DsdWriteString( p, pRes );
            Dau_DsdWriteString( p, ">" );
            RetValue = 1;
        }
    }
    else if ( p->fWriteTruth )
        p->nPos += Abc_TtWriteHexRev( p->pOutput + p->nPos, pTruth, nVars );
    Dau_DsdWriteString( p, "{" );
    for ( v = 0; v < nVars; v++ )
        Dau_DsdWriteVar( p, pVars[v], 0 );
    Dau_DsdWriteString( p, "}" );
    p->nSizeNonDec = nVars;
    return RetValue;
}
static inline void Dau_DsdFinalize( Dau_Dsd_t * p )
{
    int i;
    for ( i = 0; i < p->nConsts; i++ )
        p->pOutput[ p->nPos++ ] = ((p->uConstMask >> (p->nConsts-1-i)) & 1) ? ']' : ')';
    p->pOutput[ p->nPos++ ] = 0;
}
static inline int Dau_DsdAddVarDef( Dau_Dsd_t * p, char * pStr )
{
    int u;
    assert( strlen(pStr) < 8 );
    assert( p->nVarsUsed < 32 );
    for ( u = 0; u < p->nVarsUsed; u++ )
        p->Cache[p->nVarsUsed][u] = 0;
    for ( u = 0; u < p->nVarsUsed; u++ )
        p->Cache[u][p->nVarsUsed] = 0;
    sprintf( p->pVarDefs[p->nVarsUsed++], "%s", pStr );
    return p->nVarsUsed - 1;
}
static inline int Dau_DsdFindVarDef( int * pVars, int nVars, int VarDef )
{
    int v;
    for ( v = 0; v < nVars; v++ )
        if ( pVars[v] == VarDef )
            break;
    assert( v < nVars );
    return v;
}
static inline void Dau_DsdInsertVarCache( Dau_Dsd_t * p, int v, int u, int Status )
{
    assert( v != u );
    assert( Status > 0 && Status < 4 );
    assert( p->Cache[v][u] == 0 );
    p->Cache[v][u] = Status;
}
static inline int Dau_DsdLookupVarCache( Dau_Dsd_t * p, int v, int u )
{
    return p->Cache[v][u];
}

/**Function*************************************************************

  Synopsis    [Procedures specialized for 6-variable functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dau_Dsd6DecomposeSingleVarOne( Dau_Dsd_t * p, word * pTruth, int * pVars, int nVars, int v )
{
    // consider negative cofactors
    if ( pTruth[0] & 1 )
    {        
        if ( Abc_Tt6Cof0IsConst1( pTruth[0], v ) ) // !(ax)
        {
            Dau_DsdWriteString( p, "!(" );
            pTruth[0] = ~Abc_Tt6Cofactor1( pTruth[0], v );
            goto finish;            
        }
    }
    else
    {
        if ( Abc_Tt6Cof0IsConst0( pTruth[0], v ) ) // ax
        {
            Dau_DsdWriteString( p, "(" );
            pTruth[0] = Abc_Tt6Cofactor1( pTruth[0], v );
            goto finish;            
        }
    }
    // consider positive cofactors
    if ( pTruth[0] >> 63 )
    {        
        if ( Abc_Tt6Cof1IsConst1( pTruth[0], v ) ) // !(!ax)
        {
            Dau_DsdWriteString( p, "!(!" );
            pTruth[0] = ~Abc_Tt6Cofactor0( pTruth[0], v );
            goto finish;            
        }
    }
    else
    {
        if ( Abc_Tt6Cof1IsConst0( pTruth[0], v ) ) // !ax
        {
            Dau_DsdWriteString( p, "(!" );
            pTruth[0] = Abc_Tt6Cofactor0( pTruth[0], v );
            goto finish;            
        }
    }
    // consider equal cofactors
    if ( Abc_Tt6CofsOpposite( pTruth[0], v ) ) // [ax]
    {
        Dau_DsdWriteString( p, "[" );
        pTruth[0] = Abc_Tt6Cofactor0( pTruth[0], v );
        p->uConstMask |= (1 << p->nConsts);
        goto finish;            
    }
    return 0;

finish:
    p->nConsts++;
    Dau_DsdWriteVar( p, pVars[v], 0 );
    pVars[v] = pVars[nVars-1];
    Abc_TtSwapVars( pTruth, nVars, v, nVars-1 );
    return 1;
}
int Dau_Dsd6DecomposeSingleVar( Dau_Dsd_t * p, word * pTruth, int * pVars, int nVars )
{
    abctime clk = Abc_Clock();
    assert( nVars > 1 );
    while ( 1 )
    {
        int v;
        for ( v = nVars - 1; v >= 0 && nVars > 1; v-- )
            if ( Dau_Dsd6DecomposeSingleVarOne( p, pTruth, pVars, nVars, v ) )
            {
                nVars--;
                break;
            }
        if ( v == -1 || nVars == 1 )
            break;
    }
    if ( nVars == 1 )
        Dau_DsdWriteVar( p, pVars[--nVars], (int)(pTruth[0] & 1) );
    s_Times[0] += Abc_Clock() - clk;
    return nVars;
}
static inline int Dau_Dsd6FindSupportOne( Dau_Dsd_t * p, word tCof0, word tCof1, int * pVars, int nVars, int v, int u )
{
    int Status = p ? Dau_DsdLookupVarCache( p, pVars[v], pVars[u] ) : 0;
    if ( Status == 0 )
    {
        Status = (Abc_Tt6HasVar(tCof1, u) << 1) | Abc_Tt6HasVar(tCof0, u);
        if ( p )
            Dau_DsdInsertVarCache( p, pVars[v], pVars[u], Status );
    }
    return Status;
}
static inline unsigned Dau_Dsd6FindSupports( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars, int v )
{
    int u;
    unsigned uSupports = 0;
    word tCof0 = Abc_Tt6Cofactor0( pTruth[0], v );
    word tCof1 = Abc_Tt6Cofactor1( pTruth[0], v );
//Kit_DsdPrintFromTruth( (unsigned *)&tCof0, 6 );printf( "\n" );
//Kit_DsdPrintFromTruth( (unsigned *)&tCof1, 6 );printf( "\n" );
    for ( u = 0; u < nVars; u++ )
        if ( u != v )
            uSupports |= (Dau_Dsd6FindSupportOne( p, tCof0, tCof1, pVars, nVars, v, u ) << (2 * u));
    return uSupports;
}
static inline void Dau_DsdPrintSupports( unsigned uSupp, int nVars )
{
    int v, Value;
    printf( "Cofactor supports: " );
    for ( v = nVars - 1; v >= 0; v-- )
    {
        Value = ((uSupp >> (2*v)) & 3);
        if ( Value == 1 )
            printf( "01" );
        else if ( Value == 2 )
            printf( "10" );
        else if ( Value == 3 )
            printf( "11" );
        else 
            printf( "00" );
        if ( v )
            printf( "-" );
    }
    printf( "\n" );
}
// checks decomposability with respect to the pair (v, u)
static inline int Dau_Dsd6DecomposeDoubleVarsOne( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars, int v, int u )
{
    char pBuffer[10] = { 0 }; 
    word tCof0 = Abc_Tt6Cofactor0( pTruth[0], v );
    word tCof1 = Abc_Tt6Cofactor1( pTruth[0], v );
    int Status = Dau_Dsd6FindSupportOne( p, tCof0, tCof1, pVars, nVars, v, u );
    assert( v > u );
//printf( "Checking %s and %s.\n", p->pVarDefs[pVars[v]], p->pVarDefs[pVars[u]] );

//    Kit_DsdPrintFromTruth( (unsigned *)pTruth, 6 );printf( "\n" );
    if ( Status == 3 )
    {
        // both F(v=0) and F(v=1) depend on u
        if ( Abc_Tt6Cof0EqualCof1(tCof0, tCof1, u) && Abc_Tt6Cof0EqualCof1(tCof1, tCof0, u) ) // v+u
        {
            pTruth[0] = (s_Truths6[u] & Abc_Tt6Cofactor1(tCof0, u)) | (~s_Truths6[u] & Abc_Tt6Cofactor0(tCof0, u));
            sprintf( pBuffer, "[%c%c]", 'a' + pVars[v], 'a' + pVars[u] );
            goto finish;
        }
    }
    else if ( Status == 2 )
    {
        // F(v=0) does not depend on u; F(v=1) depends on u
        if ( Abc_Tt6Cof0EqualCof0(tCof0, tCof1, u) ) // vu
        {
            sprintf( pBuffer, "(%c%c)", 'a' + pVars[v], 'a' + pVars[u] );
            pTruth[0] = (s_Truths6[u] & Abc_Tt6Cofactor1(tCof1, u)) | (~s_Truths6[u] & Abc_Tt6Cofactor0(tCof0, u));
            goto finish;
        }
        if ( Abc_Tt6Cof0EqualCof1(tCof0, tCof1, u) ) // v!u
        {
            sprintf( pBuffer, "(%c!%c)", 'a' + pVars[v], 'a' + pVars[u] );
            pTruth[0] = (s_Truths6[u] & Abc_Tt6Cofactor0(tCof1, u)) | (~s_Truths6[u] & Abc_Tt6Cofactor0(tCof0, u));
            goto finish;
        }
    }
    else if ( Status == 1 )
    {
        // F(v=0) depends on u; F(v=1) does not depend on u
        if ( Abc_Tt6Cof0EqualCof1(tCof0, tCof1, u) ) // !vu
        {
            sprintf( pBuffer, "(!%c%c)", 'a' + pVars[v], 'a' + pVars[u] );
            pTruth[0] = (s_Truths6[u] & Abc_Tt6Cofactor1(tCof0, u)) | (~s_Truths6[u] & Abc_Tt6Cofactor0(tCof0, u));
            goto finish;
        }
        if ( Abc_Tt6Cof1EqualCof1(tCof0, tCof1, u) ) // !v!u
        {
            sprintf( pBuffer, "(!%c!%c)", 'a' + pVars[v], 'a' + pVars[u] );
            pTruth[0] = (s_Truths6[u] & Abc_Tt6Cofactor0(tCof0, u)) | (~s_Truths6[u] & Abc_Tt6Cofactor1(tCof1, u));
            goto finish;
        }
    }
    return nVars;
finish: 
    // finalize decomposition
    assert( pBuffer[0] );
    pVars[u] = Dau_DsdAddVarDef( p, pBuffer );
    pVars[v] = pVars[nVars-1];
    Abc_TtSwapVars( pTruth, nVars, v, nVars-1 );
    if ( Dau_Dsd6DecomposeSingleVarOne( p, pTruth, pVars, --nVars, u ) )
        nVars = Dau_Dsd6DecomposeSingleVar( p, pTruth, pVars, --nVars );
    return nVars;
}
int Dau_Dsd6DecomposeDoubleVars( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars )
{
    abctime clk = Abc_Clock();
    while ( 1 )
    {
        int v, u, nVarsOld;
        for ( v = nVars - 1; v > 0; v-- )
        {
            for ( u = v - 1; u >= 0; u-- )
            {
                if ( Dau_DsdLookupVarCache( p, pVars[v], pVars[u] ) )
                    continue;
                nVarsOld = nVars;
                nVars = Dau_Dsd6DecomposeDoubleVarsOne( p, pTruth, pVars, nVars, v, u );
                if ( nVars == 0 )
                {
                    s_Times[1] += Abc_Clock() - clk;
                    return 0;
                }
                if ( nVarsOld > nVars )
                    break;
            }
            if ( u >= 0 ) // found
                break;
        }
        if ( v == 0 ) // not found
            break;
    }
    s_Times[1] += Abc_Clock() - clk;
    return nVars;
}

// look for MUX-decomposable variable on top or at the bottom
static inline int Dau_Dsd6DecomposeTripleVarsOuter( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars, int v )
{
    extern int Dau_DsdDecomposeInt( Dau_Dsd_t * p, word * pTruth, int nVarsInit );
    Dau_Dsd_t P1, * p1 = &P1;
    word tCof0, tCof1;
    p1->fSplitPrime = 0;
    p1->fWriteTruth = p->fWriteTruth;
    // move this variable to the top
    ABC_SWAP( int, pVars[v], pVars[nVars-1] );
    Abc_TtSwapVars( pTruth, nVars, v, nVars-1 );
    // cofactor w.r.t the last variable
    tCof0 = Abc_Tt6Cofactor0( pTruth[0], nVars - 1 );
    tCof1 = Abc_Tt6Cofactor1( pTruth[0], nVars - 1 );
    // compose the result
    Dau_DsdWriteString( p, "<" );
    Dau_DsdWriteVar( p, pVars[nVars - 1], 0 );
    // split decomposition
    Dau_DsdDecomposeInt( p1, &tCof1, nVars - 1 );
    Dau_DsdTranslate( p, pVars, nVars - 1, p1->pOutput );
    p->nSizeNonDec = p1->nSizeNonDec;
    if ( p1->nSizeNonDec )
        *pTruth = tCof1;
    // split decomposition
    Dau_DsdDecomposeInt( p1, &tCof0, nVars - 1 );
    Dau_DsdTranslate( p, pVars, nVars - 1, p1->pOutput );
    Dau_DsdWriteString( p, ">" );
    p->nSizeNonDec = Abc_MaxInt( p->nSizeNonDec, p1->nSizeNonDec );
    if ( p1->nSizeNonDec )
        *pTruth = tCof0;
    return 0;
}
static inline int Dau_Dsd6DecomposeTripleVarsInner( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars, int v, unsigned uSupports )
{
    int iVar0 = Abc_TtSuppFindFirst(  uSupports & (~uSupports >> 1) & 0x55555555 ) >> 1;
    int iVar1 = Abc_TtSuppFindFirst( ~uSupports & ( uSupports >> 1) & 0x55555555 ) >> 1;
    word tCof0 = Abc_Tt6Cofactor0( pTruth[0], v );
    word tCof1 = Abc_Tt6Cofactor1( pTruth[0], v );
    word C00 = Abc_Tt6Cofactor0( tCof0, iVar0 );
    word C01 = Abc_Tt6Cofactor1( tCof0, iVar0 );
    word C10 = Abc_Tt6Cofactor0( tCof1, iVar1 );
    word C11 = Abc_Tt6Cofactor1( tCof1, iVar1 );
    int fEqual0 = (C00 == C10) && (C01 == C11);
    int fEqual1 = (C00 == C11) && (C01 == C10);
    if ( fEqual0 || fEqual1 )
    {
        char pBuffer[10];
        int VarId = pVars[iVar0];
        pTruth[0] = (s_Truths6[v] & C11) | (~s_Truths6[v] & C10);
        sprintf( pBuffer, "<%c%c%s%c>", 'a' + pVars[v], 'a' + pVars[iVar1], fEqual1 ? "!":"", 'a' + pVars[iVar0] );
        pVars[v] = Dau_DsdAddVarDef( p, pBuffer );
        // remove iVar1
        ABC_SWAP( int, pVars[iVar1], pVars[nVars-1] );
        Abc_TtSwapVars( pTruth, nVars, iVar1, nVars-1 ); nVars--;
        // remove iVar0
        iVar0 = Dau_DsdFindVarDef( pVars, nVars, VarId );
        ABC_SWAP( int, pVars[iVar0], pVars[nVars-1] );
        Abc_TtSwapVars( pTruth, nVars, iVar0, nVars-1 ); nVars--;
        // find the new var
        v = Dau_DsdFindVarDef( pVars, nVars, p->nVarsUsed-1 );
        // remove single variables if possible
        if ( Dau_Dsd6DecomposeSingleVarOne( p, pTruth, pVars, nVars, v ) )
            nVars = Dau_Dsd6DecomposeSingleVar( p, pTruth, pVars, --nVars );
        return nVars;
    }
    return nVars;
}
int Dau_Dsd6DecomposeTripleVars( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars )
{
    abctime clk = Abc_Clock();
    while ( 1 )
    {
        int v;
//        Kit_DsdPrintFromTruth( (unsigned *)pTruth, 6 ); printf( "\n" );
        for ( v = nVars - 1; v >= 0; v-- )
        {
            unsigned uSupports = Dau_Dsd6FindSupports( p, pTruth, pVars, nVars, v );
//            Dau_DsdPrintSupports( uSupports, nVars );
            if ( (uSupports & (uSupports >> 1) & 0x55555555) == 0 ) // non-overlapping supports
                return Dau_Dsd6DecomposeTripleVarsOuter( p, pTruth, pVars, nVars, v );
            if ( Abc_TtSuppOnlyOne( uSupports & (~uSupports >> 1) & 0x55555555) &&
                 Abc_TtSuppOnlyOne(~uSupports & ( uSupports >> 1) & 0x55555555) ) // one unique variable in each cofactor
            {
                int nVarsNew = Dau_Dsd6DecomposeTripleVarsInner( p, pTruth, pVars, nVars, v, uSupports );
                if ( nVarsNew == nVars )
                    continue;
                if ( nVarsNew == 0 )
                {
                    s_Times[2] += Abc_Clock() - clk;
                    return 0;
                }
                nVars = Dau_Dsd6DecomposeDoubleVars( p, pTruth, pVars, nVarsNew );
                if ( nVars == 0 )
                {
                    s_Times[2] += Abc_Clock() - clk;
                    return 0;
                }
                break;
            }
        }
        if ( v == -1 )
        {
            s_Times[2] += Abc_Clock() - clk;
            return nVars;
        }
    }
    assert( 0 );
    return -1;
}
int Dau_Dsd6DecomposeInternal( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars )
{
    // decompose single variales on the output side
    nVars = Dau_Dsd6DecomposeSingleVar( p, pTruth, pVars, nVars );
    if ( nVars == 0 )
        return 0;
    // decompose double variables on the input side
    nVars = Dau_Dsd6DecomposeDoubleVars( p, pTruth, pVars, nVars );
    if ( nVars == 0 )
        return 0;
    // decompose MUX on the output/input side
    nVars = Dau_Dsd6DecomposeTripleVars( p, pTruth, pVars, nVars );
    if ( nVars == 0 )
        return 0;
    // write non-decomposable function
    return Dau_DsdWritePrime( p, pTruth, pVars, nVars );
}

/**Function*************************************************************

  Synopsis    [Procedures specialized for 6-variable functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dau_DsdDecomposeSingleVarOne( Dau_Dsd_t * p, word * pTruth, int * pVars, int nVars, int v )
{
    int nWords = Abc_TtWordNum(nVars);
    // consider negative cofactors
    if ( pTruth[0] & 1 )
    {        
        if ( Abc_TtCof0IsConst1( pTruth, nWords, v ) ) // !(ax)
        {
            Dau_DsdWriteString( p, "!(" );
            Abc_TtCofactor1( pTruth, nWords, v );
            Abc_TtNot( pTruth, nWords );
            goto finish;            
        }
    }
    else
    {
        if ( Abc_TtCof0IsConst0( pTruth, nWords, v ) ) // ax
        {
            Dau_DsdWriteString( p, "(" );
            Abc_TtCofactor1( pTruth, nWords, v );
            goto finish;            
        }
    }
    // consider positive cofactors
    if ( pTruth[nWords-1] >> 63 )
    {        
        if ( Abc_TtCof1IsConst1( pTruth, nWords, v ) ) // !(!ax)
        {
            Dau_DsdWriteString( p, "!(!" );
            Abc_TtCofactor0( pTruth, nWords, v );
            Abc_TtNot( pTruth, nWords );
            goto finish;            
        }
    }
    else
    {
        if ( Abc_TtCof1IsConst0( pTruth, nWords, v ) ) // !ax
        {
            Dau_DsdWriteString( p, "(!" );
            Abc_TtCofactor0( pTruth, nWords, v );
            goto finish;            
        }
    }
    // consider equal cofactors
    if ( Abc_TtCofsOpposite( pTruth, nWords, v ) ) // [ax]
    {
        Dau_DsdWriteString( p, "[" );
        Abc_TtCofactor0( pTruth, nWords, v );
        p->uConstMask |= (1 << p->nConsts);
        goto finish;            
    }
    return 0;

finish:
    p->nConsts++;
    Dau_DsdWriteVar( p, pVars[v], 0 );
    pVars[v] = pVars[nVars-1];
    Abc_TtSwapVars( pTruth, nVars, v, nVars-1 );
    return 1;
}
int Dau_DsdDecomposeSingleVar( Dau_Dsd_t * p, word * pTruth, int * pVars, int nVars )
{
    abctime clk = Abc_Clock();
    assert( nVars > 1 );
    while ( 1 )
    {
        int v;
        for ( v = nVars - 1; v >= 0 && nVars > 1; v-- )
            if ( Dau_DsdDecomposeSingleVarOne( p, pTruth, pVars, nVars, v ) )
            {
                nVars--;
                break;
            }
        if ( v == -1 || nVars == 1 )
            break;
    }
    if ( nVars == 1 )
        Dau_DsdWriteVar( p, pVars[--nVars], (int)(pTruth[0] & 1) );
    s_Times[0] += Abc_Clock() - clk;
    return nVars;
}

static inline int Dau_DsdFindSupportOne( Dau_Dsd_t * p, word * pTruth, int * pVars, int nVars, int v, int u )
{
    int nWords = Abc_TtWordNum(nVars);
    int Status = p ? Dau_DsdLookupVarCache( p, pVars[v], pVars[u] ) : 0;
    if ( Status == 0 )
    {
//        Status = (Abc_Tt6HasVar(tCof1, u) << 1) | Abc_Tt6HasVar(tCof0, u);
        if ( v < u )
            Status = (!Abc_TtCheckEqualCofs(pTruth, nWords, v, u, 1, 3) << 1) | !Abc_TtCheckEqualCofs(pTruth, nWords, v, u, 0, 2);
        else // if ( v > u )
            Status = (!Abc_TtCheckEqualCofs(pTruth, nWords, u, v, 2, 3) << 1) | !Abc_TtCheckEqualCofs(pTruth, nWords, u, v, 0, 1);
        assert( Status != 0 );
        if ( p )
            Dau_DsdInsertVarCache( p, pVars[v], pVars[u], Status );
    }
    return Status;
}
static inline unsigned Dau_DsdFindSupports( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars, int v )
{
    int u;
    unsigned uSupports = 0;
//Kit_DsdPrintFromTruth( (unsigned *)&tCof0, 6 );printf( "\n" );
//Kit_DsdPrintFromTruth( (unsigned *)&tCof1, 6 );printf( "\n" );
    for ( u = 0; u < nVars; u++ )
        if ( u != v )
            uSupports |= (Dau_DsdFindSupportOne( p, pTruth, pVars, nVars, v, u ) << (2 * u));
    return uSupports;
}

// checks decomposability with respect to the pair (v, u)
static inline int Dau_DsdDecomposeDoubleVarsOne( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars, int v, int u )
{
    char pBuffer[10] = { 0 }; 
    int nWords = Abc_TtWordNum(nVars);
    int Status = Dau_DsdFindSupportOne( p, pTruth, pVars, nVars, v, u );
    assert( v > u );
//printf( "Checking %s and %s.\n", p->pVarDefs[pVars[v]], p->pVarDefs[pVars[u]] );
    if ( Status == 3 )
    {
        // both F(v=0) and F(v=1) depend on u
//        if ( Abc_Tt6Cof0EqualCof1(tCof0, tCof1, u) && Abc_Tt6Cof0EqualCof1(tCof1, tCof0, u) ) // v+u
        if ( Abc_TtCheckEqualCofs(pTruth, nWords, u, v, 0, 3) && Abc_TtCheckEqualCofs(pTruth, nWords, u, v, 1, 2) ) // 00=11 01=10 v+u
        {
            word pTtTemp[2][DAU_MAX_WORD];
            sprintf( pBuffer, "[%c%c]", 'a' + pVars[v], 'a' + pVars[u] );
//            pTruth[0] = (s_Truths6[u] & Abc_Tt6Cofactor1(tCof0, u)) | (~s_Truths6[u] & Abc_Tt6Cofactor0(tCof0, u));
            Abc_TtCofactor0p( pTtTemp[0], pTruth, nWords, v );
            Abc_TtCofactor0( pTtTemp[0], nWords, u );
            Abc_TtCofactor0p( pTtTemp[1], pTruth, nWords, v );
            Abc_TtCofactor1( pTtTemp[1], nWords, u );
            Abc_TtMux( pTruth, Dau_DsdTtElems()[u], pTtTemp[1], pTtTemp[0], nWords );
            goto finish;
        }
    }
    else if ( Status == 2 )
    {
        // F(v=0) does not depend on u; F(v=1) depends on u
//        if ( Abc_Tt6Cof0EqualCof0(tCof0, tCof1, u) ) // vu
        if ( Abc_TtCheckEqualCofs(pTruth, nWords, u, v, 0, 2) ) // 00=10 vu
        {
            word pTtTemp[2][DAU_MAX_WORD];
            sprintf( pBuffer, "(%c%c)", 'a' + pVars[v], 'a' + pVars[u] );
//            pTruth[0] = (s_Truths6[u] & Abc_Tt6Cofactor1(tCof1, u)) | (~s_Truths6[u] & Abc_Tt6Cofactor0(tCof0, u));
            Abc_TtCofactor0p( pTtTemp[0], pTruth, nWords, v );
            Abc_TtCofactor0( pTtTemp[0], nWords, u );
            Abc_TtCofactor1p( pTtTemp[1], pTruth, nWords, v );
            Abc_TtCofactor1( pTtTemp[1], nWords, u );
            Abc_TtMux( pTruth, Dau_DsdTtElems()[u], pTtTemp[1], pTtTemp[0], nWords );
            goto finish;
        }
//        if ( Abc_Tt6Cof0EqualCof1(tCof0, tCof1, u) ) // v!u
        if ( Abc_TtCheckEqualCofs(pTruth, nWords, u, v, 0, 3) ) // 00=11 v!u
        {
            word pTtTemp[2][DAU_MAX_WORD];
            sprintf( pBuffer, "(%c!%c)", 'a' + pVars[v], 'a' + pVars[u] );
//            pTruth[0] = (s_Truths6[u] & Abc_Tt6Cofactor0(tCof1, u)) | (~s_Truths6[u] & Abc_Tt6Cofactor0(tCof0, u));
            Abc_TtCofactor0p( pTtTemp[0], pTruth, nWords, v );
            Abc_TtCofactor0( pTtTemp[0], nWords, u );
            Abc_TtCofactor1p( pTtTemp[1], pTruth, nWords, v );
            Abc_TtCofactor0( pTtTemp[1], nWords, u );
            Abc_TtMux( pTruth, Dau_DsdTtElems()[u], pTtTemp[1], pTtTemp[0], nWords );
            goto finish;
        }
    }
    else if ( Status == 1 )
    {
        // F(v=0) depends on u; F(v=1) does not depend on u
//        if ( Abc_Tt6Cof0EqualCof1(tCof0, tCof1, u) ) // !vu
        if ( Abc_TtCheckEqualCofs(pTruth, nWords, u, v, 0, 3) ) // 00=11 !vu
        {
            word pTtTemp[2][DAU_MAX_WORD];
            sprintf( pBuffer, "(!%c%c)", 'a' + pVars[v], 'a' + pVars[u] );
//            pTruth[0] = (s_Truths6[u] & Abc_Tt6Cofactor1(tCof0, u)) | (~s_Truths6[u] & Abc_Tt6Cofactor0(tCof0, u));
            Abc_TtCofactor0p( pTtTemp[0], pTruth, nWords, v );
            Abc_TtCofactor0( pTtTemp[0], nWords, u );
            Abc_TtCofactor0p( pTtTemp[1], pTruth, nWords, v );
            Abc_TtCofactor1( pTtTemp[1], nWords, u );
            Abc_TtMux( pTruth, Dau_DsdTtElems()[u], pTtTemp[1], pTtTemp[0], nWords );
            goto finish;
        }
//        if ( Abc_Tt6Cof1EqualCof1(tCof0, tCof1, u) ) // !v!u
        if ( Abc_TtCheckEqualCofs(pTruth, nWords, u, v, 1, 3) ) // 01=11 !v!u
        {
            word pTtTemp[2][DAU_MAX_WORD];
            sprintf( pBuffer, "(!%c!%c)", 'a' + pVars[v], 'a' + pVars[u] );
//            pTruth[0] = (s_Truths6[u] & Abc_Tt6Cofactor0(tCof0, u)) | (~s_Truths6[u] & Abc_Tt6Cofactor1(tCof1, u));
            Abc_TtCofactor1p( pTtTemp[0], pTruth, nWords, v );
            Abc_TtCofactor1( pTtTemp[0], nWords, u );
            Abc_TtCofactor0p( pTtTemp[1], pTruth, nWords, v );
            Abc_TtCofactor0( pTtTemp[1], nWords, u );
            Abc_TtMux( pTruth, Dau_DsdTtElems()[u], pTtTemp[1], pTtTemp[0], nWords );
            goto finish;
        }
    }
    return nVars;
finish: 
    // finalize decomposition
    assert( pBuffer[0] );
    pVars[u] = Dau_DsdAddVarDef( p, pBuffer );
    pVars[v] = pVars[nVars-1];
    Abc_TtSwapVars( pTruth, nVars, v, nVars-1 );
    if ( Dau_DsdDecomposeSingleVarOne( p, pTruth, pVars, --nVars, u ) )
        nVars = Dau_DsdDecomposeSingleVar( p, pTruth, pVars, --nVars );
    return nVars;
}
int Dau_DsdDecomposeDoubleVars( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars )
{
    abctime clk = Abc_Clock();
    while ( 1 )
    {
        int v, u, nVarsOld;
        for ( v = nVars - 1; v > 0; v-- )
        {
            for ( u = v - 1; u >= 0; u-- )
            {
                if ( Dau_DsdLookupVarCache( p, pVars[v], pVars[u] ) )
                    continue;
                nVarsOld = nVars;
                nVars = Dau_DsdDecomposeDoubleVarsOne( p, pTruth, pVars, nVars, v, u );
                if ( nVars == 0 )
                {
                    s_Times[1] += Abc_Clock() - clk;
                    return 0;
                }
                if ( nVarsOld > nVars )
                    break;
            }
            if ( u >= 0 ) // found
                break;
        }
        if ( v == 0 ) // not found
            break;
    }
    s_Times[1] += Abc_Clock() - clk;
    return nVars;
}

// look for MUX-decomposable variable on top or at the bottom
static inline int Dau_DsdDecomposeTripleVarsOuter( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars, int v )
{
    extern int Dau_DsdDecomposeInt( Dau_Dsd_t * p, word * pTruth, int nVarsInit );
    Dau_Dsd_t P1, * p1 = &P1;
    word pTtCof[2][DAU_MAX_WORD];
    int nWords = Abc_TtWordNum(nVars);
    p1->fSplitPrime = 0;
    p1->fWriteTruth = p->fWriteTruth;
    // move this variable to the top
    ABC_SWAP( int, pVars[v], pVars[nVars-1] );
    Abc_TtSwapVars( pTruth, nVars, v, nVars-1 );
    // cofactor w.r.t the last variable
//    tCof0 = Abc_Tt6Cofactor0( pTruth[0], nVars - 1 );
//    tCof1 = Abc_Tt6Cofactor1( pTruth[0], nVars - 1 );
    Abc_TtCofactor0p( pTtCof[0], pTruth, nWords, nVars - 1 );
    Abc_TtCofactor1p( pTtCof[1], pTruth, nWords, nVars - 1 );
    // compose the result
    Dau_DsdWriteString( p, "<" );
    Dau_DsdWriteVar( p, pVars[nVars - 1], 0 );
    // split decomposition
    Dau_DsdDecomposeInt( p1, pTtCof[1], nVars - 1 );
    Dau_DsdTranslate( p, pVars, nVars - 1, p1->pOutput );
    p->nSizeNonDec = p1->nSizeNonDec;
    if ( p1->nSizeNonDec )
        Abc_TtCopy( pTruth, pTtCof[1], Abc_TtWordNum(p1->nSizeNonDec), 0 );
    // split decomposition
    Dau_DsdDecomposeInt( p1, pTtCof[0], nVars - 1 );
    Dau_DsdTranslate( p, pVars, nVars - 1, p1->pOutput );
    Dau_DsdWriteString( p, ">" );
    p->nSizeNonDec = Abc_MaxInt( p->nSizeNonDec, p1->nSizeNonDec );
    if ( p1->nSizeNonDec )
        Abc_TtCopy( pTruth, pTtCof[0], Abc_TtWordNum(p1->nSizeNonDec), 0 );
    return 0;
}
static inline int Dau_DsdDecomposeTripleVarsInner( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars, int v, unsigned uSupports )
{
    int nWords = Abc_TtWordNum(nVars);
    int iVar0 = Abc_TtSuppFindFirst(  uSupports & (~uSupports >> 1) & 0x55555555 ) >> 1;
    int iVar1 = Abc_TtSuppFindFirst( ~uSupports & ( uSupports >> 1) & 0x55555555 ) >> 1;
    int fEqual0, fEqual1;
//    word tCof0 = Abc_Tt6Cofactor0( pTruth[0], v );
//    word tCof1 = Abc_Tt6Cofactor1( pTruth[0], v );
//    word C00 = Abc_Tt6Cofactor0( tCof0, iVar0 );
//    word C01 = Abc_Tt6Cofactor1( tCof0, iVar0 );
//    word C10 = Abc_Tt6Cofactor0( tCof1, iVar1 );
//    word C11 = Abc_Tt6Cofactor1( tCof1, iVar1 );
//    int fEqual0 = (C00 == C10) && (C01 == C11);
//    int fEqual1 = (C00 == C11) && (C01 == C10);
    word pTtCof[2][DAU_MAX_WORD];
    word pTtFour[2][2][DAU_MAX_WORD];
    Abc_TtCofactor0p( pTtCof[0], pTruth, nWords, v );
    Abc_TtCofactor1p( pTtCof[1], pTruth, nWords, v );
    Abc_TtCofactor0p( pTtFour[0][0], pTtCof[0], nWords, iVar0 );
    Abc_TtCofactor1p( pTtFour[0][1], pTtCof[0], nWords, iVar0 );
    Abc_TtCofactor0p( pTtFour[1][0], pTtCof[1], nWords, iVar1 );
    Abc_TtCofactor1p( pTtFour[1][1], pTtCof[1], nWords, iVar1 );
    fEqual0 = Abc_TtEqual(pTtFour[0][0], pTtFour[1][0], nWords) && Abc_TtEqual(pTtFour[0][1], pTtFour[1][1], nWords);
    fEqual1 = Abc_TtEqual(pTtFour[0][0], pTtFour[1][1], nWords) && Abc_TtEqual(pTtFour[0][1], pTtFour[1][0], nWords);
    if ( fEqual0 || fEqual1 )
    {
        char pBuffer[10];
        int VarId = pVars[iVar0];
//        pTruth[0] = (s_Truths6[v] & C11) | (~s_Truths6[v] & C10);
        Abc_TtMux( pTruth, Dau_DsdTtElems()[v], pTtFour[1][1], pTtFour[1][0], nWords );
        sprintf( pBuffer, "<%c%c%s%c>", 'a' + pVars[v], 'a' + pVars[iVar1], fEqual1 ? "!":"", 'a' + pVars[iVar0] );
        pVars[v] = Dau_DsdAddVarDef( p, pBuffer );
        // remove iVar1
        ABC_SWAP( int, pVars[iVar1], pVars[nVars-1] );
        Abc_TtSwapVars( pTruth, nVars, iVar1, nVars-1 ); nVars--;
        // remove iVar0
        iVar0 = Dau_DsdFindVarDef( pVars, nVars, VarId );
        ABC_SWAP( int, pVars[iVar0], pVars[nVars-1] );
        Abc_TtSwapVars( pTruth, nVars, iVar0, nVars-1 ); nVars--;
        // find the new var
        v = Dau_DsdFindVarDef( pVars, nVars, p->nVarsUsed-1 );
        // remove single variables if possible
        if ( Dau_DsdDecomposeSingleVarOne( p, pTruth, pVars, nVars, v ) )
            nVars = Dau_DsdDecomposeSingleVar( p, pTruth, pVars, --nVars );
        return nVars;
    }
    return nVars;
}
int Dau_DsdDecomposeTripleVars( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars )
{
    abctime clk = Abc_Clock();
    while ( 1 )
    {
        int v;
//        Kit_DsdPrintFromTruth( (unsigned *)pTruth, 6 ); printf( "\n" );
        for ( v = nVars - 1; v >= 0; v-- )
        {
            unsigned uSupports = Dau_DsdFindSupports( p, pTruth, pVars, nVars, v );
//            Dau_DsdPrintSupports( uSupports, nVars );
            if ( (uSupports & (uSupports >> 1) & 0x55555555) == 0 ) // non-overlapping supports
                return Dau_DsdDecomposeTripleVarsOuter( p, pTruth, pVars, nVars, v );
            if ( Abc_TtSuppOnlyOne( uSupports & (~uSupports >> 1) & 0x55555555) &&
                 Abc_TtSuppOnlyOne(~uSupports & ( uSupports >> 1) & 0x55555555) ) // one unique variable in each cofactor
            {
                int nVarsNew = Dau_DsdDecomposeTripleVarsInner( p, pTruth, pVars, nVars, v, uSupports );
                if ( nVarsNew == nVars )
                    continue;
                if ( nVarsNew == 0 )
                {
                    s_Times[2] += Abc_Clock() - clk;
                    return 0;
                }
                nVars = Dau_DsdDecomposeDoubleVars( p, pTruth, pVars, nVarsNew );
                if ( nVars == 0 )
                {
                    s_Times[2] += Abc_Clock() - clk;
                    return 0;
                }
                break;
            }
        }
        if ( v == -1 )
        {
            s_Times[2] += Abc_Clock() - clk;
            return nVars;
        }
    }
    assert( 0 );
    return -1;
}
int Dau_DsdDecomposeInternal( Dau_Dsd_t * p, word  * pTruth, int * pVars, int nVars )
{
    // decompose single variales on the output side
    nVars = Dau_DsdDecomposeSingleVar( p, pTruth, pVars, nVars );
    if ( nVars == 0 )
        return 0;
    // decompose double variables on the input side
    nVars = Dau_DsdDecomposeDoubleVars( p, pTruth, pVars, nVars );
    if ( nVars == 0 )
        return 0;
    // decompose MUX on the output/input side
    nVars = Dau_DsdDecomposeTripleVars( p, pTruth, pVars, nVars );
    if ( nVars == 0 )
        return 0;
    // write non-decomposable function
    return Dau_DsdWritePrime( p, pTruth, pVars, nVars );
}

/**Function*************************************************************

  Synopsis    [Fast DSD for truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dau_DsdMinBase( word * pTruth, int nVars, int * pVarsNew )
{
    int v;
    for ( v = 0; v < nVars; v++ )
        pVarsNew[v] = v;
    for ( v = nVars - 1; v >= 0; v-- )
    {
        if ( Abc_TtHasVar( pTruth, nVars, v ) )
            continue;
        Abc_TtSwapVars( pTruth, nVars, v, nVars-1 );
        pVarsNew[v] = pVarsNew[--nVars];
    }
    return nVars;
}
int Dau_DsdDecomposeInt( Dau_Dsd_t * p, word * pTruth, int nVarsInit )
{
    int Status = 0, nVars, pVars[16];
    Dau_DsdInitialize( p, nVarsInit );
    nVars = Dau_DsdMinBase( pTruth, nVarsInit, pVars );
    assert( nVars > 0 && nVars <= nVarsInit );
    if ( nVars == 1 )
        Dau_DsdWriteVar( p, pVars[0], (int)(pTruth[0] & 1) );
    else if ( nVars <= 6 )
        Status = Dau_Dsd6DecomposeInternal( p, pTruth, pVars, nVars );
    else
        Status = Dau_DsdDecomposeInternal( p, pTruth, pVars, nVars );
    Dau_DsdFinalize( p );
    return Status;
}
int Dau_DsdDecompose( word * pTruth, int nVarsInit, int fSplitPrime, int fWriteTruth, char * pRes )
{
    Dau_Dsd_t P, * p = &P;
    p->fSplitPrime = fSplitPrime;
    p->fWriteTruth = fWriteTruth;
    p->pVarLevels  = NULL;
    p->nSizeNonDec = 0;
    if ( (pTruth[0] & 1) == 0 && Abc_TtIsConst0(pTruth, Abc_TtWordNum(nVarsInit)) )
        { if ( pRes ) pRes[0] = '0', pRes[1] = 0; }
    else if ( (pTruth[0] & 1) && Abc_TtIsConst1(pTruth, Abc_TtWordNum(nVarsInit)) )
        { if ( pRes ) pRes[0] = '1', pRes[1] = 0; }
    else 
    {
        int Status = Dau_DsdDecomposeInt( p, pTruth, nVarsInit );
        Dau_DsdRemoveBraces( p->pOutput, Dau_DsdComputeMatches(p->pOutput) );
        if ( pRes )
            strcpy( pRes, p->pOutput );
        assert( fSplitPrime || Status != 1 );
        if ( fSplitPrime && Status == 2 )
            return -1;
    }
//    assert( p->nSizeNonDec == 0 );
    return p->nSizeNonDec;
}
int Dau_DsdDecomposeLevel( word * pTruth, int nVarsInit, int fSplitPrime, int fWriteTruth, char * pRes, int * pVarLevels )
{
    Dau_Dsd_t P, * p = &P;
    p->fSplitPrime = fSplitPrime;
    p->fWriteTruth = fWriteTruth;
    p->pVarLevels  = pVarLevels;
    p->nSizeNonDec = 0;
    if ( (pTruth[0] & 1) == 0 && Abc_TtIsConst0(pTruth, Abc_TtWordNum(nVarsInit)) )
        { if ( pRes ) pRes[0] = '0', pRes[1] = 0; }
    else if ( (pTruth[0] & 1) && Abc_TtIsConst1(pTruth, Abc_TtWordNum(nVarsInit)) )
        { if ( pRes ) pRes[0] = '1', pRes[1] = 0; }
    else 
    {
        int Status = Dau_DsdDecomposeInt( p, pTruth, nVarsInit );
        Dau_DsdRemoveBraces( p->pOutput, Dau_DsdComputeMatches(p->pOutput) );
        if ( pRes )
            strcpy( pRes, p->pOutput );
        assert( fSplitPrime || Status != 1 );
        if ( fSplitPrime && Status == 2 )
            return -1;
    }
//    assert( p->nSizeNonDec == 0 );
    return p->nSizeNonDec;
}
void Dau_DsdPrintFromTruthFile( FILE * pFile, word * pTruth, int nVarsInit )
{
    char pRes[DAU_MAX_STR];
    word pTemp[DAU_MAX_WORD];
    Abc_TtCopy( pTemp, pTruth, Abc_TtWordNum(nVarsInit), 0 );
    Dau_DsdDecompose( pTemp, nVarsInit, 0, 1, pRes );
    fprintf( pFile, "%s\n", pRes );
}
void Dau_DsdPrintFromTruth( word * pTruth, int nVarsInit )
{
    char pRes[DAU_MAX_STR];
    word pTemp[DAU_MAX_WORD];
    Abc_TtCopy( pTemp, pTruth, Abc_TtWordNum(nVarsInit), 0 );
    Dau_DsdDecompose( pTemp, nVarsInit, 0, 1, pRes );
    fprintf( stdout, "%s\n", pRes );
}

void Dau_DsdTest44()
{
    char pRes[DAU_MAX_STR];
//    char * pStr = "(!(!a<bcd>)!(!fe))";
//    char * pStr = "([acb]<!edf>)";
//    char * pStr = "!(f!(b!c!(d[ea])))";
//    char * pStr = "[!(a[be])!(c!df)]";
//    char * pStr = "<(e<bca>)fd>";
//    char * pStr = "[d8001{abef}c]";
    char * pStr = "[dc<a(cbd)(!c!b!d)>{abef}]";
//    char * pStr3;
    word t = Dau_Dsd6ToTruth( pStr );
//    return;
    int nNonDec = Dau_DsdDecompose( &t, 6, 1, 1, pRes );
//    Dau_DsdNormalize( pStr2 );
//    Dau_DsdExtract( pStr, 2, 0 );
    t = 0; 
    nNonDec = 0;
}



void Dau_DsdTest888()
{
    char pDsd[DAU_MAX_STR];
    int nVars = 9;
//    char * pStr = "[(abc)(def)(ghi)]";
//    char * pStr = "[a!b!(c!d[e(fg)hi])]";
//    char * pStr = "[(abc)(def)]";
//    char * pStr = "[(abc)(def)]";
//    char * pStr = "[abcdefg]";
//    char * pStr = "[<abc>(de[ghi])]";
    char * pStr = "(<abc>(<def><ghi>))";
    word * pTruth = Dau_DsdToTruth( pStr, 9 );
    int i, Status;
//    Kit_DsdPrintFromTruth( (unsigned *)pTruth, 9 ); printf( "\n" );
/*
    for ( i = 0; i < 6; i++ )
    {
        unsigned uSupp = Dau_Dsd6FindSupports( NULL, pTruth, NULL, 6, i );
        Dau_DsdPrintSupports( uSupp, 6 );
    }
*/
/*
    printf( "\n" );
    for ( i = 0; i < nVars; i++ )
    {
        unsigned uSupp = Dau_DsdFindSupports( NULL, pTruth, NULL, nVars, i );
        Dau_DsdPrintSupports( uSupp, nVars );
    }
*/
    Status = Dau_DsdDecompose( pTruth, nVars, 0, 0, pDsd );
    i = 0;
}

void Dau_DsdTest555()
{
    int nVars = 10;
    int nWords = Abc_TtWordNum(nVars);
    char * pFileName = "_npn/npn/dsd10.txt";
    FILE * pFile = fopen( pFileName, "rb" );
    word Tru[2][DAU_MAX_WORD], * pTruth;
    char pBuffer[DAU_MAX_STR];
    char pRes[DAU_MAX_STR];
    int nSizeNonDec;
    int i, Counter = 0;
    abctime clk = Abc_Clock(), clkDec = 0, clk2;
//    return;

    while ( fgets( pBuffer, DAU_MAX_STR, pFile ) != NULL )
    {
        char * pStr2 = pBuffer + strlen(pBuffer)-1;
        if ( *pStr2 == '\n' )
            *pStr2-- = 0;
        if ( *pStr2 == '\r' )
            *pStr2-- = 0;
        if ( pBuffer[0] == 'V' || pBuffer[0] == 0 )
            continue;
        Counter++; 

        for ( i = 0; i < 1; i++ )
        {
//            Dau_DsdPermute( pBuffer );
            pTruth = Dau_DsdToTruth( pBuffer[0] == '*' ? pBuffer + 1 : pBuffer, nVars );
            Abc_TtCopy( Tru[0], pTruth, nWords, 0 );
            Abc_TtCopy( Tru[1], pTruth, nWords, 0 );
            clk2 = Abc_Clock();
            nSizeNonDec = Dau_DsdDecompose( Tru[1], nVars, 0, 1, pRes );
            clkDec += Abc_Clock() - clk2;
            Dau_DsdNormalize( pRes );
//            pStr2 = Dau_DsdPerform( t ); nSizeNonDec = 0;
            assert( nSizeNonDec == 0 );
            pTruth = Dau_DsdToTruth( pRes, nVars );
            if ( !Abc_TtEqual( pTruth, Tru[0], nWords ) )
            {
    //        Kit_DsdPrintFromTruth( (unsigned *)&t, 6 );
    //        printf( "  " );
    //        Kit_DsdPrintFromTruth( (unsigned *)&t2, 6 );
                printf( "%s -> %s \n", pBuffer, pRes );
                printf( "Verification failed.\n" );
            }
        }
    }
    printf( "Finished trying %d decompositions.  ", Counter );
    Abc_PrintTime( 1, "Time", clkDec );
    Abc_PrintTime( 1, "Total", Abc_Clock() - clk );

    Abc_PrintTime( 1, "Time1", s_Times[0] );
    Abc_PrintTime( 1, "Time2", s_Times[1] );
    Abc_PrintTime( 1, "Time3", s_Times[2] );

    fclose( pFile );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END


