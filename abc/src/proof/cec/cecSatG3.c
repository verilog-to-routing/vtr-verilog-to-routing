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
#include "aig/gia/giaCSatP.h"
#include <stdlib.h>

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
typedef struct Cec5_Man_t_ Cec5_Man_t;
struct Cec5_Man_t_
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
    int              nConflicts[3][3];
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

    int              simTravId;
    Vec_Int_t *      vPiPatsCache;
    int              fEec;
    int              LocalBatchSize;
    Vec_Bit_t *      vCexSite;
    int              simBound;
    int              simStart;
    int              approxLim;
    int              simGlobalTop;
    int              simBatchFactor;
    int              adaRecycle;
};

static inline int    Cec5_ObjSatId( Gia_Man_t * p, Gia_Obj_t * pObj )             { return Gia_ObjCopy2Array(p, Gia_ObjId(p, pObj));                                                     }
static inline int    Cec5_ObjSetSatId( Gia_Man_t * p, Gia_Obj_t * pObj, int Num ) { assert(Cec5_ObjSatId(p, pObj) == -1); Gia_ObjSetCopy2Array(p, Gia_ObjId(p, pObj), Num); Vec_IntPush(&p->vSuppVars, Gia_ObjId(p, pObj)); if ( Gia_ObjIsCi(pObj) ) Vec_IntPushTwo(&p->vCopiesTwo, Gia_ObjId(p, pObj), Num); assert(Vec_IntSize(&p->vVarMap) == Num); Vec_IntPush(&p->vVarMap, Gia_ObjId(p, pObj)); return Num;  }
static inline void   Cec5_ObjCleanSatId( Gia_Man_t * p, Gia_Obj_t * pObj )        { assert(Cec5_ObjSatId(p, pObj) != -1); Gia_ObjSetCopy2Array(p, Gia_ObjId(p, pObj), -1);               }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Wrd_t * Cec5_EvalCombine( Vec_Int_t * vPats, int nPats, int nInputs, int nWords )
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
void Cec5_EvalPatterns( Gia_Man_t * p, Vec_Int_t * vPats, int nPats )
{
    int nWords = Abc_Bit6WordNum(nPats);
    Vec_Wrd_t * vSimsPi = Cec5_EvalCombine( vPats, nPats, Gia_ManCiNum(p), nWords );
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
void Cec5_ManSetParams( Cec_ParFra_t * pPars )
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
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cec5_Man_t * Cec5_ManCreate( Gia_Man_t * pAig, Cec_ParFra_t * pPars )
{
    Cec5_Man_t * p = ABC_CALLOC( Cec5_Man_t, 1 );
    memset( p, 0, sizeof(Cec5_Man_t) );
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
    //pAig->pData     = p->pSat; // point AIG manager to the solver
    //Vec_IntFreeP( &p->pAig->vPats );
    //p->pAig->vPats = Vec_IntAlloc( 1000 );
    p->simTravId     = 0;
    p->vPiPatsCache  = Vec_IntAlloc( 100 );
    p->fEec          = 0;
    p->LocalBatchSize= 8;
    p->vCexSite      = Vec_BitStart( Gia_ManObjNum(pAig) );
    Vec_BitFill( p->vCexSite, Gia_ManObjNum(pAig), 0 );
    p->simBound      = pPars->nWords;
    p->simStart      = 0;
    p->approxLim     = 600;
    p->simBatchFactor= 1;
    p->simGlobalTop  = 0;
    p->adaRecycle    = 500;
    if ( pPars->nBTLimitPo )
    {
        int i, Driver;
        p->vCoDrivers = Vec_BitStart( Gia_ManObjNum(pAig) );
        Gia_ManForEachCoDriverId( pAig, Driver, i )
            Vec_BitWriteEntry( p->vCoDrivers, Driver, 1 );
    }
    return p;
}
void Cec5_ManDestroy( Cec5_Man_t * p )
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
    //Cec5_EvalPatterns( p->pAig, p->pAig->vPats, p->pAig->nBitPats );
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
    Vec_BitFreeP( &p->vCoDrivers );
    Vec_IntFreeP( &p->vRefClasses );
    Vec_IntFreeP( &p->vRefNodes );
    Vec_IntFreeP( &p->vRefBins );
    Vec_IntFreeP( &p->vPiPatsCache );
    Vec_BitFreeP( &p->vCexSite );
    ABC_FREE( p->pTable );
    ABC_FREE( p );
}
Gia_Man_t * Cec5_ManStartNew( Gia_Man_t * pAig )
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
void Cec5_AddClausesMux( Gia_Man_t * p, Gia_Obj_t * pNode, sat_solver * pSat )
{
    int fPolarFlip = 0;
    Gia_Obj_t * pNodeI, * pNodeT, * pNodeE;
    int pLits[4], RetValue, VarF, VarI, VarT, VarE, fCompT, fCompE;

    assert( !Gia_IsComplement( pNode ) );
    assert( pNode->fMark0 );
    // get nodes (I = if, T = then, E = else)
    pNodeI = Gia_ObjRecognizeMux( pNode, &pNodeT, &pNodeE );
    // get the variable numbers
    VarF = Cec5_ObjSatId(p, pNode);
    VarI = Cec5_ObjSatId(p, pNodeI);
    VarT = Cec5_ObjSatId(p, Gia_Regular(pNodeT));
    VarE = Cec5_ObjSatId(p, Gia_Regular(pNodeE));
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
void Cec5_AddClausesSuper( Gia_Man_t * p, Gia_Obj_t * pNode, Vec_Ptr_t * vSuper, sat_solver * pSat )
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
        pLits[0] = Abc_Var2Lit(Cec5_ObjSatId(p, Gia_Regular(pFanin)), Gia_IsComplement(pFanin));
        pLits[1] = Abc_Var2Lit(Cec5_ObjSatId(p, pNode), 1);
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
        pLits[i] = Abc_Var2Lit(Cec5_ObjSatId(p, Gia_Regular(pFanin)), !Gia_IsComplement(pFanin));
        if ( fPolarFlip )
        {
            if ( Gia_Regular(pFanin)->fPhase )  pLits[i] = Abc_LitNot( pLits[i] );
        }
    }
    pLits[nLits-1] = Abc_Var2Lit(Cec5_ObjSatId(p, pNode), 0);
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
void Cec5_CollectSuper_rec( Gia_Obj_t * pObj, Vec_Ptr_t * vSuper, int fFirst, int fUseMuxes )
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
    Cec5_CollectSuper_rec( Gia_ObjChild0(pObj), vSuper, 0, fUseMuxes );
    Cec5_CollectSuper_rec( Gia_ObjChild1(pObj), vSuper, 0, fUseMuxes );
}
void Cec5_CollectSuper( Gia_Obj_t * pObj, int fUseMuxes, Vec_Ptr_t * vSuper )
{
    assert( !Gia_IsComplement(pObj) );
    assert( !Gia_ObjIsCi(pObj) );
    Vec_PtrClear( vSuper );
    Cec5_CollectSuper_rec( pObj, vSuper, 1, fUseMuxes );
}
void Cec5_ObjAddToFrontier( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Ptr_t * vFrontier, sat_solver * pSat )
{
    assert( !Gia_IsComplement(pObj) );
    assert( !Gia_ObjIsConst0(pObj) );
    if ( Cec5_ObjSatId(p, pObj) >= 0 )
        return;
    assert( Cec5_ObjSatId(p, pObj) == -1 );
    Cec5_ObjSetSatId( p, pObj, sat_solver_addvar(pSat) );
    if ( Gia_ObjIsAnd(pObj) )
        Vec_PtrPush( vFrontier, pObj );
}
int Cec5_ObjGetCnfVar( Cec5_Man_t * p, int iObj )
{ 
    int fUseSimple = 1; // enable simple CNF
    int fUseMuxes  = 1; // enable MUXes when using complex CNF
    Gia_Obj_t * pNode, * pFanin;
    Gia_Obj_t * pObj = Gia_ManObj(p->pNew, iObj);
    int i, k;
    // quit if CNF is ready
    if ( Cec5_ObjSatId(p->pNew,pObj) >= 0 )
        return Cec5_ObjSatId(p->pNew,pObj);

    assert( iObj > 0 );
    if ( Gia_ObjIsCi(pObj) )
        return Cec5_ObjSetSatId( p->pNew, pObj, sat_solver_addvar(p->pSat) );
    assert( Gia_ObjIsAnd(pObj) );
    if ( fUseSimple )
    {
        Gia_Obj_t * pFan0, * pFan1;
        //if ( Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
        //    printf( "%d", (Gia_IsComplement(pFan1) << 1) + Gia_IsComplement(pFan0) );
        if ( p->pNew->pMuxes == NULL && Gia_ObjRecognizeExor(pObj, &pFan0, &pFan1) && Gia_IsComplement(pFan0) == Gia_IsComplement(pFan1) )
        {
            int iVar0 = Cec5_ObjGetCnfVar( p, Gia_ObjId(p->pNew, Gia_Regular(pFan0)) );
            int iVar1 = Cec5_ObjGetCnfVar( p, Gia_ObjId(p->pNew, Gia_Regular(pFan1)) );
            int iVar  = Cec5_ObjSetSatId( p->pNew, pObj, sat_solver_addvar(p->pSat) );
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
            int iVar0 = Cec5_ObjGetCnfVar( p, Gia_ObjFaninId0(pObj, iObj) );
            int iVar1 = Cec5_ObjGetCnfVar( p, Gia_ObjFaninId1(pObj, iObj) );
            int iVar  = Cec5_ObjSetSatId( p->pNew, pObj, sat_solver_addvar(p->pSat) );
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
        return Cec5_ObjSatId( p->pNew, pObj );
    }
    assert( !Gia_ObjIsXor(pObj) );
    // start the frontier
    Vec_PtrClear( p->vFrontier );
    Cec5_ObjAddToFrontier( p->pNew, pObj, p->vFrontier, p->pSat );
    // explore nodes in the frontier
    Vec_PtrForEachEntry( Gia_Obj_t *, p->vFrontier, pNode, i )
    {
        // create the supergate
        assert( Cec5_ObjSatId(p->pNew,pNode) >= 0 );
        if ( fUseMuxes && pNode->fMark0 )
        {
            Vec_PtrClear( p->vFanins );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin0( Gia_ObjFanin0(pNode) ) );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin0( Gia_ObjFanin1(pNode) ) );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin1( Gia_ObjFanin0(pNode) ) );
            Vec_PtrPushUnique( p->vFanins, Gia_ObjFanin1( Gia_ObjFanin1(pNode) ) );
            Vec_PtrForEachEntry( Gia_Obj_t *, p->vFanins, pFanin, k )
                Cec5_ObjAddToFrontier( p->pNew, Gia_Regular(pFanin), p->vFrontier, p->pSat );
            Cec5_AddClausesMux( p->pNew, pNode, p->pSat );
        }
        else
        {
            Cec5_CollectSuper( pNode, fUseMuxes, p->vFanins );
            Vec_PtrForEachEntry( Gia_Obj_t *, p->vFanins, pFanin, k )
                Cec5_ObjAddToFrontier( p->pNew, Gia_Regular(pFanin), p->vFrontier, p->pSat );
            Cec5_AddClausesSuper( p->pNew, pNode, p->vFanins, p->pSat );
        }
        assert( Vec_PtrSize(p->vFanins) > 1 );
    }
    return Cec5_ObjSatId(p->pNew,pObj);
}


