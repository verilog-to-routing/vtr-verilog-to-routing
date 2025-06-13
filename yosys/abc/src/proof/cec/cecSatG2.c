/**CFile****************************************************************

  FileName    [cecSatG2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Combinational equivalence checking.]

  Synopsis    [Detection of structural isomorphism.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: cecSatG2.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig/gia/gia.h"
#include "misc/util/utilTruth.h"
#include "cec.h"
#include "bdd/extrab/extraBdd.h"
#include "base/abc/abc.h"
#include "map/if/if.h"

#define USE_GLUCOSE2

#ifdef USE_GLUCOSE2

#include "sat/glucose2/AbcGlucose2.h"

#define sat_solver                         bmcg2_sat_solver
#define sat_solver_start                   bmcg2_sat_solver_start
#define sat_solver_stop                    bmcg2_sat_solver_stop
#define sat_solver_addclause               bmcg2_sat_solver_addclause
#define sat_solver_add_and                 bmcg2_sat_solver_add_and
#define sat_solver_add_xor                 bmcg2_sat_solver_add_xor
#define sat_solver_addvar                  bmcg2_sat_solver_addvar
#define sat_solver_read_cex_varvalue       bmcg2_sat_solver_read_cex_varvalue
#define sat_solver_reset                   bmcg2_sat_solver_reset
#define sat_solver_set_conflict_budget     bmcg2_sat_solver_set_conflict_budget
#define sat_solver_conflictnum             bmcg2_sat_solver_conflictnum
#define sat_solver_solve                   bmcg2_sat_solver_solve
#define sat_solver_read_cex_varvalue       bmcg2_sat_solver_read_cex_varvalue
#define sat_solver_read_cex                bmcg2_sat_solver_read_cex
#define sat_solver_jftr                    bmcg2_sat_solver_jftr
#define sat_solver_set_jftr                bmcg2_sat_solver_set_jftr
#define sat_solver_set_var_fanin_lit       bmcg2_sat_solver_set_var_fanin_lit
#define sat_solver_start_new_round         bmcg2_sat_solver_start_new_round
#define sat_solver_mark_cone               bmcg2_sat_solver_mark_cone

#else

#include "sat/glucose/AbcGlucose.h"

#define sat_solver                         bmcg_sat_solver
#define sat_solver_start                   bmcg_sat_solver_start
#define sat_solver_stop                    bmcg_sat_solver_stop
#define sat_solver_addclause               bmcg_sat_solver_addclause
#define sat_solver_add_and                 bmcg_sat_solver_add_and
#define sat_solver_add_xor                 bmcg_sat_solver_add_xor
#define sat_solver_addvar                  bmcg_sat_solver_addvar
#define sat_solver_read_cex_varvalue       bmcg_sat_solver_read_cex_varvalue
#define sat_solver_reset                   bmcg_sat_solver_reset
#define sat_solver_set_conflict_budget     bmcg_sat_solver_set_conflict_budget
#define sat_solver_conflictnum             bmcg_sat_solver_conflictnum
#define sat_solver_solve                   bmcg_sat_solver_solve
#define sat_solver_read_cex_varvalue       bmcg_sat_solver_read_cex_varvalue
#define sat_solver_read_cex                bmcg_sat_solver_read_cex
#define sat_solver_jftr                    bmcg_sat_solver_jftr
#define sat_solver_set_jftr                bmcg_sat_solver_set_jftr
#define sat_solver_set_var_fanin_lit       bmcg_sat_solver_set_var_fanin_lit
#define sat_solver_start_new_round         bmcg_sat_solver_start_new_round
#define sat_solver_mark_cone               bmcg_sat_solver_mark_cone

#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// SAT solving manager
typedef struct Cec4_Man_t_ Cec4_Man_t;
struct Cec4_Man_t_
{
    Cec_ParFra_t *   pPars;          // parameters
    Gia_Man_t *      pAig;           // user's AIG
    Gia_Man_t *      pNew;           // internal AIG
    // SAT solving
    sat_solver *     pSat;           // SAT solver
    Vec_Ptr_t *      vFrontier;      // CNF construction
    Vec_Ptr_t *      vFanins;        // CNF construction
    Vec_Int_t *      vCexMin;        // minimized CEX
    Vec_Int_t *      vClassUpdates;  // updated equiv classes
    Vec_Int_t *      vCexStamps;     // time stamps
    Vec_Int_t *      vCands;
    Vec_Int_t *      vVisit;
    Vec_Int_t *      vPat;
    Vec_Int_t *      vDisprPairs;
    Vec_Bit_t *      vFails;
    Vec_Bit_t *      vCoDrivers;
    Vec_Int_t *      vPairs;   
    int              iPosRead;       // candidate reading position
    int              iPosWrite;      // candidate writing position
    int              iLastConst;     // last const node proved
    // refinement
    Vec_Int_t *      vRefClasses;
    Vec_Int_t *      vRefNodes;
    Vec_Int_t *      vRefBins;
    int *            pTable;
    int              nTableSize;
    // statistics
    int              nItersSim;
    int              nItersSat;
    int              nAndNodes;
    int              nPatterns;
    int              nSatSat;
    int              nSatUnsat;
    int              nSatUndec;
    int              nCallsSince;
    int              nSimulates;
    int              nRecycles;
    int              nConflicts[2][3];
    int              nGates[2];
    int              nFaster[2];
    abctime          timeCnf;
    abctime          timeGenPats;
    abctime          timeSatSat0;
    abctime          timeSatUnsat0;
    abctime          timeSatSat;
    abctime          timeSatUnsat;
    abctime          timeSatUndec;
    abctime          timeSim;
    abctime          timeRefine;
    abctime          timeResimGlo;
    abctime          timeResimLoc;
    abctime          timeStart;
};

static inline int    Cec4_ObjSatId( Gia_Man_t * p, Gia_Obj_t * pObj )             { return Gia_ObjCopy2Array(p, Gia_ObjId(p, pObj));                                                     }
static inline int    Cec4_ObjSetSatId( Gia_Man_t * p, Gia_Obj_t * pObj, int Num ) { assert(Cec4_ObjSatId(p, pObj) == -1); Gia_ObjSetCopy2Array(p, Gia_ObjId(p, pObj), Num); Vec_IntPush(&p->vSuppVars, Gia_ObjId(p, pObj)); if ( Gia_ObjIsCi(pObj) ) Vec_IntPushTwo(&p->vCopiesTwo, Gia_ObjId(p, pObj), Num); assert(Vec_IntSize(&p->vVarMap) == Num); Vec_IntPush(&p->vVarMap, Gia_ObjId(p, pObj)); return Num;  }
static inline void   Cec4_ObjCleanSatId( Gia_Man_t * p, Gia_Obj_t * pObj )        { assert(Cec4_ObjSatId(p, pObj) != -1); Gia_ObjSetCopy2Array(p, Gia_ObjId(p, pObj), -1);               }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wrd_t * Cec4_EvalCombine( Vec_Int_t * vPats, int nPats, int nInputs, int nWords )
{
    //Vec_Wrd_t * vSimsPi = Vec_WrdStart( nInputs * nWords );
    Vec_Wrd_t * vSimsPi = Vec_WrdStartRandom( nInputs * nWords );
    int i, k, iLit, iPat = 0; word * pSim;
    for ( i = 0; i < Vec_IntSize(vPats); i += Vec_IntEntry(vPats, i), iPat++ )
        for ( k = 1; k < Vec_IntEntry(vPats, i)-1; k++ )
            if ( (iLit = Vec_IntEntry(vPats, i+k)) )
            {
                assert( Abc_Lit2Var(iLit) > 0 && Abc_Lit2Var(iLit) <= nInputs );
                pSim = Vec_WrdEntryP( vSimsPi, (Abc_Lit2Var(iLit)-1)*nWords );
                if ( Abc_InfoHasBit( (unsigned*)pSim, iPat ) != Abc_LitIsCompl(iLit) )
                    Abc_InfoXorBit( (unsigned*)pSim, iPat );
            }
    assert( iPat == nPats );
    return vSimsPi;
}
void Cec4_EvalPatterns( Gia_Man_t * p, Vec_Int_t * vPats, int nPats )
{
    int nWords = Abc_Bit6WordNum(nPats);
    Vec_Wrd_t * vSimsPi = Cec4_EvalCombine( vPats, nPats, Gia_ManCiNum(p), nWords );
    Vec_Wrd_t * vSimsPo = Gia_ManSimPatSimOut( p, vSimsPi, 1 );
    int i, Count = 0, nErrors = 0;
    for ( i = 0; i < Gia_ManCoNum(p); i++ )
    {
        int CountThis = Abc_TtCountOnesVec( Vec_WrdEntryP(vSimsPo, i*nWords), nWords );
        if ( CountThis == 0 )
            continue;
        printf( "%d ", CountThis );
        nErrors += CountThis;
        Count++;
    }
    printf( "\nDetected %d error POs with %d errors (average %.2f).\n", Count, nErrors, 1.0*nErrors/Abc_MaxInt(1, Count) );
    Vec_WrdFree( vSimsPi );
    Vec_WrdFree( vSimsPo );
}

/**Function*************************************************************

  Synopsis    [Default parameter settings.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec4_ManSetParams( Cec_ParFra_t * pPars )
{
    memset( pPars, 0, sizeof(Cec_ParFra_t) );
    pPars->jType          =       2;    // solver type
    pPars->fSatSweeping   =       1;    // conflict limit at a node
    pPars->nWords         =       4;    // simulation words
    pPars->nRounds        =      10;    // simulation rounds
    pPars->nItersMax      =    2000;    // this is a miter
    pPars->nBTLimit       = 1000000;    // use logic cones
    pPars->nBTLimitPo     =       0;    // use logic outputs
    pPars->nSatVarMax     =    1000;    // the max number of SAT variables before recycling SAT solver
    pPars->nCallsRecycle  =     500;    // calls to perform before recycling SAT solver
    pPars->nGenIters      =     100;    // pattern generation iterations
    pPars->fBMiterInfo    =       0;    // printing BMiter information
}

/**Function*************************************************************

  Synopsis    [Default parameter settings.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec_SimGenSetParDefault( Cec_ParSimGen_t * pPars )
{
    memset( pPars, 0, sizeof(Cec_ParSimGen_t) );
    pPars->fVerbose             = 0; // verbose output
    pPars->fVeryVerbose         = 0; // verbose output and outputs files
    pPars->bitwidthOutgold      = 2; // bitwidth of the output golden model
    pPars->nSimWords          = 4; // number of words in a round of random simulation
    pPars->expId                = 1; // experiment ID
    pPars->nMaxIter             = -1; // the maximum number of rounds of random simulation
    pPars->timeOutSim           = 1000.0; // the timeout for simulation in sec
    pPars->fUseWatchlist        = 0; // use watchlist
    pPars->fImplicationTime     = 0.0; // time spent in implication
    pPars->nImplicationExecution = 0; // the number of implication executions
    pPars->nImplicationSuccess   = 0; // the number of implication successes
    pPars->nImplicationTotalChecks = 0; // the number of implication checks
    pPars->nImplicationSuccessChecks = 0; // the number of implication successful checks
    pPars->pFileName            = NULL; // file name to dump simulation vectors
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cec4_Man_t * Cec4_ManCreate( Gia_Man_t * pAig, Cec_ParFra_t * pPars )
{
    Cec4_Man_t * p = ABC_CALLOC( Cec4_Man_t, 1 );
    memset( p, 0, sizeof(Cec4_Man_t) );
    p->timeStart     = Abc_Clock();
    p->pPars         = pPars;
    p->pAig          = pAig;
    p->pSat          = sat_solver_start();  
    sat_solver_set_jftr( p->pSat, pPars->jType );
    p->vFrontier     = Vec_PtrAlloc( 1000 );
    p->vFanins       = Vec_PtrAlloc( 100 );
    p->vCexMin       = Vec_IntAlloc( 100 );
    p->vClassUpdates = Vec_IntAlloc( 100 );
    p->vCexStamps    = Vec_IntStart( Gia_ManObjNum(pAig) );
    p->vCands        = Vec_IntAlloc( 100 );
    p->vVisit        = Vec_IntAlloc( 100 );
    p->vPat          = Vec_IntAlloc( 100 );
    p->vDisprPairs   = Vec_IntAlloc( 100 );
    p->vFails        = Vec_BitStart( Gia_ManObjNum(pAig) );
    p->vPairs        = pPars->fUseCones ? Vec_IntAlloc( 100 ) : NULL;
    //pAig->pData     = p->pSat; // point AIG manager to the solver
    //Vec_IntFreeP( &p->pAig->vPats );
    //p->pAig->vPats = Vec_IntAlloc( 1000 );
    if ( pPars->nBTLimitPo )
    {
        int i, Driver;
        p->vCoDrivers = Vec_BitStart( Gia_ManObjNum(pAig) );
        Gia_ManForEachCoDriverId( pAig, Driver, i )
            Vec_BitWriteEntry( p->vCoDrivers, Driver, 1 );
    }
    return p;
}
void Cec4_ManDestroy( Cec4_Man_t * p )
{
    if ( p->pPars->fVerbose ) 
    {
        abctime timeTotal = Abc_Clock() - p->timeStart;
        abctime timeSat   = p->timeSatSat0 + p->timeSatSat + p->timeSatUnsat0 + p->timeSatUnsat + p->timeSatUndec;
        abctime timeOther = timeTotal - timeSat - p->timeSim - p->timeRefine - p->timeResimLoc - p->timeGenPats;// - p->timeResimGlo;
        ABC_PRTP( "SAT solving  ", timeSat,          timeTotal );
        ABC_PRTP( "  sat(easy)  ", p->timeSatSat0,   timeTotal );
        ABC_PRTP( "  sat        ", p->timeSatSat,    timeTotal );
        ABC_PRTP( "  unsat(easy)", p->timeSatUnsat0, timeTotal );
        ABC_PRTP( "  unsat      ", p->timeSatUnsat,  timeTotal );
        ABC_PRTP( "  fail       ", p->timeSatUndec,  timeTotal );
        ABC_PRTP( "Generate CNF ", p->timeCnf,       timeTotal );
        ABC_PRTP( "Generate pats", p->timeGenPats,   timeTotal );
        ABC_PRTP( "Simulation   ", p->timeSim,       timeTotal );
        ABC_PRTP( "Refinement   ", p->timeRefine,    timeTotal );
        ABC_PRTP( "Resim global ", p->timeResimGlo,  timeTotal );
        ABC_PRTP( "Resim local  ", p->timeResimLoc,  timeTotal );
        ABC_PRTP( "Other        ", timeOther,        timeTotal );
        ABC_PRTP( "TOTAL        ", timeTotal,        timeTotal );
        fflush( stdout );
    }
    //printf( "Recorded %d patterns with %d literals (average %.2f).\n", 
    //    p->pAig->nBitPats, Vec_IntSize(p->pAig->vPats) - 2*p->pAig->nBitPats, 1.0*Vec_IntSize(p->pAig->vPats)/Abc_MaxInt(1, p->pAig->nBitPats)-2 );
    //Cec4_EvalPatterns( p->pAig, p->pAig->vPats, p->pAig->nBitPats );
    //Vec_IntFreeP( &p->pAig->vPats );
    Vec_WrdFreeP( &p->pAig->vSims );
    Vec_WrdFreeP( &p->pAig->vSimsPi );
    Gia_ManCleanMark01( p->pAig );
    sat_solver_stop( p->pSat );
    Gia_ManStopP( &p->pNew );
    Vec_PtrFreeP( &p->vFrontier );
    Vec_PtrFreeP( &p->vFanins );
    Vec_IntFreeP( &p->vCexMin );
    Vec_IntFreeP( &p->vClassUpdates );
    Vec_IntFreeP( &p->vCexStamps );
    Vec_IntFreeP( &p->vCands );
    Vec_IntFreeP( &p->vVisit );
    Vec_IntFreeP( &p->vPat );
    Vec_IntFreeP( &p->vDisprPairs );
    Vec_BitFreeP( &p->vFails );
    Vec_IntFreeP( &p->vPairs );
    Vec_BitFreeP( &p->vCoDrivers );
    Vec_IntFreeP( &p->vRefClasses );
    Vec_IntFreeP( &p->vRefNodes );
    Vec_IntFreeP( &p->vRefBins );
    ABC_FREE( p->pTable );
    ABC_FREE( p );
}
Gia_Man_t * Cec4_ManStartNew( Gia_Man_t * pAig )
{
    Gia_Obj_t * pObj; int i;
    Gia_Man_t * pNew = Gia_ManStart( Gia_ManObjNum(pAig) );
    pNew->pName = Abc_UtilStrsav( pAig->pName );
    pNew->pSpec = Abc_UtilStrsav( pAig->pSpec );
    if ( pAig->pMuxes )
        pNew->pMuxes = ABC_CALLOC( unsigned, pNew->nObjsAlloc );
    Gia_ManFillValue( pAig );
    Gia_ManConst0(pAig)->Value = 0;
    Gia_ManForEachCi( pAig, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManHashAlloc( pNew );
    Vec_IntFill( &pNew->vCopies2, Gia_ManObjNum(pAig), -1 );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(pAig) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Adds clauses to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec4_AddClausesMux( Gia_Man_t * p, Gia_Obj_t * pNode, sat_solver * pSat )
{
    int fPolarFlip = 0;
    Gia_Obj_t * pNodeI, * pNodeT, * pNodeE;
    int pLits[4], RetValue, VarF, VarI, VarT, VarE, fCompT, fCompE;

    assert( !Gia_IsComplement( pNode ) );
    assert( pNode->fMark0 );
    // get nodes (I = if, T = then, E = else)
    pNodeI = Gia_ObjRecognizeMux( pNode, &pNodeT, &pNodeE );
    // get the variable numbers
    VarF = Cec4_ObjSatId(p, pNode);
    VarI = Cec4_ObjSatId(p, pNodeI);
    VarT = Cec4_ObjSatId(p, Gia_Regular(pNodeT));
    VarE = Cec4_ObjSatId(p, Gia_Regular(pNodeE));
    // get the complementation flags
    fCompT = Gia_IsComplement(pNodeT);
    fCompE = Gia_IsComplement(pNodeE);

    // f = ITE(i, t, e)

    // i' + t' + f
    // i' + t  + f'
    // i  + e' + f
    // i  + e  + f'

    // create four clauses
    pLits[0] = Abc_Var2Lit(VarI, 1);
    pLits[1] = Abc_Var2Lit(VarT, 1^fCompT);
    pLits[2] = Abc_Var2Lit(VarF, 0);
    if ( fPolarFlip )
    {
        if ( pNodeI->fPhase )               pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeT)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = sat_solver_addclause( pSat, pLits, 3 );
    assert( RetValue );
    pLits[0] = Abc_Var2Lit(VarI, 1);
    pLits[1] = Abc_Var2Lit(VarT, 0^fCompT);
    pLits[2] = Abc_Var2Lit(VarF, 1);
    if ( fPolarFlip )
    {
        if ( pNodeI->fPhase )               pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeT)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = sat_solver_addclause( pSat, pLits, 3 );
    assert( RetValue );
    pLits[0] = Abc_Var2Lit(VarI, 0);
    pLits[1] = Abc_Var2Lit(VarE, 1^fCompE);
    pLits[2] = Abc_Var2Lit(VarF, 0);
    if ( fPolarFlip )
    {
        if ( pNodeI->fPhase )               pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeE)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = sat_solver_addclause( pSat, pLits, 3 );
    assert( RetValue );
    pLits[0] = Abc_Var2Lit(VarI, 0);
    pLits[1] = Abc_Var2Lit(VarE, 0^fCompE);
    pLits[2] = Abc_Var2Lit(VarF, 1);
    if ( fPolarFlip )
    {
        if ( pNodeI->fPhase )               pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeE)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = sat_solver_addclause( pSat, pLits, 3 );
    assert( RetValue );

    // two additional clauses
    // t' & e' -> f'
    // t  & e  -> f 

    // t  + e   + f'
    // t' + e'  + f 

    if ( VarT == VarE )
    {
//        assert( fCompT == !fCompE );
        return;
    }

    pLits[0] = Abc_Var2Lit(VarT, 0^fCompT);
    pLits[1] = Abc_Var2Lit(VarE, 0^fCompE);
    pLits[2] = Abc_Var2Lit(VarF, 1);
    if ( fPolarFlip )
    {
        if ( Gia_Regular(pNodeT)->fPhase )  pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeE)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = sat_solver_addclause( pSat, pLits, 3 );
    assert( RetValue );
    pLits[0] = Abc_Var2Lit(VarT, 1^fCompT);
    pLits[1] = Abc_Var2Lit(VarE, 1^fCompE);
    pLits[2] = Abc_Var2Lit(VarF, 0);
    if ( fPolarFlip )
    {
        if ( Gia_Regular(pNodeT)->fPhase )  pLits[0] = Abc_LitNot( pLits[0] );
        if ( Gia_Regular(pNodeE)->fPhase )  pLits[1] = Abc_LitNot( pLits[1] );
        if ( pNode->fPhase )                pLits[2] = Abc_LitNot( pLits[2] );
    }
    RetValue = sat_solver_addclause( pSat, pLits, 3 );
    assert( RetValue );
}
void Cec4_AddClausesSuper( Gia_Man_t * p, Gia_Obj_t * pNode, Vec_Ptr_t * vSuper, sat_solver * pSat )
{
    int fPolarFlip = 0;
    Gia_Obj_t * pFanin;
    int * pLits, nLits, RetValue, i;
    assert( !Gia_IsComplement(pNode) );
    assert( Gia_ObjIsAnd( pNode ) );
    // create storage for literals
    nLits = Vec_PtrSize(vSuper) + 1;
    pLits = ABC_ALLOC( int, nLits );
    // suppose AND-gate is A & B = C
    // add !A => !C   or   A + !C
    Vec_PtrForEachEntry( Gia_Obj_t *, vSuper, pFanin, i )
    {
        pLits[0] = Abc_Var2Lit(Cec4_ObjSatId(p, Gia_Regular(pFanin)), Gia_IsComplement(pFanin));
        pLits[1] = Abc_Var2Lit(Cec4_ObjSatId(p, pNode), 1);
        if ( fPolarFlip )
        {
            if ( Gia_Regular(pFanin)->fPhase )  pLits[0] = Abc_LitNot( pLits[0] );
            if ( pNode->fPhase )                pLits[1] = Abc_LitNot( pLits[1] );
        }
        RetValue = sat_solver_addclause( pSat, pLits, 2 );
        assert( RetValue );
    }
    // add A & B => C   or   !A + !B + C
    Vec_PtrForEachEntry( Gia_Obj_t *, vSuper, pFanin, i )
    {
        pLits[i] = Abc_Var2Lit(Cec4_ObjSatId(p, Gia_Regular(pFanin)), !Gia_IsComplement(pFanin));
        if ( fPolarFlip )
        {
            if ( Gia_Regular(pFanin)->fPhase )  pLits[i] = Abc_LitNot( pLits[i] );
        }
    }
    pLits[nLits-1] = Abc_Var2Lit(Cec4_ObjSatId(p, pNode), 0);
    if ( fPolarFlip )
    {
        if ( pNode->fPhase )  pLits[nLits-1] = Abc_LitNot( pLits[nLits-1] );
    }
    RetValue = sat_solver_addclause( pSat, pLits, nLits );
    assert( RetValue );
    ABC_FREE( pLits );
}

/**Function*************************************************************

  Synopsis    [Adds clauses and returns CNF variable of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec4_CollectSuper_rec( Gia_Obj_t * pObj, Vec_Ptr_t * vSuper, int fFirst, int fUseMuxes )
{
    // if the new node is complemented or a PI, another gate begins
    if ( Gia_IsComplement(pObj) || Gia_ObjIsCi(pObj) || 
         (!fFirst && Gia_ObjValue(pObj) > 1) || 
         (fUseMuxes && pObj->fMark0) )
    {
        Vec_PtrPushUnique( vSuper, pObj );
        return;
    }
    // go through the branches
    Cec4_CollectSuper_rec( Gia_ObjChild0(pObj), vSuper, 0, fUseMuxes );
    Cec4_CollectSuper_rec( Gia_ObjChild1(pObj), vSuper, 0, fUseMuxes );
}
void Cec4_CollectSuper( Gia_Obj_t * pObj, int fUseMuxes, Vec_Ptr_t * vSuper )
{
    assert( !Gia_IsComplement(pObj) );
    assert( !Gia_ObjIsCi(pObj) );
    Vec_PtrClear( vSuper );
    Cec4_CollectSuper_rec( pObj, vSuper, 1, fUseMuxes );
}
void Cec4_ObjAddToFrontier( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Ptr_t * vFrontier, sat_solver * pSat )
{
    assert( !Gia_IsComplement(pObj) );
    assert( !Gia_ObjIsConst0(pObj) );
    if ( Cec4_ObjSatId(p, pObj) >= 0 )
        return;
    assert( Cec4_ObjSatId(p, pObj) == -1 );
    Cec4_ObjSetSatId( p, pObj, sat_solver_addvar(pSat) );
    if ( Gia_ObjIsAnd(pObj) )
        Vec_PtrPush( vFrontier, pObj );
}
int Cec4_ObjGetCnfVar( Cec4_Man_t * p, int iObj )
{ 
    int fUseSimple = 1; // enable simple CNF
    int fUseMuxes  = 1; // enable MUXes when using complex CNF
    Gia_Obj_t * pNode, * pFanin;
    Gia_Obj_t * pObj = Gia_ManObj(p->pNew, iObj);
    int i, k;
    // quit if CNF is ready
    if ( Cec4_ObjSatId(p->pNew,pObj) >= 0 )
        return Cec4_ObjSatId(p->pNew,pObj);
    assert( iObj > 0 );
    if ( Gia_ObjIsCi(pObj) )
        return Cec4_ObjSetSatId( p->pNew, pObj, sat_solver_addvar(p->pSat) );
    assert( Gia_ObjIsAnd(pObj) );
    if ( fUseSimple )
    {
        Gia_Obj_t * pFan0, * pFan1;
        //if ( Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
        //    printf( "%d", (Gia_IsComplement(pFan1) << 1) + Gia_IsComplement(pFan0) );
        if ( p->pNew->pMuxes == NULL && Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) && Gia_IsComplement(pFan0) == Gia_IsComplement(pFan1) )
        {
            int iVar0 = Cec4_ObjGetCnfVar( p, Gia_ObjId(p->pNew, Gia_Regular(pFan0)) );
            int iVar1 = Cec4_ObjGetCnfVar( p, Gia_ObjId(p->pNew, Gia_Regular(pFan1)) );
            int iVar  = Cec4_ObjSetSatId( p->pNew, pObj, sat_solver_addvar(p->pSat) );
            if ( p->pPars->jType < 2 )
                sat_solver_add_xor( p->pSat, iVar, iVar0, iVar1, 0 );
            if ( p->pPars->jType > 0 )
            {
                int Lit0 = Abc_Var2Lit( iVar0, 0 );
                int Lit1 = Abc_Var2Lit( iVar1, 0 );
                if ( Lit0 < Lit1 )
                     Lit1 ^= Lit0, Lit0 ^= Lit1, Lit1 ^= Lit0;
                assert( Lit0 > Lit1 );
                sat_solver_set_var_fanin_lit( p->pSat, iVar, Lit0, Lit1 );
                p->nGates[1]++;
            }
        }
        else
        {
            int iVar0 = Cec4_ObjGetCnfVar( p, Gia_ObjFaninId0(pObj, iObj) );
            int iVar1 = Cec4_ObjGetCnfVar( p, Gia_ObjFaninId1(pObj, iObj) );
            int iVar  = Cec4_ObjSetSatId( p->pNew, pObj, sat_solver_addvar(p->pSat) );
            if ( p->pPars->jType < 2 )
            {
                if ( Gia_ObjIsXor(pObj) )
                    sat_solver_add_xor( p->pSat, iVar, iVar0, iVar1, Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC1(pObj) );
                else
                    sat_solver_add_and( p->pSat, iVar, iVar0, iVar1, Gia_ObjFaninC0(pObj), Gia_ObjFaninC1(pObj), 0 );
            }
            if ( p->pPars->jType > 0 )
            {
                int Lit0 = Abc_Var2Lit( iVar0, Gia_ObjFaninC0(pObj) );
                int Lit1 = Abc_Var2Lit( iVar1, Gia_ObjFaninC1(pObj) );
                if ( (Lit0 > Lit1) ^ Gia_ObjIsXor(pObj) )
                     Lit1 ^= Lit0, Lit0 ^= Lit1, Lit1 ^= Lit0;
                sat_solver_set_var_fanin_lit( p->pSat, iVar, Lit0, Lit1 );
                p->nGates[Gia_ObjIsXor(pObj)]++;
            }
        }
        return Cec4_ObjSatId( p->pNew, pObj );
    }
    assert( !Gia_ObjIsXor(pObj) );
    // start the frontier
    Vec_PtrClear( p->vFrontier );
    Cec4_ObjAddToFrontier( p->pNew, pObj, p->vFrontier, p->pSat );
    // explore nodes in the frontier
    Vec_PtrForEachEntry( Gia_Obj_t *, p->vFrontier, pNode, i )
    {
        // create the supergate
        assert( Cec4_ObjSatId(p->pNew,pNode) >= 0 );
        if ( fUseMuxes && pNode->fMark0 )
        {
            Vec_PtrClear( p->vFanins );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin0( Gia_ObjFanin0(pNode) ) );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin0( Gia_ObjFanin1(pNode) ) );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin1( Gia_ObjFanin0(pNode) ) );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin1( Gia_ObjFanin1(pNode) ) );
            Vec_PtrForEachEntry( Gia_Obj_t *, p->vFanins, pFanin, k )
                Cec4_ObjAddToFrontier( p->pNew, Gia_Regular(pFanin), p->vFrontier, p->pSat );
            Cec4_AddClausesMux( p->pNew, pNode, p->pSat );
        }
        else
        {
            Cec4_CollectSuper( pNode, fUseMuxes, p->vFanins );
            Vec_PtrForEachEntry( Gia_Obj_t *, p->vFanins, pFanin, k )
                Cec4_ObjAddToFrontier( p->pNew, Gia_Regular(pFanin), p->vFrontier, p->pSat );
            Cec4_AddClausesSuper( p->pNew, pNode, p->vFanins, p->pSat );
        }
        assert( Vec_PtrSize(p->vFanins) > 1 );
    }
    return Cec4_ObjSatId(p->pNew,pObj);
}


/**Function*************************************************************

  Synopsis    [Refinement of equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word * Cec4_ObjSim( Gia_Man_t * p, int iObj )
{
    return Vec_WrdEntryP( p->vSims, p->nSimWords * iObj );
}
static inline int Cec4_ObjSimEqual( Gia_Man_t * p, int iObj0, int iObj1 )
{
    int w;
    word * pSim0 = Cec4_ObjSim( p, iObj0 );
    word * pSim1 = Cec4_ObjSim( p, iObj1 );
    if ( (pSim0[0] & 1) == (pSim1[0] & 1) )
    {
        for ( w = 0; w < p->nSimWords; w++ )
            if ( pSim0[w] != pSim1[w] )
                return 0;
        return 1;
    }
    else
    {
        for ( w = 0; w < p->nSimWords; w++ )
            if ( pSim0[w] != ~pSim1[w] )
                return 0;
        return 1;
    }
}
int Cec4_ManSimHashKey( word * pSim, int nSims, int nTableSize )
{
    static int s_Primes[16] = { 
        1291, 1699, 1999, 2357, 2953, 3313, 3907, 4177, 
        4831, 5147, 5647, 6343, 6899, 7103, 7873, 8147 };
    unsigned uHash = 0, * pSimU = (unsigned *)pSim;
    int i, nSimsU = 2 * nSims;
    if ( pSimU[0] & 1 )
        for ( i = 0; i < nSimsU; i++ )
            uHash ^= ~pSimU[i] * s_Primes[i & 0xf];
    else
        for ( i = 0; i < nSimsU; i++ )
            uHash ^= pSimU[i] * s_Primes[i & 0xf];
    return (int)(uHash % nTableSize);

}
void Cec4_RefineOneClassIter( Gia_Man_t * p, int iRepr )
{
    int iObj, iPrev = iRepr, iPrev2, iRepr2;
    assert( Gia_ObjRepr(p, iRepr) == GIA_VOID );
    assert( Gia_ObjNext(p, iRepr) > 0 );
    Gia_ClassForEachObj1( p, iRepr, iRepr2 )
        if ( Cec4_ObjSimEqual(p, iRepr, iRepr2) )
            iPrev = iRepr2;
        else
            break;
    if ( iRepr2 <= 0 ) // no refinement
        return;
    // relink remaining nodes of the class
    // nodes that are equal to iRepr, remain in the class of iRepr
    // nodes that are not equal to iRepr, move to the class of iRepr2
    Gia_ObjSetRepr( p, iRepr2, GIA_VOID );
    iPrev2 = iRepr2;
    for ( iObj = Gia_ObjNext(p, iRepr2); iObj > 0; iObj = Gia_ObjNext(p, iObj) )
    {
        if ( Cec4_ObjSimEqual(p, iRepr, iObj) ) // remains with iRepr
        {
            Gia_ObjSetNext( p, iPrev, iObj );
            iPrev = iObj;
        }
        else // moves to iRepr2
        {
            Gia_ObjSetRepr( p, iObj, iRepr2 );
            Gia_ObjSetNext( p, iPrev2, iObj );
            iPrev2 = iObj;
        }
    }
    Gia_ObjSetNext( p, iPrev, -1 );
    Gia_ObjSetNext( p, iPrev2, -1 );
    // refine incrementally
    if ( Gia_ObjNext(p, iRepr2) > 0 )
        Cec4_RefineOneClassIter( p, iRepr2 );
}
void Cec4_RefineOneClass( Gia_Man_t * p, Cec4_Man_t * pMan, Vec_Int_t * vNodes )
{
    int k, iObj, Bin;
    Vec_IntClear( pMan->vRefBins );
    Vec_IntForEachEntryReverse( vNodes, iObj, k )
    {
        int Key = Cec4_ManSimHashKey( Cec4_ObjSim(p, iObj), p->nSimWords, pMan->nTableSize );
        assert( Key >= 0 && Key < pMan->nTableSize );
        if ( pMan->pTable[Key] == -1 )
            Vec_IntPush( pMan->vRefBins, Key );
        p->pNexts[iObj] = pMan->pTable[Key];
        pMan->pTable[Key] = iObj;
    }
    Vec_IntForEachEntry( pMan->vRefBins, Bin, k )
    {
        int iRepr = pMan->pTable[Bin];
        pMan->pTable[Bin] = -1;
        assert( p->pReprs[iRepr].iRepr == GIA_VOID );
        assert( p->pNexts[iRepr] != 0 );
        if ( p->pNexts[iRepr] == -1 )
            continue;
        for ( iObj = p->pNexts[iRepr]; iObj > 0; iObj = p->pNexts[iObj] )
            p->pReprs[iObj].iRepr = iRepr;
        Cec4_RefineOneClassIter( p, iRepr );
    }
    Vec_IntClear( pMan->vRefBins );
}
void Cec4_RefineClasses( Gia_Man_t * p, Cec4_Man_t * pMan, Vec_Int_t * vClasses )
{
    if ( Vec_IntSize(pMan->vRefClasses) == 0 )
        return;
    if ( Vec_IntSize(pMan->vRefNodes) > 0 )
        Cec4_RefineOneClass( p, pMan, pMan->vRefNodes );
    else
    {
        int i, k, iObj, iRepr;
        Vec_IntForEachEntry( pMan->vRefClasses, iRepr, i )
        {
            assert( p->pReprs[iRepr].fColorA );
            p->pReprs[iRepr].fColorA = 0;
            Vec_IntClear( pMan->vRefNodes );
            Vec_IntPush( pMan->vRefNodes, iRepr );
            Gia_ClassForEachObj1( p, iRepr, k )
                Vec_IntPush( pMan->vRefNodes, k );
            Vec_IntForEachEntry( pMan->vRefNodes, iObj, k )
            {
                p->pReprs[iObj].iRepr = GIA_VOID;
                p->pNexts[iObj] = -1;
            }
            Cec4_RefineOneClass( p, pMan, pMan->vRefNodes );
        }
    }
    Vec_IntClear( pMan->vRefClasses );
    Vec_IntClear( pMan->vRefNodes );
}
void Cec4_RefineInit( Gia_Man_t * p, Cec4_Man_t * pMan )
{
    Gia_Obj_t * pObj; int i;
    ABC_FREE( p->pReprs );
    ABC_FREE( p->pNexts );
    p->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p) );
    p->pNexts = ABC_FALLOC( int, Gia_ManObjNum(p) );
    pMan->nTableSize  = Abc_PrimeCudd( Gia_ManObjNum(p) );
    pMan->pTable      = ABC_FALLOC( int, pMan->nTableSize );
    pMan->vRefNodes   = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManForEachObj( p, pObj, i )
    {
        p->pReprs[i].iRepr = GIA_VOID;
        if ( !Gia_ObjIsCo(pObj) && (!pMan->pPars->nLevelMax || Gia_ObjLevel(p, pObj) <= pMan->pPars->nLevelMax) )
            Vec_IntPush( pMan->vRefNodes, i );
    }
    pMan->vRefBins    = Vec_IntAlloc( Gia_ManObjNum(p)/2 );
    pMan->vRefClasses = Vec_IntAlloc( Gia_ManObjNum(p)/2 );
    Vec_IntPush( pMan->vRefClasses, 0 );
}


/**Function*************************************************************

  Synopsis    [Internal simulation APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cec4_ObjSimSetInputBit( Gia_Man_t * p, int iObj, int Bit )
{
    word * pSim = Cec4_ObjSim( p, iObj );
    if ( Abc_InfoHasBit( (unsigned*)pSim, p->iPatsPi ) != Bit )
        Abc_InfoXorBit( (unsigned*)pSim, p->iPatsPi );
}
static inline int Cec4_ObjSimGetInputBit( Gia_Man_t * p, int iObj )
{
    word * pSim = Cec4_ObjSim( p, iObj );
    return Abc_InfoHasBit( (unsigned*)pSim, p->iPatsPi );
}
static inline void Cec4_ObjSimRo( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSimRo = Cec4_ObjSim( p, iObj );
    word * pSimRi = Cec4_ObjSim( p, Gia_ObjRoToRiId(p, iObj) );
    for ( w = 0; w < p->nSimWords; w++ )
        pSimRo[w] = pSimRi[w];
}
static inline void Cec4_ObjSimCo( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSimCo  = Cec4_ObjSim( p, iObj );
    word * pSimDri = Cec4_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] = ~pSimDri[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] =  pSimDri[w];
}
static inline void Cec4_ObjSimAnd( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSim  = Cec4_ObjSim( p, iObj );
    word * pSim0 = Cec4_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    word * pSim1 = Cec4_ObjSim( p, Gia_ObjFaninId1(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] & ~pSim1[w];
    else if ( Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] & pSim1[w];
    else if ( !Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = pSim0[w] & ~pSim1[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = pSim0[w] & pSim1[w];
}
static inline void Cec4_ObjSimXor( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSim  = Cec4_ObjSim( p, iObj );
    word * pSim0 = Cec4_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    word * pSim1 = Cec4_ObjSim( p, Gia_ObjFaninId1(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC1(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = ~pSim0[w] ^ pSim1[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] =  pSim0[w] ^ pSim1[w];
}
static inline void Cec4_ObjSimCi( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSim = Cec4_ObjSim( p, iObj );
    for ( w = 0; w < p->nSimWords; w++ )
        pSim[w] = Abc_RandomW( 0 );
    pSim[0] <<= 1;
}
static inline void Cec4_ObjClearSimCi( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSim = Cec4_ObjSim( p, iObj );
    for ( w = 0; w < p->nSimWords; w++ )
        pSim[w] = 0;
}
void Cec4_ManSimulateCis( Gia_Man_t * p )
{
    int i, Id;
    Gia_ManForEachCiId( p, Id, i )
        Cec4_ObjSimCi( p, Id );
    p->iPatsPi = 0;
}
void Cec4_ManClearCis( Gia_Man_t * p )
{
    int i, Id;
    Gia_ManForEachCiId( p, Id, i )
        Cec4_ObjClearSimCi( p, Id );
    p->iPatsPi = 0;
}
Abc_Cex_t * Cec4_ManDeriveCex( Gia_Man_t * p, int iOut, int iPat )
{
    Abc_Cex_t * pCex;
    int i, Id;
    pCex = Abc_CexAlloc( 0, Gia_ManCiNum(p), 1 );
    pCex->iPo = iOut;
    if ( iPat == -1 )
        return pCex;
    Gia_ManForEachCiId( p, Id, i )
        if ( Abc_InfoHasBit((unsigned *)Cec4_ObjSim(p, Id), iPat) )
            Abc_InfoSetBit( pCex->pData, i );
    return pCex;
}
int Cec4_ManSimulateCos( Gia_Man_t * p )
{
    int i, Id;
    // check outputs and generate CEX if they fail
    Gia_ManForEachCoId( p, Id, i )
    {
        Cec4_ObjSimCo( p, Id );
        if ( Cec4_ObjSimEqual(p, Id, 0) )
            continue;
        p->pCexSeq = Cec4_ManDeriveCex( p, i, Abc_TtFindFirstBit2(Cec4_ObjSim(p, Id), p->nSimWords) );
        return 0;
    }
    return 1;
}
void Cec4_ManSimulate( Gia_Man_t * p, Cec4_Man_t * pMan )
{
    abctime clk = Abc_Clock();
    Gia_Obj_t * pObj; int i;
    pMan->nSimulates++;
    if ( pMan->pTable == NULL )
        Cec4_RefineInit( p, pMan );
    else
        assert( Vec_IntSize(pMan->vRefClasses) == 0 );
    Gia_ManForEachAnd( p, pObj, i )
    {
        int iRepr = Gia_ObjRepr( p, i );
        if ( Gia_ObjIsXor(pObj) )
            Cec4_ObjSimXor( p, i );
        else
            Cec4_ObjSimAnd( p, i );
        if ( iRepr == GIA_VOID || p->pReprs[iRepr].fColorA || Cec4_ObjSimEqual(p, iRepr, i) )
            continue;
        p->pReprs[iRepr].fColorA = 1;
        Vec_IntPush( pMan->vRefClasses, iRepr );
    }
    pMan->timeSim += Abc_Clock() - clk;
    clk = Abc_Clock();
    Cec4_RefineClasses( p, pMan, pMan->vRefClasses );
    pMan->timeRefine += Abc_Clock() - clk;
}
void Cec4_ManSimulate_rec( Gia_Man_t * p, Cec4_Man_t * pMan, int iObj )
{
    Gia_Obj_t * pObj; 
    if ( !iObj || Vec_IntEntry(pMan->vCexStamps, iObj) == p->iPatsPi )
        return;
    Vec_IntWriteEntry( pMan->vCexStamps, iObj, p->iPatsPi );
    pObj = Gia_ManObj(p, iObj);
    if ( Gia_ObjIsCi(pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Cec4_ManSimulate_rec( p, pMan, Gia_ObjFaninId0(pObj, iObj) );
    Cec4_ManSimulate_rec( p, pMan, Gia_ObjFaninId1(pObj, iObj) );
    if ( Gia_ObjIsXor(pObj) )
        Cec4_ObjSimXor( p, iObj );
    else
        Cec4_ObjSimAnd( p, iObj );
}
void Cec4_ManSimAlloc( Gia_Man_t * p, int nWords )
{
    Vec_WrdFreeP( &p->vSims );
    Vec_WrdFreeP( &p->vSimsPi );
    p->vSims     = Vec_WrdStart( Gia_ManObjNum(p) * nWords );
    p->vSimsPi   = Vec_WrdStart( (Gia_ManCiNum(p) + 1) * nWords );
    p->nSimWords = nWords;
}


/**Function*************************************************************

  Synopsis    [Creating initial equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec4_ManPrintTfiConeStats( Gia_Man_t * p )
{
    Vec_Int_t * vRoots  = Vec_IntAlloc( 100 );
    Vec_Int_t * vNodes  = Vec_IntAlloc( 100 );
    Vec_Int_t * vLeaves = Vec_IntAlloc( 100 );
    int i, k;
    Gia_ManForEachClass0( p, i )
    {
        Vec_IntClear( vRoots );
        if ( i % 100 != 0 )
            continue;
        Vec_IntPush( vRoots, i );
        Gia_ClassForEachObj1( p, i, k )
            Vec_IntPush( vRoots, k );
        Gia_ManCollectTfi( p, vRoots, vNodes );
        printf( "Class %6d : ", i );
        printf( "Roots = %6d  ", Vec_IntSize(vRoots) );
        printf( "Nodes = %6d  ", Vec_IntSize(vNodes) );
        printf( "\n" );
    }
    Vec_IntFree( vRoots );
    Vec_IntFree( vNodes );
    Vec_IntFree( vLeaves );
}
void Cec4_ManPrintStats( Gia_Man_t * p, Cec_ParFra_t * pPars, Cec4_Man_t * pMan, int fSim )
{
    static abctime clk = 0;
    abctime clkThis = 0;
    int i, nLits, Counter = 0, Counter0 = 0, CounterX = 0;
    if ( !pPars->fVerbose )
        return;
    if ( pMan->nItersSim + pMan->nItersSat )
        clkThis = Abc_Clock() - clk;
    clk = Abc_Clock();
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
    {
        if ( Gia_ObjIsHead(p, i) )
            Counter++;
        else if ( Gia_ObjIsConst(p, i) )
            Counter0++;
        else if ( Gia_ObjIsNone(p, i) )
            CounterX++;
    }
    nLits = Gia_ManObjNum(p) - Counter - CounterX;
    if ( fSim )
    {
        printf( "Sim %4d : ",     pMan->nItersSim++ + pMan->nItersSat );
        printf( "%6.2f %%  ", 100.0*nLits/Gia_ManCandNum(p) );
    }
    else
    {
        printf( "SAT %4d : ",     pMan->nItersSim + pMan->nItersSat++ );
        printf( "%6.2f %%  ", 100.0*pMan->nAndNodes/Gia_ManAndNum(p) );
    }
    printf( "P =%7d  ",   pMan ? pMan->nSatUnsat : 0 );
    printf( "D =%7d  ",   pMan ? pMan->nSatSat   : 0 );
    printf( "F =%8d  ",   pMan ? pMan->nSatUndec : 0 );
    //printf( "Last =%6d  ", pMan ? pMan->iLastConst : 0 );
    Abc_Print( 1, "cst =%9d  cls =%8d  lit =%9d   ",  Counter0, Counter, nLits );
    Abc_PrintTime( 1, "Time", clkThis );
}
void Cec4_ManPrintClasses2( Gia_Man_t * p )
{
    int i, k;
    Gia_ManForEachClass0( p, i )
    {
        printf( "Class %d : ", i );
        Gia_ClassForEachObj1( p, i, k )
            printf( "%d ", k );
        printf( "\n" );
    }
}
void Cec4_ManPrintClasses( Gia_Man_t * p )
{
    int k, Count = 0;
    Gia_ClassForEachObj1( p, 0, k )
        Count++;
    printf( "Const0 class has %d entries.\n", Count );
}


/**Function*************************************************************

  Synopsis    [Verify counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec4_ManVerify_rec( Gia_Man_t * p, int iObj, sat_solver * pSat )
{
    int Value0, Value1;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    if ( iObj == 0 ) return 0;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return pObj->fMark1;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    if ( Gia_ObjIsCi(pObj) )
        return pObj->fMark1 = sat_solver_read_cex_varvalue(pSat, Cec4_ObjSatId(p, pObj));
    assert( Gia_ObjIsAnd(pObj) );
    Value0 = Cec4_ManVerify_rec( p, Gia_ObjFaninId0(pObj, iObj), pSat ) ^ Gia_ObjFaninC0(pObj);
    Value1 = Cec4_ManVerify_rec( p, Gia_ObjFaninId1(pObj, iObj), pSat ) ^ Gia_ObjFaninC1(pObj);
    return pObj->fMark1 = Gia_ObjIsXor(pObj) ? Value0 ^ Value1 : Value0 & Value1;
}
void Cec4_ManVerify( Gia_Man_t * p, int iObj0, int iObj1, int fPhase, sat_solver * pSat )
{
    int Value0, Value1;
    Gia_ManIncrementTravId( p );
    Value0 = Cec4_ManVerify_rec( p, iObj0, pSat );
    Value1 = Cec4_ManVerify_rec( p, iObj1, pSat );
    if ( (Value0 ^ Value1) == fPhase )
        printf( "CEX verification FAILED for obj %d and obj %d.\n", iObj0, iObj1 );
//    else
//        printf( "CEX verification succeeded for obj %d and obj %d.\n", iObj0, iObj1 );;
}


/**Function*************************************************************

  Synopsis    [Verify counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Cec4_ManCexVerify_rec( Gia_Man_t * p, int iObj )
{
    int Value0, Value1;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    if ( iObj == 0 ) return 0;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return pObj->fMark1;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    if ( Gia_ObjIsCi(pObj) )
        return pObj->fMark1 = Cec4_ObjSimGetInputBit(p, iObj);
    assert( Gia_ObjIsAnd(pObj) );
    Value0 = Cec4_ManCexVerify_rec( p, Gia_ObjFaninId0(pObj, iObj) ) ^ Gia_ObjFaninC0(pObj);
    Value1 = Cec4_ManCexVerify_rec( p, Gia_ObjFaninId1(pObj, iObj) ) ^ Gia_ObjFaninC1(pObj);
    return pObj->fMark1 = Gia_ObjIsXor(pObj) ? Value0 ^ Value1 : Value0 & Value1;
}
void Cec4_ManCexVerify( Gia_Man_t * p, int iObj0, int iObj1, int fPhase )
{
    int Value0, Value1;
    Gia_ManIncrementTravId( p );
    Value0 = Cec4_ManCexVerify_rec( p, iObj0 );
    Value1 = Cec4_ManCexVerify_rec( p, iObj1 );
    if ( (Value0 ^ Value1) == fPhase )
        printf( "CEX verification FAILED for obj %d and obj %d.\n", iObj0, iObj1 );
//    else
//        printf( "CEX verification succeeded for obj %d and obj %d.\n", iObj0, iObj1 );;
}

/**Function*************************************************************

  Synopsis    [Packs simulation patterns into array of simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

*************************************`**********************************/
void Cec4_ManPackAddPatterns( Gia_Man_t * p, int iBit, Vec_Int_t * vLits )
{
    int k, Limit = Abc_MinInt( Vec_IntSize(vLits), 64 * p->nSimWords - 1 );
    for ( k = 0; k < Limit; k++ )
    {
        int i, Lit, iBitLocal = (iBit + k + 1) % Limit + 1;
        assert( iBitLocal > 0 && iBitLocal < 64 * p->nSimWords );
        Vec_IntForEachEntry( vLits, Lit, i )
        {
            word * pInfo = Vec_WrdEntryP( p->vSims,   p->nSimWords * Abc_Lit2Var(Lit) );
            word * pPres = Vec_WrdEntryP( p->vSimsPi, p->nSimWords * Abc_Lit2Var(Lit) );
            if ( Abc_InfoHasBit( (unsigned *)pPres, iBitLocal ) )
                continue;
            if ( Abc_InfoHasBit( (unsigned *)pInfo, iBitLocal ) != Abc_LitIsCompl(Lit ^ (i == k)) )
                 Abc_InfoXorBit( (unsigned *)pInfo, iBitLocal );
        }
    }
}
int Cec4_ManPackAddPatternTry( Gia_Man_t * p, int iBit, Vec_Int_t * vLits )
{
    int i, Lit;
    assert( p->iPatsPi > 0 && p->iPatsPi < 64 * p->nSimWords );
    Vec_IntForEachEntry( vLits, Lit, i )
    {
        word * pInfo = Vec_WrdEntryP( p->vSims,   p->nSimWords * Abc_Lit2Var(Lit) );
        word * pPres = Vec_WrdEntryP( p->vSimsPi, p->nSimWords * Abc_Lit2Var(Lit) );
        if ( Abc_InfoHasBit( (unsigned *)pPres, iBit ) && 
             Abc_InfoHasBit( (unsigned *)pInfo, iBit ) != Abc_LitIsCompl(Lit) )
             return 0;
    }
    Vec_IntForEachEntry( vLits, Lit, i )
    {
        word * pInfo = Vec_WrdEntryP( p->vSims,   p->nSimWords * Abc_Lit2Var(Lit) );
        word * pPres = Vec_WrdEntryP( p->vSimsPi, p->nSimWords * Abc_Lit2Var(Lit) );
        Abc_InfoSetBit( (unsigned *)pPres, iBit );
        if ( Abc_InfoHasBit( (unsigned *)pInfo, iBit ) != Abc_LitIsCompl(Lit) )
             Abc_InfoXorBit( (unsigned *)pInfo, iBit );
    }
    return 1;
}
int Cec4_ManPackAddPattern( Gia_Man_t * p, Vec_Int_t * vLits, int fExtend )
{
    int k;
    for ( k = 1; k < 64 * p->nSimWords - 1; k++ )
    {
        if ( ++p->iPatsPi == 64 * p->nSimWords - 1 )
            p->iPatsPi = 1;
        if ( Cec4_ManPackAddPatternTry( p, p->iPatsPi, vLits ) )
        {
            if ( fExtend )
                Cec4_ManPackAddPatterns( p, p->iPatsPi, vLits );
            break;
        }
    }
    if ( k == 64 * p->nSimWords - 1 )
    {
        p->iPatsPi = k;
        if ( !Cec4_ManPackAddPatternTry( p, p->iPatsPi, vLits ) )
            printf( "Internal error.\n" );
        else if ( fExtend )
            Cec4_ManPackAddPatterns( p, p->iPatsPi, vLits );
        return 64 * p->nSimWords;
    }
    return k;
}

