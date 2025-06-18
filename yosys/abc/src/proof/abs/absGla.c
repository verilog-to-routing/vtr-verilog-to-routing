/**CFile****************************************************************

  FileName    [absGla2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Abstraction package.]

  Synopsis    [Scalable gate-level abstraction.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: absGla2.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/main/main.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver2.h"
#include "bool/kit/kit.h"
#include "abs.h"
#include "absRef.h"
//#include "absRef2.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define GA2_BIG_NUM 0x3FFFFFF0

typedef struct Ga2_Man_t_ Ga2_Man_t; // manager
struct Ga2_Man_t_
{
    // user data
    Gia_Man_t *    pGia;         // working AIG manager
    Abs_Par_t *    pPars;        // parameters
    // markings 
    Vec_Ptr_t *    vCnfs;        // for each object: CNF0, CNF1
    // abstraction
    Vec_Int_t *    vIds;         // abstraction ID for each GIA object
    Vec_Int_t *    vProofIds;    // mapping of GIA objects into their proof IDs
    Vec_Int_t *    vAbs;         // array of abstracted objects
    Vec_Int_t *    vValues;      // array of objects with abstraction ID assigned
    int            nProofIds;    // the counter of proof IDs
    int            LimAbs;       // limit value for starting abstraction objects
    int            LimPpi;       // limit value for starting PPI objects
    int            nMarked;      // total number of marked nodes and flops
    int            fUseNewLine;  // remember that you used new line
    // refinement
    Rnm_Man_t *    pRnm;         // refinement manager
//    Rf2_Man_t *    pRf2;         // refinement manager
    // SAT solver and variables
    Vec_Ptr_t *    vId2Lit;      // mapping, for each timeframe, of object ID into SAT literal
    sat_solver2 *  pSat;         // incremental SAT solver
    int            nSatVars;     // the number of SAT variables
    int            nCexes;       // the number of counter-examples
    int            nObjAdded;    // objs added during refinement
    int            nPdrCalls;    // count the number of concurrent calls
    // hash table
    int *          pTable;
    int            nTable;
    int            nHashHit;
    int            nHashMiss;
    int            nHashOver;
    // temporaries
    Vec_Int_t *    vLits;
    Vec_Int_t *    vIsopMem;
    char * pSopSizes, ** pSops;  // CNF representation
    // statistics  
    abctime        timeStart;
    abctime        timeInit;
    abctime        timeSat;
    abctime        timeUnsat;
    abctime        timeCex;
    abctime        timeOther;
};

static inline int         Ga2_ObjId( Ga2_Man_t * p, Gia_Obj_t * pObj )           { return Vec_IntEntry(p->vIds, Gia_ObjId(p->pGia, pObj));                                                  }
static inline void        Ga2_ObjSetId( Ga2_Man_t * p, Gia_Obj_t * pObj, int i ) { Vec_IntWriteEntry(p->vIds, Gia_ObjId(p->pGia, pObj), i);                                                 }

static inline Vec_Int_t * Ga2_ObjCnf0( Ga2_Man_t * p, Gia_Obj_t * pObj )         { assert(Ga2_ObjId(p,pObj) >= 0); return (Vec_Int_t *)Vec_PtrEntry( p->vCnfs, 2*Ga2_ObjId(p,pObj)   );                  }
static inline Vec_Int_t * Ga2_ObjCnf1( Ga2_Man_t * p, Gia_Obj_t * pObj )         { assert(Ga2_ObjId(p,pObj) >= 0); return (Vec_Int_t *)Vec_PtrEntry( p->vCnfs, 2*Ga2_ObjId(p,pObj)+1 );                  }

static inline int         Ga2_ObjIsAbs0( Ga2_Man_t * p, Gia_Obj_t * pObj )       { assert(Ga2_ObjId(p,pObj) >= 0); return Ga2_ObjId(p,pObj) >= 0         && Ga2_ObjId(p,pObj) < p->LimAbs;  }
static inline int         Ga2_ObjIsLeaf0( Ga2_Man_t * p, Gia_Obj_t * pObj )      { assert(Ga2_ObjId(p,pObj) >= 0); return Ga2_ObjId(p,pObj) >= p->LimAbs && Ga2_ObjId(p,pObj) < p->LimPpi;  }
static inline int         Ga2_ObjIsAbs( Ga2_Man_t * p, Gia_Obj_t * pObj )        { return Ga2_ObjId(p,pObj) >= 0 &&  Ga2_ObjCnf0(p,pObj);                                                   }
static inline int         Ga2_ObjIsLeaf( Ga2_Man_t * p, Gia_Obj_t * pObj )       { return Ga2_ObjId(p,pObj) >= 0 && !Ga2_ObjCnf0(p,pObj);                                                   }

static inline Vec_Int_t * Ga2_MapFrameMap( Ga2_Man_t * p, int f )                { return (Vec_Int_t *)Vec_PtrEntry( p->vId2Lit, f );                                                       }

// returns literal of this object, or -1 if SAT variable of the object is not assigned
static inline int Ga2_ObjFindLit( Ga2_Man_t * p, Gia_Obj_t * pObj, int f )  
{ 
//    int Id = Ga2_ObjId(p,pObj);
    assert( Ga2_ObjId(p,pObj) >= 0 && Ga2_ObjId(p,pObj) < Vec_IntSize(p->vValues) );
    return Vec_IntEntry( Ga2_MapFrameMap(p, f), Ga2_ObjId(p,pObj) );
}
// inserts literal of this object
static inline void Ga2_ObjAddLit( Ga2_Man_t * p, Gia_Obj_t * pObj, int f, int Lit )  
{ 
//    assert( Lit > 1 );
    assert( Ga2_ObjFindLit(p, pObj, f) == -1 );
    Vec_IntSetEntry( Ga2_MapFrameMap(p, f), Ga2_ObjId(p,pObj), Lit );
}
// returns or inserts-and-returns literal of this object
static inline int Ga2_ObjFindOrAddLit( Ga2_Man_t * p, Gia_Obj_t * pObj, int f )  
{ 
    int Lit = Ga2_ObjFindLit( p, pObj, f );
    if ( Lit == -1 )
    {
        Lit = toLitCond( p->nSatVars++, 0 );
        Ga2_ObjAddLit( p, pObj, f, Lit );
    }
//    assert( Lit > 1 );
    return Lit;
}


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes truth table for the marked node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Ga2_ObjComputeTruth_rec( Gia_Man_t * p, Gia_Obj_t * pObj, int fFirst )
{
    unsigned Val0, Val1;
    if ( pObj->fPhase && !fFirst )
        return pObj->Value;
    assert( Gia_ObjIsAnd(pObj) );
    Val0 = Ga2_ObjComputeTruth_rec( p, Gia_ObjFanin0(pObj), 0 );
    Val1 = Ga2_ObjComputeTruth_rec( p, Gia_ObjFanin1(pObj), 0 );
    return (Gia_ObjFaninC0(pObj) ? ~Val0 : Val0) & (Gia_ObjFaninC1(pObj) ? ~Val1 : Val1);
}
unsigned Ga2_ManComputeTruth( Gia_Man_t * p, Gia_Obj_t * pRoot, Vec_Int_t * vLeaves )
{
    static unsigned uTruth5[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    Gia_Obj_t * pObj;
    unsigned Res;
    int i;
    Gia_ManForEachObjVec( vLeaves, p, pObj, i )
        pObj->Value = uTruth5[i];
    Res = Ga2_ObjComputeTruth_rec( p, pRoot, 1 );
    Gia_ManForEachObjVec( vLeaves, p, pObj, i )
        pObj->Value = 0;
    return Res;
}

/**Function*************************************************************

  Synopsis    [Returns AIG marked for CNF generation.]

  Description [The marking satisfies the following requirements:
  Each marked node has the number of marked fanins no more than N.]
               
  SideEffects [Uses pObj->fPhase to store the markings.]

  SeeAlso     []

***********************************************************************/
int Ga2_ManBreakTree_rec( Gia_Man_t * p, Gia_Obj_t * pObj, int fFirst, int N )
{   // breaks a tree rooted at the node into N-feasible subtrees
    int Val0, Val1;
    if ( pObj->fPhase && !fFirst )
        return 1;
    Val0 = Ga2_ManBreakTree_rec( p, Gia_ObjFanin0(pObj), 0, N );
    Val1 = Ga2_ManBreakTree_rec( p, Gia_ObjFanin1(pObj), 0, N );
    if ( Val0 + Val1 < N )
        return Val0 + Val1;
    if ( Val0 + Val1 == N )
    {
        pObj->fPhase = 1;
        return 1;
    }
    assert( Val0 + Val1 > N );
    assert( Val0 < N && Val1 < N );
    if ( Val0 >= Val1 )
    {
        Gia_ObjFanin0(pObj)->fPhase = 1;
        Val0 = 1;
    }
    else 
    {
        Gia_ObjFanin1(pObj)->fPhase = 1;
        Val1 = 1;
    }
    if ( Val0 + Val1 < N )
        return Val0 + Val1;
    if ( Val0 + Val1 == N )
    {
        pObj->fPhase = 1;
        return 1;
    }
    assert( 0 );
    return -1;
}
int Ga2_ManCheckNodesAnd( Gia_Man_t * p, Vec_Int_t * vNodes )
{
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObjVec( vNodes, p, pObj, i )
        if ( (!Gia_ObjFanin0(pObj)->fPhase && Gia_ObjFaninC0(pObj)) || 
             (!Gia_ObjFanin1(pObj)->fPhase && Gia_ObjFaninC1(pObj)) )
            return 0;
    return 1;
}
void Ga2_ManCollectNodes_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vNodes, int fFirst )
{
    if ( pObj->fPhase && !fFirst )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Ga2_ManCollectNodes_rec( p, Gia_ObjFanin0(pObj), vNodes, 0 );
    Ga2_ManCollectNodes_rec( p, Gia_ObjFanin1(pObj), vNodes, 0 );
    Vec_IntPush( vNodes, Gia_ObjId(p, pObj) );

}
void Ga2_ManCollectLeaves_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vLeaves, int fFirst )
{
    if ( pObj->fPhase && !fFirst )
    {
        Vec_IntPushUnique( vLeaves, Gia_ObjId(p, pObj) );
        return;
    }
    assert( Gia_ObjIsAnd(pObj) );
    Ga2_ManCollectLeaves_rec( p, Gia_ObjFanin0(pObj), vLeaves, 0 );
    Ga2_ManCollectLeaves_rec( p, Gia_ObjFanin1(pObj), vLeaves, 0 );
}
int Ga2_ManMarkup( Gia_Man_t * p, int N, int fSimple )
{
    static unsigned uTruth5[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
//    abctime clk = Abc_Clock();
    Vec_Int_t * vLeaves;
    Gia_Obj_t * pObj;
    int i, k, Leaf, CountMarks;

    vLeaves = Vec_IntAlloc( 100 );

    if ( fSimple )
    {
        Gia_ManForEachObj( p, pObj, i )
            pObj->fPhase = !Gia_ObjIsCo(pObj);
    }
    else
    {
        // label nodes with multiple fanouts and inputs MUXes
        Gia_ManForEachObj( p, pObj, i )
        {
            pObj->Value = 0;
            if ( !Gia_ObjIsAnd(pObj) )
                continue;
            Gia_ObjFanin0(pObj)->Value++;
            Gia_ObjFanin1(pObj)->Value++;
            if ( !Gia_ObjIsMuxType(pObj) )
                continue;
            Gia_ObjFanin0(Gia_ObjFanin0(pObj))->Value++;
            Gia_ObjFanin1(Gia_ObjFanin0(pObj))->Value++;
            Gia_ObjFanin0(Gia_ObjFanin1(pObj))->Value++;
            Gia_ObjFanin1(Gia_ObjFanin1(pObj))->Value++;
        }
        Gia_ManForEachObj( p, pObj, i )
        {
            pObj->fPhase = 0;
            if ( Gia_ObjIsAnd(pObj) )
                pObj->fPhase = (pObj->Value > 1);
            else if ( Gia_ObjIsCo(pObj) )
                Gia_ObjFanin0(pObj)->fPhase = 1;
            else 
                pObj->fPhase = 1;
        } 
        // add marks when needed
        Gia_ManForEachAnd( p, pObj, i )
        {
            if ( !pObj->fPhase )
                continue;
            Vec_IntClear( vLeaves );
            Ga2_ManCollectLeaves_rec( p, pObj, vLeaves, 1 );
            if ( Vec_IntSize(vLeaves) > N )
                Ga2_ManBreakTree_rec( p, pObj, 1, N );
        }
    }

    // verify that the tree is split correctly
    Vec_IntFreeP( &p->vMapping );
    p->vMapping = Vec_IntStart( Gia_ManObjNum(p) );
    Gia_ManForEachRo( p, pObj, i )
    {
        Gia_Obj_t * pObjRi = Gia_ObjRoToRi(p, pObj);
        assert( pObj->fPhase );
        assert( Gia_ObjFanin0(pObjRi)->fPhase );
        // create map
        Vec_IntWriteEntry( p->vMapping, Gia_ObjId(p, pObj), Vec_IntSize(p->vMapping) );
        Vec_IntPush( p->vMapping, 1 );
        Vec_IntPush( p->vMapping, Gia_ObjFaninId0p(p, pObjRi) );
        Vec_IntPush( p->vMapping, Gia_ObjFaninC0(pObjRi) ? 0x55555555 : 0xAAAAAAAA );
        Vec_IntPush( p->vMapping, -1 );  // placeholder for ref counter
    }
    CountMarks = Gia_ManRegNum(p);
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !pObj->fPhase )
            continue;
        Vec_IntClear( vLeaves );
        Ga2_ManCollectLeaves_rec( p, pObj, vLeaves, 1 );
        assert( Vec_IntSize(vLeaves) <= N );
        // create map
        Vec_IntWriteEntry( p->vMapping, i, Vec_IntSize(p->vMapping) );
        Vec_IntPush( p->vMapping, Vec_IntSize(vLeaves) );
        Vec_IntForEachEntry( vLeaves, Leaf, k )
        {            
            Vec_IntPush( p->vMapping, Leaf );
            Gia_ManObj(p, Leaf)->Value = uTruth5[k];
            assert( Gia_ManObj(p, Leaf)->fPhase );
        }
        Vec_IntPush( p->vMapping,  (int)Ga2_ObjComputeTruth_rec( p, pObj, 1 ) );
        Vec_IntPush( p->vMapping, -1 );  // placeholder for ref counter
        CountMarks++;
    }
