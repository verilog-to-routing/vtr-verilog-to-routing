/**CFile****************************************************************

  FileName    [abcRec2.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Record of semi-canonical AIG subgraphs.]

  Author      [Allan Yang, Alan Mishchenko]
  
  Affiliation [Fudan University in Shanghai, UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcRec.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "map/if/if.h"
#include "bool/kit/kit.h"
#include "aig/gia/giaAig.h"
#include "misc/vec/vecMem.h"
#include "opt/dau/dau.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

#define LMS_VAR_MAX    16  // LMS_VAR_MAX >= 6
#define LMS_MAX_WORD  (1<<(LMS_VAR_MAX-6))
//#define LMS_USE_OLD_FORM

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/*
    This LMS manager can be used in two modes:
        - library constuction
        - AIG level minimization
    To switch from library construction to AIG level minimization
    LSM manager should be restarted by dumping GIA (rec_dump3 <file>.aig) 
    and starting LMS manager again (rec_start3 <file>.aig).
*/

typedef struct Lms_Man_t_ Lms_Man_t;
struct Lms_Man_t_
{
    // parameters
    int               nVars;        // the number of variables
    int               nWords;       // the number of TT words
    int               nCuts;        // the max number of cuts to use
    int               fFuncOnly;    // record only functions
    int               fLibConstr;   // this manager is used for library construction
    // internal data for library construction
    Gia_Man_t *       pGia;         // the record
    Vec_Mem_t *       vTtMem;       // truth table memory and hash table
//    Vec_Mem_t *       vTtMem2;      // truth table memory and hash table
    Vec_Int_t *       vTruthIds;    // truth table IDs of each PO
    // internal data for AIG level minimization (allocated the first time it is called)
    Vec_Int_t *       vTruthPo;     // first PO where this canonicized truth table was seen
    Vec_Wrd_t *       vDelays;      // pin-to-pin delays of each PO
    Vec_Str_t *       vAreas;       // number of AND gates in each PO
    Vec_Int_t *       vFreqs;       // subgraph usage frequencies
    Vec_Int_t *       vTruthFreqs;  // truth table usage frequencies
    // temporaries
    Vec_Ptr_t *       vNodes;       // the temporary nodes
    Vec_Ptr_t *       vLabelsP;     // temporary storage for HOP node labels
    Vec_Int_t *       vLabels;      // temporary storage for AIG node labels
    Vec_Str_t *       vSupps;       // used temporarily by TT dumping
    word              pTemp1[LMS_MAX_WORD]; // copy of the truth table
    word              pTemp2[LMS_MAX_WORD]; // copy of the truth table
    // statistics 
    int               nTried;
    int               nFilterSize;
    int               nFilterRedund;
    int               nFilterVolume;
    int               nFilterTruth;
    int               nFilterError;
    int               nFilterSame;
    int               nAdded;
    int               nAddedFuncs;
    int               nHoleInTheWall;
    // runtime
    abctime           timeTruth;
    abctime           timeCanon;
    abctime           timeBuild;
    abctime           timeCheck;
    abctime           timeInsert;
    abctime           timeOther;
    abctime           timeTotal;
};

static Lms_Man_t * s_pMan3 = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Compute delay/area profiles of POs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int  Lms_DelayGet( word D, int v )           { assert(v >= 0 && v < LMS_VAR_MAX); return (int)((D >> (v << 2)) & 0xF);                             }
static inline void Lms_DelaySet( word * pD, int v, int d ) { assert(v >= 0 && v < LMS_VAR_MAX); assert(d >= 0 && d < LMS_VAR_MAX); *pD |= ((word)d << (v << 2)); }
static inline word Lms_DelayInit( int v )                  { assert(v >= 0 && v < LMS_VAR_MAX); return (word)1 << (v << 2);                                      }
static inline word Lms_DelayMax( word D1, word D2, int nVars )
{
    int v, Max;
    word D = 0;
    for ( v = 0; v < nVars; v++ )
        if ( (Max = Abc_MaxInt(Lms_DelayGet(D1, v), Lms_DelayGet(D2, v))) )
            Lms_DelaySet( &D, v, Abc_MinInt(Max + 1, 15) );
    return D;
}
static inline word Lms_DelayDecrement( word D1, int nVars )
{
    int v;
    word D = 0;
    for ( v = 0; v < nVars; v++ )
        if ( Lms_DelayGet(D1, v) )
            Lms_DelaySet( &D, v, Lms_DelayGet(D1, v) - 1 );
    return D;
}
static inline int Lms_DelayEqual( word D1, word D2, int nVars ) // returns 1 if D1 has the same delays than D2
{
    int v;
    for ( v = 0; v < nVars; v++ )
        if ( Lms_DelayGet(D1, v) != Lms_DelayGet(D2, v) )
            return 0;
    return 1;
}
static inline int Lms_DelayDom( word D1, word D2, int nVars ) // returns 1 if D1 has the same or smaller delays than D2
{
    int v;
    for ( v = 0; v < nVars; v++ )
        if ( Lms_DelayGet(D1, v) > Lms_DelayGet(D2, v) )
            return 0;
    return 1;
}
static inline void Lms_DelayPrint( word D, int nVars )
{
    int v;
    printf( "Delay profile = {" );
    for ( v = 0; v < nVars; v++ )
        printf( " %d", Lms_DelayGet(D, v) );
    printf( " }\n" );
}
Vec_Wrd_t * Lms_GiaDelays( Gia_Man_t * p )
{
    Vec_Wrd_t * vDelays, * vResult;
    Gia_Obj_t * pObj;
    int i;
    // compute delay profiles of all objects
    vDelays = Vec_WrdAlloc( Gia_ManObjNum(p) );
    Vec_WrdPush( vDelays, 0 ); // const 0
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            Vec_WrdPush( vDelays, Lms_DelayMax( Vec_WrdEntry(vDelays, Gia_ObjFaninId0(pObj, i)), Vec_WrdEntry(vDelays, Gia_ObjFaninId1(pObj, i)), Gia_ManCiNum(p) ) );
        else if ( Gia_ObjIsCo(pObj) )
            Vec_WrdPush( vDelays, Lms_DelayDecrement( Vec_WrdEntry(vDelays, Gia_ObjFaninId0(pObj, i)), Gia_ManCiNum(p) ) );
        else if ( Gia_ObjIsCi(pObj) )
            Vec_WrdPush( vDelays, Lms_DelayInit( Gia_ObjCioId(pObj) ) );
        else assert( 0 );
    }
    // collect delay profiles of COs only
    vResult = Vec_WrdAlloc( Gia_ManCoNum(p) );
    Gia_ManForEachCo( p, pObj, i )
        Vec_WrdPush( vResult, Vec_WrdEntry(vDelays, Gia_ObjId(p, pObj)) );
    Vec_WrdFree( vDelays );
    return vResult;
}
void Lms_ObjAreaMark_rec( Gia_Obj_t * pObj )
{
    if ( pObj->fMark0 || Gia_ObjIsCi(pObj) )
        return;
    pObj->fMark0 = 1;
    Lms_ObjAreaMark_rec( Gia_ObjFanin0(pObj) );
    Lms_ObjAreaMark_rec( Gia_ObjFanin1(pObj) );
}
int  Lms_ObjAreaUnmark_rec( Gia_Obj_t * pObj )
{
    if ( !pObj->fMark0 || Gia_ObjIsCi(pObj) )
        return 0;
    pObj->fMark0 = 0;
    return 1 + Lms_ObjAreaUnmark_rec( Gia_ObjFanin0(pObj) ) 
             + Lms_ObjAreaUnmark_rec( Gia_ObjFanin1(pObj) );
}
int Lms_ObjArea( Gia_Obj_t * pObj )
{
    assert( Gia_ObjIsAnd(pObj) );
    Lms_ObjAreaMark_rec( pObj );
    return Lms_ObjAreaUnmark_rec( pObj );    
}
Vec_Str_t * Lms_GiaAreas( Gia_Man_t * p )
{
    Vec_Str_t * vAreas;
    Gia_Obj_t * pObj;
    int i;
    vAreas = Vec_StrAlloc( Gia_ManCoNum(p) );
    Gia_ManForEachCo( p, pObj, i )
        Vec_StrPush( vAreas, (char)(Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) ? Lms_ObjArea(Gia_ObjFanin0(pObj)) : 0) );
    return vAreas;
}
Vec_Str_t * Lms_GiaSuppSizes( Gia_Man_t * p )
{
    Vec_Str_t * vResult;
    Vec_Str_t * vSupps;
    Gia_Obj_t * pObj;
    int i;
    vSupps = Vec_StrAlloc( Gia_ManObjNum(p) );
    Vec_StrPush( vSupps, 0 );
    Gia_ManForEachObj1( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(pObj) )
            Vec_StrPush( vSupps, (char)Abc_MaxInt( Vec_StrEntry(vSupps, Gia_ObjFaninId0(pObj, i)), Vec_StrEntry(vSupps, Gia_ObjFaninId1(pObj, i)) ) );
        else if ( Gia_ObjIsCo(pObj) )
            Vec_StrPush( vSupps, Vec_StrEntry(vSupps, Gia_ObjFaninId0(pObj, i)) );
        else if ( Gia_ObjIsCi(pObj) )
            Vec_StrPush( vSupps, (char)(Gia_ObjCioId(pObj)+1) );
        else assert( 0 );
    }
    assert( Vec_StrSize(vSupps) == Gia_ManObjNum(p) );
    vResult = Vec_StrAlloc( Gia_ManCoNum(p) );
    Gia_ManForEachCo( p, pObj, i )
        Vec_StrPush( vResult, Vec_StrEntry(vSupps, Gia_ObjId(p, pObj)) );
    Vec_StrFree( vSupps );
    return vResult;
}
void Lms_GiaProfilesPrint( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i;
    Vec_Wrd_t * vDelays;
    Vec_Str_t * vAreas;
    vDelays = Lms_GiaDelays( p );
    vAreas = Lms_GiaAreas( p );
    Gia_ManForEachPo( p, pObj, i )
    {
        printf( "%6d : ", i );
        printf( "A = %2d  ", Vec_StrEntry(vAreas, i) );
        Lms_DelayPrint( Vec_WrdEntry(vDelays, i), Gia_ManPiNum(p) );
//        Lms_GiaPrintSubgraph( p, pObj );
//        printf( "\n" );
    }
    Vec_WrdFree( vDelays );
    Vec_StrFree( vAreas );
}