/**Function*************************************************************

  Synopsis    [Generates counter-examples to refine the candidate equivalences.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Cec4_ObjFan0IsAssigned( Gia_Obj_t * pObj )
{
    return Gia_ObjFanin0(pObj)->fMark0 || Gia_ObjFanin0(pObj)->fMark1;
}
static inline int Cec4_ObjFan1IsAssigned( Gia_Obj_t * pObj )
{
    return Gia_ObjFanin1(pObj)->fMark0 || Gia_ObjFanin1(pObj)->fMark1;
}
static inline int Cec4_ObjFan0HasValue( Gia_Obj_t * pObj, int v )
{
    return (v ^ Gia_ObjFaninC0(pObj)) ? Gia_ObjFanin0(pObj)->fMark1 : Gia_ObjFanin0(pObj)->fMark0;
}
static inline int Cec4_ObjFan1HasValue( Gia_Obj_t * pObj, int v )
{
    return (v ^ Gia_ObjFaninC1(pObj)) ? Gia_ObjFanin1(pObj)->fMark1 : Gia_ObjFanin1(pObj)->fMark0;
}
static inline int Cec4_ObjObjIsImpliedValue( Gia_Obj_t * pObj, int v )
{
    assert( !pObj->fMark0 && !pObj->fMark1 ); // not visited
    if ( v ) 
    return Cec4_ObjFan0HasValue(pObj, 1) && Cec4_ObjFan1HasValue(pObj, 1);
    return Cec4_ObjFan0HasValue(pObj, 0) || Cec4_ObjFan1HasValue(pObj, 0);
}
static inline int Cec4_ObjFan0IsImpliedValue( Gia_Obj_t * pObj, int v )
{
    return Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) && Cec4_ObjObjIsImpliedValue( Gia_ObjFanin0(pObj), v ^ Gia_ObjFaninC0(pObj) );
}
static inline int Cec4_ObjFan1IsImpliedValue( Gia_Obj_t * pObj, int v )
{
    return Gia_ObjIsAnd(Gia_ObjFanin1(pObj)) && Cec4_ObjObjIsImpliedValue( Gia_ObjFanin1(pObj), v ^ Gia_ObjFaninC1(pObj) );
}
int Cec4_ManGeneratePatterns_rec( Gia_Man_t * p, Gia_Obj_t * pObj, int Value, Vec_Int_t * vPat, Vec_Int_t * vVisit )
{
    Gia_Obj_t * pFan0, * pFan1;
    assert( !pObj->fMark0 && !pObj->fMark1 ); // not visited
    if ( Value ) pObj->fMark1 = 1; else pObj->fMark0 = 1;
    Vec_IntPush( vVisit, Gia_ObjId(p, pObj) );
    if ( Gia_ObjIsCi(pObj) )
    {
        Vec_IntPush( vPat, Abc_Var2Lit(Gia_ObjId(p, pObj), Value) );
        return 1;
    }
    assert( Gia_ObjIsAnd(pObj) );
    pFan0 = Gia_ObjFanin0(pObj);
    pFan1 = Gia_ObjFanin1(pObj);
    if ( Gia_ObjIsXor(pObj) )
    {
        int Ass0 = Cec4_ObjFan0IsAssigned(pObj);
        int Ass1 = Cec4_ObjFan1IsAssigned(pObj);
        assert( Gia_ObjFaninC0(pObj) == 0 && Gia_ObjFaninC1(pObj) == 0 );
        if ( Ass0 && Ass1 )
            return Value == (Cec4_ObjFan0HasValue(pObj, 1) ^ Cec4_ObjFan1HasValue(pObj, 1));
        if ( Ass0 )
        {
            int ValueInt = Value ^ Cec4_ObjFan0HasValue(pObj, 1);
            if ( !Cec4_ManGeneratePatterns_rec(p, pFan1, ValueInt, vPat, vVisit) )
                return 0;
        }
        else if ( Ass1 )
        {
            int ValueInt = Value ^ Cec4_ObjFan1HasValue(pObj, 1);
            if ( !Cec4_ManGeneratePatterns_rec(p, pFan0, ValueInt, vPat, vVisit) )
                return 0;
        }
        else if ( Abc_Random(0) & 1 )
        {
            if ( !Cec4_ManGeneratePatterns_rec(p, pFan0, 0,      vPat, vVisit) )
                return 0;
            if ( Cec4_ObjFan1HasValue(pObj, !Value) || (!Cec4_ObjFan1HasValue(pObj, Value) && !Cec4_ManGeneratePatterns_rec(p, pFan1, Value,  vPat, vVisit)) )
                return 0;
        }
        else
        {
            if ( !Cec4_ManGeneratePatterns_rec(p, pFan0, 1,      vPat, vVisit) )
                return 0;
            if ( Cec4_ObjFan1HasValue(pObj, Value) || (!Cec4_ObjFan1HasValue(pObj, !Value) && !Cec4_ManGeneratePatterns_rec(p, pFan1, !Value, vPat, vVisit)) )
                return 0;
        }
        assert( Value == (Cec4_ObjFan0HasValue(pObj, 1) ^ Cec4_ObjFan1HasValue(pObj, 1)) );
        return 1;
    }
    else if ( Value )
    {
        if ( Cec4_ObjFan0HasValue(pObj, 0) || Cec4_ObjFan1HasValue(pObj, 0) )
            return 0;
        if ( !Cec4_ObjFan0HasValue(pObj, 1) && !Cec4_ManGeneratePatterns_rec(p, pFan0, !Gia_ObjFaninC0(pObj), vPat, vVisit) )
            return 0;
        if ( !Cec4_ObjFan1HasValue(pObj, 1) && !Cec4_ManGeneratePatterns_rec(p, pFan1, !Gia_ObjFaninC1(pObj), vPat, vVisit) )
            return 0;
        assert( Cec4_ObjFan0HasValue(pObj, 1) && Cec4_ObjFan1HasValue(pObj, 1) );
        return 1;
    }
    else
    {
        if ( Cec4_ObjFan0HasValue(pObj, 1) && Cec4_ObjFan1HasValue(pObj, 1) )
            return 0;
        if ( Cec4_ObjFan0HasValue(pObj, 0) || Cec4_ObjFan1HasValue(pObj, 0) )
            return 1;
        if ( Cec4_ObjFan0HasValue(pObj, 1) )
        {
            if ( !Cec4_ManGeneratePatterns_rec(p, pFan1, Gia_ObjFaninC1(pObj), vPat, vVisit) )
                return 0;
        }
        else if ( Cec4_ObjFan1HasValue(pObj, 1) )
        {
            if ( !Cec4_ManGeneratePatterns_rec(p, pFan0, Gia_ObjFaninC0(pObj), vPat, vVisit) )
                return 0;
        }
        else
        {
            if ( Cec4_ObjFan0IsImpliedValue( pObj, 0 ) )
            {
                if ( !Cec4_ManGeneratePatterns_rec(p, pFan0, Gia_ObjFaninC0(pObj), vPat, vVisit) )
                    return 0;
            }
            else if ( Cec4_ObjFan1IsImpliedValue( pObj, 0 ) )
            {
                if ( !Cec4_ManGeneratePatterns_rec(p, pFan1, Gia_ObjFaninC1(pObj), vPat, vVisit) )
                    return 0;
            }
            else if ( Cec4_ObjFan0IsImpliedValue( pObj, 1 ) )
            {
                if ( !Cec4_ManGeneratePatterns_rec(p, pFan1, Gia_ObjFaninC1(pObj), vPat, vVisit) )
                    return 0;
            }
            else if ( Cec4_ObjFan1IsImpliedValue( pObj, 1 ) )
            {
                if ( !Cec4_ManGeneratePatterns_rec(p, pFan0, Gia_ObjFaninC0(pObj), vPat, vVisit) )
                    return 0;
            }
            else if ( Abc_Random(0) & 1 )
            {
                if ( !Cec4_ManGeneratePatterns_rec(p, pFan1, Gia_ObjFaninC1(pObj), vPat, vVisit) )
                    return 0;
            }
            else
            {
                if ( !Cec4_ManGeneratePatterns_rec(p, pFan0, Gia_ObjFaninC0(pObj), vPat, vVisit) )
                    return 0;
            }
        }
        assert( Cec4_ObjFan0HasValue(pObj, 0) || Cec4_ObjFan1HasValue(pObj, 0) );
        return 1;
    }
}
int Cec4_ManGeneratePatternOne( Gia_Man_t * p, int iRepr, int iReprVal, int iCand, int iCandVal, Vec_Int_t * vPat, Vec_Int_t * vVisit )
{
    int Res, k;
    Gia_Obj_t * pObj;
    assert( iCand > 0 );
    if ( !iRepr && iReprVal )
        return 0;
    Vec_IntClear( vPat );
    Vec_IntClear( vVisit );
    //Gia_ManForEachObj( p, pObj, k )
    //    assert( !pObj->fMark0 && !pObj->fMark1 );
    Res = (!iRepr || Cec4_ManGeneratePatterns_rec(p, Gia_ManObj(p, iRepr), iReprVal, vPat, vVisit)) && Cec4_ManGeneratePatterns_rec(p, Gia_ManObj(p, iCand), iCandVal, vPat, vVisit);
    Gia_ManForEachObjVec( vVisit, p, pObj, k )
        pObj->fMark0 = pObj->fMark1 = 0;
    return Res;
}
void Cec4_ManCandIterStart( Cec4_Man_t * p )
{
    int i, * pArray;
    assert( p->iPosWrite == 0 );
    assert( p->iPosRead == 0 );
    assert( Vec_IntSize(p->vCands) == 0 );
    for ( i = 1; i < Gia_ManObjNum(p->pAig); i++ )
        if ( Gia_ObjRepr(p->pAig, i) != GIA_VOID )
            Vec_IntPush( p->vCands, i );
    pArray = Vec_IntArray( p->vCands );
    for ( i = 0; i < Vec_IntSize(p->vCands); i++ )
    {
        int iNew = Abc_Random(0) % Vec_IntSize(p->vCands);
        ABC_SWAP( int, pArray[i], pArray[iNew] );
    }
}
int Cec4_ManCandIterNext( Cec4_Man_t * p )
{
    while ( Vec_IntSize(p->vCands) > 0 )
    {
        int fStop, iCand = Vec_IntEntry( p->vCands, p->iPosRead );
        if ( (fStop = (Gia_ObjRepr(p->pAig, iCand) != GIA_VOID)) )
            Vec_IntWriteEntry( p->vCands, p->iPosWrite++, iCand );
        if ( ++p->iPosRead == Vec_IntSize(p->vCands) )
        {
            Vec_IntShrink( p->vCands, p->iPosWrite );
            p->iPosWrite = 0;
            p->iPosRead = 0;
        }
        if ( fStop )
            return iCand;
    }
    return 0;
}
int Cec4_ManGeneratePatterns( Cec4_Man_t * p )
{
    abctime clk = Abc_Clock();
    int i, iCand, nPats = 100 * 64 * p->pAig->nSimWords, CountPat = 0, Packs = 0;
    //int iRepr;
    //Vec_IntForEachEntryDouble( p->vDisprPairs, iRepr, iCand, i )
    //    if ( iRepr == Gia_ObjRepr(p->pAig, iCand) )
    //        printf( "Pair %6d (%6d, %6d) (new repr = %9d) is FAILED to disprove.\n", i, iRepr, iCand, Gia_ObjRepr(p->pAig, iCand) );
    //    else
    //        printf( "Pair %6d (%6d, %6d) (new repr = %9d) is disproved.\n", i, iRepr, iCand, Gia_ObjRepr(p->pAig, iCand) );
    //Vec_IntClear( p->vDisprPairs );
    p->pAig->iPatsPi = 0;
    Vec_WrdFill( p->pAig->vSimsPi, Vec_WrdSize(p->pAig->vSimsPi), 0 );
    for ( i = 0; i < nPats; i++ )
        if ( (iCand = Cec4_ManCandIterNext(p)) )
        {
            int iRepr    = Gia_ObjRepr( p->pAig, iCand );
            int iCandVal = Gia_ManObj(p->pAig, iCand)->fPhase;
            int iReprVal = Gia_ManObj(p->pAig, iRepr)->fPhase;
            int Res = Cec4_ManGeneratePatternOne( p->pAig, iRepr,  iReprVal, iCand, !iCandVal, p->vPat, p->vVisit );
            if ( !Res )
                Res = Cec4_ManGeneratePatternOne( p->pAig, iRepr, !iReprVal, iCand,  iCandVal, p->vPat, p->vVisit );
            if ( Res )
            {
                int Ret = Cec4_ManPackAddPattern( p->pAig, p->vPat, 1 );
                if ( p->pAig->vPats )
                {
                    Vec_IntPush( p->pAig->vPats, Vec_IntSize(p->vPat)+2 );
                    Vec_IntAppend( p->pAig->vPats, p->vPat );
                    Vec_IntPush( p->pAig->vPats, -1 );
                }
                //Vec_IntPushTwo( p->vDisprPairs, iRepr, iCand );
                Packs += Ret;
                if ( Ret == 64 * p->pAig->nSimWords )
                    break;
                if ( ++CountPat == 8 * 64 * p->pAig->nSimWords )
                    break;
                //Cec4_ManCexVerify( p->pAig, iRepr, iCand, iReprVal ^ iCandVal );
                //Gia_ManCleanMark01( p->pAig );
            }
        }
    p->timeGenPats += Abc_Clock() - clk;
    p->nSatSat += CountPat;
    //printf( "%3d : %6.2f %% : Generated %6d CEXs after trying %6d pairs.  Ave packs = %9.2f  Ave tries = %9.2f  (Limit = %9.2f)\n", 
    //    p->nItersSim++, 100.0*Vec_IntSize(p->vCands)/Gia_ManAndNum(p->pAig),
    //    CountPat, i, (float)Packs / Abc_MaxInt(1, CountPat), (float)i / Abc_MaxInt(1, CountPat), (float)nPats / p->pPars->nItersMax );
    return CountPat >= i / p->pPars->nItersMax;
}


/**Function*************************************************************

  Synopsis    [Internal simulation APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec4_ManSatSolverRecycle( Cec4_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    //printf( "Solver size = %d.\n", sat_solver_varnum(p->pSat) );
    p->nRecycles++;
    p->nCallsSince = 0;
    sat_solver_reset( p->pSat );
    // clean mapping of AigIds into SatIds
    Gia_ManForEachObjVec( &p->pNew->vSuppVars, p->pNew, pObj, i )
        Cec4_ObjCleanSatId( p->pNew, pObj );
    Vec_IntClear( &p->pNew->vSuppVars  );  // AigIds for which SatId is defined
    Vec_IntClear( &p->pNew->vCopiesTwo );  // pairs (CiAigId, SatId)
    Vec_IntClear( &p->pNew->vVarMap    );  // mapping of SatId into AigId
}
int Cec4_ManSolveTwo( Cec4_Man_t * p, int iObj0, int iObj1, int fPhase, int * pfEasy, int fVerbose, int fEffort )
{
    abctime clk;
    int nBTLimit = fEffort ? p->pPars->nBTLimitPo : (Vec_BitEntry(p->vFails, iObj0) || Vec_BitEntry(p->vFails, iObj1)) ? Abc_MaxInt(1, p->pPars->nBTLimit/10) : p->pPars->nBTLimit;
    int nConfEnd, nConfBeg, status, iVar0, iVar1, Lits[2];
    int UnsatConflicts[3] = {0};
    //printf( "%d ", nBTLimit );
    if ( iObj1 <  iObj0 ) 
         iObj1 ^= iObj0, iObj0 ^= iObj1, iObj1 ^= iObj0;
    assert( iObj0 < iObj1 );    
    *pfEasy = 0;
    // check if SAT solver needs recycling
    p->nCallsSince++; 
    if ( p->nCallsSince > p->pPars->nCallsRecycle && 
         Vec_IntSize(&p->pNew->vSuppVars) > p->pPars->nSatVarMax && p->pPars->nSatVarMax )
        Cec4_ManSatSolverRecycle( p );
    // add more logic to the solver
    if ( !iObj0 && Cec4_ObjSatId(p->pNew, Gia_ManConst0(p->pNew)) == -1 )
        Cec4_ObjSetSatId( p->pNew, Gia_ManConst0(p->pNew), sat_solver_addvar(p->pSat) );
    clk = Abc_Clock();
    iVar0 = Cec4_ObjGetCnfVar( p, iObj0 );
    iVar1 = Cec4_ObjGetCnfVar( p, iObj1 );
    if( p->pPars->jType > 0 )
    {
        sat_solver_start_new_round( p->pSat );
        sat_solver_mark_cone( p->pSat, Cec4_ObjSatId(p->pNew, Gia_ManObj(p->pNew, iObj0)) );
        sat_solver_mark_cone( p->pSat, Cec4_ObjSatId(p->pNew, Gia_ManObj(p->pNew, iObj1)) );
    }
    p->timeCnf += Abc_Clock() - clk;
    // perform solving
    Lits[0] = Abc_Var2Lit(iVar0, 1);
    Lits[1] = Abc_Var2Lit(iVar1, fPhase);
    sat_solver_set_conflict_budget( p->pSat, nBTLimit );
    nConfBeg = sat_solver_conflictnum( p->pSat );
    status = sat_solver_solve( p->pSat, Lits, 2 );
    nConfEnd = sat_solver_conflictnum( p->pSat );
    assert( nConfEnd >= nConfBeg );
    if ( fVerbose )
    {
        if ( status == GLUCOSE_SAT ) 
        {
            p->nConflicts[0][0] += nConfEnd == nConfBeg;
            p->nConflicts[0][1] += nConfEnd -  nConfBeg;
            p->nConflicts[0][2]  = Abc_MaxInt(p->nConflicts[0][2], nConfEnd - nConfBeg);
            *pfEasy = nConfEnd == nConfBeg;
        }
        else if ( status == GLUCOSE_UNSAT ) 
        {
            if ( iObj0 > 0 )
            {
                UnsatConflicts[0] = nConfEnd == nConfBeg;
                UnsatConflicts[1] = nConfEnd -  nConfBeg;
                UnsatConflicts[2] = Abc_MaxInt(p->nConflicts[1][2], nConfEnd - nConfBeg);
            }
            else
            {
                p->nConflicts[1][0] += nConfEnd == nConfBeg;
                p->nConflicts[1][1] += nConfEnd -  nConfBeg;
                p->nConflicts[1][2]  = Abc_MaxInt(p->nConflicts[1][2], nConfEnd - nConfBeg);
                *pfEasy = nConfEnd == nConfBeg;
            }
        }
    }
    if ( status == GLUCOSE_UNSAT && iObj0 > 0 )
    {
        Lits[0] = Abc_Var2Lit(iVar0, 0);
        Lits[1] = Abc_Var2Lit(iVar1, !fPhase);
        sat_solver_set_conflict_budget( p->pSat, nBTLimit );
        nConfBeg = sat_solver_conflictnum( p->pSat );
        status = sat_solver_solve( p->pSat, Lits, 2 );
        nConfEnd = sat_solver_conflictnum( p->pSat );
        assert( nConfEnd >= nConfBeg );
        if ( fVerbose )
        {
            if ( status == GLUCOSE_SAT ) 
            {
                p->nConflicts[0][0] += nConfEnd == nConfBeg;
                p->nConflicts[0][1] += nConfEnd -  nConfBeg;
                p->nConflicts[0][2]  = Abc_MaxInt(p->nConflicts[0][2], nConfEnd - nConfBeg);
                *pfEasy = nConfEnd == nConfBeg;
            }
            else if ( status == GLUCOSE_UNSAT ) 
            {
                UnsatConflicts[0]  &= nConfEnd == nConfBeg;
                UnsatConflicts[1]  += nConfEnd -  nConfBeg;
                UnsatConflicts[2]   = Abc_MaxInt(p->nConflicts[1][2], nConfEnd - nConfBeg);

                p->nConflicts[1][0] += UnsatConflicts[0];
                p->nConflicts[1][1] += UnsatConflicts[1];
                p->nConflicts[1][2]  = Abc_MaxInt(p->nConflicts[1][2], UnsatConflicts[2]);
                *pfEasy = UnsatConflicts[0];
            }
        }
    }
    //if ( status == GLUCOSE_UNDEC )
    //    printf( "*  " );
    return status;
}
int Cec4_ManSweepNode( Cec4_Man_t * p, int iObj, int iRepr )
{
    abctime clk = Abc_Clock();
    int i, IdAig, IdSat, status, fEasy, RetValue = 1;
    Gia_Obj_t * pObj = Gia_ManObj( p->pAig, iObj );
    Gia_Obj_t * pRepr = Gia_ManObj( p->pAig, iRepr );
    int fCompl = Abc_LitIsCompl(pObj->Value) ^ Abc_LitIsCompl(pRepr->Value) ^ pObj->fPhase ^ pRepr->fPhase;
    int fEffort = p->vCoDrivers ? Vec_BitEntry(p->vCoDrivers, iObj) || Vec_BitEntry(p->vCoDrivers, iRepr) : 0;
    status = Cec4_ManSolveTwo( p, Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value), fCompl, &fEasy, p->pPars->fVerbose, fEffort );
    if ( status == GLUCOSE_SAT )
    {
        int iLit;
        //int iPatsOld = p->pAig->iPatsPi;
        //printf( "Disproved: %d == %d.\n", Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value) );
        p->nSatSat++;
        p->nPatterns++;
        Vec_IntClear( p->vPat );
        if ( p->pPars->jType == 0 )
        {
            Vec_IntForEachEntryDouble( &p->pNew->vCopiesTwo, IdAig, IdSat, i )
                Vec_IntPush( p->vPat, Abc_Var2Lit(IdAig, sat_solver_read_cex_varvalue(p->pSat, IdSat)) );
        }
        else
        {
            int * pCex = sat_solver_read_cex( p->pSat );
            int * pMap = Vec_IntArray(&p->pNew->vVarMap);
            for ( i = 0; i < pCex[0]; )
                Vec_IntPush( p->vPat, Abc_Lit2LitV(pMap, Abc_LitNot(pCex[++i])) );
        }
        assert( p->pAig->iPatsPi >= 0 && p->pAig->iPatsPi < 64 * p->pAig->nSimWords - 1 );
        p->pAig->iPatsPi++;
        Vec_IntForEachEntry( p->vPat, iLit, i )
            Cec4_ObjSimSetInputBit( p->pAig, Abc_Lit2Var(iLit), Abc_LitIsCompl(iLit) );
        if ( p->pAig->vPats )
        {
            Vec_IntPush( p->pAig->vPats, Vec_IntSize(p->vPat)+2 );
            Vec_IntAppend( p->pAig->vPats, p->vPat );
            Vec_IntPush( p->pAig->vPats, -1 );
        }
        //Cec4_ManPackAddPattern( p->pAig, p->vPat, 0 );
        //assert( iPatsOld + 1 == p->pAig->iPatsPi );
        if ( fEasy )
            p->timeSatSat0 += Abc_Clock() - clk;
        else
            p->timeSatSat += Abc_Clock() - clk;
        RetValue = 0;
        // this is not needed, but we keep it here anyway, because it takes very little time
        //Cec4_ManVerify( p->pNew, Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value), fCompl, p->pSat );
        // resimulated once in a while
        if ( p->pAig->iPatsPi == 64 * p->pAig->nSimWords - 2 )
        {
            abctime clk2 = Abc_Clock();
            Cec4_ManSimulate( p->pAig, p );
            //printf( "FasterSmall = %d.  FasterBig = %d.\n", p->nFaster[0], p->nFaster[1] );
            p->nFaster[0] = p->nFaster[1] = 0;
            //if ( p->nSatSat && p->nSatSat % 100 == 0 )
                Cec4_ManPrintStats( p->pAig, p->pPars, p, 0 );
            Vec_IntFill( p->vCexStamps, Gia_ManObjNum(p->pAig), 0 );
            p->pAig->iPatsPi = 0;
            Vec_WrdFill( p->pAig->vSimsPi, Vec_WrdSize(p->pAig->vSimsPi), 0 );
            p->timeResimGlo += Abc_Clock() - clk2;
        }
    }
    else if ( status == GLUCOSE_UNSAT )
    {
        //printf( "Proved: %d == %d.\n", Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value) );
        p->nSatUnsat++;
        pObj->Value = Abc_LitNotCond( pRepr->Value, fCompl );
        Gia_ObjSetProved( p->pAig, iObj );
        if ( iRepr == 0 )
            p->iLastConst = iObj;
        if ( fEasy )
            p->timeSatUnsat0 += Abc_Clock() - clk;
        else
            p->timeSatUnsat += Abc_Clock() - clk;
        RetValue = 1;
    }
    else 
    {
        p->nSatUndec++;
        assert( status == GLUCOSE_UNDEC );
        if ( p->vPairs ) // speculate
        {
            Vec_IntPushTwo( p->vPairs, Abc_Var2Lit(iRepr, 0), Abc_Var2Lit(iObj, fCompl) );
            p->timeSatUndec += Abc_Clock() - clk;
            // mark as proved
            pObj->Value = Abc_LitNotCond( pRepr->Value, fCompl );
            Gia_ObjSetProved( p->pAig, iObj );
            if ( iRepr == 0 )
                p->iLastConst = iObj;
            RetValue = 1;
        }
        else
        {
            Gia_ObjSetFailed( p->pAig, iObj );
            Vec_BitWriteEntry( p->vFails, iObj, 1 );
            //if ( iRepr )
            //Vec_BitWriteEntry( p->vFails, iRepr, 1 );
            p->timeSatUndec += Abc_Clock() - clk;
            RetValue = 2;
        }
    }
    return RetValue;
}
Gia_Obj_t * Cec4_ManFindRepr( Gia_Man_t * p, Cec4_Man_t * pMan, int iObj )
{
    abctime clk = Abc_Clock();
    int iMem, iRepr;
    assert( Gia_ObjHasRepr(p, iObj) );
    assert( !Gia_ObjProved(p, iObj) );
    iRepr = Gia_ObjRepr(p, iObj);
    assert( iRepr != iObj );
    assert( !Gia_ObjProved(p, iRepr) );
    Cec4_ManSimulate_rec( p, pMan, iObj );
    Cec4_ManSimulate_rec( p, pMan, iRepr );
    if ( Cec4_ObjSimEqual(p, iObj, iRepr) )
    {
        pMan->timeResimLoc += Abc_Clock() - clk;
        return Gia_ManObj(p, iRepr);
    }
    Gia_ClassForEachObj1( p, iRepr, iMem )
    {
        if ( iObj == iMem )
            break;
        if ( Gia_ObjProved(p, iMem) || Gia_ObjFailed(p, iMem) )
            continue;
        Cec4_ManSimulate_rec( p, pMan, iMem );
        if ( Cec4_ObjSimEqual(p, iObj, iMem) )
        {
            pMan->nFaster[0]++;
            pMan->timeResimLoc += Abc_Clock() - clk;
            return Gia_ManObj(p, iMem);
        }
    }
    pMan->nFaster[1]++;
    pMan->timeResimLoc += Abc_Clock() - clk;
    return NULL;
}
void Gia_ManRemoveWrongChoices( Gia_Man_t * p )
{
    int i = 0, iObj, iPrev, Counter = 0;
    for ( iPrev = i, iObj = Gia_ObjNext(p, i); -1 < iObj; iObj = Gia_ObjNext(p, iPrev) )
    {
        Gia_Obj_t * pRepr = Gia_ObjReprObj(p, iObj);
        assert( pRepr == Gia_ManConst0(p) );
        if( !Gia_ObjFailed(p,iObj) && Abc_Lit2Var(Gia_ManObj(p,iObj)->Value) == Abc_Lit2Var(pRepr->Value) )
        {
            iPrev = iObj;
            continue;
        }
        Gia_ObjSetRepr( p, iObj, GIA_VOID );
        Gia_ObjSetNext( p, iPrev, Gia_ObjNext(p, iObj) );
        Gia_ObjSetNext( p, iObj, 0 );
        Counter++;
    }
    Gia_ManForEachClass( p, i )
    {
        for ( iPrev = i, iObj = Gia_ObjNext(p, i); -1 < iObj; iObj = Gia_ObjNext(p, iPrev) )
        {
            Gia_Obj_t * pRepr = Gia_ObjReprObj(p, iObj);
            if( !Gia_ObjFailed(p,iObj) && Abc_Lit2Var(Gia_ManObj(p,iObj)->Value) == Abc_Lit2Var(pRepr->Value) )
            {
                iPrev = iObj;
                continue;
            }
            Gia_ObjSetRepr( p, iObj, GIA_VOID );
            Gia_ObjSetNext( p, iPrev, Gia_ObjNext(p, iObj) );
            Gia_ObjSetNext( p, iObj, 0 );
            Counter++;
        }
    }
    //Abc_Print( 1, "Removed %d wrong choices.\n", Counter );
}

void Cec4_ManSimulateDumpInfo( Cec4_Man_t * pMan )
{
    Gia_Obj_t * pObj; int i, k, nWords = pMan->pAig->nSimWords, nOuts[2] = {0};
    Vec_Wrd_t * vSims = NULL, * vSimsPi = NULL;
    FILE * pFile = fopen( pMan->pPars->pDumpName, "wb" );
    if ( pFile == NULL ) {
        printf( "Cannot open file \"%s\" for writing primary output information.\n", pMan->pPars->pDumpName );
        return;
    }
    vSimsPi = Vec_WrdDup( pMan->pAig->vSimsPi );
    memmove( Vec_WrdArray(vSimsPi), Vec_WrdArray(vSimsPi) + nWords, Gia_ManCiNum(pMan->pAig) * nWords );
    Vec_WrdShrink( vSimsPi, Gia_ManCiNum(pMan->pAig) * nWords );
    if ( Abc_TtIsConst0(Vec_WrdArray(vSimsPi), Gia_ManCiNum(pMan->pAig) * nWords) ) {
        Vec_WrdFree( vSimsPi );
        vSimsPi = Vec_WrdStartRandom( Gia_ManCiNum(pMan->pAig) * nWords );
    }
    vSims = Gia_ManSimPatSimOut( pMan->pAig, vSimsPi, 1 );
    assert( nWords * Gia_ManCiNum(pMan->pAig) == Vec_WrdSize(vSimsPi) );
    Gia_ManForEachCo( pMan->pAig, pObj, i )
    {
        void Extra_PrintHex2( FILE * pFile, unsigned * pTruth, int nVars );
        word * pSims = Vec_WrdEntryP( vSims, nWords*i );
        //Extra_PrintHex2( stdout, (unsigned *)pSims, 8 ); printf( "\n" );
        fprintf( pFile, "%d ", i );
        if ( Gia_ObjFaninLit0p(pMan->pNew, Gia_ManCo(pMan->pNew, i)) == 0 ) 
            nOuts[0]++;
        else if ( Abc_TtIsConst0(pSims, nWords) )
            fprintf( pFile, "-" );
        else {
            int iPat = Abc_TtFindFirstBit2(pSims, nWords);
            for ( k = 0; k < Gia_ManPiNum(pMan->pAig); k++ )
                fprintf( pFile, "%d", Abc_TtGetBit(Vec_WrdEntryP(vSimsPi, nWords*k), iPat) );
            nOuts[1]++;
        }
        fprintf( pFile, "\n" );
    }
    printf( "Information about %d sat, %d unsat, and %d undecided primary outputs was written into file \"%s\".\n", 
        nOuts[1], nOuts[0], Gia_ManCoNum(pMan->pAig)-nOuts[1]-nOuts[0], pMan->pPars->pDumpName );
    fclose( pFile );
    Vec_WrdFree( vSims );
    Vec_WrdFree( vSimsPi );
}
int Cec4_ManPerformSweeping( Gia_Man_t * p, Cec_ParFra_t * pPars, Gia_Man_t ** ppNew, int fSimOnly )
{

    Cec4_Man_t * pMan = Cec4_ManCreate( p, pPars ); 
    Gia_Obj_t * pObj, * pRepr; 
    int i, fSimulate = 1, Id;
    if ( pPars->fVerbose )
        printf( "Solver type = %d. Simulate %d words in %d rounds. SAT with %d confs. Recycle after %d SAT calls.\n", 
            pPars->jType, pPars->nWords, pPars->nRounds, pPars->nBTLimit, pPars->nCallsRecycle );

    // this is currently needed to have a correct mapping
    Gia_ManForEachCi( p, pObj, i )
        assert( Gia_ObjId(p, pObj) == i+1 );

    // check if any output trivially fails under all-0 pattern
    Abc_Random( 1 );
    Gia_ManSetPhase( p );
    if ( pPars->nLevelMax )
        Gia_ManLevelNum(p);
    //Gia_ManStaticFanoutStart( p );
    if ( pPars->fCheckMiter ) 
    {
        Gia_ManForEachCo( p, pObj, i )
            if ( pObj->fPhase )
            {
                p->pCexSeq = Cec4_ManDeriveCex( p, i, -1 );
                goto finalize;
            }
    }

    if( p->vSimsPi && p->nSimWords > 0){ // if the simulation pis are already set, do not generate new ones 
        pPars->nWords = p->nSimWords;
        Vec_WrdFreeP( &p->vSims );
        p->vSims     = Vec_WrdStart( Gia_ManObjNum(p) *  pPars->nWords);
        assert( Vec_WrdSize(p->vSimsPi) == Gia_ManCiNum(p) * p->nSimWords ); 
        Gia_ManForEachCiId(p, Id, i){
          memmove( Vec_WrdEntryP(p->vSims, Id*p->nSimWords), Vec_WrdEntryP(p->vSimsPi, i*p->nSimWords), sizeof(word)*p->nSimWords );
        }
        Cec4_ManSimulate( p, pMan );
        if ( pPars->fCheckMiter && !Cec4_ManSimulateCos(p) ) // cex detected
          goto finalize;
        if ( fSimOnly )
          goto finalize;
        goto execute_sat;
    }

    // simulate one round and create classes
    Cec4_ManSimAlloc( p, pPars->nWords );
    Cec4_ManSimulateCis( p );
    Cec4_ManSimulate( p, pMan );
    if ( pPars->fCheckMiter && !Cec4_ManSimulateCos(p) ) // cex detected
        goto finalize;
    if ( pPars->fVerbose )
        Cec4_ManPrintStats( p, pPars, pMan, 1 );

    // perform simulation
    for ( i = 0; i < pPars->nRounds; i++ )
    {
        Cec4_ManSimulateCis( p );
        Cec4_ManSimulate( p, pMan );
        if ( pPars->fCheckMiter && !Cec4_ManSimulateCos(p) ) // cex detected
            goto finalize;
        if ( i && i % (pPars->nRounds / 5) == 0 && pPars->fVerbose )
            Cec4_ManPrintStats( p, pPars, pMan, 1 );
    }
    if ( fSimOnly )
        goto finalize;

    // perform additional simulation
    Cec4_ManCandIterStart( pMan );
    for ( i = 0; fSimulate && i < pPars->nGenIters; i++ )
    {
        Cec4_ManSimulateCis( p );
        fSimulate = Cec4_ManGeneratePatterns( pMan );
        Cec4_ManSimulate( p, pMan );
        if ( pPars->fCheckMiter && !Cec4_ManSimulateCos(p) ) // cex detected
            goto finalize;
        if ( i && i % 5 == 0 && pPars->fVerbose )
            Cec4_ManPrintStats( p, pPars, pMan, 1 );
    }
    execute_sat:
    if ( i && i % 5 && pPars->fVerbose )
        Cec4_ManPrintStats( p, pPars, pMan, 1 );

    p->iPatsPi = 0;
    Vec_WrdFill( p->vSimsPi, Vec_WrdSize(p->vSimsPi), 0 );
    pMan->nSatSat = 0;
    pMan->pNew = Cec4_ManStartNew( p );
    Gia_ManForEachAnd( p, pObj, i )
    {
        Gia_Obj_t * pObjNew; 
        pMan->nAndNodes++;
        if ( Gia_ObjIsXor(pObj) )
            pObj->Value = Gia_ManHashXorReal( pMan->pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else
            pObj->Value = Gia_ManHashAnd( pMan->pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        if ( pPars->nLevelMax && Gia_ObjLevel(p, pObj) > pPars->nLevelMax )
            continue;
        pObjNew = Gia_ManObj( pMan->pNew, Abc_Lit2Var(pObj->Value) );
        if ( Gia_ObjIsAnd(pObjNew) )
        if ( Vec_BitEntry(pMan->vFails, Gia_ObjFaninId0(pObjNew, Abc_Lit2Var(pObj->Value))) || 
             Vec_BitEntry(pMan->vFails, Gia_ObjFaninId1(pObjNew, Abc_Lit2Var(pObj->Value))) )
            Vec_BitWriteEntry( pMan->vFails, Abc_Lit2Var(pObjNew->Value), 1 );
        //if ( Gia_ObjIsAnd(pObjNew) )
        //    Gia_ObjSetAndLevel( pMan->pNew, pObjNew );
        // select representative based on candidate equivalence classes
        pRepr = Gia_ObjReprObj( p, i );
        if ( pRepr == NULL )
            continue;
        if ( 1 ) // select representative based on recent counter-examples
        {
            pRepr = Cec4_ManFindRepr( p, pMan, i );
            if ( pRepr == NULL )
                continue;
        }
        int id_obj = Gia_ObjId( p, pObj );
        int id_repr = Gia_ObjId( p, pRepr );

        if ( Abc_Lit2Var(pObj->Value) == Abc_Lit2Var(pRepr->Value) )
        {
            if ( pPars->fBMiterInfo ) 
            {
                Bnd_ManMerge( id_repr, id_obj, pObj->fPhase ^ pRepr->fPhase );
            }

            assert( (pObj->Value ^ pRepr->Value) == (pObj->fPhase ^ pRepr->fPhase) );
            Gia_ObjSetProved( p, i );
            if ( Gia_ObjId(p, pRepr) == 0 )
                pMan->iLastConst = i;
            continue;
        }
        if ( Cec4_ManSweepNode(pMan, i, Gia_ObjId(p, pRepr)) && Gia_ObjProved(p, i) )
        {
            if (pPars->fBMiterInfo){

                Bnd_ManMerge( id_repr, id_obj, pObj->fPhase ^ pRepr->fPhase );
                // printf( "proven %d merged into %d (phase : %d)\n", Gia_ObjId(p, pObj), Gia_ObjId(p,pRepr), pObj->fPhase ^ pRepr -> fPhase );

            }
            pObj->Value = Abc_LitNotCond( pRepr->Value, pObj->fPhase ^ pRepr->fPhase );


        }
    }
    
    if ( pPars->fBMiterInfo )
    {
        // print
        Bnd_ManFinalizeMappings();
        // Bnd_ManPrintMappings();
    }

    if ( p->iPatsPi > 0 )
    {
        abctime clk2 = Abc_Clock();
        Cec4_ManSimulate( p, pMan );
        p->iPatsPi = 0;
        Vec_IntFill( pMan->vCexStamps, Gia_ManObjNum(p), 0 );
        pMan->timeResimGlo += Abc_Clock() - clk2;
    }
    if ( pPars->fVerbose )
        Cec4_ManPrintStats( p, pPars, pMan, 0 );
    if ( ppNew )
    {
        Gia_ManForEachCo( p, pObj, i )
            pObj->Value = Gia_ManAppendCo( pMan->pNew, Gia_ObjFanin0Copy(pObj) );
        *ppNew = Gia_ManCleanup( pMan->pNew );
    }
finalize:
    if ( pPars->fVerbose )
        printf( "SAT calls = %d:  P = %d (0=%d a=%.2f m=%d)  D = %d (0=%d a=%.2f m=%d)  F = %d   Sim = %d  Recyc = %d  Xor = %.2f %%\n", 
            pMan->nSatUnsat + pMan->nSatSat + pMan->nSatUndec, 
            pMan->nSatUnsat, pMan->nConflicts[1][0], (float)pMan->nConflicts[1][1]/Abc_MaxInt(1, pMan->nSatUnsat-pMan->nConflicts[1][0]), pMan->nConflicts[1][2],
            pMan->nSatSat,   pMan->nConflicts[0][0], (float)pMan->nConflicts[0][1]/Abc_MaxInt(1, pMan->nSatSat  -pMan->nConflicts[0][0]), pMan->nConflicts[0][2],  
            pMan->nSatUndec,  
            pMan->nSimulates, pMan->nRecycles, 100.0*pMan->nGates[1]/Abc_MaxInt(1, pMan->nGates[0]+pMan->nGates[1]) );
    if ( pMan->vPairs && Vec_IntSize(pMan->vPairs) )
    {
        extern char * Extra_FileNameGeneric( char * FileName );
        char pFileName[1000], * pBase = Extra_FileNameGeneric(p->pName);
        extern Gia_Man_t * Gia_ManDupMiterCones( Gia_Man_t * p, Vec_Int_t * vPairs );
        Gia_Man_t * pM = Gia_ManDupMiterCones( p, pMan->vPairs );
        sprintf( pFileName, "%s_sm.aig", pBase );
        Gia_AigerWrite( pM, pFileName, 0, 0, 0 );
        Gia_ManStop( pM );
        ABC_FREE( pBase );
        printf( "Dumped miter \"%s\" with %d pairs.\n", pFileName, pMan->vPairs ? Vec_IntSize(pMan->vPairs)/2 : -1 );
    }
    if ( pPars->pDumpName )
        Cec4_ManSimulateDumpInfo( pMan );
    Cec4_ManDestroy( pMan );
    //Gia_ManStaticFanoutStop( p );
    //Gia_ManEquivPrintClasses( p, 1, 0 );
    if ( ppNew && *ppNew == NULL )
        *ppNew = Gia_ManDup(p);
    if ( p->pNexts ) Gia_ManRemoveWrongChoices( p );
    return p->pCexSeq ? 0 : 1;
}
Gia_Man_t * Cec4_ManSimulateTest( Gia_Man_t * p, Cec_ParFra_t * pPars )
{
    Gia_Man_t * pNew = NULL;
    Cec4_ManPerformSweeping( p, pPars, &pNew, 0 );

    return pNew;
}
void Cec4_ManSimulateTest2( Gia_Man_t * p, int nConfs, int fVerbose )
{
    abctime clk = Abc_Clock();
    Cec_ParFra_t ParsFra, * pPars = &ParsFra;
    Cec4_ManSetParams( pPars );
    pPars->fVerbose = fVerbose;
    pPars->nBTLimit = nConfs;
    Cec4_ManPerformSweeping( p, pPars, NULL, 0 );
    if ( fVerbose )
        Abc_PrintTime( 1, "New choice computation time", Abc_Clock() - clk );
}
Gia_Man_t * Cec4_ManSimulateTest3( Gia_Man_t * p, int nBTLimit, int fVerbose )
{
    Gia_Man_t * pNew = NULL;
    Cec_ParFra_t ParsFra, * pPars = &ParsFra;
    Cec4_ManSetParams( pPars );
    pPars->fVerbose = fVerbose;
    pPars->nBTLimit = nBTLimit;
    Cec4_ManPerformSweeping( p, pPars, &pNew, 0 );
    return pNew;
}
Gia_Man_t * Cec4_ManSimulateTest4( Gia_Man_t * p, int nBTLimit, int nBTLimitPo, int fVerbose )
{
    Gia_Man_t * pNew = NULL;
    Cec_ParFra_t ParsFra, * pPars = &ParsFra;
    Cec4_ManSetParams( pPars );
    pPars->fVerbose = fVerbose;
    pPars->nBTLimit = nBTLimit;
    pPars->nBTLimitPo = nBTLimitPo;
    Cec4_ManPerformSweeping( p, pPars, &pNew, 0 );
    return pNew;
}
int Cec4_ManSimulateOnlyTest( Gia_Man_t * p, int fVerbose )
{
    Cec_ParFra_t ParsFra, * pPars = &ParsFra;
    Cec4_ManSetParams( pPars );
    pPars->fVerbose = fVerbose;
    return Cec4_ManPerformSweeping( p, pPars, NULL, 1 );
}

/**Function*************************************************************

  Synopsis    [Internal simulation APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec4_ManSimulateTest5Int( Gia_Man_t * p, int nConfs, int fVerbose )
{
    abctime clk = Abc_Clock();
    Cec_ParFra_t ParsFra, * pPars = &ParsFra;
    Cec4_ManSetParams( pPars );
    pPars->fVerbose = fVerbose;
    pPars->nBTLimit = nConfs;
    Cec4_ManPerformSweeping( p, pPars, NULL, 0 );
    if ( fVerbose )
        Abc_PrintTime( 1, "Equivalence detection time", Abc_Clock() - clk );
}
Gia_Man_t * Gia_ManLocalRehash( Gia_Man_t * p )  
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj;
    int i;
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else if ( Gia_ObjIsCi(pObj) )
            pObj->Value = Gia_ManAppendCi( pNew );
        else if ( Gia_ObjIsCo(pObj) )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    Gia_ManHashStop( pNew );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManForEachObj1( p, pObj, i )
    {
        int iLitNew = Gia_ManObj(pTemp, Abc_Lit2Var(pObj->Value))->Value;
        if ( iLitNew == ~0 )
            pObj->Value = iLitNew;
        else
            pObj->Value = Abc_LitNotCond(iLitNew, Abc_LitIsCompl(pObj->Value));
    }
    Gia_ManStop( pTemp );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    return pNew;
}
Vec_Int_t * Cec4_ManComputeMapping( Gia_Man_t * p, Gia_Man_t * pAig, int fVerbose )
{
    Gia_Obj_t * pObj;
    Vec_Int_t * vReprs = Vec_IntStartFull( Gia_ManObjNum(p) );
    int * pAig2Abc = ABC_FALLOC( int, Gia_ManObjNum(pAig) );
    int i, nConsts = 0, nReprs = 0;
    pAig2Abc[0] = 0;
    Gia_ManForEachCand( p, pObj, i )
    {
        int iLitGia = pObj->Value, iReprGia;
        if ( iLitGia == -1 )
            continue;
        iReprGia = Gia_ObjReprSelf( pAig, Abc_Lit2Var(iLitGia) );
        if ( pAig2Abc[iReprGia] == -1 )
            pAig2Abc[iReprGia] = i;
        else
        {
            int iLitGia2 = Gia_ManObj(p, pAig2Abc[iReprGia] )->Value;
            assert( Gia_ObjReprSelf(pAig, Abc_Lit2Var(iLitGia)) == Gia_ObjReprSelf(pAig, Abc_Lit2Var(iLitGia2)) );
            assert( i > pAig2Abc[iReprGia] );
            Vec_IntWriteEntry( vReprs, i, pAig2Abc[iReprGia] );
            if ( pAig2Abc[iReprGia] == 0 )
                nConsts++;
            else
                nReprs++;
        }
    }
    ABC_FREE( pAig2Abc );
    if ( fVerbose )
        printf( "Found %d const reprs and %d other reprs.\n", nConsts, nReprs );
    return vReprs;
}
void Cec4_ManVerifyEquivs( Gia_Man_t * p, Vec_Int_t * vRes, int fVerbose )
{
    int i, iRepr, nWords = 4; word * pSim0, * pSim1;
    Vec_Wrd_t * vSimsCi = Vec_WrdStartRandom( Gia_ManCiNum(p) * nWords );
    int nObjs = Vec_WrdShiftOne( vSimsCi, nWords ), nFails = 0;
    Vec_Wrd_t * vSims   = Gia_ManSimPatSimOut( p, vSimsCi, 0 );
    assert( Vec_IntSize(vRes) == Gia_ManObjNum(p) );
    assert( nObjs == Gia_ManCiNum(p) );
    Vec_IntForEachEntry( vRes, iRepr, i )
    {
        if ( iRepr == -1 )
            continue;
        assert( i > iRepr );
        pSim0 = Vec_WrdEntryP( vSims, nWords*i );
        pSim1 = Vec_WrdEntryP( vSims, nWords*iRepr );
        if ( (pSim0[0] ^ pSim1[0]) & 1 )
            nFails += !Abc_TtOpposite(pSim0, pSim1, nWords);
        else
            nFails += !Abc_TtEqual(pSim0, pSim1, nWords);
    }
    Vec_WrdFree( vSimsCi );
    Vec_WrdFree( vSims );
    if ( nFails )
        printf( "Verification failed at %d nodes.\n", nFails );
    else if ( fVerbose )
        printf( "Verification succeeded for all (%d) nodes.\n", Gia_ManCandNum(p) );
}
void Cec4_ManConvertToLits( Gia_Man_t * p, Vec_Int_t * vRes )
{
    Gia_Obj_t * pObj; int i, iRepr;
    Gia_ManSetPhase( p );
    Gia_ManForEachObj( p, pObj, i )
        if ( (iRepr = Vec_IntEntry(vRes, i)) >= 0 )
            Vec_IntWriteEntry( vRes, i, Abc_Var2Lit(iRepr, Gia_ManObj(p, iRepr)->fPhase ^ pObj->fPhase) );
}
void Cec4_ManSimulateTest5( Gia_Man_t * p, int nConfs, int fVerbose )
{
    Vec_Int_t * vRes = NULL;
    Gia_Man_t * pAig = Gia_ManLocalRehash( p );
    Cec4_ManSimulateTest5Int( pAig, nConfs, fVerbose );
    vRes = Cec4_ManComputeMapping( p, pAig, fVerbose );
    Cec4_ManVerifyEquivs( p, vRes, fVerbose );
    Cec4_ManConvertToLits( p, vRes );
    Vec_IntDumpBin( "_temp_.equiv", vRes, fVerbose );
    Vec_IntFree( vRes );
    Gia_ManStop( pAig );
}

/**Function*************************************************************

  Synopsis    [Get ISOP LUT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void getISOPObjId( Gia_Man_t * pMan, int ObjId, char * pSop[2] , int nCubes[2] ){

    assert( Gia_ObjIsLut(pMan, ObjId));
    char * pSopInfo = pMan->vTTISOPs->pArray + Vec_IntEntry( pMan->vTTLut, ObjId );
    int k;
    // get the ISOPs positive polarity
    for(k = 0; k < 2; k++ ){
      nCubes[k] =  *pSopInfo;
      pSopInfo++;
      if(nCubes[k] == 0){
        pSop[k] = NULL;
        continue;
      }
      pSop[k] = pSopInfo;
      pSopInfo += nCubes[k] * 2;
    }
}


/**Function*************************************************************

  Synopsis    [Encode SOPs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Vec_Str_t * encodeSOP(char * pSop, int nFanins, int nCubes){

    Vec_Str_t * vSop = Vec_StrAlloc( nCubes * 2 );
    char pEncodeCube  = 0;
    char pDCs = 0;
    int fanin=0;
    while( *pSop != '\0'){
      if( *pSop == '\0')
        continue;
      if ( *pSop == '-' ){
        //pDCs |= ( 1 << (nFanins - fanin - 1) );
        pDCs |= ( 1 << fanin );
      } else if ( *pSop == '1' ){
        //pEncodeCube |= ( 1 << (nFanins - fanin - 1) );
        pEncodeCube |= ( 1 << fanin );
      }
      fanin++;
      if(fanin == nFanins){
        Vec_StrPush( vSop, pEncodeCube );
        Vec_StrPush( vSop, pDCs );
        pEncodeCube =  0;
        pDCs =  0;
        fanin = 0;
        pSop += 3;
      }
      pSop++;
    }
    return vSop;

}

/**Function*************************************************************

  Synopsis    [Extract SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

char * extractSOP( DdManager * dd, DdNode * bFunc, int nFanins, int polarity, int * _nCubes){
    
    extern int Abc_CountZddCubes( DdManager * dd, DdNode * zCover );
    extern int Abc_ConvertZddToSop( DdManager * dd, DdNode * zCover, char * pSop, int nFanins, Vec_Str_t * vCube, int fPhase );

    Vec_Str_t * vCube = Vec_StrAlloc( 100 );
    int nCubes; char * pSop;
    DdNode * bCover, * zCover;
    if( polarity == 0){
      bCover = Cudd_zddIsop( dd, Cudd_Not(bFunc), Cudd_Not(bFunc), &zCover );
    } else {
      bCover = Cudd_zddIsop( dd, bFunc, bFunc, &zCover );
    }
    Cudd_Ref( zCover );
    Cudd_Ref( bCover );
    Cudd_RecursiveDeref( dd, bCover );
    nCubes = Abc_CountZddCubes( dd, zCover );
    pSop = ABC_ALLOC( char, (nFanins + 3) * nCubes + 1 );
    pSop[(nFanins + 3) * nCubes] = 0;
    // create the SOP
    Vec_StrFill( vCube, nFanins, '-' );
    Vec_StrPush( vCube, '\0' );
    Abc_ConvertZddToSop( dd, zCover, pSop, nFanins, vCube, polarity );
    Cudd_RecursiveDerefZdd( dd, zCover );

    if ( pSop == NULL || nFanins != Abc_SopGetVarNum( pSop )){
        if ( pSop == NULL )
          printf("SOP generation failed.\n");
        else {
          ABC_FREE( pSop );
          printf( "Node has %d fanins but its SOP has support size %d.\n", nFanins, Abc_SopGetVarNum( pSop ));
        }
        fflush( stdout );
        Vec_StrFree( vCube );
        return NULL;
    }
    
    Vec_StrFree( vCube );
    *_nCubes = nCubes;
    return pSop;
}

/**Function*************************************************************

  Synopsis    [Compute ISOPs of both polarities.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void computeISOPs( Gia_Man_t * p, Abc_Ntk_t * pNtkNew ){

    Abc_Obj_t * pObjNew;
    DdManager * dd = (DdManager *)pNtkNew->pManFunc;
    DdNode * bFunc;
    char * pSop;
    int nCubes, i, jjj;
    Vec_Str_t * encodedSop;
    // compute SOP sizes
    Vec_Int_t * vGuide = Vec_IntAlloc( Abc_NtkObjNumMax(pNtkNew) );
    Vec_IntFill( vGuide, Abc_NtkObjNumMax(pNtkNew), -1 );
    // Collect BDDs in array
    Vec_Ptr_t * vFuncs = Vec_PtrStart( Abc_NtkObjNumMax(pNtkNew) );
    assert( !Cudd_ReorderingStatus(dd, (Cudd_ReorderingType *)&nCubes) );
    Abc_NtkForEachNode( pNtkNew, pObjNew, i )
        if ( !Abc_ObjIsBarBuf(pObjNew) )
            Vec_PtrWriteEntry( vFuncs, i, pObjNew->pData );
    // compute the number of cubes in the ISOPs and detemine polarity
    nCubes = Extra_bddCountCubes( dd, (DdNode **)Vec_PtrArray(vFuncs), Vec_PtrSize(vFuncs), -1, ABC_INFINITY, Vec_IntArray(vGuide) );
    Vec_PtrFree( vFuncs );
    assert( Abc_NtkHasBdd(pNtkNew) );
    if ( dd->size > 0 )
    Cudd_zddVarsFromBddVars( dd, 2 );

    // go through the objects
    Abc_NtkForEachNode( pNtkNew, pObjNew, i )
    {
        // derive ISOPs in both polarities
        if ( Abc_ObjIsBarBuf(pObjNew) )
            continue;
        assert( pObjNew->pData );
        bFunc = (DdNode *)pObjNew->pData;
        int nFanins = Abc_ObjFaninNum(pObjNew);
        assert( nFanins <= 8); // we only support 8 fanins due to the data structure of vTTISOPs
        // Check if node function is constant
        if ( Cudd_IsConstant(bFunc) ){
          pSop = ABC_ALLOC( char, nFanins + 4 );
          pSop[0] = ' ';
          pSop[1] = '0' + (int)(bFunc == Cudd_ReadOne(dd));
          pSop[2] = '\n';
          pSop[3] = '\0';
          ABC_FREE( pSop );
          // it's not a LUT, it should be skipped
          continue;
        }
        // save location of the ISOP info in vTTISOPs
        Vec_IntInsert( p->vTTLut, pObjNew->iTemp , p->vTTISOPs->nSize );
        
        // get the ZDD of the negative polarity
        pSop = extractSOP( dd, bFunc, nFanins, 0 , &nCubes);
        if ( pSop == NULL )
            goto cleanup;
        // encode the sop and save it in the vTTISOPs
        encodedSop = encodeSOP( pSop, nFanins, nCubes );
        Vec_StrPush( p->vTTISOPs, (char) nCubes );
        if (nCubes > 0){ 
          for (jjj = 0; jjj < nCubes; jjj++){
            Vec_StrPush( p->vTTISOPs, encodedSop->pArray[jjj*2] ); Vec_StrPush( p->vTTISOPs, encodedSop->pArray[jjj*2+1] );
          }
        }
        Vec_StrFree( encodedSop );
        ABC_FREE( pSop );
    
        // get the ZDD of the positive polarity
        pSop = extractSOP( dd, bFunc, nFanins, 1 , &nCubes);
        if ( pSop == NULL )
            goto cleanup;
        // encode the sop and save it in the vTTISOPs
        encodedSop = encodeSOP( pSop, nFanins, nCubes );
        Vec_StrPush( p->vTTISOPs, (char) nCubes );
        if (nCubes > 0){ 
          for (jjj = 0; jjj < nCubes; jjj++){
            Vec_StrPush( p->vTTISOPs, encodedSop->pArray[jjj*2] ); Vec_StrPush( p->vTTISOPs, encodedSop->pArray[jjj*2+1] );
          }
        }
        Vec_StrFree( encodedSop );
        ABC_FREE( pSop );
    
    }


cleanup:
    // free all used memory to generate SOPs
    Vec_IntFree( vGuide );
    Abc_NtkDelete( pNtkNew );
    return;
}


/**Function*************************************************************

  Synopsis    [Derive SOPs with positive and negative polarity.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

void Cec_DeriveSOPs( Gia_Man_t * p ){

    extern Hop_Obj_t * Abc_ObjHopFromGia( Hop_Man_t * pHopMan, Gia_Man_t * p, int GiaId, Vec_Ptr_t * vCopies );

    // generate the structure to contain LUT ids and their corresponding TTISOPS
    int nCountLuts = Gia_ManLutNum(p);
    p->vTTISOPs = Vec_StrAlloc( nCountLuts * 10);
    p->vTTLut = Vec_IntAlloc( Gia_ManObjNum(p) );
    Vec_IntFill( p->vTTLut, Gia_ManObjNum(p), -1 );

    // Transform the Gia into a HOP network
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObjNew, * pObjNewLi, * pObjNewLo, * pConst0 = NULL;
    Gia_Obj_t * pObj, * pObjLi, * pObjLo;
    Vec_Ptr_t * vReflect = Vec_PtrStart( Gia_ManObjNum(p) );
    int i, k, iFan;
    assert( Gia_ManHasMapping(p) );
    pNtkNew = Abc_NtkAlloc( ABC_NTK_LOGIC,  ABC_FUNC_AIG, 1 );
    // duplicate the name and the spec
    pNtkNew->pName = Extra_UtilStrsav(p->pName);
    pNtkNew->pSpec = Extra_UtilStrsav(p->pSpec);
    Gia_ManFillValue( p );
    // create constant
    pConst0 = Abc_NtkCreateNodeConst0( pNtkNew );
    Gia_ManConst0(p)->Value = Abc_ObjId(pConst0);
    // create PIs
    Gia_ManForEachPi( p, pObj, i )
        pObj->Value = Abc_ObjId( Abc_NtkCreatePi( pNtkNew ) );
    // create POs
    Gia_ManForEachPo( p, pObj, i )
        pObj->Value = Abc_ObjId( Abc_NtkCreatePo( pNtkNew ) );
    // create as many latches as there are registers in the manager
    Gia_ManForEachRiRo( p, pObjLi, pObjLo, i )
    {
        pObjNew = Abc_NtkCreateLatch( pNtkNew );
        pObjNewLi = Abc_NtkCreateBi( pNtkNew );
        pObjNewLo = Abc_NtkCreateBo( pNtkNew );
        Abc_ObjAddFanin( pObjNew, pObjNewLi );
        Abc_ObjAddFanin( pObjNewLo, pObjNew );
        pObjLi->Value = Abc_ObjId( pObjNewLi );
        pObjLo->Value = Abc_ObjId( pObjNewLo );
        Abc_LatchSetInit0( pObjNew );
    }
    Gia_ManForEachLut( p, i )
    {
        pObj = Gia_ManObj(p, i);
        assert( pObj->Value == ~0 );
        if ( Gia_ObjLutSize(p, i) == 0 )
        {
            pObj->Value = Abc_ObjId(pConst0);
            continue;
        }
        pObjNew = Abc_NtkCreateNode( pNtkNew );
        Gia_LutForEachFanin( p, i, iFan, k )
            Abc_ObjAddFanin( pObjNew, Abc_NtkObj(pNtkNew, Gia_ObjValue(Gia_ManObj(p, iFan))) );
        pObjNew->pData = Abc_ObjHopFromGia( (Hop_Man_t *)pNtkNew->pManFunc, p, i, vReflect );
        pObjNew->fPersist = 0;
        pObj->Value = Abc_ObjId( pObjNew );
        pObjNew->iTemp = i;
    }
    
    Vec_PtrFree( vReflect );

    // HOP -> BDD
    Abc_NtkAigToBdd(pNtkNew);

    // Determine both polarity ISOPs
    computeISOPs( p, pNtkNew );

    return;

}

/**Function*************************************************************

  Synopsis    [Evaluate MFFC depth.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int evaluate_mffc(Gia_Man_t * p, int rootId, int fanId, Vec_Int_t * vLeaves){

    int quality = 0, jMFFCLeaf;
    int nLeaves = Vec_IntSize(vLeaves);    
    for(jMFFCLeaf = 0; jMFFCLeaf < nLeaves ; jMFFCLeaf++){
      int idMFFCLeaf = Vec_IntEntry(vLeaves, jMFFCLeaf);
      int level_leaf = Gia_ObjLevelId(p, idMFFCLeaf) ;
      int level_root = Gia_ObjLevelId(p, fanId) ;
      quality +=  level_root - level_leaf;
    }
    if( quality != 0 && nLeaves > 0)
      quality /= nLeaves;
    return quality;

}

/**Function*************************************************************

  Synopsis    [Compute MFFCs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void computeMFFCs( Gia_Man_t * p ){


    int i, ith_fan, iFan;
    Vec_Int_t * vNodes, * vLeaves, * vInners;
    vNodes  = Vec_IntAlloc( 100 );
    vLeaves = Vec_IntAlloc( 10 );
    vInners = Vec_IntAlloc( 100 );
    Vec_Int_t * vNodesNew, * vLeavesNew, * vInnersNew;
    vNodesNew  = Vec_IntAlloc( 100 );
    vLeavesNew = Vec_IntAlloc( 10 );
    vInnersNew = Vec_IntAlloc( 100 );
    p->vMFFCsLuts = Vec_IntStartFull( Gia_ManObjNum(p) );
    //Vec_IntFill( p->vMFFCsLuts, Gia_ManObjNum(p), -1 );
    p->vMFFCsInfo = Vec_IntAlloc( Gia_ManLutNum(p) * 5 );
    Gia_ManCreateRefs( p );
    Gia_ManForEachLut( p, i )
    {
        Gia_Obj_t * pObj = Gia_ManObj( p, i );

        if ( !Gia_ObjRefNum(p, pObj) )
            continue;
        if ( !Gia_ObjCheckMffc(p, pObj, 10000, vNodes, vLeaves, vInners) )
            continue;

        Vec_IntInsert( p->vMFFCsLuts, i  , Vec_IntSize(p->vMFFCsInfo) );
        Gia_LutForEachFanin( p, i, iFan, ith_fan ){
          if (Vec_IntFind(vNodes, iFan) != -1){
            if(Gia_ObjIsCi(Gia_ManObj(p, iFan)) == 1){
              Vec_IntPush(p->vMFFCsInfo, Gia_ObjLevelId(p, i) ); // push the level of the CI
              continue;
            }
            Gia_Obj_t * pObjFanin = Gia_ManObj( p, iFan );
            if ( !Gia_ObjRefNum(p, pObjFanin) ){
                Vec_IntPush(p->vMFFCsInfo, 0);
                continue;
            }
            if ( !Gia_ObjCheckMffc(p, pObjFanin, 10000, vNodesNew, vLeavesNew, vInnersNew)){
                Vec_IntPush(p->vMFFCsInfo, 0);
                continue;
            }
            assert( Vec_IntSize(vLeavesNew) > 0);
            int quality = evaluate_mffc(p, i, iFan, vLeavesNew);
            Vec_IntPush(p->vMFFCsInfo, quality);

          } else {
            Vec_IntPush( p->vMFFCsInfo, 0 );
          }

        }
    }
    Vec_IntFree( vNodes );
    Vec_IntFree( vLeaves );
    Vec_IntFree( vInners );
    Vec_IntFree( vNodesNew );
    Vec_IntFree( vLeavesNew );
    Vec_IntFree( vInnersNew );

}


/**Function*************************************************************

  Synopsis    [Evaluate MFFC depth.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int extract_quality_mffc(Gia_Man_t * p, int ObjId, char pDCs){

    if( Vec_IntEntry( p->vMFFCsLuts, ObjId ) == -1)
      return 0;
    assert( Vec_IntEntry( p->vMFFCsLuts, ObjId ) != -1);
    int * pMFFC = p->vMFFCsInfo->pArray + Vec_IntEntry( p->vMFFCsLuts, ObjId );
    int iFan, ith_fan, quality = 0;
    //int nFanins = Gia_ObjLutSize(p, ObjId);
    Gia_LutForEachFanin( p, ObjId, iFan, ith_fan ){
      int qualityMFFC = *pMFFC;
      if (qualityMFFC == 0 || (pDCs & (1 << ith_fan)) > 0){ // count the cones without don't cares
        pMFFC++;
        continue;
      }
      quality +=  qualityMFFC;
      pMFFC++;
    }
    return quality;

}

/**Function*************************************************************

  Synopsis    [Generate LUTs Ranking for SimGen algo.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int * sortArray( int * array, int n ){

    int i, j, temp;
    int * positions = (int *) malloc( n * sizeof(int) );
    for(i = 0; i < n; i++)
      positions[i] = i;
    for(i = 0; i < n; i++){
      for(j = i+1; j < n; j++){
        if(array[i] < array[j]){
          temp = array[i];
          array[i] = array[j];
          array[j] = temp;
          temp = positions[i];
          positions[i] = positions[j];
          positions[j] = temp;
        }
      }
    }
    return positions;

}

void generateLutsRankings( Gia_Man_t * p){

    p->vLutsRankings = Vec_PtrStart( Gia_ManObjNum(p) );
    Vec_Int_t * vLutsRankingsTmp = Vec_IntAlloc( Gia_ManLutNum(p) * 5 ); 
    int LutId, iii, k, jjj;
    char * pSop[2]; int nCubes[2]; int nFanins;
    int * ranks, * fanins;
    Gia_ManForEachLut( p, LutId )
    {
        nFanins = Gia_ObjLutSize(p, LutId);
        Vec_PtrInsert( p->vLutsRankings, LutId, vLutsRankingsTmp->pArray + Vec_IntSize(vLutsRankingsTmp) );
        assert( nFanins > 0 );
        ranks = (int*) malloc( nFanins * sizeof(int) );
        fanins = (int*) malloc( nFanins * sizeof(int) );
        Gia_LutForEachFanin( p, LutId, k, iii ){
          fanins[iii] = k;
          ranks[iii] = 0;
        }
        
        nFanins = Gia_ObjLutSize(p, LutId);
        getISOPObjId( p, LutId, pSop, nCubes );
        for(k = 0; k < 2; k++){
          for(iii = 0; iii < nCubes[k]; iii++){
            //char pCube =  pSop[k][iii*2];
            char pDCs =  pSop[k][iii*2+1];
            for(jjj = 0; jjj < nFanins; jjj++){
              if ( (pDCs & (1 << jjj)) == 0 )
                ranks[jjj]++;
            }
          }
        }
        int * positions = sortArray( ranks, nFanins );
        for(jjj = 0; jjj < nFanins; jjj++){
          Vec_IntPush( vLutsRankingsTmp, fanins[positions[jjj]] );
        }
        free( positions );
        free( ranks );
        free( fanins );
    }
    //Vec_IntFree( vLutsRankingsTmp );
}

/**Function*************************************************************

  Synopsis    [Generate Nodes Watchlist.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

Vec_Ptr_t * generateWatchList( Gia_Man_t * p ){

    Vec_Ptr_t * vWatchList = Vec_PtrStart( Gia_ManObjNum(p) );
    int LutId;
    Gia_ManForEachLut( p, LutId )
    {
        int * ranking = (int *) Vec_PtrEntry( p->vLutsRankings, LutId );
        int nodeId = ranking[0];
        if( Vec_PtrEntry( vWatchList, nodeId ) == NULL ){
          Vec_Int_t * vWatchListTmp = Vec_IntAlloc( 5 );
          Vec_PtrWriteEntry( vWatchList, nodeId, vWatchListTmp );
          Vec_IntPush( vWatchListTmp, LutId );
          //Vec_IntFree( vWatchListTmp );
        } else {
          Vec_IntPush( (Vec_Int_t *) Vec_PtrEntry( vWatchList, nodeId ), LutId );
        }
    }
    return vWatchList;
}

/**Function*************************************************************

  Synopsis    [Fill in the fanout vectors for the LUTs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void Gia_generateFanoutMapping( Gia_Man_t * p )
{
    int iObj, iFanin, k;
    assert( Gia_ManHasMapping(p) );
    Vec_WecFreeP( &p->vFanouts2 );
    p->vFanouts2 = Vec_WecStart( Gia_ManObjNum(p) );
    Gia_ManForEachLut( p, iObj )
    {
        Gia_LutForEachFanin( p, iObj, iFanin, k )
        {
            Vec_WecPush( p->vFanouts2, iFanin, iObj );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Remove all non-LUT nodes from equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

void Cec_RemoveNonLutNodes( Gia_Man_t * p){

    int i, iRepr, kElement, firstLutId, prevLutId;
    Vec_Int_t * vLutIds = Vec_IntAlloc( Gia_ManObjNum(p) );
    Gia_ManForEachClass0( p, iRepr )
    {
      Gia_ClassForEachObj1( p, iRepr, kElement ){
        Vec_IntPush( vLutIds, kElement );
      }

      firstLutId = -1; prevLutId = -1;
      if( !Gia_ObjIsLut(p, iRepr) ){
          Gia_ObjSetRepr( p, iRepr, GIA_VOID );
          Gia_ObjSetNext( p, iRepr, 0 );
      } else {
          firstLutId = iRepr;
          prevLutId = iRepr;
      }
      Vec_IntForEachEntry( vLutIds, kElement, i ){

        if( !Gia_ObjIsLut(p, kElement) ){
            Gia_ObjSetRepr( p, kElement, GIA_VOID );
            Gia_ObjSetNext( p, kElement, 0 );
            continue;
        }
        if(firstLutId == -1){
            Gia_ObjSetRepr( p, kElement, GIA_VOID );
            firstLutId = kElement;
            prevLutId = kElement;
        } else {
            Gia_ObjSetRepr( p, kElement, firstLutId );
            Gia_ObjSetNext( p, prevLutId , kElement );
            prevLutId = kElement;
        }
      }
      Vec_IntClear( vLutIds );
      // no lut found
      if(firstLutId == -1){
        continue;
      }
      Gia_ObjSetNext( p, prevLutId, 0 );

    }
    Vec_IntFree( vLutIds );
}

/**Function*************************************************************

  Synopsis    [Evaluate equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

int evaluate_equiv_classes(Gia_Man_t * p, int verbose){

    int i, k;
    int quality = 0;
    Gia_ManForEachClass0( p, i )
    {
        Gia_ClassForEachObj1( p, i, k ){
          quality++;
        }
    }

    if (verbose)
      printf("**Quality = %d\n", quality);

    return quality;
}


/**Function*************************************************************

  Synopsis    [Compute total number of classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int totalNumClasses(Gia_Man_t * p){
  int iRepr;
  int numClasses = 0;
  Gia_ManForEachClass0( p, iRepr ){
    numClasses++;
  }
  return numClasses;
}

/**Function*************************************************************

  Synopsis    [Save PI simulation values.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

void saveSimVectors( Gia_Man_t * p, Vec_Ptr_t * pValues, int bitLength, int jth_word, int kth_bit){

    int i, Id;
    int nWords = bitLength / 64;
    assert(nWords == 0); // Not considering case with large outgold values
    Gia_ManForEachCiId( p, Id, i ){
        Vec_Wrd_t * pValuesPi = (Vec_Wrd_t *) Vec_PtrEntry( pValues, i );
        word pValuePiJWord = 0;
        if(kth_bit == 0){
          Vec_WrdPush( pValuesPi, 0 );
        } else {
          pValuePiJWord = Vec_WrdEntry( pValuesPi, jth_word );
        }
        word * pSim = Cec4_ObjSim( p, Id );
        pValuePiJWord = (pSim[0] << kth_bit) ^ pValuePiJWord;
        Vec_WrdWriteEntry( pValuesPi, jth_word, pValuePiJWord );
    }
}

/**Function*************************************************************

  Synopsis    [Simulate the CIs with random values.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

void executeRandomSim( Gia_Man_t * p, Cec4_Man_t * pMan , int dynSim, int nMaxIter , Vec_Ptr_t * vSimSave, int verbose ){

  clock_t start_time = clock();
  int i, j, k, Id;
  int numClass = 100000;
  int oldNumClass = 0, iMaxNumber = 200, iNumber = 0;
  if(dynSim){
    while (numClass != oldNumClass && iNumber < iMaxNumber)
    {
      oldNumClass = numClass;
      Cec4_ManSimulateCis( p );
      Cec4_ManSimulate( p, pMan );
      int quality = evaluate_equiv_classes(p, 0);
      if (verbose)
        printf("Time elapsed: %f (classes size: %d)\n", (float)(clock() - start_time)/CLOCKS_PER_SEC, quality);
      numClass = totalNumClasses(p);
      iNumber++;
      Gia_ManForEachCiId( p, Id, j ){
        Vec_Wrd_t * vSimPI = (Vec_Wrd_t *) Vec_PtrEntry( vSimSave, j );
        word * pSim = Cec4_ObjSim( p, Id );
        for( k = 0; k < p->nSimWords; k++ ){
          Vec_WrdPush( vSimPI, pSim[k] );
        }
      }
    }
    

  } else {
    for(i = 0; i < nMaxIter; i++){
      Cec4_ManSimulateCis( p );
      Cec4_ManSimulate( p, pMan );
      int quality = evaluate_equiv_classes(p, 0);
      Cec_RemoveNonLutNodes( p );
      if (verbose)
        printf("Time elapsed: %f (classes size: %d)\n", (float)(clock() - start_time)/CLOCKS_PER_SEC, quality);
      Gia_ManForEachCiId( p, Id, j ){
        Vec_Wrd_t * vSimPI = (Vec_Wrd_t *) Vec_PtrEntry( vSimSave, j );
        word * pSim = Cec4_ObjSim( p, Id );
        for( k = 0; k < p->nSimWords; k++ ){
          Vec_WrdPush( vSimPI, pSim[k] );
        }
      }
    }
  }
}


/**Function*************************************************************

  Synopsis    [Export Equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void exportEquivClasses(Gia_Man_t * p, char * filename){

    FILE * pFile;
    pFile = fopen( filename, "wb" );
    int i, j, iii = 0;
    Gia_ManForEachClass0( p, i )
    {
        fprintf( pFile, "Class %d: %d ", iii , i);
        Gia_ClassForEachObj1( p, i, j ){
          fprintf( pFile, " %d ", j );
        }
        fprintf(pFile, "\n");
        iii++;
    }
    fclose( pFile );


}



/**Function*************************************************************

  Synopsis    [Generate the output golden values.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

char * generateOutGoldValues(Gia_Man_t * p){

    char * pOutGold = (char *) malloc( sizeof(char) * Gia_ManObjNum(p) );
    int i, k;
    int cnt = 85; // 0b01010101
    Gia_ManForEachLut( p, i ){
        pOutGold[i] = (char) 0;
    }
    Gia_ManForEachClass0( p, i ){
        pOutGold[i] = (char) cnt;
        cnt ^= 1;
        Gia_ClassForEachObj1( p, i, k ){
          pOutGold[k] = (char) cnt;
          cnt ^= 1;
        }
    }
    return pOutGold;

}

/**Function*************************************************************

  Synopsis    [Check if it exists at least one class.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

int existsOneClass(Gia_Man_t * p){
  int iRepr;
  Gia_ManForEachClass0( p, iRepr ){
    return 1;
  }
  return 0;
}

/**Function*************************************************************

  Synopsis    [Extract nth equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

Vec_Int_t * extractNthClass(Gia_Man_t * p, int nth_class){

  Vec_Int_t * vClass = Vec_IntAlloc( Gia_ManLutNum(p) );
  int iRepr, jLut;
  Gia_ManForEachClass0( p, iRepr ){
    if(nth_class == 0){
      Vec_IntPush(vClass, iRepr);
      Gia_ClassForEachObj1( p, iRepr, jLut )
        Vec_IntPush(vClass, jLut);
      break;
    }
    nth_class--;
  }
  assert(Vec_IntSize(vClass) > 0);
  return vClass;

}

/**Function*************************************************************

  Synopsis    [Compute the LUTs order.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

Vec_Int_t * computeLutsOrder(Gia_Man_t * p, int reorder_type){

    Vec_Int_t * luts_order = Vec_IntAlloc( Gia_ManLutNum(p) );
    int * luts_tmp = (int *) malloc( sizeof(int) * Gia_ManLutNum(p));
    int iRepr, jLut, iii, jjj, kkk;
    Gia_ManForEachClass0( p, iRepr ){
        luts_tmp[ 0 ] = iRepr;
        iii = 1;
        Gia_ClassForEachObj1( p, iRepr, jLut ){
          if(reorder_type == 0){ // decreasing level order
            int found = 0;
            for(jjj = 0; jjj < iii; jjj++){
              if(Gia_ObjLevelId(p, jLut) <= Gia_ObjLevelId(p, luts_tmp[jjj])){
                for (kkk = iii; kkk > jjj; kkk--) {
                  luts_tmp[kkk] = luts_tmp[kkk - 1];
                }
                luts_tmp[jjj] = jLut;
                found = 1;
                break;
              }
            }
            if(!found){
              luts_tmp[iii] = jLut;
            }
            iii++;
          } else if(reorder_type == 1) { // increasing level order
            int found = 0;
            for(jjj = 0; jjj < iii; jjj++){
              if(Gia_ObjLevelId(p, jLut) >= Gia_ObjLevelId(p, luts_tmp[jjj])){
                for (kkk = iii; kkk > jjj; kkk--) {
                  luts_tmp[kkk] = luts_tmp[kkk - 1];
                }
                luts_tmp[jjj] = jLut;
                found = 1;
                break;
              }
            }
            if(!found){
              luts_tmp[iii] = jLut;
            }
            iii++;
          }
        }
        for(kkk = 0; kkk < iii; kkk++)
          Vec_IntPush(luts_order, luts_tmp[kkk] );
    }
    free(luts_tmp);
    return luts_order;

}


/**Function*************************************************************

  Synopsis    [Compute the fanin cones.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

void computeFaninCones_rec(Gia_Man_t * p, int ObjId, Vec_Int_t * vLutsFaninCones ){

    int FaninId, jth_fanin;
    Gia_LutForEachFanin( p, ObjId, FaninId, jth_fanin ){
      if( Vec_IntEntry( vLutsFaninCones, FaninId ) == 0 ){
        Vec_IntWriteEntry( vLutsFaninCones, FaninId, 1 );
        if( Gia_ObjIsCi( Gia_ManObj(p, FaninId) ) == 0 )
          computeFaninCones_rec( p, FaninId, vLutsFaninCones );
      }
    }
}

Vec_Int_t * computeFaninCones( Gia_Man_t * p, Vec_Int_t * vLuts ){

    int i, jth_fanin;
    Vec_Int_t * vLutsFaninCones = Vec_IntStart(Gia_ManObjNum(p));
    Vec_IntForEachEntry( vLuts, i, jth_fanin ){
      computeFaninCones_rec( p, i, vLutsFaninCones );
    }
    return vLutsFaninCones;

}


/**Function*************************************************************

  Synopsis    [Check compatibility ISOP and Vec.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int checkCompatibilityCube( Gia_Man_t * pMan, char * pCube, int nFanins, char * pCubeGold ){

    char pCubeOnes = *pCube; char pDCs = *(pCube+1);
    char pCubeGoldOnes = *pCubeGold; char pCubeNotAssigned = *(pCubeGold+1);

    char pSkipBits = pDCs | pCubeNotAssigned;
    char pConflictBits = pCubeOnes ^ pCubeGoldOnes;
    char isConflict = ~pSkipBits & pConflictBits;
    if ( isConflict )
      return 0;

    return 1;

}


/**Function*************************************************************

  Synopsis    [Compute quality of SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

int compute_quality_sop(Gia_Man_t * p , char * pSop, int ObjId ,int nFanins, int experimentID){

    int quality = 0;
    //char pValues = *pSop;
    pSop++;
    char pDCs = *(pSop);
    switch (experimentID){
    case 1: // Reverse Simulation
    case 2: // Simple Implication
    case 3: // Advanced Implication
      break;
    case 4: // Advanced Implication + Count DCs
      while (pDCs > 0){
        if((pDCs & 1) == 1)
          quality += p->nLevels * 5; // the presence of DCs is more important than mffc 
        pDCs = pDCs >> 1;
      }      
      break;
    case 5: // Advanced Implication + Count DC + FFC
      while (pDCs > 0){
        if((pDCs & 1) == 1)
          quality += p->nLevels * 5; // the presence of DCs is more important than mffc
        pDCs = pDCs >> 1;
      }   
      pDCs = *(pSop);   
      quality += extract_quality_mffc(p, ObjId, pDCs);
      break;
    default:
      break;
    }
    return quality;
}


/**Function*************************************************************

  Synopsis    [Execute roulette wheel.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

int rouletteWheel( Vec_Int_t * vQualitySops, int numValid){

    int i, max_int = 0xffffffff;
    float totalSum;
		totalSum = 0.0;
		for ( i = 0; i < numValid; i++ )
			totalSum += (float) Vec_IntEntry(vQualitySops, i);

    unsigned int randValue = Gia_ManRandom(0);
    float randValueNormalized = (float) ((float)randValue / max_int);
		float randomNum =  randValueNormalized * totalSum;

		// Select the index based on inverse proportional probability
    float cumulativeProbability = 0.0;
		for ( i = 0; i < numValid; i++ ) {
      cumulativeProbability += Vec_IntEntry(vQualitySops, i);
      if (cumulativeProbability > randomNum) {
        return i;
      }
		}
    return -1;
}

/**Function*************************************************************

  Synopsis    [Select SOP depending on quality value and the experimentID.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

int selectSop( Vec_Int_t * vQualitySops, int ith_max_quality, int experimentID){

    assert(experimentID > 0); // random simulation should not be used here
    int numValid = Vec_IntSize(vQualitySops);
    int idSelected = -1;
    switch (experimentID){
    //case 1:
    //  idSelected = Aig_ManRandom(0) % numValid;
    //  break;    
    default:
      idSelected = rouletteWheel( vQualitySops, numValid);
      break;
    }
    if(idSelected == -1){
      idSelected = ith_max_quality;
    }
    assert( idSelected != -1);
    return idSelected;
}

/**Function*************************************************************

  Synopsis    [Check implication.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

int check_implication( Gia_Man_t * p, int ObjId , int validBit , int fanoutValue, int fanoutNotSet, char * inputsFaninsValues, char * networkValues1s, char * networkValuesNotSet , int experimentID){

    assert(experimentID > 1);
    int ith_cube, jjj;
    int nFanins = Gia_ObjLutSize(p, ObjId);
    char * pSopBoth[2]; int nCubesBoth[2];
    getISOPObjId( p, ObjId, pSopBoth, nCubesBoth );

    if (fanoutNotSet == 0){
      // case in which the output is set // backward implication
      char * pSop = pSopBoth[fanoutValue];
      int nCubes = nCubesBoth[fanoutValue];
      int compatible = 0;
      int lastCube = 0;
      for(ith_cube = 0; ith_cube < nCubes; ith_cube++){
        if(checkCompatibilityCube( p, pSop + ith_cube * 2, nFanins, inputsFaninsValues ) ){
          compatible++;
          lastCube = ith_cube;
        }
      }
      

      if(compatible == 1){
        //char selectedSop = *(pSop + lastCube * 2);
        //char selectedDCs = *(pSop + lastCube * 2 + 1);
        assert(0); // TODO: implement the case in which the output is set
        return 1;
      } else if (compatible == 0){
        return -1;
      }

      return 0;
    } 
    // forward implication
    int compatible[2];
    compatible[0] = 0;
    compatible[1] = 0;
    for(jjj = 0; jjj < 2; jjj++){

      char * pSop = pSopBoth[jjj];
      int nCubes = nCubesBoth[jjj];
      for(ith_cube = 0; ith_cube < nCubes; ith_cube++){
        if(checkCompatibilityCube( p, pSop + ith_cube * 2, nFanins, inputsFaninsValues ) ){
          compatible[jjj]++;
          if(compatible[jjj] > 1)
            break;
        }
      }
    }

    if(compatible[0] > 0 && compatible[1] > 0){
      return 0;
    } else if (compatible[0] == 0 && compatible[1] == 0){
      return -1;
    }

    if(experimentID == 2){
      // simple implication
      if(compatible[0] > 1 || compatible[1] > 1){
        return 0;
      }
      char mask = 1 << validBit;
      if(compatible[0] == 1){
        networkValuesNotSet[ObjId] &= (~mask);
      } else {
        networkValues1s[ObjId] |= (1 << validBit);
        networkValuesNotSet[ObjId] &= (~mask);
      }     
      return 1;
    } else if (experimentID > 2){
      // advanced implication
      char mask = 1 << validBit;
      if(compatible[0] >= 1){
        networkValuesNotSet[ObjId] &= (~mask);
      } else {
        networkValues1s[ObjId] |= (1 << validBit);
        networkValuesNotSet[ObjId] &= (~mask);
      }
      return 1;
    }

    assert(0);
    // In C++ assert(0) doesn't count as a proper exit.
    // needs to return a value to compile.
    return -1;
}


/**Function*************************************************************

  Synopsis    [Check compatibility of cubes for implications.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int checkCompatibilityImplication( Gia_Man_t * p, Cec_ParSimGen_t * pPars, char * netValid, int objId, char * networkValues1s, char * networkValuesNotSet , Vec_Int_t * vLuts2Imply, Vec_Int_t * vLutsValidity  , int experimentID){


    char _valid_vecs = *netValid;
    char inputsFaninsValues[2] = {0, 0};
    int firstElement = 0;
    int iii = -1, FaninId, jjj;
    pPars->nImplicationTotalChecks++;
    while (_valid_vecs > 0){
      iii++;
      if( (_valid_vecs & 1) == 0){
        _valid_vecs = _valid_vecs >> 1;
        continue;
      }
      // retrieve value of the fanout
      char mask = 1 << iii;
      int fanoutValue = (networkValues1s[objId] & mask) >> iii;
      int fanoutNotSet = (networkValuesNotSet[objId] & mask) >> iii;

      if(fanoutNotSet == 0){
        // disable backward implication in this function
        _valid_vecs = _valid_vecs >> 1;
        continue;
      }
      // re-arrange fanin values format
      Gia_LutForEachFanin( p, objId, FaninId, jjj ){
        char valueFanin = networkValues1s[FaninId];
        char valueFaninNotSet = networkValuesNotSet[FaninId];
        inputsFaninsValues[0] |= ( ((valueFanin & mask) >> iii) << jjj);
        inputsFaninsValues[1] |= ( ((valueFaninNotSet & mask) >> iii) << jjj);
      }        


      if(fanoutNotSet == 0 && inputsFaninsValues[1] == 0){
        _valid_vecs = _valid_vecs >> 1;
        continue;
      }

      int status = check_implication( p, objId, iii, fanoutValue, fanoutNotSet, inputsFaninsValues, networkValues1s, networkValuesNotSet , experimentID);
      if(status == -1){
        *netValid = *netValid & ~(1 << iii);
      } else if( status == 1){
        if(firstElement == 0){
          pPars->nImplicationSuccessChecks++;
          Vec_IntPush( vLutsValidity, 1 << iii );
          Vec_IntPush( vLuts2Imply, objId );
        } else {
          Vec_IntWriteEntry( vLutsValidity, Vec_IntSize(vLutsValidity) - 1, Vec_IntEntry(vLutsValidity, Vec_IntSize(vLutsValidity) - 1) | (1 << iii) );
        }
      }

      _valid_vecs = _valid_vecs >> 1;
    }

    if( *netValid == 0){
      return 0;
    }

    return 1;
}

/**Function*************************************************************

  Synopsis    [Compute list of luts to imply.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int computeLutsToImply( Gia_Man_t * p, Cec_ParSimGen_t * pPars, char * netValid, int ObjId, char * networkValues1s, char * networkValuesNotSet , Vec_Int_t * vLutsFaninCones, Vec_Int_t * vLuts2Imply, Vec_Int_t * vLutsValidity , Vec_Ptr_t * vNodesWatchlist, int experimentID){

    int i, FanoutId, jth_fanout, LutId, k;
    
    if(vNodesWatchlist == NULL){
      // Option 1: iterate through fanouts to check if there can be an implication and if yes, apply it
      Gia_LutForEachFanout2( p, ObjId, FanoutId, jth_fanout ){
        if( Vec_IntEntry(vLutsFaninCones, FanoutId) == 0){
          // if the lut FanoutId is not in any cone, it shouldn't be considered
          continue;
        }
        int status = checkCompatibilityImplication( p, pPars, netValid, FanoutId, networkValues1s, networkValuesNotSet, vLuts2Imply, vLutsValidity, experimentID);
        if(status <= 0){
          return status;
        }
      }
    } else {
      // Option 2: iterate through the watchlist to check if there can be an implication and if yes, apply it
      Vec_Int_t * vWatchlist = (Vec_Int_t *) Vec_PtrEntry( vNodesWatchlist, ObjId );
      if(vWatchlist == NULL){
        return 1;
      }
      Vec_IntForEachEntry( vWatchlist, FanoutId, i ){
        if( Vec_IntEntry(vLutsFaninCones, FanoutId) == 0){
          // if the lut FanoutId is not in any cone, it shouldn't be considered
          continue;
        }
        int status = checkCompatibilityImplication( p, pPars, netValid, FanoutId, networkValues1s, networkValuesNotSet, vLuts2Imply, vLutsValidity, experimentID);
        if(status <= 0){
          return status;
        }
      }
      // update watchlist of LUTs
      Vec_IntForEachEntry( vWatchlist, LutId, i ){
        int * ranking = (int *) Vec_PtrEntry( p->vLutsRankings, LutId );
        int FaninSize = Gia_ObjLutSize( p, LutId ), newWatchK = -1;
        for(k = 0; k < FaninSize ; k++){
          if(ranking[k] == ObjId){
            newWatchK = k+1;
            break;
          }
        }
        assert(newWatchK != -1);
        if(newWatchK >= FaninSize){ // the LUT is already fully watched
          continue;
        }
        int newWatch = ranking[newWatchK];
        Vec_Int_t * vNewWatch = (Vec_Int_t *) Vec_PtrEntry( vNodesWatchlist, newWatch );
        if(vNewWatch == NULL){ // case in which there was no watch for a certain variable
          vNewWatch = Vec_IntAlloc( 10 );
          Vec_PtrWriteEntry( vNodesWatchlist, newWatch, vNewWatch );
          Vec_IntPush( vNewWatch, LutId );
        } else {
          if(Vec_IntFind(vNewWatch, LutId) == -1){
            Vec_IntPush( vNewWatch, LutId );
          }
        }
      }

    }
    return 1;

}

/**Function*************************************************************

  Synopsis    [Execute implications.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

int executeImplications( Gia_Man_t * p, Cec_ParSimGen_t * pPars, char * netValid, int ObjId, char * networkValues1s, char * networkValuesNotSet , Vec_Int_t * vLutsFaninCones, Vec_Ptr_t * vNodesWatchlist , int experimentID){

    assert( *netValid > 0);
    pPars->nImplicationExecution++;
    int iii = -1, FanoutId;
    Vec_Int_t * vLuts2Imply = Vec_IntAlloc( 10 );
    Vec_Int_t * vLutsValidity = Vec_IntAlloc( 10 ); 

    // first compute the list of luts to imply
    int status = computeLutsToImply( p, pPars, netValid, ObjId, networkValues1s, networkValuesNotSet, vLutsFaninCones, vLuts2Imply, vLutsValidity, vNodesWatchlist , experimentID);
    if(status <= 0){
      Vec_IntFree( vLuts2Imply );
      Vec_IntFree( vLutsValidity );
      return status;
    } 

    // second recursively go through the implied luts if there is any implication that can be applied
    Vec_IntForEachEntry( vLuts2Imply, FanoutId, iii ){
      pPars->nImplicationSuccess++;
      char validFanoutId = Vec_IntEntry(vLutsValidity, iii);
      status = executeImplications( p, pPars, &validFanoutId, FanoutId, networkValues1s, networkValuesNotSet, vLutsFaninCones, vNodesWatchlist ,experimentID);
      if (status == -1){
        Vec_IntFree( vLuts2Imply );
        Vec_IntFree( vLutsValidity );
        return -1;
      }
    }


    Vec_IntFree( vLuts2Imply );
    Vec_IntFree( vLutsValidity );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Compute network values.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

int computeNetworkValues(Gia_Man_t * p, Cec_ParSimGen_t * pPars, int ObjId ,  char * netValid, char lutValues1s, char * networkValues1s, char * networkValuesNotSet , Vec_Int_t * modifiedLuts , Vec_Int_t * vLutsFaninCones , Vec_Ptr_t * vNodesWatchlist , int experimentID ){

    assert(experimentID > 0);
    assert(  *netValid > 0 );

    if( Gia_ObjIsCi( Gia_ManObj(p, ObjId) ) ){
      printf("[ERROR] It is not possible to call computeNetworkValues for PIs");
      return -1;
    }
    char * pSopBoth[2]; int nCubesBoth[2];
    char * pSop; int nCube, ith_cube, jth_fanin, FaninId;
    getISOPObjId( p, ObjId, pSopBoth, nCubesBoth );
    Vec_Ptr_t * vSops = Vec_PtrAlloc( 8 ); // for now consider only 8 parallalel SOPs due to char size
    Vec_Int_t * vQualitySops = Vec_IntAlloc( 8 ); // compute qualities of SOPs
    Vec_Int_t * vCubeId = Vec_IntAlloc( 8 ); // save ids of the selected cubes
    char _valid_vecs = *netValid;
    char _desired_values = lutValues1s;
    int iii = -1, max_quality, ith_max_quality, validNum =0;
    int nFanins = Gia_ObjLutSize(p, ObjId);
    // first compute all SOPs that are available 
    while (_valid_vecs > 0){
      iii++;
      if( (_valid_vecs & 1) == 0){
        _valid_vecs = _valid_vecs >> 1;
        _desired_values = _desired_values >> 1;
        continue;
      }
      validNum++;
      if( (_desired_values & 1) == 0){
        // negative polarity
        pSop = pSopBoth[0];
        nCube = nCubesBoth[0];
      } else {
        // positive polarity
        pSop = pSopBoth[1];
        nCube = nCubesBoth[1];
      }
      Vec_IntClear(vQualitySops); // reset the quality of the SOPs
      Vec_IntClear(vCubeId); // reset the ids of the cubes
      char valuesFaninsMasked[2] = {0, 0}; // re-arrange all the fanin values for a specific iteration
      Gia_LutForEachFanin( p, ObjId, FaninId, jth_fanin ){
        char valuesFanin = networkValues1s[FaninId];
        char valueFaninNotSet = networkValuesNotSet[FaninId];
        char mask = 1 << iii;
        valuesFaninsMasked[0] |= ( ((valuesFanin & mask) >> iii) << jth_fanin);
        valuesFaninsMasked[1] |= ( ((valueFaninNotSet & mask) >> iii) << jth_fanin);
      }
      max_quality = 0;
      ith_max_quality = -1;
      for(ith_cube = 0; ith_cube < nCube; ith_cube++){
        if(checkCompatibilityCube( p, pSop + ith_cube * 2, nFanins, valuesFaninsMasked ) ){
          int quality = 1;
          quality += compute_quality_sop(p, pSop + ith_cube * 2, ObjId , nFanins, experimentID);
          if(quality > max_quality){
            max_quality = quality;
            ith_max_quality = ith_cube;
          }
          Vec_IntPush(vQualitySops, quality);
          Vec_IntPush(vCubeId, ith_cube);
        }
      }
      if(ith_max_quality == -1){ // no cube found that respects the values of the fanins
        // the vector is not valid
        validNum--;
        *netValid = *netValid & ~(1 << iii);
      } else {
        int _idCube = selectSop(vQualitySops, ith_max_quality, experimentID);
        int idSop = Vec_IntEntry(vCubeId, _idCube);
        Vec_PtrPush( vSops, pSop + idSop * 2 );
      }
      _valid_vecs = _valid_vecs >> 1;
      _desired_values = _desired_values >> 1;
    }
    Vec_IntFree( vQualitySops );
    Vec_IntFree( vCubeId );
    assert( validNum == Vec_PtrSize(vSops) );
    _valid_vecs = *netValid;
    // if not SOP available is valid return 0 ( not -1 since the valid vector might have been modified to skip DCs // there might be some valid SOPs for the next fanin)
    if( _valid_vecs == 0){
      //printInfoLutValues( p, ObjId, netValid, lutValues1s, networkValues1s, networkValuesNotSet);
      //printf("[WARNING] No valid SOPs found for node %d\n", ObjId);
      Vec_PtrFree( vSops );
      return 0;
    }
    Vec_IntPush( modifiedLuts, ObjId );
    // second traverse the graph applying the selected SOPs
    Gia_LutForEachFanin( p, ObjId, FaninId, jth_fanin ){
      // re-arrange SOPs as luts values across multiple iterations for each fanin
      iii = -1;
      validNum = -1;
      _valid_vecs = *netValid;
      char valuesFanin1s = 0;
      char valuesFaninDCs = 0;
      while (_valid_vecs > 0){
        iii++;
        if( (_valid_vecs & 1) == 0){
          _valid_vecs = _valid_vecs >> 1;
          continue;
        }
        validNum++;
        char mask_fanin = 1 << jth_fanin;

        pSop = (char *) Vec_PtrEntry(vSops, validNum);
        char pSop1s = *pSop;
        pSop++;
        char pSopDCs = *pSop;
        valuesFanin1s |= ( (pSop1s & mask_fanin) >> jth_fanin) << iii;
        valuesFaninDCs |= ( (pSopDCs & mask_fanin) >> jth_fanin) << iii;
        _valid_vecs = _valid_vecs >> 1;
      }

      // compute the values of the fanin
      char prevNetValue = networkValues1s[FaninId];
      char prevNetValueNotSet = networkValuesNotSet[FaninId];
      char values2propagate = ~valuesFaninDCs & *netValid &  prevNetValueNotSet;
      networkValues1s[FaninId] |= valuesFanin1s & values2propagate;
      networkValuesNotSet[FaninId] &= valuesFaninDCs & *netValid;   

      if(values2propagate == 0){
        continue; // no value 2 propagate
      }

      // apply implications
      char tmp_values2propagate = values2propagate;
      int status;
      char difference_valid;
      if(experimentID > 1){
        clock_t start_time = clock();
        status = executeImplications(p, pPars, &values2propagate, FaninId, networkValues1s, networkValuesNotSet , vLutsFaninCones, vNodesWatchlist , experimentID);
        pPars->fImplicationTime += (float)(clock() - start_time)/CLOCKS_PER_SEC;
        if( status == -1){
          networkValues1s[FaninId] = prevNetValue;
          networkValuesNotSet[FaninId] = prevNetValueNotSet;
          Vec_PtrFree( vSops );
          return -1;
        }
        assert( tmp_values2propagate >= values2propagate );
        // remove the values that are not valid
        difference_valid = tmp_values2propagate ^ values2propagate;
        if( difference_valid ){
          // some values are not valid 
          networkValuesNotSet[FaninId] &= ~difference_valid;
          *netValid &= ~difference_valid;
          // update also the validity signal and corresponding values
          values2propagate = ~valuesFaninDCs & *netValid &  prevNetValueNotSet;
          networkValues1s[FaninId] |= valuesFanin1s & values2propagate;
          networkValuesNotSet[FaninId] &= valuesFaninDCs & *netValid;  
          tmp_values2propagate = values2propagate;
        }
        values2propagate = tmp_values2propagate;
        if(values2propagate == 0){
          continue; // no value 2 propagate
        }
      }

      if( !Gia_ObjIsCi( Gia_ManObj(p, FaninId) ) ){   
        //printf("***********************\n");
        //printInfoLutValues( p, ObjId, netValid, lutValues1s, networkValues1s, networkValuesNotSet);
        status = computeNetworkValues(p, pPars, FaninId, &values2propagate, valuesFanin1s, networkValues1s, networkValuesNotSet, modifiedLuts, vLutsFaninCones, vNodesWatchlist , experimentID);
        if(status == -1){
          networkValues1s[FaninId] = prevNetValue;
          networkValuesNotSet[FaninId] = prevNetValueNotSet;
          Vec_PtrFree( vSops );
          return -1;
        }
        assert( tmp_values2propagate >= values2propagate );
        difference_valid = tmp_values2propagate ^ values2propagate;
        if( difference_valid ){
          // some values are not valid 
          networkValuesNotSet[FaninId] &= ~difference_valid;
          *netValid &= ~difference_valid;
        }
      }
    }

    Vec_PtrFree( vSops );

    return 1;
}


/**Function*************************************************************

  Synopsis    [Set values of primary inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

void saveInputVectors( Gia_Man_t * p, Cec4_Man_t * pMan, char * pValues){

    int i, Id, w;
    Gia_ManForEachCiId( p, Id, i ){
        word * pSim = Cec4_ObjSim( p, Id );
        for ( w = 0; w < p->nSimWords; w++ )
            pSim[w] = (word)pValues[ Id ];
        pSim[0] <<= 1;
    }
}

/**Function*************************************************************

  Synopsis    [Export Simulation values.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void exportSimValues( Gia_Man_t * p, char * filename )
{
    FILE * pFile;
    pFile = fopen( filename, "wb" );
    Gia_Obj_t * pObj; int i, j;
    Gia_ManForEachObj( p, pObj, i )
    {
        word * pSim  = Cec4_ObjSim( p, i );
        fprintf( pFile, "Obj %d ", i );
        for(j = 0; j < p->nSimWords; j++){
          fprintf( pFile, "[%d]: %lu ", j, pSim[j] );
        }
        fprintf(pFile, "\n");
    }
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Compute Input Vectors using SimGen.]

  Description []
               
  SideEffects []

  SeeAlso     []

************************************************************************/

