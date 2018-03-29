/**CFile****************************************************************

  FileName    [bmcBmc3.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Simple BMC package.]

  Author      [Alan Mishchenko in collaboration with Niklas Een.]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcBmc3.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "proof/fra/fra.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"
#include "sat/satoko/satoko.h"
#include "sat/glucose/AbcGlucose.h"
#include "misc/vec/vecHsh.h"
#include "misc/vec/vecWec.h"
#include "bmc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Gia_ManBmc_t_ Gia_ManBmc_t;
struct Gia_ManBmc_t_
{
    // input/output data
    Saig_ParBmc_t *   pPars;       // parameters
    Aig_Man_t *       pAig;        // user AIG
    Vec_Ptr_t *       vCexes;      // counter-examples
    // intermediate data
    Vec_Int_t *       vMapping;    // mapping
    Vec_Int_t *       vMapRefs;    // mapping references
//    Vec_Vec_t *       vSects;      // sections
    Vec_Int_t *       vId2Num;     // number of each node 
    Vec_Ptr_t *       vTerInfo;    // ternary information
    Vec_Ptr_t *       vId2Var;     // SAT vars for each object
    Vec_Wec_t *       vVisited;    // visited nodes
    abctime *         pTime4Outs;  // timeout per output
    // hash table
    Vec_Int_t *       vData;       // storage for cuts
    Hsh_IntMan_t *    vHash;       // hash table
    Vec_Int_t *       vId2Lit;     // mapping cuts into literals
    int               nHashHit;    // hash table hits
    int               nHashMiss;   // hash table misses
    int               nBufNum;     // the number of simple nodes
    int               nDupNum;     // the number of simple nodes
    int               nUniProps;   // propagating learned clause values
    int               nLitUsed;    // used literals
    int               nLitUseless; // useless literals
    // SAT solver
    sat_solver *      pSat;        // SAT solver
    satoko_t *        pSat2;       // SAT solver
    bmcg_sat_solver * pSat3;       // SAT solver
    int               nSatVars;    // SAT variables
    int               nObjNums;    // SAT objects
    int               nWordNum;    // unsigned words for ternary simulation
    char * pSopSizes, ** pSops;    // CNF representation
};

extern int Gia_ManToBridgeResult( FILE * pFile, int Result, Abc_Cex_t * pCex, int iPoProved );

void Gia_ManReportProgress( FILE * pFile, int prop_no, int depth )
{
    extern int Gia_ManToBridgeProgress( FILE * pFile, int Size, unsigned char * pBuffer );
    char buf[100];
    sprintf(buf, "property: safe<%d>\nbug-free-depth: %d\n", prop_no, depth);
    Gia_ManToBridgeProgress(pFile, strlen(buf), (unsigned char *)buf);
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#define SAIG_TER_NON 0
#define SAIG_TER_ZER 1
#define SAIG_TER_ONE 2
#define SAIG_TER_UND 3

static inline int Saig_ManBmcSimInfoNot( int Value )
{
    if ( Value == SAIG_TER_ZER )
        return SAIG_TER_ONE;
    if ( Value == SAIG_TER_ONE )
        return SAIG_TER_ZER;
    return SAIG_TER_UND;
}

static inline int Saig_ManBmcSimInfoAnd( int Value0, int Value1 )
{
    if ( Value0 == SAIG_TER_ZER || Value1 == SAIG_TER_ZER )
        return SAIG_TER_ZER;
    if ( Value0 == SAIG_TER_ONE && Value1 == SAIG_TER_ONE )
        return SAIG_TER_ONE;
    return SAIG_TER_UND;
}

static inline int Saig_ManBmcSimInfoGet( unsigned * pInfo, Aig_Obj_t * pObj )
{
    return 3 & (pInfo[Aig_ObjId(pObj) >> 4] >> ((Aig_ObjId(pObj) & 15) << 1));
}

static inline void Saig_ManBmcSimInfoSet( unsigned * pInfo, Aig_Obj_t * pObj, int Value )
{
    assert( Value >= SAIG_TER_ZER && Value <= SAIG_TER_UND );
    Value ^= Saig_ManBmcSimInfoGet( pInfo, pObj );
    pInfo[Aig_ObjId(pObj) >> 4] ^= (Value << ((Aig_ObjId(pObj) & 15) << 1));
}

static inline int Saig_ManBmcSimInfoClear( unsigned * pInfo, Aig_Obj_t * pObj )
{
    int Value = Saig_ManBmcSimInfoGet( pInfo, pObj );
    pInfo[Aig_ObjId(pObj) >> 4] ^= (Value << ((Aig_ObjId(pObj) & 15) << 1));
    return Value;
}

/**Function*************************************************************

  Synopsis    [Returns the number of LIs with binary ternary info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManBmcTerSimCount01( Aig_Man_t * p, unsigned * pInfo )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    if ( pInfo == NULL )
        return Saig_ManRegNum(p);
    Saig_ManForEachLi( p, pObj, i )
        if ( !Aig_ObjIsConst1(Aig_ObjFanin0(pObj)) )
            Counter += (Saig_ManBmcSimInfoGet(pInfo, pObj) != SAIG_TER_UND);
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Performs ternary simulation of one frame.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned * Saig_ManBmcTerSimOne( Aig_Man_t * p, unsigned * pPrev )
{
    Aig_Obj_t * pObj, * pObjLi;
    unsigned * pInfo;
    int i, Val0, Val1;
    pInfo = ABC_CALLOC( unsigned, Abc_BitWordNum(2 * Aig_ManObjNumMax(p)) );
    Saig_ManBmcSimInfoSet( pInfo, Aig_ManConst1(p), SAIG_TER_ONE );
    Saig_ManForEachPi( p, pObj, i )
        Saig_ManBmcSimInfoSet( pInfo, pObj, SAIG_TER_UND );
    if ( pPrev == NULL )
    {
        Saig_ManForEachLo( p, pObj, i )
            Saig_ManBmcSimInfoSet( pInfo, pObj, SAIG_TER_ZER );
    }
    else
    {
        Saig_ManForEachLiLo( p, pObjLi, pObj, i )
            Saig_ManBmcSimInfoSet( pInfo, pObj, Saig_ManBmcSimInfoGet(pPrev, pObjLi) );
    }
    Aig_ManForEachNode( p, pObj, i )
    {
        Val0 = Saig_ManBmcSimInfoGet( pInfo, Aig_ObjFanin0(pObj) );
        Val1 = Saig_ManBmcSimInfoGet( pInfo, Aig_ObjFanin1(pObj) );
        if ( Aig_ObjFaninC0(pObj) )
            Val0 = Saig_ManBmcSimInfoNot( Val0 );
        if ( Aig_ObjFaninC1(pObj) )
            Val1 = Saig_ManBmcSimInfoNot( Val1 );
        Saig_ManBmcSimInfoSet( pInfo, pObj, Saig_ManBmcSimInfoAnd(Val0, Val1) );
    }
    Aig_ManForEachCo( p, pObj, i )
    {
        Val0 = Saig_ManBmcSimInfoGet( pInfo, Aig_ObjFanin0(pObj) );
        if ( Aig_ObjFaninC0(pObj) )
            Val0 = Saig_ManBmcSimInfoNot( Val0 );
        Saig_ManBmcSimInfoSet( pInfo, pObj, Val0 );
    }
    return pInfo;    
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes and PIs in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_ManBmcTerSim( Aig_Man_t * p )
{
    Vec_Ptr_t * vInfos;
    unsigned * pInfo = NULL;
    int i, TerPrev = ABC_INFINITY, TerCur, CountIncrease = 0;
    vInfos = Vec_PtrAlloc( 100 );
    for ( i = 0; i < 1000 && CountIncrease < 5 && TerPrev > 0; i++ )
    {
        TerCur = Saig_ManBmcTerSimCount01( p, pInfo );
        if ( TerCur >= TerPrev )
            CountIncrease++;
        TerPrev = TerCur;
        pInfo = Saig_ManBmcTerSimOne( p, pInfo );
        Vec_PtrPush( vInfos, pInfo );
    }
    return vInfos;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManBmcTerSimTest( Aig_Man_t * p )
{
    Vec_Ptr_t * vInfos;
    unsigned * pInfo;
    int i;
    vInfos = Saig_ManBmcTerSim( p );
    Vec_PtrForEachEntry( unsigned *, vInfos, pInfo, i )
        Abc_Print( 1, "%d=%d ", i, Saig_ManBmcTerSimCount01(p, pInfo) );
    Abc_Print( 1, "\n" );
    Vec_PtrFreeFree( vInfos );
}



/**Function*************************************************************

  Synopsis    [Count the number of non-ternary per frame.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManBmcCountNonternary_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vInfos, unsigned * pInfo, int f, int * pCounter )
{ 
    int Value = Saig_ManBmcSimInfoClear( pInfo, pObj );
    if ( Value == SAIG_TER_NON )
        return 0;
    assert( f >= 0 );
    pCounter[f] += (Value == SAIG_TER_UND);
    if ( Saig_ObjIsPi(p, pObj) || (f == 0 && Saig_ObjIsLo(p, pObj)) || Aig_ObjIsConst1(pObj) )
        return 0;
    if ( Saig_ObjIsLi(p, pObj) )
        return Saig_ManBmcCountNonternary_rec( p, Aig_ObjFanin0(pObj), vInfos, pInfo, f, pCounter );
    if ( Saig_ObjIsLo(p, pObj) )
        return Saig_ManBmcCountNonternary_rec( p, Saig_ObjLoToLi(p, pObj), vInfos, (unsigned *)Vec_PtrEntry(vInfos, f-1), f-1, pCounter );
    assert( Aig_ObjIsNode(pObj) );
    Saig_ManBmcCountNonternary_rec( p, Aig_ObjFanin0(pObj), vInfos, pInfo, f, pCounter );
    Saig_ManBmcCountNonternary_rec( p, Aig_ObjFanin1(pObj), vInfos, pInfo, f, pCounter );
    return 0;
}
void Saig_ManBmcCountNonternary( Aig_Man_t * p, Vec_Ptr_t * vInfos, int iFrame )
{
    int i, * pCounters = ABC_CALLOC( int, iFrame + 1 );
    unsigned * pInfo = (unsigned *)Vec_PtrEntry(vInfos, iFrame);
    assert( Saig_ManBmcSimInfoGet( pInfo, Aig_ManCo(p, 0) ) == SAIG_TER_UND );
    Saig_ManBmcCountNonternary_rec( p, Aig_ObjFanin0(Aig_ManCo(p, 0)), vInfos, pInfo, iFrame, pCounters );
    for ( i = 0; i <= iFrame; i++ )
        Abc_Print( 1, "%d=%d ", i, pCounters[i] );
    Abc_Print( 1, "\n" );
    ABC_FREE( pCounters );
}


/**Function*************************************************************

  Synopsis    [Returns the number of POs with binary ternary info.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManBmcTerSimCount01Po( Aig_Man_t * p, unsigned * pInfo )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    Saig_ManForEachPo( p, pObj, i )
        Counter += (Saig_ManBmcSimInfoGet(pInfo, pObj) != SAIG_TER_UND);
    return Counter;
}
Vec_Ptr_t * Saig_ManBmcTerSimPo( Aig_Man_t * p )
{
    Vec_Ptr_t * vInfos;
    unsigned * pInfo = NULL;
    int i, nPoBin;
    vInfos = Vec_PtrAlloc( 100 );
    for ( i = 0; ; i++ )
    {
        if ( i % 100 == 0 )
            Abc_Print( 1, "Frame %5d\n", i );
        pInfo = Saig_ManBmcTerSimOne( p, pInfo );
        Vec_PtrPush( vInfos, pInfo );
        nPoBin = Saig_ManBmcTerSimCount01Po( p, pInfo );
        if ( nPoBin < Saig_ManPoNum(p) )
            break;
    }
    Abc_Print( 1, "Detected terminary PO in frame %d.\n", i );
    Saig_ManBmcCountNonternary( p, vInfos, i );
    return vInfos;
}
void Saig_ManBmcTerSimTestPo( Aig_Man_t * p )
{
    Vec_Ptr_t * vInfos;
    vInfos = Saig_ManBmcTerSimPo( p );
    Vec_PtrFreeFree( vInfos );
}




/**Function*************************************************************

  Synopsis    [Collects internal nodes in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManBmcDfs_rec( Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Ptr_t * vNodes )
{
    assert( !Aig_IsComplement(pObj) );
    if ( Aig_ObjIsTravIdCurrent(p, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p, pObj);
    if ( Aig_ObjIsNode(pObj) )
    {
        Saig_ManBmcDfs_rec( p, Aig_ObjFanin0(pObj), vNodes );
        Saig_ManBmcDfs_rec( p, Aig_ObjFanin1(pObj), vNodes );
    }
    Vec_PtrPush( vNodes, pObj );
}

/**Function*************************************************************

  Synopsis    [Collects internal nodes and PIs in the DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_ManBmcDfsNodes( Aig_Man_t * p, Vec_Ptr_t * vRoots )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i;
    vNodes = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vRoots, pObj, i )
        Saig_ManBmcDfs_rec( p, Aig_ObjFanin0(pObj), vNodes );
    return vNodes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Vec_t * Saig_ManBmcSections( Aig_Man_t * p )
{
    Vec_Ptr_t * vSects, * vRoots, * vCone;
    Aig_Obj_t * pObj, * pObjPo;
    int i;
    Aig_ManIncrementTravId( p );
    Aig_ObjSetTravIdCurrent( p, Aig_ManConst1(p) );
    // start the roots
    vRoots = Vec_PtrAlloc( 1000 );
    Saig_ManForEachPo( p, pObjPo, i )
    {
        Aig_ObjSetTravIdCurrent( p, pObjPo );
        Vec_PtrPush( vRoots, pObjPo );
    }
    // compute the cones
    vSects = Vec_PtrAlloc( 20 );
    while ( Vec_PtrSize(vRoots) > 0 )
    {
        vCone = Saig_ManBmcDfsNodes( p, vRoots );
        Vec_PtrPush( vSects, vCone );
        // get the next set of roots
        Vec_PtrClear( vRoots );
        Vec_PtrForEachEntry( Aig_Obj_t *, vCone, pObj, i )
        {
            if ( !Saig_ObjIsLo(p, pObj) )
                continue;
            pObjPo = Saig_ObjLoToLi( p, pObj );
            if ( Aig_ObjIsTravIdCurrent(p, pObjPo) )
                continue;
            Aig_ObjSetTravIdCurrent( p, pObjPo );
            Vec_PtrPush( vRoots, pObjPo );
        }
    }
    Vec_PtrFree( vRoots );
    return (Vec_Vec_t *)vSects;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManBmcSectionsTest( Aig_Man_t * p )
{
    Vec_Vec_t * vSects;
    Vec_Ptr_t * vOne;
    int i;
    vSects = Saig_ManBmcSections( p );
    Vec_VecForEachLevel( vSects, vOne, i )
        Abc_Print( 1, "%d=%d ", i, Vec_PtrSize(vOne) );
    Abc_Print( 1, "\n" );
    Vec_VecFree( vSects );
}



/**Function*************************************************************

  Synopsis    [Collects the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManBmcSupergate_rec( Aig_Obj_t * pObj, Vec_Ptr_t * vSuper )
{
    // if the new node is complemented or a PI, another gate begins
    if ( Aig_IsComplement(pObj) || Aig_ObjIsCi(pObj) ) // || (Aig_ObjRefs(pObj) > 1) )
    {
        Vec_PtrPushUnique( vSuper, Aig_Regular(pObj) );
        return;
    }
    // go through the branches
    Saig_ManBmcSupergate_rec( Aig_ObjChild0(pObj), vSuper );
    Saig_ManBmcSupergate_rec( Aig_ObjChild1(pObj), vSuper );
}

/**Function*************************************************************

  Synopsis    [Collect the topmost supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_ManBmcSupergate( Aig_Man_t * p, int iPo )
{
    Vec_Ptr_t * vSuper;
    Aig_Obj_t * pObj;
    vSuper = Vec_PtrAlloc( 10 );
    pObj = Aig_ManCo( p, iPo );
    pObj = Aig_ObjChild0( pObj );
    if ( !Aig_IsComplement(pObj) )
    {
        Vec_PtrPush( vSuper, pObj );
        return vSuper;
    }
    pObj = Aig_Regular( pObj );
    if ( !Aig_ObjIsNode(pObj) )
    {
        Vec_PtrPush( vSuper, pObj );
        return vSuper;
    }
    Saig_ManBmcSupergate_rec( Aig_ObjChild0(pObj), vSuper );
    Saig_ManBmcSupergate_rec( Aig_ObjChild1(pObj), vSuper );
    return vSuper;
}

/**Function*************************************************************

  Synopsis    [Returns the number of nodes with ref counter more than 1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManBmcCountRefed( Aig_Man_t * p, Vec_Ptr_t * vSuper )
{
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    Vec_PtrForEachEntry( Aig_Obj_t *, vSuper, pObj, i )
    {
        assert( !Aig_IsComplement(pObj) );
        Counter += (Aig_ObjRefs(pObj) > 1);
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManBmcSupergateTest( Aig_Man_t * p )
{
    Vec_Ptr_t * vSuper;
    Aig_Obj_t * pObj;
    int i;
    Abc_Print( 1, "Supergates: " );
    Saig_ManForEachPo( p, pObj, i )
    {
        vSuper = Saig_ManBmcSupergate( p, i );
        Abc_Print( 1, "%d=%d(%d) ", i, Vec_PtrSize(vSuper), Saig_ManBmcCountRefed(p, vSuper) );
        Vec_PtrFree( vSuper );
    }
    Abc_Print( 1, "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManBmcWriteBlif( Aig_Man_t * p, Vec_Int_t * vMapping, char * pFileName )
{
    FILE * pFile;
    char * pSopSizes, ** pSops;
    Aig_Obj_t * pObj;
    char Vals[4];
    int i, k, b, iFan, iTruth, * pData;
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        Abc_Print( 1, "Cannot open file %s\n", pFileName );
        return;
    }
    fprintf( pFile, ".model test\n" );
    fprintf( pFile, ".inputs" );
    Aig_ManForEachCi( p, pObj, i )
        fprintf( pFile, " n%d", Aig_ObjId(pObj) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".outputs" );
    Aig_ManForEachCo( p, pObj, i )
        fprintf( pFile, " n%d", Aig_ObjId(pObj) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".names" );
    fprintf( pFile, " n%d\n", Aig_ObjId(Aig_ManConst1(p)) );
    fprintf( pFile, " 1\n" );

    Cnf_ReadMsops( &pSopSizes, &pSops );
    Aig_ManForEachNode( p, pObj, i )
    {
        if ( Vec_IntEntry(vMapping, i) == 0 )
            continue;
        pData = Vec_IntEntryP( vMapping, Vec_IntEntry(vMapping, i) );
        fprintf( pFile, ".names" );
        for ( iFan = 0; iFan < 4; iFan++ )
            if ( pData[iFan+1] >= 0 )
                fprintf( pFile, " n%d", pData[iFan+1] );
            else
                break;
        fprintf( pFile, " n%d\n", i );
        // write SOP
        iTruth = pData[0] & 0xffff;
        for ( k = 0; k < pSopSizes[iTruth]; k++ )
        {
            int Lit = pSops[iTruth][k];
            for ( b = 3; b >= 0; b-- )
            {
                if ( Lit % 3 == 0 )
                    Vals[b] = '0';
                else if ( Lit % 3 == 1 )
                    Vals[b] = '1';
                else
                    Vals[b] = '-';
                Lit = Lit / 3;
            }
            for ( b = 0; b < iFan; b++ )
                fprintf( pFile, "%c", Vals[b] );
            fprintf( pFile, " 1\n" );
        }
    }
    free( pSopSizes );
    free( pSops[1] );
    free( pSops );

    Aig_ManForEachCo( p, pObj, i )
    {
        fprintf( pFile, ".names" );
        fprintf( pFile, " n%d", Aig_ObjId(Aig_ObjFanin0(pObj)) );
        fprintf( pFile, " n%d\n", Aig_ObjId(pObj) );
        fprintf( pFile, "%d 1\n", !Aig_ObjFaninC0(pObj) );
    }
    fprintf( pFile, ".end\n" );
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManBmcMappingTest( Aig_Man_t * p )
{
    Vec_Int_t * vMapping;
    vMapping = Cnf_DeriveMappingArray( p );
    Saig_ManBmcWriteBlif( p, vMapping, "test.blif" );
    Vec_IntFree( vMapping );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManBmcComputeMappingRefs( Aig_Man_t * p, Vec_Int_t * vMap )
{
    Vec_Int_t * vRefs;
    Aig_Obj_t * pObj;
    int i, iFan, * pData;
    vRefs = Vec_IntStart( Aig_ManObjNumMax(p) );
    Aig_ManForEachCo( p, pObj, i )
        Vec_IntAddToEntry( vRefs, Aig_ObjFaninId0(pObj), 1 );
    Aig_ManForEachNode( p, pObj, i )
    {
        if ( Vec_IntEntry(vMap, i) == 0 )
            continue;
        pData = Vec_IntEntryP( vMap, Vec_IntEntry(vMap, i) );
        for ( iFan = 0; iFan < 4; iFan++ )
            if ( pData[iFan+1] >= 0 )
                Vec_IntAddToEntry( vRefs, pData[iFan+1], 1 );
    }
    return vRefs;
}

/**Function*************************************************************

  Synopsis    [Create manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_ManBmc_t * Saig_Bmc3ManStart( Aig_Man_t * pAig, int nTimeOutOne, int nConfLimit, int fUseSatoko, int fUseGlucose )
{
    Gia_ManBmc_t * p;
    Aig_Obj_t * pObj;
    int i;
//    assert( Aig_ManRegNum(pAig) > 0 );
    p = ABC_CALLOC( Gia_ManBmc_t, 1 );
    p->pAig = pAig;
    // create mapping
    p->vMapping = Cnf_DeriveMappingArray( pAig );
    p->vMapRefs = Saig_ManBmcComputeMappingRefs( pAig, p->vMapping );
    // create sections
//    p->vSects = Saig_ManBmcSections( pAig );
    // map object IDs into their numbers and section numbers
    p->nObjNums = 0;
    p->vId2Num  = Vec_IntStartFull( Aig_ManObjNumMax(pAig) );
    Vec_IntWriteEntry( p->vId2Num,  Aig_ObjId(Aig_ManConst1(pAig)), p->nObjNums++ );
    Aig_ManForEachCi( pAig, pObj, i )
        Vec_IntWriteEntry( p->vId2Num,  Aig_ObjId(pObj), p->nObjNums++ );
    Aig_ManForEachNode( pAig, pObj, i )
        if ( Vec_IntEntry(p->vMapping, Aig_ObjId(pObj)) > 0 )
            Vec_IntWriteEntry( p->vId2Num,  Aig_ObjId(pObj), p->nObjNums++ );
    Aig_ManForEachCo( pAig, pObj, i )
        Vec_IntWriteEntry( p->vId2Num,  Aig_ObjId(pObj), p->nObjNums++ );
    p->vId2Var  = Vec_PtrAlloc( 100 );
    p->vTerInfo = Vec_PtrAlloc( 100 );
    p->vVisited = Vec_WecAlloc( 100 );
    // create solver
    p->nSatVars = 1;
    if ( fUseSatoko )
    {
        satoko_opts_t opts;
        satoko_default_opts(&opts);
        opts.conf_limit = nConfLimit;
        p->pSat2 = satoko_create();  
        satoko_configure(p->pSat2, &opts);
        satoko_setnvars(p->pSat2, 1000);
    }
    else if ( fUseGlucose )
    {
        //opts.conf_limit = nConfLimit;
        p->pSat3 = bmcg_sat_solver_start();  
        for ( i = 0; i < 1000; i++ )
            bmcg_sat_solver_addvar( p->pSat3 );
    }
    else
    {
        p->pSat  = sat_solver_new();
        sat_solver_setnvars(p->pSat, 1000);
    }
    Cnf_ReadMsops( &p->pSopSizes, &p->pSops );
    // terminary simulation 
    p->nWordNum = Abc_BitWordNum( 2 * Aig_ManObjNumMax(pAig) );
    // hash table
    p->vData = Vec_IntAlloc( 5 * 10000 );
    p->vHash = Hsh_IntManStart( p->vData, 5, 10000 );
    p->vId2Lit = Vec_IntAlloc( 10000 );
    // time spent on each outputs
    if ( nTimeOutOne )
    {
        p->pTime4Outs = ABC_ALLOC( abctime, Saig_ManPoNum(pAig) );
        for ( i = 0; i < Saig_ManPoNum(pAig); i++ )
            p->pTime4Outs[i] = nTimeOutOne * CLOCKS_PER_SEC / 1000 + 1;
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Delete manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_Bmc3ManStop( Gia_ManBmc_t * p )
{
    if ( p->pPars->fVerbose )
    {
        int nUsedVars = p->pSat ? sat_solver_count_usedvars(p->pSat) : 0;
        Abc_Print( 1, "LStart(P) = %d  LDelta(Q) = %d  LRatio(R) = %d  ReduceDB = %d  Vars = %d  Used = %d (%.2f %%)\n", 
            p->pSat ? p->pSat->nLearntStart     : 0, 
            p->pSat ? p->pSat->nLearntDelta     : 0, 
            p->pSat ? p->pSat->nLearntRatio     : 0, 
            p->pSat ? p->pSat->nDBreduces       : 0, 
            p->pSat ? sat_solver_nvars(p->pSat) : p->pSat3 ? bmcg_sat_solver_varnum(p->pSat3) : satoko_varnum(p->pSat2), 
            nUsedVars, 
            100.0*nUsedVars/(p->pSat ? sat_solver_nvars(p->pSat) : p->pSat3 ? bmcg_sat_solver_varnum(p->pSat3) : satoko_varnum(p->pSat2)) );
        Abc_Print( 1, "Buffs = %d. Dups = %d.   Hash hits = %d.  Hash misses = %d.  UniProps = %d.\n", 
            p->nBufNum, p->nDupNum, p->nHashHit, p->nHashMiss, p->nUniProps );
    }
//    Aig_ManCleanMarkA( p->pAig );
    if ( p->vCexes )
    {
        assert( p->pAig->vSeqModelVec == NULL );
        p->pAig->vSeqModelVec = p->vCexes;
        p->vCexes = NULL;
    }
//    Vec_PtrFreeFree( p->vCexes );
    Vec_WecFree( p->vVisited );
    Vec_IntFree( p->vMapping );
    Vec_IntFree( p->vMapRefs );
//    Vec_VecFree( p->vSects );
    Vec_IntFree( p->vId2Num );
    Vec_VecFree( (Vec_Vec_t *)p->vId2Var );
    Vec_PtrFreeFree( p->vTerInfo );
    if ( p->pSat )  sat_solver_delete( p->pSat );
    if ( p->pSat2 ) satoko_destroy( p->pSat2 );
    if ( p->pSat3 ) bmcg_sat_solver_stop( p->pSat3 );
    ABC_FREE( p->pTime4Outs );
    Vec_IntFree( p->vData );
    Hsh_IntManStop( p->vHash );
    Vec_IntFree( p->vId2Lit );
    ABC_FREE( p->pSopSizes );
    ABC_FREE( p->pSops[1] );
    ABC_FREE( p->pSops );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int * Saig_ManBmcMapping( Gia_ManBmc_t * p, Aig_Obj_t * pObj )
{
    if ( Vec_IntEntry(p->vMapping, Aig_ObjId(pObj)) == 0 )
        return NULL;
    return Vec_IntEntryP( p->vMapping, Vec_IntEntry(p->vMapping, Aig_ObjId(pObj)) );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Saig_ManBmcLiteral( Gia_ManBmc_t * p, Aig_Obj_t * pObj, int iFrame )
{
    Vec_Int_t * vFrame;
    int ObjNum;
    assert( !Aig_ObjIsNode(pObj) || Saig_ManBmcMapping(p, pObj) );
    ObjNum  = Vec_IntEntry( p->vId2Num, Aig_ObjId(pObj) );
    assert( ObjNum >= 0 );
    vFrame  = (Vec_Int_t *)Vec_PtrEntry( p->vId2Var, iFrame );
    assert( vFrame != NULL );
    return Vec_IntEntry( vFrame, ObjNum );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Saig_ManBmcSetLiteral( Gia_ManBmc_t * p, Aig_Obj_t * pObj, int iFrame, int iLit )
{
    Vec_Int_t * vFrame;
    int ObjNum;
    assert( !Aig_ObjIsNode(pObj) || Saig_ManBmcMapping(p, pObj) );
    ObjNum  = Vec_IntEntry( p->vId2Num, Aig_ObjId(pObj) );
    vFrame  = (Vec_Int_t *)Vec_PtrEntry( p->vId2Var, iFrame );
    Vec_IntWriteEntry( vFrame, ObjNum, iLit );
/*
    if ( Vec_IntEntry( p->vMapRefs, Aig_ObjId(pObj) ) > 1 )
        p->nLitUsed++;
    else
        p->nLitUseless++;
*/
    return iLit;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Saig_ManBmcCof0( int t, int v )
{
    static int s_Truth[4] = { 0xAAAA, 0xCCCC, 0xF0F0, 0xFF00 };
    return 0xffff & ((t & ~s_Truth[v]) | ((t & ~s_Truth[v]) << (1<<v)));
}
static inline int Saig_ManBmcCof1( int t, int v )
{
    static int s_Truth[4] = { 0xAAAA, 0xCCCC, 0xF0F0, 0xFF00 };
    return 0xffff & ((t & s_Truth[v]) | ((t & s_Truth[v]) >> (1<<v)));
}
static inline int Saig_ManBmcCofEqual( int t, int v )
{
    assert( v >= 0 && v <= 3 );
    if ( v == 0 )
        return ((t & 0xAAAA) >> 1) == (t & 0x5555);
    if ( v == 1 )
        return ((t & 0xCCCC) >> 2) == (t & 0x3333);
    if ( v == 2 )
        return ((t & 0xF0F0) >> 4) == (t & 0x0F0F);
    if ( v == 3 )
        return ((t & 0xFF00) >> 8) == (t & 0x00FF);
    return -1;
}

