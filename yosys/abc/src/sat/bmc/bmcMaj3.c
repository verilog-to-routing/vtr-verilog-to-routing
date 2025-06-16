/**CFile****************************************************************

  FileName    [bmcMaj3.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Majority gate synthesis.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcMaj3.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "misc/extra/extra.h"
#include "misc/util/utilTruth.h"
#include "sat/glucose/AbcGlucose.h"
#include "opt/dau/dau.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MAJ3_OBJS 32 // nVars + nNodes

typedef struct Maj3_Man_t_ Maj3_Man_t;
struct Maj3_Man_t_ 
{
    int               nVars;     // inputs
    int               nNodes;    // internal nodes
    int               nObjs;     // total objects (nVars inputs, nNodes internal nodes)
    int               nWords;    // the truth table size in 64-bit words
    int               iVar;      // the next available SAT variable
    Vec_Wrd_t *       vInfo;     // nVars + nNodes + 1
    Vec_Int_t *       vLevels;   // distriction of nodes by levels
    int               VarMarks[MAJ3_OBJS][MAJ3_OBJS]; // variable marks
    int               ObjVals[MAJ3_OBJS];             // object values
    int               pLits[2][MAJ3_OBJS]; // neg, pos literals
    int               nLits[3];  // neg, pos, fixed literal
    bmcg_sat_solver * pSat;      // SAT solver
};

static inline word *  Maj3_ManTruth( Maj3_Man_t * p, int v ) { return Vec_WrdEntryP( p->vInfo, p->nWords * v ); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Maj3_ManValue( int iMint, int nVars )
{
    int k, Count = 0;
    for ( k = 0; k < nVars; k++ )
        Count += (iMint >> k) & 1;
    return (int)(Count > nVars/2);
}
Vec_Wrd_t * Maj3_ManTruthTables( Maj3_Man_t * p )
{
    Vec_Wrd_t * vInfo = p->vInfo = Vec_WrdStart( p->nWords * (p->nObjs + 1) ); 
    int i, nMints = Abc_MaxInt( 64, 1 << p->nVars );
    for ( i = 0; i < p->nVars; i++ )
        Abc_TtIthVar( Maj3_ManTruth(p, i), i, p->nVars );
    for ( i = 0; i < nMints; i++ )
        if ( Maj3_ManValue(i, p->nVars) )
            Abc_TtSetBit( Maj3_ManTruth(p, p->nObjs), i );
    //Dau_DsdPrintFromTruth( Maj3_ManTruth(p, p->nObjs), p->nVars );
    return vInfo;
}
void Maj3_ManFirstAndLevel( Vec_Int_t * vLevels, int Firsts[MAJ3_OBJS], int Levels[MAJ3_OBJS], int nVars, int nObjs )
{
    int i, k, Entry, Obj = 0;
    Firsts[0] = Obj;
    for ( k = 0; k < nVars; k++ )
        Levels[Obj++] = 0;
    Vec_IntReverseOrder( vLevels );
    Vec_IntForEachEntry( vLevels, Entry, i )
    {
        Firsts[i+1] = Obj;
        for ( k = 0; k < Entry; k++ )
            Levels[Obj++] = i+1;
    }
    Vec_IntReverseOrder( vLevels );
    assert( Obj == nObjs );
}
int Maj3_ManMarkup( Maj3_Man_t * p )
{
    int nSatVars = 2; // SAT variable counter
    int nLevels = Vec_IntSize(p->vLevels);
    int nSecond = Vec_IntEntry(p->vLevels, 1);
    int i, k, Firsts[MAJ3_OBJS], Levels[MAJ3_OBJS];
    assert( Vec_IntEntry(p->vLevels, 0) == 1 );
    assert( p->nObjs <= MAJ3_OBJS );
    assert( p->nNodes == Vec_IntSum(p->vLevels) );
    Maj3_ManFirstAndLevel( p->vLevels, Firsts, Levels, p->nVars, p->nObjs );
    // create map 
    for ( i = 0; i < p->nObjs; i++ )
    for ( k = 0; k < p->nObjs; k++ )
        p->VarMarks[i][k] = -1;
    for ( k = 0; k < 3; k++ ) // first node
        p->VarMarks[p->nVars][k] = 1;
    for ( k = 0; k < nSecond; k++ ) // top node
        p->VarMarks[p->nObjs-1][p->nObjs-2-k] = 1;
    for ( k = 2; k < nLevels; k++ ) // cascade
        p->VarMarks[Firsts[k]][Firsts[k-1]] = 1;
    for ( i = p->nVars+1; i < (nSecond == 3 ? p->nObjs-1 : p->nObjs); i++ )
        for ( k = 0; k < Firsts[Levels[i]]; k++ )
            if ( p->VarMarks[i][k] == -1 )
                p->VarMarks[i][k] = nSatVars++;
    //printf( "The number of variables is %d.\n", nSatVars );
    return nSatVars;
}
void Maj3_ManVarMapPrint( Maj3_Man_t * p )
{
    int i, k, Firsts[MAJ3_OBJS], Levels[MAJ3_OBJS];
    Maj3_ManFirstAndLevel( p->vLevels, Firsts, Levels, p->nVars, p->nObjs );
    // print
    printf( "Variable map for problem with %d inputs, %d nodes and %d levels: ", p->nVars, p->nNodes, Vec_IntSize(p->vLevels) );
    Vec_IntPrint( p->vLevels );
    printf( "    " );
    printf( "      " );
    for ( i = 0; i < p->nObjs; i++ )
        printf( "%3d  ", i );
    printf( "\n" );
    for ( i = p->nObjs-1; i >= p->nVars; i-- )
    {
        printf( " %2d ", i );
        printf( " %2d   ", Levels[i] );
        for ( k = 0; k < p->nObjs; k++ )
            if ( p->VarMarks[i][k] == -1 )
                printf( "  .  " );
            else if ( p->VarMarks[i][k] == 1 )
                printf( "  +  " );
            else 
                printf( "%3d%c ", p->VarMarks[i][k], bmcg_sat_solver_read_cex_varvalue(p->pSat, p->VarMarks[i][k]) ? '+' : ' ' );
        printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Maj3_ManFindFanin( Maj3_Man_t * p, int i, int * pFanins )
{
    int f, nFanins = 0;
    p->nLits[0] = p->nLits[1] = p->nLits[2] = 0;
    for ( f = 0; f < i; f++ )
    {
        if ( p->VarMarks[i][f] < 0 )
            continue;
        assert( p->VarMarks[i][f] > 0 );
        if ( p->VarMarks[i][f] == 1 ) // fixed
        {
            p->nLits[2]++;
            pFanins[nFanins++] = f;
        }
        else if ( bmcg_sat_solver_read_cex_varvalue(p->pSat, p->VarMarks[i][f]) ) // pos
        {
            p->pLits[1][p->nLits[1]++] = Abc_Var2Lit(p->VarMarks[i][f], 1);
            pFanins[nFanins++] = f;
        }
        else // neg
            p->pLits[0][p->nLits[0]++] = Abc_Var2Lit(p->VarMarks[i][f], 0);
    }
    return nFanins;
}
static inline void Maj3_ManPrintSolution( Maj3_Man_t * p )
{
    int i, k, iVar, nFanins, pFanins[MAJ3_OBJS];
    printf( "Realization of %d-input majority using %d MAJ3 gates:\n", p->nVars, p->nNodes );
//    for ( i = p->nVars; i < p->nObjs; i++ )
    for ( i = p->nObjs - 1; i >= p->nVars; i-- )
    {
        printf( "%02d = MAJ(", i );
        nFanins = Maj3_ManFindFanin( p, i, pFanins );
        assert( nFanins == 3 );
        for ( k = 0; k < 3; k++ )
        {
            iVar = pFanins[k];
            if ( iVar >= 0 && iVar < p->nVars )
                printf( " %c", 'a'+iVar );
            else
                printf( " %02d", iVar );
        }
        printf( " )\n" );
    }
}
static inline int Maj3_ManEval( Maj3_Man_t * p )
{
    int fUseMiddle = 1;
    static int Flag = 0;
    int i, k, iMint, pFanins[3]; word * pFaninsW[3];
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        int nFanins = Maj3_ManFindFanin( p, i, pFanins );
        assert( nFanins == 3 );
        for ( k = 0; k < 3; k++ )
            pFaninsW[k] = Maj3_ManTruth( p, pFanins[k] );
        Abc_TtMaj( Maj3_ManTruth(p, i), pFaninsW[0], pFaninsW[1], pFaninsW[2], p->nWords );
    }
    if ( fUseMiddle )
    {
        iMint = -1;
        for ( i = 0; i < (1 << p->nVars); i++ )
        {
            int nOnes = Abc_TtBitCount16(i);
            if ( nOnes < p->nVars/2 || nOnes > p->nVars/2+1 )
                continue;
            if ( Abc_TtGetBit(Maj3_ManTruth(p, p->nObjs), i) == Abc_TtGetBit(Maj3_ManTruth(p, p->nObjs-1), i) )
                continue;
            iMint = i;
            break;
        }
    }
    else
    {
        if ( Flag && p->nVars >= 6 )
            iMint = Abc_TtFindLastDiffBit( Maj3_ManTruth(p, p->nObjs-1), Maj3_ManTruth(p, p->nObjs), p->nVars );
        else
            iMint = Abc_TtFindFirstDiffBit( Maj3_ManTruth(p, p->nObjs-1), Maj3_ManTruth(p, p->nObjs), p->nVars );
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
void Maj3_PrintClause( int * pLits, int nLits )
{
    int i;
    for ( i = 0; i < nLits; i++ )
        printf( "%c%d ", Abc_LitIsCompl(pLits[i]) ? '-' : '+', Abc_Lit2Var(pLits[i]) );
    printf( "\n" );
}
int Maj3_ManAddCnfStart( Maj3_Man_t * p )
{
    int i, k, status, nLits, pLits[MAJ3_OBJS];
    // nodes have at least one fanin
    //printf( "Fanin clauses:\n" );
    for ( i = p->nVars; i < p->nObjs; i++ )
    {
        // check if it is already connected
        int Count = 0;
        for ( k = 0; k < p->nObjs; k++ ) 
            Count += p->VarMarks[i][k] == 1;
        assert( Count <= 3 );
        if ( Count == 3 )
            continue;
        // collect connections
        nLits = 0;
        for ( k = 0; k < p->nObjs; k++ ) 
            if ( p->VarMarks[i][k] > 1 )
                pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][k], 0 );
        //Maj3_PrintClause( pLits, nLits );
        assert( nLits > 0 );
        if ( nLits > 0 && !bmcg_sat_solver_addclause( p->pSat, pLits, nLits ) )
            assert(0);
    }
    // nodes have at least one fanout
    //printf( "Fanout clauses:\n" );
    for ( k = 0; k < p->nObjs-1; k++ )
    {
        // check if it is already connected
        int Count = 0;
        for ( i = 0; i < p->nObjs; i++ ) 
            Count += p->VarMarks[i][k] == 1;
        assert( Count <= 3 );
        if ( Count > 0 )
            continue;
        // collect connections
        nLits = 0;
        for ( i = 0; i < p->nObjs; i++ ) 
            if ( p->VarMarks[i][k] > 1 )
                pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][k], 0 );
        //Maj3_PrintClause( pLits, nLits );
        //assert( nLits > 0 );
        if ( nLits > 0 && !bmcg_sat_solver_addclause( p->pSat, pLits, nLits ) )
            assert(0);
    }
    status = bmcg_sat_solver_solve( p->pSat, NULL, 0 );
    assert( status == GLUCOSE_SAT );
    Maj3_ManVarMapPrint( p );
    return 1;
}
int Maj3_ManAddCnf( Maj3_Man_t * p, int iMint )
{
    // input values
    int i, k, j, n, * pVals = p->ObjVals;
    for ( i = 0; i < p->nVars; i++ )
        pVals[i] = (iMint >> i) & 1;
    // first node value
    pVals[p->nVars] = (pVals[0] && pVals[1]) || (pVals[0] && pVals[2]) || (pVals[1] && pVals[2]);
    // last node value
    pVals[p->nObjs-1] = Maj3_ManValue(iMint, p->nVars);
    // intermediate node values
    for ( i = p->nVars + 1; i < p->nObjs - 1; i++ )
        pVals[i] = p->iVar++;
    //printf( "Adding clauses for minterm %d.\n", iMint );
    bmcg_sat_solver_set_nvars( p->pSat, p->iVar );
    for ( n = 0; n < 2; n++ )
    for ( i = p->nVars + 1; i < p->nObjs; i++ )
    {
        //printf( "\nNode %d. Phase %d.\n", i, n );
        for ( k = 0; k < p->nObjs; k++ ) if ( p->VarMarks[i][k] >= 1 )
        {
            // add first input
            int pLits[5], nLits = 0;
            if ( pVals[k] == !n )
                continue;
            if ( pVals[k] > 1 )
                pLits[nLits++] = Abc_Var2Lit( pVals[k], n );
            if ( p->VarMarks[i][k] > 1 )
                pLits[nLits++] = Abc_Var2Lit( p->VarMarks[i][k], 1 );
            for ( j = k+1; j < p->nObjs; j++ ) if ( p->VarMarks[i][j] >= 1 )
            {
                // add second input
                int nLits2 = nLits;
                if ( pVals[j] == !n )
                    continue;
                if ( pVals[j] > 1 )
                    pLits[nLits2++] = Abc_Var2Lit( pVals[j], n );
                if ( p->VarMarks[i][j] > 1 )
                    pLits[nLits2++] = Abc_Var2Lit( p->VarMarks[i][j], 1 );
                // add output
                if ( pVals[i] == n )
                    continue;
                if ( pVals[i] > 1 )
                    pLits[nLits2++] = Abc_Var2Lit( pVals[i], !n );
                assert( nLits2 > 0 && nLits2 <= 5 );
                //Maj3_PrintClause( pLits, nLits2 );
                if ( !bmcg_sat_solver_addclause( p->pSat, pLits, nLits2 ) )
                    return 0;
            }
        }
    }
    return 1;
}
int Maj3_ManAddConstraintsLazy( Maj3_Man_t * p )
{
    int i, pFanins[MAJ3_OBJS], nConstr = 0;
    //Maj3_ManVarMapPrint( p );
    for ( i = p->nVars+1; i < p->nObjs; i++ )
    {
        int nFanins = Maj3_ManFindFanin( p, i, pFanins );
        if ( nFanins == 3 )
            continue;
        //printf( "Node %d has %d fanins.\n", i, nFanins );
        nConstr++;
        if ( nFanins < 3 )
        {
            assert( p->nLits[0] > 0 );
            //Maj3_PrintClause( p->pLits[0], p->nLits[0] );
            if ( !bmcg_sat_solver_addclause( p->pSat, p->pLits[0], p->nLits[0] ) )
                return -1;
        }
        else if ( nFanins > 3 )
        {
            int nLits = Abc_MinInt(4 - p->nLits[2], p->nLits[1]);
            assert( nLits > 0 );
            //Maj3_PrintClause( p->pLits[1], nLits );
            if ( !bmcg_sat_solver_addclause( p->pSat, p->pLits[1], nLits ) )
                return -1;
        }
    }
    return nConstr;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Maj3_Man_t * Maj3_ManAlloc( int nVars, int nNodes, Vec_Int_t * vLevels )
{
    Maj3_Man_t * p = ABC_CALLOC( Maj3_Man_t, 1 );
    p->vLevels    = vLevels;
    p->nVars      = nVars;
    p->nNodes     = nNodes;
    p->nObjs      = nVars + nNodes;
    p->nWords     = Abc_TtWordNum(nVars);
    p->iVar       = Maj3_ManMarkup( p );
    p->vInfo      = Maj3_ManTruthTables( p );
    p->pSat       = bmcg_sat_solver_start();
    bmcg_sat_solver_set_nvars( p->pSat, p->iVar );
    Maj3_ManAddCnfStart( p );
    return p;
}
void Maj3_ManFree( Maj3_Man_t * p )
{
    bmcg_sat_solver_stop( p->pSat );
    Vec_WrdFree( p->vInfo );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Maj3_ManExactSynthesis( int nVars, int nNodes, int fVerbose, Vec_Int_t * vLevels )
{
    Maj3_Man_t * p;  abctime clkTotal = Abc_Clock();
    int i, status, nLazy, nLazyAll = 0, iMint = 0;
    printf( "Running exact synthesis for %d-input majority with %d MAJ3 gates...\n", nVars, nNodes );
    p = Maj3_ManAlloc( nVars, nNodes, vLevels );
    for ( i = 0; iMint != -1; i++ )
    {
        abctime clk = Abc_Clock();
        if ( !Maj3_ManAddCnf( p, iMint ) )
            break;
        while ( (status = bmcg_sat_solver_solve(p->pSat, NULL, 0)) == GLUCOSE_SAT )
        {
            nLazy = Maj3_ManAddConstraintsLazy( p );
            if ( nLazy == -1 )
            {
                printf( "Became UNSAT after adding lazy constraints.\n" );
                status = GLUCOSE_UNSAT;
                break;
            }
            //printf( "Added %d lazy constraints.\n\n", nLazy );
            if ( nLazy == 0 )
                break;
            nLazyAll += nLazy;
        }
        if ( fVerbose )
        {
            printf( "Iter %3d : ", i );
            Extra_PrintBinary( stdout, (unsigned *)&iMint, p->nVars );
            printf( "  Var =%5d  ", p->iVar );
            printf( "Cla =%6d  ", bmcg_sat_solver_clausenum(p->pSat) );
            printf( "Conf =%9d  ", bmcg_sat_solver_conflictnum(p->pSat) );
            printf( "Lazy =%9d  ", nLazyAll );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        }
        if ( status == GLUCOSE_UNSAT )
        {
            printf( "The problem has no solution.\n" );
            break;
        }
        iMint = Maj3_ManEval( p );
    }
    if ( iMint == -1 )
        Maj3_ManPrintSolution( p );
    Maj3_ManFree( p );
    Abc_PrintTime( 1, "Total runtime", Abc_Clock() - clkTotal );
    return iMint == -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Maj3_ManTest()
{
/*
    int nVars    =  5;
    int nNodes   =  4;
    int fVerbose =  1;
    int Levels[MAJ3_OBJS] = { 1, 2, 1 };
    Vec_Int_t vLevels = { 3, 3, Levels };
*/

    int nVars    =  7;
    int nNodes   =  7;
    int fVerbose =  1;
    int Levels[MAJ3_OBJS] = { 1, 2, 2, 2 };
    Vec_Int_t vLevels = { 4, 4, Levels };