//    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Vec_IntFree( vLeaves );
    Gia_ManCleanValue( p );
    return CountMarks;
}
void Ga2_ManComputeTest( Gia_Man_t * p )
{
    abctime clk;
//    unsigned uTruth;
    Gia_Obj_t * pObj;
    int i, Counter = 0;
    clk = Abc_Clock();
    Ga2_ManMarkup( p, 5, 0 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !pObj->fPhase )
            continue;
//        uTruth = Ga2_ObjTruth( p, pObj );
//        printf( "%6d : ", Counter );
//        Kit_DsdPrintFromTruth( &uTruth, Ga2_ObjLeaveNum(p, pObj) ); 
//        printf( "\n" );
        Counter++;
    }
    Abc_Print( 1, "Marked AND nodes = %6d.  ", Counter );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ga2_Man_t * Ga2_ManStart( Gia_Man_t * pGia, Abs_Par_t * pPars )
{
    Ga2_Man_t * p;
    p = ABC_CALLOC( Ga2_Man_t, 1 );
    p->timeStart = Abc_Clock();
    p->fUseNewLine = 1;
    // user data
    p->pGia      = pGia;
    p->pPars     = pPars;
    // markings 
    p->nMarked   = Ga2_ManMarkup( pGia, 5, pPars->fUseSimple );
    p->vCnfs     = Vec_PtrAlloc( 1000 );
    Vec_PtrPush( p->vCnfs, Vec_IntAlloc(0) );
    Vec_PtrPush( p->vCnfs, Vec_IntAlloc(0) );
    // abstraction
    p->vIds      = Vec_IntStartFull( Gia_ManObjNum(pGia) );
    p->vProofIds = Vec_IntAlloc( 0 );
    p->vAbs      = Vec_IntAlloc( 1000 );
    p->vValues   = Vec_IntAlloc( 1000 );
    // add constant node to abstraction
    Ga2_ObjSetId( p, Gia_ManConst0(pGia), 0 );
    Vec_IntPush( p->vValues, 0 );
    Vec_IntPush( p->vAbs, 0 );
    // refinement
    p->pRnm      = Rnm_ManStart( pGia );
//    p->pRf2      = Rf2_ManStart( pGia );
    // SAT solver and variables
    p->vId2Lit   = Vec_PtrAlloc( 1000 );
    // temporaries
    p->vLits     = Vec_IntAlloc( 100 );
    p->vIsopMem  = Vec_IntAlloc( 100 );
    Cnf_ReadMsops( &p->pSopSizes, &p->pSops );
    // hash table
    p->nTable = Abc_PrimeCudd(1<<18);
    p->pTable = ABC_CALLOC( int, 6 * p->nTable ); 
    return p;
}

