/**CFile****************************************************************

  FileName    [giaResub6.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Resubstitution.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaResub6.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAX_NODE 100

typedef struct Res6_Man_t_ Res6_Man_t;
struct Res6_Man_t_
{
    int             nIns;     // inputs
    int             nDivs;    // divisors
    int             nDivsA;   // divisors alloc
    int             nOuts;    // outputs
    int             nPats;    // patterns
    int             nWords;   // words
    Vec_Wrd_t       vIns;     // input sim data
    Vec_Wrd_t       vOuts;    // input sim data
    word **         ppLits;   // literal sim info
    word **         ppSets;   // set sim info
    Vec_Int_t       vSol;     // current solution
    Vec_Int_t       vSolBest; // best solution
    Vec_Int_t       vTempBest;// current best solution
};

extern void Dau_DsdPrintFromTruth2( word * pTruth, int nVarsInit );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Res6_Man_t * Res6_ManStart( int nIns, int nNodes, int nOuts, int nPats )
{
    Res6_Man_t * p; int i;
    p = ABC_CALLOC( Res6_Man_t, 1 );
    p->nIns   = nIns;
    p->nDivs  = 1 + nIns + nNodes;
    p->nDivsA = p->nDivs + MAX_NODE;
    p->nOuts  = nOuts;
    p->nPats  = nPats;
    p->nWords =(nPats + 63)/64;
    Vec_WrdFill( &p->vIns,  2*p->nDivsA*p->nWords, 0 );
    Vec_WrdFill( &p->vOuts, (1 << nOuts)*p->nWords, 0 );
    p->ppLits = ABC_CALLOC( word *, 2*p->nDivsA );
    p->ppSets = ABC_CALLOC( word *, 1 << nOuts );
    for ( i = 0; i < 2*p->nDivsA; i++ )
        p->ppLits[i] = Vec_WrdEntryP( &p->vIns, i*p->nWords );
    for ( i = 0; i < (1 << nOuts); i++ )
        p->ppSets[i] = Vec_WrdEntryP( &p->vOuts, i*p->nWords );
    Abc_TtFill( p->ppLits[1], p->nWords );
    Vec_IntGrow( &p->vSol,      2*MAX_NODE+nOuts );
    Vec_IntGrow( &p->vSolBest,  2*MAX_NODE+nOuts );
    Vec_IntGrow( &p->vTempBest, 2*MAX_NODE+nOuts );
    return p;
}
static inline void Res6_ManStop( Res6_Man_t * p )
{
    Vec_WrdErase( &p->vIns );
    Vec_WrdErase( &p->vOuts );
    Vec_IntErase( &p->vSol );
    Vec_IntErase( &p->vSolBest );
    Vec_IntErase( &p->vTempBest );
    ABC_FREE( p->ppLits );
    ABC_FREE( p->ppSets );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Res6_Man_t * Res6_ManRead( char * pFileName )
{
    Res6_Man_t * p = NULL;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
        printf( "Cannot open input file \"%s\".\n", pFileName );
    else
    {
        int i, k, nIns, nNodes, nOuts, nPats;
        char Temp[100], Buffer[100];
        char * pLine = fgets( Buffer, 100, pFile );
        if ( pLine == NULL )
        {
            printf( "Cannot read the header line of input file \"%s\".\n", pFileName );
            return NULL;
        }
        if ( 5 != sscanf(pLine, "%s %d %d %d %d", Temp, &nIns, &nNodes, &nOuts, &nPats) )
        {
            printf( "Cannot read the parameters from the header of input file \"%s\".\n", pFileName );
            return NULL;
        }
        p = Res6_ManStart( nIns, nNodes, nOuts, nPats );
        pLine = ABC_ALLOC( char, nPats + 100 );
        for ( i = 1; i < p->nDivs; i++ )
        {
            char * pNext = fgets( pLine, nPats + 100, pFile );
            if ( pNext == NULL )
            {
                printf( "Cannot read line %d of input file \"%s\".\n", i, pFileName );
                Res6_ManStop( p );
                ABC_FREE( pLine );
                fclose( pFile );
                return NULL;
            }
            for ( k = 0; k < p->nPats; k++ )
                if ( pNext[k] == '0' )
                    Abc_TtSetBit( p->ppLits[2*i+1], k );
                else if ( pNext[k] == '1' )
                    Abc_TtSetBit( p->ppLits[2*i], k );
        }
        for ( i = 0; i < (1 << p->nOuts); i++ )
        {
            char * pNext = fgets( pLine, nPats + 100, pFile );
            if ( pNext == NULL )
            {
                printf( "Cannot read line %d of input file \"%s\".\n", i, pFileName );
                Res6_ManStop( p );
                ABC_FREE( pLine );
                fclose( pFile );
                return NULL;
            }
            for ( k = 0; k < p->nPats; k++ )
                if ( pNext[k] == '1' )
                    Abc_TtSetBit( p->ppSets[i], k );
        }
        ABC_FREE( pLine );
        fclose( pFile );
    }
    return p;
}
void Res6_ManWrite( char * pFileName, Res6_Man_t * p )
{
    FILE * pFile = fopen( pFileName, "wb" ); 
    if ( pFile == NULL )
        printf( "Cannot open output file \"%s\".\n", pFileName );
    else
    {
        int i, k;
        fprintf( pFile, "resyn %d %d %d %d\n", p->nIns, p->nDivs - p->nIns - 1, p->nOuts, p->nPats );
        for ( i = 1; i < p->nDivs; i++, fputc('\n', pFile) )
            for ( k = 0; k < p->nPats; k++ )
                if ( Abc_TtGetBit(p->ppLits[2*i+1], k) )
                    fputc( '0', pFile );
                else if ( Abc_TtGetBit(p->ppLits[2*i], k) )
                    fputc( '1', pFile );
                else
                    fputc( '-', pFile );
        for ( i = 0; i < (1 << p->nOuts); i++, fputc('\n', pFile) )
            for ( k = 0; k < p->nPats; k++ )
                fputc( '0' + Abc_TtGetBit(p->ppSets[i], k), pFile );
        fclose( pFile );
    }
}
void Res6_ManPrintProblem( Res6_Man_t * p, int fVerbose )
{
    int i, nInputs = (p->nIns && p->nIns < 6) ? p->nIns : 6;
    printf( "Problem:   In = %d  Div = %d  Out = %d  Pattern = %d\n", p->nIns, p->nDivs - p->nIns - 1, p->nOuts, p->nPats );
    if ( !fVerbose )
        return;
    printf( "%02d : %s\n", 0, "const0" );
    printf( "%02d : %s\n", 1, "const1" );
    for ( i = 1; i < p->nDivs; i++ )
    {
        if ( nInputs < 6 )
        {
            *p->ppLits[2*i+0] = Abc_Tt6Stretch( *p->ppLits[2*i+0], nInputs );
            *p->ppLits[2*i+1] = Abc_Tt6Stretch( *p->ppLits[2*i+1], nInputs );
        }
        printf("%02d : ", 2*i+0), Dau_DsdPrintFromTruth2(p->ppLits[2*i+0], nInputs), printf( "\n" );
        printf("%02d : ", 2*i+1), Dau_DsdPrintFromTruth2(p->ppLits[2*i+1], nInputs), printf( "\n" );
    }
    for ( i = 0; i < (1 << p->nOuts); i++ )
    {
        if ( nInputs < 6 )
            *p->ppSets[i] = Abc_Tt6Stretch( *p->ppSets[i], nInputs );
        printf("%02d : ", i), Dau_DsdPrintFromTruth2(p->ppSets[i], nInputs), printf( "\n" );
    }
}
static inline Vec_Int_t * Res6_ManReadSol( char * pFileName )
{
    Vec_Int_t * vRes = NULL; int Num;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
        printf( "Cannot open input file \"%s\".\n", pFileName );
    else
    {
        while ( fgetc(pFile) != '\n' );
        vRes = Vec_IntAlloc( 10 );
        while ( fscanf(pFile, "%d", &Num) == 1 )
            Vec_IntPush( vRes, Num );
        fclose ( pFile );
    }
    return vRes;
}
static inline void Res6_ManWriteSol( char * pFileName, Vec_Int_t * p )
{
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
        printf( "Cannot open output file \"%s\".\n", pFileName );
    else
    {
        int i, iLit;
        Vec_IntForEachEntry( p, iLit, i )
            fprintf( pFile, "%d ", iLit );
        fclose ( pFile );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Res6_LitSign( int iLit )
{
    return Abc_LitIsCompl(iLit) ? '~' : ' ';
}
static inline int Res6_LitChar( int iLit, int nDivs )
{
    return Abc_Lit2Var(iLit) < nDivs ? (nDivs < 28 ? 'a'+Abc_Lit2Var(iLit)-1 : 'd') : 'x';
}
static inline void Res6_LitPrint( int iLit, int nDivs )
{
    if ( iLit < 2 )
        printf( "%d", iLit );
    else
    {
        printf( "%c%c", Res6_LitSign(iLit), Res6_LitChar(iLit, nDivs) );
        if ( Abc_Lit2Var(iLit) >= nDivs || nDivs >= 28 )
            printf( "%d", Abc_Lit2Var(iLit) );
    }
}
Vec_Int_t * Res6_FindSupport( Vec_Int_t * vSol, int nDivs )
{
    int i, iLit;
    Vec_Int_t * vSupp = Vec_IntAlloc( 10 );
    Vec_IntForEachEntry( vSol, iLit, i )
        if ( iLit >= 2 && iLit < 2*nDivs )
            Vec_IntPushUnique( vSupp, Abc_Lit2Var(iLit) );
    return vSupp;
}
void Res6_PrintSuppSims( Vec_Int_t * vSol, word ** ppLits, int nWords, int nDivs )
{
    Vec_Int_t * vSupp = Res6_FindSupport( vSol, nDivs );
    int i, k, iObj;
    Vec_IntForEachEntry( vSupp, iObj, i )
    {
        for ( k = 0; k < 64*nWords; k++ )
            if ( Abc_TtGetBit(ppLits[2*iObj+1], k) )
                printf( "0" );
            else if ( Abc_TtGetBit(ppLits[2*iObj], k) )
                printf( "1" );
            else
                printf( "-" );
        printf( "\n" );
    }
    for ( k = 0; k < 64*nWords; k++ )
    {
        Vec_IntForEachEntry( vSupp, iObj, i )
            if ( Abc_TtGetBit(ppLits[2*iObj+1], k) )
                printf( "0" );
            else if ( Abc_TtGetBit(ppLits[2*iObj+0], k) )
                printf( "1" );
            else
                printf( "-" );
        printf( "\n" );
        if ( k == 9 )
            break;
    }
    Vec_IntFree( vSupp );
}                        
int Res6_FindSupportSize( Vec_Int_t * vSol, int nDivs )
{
    Vec_Int_t * vSupp = Res6_FindSupport( vSol, nDivs );
    int Res = Vec_IntSize(vSupp);
    Vec_IntFree( vSupp );
    return Res;
}
void Res6_PrintSolution( Vec_Int_t * vSol, int nDivs )
{
    int iNode, nNodes = Vec_IntSize(vSol)/2-1;
    assert( Vec_IntSize(vSol) % 2 == 0 );
    printf( "Solution:  In = %d  Div = %d  Node = %d  Out = %d\n", Res6_FindSupportSize(vSol, nDivs), nDivs-1, nNodes, 1 );
    for ( iNode = 0; iNode <= nNodes; iNode++ )
    {
        int * pLits = Vec_IntEntryP( vSol, 2*iNode );
        printf( "x%-2d = ", nDivs + iNode );
        Res6_LitPrint( pLits[0], nDivs );
        if ( pLits[0] != pLits[1] )
        {
            printf( "  %c ", pLits[0] < pLits[1] ? '&' : '^' );
            Res6_LitPrint( pLits[1], nDivs );
        }
        printf( "\n" );
    }
}
int Res6_FindGetCost( Res6_Man_t * p, int iDiv )
{
    int w, Cost = 0;
    //printf( "DivLit = %d\n", iDiv );
    //Abc_TtPrintBinary1( stdout, p->ppLits[iDiv], p->nIns ); printf( "\n" );
    //printf( "Set0\n" );
    //Abc_TtPrintBinary1( stdout, p->ppSets[0], p->nIns ); printf( "\n" );
    //printf( "Set1\n" );
    //Abc_TtPrintBinary1( stdout, p->ppSets[1], p->nIns ); printf( "\n" );
    for ( w = 0; w < p->nWords; w++ )
        Cost += Abc_TtCountOnes( (p->ppLits[iDiv][w] & p->ppSets[0][w]) | (p->ppLits[iDiv^1][w] & p->ppSets[1][w]) );
    return Cost;
}
int Res6_FindBestDiv( Res6_Man_t * p, int * pCost )
{
    int d, dBest = -1, CostBest = ABC_INFINITY;
    for ( d = 0; d < 2*p->nDivs; d++ )
    {
        int Cost = Res6_FindGetCost( p, d );
        printf( "Div = %d  Cost = %d\n", d, Cost );
        if ( CostBest >= Cost )
            CostBest = Cost, dBest = d;
    }
    if ( pCost )
        *pCost = CostBest;
    return dBest;
}
int Res6_FindBestEval( Res6_Man_t * p, Vec_Int_t * vSol, int Start )
{
    int i, iLit0, iLit1;
    assert( Vec_IntSize(vSol) % 2 == 0 );
    Vec_IntForEachEntryDoubleStart( vSol, iLit0, iLit1, i, 2*Start )
    {
        if ( iLit0 > iLit1 )
        {
            Abc_TtXor( p->ppLits[2*p->nDivs+i+0], p->ppLits[iLit0],   p->ppLits[iLit1],   p->nWords, 0 );
            Abc_TtXor( p->ppLits[2*p->nDivs+i+1], p->ppLits[iLit0],   p->ppLits[iLit1],   p->nWords, 1 );
        }
        else
        {
            Abc_TtAnd( p->ppLits[2*p->nDivs+i+0], p->ppLits[iLit0],   p->ppLits[iLit1],   p->nWords, 0 );
            Abc_TtOr ( p->ppLits[2*p->nDivs+i+1], p->ppLits[iLit0^1], p->ppLits[iLit1^1], p->nWords );
        }

        //printf( "Node %d\n", i/2 );
        //Abc_TtPrintBinary1( stdout, p->ppLits[2*p->nDivs+i+0], 6 ); printf( "\n" );
        //Abc_TtPrintBinary1( stdout, p->ppLits[2*p->nDivs+i+1], 6 ); printf( "\n" );
    }
    return Res6_FindGetCost( p, Vec_IntEntryLast(vSol) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Res6_ManResubVerify( Res6_Man_t * p, Vec_Int_t * vSol )
{
    int Cost = Res6_FindBestEval( p, vSol, 0 );
    if ( Cost == 0 )
        printf( "Verification successful.\n" );
    else
        printf( "Verification FAILED with %d errors on %d patterns.\n", Cost, p->nPats );
}
void Res6_ManResubCheck( char * pFileNameRes, char * pFileNameSol, int fVerbose )
{
    char FileNameSol[1000];
    if ( pFileNameSol )
        strcpy( FileNameSol, pFileNameSol );
    else
    {
        strcpy( FileNameSol, pFileNameRes );
        strcpy( FileNameSol + strlen(FileNameSol) - strlen(".resub"), ".sol" );
    }
    {
        Res6_Man_t * p = Res6_ManRead( pFileNameRes );
        Vec_Int_t * vSol = Res6_ManReadSol( FileNameSol );
        if ( p == NULL || vSol == NULL )
            return;
        if ( fVerbose )
            Res6_ManPrintProblem( p, 0 );
        if ( fVerbose )
            Res6_PrintSolution( vSol, p->nDivs );
        //if ( fVerbose )
        //    Res6_PrintSuppSims( vSol, p->ppLits, p->nWords, p->nDivs );
        Res6_ManResubVerify( p, vSol );
        Vec_IntFree( vSol );
        Res6_ManStop( p );
    }
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

