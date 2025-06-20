/**CFile****************************************************************

  FileName    [dauTree.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware unmapping.]

  Synopsis    [Canonical DSD package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: dauTree.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "dauInt.h"
#include "misc/mem/mem.h"
#include "misc/util/utilTruth.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Dss_Fun_t_ Dss_Fun_t;
struct Dss_Fun_t_
{
    unsigned       iDsd  :    26;  // DSD literal
    unsigned       nFans :     6;  // fanin count
    unsigned char  pFans[0];       // fanins
};

typedef struct Dss_Ent_t_ Dss_Ent_t;
struct Dss_Ent_t_
{
    Dss_Fun_t *    pFunc;
    Dss_Ent_t *    pNext;
    unsigned       iDsd0  :   27;  // dsd entry
    unsigned       nWords :    5;  // total word count (struct + shared)
    unsigned       iDsd1  :   27;  // dsd entry
    unsigned       nShared:    5;  // shared count
    unsigned char  pShared[0];     // shared literals
};

typedef struct Dss_Obj_t_ Dss_Obj_t;
struct Dss_Obj_t_
{
    unsigned       Id;             // node ID
    unsigned       Type    :   3;  // node type
    unsigned       nSupp   :   8;  // variable
    unsigned       iVar    :   8;  // variable
    unsigned       nWords  :   6;  // variable
    unsigned       fMark0  :   1;  // user mark
    unsigned       fMark1  :   1;  // user mark
    unsigned       nFans   :   5;  // fanin count
    unsigned       pFans[0];       // fanins
};

typedef struct Dss_Ntk_t_ Dss_Ntk_t;
struct Dss_Ntk_t_
{
    int            nVars;          // the number of variables
    int            nMem;           // memory used
    int            nMemAlloc;      // memory allocated
    word *         pMem;           // memory array
    Dss_Obj_t *    pRoot;          // root node
    Vec_Ptr_t *    vObjs;          // internal nodes
};

struct Dss_Man_t_
{
    int            nVars;          // variable number
    int            nNonDecLimit;   // limit on non-dec size
    int            nBins;          // table size
    unsigned *     pBins;          // hash table
    Mem_Flex_t *   pMem;           // memory for nodes
    Vec_Ptr_t *    vObjs;          // objects
    Vec_Int_t *    vNexts;         // next pointers
    Vec_Int_t *    vLeaves;        // temp
    Vec_Int_t *    vCopies;        // temp
    word **        pTtElems;       // elementary TTs
    Dss_Ent_t **   pCache;         // decomposition cache
    int            nCache;         // size of decomposition cache
    Mem_Flex_t *   pMemEnts;       // memory for cache entries
    int            nCacheHits[2];
    int            nCacheMisses[2];
    int            nCacheEntries[2];
    abctime        timeBeg;
    abctime        timeDec;
    abctime        timeLook;
    abctime        timeEnd;
};

static inline Dss_Obj_t *  Dss_Regular( Dss_Obj_t * p )                            { return (Dss_Obj_t *)((ABC_PTRUINT_T)(p) & ~01);                                    }
static inline Dss_Obj_t *  Dss_Not( Dss_Obj_t * p )                                { return (Dss_Obj_t *)((ABC_PTRUINT_T)(p) ^  01);                                    }
static inline Dss_Obj_t *  Dss_NotCond( Dss_Obj_t * p, int c )                     { return (Dss_Obj_t *)((ABC_PTRUINT_T)(p) ^ (c));                                    }
static inline int          Dss_IsComplement( Dss_Obj_t * p )                       { return (int)((ABC_PTRUINT_T)(p) & 01);                                             }

static inline int          Dss_EntWordNum( Dss_Ent_t * p )                         { return sizeof(Dss_Ent_t) / 8 + p->nShared / 4 + ((p->nShared & 3) > 0);            }
static inline int          Dss_FunWordNum( Dss_Fun_t * p )                         { assert(p->nFans >= 2); return (p->nFans + 4) / 8 + (((p->nFans + 4) & 7) > 0);     }
static inline int          Dss_ObjWordNum( int nFans )                             { return sizeof(Dss_Obj_t) / 8 + nFans / 2 + ((nFans & 1) > 0);                      }
static inline word *       Dss_ObjTruth( Dss_Obj_t * pObj )                        { return (word *)pObj + pObj->nWords;                                                }

static inline void         Dss_ObjClean( Dss_Obj_t * pObj )                        { memset( pObj, 0, sizeof(Dss_Obj_t) );                                              }
static inline int          Dss_ObjId( Dss_Obj_t * pObj )                           { return pObj->Id;                                                                   }
static inline int          Dss_ObjType( Dss_Obj_t * pObj )                         { return pObj->Type;                                                                 }
static inline int          Dss_ObjSuppSize( Dss_Obj_t * pObj )                     { return pObj->nSupp;                                                                }
static inline int          Dss_ObjFaninNum( Dss_Obj_t * pObj )                     { return pObj->nFans;                                                                }
static inline int          Dss_ObjFaninC( Dss_Obj_t * pObj, int i )                { assert(i < (int)pObj->nFans); return Abc_LitIsCompl(pObj->pFans[i]);               }

static inline Dss_Obj_t *  Dss_VecObj( Vec_Ptr_t * p, int Id )                     { return (Dss_Obj_t *)Vec_PtrEntry(p, Id);                                           }
static inline Dss_Obj_t *  Dss_VecConst0( Vec_Ptr_t * p )                          { return Dss_VecObj( p, 0 );                                                         }
static inline Dss_Obj_t *  Dss_VecVar( Vec_Ptr_t * p, int v )                      { return Dss_VecObj( p, v+1 );                                                       }
static inline int          Dss_VecLitSuppSize( Vec_Ptr_t * p, int iLit )           { return Dss_VecObj( p, Abc_Lit2Var(iLit) )->nSupp;                                  }

static inline int          Dss_Obj2Lit( Dss_Obj_t * pObj )                         { return Abc_Var2Lit(Dss_Regular(pObj)->Id, Dss_IsComplement(pObj));                 }
static inline Dss_Obj_t *  Dss_Lit2Obj( Vec_Ptr_t * p, int iLit )                  { return Dss_NotCond(Dss_VecObj(p, Abc_Lit2Var(iLit)), Abc_LitIsCompl(iLit));        }
static inline Dss_Obj_t *  Dss_ObjFanin( Vec_Ptr_t * p, Dss_Obj_t * pObj, int i )  { assert(i < (int)pObj->nFans); return Dss_VecObj(p, Abc_Lit2Var(pObj->pFans[i]));   }
static inline Dss_Obj_t *  Dss_ObjChild( Vec_Ptr_t * p, Dss_Obj_t * pObj, int i )  { assert(i < (int)pObj->nFans); return Dss_Lit2Obj(p, pObj->pFans[i]);               }

#define Dss_VecForEachObj( vVec, pObj, i )                \
    Vec_PtrForEachEntry( Dss_Obj_t *, vVec, pObj, i )
#define Dss_VecForEachObjVec( vLits, vVec, pObj, i )      \
    for ( i = 0; (i < Vec_IntSize(vLits)) && ((pObj) = Dss_Lit2Obj(vVec, Vec_IntEntry(vLits,i))); i++ )
#define Dss_VecForEachNode( vVec, pObj, i )               \
    Vec_PtrForEachEntry( Dss_Obj_t *, vVec, pObj, i )     \
        if ( pObj->Type == DAU_DSD_CONST0 || pObj->Type == DAU_DSD_VAR ) {} else
#define Dss_ObjForEachFanin( vVec, pObj, pFanin, i )      \
    for ( i = 0; (i < Dss_ObjFaninNum(pObj)) && ((pFanin) = Dss_ObjFanin(vVec, pObj, i)); i++ )
#define Dss_ObjForEachChild( vVec, pObj, pFanin, i )      \
    for ( i = 0; (i < Dss_ObjFaninNum(pObj)) && ((pFanin) = Dss_ObjChild(vVec, pObj, i)); i++ )

static inline int Dss_WordCountOnes( unsigned uWord )
{
    uWord = (uWord & 0x55555555) + ((uWord>>1) & 0x55555555);
    uWord = (uWord & 0x33333333) + ((uWord>>2) & 0x33333333);
    uWord = (uWord & 0x0F0F0F0F) + ((uWord>>4) & 0x0F0F0F0F);
    uWord = (uWord & 0x00FF00FF) + ((uWord>>8) & 0x00FF00FF);
    return  (uWord & 0x0000FFFF) + (uWord>>16);
}

static inline int Dss_Lit2Lit( int * pMapLit, int Lit )   { return Abc_Var2Lit( Abc_Lit2Var(pMapLit[Abc_Lit2Var(Lit)]), Abc_LitIsCompl(Lit) ^ Abc_LitIsCompl(pMapLit[Abc_Lit2Var(Lit)]) );   }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

#if 0

/**Function*************************************************************

  Synopsis    [Check decomposability for 666.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// recursively collects 6-feasible supports
int Dss_ObjCheck666_rec( Dss_Ntk_t * p, Dss_Obj_t * pObj, Vec_Int_t * vSupps )
{
    Dss_Obj_t * pFanin;
    int i, uSupp = 0;
    assert( !Dss_IsComplement(pObj) );
    if ( pObj->Type == DAU_DSD_VAR )
    {
        assert( pObj->iVar >= 0 && pObj->iVar < 30 );
        return (1 << pObj->iVar);
    }
    if ( pObj->Type == DAU_DSD_AND || pObj->Type == DAU_DSD_XOR )
    {
        int c0, c1, c2, uSuppTemp;
        int uSuppVars[16];
        int nSuppVars = 0;
        int nFanins = Dss_ObjFaninNum(pObj);
        int uSupps[16], nSuppSizes[16];
        Dss_ObjForEachFanin( p->vObjs, pObj, pFanin, i )
        {
            uSupps[i] = Dss_ObjCheck666_rec( p, pFanin, vSupps );
            nSuppSizes[i] = Dss_WordCountOnes( uSupps[i] );
            uSupp |= uSupps[i];
            if ( nSuppSizes[i] == 1 )
                uSuppVars[nSuppVars++] = uSupps[i];
        }
        // iterate through the permutations
        for ( c0 = 0; c0 < nFanins; c0++ )
        if ( nSuppSizes[c0] > 1 && nSuppSizes[c0] < 6 )
        {
            uSuppTemp = uSupps[c0];
            for ( i = 0; i < nSuppVars; i++ )
                if ( nSuppSizes[c0] + i < 6 )
                    uSuppTemp |= uSuppVars[i];
                else
                    break;
            if ( Dss_WordCountOnes(uSuppTemp) <= 6 )
                Vec_IntPush( vSupps, uSuppTemp );

            for ( c1 = c0 + 1; c1 < nFanins; c1++ )
            if ( nSuppSizes[c1] > 1 && nSuppSizes[c1] < 6 )
            {
                if ( nSuppSizes[c0] + nSuppSizes[c1] <= 6 )
                    Vec_IntPush( vSupps, uSupps[c0] | uSupps[c1] );

                uSuppTemp = uSupps[c0] | uSupps[c1];
                for ( i = 0; i < nSuppVars; i++ )
                    if ( nSuppSizes[c0] + nSuppSizes[c1] + i < 6 )
                        uSuppTemp |= uSuppVars[i];
                    else
                        break;
                if ( Dss_WordCountOnes(uSuppTemp) <= 6 )
                    Vec_IntPush( vSupps, uSuppTemp );

                for ( c2 = c1 + 1; c2 < nFanins; c2++ )
                if ( nSuppSizes[c2] > 1 && nSuppSizes[c2] < 6 )
                {
                    if ( nSuppSizes[c0] + nSuppSizes[c1] + nSuppSizes[c2] <= 6 )
                        Vec_IntPush( vSupps, uSupps[c0] | uSupps[c1] | uSupps[c2] );
                    assert( nSuppSizes[c0] + nSuppSizes[c1] + nSuppSizes[c2] >= 6 );
                }
            }
        }
        if ( nSuppVars > 1 && nSuppVars <= 6 )
        {
            uSuppTemp = 0;
            for ( i = 0; i < nSuppVars; i++ )
                uSuppTemp |= uSuppVars[i];
            Vec_IntPush( vSupps, uSuppTemp );
        }
        else if ( nSuppVars > 6 && nSuppVars <= 12 )
        {
            uSuppTemp = 0;
            for ( i = 0; i < 6; i++ )
                uSuppTemp |= uSuppVars[i];
            Vec_IntPush( vSupps, uSuppTemp );

            uSuppTemp = 0;
            for ( i = 6; i < nSuppVars; i++ )
                uSuppTemp |= uSuppVars[i];
            Vec_IntPush( vSupps, uSuppTemp );
        }
    }
    else if ( pObj->Type == DAU_DSD_MUX || pObj->Type == DAU_DSD_PRIME )
    {
        Dss_ObjForEachFanin( p->vObjs, pObj, pFanin, i )
            uSupp |= Dss_ObjCheck666_rec( p, pFanin, vSupps );
    }
    if ( Dss_WordCountOnes( uSupp ) <= 6 )
        Vec_IntPush( vSupps, uSupp );
    return uSupp;
}
int Dss_ObjCheck666( Dss_Ntk_t * p )
{
    Vec_Int_t * vSupps;
    int i, k, SuppI, SuppK;
    int nSupp = Dss_ObjSuppSize(Dss_Regular(p->pRoot));
    if ( nSupp <= 6 )
        return 1;
    // compute supports
    vSupps = Vec_IntAlloc( 100 );
    Dss_ObjCheck666_rec( p, Dss_Regular(p->pRoot), vSupps );
    Vec_IntUniqify( vSupps );
    Vec_IntForEachEntry( vSupps, SuppI, i )
    {
        k = Dss_WordCountOnes(SuppI);
        assert( k > 0 && k <= 6 );
/*
        for ( k = 0; k < 16; k++ )
            if ( (SuppI >> k) & 1 )
                printf( "%c", 'a' + k );
            else
                printf( "-" );
        printf( "\n" );
*/
    }
    // consider support pairs
    Vec_IntForEachEntry( vSupps, SuppI, i )
    Vec_IntForEachEntryStart( vSupps, SuppK, k, i+1 )
    {
        if ( SuppI & SuppK )
            continue;
        if ( Dss_WordCountOnes(SuppI | SuppK) + 4 >= nSupp )
        {
            Vec_IntFree( vSupps );
            return 1;
        }
    }
    Vec_IntFree( vSupps );
    return 0;
}
void Dau_DsdTest_()
{
/*
    extern Dss_Ntk_t * Dss_NtkCreate( char * pDsd, int nVars, word * pTruth );
    extern void Dss_NtkFree( Dss_Ntk_t * p );

//    char * pDsd = "(!(amn!(bh))[cdeij]!(fklg)o)";
    char * pDsd = "<[(ab)(cd)(ef)][(gh)(ij)(kl)](mn)>";
    Dss_Ntk_t * pNtk = Dss_NtkCreate( pDsd, 16, NULL );
    int Status = Dss_ObjCheck666( pNtk );
    Dss_NtkFree( pNtk );
*/
}

