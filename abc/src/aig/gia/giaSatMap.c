/**CFile****************************************************************

  FileName    [giaSatMap.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaSatMap.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "sat/bsat/satStore.h"
#include "misc/vec/vecWec.h"
#include "misc/util/utilNam.h"
#include "map/scl/sclCon.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


// operation manager
typedef struct Sbm_Man_t_ Sbm_Man_t;
struct Sbm_Man_t_
{
    sat_solver *     pSat;      // SAT solver
    Vec_Int_t *      vCardVars; // candinality variables
    int              LogN;      // max vars
    int              FirstVar;  // first variable to be used
    int              LitShift;  // shift in terms of literals (2*Gia_ManCiNum(pGia)+2)
    int              nInputs;   // the number of inputs
    // window
    Vec_Int_t *      vRoots;    // output drivers to be mapped (root -> lit)
    Vec_Wec_t *      vCuts;     // cuts (cut -> node lit + fanin lits)
    Vec_Wec_t *      vObjCuts;  // cuts (obj -> node lit + cut lits)
    Vec_Int_t *      vSolCuts;  // current solution (index -> cut)
    Vec_Int_t *      vCutGates; // gates (cut -> gate)
    Vec_Wrd_t *      vCutAreas; // area of each cut
    // solver
    Vec_Int_t *      vAssump;   // assumptions (root nodes)
    Vec_Int_t *      vPolar;    // polarity of nodes and cuts
    // timing
    Vec_Int_t *      vArrs;     // arrivals for the inputs (input -> time)
    Vec_Int_t *      vReqs;     // required for the outputs (root -> time)
    // internal
    Vec_Int_t *      vLit2Used; // current solution (lit -> used)
    Vec_Int_t *      vDelays;   // node arrivals (lit -> time)
    Vec_Int_t *      vReason;   // timing reasons (lit -> cut)
};

/*
    Cuts in p->vCuts have 0-based numbers and are expressed in terms of object literals.
    Cuts in p->vObjCuts are expressed in terms of the obj-lit + cut-lits (i + p->FirstVar)
    Unit cuts for each polarity are ordered in the end.
*/

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Verify solution. Create polarity and assumptions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sbm_ManCheckSol( Sbm_Man_t * p, Vec_Int_t * vSol )
{
    //int K = Vec_IntSize(vSol) - 1;
    int i, j, Lit, Cut;
    int RetValue = 1;
    Vec_Int_t * vCut;
    // clear polarity and assumptions
    Vec_IntClear( p->vPolar );
    // mark used literals
    Vec_IntFill( p->vLit2Used, Vec_WecSize(p->vObjCuts) + p->nInputs, 0 );
    Vec_IntForEachEntry( p->vSolCuts, Cut, i )
    {
        if ( Cut < 0 ) // input inverter variable
        {
            Vec_IntWriteEntry( p->vLit2Used, -Cut, 1 );
            Vec_IntPush( p->vPolar, -Cut ); 
            continue;
        }
        Vec_IntPush( p->vPolar, p->FirstVar + Cut ); 
        vCut = Vec_WecEntry( p->vCuts, Cut );
        Lit = Vec_IntEntry( vCut, 0 ) - p->LitShift; // obj literal
        if ( Vec_IntEntry(p->vLit2Used, Lit) )
            continue;
        Vec_IntWriteEntry( p->vLit2Used, Lit, 1 );
        Vec_IntPush( p->vPolar, Lit ); // literal variable
    }
    // check that the output literals are used
    Vec_IntForEachEntry( p->vRoots, Lit, i )
    {
        if ( Vec_IntEntry(p->vLit2Used, Lit) == 0 )
            printf( "Output literal %d has no cut.\n", Lit ), RetValue = 0;
    }
    // check internal nodes
    Vec_IntForEachEntry( p->vSolCuts, Cut, i )
    {
        if ( Cut < 0 )
            continue;
        vCut = Vec_WecEntry( p->vCuts, Cut );
        Vec_IntForEachEntryStart( vCut, Lit, j, 1 )
            if ( Lit - p->LitShift < 0 )
            {
                assert( Abc_LitIsCompl(Lit) );
                if ( Vec_IntEntry(p->vLit2Used, Vec_WecSize(p->vObjCuts) + Abc_Lit2Var(Lit)-1) == 0 )
                    printf( "Inverter of input %d of cut %d is not mapped.\n", Abc_Lit2Var(Lit)-1, Cut ), RetValue = 0;
            }
            else if ( Vec_IntEntry(p->vLit2Used, Lit - p->LitShift) == 0 )
                printf( "Internal literal %d of cut %d is not mapped.\n", Lit - p->LitShift, Cut ), RetValue = 0;
        // create polarity
        Vec_IntPush( p->vPolar, p->FirstVar + Cut ); // cut variable
    }
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sbm_ManCreateCnf( Sbm_Man_t * p )
{
    int i, k, Lit, Lits[2], value;
    Vec_Int_t * vLits, * vCutOne, * vLitsPrev;
    // sat_solver_rollback( p->Sat );
    if ( !Sbm_ManCheckSol(p, p->vSolCuts) )
        return 0;
    // increase var count
    assert( p->FirstVar == sat_solver_nvars(p->pSat) );
    sat_solver_setnvars( p->pSat, sat_solver_nvars(p->pSat) + Vec_WecSize(p->vCuts) );
    // iterate over arrays containing obj-lit cuts (neg-obj-lit cut-lits followed by pos-obj-lit cut-lits)
    vLitsPrev = NULL;
    Vec_WecForEachLevel( p->vObjCuts, vLits, i )
    {
        assert( Vec_IntSize(vLits) >= 2 );
        value = sat_solver_addclause( p->pSat, Vec_IntArray(vLits), Vec_IntLimit(vLits)  );
        assert( value );
/*
        // for each cut, add implied nodes
        Lits[0] = Abc_LitNot( Vec_IntEntry(vLits, 0) );
        assert( Lits[0] < 2*p->FirstVar );
        Vec_IntForEachEntryStart( vLits, Lit, k, 1 )
        {
            assert( Lit >= 2*p->FirstVar );
            Lits[1] = Abc_LitNot( Lit );
            value = sat_solver_addclause( p->pSat, Lits, Lits + 2 );
            assert( value );
            //printf( "Adding clause %d + %d.\n", Lits[0], Lits[1]-2*p->FirstVar );
        }
*/
        //printf( "\n" );
        // create invertor exclusivity clause
        if ( i & 1 )
        {
            Lits[0] = Abc_LitNot( Vec_IntEntryLast(vLits) );
            Lits[1] = Abc_LitNot( Vec_IntEntryLast(vLitsPrev) );
            value = sat_solver_addclause( p->pSat, Lits, Lits + 2 );
            assert( value );
            //printf( "Adding exclusivity clause %d + %d.\n", Lits[0]-2*p->FirstVar, Lits[1]-2*p->FirstVar );
        }
        vLitsPrev = vLits;
    }
    // add inverters
    Vec_WecForEachLevel( p->vCuts, vCutOne, i )
        Vec_IntForEachEntry( vCutOne, Lit, k )
            if ( Abc_Lit2Var(Lit)-1 < p->nInputs ) // input
            {
                assert( k > 0 );
                Lits[0] = Abc_Var2Lit( Vec_WecSize(p->vObjCuts) + Abc_Lit2Var(Lit)-1, 0 );
                Lits[1] = Abc_Var2Lit( p->FirstVar + i, 1 );
                value = sat_solver_addclause( p->pSat, Lits, Lits + 2 );
                assert( value );
            }
            else // internal node
            {
                Lits[0] = Abc_Var2Lit( Lit-p->LitShift, 0 );
                Lits[1] = Abc_Var2Lit( p->FirstVar + i, 1 );
                value = sat_solver_addclause( p->pSat, Lits, Lits + 2 );
                assert( value );
            }

    sat_solver_set_polarity( p->pSat, Vec_IntArray(p->vPolar), Vec_IntSize(p->vPolar) );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int sat_solver_add_and1( sat_solver * pSat, int iVar, int iVar0, int iVar1, int fCompl0, int fCompl1, int fCompl )
{
    lit Lits[3];
    int Cid;

    Lits[0] = toLitCond( iVar, !fCompl );
    Lits[1] = toLitCond( iVar0, fCompl0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    assert( Cid );

    Lits[0] = toLitCond( iVar, !fCompl );
    Lits[1] = toLitCond( iVar1, fCompl1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    assert( Cid );
/*
    Lits[0] = toLitCond( iVar, fCompl );
    Lits[1] = toLitCond( iVar0, !fCompl0 );
    Lits[2] = toLitCond( iVar1, !fCompl1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );
*/
    return 3;
}

static inline int sat_solver_add_and2( sat_solver * pSat, int iVar, int iVar0, int iVar1, int fCompl0, int fCompl1, int fCompl )
{
    lit Lits[3];
    int Cid;
/*
    Lits[0] = toLitCond( iVar, !fCompl );
    Lits[1] = toLitCond( iVar0, fCompl0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    assert( Cid );

    Lits[0] = toLitCond( iVar, !fCompl );
    Lits[1] = toLitCond( iVar1, fCompl1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 2 );
    assert( Cid );
*/
    Lits[0] = toLitCond( iVar, fCompl );
    Lits[1] = toLitCond( iVar0, !fCompl0 );
    Lits[2] = toLitCond( iVar1, !fCompl1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );
    return 3;
}


/**Function*************************************************************

  Synopsis    [Adds a general cardinality constraint in terms of vVars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Card_AddClause( Vec_Int_t * p, int* begin, int* end )
{
    Vec_IntPush( p, (int)(end-begin) );
    while ( begin < end )
        Vec_IntPush( p, (int)*begin++ );
    return 1;
}
static inline int Card_AddHalfSorter( Vec_Int_t * p, int iVarA, int iVarB, int iVar0, int iVar1 )
{
    lit Lits[3];
    int Cid;

    Lits[0] = toLitCond( iVarA, 0 );
    Lits[1] = toLitCond( iVar0, 1 );
    Cid = Card_AddClause( p, Lits, Lits + 2 );
    assert( Cid );

    Lits[0] = toLitCond( iVarA, 0 );
    Lits[1] = toLitCond( iVar1, 1 );
    Cid = Card_AddClause( p, Lits, Lits + 2 );
    assert( Cid );

    Lits[0] = toLitCond( iVarB, 0 );
    Lits[1] = toLitCond( iVar0, 1 );
    Lits[2] = toLitCond( iVar1, 1 );
    Cid = Card_AddClause( p, Lits, Lits + 3 );
    assert( Cid );
    return 3;
}
static inline void Card_AddSorter( Vec_Int_t * p, int * pVars, int i, int k, int * pnVars )
{
    int iVar1 = (*pnVars)++;
    int iVar2 = (*pnVars)++;
    Card_AddHalfSorter( p, iVar1, iVar2, pVars[i], pVars[k] );
    pVars[i] = iVar1;
    pVars[k] = iVar2;
}
static inline void Card_AddCardinConstrMerge( Vec_Int_t * p, int * pVars, int lo, int hi, int r, int * pnVars )
{
    int i, step = r * 2;
    if ( step < hi - lo )
    {
        Card_AddCardinConstrMerge( p, pVars, lo, hi-r, step, pnVars );
        Card_AddCardinConstrMerge( p, pVars, lo+r, hi, step, pnVars );
        for ( i = lo+r; i < hi-r; i += step )
            Card_AddSorter( p, pVars, i, i+r, pnVars );
        for ( i = lo+r; i < hi-r-1; i += r )
        {
            lit Lits[2] = { Abc_Var2Lit(pVars[i], 0), Abc_Var2Lit(pVars[i+r], 1) };
            int Cid = Card_AddClause( p, Lits, Lits + 2 );
            assert( Cid );
        }
    }
}
static inline void Card_AddCardinConstrRange( Vec_Int_t * p, int * pVars, int lo, int hi, int * pnVars )
{
    if ( hi - lo >= 1 )
    {
        int i, mid = lo + (hi - lo) / 2;
        for ( i = lo; i <= mid; i++ )
            Card_AddSorter( p, pVars, i, i + (hi - lo + 1) / 2, pnVars );
        Card_AddCardinConstrRange( p, pVars, lo, mid, pnVars );
        Card_AddCardinConstrRange( p, pVars, mid+1, hi, pnVars );
        Card_AddCardinConstrMerge( p, pVars, lo, hi, 1, pnVars );
    }
}
int Card_AddCardinConstrPairWise( Vec_Int_t * p, Vec_Int_t * vVars )
{
    int nVars = Vec_IntSize(vVars);
    Card_AddCardinConstrRange( p, Vec_IntArray(vVars), 0, nVars - 1, &nVars );
    return nVars;
}
int Card_AddCardinSolver( int LogN, Vec_Int_t ** pvVars, Vec_Int_t ** pvRes )
{
    int nVars = 1 << LogN;
    int nVarsAlloc = nVars + 2 * (nVars * LogN * (LogN-1) / 4 + nVars - 1);
    Vec_Int_t * vRes = Vec_IntAlloc( 1000 );
    Vec_Int_t * vVars = Vec_IntStartNatural( nVars );
    int nVarsReal = Card_AddCardinConstrPairWise( vRes, vVars );
    assert( nVarsReal == nVarsAlloc );
    Vec_IntPush( vRes, -1 );  
    *pvVars = vVars;
    *pvRes  = vRes;  
    return nVarsReal;
}
sat_solver * Sbm_AddCardinSolver2( int LogN, Vec_Int_t ** pvVars, Vec_Int_t ** pvRes )
{
    Vec_Int_t * vVars = NULL;
    Vec_Int_t * vRes  = NULL;    
    int nVarsReal = Card_AddCardinSolver( LogN, &vVars, &vRes ), i, size;
    sat_solver * pSat = sat_solver_new();
    sat_solver_setnvars( pSat, nVarsReal );
    for ( i = 0, size = Vec_IntEntry(vRes, i++); i < Vec_IntSize(vRes); i += size, size = Vec_IntEntry(vRes, i++) )
        sat_solver_addclause( pSat, Vec_IntEntryP(vRes, i), Vec_IntEntryP(vRes, i+size) );
    if ( pvVars ) *pvVars = vVars;
    if ( pvRes )  *pvRes  = vRes;
    return pSat;
}


/**Function*************************************************************

  Synopsis    [Adds a general cardinality constraint in terms of vVars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Sbm_AddSorter( sat_solver * p, int * pVars, int i, int k, int * pnVars )
{
    int iVar1 = (*pnVars)++;
    int iVar2 = (*pnVars)++;
    int fVerbose = 0;
    if ( fVerbose )
    {
        int v;
        for ( v = 0; v < i; v++ )
            printf( " " );
        printf( "<" );
        for ( v = i+1; v < k; v++ )
            printf( "-" );
        printf( ">" );
        for ( v = k+1; v < 8; v++ )
            printf( " " );
        printf( "    " );
        printf( "[%3d :%3d ] -> [%3d :%3d ]\n", pVars[i], pVars[k], iVar1, iVar2 );
    }
//    sat_solver_add_and1( p, iVar1, pVars[i], pVars[k], 1, 1, 1 );
//    sat_solver_add_and2( p, iVar2, pVars[i], pVars[k], 0, 0, 0 );
    sat_solver_add_half_sorter( p, iVar1, iVar2, pVars[i], pVars[k] );
    pVars[i] = iVar1;
    pVars[k] = iVar2;
}
static inline void Sbm_AddCardinConstrMerge( sat_solver * p, int * pVars, int lo, int hi, int r, int * pnVars )
{
    int i, step = r * 2;
    if ( step < hi - lo )
    {
        Sbm_AddCardinConstrMerge( p, pVars, lo, hi-r, step, pnVars );
        Sbm_AddCardinConstrMerge( p, pVars, lo+r, hi, step, pnVars );
        for ( i = lo+r; i < hi-r; i += step )
            Sbm_AddSorter( p, pVars, i, i+r, pnVars );
        for ( i = lo+r; i < hi-r-1; i += r )
        {
            lit Lits[2] = { Abc_Var2Lit(pVars[i], 0), Abc_Var2Lit(pVars[i+r], 1) };
            int Cid = sat_solver_addclause( p, Lits, Lits + 2 );
            assert( Cid );
        }
    }
}
static inline void Sbm_AddCardinConstrRange( sat_solver * p, int * pVars, int lo, int hi, int * pnVars )
{
    if ( hi - lo >= 1 )
    {
        int i, mid = lo + (hi - lo) / 2;
        for ( i = lo; i <= mid; i++ )
            Sbm_AddSorter( p, pVars, i, i + (hi - lo + 1) / 2, pnVars );
        Sbm_AddCardinConstrRange( p, pVars, lo, mid, pnVars );
        Sbm_AddCardinConstrRange( p, pVars, mid+1, hi, pnVars );
        Sbm_AddCardinConstrMerge( p, pVars, lo, hi, 1, pnVars );
    }
}
int Sbm_AddCardinConstrPairWise( sat_solver * p, Vec_Int_t * vVars, int K )
{
    int nVars = Vec_IntSize(vVars);
    Sbm_AddCardinConstrRange( p, Vec_IntArray(vVars), 0, nVars - 1, &nVars );
    sat_solver_bookmark( p );
    return nVars;
}
sat_solver * Sbm_AddCardinSolver( int LogN, Vec_Int_t ** pvVars )
{
    int nVars = 1 << LogN;
    int nVarsAlloc = nVars + 2 * (nVars * LogN * (LogN-1) / 4 + nVars - 1), nVarsReal;
    Vec_Int_t * vVars = Vec_IntStartNatural( nVars );
    sat_solver * pSat = sat_solver_new();
    sat_solver_setnvars( pSat, nVarsAlloc );
    nVarsReal = Sbm_AddCardinConstrPairWise( pSat, vVars, 2 );
    assert( nVarsReal == nVarsAlloc );
    *pvVars = vVars;
    return pSat;
}

void Sbm_AddCardinConstrTest()
{
    int LogN = 3, nVars = 1 << LogN, K = 2, Count = 1;
    Vec_Int_t * vVars, * vLits = Vec_IntAlloc( nVars );
    sat_solver * pSat = Sbm_AddCardinSolver( LogN, &vVars );
    int nVarsReal = sat_solver_nvars( pSat );

    int Lit = Abc_Var2Lit( Vec_IntEntry(vVars, K), 1 );
    printf( "LogN = %d. N = %3d.   Vars = %5d. Clauses = %6d.  Comb = %d.\n", LogN, nVars, nVarsReal, sat_solver_nclauses(pSat), nVars * (nVars-1)/2 + nVars + 1 );
    while ( 1 )
    {
        int i, status = sat_solver_solve( pSat, &Lit, &Lit+1, 0, 0, 0, 0 );
        if ( status != l_True )
            break;
        Vec_IntClear( vLits );
        printf( "%3d : ", Count++ );
        for ( i = 0; i < nVars; i++ )
        {
            Vec_IntPush( vLits, Abc_Var2Lit(i, sat_solver_var_value(pSat, i)) );
            printf( "%d", sat_solver_var_value(pSat, i) );
        }
        printf( "\n" );
        status = sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + nVars );
        if ( status == 0 )
            break;
    }

    sat_solver_delete( pSat );
    Vec_IntFree( vVars );
    Vec_IntFree( vLits );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sbm_Man_t * Sbm_ManAlloc( int LogN )
{
    Sbm_Man_t * p = ABC_CALLOC( Sbm_Man_t, 1 );
    p->pSat = Sbm_AddCardinSolver( LogN, &p->vCardVars );
    p->LogN = LogN;
    p->FirstVar  = sat_solver_nvars( p->pSat );
    // window
    p->vRoots    = Vec_IntAlloc( 100 );
    p->vCuts     = Vec_WecAlloc( 1000 );
    p->vObjCuts  = Vec_WecAlloc( 1000 );
    p->vSolCuts  = Vec_IntAlloc( 100 );
    p->vCutGates = Vec_IntAlloc( 100 );
    p->vCutAreas = Vec_WrdAlloc( 100 );
    // solver
    p->vAssump   = Vec_IntAlloc( 100 );
    p->vPolar    = Vec_IntAlloc( 100 );
    // timing
    p->vArrs     = Vec_IntAlloc( 100 );
    p->vReqs     = Vec_IntAlloc( 100 );
    // internal
    p->vLit2Used = Vec_IntAlloc( 100 );
    p->vDelays   = Vec_IntAlloc( 100 );
    p->vReason   = Vec_IntAlloc( 100 );
    return p;
}

void Sbm_ManStop( Sbm_Man_t * p )
{
    sat_solver_delete( p->pSat );
    Vec_IntFree( p->vCardVars );
    // internal
    Vec_IntFree( p->vRoots );
    Vec_WecFree( p->vCuts );
    Vec_WecFree( p->vObjCuts );
    Vec_IntFree( p->vSolCuts );
    Vec_IntFree( p->vCutGates );
    Vec_WrdFree( p->vCutAreas );
    // internal
    Vec_IntFree( p->vAssump );
    Vec_IntFree( p->vPolar );
    // internal
    Vec_IntFree( p->vArrs );
    Vec_IntFree( p->vReqs );
    // internal
    Vec_IntFree( p->vLit2Used );
    Vec_IntFree( p->vDelays );
    Vec_IntFree( p->vReason );
    ABC_FREE( p );
}

int Sbm_ManTestSat( void * pMan )
{
    abctime clk = Abc_Clock(), clk2;
    int i, k, Lit, LogN = 7, nVars = 1 << LogN, status, Root;
    Sbm_Man_t * p = Sbm_ManAlloc( LogN );
    word InvArea = 0;
    int fKeepTrying = 1;
    int StartSol;
    // derive window
    extern int Nf_ManExtractWindow( void * pMan, Vec_Int_t * vRoots, Vec_Wec_t * vCuts, Vec_Wec_t * vObjCuts, Vec_Int_t * vSolCuts, Vec_Int_t * vCutGates, Vec_Wrd_t * vCutAreas, word * pInvArea, int StartVar, int nVars );
    p->nInputs = Nf_ManExtractWindow( pMan, p->vRoots, p->vCuts, p->vObjCuts, p->vSolCuts, p->vCutGates, p->vCutAreas, &InvArea, p->FirstVar, nVars );
    p->LitShift = 2*p->nInputs+2;
    assert( Vec_WecSize(p->vObjCuts) + p->nInputs <= nVars );

    // print-out
//    Vec_WecPrint( p->vCuts, 0 );
//    printf( "\n" );
//    Vec_WecPrint( p->vObjCuts, 0 );
//    printf( "\n" );
    Vec_IntPrint( p->vSolCuts );
    printf( "All clauses = %d.  Multi clauses = %d.  Binary clauses = %d.  Other clauses = %d.\n", 
        sat_solver_nclauses(p->pSat), Vec_WecSize(p->vObjCuts), Vec_WecSizeSize(p->vCuts), 
        sat_solver_nclauses(p->pSat) - Vec_WecSize(p->vObjCuts) - Vec_WecSizeSize(p->vCuts) );

    // creating CNF
    if ( !Sbm_ManCreateCnf(p) )
        return 0;

    // create assumptions
    // cardinality
    Vec_IntClear( p->vAssump );
    Vec_IntPush( p->vAssump, -1 );
    // unused variables
    for ( i = Vec_WecSize(p->vObjCuts) + p->nInputs; i < nVars; i++ )
        Vec_IntPush( p->vAssump, Abc_Var2Lit(i, 1) );
    // root variables
    Vec_IntForEachEntry( p->vRoots, Root, i )
        Vec_IntPush( p->vAssump, Abc_Var2Lit(Root, 0) );
//    Vec_IntPrint( p->vAssump );

    StartSol = Vec_IntSize(p->vSolCuts);
//    StartSol = 30;
    while ( fKeepTrying && StartSol-fKeepTrying > 0 )
    {
        printf( "Trying to find mapping with %d gates.\n", StartSol-fKeepTrying );
    //    for ( i = Vec_IntSize(p->vSolCuts)-5; i < nVars; i++ )
    //        Vec_IntPush( p->vAssump, Abc_Var2Lit(Vec_IntEntry(p->vCardVars, i), 1) );
        Vec_IntWriteEntry( p->vAssump, 0, Abc_Var2Lit(Vec_IntEntry(p->vCardVars, StartSol-fKeepTrying), 1) );
        // solve the problem
        clk2 = Abc_Clock();
        status = sat_solver_solve( p->pSat, Vec_IntArray(p->vAssump), Vec_IntLimit(p->vAssump), 0, 0, 0, 0 );
        if ( status == l_True )
        {
            word AreaNew = 0;
            int Count = 0;
            printf( "AND Lits = %d.  Inputs = %d.  Vars = %d.  All vars = %d.\n", Vec_WecSize(p->vObjCuts), p->nInputs, Vec_WecSize(p->vObjCuts) + p->nInputs, nVars );
//            for ( i = 0; i < nVars; i++ )
//                printf( "%d", sat_solver_var_value(p->pSat, i) );
//            printf( "\n" );
            for ( i = 0; i < nVars; i++ )
                if ( sat_solver_var_value(p->pSat, i) )
                {
                    printf( "%d=%d ", i, sat_solver_var_value(p->pSat, i) );
                    Count++;
                    if ( i >= Vec_WecSize(p->vObjCuts) )
                        AreaNew += InvArea;
                }
            printf( "Count = %d\n", Count );
//            for ( i = p->FirstVar; i < sat_solver_nvars(p->pSat); i++ )
//                printf( "%d", sat_solver_var_value(p->pSat, i) );
//            printf( "\n" );
            Count = 1;
            for ( i = p->FirstVar; i < sat_solver_nvars(p->pSat); i++ )
                if ( sat_solver_var_value(p->pSat, i) )
                {
                    Vec_Int_t * vCutOne = Vec_WecEntry(p->vCuts, i-p->FirstVar);
                    printf( "%2d : Cut %3d  (Gate %2d)  ", Count, i-p->FirstVar, Vec_IntEntry(p->vCutGates, i-p->FirstVar) );
                    Vec_IntForEachEntry( vCutOne, Lit, k )
                        printf( "%d(%d) ", Lit - 2*(p->nInputs+1), Abc_Lit2Var(Lit) );
                    printf( "\n" );
                    Count++;
                    AreaNew += Vec_WrdEntry(p->vCutAreas, i-p->FirstVar);
                }
            printf( "Area = %7.2f\n", Scl_Int2Flt((int)AreaNew) );
        }
        if ( status == l_False )
            printf( "UNSAT " ), fKeepTrying = 0;
        else if ( status == l_True )
            printf( "SAT   " ), fKeepTrying++;
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk2 );
        Abc_PrintTime( 1, "Total time", Abc_Clock() - clk );
        printf( "\n" );
    }
    Sbm_ManStop( p );
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

