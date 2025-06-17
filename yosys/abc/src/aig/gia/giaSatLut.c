/**CFile****************************************************************

  FileName    [giaSatLut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaSatLut.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/tim/tim.h"
#include "misc/util/utilTruth.h"
#include "sat/bsat/satStore.h"
#include "misc/util/utilNam.h"
#include "map/scl/sclCon.h"
#include "misc/vec/vecHsh.h"

#ifdef _MSC_VER
#define unlink _unlink
#else
#include <unistd.h>
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Sbl_Man_t_ Sbl_Man_t;
struct Sbl_Man_t_
{
    sat_solver *   pSat;         // SAT solver
    Vec_Int_t *    vCardVars;    // candinality variables
    int            nVars;        // max vars
    int            LogN;         // base-2 log of max vars
    int            Power2;       // power-2 of LogN
    int            FirstVar;     // first variable to be used
    // statistics
    int            nTried;       // nodes tried
    int            nImproved;    // nodes improved
    int            nRuns;        // the number of runs
    int            nHashWins;    // the number of hashed windows
    int            nSmallWins;   // the number of small windows
    int            nLargeWins;   // the number of large windows
    int            nIterOuts;    // the number of iters exceeded
    // parameters
    int            LutSize;      // LUT size
    int            nBTLimit;     // conflicts
    int            DelayMax;     // external delay
    int            nEdges;       // the number of edges
    int            fDelay;       // delay mode
    int            fReverse;     // reverse windowing
    int            fVerbose;     // verbose
    int            fVeryVerbose; // verbose
    int            fVeryVeryVerbose; // verbose
    // window
    Gia_Man_t *    pGia;
    Vec_Int_t *    vLeaves;      // leaf nodes
    Vec_Int_t *    vAnds;        // AND-gates
    Vec_Int_t *    vNodes;       // internal LUTs
    Vec_Int_t *    vRoots;       // driver nodes (a subset of vAnds)
    Vec_Int_t *    vRootVars;    // driver nodes (as SAT variables)
    Hsh_VecMan_t * pHash;        // hash table for windows
    // timing 
    Vec_Int_t *    vArrs;        // arrival times  
    Vec_Int_t *    vReqs;        // required times  
    Vec_Wec_t *    vWindow;      // fanins of each node in the window
    Vec_Int_t *    vPath;        // critical path (as SAT variables)
    Vec_Int_t *    vEdges;       // fanin edges
    // cuts
    Vec_Wrd_t *    vCutsI1;      // input bit patterns
    Vec_Wrd_t *    vCutsI2;      // input bit patterns
    Vec_Wrd_t *    vCutsN1;      // node bit patterns
    Vec_Wrd_t *    vCutsN2;      // node bit patterns
    Vec_Int_t *    vCutsNum;     // cut counts for each obj
    Vec_Int_t *    vCutsStart;   // starting cuts for each obj
    Vec_Int_t *    vCutsObj;     // object to which this cut belongs
    Vec_Wrd_t *    vTempI1;      // temporary cuts
    Vec_Wrd_t *    vTempI2;      // temporary cuts
    Vec_Wrd_t *    vTempN1;      // temporary cuts
    Vec_Wrd_t *    vTempN2;      // temporary cuts
    Vec_Int_t *    vSolInit;     // initial solution
    Vec_Int_t *    vSolCur;      // current solution
    Vec_Int_t *    vSolBest;     // best solution
    // temporary
    Vec_Int_t *    vLits;        // literals
    Vec_Int_t *    vAssump;      // literals
    Vec_Int_t *    vPolar;       // variables polarity
    // statistics
    abctime        timeWin;      // windowing
    abctime        timeCut;      // cut computation
    abctime        timeSat;      // SAT runtime
    abctime        timeSatSat;   // satisfiable time
    abctime        timeSatUns;   // unsatisfiable time
    abctime        timeSatUnd;   // undecided time
    abctime        timeTime;     // timing time
    abctime        timeStart;    // starting time
    abctime        timeTotal;    // all runtime
    abctime        timeOther;    // other time
};

extern sat_solver * Sbm_AddCardinSolver( int LogN, Vec_Int_t ** pvVars );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sbl_Man_t * Sbl_ManAlloc( Gia_Man_t * pGia, int Number )
{
    Sbl_Man_t * p = ABC_CALLOC( Sbl_Man_t, 1 );
    p->nVars      = Number;
    p->LogN       = Abc_Base2Log(Number);
    p->Power2     = 1 << p->LogN;
    p->pSat       = Sbm_AddCardinSolver( p->LogN, &p->vCardVars );
    p->FirstVar   = sat_solver_nvars( p->pSat );
    sat_solver_bookmark( p->pSat );
    // window
    p->pGia       = pGia;
    p->vLeaves    = Vec_IntAlloc( p->nVars );
    p->vAnds      = Vec_IntAlloc( p->nVars );
    p->vNodes     = Vec_IntAlloc( p->nVars );
    p->vRoots     = Vec_IntAlloc( p->nVars );
    p->vRootVars  = Vec_IntAlloc( p->nVars );
    p->pHash      = Hsh_VecManStart( 1000 );
    // timing
    p->vArrs      = Vec_IntAlloc( 0 );
    p->vReqs      = Vec_IntAlloc( 0 );
    p->vWindow    = Vec_WecAlloc( 128 );
    p->vPath      = Vec_IntAlloc( 32 );
    p->vEdges     = Vec_IntAlloc( 32 );
    // cuts
    p->vCutsI1    = Vec_WrdAlloc( 1000 );     // input bit patterns
    p->vCutsI2    = Vec_WrdAlloc( 1000 );     // input bit patterns
    p->vCutsN1    = Vec_WrdAlloc( 1000 );     // node bit patterns
    p->vCutsN2    = Vec_WrdAlloc( 1000 );     // node bit patterns
    p->vCutsNum   = Vec_IntAlloc( 64 );       // cut counts for each obj
    p->vCutsStart = Vec_IntAlloc( 64 );       // starting cuts for each obj
    p->vCutsObj   = Vec_IntAlloc( 1000 );
    p->vSolInit   = Vec_IntAlloc( 64 );
    p->vSolCur    = Vec_IntAlloc( 64 );
    p->vSolBest   = Vec_IntAlloc( 64 );
    p->vTempI1    = Vec_WrdAlloc( 32 ); 
    p->vTempI2    = Vec_WrdAlloc( 32 ); 
    p->vTempN1    = Vec_WrdAlloc( 32 ); 
    p->vTempN2    = Vec_WrdAlloc( 32 ); 
    // internal
    p->vLits      = Vec_IntAlloc( 64 );
    p->vAssump    = Vec_IntAlloc( 64 );
    p->vPolar     = Vec_IntAlloc( 1000 );
    // other
    Gia_ManFillValue( pGia );
    return p;
}
void Sbl_ManClean( Sbl_Man_t * p )
{
    p->timeStart = Abc_Clock();
    sat_solver_rollback( p->pSat );
    sat_solver_bookmark( p->pSat );
    // internal
    Vec_IntClear( p->vLeaves );
    Vec_IntClear( p->vAnds );
    Vec_IntClear( p->vNodes );
    Vec_IntClear( p->vRoots );
    Vec_IntClear( p->vRootVars );
    // timing
    Vec_IntClear( p->vArrs );
    Vec_IntClear( p->vReqs );
    Vec_WecClear( p->vWindow );
    Vec_IntClear( p->vPath );
    Vec_IntClear( p->vEdges );
    // cuts
    Vec_WrdClear( p->vCutsI1 );
    Vec_WrdClear( p->vCutsI2 );
    Vec_WrdClear( p->vCutsN1 );
    Vec_WrdClear( p->vCutsN2 );
    Vec_IntClear( p->vCutsNum );
    Vec_IntClear( p->vCutsStart );
    Vec_IntClear( p->vCutsObj );
    Vec_IntClear( p->vSolInit );
    Vec_IntClear( p->vSolCur );
    Vec_IntClear( p->vSolBest );
    Vec_WrdClear( p->vTempI1 );
    Vec_WrdClear( p->vTempI2 );
    Vec_WrdClear( p->vTempN1 );
    Vec_WrdClear( p->vTempN2 );
    // temporary
    Vec_IntClear( p->vLits );
    Vec_IntClear( p->vAssump );
    Vec_IntClear( p->vPolar );
    // other
    Gia_ManFillValue( p->pGia );
}
void Sbl_ManStop( Sbl_Man_t * p )
{
    sat_solver_delete( p->pSat );
    Vec_IntFree( p->vCardVars );
    // internal
    Vec_IntFree( p->vLeaves );
    Vec_IntFree( p->vAnds );
    Vec_IntFree( p->vNodes );
    Vec_IntFree( p->vRoots );
    Vec_IntFree( p->vRootVars );
    Hsh_VecManStop( p->pHash );
    // timing
    Vec_IntFree( p->vArrs );
    Vec_IntFree( p->vReqs );
    Vec_WecFree( p->vWindow );
    Vec_IntFree( p->vPath );
    Vec_IntFree( p->vEdges );
    // cuts
    Vec_WrdFree( p->vCutsI1 );
    Vec_WrdFree( p->vCutsI2 );
    Vec_WrdFree( p->vCutsN1 );
    Vec_WrdFree( p->vCutsN2 );
    Vec_IntFree( p->vCutsNum );
    Vec_IntFree( p->vCutsStart );
    Vec_IntFree( p->vCutsObj );
    Vec_IntFree( p->vSolInit );
    Vec_IntFree( p->vSolCur );
    Vec_IntFree( p->vSolBest );
    Vec_WrdFree( p->vTempI1 );
    Vec_WrdFree( p->vTempI2 );
    Vec_WrdFree( p->vTempN1 );
    Vec_WrdFree( p->vTempN2 );
    // temporary
    Vec_IntFree( p->vLits );
    Vec_IntFree( p->vAssump );
    Vec_IntFree( p->vPolar );
    // other
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [For each node in the window, create fanins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sbl_ManGetCurrentMapping( Sbl_Man_t * p )
{
    Vec_Int_t * vObj;
    word CutI1, CutI2, CutN1, CutN2;
    int i, c, b, iObj;
    Vec_WecClear( p->vWindow );
    Vec_WecInit( p->vWindow, Vec_IntSize(p->vAnds) );
    assert( Vec_IntSize(p->vSolCur) > 0 );
    Vec_IntForEachEntry( p->vSolCur, c, i )
    {
        CutI1 = Vec_WrdEntry( p->vCutsI1, c );
        CutI2 = Vec_WrdEntry( p->vCutsI2, c );
        CutN1 = Vec_WrdEntry( p->vCutsN1, c );
        CutN2 = Vec_WrdEntry( p->vCutsN2, c );
        iObj  = Vec_IntEntry( p->vCutsObj, c );
        //iObj  = Vec_IntEntry( p->vAnds, iObj );
        vObj  = Vec_WecEntry( p->vWindow, iObj );
        Vec_IntClear( vObj );
        assert( Vec_IntSize(vObj) == 0 );
        for ( b = 0; b < 64; b++ )
            if ( (CutI1 >> b) & 1 )
                Vec_IntPush( vObj, Vec_IntEntry(p->vLeaves, b) );
        for ( b = 0; b < 64; b++ )
            if ( (CutI2 >> b) & 1 )
                Vec_IntPush( vObj, Vec_IntEntry(p->vLeaves, 64+b) );
        for ( b = 0; b < 64; b++ )
            if ( (CutN1 >> b) & 1 )
                Vec_IntPush( vObj, Vec_IntEntry(p->vAnds, b) );
        for ( b = 0; b < 64; b++ )
            if ( (CutN2 >> b) & 1 )
                Vec_IntPush( vObj, Vec_IntEntry(p->vAnds, 64+b) );
    }
}


/**Function*************************************************************

  Synopsis    [Timing computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sbl_ManComputeDelay( Sbl_Man_t * p, int iLut, Vec_Int_t * vFanins )
{
    int k, iFan, Delay = 0;
    Vec_IntForEachEntry( vFanins, iFan, k )
        Delay = Abc_MaxInt( Delay, Vec_IntEntry(p->vArrs, iFan) + 1 );
    return Delay;
}
int Sbl_ManCreateTiming( Sbl_Man_t * p, int DelayStart )
{
    Vec_Int_t * vFanins;
    int DelayMax = DelayStart, Delay, iLut, iFan, k;
    // compute arrival times
    Vec_IntFill( p->vArrs,  Gia_ManObjNum(p->pGia), 0 );
    if ( p->pGia->pManTime != NULL && Tim_ManBoxNum((Tim_Man_t*)p->pGia->pManTime) )
    {
        Gia_Obj_t * pObj; 
        Vec_Int_t * vNodes = Gia_ManOrderWithBoxes( p->pGia );
        Tim_ManIncrementTravId( (Tim_Man_t*)p->pGia->pManTime );
        Gia_ManForEachObjVec( vNodes, p->pGia, pObj, k )
        {
            iLut = Gia_ObjId( p->pGia, pObj );
            if ( Gia_ObjIsAnd(pObj) )
            {
                if ( Gia_ObjIsLut2(p->pGia, iLut) )
                {
                    vFanins = Gia_ObjLutFanins2(p->pGia, iLut);
                    Delay = Sbl_ManComputeDelay( p, iLut, vFanins );
                    Vec_IntWriteEntry( p->vArrs,  iLut, Delay );
                    DelayMax = Abc_MaxInt( DelayMax, Delay );
                }
            }
            else if ( Gia_ObjIsCi(pObj) )
            {
                int arrTime = Tim_ManGetCiArrival( (Tim_Man_t*)p->pGia->pManTime, Gia_ObjCioId(pObj) );
                Vec_IntWriteEntry( p->vArrs,  iLut, arrTime );
            }
            else if ( Gia_ObjIsCo(pObj) )
            {
                int arrTime = Vec_IntEntry( p->vArrs, Gia_ObjFaninId0(pObj, iLut) );
                Tim_ManSetCoArrival( (Tim_Man_t*)p->pGia->pManTime, Gia_ObjCioId(pObj), arrTime );
            }
            else if ( !Gia_ObjIsConst0(pObj) ) 
                assert( 0 );
        }
        Vec_IntFree( vNodes );
    }
    else
    {
        Gia_ManForEachLut2( p->pGia, iLut )
        {
            vFanins = Gia_ObjLutFanins2(p->pGia, iLut);
            Delay = Sbl_ManComputeDelay( p, iLut, vFanins );
            Vec_IntWriteEntry( p->vArrs,  iLut, Delay );
            DelayMax = Abc_MaxInt( DelayMax, Delay );
        }
    }
    // compute required times
    Vec_IntFill( p->vReqs, Gia_ManObjNum(p->pGia), ABC_INFINITY );
    Gia_ManForEachCoDriverId( p->pGia, iLut, k )
        Vec_IntDowndateEntry( p->vReqs, iLut, DelayMax );
    if ( p->pGia->pManTime != NULL && Tim_ManBoxNum((Tim_Man_t*)p->pGia->pManTime) )
    {
        Gia_Obj_t * pObj; 
        Vec_Int_t * vNodes = Gia_ManOrderWithBoxes( p->pGia );
        Tim_ManIncrementTravId( (Tim_Man_t*)p->pGia->pManTime );
        Tim_ManInitPoRequiredAll( (Tim_Man_t*)p->pGia->pManTime, DelayMax );
        Gia_ManForEachObjVecReverse( vNodes, p->pGia, pObj, k )
        {
            iLut = Gia_ObjId( p->pGia, pObj );
            if ( Gia_ObjIsAnd(pObj) )
            {
                if ( Gia_ObjIsLut2(p->pGia, iLut) )
                {
                    Delay = Vec_IntEntry(p->vReqs, iLut) - 1;
                    vFanins = Gia_ObjLutFanins2(p->pGia, iLut);
                    Vec_IntForEachEntry( vFanins, iFan, k )
                        Vec_IntDowndateEntry( p->vReqs, iFan, Delay );
                }
            }
            else if ( Gia_ObjIsCi(pObj) )
            {
                int reqTime = Vec_IntEntry( p->vReqs, iLut );
                Tim_ManSetCiRequired( (Tim_Man_t*)p->pGia->pManTime, Gia_ObjCioId(pObj), reqTime );
            }
            else if ( Gia_ObjIsCo(pObj) )
            {
                int reqTime = Tim_ManGetCoRequired( (Tim_Man_t*)p->pGia->pManTime, Gia_ObjCioId(pObj) );
                Vec_IntWriteEntry( p->vReqs, Gia_ObjFaninId0(pObj, iLut), reqTime );
            }
            else if ( !Gia_ObjIsConst0(pObj) ) 
                assert( 0 );
        }
        Vec_IntFree( vNodes );
    }
    else
    {
        Gia_ManForEachLut2Reverse( p->pGia, iLut )
        {
            Delay = Vec_IntEntry(p->vReqs, iLut) - 1;
            vFanins = Gia_ObjLutFanins2(p->pGia, iLut);
            Vec_IntForEachEntry( vFanins, iFan, k )
                Vec_IntDowndateEntry( p->vReqs, iFan, Delay );
        }
    }
    return DelayMax;
}


/**Function*************************************************************

  Synopsis    [Given mapping in p->vSolCur, check if mapping meets delay.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sbl_ManEvaluateMappingEdge( Sbl_Man_t * p, int DelayGlo )
{
    abctime clk = Abc_Clock();
    Vec_Int_t * vArray; 
    int i, DelayMax;
    Vec_IntClear( p->vPath );
    // update new timing
    Sbl_ManGetCurrentMapping( p );
    // derive new timing
    DelayMax = Gia_ManEvalWindow( p->pGia, p->vLeaves, p->vAnds, p->vWindow, p->vPolar, 1 );
    p->timeTime += Abc_Clock() - clk;
    if ( DelayMax <= DelayGlo )
        return 1;
    // create critical path composed of all nodes
    Vec_WecForEachLevel( p->vWindow, vArray, i )
        if ( Vec_IntSize(vArray) > 0 )
            Vec_IntPush( p->vPath, Abc_Var2Lit(i, 1) );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Given mapping in p->vSolCur, check the critical path.]

  Description [Returns 1 if the mapping satisfies the timing. Returns 0, 
  if the critical path is detected.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sbl_ManCriticalFanin( Sbl_Man_t * p, int iLut, Vec_Int_t * vFanins )
{
    int k, iFan, Delay = Vec_IntEntry(p->vArrs, iLut);
    Vec_IntForEachEntry( vFanins, iFan, k )
        if ( Vec_IntEntry(p->vArrs, iFan) + 1 == Delay )
            return iFan;
    return -1;
}
int Sbl_ManEvaluateMapping( Sbl_Man_t * p, int DelayGlo )
{
    abctime clk = Abc_Clock();
    Vec_Int_t * vFanins;
    int i, iLut = -1, iAnd, Delay, Required;
    if ( p->pGia->vEdge1 )
        return Sbl_ManEvaluateMappingEdge( p, DelayGlo );
    Vec_IntClear( p->vPath );
    // derive timing
    Sbl_ManCreateTiming( p, DelayGlo );
    // update new timing
    Sbl_ManGetCurrentMapping( p );
    Vec_IntForEachEntry( p->vAnds, iLut, i )
    {
        vFanins = Vec_WecEntry( p->vWindow, i );
        Delay   = Sbl_ManComputeDelay( p, iLut, vFanins );
        Vec_IntWriteEntry( p->vArrs, iLut, Delay );
    }
    // compare timing at the root nodes
    Vec_IntForEachEntry( p->vRoots, iLut, i )
    {
        Delay    = Vec_IntEntry( p->vArrs, iLut );
        Required = Vec_IntEntry( p->vReqs, iLut );
        if ( Delay > Required ) // updated timing exceeded original timing
            break;
    }
    p->timeTime += Abc_Clock() - clk;
    if ( i == Vec_IntSize(p->vRoots) )
        return 1;
    // derive the critical path

    // find SAT variable of the node whose GIA ID is "iLut"
    iAnd = Vec_IntFind( p->vAnds, iLut );
    assert( iAnd >= 0 );
    // critical path begins in node "iLut", which is i-th root of the window
    assert( iAnd == Vec_IntEntry(p->vRootVars, i) );
    while ( 1 )
    {
        Vec_IntPush( p->vPath, Abc_Var2Lit(iAnd, 1) );
        // find fanins of this node
        vFanins = Vec_WecEntry( p->vWindow, iAnd );
        // find critical fanin
        iLut = Sbl_ManCriticalFanin( p, iLut, vFanins );
        assert( iLut > 0  );
        // find SAT variable of the node whose GIA ID is "iLut"
        iAnd = Vec_IntFind( p->vAnds, iLut );
        if ( iAnd == -1 )
            break;
    }
    return 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sbl_ManUpdateMapping( Sbl_Man_t * p )
{
//    Gia_Obj_t * pObj;
    Vec_Int_t * vObj;
    word CutI1, CutI2, CutN1, CutN2;
    int i, c, b, iObj, iTemp; 
    assert( Vec_IntSize(p->vSolBest) < Vec_IntSize(p->vSolInit) );
    Vec_IntForEachEntry( p->vAnds, iObj, i )
    {
        vObj = Vec_WecEntry(p->pGia->vMapping2, iObj);
        Vec_IntForEachEntry( vObj, iTemp, b )
            Gia_ObjLutRefDecId( p->pGia, iTemp );
        Vec_IntClear( vObj );
    }
    Vec_IntForEachEntry( p->vSolBest, c, i )
    {
        CutI1 = Vec_WrdEntry( p->vCutsI1, c );
        CutI2 = Vec_WrdEntry( p->vCutsI2, c );
        CutN1 = Vec_WrdEntry( p->vCutsN1, c );
        CutN2 = Vec_WrdEntry( p->vCutsN2, c );
        iObj = Vec_IntEntry( p->vCutsObj, c );
        iObj = Vec_IntEntry( p->vAnds, iObj );
        vObj = Vec_WecEntry( p->pGia->vMapping2, iObj );
        Vec_IntClear( vObj );
        assert( Vec_IntSize(vObj) == 0 );
        for ( b = 0; b < 64; b++ )
            if ( (CutI1 >> b) & 1 )
                Vec_IntPush( vObj, Vec_IntEntry(p->vLeaves, b) );
        for ( b = 0; b < 64; b++ )
            if ( (CutI2 >> b) & 1 )
                Vec_IntPush( vObj, Vec_IntEntry(p->vLeaves, 64+b) );
        for ( b = 0; b < 64; b++ )
            if ( (CutN1 >> b) & 1 )
                Vec_IntPush( vObj, Vec_IntEntry(p->vAnds, b) );
        for ( b = 0; b < 64; b++ )
            if ( (CutN2 >> b) & 1 )
                Vec_IntPush( vObj, Vec_IntEntry(p->vAnds, 64+b) );
        Vec_IntForEachEntry( vObj, iTemp, b )
            Gia_ObjLutRefIncId( p->pGia, iTemp );
    }
/*
    // verify
    Gia_ManForEachLut2Vec( p->pGia, vObj, i )
        Vec_IntForEachEntry( vObj, iTemp, b )
            Gia_ObjLutRefDecId( p->pGia, iTemp );
    Gia_ManForEachCo( p->pGia, pObj, i )
        Gia_ObjLutRefDecId( p->pGia, Gia_ObjFaninId0p(p->pGia, pObj) );

    for ( i = 0; i < Gia_ManObjNum(p->pGia); i++ )
        if ( p->pGia->pLutRefs[i] )
            printf( "Object %d has %d refs\n", i, p->pGia->pLutRefs[i] );

    Gia_ManForEachCo( p->pGia, pObj, i )
        Gia_ObjLutRefIncId( p->pGia, Gia_ObjFaninId0p(p->pGia, pObj) );
    Gia_ManForEachLut2Vec( p->pGia, vObj, i )
        Vec_IntForEachEntry( vObj, iTemp, b )
            Gia_ObjLutRefIncId( p->pGia, iTemp );
*/
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Sbl_ManPrintCut( word CutI1, word CutI2, word CutN1, word CutN2 )
{
    int b, Count = 0; 
    printf( "{ " );
    for ( b = 0; b < 64; b++ )
        if ( (CutI1 >> b) & 1 )
            printf( "i%d ", b ), Count++;
    for ( b = 0; b < 64; b++ )
        if ( (CutI2 >> b) & 1 )
            printf( "i%d ", 64+b ), Count++;
    printf( " " );
    for ( b = 0; b < 64; b++ )
        if ( (CutN1 >> b) & 1 )
            printf( "n%d ", b ), Count++;
    for ( b = 0; b < 64; b++ )
        if ( (CutN2 >> b) & 1 )
            printf( "n%d ", 64+b ), Count++;
    printf( "};\n" );
    return Count;
}
static int Sbl_ManFindAndPrintCut( Sbl_Man_t * p, int c )
{
    return Sbl_ManPrintCut( Vec_WrdEntry(p->vCutsI1, c), Vec_WrdEntry(p->vCutsI2, c), Vec_WrdEntry(p->vCutsN1, c), Vec_WrdEntry(p->vCutsN2, c) );
}
static inline int Sbl_CutIsFeasible( word CutI1, word CutI2, word CutN1, word CutN2, int LutSize )
{
    int Count = (CutI1 != 0) + (CutI2 != 0) + (CutN1 != 0) + (CutN2 != 0);
    assert( LutSize <= 6 );
    CutI1 &= CutI1-1; CutI2 &= CutI2-1;   CutN1 &= CutN1-1; CutN2 &= CutN2-1;   Count += (CutI1 != 0) + (CutI2 != 0) + (CutN1 != 0) + (CutN2 != 0);
    CutI1 &= CutI1-1; CutI2 &= CutI2-1;   CutN1 &= CutN1-1; CutN2 &= CutN2-1;   Count += (CutI1 != 0) + (CutI2 != 0) + (CutN1 != 0) + (CutN2 != 0);
    CutI1 &= CutI1-1; CutI2 &= CutI2-1;   CutN1 &= CutN1-1; CutN2 &= CutN2-1;   Count += (CutI1 != 0) + (CutI2 != 0) + (CutN1 != 0) + (CutN2 != 0);  
    CutI1 &= CutI1-1; CutI2 &= CutI2-1;   CutN1 &= CutN1-1; CutN2 &= CutN2-1;   Count += (CutI1 != 0) + (CutI2 != 0) + (CutN1 != 0) + (CutN2 != 0);
    if ( LutSize <= 4 )
        return Count <= 4;
    CutI1 &= CutI1-1; CutI2 &= CutI2-1;   CutN1 &= CutN1-1; CutN2 &= CutN2-1;   Count += (CutI1 != 0) + (CutI2 != 0) + (CutN1 != 0) + (CutN2 != 0);  
    CutI1 &= CutI1-1; CutI2 &= CutI2-1;   CutN1 &= CutN1-1; CutN2 &= CutN2-1;   Count += (CutI1 != 0) + (CutI2 != 0) + (CutN1 != 0) + (CutN2 != 0);
    return Count <= 6;
}
static inline int Sbl_CutPushUncontained( Vec_Wrd_t * vCutsI1, Vec_Wrd_t * vCutsI2, Vec_Wrd_t * vCutsN1, Vec_Wrd_t * vCutsN2, word CutI1, word CutI2, word CutN1, word CutN2 )
{
    int i, k = 0;
    assert( vCutsI1->nSize == vCutsN1->nSize );
    assert( vCutsI2->nSize == vCutsN2->nSize );
    for ( i = 0; i < vCutsI1->nSize; i++ )
        if ( (vCutsI1->pArray[i] & CutI1) == vCutsI1->pArray[i] && 
             (vCutsI2->pArray[i] & CutI2) == vCutsI2->pArray[i] && 
             (vCutsN1->pArray[i] & CutN1) == vCutsN1->pArray[i] && 
             (vCutsN2->pArray[i] & CutN2) == vCutsN2->pArray[i]  )
            return 1;
    for ( i = 0; i < vCutsI1->nSize; i++ )
        if ( (vCutsI1->pArray[i] & CutI1) != CutI1 ||
             (vCutsI2->pArray[i] & CutI2) != CutI2 || 
             (vCutsN1->pArray[i] & CutN1) != CutN1 || 
             (vCutsN2->pArray[i] & CutN2) != CutN2 )
        {
            Vec_WrdWriteEntry( vCutsI1, k, vCutsI1->pArray[i] );
            Vec_WrdWriteEntry( vCutsI2, k, vCutsI2->pArray[i] );
            Vec_WrdWriteEntry( vCutsN1, k, vCutsN1->pArray[i] );
            Vec_WrdWriteEntry( vCutsN2, k, vCutsN2->pArray[i] );
            k++;
        }
    Vec_WrdShrink( vCutsI1, k );
    Vec_WrdShrink( vCutsI2, k );
    Vec_WrdShrink( vCutsN1, k );
    Vec_WrdShrink( vCutsN2, k );
    Vec_WrdPush( vCutsI1, CutI1 );
    Vec_WrdPush( vCutsI2, CutI2 );
    Vec_WrdPush( vCutsN1, CutN1 );
    Vec_WrdPush( vCutsN2, CutN2 );
    return 0;
}
static inline void Sbl_ManComputeCutsOne( Sbl_Man_t * p, int Fan0, int Fan1, int Obj )
{
    word * pCutsI1 = Vec_WrdArray(p->vCutsI1);
    word * pCutsI2 = Vec_WrdArray(p->vCutsI2);
    word * pCutsN1 = Vec_WrdArray(p->vCutsN1);
    word * pCutsN2 = Vec_WrdArray(p->vCutsN2);
    int Start0 = Vec_IntEntry( p->vCutsStart, Fan0 );
    int Start1 = Vec_IntEntry( p->vCutsStart, Fan1 );
    int Limit0 = Start0 + Vec_IntEntry( p->vCutsNum, Fan0 );
    int Limit1 = Start1 + Vec_IntEntry( p->vCutsNum, Fan1 );
    int i, k;
    assert( Obj >= 0 && Obj < 128 );
    Vec_WrdClear( p->vTempI1 );
    Vec_WrdClear( p->vTempI2 );
    Vec_WrdClear( p->vTempN1 );
    Vec_WrdClear( p->vTempN2 );
    for ( i = Start0; i < Limit0; i++ )
        for ( k = Start1; k < Limit1; k++ )
            if ( Sbl_CutIsFeasible(pCutsI1[i] | pCutsI1[k], pCutsI2[i] | pCutsI2[k], pCutsN1[i] | pCutsN1[k], pCutsN2[i] | pCutsN2[k], p->LutSize) )
                Sbl_CutPushUncontained( p->vTempI1, p->vTempI2, p->vTempN1, p->vTempN2, pCutsI1[i] | pCutsI1[k], pCutsI2[i] | pCutsI2[k], pCutsN1[i] | pCutsN1[k], pCutsN2[i] | pCutsN2[k] );
    Vec_IntPush( p->vCutsStart, Vec_WrdSize(p->vCutsI1) );
    Vec_IntPush( p->vCutsNum, Vec_WrdSize(p->vTempI1) + 1 );
    Vec_WrdAppend( p->vCutsI1, p->vTempI1 );
    Vec_WrdAppend( p->vCutsI2, p->vTempI2 );
    Vec_WrdAppend( p->vCutsN1, p->vTempN1 );
    Vec_WrdAppend( p->vCutsN2, p->vTempN2 );
    Vec_WrdPush( p->vCutsI1, 0 );
    Vec_WrdPush( p->vCutsI2, 0 );
    if ( Obj < 64 )
    {
        Vec_WrdPush( p->vCutsN1, (((word)1) << Obj) );
        Vec_WrdPush( p->vCutsN2, 0 );
    }
    else
    {
        Vec_WrdPush( p->vCutsN1, 0 );
        Vec_WrdPush( p->vCutsN2, (((word)1) << (Obj-64)) );
    }
    for ( i = 0; i <= Vec_WrdSize(p->vTempI1); i++ )
        Vec_IntPush( p->vCutsObj, Obj );
}
static inline int Sbl_ManFindCut( Sbl_Man_t * p, int Obj, word CutI1, word CutI2, word CutN1, word CutN2 )
{
    word * pCutsI1 = Vec_WrdArray(p->vCutsI1);
    word * pCutsI2 = Vec_WrdArray(p->vCutsI2);
    word * pCutsN1 = Vec_WrdArray(p->vCutsN1);
    word * pCutsN2 = Vec_WrdArray(p->vCutsN2);
    int Start0 = Vec_IntEntry( p->vCutsStart, Obj );
    int Limit0 = Start0 + Vec_IntEntry( p->vCutsNum, Obj );
    int i;
    //printf( "\nLooking for:\n" );
    //Sbl_ManPrintCut( CutI, CutN );
    //printf( "\n" );
    for ( i = Start0; i < Limit0; i++ )
    {
        //Sbl_ManPrintCut( pCutsI[i], pCutsN[i] );
        if ( pCutsI1[i] == CutI1 && pCutsI2[i] == CutI2 && pCutsN1[i] == CutN1 && pCutsN2[i] == CutN2 )
            return i;
    }
    return -1;
}
int Sbl_ManComputeCuts( Sbl_Man_t * p )
{
    abctime clk = Abc_Clock();
    Gia_Obj_t * pObj; Vec_Int_t * vFanins;
    int i, k, Index, Fanin, nObjs = Vec_IntSize(p->vLeaves) + Vec_IntSize(p->vAnds);
    assert( Vec_IntSize(p->vLeaves) <= 128 && Vec_IntSize(p->vAnds) <= p->nVars );
    // assign leaf cuts
    Vec_IntClear( p->vCutsStart );
    Vec_IntClear( p->vCutsObj );
    Vec_IntClear( p->vCutsNum );
    Vec_WrdClear( p->vCutsI1 );
    Vec_WrdClear( p->vCutsI2 );
    Vec_WrdClear( p->vCutsN1 );
    Vec_WrdClear( p->vCutsN2 );
    Gia_ManForEachObjVec( p->vLeaves, p->pGia, pObj, i )
    {
        Vec_IntPush( p->vCutsStart, Vec_WrdSize(p->vCutsI1) );
        Vec_IntPush( p->vCutsObj, -1 );
        Vec_IntPush( p->vCutsNum, 1 );
        if ( i < 64 )
        {
            Vec_WrdPush( p->vCutsI1, (((word)1) << i) );
            Vec_WrdPush( p->vCutsI2, 0 );
        }
        else
        {
            Vec_WrdPush( p->vCutsI1, 0 );
            Vec_WrdPush( p->vCutsI2, (((word)1) << (i-64)) );
        }
        Vec_WrdPush( p->vCutsN1, 0 );
        Vec_WrdPush( p->vCutsN2, 0 );
        pObj->Value = i;
    }
    // assign internal cuts
    Gia_ManForEachObjVec( p->vAnds, p->pGia, pObj, i )
    {
        assert( Gia_ObjIsAnd(pObj) );
        assert( ~Gia_ObjFanin0(pObj)->Value );
        assert( ~Gia_ObjFanin1(pObj)->Value );
        Sbl_ManComputeCutsOne( p, Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value, i );
        pObj->Value = Vec_IntSize(p->vLeaves) + i;
    }
    assert( Vec_IntSize(p->vCutsStart) == nObjs );
    assert( Vec_IntSize(p->vCutsNum)   == nObjs );
    assert( Vec_WrdSize(p->vCutsI1)    == Vec_WrdSize(p->vCutsN1) );
    assert( Vec_WrdSize(p->vCutsI2)    == Vec_WrdSize(p->vCutsN2) );
    assert( Vec_WrdSize(p->vCutsI1)    == Vec_IntSize(p->vCutsObj) );
    // check that roots are mapped nodes
    Vec_IntClear( p->vRootVars );
    Gia_ManForEachObjVec( p->vRoots, p->pGia, pObj, i )
    {
        int Obj = Gia_ObjId(p->pGia, pObj);
        if ( Gia_ObjIsCi(pObj) )
            continue;
        assert( Gia_ObjIsLut2(p->pGia, Obj) );
        assert( ~pObj->Value );
        Vec_IntPush( p->vRootVars, pObj->Value - Vec_IntSize(p->vLeaves) );
    }
    // create current solution
    Vec_IntClear( p->vPolar );
    Vec_IntClear( p->vSolInit );
    Gia_ManForEachObjVec( p->vAnds, p->pGia, pObj, i )
    {
        word CutI1 = 0, CutI2 = 0, CutN1 = 0, CutN2 = 0;
        int Obj = Gia_ObjId(p->pGia, pObj);
        if ( !Gia_ObjIsLut2(p->pGia, Obj) )
            continue;
        assert( (int)pObj->Value == Vec_IntSize(p->vLeaves) + i );
        // add node
        Vec_IntPush( p->vPolar, i );
        Vec_IntPush( p->vSolInit, i );
        // add its cut
        //Gia_LutForEachFaninObj( p->pGia, Obj, pFanin, k )
        vFanins = Gia_ObjLutFanins2( p->pGia, Obj );
        Vec_IntForEachEntry( vFanins, Fanin, k )
        {
            Gia_Obj_t * pFanin = Gia_ManObj( p->pGia, Fanin );
            assert( (int)pFanin->Value < Vec_IntSize(p->vLeaves) || Gia_ObjIsLut2(p->pGia, Fanin) );
//            if ( ~pFanin->Value == 0 )
//                Gia_ManPrintConeMulti( p->pGia, p->vAnds, p->vLeaves, p->vPath );
            if ( ~pFanin->Value == 0 ) 
                continue;
            assert( ~pFanin->Value );
            if ( (int)pFanin->Value < Vec_IntSize(p->vLeaves) )
            {
                if ( (int)pFanin->Value < 64 )
                    CutI1 |= ((word)1 << pFanin->Value);
                else
                    CutI2 |= ((word)1 << (pFanin->Value - 64));
            }
            else
            {
                if ( pFanin->Value - Vec_IntSize(p->vLeaves) < 64 )
                    CutN1 |= ((word)1 << (pFanin->Value - Vec_IntSize(p->vLeaves)));
                else
                    CutN2 |= ((word)1 << (pFanin->Value - Vec_IntSize(p->vLeaves) - 64));
            }
        }
        // find the new cut
        Index = Sbl_ManFindCut( p, Vec_IntSize(p->vLeaves) + i, CutI1, CutI2, CutN1, CutN2 );
        if ( Index < 0 )
        {
            //printf( "Cannot find the available cut.\n" );
            continue;
        }
        assert( Index >= 0 );
        Vec_IntPush( p->vPolar, p->FirstVar+Index );
    }
    // clean value
    Gia_ManForEachObjVec( p->vLeaves, p->pGia, pObj, i )
        pObj->Value = ~0;
    Gia_ManForEachObjVec( p->vAnds, p->pGia, pObj, i )
        pObj->Value = ~0;
    p->timeCut += Abc_Clock() - clk;
    return Vec_WrdSize(p->vCutsI1);
}
int Sbl_ManCreateCnf( Sbl_Man_t * p )
{
    int i, k, c, pLits[2], value;
    word * pCutsN1 = Vec_WrdArray(p->vCutsN1);
    word * pCutsN2 = Vec_WrdArray(p->vCutsN2);
    assert( p->FirstVar == sat_solver_nvars(p->pSat) );
    sat_solver_setnvars( p->pSat, sat_solver_nvars(p->pSat) + Vec_WrdSize(p->vCutsI1) );
    //printf( "\n" );
    for ( i = 0; i < Vec_IntSize(p->vAnds); i++ )
    {
        int Start0 = Vec_IntEntry( p->vCutsStart, Vec_IntSize(p->vLeaves) + i );
        int Limit0 = Start0 + Vec_IntEntry( p->vCutsNum, Vec_IntSize(p->vLeaves) + i ) - 1;
        // add main clause
        Vec_IntClear( p->vLits );
        Vec_IntPush( p->vLits, Abc_Var2Lit(i, 1) );
        //printf( "Node %d implies cuts: ", i );
        for ( k = Start0; k < Limit0; k++ )
        {
            Vec_IntPush( p->vLits, Abc_Var2Lit(p->FirstVar+k, 0) );
            //printf( "%d ", p->FirstVar+k );
        }
        //printf( "\n" );
        value = sat_solver_addclause( p->pSat, Vec_IntArray(p->vLits), Vec_IntLimit(p->vLits)  );
        assert( value );
        // binary clauses
        for ( k = Start0; k < Limit0; k++ )
        {
            word Cut1 = pCutsN1[k];
            word Cut2 = pCutsN2[k];
            //printf( "Cut %d implies root node %d.\n", p->FirstVar+k, i );
            // root clause
            pLits[0] = Abc_Var2Lit( p->FirstVar+k, 1 );
            pLits[1] = Abc_Var2Lit( i, 0 );
            value = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
            assert( value );
            // fanin clauses
            for ( c = 0; c < 64 && Cut1; c++, Cut1 >>= 1 )
            {
                if ( (Cut1 & 1) == 0 )
                    continue;
                //printf( "Cut %d implies fanin %d.\n", p->FirstVar+k, c );
                pLits[1] = Abc_Var2Lit( c, 0 );
                value = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
                assert( value );
            }
            for ( c = 0; c < 64 && Cut2; c++, Cut2 >>= 1 )
            {
                if ( (Cut2 & 1) == 0 )
                    continue;
                //printf( "Cut %d implies fanin %d.\n", p->FirstVar+k, c );
                pLits[1] = Abc_Var2Lit( c+64, 0 );
                value = sat_solver_addclause( p->pSat, pLits, pLits + 2 );
                assert( value );
            }
        }
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

void Sbl_ManWindow( Sbl_Man_t * p )
{
    int i, ObjId;
    // collect leaves
    Vec_IntClear( p->vLeaves );
    Gia_ManForEachCiId( p->pGia, ObjId, i )
        Vec_IntPush( p->vLeaves, ObjId );
    // collect internal
    Vec_IntClear( p->vAnds );
    Gia_ManForEachAndId( p->pGia, ObjId )
        Vec_IntPush( p->vAnds, ObjId );
    // collect roots
    Vec_IntClear( p->vRoots );
    Gia_ManForEachCoDriverId( p->pGia, ObjId, i )
        Vec_IntPush( p->vRoots, ObjId );
}

int Sbl_ManWindow2( Sbl_Man_t * p, int iPivot )
{
    abctime clk = Abc_Clock();
    Vec_Int_t * vRoots, * vNodes, * vLeaves, * vAnds;
    int Count = Gia_ManComputeOneWin( p->pGia, iPivot, &vRoots, &vNodes, &vLeaves, &vAnds );
    p->timeWin += Abc_Clock() - clk;
    if ( Count == 0 )
        return 0;
    Vec_IntClear( p->vRoots );   Vec_IntAppend( p->vRoots,  vRoots );
    Vec_IntClear( p->vNodes );   Vec_IntAppend( p->vNodes,  vNodes );
    Vec_IntClear( p->vLeaves );  Vec_IntAppend( p->vLeaves, vLeaves );
    Vec_IntClear( p->vAnds );    Vec_IntAppend( p->vAnds,   vAnds );
//Vec_IntPrint( vRoots );
//Vec_IntPrint( vAnds );
//Vec_IntPrint( vLeaves );
    // recompute internal nodes
//    Gia_ManIncrementTravId( p->pGia );
//    Gia_ManCollectAnds( p->pGia, Vec_IntArray(p->vRoots), Vec_IntSize(p->vRoots), p->vAnds, p->vLeaves );
    return Count;
}

int Sbl_ManTestSat( Sbl_Man_t * p, int iPivot )
{
    int fKeepTrying = 1;
    abctime clk = Abc_Clock(), clk2;
    int i, status, Root, Count, StartSol, nConfTotal = 0, nIters = 0;
    int nEntries = Hsh_VecSize( p->pHash );
    p->nTried++;

    Sbl_ManClean( p );

    // compute one window
    Count = Sbl_ManWindow2( p, iPivot );
    if ( Count == 0 )
    {
        if ( p->fVeryVerbose )
        printf( "Obj %d: Window with less than %d nodes does not exist.\n", iPivot, p->nVars );
        p->nSmallWins++;
        return 0;
    }
    Hsh_VecManAdd( p->pHash, p->vAnds );
    if ( nEntries == Hsh_VecSize(p->pHash) )
    {
        if ( p->fVeryVerbose )
        printf( "Obj %d: This window was already tried.\n", iPivot );
        p->nHashWins++;
        return 0;
    }
    if ( p->fVeryVerbose )
    printf( "\nObj = %6d : Leaf = %2d.  AND = %2d.  Root = %2d.    LUT = %2d.\n", 
        iPivot, Vec_IntSize(p->vLeaves), Vec_IntSize(p->vAnds), Vec_IntSize(p->vRoots), Vec_IntSize(p->vNodes) ); 

    if ( Vec_IntSize(p->vLeaves) > 128 || Vec_IntSize(p->vAnds) > p->nVars )
    {
        if ( p->fVeryVerbose )
        printf( "Obj %d: Encountered window with %d inputs and %d internal nodes.\n", iPivot, Vec_IntSize(p->vLeaves), Vec_IntSize(p->vAnds) );
        p->nLargeWins++;
        return 0;
    }
    if ( Vec_IntSize(p->vAnds) < 10 )
    {
        if ( p->fVeryVerbose )
        printf( "Skipping.\n" );
        return 0;
    }

    // derive cuts
    Sbl_ManComputeCuts( p );
    // derive SAT instance
    Sbl_ManCreateCnf( p );

    if ( p->fVeryVeryVerbose )
    printf( "All clauses = %d.  Multi clauses = %d.  Binary clauses = %d.  Other clauses = %d.\n\n", 
        sat_solver_nclauses(p->pSat), Vec_IntSize(p->vAnds), Vec_WrdSize(p->vCutsI1) - Vec_IntSize(p->vAnds), 
        sat_solver_nclauses(p->pSat) - Vec_WrdSize(p->vCutsI1) );

    // create assumptions
    // cardinality
    Vec_IntClear( p->vAssump );
    Vec_IntPush( p->vAssump, -1 );
    // unused variables
    for ( i = Vec_IntSize(p->vAnds); i < p->Power2; i++ )
        Vec_IntPush( p->vAssump, Abc_Var2Lit(i, 1) );
    // root variables
    Vec_IntForEachEntry( p->vRootVars, Root, i )
        Vec_IntPush( p->vAssump, Abc_Var2Lit(Root, 0) );
//    Vec_IntPrint( p->vAssump );

    StartSol = Vec_IntSize(p->vSolInit) + 1;
//    StartSol = 30;
    while ( fKeepTrying && StartSol-fKeepTrying > 0 )
    {
        int Count = 0, LitCount = 0;
        int nConfBef, nConfAft;
        if ( p->fVeryVerbose )
            printf( "Trying to find mapping with %d LUTs.\n", StartSol-fKeepTrying );
    //    for ( i = Vec_IntSize(p->vSolInit)-5; i < nVars; i++ )
    //        Vec_IntPush( p->vAssump, Abc_Var2Lit(Vec_IntEntry(p->vCardVars, i), 1) );
        Vec_IntWriteEntry( p->vAssump, 0, Abc_Var2Lit(Vec_IntEntry(p->vCardVars, StartSol-fKeepTrying), 1) );
        // solve the problem
        clk2 = Abc_Clock();
        nConfBef = (int)p->pSat->stats.conflicts;
        status = sat_solver_solve( p->pSat, Vec_IntArray(p->vAssump), Vec_IntLimit(p->vAssump), p->nBTLimit, 0, 0, 0 );
        p->timeSat += Abc_Clock() - clk2;
        nConfAft = (int)p->pSat->stats.conflicts;
        nConfTotal += nConfAft - nConfBef;
        nIters++;
        p->nRuns++;
        if ( status == l_True )
            p->timeSatSat += Abc_Clock() - clk2;
        else if ( status == l_False )
            p->timeSatUns += Abc_Clock() - clk2;
        else 
            p->timeSatUnd += Abc_Clock() - clk2;
        if ( status == l_Undef )
            break;
        if ( status == l_True )
        {
            if ( p->fVeryVeryVerbose )
            {
                for ( i = 0; i < Vec_IntSize(p->vAnds); i++ )
                    printf( "%d", sat_solver_var_value(p->pSat, i) );
                printf( "\n" );
                for ( i = 0; i < Vec_IntSize(p->vAnds); i++ )
                    if ( sat_solver_var_value(p->pSat, i) )
                    {
                        printf( "%d=%d ", i, sat_solver_var_value(p->pSat, i) );
                        Count++;
                    }
                printf( "Count = %d\n", Count );
            }
//            for ( i = p->FirstVar; i < sat_solver_nvars(p->pSat); i++ )
//                printf( "%d", sat_solver_var_value(p->pSat, i) );
//            printf( "\n" );
            Count = 1;
            Vec_IntClear( p->vSolCur );
            for ( i = p->FirstVar; i < sat_solver_nvars(p->pSat); i++ )
                if ( sat_solver_var_value(p->pSat, i) )
                {
                    if ( p->fVeryVeryVerbose )
                        printf( "Cut %3d : Node = %3d %6d  ", Count++, Vec_IntEntry(p->vCutsObj, i-p->FirstVar), Vec_IntEntry(p->vAnds, Vec_IntEntry(p->vCutsObj, i-p->FirstVar)) );
                    if ( p->fVeryVeryVerbose )
                        LitCount += Sbl_ManFindAndPrintCut( p, i-p->FirstVar );
                    Vec_IntPush( p->vSolCur, i-p->FirstVar );
                }
            //Vec_IntPrint( p->vRootVars );
            //Vec_IntPrint( p->vRoots );
            //Vec_IntPrint( p->vAnds );
            //Vec_IntPrint( p->vLeaves );
        }

//        fKeepTrying = status == l_True ? fKeepTrying + 1 : 0;
        // check the timing
        if ( status == l_True )
        {
            if ( p->fDelay && !Sbl_ManEvaluateMapping(p, p->DelayMax) )
            {
                int iLit, value;
                if ( p->fVeryVerbose )
                {
                    printf( "Critical path of length (%d) is detected:   ", Vec_IntSize(p->vPath) );
                    Vec_IntForEachEntry( p->vPath, iLit, i )
                        printf( "%d=%d ", i, Vec_IntEntry(p->vAnds, Abc_Lit2Var(iLit)) );
                    printf( "\n" );
                }
                // add the clause
                value = sat_solver_addclause( p->pSat, Vec_IntArray(p->vPath), Vec_IntLimit(p->vPath)  );
                assert( value );
            }
            else
            {
                Vec_IntClear( p->vSolBest );
                Vec_IntAppend( p->vSolBest, p->vSolCur );
                fKeepTrying++;
            }
        }
        else
            fKeepTrying = 0;
        if ( p->fVeryVerbose )
        {
            if ( status == l_False )
                printf( "UNSAT " );
            else if ( status == l_True )
                printf( "SAT   " );
            else 
                printf( "UNDEC " );
            printf( "confl =%8d.    ", nConfAft - nConfBef );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk2 );

            printf( "Total " );
            printf( "confl =%8d.    ", nConfTotal );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
            if ( p->fVeryVeryVerbose && status == l_True )
                printf( "LitCount = %d.\n", LitCount );
            printf( "\n" );
        }
        if ( nIters == 10 )
        {
            p->nIterOuts++;
            //printf( "Obj %d : Quitting after %d iterations.\n", iPivot, nIters );
            break;
        }
    }

    // update solution
    if ( Vec_IntSize(p->vSolBest) > 0 && Vec_IntSize(p->vSolBest) < Vec_IntSize(p->vSolInit) )
    {
        int nDelayCur, nEdgesCur = 0;
        Sbl_ManUpdateMapping( p );
        if ( p->pGia->vEdge1 )
        {
            nDelayCur = Gia_ManEvalEdgeDelay( p->pGia );
            nEdgesCur = Gia_ManEvalEdgeCount( p->pGia );
        }
        else
            nDelayCur = Sbl_ManCreateTiming( p, p->DelayMax );
        if ( p->fVerbose )
        printf( "Object %5d : Saved %2d nodes  (Conf =%8d)  Iter =%3d  Delay = %d  Edges = %4d\n", 
            iPivot, Vec_IntSize(p->vSolInit)-Vec_IntSize(p->vSolBest), nConfTotal, nIters, nDelayCur, nEdgesCur );
        p->timeTotal += Abc_Clock() - p->timeStart;
        p->nImproved++;
        return 2;
    }
    else
    {
//        printf( "Object %5d : Saved %2d nodes  (Conf =%8d)  Iter =%3d\n", iPivot, 0, nConfTotal, nIters );
    }
    p->timeTotal += Abc_Clock() - p->timeStart;
    return 1;
}
void Sbl_ManPrintRuntime( Sbl_Man_t * p )
{
    printf( "Runtime breakdown:\n" );
    p->timeOther = p->timeTotal - p->timeWin - p->timeCut - p->timeSat - p->timeTime;
    ABC_PRTP( "Win   ", p->timeWin  ,   p->timeTotal );
    ABC_PRTP( "Cut   ", p->timeCut  ,   p->timeTotal );
    ABC_PRTP( "Sat   ", p->timeSat,     p->timeTotal );
    ABC_PRTP( " Sat  ", p->timeSatSat,  p->timeTotal );
    ABC_PRTP( " Unsat", p->timeSatUns,  p->timeTotal );
    ABC_PRTP( " Undec", p->timeSatUnd,  p->timeTotal );
    ABC_PRTP( "Timing", p->timeTime ,   p->timeTotal );
    ABC_PRTP( "Other ", p->timeOther,   p->timeTotal );
    ABC_PRTP( "ALL   ", p->timeTotal,   p->timeTotal );
}
void Gia_ManLutSat( Gia_Man_t * pGia, int LutSize, int nNumber, int nImproves, int nBTLimit, int DelayMax, int nEdges, int fDelay, int fReverse, int fVerbose, int fVeryVerbose )
{
    int iLut, nImproveCount = 0;
    Sbl_Man_t * p   = Sbl_ManAlloc( pGia, nNumber );
    p->LutSize      = LutSize;      // LUT size
    p->nBTLimit     = nBTLimit;     // conflicts
    p->DelayMax     = DelayMax;     // external delay
    p->nEdges       = nEdges;       // the number of edges
    p->fDelay       = fDelay;       // delay mode
    p->fReverse     = fReverse;     // reverse windowing
    p->fVerbose     = fVerbose | fVeryVerbose;
    p->fVeryVerbose = fVeryVerbose;
    if ( p->fVerbose )
    printf( "Parameters: WinSize = %d AIG nodes.  Conf = %d.  DelayMax = %d.\n", p->nVars, p->nBTLimit, p->DelayMax );
    // determine delay limit
    if ( fDelay && pGia->vEdge1 && p->DelayMax == 0 )
        p->DelayMax = Gia_ManEvalEdgeDelay( pGia );
    // iterate through the internal nodes
    Gia_ManComputeOneWinStart( pGia, nNumber, fReverse );
    Gia_ManForEachLut2( pGia, iLut )
    {
        if ( Sbl_ManTestSat( p, iLut ) != 2 )
            continue;
        if ( ++nImproveCount == nImproves )
            break;
    }
    Gia_ManComputeOneWin( pGia, -1, NULL, NULL, NULL, NULL );
    if ( p->fVerbose )
    printf( "Tried = %d. Used = %d. HashWin = %d. SmallWin = %d. LargeWin = %d. IterOut = %d.  SAT runs = %d.\n", 
        p->nTried, p->nImproved, p->nHashWins, p->nSmallWins, p->nLargeWins, p->nIterOuts, p->nRuns );
    if ( p->fVerbose )
    Sbl_ManPrintRuntime( p );
    Sbl_ManStop( p );
    Vec_IntFreeP( &pGia->vPacking );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_RunKadical( char * pFileNameIn, char * pFileNameOut, int Seed, int nBTLimit, int TimeOut, int fVerbose, int * pStatus )
{
    extern Vec_Int_t * Exa4_ManParse( char *pFileName );
    int fVerboseSolver = 0;
    abctime clkTotal = Abc_Clock();
    Vec_Int_t * vRes = NULL;
#ifdef _WIN32
    char * pKadical = "kadical.exe";
#else
    char * pKadical = "./kadical";
    FILE * pFile = fopen( pKadical, "rb" );
    if ( pFile == NULL )
        pKadical += 2;
    else
        fclose( pFile );
#endif
    char Command[1000], * pCommand = (char *)&Command;
    if ( nBTLimit ) {
        if ( TimeOut )
            sprintf( pCommand, "%s --seed=%d -c %d -t %d %s %s > %s", pKadical, Seed, nBTLimit, TimeOut, fVerboseSolver ? "": "-q", pFileNameIn, pFileNameOut );
        else
            sprintf( pCommand, "%s --seed=%d -c %d  %s %s > %s", pKadical, Seed, nBTLimit, fVerboseSolver ? "": "-q", pFileNameIn, pFileNameOut );
    }
    else {
        if ( TimeOut )
            sprintf( pCommand, "%s --seed=%d -t %d %s %s > %s", pKadical, Seed, TimeOut, fVerboseSolver ? "": "-q", pFileNameIn, pFileNameOut );
        else
            sprintf( pCommand, "%s --seed=%d %s %s > %s", pKadical, Seed, fVerboseSolver ? "": "-q", pFileNameIn, pFileNameOut );
    }
#ifdef __wasm
    if ( 1 )
#else
    if ( system( pCommand ) == -1 )
#endif
    {
        fprintf( stdout, "Command \"%s\" did not succeed.\n", pCommand );
        return 0;
    }
    vRes = Exa4_ManParse( pFileNameOut );
    if ( fVerbose )
    {
        if ( vRes )
            printf( "The problem has a solution. " ), *pStatus = 0;
        else if ( vRes == NULL && TimeOut == 0 )
            printf( "The problem has no solution. " ), *pStatus = 1;
        else if ( vRes == NULL )
            printf( "The problem has no solution or reached a resource limit after %d sec. ", TimeOut ), *pStatus = -1;
        Abc_PrintTime( 1, "SAT solver time", Abc_Clock() - clkTotal );
    }
    return vRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_SatVarReqPos( int i ) { return i*7+0; } // p
int Gia_SatVarReqNeg( int i ) { return i*7+1; } // n
int Gia_SatVarAckPos( int i ) { return i*7+2; } // P
int Gia_SatVarAckNeg( int i ) { return i*7+3; } // N
int Gia_SatVarInv   ( int i ) { return i*7+4; } // i
int Gia_SatVarFan0  ( int i ) { return i*7+5; } // 0
int Gia_SatVarFan1  ( int i ) { return i*7+6; } // 1

int Gia_SatValReqPos( Vec_Int_t * p, int i ) { return Vec_IntEntry(p, i*7+0); } // p
int Gia_SatValReqNeg( Vec_Int_t * p, int i ) { return Vec_IntEntry(p, i*7+1); } // n
int Gia_SatValAckPos( Vec_Int_t * p, int i ) { return Vec_IntEntry(p, i*7+2); } // P
int Gia_SatValAckNeg( Vec_Int_t * p, int i ) { return Vec_IntEntry(p, i*7+3); } // N
int Gia_SatValInv   ( Vec_Int_t * p, int i ) { return Vec_IntEntry(p, i*7+4); } // i
int Gia_SatValFan0  ( Vec_Int_t * p, int i ) { return Vec_IntEntry(p, i*7+5); } // 0
int Gia_SatValFan1  ( Vec_Int_t * p, int i ) { return Vec_IntEntry(p, i*7+6); } // 1

void Gia_SatDumpClause( Vec_Str_t * vStr, int * pLits, int nLits )
{
    for ( int i = 0; i < nLits; i++ )
        Vec_StrPrintF( vStr, "%d ", Abc_LitIsCompl(pLits[i]) ? -Abc_Lit2Var(pLits[i])-1 : Abc_Lit2Var(pLits[i])+1 );
    Vec_StrPrintF( vStr, "0\n" );        
}
void Gia_SatDumpLiteral( Vec_Str_t * vStr, int Lit )
{
    Gia_SatDumpClause( vStr, &Lit, 1 );
}
void Gia_SatDumpKlause( Vec_Str_t * vStr, int nIns, int nAnds, int nBound )
{
    int i, nVars = nIns + 7*nAnds;
    Vec_StrPrintF( vStr, "k %d ", nVars - nBound );
    // counting primary inputs: n
    for ( i = 0; i < nIns; i++ )
        Vec_StrPrintF( vStr, "-%d ", Gia_SatVarReqNeg(1+i)+1 );
    // counting internal nodes: p, n, P, N, i, 0, 1
    for ( i = 0; i < 7*nAnds; i++ )
        Vec_StrPrintF( vStr, "-%d ", (1+nIns)*7+i+1 );
    Vec_StrPrintF( vStr, "0\n" );
}

Vec_Str_t * Gia_ManSimpleCnf( Gia_Man_t * p, int nBound )
{
    Vec_Str_t * vStr = Vec_StrAlloc( 10000 );    
    Gia_SatDumpKlause( vStr, Gia_ManCiNum(p), Gia_ManAndNum(p), nBound );    
    int i, n, m, Id, pLits[4]; Gia_Obj_t * pObj;
    for ( n = 0; n < 7; n++ )
        Gia_SatDumpLiteral( vStr, Abc_Var2Lit(n, 1) );    
    // acknowledge positive PI literals
    Gia_ManForEachCiId( p, Id, i )
        for ( n = 0; n < 7; n++ )  if ( n != 1 )
            Gia_SatDumpLiteral( vStr, Abc_Var2Lit(Gia_SatVarReqPos(Id)+n, n>0) );
    // require driving PO literals
    Gia_ManForEachCo( p, pObj, i )
        Gia_SatDumpLiteral( vStr, Abc_Var2Lit( Gia_SatVarReqPos(Gia_ObjFaninId0p(p, pObj)) + Gia_ObjFaninC0(pObj), 0 ) );
    // internal nodes
    Gia_ManForEachAnd( p, pObj, i ) {
        int fCompl[2] = { Gia_ObjFaninC0(pObj), Gia_ObjFaninC1(pObj) };
        int iFans[2]  = { Gia_ObjFaninId0(pObj, i), Gia_ObjFaninId1(pObj, i) };
        Gia_Obj_t * pFans[2] = { Gia_ObjFanin0(pObj), Gia_ObjFanin1(pObj) };
        // require inverter: p & !n & N -> i, n & !p & P -> i
        for ( n = 0; n < 2; n++ ) {
            pLits[0] = Abc_Var2Lit( Gia_SatVarReqPos(i)+n, 1 );
            pLits[1] = Abc_Var2Lit( Gia_SatVarReqNeg(i)-n, 0 );
            pLits[2] = Abc_Var2Lit( Gia_SatVarAckNeg(i)-n, 1 );
            pLits[3] = Abc_Var2Lit( Gia_SatVarInv   (i), 0 );
            Gia_SatDumpClause( vStr, pLits, 4 );
        }
        // exclusive acknowledge: !P + !N
        pLits[0] = Abc_Var2Lit( Gia_SatVarAckPos(i), 1 );
        pLits[1] = Abc_Var2Lit( Gia_SatVarAckNeg(i), 1 );
        Gia_SatDumpClause( vStr, pLits, 2 );
        // required acknowledge: p -> P + N, n -> P + N
        pLits[1] = Abc_Var2Lit( Gia_SatVarAckPos(i), 0 );
        pLits[2] = Abc_Var2Lit( Gia_SatVarAckNeg(i), 0 );
        pLits[0] = Abc_Var2Lit( Gia_SatVarReqPos(i), 1 );
        Gia_SatDumpClause( vStr, pLits, 3 );
        pLits[0] = Abc_Var2Lit( Gia_SatVarReqNeg(i), 1 );
        Gia_SatDumpClause( vStr, pLits, 3 );
        // forbid acknowledge: !p & !n -> !P, !p & !n -> !N
        pLits[0] = Abc_Var2Lit( Gia_SatVarReqPos(i), 0 );
        pLits[1] = Abc_Var2Lit( Gia_SatVarReqNeg(i), 0 );
        pLits[2] = Abc_Var2Lit( Gia_SatVarAckPos(i), 1 );
        Gia_SatDumpClause( vStr, pLits, 3 );
        pLits[2] = Abc_Var2Lit( Gia_SatVarAckNeg(i), 1 );
        Gia_SatDumpClause( vStr, pLits, 3 );
        // when fanins can be used: !N & !P -> !0, !N & !P -> !1
        pLits[0] = Abc_Var2Lit( Gia_SatVarAckPos(i), 0 );
        pLits[1] = Abc_Var2Lit( Gia_SatVarAckNeg(i), 0 );
        pLits[2] = Abc_Var2Lit( Gia_SatVarFan0(i), 1 );
        Gia_SatDumpClause( vStr, pLits, 3 );
        pLits[2] = Abc_Var2Lit( Gia_SatVarFan1(i), 1 );
        Gia_SatDumpClause( vStr, pLits, 3 );
        // when fanins are not used: 0 -> !N, 0 -> !P, 1 -> !N, 1 -> !P
        for ( m = 0; m < 2; m++ )
        for ( n = 0; n < 2; n++ ) {
            pLits[0] = Abc_Var2Lit( Gia_SatVarFan0(i)+n, 1 );
            pLits[1] = Abc_Var2Lit( Gia_SatVarReqPos(iFans[n])+m, 1 );
            Gia_SatDumpClause( vStr, pLits, 2 );
        }
        // can only extend both when both complemented: !(C0 & C1) -> !0 + !1 
        pLits[0] = Abc_Var2Lit( Gia_SatVarFan0(i), 1 );
        pLits[1] = Abc_Var2Lit( Gia_SatVarFan1(i), 1 );
        if ( !fCompl[0] || !fCompl[1] )
            Gia_SatDumpClause( vStr, pLits, 2 );
        // if fanin is a primary input, cannot extend it (pi -> !0 or pi -> !1)
        for ( n = 0; n < 2; n++ )
            if ( Gia_ObjIsCi(pFans[n]) )
                Gia_SatDumpLiteral( vStr, Abc_Var2Lit( Gia_SatVarFan0(i)+n, 1 ) );
        // propagating assignments when fanin is not used
        // P & !0 -> C0 ? P0 : N0
        // N & !0 -> C0 ? N0 : P0
        // P & !1 -> C1 ? P1 : N1
        // N & !1 -> C1 ? N1 : P1
        for ( m = 0; m < 2; m++ )
        for ( n = 0; n < 2; n++ ) {
            pLits[0] = Abc_Var2Lit( Gia_SatVarAckPos(i)+m, 1 );
            pLits[1] = Abc_Var2Lit( Gia_SatVarFan0(i)+n, 0 );
            pLits[2] = Abc_Var2Lit( Gia_SatVarReqPos(iFans[n]) + !(m ^ fCompl[n]), 0 );
            Gia_SatDumpClause( vStr, pLits, 3 );
        }
        // propagating assignments when fanins are used
        // P & 0 -> (C0 ^ C00) ? P00 : N00
        // P & 0 -> (C0 ^ C01) ? P01 : N01
        // N & 0 -> (C0 ^ C00) ? N00 : P00
        // N & 0 -> (C0 ^ C01) ? N01 : P01
        // P & 1 -> (C1 ^ C10) ? P10 : N10
        // P & 1 -> (C1 ^ C11) ? P11 : N11
        // N & 1 -> (C1 ^ C10) ? N10 : P10
        // N & 1 -> (C1 ^ C11) ? N11 : P11
        for ( m = 0; m < 2; m++ ) 
        for ( n = 0; n < 2; n++ )
        if ( Gia_ObjIsAnd(pFans[n]) ) {
            pLits[0] = Abc_Var2Lit( Gia_SatVarAckPos(i)+m, 1 );
            pLits[1] = Abc_Var2Lit( Gia_SatVarFan0(i)+n, 1 );
            pLits[2] = Abc_Var2Lit( Gia_SatVarReqPos(Gia_ObjFaninId0p(p, pFans[n])) + !(m ^ fCompl[n] ^ Gia_ObjFaninC0(pFans[n])), 0 );
            Gia_SatDumpClause( vStr, pLits, 3 );
            pLits[2] = Abc_Var2Lit( Gia_SatVarReqPos(Gia_ObjFaninId1p(p, pFans[n])) + !(m ^ fCompl[n] ^ Gia_ObjFaninC1(pFans[n])), 0 );
            Gia_SatDumpClause( vStr, pLits, 3 );
        }
    }
    Vec_StrPush( vStr, '\0' );
    return vStr;
}

typedef enum { 
    GIA_GATE_ZERO,     // 0: 
    GIA_GATE_ONE,      // 1: 
    GIA_GATE_BUF,      // 2: 
    GIA_GATE_INV,      // 3: 
    GIA_GATE_NAN2,     // 4: 
    GIA_GATE_NOR2,     // 5: 
    GIA_GATE_AOI21,    // 6: 
    GIA_GATE_NAN3,     // 7: 
    GIA_GATE_NOR3,     // 8: 
    GIA_GATE_OAI21,    // 9: 
    GIA_GATE_AOI22,    // 10: 
    GIA_GATE_OAI22,    // 11: 
    RTM_VAL_VOID       // 12: unused value
} Gia_ManGate_t;

Vec_Int_t * Gia_ManDeriveSimpleMapping( Gia_Man_t * p, Vec_Int_t * vRes )
{
    Vec_Int_t * vMapping = Vec_IntStart( 2*Gia_ManObjNum(p) );
    int i, Id; Gia_Obj_t * pObj;
    Gia_ManForEachCiId( p, Id, i )
        if ( Gia_SatValReqNeg(vRes, Id) )
            Vec_IntWriteEntry( vMapping, Abc_Var2Lit(Id, 1), -1 );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Gia_SatValAckPos(vRes, i) + Gia_SatValAckNeg(vRes, i) == 0 )
            continue;
        assert( Gia_SatValAckPos(vRes, i) != Gia_SatValAckNeg(vRes, i) );
        if ( (Gia_SatValReqPos(vRes, i) && Gia_SatValReqNeg(vRes, i)) || Gia_SatValInv(vRes, i) )
            Vec_IntWriteEntry( vMapping, Abc_Var2Lit(i, Gia_SatValAckPos(vRes, i)), -1 );
        int fComp = Gia_SatValAckNeg(vRes, i);
        int fFan0 = Gia_SatValFan0(vRes, i);
        int fFan1 = Gia_SatValFan1(vRes, i);
        Gia_Obj_t * pFans[2] = { Gia_ObjFanin0(pObj), Gia_ObjFanin1(pObj) };
        Vec_IntWriteEntry( vMapping, Abc_Var2Lit(i, fComp), Vec_IntSize(vMapping) );
        if ( fFan0 && fFan1 ) {
            assert( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) );
            Vec_IntPush( vMapping, 4 );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pFans[0]), !(fComp ^ Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC0(pFans[0]))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pFans[0]), !(fComp ^ Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC1(pFans[0]))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pFans[1]), !(fComp ^ Gia_ObjFaninC1(pObj) ^ Gia_ObjFaninC0(pFans[1]))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pFans[1]), !(fComp ^ Gia_ObjFaninC1(pObj) ^ Gia_ObjFaninC1(pFans[1]))) );
            Vec_IntPush( vMapping, fComp ? GIA_GATE_OAI22 : GIA_GATE_AOI22 );
        } else if ( fFan0 ) {
            Vec_IntPush( vMapping, 3 );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pFans[0]), !(fComp ^ Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC0(pFans[0]))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pFans[0]), !(fComp ^ Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC1(pFans[0]))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pObj),     !(fComp ^ Gia_ObjFaninC1(pObj))) );
            if ( Gia_ObjFaninC0(pObj) )
                Vec_IntPush( vMapping, fComp ? GIA_GATE_OAI21 : GIA_GATE_AOI21 );
            else
                Vec_IntPush( vMapping, fComp ? GIA_GATE_NAN3 : GIA_GATE_NOR3 );
        } else if ( fFan1 ) {
            Vec_IntPush( vMapping, 3 );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pFans[1]), !(fComp ^ Gia_ObjFaninC1(pObj) ^ Gia_ObjFaninC0(pFans[1]))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pFans[1]), !(fComp ^ Gia_ObjFaninC1(pObj) ^ Gia_ObjFaninC1(pFans[1]))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pObj),     !(fComp ^ Gia_ObjFaninC0(pObj))) );
            if ( Gia_ObjFaninC1(pObj) )
                Vec_IntPush( vMapping, fComp ? GIA_GATE_OAI21 : GIA_GATE_AOI21 );
            else
                Vec_IntPush( vMapping, fComp ? GIA_GATE_NAN3 : GIA_GATE_NOR3 );
        } else {
            Vec_IntPush( vMapping, 2 );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pObj),     !(fComp ^ Gia_ObjFaninC0(pObj))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pObj),     !(fComp ^ Gia_ObjFaninC1(pObj))) );
            Vec_IntPush( vMapping, fComp ? GIA_GATE_NAN2 : GIA_GATE_NOR2 );
        }
    }
    return vMapping;
}
void Gia_ManSimplePrintMapping( Vec_Int_t * vRes, int nIns )
{    
    int i, k, nObjs = Vec_IntSize(vRes)/7, nSteps = Abc_Base10Log(nObjs);
    int nCard = Vec_IntSum(vRes) - nIns; char NumStr[10];
    printf( "Solution with cardinality %d:\n", nCard );
    for ( k = 0; k < nSteps; k++ ) {        
        printf( "  " );
        for ( i = 0; i < nObjs; i++ ) {            
            sprintf( NumStr, "%02d", i );
            printf( "%c", NumStr[k] );
        }
        printf( "\n" );
    }
    for ( k = 0; k < 7; k++ ) {
        printf( "%c ", "pnPNi01"[k] );
        for ( i = 0; i < nObjs; i++ )
            if ( Vec_IntEntry( vRes, i*7+k ) == 0 )
                printf( " " );
            else
                printf( "1" );
        printf( "\n" );
    }
}
int Gia_ManDumpCnf( char * pFileName, Vec_Str_t * vStr, int nVars )
{
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL ) { printf( "Cannot open input file \"%s\".\n", pFileName ); return 0; }
    fprintf( pFile, "p knf %d %d\n%s\n", nVars, Vec_StrCountEntry(vStr, '\n'), Vec_StrArray(vStr) );
    fclose( pFile );
    return 1;
}
int Gia_ManDumpCnf2( Vec_Str_t * vStr, int nVars, int argc, char ** argv, abctime Time, int Status )
{
    Vec_Str_t * vFileName = Vec_StrAlloc( 100 ); int c;
    Vec_StrPrintF( vFileName, "%s", argv[0] + (argv[0][0] == '&') );
    for ( c = 1; c < argc; c++ )
        Vec_StrPrintF( vFileName, "_%s", argv[c] + (argv[c][0] == '-') );
    Vec_StrPrintF( vFileName, ".cnf" );
    Vec_StrPush( vFileName, '\0' );
    FILE * pFile = fopen( Vec_StrArray(vFileName), "wb" );
    if ( pFile == NULL ) { printf( "Cannot open output file \"%s\".\n", Vec_StrArray(vFileName) ); Vec_StrFree(vFileName); return 0; }
    Vec_StrFree(vFileName);
    fprintf( pFile, "c This file was generated by ABC command: \"" );
    fprintf( pFile, "%s", argv[0] );
    for ( c = 1; c < argc; c++ )
        fprintf( pFile, " %s", argv[c] );
    fprintf( pFile, "\" on %s\n", Gia_TimeStamp() );
    fprintf( pFile, "c Cardinality CDCL (https://github.com/jreeves3/Cardinality-CDCL) found it to be " );
    if ( Status == 1 )
        fprintf( pFile, "UNSAT" );
    if ( Status == 0 )
        fprintf( pFile, "SAT" );
    if ( Status == -1 )
        fprintf( pFile, "UNDECIDED" );
    fprintf( pFile, " in %.2f sec\n", 1.0*((double)(Time))/((double)CLOCKS_PER_SEC) );
    fprintf( pFile, "p knf %d %d\n%s\n", nVars, Vec_StrCountEntry(vStr, '\n'), Vec_StrArray(vStr) );
    fclose( pFile );
    return 1;
}
int Gia_ManSimpleMapping( Gia_Man_t * p, int nBound, int Seed, int nBTLimit, int nTimeout, int fVerbose, int fKeepFile, int argc, char ** argv )
{
    abctime clkStart = Abc_Clock();
    srand(time(NULL));
    int Status, Rand = ((((unsigned)rand()) << 12) ^ ((unsigned)rand())) & 0xFFFFFF;
    char pFileNameI[32]; sprintf( pFileNameI, "_%06x_.cnf", Rand ); 
    char pFileNameO[32]; sprintf( pFileNameO, "_%06x_.out", Rand ); 
    if ( nBound == 0 ) 
        nBound = 5 * Gia_ManAndNum(p);
    Vec_Str_t * vStr = Gia_ManSimpleCnf( p, nBound/2 );
    int nVars = 7*(Gia_ManObjNum(p)-Gia_ManCoNum(p));
    if ( !Gia_ManDumpCnf(pFileNameI, vStr, nVars) ) {
        Vec_StrFree( vStr );
        return 0;
    }
    if ( fVerbose )
        printf( "SAT variables = %d. SAT clauses = %d. Cardinality bound = %d. Conflict limit = %d. Timeout = %d.\n", 
            nVars, Vec_StrCountEntry(vStr, '\n'), nBound, nBTLimit, nTimeout );
    Vec_Int_t * vRes = Gia_RunKadical( pFileNameI, pFileNameO, Seed, nBTLimit, nTimeout, fVerbose, &Status );
    unlink( pFileNameI );
    //unlink( pFileNameO );
    if ( fKeepFile ) Gia_ManDumpCnf2( vStr, nVars, argc, argv, Abc_Clock() - clkStart, Status );
    Vec_StrFree( vStr );
    if ( vRes == NULL )
        return 0;
    Vec_IntFreeP( &p->vCellMapping );
    assert( p->vCellMapping == NULL );
    Vec_IntDrop( vRes, 0 );
    if ( fVerbose ) Gia_ManSimplePrintMapping( vRes, Gia_ManCiNum(p) );
    p->vCellMapping = Gia_ManDeriveSimpleMapping( p, vRes );
    Vec_IntFree( vRes );
    if ( fVerbose ) Abc_PrintTime( 0, "Total time", Abc_Clock() - clkStart );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

#define KSAT_OBJS  24
#define KSAT_MINTS 64
#define KSAT_SPACE (4+3*KSAT_OBJS+3*KSAT_MINTS)

int Gia_KSatVarInv( int * pMap, int i )               { return pMap[i*KSAT_SPACE+0]; }
int Gia_KSatVarAnd( int * pMap, int i )               { return pMap[i*KSAT_SPACE+1]; }
int Gia_KSatVarEqu( int * pMap, int i )               { return pMap[i*KSAT_SPACE+2]; }
int Gia_KSatVarRef( int * pMap, int i )               { return pMap[i*KSAT_SPACE+3]; }
int Gia_KSatVarFan( int * pMap, int i, int f, int k ) { return pMap[i*KSAT_SPACE+4+f*KSAT_OBJS+k];     } 
int Gia_KSatVarMin( int * pMap, int i, int m, int k ) { return pMap[i*KSAT_SPACE+4+3*KSAT_OBJS+3*m+k]; } 

void Gia_KSatSetInv( int * pMap, int i, int iVar )               { assert( -1 == pMap[i*KSAT_SPACE+0] ); pMap[i*KSAT_SPACE+0] = iVar; }
void Gia_KSatSetAnd( int * pMap, int i, int iVar )               { assert( -1 == pMap[i*KSAT_SPACE+1] ); pMap[i*KSAT_SPACE+1] = iVar; }
void Gia_KSatSetEqu( int * pMap, int i, int iVar )               { assert( -1 == pMap[i*KSAT_SPACE+2] ); pMap[i*KSAT_SPACE+2] = iVar; }
void Gia_KSatSetRef( int * pMap, int i, int iVar )               { assert( -1 == pMap[i*KSAT_SPACE+3] ); pMap[i*KSAT_SPACE+3] = iVar; }
void Gia_KSatSetFan( int * pMap, int i, int f, int k, int iVar ) { assert( -1 == pMap[i*KSAT_SPACE+4+f*KSAT_OBJS+k] );     pMap[i*KSAT_SPACE+4+f*KSAT_OBJS+k]     = iVar; }  
void Gia_KSatSetMin( int * pMap, int i, int m, int k, int iVar ) { assert( -1 == pMap[i*KSAT_SPACE+4+3*KSAT_OBJS+3*m+k] ); pMap[i*KSAT_SPACE+4+3*KSAT_OBJS+3*m+k] = iVar; } 

int Gia_KSatValInv( int * pMap, int i, Vec_Int_t * vRes )               { assert( -1 != pMap[i*KSAT_SPACE+0] ); return Vec_IntEntry( vRes, pMap[i*KSAT_SPACE+0] ); }
int Gia_KSatValAnd( int * pMap, int i, Vec_Int_t * vRes )               { assert( -1 != pMap[i*KSAT_SPACE+1] ); return Vec_IntEntry( vRes, pMap[i*KSAT_SPACE+1] ); }
int Gia_KSatValEqu( int * pMap, int i, Vec_Int_t * vRes )               { assert( -1 != pMap[i*KSAT_SPACE+2] ); return Vec_IntEntry( vRes, pMap[i*KSAT_SPACE+2] ); }
int Gia_KSatValRef( int * pMap, int i, Vec_Int_t * vRes )               { assert( -1 != pMap[i*KSAT_SPACE+3] ); return Vec_IntEntry( vRes, pMap[i*KSAT_SPACE+3] ); }
int Gia_KSatValFan( int * pMap, int i, int f, int k, Vec_Int_t * vRes ) { assert( -1 != pMap[i*KSAT_SPACE+4+f*KSAT_OBJS+k] );     return Vec_IntEntry( vRes, pMap[i*KSAT_SPACE+4+f*KSAT_OBJS+k]     ); } 
int Gia_KSatValMin( int * pMap, int i, int m, int k, Vec_Int_t * vRes ) { assert( -1 != pMap[i*KSAT_SPACE+4+3*KSAT_OBJS+3*m+k] ); return Vec_IntEntry( vRes, pMap[i*KSAT_SPACE+4+3*KSAT_OBJS+3*m+k] ); }

int * Gia_KSatMapInit( int nIns, int nNodes, word Truth, int * pnVars )
{
    assert( nIns + nNodes <= KSAT_OBJS );
    assert( (1 << nIns) <= KSAT_MINTS );
    int n, m, f, k, nVars = 2, * pMap = ABC_FALLOC( int, KSAT_OBJS*KSAT_SPACE );
    for ( n = nIns; n < nIns+nNodes; n++ ) {
        Gia_KSatSetInv(pMap, n, nVars++);
        Gia_KSatSetAnd(pMap, n, nVars++);
        Gia_KSatSetEqu(pMap, n, nVars++);
        Gia_KSatSetRef(pMap, n, nVars++);
    }
    for ( n = nIns; n < nIns+nNodes; n++ ) {
        for ( f = 0; f < 2; f++ )
        for ( k = 0; k < n; k++ )
            Gia_KSatSetFan(pMap, n, f, k, nVars++);
        for ( k = n+1; k < nIns+nNodes; k++ )
            Gia_KSatSetFan(pMap, n, 2, k, nVars++);
    }
    for ( m = 0; m < (1<<nIns); m++ ) {
        for ( n = 0; n < nIns; n++ )
            Gia_KSatSetMin(pMap, n, m, 0, (m >> n) & 1 );
        Gia_KSatSetMin(pMap, nIns+nNodes-1, m, 0, (Truth >> m) & 1 );        
        for ( n = nIns; n < nIns+nNodes; n++ ) 
        for ( k = 0; k < 3; k++ )
            if ( k || n < nIns+nNodes-1 )
                Gia_KSatSetMin(pMap, n, m, k, nVars++);

    }
    if ( pnVars ) *pnVars = nVars;
    return pMap;
}

int Gia_KSatFindFan( int * pMap, int i, int f, Vec_Int_t * vRes ) 
{ 
    assert( f < 2 );
    for ( int k = 0; k < i; k++ )
        if ( Gia_KSatValFan( pMap, i, f, k, vRes ) )
            return k;
    assert( 0 );
    return -1;
}

Vec_Int_t * Gia_ManKSatGenLevels( char * pGuide, int nIns, int nNodes )
{
    Vec_Int_t * vRes;
    int i, k, Count = 0;
    for ( i = 0; pGuide[i]; i++ )
        Count += pGuide[i] - '0';
    if ( Count != nNodes ) {
        printf( "Guidance %s has %d nodes while the problem has %d nodes.\n", pGuide, Count, nNodes );
        return NULL;
    }
    int FirstPrev = 0;
    int FirstThis = nIns;
    int FirstNext = FirstThis;
    vRes = Vec_IntStartFull( 2*nIns );
    for ( i = 0; pGuide[i]; i++ ) {
        FirstNext += pGuide[i] - '0';
        for ( k = FirstThis; k < FirstNext; k++ )
            Vec_IntPushTwo( vRes, FirstPrev, FirstThis );
        FirstPrev = FirstThis;
        FirstThis = FirstNext;
    }
    assert( Vec_IntSize(vRes) == 2*(nIns + nNodes) );
    Count = 0;
    //int Start, Stop;
    //Vec_IntForEachEntryDouble(vRes, Start, Stop, i)
    //    printf( "%2d : Start %2d  Stop %2d\n", Count++, Start, Stop );
    return vRes;
}

Vec_Str_t * Gia_ManKSatCnf( int * pMap, int nIns, int nNodes, int nBound, int fMultiLevel, char * pGuide )
{
    Vec_Str_t * vStr = Vec_StrAlloc( 10000 );
    Vec_Int_t * vRes = pGuide ? Gia_ManKSatGenLevels( pGuide, nIns, nNodes ) : NULL;
    int i, j, m, n, f, c, a, Start, Stop, nLits = 0, pLits[256] = {0};
    Gia_SatDumpLiteral( vStr, 1 );
    Gia_SatDumpLiteral( vStr, 2 );    
    if ( vRes ) {
        n = nIns;
        Vec_IntForEachEntryDoubleStart( vRes, Start, Stop, i, 2*nIns ) {
            for ( j = 0; j < Start; j++ )
                Gia_SatDumpLiteral( vStr, Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 1, j), 1 ) ); 
            for ( f = 0; f < 2; f++ )
            for ( j = Stop; j < n; j++ )
                Gia_SatDumpLiteral( vStr, Abc_Var2Lit( Gia_KSatVarFan(pMap, n, f, j), 1 ) );
            n++;
        }
        assert( n == nIns + nNodes );
    }
    // fanins are connected once
    for ( n = nIns; n < nIns+nNodes; n++ )
    for ( f = 0; f < 2; f++ ) {

        nLits = 0;
        for ( i = 0; i < n; i++ )
            pLits[nLits++] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, f, i), 0 );
        Gia_SatDumpClause( vStr, pLits, nLits );