abctime if_dec_time;

void Dau_DsdCheckStructOne( word * pTruth, int nVars, int nLeaves )
{
    extern Dss_Ntk_t * Dss_NtkCreate( char * pDsd, int nVars, word * pTruth );
    extern void Dss_NtkFree( Dss_Ntk_t * p );

    static abctime timeTt  = 0;
    static abctime timeDsd = 0;
    abctime clkTt, clkDsd;

    char pDsd[1000];
    word Truth[1024];
    Dss_Ntk_t * pNtk;
    int Status, nNonDec;

    if ( pTruth == NULL )
    {
        Abc_PrintTime( 1, "TT  runtime", timeTt );
        Abc_PrintTime( 1, "DSD runtime", timeDsd );
        Abc_PrintTime( 1, "Total      ", if_dec_time );

        if_dec_time = 0;
        timeTt = 0;
        timeDsd = 0;
        return;
    }

    Abc_TtCopy( Truth, pTruth, Abc_TtWordNum(nVars), 0 );
    nNonDec = Dau_DsdDecompose( Truth, nVars, 0, 0, pDsd );
    if ( nNonDec > 0 )
        return;

    pNtk = Dss_NtkCreate( pDsd, 16, NULL );

    // measure DSD runtime
    clkDsd = Abc_Clock();
    Status = Dss_ObjCheck666( pNtk );
    timeDsd += Abc_Clock() - clkDsd;

    Dss_NtkFree( pNtk );

    // measure TT runtime
    clkTt = Abc_Clock();
    {
        #define CLU_VAR_MAX  16

        // decomposition
        typedef struct If_Grp_t_ If_Grp_t;
        struct If_Grp_t_
        {
            char       nVars;
            char       nMyu;
            char       pVars[CLU_VAR_MAX];
        };


        int nLutLeaf  = 6;
        int nLutLeaf2 = 6;
        int nLutRoot  = 6;

        If_Grp_t G;
        If_Grp_t G2, R;
        word Func0, Func1, Func2;

        {
            extern If_Grp_t If_CluCheck3( void * p, word * pTruth0, int nVars, int nLutLeaf, int nLutLeaf2, int nLutRoot, 
                          If_Grp_t * pR, If_Grp_t * pG2, word * pFunc0, word * pFunc1, word * pFunc2 );
            G = If_CluCheck3( NULL, pTruth, nLeaves, nLutLeaf, nLutLeaf2, nLutRoot, &R, &G2, &Func0, &Func1, &Func2 );
        }

    }
    timeTt += Abc_Clock() - clkTt;
}

