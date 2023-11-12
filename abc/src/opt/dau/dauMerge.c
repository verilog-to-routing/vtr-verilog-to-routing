/**CFile****************************************************************

  FileName    [dauMerge.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    [Enumeration of decompositions.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dauMerge.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Substitution storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
typedef struct Dau_Sto_t_ Dau_Sto_t;
struct Dau_Sto_t_
{
    int      iVarUsed;                          // counter of used variables
    char     pOutput[2*DAU_MAX_STR+10];         // storage for reduced function
    char *   pPosOutput;                        // place in the output
    char     pStore[DAU_MAX_VAR][DAU_MAX_STR];  // storage for definitions
    char *   pPosStore[DAU_MAX_VAR];            // place in the store
};

static inline void Dau_DsdMergeStoreClean( Dau_Sto_t * pS, int nShared )
{
    int i;
    pS->iVarUsed  = nShared;
    for ( i = 0; i < DAU_MAX_VAR; i++ )
        pS->pStore[i][0] = 0;
}

static inline void Dau_DsdMergeStoreCleanOutput( Dau_Sto_t * pS )
{
    pS->pPosOutput = pS->pOutput;
}
static inline void Dau_DsdMergeStoreAddToOutput( Dau_Sto_t * pS, char * pBeg, char * pEnd )
{
    while ( pBeg < pEnd )
        *pS->pPosOutput++ = *pBeg++;
}
static inline void Dau_DsdMergeStoreAddToOutputChar( Dau_Sto_t * pS, char c )
{
    *pS->pPosOutput++ = c;
}

static inline int Dau_DsdMergeStoreStartDef( Dau_Sto_t * pS, char c )
{
    pS->pPosStore[pS->iVarUsed] = pS->pStore[pS->iVarUsed];
    if (c) *pS->pPosStore[pS->iVarUsed]++ = c;
    return pS->iVarUsed++;
}
static inline void Dau_DsdMergeStoreAddToDef( Dau_Sto_t * pS, int New, char * pBeg, char * pEnd )
{
    while ( pBeg < pEnd )
        *pS->pPosStore[New]++ = *pBeg++;
}
static inline void Dau_DsdMergeStoreAddToDefChar( Dau_Sto_t * pS, int New, char c )
{
    *pS->pPosStore[New]++ = c;
}
static inline void Dau_DsdMergeStoreStopDef( Dau_Sto_t * pS, int New, char c )
{
    if (c) *pS->pPosStore[New]++ = c;
    *pS->pPosStore[New]++ = 0;
}

static inline char Dau_DsdMergeStoreCreateDef( Dau_Sto_t * pS, char * pBeg, char * pEnd )
{
    int New = Dau_DsdMergeStoreStartDef( pS, 0 );
    Dau_DsdMergeStoreAddToDef( pS, New, pBeg, pEnd );
    Dau_DsdMergeStoreStopDef( pS, New, 0 );
    return New;
}
static inline void Dau_DsdMergeStorePrintDefs( Dau_Sto_t * pS )
{
    int i;
    for ( i = 0; i < DAU_MAX_VAR; i++ )
        if ( pS->pStore[i][0] )
            printf( "%c = %s\n", 'a' + i, pS->pStore[i] );
}

/**Function*************************************************************

  Synopsis    [Creates local copy.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Dau_DsdMergeCopy( char * pDsd, int fCompl, char * pRes )
{
    if ( fCompl && pDsd[0] == '!' )
        fCompl = 0, pDsd++;
    if ( Dau_DsdIsConst(pDsd) ) // constant
        pRes[0] = (fCompl ? (char)((int)pDsd[0] ^ 1) : pDsd[0]), pRes[1] = 0;
    else
        sprintf( pRes, "%s%s", fCompl ? "!" : "", pDsd );
}

/**Function*************************************************************

  Synopsis    [Replaces variables according to the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Dau_DsdMergeReplace( char * pDsd, int * pMatches, int * pMap )
{
    int i;
    for ( i = 0; pDsd[i]; i++ )
    {
        // skip non-DSD block
        if ( pDsd[i] == '<' && pDsd[pMatches[i]+1] == '{' )
            i = pMatches[i] + 1;
        if ( (pDsd[i] >= 'A' && pDsd[i] <= 'F') || (pDsd[i] >= '0' && pDsd[i] <= '9') )
            while ( (pDsd[i] >= 'A' && pDsd[i] <= 'F') || (pDsd[i] >= '0' && pDsd[i] <= '9') )
                i++;
        // detect variables
        if ( pDsd[i] >= 'a' && pDsd[i] <= 'z' )
            pDsd[i] = 'a' + pMap[ pDsd[i] - 'a' ];
    }
}
static inline void Dau_DsdMergeMatches( char * pDsd, int * pMatches )
{
    int pNested[DAU_MAX_VAR];
    int i, nNested = 0;
    for ( i = 0; pDsd[i]; i++ )
    {
        pMatches[i] = 0;
        if ( pDsd[i] == '(' || pDsd[i] == '[' || pDsd[i] == '<' || pDsd[i] == '{' )
            pNested[nNested++] = i;
        else if ( pDsd[i] == ')' || pDsd[i] == ']' || pDsd[i] == '>' || pDsd[i] == '}' )
            pMatches[pNested[--nNested]] = i;
        assert( nNested < DAU_MAX_VAR );
    }
    assert( nNested == 0 );
}
static inline void Dau_DsdMergeVarPres( char * pDsd, int * pMatches, int * pPres, int Mask )
{
    int i;
    for ( i = 0; pDsd[i]; i++ )
    {
        // skip non-DSD block
        if ( pDsd[i] == '<' && pDsd[pMatches[i]+1] == '{' )
            i = pMatches[i] + 1;
        if ( (pDsd[i] >= 'A' && pDsd[i] <= 'F') || (pDsd[i] >= '0' && pDsd[i] <= '9') )
            while ( (pDsd[i] >= 'A' && pDsd[i] <= 'F') || (pDsd[i] >= '0' && pDsd[i] <= '9') )
                i++;
        // skip non-variables
        if ( !(pDsd[i] >= 'a' && pDsd[i] <= 'z') )
            continue;
        // record the mask
        assert( pDsd[i]-'a' < DAU_MAX_VAR );
        pPres[pDsd[i]-'a'] |= Mask;
    }
}
static inline int Dau_DsdMergeCountShared( int * pPres, int Mask )
{
    int i, Counter = 0;
    for ( i = 0; i < DAU_MAX_VAR; i++ )
        Counter += (pPres[i] == Mask);
    return Counter;
}
static inline int Dau_DsdMergeFindShared( char * pDsd0, char * pDsd1, int * pMatches0, int * pMatches1, int * pVarPres )
{
    memset( pVarPres, 0, sizeof(int)*DAU_MAX_VAR );
    Dau_DsdMergeVarPres( pDsd0, pMatches0, pVarPres, 1 );
    Dau_DsdMergeVarPres( pDsd1, pMatches1, pVarPres, 2 );
    return Dau_DsdMergeCountShared( pVarPres, 3 );
}
static inline int Dau_DsdMergeCreateMaps( int * pVarPres, int nShared, int * pOld2New, int * pNew2Old )
{
    int i, Counter = 0, Counter2 = nShared;
    for ( i = 0; i < DAU_MAX_VAR; i++ )
    {
        if ( pVarPres[i] == 0 )
            continue;
        if ( pVarPres[i] == 3 )
        {
            pOld2New[i] = Counter;
            pNew2Old[Counter] = i;
            Counter++;
            continue;
        }
        assert( pVarPres[i] == 1 || pVarPres[i] == 2 );
        pOld2New[i] = Counter2;
        pNew2Old[Counter2] = i;
        Counter2++;
    }
    return Counter2;
}
static inline void Dau_DsdMergeInlineDefinitions( char * pDsd, int * pMatches, Dau_Sto_t * pS, char * pRes, int nShared )
{
    int i;
    char * pDef;
    char * pBegin = pRes;
    for ( i = 0; pDsd[i]; i++ )
    {
        // skip non-DSD block
        if ( pDsd[i] == '<' && pDsd[pMatches[i]+1] == '{' )
        {
            assert( pDsd[pMatches[i]] == '>' );
            for ( ; i <= pMatches[i]; i++ )
                *pRes++ = pDsd[i];
        }
        if ( (pDsd[i] >= 'A' && pDsd[i] <= 'F') || (pDsd[i] >= '0' && pDsd[i] <= '9') )
            while ( (pDsd[i] >= 'A' && pDsd[i] <= 'F') || (pDsd[i] >= '0' && pDsd[i] <= '9') )
                *pRes++ = pDsd[i++];
        // detect variables
        if ( !(pDsd[i] >= 'a' && pDsd[i] <= 'z') || (pDsd[i] - 'a' < nShared) )
        {
            *pRes++ = pDsd[i];
            continue;
        }
        // inline definition
        assert( pDsd[i]-'a' < DAU_MAX_STR );
        for ( pDef = pS->pStore[pDsd[i]-'a']; *pDef; pDef++ )
            *pRes++ = *pDef;
    }
    *pRes++ = 0;
    assert( pRes - pBegin < DAU_MAX_STR );
}


/**Function*************************************************************

  Synopsis    [Computes independence status for each opening parenthesis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Dau_DsdMergePrintWithStatus( char * p, int * pStatus )
{
    int i;
    printf( "%s\n", p );
    for ( i = 0; p[i]; i++ )
        if ( !(p[i] == '(' || p[i] == '[' || p[i] == '<' || p[i] == '{' || (p[i] >= 'a' && p[i] <= 'z')) )
            printf( " " );
        else if ( pStatus[i] >= 0 )
            printf( "%d", pStatus[i] );
        else
            printf( "-" );
    printf( "\n" );
}
int Dau_DsdMergeStatus_rec( char * pStr, char ** p, int * pMatches, int nShared, int * pStatus )
{
    // none pure
    // 1 one pure
    // 2 two or more pure
    // 3 all pure
    if ( **p == '!' )
    {
        pStatus[*p - pStr] = -1;
        (*p)++;
    }
    while ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
    {
        pStatus[*p - pStr] = -1;
        (*p)++;
    }
    if ( **p == '<' )
    {
        char * q = pStr + pMatches[ *p - pStr ];
        if ( *(q+1) == '{' )
        {
            char * pTemp = *p;
            *p = q+1;
            for ( ; pTemp < q+1; pTemp++ )
                pStatus[pTemp - pStr] = -1;
        }
    }
    if ( **p >= 'a' && **p <= 'z' ) // var
        return pStatus[*p - pStr] = (**p - 'a' < nShared) ? 0 : 3;
    if ( **p == '(' || **p == '[' || **p == '<' || **p == '{' ) // and/or/xor
    {
        int Status, nPure = 0, nTotal = 0;
        char * pOld = *p;
        char * q = pStr + pMatches[ *p - pStr ];
        assert( *q == **p + 1 + (**p != '(') );
        for ( (*p)++; *p < q; (*p)++ )
        {
            Status = Dau_DsdMergeStatus_rec( pStr, p, pMatches, nShared, pStatus );
            nPure += (Status == 3);
            nTotal++;
        }
        assert( *p == q );
        assert( nTotal > 1 );
        if ( nPure == 0 )
            Status = 0;
        else if ( nPure == 1 )
            Status = 1;
        else if ( nPure < nTotal )
            Status = 2;
        else if ( nPure == nTotal ) 
            Status = 3;
        else assert( 0 );
        return (pStatus[pOld - pStr] = Status);
    }
    assert( 0 );
    return 0;
}
static inline int Dau_DsdMergeStatus( char * pDsd, int * pMatches, int nShared, int * pStatus )
{
    return Dau_DsdMergeStatus_rec( pDsd, &pDsd, pMatches, nShared, pStatus );
}

/**Function*************************************************************

  Synopsis    [Extracts the formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Dau_DsdMergeGetStatus( char * pBeg, char * pStr, int * pMatches, int * pStatus )
{
    if ( *pBeg == '!' )
        pBeg++;
    while ( (*pBeg >= 'A' && *pBeg <= 'F') || (*pBeg >= '0' && *pBeg <= '9') )
        pBeg++;
    if ( *pBeg == '<' )
    {
        char * q = pStr + pMatches[pBeg - pStr];
        if ( *(q+1) == '{' )
            pBeg = q+1;
    }
    return pStatus[pBeg - pStr];
}
void Dau_DsdMergeSubstitute_rec( Dau_Sto_t * pS, char * pStr, char ** p, int * pMatches, int * pStatus, int fWrite )
{
//    assert( **p != '!' );

    if ( **p == '!' )
    {
        if ( fWrite )
            Dau_DsdMergeStoreAddToOutputChar( pS, **p );
        (*p)++;
    }

    while ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
    {
        if ( fWrite )
            Dau_DsdMergeStoreAddToOutputChar( pS, **p );
        (*p)++;
    }
    if ( **p == '<' )
    {
        char * q = pStr + pMatches[ *p - pStr ];
        if ( *(q+1) == '{' )
        {
            char * pTemp = *p;
            *p = q+1;
            if ( fWrite )
                for ( ; pTemp < q+1; pTemp++ )
                    Dau_DsdMergeStoreAddToOutputChar( pS, *pTemp );
        }
    }
    if ( **p >= 'a' && **p <= 'z' ) // var
    {
        if ( fWrite )
            Dau_DsdMergeStoreAddToOutputChar( pS, **p );
        return;
    }
    if ( **p == '(' || **p == '[' || **p == '<' || **p == '{' ) // and/or/xor
    {
        int New, StatusFan, Status = pStatus[*p - pStr];
        char * pBeg, * q = pStr + pMatches[ *p - pStr ];
        assert( *q == **p + 1 + (**p != '(') );
        if ( !fWrite )
        {
             assert( Status == 3 );
             *p = q;
             return;
        }
        assert( Status != 3 );
        if ( Status == 0 ) // none pure
        {
            Dau_DsdMergeStoreAddToOutputChar( pS, **p );
            for ( (*p)++; *p < q; (*p)++ )
            {
                if ( **p == '!' )
                {
                    Dau_DsdMergeStoreAddToOutputChar( pS, '!' );
                    (*p)++;
                }
                Dau_DsdMergeSubstitute_rec( pS, pStr, p, pMatches, pStatus, 1 );
            }
            Dau_DsdMergeStoreAddToOutputChar( pS, **p );
            assert( *p == q );
            return;
        }
        if ( Status == 1 || **p == '<' || **p == '{' ) // 1 pure
        {
            Dau_DsdMergeStoreAddToOutputChar( pS, **p );
            for ( (*p)++; *p < q; (*p)++ )
            {
                if ( **p == '!' )
                {
                    Dau_DsdMergeStoreAddToOutputChar( pS, '!' );
                    (*p)++;
                }
                pBeg = *p;
                StatusFan = Dau_DsdMergeGetStatus( pBeg, pStr, pMatches, pStatus );
                Dau_DsdMergeSubstitute_rec( pS, pStr, p, pMatches, pStatus, StatusFan != 3 );
                if ( StatusFan == 3 )
                {
                    int New = Dau_DsdMergeStoreCreateDef( pS, pBeg, *p+1 );
                    Dau_DsdMergeStoreAddToOutputChar( pS, (char)('a' + New) );
                }
            }
            Dau_DsdMergeStoreAddToOutputChar( pS, **p );
            assert( *p == q );
            return;
        }
        if ( Status == 2 )
        {
            // add more than one defs
            Dau_DsdMergeStoreAddToOutputChar( pS, **p );
            New = Dau_DsdMergeStoreStartDef( pS, **p );
            for ( (*p)++; *p < q; (*p)++ )
            {
                pBeg = *p;
                StatusFan = Dau_DsdMergeGetStatus( pBeg, pStr, pMatches, pStatus );
                if ( **p == '!' )
                {
                    if ( StatusFan != 3 )
                        Dau_DsdMergeStoreAddToOutputChar( pS, '!' );
                    else
                        Dau_DsdMergeStoreAddToDefChar( pS, New, '!' );
                    (*p)++;
                    pBeg++;
                }
                Dau_DsdMergeSubstitute_rec( pS, pStr, p, pMatches, pStatus, StatusFan != 3 );
                if ( StatusFan == 3 )
                    Dau_DsdMergeStoreAddToDef( pS, New, pBeg, *p+1 );
            }
            Dau_DsdMergeStoreStopDef( pS, New, *q );
            Dau_DsdMergeStoreAddToOutputChar( pS, (char)('a' + New) );
            Dau_DsdMergeStoreAddToOutputChar( pS, **p );
            return;
        }
        assert( 0 );
        return;
    }
    assert( 0 );
}
static inline void Dau_DsdMergeSubstitute( Dau_Sto_t * pS, char * pDsd, int * pMatches, int * pStatus )
{
/*
    int fCompl = 0;
    if ( pDsd[0] == '!' )
    {
        Dau_DsdMergeStoreAddToOutputChar( pS, '!' );
        pDsd++;
        fCompl = 1;
    }
*/
    Dau_DsdMergeSubstitute_rec( pS, pDsd, &pDsd, pMatches, pStatus, 1 );
    Dau_DsdMergeStoreAddToOutputChar( pS, 0 );
}

