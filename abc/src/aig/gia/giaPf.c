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

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define PF_LEAF_MAX  6
#define PF_CUT_MAX  32
#define PF_NO_LEAF  31
#define PF_NO_FUNC  0x3FFFFFF
#define PF_INFINITY FLT_MAX

typedef struct Pf_Cut_t_ Pf_Cut_t; 
struct Pf_Cut_t_
{
    word            Sign;           // signature
    int             Delay;          // delay
    float           Flow;           // flow
    unsigned        iFunc   : 26;   // function (PF_NO_FUNC)
    unsigned        Useless :  1;   // function
    unsigned        nLeaves :  5;   // leaf number (PF_NO_LEAF)
    int             pLeaves[PF_LEAF_MAX+1]; // leaves
};
typedef struct Pf_Mat_t_ Pf_Mat_t; 
struct Pf_Mat_t_
{
    unsigned        fCompl  :  8;   // complemented
    unsigned        Phase   :  6;   // match phase
    unsigned        Perm    : 18;   // match permutation
};
typedef struct Pf_Obj_t_ Pf_Obj_t; 
struct Pf_Obj_t_
{
    float           Area;
    unsigned        Gate    :  7;   // gate
    unsigned        nLeaves :  3;   // fanin count  
    unsigned        nRefs   : 22;   // ref count
    int             pLeaves[6];     // leaf literals
};
typedef struct Pf_Man_t_ Pf_Man_t; 
struct Pf_Man_t_
{
    // user data
    Gia_Man_t *     pGia;           // derived manager
    Jf_Par_t *      pPars;          // parameters
    // matching
    Vec_Mem_t *     vTtMem;         // truth tables
    Vec_Wec_t *     vTt2Match;      // matches for truth tables
    Mio_Cell_t *    pCells;         // library gates
    int             nCells;         // library gate count
    // cut data
    Pf_Obj_t *      pPfObjs;        // best cuts
    Vec_Ptr_t       vPages;         // cut memory
    Vec_Int_t       vCutSets;       // cut offsets
    Vec_Flt_t       vCutFlows;      // temporary cut area
    Vec_Int_t       vCutDelays;     // temporary cut delay
    int             iCur;           // current position
    int             Iter;           // mapping iterations
    int             fUseEla;        // use exact area
    int             nInvs;          // the inverter count
    float           InvDelay;       // inverter delay
    float           InvArea;        // inverter area 
    // statistics
    abctime         clkStart;       // starting time
    double          CutCount[6];    // cut counts
    int             nCutUseAll;     // objects with useful cuts
};

static inline int         Pf_Mat2Int( Pf_Mat_t Mat )                                { union { int x; Pf_Mat_t y; } v; v.y = Mat; return v.x;           }
static inline Pf_Mat_t    Pf_Int2Mat( int Int )                                     { union { int x; Pf_Mat_t y; } v; v.x = Int; return v.y;           }

static inline Pf_Obj_t *  Pf_ManObj( Pf_Man_t * p, int i )                          { return p->pPfObjs + i;                                           }
static inline Mio_Cell_t* Pf_ManCell( Pf_Man_t * p, int i )                         { return p->pCells + i;                                            }
static inline int *       Pf_ManCutSet( Pf_Man_t * p, int i )                       { return (int *)Vec_PtrEntry(&p->vPages, i >> 16) + (i & 0xFFFF);  }
static inline int         Pf_ObjCutSetId( Pf_Man_t * p, int i )                     { return Vec_IntEntry( &p->vCutSets, i );                          }
static inline int *       Pf_ObjCutSet( Pf_Man_t * p, int i )                       { return Pf_ManCutSet(p, Pf_ObjCutSetId(p, i));                    }
static inline int         Pf_ObjHasCuts( Pf_Man_t * p, int i )                      { return (int)(Vec_IntEntry(&p->vCutSets, i) > 0);                 }
static inline int         Pf_ObjCutUseless( Pf_Man_t * p, int TruthId )             { return (int)(TruthId >= Vec_WecSize(p->vTt2Match));              } 

static inline float       Pf_ObjCutFlow( Pf_Man_t * p, int i )                      { return Vec_FltEntry(&p->vCutFlows, i);                           } 
static inline int         Pf_ObjCutDelay( Pf_Man_t * p, int i )                     { return Vec_IntEntry(&p->vCutDelays, i);                          } 
static inline void        Pf_ObjSetCutFlow( Pf_Man_t * p, int i, float a )          { Vec_FltWriteEntry(&p->vCutFlows, i, a);                          } 
static inline void        Pf_ObjSetCutDelay( Pf_Man_t * p, int i, int d )           { Vec_IntWriteEntry(&p->vCutDelays, i, d);                         } 

static inline int         Pf_CutSize( int * pCut )                                  { return pCut[0] & PF_NO_LEAF;                                     }
static inline int         Pf_CutFunc( int * pCut )                                  { return ((unsigned)pCut[0] >> 5);                                 }
static inline int *       Pf_CutLeaves( int * pCut )                                { return pCut + 1;                                                 }
static inline int         Pf_CutSetBoth( int n, int f )                             { return n | (f << 5);                                             }
static inline int         Pf_CutIsTriv( int * pCut, int i )                         { return Pf_CutSize(pCut) == 1 && pCut[1] == i;                    } 
static inline int         Pf_CutHandle( int * pCutSet, int * pCut )                 { assert( pCut > pCutSet ); return pCut - pCutSet;                 } 
static inline int *       Pf_CutFromHandle( int * pCutSet, int h )                  { assert( h > 0 ); return pCutSet + h;                             }
static inline int         Pf_CutConfLit( int Conf, int i )                          { return 15 & (Conf >> (i << 2));                                  }
static inline int         Pf_CutConfVar( int Conf, int i )                          { return Abc_Lit2Var( Pf_CutConfLit(Conf, i) );                    }
static inline int         Pf_CutConfC( int Conf, int i )                            { return Abc_LitIsCompl( Pf_CutConfLit(Conf, i) );                 }