#endif

 
/**Function*************************************************************

  Synopsis    [Elementary truth tables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word ** Dss_ManTtElems()
{
    static word TtElems[DAU_MAX_VAR+1][DAU_MAX_WORD], * pTtElems[DAU_MAX_VAR+1] = {NULL};
    if ( pTtElems[0] == NULL )
    {
        int v;
        for ( v = 0; v <= DAU_MAX_VAR; v++ )
            pTtElems[v] = TtElems[v];
        Abc_TtElemInit( pTtElems, DAU_MAX_VAR );
    }
    return pTtElems;
}

/**Function*************************************************************

  Synopsis    [Creating DSD network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dss_Obj_t * Dss_ObjAllocNtk( Dss_Ntk_t * p, int Type, int nFans, int nTruthVars )
{
    Dss_Obj_t * pObj;
    pObj = (Dss_Obj_t *)(p->pMem + p->nMem);
    Dss_ObjClean( pObj );
    pObj->nFans  = nFans;
    pObj->nWords = Dss_ObjWordNum( nFans );
    pObj->Type   = Type;
    pObj->Id     = Vec_PtrSize( p->vObjs );
    pObj->iVar   = 31;
    Vec_PtrPush( p->vObjs, pObj );
    p->nMem += pObj->nWords + (nTruthVars ? Abc_TtWordNum(nTruthVars) : 0);
    assert( p->nMem < p->nMemAlloc );
    return pObj;
}
Dss_Obj_t * Dss_ObjCreateNtk( Dss_Ntk_t * p, int Type, Vec_Int_t * vFaninLits )
{
    Dss_Obj_t * pObj;
    int i, Entry;
    pObj = Dss_ObjAllocNtk( p, Type, Vec_IntSize(vFaninLits), Type == DAU_DSD_PRIME ? Vec_IntSize(vFaninLits) : 0 );
    Vec_IntForEachEntry( vFaninLits, Entry, i )
    {
        pObj->pFans[i] = Entry;
        pObj->nSupp += Dss_VecLitSuppSize(p->vObjs, Entry);
    }
    assert( i == (int)pObj->nFans );
    return pObj;
}
Dss_Ntk_t * Dss_NtkAlloc( int nVars )
{
    Dss_Ntk_t * p;
    Dss_Obj_t * pObj;
    int i;
    p = ABC_CALLOC( Dss_Ntk_t, 1 );
    p->nVars     = nVars;
    p->nMemAlloc = DAU_MAX_STR;
    p->pMem      = ABC_ALLOC( word, p->nMemAlloc );
    p->vObjs     = Vec_PtrAlloc( 100 );
    Dss_ObjAllocNtk( p, DAU_DSD_CONST0, 0, 0 );
    for ( i = 0; i < nVars; i++ )
    {
        pObj = Dss_ObjAllocNtk( p, DAU_DSD_VAR, 0, 0 );
        pObj->iVar = i;
        pObj->nSupp = 1;
    }
    return p;
}
void Dss_NtkFree( Dss_Ntk_t * p )
{
    Vec_PtrFree( p->vObjs );
    ABC_FREE( p->pMem );
    ABC_FREE( p );
}
void Dss_NtkPrint_rec( Dss_Ntk_t * p, Dss_Obj_t * pObj )
{
    char OpenType[7]  = {0, 0, 0, '(', '[', '<', '{'};
    char CloseType[7] = {0, 0, 0, ')', ']', '>', '}'};
    Dss_Obj_t * pFanin;
    int i;
    assert( !Dss_IsComplement(pObj) );
    if ( pObj->Type == DAU_DSD_VAR )
        { printf( "%c", 'a' + pObj->iVar ); return; }
    if ( pObj->Type == DAU_DSD_PRIME )
        Abc_TtPrintHexRev( stdout, Dss_ObjTruth(pObj), pObj->nFans );
    printf( "%c", OpenType[pObj->Type] );
    Dss_ObjForEachFanin( p->vObjs, pObj, pFanin, i )
    {
        printf( "%s", Dss_ObjFaninC(pObj, i) ? "!":"" );
        Dss_NtkPrint_rec( p, pFanin );
    }
    printf( "%c", CloseType[pObj->Type] );
}
void Dss_NtkPrint( Dss_Ntk_t * p )
{
    if ( Dss_Regular(p->pRoot)->Type == DAU_DSD_CONST0 )
        printf( "%d", Dss_IsComplement(p->pRoot) );
    else
    {
        printf( "%s", Dss_IsComplement(p->pRoot) ? "!":"" );        
        if ( Dss_Regular(p->pRoot)->Type == DAU_DSD_VAR )
            printf( "%c", 'a' + Dss_Regular(p->pRoot)->iVar );
        else
            Dss_NtkPrint_rec( p, Dss_Regular(p->pRoot) );
    }
    printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Creating DSD network from SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Dau_DsdMergeMatches( char * pDsd, int * pMatches )
{
    int pNested[DAU_MAX_VAR];
    int i, nNested = 0;
    for ( i = 0; pDsd[i]; i++ )
    {
        pMatches[i] = 0;
        if ( pDsd[i] == '(' || pDsd[i] == '[' || pDsd[i] == '<' || pDsd[i] == '{' )
            pNested[nNested++] = i;
        else if ( pDsd[i] == ')' || pDsd[i] == ']' || pDsd[i] == '>' || pDsd[i] == '}' )
            pMatches[pNested[--nNested]] = i;
        assert( nNested < DAU_MAX_VAR );
    }
    assert( nNested == 0 );
}
int Dss_NtkCreate_rec( char * pStr, char ** p, int * pMatches, Dss_Ntk_t * pNtk, word * pTruth )
{
    int fCompl = 0;
    if ( **p == '!' )
    {
        fCompl = 1;
        (*p)++;
    }
    while ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
        (*p)++;
/*
    if ( **p == '<' )
    {
        char * q = pStr + pMatches[ *p - pStr ];
        if ( *(q+1) == '{' )
            *p = q+1;
    }
*/
    if ( **p >= 'a' && **p <= 'z' ) // var
        return Abc_Var2Lit( Dss_ObjId(Dss_VecVar(pNtk->vObjs, **p - 'a')), fCompl );
    if ( **p == '(' || **p == '[' || **p == '<' || **p == '{' ) // and/or/xor
    {
        Dss_Obj_t * pObj;
        Vec_Int_t * vFaninLits = Vec_IntAlloc( 10 );
        char * q = pStr + pMatches[ *p - pStr ];
        int Type = 0;
        if ( **p == '(' )
            Type = DAU_DSD_AND;
        else if ( **p == '[' )
            Type = DAU_DSD_XOR;
        else if ( **p == '<' )
            Type = DAU_DSD_MUX;
        else if ( **p == '{' )
            Type = DAU_DSD_PRIME;
        else assert( 0 );
        assert( *q == **p + 1 + (**p != '(') );
        for ( (*p)++; *p < q; (*p)++ )
            Vec_IntPush( vFaninLits, Dss_NtkCreate_rec(pStr, p, pMatches, pNtk, pTruth) );
        assert( *p == q );
        if ( Type == DAU_DSD_PRIME )
        {
            Vec_Int_t * vFaninLitsNew;
            word pTemp[DAU_MAX_WORD];
            char pCanonPerm[DAU_MAX_VAR];
            int i, uCanonPhase, nFanins = Vec_IntSize(vFaninLits);
            Abc_TtCopy( pTemp, pTruth, Abc_TtWordNum(nFanins), 0 );
            uCanonPhase = Abc_TtCanonicize( pTemp, nFanins, pCanonPerm );
            fCompl = (uCanonPhase >> nFanins) & 1;
            vFaninLitsNew = Vec_IntAlloc( nFanins );
            for ( i = 0; i < nFanins; i++ )
                Vec_IntPush( vFaninLitsNew, Abc_LitNotCond(Vec_IntEntry(vFaninLits, pCanonPerm[i]), (uCanonPhase>>i)&1) );
            pObj = Dss_ObjCreateNtk( pNtk, DAU_DSD_PRIME, vFaninLitsNew );
            Abc_TtCopy( Dss_ObjTruth(pObj), pTemp, Abc_TtWordNum(nFanins), 0 );
            Vec_IntFree( vFaninLitsNew );
        }
        else
            pObj = Dss_ObjCreateNtk( pNtk, Type, vFaninLits );
        Vec_IntFree( vFaninLits );
        return Abc_LitNotCond( Dss_Obj2Lit(pObj), fCompl );
    }
    assert( 0 );
    return -1;
}
Dss_Ntk_t * Dss_NtkCreate( char * pDsd, int nVars, word * pTruth )
{
    int fCompl = 0;
    Dss_Ntk_t * pNtk = Dss_NtkAlloc( nVars );
    if ( *pDsd == '!' )
         pDsd++, fCompl = 1;
    if ( Dau_DsdIsConst(pDsd) )
        pNtk->pRoot = Dss_VecConst0(pNtk->vObjs);
    else if ( Dau_DsdIsVar(pDsd) )
        pNtk->pRoot = Dss_VecVar(pNtk->vObjs, Dau_DsdReadVar(pDsd));
    else
    {
        int iLit, pMatches[DAU_MAX_STR];
        Dau_DsdMergeMatches( pDsd, pMatches );
        iLit = Dss_NtkCreate_rec( pDsd, &pDsd, pMatches, pNtk, pTruth );
        pNtk->pRoot = Dss_Lit2Obj( pNtk->vObjs, iLit );
    }
    if ( fCompl )
        pNtk->pRoot = Dss_Not(pNtk->pRoot);
    return pNtk;
}
 
