/**CFile****************************************************************

  FileName    [ivyFraig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [Functional reduction of AIGs]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006.]

  Revision    [$Id: ivyFraig.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include <math.h>

#include "sat/bsat/satSolver.h"
#include "misc/extra/extra.h"
#include "ivy.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Ivy_FraigMan_t_        Ivy_FraigMan_t;
typedef struct Ivy_FraigSim_t_        Ivy_FraigSim_t;
typedef struct Ivy_FraigList_t_       Ivy_FraigList_t;

struct Ivy_FraigList_t_
{
    Ivy_Obj_t *         pHead;
    Ivy_Obj_t *         pTail;
    int                 nItems;
};

struct Ivy_FraigSim_t_
{
    int                 Type;
    Ivy_FraigSim_t *    pNext;
    Ivy_FraigSim_t *    pFanin0;
    Ivy_FraigSim_t *    pFanin1;
    unsigned            pData[0];
};

struct Ivy_FraigMan_t_
{
    // general info 
    Ivy_FraigParams_t * pParams;        // various parameters
    // temporary backtrack limits because "ABC_INT64_T" cannot be defined in Ivy_FraigParams_t ...
    ABC_INT64_T              nBTLimitGlobal; // global limit on the number of backtracks
    ABC_INT64_T              nInsLimitGlobal;// global limit on the number of clause inspects
    // AIG manager
    Ivy_Man_t *         pManAig;        // the starting AIG manager
    Ivy_Man_t *         pManFraig;      // the final AIG manager
    // simulation information
    int                 nSimWords;      // the number of words
    char *              pSimWords;      // the simulation info
    Ivy_FraigSim_t *    pSimStart;      // the list of simulation info for internal nodes
    // counter example storage
    int                 nPatWords;      // the number of words in the counter example
    unsigned *          pPatWords;      // the counter example
    int *               pPatScores;     // the scores of each pattern
    // equivalence classes 
    Ivy_FraigList_t     lClasses;       // equivalence classes
    Ivy_FraigList_t     lCand;          // candidatates
    int                 nPairs;         // the number of pairs of nodes
    // equivalence checking
    sat_solver *        pSat;           // SAT solver
    int                 nSatVars;       // the number of variables currently used
    Vec_Ptr_t *         vPiVars;        // the PIs of the cone used 
    // other 
    ProgressBar *       pProgress;
    // statistics
    int                 nSimRounds;
    int                 nNodesMiter;
    int                 nClassesZero;
    int                 nClassesBeg;
    int                 nClassesEnd;
    int                 nPairsBeg;
    int                 nPairsEnd;
    int                 nSatCalls;
    int                 nSatCallsSat;
    int                 nSatCallsUnsat;
    int                 nSatProof;
    int                 nSatFails;
    int                 nSatFailsReal;
    // runtime
    abctime             timeSim;
    abctime             timeTrav;
    abctime             timeSat;
    abctime             timeSatUnsat;
    abctime             timeSatSat;
    abctime             timeSatFail;
    abctime             timeRef;
    abctime             timeTotal;
    abctime             time1;
    abctime             time2;
};

typedef struct Prove_ParamsStruct_t_      Prove_Params_t;
struct Prove_ParamsStruct_t_
{
    // general parameters
    int     fUseFraiging;          // enables fraiging
    int     fUseRewriting;         // enables rewriting
    int     fUseBdds;              // enables BDD construction when other methods fail
    int     fVerbose;              // prints verbose stats
    // iterations
    int     nItersMax;             // the number of iterations
    // mitering 
    int     nMiteringLimitStart;   // starting mitering limit
    float   nMiteringLimitMulti;   // multiplicative coefficient to increase the limit in each iteration
    // rewriting 
    int     nRewritingLimitStart;  // the number of rewriting iterations
    float   nRewritingLimitMulti;  // multiplicative coefficient to increase the limit in each iteration
    // fraiging 
    int     nFraigingLimitStart;   // starting backtrack(conflict) limit
    float   nFraigingLimitMulti;   // multiplicative coefficient to increase the limit in each iteration
    // last-gasp BDD construction
    int     nBddSizeLimit;         // the number of BDD nodes when construction is aborted
    int     fBddReorder;           // enables dynamic BDD variable reordering
    // last-gasp mitering
    int     nMiteringLimitLast;    // final mitering limit
    // global SAT solver limits
    ABC_INT64_T  nTotalBacktrackLimit;  // global limit on the number of backtracks
    ABC_INT64_T  nTotalInspectLimit;    // global limit on the number of clause inspects
    // global resources applied
    ABC_INT64_T  nTotalBacktracksMade;  // the total number of backtracks made
    ABC_INT64_T  nTotalInspectsMade;    // the total number of inspects made
};

static inline Ivy_FraigSim_t * Ivy_ObjSim( Ivy_Obj_t * pObj )                            { return (Ivy_FraigSim_t *)pObj->pFanout;  }
static inline Ivy_Obj_t * Ivy_ObjClassNodeLast( Ivy_Obj_t * pObj )                       { return pObj->pNextFan0;  }
static inline Ivy_Obj_t * Ivy_ObjClassNodeRepr( Ivy_Obj_t * pObj )                       { return pObj->pNextFan0;  }
static inline Ivy_Obj_t * Ivy_ObjClassNodeNext( Ivy_Obj_t * pObj )                       { return pObj->pNextFan1;  }
static inline Ivy_Obj_t * Ivy_ObjNodeHashNext( Ivy_Obj_t * pObj )                        { return pObj->pPrevFan0;  }
static inline Ivy_Obj_t * Ivy_ObjEquivListNext( Ivy_Obj_t * pObj )                       { return pObj->pPrevFan0;  }
static inline Ivy_Obj_t * Ivy_ObjEquivListPrev( Ivy_Obj_t * pObj )                       { return pObj->pPrevFan1;  }
static inline Ivy_Obj_t * Ivy_ObjFraig( Ivy_Obj_t * pObj )                               { return pObj->pEquiv;     }
static inline int         Ivy_ObjSatNum( Ivy_Obj_t * pObj )                              { return (int)(ABC_PTRUINT_T)pObj->pNextFan0;         }
static inline Vec_Ptr_t * Ivy_ObjFaninVec( Ivy_Obj_t * pObj )                            { return (Vec_Ptr_t *)pObj->pNextFan1; }

static inline void        Ivy_ObjSetSim( Ivy_Obj_t * pObj, Ivy_FraigSim_t * pSim )       { pObj->pFanout = (Ivy_Obj_t *)pSim; }
static inline void        Ivy_ObjSetClassNodeLast( Ivy_Obj_t * pObj, Ivy_Obj_t * pLast ) { pObj->pNextFan0 = pLast; }
static inline void        Ivy_ObjSetClassNodeRepr( Ivy_Obj_t * pObj, Ivy_Obj_t * pRepr ) { pObj->pNextFan0 = pRepr; }
static inline void        Ivy_ObjSetClassNodeNext( Ivy_Obj_t * pObj, Ivy_Obj_t * pNext ) { pObj->pNextFan1 = pNext; }
static inline void        Ivy_ObjSetNodeHashNext( Ivy_Obj_t * pObj, Ivy_Obj_t * pNext )  { pObj->pPrevFan0 = pNext; }
static inline void        Ivy_ObjSetEquivListNext( Ivy_Obj_t * pObj, Ivy_Obj_t * pNext ) { pObj->pPrevFan0 = pNext; }
static inline void        Ivy_ObjSetEquivListPrev( Ivy_Obj_t * pObj, Ivy_Obj_t * pPrev ) { pObj->pPrevFan1 = pPrev; }
static inline void        Ivy_ObjSetFraig( Ivy_Obj_t * pObj, Ivy_Obj_t * pNode )         { pObj->pEquiv    = pNode; }
static inline void        Ivy_ObjSetSatNum( Ivy_Obj_t * pObj, int Num )                  { pObj->pNextFan0 = (Ivy_Obj_t *)(ABC_PTRUINT_T)Num;     }
static inline void        Ivy_ObjSetFaninVec( Ivy_Obj_t * pObj, Vec_Ptr_t * vFanins )    { pObj->pNextFan1 = (Ivy_Obj_t *)vFanins; }

static inline unsigned    Ivy_ObjRandomSim()                       { return (rand() << 24) ^ (rand() << 12) ^ rand(); }

// iterate through equivalence classes
#define Ivy_FraigForEachEquivClass( pList, pEnt )                 \
    for ( pEnt = pList;                                           \
          pEnt;                                                   \
          pEnt = Ivy_ObjEquivListNext(pEnt) )
#define Ivy_FraigForEachEquivClassSafe( pList, pEnt, pEnt2 )      \
    for ( pEnt = pList,                                           \
          pEnt2 = pEnt? Ivy_ObjEquivListNext(pEnt): NULL;         \
          pEnt;                                                   \
          pEnt = pEnt2,                                           \
          pEnt2 = pEnt? Ivy_ObjEquivListNext(pEnt): NULL )
// iterate through nodes in one class
#define Ivy_FraigForEachClassNode( pClass, pEnt )                 \
    for ( pEnt = pClass;                                          \
          pEnt;                                                   \
          pEnt = Ivy_ObjClassNodeNext(pEnt) )
// iterate through nodes in the hash table
#define Ivy_FraigForEachBinNode( pBin, pEnt )                     \
    for ( pEnt = pBin;                                            \
          pEnt;                                                   \
          pEnt = Ivy_ObjNodeHashNext(pEnt) )

static Ivy_FraigMan_t * Ivy_FraigStart( Ivy_Man_t * pManAig, Ivy_FraigParams_t * pParams );
static Ivy_FraigMan_t * Ivy_FraigStartSimple( Ivy_Man_t * pManAig, Ivy_FraigParams_t * pParams );
static Ivy_Man_t *   Ivy_FraigPerform_int( Ivy_Man_t * pManAig, Ivy_FraigParams_t * pParams, ABC_INT64_T nBTLimitGlobal, ABC_INT64_T nInsLimitGlobal, ABC_INT64_T * pnSatConfs, ABC_INT64_T * pnSatInspects );
static void          Ivy_FraigPrint( Ivy_FraigMan_t * p );
static void          Ivy_FraigStop( Ivy_FraigMan_t * p );
static void          Ivy_FraigSimulate( Ivy_FraigMan_t * p );
static void          Ivy_FraigSweep( Ivy_FraigMan_t * p );
static Ivy_Obj_t *   Ivy_FraigAnd( Ivy_FraigMan_t * p, Ivy_Obj_t * pObjOld );
static int           Ivy_FraigNodesAreEquiv( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj0, Ivy_Obj_t * pObj1 );
static int           Ivy_FraigNodeIsConst( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj );
static void          Ivy_FraigNodeAddToSolver( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj0, Ivy_Obj_t * pObj1 );
static int           Ivy_FraigSetActivityFactors( Ivy_FraigMan_t * p, Ivy_Obj_t * pOld, Ivy_Obj_t * pNew );
static void          Ivy_FraigAddToPatScores( Ivy_FraigMan_t * p, Ivy_Obj_t * pClass, Ivy_Obj_t * pClassNew );
static int           Ivy_FraigMiterStatus( Ivy_Man_t * pMan );
static void          Ivy_FraigMiterProve( Ivy_FraigMan_t * p );
static void          Ivy_FraigMiterPrint( Ivy_Man_t * pNtk, char * pString, abctime clk, int fVerbose );
static int *         Ivy_FraigCreateModel( Ivy_FraigMan_t * p );

static int Ivy_FraigNodesAreEquivBdd( Ivy_Obj_t * pObj1, Ivy_Obj_t * pObj2 );
static int Ivy_FraigCheckCone( Ivy_FraigMan_t * pGlo, Ivy_Man_t * p, Ivy_Obj_t * pObj1, Ivy_Obj_t * pObj2, int nConfLimit );

static ABC_INT64_T s_nBTLimitGlobal = 0;
static ABC_INT64_T s_nInsLimitGlobal = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sets the default solving parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigParamsDefault( Ivy_FraigParams_t * pParams )
{
    memset( pParams, 0, sizeof(Ivy_FraigParams_t) );
    pParams->nSimWords        =      32;  // the number of words in the simulation info
    pParams->dSimSatur        =   0.005;  // the ratio of refined classes when saturation is reached
    pParams->fPatScores       =       0;  // enables simulation pattern scoring
    pParams->MaxScore         =      25;  // max score after which resimulation is used
    pParams->fDoSparse        =       1;  // skips sparse functions
//    pParams->dActConeRatio    =    0.05;  // the ratio of cone to be bumped
//    pParams->dActConeBumpMax  =     5.0;  // the largest bump of activity
    pParams->dActConeRatio    =     0.3;  // the ratio of cone to be bumped
    pParams->dActConeBumpMax  =    10.0;  // the largest bump of activity

    pParams->nBTLimitNode     =     100;  // conflict limit at a node
    pParams->nBTLimitMiter    =  500000;  // conflict limit at an output
//    pParams->nBTLimitGlobal   =       0;  // conflict limit global
//    pParams->nInsLimitGlobal  =       0;  // inspection limit global
}

/**Function*************************************************************

  Synopsis    [Performs combinational equivalence checking for the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigProve( Ivy_Man_t ** ppManAig, void * pPars )
{
    Prove_Params_t * pParams = (Prove_Params_t *)pPars;
    Ivy_FraigParams_t Params, * pIvyParams = &Params; 
    Ivy_Man_t * pManAig, * pManTemp;
    int RetValue, nIter;
    abctime clk;//, Counter;
    ABC_INT64_T nSatConfs = 0, nSatInspects = 0;

    // start the network and parameters
    pManAig = *ppManAig;
    Ivy_FraigParamsDefault( pIvyParams );
    pIvyParams->fVerbose = pParams->fVerbose;
    pIvyParams->fProve = 1;

    if ( pParams->fVerbose )
    {
        printf( "RESOURCE LIMITS: Iterations = %d. Rewriting = %s. Fraiging = %s.\n",
            pParams->nItersMax, pParams->fUseRewriting? "yes":"no", pParams->fUseFraiging? "yes":"no" );
        printf( "Miter = %d (%3.1f).  Rwr = %d (%3.1f).  Fraig = %d (%3.1f).  Last = %d.\n", 
            pParams->nMiteringLimitStart,  pParams->nMiteringLimitMulti, 
            pParams->nRewritingLimitStart, pParams->nRewritingLimitMulti,
            pParams->nFraigingLimitStart,  pParams->nFraigingLimitMulti, pParams->nMiteringLimitLast );
    }

    // if SAT only, solve without iteration
    if ( !pParams->fUseRewriting && !pParams->fUseFraiging )
    {
        clk = Abc_Clock();
        pIvyParams->nBTLimitMiter = pParams->nMiteringLimitLast / Ivy_ManPoNum(pManAig);
        pManAig = Ivy_FraigMiter( pManTemp = pManAig, pIvyParams );  Ivy_ManStop( pManTemp );
        RetValue = Ivy_FraigMiterStatus( pManAig );
        Ivy_FraigMiterPrint( pManAig, "SAT solving", clk, pParams->fVerbose );
        *ppManAig = pManAig;
        return RetValue;
    }

    if ( Ivy_ManNodeNum(pManAig) < 500 )
    {
        // run the first mitering
        clk = Abc_Clock();
        pIvyParams->nBTLimitMiter = pParams->nMiteringLimitStart / Ivy_ManPoNum(pManAig);
        pManAig = Ivy_FraigMiter( pManTemp = pManAig, pIvyParams );  Ivy_ManStop( pManTemp );
        RetValue = Ivy_FraigMiterStatus( pManAig );
        Ivy_FraigMiterPrint( pManAig, "SAT solving", clk, pParams->fVerbose );
        if ( RetValue >= 0 )
        {
            *ppManAig = pManAig;
            return RetValue;
        }
    }

    // check the current resource limits
    RetValue = -1;
    for ( nIter = 0; nIter < pParams->nItersMax; nIter++ )
    {
        if ( pParams->fVerbose )
        {
            printf( "ITERATION %2d : Confs = %6d. FraigBTL = %3d. \n", nIter+1, 
                 (int)(pParams->nMiteringLimitStart * pow(pParams->nMiteringLimitMulti,nIter)), 
                 (int)(pParams->nFraigingLimitStart * pow(pParams->nFraigingLimitMulti,nIter)) );
            fflush( stdout );
        }

        // try rewriting
        if ( pParams->fUseRewriting )
        { // bug in Ivy_NodeFindCutsAll() when leaves are identical!
/*
            clk = Abc_Clock();
            Counter = (int)(pParams->nRewritingLimitStart * pow(pParams->nRewritingLimitMulti,nIter));
            pManAig = Ivy_ManRwsat( pManAig, 0 );  
            RetValue = Ivy_FraigMiterStatus( pManAig );
            Ivy_FraigMiterPrint( pManAig, "Rewriting  ", clk, pParams->fVerbose );
*/
        }
        if ( RetValue >= 0 )
            break;

        // catch the situation when ref pattern detects the bug
        RetValue = Ivy_FraigMiterStatus( pManAig );
        if ( RetValue >= 0 )
            break;

        // try fraiging followed by mitering
        if ( pParams->fUseFraiging )
        {
            clk = Abc_Clock();
            pIvyParams->nBTLimitNode  = (int)(pParams->nFraigingLimitStart * pow(pParams->nFraigingLimitMulti,nIter));
            pIvyParams->nBTLimitMiter = 1 + (int)(pParams->nMiteringLimitStart * pow(pParams->nMiteringLimitMulti,nIter)) / Ivy_ManPoNum(pManAig);
            pManAig = Ivy_FraigPerform_int( pManTemp = pManAig, pIvyParams, pParams->nTotalBacktrackLimit, pParams->nTotalInspectLimit, &nSatConfs, &nSatInspects );  Ivy_ManStop( pManTemp );
            RetValue = Ivy_FraigMiterStatus( pManAig );
            Ivy_FraigMiterPrint( pManAig, "Fraiging   ", clk, pParams->fVerbose );
        }
        if ( RetValue >= 0 )
            break;

        // add to the number of backtracks and inspects
        pParams->nTotalBacktracksMade += nSatConfs;
        pParams->nTotalInspectsMade   += nSatInspects;
        // check if global resource limit is reached
        if ( (pParams->nTotalBacktrackLimit && pParams->nTotalBacktracksMade >= pParams->nTotalBacktrackLimit) ||
             (pParams->nTotalInspectLimit   && pParams->nTotalInspectsMade   >= pParams->nTotalInspectLimit) )
        {
            printf( "Reached global limit on conflicts/inspects. Quitting.\n" );
            *ppManAig = pManAig;
            return -1;
        }
    }    
