/**CFile****************************************************************

  FileName    [bmcMaj.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Exact synthesis with majority gates.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 1, 2017.]

  Revision    [$Id: bmcMaj.c,v 1.00 2017/10/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "misc/extra/extra.h"
#include "misc/util/utilTruth.h"
#include "sat/glucose/AbcGlucose.h"
#include "aig/miniaig/miniaig.h"

ABC_NAMESPACE_IMPL_START


#ifdef WIN32
#include <process.h> 
#define unlink _unlink
#else
#include <unistd.h>
#endif

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAJ_NOBJS  64 // Const0 + Const1 + nVars + nNodes

typedef struct Maj_Man_t_ Maj_Man_t;
struct Maj_Man_t_ 
{
    int               nVars;     // inputs
    int               nNodes;    // internal nodes
    int               nObjs;     // total objects (2 consts, nVars inputs, nNodes internal nodes)
    int               nWords;    // the truth table size in 64-bit words
    int               iVar;      // the next available SAT variable
    int               fUseConst; // use constant fanins
    int               fUseLine;  // use cascade topology
    Vec_Wrd_t *       vInfo;     // Const0 + Const1 + nVars + nNodes + Maj(nVars)
    int               VarMarks[MAJ_NOBJS][3][MAJ_NOBJS]; // variable marks
    int               VarVals[MAJ_NOBJS+2]; // values of the first 2 + nVars variables
    Vec_Wec_t *       vOutLits;  // output vars
    bmcg_sat_solver * pSat;      // SAT solver
};

static inline word *  Maj_ManTruth( Maj_Man_t * p, int v ) { return Vec_WrdEntryP( p->vInfo, p->nWords * v ); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Maj_ManValue( int iMint, int nVars )
{
    int k, Count = 0;
    for ( k = 0; k < nVars; k++ )
        Count += (iMint >> k) & 1;
    return (int)(Count > nVars/2);
}
Vec_Wrd_t * Maj_ManTruthTables( Maj_Man_t * p )
{
    Vec_Wrd_t * vInfo = p->vInfo = Vec_WrdStart( p->nWords * (p->nObjs + 1) ); 
    int i, nMints = Abc_MaxInt( 64, 1 << p->nVars );
    Abc_TtFill( Maj_ManTruth(p, 1), p->nWords );
    for ( i = 0; i < p->nVars; i++ )
        Abc_TtIthVar( Maj_ManTruth(p, i+2), i, p->nVars );
    for ( i = 0; i < nMints; i++ )
        if ( Maj_ManValue(i, p->nVars) )
            Abc_TtSetBit( Maj_ManTruth(p, p->nObjs), i );
    //Dau_DsdPrintFromTruth( Maj_ManTruth(p, p->nObjs), p->nVars );
    return vInfo;
}
int Maj_ManMarkup( Maj_Man_t * p )
{
    int i, k, j;
    p->iVar = 1;
    assert( p->nObjs <= MAJ_NOBJS );
    // make exception for the first node
    i = p->nVars + 2;
    for ( k = 0; k < 3; k++ )
    {
        j = 4-k;
        Vec_WecPush( p->vOutLits, j, Abc_Var2Lit(p->iVar, 0) );
        p->VarMarks[i][k][j] = p->iVar++;
    }
    // assign variables for other nodes
    for ( i = p->nVars + 3; i < p->nObjs; i++ )
    {
        for ( k = 0; k < 3; k++ )
        {
            if ( p->fUseLine && k == 0 )
            {
                j = i-1;
                Vec_WecPush( p->vOutLits, j, Abc_Var2Lit(p->iVar, 0) );
                p->VarMarks[i][k][j] = p->iVar++;
                continue;
            }
            for ( j = (p->fUseConst && k == 2) ? 0 : 2; j < i - k; j++ )
            {
                Vec_WecPush( p->vOutLits, j, Abc_Var2Lit(p->iVar, 0) );
                p->VarMarks[i][k][j] = p->iVar++;
            }
        }
    }
    printf( "The number of parameter variables = %d.\n", p->iVar );
    return p->iVar;
    // printout
    for ( i = p->nVars + 2; i < p->nObjs; i++ )
    {
        printf( "Node %d\n", i );
        for ( j = 0; j < p->nObjs; j++ )
        {
            for ( k = 0; k < 3; k++ )
                printf( "%3d ", p->VarMarks[i][k][j] );
            printf( "\n" );
        }
    }
    return p->iVar;
}
Maj_Man_t * Maj_ManAlloc( int nVars, int nNodes, int fUseConst, int fUseLine )
{
    Maj_Man_t * p = ABC_CALLOC( Maj_Man_t, 1 );
    p->nVars      = nVars;
    p->nNodes     = nNodes;
    p->nObjs      = 2 + nVars + nNodes;
    p->fUseConst  = fUseConst;
    p->fUseLine   = fUseLine;
    p->nWords     = Abc_TtWordNum(nVars);
    p->vOutLits   = Vec_WecStart( p->nObjs );
    p->iVar       = Maj_ManMarkup( p );
    p->VarVals[1] = 1;
    p->vInfo      = Maj_ManTruthTables( p );
    p->pSat       = bmcg_sat_solver_start();
    bmcg_sat_solver_set_nvars( p->pSat, p->iVar );
    return p;
}
void Maj_ManFree( Maj_Man_t * p )
{
    bmcg_sat_solver_stop( p->pSat );
    Vec_WrdFree( p->vInfo );
    Vec_WecFree( p->vOutLits );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Maj_ManFindFanin( Maj_Man_t * p, int i, int k )
{
    int j, Count = 0, iVar = -1;
    for ( j = 0; j < p->nObjs; j++ )
        if ( p->VarMarks[i][k][j] && bmcg_sat_solver_read_cex_varvalue(p->pSat, p->VarMarks[i][k][j]) )
        {
            iVar = j;
            Count++;
        }
    assert( Count == 1 );
    return iVar;
}
static inline int Maj_ManEval( Maj_Man_t * p )
{
    int fUseMiddle = 1;
    static int Flag = 0;
    int i, k, iMint; word * pFanins[3];
    for ( i = p->nVars + 2; i < p->nObjs; i++ )
    {
        for ( k = 0; k < 3; k++ )
            pFanins[k] = Maj_ManTruth( p, Maj_ManFindFanin(p, i, k) );
        Abc_TtMaj( Maj_ManTruth(p, i), pFanins[0], pFanins[1], pFanins[2], p->nWords );
    }
    if ( fUseMiddle )
    {
        iMint = -1;
        for ( i = 0; i < (1 << p->nVars); i++ )
        {
            int nOnes = Abc_TtBitCount16(i);
            if ( nOnes < p->nVars/2 || nOnes > p->nVars/2+1 )
                continue;
            if ( Abc_TtGetBit(Maj_ManTruth(p, p->nObjs), i) == Abc_TtGetBit(Maj_ManTruth(p, p->nObjs-1), i) )
                continue;
            iMint = i;
            break;
        }
    }
    else
    {
        if ( Flag && p->nVars >= 6 )
            iMint = Abc_TtFindLastDiffBit( Maj_ManTruth(p, p->nObjs-1), Maj_ManTruth(p, p->nObjs), p->nVars );
        else
            iMint = Abc_TtFindFirstDiffBit( Maj_ManTruth(p, p->nObjs-1), Maj_ManTruth(p, p->nObjs), p->nVars );
    }
    //Flag ^= 1;
    assert( iMint < (1 << p->nVars) );
    return iMint;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Maj_ManPrintSolution( Maj_Man_t * p )
{
    int i, k, iVar;
    printf( "Realization of %d-input majority using %d MAJ3 gates:\n", p->nVars, p->nNodes );
//    for ( i = p->nVars + 2; i < p->nObjs; i++ )
    for ( i = p->nObjs - 1; i >= p->nVars + 2; i-- )
    {
        printf( "%02d = MAJ(", i-2 );
        for ( k = 2; k >= 0; k-- )
        {
            iVar = Maj_ManFindFanin( p, i, k );
            if ( iVar >= 2 && iVar < p->nVars + 2 )
                printf( " %c", 'a'+iVar-2 );
            else if ( iVar < 2 )
                printf( " %d", iVar );
            else
                printf( " %02d", iVar-2 );
        }
        printf( " )\n" );
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Maj_ManAddCnfStart( Maj_Man_t * p )
{
    int pLits[MAJ_NOBJS], pLits2[2], i, j, k, n, m;
    // input constraints
    for ( i = p->nVars + 2; i < p->nObjs; i++ )
    {
        for ( k = 0; k < 3; k++ )
        {
            int nLits = 0;
            for ( j = 0; j < p->nObjs; j++ )
                if ( p->VarMarks[i][k][j] )
                    pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][k][j], 0 );
            assert( nLits > 0 );
            // input uniqueness
            if ( !bmcg_sat_solver_addclause( p->pSat, pLits, nLits ) )
                return 0;
            for ( n = 0;   n < nLits; n++ )
            for ( m = n+1; m < nLits; m++ )
            {
                pLits2[0] = Abc_LitNot(pLits[n]);
                pLits2[1] = Abc_LitNot(pLits[m]);
                if ( !bmcg_sat_solver_addclause( p->pSat, pLits2, 2 ) )
                    return 0;
            }
            if ( k == 2 )
                break;
            // symmetry breaking
            for ( j = 0; j < p->nObjs; j++ ) if ( p->VarMarks[i][k][j] )
            for ( n = j; n < p->nObjs; n++ ) if ( p->VarMarks[i][k+1][n] )
            {
                pLits2[0] = Abc_Var2Lit( p->VarMarks[i][k][j], 1 );
                pLits2[1] = Abc_Var2Lit( p->VarMarks[i][k+1][n], 1 );
                if ( !bmcg_sat_solver_addclause( p->pSat, pLits2, 2 ) )
                    return 0;
            }
        }
    }
    // outputs should be used
    for ( i = 2; i < p->nObjs - 1; i++ )
    {
        Vec_Int_t * vArray = Vec_WecEntry(p->vOutLits, i);
        assert( Vec_IntSize(vArray) > 0 );
        if ( !bmcg_sat_solver_addclause( p->pSat, Vec_IntArray(vArray), Vec_IntSize(vArray) ) )
            return 0;
    }
    return 1;
}
int Maj_ManAddCnf( Maj_Man_t * p, int iMint )
{
    // save minterm values
    int i, k, n, j, Value = Maj_ManValue(iMint, p->nVars);
    for ( i = 0; i < p->nVars; i++ )
        p->VarVals[i+2] = (iMint >> i) & 1;
    bmcg_sat_solver_set_nvars( p->pSat, p->iVar + 4*p->nNodes );
    //printf( "Adding clauses for minterm %d.\n", iMint );
    for ( i = p->nVars + 2; i < p->nObjs; i++ )
    {
        // fanin connectivity
        int iBaseSatVarI = p->iVar + 4*(i - p->nVars - 2);
        for ( k = 0; k < 3; k++ )
        {
            for ( j = 0; j < p->nObjs; j++ ) if ( p->VarMarks[i][k][j] )
            {
                int iBaseSatVarJ = p->iVar + 4*(j - p->nVars - 2);
                for ( n = 0; n < 2; n++ )
                {
                    int pLits[3], nLits = 0;
                    pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][k][j], 1 );
                    pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + k, n );
                    if ( j >= p->nVars + 2 )
                        pLits[nLits++] = Abc_Var2Lit( iBaseSatVarJ + 3, !n );
                    else if ( p->VarVals[j] == n )
                        continue;
                    if ( !bmcg_sat_solver_addclause( p->pSat, pLits, nLits ) )
                        return 0;
                }
            }
        }
        // node functionality
        for ( n = 0; n < 2; n++ )
        {
            if ( i == p->nObjs - 1 && n == Value )
                continue;
            for ( k = 0; k < 3; k++ )
            {
                int pLits[3], nLits = 0;
                if ( k != 0 ) pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + 0, n );
                if ( k != 1 ) pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + 1, n );
                if ( k != 2 ) pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + 2, n );
                if ( i != p->nObjs - 1 ) pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + 3, !n );
                assert( nLits <= 3 );
                if ( !bmcg_sat_solver_addclause( p->pSat, pLits, nLits ) )
                    return 0;
            }
        }
    }
    p->iVar += 4*p->nNodes;
    return 1;
}
int Maj_ManExactSynthesis( int nVars, int nNodes, int fUseConst, int fUseLine, int fVerbose )
{
    int i, iMint = 0;
    abctime clkTotal = Abc_Clock();
    Maj_Man_t * p = Maj_ManAlloc( nVars, nNodes, fUseConst, fUseLine );
    int status = Maj_ManAddCnfStart( p );
    assert( status );
    printf( "Running exact synthesis for %d-input majority with %d MAJ3 gates...\n", p->nVars, p->nNodes );
    for ( i = 0; iMint != -1; i++ )
    {
        abctime clk = Abc_Clock();
        if ( !Maj_ManAddCnf( p, iMint ) )
            break;
        status = bmcg_sat_solver_solve( p->pSat, NULL, 0 );
        if ( fVerbose )
        {
            printf( "Iter %3d : ", i );
            Extra_PrintBinary( stdout, (unsigned *)&iMint, p->nVars );
            printf( "  Var =%5d  ", p->iVar );
            printf( "Cla =%6d  ", bmcg_sat_solver_clausenum(p->pSat) );
            printf( "Conf =%9d  ", bmcg_sat_solver_conflictnum(p->pSat) );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        }
        if ( status == GLUCOSE_UNSAT )
        {
            printf( "The problem has no solution.\n" );
            break;
        }
        iMint = Maj_ManEval( p );
    }
    if ( iMint == -1 )
        Maj_ManPrintSolution( p );
    Maj_ManFree( p );
    Abc_PrintTime( 1, "Total runtime", Abc_Clock() - clkTotal );
    return iMint == -1;
}








typedef struct Exa_Man_t_ Exa_Man_t;
struct Exa_Man_t_ 
{
    Bmc_EsPar_t *     pPars;     // parameters
    int               nVars;     // inputs
    int               nNodes;    // internal nodes
    int               nObjs;     // total objects (nVars inputs + nNodes internal nodes)
    int               nWords;    // the truth table size in 64-bit words
    int               iVar;      // the next available SAT variable
    word *            pTruth;    // truth table
    Vec_Wrd_t *       vInfo;     // nVars + nNodes + 1
    int               VarMarks[MAJ_NOBJS][2][MAJ_NOBJS]; // variable marks
    int               VarVals[MAJ_NOBJS]; // values of the first nVars variables
    Vec_Wec_t *       vOutLits;  // output vars
    bmcg_sat_solver * pSat;      // SAT solver
    FILE *            pFile;
    int               nCnfClauses;
};

static inline word *  Exa_ManTruth( Exa_Man_t * p, int v ) { return Vec_WrdEntryP( p->vInfo, p->nWords * v ); }


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wrd_t * Exa_ManTruthTables( Exa_Man_t * p )
{
    Vec_Wrd_t * vInfo = p->vInfo = Vec_WrdStart( p->nWords * (p->nObjs+1) ); int i;
    for ( i = 0; i < p->nVars; i++ )
        Abc_TtIthVar( Exa_ManTruth(p, i), i, p->nVars );
    //Dau_DsdPrintFromTruth( Exa_ManTruth(p, p->nObjs), p->nVars );
    return vInfo;
}
int Exa_ManMarkup( Exa_Man_t * p )
{
    int i, k, j;
    assert( p->nObjs <= MAJ_NOBJS );
    // assign functionality
    p->iVar = 1 + 3*p->nNodes;//
    // assign connectivity variables
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        for ( k = 0; k < 2; k++ )
        {
            if ( p->pPars->fFewerVars && i == p->nObjs - 1 && k == 0 )
            {
                j = p->nObjs - 2;
                Vec_WecPush( p->vOutLits, j, Abc_Var2Lit(p->iVar, 0) );
                p->VarMarks[i][k][j] = p->iVar++;
                continue;
            }
            for ( j = p->pPars->fFewerVars ? 1 - k : 0; j < i - k; j++ )
            {
                Vec_WecPush( p->vOutLits, j, Abc_Var2Lit(p->iVar, 0) );
                p->VarMarks[i][k][j] = p->iVar++;
            }
        }
    }
    printf( "The number of parameter variables = %d.\n", p->iVar );
    return p->iVar;
    // printout
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        printf( "Node %d\n", i );
        for ( j = 0; j < p->nObjs; j++ )
        {
            for ( k = 0; k < 2; k++ )
                printf( "%3d ", p->VarMarks[i][k][j] );
            printf( "\n" );
        }
    }
    return p->iVar;
}
Exa_Man_t * Exa_ManAlloc( Bmc_EsPar_t * pPars, word * pTruth )
{
    Exa_Man_t * p = ABC_CALLOC( Exa_Man_t, 1 );
    p->pPars      = pPars;
    p->nVars      = pPars->nVars;
    p->nNodes     = pPars->nNodes;
    p->nObjs      = pPars->nVars + pPars->nNodes;
    p->nWords     = Abc_TtWordNum(pPars->nVars);
    p->pTruth     = pTruth;
    p->vOutLits   = Vec_WecStart( p->nObjs );
    p->iVar       = Exa_ManMarkup( p );
    p->vInfo      = Exa_ManTruthTables( p );
    p->pSat       = bmcg_sat_solver_start();
    bmcg_sat_solver_set_nvars( p->pSat, p->iVar );
    if ( pPars->RuntimeLim )
        bmcg_sat_solver_set_runtime_limit( p->pSat, Abc_Clock() + pPars->RuntimeLim * CLOCKS_PER_SEC );
    if ( pPars->fDumpCnf )
    {
        char Buffer[1000];
        sprintf( Buffer, "%s_%d_%d.cnf", p->pPars->pTtStr, 2, p->nNodes );
        p->pFile = fopen( Buffer, "wb" );
        fputs( "p cnf                \n", p->pFile );
    }
    return p;
}
void Exa_ManFree( Exa_Man_t * p )
{
    if ( p->pFile ) 
    {
        char Buffer[1000];
        sprintf( Buffer, "%s_%d_%d.cnf", p->pPars->pTtStr, 2, p->nNodes );
        rewind( p->pFile );
        fprintf( p->pFile, "p cnf %d %d", bmcg_sat_solver_varnum(p->pSat), p->nCnfClauses );
        fclose( p->pFile );
        printf( "CNF was dumped into file \"%s\".\n", Buffer );
    }
    bmcg_sat_solver_stop( p->pSat );
    Vec_WrdFree( p->vInfo );
    Vec_WecFree( p->vOutLits );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Exa_ManFindFanin( Exa_Man_t * p, int i, int k )
{
    int j, Count = 0, iVar = -1;
    for ( j = 0; j < p->nObjs; j++ )
        if ( p->VarMarks[i][k][j] && bmcg_sat_solver_read_cex_varvalue(p->pSat, p->VarMarks[i][k][j]) )
        {
            iVar = j;
            Count++;
        }
    assert( Count == 1 );
    return iVar;
}
static inline int Exa_ManEval( Exa_Man_t * p )
{
    static int Flag = 0;
    int i, k, iMint; word * pFanins[2];
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        int iVarStart = 1 + 3*(i - p->nVars);//
        for ( k = 0; k < 2; k++ )
            pFanins[k] = Exa_ManTruth( p, Exa_ManFindFanin(p, i, k) );
        Abc_TtConst0( Exa_ManTruth(p, i), p->nWords );
        for ( k = 1; k < 4; k++ )
        {
            if ( !bmcg_sat_solver_read_cex_varvalue(p->pSat, iVarStart+k-1) )
                continue;
            Abc_TtAndCompl( Exa_ManTruth(p, p->nObjs), pFanins[0], !(k&1), pFanins[1], !(k>>1), p->nWords );
            Abc_TtOr( Exa_ManTruth(p, i), Exa_ManTruth(p, i), Exa_ManTruth(p, p->nObjs), p->nWords );
        }
    }
    if ( Flag && p->nVars >= 6 )
        iMint = Abc_TtFindLastDiffBit( Exa_ManTruth(p, p->nObjs-1), p->pTruth, p->nVars );
    else
        iMint = Abc_TtFindFirstDiffBit( Exa_ManTruth(p, p->nObjs-1), p->pTruth, p->nVars );
    //Flag ^= 1;
    assert( iMint < (1 << p->nVars) );
    return iMint;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Exa_ManDumpBlif( Exa_Man_t * p, int fCompl )
{
    char Buffer[1000];
    char FileName[1100];
    FILE * pFile;
    int i, k, iVar;
    if ( fCompl )
        Abc_TtNot( p->pTruth, p->nWords );
    Extra_PrintHexadecimalString( Buffer, (unsigned *)p->pTruth, p->nVars );
    if ( fCompl )
        Abc_TtNot( p->pTruth, p->nWords );
    sprintf( FileName, "%s_%d_%d.blif", Buffer, 2, p->nNodes );
    pFile = fopen( FileName, "wb" );
    fprintf( pFile, "# Realization of the %d-input function %s using %d two-input gates:\n", p->nVars, Buffer, p->nNodes );
    fprintf( pFile, ".model %s_%d_%d\n", Buffer, 2, p->nNodes );
    fprintf( pFile, ".inputs" );
    for ( i = 0; i < p->nVars; i++ )
        fprintf( pFile, " %c", 'a'+i );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs F\n" );
    for ( i = p->nObjs - 1; i >= p->nVars; i-- )
    {
        int iVarStart = 1 + 3*(i - p->nVars);//
        int Val1 = bmcg_sat_solver_read_cex_varvalue(p->pSat, iVarStart);
        int Val2 = bmcg_sat_solver_read_cex_varvalue(p->pSat, iVarStart+1);
        int Val3 = bmcg_sat_solver_read_cex_varvalue(p->pSat, iVarStart+2);

        fprintf( pFile, ".names" );
        for ( k = 1; k >= 0; k-- )
        {
            iVar = Exa_ManFindFanin( p, i, k );
            if ( iVar >= 0 && iVar < p->nVars )
                fprintf( pFile, " %c", 'a'+iVar );
            else
                fprintf( pFile, " %02d", iVar );
        }
        if ( i == p->nObjs - 1 )
            fprintf( pFile, " F\n" );
        else
            fprintf( pFile, " %02d\n", i );
        if ( i == p->nObjs - 1 && fCompl )
            fprintf( pFile, "00 1\n" );
        if ( (i == p->nObjs - 1 && fCompl) ^ Val1 )
            fprintf( pFile, "01 1\n" );
        if ( (i == p->nObjs - 1 && fCompl) ^ Val2 )
            fprintf( pFile, "10 1\n" );
        if ( (i == p->nObjs - 1 && fCompl) ^ Val3 )
            fprintf( pFile, "11 1\n" );
    }
    fprintf( pFile, ".end\n\n" );
    fclose( pFile );
    printf( "Solution was dumped into file \"%s\".\n", FileName );
}
void Exa_ManPrintSolution( Exa_Man_t * p, int fCompl )
{
    int i, k, iVar;
    printf( "Realization of given %d-input function using %d two-input gates:\n", p->nVars, p->nNodes );
//    for ( i = p->nVars + 2; i < p->nObjs; i++ )
    for ( i = p->nObjs - 1; i >= p->nVars; i-- )
    {
        int iVarStart = 1 + 3*(i - p->nVars);//
        int Val1 = bmcg_sat_solver_read_cex_varvalue(p->pSat, iVarStart);
        int Val2 = bmcg_sat_solver_read_cex_varvalue(p->pSat, iVarStart+1);
        int Val3 = bmcg_sat_solver_read_cex_varvalue(p->pSat, iVarStart+2);
        if ( i == p->nObjs - 1 && fCompl )
            printf( "%02d = 4\'b%d%d%d1(", i, !Val3, !Val2, !Val1 );
        else
            printf( "%02d = 4\'b%d%d%d0(", i, Val3, Val2, Val1 );
        for ( k = 1; k >= 0; k-- )
        {
            iVar = Exa_ManFindFanin( p, i, k );
            if ( iVar >= 0 && iVar < p->nVars )
                printf( " %c", 'a'+iVar );
            else
                printf( " %02d", iVar );
        }
        printf( " )\n" );
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Exa_ManAddClause( Exa_Man_t * p, int * pLits, int nLits )
{
    int i;
    if ( p->pFile )
    {
        p->nCnfClauses++;
        for ( i = 0; i < nLits; i++ )
            fprintf( p->pFile, "%s%d ", Abc_LitIsCompl(pLits[i]) ? "-" : "", Abc_Lit2Var(pLits[i]) );
        fprintf( p->pFile, "0\n" );
    }
    return bmcg_sat_solver_addclause( p->pSat, pLits, nLits );
}
int Exa_ManAddCnfAdd( Exa_Man_t * p, int * pnAdded )
{
    int pLits[MAJ_NOBJS], pLits2[2], i, j, k, n, m;
    *pnAdded = 0;
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        for ( k = 0; k < 2; k++ )
        {
            int nLits = 0;
            for ( j = 0; j < p->nObjs; j++ )
                if ( p->VarMarks[i][k][j] && bmcg_sat_solver_read_cex_varvalue(p->pSat, p->VarMarks[i][k][j]) )
                    pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][k][j], 0 );
            assert( nLits > 0 );
            // input uniqueness
            for ( n = 0;   n < nLits; n++ )
            for ( m = n+1; m < nLits; m++ )
            {
                (*pnAdded)++;
                pLits2[0] = Abc_LitNot(pLits[n]);
                pLits2[1] = Abc_LitNot(pLits[m]);
                if ( !Exa_ManAddClause( p, pLits2, 2 ) )
                    return 0;
            }
            if ( k == 1 )
                break;
            // symmetry breaking
            for ( j = 0; j < p->nObjs; j++ ) if ( p->VarMarks[i][k][j]   && bmcg_sat_solver_read_cex_varvalue(p->pSat, p->VarMarks[i][k][j]) )
            for ( n = j; n < p->nObjs; n++ ) if ( p->VarMarks[i][k+1][n] && bmcg_sat_solver_read_cex_varvalue(p->pSat, p->VarMarks[i][k+1][j]) )
            {
                (*pnAdded)++;
                pLits2[0] = Abc_Var2Lit( p->VarMarks[i][k][j], 1 );
                pLits2[1] = Abc_Var2Lit( p->VarMarks[i][k+1][n], 1 );
                if ( !Exa_ManAddClause( p, pLits2, 2 ) )
                    return 0;
            }
        }
    }
    return 1;
}
int Exa_ManSolverSolve( Exa_Man_t * p )
{
    int nAdded = 1;
    while ( nAdded )
    {
        int status = bmcg_sat_solver_solve( p->pSat, NULL, 0 );
        if ( status != GLUCOSE_SAT )
            return status;
        status = Exa_ManAddCnfAdd( p, &nAdded );
        //printf( "Added %d clauses.\n", nAdded );
        if ( status != GLUCOSE_SAT )
            return status;
    }
    return GLUCOSE_SAT;
}
int Exa_ManAddCnfStart( Exa_Man_t * p, int fOnlyAnd )
{
    int pLits[MAJ_NOBJS], pLits2[3], i, j, k, n, m;
    // input constraints
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        int iVarStart = 1 + 3*(i - p->nVars);//
        for ( k = 0; k < 2; k++ )
        {
            int nLits = 0;
            for ( j = 0; j < p->nObjs; j++ )
                if ( p->VarMarks[i][k][j] )
                    pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][k][j], 0 );
            assert( nLits > 0 );
            if ( !Exa_ManAddClause( p, pLits, nLits ) )
                return 0;
            // input uniqueness
            if ( !p->pPars->fDynConstr )
            {
                for ( n = 0;   n < nLits; n++ )
                for ( m = n+1; m < nLits; m++ )
                {
                    pLits2[0] = Abc_LitNot(pLits[n]);
                    pLits2[1] = Abc_LitNot(pLits[m]);
                    if ( !Exa_ManAddClause( p, pLits2, 2 ) )
                        return 0;
                }
            }
            if ( k == 1 )
                break;
            // symmetry breaking
            if ( !p->pPars->fDynConstr )
            {
                for ( j = 0; j < p->nObjs; j++ ) if ( p->VarMarks[i][k][j] )
                for ( n = j; n < p->nObjs; n++ ) if ( p->VarMarks[i][k+1][n] )
                {
                    pLits2[0] = Abc_Var2Lit( p->VarMarks[i][k][j], 1 );
                    pLits2[1] = Abc_Var2Lit( p->VarMarks[i][k+1][n], 1 );
                    if ( !Exa_ManAddClause( p, pLits2, 2 ) )
                        return 0;
                }
            }
        }
#ifdef USE_NODE_ORDER
        // node ordering
        for ( j = p->nVars; j < i; j++ )
        for ( n = 0;   n < p->nObjs; n++ ) if ( p->VarMarks[i][0][n] )
        for ( m = n+1; m < p->nObjs; m++ ) if ( p->VarMarks[j][0][m] )
        {
            pLits2[0] = Abc_Var2Lit( p->VarMarks[i][0][n], 1 );
            pLits2[1] = Abc_Var2Lit( p->VarMarks[j][0][m], 1 );
            if ( !Exa_ManAddClause( p, pLits2, 2 ) )
                return 0;
        }
#endif
        // unique functions
        for ( j = p->nVars; j < i; j++ )
        for ( k = 0; k < 2; k++ ) if ( p->VarMarks[i][k][j] )
        {
            pLits2[0] = Abc_Var2Lit( p->VarMarks[i][k][j], 1 );
            for ( n = 0; n < p->nObjs; n++ ) 
            for ( m = 0; m < 2; m++ )
            {
                if ( p->VarMarks[i][!k][n] && p->VarMarks[j][m][n] )
                {
                    pLits2[1] = Abc_Var2Lit( p->VarMarks[i][!k][n], 1 );
                    pLits2[2] = Abc_Var2Lit( p->VarMarks[j][m][n], 1 );
                    if ( !Exa_ManAddClause( p, pLits2, 3 ) )
                        return 0;
                }
            }
        }
        // two input functions
        for ( k = 0; k < 3; k++ )
        {
            pLits[0] = Abc_Var2Lit( iVarStart,   k==1 );
            pLits[1] = Abc_Var2Lit( iVarStart+1, k==2 );
            pLits[2] = Abc_Var2Lit( iVarStart+2, k!=0 );
            if ( !Exa_ManAddClause( p, pLits, 3 ) )
                return 0;
        }
        if ( fOnlyAnd )
        {
            pLits[0] = Abc_Var2Lit( iVarStart,   1 );
            pLits[1] = Abc_Var2Lit( iVarStart+1, 1 );
            pLits[2] = Abc_Var2Lit( iVarStart+2, 0 );
            if ( !Exa_ManAddClause( p, pLits, 3 ) )
                return 0;
        }
    }
    // outputs should be used
    for ( i = 0; i < p->nObjs - 1; i++ )
    {
        Vec_Int_t * vArray = Vec_WecEntry(p->vOutLits, i);
        assert( Vec_IntSize(vArray) > 0 );
        if ( !Exa_ManAddClause( p, Vec_IntArray(vArray), Vec_IntSize(vArray) ) )
            return 0;
    }
    return 1;
}
int Exa_ManAddCnf( Exa_Man_t * p, int iMint )
{
    // save minterm values
    int i, k, n, j, Value = Abc_TtGetBit(p->pTruth, iMint);
    for ( i = 0; i < p->nVars; i++ )
        p->VarVals[i] = (iMint >> i) & 1;
    bmcg_sat_solver_set_nvars( p->pSat, p->iVar + 3*p->nNodes );
    //printf( "Adding clauses for minterm %d with value %d.\n", iMint, Value );
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        // fanin connectivity
        int iVarStart = 1 + 3*(i - p->nVars);//
        int iBaseSatVarI = p->iVar + 3*(i - p->nVars);
        for ( k = 0; k < 2; k++ )
        {
            for ( j = 0; j < p->nObjs; j++ ) if ( p->VarMarks[i][k][j] )
            {
                int iBaseSatVarJ = p->iVar + 3*(j - p->nVars);
                for ( n = 0; n < 2; n++ )
                {
                    int pLits[3], nLits = 0;
                    pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][k][j], 1 );
                    pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + k, n );
                    if ( j >= p->nVars )
                        pLits[nLits++] = Abc_Var2Lit( iBaseSatVarJ + 2, !n );
                    else if ( p->VarVals[j] == n )
                        continue;
                    if ( !Exa_ManAddClause( p, pLits, nLits ) )
                        return 0;
                }
            }
        }
        // node functionality
        for ( n = 0; n < 2; n++ )
        {
            if ( i == p->nObjs - 1 && n == Value )
                continue;
            for ( k = 0; k < 4; k++ )
            {
                int pLits[4], nLits = 0;
                if ( k == 0 && n == 1 )
                    continue;
                pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + 0, (k&1)  );
                pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + 1, (k>>1) );
                if ( i != p->nObjs - 1 ) pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + 2, !n );
                if ( k > 0 )             pLits[nLits++] = Abc_Var2Lit( iVarStart +  k-1,  n );
                assert( nLits <= 4 );
                if ( !Exa_ManAddClause( p, pLits, nLits ) )
                    return 0;
            }
        }
    }
    p->iVar += 3*p->nNodes;
    return 1;
}
void Exa_ManExactSynthesis( Bmc_EsPar_t * pPars )
{
    int i, status, iMint = 1;
    abctime clkTotal = Abc_Clock();
    Exa_Man_t * p; int fCompl = 0;
    word pTruth[16]; Abc_TtReadHex( pTruth, pPars->pTtStr );
    assert( pPars->nVars <= 10 );
    p = Exa_ManAlloc( pPars, pTruth );
    if ( pTruth[0] & 1 ) { fCompl = 1; Abc_TtNot( pTruth, p->nWords ); }
    status = Exa_ManAddCnfStart( p, pPars->fOnlyAnd );
    assert( status );
    printf( "Running exact synthesis for %d-input function with %d two-input gates...\n", p->nVars, p->nNodes );
    for ( i = 0; iMint != -1; i++ )
    {
        abctime clk = Abc_Clock();
        if ( !Exa_ManAddCnf( p, iMint ) )
            break;
        if ( pPars->fDynConstr )
            status = Exa_ManSolverSolve( p );
        else
            status = bmcg_sat_solver_solve( p->pSat, NULL, 0 );
        if ( pPars->fVerbose )
        {
            printf( "Iter %3d : ", i );
            Extra_PrintBinary( stdout, (unsigned *)&iMint, p->nVars );
            printf( "  Var =%5d  ", p->iVar );
            printf( "Cla =%6d  ", bmcg_sat_solver_clausenum(p->pSat) );
            printf( "Conf =%9d  ", bmcg_sat_solver_conflictnum(p->pSat) );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        }
        if ( status == GLUCOSE_UNSAT )
        {
            printf( "The problem has no solution.\n" );
            break;
        }
        if ( status == GLUCOSE_UNDEC )
        {
            printf( "The problem timed out after %d sec.\n", pPars->RuntimeLim );
            break;
        }
        iMint = Exa_ManEval( p );
    }
    if ( iMint == -1 )
    {
        Exa_ManPrintSolution( p, fCompl );
        Exa_ManDumpBlif( p, fCompl );
    }
    Exa_ManFree( p );
    Abc_PrintTime( 1, "Total runtime", Abc_Clock() - clkTotal );
}




typedef struct Exa3_Man_t_ Exa3_Man_t;
struct Exa3_Man_t_ 
{
    Bmc_EsPar_t *     pPars;     // parameters
    int               nVars;     // inputs
    int               nNodes;    // internal nodes
    int               nLutSize;  // lut size
    int               LutMask;   // lut mask
    int               nObjs;     // total objects (nVars inputs + nNodes internal nodes)
    int               nWords;    // the truth table size in 64-bit words
    int               iVar;      // the next available SAT variable
    word *            pTruth;    // truth table
    Vec_Wrd_t *       vInfo;     // nVars + nNodes + 1
    Vec_Bit_t *       vUsed2;    // bit masks
    Vec_Bit_t *       vUsed3;    // bit masks
    int               VarMarks[MAJ_NOBJS][6][MAJ_NOBJS]; // variable marks
    int               VarVals[MAJ_NOBJS]; // values of the first nVars variables
    Vec_Wec_t *       vOutLits;  // output vars
    bmcg_sat_solver * pSat;      // SAT solver
    int               nUsed[2];
};

static inline word *  Exa3_ManTruth( Exa3_Man_t * p, int v ) { return Vec_WrdEntryP( p->vInfo, p->nWords * v ); }

static inline int     Exa3_ManIsUsed2( Exa3_Man_t * p, int m, int n, int i, int j )
{
    int Pos = ((m * p->pPars->nNodes + n - p->pPars->nVars) * p->nObjs + i) * p->nObjs + j;
    p->nUsed[0]++;
    assert( i < n && j < n && i < j );
    if ( Vec_BitEntry(p->vUsed2, Pos) )
        return 1;
    p->nUsed[1]++;
    Vec_BitWriteEntry( p->vUsed2, Pos, 1 );
    return 0;
}

static inline int     Exa3_ManIsUsed3( Exa3_Man_t * p, int m, int n, int i, int j, int k )
{
    int Pos = (((m * p->pPars->nNodes + n - p->pPars->nVars) * p->nObjs + i) * p->nObjs + j) * p->nObjs + k;
    p->nUsed[0]++;
    assert( i < n && j < n && k < n && i < j && j < k );
    if ( Vec_BitEntry(p->vUsed3, Pos) )
        return 1;
    p->nUsed[1]++;
    Vec_BitWriteEntry( p->vUsed3, Pos, 1 );
    return 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Vec_Wrd_t * Exa3_ManTruthTables( Exa3_Man_t * p )
{
    Vec_Wrd_t * vInfo = p->vInfo = Vec_WrdStart( p->nWords * (p->nObjs+1) ); int i;
    for ( i = 0; i < p->nVars; i++ )
        Abc_TtIthVar( Exa3_ManTruth(p, i), i, p->nVars );
    //Dau_DsdPrintFromTruth( Exa3_ManTruth(p, p->nObjs), p->nVars );
    return vInfo;
}
static int Exa3_ManMarkup( Exa3_Man_t * p )
{
    int i, k, j;
    assert( p->nObjs <= MAJ_NOBJS );
    // assign functionality variables
    p->iVar = 1 + p->LutMask * p->nNodes;
    // assign connectivity variables
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        for ( k = 0; k < p->nLutSize; k++ )
        {
            if ( p->pPars->fFewerVars && i == p->nObjs - 1 && k == 0 )
            {
                j = p->nObjs - 2;
                Vec_WecPush( p->vOutLits, j, Abc_Var2Lit(p->iVar, 0) );
                p->VarMarks[i][k][j] = p->iVar++;
                continue;
            }
            for ( j = p->pPars->fFewerVars ? p->nLutSize - 1 - k : 0; j < i - k; j++ )
            {
                Vec_WecPush( p->vOutLits, j, Abc_Var2Lit(p->iVar, 0) );
                p->VarMarks[i][k][j] = p->iVar++;
            }
        }
    }
    printf( "The number of parameter variables = %d.\n", p->iVar );
    return p->iVar;
    // printout
    for ( i = p->nObjs - 1; i >= p->nVars; i-- )
    {
        printf( "       Node %2d\n", i );
        for ( j = 0; j < p->nObjs; j++ )
        {
            printf( "Fanin %2d : ", j );
            for ( k = 0; k < p->nLutSize; k++ )
                printf( "%3d ", p->VarMarks[i][k][j] );
            printf( "\n" );
        }
    }
    return p->iVar;
}
static Exa3_Man_t * Exa3_ManAlloc( Bmc_EsPar_t * pPars, word * pTruth )
{
    Exa3_Man_t * p = ABC_CALLOC( Exa3_Man_t, 1 );
    p->pPars      = pPars;
    p->nVars      = pPars->nVars;
    p->nNodes     = pPars->nNodes;
    p->nLutSize   = pPars->nLutSize;
    p->LutMask    = (1 << pPars->nLutSize) - 1;
    p->nObjs      = pPars->nVars + pPars->nNodes;
    p->nWords     = Abc_TtWordNum(pPars->nVars);
    p->pTruth     = pTruth;
    p->vOutLits   = Vec_WecStart( p->nObjs );
    p->iVar       = Exa3_ManMarkup( p );
    p->vInfo      = Exa3_ManTruthTables( p );
    if ( p->pPars->nLutSize == 2 )
    p->vUsed2     = Vec_BitStart( (1 << p->pPars->nVars) * p->pPars->nNodes * p->nObjs * p->nObjs );
    else if ( p->pPars->nLutSize == 3 )
    p->vUsed3     = Vec_BitStart( (1 << p->pPars->nVars) * p->pPars->nNodes * p->nObjs * p->nObjs * p->nObjs );
    p->pSat       = bmcg_sat_solver_start();
    bmcg_sat_solver_set_nvars( p->pSat, p->iVar );
    if ( pPars->RuntimeLim )
        bmcg_sat_solver_set_runtime_limit( p->pSat, Abc_Clock() + pPars->RuntimeLim * CLOCKS_PER_SEC );
    return p;
}
static void Exa3_ManFree( Exa3_Man_t * p )
{
    bmcg_sat_solver_stop( p->pSat );
    Vec_WrdFree( p->vInfo );
    Vec_BitFreeP( &p->vUsed2 );
    Vec_BitFreeP( &p->vUsed3 );
    Vec_WecFree( p->vOutLits );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Exa3_ManFindFanin( Exa3_Man_t * p, int i, int k )
{
    int j, Count = 0, iVar = -1;
    for ( j = 0; j < p->nObjs; j++ )
        if ( p->VarMarks[i][k][j] && bmcg_sat_solver_read_cex_varvalue(p->pSat, p->VarMarks[i][k][j]) )
        {
            iVar = j;
            Count++;
        }
    assert( Count == 1 );
    return iVar;
}
static inline int Exa3_ManEval( Exa3_Man_t * p )
{
    static int Flag = 0;
    int i, k, j, iMint; word * pFanins[6];
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        int iVarStart = 1 + p->LutMask*(i - p->nVars);
        for ( k = 0; k < p->nLutSize; k++ )
            pFanins[k] = Exa3_ManTruth( p, Exa3_ManFindFanin(p, i, k) );
        Abc_TtConst0( Exa3_ManTruth(p, i),        p->nWords );
        for ( k = 1; k <= p->LutMask; k++ )
        {
            if ( !bmcg_sat_solver_read_cex_varvalue(p->pSat, iVarStart+k-1) )
                continue;
//            Abc_TtAndCompl( Exa3_ManTruth(p, p->nObjs), pFanins[0], !(k&1), pFanins[1], !(k>>1), p->nWords );
            Abc_TtConst1( Exa3_ManTruth(p, p->nObjs), p->nWords );
            for ( j = 0; j < p->nLutSize; j++ )
                Abc_TtAndCompl( Exa3_ManTruth(p, p->nObjs), Exa3_ManTruth(p, p->nObjs), 0, pFanins[j], !((k >> j) & 1), p->nWords );
            Abc_TtOr( Exa3_ManTruth(p, i), Exa3_ManTruth(p, i), Exa3_ManTruth(p, p->nObjs), p->nWords );
        }
    }
    if ( Flag && p->nVars >= 6 )
        iMint = Abc_TtFindLastDiffBit( Exa3_ManTruth(p, p->nObjs-1), p->pTruth, p->nVars );
    else
        iMint = Abc_TtFindFirstDiffBit( Exa3_ManTruth(p, p->nObjs-1), p->pTruth, p->nVars );
    //Flag ^= 1;
    assert( iMint < (1 << p->nVars) );
    return iMint;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Exa3_ManPrintSolution( Exa3_Man_t * p, int fCompl )
{
    int i, k, iVar;
    printf( "Realization of given %d-input function using %d %d-input LUTs:\n", p->nVars, p->nNodes, p->nLutSize );
    for ( i = p->nObjs - 1; i >= p->nVars; i-- )
    {
        int Val, iVarStart = 1 + p->LutMask*(i - p->nVars);
        printf( "%02d = %d\'b", i, 1 << p->nLutSize );
        for ( k = p->LutMask - 1; k >= 0; k-- )
        {
            Val = bmcg_sat_solver_read_cex_varvalue(p->pSat, iVarStart+k); 
            if ( i == p->nObjs - 1 && fCompl )
                printf( "%d", !Val );
            else
                printf( "%d", Val );
        }
        if ( i == p->nObjs - 1 && fCompl )
            printf( "1(" );
        else
            printf( "0(" );

        for ( k = p->nLutSize - 1; k >= 0; k-- )
        {
            iVar = Exa3_ManFindFanin( p, i, k );
            if ( iVar >= 0 && iVar < p->nVars )
                printf( " %c", 'a'+iVar );
            else
                printf( " %02d", iVar );
        }
        printf( " )\n" );
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Exa3_ManAddCnfStart( Exa3_Man_t * p, int fOnlyAnd )
{
    int pLits[MAJ_NOBJS], pLits2[2], i, j, k, n, m;
    // input constraints
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        int iVarStart = 1 + p->LutMask*(i - p->nVars);
        for ( k = 0; k < p->nLutSize; k++ )
        {
            int nLits = 0;
            for ( j = 0; j < p->nObjs; j++ )
                if ( p->VarMarks[i][k][j] )
                    pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][k][j], 0 );
            assert( nLits > 0 );
            // input uniqueness
            if ( !bmcg_sat_solver_addclause( p->pSat, pLits, nLits ) )
                return 0;
            for ( n = 0;   n < nLits; n++ )
            for ( m = n+1; m < nLits; m++ )
            {
                pLits2[0] = Abc_LitNot(pLits[n]);
                pLits2[1] = Abc_LitNot(pLits[m]);
                if ( !bmcg_sat_solver_addclause( p->pSat, pLits2, 2 ) )
                    return 0;
            }
            if ( k == p->nLutSize - 1 )
                break;
            // symmetry breaking
            for ( j = 0; j < p->nObjs; j++ ) if ( p->VarMarks[i][k][j] )
            for ( n = j; n < p->nObjs; n++ ) if ( p->VarMarks[i][k+1][n] )
            {
                pLits2[0] = Abc_Var2Lit( p->VarMarks[i][k][j], 1 );
                pLits2[1] = Abc_Var2Lit( p->VarMarks[i][k+1][n], 1 );
                if ( !bmcg_sat_solver_addclause( p->pSat, pLits2, 2 ) )
                    return 0;
            }
        }
//#ifdef USE_NODE_ORDER
        // node ordering
        if ( p->pPars->fOrderNodes )
        {
            for ( j = p->nVars; j < i; j++ )
            for ( n = 0;   n < p->nObjs; n++ ) if ( p->VarMarks[i][0][n] )
            for ( m = n+1; m < p->nObjs; m++ ) if ( p->VarMarks[j][0][m] )
            {
                pLits2[0] = Abc_Var2Lit( p->VarMarks[i][0][n], 1 );
                pLits2[1] = Abc_Var2Lit( p->VarMarks[j][0][m], 1 );
                if ( !bmcg_sat_solver_addclause( p->pSat, pLits2, 2 ) )
                    return 0;
            }
        }
//#endif
        if ( p->nLutSize != 2 )
            continue;
        // two-input functions
        for ( k = 0; k < 3; k++ )
        {
            pLits[0] = Abc_Var2Lit( iVarStart,   k==1 );
            pLits[1] = Abc_Var2Lit( iVarStart+1, k==2 );
            pLits[2] = Abc_Var2Lit( iVarStart+2, k!=0 );
            if ( !bmcg_sat_solver_addclause( p->pSat, pLits, 3 ) )
                return 0;
        }
        if ( fOnlyAnd )
        {
            pLits[0] = Abc_Var2Lit( iVarStart,   1 );
            pLits[1] = Abc_Var2Lit( iVarStart+1, 1 );
            pLits[2] = Abc_Var2Lit( iVarStart+2, 0 );
            if ( !bmcg_sat_solver_addclause( p->pSat, pLits, 3 ) )
                return 0;
        }
    }
    // outputs should be used
    for ( i = 0; i < p->nObjs - 1; i++ )
    {
        Vec_Int_t * vArray = Vec_WecEntry(p->vOutLits, i);
        assert( Vec_IntSize(vArray) > 0 );
        if ( !bmcg_sat_solver_addclause( p->pSat, Vec_IntArray(vArray), Vec_IntSize(vArray) ) )
            return 0;
    }
    return 1;
}
static int Exa3_ManAddCnf( Exa3_Man_t * p, int iMint )
{
    // save minterm values
    int i, k, n, j, Value = Abc_TtGetBit(p->pTruth, iMint);
    for ( i = 0; i < p->nVars; i++ )
        p->VarVals[i] = (iMint >> i) & 1;
//    sat_solver_setnvars( p->pSat, p->iVar + (p->nLutSize+1)*p->nNodes );
    bmcg_sat_solver_set_nvars( p->pSat, p->iVar + (p->nLutSize+1)*p->nNodes );
    //printf( "Adding clauses for minterm %d with value %d.\n", iMint, Value );
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        // fanin connectivity
        int iVarStart = 1 + p->LutMask*(i - p->nVars);
        int iBaseSatVarI = p->iVar + (p->nLutSize+1)*(i - p->nVars);
        for ( k = 0; k < p->nLutSize; k++ )
        {
            for ( j = 0; j < p->nObjs; j++ ) if ( p->VarMarks[i][k][j] )
            {
                int iBaseSatVarJ = p->iVar + (p->nLutSize+1)*(j - p->nVars);
                for ( n = 0; n < 2; n++ )
                {
                    int pLits[3], nLits = 0;
                    pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][k][j], 1 );
                    pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + k, n );
                    if ( j >= p->nVars )
                        pLits[nLits++] = Abc_Var2Lit( iBaseSatVarJ + p->nLutSize, !n );
                    else if ( p->VarVals[j] == n )
                        continue;
                    if ( !bmcg_sat_solver_addclause( p->pSat, pLits, nLits ) )
                        return 0;
                }
            }
        }
        // node functionality
        for ( n = 0; n < 2; n++ )
        {
            if ( i == p->nObjs - 1 && n == Value )
                continue;
            for ( k = 0; k <= p->LutMask; k++ )
            {
                int pLits[16], nLits = 0;
                if ( k == 0 && n == 1 )
                    continue;
                //pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + 0, (k&1)  );
                //pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + 1, (k>>1) );
                //if ( i != p->nObjs - 1 ) pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + 2, !n );
                for ( j = 0; j < p->nLutSize; j++ )
                    pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + j, (k >> j) & 1 );
                if ( i != p->nObjs - 1 ) pLits[nLits++] = Abc_Var2Lit( iBaseSatVarI + j, !n );
                if ( k > 0 )             pLits[nLits++] = Abc_Var2Lit( iVarStart +  k-1,  n );
                assert( nLits <= p->nLutSize+2 );
                if ( !bmcg_sat_solver_addclause( p->pSat, pLits, nLits ) )
                    return 0;
            }
        }
    }
    p->iVar += (p->nLutSize+1)*p->nNodes;
    return 1;
}

static int Exa3_ManAddCnf2( Exa3_Man_t * p, int iMint )
{
    int iBaseSatVar = p->iVar + p->nNodes*iMint;
    // save minterm values
    int i, k, n, j, Value = Abc_TtGetBit(p->pTruth, iMint);
    for ( i = 0; i < p->nVars; i++ )
        p->VarVals[i] = (iMint >> i) & 1;
    //printf( "Adding clauses for minterm %d with value %d.\n", iMint, Value );
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
//        int iBaseSatVarI = p->iVar + (p->nLutSize+1)*(i - p->nVars);
        int iVarStart = 1 + p->LutMask*(i - p->nVars);
        // collect fanin variables
        int pFanins[16];
        for ( j = 0; j < p->nLutSize; j++ )
            pFanins[j] = Exa3_ManFindFanin(p, i, j);
        // check cache
        if ( p->pPars->nLutSize == 2 && Exa3_ManIsUsed2(p, iMint, i, pFanins[1], pFanins[0]) )
            continue;
        if ( p->pPars->nLutSize == 3 && Exa3_ManIsUsed3(p, iMint, i, pFanins[2], pFanins[1], pFanins[0]) )
            continue;
        // node functionality
        for ( n = 0; n < 2; n++ )
        {
            if ( i == p->nObjs - 1 && n == Value )
                continue;
            for ( k = 0; k <= p->LutMask; k++ )
            {
                int pLits[16], nLits = 0;
                if ( k == 0 && n == 1 )
                    continue;
                for ( j = 0; j < p->nLutSize; j++ )
                {
//                    pLits[nLits++] = Abc_Var2Lit( iBaseSatVar + j, (k >> j) & 1 );
                    pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][j][pFanins[j]], 1 );
                    if ( pFanins[j] >= p->nVars )
                        pLits[nLits++] = Abc_Var2Lit( iBaseSatVar + pFanins[j] - p->nVars, (k >> j) & 1 );
                    else if ( p->VarVals[pFanins[j]] != ((k >> j) & 1) )
                        break;
                }
                if ( j < p->nLutSize )
                    continue;
//                if ( i != p->nObjs - 1 ) pLits[nLits++] = Abc_Var2Lit( iBaseSatVar + j, !n );
                if ( i != p->nObjs - 1 ) pLits[nLits++] = Abc_Var2Lit( iBaseSatVar + i - p->nVars, !n );
                if ( k > 0 )             pLits[nLits++] = Abc_Var2Lit( iVarStart + k-1,             n );
                assert( nLits <= 2*p->nLutSize+2 );
                if ( !bmcg_sat_solver_addclause( p->pSat, pLits, nLits ) )
                    return 0;
            }
        }
    }
    return 1;
}
void Exa3_ManPrint( Exa3_Man_t * p, int i, int iMint, abctime clk )
{
    printf( "Iter%6d : ", i );
    Extra_PrintBinary( stdout, (unsigned *)&iMint, p->nVars );
    printf( "  Var =%5d  ", bmcg_sat_solver_varnum(p->pSat) );
    printf( "Cla =%6d  ", bmcg_sat_solver_clausenum(p->pSat) );
    printf( "Conf =%9d  ", bmcg_sat_solver_conflictnum(p->pSat) );
    Abc_PrintTime( 1, "Time", clk );
}
void Exa3_ManExactSynthesis( Bmc_EsPar_t * pPars )
{
    int i, status, iMint = 1;
    abctime clkTotal = Abc_Clock();
    Exa3_Man_t * p; int fCompl = 0;
    word pTruth[16]; Abc_TtReadHex( pTruth, pPars->pTtStr );
    assert( pPars->nVars <= 10 );
    assert( pPars->nLutSize <= 6 );
    p = Exa3_ManAlloc( pPars, pTruth );
    if ( pTruth[0] & 1 ) { fCompl = 1; Abc_TtNot( pTruth, p->nWords ); }
    status = Exa3_ManAddCnfStart( p, pPars->fOnlyAnd );
    assert( status );
    printf( "Running exact synthesis for %d-input function with %d %d-input LUTs...\n", p->nVars, p->nNodes, p->nLutSize );
    if ( pPars->fUseIncr ) 
    {
        bmcg_sat_solver_set_nvars( p->pSat, p->iVar + p->nNodes*(1 << p->nVars) );
        status = bmcg_sat_solver_solve( p->pSat, NULL, 0 );
        assert( status == GLUCOSE_SAT );
    }
    for ( i = 0; iMint != -1; i++ )
    {
        if ( pPars->fUseIncr ? !Exa3_ManAddCnf2( p, iMint ) : !Exa3_ManAddCnf( p, iMint ) )
            break;
        status = bmcg_sat_solver_solve( p->pSat, NULL, 0 );
        if ( pPars->fVerbose && (!pPars->fUseIncr || i % 100 == 0) )
            Exa3_ManPrint( p, i, iMint, Abc_Clock() - clkTotal );
        if ( status == GLUCOSE_UNSAT || status == GLUCOSE_UNDEC )
            break;
        iMint = Exa3_ManEval( p );
    }
    if ( pPars->fVerbose && status != GLUCOSE_UNDEC )
        Exa3_ManPrint( p, i, iMint, Abc_Clock() - clkTotal );
    if ( iMint == -1 )
        Exa3_ManPrintSolution( p, fCompl );
    else if ( status == GLUCOSE_UNDEC )
        printf( "The problem timed out after %d sec.\n", pPars->RuntimeLim );
    else 
        printf( "The problem has no solution.\n" );
    printf( "Added = %d.  Tried = %d.  ", p->nUsed[1], p->nUsed[0] );
    Exa3_ManFree( p );
    Abc_PrintTime( 1, "Total runtime", Abc_Clock() - clkTotal );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Exa_ManIsNormalized( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut )
{
    int i, Count = 0; word Temp;
    Vec_WrdForEachEntry( vSimsIn, Temp, i )
        if ( Temp & 1 )
            Count++;
    if ( Count )
        printf( "The data for %d divisors are not normalized.\n", Count );
    if ( !(Vec_WrdEntry(vSimsOut, 0) & 1) )
        printf( "The output data is not normalized.\n" );
//    else if ( Count == 0 )
//        printf( "The data is fully normalized.\n" );
}
static inline void Exa_ManPrintFanin( int nIns, int nDivs, int iNode, int fComp )
{
    if ( iNode == 0 )
        printf( " %s", fComp ? "const1" : "const0" );
    else if ( iNode > 0 && iNode <= nIns )
        printf( " %s%c", fComp ? "~" : "", 'a'+iNode-1 );
    else if ( iNode > nIns && iNode < nDivs )
        printf( " %s%c", fComp ? "~" : "", 'A'+iNode-nIns-1 );
    else
        printf( " %s%d", fComp ? "~" : "", iNode );
}
void Exa_ManMiniPrint( Mini_Aig_t * p, int nIns )
{
    int i, nDivs = 1 + Mini_AigPiNum(p), nNodes = Mini_AigAndNum(p);
    printf( "This %d-var function (%d divisors) has %d gates (%d xor) and %d levels:\n", 
        nIns, nDivs, nNodes, Mini_AigXorNum(p), Mini_AigLevelNum(p) );
    for ( i = nDivs + nNodes; i < Mini_AigNodeNum(p); i++ )
    {
        int Lit0 = Mini_AigNodeFanin0( p, i );
        printf( "%2d  = ", i );
        Exa_ManPrintFanin( nIns, nDivs, Abc_Lit2Var(Lit0), Abc_LitIsCompl(Lit0) );
        printf( "\n" );
    }
    for ( i = nDivs + nNodes - 1; i >= nDivs; i-- )
    {
        int Lit0 = Mini_AigNodeFanin0( p, i );
        int Lit1 = Mini_AigNodeFanin1( p, i );
        printf( "%2d  = ", i );
        if ( Lit0 < Lit1 )
        {
            Exa_ManPrintFanin( nIns, nDivs, Abc_Lit2Var(Lit0), Abc_LitIsCompl(Lit0) );
            printf( " &" );
            Exa_ManPrintFanin( nIns, nDivs, Abc_Lit2Var(Lit1), Abc_LitIsCompl(Lit1) );
        }
        else
        {
            Exa_ManPrintFanin( nIns, nDivs, Abc_Lit2Var(Lit1), Abc_LitIsCompl(Lit1) );
            printf( " ^" );
            Exa_ManPrintFanin( nIns, nDivs, Abc_Lit2Var(Lit0), Abc_LitIsCompl(Lit0) );
        }
        printf( "\n" );
    }
}
void Exa_ManMiniVerify( Mini_Aig_t * p, Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut )
{
    extern void Extra_BitMatrixTransposeP( Vec_Wrd_t * vSimsIn, int nWordsIn, Vec_Wrd_t * vSimsOut, int nWordsOut );
    int i, nDivs = 1 + Mini_AigPiNum(p), nNodes = Mini_AigAndNum(p); 
    int k, nOuts = Mini_AigPoNum(p), nErrors = 0; word Outs[6] = {0};
    Vec_Wrd_t * vSimsIn2 = Vec_WrdStart( 64 );
    assert( nOuts <= 6 );
    assert( Vec_WrdSize(vSimsIn) <= 64 );
    assert( Vec_WrdSize(vSimsIn) == Vec_WrdSize(vSimsOut) );
    Vec_WrdFillExtra( vSimsIn, 64, 0 );
    Extra_BitMatrixTransposeP( vSimsIn, 1, vSimsIn2, 1 );
    assert( Mini_AigNodeNum(p) <= 64 );
    for ( i = nDivs; i < nDivs + nNodes; i++ )
    {
        int Lit0 = Mini_AigNodeFanin0( p, i );
        int Lit1 = Mini_AigNodeFanin1( p, i );
        word Sim0 = Vec_WrdEntry( vSimsIn2, Abc_Lit2Var(Lit0) );
        word Sim1 = Vec_WrdEntry( vSimsIn2, Abc_Lit2Var(Lit1) );
        Sim0 = Abc_LitIsCompl(Lit0) ? ~Sim0 : Sim0;
        Sim1 = Abc_LitIsCompl(Lit1) ? ~Sim1 : Sim1;
        Vec_WrdWriteEntry( vSimsIn2, i, Lit0 < Lit1 ? Sim0 & Sim1 : Sim0 ^ Sim1 );
    }
    for ( i = nDivs + nNodes; i < Mini_AigNodeNum(p); i++ )
    {
        int Lit0 = Mini_AigNodeFanin0( p, i );
        word Sim0 = Vec_WrdEntry( vSimsIn2, Abc_Lit2Var(Lit0) );
        Outs[i - (nDivs + nNodes)] = Abc_LitIsCompl(Lit0) ? ~Sim0 : Sim0;
    }
    Vec_WrdFree( vSimsIn2 );
    for ( i = 0; i < Vec_WrdSize(vSimsOut); i++ )
    {
        int iOutMint = 0;
        for ( k = 0; k < nOuts; k++ )
            if ( (Outs[k] >> i) & 1 )
                iOutMint |= 1 << k;
        nErrors += !Abc_TtGetBit(Vec_WrdEntryP(vSimsOut, i), iOutMint);
    }
    if ( nErrors == 0 )
        printf( "Verification successful.  " );
    else 
        printf( "Verification failed for %d (out of %d) minterms.\n", nErrors, Vec_WrdSize(vSimsOut) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Exa4_ManParse( char * pFileName )
{
    Vec_Int_t * vRes = NULL;
    char * pToken, pBuffer[1000];
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        Abc_Print( -1, "Cannot open file \"%s\".\n", pFileName );
        return NULL;
    }
    while ( fgets( pBuffer, 1000, pFile ) != NULL )
    {
        if ( pBuffer[0] == 's' )
        {
            if ( strncmp(pBuffer+2, "SAT", 3) )
                break;
            assert( vRes == NULL );
            vRes = Vec_IntAlloc( 100 );
        }
        else if ( pBuffer[0] == 'v' )
        {
            pToken = strtok( pBuffer+1, " \n\r\t" );
            while ( pToken )
            {
                int Token = atoi(pToken);
                if ( Token == 0 )
                    break;
                Vec_IntSetEntryFull( vRes, Abc_AbsInt(Token), Token > 0 );
                pToken = strtok( NULL, " \n\r\t" );
            }
        }
        else if ( pBuffer[0] != 'c' )
            assert( 0 );
    }
    fclose( pFile );
    unlink( pFileName );
    return vRes;
}
Vec_Int_t * Exa4_ManSolve( char * pFileNameIn, char * pFileNameOut, int TimeOut, int fVerbose )
{
    int fVerboseSolver = 0;
    abctime clkTotal = Abc_Clock();
    Vec_Int_t * vRes = NULL;
#ifdef _WIN32
    char * pKissat = "kissat.exe";
#else
    char * pKissat = "kissat";
#endif
    char Command[1000], * pCommand = (char *)&Command;
    if ( TimeOut )
        sprintf( pCommand, "%s --time=%d %s %s > %s", pKissat, TimeOut, fVerboseSolver ? "": "-q", pFileNameIn, pFileNameOut );
    else
        sprintf( pCommand, "%s %s %s > %s", pKissat, fVerboseSolver ? "": "-q", pFileNameIn, pFileNameOut );
    if ( system( pCommand ) == -1 )
    {
        fprintf( stdout, "Command \"%s\" did not succeed.\n", pCommand );
        return 0;
    }
    vRes = Exa4_ManParse( pFileNameOut );
    if ( fVerbose )
    {
        if ( vRes )
            printf( "The problem has a solution. " );
        else if ( vRes == NULL && TimeOut == 0 )
            printf( "The problem has no solution. " );
        else if ( vRes == NULL )
            printf( "The problem has no solution or timed out after %d sec. ", TimeOut );
        Abc_PrintTime( 1, "SAT solver time", Abc_Clock() - clkTotal );
    }
    return vRes;
}


typedef struct Exa4_Man_t_ Exa4_Man_t;
struct Exa4_Man_t_ 
{
    Vec_Wrd_t *       vSimsIn;   // input signatures  (nWords = 1, nIns <= 64)
    Vec_Wrd_t *       vSimsOut;  // output signatures (nWords = 1, nOuts <= 6)
    int               fVerbose; 
    int               nIns;     
    int               nDivs;     // divisors (const + inputs + tfi + sidedivs)
    int               nNodes;
    int               nOuts; 
    int               nObjs;
    int               VarMarks[MAJ_NOBJS][2][MAJ_NOBJS];
    int               nCnfVars; 
    int               nCnfClauses;
    FILE *            pFile;
};

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Exa4_ManMarkup( Exa4_Man_t * p )
{
    int i, k, j, nVars[3] = {1 + 5*p->nNodes, 0, 3*p->nNodes*Vec_WrdSize(p->vSimsIn)};
    assert( p->nObjs <= MAJ_NOBJS );
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
        for ( k = 0; k < 2; k++ )
            for ( j = 1+!k; j < i-k; j++ )
                p->VarMarks[i][k][j] = nVars[0] + nVars[1]++;
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
        for ( j = p->nOuts == 1 ? p->nDivs + p->nNodes - 1 : 0; j < p->nDivs + p->nNodes; j++ )
            p->VarMarks[i][0][j] = nVars[0] + nVars[1]++;
    if ( p->fVerbose )
        printf( "Variables:  Function = %d.  Structure = %d.  Internal = %d.  Total = %d.\n", 
            nVars[0], nVars[1], nVars[2], nVars[0] + nVars[1] + nVars[2] );
    if ( 0 )
    {
        for ( j = 0; j < 2; j++ )
        {
            printf( "   : " );
            for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
            {
                for ( k = 0; k < 2; k++ )
                    printf( "%3d ", j ? k : i );
                printf( " " );
            }
            printf( " " );
            for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
            {
                printf( "%3d ", j ? 0 : i );
                printf( " " );
            }
            printf( "\n" );
        }
        for ( j = 0; j < 5 + p->nNodes*9 + p->nOuts*5; j++ )
            printf( "-" );
        printf( "\n" );
        for ( j = p->nObjs - 2; j >= 0; j-- )
        {
            if ( j > 0 && j <= p->nIns )
                printf( " %c : ", 'a'+j-1 );
            else
                printf( "%2d : ", j );
            for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
            {
                for ( k = 0; k < 2; k++ )
                    printf( "%3d ", p->VarMarks[i][k][j] );
                printf( " " );
            }
            printf( " " );
            for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
            {
                printf( "%3d ", p->VarMarks[i][0][j] );
                printf( " " );
            }
            printf( "\n" );
        }
    }
    return nVars[0] + nVars[1] + nVars[2];
}
Exa4_Man_t * Exa4_ManAlloc( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, int fVerbose )
{
    Exa4_Man_t * p = ABC_CALLOC( Exa4_Man_t, 1 );
    assert( Vec_WrdSize(vSimsIn) == Vec_WrdSize(vSimsOut) );
    p->vSimsIn     = vSimsIn;
    p->vSimsOut    = vSimsOut;
    p->fVerbose    = fVerbose;
    p->nIns        = nIns;  
    p->nDivs       = nDivs; 
    p->nNodes      = nNodes;
    p->nOuts       = nOuts; 
    p->nObjs       = p->nDivs + p->nNodes + p->nOuts;
    p->nCnfVars    = Exa4_ManMarkup( p );
    return p;
}
void Exa4_ManFree( Exa4_Man_t * p )
{
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Exa4_ManAddClause( Exa4_Man_t * p, int * pLits, int nLits )
{
    int i, k = 0;
    for ( i = 0; i < nLits; i++ )
        if ( pLits[i] == 1 )
            return 0;
        else if ( pLits[i] == 0 )
            continue;
        else if ( pLits[i] <= 2*p->nCnfVars )
            pLits[k++] = pLits[i];
        else assert( 0 );
    nLits = k;
    assert( nLits > 0 );
    if ( p->pFile )
    {
        p->nCnfClauses++;
        for ( i = 0; i < nLits; i++ )
            fprintf( p->pFile, "%s%d ", Abc_LitIsCompl(pLits[i]) ? "-" : "", Abc_Lit2Var(pLits[i]) );
        fprintf( p->pFile, "0\n" );
    }
    if ( 0 )
    {
        for ( i = 0; i < nLits; i++ )
            fprintf( stdout, "%s%d ", Abc_LitIsCompl(pLits[i]) ? "-" : "", Abc_Lit2Var(pLits[i]) );
        fprintf( stdout, "\n" );
    }
    return 1;
}
static inline int Exa4_ManAddClause4( Exa4_Man_t * p, int Lit0, int Lit1, int Lit2, int Lit3 )
{
    int pLits[4] = { Lit0, Lit1, Lit2, Lit3 };
    return Exa4_ManAddClause( p, pLits, 4 );
}
int Exa4_ManGenStart( Exa4_Man_t * p, int fOnlyAnd, int fFancy, int fOrderNodes, int fUniqFans )
{
    int pLits[2*MAJ_NOBJS], i, j, k, n, m, nLits;
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
    {
        int iVarStart = 1 + 5*(i - p->nDivs);//
        for ( k = 0; k < 2; k++ )
        {
            nLits = 0;
            for ( j = 0; j < p->nObjs; j++ )
                if ( p->VarMarks[i][k][j] )
                    pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][k][j], 0 );
            assert( nLits > 0 );
            Exa4_ManAddClause( p, pLits, nLits );
            for ( n = 0;   n < nLits; n++ )
            for ( m = n+1; m < nLits; m++ )
                Exa4_ManAddClause4( p, Abc_LitNot(pLits[n]), Abc_LitNot(pLits[m]), 0, 0 );
            if ( k == 1 )
                break;
            for ( j = 0; j < p->nObjs; j++ ) if ( p->VarMarks[i][0][j] )
            for ( n = j; n < p->nObjs; n++ ) if ( p->VarMarks[i][1][n] )
                Exa4_ManAddClause4( p, Abc_Var2Lit(p->VarMarks[i][0][j], 1), Abc_Var2Lit(p->VarMarks[i][1][n], 1), 0, 0 );
        }
        if ( fOrderNodes )
        for ( j = p->nDivs; j < i; j++ )
        for ( n = 0;   n < p->nObjs; n++ ) if ( p->VarMarks[i][0][n] )
        for ( m = n+1; m < p->nObjs; m++ ) if ( p->VarMarks[j][0][m] )
            Exa4_ManAddClause4( p, Abc_Var2Lit(p->VarMarks[i][0][n], 1), Abc_Var2Lit(p->VarMarks[j][0][m], 1), 0, 0 );
        for ( j = p->nDivs; j < i; j++ )
        for ( k = 0; k < 2; k++ )        if ( p->VarMarks[i][k][j] )
        for ( n = 0; n < p->nObjs; n++ ) if ( p->VarMarks[i][!k][n] )
        for ( m = 0; m < 2; m++ )        if ( p->VarMarks[j][m][n] )
            Exa4_ManAddClause4( p, Abc_Var2Lit(p->VarMarks[i][k][j], 1), Abc_Var2Lit(p->VarMarks[i][!k][n], 1), Abc_Var2Lit(p->VarMarks[j][m][n], 1), 0 );
        if ( fFancy )
        {
            nLits = 0;
            for ( k = 0; k < 5-fOnlyAnd; k++ )
                pLits[nLits++] = Abc_Var2Lit( iVarStart+k, 0 );
            Exa4_ManAddClause( p, pLits, nLits );
            for ( n = 0;   n < nLits; n++ )
            for ( m = n+1; m < nLits; m++ )
                Exa4_ManAddClause4( p, Abc_LitNot(pLits[n]), Abc_LitNot(pLits[m]), 0, 0 );
        }
        else
        {
            for ( k = 0; k < 3; k++ )
                Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart, k==1), Abc_Var2Lit(iVarStart+1, k==2), Abc_Var2Lit(iVarStart+2, k!=0), 0);
            if ( fOnlyAnd )
                Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart, 1), Abc_Var2Lit(iVarStart+1, 1), Abc_Var2Lit(iVarStart+2, 0), 0);
        }
    }
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
    {
        nLits = 0;
        for ( k = 0; k < 2; k++ )
            for ( j = i+1; j < p->nObjs; j++ )
                if ( p->VarMarks[j][k][i] )
                    pLits[nLits++] = Abc_Var2Lit( p->VarMarks[j][k][i], 0 );
        Exa4_ManAddClause( p, pLits, nLits );
        if ( fUniqFans )
            for ( n = 0;   n < nLits; n++ )
            for ( m = n+1; m < nLits; m++ )
                Exa4_ManAddClause4( p, Abc_LitNot(pLits[n]), Abc_LitNot(pLits[m]), 0, 0 );
    }
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
    {
        nLits = 0;
        for ( j = 0; j < p->nDivs + p->nNodes; j++ ) if ( p->VarMarks[i][0][j] )
            pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][0][j], 0 );
        Exa4_ManAddClause( p, pLits, nLits );
        for ( n = 0;   n < nLits; n++ )
        for ( m = n+1; m < nLits; m++ )
            Exa4_ManAddClause4( p, Abc_LitNot(pLits[n]), Abc_LitNot(pLits[m]), 0, 0 );
    }
    return 1;
}
void Exa4_ManGenMint( Exa4_Man_t * p, int iMint, int fOnlyAnd, int fFancy )
{
    int iNodeVar = p->nCnfVars + 3*p->nNodes*(iMint - Vec_WrdSize(p->vSimsIn));
    int iOutMint = Abc_Tt6FirstBit( Vec_WrdEntry(p->vSimsOut, iMint) );
    int i, k, n, j, VarVals[MAJ_NOBJS];
    assert( p->nObjs <= MAJ_NOBJS );
    assert( iMint < Vec_WrdSize(p->vSimsIn) );
    for ( i = 0; i < p->nDivs; i++ )
        VarVals[i] = (Vec_WrdEntry(p->vSimsIn, iMint) >> i) & 1;
    for ( i = 0; i < p->nNodes; i++ )
        VarVals[p->nDivs + i] = Abc_Var2Lit(iNodeVar + 3*i + 2, 0);
    for ( i = 0; i < p->nOuts; i++ )
        VarVals[p->nDivs + p->nNodes + i] = (iOutMint >> i) & 1;
    if ( 0 )
    {
        printf( "Adding minterm %d: ", iMint );
        for ( i = 0; i < p->nObjs; i++ )
            printf( " %d=%d", i, VarVals[i] );
        printf( "\n" );
    }
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
    {
        int iVarStart = 1 + 5*(i - p->nDivs);//
        int iBaseVarI = iNodeVar + 3*(i - p->nDivs);
        for ( k = 0; k < 2; k++ )
        for ( j = 0; j < i; j++ ) if ( p->VarMarks[i][k][j] )
        for ( n = 0; n < 2; n++ )
            Exa4_ManAddClause4( p, Abc_Var2Lit(p->VarMarks[i][k][j], 1), Abc_Var2Lit(iBaseVarI + k, n), Abc_LitNotCond(VarVals[j], !n), 0);
        if ( fFancy )
        {
            //  Clauses for a*b = c
            //  a + ~c
            //  b + ~c
            // ~a + ~b + c
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 0, 1), Abc_Var2Lit(iBaseVarI + 0, 0), 0,                             Abc_Var2Lit(iBaseVarI + 2, 1) );
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 0, 1), 0,                             Abc_Var2Lit(iBaseVarI + 1, 0), Abc_Var2Lit(iBaseVarI + 2, 1) );
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 0, 1), Abc_Var2Lit(iBaseVarI + 0, 1), Abc_Var2Lit(iBaseVarI + 1, 1), Abc_Var2Lit(iBaseVarI + 2, 0) );

            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 1, 1), Abc_Var2Lit(iBaseVarI + 0, 1), 0,                             Abc_Var2Lit(iBaseVarI + 2, 1) );
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 1, 1), 0,                             Abc_Var2Lit(iBaseVarI + 1, 0), Abc_Var2Lit(iBaseVarI + 2, 1) );
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 1, 1), Abc_Var2Lit(iBaseVarI + 0, 0), Abc_Var2Lit(iBaseVarI + 1, 1), Abc_Var2Lit(iBaseVarI + 2, 0) );

            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 2, 1), Abc_Var2Lit(iBaseVarI + 0, 0), 0,                             Abc_Var2Lit(iBaseVarI + 2, 1) );
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 2, 1), 0,                             Abc_Var2Lit(iBaseVarI + 1, 1), Abc_Var2Lit(iBaseVarI + 2, 1) );
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 2, 1), Abc_Var2Lit(iBaseVarI + 0, 1), Abc_Var2Lit(iBaseVarI + 1, 0), Abc_Var2Lit(iBaseVarI + 2, 0) );
            //  Clauses for a+b = c
            // ~a +  c
            // ~b +  c
            //  a +  b + ~c
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 3, 1), Abc_Var2Lit(iBaseVarI + 0, 1), 0,                             Abc_Var2Lit(iBaseVarI + 2, 0) );
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 3, 1), 0,                             Abc_Var2Lit(iBaseVarI + 1, 1), Abc_Var2Lit(iBaseVarI + 2, 0) );
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 3, 1), Abc_Var2Lit(iBaseVarI + 0, 0), Abc_Var2Lit(iBaseVarI + 1, 0), Abc_Var2Lit(iBaseVarI + 2, 1) );
            //  Clauses for a^b = c
            // ~a + ~b + ~c
            // ~a +  b +  c
            //  a + ~b +  c
            //  a +  b + ~c
            if ( !fOnlyAnd )
            {
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 4, 1), Abc_Var2Lit(iBaseVarI + 0, 1), Abc_Var2Lit(iBaseVarI + 1, 1), Abc_Var2Lit(iBaseVarI + 2, 1) );
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 4, 1), Abc_Var2Lit(iBaseVarI + 0, 1), Abc_Var2Lit(iBaseVarI + 1, 0), Abc_Var2Lit(iBaseVarI + 2, 0) );
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 4, 1), Abc_Var2Lit(iBaseVarI + 0, 0), Abc_Var2Lit(iBaseVarI + 1, 1), Abc_Var2Lit(iBaseVarI + 2, 0) );
            Exa4_ManAddClause4( p, Abc_Var2Lit(iVarStart + 4, 1), Abc_Var2Lit(iBaseVarI + 0, 0), Abc_Var2Lit(iBaseVarI + 1, 0), Abc_Var2Lit(iBaseVarI + 2, 1) );
            }
        }
        else
        {
            for ( k = 0; k < 4; k++ )
            for ( n = 0; n < 2; n++ )
                Exa4_ManAddClause4( p, Abc_Var2Lit(iBaseVarI + 0, k&1), Abc_Var2Lit(iBaseVarI + 1, k>>1), Abc_Var2Lit(iBaseVarI + 2, !n), Abc_Var2Lit(k ? iVarStart + k-1 : 0, n));
        }
    }
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
    {
        for ( j = 0; j < p->nDivs + p->nNodes; j++ ) if ( p->VarMarks[i][0][j] )
        for ( n = 0; n < 2; n++ )
            Exa4_ManAddClause4( p, Abc_Var2Lit(p->VarMarks[i][0][j], 1), Abc_LitNotCond(VarVals[j], n), Abc_LitNotCond(VarVals[i], !n), 0);
    }
}
void Exa4_ManGenCnf( Exa4_Man_t * p, char * pFileName, int fOnlyAnd, int fFancy, int fOrderNodes, int fUniqFans )
{
    int m;
    assert( p->pFile == NULL );
    p->pFile = fopen( pFileName, "wb" );
    fputs( "p cnf                \n", p->pFile );
    Exa4_ManGenStart( p, fOnlyAnd, fFancy, fOrderNodes, fUniqFans );
    for ( m = 1; m < Vec_WrdSize(p->vSimsIn); m++ )
        Exa4_ManGenMint( p, m, fOnlyAnd, fFancy );
    rewind( p->pFile );
    fprintf( p->pFile, "p cnf %d %d", p->nCnfVars, p->nCnfClauses );
    fclose( p->pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Exa4_ManFindFanin( Exa4_Man_t * p, Vec_Int_t * vValues, int i, int k )
{
    int j, Count = 0, iVar = -1;
    for ( j = 0; j < p->nObjs; j++ )
        if ( p->VarMarks[i][k][j] && Vec_IntEntry(vValues, p->VarMarks[i][k][j]) )
        {
            iVar = j;
            Count++;
        }
    assert( Count == 1 );
    return iVar;
}
static inline void Exa4_ManPrintFanin( Exa4_Man_t * p, int iNode, int fComp )
{
    if ( iNode == 0 )
        printf( " %s", fComp ? "const1" : "const0" );
    else if ( iNode > 0 && iNode <= p->nIns )
        printf( " %s%c", fComp ? "~" : "", 'a'+iNode-1 );
    else if ( iNode > p->nIns && iNode < p->nDivs )
        printf( " %s%c", fComp ? "~" : "", 'A'+iNode-p->nIns-1 );
    else
        printf( " %s%d", fComp ? "~" : "", iNode );
}
void Exa4_ManPrintSolution( Exa4_Man_t * p, Vec_Int_t * vValues, int fFancy )
{
    int i, k;
    printf( "Circuit for %d-var function with %d divisors contains %d two-input gates:\n", p->nIns, p->nDivs, p->nNodes );
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
    {
        int iVar = Exa4_ManFindFanin( p, vValues, i, 0 );
        printf( "%2d  = ", i );
        Exa4_ManPrintFanin( p, iVar, 0 );
        printf( "\n" );
    }
    for ( i = p->nDivs + p->nNodes - 1; i >= p->nDivs; i-- )
    {
        int iVarStart = 1 + 5*(i - p->nDivs);//
        if ( fFancy )
        {
            int Val1 = Vec_IntEntry(vValues, iVarStart);
            int Val2 = Vec_IntEntry(vValues, iVarStart+1);
            int Val3 = Vec_IntEntry(vValues, iVarStart+2);
            int Val4 = Vec_IntEntry(vValues, iVarStart+3);
            //int Val5 = Vec_IntEntry(vValues, iVarStart+4);
            printf( "%2d  = ", i );
            for ( k = 0; k < 2; k++ )
            {
                int iNode = Exa4_ManFindFanin( p, vValues, i, !k );
                int fComp = k ? Val1 | Val3 : Val2 | Val3;
                Exa4_ManPrintFanin( p, iNode, fComp );
                if ( k ) break;
                printf( " %c", (Val1 || Val2 || Val3) ? '&' : (Val4 ? '|' : '^') );
            }
        }
        else
        {
            int Val1 = Vec_IntEntry(vValues, iVarStart);
            int Val2 = Vec_IntEntry(vValues, iVarStart+1);
            int Val3 = Vec_IntEntry(vValues, iVarStart+2);
            printf( "%2d  = ", i );
            for ( k = 0; k < 2; k++ )
            {
                int iNode = Exa4_ManFindFanin( p, vValues, i, !k );
                int fComp = k ? !Val1 && Val2 && !Val3 : Val1 && !Val2 && !Val3;
                Exa4_ManPrintFanin( p, iNode, fComp );
                if ( k ) break;
                printf( " %c", (Val1 && Val2) ? (Val3 ? '|' : '^') : '&' );
            }
        }
        printf( "\n" );
    }
}
Mini_Aig_t * Exa4_ManMiniAig( Exa4_Man_t * p, Vec_Int_t * vValues, int fFancy )
{
    int i, k, Compl[MAJ_NOBJS] = {0};
    Mini_Aig_t * pMini = Mini_AigStartSupport( p->nDivs-1, p->nObjs );
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
    {
        int iNodes[2] = {0};
        int iVarStart = 1 + 5*(i - p->nDivs);//
        if ( fFancy )
        {
            int Val1 = Vec_IntEntry(vValues, iVarStart);
            int Val2 = Vec_IntEntry(vValues, iVarStart+1);
            int Val3 = Vec_IntEntry(vValues, iVarStart+2);
            int Val4 = Vec_IntEntry(vValues, iVarStart+3);
            int Val5 = Vec_IntEntry(vValues, iVarStart+4);
            for ( k = 0; k < 2; k++ )
            {
                int iNode = Exa4_ManFindFanin( p, vValues, i, !k );
                int fComp = k ? Val1 | Val3 : Val2 | Val3;
                iNodes[k] = Abc_Var2Lit(iNode, fComp ^ Compl[iNode]);
            }
            if ( Val1 || Val2 || Val3 ) 
                Mini_AigAnd( pMini, iNodes[0], iNodes[1] );
            else
            {
                if ( Val4 )
                    Mini_AigOr( pMini, iNodes[0], iNodes[1] );
                else if ( Val5 )
                    Mini_AigXorSpecial( pMini, iNodes[0], iNodes[1] );
                else assert( 0 );
            }
        }
        else
        {
            int Val1 = Vec_IntEntry(vValues, iVarStart);
            int Val2 = Vec_IntEntry(vValues, iVarStart+1);
            int Val3 = Vec_IntEntry(vValues, iVarStart+2);
            Compl[i] = Val1 && Val2 && Val3;
            for ( k = 0; k < 2; k++ )
            {
                int iNode = Exa4_ManFindFanin( p, vValues, i, !k );
                int fComp = k ? !Val1 && Val2 && !Val3 : Val1 && !Val2 && !Val3;
                iNodes[k] = Abc_Var2Lit(iNode, fComp ^ Compl[iNode]);
            }
            if ( Val1 && Val2 ) 
            {
                if ( Val3 )
                    Mini_AigOr( pMini, iNodes[0], iNodes[1] );
                else
                    Mini_AigXorSpecial( pMini, iNodes[0], iNodes[1] );
            }
            else
                Mini_AigAnd( pMini, iNodes[0], iNodes[1] );
        }
    }
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
    {
        int iVar = Exa4_ManFindFanin( p, vValues, i, 0 );
        Mini_AigCreatePo( pMini, Abc_Var2Lit(iVar, Compl[iVar]) );
    }
    assert( p->nObjs == Mini_AigNodeNum(pMini) );
    return pMini;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mini_Aig_t * Exa4_ManGenTest( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, int TimeOut, int fOnlyAnd, int fFancy, int fOrderNodes, int fUniqFans, int fVerbose )
{
    Mini_Aig_t * pMini = NULL;
    abctime clkTotal = Abc_Clock();
    Vec_Int_t * vValues = NULL;
    char * pFileNameIn  = "_temp_.cnf";
    char * pFileNameOut = "_temp_out.cnf";
    Exa4_Man_t * p = Exa4_ManAlloc( vSimsIn, vSimsOut, nIns, nDivs, nOuts, nNodes, fVerbose );
    Exa_ManIsNormalized( vSimsIn, vSimsOut );
    Exa4_ManGenCnf( p, pFileNameIn, fOnlyAnd, fFancy, fOrderNodes, fUniqFans );
    if ( fVerbose )
        printf( "Timeout = %d. OnlyAnd = %d. Fancy = %d. OrderNodes = %d. UniqueFans = %d. Verbose = %d.\n", TimeOut, fOnlyAnd, fFancy, fOrderNodes, fUniqFans, fVerbose );
    if ( fVerbose )
        printf( "CNF with %d variables and %d clauses was dumped into file \"%s\".\n", p->nCnfVars, p->nCnfClauses, pFileNameIn );
    vValues = Exa4_ManSolve( pFileNameIn, pFileNameOut, TimeOut, fVerbose );
    if ( vValues ) pMini = Exa4_ManMiniAig( p, vValues, fFancy );
    //if ( vValues ) Exa4_ManPrintSolution( p, vValues, fFancy );
    if ( vValues ) Exa_ManMiniPrint( pMini, p->nIns );
    if ( vValues ) Exa_ManMiniVerify( pMini, vSimsIn, vSimsOut );
    Vec_IntFreeP( &vValues );
    Exa4_ManFree( p );
    Abc_PrintTime( 1, "Total runtime", Abc_Clock() - clkTotal );
    return pMini;
}
void Exa_ManExactSynthesis4_( Bmc_EsPar_t * pPars )
{
    Mini_Aig_t * pMini = NULL;
    int i, m;
    Vec_Wrd_t * vSimsIn  = Vec_WrdStart( 8 );
    Vec_Wrd_t * vSimsOut = Vec_WrdStart( 8 );
    int Truths[2] = { 0x96, 0xE8 };
    for ( m = 0; m < 8; m++ )
    {
        int iOutMint = 0;
        for ( i = 0; i < 2; i++ )
            if ( (Truths[i] >> m) & 1 )
                iOutMint |= 1 << i;
        Abc_TtSetBit( Vec_WrdEntryP(vSimsOut, m), iOutMint );
        for ( i = 0; i < 3; i++ )
            if ( (m >> i) & 1 )
                Abc_TtSetBit( Vec_WrdEntryP(vSimsIn, m), 1+i );
    }
    pMini = Exa4_ManGenTest( vSimsIn, vSimsOut, 3, 4, 2, pPars->nNodes, pPars->RuntimeLim, pPars->fOnlyAnd, pPars->fFewerVars, pPars->fOrderNodes, pPars->fUniqFans, pPars->fVerbose );
    if ( pMini ) Mini_AigStop( pMini );
    Vec_WrdFree( vSimsIn );
    Vec_WrdFree( vSimsOut );
}
void Exa_ManExactSynthesis4( Bmc_EsPar_t * pPars )
{
    Mini_Aig_t * pMini = NULL;
    int i, m, nMints = 1 << pPars->nVars, fCompl = 0;
    Vec_Wrd_t * vSimsIn  = Vec_WrdStart( nMints );
    Vec_Wrd_t * vSimsOut = Vec_WrdStart( nMints );
    word pTruth[16]; Abc_TtReadHex( pTruth, pPars->pTtStr );
    if ( pTruth[0] & 1 ) { fCompl = 1; Abc_TtNot( pTruth, Abc_TtWordNum(pPars->nVars) ); }
    assert( pPars->nVars <= 10 );
    for ( m = 0; m < nMints; m++ )
    {
        Abc_TtSetBit( Vec_WrdEntryP(vSimsOut, m), Abc_TtGetBit(pTruth, m) );
        for ( i = 0; i < pPars->nVars; i++ )
            if ( (m >> i) & 1 )
                Abc_TtSetBit( Vec_WrdEntryP(vSimsIn, m), 1+i );
    }
    assert( Vec_WrdSize(vSimsIn) == (1 << pPars->nVars) );
    pMini = Exa4_ManGenTest( vSimsIn, vSimsOut, pPars->nVars, 1+pPars->nVars, 1, pPars->nNodes, pPars->RuntimeLim, pPars->fOnlyAnd, pPars->fFewerVars, pPars->fOrderNodes, pPars->fUniqFans, pPars->fVerbose );
    if ( pMini ) Mini_AigStop( pMini );
    if ( fCompl ) printf( "The resulting circuit, if computed, will be complemented.\n" );
    Vec_WrdFree( vSimsIn );
    Vec_WrdFree( vSimsOut );
}



typedef struct Exa5_Man_t_ Exa5_Man_t;
struct Exa5_Man_t_ 
{
    Vec_Wrd_t *       vSimsIn;  
    Vec_Wrd_t *       vSimsOut; 
    int               fVerbose; 
    int               nIns;     
    int               nDivs;   // divisors (const + inputs + tfi + sidedivs)   
    int               nNodes;   
    int               nOuts;    
    int               nObjs;    
    int               VarMarks[MAJ_NOBJS][MAJ_NOBJS];
    int               nCnfVars;
    int               nCnfClauses;
    FILE *            pFile;
    Vec_Int_t *       vFans;
};


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Exa5_ManMarkup( Exa5_Man_t * p )
{
    int i, j, k, nVars[3] = {1 + 3*p->nNodes, 0, p->nNodes*Vec_WrdSize(p->vSimsIn)};
    assert( p->nObjs <= MAJ_NOBJS );
    Vec_IntFill( p->vFans, 1 + 3*p->nNodes, 0 );
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
    for ( j = 2; j < i; j++ )
    {
        p->VarMarks[i][j] = nVars[0] + nVars[1];
        Vec_IntPush( p->vFans, 0 );
        for ( k = 1; k < j; k++ )
            Vec_IntPush( p->vFans, (i << 16) | (j << 8) | k );
        nVars[1] += j;
    }
    assert( Vec_IntSize(p->vFans) == nVars[0] + nVars[1] );
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
        for ( j = p->nOuts == 1 ? p->nDivs + p->nNodes - 1 : 0; j < p->nDivs + p->nNodes; j++ )
            p->VarMarks[i][j] = nVars[0] + nVars[1]++;
    if ( p->fVerbose )
        printf( "Variables:  Function = %d.  Structure = %d.  Internal = %d.  Total = %d.\n", 
            nVars[0], nVars[1], nVars[2], nVars[0] + nVars[1] + nVars[2] );
    if ( 0 )
    {
        {
            printf( "   : " );
            for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
            {
                printf( "%3d ", i );
                printf( " " );
            }
            printf( " " );
            for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
            {
                printf( "%3d ", i );
                printf( " " );
            }
            printf( "\n" );
        }
        for ( j = 0; j < 5 + p->nNodes*5 + p->nOuts*5; j++ )
            printf( "-" );
        printf( "\n" );
        for ( j = p->nObjs - 2; j >= 0; j-- )
        {
            if ( j > 0 && j <= p->nIns )
                printf( " %c : ", 'a'+j-1 );
            else
                printf( "%2d : ", j );
            for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
            {
                printf( "%3d ", p->VarMarks[i][j] );
                printf( " " );
            }
            printf( " " );
            for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
            {
                printf( "%3d ", p->VarMarks[i][j] );
                printf( " " );
            }
            printf( "\n" );
        }
    }
    return nVars[0] + nVars[1] + nVars[2];
}
Exa5_Man_t * Exa5_ManAlloc( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, int fVerbose )
{
    Exa5_Man_t * p = ABC_CALLOC( Exa5_Man_t, 1 );
    assert( Vec_WrdSize(vSimsIn) == Vec_WrdSize(vSimsOut) );
    p->vSimsIn     = vSimsIn;
    p->vSimsOut    = vSimsOut;
    p->fVerbose    = fVerbose;
    p->nIns        = nIns;  
    p->nDivs       = nDivs;
    p->nNodes      = nNodes;
    p->nOuts       = nOuts; 
    p->nObjs       = p->nDivs + p->nNodes + p->nOuts;
    p->vFans       = Vec_IntAlloc( 5000 );
    p->nCnfVars    = Exa5_ManMarkup( p );
    return p;
}
void Exa5_ManFree( Exa5_Man_t * p )
{
    Vec_IntFree( p->vFans );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Exa5_ManAddClause( Exa5_Man_t * p, int * pLits, int nLits )
{
    int i, k = 0;
    for ( i = 0; i < nLits; i++ )
        if ( pLits[i] == 1 )
            return 0;
        else if ( pLits[i] == 0 )
            continue;
        else if ( pLits[i] <= 2*p->nCnfVars )
            pLits[k++] = pLits[i];
        else assert( 0 );
    nLits = k;
    assert( nLits > 0 );
    if ( p->pFile )
    {
        p->nCnfClauses++;
        for ( i = 0; i < nLits; i++ )
            fprintf( p->pFile, "%s%d ", Abc_LitIsCompl(pLits[i]) ? "-" : "", Abc_Lit2Var(pLits[i]) );
        fprintf( p->pFile, "0\n" );
    }
    if ( 0 )
    {
        for ( i = 0; i < nLits; i++ )
            fprintf( stdout, "%s%d ", Abc_LitIsCompl(pLits[i]) ? "-" : "", Abc_Lit2Var(pLits[i]) );
        fprintf( stdout, "\n" );
    }
    return 1;
}
static inline int Exa5_ManAddClause4( Exa5_Man_t * p, int Lit0, int Lit1, int Lit2, int Lit3, int Lit4 )
{
    int pLits[5] = { Lit0, Lit1, Lit2, Lit3, Lit4 };
    return Exa5_ManAddClause( p, pLits, 5 );
}
static inline void Exa5_ManAddOneHot( Exa5_Man_t * p, int * pLits, int nLits )
{
    int n, m;
    for ( n = 0;   n < nLits; n++ )
    for ( m = n+1; m < nLits; m++ )
        Exa5_ManAddClause4( p, Abc_LitNot(pLits[n]), Abc_LitNot(pLits[m]), 0, 0, 0 );
}
static inline void Exa5_ManAddGroup( Exa5_Man_t * p, int iVar, int nVars )
{
    int i, pLits[MAJ_NOBJS];
    assert( nVars+1 <= MAJ_NOBJS );
    pLits[0] = Abc_Var2Lit( iVar, 1 );
    for ( i = 1; i <= nVars; i++ )
        pLits[i] = Abc_Var2Lit( iVar+i, 0 );
    Exa5_ManAddClause( p, pLits, nVars+1 );
    Exa5_ManAddOneHot( p, pLits+1, nVars );
    for ( i = 1; i <= nVars; i++ )
        Exa5_ManAddClause4( p, Abc_LitNot(pLits[0]), Abc_LitNot(pLits[i]), 0, 0, 0 );
}
int Exa5_ManGenStart( Exa5_Man_t * p, int fOnlyAnd, int fFancy, int fOrderNodes, int fUniqFans )
{
    Vec_Int_t * vArray = Vec_IntAlloc( 100 );
    int pLits[MAJ_NOBJS], i, j, k, n, m, nLits, iObj;
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
    {
        int iVarStart = 1 + 3*(i - p->nDivs);//
        nLits = 0;
        for ( j = 0; j < i; j++ ) if ( p->VarMarks[i][j] )
            Exa5_ManAddGroup( p, p->VarMarks[i][j], j-1 ), pLits[nLits++] = Abc_Var2Lit(p->VarMarks[i][j], 0);
        Exa5_ManAddClause( p, pLits, nLits );
        Exa5_ManAddOneHot( p, pLits, nLits );
        if ( fOrderNodes )
        for ( j = p->nDivs; j < i; j++ )
        for ( n = 0;   n < p->nObjs; n++ ) if ( p->VarMarks[i][n] )
        for ( m = n+1; m < p->nObjs; m++ ) if ( p->VarMarks[j][m] )
            Exa5_ManAddClause4( p, Abc_Var2Lit(p->VarMarks[i][n], 1), Abc_Var2Lit(p->VarMarks[j][m], 1), 0, 0, 0 );
        for ( j = p->nDivs; j < i; j++ ) if ( p->VarMarks[i][j] )
        {
            // go through all variables of i that point to j and k
            for ( n = 1; n < j; n++ )
            {
                int iVar   = p->VarMarks[i][j] + n; // variable of i pointing to j
                int iObj   = Vec_IntEntry( p->vFans, iVar );
                int iNode0 = (iObj >>  0) & 0xFF;
                int iNode1 = (iObj >>  8) & 0xFF;
                int iNode2 = (iObj >> 16) & 0xFF;
                assert( iObj > 0 );
                assert( iNode1 == j );
                assert( iNode2 == i );
                // go through all variables of j and block those that point to k
                assert( p->VarMarks[j][2]   > 0 );
                assert( p->VarMarks[j+1][2] > 0 );
                for ( m = p->VarMarks[j][2]+1; m < p->VarMarks[j+1][2]; m++ )
                {
                    int jObj   = Vec_IntEntry( p->vFans, m );
                    int jNode0 = (jObj >>  0) & 0xFF;
                    int jNode1 = (jObj >>  8) & 0xFF;
                    int jNode2 = (jObj >> 16) & 0xFF;
                    if ( jObj == 0 )
                        continue;
                    assert( jNode2 == j );
                    if ( iNode0 == jNode0 || iNode0 == jNode1 )
                        Exa5_ManAddClause4( p, Abc_Var2Lit(iVar, 1), Abc_Var2Lit(m, 1), 0, 0, 0 );
                }
            }
        }
        for ( k = 0; k < 3; k++ )
            Exa5_ManAddClause4( p, Abc_Var2Lit(iVarStart, k==1), Abc_Var2Lit(iVarStart+1, k==2), Abc_Var2Lit(iVarStart+2, k!=0), 0, 0);
        if ( fOnlyAnd )
            Exa5_ManAddClause4( p, Abc_Var2Lit(iVarStart, 1), Abc_Var2Lit(iVarStart+1, 1), Abc_Var2Lit(iVarStart+2, 0), 0, 0);
    }
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
    {
        Vec_IntClear( vArray );
        Vec_IntForEachEntry( p->vFans, iObj, k )
            if ( iObj && ((iObj & 0xFF) == i || ((iObj >> 8) & 0xFF) == i) )
                Vec_IntPush( vArray, Abc_Var2Lit(k, 0) );
        for ( k = p->nDivs + p->nNodes; k < p->nObjs; k++ ) if ( p->VarMarks[k][i] )
            Vec_IntPush( vArray, Abc_Var2Lit(p->VarMarks[k][i], 0) );
        Exa5_ManAddClause( p, Vec_IntArray(vArray), Vec_IntSize(vArray) );
        if ( fUniqFans )
            Exa5_ManAddOneHot( p, Vec_IntArray(vArray), Vec_IntSize(vArray) );
    }
    Vec_IntFree( vArray );
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
    {
        nLits = 0;
        for ( j = 0; j < p->nDivs + p->nNodes; j++ ) if ( p->VarMarks[i][j] )
            pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][j], 0 );
        Exa5_ManAddClause( p, pLits, nLits );
        Exa5_ManAddOneHot( p, pLits, nLits );
    }
    return 1;
}
void Exa5_ManGenMint( Exa5_Man_t * p, int iMint, int fOnlyAnd, int fFancy )
{
    int iNodeVar = p->nCnfVars + p->nNodes*(iMint - Vec_WrdSize(p->vSimsIn));
    int iOutMint = Abc_Tt6FirstBit( Vec_WrdEntry(p->vSimsOut, iMint) );
    int i, k, n, j, VarVals[MAJ_NOBJS], iNodes[3], iVarStart, iObj;
    assert( p->nObjs <= MAJ_NOBJS );
    assert( iMint < Vec_WrdSize(p->vSimsIn) );
    for ( i = 0; i < p->nDivs; i++ )
        VarVals[i] = (Vec_WrdEntry(p->vSimsIn, iMint) >> i) & 1;
    for ( i = 0; i < p->nNodes; i++ )
        VarVals[p->nDivs + i] = Abc_Var2Lit(iNodeVar + i, 0);
    for ( i = 0; i < p->nOuts; i++ )
        VarVals[p->nDivs + p->nNodes + i] = (iOutMint >> i) & 1;
    if ( 0 )
    {
        printf( "Adding minterm %d: ", iMint );
        for ( i = 0; i < p->nObjs; i++ )
            printf( " %d=%d", i, VarVals[i] );
        printf( "\n" );
    }
    Vec_IntForEachEntry( p->vFans, iObj, i )
    {
        if ( iObj == 0 ) continue;
        iNodes[1] = (iObj >>  0) & 0xFF;
        iNodes[0] = (iObj >>  8) & 0xFF;
        iNodes[2] = (iObj >> 16) & 0xFF;
        iVarStart = 1 + 3*(iNodes[2] - p->nDivs);//
        for ( k = 0; k < 4; k++ )
        for ( n = 0; n < 2; n++ )
            Exa5_ManAddClause4( p, Abc_Var2Lit(i, 1), Abc_LitNotCond(VarVals[iNodes[0]], k&1), Abc_LitNotCond(VarVals[iNodes[1]], k>>1), Abc_LitNotCond(VarVals[iNodes[2]], !n), Abc_Var2Lit(k ? iVarStart + k-1 : 0, n));
    }
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
    {
        for ( j = 0; j < p->nDivs + p->nNodes; j++ ) if ( p->VarMarks[i][j] )
        for ( n = 0; n < 2; n++ )
            Exa5_ManAddClause4( p, Abc_Var2Lit(p->VarMarks[i][j], 1), Abc_LitNotCond(VarVals[j], n), Abc_LitNotCond(VarVals[i], !n), 0, 0);
    }
}
void Exa5_ManGenCnf( Exa5_Man_t * p, char * pFileName, int fOnlyAnd, int fFancy, int fOrderNodes, int fUniqFans )
{
    int m;
    assert( p->pFile == NULL );
    p->pFile = fopen( pFileName, "wb" );
    fputs( "p cnf                \n", p->pFile );
    Exa5_ManGenStart( p, fOnlyAnd, fFancy, fOrderNodes, fUniqFans );
    for ( m = 1; m < Vec_WrdSize(p->vSimsIn); m++ )
        Exa5_ManGenMint( p, m, fOnlyAnd, fFancy );
    rewind( p->pFile );
    fprintf( p->pFile, "p cnf %d %d", p->nCnfVars, p->nCnfClauses );
    fclose( p->pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Exa5_ManFindFanin( Exa5_Man_t * p, Vec_Int_t * vValues, int i )
{
    int j, Count = 0, iVar = -1;
    for ( j = 0; j < p->nObjs; j++ )
        if ( p->VarMarks[i][j] && Vec_IntEntry(vValues, p->VarMarks[i][j]) )
        {
            iVar = j;
            Count++;
        }
    assert( Count == 1 );
    return iVar;
}
static inline void Exa5_ManPrintFanin( Exa5_Man_t * p, int iNode, int fComp )
{
    if ( iNode == 0 )
        printf( " %s", fComp ? "const1" : "const0" );
    else if ( iNode > 0 && iNode <= p->nIns )
        printf( " %s%c", fComp ? "~" : "", 'a'+iNode-1 );
    else if ( iNode > p->nIns && iNode < p->nDivs )
        printf( " %s%c", fComp ? "~" : "", 'A'+iNode-p->nIns-1 );
    else
        printf( " %s%d", fComp ? "~" : "", iNode );
}
void Exa5_ManPrintSolution( Exa5_Man_t * p, Vec_Int_t * vValues, int fFancy )
{
    int Fan0[MAJ_NOBJS] = {0};
    int Fan1[MAJ_NOBJS] = {0};
    int Count[MAJ_NOBJS] = {0};
    int i, k, iObj, iNodes[3];
    printf( "Circuit for %d-var function with %d divisors contains %d two-input gates:\n", p->nIns, p->nDivs, p->nNodes );
    Vec_IntForEachEntry( p->vFans, iObj, i )
    {
        if ( iObj == 0 || Vec_IntEntry(vValues, i) == 0 )
            continue;
        iNodes[0] = (iObj >>  0) & 0xFF;
        iNodes[1] = (iObj >>  8) & 0xFF;
        iNodes[2] = (iObj >> 16) & 0xFF;
        assert( p->nDivs <= iNodes[2] && iNodes[2] < p->nDivs + p->nNodes );
        Fan0[iNodes[2]] = iNodes[0];
        Fan1[iNodes[2]] = iNodes[1];
        Count[iNodes[2]]++;
    }
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
    {
        int iVar = Exa5_ManFindFanin( p, vValues, i );
        printf( "%2d  = ", i );
        Exa5_ManPrintFanin( p, iVar, 0 );
        printf( "\n" );
    }
    for ( i = p->nDivs + p->nNodes - 1; i >= p->nDivs; i-- )
    {
        int iVarStart = 1 + 3*(i - p->nDivs);//
        int Val1 = Vec_IntEntry(vValues, iVarStart);
        int Val2 = Vec_IntEntry(vValues, iVarStart+1);
        int Val3 = Vec_IntEntry(vValues, iVarStart+2);
        assert( Count[i] == 1 );
        printf( "%2d  = ", i );
        for ( k = 0; k < 2; k++ )
        {
            int iNode = k ? Fan1[i] : Fan0[i];
            int fComp = k ? !Val1 && Val2 && !Val3 : Val1 && !Val2 && !Val3;
            Exa5_ManPrintFanin( p, iNode, fComp );
            if ( k ) break;
            printf( " %c", (Val1 && Val2) ? (Val3 ? '|' : '^') : '&' );
        }
        printf( "\n" );
    }
}
Mini_Aig_t * Exa5_ManMiniAig( Exa5_Man_t * p, Vec_Int_t * vValues )
{
    Mini_Aig_t * pMini = Mini_AigStartSupport( p->nDivs-1, p->nObjs );
    int Compl[MAJ_NOBJS] = {0};
    int Fan0[MAJ_NOBJS]  = {0};
    int Fan1[MAJ_NOBJS]  = {0};
    int Count[MAJ_NOBJS] = {0};
    int i, k, iObj, iNodes[3];
    Vec_IntForEachEntry( p->vFans, iObj, i )
    {
        if ( iObj == 0 || Vec_IntEntry(vValues, i) == 0 )
            continue;
        iNodes[0] = (iObj >>  0) & 0xFF;
        iNodes[1] = (iObj >>  8) & 0xFF;
        iNodes[2] = (iObj >> 16) & 0xFF;
        assert( p->nDivs <= iNodes[2] && iNodes[2] < p->nDivs + p->nNodes );
        Fan0[iNodes[2]] = iNodes[0];
        Fan1[iNodes[2]] = iNodes[1];
        Count[iNodes[2]]++;
    }
    assert( p->nDivs == Mini_AigNodeNum(pMini) );
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
    {
        int iNodes[2] = {0};
        int iVarStart = 1 + 3*(i - p->nDivs);//
        int Val1 = Vec_IntEntry(vValues, iVarStart);
        int Val2 = Vec_IntEntry(vValues, iVarStart+1);
        int Val3 = Vec_IntEntry(vValues, iVarStart+2);
        assert( Count[i] == 1 );
        Compl[i] = Val1 && Val2 && Val3;
        for ( k = 0; k < 2; k++ )
        {
            int iNode = k ? Fan1[i] : Fan0[i];
            int fComp = k ? !Val1 && Val2 && !Val3 : Val1 && !Val2 && !Val3;
            iNodes[k] = Abc_Var2Lit(iNode, fComp ^ Compl[iNode]);
        }
        if ( Val1 && Val2 ) 
        {
            if ( Val3 )
                Mini_AigOr( pMini, iNodes[0], iNodes[1] );
            else
                Mini_AigXorSpecial( pMini, iNodes[0], iNodes[1] );
        }
        else
            Mini_AigAnd( pMini, iNodes[0], iNodes[1] );
    }
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
    {
        int iVar = Exa5_ManFindFanin( p, vValues, i );
        Mini_AigCreatePo( pMini, Abc_Var2Lit(iVar, Compl[iVar]) );
    }
    assert( p->nObjs == Mini_AigNodeNum(pMini) );
    return pMini;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mini_Aig_t * Exa5_ManGenTest( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, int TimeOut, int fOnlyAnd, int fFancy, int fOrderNodes, int fUniqFans, int fVerbose )
{
    abctime clkTotal = Abc_Clock();
    Mini_Aig_t * pMini  = NULL;
    Vec_Int_t * vValues = NULL;
    char * pFileNameIn  = "_temp_.cnf";
    char * pFileNameOut = "_temp_out.cnf";
    Exa5_Man_t * p = Exa5_ManAlloc( vSimsIn, vSimsOut, nIns, nDivs, nOuts, nNodes, fVerbose );
    Exa_ManIsNormalized( vSimsIn, vSimsOut );
    Exa5_ManGenCnf( p, pFileNameIn, fOnlyAnd, fFancy, fOrderNodes, fUniqFans );
    if ( fVerbose )
        printf( "Timeout = %d. OnlyAnd = %d. Fancy = %d. OrderNodes = %d. UniqueFans = %d. Verbose = %d.\n", TimeOut, fOnlyAnd, fFancy, fOrderNodes, fUniqFans, fVerbose );
    if ( fVerbose )
        printf( "CNF with %d variables and %d clauses was dumped into file \"%s\".\n", p->nCnfVars, p->nCnfClauses, pFileNameIn );
    vValues = Exa4_ManSolve( pFileNameIn, pFileNameOut, TimeOut, fVerbose );
    if ( vValues ) pMini = Exa5_ManMiniAig( p, vValues );
    //if ( vValues ) Exa5_ManPrintSolution( p, vValues, fFancy );
    if ( vValues ) Exa_ManMiniPrint( pMini, p->nIns );
    if ( vValues ) Exa_ManMiniVerify( pMini, vSimsIn, vSimsOut );
    Vec_IntFreeP( &vValues );
    Exa5_ManFree( p );
    Abc_PrintTime( 1, "Total runtime", Abc_Clock() - clkTotal );
    return pMini;
}
void Exa_ManExactSynthesis5( Bmc_EsPar_t * pPars )
{
    Mini_Aig_t * pMini = NULL;
    int i, m, nMints = 1 << pPars->nVars, fCompl = 0;
    Vec_Wrd_t * vSimsIn  = Vec_WrdStart( nMints );
    Vec_Wrd_t * vSimsOut = Vec_WrdStart( nMints );
    word pTruth[16]; Abc_TtReadHex( pTruth, pPars->pTtStr );
    if ( pTruth[0] & 1 ) { fCompl = 1; Abc_TtNot( pTruth, Abc_TtWordNum(pPars->nVars) ); }
    assert( pPars->nVars <= 10 );
    for ( m = 0; m < nMints; m++ )
    {
        Abc_TtSetBit( Vec_WrdEntryP(vSimsOut, m), Abc_TtGetBit(pTruth, m) );
        for ( i = 0; i < pPars->nVars; i++ )
            if ( (m >> i) & 1 )
                Abc_TtSetBit( Vec_WrdEntryP(vSimsIn, m), 1+i );
    }
    assert( Vec_WrdSize(vSimsIn) == (1 << pPars->nVars) );
    pMini = Exa5_ManGenTest( vSimsIn, vSimsOut, pPars->nVars, 1+pPars->nVars, 1, pPars->nNodes, pPars->RuntimeLim, pPars->fOnlyAnd, pPars->fFewerVars, pPars->fOrderNodes, pPars->fUniqFans, pPars->fVerbose );
    if ( pMini ) Mini_AigStop( pMini );
    if ( fCompl ) printf( "The resulting circuit, if computed, will be complemented.\n" );
    Vec_WrdFree( vSimsIn );
    Vec_WrdFree( vSimsOut );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mini_AigPrintArray( FILE * pFile, Mini_Aig_t * p )
{
    int i, Count = 0;
    fprintf( pFile, "    { " );
    Mini_AigForEachAnd( p, i )
        fprintf( pFile, "%2d,%2d, ", Mini_AigNodeFanin0(p, i), Mini_AigNodeFanin1(p, i) ), Count++;
    Mini_AigForEachPo( p, i )
        fprintf( pFile, "%2d,%2d", Mini_AigNodeFanin0(p, i), Mini_AigNodeFanin0(p, i) ), Count++;
    for ( i = Count; i < 8; i++ )
        fprintf( pFile, ", %2d,%2d", 0, 0 );
    fprintf( pFile, " }" );
}
word Mini_AigWriteEntry( Mini_Aig_t * p )
{
    word Res = 0; int i, k = 0;
    Mini_AigForEachAnd( p, i )
    {
        int iLit0 = Mini_AigNodeFanin0(p, i);
        int iLit1 = Mini_AigNodeFanin1(p, i);
        int Pair;
        if ( k < 4 )
        {
            assert( (iLit1 & 0xF) != (iLit0 & 0xF) );
            Pair = ((iLit1 & 0xF) << 4) | (iLit0 & 0xF);
            Res |= (word)Pair << (k*8);
        }
        else
        {
            assert( (iLit1 & 0x1F) != (iLit0 & 0x1F) );
            Pair = ((iLit1 & 0x1F) << 5) | (iLit0 & 0x1F);
            Res |= (word)Pair << (32 + (k-4)*10);
        }
        k++;
    }
    Mini_AigForEachPo( p, i )
        if ( Mini_AigNodeFanin0(p, i) & 1 )
            Res |= (word)1 << 62;
    return Res;
}
word Abc_TtConvertEntry( word Res )
{
    unsigned First  = (unsigned)Res;
    unsigned Second = (unsigned)(Res >> 32);
    word Fun0, Fun1, Nodes[16] = {0}; int i, k = 5;
    for ( i = 0; i < 4; i++ )
        Nodes[i+1] = s_Truths6[i];
    for ( i = 0; i < 4; i++ )
    {
        int Lit0, Lit1, Pair = (First >> (i*8)) & 0xFF;
        if ( Pair == 0 )
            break;
        Lit0 = Pair & 0xF;
        Lit1 = Pair >> 4;
        assert( Lit0 != Lit1 );
        Fun0 = (Lit0 & 1) ? ~Nodes[Lit0 >> 1] : Nodes[Lit0 >> 1];
        Fun1 = (Lit1 & 1) ? ~Nodes[Lit1 >> 1] : Nodes[Lit1 >> 1];
        Nodes[k++] = Lit0 < Lit1 ? Fun0 & Fun1  : Fun0 ^ Fun1;
    }
    for ( i = 0; i < 3; i++ )
    {
        int Lit0, Lit1, Pair = (Second >> (i*10)) & 0x3FF;
        if ( Pair == 0 )
            break;
        Lit0 = Pair & 0x1F;
        Lit1 = Pair >> 5;
        assert( Lit0 != Lit1 );
        Fun0 = (Lit0 & 1) ? ~Nodes[Lit0 >> 1] : Nodes[Lit0 >> 1];
        Fun1 = (Lit1 & 1) ? ~Nodes[Lit1 >> 1] : Nodes[Lit1 >> 1];
        Nodes[k++] = Lit0 < Lit1 ? Fun0 & Fun1  : Fun0 ^ Fun1;
    }
    return (Res >> 62) ? ~Nodes[k-1] : Nodes[k-1];
}
word Exa_ManExactSynthesis4VarsOne( int Index, int Truth, int nNodes )
{
    Mini_Aig_t * pMini = NULL;
    int i, m, nMints = 16, fCompl = 0;
    Vec_Wrd_t * vSimsIn  = Vec_WrdStart( nMints );
    Vec_Wrd_t * vSimsOut = Vec_WrdStart( nMints );
    word pTruth[16] = { Abc_Tt6Stretch((word)Truth, 4) };
    if ( pTruth[0] & 1 ) { fCompl = 1; Abc_TtNot( pTruth, 1 ); }
    for ( m = 0; m < nMints; m++ )
    {
        Abc_TtSetBit( Vec_WrdEntryP(vSimsOut, m), Abc_TtGetBit(pTruth, m) );
        for ( i = 0; i < 4; i++ )
            if ( (m >> i) & 1 )
                Abc_TtSetBit( Vec_WrdEntryP(vSimsIn, m), 1+i );
    }
    assert( Vec_WrdSize(vSimsIn) == 16 );
    pMini = Exa5_ManGenTest( vSimsIn, vSimsOut, 4, 5, 1, nNodes, 0, 0, 0, 0, 0, 0 );
    if ( pMini && fCompl ) Mini_AigFlipLastPo( pMini );
    Vec_WrdFree( vSimsIn );
    Vec_WrdFree( vSimsOut );
    if ( pMini )
    {
        /*
        FILE * pTable = fopen( "min_xaig4.txt", "a+" );
        Mini_AigPrintArray( pTable, pMini );
        fprintf( pTable, ", // %d : 0x%04x (%d)\n", Index, Truth, nNodes );
        fclose( pTable );
        */
        word Res = Mini_AigWriteEntry( pMini );
        int uTruth = 0xFFFF & (int)Abc_TtConvertEntry( Res );
        if ( uTruth == Truth )
            printf( "Check ok.\n" );
        else
            printf( "Check NOT ok!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
        Mini_AigStop( pMini );
        return Res;
    }
    return 0;
}
void Exa_ManExactSynthesis4Vars()
{
    int i, k, nFuncs = 1 << 15;
    Vec_Wrd_t * vRes = Vec_WrdAlloc( nFuncs );
    Vec_WrdPush( vRes, 0 );
    for ( i = 1; i < nFuncs; i++ )
    {
        word Res = 0;
        printf( "\nFunction %d:\n", i );
        for ( k = 1; k < 8; k++ )
            if ( (Res = Exa_ManExactSynthesis4VarsOne( i, i, k )) )
                break;
        assert( Res );
        Vec_WrdPush( vRes, Res );
    }
    Vec_WrdDumpBin( "minxaig4.data", vRes, 1 );
    Vec_WrdFree( vRes );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Exa6_SortSims( Vec_Wrd_t * vSimsDiv, Vec_Wrd_t * vSimsOut )
{
    word * pSims = Vec_WrdArray( vSimsDiv );
    word * pOuts = Vec_WrdArray( vSimsOut ), temp;
    int i, j, best_i, nSize = Vec_WrdSize(vSimsDiv);
    assert( Vec_WrdSize(vSimsOut) == nSize );
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( pSims[j] < pSims[best_i] )
                best_i = j;
        if ( i == best_i )
            continue;
        temp = pSims[i]; 
        pSims[i] = pSims[best_i]; 
        pSims[best_i] = temp;
        temp = pOuts[i]; 
        pOuts[i] = pOuts[best_i]; 
        pOuts[best_i] = temp;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Exa6_ReadFile( char * pFileName, Vec_Wrd_t ** pvSimsDiv, Vec_Wrd_t ** pvSimsOut, int * pnDivs, int * pnOuts )
{
    int nIns = 0, nDivs = 0, nOuts = 0, nPats = 0, iLine = 0;
    int iIns = 0, iDivs = 0, iOuts = 0, Value, i;
    char pBuffer[1000];
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        Abc_Print( -1, "Cannot open file \"%s\".\n", pFileName );
        return 0;
    }
    while ( fgets( pBuffer, 1000, pFile ) != NULL )
    {
        if ( pBuffer[0] == 'c' )
            break;
        if ( iLine++ == 0 )
        {
            char * pLine = pBuffer;
            while ( (*pLine >= 'a' && *pLine <= 'z') || (*pLine >= 'A' && *pLine <= 'Z') )
                pLine++;
            Value = sscanf( pLine, "%d %d %d %d", &nIns, &nDivs, &nOuts, &nPats );
            if ( Value != 4 )
            {
                Abc_Print( -1, "Cannot read the parameter line in file \"%s\".\n", pFileName );
                fclose( pFile );
                return 0;
            }
            if ( nIns + nDivs >= 64 )
            {
                printf( "The number of variables and divisors should not exceed 64.\n" );
                return 0;
            }
            if ( nOuts > 6 )
            {
                printf( "The number of outputs should not exceed 6.\n" );
                return 0;
            }
            if ( nPats >= 1000 )
            {
                printf( "The number of patterns should not exceed 1000.\n" );
                return 0;
            }
            assert( nIns + nDivs < 64 && nOuts <= 6 && (nIns == 0 || nPats <= (1 << nIns)) && nPats < 1000 );
            *pvSimsDiv = Vec_WrdStart( nPats );
            *pvSimsOut = Vec_WrdStart( nPats );
            continue;
        }
        if ( pBuffer[0] == '\n' || pBuffer[0] == '\r' || pBuffer[0] == ' ' )
            continue;
        for ( i = 0; i < nPats; i++ )
        {
            if ( pBuffer[i] == '0' )
                continue;
            assert( pBuffer[i] == '1' );
            if ( iIns < nIns )
                Abc_TtSetBit( Vec_WrdEntryP(*pvSimsDiv, nPats-1-i), 1+iIns );
            else if ( iDivs < nDivs )
                Abc_TtSetBit( Vec_WrdEntryP(*pvSimsDiv, nPats-1-i), 1+nIns+iDivs );
            else if ( iOuts < (1 << nOuts) )
                Abc_TtSetBit( Vec_WrdEntryP(*pvSimsOut, nPats-1-i), iOuts );
        }
        assert( pBuffer[nPats] != '0' && pBuffer[nPats] != '1' );
        if ( iIns < nIns )
            iIns++;
        else if ( iDivs < nDivs )
            iDivs++;
        else if ( iOuts < (1 << nOuts) )
            iOuts++;
    }
    printf( "Finished reading file \"%s\" with %d inputs, %d divisors, %d outputs, and %d patterns.\n", pFileName, nIns, nDivs, nOuts, nPats );
    fclose( pFile );
    assert( iIns == nIns && iDivs == nDivs && iOuts == (1 << nOuts) );
    if ( pnDivs )
        *pnDivs = nDivs;
    if ( pnOuts )
        *pnOuts = nOuts;
    return nIns;
}
void Exa6_WriteFile( char * pFileName, int nVars, word * pTruths, int nTruths )
{
    int i, o, m, nMintsI = 1 << nVars, nMintsO = 1 << nTruths;
    FILE * pFile = fopen( pFileName, "wb" );
    fprintf( pFile, "%d %d %d %d\n", nVars, 0, nTruths, nMintsI );
    fprintf( pFile, "\n" );
    for ( i = 0; i < nVars; i++, fprintf( pFile, "\n" ) )
        for ( m = nMintsI - 1; m >= 0; m-- )
            fprintf( pFile, "%d", (m >> i) & 1 );
    fprintf( pFile, "\n" );
    for ( o = 0; o < nMintsO; o++, fprintf( pFile, "\n" ) )
        for ( m = nMintsI - 1; m >= 0; m-- )
        {
            int oMint = 0;
            for ( i = 0; i < nTruths; i++ )
                if ( Abc_TtGetBit(pTruths+i, m) )
                    oMint |= (word)1 << i;
            fprintf( pFile, "%d", oMint == o );
        }
    fclose( pFile );
}
void Exa6_WriteFile2( char * pFileName, int nVars, int nDivs, int nOuts, Vec_Wrd_t * vSimsDiv, Vec_Wrd_t * vSimsOut )
{
    int i, o, m, nMintsO = 1 << nOuts;
    FILE * pFile = fopen( pFileName, "wb" );
    assert( Vec_WrdSize(vSimsDiv) == Vec_WrdSize(vSimsOut) );
    fprintf( pFile, "%d %d %d %d\n", nVars, nDivs, nOuts, Vec_WrdSize(vSimsOut) );
    fprintf( pFile, "\n" );
    for ( i = 0; i < nVars+nDivs; i++, fprintf( pFile, "\n%s", (i == nVars && nDivs) ? "\n":"" )  )
        for ( m = Vec_WrdSize(vSimsOut) - 1; m >= 0; m-- )
            fprintf( pFile, "%d", Abc_TtGetBit(Vec_WrdEntryP(vSimsDiv, m), 1+i) );
    fprintf( pFile, "\n" );
    for ( o = 0; o < nMintsO; o++, fprintf( pFile, "\n" ) )
        for ( m = Vec_WrdSize(vSimsOut) - 1; m >= 0; m-- )
            fprintf( pFile, "%d", Abc_TtGetBit(Vec_WrdEntryP(vSimsOut, m), o) );
    fclose( pFile );
}
void Exa6_GenCount( word * pTruths, int nVars )
{
    int i, k;
    assert( nVars >= 3 && nVars <= 7 );
    for ( k = 0; k < 3; k++ )
        pTruths[k] = 0;
    for ( i = 0; i < (1 << nVars); i++ )
    {
        int n = Abc_TtCountOnes( (word)i );
        for ( k = 0; k < 3; k++ )
            if ( (n >> k) & 1 )
                Abc_TtSetBit( pTruths+k, i );
    }
}
void Exa6_GenProd( word * pTruths, int nVars )
{
    int i, j, k;
    nVars /= 2;
    assert( nVars >= 2 && nVars <= 3 );
    for ( k = 0; k < 2*nVars; k++ )
        pTruths[k] = 0;
    for ( i = 0; i < (1 << nVars); i++ )
    for ( j = 0; j < (1 << nVars); j++ )
    {
        int n = i * j;
        for ( k = 0; k < 2*nVars; k++ )
            if ( (n >> k) & 1 )
                Abc_TtSetBit( pTruths+k, (i << nVars) | j );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Exa_ManExactSynthesis6_( Bmc_EsPar_t * pPars, char * pFileName )
{
    Vec_Wrd_t * vSimsDiv = NULL, * vSimsOut = NULL;
    word Entry, Truths[100] = { 0x96, 0xE8 };
//    int i, nVars = 3, nOuts = 2; 
//    int i, nVars = 6, nOuts = 3; 
    int i, nVars = 4, nOuts = 4, nDivs2, nOuts2; 
//    Exa6_GenCount( Truths, nVars );
    Exa6_GenProd( Truths, nVars );
    Exa6_WriteFile( pFileName, nVars, Truths, nOuts );

    nVars = Exa6_ReadFile( pFileName, &vSimsDiv, &vSimsOut, &nDivs2, &nOuts2 );

    Vec_WrdForEachEntry( vSimsDiv, Entry, i )
        Abc_TtPrintBits( &Entry, 1 + nVars );
    printf( "\n" );
    Vec_WrdForEachEntry( vSimsOut, Entry, i )
        Abc_TtPrintBits( &Entry, 1 << nOuts );
    printf( "\n" );

    Vec_WrdFree( vSimsDiv );
    Vec_WrdFree( vSimsOut );
}




/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManDupMini_rec( Gia_Man_t * pNew, Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( ~pObj->Value )
        return pObj->Value;
    assert( Gia_ObjIsAnd(pObj) );
    Gia_ManDupMini_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManDupMini_rec( pNew, p, Gia_ObjFanin1(pObj) );
    return pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
}
Gia_Man_t * Gia_ManDupMini( Gia_Man_t * p, Vec_Int_t * vIns, Vec_Int_t * vDivs, Vec_Int_t * vOuts, Mini_Aig_t * pMini )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj; int i, k;
    Vec_Int_t * vCopies = Vec_IntStartFull( Mini_AigNodeNum(pMini) );
    //int nPis = Mini_AigPiNum(pMini);
    Vec_IntWriteEntry( vCopies, 0, 0 );
    assert( Mini_AigPiNum(pMini) == Vec_IntSize(vIns)+Vec_IntSize(vDivs) );
    assert( Mini_AigPoNum(pMini) == Vec_IntSize(vOuts) );
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi(pNew);
    Gia_ManForEachObjVec( vIns, p, pObj, i )
        Vec_IntWriteEntry( vCopies, 1+i, Gia_ManDupMini_rec(pNew, p, pObj) );
    Gia_ManForEachObjVec( vDivs, p, pObj, i )
        Vec_IntWriteEntry( vCopies, 1+Vec_IntSize(vIns)+i, Gia_ManDupMini_rec(pNew, p, pObj) );
    Mini_AigForEachAnd( pMini, k )
    {
        int iFaninLit0 = Mini_AigNodeFanin0( pMini, k );
        int iFaninLit1 = Mini_AigNodeFanin1( pMini, k );
        int iLit0 = Vec_IntEntry(vCopies, Mini_AigLit2Var(iFaninLit0)) ^ Mini_AigLitIsCompl(iFaninLit0);
        int iLit1 = Vec_IntEntry(vCopies, Mini_AigLit2Var(iFaninLit1)) ^ Mini_AigLitIsCompl(iFaninLit1);
        if ( iFaninLit0 < iFaninLit1 )
            Vec_IntWriteEntry( vCopies, k, Gia_ManAppendAnd(pNew, iLit0, iLit1) );
        else
            Vec_IntWriteEntry( vCopies, k, Gia_ManAppendXorReal(pNew, iLit0, iLit1) );
    }
    i = 0;
    Mini_AigForEachPo( pMini, k )
    {
        int iFaninLit0 = Mini_AigNodeFanin0( pMini, k );
        int iLit0 = Vec_IntEntry(vCopies, Mini_AigLit2Var(iFaninLit0)) ^ Mini_AigLitIsCompl(iFaninLit0);
        Gia_ManObj( p, Vec_IntEntry(vOuts, i++) )->Value = iLit0;
    }
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManDupMini_rec( pNew, p, Gia_ObjFanin0(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    Vec_IntFree( vCopies );
    return pNew;
}






typedef struct Exa6_Man_t_ Exa6_Man_t;
struct Exa6_Man_t_ 
{
    Vec_Wrd_t *       vSimsIn;   // input signatures  (nWords = 1, nIns <= 64)
    Vec_Wrd_t *       vSimsOut;  // output signatures (nWords = 1, nOuts <= 6)
    int               fVerbose; 
    int               nIns;     
    int               nDivs;     // divisors (const + inputs + tfi + sidedivs)
    int               nNodes;
    int               nOuts; 
    int               nObjs;
    int               VarMarks[MAJ_NOBJS][2][MAJ_NOBJS];
    int               nCnfVars; 
    int               nCnfVars2; 
    int               nCnfClauses;
    FILE *            pFile;
};

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Exa6_ManMarkup( Exa6_Man_t * p )
{
    int i, k, j, nVars[3] = {1 + 3*p->nNodes, 0, 3*p->nNodes*Vec_WrdSize(p->vSimsIn)};
    assert( p->nObjs <= MAJ_NOBJS );
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
        for ( k = 0; k < 2; k++ )
            for ( j = 1+!k; j < i-k; j++ )
                p->VarMarks[i][k][j] = nVars[0] + nVars[1]++;
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
        for ( j = 0; j < p->nDivs + p->nNodes; j++ )
            p->VarMarks[i][0][j] = nVars[0] + nVars[1]++;
    if ( p->fVerbose )
        printf( "Variables:  Function = %d.  Structure = %d.  Internal = %d.  Total = %d.\n", 
            nVars[0], nVars[1], nVars[2], nVars[0] + nVars[1] + nVars[2] );
    if ( 0 )
    {
        for ( j = 0; j < 2; j++ )
        {
            printf( "   : " );
            for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
            {
                for ( k = 0; k < 2; k++ )
                    printf( "%3d ", j ? k : i );
                printf( " " );
            }
            printf( " " );
            for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
            {
                printf( "%3d ", j ? 0 : i );
                printf( " " );
            }
            printf( "\n" );
        }
        for ( j = 0; j < 5 + p->nNodes*9 + p->nOuts*5; j++ )
            printf( "-" );
        printf( "\n" );
        for ( j = p->nObjs - 2; j >= 0; j-- )
        {
            if ( j > 0 && j <= p->nIns )
                printf( " %c : ", 'a'+j-1 );
            else if ( j > p->nIns && j < p->nDivs )
                printf( " %c : ", 'A'+j-1-p->nIns );
            else
                printf( "%2d : ", j );
            for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
            {
                for ( k = 0; k < 2; k++ )
                    printf( "%3d ", p->VarMarks[i][k][j] );
                printf( " " );
            }
            printf( " " );
            for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
            {
                printf( "%3d ", p->VarMarks[i][0][j] );
                printf( " " );
            }
            printf( "\n" );
        }
    }
    return nVars[0] + nVars[1] + nVars[2];
}
Exa6_Man_t * Exa6_ManAlloc( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, int fVerbose )
{
    Exa6_Man_t * p = ABC_CALLOC( Exa6_Man_t, 1 );
    assert( Vec_WrdSize(vSimsIn) == Vec_WrdSize(vSimsOut) );
    p->vSimsIn     = vSimsIn;
    p->vSimsOut    = vSimsOut;
    p->fVerbose    = fVerbose;
    p->nIns        = nIns;  
    p->nDivs       = nDivs; 
    p->nNodes      = nNodes;
    p->nOuts       = nOuts; 
    p->nObjs       = p->nDivs + p->nNodes + p->nOuts;
    p->nCnfVars    = Exa6_ManMarkup( p );
    p->nCnfVars2   = 0;
    assert( p->nObjs < 64 );
    return p;
}
void Exa6_ManFree( Exa6_Man_t * p )
{
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Exa6_ManAddClause( Exa6_Man_t * p, int * pLits, int nLits )
{
    int i, k = 0;
    for ( i = 0; i < nLits; i++ )
        if ( pLits[i] == 1 )
            return 0;
        else if ( pLits[i] == 0 )
            continue;
        else if ( pLits[i] <= 2*(p->nCnfVars + p->nCnfVars2) )
            pLits[k++] = pLits[i];
        else assert( 0 );
    nLits = k;
    assert( nLits > 0 );
    if ( p->pFile )
    {
        p->nCnfClauses++;
        for ( i = 0; i < nLits; i++ )
            fprintf( p->pFile, "%s%d ", Abc_LitIsCompl(pLits[i]) ? "-" : "", Abc_Lit2Var(pLits[i]) );
        fprintf( p->pFile, "0\n" );
    }
    if ( 0 )
    {
        for ( i = 0; i < nLits; i++ )
            fprintf( stdout, "%s%d ", Abc_LitIsCompl(pLits[i]) ? "-" : "", Abc_Lit2Var(pLits[i]) );
        fprintf( stdout, "\n" );
    }
    return 1;
}
static inline int Exa6_ManAddClause4( Exa6_Man_t * p, int Lit0, int Lit1, int Lit2, int Lit3 )
{
    int pLits[4] = { Lit0, Lit1, Lit2, Lit3 };
    return Exa6_ManAddClause( p, pLits, 4 );
}
int Exa6_ManGenStart( Exa6_Man_t * p, int fOnlyAnd, int fFancy, int fOrderNodes, int fUniqFans )
{
    int pLits[2*MAJ_NOBJS], i, j, k, n, m, nLits;
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
    {
        int iVarStart = 1 + 3*(i - p->nDivs);//
        for ( k = 0; k < 2; k++ )
        {
            nLits = 0;
            for ( j = 0; j < p->nObjs; j++ )
                if ( p->VarMarks[i][k][j] )
                    pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][k][j], 0 );
            assert( nLits > 0 );
            Exa6_ManAddClause( p, pLits, nLits );
            for ( n = 0;   n < nLits; n++ )
            for ( m = n+1; m < nLits; m++ )
                Exa6_ManAddClause4( p, Abc_LitNot(pLits[n]), Abc_LitNot(pLits[m]), 0, 0 );
            if ( k == 1 )
                break;
            for ( j = 0; j < p->nObjs; j++ ) if ( p->VarMarks[i][0][j] )
            for ( n = j; n < p->nObjs; n++ ) if ( p->VarMarks[i][1][n] )
                Exa6_ManAddClause4( p, Abc_Var2Lit(p->VarMarks[i][0][j], 1), Abc_Var2Lit(p->VarMarks[i][1][n], 1), 0, 0 );
        }
        if ( fOrderNodes )
        for ( j = p->nDivs; j < i; j++ )
        for ( n = 0;   n < p->nObjs; n++ ) if ( p->VarMarks[i][0][n] )
        for ( m = n+1; m < p->nObjs; m++ ) if ( p->VarMarks[j][0][m] )
            Exa6_ManAddClause4( p, Abc_Var2Lit(p->VarMarks[i][0][n], 1), Abc_Var2Lit(p->VarMarks[j][0][m], 1), 0, 0 );
        for ( j = p->nDivs; j < i; j++ )
        for ( k = 0; k < 2; k++ )        if ( p->VarMarks[i][k][j] )
        for ( n = 0; n < p->nObjs; n++ ) if ( p->VarMarks[i][!k][n] )
        for ( m = 0; m < 2; m++ )        if ( p->VarMarks[j][m][n] )
            Exa6_ManAddClause4( p, Abc_Var2Lit(p->VarMarks[i][k][j], 1), Abc_Var2Lit(p->VarMarks[i][!k][n], 1), Abc_Var2Lit(p->VarMarks[j][m][n], 1), 0 );
        if ( fFancy )
        {
            nLits = 0;
            for ( k = 0; k < 5-fOnlyAnd; k++ )
                pLits[nLits++] = Abc_Var2Lit( iVarStart+k, 0 );
            Exa6_ManAddClause( p, pLits, nLits );
            for ( n = 0;   n < nLits; n++ )
            for ( m = n+1; m < nLits; m++ )
                Exa6_ManAddClause4( p, Abc_LitNot(pLits[n]), Abc_LitNot(pLits[m]), 0, 0 );
        }
        else
        {
            for ( k = 0; k < 3; k++ )
                Exa6_ManAddClause4( p, Abc_Var2Lit(iVarStart, k==1), Abc_Var2Lit(iVarStart+1, k==2), Abc_Var2Lit(iVarStart+2, k!=0), 0);
            if ( fOnlyAnd )
                Exa6_ManAddClause4( p, Abc_Var2Lit(iVarStart, 1), Abc_Var2Lit(iVarStart+1, 1), Abc_Var2Lit(iVarStart+2, 0), 0);
        }
    }
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
    {
        nLits = 0;
        for ( k = 0; k < 2; k++ )
            for ( j = i+1; j < p->nObjs; j++ )
                if ( p->VarMarks[j][k][i] )
                    pLits[nLits++] = Abc_Var2Lit( p->VarMarks[j][k][i], 0 );
        //Exa6_ManAddClause( p, pLits, nLits );
        if ( fUniqFans )
            for ( n = 0;   n < nLits; n++ )
            for ( m = n+1; m < nLits; m++ )
                Exa6_ManAddClause4( p, Abc_LitNot(pLits[n]), Abc_LitNot(pLits[m]), 0, 0 );
    }
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
    {
        nLits = 0;
        for ( j = 0; j < p->nDivs + p->nNodes; j++ ) if ( p->VarMarks[i][0][j] )
            pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][0][j], 0 );
        Exa6_ManAddClause( p, pLits, nLits );
        for ( n = 0;   n < nLits; n++ )
        for ( m = n+1; m < nLits; m++ )
            Exa6_ManAddClause4( p, Abc_LitNot(pLits[n]), Abc_LitNot(pLits[m]), 0, 0 );
    }
    return 1;
}
void Exa6_ManGenMint( Exa6_Man_t * p, int iMint, int fOnlyAnd, int fFancy )
{
    int iNodeVar = p->nCnfVars + 3*p->nNodes*(iMint - Vec_WrdSize(p->vSimsIn));
    int iOutMint = Abc_Tt6FirstBit( Vec_WrdEntry(p->vSimsOut, iMint) );
    int fOnlyOne = Abc_TtSuppOnlyOne( (int)Vec_WrdEntry(p->vSimsOut, iMint) );
    int i, k, n, j, VarVals[MAJ_NOBJS];
    int fAllOnes = Abc_TtCountOnes( Vec_WrdEntry(p->vSimsOut, iMint) ) == (1 << p->nOuts);
    if ( fAllOnes )
        return;
    assert( p->nObjs <= MAJ_NOBJS );
    assert( iMint < Vec_WrdSize(p->vSimsIn) );
    assert( p->nOuts <= 6 );
    for ( i = 0; i < p->nDivs; i++ )
        VarVals[i] = (Vec_WrdEntry(p->vSimsIn, iMint) >> i) & 1;
    for ( i = 0; i < p->nNodes; i++ )
        VarVals[p->nDivs + i] = Abc_Var2Lit(iNodeVar + 3*i + 2, 0);
    if ( fOnlyOne )
    {
        for ( i = 0; i < p->nOuts; i++ )
            VarVals[p->nDivs + p->nNodes + i] = (iOutMint >> i) & 1;
    }
    else
    {
        word t = Abc_Tt6Stretch( Vec_WrdEntry(p->vSimsOut, iMint), p->nOuts );
        int i, c, nCubes = 0, pCover[100], pLits[10];
        int iOutVar = p->nCnfVars + p->nCnfVars2;  p->nCnfVars2 += p->nOuts;
        for ( i = 0; i < p->nOuts; i++ )
            VarVals[p->nDivs + p->nNodes + i] = Abc_Var2Lit(iOutVar + i, 0);
        assert( t );
        if ( ~t )
        {
            Abc_Tt6IsopCover( t, t, p->nOuts, pCover, &nCubes );
            for ( c = 0; c < nCubes; c++ )
            {
                int nLits = 0;
                for ( i = 0; i < p->nOuts; i++ )
                {
                    int iLit = (pCover[c] >> (2*i)) & 3;
                    if ( iLit == 1 || iLit == 2 )
                        pLits[nLits++] = Abc_Var2Lit(iOutVar + i, iLit != 1);
                }
                Exa6_ManAddClause( p, pLits, nLits );
            }
        }
    }
    if ( 0 )
    {
        printf( "Adding minterm %d: ", iMint );
        for ( i = 0; i < p->nObjs; i++ )
            printf( " %d=%d", i, VarVals[i] );
        printf( "\n" );
    }
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
    {
        int iVarStart = 1 + 3*(i - p->nDivs);//
        int iBaseVarI = iNodeVar + 3*(i - p->nDivs);
        for ( k = 0; k < 2; k++ )
        for ( j = 0; j < i; j++ ) if ( p->VarMarks[i][k][j] )
        for ( n = 0; n < 2; n++ )
            Exa6_ManAddClause4( p, Abc_Var2Lit(p->VarMarks[i][k][j], 1), Abc_Var2Lit(iBaseVarI + k, n), Abc_LitNotCond(VarVals[j], !n), 0);
        for ( k = 0; k < 4; k++ )
        for ( n = 0; n < 2; n++ )
            Exa6_ManAddClause4( p, Abc_Var2Lit(iBaseVarI + 0, k&1), Abc_Var2Lit(iBaseVarI + 1, k>>1), Abc_Var2Lit(iBaseVarI + 2, !n), Abc_Var2Lit(k ? iVarStart + k-1 : 0, n));
    }
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
    {
        for ( j = 0; j < p->nDivs + p->nNodes; j++ ) if ( p->VarMarks[i][0][j] )
        for ( n = 0; n < 2; n++ )
            Exa6_ManAddClause4( p, Abc_Var2Lit(p->VarMarks[i][0][j], 1), Abc_LitNotCond(VarVals[j], n), Abc_LitNotCond(VarVals[i], !n), 0);
    }
}
void Exa6_ManGenCnf( Exa6_Man_t * p, char * pFileName, int fOnlyAnd, int fFancy, int fOrderNodes, int fUniqFans )
{
    int m;
    assert( p->pFile == NULL );
    p->pFile = fopen( pFileName, "wb" );
    fputs( "p cnf                \n", p->pFile );
    Exa6_ManGenStart( p, fOnlyAnd, fFancy, fOrderNodes, fUniqFans );
    for ( m = 1; m < Vec_WrdSize(p->vSimsIn); m++ )
        Exa6_ManGenMint( p, m, fOnlyAnd, fFancy );
    rewind( p->pFile );
    fprintf( p->pFile, "p cnf %d %d", p->nCnfVars + p->nCnfVars2, p->nCnfClauses );
    fclose( p->pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Exa6_ManFindFanin( Exa6_Man_t * p, Vec_Int_t * vValues, int i, int k )
{
    int j, Count = 0, iVar = -1;
    for ( j = 0; j < p->nObjs; j++ )
        if ( p->VarMarks[i][k][j] && Vec_IntEntry(vValues, p->VarMarks[i][k][j]) )
        {
            iVar = j;
            Count++;
        }
    assert( Count == 1 );
    return iVar;
}
static inline void Exa6_ManPrintFanin( Exa6_Man_t * p, int iNode, int fComp )
{
    if ( iNode == 0 )
        printf( " %s", fComp ? "const1" : "const0" );
    else if ( iNode > 0 && iNode <= p->nIns )
        printf( " %s%c", fComp ? "~" : "", 'a'+iNode-1 );
    else if ( iNode > p->nIns && iNode < p->nDivs )
        printf( " %s%c", fComp ? "~" : "", 'A'+iNode-p->nIns-1 );
    else
        printf( " %s%d", fComp ? "~" : "", iNode );
}
void Exa6_ManPrintSolution( Exa6_Man_t * p, Vec_Int_t * vValues, int fFancy )
{
    int i, k;
    printf( "Circuit for %d-var function with %d divisors contains %d two-input gates:\n", p->nIns, p->nDivs, p->nNodes );
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
    {
        int iVar = Exa6_ManFindFanin( p, vValues, i, 0 );
        printf( "%2d  = ", i );
        Exa6_ManPrintFanin( p, iVar, 0 );
        printf( "\n" );
    }
    for ( i = p->nDivs + p->nNodes - 1; i >= p->nDivs; i-- )
    {
        int iVarStart = 1 + 3*(i - p->nDivs);//
        int Val1 = Vec_IntEntry(vValues, iVarStart);
        int Val2 = Vec_IntEntry(vValues, iVarStart+1);
        int Val3 = Vec_IntEntry(vValues, iVarStart+2);
        printf( "%2d  = ", i );
        for ( k = 0; k < 2; k++ )
        {
            int iNode = Exa6_ManFindFanin( p, vValues, i, !k );
            int fComp = k ? !Val1 && Val2 && !Val3 : Val1 && !Val2 && !Val3;
            Exa6_ManPrintFanin( p, iNode, fComp );
            if ( k ) break;
            printf( " %c", (Val1 && Val2) ? (Val3 ? '|' : '^') : '&' );
        }
        printf( "\n" );
    }
}
Mini_Aig_t * Exa6_ManMiniAig( Exa6_Man_t * p, Vec_Int_t * vValues, int fFancy )
{
    int i, k, Compl[MAJ_NOBJS] = {0};
    Mini_Aig_t * pMini = Mini_AigStartSupport( p->nDivs-1, p->nObjs );
    for ( i = p->nDivs; i < p->nDivs + p->nNodes; i++ )
    {
        int iNodes[2] = {0};
        int iVarStart = 1 + 3*(i - p->nDivs);//
        int Val1 = Vec_IntEntry(vValues, iVarStart);
        int Val2 = Vec_IntEntry(vValues, iVarStart+1);
        int Val3 = Vec_IntEntry(vValues, iVarStart+2);
        Compl[i] = Val1 && Val2 && Val3;
        for ( k = 0; k < 2; k++ )
        {
            int iNode = Exa6_ManFindFanin( p, vValues, i, !k );
            int fComp = k ? !Val1 && Val2 && !Val3 : Val1 && !Val2 && !Val3;
            iNodes[k] = Abc_Var2Lit(iNode, fComp ^ Compl[iNode]);
        }
        if ( Val1 && Val2 ) 
        {
            if ( Val3 )
                Mini_AigOr( pMini, iNodes[0], iNodes[1] );
            else
                Mini_AigXorSpecial( pMini, iNodes[0], iNodes[1] );
        }
        else
            Mini_AigAnd( pMini, iNodes[0], iNodes[1] );
    }
    for ( i = p->nDivs + p->nNodes; i < p->nObjs; i++ )
    {
        int iVar = Exa6_ManFindFanin( p, vValues, i, 0 );
        Mini_AigCreatePo( pMini, Abc_Var2Lit(iVar, Compl[iVar]) );
    }
    assert( p->nObjs == Mini_AigNodeNum(pMini) );
    return pMini;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mini_Aig_t * Exa6_ManGenTest( Vec_Wrd_t * vSimsIn, Vec_Wrd_t * vSimsOut, int nIns, int nDivs, int nOuts, int nNodes, int TimeOut, int fOnlyAnd, int fFancy, int fOrderNodes, int fUniqFans, int fVerbose )
{
    Mini_Aig_t * pMini = NULL;
    abctime clkTotal = Abc_Clock();
    Vec_Int_t * vValues = NULL;
    char * pFileNameIn  = "_temp_.cnf";
    char * pFileNameOut = "_temp_out.cnf";
    Exa6_Man_t * p = Exa6_ManAlloc( vSimsIn, vSimsOut, nIns, 1+nIns+nDivs, nOuts, nNodes, fVerbose );
    Exa_ManIsNormalized( vSimsIn, vSimsOut );
    Exa6_ManGenCnf( p, pFileNameIn, fOnlyAnd, fFancy, fOrderNodes, fUniqFans );
    if ( fVerbose )
        printf( "Timeout = %d. OnlyAnd = %d. Fancy = %d. OrderNodes = %d. UniqueFans = %d. Verbose = %d.\n", TimeOut, fOnlyAnd, fFancy, fOrderNodes, fUniqFans, fVerbose );
    if ( fVerbose )
        printf( "CNF with %d variables and %d clauses was dumped into file \"%s\".\n", p->nCnfVars, p->nCnfClauses, pFileNameIn );
    vValues = Exa4_ManSolve( pFileNameIn, pFileNameOut, TimeOut, fVerbose );
    if ( vValues ) pMini = Exa6_ManMiniAig( p, vValues, fFancy );
    //if ( vValues ) Exa6_ManPrintSolution( p, vValues, fFancy );
    if ( vValues ) Exa_ManMiniPrint( pMini, p->nIns );
    if ( vValues && nIns <= 6 ) Exa_ManMiniVerify( pMini, vSimsIn, vSimsOut );
    Vec_IntFreeP( &vValues );
    Exa6_ManFree( p );
    Abc_PrintTime( 1, "Total runtime", Abc_Clock() - clkTotal );
    return pMini;
}
Mini_Aig_t * Mini_AigDupCompl( Mini_Aig_t * p, int ComplIns, int ComplOuts )
{
    Mini_Aig_t * pNew = Mini_AigStartSupport( Mini_AigPiNum(p), Mini_AigNodeNum(p) );
    Vec_Int_t * vCopies = Vec_IntStartFull( Mini_AigNodeNum(p) ); int k, i = 0, o = 0;
    Vec_IntWriteEntry( vCopies, 0, 0 );
    Mini_AigForEachPi( p, k )
        Vec_IntWriteEntry( vCopies, k, Abc_Var2Lit(k, (ComplIns>>i++) & 1) );
    Mini_AigForEachAnd( p, k )
    {
        int iFaninLit0 = Mini_AigNodeFanin0( p, k );
        int iFaninLit1 = Mini_AigNodeFanin1( p, k );
        int iLit0 = Vec_IntEntry(vCopies, Mini_AigLit2Var(iFaninLit0)) ^ Mini_AigLitIsCompl(iFaninLit0);
        int iLit1 = Vec_IntEntry(vCopies, Mini_AigLit2Var(iFaninLit1)) ^ Mini_AigLitIsCompl(iFaninLit1);
        if ( iFaninLit0 < iFaninLit1 )
            Vec_IntWriteEntry( vCopies, k, Mini_AigAnd(pNew, iLit0, iLit1) );
        else
            Vec_IntWriteEntry( vCopies, k, Mini_AigXorSpecial(pNew, iLit0, iLit1) );
    }
    Mini_AigForEachPo( p, k )
    {
        int iFaninLit0 = Mini_AigNodeFanin0( p, k );
        int iLit0 = Vec_IntEntry(vCopies, Mini_AigLit2Var(iFaninLit0)) ^ Mini_AigLitIsCompl(iFaninLit0);
        Mini_AigCreatePo( pNew, iLit0 ^ ((ComplOuts >> o++) & 1) );
    }
    Vec_IntFree( vCopies );
    return pNew;
}
word Exa6_ManPolarMinterm( word Mint, int nOuts, int Polar )
{
    word MintNew = 0; 
    int m, nMints = 1 << nOuts;
    for ( m = 0; m < nMints; m++ )
        if ( (Mint >> m) & 1 )
            MintNew |= (word)1 << (m ^ Polar);
    return MintNew;
}
int Exa6_ManFindPolar( word First, int nOuts )
{
    int i;
    for ( i = 0; i < (1 << nOuts); i++ )
        if ( Exa6_ManPolarMinterm(First, nOuts, i) & 1 )
            return i;
    return -1;
}
Vec_Wrd_t * Exa6_ManTransformOutputs( Vec_Wrd_t * vOuts, int nOuts )
{
    Vec_Wrd_t * vRes = Vec_WrdAlloc( Vec_WrdSize(vOuts) );
    int i, Polar = Exa6_ManFindPolar( Vec_WrdEntry(vOuts, 0), nOuts ); word Entry;
    Vec_WrdForEachEntry( vOuts, Entry, i )
        Vec_WrdPush( vRes, Exa6_ManPolarMinterm(Entry, nOuts, Polar) );
    return vRes;
}
Vec_Wrd_t * Exa6_ManTransformInputs( Vec_Wrd_t * vIns )
{
    Vec_Wrd_t * vRes = Vec_WrdAlloc( Vec_WrdSize(vIns) );
    int i, Polar = Vec_WrdEntry( vIns, 0 ); word Entry;
    Vec_WrdForEachEntry( vIns, Entry, i )
        Vec_WrdPush( vRes, Entry ^ Polar );
    return vRes;
}
void Exa_ManExactPrint( Vec_Wrd_t * vSimsDiv, Vec_Wrd_t * vSimsOut, int nDivs, int nOuts )
{
    word Entry; int i;
    Vec_WrdForEachEntry( vSimsDiv, Entry, i )
        Abc_TtPrintBits( &Entry, nDivs );
    printf( "\n" );
    Vec_WrdForEachEntry( vSimsOut, Entry, i )
        Abc_TtPrintBits( &Entry, 1 << nOuts );
    printf( "\n" );
}
Mini_Aig_t * Exa_ManExactSynthesis6Int( Vec_Wrd_t * vSimsDiv, Vec_Wrd_t * vSimsOut, int nVars, int nDivs, int nOuts, int nNodes, int fOnlyAnd, int fVerbose )
{
    Mini_Aig_t * pTemp, * pMini; 
    Vec_Wrd_t * vSimsDiv2, * vSimsOut2;
    int DivCompl, OutCompl;
    if ( nVars == 0 ) return NULL;
    assert( nVars <= 8 );
    DivCompl = (int)Vec_WrdEntry(vSimsDiv, 0) >> 1;
    OutCompl = Exa6_ManFindPolar( Vec_WrdEntry(vSimsOut, 0), nOuts );
    if ( fVerbose )
        printf( "Inputs = %d. Divisors = %d. Outputs = %d. Nodes = %d.  InP = %d. OutP = %d.\n", 
            nVars, nDivs, nOuts, nNodes, DivCompl, OutCompl );
    vSimsDiv2 = Exa6_ManTransformInputs( vSimsDiv );
    vSimsOut2 = Exa6_ManTransformOutputs( vSimsOut, nOuts );
    pMini = Exa6_ManGenTest( vSimsDiv2, vSimsOut2, nVars, nDivs, nOuts, nNodes, 0, fOnlyAnd, 0, 0, 0, fVerbose );
    if ( pMini )
    {
        if ( DivCompl || OutCompl )
        {
            pMini = Mini_AigDupCompl( pTemp = pMini, DivCompl, OutCompl );
            Mini_AigStop( pTemp );        
        }
        Mini_AigerWrite( "exa6.aig", pMini, 1 );
        if ( nVars <= 6 )
            Exa_ManMiniVerify( pMini, vSimsDiv, vSimsOut );
        printf( "\n" );
        //Mini_AigStop( pMini );
    }
    Vec_WrdFreeP( &vSimsDiv2 );
    Vec_WrdFreeP( &vSimsOut2 );
    return pMini;
}
void Exa_ManExactSynthesis6( Bmc_EsPar_t * pPars, char * pFileName )
{
    Mini_Aig_t * pMini = NULL;
    Vec_Wrd_t * vSimsDiv = NULL, * vSimsOut = NULL;
    int nDivs, nOuts, nVars = Exa6_ReadFile( pFileName, &vSimsDiv, &vSimsOut, &nDivs, &nOuts );
    if ( nVars == 0 )
        return;
    Exa6_SortSims( vSimsDiv, vSimsOut );
    pMini = Exa_ManExactSynthesis6Int( vSimsDiv, vSimsOut, nVars, nDivs, nOuts, pPars->nNodes, pPars->fOnlyAnd, pPars->fVerbose );
    Vec_WrdFreeP( &vSimsDiv );
    Vec_WrdFreeP( &vSimsOut );
    if ( pMini ) Mini_AigStop( pMini );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