/**Function*************************************************************

  Synopsis    [Derives CNF for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Saig_ManBmcReduceTruth( int uTruth, int Lits[] )
{
    int v;
    for ( v = 0; v < 4; v++ )
        if ( Lits[v] == 0 )
        {
            uTruth = Saig_ManBmcCof0(uTruth, v);
            Lits[v] = -1;
        }
        else if ( Lits[v] == 1 )
        {
            uTruth = Saig_ManBmcCof1(uTruth, v);
            Lits[v] = -1;
        }
    for ( v = 0; v < 4; v++ )
        if ( Lits[v] == -1 )
            assert( Saig_ManBmcCofEqual(uTruth, v) );
        else if ( Saig_ManBmcCofEqual(uTruth, v) )
            Lits[v] = -1;
    return uTruth;
}

/**Function*************************************************************

  Synopsis    [Derives CNF for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Saig_ManBmcAddClauses( Gia_ManBmc_t * p, int uTruth, int Lits[], int iLitOut )
{
    int i, k, b, CutLit, nClaLits, ClaLits[5];
    assert( uTruth > 0 && uTruth < 0xffff );
    // write positive/negative polarity
    for ( i = 0; i < 2; i++ )
    {
        if ( i )
            uTruth = 0xffff & ~uTruth;
//        Extra_PrintBinary( stdout, &uTruth, 16 ); Abc_Print( 1, "\n" );
        for ( k = 0; k < p->pSopSizes[uTruth]; k++ )
        {
            nClaLits = 0;
            ClaLits[nClaLits++] = i ? lit_neg(iLitOut) : iLitOut;
            CutLit = p->pSops[uTruth][k];
            for ( b = 3; b >= 0; b-- )
            {
                if ( CutLit % 3 == 0 ) // value 0 --> write positive literal
                {
                    assert( Lits[b] > 1 );
                    ClaLits[nClaLits++] = Lits[b];
                }
                else if ( CutLit % 3 == 1 ) // value 1 --> write negative literal
                {
                    assert( Lits[b] > 1 );
                    ClaLits[nClaLits++] = lit_neg(Lits[b]);
                }
                CutLit = CutLit / 3;
            }
            if ( p->pSat2 )
            {
                if ( !satoko_add_clause( p->pSat2, ClaLits, nClaLits ) )
                    assert( 0 );
            }
            else if ( p->pSat3 )
            {
                if ( !bmcg_sat_solver_addclause( p->pSat3, ClaLits, nClaLits ) )
                    assert( 0 );
            }
            else
            {
                if ( !sat_solver_addclause( p->pSat, ClaLits, ClaLits+nClaLits ) )
                    assert( 0 );
            }
        }
    }
}

/**Function*************************************************************

  Synopsis    [Derives CNF for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManBmcCreateCnf_rec( Gia_ManBmc_t * p, Aig_Obj_t * pObj, int iFrame )
{
    extern unsigned Dar_CutSortVars( unsigned uTruth, int * pVars );
    int * pMapping, i, iLit, Lits[5], uTruth;
    iLit = Saig_ManBmcLiteral( p, pObj, iFrame );
    if ( iLit != ~0 )
        return iLit; 
    assert( iFrame >= 0 );
    if ( Aig_ObjIsCi(pObj) )
    {
        if ( Saig_ObjIsPi(p->pAig, pObj) )
            iLit = toLit( p->nSatVars++ );
        else
            iLit = Saig_ManBmcCreateCnf_rec( p, Saig_ObjLoToLi(p->pAig, pObj), iFrame-1 );
        return Saig_ManBmcSetLiteral( p, pObj, iFrame, iLit );
    }
    if ( Aig_ObjIsCo(pObj) )
    {
        iLit = Saig_ManBmcCreateCnf_rec( p, Aig_ObjFanin0(pObj), iFrame );
        if ( Aig_ObjFaninC0(pObj) )
            iLit = lit_neg(iLit);
        return Saig_ManBmcSetLiteral( p, pObj, iFrame, iLit );
    }
    assert( Aig_ObjIsNode(pObj) );
    pMapping = Saig_ManBmcMapping( p, pObj );
    for ( i = 0; i < 4; i++ )
        if ( pMapping[i+1] == -1 )
            Lits[i] = -1;
        else
            Lits[i] = Saig_ManBmcCreateCnf_rec( p, Aig_ManObj(p->pAig, pMapping[i+1]), iFrame );
    uTruth = 0xffff & (unsigned)pMapping[0];
    // propagate constants
    uTruth = Saig_ManBmcReduceTruth( uTruth, Lits );
    if ( uTruth == 0 || uTruth == 0xffff )
    {
        iLit = (uTruth == 0xffff);
        return Saig_ManBmcSetLiteral( p, pObj, iFrame, iLit );
    }
    // canonicize inputs
    uTruth = Dar_CutSortVars( uTruth, Lits );
    assert( uTruth != 0 && uTruth != 0xffff );
    if ( uTruth == 0xAAAA || uTruth == 0x5555 )
    {
        iLit = Abc_LitNotCond( Lits[0], uTruth == 0x5555 );
        p->nBufNum++;
    }
    else 
    {
        int iEntry, iRes;
        int fCompl = (uTruth & 1);
        Lits[4] = (uTruth & 1) ? 0xffff & ~uTruth : uTruth;
        iEntry = Vec_IntSize(p->vData) / 5;
        assert( iEntry * 5 == Vec_IntSize(p->vData) );
        for ( i = 0; i < 5; i++ )
            Vec_IntPush( p->vData, Lits[i] );
        iRes = Hsh_IntManAdd( p->vHash, iEntry );
        if ( iRes == iEntry )
        {
            iLit = toLit( p->nSatVars++ );
            Saig_ManBmcAddClauses( p, Lits[4], Lits, iLit );
            assert( iEntry == Vec_IntSize(p->vId2Lit) );
            Vec_IntPush( p->vId2Lit, iLit );
            p->nHashMiss++;
        }
        else
        {
            iLit = Vec_IntEntry( p->vId2Lit, iRes );
            Vec_IntShrink( p->vData, Vec_IntSize(p->vData) - 5 );
            p->nHashHit++;
        }
        iLit = Abc_LitNotCond( iLit, fCompl );
    }
    return Saig_ManBmcSetLiteral( p, pObj, iFrame, iLit );
}


/**Function*************************************************************

  Synopsis    [Derives CNF for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManBmcCreateCnf_iter( Gia_ManBmc_t * p, Aig_Obj_t * pObj, int iFrame, Vec_Int_t * vVisit )
{
    if ( Saig_ManBmcLiteral( p, pObj, iFrame ) != ~0 )
        return; 
    if ( Aig_ObjIsTravIdCurrent(p->pAig, pObj) )
        return;
    Aig_ObjSetTravIdCurrent(p->pAig, pObj);
    if ( Aig_ObjIsCi(pObj) )
    {
        if ( Saig_ObjIsLo(p->pAig, pObj) )
            Vec_IntPush( vVisit, Saig_ObjLoToLi(p->pAig, pObj)->Id );
        return;
    }
    if ( Aig_ObjIsCo(pObj) )
    {
        Saig_ManBmcCreateCnf_iter( p, Aig_ObjFanin0(pObj), iFrame, vVisit );
        return;
    }
    else
    {
        int * pMapping, i;
        assert( Aig_ObjIsNode(pObj) );
        pMapping = Saig_ManBmcMapping( p, pObj );
        for ( i = 0; i < 4; i++ )
            if ( pMapping[i+1] != -1 )
                Saig_ManBmcCreateCnf_iter( p, Aig_ManObj(p->pAig, pMapping[i+1]), iFrame, vVisit );
    }
}

/**Function*************************************************************

  Synopsis    [Recursively performs terminary simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManBmcRunTerSim_rec( Gia_ManBmc_t * p, Aig_Obj_t * pObj, int iFrame )
{
    unsigned * pInfo = (unsigned *)Vec_PtrEntry( p->vTerInfo, iFrame );
    int Val0, Val1, Value = Saig_ManBmcSimInfoGet( pInfo, pObj );
    if ( Value != SAIG_TER_NON )
    {
/*
        // check the value of this literal in the SAT solver
        if ( Value == SAIG_TER_UND && Saig_ManBmcMapping(p, pObj) )
        {
            int Lit = Saig_ManBmcLiteral( p, pObj, iFrame );
            if ( Lit >= 0  )
            {
                assert( Lit >= 2 );
                if ( p->pSat->assigns[Abc_Lit2Var(Lit)] < 2 )
                {
                    p->nUniProps++;
                    if ( Abc_LitIsCompl(Lit) ^ (p->pSat->assigns[Abc_Lit2Var(Lit)] == 0) )
                        Value = SAIG_TER_ONE;
                    else
                        Value = SAIG_TER_ZER;

//                    Value = SAIG_TER_UND;  // disable!

                    // use the new value
                    Saig_ManBmcSimInfoSet( pInfo, pObj, Value );
                    // transfer to the unrolling
                    if ( Value != SAIG_TER_UND )
                        Saig_ManBmcSetLiteral( p, pObj, iFrame, (int)(Value == SAIG_TER_ONE) );
                }
            }
        }
*/
        return Value;
    }
    if ( Aig_ObjIsCo(pObj) )
    {
        Value = Saig_ManBmcRunTerSim_rec( p, Aig_ObjFanin0(pObj), iFrame );
        if ( Aig_ObjFaninC0(pObj) )
            Value = Saig_ManBmcSimInfoNot( Value );
    }
    else if ( Saig_ObjIsLo(p->pAig, pObj) )
    {
        assert( iFrame > 0 );
        Value = Saig_ManBmcRunTerSim_rec( p, Saig_ObjLoToLi(p->pAig, pObj), iFrame - 1 );
    }
    else if ( Aig_ObjIsNode(pObj) )
    {
        Val0 = Saig_ManBmcRunTerSim_rec( p, Aig_ObjFanin0(pObj), iFrame  );
        Val1 = Saig_ManBmcRunTerSim_rec( p, Aig_ObjFanin1(pObj), iFrame  );
        if ( Aig_ObjFaninC0(pObj) )
            Val0 = Saig_ManBmcSimInfoNot( Val0 );
        if ( Aig_ObjFaninC1(pObj) )
            Val1 = Saig_ManBmcSimInfoNot( Val1 );
        Value = Saig_ManBmcSimInfoAnd( Val0, Val1 );
    }
    else assert( 0 );
    Saig_ManBmcSimInfoSet( pInfo, pObj, Value );
    // transfer to the unrolling
    if ( Saig_ManBmcMapping(p, pObj) && Value != SAIG_TER_UND )
        Saig_ManBmcSetLiteral( p, pObj, iFrame, (int)(Value == SAIG_TER_ONE) );
    return Value;
}

