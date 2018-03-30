/**CFile****************************************************************

  FileName    [giaNf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Standard-cell mapper.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaNf.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <float.h>
#include "gia.h"
#include "misc/st/st.h"
#include "map/mio/mio.h"
#include "misc/util/utilTruth.h"
#include "misc/extra/extra.h"
#include "base/main/main.h"
#include "misc/vec/vecMem.h"
#include "misc/vec/vecWec.h"
#include "opt/dau/dau.h"
#include "misc/util/utilNam.h"
#include "map/scl/sclCon.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define NF_LEAF_MAX  6
#define NF_CUT_MAX  32
#define NF_NO_LEAF  31
#define NF_NO_FUNC  0x3FFFFFF
#define NF_EPSILON  0.001

typedef struct Nf_Cut_t_ Nf_Cut_t; 
struct Nf_Cut_t_
{
    word            Sign;           // signature
    int             Delay;          // delay
    float           Flow;           // flow
    unsigned        iFunc   : 26;   // function (NF_NO_FUNC)
    unsigned        Useless :  1;   // function
    unsigned        nLeaves :  5;   // leaf number (NF_NO_LEAF)
    int             pLeaves[NF_LEAF_MAX+1]; // leaves
};
typedef struct Nf_Cfg_t_ Nf_Cfg_t; 
struct Nf_Cfg_t_
{
    unsigned        fCompl  :  1;   // complemented
    unsigned        Phase   :  7;   // match phase
    unsigned        Perm    : 24;   // match permutation
};
typedef struct Nf_Mat_t_ Nf_Mat_t; 
struct Nf_Mat_t_
{
    unsigned        Gate    : 20;   // gate
    unsigned        CutH    : 10;   // cut handle
    unsigned        fCompl  :  1;   // complemented
    unsigned        fBest   :  1;   // best cut
    Nf_Cfg_t        Cfg;            // input literals
    int             D;              // delay
    float           F;              // area 
};
typedef struct Nf_Obj_t_ Nf_Obj_t; 
struct Nf_Obj_t_
{
    Nf_Mat_t        M[2][2];         // del/area (2x)
};
typedef struct Nf_Man_t_ Nf_Man_t; 
struct Nf_Man_t_
{
    // user data
    Gia_Man_t *     pGia;           // derived manager
    Jf_Par_t *      pPars;          // parameters
    // matching
    Vec_Mem_t *     vTtMem;         // truth tables
    Vec_Wec_t *     vTt2Match;      // matches for truth tables
    Mio_Cell2_t *   pCells;         // library gates
    int             nCells;         // library gate count
    // cut data
    Nf_Obj_t *      pNfObjs;        // best cuts
    Vec_Ptr_t       vPages;         // cut memory
    Vec_Int_t       vCutSets;       // cut offsets
    Vec_Int_t       vMapRefs;       // mapping refs   (2x)
    Vec_Flt_t       vFlowRefs;      // flow refs      (2x)
    Vec_Int_t       vRequired;      // required times (2x)
    Vec_Flt_t       vCutFlows;      // temporary cut area
    Vec_Int_t       vCutDelays;     // temporary cut delay
    Vec_Int_t       vBackup;        // backup literals
    int             iCur;           // current position
    int             Iter;           // mapping iterations
    int             fUseEla;        // use exact area
    int             nInvs;          // the inverter count
    int             InvDelayI;      // inverter delay
    word            InvAreaW;       // inverter delay
    float           InvAreaF;       // inverter area 
    // statistics
    abctime         clkStart;       // starting time
    double          CutCount[6];    // cut counts
    int             nCutUseAll;     // objects with useful cuts
};

static inline int          Nf_Cfg2Int( Nf_Cfg_t Mat )                                { union { int x; Nf_Cfg_t y; } v; v.y = Mat; return v.x;           }
static inline Nf_Cfg_t     Nf_Int2Cfg( int Int )                                     { union { int x; Nf_Cfg_t y; } v; v.x = Int; return v.y;           }

static inline Nf_Obj_t *   Nf_ManObj( Nf_Man_t * p, int i )                          { return p->pNfObjs + i;                                           }
static inline Mio_Cell2_t* Nf_ManCell( Nf_Man_t * p, int i )                         { return p->pCells + i;                                            }
static inline int *        Nf_ManCutSet( Nf_Man_t * p, int i )                       { return (int *)Vec_PtrEntry(&p->vPages, i >> 16) + (i & 0xFFFF);  }
static inline int          Nf_ObjCutSetId( Nf_Man_t * p, int i )                     { return Vec_IntEntry( &p->vCutSets, i );                          }
static inline int *        Nf_ObjCutSet( Nf_Man_t * p, int i )                       { return Nf_ManCutSet(p, Nf_ObjCutSetId(p, i));                    }
static inline int          Nf_ObjHasCuts( Nf_Man_t * p, int i )                      { return (int)(Vec_IntEntry(&p->vCutSets, i) > 0);                 }
static inline int *        Nf_ObjCutBest( Nf_Man_t * p, int i )                      { return NULL;                                                     } 
static inline int          Nf_ObjCutUseless( Nf_Man_t * p, int TruthId )             { return (int)(TruthId >= Vec_WecSize(p->vTt2Match));              } 

static inline float        Nf_ObjCutFlow( Nf_Man_t * p, int i )                      { return Vec_FltEntry(&p->vCutFlows, i);                           } 
static inline int          Nf_ObjCutDelay( Nf_Man_t * p, int i )                     { return Vec_IntEntry(&p->vCutDelays, i);                          } 
static inline void         Nf_ObjSetCutFlow( Nf_Man_t * p, int i, float a )          { Vec_FltWriteEntry(&p->vCutFlows, i, a);                          } 
static inline void         Nf_ObjSetCutDelay( Nf_Man_t * p, int i, int d )           { Vec_IntWriteEntry(&p->vCutDelays, i, d);                         } 

static inline int          Nf_ObjMapRefNum( Nf_Man_t * p, int i, int c )             { return Vec_IntEntry(&p->vMapRefs, Abc_Var2Lit(i,c));             }
static inline int          Nf_ObjMapRefInc( Nf_Man_t * p, int i, int c )             { return (*Vec_IntEntryP(&p->vMapRefs, Abc_Var2Lit(i,c)))++;       }
static inline int          Nf_ObjMapRefDec( Nf_Man_t * p, int i, int c )             { return --(*Vec_IntEntryP(&p->vMapRefs, Abc_Var2Lit(i,c)));       }
static inline float        Nf_ObjFlowRefs( Nf_Man_t * p, int i, int c )              { return Vec_FltEntry(&p->vFlowRefs, Abc_Var2Lit(i,c));            }
static inline int          Nf_ObjRequired( Nf_Man_t * p, int i, int c )              { return Vec_IntEntry(&p->vRequired, Abc_Var2Lit(i,c));            }
static inline void         Nf_ObjSetRequired( Nf_Man_t * p,int i, int c, int f )     { Vec_IntWriteEntry(&p->vRequired, Abc_Var2Lit(i,c), f);           }
static inline void         Nf_ObjUpdateRequired( Nf_Man_t * p,int i, int c, int f )  { if (Nf_ObjRequired(p, i, c) > f) Nf_ObjSetRequired(p, i, c, f);  }

static inline Nf_Mat_t *   Nf_ObjMatchD( Nf_Man_t * p, int i, int c )                { return &Nf_ManObj(p, i)->M[c][0];                                }
static inline Nf_Mat_t *   Nf_ObjMatchA( Nf_Man_t * p, int i, int c )                { return &Nf_ManObj(p, i)->M[c][1];                                }

static inline int          Nf_CutSize( int * pCut )                                  { return pCut[0] & NF_NO_LEAF;                                     }
static inline int          Nf_CutFunc( int * pCut )                                  { return ((unsigned)pCut[0] >> 5);                                 }
static inline int *        Nf_CutLeaves( int * pCut )                                { return pCut + 1;                                                 }
static inline int          Nf_CutSetBoth( int n, int f )                             { return n | (f << 5);                                             }
static inline int          Nf_CutIsTriv( int * pCut, int i )                         { return Nf_CutSize(pCut) == 1 && pCut[1] == i;                    } 
static inline int          Nf_CutHandle( int * pCutSet, int * pCut )                 { assert( pCut > pCutSet ); return pCut - pCutSet;                 } 
static inline int *        Nf_CutFromHandle( int * pCutSet, int h )                  { assert( h > 0 ); return pCutSet + h;                             }

static inline int          Nf_CfgVar( Nf_Cfg_t Cfg, int i )                          { return (Cfg.Perm >> (i<<2)) & 15;                                }
static inline int          Nf_CfgCompl( Nf_Cfg_t Cfg, int i )                        { return (Cfg.Phase >> i) & 1;                                     }

#define Nf_SetForEachCut( pList, pCut, i )                   for ( i = 0, pCut = pList + 1; i < pList[0]; i++, pCut += Nf_CutSize(pCut) + 1 )
#define Nf_CutForEachVarCompl( pCut, Cfg, iVar, fCompl, i )  for ( i = 0; i < Nf_CutSize(pCut) && (iVar = Nf_CutLeaves(pCut)[Nf_CfgVar(Cfg, i)]) && ((fCompl = Nf_CfgCompl(Cfg, i)), 1); i++ )
#define Nf_CfgForEachVarCompl( Cfg, Size, iVar, fCompl, i )  for ( i = 0; i < Size && ((iVar = Nf_CfgVar(Cfg, i)), 1) && ((fCompl = Nf_CfgCompl(Cfg, i)), 1); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nf_StoCellIsDominated( Mio_Cell2_t * pCell, int * pFans, int * pProf )
{
    int k;
    if ( pCell->AreaF + NF_EPSILON < Abc_Int2Float(pProf[0]) )
        return 0;
    for ( k = 0; k < (int)pCell->nFanins; k++ )
        if ( pCell->iDelays[Abc_Lit2Var(pFans[k])] < pProf[k+1] )
            return 0;
    return 1; // pCell is dominated
}
void Nf_StoCreateGateAdd( Vec_Mem_t * vTtMem, Vec_Wec_t * vTt2Match, Mio_Cell2_t * pCell, word uTruth, int * pFans, int nFans, Vec_Wec_t * vProfs, Vec_Int_t * vStore, int fPinFilter, int fPinPerm, int fPinQuick )
{
    Vec_Int_t * vArray, * vArrayProfs = NULL;
    int i, k, GateId, Entry, fCompl = (int)(uTruth & 1);
    word uFunc = fCompl ? ~uTruth : uTruth;
    int iFunc = Vec_MemHashInsert( vTtMem, &uFunc );
    Nf_Cfg_t Mat = Nf_Int2Cfg(0);
    // get match array
    if ( iFunc == Vec_WecSize(vTt2Match) )
        Vec_WecPushLevel( vTt2Match );
    vArray = Vec_WecEntry( vTt2Match, iFunc );
    // create match
    Mat.fCompl = fCompl;
    assert( nFans == (int)pCell->nFanins );
    for ( i = 0; i < nFans; i++ )
    {
        Mat.Perm  |= (unsigned)(i << (Abc_Lit2Var(pFans[i]) << 2));
        Mat.Phase |= (unsigned)(Abc_LitIsCompl(pFans[i]) << Abc_Lit2Var(pFans[i]));
    }
    // check other profiles
    if ( fPinFilter )
    {
        // get profile array
        assert( Vec_WecSize(vTt2Match) == Vec_WecSize(vProfs) );
        if ( iFunc == Vec_WecSize(vProfs) )
            Vec_WecPushLevel( vProfs );
        vArrayProfs = Vec_WecEntry( vProfs, iFunc );
        assert( Vec_IntSize(vArray) == 2 * Vec_IntSize(vArrayProfs) );
        // skip dominated matches
        Vec_IntForEachEntryDouble( vArray, GateId, Entry, i )
            if ( Nf_Int2Cfg(Entry).Phase == Mat.Phase && Nf_Int2Cfg(Entry).fCompl == Mat.fCompl )
            {
                int Offset = Vec_IntEntry(vArrayProfs, i/2);
                int * pProf = Vec_IntEntryP(vStore, Offset);
                if ( Nf_StoCellIsDominated(pCell, pFans, pProf) )
                    return;
            }
    }
    // check pin permutation
    if ( !fPinPerm ) // do not use  pin-permutation (improves delay when pin-delays differ)
    {
        if ( fPinQuick ) // reduce the number of matches agressively
        {
            Vec_IntForEachEntryDouble( vArray, GateId, Entry, i )
                if ( GateId == (int)pCell->Id && Abc_TtBitCount8[Nf_Int2Cfg(Entry).Phase] == Abc_TtBitCount8[Mat.Phase] )
                    return;
        }
        else // reduce the number of matches less agressively
        {
            Vec_IntForEachEntryDouble( vArray, GateId, Entry, i )
                if ( GateId == (int)pCell->Id && Nf_Int2Cfg(Entry).Phase == Mat.Phase )
                    return;
        }
    }
    // save data and profile
    Vec_IntPush( vArray, pCell->Id );
    Vec_IntPush( vArray, Nf_Cfg2Int(Mat) );
    // add delay profile
    if ( fPinFilter )
    {
        Vec_IntPush( vArrayProfs, Vec_IntSize(vStore) );
        Vec_IntPush( vStore, Abc_Float2Int(pCell->AreaF) );
        for ( k = 0; k < nFans; k++ )
            Vec_IntPush( vStore, pCell->iDelays[Abc_Lit2Var(pFans[k])] );
    }
}
void Nf_StoCreateGateMaches( Vec_Mem_t * vTtMem, Vec_Wec_t * vTt2Match, Mio_Cell2_t * pCell, int ** pComp, int ** pPerm, int * pnPerms, Vec_Wec_t * vProfs, Vec_Int_t * vStore, int fPinFilter, int fPinPerm, int fPinQuick )
{
    int Perm[NF_LEAF_MAX], * Perm1, * Perm2;
    int nPerms = pnPerms[pCell->nFanins];
    int nMints = (1 << pCell->nFanins);
    word tCur, tTemp1, tTemp2;
    int i, p, c;
    assert( pCell->nFanins <= 6 );
    for ( i = 0; i < (int)pCell->nFanins; i++ )
        Perm[i] = Abc_Var2Lit( i, 0 );
    tCur = tTemp1 = pCell->uTruth;
    for ( p = 0; p < nPerms; p++ )
    {
        tTemp2 = tCur;
        for ( c = 0; c < nMints; c++ )
        {
            Nf_StoCreateGateAdd( vTtMem, vTt2Match, pCell, tCur, Perm, pCell->nFanins, vProfs, vStore, fPinFilter, fPinPerm, fPinQuick );
            // update
            tCur  = Abc_Tt6Flip( tCur, pComp[pCell->nFanins][c] );
            Perm1 = Perm + pComp[pCell->nFanins][c];
            *Perm1 = Abc_LitNot( *Perm1 );
        }
        assert( tTemp2 == tCur );
        if ( nPerms == 1 )
            continue;
        // update
        tCur = Abc_Tt6SwapAdjacent( tCur, pPerm[pCell->nFanins][p] );
        Perm1 = Perm + pPerm[pCell->nFanins][p];
        Perm2 = Perm1 + 1;
        ABC_SWAP( int, *Perm1, *Perm2 );
    }
    assert( tTemp1 == tCur );
}
Mio_Cell2_t * Nf_StoDeriveMatches( Vec_Mem_t * vTtMem, Vec_Wec_t * vTt2Match, int * pnCells, int fPinFilter, int fPinPerm, int fPinQuick )
{
    int fVerbose = 0;
    //abctime clk = Abc_Clock();
    Vec_Wec_t * vProfs = Vec_WecAlloc( 1000 );
    Vec_Int_t * vStore = Vec_IntAlloc( 10000 );
    int * pComp[7], * pPerm[7], nPerms[7], i;
    Mio_Cell2_t * pCells;
    Vec_WecPushLevel( vProfs );
    Vec_WecPushLevel( vProfs );
    for ( i = 1; i <= 6; i++ )
        pComp[i] = Extra_GreyCodeSchedule( i );
    for ( i = 1; i <= 6; i++ )
        pPerm[i] = Extra_PermSchedule( i );
    for ( i = 1; i <= 6; i++ )
        nPerms[i] = Extra_Factorial( i );
    pCells = Mio_CollectRootsNewDefault2( 6, pnCells, fVerbose );
    if ( pCells != NULL )
    for ( i = 2; i < *pnCells; i++ )
        Nf_StoCreateGateMaches( vTtMem, vTt2Match, pCells+i, pComp, pPerm, nPerms, vProfs, vStore, fPinFilter, fPinPerm, fPinQuick );
    for ( i = 1; i <= 6; i++ )
        ABC_FREE( pComp[i] );
    for ( i = 1; i <= 6; i++ )
        ABC_FREE( pPerm[i] );
    Vec_WecFree( vProfs );
    Vec_IntFree( vStore );
    //Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    return pCells;
}
void Nf_StoPrintOne( Nf_Man_t * p, int Count, int t, int i, int GateId, Nf_Cfg_t Mat )
{
    Mio_Cell2_t * pC = p->pCells + GateId;
    word * pTruth = Vec_MemReadEntry(p->vTtMem, t);
    int k, nSuppSize = Abc_TtSupportSize(pTruth, 6);
    printf( "%6d : ", Count );
    printf( "%6d : ", t );
    printf( "%6d : ", i );
    printf( "Gate %16s  ",   pC->pName );
    printf( "Area =%8.2f  ", pC->AreaF );
    printf( "In = %d   ",    pC->nFanins );
    if ( Mat.fCompl )
        printf( " compl " );
    else
        printf( "       " );
    for ( k = 0; k < (int)pC->nFanins; k++ )
    {
        int fComplF = (Mat.Phase >> k) & 1;
        int iFanin  = (Mat.Perm >> (3*k)) & 7;
        printf( "%c", 'a' + iFanin - fComplF * ('a' - 'A') );
    }
    printf( "  " );
    Dau_DsdPrintFromTruth( pTruth, nSuppSize );
}
void Nf_StoPrint( Nf_Man_t * p, int fVerbose )
{
    int t, i, GateId, Entry, Count = 0;
    for ( t = 2; t < Vec_WecSize(p->vTt2Match); t++ )
    {
        Vec_Int_t * vArr = Vec_WecEntry( p->vTt2Match, t );
        Vec_IntForEachEntryDouble( vArr, GateId, Entry, i )
        {
            Count++;
            if ( !fVerbose )
                continue;
            //if ( t < 10 )
            //    Nf_StoPrintOne( p, Count, t, i/2, GateId, Pf_Int2Mat(Entry) );
        }
    }
    printf( "Gates = %d.  Truths = %d.  Matches = %d.\n", 
        p->nCells, Vec_MemEntryNum(p->vTtMem), Count );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nf_Man_t * Nf_StoCreate( Gia_Man_t * pGia, Jf_Par_t * pPars )
{
    extern void Mf_ManSetFlowRefs( Gia_Man_t * p, Vec_Int_t * vRefs );
    Vec_Int_t * vFlowRefs;
    Nf_Man_t * p;
    int i, Entry;
    assert( pPars->nCutNum > 1  && pPars->nCutNum <= NF_CUT_MAX );
    assert( pPars->nLutSize > 1 && pPars->nLutSize <= NF_LEAF_MAX );
    ABC_FREE( pGia->pRefs );
    Vec_IntFreeP( &pGia->vCellMapping );
    if ( Gia_ManHasChoices(pGia) )
        Gia_ManSetPhase(pGia);
    // create
    p = ABC_CALLOC( Nf_Man_t, 1 );
    p->clkStart = Abc_Clock();
    p->pGia     = pGia;
    p->pPars    = pPars;
    p->pNfObjs  = ABC_CALLOC( Nf_Obj_t, Gia_ManObjNum(pGia) );
    p->iCur     = 2;
    // other
    Vec_PtrGrow( &p->vPages, 256 );                                    // cut memory
    Vec_IntFill( &p->vMapRefs,  2*Gia_ManObjNum(pGia), 0 );            // mapping refs   (2x)
    Vec_FltFill( &p->vFlowRefs, 2*Gia_ManObjNum(pGia), 0 );            // flow refs      (2x)
    Vec_IntFill( &p->vRequired, 2*Gia_ManObjNum(pGia), SCL_INFINITY ); // required times (2x)
    Vec_IntFill( &p->vCutSets,  Gia_ManObjNum(pGia), 0 );              // cut offsets
    Vec_FltFill( &p->vCutFlows, Gia_ManObjNum(pGia), 0 );              // cut area
    Vec_IntFill( &p->vCutDelays,Gia_ManObjNum(pGia), 0 );              // cut delay
    Vec_IntGrow( &p->vBackup, 1000 );
    // references
    vFlowRefs = Vec_IntAlloc(0);
    Mf_ManSetFlowRefs( pGia, vFlowRefs );
    Vec_IntForEachEntry( vFlowRefs, Entry, i )
    {
        Vec_FltWriteEntry( &p->vFlowRefs, 2*i,   /*0.5* */Entry );
        Vec_FltWriteEntry( &p->vFlowRefs, 2*i+1, /*0.5* */Entry );
    }
    Vec_IntFree(vFlowRefs);
    // matching
    Mio_LibraryMatchesFetch( (Mio_Library_t *)Abc_FrameReadLibGen(), &p->vTtMem, &p->vTt2Match, &p->pCells, &p->nCells, p->pPars->fPinFilter, p->pPars->fPinPerm, p->pPars->fPinQuick );
    if ( p->pCells == NULL )
        return NULL;
    p->InvDelayI = p->pCells[3].iDelays[0];
    p->InvAreaW  = p->pCells[3].AreaW;
    p->InvAreaF  = p->pCells[3].AreaF;
    Nf_ObjMatchD(p, 0, 0)->Gate = 0;
    Nf_ObjMatchD(p, 0, 1)->Gate = 1;
    // prepare cuts
    return p;
}
void Nf_StoDelete( Nf_Man_t * p )
{
    Vec_PtrFreeData( &p->vPages );
    ABC_FREE( p->vPages.pArray );
    ABC_FREE( p->vMapRefs.pArray );
    ABC_FREE( p->vFlowRefs.pArray );
    ABC_FREE( p->vRequired.pArray );
    ABC_FREE( p->vCutSets.pArray );
    ABC_FREE( p->vCutFlows.pArray );
    ABC_FREE( p->vCutDelays.pArray );
    ABC_FREE( p->vBackup.pArray );
    ABC_FREE( p->pNfObjs );
    ABC_FREE( p );
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Nf_CutComputeTruth6( Nf_Man_t * p, Nf_Cut_t * pCut0, Nf_Cut_t * pCut1, int fCompl0, int fCompl1, Nf_Cut_t * pCutR, int fIsXor )
{
//    extern int Nf_ManTruthCanonicize( word * t, int nVars );
    int nOldSupp = pCutR->nLeaves, truthId, fCompl; word t;
    word t0 = *Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut0->iFunc));
    word t1 = *Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut1->iFunc));
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
    pCutR->Useless = Nf_ObjCutUseless( p, truthId );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
}
static inline int Nf_CutComputeTruthMux6( Nf_Man_t * p, Nf_Cut_t * pCut0, Nf_Cut_t * pCut1, Nf_Cut_t * pCutC, int fCompl0, int fCompl1, int fComplC, Nf_Cut_t * pCutR )
{
    int nOldSupp = pCutR->nLeaves, truthId, fCompl; word t;
    word t0 = *Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut0->iFunc));
    word t1 = *Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut1->iFunc));
    word tC = *Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCutC->iFunc));
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
    pCutR->Useless = Nf_ObjCutUseless( p, truthId );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Nf_CutCountBits( word i )
{
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F);
    return (i*(0x0101010101010101))>>56;
}
static inline word Nf_CutGetSign( int * pLeaves, int nLeaves )
{
    word Sign = 0; int i; 
    for ( i = 0; i < nLeaves; i++ )
        Sign |= ((word)1) << (pLeaves[i] & 0x3F);
    return Sign;
}
static inline int Nf_CutCreateUnit( Nf_Cut_t * p, int i )
{
    p->Delay      = 0;
    p->Flow       = 0;
    p->iFunc      = 2;
    p->nLeaves    = 1;
    p->pLeaves[0] = i;
    p->Sign       = ((word)1) << (i & 0x3F);
    return 1;
}
static inline void Nf_CutPrint( Nf_Man_t * p, Nf_Cut_t * pCut )
{
    int i, nDigits = Abc_Base10Log(Gia_ManObjNum(p->pGia)); 
    printf( "%d  {", pCut->nLeaves );
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        printf( " %*d", nDigits, pCut->pLeaves[i] );
    for ( ; i < (int)p->pPars->nLutSize; i++ )
        printf( " %*s", nDigits, " " );
    printf( "  }   Useless = %d. D = %4d  A = %9.4f  F = %6d  ", 
        pCut->Useless, pCut->Delay, pCut->Flow, pCut->iFunc );
    if ( p->vTtMem )
        Dau_DsdPrintFromTruth( Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut->iFunc)), pCut->nLeaves );
    else
        printf( "\n" );
}
static inline int Nf_ManPrepareCuts( Nf_Cut_t * pCuts, Nf_Man_t * p, int iObj, int fAddUnit )
{
    if ( Nf_ObjHasCuts(p, iObj) )
    {
        Nf_Cut_t * pMfCut = pCuts;
        int i, * pCut, * pList = Nf_ObjCutSet(p, iObj);
        Nf_SetForEachCut( pList, pCut, i )
        {
            pMfCut->Delay   = 0;
            pMfCut->Flow    = 0;
            pMfCut->iFunc   = Nf_CutFunc( pCut );
            pMfCut->nLeaves = Nf_CutSize( pCut );
            pMfCut->Sign    = Nf_CutGetSign( pCut+1, Nf_CutSize(pCut) );
            pMfCut->Useless = Nf_ObjCutUseless( p, Abc_Lit2Var(pMfCut->iFunc) );
            memcpy( pMfCut->pLeaves, pCut+1, sizeof(int) * Nf_CutSize(pCut) );
            pMfCut++;
        }
        if ( fAddUnit && pCuts->nLeaves > 1 )
            return pList[0] + Nf_CutCreateUnit( pMfCut, iObj );
        return pList[0];
    }
    return Nf_CutCreateUnit( pCuts, iObj );
}
static inline int Nf_ManSaveCuts( Nf_Man_t * p, Nf_Cut_t ** pCuts, int nCuts, int fUseful )
{
    int i, * pPlace, iCur, nInts = 1, nCutsNew = 0;
    for ( i = 0; i < nCuts; i++ )
        if ( !fUseful || !pCuts[i]->Useless )
            nInts += pCuts[i]->nLeaves + 1, nCutsNew++;
    if ( (p->iCur & 0xFFFF) + nInts > 0xFFFF )
        p->iCur = ((p->iCur >> 16) + 1) << 16;
    if ( Vec_PtrSize(&p->vPages) == (p->iCur >> 16) )
        Vec_PtrPush( &p->vPages, ABC_ALLOC(int, (1<<16)) );
    iCur = p->iCur; p->iCur += nInts;
    pPlace = Nf_ManCutSet( p, iCur );
    *pPlace++ = nCutsNew;
    for ( i = 0; i < nCuts; i++ )
        if ( !fUseful || !pCuts[i]->Useless )
        {
            *pPlace++ = Nf_CutSetBoth( pCuts[i]->nLeaves, pCuts[i]->iFunc );
            memcpy( pPlace, pCuts[i]->pLeaves, sizeof(int) * pCuts[i]->nLeaves );
            pPlace += pCuts[i]->nLeaves;
        }
    return iCur;
}
static inline int Nf_ManCountUseful( Nf_Cut_t ** pCuts, int nCuts )
{
    int i, Count = 0;
    for ( i = 0; i < nCuts; i++ )
        Count += !pCuts[i]->Useless;
    return Count;
}
static inline int Nf_ManCountMatches( Nf_Man_t * p, Nf_Cut_t ** pCuts, int nCuts )
{
    int i, Count = 0;
    for ( i = 0; i < nCuts; i++ )
        if ( !pCuts[i]->Useless )
            Count += Vec_IntSize(Vec_WecEntry(p->vTt2Match, Abc_Lit2Var(pCuts[i]->iFunc))) / 2;
    return Count;
}

