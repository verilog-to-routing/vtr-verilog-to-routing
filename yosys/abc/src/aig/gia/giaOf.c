/**CFile****************************************************************

  FileName    [giaOf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [LUT structure mapper.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaOf.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/st/st.h"
#include "map/mio/mio.h"
#include "misc/util/utilTruth.h"
#include "misc/extra/extra.h"
#include "base/main/main.h"
#include "misc/vec/vecMem.h"
#include "misc/vec/vecWec.h"
#include "opt/dau/dau.h"
#include "sat/bsat/satStore.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define OF_LEAF_MAX  6
#define OF_CUT_MAX  32
#define OF_NO_LEAF  31
#define OF_NO_FUNC  0x7FFFFFF
#define OF_CUT_EXTRA 4 // size; delay1, delay2; area

typedef struct Of_Cut_t_ Of_Cut_t; 
struct Of_Cut_t_
{
    word            Sign;           // signature
    int             Delay;          // delay
    int             Flow;           // flow
    unsigned        iFunc   : 27;   // function (OF_NO_FUNC)
    unsigned        nLeaves :  5;   // leaf number (OF_NO_LEAF)
    int             pLeaves[OF_LEAF_MAX+1]; // leaves
};
typedef struct Of_Obj_t_ Of_Obj_t; 
struct Of_Obj_t_
{
    int             iCutH;          // best cut
    int             iCutH2;         // best cut
    int             Delay1;         // arrival time
    int             Delay2;         // arrival time
    int             Required;       // required
    int             nRefs;          // references 
    int             Flow;           // area flow
    int             Temp;           // unused
};
typedef struct Of_Man_t_ Of_Man_t; 
struct Of_Man_t_
{
    // user data
    Gia_Man_t *     pGia;           // derived manager
    Jf_Par_t *      pPars;          // parameters
    // cut data
    Vec_Mem_t *     vTtMem;         // truth tables
    Vec_Ptr_t       vPages;         // cut memory
    Vec_Int_t       vCutSets;       // cut offsets
    Vec_Int_t       vCutFlows;      // temporary cut area
    Vec_Int_t       vCutDelays;     // temporary cut delay
    Vec_Int_t       vCutRefs;       // temporary cut referebces
    int             iCur;           // current position
    int             Iter;           // mapping iterations
    // object data
    Of_Obj_t *      pObjs;          
    // statistics
    abctime         clkStart;       // starting time
    double          CutCount[6];    // cut counts
};

#define OF_NUM       10
#define OF_NUMINV   0.1

static inline int         Of_Flt2Int( float f )                                     { return (int)(OF_NUM*f);                                          }
static inline float       Of_Int2Flt( int i )                                       { return OF_NUMINV*i;                                              }

static inline int *       Of_ManCutSet( Of_Man_t * p, int i )                       { return (int *)Vec_PtrEntry(&p->vPages, i >> 16) + (i & 0xFFFF);  }
static inline int         Of_ObjCutSetId( Of_Man_t * p, int i )                     { return Vec_IntEntry( &p->vCutSets, i );                          }
static inline int *       Of_ObjCutSet( Of_Man_t * p, int i )                       { return Of_ManCutSet(p, Of_ObjCutSetId(p, i));                    }
static inline int         Of_ObjHasCuts( Of_Man_t * p, int i )                      { return (int)(Vec_IntEntry(&p->vCutSets, i) > 0);                 }

static inline int         Of_ObjCutFlow( Of_Man_t * p, int i )                      { return Vec_IntEntry(&p->vCutFlows, i);                           } 
static inline int         Of_ObjCutDelay( Of_Man_t * p, int i )                     { return Vec_IntEntry(&p->vCutDelays, i);                          } 
static inline void        Of_ObjSetCutFlow( Of_Man_t * p, int i, int a )            { Vec_IntWriteEntry(&p->vCutFlows, i, a);                          } 
static inline void        Of_ObjSetCutDelay( Of_Man_t * p, int i, int d )           { Vec_IntWriteEntry(&p->vCutDelays, i, d);                         } 

static inline int         Of_CutSize( int * pCut )                                  { return pCut[0] & OF_NO_LEAF;                                     }
static inline int         Of_CutFunc( int * pCut )                                  { return ((unsigned)pCut[0] >> 5);                                 }
static inline int *       Of_CutLeaves( int * pCut )                                { return pCut + 1;                                                 }
static inline int         Of_CutSetBoth( int n, int f )                             { return n | (f << 5);                                             }
static inline int         Of_CutHandle( int * pCutSet, int * pCut )                 { assert( pCut > pCutSet ); return pCut - pCutSet;                 } 
static inline int *       Of_CutFromHandle( int * pCutSet, int h )                  { assert( h > 0 ); return pCutSet + h;                             }

static inline int         Of_CutDelay1( int * pCut )                                { return pCut[1 + Of_CutSize(pCut)];                               }
static inline int         Of_CutDelay2( int * pCut )                                { return pCut[2 + Of_CutSize(pCut)];                               }
static inline int         Of_CutAreaFlow( int * pCut )                              { return pCut[3 + Of_CutSize(pCut)];                               }
static inline void        Of_CutSetDelay1( int * pCut, int d )                      { pCut[1 + Of_CutSize(pCut)] = d;                                  } 
static inline void        Of_CutSetDelay2( int * pCut, int d )                      { pCut[2 + Of_CutSize(pCut)] = d;                                  } 
static inline void        Of_CutSetAreaFlow( int * pCut, int d )                    { pCut[3 + Of_CutSize(pCut)] = d;                                  } 

static inline int         Of_CutVar( int * pCut, int v )                            { return Abc_Lit2Var(Of_CutLeaves(pCut)[v]);                       } 
static inline int         Of_CutFlag( int * pCut, int v )                           { return Abc_LitIsCompl(Of_CutLeaves(pCut)[v]);                    } 
static inline void        Of_CutCleanFlag( int * pCut, int v )                      { Of_CutLeaves(pCut)[v] = Abc_LitRegular(Of_CutLeaves(pCut)[v]);   } 
static inline void        Of_CutSetFlag( int * pCut, int v, int x )                 { Of_CutLeaves(pCut)[v] = Abc_Var2Lit(Of_CutVar(pCut, v), x);      } 

static inline Of_Obj_t *  Of_ObjData( Of_Man_t * p, int i )                         { return p->pObjs + i;                                             }

static inline int         Of_ObjCutBest( Of_Man_t * p, int i )                      { return Of_ObjData(p, i)->iCutH;                                  }
static inline int         Of_ObjCutBest2( Of_Man_t * p, int i )                     { return Of_ObjData(p, i)->iCutH2;                                 }
static inline int         Of_ObjDelay1( Of_Man_t * p, int i )                       { return Of_ObjData(p, i)->Delay1;                                 }
static inline int         Of_ObjDelay2( Of_Man_t * p, int i )                       { return Of_ObjData(p, i)->Delay2;                                 }
static inline int         Of_ObjRequired( Of_Man_t * p, int i )                     { return Of_ObjData(p, i)->Required;                               }
static inline int         Of_ObjRefNum( Of_Man_t * p, int i )                       { return Of_ObjData(p, i)->nRefs;                                  }
static inline int         Of_ObjFlow( Of_Man_t * p, int i )                         { return Of_ObjData(p, i)->Flow;                                   }

static inline void        Of_ObjSetCutBest( Of_Man_t * p, int i, int x )            { Of_ObjData(p, i)->iCutH = x;                                     }
static inline void        Of_ObjSetCutBest2( Of_Man_t * p, int i, int x )           { Of_ObjData(p, i)->iCutH2 = x;                                    }
static inline void        Of_ObjSetDelay1( Of_Man_t * p, int i, int x )             { Of_ObjData(p, i)->Delay1 = x;                                    }
static inline void        Of_ObjSetDelay2( Of_Man_t * p, int i, int x )             { Of_ObjData(p, i)->Delay2 = x;                                    }
static inline void        Of_ObjSetRequired( Of_Man_t * p, int i, int x )           { Of_ObjData(p, i)->Required = x;                                  }
static inline void        Of_ObjSetRefNum( Of_Man_t * p, int i, int x )             { Of_ObjData(p, i)->nRefs = x;                                     }
static inline void        Of_ObjSetFlow( Of_Man_t * p, int i, int x )               { Of_ObjData(p, i)->Flow = x;                                      }
static inline void        Of_ObjUpdateRequired( Of_Man_t * p,int i, int x )         { if ( Of_ObjRequired(p, i) > x ) Of_ObjSetRequired(p, i, x);      }
static inline int         Of_ObjRefInc( Of_Man_t * p, int i )                       { return Of_ObjData(p, i)->nRefs++;                                }
static inline int         Of_ObjRefDec( Of_Man_t * p, int i )                       { return --Of_ObjData(p, i)->nRefs;                                }

static inline int *       Of_ObjCutBestP( Of_Man_t * p, int iObj )                  { assert(iObj>0 && iObj<Gia_ManObjNum(p->pGia));return Of_ManCutSet( p, Of_ObjCutBest(p, iObj) );  }
static inline void        Of_ObjSetCutBestP( Of_Man_t * p, int * pCutSet, int iObj, int * pCut ) { Of_ObjSetCutBest( p, iObj, Of_ObjCutSetId(p, iObj) + Of_CutHandle(pCutSet, pCut) ); }

static inline int *       Of_ObjCutBestP2( Of_Man_t * p, int iObj )                 { assert(iObj>0 && iObj<Gia_ManObjNum(p->pGia));return Of_ManCutSet( p, Of_ObjCutBest2(p, iObj) );  }
static inline void        Of_ObjSetCutBestP2( Of_Man_t * p, int * pCutSet, int iObj, int * pCut ) { Of_ObjSetCutBest2( p, iObj, Of_ObjCutSetId(p, iObj) + Of_CutHandle(pCutSet, pCut) ); }

#define Of_SetForEachCut( pList, pCut, i )          for ( i = 0, pCut = pList + 1; i < pList[0]; i++, pCut += Of_CutSize(pCut) + OF_CUT_EXTRA )
#define Of_ObjForEachCut( pCuts, i, nCuts )         for ( i = 0, i < nCuts; i++ )
#define Of_CutForEachVar( pCut, iVar, i )           for ( i = 0; i < Of_CutSize(pCut) && (iVar = Of_CutVar(pCut,i)); i++ )
#define Of_CutForEachVarFlag( pCut, iVar, Flag, i ) for ( i = 0; i < Of_CutSize(pCut) && (iVar = Of_CutVar(pCut,i)) && ((Flag = Of_CutFlag(pCut,i)), 1); i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Area flow.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Of_ManAreaFlow( Of_Man_t * p )
{
    int AreaUnit = 1000;
    int i, Id, Total = 0;
    Gia_Obj_t * pObj;
    assert( p->pGia->pRefs == NULL );
    Gia_ManCreateRefs( p->pGia );
    Of_ObjSetFlow( p, 0, 0 );
    Gia_ManForEachCiId( p->pGia, Id, i )
        Of_ObjSetFlow( p, Id, 0 );
    Gia_ManForEachAnd( p->pGia, pObj, Id )
        Of_ObjSetFlow( p, Id, (Gia_ObjFanin0(pObj)->Value + Gia_ObjFanin1(pObj)->Value + AreaUnit) / Gia_ObjRefNum(p->pGia, pObj) );
    Gia_ManForEachCo( p->pGia, pObj, i )
        Total += Gia_ObjFanin0(pObj)->Value;
    ABC_FREE( p->pGia->pRefs );
    if ( 1 )
        return;
    printf( "CI = %5d.  ", Gia_ManCiNum(p->pGia) );
    printf( "CO = %5d.  ", Gia_ManCoNum(p->pGia) );
    printf( "And = %8d.  ", Gia_ManAndNum(p->pGia) );
    printf( "Area = %8d.  ", Total/AreaUnit );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Of_Man_t * Of_StoCreate( Gia_Man_t * pGia, Jf_Par_t * pPars )
{
    extern void Mf_ManSetFlowRefs( Gia_Man_t * p, Vec_Int_t * vRefs );
    Of_Man_t * p;
    Vec_Int_t * vFlowRefs;
    int * pRefs = NULL;
    assert( pPars->nCutNum > 1  && pPars->nCutNum <= OF_CUT_MAX );
    assert( pPars->nLutSize > 1 && pPars->nLutSize <= OF_LEAF_MAX );
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
    p = ABC_CALLOC( Of_Man_t, 1 );
    p->clkStart = Abc_Clock();
    p->pGia     = pGia;
    p->pPars    = pPars;
    p->pObjs    = ABC_CALLOC( Of_Obj_t, Gia_ManObjNum(pGia) );
    p->iCur     = 2;
    // other
    Vec_PtrGrow( &p->vPages, 256 );                                    // cut memory
    Vec_IntFill( &p->vCutSets,  Gia_ManObjNum(pGia), 0 );              // cut offsets
    Vec_IntFill( &p->vCutFlows, Gia_ManObjNum(pGia), 0 );              // cut area
    Vec_IntFill( &p->vCutDelays,Gia_ManObjNum(pGia), 0 );              // cut delay
    Vec_IntGrow( &p->vCutRefs,  1000 );                                // cut references
    if ( pPars->fCutMin )
        p->vTtMem = Vec_MemAllocForTT( 6, 0 );          
    // compute area flow
    pRefs = pGia->pRefs; pGia->pRefs = NULL;
    Of_ManAreaFlow( p );
    pGia->pRefs = pRefs;
    return p;
}
void Of_StoDelete( Of_Man_t * p )
{
    Vec_PtrFreeData( &p->vPages );
    Vec_PtrErase( &p->vPages );
    Vec_IntErase( &p->vCutSets );
    Vec_IntErase( &p->vCutFlows );
    Vec_IntErase( &p->vCutDelays );
    Vec_IntErase( &p->vCutRefs );
    ABC_FREE( p->pObjs );
    // matching
    if ( p->pPars->fCutMin )
        Vec_MemHashFree( p->vTtMem );
    if ( p->pPars->fCutMin )
        Vec_MemFree( p->vTtMem );
    ABC_FREE( p );
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Of_CutComputeTruth6( Of_Man_t * p, Of_Cut_t * pCut0, Of_Cut_t * pCut1, int fCompl0, int fCompl1, Of_Cut_t * pCutR, int fIsXor )
{
//    extern int Of_ManTruthCanonicize( word * t, int nVars );
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
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
}
static inline int Of_CutComputeTruthMux6( Of_Man_t * p, Of_Cut_t * pCut0, Of_Cut_t * pCut1, Of_Cut_t * pCutC, int fCompl0, int fCompl1, int fComplC, Of_Cut_t * pCutR )
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
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Of_CutCountBits( word i )
{
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F);
    return (i*(0x0101010101010101))>>56;
}
static inline word Of_CutGetSign( int * pLeaves, int nLeaves )
{
    word Sign = 0; int i; 
    for ( i = 0; i < nLeaves; i++ )
        Sign |= ((word)1) << (pLeaves[i] & 0x3F);
    return Sign;
}
static inline int Of_CutCreateUnit( Of_Cut_t * p, int i )
{
    p->Delay      = 0;
    p->Flow       = 0;
    p->iFunc      = 2;
    p->nLeaves    = 1;
    p->pLeaves[0] = i;
    p->Sign       = ((word)1) << (i & 0x3F);
    return 1;
}
static inline void Of_Cutprintf( Of_Man_t * p, Of_Cut_t * pCut )
{
    int i, nDigits = Abc_Base10Log(Gia_ManObjNum(p->pGia)); 
    printf( "%d  {", pCut->nLeaves );
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        printf( " %*d", nDigits, pCut->pLeaves[i] );
    for ( ; i < (int)p->pPars->nLutSize; i++ )
        printf( " %*s", nDigits, " " );
    printf( "  }   D = %4d  A = %9d  F = %6d  ", 
        pCut->Delay, pCut->Flow, pCut->iFunc );
    if ( p->vTtMem )
        Dau_DsdPrintFromTruth( Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut->iFunc)), pCut->nLeaves );
    else
        printf( "\n" );
}
static inline int Of_ManPrepareCuts( Of_Cut_t * pCuts, Of_Man_t * p, int iObj, int fAddUnit )
{
    if ( Of_ObjHasCuts(p, iObj) )
    {
        Of_Cut_t * pMfCut = pCuts;
        int i, * pCut, * pList = Of_ObjCutSet(p, iObj);
        Of_SetForEachCut( pList, pCut, i )
        {
            pMfCut->Delay   = 0;
            pMfCut->Flow    = 0;
            pMfCut->iFunc   = Of_CutFunc( pCut );
            pMfCut->nLeaves = Of_CutSize( pCut );
            pMfCut->Sign    = Of_CutGetSign( pCut+1, Of_CutSize(pCut) );
            memcpy( pMfCut->pLeaves, pCut+1, sizeof(int) * Of_CutSize(pCut) );
            pMfCut++;
        }
        if ( fAddUnit && pCuts->nLeaves > 1 )
            return pList[0] + Of_CutCreateUnit( pMfCut, iObj );
        return pList[0];
    }
    return Of_CutCreateUnit( pCuts, iObj );
}
static inline int Of_ManSaveCuts( Of_Man_t * p, Of_Cut_t ** pCuts, int nCuts )
{
    int i, * pPlace, iCur, nInts = 1, nCutsNew = 0;
    for ( i = 0; i < nCuts; i++ )
        nInts += pCuts[i]->nLeaves + OF_CUT_EXTRA, nCutsNew++;
    if ( (p->iCur & 0xFFFF) + nInts > 0xFFFF )
        p->iCur = ((p->iCur >> 16) + 1) << 16;
    if ( Vec_PtrSize(&p->vPages) == (p->iCur >> 16) )
        Vec_PtrPush( &p->vPages, ABC_CALLOC(int, (1<<16)) );
    iCur = p->iCur; p->iCur += nInts;
    pPlace = Of_ManCutSet( p, iCur );
    *pPlace++ = nCutsNew;
    for ( i = 0; i < nCuts; i++ )
    {
        *pPlace++ = Of_CutSetBoth( pCuts[i]->nLeaves, pCuts[i]->iFunc );
        memcpy( pPlace, pCuts[i]->pLeaves, sizeof(int) * pCuts[i]->nLeaves );
        pPlace += pCuts[i]->nLeaves;
        memset( pPlace, 0xFF, sizeof(int) * (OF_CUT_EXTRA - 1) );
        pPlace += OF_CUT_EXTRA - 1;
    }
    return iCur;
}
static inline void Of_ManLiftCuts( Of_Man_t * p, int iObj )
{
    int i, k, * pCut, * pList = Of_ObjCutSet(p, iObj);
    assert( Of_ObjHasCuts(p, iObj) );
    Of_SetForEachCut( pList, pCut, i )
    {
        for ( k = 1; k <= Of_CutSize(pCut); k++ )
            pCut[k] = Abc_Var2Lit(pCut[k], 0);
    }
}
static inline void Of_CutPrint( int * pCut )
{
    int k, iVar;
    printf( "Cut with %d inputs and function %3d : { ", Of_CutSize(pCut), Of_CutFunc(pCut) == OF_NO_FUNC ? 0 : Of_CutFunc(pCut) );
    Of_CutForEachVar( pCut, iVar, k )
        printf( "%d ", iVar );
    printf( "}\n" );
}

/**Function*************************************************************

  Synopsis    [Check correctness of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Of_CutCheck( Of_Cut_t * pBase, Of_Cut_t * pCut ) // check if pCut is contained in pBase
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
static inline int Of_SetCheckArray( Of_Cut_t ** ppCuts, int nCuts )
{
    Of_Cut_t * pCut0, * pCut1; 
    int i, k, m, n, Value;
    assert( nCuts > 0 );
    for ( i = 0; i < nCuts; i++ )
    {
        pCut0 = ppCuts[i];
        assert( pCut0->nLeaves <= OF_LEAF_MAX );
        assert( pCut0->Sign == Of_CutGetSign(pCut0->pLeaves, pCut0->nLeaves) );
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
            Value = Of_CutCheck( pCut0, pCut1 );
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
static inline int Of_CutMergeOrder( Of_Cut_t * pCut0, Of_Cut_t * pCut1, Of_Cut_t * pCut, int nLutSize )
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
        pCut->iFunc = OF_NO_FUNC;
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
    pCut->iFunc = OF_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;

FlushCut1:
    if ( c + nSize1 > nLutSize + k ) return 0;
    while ( k < nSize1 )
        pC[c++] = pC1[k++];
    pCut->nLeaves = c;
    pCut->iFunc = OF_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;
}
static inline int Of_CutMergeOrderMux( Of_Cut_t * pCut0, Of_Cut_t * pCut1, Of_Cut_t * pCut2, Of_Cut_t * pCut, int nLutSize )
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
    pCut->iFunc = OF_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign | pCut2->Sign;
    return 1;
}
static inline int Of_SetCutIsContainedOrder( Of_Cut_t * pBase, Of_Cut_t * pCut ) // check if pCut is contained in pBase
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
static inline int Of_SetLastCutIsContained( Of_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[i]->nLeaves <= pCuts[nCuts]->nLeaves && (pCuts[i]->Sign & pCuts[nCuts]->Sign) == pCuts[i]->Sign && Of_SetCutIsContainedOrder(pCuts[nCuts], pCuts[i]) )
            return 1;
    return 0;
}
static inline int Of_SetLastCutContainsArea( Of_Cut_t ** pCuts, int nCuts )
{
    int i, k, fChanges = 0;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[nCuts]->nLeaves < pCuts[i]->nLeaves && (pCuts[nCuts]->Sign & pCuts[i]->Sign) == pCuts[nCuts]->Sign && Of_SetCutIsContainedOrder(pCuts[i], pCuts[nCuts]) )
            pCuts[i]->nLeaves = OF_NO_LEAF, fChanges = 1;
    if ( !fChanges )
        return nCuts;
    for ( i = k = 0; i <= nCuts; i++ )
    {
        if ( pCuts[i]->nLeaves == OF_NO_LEAF )
            continue;
        if ( k < i )
            ABC_SWAP( Of_Cut_t *, pCuts[k], pCuts[i] );
        k++;
    }
    return k - 1;
}
static inline int Of_CutCompareArea( Of_Cut_t * pCut0, Of_Cut_t * pCut1 )
{
    if ( pCut0->Delay   < pCut1->Delay   )  return -1;
    if ( pCut0->Delay   > pCut1->Delay   )  return  1;
    if ( pCut0->Flow    < pCut1->Flow    )  return -1;
    if ( pCut0->Flow    > pCut1->Flow    )  return  1;
    if ( pCut0->nLeaves < pCut1->nLeaves )  return -1;
    if ( pCut0->nLeaves > pCut1->nLeaves )  return  1;
    return 0;
}
static inline void Of_SetSortByArea( Of_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = nCuts; i > 0; i-- )
    {
        if ( Of_CutCompareArea(pCuts[i - 1], pCuts[i]) < 0 )//!= 1 )
            return;
        ABC_SWAP( Of_Cut_t *, pCuts[i - 1], pCuts[i] );
    }
}
static inline int Of_SetAddCut( Of_Cut_t ** pCuts, int nCuts, int nCutNum )
{
    if ( nCuts == 0 )
        return 1;
    nCuts = Of_SetLastCutContainsArea(pCuts, nCuts);
    Of_SetSortByArea( pCuts, nCuts );
    return Abc_MinInt( nCuts + 1, nCutNum - 1 );
}
static inline int Of_CutArea( Of_Man_t * p, int nLeaves )
{
    if ( nLeaves < 2 )
        return 0;
    return nLeaves + p->pPars->nAreaTuner;
}
static inline void Of_CutParams( Of_Man_t * p, Of_Cut_t * pCut, int nGiaRefs )
{
    int i, nLeaves = pCut->nLeaves; 
    assert( nLeaves <= p->pPars->nLutSize );
    pCut->Delay = 0;
    pCut->Flow  = 0;
    for ( i = 0; i < nLeaves; i++ )
    {
        pCut->Delay = Abc_MaxInt( pCut->Delay, Of_ObjCutDelay(p, pCut->pLeaves[i]) );
        pCut->Flow += Of_ObjCutFlow(p, pCut->pLeaves[i]);
    }
    pCut->Delay += (int)(nLeaves > 1);
    pCut->Flow = (pCut->Flow + 100 * Of_CutArea(p, nLeaves)) / (nGiaRefs ? nGiaRefs : 1);
}
void Of_ObjMergeOrder( Of_Man_t * p, int iObj )
{
    Of_Cut_t pCuts0[OF_CUT_MAX], pCuts1[OF_CUT_MAX], pCuts[OF_CUT_MAX], * pCutsR[OF_CUT_MAX];
    Gia_Obj_t * pObj = Gia_ManObj(p->pGia, iObj);
    int nGiaRefs = 2*Gia_ObjRefNumId(p->pGia, iObj);
    int nLutSize = p->pPars->nLutSize;
    int nCutNum  = p->pPars->nCutNum;
    int nCuts0   = Of_ManPrepareCuts(pCuts0, p, Gia_ObjFaninId0(pObj, iObj), 1);
    int nCuts1   = Of_ManPrepareCuts(pCuts1, p, Gia_ObjFaninId1(pObj, iObj), 1);
    int fComp0   = Gia_ObjFaninC0(pObj);
    int fComp1   = Gia_ObjFaninC1(pObj);
    int iSibl    = Gia_ObjSibl(p->pGia, iObj);
    Of_Cut_t * pCut0, * pCut1, * pCut0Lim = pCuts0 + nCuts0, * pCut1Lim = pCuts1 + nCuts1;
    int i, nCutsR = 0;
    assert( !Gia_ObjIsBuf(pObj) );
    for ( i = 0; i < nCutNum; i++ )
        pCutsR[i] = pCuts + i;
    if ( iSibl )
    {
        Of_Cut_t pCuts2[OF_CUT_MAX];
        Gia_Obj_t * pObjE = Gia_ObjSiblObj(p->pGia, iObj);
        int fCompE = Gia_ObjPhase(pObj) ^ Gia_ObjPhase(pObjE);
        int nCuts2 = Of_ManPrepareCuts(pCuts2, p, iSibl, 0);
        Of_Cut_t * pCut2, * pCut2Lim = pCuts2 + nCuts2;
        for ( pCut2 = pCuts2; pCut2 < pCut2Lim; pCut2++ )
        {
            *pCutsR[nCutsR] = *pCut2;
            if ( p->pPars->fCutMin )
                pCutsR[nCutsR]->iFunc = Abc_LitNotCond( pCutsR[nCutsR]->iFunc, fCompE );
            Of_CutParams( p, pCutsR[nCutsR], nGiaRefs );
            nCutsR = Of_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    if ( Gia_ObjIsMuxId(p->pGia, iObj) )
    {
        Of_Cut_t pCuts2[OF_CUT_MAX];
        int nCuts2  = Of_ManPrepareCuts(pCuts2, p, Gia_ObjFaninId2(p->pGia, iObj), 1);
        int fComp2  = Gia_ObjFaninC2(p->pGia, pObj);
        Of_Cut_t * pCut2, * pCut2Lim = pCuts2 + nCuts2;
        p->CutCount[0] += nCuts0 * nCuts1 * nCuts2;
        for ( pCut0 = pCuts0; pCut0 < pCut0Lim; pCut0++ )
        for ( pCut1 = pCuts1; pCut1 < pCut1Lim; pCut1++ )
        for ( pCut2 = pCuts2; pCut2 < pCut2Lim; pCut2++ )
        {
            if ( Of_CutCountBits(pCut0->Sign | pCut1->Sign | pCut2->Sign) > nLutSize )
                continue;
            p->CutCount[1]++; 
            if ( !Of_CutMergeOrderMux(pCut0, pCut1, pCut2, pCutsR[nCutsR], nLutSize) )
                continue;
            if ( Of_SetLastCutIsContained(pCutsR, nCutsR) )
                continue;
            p->CutCount[2]++;
            if ( p->pPars->fCutMin && Of_CutComputeTruthMux6(p, pCut0, pCut1, pCut2, fComp0, fComp1, fComp2, pCutsR[nCutsR]) )
                pCutsR[nCutsR]->Sign = Of_CutGetSign(pCutsR[nCutsR]->pLeaves, pCutsR[nCutsR]->nLeaves);
            Of_CutParams( p, pCutsR[nCutsR], nGiaRefs );
            nCutsR = Of_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    else
    {
        int fIsXor = Gia_ObjIsXor(pObj);
        p->CutCount[0] += nCuts0 * nCuts1;
        for ( pCut0 = pCuts0; pCut0 < pCut0Lim; pCut0++ )
        for ( pCut1 = pCuts1; pCut1 < pCut1Lim; pCut1++ )
        {
            if ( (int)(pCut0->nLeaves + pCut1->nLeaves) > nLutSize && Of_CutCountBits(pCut0->Sign | pCut1->Sign) > nLutSize )
                continue;
            p->CutCount[1]++; 
            if ( !Of_CutMergeOrder(pCut0, pCut1, pCutsR[nCutsR], nLutSize) )
                continue;
            if ( Of_SetLastCutIsContained(pCutsR, nCutsR) )
                continue;
            p->CutCount[2]++;
            if ( p->pPars->fCutMin && Of_CutComputeTruth6(p, pCut0, pCut1, fComp0, fComp1, pCutsR[nCutsR], fIsXor) )
                pCutsR[nCutsR]->Sign = Of_CutGetSign(pCutsR[nCutsR]->pLeaves, pCutsR[nCutsR]->nLeaves);
            Of_CutParams( p, pCutsR[nCutsR], nGiaRefs );
            nCutsR = Of_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    // debug printout
    if ( 0 )
    {
        printf( "*** Obj = %d\n", iObj );
        for ( i = 0; i < nCutsR; i++ )
            Of_Cutprintf( p, pCutsR[i] );
        printf( "\n" );
    } 
    // verify
    assert( nCutsR > 0 && nCutsR < nCutNum );
    //assert( Of_SetCheckArray(pCutsR, nCutsR) );
    // store the cutset
    Of_ObjSetCutFlow( p, iObj, pCutsR[0]->Flow );
    Of_ObjSetCutDelay( p, iObj, pCutsR[0]->Delay );
    *Vec_IntEntryP(&p->vCutSets, iObj) = Of_ManSaveCuts(p, pCutsR, nCutsR);
    p->CutCount[3] += nCutsR;
}
void Of_ManComputeCuts( Of_Man_t * p )
{
    Gia_Obj_t * pObj; int i, iFanin;
    Gia_ManForEachAnd( p->pGia, pObj, i )
        if ( Gia_ObjIsBuf(pObj) )
        {
            iFanin = Gia_ObjFaninId0(pObj, i);
            Of_ObjSetCutFlow( p, i,  Of_ObjCutFlow(p, iFanin) );
            Of_ObjSetCutDelay( p, i, Of_ObjCutDelay(p, iFanin) );
        }
        else
            Of_ObjMergeOrder( p, i );
    Gia_ManForEachAnd( p->pGia, pObj, i )
        if ( !Gia_ObjIsBuf(pObj) )
            Of_ManLiftCuts( p, i );
}




/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Of_ManPrintStats( Of_Man_t * p, char * pTitle )
{
    if ( !p->pPars->fVerbose )
        return;
    printf( "%s :  ", pTitle );
    printf( "Delay =%8.2f ", Of_Int2Flt((int)p->pPars->Delay) );
    printf( "Area =%8d  ",   (int)p->pPars->Area );
    printf( "Edge =%9d  ",   (int)p->pPars->Edge );
    Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
    fflush( stdout );
}
void Of_ManPrintInit( Of_Man_t * p )
{
    int nChoices;
    if ( !p->pPars->fVerbose )
        return;
    printf( "LutSize = %d  ", p->pPars->nLutSize );
    printf( "CutNum = %d  ",  p->pPars->nCutNum );
    printf( "Iter = %d  ",    p->pPars->nRounds + p->pPars->nRoundsEla );
    printf( "Coarse = %d   ", p->pPars->fCoarsen );
    if ( p->pPars->fCutMin )
    printf( "Funcs = %d  ",   Vec_MemEntryNum(p->vTtMem) );
    nChoices = Gia_ManChoiceNum( p->pGia );
    if ( nChoices )
    printf( "Choices = %d  ", nChoices );
    printf( "\n" );
    printf( "Computing cuts...\r" );
    fflush( stdout );
}
void Of_ManPrintQuit( Of_Man_t * p )
{
    float MemGia  = Gia_ManMemory(p->pGia) / (1<<20);
    float MemMan  = 1.0 * sizeof(Of_Obj_t) * Gia_ManObjNum(p->pGia) / (1<<20);
    float MemCuts = 1.0 * sizeof(int) * (1 << 16) * Vec_PtrSize(&p->vPages) / (1<<20);
    float MemTt   = p->vTtMem ? Vec_MemMemory(p->vTtMem) / (1<<20) : 0;
    if ( p->CutCount[0] == 0 )
        p->CutCount[0] = 1;
    if ( !p->pPars->fVerbose )
        return;
    printf( "CutPair = %.0f  ",         p->CutCount[0] );
    printf( "Merge = %.0f (%.1f)  ",    p->CutCount[1], 1.0*p->CutCount[1]/Gia_ManAndNum(p->pGia) );
    printf( "Eval = %.0f (%.1f)  ",     p->CutCount[2], 1.0*p->CutCount[2]/Gia_ManAndNum(p->pGia) );
    printf( "Cut = %.0f (%.1f)  ",      p->CutCount[3], 1.0*p->CutCount[3]/Gia_ManAndNum(p->pGia) );
//    printf( "Use = %.0f (%.1f)  ",      p->CutCount[4], 1.0*p->CutCount[4]/Gia_ManAndNum(p->pGia) );
//    printf( "Mat = %.0f (%.1f)  ",      p->CutCount[5], 1.0*p->CutCount[5]/Gia_ManAndNum(p->pGia) );
//    printf( "Equ = %d (%.2f %%)  ",     p->nCutUseAll,  100.0*p->nCutUseAll /p->CutCount[0] );
    printf( "\n" );
    printf( "Gia = %.2f MB  ",          MemGia );
    printf( "Man = %.2f MB  ",          MemMan ); 
    printf( "Cut = %.2f MB   ",         MemCuts );
    if ( p->pPars->fCutMin )
    printf( "TT = %.2f MB  ",           MemTt ); 
    printf( "Total = %.2f MB   ",       MemGia + MemMan + MemCuts + MemTt ); 
//    printf( "\n" );
    Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
    fflush( stdout );
}


/**Function*************************************************************

  Synopsis    [Technology mappping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
static inline int Of_ManComputeForwardCut( Of_Man_t * p, int iObj, int * pCut )
{
    int k, iVar, Delay = 0, Area = Of_CutArea(p, Of_CutSize(pCut));
    int DelayLut1 = p->pPars->nDelayLut1;
    Of_CutForEachVar( pCut, iVar, k )
    {
        Delay = Abc_MaxInt( Delay, Of_ObjDelay1(p, iVar) + DelayLut1 );
        if ( p->Iter )
            Area += Of_ObjRefNum(p, iVar) ? 0 : Of_ObjFlow(p, iVar);
    }
    Of_CutSetDelay1( pCut, Delay );
    if ( p->Iter )
        Of_CutSetAreaFlow( pCut, Area );
    return Delay;
}
static inline void Of_ManComputeForwardObj( Of_Man_t * p, int iObj )
{
    int Delay1 = ABC_INFINITY, Area1 = ABC_INFINITY;
    int * pList = Of_ObjCutSet(p, iObj);
    int i, * pCut, * pCutMin = NULL, * pCutMin2 = NULL;
    // compute cut arrivals
    Of_SetForEachCut( pList, pCut, i )
    {
        int Delay1This = Of_ManComputeForwardCut(p, iObj, pCut);
        if ( Delay1 > Delay1This )
        {
            Delay1  = Delay1This;
            pCutMin = pCut;
        }
        if ( p->Iter && Area1 > Of_CutAreaFlow(pCut) )
        {
            Area1    = Of_CutAreaFlow(pCut);
            pCutMin2 = pCut;
        }
    }
    // if mapping is present, set object arrival equal to cut arrival
    if ( Of_ObjRefNum(p, iObj) )
    {
        pCutMin = Of_ObjCutBestP(p, iObj);
        Delay1 = Of_CutDelay1( pCutMin );
        Of_ObjSetDelay1( p, iObj, Delay1 );
        if ( p->Iter )
            Of_ObjSetFlow( p, iObj, Of_CutAreaFlow(pCutMin) );
    }
    else
    {
        if ( p->Iter == 0 )
        {
            Of_ObjSetCutBestP( p, pList, iObj, pCutMin );
            Of_ObjSetDelay1( p, iObj, Delay1 );
        }
        else
        {
            Of_ObjSetCutBestP( p, pList, iObj, pCutMin2 );
            Of_ObjSetDelay1( p, iObj, Of_CutDelay1(pCutMin2) );
            Of_ObjSetFlow( p, iObj, Of_CutAreaFlow(pCutMin2) );
        }
    }
}
*/

