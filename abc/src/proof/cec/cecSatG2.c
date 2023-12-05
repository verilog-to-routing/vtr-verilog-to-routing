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
int Cec4_ManPerformSweeping( Gia_Man_t * p, Cec_ParFra_t * pPars, Gia_Man_t ** ppNew, int fSimOnly )
{
    Cec4_Man_t * pMan = Cec4_ManCreate( p, pPars ); 
    Gia_Obj_t * pObj, * pRepr; 
    int i, fSimulate = 1;
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
        if ( Abc_Lit2Var(pObj->Value) == Abc_Lit2Var(pRepr->Value) )
        {
            assert( (pObj->Value ^ pRepr->Value) == (pObj->fPhase ^ pRepr->fPhase) );
            Gia_ObjSetProved( p, i );
            if ( Gia_ObjId(p, pRepr) == 0 )
                pMan->iLastConst = i;
            continue;
        }
        if ( Cec4_ManSweepNode(pMan, i, Gia_ObjId(p, pRepr)) && Gia_ObjProved(p, i) )
            pObj->Value = Abc_LitNotCond( pRepr->Value, pObj->fPhase ^ pRepr->fPhase );
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
    Cec4_ManDestroy( pMan );
    //Gia_ManStaticFanoutStop( p );
    //Gia_ManEquivPrintClasses( p, 1, 0 );
    if ( ppNew && *ppNew == NULL )
        *ppNew = Gia_ManDup(p);
    Gia_ManRemoveWrongChoices( p );
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

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