/**Function*************************************************************

  Synopsis    [Check correctness of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Nf_CutCheck( Nf_Cut_t * pBase, Nf_Cut_t * pCut ) // check if pCut is contained in pBase
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
static inline int Nf_SetCheckArray( Nf_Cut_t ** ppCuts, int nCuts )
{
    Nf_Cut_t * pCut0, * pCut1; 
    int i, k, m, n, Value;
    assert( nCuts > 0 );
    for ( i = 0; i < nCuts; i++ )
    {
        pCut0 = ppCuts[i];
        assert( pCut0->nLeaves <= NF_LEAF_MAX );
        assert( pCut0->Sign == Nf_CutGetSign(pCut0->pLeaves, pCut0->nLeaves) );
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
            Value = Nf_CutCheck( pCut0, pCut1 );
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
static inline int Nf_CutMergeOrder( Nf_Cut_t * pCut0, Nf_Cut_t * pCut1, Nf_Cut_t * pCut, int nLutSize )
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
        pCut->iFunc = NF_NO_FUNC;
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
    pCut->iFunc = NF_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;

FlushCut1:
    if ( c + nSize1 > nLutSize + k ) return 0;
    while ( k < nSize1 )
        pC[c++] = pC1[k++];
    pCut->nLeaves = c;
    pCut->iFunc = NF_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;
}
static inline int Nf_CutMergeOrderMux( Nf_Cut_t * pCut0, Nf_Cut_t * pCut1, Nf_Cut_t * pCut2, Nf_Cut_t * pCut, int nLutSize )
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
    pCut->iFunc = NF_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign | pCut2->Sign;
    return 1;
}
static inline int Nf_SetCutIsContainedOrder( Nf_Cut_t * pBase, Nf_Cut_t * pCut ) // check if pCut is contained in pBase
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
static inline int Nf_SetLastCutIsContained( Nf_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[i]->nLeaves <= pCuts[nCuts]->nLeaves && (pCuts[i]->Sign & pCuts[nCuts]->Sign) == pCuts[i]->Sign && Nf_SetCutIsContainedOrder(pCuts[nCuts], pCuts[i]) )
            return 1;
    return 0;
}
static inline int Nf_SetLastCutContainsArea( Nf_Cut_t ** pCuts, int nCuts )
{
    int i, k, fChanges = 0;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[nCuts]->nLeaves < pCuts[i]->nLeaves && (pCuts[nCuts]->Sign & pCuts[i]->Sign) == pCuts[nCuts]->Sign && Nf_SetCutIsContainedOrder(pCuts[i], pCuts[nCuts]) )
            pCuts[i]->nLeaves = NF_NO_LEAF, fChanges = 1;
    if ( !fChanges )
        return nCuts;
    for ( i = k = 0; i <= nCuts; i++ )
    {
        if ( pCuts[i]->nLeaves == NF_NO_LEAF )
            continue;
        if ( k < i )
            ABC_SWAP( Nf_Cut_t *, pCuts[k], pCuts[i] );
        k++;
    }
    return k - 1;
}
static inline int Nf_CutCompareArea( Nf_Cut_t * pCut0, Nf_Cut_t * pCut1 )
{
    if ( pCut0->Useless < pCut1->Useless )  return -1;
    if ( pCut0->Useless > pCut1->Useless )  return  1;
    if ( pCut0->Flow    < pCut1->Flow - NF_EPSILON )  return -1;
    if ( pCut0->Flow    > pCut1->Flow + NF_EPSILON )  return  1;
    if ( pCut0->Delay   < pCut1->Delay   )  return -1;
    if ( pCut0->Delay   > pCut1->Delay   )  return  1;
    if ( pCut0->nLeaves < pCut1->nLeaves )  return -1;
    if ( pCut0->nLeaves > pCut1->nLeaves )  return  1;
    return 0;
}
static inline void Nf_SetSortByArea( Nf_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = nCuts; i > 0; i-- )
    {
        if ( Nf_CutCompareArea(pCuts[i - 1], pCuts[i]) < 0 )//!= 1 )
            return;
        ABC_SWAP( Nf_Cut_t *, pCuts[i - 1], pCuts[i] );
    }
}
static inline int Nf_SetAddCut( Nf_Cut_t ** pCuts, int nCuts, int nCutNum )
{
    if ( nCuts == 0 )
        return 1;
    nCuts = Nf_SetLastCutContainsArea(pCuts, nCuts);
    Nf_SetSortByArea( pCuts, nCuts );
    return Abc_MinInt( nCuts + 1, nCutNum - 1 );
}
static inline int Nf_CutArea( Nf_Man_t * p, int nLeaves )
{
    if ( nLeaves < 2 )
        return 0;
    return nLeaves + p->pPars->nAreaTuner;
}
static inline void Nf_CutParams( Nf_Man_t * p, Nf_Cut_t * pCut, float FlowRefs )
{
    int i, nLeaves = pCut->nLeaves; 
    assert( nLeaves <= p->pPars->nLutSize );
    pCut->Delay = 0;
    pCut->Flow  = 0;
    for ( i = 0; i < nLeaves; i++ )
    {
        pCut->Delay = Abc_MaxInt( pCut->Delay, Nf_ObjCutDelay(p, pCut->pLeaves[i]) );
        pCut->Flow += Nf_ObjCutFlow(p, pCut->pLeaves[i]);
    }
    pCut->Delay += (int)(nLeaves > 1);
    pCut->Flow = (pCut->Flow + Nf_CutArea(p, nLeaves)) / FlowRefs;
}
void Nf_ObjMergeOrder( Nf_Man_t * p, int iObj )
{
    Nf_Cut_t pCuts0[NF_CUT_MAX], pCuts1[NF_CUT_MAX], pCuts[NF_CUT_MAX], * pCutsR[NF_CUT_MAX];
    Gia_Obj_t * pObj = Gia_ManObj(p->pGia, iObj);
    //Nf_Obj_t * pBest = Nf_ManObj(p, iObj);
    float dFlowRefs  = Nf_ObjFlowRefs(p, iObj, 0) + Nf_ObjFlowRefs(p, iObj, 1);
    int nLutSize = p->pPars->nLutSize;
    int nCutNum  = p->pPars->nCutNum;
    int nCuts0   = Nf_ManPrepareCuts(pCuts0, p, Gia_ObjFaninId0(pObj, iObj), 1);
    int nCuts1   = Nf_ManPrepareCuts(pCuts1, p, Gia_ObjFaninId1(pObj, iObj), 1);
    int fComp0   = Gia_ObjFaninC0(pObj);
    int fComp1   = Gia_ObjFaninC1(pObj);
    int iSibl    = Gia_ObjSibl(p->pGia, iObj);
    Nf_Cut_t * pCut0, * pCut1, * pCut0Lim = pCuts0 + nCuts0, * pCut1Lim = pCuts1 + nCuts1;
    int i, nCutsUse, nCutsR = 0;
    assert( !Gia_ObjIsBuf(pObj) );
    for ( i = 0; i < nCutNum; i++ )
        pCutsR[i] = pCuts + i;
    if ( iSibl )
    {
        Nf_Cut_t pCuts2[NF_CUT_MAX];
        Gia_Obj_t * pObjE = Gia_ObjSiblObj(p->pGia, iObj);
        int fCompE = Gia_ObjPhase(pObj) ^ Gia_ObjPhase(pObjE);
        int nCuts2 = Nf_ManPrepareCuts(pCuts2, p, iSibl, 0);
        Nf_Cut_t * pCut2, * pCut2Lim = pCuts2 + nCuts2;
        for ( pCut2 = pCuts2; pCut2 < pCut2Lim; pCut2++ )
        {
            *pCutsR[nCutsR] = *pCut2;
            pCutsR[nCutsR]->iFunc = Abc_LitNotCond( pCutsR[nCutsR]->iFunc, fCompE );
            Nf_CutParams( p, pCutsR[nCutsR], dFlowRefs );
            nCutsR = Nf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    if ( Gia_ObjIsMuxId(p->pGia, iObj) )
    {
        Nf_Cut_t pCuts2[NF_CUT_MAX];
        int nCuts2  = Nf_ManPrepareCuts(pCuts2, p, Gia_ObjFaninId2(p->pGia, iObj), 1);
        int fComp2  = Gia_ObjFaninC2(p->pGia, pObj);
        Nf_Cut_t * pCut2, * pCut2Lim = pCuts2 + nCuts2;
        p->CutCount[0] += nCuts0 * nCuts1 * nCuts2;
        for ( pCut0 = pCuts0; pCut0 < pCut0Lim; pCut0++ )
        for ( pCut1 = pCuts1; pCut1 < pCut1Lim; pCut1++ )
        for ( pCut2 = pCuts2; pCut2 < pCut2Lim; pCut2++ )
        {
            if ( Nf_CutCountBits(pCut0->Sign | pCut1->Sign | pCut2->Sign) > nLutSize )
                continue;
            p->CutCount[1]++; 
            if ( !Nf_CutMergeOrderMux(pCut0, pCut1, pCut2, pCutsR[nCutsR], nLutSize) )
                continue;
            if ( Nf_SetLastCutIsContained(pCutsR, nCutsR) )
                continue;
            p->CutCount[2]++;
            if ( Nf_CutComputeTruthMux6(p, pCut0, pCut1, pCut2, fComp0, fComp1, fComp2, pCutsR[nCutsR]) )
                pCutsR[nCutsR]->Sign = Nf_CutGetSign(pCutsR[nCutsR]->pLeaves, pCutsR[nCutsR]->nLeaves);
            Nf_CutParams( p, pCutsR[nCutsR], dFlowRefs );
            nCutsR = Nf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    else
    {
        int fIsXor = Gia_ObjIsXor(pObj);
        p->CutCount[0] += nCuts0 * nCuts1;
        for ( pCut0 = pCuts0; pCut0 < pCut0Lim; pCut0++ )
        for ( pCut1 = pCuts1; pCut1 < pCut1Lim; pCut1++ )
        {
            if ( (int)(pCut0->nLeaves + pCut1->nLeaves) > nLutSize && Nf_CutCountBits(pCut0->Sign | pCut1->Sign) > nLutSize )
                continue;
            p->CutCount[1]++; 
            if ( !Nf_CutMergeOrder(pCut0, pCut1, pCutsR[nCutsR], nLutSize) )
                continue;
            if ( Nf_SetLastCutIsContained(pCutsR, nCutsR) )
                continue;
            p->CutCount[2]++;
            if ( Nf_CutComputeTruth6(p, pCut0, pCut1, fComp0, fComp1, pCutsR[nCutsR], fIsXor) )
                pCutsR[nCutsR]->Sign = Nf_CutGetSign(pCutsR[nCutsR]->pLeaves, pCutsR[nCutsR]->nLeaves);
            Nf_CutParams( p, pCutsR[nCutsR], dFlowRefs );
            nCutsR = Nf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    // debug printout
    if ( 0 )
//    if ( iObj % 10000 == 0 )
//    if ( iObj == 1090 )
    {
        printf( "*** Obj = %d  Useful = %d\n", iObj, Nf_ManCountUseful(pCutsR, nCutsR) );
        for ( i = 0; i < nCutsR; i++ )
            Nf_CutPrint( p, pCutsR[i] );
        printf( "\n" );
    } 
    // verify
    assert( nCutsR > 0 && nCutsR < nCutNum );
//    assert( Nf_SetCheckArray(pCutsR, nCutsR) );
    // store the cutset
    Nf_ObjSetCutFlow( p, iObj, pCutsR[0]->Flow );
    Nf_ObjSetCutDelay( p, iObj, pCutsR[0]->Delay );
    *Vec_IntEntryP(&p->vCutSets, iObj) = Nf_ManSaveCuts(p, pCutsR, nCutsR, 0);
    p->CutCount[3] += nCutsR;
    nCutsUse = Nf_ManCountUseful(pCutsR, nCutsR);
    p->CutCount[4] += nCutsUse;
    p->nCutUseAll  += nCutsUse == nCutsR;
    p->CutCount[5] += Nf_ManCountMatches(p, pCutsR, nCutsR);
}
void Nf_ManComputeCuts( Nf_Man_t * p )
{
    Gia_Obj_t * pObj; int i, iFanin;
    Gia_ManForEachAnd( p->pGia, pObj, i )
        if ( Gia_ObjIsBuf(pObj) )
        {
            iFanin = Gia_ObjFaninId0(pObj, i);
            Nf_ObjSetCutFlow( p, i,  Nf_ObjCutFlow(p, iFanin) );
            Nf_ObjSetCutDelay( p, i, Nf_ObjCutDelay(p, iFanin) );
        }
        else
            Nf_ObjMergeOrder( p, i );
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nf_ManPrintStats( Nf_Man_t * p, char * pTitle )
{
    if ( !p->pPars->fVerbose )
        return;
    printf( "%s :  ", pTitle );
    printf( "Delay =%8.2f  ",  Scl_Int2Flt(p->pPars->MapDelay) );
    printf( "Area =%12.2f  ",  p->pPars->MapAreaF );
    printf( "Gate =%6d  ",    (int)p->pPars->Area );
    printf( "Inv =%6d  ",     (int)p->nInvs );
    printf( "Edge =%7d  ",    (int)p->pPars->Edge );
    Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
    fflush( stdout );
}
void Nf_ManPrintInit( Nf_Man_t * p )
{
    int nChoices;
    if ( !p->pPars->fVerbose )
        return;
    printf( "LutSize = %d  ", p->pPars->nLutSize );
    printf( "CutNum = %d  ",  p->pPars->nCutNum );
    printf( "Iter = %d  ",    p->pPars->nRounds );//+ p->pPars->nRoundsEla );
    printf( "Coarse = %d   ", p->pPars->fCoarsen );
    printf( "Cells = %d  ",   p->nCells );
    printf( "Funcs = %d  ",   Vec_MemEntryNum(p->vTtMem) );
    printf( "Matches = %d  ", Vec_WecSizeSize(p->vTt2Match)/2 );
    printf( "And = %d  ",     Gia_ManAndNum(p->pGia) );
    nChoices = Gia_ManChoiceNum( p->pGia );
    if ( nChoices )
    printf( "Choices = %d  ", nChoices );
    printf( "\n" );
    printf( "Computing cuts...\r" );
    fflush( stdout );
}
void Nf_ManPrintQuit( Nf_Man_t * p )
{
    float MemGia   = Gia_ManMemory(p->pGia) / (1<<20);
    float MemMan   =(1.0 * sizeof(Nf_Obj_t) + 8.0 * sizeof(int)) * Gia_ManObjNum(p->pGia) / (1<<20);
    float MemCuts  = 1.0 * sizeof(int) * (1 << 16) * Vec_PtrSize(&p->vPages) / (1<<20);
    float MemTt    = p->vTtMem ? Vec_MemMemory(p->vTtMem) / (1<<20) : 0;
    if ( p->CutCount[0] == 0 )
        p->CutCount[0] = 1;
    if ( !p->pPars->fVerbose )
        return;
    printf( "CutPair = %.0f  ",         p->CutCount[0] );
    printf( "Merge = %.0f (%.1f)  ",    p->CutCount[1], 1.0*p->CutCount[1]/Gia_ManAndNum(p->pGia) );
    printf( "Eval = %.0f (%.1f)  ",     p->CutCount[2], 1.0*p->CutCount[2]/Gia_ManAndNum(p->pGia) );
    printf( "Cut = %.0f (%.1f)  ",      p->CutCount[3], 1.0*p->CutCount[3]/Gia_ManAndNum(p->pGia) );
    printf( "Use = %.0f (%.1f)  ",      p->CutCount[4], 1.0*p->CutCount[4]/Gia_ManAndNum(p->pGia) );
    printf( "Mat = %.0f (%.1f)  ",      p->CutCount[5], 1.0*p->CutCount[5]/Gia_ManAndNum(p->pGia) );
//    printf( "Equ = %d (%.2f %%)  ",     p->nCutUseAll,  100.0*p->nCutUseAll /p->CutCount[0] );
    printf( "\n" );
    printf( "Gia = %.2f MB  ",          MemGia );
    printf( "Man = %.2f MB  ",          MemMan ); 
    printf( "Cut = %.2f MB   ",         MemCuts );
    printf( "TT = %.2f MB  ",           MemTt ); 
    printf( "Total = %.2f MB   ",       MemGia + MemMan + MemCuts + MemTt ); 
//    printf( "\n" );
    Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
    fflush( stdout );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nf_ManCutMatchPrint( Nf_Man_t * p, int iObj, char * pStr, Nf_Mat_t * pM )
{
    Mio_Cell2_t * pCell;
    int i, * pCut;
    printf( "%5d %s : ", iObj, pStr );
    if ( pM->CutH == 0 )
    {
        printf( "Unassigned\n" );
        return;
    }
    pCell = Nf_ManCell( p, pM->Gate );
    pCut = Nf_CutFromHandle( Nf_ObjCutSet(p, iObj), pM->CutH );
    printf( "D =%6.2f  ", Scl_Int2Flt(pM->D) );
    printf( "A =%6.2f  ", pM->F );
    printf( "C = %d ", pM->fCompl );
//    printf( "B = %d ", pM->fBest );
    printf( "  " );
    printf( "Cut = {" );
    for ( i = 0; i < (int)pCell->nFanins; i++ )
        printf( "%4d ", Nf_CutLeaves(pCut)[i] );
    for ( ; i < 6; i++ )
        printf( "     " );
    printf( "}  " );
    printf( "%10s ", pCell->pName );
    printf( "%d  ", pCell->nFanins );
    printf( "{" );
    for ( i = 0; i < (int)pCell->nFanins; i++ )
        printf( "%6.2f ", Scl_Int2Flt(pCell->iDelays[i]) );
    for ( ; i < 6; i++ )
        printf( "       " );
    printf( " } " );
    for ( i = 0; i < (int)pCell->nFanins; i++ )
        printf( "%s%d ", Nf_CfgCompl(pM->Cfg, i) ? "!":" ", Nf_CfgVar(pM->Cfg, i) );
    for ( ; i < 6; i++ )
        printf( "  " );
    Dau_DsdPrintFromTruth( &pCell->uTruth, pCell->nFanins );
}
void Nf_ManCutMatchOne( Nf_Man_t * p, int iObj, int * pCut, int * pCutSet )
{
    Nf_Obj_t * pBest = Nf_ManObj(p, iObj);
    int * pFans      = Nf_CutLeaves(pCut);
    int nFans        = Nf_CutSize(pCut);
    int iFuncLit     = Nf_CutFunc(pCut);
    int fComplExt    = Abc_LitIsCompl(iFuncLit);
    Vec_Int_t * vArr = Vec_WecEntry( p->vTt2Match, Abc_Lit2Var(iFuncLit) );
    int i, k, c, Info, Offset, iFanin, fComplF;
    int ArrivalD, ArrivalA;
    Nf_Mat_t * pD, * pA;
    // assign fanins matches
    Nf_Obj_t * pBestF[NF_LEAF_MAX];
    for ( i = 0; i < nFans; i++ )
        pBestF[i] = Nf_ManObj( p, pFans[i] );
    // special cases
    if ( nFans == 0 )
    {
        int Const = (iFuncLit == 1);
        assert( iFuncLit == 0 || iFuncLit == 1 );
        for ( c = 0; c < 2; c++ )
        { 
            pD = Nf_ObjMatchD( p, iObj, c );
            pA = Nf_ObjMatchA( p, iObj, c );
            pD->D = pA->D = 0;
            pD->F = pA->F = p->pCells[c ^ Const].AreaF;
            pD->CutH = pA->CutH = Nf_CutHandle(pCutSet, pCut);
            pD->Gate = pA->Gate = c ^ Const;
//            pD->Conf = pA->Conf = 0;
            pD->Cfg = pA->Cfg = Nf_Int2Cfg(0);
        }
        return;
    }
    // consider matches of this function
    Vec_IntForEachEntryDouble( vArr, Info, Offset, i )
    {
        Nf_Cfg_t Cfg   = Nf_Int2Cfg(Offset);
        Mio_Cell2_t*pC = Nf_ManCell( p, Info );
        int fCompl     = Cfg.fCompl ^ fComplExt;
        int Required   = Nf_ObjRequired( p, iObj, fCompl ), Delay = 0;
        Nf_Mat_t * pD  = &pBest->M[fCompl][0];
        Nf_Mat_t * pA  = &pBest->M[fCompl][1];
        float AreaF    = pC->AreaF;
        assert( nFans == (int)pC->nFanins );
        Nf_CfgForEachVarCompl( Cfg, nFans, iFanin, fComplF, k )
        {
            ArrivalD  = pBestF[iFanin]->M[fComplF][0].D;
            ArrivalA  = pBestF[iFanin]->M[fComplF][1].D;
            if ( ArrivalA + pC->iDelays[k] <= Required && Required != SCL_INFINITY )
            {
                Delay = Abc_MaxInt( Delay, ArrivalA + pC->iDelays[k] );
                AreaF += pBestF[iFanin]->M[fComplF][1].F;
            }
            else 
            {
                if ( pD->D < SCL_INFINITY && pA->D < SCL_INFINITY && ArrivalD + pC->iDelays[k] > Required )
                    break;
                Delay = Abc_MaxInt( Delay, ArrivalD + pC->iDelays[k] );
                //AreaF += pBestF[iFanin]->M[fComplF][0].F;
                if ( AreaF >= (float)1e32 || pBestF[iFanin]->M[fComplF][0].F >= (float)1e32 )
                    AreaF = (float)1e32;
                else 
                    AreaF += pBestF[iFanin]->M[fComplF][0].F;
            }
        }
        if ( k < nFans )
            continue;
        // select best Cfgch
        if ( pD->D > Delay )
        {
            pD->D = Delay;
            pD->F = AreaF;
            pD->CutH = Nf_CutHandle(pCutSet, pCut);
            pD->Gate = pC->Id;
            pD->Cfg = Cfg;
            pD->Cfg.fCompl = 0;
        }

        if ( pA->F > AreaF + NF_EPSILON )
        {
            pA->D = Delay;
            pA->F = AreaF;
            pA->CutH = Nf_CutHandle(pCutSet, pCut);
            pA->Gate = pC->Id;
            pA->Cfg = Cfg;
            pA->Cfg.fCompl = 0;
        }
    }
}
static inline void Nf_ObjPrepareCi( Nf_Man_t * p, int iObj, int Time )
{
    Nf_Mat_t * pD0 = Nf_ObjMatchD( p, iObj, 0 );
    Nf_Mat_t * pA0 = Nf_ObjMatchA( p, iObj, 0 );
    Nf_Mat_t * pD = Nf_ObjMatchD( p, iObj, 1 );
    Nf_Mat_t * pA = Nf_ObjMatchA( p, iObj, 1 );
    pD0->D = pA0->D = pD->D = pA->D = Time;
    pD->fCompl = 1;
    pD->D += p->InvDelayI;
    pD->F = p->InvAreaF;
    pA->fCompl = 1;
    pA->D += p->InvDelayI;
    pA->F = p->InvAreaF;
    Nf_ObjMatchD( p, iObj, 0 )->fBest = 1;
    Nf_ObjMatchD( p, iObj, 1 )->fBest = 1;
}
static inline void Nf_ObjPrepareBuf( Nf_Man_t * p, Gia_Obj_t * pObj )
{
    // get fanin info
    int iObj = Gia_ObjId( p->pGia, pObj );
    int iFanin = Gia_ObjFaninId0( pObj, iObj );
    Nf_Mat_t * pDf = Nf_ObjMatchD( p, iFanin, Gia_ObjFaninC0(pObj) );
    //Nf_Mat_t * pAf = Nf_ObjMatchA( p, iFanin, Gia_ObjFaninC0(pObj) );
    // set the direct phase
    Nf_Mat_t * pDp = Nf_ObjMatchD( p, iObj, 0 );
    Nf_Mat_t * pAp = Nf_ObjMatchA( p, iObj, 0 );
    Nf_Mat_t * pDn = Nf_ObjMatchD( p, iObj, 1 );
    Nf_Mat_t * pAn = Nf_ObjMatchA( p, iObj, 1 );
    assert( Gia_ObjIsBuf(pObj) );
    memset( Nf_ManObj(p, iObj), 0, sizeof(Nf_Obj_t) );
    // set the direct phase
    pDp->D = pAp->D = pDf->D;
    pDp->F = pAp->F = pDf->F; // do not pass flow???
    pDp->fBest = 1;
    // set the inverted phase
    pDn->D = pAn->D = pDf->D + p->InvDelayI;
    pDn->F = pAn->F = pDf->F + p->InvAreaF;
    pDn->fCompl = pAn->fCompl = 1;
    pDn->fBest = 1;
}
static inline int Nf_CutRequired( Nf_Man_t * p, Nf_Mat_t * pM, int * pCutSet )
{
    Mio_Cell2_t * pCell = Nf_ManCell( p, pM->Gate );
    int * pCut = Nf_CutFromHandle( pCutSet, pM->CutH );
    int i, iVar, fCompl;
    int Arr, Req, Arrival = 0, Required = 0;
    Nf_CutForEachVarCompl( pCut, pM->Cfg, iVar, fCompl, i )
    {
        Arr     = Nf_ManObj(p, iVar)->M[fCompl][0].D + pCell->iDelays[i];
        Req     = Nf_ObjRequired(p, iVar, fCompl);
        Arrival = Abc_MaxInt( Arrival, Arr );
        if ( Req < SCL_INFINITY )
            Required = Abc_MaxInt( Required, Req + pCell->iDelays[i] );
    }
    return Abc_MaxInt( Required + p->pPars->nReqTimeFlex*p->InvDelayI, Arrival ); 
}
static inline void Nf_ObjComputeRequired( Nf_Man_t * p, int iObj )
{
    Nf_Obj_t * pBest = Nf_ManObj(p, iObj);
    int c, * pCutSet = Nf_ObjCutSet( p, iObj );
    for ( c = 0; c < 2; c++ )
        if ( Nf_ObjRequired(p, iObj, c) == SCL_INFINITY )
            Nf_ObjSetRequired( p, iObj, c, Nf_CutRequired(p, &pBest->M[c][0], pCutSet) );
}
void Nf_ManCutMatch( Nf_Man_t * p, int iObj )
{
    Nf_Obj_t * pBest = Nf_ManObj(p, iObj);
    Nf_Mat_t * pDp = &pBest->M[0][0];
    Nf_Mat_t * pDn = &pBest->M[1][0];
    Nf_Mat_t * pAp = &pBest->M[0][1];
    Nf_Mat_t * pAn = &pBest->M[1][1];
    float FlowRefPf  = Nf_ObjFlowRefs(p, iObj, 0);
    float FlowRefNf  = Nf_ObjFlowRefs(p, iObj, 1);
    int i, * pCut, * pCutSet = Nf_ObjCutSet( p, iObj );
    int Required[2] = {0};
    if ( p->Iter )
    {
        Nf_ObjComputeRequired( p, iObj );
        Required[0] = Nf_ObjRequired( p, iObj, 0 );
        Required[1] = Nf_ObjRequired( p, iObj, 1 );
    }
    memset( pBest, 0, sizeof(Nf_Obj_t) );
    pDp->D = SCL_INFINITY; pDp->F = FLT_MAX;
    pDn->D = SCL_INFINITY; pDn->F = FLT_MAX;
    pAp->D = SCL_INFINITY; pAp->F = FLT_MAX;
    pAn->D = SCL_INFINITY; pAn->F = FLT_MAX;
    Nf_SetForEachCut( pCutSet, pCut, i )
    {
        if ( Abc_Lit2Var(Nf_CutFunc(pCut)) >= Vec_WecSize(p->vTt2Match) )
            continue;
        assert( !Nf_CutIsTriv(pCut, iObj) );
        assert( Nf_CutSize(pCut) <= p->pPars->nLutSize );
        assert( Abc_Lit2Var(Nf_CutFunc(pCut)) < Vec_WecSize(p->vTt2Match) );
        Nf_ManCutMatchOne( p, iObj, pCut, pCutSet );
    }

/*
    if ( 461 == iObj && p->Iter == 0 )
    {
        printf( "\nObj %6d (%.2f %.2f):\n", iObj, Scl_Int2Flt(Required[0]), Scl_Int2Flt(Required[1]) );
        Nf_ManCutMatchPrint( p, iObj, "Dp", &pBest->M[0][0] );
        Nf_ManCutMatchPrint( p, iObj, "Dn", &pBest->M[1][0] );
        Nf_ManCutMatchPrint( p, iObj, "Ap", &pBest->M[0][1] );
        Nf_ManCutMatchPrint( p, iObj, "An", &pBest->M[1][1] );
        printf( "\n" );
    }
*/
    // divide by ref count
    pDp->F = pDp->F / FlowRefPf;
    pAp->F = pAp->F / FlowRefPf;
    pDn->F = pDn->F / FlowRefNf;
    pAn->F = pAn->F / FlowRefNf;

    // add the inverters
    assert( pDp->D < SCL_INFINITY || pDn->D < SCL_INFINITY );
    if ( pDp->D > pDn->D + p->InvDelayI )
    {
        *pDp = *pDn;
        pDp->D += p->InvDelayI;
        pDp->F += p->InvAreaF;
        pDp->fCompl = 1;
        if ( pAp->D == SCL_INFINITY )
            *pAp = *pDp;
        //printf( "Using inverter to improve delay at node %d in phase %d.\n", iObj, 1 );
    }
    else if ( pDn->D > pDp->D + p->InvDelayI )
    {
        *pDn = *pDp;
        pDn->D += p->InvDelayI;
        pDn->F += p->InvAreaF;
        pDn->fCompl = 1;
        if ( pAn->D == SCL_INFINITY )
            *pAn = *pDn;
        //printf( "Using inverter to improve delay at node %d in phase %d.\n", iObj, 0 );
    }
    //assert( pAp->F < FLT_MAX || pAn->F < FLT_MAX );
    // try replacing pos with neg
    if ( pAp->D == SCL_INFINITY || (pAp->F > pAn->F + p->InvAreaF + NF_EPSILON && pAn->D + p->InvDelayI <= Required[0]) )
    {
        assert( p->Iter > 0 );
        *pAp = *pAn;
        pAp->D += p->InvDelayI;
        pAp->F += p->InvAreaF;
        pAp->fCompl = 1;
        if ( pDp->D == SCL_INFINITY )
            *pDp = *pAp;
        //printf( "Using inverter to improve area at node %d in phase %d.\n", iObj, 1 );
    }
    // try replacing neg with pos
    else if ( pAn->D == SCL_INFINITY || (pAn->F > pAp->F + p->InvAreaF + NF_EPSILON && pAp->D + p->InvDelayI <= Required[1]) )
    {
        assert( p->Iter > 0 );
        *pAn = *pAp;
        pAn->D += p->InvDelayI;
        pAn->F += p->InvAreaF;
        pAn->fCompl = 1;
        if ( pDn->D == SCL_INFINITY )
            *pDn = *pAn;
        //printf( "Using inverter to improve area at node %d in phase %d.\n", iObj, 0 );
    }

    if ( pDp->D == SCL_INFINITY )
        printf( "Object %d has pDp unassigned.\n", iObj );
    if ( pDn->D == SCL_INFINITY )
        printf( "Object %d has pDn unassigned.\n", iObj );
    if ( pAp->D == SCL_INFINITY )
        printf( "Object %d has pAp unassigned.\n", iObj );
    if ( pAn->D == SCL_INFINITY )
        printf( "Object %d has pAn unassigned.\n", iObj );
