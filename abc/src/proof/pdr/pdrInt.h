/**CFile****************************************************************

  FileName    [pdrInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Property driven reachability.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 20, 2010.]

  Revision    [$Id: pdrInt.h,v 1.00 2010/11/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__sat__pdr__pdrInt_h
#define ABC__sat__pdr__pdrInt_h

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/saig/saig.h"
#include "misc/vec/vecWec.h"
#include "sat/cnf/cnf.h"
#include "pdr.h" 
#include "misc/hash/hashInt.h"
#include "aig/gia/giaAig.h"

//#define PDR_USE_SATOKO 1

#ifndef PDR_USE_SATOKO
    #include "sat/bsat/satSolver.h"
#else
    #include "sat/satoko/satoko.h"
    #define l_Undef  0
    #define l_True   1
    #define l_False -1
    #define sat_solver                       satoko_t
    #define sat_solver_new                   satoko_create
    #define sat_solver_delete                satoko_destroy
    #define zsat_solver_new_seed(s)          satoko_create()
    #define zsat_solver_restart_seed(s,a)    satoko_reset(s)
    #define sat_solver_nvars                 satoko_varnum
    #define sat_solver_setnvars              satoko_setnvars
    #define sat_solver_addclause(s,b,e)      satoko_add_clause(s,b,e-b)
    #define sat_solver_final                 satoko_final_conflict
    #define sat_solver_solve(s,b,e,c,x,y,z)  satoko_solve_assumptions_limit(s,b,e-b,(int)c)
    #define sat_solver_var_value             satoko_read_cex_varvalue
    #define sat_solver_set_runtime_limit     satoko_set_runtime_limit
    #define sat_solver_set_runid             satoko_set_runid           
    #define sat_solver_set_stop_func         satoko_set_stop_func          
    #define sat_solver_compress(s)             
#endif

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////
             
////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Txs_Man_t_  Txs_Man_t;
typedef struct Txs3_Man_t_ Txs3_Man_t;

typedef struct Pdr_Set_t_ Pdr_Set_t;
struct Pdr_Set_t_
{
    word        Sign;      // signature
    int         nRefs;     // ref counter
    int         nTotal;    // total literals
    int         nLits;     // num flop literals
    int         Lits[0];
};

typedef struct Pdr_Obl_t_ Pdr_Obl_t;
struct Pdr_Obl_t_
{
    int         iFrame;    // time frame
    int         prio;      // priority
    int         nRefs;     // reference counter
    Pdr_Set_t * pState;    // state cube
    Pdr_Obl_t * pNext;     // next one
    Pdr_Obl_t * pLink;     // queue link
};

typedef struct Pdr_Man_t_ Pdr_Man_t;
struct Pdr_Man_t_
{
    // input problem
    Pdr_Par_t * pPars;     // parameters
    Aig_Man_t * pAig;      // user's AIG
    Gia_Man_t * pGia;      // user's AIG
    // static CNF representation
    Cnf_Man_t * pCnfMan;   // CNF manager
    Cnf_Dat_t * pCnf1;     // CNF for this AIG
    Vec_Int_t * vVar2Reg;  // mapping of SAT var into registers
    // dynamic CNF representation
    Cnf_Dat_t * pCnf2;     // CNF for this AIG
    Vec_Int_t * pvId2Vars; // for each used ObjId, maps frame into SAT var
    Vec_Ptr_t   vVar2Ids;  // for each used frame, maps SAT var into ObjId
    Vec_Wec_t * vVLits;    // CNF literals
    // data representation
    int         iOutCur;   // current output
    int         nPrioShift;// priority shift
    Vec_Ptr_t * vCexes;    // counter-examples for each output
    Vec_Ptr_t * vSolvers;  // SAT solvers
    Vec_Vec_t * vClauses;  // clauses by timeframe
    Pdr_Obl_t * pQueue;    // proof obligations
    int *       pOrder;    // ordering of the lits
    Vec_Int_t * vActVars;  // the counter of activation variables
    int         iUseFrame; // the first used frame
    int         nAbsFlops; // the number of flops used
    Vec_Int_t * vAbsFlops; // flops currently used
    Vec_Int_t * vMapFf2Ppi;
    Vec_Int_t * vMapPpi2Ff;
    int         nCexes;
    int         nCexesTotal;
    // terminary simulation
    Txs3_Man_t * pTxs3;      
    // internal use
    Vec_Int_t * vPrio;     // priority flops
    Vec_Int_t * vLits;     // array of literals
    Vec_Int_t * vCiObjs;   // cone leaves
    Vec_Int_t * vCoObjs;   // cone roots
    Vec_Int_t * vCiVals;   // cone leaf values
    Vec_Int_t * vCoVals;   // cone root values
    Vec_Int_t * vNodes;    // cone nodes
    Vec_Int_t * vUndo;     // cone undos
    Vec_Int_t * vVisits;   // intermediate
    Vec_Int_t * vCi2Rem;   // CIs to be removed
    Vec_Int_t * vRes;      // final result
    abctime *   pTime4Outs;// timeout per output
    Vec_Ptr_t * vInfCubes; // infinity clauses/cubes
    // statistics
    int         nBlocks;   // the number of times blockState was called
    int         nObligs;   // the number of proof obligations derived
    int         nCubes;    // the number of cubes derived
    int         nCalls;    // the number of SAT calls
    int         nCallsS;   // the number of SAT calls (sat)
    int         nCallsU;   // the number of SAT calls (unsat)
    int         nStarts;   // the number of SAT solver restarts
    int         nFrames;   // frames explored
    int         nCasesSS;
    int         nCasesSU;
    int         nCasesUS;
    int         nCasesUU;
    int         nQueCur;
    int         nQueMax;
    int         nQueLim;
    int         nXsimRuns;
    int         nXsimLits;
    // runtime
    abctime     timeToStop;
    abctime     timeToStopOne;
    // time stats
    abctime     tSat;
    abctime     tSatSat;
    abctime     tSatUnsat;
    abctime     tGeneral;
    abctime     tPush;
    abctime     tTsim;
    abctime     tContain;
    abctime     tCnf;
    abctime     tAbs;
    abctime     tTotal;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline sat_solver * Pdr_ManSolver( Pdr_Man_t * p, int k )  { return (sat_solver *)Vec_PtrEntry(p->vSolvers, k); }

static inline abctime      Pdr_ManTimeLimit( Pdr_Man_t * p )
{
    if ( p->timeToStop == 0 )
        return p->timeToStopOne;
    if ( p->timeToStopOne == 0 )
        return p->timeToStop;
    if ( p->timeToStop < p->timeToStopOne )
        return p->timeToStop;
    return p->timeToStopOne;
}

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== pdrCnf.c ==========================================================*/
extern int             Pdr_ObjSatVar( Pdr_Man_t * p, int k, int Pol, Aig_Obj_t * pObj );
extern int             Pdr_ObjRegNum( Pdr_Man_t * p, int k, int iSatVar );
extern int             Pdr_ManFreeVar( Pdr_Man_t * p, int k );
extern sat_solver *    Pdr_ManNewSolver( sat_solver * pSat, Pdr_Man_t * p, int k, int fInit );
/*=== pdrCore.c ==========================================================*/
extern int             Pdr_ManCheckContainment( Pdr_Man_t * p, int k, Pdr_Set_t * pSet );
/*=== pdrInv.c ==========================================================*/
extern Vec_Int_t *     Pdr_ManCountFlopsInv( Pdr_Man_t * p );
extern void            Pdr_ManPrintProgress( Pdr_Man_t * p, int fClose, abctime Time );
extern void            Pdr_ManPrintClauses( Pdr_Man_t * p, int kStart );
extern void            Pdr_ManDumpClauses( Pdr_Man_t * p, char * pFileName, int fProved );
extern Vec_Str_t *     Pdr_ManDumpString( Pdr_Man_t * p );
extern void            Pdr_ManReportInvariant( Pdr_Man_t * p );
extern void            Pdr_ManVerifyInvariant( Pdr_Man_t * p );
extern Vec_Int_t *     Pdr_ManDeriveInfinityClauses( Pdr_Man_t * p, int fReduce );
/*=== pdrMan.c ==========================================================*/
extern Pdr_Man_t *     Pdr_ManStart( Aig_Man_t * pAig, Pdr_Par_t * pPars, Vec_Int_t * vPrioInit );
extern void            Pdr_ManStop( Pdr_Man_t * p );
extern Abc_Cex_t *     Pdr_ManDeriveCex( Pdr_Man_t * p );
extern Abc_Cex_t *     Pdr_ManDeriveCexAbs( Pdr_Man_t * p );
/*=== pdrSat.c ==========================================================*/
extern sat_solver *    Pdr_ManCreateSolver( Pdr_Man_t * p, int k );
extern sat_solver *    Pdr_ManFetchSolver( Pdr_Man_t * p, int k );
extern void            Pdr_ManSetPropertyOutput( Pdr_Man_t * p, int k );
extern Vec_Int_t *     Pdr_ManCubeToLits( Pdr_Man_t * p, int k, Pdr_Set_t * pCube, int fCompl, int fNext );
extern Vec_Int_t *     Pdr_ManLitsToCube( Pdr_Man_t * p, int k, int * pArray, int nArray );
extern void            Pdr_ManSolverAddClause( Pdr_Man_t * p, int k, Pdr_Set_t * pCube );
extern void            Pdr_ManCollectValues( Pdr_Man_t * p, int k, Vec_Int_t * vObjIds, Vec_Int_t * vValues );
extern int             Pdr_ManCheckCubeCs( Pdr_Man_t * p, int k, Pdr_Set_t * pCube );
extern int             Pdr_ManCheckCube( Pdr_Man_t * p, int k, Pdr_Set_t * pCube, Pdr_Set_t ** ppPred, int nConfLimit, int fTryConf, int fUseLit );
/*=== pdrTsim.c ==========================================================*/
extern Pdr_Set_t *     Pdr_ManTernarySim( Pdr_Man_t * p, int k, Pdr_Set_t * pCube );
/*=== pdrTsim2.c ==========================================================*/
extern Txs_Man_t *     Txs_ManStart( Pdr_Man_t * pMan, Aig_Man_t * pAig, Vec_Int_t * vPrio );
extern void            Txs_ManStop( Txs_Man_t * );
extern Pdr_Set_t *     Txs_ManTernarySim( Txs_Man_t * p, int k, Pdr_Set_t * pCube );
/*=== pdrTsim3.c ==========================================================*/
extern Txs3_Man_t *    Txs3_ManStart( Pdr_Man_t * pMan, Aig_Man_t * pAig, Vec_Int_t * vPrio );
extern void            Txs3_ManStop( Txs3_Man_t * );
extern Pdr_Set_t *     Txs3_ManTernarySim( Txs3_Man_t * p, int k, Pdr_Set_t * pCube );
/*=== pdrUtil.c ==========================================================*/
extern Pdr_Set_t *     Pdr_SetAlloc( int nSize );
extern Pdr_Set_t *     Pdr_SetCreate( Vec_Int_t * vLits, Vec_Int_t * vPiLits );
extern Pdr_Set_t *     Pdr_SetCreateFrom( Pdr_Set_t * pSet, int iRemove );
extern Pdr_Set_t *     Pdr_SetCreateSubset( Pdr_Set_t * pSet, int * pLits, int nLits );
extern Pdr_Set_t *     Pdr_SetDup( Pdr_Set_t * pSet );
extern Pdr_Set_t *     Pdr_SetRef( Pdr_Set_t * p );
extern void            Pdr_SetDeref( Pdr_Set_t * p );
extern Pdr_Set_t *     ZPdr_SetIntersection( Pdr_Set_t * p1, Pdr_Set_t * p2, Hash_Int_t * keep );
extern int             Pdr_SetContains( Pdr_Set_t * pOld, Pdr_Set_t * pNew );
extern int             Pdr_SetContainsSimple( Pdr_Set_t * pOld, Pdr_Set_t * pNew );
extern int             Pdr_SetIsInit( Pdr_Set_t * p, int iRemove );
extern int             ZPdr_SetIsInit( Pdr_Set_t * p );
extern void            Pdr_SetPrint( FILE * pFile, Pdr_Set_t * p, int nRegs, Vec_Int_t * vFlopCounts );
extern void            ZPdr_SetPrint( Pdr_Set_t * p );
extern void            Pdr_SetPrintStr( Vec_Str_t * vStr, Pdr_Set_t * p, int nRegs, Vec_Int_t * vFlopCounts );
extern int             Pdr_SetCompare( Pdr_Set_t ** pp1, Pdr_Set_t ** pp2 );
extern Pdr_Obl_t *     Pdr_OblStart( int k, int prio, Pdr_Set_t * pState, Pdr_Obl_t * pNext );
extern Pdr_Obl_t *     Pdr_OblRef( Pdr_Obl_t * p );
extern void            Pdr_OblDeref( Pdr_Obl_t * p );
extern int             Pdr_QueueIsEmpty( Pdr_Man_t * p );
extern Pdr_Obl_t *     Pdr_QueueHead( Pdr_Man_t * p );
extern Pdr_Obl_t *     Pdr_QueuePop( Pdr_Man_t * p );
extern void            Pdr_QueueClean( Pdr_Man_t * p );
extern void            Pdr_QueuePush( Pdr_Man_t * p, Pdr_Obl_t * pObl );
extern void            Pdr_QueuePrint( Pdr_Man_t * p );
extern void            Pdr_QueueStop( Pdr_Man_t * p );

ABC_NAMESPACE_HEADER_END


#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

