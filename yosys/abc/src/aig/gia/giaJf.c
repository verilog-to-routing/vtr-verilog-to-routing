/**CFile****************************************************************

  FileName    [giaJf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaJf.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/vec/vecSet.h"
#include "misc/vec/vecMem.h"
#include "misc/extra/extra.h"
#include "bool/kit/kit.h"
#include "misc/util/utilTruth.h"
#include "opt/dau/dau.h"
#include "sat/cnf/cnf.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define JF_LEAF_MAX   8
#define JF_WORD_MAX  ((JF_LEAF_MAX > 6) ? 1 << (JF_LEAF_MAX-6) : 1)
#define JF_CUT_MAX   16
#define JF_EPSILON 0.005

typedef struct Jf_Cut_t_ Jf_Cut_t; 
struct Jf_Cut_t_
{
    word             Sign;        // signature
    float            Flow;        // flow
    int              Time;        // arrival time
    int              iFunc;       // function 
    int              Cost;        // cut cost
    int              pCut[JF_LEAF_MAX+2]; // cut
};

typedef struct Jf_Man_t_ Jf_Man_t; 
struct Jf_Man_t_
{
    Gia_Man_t *      pGia;        // user's manager
    Jf_Par_t *       pPars;       // users parameter
    Sdm_Man_t *      pDsd;        // extern DSD manager
    Vec_Int_t *      vCnfs;       // costs of elementary CNFs
    Vec_Mem_t *      vTtMem;      // truth table memory and hash table
    Vec_Int_t        vCuts;       // cuts for each node
    Vec_Int_t        vArr;        // arrival time
    Vec_Int_t        vDep;        // departure time
    Vec_Flt_t        vFlow;       // area flow
    Vec_Flt_t        vRefs;       // ref counters
    Vec_Set_t        pMem;        // cut storage
    Vec_Int_t *      vTemp;       // temporary
    float (*pCutCmp) (Jf_Cut_t *, Jf_Cut_t *);// procedure to compare cuts
    abctime          clkStart;    // starting time
    word             CutCount[4]; // statistics
    int              nCoarse;     // coarse nodes
};

static inline int    Jf_ObjIsUnit( Gia_Obj_t * p )          { return !p->fMark0;                                       }
static inline void   Jf_ObjCleanUnit( Gia_Obj_t * p )       { assert(Jf_ObjIsUnit(p)); p->fMark0 = 1;                  }
static inline void   Jf_ObjSetUnit( Gia_Obj_t * p )         { p->fMark0 = 0;                                           }

static inline int    Jf_ObjCutH( Jf_Man_t * p, int i )      { return Vec_IntEntry(&p->vCuts, i);                       }
static inline int *  Jf_ObjCuts( Jf_Man_t * p, int i )      { return (int *)Vec_SetEntry(&p->pMem, Jf_ObjCutH(p, i));  }
static inline int *  Jf_ObjCutBest( Jf_Man_t * p, int i )   { return Jf_ObjCuts(p, i) + 1;                             }
static inline int    Jf_ObjArr( Jf_Man_t * p, int i )       { return Vec_IntEntry(&p->vArr, i);                        }
static inline int    Jf_ObjDep( Jf_Man_t * p, int i )       { return Vec_IntEntry(&p->vDep, i);                        }
static inline float  Jf_ObjFlow( Jf_Man_t * p, int i )      { return Vec_FltEntry(&p->vFlow, i);                       }
static inline float  Jf_ObjRefs( Jf_Man_t * p, int i )      { return Vec_FltEntry(&p->vRefs, i);                       }
//static inline int    Jf_ObjLit( int i, int c )            { return i;                                                }
static inline int    Jf_ObjLit( int i, int c )              { return Abc_Var2Lit( i, c );                              }

static inline int    Jf_CutSize( int * pCut )               { return pCut[0] & 0xF;                                    }  //  4 bits
static inline int    Jf_CutCost( int * pCut )               { return (pCut[0] >> 4) & 0xF;                             }  //  4 bits
static inline int    Jf_CutFunc( int * pCut )               { return ((unsigned)pCut[0] >> 8);                         }  // 24 bits
static inline int    Jf_CutSetAll( int f, int c, int s )    { return (f << 8) | (c << 4) | s;                          }
static inline void   Jf_CutSetSize( int * pCut, int s )     { assert(s>=0 && s<16); pCut[0] ^= (Jf_CutSize(pCut) ^ s); }
static inline void   Jf_CutSetCost( int * pCut, int c )     { assert(c>=0 && c<16); pCut[0] ^=((Jf_CutCost(pCut) ^ c) << 4); }
static inline void   Jf_CutSetFunc( int * pCut, int f )     { assert(f>=0); pCut[0] ^=((Jf_CutFunc(pCut) ^ f) << 8);   }

static inline int    Jf_CutFuncClass( int * pCut )          { return Abc_Lit2Var(Jf_CutFunc(pCut));                    }
static inline int    Jf_CutFuncCompl( int * pCut )          { return Abc_LitIsCompl(Jf_CutFunc(pCut));                 }
static inline int *  Jf_CutLits( int * pCut )               { return pCut + 1;                                         }
static inline int    Jf_CutLit( int * pCut, int i )         { assert(i);return pCut[i];                                }
//static inline int    Jf_CutVar( int * pCut, int i )         { assert(i); return pCut[i];                               }
static inline int    Jf_CutVar( int * pCut, int i )         {  assert(i);return Abc_Lit2Var(pCut[i]);                  }
static inline int    Jf_CutIsTriv( int * pCut, int i )      { return Jf_CutSize(pCut) == 1 && Jf_CutVar(pCut, 1) == i; } 
static inline int    Jf_CutCnfSizeF( Jf_Man_t * p, int f )  { return Vec_IntEntry( p->vCnfs, f );                      }
static inline int    Jf_CutCnfSize( Jf_Man_t * p, int * c ) { return Jf_CutCnfSizeF( p, Jf_CutFuncClass(c) );          }

static inline int    Jf_ObjFunc0( Gia_Obj_t * p, int * c )  { return Abc_LitNotCond(Jf_CutFunc(c), Gia_ObjFaninC0(p)); } 
static inline int    Jf_ObjFunc1( Gia_Obj_t * p, int * c )  { return Abc_LitNotCond(Jf_CutFunc(c), Gia_ObjFaninC1(p)); } 

#define Jf_ObjForEachCut( pList, pCut, i )   for ( i = 0, pCut = pList + 1; i < pList[0]; i++, pCut += Jf_CutSize(pCut) + 1 )
#define Jf_CutForEachLit( pCut, Lit, i )     for ( i = 1; i <= Jf_CutSize(pCut) && (Lit = Jf_CutLit(pCut, i)); i++ )
#define Jf_CutForEachVar( pCut, Var, i )     for ( i = 1; i <= Jf_CutSize(pCut) && (Var = Jf_CutVar(pCut, i)); i++ )

extern int Kit_TruthToGia( Gia_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory, Vec_Int_t * vLeaves, int fHash );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives CNF for the mapped GIA.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Jf_ManGenCnf( word uTruth, int iLitOut, Vec_Int_t * vLeaves, Vec_Int_t * vLits, Vec_Int_t * vClas, Vec_Int_t * vCover )
{
    if ( uTruth == 0 || ~uTruth == 0 )
    {
        Vec_IntPush( vClas, Vec_IntSize(vLits) );
        Vec_IntPush( vLits, Abc_LitNotCond(iLitOut, (uTruth == 0)) );
    }
    else 
    {
        int i, k, c, Literal, Cube;
        assert( Vec_IntSize(vLeaves) > 0 );
        for ( c = 0; c < 2; c ++ )
        {
            int RetValue = Kit_TruthIsop( (unsigned *)&uTruth, Vec_IntSize(vLeaves), vCover, 0 );
            assert( RetValue == 0 );
            Vec_IntForEachEntry( vCover, Cube, i )
            {
                Vec_IntPush( vClas, Vec_IntSize(vLits) );
                Vec_IntPush( vLits, Abc_LitNotCond(iLitOut, c) );
                for ( k = 0; k < Vec_IntSize(vLeaves); k++ )
                {
                    Literal = 3 & (Cube >> (k << 1));
                    if ( Literal == 1 )      // '0'  -> pos lit
                        Vec_IntPush( vLits, Abc_LitNotCond(Vec_IntEntry(vLeaves, k), 0) );
                    else if ( Literal == 2 ) // '1'  -> neg lit
                        Vec_IntPush( vLits, Abc_LitNotCond(Vec_IntEntry(vLeaves, k), 1) );
                    else if ( Literal != 0 )
                        assert( 0 );
                }
            }
            uTruth = ~uTruth;
        }
    }
}
Cnf_Dat_t * Jf_ManCreateCnfRemap( Gia_Man_t * p, Vec_Int_t * vLits, Vec_Int_t * vClas, int fAddOrCla )
{
    Cnf_Dat_t * pCnf; 
    Gia_Obj_t * pObj;
    int i, Entry, * pMap, nVars = 0;
    if ( fAddOrCla )
    {
        Vec_IntPush( vClas, Vec_IntSize(vLits) );
        Gia_ManForEachPo( p, pObj, i )
            Vec_IntPush( vLits, Abc_Var2Lit(Gia_ObjId(p, pObj), 0) );
    }
    // label nodes present in the mapping
    Vec_IntForEachEntry( vLits, Entry, i )
        Gia_ManObj(p, Abc_Lit2Var(Entry))->fMark0 = 1;
    // create variable map
    pMap = ABC_FALLOC( int, Gia_ManObjNum(p) );
    Gia_ManForEachObjReverse( p, pObj, i )
        if ( pObj->fMark0 )
            pObj->fMark0 = 0, pMap[i] = nVars++;
    // relabel literals
    Vec_IntForEachEntry( vLits, Entry, i )
        Vec_IntWriteEntry( vLits, i, Abc_Lit2LitV(pMap, Entry) );
    // generate CNF
    pCnf = ABC_CALLOC( Cnf_Dat_t, 1 );
    pCnf->pMan = (Aig_Man_t *)p;
    pCnf->nVars = nVars;
    pCnf->nLiterals = Vec_IntSize(vLits);
    pCnf->nClauses = Vec_IntSize(vClas);
    pCnf->pClauses = ABC_ALLOC( int *, pCnf->nClauses+1 );
    pCnf->pClauses[0] = Vec_IntReleaseArray(vLits);
    Vec_IntForEachEntry( vClas, Entry, i )
        pCnf->pClauses[i] = pCnf->pClauses[0] + Entry;
    pCnf->pClauses[i] = pCnf->pClauses[0] + pCnf->nLiterals;
    pCnf->pVarNums = pMap;
    return pCnf;
}
Cnf_Dat_t * Jf_ManCreateCnf( Gia_Man_t * p, Vec_Int_t * vLits, Vec_Int_t * vClas )
{
    Cnf_Dat_t * pCnf; 
    int i, Entry, iOut;
    // generate CNF
    pCnf = ABC_CALLOC( Cnf_Dat_t, 1 );
    pCnf->pMan = (Aig_Man_t *)p;
    pCnf->nVars = Gia_ManObjNum(p);
    pCnf->nLiterals = Vec_IntSize(vLits);
    pCnf->nClauses = Vec_IntSize(vClas);
    pCnf->pClauses = ABC_ALLOC( int *, pCnf->nClauses+1 );
    pCnf->pClauses[0] = Vec_IntReleaseArray(vLits);
    Vec_IntForEachEntry( vClas, Entry, i )
        pCnf->pClauses[i] = pCnf->pClauses[0] + Entry;
    pCnf->pClauses[i] = pCnf->pClauses[0] + pCnf->nLiterals;
    // create mapping of objects into their clauses
    pCnf->pObj2Clause = ABC_FALLOC( int, Gia_ManObjNum(p) );
    pCnf->pObj2Count  = ABC_FALLOC( int, Gia_ManObjNum(p) );
    for ( i = 0; i < pCnf->nClauses; i++ )
    {
        iOut = Abc_Lit2Var(pCnf->pClauses[i][0]);
        if ( pCnf->pObj2Clause[iOut] == -1 )
        {
            pCnf->pObj2Clause[iOut] = i;
            pCnf->pObj2Count[iOut] = 1;
        }
        else
        {
            assert( pCnf->pObj2Count[iOut] > 0 );
            pCnf->pObj2Count[iOut]++;
        }
    }
    return pCnf;
}

/**Function*************************************************************

  Synopsis    [Computing references while discounting XOR/MUX.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float * Jf_ManInitRefs( Jf_Man_t * pMan )
{
    Gia_Man_t * p = pMan->pGia;
    Gia_Obj_t * pObj, * pCtrl, * pData0, * pData1;
    float * pRes; int i;
    assert( p->pRefs == NULL );
    p->pRefs = ABC_CALLOC( int, Gia_ManObjNum(p) );
    Gia_ManForEachAnd( p, pObj, i )
    {
        Gia_ObjRefFanin0Inc( p, pObj );
        if ( Gia_ObjIsBuf(pObj) )
            continue;
        Gia_ObjRefFanin1Inc( p, pObj );
        if ( !Gia_ObjIsMuxType(pObj) )
            continue;
        // discount XOR/MUX
        pCtrl = Gia_ObjRecognizeMux( pObj, &pData1, &pData0 );
        Gia_ObjRefDec( p, Gia_Regular(pCtrl) );
        if ( Gia_Regular(pData1) == Gia_Regular(pData0) )
            Gia_ObjRefDec( p, Gia_Regular(pData1) );
    }
    Gia_ManForEachCo( p, pObj, i )
        Gia_ObjRefFanin0Inc( p, pObj );
    // mark XOR/MUX internal nodes, which are not used elsewhere
    if ( pMan->pPars->fCoarsen )
    {
        pMan->nCoarse = 0;
        Gia_ManForEachAnd( p, pObj, i )
        {
            if ( !Gia_ObjIsMuxType(pObj) )
                continue;
            if ( Gia_ObjRefNum(p, Gia_ObjFanin0(pObj)) == 1 )
            {
                Jf_ObjSetUnit(Gia_ObjFanin0(Gia_ObjFanin0(pObj)));
                Jf_ObjSetUnit(Gia_ObjFanin0(Gia_ObjFanin1(pObj)));
                Jf_ObjCleanUnit(Gia_ObjFanin0(pObj)), pMan->nCoarse++;
            }
            if ( Gia_ObjRefNum(p, Gia_ObjFanin1(pObj)) == 1 )
            {
                Jf_ObjSetUnit(Gia_ObjFanin1(Gia_ObjFanin0(pObj)));
                Jf_ObjSetUnit(Gia_ObjFanin1(Gia_ObjFanin1(pObj)));
                Jf_ObjCleanUnit(Gia_ObjFanin1(pObj)), pMan->nCoarse++;
            }
        }
    }
    // multiply by factor
    pRes = ABC_ALLOC( float, Gia_ManObjNum(p) );
    for ( i = 0; i < Gia_ManObjNum(p); i++ )
        pRes[i] = Abc_MaxInt( 1, p->pRefs[i] );
    return pRes;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Jf_ManProfileClasses( Jf_Man_t * p )
{
    Gia_Obj_t * pObj; 
    int Counts[595] = {0}, Costs[595] = {0};
    int i, iFunc, Total = 0, CostTotal = 0, Other = 0, CostOther = 0;
    printf( "DSD classes that appear in more than %.1f %% of mapped nodes:\n", 0.1 * p->pPars->nVerbLimit );
    Gia_ManForEachAnd( p->pGia, pObj, i )
        if ( !Gia_ObjIsBuf(pObj) && Gia_ObjRefNumId(p->pGia, i) )
        {
            iFunc = Jf_CutFuncClass( Jf_ObjCutBest(p, i) );
            assert( iFunc < 595 );
            if ( p->pPars->fGenCnf )
            {
                Costs[iFunc] += Jf_CutCnfSizeF(p, iFunc);
                CostTotal += Jf_CutCnfSizeF(p, iFunc);
            }
            Counts[iFunc]++;
            Total++;
        }
    CostTotal = Abc_MaxInt(CostTotal, 1);
    Total = Abc_MaxInt(Total, 1);
    for ( i = 0; i < 595; i++ )
        if ( Counts[i] && 100.0 * Counts[i] / Total >= 0.1 * p->pPars->nVerbLimit )
        {
            printf( "%5d  :  ", i );
            printf( "%-20s   ", Sdm_ManReadDsdStr(p->pDsd, i) );
            printf( "%8d  ",    Counts[i] );
            printf( "%5.1f %%   ", 100.0 * Counts[i] / Total );
            printf( "%8d  ",    Costs[i] );
            printf( "%5.1f %%", 100.0 * Costs[i] / CostTotal );
            printf( "\n" );
        }
        else
        {
            Other += Counts[i];
            CostOther += Costs[i];
        }
    printf( "Other  :  " );
    printf( "%-20s   ",   "" );
    printf( "%8d  ",    Other );
    printf( "%5.1f %%   ", 100.0 * Other / Total );
    printf( "%8d  ",    CostOther );
    printf( "%5.1f %%", 100.0 * CostOther / CostTotal );
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Manager manipulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Jf_Man_t * Jf_ManAlloc( Gia_Man_t * pGia, Jf_Par_t * pPars )
{
    Jf_Man_t * p;
    assert( pPars->nLutSize <= JF_LEAF_MAX );
    assert( pPars->nCutNum <= JF_CUT_MAX );
    Vec_IntFreeP( &pGia->vMapping );
    p = ABC_CALLOC( Jf_Man_t, 1 );
    p->pGia      = pGia;
    p->pPars     = pPars;
    if ( pPars->fCutMin && !pPars->fFuncDsd )
        p->vTtMem = Vec_MemAllocForTT( pPars->nLutSize, 0 );
    else if ( pPars->fCutMin && pPars->fFuncDsd )
    {
        p->pDsd = Sdm_ManRead();
        if ( pPars->fGenCnf )
        {
            p->vCnfs = Vec_IntStart( 595 );
            Sdm_ManReadCnfCosts( p->pDsd, Vec_IntArray(p->vCnfs), Vec_IntSize(p->vCnfs) );
        }
    }
    Vec_IntFill( &p->vCuts, Gia_ManObjNum(pGia), 0 );
    Vec_IntFill( &p->vArr,  Gia_ManObjNum(pGia), 0 );
    Vec_IntFill( &p->vDep,  Gia_ManObjNum(pGia), 0 );
    Vec_FltFill( &p->vFlow, Gia_ManObjNum(pGia), 0 );
    p->vRefs.nCap = p->vRefs.nSize = Gia_ManObjNum(pGia);
    p->vRefs.pArray = Jf_ManInitRefs( p );
    Vec_SetAlloc_( &p->pMem, 20 );
    p->vTemp     = Vec_IntAlloc( 1000 );
    p->clkStart  = Abc_Clock();
    return p;
}
void Jf_ManFree( Jf_Man_t * p )
{
    if ( p->pPars->fVerbose && p->pDsd )
        Sdm_ManPrintDsdStats( p->pDsd, 0 );
    if ( p->pPars->fVerbose && p->vTtMem )
    {
        printf( "Unique truth tables = %d. Memory = %.2f MB   ", Vec_MemEntryNum(p->vTtMem), Vec_MemMemory(p->vTtMem) / (1<<20) ); 
        Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
    }
    if ( p->pPars->fVeryVerbose && p->pPars->fCutMin && p->pPars->fFuncDsd )
        Jf_ManProfileClasses( p );
    if ( p->pPars->fCoarsen )
        Gia_ManCleanMark0( p->pGia );
    ABC_FREE( p->pGia->pRefs );
    ABC_FREE( p->vCuts.pArray );
    ABC_FREE( p->vArr.pArray );
    ABC_FREE( p->vDep.pArray );
    ABC_FREE( p->vFlow.pArray );
    ABC_FREE( p->vRefs.pArray );
    if ( p->pPars->fCutMin && !p->pPars->fFuncDsd )
    {
        Vec_MemHashFree( p->vTtMem );
        Vec_MemFree( p->vTtMem );
    }
    Vec_IntFreeP( &p->vCnfs );
    Vec_SetFree_( &p->pMem );
    Vec_IntFreeP( &p->vTemp );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Cut functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Jf_CutPrint( int * pCut )
{
    int i; 
    printf( "%d {", Jf_CutSize(pCut) );
    for ( i = 1; i <= Jf_CutSize(pCut); i++ )
        printf( " %d", Jf_CutLit(pCut, i) );
    printf( " } Func = %d\n", Jf_CutFunc(pCut) );
}
static inline void Jf_ObjCutPrint( int * pCuts )
{
    int i, * pCut; 
    Jf_ObjForEachCut( pCuts, pCut, i )
        Jf_CutPrint( pCut );
    printf( "\n" );
}
static inline void Jf_ObjBestCutConePrint( Jf_Man_t * p, Gia_Obj_t * pObj )
{
    int * pCut = Jf_ObjCutBest( p, Gia_ObjId(p->pGia, pObj) );
    printf( "Best cut of node %d : ", Gia_ObjId(p->pGia, pObj) );
    Jf_CutPrint( pCut );
    Gia_ManPrintCone( p->pGia, pObj, Jf_CutLits(pCut), Jf_CutSize(pCut), p->vTemp );
}
static inline void Jf_CutCheck( int * pCut )
{
    int i, k;
    for ( i = 2; i <= Jf_CutSize(pCut); i++ )
        for ( k = 1; k < i; k++ )
            assert( Jf_CutLit(pCut, i) != Jf_CutLit(pCut, k) );
}
static inline int Jf_CountBitsSimple( unsigned n )
{
    int i, Count = 0;
    for ( i = 0; i < 32; i++ )
        Count += ((n >> i) & 1);
    return Count;
}
static inline int Jf_CountBits32( unsigned i )
{
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F);
    return (i*(0x01010101))>>24;
}
static inline int Jf_CountBits( word i )
{
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F);
    return (i*(0x0101010101010101))>>56;
}
static inline unsigned Jf_CutGetSign32( int * pCut )
{
    unsigned Sign = 0; int i; 
    for ( i = 1; i <= Jf_CutSize(pCut); i++ )
        Sign |= 1 << (Jf_CutVar(pCut, i) & 0x1F);
    return Sign;
}
static inline word Jf_CutGetSign( int * pCut )
{
    word Sign = 0; int i; 
    for ( i = 1; i <= Jf_CutSize(pCut); i++ )
        Sign |= ((word)1) << (Jf_CutVar(pCut, i) & 0x3F);
    return Sign;
}
static inline int Jf_CutArr( Jf_Man_t * p, int * pCut )
{
    int i, Time = 0;
    for ( i = 1; i <= Jf_CutSize(pCut); i++ )
        Time = Abc_MaxInt( Time, Jf_ObjArr(p, Jf_CutVar(pCut, i)) );
    return Time + 1; 
}

static inline void Jf_ObjSetBestCut( int * pCuts, int * pCut, Vec_Int_t * vTemp )
{
    assert( pCuts < pCut );
    if ( ++pCuts < pCut )
    {
        int nBlock = pCut - pCuts;
        int nSize = Jf_CutSize(pCut) + 1;
        Vec_IntGrow( vTemp, nBlock );
        memmove( Vec_IntArray(vTemp), pCuts, sizeof(int) * nBlock );
        memmove( pCuts, pCut, sizeof(int) * nSize );
        memmove( pCuts + nSize, Vec_IntArray(vTemp), sizeof(int) * nBlock );
    }
}
static inline void Jf_CutRef( Jf_Man_t * p, int * pCut )
{
    int i;
    for ( i = 1; i <= Jf_CutSize(pCut); i++ )
        Gia_ObjRefIncId( p->pGia, Jf_CutVar(pCut, i) );
}
static inline void Jf_CutDeref( Jf_Man_t * p, int * pCut )
{
    int i;
    for ( i = 1; i <= Jf_CutSize(pCut); i++ )
        Gia_ObjRefDecId( p->pGia, Jf_CutVar(pCut, i) );
}
static inline float Jf_CutFlow( Jf_Man_t * p, int * pCut )
{
    float Flow = 0; int i; 
    for ( i = 1; i <= Jf_CutSize(pCut); i++ )
        Flow += Jf_ObjFlow( p, Jf_CutVar(pCut, i) );
    assert( Flow >= 0 );
    return Flow; 
}

/**Function*************************************************************

  Synopsis    [Cut merging.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Jf_CutIsContainedOrder( int * pBase, int * pCut ) // check if pCut is contained pBase
{
    int nSizeB = Jf_CutSize(pBase);
    int nSizeC = Jf_CutSize(pCut);
    int i, k;
    if ( nSizeB == nSizeC )
    {
        for ( i = 1; i <= nSizeB; i++ )
            if ( pBase[i] != pCut[i] )
                return 0;
        return 1;
    }
    assert( nSizeB > nSizeC ); 
    for ( i = k = 1; i <= nSizeB; i++ )
    {
        if ( pBase[i] > pCut[k] )
            return 0;
        if ( pBase[i] == pCut[k] )
        {
            if ( k++ == nSizeC )
                return 1;
        }
    }
    return 0;
}
static inline int Jf_CutMergeOrder( int * pCut0, int * pCut1, int * pCut, int LutSize )
{ 
    int nSize0 = Jf_CutSize(pCut0);
    int nSize1 = Jf_CutSize(pCut1);
    int * pC0 = pCut0 + 1;
    int * pC1 = pCut1 + 1;
    int * pC = pCut + 1;
    int i, k, c, s;
    // the case of the largest cut sizes
    if ( nSize0 == LutSize && nSize1 == LutSize )
    {
        for ( i = 0; i < nSize0; i++ )
        {
            if ( pC0[i] != pC1[i] )
                return 0;
            pC[i] = pC0[i];
        }
        pCut[0] = LutSize;
        return 1;
    }
    // compare two cuts with different numbers
    i = k = c = s = 0;
    if ( nSize0 == 0 ) goto FlushCut1;
    if ( nSize1 == 0 ) goto FlushCut0;
    while ( 1 )
    {
        if ( c == LutSize ) return 0;
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
    if ( c + nSize0 > LutSize + i ) return 0;
    while ( i < nSize0 )
        pC[c++] = pC0[i++];
    pCut[0] = c;
    return 1;

FlushCut1:
    if ( c + nSize1 > LutSize + k ) return 0;
    while ( k < nSize1 )
        pC[c++] = pC1[k++];
    pCut[0] = c;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Cut merging.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Jf_CutFindLeaf0( int * pCut, int iObj )
{
    int i, nLits = Jf_CutSize(pCut);
    for ( i = 1; i <= nLits; i++ )
        if ( pCut[i] == iObj )
            return i;
    return i;
}
static inline int Jf_CutIsContained0( int * pBase, int * pCut ) // check if pCut is contained pBase
{
    int i, nLits = Jf_CutSize(pCut);
    for ( i = 1; i <= nLits; i++ )
        if ( Jf_CutFindLeaf0(pBase, pCut[i]) > pBase[0] )
            return 0;
    return 1;
}
static inline int Jf_CutMerge0( int * pCut0, int * pCut1, int * pCut, int LutSize )
{
    int nSize0 = Jf_CutSize(pCut0);
    int nSize1 = Jf_CutSize(pCut1), i;
    pCut[0] = nSize0;
    for ( i = 1; i <= nSize1; i++ )
        if ( Jf_CutFindLeaf0(pCut0, pCut1[i]) > nSize0 )
        {
            if ( pCut[0] == LutSize )
                return 0;
            pCut[++pCut[0]] = pCut1[i];
        }
    memcpy( pCut + 1, pCut0 + 1, sizeof(int) * nSize0 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Cut merging.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Jf_CutFindLeaf1( int * pCut, int iLit )
{
    int i, nLits = Jf_CutSize(pCut);
    for ( i = 1; i <= nLits; i++ )
        if ( Abc_Lit2Var(pCut[i]) == iLit )
            return i;
    return i;
}
static inline int Jf_CutIsContained1( int * pBase, int * pCut ) // check if pCut is contained pBase
{
    int i, nLits = Jf_CutSize(pCut);
    for ( i = 1; i <= nLits; i++ )
        if ( Jf_CutFindLeaf1(pBase, Abc_Lit2Var(pCut[i])) > pBase[0] )
            return 0;
    return 1;
}
static inline int Jf_CutMerge1( int * pCut0, int * pCut1, int * pCut, int LutSize )
{
    int nSize0 = Jf_CutSize(pCut0);
    int nSize1 = Jf_CutSize(pCut1), i;
    pCut[0] = nSize0;
    for ( i = 1; i <= nSize1; i++ )
        if ( Jf_CutFindLeaf1(pCut0, Abc_Lit2Var(pCut1[i])) > nSize0 )
        {
            if ( pCut[0] == LutSize )
                return 0;
            pCut[++pCut[0]] = pCut1[i];
        }
    memcpy( pCut + 1, pCut0 + 1, sizeof(int) * nSize0 );
    return 1;
}
static inline int Jf_CutMerge2( int * pCut0, int * pCut1, int * pCut, int LutSize )
{
    int ConfigMask = 0x3FFFF; // 18 bits
    int nSize0 = Jf_CutSize(pCut0);
    int nSize1 = Jf_CutSize(pCut1);
    int i, iPlace;
    pCut[0] = nSize0;
    for ( i = 1; i <= nSize1; i++ )
    {
        iPlace = Jf_CutFindLeaf1(pCut0, Abc_Lit2Var(pCut1[i]));
        if ( iPlace > nSize0 )
        {
            if ( pCut[0] == LutSize )
                return 0;
            pCut[(iPlace = ++pCut[0])] = pCut1[i];
        }
        else if ( pCut0[iPlace] != pCut1[i] )
            ConfigMask |= (1 << (iPlace+17));
        ConfigMask ^= (((i-1) ^ 7) << (3*(iPlace-1)));
    }
    memcpy( pCut + 1, pCut0 + 1, sizeof(int) * nSize0 );
    return ConfigMask;
}


/**Function*************************************************************

  Synopsis    [Cut filtering.]

  Description [Returns the number of cuts after filtering and the last
  cut in the last entry.  If the cut is filtered, its size is set to -1.]
               
  SideEffects [This was found to be 15% slower.]

  SeeAlso     []

***********************************************************************/
int Jf_ObjCutFilterBoth( Jf_Man_t * p, Jf_Cut_t ** pSto, int c )
{
    int k, last;
    // filter this cut using other cuts
    for ( k = 0; k < c; k++ )
        if ( pSto[c]->pCut[0] >= pSto[k]->pCut[0] && 
            (pSto[c]->Sign & pSto[k]->Sign) == pSto[k]->Sign && 
             Jf_CutIsContained1(pSto[c]->pCut, pSto[k]->pCut) )
        {
                pSto[c]->pCut[0] = -1;
                return c;
        }
    // filter other cuts using this cut
    for ( k = last = 0; k < c; k++ )
        if ( !(pSto[c]->pCut[0] < pSto[k]->pCut[0] && 
              (pSto[c]->Sign & pSto[k]->Sign) == pSto[c]->Sign && 
               Jf_CutIsContained1(pSto[k]->pCut, pSto[c]->pCut)) )
        {
            if ( last++ == k )
                continue;
            ABC_SWAP( Jf_Cut_t *, pSto[last-1], pSto[k] );
        }
    assert( last <= c );
    if ( last < c )
        ABC_SWAP( Jf_Cut_t *, pSto[last], pSto[c] );
    return last;
}
int Jf_ObjCutFilter( Jf_Man_t * p, Jf_Cut_t ** pSto, int c )
{
    int k;
    if ( p->pPars->fCutMin )
    {
        for ( k = 0; k < c; k++ )
            if ( pSto[c]->pCut[0] >= pSto[k]->pCut[0] && 
                (pSto[c]->Sign & pSto[k]->Sign) == pSto[k]->Sign && 
                 Jf_CutIsContained1(pSto[c]->pCut, pSto[k]->pCut) )
                 return 0;
    }
    else
    {
        for ( k = 0; k < c; k++ )
            if ( pSto[c]->pCut[0] >= pSto[k]->pCut[0] && 
                (pSto[c]->Sign & pSto[k]->Sign) == pSto[k]->Sign && 
                 Jf_CutIsContainedOrder(pSto[c]->pCut, pSto[k]->pCut) )
                 return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Sorting cuts by size.]

  Description []
               
  SideEffects [Did not really help.]

  SeeAlso     []

***********************************************************************/
static inline void Jf_ObjSortCuts( Jf_Cut_t ** pSto, int nSize )
{
    int i, j, best_i;
    for ( i = 0; i < nSize-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nSize; j++ )
            if ( pSto[j]->pCut[0] < pSto[best_i]->pCut[0] )
                best_i = j;
        ABC_SWAP( Jf_Cut_t *, pSto[i], pSto[best_i] );
    }
}