/*
    pDp->F = Abc_MinFloat( pDp->F, FLT_MAX/SCL_NUM );
    pDn->F = Abc_MinFloat( pDn->F, FLT_MAX/SCL_NUM );
    pAp->F = Abc_MinFloat( pAp->F, FLT_MAX/SCL_NUM );  
    pAn->F = Abc_MinFloat( pAn->F, FLT_MAX/SCL_NUM );
*/    
    assert( pDp->D < SCL_INFINITY );
    assert( pDn->D < SCL_INFINITY );
    assert( pAp->D < SCL_INFINITY );
    assert( pAn->D < SCL_INFINITY );

    assert( pDp->F < FLT_MAX );
    assert( pDn->F < FLT_MAX );
    assert( pAp->F < FLT_MAX );
    assert( pAn->F < FLT_MAX );

/*
    if ( p->Iter && (pDp->D > Required[0] || pDn->D > Required[1]) )
    {
        printf( "%5d : ", iObj );
        printf( "Dp = %6.2f  ", Scl_Int2Flt(pDp->D) );
        printf( "Dn = %6.2f  ", Scl_Int2Flt(pDn->D) );
        printf( "  " );
        printf( "Ap = %6.2f  ", Scl_Int2Flt(pAp->D) );
        printf( "An = %6.2f  ", Scl_Int2Flt(pAn->D) );
        printf( "  " );
        printf( "Rp = %6.2f  ", Scl_Int2Flt(Required[0]) );
        printf( "Rn = %6.2f  ", Scl_Int2Flt(Required[1]) );
        printf( "\n" );
    }
*/
}
void Nf_ManComputeMapping( Nf_Man_t * p )
{
    Gia_Obj_t * pObj; int i;
    Gia_ManForEachAnd( p->pGia, pObj, i )
        if ( Gia_ObjIsBuf(pObj) )
            Nf_ObjPrepareBuf( p, pObj );
        else
            Nf_ManCutMatch( p, i );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Nf_Mat_t * Nf_ObjMatchBest( Nf_Man_t * p, int i, int c )             
{
    Nf_Mat_t * pD = Nf_ObjMatchD(p, i, c);
    Nf_Mat_t * pA = Nf_ObjMatchA(p, i, c);
    assert( pD->fBest != pA->fBest );
    //assert( Nf_ObjMapRefNum(p, i, c) > 0 );
    if ( pA->fBest )
        return pA;
    if ( pD->fBest )
        return pD;
    return NULL;
}
void Nf_ManSetOutputRequireds( Nf_Man_t * p, int fPropCompl )
{
    Gia_Obj_t * pObj;
    int Required = 0, MapDelayOld = p->pPars->MapDelay;
    int fUseConMan = Scl_ConIsRunning() && Scl_ConHasOutReqs();
    int i, iObj, fCompl, nLits = 2*Gia_ManObjNum(p->pGia);
    Vec_IntFill( &p->vRequired, nLits, SCL_INFINITY );
    // compute delay
    p->pPars->MapDelay = 0;
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        Required = Nf_ObjMatchD( p, Gia_ObjFaninId0p(p->pGia, pObj), Gia_ObjFaninC0(pObj) )->D;
        p->pPars->MapDelay = Abc_MaxInt( p->pPars->MapDelay, Required );
    }
    if ( p->Iter && MapDelayOld < p->pPars->MapDelay && p->pGia->vOutReqs == NULL )
        printf( "******** Critical delay violation %.2f -> %.2f ********\n", Scl_Int2Flt(MapDelayOld), Scl_Int2Flt(p->pPars->MapDelay) ); 
    p->pPars->MapDelay = Abc_MaxInt( p->pPars->MapDelay, MapDelayOld );
    // check delay target
    if ( p->pPars->MapDelayTarget == 0 && p->pPars->nRelaxRatio )
        p->pPars->MapDelayTarget = p->pPars->MapDelay * (100 + p->pPars->nRelaxRatio) / 100;
    if ( p->pPars->MapDelayTarget > 0 )
    {
        if ( p->pPars->MapDelay < p->pPars->MapDelayTarget )
            p->pPars->MapDelay = p->pPars->MapDelayTarget;
        else if ( p->pPars->nRelaxRatio == 0 )
            Abc_Print( 0, "Relaxing user-specified delay target from %.2f to %.2f.\n", Scl_Int2Flt(p->pPars->MapDelayTarget), Scl_Int2Flt(p->pPars->MapDelay) );
    }
    //assert( p->pPars->MapDelayTarget == 0 );
    // set required times
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        iObj     = Gia_ObjFaninId0p(p->pGia, pObj);
        fCompl   = Gia_ObjFaninC0(pObj);
        Required = Nf_ObjMatchD(p, iObj, fCompl)->D;
        Required = p->pPars->fDoAverage ? Required * (100 + p->pPars->nRelaxRatio) / 100 : p->pPars->MapDelay;
        // if external required time can be achieved, use it
        if ( fUseConMan )
        {
            if ( Scl_ConGetOutReq(i) > 0 && Required <= Scl_ConGetOutReq(i) )
                Required = Scl_ConGetOutReq(i);
        }
        else if ( p->pGia->vOutReqs )
        {
            int NewRequired = Scl_Flt2Int(Vec_FltEntry(p->pGia->vOutReqs, i));
            if ( NewRequired > 0 && Required <= NewRequired )
                Required = Abc_MinInt( 2*Required, NewRequired );
        }
        // if external required cannot be achieved, set the earliest possible arrival time
//        else if ( p->pGia->vOutReqs && Vec_FltEntry(p->pGia->vOutReqs, i) > 0 && Required > Vec_FltEntry(p->pGia->vOutReqs, i) )
//            ptTime->Rise = ptTime->Fall = ptTime->Worst = Required;
        // otherwise, set the global required time
        Nf_ObjUpdateRequired( p, iObj, fCompl, Required );
        if ( fPropCompl && iObj > 0 && Nf_ObjMatchBest(p, iObj, fCompl)->fCompl )
            Nf_ObjUpdateRequired( p, iObj, !fCompl, Required - p->InvDelayI );
        //Nf_ObjMapRefInc( p, Gia_ObjFaninId0p(p->pGia, pObj), Gia_ObjFaninC0(pObj));
    }
}
void Nf_ManSetMapRefsGate( Nf_Man_t * p, int iObj, int Required, Nf_Mat_t * pM )
{
    int k, iVar, fCompl;
    Mio_Cell2_t * pCell = Nf_ManCell( p, pM->Gate );
    int * pCut = Nf_CutFromHandle( Nf_ObjCutSet(p, iObj), pM->CutH );
    Nf_CutForEachVarCompl( pCut, pM->Cfg, iVar, fCompl, k )
    {
        Nf_ObjMapRefInc( p, iVar, fCompl );
        Nf_ObjUpdateRequired( p, iVar, fCompl, Required - pCell->iDelays[k] );
    }
    assert( Nf_CutSize(pCut) == (int)pCell->nFanins );
    // update global stats
    p->pPars->MapAreaF += pCell->AreaF;
    p->pPars->Edge += Nf_CutSize(pCut);
    p->pPars->Area++;
    // update status of the gate
    assert( pM->fBest == 0 );
    pM->fBest = 1;
}
void Nf_ManPrintMatches( Nf_Man_t * p )
{
    Gia_Obj_t * pObj; int i;
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
        Nf_Mat_t * pDp = Nf_ObjMatchD( p, i, 0 );
        Nf_Mat_t * pAp = Nf_ObjMatchA( p, i, 0 );
        Nf_Mat_t * pDn = Nf_ObjMatchD( p, i, 1 );
        Nf_Mat_t * pAn = Nf_ObjMatchA( p, i, 1 );

        printf( "%5d : ", i );
        printf( "Dp = %6.2f  ", Scl_Int2Flt(pDp->D) );
        printf( "Dn = %6.2f  ", Scl_Int2Flt(pDn->D) );
        printf( "  " );
        printf( "Ap = %6.2f  ", Scl_Int2Flt(pAp->D) );
        printf( "An = %6.2f  ", Scl_Int2Flt(pAn->D) );
        printf( "  " );
        printf( "Dp = %8s ", Nf_ManCell(p, pDp->Gate)->pName );
        printf( "Dn = %8s ", Nf_ManCell(p, pDn->Gate)->pName );
        printf( "Ap = %8s ", Nf_ManCell(p, pAp->Gate)->pName );
        printf( "An = %8s ", Nf_ManCell(p, pAn->Gate)->pName );
        printf( "\n" );

    }
}
int Nf_ManSetMapRefs( Nf_Man_t * p )
{
    float Coef = 1.0 / (1.0 + (p->Iter + 1) * (p->Iter + 1));
    float * pFlowRefs = Vec_FltArray( &p->vFlowRefs );
    int * pMapRefs = Vec_IntArray( &p->vMapRefs );
    int nLits = 2*Gia_ManObjNum(p->pGia);
    int i, c, Id, nRefs[2];
    Gia_Obj_t * pObj;
    Nf_Mat_t * pD, * pA, * pM;
    Nf_Mat_t * pDs[2], * pAs[2], * pMs[2];
    int Required = 0, Requireds[2];
    assert( !p->fUseEla );
//    if ( p->Iter == 0 )
//        Nf_ManPrintMatches( p );
    Nf_ManSetOutputRequireds( p, 0 );
    // set output references
    memset( pMapRefs, 0, sizeof(int) * nLits );
    Gia_ManForEachCo( p->pGia, pObj, i )
        Nf_ObjMapRefInc( p, Gia_ObjFaninId0p(p->pGia, pObj), Gia_ObjFaninC0(pObj));

    // compute area and edges
    p->nInvs = 0;
    p->pPars->MapAreaF = 0; 
    p->pPars->Area = p->pPars->Edge = 0;
    Gia_ManForEachAndReverse( p->pGia, pObj, i )
    {
        if ( Gia_ObjIsBuf(pObj) )
        {
            if ( Nf_ObjMapRefNum(p, i, 1) )
            {
                Nf_ObjMapRefInc( p, i, 0 );
                Nf_ObjUpdateRequired( p, i, 0, Nf_ObjRequired(p, i, 1) - p->InvDelayI );
                p->pPars->MapAreaF += p->InvAreaF;
                p->pPars->Edge++;
                p->pPars->Area++;
                p->nInvs++;
            }
            Nf_ObjUpdateRequired( p, Gia_ObjFaninId0(pObj, i), Gia_ObjFaninC0(pObj), Nf_ObjRequired(p, i, 0) );
            Nf_ObjMapRefInc( p, Gia_ObjFaninId0(pObj, i), Gia_ObjFaninC0(pObj));
            continue;
        }
        // skip if this node is not used
        for ( c = 0; c < 2; c++ )
            nRefs[c] = Nf_ObjMapRefNum(p, i, c);
        if ( !nRefs[0] && !nRefs[1] )
            continue;

        // consider two cases
        if ( nRefs[0] && nRefs[1] )
        {
            // find best matches for both phases
            for ( c = 0; c < 2; c++ )
            {
                Requireds[c] = Nf_ObjRequired( p, i, c );
                //assert( Requireds[c] < SCL_INFINITY );
                pDs[c] = Nf_ObjMatchD( p, i, c );
                pAs[c] = Nf_ObjMatchA( p, i, c );
                pMs[c] = (pAs[c]->D <= Requireds[c]) ? pAs[c] : pDs[c];
            }
            // swap complemented matches
            if ( pMs[0]->fCompl && pMs[1]->fCompl )
            {
//                pMs[0]->fCompl = pMs[1]->fCompl = 0;
//                ABC_SWAP( Nf_Mat_t *, pMs[0], pMs[1] );
                // find best matches for both phases
                pMs[0] = Nf_ObjMatchD( p, i, 0 );
                pMs[1] = Nf_ObjMatchD( p, i, 1 );
                assert( !pMs[0]->fCompl || !pMs[1]->fCompl );
            }
            // check if intervers are involved
            if ( !pMs[0]->fCompl && !pMs[1]->fCompl ) // no inverters
            {
                for ( c = 0; c < 2; c++ )
                    Nf_ManSetMapRefsGate( p, i, Requireds[c], pMs[c] );
            }
            else 
            {
                // one interver
                assert( !pMs[0]->fCompl || !pMs[1]->fCompl );
                c = pMs[1]->fCompl;
                assert( pMs[c]->fCompl && !pMs[!c]->fCompl );
                //printf( "Using inverter at node %d in phase %d\n", i, c );
                // update this phase
                pM = pMs[c];
                pM->fBest = 1;
                Required = Requireds[c];
                // update opposite phase
                Nf_ObjMapRefInc( p, i, !c );
                Nf_ObjUpdateRequired( p, i, !c, Required - p->InvDelayI );
                // select opposite phase
                Required = Nf_ObjRequired( p, i, !c );
                //assert( Required < SCL_INFINITY );
                pD = Nf_ObjMatchD( p, i, !c );
                pA = Nf_ObjMatchA( p, i, !c );
                pM = (pA->D <= Required) ? pA : pD;
                assert( !pM->fCompl );
                // create gate
                Nf_ManSetMapRefsGate( p, i, Required, pM );
                // account for the inverter
                p->pPars->MapAreaF += p->InvAreaF;
                p->pPars->Edge++;
                p->pPars->Area++;
                p->nInvs++;
            }
        }
        else
        {
            c = (int)(nRefs[1] > 0);
            assert( nRefs[c] && !nRefs[!c] );
            // consider this phase
            Required = Nf_ObjRequired( p, i, c );
            //assert( Required < SCL_INFINITY );
            pD = Nf_ObjMatchD( p, i, c );
            pA = Nf_ObjMatchA( p, i, c );
            pM = (pA->D <= Required) ? pA : pD;
            if ( pM->fCompl ) // use inverter
            {
                p->nInvs++;
                //printf( "Using inverter at node %d in phase %d\n", i, c );
                pM->fBest = 1;
                // update opposite phase
                Nf_ObjMapRefInc( p, i, !c );
                Nf_ObjUpdateRequired( p, i, !c, Required - p->InvDelayI );
                // select opposite phase
                Required = Nf_ObjRequired( p, i, !c );
                //assert( Required < SCL_INFINITY );
                pD = Nf_ObjMatchD( p, i, !c );
                pA = Nf_ObjMatchA( p, i, !c );
                pM = (pA->D <= Required) ? pA : pD;
                assert( !pM->fCompl );
                // account for the inverter
                p->pPars->MapAreaF += p->InvAreaF;
                p->pPars->Edge++;
                p->pPars->Area++;
            }
            // create gate
            Nf_ManSetMapRefsGate( p, i, Required, pM );
        }
        // the result of this:
        // - only one phase can be implemented as inverter of the other phase
        // - required times are propagated correctly
        // - references are set correctly
    }
    Gia_ManForEachCiId( p->pGia, Id, i )
        if ( Nf_ObjMapRefNum(p, Id, 1) )
        {
            Nf_ObjMapRefInc( p, Id, 0 );
            Nf_ObjUpdateRequired( p, Id, 0, Required - p->InvDelayI );
            p->pPars->MapAreaF += p->InvAreaF;
            p->pPars->Edge++;
            p->pPars->Area++;
            p->nInvs++;
        }
    // blend references
    for ( i = 0; i < nLits; i++ )
        pFlowRefs[i] = Abc_MaxFloat(1.0, Coef * pFlowRefs[i] + (1.0 - Coef) * Abc_MaxFloat(1, pMapRefs[i]));
//        pFlowRefs[i] = 0.2 * pFlowRefs[i] + 0.8 * Abc_MaxFloat(1, pMapRefs[i]);
    return p->pPars->Area;
}