/*
int * Of_CutReferChooseCut( Of_Man_t * p, int Var, int Required, int fSetBest )
{
    int i, CostMin = ABC_INFINITY;
    int * pCutMin = NULL, * pList = Of_ObjCutSet(p, Var);
    int * pCut = Of_ObjCutBestP(p, Var);
    assert( Of_CutDelay1(pCut) <= Required );
//    return pCut;
    // choose cut with smaller area
    Of_SetForEachCut( pList, pCut, i )
    {
        if ( Of_CutDelay1(pCut) > Required )
            continue;
        if ( CostMin > Of_CutAreaFlow(pCut) )
        {
            CostMin = Of_CutAreaFlow(pCut);
            pCutMin = pCut;
        }
    }
    assert( pCutMin != NULL );
    assert( Of_CutDelay1(pCutMin) <= Required );
    if ( fSetBest )
        Of_ObjSetCutBestP( p, pList, Var, pCutMin );
    return pCutMin;
}
int Of_CutRef2_rec( Of_Man_t * p, int * pCut, int Required, int fSetBest )
{
    int i, Var, Count = Of_CutArea(p, Of_CutSize(pCut));
    assert( Of_CutDelay1(pCut) <= Required );
    Required -= p->pPars->nDelayLut1;
    Of_CutForEachVar( pCut, Var, i )
    {
        if ( !Of_ObjCutBest(p, Var) )
            continue;
        if ( !fSetBest )
            Vec_IntPush( &p->vCutRefs, Var );
        if ( Of_ObjRefInc(p, Var) )
            continue;
        Count += Of_CutRef2_rec( p, Of_CutReferChooseCut(p, Var, Required, fSetBest), Required, fSetBest );
    }
    return Count;
}
static inline int Of_CutAreaDerefed2( Of_Man_t * p, int * pCut, int Required )
{
    int Ela1, i, iObj;
    assert( Vec_IntSize(&p->vCutRefs) == 0 );
    Ela1 = Of_CutRef2_rec( p, pCut, Required, 0 );
    Vec_IntForEachEntry( &p->vCutRefs, iObj, i )
        Of_ObjRefDec(p, iObj);
    Vec_IntClear( &p->vCutRefs );
    return Ela1;
}
*/


