/**CFile****************************************************************

  FileName    [sfmDec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [SAT-based decomposition.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sfmDec.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sfmInt.h"
#include "misc/st/st.h"
#include "map/mio/mio.h"
#include "base/abc/abc.h"
#include "misc/util/utilTruth.h"
#include "opt/dau/dau.h"
#include "map/mio/exp.h"
#include "map/scl/sclCon.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Sfm_Dec_t_ Sfm_Dec_t;
struct Sfm_Dec_t_
{
    // external
    Sfm_Par_t *       pPars;       // parameters
    Sfm_Lib_t *       pLib;        // library
    Sfm_Tim_t *       pTim;        // timing
    Sfm_Mit_t *       pMit;        // timing
    Abc_Ntk_t *       pNtk;        // network
    // library
    Vec_Int_t         vGateSizes;  // fanin counts
    Vec_Wrd_t         vGateFuncs;  // gate truth tables
    Vec_Wec_t         vGateCnfs;   // gate CNFs
    Vec_Ptr_t         vGateHands;  // gate handles
    int               GateConst0;  // special gates
    int               GateConst1;  // special gates
    int               GateBuffer;  // special gates
    int               GateInvert;  // special gates
    int               GateAnd[4];  // special gates
    int               GateOr[4];   // special gates
    // objects
    int               nDivs;       // the number of divisors
    int               nMffc;       // the number of divisors
    int               AreaMffc;    // the area of gates in MFFC
    int               DelayMin;    // temporary min delay
    int               iTarget;     // target node
    int               iUseThis;    // next cofactoring var to try
    int               DeltaCrit;   // critical delta
    int               AreaInv;     // inverter area
    int               DelayInv;    // inverter delay
    Mio_Gate_t *      pGateInv;    // inverter
    word              uCareSet;    // computed careset
    Vec_Int_t         vObjRoots;   // roots of the window
    Vec_Int_t         vObjGates;   // functionality
    Vec_Wec_t         vObjFanins;  // fanin IDs
    Vec_Int_t         vObjMap;     // object map
    Vec_Int_t         vObjDec;     // decomposition
    Vec_Int_t         vObjMffc;    // MFFC nodes
    Vec_Int_t         vObjInMffc;  // inputs of MFFC nodes
    Vec_Wrd_t         vObjSims;    // simulation patterns
    Vec_Wrd_t         vObjSims2;   // simulation patterns
    Vec_Ptr_t         vMatchGates; // matched gates
    Vec_Ptr_t         vMatchFans;  // matched fanins
    // solver
    sat_solver *      pSat;        // reusable solver 
    Vec_Wec_t         vClauses;    // CNF clauses for the node
    Vec_Int_t         vImpls[2];   // onset/offset implications
    Vec_Wrd_t         vSets[2];    // onset/offset patterns
    int               nPats[2];    // CEX count
    int               nPatWords[2];// CEX words
    int               nDivWords;   // div words
    int               nDivWordsAlloc; // div words
    word              TtElems[SFM_SUPP_MAX][SFM_WORD_MAX];
    word *            pTtElems[SFM_SUPP_MAX];
    word *            pDivWords[SFM_SUPP_MAX];
    // temporary
    Vec_Int_t         vNewNodes;
    Vec_Int_t         vGateTfi;
    Vec_Int_t         vGateTfo;
    Vec_Int_t         vGateCut;
    Vec_Int_t         vGateTemp;
    Vec_Int_t         vGateMffc;
    Vec_Int_t         vCands;
    word              Copy[4];
    int               nSuppVars;
    // statistics
    abctime           timeLib;
    abctime           timeWin;
    abctime           timeCnf;
    abctime           timeSat;
    abctime           timeSatSat;
    abctime           timeSatUnsat;
    abctime           timeEval;
    abctime           timeTime;
    abctime           timeOther;
    abctime           timeStart;
    abctime           timeTotal;
    int               nTotalNodesBeg;
    int               nTotalEdgesBeg;
    int               nTotalNodesEnd;
    int               nTotalEdgesEnd;
    int               nNodesTried;
    int               nNodesChanged;
    int               nNodesConst0;
    int               nNodesConst1;
    int               nNodesBuf;
    int               nNodesInv;
    int               nNodesAndOr;
    int               nNodesResyn;
    int               nSatCalls;
    int               nSatCallsSat;
    int               nSatCallsUnsat;
    int               nSatCallsOver;
    int               nTimeOuts;
    int               nNoDecs;
    int               nEfforts;
    int               nMaxDivs;
    int               nMaxWin;
    word              nAllDivs;
    word              nAllWin;
    int               nLuckySizes[SFM_SUPP_MAX+1];
    int               nLuckyGates[SFM_SUPP_MAX+1];
};

#define SFM_MASK_PI     1  // supp(node) is contained in supp(TFI(pivot))
#define SFM_MASK_INPUT  2  // supp(node) does not overlap with supp(TFI(pivot))
#define SFM_MASK_FANIN  4  // the same as above (pointed to by node with SFM_MASK_PI | SFM_MASK_INPUT)
#define SFM_MASK_MFFC   8  // MFFC nodes, including the target node
#define SFM_MASK_PIVOT 16  // the target node

static inline Sfm_Dec_t * Sfm_DecMan( Abc_Obj_t * p )                        { return (Sfm_Dec_t *)p->pNtk->pData;                  }
static inline word        Sfm_DecObjSim( Sfm_Dec_t * p, Abc_Obj_t * pObj )   { return Vec_WrdEntry(&p->vObjSims, Abc_ObjId(pObj));  }
static inline word        Sfm_DecObjSim2( Sfm_Dec_t * p, Abc_Obj_t * pObj )  { return Vec_WrdEntry(&p->vObjSims2, Abc_ObjId(pObj)); }
static inline word *      Sfm_DecDivPats( Sfm_Dec_t * p, int d, int c )      { return Vec_WrdEntryP(&p->vSets[c], d*SFM_SIM_WORDS); }

static inline int         Sfm_ManReadObjDelay( Sfm_Dec_t * p, int Id )       { return p->pMit ? Sfm_MitReadObjDelay(p->pMit, Id) : Sfm_TimReadObjDelay(p->pTim, Id); }
static inline int         Sfm_ManReadNtkDelay( Sfm_Dec_t * p )               { return p->pMit ? Sfm_MitReadNtkDelay(p->pMit)     : Sfm_TimReadNtkDelay(p->pTim);     }
static inline int         Sfm_ManReadNtkMinSlack( Sfm_Dec_t * p )            { return p->pMit ? Sfm_MitReadNtkMinSlack(p->pMit)  : 0;                                }



////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Setup parameter structure.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_ParSetDefault3( Sfm_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Sfm_Par_t) );
    pPars->nTfoLevMax   =  100;  // the maximum fanout levels
    pPars->nTfiLevMax   =  100;  // the maximum fanin levels
    pPars->nFanoutMax   =   10;  // the maximum number of fanouts
    pPars->nMffcMin     =    1;  // the maximum MFFC size
    pPars->nMffcMax     =    3;  // the maximum MFFC size
    pPars->nVarMax      =    6;  // the maximum variable count
    pPars->nDecMax      =    1;  // the maximum number of decompositions
    pPars->nWinSizeMax  =    0;  // the maximum window size
    pPars->nGrowthLevel =    0;  // the maximum allowed growth in level
    pPars->nBTLimit     =    0;  // the maximum number of conflicts in one SAT run
    pPars->nTimeWin     =    1;  // the size of timing window in percents
    pPars->DeltaCrit    =    0;  // delta delay in picoseconds
    pPars->fUseAndOr    =    0;  // enable internal detection of AND/OR gates
    pPars->fZeroCost    =    0;  // enable zero-cost replacement
    pPars->fMoreEffort  =    0;  // enables using more effort
    pPars->fUseSim      =    0;  // enable simulation
    pPars->fArea        =    0;  // performs optimization for area
    pPars->fVerbose     =    0;  // enable basic stats
    pPars->fVeryVerbose =    0;  // enable detailed stats
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sfm_Dec_t * Sfm_DecStart( Sfm_Par_t * pPars, Mio_Library_t * pLib, Abc_Ntk_t * pNtk )
{
    extern void Sfm_LibPreprocess( Mio_Library_t * pLib, Vec_Int_t * vGateSizes, Vec_Wrd_t * vGateFuncs, Vec_Wec_t * vGateCnfs, Vec_Ptr_t * vGateHands );
    Sfm_Dec_t * p = ABC_CALLOC( Sfm_Dec_t, 1 ); int i;
    p->timeStart = Abc_Clock();
    p->pPars     = pPars;
    p->pNtk      = pNtk;
    p->pSat      = sat_solver_new();
    p->pGateInv  = Mio_LibraryReadInv( pLib );
    p->AreaInv   = Scl_Flt2Int(Mio_GateReadArea(p->pGateInv));
    p->DelayInv  = Scl_Flt2Int(Mio_GateReadDelayMax(p->pGateInv));
    p->DeltaCrit = pPars->DeltaCrit ? Scl_Flt2Int((float)pPars->DeltaCrit) : 5 * Scl_Flt2Int(Mio_LibraryReadDelayInvMax(pLib)) / 2;
p->timeLib = Abc_Clock();
    p->pLib = Sfm_LibPrepare( pPars->nVarMax, 1, !pPars->fArea, pPars->fVerbose, pPars->fLibVerbose );
p->timeLib = Abc_Clock() - p->timeLib;
    if ( !pPars->fArea )
    {
        if ( Abc_FrameReadLibScl() )
            p->pMit = Sfm_MitStart( pLib, (SC_Lib *)Abc_FrameReadLibScl(), Scl_ConReadMan(), pNtk, p->DeltaCrit );
        if ( p->pMit == NULL )
            p->pTim = Sfm_TimStart( pLib, Scl_ConReadMan(), pNtk, p->DeltaCrit );
    }
    if ( pPars->fVeryVerbose )
//    if ( pPars->fVerbose )
        Sfm_LibPrint( p->pLib );
    pNtk->pData = p;
    // enter library
    assert( Abc_NtkIsMappedLogic(pNtk) );
    Sfm_LibPreprocess( pLib, &p->vGateSizes, &p->vGateFuncs, &p->vGateCnfs, &p->vGateHands );
    p->GateConst0 = Mio_GateReadValue( Mio_LibraryReadConst0(pLib) );
    p->GateConst1 = Mio_GateReadValue( Mio_LibraryReadConst1(pLib) );
    p->GateBuffer = Mio_GateReadValue( Mio_LibraryReadBuf(pLib) );
    p->GateInvert = Mio_GateReadValue( Mio_LibraryReadInv(pLib) );
    // elementary truth tables
    for ( i = 0; i < SFM_SUPP_MAX; i++ )
        p->pTtElems[i] = p->TtElems[i];
    Abc_TtElemInit( p->pTtElems, SFM_SUPP_MAX );
    p->iUseThis = -1;
    return p;
}
void Sfm_DecStop( Sfm_Dec_t * p )
{
    Abc_Ntk_t * pNtk = p->pNtk;
    Abc_Obj_t * pObj; int i;
    Abc_NtkForEachNode( pNtk, pObj, i )
        if ( (int)pObj->Level != Abc_ObjLevelNew(pObj) )
            printf( "Level count mismatch at node %d.\n", i );
    Sfm_LibStop( p->pLib );
    if ( p->pTim ) Sfm_TimStop( p->pTim );
    if ( p->pMit ) Sfm_MitStop( p->pMit );
    // divisors
    for ( i = 0; i < SFM_SUPP_MAX; i++ )
        ABC_FREE( p->pDivWords[i] );
    // library
    Vec_IntErase( &p->vGateSizes );
    Vec_WrdErase( &p->vGateFuncs );
    Vec_WecErase( &p->vGateCnfs );
    Vec_PtrErase( &p->vGateHands );
    // objects
    Vec_IntErase( &p->vObjRoots );
    Vec_IntErase( &p->vObjGates );
    Vec_WecErase( &p->vObjFanins );
    Vec_IntErase( &p->vObjMap );
    Vec_IntErase( &p->vObjDec );
    Vec_IntErase( &p->vObjMffc );
    Vec_IntErase( &p->vObjInMffc );
    Vec_WrdErase( &p->vObjSims );
    Vec_WrdErase( &p->vObjSims2 );
    Vec_PtrErase( &p->vMatchGates );
    Vec_PtrErase( &p->vMatchFans );
    // solver
    sat_solver_delete( p->pSat );
    Vec_WecErase( &p->vClauses );
    Vec_IntErase( &p->vImpls[0] );
    Vec_IntErase( &p->vImpls[1] );
    Vec_WrdErase( &p->vSets[0] );
    Vec_WrdErase( &p->vSets[1] );
    // temporary
    Vec_IntErase( &p->vNewNodes );
    Vec_IntErase( &p->vGateTfi );
    Vec_IntErase( &p->vGateTfo );
    Vec_IntErase( &p->vGateCut );
    Vec_IntErase( &p->vGateTemp );
    Vec_IntErase( &p->vGateMffc );
    Vec_IntErase( &p->vCands );
    ABC_FREE( p );
    pNtk->pData = NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Sfm_ObjSimulate( Abc_Obj_t * pObj )
{
    Sfm_Dec_t * p = Sfm_DecMan( pObj );
    Vec_Int_t * vExpr = Mio_GateReadExpr( (Mio_Gate_t *)pObj->pData );
    Abc_Obj_t * pFanin; int i; word uFanins[6];
    assert( Abc_ObjFaninNum(pObj) <= 6 );
    Abc_ObjForEachFanin( pObj, pFanin, i )
        uFanins[i] = Sfm_DecObjSim( p, pFanin );
    return Exp_Truth6( Abc_ObjFaninNum(pObj), vExpr, uFanins );
}
static inline word Sfm_ObjSimulate2( Abc_Obj_t * pObj )
{
    Sfm_Dec_t * p = Sfm_DecMan( pObj );
    Vec_Int_t * vExpr = Mio_GateReadExpr( (Mio_Gate_t *)pObj->pData );
    Abc_Obj_t * pFanin; int i; word uFanins[6];
    Abc_ObjForEachFanin( pObj, pFanin, i )
        if ( (pFanin->iTemp & SFM_MASK_PIVOT) )
            uFanins[i] = Sfm_DecObjSim2( p, pFanin );
        else
            uFanins[i] = Sfm_DecObjSim( p, pFanin );
    return Exp_Truth6( Abc_ObjFaninNum(pObj), vExpr, uFanins );
}
static inline void Sfm_NtkSimulate( Abc_Ntk_t * pNtk )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj; int i; word uTemp;
    Sfm_Dec_t * p = Sfm_DecMan( Abc_NtkPi(pNtk, 0) );
    Vec_WrdFill( &p->vObjSims,  2*Abc_NtkObjNumMax(pNtk), 0 );
    Vec_WrdFill( &p->vObjSims2, 2*Abc_NtkObjNumMax(pNtk), 0 );
    Gia_ManRandomW(1);
    assert( p->pPars->fUseSim );
    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        Vec_WrdWriteEntry( &p->vObjSims, Abc_ObjId(pObj), (uTemp = Gia_ManRandomW(0)) );
        //printf( "Inpt = %5d : ", Abc_ObjId(pObj) );
        //Extra_PrintBinary( stdout, (unsigned *)&uTemp, 64 );
        //printf( "\n" );
    }
    vNodes = Abc_NtkDfs( pNtk, 1 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        Vec_WrdWriteEntry( &p->vObjSims, Abc_ObjId(pObj), (uTemp = Sfm_ObjSimulate(pObj)) );
        //printf( "Obj  = %5d : ", Abc_ObjId(pObj) );
        //Extra_PrintBinary( stdout, (unsigned *)&uTemp, 64 );
        //printf( "\n" );
    }
    Vec_PtrFree( vNodes );
}
static inline void Sfm_ObjSimulateNode( Abc_Obj_t * pObj )
{
    Sfm_Dec_t * p = Sfm_DecMan( pObj );
    if ( !p->pPars->fUseSim )
        return;
    Vec_WrdWriteEntry( &p->vObjSims, Abc_ObjId(pObj), Sfm_ObjSimulate(pObj) );
    if ( (pObj->iTemp & SFM_MASK_PIVOT) )
        Vec_WrdWriteEntry( &p->vObjSims2, Abc_ObjId(pObj), Sfm_ObjSimulate2(pObj) );
}
static inline void Sfm_ObjFlipNode( Abc_Obj_t * pObj )
{
    Sfm_Dec_t * p = Sfm_DecMan( pObj );
    if ( !p->pPars->fUseSim )
        return;
    Vec_WrdWriteEntry( &p->vObjSims2, Abc_ObjId(pObj), ~Sfm_DecObjSim(p, pObj) );
}
static inline word Sfm_ObjFindCareSet( Abc_Ntk_t * pNtk, Vec_Int_t * vRoots )
{
    Sfm_Dec_t * p = Sfm_DecMan( Abc_NtkPi(pNtk, 0) );
    Abc_Obj_t * pObj; int i; word Res = 0;
    if ( !p->pPars->fUseSim )
        return 0;
    Abc_NtkForEachObjVec( vRoots, pNtk, pObj, i )
        Res |= Sfm_DecObjSim(p, pObj) ^ Sfm_DecObjSim2(p, pObj);
    return Res;
}
static inline void Sfm_ObjSetupSimInfo( Abc_Obj_t * pObj )
{
    Sfm_Dec_t * p = Sfm_DecMan( pObj ); int i;
    p->nPats[0]     = p->nPats[1] = 0;
    p->nPatWords[0] = p->nPatWords[1] = 0;
    Vec_WrdFill( &p->vSets[0], p->nDivs*SFM_SIM_WORDS, 0 );
    Vec_WrdFill( &p->vSets[1], p->nDivs*SFM_SIM_WORDS, 0 );
    // alloc divwords
    p->nDivWords = Abc_Bit6WordNum( 4 * p->nDivs );
    if ( p->nDivWordsAlloc < p->nDivWords )
    {
        p->nDivWordsAlloc = Abc_MaxInt( 16, p->nDivWords );
        for ( i = 0; i < SFM_SUPP_MAX; i++ )
            p->pDivWords[i] = ABC_REALLOC( word, p->pDivWords[i], p->nDivWordsAlloc );
    }
    memset( p->pDivWords[0], 0, sizeof(word) * p->nDivWords );
    // collect simulation info
    if ( p->pPars->fUseSim && p->uCareSet != 0 )
    {
        word uCareSet = p->uCareSet;
        word uValues  = Sfm_DecObjSim(p, pObj);
        int c, d, i, Indexes[2][64];
        assert( p->iTarget == pObj->iTemp );
        assert( p->pPars->fUseSim );
        // find what patterns go to on-set/off-set
        for ( i = 0; i < 64; i++ )
            if ( (uCareSet >> i) & 1 )
            {
                c = !((uValues >> i) & 1);
                Indexes[c][p->nPats[c]++] = i;
            }
        for ( c = 0; c < 2; c++ )
            p->nPatWords[c] = 1 + (p->nPats[c] >> 6);
        // write patterns
        for ( d = 0; d < p->nDivs; d++ )
        {
            word uSim = Vec_WrdEntry( &p->vObjSims, Vec_IntEntry(&p->vObjMap, d) );
            for ( c = 0; c < 2; c++ )
                for ( i = 0; i < p->nPats[c]; i++ )
                    if ( (uSim >> Indexes[c][i]) & 1 )
                        Abc_TtSetBit( Sfm_DecDivPats(p, d, c), i );
        }
        //printf( "Node %d : Onset = %d. Offset = %d.\n", pObj->Id, p->nPats[0], p->nPats[1] );
    }
}
static inline void Sfm_ObjSetdownSimInfo( Abc_Obj_t * pObj )
{
    int nPatKeep = 32;
    Sfm_Dec_t * p = Sfm_DecMan( pObj );
    int c, d; word uSim, uSims[2], uMask;
    if ( !p->pPars->fUseSim )
        return;
    for ( d = 0; d < p->nDivs; d++ )
    {
        uSim = Vec_WrdEntry( &p->vObjSims, Vec_IntEntry(&p->vObjMap, d) );
        for ( c = 0; c < 2; c++ )
        {
            uMask = Abc_Tt6Mask( Abc_MinInt(p->nPats[c], nPatKeep) );
            uSims[c] = (Sfm_DecDivPats(p, d, c)[0] & uMask) | (uSim & ~uMask);
            uSim >>= 32;
        }
        uSim = (uSims[0] & 0xFFFFFFFF) | (uSims[1] << 32);
        Vec_WrdWriteEntry( &p->vObjSims, Vec_IntEntry(&p->vObjMap, d), uSim );
    }
}
/*
void Sfm_ObjSetdownSimInfo( Abc_Obj_t * pObj )
{
    int nPatKeep = 32;
    Sfm_Dec_t * p = Sfm_DecMan( pObj );
    word uSim, uMaskKeep[2];
    int c, d, nKeeps[2]; 
    for ( c = 0; c < 2; c++ )
    {
        nKeeps[c] = Abc_MaxInt(p->nPats[c], nPatKeep);
        uMaskKeep[c] = Abc_Tt6Mask( nKeeps[c] );
    }
    for ( d = 0; d < p->nDivs; d++ )
    {
        uSim = Vec_WrdEntry( &p->vObjSims, Vec_IntEntry(&p->vObjMap, d) ) << (nKeeps[0] + nKeeps[1]);
        uSim |= (Vec_WrdEntry(&p->vSets[0], d) & uMaskKeep[0]) | ((Vec_WrdEntry(&p->vSets[1], d) & uMaskKeep[1]) << nKeeps[0]);
        Vec_WrdWriteEntry( &p->vObjSims, Vec_IntEntry(&p->vObjMap, d), uSim );
    }
}
*/

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_DecPrepareSolver( Sfm_Dec_t * p )
{
    Vec_Int_t * vRoots = &p->vObjRoots;
    Vec_Int_t * vFaninVars = &p->vGateTemp;
    Vec_Int_t * vLevel, * vClause;
    int i, k, Gate, iObj, RetValue;
    int nTfiSize = p->iTarget + 1; // including node
    int nWinSize = Vec_IntSize(&p->vObjGates);
    int nSatVars = 2 * nWinSize - nTfiSize;
    assert( nWinSize == Vec_IntSize(&p->vObjGates) );
    assert( p->iTarget < nWinSize );
    // create SAT solver
    sat_solver_restart( p->pSat );
    sat_solver_setnvars( p->pSat, nSatVars + Vec_IntSize(vRoots) );
    // add CNF clauses for the TFI
    Vec_IntForEachEntry( &p->vObjGates, Gate, i )
    {
        if ( Gate == -1 )
            continue;
        // generate CNF 
        vLevel = Vec_WecEntry( &p->vObjFanins, i );
        Vec_IntPush( vLevel, i );
        Sfm_TranslateCnf( &p->vClauses, (Vec_Str_t *)Vec_WecEntry(&p->vGateCnfs, Gate), vLevel, -1 );
        Vec_IntPop( vLevel );
        // add clauses
        Vec_WecForEachLevel( &p->vClauses, vClause, k )
        {
            if ( Vec_IntSize(vClause) == 0 )
                break;
            RetValue = sat_solver_addclause( p->pSat, Vec_IntArray(vClause), Vec_IntArray(vClause) + Vec_IntSize(vClause) );
            if ( RetValue == 0 )
                return 0;
        }
    }
    // add CNF clauses for the TFO
    Vec_IntForEachEntryStart( &p->vObjGates, Gate, i, nTfiSize )
    {
        assert( Gate != -1 );
        vLevel = Vec_WecEntry( &p->vObjFanins, i );
        Vec_IntClear( vFaninVars );
        Vec_IntForEachEntry( vLevel, iObj, k )
            Vec_IntPush( vFaninVars, iObj <= p->iTarget ? iObj : iObj + nWinSize - nTfiSize );
        Vec_IntPush( vFaninVars, i + nWinSize - nTfiSize );
        // generate CNF 
        Sfm_TranslateCnf( &p->vClauses, (Vec_Str_t *)Vec_WecEntry(&p->vGateCnfs, Gate), vFaninVars, p->iTarget );
        // add clauses
        Vec_WecForEachLevel( &p->vClauses, vClause, k )
        {
            if ( Vec_IntSize(vClause) == 0 )
                break;
            RetValue = sat_solver_addclause( p->pSat, Vec_IntArray(vClause), Vec_IntArray(vClause) + Vec_IntSize(vClause) );
            if ( RetValue == 0 )
                return 0;
        }
    }
    if ( nTfiSize < nWinSize )
    {
        // create XOR clauses for the roots
        Vec_IntClear( vFaninVars );
        Vec_IntForEachEntry( vRoots, iObj, i )
        {
            Vec_IntPush( vFaninVars, Abc_Var2Lit(nSatVars, 0) );
            sat_solver_add_xor( p->pSat, iObj, iObj + nWinSize - nTfiSize, nSatVars++, 0 );
        }
        // make OR clause for the last nRoots variables
        RetValue = sat_solver_addclause( p->pSat, Vec_IntArray(vFaninVars), Vec_IntLimit(vFaninVars) );
        if ( RetValue == 0 )
            return 0;
        assert( nSatVars == sat_solver_nvars(p->pSat) );
    }
    else assert( Vec_IntSize(vRoots) == 1 );
    // finalize
    RetValue = sat_solver_simplify( p->pSat );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_DecFindCost( Sfm_Dec_t * p, int c, int iLit, word * pMask )
{
    word * pPats = Sfm_DecDivPats( p, Abc_Lit2Var(iLit), !c );
    return Abc_TtCountOnesVecMask( pPats, pMask, p->nPatWords[!c], Abc_LitIsCompl(iLit) );
}
void Sfm_DecPrint( Sfm_Dec_t * p, word Masks[2][SFM_SIM_WORDS] )
{
    int c, i, k, Entry;
    for ( c = 0; c < 2; c++ )
    {
        Vec_Int_t * vLevel = Vec_WecEntry( &p->vObjFanins, p->iTarget );
        printf( "%s-SET of object %d (divs = %d) with gate \"%s\" and fanins: ", 
            c ? "OFF": "ON", p->iTarget, p->nDivs, 
            Mio_GateReadName((Mio_Gate_t *)Vec_PtrEntry(&p->vGateHands, Vec_IntEntry(&p->vObjGates,p->iTarget))) );
        Vec_IntForEachEntry( vLevel, Entry, i )
            printf( "%d ", Entry );
        printf( "\n" );

        printf( "Implications: " );
        Vec_IntForEachEntry( &p->vImpls[c], Entry, i )
            printf( "%s%d(%d) ", Abc_LitIsCompl(Entry)? "!":"", Abc_Lit2Var(Entry), Sfm_DecFindCost(p, c, Entry, Masks[!c]) );
        printf( "\n" );
        printf( "     " );
        for ( i = 0; i < p->nDivs; i++ )
            printf( "%d", (i / 10) % 10 );
        printf( "\n" );
        printf( "     " );
        for ( i = 0; i < p->nDivs; i++ )
            printf( "%d", i % 10 );
        printf( "\n" );
        for ( k = 0; k < p->nPats[c]; k++ )
        {
            printf( "%2d : ", k );
            for ( i = 0; i < p->nDivs; i++ )
                printf( "%d", Abc_TtGetBit(Sfm_DecDivPats(p, i, c), k) );
            printf( "\n" );
        }
        //printf( "\n" );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_DecVarCost( Sfm_Dec_t * p, word Masks[2][SFM_SIM_WORDS], int d, int Counts[2][2] )
{
    int c; 
    for ( c = 0; c < 2; c++ )
    {
        word * pPats = Sfm_DecDivPats( p, d, c );
        int Num = Abc_TtCountOnesVec( Masks[c], p->nPatWords[c] );
        Counts[c][1] = Abc_TtCountOnesVecMask( pPats, Masks[c], p->nPatWords[c], 0 );
        Counts[c][0] = Num - Counts[c][1];
        assert( Counts[c][0] >= 0 && Counts[c][1] >= 0 );
    }
    //printf( "%5d %5d   %5d %5d \n", Counts[0][0], Counts[0][1], Counts[1][0], Counts[1][1] );
}
int Sfm_DecFindBestVar2( Sfm_Dec_t * p, word Masks[2][SFM_SIM_WORDS] )
{
    int Counts[2][2];
    int d, VarBest = -1, CostBest = ABC_INFINITY, Cost;
    for ( d = 0; d < p->nDivs; d++ )
    {
        Sfm_DecVarCost( p, Masks, d, Counts );
        if ( (Counts[0][0] < Counts[0][1]) == (Counts[1][0] < Counts[1][1]) )
            continue;
        Cost = Abc_MinInt(Counts[0][0], Counts[0][1]) + Abc_MinInt(Counts[1][0], Counts[1][1]);
        if ( CostBest > Cost )
        {
            CostBest = Cost;
            VarBest = d;
        }
    }
    return VarBest;
}
int Sfm_DecFindBestVar( Sfm_Dec_t * p, word Masks[2][SFM_SIM_WORDS] )
{
    int c, i, iLit, Var = -1, Cost, CostMin = ABC_INFINITY;
    for ( c = 0; c < 2; c++ )
    {
        Vec_IntForEachEntry( &p->vImpls[c], iLit, i )
        {
            if ( Vec_IntSize(&p->vImpls[c]) > 1 && Vec_IntFind(&p->vObjDec, Abc_Lit2Var(iLit)) >= 0 )
                continue;
            Cost = Sfm_DecFindCost( p, c, iLit, Masks[!c] );
            if ( CostMin > Cost )
            {
                CostMin = Cost;
                Var = Abc_Lit2Var(iLit);
            }
        }
    }
    return Var;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_DecMffcArea( Abc_Ntk_t * pNtk, Vec_Int_t * vMffc )
{
    Abc_Obj_t * pObj; 
    int i, nAreaMffc = 0;
    Abc_NtkForEachObjVec( vMffc, pNtk, pObj, i )
        nAreaMffc += Scl_Flt2Int(Mio_GateReadArea((Mio_Gate_t *)pObj->pData));
    return nAreaMffc;
}
int Sfm_MffcDeref_rec( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin;
    int i, Area = Scl_Flt2Int(Mio_GateReadArea((Mio_Gate_t *)pObj->pData));
    Abc_ObjForEachFanin( pObj, pFanin, i )
    {
        assert( pFanin->vFanouts.nSize > 0 );
        if ( --pFanin->vFanouts.nSize == 0 && !Abc_ObjIsCi(pFanin) )
            Area += Sfm_MffcDeref_rec( pFanin );
    }
    return Area;
}
int Sfm_MffcRef_rec( Abc_Obj_t * pObj, Vec_Int_t * vMffc )
{
    Abc_Obj_t * pFanin;
    int i, Area = Scl_Flt2Int(Mio_GateReadArea((Mio_Gate_t *)pObj->pData));
    Abc_ObjForEachFanin( pObj, pFanin, i )
    {
        if ( pFanin->vFanouts.nSize++ == 0 && !Abc_ObjIsCi(pFanin) )
            Area += Sfm_MffcRef_rec( pFanin, vMffc );
    }
    if ( vMffc )
        Vec_IntPush( vMffc, Abc_ObjId(pObj) );
    return Area;
}
int Sfm_DecMffcAreaReal( Abc_Obj_t * pPivot, Vec_Int_t * vCut, Vec_Int_t * vMffc )
{
    Abc_Ntk_t * pNtk = pPivot->pNtk;
    Abc_Obj_t * pObj; 
    int i, Area1, Area2;
    assert( Abc_ObjIsNode(pPivot) );
    if ( vMffc )
        Vec_IntClear( vMffc );
    Abc_NtkForEachObjVec( vCut, pNtk, pObj, i )
        pObj->vFanouts.nSize++;
    Area1 = Sfm_MffcDeref_rec( pPivot );
    Area2 = Sfm_MffcRef_rec( pPivot, vMffc );
    Abc_NtkForEachObjVec( vCut, pNtk, pObj, i )
        pObj->vFanouts.nSize--;
    assert( Area1 == Area2 );
    return Area1;
}
void Sfm_DecPrepareVec( Vec_Int_t * vMap, int * pNodes, int nNodes, Vec_Int_t * vCut )
{
    int i;
    Vec_IntClear( vCut );
    for ( i = 0; i < nNodes; i++ )
        Vec_IntPush( vCut, Vec_IntEntry(vMap, pNodes[i]) );    
}
int Sfm_DecComputeFlipInvGain( Sfm_Dec_t * p, Abc_Obj_t * pPivot, int * pfNeedInv )
{
    Abc_Obj_t * pFanout; 
    Mio_Gate_t * pGate, * pGateNew;
    int i, Handle, fNeedInv = 0, Gain = 0;
    Abc_ObjForEachFanout( pPivot, pFanout, i )
    {
        if ( !Abc_ObjIsNode(pFanout) )
        {
            fNeedInv = 1;
            continue;
        }
        pGate = (Mio_Gate_t*)pFanout->pData;
        if ( Abc_ObjFaninNum(pFanout) == 1 && Mio_GateIsInv(pGate) )
        {
            Gain += p->AreaInv;
            continue;
        }
        Handle = Sfm_LibFindComplInputGate( &p->vGateFuncs, Mio_GateReadValue(pGate), Abc_ObjFaninNum(pFanout), Abc_NodeFindFanin(pFanout, pPivot), NULL );
        if ( Handle == -1 )
        {
            fNeedInv = 1;
            continue;
        }
        pGateNew = (Mio_Gate_t *)Vec_PtrEntry( &p->vGateHands, Handle );
        Gain += Scl_Flt2Int(Mio_GateReadArea(pGate)) - Scl_Flt2Int(Mio_GateReadArea(pGateNew));
    }
    if ( fNeedInv )
        Gain -= p->AreaInv;
    if ( pfNeedInv )
        *pfNeedInv = fNeedInv;
    return Gain;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_DecCombineDec( Sfm_Dec_t * p, word * pTruth0, word * pTruth1, int * pSupp0, int * pSupp1, int nSupp0, int nSupp1, word * pTruth, int * pSupp, int Var )
{
    Vec_Int_t vVec0 = { 2*SFM_SUPP_MAX, nSupp0, pSupp0 };
    Vec_Int_t vVec1 = { 2*SFM_SUPP_MAX, nSupp1, pSupp1 };
    Vec_Int_t vVec  = { 2*SFM_SUPP_MAX, 0,      pSupp  };
    int nWords0 = Abc_TtWordNum(nSupp0);
    int nSupp, iSuppVar;
    // check the case of equal cofactors
    if ( nSupp0 == nSupp1 && !memcmp(pSupp0, pSupp1, sizeof(int)*nSupp0) && !memcmp(pTruth0, pTruth1, sizeof(word)*nWords0) )
    {
        memcpy( pSupp,  pSupp0,  sizeof(int)*nSupp0   );
        memcpy( pTruth, pTruth0, sizeof(word)*nWords0 );
        Abc_TtStretch6( pTruth, nSupp0, p->pPars->nVarMax );
        return nSupp0;
    }
    // merge support variables
    Vec_IntTwoMerge2Int( &vVec0, &vVec1, &vVec );
    Vec_IntPushOrder( &vVec, Var );
    nSupp = Vec_IntSize( &vVec );
    if ( nSupp > p->pPars->nVarMax )
        return -2;
    // expand truth tables
    Abc_TtStretch6( pTruth0, nSupp0, nSupp );
    Abc_TtStretch6( pTruth1, nSupp1, nSupp );
    Abc_TtExpand( pTruth0, nSupp, pSupp0, nSupp0, pSupp, nSupp );
    Abc_TtExpand( pTruth1, nSupp, pSupp1, nSupp1, pSupp, nSupp );
    // perform operation
    iSuppVar = Vec_IntFind( &vVec, Var );
    Abc_TtMux( pTruth, p->pTtElems[iSuppVar], pTruth1, pTruth0, Abc_TtWordNum(nSupp) );
    Abc_TtStretch6( pTruth, nSupp, p->pPars->nVarMax );
    return nSupp;
}
int Sfm_DecPeformDec_rec( Sfm_Dec_t * p, word * pTruth, int * pSupp, int * pAssump, int nAssump, word Masks[2][SFM_SIM_WORDS], int fCofactor, int nSuppAdd )
{
    int nBTLimit = p->pPars->nBTLimit;
//    int fVerbose = p->pPars->fVeryVerbose;
    int c, i, d, iLit, status, Var = -1;
    word * pDivWords = p->pDivWords[nAssump];
    abctime clk;
    assert( nAssump <= SFM_SUPP_MAX );
    if ( p->pPars->fVeryVerbose )
    {
        printf( "\nObject %d\n", p->iTarget );
        printf( "Divs = %d.  Nodes = %d.  Mffc = %d.  Mffc area = %.2f.    ", p->nDivs, Vec_IntSize(&p->vObjGates), p->nMffc, Scl_Int2Flt(p->AreaMffc) );
        printf( "Pat0 = %d.  Pat1 = %d.    ", p->nPats[0], p->nPats[1] );
        printf( "\n" );
        if ( nAssump )
        {
            printf( "Cofactor: " );
            for ( i = 0; i < nAssump; i++ )
                printf( " %s%d", Abc_LitIsCompl(pAssump[i])? "!":"", Abc_Lit2Var(pAssump[i]) );
            printf( "\n" );
        }
    }
    // check constant
    for ( c = 0; c < 2; c++ )
    {
        if ( !Abc_TtIsConst0(Masks[c], p->nPatWords[c]) ) // there are some patterns
            continue;
        p->nSatCalls++;
        pAssump[nAssump] = Abc_Var2Lit( p->iTarget, c );
        clk = Abc_Clock();
        status = sat_solver_solve( p->pSat, pAssump, pAssump + nAssump + 1, nBTLimit, 0, 0, 0 );
        if ( status == l_Undef )
        {
            p->nTimeOuts++;
            return -2;
        }
        if ( status == l_False )
        {
            p->nSatCallsUnsat++;
            p->timeSatUnsat += Abc_Clock() - clk;
            Abc_TtConst( pTruth, Abc_TtWordNum(p->pPars->nVarMax), c );
            if ( p->pPars->fVeryVerbose )
                printf( "Found constant %d.\n", c );
            return 0;
        }
        assert( status == l_True );
        p->nSatCallsSat++;
        p->timeSatSat += Abc_Clock() - clk;
        if ( p->nPats[c] == 64*SFM_SIM_WORDS )
        {
            p->nSatCallsOver++;
            continue;//return -2;//continue;
        }
        for ( i = 0; i < p->nDivs; i++ )
            if ( sat_solver_var_value(p->pSat, i) )
                Abc_TtSetBit( Sfm_DecDivPats(p, i, c), p->nPats[c] );
        p->nPatWords[c] = 1 + (p->nPats[c] >> 6);
        Abc_TtSetBit( Masks[c], p->nPats[c]++ );
    }

    if ( p->iUseThis != -1 )
    {
        Var = p->iUseThis;
        p->iUseThis = -1;
        goto cofactor;
    }

    // check implications
    Vec_IntClear( &p->vImpls[0] );
    Vec_IntClear( &p->vImpls[1] );
    for ( d = 0; d < p->nDivs; d++ )
    {
        int Impls[2] = {-1, -1};
        for ( c = 0; c < 2; c++ )
        {
            word * pPats = Sfm_DecDivPats( p, d, c );
            int fHas0s = Abc_TtIntersect( pPats, Masks[c], p->nPatWords[c], 1 );
            int fHas1s = Abc_TtIntersect( pPats, Masks[c], p->nPatWords[c], 0 );
            if ( fHas0s && fHas1s )
                continue;
            pAssump[nAssump]   = Abc_Var2Lit( p->iTarget, c );
            pAssump[nAssump+1] = Abc_Var2Lit( d, fHas1s ); // if there are 1s, check if 0 is SAT
            clk = Abc_Clock();
            if ( Abc_TtGetBit( pDivWords, 4*d+2*c+fHas1s ) )
            {
                p->nSatCallsUnsat--;
                status = l_False;
            }
            else
            {
                p->nSatCalls++;
                status = sat_solver_solve( p->pSat, pAssump, pAssump + nAssump + 2, nBTLimit, 0, 0, 0 );
            }
            if ( status == l_Undef )
            {
                p->nTimeOuts++;
                return -2;
            }
            if ( status == l_False )
            {
                p->nSatCallsUnsat++;
                p->timeSatUnsat += Abc_Clock() - clk;
                Impls[c] = Abc_LitNot(pAssump[nAssump+1]);
                Vec_IntPush( &p->vImpls[c], Abc_LitNot(pAssump[nAssump+1]) );
                Abc_TtSetBit( pDivWords, 4*d+2*c+fHas1s );
                continue;
            }
            assert( status == l_True );
            p->nSatCallsSat++;
            p->timeSatSat += Abc_Clock() - clk;
            if ( p->nPats[c] == 64*SFM_SIM_WORDS )
            {
                p->nSatCallsOver++;
                continue;//return -2;//continue;
            }
            // record this status
            for ( i = 0; i < p->nDivs; i++ )
                if ( sat_solver_var_value(p->pSat, i) )
                    Abc_TtSetBit( Sfm_DecDivPats(p, i, c), p->nPats[c] );
            p->nPatWords[c] = 1 + (p->nPats[c] >> 6);
            Abc_TtSetBit( Masks[c], p->nPats[c]++ );
        }
        if ( Impls[0] == -1 || Impls[1] == -1 )
            continue;
        if ( Impls[0] == Impls[1] )
        {
            Vec_IntPop( &p->vImpls[0] );
            Vec_IntPop( &p->vImpls[1] );
            continue;
        }
        assert( Abc_Lit2Var(Impls[0]) == Abc_Lit2Var(Impls[1]) );
        // found buffer/inverter
        Abc_TtUnit( pTruth, Abc_TtWordNum(p->pPars->nVarMax), Abc_LitIsCompl(Impls[0]) );
        pSupp[0] = Abc_Lit2Var(Impls[0]);
        if ( p->pPars->fVeryVerbose )
            printf( "Found variable %s%d.\n", Abc_LitIsCompl(Impls[0]) ? "!":"", pSupp[0] );
        return 1;        
    }
    if ( nSuppAdd > p->pPars->nVarMax - 2 )
    {
        if ( p->pPars->fVeryVerbose )
            printf( "The number of assumption is more than MFFC size.\n" );
        return -2;
    }
    // try using all implications at once
    if ( p->pPars->fUseAndOr )
    for ( c = 0; c < 2; c++ )
    {
        if ( Vec_IntSize(&p->vImpls[!c]) < 2 )
            continue;
        p->nSatCalls++;
        pAssump[nAssump] = Abc_Var2Lit( p->iTarget, c );
        assert( Vec_IntSize(&p->vImpls[!c]) < SFM_WIN_MAX-10 );
        Vec_IntForEachEntry( &p->vImpls[!c], iLit, i )
            pAssump[nAssump+1+i] = iLit;
        clk = Abc_Clock();
        status = sat_solver_solve( p->pSat, pAssump, pAssump + nAssump+1+i, nBTLimit, 0, 0, 0 );
        if ( status == l_Undef )
        {
            p->nTimeOuts++;
            return -2;
        }
        if ( status == l_False )
        {
            int * pFinal, nFinal = sat_solver_final( p->pSat, &pFinal );
            p->nSatCallsUnsat++;
            p->timeSatUnsat += Abc_Clock() - clk;
            if ( nFinal + nSuppAdd > 6 )
                continue;
            // collect only relevant literals
            for ( i = d = 0; i < nFinal; i++ )
                if ( Vec_IntFind(&p->vImpls[!c], Abc_LitNot(pFinal[i])) >= 0 )
                    pSupp[d++] = Abc_LitNot(pFinal[i]);
            nFinal = d;
            // create AND/OR gate
            assert( nFinal <= 6 );
            if ( c )
            {
                *pTruth = ~(word)0;
                for ( i = 0; i < nFinal; i++ )
                {
                    *pTruth &= Abc_LitIsCompl(pSupp[i]) ? ~s_Truths6[i] : s_Truths6[i];
                    pSupp[i] = Abc_Lit2Var(pSupp[i]);
                }
            }
            else
            {
                *pTruth = 0;
                for ( i = 0; i < nFinal; i++ )
                {
                    *pTruth |= Abc_LitIsCompl(pSupp[i]) ? s_Truths6[i] : ~s_Truths6[i];
                    pSupp[i] = Abc_Lit2Var(pSupp[i]);
                }
            }
            Abc_TtStretch6( pTruth, nFinal, p->pPars->nVarMax );
            p->nNodesAndOr++;
            if ( p->pPars->fVeryVerbose )
                printf( "Found %d-input AND/OR gate.\n", nFinal );
            return nFinal;
        }
        assert( status == l_True );
        p->nSatCallsSat++;
        p->timeSatSat += Abc_Clock() - clk;
        if ( p->nPats[c] == 64*SFM_SIM_WORDS )
        {
            p->nSatCallsOver++;
            continue;//return -2;//continue;
        }
        for ( i = 0; i < p->nDivs; i++ )
            if ( sat_solver_var_value(p->pSat, i) )
                Abc_TtSetBit( Sfm_DecDivPats(p, i, c), p->nPats[c] );
        p->nPatWords[c] = 1 + (p->nPats[c] >> 6);
        Abc_TtSetBit( Masks[c], p->nPats[c]++ );
    }

    // find the best cofactoring variable
//    if ( !fCofactor || Vec_IntSize(&p->vImpls[0]) + Vec_IntSize(&p->vImpls[1]) > 2 )
    Var = Sfm_DecFindBestVar( p, Masks );
//    if ( Var == -1 )
//        Var = Sfm_DecFindBestVar2( p, Masks );

/*
    {
        int Lit0 = Vec_IntSize(&p->vImpls[0]) ? Vec_IntEntry(&p->vImpls[0], 0) : -1;
        int Lit1 = Vec_IntSize(&p->vImpls[1]) ? Vec_IntEntry(&p->vImpls[1], 0) : -1;
        if ( Lit0 == -1 && Lit1 >= 0 )
            Var = Abc_Lit2Var(Lit1);
        else if ( Lit1 == -1 && Lit0 >= 0 )
            Var = Abc_Lit2Var(Lit0);
        else if ( Lit0 >= 0 && Lit1 >= 0 )
        {
            if ( Lit0 < Lit1 )
                Var = Abc_Lit2Var(Lit0);
            else
                Var = Abc_Lit2Var(Lit1);
        }
    }
*/

    if ( Var == -1 && fCofactor )
    {
        //for ( Var = p->nDivs - 1; Var >= 0; Var-- )
        Vec_IntForEachEntryReverse( &p->vObjInMffc, Var, i )
            if ( Vec_IntFind(&p->vObjDec, Var) == -1 )
                break;
//        if ( i == Vec_IntSize(&p->vObjInMffc) )
        if ( i == -1 )
            Var = -1;
        fCofactor = 0;
    }

    if ( p->pPars->fVeryVerbose )
    {
        Sfm_DecPrint( p, Masks );
        printf( "Best var %d\n", Var );
        printf( "\n" );
    }
cofactor:
    // cofactor the problem
    if ( Var >= 0 )
    {
        word uTruth[2][SFM_WORD_MAX], MasksNext[2][SFM_SIM_WORDS];
        int w, Supp[2][2*SFM_SUPP_MAX], nSupp[2] = {0};
        Vec_IntPush( &p->vObjDec, Var );
        for ( i = 0; i < 2; i++ )
        {
            for ( c = 0; c < 2; c++ )
            {
                Abc_TtAndSharp( MasksNext[c], Masks[c], Sfm_DecDivPats(p, Var, c), p->nPatWords[c], !i );
                for ( w = p->nPatWords[c]; w < SFM_SIM_WORDS; w++ )
                    MasksNext[c][w] = 0;
            }
            pAssump[nAssump] = Abc_Var2Lit( Var, !i );
            memcpy( p->pDivWords[nAssump+1], p->pDivWords[nAssump], sizeof(word) * p->nDivWords );
            nSupp[i] = Sfm_DecPeformDec_rec( p, uTruth[i], Supp[i], pAssump, nAssump+1, MasksNext, fCofactor, (i ? nSupp[0] : 0) + nSuppAdd + 1 );
            if ( nSupp[i] == -2 )
                return -2;
        }
        // combine solutions
        return Sfm_DecCombineDec( p, uTruth[0], uTruth[1], Supp[0], Supp[1], nSupp[0], nSupp[1], pTruth, pSupp, Var );
    }
    return -2;
}
int Sfm_DecPeformDec2( Sfm_Dec_t * p, Abc_Obj_t * pObj )
{
    word uTruth[SFM_DEC_MAX][SFM_WORD_MAX], Masks[2][SFM_SIM_WORDS];
    int pSupp[SFM_DEC_MAX][2*SFM_SUPP_MAX];
    int nSupp[SFM_DEC_MAX], pAssump[SFM_WIN_MAX];
    int fVeryVerbose = p->pPars->fPrintDecs || p->pPars->fVeryVerbose;
    int nDecs = Abc_MaxInt(p->pPars->nDecMax, 1);
    //int fNeedInv, AreaGainInv = Sfm_DecComputeFlipInvGain(p, pObj, &fNeedInv);
    int i, RetValue, Prev = 0, iBest = -1, AreaThis, AreaNew;//, AreaNewInv;
    int GainThis, GainBest = -1, iLibObj, iLibObjBest = -1; 
    assert( p->pPars->fArea == 1 );
//printf( "AreaGainInv = %8.2f  ", Scl_Int2Flt(AreaGainInv) );
    //Sfm_DecPrint( p, NULL );
    if ( fVeryVerbose )
        printf( "\nNode %4d : MFFC %2d\n", p->iTarget, p->nMffc );
    assert( p->pPars->nDecMax <= SFM_DEC_MAX );
    Sfm_ObjSetupSimInfo( pObj );
    Vec_IntClear( &p->vObjDec );
    for ( i = 0; i < nDecs; i++ )
    {
        // reduce the variable array
        if ( Vec_IntSize(&p->vObjDec) > Prev )
            Vec_IntShrink( &p->vObjDec, Prev );
        Prev = Vec_IntSize(&p->vObjDec) + 1;
        // perform decomposition 
        Abc_TtMask( Masks[0], SFM_SIM_WORDS, p->nPats[0] );
        Abc_TtMask( Masks[1], SFM_SIM_WORDS, p->nPats[1] );        
        nSupp[i] = Sfm_DecPeformDec_rec( p, uTruth[i], pSupp[i], pAssump, 0, Masks, 1, 0 );
        if ( nSupp[i] == -2 )
        {
            if ( fVeryVerbose )
                printf( "Dec  %d: Pat0 = %2d  Pat1 = %2d  NO DEC.\n", i, p->nPats[0], p->nPats[1] );
            continue;
        }
        if ( fVeryVerbose )
            printf( "Dec  %d: Pat0 = %2d  Pat1 = %2d  Supp = %d  ", i, p->nPats[0], p->nPats[1], nSupp[i] );
        if ( fVeryVerbose )
            Dau_DsdPrintFromTruth( uTruth[i], nSupp[i] );
        if ( nSupp[i] < 2 )
        {
            p->nSuppVars = nSupp[i];
            Abc_TtCopy( p->Copy, uTruth[i], SFM_WORD_MAX, 0 ); 
            RetValue = Sfm_LibImplementSimple( p->pLib, uTruth[i], pSupp[i], nSupp[i], &p->vObjGates, &p->vObjFanins );
            assert( nSupp[i] <= p->pPars->nVarMax );
            p->nLuckySizes[nSupp[i]]++;
            assert( RetValue <= 2 );
            p->nLuckyGates[RetValue]++;
            //printf( "\n" );
            return RetValue;
        }

        p->nSuppVars = nSupp[i];
        Abc_TtCopy( p->Copy, uTruth[i], SFM_WORD_MAX, 0 ); 
        AreaNew = Sfm_LibFindAreaMatch( p->pLib, uTruth[i], nSupp[i], &iLibObj );
/*
        uTruth[i][0] = ~uTruth[i][0];
        AreaNewInv = Sfm_LibFindAreaMatch( p->pLib, uTruth[i], nSupp[i], NULL );
        uTruth[i][0] = ~uTruth[i][0];

        if ( AreaNew > 0 && AreaNewInv > 0 && AreaNew - AreaNewInv + AreaGainInv > 0 )
            printf( "AreaNew = %8.2f   AreaNewInv = %8.2f   Gain = %8.2f   Total = %8.2f\n", 
                Scl_Int2Flt(AreaNew), Scl_Int2Flt(AreaNewInv), Scl_Int2Flt(AreaNew - AreaNewInv), Scl_Int2Flt(AreaNew - AreaNewInv + AreaGainInv) );
        else
            printf( "\n" );
*/
        if ( AreaNew == -1 )
            continue;


        // compute area savings
        Sfm_DecPrepareVec( &p->vObjMap, pSupp[i], nSupp[i], &p->vGateCut );
        AreaThis = Sfm_DecMffcAreaReal(pObj, &p->vGateCut, NULL);
        assert( p->AreaMffc <= AreaThis );
        if ( p->pPars->fZeroCost ? (AreaNew > AreaThis) : (AreaNew >= AreaThis) )
            continue;
        // find the best gain
        GainThis = AreaThis - AreaNew;
        assert( GainThis >= 0 );
        if ( GainBest < GainThis )
        {
            GainBest    = GainThis;
            iLibObjBest = iLibObj;
            iBest       = i;
        }
    }
    Sfm_ObjSetdownSimInfo( pObj );
    if ( iBest == -1 )
    {
        if ( fVeryVerbose )
            printf( "Best  : NO DEC.\n" );
        p->nNoDecs++;
        //printf( "\n" );
        return -2;
    }
    if ( fVeryVerbose )
        printf( "Best %d: %d  ", iBest, nSupp[iBest] );
    if ( fVeryVerbose )
      Dau_DsdPrintFromTruth( uTruth[iBest], nSupp[iBest] );
    // implement
    assert( iLibObjBest >= 0 );
    RetValue = Sfm_LibImplementGatesArea( p->pLib, pSupp[iBest], nSupp[iBest], iLibObjBest, &p->vObjGates, &p->vObjFanins );
    assert( nSupp[iBest] <= p->pPars->nVarMax );
    p->nLuckySizes[nSupp[iBest]]++;
    assert( RetValue <= 2 );
    p->nLuckyGates[RetValue]++;
    return 1;
}
int Sfm_DecPeformDec3( Sfm_Dec_t * p, Abc_Obj_t * pObj )
{
    word uTruth[SFM_DEC_MAX][SFM_WORD_MAX], Masks[2][SFM_SIM_WORDS];
    int pSupp[SFM_DEC_MAX][2*SFM_SUPP_MAX];
    int nSupp[SFM_DEC_MAX], pAssump[SFM_WIN_MAX];
    int fVeryVerbose = p->pPars->fPrintDecs || p->pPars->fVeryVerbose;
    int nDecs = Abc_MaxInt(p->pPars->nDecMax, 1);
    int i, k, DelayOrig = 0, DelayMin, GainMax, AreaMffc, nMatches, iBest = -1, RetValue, Prev = 0; 
    Mio_Gate_t * pGate1Best = NULL, * pGate2Best = NULL; 
    char * pFans1Best = NULL, * pFans2Best = NULL;
    assert( p->pPars->fArea == 0 );
    p->DelayMin = 0;
    //Sfm_DecPrint( p, NULL );
    if ( fVeryVerbose )
        printf( "\nNode %4d : MFFC %2d\n", p->iTarget, p->nMffc );
    // set limit on search for decompositions in delay-model
    assert( p->pPars->nDecMax <= SFM_DEC_MAX );
    Sfm_ObjSetupSimInfo( pObj );
    Vec_IntClear( &p->vObjDec );
    for ( i = 0; i < nDecs; i++ )
    {
        GainMax = 0;
        DelayMin = DelayOrig = Sfm_ManReadObjDelay( p, Abc_ObjId(pObj) );
        // reduce the variable array
        if ( Vec_IntSize(&p->vObjDec) > Prev )
            Vec_IntShrink( &p->vObjDec, Prev );
        Prev = Vec_IntSize(&p->vObjDec) + 1;
        // perform decomposition 
        Abc_TtMask( Masks[0], SFM_SIM_WORDS, p->nPats[0] );
        Abc_TtMask( Masks[1], SFM_SIM_WORDS, p->nPats[1] );        
        nSupp[i] = Sfm_DecPeformDec_rec( p, uTruth[i], pSupp[i], pAssump, 0, Masks, 1, 0 );
        if ( nSupp[i] == -2 )
        {
            if ( fVeryVerbose )
                printf( "Dec  %d: Pat0 = %2d  Pat1 = %2d  NO DEC.\n", i, p->nPats[0], p->nPats[1] );
            continue;
        }
        if ( fVeryVerbose )
            printf( "Dec  %d: Pat0 = %2d  Pat1 = %2d  Supp = %d  ", i, p->nPats[0], p->nPats[1], nSupp[i] );
        if ( fVeryVerbose )
            Dau_DsdPrintFromTruth( uTruth[i], nSupp[i] );
        if ( p->pTim && nSupp[i] == 1 && uTruth[i][0] == ABC_CONST(0x5555555555555555) && DelayMin <= p->DelayInv + Sfm_ManReadObjDelay(p, Vec_IntEntry(&p->vObjMap, pSupp[i][0])) )
        {
            if ( fVeryVerbose )
                printf( "Dec  %d: Pat0 = %2d  Pat1 = %2d  NO DEC.\n", i, p->nPats[0], p->nPats[1] );
            continue;
        }
        if ( p->pMit && nSupp[i] == 1 && uTruth[i][0] == ABC_CONST(0x5555555555555555) )
        {
            if ( fVeryVerbose )
                printf( "Dec  %d: Pat0 = %2d  Pat1 = %2d  NO DEC.\n", i, p->nPats[0], p->nPats[1] );
            continue;
        }
        if ( nSupp[i] < 2 )
        {
            p->nSuppVars = nSupp[i];
            Abc_TtCopy( p->Copy, uTruth[i], SFM_WORD_MAX, 0 ); 
            RetValue = Sfm_LibImplementSimple( p->pLib, uTruth[i], pSupp[i], nSupp[i], &p->vObjGates, &p->vObjFanins );
            assert( nSupp[i] <= p->pPars->nVarMax );
            p->nLuckySizes[nSupp[i]]++;
            assert( RetValue <= 2 );
            p->nLuckyGates[RetValue]++;
            return RetValue;
        }

        // get MFFC
        Sfm_DecPrepareVec( &p->vObjMap, pSupp[i], nSupp[i], &p->vGateCut ); // returns cut in p->vGateCut
        AreaMffc = Sfm_DecMffcAreaReal(pObj, &p->vGateCut, &p->vGateMffc ); // returns MFFC in p->vGateMffc

        // try the delay
        p->nSuppVars = nSupp[i];
        Abc_TtCopy( p->Copy, uTruth[i], SFM_WORD_MAX, 0 ); 
        nMatches = Sfm_LibFindDelayMatches( p->pLib, uTruth[i], pSupp[i], nSupp[i], &p->vMatchGates, &p->vMatchFans );
        for ( k = 0; k < nMatches; k++ )
        {
            abctime clk = Abc_Clock();
            Mio_Gate_t * pGate1 = (Mio_Gate_t *)Vec_PtrEntry( &p->vMatchGates, 2*k+0 );
            Mio_Gate_t * pGate2 = (Mio_Gate_t *)Vec_PtrEntry( &p->vMatchGates, 2*k+1 );
            int AreaNew = Scl_Flt2Int( Mio_GateReadArea(pGate1) + (pGate2 ? Mio_GateReadArea(pGate2) : 0.0) );
            char * pFans1 = (char *)Vec_PtrEntry( &p->vMatchFans, 2*k+0 );
            char * pFans2 = (char *)Vec_PtrEntry( &p->vMatchFans, 2*k+1 );
            Vec_Int_t vFanins = { nSupp[i], nSupp[i], pSupp[i] };
            // skip identical gate
            //if ( pGate2 == NULL && pGate1 == (Mio_Gate_t *)pObj->pData )
            //    continue;
            if ( p->pMit )
            {
                int Gain = Sfm_MitEvalRemapping( p->pMit, &p->vGateMffc, pObj, &vFanins, &p->vObjMap, pGate1, pFans1, pGate2, pFans2 );
                if ( p->pPars->DelAreaRatio && AreaNew > AreaMffc && (Gain / (AreaNew - AreaMffc)) < p->pPars->DelAreaRatio )
                    continue;
                if ( GainMax < Gain )
                {
                    GainMax    = Gain;
                    pGate1Best = pGate1;
                    pGate2Best = pGate2;
                    pFans1Best = pFans1;
                    pFans2Best = pFans2;
                    iBest      = i;
                }
            }
            else
            {
                int Delay = Sfm_TimEvalRemapping( p->pTim, &vFanins, &p->vObjMap, pGate1, pFans1, pGate2, pFans2 );
                if ( p->pPars->DelAreaRatio && AreaNew > AreaMffc && (Delay / (AreaNew - AreaMffc)) < p->pPars->DelAreaRatio )
                    continue;
                if ( DelayMin > Delay )
                {
                    DelayMin   = Delay;
                    pGate1Best = pGate1;
                    pGate2Best = pGate2;
                    pFans1Best = pFans1;
                    pFans2Best = pFans2;
                    iBest      = i;
                }
            }
            p->timeEval += Abc_Clock() - clk;
        }
    }
//printf( "Gain max = %d.\n", GainMax );
    Sfm_ObjSetdownSimInfo( pObj );
    if ( iBest == -1 )
    {
        if ( fVeryVerbose )
            printf( "Best  : NO DEC.\n" );
        p->nNoDecs++;
        return -2;
    }
    if ( fVeryVerbose )
        printf( "Best %d: %d  ", iBest, nSupp[iBest] );
//    if ( fVeryVerbose )
//        Dau_DsdPrintFromTruth( uTruth[iBest], nSupp[iBest] );
    RetValue = Sfm_LibImplementGatesDelay( p->pLib, pSupp[iBest], pGate1Best, pGate2Best, pFans1Best, pFans2Best, &p->vObjGates, &p->vObjFanins );
    assert( nSupp[iBest] <= p->pPars->nVarMax );
    p->nLuckySizes[nSupp[iBest]]++;
    assert( RetValue <= 2 );
    p->nLuckyGates[RetValue]++;
    p->DelayMin = DelayMin;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Incremental level update.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkUpdateIncLevel_rec( Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanout;
    int i, LevelNew = Abc_ObjLevelNew(pObj);
    if ( LevelNew == Abc_ObjLevel(pObj) && Abc_ObjIsNode(pObj) && Abc_ObjFaninNum(pObj) > 0 )
        return;
    pObj->Level = LevelNew;
    if ( !Abc_ObjIsCo(pObj) )
        Abc_ObjForEachFanout( pObj, pFanout, i )
            Abc_NtkUpdateIncLevel_rec( pFanout );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDfsCheck_rec( Abc_Obj_t * pObj, Abc_Obj_t * pPivot )
{
    Abc_Obj_t * pFanin; int i;
    if ( pObj == pPivot )
        return 0;
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return 1;
    Abc_NodeSetTravIdCurrent( pObj );
    if ( Abc_ObjIsCi(pObj) )
        return 1;
    assert( Abc_ObjIsNode(pObj) );
    Abc_ObjForEachFanin( pObj, pFanin, i )
        if ( !Abc_NtkDfsCheck_rec(pFanin, pPivot) )
            return 0;
    return 1;
}
void Abc_NtkDfsReverseOne_rec( Abc_Obj_t * pObj, Vec_Int_t * vTfo, int nLevelMax, int nFanoutMax )
{
    Abc_Obj_t * pFanout; int i;
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    if ( Abc_ObjIsCo(pObj) || Abc_ObjLevel(pObj) > nLevelMax )
        return;
    assert( Abc_ObjIsNode( pObj ) );
    if ( Abc_ObjFanoutNum(pObj) <= nFanoutMax )
    {
        Abc_ObjForEachFanout( pObj, pFanout, i )
            if ( Abc_ObjIsCo(pFanout) || Abc_ObjLevel(pFanout) > nLevelMax )
                break;
        if ( i == Abc_ObjFanoutNum(pObj) )
            Abc_ObjForEachFanout( pObj, pFanout, i )
                Abc_NtkDfsReverseOne_rec( pFanout, vTfo, nLevelMax, nFanoutMax );
    }
    Vec_IntPush( vTfo, Abc_ObjId(pObj) );
    pObj->iTemp = 0;
}
int Abc_NtkDfsOne_rec( Abc_Obj_t * pObj, Vec_Int_t * vTfi, int nLevelMin, int CiLabel )
{
    Abc_Obj_t * pFanin; int i;
    if ( Abc_NodeIsTravIdCurrent( pObj ) )
        return pObj->iTemp;
    Abc_NodeSetTravIdCurrent( pObj );
    if ( Abc_ObjIsCi(pObj) || (Abc_ObjLevel(pObj) < nLevelMin && Abc_ObjFaninNum(pObj) > 0) )
    {
        Vec_IntPush( vTfi, Abc_ObjId(pObj) );
        return (pObj->iTemp = CiLabel);
    }
    assert( Abc_ObjIsNode(pObj) );
    pObj->iTemp = Abc_ObjFaninNum(pObj) ? 0 : CiLabel;
    Abc_ObjForEachFanin( pObj, pFanin, i )
        pObj->iTemp |= Abc_NtkDfsOne_rec( pFanin, vTfi, nLevelMin, CiLabel );
    Vec_IntPush( vTfi, Abc_ObjId(pObj) );
    Sfm_ObjSimulateNode( pObj );
    return pObj->iTemp;
}
void Sfm_DecAddNode( Abc_Obj_t * pObj, Vec_Int_t * vMap, Vec_Int_t * vGates, int fSkip, int fVeryVerbose )
{
    if ( fVeryVerbose )
        printf( "%d:%d(%d) ", Vec_IntSize(vMap), Abc_ObjId(pObj), pObj->iTemp );
    if ( fVeryVerbose )
        Abc_ObjPrint( stdout, pObj );
    Vec_IntPush( vMap, Abc_ObjId(pObj) );
    Vec_IntPush( vGates, fSkip ? -1 : Mio_GateReadValue((Mio_Gate_t *)pObj->pData) );
}
static inline int Sfm_DecNodeIsMffc( Abc_Obj_t * p, int nLevelMin )
{
    return Abc_ObjIsNode(p) && Abc_ObjFanoutNum(p) == 1 && Abc_NodeIsTravIdCurrent(p) && (Abc_ObjLevel(p) >= nLevelMin || Abc_ObjFaninNum(p) == 0);
}
static inline int Sfm_DecNodeIsMffcInput( Abc_Obj_t * p, int nLevelMin, Sfm_Tim_t * pTim, Abc_Obj_t * pPivot )
{
    return Abc_NodeIsTravIdCurrent(p) && Sfm_TimNodeIsNonCritical(pTim, pPivot, p);
}
static inline int Sfm_DecNodeIsMffcInput2( Abc_Obj_t * p, int nLevelMin, Sfm_Mit_t * pMit, Abc_Obj_t * pPivot )
{
    return Abc_NodeIsTravIdCurrent(p) && Sfm_MitNodeIsNonCritical(pMit, pPivot, p);
}
void Sfm_DecMarkMffc( Abc_Obj_t * pPivot, int nLevelMin, int nMffcMax, int fVeryVerbose, Vec_Int_t * vMffc, Vec_Int_t * vInMffc, Sfm_Tim_t * pTim, Sfm_Mit_t * pMit )
{
    Abc_Obj_t * pFanin, * pFanin2, * pFanin3, * pObj; int i, k, n;
    assert( nMffcMax > 0 );
    Vec_IntFill( vMffc, 1, Abc_ObjId(pPivot) );
    if ( pMit != NULL )
    {
        pPivot->iTemp |= SFM_MASK_MFFC;
        pPivot->iTemp |= SFM_MASK_PIVOT;
        // collect MFFC inputs (these are low-delay nodes close to the pivot)
        Vec_IntClear(vInMffc);
        Abc_ObjForEachFanin( pPivot, pFanin, i )
            if ( Sfm_DecNodeIsMffcInput2(pFanin, nLevelMin, pMit, pPivot) )
                Vec_IntPushUnique( vInMffc, Abc_ObjId(pFanin) );
        Abc_ObjForEachFanin( pPivot, pFanin, i )
            Abc_ObjForEachFanin( pFanin, pFanin2, k )
                if ( Sfm_DecNodeIsMffcInput2(pFanin2, nLevelMin, pMit, pPivot) )
                    Vec_IntPushUnique( vInMffc, Abc_ObjId(pFanin2) );
        Abc_ObjForEachFanin( pPivot, pFanin, i )
            Abc_ObjForEachFanin( pFanin, pFanin2, k )
                Abc_ObjForEachFanin( pFanin2, pFanin3, n )
                    if ( Sfm_DecNodeIsMffcInput2(pFanin3, nLevelMin, pMit, pPivot) )
                        Vec_IntPushUnique( vInMffc, Abc_ObjId(pFanin3) );
    }
    else if ( pTim != NULL )
    {
        pPivot->iTemp |= SFM_MASK_MFFC;
        pPivot->iTemp |= SFM_MASK_PIVOT;
        // collect MFFC inputs (these are low-delay nodes close to the pivot)
        Vec_IntClear(vInMffc);
        Abc_ObjForEachFanin( pPivot, pFanin, i )
            if ( Sfm_DecNodeIsMffcInput(pFanin, nLevelMin, pTim, pPivot) )
                Vec_IntPushUnique( vInMffc, Abc_ObjId(pFanin) );
        Abc_ObjForEachFanin( pPivot, pFanin, i )
            Abc_ObjForEachFanin( pFanin, pFanin2, k )
                if ( Sfm_DecNodeIsMffcInput(pFanin2, nLevelMin, pTim, pPivot) )
                    Vec_IntPushUnique( vInMffc, Abc_ObjId(pFanin2) );
        Abc_ObjForEachFanin( pPivot, pFanin, i )
            Abc_ObjForEachFanin( pFanin, pFanin2, k )
                Abc_ObjForEachFanin( pFanin2, pFanin3, n )
                    if ( Sfm_DecNodeIsMffcInput(pFanin3, nLevelMin, pTim, pPivot) )
                        Vec_IntPushUnique( vInMffc, Abc_ObjId(pFanin3) );

/*
        printf( "Node %d: (%.2f)  ", pPivot->Id, Scl_Int2Flt(Sfm_ManReadObjDelay(p, Abc_ObjId(pPivot)))  );
        Abc_ObjForEachFanin( pPivot, pFanin, i )
            printf( "%d: %.2f  ", Abc_ObjLevel(pFanin), Scl_Int2Flt(Sfm_ManReadObjDelay(p, Abc_ObjId(pFanin))) );
        printf( "\n" );

        printf( "Node %d: ", pPivot->Id );
        Abc_NtkForEachObjVec( vInMffc, pPivot->pNtk, pObj, i )
            printf( "%d: %.2f  ", Abc_ObjLevel(pObj), Scl_Int2Flt(Sfm_ManReadObjDelay(p, Abc_ObjId(pObj))) );
        printf( "\n" );
*/
    }
    else
    {

        // collect MFFC
        Abc_ObjForEachFanin( pPivot, pFanin, i )
            if ( Sfm_DecNodeIsMffc(pFanin, nLevelMin) && Vec_IntSize(vMffc) < nMffcMax )
                Vec_IntPushUnique( vMffc, Abc_ObjId(pFanin) );
        Abc_ObjForEachFanin( pPivot, pFanin, i )
            if ( Sfm_DecNodeIsMffc(pFanin, nLevelMin) && Vec_IntSize(vMffc) < nMffcMax )
                Abc_ObjForEachFanin( pFanin, pFanin2, k )
                    if ( Sfm_DecNodeIsMffc(pFanin2, nLevelMin) && Vec_IntSize(vMffc) < nMffcMax )
                        Vec_IntPushUnique( vMffc, Abc_ObjId(pFanin2) );
        Abc_ObjForEachFanin( pPivot, pFanin, i )
            if ( Sfm_DecNodeIsMffc(pFanin, nLevelMin) && Vec_IntSize(vMffc) < nMffcMax )
                Abc_ObjForEachFanin( pFanin, pFanin2, k )
                    if ( Sfm_DecNodeIsMffc(pFanin2, nLevelMin) && Vec_IntSize(vMffc) < nMffcMax )
                        Abc_ObjForEachFanin( pFanin2, pFanin3, n )
                            if ( Sfm_DecNodeIsMffc(pFanin3, nLevelMin) && Vec_IntSize(vMffc) < nMffcMax )
                                Vec_IntPushUnique( vMffc, Abc_ObjId(pFanin3) );
        // mark MFFC
        assert( Vec_IntSize(vMffc) <= nMffcMax );
        Abc_NtkForEachObjVec( vMffc, pPivot->pNtk, pObj, i )
            pObj->iTemp |= SFM_MASK_MFFC;
        pPivot->iTemp |= SFM_MASK_PIVOT;
        // collect MFFC inputs
        Vec_IntClear(vInMffc);
        Abc_NtkForEachObjVec( vMffc, pPivot->pNtk, pObj, i )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                if ( Abc_NodeIsTravIdCurrent(pFanin) && pFanin->iTemp == SFM_MASK_PI )
                    Vec_IntPushUnique( vInMffc, Abc_ObjId(pFanin) );

//        printf( "Node %d: ", pPivot->Id );
//        Abc_ObjForEachFanin( pPivot, pFanin, i )
//            printf( "%d ", Abc_ObjFanoutNum(pFanin) );
//        printf( "\n" );

//        Abc_NtkForEachObjVec( vInMffc, pPivot->pNtk, pObj, i )
//            printf( "%d ", Abc_ObjFanoutNum(pObj) );
//        printf( "\n" );

    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_DecExtract( Abc_Ntk_t * pNtk, Sfm_Par_t * pPars, Abc_Obj_t * pPivot, Vec_Int_t * vRoots, Vec_Int_t * vGates, Vec_Wec_t * vFanins, Vec_Int_t * vMap, Vec_Int_t * vTfi, Vec_Int_t * vTfo, Vec_Int_t * vMffc, Vec_Int_t * vInMffc, Sfm_Tim_t * pTim, Sfm_Mit_t * pMit )
{
    int fVeryVerbose = 0;//pPars->fVeryVerbose;
    Vec_Int_t * vLevel;
    Abc_Obj_t * pObj, * pFanin;
    int nLevelMax = pPivot->Level + pPars->nTfoLevMax;
    int nLevelMin = pPivot->Level - pPars->nTfiLevMax;
    int i, k, nTfiSize, nDivs = -1;
    assert( Abc_ObjIsNode(pPivot) );
if ( fVeryVerbose )
printf( "\n\nTarget %d\n", Abc_ObjId(pPivot) );
    // collect TFO nodes
    Vec_IntClear( vTfo );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkDfsReverseOne_rec( pPivot, vTfo, nLevelMax, pPars->nFanoutMax );
    // count internal fanouts
    Abc_NtkForEachObjVec( vTfo, pNtk, pObj, i )
        Abc_ObjForEachFanin( pObj, pFanin, k )
            pFanin->iTemp++;
    // compute roots
    Vec_IntClear( vRoots );
    Abc_NtkForEachObjVec( vTfo, pNtk, pObj, i )
        if ( pObj->iTemp != Abc_ObjFanoutNum(pObj) )
            Vec_IntPush( vRoots, Abc_ObjId(pObj) );
    assert( Vec_IntSize(vRoots) > 0 );
    // collect TFI and mark nodes
    Vec_IntClear( vTfi );
    Abc_NtkIncrementTravId( pNtk );
    Abc_NtkDfsOne_rec( pPivot, vTfi, nLevelMin, SFM_MASK_PI );
    nTfiSize = Vec_IntSize(vTfi);
    Sfm_ObjFlipNode( pPivot );
    // additinally mark MFFC
    Sfm_DecMarkMffc( pPivot, nLevelMin, pPars->nMffcMax, fVeryVerbose, vMffc, vInMffc, pTim, pMit );
    assert( Vec_IntSize(vMffc) <= pPars->nMffcMax );
if ( fVeryVerbose )
printf( "Mffc size = %d. Mffc area = %.2f. InMffc size = %d.\n", Vec_IntSize(vMffc), Scl_Int2Flt(Sfm_DecMffcArea(pNtk, vMffc)), Vec_IntSize(vInMffc) );
    // collect TFI(TFO)
    Abc_NtkForEachObjVec( vRoots, pNtk, pObj, i )
        Abc_NtkDfsOne_rec( pObj, vTfi, nLevelMin, SFM_MASK_INPUT );
    // mark input-only nodes pointed to by mixed nodes
    Abc_NtkForEachObjVecStart( vTfi, pNtk, pObj, i, nTfiSize )
        if ( pObj->iTemp != SFM_MASK_INPUT )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                if ( pFanin->iTemp == SFM_MASK_INPUT )
                    pFanin->iTemp = SFM_MASK_FANIN;
    // collect nodes supported only on TFI fanins and not MFFC
if ( fVeryVerbose )
printf( "\nDivs:\n" );
    Vec_IntClear( vMap );
    Vec_IntClear( vGates );
    Abc_NtkForEachObjVec( vTfi, pNtk, pObj, i )
        if ( pObj->iTemp == SFM_MASK_PI )
            Sfm_DecAddNode( pObj, vMap, vGates, Abc_ObjIsCi(pObj) || (Abc_ObjLevel(pObj) < nLevelMin && Abc_ObjFaninNum(pObj) > 0), fVeryVerbose );
    nDivs = Vec_IntSize(vMap);
    // add other nodes that are not in TFO and not in MFFC
if ( fVeryVerbose )
printf( "\nSides:\n" );
    Abc_NtkForEachObjVec( vTfi, pNtk, pObj, i )
        if ( pObj->iTemp == (SFM_MASK_PI | SFM_MASK_INPUT) || pObj->iTemp == SFM_MASK_FANIN )
            Sfm_DecAddNode( pObj, vMap, vGates, pObj->iTemp == SFM_MASK_FANIN, fVeryVerbose );
    // reorder nodes acording to delay
    if ( pMit )
    {
        int nDivsNew, nOldSize = Vec_IntSize(vMap);
        Vec_IntClear( vTfo );
        Vec_IntAppend( vTfo, vMap );
        nDivsNew = Sfm_MitSortArrayByArrival( pMit, vTfo, Abc_ObjId(pPivot) );
        // collect again
        Vec_IntClear( vMap );
        Vec_IntClear( vGates );
        Abc_NtkForEachObjVec( vTfo, pNtk, pObj, i )
            Sfm_DecAddNode( pObj, vMap, vGates, Abc_ObjIsCi(pObj) || (Abc_ObjLevel(pObj) < nLevelMin && Abc_ObjFaninNum(pObj) > 0) || pObj->iTemp == SFM_MASK_FANIN, 0 );
        assert( nOldSize == Vec_IntSize(vMap) );
        // update divisor count
        nDivs = nDivsNew;
    }
    else if ( pTim )
    {
        int nDivsNew, nOldSize = Vec_IntSize(vMap);
        Vec_IntClear( vTfo );
        Vec_IntAppend( vTfo, vMap );
        nDivsNew = Sfm_TimSortArrayByArrival( pTim, vTfo, Abc_ObjId(pPivot) );
        // collect again
        Vec_IntClear( vMap );
        Vec_IntClear( vGates );
        Abc_NtkForEachObjVec( vTfo, pNtk, pObj, i )
            Sfm_DecAddNode( pObj, vMap, vGates, Abc_ObjIsCi(pObj) || (Abc_ObjLevel(pObj) < nLevelMin && Abc_ObjFaninNum(pObj) > 0) || pObj->iTemp == SFM_MASK_FANIN, 0 );
        assert( nOldSize == Vec_IntSize(vMap) );
        // update divisor count
        nDivs = nDivsNew;
    }
    // add the TFO nodes
if ( fVeryVerbose )
printf( "\nTFO:\n" );
    Abc_NtkForEachObjVec( vTfi, pNtk, pObj, i )
        if ( pObj->iTemp >= SFM_MASK_MFFC )
            Sfm_DecAddNode( pObj, vMap, vGates, 0, fVeryVerbose );
    assert( Vec_IntSize(vMap) == Vec_IntSize(vGates) );
if ( fVeryVerbose )
printf( "\n" );
    // create node IDs
    Vec_WecClear( vFanins );
    Abc_NtkForEachObjVec( vMap, pNtk, pObj, i )
    {
        pObj->iTemp = i;
        vLevel = Vec_WecPushLevel( vFanins );
        if ( Vec_IntEntry(vGates, i) >= 0 )
            Abc_ObjForEachFanin( pObj, pFanin, k )
                Vec_IntPush( vLevel, pFanin->iTemp );
    }
    // compute care set
    Sfm_DecMan(pPivot)->uCareSet = Sfm_ObjFindCareSet(pPivot->pNtk, vRoots);

    //printf( "care = %5d : ", Abc_ObjId(pPivot) );
    //Extra_PrintBinary( stdout, (unsigned *)&Sfm_DecMan(pPivot)->uCareSet, 64 );
    //printf( "\n" );

    // remap roots
    Abc_NtkForEachObjVec( vRoots, pNtk, pObj, i )
        Vec_IntWriteEntry( vRoots, i, pObj->iTemp );
    // remap inputs to MFFC
    Abc_NtkForEachObjVec( vInMffc, pNtk, pObj, i )
        Vec_IntWriteEntry( vInMffc, i, pObj->iTemp );

/*
    // check
    Abc_NtkForEachObjVec( vMap, pNtk, pObj, i )
    {
        if ( i == nDivs )
            break;
        Abc_NtkIncrementTravId( pNtk );
        assert( Abc_NtkDfsCheck_rec(pObj, pPivot) );
    }
*/  
    return nDivs;
}
Abc_Obj_t * Sfm_DecInsert( Abc_Ntk_t * pNtk, Abc_Obj_t * pPivot, int Limit, Vec_Int_t * vGates, Vec_Wec_t * vFanins, Vec_Int_t * vMap, Vec_Ptr_t * vGateHandles, int GateBuf, int GateInv, Vec_Wrd_t * vFuncs, Vec_Int_t * vNewNodes, Sfm_Mit_t * pMit )
{
    Abc_Obj_t * pObjNew = NULL; 
    Vec_Int_t * vLevel;
    int i, k, iObj, Gate;
    if ( vNewNodes )
        Vec_IntClear( vNewNodes );
    // assuming that new gates are appended at the end
    assert( Limit < Vec_IntSize(vGates) );
    assert( Limit == Vec_IntSize(vMap) );
    if ( Limit + 1 == Vec_IntSize(vGates) )
    {
        Gate = Vec_IntEntryLast(vGates);
        if ( Gate == GateBuf )
        {
            iObj = Vec_WecEntryEntry( vFanins, Limit, 0 );
            pObjNew = Abc_NtkObj( pNtk, Vec_IntEntry(vMap, iObj) );
            // transfer load
            if ( pMit )
                Sfm_MitTransferLoad( pMit, pObjNew, pPivot );
            // replace logic cone
            Abc_ObjReplace( pPivot, pObjNew );
            // update level
            pObjNew->Level = 0;
            Abc_NtkUpdateIncLevel_rec( pObjNew );
            if ( vNewNodes )
                Vec_IntPush( vNewNodes, Abc_ObjId(pObjNew) );
            return pObjNew;
        }
        else if ( vNewNodes == NULL && Gate == GateInv )
        {
            // check if fanouts can be updated
            Abc_Obj_t * pFanout;
            Abc_ObjForEachFanout( pPivot, pFanout, i )
                if ( !Abc_ObjIsNode(pFanout) || Sfm_LibFindComplInputGate(vFuncs, Mio_GateReadValue((Mio_Gate_t*)pFanout->pData), Abc_ObjFaninNum(pFanout), Abc_NodeFindFanin(pFanout, pPivot), NULL) == -1 )
                    break;
            // update fanouts
            if ( i == Abc_ObjFanoutNum(pPivot) )
            {
                Abc_ObjForEachFanout( pPivot, pFanout, i )
                {
                    int iFanin = Abc_NodeFindFanin(pFanout, pPivot), iFaninNew = -1;
                    int iGate = Mio_GateReadValue((Mio_Gate_t*)pFanout->pData);
                    int iGateNew = Sfm_LibFindComplInputGate( vFuncs, iGate, Abc_ObjFaninNum(pFanout), iFanin, &iFaninNew );
                    assert( iGateNew >= 0 && iGateNew != iGate && iFaninNew >= 0 );
                    pFanout->pData = Vec_PtrEntry( vGateHandles, iGateNew );
                    //assert( iFanin == iFaninNew );
                    // swap fanins
                    if ( iFanin != iFaninNew )
                    {
                        int * pArray = Vec_IntArray( &pFanout->vFanins );
                        ABC_SWAP( int, pArray[iFanin], pArray[iFaninNew] );
                    }
                }
                iObj = Vec_WecEntryEntry( vFanins, Limit, 0 );
                pObjNew = Abc_NtkObj( pNtk, Vec_IntEntry(vMap, iObj) );
                Abc_ObjReplace( pPivot, pObjNew );
                // update level
                pObjNew->Level = 0;
                Abc_NtkUpdateIncLevel_rec( pObjNew );
                return pObjNew;
            }
        }
    }
    // introduce new gates
    Vec_IntForEachEntryStart( vGates, Gate, i, Limit )
    {
        vLevel = Vec_WecEntry( vFanins, i );
        pObjNew = Abc_NtkCreateNode( pNtk );
        Vec_IntForEachEntry( vLevel, iObj, k )
            Abc_ObjAddFanin( pObjNew, Abc_NtkObj(pNtk, Vec_IntEntry(vMap, iObj)) );
        pObjNew->pData = Vec_PtrEntry( vGateHandles, Gate );
        assert( Abc_ObjFaninNum(pObjNew) == Mio_GateReadPinNum((Mio_Gate_t *)pObjNew->pData) );
        Vec_IntPush( vMap, Abc_ObjId(pObjNew) );
        if ( vNewNodes )
            Vec_IntPush( vNewNodes, Abc_ObjId(pObjNew) );
    }
    // transfer load
    if ( pMit )
    {
        Sfm_MitTimingGrow( pMit );
        Sfm_MitTransferLoad( pMit, pObjNew, pPivot );
    }
    // replace logic cone
    Abc_ObjReplace( pPivot, pObjNew );
    // update level
    Abc_NtkForEachObjVecStart( vMap, pNtk, pObjNew, i, Limit )
        Abc_NtkUpdateIncLevel_rec( pObjNew );
    return pObjNew;
}
void Sfm_DecPrintStats( Sfm_Dec_t * p )
{
    int i;
    printf( "Node = %d. Try = %d. Change = %d.   Const0 = %d. Const1 = %d. Buf = %d. Inv = %d. Gate = %d. AndOr = %d. Effort = %d.  NoDec = %d.\n",
        p->nTotalNodesBeg, p->nNodesTried, p->nNodesChanged, p->nNodesConst0, p->nNodesConst1, p->nNodesBuf, p->nNodesInv, p->nNodesResyn, p->nNodesAndOr, p->nEfforts, p->nNoDecs );
    printf( "MaxDiv = %d. MaxWin = %d.   AveDiv = %d. AveWin = %d.   Calls = %d. (Sat = %d. Unsat = %d.)  Over = %d.  T/O = %d.\n",
        p->nMaxDivs, p->nMaxWin, (int)(p->nAllDivs/Abc_MaxInt(1, p->nNodesTried)), (int)(p->nAllWin/Abc_MaxInt(1, p->nNodesTried)), p->nSatCalls, p->nSatCallsSat, p->nSatCallsUnsat, p->nSatCallsOver, p->nTimeOuts );

    p->timeTotal = Abc_Clock() - p->timeStart;
    p->timeOther = p->timeTotal - p->timeLib - p->timeWin - p->timeCnf - p->timeSat - p->timeTime;

    ABC_PRTP( "Lib   ", p->timeLib  ,           p->timeTotal );
    ABC_PRTP( "Win   ", p->timeWin  ,           p->timeTotal );
    ABC_PRTP( "Cnf   ", p->timeCnf  ,           p->timeTotal );
    ABC_PRTP( "Sat   ", p->timeSat-p->timeEval, p->timeTotal );
    ABC_PRTP( " Sat  ", p->timeSatSat,          p->timeTotal );
    ABC_PRTP( " Unsat", p->timeSatUnsat,        p->timeTotal );
    ABC_PRTP( "Eval  ", p->timeEval ,           p->timeTotal );
    ABC_PRTP( "Timing", p->timeTime ,           p->timeTotal );
    ABC_PRTP( "Other ", p->timeOther,           p->timeTotal );
    ABC_PRTP( "ALL   ", p->timeTotal,           p->timeTotal );

    printf( "Cone sizes:  " );
    for ( i = 0; i <= SFM_SUPP_MAX; i++ )
        if ( p->nLuckySizes[i] )
            printf( "%d=%d  ", i, p->nLuckySizes[i] );
    printf( "  " );

    printf( "Gate sizes:  " );
    for ( i = 0; i <= SFM_SUPP_MAX; i++ )
        if ( p->nLuckyGates[i] )
            printf( "%d=%d  ", i, p->nLuckyGates[i] );
    printf( "\n" );

    printf( "Reduction:   " );
    printf( "Nodes  %6d out of %6d (%6.2f %%)   ", p->nTotalNodesBeg-p->nTotalNodesEnd, p->nTotalNodesBeg, 100.0*(p->nTotalNodesBeg-p->nTotalNodesEnd)/Abc_MaxInt(1, p->nTotalNodesBeg) );
    printf( "Edges  %6d out of %6d (%6.2f %%)   ", p->nTotalEdgesBeg-p->nTotalEdgesEnd, p->nTotalEdgesBeg, 100.0*(p->nTotalEdgesBeg-p->nTotalEdgesEnd)/Abc_MaxInt(1, p->nTotalEdgesBeg) );
    printf( "\n" );
}
void Abc_NtkCountStats( Sfm_Dec_t * p, int Limit )
{
    int Gate, nGates = Vec_IntSize(&p->vObjGates);
    if ( nGates == Limit )
        return;
    Gate = Vec_IntEntryLast(&p->vObjGates);
    if ( nGates > Limit + 1 )
        p->nNodesResyn++;
    else if ( Gate == p->GateConst0 )
        p->nNodesConst0++;
    else if ( Gate == p->GateConst1 )
        p->nNodesConst1++;
    else if ( Gate == p->GateBuffer )
        p->nNodesBuf++;
    else if ( Gate == p->GateInvert )
        p->nNodesInv++;
    else 
        p->nNodesResyn++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t * Abc_NtkAreaOptOne( Sfm_Dec_t * p, int i )
{
    abctime clk;
    Abc_Ntk_t * pNtk = p->pNtk;
    Sfm_Par_t * pPars = p->pPars;
    Abc_Obj_t * pObj = Abc_NtkObj( p->pNtk, i ); 
    int Limit, RetValue;
    if ( pPars->nMffcMin > 1 && Abc_NodeMffcLabel(pObj) < pPars->nMffcMin )
        return NULL;
    if ( pPars->iNodeOne && i != pPars->iNodeOne )
        return NULL;
    if ( pPars->iNodeOne )
        pPars->fVeryVerbose = (int)(i == pPars->iNodeOne);
    p->nNodesTried++;
clk = Abc_Clock();
    p->nDivs = Sfm_DecExtract( pNtk, pPars, pObj, &p->vObjRoots, &p->vObjGates, &p->vObjFanins, &p->vObjMap, &p->vGateTfi, &p->vGateTfo, &p->vObjMffc, &p->vObjInMffc, NULL, NULL );
p->timeWin += Abc_Clock() - clk;
    if ( pPars->nWinSizeMax && pPars->nWinSizeMax < Vec_IntSize(&p->vObjGates) )
        return NULL;
    p->nMffc = Vec_IntSize(&p->vObjMffc);
    p->AreaMffc = Sfm_DecMffcArea(pNtk, &p->vObjMffc);
    p->nMaxDivs = Abc_MaxInt( p->nMaxDivs, p->nDivs );
    p->nAllDivs += p->nDivs;
    p->iTarget = pObj->iTemp;
    Limit = Vec_IntSize( &p->vObjGates );
    p->nMaxWin = Abc_MaxInt( p->nMaxWin, Limit );
    p->nAllWin += Limit;
clk = Abc_Clock();
    RetValue = Sfm_DecPrepareSolver( p );
p->timeCnf += Abc_Clock() - clk;
    if ( !RetValue )
        return NULL;
clk = Abc_Clock();
    RetValue = Sfm_DecPeformDec2( p, pObj );
    if ( pPars->fMoreEffort && RetValue < 0 )
    {
        int Var, i;
        Vec_IntForEachEntryReverse( &p->vObjInMffc, Var, i )
        {
            p->iUseThis = Var; 
            RetValue = Sfm_DecPeformDec2( p, pObj );
            p->iUseThis = -1;
            if ( RetValue < 0 )
            {
                //printf( "Node %d: Not found among %d.\n", Abc_ObjId(pObj), Vec_IntSize(&p->vObjInMffc) );
            }
            else
            {
                p->nEfforts++;
                if ( p->pPars->fVerbose )
                {
                    //printf( "Node %5d: (%2d out of %2d)  Gate=%s ", Abc_ObjId(pObj), i, Vec_IntSize(&p->vObjInMffc), Mio_GateReadName((Mio_Gate_t*)pObj->pData) );
                    //Dau_DsdPrintFromTruth( p->Copy, p->nSuppVars );
                }
                break;
            }
        }
    }
    if ( p->pPars->fVeryVerbose )
        printf( "\n\n" );
p->timeSat += Abc_Clock() - clk;
    if ( RetValue < 0 )
        return NULL;
    p->nNodesChanged++;
    Abc_NtkCountStats( p, Limit );
    return Sfm_DecInsert( pNtk, pObj, Limit, &p->vObjGates, &p->vObjFanins, &p->vObjMap, &p->vGateHands, p->GateBuffer, p->GateInvert, &p->vGateFuncs, NULL, p->pMit );
}
void Abc_NtkAreaOpt( Sfm_Dec_t * p )
{
    Abc_Obj_t * pObj; 
    int i, nStop = Abc_NtkObjNumMax(p->pNtk);
    Abc_NtkForEachNode( p->pNtk, pObj, i )
    {
        if ( i >= nStop || (p->pPars->nNodesMax && i > p->pPars->nNodesMax) )
            break;
        Abc_NtkAreaOptOne( p, i );
    }
}
void Abc_NtkAreaOpt2( Sfm_Dec_t * p )
{
    Abc_Obj_t * pObj, * pObjNew, * pFanin; 
    int i, k, nStop = Abc_NtkObjNumMax(p->pNtk);
    Vec_Ptr_t * vFront = Vec_PtrAlloc( 1000 );
    Abc_NtkForEachObj( p->pNtk, pObj, i )
        assert( pObj->fMarkB == 0 );
    // start the queue of nodes to be tried
    Abc_NtkForEachCo( p->pNtk, pObj, i )
        if ( Abc_ObjIsNode(Abc_ObjFanin0(pObj)) && !Abc_ObjFanin0(pObj)->fMarkB )
        {
            Abc_ObjFanin0(pObj)->fMarkB = 1;
            Vec_PtrPush( vFront, Abc_ObjFanin0(pObj) );
        }
    // process nodes in this order
    Vec_PtrForEachEntry( Abc_Obj_t *, vFront, pObj, i )
    {
        if ( Abc_ObjIsNone(pObj) )
            continue;
        pObjNew = Abc_NtkAreaOptOne( p, Abc_ObjId(pObj) );
        if ( pObjNew != NULL )
        {
            if ( !Abc_ObjIsNode(pObjNew) || Abc_ObjFaninNum(pObjNew) == 0 || pObjNew->fMarkB )
                continue;
            if ( (int)Abc_ObjId(pObjNew) < nStop )
            {
                pObjNew->fMarkB = 1;
                Vec_PtrPush( vFront, pObjNew );
                continue;
            }
        }
        else
            pObjNew = pObj;
        Abc_ObjForEachFanin( pObjNew, pFanin, k )
            if ( Abc_ObjIsNode(pFanin) && Abc_ObjFaninNum(pObjNew) > 0 && !pFanin->fMarkB )
            {
                pFanin->fMarkB = 1;
                Vec_PtrPush( vFront, pFanin );
            }
    }
    Abc_NtkForEachObj( p->pNtk, pObj, i )
        pObj->fMarkB = 0;
    Vec_PtrFree( vFront );
}

void Abc_NtkDelayOpt( Sfm_Dec_t * p )
{
    Abc_Ntk_t * pNtk = p->pNtk;
    Sfm_Par_t * pPars = p->pPars; int n;
    Abc_NtkCleanMarkABC( pNtk );
    for ( n = 0; pPars->nNodesMax == 0 || n < pPars->nNodesMax; n++ )
    {
        Abc_Obj_t * pObj, * pObjNew; abctime clk;
        int i = 0, Limit, RetValue;
        // collect nodes
        if ( pPars->iNodeOne )
            Vec_IntFill( &p->vCands, 1, pPars->iNodeOne );
        else if ( p->pTim && !Sfm_TimPriorityNodes(p->pTim, &p->vCands, p->pPars->nTimeWin) )
            break;
        else if ( p->pMit && !Sfm_MitPriorityNodes(p->pMit, &p->vCands, p->pPars->nTimeWin) )
            break;
        // try improving delay for the nodes according to the priority
        Abc_NtkForEachObjVec( &p->vCands, p->pNtk, pObj, i )
        {
            int OldId = Abc_ObjId(pObj);
            int DelayOld = Sfm_ManReadObjDelay(p, OldId);
            assert( pObj->fMarkA == 0 );
            p->nNodesTried++;
clk = Abc_Clock();
            p->nDivs = Sfm_DecExtract( pNtk, pPars, pObj, &p->vObjRoots, &p->vObjGates, &p->vObjFanins, &p->vObjMap, &p->vGateTfi, &p->vGateTfo, &p->vObjMffc, &p->vObjInMffc, p->pTim, p->pMit );
p->timeWin += Abc_Clock() - clk;
            if ( p->nDivs < 2 || (pPars->nWinSizeMax && pPars->nWinSizeMax < Vec_IntSize(&p->vObjGates)) )
            { 
                pObj->fMarkA = 1;
                continue;
            }
            p->nMffc = Vec_IntSize(&p->vObjMffc);
            p->AreaMffc = Sfm_DecMffcArea(pNtk, &p->vObjMffc);
            p->nMaxDivs = Abc_MaxInt( p->nMaxDivs, p->nDivs );
            p->nAllDivs += p->nDivs;
            p->iTarget = pObj->iTemp;
            Limit = Vec_IntSize( &p->vObjGates );
            p->nMaxWin = Abc_MaxInt( p->nMaxWin, Limit );
            p->nAllWin += Limit;
clk = Abc_Clock();
            RetValue = Sfm_DecPrepareSolver( p );
p->timeCnf += Abc_Clock() - clk;
            if ( !RetValue )
            {
                pObj->fMarkA = 1;
                continue;
            }
clk = Abc_Clock();
            RetValue = Sfm_DecPeformDec3( p, pObj );
            if ( pPars->fMoreEffort && RetValue < 0 )
            {
                int Var, i;
                Vec_IntForEachEntryReverse( &p->vObjInMffc, Var, i )
                {
                    p->iUseThis = Var; 
                    RetValue = Sfm_DecPeformDec3( p, pObj );
                    p->iUseThis = -1;
                    if ( RetValue < 0 )
                    {
                        //printf( "Node %d: Not found among %d.\n", Abc_ObjId(pObj), Vec_IntSize(&p->vObjInMffc) );
                    }
                    else
                    {
                        p->nEfforts++;
                        if ( p->pPars->fVerbose )
                        {
                            //printf( "Node %5d: (%2d out of %2d)  Gate=%s ", Abc_ObjId(pObj), i, Vec_IntSize(&p->vObjInMffc), Mio_GateReadName((Mio_Gate_t*)pObj->pData) );
                            //Dau_DsdPrintFromTruth( p->Copy, p->nSuppVars );
                        }
                        break;
                    }
                }
            }
            if ( p->pPars->fVeryVerbose )
                printf( "\n\n" );
p->timeSat += Abc_Clock() - clk;
            if ( RetValue < 0 )
            {
                pObj->fMarkA = 1;
                continue;
            }
            assert( Vec_IntSize(&p->vObjGates) - Limit > 0 );
            assert( Vec_IntSize(&p->vObjGates) - Limit <= 2 );
            p->nNodesChanged++;
            Abc_NtkCountStats( p, Limit );
            // reduce load due to removed MFFC
            if ( p->pMit ) Sfm_MitUpdateLoad( p->pMit, &p->vGateMffc, 0 ); // assuming &p->vGateMffc contains MFFC
            Sfm_DecInsert( pNtk, pObj, Limit, &p->vObjGates, &p->vObjFanins, &p->vObjMap, &p->vGateHands, p->GateBuffer, p->GateInvert, &p->vGateFuncs, &p->vNewNodes, p->pMit );
            // increase load due to added new nodes
            if ( p->pMit ) Sfm_MitUpdateLoad( p->pMit, &p->vNewNodes, 1 ); // assuming &p->vNewNodes contains new nodes
clk = Abc_Clock();
            if ( p->pMit )
                Sfm_MitUpdateTiming( p->pMit, &p->vNewNodes );
            else
                Sfm_TimUpdateTiming( p->pTim, &p->vNewNodes );
p->timeTime += Abc_Clock() - clk;
            pObjNew = Abc_NtkObj( pNtk, Abc_NtkObjNumMax(pNtk)-1 );
            assert( p->pMit || p->DelayMin == 0 || p->DelayMin == Sfm_ManReadObjDelay(p, Abc_ObjId(pObjNew)) );
            // report
            if ( pPars->fDelayVerbose )
                printf( "Node %5d  %5d :  I =%3d.  Cand = %5d (%6.2f %%)   Old =%8.2f.  New =%8.2f.  Final =%8.2f.  WNS =%8.2f.\n", 
                    OldId, Abc_NtkObjNumMax(p->pNtk), i, Vec_IntSize(&p->vCands), 100.0 * Vec_IntSize(&p->vCands) / Abc_NtkNodeNum(p->pNtk),
                    Scl_Int2Flt(DelayOld), Scl_Int2Flt(Sfm_ManReadObjDelay(p, Abc_ObjId(pObjNew))), 
                    Scl_Int2Flt(Sfm_ManReadNtkDelay(p)), Scl_Int2Flt(Sfm_ManReadNtkMinSlack(p)) );
            break;
        }
        if ( pPars->iNodeOne )
            break;
    }
    Abc_NtkCleanMarkABC( pNtk );
}
void Abc_NtkPerformMfs3( Abc_Ntk_t * pNtk, Sfm_Par_t * pPars )
{
    Sfm_Dec_t * p = Sfm_DecStart( pPars, (Mio_Library_t *)pNtk->pManFunc, pNtk );
    if ( pPars->fVerbose )
    {
        printf( "Remapping parameters: " );
        if ( pPars->nTfoLevMax )
            printf( "TFO = %d. ",     pPars->nTfoLevMax );
        if ( pPars->nTfiLevMax )
            printf( "TFI = %d. ",     pPars->nTfiLevMax );
        if ( pPars->nFanoutMax )
            printf( "FanMax = %d. ",  pPars->nFanoutMax );
        if ( pPars->nWinSizeMax )
            printf( "WinMax = %d. ",  pPars->nWinSizeMax );
        if ( pPars->nBTLimit )
            printf( "Confl = %d. ",   pPars->nBTLimit );
        if ( pPars->nMffcMin && pPars->fArea )
            printf( "MffcMin = %d. ", pPars->nMffcMin );
        if ( pPars->nMffcMax && pPars->fArea )
            printf( "MffcMax = %d. ", pPars->nMffcMax );
        if ( pPars->nDecMax )
            printf( "DecMax = %d. ",  pPars->nDecMax );
        if ( pPars->iNodeOne )
            printf( "Pivot = %d. ",   pPars->iNodeOne );
        if ( !pPars->fArea )
            printf( "Win = %d. ",     pPars->nTimeWin );
        if ( !pPars->fArea )
            printf( "Delta = %.2f ps. ", Scl_Int2Flt(p->DeltaCrit) );
        if ( pPars->fArea )
            printf( "0-cost = %s. ",  pPars->fZeroCost ? "yes" : "no" );
        printf( "Effort = %s. ",      pPars->fMoreEffort ? "yes" : "no" );
        printf( "Sim = %s. ",         pPars->fUseSim ? "yes" : "no" );
        printf( "\n" );
    }
    // preparation steps
    Abc_NtkLevel( pNtk );
    if ( p->pPars->fUseSim )
        Sfm_NtkSimulate( pNtk );
    // record statistics
    if ( pPars->fVerbose ) p->nTotalNodesBeg = Abc_NtkNodeNum(pNtk);
    if ( pPars->fVerbose ) p->nTotalEdgesBeg = Abc_NtkGetTotalFanins(pNtk);
    // perform optimization
    if ( pPars->fArea )
    {
        if ( pPars->fAreaRev )
            Abc_NtkAreaOpt2( p );
        else
            Abc_NtkAreaOpt( p );
    }
    else
        Abc_NtkDelayOpt( p );
    // record statistics
    if ( pPars->fVerbose ) p->nTotalNodesEnd = Abc_NtkNodeNum(pNtk);
    if ( pPars->fVerbose ) p->nTotalEdgesEnd = Abc_NtkGetTotalFanins(pNtk);
    // print stats and quit
    if ( pPars->fVerbose )
    Sfm_DecPrintStats( p );
    if ( pPars->fLibVerbose )
        Sfm_LibPrint( p->pLib );
    Sfm_DecStop( p );
    if ( pPars->fArea )
    {
        extern void Abc_NtkChangePerform( Abc_Ntk_t * pNtk, int fVerbose );
        Abc_NtkChangePerform( pNtk, pPars->fVerbose );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