/**Function*************************************************************

  Synopsis    [Area recovery.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Nf_MatchDeref_rec( Nf_Man_t * p, int i, int c, Nf_Mat_t * pM )
{
    word Area = 0;
    int k, iVar, fCompl, * pCut;
    assert( pM->fBest );
    if ( pM->fCompl )
    {
        assert( Nf_ObjMapRefNum(p, i, !c) > 0 );
        if ( !Nf_ObjMapRefDec(p, i, !c) )
            Area += Nf_MatchDeref_rec( p, i, !c, Nf_ObjMatchD(p, i, !c) );
        return Area + p->InvAreaW;
    }
    if ( Nf_ObjCutSetId(p, i) == 0 )
        return 0;
    pCut = Nf_CutFromHandle( Nf_ObjCutSet(p, i), pM->CutH );
    Nf_CutForEachVarCompl( pCut, pM->Cfg, iVar, fCompl, k )
    {
        assert( Nf_ObjMapRefNum(p, iVar, fCompl) > 0 );
        if ( !Nf_ObjMapRefDec(p, iVar, fCompl) )
            Area += Nf_MatchDeref_rec( p, iVar, fCompl, Nf_ObjMatchD(p, iVar, fCompl) );
    }
    return Area + Nf_ManCell(p, pM->Gate)->AreaW;
}
word Nf_MatchRef_rec( Nf_Man_t * p, int i, int c, Nf_Mat_t * pM, int Required, Vec_Int_t * vBackup )
{
    word Area = 0;
    int ReqFanin;
    int k, iVar, fCompl, * pCut;
    assert( pM->fBest );
    assert( pM->D <= Required );
    if ( pM->fCompl )
    {
        ReqFanin = Required - p->InvDelayI;
        if ( vBackup )
            Vec_IntPush( vBackup, Abc_Var2Lit(i, !c) );
        assert( Nf_ObjMapRefNum(p, i, !c) >= 0 );
        if ( !Nf_ObjMapRefInc(p, i, !c) )
            Area += Nf_MatchRef_rec( p, i, !c, Nf_ObjMatchD(p, i, !c), ReqFanin, vBackup );
        return Area + p->InvAreaW;
    }
    if ( Nf_ObjCutSetId(p, i) == 0 )
        return 0;
    pCut = Nf_CutFromHandle( Nf_ObjCutSet(p, i), pM->CutH );
    Nf_CutForEachVarCompl( pCut, pM->Cfg, iVar, fCompl, k )
    {
        ReqFanin = Required - Nf_ManCell(p, pM->Gate)->iDelays[k];
        if ( vBackup )
            Vec_IntPush( vBackup, Abc_Var2Lit(iVar, fCompl) );
        assert( Nf_ObjMapRefNum(p, iVar, fCompl) >= 0 );
        if ( !Nf_ObjMapRefInc(p, iVar, fCompl) )
            Area += Nf_MatchRef_rec( p, iVar, fCompl, Nf_ObjMatchD(p, iVar, fCompl), ReqFanin, vBackup );
    }
    return Area + Nf_ManCell(p, pM->Gate)->AreaW;
}
word Nf_MatchRefArea( Nf_Man_t * p, int i, int c, Nf_Mat_t * pM, int Required )
{
    word Area;  int iLit, k; 
    Vec_IntClear( &p->vBackup );
    Area = Nf_MatchRef_rec( p, i, c, pM, Required, &p->vBackup );
    Vec_IntForEachEntry( &p->vBackup, iLit, k )
    {
        assert( Nf_ObjMapRefNum(p, Abc_Lit2Var(iLit), Abc_LitIsCompl(iLit)) > 0 );
        Nf_ObjMapRefDec( p, Abc_Lit2Var(iLit), Abc_LitIsCompl(iLit) );
    }
    return Area;
}
void Nf_ManElaBestMatchOne( Nf_Man_t * p, int iObj, int c, int * pCut, int * pCutSet, Nf_Mat_t * pRes, int Required )
{
    Nf_Mat_t Mb,*pMb = &Mb, * pMd;
    int * pFans      = Nf_CutLeaves(pCut);
    int nFans        = Nf_CutSize(pCut);
    int iFuncLit     = Nf_CutFunc(pCut);
    int fComplExt    = Abc_LitIsCompl(iFuncLit);
    Vec_Int_t * vArr = Vec_WecEntry( p->vTt2Match, Abc_Lit2Var(iFuncLit) );
    int i, k, Info, Offset, iFanin, fComplF;
    // assign fanins matches
    Nf_Obj_t * pBestF[NF_LEAF_MAX];
    for ( i = 0; i < nFans; i++ )
        pBestF[i] = Nf_ManObj( p, pFans[i] );
    // consider matches of this function
    memset( pMb, 0, sizeof(Nf_Mat_t) );
    pMb->D = SCL_INFINITY; pMb->F = FLT_MAX;
    // special cases
    if ( nFans == 0 )
    {
        int Const = (iFuncLit == 1);
        //printf( "Node %d(%d) is const\n", iObj, c );
        assert( iFuncLit == 0 || iFuncLit == 1 );
        pMb->D = 0;
        pMb->F = p->pCells[c ^ Const].AreaF;
        pMb->CutH = Nf_CutHandle(pCutSet, pCut);
        pMb->Gate = c ^ Const;
//        pMb->Conf = 0;
        pMb->Cfg = Nf_Int2Cfg(0);
        pMb->fBest = 1;
        // compare
        if ( pRes->F > pMb->F + NF_EPSILON || (pRes->F > pMb->F - NF_EPSILON && pRes->D > pMb->D) )
            *pRes = *pMb;
        return;
    }
    // consider matches of this function
    Vec_IntForEachEntryDouble( vArr, Info, Offset, i )
    {
        Nf_Cfg_t Cfg   = Nf_Int2Cfg(Offset);
        Mio_Cell2_t*pC = Nf_ManCell( p, Info );
        int fCompl     = Cfg.fCompl ^ fComplExt;
        int Delay      = 0;
        assert( nFans == (int)pC->nFanins );
        if ( fCompl != c )
            continue;
        Nf_CfgForEachVarCompl( Cfg, nFans, iFanin, fComplF, k )
        {
            pMd     = &pBestF[iFanin]->M[fComplF][0];
            assert( pMd->fBest );
            Delay = Abc_MaxInt( Delay, pMd->D + pC->iDelays[k] );
            if ( Delay > Required )
                break;
        }
        if ( k < nFans )
            continue;
        // create match
        pMb->D = Delay;
        pMb->F = FLT_MAX;
        pMb->fBest = 1;
        pMb->fCompl = 0;
        pMb->CutH = Nf_CutHandle(pCutSet, pCut);
        pMb->Gate = pC->Id;
        pMb->Cfg = Cfg;
        pMb->Cfg.fCompl = 0;
        // compute area
        pMb->F = Scl_Int2Flt((int)Nf_MatchRefArea(p, iObj, c, pMb, Required));
        // compare
        if ( pRes->F > pMb->F + NF_EPSILON || (pRes->F > pMb->F - NF_EPSILON && pRes->D > pMb->D) )
            *pRes = *pMb;
    }
}
void Nf_ManElaBestMatch( Nf_Man_t * p, int iObj, int c, Nf_Mat_t * pRes, int Required )
{
    int k, * pCut, * pCutSet = Nf_ObjCutSet( p, iObj );
    memset( pRes, 0, sizeof(Nf_Mat_t) );
    pRes->D = SCL_INFINITY; pRes->F = FLT_MAX;
    Nf_SetForEachCut( pCutSet, pCut, k )
    {
        if ( Abc_Lit2Var(Nf_CutFunc(pCut)) >= Vec_WecSize(p->vTt2Match) )
            continue;
        Nf_ManElaBestMatchOne( p, iObj, c, pCut, pCutSet, pRes, Required );
    }
}
int Nf_ManComputeArrival( Nf_Man_t * p, Nf_Mat_t * pM, int * pCutSet )
{
    int Delay = 0; 
    Nf_Mat_t * pMfan;
    int iVar, fCompl, k;
    Mio_Cell2_t * pCell = Nf_ManCell( p, pM->Gate );
    int * pCut = Nf_CutFromHandle( pCutSet, pM->CutH );
    assert( !pM->fCompl );
    Nf_CutForEachVarCompl( pCut, pM->Cfg, iVar, fCompl, k )
    {
        pMfan = Nf_ObjMatchBest( p, iVar, fCompl );
        Delay = Abc_MaxInt( Delay, pMfan->D + pCell->iDelays[k] );
    }
    //if ( pM->fCompl ) Delay += p->InvDelayI;
    return Delay;
}
void Nf_ManResetMatches( Nf_Man_t * p, int Round )
{
    Gia_Obj_t * pObj;
    Nf_Mat_t * pDc, * pAc, * pMfan, * pM[2]; 
    int i, c, Arrival;
    // go through matches in the topo order
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
        if ( Gia_ObjIsBuf(pObj) )
        {
            pMfan = Nf_ObjMatchBest( p, Gia_ObjFaninId0(pObj, i), Gia_ObjFaninC0(pObj) );
            for ( c = 0; c < 2; c++ )
            {
                pDc = Nf_ObjMatchD( p, i, c );
                pAc = Nf_ObjMatchA( p, i, c );
                pDc->F = pAc->F = 0;
                pDc->D = pMfan->D + (c ? p->InvDelayI : 0); 
                assert( pDc->fBest );
                assert( !pAc->fBest );
                assert( c==0 || pDc->fCompl );
            }
            continue;
        }
        // select the best match for each phase
        for ( c = 0; c < 2; c++ )
        {
            pDc = Nf_ObjMatchD( p, i, c );
            pAc = Nf_ObjMatchA( p, i, c );
            pDc->F = pAc->F = 0;
            if ( Nf_ObjMapRefNum(p, i, c) )
            {
                assert( pDc->fBest != pAc->fBest );
                if ( pAc->fBest )
                    ABC_SWAP( Nf_Mat_t, *pDc, *pAc );
                assert( pDc->fBest );
                assert( !pAc->fBest );
            }
            else
            {
                assert( Round > 0 || (!pDc->fBest && !pAc->fBest) );
//                if ( (p->pPars->fAreaOnly || (Round & 1)) && !pAc->fCompl )
                if ( (Round & 1) && !pAc->fCompl )
                    ABC_SWAP( Nf_Mat_t, *pDc, *pAc );
                pDc->fBest = 1;
                pAc->fBest = 0;
            }
        }
        // consider best matches of both phases
        pM[0] = Nf_ObjMatchD( p, i, 0 );
        pM[1] = Nf_ObjMatchD( p, i, 1 );
        assert( pM[0]->fBest && pM[1]->fBest );
        // swap complemented matches
        if ( pM[0]->fCompl && pM[1]->fCompl )
        {
//            pM[0]->fCompl = pM[1]->fCompl = 0;
//            ABC_SWAP( Nf_Mat_t *, pM[0], pM[1] );
            assert( 0 );
        }
        if ( !pM[0]->fCompl && !pM[1]->fCompl )
        {
            for ( c = 0; c < 2; c++ )
            {
                Arrival = Nf_ManComputeArrival( p, pM[c], Nf_ObjCutSet(p, i) );
                //if ( Nf_ObjMapRefNum(p, i, c) )
                //    assert( Round || Arrival <= pM[c]->D );
                pM[c]->D = Arrival;
            }
        }
        else 
        {
            // consider non-complemented match
            c = !pM[1]->fCompl;
            assert( !pM[c]->fCompl );
            assert( pM[!c]->fCompl );
            Arrival = Nf_ManComputeArrival( p, pM[c], Nf_ObjCutSet(p, i) );
            //if ( Nf_ObjMapRefNum(p, i, c) )
            //    assert( Round || Arrival <= pM[c]->D );
            pM[c]->D = Arrival;
            // consider complemented match
            Arrival = pM[!c]->D;
            *pM[!c] = *pM[c];
            pM[!c]->D += p->InvDelayI;
            pM[!c]->fCompl = 1;
            //if ( Nf_ObjMapRefNum(p, i, !c) )
            //    assert( Round || pM[!c]->D <= Arrival );
        }
    }
}
void Nf_ManComputeMappingEla( Nf_Man_t * p )
{
    int fVerbose = 0;
    Gia_Obj_t * pObj;
    Mio_Cell2_t * pCell;
    Nf_Mat_t Mb, * pMb = &Mb, * pM;
    word AreaBef, AreaAft, Gain = 0;
    int i, c, iVar, Id, fCompl, k, * pCut;
    int Required;
    Nf_ManSetOutputRequireds( p, 1 );
    Nf_ManResetMatches( p, p->Iter - p->pPars->nRounds );
    Gia_ManForEachAndReverse( p->pGia, pObj, i )
    {
        if ( Gia_ObjIsBuf(pObj) )
        {
            if ( Nf_ObjMapRefNum(p, i, 1) )
                Nf_ObjUpdateRequired( p, i, 0, Nf_ObjRequired(p, i, 1) - p->InvDelayI );
            Nf_ObjUpdateRequired( p, Gia_ObjFaninId0(pObj, i), Gia_ObjFaninC0(pObj), Nf_ObjRequired(p, i, 0) );
            continue;
        }
        for ( c = 0; c < 2; c++ )
        if ( Nf_ObjMapRefNum(p, i, c) )
        {
            pM = Nf_ObjMatchBest( p, i, c );
            Required = Nf_ObjRequired( p, i, c );
            assert( pM->D <= Required );
            if ( pM->fCompl )
                continue;
            // search for a better match
            assert( !pM->fCompl );
            AreaBef = Nf_MatchDeref_rec( p, i, c, pM );
            assert( pM->fBest );
            Nf_ManElaBestMatch( p, i, c, pMb, Required );
            AreaAft = Nf_MatchRef_rec( p, i, c, pMb, Required, NULL );
            Gain += AreaBef - AreaAft;
            // print area recover progress
            if ( fVerbose && Nf_ManCell(p, pM->Gate)->pName != Nf_ManCell(p, pMb->Gate)->pName )
            {
                printf( "%4d (%d)  ", i, c );
                printf( "%8s ->%8s  ",         Nf_ManCell(p, pM->Gate)->pName, Nf_ManCell(p, pMb->Gate)->pName );
                printf( "%d -> %d  ",          Nf_ManCell(p, pM->Gate)->nFanins, Nf_ManCell(p, pMb->Gate)->nFanins );
                printf( "D: %7.2f -> %7.2f  ", Scl_Int2Flt(pM->D), Scl_Int2Flt(pMb->D) );
                printf( "R: %7.2f  ",          Required == SCL_INFINITY ? 9999.99 : Scl_Int2Flt(Required) );
                printf( "A: %7.2f -> %7.2f  ", Scl_Int2Flt((int)AreaBef), Scl_Int2Flt((int)AreaAft) );
                printf( "G: %7.2f (%7.2f) ",   Scl_Int2Flt((int)AreaBef - (int)AreaAft), Scl_Int2Flt((int)Gain) );
                printf( "\n" );
            }
            // set best match
            assert( pMb->fBest );
            assert( pMb->D <= Required );
            //assert( Scl_Flt2Int(pMb->F) == (int)AreaAft );
            //assert( AreaBef >= AreaAft );
            *pM = *pMb;
            // update timing
            pCell = Nf_ManCell( p, pMb->Gate );
            pCut = Nf_CutFromHandle( Nf_ObjCutSet(p, i), pMb->CutH );
            Nf_CutForEachVarCompl( pCut, pMb->Cfg, iVar, fCompl, k )
            {
                pM = Nf_ObjMatchBest( p, iVar, fCompl );
                assert( pM->D <= Required - pCell->iDelays[k] );
                Nf_ObjUpdateRequired( p, iVar, fCompl, Required - pCell->iDelays[k] );
                if ( pM->fCompl )
                {
                    pM = Nf_ObjMatchBest( p, iVar, !fCompl );
                    assert( pM->D <= Required - pCell->iDelays[k] - p->InvDelayI );
                    Nf_ObjUpdateRequired( p, iVar, !fCompl, Required - pCell->iDelays[k] - p->InvDelayI );
                }
            }
        }
    }
    Gia_ManForEachCiId( p->pGia, Id, i )
        if ( Nf_ObjMapRefNum(p, Id, 1) )
        {
            Required = Nf_ObjRequired( p, i, 1 );
            Nf_ObjUpdateRequired( p, Id, 0, Required - p->InvDelayI );
        }
}
void Nf_ManFixPoDrivers( Nf_Man_t * p )
{
    Gia_Obj_t * pObj;
    Nf_Mat_t * pM, * pMc;
    int i, iDriver, Count = 0;
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        iDriver = Gia_ObjFaninId0p(p->pGia, pObj);
        if ( !Gia_ObjIsAnd(Gia_ManObj(p->pGia, iDriver)) )
            continue;
        // skip unless both are used
        if ( !Nf_ObjMapRefNum(p, iDriver, 0) || !Nf_ObjMapRefNum(p, iDriver, 1) )
            continue;
        pM  = Nf_ObjMatchD( p, iDriver,  Gia_ObjFaninC0(pObj) );
        pMc = Nf_ObjMatchD( p, iDriver, !Gia_ObjFaninC0(pObj) );
        // skip unless both are non-complemented
        if ( pM->fCompl || pMc->fCompl )
            continue;
        // skip if arrival time exceeds the required time
        if ( pMc->D + p->InvDelayI > p->pPars->MapDelay )
            continue;
        // update references
        Nf_MatchDeref_rec( p, iDriver, Gia_ObjFaninC0(pObj), pM );
        Nf_ObjMapRefInc( p, iDriver, !Gia_ObjFaninC0(pObj) );
        // add inverter
        *pM = *pMc;
        pM->D += p->InvDelayI;
        pM->fCompl = 1;
        pM->fBest = 1;
        pMc->fBest = 1;
        Count++;
    }
    //printf( "Fixed %d PO drivers.\n", Count );
}

/**Function*************************************************************

  Synopsis    [Deriving mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Nf_ManDeriveMapping( Nf_Man_t * p )
{
    Vec_Int_t * vMapping;
    Nf_Mat_t * pM;
    int i, k, c, Id, iVar, fCompl, * pCut;
    assert( p->pGia->vCellMapping == NULL );
    vMapping = Vec_IntAlloc( 2*Gia_ManObjNum(p->pGia) + (int)p->pPars->Edge + (int)p->pPars->Area * 2 );
    Vec_IntFill( vMapping, 2*Gia_ManObjNum(p->pGia), 0 );
    // create CI inverters
    Gia_ManForEachCiId( p->pGia, Id, i )
    if ( Nf_ObjMapRefNum(p, Id, 1) )
        Vec_IntWriteEntry( vMapping, Abc_Var2Lit(Id, 1), -1 );
    // create internal nodes
    Gia_ManForEachAndId( p->pGia, i )
    {
        Gia_Obj_t * pObj = Gia_ManObj(p->pGia, i);
        if ( Gia_ObjIsBuf(pObj) )
        {
            if ( Nf_ObjMapRefNum(p, i, 1) )
                Vec_IntWriteEntry( vMapping, Abc_Var2Lit(i, 1), -1 );
            Vec_IntWriteEntry( vMapping, Abc_Var2Lit(i, 0), -2 );
            continue;
        }
        for ( c = 0; c < 2; c++ )
        if ( Nf_ObjMapRefNum(p, i, c) )
        {
            pM = Nf_ObjMatchBest( p, i, c );
            // remember inverter
            if ( pM->fCompl )
            {
                Vec_IntWriteEntry( vMapping, Abc_Var2Lit(i, c), -1 );
                continue;
            }
    //        Nf_ManCutMatchPrint( p, i, c, pM );
            pCut = Nf_CutFromHandle( Nf_ObjCutSet(p, i), pM->CutH );
            Vec_IntWriteEntry( vMapping, Abc_Var2Lit(i, c), Vec_IntSize(vMapping) );
            Vec_IntPush( vMapping, Nf_CutSize(pCut) );
            Nf_CutForEachVarCompl( pCut, pM->Cfg, iVar, fCompl, k )
                Vec_IntPush( vMapping, Abc_Var2Lit(iVar, fCompl) );
            Vec_IntPush( vMapping, pM->Gate );
        }
    }
//    assert( Vec_IntCap(vMapping) == 16 || Vec_IntSize(vMapping) == Vec_IntCap(vMapping) );
    p->pGia->vCellMapping = vMapping;
    return p->pGia;
}
void Nf_ManUpdateStats( Nf_Man_t * p )
{
    Nf_Mat_t * pM;
    Gia_Obj_t * pObj;
    Mio_Cell2_t * pCell;
    int i, c, Id, * pCut;
    p->pPars->MapAreaF = 0; p->nInvs = 0;
    p->pPars->Area = p->pPars->Edge = 0;
    Gia_ManForEachAndReverse( p->pGia, pObj, i )
    {
        if ( Gia_ObjIsBuf(pObj) )
        {
            if ( Nf_ObjMapRefNum(p, i, 1) )
            {
                p->pPars->MapAreaF += p->InvAreaF;
                p->pPars->Edge++;
                p->pPars->Area++;
                p->nInvs++;
            }
            continue;
        }
        for ( c = 0; c < 2; c++ )
        if ( Nf_ObjMapRefNum(p, i, c) )
        {
            pM = Nf_ObjMatchBest( p, i, c );
            if ( pM->fCompl )
            {
                p->pPars->MapAreaF += p->InvAreaF;
                p->pPars->Edge++;
                p->pPars->Area++;
                p->nInvs++;
                continue;
            }
            pCut = Nf_CutFromHandle( Nf_ObjCutSet(p, i), pM->CutH );
            pCell = Nf_ManCell( p, pM->Gate );
            assert( Nf_CutSize(pCut) == (int)pCell->nFanins );
            p->pPars->MapAreaF += pCell->AreaF;
            p->pPars->Edge += Nf_CutSize(pCut);
            p->pPars->Area++;
            //printf( "%5d (%d) : Gate = %7s \n", i, c, pCell->pName );
        }
    }
    Gia_ManForEachCiId( p->pGia, Id, i )
        if ( Nf_ObjMapRefNum(p, Id, 1) )
        {
            p->pPars->MapAreaF += p->InvAreaF;
            p->pPars->Edge++;
            p->pPars->Area++;
            p->nInvs++;
        }
}

/**Function*************************************************************

  Synopsis    [Extract window.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

/*
    int              nInputs;   // the number of inputs
    int              nObjs;     // number of all objects
    Vec_Int_t *      vRoots;    // output drivers to be mapped (root -> obj lit)
    Vec_Wec_t *      vCuts;     // cuts (cut -> obj lit + fanin lits)
    Vec_Wec_t *      vObjCuts;  // cuts (obj lit -> obj lit + cut lits)
    Vec_Int_t *      vSolCuts;  // current solution (index -> cut)
    Vec_Int_t *      vCutGates; // gates (cut -> gate)
*/