/**Function*************************************************************

  Synopsis    [Prints one GIA subgraph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lms_GiaPrintSubgraph_rec( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    if ( !pObj->fMark0 || Gia_ObjIsCi(pObj) )
        return;
    pObj->fMark0 = 0;
    assert( Gia_ObjIsAnd(pObj) );
    Lms_GiaPrintSubgraph_rec( p, Gia_ObjFanin0(pObj) );
    Lms_GiaPrintSubgraph_rec( p, Gia_ObjFanin1(pObj) );
    Gia_ObjPrint( p, pObj );
}
void Lms_GiaPrintSubgraph( Gia_Man_t * p, Gia_Obj_t * pObj )
{
    assert( Gia_ObjIsCo(pObj) );
    if ( Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) )
    {
        Lms_ObjAreaMark_rec( Gia_ObjFanin0(pObj) );
        Lms_GiaPrintSubgraph_rec( p, Gia_ObjFanin0(pObj) ); 
    }
    else
        Gia_ObjPrint( p, Gia_ObjFanin0(pObj) );
    Gia_ObjPrint( p, pObj );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lms_Man_t * Lms_ManStart( Gia_Man_t * pGia, int nVars, int nCuts, int fFuncOnly, int fVerbose )
{
    Lms_Man_t * p;
    abctime clk, clk2 = Abc_Clock();
    // if GIA is given, use the number of variables from GIA
    nVars = pGia ? Gia_ManCiNum(pGia) : nVars;
    assert( nVars >= 6 && nVars <= LMS_VAR_MAX );
    // allocate manager
    p = ABC_CALLOC( Lms_Man_t, 1 );
    // parameters
    p->nVars = nVars;
    p->nCuts = nCuts;
    p->nWords = Abc_Truth6WordNum( nVars );
    p->fFuncOnly = fFuncOnly;
    // internal data for library construction
    p->vTtMem = Vec_MemAlloc( p->nWords, 12 ); // 32 KB/page for 6-var functions
//    p->vTtMem2 = Vec_MemAlloc( p->nWords, 12 ); // 32 KB/page for 6-var functions
    Vec_MemHashAlloc( p->vTtMem, 10000 );
//    Vec_MemHashAlloc( p->vTtMem2, 10000 );
    if ( fFuncOnly )
        return p;    
    p->vTruthIds = Vec_IntAlloc( 10000 );
    if ( pGia == NULL )
    {
        int i;
        p->pGia = Gia_ManStart( 10000 );
        p->pGia->pName = Abc_UtilStrsav( "record" );
        for ( i = 0; i < nVars; i++ )
            Gia_ManAppendCi( p->pGia );
    }
    else
    {
        Gia_Obj_t * pObj;
        word * pTruth;
        int i, Index, Prev = -1;
        p->pGia = pGia;
        // populate the manager with subgraphs present in GIA
        p->nAdded = Gia_ManCoNum( p->pGia );
        Gia_ManForEachCo( p->pGia, pObj, i )
        {
            clk = Abc_Clock();
            pTruth = Gia_ObjComputeTruthTable( p->pGia, pObj );
            p->timeTruth += Abc_Clock() - clk;
            clk = Abc_Clock();
            Index = Vec_MemHashInsert( p->vTtMem, pTruth );
            p->timeInsert += Abc_Clock() - clk;
            assert( Index == Prev || Index == Prev + 1 ); // GIA subgraphs should be ordered
            Vec_IntPush( p->vTruthIds, Index );
            Prev = Index;
        }
    }
    // temporaries
    p->vNodes    = Vec_PtrAlloc( 1000 );
    p->vLabelsP  = Vec_PtrAlloc( 1000 );
    p->vLabels   = Vec_IntAlloc( 1000 );
p->timeTotal += Abc_Clock() - clk2;
    return p;    
}
void Lms_ManStop( Lms_Man_t * p )
{
    // temporaries
    Vec_IntFreeP( &p->vLabels );
    Vec_PtrFreeP( &p->vLabelsP );
    Vec_PtrFreeP( &p->vNodes );
    // internal data for AIG level minimization
    Vec_IntFreeP( &p->vTruthPo );
    Vec_WrdFreeP( &p->vDelays );
    Vec_StrFreeP( &p->vAreas );
    Vec_IntFreeP( &p->vFreqs );
    Vec_IntFreeP( &p->vTruthFreqs );
    // internal data for library construction
    Vec_IntFreeP( &p->vTruthIds );
    Vec_MemHashFree( p->vTtMem );
//    Vec_MemHashFree( p->vTtMem2 );
    Vec_MemFree( p->vTtMem );
//    Vec_MemFree( p->vTtMem2 );
    Gia_ManStopP( &p->pGia );
    ABC_FREE( p );
}
void Lms_ManPrepare( Lms_Man_t * p )
{
    // compute the first PO for each semi-canonical form
    int i, Entry;
    assert( !p->fLibConstr );
    assert( p->vTruthPo == NULL );
    p->vTruthPo = Vec_IntStartFull( Vec_MemEntryNum(p->vTtMem)+1 );
    assert( Vec_IntFindMin(p->vTruthIds) >= 0 );
    assert( Vec_IntFindMax(p->vTruthIds) < Vec_MemEntryNum(p->vTtMem) );
    Vec_IntForEachEntry( p->vTruthIds, Entry, i )
        if ( Vec_IntEntry(p->vTruthPo, Entry) == -1 )
            Vec_IntWriteEntry( p->vTruthPo, Entry, i );
    Vec_IntWriteEntry( p->vTruthPo, Vec_MemEntryNum(p->vTtMem), Gia_ManCoNum(p->pGia) );
    // compute delay/area and init frequency
    assert( p->vDelays == NULL );
    assert( p->vAreas == NULL );
    assert( p->vFreqs == NULL );
    p->vDelays = Lms_GiaDelays( p->pGia );
    p->vAreas  = Lms_GiaAreas( p->pGia );
    p->vFreqs  = Vec_IntStart( Gia_ManCoNum(p->pGia) );
}
void Lms_ManPrintFuncStats( Lms_Man_t * p )
{
    Vec_Str_t * vSupps;
    int Counters[LMS_VAR_MAX+1] = {0}, CountersS[LMS_VAR_MAX+1] = {0};
    int i, Entry, Next;
    if ( p->pGia == NULL )
        return;
    if ( p->fLibConstr )
        return;
    if ( p->vTruthPo == NULL )
        Lms_ManPrepare( p );
    vSupps = Lms_GiaSuppSizes( p->pGia );
    Vec_IntForEachEntry( p->vTruthPo, Entry, i )
    {
        if ( i == Vec_IntSize(p->vTruthPo) - 1 )
            break;
        Next = Vec_IntEntry( p->vTruthPo, i+1 );
        Counters[(int)Vec_StrEntry(vSupps, Entry)]++;
        CountersS[(int)Vec_StrEntry(vSupps, Entry)] += Next - Entry;
    }
    for ( i = 0; i <= LMS_VAR_MAX; i++ )
        if ( Counters[i] )
            printf( "Inputs = %2d.  Funcs = %8d.  Subgrs = %8d.  Ratio = %6.2f.\n", i, Counters[i], CountersS[i], 1.0*CountersS[i]/Counters[i] );
    Vec_StrFree( vSupps );
}
void Lms_ManPrintFreqStats( Lms_Man_t * p )
{
    int CountDsdNpn[3]  = {0};  // full/part/none
    int CountDsdAll[3]  = {0};  // full/part/none
    int CountStepNpn[3] = {0};  // full/1step/complex
    int CountStepAll[3] = {0};  // full/1step/complex
    char pBuffer[1000];
    int nSuppSize;
    int nNonDecSize;
    word * pTruth;
    int i, Freq, Status;
    printf( "Cuts  = %10d. ",            p->nTried );
//    printf( "Funcs = %10d (%6.2f %%). ", Vec_MemEntryNum(p->vTtMem2), 100.0*Vec_MemEntryNum(p->vTtMem2)/p->nTried );
    printf( "Class = %10d (%6.2f %%). ", Vec_MemEntryNum(p->vTtMem),  100.0*Vec_MemEntryNum(p->vTtMem)/p->nTried );
    printf( "\n" );
//    return;

    Vec_IntForEachEntry( p->vTruthFreqs, Freq, i )
    {
        pTruth = Vec_MemReadEntry(p->vTtMem, i);
/*
        printf( "%6d -- %6d : ", i, Freq );
        Kit_DsdWriteFromTruth( pBuffer, (unsigned *)pTruth, p->nVars );
        printf( "%s\n", pBuffer );
*/
        nSuppSize = Abc_TtSupportSize( pTruth, p->nVars );
        nNonDecSize = Dau_DsdDecompose( pTruth, p->nVars, 0, 0, pBuffer );
        if ( nNonDecSize == 0 )
        {
            CountDsdNpn[0]++;
            CountDsdAll[0] += Freq;
        }
        else if ( nNonDecSize < nSuppSize )
        {
            CountDsdNpn[1]++;
            CountDsdAll[1] += Freq;
        }
        else // non-dec
        {
            CountDsdNpn[2]++;
            CountDsdAll[2] += Freq;
        }

        if ( nNonDecSize == 0 )
        {
            CountStepNpn[0]++;
            CountStepAll[0] += Freq;
            continue;
        }

        // check the non dec core
        Status = Dau_DsdCheck1Step( NULL, pTruth, nNonDecSize, NULL );
        if ( Status >= 0 )
        {
            CountStepNpn[1]++;
            CountStepAll[1] += Freq;
        }
        else
        {
            assert( Status == -2 );
            CountStepNpn[2]++;
            CountStepAll[2] += Freq;
        }
    }

    // print the results
    printf( "NPN: " );
    printf( "Full = %6.2f %%  ", 100.0 * CountDsdNpn[0] / Vec_MemEntryNum(p->vTtMem) );
    printf( "Part = %6.2f %%  ", 100.0 * CountDsdNpn[1] / Vec_MemEntryNum(p->vTtMem) );
    printf( "None = %6.2f %%  ", 100.0 * CountDsdNpn[2] / Vec_MemEntryNum(p->vTtMem) );