int computeInputVectors(Gia_Man_t * p, Cec4_Man_t * pMan, Cec_ParSimGen_t * pPars , Vec_Int_t * vLuts , char * outGold, int outGold_bitwidth , Vec_Int_t * vLutsFaninCones , Vec_Ptr_t * vNodesWatchlist , Vec_Ptr_t * vSimSave ,int experimentID){

    char * networkValues1s, * networkValuesNotSet; // data structure containing the values inside the network
    char networkValid = (char) (1 << outGold_bitwidth) - 1; // keeps track which array out of the 8 is valid
    int jLut, mainLoop_condition, iii, numLuts;
    
    numLuts = Vec_IntSize(vLuts);
    assert(numLuts > 0);

    networkValues1s = (char *) malloc( Gia_ManObjNum(p));
    memset(networkValues1s, 0, sizeof(char) * Gia_ManObjNum(p));
    networkValuesNotSet = (char *) malloc( Gia_ManObjNum(p));
    memset(networkValuesNotSet, 0xFF, sizeof(char) * Gia_ManObjNum(p));
    mainLoop_condition = numLuts > 0;
    iii = 0;
    int success = 0, lastjGold = -1;
    while ( mainLoop_condition ){
      jLut = Vec_IntEntry(vLuts, iii);
      char jLutGold = outGold[jLut];

      // TODO: Dynamic outgold

      // check if the value of the lut has been set already or not 
      char lutValueNotSet = networkValuesNotSet[jLut];
      char lutValues1s = networkValues1s[jLut];
      if( lutValueNotSet & networkValid){

        char netValid = networkValid & lutValueNotSet;
        char * networkValues1sCopy = (char *) malloc( Gia_ManObjNum(p));
        memcpy(networkValues1sCopy, networkValues1s, sizeof(char) * Gia_ManObjNum(p));
        char * networkValuesNotSetCopy = (char *) malloc( Gia_ManObjNum(p));
        memcpy(networkValuesNotSetCopy, networkValuesNotSet, sizeof(char) * Gia_ManObjNum(p));
        Vec_Int_t * modifiedLuts = Vec_IntAlloc( Gia_ManLutNum(p) );
        // add the first lut to the modified luts
        Vec_IntPush(modifiedLuts, jLut);
        networkValues1sCopy[jLut] |= jLutGold & netValid;
        networkValuesNotSetCopy[jLut] &= ~netValid;
        // traverse the graph to retrieve values in the middle of the network
        int status = computeNetworkValues(p, pPars, jLut , &netValid, jLutGold, networkValues1sCopy, networkValuesNotSetCopy, modifiedLuts, vLutsFaninCones, vNodesWatchlist , experimentID);
        if( status == -1 ){
          Vec_IntFree(modifiedLuts);
          free(networkValues1sCopy);
          free(networkValuesNotSetCopy);
          goto exit_cond;
        }

        char orig_netValid = networkValid & lutValueNotSet;
        char difference_valid = orig_netValid ^ netValid;
        if( difference_valid == orig_netValid){
          if(pPars->fVeryVerbose)
            printf("FAILED 0 out of %d - node %d (gold = %d valid = %d)\n", outGold_bitwidth, jLut, jLutGold, netValid);
        } else if( difference_valid != 0){
          int jjj = 0, iLut;
          Vec_IntForEachEntry( modifiedLuts, iLut, jjj ){
            networkValues1sCopy[jLut] = networkValues1s[iLut];
            networkValuesNotSetCopy[jLut] = networkValuesNotSet[iLut];
          }
          memcpy(networkValues1s, networkValues1sCopy, sizeof(char) * Gia_ManObjNum(p));
          memcpy(networkValuesNotSet, networkValuesNotSetCopy, sizeof(char) * Gia_ManObjNum(p));

          int different = 0;
          while (difference_valid != 0) {
            if (difference_valid & 1) {
              different++;
            }
            difference_valid >>= 1;
          }
          if( outGold_bitwidth - different >= 2){ // at least 2 bits should be respected
            if(lastjGold != jLutGold){
              success += 1;
              lastjGold = jLutGold;
            }
          }
          if(pPars->fVeryVerbose)
            printf("SUCCESSFUL %d out of %d - node %d (gold = %d valid = %d)\n", outGold_bitwidth - different, outGold_bitwidth, jLut, jLutGold, netValid);
        } else {
          if(lastjGold != jLutGold){
            success += 1;
            lastjGold = jLutGold;
          }
          memcpy(networkValues1s, networkValues1sCopy, sizeof(char) * Gia_ManObjNum(p));
          memcpy(networkValuesNotSet, networkValuesNotSetCopy, sizeof(char) * Gia_ManObjNum(p));
          if(pPars->fVeryVerbose)
            printf("SUCCESSFUL %d out of %d - node %d (gold = %d valid = %d)\n", outGold_bitwidth, outGold_bitwidth, jLut, jLutGold, netValid);
        }


        free(networkValues1sCopy);
        free(networkValuesNotSetCopy);
        Vec_IntFree(modifiedLuts);
        

      } else { // all the different values that are still valid have been set
        char luts_values = lutValues1s & networkValid;
        char out_gold_values = jLutGold & networkValid;        
        char validAssignments = ~( luts_values ^ out_gold_values ) & networkValid;
        char numOnes = 0;
        if (validAssignments != 0){
          while (validAssignments != 0) {
            if (validAssignments & 1) {
              numOnes++;
            }
            validAssignments >>= 1;
          }
        }
        validAssignments = ~( luts_values ^ out_gold_values ) & networkValid;
        if(pPars->fVeryVerbose)
          printf("(1) SUCCESSFUL %d out of %d - node %d (gold = %d valid = %d)\n", numOnes, outGold_bitwidth, jLut, jLutGold, validAssignments);
      }

      iii++;
      mainLoop_condition = (iii < numLuts);
    }

    if(success >= 2){
      saveInputVectors(p, pMan, networkValues1s);
      Cec4_ManSimulate( p, pMan );
      // saving the pi simulation values in a compacted format
      int jth_word = p->iData & 0xFFFF;
      int kth_bit = (p->iData & 0xFFFF0000) >> 16; 
      if( kth_bit + outGold_bitwidth >= 64 ){
        kth_bit = 0;
        jth_word += 1;
      }
      saveSimVectors(p, vSimSave, outGold_bitwidth, jth_word, kth_bit);
      kth_bit += outGold_bitwidth;
      p->iData = (kth_bit << 16) | jth_word;
      if( pPars->fVerbose || pPars->fVeryVerbose){
        printf("**Exporting simulation values in file sim_values.txt\n");
        exportSimValues( p, "sim_values.txt" );
      }
    }


exit_cond:
    free(networkValues1s);
    free(networkValuesNotSet);

    if(success < 2){
      return -1;
    } else {
      return 1;
    }

}