/*
    if ( RetValue < 0 )
    {
        if ( pParams->fVerbose )
        {
            printf( "Attempting SAT with conflict limit %d ...\n", pParams->nMiteringLimitLast );
            fflush( stdout );
        }
        clk = Abc_Clock();
        pIvyParams->nBTLimitMiter = pParams->nMiteringLimitLast / Ivy_ManPoNum(pManAig);
        if ( pParams->nTotalBacktrackLimit )
            s_nBTLimitGlobal  = pParams->nTotalBacktrackLimit - pParams->nTotalBacktracksMade;
        if ( pParams->nTotalInspectLimit )
            s_nInsLimitGlobal = pParams->nTotalInspectLimit -   pParams->nTotalInspectsMade;        
        pManAig = Ivy_FraigMiter( pManTemp = pManAig, pIvyParams );  Ivy_ManStop( pManTemp );
        s_nBTLimitGlobal  = 0;
        s_nInsLimitGlobal = 0;        
        RetValue = Ivy_FraigMiterStatus( pManAig );
        Ivy_FraigMiterPrint( pManAig, "SAT solving", clk, pParams->fVerbose );
        // make sure that the sover never returns "undecided" when infinite resource limits are set
        if( RetValue == -1 && pParams->nTotalInspectLimit == 0 &&
            pParams->nTotalBacktrackLimit == 0 )
        {
            extern void Prove_ParamsPrint( Prove_Params_t * pParams );
            Prove_ParamsPrint( pParams );
            printf("ERROR: ABC has returned \"undecided\" in spite of no limits...\n");
            exit(1);
        }
    }
*/
    // assign the model if it was proved by rewriting (const 1 miter)
    if ( RetValue == 0 && pManAig->pData == NULL )
    {
        pManAig->pData = ABC_ALLOC( int, Ivy_ManPiNum(pManAig) );
        memset( pManAig->pData, 0, sizeof(int) * Ivy_ManPiNum(pManAig) );
    }
    *ppManAig = pManAig;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Man_t * Ivy_FraigPerform_int( Ivy_Man_t * pManAig, Ivy_FraigParams_t * pParams, ABC_INT64_T nBTLimitGlobal, ABC_INT64_T nInsLimitGlobal, ABC_INT64_T * pnSatConfs, ABC_INT64_T * pnSatInspects )
{ 
    Ivy_FraigMan_t * p;
    Ivy_Man_t * pManAigNew;
    abctime clk;
    if ( Ivy_ManNodeNum(pManAig) == 0 )
        return Ivy_ManDup(pManAig);
clk = Abc_Clock();
    assert( Ivy_ManLatchNum(pManAig) == 0 );
    p = Ivy_FraigStart( pManAig, pParams );
    // set global limits
    p->nBTLimitGlobal  = nBTLimitGlobal;
    p->nInsLimitGlobal = nInsLimitGlobal; 
    
    Ivy_FraigSimulate( p );
    Ivy_FraigSweep( p );
    pManAigNew = p->pManFraig;
p->timeTotal = Abc_Clock() - clk;
    if ( pnSatConfs )
        *pnSatConfs = p->pSat? p->pSat->stats.conflicts : 0;
    if ( pnSatInspects )
        *pnSatInspects = p->pSat? p->pSat->stats.inspects : 0;
    Ivy_FraigStop( p );
    return pManAigNew;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Man_t * Ivy_FraigPerform( Ivy_Man_t * pManAig, Ivy_FraigParams_t * pParams )
{
    Ivy_FraigMan_t * p;
    Ivy_Man_t * pManAigNew;
    abctime clk;
    if ( Ivy_ManNodeNum(pManAig) == 0 )
        return Ivy_ManDup(pManAig);
clk = Abc_Clock();
    assert( Ivy_ManLatchNum(pManAig) == 0 );
    p = Ivy_FraigStart( pManAig, pParams );
    Ivy_FraigSimulate( p );
    Ivy_FraigSweep( p );
    pManAigNew = p->pManFraig;
p->timeTotal = Abc_Clock() - clk;
    Ivy_FraigStop( p );
    return pManAigNew;
}

/**Function*************************************************************

  Synopsis    [Applies brute-force SAT to the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Man_t * Ivy_FraigMiter( Ivy_Man_t * pManAig, Ivy_FraigParams_t * pParams )
{
    Ivy_FraigMan_t * p;
    Ivy_Man_t * pManAigNew;
    Ivy_Obj_t * pObj;
    int i;
    abctime clk;
clk = Abc_Clock();
    assert( Ivy_ManLatchNum(pManAig) == 0 );
    p = Ivy_FraigStartSimple( pManAig, pParams );
    // set global limits
    p->nBTLimitGlobal  = s_nBTLimitGlobal;
    p->nInsLimitGlobal = s_nInsLimitGlobal; 
    // duplicate internal nodes
    Ivy_ManForEachNode( p->pManAig, pObj, i )
        pObj->pEquiv = Ivy_And( p->pManFraig, Ivy_ObjChild0Equiv(pObj), Ivy_ObjChild1Equiv(pObj) );
    // try to prove each output of the miter
    Ivy_FraigMiterProve( p );
    // add the POs
    Ivy_ManForEachPo( p->pManAig, pObj, i )
        Ivy_ObjCreatePo( p->pManFraig, Ivy_ObjChild0Equiv(pObj) );
    // clean the new manager
    Ivy_ManForEachObj( p->pManFraig, pObj, i )
    {
        if ( Ivy_ObjFaninVec(pObj) )
            Vec_PtrFree( Ivy_ObjFaninVec(pObj) );
        pObj->pNextFan0 = pObj->pNextFan1 = NULL;
    }
    // remove dangling nodes 
    Ivy_ManCleanup( p->pManFraig );
    pManAigNew = p->pManFraig;
p->timeTotal = Abc_Clock() - clk;

//printf( "Final nodes = %6d. ", Ivy_ManNodeNum(pManAigNew) );
//ABC_PRT( "Time", p->timeTotal );
    Ivy_FraigStop( p );
    return pManAigNew;
}

/**Function*************************************************************

  Synopsis    [Starts the fraiging manager without simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_FraigMan_t * Ivy_FraigStartSimple( Ivy_Man_t * pManAig, Ivy_FraigParams_t * pParams )
{
    Ivy_FraigMan_t * p;
    // allocat the fraiging manager
    p = ABC_ALLOC( Ivy_FraigMan_t, 1 );
    memset( p, 0, sizeof(Ivy_FraigMan_t) );
    p->pParams   = pParams;
    p->pManAig   = pManAig;
    p->pManFraig = Ivy_ManStartFrom( pManAig );
    p->vPiVars   = Vec_PtrAlloc( 100 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Starts the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_FraigMan_t * Ivy_FraigStart( Ivy_Man_t * pManAig, Ivy_FraigParams_t * pParams )
{
    Ivy_FraigMan_t * p;
    Ivy_FraigSim_t * pSims;
    Ivy_Obj_t * pObj;
    int i, k, EntrySize;
    // clean the fanout representation
    Ivy_ManForEachObj( pManAig, pObj, i )
//        pObj->pEquiv = pObj->pFanout = pObj->pNextFan0 = pObj->pNextFan1 = pObj->pPrevFan0 = pObj->pPrevFan1 = NULL;
        assert( !pObj->pEquiv && !pObj->pFanout );
    // allocat the fraiging manager
    p = ABC_ALLOC( Ivy_FraigMan_t, 1 );
    memset( p, 0, sizeof(Ivy_FraigMan_t) );
    p->pParams   = pParams;
    p->pManAig   = pManAig;
    p->pManFraig = Ivy_ManStartFrom( pManAig );
    // allocate simulation info
    p->nSimWords    = pParams->nSimWords;
//    p->pSimWords    = ABC_ALLOC( unsigned, Ivy_ManObjNum(pManAig) * p->nSimWords ); 
    EntrySize    = sizeof(Ivy_FraigSim_t) + sizeof(unsigned) * p->nSimWords;
    p->pSimWords = (char *)ABC_ALLOC( char, Ivy_ManObjNum(pManAig) * EntrySize ); 
    memset( p->pSimWords, 0, EntrySize );
    k = 0;
    Ivy_ManForEachObj( pManAig, pObj, i )
    {
        pSims = (Ivy_FraigSim_t *)(p->pSimWords + EntrySize * k++);
        pSims->pNext = NULL;
        if ( Ivy_ObjIsNode(pObj) )
        {
            if ( p->pSimStart == NULL )
                p->pSimStart = pSims;
            else
                ((Ivy_FraigSim_t *)(p->pSimWords + EntrySize * (k-2)))->pNext = pSims;
            pSims->pFanin0 = Ivy_ObjSim( Ivy_ObjFanin0(pObj) );
            pSims->pFanin1 = Ivy_ObjSim( Ivy_ObjFanin1(pObj) );
            pSims->Type = (Ivy_ObjFaninPhase(Ivy_ObjChild0(pObj)) << 2) | (Ivy_ObjFaninPhase(Ivy_ObjChild1(pObj)) << 1) | pObj->fPhase;
        }
        else
        {
            pSims->pFanin0 = NULL;
            pSims->pFanin1 = NULL;
            pSims->Type = 0;
        }
        Ivy_ObjSetSim( pObj, pSims );
    }
    assert( k == Ivy_ManObjNum(pManAig) );
    // allocate storage for sim pattern
    p->nPatWords  = Ivy_BitWordNum( Ivy_ManPiNum(pManAig) );
    p->pPatWords  = ABC_ALLOC( unsigned, p->nPatWords ); 
    p->pPatScores = ABC_ALLOC( int, 32 * p->nSimWords ); 
    p->vPiVars    = Vec_PtrAlloc( 100 );
    // set random number generator
    srand( 0xABCABC );
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigStop( Ivy_FraigMan_t * p )
{
    if ( p->pParams->fVerbose )
        Ivy_FraigPrint( p );
    if ( p->vPiVars ) Vec_PtrFree( p->vPiVars );
    if ( p->pSat ) sat_solver_delete( p->pSat );
    ABC_FREE( p->pPatScores );
    ABC_FREE( p->pPatWords );
    ABC_FREE( p->pSimWords );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Prints stats for the fraiging manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigPrint( Ivy_FraigMan_t * p )
{
    double nMemory;
    nMemory = (double)Ivy_ManObjNum(p->pManAig)*p->nSimWords*sizeof(unsigned)/(1<<20);
    printf( "SimWords = %d. Rounds = %d. Mem = %0.2f MB.  ", p->nSimWords, p->nSimRounds, nMemory );
    printf( "Classes: Beg = %d. End = %d.\n", p->nClassesBeg, p->nClassesEnd );
//    printf( "Limits: BTNode = %d. BTMiter = %d.\n", p->pParams->nBTLimitNode, p->pParams->nBTLimitMiter );
    printf( "Proof = %d. Counter-example = %d. Fail = %d. FailReal = %d. Zero = %d.\n", 
        p->nSatProof, p->nSatCallsSat, p->nSatFails, p->nSatFailsReal, p->nClassesZero );
    printf( "Final = %d. Miter = %d. Total = %d. Mux = %d. (Exor = %d.) SatVars = %d.\n", 
        Ivy_ManNodeNum(p->pManFraig), p->nNodesMiter, Ivy_ManNodeNum(p->pManAig), 0, 0, p->nSatVars );
    if ( p->pSat ) Sat_SolverPrintStats( stdout, p->pSat );
    ABC_PRT( "AIG simulation  ", p->timeSim  );
    ABC_PRT( "AIG traversal   ", p->timeTrav  );
    ABC_PRT( "SAT solving     ", p->timeSat   );
    ABC_PRT( "    Unsat       ", p->timeSatUnsat );
    ABC_PRT( "    Sat         ", p->timeSatSat   );
    ABC_PRT( "    Fail        ", p->timeSatFail  );
    ABC_PRT( "Class refining  ", p->timeRef   );
    ABC_PRT( "TOTAL RUNTIME   ", p->timeTotal );
    if ( p->time1 ) { ABC_PRT( "time1           ", p->time1 ); }
}



/**Function*************************************************************

  Synopsis    [Assigns random patterns to the PI node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeAssignRandom( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj )
{
    Ivy_FraigSim_t * pSims;
    int i;
    assert( Ivy_ObjIsPi(pObj) );
    pSims = Ivy_ObjSim(pObj);
    for ( i = 0; i < p->nSimWords; i++ )
        pSims->pData[i] = Ivy_ObjRandomSim();
}

/**Function*************************************************************

  Synopsis    [Assigns constant patterns to the PI node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeAssignConst( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj, int fConst1 )
{
    Ivy_FraigSim_t * pSims;
    int i;
    assert( Ivy_ObjIsPi(pObj) );
    pSims = Ivy_ObjSim(pObj);
    for ( i = 0; i < p->nSimWords; i++ )
        pSims->pData[i] = fConst1? ~(unsigned)0 : 0;
}

/**Function*************************************************************

  Synopsis    [Assings random simulation info for the PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigAssignRandom( Ivy_FraigMan_t * p )
{
    Ivy_Obj_t * pObj;
    int i;
    Ivy_ManForEachPi( p->pManAig, pObj, i )
        Ivy_NodeAssignRandom( p, pObj );
}

/**Function*************************************************************

  Synopsis    [Assings distance-1 simulation info for the PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigAssignDist1( Ivy_FraigMan_t * p, unsigned * pPat )
{
    Ivy_Obj_t * pObj;
    int i, Limit;
    Ivy_ManForEachPi( p->pManAig, pObj, i )
    {
        Ivy_NodeAssignConst( p, pObj, Ivy_InfoHasBit(pPat, i) );
//        printf( "%d", Ivy_InfoHasBit(pPat, i) );
    }
//    printf( "\n" );

    Limit = IVY_MIN( Ivy_ManPiNum(p->pManAig), p->nSimWords * 32 - 1 );
    for ( i = 0; i < Limit; i++ )
        Ivy_InfoXorBit( Ivy_ObjSim( Ivy_ManPi(p->pManAig,i) )->pData, i+1 );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_NodeHasZeroSim( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj )
{
    Ivy_FraigSim_t * pSims;
    int i;
    pSims = Ivy_ObjSim(pObj);
    for ( i = 0; i < p->nSimWords; i++ )
        if ( pSims->pData[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation info is composed of all zeros.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeComplementSim( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj )
{
    Ivy_FraigSim_t * pSims;
    int i;
    pSims = Ivy_ObjSim(pObj);
    for ( i = 0; i < p->nSimWords; i++ )
        pSims->pData[i] = ~pSims->pData[i];
}

/**Function*************************************************************

  Synopsis    [Returns 1 if simulation infos are equal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_NodeCompareSims( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj0, Ivy_Obj_t * pObj1 )
{
    Ivy_FraigSim_t * pSims0, * pSims1;
    int i;
    pSims0 = Ivy_ObjSim(pObj0);
    pSims1 = Ivy_ObjSim(pObj1);
    for ( i = 0; i < p->nSimWords; i++ )
        if ( pSims0->pData[i] != pSims1->pData[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeSimulateSim( Ivy_FraigMan_t * p, Ivy_FraigSim_t * pSims )
{
    unsigned * pData, * pData0, * pData1;
    int i;
    pData  = pSims->pData;
    pData0 = pSims->pFanin0->pData;
    pData1 = pSims->pFanin1->pData;
    switch( pSims->Type )
    {
    case 0:
        for ( i = 0; i < p->nSimWords; i++ )
            pData[i] = (pData0[i] & pData1[i]);
        break;
    case 1:
        for ( i = 0; i < p->nSimWords; i++ )
            pData[i] = ~(pData0[i] & pData1[i]);
        break;
    case 2:
        for ( i = 0; i < p->nSimWords; i++ )
            pData[i] = (pData0[i] & ~pData1[i]);
        break;
    case 3:
        for ( i = 0; i < p->nSimWords; i++ )
            pData[i] = (~pData0[i] | pData1[i]);
        break;
    case 4:
        for ( i = 0; i < p->nSimWords; i++ )
            pData[i] = (~pData0[i] & pData1[i]);
        break;
    case 5:
        for ( i = 0; i < p->nSimWords; i++ )
            pData[i] = (pData0[i] | ~pData1[i]);
        break;
    case 6:
        for ( i = 0; i < p->nSimWords; i++ )
            pData[i] = ~(pData0[i] | pData1[i]);
        break;
    case 7:
        for ( i = 0; i < p->nSimWords; i++ )
            pData[i] = (pData0[i] | pData1[i]);
        break;
    }
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeSimulate( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj )
{
    Ivy_FraigSim_t * pSims, * pSims0, * pSims1;
    int fCompl, fCompl0, fCompl1, i;
    assert( !Ivy_IsComplement(pObj) );
    // get hold of the simulation information
    pSims  = Ivy_ObjSim(pObj);
    pSims0 = Ivy_ObjSim(Ivy_ObjFanin0(pObj));
    pSims1 = Ivy_ObjSim(Ivy_ObjFanin1(pObj));
    // get complemented attributes of the children using their random info
    fCompl  = pObj->fPhase;
    fCompl0 = Ivy_ObjFaninPhase(Ivy_ObjChild0(pObj));
    fCompl1 = Ivy_ObjFaninPhase(Ivy_ObjChild1(pObj));
    // simulate
    if ( fCompl0 && fCompl1 )
    {
        if ( fCompl )
            for ( i = 0; i < p->nSimWords; i++ )
                pSims->pData[i] = (pSims0->pData[i] | pSims1->pData[i]);
        else
            for ( i = 0; i < p->nSimWords; i++ )
                pSims->pData[i] = ~(pSims0->pData[i] | pSims1->pData[i]);
    }
    else if ( fCompl0 && !fCompl1 )
    {
        if ( fCompl )
            for ( i = 0; i < p->nSimWords; i++ )
                pSims->pData[i] = (pSims0->pData[i] | ~pSims1->pData[i]);
        else
            for ( i = 0; i < p->nSimWords; i++ )
                pSims->pData[i] = (~pSims0->pData[i] & pSims1->pData[i]);
    }
    else if ( !fCompl0 && fCompl1 )
    {
        if ( fCompl )
            for ( i = 0; i < p->nSimWords; i++ )
                pSims->pData[i] = (~pSims0->pData[i] | pSims1->pData[i]);
        else
            for ( i = 0; i < p->nSimWords; i++ )
                pSims->pData[i] = (pSims0->pData[i] & ~pSims1->pData[i]);
    }
    else // if ( !fCompl0 && !fCompl1 )
    {
        if ( fCompl )
            for ( i = 0; i < p->nSimWords; i++ )
                pSims->pData[i] = ~(pSims0->pData[i] & pSims1->pData[i]);
        else
            for ( i = 0; i < p->nSimWords; i++ )
                pSims->pData[i] = (pSims0->pData[i] & pSims1->pData[i]);
    }
}

/**Function*************************************************************

  Synopsis    [Computes hash value using simulation info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Ivy_NodeHash( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj )
{
    static int s_FPrimes[128] = { 
        1009, 1049, 1093, 1151, 1201, 1249, 1297, 1361, 1427, 1459, 
        1499, 1559, 1607, 1657, 1709, 1759, 1823, 1877, 1933, 1997, 
        2039, 2089, 2141, 2213, 2269, 2311, 2371, 2411, 2467, 2543, 
        2609, 2663, 2699, 2741, 2797, 2851, 2909, 2969, 3037, 3089, 
        3169, 3221, 3299, 3331, 3389, 3461, 3517, 3557, 3613, 3671, 
        3719, 3779, 3847, 3907, 3943, 4013, 4073, 4129, 4201, 4243, 
        4289, 4363, 4441, 4493, 4549, 4621, 4663, 4729, 4793, 4871, 
        4933, 4973, 5021, 5087, 5153, 5227, 5281, 5351, 5417, 5471, 
        5519, 5573, 5651, 5693, 5749, 5821, 5861, 5923, 6011, 6073, 
        6131, 6199, 6257, 6301, 6353, 6397, 6481, 6563, 6619, 6689, 
        6737, 6803, 6863, 6917, 6977, 7027, 7109, 7187, 7237, 7309, 
        7393, 7477, 7523, 7561, 7607, 7681, 7727, 7817, 7877, 7933, 
        8011, 8039, 8059, 8081, 8093, 8111, 8123, 8147
    };
    Ivy_FraigSim_t * pSims;
    unsigned uHash;
    int i;
    assert( p->nSimWords <= 128 );
    uHash = 0;
    pSims  = Ivy_ObjSim(pObj);
    for ( i = 0; i < p->nSimWords; i++ )
        uHash ^= pSims->pData[i] * s_FPrimes[i];
    return uHash;
}

/**Function*************************************************************

  Synopsis    [Simulates AIG manager.]

  Description [Assumes that the PI simulation info is attached.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigSimulateOne( Ivy_FraigMan_t * p )
{
    Ivy_Obj_t * pObj;
    int i;
    abctime clk;
clk = Abc_Clock();
    Ivy_ManForEachNode( p->pManAig, pObj, i )
    {
        Ivy_NodeSimulate( p, pObj );
/*
        if ( Ivy_ObjFraig(pObj) == NULL )
            printf( "%3d --- -- %d  :  ", pObj->Id, pObj->fPhase );
        else
            printf( "%3d %3d %2d %d  :  ", pObj->Id, Ivy_Regular(Ivy_ObjFraig(pObj))->Id, Ivy_ObjSatNum(Ivy_Regular(Ivy_ObjFraig(pObj))), pObj->fPhase );
        Extra_PrintBinary( stdout, Ivy_ObjSim(pObj), 30 );
        printf( "\n" );
*/
    }