void Ga2_ManDumpStats( Gia_Man_t * pGia, Abs_Par_t * pPars, sat_solver2 * pSat, int iFrame, int fUseN )
{
    FILE * pFile;
    char pFileName[32];
    sprintf( pFileName, "stats_gla%s%s.txt", fUseN ? "n":"", pPars->fUseFullProof ? "p":"" ); 

    pFile = fopen( pFileName, "a+" );

    fprintf( pFile, "%s pi=%d ff=%d and=%d mem=%d bmc=%d", 
        pGia->pName, 
        Gia_ManPiNum(pGia), Gia_ManRegNum(pGia), Gia_ManAndNum(pGia), 
        (int)(1 + sat_solver2_memory_proof(pSat)/(1<<20)),
        iFrame );

    if ( pGia->vGateClasses )
    fprintf( pFile, " ff=%d and=%d",                 
        Gia_GlaCountFlops( pGia, pGia->vGateClasses ),
        Gia_GlaCountNodes( pGia, pGia->vGateClasses )  );

    fprintf( pFile, "\n" );
    fclose( pFile );
}
void Ga2_ManReportMemory( Ga2_Man_t * p )
{
    double memTot = 0;
    double memAig = 1.0 * p->pGia->nObjsAlloc * sizeof(Gia_Obj_t) + Vec_IntMemory(p->pGia->vMapping);
    double memSat = sat_solver2_memory( p->pSat, 1 );
    double memPro = sat_solver2_memory_proof( p->pSat );
    double memMap = Vec_VecMemoryInt( (Vec_Vec_t *)p->vId2Lit );
    double memRef = Rnm_ManMemoryUsage( p->pRnm );
    double memHash= sizeof(int) * 6 * p->nTable;
    double memOth = sizeof(Ga2_Man_t);
    memOth += Vec_VecMemoryInt( (Vec_Vec_t *)p->vCnfs );
    memOth += Vec_IntMemory( p->vIds );
    memOth += Vec_IntMemory( p->vProofIds );
    memOth += Vec_IntMemory( p->vAbs );
    memOth += Vec_IntMemory( p->vValues );
    memOth += Vec_IntMemory( p->vLits );
    memOth += Vec_IntMemory( p->vIsopMem );
    memOth += 336450 + (sizeof(char) + sizeof(char*)) * 65536;
    memTot = memAig + memSat + memPro + memMap + memRef + memHash + memOth;
    ABC_PRMP( "Memory: AIG      ", memAig, memTot );
    ABC_PRMP( "Memory: SAT      ", memSat, memTot );
    ABC_PRMP( "Memory: Proof    ", memPro, memTot );
    ABC_PRMP( "Memory: Map      ", memMap, memTot );
    ABC_PRMP( "Memory: Refine   ", memRef, memTot );
    ABC_PRMP( "Memory: Hash     ", memHash,memTot );
    ABC_PRMP( "Memory: Other    ", memOth, memTot );
    ABC_PRMP( "Memory: TOTAL    ", memTot, memTot );
}
void Ga2_ManStop( Ga2_Man_t * p )
{
    Vec_IntFreeP( &p->pGia->vMapping );
    Gia_ManSetPhase( p->pGia );
    if ( p->pPars->fVerbose )
        Abc_Print( 1, "SAT solver:  Var = %d  Cla = %d  Conf = %d  Lrn = %d  Reduce = %d  Cex = %d  ObjsAdded = %d\n", 
            sat_solver2_nvars(p->pSat), sat_solver2_nclauses(p->pSat), 
            sat_solver2_nconflicts(p->pSat), sat_solver2_nlearnts(p->pSat), 
            p->pSat->nDBreduces, p->nCexes, p->nObjAdded );
    if ( p->pPars->fVerbose )
    Abc_Print( 1, "Hash hits = %d.  Hash misses = %d.  Hash overs = %d.  Concurrent calls = %d.\n", 
        p->nHashHit, p->nHashMiss, p->nHashOver, p->nPdrCalls );

    if( p->pSat ) sat_solver2_delete( p->pSat );
    Vec_VecFree( (Vec_Vec_t *)p->vCnfs );
    Vec_VecFree( (Vec_Vec_t *)p->vId2Lit );
    Vec_IntFree( p->vIds );
    Vec_IntFree( p->vProofIds );
    Vec_IntFree( p->vAbs );
    Vec_IntFree( p->vValues );
    Vec_IntFree( p->vLits );
    Vec_IntFree( p->vIsopMem );
    Rnm_ManStop( p->pRnm, 0 );
//    Rf2_ManStop( p->pRf2, p->pPars->fVerbose );
    ABC_FREE( p->pTable );
    ABC_FREE( p->pSopSizes );
    ABC_FREE( p->pSops[1] );
    ABC_FREE( p->pSops );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Computes a minimized truth table.]

  Description [Input literals can be 0/1 (const 0/1), non-trivial literals 
  (integers that are more than 1) and unassigned literals (large integers).
  This procedure computes the truth table that essentially depends on input
  variables ordered in the increasing order of their positive literals.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Ga2_ObjTruthDepends( unsigned t, int v )
{
    static unsigned uInvTruth5[5] = { 0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF };
    assert( v >= 0 && v <= 4 );
    return ((t ^ (t >> (1 << v))) & uInvTruth5[v]);
}
unsigned Ga2_ObjComputeTruthSpecial( Gia_Man_t * p, Gia_Obj_t * pRoot, Vec_Int_t * vLeaves, Vec_Int_t * vLits )
{
    int fVerbose = 0;
    static unsigned uTruth5[5] = { 0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000 };
    unsigned Res;
    Gia_Obj_t * pObj;
    int i, Entry;
//    int Id = Gia_ObjId(p, pRoot);
    assert( Gia_ObjIsAnd(pRoot) );

    if ( fVerbose )
    printf( "Object %d.\n", Gia_ObjId(p, pRoot) );

    // assign elementary truth tables
    Gia_ManForEachObjVec( vLeaves, p, pObj, i )
    {
        Entry = Vec_IntEntry( vLits, i );
        assert( Entry >= 0 );
        if ( Entry == 0 )
            pObj->Value = 0;
        else if ( Entry == 1 )
            pObj->Value = ~0;
        else // non-trivial literal
            pObj->Value = uTruth5[i];
        if ( fVerbose )
        printf( "%d ", Entry );
    }

    if ( fVerbose )
    {
    Res = Ga2_ObjTruth( p, pRoot );
//    Kit_DsdPrintFromTruth( &Res, Vec_IntSize(vLeaves) );
    printf( "\n" );
    }

    // compute truth table
    Res = Ga2_ObjComputeTruth_rec( p, pRoot, 1 );
    if ( Res != 0 && Res != ~0 )
    {
        // find essential variables
        int nUsed = 0, pUsed[5];
        for ( i = 0; i < Vec_IntSize(vLeaves); i++ )
            if ( Ga2_ObjTruthDepends( Res, i ) )
                pUsed[nUsed++] = i;
        assert( nUsed > 0 );
        // order positions by literal value
        Vec_IntSelectSortCost( pUsed, nUsed, vLits );
        assert( Vec_IntEntry(vLits, pUsed[0]) <= Vec_IntEntry(vLits, pUsed[nUsed-1]) );
        // assign elementary truth tables to the leaves
        Gia_ManForEachObjVec( vLeaves, p, pObj, i )
        {
            Entry = Vec_IntEntry( vLits, i );
            assert( Entry >= 0 );
            if ( Entry == 0 )
                pObj->Value = 0;
            else if ( Entry == 1 )
                pObj->Value = ~0;
            else // non-trivial literal
                pObj->Value = 0xDEADCAFE; // not important
        }
        for ( i = 0; i < nUsed; i++ )
        {
            Entry = Vec_IntEntry( vLits, pUsed[i] );
            assert( Entry > 1 );
            pObj = Gia_ManObj( p, Vec_IntEntry(vLeaves, pUsed[i]) );
            pObj->Value = Abc_LitIsCompl(Entry) ? ~uTruth5[i] : uTruth5[i];
//            pObj->Value = uTruth5[i];
            // remember this literal
            pUsed[i] = Abc_LitRegular(Entry);
//            pUsed[i] = Entry;
        }
        // compute truth table
        Res = Ga2_ObjComputeTruth_rec( p, pRoot, 1 );
        // reload the literals
        Vec_IntClear( vLits );
        for ( i = 0; i < nUsed; i++ )
        {
            Vec_IntPush( vLits, pUsed[i] );
            assert( Ga2_ObjTruthDepends(Res, i) );
            if ( fVerbose )
            printf( "%d ", pUsed[i] );
        }
        for ( ; i < 5; i++ )
            assert( !Ga2_ObjTruthDepends(Res, i) );

if ( fVerbose )
{
//    Kit_DsdPrintFromTruth( &Res, nUsed );
    printf( "\n" );
}

    }
    else
    {

if ( fVerbose )
{
    Vec_IntClear( vLits );
    printf( "Const %d\n", Res > 0 );
}

    }
    Gia_ManForEachObjVec( vLeaves, p, pObj, i )
        pObj->Value = 0;
    return Res;
}

/**Function*************************************************************

  Synopsis    [Returns CNF of the function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Ga2_ManCnfCompute( unsigned uTruth, int nVars, Vec_Int_t * vCover )
{
    int RetValue;
    assert( nVars <= 5 );
    // transform truth table into the SOP
    RetValue = Kit_TruthIsop( &uTruth, nVars, vCover, 0 );
    assert( RetValue == 0 );
    // check the case of constant cover
    return Vec_IntDup( vCover );
}

/**Function*************************************************************

  Synopsis    [Derives CNF for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Ga2_ManCnfAddDynamic( Ga2_Man_t * p, int uTruth, int Lits[], int iLitOut, int ProofId )
{
    int i, k, b, Cube, nClaLits, ClaLits[6];
//    assert( uTruth > 0 && uTruth < 0xffff );
    for ( i = 0; i < 2; i++ )
    {
        if ( i )
            uTruth = 0xffff & ~uTruth;
//        Extra_PrintBinary( stdout, &uTruth, 16 ); printf( "\n" );
        for ( k = 0; k < p->pSopSizes[uTruth]; k++ )
        {
            nClaLits = 0;
            ClaLits[nClaLits++] = i ? lit_neg(iLitOut) : iLitOut;
            Cube = p->pSops[uTruth][k];
            for ( b = 3; b >= 0; b-- )
            {
                if ( Cube % 3 == 0 ) // value 0 --> add positive literal
                {
                    assert( Lits[b] > 1 );
                    ClaLits[nClaLits++] = Lits[b];
                }
                else if ( Cube % 3 == 1 ) // value 1 --> add negative literal
                {
                    assert( Lits[b] > 1 );
                    ClaLits[nClaLits++] = lit_neg(Lits[b]);
                }
                Cube = Cube / 3;
            }
            sat_solver2_addclause( p->pSat, ClaLits, ClaLits+nClaLits, ProofId );
        }
    }
}
void Ga2_ManCnfAddStatic( sat_solver2 * pSat, Vec_Int_t * vCnf0, Vec_Int_t * vCnf1, int Lits[], int iLitOut, int ProofId )
{
    Vec_Int_t * vCnf;
    int i, k, b, Cube, Literal, nClaLits, ClaLits[6];
    for ( i = 0; i < 2; i++ )
    {
        vCnf = i ? vCnf1 : vCnf0;
        Vec_IntForEachEntry( vCnf, Cube, k )
        {
            nClaLits = 0;
            ClaLits[nClaLits++] = i ? lit_neg(iLitOut) : iLitOut;
            for ( b = 0; b < 5; b++ )
            {
                Literal = 3 & (Cube >> (b << 1));
                if ( Literal == 1 ) // value 0 --> add positive literal
                {
//                    assert( Lits[b] > 1 );
                    ClaLits[nClaLits++] = Lits[b];
                }
                else if ( Literal == 2 ) // value 1 --> add negative literal
                {
//                    assert( Lits[b] > 1 );
                    ClaLits[nClaLits++] = lit_neg(Lits[b]);
                }
                else if ( Literal != 0 )
                    assert( 0 );
            }
            sat_solver2_addclause( pSat, ClaLits, ClaLits+nClaLits, ProofId );
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Saig_ManBmcHashKey( int * pArray )
{
    static int s_Primes[5] = { 12582917, 25165843, 50331653, 100663319, 201326611 };
    unsigned i, Key = 0;
    for ( i = 0; i < 5; i++ )
        Key += pArray[i] * s_Primes[i];
    return Key;
}
static inline int * Saig_ManBmcLookup( Ga2_Man_t * p, int * pArray )
{
    int * pTable = p->pTable + 6 * (Saig_ManBmcHashKey(pArray) % p->nTable);
    if ( memcmp( pTable, pArray, 20 ) )
    {
        if ( pTable[0] == 0 )
            p->nHashMiss++;
        else
            p->nHashOver++;
        memcpy( pTable, pArray, 20 );
        pTable[5] = 0;
    }
    else
        p->nHashHit++;
    assert( pTable + 5 < pTable + 6 * p->nTable );
    return pTable + 5;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Ga2_ManSetupNode( Ga2_Man_t * p, Gia_Obj_t * pObj, int fAbs )
{
    unsigned uTruth;
    int nLeaves;
//    int Id = Gia_ObjId(p->pGia, pObj);
    assert( pObj->fPhase );
    assert( Vec_PtrSize(p->vCnfs) == 2 * Vec_IntSize(p->vValues) );
    // assign abstraction ID to the node
    if ( Ga2_ObjId(p,pObj) == -1 )
    {
        Ga2_ObjSetId( p, pObj, Vec_IntSize(p->vValues) );
        Vec_IntPush( p->vValues, Gia_ObjId(p->pGia, pObj) );
        Vec_PtrPush( p->vCnfs, NULL );
        Vec_PtrPush( p->vCnfs, NULL );
    }
    assert( Ga2_ObjCnf0(p, pObj) == NULL );
    if ( !fAbs )
        return;
    Vec_IntPush( p->vAbs, Gia_ObjId(p->pGia, pObj) );
    assert( Gia_ObjIsAnd(pObj) || Gia_ObjIsRo(p->pGia, pObj) );
    // compute parameters
    nLeaves = Ga2_ObjLeaveNum(p->pGia, pObj);
    uTruth = Ga2_ObjTruth( p->pGia, pObj );
    // create CNF for pos/neg phases
    Vec_PtrWriteEntry( p->vCnfs, 2 * Ga2_ObjId(p,pObj),     Ga2_ManCnfCompute( uTruth, nLeaves, p->vIsopMem) );    
    Vec_PtrWriteEntry( p->vCnfs, 2 * Ga2_ObjId(p,pObj) + 1, Ga2_ManCnfCompute(~uTruth, nLeaves, p->vIsopMem) );
}

static inline void Ga2_ManAddToAbsOneStatic( Ga2_Man_t * p, Gia_Obj_t * pObj, int f, int fUseId )
{
    Vec_Int_t * vLeaves;
    Gia_Obj_t * pLeaf;
    int k, Lit, iLitOut = Ga2_ObjFindOrAddLit( p, pObj, f );
    assert( iLitOut > 1 );
    assert( Gia_ObjIsConst0(pObj) || Gia_ObjIsRo(p->pGia, pObj) || Gia_ObjIsAnd(pObj) );
    if ( Gia_ObjIsConst0(pObj) || (f == 0 && Gia_ObjIsRo(p->pGia, pObj)) )
    {
        iLitOut = Abc_LitNot( iLitOut );
        sat_solver2_addclause( p->pSat, &iLitOut, &iLitOut + 1, fUseId ? Gia_ObjId(p->pGia, pObj) : -1 );
    }
    else
    {
        int fUseStatic = 1;
        Vec_IntClear( p->vLits );
        vLeaves = Ga2_ObjLeaves( p->pGia, pObj );
        Gia_ManForEachObjVec( vLeaves, p->pGia, pLeaf, k )
        {
            Lit = Ga2_ObjFindOrAddLit( p, pLeaf, f - Gia_ObjIsRo(p->pGia, pObj) );
            Vec_IntPush( p->vLits, Lit );
            if ( Lit < 2 )
                fUseStatic = 0;
        }
        if ( fUseStatic || Gia_ObjIsRo(p->pGia, pObj) )
            Ga2_ManCnfAddStatic( p->pSat, Ga2_ObjCnf0(p, pObj), Ga2_ObjCnf1(p, pObj), Vec_IntArray(p->vLits), iLitOut, fUseId ? Gia_ObjId(p->pGia, pObj) : -1 );
        else
        {
            unsigned uTruth = Ga2_ObjComputeTruthSpecial( p->pGia, pObj, vLeaves, p->vLits );
            Ga2_ManCnfAddDynamic( p, uTruth & 0xFFFF, Vec_IntArray(p->vLits), iLitOut, Gia_ObjId(p->pGia, pObj) );
        }
    }
}
static inline void Ga2_ManAddToAbsOneDynamic( Ga2_Man_t * p, Gia_Obj_t * pObj, int f )
{
//    int Id = Gia_ObjId(p->pGia, pObj);
    Vec_Int_t * vLeaves;
    Gia_Obj_t * pLeaf;
    unsigned uTruth;
    int i, Lit = 0;

    assert( Ga2_ObjIsAbs0(p, pObj) );
    assert( Gia_ObjIsConst0(pObj) || Gia_ObjIsRo(p->pGia, pObj) || Gia_ObjIsAnd(pObj) );
    if ( Gia_ObjIsConst0(pObj) || (f == 0 && Gia_ObjIsRo(p->pGia, pObj)) )
    {
        Ga2_ObjAddLit( p, pObj, f, 0 );
    }
    else if ( Gia_ObjIsRo(p->pGia, pObj) )
    {
        // if flop is included in the abstraction, but its driver is not
        // flop input driver has no variable assigned -- we assign it here
        pLeaf = Gia_ObjRoToRi( p->pGia, pObj );
        Lit = Ga2_ObjFindOrAddLit( p, Gia_ObjFanin0(pLeaf), f-1 );
        assert( Lit >= 0 );
        Lit = Abc_LitNotCond( Lit, Gia_ObjFaninC0(pLeaf) );
        Ga2_ObjAddLit( p, pObj, f, Lit );
    }
    else
    {
        assert( Gia_ObjIsAnd(pObj) );
        Vec_IntClear( p->vLits );
        vLeaves = Ga2_ObjLeaves( p->pGia, pObj );
        Gia_ManForEachObjVec( vLeaves, p->pGia, pLeaf, i )
        {
            if ( Ga2_ObjIsAbs0(p, pLeaf) )      // belongs to original abstraction
            {
                Lit = Ga2_ObjFindLit( p, pLeaf, f );
                assert( Lit >= 0 );
            }
            else if ( Ga2_ObjIsLeaf0(p, pLeaf) ) // belongs to original PPIs
            {
                Lit = Ga2_ObjFindLit( p, pLeaf, f );
//                Lit = Ga2_ObjFindOrAddLit( p, pLeaf, f );
                if ( Lit == -1 )
                {
                    Lit = GA2_BIG_NUM + 2*i;
//                    assert( 0 );
                }
            }
            else assert( 0 );
            Vec_IntPush( p->vLits, Lit );
        }

        // minimize truth table
        uTruth = Ga2_ObjComputeTruthSpecial( p->pGia, pObj, vLeaves, p->vLits );
        if ( uTruth == 0 || uTruth == ~0 ) // const 0 / 1
        {
            Lit = (uTruth > 0);
            Ga2_ObjAddLit( p, pObj, f, Lit );
        }
        else if ( uTruth == 0xAAAAAAAA || uTruth == 0x55555555 )  // buffer / inverter
        {
            Lit = Vec_IntEntry( p->vLits, 0 );
            if ( Lit >= GA2_BIG_NUM )
            {
                pLeaf = Gia_ManObj( p->pGia, Vec_IntEntry(vLeaves, (Lit-GA2_BIG_NUM)/2) );
                Lit = Ga2_ObjFindLit( p, pLeaf, f );
                assert( Lit == -1 );
                Lit = Ga2_ObjFindOrAddLit( p, pLeaf, f );
            }
            assert( Lit >= 0 );
            Lit = Abc_LitNotCond( Lit, uTruth == 0x55555555 );
            Ga2_ObjAddLit( p, pObj, f, Lit );
            assert( Lit < 10000000 );
        }
        else
        {
            assert( Vec_IntSize(p->vLits) > 1 && Vec_IntSize(p->vLits) < 6 );
            // replace literals
            Vec_IntForEachEntry( p->vLits, Lit, i )
            {
                if ( Lit >= GA2_BIG_NUM )
                {
                    pLeaf = Gia_ManObj( p->pGia, Vec_IntEntry(vLeaves, (Lit-GA2_BIG_NUM)/2) );
                    Lit = Ga2_ObjFindLit( p, pLeaf, f );
                    assert( Lit == -1 );
                    Lit = Ga2_ObjFindOrAddLit( p, pLeaf, f );
                    Vec_IntWriteEntry( p->vLits, i, Lit );
                }
                assert( Lit < 10000000 );
            }

            // add new nodes
            if ( Vec_IntSize(p->vLits) == 5 )
            {
                Vec_IntClear( p->vLits );
                Gia_ManForEachObjVec( vLeaves, p->pGia, pLeaf, i )
                    Vec_IntPush( p->vLits, Ga2_ObjFindOrAddLit( p, pLeaf, f ) );
                Lit = Ga2_ObjFindOrAddLit(p, pObj, f);
                Ga2_ManCnfAddStatic( p->pSat, Ga2_ObjCnf0(p, pObj), Ga2_ObjCnf1(p, pObj), Vec_IntArray(p->vLits), Lit, -1 );
            }
            else
            {
//                int fUseHash = 1;
                if ( !p->pPars->fSkipHash )
                {
                    int * pLookup, nSize = Vec_IntSize(p->vLits);
                    assert( Vec_IntSize(p->vLits) < 5 );
                    assert( Vec_IntEntry(p->vLits, 0) <= Vec_IntEntryLast(p->vLits) );
                    assert( Ga2_ObjFindLit(p, pObj, f) == -1 );
                    for ( i = Vec_IntSize(p->vLits); i < 4; i++ )
                        Vec_IntPush( p->vLits, GA2_BIG_NUM );
                    Vec_IntPush( p->vLits, (int)uTruth );
                    assert( Vec_IntSize(p->vLits) == 5 );

                    // perform structural hashing here!!!
                    pLookup = Saig_ManBmcLookup( p, Vec_IntArray(p->vLits) );
                    if ( *pLookup == 0 )
                    {
                        *pLookup = Ga2_ObjFindOrAddLit(p, pObj, f);
                        Vec_IntShrink( p->vLits, nSize );
                        Ga2_ManCnfAddDynamic( p, uTruth & 0xFFFF, Vec_IntArray(p->vLits), *pLookup, -1 );
                    }
                    else
                        Ga2_ObjAddLit( p, pObj, f, *pLookup );

                }
                else
                {
                    Lit = Ga2_ObjFindOrAddLit(p, pObj, f);
                    Ga2_ManCnfAddDynamic( p, uTruth & 0xFFFF, Vec_IntArray(p->vLits), Lit, -1 );
                }
            }
        }
    }
}

void Ga2_ManAddAbsClauses( Ga2_Man_t * p, int f )
{
    int fSimple = 0;
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObjVec( p->vValues, p->pGia, pObj, i )
    {
        if ( i == p->LimAbs )
            break;
        if ( fSimple )
            Ga2_ManAddToAbsOneStatic( p, pObj, f, 0 );
        else
            Ga2_ManAddToAbsOneDynamic( p, pObj, f );
    }
    Gia_ManForEachObjVec( p->vAbs, p->pGia, pObj, i )
        if ( i >= p->LimAbs )
            Ga2_ManAddToAbsOneStatic( p, pObj, f, 1 );
//    sat_solver2_simplify( p->pSat );
}

void Ga2_ManAddToAbs( Ga2_Man_t * p, Vec_Int_t * vToAdd )
{
    Vec_Int_t * vLeaves;
    Gia_Obj_t * pObj, * pFanin;
    int f, i, k;
    // add abstraction objects
    Gia_ManForEachObjVec( vToAdd, p->pGia, pObj, i )
    {
        Ga2_ManSetupNode( p, pObj, 1 );
        if ( p->pSat->pPrf2 )
            Vec_IntWriteEntry( p->vProofIds, Gia_ObjId(p->pGia, pObj), p->nProofIds++ );
    }
    // add PPI objects
    Gia_ManForEachObjVec( vToAdd, p->pGia, pObj, i )
    {
        vLeaves = Ga2_ObjLeaves( p->pGia, pObj );
        Gia_ManForEachObjVec( vLeaves, p->pGia, pFanin, k )
            if ( Ga2_ObjId( p, pFanin ) == -1 )
                Ga2_ManSetupNode( p, pFanin, 0 );
    }
    // add new clauses to the timeframes
    for ( f = 0; f <= p->pPars->iFrame; f++ )
    {
        Vec_IntFillExtra( Ga2_MapFrameMap(p, f), Vec_IntSize(p->vValues), -1 );
        Gia_ManForEachObjVec( vToAdd, p->pGia, pObj, i )
            Ga2_ManAddToAbsOneStatic( p, pObj, f, 1 );
    }
//    sat_solver2_simplify( p->pSat );
}

void Ga2_ManShrinkAbs( Ga2_Man_t * p, int nAbs, int nValues, int nSatVars )
{
    Vec_Int_t * vMap;
    Gia_Obj_t * pObj;
    int i, k, Entry;
    assert( nAbs > 0 );
    assert( nValues > 0 );
    assert( nSatVars > 0 );
    // shrink abstraction
    Gia_ManForEachObjVec( p->vAbs, p->pGia, pObj, i )
    {
        if ( !i ) continue;
        assert( Ga2_ObjCnf0(p, pObj) != NULL );
        assert( Ga2_ObjCnf1(p, pObj) != NULL );
        if ( i < nAbs )
            continue;
        Vec_IntFree( Ga2_ObjCnf0(p, pObj) );
        Vec_IntFree( Ga2_ObjCnf1(p, pObj) );
        Vec_PtrWriteEntry( p->vCnfs, 2 * Ga2_ObjId(p,pObj),     NULL );    
        Vec_PtrWriteEntry( p->vCnfs, 2 * Ga2_ObjId(p,pObj) + 1, NULL );
    }
    Vec_IntShrink( p->vAbs, nAbs );
    // shrink values
    Gia_ManForEachObjVec( p->vValues, p->pGia, pObj, i )
    {
        assert( Ga2_ObjId(p,pObj) >= 0 );
        if ( i < nValues )
            continue;
        Ga2_ObjSetId( p, pObj, -1 );
    }
    Vec_IntShrink( p->vValues, nValues );
    Vec_PtrShrink( p->vCnfs, 2 * nValues );
    // hack to clear constant
    if ( nValues == 1 )
        nValues = 0;
    // clean mapping for each timeframe
    Vec_PtrForEachEntry( Vec_Int_t *, p->vId2Lit, vMap, i )
    {
        Vec_IntShrink( vMap, nValues );
        Vec_IntForEachEntryStart( vMap, Entry, k, p->LimAbs )
            if ( Entry >= 2*nSatVars )
                Vec_IntWriteEntry( vMap, k, -1 );
    }
    // shrink SAT variables
    p->nSatVars = nSatVars;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ga2_ManAbsTranslate_rec( Gia_Man_t * p, Gia_Obj_t * pObj, Vec_Int_t * vClasses, int fFirst )
{
    if ( pObj->fPhase && !fFirst )
        return;
    assert( Gia_ObjIsAnd(pObj) );
    Ga2_ManAbsTranslate_rec( p, Gia_ObjFanin0(pObj), vClasses, 0 );
    Ga2_ManAbsTranslate_rec( p, Gia_ObjFanin1(pObj), vClasses, 0 );
    Vec_IntWriteEntry( vClasses, Gia_ObjId(p, pObj), 1 );
}

Vec_Int_t * Ga2_ManAbsTranslate( Ga2_Man_t * p )
{
    Vec_Int_t * vGateClasses;
    Gia_Obj_t * pObj;
    int i;
    vGateClasses = Vec_IntStart( Gia_ManObjNum(p->pGia) );
    Vec_IntWriteEntry( vGateClasses, 0, 1 );
    Gia_ManForEachObjVec( p->vAbs, p->pGia, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            Ga2_ManAbsTranslate_rec( p->pGia, pObj, vGateClasses, 1 );
        else if ( Gia_ObjIsRo(p->pGia, pObj) )
            Vec_IntWriteEntry( vGateClasses, Gia_ObjId(p->pGia, pObj), 1 );
        else if ( !Gia_ObjIsConst0(pObj) )
            assert( 0 );
//        Gia_ObjPrint( p->pGia, pObj );
    }
    return vGateClasses;
}

Vec_Int_t * Ga2_ManAbsDerive( Gia_Man_t * p )
{
    Vec_Int_t * vToAdd;
    Gia_Obj_t * pObj;
    int i;
    vToAdd = Vec_IntAlloc( 1000 );
    Gia_ManForEachRo( p, pObj, i )
        if ( pObj->fPhase && Vec_IntEntry(p->vGateClasses, Gia_ObjId(p, pObj)) )
            Vec_IntPush( vToAdd, Gia_ObjId(p, pObj) );
    Gia_ManForEachAnd( p, pObj, i )
        if ( pObj->fPhase && Vec_IntEntry(p->vGateClasses, i) )
            Vec_IntPush( vToAdd, i );
    return vToAdd;
}

void Ga2_ManRestart( Ga2_Man_t * p )
{
    Vec_Int_t * vToAdd;
    int Lit = 1;
    assert( p->pGia != NULL && p->pGia->vGateClasses != NULL );
    assert( Gia_ManPi(p->pGia, 0)->fPhase ); // marks are set
    // clear SAT variable numbers (begin with 1)
    if ( p->pSat ) sat_solver2_delete( p->pSat );
    p->pSat      = sat_solver2_new();
    p->pSat->nLearntStart = p->pPars->nLearnedStart;
    p->pSat->nLearntDelta = p->pPars->nLearnedDelta;
    p->pSat->nLearntRatio = p->pPars->nLearnedPerce;
    p->pSat->nLearntMax   = p->pSat->nLearntStart;
    // add clause x0 = 0  (lit0 = 1; lit1 = 0)
    sat_solver2_addclause( p->pSat, &Lit, &Lit + 1, -1 );
    // remove previous abstraction
    Ga2_ManShrinkAbs( p, 1, 1, 1 );
    // start new abstraction
    vToAdd = Ga2_ManAbsDerive( p->pGia );
    assert( p->pSat->pPrf2 == NULL );
    assert( p->pPars->iFrame < 0 );
    Ga2_ManAddToAbs( p, vToAdd );
    Vec_IntFree( vToAdd );
    p->LimAbs = Vec_IntSize(p->vAbs);
    p->LimPpi = Vec_IntSize(p->vValues);
    // set runtime limit
    if ( p->pPars->nTimeOut )
        sat_solver2_set_runtime_limit( p->pSat, p->pPars->nTimeOut * CLOCKS_PER_SEC + p->timeStart );
    // clean the hash table
    memset( p->pTable, 0, 6 * sizeof(int) * p->nTable );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Ga2_ObjSatValue( Ga2_Man_t * p, Gia_Obj_t * pObj, int f )
{
    int Lit = Ga2_ObjFindLit( p, pObj, f );
    assert( !Gia_ObjIsConst0(pObj) );
    if ( Lit == -1 )
        return 0;
    if ( Abc_Lit2Var(Lit) >= p->pSat->size )
        return 0;
    return Abc_LitIsCompl(Lit) ^ sat_solver2_var_value( p->pSat, Abc_Lit2Var(Lit) );
}
Abc_Cex_t * Ga2_ManDeriveCex( Ga2_Man_t * p, Vec_Int_t * vPis )
{
    Abc_Cex_t * pCex;
    Gia_Obj_t * pObj;
    int i, f;
    pCex = Abc_CexAlloc( Gia_ManRegNum(p->pGia), Gia_ManPiNum(p->pGia), p->pPars->iFrame+1 );
    pCex->iPo = 0;
    pCex->iFrame = p->pPars->iFrame;
    Gia_ManForEachObjVec( vPis, p->pGia, pObj, i )
    {
        if ( !Gia_ObjIsPi(p->pGia, pObj) )
            continue;
        assert( Gia_ObjIsPi(p->pGia, pObj) );
        for ( f = 0; f <= pCex->iFrame; f++ )
            if ( Ga2_ObjSatValue( p, pObj, f ) )
                Abc_InfoSetBit( pCex->pData, pCex->nRegs + f * pCex->nPis + Gia_ObjCioId(pObj) );
    }
    return pCex;
} 
void Ga2_ManRefinePrint( Ga2_Man_t * p, Vec_Int_t * vVec )
{
    Gia_Obj_t * pObj, * pFanin;
    int i, k;
    printf( "\n         Unsat core: \n" );
    Gia_ManForEachObjVec( vVec, p->pGia, pObj, i )
    {
        Vec_Int_t * vLeaves = Ga2_ObjLeaves( p->pGia, pObj );
        printf( "%12d : ", i );
        printf( "Obj =%6d ", Gia_ObjId(p->pGia, pObj) );
        if ( Gia_ObjIsRo(p->pGia, pObj) )
            printf( "ff " );
        else
            printf( "   " );
        if ( Ga2_ObjIsAbs0(p, pObj) )
            printf( "a " );
        else if ( Ga2_ObjIsLeaf0(p, pObj) )
            printf( "l " );
        else 
            printf( "  " );
        printf( "Fanins: " );
        Gia_ManForEachObjVec( vLeaves, p->pGia, pFanin, k )
        {
            printf( "%6d ", Gia_ObjId(p->pGia, pFanin) );
            if ( Gia_ObjIsRo(p->pGia, pFanin) )
                printf( "ff " );
            else
                printf( "   " );
            if ( Ga2_ObjIsAbs0(p, pFanin) )
                printf( "a " );
            else if ( Ga2_ObjIsLeaf0(p, pFanin) )
                printf( "l " );
            else
                printf( "  " );
        }
        printf( "\n" );
    }
}
void Ga2_ManRefinePrintPPis( Ga2_Man_t * p )
{
    Vec_Int_t * vVec = Vec_IntAlloc( 100 );
    Gia_Obj_t * pObj;
    int i;
    Gia_ManForEachObjVec( p->vValues, p->pGia, pObj, i )
    {
        if ( !i ) continue;
        if ( Ga2_ObjIsAbs(p, pObj) )
            continue;
        assert( pObj->fPhase );
        assert( Ga2_ObjIsLeaf(p, pObj) );
        assert( Gia_ObjIsAnd(pObj) || Gia_ObjIsCi(pObj) );
        Vec_IntPush( vVec, Gia_ObjId(p->pGia, pObj) );
    }
    printf( "        Current PPIs (%d): ", Vec_IntSize(vVec) );
    Vec_IntSort( vVec, 1 );
    Gia_ManForEachObjVec( vVec, p->pGia, pObj, i )
        printf( "%d ", Gia_ObjId(p->pGia, pObj) );
    printf( "\n" );
    Vec_IntFree( vVec );
}


void Ga2_GlaPrepareCexAndMap( Ga2_Man_t * p, Abc_Cex_t ** ppCex, Vec_Int_t ** pvMaps )
{
    Abc_Cex_t * pCex;
    Vec_Int_t * vMap;
    Gia_Obj_t * pObj;
    int f, i, k;
/*
    Gia_ManForEachObj( p->pGia, pObj, i )
        if ( Ga2_ObjId(p, pObj) >= 0 )
            assert( Vec_IntEntry(p->vValues, Ga2_ObjId(p, pObj)) == i );
*/
    // find PIs and PPIs
    vMap = Vec_IntAlloc( 1000 );
    Gia_ManForEachObjVec( p->vValues, p->pGia, pObj, i )
    {
        if ( !i ) continue;
        if ( Ga2_ObjIsAbs(p, pObj) )
            continue;
        assert( pObj->fPhase );
        assert( Ga2_ObjIsLeaf(p, pObj) );
        assert( Gia_ObjIsAnd(pObj) || Gia_ObjIsCi(pObj) );
        Vec_IntPush( vMap, Gia_ObjId(p->pGia, pObj) );
    }
    // derive counter-example
    pCex = Abc_CexAlloc( 0, Vec_IntSize(vMap), p->pPars->iFrame+1 );
    pCex->iFrame = p->pPars->iFrame;
    for ( f = 0; f <= p->pPars->iFrame; f++ )
        Gia_ManForEachObjVec( vMap, p->pGia, pObj, k )
            if ( Ga2_ObjSatValue( p, pObj, f ) )
                Abc_InfoSetBit( pCex->pData, f * Vec_IntSize(vMap) + k );
    *pvMaps = vMap;
    *ppCex = pCex;
}
Vec_Int_t * Ga2_ManRefine( Ga2_Man_t * p )
{
    Abc_Cex_t * pCex;
    Vec_Int_t * vMap, * vVec;
    Gia_Obj_t * pObj;
    int i, k;
    if ( p->pPars->fAddLayer )
    {
        // use simplified refinement strategy, which adds logic near at PPI without finding important ones
        vVec = Vec_IntAlloc( 100 );
        Gia_ManForEachObjVec( p->vValues, p->pGia, pObj, i )
        {
            if ( !i ) continue;
            if ( Ga2_ObjIsAbs(p, pObj) )
                continue;
            assert( pObj->fPhase );
            assert( Ga2_ObjIsLeaf(p, pObj) );
            assert( Gia_ObjIsAnd(pObj) || Gia_ObjIsCi(pObj) );
            if ( Gia_ObjIsPi(p->pGia, pObj) )
                continue;
            Vec_IntPush( vVec, Gia_ObjId(p->pGia, pObj) );
        }
        p->nObjAdded += Vec_IntSize(vVec);
        return vVec;
    }
    Ga2_GlaPrepareCexAndMap( p, &pCex, &vMap );
 //    Rf2_ManRefine( p->pRf2, pCex, vMap, p->pPars->fPropFanout, 1 );
    vVec = Rnm_ManRefine( p->pRnm, pCex, vMap, p->pPars->fPropFanout, p->pPars->fNewRefine, 1 );
//    printf( "Refinement %d\n", Vec_IntSize(vVec) );
    Abc_CexFree( pCex );
    if ( Vec_IntSize(vVec) == 0 )
    {
        Vec_IntFree( vVec );
        Abc_CexFreeP( &p->pGia->pCexSeq );
        p->pGia->pCexSeq = Ga2_ManDeriveCex( p, vMap );
        Vec_IntFree( vMap );
        return NULL;
    }
    Vec_IntFree( vMap );
    // remove those already added
    k = 0;
    Gia_ManForEachObjVec( vVec, p->pGia, pObj, i )
        if ( !Ga2_ObjIsAbs(p, pObj) )
            Vec_IntWriteEntry( vVec, k++, Gia_ObjId(p->pGia, pObj) );
    Vec_IntShrink( vVec, k );

    // these objects should be PPIs that are not abstracted yet
    Gia_ManForEachObjVec( vVec, p->pGia, pObj, i )
        assert( pObj->fPhase );//&& Ga2_ObjIsLeaf(p, pObj) );
    p->nObjAdded += Vec_IntSize(vVec);
    return vVec;
}