/**Function*************************************************************

  Synopsis    [Derives CNF for one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManBmcCreateCnf( Gia_ManBmc_t * p, Aig_Obj_t * pObj, int iFrame )
{
    Vec_Int_t * vVisit, * vVisit2;
    Aig_Obj_t * pTemp;
    int Lit, f, i;
    // perform ternary simulation
    int Value = Saig_ManBmcRunTerSim_rec( p, pObj, iFrame );
    if ( Value != SAIG_TER_UND )
        return (int)(Value == SAIG_TER_ONE);
    // construct CNF if value is ternary
//    Lit = Saig_ManBmcCreateCnf_rec( p, pObj, iFrame );
    Vec_WecClear( p->vVisited );
    vVisit = Vec_WecPushLevel( p->vVisited );
    Vec_IntPush( vVisit, Aig_ObjId(pObj) );
    for ( f = iFrame; f >= 0; f-- )
    {
        Aig_ManIncrementTravId( p->pAig );
        vVisit2 = Vec_WecPushLevel( p->vVisited );
        vVisit = Vec_WecEntry( p->vVisited, Vec_WecSize(p->vVisited)-2 );
        Aig_ManForEachObjVec( vVisit, p->pAig, pTemp, i )
            Saig_ManBmcCreateCnf_iter( p, pTemp, f, vVisit2 );
        if ( Vec_IntSize(vVisit2) == 0 )
            break;
    }
    Vec_WecForEachLevelReverse( p->vVisited, vVisit, f )
        Aig_ManForEachObjVec( vVisit, p->pAig, pTemp, i )
            Saig_ManBmcCreateCnf_rec( p, pTemp, iFrame-f );
    Lit = Saig_ManBmcLiteral( p, pObj, iFrame );
    // extend the SAT solver
    if ( p->pSat2 )
        satoko_setnvars( p->pSat2, p->nSatVars );
    else if ( p->pSat3 )
    {
        for ( i = bmcg_sat_solver_varnum(p->pSat3); i < p->nSatVars; i++ )
            bmcg_sat_solver_addvar( p->pSat3 );
    }
    else
        sat_solver_setnvars( p->pSat, p->nSatVars );
    return Lit;
}



/**Function*************************************************************

  Synopsis    [Procedure used for sorting the nodes in decreasing order of levels.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_NodeCompareRefsIncrease( Aig_Obj_t ** pp1, Aig_Obj_t ** pp2 )
{
    int Diff = Aig_ObjRefs(*pp1) - Aig_ObjRefs(*pp2);
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    Diff = Aig_ObjId(*pp1) - Aig_ObjId(*pp2);
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [This procedure sets default parameters.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ParBmcSetDefaultParams( Saig_ParBmc_t * p )
{
    memset( p, 0, sizeof(Saig_ParBmc_t) );
    p->nStart         =     0;    // maximum number of timeframes 
    p->nFramesMax     =     0;    // maximum number of timeframes 
    p->nConfLimit     =     0;    // maximum number of conflicts at a node
    p->nConfLimitJump =     0;    // maximum number of conflicts after jumping
    p->nFramesJump    =     0;    // the number of tiemframes to jump
    p->nTimeOut       =     0;    // approximate timeout in seconds
    p->nTimeOutGap    =     0;    // time since the last CEX found
    p->nPisAbstract   =     0;    // the number of PIs to abstract
    p->fSolveAll      =     0;    // stops on the first SAT instance
    p->fDropSatOuts   =     0;    // replace sat outputs by constant 0
    p->nLearnedStart  = 10000;    // starting learned clause limit
    p->nLearnedDelta  =  2000;    // delta of learned clause limit
    p->nLearnedPerce  =    80;    // ratio of learned clause limit
    p->fVerbose       =     0;    // verbose 
    p->fNotVerbose    =     0;    // skip line-by-line print-out 
    p->iFrame         =    -1;    // explored up to this frame
    p->nFailOuts      =     0;    // the number of failed outputs
    p->nDropOuts      =     0;    // the number of timed out outputs
    p->timeLastSolved =     0;    // time when the last one was solved
}

/**Function*************************************************************

  Synopsis    [Returns time to stop.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
abctime Saig_ManBmcTimeToStop( Saig_ParBmc_t * pPars, abctime nTimeToStopNG )
{
    abctime nTimeToStopGap = pPars->nTimeOutGap ? pPars->nTimeOutGap * CLOCKS_PER_SEC + Abc_Clock(): 0;
    abctime nTimeToStop = 0;
    if ( nTimeToStopNG && nTimeToStopGap )
        nTimeToStop = nTimeToStopNG < nTimeToStopGap ? nTimeToStopNG : nTimeToStopGap;
    else if ( nTimeToStopNG )
        nTimeToStop = nTimeToStopNG;
    else if ( nTimeToStopGap )
        nTimeToStop = nTimeToStopGap;
    return nTimeToStop;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Cex_t * Saig_ManGenerateCex( Gia_ManBmc_t * p, int f, int i )
{
    Aig_Obj_t * pObjPi;
    Abc_Cex_t * pCex = Abc_CexMakeTriv( Aig_ManRegNum(p->pAig), Saig_ManPiNum(p->pAig), Saig_ManPoNum(p->pAig), f*Saig_ManPoNum(p->pAig)+i );
    int j, k, iBit = Saig_ManRegNum(p->pAig);
    for ( j = 0; j <= f; j++, iBit += Saig_ManPiNum(p->pAig) )
        Saig_ManForEachPi( p->pAig, pObjPi, k )
        {
            int iLit = Saig_ManBmcLiteral( p, pObjPi, j );
            if ( p->pSat2 )
            {
                if ( iLit != ~0 && satoko_read_cex_varvalue(p->pSat2, lit_var(iLit)) )
                    Abc_InfoSetBit( pCex->pData, iBit + k );
            }
            else if ( p->pSat3 )
            {
                if ( iLit != ~0 && bmcg_sat_solver_read_cex_varvalue(p->pSat3, lit_var(iLit)) )
                    Abc_InfoSetBit( pCex->pData, iBit + k );
            }
            else
            {
                if ( iLit != ~0 && sat_solver_var_value(p->pSat, lit_var(iLit)) )
                    Abc_InfoSetBit( pCex->pData, iBit + k );
            }
        }
    return pCex;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManCallSolver( Gia_ManBmc_t * p, int Lit )
{
    if ( Lit == 0 )
        return l_False;
    if ( Lit == 1 )
        return l_True;
    if ( p->pSat2 )
        return satoko_solve_assumptions_limit( p->pSat2, &Lit, 1, p->pPars->nConfLimit );
    else if ( p->pSat3 )
    {
        bmcg_sat_solver_set_conflict_budget( p->pSat3, p->pPars->nConfLimit );
        return bmcg_sat_solver_solve( p->pSat3, &Lit, 1 );
    }
    else
        return sat_solver_solve( p->pSat, &Lit, &Lit + 1, (ABC_INT64_T)p->pPars->nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
}

/**Function*************************************************************

  Synopsis    [Bounded model checking engine.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManBmcScalable( Aig_Man_t * pAig, Saig_ParBmc_t * pPars )
{
    Gia_ManBmc_t * p;
    Aig_Obj_t * pObj;
    Abc_Cex_t * pCexNew, * pCexNew0;
    FILE * pLogFile = NULL;
    unsigned * pInfo;
    int RetValue = -1, fFirst = 1, nJumpFrame = 0, fUnfinished = 0;
    int nOutDigits = Abc_Base10Log( Saig_ManPoNum(pAig) );
    int i, f, k, Lit, status;
    abctime clk, clk2, clkSatRun, clkOther = 0, clkTotal = Abc_Clock();
    abctime nTimeUnsat = 0, nTimeSat = 0, nTimeUndec = 0, clkOne = 0;
    abctime nTimeToStopNG, nTimeToStop;
    if ( pPars->pLogFileName )
        pLogFile = fopen( pPars->pLogFileName, "wb" );
    if ( pPars->nTimeOutOne && pPars->nTimeOut == 0 )
        pPars->nTimeOut = pPars->nTimeOutOne * Saig_ManPoNum(pAig) / 1000 + 1;
    if ( pPars->nTimeOutOne && !pPars->fSolveAll )
        pPars->nTimeOutOne = 0;
    nTimeToStopNG = pPars->nTimeOut ? pPars->nTimeOut * CLOCKS_PER_SEC + Abc_Clock(): 0;
    nTimeToStop   = Saig_ManBmcTimeToStop( pPars, nTimeToStopNG );
    // create BMC manager
    p = Saig_Bmc3ManStart( pAig, pPars->nTimeOutOne, pPars->nConfLimit, pPars->fUseSatoko, pPars->fUseGlucose );
    p->pPars = pPars;
    if ( p->pSat )
    {
        p->pSat->nLearntStart = p->pPars->nLearnedStart;
        p->pSat->nLearntDelta = p->pPars->nLearnedDelta;
        p->pSat->nLearntRatio = p->pPars->nLearnedPerce;
        p->pSat->nLearntMax   = p->pSat->nLearntStart;
        p->pSat->fNoRestarts  = p->pPars->fNoRestarts;
        p->pSat->RunId        = p->pPars->RunId;
        p->pSat->pFuncStop    = p->pPars->pFuncStop;
    }
    else if ( p->pSat3 )
    {
//        satoko_set_runid(p->pSat3, p->pPars->RunId);
//        satoko_set_stop_func(p->pSat3, p->pPars->pFuncStop);
    }
    else
    {
        satoko_set_runid(p->pSat2, p->pPars->RunId);
        satoko_set_stop_func(p->pSat2, p->pPars->pFuncStop);
    }
    if ( pPars->fSolveAll && p->vCexes == NULL )
        p->vCexes = Vec_PtrStart( Saig_ManPoNum(pAig) );
    if ( pPars->fVerbose )
    {
        Abc_Print( 1, "Running \"bmc3\". PI/PO/Reg = %d/%d/%d. And =%7d. Lev =%6d. ObjNums =%6d.\n",// Sect =%3d.\n", 
            Saig_ManPiNum(pAig), Saig_ManPoNum(pAig), Saig_ManRegNum(pAig),
            Aig_ManNodeNum(pAig), Aig_ManLevelNum(pAig), p->nObjNums );//, Vec_VecSize(p->vSects) );
        Abc_Print( 1, "Params: FramesMax = %d. Start = %d. ConfLimit = %d. TimeOut = %d. SolveAll = %d.\n", 
            pPars->nFramesMax, pPars->nStart, pPars->nConfLimit, pPars->nTimeOut, pPars->fSolveAll );
    } 
    pPars->nFramesMax = pPars->nFramesMax ? pPars->nFramesMax : ABC_INFINITY;
    // set runtime limit
    if ( nTimeToStop )
    {
        if ( p->pSat2 )
            satoko_set_runtime_limit( p->pSat2, nTimeToStop );
        else if ( p->pSat3 )
            bmcg_sat_solver_set_runtime_limit( p->pSat3, nTimeToStop );
        else
            sat_solver_set_runtime_limit( p->pSat, nTimeToStop );
    }
    // perform frames
    Aig_ManRandom( 1 );
    pPars->timeLastSolved = Abc_Clock();
    for ( f = 0; f < pPars->nFramesMax; f++ )
    {
        // stop BMC after exploring all reachable states
        if ( !pPars->nFramesJump && Aig_ManRegNum(pAig) < 30 && f == (1 << Aig_ManRegNum(pAig)) )
        {
            Abc_Print( 1, "Stopping BMC because all 2^%d reachable states are visited.\n", Aig_ManRegNum(pAig) );
            if ( p->pPars->fUseBridge )
                Saig_ManForEachPo( pAig, pObj, i )
                    if ( !(p->vCexes && Vec_PtrEntry(p->vCexes, i)) && !(p->pTime4Outs && p->pTime4Outs[i] == 0) ) // not SAT and not timed out
                        Gia_ManToBridgeResult( stdout, 1, NULL, i );
            RetValue = pPars->nFailOuts ? 0 : 1;
            goto finish;
        }
        // stop BMC if all targets are solved
        if ( pPars->fSolveAll && pPars->nFailOuts + pPars->nDropOuts >= Saig_ManPoNum(pAig) )
        {
            Abc_Print( 1, "Stopping BMC because all targets are disproved or timed out.\n" );
            RetValue = pPars->nFailOuts ? 0 : 1;
            goto finish;
        }
        // consider the next timeframe
        if ( (RetValue == -1 || pPars->fSolveAll) && pPars->nStart == 0 && !nJumpFrame )
            pPars->iFrame = f-1;
        // map nodes of this section
        Vec_PtrPush( p->vId2Var, Vec_IntStartFull(p->nObjNums) );
        Vec_PtrPush( p->vTerInfo, (pInfo = ABC_CALLOC(unsigned, p->nWordNum)) );
/*
        // cannot remove mapping of frame values for any timeframes
        // because with constant propagation they may be needed arbitrarily far
        if ( f > 2*Vec_VecSize(p->vSects) )
        {
            int iFrameOld = f - 2*Vec_VecSize( p->vSects );
            void * pMemory = Vec_IntReleaseArray( Vec_PtrEntry(p->vId2Var, iFrameOld) );
            ABC_FREE( pMemory );
        } 
*/
        // prepare some nodes
        Saig_ManBmcSetLiteral( p, Aig_ManConst1(pAig), f, 1 );
        Saig_ManBmcSimInfoSet( pInfo, Aig_ManConst1(pAig), SAIG_TER_ONE );
        Saig_ManForEachPi( pAig, pObj, i )
            Saig_ManBmcSimInfoSet( pInfo, pObj, SAIG_TER_UND );
        if ( f == 0 )
        {
            Saig_ManForEachLo( p->pAig, pObj, i )
            {
                Saig_ManBmcSetLiteral( p, pObj, 0, 0 );
                Saig_ManBmcSimInfoSet( pInfo, pObj, SAIG_TER_ZER );
            }
        }
        if ( (pPars->nStart && f < pPars->nStart) || (nJumpFrame && f < nJumpFrame) )
            continue;
        // create CNF upfront
        if ( pPars->fSolveAll )
        {
            Saig_ManForEachPo( pAig, pObj, i )
            {
                if ( i >= Saig_ManPoNum(pAig) )
                    break;
                // check for timeout
                if ( pPars->nTimeOutGap && pPars->timeLastSolved && Abc_Clock() > pPars->timeLastSolved + pPars->nTimeOutGap * CLOCKS_PER_SEC )
                {
                    Abc_Print( 1, "Reached gap timeout (%d seconds).\n",  pPars->nTimeOutGap );
                    goto finish;
                }
                if ( nTimeToStop && Abc_Clock() > nTimeToStop )
                {
                    if ( !pPars->fSilent )
                        Abc_Print( 1, "Reached timeout (%d seconds).\n",  pPars->nTimeOut );
                    goto finish;
                }
                // skip solved outputs
                if ( p->vCexes && Vec_PtrEntry(p->vCexes, i) )
                    continue;
                // skip output whose time has run out
                if ( p->pTime4Outs && p->pTime4Outs[i] == 0 )
                    continue;
                // add constraints for this output
clk2 = Abc_Clock();
                Saig_ManBmcCreateCnf( p, pObj, f );
clkOther += Abc_Clock() - clk2;
            }
        }
        // solve SAT
        clk = Abc_Clock(); 
        Saig_ManForEachPo( pAig, pObj, i )
        {
            if ( i >= Saig_ManPoNum(pAig) )
                break;
            // check for timeout
            if ( pPars->nTimeOutGap && pPars->timeLastSolved && Abc_Clock() > pPars->timeLastSolved + pPars->nTimeOutGap * CLOCKS_PER_SEC )
            {
                Abc_Print( 1, "Reached gap timeout (%d seconds).\n",  pPars->nTimeOutGap );
                goto finish;
            }
            if ( nTimeToStop && Abc_Clock() > nTimeToStop )
            {
                if ( !pPars->fSilent )
                    Abc_Print( 1, "Reached timeout (%d seconds).\n",  pPars->nTimeOut );
                goto finish;
            }
            if ( p->pPars->pFuncStop && p->pPars->pFuncStop(p->pPars->RunId) )
            {
                if ( !pPars->fSilent )
                    Abc_Print( 1, "Bmc3 got callbacks.\n" );
                goto finish;
            }
            // skip solved outputs
            if ( p->vCexes && Vec_PtrEntry(p->vCexes, i) )
                continue;
            // skip output whose time has run out
            if ( p->pTime4Outs && p->pTime4Outs[i] == 0 )
                continue;
            // add constraints for this output
clk2 = Abc_Clock();
            Lit = Saig_ManBmcCreateCnf( p, pObj, f );
clkOther += Abc_Clock() - clk2;
            // solve this output
            fUnfinished = 0;
            if ( p->pSat ) sat_solver_compress( p->pSat );
            if ( p->pTime4Outs )
            {
                assert( p->pTime4Outs[i] > 0 );
                clkOne = Abc_Clock();
                if ( p->pSat2 )
                    satoko_set_runtime_limit( p->pSat2, p->pTime4Outs[i] + Abc_Clock() );
                else if ( p->pSat3 )
                    bmcg_sat_solver_set_runtime_limit( p->pSat3, p->pTime4Outs[i] + Abc_Clock() );
                else
                    sat_solver_set_runtime_limit( p->pSat, p->pTime4Outs[i] + Abc_Clock() );
            }
clk2 = Abc_Clock();
            status = Saig_ManCallSolver( p, Lit );
clkSatRun = Abc_Clock() - clk2;
            if ( pLogFile )
                fprintf( pLogFile, "Frame %5d  Output %5d  Time(ms) %8d %8d\n", f, i, 
                    Lit < 2 ? 0 : (int)(clkSatRun * 1000 / CLOCKS_PER_SEC),
                    Lit < 2 ? 0 : Abc_MaxInt(0, Abc_MinInt(pPars->nTimeOutOne, pPars->nTimeOutOne - (int)((p->pTime4Outs[i] - clkSatRun) * 1000 / CLOCKS_PER_SEC))) );
            if ( p->pTime4Outs )
            {
                abctime timeSince = Abc_Clock() - clkOne;
                assert( p->pTime4Outs[i] > 0 );
                p->pTime4Outs[i] = (p->pTime4Outs[i] > timeSince) ? p->pTime4Outs[i] - timeSince : 0;
                if ( p->pTime4Outs[i] == 0 && status != l_True )
                    pPars->nDropOuts++;
            }
            if ( status == l_False )
            {
nTimeUnsat += clkSatRun;
                if ( Lit != 0 )
                {
                    // add final unit clause
                    Lit = lit_neg( Lit );
                    if ( p->pSat2 )
                        status = satoko_add_clause( p->pSat2, &Lit, 1 );
                    else if ( p->pSat3 )
                        status = bmcg_sat_solver_addclause( p->pSat3, &Lit, 1 );
                    else
                        status = sat_solver_addclause( p->pSat, &Lit, &Lit + 1 );
                    assert( status );
                    // add learned units
                    if ( p->pSat )
                    {
                        for ( k = 0; k < veci_size(&p->pSat->unit_lits); k++ )
                        {
                            Lit = veci_begin(&p->pSat->unit_lits)[k];
                            status = sat_solver_addclause( p->pSat, &Lit, &Lit + 1 );
                            assert( status );
                        }
                        veci_resize(&p->pSat->unit_lits, 0);
                        // propagate units
                        sat_solver_compress( p->pSat );
                    }
                }
                if ( p->pPars->fUseBridge )
                    Gia_ManReportProgress( stdout, i, f );
            }
            else if ( status == l_True )
            {
nTimeSat += clkSatRun;
                RetValue = 0;
                fFirst = 0;
                if ( !pPars->fSolveAll )
                {
                    if ( pPars->fVerbose )
                    {
                        Abc_Print( 1, "%4d %s : ", f,  fUnfinished ? "-" : "+" );
                        Abc_Print( 1, "Var =%8.0f. ",  (double)p->nSatVars );
                        Abc_Print( 1, "Cla =%9.0f. ",  (double)(p->pSat ? p->pSat->stats.clauses   : p->pSat3 ? bmcg_sat_solver_clausenum(p->pSat3)   : satoko_clausenum(p->pSat2)) );
                        Abc_Print( 1, "Conf =%7.0f. ", (double)(p->pSat ? p->pSat->stats.conflicts : p->pSat3 ? bmcg_sat_solver_conflictnum(p->pSat3) : satoko_conflictnum(p->pSat2)) );
//                        Abc_Print( 1, "Imp =%10.0f. ", (double)p->pSat->stats.propagations );
//                        Abc_Print( 1, "Uni =%7.0f. ",(double)(p->pSat ? sat_solver_count_assigned(p->pSat) : 0) );
//                        ABC_PRT( "Time", Abc_Clock() - clk );
                        Abc_Print( 1, "Learn =%7.0f. ", (double)(p->pSat ? p->pSat->stats.learnts : p->pSat3 ? bmcg_sat_solver_learntnum(p->pSat3) : satoko_learntnum(p->pSat2)) );
                        Abc_Print( 1, "%4.0f MB",      4.25*(f+1)*p->nObjNums /(1<<20) );
                        Abc_Print( 1, "%4.0f MB",      1.0*(p->pSat ? sat_solver_memory(p->pSat) : 0)/(1<<20) );
                        Abc_Print( 1, "%9.2f sec  ",   (float)(Abc_Clock() - clkTotal)/(float)(CLOCKS_PER_SEC) );
//                        Abc_Print( 1, "\n" );
//                        ABC_PRMn( "Id2Var", (f+1)*p->nObjNums*4 );
//                        ABC_PRMn( "SAT", 42 * p->pSat->size + 16 * (int)p->pSat->stats.clauses + 4 * (int)p->pSat->stats.clauses_literals );
//                        Abc_Print( 1, "S =%6d. ", p->nBufNum );
//                        Abc_Print( 1, "D =%6d. ", p->nDupNum );
                        Abc_Print( 1, "\n" );
                        fflush( stdout );
                    }
                    ABC_FREE( pAig->pSeqModel );
                    pAig->pSeqModel = Saig_ManGenerateCex( p, f, i );
                    goto finish;
                }
                pPars->nFailOuts++;
                if ( !pPars->fNotVerbose )
                    Abc_Print( 1, "Output %*d was asserted in frame %2d (solved %*d out of %*d outputs).\n",  
                        nOutDigits, i, f, nOutDigits, pPars->nFailOuts, nOutDigits, Saig_ManPoNum(pAig) );
                if ( p->vCexes == NULL )
                    p->vCexes = Vec_PtrStart( Saig_ManPoNum(pAig) );
                pCexNew = (p->pPars->fUseBridge || pPars->fStoreCex) ? Saig_ManGenerateCex( p, f, i ) : (Abc_Cex_t *)(ABC_PTRINT_T)1;
                pCexNew0 = NULL;
                if ( p->pPars->fUseBridge )
                {
                    Gia_ManToBridgeResult( stdout, 0, pCexNew, pCexNew->iPo );
                    //Abc_CexFree( pCexNew );
                    pCexNew0 = pCexNew; 
                    pCexNew = (Abc_Cex_t *)(ABC_PTRINT_T)1;
                }
                Vec_PtrWriteEntry( p->vCexes, i, Abc_CexDup(pCexNew, Saig_ManRegNum(pAig)) ); 
                if ( pPars->pFuncOnFail && pPars->pFuncOnFail(i, pPars->fStoreCex ? (Abc_Cex_t *)Vec_PtrEntry(p->vCexes, i) : NULL) )
                {
                    Abc_CexFreeP( &pCexNew0 );
                    Abc_Print( 1, "Quitting due to callback on fail.\n" );
                    goto finish;
                }
                // reset the timeout
                pPars->timeLastSolved = Abc_Clock();
                nTimeToStop = Saig_ManBmcTimeToStop( pPars, nTimeToStopNG );
                if ( nTimeToStop )
                {
                    if ( p->pSat2 )
                        satoko_set_runtime_limit( p->pSat2, nTimeToStop );
                    else if ( p->pSat3 )
                        bmcg_sat_solver_set_runtime_limit( p->pSat3, nTimeToStop );
                    else
                        sat_solver_set_runtime_limit( p->pSat, nTimeToStop );
                }

                // check if other outputs failed under the same counter-example
                Saig_ManForEachPo( pAig, pObj, k )
                {
                    if ( k >= Saig_ManPoNum(pAig) )
                        break;
                    // skip solved outputs
                    if ( p->vCexes && Vec_PtrEntry(p->vCexes, k) )
                        continue;
                    // check if this output is solved
                    Lit = Saig_ManBmcCreateCnf( p, pObj, f );
                    if ( p->pSat2 )
                    {
                        if ( satoko_read_cex_varvalue(p->pSat2, lit_var(Lit)) == Abc_LitIsCompl(Lit) )
                            continue;
                    }
                    else if ( p->pSat3 )
                    {
                        if ( bmcg_sat_solver_read_cex_varvalue(p->pSat3, lit_var(Lit)) == Abc_LitIsCompl(Lit) )
                            continue;
                    }
                    else
                    {
                        if ( sat_solver_var_value(p->pSat, lit_var(Lit)) == Abc_LitIsCompl(Lit) )
                            continue;
                    }
                    // write entry
                    pPars->nFailOuts++;
                    if ( !pPars->fNotVerbose )
                        Abc_Print( 1, "Output %*d was asserted in frame %2d (solved %*d out of %*d outputs).\n",  
                            nOutDigits, k, f, nOutDigits, pPars->nFailOuts, nOutDigits, Saig_ManPoNum(pAig) );
                    // report to the bridge
                    if ( p->pPars->fUseBridge )
                    {
                        // set the output number
                        pCexNew0->iPo = k;
                        Gia_ManToBridgeResult( stdout, 0, pCexNew0, pCexNew0->iPo );
                    }
                    // remember solved output
                    Vec_PtrWriteEntry( p->vCexes, k, Abc_CexDup(pCexNew, Saig_ManRegNum(pAig)) );
                }
                Abc_CexFreeP( &pCexNew0 );
                Abc_CexFree( pCexNew );
            }
            else 
            {
nTimeUndec += clkSatRun;
                assert( status == l_Undef );
                if ( pPars->nFramesJump )
                {
                    pPars->nConfLimit = pPars->nConfLimitJump;
                    nJumpFrame = f + pPars->nFramesJump;
                    fUnfinished = 1;
                    break;
                }
                if ( p->pTime4Outs == NULL )
                    goto finish;
            }
        }
        if ( pPars->fVerbose ) 
        {
            if ( fFirst == 1 && f > 0 && (p->pSat ? p->pSat->stats.conflicts : p->pSat3 ? bmcg_sat_solver_conflictnum(p->pSat3) : satoko_conflictnum(p->pSat2)) > 1 )
            {
                fFirst = 0;
//                Abc_Print( 1, "Outputs of frames up to %d are trivially UNSAT.\n", f );
            }
            Abc_Print( 1, "%4d %s : ", f, fUnfinished ? "-" : "+" );
            Abc_Print( 1, "Var =%8.0f. ", (double)p->nSatVars );
//            Abc_Print( 1, "Used =%8.0f. ", (double)sat_solver_count_usedvars(p->pSat) );
            Abc_Print( 1, "Cla =%9.0f. ", (double)(p->pSat ? p->pSat->stats.clauses   : p->pSat3 ? bmcg_sat_solver_clausenum(p->pSat3)   : satoko_clausenum(p->pSat2))   );
            Abc_Print( 1, "Conf =%7.0f. ",(double)(p->pSat ? p->pSat->stats.conflicts : p->pSat3 ? bmcg_sat_solver_conflictnum(p->pSat3) : satoko_conflictnum(p->pSat2)) );
//            Abc_Print( 1, "Imp =%10.0f. ", (double)p->pSat->stats.propagations );
//            Abc_Print( 1, "Uni =%7.0f. ", (double)(p->pSat ? sat_solver_count_assigned(p->pSat) : 0) );
            Abc_Print( 1, "Learn =%7.0f. ", (double)(p->pSat ? p->pSat->stats.learnts : p->pSat3 ? bmcg_sat_solver_learntnum(p->pSat3) : satoko_learntnum(p->pSat2)) );
            if ( pPars->fSolveAll )
                Abc_Print( 1, "CEX =%5d. ", pPars->nFailOuts );
            if ( pPars->nTimeOutOne )
                Abc_Print( 1, "T/O =%4d. ", pPars->nDropOuts );
//            ABC_PRT( "Time", Abc_Clock() - clk );
//            Abc_Print( 1, "%4.0f MB",     4.0*Vec_IntSize(p->vVisited) /(1<<20) );
            Abc_Print( 1, "%4.0f MB",     4.0*(f+1)*p->nObjNums /(1<<20) );
            Abc_Print( 1, "%4.0f MB",     1.0*(p->pSat ? sat_solver_memory(p->pSat) : 0)/(1<<20) );
//            Abc_Print( 1, " %6d %6d ",   p->nLitUsed, p->nLitUseless );
            Abc_Print( 1, "%9.2f sec ",   1.0*(Abc_Clock() - clkTotal)/CLOCKS_PER_SEC );
//            Abc_Print( 1, "\n" );
//            ABC_PRMn( "Id2Var", (f+1)*p->nObjNums*4 );
//            ABC_PRMn( "SAT", 42 * p->pSat->size + 16 * (int)p->pSat->stats.clauses + 4 * (int)p->pSat->stats.clauses_literals );
//            Abc_Print( 1, "Simples = %6d. ", p->nBufNum );
//            Abc_Print( 1, "Dups = %6d. ", p->nDupNum );
            Abc_Print( 1, "\n" );
            fflush( stdout );
        }
    }
    // consider the next timeframe
    if ( nJumpFrame && pPars->nStart == 0 )
        pPars->iFrame = nJumpFrame - pPars->nFramesJump;
    else if ( RetValue == -1 && pPars->nStart == 0 )
        pPars->iFrame = f-1;
//ABC_PRT( "CNF generation runtime", clkOther );
finish:
    if ( pPars->fVerbose )
    {
        Abc_Print( 1, "Runtime:  " );
        Abc_Print( 1, "CNF = %.1f sec (%.1f %%)  ",   1.0*clkOther/CLOCKS_PER_SEC,   100.0*clkOther/(Abc_Clock() - clkTotal)   );
        Abc_Print( 1, "UNSAT = %.1f sec (%.1f %%)  ", 1.0*nTimeUnsat/CLOCKS_PER_SEC, 100.0*nTimeUnsat/(Abc_Clock() - clkTotal) );
        Abc_Print( 1, "SAT = %.1f sec (%.1f %%)  ",   1.0*nTimeSat/CLOCKS_PER_SEC,   100.0*nTimeSat/(Abc_Clock() - clkTotal)   );
        Abc_Print( 1, "UNDEC = %.1f sec (%.1f %%)",   1.0*nTimeUndec/CLOCKS_PER_SEC, 100.0*nTimeUndec/(Abc_Clock() - clkTotal) );
        Abc_Print( 1, "\n" );
    }
    Saig_Bmc3ManStop( p );
    fflush( stdout );
    if ( pLogFile )
        fclose( pLogFile );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