/*
    int nVars    =  9;
    int nNodes   = 10;
    int fVerbose =  1;
    int Levels[MAJ3_OBJS] = { 1, 2, 3, 2, 2 };
    Vec_Int_t vLevels = { 5, 5, Levels };
*/
/*
    int nVars    =  9;
    int nNodes   = 10;
    int fVerbose =  1;
    int Levels[MAJ3_OBJS] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    Vec_Int_t vLevels = { 10, 10, Levels };
*/

//    Maj3_Man_t * p = Maj3_ManAlloc( nVars, nNodes, &vLevels );
//    Maj3_ManFree( p );
    Maj3_ManExactSynthesis( nVars, nNodes, fVerbose, &vLevels );
    return 0;
}





typedef struct Zyx_Man_t_ Zyx_Man_t;
struct Zyx_Man_t_ 
{
    Bmc_EsPar_t *     pPars;      // parameters
    word *            pTruth;     // truth table
    int               nObjs;      // total objects (nVars inputs, nNodes internal nodes)
    int               nWords;     // the truth table size in 64-bit words
    int               LutMask;    // 1 << nLutSize
    int               TopoBase;   // (1 << nLutSize) * nObjs
    int               MintBase;   // TopoBase + nObjs * nObjs
    Vec_Wrd_t *       vInfo;      // nVars + nNodes + 1
    Vec_Int_t *       vVarValues; // variable values
    Vec_Int_t *       vMidMints;  // middle minterms
    Vec_Bit_t *       vUsed2;     // bit masks
    Vec_Bit_t *       vUsed3;     // bit masks
    Vec_Int_t *       vPairs;     // sym var pairs
    int               nUsed[2];
    int               pFanins[MAJ3_OBJS][MAJ3_OBJS];  // fanins
    int               pLits[2][2*MAJ3_OBJS]; // neg/pos literals
    int               nLits[2];   // neg/pos literal
    int               Counts[1024];
    bmcg_sat_solver * pSat;       // SAT solver
    abctime           clkEval;
};