/**Function*************************************************************

  Synopsis    [Core function of SimGen.]

  Description []
               
  SideEffects []

  SeeAlso     []
***********************************************************************/


void executeControlledSim(Gia_Man_t * p, Cec4_Man_t * pMan, Cec_ParSimGen_t * pPars , int levelOrder, Vec_Ptr_t * vSimSave, int experimentID){

    char * outGold;
    //int outGold_bitwidth = pPars->bitwidthOutgold;
    int numClasses = totalNumClasses(p);
    int iTrials = 0, outer_loop_condition = iTrials < (numClasses * 1.5); 
    int nthClass = 0, status, oldNumClasses = numClasses, iter;
    Vec_Int_t * luts_order, * vecTmp;
    Vec_Int_t * vLutsFaninCones;
    clock_t start_time = clock();
    outGold = generateOutGoldValues(p); // generate the first outgold values
    pPars->outGold = outGold;
    Vec_Ptr_t * vNodesWatchList = NULL; 
    if(pPars->fUseWatchlist){
      printf("**Activating watchlist feauture\n");
    }
    while( existsOneClass(p) && outer_loop_condition ){
      luts_order = extractNthClass(p, nthClass);
      if( levelOrder != -1 ){
        // test different luts orders on which to apply simgen
        luts_order = computeLutsOrder(p, 1); // incresing level order // TODO: redo taking luts_order as input
      }
      // compute the fanin cones of the luts to apply implication only in the cone
      vLutsFaninCones = computeFaninCones( p, luts_order );
      if(pPars->fUseWatchlist){
        vNodesWatchList = generateWatchList(p);
      }
      status = computeInputVectors(p, pMan, pPars, luts_order, outGold, pPars->bitwidthOutgold, vLutsFaninCones, vNodesWatchList, vSimSave, experimentID);
      if( status == -1 ){
        if(pPars->fVerbose || pPars->fVeryVerbose)
          printf("FAILED - no valid input vectors found for class %d\n", nthClass);
        nthClass++;
      } else {
        numClasses = totalNumClasses(p);
        if (oldNumClasses == numClasses){
          nthClass++;
        }
        oldNumClasses = numClasses;
        free(outGold);
        outGold = generateOutGoldValues(p); // generate a new outgold since the classes are updated
        pPars->outGold = outGold;
        if(pPars->fVerbose || pPars->fVeryVerbose)
          printf("SUCCESSFUL - input vectors found for class %d\n", nthClass);
        //nthClass = 0;
      }

      Vec_IntFree(vLutsFaninCones);
      Vec_IntFree(luts_order);
      if(pPars->fUseWatchlist){
        Vec_PtrForEachEntry( Vec_Int_t *, vNodesWatchList, vecTmp, iter ){
          if (vecTmp != NULL)
            Vec_IntFree(vecTmp);        
        }
        Vec_PtrFree(vNodesWatchList);
      }
      if(nthClass >= numClasses){
        nthClass = 0;
        if (oldNumClasses == numClasses){
          iTrials++;
        } 
        oldNumClasses = numClasses;
        iTrials++;
      }
      outer_loop_condition = iTrials < 3; 
      clock_t end_time = clock();
      if ( (float)(end_time - start_time)/CLOCKS_PER_SEC > pPars->timeOutSim){ // hard limit of 1000 sec
        printf("Timeout of %f sec reached\n", pPars->timeOutSim);
        break;
      }
      if(pPars->fVerbose || pPars->fVeryVerbose){
        int quality = evaluate_equiv_classes(p, 0);
        printf("Time elapsed: %f (classes quality: %d)\n", (float)(clock() - start_time)/CLOCKS_PER_SEC, quality);
      }
    }

    //drawNetworkGia( p, "network.dot");
    //drawConeNetworkGia( p, "cone_network.dot", 31);

    free(outGold);

}