/*        
        for ( i = 0; i < n; i++ )
        for ( j = 0; j < i; j++ ) {
            pLits[0] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, f, i), 1 );
            pLits[1] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, f, j), 1 );
            Gia_SatDumpClause(  vStr, pLits, 2 );
        }
*/        
        Vec_StrPrintF( vStr, "k %d ", n-1 );
        for ( i = 0; i < n; i++ )
            pLits[i] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, f, i), 1 );
        Gia_SatDumpClause( vStr, pLits, n );
    }
    for ( n = nIns; n < nIns+nNodes; n++ ) {
        // fanins are equal
        for ( i = 0; i < n; i++ ) {
            pLits[0] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 0, i), 1 );
            pLits[1] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 1, i), 1 );
            pLits[2] = Abc_Var2Lit( Gia_KSatVarEqu(pMap, n), 0 );
            Gia_SatDumpClause( vStr, pLits, 3 );
        }
        for ( i = 0; i < n; i++ )
        for ( j = i+1; j < n; j++ ) {
            pLits[0] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 0, i), 1 );
            pLits[1] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 1, j), 1 );
            pLits[2] = Abc_Var2Lit( Gia_KSatVarEqu(pMap, n), 1 );
            Gia_SatDumpClause( vStr, pLits, 3 );
        }
        // if fanins are equal, inv is used
        pLits[0] = Abc_Var2Lit( Gia_KSatVarEqu(pMap, n), 1 );
        pLits[1] = Abc_Var2Lit( Gia_KSatVarInv(pMap, n), 0 );
        Gia_SatDumpClause( vStr, pLits, 2 );
        // fanin ordering 
        for ( i = 0; i < n; i++ )
        for ( j = 0; j < i; j++ ) {
            pLits[0] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 0, i), 1 );
            pLits[1] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 1, j), 1 );
            Gia_SatDumpClause( vStr, pLits, 2 );
        }
    }    
    for ( n = nIns; n < nIns+nNodes-1; n++ ) {
        // there is a fanout to the node above
        for ( i = n+1; i < nIns+nNodes; i++ ) {
            for ( f = 0; f < 2; f++ ) {
                pLits[0] = Abc_Var2Lit( Gia_KSatVarFan(pMap, i, f, n), 1 );
                pLits[1] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 2, i), 0 );
                Gia_SatDumpClause( vStr, pLits, 2 );            
            }
            pLits[0] = Abc_Var2Lit( Gia_KSatVarFan(pMap, i, 0, n), 0 );
            pLits[1] = Abc_Var2Lit( Gia_KSatVarFan(pMap, i, 1, n), 0 );
            pLits[2] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 2, i), 1 );
            Gia_SatDumpClause( vStr, pLits, 3 );
        }
        // there is at least one fanout, except the last one
        nLits = 0;
        for ( i = n+1; i < nIns+nNodes; i++ )
            pLits[nLits++] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 2, i), 0 );
        assert( nLits > 0 );
        Gia_SatDumpClause( vStr, pLits, nLits );
    }
    // there is more than one fanout, except the last one
    for ( n = nIns; n < nIns+nNodes-1; n++ ) {
        for ( i = n+1; i < nIns+nNodes; i++ )
        for ( j = i+1; j < nIns+nNodes; j++ ) {
            pLits[0] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 2, i), 1 );
            pLits[1] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 2, j), 1 );
            pLits[2] = Abc_Var2Lit( Gia_KSatVarRef(pMap, n), 0 );
            Gia_SatDumpClause( vStr, pLits, 3 );               
        }
        // if more than one fanout, inv is used
        pLits[0] = Abc_Var2Lit( Gia_KSatVarRef(pMap, n), 1 );
        pLits[1] = Abc_Var2Lit( Gia_KSatVarInv(pMap, n), 0 );
        Gia_SatDumpClause( vStr, pLits, 2 );
        // if inv is not used, its fanins' invs are used
        if ( !fMultiLevel ) {
            pLits[0] = Abc_Var2Lit( Gia_KSatVarInv(pMap, n), 0 );
            for ( i = nIns; i < n; i++ ) 
            for ( f = 0; f < 2; f++ ) {
                pLits[1] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, f, i), 1 );
                pLits[2] = Abc_Var2Lit( Gia_KSatVarInv(pMap, i), 0 );
                Gia_SatDumpClause( vStr, pLits, 3 );
            }
        }
    }
    // the last one always uses inverter
    Gia_SatDumpLiteral( vStr, Abc_Var2Lit( Gia_KSatVarInv(pMap, nIns+nNodes-1), 0 ) );  