static inline int Of_ManComputeForwardCut( Of_Man_t * p, int iObj, int * pCut )
{
    int k, iVar, Delay = 0;
    int DelayLut1 = p->pPars->nDelayLut1;
    Of_CutForEachVar( pCut, iVar, k )
        Delay = Abc_MaxInt( Delay, Of_ObjDelay1(p, iVar) + DelayLut1 );
    Of_CutSetDelay1( pCut, Delay );
    return Delay;
}
static inline int Of_ManComputeForwardCutArea( Of_Man_t * p, int iObj, int * pCut )
{
    int k, iVar, Area = 100 * Of_CutArea(p, Of_CutSize(pCut));
    Of_CutForEachVar( pCut, iVar, k )
         Area += Of_ObjFlow(p, iVar);
    return Area / Abc_MaxInt(1, Of_ObjRefNum(p, iObj));
}
static inline void Of_ManComputeForwardObj( Of_Man_t * p, int iObj )
{
    int Delay1 = ABC_INFINITY;
    int i, * pCut, * pCutMin = NULL, * pList = Of_ObjCutSet(p, iObj);
    // compute cut arrivals
    Of_SetForEachCut( pList, pCut, i )
    {
        int Delay1This = Of_ManComputeForwardCut(p, iObj, pCut);
        if ( Delay1 > Delay1This )
        {
            Delay1  = Delay1This;
            pCutMin = pCut;
        }
    }
    // if mapping is present, set object arrival equal to cut arrival
    if ( Of_ObjRefNum(p, iObj) )
        pCutMin = Of_ObjCutBestP(p, iObj);
    Of_ObjSetCutBestP( p, pList, iObj, pCutMin );
    Of_ObjSetDelay1( p, iObj, Of_CutDelay1(pCutMin) );
    if ( p->Iter )
        Of_ObjSetFlow( p, iObj, Of_ManComputeForwardCutArea(p, iObj, pCutMin) );
}
void Of_ManComputeForward1( Of_Man_t * p )
{
    Gia_Obj_t * pObj; int i;
    Gia_ManForEachAnd( p->pGia, pObj, i )
        if ( Gia_ObjIsBuf(pObj) )
            Of_ObjSetDelay1( p, i, Of_ObjDelay1(p, Gia_ObjFaninId0(pObj, i)) );
        else
            Of_ManComputeForwardObj( p, i );
}