//    printf( "\n" );
    printf( "   " );
    // print the results
    printf( "All: " );
    printf( "Full = %6.2f %%  ", 100.0 * CountDsdAll[0] / p->nTried );
    printf( "Part = %6.2f %%  ", 100.0 * CountDsdAll[1] / p->nTried );
    printf( "None = %6.2f %%  ", 100.0 * CountDsdAll[2] / p->nTried );
    printf( "\n" );

    // print the results
    printf( "NPN: " );
    printf( "Full = %6.2f %%  ", 100.0 * CountStepNpn[0] / Vec_MemEntryNum(p->vTtMem) );
    printf( "1stp = %6.2f %%  ", 100.0 * CountStepNpn[1] / Vec_MemEntryNum(p->vTtMem) );
    printf( "Comp = %6.2f %%  ", 100.0 * CountStepNpn[2] / Vec_MemEntryNum(p->vTtMem) );
//    printf( "\n" );
    printf( "   " );
    // print the results
    printf( "All: " );
    printf( "Full = %6.2f %%  ", 100.0 * CountStepAll[0] / p->nTried );
    printf( "1stp = %6.2f %%  ", 100.0 * CountStepAll[1] / p->nTried );
    printf( "Comp = %6.2f %%  ", 100.0 * CountStepAll[2] / p->nTried );
    printf( "\n" );

}
void Lms_ManPrint( Lms_Man_t * p )
{
//    Gia_ManPrintStats( p->pGia, 0, 0 );
    printf( "Library with %d vars has %d classes and %d AIG subgraphs with %d AND nodes.\n", 
        p->nVars, Vec_MemEntryNum(p->vTtMem), p->nAdded, p->pGia ? Gia_ManAndNum(p->pGia) : 0 );

//    Lms_ManPrintFreqStats( p );
    Lms_ManPrintFuncStats( p );

    p->nAddedFuncs = Vec_MemEntryNum(p->vTtMem);
    printf( "Subgraphs tried                             = %10d. (%6.2f %%)\n", p->nTried,         !p->nTried? 0 : 100.0*p->nTried/p->nTried );
    printf( "Subgraphs filtered by support size          = %10d. (%6.2f %%)\n", p->nFilterSize,    !p->nTried? 0 : 100.0*p->nFilterSize/p->nTried );
    printf( "Subgraphs filtered by structural redundancy = %10d. (%6.2f %%)\n", p->nFilterRedund,  !p->nTried? 0 : 100.0*p->nFilterRedund/p->nTried );
    printf( "Subgraphs filtered by volume                = %10d. (%6.2f %%)\n", p->nFilterVolume,  !p->nTried? 0 : 100.0*p->nFilterVolume/p->nTried );
    printf( "Subgraphs filtered by TT redundancy         = %10d. (%6.2f %%)\n", p->nFilterTruth,   !p->nTried? 0 : 100.0*p->nFilterTruth/p->nTried );
    printf( "Subgraphs filtered by error                 = %10d. (%6.2f %%)\n", p->nFilterError,   !p->nTried? 0 : 100.0*p->nFilterError/p->nTried );
    printf( "Subgraphs filtered by isomorphism           = %10d. (%6.2f %%)\n", p->nFilterSame,    !p->nTried? 0 : 100.0*p->nFilterSame/p->nTried );
    printf( "Subgraphs added                             = %10d. (%6.2f %%)\n", p->nAdded,         !p->nTried? 0 : 100.0*p->nAdded/p->nTried );
    printf( "Functions added                             = %10d. (%6.2f %%)\n", p->nAddedFuncs,    !p->nTried? 0 : 100.0*p->nAddedFuncs/p->nTried );
    if ( p->nHoleInTheWall )
    printf( "Cuts whose logic structure has a hole       = %10d. (%6.2f %%)\n", p->nHoleInTheWall, !p->nTried? 0 : 100.0*p->nHoleInTheWall/p->nTried );

    p->timeOther = p->timeTotal - p->timeTruth - p->timeCanon - p->timeBuild - p->timeCheck - p->timeInsert;
    ABC_PRTP( "Runtime: Truth ", p->timeTruth,  p->timeTotal );
    ABC_PRTP( "Runtime: Canon ", p->timeCanon,  p->timeTotal );
    ABC_PRTP( "Runtime: Build ", p->timeBuild,  p->timeTotal );
    ABC_PRTP( "Runtime: Check ", p->timeCheck,  p->timeTotal );
    ABC_PRTP( "Runtime: Insert", p->timeInsert, p->timeTotal );
    ABC_PRTP( "Runtime: Other ", p->timeOther,  p->timeTotal );
    ABC_PRTP( "Runtime: TOTAL ", p->timeTotal,  p->timeTotal );
}