static inline word *  Zyx_ManTruth( Zyx_Man_t * p, int v )        { return Vec_WrdEntryP( p->vInfo, p->nWords * v ); }

static inline int     Zyx_FuncVar( Zyx_Man_t * p, int i, int m )  { return (p->LutMask + 1) * (i - p->pPars->nVars) + m;         }
static inline int     Zyx_TopoVar( Zyx_Man_t * p, int i, int f )  { return p->TopoBase + p->nObjs * (i - p->pPars->nVars) + f;   }
static inline int     Zyx_MintVar( Zyx_Man_t * p, int m, int i )  { return p->MintBase + p->nObjs * m + i;                       }

//static inline int     Zyx_MintVar2( Zyx_Man_t * p, int m, int i, int f ) { assert(i >= p->pPars->nVars); return p->MintBase + m * p->pPars->nNodes * (p->pPars->nLutSize + 1) + (i - p->pPars->nVars) * (p->pPars->nLutSize + 1) + f; }

static inline int     Zyx_ManIsUsed2( Zyx_Man_t * p, int m, int n, int i, int j )
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

static inline int     Zyx_ManIsUsed3( Zyx_Man_t * p, int m, int n, int i, int j, int k )
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
void Zyx_SetConstVar( Zyx_Man_t * p, int Var, int Value )
{
    int iLit = Abc_Var2Lit( Var, !Value );
    int status = bmcg_sat_solver_addclause( p->pSat, &iLit, 1 );
    assert( status );
    assert( Vec_IntEntry(p->vVarValues, Var) == -1 );
    Vec_IntWriteEntry( p->vVarValues, Var, Value );
    //printf( "Setting var %d to value %d.\n", Var, Value );
}
void Zyx_ManSetupVars( Zyx_Man_t * p )
{
    int i, k, m;
    word * pSpec = p->pPars->fMajority ? Zyx_ManTruth(p, p->nObjs) : p->pTruth;
    // set unused functionality vars to 0
    for ( i = p->pPars->nVars; i < p->nObjs; i++ )
        Zyx_SetConstVar( p, Zyx_FuncVar(p, i, 0), 0 );
    // set unused topology vars
    for ( i = p->pPars->nVars; i < p->nObjs; i++ )
        for ( k = i; k < p->nObjs; k++ )
            Zyx_SetConstVar( p, Zyx_TopoVar(p, i, k), 0 );
    // connect topmost node
    Zyx_SetConstVar( p, Zyx_TopoVar(p, p->nObjs-1, p->nObjs-2), 1 );
    // connect first node
    if ( p->pPars->fMajority )
        for ( k = 0; k < p->pPars->nVars; k++ )
            Zyx_SetConstVar( p, Zyx_TopoVar(p, p->pPars->nVars, k), k < 3 );
    // set minterm vars
    for ( m = 0; m < (1 << p->pPars->nVars); m++ )
    {
        for ( i = 0; i < p->pPars->nVars; i++ )
            Zyx_SetConstVar( p, Zyx_MintVar(p, m, i), (m >> i) & 1 );
        Zyx_SetConstVar( p, Zyx_MintVar(p, m, p->nObjs-1), Abc_TtGetBit(pSpec, m) );
    }
}
int Zyx_ManAddCnfStart( Zyx_Man_t * p )
{
    // nodes have fanins
    int i, k, pLits[MAJ3_OBJS];
    //printf( "Adding initial clauses:\n" );
    for ( i = p->pPars->nVars; i < p->nObjs; i++ )
    {
        int nLits = 0;
        for ( k = 0; k < i; k++ )
            pLits[nLits++] = Abc_Var2Lit( Zyx_TopoVar(p, i, k), 0 );
        assert( nLits > 0 );
        if ( !bmcg_sat_solver_addclause( p->pSat, pLits, nLits ) )
            return 0;
    }
 
    // nodes have fanouts
    for ( k = 0; k < p->nObjs-1; k++ )
    {
        int nLits = 0;
        for ( i = p->pPars->nVars; i < p->nObjs; i++ )
            pLits[nLits++] = Abc_Var2Lit( Zyx_TopoVar(p, i, k), 0 );
        assert( nLits > 0 );
        if ( !bmcg_sat_solver_addclause( p->pSat, pLits, nLits ) )
            return 0;
    }
 
    // two input functions
    if ( p->pPars->nLutSize != 2 )
        return 1;
    for ( i = p->pPars->nVars; i < p->nObjs; i++ )
    {
        for ( k = 0; k < 3; k++ )
        {
            pLits[0] = Abc_Var2Lit( Zyx_FuncVar(p, i, 1), k==1 );
            pLits[1] = Abc_Var2Lit( Zyx_FuncVar(p, i, 2), k==2 );
            pLits[2] = Abc_Var2Lit( Zyx_FuncVar(p, i, 3), k!=0 );
            if ( !bmcg_sat_solver_addclause( p->pSat, pLits, 3 ) )
                return 0;
        }
        if ( p->pPars->fOnlyAnd )
        {
            pLits[0] = Abc_Var2Lit( Zyx_FuncVar(p, i, 1), 1 );
            pLits[1] = Abc_Var2Lit( Zyx_FuncVar(p, i, 2), 1 );
            pLits[2] = Abc_Var2Lit( Zyx_FuncVar(p, i, 3), 0 );
            if ( !bmcg_sat_solver_addclause( p->pSat, pLits, 3 ) )
                return 0;
        }
    }
    return 1;
}
void Zyx_ManPrintVarMap( Zyx_Man_t * p, int fSat )
{
    int i, k, nTopoVars = 0;
    printf( "      " );
    for ( k = 0; k < p->nObjs-1; k++ )
        printf( "%3d  ", k );
    printf( "\n" );
    for ( i = p->nObjs-1; i >= p->pPars->nVars; i-- )
    {
        printf( "%3d   ", i );
        for ( k = 0; k < p->nObjs-1; k++ )
        {
            int iVar = Zyx_TopoVar(p, i, k);
            if ( Vec_IntEntry(p->vVarValues, iVar) == -1 )
                printf( "%3d%c ", iVar, (fSat && bmcg_sat_solver_read_cex_varvalue(p->pSat, iVar)) ? '*' : ' ' ), nTopoVars++;
            else
                printf( "%3d  ", Vec_IntEntry(p->vVarValues, iVar) );
        }
        printf( "\n" );
    }
    if ( fSat ) return;
    printf( "Using %d active functionality vars and %d active topology vars (out of %d SAT vars).\n", 
        p->pPars->fMajority ? 0 : p->pPars->nNodes * p->LutMask, nTopoVars, bmcg_sat_solver_varnum(p->pSat) );
}
void Zyx_PrintClause( int * pLits, int nLits )
{
    int i;
    for ( i = 0; i < nLits; i++ )
        printf( "%c%d ", Abc_LitIsCompl(pLits[i]) ? '-' : '+', Abc_Lit2Var(pLits[i]) );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Zyx_ManValue( int iMint, int nVars )
{
    int k, Count = 0;
    for ( k = 0; k < nVars; k++ )
        Count += (iMint >> k) & 1;
    return (int)(Count > nVars/2);
}
Vec_Wrd_t * Zyx_ManTruthTables( Zyx_Man_t * p, word * pTruth )
{
    Vec_Wrd_t * vInfo = p->vInfo = Vec_WrdStart( p->nWords * (p->nObjs + 1) ); 
    int i, nMints = Abc_MaxInt( 64, 1 << p->pPars->nVars );
    assert( p->pPars->fMajority == (pTruth == NULL) );
    for ( i = 0; i < p->pPars->nVars; i++ )
        Abc_TtIthVar( Zyx_ManTruth(p, i), i, p->pPars->nVars );
    if ( p->pPars->fMajority )
    {
        for ( i = 0; i < nMints; i++ )
            if ( Zyx_ManValue(i, p->pPars->nVars) )
                Abc_TtSetBit( Zyx_ManTruth(p, p->nObjs), i );
        for ( i = 0; i < nMints; i++ )
            if ( Abc_TtBitCount16(i) == p->pPars->nVars/2 || Abc_TtBitCount16(i) == p->pPars->nVars/2+1 )
                Vec_IntPush( p->vMidMints, i );
    }
    //Dau_DsdPrintFromTruth( Zyx_ManTruth(p, p->nObjs), p->pPars->nVars );
    return vInfo;
}
Vec_Int_t * Zyx_ManCreateSymVarPairs( word * pTruth, int nVars )
{
    Vec_Int_t * vPairs = Vec_IntAlloc( 100 ); 
    int i, k, nWords = Abc_TtWordNum(nVars);
    word Cof0[64], Cof1[64]; 
    word Cof01[64], Cof10[64];
    assert( nVars <= 12 );
    for ( i = 0; i < nVars; i++ )
    {
        Abc_TtCofactor0p( Cof0, pTruth, nWords, i );
        Abc_TtCofactor1p( Cof1, pTruth, nWords, i );
        for ( k = i+1; k < nVars; k++ )
        {
            Abc_TtCofactor1p( Cof01, Cof0, nWords, k );
            Abc_TtCofactor0p( Cof10, Cof1, nWords, k );
            if ( Abc_TtEqual( Cof01, Cof10, nWords ) )
                 Vec_IntPushTwo( vPairs, i, k );
        }
    }
    return vPairs;
}
Zyx_Man_t * Zyx_ManAlloc( Bmc_EsPar_t * pPars, word * pTruth )
{
    Zyx_Man_t * p = ABC_CALLOC( Zyx_Man_t, 1 );
    p->pPars      = pPars;
    p->pTruth     = pTruth;
    p->nObjs      = p->pPars->nVars + p->pPars->nNodes;
    p->nWords     = Abc_TtWordNum(p->pPars->nVars);
    p->LutMask    = (1 << p->pPars->nLutSize) - 1;
    p->TopoBase   = (1 << p->pPars->nLutSize) * p->pPars->nNodes;
    p->MintBase   = p->TopoBase + p->pPars->nNodes * p->nObjs;
    p->vVarValues = Vec_IntStartFull( p->MintBase + (1 << p->pPars->nVars) * p->nObjs );
    p->vMidMints  = Vec_IntAlloc( 1 << p->pPars->nVars );
    p->vInfo      = Zyx_ManTruthTables( p, pTruth );
    p->vPairs     = Zyx_ManCreateSymVarPairs( p->pPars->fMajority ? Zyx_ManTruth(p, p->nObjs) : pTruth, p->pPars->nVars );
    p->pSat       = bmcg_sat_solver_start();
    if ( pPars->fUseIncr )
    {
        if ( p->pPars->nLutSize == 2 || p->pPars->fMajority )
            p->vUsed2     = Vec_BitStart( (1 << p->pPars->nVars) * p->pPars->nNodes * p->nObjs * p->nObjs );
        else if ( p->pPars->nLutSize == 3 )
            p->vUsed3     = Vec_BitStart( (1 << p->pPars->nVars) * p->pPars->nNodes * p->nObjs * p->nObjs * p->nObjs );
    }
    bmcg_sat_solver_set_nvars( p->pSat, p->MintBase + (1 << p->pPars->nVars) * p->nObjs );
    Zyx_ManSetupVars( p );
    Zyx_ManAddCnfStart( p );
    Zyx_ManPrintVarMap( p, 0 );
    return p;
}
void Zyx_ManFree( Zyx_Man_t * p )
{
    bmcg_sat_solver_stop( p->pSat );
    Vec_WrdFree( p->vInfo );
    Vec_BitFreeP( &p->vUsed2 );
    Vec_BitFreeP( &p->vUsed3 );
    Vec_IntFree( p->vPairs );
    Vec_IntFree( p->vMidMints );
    Vec_IntFree( p->vVarValues );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Zyx_ManCollectFanins( Zyx_Man_t * p, int i )
{
    int k, Val;
    assert( i >= p->pPars->nVars && i < p->nObjs );
    p->nLits[0] = p->nLits[1] = 0;
    for ( k = 0; k < i; k++ )
    {
        Val = bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, i, k));
        p->pFanins[i][p->nLits[1]] = k;
        p->pLits[Val][p->nLits[Val]++] = Abc_Var2Lit(Zyx_TopoVar(p, i, k), Val);
    }
    return p->nLits[1];
}
int Zyx_ManAddCnfLazyTopo( Zyx_Man_t * p )
{
    int i, k, j, Entry[2], Node[2], nLazy = 0;
    // fanin count
    //printf( "Adding topology clauses.\n" );
    for ( i = p->pPars->nVars; i < p->nObjs; i++ )
    {
        int nFanins = Zyx_ManCollectFanins( p, i );
        if ( nFanins == p->pPars->nLutSize )
            continue;
        nLazy++;
        assert( nFanins == p->nLits[1] );
        if ( p->nLits[1] > p->pPars->nLutSize )
        {
            p->nLits[1] = p->pPars->nLutSize + 1;
            //Zyx_PrintClause( p->pLits[1], p->nLits[1] );
            if ( !bmcg_sat_solver_addclause( p->pSat, p->pLits[1], p->nLits[1] ) )
                return -1;
        }
        else // if ( p->nLits[1] < p->pPars->nLutSize )
        {
            //Zyx_PrintClause( p->pLits[0], p->nLits[0] );
            if ( !bmcg_sat_solver_addclause( p->pSat, p->pLits[0], p->nLits[0] ) )
                return -1;
        }
    }
    if ( nLazy || !p->pPars->fOrderNodes )
        return nLazy;
    // ordering
    for ( i = p->pPars->nVars + 1; i < p->nObjs; i++ )
    {
        for ( k = p->pPars->nLutSize - 1; k >= 0; k-- )
            if ( p->pFanins[i-1][k] != p->pFanins[i][k] )
                break;
        if ( k == -1 ) // fanins are equal
        {
            if ( p->pPars->fMajority )
                continue;
            // compare by LUT functions
            for ( k = p->LutMask; k >= 0; k-- )
                if ( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_FuncVar(p, i-1, k)) != 
                     bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_FuncVar(p, i  , k)) )
                    break;
            if ( k == -1 ) // truth tables cannot be equal
                continue;
            // rule out these truth tables
            if ( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_FuncVar(p, i-1, k)) == 0 && 
                 bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_FuncVar(p, i  , k)) == 1 )
            {
                continue;
            }
            nLazy++;
            assert( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_FuncVar(p, i-1, k)) == 1 );
            assert( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_FuncVar(p, i  , k)) == 0 );
            // rule out this order
            p->nLits[0] = 0;
            for ( j = p->LutMask; j >= k; j-- )
            {
                int ValA = bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_FuncVar(p, i-1, j));
                int ValB = bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_FuncVar(p, i,   j));
                p->pLits[0][p->nLits[0]++] = Abc_Var2Lit(Zyx_FuncVar(p, i-1, j), ValA );
                p->pLits[0][p->nLits[0]++] = Abc_Var2Lit(Zyx_FuncVar(p, i,   j), ValB );
            }
            if ( !bmcg_sat_solver_addclause( p->pSat, p->pLits[0], p->nLits[0] ) )
                return -1;
            continue;
        }
        if ( p->pFanins[i-1][k] < p->pFanins[i][k] )
        {
            assert( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, i-1, p->pFanins[i][k])) == 0 );
            assert( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, i  , p->pFanins[i][k])) == 1 );
            continue;
        }
        nLazy++;
        assert( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, i-1, p->pFanins[i-1][k])) == 1 );
        assert( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, i  , p->pFanins[i-1][k])) == 0 );
        // rule out this order
        p->nLits[0] = 0;
        for ( j = p->pFanins[i-1][k]; j < p->nObjs-1; j++ )
        {
            int ValA = bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, i-1, j));
            int ValB = bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, i,   j));
            p->pLits[0][p->nLits[0]++] = Abc_Var2Lit(Zyx_TopoVar(p, i-1, j), ValA );
            p->pLits[0][p->nLits[0]++] = Abc_Var2Lit(Zyx_TopoVar(p, i,   j), ValB );
        }
        //printf( "\n" );
        //Zyx_ManPrintVarMap( p, 1 );
        //Zyx_PrintClause( p->pLits[0], p->nLits[0] );
        if ( !bmcg_sat_solver_addclause( p->pSat, p->pLits[0], p->nLits[0] ) )
            return -1;
        //break;
    }
    // check symmetric variables
    Vec_IntForEachEntryDouble( p->vPairs, Entry[0], Entry[1], k )
    {
        assert( Entry[0] < Entry[1] );
        for ( j = 0; j < 2; j++ )
        {
            Node[j] = -1;
            for ( i = p->pPars->nVars; i < p->nObjs; i++ )
                if ( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, i, Entry[j])) )
                {
                    Node[j] = i;
                    break;
                }
            assert( Node[j] >= p->pPars->nVars );
        }
        // compare the nodes
        if ( Node[0] <= Node[1] )
            continue;
        assert( Node[0] > Node[1] );
        // create blocking clause
        nLazy++;
        assert( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, Node[1], Entry[0])) == 0 );
        assert( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, Node[1], Entry[1])) == 1 );
        // rule out this order
        p->nLits[0] = 0;
        for ( j = p->pPars->nVars; j <= Node[1]; j++ )
        {
            int ValA = bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, j, Entry[0]));
            int ValB = bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, j, Entry[1]));
            p->pLits[0][p->nLits[0]++] = Abc_Var2Lit(Zyx_TopoVar(p, j, Entry[0]), ValA );
            p->pLits[0][p->nLits[0]++] = Abc_Var2Lit(Zyx_TopoVar(p, j, Entry[1]), ValB );
        }
        if ( !bmcg_sat_solver_addclause( p->pSat, p->pLits[0], p->nLits[0] ) )
            return -1;
    }
    return nLazy;
}
int Zyx_ManAddCnfBlockSolution( Zyx_Man_t * p )
{
    Vec_Int_t * vLits = Vec_IntAlloc( 100 ); int i, k;
    for ( i = p->pPars->nVars; i < p->nObjs; i++ )
    {
        int nFanins = Zyx_ManCollectFanins( p, i );
        assert( nFanins == p->pPars->nLutSize );
        for ( k = 0; k < p->pPars->nLutSize; k++ )
            Vec_IntPush( vLits, Abc_Var2Lit(Zyx_TopoVar(p, i, p->pFanins[i][k]), 1) );
    }
    //Zyx_ManPrintVarMap( p, 1 );
    //Zyx_PrintClause( Vec_IntArray(vLits), Vec_IntSize(vLits) );
    if ( !bmcg_sat_solver_addclause( p->pSat, Vec_IntArray(vLits), Vec_IntSize(vLits) ) )
        return 0;
    Vec_IntFree( vLits );
    return 1;
}
int Zyx_ManAddCnfLazyFunc2( Zyx_Man_t * p, int iMint )
{
    int i, k, n, j, s;
    assert( !p->pPars->fMajority || p->pPars->nLutSize == 3 );
    //printf( "Adding functionality clauses for minterm %d.\n", iMint );
    p->Counts[iMint]++;
    for ( i = p->pPars->nVars; i < p->nObjs; i++ )
    {
        int nFanins = Zyx_ManCollectFanins( p, i );
        assert( nFanins == p->pPars->nLutSize );
        if ( p->pPars->fMajority )
        {
            int Sets[3][2] = {{0, 1}, {0, 2}, {1, 2}};
            for ( k = 0; k < 3; k++ )
            {
                if ( Zyx_ManIsUsed2(p, iMint, i, p->pFanins[i][Sets[k][0]], p->pFanins[i][Sets[k][1]]) )
                    continue;
                for ( n = 0; n < 2; n++ )
                {
                    p->nLits[0] = 0;
                    for ( s = 0; s < 2; s++ )
                    {
                        p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_TopoVar(p, i, p->pFanins[i][Sets[k][s]]), 1 );
                        p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_MintVar(p, iMint, p->pFanins[i][Sets[k][s]]), n );
                    }
                    p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_MintVar(p, iMint, i), !n );
                    if ( !bmcg_sat_solver_addclause( p->pSat, p->pLits[0], p->nLits[0] ) )
                        return 0;
                }
            }
        }
        else
        {
            if ( p->pPars->nLutSize == 2 && Zyx_ManIsUsed2(p, iMint, i, p->pFanins[i][0], p->pFanins[i][1]) )
                continue;
            if ( p->pPars->nLutSize == 3 && Zyx_ManIsUsed3(p, iMint, i, p->pFanins[i][0], p->pFanins[i][1], p->pFanins[i][2]) )
                continue;
            for ( k = 0; k <= p->LutMask; k++ )
            for ( n = 0; n < 2; n++ )
            {
                p->nLits[0] = 0;
                p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_FuncVar(p, i, k), n );
                for ( j = 0; j < p->pPars->nLutSize; j++ )
                {
                    p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_TopoVar(p, i, p->pFanins[i][j]), 1 );
                    p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_MintVar(p, iMint, p->pFanins[i][j]), (k >> j) & 1 );
                }
                p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_MintVar(p, iMint, i), !n );
                if ( !bmcg_sat_solver_addclause( p->pSat, p->pLits[0], p->nLits[0] ) )
                    return 0;
            }
        }
    }
    return 1;
}