p->timeSim += Abc_Clock() - clk;
p->nSimRounds++;
}

/**Function*************************************************************

  Synopsis    [Simulates AIG manager.]

  Description [Assumes that the PI simulation info is attached.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigSimulateOneSim( Ivy_FraigMan_t * p )
{
    Ivy_FraigSim_t * pSims;
    abctime clk;
clk = Abc_Clock();
    for ( pSims = p->pSimStart; pSims; pSims = pSims->pNext )
        Ivy_NodeSimulateSim( p, pSims );
p->timeSim += Abc_Clock() - clk;
p->nSimRounds++;
}

/**Function*************************************************************

  Synopsis    [Adds one node to the equivalence class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_NodeAddToClass( Ivy_Obj_t * pClass, Ivy_Obj_t * pObj )
{
    if ( Ivy_ObjClassNodeNext(pClass) == NULL )
        Ivy_ObjSetClassNodeNext( pClass, pObj );
    else
        Ivy_ObjSetClassNodeNext( Ivy_ObjClassNodeLast(pClass), pObj );
    Ivy_ObjSetClassNodeLast( pClass, pObj );
    Ivy_ObjSetClassNodeRepr( pObj, pClass );
    Ivy_ObjSetClassNodeNext( pObj, NULL );
}

/**Function*************************************************************

  Synopsis    [Adds equivalence class to the list of classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigAddClass( Ivy_FraigList_t * pList, Ivy_Obj_t * pClass )
{
    if ( pList->pHead == NULL )
    {
        pList->pHead = pClass;
        pList->pTail = pClass;
        Ivy_ObjSetEquivListPrev( pClass, NULL );
        Ivy_ObjSetEquivListNext( pClass, NULL ); 
    }
    else
    {
        Ivy_ObjSetEquivListNext( pList->pTail, pClass ); 
        Ivy_ObjSetEquivListPrev( pClass, pList->pTail );
        Ivy_ObjSetEquivListNext( pClass, NULL ); 
        pList->pTail = pClass;
    }
    pList->nItems++;
}
 
/**Function*************************************************************

  Synopsis    [Updates the list of classes after base class has split.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigInsertClass( Ivy_FraigList_t * pList, Ivy_Obj_t * pBase, Ivy_Obj_t * pClass )
{
    Ivy_ObjSetEquivListPrev( pClass, pBase );
    Ivy_ObjSetEquivListNext( pClass, Ivy_ObjEquivListNext(pBase) ); 
    if ( Ivy_ObjEquivListNext(pBase) )
        Ivy_ObjSetEquivListPrev( Ivy_ObjEquivListNext(pBase), pClass );
    Ivy_ObjSetEquivListNext( pBase, pClass ); 
    if ( pList->pTail == pBase )
        pList->pTail = pClass;
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    [Removes equivalence class from the list of classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigRemoveClass( Ivy_FraigList_t * pList, Ivy_Obj_t * pClass )
{
    if ( pList->pHead == pClass )
        pList->pHead = Ivy_ObjEquivListNext(pClass);
    if ( pList->pTail == pClass )
        pList->pTail = Ivy_ObjEquivListPrev(pClass);
    if ( Ivy_ObjEquivListPrev(pClass) )
        Ivy_ObjSetEquivListNext( Ivy_ObjEquivListPrev(pClass), Ivy_ObjEquivListNext(pClass) ); 
    if ( Ivy_ObjEquivListNext(pClass) )
        Ivy_ObjSetEquivListPrev( Ivy_ObjEquivListNext(pClass), Ivy_ObjEquivListPrev(pClass) );
    Ivy_ObjSetEquivListNext( pClass, NULL ); 
    Ivy_ObjSetEquivListPrev( pClass, NULL );
    pClass->fMarkA = 0;
    pList->nItems--;
}

/**Function*************************************************************

  Synopsis    [Count the number of pairs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigCountPairsClasses( Ivy_FraigMan_t * p )
{
    Ivy_Obj_t * pClass, * pNode;
    int nPairs = 0, nNodes;
    return nPairs;

    Ivy_FraigForEachEquivClass( p->lClasses.pHead, pClass )
    {
        nNodes = 0;
        Ivy_FraigForEachClassNode( pClass, pNode )
            nNodes++;
        nPairs += nNodes * (nNodes - 1) / 2;
    }
    return nPairs;
}

/**Function*************************************************************

  Synopsis    [Creates initial simulation classes.]

  Description [Assumes that simulation info is assigned.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigCreateClasses( Ivy_FraigMan_t * p )
{
    Ivy_Obj_t ** pTable;
    Ivy_Obj_t * pObj, * pConst1, * pBin, * pEntry;
    int i, nTableSize;
    unsigned Hash;
    pConst1 = Ivy_ManConst1(p->pManAig);
    // allocate the table
    nTableSize = Ivy_ManObjNum(p->pManAig) / 2 + 13;
    pTable = ABC_ALLOC( Ivy_Obj_t *, nTableSize ); 
    memset( pTable, 0, sizeof(Ivy_Obj_t *) * nTableSize );
    // collect nodes into the table
    Ivy_ManForEachObj( p->pManAig, pObj, i )
    {
        if ( !Ivy_ObjIsPi(pObj) && !Ivy_ObjIsNode(pObj) )
            continue;
        Hash = Ivy_NodeHash( p, pObj );
        if ( Hash == 0 && Ivy_NodeHasZeroSim( p, pObj ) )
        {
            Ivy_NodeAddToClass( pConst1, pObj );
            continue;
        }
        // add the node to the table
        pBin = pTable[Hash % nTableSize];
        Ivy_FraigForEachBinNode( pBin, pEntry )
            if ( Ivy_NodeCompareSims( p, pEntry, pObj ) )
            {
                Ivy_NodeAddToClass( pEntry, pObj );
                break;
            }
        // check if the entry was added
        if ( pEntry )
            continue;
        Ivy_ObjSetNodeHashNext( pObj, pBin );
        pTable[Hash % nTableSize] = pObj;
    }
    // collect non-trivial classes
    assert( p->lClasses.pHead == NULL );
    Ivy_ManForEachObj( p->pManAig, pObj, i )
    {
        if ( !Ivy_ObjIsConst1(pObj) && !Ivy_ObjIsPi(pObj) && !Ivy_ObjIsNode(pObj) )
            continue;
        Ivy_ObjSetNodeHashNext( pObj, NULL );
        if ( Ivy_ObjClassNodeRepr(pObj) == NULL )
        {
            assert( Ivy_ObjClassNodeNext(pObj) == NULL );
            continue;
        }
        // recognize the head of the class
        if ( Ivy_ObjClassNodeNext( Ivy_ObjClassNodeRepr(pObj) ) != NULL )
            continue;
        // clean the class representative and add it to the list
        Ivy_ObjSetClassNodeRepr( pObj, NULL );
        Ivy_FraigAddClass( &p->lClasses, pObj );
    }
    // free the table
    ABC_FREE( pTable );
}

/**Function*************************************************************

  Synopsis    [Recursively refines the class after simulation.]

  Description [Returns 1 if the class has changed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigRefineClass_rec( Ivy_FraigMan_t * p, Ivy_Obj_t * pClass )
{
    Ivy_Obj_t * pClassNew, * pListOld, * pListNew, * pNode;
    int RetValue = 0;
    // check if there is refinement
    pListOld = pClass;
    Ivy_FraigForEachClassNode( Ivy_ObjClassNodeNext(pClass), pClassNew )
    {
        if ( !Ivy_NodeCompareSims(p, pClass, pClassNew) )
        {
            if ( p->pParams->fPatScores )
                Ivy_FraigAddToPatScores( p, pClass, pClassNew );
            break;
        }
        pListOld = pClassNew;
    }
    if ( pClassNew == NULL )
        return 0;
    // set representative of the new class
    Ivy_ObjSetClassNodeRepr( pClassNew, NULL );
    // start the new list
    pListNew = pClassNew;
    // go through the remaining nodes and sort them into two groups:
    // (1) matches of the old node; (2) non-matches of the old node
    Ivy_FraigForEachClassNode( Ivy_ObjClassNodeNext(pClassNew), pNode )
        if ( Ivy_NodeCompareSims( p, pClass, pNode ) )
        {
            Ivy_ObjSetClassNodeNext( pListOld, pNode );
            pListOld = pNode;
        }
        else
        {
            Ivy_ObjSetClassNodeNext( pListNew, pNode );
            Ivy_ObjSetClassNodeRepr( pNode, pClassNew );
            pListNew = pNode;
        }
    // finish both lists
    Ivy_ObjSetClassNodeNext( pListNew, NULL );
    Ivy_ObjSetClassNodeNext( pListOld, NULL );
    // update the list of classes
    Ivy_FraigInsertClass( &p->lClasses, pClass, pClassNew );
    // if the old class is trivial, remove it
    if ( Ivy_ObjClassNodeNext(pClass) == NULL )
        Ivy_FraigRemoveClass( &p->lClasses, pClass );
    // if the new class is trivial, remove it; otherwise, try to refine it
    if ( Ivy_ObjClassNodeNext(pClassNew) == NULL )
        Ivy_FraigRemoveClass( &p->lClasses, pClassNew );
    else
        RetValue = Ivy_FraigRefineClass_rec( p, pClassNew );
    return RetValue + 1;
}
 
/**Function*************************************************************

  Synopsis    [Creates the counter-example from the successful pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigCheckOutputSimsSavePattern( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj )
{ 
    Ivy_FraigSim_t * pSims;
    int i, k, BestPat, * pModel;
    // find the word of the pattern
    pSims = Ivy_ObjSim(pObj);
    for ( i = 0; i < p->nSimWords; i++ )
        if ( pSims->pData[i] )
            break;
    assert( i < p->nSimWords );
    // find the bit of the pattern
    for ( k = 0; k < 32; k++ )
        if ( pSims->pData[i] & (1 << k) )
            break;
    assert( k < 32 );
    // determine the best pattern
    BestPat = i * 32 + k;
    // fill in the counter-example data
    pModel = ABC_ALLOC( int, Ivy_ManPiNum(p->pManFraig) );
    Ivy_ManForEachPi( p->pManAig, pObj, i )
    {
        pModel[i] = Ivy_InfoHasBit(Ivy_ObjSim(pObj)->pData, BestPat);
//        printf( "%d", pModel[i] );
    }
//    printf( "\n" );
    // set the model
    assert( p->pManFraig->pData == NULL );
    p->pManFraig->pData = pModel;
    return;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the one of the output is already non-constant 0.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigCheckOutputSims( Ivy_FraigMan_t * p )
{
    Ivy_Obj_t * pObj;
    int i;
    // make sure the reference simulation pattern does not detect the bug
//    pObj = Ivy_ManPo( p->pManAig, 0 );
    Ivy_ManForEachPo( p->pManAig, pObj, i )
    {
        assert( Ivy_ObjFanin0(pObj)->fPhase == (unsigned)Ivy_ObjFaninC0(pObj) ); // Ivy_ObjFaninPhase(Ivy_ObjChild0(pObj)) == 0
        // complement simulation info
//        if ( Ivy_ObjFanin0(pObj)->fPhase ^ Ivy_ObjFaninC0(pObj) ) // Ivy_ObjFaninPhase(Ivy_ObjChild0(pObj))
//            Ivy_NodeComplementSim( p, Ivy_ObjFanin0(pObj) );
        // check 
        if ( !Ivy_NodeHasZeroSim( p, Ivy_ObjFanin0(pObj) ) )
        {
            // create the counter-example from this pattern
            Ivy_FraigCheckOutputSimsSavePattern( p, Ivy_ObjFanin0(pObj) );
            return 1;
        }
        // complement simulation info
//        if ( Ivy_ObjFanin0(pObj)->fPhase ^ Ivy_ObjFaninC0(pObj) )
//            Ivy_NodeComplementSim( p, Ivy_ObjFanin0(pObj) );
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Refines the classes after simulation.]

  Description [Assumes that simulation info is assigned. Returns the
  number of classes refined.]
               
  SideEffects [Large equivalence class of constant 0 may cause problems.]

  SeeAlso     []

***********************************************************************/
int Ivy_FraigRefineClasses( Ivy_FraigMan_t * p )
{
    Ivy_Obj_t * pClass, * pClass2;
    int RetValue, Counter = 0;
    abctime clk;
    // check if some outputs already became non-constant
    // this is a special case when computation can be stopped!!!
    if ( p->pParams->fProve )
        Ivy_FraigCheckOutputSims( p );
    if ( p->pManFraig->pData )
        return 0;
    // refine the classed
clk = Abc_Clock();
    Ivy_FraigForEachEquivClassSafe( p->lClasses.pHead, pClass, pClass2 )
    {
        if ( pClass->fMarkA )
            continue;
        RetValue = Ivy_FraigRefineClass_rec( p, pClass );
        Counter += ( RetValue > 0 );
//if ( Ivy_ObjIsConst1(pClass) )
//printf( "%d ", RetValue );
//if ( Ivy_ObjIsConst1(pClass) )
//    p->time1 += Abc_Clock() - clk;
    }
p->timeRef += Abc_Clock() - clk;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Print the class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigPrintClass( Ivy_Obj_t * pClass )
{
    Ivy_Obj_t * pObj;
    printf( "Class {" );
    Ivy_FraigForEachClassNode( pClass, pObj )
        printf( " %d(%d)%c", pObj->Id, pObj->Level, pObj->fPhase? '+' : '-' );
    printf( " }\n" );
}

/**Function*************************************************************

  Synopsis    [Count the number of elements in the class.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigCountClassNodes( Ivy_Obj_t * pClass )
{
    Ivy_Obj_t * pObj;
    int Counter = 0;
    Ivy_FraigForEachClassNode( pClass, pObj )
        Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Prints simulation classes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigPrintSimClasses( Ivy_FraigMan_t * p )
{
    Ivy_Obj_t * pClass;
    Ivy_FraigForEachEquivClass( p->lClasses.pHead, pClass )
    {
//        Ivy_FraigPrintClass( pClass );
        printf( "%d ", Ivy_FraigCountClassNodes( pClass ) );
    }
//    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Generated const 0 pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigSavePattern0( Ivy_FraigMan_t * p )
{
    memset( p->pPatWords, 0, sizeof(unsigned) * p->nPatWords );
}

/**Function*************************************************************

  Synopsis    [[Generated const 1 pattern.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigSavePattern1( Ivy_FraigMan_t * p )
{
    memset( p->pPatWords, 0xff, sizeof(unsigned) * p->nPatWords );
}

/**Function*************************************************************

  Synopsis    [Generates the counter-example satisfying the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Ivy_FraigCreateModel( Ivy_FraigMan_t * p )
{
    int * pModel;
    Ivy_Obj_t * pObj;
    int i;
    pModel = ABC_ALLOC( int, Ivy_ManPiNum(p->pManFraig) );
    Ivy_ManForEachPi( p->pManFraig, pObj, i )
//        pModel[i] = ( p->pSat->model.ptr[Ivy_ObjSatNum(pObj)] == l_True );
        pModel[i] = ( p->pSat->model[Ivy_ObjSatNum(pObj)] == l_True );
    return pModel;
}

/**Function*************************************************************

  Synopsis    [Copy pattern from the solver into the internal storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigSavePattern( Ivy_FraigMan_t * p )
{
    Ivy_Obj_t * pObj;
    int i;
    memset( p->pPatWords, 0, sizeof(unsigned) * p->nPatWords );
    Ivy_ManForEachPi( p->pManFraig, pObj, i )
//    Vec_PtrForEachEntry( Ivy_Obj_t *, p->vPiVars, pObj, i )
//        if ( p->pSat->model.ptr[Ivy_ObjSatNum(pObj)] == l_True )
        if ( p->pSat->model[Ivy_ObjSatNum(pObj)] == l_True )
            Ivy_InfoSetBit( p->pPatWords, i );
//            Ivy_InfoSetBit( p->pPatWords, pObj->Id - 1 );
}

/**Function*************************************************************

  Synopsis    [Copy pattern from the solver into the internal storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigSavePattern2( Ivy_FraigMan_t * p )
{
    Ivy_Obj_t * pObj;
    int i;
    memset( p->pPatWords, 0, sizeof(unsigned) * p->nPatWords );
//    Ivy_ManForEachPi( p->pManFraig, pObj, i )
    Vec_PtrForEachEntry( Ivy_Obj_t *, p->vPiVars, pObj, i )
//        if ( p->pSat->model.ptr[Ivy_ObjSatNum(pObj)] == l_True )
        if ( p->pSat->model[Ivy_ObjSatNum(pObj)] == l_True )
//            Ivy_InfoSetBit( p->pPatWords, i );
            Ivy_InfoSetBit( p->pPatWords, pObj->Id - 1 );
}

/**Function*************************************************************

  Synopsis    [Copy pattern from the solver into the internal storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigSavePattern3( Ivy_FraigMan_t * p )
{
    Ivy_Obj_t * pObj;
    int i;
    for ( i = 0; i < p->nPatWords; i++ )
        p->pPatWords[i] = Ivy_ObjRandomSim();
    Vec_PtrForEachEntry( Ivy_Obj_t *, p->vPiVars, pObj, i )
//        if ( Ivy_InfoHasBit( p->pPatWords, pObj->Id - 1 ) ^ (p->pSat->model.ptr[Ivy_ObjSatNum(pObj)] == l_True) )
        if ( Ivy_InfoHasBit( p->pPatWords, pObj->Id - 1 ) ^ sat_solver_var_value(p->pSat, Ivy_ObjSatNum(pObj)) )
            Ivy_InfoXorBit( p->pPatWords, pObj->Id - 1 );
}


/**Function*************************************************************

  Synopsis    [Performs simulation of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigSimulate( Ivy_FraigMan_t * p )
{
    int nChanges, nClasses;
    // start the classes
    Ivy_FraigAssignRandom( p );
    Ivy_FraigSimulateOne( p );
    Ivy_FraigCreateClasses( p );
//printf( "Starting classes = %5d.   Pairs = %6d.\n", p->lClasses.nItems, Ivy_FraigCountPairsClasses(p) );
    // refine classes by walking 0/1 patterns
    Ivy_FraigSavePattern0( p );
    Ivy_FraigAssignDist1( p, p->pPatWords );
    Ivy_FraigSimulateOne( p );
    nChanges = Ivy_FraigRefineClasses( p );
    if ( p->pManFraig->pData )
        return;
//printf( "Refined classes  = %5d.   Changes = %4d.   Pairs = %6d.\n", p->lClasses.nItems, nChanges, Ivy_FraigCountPairsClasses(p) );
    Ivy_FraigSavePattern1( p );
    Ivy_FraigAssignDist1( p, p->pPatWords );
    Ivy_FraigSimulateOne( p );
    nChanges = Ivy_FraigRefineClasses( p );
    if ( p->pManFraig->pData )
        return;
//printf( "Refined classes  = %5d.   Changes = %4d.   Pairs = %6d.\n", p->lClasses.nItems, nChanges, Ivy_FraigCountPairsClasses(p) );
    // refine classes by random simulation
    do {
        Ivy_FraigAssignRandom( p );
        Ivy_FraigSimulateOne( p );
        nClasses = p->lClasses.nItems;
        nChanges = Ivy_FraigRefineClasses( p );
        if ( p->pManFraig->pData )
            return;
//printf( "Refined classes  = %5d.   Changes = %4d.   Pairs = %6d.\n", p->lClasses.nItems, nChanges, Ivy_FraigCountPairsClasses(p) );
    } while ( (double)nChanges / nClasses > p->pParams->dSimSatur );
//    Ivy_FraigPrintSimClasses( p );
}



/**Function*************************************************************

  Synopsis    [Cleans pattern scores.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigCleanPatScores( Ivy_FraigMan_t * p )
{
    int i, nLimit = p->nSimWords * 32;
    for ( i = 0; i < nLimit; i++ )
        p->pPatScores[i] = 0;
}

/**Function*************************************************************

  Synopsis    [Adds to pattern scores.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigAddToPatScores( Ivy_FraigMan_t * p, Ivy_Obj_t * pClass, Ivy_Obj_t * pClassNew )
{
    Ivy_FraigSim_t * pSims0, * pSims1;
    unsigned uDiff;
    int i, w;
    // get hold of the simulation information
    pSims0 = Ivy_ObjSim(pClass);
    pSims1 = Ivy_ObjSim(pClassNew);
    // iterate through the differences and record the score
    for ( w = 0; w < p->nSimWords; w++ )
    {
        uDiff = pSims0->pData[w] ^ pSims1->pData[w];
        if ( uDiff == 0 )
            continue;
        for ( i = 0; i < 32; i++ )
            if ( uDiff & ( 1 << i ) )
                p->pPatScores[w*32+i]++;
    }
}

/**Function*************************************************************

  Synopsis    [Selects the best pattern.]

  Description [Returns 1 if such pattern is found.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigSelectBestPat( Ivy_FraigMan_t * p )
{
    Ivy_FraigSim_t * pSims;
    Ivy_Obj_t * pObj;
    int i, nLimit = p->nSimWords * 32, MaxScore = 0, BestPat = -1;
    for ( i = 1; i < nLimit; i++ )
    {
        if ( MaxScore < p->pPatScores[i] )
        {
            MaxScore = p->pPatScores[i];
            BestPat = i;
        }
    }
    if ( MaxScore == 0 )
        return 0;
//    if ( MaxScore > p->pParams->MaxScore )
//    printf( "Max score is %3d.  ", MaxScore );
    // copy the best pattern into the selected pattern
    memset( p->pPatWords, 0, sizeof(unsigned) * p->nPatWords );
    Ivy_ManForEachPi( p->pManAig, pObj, i )
    {
        pSims = Ivy_ObjSim(pObj);
        if ( Ivy_InfoHasBit(pSims->pData, BestPat) )
            Ivy_InfoSetBit(p->pPatWords, i);
    }
    return MaxScore;
}

/**Function*************************************************************

  Synopsis    [Resimulates fraiging manager after finding a counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigResimulate( Ivy_FraigMan_t * p )
{
    int nChanges;
    Ivy_FraigAssignDist1( p, p->pPatWords );
    Ivy_FraigSimulateOne( p );
    if ( p->pParams->fPatScores )
        Ivy_FraigCleanPatScores( p );
    nChanges = Ivy_FraigRefineClasses( p );
    if ( p->pManFraig->pData )
        return;
    if ( nChanges < 1 )
        printf( "Error: A counter-example did not refine classes!\n" );
    assert( nChanges >= 1 );
//printf( "Refined classes! = %5d.   Changes = %4d.\n", p->lClasses.nItems, nChanges );
    if ( !p->pParams->fPatScores )
        return;

    // perform additional simulation using dist1 patterns derived from successful patterns
    while ( Ivy_FraigSelectBestPat(p) > p->pParams->MaxScore )
    {
        Ivy_FraigAssignDist1( p, p->pPatWords );
        Ivy_FraigSimulateOne( p );
        Ivy_FraigCleanPatScores( p );
        nChanges = Ivy_FraigRefineClasses( p );
        if ( p->pManFraig->pData )
            return;
//printf( "Refined class!!! = %5d.   Changes = %4d.   Pairs = %6d.\n", p->lClasses.nItems, nChanges, Ivy_FraigCountPairsClasses(p) );
        if ( nChanges == 0 )
            break;
    }
}


/**Function*************************************************************

  Synopsis    [Prints the status of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigMiterPrint( Ivy_Man_t * pNtk, char * pString, abctime clk, int fVerbose )
{
    if ( !fVerbose )
        return;
    printf( "Nodes = %7d.  Levels = %4d.  ", Ivy_ManNodeNum(pNtk), Ivy_ManLevels(pNtk) );
    ABC_PRT( pString, Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Reports the status of the miter.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigMiterStatus( Ivy_Man_t * pMan )
{
    Ivy_Obj_t * pObj, * pObjNew;
    int i, CountConst0 = 0, CountNonConst0 = 0, CountUndecided = 0;
    if ( pMan->pData )
        return 0;
    Ivy_ManForEachPo( pMan, pObj, i )
    {
        pObjNew = Ivy_ObjChild0(pObj);
        // check if the output is constant 1
        if ( pObjNew == pMan->pConst1 )
        {
            CountNonConst0++;
            continue;
        }
        // check if the output is constant 0
        if ( pObjNew == Ivy_Not(pMan->pConst1) )
        {
            CountConst0++;
            continue;
        }
/*
        // check if the output is a primary input
        if ( Ivy_ObjIsPi(Ivy_Regular(pObjNew)) )
        {
            CountNonConst0++;
            continue;
        }
*/
        // check if the output can be constant 0
        if ( Ivy_Regular(pObjNew)->fPhase != (unsigned)Ivy_IsComplement(pObjNew) )
        {
            CountNonConst0++;
            continue;
        }
        CountUndecided++;
    }