/**Function*************************************************************

  Synopsis    [Removes braces.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_DsdRemoveBraces_rec( char * pStr, char ** p, int * pMatches )
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
        return;
    if ( **p == '(' || **p == '[' || **p == '<' || **p == '{' ) 
    {
        char * q = pStr + pMatches[ *p - pStr ];
        assert( *q == **p + 1 + (**p != '(') );
        for ( (*p)++; *p < q; (*p)++ )
        {
            int fCompl = (**p == '!');
            char * pBeg = fCompl ? *p + 1 : *p;
            Dau_DsdRemoveBraces_rec( pStr, p, pMatches );
            if ( (!fCompl && *pBeg == '(' && *q == ')') || (*pBeg == '[' && *q == ']') )
            {
                assert( **p == ')' || **p == ']' );
                *pBeg = **p = ' ';
            }
        }
        assert( *p == q );
        return;
    }
    assert( 0 );
}
void Dau_DsdRemoveBraces( char * pDsd, int * pMatches )
{
    char * q, * p = pDsd;
    if ( pDsd[1] == 0 )
        return;
    Dau_DsdRemoveBraces_rec( pDsd, &pDsd, pMatches );
    for ( q = p; *p; p++ )
        if ( *p != ' ' )
        {
            if ( *p == '!' && *(q-1) == '!' && p != q )
            {
                q--;
                continue;
            }
            *q++ = *p;
        }
    *q = 0;
}


abctime s_TimeComp[4] = {0};

/**Function*************************************************************

  Synopsis    [Performs merging of two DSD formulas.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Dau_DsdMerge( char * pDsd0i, int * pPerm0, char * pDsd1i, int * pPerm1, int fCompl0, int fCompl1, int nVars )
{
    int fVerbose = 0;
    int fCheck = 0;
    static int Counter = 0;
    static char pRes[2*DAU_MAX_STR+10];
    char pDsd0[DAU_MAX_STR];
    char pDsd1[DAU_MAX_STR];
    int pMatches0[DAU_MAX_STR];
    int pMatches1[DAU_MAX_STR];
    int pVarPres[DAU_MAX_VAR];
    int pOld2New[DAU_MAX_VAR];
    int pNew2Old[DAU_MAX_VAR];
    int pStatus0[DAU_MAX_STR];
    int pStatus1[DAU_MAX_STR];
    int pMatches[DAU_MAX_STR];
    int nVarsShared, nVarsTotal;
    Dau_Sto_t S, * pS = &S;
    word * pTruth, * pt = NULL, * pt0 = NULL, * pt1 = NULL;
    word pParts[3][DAU_MAX_WORD];
    int Status;
    abctime clk = Abc_Clock();
    Counter++;
    // create local copies
    Dau_DsdMergeCopy( pDsd0i, fCompl0, pDsd0 );
    Dau_DsdMergeCopy( pDsd1i, fCompl1, pDsd1 );
if ( fVerbose )
printf( "\nAfter copying:\n" );
if ( fVerbose )
printf( "%s\n", pDsd0 );
if ( fVerbose )
printf( "%s\n", pDsd1 );
    // handle constants
    if ( Dau_DsdIsConst(pDsd0) || Dau_DsdIsConst(pDsd1) )
    {
        if ( Dau_DsdIsConst0(pDsd0) )
            strcpy( pRes, pDsd0 );
        else if ( Dau_DsdIsConst1(pDsd0) )
            strcpy( pRes, pDsd1 );
        else if ( Dau_DsdIsConst0(pDsd1) )
            strcpy( pRes, pDsd1 );
        else if ( Dau_DsdIsConst1(pDsd1) )
            strcpy( pRes, pDsd0 );
        else assert( 0 );
        return pRes;
    }

    // compute matches
    Dau_DsdMergeMatches( pDsd0, pMatches0 );
    Dau_DsdMergeMatches( pDsd1, pMatches1 );
    // implement permutation
    Dau_DsdMergeReplace( pDsd0, pMatches0, pPerm0 );
    Dau_DsdMergeReplace( pDsd1, pMatches1, pPerm1 );
if ( fVerbose )
printf( "After replacement:\n" );
if ( fVerbose )
printf( "%s\n", pDsd0 );
if ( fVerbose )
printf( "%s\n", pDsd1 );


    if ( fCheck )
    {
        pt0 = Dau_DsdToTruth( pDsd0, nVars );
        Abc_TtCopy( pParts[0], pt0, Abc_TtWordNum(nVars), 0 );
    }
    if ( fCheck )
    {
        pt1 = Dau_DsdToTruth( pDsd1, nVars );
        Abc_TtCopy( pParts[1], pt1, Abc_TtWordNum(nVars), 0 );
        Abc_TtAnd( pParts[2], pParts[0], pParts[1], Abc_TtWordNum(nVars), 0 );
    }

    // find shared varaiables
    nVarsShared = Dau_DsdMergeFindShared(pDsd0, pDsd1, pMatches0, pMatches1, pVarPres);
    if ( nVarsShared == 0 )
    {
        sprintf( pRes, "(%s%s)", pDsd0, pDsd1 );
if ( fVerbose )
printf( "Disjoint:\n" );
if ( fVerbose )
printf( "%s\n", pRes );

        Dau_DsdMergeMatches( pRes, pMatches );
        Dau_DsdRemoveBraces( pRes, pMatches );
        Dau_DsdNormalize( pRes );
if ( fVerbose )
printf( "Normalized:\n" );
if ( fVerbose )
printf( "%s\n", pRes );

        s_TimeComp[0] += Abc_Clock() - clk;
        return pRes;
    }
s_TimeComp[3] += Abc_Clock() - clk;
    // create variable mapping
    nVarsTotal = Dau_DsdMergeCreateMaps( pVarPres, nVarsShared, pOld2New, pNew2Old );
    // perform variable replacement
    Dau_DsdMergeReplace( pDsd0, pMatches0, pOld2New );
    Dau_DsdMergeReplace( pDsd1, pMatches1, pOld2New );
    // find uniqueness status
    Dau_DsdMergeStatus( pDsd0, pMatches0, nVarsShared, pStatus0 );
    Dau_DsdMergeStatus( pDsd1, pMatches1, nVarsShared, pStatus1 );
if ( fVerbose )
printf( "Individual status:\n" );
if ( fVerbose )
Dau_DsdMergePrintWithStatus( pDsd0, pStatus0 );
if ( fVerbose )
Dau_DsdMergePrintWithStatus( pDsd1, pStatus1 );
    // prepare storage
    Dau_DsdMergeStoreClean( pS, nVarsShared );
    // perform substitutions
    Dau_DsdMergeStoreCleanOutput( pS );
    Dau_DsdMergeSubstitute( pS, pDsd0, pMatches0, pStatus0 );
    strcpy( pDsd0, pS->pOutput );
if ( fVerbose )
printf( "Substitutions:\n" );
if ( fVerbose )
printf( "%s\n", pDsd0 );

    // perform substitutions
    Dau_DsdMergeStoreCleanOutput( pS );
    Dau_DsdMergeSubstitute( pS, pDsd1, pMatches1, pStatus1 );
    strcpy( pDsd1, pS->pOutput );
if ( fVerbose )
printf( "%s\n", pDsd1 );
if ( fVerbose )
Dau_DsdMergeStorePrintDefs( pS );

    // create new function
//    assert( nVarsTotal <= 6 );
    sprintf( pS->pOutput, "(%s%s)", pDsd0, pDsd1 );
    pTruth = Dau_DsdToTruth( pS->pOutput, nVarsTotal );
    Status = Dau_DsdDecompose( pTruth, nVarsTotal, 0, 1, pS->pOutput );
//printf( "%d ", Status );
    if ( Status == -1 ) // did not find 1-step DSD
        return NULL;
//    if ( Status > 6 ) // non-DSD part is too large
//        return NULL;
    if ( Dau_DsdIsConst(pS->pOutput) )
    {
        strcpy( pRes, pS->pOutput );
        return pRes;
    }
if ( fVerbose )
printf( "Decomposition:\n" );
if ( fVerbose )
printf( "%s\n", pS->pOutput );
    // substitute definitions
    Dau_DsdMergeMatches( pS->pOutput, pMatches );
    Dau_DsdMergeInlineDefinitions( pS->pOutput, pMatches, pS, pRes, nVarsShared );
if ( fVerbose )
printf( "Inlining:\n" );
if ( fVerbose )
printf( "%s\n", pRes );
    // perform variable replacement
    Dau_DsdMergeMatches( pRes, pMatches );
    Dau_DsdMergeReplace( pRes, pMatches, pNew2Old );
    Dau_DsdRemoveBraces( pRes, pMatches );
if ( fVerbose )
printf( "Replaced:\n" );
if ( fVerbose )
printf( "%s\n", pRes );
    Dau_DsdNormalize( pRes );
if ( fVerbose )
printf( "Normalized:\n" );
if ( fVerbose )
printf( "%s\n", pRes );

    if ( fCheck )
    {
        pt = Dau_DsdToTruth( pRes, nVars );
        if ( !Abc_TtEqual( pParts[2], pt, Abc_TtWordNum(nVars) ) )
            printf( "Dau_DsdMerge(): Verification failed!\n" );
    }

    if ( Status == 0 )
        s_TimeComp[1] += Abc_Clock() - clk;
    else
        s_TimeComp[2] += Abc_Clock() - clk;
    return pRes;
}


void Dau_DsdTest66()
{
    int Perm0[DAU_MAX_VAR] = { 0, 1, 2, 3, 4, 5 };
//    int pMatches[DAU_MAX_STR];
//    int pStatus[DAU_MAX_STR];

//    char * pStr = "(!(!a<bcd>)!(!fe))";
//    char * pStr = "([acb]<!edf>)";
//    char * pStr = "!(f!(b!c!(d[ea])))";
    char * pStr = "[!(a[be])!(c!df)]";
//    char * pStr = "<(e<bca>)fd>";
//    char * pStr = "[d8001{abef}c]";
//    char * pStr1 = "(abc)";
//    char * pStr2 = "[adf]";
//    char * pStr1 = "(!abce)";
//    char * pStr2 = "[adf!b]";
//    char * pStr1 = "(!abc)";
//    char * pStr2 = "[ab]";
//    char * pStr1 = "[d81{abe}c]";
//    char * pStr1 = "[d<a(bc)(!b!c)>{abe}c]";
//    char * pStr1 = "[d81{abe}c]";
//    char * pStr1 = "[d(a[be])c]";
//    char * pStr2 = "(df)";
//    char * pStr1 = "(abf)";
//    char * pStr2 = "(a[(bc)(fde)])";
//    char * pStr1 = "8001{abc[ef]}";
//    char * pStr2 = "(abe)";

    char * pStr1 = "(!(ab)de)";
    char * pStr2 = "(!(ac)f)";

    char * pRes;
    word t = Dau_Dsd6ToTruth( pStr );
    return;

//    pStr2 = Dau_DsdDecompose( &t, 6, 0, NULL );
//    Dau_DsdNormalize( pStr2 );

//    Dau_DsdMergeMatches( pStr, pMatches );
//    Dau_DsdMergeStatus( pStr, pMatches, 2, pStatus );
//    Dau_DsdMergePrintWithStatus( pStr, pStatus );

    pRes = Dau_DsdMerge( pStr1, Perm0, pStr2, Perm0, 0, 0, 6 );

    t = 0; 
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