/*    
    // for each minterm, for each pair of possible fanins, the node's output is determined using and/or and inv (4*N*N*M)
    for ( m = 0; m < (1 << nIns); m++ )
    for ( n = nIns; n < nIns+nNodes; n++ ) 
    for ( c = 0; c < 2; c++ )
    for ( a = 0; a < 2; a++ ) {        
        // implications: Fan(f) & Mint(m) & !And & !Inv -> Val1
        for ( f = 0; f < 2; f++ ) 
        for ( i = 0; i < n; i++ ) {
            pLits[0] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, f, i), 1 );
            pLits[1] = Abc_Var2Lit( Gia_KSatVarMin(pMap, i, m, 0), !a );
            pLits[2] = Abc_Var2Lit( Gia_KSatVarAnd(pMap, n), a );
            pLits[3] = Abc_Var2Lit( Gia_KSatVarInv(pMap, n), c );
            pLits[4] = Abc_Var2Lit( Gia_KSatVarMin(pMap, n, m, 0), a^c );
            Gia_SatDumpClause( vStr, pLits, 5 );
        }
        // large clauses: Fan(0) & Fan(1) & !Mint(m) & !Mint(m) & !And & !Inv -> Val0
        for ( i = 0; i < n; i++ ) 
        for ( j = i; j < n; j++ ) {
            pLits[0] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 0, i), 1 );
            pLits[1] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, 1, j), 1 );
            pLits[2] = Abc_Var2Lit( Gia_KSatVarMin(pMap, i, m, 0), a );
            pLits[3] = Abc_Var2Lit( Gia_KSatVarMin(pMap, j, m, 0), a );
            pLits[4] = Abc_Var2Lit( Gia_KSatVarAnd(pMap, n), a );
            pLits[5] = Abc_Var2Lit( Gia_KSatVarInv(pMap, n), c );
            pLits[6] = Abc_Var2Lit( Gia_KSatVarMin(pMap, n, m, 0), a==c );
            Gia_SatDumpClause( vStr, pLits, 7 );
        }
    }
*/
    // for each minterm, define a fanin variable and use it to get the node's output based on and/or and inv (4*N*N*M)
    for ( m = 0; m < (1 << nIns); m++ )
    for ( n = nIns; n < nIns+nNodes; n++ ) {
        for ( i = 0; i < n; i++ )
        for ( f = 0; f < 2; f++ )
        for ( c = 0; c < 2; c++ ) {
            pLits[0] = Abc_Var2Lit( Gia_KSatVarFan(pMap, n, f, i), 1 );
            pLits[1] = Abc_Var2Lit( Gia_KSatVarMin(pMap, i, m, 0), c );
            pLits[2] = Abc_Var2Lit( Gia_KSatVarMin(pMap, n, m, 1+f), !c );
            Gia_SatDumpClause( vStr, pLits, 3 );
        }
        for ( c = 0; c < 2; c++ )
        for ( a = 0; a < 2; a++ ) {        
            // implications: Mint(m,f) & !And & !Inv -> Val1
            for ( f = 0; f < 2; f++ ) {
                pLits[0] = Abc_Var2Lit( Gia_KSatVarMin(pMap, n, m, 1+f), !a );
                pLits[1] = Abc_Var2Lit( Gia_KSatVarAnd(pMap, n), a );
                pLits[2] = Abc_Var2Lit( Gia_KSatVarInv(pMap, n), c );
                pLits[3] = Abc_Var2Lit( Gia_KSatVarMin(pMap, n, m, 0), a^c );
                Gia_SatDumpClause( vStr, pLits, 4 );
            }
            // large clauses: !Mint(m,0) & !Mint(m,1) & !And & !Inv -> Val0
            pLits[0] = Abc_Var2Lit( Gia_KSatVarMin(pMap, n, m, 1), a );
            pLits[1] = Abc_Var2Lit( Gia_KSatVarMin(pMap, n, m, 2), a );
            pLits[2] = Abc_Var2Lit( Gia_KSatVarAnd(pMap, n), a );
            pLits[3] = Abc_Var2Lit( Gia_KSatVarInv(pMap, n), c );
            pLits[4] = Abc_Var2Lit( Gia_KSatVarMin(pMap, n, m, 0), a==c );
            Gia_SatDumpClause( vStr, pLits, 5 );
        }
    }
    // the number of nodes with duplicated fanins and without inv is maximized
    if ( nBound && 2*nNodes > nBound ) {
        Vec_StrPrintF( vStr, "k %d ", 2*nNodes-nBound );
        nLits = 0;        
        for ( n = nIns; n < nIns+nNodes; n++ ) {
            pLits[nLits++] = Abc_Var2Lit(Gia_KSatVarEqu(pMap, n), 0);
            pLits[nLits++] = Abc_Var2Lit(Gia_KSatVarInv(pMap, n), 1);
        }
        Gia_SatDumpClause( vStr, pLits, nLits );
    }
    Vec_StrPush( vStr, '\0' );
    Vec_IntFreeP( &vRes );
    return vStr;    
}