/**Function*************************************************************

  Synopsis    [Comparing two DSD nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dss_ObjCompare( Vec_Ptr_t * p, Dss_Obj_t * p0i, Dss_Obj_t * p1i )
{
    Dss_Obj_t * p0 = Dss_Regular(p0i);
    Dss_Obj_t * p1 = Dss_Regular(p1i);
    Dss_Obj_t * pChild0, * pChild1;
    int i, Res;
    if ( Dss_ObjType(p0) < Dss_ObjType(p1) )
        return -1;
    if ( Dss_ObjType(p0) > Dss_ObjType(p1) )
        return 1;
    if ( Dss_ObjType(p0) < DAU_DSD_AND )
        return 0;
    if ( Dss_ObjFaninNum(p0) < Dss_ObjFaninNum(p1) )
        return -1;
    if ( Dss_ObjFaninNum(p0) > Dss_ObjFaninNum(p1) )
        return 1;
    for ( i = 0; i < Dss_ObjFaninNum(p0); i++ )
    {
        pChild0 = Dss_ObjChild( p, p0, i );
        pChild1 = Dss_ObjChild( p, p1, i );
        Res = Dss_ObjCompare( p, pChild0, pChild1 );
        if ( Res != 0 )
            return Res;
    }
    if ( Dss_IsComplement(p0i) < Dss_IsComplement(p1i) )
        return -1;
    if ( Dss_IsComplement(p0i) > Dss_IsComplement(p1i) )
        return 1;
    return 0;
}
void Dss_ObjSort( Vec_Ptr_t * p, Dss_Obj_t ** pNodes, int nNodes, int * pPerm )
{
    int i, j, best_i;
    for ( i = 0; i < nNodes-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nNodes; j++ )
            if ( Dss_ObjCompare(p, pNodes[best_i], pNodes[j]) == 1 )
                best_i = j;
        if ( i == best_i )
            continue;
        ABC_SWAP( Dss_Obj_t *, pNodes[i], pNodes[best_i] );
        if ( pPerm )
            ABC_SWAP( int, pPerm[i], pPerm[best_i] );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dss_NtkCheck( Dss_Ntk_t * p )
{
    Dss_Obj_t * pObj, * pFanin;
    int i, k;
    Dss_VecForEachNode( p->vObjs, pObj, i )
    {
        Dss_ObjForEachFanin( p->vObjs, pObj, pFanin, k )
        {
            if ( pObj->Type == DAU_DSD_AND && pFanin->Type == DAU_DSD_AND )
                assert( Dss_ObjFaninC(pObj, k) );
            else if ( pObj->Type == DAU_DSD_XOR )
                assert( pFanin->Type != DAU_DSD_XOR );
            else if ( pObj->Type == DAU_DSD_MUX )
                assert( !Dss_ObjFaninC(pObj, 0) );
        }
    }
}
int Dss_NtkCollectPerm_rec( Dss_Ntk_t * p, Dss_Obj_t * pObj, int * pPermDsd, int * pnPerms )
{
    Dss_Obj_t * pChild;
    int k, fCompl = Dss_IsComplement(pObj);
    pObj = Dss_Regular( pObj );
    if ( pObj->Type == DAU_DSD_VAR )
    {
        pPermDsd[*pnPerms] = Abc_Var2Lit(pObj->iVar, fCompl);
        pObj->iVar = (*pnPerms)++;
        return fCompl;
    }
    Dss_ObjForEachChild( p->vObjs, pObj, pChild, k )
        if ( Dss_NtkCollectPerm_rec( p, pChild, pPermDsd, pnPerms ) )
            pObj->pFans[k] = (unsigned char)Abc_LitRegular((int)pObj->pFans[k]);
    return 0;
}
void Dss_NtkTransform( Dss_Ntk_t * p, int * pPermDsd )
{
    Dss_Obj_t * pChildren[DAU_MAX_VAR];
    Dss_Obj_t * pObj, * pChild;
    int i, k, nPerms;
    if ( Dss_Regular(p->pRoot)->Type == DAU_DSD_CONST0 )
        return;
    Dss_VecForEachNode( p->vObjs, pObj, i )
    {
        if ( pObj->Type == DAU_DSD_MUX || pObj->Type == DAU_DSD_PRIME )
            continue;
        Dss_ObjForEachChild( p->vObjs, pObj, pChild, k )
            pChildren[k] = pChild;
        Dss_ObjSort( p->vObjs, pChildren, Dss_ObjFaninNum(pObj), NULL );
        for ( k = 0; k < Dss_ObjFaninNum(pObj); k++ )
            pObj->pFans[k] = Dss_Obj2Lit( pChildren[k] );
    }
    nPerms = 0;
    if ( Dss_NtkCollectPerm_rec( p, p->pRoot, pPermDsd, &nPerms ) )
        p->pRoot = Dss_Regular(p->pRoot);
    assert( nPerms == (int)Dss_Regular(p->pRoot)->nSupp );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dss_Obj_t * Dss_ObjAlloc( Dss_Man_t * p, int Type, int nFans, int nTruthVars )
{
    int nWords = Dss_ObjWordNum(nFans) + (nTruthVars ? Abc_TtWordNum(nTruthVars) : 0);
    Dss_Obj_t * pObj = (Dss_Obj_t *)Mem_FlexEntryFetch( p->pMem, sizeof(word) * nWords );
    Dss_ObjClean( pObj );
    pObj->Type   = Type;
    pObj->nFans  = nFans;
    pObj->nWords = Dss_ObjWordNum(nFans);
    pObj->Id     = Vec_PtrSize( p->vObjs );
    pObj->iVar   = 31;
    Vec_PtrPush( p->vObjs, pObj );
    Vec_IntPush( p->vNexts, 0 );
    return pObj;
}
Dss_Obj_t * Dss_ObjCreate( Dss_Man_t * p, int Type, Vec_Int_t * vFaninLits, word * pTruth )
{
    Dss_Obj_t * pObj, * pFanin, * pPrev = NULL;
    int i, Entry;
    // check structural canonicity
    assert( Type != DAU_DSD_MUX || Vec_IntSize(vFaninLits) == 3 );
    assert( Type != DAU_DSD_MUX || !Abc_LitIsCompl(Vec_IntEntry(vFaninLits, 0)) );
    assert( Type != DAU_DSD_MUX || !Abc_LitIsCompl(Vec_IntEntry(vFaninLits, 1)) || !Abc_LitIsCompl(Vec_IntEntry(vFaninLits, 2)) );
    // check that leaves are in good order
    if ( Type == DAU_DSD_AND || Type == DAU_DSD_XOR )
    Dss_VecForEachObjVec( vFaninLits, p->vObjs, pFanin, i )
    {
        assert( Type != DAU_DSD_AND || Abc_LitIsCompl(Vec_IntEntry(vFaninLits, i)) || Dss_ObjType(pFanin) != DAU_DSD_AND );
        assert( Type != DAU_DSD_XOR || Dss_ObjType(pFanin) != DAU_DSD_XOR );
        assert( pPrev == NULL || Dss_ObjCompare(p->vObjs, pPrev, pFanin) <= 0 );
        pPrev = pFanin;
    }
    // create new node
    pObj = Dss_ObjAlloc( p, Type, Vec_IntSize(vFaninLits), Type == DAU_DSD_PRIME ? Vec_IntSize(vFaninLits) : 0 );
    if ( Type == DAU_DSD_PRIME )
        Abc_TtCopy( Dss_ObjTruth(pObj), pTruth, Abc_TtWordNum(Vec_IntSize(vFaninLits)), 0 );
    assert( pObj->nSupp == 0 );
    Vec_IntForEachEntry( vFaninLits, Entry, i )
    {
        pObj->pFans[i] = Entry;
        pObj->nSupp += Dss_VecLitSuppSize(p->vObjs, Entry);
    }
/*
    {
        extern void Dss_ManPrintOne( Dss_Man_t * p, int iDsdLit, int * pPermLits );
        Dss_ManPrintOne( p, Dss_Obj2Lit(pObj), NULL );
    }
*/
    return pObj;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dss_ManHashProfile( Dss_Man_t * p )
{
    Dss_Obj_t * pObj;
    unsigned * pSpot;
    int i, Counter;
    for ( i = 0; i < p->nBins; i++ )
    {
        Counter = 0;
        for ( pSpot = p->pBins + i; *pSpot; pSpot = (unsigned *)Vec_IntEntryP(p->vNexts, pObj->Id), Counter++ )
             pObj = Dss_VecObj( p->vObjs, *pSpot );
        if ( Counter )
            printf( "%d ", Counter );
    }
    printf( "\n" );
}
static inline unsigned Dss_ObjHashKey( Dss_Man_t * p, int Type, Vec_Int_t * vFaninLits, word * pTruth )
{
    static int s_Primes[8] = { 1699, 4177, 5147, 5647, 6343, 7103, 7873, 8147 };
    int i, Entry;
    unsigned uHash = Type * 7873 + Vec_IntSize(vFaninLits) * 8147;
    Vec_IntForEachEntry( vFaninLits, Entry, i )
        uHash += Entry * s_Primes[i & 0x7];
    assert( (Type == DAU_DSD_PRIME) == (pTruth != NULL) );
    if ( pTruth )
    {
        unsigned char * pTruthC = (unsigned char *)pTruth;
        int nBytes = Abc_TtByteNum(Vec_IntSize(vFaninLits));
        for ( i = 0; i < nBytes; i++ )
            uHash += pTruthC[i] * s_Primes[i & 0x7];
    }
    return uHash % p->nBins;
}
unsigned * Dss_ObjHashLookup( Dss_Man_t * p, int Type, Vec_Int_t * vFaninLits, word * pTruth )
{
    Dss_Obj_t * pObj;
    unsigned * pSpot = p->pBins + Dss_ObjHashKey(p, Type, vFaninLits, pTruth);
    for ( ; *pSpot; pSpot = (unsigned *)Vec_IntEntryP(p->vNexts, pObj->Id) )
    {
        pObj = Dss_VecObj( p->vObjs, *pSpot );
        if ( (int)pObj->Type == Type && 
             (int)pObj->nFans == Vec_IntSize(vFaninLits) && 
             !memcmp(pObj->pFans, Vec_IntArray(vFaninLits), sizeof(int)*pObj->nFans) &&
             (pTruth == NULL || !memcmp(Dss_ObjTruth(pObj), pTruth, (size_t)Abc_TtByteNum(pObj->nFans))) ) // equal
            return pSpot;
    }
    return pSpot;
}
Dss_Obj_t * Dss_ObjFindOrAdd( Dss_Man_t * p, int Type, Vec_Int_t * vFaninLits, word * pTruth )
{
    Dss_Obj_t * pObj;
    unsigned * pSpot = Dss_ObjHashLookup( p, Type, vFaninLits, pTruth );
    if ( *pSpot )
        return Dss_VecObj( p->vObjs, *pSpot );
    *pSpot = Vec_PtrSize( p->vObjs );
    pObj = Dss_ObjCreate( p, Type, vFaninLits, pTruth );
    return pObj;
}