int Of_CutRef_rec( Of_Man_t * p, int * pCut )
{
    int i, Var, Count = (p->Iter & 1) ? 1 : Of_CutArea(p, Of_CutSize(pCut));
    Of_CutForEachVar( pCut, Var, i )
        if ( Of_ObjCutBest(p, Var) && !Of_ObjRefInc(p, Var) )
            Count += Of_CutRef_rec( p, Of_ObjCutBestP(p, Var) );
    return Count;
}
int Of_CutDeref_rec( Of_Man_t * p, int * pCut )
{
    int i, Var, Count = (p->Iter & 1) ? 1 : Of_CutArea(p, Of_CutSize(pCut));
    Of_CutForEachVar( pCut, Var, i )
        if ( Of_ObjCutBest(p, Var) && !Of_ObjRefDec(p, Var) )
            Count += Of_CutDeref_rec( p, Of_ObjCutBestP(p, Var) );
    return Count;
}
static inline int Of_CutAreaDerefed( Of_Man_t * p, int * pCut )
{
    int Ela1 = Of_CutRef_rec( p, pCut );
    int Ela2 = Of_CutDeref_rec( p, pCut );
    assert( Ela1 == Ela2 );
    return Ela1;
}

int Of_CutRef2_rec( Of_Man_t * p, int * pCut )
{
    int i, Var, Count = (p->Iter & 1) ? 1 : Of_CutArea(p, Of_CutSize(pCut));
    Of_CutForEachVar( pCut, Var, i )
    {
        if ( !Of_ObjCutBest(p, Var) )
            continue;
        Vec_IntPush( &p->vCutRefs, Var );
        if ( Of_ObjRefInc(p, Var) )
            continue;
        Count += Of_CutRef2_rec( p, Of_ObjCutBestP(p, Var) );
    }
    return Count;
}
static inline int Of_CutAreaDerefed2( Of_Man_t * p, int * pCut )
{
    int Ela1, i, iObj;
    assert( Vec_IntSize(&p->vCutRefs) == 0 );
    Ela1 = Of_CutRef2_rec( p, pCut );
    Vec_IntForEachEntry( &p->vCutRefs, iObj, i )
        Of_ObjRefDec(p, iObj);
    Vec_IntClear( &p->vCutRefs );
    return Ela1;
}