/*
    if ( p->pParams->fVerbose )
    {
        printf( "Miter has %d outputs. ", Ivy_ManPoNum(p->pManAig) );
        printf( "Const0 = %d.  ", CountConst0 );
        printf( "NonConst0 = %d.  ", CountNonConst0 );
        printf( "Undecided = %d.  ", CountUndecided );
        printf( "\n" );
    }
*/
    if ( CountNonConst0 )
        return 0;
    if ( CountUndecided )
        return -1;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Tries to prove each output of the miter until encountering a sat output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigMiterProve( Ivy_FraigMan_t * p )
{
    Ivy_Obj_t * pObj, * pObjNew;
    int i, RetValue;
    abctime clk = Abc_Clock();
    int fVerbose = 0;
    Ivy_ManForEachPo( p->pManAig, pObj, i )
    {
        if ( i && fVerbose )
        {
            ABC_PRT( "Time", Abc_Clock() -clk );
        }
        pObjNew = Ivy_ObjChild0Equiv(pObj);
        // check if the output is constant 1
        if ( pObjNew == p->pManFraig->pConst1 )
        {
            if ( fVerbose )
                printf( "Output %2d (out of %2d) is constant 1.  ", i, Ivy_ManPoNum(p->pManAig) );
            // assing constant 0 model
            p->pManFraig->pData = ABC_ALLOC( int, Ivy_ManPiNum(p->pManFraig) );
            memset( p->pManFraig->pData, 0, sizeof(int) * Ivy_ManPiNum(p->pManFraig) );
            break;
        }
        // check if the output is constant 0
        if ( pObjNew == Ivy_Not(p->pManFraig->pConst1) )
        {
            if ( fVerbose )
                printf( "Output %2d (out of %2d) is already constant 0.  ", i, Ivy_ManPoNum(p->pManAig) );
            continue;
        }
        // check if the output can be constant 0
        if ( Ivy_Regular(pObjNew)->fPhase != (unsigned)Ivy_IsComplement(pObjNew) )
        {
            if ( fVerbose )
                printf( "Output %2d (out of %2d) cannot be constant 0.  ", i, Ivy_ManPoNum(p->pManAig) );
            // assing constant 0 model
            p->pManFraig->pData = ABC_ALLOC( int, Ivy_ManPiNum(p->pManFraig) );
            memset( p->pManFraig->pData, 0, sizeof(int) * Ivy_ManPiNum(p->pManFraig) );
            break;
        }
/*
        // check the representative of this node
        pRepr = Ivy_ObjClassNodeRepr(Ivy_ObjFanin0(pObj));
        if ( Ivy_Regular(pRepr) != p->pManAig->pConst1 )
            printf( "Representative is not constant 1.\n" );
        else
            printf( "Representative is constant 1.\n" );
*/
        // try to prove the output constant 0
        RetValue = Ivy_FraigNodeIsConst( p, Ivy_Regular(pObjNew) );
        if ( RetValue == 1 )  // proved equivalent
        {
            if ( fVerbose )
                printf( "Output %2d (out of %2d) was proved constant 0.  ", i, Ivy_ManPoNum(p->pManAig) );
            // set the constant miter
            Ivy_ObjFanin0(pObj)->pEquiv = Ivy_NotCond( p->pManFraig->pConst1, !Ivy_ObjFaninC0(pObj) );
            continue;
        }
        if ( RetValue == -1 ) // failed
        {
            if ( fVerbose )
                printf( "Output %2d (out of %2d) has timed out at %d backtracks.  ", i, Ivy_ManPoNum(p->pManAig), p->pParams->nBTLimitMiter );
            continue;
        }
        // proved satisfiable
        if ( fVerbose )
            printf( "Output %2d (out of %2d) was proved NOT a constant 0.  ", i, Ivy_ManPoNum(p->pManAig) );
        // create the model
        p->pManFraig->pData = Ivy_FraigCreateModel(p);
        break;
    }
    if ( fVerbose )
    {
        ABC_PRT( "Time", Abc_Clock() -clk );
    }
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for the internal nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigSweep( Ivy_FraigMan_t * p )
{
    Ivy_Obj_t * pObj;//, * pTemp;
    int i, k = 0;
p->nClassesZero = p->lClasses.pHead? (Ivy_ObjIsConst1(p->lClasses.pHead) ? Ivy_FraigCountClassNodes(p->lClasses.pHead) : 0) : 0;
p->nClassesBeg  = p->lClasses.nItems;
    // duplicate internal nodes
    p->pProgress = Extra_ProgressBarStart( stdout, Ivy_ManNodeNum(p->pManAig) );
    Ivy_ManForEachNode( p->pManAig, pObj, i )
    {
        Extra_ProgressBarUpdate( p->pProgress, k++, NULL );
        // default to simple strashing if simulation detected a counter-example for a PO
        if ( p->pManFraig->pData )
            pObj->pEquiv = Ivy_And( p->pManFraig, Ivy_ObjChild0Equiv(pObj), Ivy_ObjChild1Equiv(pObj) );
        else
            pObj->pEquiv = Ivy_FraigAnd( p, pObj );
        assert( pObj->pEquiv != NULL );
//        pTemp = Ivy_Regular(pObj->pEquiv);
//        assert( Ivy_Regular(pObj->pEquiv)->Type );
    }
    Extra_ProgressBarStop( p->pProgress );
p->nClassesEnd = p->lClasses.nItems;
    // try to prove the outputs of the miter
    p->nNodesMiter = Ivy_ManNodeNum(p->pManFraig);
//    Ivy_FraigMiterStatus( p->pManFraig );
    if ( p->pParams->fProve && p->pManFraig->pData == NULL )
        Ivy_FraigMiterProve( p );
    // add the POs
    Ivy_ManForEachPo( p->pManAig, pObj, i )
        Ivy_ObjCreatePo( p->pManFraig, Ivy_ObjChild0Equiv(pObj) );
    // clean the old manager
    Ivy_ManForEachObj( p->pManAig, pObj, i )
        pObj->pFanout = pObj->pNextFan0 = pObj->pNextFan1 = pObj->pPrevFan0 = pObj->pPrevFan1 = NULL;
    // clean the new manager
    Ivy_ManForEachObj( p->pManFraig, pObj, i )
    {
        if ( Ivy_ObjFaninVec(pObj) )
            Vec_PtrFree( Ivy_ObjFaninVec(pObj) );
        pObj->pNextFan0 = pObj->pNextFan1 = NULL;
        pObj->pEquiv = NULL;
    }
    // remove dangling nodes 
    Ivy_ManCleanup( p->pManFraig );
    // clean up the class marks
    Ivy_FraigForEachEquivClass( p->lClasses.pHead, pObj )
        pObj->fMarkA = 0;
}

/**Function*************************************************************

  Synopsis    [Performs fraiging for one node.]

  Description [Returns the fraiged node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_FraigAnd( Ivy_FraigMan_t * p, Ivy_Obj_t * pObjOld )
{ 
    Ivy_Obj_t * pObjNew, * pFanin0New, * pFanin1New, * pObjReprNew;
    int RetValue;
    // get the fraiged fanins
    pFanin0New = Ivy_ObjChild0Equiv(pObjOld);
    pFanin1New = Ivy_ObjChild1Equiv(pObjOld);
    // get the candidate fraig node
    pObjNew = Ivy_And( p->pManFraig, pFanin0New, pFanin1New );
    // get representative of this class
    if ( Ivy_ObjClassNodeRepr(pObjOld) == NULL || // this is a unique node
         (!p->pParams->fDoSparse && Ivy_ObjClassNodeRepr(pObjOld) == p->pManAig->pConst1) ) // this is a sparse node
    {
//        assert( Ivy_Regular(pFanin0New) != Ivy_Regular(pFanin1New) );
//        assert( pObjNew != Ivy_Regular(pFanin0New) );
//        assert( pObjNew != Ivy_Regular(pFanin1New) );
        return pObjNew;
    }
    // get the fraiged representative
    pObjReprNew = Ivy_ObjFraig(Ivy_ObjClassNodeRepr(pObjOld));
    // if the fraiged nodes are the same return
    if ( Ivy_Regular(pObjNew) == Ivy_Regular(pObjReprNew) )
        return pObjNew;
    assert( Ivy_Regular(pObjNew) != Ivy_ManConst1(p->pManFraig) );
//    printf( "Node = %d. Repr = %d.\n", pObjOld->Id, Ivy_ObjClassNodeRepr(pObjOld)->Id );

    // they are different (the counter-example is in p->pPatWords)
    RetValue = Ivy_FraigNodesAreEquiv( p, Ivy_Regular(pObjReprNew), Ivy_Regular(pObjNew) );
    if ( RetValue == 1 )  // proved equivalent
    {
        // mark the class as proved
        if ( Ivy_ObjClassNodeNext(pObjOld) == NULL )
            Ivy_ObjClassNodeRepr(pObjOld)->fMarkA = 1;
        return Ivy_NotCond( pObjReprNew, pObjOld->fPhase ^ Ivy_ObjClassNodeRepr(pObjOld)->fPhase );
    }
    if ( RetValue == -1 ) // failed
        return pObjNew;
    // simulate the counter-example and return the new node
    Ivy_FraigResimulate( p );
    return pObjNew;
}

/**Function*************************************************************

  Synopsis    [Prints variable activity.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigPrintActivity( Ivy_FraigMan_t * p )
{
    int i;
    for ( i = 0; i < p->nSatVars; i++ )
        printf( "%d %d  ", i, (int)p->pSat->activity[i] );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Runs equivalence test for the two nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigNodesAreEquiv( Ivy_FraigMan_t * p, Ivy_Obj_t * pOld, Ivy_Obj_t * pNew )
{
    int pLits[4], RetValue, RetValue1, nBTLimit;
    abctime clk; //, clk2 = Abc_Clock();

    // make sure the nodes are not complemented
    assert( !Ivy_IsComplement(pNew) );
    assert( !Ivy_IsComplement(pOld) );
    assert( pNew != pOld );

    // if at least one of the nodes is a failed node, perform adjustments:
    // if the backtrack limit is small, simply skip this node
    // if the backtrack limit is > 10, take the quare root of the limit
    nBTLimit = p->pParams->nBTLimitNode;
    if ( nBTLimit > 0 && (pOld->fFailTfo || pNew->fFailTfo) )
    {
        p->nSatFails++;
        // fail immediately
//        return -1;
        if ( nBTLimit <= 10 )
            return -1;
        nBTLimit = (int)pow(nBTLimit, 0.7);
    }
    p->nSatCalls++;

    // make sure the solver is allocated and has enough variables
    if ( p->pSat == NULL )
    {
        p->pSat = sat_solver_new();
        sat_solver_setnvars( p->pSat, 1000 );
        p->pSat->factors = ABC_CALLOC( double, p->pSat->cap );
        p->nSatVars = 1;
        // var 0 is reserved for const1 node - add the clause
//        pLits[0] = toLit( 0 );
//        sat_solver_addclause( p->pSat, pLits, pLits + 1 );
    }

    // if the nodes do not have SAT variables, allocate them
    Ivy_FraigNodeAddToSolver( p, pOld, pNew );

    // prepare variable activity
    Ivy_FraigSetActivityFactors( p, pOld, pNew ); 

    // solve under assumptions
    // A = 1; B = 0     OR     A = 1; B = 1 
clk = Abc_Clock();
    pLits[0] = toLitCond( Ivy_ObjSatNum(pOld), 0 );
    pLits[1] = toLitCond( Ivy_ObjSatNum(pNew), pOld->fPhase == pNew->fPhase );
//Sat_SolverWriteDimacs( p->pSat, "temp.cnf", pLits, pLits + 2, 1 );
    RetValue1 = sat_solver_solve( p->pSat, pLits, pLits + 2, 
        (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, 
        p->nBTLimitGlobal, p->nInsLimitGlobal );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue1 == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        pLits[0] = lit_neg( pLits[0] );
        pLits[1] = lit_neg( pLits[1] );
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
        assert( RetValue );
        // continue solving the other implication
        p->nSatCallsUnsat++;
    }
    else if ( RetValue1 == l_True )
    {
p->timeSatSat += Abc_Clock() - clk;
        Ivy_FraigSavePattern( p );
        p->nSatCallsSat++;
        return 0;
    }
    else // if ( RetValue1 == l_Undef )
    {
p->timeSatFail += Abc_Clock() - clk;
/*
        if ( nBTLimit > 1000 )
        {
            RetValue = Ivy_FraigCheckCone( p, p->pManFraig, pOld, pNew, nBTLimit );
            if ( RetValue != -1 )
                return RetValue;
        }
*/
        // mark the node as the failed node
        if ( pOld != p->pManFraig->pConst1 ) 
            pOld->fFailTfo = 1;
        pNew->fFailTfo = 1;
        p->nSatFailsReal++;
        return -1;
    }

    // if the old node was constant 0, we already know the answer
    if ( pOld == p->pManFraig->pConst1 )
    {
        p->nSatProof++;
        return 1;
    }

    // solve under assumptions
    // A = 0; B = 1     OR     A = 0; B = 0 