/**Function*************************************************************

  Synopsis    [Call SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void Cec4_CallSATsolver(Gia_Man_t * p, Cec4_Man_t * pMan, Cec_ParFra_t * pPars){

    extern Gia_Obj_t *  Cec4_ManFindRepr( Gia_Man_t * p, Cec4_Man_t * pMan, int iObj );
    int i;
    Gia_Obj_t * pObj, * pRepr; 
    p->iPatsPi = 0;
    Vec_WrdFill( p->vSimsPi, Vec_WrdSize(p->vSimsPi), 0 );
    pMan->nSatSat = 0;
    pMan->pNew = Cec4_ManStartNew( p );
    Gia_ManForEachAnd( p, pObj, i )
    {
        Gia_Obj_t * pObjNew; 
        pMan->nAndNodes++;
        if ( Gia_ObjIsXor(pObj) )
            pObj->Value = Gia_ManHashXorReal( pMan->pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else
            pObj->Value = Gia_ManHashAnd( pMan->pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        if ( pPars->nLevelMax && Gia_ObjLevel(p, pObj) > pPars->nLevelMax )
            continue;
        pObjNew = Gia_ManObj( pMan->pNew, Abc_Lit2Var(pObj->Value) );
        if ( Gia_ObjIsAnd(pObjNew) )
        if ( Vec_BitEntry(pMan->vFails, Gia_ObjFaninId0(pObjNew, Abc_Lit2Var(pObj->Value))) || 
             Vec_BitEntry(pMan->vFails, Gia_ObjFaninId1(pObjNew, Abc_Lit2Var(pObj->Value))) )
            Vec_BitWriteEntry( pMan->vFails, Abc_Lit2Var(pObjNew->Value), 1 );
        //if ( Gia_ObjIsAnd(pObjNew) )
        //    Gia_ObjSetAndLevel( pMan->pNew, pObjNew );
        // select representative based on candidate equivalence classes
        pRepr= Gia_ObjReprObj( p, i );
        if ( pRepr == NULL )
            continue; 
        if ( 1 ) // select representative based on recent counter-examples
        {
            pRepr = (Gia_Obj_t *) Cec4_ManFindRepr( p, pMan, i );
            if ( pRepr == NULL )
                continue;
        }
        int id_obj = Gia_ObjId( p, pObj );
        int id_repr = Gia_ObjId( p, pRepr );

        if ( Abc_Lit2Var(pObj->Value) == Abc_Lit2Var(pRepr->Value))
        {
            if ( pPars->fBMiterInfo ) 
            {
                Bnd_ManMerge( id_repr, id_obj, pObj->fPhase ^ pRepr->fPhase );
            }
            if((pObj->Value ^ pRepr->Value) != (pObj->fPhase ^ pRepr->fPhase)){
              pObj->Value = Abc_LitNotCond( pRepr->Value, pObj->fPhase ^ pRepr->fPhase );
            }
            assert( (pObj->Value ^ pRepr->Value) == (pObj->fPhase ^ pRepr->fPhase) );
            Gia_ObjSetProved( p, i );
            if ( Gia_ObjId(p, pRepr) == 0 )
                pMan->iLastConst = i;
            continue;
        }
        if ( Cec4_ManSweepNode(pMan, i, Gia_ObjId(p, pRepr)) && Gia_ObjProved(p, i) )
        {
            if (pPars->fBMiterInfo){

                Bnd_ManMerge( id_repr, id_obj, pObj->fPhase ^ pRepr->fPhase );
                // printf( "proven %d merged into %d (phase : %d)\n", Gia_ObjId(p, pObj), Gia_ObjId(p,pRepr), pObj->fPhase ^ pRepr -> fPhase );

            }
            pObj->Value = Abc_LitNotCond( pRepr->Value, pObj->fPhase ^ pRepr->fPhase );


        }
    }

    printf( "SAT calls = %d:  P = %d (0=%d a=%.2f m=%d)  D = %d (0=%d a=%.2f m=%d)  F = %d   Sim = %d  Recyc = %d  Xor = %.2f %%\n", 
            pMan->nSatUnsat + pMan->nSatSat + pMan->nSatUndec, 
            pMan->nSatUnsat, pMan->nConflicts[1][0], (float)pMan->nConflicts[1][1]/Abc_MaxInt(1, pMan->nSatUnsat-pMan->nConflicts[1][0]), pMan->nConflicts[1][2],
            pMan->nSatSat,   pMan->nConflicts[0][0], (float)pMan->nConflicts[0][1]/Abc_MaxInt(1, pMan->nSatSat  -pMan->nConflicts[0][0]), pMan->nConflicts[0][2],  
            pMan->nSatUndec,  
            pMan->nSimulates, pMan->nRecycles, 100.0*pMan->nGates[1]/Abc_MaxInt(1, pMan->nGates[0]+pMan->nGates[1]) );

}

/**Function*************************************************************

  Synopsis    [Print encoded cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void printEncodedCube( char pCube, char pDCs, int nFanins ){

    int i;
    for (i = 0; i < nFanins; i++){
      if ( pCube & (1 << i) )
        printf("1");
      else if ( pDCs & (1 << i) )
        printf("-");
      else
        printf("0");
    }
    printf("\n");

}

/**Function*************************************************************

  Synopsis    [Print ISOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void printISOP( char * pSop, int nCubes, int nFanins ){

    int jth_cube = 0;
    while (jth_cube < nCubes){
      char pCube =  *pSop; pSop++;
      char pDCs =  *pSop; pSop++;
      printEncodedCube( pCube, pDCs, nFanins );
      jth_cube++;
    }

}

/**Function*************************************************************

  Synopsis    [Print ISOP LUT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void printISOPLUT(Gia_Man_t * pMan, int ObjId){

    assert( Gia_ObjIsLut(pMan, ObjId));
    char * pSop[2]; int nCubes[2];
    getISOPObjId( pMan, ObjId, pSop, nCubes );
    printf("Negative Polarity\n");
    printISOP( pSop[0], nCubes[0], Gia_ObjLutSize(pMan, ObjId) );
    printf("Positive Polarity\n");
    printISOP( pSop[1], nCubes[1], Gia_ObjLutSize(pMan, ObjId) );

}

/**Function*************************************************************

  Synopsis    [Execute SimGen.]

  Description []
               
  SideEffects []

  SeeAlso     []
***********************************************************************/