static inline void Of_ManComputeForwardObj2( Of_Man_t * p, int iObj )
{
    int Delay, Required = Of_ObjRequired(p, iObj);
    int AreaBef = 0, AreaAft = 0, Area, AreaMin = ABC_INFINITY;
    int k, * pCut, * pCutMin = NULL, * pList = Of_ObjCutSet(p, iObj);
    if ( Of_ObjRefNum(p, iObj) )
        AreaBef = Of_CutDeref_rec( p, Of_ObjCutBestP(p, iObj) );        
    Of_SetForEachCut( pList, pCut, k )
    {
        Delay = Of_ManComputeForwardCut(p, iObj, pCut);
        if ( Delay > Required )
            continue;
        Area = Of_CutAreaDerefed2( p, pCut );
        if ( AreaMin > Area )
        {
            AreaMin = Area;
            pCutMin = pCut;
        }
    }
    assert( pCutMin != NULL );
    Of_ObjSetCutBestP( p, pList, iObj, pCutMin );
    if ( Of_ObjRefNum(p, iObj) )
        AreaAft = Of_CutRef_rec( p, pCutMin );
    assert( AreaAft <= AreaBef );
    Delay = Of_CutDelay1(pCutMin);
    assert( Delay <= Required );
    Of_ObjSetDelay1( p, iObj, Delay );
}
void Of_ManComputeForward2( Of_Man_t * p )
{
    Gia_Obj_t * pObj; int i;
    Gia_ManForEachAnd( p->pGia, pObj, i )
        if ( Gia_ObjIsBuf(pObj) )
            Of_ObjSetDelay1( p, i, Of_ObjDelay1(p, Gia_ObjFaninId0(pObj, i)) );
        else
            Of_ManComputeForwardObj2( p, i );
}