clk = Abc_Clock();
    pLits[0] = toLitCond( Ivy_ObjSatNum(pOld), 1 );
    pLits[1] = toLitCond( Ivy_ObjSatNum(pNew), pOld->fPhase ^ pNew->fPhase );
    RetValue1 = sat_solver_solve( p->pSat, pLits, pLits + 2, 
        (ABC_INT64_T)nBTLimit, (ABC_INT64_T)0, 
        p->nBTLimitGlobal, p->nInsLimitGlobal );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue1 == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        pLits[0] = lit_neg( pLits[0] );
        pLits[1] = lit_neg( pLits[1] );
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
        assert( RetValue );
        p->nSatCallsUnsat++;
    }
    else if ( RetValue1 == l_True )
    {
p->timeSatSat += Abc_Clock() - clk;
        Ivy_FraigSavePattern( p );
        p->nSatCallsSat++;
        return 0;
    }
    else // if ( RetValue1 == l_Undef )
    {
p->timeSatFail += Abc_Clock() - clk;
/*
        if ( nBTLimit > 1000 )
        {
            RetValue = Ivy_FraigCheckCone( p, p->pManFraig, pOld, pNew, nBTLimit );
            if ( RetValue != -1 )
                return RetValue;
        }
*/
        // mark the node as the failed node
        pOld->fFailTfo = 1;
        pNew->fFailTfo = 1;
        p->nSatFailsReal++;
        return -1;
    }