typedef enum { 
    GIA_GATE2_ZERO,     // 0: 
    GIA_GATE2_ONE,      // 1: 
    GIA_GATE2_BUF,      // 2: 
    GIA_GATE2_INV,      // 3: 
    GIA_GATE2_NAN2,     // 4: 
    GIA_GATE2_NOR2,     // 5: 
    GIA_GATE2_AOI21,    // 6: 
    GIA_GATE2_NAN3,     // 7: 
    GIA_GATE2_NOR3,     // 8: 
    GIA_GATE2_OAI21,    // 9: 
    GIA_GATE2_NOR4,     // 10: 
    GIA_GATE2_AOI211,   // 11: 
    GIA_GATE2_AOI22,    // 12: 
    GIA_GATE2_NAN4,     // 13: 
    GIA_GATE2_OAI211,   // 14: 
    GIA_GATE2_OAI22,    // 15: 
    GIA_GATE2_VOID      // 16: unused value
} Gia_ManGate2_t;

Vec_Int_t * Gia_ManDeriveKSatMappingArray( Gia_Man_t * p, Vec_Int_t * vRes )
{
    Vec_Int_t * vMapping = Vec_IntStart( 2*Gia_ManObjNum(p) );
    int i, Id; Gia_Obj_t * pObj;
    Gia_ManForEachCiId( p, Id, i )
        if ( Vec_IntEntry(vRes, Id) )
            Vec_IntWriteEntry( vMapping, Abc_Var2Lit(Id, 1), -1 );
    Gia_ManForEachAnd( p, pObj, i ) {
        assert( Vec_IntEntry(vRes, i) > 0 );
        if ( (Vec_IntEntry(vRes, i) & 2) == 0 ) {
            assert( (Vec_IntEntry(vRes, i) & 1) == 0 );
            continue;
        }
        Gia_Obj_t * pFans[2] = { Gia_ObjFanin0(pObj), Gia_ObjFanin1(pObj) };
        int fComp = ((Vec_IntEntry(vRes, i) >> 2) & 1) != 0;
        int fFan0 = ((Vec_IntEntry(vRes, Gia_ObjFaninId0(pObj, i)) >> 1) & 1) == 0 && Gia_ObjIsAnd(pFans[0]);
        int fFan1 = ((Vec_IntEntry(vRes, Gia_ObjFaninId1(pObj, i)) >> 1) & 1) == 0 && Gia_ObjIsAnd(pFans[1]);
        if ( Vec_IntEntry(vRes, i) & 1 )
            Vec_IntWriteEntry( vMapping, Abc_Var2Lit(i, !fComp), -1 );
        Vec_IntWriteEntry( vMapping, Abc_Var2Lit(i, fComp), Vec_IntSize(vMapping) );
        if ( fFan0 && fFan1 ) {
            Vec_IntPush( vMapping, 4 );
            if ( !Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) ) {
                Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pFans[1]), !(fComp ^ Gia_ObjFaninC1(pObj) ^ Gia_ObjFaninC0(pFans[1]))) );
                Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pFans[1]), !(fComp ^ Gia_ObjFaninC1(pObj) ^ Gia_ObjFaninC1(pFans[1]))) );
                Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pFans[0]), !(fComp ^ Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC0(pFans[0]))) );
                Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pFans[0]), !(fComp ^ Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC1(pFans[0]))) );
            }
            else {
                Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pFans[0]), !(fComp ^ Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC0(pFans[0]))) );
                Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pFans[0]), !(fComp ^ Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC1(pFans[0]))) );
                Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pFans[1]), !(fComp ^ Gia_ObjFaninC1(pObj) ^ Gia_ObjFaninC0(pFans[1]))) );
                Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pFans[1]), !(fComp ^ Gia_ObjFaninC1(pObj) ^ Gia_ObjFaninC1(pFans[1]))) );
            }
            if ( Gia_ObjFaninC0(pObj) && Gia_ObjFaninC1(pObj) ) 
                Vec_IntPush( vMapping, fComp ? GIA_GATE2_OAI22 : GIA_GATE2_AOI22 );
            else if ( !Gia_ObjFaninC0(pObj) && !Gia_ObjFaninC1(pObj) )
                Vec_IntPush( vMapping, fComp ? GIA_GATE2_NAN4 : GIA_GATE2_NOR4 );
            else 
                Vec_IntPush( vMapping, fComp ? GIA_GATE2_OAI211 : GIA_GATE2_AOI211 );
        } else if ( fFan0 ) {
            Vec_IntPush( vMapping, 3 );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pFans[0]), !(fComp ^ Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC0(pFans[0]))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pFans[0]), !(fComp ^ Gia_ObjFaninC0(pObj) ^ Gia_ObjFaninC1(pFans[0]))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pObj),     !(fComp ^ Gia_ObjFaninC1(pObj))) );
            if ( Gia_ObjFaninC0(pObj) )
                Vec_IntPush( vMapping, fComp ? GIA_GATE2_OAI21 : GIA_GATE2_AOI21 );
            else
                Vec_IntPush( vMapping, fComp ? GIA_GATE2_NAN3 : GIA_GATE2_NOR3 );
        } else if ( fFan1 ) {
            Vec_IntPush( vMapping, 3 );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pFans[1]), !(fComp ^ Gia_ObjFaninC1(pObj) ^ Gia_ObjFaninC0(pFans[1]))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pFans[1]), !(fComp ^ Gia_ObjFaninC1(pObj) ^ Gia_ObjFaninC1(pFans[1]))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pObj),     !(fComp ^ Gia_ObjFaninC0(pObj))) );
            if ( Gia_ObjFaninC1(pObj) )
                Vec_IntPush( vMapping, fComp ? GIA_GATE2_OAI21 : GIA_GATE2_AOI21 );
            else
                Vec_IntPush( vMapping, fComp ? GIA_GATE2_NAN3 : GIA_GATE2_NOR3 );
        } else {
            Vec_IntPush( vMapping, 2 );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId0p(p, pObj),     !(fComp ^ Gia_ObjFaninC0(pObj))) );
            Vec_IntPush( vMapping, Abc_Var2Lit(Gia_ObjFaninId1p(p, pObj),     !(fComp ^ Gia_ObjFaninC1(pObj))) );
            Vec_IntPush( vMapping, fComp ? GIA_GATE2_NAN2 : GIA_GATE2_NOR2 );
        }
    }
    return vMapping;
}