int Zyx_ManAddCnfLazyFunc( Zyx_Man_t * p, int iMint )
{
    int i, k, n, j, s, t, u;
    //printf( "Adding clauses for minterm %d with value %d.\n", iMint, Value );
    assert( !p->pPars->fMajority && p->pPars->nLutSize < 4 );
    p->Counts[iMint]++;
    if ( p->pPars->nLutSize == 2 )
    {
        for ( i = p->pPars->nVars; i < p->nObjs; i++ )
        for ( s = 0;               s < i;        s++ )
        for ( t = s+1;             t < i;        t++ )
        {
            p->pFanins[i][0] = s;
            p->pFanins[i][1] = t;
            for ( k = 0; k <= p->LutMask; k++ )
            for ( n = 0; n < 2; n++ )
            {
                p->nLits[0] = 0;
                p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_FuncVar(p, i, k), n );
                for ( j = 0; j < p->pPars->nLutSize; j++ )
                {
                    p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_TopoVar(p, i, p->pFanins[i][j]), 1 );
                    p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_MintVar(p, iMint, p->pFanins[i][j]), (k >> j) & 1 );
                }
                p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_MintVar(p, iMint, i), !n );
                if ( !bmcg_sat_solver_addclause( p->pSat, p->pLits[0], p->nLits[0] ) )
                    return 0;
            }
        }
    }
    else if ( p->pPars->nLutSize == 3 )
    {
        for ( i = p->pPars->nVars; i < p->nObjs; i++ )
        for ( s = 0;               s < i;        s++ )
        for ( t = s+1;             t < i;        t++ )
        for ( u = t+1;             u < i;        u++ )
        {
            p->pFanins[i][0] = s;
            p->pFanins[i][1] = t;
            p->pFanins[i][2] = u;
            for ( k = 0; k <= p->LutMask; k++ )
            for ( n = 0; n < 2; n++ )
            {
                p->nLits[0] = 0;
                p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_FuncVar(p, i, k), n );
                for ( j = 0; j < p->pPars->nLutSize; j++ )
                {
                    p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_TopoVar(p, i, p->pFanins[i][j]), 1 );
                    p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_MintVar(p, iMint, p->pFanins[i][j]), (k >> j) & 1 );
                }
                p->pLits[0][p->nLits[0]++] = Abc_Var2Lit( Zyx_MintVar(p, iMint, i), !n );
                if ( !bmcg_sat_solver_addclause( p->pSat, p->pLits[0], p->nLits[0] ) )
                    return 0;
            }
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Zyx_ManPrintSolutionFile( Zyx_Man_t * p, int fCompl, int fFirst )
{
    FILE * pFile;
    char FileName[1000];  int i, k;
    if ( fCompl ) Abc_TtNot( p->pTruth, Abc_TtWordNum(p->pPars->nVars) );
    Abc_TtWriteHexRev( FileName, p->pTruth, p->pPars->nVars );
    if ( fCompl ) Abc_TtNot( p->pTruth, Abc_TtWordNum(p->pPars->nVars) );
    sprintf( FileName + (1 << (p->pPars->nVars-2)), "-%d-%d.bool", p->pPars->nLutSize, p->pPars->nNodes );
    pFile = fopen( FileName, fFirst ? "wb" : "ab" );
    if ( pFile == NULL )
    {
        printf( "Cannot open input file \"%s\".\n", FileName );
        return;
    }
    for ( i = p->pPars->nVars; i < p->nObjs; i++ )
    {
        fprintf( pFile, "%c", 'A' + i );
        if ( p->pPars->fMajority )
            fprintf( pFile, "maj3" );
        else
        {
            for ( k = p->LutMask; k >= 0; k-- )
                fprintf( pFile, "%d", bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_FuncVar(p, i, k)) ^ (i == p->nObjs - 1 && fCompl) );
            for ( k = 0; k < i; k++ )
                if ( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, i, k)) )
                {
                    if ( k >= 0 && k < p->pPars->nVars )
                        fprintf( pFile, "%c", 'a' + k );
                    else
                        fprintf( pFile, "%c", 'A' + k );
                }
        }
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, "\n" );
    fclose( pFile );
    printf( "Dumped solution into file \"%s\".\n", FileName );
}
static void Zyx_ManPrintSolution( Zyx_Man_t * p, int fCompl, int fFirst )
{
    int i, k;
    printf( "Realization of given %d-input function using %d %d-input %s:\n", 
        p->pPars->nVars, p->pPars->nNodes, p->pPars->nLutSize, p->pPars->fMajority ? "MAJ-gates" : "LUTs" );
    for ( i = p->nObjs - 1; i >= p->pPars->nVars; i-- )
    {
        printf( "%02d = ", i );
        if ( p->pPars->fMajority )
            printf( "MAJ3" );
        else
        {
            printf( "%d\'b", 1 << p->pPars->nLutSize );
            for ( k = p->LutMask; k >= 0; k-- )
                printf( "%d", bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_FuncVar(p, i, k)) ^ (i == p->nObjs - 1 && fCompl) );
        }
        printf( "(" );
        for ( k = 0; k < i; k++ )
            if ( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_TopoVar(p, i, k)) )
            {
                if ( k >= 0 && k < p->pPars->nVars )
                    printf( " %c", 'a'+k );
                else
                    printf( " %02d", k );
            }
        printf( " )\n" );
    }
    if ( !p->pPars->fMajority )
        Zyx_ManPrintSolutionFile( p, fCompl, fFirst );
}
static inline int Zyx_ManEval( Zyx_Man_t * p )
{
    static int Flag = 0;
    //abctime clk = Abc_Clock();
    int i, k, j, iMint; word * pFaninsW[6], * pSpec;
    for ( i = p->pPars->nVars; i < p->nObjs; i++ )
    {
        int nFanins = Zyx_ManCollectFanins( p, i );
        assert( nFanins == p->pPars->nLutSize );
        for ( k = 0; k < p->pPars->nLutSize; k++ )
            pFaninsW[k] = Zyx_ManTruth( p, p->pFanins[i][k] );
        if ( p->pPars->fMajority )
            Abc_TtMaj( Zyx_ManTruth(p, i), pFaninsW[0], pFaninsW[1], pFaninsW[2], p->nWords );
        else
        {
            Abc_TtConst0( Zyx_ManTruth(p, i), p->nWords );
            for ( k = 1; k <= p->LutMask; k++ )
                if ( bmcg_sat_solver_read_cex_varvalue(p->pSat, Zyx_FuncVar(p, i, k)) )
                {
                    Abc_TtConst1( Zyx_ManTruth(p, p->nObjs), p->nWords );
                    for ( j = 0; j < p->pPars->nLutSize; j++ )
                        Abc_TtAndCompl( Zyx_ManTruth(p, p->nObjs), Zyx_ManTruth(p, p->nObjs), 0, pFaninsW[j], !((k >> j) & 1), p->nWords );
                    Abc_TtOr( Zyx_ManTruth(p, i), Zyx_ManTruth(p, i), Zyx_ManTruth(p, p->nObjs), p->nWords );
                }
        }
    }
    pSpec = p->pPars->fMajority ? Zyx_ManTruth(p, p->nObjs) : p->pTruth;
    if ( p->pPars->fMajority )
    {
        Vec_IntForEachEntry( p->vMidMints, iMint, i )
            if ( Abc_TtGetBit(pSpec, iMint) != Abc_TtGetBit(Zyx_ManTruth(p, p->nObjs-1), iMint) )
                return iMint;
        return -1;
    }
    else
    {
        if ( Flag && p->pPars->nVars >= 6 )
            iMint = Abc_TtFindLastDiffBit( Zyx_ManTruth(p, p->nObjs-1), pSpec, p->pPars->nVars );
        else
            iMint = Abc_TtFindFirstDiffBit( Zyx_ManTruth(p, p->nObjs-1), pSpec, p->pPars->nVars );
    }
    //Flag ^= 1;
    assert( iMint < (1 << p->pPars->nVars) );
    //p->clkEval += Abc_Clock() - clk;
    return iMint;
}
static inline void Zyx_ManEvalStats( Zyx_Man_t * p )
{
    int i;
    for ( i = 0; i < (1 << p->pPars->nVars); i++ )
        printf( "%d=%d ", i, p->Counts[i] );
    printf( "\n" );
}
static inline void Zyx_ManPrint( Zyx_Man_t * p, int Iter, int iMint, int nLazyAll, abctime clk )
{
    printf( "Iter %6d : ", Iter );
    Extra_PrintBinary( stdout, (unsigned *)&iMint, p->pPars->nVars );
    printf( "  " );
    printf( "Cla =%9d  ", bmcg_sat_solver_clausenum(p->pSat) );
    printf( "Lazy =%6d  ", nLazyAll );
    printf( "Conf =%9d  ", bmcg_sat_solver_conflictnum(p->pSat) );
    Abc_PrintTime( 1, "Time", clk );
    //Zyx_ManEvalStats( p );
    //Abc_PrintTime( 1, "Eval", p->clkEval );
}
void Zyx_ManExactSynthesis( Bmc_EsPar_t * pPars )
{
    int status, Iter, iMint = 0, fCompl = 0, nLazyAll = 0, nSols = 0;
    abctime clkTotal = Abc_Clock(), clk = Abc_Clock();  Zyx_Man_t * p; 
    word pTruth[16]; 
    if ( !pPars->fMajority )
    {
        Abc_TtReadHex( pTruth, pPars->pTtStr );
        if ( pTruth[0] & 1 ) { fCompl = 1; Abc_TtNot( pTruth, Abc_TtWordNum(pPars->nVars) ); }
    }
    assert( pPars->nVars <= 10 );
    assert( pPars->nLutSize <= 6 );
    p = Zyx_ManAlloc( pPars, pPars->fMajority ? NULL : pTruth );
    printf( "Running exact synthesis for %d-input function with %d %d-input %s...\n", 
        p->pPars->nVars, p->pPars->nNodes, p->pPars->nLutSize, p->pPars->fMajority ? "MAJ-gates" : "LUTs" );
    for ( Iter = 0 ; ; Iter++ )
    {
        while ( (status = bmcg_sat_solver_solve(p->pSat, NULL, 0)) == GLUCOSE_SAT )
        {
            int nLazy = Zyx_ManAddCnfLazyTopo( p );
            if ( nLazy == -1 )
            {
                printf( "Became UNSAT after adding lazy constraints.\n" );
                status = GLUCOSE_UNSAT;
                break;
            }
            //printf( "Added %d lazy constraints.\n\n", nLazy );
            if ( nLazy == 0 )
                break;
            nLazyAll += nLazy;
        }
        if ( status == GLUCOSE_UNSAT )
            break;
        // find mismatch
        iMint = Zyx_ManEval( p );
        if ( iMint == -1 )
        {
            if ( pPars->fEnumSols )
            {
                nSols++;
                if ( pPars->fVerbose )
                {
                    Zyx_ManPrint( p, Iter, iMint, nLazyAll, Abc_Clock() - clkTotal );
                    clk = Abc_Clock();
                }
                Zyx_ManPrintSolution( p, fCompl, nSols==1 );
                if ( !Zyx_ManAddCnfBlockSolution( p ) )
                {
                    status = GLUCOSE_UNSAT;
                    break;
                }
                continue;
            }
            else
                break;
        }
        if ( pPars->fUseIncr ? !Zyx_ManAddCnfLazyFunc2(p, iMint) : !Zyx_ManAddCnfLazyFunc(p, iMint) )
        {
            printf( "Became UNSAT after adding constraints for minterm %d\n", iMint );
            status = GLUCOSE_UNSAT;
            break;
        }
        status = bmcg_sat_solver_solve( p->pSat, NULL, 0 );
        if ( pPars->fVerbose && (!pPars->fUseIncr || Iter % 100 == 0) )
        {
            Zyx_ManPrint( p, Iter, iMint, nLazyAll, Abc_Clock() - clk );
            clk = Abc_Clock();
        }
        if ( status == GLUCOSE_UNSAT )
            break;
    }
    if ( pPars->fVerbose )
        Zyx_ManPrint( p, Iter, iMint, nLazyAll, Abc_Clock() - clkTotal );
    if ( pPars->fEnumSols )
        printf( "Finished enumerating %d solutions.\n", nSols );
    else if ( iMint == -1 )
        Zyx_ManPrintSolution( p, fCompl, 1 );
    else
        printf( "The problem has no solution.\n" );
    //Zyx_ManEvalStats( p );
    printf( "Added = %d.  Tried = %d.  ", p->nUsed[1], p->nUsed[0] );
    Abc_PrintTime( 1, "Total runtime", Abc_Clock() - clkTotal );
    Zyx_ManFree( p );
}



