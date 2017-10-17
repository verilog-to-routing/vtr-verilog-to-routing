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
    //printf( "The number of parameter variables = %d.\n", p->iVar );
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
    static int Flag = 0;
    int i, k, iMint; word * pFanins[3];
    for ( i = p->nVars + 2; i < p->nObjs; i++ )
    {
        for ( k = 0; k < 3; k++ )
            pFanins[k] = Maj_ManTruth( p, Maj_ManFindFanin(p, i, k) );
        Abc_TtMaj( Maj_ManTruth(p, i), pFanins[0], pFanins[1], pFanins[2], p->nWords );
    }
    if ( Flag && p->nVars >= 6 )
        iMint = Abc_TtFindLastDiffBit( Maj_ManTruth(p, p->nObjs-1), Maj_ManTruth(p, p->nObjs), p->nVars );
    else
        iMint = Abc_TtFindFirstDiffBit( Maj_ManTruth(p, p->nObjs-1), Maj_ManTruth(p, p->nObjs), p->nVars );
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
void Maj_ManExactSynthesis( int nVars, int nNodes, int fUseConst, int fUseLine, int fVerbose )
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
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