Gia_Man_t * Gia_ManDeriveKSatMapping( Vec_Int_t * vRes, int * pMap, int nIns, int nNodes, int fVerbose )
{
    Vec_Int_t * vGuide = Vec_IntStart( 1000 );
    Gia_Man_t * pNew = Gia_ManStart( nIns + nNodes + 2 );
    pNew->pName = Abc_UtilStrsav( "test" );
    int i, nSave = 0, pCopy[256] = {0};
    for ( i = 1; i <= nIns; i++ )
        pCopy[i] = Gia_ManAppendCi( pNew );
    for ( i = nIns; i < nIns+nNodes; i++ ) {
        int iFan0 = Gia_KSatFindFan( pMap, i, 0, vRes );
        int iFan1 = Gia_KSatFindFan( pMap, i, 1, vRes );
        if ( iFan0 == iFan1 )
            pCopy[i+1] = pCopy[iFan0+1];
        else if ( Gia_KSatValAnd(pMap, i, vRes) )
            pCopy[i+1] = Gia_ManAppendAnd( pNew, pCopy[iFan0+1], pCopy[iFan1+1] );
        else
            pCopy[i+1] = Gia_ManAppendOr( pNew, pCopy[iFan0+1], pCopy[iFan1+1] );
        pCopy[i+1] = Abc_LitNotCond( pCopy[i+1], Gia_KSatValInv(pMap, i, vRes) );
        if ( iFan0 == iFan1 )
            *Vec_IntEntryP(vGuide, Abc_Lit2Var(pCopy[i+1])) ^= 1;
        else if ( Gia_KSatValAnd(pMap, i, vRes) ) 
            *Vec_IntEntryP(vGuide, Abc_Lit2Var(pCopy[i+1])) ^= 4 | (2*Gia_KSatValInv(pMap, i, vRes));
        else 
            *Vec_IntEntryP(vGuide, Abc_Lit2Var(pCopy[i+1])) ^= 8 | (2*Gia_KSatValInv(pMap, i, vRes));
        if ( fVerbose ) {
            if ( i == nIns+nNodes-1 )
                printf( " F = " );
            else
                printf( "%2d = ", i );
            if ( iFan0 == iFan1 )
                printf( "INV( %d )\n", iFan0 );
            else if ( Gia_KSatValAnd(pMap, i, vRes) ) 
                printf( "%sAND( %d, %d )\n", Gia_KSatValInv(pMap, i, vRes) ? "N":"", iFan0, iFan1 );
            else 
                printf( "%sOR( %d, %d )\n", Gia_KSatValInv(pMap, i, vRes) ? "N":"", iFan0, iFan1 );
            nSave += (iFan0 == iFan1) || !Gia_KSatValInv(pMap, i, vRes);
            if ( i == nIns+nNodes-1 )
                printf( "Solution cost = %d\n", 2*(2*nNodes - nSave) );
        }
    }
    Gia_ManAppendCo( pNew, pCopy[nIns+nNodes] );
    //pNew->vCellMapping = Gia_ManDeriveKSatMappingArray( pNew, vGuide );
    Vec_IntFree( vGuide );
    return pNew;
}