/**Function*************************************************************

  Synopsis    [Tests solution to the exact synthesis problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Zyx_TestGetTruthTablePars( char * pFileName, word * pTruth, int * nVars, int * nLutSize, int * nNodes )
{
    char Symb, * pCur, * pBuffer = Abc_UtilStrsav( pFileName );
    int nLength;
    for ( pCur = pBuffer; *pCur; pCur++ )
        if ( !Abc_TtIsHexDigit(*pCur) )
            break;
    Symb = *pCur; *pCur = 0;
    nLength = (int)strlen(pBuffer);
    if ( nLength == 1 )
        *nVars = 2;
    else if ( nLength == 2 )
        *nVars = 3;
    else if ( nLength == 4 )
        *nVars = 4;
    else if ( nLength == 8 )
        *nVars = 5;
    else if ( nLength == 16 )
        *nVars = 6;
    else if ( nLength == 32 )
        *nVars = 7;
    else if ( nLength == 64 )
        *nVars = 8;
    else
    {
        ABC_FREE( pBuffer );
        printf( "Invalid truth table size.\n" );
        return 0;
    }
    Abc_TtReadHex( pTruth, pBuffer );
    *pCur = Symb;
    // read LUT size
    while ( *pCur && *pCur++ != '-' );
    if ( *pCur == 0 )
    {
        ABC_FREE( pBuffer );
        printf( "Expecting \'-\' after truth table before LUT size.\n" );
        return 0;
    }
    // read node count
    *nLutSize = atoi(pCur);
    while ( *pCur && *pCur++ != '-' );
    if ( *pCur == 0 )
    {
        ABC_FREE( pBuffer );
        printf( "Expecting \'-\' after LUT size before node count.\n" );
        return 0;
    }
    *nNodes = atoi(pCur);
    ABC_FREE( pBuffer );
    return 1;
}

static inline word *  Zyx_TestTruth( Vec_Wrd_t * vInfo, int i, int nWords ) { return Vec_WrdEntryP(vInfo, nWords * i); }

Vec_Wrd_t * Zyx_TestCreateTruthTables( int nVars, int nNodes )
{
    int i, nWords = Abc_TtWordNum(nVars);
    Vec_Wrd_t * vInfo = Vec_WrdStart( nWords * (nVars + nNodes + 1) ); 
    for ( i = 0; i < nVars; i++ )
        Abc_TtIthVar( Zyx_TestTruth(vInfo, i, nWords), i, nVars );
    //Dau_DsdPrintFromTruth( Maj3_ManTruth(p, p->nObjs), p->nVars );
    return vInfo;
}
int Zyx_TestReadNode( char * pLine, Vec_Wrd_t * vTruths, int nVars, int nLutSize, int iObj )
{
    int k, j, nWords = Abc_TtWordNum(nVars);
    word * pFaninsW[6]; char * pTruth, * pFanins;
    word * pThis, * pLast = Zyx_TestTruth( vTruths, Vec_WrdSize(vTruths)/nWords - 1, nWords );
    if ( pLine[strlen(pLine)-1] == '\n' ) pLine[strlen(pLine)-1] = 0;
    if ( pLine[strlen(pLine)-1] == '\r' ) pLine[strlen(pLine)-1] = 0;
    if ( pLine[0] == 0 )
        return 0;
    if ( (int)strlen(pLine) != 1 + nLutSize + (1 << nLutSize) )
    {
        printf( "Node representation has %d chars (expecting %d chars).\n", (int)strlen(pLine), 1 + nLutSize + (1 << nLutSize) );
        return 0;
    }
    if ( pLine[0] != 'A' + iObj )
    {
        printf( "The output node in line %s is not correct.\n", pLine );
        return 0;
    }
    pTruth  = pLine + 1;
    pFanins = pTruth + (1 << nLutSize);
    for ( k = nLutSize - 1; k >= 0; k-- )
        pFaninsW[k] = Zyx_TestTruth( vTruths, pFanins[k] >= 'a' ? pFanins[k] - 'a' : pFanins[k] - 'A', nWords );
    pThis = Zyx_TestTruth(vTruths, iObj, nWords);
    Abc_TtConst0( pThis, nWords );
    for ( k = 0; k < (1 << nLutSize); k++ )
    {
        if ( pTruth[(1 << nLutSize) - 1 - k] == '0' )
            continue;
        Abc_TtConst1( pLast, nWords );
        for ( j = 0; j < nLutSize; j++ )
            Abc_TtAndCompl( pLast, pLast, 0, pFaninsW[j], !((k >> j) & 1), nWords );
        Abc_TtOr( pThis, pThis, pLast, nWords );
    }
    //Dau_DsdPrintFromTruth( pThis, nVars );
    return 1;
}
void Zyx_TestExact( char * pFileName )
{
    int iObj, nStrs = 0, nVars = -1, nLutSize = -1, nNodes = -1;
    word * pImpl, pSpec[4]; Vec_Wrd_t * vInfo; char Line[1000];
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open input file \"%s\".\n", pFileName );
        return;
    }
    if ( !Zyx_TestGetTruthTablePars( pFileName, pSpec, &nVars, &nLutSize, &nNodes ) )
        return;
    if ( nVars > 8 )
    {
        printf( "This tester does not support functions with more than 8 inputs.\n" );
        return;
    }
    if ( nLutSize > 6 )
    {
        printf( "This tester does not support nodes with more than 6 inputs.\n" );
        return;
    }
    if ( nNodes > 16 )
    {
        printf( "This tester does not support structures with more than 16 inputs.\n" );
        return;
    }
    vInfo = Zyx_TestCreateTruthTables( nVars, nNodes );
    for ( iObj = nVars; fgets(Line, 1000, pFile) != NULL; iObj++ )
    {
        if ( Zyx_TestReadNode( Line, vInfo, nVars, nLutSize, iObj ) )
            continue;
        if ( iObj != nVars + nNodes )
        {
            printf( "The number of nodes in the structure is not correct.\n" );
            break;
        }
        nStrs++;
        pImpl = Zyx_TestTruth( vInfo, iObj-1, Abc_TtWordNum(nVars) );
        if ( Abc_TtEqual( pImpl, pSpec, Abc_TtWordNum(nVars) ) )
            printf( "Structure %3d : Verification successful.\n", nStrs );
        else
        {
            printf( "Structure %3d : Verification FAILED.\n", nStrs );
            printf( "Implementation: " ); Dau_DsdPrintFromTruth( pImpl, nVars );
            printf( "Specification:  " ); Dau_DsdPrintFromTruth( pSpec, nVars );
        }
        iObj = nVars - 1;
    }
    Vec_WrdFree( vInfo );
    fclose( pFile );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