static inline int Of_ManComputeOutputRequired( Of_Man_t * p, int fCleanRefs )
{
    int i, Id, Delay = 0;
    for ( i = 0; i < Gia_ManObjNum(p->pGia); i++ )
    {
        Of_ObjSetRequired( p, i, ABC_INFINITY );
        if ( fCleanRefs )
            Of_ObjSetRefNum( p, i, 0 );
    }
    Gia_ManForEachCoDriverId( p->pGia, Id, i )
        Delay = Abc_MaxInt( Delay, Of_ObjDelay1(p, Id) );
    Gia_ManForEachCoDriverId( p->pGia, Id, i )
    {
        Of_ObjUpdateRequired( p, Id, Delay );
        if ( fCleanRefs )
            Of_ObjRefInc( p, Id );
    }
    if ( p->pPars->Delay && p->pPars->Delay < Delay )
        printf( "Error: Delay violation.\n" );
    p->pPars->Delay = Delay;
    return Delay;
}
static inline int Of_ManComputeBackwardCut( Of_Man_t * p, int * pCut )
{
    int k, iVar, Cost = 0;
    Of_CutForEachVar( pCut, iVar, k )
        if ( !Of_ObjRefNum(p, iVar) )
            Cost += Of_ObjFlow( p, iVar );
    return Cost;
}
void Of_ManComputeBackward1( Of_Man_t * p )
{
    Gia_Obj_t * pObj; 
    int DelayLut1 = p->pPars->nDelayLut1;
    int i, k, iVar, * pList, * pCut, * pCutMin;
    Of_ManComputeOutputRequired( p, 1 );
    // compute area and edges
    p->pPars->Area = p->pPars->Edge = 0;
    Gia_ManForEachAndReverse( p->pGia, pObj, i )
    {
        int CostMin, Cost, Required = Of_ObjRequired(p, i);
        if ( Gia_ObjIsBuf(pObj) )
        {
            int FaninId = Gia_ObjFaninId0(pObj, i);
            Of_ObjUpdateRequired( p, FaninId, Required );
            Of_ObjRefInc( p, FaninId );
            continue;
        }
        if ( !Of_ObjRefNum(p, i) )
            continue;
        // select the best cut
        pCutMin = NULL;
        CostMin = ABC_INFINITY;
        pList = Of_ObjCutSet( p, i );
        Of_SetForEachCut( pList, pCut, k )
        {
            if ( Of_CutDelay1(pCut) > Required )
                continue;
            Cost = Of_ManComputeBackwardCut( p, pCut );
            if ( CostMin > Cost )
            {
                CostMin = Cost;
                pCutMin = pCut;
            }
        }
        // the cut is selected
        assert( pCutMin != NULL );
        Of_ObjSetCutBestP( p, pList, i, pCutMin );
        Of_CutForEachVar( pCutMin, iVar, k )
        {
            Of_ObjUpdateRequired( p, iVar, Required - DelayLut1 );
            Of_ObjRefInc( p, iVar );
        }
        // update parameters
        p->pPars->Edge += Of_CutSize(pCutMin);
        p->pPars->Area++;
    }
}
void Of_ManComputeBackward2( Of_Man_t * p )
{
    Gia_Obj_t * pObj; 
    int DelayLut1 = p->pPars->nDelayLut1;
    int i, k, iVar, * pCutMin;
    Of_ManComputeOutputRequired( p, 0 );
    // compute area and edges
    p->pPars->Area = p->pPars->Edge = 0;
    Gia_ManForEachAndReverse( p->pGia, pObj, i )
    {
        int Required = Of_ObjRequired(p, i);
        if ( Gia_ObjIsBuf(pObj) )
        {
            int FaninId = Gia_ObjFaninId0(pObj, i);
            Of_ObjUpdateRequired( p, FaninId, Required );
            continue;
        }
        if ( !Of_ObjRefNum(p, i) )
            continue;
        // lookup for the cut
        pCutMin = Of_ObjCutBestP( p, i );
        Of_CutForEachVar( pCutMin, iVar, k )
            Of_ObjUpdateRequired( p, iVar, Required - DelayLut1 );
        // update parameters
        p->pPars->Edge += Of_CutSize(pCutMin);
        p->pPars->Area++;
    }
}
void Of_ManComputeBackward3( Of_Man_t * p )
{
    Gia_Obj_t * pObj; 
    int DelayLut1 = p->pPars->nDelayLut1;
    int i, k, iVar, * pList, * pCut, * pCutMin;
    int AreaBef = 0, AreaAft = 0;
    Of_ManComputeOutputRequired( p, 0 );
    // compute area and edges
    p->pPars->Area = p->pPars->Edge = 0;
    Gia_ManForEachAndReverse( p->pGia, pObj, i )
    {
        int CostMin, Cost, Required = Of_ObjRequired(p, i);
        if ( Gia_ObjIsBuf(pObj) )
        {
            int FaninId = Gia_ObjFaninId0(pObj, i);
            Of_ObjUpdateRequired( p, FaninId, Required );
            continue;
        }
        if ( !Of_ObjRefNum(p, i) )
            continue;
        // deref best cut
        AreaBef = Of_CutDeref_rec( p, Of_ObjCutBestP(p, i) );        
        // select the best cut
        pCutMin = NULL;
        CostMin = ABC_INFINITY;
        pList = Of_ObjCutSet( p, i );
        Of_SetForEachCut( pList, pCut, k )
        {
            if ( Of_CutDelay1(pCut) > Required )
                continue;
            Cost = Of_CutAreaDerefed2( p, pCut );
            if ( CostMin > Cost )
            {
                CostMin = Cost;
                pCutMin = pCut;
            }
        }
        // the cut is selected
        assert( pCutMin != NULL );
        Of_ObjSetCutBestP( p, pList, i, pCutMin );
        Of_CutForEachVar( pCutMin, iVar, k )
            Of_ObjUpdateRequired( p, iVar, Required - DelayLut1 );
        // ref best cut
        AreaAft = Of_CutRef_rec( p, pCutMin );
        assert( AreaAft <= AreaBef );
        // update parameters
        p->pPars->Edge += Of_CutSize(pCutMin);
        p->pPars->Area++;
    }
}