word Gia_ManGetTruth( Gia_Man_t * p )
{
    Gia_Obj_t * pObj; int i, Id; 
    word pFuncs[256] = {0}, Const[2] = {0, ~(word)0};
    assert( Gia_ManObjNum(p) <= 256 );
    Gia_ManForEachCiId( p, Id, i )
        pFuncs[Id] = s_Truths6[i];
    Gia_ManForEachAnd( p, pObj, i )
        pFuncs[i] = (Const[Gia_ObjFaninC0(pObj)] ^ pFuncs[Gia_ObjFaninId0(pObj, i)]) & (Const[Gia_ObjFaninC1(pObj)] ^ pFuncs[Gia_ObjFaninId1(pObj, i)]);
    pObj = Gia_ManCo(p, 0);
    return Const[Gia_ObjFaninC0(pObj)] ^ pFuncs[Gia_ObjFaninId0p(p, pObj)];
}

Gia_Man_t * Gia_ManKSatMapping( word Truth, int nIns, int nNodes, int nBound, int Seed, int fMultiLevel, int nBTLimit, int nTimeout, int fVerbose, int fKeepFile, int argc, char ** argv, char * pGuide )
{
    abctime clkStart = Abc_Clock();
    Gia_Man_t * pNew = NULL;
    srand(time(NULL));
    int Status, Rand = ((((unsigned)rand()) << 12) ^ ((unsigned)rand())) & 0xFFFFFF;
    char pFileNameI[32]; sprintf( pFileNameI, "_%06x_.cnf", Rand ); 
    char pFileNameO[32]; sprintf( pFileNameO, "_%06x_.out", Rand ); 
    int nVars = 0, * pMap = Gia_KSatMapInit( nIns, nNodes, Truth, &nVars );
    Vec_Str_t * vStr = Gia_ManKSatCnf( pMap, nIns, nNodes, nBound/2, fMultiLevel, pGuide );
    if ( !Gia_ManDumpCnf(pFileNameI, vStr, nVars) ) {
        Vec_StrFree( vStr );
        return NULL;
    }
    if ( fVerbose )
        printf( "Vars = %d. Nodes = %d. Cardinality bound = %d.  SAT vars = %d. SAT clauses = %d. Conflict limit = %d. Timeout = %d.\n", 
            nIns, nNodes, nBound,  nVars, Vec_StrCountEntry(vStr, '\n'), nBTLimit, nTimeout );
    Vec_Int_t * vRes = Gia_RunKadical( pFileNameI, pFileNameO, Seed, nBTLimit, nTimeout, 1, &Status );
    unlink( pFileNameI );
    //unlink( pFileNameO );
    if ( fKeepFile ) Gia_ManDumpCnf2( vStr, nVars, argc, argv, Abc_Clock() - clkStart, Status );
    Vec_StrFree( vStr );
    if ( vRes == NULL )
        return 0;
    Vec_IntDrop( vRes, 0 );
    pNew = Gia_ManDeriveKSatMapping( vRes, pMap, nIns, nNodes, fVerbose );
    printf( "Verification %s.  ", Truth == Gia_ManGetTruth(pNew) ? "passed" : "failed" );
    Abc_PrintTime( 0, "Total time", Abc_Clock() - clkStart );
    Vec_IntFree( vRes );
    ABC_FREE( pMap );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