#define Pf_SetForEachCut( pList, pCut, i )         for ( i = 0, pCut = pList + 1; i < pList[0]; i++, pCut += Pf_CutSize(pCut) + 1 )
#define Pf_ObjForEachCut( pCuts, i, nCuts )        for ( i = 0, i < nCuts; i++ )
#define Pf_CutForEachLit( pCut, Conf, iLit, i )    for ( i = 0; i < Pf_CutSize(pCut) && (iLit = Abc_Lit2LitV(Pf_CutLeaves(pCut), Pf_CutConfLit(Conf, i))); i++ )
#define Pf_CutForEachVar( pCut, Conf, iVar, c, i ) for ( i = 0; i < Pf_CutSize(pCut) && (iVar = Pf_CutLeaves(pCut)[Pf_CutConfVar(Conf, i)]) && ((c = Pf_CutConfC(Conf, i)), 1); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pf_StoCreateGateAdd( Pf_Man_t * pMan, word uTruth, int * pFans, int nFans, int CellId )
{
    Vec_Int_t * vArray;
    Pf_Mat_t Mat = Pf_Int2Mat(0);
    int i, GateId, Entry, fCompl = (int)(uTruth & 1);
    word uFunc = fCompl ? ~uTruth : uTruth;
    int iFunc = Vec_MemHashInsert( pMan->vTtMem, &uFunc );
    if ( iFunc == Vec_WecSize(pMan->vTt2Match) )
        Vec_WecPushLevel( pMan->vTt2Match );
    vArray = Vec_WecEntry( pMan->vTt2Match, iFunc );
    Mat.fCompl = fCompl;
    assert( nFans < 7 );
    for ( i = 0; i < nFans; i++ )
    {
        Mat.Perm  |= (unsigned)(Abc_Lit2Var(pFans[i]) << (3*i));
        Mat.Phase |= (unsigned)(Abc_LitIsCompl(pFans[i]) << i);
    }
    // check if the same one exists
    Vec_IntForEachEntryDouble( vArray, GateId, Entry, i )
        if ( GateId == CellId && Pf_Int2Mat(Entry).Phase == Mat.Phase )
            break;
    if ( i == Vec_IntSize(vArray) )
    {
        Vec_IntPush( vArray, CellId );
        Vec_IntPush( vArray, Pf_Mat2Int(Mat) );
    }
}
void Pf_StoCreateGate( Pf_Man_t * pMan, Mio_Cell_t * pCell, int ** pComp, int ** pPerm, int * pnPerms )
{
    int Perm[PF_LEAF_MAX], * Perm1, * Perm2;
    int nPerms = pnPerms[pCell->nFanins];
    int nMints = (1 << pCell->nFanins);
    word tCur, tTemp1, tTemp2;
    int i, p, c;
    for ( i = 0; i < (int)pCell->nFanins; i++ )
        Perm[i] = Abc_Var2Lit( i, 0 );
    tCur = tTemp1 = pCell->uTruth;
    for ( p = 0; p < nPerms; p++ )
    {
        tTemp2 = tCur;
        for ( c = 0; c < nMints; c++ )
        {
            Pf_StoCreateGateAdd( pMan, tCur, Perm, pCell->nFanins, pCell->Id );
            // update
            tCur  = Abc_Tt6Flip( tCur, pComp[pCell->nFanins][c] );
            Perm1 = Perm + pComp[pCell->nFanins][c];
            *Perm1 = Abc_LitNot( *Perm1 );
        }
        assert( tTemp2 == tCur );
        // update
        tCur = Abc_Tt6SwapAdjacent( tCur, pPerm[pCell->nFanins][p] );
        Perm1 = Perm + pPerm[pCell->nFanins][p];
        Perm2 = Perm1 + 1;
        ABC_SWAP( int, *Perm1, *Perm2 );
    }
    assert( tTemp1 == tCur );
}
void Pf_StoDeriveMatches( Pf_Man_t * p, int fVerbose )
{
//    abctime clk = Abc_Clock();
    int * pComp[7];
    int * pPerm[7];
    int nPerms[7], i;
    for ( i = 2; i <= 6; i++ )
        pComp[i] = Extra_GreyCodeSchedule( i );
    for ( i = 2; i <= 6; i++ )
        pPerm[i] = Extra_PermSchedule( i );
    for ( i = 2; i <= 6; i++ )
        nPerms[i] = Extra_Factorial( i );
    p->pCells = Mio_CollectRootsNewDefault( 6, &p->nCells, fVerbose );
    for ( i = 4; i < p->nCells; i++ )
        Pf_StoCreateGate( p, p->pCells + i, pComp, pPerm, nPerms );
    for ( i = 2; i <= 6; i++ )
        ABC_FREE( pComp[i] );
    for ( i = 2; i <= 6; i++ )
        ABC_FREE( pPerm[i] );
//    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}
void Pf_StoPrintOne( Pf_Man_t * p, int Count, int t, int i, int GateId, Pf_Mat_t Mat )
{
    Mio_Cell_t * pC = p->pCells + GateId;
    word * pTruth = Vec_MemReadEntry(p->vTtMem, t);
    int k, nSuppSize = Abc_TtSupportSize(pTruth, 6);
    printf( "%6d : ", Count );
    printf( "%6d : ", t );
    printf( "%6d : ", i );
    printf( "Gate %16s  ",   pC->pName );
    printf( "Area =%8.2f  ", pC->Area );
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
void Pf_StoPrint( Pf_Man_t * p, int fVerbose )
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
            if ( t < 10 )
                Pf_StoPrintOne( p, Count, t, i/2, GateId, Pf_Int2Mat(Entry) );
        }
    }
    printf( "Gates = %d.  Truths = %d.  Matches = %d.\n", 
        p->nCells, Vec_MemEntryNum(p->vTtMem), Count );
}
/*
void Pf_ManPrepareLibraryTest()
{
    int fVerbose = 0;
    abctime clk = Abc_Clock();
    Pf_Man_t * p;
    p = Pf_StoCreate( NULL, NULL, fVerbose );
    Pf_StoPrint( p, fVerbose );
    Pf_StoDelete(p);
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
}
*/



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Pf_Man_t * Pf_StoCreate( Gia_Man_t * pGia, Jf_Par_t * pPars )
{
    extern void Mf_ManSetFlowRefs( Gia_Man_t * p, Vec_Int_t * vRefs );
    Pf_Man_t * p;
    Vec_Int_t * vFlowRefs;
    assert( pPars->nCutNum > 1  && pPars->nCutNum <= PF_CUT_MAX );
    assert( pPars->nLutSize > 1 && pPars->nLutSize <= PF_LEAF_MAX );
    ABC_FREE( pGia->pRefs );
    Vec_IntFreeP( &pGia->vCellMapping );
    if ( Gia_ManHasChoices(pGia) )
        Gia_ManSetPhase(pGia);
    // create references
    ABC_FREE( pGia->pRefs );
    vFlowRefs = Vec_IntAlloc(0);
    Mf_ManSetFlowRefs( pGia, vFlowRefs );
    pGia->pRefs= Vec_IntReleaseArray(vFlowRefs);
    Vec_IntFree(vFlowRefs);
    // create
    p = ABC_CALLOC( Pf_Man_t, 1 );
    p->clkStart = Abc_Clock();
    p->pGia     = pGia;
    p->pPars    = pPars;
    p->pPfObjs  = ABC_CALLOC( Pf_Obj_t, Gia_ManObjNum(pGia) );
    p->iCur     = 2;
    // other
    Vec_PtrGrow( &p->vPages, 256 );                                    // cut memory
    Vec_IntFill( &p->vCutSets,  Gia_ManObjNum(pGia), 0 );              // cut offsets
    Vec_FltFill( &p->vCutFlows, Gia_ManObjNum(pGia), 0 );              // cut area
    Vec_IntFill( &p->vCutDelays,Gia_ManObjNum(pGia), 0 );              // cut delay
    // matching
    p->vTtMem    = Vec_MemAllocForTT( 6, 0 );          
    p->vTt2Match = Vec_WecAlloc( 1000 ); 
    Vec_WecPushLevel( p->vTt2Match );
    Vec_WecPushLevel( p->vTt2Match );
    assert( Vec_WecSize(p->vTt2Match) == Vec_MemEntryNum(p->vTtMem) );
    Pf_StoDeriveMatches( p, 0 );//pPars->fVerbose );
    p->InvDelay = p->pCells[3].Delays[0];
    p->InvArea  = p->pCells[3].Area;
    //Pf_ObjMatchD(p, 0, 0)->Gate = 0;
    //Pf_ObjMatchD(p, 0, 1)->Gate = 1;
    // prepare cuts
    return p;
}
void Pf_StoDelete( Pf_Man_t * p )
{
    Vec_PtrFreeData( &p->vPages );
    ABC_FREE( p->vPages.pArray );
    ABC_FREE( p->vCutSets.pArray );
    ABC_FREE( p->vCutFlows.pArray );
    ABC_FREE( p->vCutDelays.pArray );
    ABC_FREE( p->pPfObjs );
    // matching
    Vec_WecFree( p->vTt2Match );
    Vec_MemHashFree( p->vTtMem );
    Vec_MemFree( p->vTtMem );
    ABC_FREE( p->pCells );
    ABC_FREE( p );
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Pf_CutComputeTruth6( Pf_Man_t * p, Pf_Cut_t * pCut0, Pf_Cut_t * pCut1, int fCompl0, int fCompl1, Pf_Cut_t * pCutR, int fIsXor )
{
//    extern int Pf_ManTruthCanonicize( word * t, int nVars );
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
    pCutR->Useless = Pf_ObjCutUseless( p, truthId );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
}
static inline int Pf_CutComputeTruthMux6( Pf_Man_t * p, Pf_Cut_t * pCut0, Pf_Cut_t * pCut1, Pf_Cut_t * pCutC, int fCompl0, int fCompl1, int fComplC, Pf_Cut_t * pCutR )
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
    pCutR->Useless = Pf_ObjCutUseless( p, truthId );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Pf_CutCountBits( word i )
{
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F);
    return (i*(0x0101010101010101))>>56;
}
static inline word Pf_CutGetSign( int * pLeaves, int nLeaves )
{
    word Sign = 0; int i; 
    for ( i = 0; i < nLeaves; i++ )
        Sign |= ((word)1) << (pLeaves[i] & 0x3F);
    return Sign;
}
static inline int Pf_CutCreateUnit( Pf_Cut_t * p, int i )
{
    p->Delay      = 0;
    p->Flow       = 0;
    p->iFunc      = 2;
    p->nLeaves    = 1;
    p->pLeaves[0] = i;
    p->Sign       = ((word)1) << (i & 0x3F);
    return 1;
}
static inline void Pf_Cutprintf( Pf_Man_t * p, Pf_Cut_t * pCut )
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
static inline int Pf_ManPrepareCuts( Pf_Cut_t * pCuts, Pf_Man_t * p, int iObj, int fAddUnit )
{
    if ( Pf_ObjHasCuts(p, iObj) )
    {
        Pf_Cut_t * pMfCut = pCuts;
        int i, * pCut, * pList = Pf_ObjCutSet(p, iObj);
        Pf_SetForEachCut( pList, pCut, i )
        {
            pMfCut->Delay   = 0;
            pMfCut->Flow    = 0;
            pMfCut->iFunc   = Pf_CutFunc( pCut );
            pMfCut->nLeaves = Pf_CutSize( pCut );
            pMfCut->Sign    = Pf_CutGetSign( pCut+1, Pf_CutSize(pCut) );
            pMfCut->Useless = Pf_ObjCutUseless( p, Abc_Lit2Var(pMfCut->iFunc) );
            memcpy( pMfCut->pLeaves, pCut+1, sizeof(int) * Pf_CutSize(pCut) );
            pMfCut++;
        }
        if ( fAddUnit && pCuts->nLeaves > 1 )
            return pList[0] + Pf_CutCreateUnit( pMfCut, iObj );
        return pList[0];
    }
    return Pf_CutCreateUnit( pCuts, iObj );
}
static inline int Pf_ManSaveCuts( Pf_Man_t * p, Pf_Cut_t ** pCuts, int nCuts, int fUseful )
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
    pPlace = Pf_ManCutSet( p, iCur );
    *pPlace++ = nCutsNew;
    for ( i = 0; i < nCuts; i++ )
        if ( !fUseful || !pCuts[i]->Useless )
        {
            *pPlace++ = Pf_CutSetBoth( pCuts[i]->nLeaves, pCuts[i]->iFunc );
            memcpy( pPlace, pCuts[i]->pLeaves, sizeof(int) * pCuts[i]->nLeaves );
            pPlace += pCuts[i]->nLeaves;
        }
    return iCur;
}
static inline int Pf_ManCountUseful( Pf_Cut_t ** pCuts, int nCuts )
{
    int i, Count = 0;
    for ( i = 0; i < nCuts; i++ )
        Count += !pCuts[i]->Useless;
    return Count;
}
static inline int Pf_ManCountMatches( Pf_Man_t * p, Pf_Cut_t ** pCuts, int nCuts )
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
static inline int Pf_CutCheck( Pf_Cut_t * pBase, Pf_Cut_t * pCut ) // check if pCut is contained in pBase
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
static inline int Pf_SetCheckArray( Pf_Cut_t ** ppCuts, int nCuts )
{
    Pf_Cut_t * pCut0, * pCut1; 
    int i, k, m, n, Value;
    assert( nCuts > 0 );
    for ( i = 0; i < nCuts; i++ )
    {
        pCut0 = ppCuts[i];
        assert( pCut0->nLeaves <= PF_LEAF_MAX );
        assert( pCut0->Sign == Pf_CutGetSign(pCut0->pLeaves, pCut0->nLeaves) );
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
            Value = Pf_CutCheck( pCut0, pCut1 );
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
static inline int Pf_CutMergeOrder( Pf_Cut_t * pCut0, Pf_Cut_t * pCut1, Pf_Cut_t * pCut, int nLutSize )
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
        pCut->iFunc = PF_NO_FUNC;
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
    pCut->iFunc = PF_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;

FlushCut1:
    if ( c + nSize1 > nLutSize + k ) return 0;
    while ( k < nSize1 )
        pC[c++] = pC1[k++];
    pCut->nLeaves = c;
    pCut->iFunc = PF_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;
}
static inline int Pf_CutMergeOrderMux( Pf_Cut_t * pCut0, Pf_Cut_t * pCut1, Pf_Cut_t * pCut2, Pf_Cut_t * pCut, int nLutSize )
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
    pCut->iFunc = PF_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign | pCut2->Sign;
    return 1;
}
static inline int Pf_SetCutIsContainedOrder( Pf_Cut_t * pBase, Pf_Cut_t * pCut ) // check if pCut is contained in pBase
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
static inline int Pf_SetLastCutIsContained( Pf_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[i]->nLeaves <= pCuts[nCuts]->nLeaves && (pCuts[i]->Sign & pCuts[nCuts]->Sign) == pCuts[i]->Sign && Pf_SetCutIsContainedOrder(pCuts[nCuts], pCuts[i]) )
            return 1;
    return 0;
}
static inline int Pf_SetLastCutContainsArea( Pf_Cut_t ** pCuts, int nCuts )
{
    int i, k, fChanges = 0;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[nCuts]->nLeaves < pCuts[i]->nLeaves && (pCuts[nCuts]->Sign & pCuts[i]->Sign) == pCuts[nCuts]->Sign && Pf_SetCutIsContainedOrder(pCuts[i], pCuts[nCuts]) )
            pCuts[i]->nLeaves = PF_NO_LEAF, fChanges = 1;
    if ( !fChanges )
        return nCuts;
    for ( i = k = 0; i <= nCuts; i++ )
    {
        if ( pCuts[i]->nLeaves == PF_NO_LEAF )
            continue;
        if ( k < i )
            ABC_SWAP( Pf_Cut_t *, pCuts[k], pCuts[i] );
        k++;
    }
    return k - 1;
}
static inline int Pf_CutCompareArea( Pf_Cut_t * pCut0, Pf_Cut_t * pCut1 )
{
    if ( pCut0->Useless < pCut1->Useless )  return -1;
    if ( pCut0->Useless > pCut1->Useless )  return  1;
    if ( pCut0->Flow    < pCut1->Flow    )  return -1;
    if ( pCut0->Flow    > pCut1->Flow    )  return  1;
    if ( pCut0->Delay   < pCut1->Delay   )  return -1;
    if ( pCut0->Delay   > pCut1->Delay   )  return  1;
    if ( pCut0->nLeaves < pCut1->nLeaves )  return -1;
    if ( pCut0->nLeaves > pCut1->nLeaves )  return  1;
    return 0;
}
static inline void Pf_SetSortByArea( Pf_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = nCuts; i > 0; i-- )
    {
        if ( Pf_CutCompareArea(pCuts[i - 1], pCuts[i]) < 0 )//!= 1 )
            return;
        ABC_SWAP( Pf_Cut_t *, pCuts[i - 1], pCuts[i] );
    }
}
static inline int Pf_SetAddCut( Pf_Cut_t ** pCuts, int nCuts, int nCutNum )
{
    if ( nCuts == 0 )
        return 1;
    nCuts = Pf_SetLastCutContainsArea(pCuts, nCuts);
    Pf_SetSortByArea( pCuts, nCuts );
    return Abc_MinInt( nCuts + 1, nCutNum - 1 );
}
static inline int Pf_CutArea( Pf_Man_t * p, int nLeaves )
{
    if ( nLeaves < 2 )
        return 0;
    return nLeaves + p->pPars->nAreaTuner;
}
static inline void Pf_CutParams( Pf_Man_t * p, Pf_Cut_t * pCut, int nGiaRefs )
{
    int i, nLeaves = pCut->nLeaves; 
    assert( nLeaves <= p->pPars->nLutSize );
    pCut->Delay = 0;
    pCut->Flow  = 0;
    for ( i = 0; i < nLeaves; i++ )
    {
        pCut->Delay = Abc_MaxInt( pCut->Delay, Pf_ObjCutDelay(p, pCut->pLeaves[i]) );
        pCut->Flow += Pf_ObjCutFlow(p, pCut->pLeaves[i]);
    }
    pCut->Delay += (int)(nLeaves > 1);
    pCut->Flow = (pCut->Flow + Pf_CutArea(p, nLeaves)) / (nGiaRefs ? nGiaRefs : 1);
}
void Pf_ObjMergeOrder( Pf_Man_t * p, int iObj )
{
    Pf_Cut_t pCuts0[PF_CUT_MAX], pCuts1[PF_CUT_MAX], pCuts[PF_CUT_MAX], * pCutsR[PF_CUT_MAX];
    Gia_Obj_t * pObj = Gia_ManObj(p->pGia, iObj);
    int nGiaRefs = 2*Gia_ObjRefNumId(p->pGia, iObj);
    int nLutSize = p->pPars->nLutSize;
    int nCutNum  = p->pPars->nCutNum;
    int nCuts0   = Pf_ManPrepareCuts(pCuts0, p, Gia_ObjFaninId0(pObj, iObj), 1);
    int nCuts1   = Pf_ManPrepareCuts(pCuts1, p, Gia_ObjFaninId1(pObj, iObj), 1);
    int fComp0   = Gia_ObjFaninC0(pObj);
    int fComp1   = Gia_ObjFaninC1(pObj);
    int iSibl    = Gia_ObjSibl(p->pGia, iObj);
    Pf_Cut_t * pCut0, * pCut1, * pCut0Lim = pCuts0 + nCuts0, * pCut1Lim = pCuts1 + nCuts1;
    int i, nCutsUse, nCutsR = 0;
    assert( !Gia_ObjIsBuf(pObj) );
    for ( i = 0; i < nCutNum; i++ )
        pCutsR[i] = pCuts + i;
    if ( iSibl )
    {
        Pf_Cut_t pCuts2[PF_CUT_MAX];
        Gia_Obj_t * pObjE = Gia_ObjSiblObj(p->pGia, iObj);
        int fCompE = Gia_ObjPhase(pObj) ^ Gia_ObjPhase(pObjE);
        int nCuts2 = Pf_ManPrepareCuts(pCuts2, p, iSibl, 0);
        Pf_Cut_t * pCut2, * pCut2Lim = pCuts2 + nCuts2;
        for ( pCut2 = pCuts2; pCut2 < pCut2Lim; pCut2++ )
        {
            *pCutsR[nCutsR] = *pCut2;
            pCutsR[nCutsR]->iFunc = Abc_LitNotCond( pCutsR[nCutsR]->iFunc, fCompE );
            Pf_CutParams( p, pCutsR[nCutsR], nGiaRefs );
            nCutsR = Pf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    if ( Gia_ObjIsMuxId(p->pGia, iObj) )
    {
        Pf_Cut_t pCuts2[PF_CUT_MAX];
        int nCuts2  = Pf_ManPrepareCuts(pCuts2, p, Gia_ObjFaninId2(p->pGia, iObj), 1);
        int fComp2  = Gia_ObjFaninC2(p->pGia, pObj);
        Pf_Cut_t * pCut2, * pCut2Lim = pCuts2 + nCuts2;
        p->CutCount[0] += nCuts0 * nCuts1 * nCuts2;
        for ( pCut0 = pCuts0; pCut0 < pCut0Lim; pCut0++ )
        for ( pCut1 = pCuts1; pCut1 < pCut1Lim; pCut1++ )
        for ( pCut2 = pCuts2; pCut2 < pCut2Lim; pCut2++ )
        {
            if ( Pf_CutCountBits(pCut0->Sign | pCut1->Sign | pCut2->Sign) > nLutSize )
                continue;
            p->CutCount[1]++; 
            if ( !Pf_CutMergeOrderMux(pCut0, pCut1, pCut2, pCutsR[nCutsR], nLutSize) )
                continue;
            if ( Pf_SetLastCutIsContained(pCutsR, nCutsR) )
                continue;
            p->CutCount[2]++;
            if ( Pf_CutComputeTruthMux6(p, pCut0, pCut1, pCut2, fComp0, fComp1, fComp2, pCutsR[nCutsR]) )
                pCutsR[nCutsR]->Sign = Pf_CutGetSign(pCutsR[nCutsR]->pLeaves, pCutsR[nCutsR]->nLeaves);
            Pf_CutParams( p, pCutsR[nCutsR], nGiaRefs );
            nCutsR = Pf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    else
    {
        int fIsXor = Gia_ObjIsXor(pObj);
        p->CutCount[0] += nCuts0 * nCuts1;
        for ( pCut0 = pCuts0; pCut0 < pCut0Lim; pCut0++ )
        for ( pCut1 = pCuts1; pCut1 < pCut1Lim; pCut1++ )
        {
            if ( (int)(pCut0->nLeaves + pCut1->nLeaves) > nLutSize && Pf_CutCountBits(pCut0->Sign | pCut1->Sign) > nLutSize )
                continue;
            p->CutCount[1]++; 
            if ( !Pf_CutMergeOrder(pCut0, pCut1, pCutsR[nCutsR], nLutSize) )
                continue;
            if ( Pf_SetLastCutIsContained(pCutsR, nCutsR) )
                continue;
            p->CutCount[2]++;
            if ( Pf_CutComputeTruth6(p, pCut0, pCut1, fComp0, fComp1, pCutsR[nCutsR], fIsXor) )
                pCutsR[nCutsR]->Sign = Pf_CutGetSign(pCutsR[nCutsR]->pLeaves, pCutsR[nCutsR]->nLeaves);
            Pf_CutParams( p, pCutsR[nCutsR], nGiaRefs );
            nCutsR = Pf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    // debug printout
    if ( 0 )
//    if ( iObj % 10000 == 0 )
//    if ( iObj == 1090 )
    {
        printf( "*** Obj = %d  Useful = %d\n", iObj, Pf_ManCountUseful(pCutsR, nCutsR) );
        for ( i = 0; i < nCutsR; i++ )
            Pf_Cutprintf( p, pCutsR[i] );
        printf( "\n" );
    } 
    // verify
    assert( nCutsR > 0 && nCutsR < nCutNum );
//    assert( Pf_SetCheckArray(pCutsR, nCutsR) );
    // store the cutset
    Pf_ObjSetCutFlow( p, iObj, pCutsR[0]->Flow );
    Pf_ObjSetCutDelay( p, iObj, pCutsR[0]->Delay );
    *Vec_IntEntryP(&p->vCutSets, iObj) = Pf_ManSaveCuts(p, pCutsR, nCutsR, 0);
    p->CutCount[3] += nCutsR;
    nCutsUse = Pf_ManCountUseful(pCutsR, nCutsR);
    p->CutCount[4] += nCutsUse;
    p->nCutUseAll  += nCutsUse == nCutsR;
    p->CutCount[5] += Pf_ManCountMatches(p, pCutsR, nCutsR);
}
void Pf_ManComputeCuts( Pf_Man_t * p )
{
    Gia_Obj_t * pObj; int i, iFanin;
    Gia_ManForEachAnd( p->pGia, pObj, i )
        if ( Gia_ObjIsBuf(pObj) )
        {
            iFanin = Gia_ObjFaninId0(pObj, i);
            Pf_ObjSetCutFlow( p, i,  Pf_ObjCutFlow(p, iFanin) );
            Pf_ObjSetCutDelay( p, i, Pf_ObjCutDelay(p, iFanin) );
        }
        else
            Pf_ObjMergeOrder( p, i );
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pf_ManPrintStats( Pf_Man_t * p, char * pTitle )
{
    if ( !p->pPars->fVerbose )
        return;
    printf( "%s :  ", pTitle );
    printf( "Delay =%8.2f  ",  p->pPars->MapDelay );
    printf( "Area =%12.2f  ",  p->pPars->MapArea );
    printf( "Gate =%6d  ",    (int)p->pPars->Area );
    printf( "Inv =%6d  ",     (int)p->nInvs );
    printf( "Edge =%7d  ",    (int)p->pPars->Edge );
    Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
    fflush( stdout );
}
void Pf_ManPrintInit( Pf_Man_t * p )
{
    int nChoices;
    if ( !p->pPars->fVerbose )
        return;
    printf( "LutSize = %d  ", p->pPars->nLutSize );
    printf( "CutNum = %d  ",  p->pPars->nCutNum );
    printf( "Iter = %d  ",    p->pPars->nRounds + p->pPars->nRoundsEla );
    printf( "Coarse = %d   ", p->pPars->fCoarsen );
    printf( "Cells = %d  ",   p->nCells );
    printf( "Funcs = %d  ",   Vec_MemEntryNum(p->vTtMem) );
    printf( "Matches = %d  ", Vec_WecSizeSize(p->vTt2Match)/2 );
    nChoices = Gia_ManChoiceNum( p->pGia );
    if ( nChoices )
    printf( "Choices = %d  ", nChoices );
    printf( "\n" );
    printf( "Computing cuts...\r" );
    fflush( stdout );
}
void Pf_ManPrintQuit( Pf_Man_t * p )
{
    float MemGia   = Gia_ManMemory(p->pGia) / (1<<20);
    float MemMan   =(1.0 * sizeof(Pf_Obj_t) + 3.0 * sizeof(int)) * Gia_ManObjNum(p->pGia) / (1<<20);
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
/*
void Pf_ManSetMapRefsGate( Pf_Man_t * p, int iObj, float Required, Pf_Mat_t * pM )
{
    int k, iVar, fCompl;
    Mio_Cell_t * pCell = Pf_ManCell( p, pM->Gate );
    int * pCut = Pf_CutFromHandle( Pf_ObjCutSet(p, iObj), pM->CutH );
    Pf_CutForEachVar( pCut, pM->Conf, iVar, fCompl, k )
    {
        Pf_ObjMapRefInc( p, iVar, fCompl );
        Pf_ObjUpdateRequired( p, iVar, fCompl, Required - pCell->Delays[k] );
    }
    assert( Pf_CutSize(pCut) == (int)pCell->nFanins );
    // update global stats
    p->pPars->MapArea += pCell->Area;
    p->pPars->Edge += Pf_CutSize(pCut);
    p->pPars->Area++;
    // update status of the gate
    assert( pM->fBest == 0 );
    pM->fBest = 1;
}
int Pf_ManSetMapRefs( Pf_Man_t * p )
{
    float Coef = 1.0 / (1.0 + (p->Iter + 1) * (p->Iter + 1));
    float * pFlowRefs = Vec_FltArray( &p->vFlowRefs );
    int * pMapRefs = Vec_IntArray( &p->vMapRefs );
    float Epsilon = p->pPars->Epsilon;
    int nLits = 2*Gia_ManObjNum(p->pGia);
    int i, c, Id, nRefs[2];
    Pf_Mat_t * pD, * pA, * pM;
    Pf_Mat_t * pDs[2], * pAs[2], * pMs[2];
    Gia_Obj_t * pObj;
    float Required = 0, Requireds[2];
    // check references
    assert( !p->fUseEla );
    memset( pMapRefs, 0, sizeof(int) * nLits );
    Vec_FltFill( &p->vRequired, nLits, PF_INFINITY );
//    for ( i = 0; i < Gia_ManObjNum(p->pGia); i++ )
//        assert( !Pf_ObjMapRefNum(p, i, 0) && !Pf_ObjMapRefNum(p, i, 1) );
    // compute delay
    p->pPars->MapDelay = 0;
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        Required = Pf_ObjMatchD( p, Gia_ObjFaninId0p(p->pGia, pObj), Gia_ObjFaninC0(pObj) )->D;
        if ( Required == PF_INFINITY )
        {
            Pf_ManCutMatchprintf( p, Gia_ObjFaninId0p(p->pGia, pObj), Gia_ObjFaninC0(pObj), Pf_ObjMatchD( p, Gia_ObjFaninId0p(p->pGia, pObj), Gia_ObjFaninC0(pObj) ) );
        }
        p->pPars->MapDelay = Abc_MaxFloat( p->pPars->MapDelay, Required );
    }
    // check delay target
    if ( p->pPars->MapDelayTarget == -1 && p->pPars->nRelaxRatio )
        p->pPars->MapDelayTarget = (int)((float)p->pPars->MapDelay * (100.0 + p->pPars->nRelaxRatio) / 100.0);
    if ( p->pPars->MapDelayTarget != -1 )
    {
        if ( p->pPars->MapDelay < p->pPars->MapDelayTarget + Epsilon )
            p->pPars->MapDelay = p->pPars->MapDelayTarget;
        else if ( p->pPars->nRelaxRatio == 0 )
            Abc_Print( 0, "Relaxing user-specified delay target from %.2f to %.2f.\n", p->pPars->MapDelayTarget, p->pPars->MapDelay );
    }
    // set required times
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        Required = Pf_ObjMatchD( p, Gia_ObjFaninId0p(p->pGia, pObj), Gia_ObjFaninC0(pObj) )->D;
        Required = p->pPars->fDoAverage ? Required * (100.0 + p->pPars->nRelaxRatio) / 100.0 : p->pPars->MapDelay;
        Pf_ObjUpdateRequired( p, Gia_ObjFaninId0p(p->pGia, pObj), Gia_ObjFaninC0(pObj), Required );
        Pf_ObjMapRefInc( p, Gia_ObjFaninId0p(p->pGia, pObj), Gia_ObjFaninC0(pObj));
    }
    // compute area and edges
    p->nInvs = 0;
    p->pPars->MapArea = 0; 
    p->pPars->Area = p->pPars->Edge = 0;
    Gia_ManForEachAndReverse( p->pGia, pObj, i )
    {
        if ( Gia_ObjIsBuf(pObj) )
        {
            if ( Pf_ObjMapRefNum(p, i, 1) )
            {
                Pf_ObjMapRefInc( p, i, 0 );
                Pf_ObjUpdateRequired( p, i, 0, Pf_ObjRequired(p, i, 1) - p->InvDelay );
                p->pPars->MapArea += p->InvArea;
                p->pPars->Edge++;
                p->pPars->Area++;
                p->nInvs++;
            }
            Pf_ObjUpdateRequired( p, Gia_ObjFaninId0(pObj, i), Gia_ObjFaninC0(pObj), Pf_ObjRequired(p, i, 0) );
            Pf_ObjMapRefInc( p, Gia_ObjFaninId0(pObj, i), Gia_ObjFaninC0(pObj));
            continue;
        }
        // skip if this node is not used
        for ( c = 0; c < 2; c++ )
        {
            nRefs[c] = Pf_ObjMapRefNum(p, i, c);

            //if ( Pf_ObjMatchD( p, i, c )->fCompl )
            //    printf( "Match D of node %d has inv in phase %d.\n", i, c );
            //if ( Pf_ObjMatchA( p, i, c )->fCompl )
            //    printf( "Match A of node %d has inv in phase %d.\n", i, c );
        }
        if ( !nRefs[0] && !nRefs[1] )
            continue;

        // consider two cases
        if ( nRefs[0] && nRefs[1] )
        {
            // find best matches for both phases
            for ( c = 0; c < 2; c++ )
            {
                Requireds[c] = Pf_ObjRequired( p, i, c );
                //assert( Requireds[c] < PF_INFINITY );
                pDs[c] = Pf_ObjMatchD( p, i, c );
                pAs[c] = Pf_ObjMatchA( p, i, c );
                pMs[c] = (pAs[c]->D < Requireds[c] + Epsilon) ? pAs[c] : pDs[c];
            }
            // swap complemented matches
            if ( pMs[0]->fCompl && pMs[1]->fCompl )
            {
                pMs[0]->fCompl = pMs[1]->fCompl = 0;
                ABC_SWAP( Pf_Mat_t *, pMs[0], pMs[1] );
            }
            // check if intervers are involved
            if ( !pMs[0]->fCompl && !pMs[1]->fCompl )
            {
                // no inverters
                for ( c = 0; c < 2; c++ )
                    Pf_ManSetMapRefsGate( p, i, Requireds[c], pMs[c] );
            }
            else 
            {
                // one interver
                assert( !pMs[0]->fCompl || !pMs[1]->fCompl );
                c = pMs[1]->fCompl;
                assert( pMs[c]->fCompl && !pMs[!c]->fCompl );
                //printf( "Using inverter at node %d in phase %d\n", i, c );

                // update this phase phase
                pM = pMs[c];
                pM->fBest = 1;
                Required = Requireds[c];

                // update opposite phase
                Pf_ObjMapRefInc( p, i, !c );
                Pf_ObjUpdateRequired( p, i, !c, Required - p->InvDelay );

                // select oppositve phase
                Required = Pf_ObjRequired( p, i, !c );
                //assert( Required < PF_INFINITY );
                pD = Pf_ObjMatchD( p, i, !c );
                pA = Pf_ObjMatchA( p, i, !c );
                pM = (pA->D < Required + Epsilon) ? pA : pD;
                assert( !pM->fCompl );

                // account for the inverter
                p->pPars->MapArea += p->InvArea;
                p->pPars->Edge++;
                p->pPars->Area++;
                p->nInvs++;

                // create gate
                Pf_ManSetMapRefsGate( p, i, Required, pM );
            }
        }
        else
        {
            c = (int)(nRefs[1] > 0);
            assert( nRefs[c] && !nRefs[!c] );
            // consider this phase
            Required = Pf_ObjRequired( p, i, c );
            //assert( Required < PF_INFINITY );
            pD = Pf_ObjMatchD( p, i, c );
            pA = Pf_ObjMatchA( p, i, c );
            pM = (pA->D < Required + Epsilon) ? pA : pD;

            if ( pM->fCompl ) // use inverter
            {
                p->nInvs++;
                //printf( "Using inverter at node %d in phase %d\n", i, c );
                pM->fBest = 1;
                // update opposite phase
                Pf_ObjMapRefInc( p, i, !c );
                Pf_ObjUpdateRequired( p, i, !c, Required - p->InvDelay );
                // select oppositve phase
                Required = Pf_ObjRequired( p, i, !c );
                //assert( Required < PF_INFINITY );
                pD = Pf_ObjMatchD( p, i, !c );
                pA = Pf_ObjMatchA( p, i, !c );
                pM = (pA->D < Required + Epsilon) ? pA : pD;
                assert( !pM->fCompl );

                // account for the inverter
                p->pPars->MapArea += p->InvArea;
                p->pPars->Edge++;
                p->pPars->Area++;
            }

            // create gate
            Pf_ManSetMapRefsGate( p, i, Required, pM );
        }


        // the result of this:
        // - only one phase can be implemented as inverter of the other phase
        // - required times are propagated correctly
        // - references are set correctly
    }
    Gia_ManForEachCiId( p->pGia, Id, i )
        if ( Pf_ObjMapRefNum(p, Id, 1) )
        {
            Pf_ObjMapRefInc( p, Id, 0 );
            Pf_ObjUpdateRequired( p, Id, 0, Required - p->InvDelay );
            p->pPars->MapArea += p->InvArea;
            p->pPars->Edge++;
            p->pPars->Area++;
            p->nInvs++;
        }
    // blend references
    for ( i = 0; i < nLits; i++ )
//        pFlowRefs[i] = Abc_MaxFloat(1.0, pMapRefs[i]);
        pFlowRefs[i] = Abc_MaxFloat(1.0, Coef * pFlowRefs[i] + (1.0 - Coef) * Abc_MaxFloat(1, pMapRefs[i]));
//        pFlowRefs[i] = 0.2 * pFlowRefs[i] + 0.8 * Abc_MaxFloat(1, pMapRefs[i]);
//    memset( pMapRefs, 0, sizeof(int) * nLits );
    return p->pPars->Area;
}
Gia_Man_t * Pf_ManDeriveMapping( Pf_Man_t * p )
{
    Vec_Int_t * vMapping;
    Pf_Mat_t * pM;
    int i, k, c, Id, iLit, * pCut;
    assert( p->pGia->vCellMapping == NULL );
    vMapping = Vec_IntAlloc( 2*Gia_ManObjNum(p->pGia) + (int)p->pPars->Edge + (int)p->pPars->Area * 2 );
    Vec_IntFill( vMapping, 2*Gia_ManObjNum(p->pGia), 0 );
    // create CI inverters
    Gia_ManForEachCiId( p->pGia, Id, i )
    if ( Pf_ObjMapRefNum(p, Id, 1) )
        Vec_IntWriteEntry( vMapping, Abc_Var2Lit(Id, 1), -1 );
    // create internal nodes
    Gia_ManForEachAndId( p->pGia, i )
    {
        Gia_Obj_t * pObj = Gia_ManObj(p->pGia, i);
        if ( Gia_ObjIsBuf(pObj) )
        {
            if ( Pf_ObjMapRefNum(p, i, 1) )
                Vec_IntWriteEntry( vMapping, Abc_Var2Lit(i, 1), -1 );
            Vec_IntWriteEntry( vMapping, Abc_Var2Lit(i, 0), -2 );
            continue;
        }
        for ( c = 0; c < 2; c++ )
        if ( Pf_ObjMapRefNum(p, i, c) )
        {
    //        printf( "Using %d %d\n", i, c );
            pM = Pf_ObjMatchBest( p, i, c );
            // remember inverter
            if ( pM->fCompl )
            {
                Vec_IntWriteEntry( vMapping, Abc_Var2Lit(i, c), -1 );
                continue;
            }
    //        Pf_ManCutMatchprintf( p, i, c, pM );
            pCut = Pf_CutFromHandle( Pf_ObjCutSet(p, i), pM->CutH );
            // create mapping
            Vec_IntWriteEntry( vMapping, Abc_Var2Lit(i, c), Vec_IntSize(vMapping) );
            Vec_IntPush( vMapping, Pf_CutSize(pCut) );
            Pf_CutForEachLit( pCut, pM->Conf, iLit, k )
                Vec_IntPush( vMapping, iLit );
            Vec_IntPush( vMapping, pM->Gate );
        }
    }
//    assert( Vec_IntCap(vMapping) == 16 || Vec_IntSize(vMapping) == Vec_IntCap(vMapping) );
    p->pGia->vCellMapping = vMapping;
    return p->pGia;
}
*/


/**Function*************************************************************

  Synopsis    [Technology mappping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pf_ManComputeMapping( Pf_Man_t * p )
{
}

/**Function*************************************************************

  Synopsis    [Technology mappping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Pf_ManSetDefaultPars( Jf_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Jf_Par_t) );
    pPars->nLutSize     =  6;
    pPars->nCutNum      = 16;
    pPars->nProcNum     =  0;
    pPars->nRounds      =  3;
    pPars->nRoundsEla   =  0;
    pPars->nRelaxRatio  =  0;
    pPars->nCoarseLimit =  3;
    pPars->nAreaTuner   =  1;
    pPars->nVerbLimit   =  5;
    pPars->DelayTarget  = -1;
    pPars->fAreaOnly    =  0;
    pPars->fOptEdge     =  1; 
    pPars->fCoarsen     =  0;
    pPars->fCutMin      =  1;
    pPars->fGenCnf      =  0;
    pPars->fPureAig     =  0;
    pPars->fVerbose     =  0;
    pPars->fVeryVerbose =  0;
    pPars->nLutSizeMax  =  PF_LEAF_MAX;
    pPars->nCutNumMax   =  PF_CUT_MAX;
    pPars->MapDelayTarget = -1;
    pPars->Epsilon      = (float)0.01;
}
Gia_Man_t * Pf_ManPerformMapping( Gia_Man_t * pGia, Jf_Par_t * pPars )
{
    Gia_Man_t * pNew = NULL, * pCls;
    Pf_Man_t * p; 
    if ( Gia_ManHasChoices(pGia) )
        pPars->fCoarsen = 0; 
    pCls = pPars->fCoarsen ? Gia_ManDupMuxes(pGia, pPars->nCoarseLimit) : pGia;
    p = Pf_StoCreate( pCls, pPars );
//    if ( pPars->fVeryVerbose )
        Pf_StoPrint( p, 1 );
    if ( pPars->fVerbose && pPars->fCoarsen )
    {
        printf( "Initial " );  Gia_ManPrintMuxStats( pGia );  printf( "\n" );
        printf( "Derived " );  Gia_ManPrintMuxStats( pCls );  printf( "\n" );
    }
    Pf_ManPrintInit( p );
    Pf_ManComputeCuts( p );
    Pf_ManPrintQuit( p );
/*
    Gia_ManForEachCiId( p->pGia, Id, i )
        Pf_ObjPrepareCi( p, Id );
    for ( p->Iter = 0; p->Iter < p->pPars->nRounds; p->Iter++ )
    {
        Pf_ManComputeMapping( p );
        //Pf_ManSetMapRefs( p );
        Pf_ManPrintStats( p, p->Iter ? "Area " : "Delay" );
    }
    p->fUseEla = 1;
    for ( ; p->Iter < p->pPars->nRounds + pPars->nRoundsEla; p->Iter++ )
    {
        Pf_ManComputeMapping( p );
        //Pf_ManUpdateStats( p );
        Pf_ManPrintStats( p, "Ela  " );
    }
*/
    pNew = NULL; //Pf_ManDeriveMapping( p );
//    Gia_ManMappingVerify( pNew );
    Pf_StoDelete( p );
    if ( pCls != pGia )
        Gia_ManStop( pCls );
    if ( pNew == NULL )
        return Gia_ManDup( pGia );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