int Nf_ManExtractWindow( void * pMan, Vec_Int_t * vRoots, Vec_Wec_t * vCuts, Vec_Wec_t * vObjCuts, Vec_Int_t * vSolCuts, Vec_Int_t * vCutGates, Vec_Wrd_t * vCutAreas, word * pInvArea, int StartVar, int nVars )
{
    Nf_Man_t * p = (Nf_Man_t *)pMan;
    int nInputs = Gia_ManCiNum(p->pGia);
    int LitShift = 2*nInputs+2;
    Gia_Obj_t * pObj;
    int c, iObj;
    if ( 2*Gia_ManAndNum(p->pGia) + Gia_ManCiNum(p->pGia) > nVars )
    {
        printf( "The number of variables is too large: 2*%d + %d = %d > %d.\n", Gia_ManAndNum(p->pGia), Gia_ManCiNum(p->pGia), 2*Gia_ManAndNum(p->pGia) + Gia_ManCiNum(p->pGia), nVars );
        return 0;
    }
    *pInvArea = p->InvAreaW;
    // save roots
    Vec_IntClear( vRoots );
    Gia_ManForEachCo( p->pGia, pObj, c )
    {
        assert( !Gia_ObjIsCi(Gia_ObjFanin0(pObj)) );
        Vec_IntPush( vRoots, Gia_ObjFaninLit0p(p->pGia, pObj)-LitShift );
    }
    // prepare
    Vec_WecClear( vCuts );
    Vec_WecClear( vObjCuts );
    Vec_IntClear( vSolCuts );
    Vec_IntClear( vCutGates );
    Vec_WrdClear( vCutAreas );
    // collect cuts for each node
    Gia_ManForEachAndId( p->pGia, iObj )
    {
        Vec_Int_t * vObj[2], * vCutOne;
        int iCut, * pCut, * pCutSet;
        int iCutInv[2] = {-1, -1};
        // get matches
        Nf_Mat_t * pM[2] = {NULL, NULL};
        for ( c = 0; c < 2; c++ )
        {
            if ( Nf_ObjMapRefNum(p, iObj, c) == 0 )
                continue;
            if ( Nf_ObjMatchBest(p, iObj, c)->fCompl )
            {
                assert( iCutInv[c] == -1 );
                iCutInv[c] = Vec_IntSize(vSolCuts);
                Vec_IntPush( vSolCuts, -1 );
                continue;
            }
            pM[c] = Nf_ObjMatchBest(p, iObj, c);
        }
        // start collecting cuts of pos-obj and neg-obj
        assert( Vec_WecSize(vObjCuts) == 2*iObj-LitShift );
        for ( c = 0; c < 2; c++ )
        {
            vObj[c] = Vec_WecPushLevel( vObjCuts );
            Vec_IntPush( vObj[c], Abc_Var2Lit(Abc_Var2Lit(iObj, c)-LitShift, 1) );
        }
        // enumerate cuts
        pCutSet = Nf_ObjCutSet( p, iObj );
        Nf_SetForEachCut( pCutSet, pCut, iCut )
        {
            assert( !Nf_CutIsTriv(pCut, iObj) );
            assert( Nf_CutSize(pCut) <= p->pPars->nLutSize );
            if ( Abc_Lit2Var(Nf_CutFunc(pCut)) < Vec_WecSize(p->vTt2Match) )
            {
                int * pFans      = Nf_CutLeaves(pCut);
                int nFans        = Nf_CutSize(pCut);
                int iFuncLit     = Nf_CutFunc(pCut);
                int fComplExt    = Abc_LitIsCompl(iFuncLit);
                Vec_Int_t * vArr = Vec_WecEntry( p->vTt2Match, Abc_Lit2Var(iFuncLit) );
                int i, k, c, Info, Offset, iFanin, fComplF, iCutLit;
                Vec_IntForEachEntryDouble( vArr, Info, Offset, i )
                {
                    Nf_Cfg_t Cfg   = Nf_Int2Cfg(Offset);
                    int fCompl     = Cfg.fCompl ^ fComplExt;
                    Mio_Cell2_t*pC = Nf_ManCell( p, Info );
                    assert( nFans == (int)pC->nFanins );
                    Vec_IntPush( vCutGates, Info );
                    Vec_WrdPush( vCutAreas, pC->AreaW );
                    // to make comparison possible
                    Cfg.fCompl = 0;
                    // add solution cut
                    for ( c = 0; c < 2; c++ )
                    {
                        if ( pM[c] == NULL )
                            continue;
                        if ( (int)pM[c]->CutH == Nf_CutHandle(pCutSet, pCut) && (int)pM[c]->Gate == Info && Nf_Cfg2Int(pM[c]->Cfg) == Nf_Cfg2Int(Cfg) )
                        {
                            Vec_IntPush( vSolCuts, Vec_WecSize(vCuts) );
                            //printf( "adding solution for %d\n", Abc_Var2Lit(iObj, c)-LitShift );
                        }
                    }
                    // add new cut
                    iCutLit = Abc_Var2Lit( StartVar + Vec_WecSize(vCuts), 0 );
                    vCutOne = Vec_WecPushLevel( vCuts );
                    // add literals
                    Vec_IntPush( vCutOne, Abc_Var2Lit(iObj, fCompl) );
                    Vec_IntPush( vObj[fCompl], iCutLit );
                    Nf_CfgForEachVarCompl( Cfg, nFans, iFanin, fComplF, k )
                        if ( pFans[iFanin] >= nInputs + 1 ) // internal node
                        {
                            Vec_IntPush( vCutOne, Abc_Var2Lit(pFans[iFanin], fComplF) );
                            //Vec_IntPush( Vec_WecEntry(vObjCuts, Abc_Var2Lit(pFans[iFanin], fComplF)-LitShift), iCutLit );
                        }
                        else if ( fComplF ) // complemented primary input
                            Vec_IntPush( vCutOne, Abc_Var2Lit(pFans[iFanin], 1) );
                } 
            }
        }
        assert( iCutInv[0] == -1 || iCutInv[1] == -1 );
        // add inverter cut
        for ( c = 0; c < 2; c++ )
        {
            if ( iCutInv[c] != -1 )
                Vec_IntWriteEntry( vSolCuts, iCutInv[c], Vec_WecSize(vCuts) );
            // the obj-lit implies its cut
            Vec_IntPush( Vec_WecEntry(vObjCuts, Abc_Var2Lit(iObj, c)-LitShift), Abc_Var2Lit(StartVar + Vec_WecSize(vCuts), 0) );
            // the cut includes both literals
            vCutOne = Vec_WecPushLevel( vCuts );
            Vec_IntPush( vCutOne, Abc_Var2Lit(iObj, c) );
            Vec_IntPush( vCutOne, Abc_Var2Lit(iObj, !c) );
            Vec_IntPush( vCutGates, 3 );
            Vec_WrdPush( vCutAreas, p->InvAreaW );
        }
    }
//    for ( c = 0; c < p->nCells; c++ )
//        printf( "%d=%s ", c, p->pCells[c].pName );
//    printf( "\n" );
    // add complemented inputs
    Gia_ManForEachCiId( p->pGia, iObj, c )
        if ( Nf_ObjMapRefNum(p, iObj, 1) )
            Vec_IntPush( vSolCuts, -(2*Gia_ManAndNum(p->pGia)+c) );
    assert( Vec_WecSize(vCuts) == Vec_IntSize(vCutGates) );
    assert( Vec_WecSize(vCuts) == Vec_WrdSize(vCutAreas) );
    assert( Vec_WecSize(vObjCuts) == 2*Gia_ManAndNum(p->pGia) );
    return nInputs;
}