/**Function*************************************************************

  Synopsis    [Recanonicizes the library and add it to the current library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRecLibMerge3( Gia_Man_t * pLib )
{
    int fCheck = 0;
    Lms_Man_t * p = s_pMan3;
    Gia_Man_t * pGia = p->pGia;
    Vec_Str_t * vSupps;
    char pCanonPerm[LMS_VAR_MAX];
    unsigned uCanonPhase;
    word * pTruth;
    int i, k, Index, iFanin0, iFanin1, nLeaves;
    Gia_Obj_t * pObjPo, * pDriver, * pTemp = NULL;
    abctime clk, clk2 = Abc_Clock();

    if ( Gia_ManCiNum(pLib) != Gia_ManCiNum(pGia) )
    {
        printf( "The number of Library inputs (%d) differs from the number of Gia inputs (%d).\n", Gia_ManCiNum(pLib), Gia_ManCiNum(pGia) );
        return;
    }
    assert( Gia_ManCiNum(pLib) == Gia_ManCiNum(pGia) );

    // create hash table if not available
    if ( Vec_IntSize(&pGia->vHTable) == 0 )
        Gia_ManHashStart( pGia );

    // add AIG subgraphs
    vSupps = Lms_GiaSuppSizes( pLib );
    Gia_ManForEachCo( pLib, pObjPo, k )
    {
        // get support size
        nLeaves = Vec_StrEntry(vSupps, k);
        assert( nLeaves > 1 );

        // compute the truth table
clk = Abc_Clock();
        pTruth = Gia_ObjComputeTruthTable( pLib, Gia_ObjFanin0(pObjPo) );
p->timeTruth += Abc_Clock() - clk;
        // semi-canonicize
clk = Abc_Clock();
        memcpy( p->pTemp1, pTruth, p->nWords * sizeof(word) );
#ifdef LMS_USE_OLD_FORM
        uCanonPhase = Kit_TruthSemiCanonicize( (unsigned *)p->pTemp1, (unsigned *)p->pTemp2, nLeaves, pCanonPerm );
#else
        uCanonPhase = Abc_TtCanonicize( p->pTemp1, nLeaves, pCanonPerm );
#endif
        Abc_TtStretch5( (unsigned *)p->pTemp1, nLeaves, p->nVars );
p->timeCanon += Abc_Clock() - clk;
        // pCanonPerm and uCanonPhase show what was the variable corresponding to each var in the current truth
        if ( nLeaves == 2 && Abc_TtSupportSize(pTruth, 2) != 2 )
            continue;

clk = Abc_Clock();
        // map cut leaves into elementary variables of GIA
        for ( i = 0; i < nLeaves; i++ )
            Gia_ManCi( pLib, pCanonPerm[i] )->Value = Abc_Var2Lit( Gia_ObjId(pGia, Gia_ManPi(pGia, i)), (uCanonPhase >> i) & 1 );
        // build internal nodes
        assert( Vec_IntSize(pLib->vTtNodes) > 0 );
        Gia_ManForEachObjVec( pLib->vTtNodes, pLib, pTemp, i )
        {
            iFanin0 = Abc_LitNotCond( Gia_ObjFanin0(pTemp)->Value, Gia_ObjFaninC0(pTemp) );
            iFanin1 = Abc_LitNotCond( Gia_ObjFanin1(pTemp)->Value, Gia_ObjFaninC1(pTemp) );
            pTemp->Value = Gia_ManHashAnd( pGia, iFanin0, iFanin1 );
        }
p->timeBuild += Abc_Clock() - clk;

        // check if this node is already driving a PO
        assert( Gia_ObjIsAnd(pTemp) );
        pDriver = Gia_ManObj(pGia, Abc_Lit2Var(pTemp->Value));
        if ( pDriver->fMark1 )
        {
            p->nFilterSame++;
            continue;
        }
        pDriver->fMark1 = 1;
        // create output
        Gia_ManAppendCo( pGia, Abc_LitNotCond( pTemp->Value, (uCanonPhase >> nLeaves) & 1 ) );

        // verify truth table
        if ( fCheck )
        {
clk = Abc_Clock();
        pTemp = Gia_ManCo(pGia, Gia_ManCoNum(pGia)-1);
        pTruth = Gia_ObjComputeTruthTable( pGia, Gia_ManCo(pGia, Gia_ManCoNum(pGia)-1) );
p->timeCheck += Abc_Clock() - clk;
        if ( memcmp( p->pTemp1, pTruth, p->nWords * sizeof(word) ) != 0 )
        {
    
            Kit_DsdPrintFromTruth( (unsigned *)pTruth, nLeaves ); printf( "\n" );
            Kit_DsdPrintFromTruth( (unsigned *)p->pTemp1, nLeaves ); printf( "\n" );
            printf( "Truth table verification has failed.\n" );
    
            // drive PO with constant
            Gia_ManPatchCoDriver( pGia, Gia_ManCoNum(pGia)-1, 0 );
            // save truth table ID
            Vec_IntPush( p->vTruthIds, -1 );
            p->nFilterTruth++;
            continue;
        }
        }

clk = Abc_Clock();
        // add the resulting truth table to the hash table 
        Index = Vec_MemHashInsert( p->vTtMem, p->pTemp1 );
        // save truth table ID
        Vec_IntPush( p->vTruthIds, Index );
        assert( Gia_ManCoNum(pGia) == Vec_IntSize(p->vTruthIds) );
        p->nAdded++;
p->timeInsert += Abc_Clock() - clk;
    }
    Vec_StrFree( vSupps );
p->timeTotal += Abc_Clock() - clk2;
}


/**Function*************************************************************

  Synopsis    [Evaluates one cut during library construction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRecAddCut3( If_Man_t * pIfMan, If_Obj_t * pRoot, If_Cut_t * pCut )
{
    Lms_Man_t * p = s_pMan3;
    char pCanonPerm[LMS_VAR_MAX];
    unsigned uCanonPhase;
    int i, Index, iFanin0, iFanin1, fHole;
    int nLeaves = If_CutLeaveNum(pCut);
    Vec_Ptr_t * vNodes = p->vNodes;
    Gia_Man_t * pGia = p->pGia;
    Gia_Obj_t * pDriver;
    If_Obj_t * pIfObj = NULL;
    word * pTruth;
    abctime clk;
    p->nTried++;

    // skip small cuts
    assert( p->nVars == (int)pCut->nLimit );
    if ( nLeaves < 2 || (nLeaves == 2 && Abc_TtSupportSize(If_CutTruthW(pIfMan, pCut), 2) != 2) )
    {
        p->nFilterSize++;
        return 1;
    }

//    if ( p->vTtMem2 )
//        Vec_MemHashInsert( p->vTtMem2, If_CutTruthW(pCut) );

    // semi-canonicize truth table
clk = Abc_Clock();
    memcpy( p->pTemp1, If_CutTruthW(pIfMan, pCut), p->nWords * sizeof(word) );
#ifdef LMS_USE_OLD_FORM
    uCanonPhase = Kit_TruthSemiCanonicize( (unsigned *)p->pTemp1, (unsigned *)p->pTemp2, nLeaves, pCanonPerm );
#else
    uCanonPhase = Abc_TtCanonicize( p->pTemp1, nLeaves, pCanonPerm );
#endif
    Abc_TtStretch5( (unsigned *)p->pTemp1, nLeaves, p->nVars );
p->timeCanon += Abc_Clock() - clk;
    // pCanonPerm and uCanonPhase show what was the variable corresponding to each var in the current truth

    if ( p->pGia == NULL )
    {
clk = Abc_Clock();
        // add the resulting truth table to the hash table 
        Index = Vec_MemHashInsert( p->vTtMem, p->pTemp1 );
/*
        if ( p->vTruthFreqs == NULL )
            p->vTruthFreqs = Vec_IntAlloc( 1000 );
        assert( Index <= Vec_IntSize(p->vTruthFreqs)  );
        if ( Index < Vec_IntSize(p->vTruthFreqs) )
            Vec_IntAddToEntry( p->vTruthFreqs, Index, 1 );
        else
            Vec_IntPush( p->vTruthFreqs, 1 );
*/
        p->nAdded++;