Gia_Man_t * Cec_SimGenRun( Gia_Man_t * p, Cec_ParSimGen_t * pPars ){

    extern void Gia_ManDupMapping( Gia_Man_t * pNew, Gia_Man_t * p );
    Cec4_Man_t * pManSim;  
    int i, k, nWordsPerCi;
    Gia_Man_t * pMapped;
    Vec_Ptr_t * vSimPisSave;
    Cec_ParFra_t * pCECPars;

    if (!Gia_ManHasMapping(p)){
      // apply technology mapping if not already done
      If_Par_t IfPars, * pIfPars = &IfPars;
      Gia_ManSetIfParsDefault( pIfPars );
      pIfPars->nLutSize = 6;
      pMapped = Gia_ManPerformMapping( p, pIfPars );
      if(pPars->fVerbose)
        printf("Performing LUT-mapping\n");
    } else {
      pMapped = Gia_ManDup( p );
      Gia_ManDupMapping( pMapped, p );
      if(pPars->fVerbose)
        printf("Using already mapped network\n");
    }
    pCECPars = (Cec_ParFra_t *) malloc(sizeof( Cec_ParFra_t )); // parameters of CEC
    pManSim = Cec4_ManCreate( pMapped, pCECPars );
    pManSim->pPars->fVerbose = 0; // disabling verbose sat solver

    Cec_DeriveSOPs( pMapped );

    /*
    if (pPars->fVeryVerbose)
    {    
      printf("**Printing LUTs information**\n");
      Gia_ManForEachLut( pMapped, i ){
        printf("LUT %d\n", i);
        Gia_LutForEachFanin( pMapped, i, iFan, k ){
          printf("\tFANIN %d\n", iFan);
        }
        printISOPLUT( pMapped, i );
      }
    }
    */

    // compute MFFCs
    Gia_ManLevelNum( pMapped); // compute levels
    computeMFFCs( pMapped );
    if (pPars->fUseWatchlist) {
      generateLutsRankings( pMapped );
    }
    

    // generate vectors of fanouts
    Gia_generateFanoutMapping( pMapped );

    pMapped->nSimWords = (int) pPars->nSimWords ;
    vSimPisSave = Vec_PtrAlloc( Gia_ManCiNum(pMapped) );
    for ( i = 0; i < Gia_ManCiNum(pMapped); i++ ){
      Vec_Wrd_t * vSim = Vec_WrdAlloc( pMapped->nSimWords );
      Vec_PtrPush( vSimPisSave, vSim );
    }
    // simulate n rounds of random simulation and create classes      
    Cec4_ManSimAlloc( pMapped, pMapped->nSimWords );
    if(pPars->nMaxIter >= 0){
      executeRandomSim( pMapped, pManSim, 0, pPars->nMaxIter, vSimPisSave, pPars->fVeryVerbose);
    } else if (pPars->nMaxIter == -1){
      executeRandomSim( pMapped, pManSim, 1, pPars->nMaxIter, vSimPisSave, pPars->fVeryVerbose);
    } else {
      printf("Invalid number of iterations %d\n", pPars->nMaxIter);
      return NULL;
    }

    // extra data to compact multiple pi values in single words
    pMapped->iData = ((Vec_Wrd_t *)Vec_PtrEntry( vSimPisSave, 0 ))->nSize; // [0:15] number of words filled
                         // [16:31] number of bits filled in the iData word
    assert(pMapped->iData > 0);

    Cec_RemoveNonLutNodes( pMapped ); // remove all the non-LUT nodes from the equivalence classes
    
    if(pPars->fVerbose || pPars->fVeryVerbose){
      // IMPORTANT: the number of classes changes due to the previous operation
      evaluate_equiv_classes(pMapped, 1);
      printf("**Printing Class information before running the Sim algos in file pre_classes.txt**\n");
      exportEquivClasses(pMapped, "pre_classes.txt");
    }
    //Cec4_ManPrintClasses2(pMapped);
    clock_t begin = clock();      
    assert(pPars->expId > 0);
    executeControlledSim( pMapped, pManSim, pPars, -1 , vSimPisSave, pPars->expId );
    
    float implicationSuccessRate = (float)pPars->nImplicationSuccess / (float)pPars->nImplicationExecution;
    float implicationSuccessCheckRate = (float)pPars->nImplicationSuccessChecks / (float)pPars->nImplicationTotalChecks;
    printf("Time elapsed: %f (implication time %f - %f successful recursions - %f successful checks)\n", (double)(clock() - begin) / CLOCKS_PER_SEC, pPars->fImplicationTime, implicationSuccessRate, implicationSuccessCheckRate);

    //Cec4_ManPrintClasses2(pMapped);
    if(pPars->fVerbose || pPars->fVeryVerbose){
      printf("**Printing Class information before running the Sim algos in file post_classes.txt**\n");
      exportEquivClasses(pMapped, "post_classes.txt");
      evaluate_equiv_classes(pMapped, 1);
    }

    // save random and controlled sim pis values
    if(p->vSimsPi != NULL){
      Vec_WrdFree( p->vSimsPi );
    } 
    nWordsPerCi = ((Vec_Wrd_t *)Vec_PtrEntry( vSimPisSave, 0 ))->nSize;
    assert( nWordsPerCi > 0 );
    p->vSimsPi = Vec_WrdStart( Gia_ManCiNum(p) * nWordsPerCi );

    // copy the sim pis
    assert( Gia_ManCiNum(p) == Vec_PtrSize(vSimPisSave) );
    for (i = 0; i < Gia_ManCiNum(p); i++){
      Vec_Wrd_t * vSim = (Vec_Wrd_t *)Vec_PtrEntry( vSimPisSave, i );
      assert( vSim->nSize == nWordsPerCi );
      word * pSim = Vec_WrdEntryP( p->vSimsPi, i * nWordsPerCi );
      for ( k = 0; k < nWordsPerCi; k++ )
        pSim[k] = Vec_WrdEntry( vSim, k );
    }
    p->nSimWords = nWordsPerCi;

    if (pPars->pFileName != NULL){
      Vec_WrdDumpHex( pPars->pFileName, p->vSimsPi, nWordsPerCi , 1 );
    }

    // free memory
    Vec_IntFree( pMapped->vTTLut );
    Vec_StrFree( pMapped->vTTISOPs );
    Cec4_ManDestroy( pManSim );
    Gia_ManStop( pMapped );

    return p;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

