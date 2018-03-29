/**CFile****************************************************************

  FileName    [giaLf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Cut computation.]

  Author      [Alan Mishchenko]`
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaLf.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/tim/tim.h"
#include "misc/vec/vecSet.h"
#include "misc/vec/vecMem.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define LF_LEAF_MAX   13
#define LF_CUT_MAX    32
#define LF_LOG_PAGE   12
#define LF_NO_LEAF   255
#define LF_CUT_WORDS (4+LF_LEAF_MAX/2)
#define LF_TT_WORDS  ((LF_LEAF_MAX > 6) ? 1 << (LF_LEAF_MAX-6) : 1)
#define LF_EPSILON 0.005

typedef struct Lf_Cut_t_ Lf_Cut_t; 
struct Lf_Cut_t_
{
    word            Sign;            // signature
    int             Delay;           // delay
    float           Flow;            // flow 
    int             iFunc;           // functionality
    unsigned        Cost      : 22;  // misc cut cost
    unsigned        fLate     :  1;  // fails timing
    unsigned        fMux7     :  1;  // specialized cut
    unsigned        nLeaves   :  8;  // the number of leaves
    int             pLeaves[0];      // leaves
};
typedef struct Lf_Plc_t_ Lf_Plc_t; 
struct Lf_Plc_t_
{
    unsigned        fUsed    :  1;   // the cut is used
    unsigned        Handle   : 31;   // the cut handle
};
typedef struct Lf_Bst_t_ Lf_Bst_t; 
struct Lf_Bst_t_
{
    int             Delay[3];        // delay
    float           Flow[3];         // flow 
    Lf_Plc_t        Cut[2];          // cut info
};
typedef struct Lf_Mem_t_ Lf_Mem_t; 
struct Lf_Mem_t_
{
    int             LogPage;         // log size of memory page
    int             MaskPage;        // page mask
    int             nCutWords;       // cut size in words
    int             iCur;            // writing position 
    Vec_Ptr_t       vPages;          // memory pages
    Vec_Ptr_t *     vFree;           // free pages 
};
typedef struct Lf_Man_t_ Lf_Man_t; 
struct Lf_Man_t_
{
    // user data
    Gia_Man_t *     pGia;            // manager
    Jf_Par_t *      pPars;           // parameters
    // cut data
    int             nCutWords;       // cut size in words
    int             nSetWords;       // set size in words
    Lf_Bst_t *      pObjBests;       // best cuts
    Vec_Ptr_t       vMemSets;        // memory for cutsets
    Vec_Int_t       vFreeSets;       // free cutsets
    Vec_Mem_t *     vTtMem;          // truth tables
    Vec_Ptr_t       vFreePages;      // free memory pages
    Lf_Mem_t        vStoreOld;       // previous cuts
    Lf_Mem_t        vStoreNew;       // current cuts
    // mapper data
    Vec_Int_t       vOffsets;        // offsets
    Vec_Int_t       vRequired;       // required times
    Vec_Int_t       vCutSets;        // cutsets (pObj->Value stores cut refs)
    Vec_Flt_t       vFlowRefs;       // flow refs
    Vec_Int_t       vMapRefs;        // mapping refs
    Vec_Flt_t       vSwitches;       // switching activity
    Vec_Int_t       vCiArrivals;     // arrival times of the CIs
    // statistics
    abctime         clkStart;        // starting time
    double          CutCount[4];     // cut counts
    double          Switches;        // switching activity
    int             nFrontMax;       // frontier
    int             nCoDrivers;      // CO drivers
    int             nInverters;      // inverters
    int             nTimeFails;      // timing fails
    int             Iter;            // mapping iteration
    int             fUseEla;         // use exact local area
    int             nCutMux;         // non-trivial MUX cuts
    int             nCutEqual;       // equal two cuts
    int             nCutCounts[LF_LEAF_MAX+1];
};

static inline void        Lf_CutCopy( Lf_Cut_t * p, Lf_Cut_t * q, int n ) { memcpy(p, q, sizeof(word) * n);                                         }
static inline Lf_Cut_t *  Lf_CutNext( Lf_Cut_t * p, int n )               { return (Lf_Cut_t *)((word *)p + n);                                     }
static inline word *      Lf_CutTruth( Lf_Man_t * p, Lf_Cut_t * pCut )    { return Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut->iFunc));           }

static inline int         Lf_ObjOff( Lf_Man_t * p, int i )                { return Vec_IntEntry(&p->vOffsets, i);                                   }
static inline int         Lf_ObjRequired( Lf_Man_t * p, int i )           { return Vec_IntEntry(&p->vRequired, i);                                  }
static inline void        Lf_ObjSetRequired( Lf_Man_t * p, int i, int t ) { Vec_IntDowndateEntry(&p->vRequired, i, t);                              }
static inline Lf_Bst_t *  Lf_ObjReadBest( Lf_Man_t * p, int i )           { return p->pObjBests + Lf_ObjOff(p,i);                                   }
static inline float       Lf_ObjFlowRefs( Lf_Man_t * p, int i )           { return Vec_FltEntry(&p->vFlowRefs, Lf_ObjOff(p,i));                     }
static inline int         Lf_ObjMapRefNum( Lf_Man_t * p, int i )          { return Vec_IntEntry(&p->vMapRefs, Lf_ObjOff(p,i));                      }
static inline int         Lf_ObjMapRefInc( Lf_Man_t * p, int i )          { return (*Vec_IntEntryP(&p->vMapRefs, Lf_ObjOff(p,i)))++;                }
static inline int         Lf_ObjMapRefDec( Lf_Man_t * p, int i )          { return --(*Vec_IntEntryP(&p->vMapRefs, Lf_ObjOff(p,i)));                }
static inline float       Lf_ObjSwitches( Lf_Man_t * p, int i )           { return Vec_FltEntry(&p->vSwitches, i);                                  }
static inline int         Lf_BestDiffCuts( Lf_Bst_t * p )                 { return p->Cut[0].Handle != p->Cut[1].Handle;                            }
static inline int         Lf_BestIsMapped( Lf_Bst_t * p )                 { return (int)(p->Cut[0].fUsed ^ p->Cut[1].fUsed);                        }
static inline int         Lf_BestIndex( Lf_Bst_t * p )                    { return p->Cut[1].fUsed;                                                 }
static inline int         Lf_BestCutIndex( Lf_Bst_t * p )                 { if (p->Cut[0].fUsed) return 0; if (p->Cut[1].fUsed) return 1; return 2; }

#define Lf_CutSetForEachCut( nWords, pCutSet, pCut, i, nCuts )  for ( i = 0, pCut = pCutSet; i < nCuts; pCut = Lf_CutNext(pCut, nWords), i++ ) 
#define Lf_CutForEachVar( pCut, Var, i )                        for ( i = 0; i < (int)pCut->nLeaves && (Var = pCut->pLeaves[i]); i++ ) if ( Lf_ObjOff(p, Var) < 0 ) {} else

extern int Kit_TruthToGia( Gia_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory, Vec_Int_t * vLeaves, int fHash );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Lf_ObjSetCiArrival( Lf_Man_t * p, int iCi, int Time )
{
    Vec_IntWriteEntry( &p->vCiArrivals, iCi, Time );
}
static inline int Lf_ObjCiArrival( Lf_Man_t * p, int iCi )
{
    return Vec_IntEntry( &p->vCiArrivals, iCi );
}
int Lf_ObjArrival_rec( Lf_Man_t * p, Gia_Obj_t * pDriver )
{
    if ( Gia_ObjIsBuf(pDriver) )
        return Lf_ObjArrival_rec( p, Gia_ObjFanin0(pDriver) );
    if ( Gia_ObjIsAnd(pDriver) )
        return Lf_ObjReadBest(p, Gia_ObjId(p->pGia, pDriver))->Delay[0];
    if ( Gia_ObjIsCi(pDriver) )
        return Lf_ObjCiArrival(p, Gia_ObjCioId(pDriver));
    return 0;
}
static inline int Lf_ObjCoArrival( Lf_Man_t * p, int iCo )
{
    Gia_Obj_t * pObj = Gia_ManCo(p->pGia, iCo);
    Gia_Obj_t * pDriver = Gia_ObjFanin0(pObj);
    return Lf_ObjArrival_rec( p, pDriver );
//    if ( Gia_ObjIsAnd(pDriver) )
//        return Lf_ObjReadBest(p, Gia_ObjId(p->pGia, pDriver))->Delay[0];
//    if ( Gia_ObjIsCi(pDriver) )
//        return Lf_ObjCiArrival(p, Gia_ObjCioId(pDriver));
//    return 0;
}
int Lf_ObjCoArrival2_rec( Lf_Man_t * p, Gia_Obj_t * pDriver )
{
    if ( Gia_ObjIsBuf(pDriver) )
        return Lf_ObjCoArrival2_rec( p, Gia_ObjFanin0(pDriver) );
    if ( Gia_ObjIsAnd(pDriver) )
    {
        Lf_Bst_t * pBest = Lf_ObjReadBest(p, Gia_ObjId(p->pGia, pDriver));
        int Index = Lf_BestCutIndex( pBest );
        assert( Index < 2 || Gia_ObjIsMux(p->pGia, pDriver) );
        return pBest->Delay[Index];
    }
    if ( Gia_ObjIsCi(pDriver) )
        return Lf_ObjCiArrival(p, Gia_ObjCioId(pDriver));
    return 0;
}
static inline int Lf_ObjCoArrival2( Lf_Man_t * p, int iCo )
{
    Gia_Obj_t * pObj = Gia_ManCo(p->pGia, iCo);
    Gia_Obj_t * pDriver = Gia_ObjFanin0(pObj);
    return Lf_ObjCoArrival2_rec( p, pDriver );
//    if ( Gia_ObjIsAnd(pDriver) )
//    {
//        Lf_Bst_t * pBest = Lf_ObjReadBest(p, Gia_ObjId(p->pGia, pDriver));
//        int Index = Lf_BestCutIndex( pBest );
//        assert( Index < 2 || Gia_ObjIsMux(p->pGia, pDriver) );
//        return pBest->Delay[Index];
//    }
//    if ( Gia_ObjIsCi(pDriver) )
//        return Lf_ObjCiArrival(p, Gia_ObjCioId(pDriver));
//    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lf_ManComputeCrossCut( Gia_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, nCutMax = 0, nCutCur = 0;
    assert( p->pMuxes == NULL );
    Gia_ManForEachObj( p, pObj, i )
        pObj->Value = 0;
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) )
            Gia_ObjFanin0(pObj)->Value++;
        if ( Gia_ObjIsAnd(Gia_ObjFanin1(pObj)) )
            Gia_ObjFanin1(pObj)->Value++;
    }
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( pObj->Value )
            nCutCur++;
        if ( nCutMax < nCutCur )
            nCutMax = nCutCur;
        if ( Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) && --Gia_ObjFanin0(pObj)->Value == 0 )
            nCutCur--;
        if ( Gia_ObjIsAnd(Gia_ObjFanin1(pObj)) && --Gia_ObjFanin1(pObj)->Value == 0 )
            nCutCur--;
    }
    assert( nCutCur == 0 );
    if ( nCutCur )
        printf( "Cutset is not 0\n" );
    Gia_ManForEachObj( p, pObj, i )
        assert( pObj->Value == 0 );
    printf( "CutMax = %d\n", nCutMax );
    return nCutMax;
}

/**Function*************************************************************

  Synopsis    [Detect MUX truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Lf_ManTtIsMux( word t )
{
    static unsigned s_Muxes[24] = {
        (~0xAAAAAAAA & ~0xCCCCCCCC) | ( 0xAAAAAAAA & ~0xF0F0F0F0),
        (~0xAAAAAAAA & ~0xCCCCCCCC) | ( 0xAAAAAAAA &  0xF0F0F0F0),
        (~0xAAAAAAAA &  0xCCCCCCCC) | ( 0xAAAAAAAA & ~0xF0F0F0F0),
        (~0xAAAAAAAA &  0xCCCCCCCC) | ( 0xAAAAAAAA &  0xF0F0F0F0),
        ( 0xAAAAAAAA & ~0xCCCCCCCC) | (~0xAAAAAAAA & ~0xF0F0F0F0),
        ( 0xAAAAAAAA & ~0xCCCCCCCC) | (~0xAAAAAAAA &  0xF0F0F0F0),
        ( 0xAAAAAAAA &  0xCCCCCCCC) | (~0xAAAAAAAA & ~0xF0F0F0F0),
        ( 0xAAAAAAAA &  0xCCCCCCCC) | (~0xAAAAAAAA &  0xF0F0F0F0),

        (~0xCCCCCCCC & ~0xAAAAAAAA) | ( 0xCCCCCCCC & ~0xF0F0F0F0),
        (~0xCCCCCCCC & ~0xAAAAAAAA) | ( 0xCCCCCCCC &  0xF0F0F0F0),
        (~0xCCCCCCCC &  0xAAAAAAAA) | ( 0xCCCCCCCC & ~0xF0F0F0F0),
        (~0xCCCCCCCC &  0xAAAAAAAA) | ( 0xCCCCCCCC &  0xF0F0F0F0),
        ( 0xCCCCCCCC & ~0xAAAAAAAA) | (~0xCCCCCCCC & ~0xF0F0F0F0),
        ( 0xCCCCCCCC & ~0xAAAAAAAA) | (~0xCCCCCCCC &  0xF0F0F0F0),
        ( 0xCCCCCCCC &  0xAAAAAAAA) | (~0xCCCCCCCC & ~0xF0F0F0F0),
        ( 0xCCCCCCCC &  0xAAAAAAAA) | (~0xCCCCCCCC &  0xF0F0F0F0),

        (~0xF0F0F0F0 & ~0xCCCCCCCC) | ( 0xF0F0F0F0 & ~0xAAAAAAAA),
        (~0xF0F0F0F0 & ~0xCCCCCCCC) | ( 0xF0F0F0F0 &  0xAAAAAAAA),
        (~0xF0F0F0F0 &  0xCCCCCCCC) | ( 0xF0F0F0F0 & ~0xAAAAAAAA),
        (~0xF0F0F0F0 &  0xCCCCCCCC) | ( 0xF0F0F0F0 &  0xAAAAAAAA),
        ( 0xF0F0F0F0 & ~0xCCCCCCCC) | (~0xF0F0F0F0 & ~0xAAAAAAAA),
        ( 0xF0F0F0F0 & ~0xCCCCCCCC) | (~0xF0F0F0F0 &  0xAAAAAAAA),
        ( 0xF0F0F0F0 &  0xCCCCCCCC) | (~0xF0F0F0F0 & ~0xAAAAAAAA),
        ( 0xF0F0F0F0 &  0xCCCCCCCC) | (~0xF0F0F0F0 &  0xAAAAAAAA)
    };
    int i;
    for ( i = 0; i < 24; i++ )
        if ( ((unsigned)t) == s_Muxes[i] )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Count the number of unique drivers and invertors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lf_ManAnalyzeCoDrivers( Gia_Man_t * p, int * pnDrivers, int * pnInverts )
{
    Gia_Obj_t * pObj;
    int i, Entry, nDrivers, nInverts;
    Vec_Int_t * vMarks = Vec_IntStart( Gia_ManObjNum(p) );
    nDrivers = nInverts = 0;
    Gia_ManForEachCo( p, pObj, i )
        *Vec_IntEntryP( vMarks, Gia_ObjFaninId0p(p, pObj) ) |= Gia_ObjFaninC0(pObj) ? 2 : 1;
    Vec_IntForEachEntry( vMarks, Entry, i )
        nDrivers += (int)(Entry != 0), nInverts += (int)(Entry == 3);
    Vec_IntFree( vMarks );
    *pnDrivers = nDrivers;
    *pnInverts = nInverts;
}
void Lf_ManComputeSwitching( Gia_Man_t * p, Vec_Flt_t * vSwitches )
{
//    abctime clk = Abc_Clock();
    Vec_Flt_t * vSwitching = (Vec_Flt_t *)Gia_ManComputeSwitchProbs( p, 48, 16, 0 );
    assert( Vec_FltCap(vSwitches) == 0 );
    *vSwitches = *vSwitching;
    ABC_FREE( vSwitching );
//    Abc_PrintTime( 1, "Computing switching activity", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Lf_CutCreateUnit( Lf_Cut_t * p, int i )
{
    p->fLate = 0;
    p->fMux7 = 0;
    p->iFunc = 2;
    p->nLeaves = 1;
    p->pLeaves[0] = i;
    p->Sign = ((word)1) << (i & 0x3F);
    return 1;
}
static inline Lf_Cut_t * Lf_ManFetchSet( Lf_Man_t * p, int i )
{
    int uMaskPage = (1 << LF_LOG_PAGE) - 1;
    Gia_Obj_t * pObj = Gia_ManObj( p->pGia, i );
    int iOffSet = Vec_IntEntry( &p->vOffsets, i );
    int Entry = Vec_IntEntry( &p->vCutSets, iOffSet );
    assert( Gia_ObjIsAndNotBuf(pObj) );
    assert( pObj->Value > 0 );
    if ( Entry == -1 ) // first visit
    {
        if ( Vec_IntSize(&p->vFreeSets) == 0 ) // add new
        {
            Lf_Cut_t * pCut = (Lf_Cut_t *)ABC_CALLOC( word, p->nSetWords * (1 << LF_LOG_PAGE) );
            int uMaskShift = Vec_PtrSize(&p->vMemSets) << LF_LOG_PAGE;
            Vec_PtrPush( &p->vMemSets, pCut );
            for ( Entry = uMaskPage; Entry >= 0; Entry-- )
            {
                Vec_IntPush( &p->vFreeSets, uMaskShift | Entry );
                pCut[Entry].nLeaves   = LF_NO_LEAF;
            }
        }
        Entry = Vec_IntPop( &p->vFreeSets );
        Vec_IntWriteEntry( &p->vCutSets, iOffSet, Entry );
        p->nFrontMax = Abc_MaxInt( p->nFrontMax, Entry + 1 );
    }
    else if ( --pObj->Value == 0 )
    {
        Vec_IntPush( &p->vFreeSets, Entry );
        Vec_IntWriteEntry( &p->vCutSets, iOffSet, -1 );
    }
    return (Lf_Cut_t *)((word *)Vec_PtrEntry(&p->vMemSets, Entry >> LF_LOG_PAGE) + p->nSetWords * (Entry & uMaskPage));
}
static inline int Lf_ManPrepareSet( Lf_Man_t * p, int iObj, int Index, Lf_Cut_t ** ppCutSet )
{
    static word CutTemp[3][LF_CUT_WORDS];
    if ( Vec_IntEntry(&p->vOffsets, iObj) == -1 )
        return Lf_CutCreateUnit( (*ppCutSet = (Lf_Cut_t *)CutTemp[Index]), iObj );
    {
        Lf_Cut_t * pCut; 
        int i, nCutNum = p->pPars->nCutNum;
        *ppCutSet = Lf_ManFetchSet(p, iObj);
        Lf_CutSetForEachCut( p->nCutWords, *ppCutSet, pCut, i, nCutNum )
            if ( pCut->nLeaves == LF_NO_LEAF )
                return i;
        return i;
    }
}

/**Function*************************************************************

  Synopsis    [Cut manipulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Lf_CutGetSign( Lf_Cut_t * pCut )
{
    word Sign = 0; int i; 
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        Sign |= ((word)1) << (pCut->pLeaves[i] & 0x3F);
    return Sign;
}
static inline int Lf_CutCountBits( word i )
{
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F);
    return (i*(0x0101010101010101))>>56;
}
static inline int Lf_CutEqual( Lf_Cut_t * pCut0, Lf_Cut_t * pCut1 )
{
    int i;
    if ( pCut0->iFunc != pCut1->iFunc )
        return 0;
    if ( pCut0->nLeaves != pCut1->nLeaves )
        return 0;
    for ( i = 0; i < (int)pCut0->nLeaves; i++ )
        if ( pCut0->pLeaves[i] != pCut1->pLeaves[i] )
            return 0;
    return 1;
}
static inline float Lf_CutSwitches( Lf_Man_t * p, Lf_Cut_t * pCut )
{
    float Switches = 0; int i; 
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        Switches += Lf_ObjSwitches(p, pCut->pLeaves[i]);
//printf( "%.2f ", Switches );
    return Switches;
}
static inline void Lf_CutPrint( Lf_Man_t * p, Lf_Cut_t * pCut )
{
    int i, nDigits = Abc_Base10Log(Gia_ManObjNum(p->pGia)); 
    printf( "%d  {", pCut->nLeaves );
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        printf( " %*d", nDigits, pCut->pLeaves[i] );
    for ( ; i < (int)p->pPars->nLutSize; i++ )
        printf( " %*s", nDigits, " " );
    printf( "  }   Late = %d  D = %4d  A = %9.4f  F = %6d\n", 
        pCut->fLate, pCut->Delay, pCut->Flow, pCut->iFunc );
}
static inline float Lf_CutArea( Lf_Man_t * p, Lf_Cut_t * pCut )
{
    if ( pCut->nLeaves < 2 || pCut->fMux7 )
        return 0;
    if ( p->pPars->fPower )
        return 1.0 * pCut->nLeaves + Lf_CutSwitches( p, pCut );
    if ( p->pPars->fOptEdge )
        return (pCut->nLeaves + p->pPars->nAreaTuner) * (1 + (p->pPars->fCutGroup && (int)pCut->nLeaves > p->pPars->nLutSize/2));
    return 1 + (p->pPars->fCutGroup && (int)pCut->nLeaves > p->pPars->nLutSize/2);
}
static inline int Lf_CutIsMux( Lf_Man_t * p, Lf_Cut_t * pCut, Gia_Obj_t * pMux )
{
    int i, Id;
    if ( pCut->nLeaves != 3 )
        return 0;
    assert( Gia_ObjIsMux(p->pGia, pMux) );
    if ( Gia_ObjIsCi(Gia_ObjFanin0(pMux)) || Gia_ObjIsCi(Gia_ObjFanin1(pMux)) )
        return 0;
    Id = Gia_ObjFaninId0p( p->pGia, pMux );
    for ( i = 0; i < 3; i++ )
        if ( pCut->pLeaves[i] == Id )
            break;
    if ( i == 3 ) 
        return 0;
    Id = Gia_ObjFaninId1p( p->pGia, pMux );
    for ( i = 0; i < 3; i++ )
        if ( pCut->pLeaves[i] == Id )
            break;
    if ( i == 3 ) 
        return 0;
    Id = Gia_ObjFaninId2p( p->pGia, pMux );
    for ( i = 0; i < 3; i++ )
        if ( pCut->pLeaves[i] == Id )
            break;
    if ( i == 3 ) 
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Cut packing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Lf_MemAlloc( Lf_Mem_t * p, int LogPage, Vec_Ptr_t * vFree, int nCutWords )
{
    memset( p, 0, sizeof(Lf_Mem_t) );
    p->LogPage   = LogPage;
    p->MaskPage  = (1 << LogPage) - 1;
    p->nCutWords = nCutWords;
    p->vFree     = vFree;
}
static inline int Lf_MemSaveCut( Lf_Mem_t * p, Lf_Cut_t * pCut, int iObj )
{
    unsigned char * pPlace;
    int i, iPlace, Prev = iObj, iCur = p->iCur;
    assert( !pCut->fMux7 );
    if ( Vec_PtrSize(&p->vPages) == (p->iCur >> p->LogPage) )
        Vec_PtrPush( &p->vPages, Vec_PtrSize(p->vFree) ? Vec_PtrPop(p->vFree) : ABC_ALLOC(char,p->MaskPage+1) );
    assert( p->MaskPage - (p->iCur & p->MaskPage) >= 4 * (LF_LEAF_MAX + 2) );
    iPlace = iCur & p->MaskPage;
    pPlace = (unsigned char *)Vec_PtrEntry(&p->vPages, p->iCur >> p->LogPage);
    iPlace = Gia_AigerWriteUnsignedBuffer( pPlace, iPlace, pCut->nLeaves );
    for ( i = pCut->nLeaves - 1; i >= 0; i-- )
        iPlace = Gia_AigerWriteUnsignedBuffer( pPlace, iPlace, Prev - pCut->pLeaves[i] ), Prev = pCut->pLeaves[i];
    assert( pCut->nLeaves >= 2 || pCut->iFunc <= 3 );
    if ( pCut->iFunc >= 0 )
        iPlace = Gia_AigerWriteUnsignedBuffer( pPlace, iPlace, pCut->iFunc );
    if ( p->MaskPage - (iPlace & p->MaskPage) < 4 * (LF_LEAF_MAX + 2) )
        p->iCur = ((p->iCur >> p->LogPage) + 1) << p->LogPage;
    else
        p->iCur = (p->iCur & ~p->MaskPage) | iPlace;
    return iCur;
}
static inline Lf_Cut_t * Lf_MemLoadCut( Lf_Mem_t * p, int iCur, int iObj, Lf_Cut_t * pCut, int fTruth, int fRecycle )
{
    unsigned char * pPlace;  
    int i, Prev = iObj, Page = iCur >> p->LogPage;
    assert( Page < Vec_PtrSize(&p->vPages) );
    pPlace = (unsigned char *)Vec_PtrEntry(&p->vPages, Page) + (iCur & p->MaskPage);
    pCut->nLeaves = Gia_AigerReadUnsigned(&pPlace);
    assert( pCut->nLeaves <= LF_LEAF_MAX );
    for ( i = pCut->nLeaves - 1; i >= 0; i-- )
        pCut->pLeaves[i] = Prev - Gia_AigerReadUnsigned(&pPlace), Prev = pCut->pLeaves[i];
    pCut->iFunc = fTruth ? Gia_AigerReadUnsigned(&pPlace) : -1;
    assert( pCut->nLeaves >= 2 || pCut->iFunc <= 3 );
    if ( fRecycle && Page && Vec_PtrEntry(&p->vPages, Page-1) )
    {
        Vec_PtrPush( p->vFree, Vec_PtrEntry(&p->vPages, Page-1) );
        Vec_PtrWriteEntry( &p->vPages, Page-1, NULL );
    }
    pCut->Sign = fRecycle ? Lf_CutGetSign(pCut) : 0;
    pCut->fMux7 = 0;
    return pCut;
}
static inline void Lf_MemRecycle( Lf_Mem_t * p )
{
    void * pPlace; int i;
    Vec_PtrForEachEntry( void *, &p->vPages, pPlace, i )
        if ( pPlace )
            Vec_PtrPush( p->vFree, pPlace );
    Vec_PtrClear( &p->vPages );
    p->iCur = 0;
}
static inline Lf_Cut_t * Lf_MemLoadMuxCut( Lf_Man_t * p, int iObj, Lf_Cut_t * pCut )
{
    Gia_Obj_t * pMux = Gia_ManObj( p->pGia, iObj );
    assert( Gia_ObjIsMux(p->pGia, pMux) );
    pCut->iFunc = p->pPars->fCutMin ? 4 : -1;
    pCut->pLeaves[0] = Gia_ObjFaninId0( pMux, iObj );
    pCut->pLeaves[1] = Gia_ObjFaninId1( pMux, iObj );
    pCut->pLeaves[2] = Gia_ObjFaninId2( p->pGia, iObj );
    pCut->nLeaves = 3;
    pCut->fMux7 = 1;
    return pCut;
}
static inline Lf_Cut_t * Lf_ObjCutMux( Lf_Man_t * p, int i )
{
    static word CutSet[LF_CUT_WORDS];
    return Lf_MemLoadMuxCut( p, i, (Lf_Cut_t *)CutSet );
}
static inline Lf_Cut_t * Lf_ObjCutBest( Lf_Man_t * p, int i )
{
    static word CutSet[LF_CUT_WORDS];
    Lf_Bst_t * pBest = Lf_ObjReadBest( p, i );
    Lf_Cut_t * pCut = (Lf_Cut_t *)CutSet;
    int Index = Lf_BestCutIndex( pBest );
    pCut->Delay = pBest->Delay[Index];
    pCut->Flow  = pBest->Flow[Index];
    if ( Index == 2 )
        return Lf_MemLoadMuxCut( p, i, pCut );
    return Lf_MemLoadCut( &p->vStoreOld, pBest->Cut[Index].Handle, i, pCut, p->pPars->fCutMin, 0 );
}
static inline Lf_Cut_t * Lf_ObjCutBestNew( Lf_Man_t * p, int i, Lf_Cut_t * pCut )
{
    Lf_Bst_t * pBest = Lf_ObjReadBest( p, i );
    int Index = Lf_BestCutIndex( pBest );
    pCut->Delay = pBest->Delay[Index];
    pCut->Flow  = pBest->Flow[Index];
    if ( Index == 2 )
        return Lf_MemLoadMuxCut( p, i, pCut );
    return Lf_MemLoadCut( &p->vStoreNew, pBest->Cut[Index].Handle, i, pCut, 0, 0 );
}

/**Function*************************************************************

  Synopsis    [Check correctness of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Lf_CutCheck( Lf_Cut_t * pBase, Lf_Cut_t * pCut ) // check if pCut is contained in pBase
{
    int nSizeB = pBase->nLeaves;
    int nSizeC = pCut->nLeaves;
    int i, * pB = pBase->pLeaves;
    int k, * pC = pCut->pLeaves;
    for ( i = 0; i < nSizeC; i++ )
    {
        for ( k = 0; k < nSizeB; k++ )
            if ( pC[i] == pB[k] )
                break;
        if ( k == nSizeB )
            return 0;
    }
    return 1;
}
static inline int Lf_SetCheckArray( Lf_Cut_t ** ppCuts, int nCuts )
{
    Lf_Cut_t * pCut0, * pCut1; 
    int i, k, m, n, Value;
    assert( nCuts > 0 );
    for ( i = 0; i < nCuts; i++ )
    {
        pCut0 = ppCuts[i];
        assert( !pCut0->fMux7 );
        assert( pCut0->nLeaves < LF_LEAF_MAX );
        assert( pCut0->Sign == Lf_CutGetSign(pCut0) );
        // check duplicates
        for ( m = 0; m < (int)pCut0->nLeaves; m++ )
        for ( n = m + 1; n < (int)pCut0->nLeaves; n++ )
            assert( pCut0->pLeaves[m] < pCut0->pLeaves[n] );
        // check pairs
        for ( k = 0; k < nCuts; k++ )
        {
            pCut1 = ppCuts[k];
            if ( pCut0 == pCut1 )
                continue;
            // check containments
            Value = Lf_CutCheck( pCut0, pCut1 );
            assert( Value == 0 );
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Lf_CutMergeOrder( Lf_Cut_t * pCut0, Lf_Cut_t * pCut1, Lf_Cut_t * pCut, int nLutSize )
{ 
    int nSize0   = pCut0->nLeaves;
    int nSize1   = pCut1->nLeaves;
    int i, * pC0 = pCut0->pLeaves;
    int k, * pC1 = pCut1->pLeaves;
    int c, * pC  = pCut->pLeaves;
    // the case of the largest cut sizes
    if ( nSize0 == nLutSize && nSize1 == nLutSize )
    {
        for ( i = 0; i < nSize0; i++ )
        {
            if ( pC0[i] != pC1[i] )  return 0;
            pC[i] = pC0[i];
        }
        pCut->nLeaves = nLutSize;
        pCut->iFunc = -1;
        pCut->Sign = pCut0->Sign | pCut1->Sign;
        return 1;
    }
    // compare two cuts with different numbers
    i = k = c = 0;
    if ( nSize0 == 0 ) goto FlushCut1;
    if ( nSize1 == 0 ) goto FlushCut0;
    while ( 1 )
    {
        if ( c == nLutSize ) return 0;
        if ( pC0[i] < pC1[k] )
        {
            pC[c++] = pC0[i++];
            if ( i >= nSize0 ) goto FlushCut1;
        }
        else if ( pC0[i] > pC1[k] )
        {
            pC[c++] = pC1[k++];
            if ( k >= nSize1 ) goto FlushCut0;
        }
        else
        {
            pC[c++] = pC0[i++]; k++;
            if ( i >= nSize0 ) goto FlushCut1;
            if ( k >= nSize1 ) goto FlushCut0;
        }
    }

FlushCut0:
    if ( c + nSize0 > nLutSize + i ) return 0;
    while ( i < nSize0 )
        pC[c++] = pC0[i++];
    pCut->nLeaves = c;
    pCut->iFunc = -1;
    pCut->fMux7 = 0;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;

FlushCut1:
    if ( c + nSize1 > nLutSize + k ) return 0;
    while ( k < nSize1 )
        pC[c++] = pC1[k++];
    pCut->nLeaves = c;
    pCut->iFunc = -1;
    pCut->fMux7 = 0;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;
}
static inline int Lf_CutMergeOrder2( Lf_Cut_t * pCut0, Lf_Cut_t * pCut1, Lf_Cut_t * pCut, int nLutSize )
{ 
    int x0, i0 = 0, nSize0 = pCut0->nLeaves, * pC0 = pCut0->pLeaves;
    int x1, i1 = 0, nSize1 = pCut1->nLeaves, * pC1 = pCut1->pLeaves;
    int xMin, c = 0, * pC  = pCut->pLeaves;
    while ( 1 )
    {
        x0 = (i0 == nSize0) ? ABC_INFINITY : pC0[i0];
        x1 = (i1 == nSize1) ? ABC_INFINITY : pC1[i1];
        xMin = Abc_MinInt(x0, x1);
        if ( xMin == ABC_INFINITY ) break;
        if ( c == nLutSize ) return 0;
        pC[c++] = xMin;
        if (x0 == xMin) i0++;
        if (x1 == xMin) i1++;
    }
    pCut->nLeaves = c;
    pCut->iFunc = -1;
    pCut->fMux7 = 0;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;
}
static inline int Lf_CutMergeOrderMux( Lf_Cut_t * pCut0, Lf_Cut_t * pCut1, Lf_Cut_t * pCut2, Lf_Cut_t * pCut, int nLutSize )
{ 
    int x0, i0 = 0, nSize0 = pCut0->nLeaves, * pC0 = pCut0->pLeaves;
    int x1, i1 = 0, nSize1 = pCut1->nLeaves, * pC1 = pCut1->pLeaves;
    int x2, i2 = 0, nSize2 = pCut2->nLeaves, * pC2 = pCut2->pLeaves;
    int xMin, c = 0, * pC  = pCut->pLeaves;
    while ( 1 )
    {
        x0 = (i0 == nSize0) ? ABC_INFINITY : pC0[i0];
        x1 = (i1 == nSize1) ? ABC_INFINITY : pC1[i1];
        x2 = (i2 == nSize2) ? ABC_INFINITY : pC2[i2];
        xMin = Abc_MinInt( Abc_MinInt(x0, x1), x2 );
        if ( xMin == ABC_INFINITY ) break;
        if ( c == nLutSize ) return 0;
        pC[c++] = xMin;
        if (x0 == xMin) i0++;
        if (x1 == xMin) i1++;
        if (x2 == xMin) i2++;
    }
    pCut->nLeaves = c;
    pCut->iFunc = -1;
    pCut->fMux7 = 0;
    pCut->Sign = pCut0->Sign | pCut1->Sign | pCut2->Sign;
    return 1;
}

static inline int Lf_SetCutIsContainedOrder( Lf_Cut_t * pBase, Lf_Cut_t * pCut ) // check if pCut is contained in pBase
{
    int i, nSizeB = pBase->nLeaves;
    int k, nSizeC = pCut->nLeaves;
    if ( nSizeB == nSizeC )
    {
        for ( i = 0; i < nSizeB; i++ )
            if ( pBase->pLeaves[i] != pCut->pLeaves[i] )
                return 0;
        return 1;
    }
    assert( nSizeB > nSizeC ); 
    if ( nSizeC == 0 )
        return 1;
    for ( i = k = 0; i < nSizeB; i++ )
    {
        if ( pBase->pLeaves[i] > pCut->pLeaves[k] )
            return 0;
        if ( pBase->pLeaves[i] == pCut->pLeaves[k] )
        {
            if ( ++k == nSizeC )
                return 1;
        }
    }
    return 0;
}
static inline int Lf_SetLastCutIsContained( Lf_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[i]->nLeaves <= pCuts[nCuts]->nLeaves && (pCuts[i]->Sign & pCuts[nCuts]->Sign) == pCuts[i]->Sign && Lf_SetCutIsContainedOrder(pCuts[nCuts], pCuts[i]) )
            return 1;
    return 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Lf_CutCompareDelay( Lf_Cut_t * pCut0, Lf_Cut_t * pCut1 )
{
    if ( pCut0->Delay   < pCut1->Delay   )  return -1;
    if ( pCut0->Delay   > pCut1->Delay   )  return  1;
    if ( pCut0->nLeaves < pCut1->nLeaves )  return -1;
    if ( pCut0->nLeaves > pCut1->nLeaves )  return  1;
    if ( pCut0->Flow    < pCut1->Flow - LF_EPSILON )  return -1;
    if ( pCut0->Flow    > pCut1->Flow + LF_EPSILON )  return  1;
    return 0;
}
static inline int Lf_CutCompareArea( Lf_Cut_t * pCut0, Lf_Cut_t * pCut1 )
{
    if ( pCut0->fLate   < pCut1->fLate   )  return -1;
    if ( pCut0->fLate   > pCut1->fLate   )  return  1;
    if ( pCut0->Flow    < pCut1->Flow - LF_EPSILON )  return -1;
    if ( pCut0->Flow    > pCut1->Flow + LF_EPSILON )  return  1;
    if ( pCut0->Delay   < pCut1->Delay   )  return -1;
    if ( pCut0->Delay   > pCut1->Delay   )  return  1;
    if ( pCut0->nLeaves < pCut1->nLeaves )  return -1;
    if ( pCut0->nLeaves > pCut1->nLeaves )  return  1;
    return 0;
}
static inline int Lf_SetLastCutContainsArea( Lf_Cut_t ** pCuts, int nCuts )
{
    int i, k, fChanges = 0;
    for ( i = 1; i < nCuts; i++ )
        if ( pCuts[nCuts]->nLeaves < pCuts[i]->nLeaves && (pCuts[nCuts]->Sign & pCuts[i]->Sign) == pCuts[nCuts]->Sign && Lf_SetCutIsContainedOrder(pCuts[i], pCuts[nCuts]) )
            pCuts[i]->nLeaves = LF_NO_LEAF, fChanges = 1;
    if ( !fChanges )
        return nCuts;
    for ( i = k = 1; i <= nCuts; i++ )
    {
        if ( pCuts[i]->nLeaves == LF_NO_LEAF )
            continue;
        if ( k < i )
            ABC_SWAP( Lf_Cut_t *, pCuts[k], pCuts[i] );
        k++;
    }
    return k - 1;
}
static inline void Lf_SetSortByArea( Lf_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = nCuts; i > 1; i-- )
    {
        if ( Lf_CutCompareArea(pCuts[i - 1], pCuts[i]) < 0 )//!= 1 )
            return;
        ABC_SWAP( Lf_Cut_t *, pCuts[i - 1], pCuts[i] );
    }
}
static inline int Lf_SetAddCut( Lf_Cut_t ** pCuts, int nCuts, int nCutNum )
{
    if ( nCuts == 0 )
        return 1;
    nCuts = Lf_SetLastCutContainsArea(pCuts, nCuts);
    assert( nCuts >= 1 );
    if ( Lf_CutCompareDelay(pCuts[0], pCuts[nCuts]) == 1 ) // new cut is better for delay
    {
        ABC_SWAP( Lf_Cut_t *, pCuts[0], pCuts[nCuts] );
        // if old cut (now cut number nCuts) is contained - remove it
        if ( pCuts[0]->nLeaves < pCuts[nCuts]->nLeaves && (pCuts[0]->Sign & pCuts[nCuts]->Sign) == pCuts[0]->Sign && Lf_SetCutIsContainedOrder(pCuts[nCuts], pCuts[0]) )
            return nCuts;
    }
    // sort area cuts by area
    Lf_SetSortByArea( pCuts, nCuts );
    // add new cut if there is room
    return Abc_MinInt( nCuts + 1, nCutNum - 1 );
}
static inline void Lf_SetSortBySize( Lf_Cut_t ** pCutsR, int nCutsR )
{
    int i, j, best_i;
    for ( i = 1; i < nCutsR-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nCutsR; j++ )
            if ( pCutsR[j]->nLeaves > pCutsR[best_i]->nLeaves )
                best_i = j;
        ABC_SWAP( Lf_Cut_t *, pCutsR[i], pCutsR[best_i] );
    }
}

/**Function*************************************************************

  Synopsis    [Check if truth table has non-const-cof cofactoring variable.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Lf_ManFindCofVar( word * pTruth, int nWords, int nVars )
{
    word uTruthCof[LF_TT_WORDS]; int iVar;
    for ( iVar = 0; iVar < nVars; iVar++ )
    {
        Abc_TtCofactor0p( uTruthCof, pTruth, nWords, iVar );
        if ( Abc_TtSupportSize(uTruthCof, nVars) < 2 )
            continue;
        Abc_TtCofactor1p( uTruthCof, pTruth, nWords, iVar );
        if ( Abc_TtSupportSize(uTruthCof, nVars) < 2 )
            continue;
        return iVar;
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Lf_CutComputeTruth6( Lf_Man_t * p, Lf_Cut_t * pCut0, Lf_Cut_t * pCut1, int fCompl0, int fCompl1, Lf_Cut_t * pCutR, int fIsXor )
{
//    extern int Mf_ManTruthCanonicize( word * t, int nVars );
    int nOldSupp = pCutR->nLeaves, truthId, fCompl; word t;
    word t0 = *Lf_CutTruth(p, pCut0);
    word t1 = *Lf_CutTruth(p, pCut1);
    if ( Abc_LitIsCompl(pCut0->iFunc) ^ fCompl0 ) t0 = ~t0;
    if ( Abc_LitIsCompl(pCut1->iFunc) ^ fCompl1 ) t1 = ~t1;
    t0 = Abc_Tt6Expand( t0, pCut0->pLeaves, pCut0->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    t1 = Abc_Tt6Expand( t1, pCut1->pLeaves, pCut1->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    t =  fIsXor ? t0 ^ t1 : t0 & t1;
    if ( (fCompl = (int)(t & 1)) ) t = ~t;
    pCutR->nLeaves = Abc_Tt6MinBase( &t, pCutR->pLeaves, pCutR->nLeaves );
    assert( (int)(t & 1) == 0 );
    truthId        = Vec_MemHashInsert(p->vTtMem, &t);
    pCutR->iFunc   = Abc_Var2Lit( truthId, fCompl );
//    p->nCutMux += Lf_ManTtIsMux( t );
    assert( (int)pCutR->nLeaves <= nOldSupp );
//    Mf_ManTruthCanonicize( &t, pCutR->nLeaves );
    return (int)pCutR->nLeaves < nOldSupp;
}
static inline int Lf_CutComputeTruth( Lf_Man_t * p, Lf_Cut_t * pCut0, Lf_Cut_t * pCut1, int fCompl0, int fCompl1, Lf_Cut_t * pCutR, int fIsXor )
{
    if ( p->pPars->nLutSize <= 6 )
        return Lf_CutComputeTruth6( p, pCut0, pCut1, fCompl0, fCompl1, pCutR, fIsXor );
    {
    word uTruth[LF_TT_WORDS], uTruth0[LF_TT_WORDS], uTruth1[LF_TT_WORDS];
    int nOldSupp   = pCutR->nLeaves, truthId;
    int LutSize    = p->pPars->nLutSize, fCompl;
    int nWords     = Abc_Truth6WordNum(LutSize);
    word * pTruth0 = Lf_CutTruth(p, pCut0);
    word * pTruth1 = Lf_CutTruth(p, pCut1);
    Abc_TtCopy( uTruth0, pTruth0, nWords, Abc_LitIsCompl(pCut0->iFunc) ^ fCompl0 );
    Abc_TtCopy( uTruth1, pTruth1, nWords, Abc_LitIsCompl(pCut1->iFunc) ^ fCompl1 );
    Abc_TtExpand( uTruth0, LutSize, pCut0->pLeaves, pCut0->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    Abc_TtExpand( uTruth1, LutSize, pCut1->pLeaves, pCut1->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    if ( fIsXor )
        Abc_TtXor( uTruth, uTruth0, uTruth1, nWords, (fCompl = (int)((uTruth0[0] ^ uTruth1[0]) & 1)) );
    else
        Abc_TtAnd( uTruth, uTruth0, uTruth1, nWords, (fCompl = (int)((uTruth0[0] & uTruth1[0]) & 1)) );
    pCutR->nLeaves = Abc_TtMinBase( uTruth, pCutR->pLeaves, pCutR->nLeaves, LutSize );
    assert( (uTruth[0] & 1) == 0 );
//Kit_DsdPrintFromTruth( uTruth, pCutR->nLeaves ), printf("\n" ), printf("\n" );
    truthId        = Vec_MemHashInsert(p->vTtMem, uTruth);
    pCutR->iFunc   = Abc_Var2Lit( truthId, fCompl );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
    }
}
static inline int Lf_CutComputeTruthMux6( Lf_Man_t * p, Lf_Cut_t * pCut0, Lf_Cut_t * pCut1, Lf_Cut_t * pCutC, int fCompl0, int fCompl1, int fComplC, Lf_Cut_t * pCutR )
{
    int nOldSupp = pCutR->nLeaves, truthId, fCompl; word t;
    word t0 = *Lf_CutTruth(p, pCut0);
    word t1 = *Lf_CutTruth(p, pCut1);
    word tC = *Lf_CutTruth(p, pCutC);
    if ( Abc_LitIsCompl(pCut0->iFunc) ^ fCompl0 ) t0 = ~t0;
    if ( Abc_LitIsCompl(pCut1->iFunc) ^ fCompl1 ) t1 = ~t1;
    if ( Abc_LitIsCompl(pCutC->iFunc) ^ fComplC ) tC = ~tC;
    t0 = Abc_Tt6Expand( t0, pCut0->pLeaves, pCut0->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    t1 = Abc_Tt6Expand( t1, pCut1->pLeaves, pCut1->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    tC = Abc_Tt6Expand( tC, pCutC->pLeaves, pCutC->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    t = (tC & t1) | (~tC & t0);
    if ( (fCompl = (int)(t & 1)) ) t = ~t;
    pCutR->nLeaves = Abc_Tt6MinBase( &t, pCutR->pLeaves, pCutR->nLeaves );
    assert( (int)(t & 1) == 0 );
    truthId        = Vec_MemHashInsert(p->vTtMem, &t);
    pCutR->iFunc   = Abc_Var2Lit( truthId, fCompl );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
}
static inline int Lf_CutComputeTruthMux( Lf_Man_t * p, Lf_Cut_t * pCut0, Lf_Cut_t * pCut1, Lf_Cut_t * pCutC, int fCompl0, int fCompl1, int fComplC, Lf_Cut_t * pCutR )
{
    if ( p->pPars->nLutSize <= 6 )
        return Lf_CutComputeTruthMux6( p, pCut0, pCut1, pCutC, fCompl0, fCompl1, fComplC, pCutR );
    {
    word uTruth[LF_TT_WORDS], uTruth0[LF_TT_WORDS], uTruth1[LF_TT_WORDS], uTruthC[LF_TT_WORDS];
    int nOldSupp   = pCutR->nLeaves, truthId;
    int LutSize    = p->pPars->nLutSize, fCompl;
    int nWords     = Abc_Truth6WordNum(LutSize);
    word * pTruth0 = Lf_CutTruth(p, pCut0);
    word * pTruth1 = Lf_CutTruth(p, pCut1);
    word * pTruthC = Lf_CutTruth(p, pCutC);
    Abc_TtCopy( uTruth0, pTruth0, nWords, Abc_LitIsCompl(pCut0->iFunc) ^ fCompl0 );
    Abc_TtCopy( uTruth1, pTruth1, nWords, Abc_LitIsCompl(pCut1->iFunc) ^ fCompl1 );
    Abc_TtCopy( uTruthC, pTruthC, nWords, Abc_LitIsCompl(pCutC->iFunc) ^ fComplC );
    Abc_TtExpand( uTruth0, LutSize, pCut0->pLeaves, pCut0->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    Abc_TtExpand( uTruth1, LutSize, pCut1->pLeaves, pCut1->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    Abc_TtExpand( uTruthC, LutSize, pCutC->pLeaves, pCutC->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    Abc_TtMux( uTruth, uTruthC, uTruth1, uTruth0, nWords );
    fCompl         = (int)(uTruth[0] & 1);
    if ( fCompl ) Abc_TtNot( uTruth, nWords );
    pCutR->nLeaves = Abc_TtMinBase( uTruth, pCutR->pLeaves, pCutR->nLeaves, LutSize );
    assert( (uTruth[0] & 1) == 0 );
    truthId        = Vec_MemHashInsert(p->vTtMem, uTruth);
    pCutR->iFunc   = Abc_Var2Lit( truthId, fCompl );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
    }
}

/**Function*************************************************************

  Synopsis    [Exact local area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Lf_CutRef_rec( Lf_Man_t * p, Lf_Cut_t * pCut )
{
    word CutTemp[LF_CUT_WORDS] = {0};
    float Count = Lf_CutArea(p, pCut);
    int i, Var; 
    Lf_CutForEachVar( pCut, Var, i )
        if ( !Lf_ObjMapRefInc(p, Var) )
            Count += Lf_CutRef_rec( p, Lf_ObjCutBestNew(p, Var, (Lf_Cut_t *)CutTemp) );
    return Count;
}
float Lf_CutDeref_rec( Lf_Man_t * p, Lf_Cut_t * pCut )
{
    word CutTemp[LF_CUT_WORDS] = {0};
    float Count = Lf_CutArea(p, pCut);
    int i, Var; 
    Lf_CutForEachVar( pCut, Var, i )
        if ( !Lf_ObjMapRefDec(p, Var) )
            Count += Lf_CutDeref_rec( p, Lf_ObjCutBestNew(p, Var, (Lf_Cut_t *)CutTemp) );
    return Count;
}
static inline float Lf_CutAreaDerefed( Lf_Man_t * p, Lf_Cut_t * pCut )
{
    float Ela1 = Lf_CutRef_rec( p, pCut );
    Lf_CutDeref_rec( p, pCut );
//    float Ela2 = Lf_CutDeref_rec( p, pCut );
//    assert( Ela1 == Ela2 );
    return Ela1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int  Lf_CutRequired( Lf_Man_t * p, Lf_Cut_t * pCut )
{
    int i, Arr, Req, Arrival = 0, Required = 0;
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
    {
        if ( Lf_ObjOff(p, pCut->pLeaves[i]) < 0 )
//            Arr = Lf_ObjCiArrival( p, Gia_ObjCioId(Gia_ManObj(p->pGia, pCut->pLeaves[i])) ); 
            Arr = Lf_ObjArrival_rec( p, Gia_ManObj(p->pGia, pCut->pLeaves[i]) );
        else
            Arr = Lf_ObjReadBest(p, pCut->pLeaves[i])->Delay[0];
        Arrival = Abc_MaxInt( Arrival, Arr );
        Req = Lf_ObjRequired(p, pCut->pLeaves[i]);
        if ( Req < ABC_INFINITY )
            Required = Abc_MaxInt( Required, Req );
    }
    return Abc_MaxInt( Required + 2, Arrival + 1 ); 
}
static inline void Lf_CutParams( Lf_Man_t * p, Lf_Cut_t * pCut, int Required, float FlowRefs, Gia_Obj_t * pMux )
{
    Lf_Bst_t * pBest;
    int i, Index, Delay;
    assert( !pCut->fMux7 || Gia_ObjIsMux(p->pGia, pMux) );
    pCut->fLate = 0;
    pCut->Delay = 0;
    pCut->Flow  = 0;
    assert( pCut->nLeaves < LF_NO_LEAF );
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
    {
        if ( Lf_ObjOff(p, pCut->pLeaves[i]) < 0 )
//            Delay = Lf_ObjCiArrival( p, Gia_ObjCioId(Gia_ManObj(p->pGia, pCut->pLeaves[i])) );   
            Delay = Lf_ObjArrival_rec( p, Gia_ManObj(p->pGia, pCut->pLeaves[i]) );
        else
        {
            pBest = Lf_ObjReadBest(p, pCut->pLeaves[i]);
            assert( pBest->Delay[0] <= pBest->Delay[1] );
            assert( pBest->Flow[0]  >= pBest->Flow[1]  );
            if ( p->fUseEla )
                Index = Lf_BestIndex(pBest);
            else
            {
                Index = (int)(pBest->Delay[1] + 1 <= Required && Required != ABC_INFINITY);
                //pCut->Flow += pBest->Flow[Index];
                if ( pCut->Flow >= (float)1e32 || pBest->Flow[Index] >= (float)1e32 )
                    pCut->Flow = (float)1e32;
                else 
                    pCut->Flow += pBest->Flow[Index];
            }
            Delay = pBest->Delay[Index];
        }
//        if ( pCut->fMux7 && pCut->pLeaves[i] == Gia_ObjFaninId2p(p->pGia, pMux) )
//            Delay += 1;
        pCut->Delay = Abc_MaxInt( pCut->Delay, Delay );
    }
    pCut->Delay += (int)(pCut->nLeaves > 1);// && !pCut->fMux7;
    if ( pCut->Delay > Required )
        pCut->fLate = 1;
    if ( p->fUseEla )
        pCut->Flow = Lf_CutAreaDerefed(p, pCut) / FlowRefs;
    else
        pCut->Flow = (pCut->Flow + Lf_CutArea(p, pCut)) / FlowRefs;
}

void Lf_ObjMergeOrder( Lf_Man_t * p, int iObj )
{
    word CutSet[LF_CUT_MAX][LF_CUT_WORDS] = {{0}};
    Lf_Cut_t * pCutSet0, * pCutSet1, * pCutSet2, * pCut0, * pCut1, * pCut2;
    Lf_Cut_t * pCutSet = (Lf_Cut_t *)CutSet, * pCutsR[LF_CUT_MAX];
    Gia_Obj_t * pObj = Gia_ManObj(p->pGia, iObj);
    Lf_Bst_t * pBest = Lf_ObjReadBest(p, iObj);
    float FlowRefs = Lf_ObjFlowRefs(p, iObj);
    int Required   = Lf_ObjRequired(p, iObj);
    int nLutSize   = p->pPars->fCutGroup ? p->pPars->nLutSize/2 : p->pPars->nLutSize;
    int nCutNum    = p->pPars->nCutNum;
    int nCutWords  = p->nCutWords;
    int fComp0     = Gia_ObjFaninC0(pObj);
    int fComp1     = Gia_ObjFaninC1(pObj);
    int nCuts0     = Lf_ManPrepareSet( p, Gia_ObjFaninId0(pObj, iObj), 0, &pCutSet0 );
    int nCuts1     = Lf_ManPrepareSet( p, Gia_ObjFaninId1(pObj, iObj), 1, &pCutSet1 );
    int iSibl      = Gia_ObjSibl(p->pGia, iObj);
    int i, k, n, iCutUsed, nCutsR = 0;
    float Value1 = -1, Value2 = -1;
    assert( !Gia_ObjIsBuf(pObj) );
    Lf_CutSetForEachCut( nCutWords, pCutSet, pCut0, i, nCutNum )
        pCutsR[i] = pCut0;
    if ( p->Iter )
    {
        assert( nCutsR == 0 );
        // load cuts
        Lf_MemLoadCut( &p->vStoreOld, pBest->Cut[0].Handle, iObj, pCutsR[0], p->pPars->fCutMin, 1 );
        if ( Lf_BestDiffCuts(pBest) )
            Lf_MemLoadCut( &p->vStoreOld, pBest->Cut[1].Handle, iObj, pCutsR[1], p->pPars->fCutMin, 1 );
        // deref the cut
        if ( p->fUseEla && Lf_ObjMapRefNum(p, iObj) > 0 )
            Value1 = Lf_CutDeref_rec( p, pCutsR[Lf_BestIndex(pBest)] );
        // update required times
        if ( Required == ABC_INFINITY )//&& !p->fUseEla )
            Required = Lf_CutRequired( p, pCutsR[0] );
        // compute parameters
        Lf_CutParams( p, pCutsR[nCutsR++], Required, FlowRefs, pObj );
        if ( Lf_BestDiffCuts(pBest) )
        {
            assert( nCutsR == 1 );
            Lf_CutParams( p, pCutsR[nCutsR], Required, FlowRefs, pObj );
            nCutsR = Lf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
        if ( pCutsR[0]->fLate )
            p->nTimeFails++;
    }
    if ( iSibl )
    {
        Gia_Obj_t * pObjE = Gia_ObjSiblObj(p->pGia, iObj);
        int fCompE = Gia_ObjPhase(pObj) ^ Gia_ObjPhase(pObjE);
        int nCutsE = Lf_ManPrepareSet( p, iSibl, 2, &pCutSet2 );
        Lf_CutSetForEachCut( nCutWords, pCutSet2, pCut2, n, nCutsE )
        {
            if ( pCut2->pLeaves[0] == iSibl )
                continue;
            Lf_CutCopy( pCutsR[nCutsR], pCut2, nCutWords );
            if ( pCutsR[nCutsR]->iFunc >= 0 )
                pCutsR[nCutsR]->iFunc = Abc_LitNotCond( pCutsR[nCutsR]->iFunc, fCompE );
            Lf_CutParams( p, pCutsR[nCutsR], Required, FlowRefs, pObj );
            nCutsR = Lf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    if ( Gia_ObjIsMuxId(p->pGia, iObj) )
    {
        Lf_Cut_t * pCutSave = NULL;
        int fComp2 = Gia_ObjFaninC2(p->pGia, pObj);
        int nCuts2 = Lf_ManPrepareSet( p, Gia_ObjFaninId2(p->pGia, iObj), 2, &pCutSet2 );
        p->CutCount[0] += nCuts0 * nCuts1 * nCuts2;
        Lf_CutSetForEachCut( nCutWords, pCutSet0, pCut0, i, nCuts0 ) if ( (int)pCut0->nLeaves <= nLutSize )
        Lf_CutSetForEachCut( nCutWords, pCutSet1, pCut1, k, nCuts1 ) if ( (int)pCut1->nLeaves <= nLutSize )
        Lf_CutSetForEachCut( nCutWords, pCutSet2, pCut2, n, nCuts2 ) if ( (int)pCut2->nLeaves <= nLutSize )
        {
            pCutSave = pCut2;
            if ( Lf_CutCountBits(pCut0->Sign | pCut1->Sign | pCut2->Sign) > nLutSize )
                continue;
            p->CutCount[1]++; 
            if ( !Lf_CutMergeOrderMux(pCut0, pCut1, pCut2, pCutsR[nCutsR], nLutSize) )
                continue;
            if ( Lf_SetLastCutIsContained(pCutsR, nCutsR) )
                continue;
            p->CutCount[2]++;
            if ( p->pPars->fCutMin && Lf_CutComputeTruthMux(p, pCut0, pCut1, pCut2, fComp0, fComp1, fComp2, pCutsR[nCutsR]) )
                pCutsR[nCutsR]->Sign = Lf_CutGetSign(pCutsR[nCutsR]);
            if ( p->pPars->nLutSizeMux && p->pPars->nLutSizeMux == (int)pCutsR[nCutsR]->nLeaves && 
                Lf_ManFindCofVar(Lf_CutTruth(p,pCutsR[nCutsR]), Abc_Truth6WordNum(nLutSize), pCutsR[nCutsR]->nLeaves) == -1 )
                continue;
            Lf_CutParams( p, pCutsR[nCutsR], Required, FlowRefs, pObj );
            nCutsR = Lf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
        if ( p->pPars->fCutGroup )
        {
            assert( pCutSave->nLeaves == 1 );
            assert( pCutSave->pLeaves[0] == Gia_ObjFaninId2(p->pGia, iObj) );
            Lf_CutSetForEachCut( nCutWords, pCutSet0, pCut0, i, nCuts0 ) if ( (int)pCut0->nLeaves <= nLutSize )
            Lf_CutSetForEachCut( nCutWords, pCutSet1, pCut1, k, nCuts1 ) if ( (int)pCut1->nLeaves <= nLutSize )
            {
                assert( (int)pCut0->nLeaves + (int)pCut1->nLeaves + 1 <= p->pPars->nLutSize );
    //            if ( Lf_CutCountBits(pCut0->Sign | pCut1->Sign | pCutSave->Sign) > p->pPars->nLutSize )
    //                continue;
                p->CutCount[1]++; 
                if ( !Lf_CutMergeOrderMux(pCut0, pCut1, pCutSave, pCutsR[nCutsR], p->pPars->nLutSize) )
                    continue;
                if ( Lf_SetLastCutIsContained(pCutsR, nCutsR) )
                    continue;
                p->CutCount[2]++;
                if ( p->pPars->fCutMin && Lf_CutComputeTruthMux(p, pCut0, pCut1, pCutSave, fComp0, fComp1, fComp2, pCutsR[nCutsR]) )
                    pCutsR[nCutsR]->Sign = Lf_CutGetSign(pCutsR[nCutsR]);
    //            if ( p->pPars->nLutSizeMux && p->pPars->nLutSizeMux == (int)pCutsR[nCutsR]->nLeaves && 
    //                Lf_ManFindCofVar(Lf_CutTruth(p,pCutsR[nCutsR]), Abc_Truth6WordNum(nLutSize), pCutsR[nCutsR]->nLeaves) == -1 )
    //                continue;
                Lf_CutParams( p, pCutsR[nCutsR], Required, FlowRefs, pObj );
                nCutsR = Lf_SetAddCut( pCutsR, nCutsR, nCutNum );
            }
        }
    }
    else
    {
        int fIsXor = Gia_ObjIsXor(pObj);
        p->CutCount[0] += nCuts0 * nCuts1;
        Lf_CutSetForEachCut( nCutWords, pCutSet0, pCut0, i, nCuts0 ) if ( (int)pCut0->nLeaves <= nLutSize )
        Lf_CutSetForEachCut( nCutWords, pCutSet1, pCut1, k, nCuts1 ) if ( (int)pCut1->nLeaves <= nLutSize )
        {
            if ( (int)(pCut0->nLeaves + pCut1->nLeaves) > nLutSize && Lf_CutCountBits(pCut0->Sign | pCut1->Sign) > nLutSize )
                continue;
            p->CutCount[1]++; 
            if ( !Lf_CutMergeOrder(pCut0, pCut1, pCutsR[nCutsR], nLutSize) )
                continue;
            if ( Lf_SetLastCutIsContained(pCutsR, nCutsR) )
                continue;
            p->CutCount[2]++;
            if ( p->pPars->fCutMin && Lf_CutComputeTruth(p, pCut0, pCut1, fComp0, fComp1, pCutsR[nCutsR], fIsXor) )
                pCutsR[nCutsR]->Sign = Lf_CutGetSign(pCutsR[nCutsR]);
            if ( p->pPars->nLutSizeMux && p->pPars->nLutSizeMux == (int)pCutsR[nCutsR]->nLeaves && 
                Lf_ManFindCofVar(Lf_CutTruth(p,pCutsR[nCutsR]), Abc_Truth6WordNum(nLutSize), pCutsR[nCutsR]->nLeaves) == -1 )
                continue;
            Lf_CutParams( p, pCutsR[nCutsR], Required, FlowRefs, pObj );
            nCutsR = Lf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    // debug printout
    if ( 0 )
    {
        printf( "*** Obj = %d  FlowRefs = %.2f  MapRefs = %2d  Required = %2d\n", iObj, FlowRefs, Lf_ObjMapRefNum(p, iObj), Required );
        for ( i = 0; i < nCutsR; i++ )
            Lf_CutPrint( p, pCutsR[i] );
        printf( "\n" );
    }
    // verify
    assert( nCutsR > 0 && nCutsR < nCutNum );
//    assert( Lf_SetCheckArray(pCutsR, nCutsR) );
    // delay cut
    assert( nCutsR == 1 || pCutsR[0]->Delay <= pCutsR[1]->Delay );
    pBest->Cut[0].fUsed = pBest->Cut[1].fUsed = 0;
    pBest->Cut[0].Handle = pBest->Cut[1].Handle = Lf_MemSaveCut(&p->vStoreNew, pCutsR[0], iObj);
    pBest->Delay[0] = pBest->Delay[1] = pCutsR[0]->Delay;
    pBest->Flow[0] = pBest->Flow[1] = pCutsR[0]->Flow;
    p->nCutCounts[pCutsR[0]->nLeaves]++;
    p->CutCount[3] += nCutsR;
    p->nCutEqual++;
    // area cut
    iCutUsed = 0;
    if ( nCutsR > 1 && pCutsR[0]->Flow > pCutsR[1]->Flow + LF_EPSILON )//&& !pCutsR[1]->fLate ) // can remove !fLate
    {
        pBest->Cut[1].Handle = Lf_MemSaveCut(&p->vStoreNew, pCutsR[1], iObj);
        pBest->Delay[1] = pCutsR[1]->Delay;
        pBest->Flow[1] = pCutsR[1]->Flow;
        p->nCutCounts[pCutsR[1]->nLeaves]++;
        p->nCutEqual--;
        if ( !pCutsR[1]->fLate )
            iCutUsed = 1;
    }
    // mux cut
    if ( p->pPars->fUseMux7 && Gia_ObjIsMuxId(p->pGia, iObj) )
    {
        pCut2 = Lf_ObjCutMux( p, iObj );
        Lf_CutParams( p, pCut2, Required, FlowRefs, pObj );
        pBest->Delay[2] = pCut2->Delay;
        pBest->Flow[2] = pCut2->Flow;
        // update area value of the best area cut
//        if ( !pCut2->fLate )
//            pBest->Flow[1] = Abc_MinFloat( pBest->Flow[1], pBest->Flow[2] );
    }
    // reference resulting cut
    if ( p->fUseEla )
    {
        pBest->Cut[iCutUsed].fUsed = 1;
        if ( Lf_ObjMapRefNum(p, iObj) > 0 )
            Value2 = Lf_CutRef_rec( p, pCutsR[iCutUsed] );
//        if ( Value1 < Value2 )
//            printf( "ELA degradated cost at node %d from %d to %d.\n", iObj, Value1, Value2 ), fflush(stdout);
//        assert( Value1 >= Value2 );
//        if ( Value1 != -1 )
//            printf( "%.2f -> %.2f    ", Value1, Value2 );
    }
    if ( pObj->Value == 0 )
        return;
    // store the cutset
    pCutSet = Lf_ManFetchSet(p, iObj);
    Lf_CutSetForEachCut( nCutWords, pCutSet, pCut0, i, nCutNum )
    {
        assert( !pCut0->fMux7 );
        if ( i < nCutsR )
            Lf_CutCopy( pCut0, pCutsR[i], nCutWords );
        else if ( i == nCutsR && pCutsR[0]->nLeaves > 1 && (nCutsR == 1 || pCutsR[1]->nLeaves > 1) )
            Lf_CutCreateUnit( pCut0, iObj );
        else
            pCut0->nLeaves = LF_NO_LEAF;
    }
}

/**Function*************************************************************

  Synopsis    [Computing delay/area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Lf_ManSetFlowRefInc( Gia_Man_t * p, Vec_Flt_t * vRefs, Vec_Int_t * vOffsets, int i )
{
    if ( Gia_ObjIsAndNotBuf(Gia_ManObj(p, i)) )
        Vec_FltAddToEntry( vRefs, Vec_IntEntry(vOffsets, i), 1 );
}
void Lf_ManSetFlowRefs( Gia_Man_t * p, Vec_Flt_t * vRefs, Vec_Int_t * vOffsets )
{
    int fDiscount = 1;
    Gia_Obj_t * pObj, * pCtrl, * pData0, * pData1; 
    int i, Id;
    Vec_FltFill( vRefs, Gia_ManAndNotBufNum(p), 0 );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Gia_ObjIsAndNotBuf(Gia_ObjFanin0(pObj)) )
            Vec_FltAddToEntry( vRefs, Vec_IntEntry(vOffsets, Gia_ObjFaninId0(pObj, i)), 1 );
        if ( Gia_ObjIsBuf(pObj) )
            continue;
        if ( Gia_ObjIsAndNotBuf(Gia_ObjFanin1(pObj)) )
            Vec_FltAddToEntry( vRefs, Vec_IntEntry(vOffsets, Gia_ObjFaninId1(pObj, i)), 1 );
        if ( p->pMuxes )
        {
            if ( Gia_ObjIsMuxId(p, i) && Gia_ObjIsAndNotBuf(Gia_ObjFanin2(p, pObj)) )
                Vec_FltAddToEntry( vRefs, Vec_IntEntry(vOffsets, Gia_ObjFaninId2(p, i)), 1 );
        }
        else if ( fDiscount && Gia_ObjIsMuxType(pObj) ) // discount XOR/MUX
        {
            pCtrl  = Gia_Regular(Gia_ObjRecognizeMux(pObj, &pData1, &pData0));
            pData0 = Gia_Regular(pData0);
            pData1 = Gia_Regular(pData1);
            if ( Gia_ObjIsAndNotBuf(pCtrl) )
                Vec_FltAddToEntry( vRefs, Vec_IntEntry(vOffsets, Gia_ObjId(p, pCtrl)), -1 );
            if ( pData0 == pData1 && Gia_ObjIsAndNotBuf(pData0) )
                Vec_FltAddToEntry( vRefs, Vec_IntEntry(vOffsets, Gia_ObjId(p, pData0)), -1 );
        }
    }
    Gia_ManForEachCoDriverId( p, Id, i )
        if ( Gia_ObjIsAndNotBuf(Gia_ManObj(p, Id)) )
            Vec_FltAddToEntry( vRefs, Vec_IntEntry(vOffsets, Id), 1 );
    for ( i = 0; i < Vec_FltSize(vRefs); i++ )
        Vec_FltUpdateEntry( vRefs, i, 1 );
}
void Lf_ManSetCutRefs( Lf_Man_t * p )
{
    Gia_Obj_t * pObj; int i;
    if ( Vec_PtrSize(&p->vMemSets) * (1 << LF_LOG_PAGE) != Vec_IntSize(&p->vFreeSets) )
        printf( "The number of used cutsets = %d.\n", Vec_PtrSize(&p->vMemSets) * (1 << LF_LOG_PAGE) - Vec_IntSize(&p->vFreeSets) );
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
        assert( pObj->Value == 0 );
        if ( Gia_ObjIsBuf(pObj) )
            continue;
        if ( Gia_ObjIsAndNotBuf(Gia_ObjFanin0(pObj)) )
            Gia_ObjFanin0(pObj)->Value++;
        if ( Gia_ObjIsAndNotBuf(Gia_ObjFanin1(pObj)) )
            Gia_ObjFanin1(pObj)->Value++;
        if ( Gia_ObjIsMuxId(p->pGia, i) && Gia_ObjIsAndNotBuf(Gia_ObjFanin2(p->pGia, pObj)) )
            Gia_ObjFanin2(p->pGia, pObj)->Value++;
        if ( Gia_ObjSibl(p->pGia, i) && Gia_ObjIsAndNotBuf(Gia_ObjSiblObj(p->pGia, i)) )
            Gia_ObjSiblObj(p->pGia, i)->Value++;
    }
}

static inline int Lf_ManSetMuxCut( Lf_Man_t * p, Lf_Bst_t * pBest, int iObj, int Required )
{
    Gia_Obj_t * pMux;
    if ( !Gia_ObjIsMuxId(p->pGia, iObj) )
        return 0;
    if ( pBest->Delay[2] > Required )
        return 0;
    if ( pBest->Flow[2] > 1.1 * pBest->Flow[1] )
        return 0;
    pMux = Gia_ManObj(p->pGia, iObj);
    if ( pMux->fMark0 || Gia_ObjFanin0(pMux)->fMark0 || Gia_ObjFanin1(pMux)->fMark0 )
        return 0;
    Gia_ObjFanin0(pMux)->fMark0 = 1;
    Gia_ObjFanin1(pMux)->fMark0 = 1;
    return 1;
}
void Lf_ManSetMapRefsOne( Lf_Man_t * p, int iObj )
{
    Lf_Cut_t * pCut;
    Lf_Bst_t * pBest = Lf_ObjReadBest( p, iObj );
    int k, Index, Required = Lf_ObjRequired( p, iObj );
    assert( Lf_ObjMapRefNum(p, iObj) > 0 );
    assert( !pBest->Cut[0].fUsed && !pBest->Cut[1].fUsed );
    if ( !p->pPars->fUseMux7 || !Lf_ManSetMuxCut(p, pBest, iObj, Required) )
    {
        Index = (int)(Lf_BestDiffCuts(pBest) && pBest->Delay[1] <= Required);
        pBest->Cut[Index].fUsed = 1;
    }
    pCut = Lf_ObjCutBest( p, iObj );
    assert( !pCut->fMux7 || pCut->nLeaves == 3 );
//    assert( pCut->Delay <= Required );
    for ( k = 0; k < (int)pCut->nLeaves; k++ )
    {
//        if ( pCut->fMux7 && pCut->pLeaves[k] != Gia_ObjFaninId2(p->pGia, iObj) )
//            Lf_ObjSetRequired( p, pCut->pLeaves[k], Required );
//        else
            Lf_ObjSetRequired( p, pCut->pLeaves[k], Required - 1 );
        if ( Gia_ObjIsAndNotBuf(Gia_ManObj(p->pGia, pCut->pLeaves[k])) )
            Lf_ObjMapRefInc( p, pCut->pLeaves[k] );
    }
    if ( pCut->fMux7 )
    {
        p->pPars->Mux7++;
        p->pPars->Edge++;
        return;
    }
    if ( Vec_FltSize(&p->vSwitches) )
        p->Switches += Lf_CutSwitches(p, pCut);
    p->pPars->Edge += pCut->nLeaves;
    p->pPars->Area++;
}
int Lf_ManSetMapRefs( Lf_Man_t * p )
{
    float Coef = 1.0 / (1.0 + (p->Iter + 1) * (p->Iter + 1));
    float * pFlowRefs; 
    int * pMapRefs, i;
    Gia_Obj_t * pObj;
    // compute delay
    int Delay = 0;
    for ( i = 0; i < Gia_ManCoNum(p->pGia); i++ )
        Delay = Abc_MaxInt( Delay, Lf_ObjCoArrival(p, i) );
    // check delay target
    if ( p->pPars->DelayTarget == -1 && p->pPars->nRelaxRatio )
        p->pPars->DelayTarget = (int)((float)Delay * (100.0 + p->pPars->nRelaxRatio) / 100.0);
    if ( p->pPars->DelayTarget != -1 )
    {
        if ( Delay < p->pPars->DelayTarget + 0.01 )
            Delay = p->pPars->DelayTarget;
        else if ( p->pPars->nRelaxRatio == 0 )
            Abc_Print( 0, "Relaxing user-specified delay target from %d to %d.\n", p->pPars->DelayTarget, Delay );
    } 
    p->pPars->Delay = Delay;
    // compute area/edges/required
    p->pPars->Mux7 = p->pPars->Area = p->pPars->Edge = p->Switches = 0;
    Vec_IntFill( &p->vMapRefs, Gia_ManAndNotBufNum(p->pGia), 0 );
    Vec_IntFill( &p->vRequired, Gia_ManObjNum(p->pGia), ABC_INFINITY );
    if ( p->pPars->fUseMux7 )
    {
        Gia_ManCleanMark0(p->pGia);
        Gia_ManForEachCi( p->pGia, pObj, i )
            pObj->fMark0 = 1;
    }
    if ( p->pGia->pManTime != NULL )
    {
        assert( Gia_ManBufNum(p->pGia) );
        Tim_ManIncrementTravId( (Tim_Man_t*)p->pGia->pManTime );
        if ( p->pPars->fDoAverage )
            for ( i = 0; i < Gia_ManCoNum(p->pGia); i++ )
               Tim_ManSetCoRequired( (Tim_Man_t*)p->pGia->pManTime, i, (int)(Lf_ObjCoArrival(p, i) * (100.0 + p->pPars->nRelaxRatio) / 100.0) );
        else  
            Tim_ManInitPoRequiredAll( (Tim_Man_t*)p->pGia->pManTime, Delay );
        Gia_ManForEachObjReverse1( p->pGia, pObj, i )
        {
            if ( Gia_ObjIsBuf(pObj) )
                Lf_ObjSetRequired( p, Gia_ObjFaninId0(pObj, i), Lf_ObjRequired(p, i) );
            else if ( Gia_ObjIsAnd(pObj) )
            {
                if ( Lf_ObjMapRefNum(p, i) )
                    Lf_ManSetMapRefsOne( p, i );
            }
            else if ( Gia_ObjIsCi(pObj) )
                Tim_ManSetCiRequired( (Tim_Man_t*)p->pGia->pManTime, Gia_ObjCioId(pObj), Lf_ObjRequired(p, i) );
            else if ( Gia_ObjIsCo(pObj) )
            {
                int iDriverId = Gia_ObjFaninId0(pObj, i);
                int reqTime = Tim_ManGetCoRequired( (Tim_Man_t*)p->pGia->pManTime, Gia_ObjCioId(pObj) );
                Lf_ObjSetRequired( p, iDriverId, reqTime );
                if ( Gia_ObjIsAndNotBuf(Gia_ObjFanin0(pObj)) )
                    Lf_ObjMapRefInc( p, iDriverId );
            }
            else assert( 0 );
        }
    }
    else
    {
        Gia_ManForEachCo( p->pGia, pObj, i )
        {
            int iDriverId = Gia_ObjFaninId0p(p->pGia, pObj);
            int reqTime = p->pPars->fDoAverage ? (int)(Lf_ObjCoArrival(p, i) * (100.0 + p->pPars->nRelaxRatio) / 100.0) : Delay;
            Lf_ObjSetRequired( p, iDriverId, reqTime );
            if ( Gia_ObjIsAndNotBuf(Gia_ObjFanin0(pObj)) )
                Lf_ObjMapRefInc( p, iDriverId );
        }
        Gia_ManForEachAndReverse( p->pGia, pObj, i )
        {
            if ( Gia_ObjIsBuf(pObj) )
            {
                Lf_ObjSetRequired( p, Gia_ObjFaninId0(pObj, i), Lf_ObjRequired(p, i) );
                if ( Gia_ObjIsAndNotBuf(Gia_ObjFanin0(pObj)) )
                    Lf_ObjMapRefInc( p, Gia_ObjFaninId0(pObj, i) );
            }
            else if ( Lf_ObjMapRefNum(p, i) )
                Lf_ManSetMapRefsOne( p, i );
        }
    }
    if ( p->pPars->fUseMux7 )
        Gia_ManCleanMark0(p->pGia);
    // blend references
    assert( Vec_IntSize(&p->vMapRefs)  == Gia_ManAndNotBufNum(p->pGia) );
    assert( Vec_FltSize(&p->vFlowRefs) == Gia_ManAndNotBufNum(p->pGia) );
    pMapRefs  = Vec_IntArray(&p->vMapRefs);
    pFlowRefs = Vec_FltArray(&p->vFlowRefs);
    for ( i = 0; i < Vec_IntSize(&p->vMapRefs); i++ )
        pFlowRefs[i] = Coef * pFlowRefs[i] + (1.0 - Coef) * Abc_MaxFloat(1, pMapRefs[i]);
//        pFlowRefs[i] = 0.2 * pFlowRefs[i] + 0.8 * Abc_MaxFloat(1, pMapRefs[i]);
    return p->pPars->Area;
}

void Lf_ManCountMapRefsOne( Lf_Man_t * p, int iObj )
{
    Lf_Bst_t * pBest = Lf_ObjReadBest( p, iObj );
    Lf_Cut_t * pCut = Lf_ObjCutBest( p, iObj );
    int k ,Required = Lf_ObjRequired( p, iObj );
    assert( Lf_ObjMapRefNum(p, iObj) > 0 );
    assert( Lf_BestIsMapped(pBest) );
    assert( !pCut->fMux7 );
//    assert( pCut->Delay <= Required );
    for ( k = 0; k < (int)pCut->nLeaves; k++ )
            Lf_ObjSetRequired( p, pCut->pLeaves[k], Required - 1 );
    if ( Vec_FltSize(&p->vSwitches) )
        p->Switches += Lf_CutSwitches(p, pCut);
    p->pPars->Edge += pCut->nLeaves;
    p->pPars->Area++;
}
void Lf_ManCountMapRefs( Lf_Man_t * p )
{
    // compute delay
    Gia_Obj_t * pObj;
    int i, Id, Delay = 0;
    for ( i = 0; i < Gia_ManCoNum(p->pGia); i++ )
        Delay = Abc_MaxInt( Delay, Lf_ObjCoArrival2(p, i) );
    // check delay target
    if ( p->pPars->DelayTarget == -1 && p->pPars->nRelaxRatio )
        p->pPars->DelayTarget = (int)((float)Delay * (100.0 + p->pPars->nRelaxRatio) / 100.0);
    if ( p->pPars->DelayTarget != -1 )
    {
        if ( Delay < p->pPars->DelayTarget + 0.01 )
            Delay = p->pPars->DelayTarget;
        else if ( p->pPars->nRelaxRatio == 0 )
            Abc_Print( 0, "Relaxing user-specified delay target from %d to %d.\n", p->pPars->DelayTarget, Delay );
    }
    p->pPars->Delay = Delay;
    // compute area/edges/required
    p->pPars->Mux7 = p->pPars->Area = p->pPars->Edge = p->Switches = 0;
    Vec_IntFill( &p->vRequired, Gia_ManObjNum(p->pGia), ABC_INFINITY );
    if ( p->pPars->fUseMux7 )
        Gia_ManCleanMark0(p->pGia);
    if ( p->pGia->pManTime != NULL )
    {
        Tim_ManIncrementTravId( (Tim_Man_t*)p->pGia->pManTime );
        if ( p->pPars->fDoAverage )
            for ( i = 0; i < Gia_ManCoNum(p->pGia); i++ )
               Tim_ManSetCoRequired( (Tim_Man_t*)p->pGia->pManTime, i, (int)(Lf_ObjCoArrival(p, i) * (100.0 + p->pPars->nRelaxRatio) / 100.0) );
        else  
            Tim_ManInitPoRequiredAll( (Tim_Man_t*)p->pGia->pManTime, Delay );
        Gia_ManForEachObjReverse1( p->pGia, pObj, i )
        {
            if ( Gia_ObjIsBuf(pObj) )
                Lf_ObjSetRequired( p, Gia_ObjFaninId0(pObj, i), Lf_ObjRequired(p, i) );
            else if ( Gia_ObjIsAnd(pObj) )
            {
                if ( Lf_ObjMapRefNum(p, i) )
                    Lf_ManCountMapRefsOne( p, i );
            }
            else if ( Gia_ObjIsCi(pObj) )
                Tim_ManSetCiRequired( (Tim_Man_t*)p->pGia->pManTime, Gia_ObjCioId(pObj), Lf_ObjRequired(p, i) );
            else if ( Gia_ObjIsCo(pObj) )
            {
                int reqTime = Tim_ManGetCoRequired( (Tim_Man_t*)p->pGia->pManTime, Gia_ObjCioId(pObj) );
                Lf_ObjSetRequired( p, Gia_ObjFaninId0(pObj, i), reqTime );
            }
            else assert( 0 );
        }
    }
    else
    {
        Gia_ManForEachCoDriverId( p->pGia, Id, i )
            Lf_ObjSetRequired( p, Id, p->pPars->fDoAverage ? (int)(Lf_ObjCoArrival(p, i) * (100.0 + p->pPars->nRelaxRatio) / 100.0) : Delay );
        Gia_ManForEachAndReverse( p->pGia, pObj, i )
            if ( Gia_ObjIsBuf(pObj) )
                Lf_ObjSetRequired( p, Gia_ObjFaninId0(pObj, i), Lf_ObjRequired(p, i) );
            else if ( Lf_ObjMapRefNum(p, i) )
                Lf_ManCountMapRefsOne( p, i );
    }
    if ( p->pPars->fUseMux7 )
        Gia_ManCleanMark0(p->pGia);
}    


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Lf_ManDeriveMapping( Lf_Man_t * p )
{
    Vec_Int_t * vMapping;
    Lf_Cut_t * pCut;
    int i, k;
    assert( !p->pPars->fCutMin && p->pGia->vMapping == NULL );
    vMapping = Vec_IntAlloc( Gia_ManObjNum(p->pGia) + (int)p->pPars->Edge + (int)p->pPars->Area * 2 );
    Vec_IntFill( vMapping, Gia_ManObjNum(p->pGia), 0 );
    Gia_ManForEachAndId( p->pGia, i )
    {
        if ( !Lf_ObjMapRefNum(p, i) )
            continue;
        assert( !Gia_ObjIsBuf(Gia_ManObj(p->pGia,i)) );
        pCut = Lf_ObjCutBest( p, i );
        assert( !pCut->fMux7 );
        Vec_IntWriteEntry( vMapping, i, Vec_IntSize(vMapping) );
        Vec_IntPush( vMapping, pCut->nLeaves );
        for ( k = 0; k < (int)pCut->nLeaves; k++ )
            Vec_IntPush( vMapping, pCut->pLeaves[k] );
        Vec_IntPush( vMapping, i );
    }
    assert( Vec_IntCap(vMapping) == 16 || Vec_IntSize(vMapping) == Vec_IntCap(vMapping) );
    p->pGia->vMapping = vMapping;
    return p->pGia;
}
Gia_Man_t * Lf_ManDeriveMappingCoarse( Lf_Man_t * p )
{
    Gia_Man_t * pNew, * pGia = p->pGia;
    Gia_Obj_t * pObj;
    Lf_Cut_t * pCut;
    int i, k;
    assert( !p->pPars->fCutMin && pGia->pMuxes );
    // create new manager
    pNew = Gia_ManStart( Gia_ManObjNum(pGia) );
    pNew->pName = Abc_UtilStrsav( pGia->pName );
    pNew->pSpec = Abc_UtilStrsav( pGia->pSpec );
    // start mapping
    pNew->vMapping = Vec_IntAlloc( Gia_ManObjNum(pGia) + 2*Gia_ManXorNum(pGia) + 2*Gia_ManMuxNum(pGia) + (int)p->pPars->Edge + 2*(int)p->pPars->Area + 4*(int)p->pPars->Mux7 );
    Vec_IntFill( pNew->vMapping, Gia_ManObjNum(pGia) + 2*Gia_ManXorNum(pGia) + 2*Gia_ManMuxNum(pGia), 0 );
    // process objects
    Gia_ManConst0(pGia)->Value = 0;
    Gia_ManForEachObj1( pGia, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
            { pObj->Value = Gia_ManAppendCi( pNew ); continue; }
        if ( Gia_ObjIsCo(pObj) )
            { pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) ); continue; }
        if ( Gia_ObjIsBuf(pObj) )
            { pObj->Value = Gia_ManAppendBuf( pNew, Gia_ObjFanin0Copy(pObj) ); continue; }
        if ( Gia_ObjIsMuxId(pGia, i) )
            pObj->Value = Gia_ManAppendMux( pNew, Gia_ObjFanin2Copy(pGia, pObj), Gia_ObjFanin1Copy(pObj), Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsXor(pObj) )
            pObj->Value = Gia_ManAppendXor( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else 
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        if ( !Lf_ObjMapRefNum(p, i) )
            continue;
        pCut = Lf_ObjCutBest( p, i );
        Vec_IntWriteEntry( pNew->vMapping, Abc_Lit2Var(pObj->Value), Vec_IntSize(pNew->vMapping) );
        Vec_IntPush( pNew->vMapping, pCut->nLeaves );
        for ( k = 0; k < (int)pCut->nLeaves; k++ )
            Vec_IntPush( pNew->vMapping, Abc_Lit2Var(Gia_ManObj(pGia, pCut->pLeaves[k])->Value) );
        Vec_IntPush( pNew->vMapping, pCut->fMux7 ? -Abc_Lit2Var(pObj->Value) : Abc_Lit2Var(pObj->Value) );
    }
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(pGia) );
    assert( Vec_IntCap(pNew->vMapping) == 16 || Vec_IntSize(pNew->vMapping) == Vec_IntCap(pNew->vMapping) );
    return pNew;
}
static inline int Lf_ManDerivePart( Lf_Man_t * p, Gia_Man_t * pNew, Vec_Int_t * vMapping, Vec_Int_t * vMapping2, Vec_Int_t * vCopies, Lf_Cut_t * pCut, Vec_Int_t * vLeaves, Vec_Int_t * vCover, Gia_Obj_t * pObj )
{
    word * pTruth;
    int k, iLit, iTemp;
    if ( p->pPars->nLutSizeMux && p->pPars->nLutSizeMux == (int)pCut->nLeaves )
    {
        word pTruthCof[LF_TT_WORDS], * pTruth = Lf_CutTruth( p, pCut );
        int pVarsNew[LF_LEAF_MAX], nVarsNew, iLitCofs[2]; 
        int LutSize   = p->pPars->nLutSize;
        int nWords    = Abc_Truth6WordNum(LutSize);
        int c, iVar   = Lf_ManFindCofVar( pTruth, nWords, pCut->nLeaves );
        assert( iVar >= 0 && iVar < (int)pCut->nLeaves );
        for ( c = 0; c < 2; c++ )
        {
            for ( k = 0; k < (int)pCut->nLeaves; k++ )
                pVarsNew[k] = k;
            if ( c )
                Abc_TtCofactor1p( pTruthCof, pTruth, nWords, iVar );
            else
                Abc_TtCofactor0p( pTruthCof, pTruth, nWords, iVar );
            nVarsNew = Abc_TtMinBase( pTruthCof, pVarsNew, pCut->nLeaves, LutSize );
            assert( nVarsNew > 0 );
            // derive LUT
            Vec_IntClear( vLeaves );
            for ( k = 0; k < nVarsNew; k++ )
                Vec_IntPush( vLeaves, Vec_IntEntry(vCopies, pCut->pLeaves[pVarsNew[k]]) );
            iLitCofs[c] = Kit_TruthToGia( pNew, (unsigned *)pTruthCof, nVarsNew, vCover, vLeaves, 0 );
            // create mapping
            Vec_IntSetEntry( vMapping, Abc_Lit2Var(iLitCofs[c]), Vec_IntSize(vMapping2) );
            Vec_IntPush( vMapping2, Vec_IntSize(vLeaves) );
            Vec_IntForEachEntry( vLeaves, iTemp, k )
                Vec_IntPush( vMapping2, Abc_Lit2Var(iTemp) );
            Vec_IntPush( vMapping2, Abc_Lit2Var(iLitCofs[c]) );
        }
        // derive MUX
        pTruthCof[0] = ABC_CONST(0xCACACACACACACACA);
        Vec_IntClear( vLeaves );
        Vec_IntPush( vLeaves, iLitCofs[0] );
        Vec_IntPush( vLeaves, iLitCofs[1] );
        Vec_IntPush( vLeaves, Vec_IntEntry(vCopies, pCut->pLeaves[iVar]) );
        iLit = Kit_TruthToGia( pNew, (unsigned *)pTruthCof, Vec_IntSize(vLeaves), vCover, vLeaves, 0 );
        // create mapping
        Vec_IntSetEntry( vMapping, Abc_Lit2Var(iLit), Vec_IntSize(vMapping2) );
        Vec_IntPush( vMapping2, Vec_IntSize(vLeaves) );
        Vec_IntForEachEntry( vLeaves, iTemp, k )
            Vec_IntPush( vMapping2, Abc_Lit2Var(iTemp) );
        Vec_IntPush( vMapping2, -Abc_Lit2Var(iLit) );
        return iLit;
    }
    Vec_IntClear( vLeaves );
    if ( pCut->fMux7 )
    {
        assert( pCut->nLeaves == 3 );
        Vec_IntPush( vLeaves, Abc_LitNotCond(Vec_IntEntry(vCopies, pCut->pLeaves[0]), Gia_ObjFaninC0(pObj)) );
        Vec_IntPush( vLeaves, Abc_LitNotCond(Vec_IntEntry(vCopies, pCut->pLeaves[1]), Gia_ObjFaninC1(pObj)) );
        Vec_IntPush( vLeaves, Abc_LitNotCond(Vec_IntEntry(vCopies, pCut->pLeaves[2]), Gia_ObjFaninC2(p->pGia,pObj)) );
    }
    else
    {
        for ( k = 0; k < (int)pCut->nLeaves; k++ )
            Vec_IntPush( vLeaves, Vec_IntEntry(vCopies, pCut->pLeaves[k]) );
    }
    pTruth = Lf_CutTruth( p, pCut );
    iLit = Kit_TruthToGia( pNew, (unsigned *)pTruth, Vec_IntSize(vLeaves), vCover, vLeaves, 0 );
    // create mapping
    Vec_IntSetEntry( vMapping, Abc_Lit2Var(iLit), Vec_IntSize(vMapping2) );
    Vec_IntPush( vMapping2, Vec_IntSize(vLeaves) );
    Vec_IntForEachEntry( vLeaves, iTemp, k )
        Vec_IntPush( vMapping2, Abc_Lit2Var(iTemp) );
    Vec_IntPush( vMapping2, pCut->fMux7 ? -Abc_Lit2Var(iLit) : Abc_Lit2Var(iLit) );
    return iLit;
}
Gia_Man_t * Lf_ManDeriveMappingGia( Lf_Man_t * p )
{
    Gia_Man_t * pNew; 
    Gia_Obj_t * pObj; 
    Vec_Int_t * vCopies   = Vec_IntStartFull( Gia_ManObjNum(p->pGia) );
    Vec_Int_t * vMapping  = Vec_IntStart( 2*Gia_ManObjNum(p->pGia) + (int)p->pPars->Edge + 2*(int)p->pPars->Area + 4*(int)p->pPars->Mux7 );
    Vec_Int_t * vMapping2 = Vec_IntStart( (int)p->pPars->Edge + 2*(int)p->pPars->Area + 1000 );
    Vec_Int_t * vCover    = Vec_IntAlloc( 1 << 16 );
    Vec_Int_t * vLeaves   = Vec_IntAlloc( 16 );
    Lf_Cut_t * pCut;
    int i, iLit; 
    assert( p->pPars->fCutMin );
    // create new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p->pGia) );
    pNew->pName = Abc_UtilStrsav( p->pGia->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pGia->pSpec );
    Vec_IntWriteEntry( vCopies, 0, 0 );
    Gia_ManForEachObj1( p->pGia, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) )
        {
            Vec_IntWriteEntry( vCopies, i, Gia_ManAppendCi(pNew) );
            continue;
        }
        if ( Gia_ObjIsCo(pObj) )
        {
            iLit = Vec_IntEntry( vCopies, Gia_ObjFaninId0p(p->pGia, pObj) );
            iLit = Gia_ManAppendCo( pNew, Abc_LitNotCond(iLit, Gia_ObjFaninC0(pObj)) );
            continue;
        }
        if ( Gia_ObjIsBuf(pObj) )
        {
            iLit = Vec_IntEntry( vCopies, Gia_ObjFaninId0p(p->pGia, pObj) );
            iLit = Gia_ManAppendBuf( pNew, Abc_LitNotCond(iLit, Gia_ObjFaninC0(pObj)) );
            Vec_IntWriteEntry( vCopies, i, iLit );
            continue;
        }
        if ( !Lf_ObjMapRefNum(p, i) )
            continue;
        pCut = Lf_ObjCutBest( p, i );
        assert( pCut->iFunc >= 0 );
        if ( pCut->nLeaves == 0 )
        {
            assert( Abc_Lit2Var(pCut->iFunc) == 0 );
            Vec_IntWriteEntry( vCopies, i, pCut->iFunc );
            continue;
        }
        if ( pCut->nLeaves == 1 )
        {
            assert( Abc_Lit2Var(pCut->iFunc) == 1 );
            iLit = Vec_IntEntry( vCopies, pCut->pLeaves[0] );
            Vec_IntWriteEntry( vCopies, i, Abc_LitNotCond(iLit, Abc_LitIsCompl(pCut->iFunc)) );
            continue;
        }
        iLit = Lf_ManDerivePart( p, pNew, vMapping, vMapping2, vCopies, pCut, vLeaves, vCover, pObj );
        Vec_IntWriteEntry( vCopies, i, Abc_LitNotCond(iLit, Abc_LitIsCompl(pCut->iFunc)) );
    }
    Vec_IntFree( vCopies );
    Vec_IntFree( vCover );
    Vec_IntFree( vLeaves );
    // finish mapping 
    if ( Vec_IntSize(vMapping) > Gia_ManObjNum(pNew) )
        Vec_IntShrink( vMapping, Gia_ManObjNum(pNew) );
    else
        Vec_IntFillExtra( vMapping, Gia_ManObjNum(pNew), 0 );
    assert( Vec_IntSize(vMapping) == Gia_ManObjNum(pNew) );
    Vec_IntForEachEntry( vMapping, iLit, i )
        if ( iLit > 0 )
            Vec_IntAddToEntry( vMapping, i, Gia_ManObjNum(pNew) );
    Vec_IntAppend( vMapping, vMapping2 );
    Vec_IntFree( vMapping2 );
    // attach mapping and packing
    assert( pNew->vMapping == NULL );
    pNew->vMapping = vMapping;
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p->pGia) );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Lf_Man_t * Lf_ManAlloc( Gia_Man_t * pGia, Jf_Par_t * pPars )
{
    Lf_Man_t * p; int i, k = 0;
    assert( pPars->nCutNum > 1  && pPars->nCutNum <= LF_CUT_MAX );
    assert( pPars->nLutSize > 1 && pPars->nLutSize <= LF_LEAF_MAX );
    ABC_FREE( pGia->pRefs );
    Vec_IntFreeP( &pGia->vMapping );
    Gia_ManCleanValue( pGia );
    if ( Gia_ManHasChoices(pGia) )
        Gia_ManSetPhase(pGia);
    p = ABC_CALLOC( Lf_Man_t, 1 );
    Lf_ManAnalyzeCoDrivers( pGia, &p->nCoDrivers, &p->nInverters );
    if ( pPars->fPower )
        Lf_ManComputeSwitching( pGia, &p->vSwitches );
    p->clkStart  = Abc_Clock();
    p->pGia      = pGia;
    p->pPars     = pPars;
    p->nCutWords = (sizeof(Lf_Cut_t)/sizeof(int) + pPars->nLutSize + 1) >> 1;
    p->nSetWords = p->nCutWords * pPars->nCutNum;
    p->vTtMem    = pPars->fCutMin ? Vec_MemAllocForTT( pPars->nLutSize, 0 ) : NULL;
    if ( pPars->fCutMin && pPars->fUseMux7 )
        Vec_MemAddMuxTT( p->vTtMem, pPars->nLutSize );
    p->pObjBests = ABC_CALLOC( Lf_Bst_t, Gia_ManAndNotBufNum(pGia) );
    Vec_IntGrow( &p->vFreeSets, (1<<14) );
    Vec_PtrGrow( &p->vFreePages, 256 );
    Lf_MemAlloc( &p->vStoreOld, 16, &p->vFreePages, p->nCutWords );
    Lf_MemAlloc( &p->vStoreNew, 16, &p->vFreePages, p->nCutWords );
    Vec_IntFill( &p->vOffsets,  Gia_ManObjNum(pGia), -1 );
    Vec_IntFill( &p->vRequired, Gia_ManObjNum(pGia), ABC_INFINITY );
    Vec_IntFill( &p->vCutSets,  Gia_ManAndNotBufNum(pGia), -1 );
    Vec_FltFill( &p->vFlowRefs, Gia_ManAndNotBufNum(pGia), 0 );
    Vec_IntFill( &p->vMapRefs,  Gia_ManAndNotBufNum(pGia), 0 );
    Vec_IntFill( &p->vCiArrivals, Gia_ManCiNum(pGia), 0 );
    Gia_ManForEachAndId( pGia, i )
        if ( !Gia_ObjIsBuf(Gia_ManObj(pGia, i)) )
            Vec_IntWriteEntry( &p->vOffsets, i, k++ );
    assert( k == Gia_ManAndNotBufNum(pGia) );
    Lf_ManSetFlowRefs( pGia, &p->vFlowRefs, &p->vOffsets );
    if ( pPars->pTimesArr )
        for ( i = 0; i < Gia_ManPiNum(pGia); i++ )
            Vec_IntWriteEntry( &p->vCiArrivals, i, pPars->pTimesArr[i] );
    return p;
}
void Lf_ManFree( Lf_Man_t * p )
{
    ABC_FREE( p->pPars->pTimesArr );
    ABC_FREE( p->pPars->pTimesReq );
    if ( p->pPars->fCutMin )
        Vec_MemHashFree( p->vTtMem );
    if ( p->pPars->fCutMin )
        Vec_MemFree( p->vTtMem );
    Vec_PtrFreeData( &p->vMemSets );
    Vec_PtrFreeData( &p->vFreePages );
    Vec_PtrFreeData( &p->vStoreOld.vPages );
    Vec_PtrFreeData( &p->vStoreNew.vPages );
    ABC_FREE( p->vMemSets.pArray );
    ABC_FREE( p->vFreePages.pArray );
    ABC_FREE( p->vStoreOld.vPages.pArray );
    ABC_FREE( p->vStoreNew.vPages.pArray );
    ABC_FREE( p->vFreePages.pArray );
    ABC_FREE( p->vFreeSets.pArray );
    ABC_FREE( p->vOffsets.pArray );
    ABC_FREE( p->vRequired.pArray );
    ABC_FREE( p->vCutSets.pArray );
    ABC_FREE( p->vFlowRefs.pArray );
    ABC_FREE( p->vMapRefs.pArray );
    ABC_FREE( p->vSwitches.pArray );
    ABC_FREE( p->vCiArrivals.pArray );
    ABC_FREE( p->pObjBests );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Lf_ManSetDefaultPars( Jf_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Jf_Par_t) );
    pPars->nLutSize     =  6;
    pPars->nCutNum      =  8;
    pPars->nProcNum     =  0;
    pPars->nRounds      =  4;
    pPars->nRoundsEla   =  1;
    pPars->nRelaxRatio  =  0;
    pPars->nCoarseLimit =  3;
    pPars->nAreaTuner   =  1;
    pPars->nVerbLimit   =  5;
    pPars->DelayTarget  = -1;
    pPars->fAreaOnly    =  0;
    pPars->fOptEdge     =  1; 
    pPars->fUseMux7     =  0;
    pPars->fPower       =  0;
    pPars->fCoarsen     =  1;
    pPars->fCutMin      =  0;
    pPars->fFuncDsd     =  0;
    pPars->fGenCnf      =  0;
    pPars->fPureAig     =  0;
    pPars->fCutHashing  =  0;
    pPars->fCutSimple   =  0;
    pPars->fVerbose     =  0;
    pPars->fVeryVerbose =  0;
    pPars->nLutSizeMax  =  LF_LEAF_MAX;
    pPars->nCutNumMax   =  LF_CUT_MAX;
}
void Lf_ManPrintStats( Lf_Man_t * p, char * pTitle )
{
    if ( !p->pPars->fVerbose )
        return;
    printf( "%s :  ", pTitle );
    printf( "Level =%6lu   ",   (long)p->pPars->Delay );
    printf( "Area =%9lu   ",    (long)p->pPars->Area );
    printf( "Edge =%9lu   ",    (long)p->pPars->Edge );
    printf( "LUT =%9lu  ",      (long)p->pPars->Area+p->nInverters );
    if ( Vec_FltSize(&p->vSwitches) )
        printf( "Swt =%8.1f  ", p->Switches );
    if ( p->pPars->fUseMux7 )
        printf( "Mux7 =%7lu  ", (long)p->pPars->Mux7 );
    Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
    fflush( stdout );
}
void Lf_ManPrintInit( Lf_Man_t * p )
{
    if ( !p->pPars->fVerbose )
        return;
    printf( "LutSize = %d  ", p->pPars->nLutSize );
    printf( "CutNum = %d  ",  p->pPars->nCutNum );
    printf( "Iter = %d  ",    p->pPars->nRounds + p->pPars->nRoundsEla );
    if ( p->pPars->nRelaxRatio )
    printf( "Ratio = %d  ",   p->pPars->nRelaxRatio );
    printf( "Edge = %d  ",    p->pPars->fOptEdge );
    if ( p->pPars->DelayTarget != -1 )
    printf( "Delay = %d  ",   p->pPars->DelayTarget );
    printf( "CutMin = %d  ",  p->pPars->fCutMin );
    printf( "Coarse = %d  ",  p->pPars->fCoarsen );
    printf( "Cut/Set = %d/%d Bytes", 8*p->nCutWords, 8*p->nSetWords );
    printf( "\n" );
    printf( "Computing cuts...\r" );
    fflush( stdout );
}
void Lf_ManPrintQuit( Lf_Man_t * p, Gia_Man_t * pNew )
{
    float MemGia   = Gia_ManMemory(p->pGia) / (1<<20);
    float MemMan   = 1.0 * sizeof(int) * (2 * Gia_ManObjNum(p->pGia) + 3 * Gia_ManAndNotBufNum(p->pGia)) / (1<<20); // offset, required, cutsets, maprefs, flowrefs
    float MemCutsB = 1.0 * (p->vStoreOld.MaskPage + 1) * (Vec_PtrSize(&p->vFreePages) + Vec_PtrSize(&p->vStoreOld.vPages)) / (1<<20) + 1.0 * sizeof(Lf_Bst_t) * Gia_ManAndNotBufNum(p->pGia) / (1<<20);
    float MemCutsF = 1.0 * sizeof(word) * p->nSetWords * (1<<LF_LOG_PAGE) * Vec_PtrSize(&p->vMemSets) / (1<<20);
    float MemTt    = p->vTtMem ? Vec_MemMemory(p->vTtMem) / (1<<20) : 0;
    float MemMap   = Vec_IntMemory(pNew->vMapping) / (1<<20);
    if ( p->CutCount[0] == 0 )
        p->CutCount[0] = 1;
    if ( !p->pPars->fVerbose )
    {
        int i, CountOver[2] = {0};
        int nLutSize = p->pPars->fCutGroup ? p->pPars->nLutSize/2 : p->pPars->nLutSize;
        Gia_ManForEachLut( pNew, i )
            CountOver[Gia_ObjLutSize(pNew, i) > nLutSize]++;
        if ( p->pPars->fCutGroup )
            printf( "Created %d regular %d-LUTs and %d dual %d-LUTs. The total of %d %d-LUTs.\n", 
            CountOver[0], nLutSize, CountOver[1], nLutSize, CountOver[0] + 2*CountOver[1], nLutSize );
        return;
    }
    printf( "CutPair = %.0f  ",         p->CutCount[0] );
    printf( "Merge = %.0f (%.2f %%)  ", p->CutCount[1], 100.0*p->CutCount[1]/p->CutCount[0] );
    printf( "Eval = %.0f (%.2f %%)  ",  p->CutCount[2], 100.0*p->CutCount[2]/p->CutCount[0] );
    printf( "Cut = %.0f (%.2f %%)  ",   p->CutCount[3], 100.0*p->CutCount[3]/p->CutCount[0] );
    printf( "\n" );
    printf( "Gia = %.2f MB  ",          MemGia );
    printf( "Man = %.2f MB  ",          MemMan ); 
    printf( "Best = %.2f MB  ",         MemCutsB );
    printf( "Front = %.2f MB   ",       MemCutsF );
    printf( "Map = %.2f MB  ",          MemMap ); 
    printf( "TT = %.2f MB  ",           MemTt ); 
    printf( "Total = %.2f MB",          MemGia + MemMan + MemCutsB + MemCutsF + MemMap + MemTt ); 
    printf( "\n" );
    if ( 1 )
    {
        int i;
        for ( i = 0; i <= p->pPars->nLutSize; i++ )
            printf( "%d:%d  ", i, p->nCutCounts[i] );
        printf( "Equal = %d (%.0f %%) ", p->nCutEqual, 100.0 * p->nCutEqual / p->Iter / Gia_ManAndNotBufNum(p->pGia) );
        if ( p->vTtMem )
            printf( "TT = %d (%.2f %%)  ", Vec_MemEntryNum(p->vTtMem), 100.0 * Vec_MemEntryNum(p->vTtMem) / p->CutCount[2] );
        if ( p->pGia->pMuxes && p->nCutMux )
            printf( "MuxTT = %d (%.0f %%) ", p->nCutMux, 100.0 * p->nCutMux / p->Iter / Gia_ManMuxNum(p->pGia) );
        printf( "\n" );
    }
    printf( "CoDrvs = %d (%.2f %%)  ",  p->nCoDrivers, 100.0*p->nCoDrivers/Gia_ManCoNum(p->pGia) );
    printf( "CoInvs = %d (%.2f %%)  ",  p->nInverters, 100.0*p->nInverters/Gia_ManCoNum(p->pGia) );
    printf( "Front = %d (%.2f %%)  ",   p->nFrontMax,  100.0*p->nFrontMax/Gia_ManAndNum(p->pGia) );
    printf( "TimeFails = %d   ",        p->nTimeFails );
    Abc_PrintTime( 1, "Time",    Abc_Clock() - p->clkStart );
    fflush( stdout );
}
void Lf_ManComputeMapping( Lf_Man_t * p )
{
    Gia_Obj_t * pObj;
    int i, arrTime;
    assert( p->vStoreNew.iCur == 0 );
    Lf_ManSetCutRefs( p );
    if ( p->pGia->pManTime != NULL )
    {
        assert( !Gia_ManBufNum(p->pGia) );
        Tim_ManIncrementTravId( (Tim_Man_t*)p->pGia->pManTime );
        Gia_ManForEachObj1( p->pGia, pObj, i )
        {
            if ( Gia_ObjIsBuf(pObj) )
                continue;
            if ( Gia_ObjIsAnd(pObj) )
                Lf_ObjMergeOrder( p, i );
            else if ( Gia_ObjIsCi(pObj) )
            {
                arrTime = Tim_ManGetCiArrival( (Tim_Man_t*)p->pGia->pManTime, Gia_ObjCioId(pObj) );
                Lf_ObjSetCiArrival( p, Gia_ObjCioId(pObj), arrTime );
            }
            else if ( Gia_ObjIsCo(pObj) )
            {
                arrTime = Lf_ObjCoArrival( p, Gia_ObjCioId(pObj) );
                Tim_ManSetCoArrival( (Tim_Man_t*)p->pGia->pManTime, Gia_ObjCioId(pObj), arrTime );
            }
            else assert( 0 );
        }
//        Tim_ManPrint( p->pGia->pManTime );
    }
    else
    {
        Gia_ManForEachAnd( p->pGia, pObj, i )
            if ( !Gia_ObjIsBuf(pObj) )
                Lf_ObjMergeOrder( p, i );
    }
    Lf_MemRecycle( &p->vStoreOld );
    ABC_SWAP( Lf_Mem_t, p->vStoreOld, p->vStoreNew );
    if ( p->fUseEla )
        Lf_ManCountMapRefs( p );
    else
        Lf_ManSetMapRefs( p );
    Lf_ManPrintStats( p, (char *)(p->fUseEla ? "Ela  " : (p->Iter ? "Area " : "Delay")) );
}
Gia_Man_t * Lf_ManPerformMappingInt( Gia_Man_t * pGia, Jf_Par_t * pPars )
{
    int fUsePowerMode = 0;
    Lf_Man_t * p;
    Gia_Man_t * pNew, * pCls;
    if ( pPars->fUseMux7 )
        pPars->fCoarsen = 1, pPars->nRoundsEla = 0;
    if ( Gia_ManHasChoices(pGia) || pPars->nLutSizeMux )
        pPars->fCutMin = 1; 
    if ( pPars->fCoarsen )
    {
        pCls = Gia_ManDupMuxes(pGia, pPars->nCoarseLimit);
        pCls->pManTime = pGia->pManTime; pGia->pManTime = NULL;
    }
    else pCls = pGia;
    p = Lf_ManAlloc( pCls, pPars );
    if ( pPars->fVerbose && pPars->fCoarsen )
    {
        printf( "Initial " );  Gia_ManPrintMuxStats( pGia );  printf( "\n" );
        printf( "Derived " );  Gia_ManPrintMuxStats( pCls );  printf( "\n" );
    }
    Lf_ManPrintInit( p );

    // power mode
    if ( fUsePowerMode && Vec_FltSize(&p->vSwitches) )
        pPars->fPower = 0;

    // perform mapping
    for ( p->Iter = 0; p->Iter < p->pPars->nRounds; p->Iter++ )
        Lf_ManComputeMapping( p );
    p->fUseEla = 1;
    for ( ; p->Iter < p->pPars->nRounds + pPars->nRoundsEla; p->Iter++ )
        Lf_ManComputeMapping( p );

    // power mode
    if ( fUsePowerMode && Vec_FltSize(&p->vSwitches) )
    {
        pPars->fPower = 1;
        for ( ; p->Iter < p->pPars->nRounds + pPars->nRoundsEla + 2; p->Iter++ )
            Lf_ManComputeMapping( p );
    }

    if ( pPars->fVeryVerbose && pPars->fCutMin )
        Vec_MemDumpTruthTables( p->vTtMem, Gia_ManName(p->pGia), pPars->nLutSize );
    if ( pPars->fCutMin )
        pNew = Lf_ManDeriveMappingGia( p );
    else if ( pPars->fCoarsen )
        pNew = Lf_ManDeriveMappingCoarse( p );
    else
        pNew = Lf_ManDeriveMapping( p );
    Gia_ManMappingVerify( pNew );
    Lf_ManPrintQuit( p, pNew );
    Lf_ManFree( p );
    if ( pCls != pGia )
    {
        pGia->pManTime = pCls->pManTime; pCls->pManTime = NULL;
        Gia_ManStop( pCls );
    }
    return pNew;
}
Gia_Man_t * Lf_ManPerformMapping( Gia_Man_t * p, Jf_Par_t * pPars )
{
    Gia_Man_t * pNew;
    if ( p->pManTime && Tim_ManBoxNum((Tim_Man_t*)p->pManTime) && Gia_ManIsNormalized(p) )
    {
        Tim_Man_t * pTimOld = (Tim_Man_t *)p->pManTime;
        p->pManTime = Tim_ManDup( pTimOld, 1 );
        pNew = Gia_ManDupUnnormalize( p );
        if ( pNew == NULL )
            return NULL;
        Gia_ManTransferTiming( pNew, p );
        p = pNew;
        // mapping
        pNew = Lf_ManPerformMappingInt( p, pPars );
        if ( pNew != p )
        {
            Gia_ManTransferTiming( pNew, p );
            Gia_ManStop( p );
        }
        // normalize
        pNew = Gia_ManDupNormalize( p = pNew, 0 );
        Gia_ManTransferMapping( pNew, p );
//        Gia_ManTransferPacking( pNew, p );
        Gia_ManTransferTiming( pNew, p );
        Gia_ManStop( p ); // do not delete if the original one!
        // cleanup
        Tim_ManStop( (Tim_Man_t *)pNew->pManTime );
        pNew->pManTime = pTimOld;
        assert( Gia_ManIsNormalized(pNew) );
    }
    else 
    {
        // mapping
        pNew = Lf_ManPerformMappingInt( p, pPars );
        Gia_ManTransferTiming( pNew, p );
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Interface of LUT mapping package.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
Gia_Man_t * Gia_ManPerformLfMapping( Gia_Man_t * p, Jf_Par_t * pPars, int fNormalized )
{
    Gia_Man_t * pNew;
    assert( !pPars->fCutGroup || pPars->nLutSize == 9 || pPars->nLutSize == 11 || pPars->nLutSize == 13 );
    // reconstruct GIA according to the hierarchy manager
    assert( pPars->pTimesArr == NULL );
    assert( pPars->pTimesReq == NULL );
    if ( p->pManTime )
    {
        if ( fNormalized )
        {
            pNew = Gia_ManDupUnnormalize( p );
            if ( pNew == NULL )
                return NULL;
            Gia_ManTransferTiming( pNew, p );
            p = pNew;
            // set arrival and required times
            pPars->pTimesArr = Tim_ManGetArrTimes( (Tim_Man_t *)p->pManTime );
            pPars->pTimesReq = Tim_ManGetReqTimes( (Tim_Man_t *)p->pManTime );
        }
        else
            p = Gia_ManDup( p );
    }
    else 
        p = Gia_ManDup( p );
    // perform mapping
    pNew = Lf_ManPerformMappingInt( p, pPars );
    if ( pNew != p )
    {
        // transfer name
        ABC_FREE( pNew->pName );
        ABC_FREE( pNew->pSpec );
        pNew->pName = Abc_UtilStrsav( p->pName );
        pNew->pSpec = Abc_UtilStrsav( p->pSpec );
        Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
        // return the original (unmodified by the mapper) timing manager
        Gia_ManTransferTiming( pNew, p );
        Gia_ManStop( p );
    }
    // normalize and transfer mapping
    pNew = Gia_ManDupNormalize( p = pNew, 0 );
    Gia_ManTransferMapping( pNew, p );
//    Gia_ManTransferPacking( pNew, p );
    Gia_ManTransferTiming( pNew, p );
    Gia_ManStop( p );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