/*
    // check BDD proof
    {
        int RetVal;
        ABC_PRT( "Sat", Abc_Clock() - clk2 );
        clk2 = Abc_Clock();
        RetVal = Ivy_FraigNodesAreEquivBdd( pOld, pNew );
//        printf( "%d ", RetVal );
        assert( RetVal );
        ABC_PRT( "Bdd", Abc_Clock() - clk2 );
        printf( "\n" );
    }
*/
    // return SAT proof
    p->nSatProof++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Runs equivalence test for one node.]

  Description [Returns the fraiged node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigNodeIsConst( Ivy_FraigMan_t * p, Ivy_Obj_t * pNew )
{
    int pLits[2], RetValue1;
    abctime clk;
    int RetValue;

    // make sure the nodes are not complemented
    assert( !Ivy_IsComplement(pNew) );
    assert( pNew != p->pManFraig->pConst1 );
    p->nSatCalls++;

    // make sure the solver is allocated and has enough variables
    if ( p->pSat == NULL )
    {
        p->pSat = sat_solver_new();
        sat_solver_setnvars( p->pSat, 1000 );
        p->pSat->factors = ABC_CALLOC( double, p->pSat->cap );
        p->nSatVars = 1;
        // var 0 is reserved for const1 node - add the clause
//        pLits[0] = toLit( 0 );
//        sat_solver_addclause( p->pSat, pLits, pLits + 1 );
    }

    // if the nodes do not have SAT variables, allocate them
    Ivy_FraigNodeAddToSolver( p, NULL, pNew );

    // prepare variable activity
    Ivy_FraigSetActivityFactors( p, NULL, pNew ); 

    // solve under assumptions
clk = Abc_Clock();
    pLits[0] = toLitCond( Ivy_ObjSatNum(pNew), pNew->fPhase );
    RetValue1 = sat_solver_solve( p->pSat, pLits, pLits + 1, 
        (ABC_INT64_T)p->pParams->nBTLimitMiter, (ABC_INT64_T)0, 
        p->nBTLimitGlobal, p->nInsLimitGlobal );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue1 == l_False )
    {
p->timeSatUnsat += Abc_Clock() - clk;
        pLits[0] = lit_neg( pLits[0] );
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 1 );
        assert( RetValue );
        // continue solving the other implication
        p->nSatCallsUnsat++;
    }
    else if ( RetValue1 == l_True )
    {
p->timeSatSat += Abc_Clock() - clk;
        if ( p->pPatWords )
            Ivy_FraigSavePattern( p );
        p->nSatCallsSat++;
        return 0;
    }
    else // if ( RetValue1 == l_Undef )
    {
p->timeSatFail += Abc_Clock() - clk;
/*
        if ( p->pParams->nBTLimitMiter > 1000 )
        {
            RetValue = Ivy_FraigCheckCone( p, p->pManFraig, p->pManFraig->pConst1, pNew, p->pParams->nBTLimitMiter );
            if ( RetValue != -1 )
                return RetValue;
        }
*/
        // mark the node as the failed node
        pNew->fFailTfo = 1;
        p->nSatFailsReal++;
        return -1;
    }

    // return SAT proof
    p->nSatProof++;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Addes clauses to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigAddClausesMux( Ivy_FraigMan_t * p, Ivy_Obj_t * pNode )
{
    Ivy_Obj_t * pNodeI, * pNodeT, * pNodeE;
    int pLits[4], RetValue, VarF, VarI, VarT, VarE, fCompT, fCompE;

    assert( !Ivy_IsComplement( pNode ) );
    assert( Ivy_ObjIsMuxType( pNode ) );
    // get nodes (I = if, T = then, E = else)
    pNodeI = Ivy_ObjRecognizeMux( pNode, &pNodeT, &pNodeE );
    // get the variable numbers
    VarF = Ivy_ObjSatNum(pNode);
    VarI = Ivy_ObjSatNum(pNodeI);
    VarT = Ivy_ObjSatNum(Ivy_Regular(pNodeT));
    VarE = Ivy_ObjSatNum(Ivy_Regular(pNodeE));
    // get the complementation flags
    fCompT = Ivy_IsComplement(pNodeT);
    fCompE = Ivy_IsComplement(pNodeE);

    // f = ITE(i, t, e)

    // i' + t' + f
    // i' + t  + f'
    // i  + e' + f
    // i  + e  + f'

    // create four clauses
    pLits[0] = toLitCond(VarI, 1);
    pLits[1] = toLitCond(VarT, 1^fCompT);
    pLits[2] = toLitCond(VarF, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
    pLits[0] = toLitCond(VarI, 1);
    pLits[1] = toLitCond(VarT, 0^fCompT);
    pLits[2] = toLitCond(VarF, 1);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
    pLits[0] = toLitCond(VarI, 0);
    pLits[1] = toLitCond(VarE, 1^fCompE);
    pLits[2] = toLitCond(VarF, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
    pLits[0] = toLitCond(VarI, 0);
    pLits[1] = toLitCond(VarE, 0^fCompE);
    pLits[2] = toLitCond(VarF, 1);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
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

    pLits[0] = toLitCond(VarT, 0^fCompT);
    pLits[1] = toLitCond(VarE, 0^fCompE);
    pLits[2] = toLitCond(VarF, 1);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
    pLits[0] = toLitCond(VarT, 1^fCompT);
    pLits[1] = toLitCond(VarE, 1^fCompE);
    pLits[2] = toLitCond(VarF, 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 3 );
    assert( RetValue );
}

/**Function*************************************************************

  Synopsis    [Addes clauses to the solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigAddClausesSuper( Ivy_FraigMan_t * p, Ivy_Obj_t * pNode, Vec_Ptr_t * vSuper )
{
    Ivy_Obj_t * pFanin;
    int * pLits, nLits, RetValue, i;
    assert( !Ivy_IsComplement(pNode) );
    assert( Ivy_ObjIsNode( pNode ) );
    // create storage for literals
    nLits = Vec_PtrSize(vSuper) + 1;
    pLits = ABC_ALLOC( int, nLits );
    // suppose AND-gate is A & B = C
    // add !A => !C   or   A + !C
    Vec_PtrForEachEntry( Ivy_Obj_t *, vSuper, pFanin, i )
    {
        pLits[0] = toLitCond(Ivy_ObjSatNum(Ivy_Regular(pFanin)), Ivy_IsComplement(pFanin));
        pLits[1] = toLitCond(Ivy_ObjSatNum(pNode), 1);
        RetValue = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
        assert( RetValue );
    }
    // add A & B => C   or   !A + !B + C
    Vec_PtrForEachEntry( Ivy_Obj_t *, vSuper, pFanin, i )
        pLits[i] = toLitCond(Ivy_ObjSatNum(Ivy_Regular(pFanin)), !Ivy_IsComplement(pFanin));
    pLits[nLits-1] = toLitCond(Ivy_ObjSatNum(pNode), 0);
    RetValue = sat_solver_addclause( p->pSat, pLits, pLits + nLits );
    assert( RetValue );
    ABC_FREE( pLits );
}

/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigCollectSuper_rec( Ivy_Obj_t * pObj, Vec_Ptr_t * vSuper, int fFirst, int fUseMuxes )
{
    // if the new node is complemented or a PI, another gate begins
    if ( Ivy_IsComplement(pObj) || Ivy_ObjIsPi(pObj) || (!fFirst && Ivy_ObjRefs(pObj) > 1) || 
         (fUseMuxes && Ivy_ObjIsMuxType(pObj)) )
    {
        Vec_PtrPushUnique( vSuper, pObj );
        return;
    }
    // go through the branches
    Ivy_FraigCollectSuper_rec( Ivy_ObjChild0(pObj), vSuper, 0, fUseMuxes );
    Ivy_FraigCollectSuper_rec( Ivy_ObjChild1(pObj), vSuper, 0, fUseMuxes );
}

/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Ivy_FraigCollectSuper( Ivy_Obj_t * pObj, int fUseMuxes )
{
    Vec_Ptr_t * vSuper;
    assert( !Ivy_IsComplement(pObj) );
    assert( !Ivy_ObjIsPi(pObj) );
    vSuper = Vec_PtrAlloc( 4 );
    Ivy_FraigCollectSuper_rec( pObj, vSuper, 1, fUseMuxes );
    return vSuper;
}

/**Function*************************************************************

  Synopsis    [Updates the solver clause database.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigObjAddToFrontier( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj, Vec_Ptr_t * vFrontier )
{
    assert( !Ivy_IsComplement(pObj) );
    if ( Ivy_ObjSatNum(pObj) )
        return;
    assert( Ivy_ObjSatNum(pObj) == 0 );
    assert( Ivy_ObjFaninVec(pObj) == NULL );
    if ( Ivy_ObjIsConst1(pObj) )
        return;
//printf( "Assigning node %d number %d\n", pObj->Id, p->nSatVars );
    Ivy_ObjSetSatNum( pObj, p->nSatVars++ );
    if ( Ivy_ObjIsNode(pObj) )
        Vec_PtrPush( vFrontier, pObj );
}

/**Function*************************************************************

  Synopsis    [Updates the solver clause database.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigNodeAddToSolver( Ivy_FraigMan_t * p, Ivy_Obj_t * pOld, Ivy_Obj_t * pNew )
{ 
    Vec_Ptr_t * vFrontier, * vFanins;
    Ivy_Obj_t * pNode, * pFanin;
    int i, k, fUseMuxes = 1;
    assert( pOld || pNew );
    // quit if CNF is ready
    if ( (!pOld || Ivy_ObjFaninVec(pOld)) && (!pNew || Ivy_ObjFaninVec(pNew)) )
        return;
    // start the frontier
    vFrontier = Vec_PtrAlloc( 100 );
    if ( pOld ) Ivy_FraigObjAddToFrontier( p, pOld, vFrontier );
    if ( pNew ) Ivy_FraigObjAddToFrontier( p, pNew, vFrontier );
    // explore nodes in the frontier
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFrontier, pNode, i )
    {
        // create the supergate
        assert( Ivy_ObjSatNum(pNode) );
        assert( Ivy_ObjFaninVec(pNode) == NULL );
        if ( fUseMuxes && Ivy_ObjIsMuxType(pNode) )
        {
            vFanins = Vec_PtrAlloc( 4 );
            Vec_PtrPushUnique( vFanins, Ivy_ObjFanin0( Ivy_ObjFanin0(pNode) ) );
            Vec_PtrPushUnique( vFanins, Ivy_ObjFanin0( Ivy_ObjFanin1(pNode) ) );
            Vec_PtrPushUnique( vFanins, Ivy_ObjFanin1( Ivy_ObjFanin0(pNode) ) );
            Vec_PtrPushUnique( vFanins, Ivy_ObjFanin1( Ivy_ObjFanin1(pNode) ) );
            Vec_PtrForEachEntry( Ivy_Obj_t *, vFanins, pFanin, k )
                Ivy_FraigObjAddToFrontier( p, Ivy_Regular(pFanin), vFrontier );
            Ivy_FraigAddClausesMux( p, pNode );
        }
        else
        {
            vFanins = Ivy_FraigCollectSuper( pNode, fUseMuxes );
            Vec_PtrForEachEntry( Ivy_Obj_t *, vFanins, pFanin, k )
                Ivy_FraigObjAddToFrontier( p, Ivy_Regular(pFanin), vFrontier );
            Ivy_FraigAddClausesSuper( p, pNode, vFanins );
        }
        assert( Vec_PtrSize(vFanins) > 1 );
        Ivy_ObjSetFaninVec( pNode, vFanins );
    }
    Vec_PtrFree( vFrontier );
    sat_solver_simplify( p->pSat );
}

/**Function*************************************************************

  Synopsis    [Sets variable activities in the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigSetActivityFactors_rec( Ivy_FraigMan_t * p, Ivy_Obj_t * pObj, int LevelMin, int LevelMax )
{
    Vec_Ptr_t * vFanins;
    Ivy_Obj_t * pFanin;
    int i, Counter = 0;
    assert( !Ivy_IsComplement(pObj) );
    assert( Ivy_ObjSatNum(pObj) );
    // skip visited variables
    if ( Ivy_ObjIsTravIdCurrent(p->pManFraig, pObj) )
        return 0;
    Ivy_ObjSetTravIdCurrent(p->pManFraig, pObj);
    // add the PI to the list
    if ( pObj->Level <= (unsigned)LevelMin || Ivy_ObjIsPi(pObj) )
        return 0;
    // set the factor of this variable
    // (LevelMax-LevelMin) / (pObj->Level-LevelMin) = p->pParams->dActConeBumpMax / ThisBump
    p->pSat->factors[Ivy_ObjSatNum(pObj)] = p->pParams->dActConeBumpMax * (pObj->Level - LevelMin)/(LevelMax - LevelMin);
    veci_push(&p->pSat->act_vars, Ivy_ObjSatNum(pObj));
    // explore the fanins
    vFanins = Ivy_ObjFaninVec( pObj );
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFanins, pFanin, i )
        Counter += Ivy_FraigSetActivityFactors_rec( p, Ivy_Regular(pFanin), LevelMin, LevelMax );
    return 1 + Counter;
}

/**Function*************************************************************

  Synopsis    [Sets variable activities in the cone.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigSetActivityFactors( Ivy_FraigMan_t * p, Ivy_Obj_t * pOld, Ivy_Obj_t * pNew )
{
    int LevelMin, LevelMax;
    abctime clk;
    assert( pOld || pNew );
clk = Abc_Clock(); 
    // reset the active variables
    veci_resize(&p->pSat->act_vars, 0);
    // prepare for traversal
    Ivy_ManIncrementTravId( p->pManFraig );
    // determine the min and max level to visit
    assert( p->pParams->dActConeRatio > 0 && p->pParams->dActConeRatio < 1 );
    LevelMax = IVY_MAX( (pNew ? pNew->Level : 0), (pOld ? pOld->Level : 0) );
    LevelMin = (int)(LevelMax * (1.0 - p->pParams->dActConeRatio));
    // traverse
    if ( pOld && !Ivy_ObjIsConst1(pOld) )
        Ivy_FraigSetActivityFactors_rec( p, pOld, LevelMin, LevelMax );
    if ( pNew && !Ivy_ObjIsConst1(pNew) )
        Ivy_FraigSetActivityFactors_rec( p, pNew, LevelMin, LevelMax );
//Ivy_FraigPrintActivity( p );
p->timeTrav += Abc_Clock() - clk;
    return 1;
}

ABC_NAMESPACE_IMPL_END

#ifdef ABC_USE_CUDD
#include "bdd/cudd/cuddInt.h"
#endif

ABC_NAMESPACE_IMPL_START

#ifdef ABC_USE_CUDD

/**Function*************************************************************

  Synopsis    [Checks equivalence using BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
DdNode * Ivy_FraigNodesAreEquivBdd_int( DdManager * dd, DdNode * bFunc, Vec_Ptr_t * vFront, int Level )
{
    DdNode ** pFuncs;
    DdNode * bFuncNew;
    Vec_Ptr_t * vTemp;
    Ivy_Obj_t * pObj, * pFanin;
    int i, NewSize;
    // create new frontier
    vTemp = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pObj, i )
    {
        if ( (int)pObj->Level != Level )
        {
            pObj->fMarkB = 1;
            pObj->TravId = Vec_PtrSize(vTemp);
            Vec_PtrPush( vTemp, pObj );
            continue;
        }

        pFanin = Ivy_ObjFanin0(pObj);
        if ( pFanin->fMarkB == 0 )
        {
            pFanin->fMarkB = 1;
            pFanin->TravId = Vec_PtrSize(vTemp);
            Vec_PtrPush( vTemp, pFanin );
        }

        pFanin = Ivy_ObjFanin1(pObj);
        if ( pFanin->fMarkB == 0 )
        {
            pFanin->fMarkB = 1;
            pFanin->TravId = Vec_PtrSize(vTemp);
            Vec_PtrPush( vTemp, pFanin );
        }
    }
    // collect the permutation
    NewSize = IVY_MAX(dd->size, Vec_PtrSize(vTemp));
    pFuncs = ABC_ALLOC( DdNode *, NewSize );
    Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pObj, i )
    {
        if ( (int)pObj->Level != Level )
            pFuncs[i] = Cudd_bddIthVar( dd, pObj->TravId );
        else
            pFuncs[i] = Cudd_bddAnd( dd, 
                Cudd_NotCond( Cudd_bddIthVar(dd, Ivy_ObjFanin0(pObj)->TravId), Ivy_ObjFaninC0(pObj) ),
                Cudd_NotCond( Cudd_bddIthVar(dd, Ivy_ObjFanin1(pObj)->TravId), Ivy_ObjFaninC1(pObj) ) );
        Cudd_Ref( pFuncs[i] );
    }
    // add the remaining vars
    assert( NewSize == dd->size );
    for ( i = Vec_PtrSize(vFront); i < dd->size; i++ )
    {
        pFuncs[i] = Cudd_bddIthVar( dd, i );
        Cudd_Ref( pFuncs[i] );
    }

    // create new
    bFuncNew = Cudd_bddVectorCompose( dd, bFunc, pFuncs ); Cudd_Ref( bFuncNew );
    // clean trav Id
    Vec_PtrForEachEntry( Ivy_Obj_t *, vTemp, pObj, i )
    {
        pObj->fMarkB = 0;
        pObj->TravId = 0;
    }
    // deref
    for ( i = 0; i < dd->size; i++ )
        Cudd_RecursiveDeref( dd, pFuncs[i] );
    ABC_FREE( pFuncs );

    ABC_FREE( vFront->pArray );
    *vFront = *vTemp;

    vTemp->nCap = vTemp->nSize = 0;
    vTemp->pArray = NULL;
    Vec_PtrFree( vTemp );

    Cudd_Deref( bFuncNew );
    return bFuncNew;
}

/**Function*************************************************************

  Synopsis    [Checks equivalence using BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigNodesAreEquivBdd( Ivy_Obj_t * pObj1, Ivy_Obj_t * pObj2 )
{
    static DdManager * dd = NULL;
    DdNode * bFunc, * bTemp;
    Vec_Ptr_t * vFront;
    Ivy_Obj_t * pObj;
    int i, RetValue, Iter, Level;
    // start the manager
    if ( dd == NULL )
        dd = Cudd_Init( 50, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0 );
    // create front
    vFront = Vec_PtrAlloc( 100 );
    Vec_PtrPush( vFront, pObj1 );
    Vec_PtrPush( vFront, pObj2 );
    // get the function
    bFunc = Cudd_bddXor( dd, Cudd_bddIthVar(dd,0), Cudd_bddIthVar(dd,1) );  Cudd_Ref( bFunc );
    bFunc = Cudd_NotCond( bFunc, pObj1->fPhase != pObj2->fPhase );
    // try running BDDs
    for ( Iter = 0; ; Iter++ )
    {
        // find max level
        Level = 0;
        Vec_PtrForEachEntry( Ivy_Obj_t *, vFront, pObj, i )
            if ( Level < (int)pObj->Level )
                Level = (int)pObj->Level;
        if ( Level == 0 )
            break;            
        bFunc = Ivy_FraigNodesAreEquivBdd_int( dd, bTemp = bFunc, vFront, Level ); Cudd_Ref( bFunc );
        Cudd_RecursiveDeref( dd, bTemp );
        if ( bFunc == Cudd_ReadLogicZero(dd) ) // proved
            {printf( "%d", Iter ); break;}
        if ( Cudd_DagSize(bFunc) > 1000 )
            {printf( "b" ); break;}
        if ( dd->size > 120 )
            {printf( "s" ); break;}
        if ( Iter > 50 )
            {printf( "i" ); break;}
    }
    if ( bFunc == Cudd_ReadLogicZero(dd) ) // unsat
        RetValue = 1;
    else if ( Level == 0 ) // sat
        RetValue = 0;
    else 
        RetValue = -1; // spaceout/timeout
    Cudd_RecursiveDeref( dd, bFunc );
    Vec_PtrFree( vFront );
    return RetValue;
}
#endif

ABC_NAMESPACE_IMPL_END

#include "aig/aig/aig.h"

ABC_NAMESPACE_IMPL_START


/**Function*************************************************************

  Synopsis    [Computes truth table of the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_FraigExtractCone_rec( Ivy_Man_t * p, Ivy_Obj_t * pNode, Vec_Int_t * vLeaves, Vec_Int_t * vNodes )
{
    if ( pNode->fMarkB )
        return;
    pNode->fMarkB = 1;
    if ( Ivy_ObjIsPi(pNode) )
    {
        Vec_IntPush( vLeaves, pNode->Id );
        return;
    }
    assert( Ivy_ObjIsAnd(pNode) );
    Ivy_FraigExtractCone_rec( p, Ivy_ObjFanin0(pNode), vLeaves, vNodes );
    Ivy_FraigExtractCone_rec( p, Ivy_ObjFanin1(pNode), vLeaves, vNodes );
    Vec_IntPush( vNodes, pNode->Id );
}

/**Function*************************************************************

  Synopsis    [Checks equivalence using BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Ivy_FraigExtractCone( Ivy_Man_t * p, Ivy_Obj_t * pObj1, Ivy_Obj_t * pObj2, Vec_Int_t * vLeaves )
{
    Aig_Man_t * pMan;
    Aig_Obj_t * pMiter;
    Vec_Int_t * vNodes;
    Ivy_Obj_t * pObjIvy;
    int i;
    // collect nodes
    vNodes  = Vec_IntAlloc( 100 );
    Ivy_ManConst1(p)->fMarkB = 1;
    Ivy_FraigExtractCone_rec( p, pObj1, vLeaves, vNodes );
    Ivy_FraigExtractCone_rec( p, pObj2, vLeaves, vNodes );
    Ivy_ManConst1(p)->fMarkB = 0;
    // create new manager
    pMan = Aig_ManStart( 1000 );
    Ivy_ManConst1(p)->pEquiv = (Ivy_Obj_t *)Aig_ManConst1(pMan);
    Ivy_ManForEachNodeVec( p, vLeaves, pObjIvy, i )
    {
        pObjIvy->pEquiv = (Ivy_Obj_t *)Aig_ObjCreateCi( pMan );
        pObjIvy->fMarkB = 0;
    }
    // duplicate internal nodes
    Ivy_ManForEachNodeVec( p, vNodes, pObjIvy, i )
    {

        pObjIvy->pEquiv = (Ivy_Obj_t *)Aig_And( pMan, (Aig_Obj_t *)Ivy_ObjChild0Equiv(pObjIvy), (Aig_Obj_t *)Ivy_ObjChild1Equiv(pObjIvy) );
        pObjIvy->fMarkB = 0;

        pMiter = (Aig_Obj_t *)pObjIvy->pEquiv;
        assert( pMiter->fPhase == pObjIvy->fPhase );
    }
    // create the PO
    pMiter = Aig_Exor( pMan, (Aig_Obj_t *)pObj1->pEquiv, (Aig_Obj_t *)pObj2->pEquiv );
    pMiter = Aig_NotCond( pMiter, Aig_Regular(pMiter)->fPhase ^ Aig_IsComplement(pMiter) );

/*
printf( "Polarity = %d\n", pMiter->fPhase );
    if ( Ivy_ObjIsConst1(pObj1) || Ivy_ObjIsConst1(pObj2) )
    {
        pMiter = Aig_NotCond( pMiter, 1 );
printf( "***************\n" );
    }
*/
    pMiter = Aig_ObjCreateCo( pMan, pMiter );
//printf( "Polarity = %d\n", pMiter->fPhase );
    Aig_ManCleanup( pMan );
    Vec_IntFree( vNodes );
    return pMan;
}

/**Function*************************************************************

  Synopsis    [Checks equivalence using BDDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_FraigCheckCone( Ivy_FraigMan_t * pGlo, Ivy_Man_t * p, Ivy_Obj_t * pObj1, Ivy_Obj_t * pObj2, int nConfLimit )
{
    extern int Fra_FraigSat( Aig_Man_t * pMan, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, int nLearnedStart, int nLearnedDelta, int nLearnedPerce, int fFlipBits, int fAndOuts, int fNewSolver, int fVerbose );
    Vec_Int_t * vLeaves;
    Aig_Man_t * pMan;
    Aig_Obj_t * pObj;
    int i, RetValue;
    vLeaves  = Vec_IntAlloc( 100 );
    pMan     = Ivy_FraigExtractCone( p, pObj1, pObj2, vLeaves );
    RetValue = Fra_FraigSat( pMan, nConfLimit, 0, 0, 0, 0, 0, 0, 0, 1 ); 
    if ( RetValue == 0 )
    {
        int Counter = 0;
        memset( pGlo->pPatWords, 0, sizeof(unsigned) * pGlo->nPatWords );
        Aig_ManForEachCi( pMan, pObj, i )
            if ( ((int *)pMan->pData)[i] )
            {
                int iObjIvy = Vec_IntEntry( vLeaves, i );
                assert( iObjIvy > 0 && iObjIvy <= Ivy_ManPiNum(p) );
                Ivy_InfoSetBit( pGlo->pPatWords, iObjIvy-1 );
//printf( "%d ", iObjIvy );
Counter++;
            }
        assert( Counter > 0 );
    }
    Vec_IntFree( vLeaves );
    if ( RetValue == 1 )
        printf( "UNSAT\n" );
    else if ( RetValue == 0 )
        printf( "SAT\n" );
    else if ( RetValue == -1 )
        printf( "UNDEC\n" );

//    p->pModel = (int *)pMan->pData, pMan2->pData = NULL;
    Aig_ManStop( pMan );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

