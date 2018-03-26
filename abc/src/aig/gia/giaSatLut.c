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
#include "sat/bsat/satStore.h"
#include "misc/util/utilNam.h"
#include "map/scl/sclCon.h"
#include "misc/vec/vecHsh.h"

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

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