/**Function*************************************************************

  Synopsis    [Technology mappping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nf_ManSetDefaultPars( Jf_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Jf_Par_t) );
    pPars->nLutSize     =  6;
    pPars->nCutNum      = 16;
    pPars->nProcNum     =  0;
    pPars->nRounds      =  4;
    pPars->nRoundsEla   =  2;
    pPars->nRelaxRatio  =  0;
    pPars->nCoarseLimit =  3;
    pPars->nAreaTuner   =  0;
    pPars->nReqTimeFlex =  0;
    pPars->nVerbLimit   =  5;
    pPars->DelayTarget  = -1;
    pPars->fAreaOnly    =  0;
    pPars->fPinPerm     =  0;
    pPars->fPinQuick    =  0;
    pPars->fPinFilter   =  0;
    pPars->fOptEdge     =  1; 
    pPars->fCoarsen     =  0;
    pPars->fCutMin      =  1;
    pPars->fGenCnf      =  0;
    pPars->fPureAig     =  0;
    pPars->fVerbose     =  0;
    pPars->fVeryVerbose =  0;
    pPars->nLutSizeMax  =  NF_LEAF_MAX;
    pPars->nCutNumMax   =  NF_CUT_MAX;
    pPars->MapDelayTarget = 0;
}
Gia_Man_t * Nf_ManPerformMapping( Gia_Man_t * pGia, Jf_Par_t * pPars )
{
    Gia_Man_t * pNew = NULL, * pCls;
    Nf_Man_t * p; int i, Id;
    if ( Gia_ManHasChoices(pGia) )
        pPars->fCoarsen = 0; 
    pCls = pPars->fCoarsen ? Gia_ManDupMuxes(pGia, pPars->nCoarseLimit) : pGia;
    p = Nf_StoCreate( pCls, pPars );
    if ( p == NULL )
        return NULL;
//    if ( pPars->fVeryVerbose )
//        Nf_StoPrint( p, pPars->fVeryVerbose );
    if ( pPars->fVerbose && pPars->fCoarsen )
    {
        printf( "Initial " );  Gia_ManPrintMuxStats( pGia );  printf( "\n" );
        printf( "Derived " );  Gia_ManPrintMuxStats( pCls );  printf( "\n" );
    }
    Nf_ManPrintInit( p );
    Nf_ManComputeCuts( p );
    Nf_ManPrintQuit( p );
    if ( Scl_ConIsRunning() )
    {
        Gia_ManForEachCiId( p->pGia, Id, i )
            Nf_ObjPrepareCi( p, Id, Scl_ConGetInArr(i) );
    }
    else
    {
        Gia_ManForEachCiId( p->pGia, Id, i )
//            Nf_ObjPrepareCi( p, Id, Scl_Flt2Int(p->pGia->vInArrs ? Abc_MaxFloat(0.0, Vec_FltEntry(p->pGia->vInArrs, i)) : 0.0) );
            Nf_ObjPrepareCi( p, Id, Scl_Flt2Int(p->pGia->vInArrs ? Vec_FltEntry(p->pGia->vInArrs, i) : 0.0) );
    }
    for ( p->Iter = 0; p->Iter < p->pPars->nRounds; p->Iter++ )
    {
        Nf_ManComputeMapping( p );
        Nf_ManSetMapRefs( p );
        Nf_ManPrintStats( p, (char *)(p->Iter ? "Area " : "Delay") );
    }

    p->fUseEla = 1;
    for ( ; p->Iter < p->pPars->nRounds + pPars->nRoundsEla; p->Iter++ )
    {
        Nf_ManComputeMappingEla( p );
        Nf_ManUpdateStats( p );
        Nf_ManPrintStats( p, "Ela  " );
    }
    Nf_ManFixPoDrivers( p );
    pNew = Nf_ManDeriveMapping( p );
    if ( pPars->fAreaOnly )
    {
        int Sbm_ManTestSat( void * pMan );
        Sbm_ManTestSat( p );
    }
    Nf_StoDelete( p );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