/**Function*************************************************************

  Synopsis    [Reference counting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Jf_CutRef_rec( Jf_Man_t * p, int * pCut )
{
    int i, Var, Count = Jf_CutCost(pCut);
    Jf_CutForEachVar( pCut, Var, i )
        if ( !Gia_ObjRefIncId(p->pGia, Var) && !Jf_CutIsTriv(Jf_ObjCutBest(p, Var), Var) )
            Count += Jf_CutRef_rec( p, Jf_ObjCutBest(p, Var) );
    return Count;
}
int Jf_CutDeref_rec( Jf_Man_t * p, int * pCut )
{
    int i, Var, Count = Jf_CutCost(pCut);
    Jf_CutForEachVar( pCut, Var, i )
        if ( !Gia_ObjRefDecId(p->pGia, Var) && !Jf_CutIsTriv(Jf_ObjCutBest(p, Var), Var) )
            Count += Jf_CutDeref_rec( p, Jf_ObjCutBest(p, Var) );
    return Count;
}
static inline int Jf_CutAreaOld( Jf_Man_t * p, int * pCut )
{
    int Ela1, Ela2;
    Ela1 = Jf_CutRef_rec( p, pCut );
    Ela2 = Jf_CutDeref_rec( p, pCut );
    assert( Ela1 == Ela2 );
    return Ela1;
}

int Jf_CutAreaRef_rec( Jf_Man_t * p, int * pCut )
{
    int i, Var, Count = Jf_CutCost(pCut);
    Jf_CutForEachVar( pCut, Var, i )
    {
        if ( !Gia_ObjRefIncId(p->pGia, Var) && !Jf_CutIsTriv(Jf_ObjCutBest(p, Var), Var) )
            Count += Jf_CutAreaRef_rec( p, Jf_ObjCutBest(p, Var) );
        Vec_IntPush( p->vTemp, Var );
    }
    return Count;
}
int Jf_CutAreaRefEdge_rec( Jf_Man_t * p, int * pCut )
{
    int i, Var, Count = (Jf_CutCost(pCut) << 4) | Jf_CutSize(pCut);
    Jf_CutForEachVar( pCut, Var, i )
    {
        if ( !Gia_ObjRefIncId(p->pGia, Var) && !Jf_CutIsTriv(Jf_ObjCutBest(p, Var), Var) )
            Count += Jf_CutAreaRefEdge_rec( p, Jf_ObjCutBest(p, Var) );
        Vec_IntPush( p->vTemp, Var );
    }
    return Count;
}
static inline int Jf_CutArea( Jf_Man_t * p, int * pCut, int fEdge )
{
    int Ela, Entry, i;
    Vec_IntClear( p->vTemp );
    if ( fEdge )
        Ela = Jf_CutAreaRefEdge_rec( p, pCut );
    else
        Ela = Jf_CutAreaRef_rec( p, pCut );
    Vec_IntForEachEntry( p->vTemp, Entry, i )
        Gia_ObjRefDecId( p->pGia, Entry );
    return Ela;
}
// returns 1 if MFFC size is less than limit
int Jf_CutCheckMffc_rec( Jf_Man_t * p, int * pCut, int Limit )
{
    int i, Var;
    Jf_CutForEachVar( pCut, Var, i )
    {
        int fRecur = (!Gia_ObjRefDecId(p->pGia, Var) && !Jf_CutIsTriv(Jf_ObjCutBest(p, Var), Var));
        Vec_IntPush( p->vTemp, Var );
        if ( Vec_IntSize(p->vTemp) >= Limit )
            return 0;
        if ( fRecur && !Jf_CutCheckMffc_rec( p, Jf_ObjCutBest(p, Var), Limit ) )
            return 0;
    }
    return 1;
}
static inline int Jf_CutCheckMffc( Jf_Man_t * p, int * pCut, int Limit )
{
    int RetValue, Entry, i;
    Vec_IntClear( p->vTemp );
    RetValue = Jf_CutCheckMffc_rec( p, pCut, Limit );
    Vec_IntForEachEntry( p->vTemp, Entry, i )
        Gia_ObjRefIncId( p->pGia, Entry );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Comparison procedures.]

  Description [Return positive value if the new cut is better than the old cut.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Jf_CutCompareDelay( Jf_Cut_t * pOld, Jf_Cut_t * pNew )
{
    if ( pOld->Time    != pNew->Time    ) return pOld->Time    - pNew->Time;
    if ( pOld->pCut[0] != pNew->pCut[0] ) return pOld->pCut[0] - pNew->pCut[0];
//    if ( pOld->Flow    != pNew->Flow    ) return pOld->Flow    - pNew->Flow;
    if ( pOld->Flow < pNew->Flow - JF_EPSILON ) return -1;
    if ( pOld->Flow > pNew->Flow + JF_EPSILON ) return 1;
    return 0;
}
float Jf_CutCompareArea( Jf_Cut_t * pOld, Jf_Cut_t * pNew )
{
//    if ( pOld->Flow    != pNew->Flow    ) return pOld->Flow    - pNew->Flow;
    if ( pOld->Flow < pNew->Flow - JF_EPSILON ) return -1;
    if ( pOld->Flow > pNew->Flow + JF_EPSILON ) return 1;
    if ( pOld->pCut[0] != pNew->pCut[0] ) return pOld->pCut[0] - pNew->pCut[0];
    if ( pOld->Time    != pNew->Time    ) return pOld->Time    - pNew->Time;
    return 0;
}
static inline int Jf_ObjAddCutToStore( Jf_Man_t * p, Jf_Cut_t ** pSto, int c, int cMax )
{
    Jf_Cut_t * pTemp;
    int k, last, iPivot;
    // if the store is empty, add anything
    if ( c == 0 )
        return 1;
    // special case when the cut store is full and last cut is better than new cut
    if ( c == cMax && p->pCutCmp(pSto[c-1], pSto[c]) <= 0 )
        return c;
    // find place of the given cut in the store
    assert( c <= cMax );
    for ( iPivot = c-1; iPivot >= 0; iPivot-- )
        if ( p->pCutCmp(pSto[iPivot], pSto[c]) < 0 ) // iPivot-th cut is better than new cut
            break;
    // filter this cut using other cuts
    if ( p->pPars->fCutMin )
    {
        for ( k = 0; k <= iPivot; k++ )
            if ( pSto[c]->pCut[0] >= pSto[k]->pCut[0] && 
                (pSto[c]->Sign & pSto[k]->Sign) == pSto[k]->Sign && 
                 Jf_CutIsContained1(pSto[c]->pCut, pSto[k]->pCut) )
                    return c;
    }
    else
    {
        for ( k = 0; k <= iPivot; k++ )
            if ( pSto[c]->pCut[0] >= pSto[k]->pCut[0] && 
                (pSto[c]->Sign & pSto[k]->Sign) == pSto[k]->Sign && 
                 Jf_CutIsContainedOrder(pSto[c]->pCut, pSto[k]->pCut) )
                    return c;
    }
    // insert this cut after iPivot
    pTemp = pSto[c];
    for ( ++iPivot, k = c++; k > iPivot; k-- )
        pSto[k] = pSto[k-1];
    pSto[iPivot] = pTemp;
    // filter other cuts using this cut
    if ( p->pPars->fCutMin )
    {
        for ( k = last = iPivot+1; k < c; k++ )
            if ( !(pSto[iPivot]->pCut[0] <= pSto[k]->pCut[0] && 
                  (pSto[iPivot]->Sign & pSto[k]->Sign) == pSto[iPivot]->Sign && 
                   Jf_CutIsContained1(pSto[k]->pCut, pSto[iPivot]->pCut)) )
            {
                if ( last++ == k )
                    continue;
                ABC_SWAP( Jf_Cut_t *, pSto[last-1], pSto[k] );
            }
    }
    else
    {
        for ( k = last = iPivot+1; k < c; k++ )
            if ( !(pSto[iPivot]->pCut[0] <= pSto[k]->pCut[0] && 
                  (pSto[iPivot]->Sign & pSto[k]->Sign) == pSto[iPivot]->Sign && 
                   Jf_CutIsContainedOrder(pSto[k]->pCut, pSto[iPivot]->pCut)) )
            {
                if ( last++ == k )
                    continue;
                ABC_SWAP( Jf_Cut_t *, pSto[last-1], pSto[k] );
            }
    }
    c = last;
    // remove the last cut if too many
    if ( c == cMax + 1 )
        return c - 1;
    return c;
}
static inline void Jf_ObjPrintStore( Jf_Man_t * p, Jf_Cut_t ** pSto, int c )
{
    int i;
    for ( i = 0; i < c; i++ )
    {
        printf( "Flow =%9.5f  ", pSto[i]->Flow );
        printf( "Time = %5d   ", pSto[i]->Time );
        printf( "Func = %5d   ", pSto[i]->iFunc );
        printf( "  " );
        Jf_CutPrint( pSto[i]->pCut );
    }
    printf( "\n" );
}
static inline void Jf_ObjCheckPtrs( Jf_Cut_t ** pSto, int c )
{
    int i, k;
    for ( i = 1; i < c; i++ )
        for ( k = 0; k < i; k++ )
            assert( pSto[k] != pSto[i] );
}
static inline void Jf_ObjCheckStore( Jf_Man_t * p, Jf_Cut_t ** pSto, int c, int iObj )
{
    int i, k;
    for ( i = 1; i < c; i++ )
        assert( p->pCutCmp(pSto[i-1], pSto[i]) <= 0 );
    for ( i = 1; i < c; i++ )
        for ( k = 0; k < i; k++ )
        {
            assert( !Jf_CutIsContained1(pSto[k]->pCut, pSto[i]->pCut) );
            assert( !Jf_CutIsContained1(pSto[i]->pCut, pSto[k]->pCut) );
        }
}

/**Function*************************************************************

  Synopsis    [Cut minimization.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Jf_TtComputeForCut( Jf_Man_t * p, int iFuncLit0, int iFuncLit1, int * pCut0, int * pCut1, int * pCutOut )
{
    word uTruth[JF_WORD_MAX], uTruth0[JF_WORD_MAX], uTruth1[JF_WORD_MAX];
    int fCompl, truthId;
    int LutSize    = p->pPars->nLutSize;
    int nWords     = Abc_Truth6WordNum(p->pPars->nLutSize);
    word * pTruth0 = Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(iFuncLit0));
    word * pTruth1 = Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(iFuncLit1));
    Abc_TtCopy( uTruth0, pTruth0, nWords, Abc_LitIsCompl(iFuncLit0) );
    Abc_TtCopy( uTruth1, pTruth1, nWords, Abc_LitIsCompl(iFuncLit1) );
    Abc_TtExpand( uTruth0, LutSize, pCut0 + 1, Jf_CutSize(pCut0), pCutOut + 1, Jf_CutSize(pCutOut) );
    Abc_TtExpand( uTruth1, LutSize, pCut1 + 1, Jf_CutSize(pCut1), pCutOut + 1, Jf_CutSize(pCutOut) );
    fCompl         = (int)(uTruth0[0] & uTruth1[0] & 1);
    Abc_TtAnd( uTruth, uTruth0, uTruth1, nWords, fCompl );
    pCutOut[0]     = Abc_TtMinBase( uTruth, pCutOut + 1, pCutOut[0], LutSize );
    assert( (uTruth[0] & 1) == 0 );
    truthId        = Vec_MemHashInsert(p->vTtMem, uTruth);
    return Abc_Var2Lit( truthId, fCompl );
}

/**Function*************************************************************

  Synopsis    [Cut enumeration.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Jf_ObjAssignCut( Jf_Man_t * p, Gia_Obj_t * pObj )
{
    int iObj = Gia_ObjId(p->pGia, pObj);
    int pClause[3] = { 1, Jf_CutSetAll(2, 0, 1), Jf_ObjLit(iObj, 0) }; // set function
    assert( Gia_ObjIsCi(pObj) || Gia_ObjIsBuf(pObj) );
    Vec_IntWriteEntry( &p->vCuts, iObj, Vec_SetAppend( &p->pMem, pClause, 3 ) );
}
static inline void Jf_ObjPropagateBuf( Jf_Man_t * p, Gia_Obj_t * pObj, int fReverse )
{
    int iObj = Gia_ObjId( p->pGia, pObj );
    int iFanin = Gia_ObjFaninId0( pObj, iObj );
    assert( 0 );
    assert( Gia_ObjIsBuf(pObj) );
    if ( fReverse )
        ABC_SWAP( int, iObj, iFanin );
    Vec_IntWriteEntry( &p->vArr,  iObj, Jf_ObjArr(p, iFanin) );
    Vec_FltWriteEntry( &p->vFlow, iObj, Jf_ObjFlow(p, iFanin) );
}
static inline int Jf_ObjHasCutWithSize( Jf_Cut_t ** pSto, int c, int nSize )
{
    int i;
    for ( i = 0; i < c; i++ )
        if ( pSto[i]->pCut[0] <= nSize )
            return 1;
    return 0;
}
void Jf_ObjComputeCuts( Jf_Man_t * p, Gia_Obj_t * pObj, int fEdge )
{
    int        LutSize = p->pPars->nLutSize;
    int        CutNum = p->pPars->nCutNum;
    int        iObj = Gia_ObjId(p->pGia, pObj);
    word       Sign0[JF_CUT_MAX+2]; // signatures of the first cut
    word       Sign1[JF_CUT_MAX+2]; // signatures of the second cut
    Jf_Cut_t   Sto[JF_CUT_MAX+2];   // cut storage
    Jf_Cut_t * pSto[JF_CUT_MAX+2];  // pointers to cut storage
    int *      pCut0, * pCut1, * pCuts0, * pCuts1;
    int        nOldSupp, Config, i, k, c = 0;
    // prepare cuts
    for ( i = 0; i <= CutNum+1; i++ )
        pSto[i] = Sto + i, pSto[i]->Cost = 0, pSto[i]->iFunc = ~0;
    // compute signatures
    pCuts0 = Jf_ObjCuts( p, Gia_ObjFaninId0(pObj, iObj) );
    Jf_ObjForEachCut( pCuts0, pCut0, i )
        Sign0[i] = Jf_CutGetSign( pCut0 );
    // compute signatures
    pCuts1 = Jf_ObjCuts( p, Gia_ObjFaninId1(pObj, iObj) );
    Jf_ObjForEachCut( pCuts1, pCut1, i )
        Sign1[i] = Jf_CutGetSign( pCut1 );
    // merge cuts
    p->CutCount[0] += pCuts0[0] * pCuts1[0];
    Jf_ObjForEachCut( pCuts0, pCut0, i )
    Jf_ObjForEachCut( pCuts1, pCut1, k )
    {
        if ( Jf_CountBits(Sign0[i] | Sign1[k]) > LutSize )
            continue;
        p->CutCount[1]++;        
        if ( !p->pPars->fCutMin )
        {
            if ( !Jf_CutMergeOrder(pCut0, pCut1, pSto[c]->pCut, LutSize) )
                continue;
            pSto[c]->Sign = Sign0[i] | Sign1[k];
        }
        else if ( p->pPars->fFuncDsd )
        {
            if ( !(Config = Jf_CutMerge2(pCut0, pCut1, pSto[c]->pCut, LutSize)) )
                continue;
            pSto[c]->Sign = Sign0[i] | Sign1[k];
            nOldSupp = pSto[c]->pCut[0];
            pSto[c]->iFunc = Sdm_ManComputeFunc( p->pDsd, Jf_ObjFunc0(pObj, pCut0), Jf_ObjFunc1(pObj, pCut1), pSto[c]->pCut, Config, 0 );
            if ( pSto[c]->iFunc == -1 )
                continue;
            if ( p->pPars->fGenCnf && Jf_CutCnfSizeF(p, Abc_Lit2Var(pSto[c]->iFunc)) >= 12 ) // no more than 15
                continue;
            assert( pSto[c]->pCut[0] <= nOldSupp );
            if ( pSto[c]->pCut[0] < nOldSupp )
                pSto[c]->Sign = Jf_CutGetSign( pSto[c]->pCut );
        }
        else
        {
            if ( !Jf_CutMergeOrder(pCut0, pCut1, pSto[c]->pCut, LutSize) )
                continue;
            pSto[c]->Sign = Sign0[i] | Sign1[k];
            nOldSupp = pSto[c]->pCut[0];
            pSto[c]->iFunc = Jf_TtComputeForCut( p, Jf_ObjFunc0(pObj, pCut0), Jf_ObjFunc1(pObj, pCut1), pCut0, pCut1, pSto[c]->pCut );
            assert( pSto[c]->pCut[0] <= nOldSupp );
            if ( pSto[c]->pCut[0] < nOldSupp )
                pSto[c]->Sign = Jf_CutGetSign( pSto[c]->pCut );
            if ( pSto[c]->iFunc >= (1 << 24) )
                printf( "Hard limit on the number of different Boolean functions (2^23) is reached. Quitting...\n" ), exit(1);
        }
        p->CutCount[2]++;
        pSto[c]->Time = p->pPars->fAreaOnly ? 0 : Jf_CutArr(p, pSto[c]->pCut);
        pSto[c]->Flow = Jf_CutFlow(p, pSto[c]->pCut);
        c = Jf_ObjAddCutToStore( p, pSto, c, CutNum );
        assert( c <= CutNum );
    }
//    Jf_ObjPrintStore( p, pSto, c );
//    Jf_ObjCheckStore( p, pSto, c, iObj );
    // add two variable cut
    if ( !Jf_ObjIsUnit(pObj) && !Jf_ObjHasCutWithSize(pSto, c, 2) )
    {
        assert( Jf_ObjIsUnit(Gia_ObjFanin0(pObj)) && Jf_ObjIsUnit(Gia_ObjFanin1(pObj)) );
        if ( p->pPars->fCutMin ) pSto[c]->iFunc = 4;  // set function (DSD only!)
        pSto[c]->pCut[0] = 2; 
        pSto[c]->pCut[1] = Jf_ObjLit(Gia_ObjFaninId0(pObj, iObj), Gia_ObjFaninC0(pObj)); 
        pSto[c]->pCut[2] = Jf_ObjLit(Gia_ObjFaninId1(pObj, iObj), Gia_ObjFaninC1(pObj)); 
        c++;
    }
    // add elementary cut
    if ( Jf_ObjIsUnit(pObj) && !(p->pPars->fCutMin && Jf_ObjHasCutWithSize(pSto, c, 1)) )
    {
        if ( p->pPars->fCutMin ) pSto[c]->iFunc = 2;  // set function
        pSto[c]->pCut[0] = 1;
        pSto[c]->pCut[1] = Jf_ObjLit(iObj, 0);
        c++;
    }
    // reorder cuts
//    Jf_ObjSortCuts( pSto + 1, c - 1 );
//    Jf_ObjCheckPtrs( pSto, CutNum );
    // find cost of the best cut
    pSto[0]->Cost = p->pPars->fGenCnf ? Jf_CutCnfSizeF(p, Abc_Lit2Var(pSto[0]->iFunc)) : 1;
    assert( pSto[0]->Cost >= 0 );
    // save best info
    assert( pSto[0]->Flow >= 0 );
    Vec_IntWriteEntry( &p->vArr,  iObj, pSto[0]->Time );
    Vec_FltWriteEntry( &p->vFlow, iObj, (pSto[0]->Flow + (fEdge ? pSto[0]->pCut[0] : pSto[0]->Cost)) / Jf_ObjRefs(p, iObj) );
    // add cuts to storage cuts
    Vec_IntClear( p->vTemp );
    Vec_IntPush( p->vTemp, c );
    for ( i = 0; i < c; i++ )
    {
        pSto[i]->Cost = p->pPars->fGenCnf ? Jf_CutCnfSizeF(p, Abc_Lit2Var(pSto[i]->iFunc)) : 1;
        Vec_IntPush( p->vTemp, Jf_CutSetAll(pSto[i]->iFunc, pSto[i]->Cost, pSto[i]->pCut[0]) );
        for ( k = 1; k <= pSto[i]->pCut[0]; k++ )
            Vec_IntPush( p->vTemp, pSto[i]->pCut[k] );
    }
    Vec_IntWriteEntry( &p->vCuts, iObj, Vec_SetAppend(&p->pMem, Vec_IntArray(p->vTemp), Vec_IntSize(p->vTemp)) );
    p->CutCount[3] += c;
}
void Jf_ManComputeCuts( Jf_Man_t * p, int fEdge )
{
    Gia_Obj_t * pObj; int i;
    if ( p->pPars->fVerbose )
    {
        printf( "Aig: CI = %d  CO = %d  AND = %d    ", Gia_ManCiNum(p->pGia), Gia_ManCoNum(p->pGia), Gia_ManAndNum(p->pGia) );
        printf( "LutSize = %d  CutMax = %d  Rounds = %d\n", p->pPars->nLutSize, p->pPars->nCutNum, p->pPars->nRounds );
        printf( "Computing cuts...\r" );
        fflush( stdout );
    }
    Gia_ManForEachObj( p->pGia, pObj, i )
    {
        if ( Gia_ObjIsCi(pObj) || Gia_ObjIsBuf(pObj) )
            Jf_ObjAssignCut( p, pObj );
        if ( Gia_ObjIsBuf(pObj) )
            Jf_ObjPropagateBuf( p, pObj, 0 );
        else if ( Gia_ObjIsAnd(pObj) )
            Jf_ObjComputeCuts( p, pObj, fEdge );
    }
    if ( p->pPars->fVerbose )
    {
        printf( "CutPair = %lu  ", (long)p->CutCount[0] );
        printf( "Merge = %lu  ",   (long)p->CutCount[1] );
        printf( "Eval = %lu  ",    (long)p->CutCount[2] );
        printf( "Cut = %lu  ",     (long)p->CutCount[3] );
        Abc_PrintTime( 1, "Time",  Abc_Clock() - p->clkStart );
        printf( "Memory:  " );
        printf( "Gia = %.2f MB  ", Gia_ManMemory(p->pGia) / (1<<20) );
        printf( "Man = %.2f MB  ", 6.0 * sizeof(int) * Gia_ManObjNum(p->pGia) / (1<<20) );
        printf( "Cuts = %.2f MB",  Vec_ReportMemory(&p->pMem) / (1<<20) );
        if ( p->nCoarse )
        printf( "   Coarse = %d (%.1f %%)",  p->nCoarse, 100.0 * p->nCoarse / Gia_ManObjNum(p->pGia) );
        printf( "\n" );
        fflush( stdout );
    }
}


/**Function*************************************************************

  Synopsis    [Computing delay/area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Jf_ManComputeDelay( Jf_Man_t * p, int fEval )
{
    Gia_Obj_t * pObj; 
    int i, Delay = 0;
    if ( fEval )
    {
        Gia_ManForEachObj( p->pGia, pObj, i )
            if ( Gia_ObjIsBuf(pObj) )
                Jf_ObjPropagateBuf( p, pObj, 0 );
            else if ( Gia_ObjIsAnd(pObj) && Gia_ObjRefNum(p->pGia, pObj) > 0 )
                Vec_IntWriteEntry( &p->vArr, i, Jf_CutArr(p, Jf_ObjCutBest(p, i)) );
    }
    Gia_ManForEachCoDriver( p->pGia, pObj, i )
    {
        assert( Gia_ObjRefNum(p->pGia, pObj) > 0 );
        Delay = Abc_MaxInt( Delay, Jf_ObjArr(p, Gia_ObjId(p->pGia, pObj)) );
    }
    return Delay;
}
int Jf_ManComputeRefs( Jf_Man_t * p )
{
    Gia_Obj_t * pObj; 
    float nRefsNew; int i, * pCut;
    float * pRefs = Vec_FltArray(&p->vRefs);
    float * pFlow = Vec_FltArray(&p->vFlow);
    assert( p->pGia->pRefs != NULL );
    memset( p->pGia->pRefs, 0, sizeof(int) * Gia_ManObjNum(p->pGia) );
    p->pPars->Area = p->pPars->Edge = 0;
    Gia_ManForEachObjReverse( p->pGia, pObj, i )
    {
        if ( Gia_ObjIsCo(pObj) || Gia_ObjIsBuf(pObj) )
            Gia_ObjRefInc( p->pGia, Gia_ObjFanin0(pObj) );
        else if ( Gia_ObjIsAnd(pObj) && Gia_ObjRefNum(p->pGia, pObj) > 0 )
        {
            assert( Jf_ObjIsUnit(pObj) );
            pCut = Jf_ObjCutBest(p, i);
            Jf_CutRef( p, pCut );
            if ( p->pPars->fGenCnf )
            p->pPars->Clause += Jf_CutCnfSize(p, pCut);
            p->pPars->Edge += Jf_CutSize(pCut);
            p->pPars->Area++;
        }
    }
    // blend references and normalize flow
    for ( i = 0; i < Gia_ManObjNum(p->pGia); i++ )
    {
        if ( p->pPars->fOptEdge )
            nRefsNew = Abc_MaxFloat( 1, 0.8 * pRefs[i] + 0.2 * p->pGia->pRefs[i] );
        else
            nRefsNew = Abc_MaxFloat( 1, 0.2 * pRefs[i] + 0.8 * p->pGia->pRefs[i] );
        pFlow[i] = pFlow[i] * pRefs[i] / nRefsNew;
        pRefs[i] = nRefsNew;
        assert( pFlow[i] >= 0 );
    }
    // compute delay
    p->pPars->Delay = Jf_ManComputeDelay( p, 1 );
    return p->pPars->Area;
}

/**Function*************************************************************

  Synopsis    [Mapping rounds.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Jf_ObjComputeBestCut( Jf_Man_t * p, Gia_Obj_t * pObj, int fEdge, int fEla )
{
    int i, iObj = Gia_ObjId( p->pGia, pObj );
    int * pCuts = Jf_ObjCuts( p, iObj );
    int * pCut, * pCutBest = NULL;
    int Time = ABC_INFINITY, TimeBest = ABC_INFINITY;
    float Area, AreaBest = ABC_INFINITY;
    Jf_ObjForEachCut( pCuts, pCut, i )
    {
        if ( Jf_CutIsTriv(pCut, iObj) ) continue;
        if ( fEdge && !fEla ) 
            Jf_CutSetCost(pCut, Jf_CutSize(pCut));
        Area = fEla ? Jf_CutArea(p, pCut, fEdge) : Jf_CutFlow(p, pCut) + Jf_CutCost(pCut);
        if ( pCutBest == NULL || AreaBest > Area + JF_EPSILON || (AreaBest > Area - JF_EPSILON && TimeBest > (Time = Jf_CutArr(p, pCut))) )
            pCutBest = pCut, AreaBest = Area, TimeBest = Time;
    }
    Vec_IntWriteEntry( &p->vArr,  iObj, Jf_CutArr(p, pCutBest) );
    if ( !fEla )
    Vec_FltWriteEntry( &p->vFlow, iObj, AreaBest / Jf_ObjRefs(p, iObj) );
    Jf_ObjSetBestCut( pCuts, pCutBest, p->vTemp );
//    Jf_CutPrint( Jf_ObjCutBest(p, iObj) ); printf( "\n" );
}
void Jf_ManPropagateFlow( Jf_Man_t * p, int fEdge )
{
    Gia_Obj_t * pObj; 
    int i;
    Gia_ManForEachObj( p->pGia, pObj, i )
        if ( Gia_ObjIsBuf(pObj) )
            Jf_ObjPropagateBuf( p, pObj, 0 );
        else if ( Gia_ObjIsAnd(pObj) && Jf_ObjIsUnit(pObj) )
            Jf_ObjComputeBestCut( p, pObj, fEdge, 0 );
    Jf_ManComputeRefs( p );
}
void Jf_ManPropagateEla( Jf_Man_t * p, int fEdge )
{
    Gia_Obj_t * pObj; 
    int i, CostBef, CostAft;
    p->pPars->Area = p->pPars->Edge = p->pPars->Clause = 0;
    Gia_ManForEachObjReverse( p->pGia, pObj, i )
        if ( Gia_ObjIsBuf(pObj) )
            Jf_ObjPropagateBuf( p, pObj, 1 );
        else if ( Gia_ObjIsAnd(pObj) && Gia_ObjRefNum(p->pGia, pObj) > 0 )
        {
            assert( Jf_ObjIsUnit(pObj) );
            if ( Jf_CutCheckMffc(p, Jf_ObjCutBest(p, i), 50) )
            {
                CostBef = Jf_CutDeref_rec( p, Jf_ObjCutBest(p, i) );
                Jf_ObjComputeBestCut( p, pObj, fEdge, 1 );
                CostAft = Jf_CutRef_rec( p, Jf_ObjCutBest(p, i) );
    //            if ( CostBef != CostAft )  printf( "%d -> %d   ", CostBef, CostAft );
                assert( CostBef >= CostAft ); // does not hold because of JF_EDGE_LIM
            }
            if ( p->pPars->fGenCnf )
            p->pPars->Clause += Jf_CutCnfSize(p, Jf_ObjCutBest(p, i));
            p->pPars->Edge += Jf_CutSize(Jf_ObjCutBest(p, i));
            p->pPars->Area++;
        }
    p->pPars->Delay = Jf_ManComputeDelay( p, 1 );
//    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Derives the result of mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Jf_ManDeriveMappingGia( Jf_Man_t * p )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj; 
    Vec_Int_t * vCopies   = Vec_IntStartFull( Gia_ManObjNum(p->pGia) );
    Vec_Int_t * vMapping  = Vec_IntStart( 2 * Gia_ManObjNum(p->pGia) + (int)p->pPars->Edge + 2 * (int)p->pPars->Area );
    Vec_Int_t * vMapping2 = Vec_IntStart( (int)p->pPars->Edge + 2 * (int)p->pPars->Area + 1000 );
    Vec_Int_t * vCover    = Vec_IntAlloc( 1 << 16 );
    Vec_Int_t * vLeaves   = Vec_IntAlloc( 16 );
    Vec_Int_t * vLits = NULL, * vClas = NULL;
    int i, k, iLit, Class, * pCut;
    word uTruth = 0, * pTruth = &uTruth;
    assert( p->pPars->fCutMin );
    if ( p->pPars->fGenCnf )
    {
        vLits = Vec_IntAlloc( 1000 );
        vClas = Vec_IntAlloc( 1000 );
        Vec_IntPush( vClas, Vec_IntSize(vLits) );
        Vec_IntPush( vLits, 1 );
    }
    // create new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p->pGia) );
    pNew->pName = Abc_UtilStrsav( p->pGia->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pGia->pSpec );
    // map primary inputs
    Vec_IntWriteEntry( vCopies, 0, 0 );
    Gia_ManForEachCi( p->pGia, pObj, i )
        Vec_IntWriteEntry( vCopies, Gia_ObjId(p->pGia, pObj), Gia_ManAppendCi(pNew) );
    // iterate through nodes used in the mapping
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
       if ( Gia_ObjIsBuf(pObj) || Gia_ObjRefNum(p->pGia, pObj) == 0 )
            continue;
        pCut = Jf_ObjCutBest( p, i );
//        printf( "Best cut of node %d:  ", i );  Jf_CutPrint(pCut);
        Class = Jf_CutFuncClass( pCut );
        if ( Jf_CutSize(pCut) == 0 )
        {
            assert( Class == 0 );
            Vec_IntWriteEntry( vCopies, i, Jf_CutFunc(pCut) );
            continue;
        }
        if ( Jf_CutSize(pCut) == 1 )
        {
            assert( Class == 1 );
            iLit = Abc_LitNotCond( Jf_CutLit(pCut, 1) , Jf_CutFuncCompl(pCut) );
            iLit = Abc_Lit2LitL( Vec_IntArray(vCopies), iLit );
            Vec_IntWriteEntry( vCopies, i, iLit );
            continue;
        }
        if ( p->pPars->fFuncDsd )
            uTruth = Sdm_ManReadDsdTruth(p->pDsd, Class);
        else
            pTruth = Vec_MemReadEntry(p->vTtMem, Class);
        assert( p->pDsd == NULL || Sdm_ManReadDsdVarNum(p->pDsd, Class) == Jf_CutSize(pCut) );
        // collect leaves
        Vec_IntClear( vLeaves );
        Jf_CutForEachLit( pCut, iLit, k )
            Vec_IntPush( vLeaves, Abc_Lit2LitL(Vec_IntArray(vCopies), iLit) );
        // create GIA
        iLit = Kit_TruthToGia( pNew, (unsigned *)pTruth, Vec_IntSize(vLeaves), vCover, vLeaves, 0 );
        if ( p->pPars->fGenCnf )
            Jf_ManGenCnf( uTruth, iLit, vLeaves, vLits, vClas, vCover );
        iLit = Abc_LitNotCond( iLit, Jf_CutFuncCompl(pCut) );
        Vec_IntWriteEntry( vCopies, i, iLit );
        // create mapping
        Vec_IntSetEntry( vMapping, Abc_Lit2Var(iLit), Vec_IntSize(vMapping2) );
        Vec_IntPush( vMapping2, Vec_IntSize(vLeaves) );
        Vec_IntForEachEntry( vLeaves, iLit, k )
            Vec_IntPush( vMapping2, Abc_Lit2Var(iLit) );
        Vec_IntPush( vMapping2, Abc_Lit2Var(Vec_IntEntry(vCopies, i)) );
    }
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        if ( p->pPars->fGenCnf )
            Vec_IntClear( vLeaves );
        iLit = Vec_IntEntry( vCopies, Gia_ObjFaninId0p(p->pGia, pObj) );
        if ( p->pPars->fGenCnf )
            Vec_IntPush( vLeaves, Abc_LitNotCond(iLit, Gia_ObjFaninC0(pObj)) );
        iLit = Gia_ManAppendCo( pNew, Abc_LitNotCond(iLit, Gia_ObjFaninC0(pObj)) );
        if ( p->pPars->fGenCnf )
            Jf_ManGenCnf( ABC_CONST(0xAAAAAAAAAAAAAAAA), iLit, vLeaves, vLits, vClas, vCover );
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
    // derive CNF
    if ( p->pPars->fGenCnf )
    {
        if ( p->pPars->fCnfObjIds )
            pNew->pData = Jf_ManCreateCnf( pNew, vLits, vClas );
        else
            pNew->pData = Jf_ManCreateCnfRemap( pNew, vLits, vClas, p->pPars->fAddOrCla );
    }
    Vec_IntFreeP( &vLits );
    Vec_IntFreeP( &vClas );
    return pNew;
}
void Jf_ManDeriveMapping( Jf_Man_t * p )
{
    Vec_Int_t * vMapping;
    Gia_Obj_t * pObj; 
    int i, k, * pCut;
    assert( !p->pPars->fCutMin );
    vMapping = Vec_IntAlloc( Gia_ManObjNum(p->pGia) + (int)p->pPars->Edge + (int)p->pPars->Area * 2 );
    Vec_IntFill( vMapping, Gia_ManObjNum(p->pGia), 0 );
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
        if ( Gia_ObjIsBuf(pObj) || Gia_ObjRefNum(p->pGia, pObj) == 0 )
            continue;
        pCut = Jf_ObjCutBest( p, i );
        Vec_IntWriteEntry( vMapping, i, Vec_IntSize(vMapping) );
        assert( !p->pPars->fCutMin || Jf_CutSize(pCut) <= 6 );
        Vec_IntPush( vMapping, Jf_CutSize(pCut) );
        for ( k = 1; k <= Jf_CutSize(pCut); k++ )
            Vec_IntPush( vMapping, Jf_CutVar(pCut, k) );
        Vec_IntPush( vMapping, i );
    }
    assert( Vec_IntCap(vMapping) == 16 || Vec_IntSize(vMapping) == Vec_IntCap(vMapping) );
    p->pGia->vMapping = vMapping;
//    Gia_ManMappingVerify( p->pGia );
}

/**Function*************************************************************

  Synopsis    [Derive GIA without mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Jf_ManDeriveGia( Jf_Man_t * p )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj; 
    Vec_Int_t * vCopies   = Vec_IntStartFull( Gia_ManObjNum(p->pGia) );
    Vec_Int_t * vCover    = Vec_IntAlloc( 1 << 16 );
    Vec_Int_t * vLeaves   = Vec_IntAlloc( 16 );
    int i, k, iLit, Class, * pCut;
    int nWords = Abc_Truth6WordNum(p->pPars->nLutSize);
    word uTruth = 0, * pTruth = &uTruth, Truth[JF_WORD_MAX];
    // create new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p->pGia) );
    pNew->pName = Abc_UtilStrsav( p->pGia->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pGia->pSpec );
    pNew->vLevels = Vec_IntStart( 6*Gia_ManObjNum(p->pGia)/5 + 100 );
    // map primary inputs
    Vec_IntWriteEntry( vCopies, 0, 0 );
    Gia_ManForEachCi( p->pGia, pObj, i )
        Vec_IntWriteEntry( vCopies, Gia_ObjId(p->pGia, pObj), Gia_ManAppendCi(pNew) );
    // iterate through nodes used in the mapping
    if ( !p->pPars->fCutMin )
        Gia_ObjComputeTruthTableStart( p->pGia, p->pPars->nLutSize );
    Gia_ManHashStart( pNew );
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
       if ( Gia_ObjIsBuf(pObj) || Gia_ObjRefNum(p->pGia, pObj) == 0 )
            continue;
        pCut = Jf_ObjCutBest( p, i );
//        printf( "Best cut of node %d:  ", i );  Jf_CutPrint(pCut);
        // get the truth table
        if ( p->pPars->fCutMin )
        {
            Class = Jf_CutFuncClass( pCut );
            if ( Jf_CutSize(pCut) == 0 )
            {
                assert( Class == 0 );
                Vec_IntWriteEntry( vCopies, i, Jf_CutFunc(pCut) );
                continue;
            }
            if ( Jf_CutSize(pCut) == 1 )
            {
                assert( Class == 1 );
                iLit = Abc_LitNotCond( Jf_CutLit(pCut, 1) , Jf_CutFuncCompl(pCut) );
                iLit = Abc_Lit2LitL( Vec_IntArray(vCopies), iLit );
                Vec_IntWriteEntry( vCopies, i, iLit );
                continue;
            }
            if ( p->pPars->fFuncDsd )
                uTruth = Sdm_ManReadDsdTruth(p->pDsd, Class);
            else
                Abc_TtCopy( (pTruth = Truth), Vec_MemReadEntry(p->vTtMem, Class), nWords, 0 );
            assert( p->pDsd == NULL || Sdm_ManReadDsdVarNum(p->pDsd, Class) == Jf_CutSize(pCut) );
        }
        else
        {
            Vec_IntClear( vLeaves );
            Jf_CutForEachLit( pCut, iLit, k )
                Vec_IntPush( vLeaves, Abc_Lit2Var(iLit) );
            pTruth = Gia_ObjComputeTruthTableCut( p->pGia, pObj, vLeaves );
        }
        // collect incoming literals
        Vec_IntClear( vLeaves );
        Jf_CutForEachLit( pCut, iLit, k )
            Vec_IntPush( vLeaves, Abc_Lit2LitL(Vec_IntArray(vCopies), iLit) );
        // create GIA
        iLit = Dsm_ManTruthToGia( pNew, pTruth, vLeaves, vCover );
        iLit = Abc_LitNotCond( iLit, (p->pPars->fCutMin && Jf_CutFuncCompl(pCut)) );
        Vec_IntWriteEntry( vCopies, i, iLit );
    }
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        iLit = Vec_IntEntry( vCopies, Gia_ObjFaninId0p(p->pGia, pObj) );
        Gia_ManAppendCo( pNew, Abc_LitNotCond(iLit, Gia_ObjFaninC0(pObj)) );
    }
    if ( !p->pPars->fCutMin )
        Gia_ObjComputeTruthTableStop( p->pGia );
    Vec_IntFree( vCopies );
    Vec_IntFree( vLeaves );
    Vec_IntFree( vCover );
    Gia_ManHashStop( pNew );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p->pGia) );
//    Dsm_ManReportStats();
    // perform cleanup
    if ( !p->pPars->fCutMin )
    {
        pNew = Gia_ManCleanup( pTemp = pNew );
        Gia_ManStop( pTemp );
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Jf_ManSetDefaultPars( Jf_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Jf_Par_t) );
    pPars->nLutSize     =  6;
    pPars->nCutNum      =  8;
    pPars->nRounds      =  1;
    pPars->nVerbLimit   =  5;
    pPars->DelayTarget  = -1;
    pPars->fAreaOnly    =  1;
    pPars->fOptEdge     =  1; 
    pPars->fCoarsen     =  0;
    pPars->fCutMin      =  0;
    pPars->fFuncDsd     =  0;
    pPars->fGenCnf      =  0;
    pPars->fPureAig     =  0;
    pPars->fVerbose     =  0;
    pPars->fVeryVerbose =  0;
    pPars->nLutSizeMax  =  JF_LEAF_MAX;
    pPars->nCutNumMax   =  JF_CUT_MAX;
}
void Jf_ManPrintStats( Jf_Man_t * p, char * pTitle )
{
    if ( !p->pPars->fVerbose )
        return;
    printf( "%s :  ", pTitle );
    printf( "Level =%6lu   ", (long)p->pPars->Delay );
    printf( "Area =%9lu   ",  (long)p->pPars->Area );
    printf( "Edge =%9lu   ",  (long)p->pPars->Edge );
    if ( p->pPars->fGenCnf )
    printf( "Cnf =%9lu   ",   (long)p->pPars->Clause );
    Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
    fflush( stdout );
}
Gia_Man_t * Jf_ManPerformMapping( Gia_Man_t * pGia, Jf_Par_t * pPars )
{
    Gia_Man_t * pNew = pGia;
    Jf_Man_t * p; int i;
    assert( !Gia_ManBufNum(pGia) );
    assert( !pPars->fCutMin || !pPars->fFuncDsd || pPars->nLutSize <= 6 );
    if ( pPars->fGenCnf )
        pPars->fCutMin = 1, pPars->fFuncDsd = 1, pPars->fOptEdge = 0;
    if ( pPars->fCutMin && !pPars->fFuncDsd )
        pPars->fCoarsen = 0;
    p = Jf_ManAlloc( pGia, pPars );
    p->pCutCmp = pPars->fAreaOnly ? Jf_CutCompareArea : Jf_CutCompareDelay;
    Jf_ManComputeCuts( p, 0 );
    Jf_ManComputeRefs( p );                         Jf_ManPrintStats( p, "Start" );
    for ( i = 0; i < pPars->nRounds; i++ )
    {
        if ( !p->pPars->fGenCnf )
        {
        Jf_ManPropagateFlow( p, pPars->fOptEdge );  Jf_ManPrintStats( p, "Flow " );
        }
        Jf_ManPropagateEla( p, 0 );                 Jf_ManPrintStats( p, "Area " );
        Jf_ManPropagateEla( p, 1 );                 Jf_ManPrintStats( p, "Edge " );
    }
    if ( p->pPars->fVeryVerbose && p->pPars->fCutMin && !p->pPars->fFuncDsd )
        Vec_MemDumpTruthTables( p->vTtMem, Gia_ManName(p->pGia), p->pPars->nLutSize );
    if ( p->pPars->fPureAig )
        pNew = Jf_ManDeriveGia(p);
    else if ( p->pPars->fCutMin )
        pNew = Jf_ManDeriveMappingGia(p);
    else
        Jf_ManDeriveMapping(p);
    Jf_ManFree( p );
    return pNew;
}
Gia_Man_t * Jf_ManDeriveCnf( Gia_Man_t * p, int fCnfObjIds )
{
    Jf_Par_t Pars, * pPars = &Pars;
    Jf_ManSetDefaultPars( pPars );
    pPars->fGenCnf = 1;
    pPars->fCnfObjIds = fCnfObjIds;
    return Jf_ManPerformMapping( p, pPars );
}
Gia_Man_t * Jf_ManDeriveCnfMiter( Gia_Man_t * p, int fVerbose )
{
    Jf_Par_t Pars, * pPars = &Pars;
    Jf_ManSetDefaultPars( pPars );
    pPars->fGenCnf = 1;
    pPars->fCnfObjIds = 0;
    pPars->fAddOrCla = 1;
    pPars->fVerbose = fVerbose;
    return Jf_ManPerformMapping( p, pPars );
}
void Jf_ManDumpCnf( Gia_Man_t * p, char * pFileName, int fVerbose )
{
    abctime clk = Abc_Clock();
    Gia_Man_t * pNew;
    Cnf_Dat_t * pCnf;
    pNew = Jf_ManDeriveCnfMiter( p, fVerbose );
    pCnf = (Cnf_Dat_t *)pNew->pData; pNew->pData = NULL;
    Cnf_DataWriteIntoFile( pCnf, pFileName, 0, NULL, NULL );
    Gia_ManStop( pNew );
//    if ( fVerbose )
    {
        printf( "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d. ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    Cnf_DataFree(pCnf);
}

void Jf_ManTestCnf( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Cnf_Dat_t * pCnf;
    int i;
//    Cnf_Dat_t * pCnf = Cnf_DeriveGia( p );
    pNew = Jf_ManDeriveCnf( p, 1 );
    pCnf = (Cnf_Dat_t *)pNew->pData; pNew->pData = NULL;
    Cnf_DataWriteIntoFile( pCnf, "test.cnf", 0, NULL, NULL );
    for ( i = 0; i < pCnf->nVars; i++ )
        printf( "%d : %d %d\n", i, pCnf->pObj2Count[i], pCnf->pObj2Clause[i] );
    Gia_ManStop( pNew );
    Cnf_DataFree(pCnf);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