/**Function*************************************************************

  Synopsis    [Technology mappping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Of_ManComputeForwardDirconCut( Of_Man_t * p, int iObj, int * pCut, int * pDelay1, int * pDelay2 )
{
    // Delay1 - main delay;  Delay2 - precomputed LUT delay in terms of Delay1 for the fanins
    int Delays[6], Perm[6] = {0, 1, 2, 3, 4, 5}; 
    int DelayLut1 = p->pPars->nDelayLut1;
    int DelayLut2 = p->pPars->nDelayLut2;
    int nSize = Of_CutSize(pCut);
    int k, iVar, Flag, SlowCon, Delay, DelayAfter, fDirConWorks;
    Of_CutForEachVar( pCut, iVar, k )
    {
        Delays[k] = Of_ObjDelay1(p, iVar) + DelayLut1;
//        printf( "%3d%s ", iVar, Flag ? "*" : " " );
    }
    for ( ; k < p->pPars->nLutSize; k++ )
    {
        Delays[k] = -ABC_INFINITY;
//        printf( "     " );
    }
    Vec_IntSelectSortCost2Reverse( Perm, nSize, Delays );
    assert( nSize < 2 || Delays[0] >= Delays[nSize-1] );
    assert( Delays[0] >= 0 && Delays[nSize-1] >= 0 );
    // consider speedup due to dircons
    fDirConWorks = 1;
    *pDelay1 = *pDelay2 = 0;
    SlowCon = p->pPars->nFastEdges < nSize ? Delays[p->pPars->nFastEdges] : 0;
    for ( k = 0; k < nSize; k++ )
    {
        // use dircon if the following is true
        // - the input is eligible for dircon (does not exceed the limit)
        // - there is an expected gain in delay, compared the largest delay without dircon
        // - the dircon delay is indeed lower than the largest delay without dircon
        // - all previous dircons worked out well
        // - the node is an AND-gate 
        iVar = Of_CutVar( pCut, Perm[k] );
        assert( Delays[k] == Of_ObjDelay1(p, iVar) + DelayLut1 );
        DelayAfter = Of_ObjDelay2(p, iVar) + DelayLut2;
        if ( k < p->pPars->nFastEdges && Delays[k] > SlowCon && DelayAfter < Delays[k] && fDirConWorks && Gia_ObjIsAndNotBuf(Gia_ManObj(p->pGia, iVar)) )
        {
            Delay = DelayAfter;
            Of_CutSetFlag( pCut, Perm[k], 1 );
        }
        else
        {
            Delay = Delays[k];// + DelayLut2;
            Of_CutSetFlag( pCut, Perm[k], 0 );
            fDirConWorks = 0;
        }
        *pDelay1 = Abc_MaxInt( *pDelay1, Delay );
        *pDelay2 = Abc_MaxInt( *pDelay2, Delays[k] );
    }
//    printf( "   %5.2f",   Of_Int2Flt(*pDelay1) );
//    printf( "   %5.2f\n", Of_Int2Flt(*pDelay2) );
    // do not use the structure if simple LUT is better
    if ( *pDelay1 > *pDelay2 )
    {
        for ( k = 0; k < nSize; k++ )
            Of_CutSetFlag( pCut, k, 0 );
        *pDelay1 = *pDelay2;
    }
    assert( *pDelay1 <= *pDelay2 );
    Of_CutSetDelay1( pCut, *pDelay1 );
    Of_CutSetDelay2( pCut, *pDelay2 );
    // verify
    Of_CutForEachVarFlag( pCut, iVar, Flag, k )
    {
        if ( Flag )
            assert( Of_ObjDelay2(p, iVar) + DelayLut2 <= *pDelay1 );
        else
            assert( Of_ObjDelay1(p, iVar) + DelayLut1 <= *pDelay1 );
        assert( Of_ObjDelay1(p, iVar) + DelayLut1 <= *pDelay2 );
    }
}
int Of_ManComputeForwardDirconObj( Of_Man_t * p, int iObj )
{
    int Delay1 = ABC_INFINITY, Delay2 = ABC_INFINITY;
    int i, * pCut, * pCutMin = NULL, * pCutMin2 = NULL, * pList = Of_ObjCutSet(p, iObj);
    Of_SetForEachCut( pList, pCut, i )
    {
        int Delay1This, Delay2This;
        Of_ManComputeForwardDirconCut( p, iObj, pCut, &Delay1This, &Delay2This );
        if ( Delay1 > Delay1This )
            pCutMin = pCut;
        if ( Delay2 > Delay2This )
            pCutMin2 = pCut;
        Delay1 = Abc_MinInt( Delay1, Delay1This );
        Delay2 = Abc_MinInt( Delay2, Delay2This );
    }
    Of_ObjSetDelay1( p, iObj, Delay1 );
    Of_ObjSetDelay2( p, iObj, Delay2 );
    Of_ObjSetCutBestP( p, pList, iObj, pCutMin );
    Of_ObjSetCutBestP2( p, pList, iObj, pCutMin2 );
    return Delay1;
}
void Of_ManComputeForwardDircon1( Of_Man_t * p )
{
    Gia_Obj_t * pObj; int i;
    Gia_ManForEachAnd( p->pGia, pObj, i )
        if ( Gia_ObjIsBuf(pObj) )
        {
            Of_ObjSetDelay1( p, i, Of_ObjDelay1(p, Gia_ObjFaninId0(pObj, i)) );
            Of_ObjSetDelay2( p, i, Of_ObjDelay2(p, Gia_ObjFaninId0(pObj, i)) );
        }
        else
            Of_ManComputeForwardDirconObj( p, i );
}
void Of_ManComputeBackwardDircon1( Of_Man_t * p )
{
    Gia_Obj_t * pObj; 
    Vec_Bit_t * vPointed; 
    int DelayLut1 = p->pPars->nDelayLut1;
    int DelayLut2 = p->pPars->nDelayLut2;
    int i, k, iVar, Flag, * pList, * pCutMin;
    int CountNodes = 0, CountEdges = 0;
    Of_ManComputeOutputRequired( p, 1 );
    printf( "Global delay =%8.2f\n", Of_Int2Flt((int)p->pPars->Delay) );
    //return;
    // compute area and edges
    vPointed = Vec_BitStart( Gia_ManObjNum(p->pGia) );
    p->pPars->Area = p->pPars->Edge = 0;
    Gia_ManForEachAndReverse( p->pGia, pObj, i )
    {
        int CostMin, fPointed, Required = Of_ObjRequired(p, i);
        if ( Gia_ObjIsBuf(pObj) )
        {
            int FaninId = Gia_ObjFaninId0(pObj, i);
            Of_ObjUpdateRequired( p, FaninId, Required );
            Of_ObjRefInc( p, FaninId );
            continue;
        }
        if ( !Of_ObjRefNum(p, i) )
            continue;
        // check if the LUT is has an outgoing dircon edge
        fPointed = Vec_BitEntry(vPointed, i);
        CountNodes += fPointed;

/*
        // select the best cut
        {
            int * pCut;
            pCutMin = NULL;
            CostMin = ABC_INFINITY;
            pList = Of_ObjCutSet( p, i );
            Of_SetForEachCut( pList, pCut, k )
            {
                int Cost;
                if ( (fPointed ? Of_CutDelay2(pCut) : Of_CutDelay1(pCut)) > Required )
                    continue;
                Cost = Of_ManComputeBackwardCut( p, pCut );
                if ( CostMin > Cost )
                {
                    CostMin = Cost;
                    pCutMin = pCut;
                }
            }
        }
*/

        if ( fPointed )
        {
            pCutMin = Of_ObjCutBestP2( p, i );
            CostMin = Of_CutDelay2(pCutMin);
            //assert( Of_CutDelay2(pCutMin) <= Required );
        }
        else
        {
            pCutMin = Of_ObjCutBestP( p, i );
            CostMin = Of_CutDelay1(pCutMin);
            //assert( Of_CutDelay1(pCutMin) <= Required );
        }

        // remove dircon markers
        //if ( fPointed )
        //    Of_CutForEachVarFlag( pCutMin, iVar, Flag, k )
        //        Of_CutSetFlag( pCutMin, k, 0 );

        // the cut is selected
        assert( pCutMin != NULL );
        pList = Of_ObjCutSet( p, i );
        Of_ObjSetCutBestP( p, pList, i, pCutMin );  ///// SET THE BEST CUT
        Of_CutForEachVarFlag( pCutMin, iVar, Flag, k )
        {
            Of_ObjUpdateRequired( p, iVar, Required - ((Flag && !fPointed) ? DelayLut2 : DelayLut1) );
            Of_ObjRefInc( p, iVar );           
            if ( Flag && !fPointed )
            {             
                Vec_BitWriteEntry( vPointed, iVar, 1 );
                CountEdges++;
            }
        }
        // update parameters
        p->pPars->Edge += Of_CutSize(pCutMin);
        p->pPars->Area++;
    }
    Vec_BitFree( vPointed );
    //printf( "Dircon nodes = %d.  Dircon edges = %d.\n", CountNodes, CountEdges );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Of_ManCreateSat( Of_Man_t * p, int nCutsAll, Vec_Int_t * vFirst, Vec_Int_t * vCutNum, Vec_Int_t * vBestNode, Vec_Int_t * vBestCut )
{
    extern void Cnf_AddCardinConstrPairWise( sat_solver * p, Vec_Int_t * vVars, int K, int fStrict );

    Gia_Obj_t * pObj, * pVar;
    int * pCutSet, * pCut;
    int i, k, v, c, Var, Lit, pLits[2], status, RetValue, nCutCount, nClauses;
    Vec_Int_t * vLits = Vec_IntAlloc( 100 );
    abctime clk = Abc_Clock();

    // start solver
    sat_solver * pSat = sat_solver_new();
    sat_solver_setnvars( pSat, Gia_ManAndNum(p->pGia) + nCutsAll );

    // set polarity
    Vec_IntAppend( vBestNode, vBestCut );
    //Vec_IntPrint( vBestNode );
    sat_solver_set_polarity( pSat, Vec_IntArray(vBestNode), Vec_IntSize(vBestNode) );
    Vec_IntShrink( vBestNode, Vec_IntSize(vBestNode) - Vec_IntSize(vBestCut) );

    // add clauses for nodes
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
        int iFirst = Vec_IntEntry(vFirst, i);
        int nCuts = Vec_IntEntry(vCutNum, i);
        Vec_IntClear( vLits );
        Vec_IntPush( vLits, Abc_Var2Lit(pObj->Value, 1) );
        for ( c = 0; c < nCuts; c++ )
            Vec_IntPush( vLits, Abc_Var2Lit(iFirst + c, 0) );
        RetValue = sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntLimit(vLits) );
        assert( RetValue );
    }

    // add clauses for cuts
    nCutCount = 0;
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
        pCutSet = Of_ObjCutSet(p, i);
        Of_SetForEachCut( pCutSet, pCut, k )
        {
            pLits[0] = Abc_Var2Lit( Gia_ManAndNum(p->pGia) + nCutCount, 1 );
            pLits[1] = Abc_Var2Lit( pObj->Value, 0 );
            RetValue = sat_solver_addclause( pSat, pLits, pLits+2 );
            assert( RetValue );
            Of_CutForEachVar( pCut, Var, v )
            {
                pVar = Gia_ManObj(p->pGia, Var);
                if ( !Gia_ObjIsAnd(pVar) )
                    continue;
                pLits[1] = Abc_Var2Lit( pVar->Value, 0 );
                RetValue = sat_solver_addclause( pSat, pLits, pLits+2 );
                assert( RetValue );
            }
            nCutCount++;
        }
    }
    assert( nCutCount == nCutsAll );

    // mark CO drivers
    Gia_ManForEachCo( p->pGia, pObj, i )
        Gia_ObjFanin0(pObj)->fMark0 = 1;
    // set used nodes to 1
    Gia_ManForEachAnd( p->pGia, pObj, i )
        if ( pObj->fMark0 )
        {
            Lit = Abc_Var2Lit( pObj->Value, 0 );
            RetValue = sat_solver_addclause( pSat, &Lit, &Lit + 1 );
            assert( RetValue );
        }
    // unmark CO drivers
    Gia_ManForEachCo( p->pGia, pObj, i )
        Gia_ObjFanin0(pObj)->fMark0 = 0;