p->timeInsert += Abc_Clock() - clk;
        return 1;
    }

    // collect internal nodes and skip redundant cuts
clk = Abc_Clock();
    If_CutTraverse( pIfMan, pRoot, pCut, vNodes );
p->timeTruth += Abc_Clock() - clk;
    if ( Vec_PtrSize(vNodes) > 253 )
    {
        p->nFilterSize++;
        return 1;
    }

clk = Abc_Clock();
    // map cut leaves into elementary variables of GIA
    for ( i = 0; i < nLeaves; i++ )
        If_ManObj( pIfMan, pCut->pLeaves[(int)pCanonPerm[i]] )->iCopy = Abc_Var2Lit( Gia_ObjId(pGia, Gia_ManPi(pGia, i)), (uCanonPhase >> i) & 1 );
    // build internal nodes
    fHole = 0;
    assert( Vec_PtrSize(vNodes) > 0 );
    Vec_PtrForEachEntryStart( If_Obj_t *, vNodes, pIfObj, i, nLeaves )
    {
        if ( If_ObjIsCi(pIfObj) )
        {
            pIfObj->iCopy = 0;
            fHole = 1;
            continue;
        }
        iFanin0 = Abc_LitNotCond( If_ObjFanin0(pIfObj)->iCopy, If_ObjFaninC0(pIfObj) );
        iFanin1 = Abc_LitNotCond( If_ObjFanin1(pIfObj)->iCopy, If_ObjFaninC1(pIfObj) );
        pIfObj->iCopy = Gia_ManHashAnd( pGia, iFanin0, iFanin1 );
    }
    p->nHoleInTheWall += fHole;
p->timeBuild += Abc_Clock() - clk;

    // check if this node is already driving a PO
    assert( If_ObjIsAnd(pIfObj) );
    pDriver = Gia_ManObj(pGia, Abc_Lit2Var(pIfObj->iCopy));
    if ( pDriver->fMark1 )
    {
        p->nFilterSame++;
        return 1;
    }
    pDriver->fMark1 = 1;
    // create output
    Gia_ManAppendCo( pGia, Abc_LitNotCond( pIfObj->iCopy, (uCanonPhase >> nLeaves) & 1 ) );

    // verify truth table
clk = Abc_Clock();
    pTruth = Gia_ObjComputeTruthTable( pGia, Gia_ManCo(pGia, Gia_ManCoNum(pGia)-1) );
p->timeCheck += Abc_Clock() - clk;
    if ( memcmp( p->pTemp1, pTruth, p->nWords * sizeof(word) ) != 0 )
    {
/*
        Kit_DsdPrintFromTruth( pTruth, nLeaves ); printf( "\n" );
        Kit_DsdPrintFromTruth( (unsigned *)p->pTemp1, nLeaves ); printf( "\n" );
        printf( "Truth table verification has failed.\n" );
*/
        // drive PO with constant
        Gia_ManPatchCoDriver( pGia, Gia_ManCoNum(pGia)-1, 0 );
        // save truth table ID
        Vec_IntPush( p->vTruthIds, -1 );
        p->nFilterTruth++;
        return 1;
    }

clk = Abc_Clock();
    // add the resulting truth table to the hash table 
    Index = Vec_MemHashInsert( p->vTtMem, p->pTemp1 );
    // save truth table ID
    Vec_IntPush( p->vTruthIds, Index );
    assert( Gia_ManCoNum(pGia) == Vec_IntSize(p->vTruthIds) );
    p->nAdded++;