/**Function*************************************************************

  Synopsis    [Creates a new manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ga2_GlaAbsCount( Ga2_Man_t * p, int fRo, int fAnd )
{
    Gia_Obj_t * pObj;
    int i, Counter = 0;
    if ( fRo )
        Gia_ManForEachObjVec( p->vAbs, p->pGia, pObj, i )
            Counter += Gia_ObjIsRo(p->pGia, pObj);
    else if ( fAnd )
        Gia_ManForEachObjVec( p->vAbs, p->pGia, pObj, i )
            Counter += Gia_ObjIsAnd(pObj);
    else assert( 0 );
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ga2_ManAbsPrintFrame( Ga2_Man_t * p, int nFrames, int nConfls, int nCexes, abctime Time, int fFinal )
{
    int fUseNewLine = ((fFinal && nCexes) || p->pPars->fVeryVerbose);
    if ( Abc_FrameIsBatchMode() && !fUseNewLine )
        return;
    p->fUseNewLine = fUseNewLine;
    Abc_Print( 1, "%4d :", nFrames );
    Abc_Print( 1, "%4d", Abc_MinInt(100, 100 * Vec_IntSize(p->vAbs) / p->nMarked) ); 
    Abc_Print( 1, "%6d", Vec_IntSize(p->vAbs) );
    Abc_Print( 1, "%5d", Vec_IntSize(p->vValues)-Vec_IntSize(p->vAbs)-1 );
    Abc_Print( 1, "%5d", Ga2_GlaAbsCount(p, 1, 0) );
    Abc_Print( 1, "%6d", Ga2_GlaAbsCount(p, 0, 1) );
    Abc_Print( 1, "%8d", nConfls );
    if ( nCexes == 0 )
        Abc_Print( 1, "%5c", '-' ); 
    else
        Abc_Print( 1, "%5d", nCexes ); 
    Abc_PrintInt( sat_solver2_nvars(p->pSat) );
    Abc_PrintInt( sat_solver2_nclauses(p->pSat) );
    Abc_PrintInt( sat_solver2_nlearnts(p->pSat) );
    Abc_Print( 1, "%9.2f sec", 1.0*Time/CLOCKS_PER_SEC );
    Abc_Print( 1, "%5.0f MB", (sat_solver2_memory_proof(p->pSat) + sat_solver2_memory(p->pSat, 0)) / (1<<20) );
    Abc_Print( 1, "%s", fUseNewLine ? "\n" : "\r" );
    fflush( stdout );
}

/**Function*************************************************************

  Synopsis    [Send abstracted model or send cancel.]

  Description [Counter-example will be sent automatically when &vta terminates.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ga2_GlaGetFileName( Ga2_Man_t * p, int fAbs )
{
    static char * pFileNameDef = "glabs.aig";
    if ( p->pPars->pFileVabs )
        return p->pPars->pFileVabs;
    if ( p->pGia->pSpec )
    {
        if ( fAbs )
            return Extra_FileNameGenericAppend( p->pGia->pSpec, "_abs.aig");
        else
            return Extra_FileNameGenericAppend( p->pGia->pSpec, "_gla.aig");
    }
    return pFileNameDef;
}

void Ga2_GlaDumpAbsracted( Ga2_Man_t * p, int fVerbose )
{
    char * pFileName;
    assert( p->pPars->fDumpMabs || p->pPars->fDumpVabs );
    if ( p->pPars->fDumpMabs )
    {
        pFileName = Ga2_GlaGetFileName(p, 0);
        if ( fVerbose )
            Abc_Print( 1, "Dumping miter with abstraction map into file \"%s\"...\n", pFileName );
        // dump abstraction map
        Vec_IntFreeP( &p->pGia->vGateClasses );
        p->pGia->vGateClasses = Ga2_ManAbsTranslate( p );
        Gia_AigerWrite( p->pGia, pFileName, 0, 0, 0 );
    }
    else if ( p->pPars->fDumpVabs )
    {
        Vec_Int_t * vGateClasses;
        Gia_Man_t * pAbs;
        pFileName = Ga2_GlaGetFileName(p, 1);
        if ( fVerbose )
            Abc_Print( 1, "Dumping abstracted model into file \"%s\"...\n", pFileName );
        // dump absracted model
        vGateClasses = Ga2_ManAbsTranslate( p );
        pAbs = Gia_ManDupAbsGates( p->pGia, vGateClasses );
        Gia_ManCleanValue( p->pGia );
        Gia_AigerWrite( pAbs, pFileName, 0, 0, 0 );
        Gia_ManStop( pAbs );
        Vec_IntFreeP( &vGateClasses );
    }
    else assert( 0 );
}

/**Function*************************************************************

  Synopsis    [Send abstracted model or send cancel.]

  Description [Counter-example will be sent automatically when &vta terminates.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_Ga2SendAbsracted( Ga2_Man_t * p, int fVerbose )
{
    Gia_Man_t * pAbs;
    Vec_Int_t * vGateClasses;
    assert( Abc_FrameIsBridgeMode() );
//    if ( fVerbose )
//        Abc_Print( 1, "Sending abstracted model...\n" );
    // create abstraction (value of p->pGia is not used here)
    vGateClasses = Ga2_ManAbsTranslate( p );
    pAbs = Gia_ManDupAbsGates( p->pGia, vGateClasses );
    Vec_IntFreeP( &vGateClasses );
    Gia_ManCleanValue( p->pGia );
    // send it out
    Gia_ManToBridgeAbsNetlist( stdout, pAbs, BRIDGE_ABS_NETLIST );
    Gia_ManStop( pAbs );
}
void Gia_Ga2SendCancel( Ga2_Man_t * p, int fVerbose )
{
    extern int Gia_ManToBridgeBadAbs( FILE * pFile );
    assert( Abc_FrameIsBridgeMode() );
//    if ( fVerbose )
//        Abc_Print( 1, "Cancelling previously sent model...\n" );
    Gia_ManToBridgeBadAbs( stdout );
}

/**Function*************************************************************

  Synopsis    [Performs gate-level abstraction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManPerformGla( Gia_Man_t * pAig, Abs_Par_t * pPars )
{
    int fUseSecondCore = 1;
    Ga2_Man_t * p;
    Vec_Int_t * vCore, * vPPis;
    abctime clk2, clk = Abc_Clock();
    int Status = l_Undef, RetValue = -1, iFrameTryToProve = -1, fOneIsSent = 0;
    int i, c, f, Lit;
    pPars->iFrame = -1;
    // check trivial case 
    assert( Gia_ManPoNum(pAig) == 1 ); 
    ABC_FREE( pAig->pCexSeq );
    if ( Gia_ObjIsConst0(Gia_ObjFanin0(Gia_ManPo(pAig,0))) )
    {
        if ( !Gia_ObjFaninC0(Gia_ManPo(pAig,0)) )
        {
            Abc_Print( 1, "Sequential miter is trivially UNSAT.\n" );
            return 1;
        }
        pAig->pCexSeq = Abc_CexMakeTriv( Gia_ManRegNum(pAig), Gia_ManPiNum(pAig), 1, 0 );
        Abc_Print( 1, "Sequential miter is trivially SAT.\n" );
        return 0;
    }
    // create gate classes if not given
    if ( pAig->vGateClasses == NULL )
    {
        pAig->vGateClasses = Vec_IntStart( Gia_ManObjNum(pAig) );
        Vec_IntWriteEntry( pAig->vGateClasses, 0, 1 );
        Vec_IntWriteEntry( pAig->vGateClasses, Gia_ObjFaninId0p(pAig, Gia_ManPo(pAig, 0)), 1 );
    }
    // start the manager
    p = Ga2_ManStart( pAig, pPars );
    p->timeInit = Abc_Clock() - clk;
    // perform initial abstraction
    if ( p->pPars->fVerbose )
    {
        Abc_Print( 1, "Running gate-level abstraction (GLA) with the following parameters:\n" );
        Abc_Print( 1, "FrameMax = %d  ConfMax = %d  Timeout = %d  Limit = %d %%  Limit2 = %d %%  RatioMax = %d %%\n", 
            pPars->nFramesMax, pPars->nConfLimit, pPars->nTimeOut, pPars->nRatioMin, pPars->nRatioMin2, pPars->nRatioMax );
        Abc_Print( 1, "LrnStart = %d  LrnDelta = %d  LrnRatio = %d %%  Skip = %d  SimpleCNF = %d  Dump = %d\n", 
            pPars->nLearnedStart, pPars->nLearnedDelta, pPars->nLearnedPerce, pPars->fUseSkip, pPars->fUseSimple, pPars->fDumpVabs|pPars->fDumpMabs );
        if ( pPars->fDumpVabs || pPars->fDumpMabs )
            Abc_Print( 1, "%s will be continuously dumped into file \"%s\".\n", 
                pPars->fDumpVabs ? "Abstracted model" : "Miter with abstraction map",
                Ga2_GlaGetFileName(p, pPars->fDumpVabs) );
        if ( pPars->fDumpMabs )
        {
            {
                char Command[1000];
                Abc_FrameSetStatus( -1 );
                Abc_FrameSetCex( NULL );
                Abc_FrameSetNFrames( -1 );
                sprintf( Command, "write_status %s", Extra_FileNameGenericAppend((char *)(p->pPars->pFileVabs ? p->pPars->pFileVabs : "glabs.aig"), ".status"));
                Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), Command );
            }
            {
                // create trivial abstraction map
                Gia_Obj_t * pObj; 
                char * pFileName = Ga2_GlaGetFileName(p, 0);
                Vec_Int_t * vMap = pAig->vGateClasses; pAig->vGateClasses = NULL;
                pAig->vGateClasses = Vec_IntStart( Gia_ManObjNum(pAig) );
                Vec_IntWriteEntry( pAig->vGateClasses, 0, 1 );
                Gia_ManForEachAnd( pAig, pObj, i )
                    Vec_IntWriteEntry( pAig->vGateClasses, i, 1 );
                Gia_ManForEachRo( pAig, pObj, i )
                    Vec_IntWriteEntry( pAig->vGateClasses, Gia_ObjId(pAig, pObj), 1 );
                Gia_AigerWrite( pAig, pFileName, 0, 0, 0 );
                Vec_IntFree( pAig->vGateClasses );
                pAig->vGateClasses = vMap;
                if ( p->pPars->fVerbose )
                    Abc_Print( 1, "Dumping miter with abstraction map into file \"%s\"...\n", pFileName );
            }
        }
        Abc_Print( 1, " Frame   %%   Abs  PPI   FF   LUT   Confl  Cex   Vars   Clas   Lrns     Time        Mem\n" );
    }
    // iterate unrolling
    for ( i = f = 0; !pPars->nFramesMax || f < pPars->nFramesMax; i++ )
    {
        int nAbsOld;
        // remember the timeframe
        p->pPars->iFrame = -1;
        // create new SAT solver
        Ga2_ManRestart( p );
        // remember abstraction size after the last restart
        nAbsOld = Vec_IntSize(p->vAbs);
        // unroll the circuit
        for ( f = 0; !pPars->nFramesMax || f < pPars->nFramesMax; f++ )
        {
            // remember current limits
            int nConflsBeg = sat_solver2_nconflicts(p->pSat);
            int nAbs       = Vec_IntSize(p->vAbs);
            int nValues    = Vec_IntSize(p->vValues);
            int nVarsOld;
            // remember the timeframe
            p->pPars->iFrame = f;
            // extend and clear storage
            if ( f == Vec_PtrSize(p->vId2Lit) )
                Vec_PtrPush( p->vId2Lit, Vec_IntAlloc(0) );
            Vec_IntFillExtra( Ga2_MapFrameMap(p, f), Vec_IntSize(p->vValues), -1 );
            // add static clauses to this timeframe
            Ga2_ManAddAbsClauses( p, f );
            // skip checking if skipcheck is enabled (&gla -s)
            if ( p->pPars->fUseSkip && f <= p->pPars->iFrameProved )
                continue;
            // skip checking if we need to skip several starting frames (&gla -S <num>)
            if ( p->pPars->nFramesStart && f <= p->pPars->nFramesStart )
                continue;
            // get the output literal
//            Lit = Ga2_ManUnroll_rec( p, Gia_ManPo(pAig,0), f );
            Lit = Ga2_ObjFindLit( p, Gia_ObjFanin0(Gia_ManPo(pAig,0)), f );
            assert( Lit >= 0 );
            Lit = Abc_LitNotCond( Lit, Gia_ObjFaninC0(Gia_ManPo(pAig,0)) );
            if ( Lit == 0 )
                continue;
            assert( Lit > 1 );
            // check for counter-examples
            if ( p->nSatVars > sat_solver2_nvars(p->pSat) )
                sat_solver2_setnvars( p->pSat, p->nSatVars );
            nVarsOld = p->nSatVars;
            for ( c = 0; ; c++ )
            {
                // consider the special case when the target literal is implied false
                // by implications which happened as a result of previous refinements
                // note that incremental UNSAT core cannot be computed because there is no learned clauses
                // in this case, we will assume that UNSAT core cannot reduce the problem
                if ( var_is_assigned(p->pSat, Abc_Lit2Var(Lit)) )
                {
                    Prf_ManStopP( &p->pSat->pPrf2 );
                    break;
                }
                // perform SAT solving
                clk2 = Abc_Clock();
                Status = sat_solver2_solve( p->pSat, &Lit, &Lit+1, (ABC_INT64_T)pPars->nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
                if ( Status == l_True ) // perform refinement
                {
                    p->nCexes++;
                    p->timeSat += Abc_Clock() - clk2;

                    // cancel old one if it was sent
                    if ( Abc_FrameIsBridgeMode() && fOneIsSent )
                    {
                        Gia_Ga2SendCancel( p, pPars->fVerbose );
                        fOneIsSent = 0;
                    }
                    if ( iFrameTryToProve >= 0 )
                    {
                        Gia_GlaProveCancel( pPars->fVerbose );
                        iFrameTryToProve = -1;
                    }

                    // perform refinement
                    clk2 = Abc_Clock();
                    Rnm_ManSetRefId( p->pRnm, c );
                    vPPis = Ga2_ManRefine( p );
                    p->timeCex += Abc_Clock() - clk2;
                    if ( vPPis == NULL )
                    {
                        if ( pPars->fVerbose )
                            Ga2_ManAbsPrintFrame( p, f, sat_solver2_nconflicts(p->pSat)-nConflsBeg, c, Abc_Clock() - clk, 1 );
                        goto finish;
                    }

                    if ( c == 0 )
                    {
//                        Ga2_ManRefinePrintPPis( p );
                        // create bookmark to be used for rollback
                        assert( nVarsOld == p->pSat->size );
                        sat_solver2_bookmark( p->pSat );
                        // start incremental proof manager
                        assert( p->pSat->pPrf2 == NULL );
                        p->pSat->pPrf2 = Prf_ManAlloc();
                        if ( p->pSat->pPrf2 )
                        {
                            p->nProofIds = 0;
                            Vec_IntFill( p->vProofIds, Gia_ManObjNum(p->pGia), -1 );
                            Prf_ManRestart( p->pSat->pPrf2, p->vProofIds, sat_solver2_nlearnts(p->pSat), Vec_IntSize(vPPis) );
                        }
                    }
                    else
                    {
                        // resize the proof logger
                        if ( p->pSat->pPrf2 )
                            Prf_ManGrow( p->pSat->pPrf2, p->nProofIds + Vec_IntSize(vPPis) );
                    }

                    Ga2_ManAddToAbs( p, vPPis );
                    Vec_IntFree( vPPis );
                    if ( pPars->fVerbose )
                        Ga2_ManAbsPrintFrame( p, f, sat_solver2_nconflicts(p->pSat)-nConflsBeg, c+1, Abc_Clock() - clk, 0 );
                    // check if the number of objects is below limit
                    if ( pPars->nRatioMin2 && Vec_IntSize(p->vAbs) >= p->nMarked * pPars->nRatioMin2 / 100 )
                    {
                        Status = l_Undef;
                        goto finish;
                    }
                    continue;
                }
                p->timeUnsat += Abc_Clock() - clk2;
                if ( Status == l_Undef ) // ran out of resources
                    goto finish;
                if ( p->pSat->nRuntimeLimit && Abc_Clock() > p->pSat->nRuntimeLimit ) // timeout
                    goto finish;
                if ( c == 0 )
                {
                    if ( f > p->pPars->iFrameProved )
                        p->pPars->nFramesNoChange++;
                    break;
                }
                if ( f > p->pPars->iFrameProved )
                    p->pPars->nFramesNoChange = 0;

                // derive the core
                assert( p->pSat->pPrf2 != NULL );
                vCore = (Vec_Int_t *)Sat_ProofCore( p->pSat );
                Prf_ManStopP( &p->pSat->pPrf2 );
                // update the SAT solver
                sat_solver2_rollback( p->pSat );
                // reduce abstraction
                Ga2_ManShrinkAbs( p, nAbs, nValues, nVarsOld );

                // purify UNSAT core
                if ( fUseSecondCore )
                {
//                    int nOldCore = Vec_IntSize(vCore);
                    // reverse the order of objects in the core
//                    Vec_IntSort( vCore, 0 );
//                    Vec_IntPrint( vCore );
                    // create bookmark to be used for rollback
                    assert( nVarsOld == p->pSat->size );
                    sat_solver2_bookmark( p->pSat );
                    // start incremental proof manager
                    assert( p->pSat->pPrf2 == NULL );
                    p->pSat->pPrf2 = Prf_ManAlloc();
                    if ( p->pSat->pPrf2 )
                    {
                        p->nProofIds = 0;
                        Vec_IntFill( p->vProofIds, Gia_ManObjNum(p->pGia), -1 );
                        Prf_ManRestart( p->pSat->pPrf2, p->vProofIds, sat_solver2_nlearnts(p->pSat), Vec_IntSize(vCore) );

                        Ga2_ManAddToAbs( p, vCore );
                        Vec_IntFree( vCore );
                    }
                    // run SAT solver
                    clk2 = Abc_Clock();
                    Status = sat_solver2_solve( p->pSat, &Lit, &Lit+1, (ABC_INT64_T)pPars->nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
                    if ( Status == l_Undef )
                        goto finish;
                    assert( Status == l_False );
                    p->timeUnsat += Abc_Clock() - clk2;

                    // derive the core
                    assert( p->pSat->pPrf2 != NULL );
                    vCore = (Vec_Int_t *)Sat_ProofCore( p->pSat );
                    Prf_ManStopP( &p->pSat->pPrf2 );
                    // update the SAT solver
                    sat_solver2_rollback( p->pSat );
                    // reduce abstraction
                    Ga2_ManShrinkAbs( p, nAbs, nValues, nVarsOld );
//                    printf( "\n%4d -> %4d\n", nOldCore, Vec_IntSize(vCore) );
                }

                Ga2_ManAddToAbs( p, vCore );
//                Ga2_ManRefinePrint( p, vCore );
                Vec_IntFree( vCore );
                break;
            }
            // remember the last proved frame
            if ( p->pPars->iFrameProved < f )
                p->pPars->iFrameProved = f;
            // print statistics
            if ( pPars->fVerbose )
                Ga2_ManAbsPrintFrame( p, f, sat_solver2_nconflicts(p->pSat)-nConflsBeg, c, Abc_Clock() - clk, 1 );
            // check if abstraction was proved
            if ( Gia_GlaProveCheck( pPars->fVerbose ) )
            {
                RetValue = 1;
                goto finish;
            }
            if ( c > 0 ) 
            {
                if ( p->pPars->fVeryVerbose )
                    Abc_Print( 1, "\n" );
                // recompute the abstraction
                Vec_IntFreeP( &pAig->vGateClasses );
                pAig->vGateClasses = Ga2_ManAbsTranslate( p );
                // check if the number of objects is below limit
                if ( pPars->nRatioMin && Vec_IntSize(p->vAbs) >= p->nMarked * pPars->nRatioMin / 100 )
                {
                    Status = l_Undef;
                    goto finish;
                }
            }
            // check the number of stable frames
            if ( p->pPars->nFramesNoChange == p->pPars->nFramesNoChangeLim )
            {
                // dump the model into file
                if ( p->pPars->fDumpVabs || p->pPars->fDumpMabs )
                {
                    char Command[1000];
                    Abc_FrameSetStatus( -1 );
                    Abc_FrameSetCex( NULL );
                    Abc_FrameSetNFrames( f );
                    sprintf( Command, "write_status %s", Extra_FileNameGenericAppend((char *)(p->pPars->pFileVabs ? p->pPars->pFileVabs : "glabs.aig"), ".status"));
                    Cmd_CommandExecute( Abc_FrameGetGlobalFrame(), Command );
                    Ga2_GlaDumpAbsracted( p, pPars->fVerbose );
                }
                // call the prover
                if ( p->pPars->fCallProver )
                {
                    // cancel old one if it is proving
                    if ( iFrameTryToProve >= 0 )
                        Gia_GlaProveCancel( pPars->fVerbose );
                    // prove new one
                    Gia_GlaProveAbsracted( pAig, pPars->fSimpProver, pPars->fVeryVerbose );
                    iFrameTryToProve = f;
                    p->nPdrCalls++;
                }
                // speak to the bridge
                if ( Abc_FrameIsBridgeMode() )
                {
                    // cancel old one if it was sent
                    if ( fOneIsSent )
                        Gia_Ga2SendCancel( p, pPars->fVerbose );
                    // send new one 
                    Gia_Ga2SendAbsracted( p, pPars->fVerbose );
                    fOneIsSent = 1;
                }
            }
            // if abstraction grew more than a certain percentage, force a restart
            if ( pPars->nRatioMax == 0 )
                continue;
            if ( c > 0 && (f > 20 || Vec_IntSize(p->vAbs) > 100) && Vec_IntSize(p->vAbs) - nAbsOld >= nAbsOld * pPars->nRatioMax / 100 )
            {
                if ( p->pPars->fVerbose )
                Abc_Print( 1, "Forcing restart because abstraction grew from %d to %d (more than %d %%).\n", 
                    nAbsOld, Vec_IntSize(p->vAbs), pPars->nRatioMax );
                break;
            }
        }
    }
finish:
    Prf_ManStopP( &p->pSat->pPrf2 );
    // cancel old one if it is proving
    if ( iFrameTryToProve >= 0 )
        Gia_GlaProveCancel( pPars->fVerbose );
    // analize the results
    if ( !p->fUseNewLine )
        Abc_Print( 1, "\n" );
    if ( RetValue == 1 )
        Abc_Print( 1, "GLA completed %d frames and proved abstraction derived in frame %d  ", p->pPars->iFrameProved+1, iFrameTryToProve );
    else if ( pAig->pCexSeq == NULL )
    {
        Vec_IntFreeP( &pAig->vGateClasses );
        pAig->vGateClasses = Ga2_ManAbsTranslate( p );
        if ( p->pPars->nTimeOut && Abc_Clock() >= p->pSat->nRuntimeLimit ) 
            Abc_Print( 1, "GLA reached timeout %d sec in frame %d with a %d-stable abstraction.    ", p->pPars->nTimeOut, p->pPars->iFrameProved+1, p->pPars->nFramesNoChange );
        else if ( pPars->nConfLimit && sat_solver2_nconflicts(p->pSat) >= pPars->nConfLimit )
            Abc_Print( 1, "GLA exceeded %d conflicts in frame %d with a %d-stable abstraction.  ", pPars->nConfLimit, p->pPars->iFrameProved+1, p->pPars->nFramesNoChange );
        else if ( pPars->nRatioMin2 && Vec_IntSize(p->vAbs) >= p->nMarked * pPars->nRatioMin2 / 100 )
            Abc_Print( 1, "GLA found that the size of abstraction exceeds %d %% in frame %d during refinement.  ", pPars->nRatioMin2, p->pPars->iFrameProved+1 );
        else if ( pPars->nRatioMin && Vec_IntSize(p->vAbs) >= p->nMarked * pPars->nRatioMin / 100 )
            Abc_Print( 1, "GLA found that the size of abstraction exceeds %d %% in frame %d.  ", pPars->nRatioMin, p->pPars->iFrameProved+1 );
        else
            Abc_Print( 1, "GLA finished %d frames and produced a %d-stable abstraction.  ", p->pPars->iFrameProved+1, p->pPars->nFramesNoChange );
        p->pPars->iFrame = p->pPars->iFrameProved;
    }
    else
    {
        if ( p->pPars->fVerbose )
            Abc_Print( 1, "\n" );
        if ( !Gia_ManVerifyCex( pAig, pAig->pCexSeq, 0 ) )
            Abc_Print( 1, "    Gia_ManPerformGlaOld(): CEX verification has failed!\n" );
        Abc_Print( 1, "True counter-example detected in frame %d.  ", f );
        p->pPars->iFrame = f - 1;
        Vec_IntFreeP( &pAig->vGateClasses );
        RetValue = 0;
    }
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( p->pPars->fVerbose )
    {
        p->timeOther = (Abc_Clock() - clk) - p->timeUnsat - p->timeSat - p->timeCex - p->timeInit;
        ABC_PRTP( "Runtime: Initializing", p->timeInit,   Abc_Clock() - clk );
        ABC_PRTP( "Runtime: Solver UNSAT", p->timeUnsat,  Abc_Clock() - clk );
        ABC_PRTP( "Runtime: Solver SAT  ", p->timeSat,    Abc_Clock() - clk );
        ABC_PRTP( "Runtime: Refinement  ", p->timeCex,    Abc_Clock() - clk );
        ABC_PRTP( "Runtime: Other       ", p->timeOther,  Abc_Clock() - clk );
        ABC_PRTP( "Runtime: TOTAL       ", Abc_Clock() - clk, Abc_Clock() - clk );
        Ga2_ManReportMemory( p );
    }
//    Ga2_ManDumpStats( p->pGia, p->pPars, p->pSat, p->pPars->iFrameProved, 0 );
    Ga2_ManStop( p );
    fflush( stdout );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