//    Sat_SolverWriteDimacs( pSat, "temp.cnf", NULL, NULL, 0 );

    // add cardinality constraint
    nClauses = pSat->stats.clauses;
    Vec_IntClear( vLits );
    Vec_IntFillNatural( vLits, Gia_ManAndNum(p->pGia) );
    Cnf_AddCardinConstrPairWise( pSat, vLits, Vec_IntSize(vBestNode)-2, 0 );
    printf( "Problem clauses = %d.  Cardinality clauses = %d.\n", nClauses, pSat->stats.clauses - nClauses );

    // solve the problem
    status = sat_solver_solve( pSat, NULL, NULL, 1000000, 0, 0, 0 );
    if ( status == l_Undef )
        printf( "Undecided.  " );
    if ( status == l_True )
        printf( "Satisfiable.  " );
    if ( status == l_False )
        printf( "Unsatisfiable. " );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    Sat_SolverPrintStats( stdout, pSat );
    if ( status == l_True )
    {
        int nOnes = 0;
        for ( v = 0; v < Gia_ManAndNum(p->pGia); v++ )
        {
            printf( "%d", sat_solver_var_value(pSat, v) );
            nOnes += sat_solver_var_value(pSat, v);
        }
        printf( " Nodes = %d\n", nOnes );

        nOnes = 0;
        for ( ; v < Gia_ManAndNum(p->pGia) + nCutsAll; v++ )
        {
            printf( "%d", sat_solver_var_value(pSat, v) );
            nOnes += sat_solver_var_value(pSat, v);
        }
        printf( " LUTs = %d\n", nOnes );
    }

    // cleanup
    sat_solver_delete( pSat );
    Vec_IntFree( vLits );
}
void Of_ManPrintCuts( Of_Man_t * p )
{
    int fVerbose = 0;
    Gia_Obj_t * pObj;
    int * pCutSet, * pCut, * pCutBest;
    int i, k, v, Var, nCuts;
    Vec_Int_t * vFirst  = Vec_IntStartFull( Gia_ManObjNum(p->pGia) );
    Vec_Int_t * vCutNum = Vec_IntStartFull( Gia_ManObjNum(p->pGia) );
    Vec_Int_t * vBestNode = Vec_IntAlloc( 100 );
    Vec_Int_t * vBestCut  = Vec_IntAlloc( 100 );
    int nAndsAll = 0, nCutsAll = 0, Shift = Gia_ManAndNum(p->pGia);
    Gia_ManFillValue( p->pGia );
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
        // get the best cut
        pCutBest = NULL;
        if ( Of_ObjRefNum(p, i) )
        {
            Vec_IntPush( vBestNode, nAndsAll );
            pCutBest = Of_ObjCutBestP( p, i ); 
        }
        pObj->Value = nAndsAll++;
        // get the cutset
        pCutSet = Of_ObjCutSet(p, i);
        // count cuts
        nCuts = 0;
        Of_SetForEachCut( pCutSet, pCut, k )
            nCuts++;
        // save
        Vec_IntWriteEntry( vFirst,  i, Shift + nCutsAll );
        Vec_IntWriteEntry( vCutNum, i, nCuts );
        // print cuts
        if ( fVerbose )
            printf( "Node %d. Cuts %d.\n", i, nCuts );
        Of_SetForEachCut( pCutSet, pCut, k )
        {
            if ( fVerbose )
            {
                printf( "{ " );
                Of_CutForEachVar( pCut, Var, v )
                    printf( "%d ", Var );
                printf( "} %s\n", pCutBest == pCut ? "best" :"" );
            }
            if ( pCutBest == pCut )
                Vec_IntPush( vBestCut, Shift + nCutsAll );
            nCutsAll++;
        }
    }
    assert( nAndsAll == Shift );
    printf( "Total:   Ands = %d.  Luts = %d.  Cuts = %d.\n", nAndsAll, Vec_IntSize(vBestNode), nCutsAll );

    // create SAT problem
    Of_ManCreateSat( p, nCutsAll, vFirst, vCutNum, vBestNode, vBestCut );

    Vec_IntFree( vFirst );
    Vec_IntFree( vCutNum );
    Vec_IntFree( vBestNode );
    Vec_IntFree( vBestCut );
}

/**Function*************************************************************

  Synopsis    [Technology mappping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Of_ManSetDefaultPars( Jf_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Jf_Par_t) );
    pPars->nLutSize     =  4;
    pPars->nCutNum      = 16;
    pPars->nProcNum     =  0;
    pPars->nRounds      =  3;
    pPars->nRoundsEla   =  4;
    pPars->nRelaxRatio  =  0;
    pPars->nCoarseLimit =  3;
    pPars->nAreaTuner   = 10;
    pPars->DelayTarget  = -1;
    pPars->nDelayLut1   = 10;
    pPars->nDelayLut2   =  2;
    pPars->nFastEdges   =  0; //
    pPars->fAreaOnly    =  0;
    pPars->fOptEdge     =  1; 
    pPars->fCoarsen     =  0;
    pPars->fCutMin      =  0;
    pPars->fGenCnf      =  0;
    pPars->fPureAig     =  0;
    pPars->fVerbose     =  0;
    pPars->fVeryVerbose =  0;
    pPars->nLutSizeMax  =  OF_LEAF_MAX;
    pPars->nCutNumMax   =  OF_CUT_MAX;
    pPars->MapDelayTarget = -1;
}
Gia_Man_t * Of_ManDeriveMapping( Of_Man_t * p )
{
    Vec_Int_t * vMapping, * vPacking = NULL;
    Vec_Bit_t * vPointed; 
    int i, k, iVar, * pCut, Place, Flag;
    assert( !p->pPars->fCutMin && p->pGia->vMapping == NULL );
    vMapping = Vec_IntAlloc( Gia_ManObjNum(p->pGia) + (int)p->pPars->Edge + (int)p->pPars->Area * 2 );
    Vec_IntFill( vMapping, Gia_ManObjNum(p->pGia), 0 );
    if ( p->pPars->nFastEdges )
    {
        vPacking = Vec_IntAlloc( 1000 );
        Vec_IntPush( vPacking, 0 );
    }
    vPointed = Vec_BitStart( Gia_ManObjNum(p->pGia) );
    Gia_ManForEachAndId( p->pGia, i )
    {
        if ( !Of_ObjRefNum(p, i) )
            continue;
        assert( !Gia_ObjIsBuf(Gia_ManObj(p->pGia,i)) );
        pCut = Of_ObjCutBestP( p, i );
        Vec_IntWriteEntry( vMapping, i, Vec_IntSize(vMapping) );
        Vec_IntPush( vMapping, Of_CutSize(pCut) );
        Of_CutForEachVar( pCut, iVar, k )
            Vec_IntPush( vMapping, iVar );
        Vec_IntPush( vMapping, i );
        if ( vPacking == NULL || Vec_BitEntry(vPointed, i) )
            continue;
        Place = Vec_IntSize( vPacking );
        Vec_IntPush( vPacking, 0 );
        Vec_IntPush( vPacking, i );
        Of_CutForEachVarFlag( pCut, iVar, Flag, k )
            if ( Flag )
            {
                Vec_IntPush( vPacking, iVar );
                Vec_BitWriteEntry( vPointed, iVar, 1 );
            }
        Vec_IntAddToEntry( vPacking, Place, Vec_IntSize(vPacking)-Place-1 );
        Vec_IntAddToEntry( vPacking, 0, 1 );
    }
    assert( Vec_IntCap(vMapping) == 16 || Vec_IntSize(vMapping) == Vec_IntCap(vMapping) );
    p->pGia->vMapping = vMapping;
    p->pGia->vPacking = vPacking;
    Vec_BitFree( vPointed );
    return p->pGia;
}
Gia_Man_t * Of_ManPerformMapping( Gia_Man_t * pGia, Jf_Par_t * pPars )
{
    Gia_Man_t * pNew = NULL, * pCls;
    Of_Man_t * p; int i, Id;
    if ( Gia_ManHasChoices(pGia) )
        pPars->fCoarsen = 0, pPars->fCutMin = 1; 
    pCls = pPars->fCoarsen ? Gia_ManDupMuxes(pGia, pPars->nCoarseLimit) : pGia;
    p = Of_StoCreate( pCls, pPars );
    if ( pPars->fVerbose && pPars->fCoarsen )
    {
        printf( "Initial " );  Gia_ManPrintMuxStats( pGia );  printf( "\n" );
        printf( "Derived " );  Gia_ManPrintMuxStats( pCls );  printf( "\n" );
    }
    Of_ManPrintInit( p );
    Of_ManComputeCuts( p );
    Of_ManPrintQuit( p );

    Gia_ManForEachCiId( p->pGia, Id, i )
    {
        int Time = Of_Flt2Int(p->pGia->vInArrs ? Abc_MaxFloat(0.0, Vec_FltEntry(p->pGia->vInArrs, i)) : 0.0);
        Of_ObjSetDelay1( p, Id, Time );
        Of_ObjSetDelay2( p, Id, Time );
    }

    if ( p->pPars->nFastEdges )
    {
        p->pPars->nRounds = 1;
        for ( p->Iter = 0; p->Iter < p->pPars->nRounds; p->Iter++ )
        {
            if ( p->Iter == 0 )
            {
                Of_ManComputeForwardDircon1( p );
                Of_ManComputeBackwardDircon1( p );
                Of_ManPrintStats( p, "Delay" );
            }
            else 
            {
                Of_ManComputeForwardDircon1( p );
                Of_ManComputeBackwardDircon1( p );
                Of_ManPrintStats( p, "Flow " );
            }
        }
    }
    else
    {
        for ( p->Iter = 0; p->Iter < p->pPars->nRounds; p->Iter++ )
        {
            if ( p->Iter == 0 )
            {
                Of_ManComputeForward1( p );
                Of_ManComputeBackward1( p );
                Of_ManPrintStats( p, "Delay" );
            }
            else 
            {
                Of_ManComputeForward1( p );
                Of_ManComputeBackward1( p );
                Of_ManPrintStats( p, "Flow " );
            }
        }
        for ( ; p->Iter < p->pPars->nRounds + p->pPars->nRoundsEla; p->Iter++ )
        {
            if ( p->Iter < p->pPars->nRounds + p->pPars->nRoundsEla - 1 )
            {
                Of_ManComputeForward2( p );
                Of_ManComputeBackward3( p );
                Of_ManPrintStats( p, "Area " );
            }
            else
            {
                Of_ManComputeForward1( p );
                Of_ManComputeBackward3( p );
                Of_ManPrintStats( p, "Area " );
            }
        }
    }

    pNew = Of_ManDeriveMapping( p );
    Gia_ManMappingVerify( pNew );
    if ( pNew->vPacking )
        Gia_ManConvertPackingToEdges( pNew );
    //Of_ManPrintCuts( p );
    Of_StoDelete( p );
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