p->timeInsert += Abc_Clock() - clk;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Top level procedure for library construction.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRecAdd3( Abc_Ntk_t * pNtk, int fUseSOPB )
{
    extern Abc_Ntk_t * Abc_NtkIf( Abc_Ntk_t * pNtk, If_Par_t * pPars );
    If_Par_t Pars, * pPars = &Pars;
    Abc_Ntk_t * pNtkNew;
    int clk = Abc_Clock();
    if ( Abc_NtkGetChoiceNum( pNtk ) )
        printf( "Performing recoding structures with choices.\n" );
    // remember that the manager was used for library construction
    s_pMan3->fLibConstr = 1;
    // create hash table if not available
    if ( s_pMan3->pGia && Vec_IntSize(&s_pMan3->pGia->vHTable) == 0 )
        Gia_ManHashStart( s_pMan3->pGia );

    // set defaults
    memset( pPars, 0, sizeof(If_Par_t) );
    // user-controlable paramters
    pPars->nLutSize    =  s_pMan3->nVars;
    pPars->nCutsMax    =  s_pMan3->nCuts;
    pPars->DelayTarget = -1;
    pPars->Epsilon     =  (float)0.005;
    pPars->fArea       =  1;
    // internal parameters
    if ( fUseSOPB )
    {
        pPars->fTruth      =  1;
        pPars->fCutMin     =  0;
        pPars->fUsePerm    =  1; 
        pPars->fDelayOpt   =  1;
    }
    else
    {
        pPars->fTruth      =  1;
        pPars->fCutMin     =  1;
        pPars->fUsePerm    =  0; 
        pPars->fDelayOpt   =  0;
    }
    pPars->fSkipCutFilter = 0;
    pPars->pFuncCost   =  NULL;
    pPars->pFuncUser   =  Abc_NtkRecAddCut3;
    // perform recording
    pNtkNew = Abc_NtkIf( pNtk, pPars );
    Abc_NtkDelete( pNtkNew );
s_pMan3->timeTotal += Abc_Clock() - clk;
}

 /**Function*************************************************************

  Synopsis    [Returns min AIG level at the output fo the cut using the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int If_CutComputeDelay( If_Man_t * p, If_Cut_t * pCut, char * pCanonPerm, word DelayProfile )
{
    If_Obj_t* pLeaf;
    int nLeaves = If_CutLeaveNum(pCut);
    int i, delayTemp, delayMax = -ABC_INFINITY;
    for ( i = 0; i < nLeaves; i++ )
    {
        pLeaf     = If_ManObj(p, (pCut)->pLeaves[(int)pCanonPerm[i]]);
        delayTemp = If_ObjCutBest(pLeaf)->Delay + Lms_DelayGet(DelayProfile, i);
        delayMax  = Abc_MaxInt( delayMax, delayTemp );
    }
    return delayMax;
}
static inline int If_CutFindBestStruct( If_Man_t * pIfMan, If_Cut_t * pCut, char * pCanonPerm, unsigned * puCanonPhase, int * pBestPo )
{
    Lms_Man_t * p = s_pMan3;
    int i, * pTruthId, iFirstPo, iFirstPoNext, iBestPo;
    int BestDelay = ABC_INFINITY, BestArea = ABC_INFINITY, Delay, Area;
    int uSupport, nLeaves = If_CutLeaveNum( pCut );
    char * pPerm = If_CutPerm( pCut );
    word DelayProfile;
    abctime clk;
    pCut->fUser = 1;
    // compute support
    uSupport = Abc_TtSupport( If_CutTruthW(pIfMan, pCut), nLeaves );
    if ( uSupport == 0 )
    {
        pCut->Cost = 1;
        for ( i = 0; i < nLeaves; i++ )
            pPerm[i] = IF_BIG_CHAR;
        return 0;
    }
    if ( !Abc_TtSuppIsMinBase(uSupport) || uSupport == 1 )
    {
        assert( Abc_TtSuppOnlyOne(uSupport) );
        pCut->Cost = 1;
        for ( i = 0; i < nLeaves; i++ )
            pPerm[i] = IF_BIG_CHAR;
        pPerm[Abc_TtSuppFindFirst(uSupport)] = 0;
        return If_ObjCutBest(If_ManObj(pIfMan, pCut->pLeaves[Abc_TtSuppFindFirst(uSupport)]))->Delay;
    }
    assert( Gia_WordCountOnes(uSupport) == nLeaves );

    // semicanonicize the function
clk = Abc_Clock();
    memcpy( p->pTemp1, If_CutTruthW(pIfMan, pCut), p->nWords * sizeof(word) );
#ifdef LMS_USE_OLD_FORM
    *puCanonPhase = Kit_TruthSemiCanonicize( (unsigned *)p->pTemp1, (unsigned *)p->pTemp2, nLeaves, pCanonPerm );
#else
    *puCanonPhase = Abc_TtCanonicize( p->pTemp1, nLeaves, pCanonPerm );
#endif
    Abc_TtStretch5( (unsigned *)p->pTemp1, nLeaves, p->nVars );
p->timeCanon += Abc_Clock() - clk;

    // get TT ID for the given class
    pTruthId = Vec_MemHashLookup( p->vTtMem, p->pTemp1 );
    if ( *pTruthId == -1 )
    {
        pCut->Cost = IF_COST_MAX;
        pCut->fUseless = 1;
        return ABC_INFINITY;
    }

    // note that array p->vTruthPo contains the first PO for the given truth table
    // other POs belonging to the same equivalence class follow immediately after this one
    // to iterate through the POs, we need to perform the following steps

    // find the first PO of this class
    iFirstPo = Vec_IntEntry( p->vTruthPo, *pTruthId );
    // find the first PO of the next class
    iFirstPoNext = Vec_IntEntry( p->vTruthPo, *pTruthId+1 );
    // iterate through the subgraphs of this class
    iBestPo = -1;
    for ( i = iFirstPo; i < iFirstPoNext; i++ )
    {
        Delay = If_CutComputeDelay( pIfMan, pCut, pCanonPerm, Vec_WrdEntry(p->vDelays, i) );
        Area  = Vec_StrEntry(p->vAreas, i);
        if ( iBestPo == -1 || BestDelay > Delay || (BestDelay == Delay && BestArea > Area) )
        {
            iBestPo = i;
            BestDelay = Delay;
            BestArea = Area;
        }
    }
    if ( pBestPo )
        *pBestPo = iBestPo;

    // mark as user cut.
    DelayProfile = Vec_WrdEntry(p->vDelays, iBestPo);
    pCut->Cost = Vec_StrEntry(p->vAreas, iBestPo);
    for ( i = 0; i < nLeaves; i++ )
        pPerm[(int)pCanonPerm[i]] = Lms_DelayGet(DelayProfile, i);
    if ( 0 )
    {
        int Max = 0, Two = 0, pTimes[16];
        for ( i = 0; i < nLeaves; i++ )
            pTimes[i] = (int)If_ObjCutBest(If_CutLeaf(pIfMan, pCut, i))->Delay;
        for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
            Max = Abc_MaxInt( Max, pTimes[i] );
        for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
            if ( pTimes[i] != Max )
                Two = Abc_MaxInt( Two, pTimes[i] );
        if ( Two + 2 < Max && Max + 3 < BestDelay )
        {
            for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
                printf( "%3d ", pTimes[i] );
            for ( ; i < pIfMan->pPars->nLutSize; i++ )
                printf( "    " );
            printf( "-> %3d   ", BestDelay );
            Dau_DsdPrintFromTruth( If_CutTruthW(pIfMan, pCut), If_CutLeaveNum(pCut) );
        }
    }
    return BestDelay; 
}
int If_CutDelayRecCost3( If_Man_t * pIfMan, If_Cut_t * pCut, If_Obj_t * pObj )
{
    Lms_Man_t * p = s_pMan3;
    char pCanonPerm[LMS_VAR_MAX];
    unsigned uCanonPhase = 0;
    // make sure the cut functions match the library
    assert( p->nVars == (int)pCut->nLimit );
    // if this assertion fires, it means that LMS manager was used for library construction
    // in this case, GIA has to be written out and the manager restarted as described above
    assert( !p->fLibConstr );
    if ( p->vTruthPo == NULL )
        Lms_ManPrepare( p );
    // return the delay of the best structure
    return If_CutFindBestStruct( pIfMan, pCut, pCanonPerm, &uCanonPhase, NULL );
}

/**Function*************************************************************

  Synopsis    [Reexpresses the best structure of the cut in the HOP manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Abc_RecToHop3( Hop_Man_t * pMan, If_Man_t * pIfMan, If_Cut_t * pCut, If_Obj_t * pIfObj )
{
    Lms_Man_t * p = s_pMan3;
    char pCanonPerm[LMS_VAR_MAX];
    unsigned uCanonPhase = 0;
    Hop_Obj_t * pFan0, * pFan1, * pHopObj;
    Gia_Man_t * pGia = p->pGia;
    Gia_Obj_t * pGiaPo, * pGiaTemp = NULL;
    int i, uSupport, BestPo = -1, nLeaves = If_CutLeaveNum(pCut);
    assert( pIfMan->pPars->fCutMin == 1 );

    // compute support
    uSupport = Abc_TtSupport( If_CutTruthW(pIfMan, pCut), nLeaves );
    if ( uSupport == 0 )
        return Hop_NotCond( Hop_ManConst0(pMan), If_CutTruthIsCompl(pCut) );
    if ( !Abc_TtSuppIsMinBase(uSupport) || uSupport == 1 )
    {
        assert( Abc_TtSuppOnlyOne(uSupport) );
        return Hop_NotCond( Hop_IthVar(pMan, Abc_TtSuppFindFirst(uSupport)), If_CutTruthIsCompl(pCut) );
    }
    assert( Gia_WordCountOnes(uSupport) == nLeaves );

    // get the best output for this node
    If_CutFindBestStruct( pIfMan, pCut, pCanonPerm, &uCanonPhase, &BestPo );
    assert( BestPo >= 0 );
    pGiaPo = Gia_ManCo( pGia, BestPo );
    // collect internal nodes into pGia->vTtNodes
    if ( pGia->vTtNodes == NULL )
        pGia->vTtNodes = Vec_IntAlloc( 256 );
    assert( Gia_ObjIsAnd( Gia_ObjFanin0(pGiaPo) ) );
    Gia_ObjCollectInternal( pGia, Gia_ObjFanin0(pGiaPo) );
    assert( Vec_IntSize(pGia->vTtNodes) > 0 );

    // collect HOP nodes for leaves
    Vec_PtrClear( p->vLabelsP );
    for ( i = 0; i < nLeaves; i++ )
        Vec_PtrPush( p->vLabelsP, Hop_NotCond(Hop_IthVar(pMan, pCanonPerm[i]), (uCanonPhase >> i) & 1) );

    // compute HOP nodes for internal nodes
    Gia_ManForEachObjVec( pGia->vTtNodes, pGia, pGiaTemp, i )
    {
        pGiaTemp->fMark0 = 0; // unmark node marked by Gia_ObjCollectInternal()
        if ( Gia_ObjIsAnd(Gia_ObjFanin0(pGiaTemp)) )
            pFan0 = (Hop_Obj_t *)Vec_PtrEntry(p->vLabelsP, Gia_ObjNum(pGia, Gia_ObjFanin0(pGiaTemp)) + nLeaves);
        else
            pFan0 = (Hop_Obj_t *)Vec_PtrEntry(p->vLabelsP, Gia_ObjCioId(Gia_ObjFanin0(pGiaTemp)));
        pFan0 = Hop_NotCond(pFan0, Gia_ObjFaninC0(pGiaTemp));
        if ( Gia_ObjIsAnd(Gia_ObjFanin1(pGiaTemp)) )
            pFan1 = (Hop_Obj_t *)Vec_PtrEntry(p->vLabelsP, Gia_ObjNum(pGia, Gia_ObjFanin1(pGiaTemp)) + nLeaves);
        else
            pFan1 = (Hop_Obj_t *)Vec_PtrEntry(p->vLabelsP, Gia_ObjCioId(Gia_ObjFanin1(pGiaTemp)));
        pFan1 = Hop_NotCond(pFan1, Gia_ObjFaninC1(pGiaTemp));

        pHopObj = Hop_And(pMan, pFan0, pFan1);
        Vec_PtrPush(p->vLabelsP, pHopObj);
    }
    // get the final result
    assert( Gia_ObjIsAnd(pGiaTemp) );
    pHopObj = (Hop_Obj_t *)Vec_PtrEntry(p->vLabelsP, Gia_ObjNum(pGia, pGiaTemp) + nLeaves);
    // complement the result if needed
    return Hop_NotCond( pHopObj,  Gia_ObjFaninC0(pGiaPo) ^ ((uCanonPhase >> nLeaves) & 1) );    
}

/**Function*************************************************************

  Synopsis    [Reexpresses the best structure of the cut in the GIA manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_RecToGia3( Gia_Man_t * pMan, If_Man_t * pIfMan, If_Cut_t * pCut, Vec_Int_t * vLeaves, int fHash )
{
    Lms_Man_t * p = s_pMan3;
    char pCanonPerm[LMS_VAR_MAX];
    unsigned uCanonPhase = 0;
    int iFan0, iFan1, iGiaObj;
    Gia_Man_t * pGia = p->pGia;
    Gia_Obj_t * pGiaPo, * pGiaTemp = NULL;
    int i, uSupport, BestPo = -1, nLeaves = If_CutLeaveNum(pCut);
    assert( pIfMan->pPars->fCutMin == 1 );
    assert( nLeaves == Vec_IntSize(vLeaves) );

    // compute support
    uSupport = Abc_TtSupport( If_CutTruthW(pIfMan, pCut), nLeaves );
    if ( uSupport == 0 )
        return Abc_LitNotCond( 0, If_CutTruthIsCompl(pCut) );
    if ( !Abc_TtSuppIsMinBase(uSupport) || uSupport == 1 )
    {
        assert( Abc_TtSuppOnlyOne(uSupport) );
        return Abc_LitNotCond( Vec_IntEntry(vLeaves, Abc_TtSuppFindFirst(uSupport)), If_CutTruthIsCompl(pCut) );
    }
    assert( Gia_WordCountOnes(uSupport) == nLeaves );

    // get the best output for this node
    If_CutFindBestStruct( pIfMan, pCut, pCanonPerm, &uCanonPhase, &BestPo );
    assert( BestPo >= 0 );
    pGiaPo = Gia_ManCo( pGia, BestPo );

    // collect internal nodes into pGia->vTtNodes
    if ( pGia->vTtNodes == NULL )
        pGia->vTtNodes = Vec_IntAlloc( 256 );
    assert( Gia_ObjIsAnd( Gia_ObjFanin0(pGiaPo) ) );
    Gia_ObjCollectInternal( pGia, Gia_ObjFanin0(pGiaPo) );
    assert( Vec_IntSize(pGia->vTtNodes) > 0 );

    // collect GIA nodes for leaves
    Vec_IntClear( p->vLabels );
    for (i = 0; i < nLeaves; i++)
        Vec_IntPush( p->vLabels, Abc_LitNotCond(Vec_IntEntry(vLeaves, pCanonPerm[i]), (uCanonPhase >> i) & 1) );

    // compute HOP nodes for internal nodes
    Gia_ManForEachObjVec( pGia->vTtNodes, pGia, pGiaTemp, i )
    {
        pGiaTemp->fMark0 = 0; // unmark node marked by Gia_ObjCollectInternal()
        if ( Gia_ObjIsAnd(Gia_ObjFanin0(pGiaTemp)) )
            iFan0 = Vec_IntEntry(p->vLabels, Gia_ObjNum(pGia, Gia_ObjFanin0(pGiaTemp)) + nLeaves);
        else
            iFan0 = Vec_IntEntry(p->vLabels, Gia_ObjCioId(Gia_ObjFanin0(pGiaTemp)));
        iFan0 = Abc_LitNotCond(iFan0, Gia_ObjFaninC0(pGiaTemp));
        if ( Gia_ObjIsAnd(Gia_ObjFanin1(pGiaTemp)) )
            iFan1 = Vec_IntEntry(p->vLabels, Gia_ObjNum(pGia, Gia_ObjFanin1(pGiaTemp)) + nLeaves);
        else
            iFan1 = Vec_IntEntry(p->vLabels, Gia_ObjCioId(Gia_ObjFanin1(pGiaTemp)));
        iFan1 = Abc_LitNotCond(iFan1, Gia_ObjFaninC1(pGiaTemp));
        if ( fHash )
            iGiaObj = Gia_ManHashAnd(pMan, iFan0, iFan1);
        else
            iGiaObj = Gia_ManAppendAnd(pMan, iFan0, iFan1);
        Vec_IntPush(p->vLabels, iGiaObj);
    }
    // get the final result
    assert( Gia_ObjIsAnd(pGiaTemp) );
    iGiaObj = Vec_IntEntry(p->vLabels, Gia_ObjNum(pGia, pGiaTemp) + nLeaves);
    // complement the result if needed
    return Abc_LitNotCond( iGiaObj,  Gia_ObjFaninC0(pGiaPo) ^ ((uCanonPhase >> nLeaves) & 1) ^ pCut->fCompl );    
}


/**Function*************************************************************

  Synopsis    [Reduces GIA to contain only useful COs and internal nodes.]

  Description [During library construction, redundant nodes are added.
  Some COs are found to be useless because their TT does not match the
  (semi-canonicized TT) of the cut, etc.  This procedure reduces GIA
  to contains only useful (non-redundant, non-dominated) COs and the
  corresponding internal nodes. This procedure replaces GIA by a new GIA
  and creates new vTruthIds. The COs with the same truth table have
  adjacent IDs. This procedure does not change the truth tables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// count how many times TT occurs
Vec_Int_t * Lms_GiaCountTruths( Lms_Man_t * p )
{
    Vec_Int_t * vCounts = Vec_IntStart( Vec_MemEntryNum(p->vTtMem) );
    int i, Entry;
    Vec_IntForEachEntry( p->vTruthIds, Entry, i )
        if ( Entry >= 0 )
            Vec_IntAddToEntry( vCounts, Entry, 1 );
    return vCounts;
}
// collect PO indexes worth visiting
Vec_Int_t * Lms_GiaCollectUsefulCos( Lms_Man_t * p )
{
    Vec_Int_t * vBegins = Vec_IntAlloc( Vec_MemEntryNum(p->vTtMem) );
    Vec_Int_t * vUseful = Vec_IntStartFull( Gia_ManCoNum(p->pGia) + Vec_MemEntryNum(p->vTtMem) );
    Vec_Int_t * vCounts = Lms_GiaCountTruths( p );
    int i, Entry, * pPlace, SumTotal = 0;
    // mark up the place for POs
    Vec_IntForEachEntry( vCounts, Entry, i )
    {
        assert( Entry > 0 );
        Vec_IntPush( vBegins, SumTotal );
        SumTotal += Entry + 1;
//        printf( "%d ", Entry );
    }
    Vec_IntPush( vBegins, SumTotal );
    // fill out POs in their places
    Vec_IntFill( vCounts, Vec_IntSize(vCounts), 0 );
    Vec_IntForEachEntry( p->vTruthIds, Entry, i )
    {
        if ( Entry < 0 )
            continue;
        pPlace = Vec_IntEntryP( vUseful, Vec_IntEntry(vBegins, Entry) + Vec_IntEntry(vCounts, Entry) );
        assert( *pPlace == -1 );
        *pPlace = i;
        Vec_IntAddToEntry( vCounts, Entry, 1 );
    }
    Vec_IntFree( vBegins );
    Vec_IntFree( vCounts );
    return vUseful;
}
// collect non-dominated COs
Vec_Int_t * Lms_GiaFindNonRedundantCos( Lms_Man_t * p )
{
    Vec_Int_t * vRemain;
    Vec_Int_t * vUseful;
    Vec_Wrd_t * vDelays;
    int i, k, EntryI, EntryK;
    word D1, D2;
    vDelays = Lms_GiaDelays( p->pGia );
    vUseful = Lms_GiaCollectUsefulCos( p );
    Vec_IntForEachEntry( vUseful, EntryI, i )
    {
        if ( EntryI < 0 )
            continue;
        D1 = Vec_WrdEntry(vDelays, EntryI);
        assert( D1 > 0 );
        Vec_IntForEachEntryStart( vUseful, EntryK, k, i+1 )
        {
            if ( EntryK == -1 )
                break;
            if ( EntryK == -2 )
                continue;
            D2 = Vec_WrdEntry(vDelays, EntryK);
            assert( D2 > 0 );
            if ( Lms_DelayDom(D1, D2, Gia_ManCiNum(p->pGia)) ) // D1 dominate D2
            {
                Vec_IntWriteEntry( vUseful, k, -2 );
                continue;
            }
            if ( Lms_DelayDom(D2, D1, Gia_ManCiNum(p->pGia)) ) // D2 dominate D1
            {
                Vec_IntWriteEntry( vUseful, i, -2 );
                break;
            }
        }
    }

    vRemain = Vec_IntAlloc( 1000 );
    Vec_IntForEachEntry( vUseful, EntryI, i )
        if ( EntryI >= 0 )
            Vec_IntPush( vRemain, EntryI );
    Vec_IntFree( vUseful );
    Vec_WrdFree( vDelays );
    return vRemain;
}
// replace GIA and vTruthIds by filtered ones
void Lms_GiaNormalize( Lms_Man_t * p )
{
    Gia_Man_t * pGiaNew;
    Gia_Obj_t * pObj;
    Vec_Int_t * vRemain;
    Vec_Int_t * vTruthIdsNew;
    int i, Entry, Prev = -1, Next;
    // collect non-redundant COs
    vRemain = Lms_GiaFindNonRedundantCos( p );
    // change these to be useful literals
    vTruthIdsNew = Vec_IntAlloc( Vec_IntSize(vRemain) );
    Vec_IntForEachEntry( vRemain, Entry, i )
    {
        pObj = Gia_ManCo(p->pGia, Entry);
        assert( Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) );
        Vec_IntWriteEntry( vRemain, i, Gia_ObjFaninLit0p(p->pGia, pObj) );
        // create new truth IDs
        Next = Vec_IntEntry(p->vTruthIds, Gia_ObjCioId(pObj));
        assert( Prev <= Next );
        Vec_IntPush( vTruthIdsNew, Next );
        Prev = Next;
    }
    // create a new GIA
    Gia_ManForEachObj( p->pGia, pObj, i )
        assert( pObj->fMark0 == 0 );
    for ( i = 0; i < Gia_ManCoNum(p->pGia); i++ )
        Gia_ManPatchCoDriver( p->pGia, i, 0 );
    Vec_IntForEachEntry( vRemain, Entry, i )
        Gia_ManAppendCo( p->pGia, Entry );
//    pGiaNew = Gia_ManCleanup( p->pGia );
    pGiaNew = Gia_ManCleanupOutputs( p->pGia, Gia_ManCoNum(p->pGia) - Vec_IntSize(vRemain) );
    Gia_ManStop( p->pGia );
    p->pGia = pGiaNew;
    Vec_IntFree( vRemain );
    // update truth IDs
    Vec_IntFree( p->vTruthIds );
    p->vTruthIds = vTruthIdsNew;
//    Vec_IntPrint( vTruthIdsNew );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRecTruthCompare( int * p1, int * p2 ) 
{
    int Diff = Vec_StrEntry( s_pMan3->vSupps, *p1 ) - Vec_StrEntry( s_pMan3->vSupps, *p2 );
    if ( Diff )
        return Diff;
    return memcmp( Vec_MemReadEntry(s_pMan3->vTtMem, *p1), Vec_MemReadEntry(s_pMan3->vTtMem, *p2), sizeof(word) * s_pMan3->nWords ); 
}
void Abc_NtkRecDumpTt3( char * pFileName, int fBinary )
{
    FILE * pFile;
    char pBuffer[1000];
    Lms_Man_t * p = s_pMan3;
    Vec_Int_t * vEntries;
    word * pTruth;
    int i, Entry, nVars = p->nVars;
    int nEntries = Vec_MemEntryNum(p->vTtMem);
    if ( nEntries == 0 )
    {
        printf( "There is not truth tables.\n" );
        return;
    }
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "The file cannot be opened.\n" );
        return;
    }
    p->vSupps = Vec_StrAlloc( nEntries );
    Vec_MemForEachEntry( p->vTtMem, pTruth, i )
        Vec_StrPush( p->vSupps, (char)Abc_TtSupportSize(pTruth, nVars) );
    vEntries = Vec_IntStartNatural( nEntries );
    qsort( (void *)Vec_IntArray(vEntries), (size_t)nEntries, sizeof(int), (int(*)(const void *,const void *))Abc_NtkRecTruthCompare );
    Vec_StrFreeP( &p->vSupps );
    // write the file
    Vec_IntForEachEntry( vEntries, Entry, i )
    {
        pTruth = Vec_MemReadEntry(p->vTtMem, Entry);
        if ( fBinary )
        {
            fwrite( pTruth, 1, sizeof(word) * p->nWords, pFile );
            continue;
        }
        Extra_PrintHex( pFile, (unsigned *)pTruth, nVars );
        fprintf( pFile, "  " );
//        Kit_DsdWriteFromTruth( pBuffer, (unsigned *)pTruth, nVars );
        Dau_DsdDecompose( pTruth, p->nVars, 0, (int)(nVars <= 10), pBuffer );
        fprintf( pFile, "%s\n", pBuffer );
    }
    fclose( pFile );
    Vec_IntFree( vEntries );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRecInputNum3()
{
    return Gia_ManCiNum(s_pMan3->pGia);
}
int Abc_NtkRecIsRunning3()
{
    return s_pMan3 != NULL;
}
Gia_Man_t * Abc_NtkRecGetGia3()
{
    abctime clk = Abc_Clock();
    printf( "Before normalizing: Library has %d classes and %d AIG subgraphs with %d AND nodes.\n", 
        Vec_MemEntryNum(s_pMan3->vTtMem), Gia_ManPoNum(s_pMan3->pGia), Gia_ManAndNum(s_pMan3->pGia) );
    Lms_GiaNormalize( s_pMan3 );
    printf( "After normalizing:  Library has %d classes and %d AIG subgraphs with %d AND nodes.\n", 
        Vec_MemEntryNum(s_pMan3->vTtMem), Gia_ManPoNum(s_pMan3->pGia), Gia_ManAndNum(s_pMan3->pGia) );
    Abc_PrintTime( 1, "Normalization runtime", Abc_Clock() - clk );
    s_pMan3->fLibConstr = 0;
    return s_pMan3->pGia;
}
void Abc_NtkRecPs3(int fPrintLib)
{
    Lms_ManPrint( s_pMan3 );
}
void Abc_NtkRecStart3( Gia_Man_t * p, int nVars, int nCuts, int fFuncOnly, int fVerbose )
{
    assert( s_pMan3 == NULL );
    s_pMan3 = Lms_ManStart( p, nVars, nCuts, fFuncOnly, fVerbose );
}

void Abc_NtkRecStop3()
{
    assert( s_pMan3 != NULL );
    Lms_ManStop( s_pMan3 );
    s_pMan3 = NULL;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