/**Function*************************************************************

  Synopsis    [Refinement of equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word * Cec5_ObjSim( Gia_Man_t * p, int iObj )
{
    return Vec_WrdEntryP( p->vSims, p->nSimWords * iObj );
}
static inline int Cec5_ObjSimEqual( Gia_Man_t * p, int iObj0, int iObj1 )
{
    int w;
    word * pSim0 = Cec5_ObjSim( p, iObj0 );
    word * pSim1 = Cec5_ObjSim( p, iObj1 );
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
int Cec5_ManSimHashKey( word * pSim, int nSims, int nTableSize )
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
void Cec5_RefineOneClassIter( Gia_Man_t * p, int iRepr )
{
    int iObj, iPrev = iRepr, iPrev2, iRepr2;
    assert( Gia_ObjRepr(p, iRepr) == GIA_VOID );
    assert( Gia_ObjNext(p, iRepr) > 0 );
    Gia_ClassForEachObj1( p, iRepr, iRepr2 )
        if ( Cec5_ObjSimEqual(p, iRepr, iRepr2) )
            iPrev = iRepr2;
        else
            break;
    if ( iRepr2 <= 0 ) // no refinement
        return;
    // relink remaining nodes of the class
    // nodes that are equal to iRepr, remain in the class of iRepr
    // nodes that are not equal to iRepr, move to the class of iRepr2
    Gia_ObjSetRepr( p, iRepr2, GIA_VOID );
    assert( !Gia_ObjProved(p,iRepr2) );
    iPrev2 = iRepr2;
    for ( iObj = Gia_ObjNext(p, iRepr2); iObj > 0; iObj = Gia_ObjNext(p, iObj) )
    {
        if ( Cec5_ObjSimEqual(p, iRepr, iObj) ) // remains with iRepr
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
        Cec5_RefineOneClassIter( p, iRepr2 );
}
void Cec5_RefineOneClass( Gia_Man_t * p, Cec5_Man_t * pMan, Vec_Int_t * vNodes )
{
    int k, iObj, Bin;
    Vec_IntClear( pMan->vRefBins );
    Vec_IntForEachEntryReverse( vNodes, iObj, k )
    {
        int Key = Cec5_ManSimHashKey( Cec5_ObjSim(p, iObj), p->nSimWords, pMan->nTableSize );
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
        assert( !Gia_ObjProved(p,iRepr) );
        if ( p->pNexts[iRepr] == -1 )
            continue;
        for ( iObj = p->pNexts[iRepr]; iObj > 0; iObj = p->pNexts[iObj] )
            p->pReprs[iObj].iRepr = iRepr;
        Cec5_RefineOneClassIter( p, iRepr );
    }
    Vec_IntClear( pMan->vRefBins );
}
void Cec5_RefineClasses( Gia_Man_t * p, Cec5_Man_t * pMan, Vec_Int_t * vClasses )
{
    if ( Vec_IntSize(pMan->vRefClasses) == 0 )
        return;
    if ( Vec_IntSize(pMan->vRefNodes) > 0 )
        Cec5_RefineOneClass( p, pMan, pMan->vRefNodes );
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
            Cec5_RefineOneClass( p, pMan, pMan->vRefNodes );
        }
    }
    Vec_IntClear( pMan->vRefClasses );
    Vec_IntClear( pMan->vRefNodes );
}
void Cec5_RefineInit( Gia_Man_t * p, Cec5_Man_t * pMan )
{
    Gia_Obj_t * pObj; int i;
    if( pMan->fEec ){
        assert( p->pReprs );
        assert( p->pNexts );
    } else {
        ABC_FREE( p->pReprs );
        ABC_FREE( p->pNexts );
        p->pReprs = ABC_CALLOC( Gia_Rpr_t, Gia_ManObjNum(p) );
        p->pNexts = ABC_FALLOC( int, Gia_ManObjNum(p) );
    }

    pMan->nTableSize  = Abc_PrimeCudd( Gia_ManObjNum(p) );
    pMan->pTable      = ABC_FALLOC( int, pMan->nTableSize );
    pMan->vRefNodes   = Vec_IntAlloc( Gia_ManObjNum(p) );
    
    pMan->vRefBins    = Vec_IntAlloc( Gia_ManObjNum(p)/2 );
    pMan->vRefClasses = Vec_IntAlloc( Gia_ManObjNum(p)/2 );

    if( pMan->fEec ) return;

    Gia_ManForEachObj( p, pObj, i )
    {
        p->pReprs[i].iRepr = GIA_VOID;
        if ( !Gia_ObjIsCo(pObj) && (!pMan->pPars->nLevelMax || Gia_ObjLevel(p, pObj) <= pMan->pPars->nLevelMax) )
            Vec_IntPush( pMan->vRefNodes, i );
    }
    
    Vec_IntPush( pMan->vRefClasses, 0 );
}


/**Function*************************************************************

  Synopsis    [Internal simulation APIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Cec5_ObjSimSetInputBit( Gia_Man_t * p, int iObj, int Bit )
{
    word * pSim = Cec5_ObjSim( p, iObj );
    if ( Abc_InfoHasBit( (unsigned*)pSim, p->iPatsPi ) != Bit )
        Abc_InfoXorBit( (unsigned*)pSim, p->iPatsPi );
}
static inline int Cec5_ObjSimGetInputBit( Gia_Man_t * p, int iObj )
{
    word * pSim = Cec5_ObjSim( p, iObj );
    return Abc_InfoHasBit( (unsigned*)pSim, p->iPatsPi );
}
static inline void Cec5_ObjSimRo( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSimRo = Cec5_ObjSim( p, iObj );
    word * pSimRi = Cec5_ObjSim( p, Gia_ObjRoToRiId(p, iObj) );
    for ( w = 0; w < p->nSimWords; w++ )
        pSimRo[w] = pSimRi[w];
}
static inline void Cec5_ObjSimCo( Gia_Man_t * p, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSimCo  = Cec5_ObjSim( p, iObj );
    word * pSimDri = Cec5_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) )
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] = ~pSimDri[w];
    else
        for ( w = 0; w < p->nSimWords; w++ )
            pSimCo[w] =  pSimDri[w];
}
static inline void Cec5_ObjSimAnd( Gia_Man_t * p, Cec5_Man_t * pMan, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSim  = Cec5_ObjSim( p, iObj );
    word * pSim0 = Cec5_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    word * pSim1 = Cec5_ObjSim( p, Gia_ObjFaninId1(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = pMan->simStart; w < pMan->simBound; w++ )
            pSim[w] = ~pSim0[w] & ~pSim1[w];
    else if ( Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) )
        for ( w = pMan->simStart; w < pMan->simBound; w++ )
            pSim[w] = ~pSim0[w] & pSim1[w];
    else if ( !Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) )
        for ( w = pMan->simStart; w < pMan->simBound; w++ )
            pSim[w] = pSim0[w] & ~pSim1[w];
    else
        for ( w = pMan->simStart; w < pMan->simBound; w++ )
            pSim[w] = pSim0[w] & pSim1[w];
}
static inline void Cec5_ObjSimXor( Gia_Man_t * p, Cec5_Man_t * pMan, int iObj )
{
    int w;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    word * pSim  = Cec5_ObjSim( p, iObj );
    word * pSim0 = Cec5_ObjSim( p, Gia_ObjFaninId0(pObj, iObj) );
    word * pSim1 = Cec5_ObjSim( p, Gia_ObjFaninId1(pObj, iObj) );
    if ( Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC1(pObj) )
        for ( w = pMan->simStart; w < pMan->simBound; w++ )
            pSim[w] = ~pSim0[w] ^ pSim1[w];
    else
        for ( w = pMan->simStart; w < pMan->simBound; w++ )
            pSim[w] =  pSim0[w] ^ pSim1[w];
}
static inline void Cec5_ObjSimCi( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSim = Cec5_ObjSim( p, iObj );
    for ( w = 0; w < p->nSimWords; w++ )
        pSim[w] = Abc_RandomW( 0 );
    pSim[0] <<= 1;
}
static inline void Cec5_ObjClearSimCi( Gia_Man_t * p, int iObj )
{
    int w;
    word * pSim = Cec5_ObjSim( p, iObj );
    for ( w = 0; w < p->nSimWords; w++ )
        pSim[w] = 0;
}
void Cec5_ManSimulateCis( Gia_Man_t * p )
{
    int i, Id;
    Gia_ManForEachCiId( p, Id, i )
        Cec5_ObjSimCi( p, Id );
    p->iPatsPi = 0;
}
void Cec5_ManClearCis( Gia_Man_t * p )
{
    int i, Id;
    Gia_ManForEachCiId( p, Id, i )
        Cec5_ObjClearSimCi( p, Id );
    p->iPatsPi = 0;
}
Abc_Cex_t * Cec5_ManDeriveCex( Gia_Man_t * p, int iOut, int iPat )
{
    Abc_Cex_t * pCex;
    int i, Id;
    pCex = Abc_CexAlloc( 0, Gia_ManCiNum(p), 1 );
    pCex->iPo = iOut;
    if ( iPat == -1 )
        return pCex;
    Gia_ManForEachCiId( p, Id, i )
        if ( Abc_InfoHasBit((unsigned *)Cec5_ObjSim(p, Id), iPat) )
            Abc_InfoSetBit( pCex->pData, i );
    return pCex;
}
int Cec5_ManSimulateCos( Gia_Man_t * p )
{
    int i, Id;
    // check outputs and generate CEX if they fail
    Gia_ManForEachCoId( p, Id, i )
    {
        Cec5_ObjSimCo( p, Id );
        if ( Cec5_ObjSimEqual(p, Id, 0) )
            continue;
        p->pCexSeq = Cec5_ManDeriveCex( p, i, Abc_TtFindFirstBit2(Cec5_ObjSim(p, Id), p->nSimWords) );
        return 0;
    }
    return 1;
}
void Cec5_ManSimulate( Gia_Man_t * p, Cec5_Man_t * pMan )
{
    abctime clk = Abc_Clock();
    Gia_Obj_t * pObj; int i;
    pMan->nSimulates++;
    if ( pMan->pTable == NULL )
        Cec5_RefineInit( p, pMan );
    else
        assert( Vec_IntSize(pMan->vRefClasses) == 0 );

    pMan->simStart = pMan->simGlobalTop;
    Gia_ManForEachAnd( p, pObj, i )
    {
        int iRepr = Gia_ObjRepr( p, i );
        if ( Gia_ObjIsXor(pObj) )
            Cec5_ObjSimXor( p, pMan, i );
        else
            Cec5_ObjSimAnd( p, pMan, i );
        if ( iRepr == GIA_VOID || p->pReprs[iRepr].fColorA || Cec5_ObjSimEqual(p, iRepr, i) )
            continue;
        p->pReprs[iRepr].fColorA = 1;
        Vec_IntPush( pMan->vRefClasses, iRepr );
    }
    pMan->simStart = 0;
    pMan->timeSim += Abc_Clock() - clk;
    clk = Abc_Clock();
    Cec5_RefineClasses( p, pMan, pMan->vRefClasses );
    pMan->timeRefine += Abc_Clock() - clk;
}
void Cec5_ManSimulate_rec( Gia_Man_t * p, Cec5_Man_t * pMan, int iObj )
{
    Gia_Obj_t * pObj; 
    int progress;
    if ( !iObj || (progress = Vec_IntEntry(pMan->vCexStamps, iObj)) == pMan->simTravId )
        return;
    Vec_IntWriteEntry( pMan->vCexStamps, iObj, pMan->simTravId );
    pObj = Gia_ManObj(p, iObj);
    if ( Gia_ObjIsCi(pObj) )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Cec5_ManSimulate_rec( p, pMan, Gia_ObjFaninId0(pObj, iObj) );
    Cec5_ManSimulate_rec( p, pMan, Gia_ObjFaninId1(pObj, iObj) );
    pMan->simStart = progress * pMan->LocalBatchSize / (sizeof(word)<<3);
    if ( Gia_ObjIsXor(pObj) )
        Cec5_ObjSimXor( p, pMan, iObj );
    else
        Cec5_ObjSimAnd( p, pMan, iObj );
    pMan->simStart = 0;
}
void Cec5_ManSimAlloc( Gia_Man_t * p, int nWords, int fPrep )
{
    if( !fPrep ){
        Vec_WrdFreeP( &p->vSimsPi );
        p->vSimsPi   = Vec_WrdStart( (Gia_ManCiNum(p) + 1) * nWords );
    }
    Vec_WrdFreeP( &p->vSims );
    p->vSims     = Vec_WrdStart( Gia_ManObjNum(p) * nWords );
    p->nSimWords = nWords;
}


/**Function*************************************************************

  Synopsis    [Creating initial equivalence classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Cec5_ManPrintTfiConeStats( Gia_Man_t * p )
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
void Cec5_ManPrintStats( Gia_Man_t * p, Cec_ParFra_t * pPars, Cec5_Man_t * pMan, int fSim )
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
void Cec5_ManPrintClasses2( Gia_Man_t * p )
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
void Cec5_ManPrintClasses( Gia_Man_t * p )
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
int Cec5_ManVerify_rec( Gia_Man_t * p, int iObj, sat_solver * pSat )
{
    int Value0, Value1;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    if ( iObj == 0 ) return 0;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return pObj->fMark1;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    if ( Gia_ObjIsCi(pObj) )
        return pObj->fMark1 = sat_solver_read_cex_varvalue(pSat, Cec5_ObjSatId(p, pObj));
    assert( Gia_ObjIsAnd(pObj) );
    Value0 = Cec5_ManVerify_rec( p, Gia_ObjFaninId0(pObj, iObj), pSat ) ^ Gia_ObjFaninC0(pObj);
    Value1 = Cec5_ManVerify_rec( p, Gia_ObjFaninId1(pObj, iObj), pSat ) ^ Gia_ObjFaninC1(pObj);
    return pObj->fMark1 = Gia_ObjIsXor(pObj) ? Value0 ^ Value1 : Value0 & Value1;
}
void Cec5_ManVerify( Gia_Man_t * p, int iObj0, int iObj1, int fPhase, sat_solver * pSat )
{
    int Value0, Value1;
    Gia_ManIncrementTravId( p );
    Value0 = Cec5_ManVerify_rec( p, iObj0, pSat );
    Value1 = Cec5_ManVerify_rec( p, iObj1, pSat );
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
int Cec5_ManCexVerify_rec( Gia_Man_t * p, int iObj )
{
    int Value0, Value1;
    Gia_Obj_t * pObj = Gia_ManObj( p, iObj );
    if ( iObj == 0 ) return 0;
    if ( Gia_ObjIsTravIdCurrentId(p, iObj) )
        return pObj->fMark1;
    Gia_ObjSetTravIdCurrentId(p, iObj);
    if ( Gia_ObjIsCi(pObj) )
        return pObj->fMark1 = Cec5_ObjSimGetInputBit(p, iObj);
    assert( Gia_ObjIsAnd(pObj) );
    Value0 = Cec5_ManCexVerify_rec( p, Gia_ObjFaninId0(pObj, iObj) ) ^ Gia_ObjFaninC0(pObj);
    Value1 = Cec5_ManCexVerify_rec( p, Gia_ObjFaninId1(pObj, iObj) ) ^ Gia_ObjFaninC1(pObj);
    return pObj->fMark1 = Gia_ObjIsXor(pObj) ? Value0 ^ Value1 : Value0 & Value1;
}
void Cec5_ManCexVerify( Gia_Man_t * p, int iObj0, int iObj1, int fPhase )
{
    int Value0, Value1;
    Gia_ManIncrementTravId( p );
    Value0 = Cec5_ManCexVerify_rec( p, iObj0 );
    Value1 = Cec5_ManCexVerify_rec( p, iObj1 );
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
void Cec5_ManPackAddPatterns( Gia_Man_t * p, int iBit, Vec_Int_t * vLits )
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
int Cec5_ManPackAddPatternTry( Gia_Man_t * p, int iBit, Vec_Int_t * vLits )
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
int Cec5_ManPackAddPattern( Gia_Man_t * p, Vec_Int_t * vLits, int fExtend )
{
    int k;
    for ( k = 1; k < 64 * p->nSimWords - 1; k++ )
    {
        if ( ++p->iPatsPi == 64 * p->nSimWords - 1 )
            p->iPatsPi = 1;
        if ( Cec5_ManPackAddPatternTry( p, p->iPatsPi, vLits ) )
        {
            if ( fExtend )
                Cec5_ManPackAddPatterns( p, p->iPatsPi, vLits );
            break;
        }
    }
    if ( k == 64 * p->nSimWords - 1 )
    {
        p->iPatsPi = k;
        if ( !Cec5_ManPackAddPatternTry( p, p->iPatsPi, vLits ) )
            printf( "Internal error.\n" );
        else if ( fExtend )
            Cec5_ManPackAddPatterns( p, p->iPatsPi, vLits );
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
static inline int Cec5_ObjFan0IsAssigned( Gia_Obj_t * pObj )
{
    return Gia_ObjFanin0(pObj)->fMark0 || Gia_ObjFanin0(pObj)->fMark1;
}
static inline int Cec5_ObjFan1IsAssigned( Gia_Obj_t * pObj )
{
    return Gia_ObjFanin1(pObj)->fMark0 || Gia_ObjFanin1(pObj)->fMark1;
}
static inline int Cec5_ObjFan0HasValue( Gia_Obj_t * pObj, int v )
{
    return (v ^ Gia_ObjFaninC0(pObj)) ? Gia_ObjFanin0(pObj)->fMark1 : Gia_ObjFanin0(pObj)->fMark0;
}
static inline int Cec5_ObjFan1HasValue( Gia_Obj_t * pObj, int v )
{
    return (v ^ Gia_ObjFaninC1(pObj)) ? Gia_ObjFanin1(pObj)->fMark1 : Gia_ObjFanin1(pObj)->fMark0;
}
static inline int Cec5_ObjObjIsImpliedValue( Gia_Obj_t * pObj, int v )
{
    assert( !pObj->fMark0 && !pObj->fMark1 ); // not visited
    if ( v ) 
    return Cec5_ObjFan0HasValue(pObj, 1) && Cec5_ObjFan1HasValue(pObj, 1);
    return Cec5_ObjFan0HasValue(pObj, 0) || Cec5_ObjFan1HasValue(pObj, 0);
}
static inline int Cec5_ObjFan0IsImpliedValue( Gia_Obj_t * pObj, int v )
{
    return Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) && Cec5_ObjObjIsImpliedValue( Gia_ObjFanin0(pObj), v ^ Gia_ObjFaninC0(pObj) );
}
static inline int Cec5_ObjFan1IsImpliedValue( Gia_Obj_t * pObj, int v )
{
    return Gia_ObjIsAnd(Gia_ObjFanin1(pObj)) && Cec5_ObjObjIsImpliedValue( Gia_ObjFanin1(pObj), v ^ Gia_ObjFaninC1(pObj) );
}
int Cec5_ManGeneratePatterns_rec( Gia_Man_t * p, Gia_Obj_t * pObj, int Value, Vec_Int_t * vPat, Vec_Int_t * vVisit )
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
        int Ass0 = Cec5_ObjFan0IsAssigned(pObj);
        int Ass1 = Cec5_ObjFan1IsAssigned(pObj);
        assert( Gia_ObjFaninC0(pObj) == 0 && Gia_ObjFaninC1(pObj) == 0 );
        if ( Ass0 && Ass1 )
            return Value == (Cec5_ObjFan0HasValue(pObj, 1) ^ Cec5_ObjFan1HasValue(pObj, 1));
        if ( Ass0 )
        {
            int ValueInt = Value ^ Cec5_ObjFan0HasValue(pObj, 1);
            if ( !Cec5_ManGeneratePatterns_rec(p, pFan1, ValueInt, vPat, vVisit) )
                return 0;
        }
        else if ( Ass1 )
        {
            int ValueInt = Value ^ Cec5_ObjFan1HasValue(pObj, 1);
            if ( !Cec5_ManGeneratePatterns_rec(p, pFan0, ValueInt, vPat, vVisit) )
                return 0;
        }
        else if ( Abc_Random(0) & 1 )
        {
            if ( !Cec5_ManGeneratePatterns_rec(p, pFan0, 0,      vPat, vVisit) )
                return 0;
            if ( Cec5_ObjFan1HasValue(pObj, !Value) || (!Cec5_ObjFan1HasValue(pObj, Value) && !Cec5_ManGeneratePatterns_rec(p, pFan1, Value,  vPat, vVisit)) )
                return 0;
        }
        else
        {
            if ( !Cec5_ManGeneratePatterns_rec(p, pFan0, 1,      vPat, vVisit) )
                return 0;
            if ( Cec5_ObjFan1HasValue(pObj, Value) || (!Cec5_ObjFan1HasValue(pObj, !Value) && !Cec5_ManGeneratePatterns_rec(p, pFan1, !Value, vPat, vVisit)) )
                return 0;
        }
        assert( Value == (Cec5_ObjFan0HasValue(pObj, 1) ^ Cec5_ObjFan1HasValue(pObj, 1)) );
        return 1;
    }
    else if ( Value )
    {
        if ( Cec5_ObjFan0HasValue(pObj, 0) || Cec5_ObjFan1HasValue(pObj, 0) )
            return 0;
        if ( !Cec5_ObjFan0HasValue(pObj, 1) && !Cec5_ManGeneratePatterns_rec(p, pFan0, !Gia_ObjFaninC0(pObj), vPat, vVisit) )
            return 0;
        if ( !Cec5_ObjFan1HasValue(pObj, 1) && !Cec5_ManGeneratePatterns_rec(p, pFan1, !Gia_ObjFaninC1(pObj), vPat, vVisit) )
            return 0;
        assert( Cec5_ObjFan0HasValue(pObj, 1) && Cec5_ObjFan1HasValue(pObj, 1) );
        return 1;
    }
    else
    {
        if ( Cec5_ObjFan0HasValue(pObj, 1) && Cec5_ObjFan1HasValue(pObj, 1) )
            return 0;
        if ( Cec5_ObjFan0HasValue(pObj, 0) || Cec5_ObjFan1HasValue(pObj, 0) )
            return 1;
        if ( Cec5_ObjFan0HasValue(pObj, 1) )
        {
            if ( !Cec5_ManGeneratePatterns_rec(p, pFan1, Gia_ObjFaninC1(pObj), vPat, vVisit) )
                return 0;
        }
        else if ( Cec5_ObjFan1HasValue(pObj, 1) )
        {
            if ( !Cec5_ManGeneratePatterns_rec(p, pFan0, Gia_ObjFaninC0(pObj), vPat, vVisit) )
                return 0;
        }
        else
        {
            if ( Cec5_ObjFan0IsImpliedValue( pObj, 0 ) )
            {
                if ( !Cec5_ManGeneratePatterns_rec(p, pFan0, Gia_ObjFaninC0(pObj), vPat, vVisit) )
                    return 0;
            }
            else if ( Cec5_ObjFan1IsImpliedValue( pObj, 0 ) )
            {
                if ( !Cec5_ManGeneratePatterns_rec(p, pFan1, Gia_ObjFaninC1(pObj), vPat, vVisit) )
                    return 0;
            }
            else if ( Cec5_ObjFan0IsImpliedValue( pObj, 1 ) )
            {
                if ( !Cec5_ManGeneratePatterns_rec(p, pFan1, Gia_ObjFaninC1(pObj), vPat, vVisit) )
                    return 0;
            }
            else if ( Cec5_ObjFan1IsImpliedValue( pObj, 1 ) )
            {
                if ( !Cec5_ManGeneratePatterns_rec(p, pFan0, Gia_ObjFaninC0(pObj), vPat, vVisit) )
                    return 0;
            }
            else if ( Abc_Random(0) & 1 )
            {
                if ( !Cec5_ManGeneratePatterns_rec(p, pFan1, Gia_ObjFaninC1(pObj), vPat, vVisit) )
                    return 0;
            }
            else
            {
                if ( !Cec5_ManGeneratePatterns_rec(p, pFan0, Gia_ObjFaninC0(pObj), vPat, vVisit) )
                    return 0;
            }
        }
        assert( Cec5_ObjFan0HasValue(pObj, 0) || Cec5_ObjFan1HasValue(pObj, 0) );
        return 1;
    }
}
int Cec5_ManGeneratePatternOne( Gia_Man_t * p, int iRepr, int iReprVal, int iCand, int iCandVal, Vec_Int_t * vPat, Vec_Int_t * vVisit )
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
    Res = (!iRepr || Cec5_ManGeneratePatterns_rec(p, Gia_ManObj(p, iRepr), iReprVal, vPat, vVisit)) && Cec5_ManGeneratePatterns_rec(p, Gia_ManObj(p, iCand), iCandVal, vPat, vVisit);
    Gia_ManForEachObjVec( vVisit, p, pObj, k )
        pObj->fMark0 = pObj->fMark1 = 0;
    return Res;
}
void Cec5_ManCandIterStart( Cec5_Man_t * p )
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
int Cec5_ManCandIterNext( Cec5_Man_t * p )
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
int Cec5_ManGeneratePatterns( Cec5_Man_t * p )
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
        if ( (iCand = Cec5_ManCandIterNext(p)) )
        {
            int iRepr    = Gia_ObjRepr( p->pAig, iCand );
            int iCandVal = Gia_ManObj(p->pAig, iCand)->fPhase;
            int iReprVal = Gia_ManObj(p->pAig, iRepr)->fPhase;
            int Res = Cec5_ManGeneratePatternOne( p->pAig, iRepr,  iReprVal, iCand, !iCandVal, p->vPat, p->vVisit );
            if ( !Res )
                Res = Cec5_ManGeneratePatternOne( p->pAig, iRepr, !iReprVal, iCand,  iCandVal, p->vPat, p->vVisit );
            if ( Res )
            {
                int Ret = Cec5_ManPackAddPattern( p->pAig, p->vPat, 1 );
                if ( p->pAig->vPats )
                {
                    Vec_IntPush( p->pAig->vPats, Vec_IntSize(p->vPat)+2 );
                    Vec_IntAppend( p->pAig->vPats, p->vPat );
                    Vec_IntPush( p->pAig->vPats, -1 );
                }
                //Vec_IntPushTwo( p->vDisprPairs, iRepr, iCand );
                Packs += Ret;
                if ( 0 == (Ret % (64 * p->pAig->nSimWords / p->simBatchFactor)) )
                    break;
                if ( ++CountPat == 8 * 64 * p->pAig->nSimWords )
                    break;
                //Cec5_ManCexVerify( p->pAig, iRepr, iCand, iReprVal ^ iCandVal );
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
void Cec5_ManSatSolverRecycle( Cec5_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    //printf( "Solver size = %d.\n", sat_solver_varnum(p->pSat) );
    if( p->adaRecycle && p->adaRecycle < p->nConflicts[2][2] )
        return;
    p->nRecycles++;
    p->nCallsSince = 0;
    sat_solver_reset( p->pSat );
    // clean mapping of AigIds into SatIds
    Gia_ManForEachObjVec( &p->pNew->vSuppVars, p->pNew, pObj, i )
        Cec5_ObjCleanSatId( p->pNew, pObj );
    Vec_IntClear( &p->pNew->vSuppVars  );  // AigIds for which SatId is defined
    Vec_IntClear( &p->pNew->vCopiesTwo );  // pairs (CiAigId, SatId)
    Vec_IntClear( &p->pNew->vVarMap    );  // mapping of SatId into AigId
}
void Cec5_ManLoadInstance( Cec5_Man_t * p, int iObj0, int iObj1, int * piVar0, int * piVar1 ){
    int iVar0 = Cec5_ObjGetCnfVar( p, iObj0 );
    int iVar1 = Cec5_ObjGetCnfVar( p, iObj1 );
    if( p->pPars->jType > 0 )
    {
        int nlim = p->approxLim;
        bmcg2_sat_solver_markapprox( p->pSat, iVar0, iVar1, nlim );
    }

    * piVar0 = iVar0;
    * piVar1 = iVar1;
}

int Cec5_ManSolveTwo( Cec5_Man_t * p, int iObj0, int iObj1, int fPhase, int * pfEasy, int fVerbose, int fEffort )
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
        Cec5_ManSatSolverRecycle( p );
    // add more logic to the solver
    if ( !iObj0 && Cec5_ObjSatId(p->pNew, Gia_ManConst0(p->pNew)) == -1 )
        Cec5_ObjSetSatId( p->pNew, Gia_ManConst0(p->pNew), sat_solver_addvar(p->pSat) );
    clk = Abc_Clock();
    Cec5_ManLoadInstance( p, iObj0, iObj1, &iVar0, &iVar1);
    p->timeCnf += Abc_Clock() - clk;
    // perform solving
    Lits[0] = Abc_Var2Lit(iVar0, 1);
    Lits[1] = Abc_Var2Lit(iVar1, fPhase);
    sat_solver_set_conflict_budget( p->pSat, nBTLimit );
    nConfBeg = sat_solver_conflictnum( p->pSat );
    status = sat_solver_solve( p->pSat, Lits, 2 );
    nConfEnd = sat_solver_conflictnum( p->pSat );
    assert( nConfEnd >= nConfBeg );
    p->nConflicts[2][2]  = Abc_MaxInt(p->nConflicts[2][2], nConfEnd - nConfBeg);
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
                p->nConflicts[1][2]  = Abc_MaxInt(p->nConflicts[1][2], UnsatConflicts[2]);
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
        p->nConflicts[2][2]  = Abc_MaxInt(p->nConflicts[2][2], nConfEnd - nConfBeg);
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
void Cec5_FlushCache2Pattern( Cec5_Man_t * p ){
    int j, iLit, nWrite = 0;
    int iPatsOld = p->pAig->iPatsPi;
    j = 0;
    p->pAig->iPatsPi -- ;
    while( j < p->vPiPatsCache->nSize ){
        if( -1 < (iLit = p->vPiPatsCache->pArray[j++]) ){
            Cec5_ObjSimSetInputBit( p->pAig, Abc_Lit2Var(iLit), Abc_LitIsCompl(iLit) );
            continue;
        }
        p->pAig->iPatsPi -- ;
        nWrite ++ ;
    }
    p->pAig->iPatsPi += nWrite+1;
    assert( iPatsOld == p->pAig->iPatsPi );
    p->vPiPatsCache->nSize = 0;
}
void Cec5_ClearCexMarks( Cec5_Man_t * p ){
    Vec_IntFill( p->vCexStamps, Gia_ManObjNum(p->pAig), 0 );
    Vec_BitFill( p->vCexSite  , Gia_ManObjNum(p->pAig), 0 );
}
void Cec5_ManCheckGlobalSim( Cec5_Man_t * p){
    int iPatTop = p->pAig->iPatsPi;
    if ( (iPatTop % (sizeof(word) * 8 * p->pAig->nSimWords / p->simBatchFactor)) == 0
     || iPatTop == (int)sizeof(word) * 8 * p->pAig->nSimWords - 2 )
    {
        abctime clk2 = Abc_Clock();
        Cec5_FlushCache2Pattern(p);
        //p->simBound = iPatTop / (sizeof(word) * 8 * p->pAig->nSimWords / p->simBatchFactor)+2;//p->simTravId * p->LocalBatchSize / (sizeof(word) * 8<<3)+1;
        p->simBound = iPatTop / (sizeof(word) * 8) + ((iPatTop % (sizeof(word) * 8)) ? 1: 0);
        //if( iPatTop == sizeof(word) * 8 * p->pAig->nSimWords - 2 )
        //    p->simBound += 1;
        //printf("re-sim %5d %5d bound %5d\n", p->pAig->iPatsPi, p->simTravId, p->simBound );
        Cec5_ManSimulate( p->pAig, p );
        p->simBound = p->pPars->nWords;
        //printf( "FasterSmall = %d.  FasterBig = %d.\n", p->nFaster[0], p->nFaster[1] );
        p->nFaster[0] = p->nFaster[1] = 0;
        //if ( p->nSatSat && p->nSatSat % 100 == 0 )
            //Cec5_ManPrintStats( p->pAig, p->pPars, p, 0 );
        Cec5_ClearCexMarks(p);
        if( iPatTop == (int)sizeof(word) * 8 * p->pAig->nSimWords - 2 ){
            Cec5_ManPrintStats( p->pAig, p->pPars, p, 0 );
            p->pAig->iPatsPi = 0;
            p->simTravId = 0;
            p->simGlobalTop = 0;
        } else {
            //assert( 0 == iPatTop % (sizeof(word) * 8 * p->pAig->nSimWords) );
            p->pAig->iPatsPi = iPatTop;
            p->simGlobalTop = iPatTop / (sizeof(word) * 8 );
        }
        Vec_WrdFill( p->pAig->vSimsPi, Vec_WrdSize(p->pAig->vSimsPi), 0 );
        p->timeResimGlo += Abc_Clock() - clk2;
    }
}
int Cec5_ManSweepNode( Cec5_Man_t * p, int iObj, int iRepr )
{
    abctime clk = Abc_Clock();
    int i, IdAig, IdSat, status, fEasy, RetValue = 1;
    Gia_Obj_t * pObj = Gia_ManObj( p->pAig, iObj );
    Gia_Obj_t * pRepr = Gia_ManObj( p->pAig, iRepr );
    int fCompl = Abc_LitIsCompl(pObj->Value) ^ Abc_LitIsCompl(pRepr->Value) ^ pObj->fPhase ^ pRepr->fPhase;
    int fEffort = p->vCoDrivers ? Vec_BitEntry(p->vCoDrivers, iObj) || Vec_BitEntry(p->vCoDrivers, iRepr) : 0;
    status = Cec5_ManSolveTwo( p, Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value), fCompl, &fEasy, p->pPars->fVerbose, fEffort );
    if ( status == GLUCOSE_SAT )
    {
        int iLit;
        Vec_BitWriteEntry( p->vCexSite, iObj, 1 );
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
//        Vec_IntForEachEntry( p->vPat, iLit, i )
//            Cec5_ObjSimSetInputBit( p->pAig, Abc_Lit2Var(iLit), Abc_LitIsCompl(iLit) );
        Vec_IntForEachEntry( p->vPat, iLit, i )
            Vec_IntPush( p->vPiPatsCache, iLit );
        Vec_IntPush( p->vPiPatsCache, -1 );
        if ( p->pAig->vPats )
        {
            Vec_IntPush( p->pAig->vPats, Vec_IntSize(p->vPat)+2 );
            Vec_IntAppend( p->pAig->vPats, p->vPat );
            Vec_IntPush( p->pAig->vPats, -1 );
        }
        //Cec5_ManPackAddPattern( p->pAig, p->vPat, 0 );
        //assert( iPatsOld + 1 == p->pAig->iPatsPi );
        if ( fEasy )
            p->timeSatSat0 += Abc_Clock() - clk;
        else
            p->timeSatSat += Abc_Clock() - clk;
        RetValue = 0;

        p->simTravId = p->pAig->iPatsPi / p->LocalBatchSize;
        if( (p->pAig->iPatsPi % p->LocalBatchSize) == 0 || 1 == p->LocalBatchSize )
            Cec5_FlushCache2Pattern(p);

        // this is not needed, but we keep it here anyway, because it takes very little time
        //Cec5_ManVerify( p->pNew, Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value), fCompl, p->pSat );
        // resimulated once in a while
        //if ( p->pAig->iPatsPi == 64 * p->pAig->nSimWords - 2 )
        Cec5_ManCheckGlobalSim(p);
    }
    else if ( status == GLUCOSE_UNSAT )
    {
        //printf( "Proved: %d == %d.\n", Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value) );
        p->nSatUnsat++;
        pObj->Value = Abc_LitNotCond( pRepr->Value, fCompl );
        assert( !Gia_ObjProved( p->pAig, iObj ) );
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
        Gia_ObjSetFailed( p->pAig, iObj );
        Vec_BitWriteEntry( p->vFails, iObj, 1 );
        //if ( iRepr )
        //Vec_BitWriteEntry( p->vFails, iRepr, 1 );
        p->timeSatUndec += Abc_Clock() - clk;
        RetValue = 2;
    }
    return RetValue;
}
Gia_Obj_t * Cec5_ManFindRepr( Gia_Man_t * p, Cec5_Man_t * pMan, int iObj )
{
    abctime clk = Abc_Clock();
    Gia_Obj_t * pRepr = NULL;
    int iMem, iRepr;
    assert( Gia_ObjHasRepr(p, iObj) );
    assert( !Gia_ObjProved(p, iObj) );
    iRepr = Gia_ObjRepr(p, iObj);
    assert( iRepr != iObj );
    assert( !Gia_ObjProved(p, iRepr) );

    pMan->simBound = pMan->simTravId * pMan->LocalBatchSize / (sizeof(word)<<3)+1;
    assert( pMan->simBound <= pMan->pPars->nWords );

    if( (Vec_BitEntry( pMan->vCexSite, iObj ) | Vec_BitEntry( pMan->vCexSite, iRepr ))
        ||(Vec_IntEntry( pMan->vCexStamps, iObj ) != Vec_IntEntry( pMan->vCexStamps, iRepr )) ){
        Cec5_ManSimulate_rec( p, pMan, iObj );
        Cec5_ManSimulate_rec( p, pMan, iRepr );
    }
    if ( Cec5_ObjSimEqual(p, iObj, iRepr) )
    {
        pMan->timeResimLoc += Abc_Clock() - clk;
        pRepr = Gia_ManObj(p, iRepr);
        goto finalize;
    }
    Gia_ClassForEachObj1( p, iRepr, iMem )
    {
        if ( iObj == iMem )
            break;
        assert(iMem<iObj);
        if ( Gia_ObjProved(p, iMem) || Gia_ObjFailed(p, iMem) )
            continue;
        if( Vec_IntEntry( pMan->vCexStamps, iObj ) != Vec_IntEntry( pMan->vCexStamps, iMem ) ){
            Cec5_ManSimulate_rec( p, pMan, iMem );
            Cec5_ManSimulate_rec( p, pMan, iObj );
        }
        if ( Cec5_ObjSimEqual(p, iObj, iMem) )
        {
            pMan->nFaster[0]++;
            pMan->timeResimLoc += Abc_Clock() - clk;
            pRepr = Gia_ManObj(p, iMem);
            goto finalize;
        }
    }
    pMan->nFaster[1]++;
    pMan->timeResimLoc += Abc_Clock() - clk;
finalize:
    pMan->simBound = pMan->pPars->nWords;
    return pRepr;
}
void Cec5_ManExtend( Cec5_Man_t * pMan, CbsP_Man_t * pCbs ){
    while( pMan->pNew->vCopies2.nSize < Gia_ManObjNum(pMan->pNew) ){
        Vec_IntPush( &pMan->pNew->vCopies2, -1 );
        Vec_BitPush( pMan->vFails, 0 );
        if( pCbs )
            Vec_IntPush( pCbs->vValue, ~0 );
    }
}

int Cec5_ManSweepNodeCbs( Cec5_Man_t * p, CbsP_Man_t * pCbs, int iObj, int iRepr, int fTagFail );
int Cec5_ManPerformSweeping( Gia_Man_t * p, Cec_ParFra_t * pPars, Gia_Man_t ** ppNew, int fSimOnly, int fCbs, int approxLim, int subBatchSz, int adaRecycle )
{
    extern void Gia_ManRemoveWrongChoices( Gia_Man_t * p );
    Gia_Obj_t * pObj, * pRepr; 
    CbsP_Man_t * pCbs = NULL;
    int i, fSimulate = 1;
    int fPrep = 0;
    int AccuSat = 0;
    int * vMerged;
    int go_back = -1;

    Cec5_Man_t * pMan = Cec5_ManCreate( p, pPars ); 
    pMan->approxLim = approxLim;
    if( pMan->simBatchFactor != subBatchSz ){
        printf("overwrite default batch size: from %3d to %3d\n", pMan->simBatchFactor, subBatchSz);
        pMan->simBatchFactor = subBatchSz;
    }
    if( pMan->adaRecycle != adaRecycle ){
        printf("overwrite default adaptive recycle: from %3d to %3d\n", pMan->adaRecycle, adaRecycle);
        pMan->adaRecycle = adaRecycle;
    }
    if ( pPars->fVerbose )
        printf( "Solver type = %d. Simulate %d words in %d rounds. SAT with %d confs. Recycle after %d SAT calls.\n", 
            pPars->jType, pPars->nWords, pPars->nRounds, pPars->nBTLimit, pPars->nCallsRecycle );

    if( fPrep )
        pMan->fEec = 1;


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
                p->pCexSeq = Cec5_ManDeriveCex( p, i, -1 );
                goto finalize;
            }
    }

    // simulate one round and create classes
    Cec5_ManSimAlloc( p, pPars->nWords, 0 );
    Cec5_ManSimulateCis( p );
    Cec5_ManSimulate( p, pMan );
    if ( pPars->fCheckMiter && !Cec5_ManSimulateCos(p) ) // cex detected
        goto finalize;
    if ( pPars->fVerbose )
        Cec5_ManPrintStats( p, pPars, pMan, 1 );

    // perform simulation
    for ( i = 0; i < pPars->nRounds; i++ )
    {
        Cec5_ManSimulateCis( p );
        Cec5_ManSimulate( p, pMan );
        if ( pPars->fCheckMiter && !Cec5_ManSimulateCos(p) ) // cex detected
            goto finalize;
        if ( i && i % (pPars->nRounds / 5) == 0 && pPars->fVerbose )
            Cec5_ManPrintStats( p, pPars, pMan, 1 );
    }
    if ( fSimOnly )
        goto finalize;
    

    // perform additional simulation
    Cec5_ManCandIterStart( pMan );
    for ( i = 0; fSimulate && i < pPars->nGenIters; i++ )
    {
        Cec5_ManSimulateCis( p );
        fSimulate = Cec5_ManGeneratePatterns( pMan );
        Cec5_ManSimulate( p, pMan );
        if ( pPars->fCheckMiter && !Cec5_ManSimulateCos(p) ) // cex detected
            goto finalize;
        if ( i && i % 5 == 0 && pPars->fVerbose )
            Cec5_ManPrintStats( p, pPars, pMan, 1 );
        if( pMan->nSatSat - AccuSat < p->nSimWords * (int)sizeof(word) * 8 )
            break;

        AccuSat = pMan->nSatSat;
    }
    if ( i && i % 5 && pPars->fVerbose )
        Cec5_ManPrintStats( p, pPars, pMan, 1 );

    vMerged = ABC_FALLOC(int, Gia_ManObjNum(p)); // refinement may move non-repr merge around . record the true merged performed

    p->iPatsPi = 0;
    Vec_WrdFill( p->vSimsPi, Vec_WrdSize(p->vSimsPi), 0 );
    pMan->nSatSat = 0;
    pMan->pNew = Cec5_ManStartNew( p );

    if( fCbs ){
        //Gia_ManCreateRefs( pMan->pNew );
        pMan->pNew->pRefs = ABC_CALLOC( int, Gia_ManObjNum(p) );
        Gia_ManCleanMark0( pMan->pNew );
        Gia_ManCleanMark1( pMan->pNew );
        Gia_ManFillValue ( pMan->pNew );
        pCbs = CbsP_ManAlloc( p );
        pCbs->pAig = pMan->pNew;
        pCbs->Pars.nBTLimit    = 100;//pPars->nBTLimit;
        pCbs->Pars.nJustLimit  = 100;
        pCbs->Pars.nJscanLimit = 100;
        pCbs->Pars.nRscanLimit = 100;
        pCbs->Pars.nPropLimit  = 100;

        if( pPars->fVerbose ){
            printf("cbs: clim = %4d jlim = %4d\n"
                , pCbs->Pars.nBTLimit
                , pCbs->Pars.nJustLimit );
        }
    }
    
    //Gia_ManForEachAnd( p, pObj, i )
    i = 0;
resume:
    for( ; i < Gia_ManObjNum(p) && (pObj = Gia_ManObj(p,i)); i ++ )
    {
        int status = 2;
        Gia_Obj_t * pObjNew; 
        if ( !Gia_ObjIsAnd(pObj) )
            continue;
        
        Vec_BitWriteEntry( pMan->vCexSite, i
            , Vec_BitEntry( pMan->vCexSite, Gia_ObjFaninId0(pObj,i) ) 
            | Vec_BitEntry( pMan->vCexSite, Gia_ObjFaninId1(pObj,i) ) );

        if ( Gia_ObjFailed(p,i) )
            continue;
        pMan->nAndNodes++;
        if ( Gia_ObjProved(p, i) ){
            pRepr = Gia_ManObj( p, vMerged[i] );
            pObj->Value = Abc_LitNotCond( pRepr->Value, pObj->fPhase ^ pRepr->fPhase ); // upate in case repr rehashed due to fanin its backtrace
            continue;
        }

        if ( Gia_ObjIsXor(pObj) )
            pObj->Value = Gia_ManHashXorReal( pMan->pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else
            pObj->Value = Gia_ManHashAnd( pMan->pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );

        Cec5_ManExtend( pMan, pCbs );

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
            pRepr = Cec5_ManFindRepr( p, pMan, i );
            if ( pRepr == NULL )
                continue;
        }
        if ( Abc_Lit2Var(pObj->Value) == Abc_Lit2Var(pRepr->Value) )
        {
            assert( (pObj->Value ^ pRepr->Value) == (pObj->fPhase ^ pRepr->fPhase) );
            vMerged[i] = Gia_ObjId(p, pRepr);
            Gia_ObjSetProved( p, i );
            if ( Gia_ObjId(p, pRepr) == 0 )
                pMan->iLastConst = i;

            continue;
        }

        if ( fCbs && (status = Cec5_ManSweepNodeCbs(pMan, pCbs, i, Gia_ObjId(p, pRepr), 0)) && Gia_ObjProved(p, i) ){
            vMerged[i] = Gia_ObjId(p, pRepr);
            pObj->Value = Abc_LitNotCond( pRepr->Value, pObj->fPhase ^ pRepr->fPhase );
        }

        if ( 2 == status && (status = Cec5_ManSweepNode(pMan, i, Gia_ObjId(p, pRepr))) && Gia_ObjProved(p, i) ){
            vMerged[i] = Gia_ObjId(p, pRepr);
            pObj->Value = Abc_LitNotCond( pRepr->Value, pObj->fPhase ^ pRepr->fPhase );
        }

        if( 0 == status && -1 == go_back )
            go_back = i;

        if( 0 == pMan->nPatterns || 0!=status || pMan->nPatterns % (64 * p->nSimWords-2) )
            continue;

        if( -1 < go_back ){
            //printf("go back to %6d from %6d\n", go_back, i );
            pMan->nAndNodes -= i-go_back;
            i = go_back-1, go_back = -1;
        }
    }
    if ( p->iPatsPi > 0 )
    {
        abctime clk2 = Abc_Clock();
        Cec5_FlushCache2Pattern(pMan);
        Cec5_ManSimulate( p, pMan );
        p->iPatsPi = 0;
        pMan->simTravId = 0;
        pMan->simGlobalTop = 0;
        Cec5_ClearCexMarks(pMan);
        pMan->timeResimGlo += Abc_Clock() - clk2;
        if( -1 < go_back ){
            i = go_back - 1;
            go_back = -1;
            goto resume;
        }
    }

    ABC_FREE(vMerged);

    if ( pPars->fVerbose )
        Cec5_ManPrintStats( p, pPars, pMan, 0 );
    if ( ppNew )
    {
        Gia_ManForEachCo( p, pObj, i )
            pObj->Value = Gia_ManAppendCo( pMan->pNew, Gia_ObjFanin0Copy(pObj) );
        *ppNew = Gia_ManCleanup( pMan->pNew );
    }

    if( fCbs ){
        if ( pPars->fVerbose ){
            CbsP_ManSatPrintStats( pCbs );
            CbsP_PrintRecord(&pCbs->Pars);
        }
    }
finalize:
    if ( pPars->fVerbose )
        printf( "SAT calls = %d:  P = %d (0=%d a=%.2f m=%d)  D = %d (0=%d a=%.2f m=%d)  F = %d   Sim = %d  Recyc = %d  Xor = %.2f %%\n", 
            pMan->nSatUnsat + pMan->nSatSat + pMan->nSatUndec, 
            pMan->nSatUnsat, pMan->nConflicts[1][0], (float)pMan->nConflicts[1][1]/Abc_MaxInt(1, pMan->nSatUnsat-pMan->nConflicts[1][0]), pMan->nConflicts[1][2],
            pMan->nSatSat,   pMan->nConflicts[0][0], (float)pMan->nConflicts[0][1]/Abc_MaxInt(1, pMan->nSatSat  -pMan->nConflicts[0][0]), pMan->nConflicts[0][2],  
            pMan->nSatUndec,  
            pMan->nSimulates, pMan->nRecycles, 100.0*pMan->nGates[1]/Abc_MaxInt(1, pMan->nGates[0]+pMan->nGates[1]) );
    Cec5_ManDestroy( pMan );
    if( pCbs )
        CbsP_ManStop(pCbs);
    //Gia_ManStaticFanoutStop( p );
    //Gia_ManEquivPrintClasses( p, 1, 0 );
    Gia_ManRemoveWrongChoices( p );
    return p->pCexSeq ? 0 : 1;
}
Gia_Man_t * Cec5_ManSimulateTest( Gia_Man_t * p, Cec_ParFra_t * pPars, int fCbs, int approxLim, int subBatchSz, int adaRecycle )
{
    Gia_Man_t * pNew = NULL;
    Cec5_ManPerformSweeping( p, pPars, &pNew, 0, fCbs, approxLim, subBatchSz, adaRecycle );
    if ( pNew == NULL )
        pNew = Gia_ManDup( p );
    return pNew;
}



int Cec5_ManSolveTwoCbs( Cec5_Man_t * p, CbsP_Man_t * pCbs, int iObj0, int iObj1, int fPhase, int * pfEasy, int fVerbose, int fEffort )
{
//    abctime clk;
//    int nBTLimit = fEffort ? p->pPars->nBTLimitPo : (Vec_BitEntry(p->vFails, iObj0) || Vec_BitEntry(p->vFails, iObj1)) ? Abc_MaxInt(1, p->pPars->nBTLimit/10) : p->pPars->nBTLimit;
    Gia_Obj_t * pRepr, * pObj;
    int nConfEnd, nConfBeg, status;//, iVar0, iVar1, Lits[2];
    int UnsatConflicts[3] = {0};
    //printf( "%d ", nBTLimit );
    if ( iObj1 <  iObj0 ) 
         iObj1 ^= iObj0, iObj0 ^= iObj1, iObj1 ^= iObj0;
    assert( iObj0 < iObj1 );
    pRepr = Gia_ManObj( p->pNew, iObj0 );
    pObj  = Gia_ManObj( p->pNew, iObj1 );
    *pfEasy = 0;
    // check if SAT solver needs recycling
    p->nCallsSince++; 
//    if ( p->nCallsSince > p->pPars->nCallsRecycle && 
//         Vec_IntSize(&p->pNew->vSuppVars) > p->pPars->nSatVarMax && p->pPars->nSatVarMax )
//        Cec5_ManSatSolverRecycle( p );
//    // add more logic to the solver
//    if ( !iObj0 && Cec5_ObjSatId(p->pNew, Gia_ManConst0(p->pNew)) == -1 )
//        Cec5_ObjSetSatId( p->pNew, Gia_ManConst0(p->pNew), sat_solver_addvar(p->pSat) );
//    clk = Abc_Clock();
//    iVar0 = Cec5_ObjGetCnfVar( p, iObj0 );
//    iVar1 = Cec5_ObjGetCnfVar( p, iObj1 );
//    if( p->pPars->jType > 0 )
//    {
//        sat_solver_start_new_round( p->pSat );
//        sat_solver_mark_cone( p->pSat, Cec5_ObjSatId(p->pNew, Gia_ManObj(p->pNew, iObj0)) );
//        sat_solver_mark_cone( p->pSat, Cec5_ObjSatId(p->pNew, Gia_ManObj(p->pNew, iObj1)) );
//    }
//    p->timeCnf += Abc_Clock() - clk;
    // perform solving
//    Lits[0] = Abc_Var2Lit(iVar0, 1);
//    Lits[1] = Abc_Var2Lit(iVar1, fPhase);
//    sat_solver_set_conflict_budget( p->pSat, nBTLimit );
//    nConfBeg = sat_solver_conflictnum( p->pSat );
//    status = sat_solver_solve( p->pSat, Lits, 2 );
//    nConfEnd = sat_solver_conflictnum( p->pSat );
    nConfBeg = 0;
    if( !Gia_ObjIsConst0(pRepr) )
        status = CbsP_ManSolve2(pCbs,Gia_Not(pObj),Gia_NotCond(pRepr, fPhase));
    else
        status = CbsP_ManSolve2(pCbs,Gia_NotCond(pObj,fPhase),NULL);
    nConfEnd = pCbs->Pars.nBTThis;
    assert( nConfEnd >= nConfBeg );
    if ( fVerbose )
    {
        if ( status == CBS_SAT ) 
        {
            p->nConflicts[0][0] += nConfEnd == nConfBeg;
            p->nConflicts[0][1] += nConfEnd -  nConfBeg;
            p->nConflicts[0][2]  = Abc_MaxInt(p->nConflicts[0][2], nConfEnd - nConfBeg);
            *pfEasy = nConfEnd == nConfBeg;
        }
        else if ( status == CBS_UNSAT ) 
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
    if ( status == CBS_UNSAT && iObj0 > 0 )
    {
        //Lits[0] = Abc_Var2Lit(iVar0, 0);
        //Lits[1] = Abc_Var2Lit(iVar1, !fPhase);
        //sat_solver_set_conflict_budget( p->pSat, nBTLimit );
        //nConfBeg = sat_solver_conflictnum( p->pSat );
        //status = sat_solver_solve( p->pSat, Lits, 2 );
        //nConfEnd = sat_solver_conflictnum( p->pSat );
        nConfBeg = 0;
        status = CbsP_ManSolve2(pCbs,pObj,Gia_NotCond(pRepr,!fPhase));
        nConfEnd = pCbs->Pars.nBTThis;
        assert( nConfEnd >= nConfBeg );
        if ( fVerbose )
        {
            if ( status == CBS_SAT ) 
            {
                p->nConflicts[0][0] += nConfEnd == nConfBeg;
                p->nConflicts[0][1] += nConfEnd -  nConfBeg;
                p->nConflicts[0][2]  = Abc_MaxInt(p->nConflicts[0][2], nConfEnd - nConfBeg);
                *pfEasy = nConfEnd == nConfBeg;
            }
            else if ( status == CBS_UNSAT ) 
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


int Cec5_ManSweepNodeCbs( Cec5_Man_t * p, CbsP_Man_t * pCbs, int iObj, int iRepr, int fTagFail )
{
    extern int CbsP_ManSolve2( CbsP_Man_t * p, Gia_Obj_t * pObj, Gia_Obj_t * pObj2 );
    abctime clk = Abc_Clock();
    int i, status, fEasy, RetValue = 1;
    Gia_Obj_t * pObj = Gia_ManObj( p->pAig, iObj );
    Gia_Obj_t * pRepr = Gia_ManObj( p->pAig, iRepr );
    int fCompl = Abc_LitIsCompl(pObj->Value) ^ Abc_LitIsCompl(pRepr->Value) ^ pObj->fPhase ^ pRepr->fPhase;
    //int fCompl = pObj->fPhase ^ pRepr->fPhase;
    int fEffort = p->vCoDrivers ? Vec_BitEntry(p->vCoDrivers, iObj) || Vec_BitEntry(p->vCoDrivers, iRepr) : 0;
    //status = Cec5_ManSolveTwo( p, Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value), fCompl, &fEasy, p->pPars->fVerbose, fEffort );

    status = Cec5_ManSolveTwoCbs( p, pCbs, Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value), fCompl, &fEasy, p->pPars->fVerbose, fEffort );
    //status = CbsP_ManSolve2(pCbs,pObj,pRepr);
    if ( status == CBS_SAT )
    {
        int iLit;
        Vec_BitWriteEntry( p->vCexSite, iObj, 1 );
//        int iPatsOld = p->pAig->iPatsPi;
//        //printf( "Disproved: %d == %d.\n", Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value) );
        p->nSatSat++;
        p->nPatterns++;
        Vec_IntClear( p->vPat );
        Vec_IntForEachEntry( pCbs->vModel, iLit, i )
            Vec_IntPush( p->vPat, Abc_LitNot(iLit) );
        assert( p->pAig->iPatsPi >= 0 && p->pAig->iPatsPi < 64 * p->pAig->nSimWords - 1 );
        p->pAig->iPatsPi++;
        Vec_IntForEachEntry( p->vPat, iLit, i )
            Vec_IntPush( p->vPiPatsCache, iLit );
        Vec_IntPush( p->vPiPatsCache, -1 );

        if ( fEasy )
            p->timeSatSat0 += Abc_Clock() - clk;
        else
            p->timeSatSat += Abc_Clock() - clk;
        RetValue = 0;

        p->simTravId = p->pAig->iPatsPi / p->LocalBatchSize;
        if( (p->pAig->iPatsPi % p->LocalBatchSize) == 0 || 1 == p->LocalBatchSize )
            Cec5_FlushCache2Pattern(p);

//        // this is not needed, but we keep it here anyway, because it takes very little time
//        //Cec5_ManVerify( p->pNew, Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value), fCompl, p->pSat );
//        // resimulated once in a while
        Cec5_ManCheckGlobalSim(p);
    }
    else if ( status == CBS_UNSAT )
    {
        //printf( "Proved: %d == %d.\n", Abc_Lit2Var(pRepr->Value), Abc_Lit2Var(pObj->Value) );
        p->nSatUnsat++;
        pObj->Value = Abc_LitNotCond( pRepr->Value, fCompl );
        assert( !Gia_ObjProved( p->pAig, iObj ) );
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
        if( fTagFail ){
            p->nSatUndec++;
            assert( status == CBS_UNDEC );
            Gia_ObjSetFailed( p->pAig, iObj );
            Gia_ObjSetFailed( p->pAig, iRepr );
            Vec_BitWriteEntry( p->vFails, iObj, 1 );
            p->timeSatUndec += Abc_Clock() - clk;
        }
        //if ( iRepr )
        //Vec_BitWriteEntry( p->vFails, iRepr, 1 );
        RetValue = 2;
    }
    return RetValue;
}
Gia_Man_t * Cec5_ManSimulateTest3( Gia_Man_t * p, int nBTLimit, int fVerbose )
{
    int fCbs = 1, approxLim = 600, subBatchSz = 1, adaRecycle = 500;
    Gia_Man_t * pNew = NULL;
    Cec_ParFra_t ParsFra, * pPars = &ParsFra;
    Cec5_ManSetParams( pPars );
    pPars->fVerbose = fVerbose;
    pPars->nBTLimit = nBTLimit;
    Cec5_ManPerformSweeping( p, pPars, &pNew, 0, fCbs, approxLim, subBatchSz, adaRecycle );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