/**Function*************************************************************

  Synopsis    [Cache for decomposition calls.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dss_ManCacheAlloc( Dss_Man_t * p )
{
    assert( p->nCache == 0 );
    p->nCache = Abc_PrimeCudd( 100000 );
    p->pCache = ABC_CALLOC( Dss_Ent_t *, p->nCache );
}
void Dss_ManCacheFree( Dss_Man_t * p )
{
    if ( p->pCache == NULL )
        return;
    assert( p->nCache != 0 );
    p->nCache = 0;
    ABC_FREE( p->pCache );
}
static inline unsigned Dss_ManCacheHashKey( Dss_Man_t * p, Dss_Ent_t * pEnt )
{ 
    static int s_Primes[8] = { 1699, 4177, 5147, 5647, 6343, 7103, 7873, 8147 };
    int i;
    unsigned uHash = pEnt->nShared * 7103 + pEnt->iDsd0 * 7873 + pEnt->iDsd1 * 8147;
    for ( i = 0; i < 2*(int)pEnt->nShared; i++ )
        uHash += pEnt->pShared[i] * s_Primes[i & 0x7];
    return uHash % p->nCache;
}
void Dss_ManCacheProfile( Dss_Man_t * p )
{
    Dss_Ent_t ** pSpot;
    int i, Counter;
    for ( i = 0; i < p->nCache; i++ )
    {
        Counter = 0;
        for ( pSpot = p->pCache + i; *pSpot; pSpot = &(*pSpot)->pNext, Counter++ )
            ;
        if ( Counter )
            printf( "%d ", Counter );
    }
    printf( "\n" );
}
Dss_Ent_t ** Dss_ManCacheLookup( Dss_Man_t * p, Dss_Ent_t * pEnt )
{
    Dss_Ent_t ** pSpot = p->pCache + Dss_ManCacheHashKey( p, pEnt );
    for ( ; *pSpot; pSpot = &(*pSpot)->pNext )
    {
        if ( (*pSpot)->iDsd0   == pEnt->iDsd0 && 
             (*pSpot)->iDsd1   == pEnt->iDsd1 && 
             (*pSpot)->nShared == pEnt->nShared && 
             !memcmp((*pSpot)->pShared, pEnt->pShared, sizeof(char)*2*pEnt->nShared)  ) // equal
        {
            p->nCacheHits[pEnt->nShared!=0]++;
            return pSpot;
        }
    }
    p->nCacheMisses[pEnt->nShared!=0]++;
    return pSpot;
}
Dss_Ent_t * Dss_ManCacheCreate( Dss_Man_t * p, Dss_Ent_t * pEnt0, Dss_Fun_t * pFun0 )
{
    Dss_Ent_t * pEnt = (Dss_Ent_t *)Mem_FlexEntryFetch( p->pMemEnts, sizeof(word) * pEnt0->nWords );
    Dss_Fun_t * pFun = (Dss_Fun_t *)Mem_FlexEntryFetch( p->pMemEnts, sizeof(word) * Dss_FunWordNum(pFun0) );
    memcpy( pEnt, pEnt0, sizeof(word) * pEnt0->nWords );
    memcpy( pFun, pFun0, sizeof(word) * Dss_FunWordNum(pFun0) );
    pEnt->pFunc = pFun;
    pEnt->pNext = NULL;
    p->nCacheEntries[pEnt->nShared!=0]++;
    return pEnt;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dss_Man_t * Dss_ManAlloc( int nVars, int nNonDecLimit )
{
    Dss_Man_t * p;
    p = ABC_CALLOC( Dss_Man_t, 1 );
    p->nVars  = nVars;
    p->nNonDecLimit = nNonDecLimit;
    p->nBins  = Abc_PrimeCudd( 1000000 );
    p->pBins  = ABC_CALLOC( unsigned, p->nBins );
    p->pMem   = Mem_FlexStart();
    p->vObjs  = Vec_PtrAlloc( 10000 );
    p->vNexts = Vec_IntAlloc( 10000 );
    Dss_ObjAlloc( p, DAU_DSD_CONST0, 0, 0 );
    Dss_ObjAlloc( p, DAU_DSD_VAR, 0, 0 )->nSupp = 1;
    p->vLeaves = Vec_IntAlloc( 32 );
    p->vCopies = Vec_IntAlloc( 32 );
    p->pTtElems = Dss_ManTtElems();
    p->pMemEnts = Mem_FlexStart();
//    Dss_ManCacheAlloc( p );
    return p;
}
void Dss_ManFree( Dss_Man_t * p )
{
    Abc_PrintTime( 1, "Time begin ", p->timeBeg );
    Abc_PrintTime( 1, "Time decomp", p->timeDec );
    Abc_PrintTime( 1, "Time lookup", p->timeLook );
    Abc_PrintTime( 1, "Time end   ", p->timeEnd );

//    Dss_ManCacheProfile( p );
    Dss_ManCacheFree( p );
    Mem_FlexStop( p->pMemEnts, 0 );
    Vec_IntFreeP( &p->vCopies );
    Vec_IntFreeP( &p->vLeaves );
    Vec_IntFreeP( &p->vNexts );
    Vec_PtrFreeP( &p->vObjs );
    Mem_FlexStop( p->pMem, 0 );
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}
void Dss_ManPrint_rec( FILE * pFile, Dss_Man_t * p, Dss_Obj_t * pObj, int * pPermLits, int * pnSupp )
{
    char OpenType[7]  = {0, 0, 0, '(', '[', '<', '{'};
    char CloseType[7] = {0, 0, 0, ')', ']', '>', '}'};
    Dss_Obj_t * pFanin;
    int i;
    assert( !Dss_IsComplement(pObj) );
    if ( pObj->Type == DAU_DSD_CONST0 )
        { fprintf( pFile, "0" ); return; }
    if ( pObj->Type == DAU_DSD_VAR )
    {
        int iPermLit = pPermLits ? pPermLits[(*pnSupp)++] : Abc_Var2Lit((*pnSupp)++, 0);
        fprintf( pFile, "%s%c", Abc_LitIsCompl(iPermLit)? "!":"", 'a' + Abc_Lit2Var(iPermLit) );
        return;
    }
    if ( pObj->Type == DAU_DSD_PRIME )
        Abc_TtPrintHexRev( pFile, Dss_ObjTruth(pObj), pObj->nFans );
    fprintf( pFile, "%c", OpenType[pObj->Type] );
    Dss_ObjForEachFanin( p->vObjs, pObj, pFanin, i )
    {
        fprintf( pFile, "%s", Dss_ObjFaninC(pObj, i) ? "!":"" );
        Dss_ManPrint_rec( pFile, p, pFanin, pPermLits, pnSupp );
    }
    fprintf( pFile, "%c", CloseType[pObj->Type] );
}
void Dss_ManPrintOne( FILE * pFile, Dss_Man_t * p, int iDsdLit, int * pPermLits )
{
    int nSupp = 0;
    fprintf( pFile, "%6d : ", Abc_Lit2Var(iDsdLit) );
    fprintf( pFile, "%2d ",   Dss_VecLitSuppSize(p->vObjs, iDsdLit) );
    fprintf( pFile, "%s",     Abc_LitIsCompl(iDsdLit) ? "!" : ""  );
    Dss_ManPrint_rec( pFile, p, Dss_VecObj(p->vObjs, Abc_Lit2Var(iDsdLit)), pPermLits, &nSupp );
    fprintf( pFile, "\n" );
    assert( nSupp == (int)Dss_VecObj(p->vObjs, Abc_Lit2Var(iDsdLit))->nSupp );
}
int Dss_ManCheckNonDec_rec( Dss_Man_t * p, Dss_Obj_t * pObj )
{
    Dss_Obj_t * pFanin;
    int i;
    assert( !Dss_IsComplement(pObj) );
    if ( pObj->Type == DAU_DSD_CONST0 )
        return 0;
    if ( pObj->Type == DAU_DSD_VAR )
        return 0;
    if ( pObj->Type == DAU_DSD_PRIME )
        return 1;
    Dss_ObjForEachFanin( p->vObjs, pObj, pFanin, i )
        if ( Dss_ManCheckNonDec_rec( p, pFanin ) )
            return 1;
    return 0;
}
void Dss_ManDump( Dss_Man_t * p )
{
    char * pFileName = "dss_tts.txt";
    FILE * pFile;
    word Temp[DAU_MAX_WORD];
    Dss_Obj_t * pObj;
    int i;
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\".\n", pFileName );
        return;
    }
    Dss_VecForEachObj( p->vObjs, pObj, i )
    {
        if ( pObj->Type != DAU_DSD_PRIME )
            continue;
        Abc_TtCopy( Temp, Dss_ObjTruth(pObj), Abc_TtWordNum(pObj->nFans), 0 );
        Abc_TtStretch6( Temp, pObj->nFans, p->nVars );
        fprintf( pFile, "0x" );
        Abc_TtPrintHexRev( pFile, Temp, p->nVars );
        fprintf( pFile, "\n" );

//        printf( "%6d : ", i );
//        Abc_TtPrintHexRev( stdout, Temp, p->nVars );
//        printf( "    " );
//        Dau_DsdPrintFromTruth( stdout, Temp, p->nVars );
    }
    fclose( pFile );
}
void Dss_ManPrint( char * pFileName, Dss_Man_t * p )
{
    Dss_Obj_t * pObj;
    int CountNonDsd = 0, CountNonDsdStr = 0;
    int i, clk = Abc_Clock();
    FILE * pFile;
    pFile = pFileName ? fopen( pFileName, "wb" ) : stdout;
    if ( pFileName && pFile == NULL )
    {
        printf( "cannot open output file\n" );
        return;
    }
    Dss_VecForEachObj( p->vObjs, pObj, i )
    {
        CountNonDsd += (pObj->Type == DAU_DSD_PRIME);
        CountNonDsdStr += Dss_ManCheckNonDec_rec( p, pObj );
    }
    fprintf( pFile, "Total number of objects    = %8d\n", Vec_PtrSize(p->vObjs) );
    fprintf( pFile, "Non-DSD objects (max =%2d)  = %8d\n", p->nNonDecLimit, CountNonDsd );
    fprintf( pFile, "Non-DSD structures         = %8d\n", CountNonDsdStr );
    fprintf( pFile, "Memory used for objects    = %6.2f MB.\n", 1.0*Mem_FlexReadMemUsage(p->pMem)/(1<<20) );
    fprintf( pFile, "Memory used for array      = %6.2f MB.\n", 1.0*sizeof(void *)*Vec_PtrCap(p->vObjs)/(1<<20) );
    fprintf( pFile, "Memory used for hash table = %6.2f MB.\n", 1.0*sizeof(int)*p->nBins/(1<<20) );
    fprintf( pFile, "Memory used for cache      = %6.2f MB.\n", 1.0*Mem_FlexReadMemUsage(p->pMemEnts)/(1<<20) );
    fprintf( pFile, "Cache hits    = %8d %8d\n", p->nCacheHits[0],    p->nCacheHits[1] );
    fprintf( pFile, "Cache misses  = %8d %8d\n", p->nCacheMisses[0],  p->nCacheMisses[1] );
    fprintf( pFile, "Cache entries = %8d %8d\n", p->nCacheEntries[0], p->nCacheEntries[1] );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
//    Dss_ManHashProfile( p );
//    Dss_ManDump( p );
//    return;
    Dss_VecForEachObj( p->vObjs, pObj, i )
    {
        if ( i == 50 )
            break;
        Dss_ManPrintOne( pFile, p, Dss_Obj2Lit(pObj), NULL );
    }
    fprintf( pFile, "\n" );
    if ( pFileName )
        fclose( pFile );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dss_ManComputeTruth_rec( Dss_Man_t * p, Dss_Obj_t * pObj, int nVars, word * pRes, int * pPermLits, int * pnSupp )
{
    Dss_Obj_t * pChild;
    int nWords = Abc_TtWordNum(nVars);
    int i, fCompl = Dss_IsComplement(pObj);
    pObj = Dss_Regular(pObj);
    if ( pObj->Type == DAU_DSD_VAR )
    {
        int iPermLit = pPermLits[(*pnSupp)++];
        assert( (*pnSupp) <= nVars );
        Abc_TtCopy( pRes, p->pTtElems[Abc_Lit2Var(iPermLit)], nWords, fCompl ^ Abc_LitIsCompl(iPermLit) );
        return;
    }
    if ( pObj->Type == DAU_DSD_AND || pObj->Type == DAU_DSD_XOR )
    {
        word pTtTemp[DAU_MAX_WORD];
        if ( pObj->Type == DAU_DSD_AND )
            Abc_TtConst1( pRes, nWords );
        else
            Abc_TtConst0( pRes, nWords );
        Dss_ObjForEachChild( p->vObjs, pObj, pChild, i )
        {
            Dss_ManComputeTruth_rec( p, pChild, nVars, pTtTemp, pPermLits, pnSupp );
            if ( pObj->Type == DAU_DSD_AND )
                Abc_TtAnd( pRes, pRes, pTtTemp, nWords, 0 );
            else
                Abc_TtXor( pRes, pRes, pTtTemp, nWords, 0 );
        }
        if ( fCompl ) Abc_TtNot( pRes, nWords );
        return;
    }
    if ( pObj->Type == DAU_DSD_MUX ) // mux
    {
        word pTtTemp[3][DAU_MAX_WORD];
        Dss_ObjForEachChild( p->vObjs, pObj, pChild, i )
            Dss_ManComputeTruth_rec( p, pChild, nVars, pTtTemp[i], pPermLits, pnSupp );
        assert( i == 3 );
        Abc_TtMux( pRes, pTtTemp[0], pTtTemp[1], pTtTemp[2], nWords );
        if ( fCompl ) Abc_TtNot( pRes, nWords );
        return;
    }
    if ( pObj->Type == DAU_DSD_PRIME ) // function
    {
        word pFanins[DAU_MAX_VAR][DAU_MAX_WORD];
        Dss_ObjForEachChild( p->vObjs, pObj, pChild, i )
            Dss_ManComputeTruth_rec( p, pChild, nVars, pFanins[i], pPermLits, pnSupp );
        Dau_DsdTruthCompose_rec( Dss_ObjTruth(pObj), pFanins, pRes, pObj->nFans, nWords );
        if ( fCompl ) Abc_TtNot( pRes, nWords );
        return;
    }
    assert( 0 );

}
word * Dss_ManComputeTruth( Dss_Man_t * p, int iDsd, int nVars, int * pPermLits )
{
    Dss_Obj_t * pObj = Dss_Lit2Obj(p->vObjs, iDsd);
    word * pRes = p->pTtElems[DAU_MAX_VAR];
    int nWords = Abc_TtWordNum( nVars );
    int nSupp = 0;
    assert( nVars <= DAU_MAX_VAR );
    if ( iDsd == 0 )
        Abc_TtConst0( pRes, nWords );
    else if ( iDsd == 1 )
        Abc_TtConst1( pRes, nWords );
    else if ( Dss_Regular(pObj)->Type == DAU_DSD_VAR )
    {
        int iPermLit = pPermLits[nSupp++];
        Abc_TtCopy( pRes, p->pTtElems[Abc_Lit2Var(iPermLit)], nWords, Abc_LitIsCompl(iDsd) ^ Abc_LitIsCompl(iPermLit) );
    }
    else
        Dss_ManComputeTruth_rec( p, pObj, nVars, pRes, pPermLits, &nSupp );
    assert( nSupp == (int)Dss_Regular(pObj)->nSupp );
    return pRes;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// returns literal of non-shifted tree in p, corresponding to pObj in pNtk, which may be compl
int Dss_NtkRebuild_rec( Dss_Man_t * p, Dss_Ntk_t * pNtk, Dss_Obj_t * pObj )
{
    Dss_Obj_t * pChildren[DAU_MAX_VAR];
    Dss_Obj_t * pChild, * pObjNew;
    int i, k, fCompl = Dss_IsComplement(pObj);
    pObj = Dss_Regular(pObj);
    if ( pObj->Type == DAU_DSD_VAR )
        return Abc_Var2Lit( 1, fCompl );
    Dss_ObjForEachChild( pNtk->vObjs, pObj, pChild, k )
    {
        pChildren[k] = Dss_Lit2Obj( p->vObjs, Dss_NtkRebuild_rec( p, pNtk, pChild ) );
        if ( pObj->Type == DAU_DSD_XOR && Dss_IsComplement(pChildren[k]) )
            pChildren[k] = Dss_Not(pChildren[k]), fCompl ^= 1;
    }
    // normalize MUX
    if ( pObj->Type == DAU_DSD_MUX )
    {
        if ( Dss_IsComplement(pChildren[0]) )
        {
            pChildren[0] = Dss_Not(pChildren[0]);
            ABC_SWAP( Dss_Obj_t *, pChildren[1], pChildren[2] );
        }
        if ( Dss_IsComplement(pChildren[1]) )
        {
            pChildren[1] = Dss_Not(pChildren[1]);
            pChildren[2] = Dss_Not(pChildren[2]);
            fCompl ^= 1;
        }
    }
    // shift subgraphs
    Vec_IntClear( p->vLeaves );
    for ( i = 0; i < k; i++ )
        Vec_IntPush( p->vLeaves, Dss_Obj2Lit(pChildren[i]) );
    // create new graph
    pObjNew = Dss_ObjFindOrAdd( p, pObj->Type, p->vLeaves, pObj->Type == DAU_DSD_PRIME ? Dss_ObjTruth(pObj) : NULL );
    return Abc_Var2Lit( pObjNew->Id, fCompl );
}
int Dss_NtkRebuild( Dss_Man_t * p, Dss_Ntk_t * pNtk )
{
    assert( p->nVars == pNtk->nVars );
    if ( Dss_Regular(pNtk->pRoot)->Type == DAU_DSD_CONST0 )
        return Dss_IsComplement(pNtk->pRoot);
    if ( Dss_Regular(pNtk->pRoot)->Type == DAU_DSD_VAR )
        return Abc_Var2Lit( Dss_Regular(pNtk->pRoot)->iVar + 1, Dss_IsComplement(pNtk->pRoot) );
    return Dss_NtkRebuild_rec( p, pNtk, pNtk->pRoot );
}

/**Function*************************************************************

  Synopsis    [Performs DSD operation on the two literals.]

  Description [Returns the perm of the resulting literals. The perm size 
  is equal to the number of support variables. The perm variables are 0-based
  numbers of pLits[0] followed by nLits[0]-based numbers of pLits[1].]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dss_ManOperation( Dss_Man_t * p, int Type, int * pLits, int nLits, unsigned char * pPerm, word * pTruth )
{
    Dss_Obj_t * pChildren[DAU_MAX_VAR];
    Dss_Obj_t * pObj, * pChild;
    int i, k, nChildren = 0, fCompl = 0, fComplFan;

    assert( Type == DAU_DSD_AND || pPerm == NULL );
    if ( Type == DAU_DSD_AND && pPerm != NULL )
    {
        int pBegEnd[DAU_MAX_VAR];
        int j, nSSize = 0;
        for ( k = 0; k < nLits; k++ )
        {
            pObj = Dss_Lit2Obj(p->vObjs, pLits[k]);
            if ( Dss_IsComplement(pObj) || pObj->Type != DAU_DSD_AND )
            {
                fComplFan = (Dss_Regular(pObj)->Type == DAU_DSD_VAR && Dss_IsComplement(pObj));
                if ( fComplFan )
                    pObj = Dss_Regular(pObj);
                pBegEnd[nChildren] = (nSSize << 16) | (fComplFan << 8) | (nSSize + Dss_Regular(pObj)->nSupp);
                nSSize += Dss_Regular(pObj)->nSupp;
                pChildren[nChildren++] = pObj;
            }
            else
                Dss_ObjForEachChild( p->vObjs, pObj, pChild, i )
                {
                    fComplFan = (Dss_Regular(pChild)->Type == DAU_DSD_VAR && Dss_IsComplement(pChild));
                    if ( fComplFan )
                        pChild = Dss_Regular(pChild);
                    pBegEnd[nChildren] = (nSSize << 16) | (fComplFan << 8) | (nSSize + Dss_Regular(pChild)->nSupp);
                    nSSize += Dss_Regular(pChild)->nSupp;
                    pChildren[nChildren++] = pChild;
                }
        }
        Dss_ObjSort( p->vObjs, pChildren, nChildren, pBegEnd );
        // create permutation
        for ( j = i = 0; i < nChildren; i++ )
            for ( k = (pBegEnd[i] >> 16); k < (pBegEnd[i] & 0xFF); k++ )
                pPerm[j++] = (unsigned char)Abc_Var2Lit( k, (pBegEnd[i] >> 8) & 1 );
        assert( j == nSSize );
    }
    else if ( Type == DAU_DSD_AND )
    {
        for ( k = 0; k < nLits; k++ )
        {
            pObj = Dss_Lit2Obj(p->vObjs, pLits[k]);
            if ( Dss_IsComplement(pObj) || pObj->Type != DAU_DSD_AND )
                pChildren[nChildren++] = pObj;
            else
                Dss_ObjForEachChild( p->vObjs, pObj, pChild, i )
                    pChildren[nChildren++] = pChild;
        }
        Dss_ObjSort( p->vObjs, pChildren, nChildren, NULL );
    }
    else if ( Type == DAU_DSD_XOR )
    {
        for ( k = 0; k < nLits; k++ )
        {
            fCompl ^= Abc_LitIsCompl(pLits[k]);
            pObj = Dss_Lit2Obj(p->vObjs, Abc_LitRegular(pLits[k]));
            if ( pObj->Type != DAU_DSD_XOR )
                pChildren[nChildren++] = pObj;
            else
                Dss_ObjForEachChild( p->vObjs, pObj, pChild, i )
                {
                    assert( !Dss_IsComplement(pChild) );
                    pChildren[nChildren++] = pChild;
                }
        }
        Dss_ObjSort( p->vObjs, pChildren, nChildren, NULL );
    }
    else if ( Type == DAU_DSD_MUX )
    {
        if ( Abc_LitIsCompl(pLits[0]) )
        {
            pLits[0] = Abc_LitNot(pLits[0]);
            ABC_SWAP( int, pLits[1], pLits[2] );
        }
        if ( Abc_LitIsCompl(pLits[1]) )
        {
            pLits[1] = Abc_LitNot(pLits[1]);
            pLits[2] = Abc_LitNot(pLits[2]);
            fCompl ^= 1;
        }
        for ( k = 0; k < nLits; k++ )
            pChildren[nChildren++] = Dss_Lit2Obj(p->vObjs, pLits[k]);
    }
    else if ( Type == DAU_DSD_PRIME )
    {
        for ( k = 0; k < nLits; k++ )
            pChildren[nChildren++] = Dss_Lit2Obj(p->vObjs, pLits[k]);
    }
    else assert( 0 );

    // shift subgraphs
    Vec_IntClear( p->vLeaves );
    for ( i = 0; i < nChildren; i++ )
        Vec_IntPush( p->vLeaves, Dss_Obj2Lit(pChildren[i]) );
    // create new graph
    pObj = Dss_ObjFindOrAdd( p, Type, p->vLeaves, pTruth );
    return Abc_Var2Lit( pObj->Id, fCompl );
}
Dss_Fun_t * Dss_ManOperationFun( Dss_Man_t * p, int * iDsd, int nFansTot )
{
    static char Buffer[100];
    Dss_Fun_t * pFun = (Dss_Fun_t *)Buffer;
    pFun->iDsd = Dss_ManOperation( p, DAU_DSD_AND, iDsd, 2, pFun->pFans, NULL );
//printf( "%d %d -> %d  ", iDsd[0], iDsd[1], pFun->iDsd );
    pFun->nFans = nFansTot;
    assert( (int)pFun->nFans == Dss_VecLitSuppSize(p->vObjs, pFun->iDsd) );
    return pFun;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dss_EntPrint( Dss_Ent_t * p, Dss_Fun_t * pFun )
{
    int i;
    printf( "%d %d ", p->iDsd0, p->iDsd1 );
    for ( i = 0; i < (int)p->nShared; i++ )
        printf( "%d=%d ", p->pShared[2*i], p->pShared[2*i+1] );
    printf( "-> %d   ", pFun->iDsd );
}

/**Function*************************************************************

  Synopsis    [Performs AND on two DSD functions with support overlap.]

  Description [Returns the perm of the resulting literals. The perm size 
  is equal to the number of support variables. The perm variables are 0-based
  numbers of pLits[0] followed by nLits[0]-based numbers of pLits[1].]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dss_Fun_t * Dss_ManBooleanAnd( Dss_Man_t * p, Dss_Ent_t * pEnt, int Counter )
{
    static char Buffer[100];
    Dss_Fun_t * pFun = (Dss_Fun_t *)Buffer;
    Dss_Ntk_t * pNtk;
    word * pTruthOne, pTruth[DAU_MAX_WORD];
    char pDsd[DAU_MAX_STR];
    int pMapDsd2Truth[DAU_MAX_VAR];
    int pPermLits[DAU_MAX_VAR];
    int pPermDsd[DAU_MAX_VAR];
    int i, nNonDec, nSuppSize = 0;
    int nFans[2];
    nFans[0] = Dss_VecLitSuppSize( p->vObjs, pEnt->iDsd0 );
    nFans[1] = Dss_VecLitSuppSize( p->vObjs, pEnt->iDsd1 );
    // create first truth table
    for ( i = 0; i < nFans[0]; i++ )
    {
        pMapDsd2Truth[nSuppSize] = i;
        pPermLits[i] = Abc_Var2Lit( nSuppSize++, 0 );
    }
    pTruthOne = Dss_ManComputeTruth( p, pEnt->iDsd0, p->nVars, pPermLits );
    Abc_TtCopy( pTruth, pTruthOne, Abc_TtWordNum(p->nVars), 0 );
if ( Counter )
{
//Kit_DsdPrintFromTruth( pTruthOne, p->nVars );  printf( "\n" );
}
    // create second truth table
    for ( i = 0; i < nFans[1]; i++ )
        pPermLits[i] = -1;
    for ( i = 0; i < (int)pEnt->nShared; i++ )
        pPermLits[pEnt->pShared[2*i+0]] = pEnt->pShared[2*i+1];
    for ( i = 0; i < nFans[1]; i++ )
        if ( pPermLits[i] == -1 )
        {
            pMapDsd2Truth[nSuppSize] = nFans[0] + i;
            pPermLits[i] = Abc_Var2Lit( nSuppSize++, 0 );
        }
    pTruthOne = Dss_ManComputeTruth( p, pEnt->iDsd1, p->nVars, pPermLits );
if ( Counter )
{
//Kit_DsdPrintFromTruth( pTruthOne, p->nVars );  printf( "\n" );
}
    Abc_TtAnd( pTruth, pTruth, pTruthOne, Abc_TtWordNum(p->nVars), 0 );
    // perform decomposition
    nNonDec = Dau_DsdDecompose( pTruth, nSuppSize, 0, 0, pDsd );
    if ( p->nNonDecLimit && nNonDec > p->nNonDecLimit )
        return NULL;
    // derive network and convert it into the manager
    pNtk = Dss_NtkCreate( pDsd, p->nVars, nNonDec ? pTruth : NULL );
//Dss_NtkPrint( pNtk );    
    Dss_NtkCheck( pNtk );
    Dss_NtkTransform( pNtk, pPermDsd );
//Dss_NtkPrint( pNtk );    
    pFun->iDsd = Dss_NtkRebuild( p, pNtk );
    Dss_NtkFree( pNtk );
    // pPermDsd maps vars of iDsdRes into literals of pTruth
    // translate this map into the one that maps vars of iDsdRes into literals of cut
    pFun->nFans = Dss_VecLitSuppSize( p->vObjs, pFun->iDsd );
    for ( i = 0; i < (int)pFun->nFans; i++ )
        pFun->pFans[i] = (unsigned char)Abc_Lit2LitV( pMapDsd2Truth, pPermDsd[i] );

//    Dss_EntPrint( pEnt, pFun );
    return pFun;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// returns mapping of variables of dsd1 into literals of dsd0
Dss_Ent_t * Dss_ManSharedMap( Dss_Man_t * p, int * iDsd, int * nFans, int ** pFans, unsigned uSharedMask )
{
    static char Buffer[100];
    Dss_Ent_t * pEnt = (Dss_Ent_t *)Buffer;
    pEnt->iDsd0 = iDsd[0];
    pEnt->iDsd1 = iDsd[1];
    pEnt->nShared = 0;
    if ( uSharedMask )
    {
        int i, g, pMapGtoL[DAU_MAX_VAR] = {-1};
        for ( i = 0; i < nFans[0]; i++ )
            pMapGtoL[ Abc_Lit2Var(pFans[0][i]) ] = Abc_Var2Lit( i, Abc_LitIsCompl(pFans[0][i]) );
        for ( i = 0; i < nFans[1]; i++ )
        {
            g = Abc_Lit2Var( pFans[1][i] );
            if ( (uSharedMask >> g) & 1 )
            {
                assert( pMapGtoL[g] >= 0 );
                pEnt->pShared[2*pEnt->nShared+0] = (unsigned char)i;
                pEnt->pShared[2*pEnt->nShared+1] = (unsigned char)Abc_LitNotCond( pMapGtoL[g], Abc_LitIsCompl(pFans[1][i]) );
                pEnt->nShared++;
            }
        }
    }
    pEnt->nWords = Dss_EntWordNum( pEnt );
    return pEnt;
}

// merge two DSD functions
int Dss_ManMerge( Dss_Man_t * p, int * iDsd, int * nFans, int ** pFans, unsigned uSharedMask, int nKLutSize, unsigned char * pPermRes, word * pTruth )
{
    int fVerbose = 0;
    int fCheck = 0;
    static int Counter = 0;
//    word pTtTemp[DAU_MAX_WORD];
    word * pTruthOne;
    int pPermResInt[DAU_MAX_VAR];
    Dss_Ent_t * pEnt, ** ppSpot;
    Dss_Fun_t * pFun;
    int i;
    abctime clk;
    Counter++;
    if ( DAU_MAX_VAR < nKLutSize )
    {
        printf( "Parameter DAU_MAX_VAR (%d) smaller than LUT size (%d).\n", DAU_MAX_VAR, nKLutSize );
        return -1;
    }
    assert( iDsd[0] <= iDsd[1] );

if ( fVerbose )
{
Dss_ManPrintOne( stdout, p, iDsd[0], pFans[0] );
Dss_ManPrintOne( stdout, p, iDsd[1], pFans[1] );
} 

    // constant argument
    if ( iDsd[0] == 0 ) return 0;
    if ( iDsd[0] == 1 ) return iDsd[1];
    if ( iDsd[1] == 0 ) return 0;
    if ( iDsd[1] == 1 ) return iDsd[0];

    // no overlap
clk = Abc_Clock();
    assert( nFans[0] == Dss_VecLitSuppSize(p->vObjs, iDsd[0]) );
    assert( nFans[1] == Dss_VecLitSuppSize(p->vObjs, iDsd[1]) );
    assert( nFans[0] + nFans[1] <= nKLutSize + Dss_WordCountOnes(uSharedMask) );
    // create map of shared variables
    pEnt = Dss_ManSharedMap( p, iDsd, nFans, pFans, uSharedMask );
p->timeBeg += Abc_Clock() - clk;
    // check cache
    if ( p->pCache == NULL )
    {
clk = Abc_Clock();
        if ( uSharedMask == 0 )
            pFun = Dss_ManOperationFun( p, iDsd, nFans[0] + nFans[1] );
        else
            pFun = Dss_ManBooleanAnd( p, pEnt, 0 );
        if ( pFun == NULL )
            return -1;
        assert( (int)pFun->nFans == Dss_VecLitSuppSize(p->vObjs, pFun->iDsd) );
        assert( (int)pFun->nFans <= nKLutSize );
p->timeDec += Abc_Clock() - clk;
    }
    else
    {
clk = Abc_Clock();
        ppSpot = Dss_ManCacheLookup( p, pEnt );
p->timeLook += Abc_Clock() - clk;
clk = Abc_Clock();
        if ( *ppSpot == NULL )
        {
            if ( uSharedMask == 0 )
                pFun = Dss_ManOperationFun( p, iDsd, nFans[0] + nFans[1] );
            else
                pFun = Dss_ManBooleanAnd( p, pEnt, 0 );
            if ( pFun == NULL )
                return -1;
            assert( (int)pFun->nFans == Dss_VecLitSuppSize(p->vObjs, pFun->iDsd) );
            assert( (int)pFun->nFans <= nKLutSize );
            // create cache entry
            *ppSpot = Dss_ManCacheCreate( p, pEnt, pFun );
        }
        pFun = (*ppSpot)->pFunc;
p->timeDec += Abc_Clock() - clk;
    }

clk = Abc_Clock();
    for ( i = 0; i < (int)pFun->nFans; i++ )
        if ( pFun->pFans[i] < 2 * nFans[0] ) // first dec
            pPermRes[i] = (unsigned char)Dss_Lit2Lit( pFans[0], pFun->pFans[i] );
        else
            pPermRes[i] = (unsigned char)Dss_Lit2Lit( pFans[1], pFun->pFans[i] - 2 * nFans[0] );
    // perform support minimization
    if ( uSharedMask && pFun->nFans > 1 )
    {
        int pVarPres[DAU_MAX_VAR];
        int nSupp = 0;
        for ( i = 0; i < p->nVars; i++ )
            pVarPres[i] = -1;
        for ( i = 0; i < (int)pFun->nFans; i++ )
            pVarPres[ Abc_Lit2Var(pPermRes[i]) ] = i;
        for ( i = 0; i < p->nVars; i++ )
            if ( pVarPres[i] >= 0 )
                pPermRes[pVarPres[i]] = Abc_Var2Lit( nSupp++, Abc_LitIsCompl(pPermRes[pVarPres[i]]) );
        assert( nSupp == (int)pFun->nFans );
    }

    for ( i = 0; i < (int)pFun->nFans; i++ )
        pPermResInt[i] = pPermRes[i];
p->timeEnd += Abc_Clock() - clk;

if ( fVerbose )
{
Dss_ManPrintOne( stdout, p, pFun->iDsd, pPermResInt );
printf( "\n" );
}

if ( Counter == 43418 )
{
//    int s = 0;
//    Dss_ManPrint( NULL, p );
}


    if ( fCheck )
    {
        pTruthOne = Dss_ManComputeTruth( p, pFun->iDsd, p->nVars, pPermResInt );
        if ( !Abc_TtEqual( pTruthOne, pTruth, Abc_TtWordNum(p->nVars) ) )
        {
            int s;
    //        Kit_DsdPrintFromTruth( pTruthOne, p->nVars );  printf( "\n" );
    //        Kit_DsdPrintFromTruth( pTruth, p->nVars );     printf( "\n" );
            printf( "Verification failed.\n" );
            s = 0;
        }
    }
    return pFun->iDsd;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Dss_Ent_t * Dss_ManSharedMapDerive( Dss_Man_t * p, int iDsd0, int iDsd1, Vec_Str_t * vShared )
{
    static char Buffer[100];
    Dss_Ent_t * pEnt = (Dss_Ent_t *)Buffer;
    pEnt->iDsd0 = iDsd0;
    pEnt->iDsd1 = iDsd1;
    pEnt->nShared = Vec_StrSize(vShared)/2;
    memcpy( pEnt->pShared, (unsigned char *)Vec_StrArray(vShared), sizeof(char) * Vec_StrSize(vShared) );
    pEnt->nWords = Dss_EntWordNum( pEnt );
    return pEnt;
}

int Mpm_FuncCompute( Dss_Man_t * p, int iDsd0, int iDsd1, Vec_Str_t * vShared, int * pPerm, int * pnLeaves )
{
    int fVerbose = 0;
//    int fCheck = 0;
    Dss_Ent_t * pEnt, ** ppSpot;
    Dss_Fun_t * pFun;
    int iDsd[2] = { iDsd0, iDsd1 };
    int i;
    abctime clk;

    assert( iDsd0 <= iDsd1 );
    if ( DAU_MAX_VAR < *pnLeaves )
    {
        printf( "Parameter DAU_MAX_VAR (%d) smaller than LUT size (%d).\n", DAU_MAX_VAR, *pnLeaves );
        return -1;
    }
    if ( fVerbose )
    {
        Dss_ManPrintOne( stdout, p, iDsd0, NULL );
        Dss_ManPrintOne( stdout, p, iDsd1, NULL );
    } 

clk = Abc_Clock();
    pEnt = Dss_ManSharedMapDerive( p, iDsd0, iDsd1, vShared );
    ppSpot = Dss_ManCacheLookup( p, pEnt );
p->timeLook += Abc_Clock() - clk;

clk = Abc_Clock();
    if ( *ppSpot == NULL )
    {
        if ( Vec_StrSize(vShared) == 0 )
            pFun = Dss_ManOperationFun( p, iDsd, *pnLeaves );
        else
            pFun = Dss_ManBooleanAnd( p, pEnt, 0 );
        if ( pFun == NULL )
            return -1;
        assert( (int)pFun->nFans == Dss_VecLitSuppSize(p->vObjs, pFun->iDsd) );
        assert( (int)pFun->nFans <= *pnLeaves );
        // create cache entry
        *ppSpot = Dss_ManCacheCreate( p, pEnt, pFun );
    }
    pFun = (*ppSpot)->pFunc;
p->timeDec += Abc_Clock() - clk;

    *pnLeaves = (int)pFun->nFans;
    for ( i = 0; i < (int)pFun->nFans; i++ )
        pPerm[i] = (int)pFun->pFans[i];

    if ( fVerbose )
    {
        Dss_ManPrintOne( stdout, p, pFun->iDsd, NULL );
        printf( "\n" );
    }

/*
    if ( fCheck )
    {
        pTruthOne = Dss_ManComputeTruth( p, pFun->iDsd, p->nVars, pPermResInt );
        if ( !Abc_TtEqual( pTruthOne, pTruth, Abc_TtWordNum(p->nVars) ) )
        {
            int s;
    //        Kit_DsdPrintFromTruth( pTruthOne, p->nVars );  printf( "\n" );
    //        Kit_DsdPrintFromTruth( pTruth, p->nVars );     printf( "\n" );
            printf( "Verification failed.\n" );
            s = 0;
        }
    }
*/
    return pFun->iDsd;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Dss_ObjCheckTransparent( Dss_Man_t * p, Dss_Obj_t * pObj )
{
    Dss_Obj_t * pFanin;
    int i;
    if ( pObj->Type == DAU_DSD_VAR )
        return 1;
    if ( pObj->Type == DAU_DSD_AND )
        return 0;
    if ( pObj->Type == DAU_DSD_XOR )
    {
        Dss_ObjForEachFanin( p->vObjs, pObj, pFanin, i )
            if ( Dss_ObjCheckTransparent( p, pFanin ) )
                return 1;
        return 0;
    }
    if ( pObj->Type == DAU_DSD_MUX )
    {
        pFanin = Dss_ObjFanin( p->vObjs, pObj, 1 );
        if ( !Dss_ObjCheckTransparent(p, pFanin) )
            return 0;
        pFanin = Dss_ObjFanin( p->vObjs, pObj, 2 );
        if ( !Dss_ObjCheckTransparent(p, pFanin) )
            return 0;
        return 1;
    }
    assert( pObj->Type == DAU_DSD_PRIME );
    return 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_DsdTest__()
{
    int nVars = 8;
//    char * pDsd = "[(ab)(cd)]";
    char * pDsd = "(!(a!(bh))[cde]!(fg))";
    Dss_Ntk_t * pNtk = Dss_NtkCreate( pDsd, nVars, NULL );
//    Dss_NtkPrint( pNtk );
//    Dss_NtkCheck( pNtk );
//    Dss_NtkTransform( pNtk );
//    Dss_NtkPrint( pNtk );
    Dss_NtkFree( pNtk );
    nVars = 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_DsdTest()
{
    int nVars = 8;
    Vec_Vec_t * vFuncs;
    Vec_Int_t * vOne, * vTwo, * vRes;//, * vThree;
    Dss_Man_t * p;
    int pEntries[3];
    int iLit, e0, e1;//, e2;
    int i, k, s;//, j;

    return;

    vFuncs = Vec_VecStart( nVars+1 );
    assert( nVars < DAU_MAX_VAR );
    p = Dss_ManAlloc( nVars, 0 );

    // init
    Vec_VecPushInt( vFuncs, 1, Dss_Obj2Lit(Dss_VecVar(p->vObjs,0)) );

    // enumerate
    for ( s = 2; s <= nVars; s++ )
    {
        vRes = Vec_VecEntryInt( vFuncs, s );
        for ( i = 1; i < s; i++ )
        for ( k = i; k < s; k++ )
        if ( i + k == s )
        {
            vOne = Vec_VecEntryInt( vFuncs, i );
            vTwo = Vec_VecEntryInt( vFuncs, k );
            Vec_IntForEachEntry( vOne, pEntries[0], e0 )
            Vec_IntForEachEntry( vTwo, pEntries[1], e1 )
            {
                int fAddInv0 = !Dss_ObjCheckTransparent( p, Dss_VecObj(p->vObjs, Abc_Lit2Var(pEntries[0])) );
                int fAddInv1 = !Dss_ObjCheckTransparent( p, Dss_VecObj(p->vObjs, Abc_Lit2Var(pEntries[1])) );

                iLit = Dss_ManOperation( p, DAU_DSD_AND, pEntries, 2, NULL, NULL );
                assert( !Abc_LitIsCompl(iLit) );
                Vec_IntPush( vRes, iLit );

                if ( fAddInv0 )
                {
                    pEntries[0] = Abc_LitNot( pEntries[0] );
                    iLit = Dss_ManOperation( p, DAU_DSD_AND, pEntries, 2, NULL, NULL );
                    pEntries[0] = Abc_LitNot( pEntries[0] );
                    assert( !Abc_LitIsCompl(iLit) );
                    Vec_IntPush( vRes, iLit );
                }

                if ( fAddInv1 )
                {
                    pEntries[1] = Abc_LitNot( pEntries[1] );
                    iLit = Dss_ManOperation( p, DAU_DSD_AND, pEntries, 2, NULL, NULL );
                    pEntries[1] = Abc_LitNot( pEntries[1] );
                    assert( !Abc_LitIsCompl(iLit) );
                    Vec_IntPush( vRes, iLit );
                }

                if ( fAddInv0 && fAddInv1 )
                {
                    pEntries[0] = Abc_LitNot( pEntries[0] );
                    pEntries[1] = Abc_LitNot( pEntries[1] );
                    iLit = Dss_ManOperation( p, DAU_DSD_AND, pEntries, 2, NULL, NULL );
                    pEntries[0] = Abc_LitNot( pEntries[0] );
                    pEntries[1] = Abc_LitNot( pEntries[1] );
                    assert( !Abc_LitIsCompl(iLit) );
                    Vec_IntPush( vRes, iLit );
                }

                iLit = Dss_ManOperation( p, DAU_DSD_XOR, pEntries, 2, NULL, NULL );
                assert( !Abc_LitIsCompl(iLit) );
                Vec_IntPush( vRes, Abc_LitRegular(iLit) );
            }
        }
/*
        for ( i = 1; i < s; i++ )
        for ( k = 1; k < s; k++ )
        for ( j = 1; j < s; j++ )
        if ( i + k + j == s )
        {
            vOne   = Vec_VecEntryInt( vFuncs, i );
            vTwo   = Vec_VecEntryInt( vFuncs, k );
            vThree = Vec_VecEntryInt( vFuncs, j );
            Vec_IntForEachEntry( vOne,   pEntries[0], e0 )
            Vec_IntForEachEntry( vTwo,   pEntries[1], e1 )
            Vec_IntForEachEntry( vThree, pEntries[2], e2 )
            {
                int fAddInv0 = !Dss_ObjCheckTransparent( p, Dss_VecObj(p->vObjs, Abc_Lit2Var(pEntries[0])) );
                int fAddInv1 = !Dss_ObjCheckTransparent( p, Dss_VecObj(p->vObjs, Abc_Lit2Var(pEntries[1])) );
                int fAddInv2 = !Dss_ObjCheckTransparent( p, Dss_VecObj(p->vObjs, Abc_Lit2Var(pEntries[2])) );

                if ( !fAddInv0 && k > j )
                    continue;

                iLit = Dss_ManOperation( p, DAU_DSD_MUX, pEntries, 3, NULL, NULL );
                assert( !Abc_LitIsCompl(iLit) );
                Vec_IntPush( vRes, iLit );

                if ( fAddInv1 )
                {
                    pEntries[1] = Abc_LitNot( pEntries[1] );
                    iLit = Dss_ManOperation( p, DAU_DSD_MUX, pEntries, 3, NULL, NULL );
                    pEntries[1] = Abc_LitNot( pEntries[1] );
                    assert( !Abc_LitIsCompl(iLit) );
                    Vec_IntPush( vRes, iLit );
                }

                if ( fAddInv2 )
                {
                    pEntries[2] = Abc_LitNot( pEntries[2] );
                    iLit = Dss_ManOperation( p, DAU_DSD_MUX, pEntries, 3, NULL, NULL );
                    pEntries[2] = Abc_LitNot( pEntries[2] );
                    assert( !Abc_LitIsCompl(iLit) );
                    Vec_IntPush( vRes, iLit );
                }
            }
        }
*/
        Vec_IntUniqify( vRes );
    }
    Dss_ManPrint( "_npn/npn/dsdcanon.txt", p );

    Dss_ManFree( p );
    Vec_VecFree( vFuncs );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Dau_DsdTest444()
{
    Dss_Man_t * p = Dss_ManAlloc( 6, 0 );
    int iLit1[3] = { 2, 4 };
    int iLit2[3] = { 2, 4, 6 };
    int iRes[5];
    int nFans[2] = { 4, 3 };
    int pPermLits1[4] = { 0, 2, 5, 6 };
    int pPermLits2[5] = { 2, 9, 10 };
    int * pPermLits[2] = { pPermLits1, pPermLits2 };
    unsigned char pPermRes[6];
    int pPermResInt[6];
    unsigned uMaskShared = 2;
    int i;

    iRes[0] = 1 ^ Dss_ManOperation( p, DAU_DSD_AND, iLit1, 2, NULL, NULL );
    iRes[1] = iRes[0]; 
    iRes[2] = 1 ^ Dss_ManOperation( p, DAU_DSD_AND, iRes, 2, NULL, NULL );
    iRes[3] = Dss_ManOperation( p, DAU_DSD_AND, iLit2, 3, NULL, NULL );

    Dss_ManPrintOne( stdout, p, iRes[0], NULL );
    Dss_ManPrintOne( stdout, p, iRes[2], NULL );
    Dss_ManPrintOne( stdout, p, iRes[3], NULL );

    Dss_ManPrintOne( stdout, p, iRes[2], pPermLits1 );
    Dss_ManPrintOne( stdout, p, iRes[3], pPermLits2 );

    iRes[4] = Dss_ManMerge( p, iRes+2, nFans, pPermLits, uMaskShared, 6, pPermRes, NULL );

    for ( i = 0; i < 6; i++ )
        pPermResInt[i] = pPermRes[i];

    Dss_ManPrintOne( stdout, p, iRes[4], NULL );
    Dss_ManPrintOne( stdout, p, iRes[4], pPermResInt );

    Dss_ManFree( p );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

