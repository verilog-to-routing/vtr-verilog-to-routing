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

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAJ_NOBJS  32 // Const0 + Const1 + nVars + nNodes

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
    p->iVar = 1 + p->nNodes * 3;
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
    return p;
}
void Exa_ManFree( Exa_Man_t * p )
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
        int iVarStart = 1 + 3*(i - p->nVars);
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
void Exa_ManPrintSolution( Exa_Man_t * p, int fCompl )
{
    int i, k, iVar;
    printf( "Realization of given %d-input function using %d two-input gates:\n", p->nVars, p->nNodes );
//    for ( i = p->nVars + 2; i < p->nObjs; i++ )
    for ( i = p->nObjs - 1; i >= p->nVars; i-- )
    {
        int iVarStart = 1 + 3*(i - p->nVars);
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
int Exa_ManAddCnfStart( Exa_Man_t * p, int fOnlyAnd )
{
    int pLits[MAJ_NOBJS], pLits2[2], i, j, k, n, m;
    // input constraints
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        int iVarStart = 1 + 3*(i - p->nVars);
        for ( k = 0; k < 2; k++ )
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
            if ( k == 1 )
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
#ifdef USE_NODE_ORDER
        // node ordering
        for ( j = p->nVars; j < i; j++ )
        for ( n = 0;   n < p->nObjs; n++ ) if ( p->VarMarks[i][0][n] )
        for ( m = n+1; m < p->nObjs; m++ ) if ( p->VarMarks[j][0][m] )
        {
            pLits2[0] = Abc_Var2Lit( p->VarMarks[i][0][n], 1 );
            pLits2[1] = Abc_Var2Lit( p->VarMarks[j][0][m], 1 );
            if ( !bmcg_sat_solver_addclause( p->pSat, pLits2, 2 ) )
                return 0;
        }
#endif
        // two input functions
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
        int iVarStart = 1 + 3*(i - p->nVars);
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
                if ( !bmcg_sat_solver_addclause( p->pSat, pLits, nLits ) )
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
        iMint = Exa_ManEval( p );
    }
    if ( iMint == -1 )
        Exa_ManPrintSolution( p, fCompl );
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
        if ( p->pPars->fUseIncr )
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
        if ( status == GLUCOSE_UNSAT )
            break;
        iMint = Exa3_ManEval( p );
    }
    if ( pPars->fVerbose )
        Exa3_ManPrint( p, i, iMint, Abc_Clock() - clkTotal );
    if ( iMint == -1 )
        Exa3_ManPrintSolution( p, fCompl );
    else
        printf( "The problem has no solution.\n" );
    printf( "Added = %d.  Tried = %d.  ", p->nUsed[1], p->nUsed[0] );
    Exa3_ManFree( p );
    Abc_PrintTime( 1, "Total runtime", Abc_Clock() - clkTotal );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

