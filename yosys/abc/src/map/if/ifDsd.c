/**CFile****************************************************************

  FileName    [ifDsd.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [Computation of DSD representation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifTruth.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include <math.h>
#include "if.h"
#include "ifCount.h"
#include "misc/extra/extra.h"
#include "sat/bsat/satSolver.h"
#include "aig/gia/gia.h"
#include "bool/kit/kit.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

#ifdef ABC_USE_PTHREADS

#ifdef _WIN32
#include "../lib/pthread.h"
#else
#include <pthread.h>
#include <unistd.h>
#endif

#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define DSD_VERSION "dsd1"

// network types
typedef enum { 
    IF_DSD_NONE = 0,               // 0:  unknown
    IF_DSD_CONST0,                 // 1:  constant
    IF_DSD_VAR,                    // 2:  variable
    IF_DSD_AND,                    // 3:  AND
    IF_DSD_XOR,                    // 4:  XOR
    IF_DSD_MUX,                    // 5:  MUX
    IF_DSD_PRIME                   // 6:  PRIME
} If_DsdType_t;

typedef struct If_DsdObj_t_ If_DsdObj_t;
struct If_DsdObj_t_
{
    unsigned       Id;             // node ID
    unsigned       Type    :  3;   // node type
    unsigned       nSupp   :  5;   // variable
    unsigned       fMark   :  1;   // user mark
    unsigned       Count   : 18;   // variable
    unsigned       nFans   :  5;   // fanin count
    unsigned       pFans[0];       // fanins
};

struct If_DsdMan_t_
{
    char *         pStore;         // input/output file
    int            nVars;          // max var number
    int            LutSize;        // LUT size
    int            nWords;         // word number
    int            nBins;          // table size
    unsigned *     pBins;          // hash table
    Mem_Flex_t *   pMem;           // memory for nodes
    Vec_Ptr_t      vObjs;          // objects
    Vec_Int_t      vNexts;         // next pointers
    Vec_Int_t      vTruths;        // truth IDs of prime nodes
    Vec_Int_t *    vTemp1;         // temp
    Vec_Int_t *    vTemp2;         // temp
    word **        pTtElems;       // elementary TTs
    Vec_Mem_t *    vTtMem[IF_MAX_FUNC_LUTSIZE+1];  // truth table memory and hash table
    Vec_Ptr_t *    vTtDecs[IF_MAX_FUNC_LUTSIZE+1]; // truth table decompositions
    Vec_Wec_t *    vIsops[IF_MAX_FUNC_LUTSIZE+1];  // ISOP for each function
    int *          pSched[IF_MAX_FUNC_LUTSIZE+1];  // grey code schedules
    int            nTtBits;        // the number of truth table bits
    int            nConfigWords;   // the number of words for config data per node
    Vec_Wrd_t *    vConfigs;       // permutations
    Gia_Man_t *    pTtGia;         // GIA to represent truth tables
    Vec_Int_t *    vCover;         // temporary memory
    void *         pSat;           // SAT solver
    char *         pCellStr;       // symbolic cell description
    int            nObjsPrev;      // previous number of objects
    int            fNewAsUseless;  // set new as useless
    int            nUniqueHits;    // statistics
    int            nUniqueMisses;  // statistics
    abctime        timeDsd;        // statistics
    abctime        timeCanon;      // statistics
    abctime        timeCheck;      // statistics
    abctime        timeCheck2;     // statistics
    abctime        timeVerify;     // statistics
};

static inline int           If_DsdObjWordNum( int nFans )                                    { return sizeof(If_DsdObj_t) / 8 + nFans / 2 + ((nFans & 1) > 0);              }
static inline int           If_DsdObjTruthId( If_DsdMan_t * p, If_DsdObj_t * pObj )          { return (pObj->Type == IF_DSD_PRIME && pObj->nFans > 2) ? Vec_IntEntry(&p->vTruths, pObj->Id) : -1;     }
static inline word *        If_DsdObjTruth( If_DsdMan_t * p, If_DsdObj_t * pObj )            { return Vec_MemReadEntry(p->vTtMem[pObj->nFans], If_DsdObjTruthId(p, pObj));  }
static inline void          If_DsdObjSetTruth( If_DsdMan_t * p, If_DsdObj_t * pObj, int Id ) { assert( pObj->Type == IF_DSD_PRIME && pObj->nFans > 2 ); Vec_IntWriteEntry(&p->vTruths, pObj->Id, Id); }

static inline void          If_DsdObjClean( If_DsdObj_t * pObj )                       { memset( pObj, 0, sizeof(If_DsdObj_t) );                                            }
static inline int           If_DsdObjId( If_DsdObj_t * pObj )                          { return pObj->Id;                                                                   }
static inline int           If_DsdObjType( If_DsdObj_t * pObj )                        { return pObj->Type;                                                                 }
static inline int           If_DsdObjIsVar( If_DsdObj_t * pObj )                       { return (int)(pObj->Type == IF_DSD_VAR);                                            }
static inline int           If_DsdObjSuppSize( If_DsdObj_t * pObj )                    { return pObj->nSupp;                                                                }
static inline int           If_DsdObjFaninNum( If_DsdObj_t * pObj )                    { return pObj->nFans;                                                                }
static inline int           If_DsdObjFaninC( If_DsdObj_t * pObj, int i )               { assert(i < (int)pObj->nFans); return Abc_LitIsCompl(pObj->pFans[i]);               }
static inline int           If_DsdObjFaninLit( If_DsdObj_t * pObj, int i )             { assert(i < (int)pObj->nFans); return pObj->pFans[i];                               }

static inline If_DsdObj_t * If_DsdVecObj( Vec_Ptr_t * p, int Id )                      { return (If_DsdObj_t *)Vec_PtrEntry(p, Id);                                         }
static inline If_DsdObj_t * If_DsdVecConst0( Vec_Ptr_t * p )                           { return If_DsdVecObj( p, 0 );                                                       }
static inline If_DsdObj_t * If_DsdVecVar( Vec_Ptr_t * p, int v )                       { return If_DsdVecObj( p, v+1 );                                                     }
static inline int           If_DsdVecObjSuppSize( Vec_Ptr_t * p, int iObj )            { return If_DsdVecObj( p, iObj )->nSupp;                                             }
static inline int           If_DsdVecLitSuppSize( Vec_Ptr_t * p, int iLit )            { return If_DsdVecObjSuppSize( p, Abc_Lit2Var(iLit) );                               }
static inline int           If_DsdVecObjRef( Vec_Ptr_t * p, int iObj )                 { return If_DsdVecObj( p, iObj )->Count;                                             }
static inline void          If_DsdVecObjIncRef( Vec_Ptr_t * p, int iObj )              { if ( If_DsdVecObjRef(p, iObj) < 0x3FFFF ) If_DsdVecObj( p, iObj )->Count++;        }
static inline If_DsdObj_t * If_DsdObjFanin( Vec_Ptr_t * p, If_DsdObj_t * pObj, int i ) { assert(i < (int)pObj->nFans); return If_DsdVecObj(p, Abc_Lit2Var(pObj->pFans[i])); }
static inline int           If_DsdVecObjMark( Vec_Ptr_t * p, int iObj )                { return If_DsdVecObj( p, iObj )->fMark;                                             }
static inline void          If_DsdVecObjSetMark( Vec_Ptr_t * p, int iObj )             { If_DsdVecObj( p, iObj )->fMark = 1;                                                }
static inline void          If_DsdVecObjClearMark( Vec_Ptr_t * p, int iObj )           { If_DsdVecObj( p, iObj )->fMark = 0;                                                }

#define If_DsdVecForEachObj( vVec, pObj, i )                \
    Vec_PtrForEachEntry( If_DsdObj_t *, vVec, pObj, i )
#define If_DsdVecForEachObjStart( vVec, pObj, i, Start )    \
    Vec_PtrForEachEntryStart( If_DsdObj_t *, vVec, pObj, i, Start )
#define If_DsdVecForEachObjVec( vNodes, vVec, pObj, i )     \
    for ( i = 0; (i < Vec_IntSize(vNodes)) && ((pObj) = If_DsdVecObj(vVec, Vec_IntEntry(vNodes,i))); i++ )
#define If_DsdVecForEachNode( vVec, pObj, i )               \
    Vec_PtrForEachEntryStart( If_DsdObj_t *, vVec, pObj, i, 2 )
#define If_DsdObjForEachFanin( vVec, pObj, pFanin, i )      \
    for ( i = 0; (i < If_DsdObjFaninNum(pObj)) && ((pFanin) = If_DsdObjFanin(vVec, pObj, i)); i++ )
#define If_DsdObjForEachFaninLit( vVec, pObj, iLit, i )     \
    for ( i = 0; (i < If_DsdObjFaninNum(pObj)) && ((iLit) = If_DsdObjFaninLit(pObj, i)); i++ )

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
char * If_DsdManFileName( If_DsdMan_t * p )
{
    return p->pStore;
}
int If_DsdManVarNum( If_DsdMan_t * p )
{
    return p->nVars;
}
int If_DsdManObjNum( If_DsdMan_t * p )
{
    return Vec_PtrSize( &p->vObjs );
}
int If_DsdManLutSize( If_DsdMan_t * p )
{
    return p->LutSize;
}
int If_DsdManTtBitNum( If_DsdMan_t * p )
{
    return p->nTtBits;
}
int If_DsdManPermBitNum( If_DsdMan_t * p )
{
    return (Abc_Base2Log(p->nVars + 1) + 1) * p->nVars;
}
void If_DsdManSetLutSize( If_DsdMan_t * p, int nLutSize )
{
    p->LutSize = nLutSize;
}
int If_DsdManSuppSize( If_DsdMan_t * p, int iDsd )
{
    return If_DsdVecLitSuppSize( &p->vObjs, iDsd );
}
int If_DsdManCheckDec( If_DsdMan_t * p, int iDsd )
{
    return If_DsdVecObjMark( &p->vObjs, Abc_Lit2Var(iDsd) );
}
int If_DsdManReadMark( If_DsdMan_t * p, int iDsd )
{
    return If_DsdVecObjMark( &p->vObjs, Abc_Lit2Var(iDsd) );
}
void If_DsdManSetNewAsUseless( If_DsdMan_t * p )
{
    if ( p->nObjsPrev == 0 )
        p->nObjsPrev = If_DsdManObjNum(p);
    p->fNewAsUseless = 1;
}
word * If_DsdManGetFuncConfig( If_DsdMan_t * p, int iDsd )
{
    return p->vConfigs ? Vec_WrdEntryP(p->vConfigs, p->nConfigWords * Abc_Lit2Var(iDsd)) : NULL;
}
char * If_DsdManGetCellStr( If_DsdMan_t * p )
{
    return p->pCellStr;
}

/**Function*************************************************************

  Synopsis    [DSD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word ** If_ManDsdTtElems()
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
If_DsdObj_t * If_DsdObjAlloc( If_DsdMan_t * p, int Type, int nFans )
{
    int nWords = If_DsdObjWordNum( nFans );
    If_DsdObj_t * pObj = (If_DsdObj_t *)Mem_FlexEntryFetch( p->pMem, sizeof(word) * nWords );
    If_DsdObjClean( pObj );
    pObj->Type   = Type;
    pObj->nFans  = nFans;
    pObj->Id     = Vec_PtrSize( &p->vObjs );
    pObj->fMark  = p->fNewAsUseless;
    pObj->Count  = 0;
    Vec_PtrPush( &p->vObjs, pObj );
    Vec_IntPush( &p->vNexts, 0 );
    Vec_IntPush( &p->vTruths, -1 );
    assert( Vec_IntSize(&p->vNexts) == Vec_PtrSize(&p->vObjs) );
    assert( Vec_IntSize(&p->vTruths) == Vec_PtrSize(&p->vObjs) );
    return pObj;
}
If_DsdMan_t * If_DsdManAlloc( int nVars, int LutSize )
{
    If_DsdMan_t * p; int v;
    char pFileName[100];
    assert( nVars <= DAU_MAX_VAR );
    sprintf( pFileName, "%02d.dsd", nVars );
    p = ABC_CALLOC( If_DsdMan_t, 1 );
    p->pStore  = Abc_UtilStrsav( pFileName );
    p->nVars   = nVars;
    p->LutSize = LutSize;
    p->nWords  = Abc_TtWordNum( nVars );
    p->nBins   = Abc_PrimeCudd( 100000 );
    p->pBins   = ABC_CALLOC( unsigned, p->nBins );
    p->pMem    = Mem_FlexStart();
    p->nConfigWords = 1;
    Vec_PtrGrow( &p->vObjs, 10000 );
    Vec_IntGrow( &p->vNexts, 10000 );
    Vec_IntGrow( &p->vTruths, 10000 );
    If_DsdObjAlloc( p, IF_DSD_CONST0, 0 );
    If_DsdObjAlloc( p, IF_DSD_VAR, 0 )->nSupp = 1;
    p->vTemp1   = Vec_IntAlloc( 32 );
    p->vTemp2   = Vec_IntAlloc( 32 );
    p->pTtElems = If_ManDsdTtElems();
    for ( v = 3; v <= nVars; v++ )
    {
        p->vTtMem[v] = Vec_MemAlloc( Abc_TtWordNum(v), 12 );
        Vec_MemHashAlloc( p->vTtMem[v], 10000 );
        p->vTtDecs[v] = Vec_PtrAlloc( 1000 );
    }
/*
    p->pTtGia   = Gia_ManStart( nVars );
    Gia_ManHashAlloc( p->pTtGia );
    for ( v = 0; v < nVars; v++ )
        Gia_ManAppendCi( p->pTtGia );
*/
    for ( v = 2; v < nVars; v++ )
        p->pSched[v] = Extra_GreyCodeSchedule( v );
    if ( LutSize )
    p->pSat     = If_ManSatBuildXY( LutSize );
    p->vCover   = Vec_IntAlloc( 0 );
    return p;
}
void If_DsdManAllocIsops( If_DsdMan_t * p, int nLutSize )
{
    Vec_Int_t * vLevel;
    int v, i, fCompl;
    word * pTruth;
    if ( p->vIsops[3] != NULL )
        return;
    if ( Vec_PtrSize(&p->vObjs) > 2 )
        printf( "Warning: DSD manager is already started without ISOPs.\n" );
    for ( v = 3; v <= nLutSize; v++ )
    {
        p->vIsops[v] = Vec_WecAlloc( 100 );
        Vec_MemForEachEntry( p->vTtMem[v], pTruth, i )
        {
            vLevel = Vec_WecPushLevel( p->vIsops[v] );
            fCompl = Kit_TruthIsop( (unsigned *)pTruth, v, p->vCover, 1 );
            if ( fCompl >= 0 && Vec_IntSize(p->vCover) <= 8 )
            {
                Vec_IntGrow( vLevel, Vec_IntSize(p->vCover) );
                Vec_IntAppend( vLevel, p->vCover );
                if ( fCompl )
                    vLevel->nCap ^= (1<<16); // hack to remember complemented attribute
            }
        }
        assert( Vec_WecSize(p->vIsops[v]) == Vec_MemEntryNum(p->vTtMem[v]) );
    }
}
void If_DsdManFree( If_DsdMan_t * p, int fVerbose )
{
    int v;
//    If_DsdManDumpDsd( p );
    if ( fVerbose )
        If_DsdManPrint( p, NULL, 0, 0, 0, 0, 0 );
    if ( fVerbose )
    {
        char FileName[10];
        for ( v = 3; v <= p->nVars; v++ )
        {
            sprintf( FileName, "dumpdsd%02d", v );
            Vec_MemDumpTruthTables( p->vTtMem[v], FileName, v );
        }
    }
    for ( v = 2; v < p->nVars; v++ )
        ABC_FREE( p->pSched[v] );
    for ( v = 3; v <= p->nVars; v++ )
    {
        Vec_MemHashFree( p->vTtMem[v] );
        Vec_MemFree( p->vTtMem[v] );
        Vec_VecFree( (Vec_Vec_t *)(p->vTtDecs[v]) );
        if ( p->vIsops[v] )
            Vec_WecFree( p->vIsops[v] );
    }
    Vec_WrdFreeP( &p->vConfigs );
    Vec_IntFreeP( &p->vTemp1 );
    Vec_IntFreeP( &p->vTemp2 );
    ABC_FREE( p->vObjs.pArray );
    ABC_FREE( p->vNexts.pArray );
    ABC_FREE( p->vTruths.pArray );
    Mem_FlexStop( p->pMem, 0 );
    Gia_ManStopP( &p->pTtGia );
    Vec_IntFreeP( &p->vCover );
    If_ManSatUnbuild( p->pSat );
    ABC_FREE( p->pCellStr );
    ABC_FREE( p->pStore );
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}
void If_DsdManDumpDsd( If_DsdMan_t * p, int Support )
{
    char * pFileName = "tts_nondsd.txt";
    If_DsdObj_t * pObj; 
    Vec_Int_t * vMap;
    FILE * pFile = fopen( pFileName, "wb" );
    int v, i;
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\".\n", pFileName );
        return;
    }
    for ( v = 3; v <= p->nVars; v++ )
    {
        vMap = Vec_IntStart( Vec_MemEntryNum(p->vTtMem[v]) );
        If_DsdVecForEachObj( &p->vObjs, pObj, i )
        {
            if ( Support && Support != If_DsdObjSuppSize(pObj) )
                continue;
            if ( If_DsdObjType(pObj) != IF_DSD_PRIME )
                continue;
            if ( Vec_IntEntry(vMap, If_DsdObjTruthId(p, pObj)) )
                continue;
            Vec_IntWriteEntry(vMap, If_DsdObjTruthId(p, pObj), 1);
            fprintf( pFile, "0x" );
            Abc_TtPrintHexRev( pFile, If_DsdObjTruth(p, pObj), Support ? Abc_MaxInt(Support, 6) : v );
            fprintf( pFile, "\n" );
            //printf( "    " );
            //Dau_DsdPrintFromTruth( If_DsdObjTruth(p, pObj), p->nVars );
        }
        Vec_IntFree( vMap );
    }
    fclose( pFile );
}
void If_DsdManDumpAll( If_DsdMan_t * p, int Support )
{
    extern word * If_DsdManComputeTruth( If_DsdMan_t * p, int iDsd, unsigned char * pPermLits );
    char * pFileName = "tts_all.txt";
    If_DsdObj_t * pObj;
    word * pRes; int i;
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\".\n", pFileName );
        return;
    }
    If_DsdVecForEachObj( &p->vObjs, pObj, i )
    {
        if ( Support && Support != If_DsdObjSuppSize(pObj) )
            continue;
        pRes = If_DsdManComputeTruth( p, Abc_Var2Lit(i, 0), NULL );
        fprintf( pFile, "0x" );
        Abc_TtPrintHexRev( pFile, pRes, Support ? Abc_MaxInt(Support, 6) : p->nVars );
        fprintf( pFile, "\n" );
//        printf( "    " );
//        Dau_DsdPrintFromTruth( pRes, p->nVars );
    }
    fclose( pFile );
}
int If_DsdManHasMarks( If_DsdMan_t * p )
{
    If_DsdObj_t * pObj;
    int i;
    If_DsdVecForEachObj( &p->vObjs, pObj, i )
        if ( pObj->fMark )
            return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Printing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_DsdManHashProfile( If_DsdMan_t * p )
{
    If_DsdObj_t * pObj;
    unsigned * pSpot;
    int i, Counter;
    for ( i = 0; i < p->nBins; i++ )
    {
        Counter = 0;
        for ( pSpot = p->pBins + i; *pSpot; pSpot = (unsigned *)Vec_IntEntryP(&p->vNexts, pObj->Id), Counter++ )
             pObj = If_DsdVecObj( &p->vObjs, *pSpot );
//        if ( Counter > 5 )
//            printf( "%d ", Counter );
//        if ( i > 10000 )
//            break;
    }
//    printf( "\n" );
}
int If_DsdManCheckNonDec_rec( If_DsdMan_t * p, int Id )
{
    If_DsdObj_t * pObj;
    int i, iFanin;
    pObj = If_DsdVecObj( &p->vObjs, Id );
    if ( If_DsdObjType(pObj) == IF_DSD_CONST0 )
        return 0;
    if ( If_DsdObjType(pObj) == IF_DSD_VAR )
        return 0;
    if ( If_DsdObjType(pObj) == IF_DSD_PRIME )
        return 1;
    If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
        if ( If_DsdManCheckNonDec_rec( p, Abc_Lit2Var(iFanin) ) )
            return 1;
    return 0;
}
void If_DsdManPrint_rec( FILE * pFile, If_DsdMan_t * p, int iDsdLit, unsigned char * pPermLits, int * pnSupp )
{
    char OpenType[7]  = {0, 0, 0, '(', '[', '<', '{'};
    char CloseType[7] = {0, 0, 0, ')', ']', '>', '}'};
    If_DsdObj_t * pObj;
    int i, iFanin;
    fprintf( pFile, "%s", Abc_LitIsCompl(iDsdLit) ? "!" : ""  );
    pObj = If_DsdVecObj( &p->vObjs, Abc_Lit2Var(iDsdLit) );
    if ( If_DsdObjType(pObj) == IF_DSD_CONST0 )
        { fprintf( pFile, "0" ); return; }
    if ( If_DsdObjType(pObj) == IF_DSD_VAR )
    {
        int iPermLit = pPermLits ? (int)pPermLits[(*pnSupp)++] : Abc_Var2Lit((*pnSupp)++, 0);
        fprintf( pFile, "%s%c", Abc_LitIsCompl(iPermLit)? "!":"", 'a' + Abc_Lit2Var(iPermLit) );
        return;
    }
    if ( If_DsdObjType(pObj) == IF_DSD_PRIME )
        Abc_TtPrintHexRev( pFile, If_DsdObjTruth(p, pObj), If_DsdObjFaninNum(pObj) );
    fprintf( pFile, "%c", OpenType[If_DsdObjType(pObj)] );
    If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
        If_DsdManPrint_rec( pFile, p, iFanin, pPermLits, pnSupp );
    fprintf( pFile, "%c", CloseType[If_DsdObjType(pObj)] );
}
void If_DsdManPrintOne( FILE * pFile, If_DsdMan_t * p, int iObjId, unsigned char * pPermLits, int fNewLine )
{
    int nSupp = 0;
    fprintf( pFile, "%6d : ", iObjId );
    fprintf( pFile, "%2d ",   If_DsdVecObjSuppSize(&p->vObjs, iObjId) );
    fprintf( pFile, "%8d ",   If_DsdVecObjRef(&p->vObjs, iObjId) );
    fprintf( pFile, "%d  ",    If_DsdVecObjMark(&p->vObjs, iObjId) );
    If_DsdManPrint_rec( pFile, p, Abc_Var2Lit(iObjId, 0), pPermLits, &nSupp );
    if ( fNewLine )
        fprintf( pFile, "\n" );
    assert( nSupp == If_DsdVecObjSuppSize(&p->vObjs, iObjId) );
}
#define DSD_ARRAY_LIMIT 16
void If_DsdManPrintDecs( FILE * pFile, If_DsdMan_t * p )
{
    Vec_Int_t * vDecs;
    int i, k, v, nSuppSize, nDecMax = 0;
    int pDecMax[IF_MAX_FUNC_LUTSIZE] = {0};
    int pCountsAll[IF_MAX_FUNC_LUTSIZE] = {0};
    int pCountsSSizes[IF_MAX_FUNC_LUTSIZE] = {0};
    int pCounts[IF_MAX_FUNC_LUTSIZE][DSD_ARRAY_LIMIT+2] = {{0}};
    word * pTruth;
    for ( v = 3; v <= p->nVars; v++ )
    {
        assert( Vec_MemEntryNum(p->vTtMem[v]) == Vec_PtrSize(p->vTtDecs[v]) );
        // find max number of decompositions
        Vec_PtrForEachEntry( Vec_Int_t *, p->vTtDecs[v], vDecs, i )
        {
            pTruth = Vec_MemReadEntry( p->vTtMem[v], i );
            nSuppSize = Abc_TtSupportSize( pTruth, p->nVars );
            pDecMax[nSuppSize] = Abc_MaxInt( pDecMax[nSuppSize], Vec_IntSize(vDecs) );
            nDecMax = Abc_MaxInt( nDecMax, Vec_IntSize(vDecs) );
        }
        // fill up
        Vec_PtrForEachEntry( Vec_Int_t *, p->vTtDecs[v], vDecs, i )
        {
            pTruth = Vec_MemReadEntry( p->vTtMem[v], i );
            nSuppSize = Abc_TtSupportSize( pTruth, p->nVars );
            pCountsAll[nSuppSize]++;
            pCountsSSizes[nSuppSize] += Vec_IntSize(vDecs);
            pCounts[nSuppSize][Abc_MinInt(DSD_ARRAY_LIMIT+1,Vec_IntSize(vDecs))]++;
    //        pCounts[nSuppSize][Abc_MinInt(DSD_ARRAY_LIMIT+1,Vec_IntSize(vDecs)?1+(Vec_IntSize(vDecs)/10):0)]++;
    /*
            if ( nSuppSize == 6 && Vec_IntSize(vDecs) == pDecMax[6] )
            {
                fprintf( pFile, "0x" );
                Abc_TtPrintHex( pTruth, nSuppSize );
                Dau_DecPrintSets( vDecs, nSuppSize );
            }
    */
        }
    }
    // print header
    fprintf( pFile, " N :  " );
    fprintf( pFile, " Total  " );
    for ( k = 0; k <= DSD_ARRAY_LIMIT; k++ )
        fprintf( pFile, "%6d", k );
    fprintf( pFile, "  " );
    fprintf( pFile, "  More" );
    fprintf( pFile, "     Ave" );
    fprintf( pFile, "     Max" );
    fprintf( pFile, "\n" );
    // print rows
    for ( i = 0; i <= p->nVars; i++ )
    {
        fprintf( pFile, "%2d :  ", i );
        fprintf( pFile, "%6d  ", pCountsAll[i] );
        for ( k = 0; k <= DSD_ARRAY_LIMIT; k++ )
//            fprintf( pFile, "%6d", pCounts[i][k] );
            fprintf( pFile, "%6.1f", 100.0*pCounts[i][k]/Abc_MaxInt(1,pCountsAll[i]) );
        fprintf( pFile, "  " );
//        fprintf( pFile, "%6d", pCounts[i][k] );
        fprintf( pFile, "%6.1f", 100.0*pCounts[i][k]/Abc_MaxInt(1,pCountsAll[i]) );
        fprintf( pFile, "  " );
        fprintf( pFile, "%6.1f", 1.0*pCountsSSizes[i]/Abc_MaxInt(1,pCountsAll[i]) );
        fprintf( pFile, "  " );
        fprintf( pFile, "%6d", pDecMax[i] );
        fprintf( pFile, "\n" );
    }
}
void If_DsdManPrintOccurs( FILE * pFile, If_DsdMan_t * p )
{
    char Buffer[100];
    If_DsdObj_t * pObj;
    Vec_Int_t * vOccurs;
    int nOccurs, nOccursMax, nOccursAll;
    int i, k, nSizeMax, Counter = 0;
    // determine the largest fanin and fanout
    nOccursMax = nOccursAll = 0;
    If_DsdVecForEachNode( &p->vObjs, pObj, i )
    {
        nOccurs = pObj->Count;
        nOccursAll += nOccurs;
        nOccursMax  = Abc_MaxInt( nOccursMax, nOccurs );
    }
    // allocate storage for fanin/fanout numbers
    nSizeMax = 10 * (Abc_Base10Log(nOccursMax) + 1);
    vOccurs  = Vec_IntStart( nSizeMax );
    // count the number of fanins and fanouts
    If_DsdVecForEachNode( &p->vObjs, pObj, i )
    {
        nOccurs = pObj->Count;
        if ( nOccurs < 10 )
            Vec_IntAddToEntry( vOccurs, nOccurs, 1 );
        else if ( nOccurs < 100 )
            Vec_IntAddToEntry( vOccurs, 10 + nOccurs/10, 1 );
        else if ( nOccurs < 1000 )
            Vec_IntAddToEntry( vOccurs, 20 + nOccurs/100, 1 );
        else if ( nOccurs < 10000 )
            Vec_IntAddToEntry( vOccurs, 30 + nOccurs/1000, 1 );
        else if ( nOccurs < 100000 )
            Vec_IntAddToEntry( vOccurs, 40 + nOccurs/10000, 1 );
        else if ( nOccurs < 1000000 )
            Vec_IntAddToEntry( vOccurs, 50 + nOccurs/100000, 1 );
        else if ( nOccurs < 10000000 )
            Vec_IntAddToEntry( vOccurs, 60 + nOccurs/1000000, 1 );
    }
    fprintf( pFile, "The distribution of object occurrences:\n" );
    for ( k = 0; k < nSizeMax; k++ )
    {
        if ( Vec_IntEntry(vOccurs, k) == 0 )
            continue;
        if ( k < 10 )
            fprintf( pFile, "%15d : ", k );
        else
        {
            sprintf( Buffer, "%d - %d", (int)pow((double)10, k/10) * (k%10), (int)pow((double)10, k/10) * (k%10+1) - 1 );
            fprintf( pFile, "%15s : ", Buffer );
        }
        fprintf( pFile, "%12d   ", Vec_IntEntry(vOccurs, k) );
        Counter += Vec_IntEntry(vOccurs, k);
        fprintf( pFile, "(%6.2f %%)", 100.0*Counter/Vec_PtrSize(&p->vObjs) );
        fprintf( pFile, "\n" );
    }
    Vec_IntFree( vOccurs );
    fprintf( pFile, "Fanins: Max = %d. Ave = %.2f.\n", nOccursMax,  1.0*nOccursAll/Vec_PtrSize(&p->vObjs) );
}

void If_DsdManPrintDistrib( If_DsdMan_t * p )
{
    If_DsdObj_t * pObj; int i;
    int CountObj[IF_MAX_FUNC_LUTSIZE+2] = {0};
    int CountObjNon[IF_MAX_FUNC_LUTSIZE+2] = {0};
    int CountObjNpn[IF_MAX_FUNC_LUTSIZE+2] = {0};
    int CountStr[IF_MAX_FUNC_LUTSIZE+2] = {0};
    int CountStrNon[IF_MAX_FUNC_LUTSIZE+2] = {0};
    int CountMarked[IF_MAX_FUNC_LUTSIZE+2] = {0};
    for ( i = 3; i <= p->nVars; i++ )
    {
        CountObjNpn[i] = Vec_MemEntryNum(p->vTtMem[i]);
        CountObjNpn[p->nVars+1] += Vec_MemEntryNum(p->vTtMem[i]);
    }
    If_DsdVecForEachObj( &p->vObjs, pObj, i )
    {
        CountObj[If_DsdObjFaninNum(pObj)]++,        CountObj[p->nVars+1]++;
        if ( If_DsdObjType(pObj) == IF_DSD_PRIME )
            CountObjNon[If_DsdObjFaninNum(pObj)]++, CountObjNon[p->nVars+1]++;
        CountStr[If_DsdObjSuppSize(pObj)]++,        CountStr[p->nVars+1]++;
        if ( If_DsdManCheckNonDec_rec(p, i) )
            CountStrNon[If_DsdObjSuppSize(pObj)]++, CountStrNon[p->nVars+1]++;
        if ( If_DsdVecObjMark(&p->vObjs, i) )
            CountMarked[If_DsdObjSuppSize(pObj)]++, CountMarked[p->nVars+1]++;
    }
    printf( "***** DSD MANAGER STATISTICS *****\n" );
    printf( "Support     " );
    printf( "Obj   " );
    printf( "ObjNDSD            " );
    printf( "NPNNDSD                  " );
    printf( "Str   " );
    printf( "StrNDSD             " );
    printf( "Marked  " );
    printf( "\n" );
    for ( i = 0; i <= p->nVars + 1; i++ )
    {
        if ( i == p->nVars + 1 )
            printf( "All : " );
        else
            printf( "%3d : ", i );
        printf( "%9d ", CountObj[i] );
        printf( "%9d ", CountObjNon[i] );
        printf( "%6.2f %% ", 100.0 * CountObjNon[i] / Abc_MaxInt(1, CountObj[i]) );
        printf( "%9d ", CountObjNpn[i] );
        printf( "%6.2f %% ", 100.0 * CountObjNpn[i] / Abc_MaxInt(1, CountObj[i]) );
        printf( "  " );
        printf( "%9d ", CountStr[i] );
        printf( "%9d ", CountStrNon[i] );
        printf( "%6.2f %% ", 100.0 * CountStrNon[i] / Abc_MaxInt(1, CountStr[i]) );
        printf( "%9d ", CountMarked[i] );
        printf( "%6.2f %%",  100.0 * CountMarked[i] / Abc_MaxInt(1, CountStr[i]) );
        printf( "\n" );
    }
}
void If_DsdManPrint( If_DsdMan_t * p, char * pFileName, int Number, int Support, int fOccurs, int fTtDump, int fVerbose )
{
    If_DsdObj_t * pObj;
    Vec_Int_t * vStructs, * vCounts;
    int CountUsed = 0, CountNonDsd = 0, CountNonDsdStr = 0, CountMarked = 0, CountPrime = 0;
    int i, v, * pPerm, DsdMax = 0, MemSizeTTs = 0, MemSizeDecs = 0;
    FILE * pFile;
    pFile = pFileName ? fopen( pFileName, "wb" ) : stdout;
    if ( pFileName && pFile == NULL )
    {
        printf( "cannot open output file\n" );
        return;
    }
    if ( fVerbose )
    {
        fprintf( pFile, "*****  NOTATIONS USED BELOW  *****\n" );
        fprintf( pFile, "Support -- the support size\n" );
        fprintf( pFile, "Obj     -- the number of nodes in the DSD manager for each support size\n" );
        fprintf( pFile, "           (the constant node and the primary input node have no support)\n" );
        fprintf( pFile, "ObjNDSD -- the number of prime nodes (that is, nodes whose function has no DSD)\n" );
        fprintf( pFile, "           (percentage is relative to the number of all nodes of that size)\n" );
        fprintf( pFile, "NPNNDSD -- the number of different NPN classes of prime nodes\n" );
        fprintf( pFile, "           (Each NPN class may appear more than once. For example: F1 = 17(ab(cd))\n" ); 
        fprintf( pFile, "           and F2 = 17(ab[cd]) both have prime majority node (hex TT is 17),\n" ); 
        fprintf( pFile, "           but in one case the majority node is fed by AND, and in another by XOR.\n" );
        fprintf( pFile, "           These two majority nodes are different nodes in the DSD manager\n" );
        fprintf( pFile, "Str     -- the number of structures for each support size\n" );
        fprintf( pFile, "           (each structure is composed of one or more nodes)\n" );
        fprintf( pFile, "StrNDSD -- the number of DSD structures containing at least one prime node\n" );
        fprintf( pFile, "Marked  -- the number of DSD structures matchable with the LUT structure (say, \"44\")\n" );
    }
    If_DsdVecForEachObj( &p->vObjs, pObj, i )
    {
        if ( If_DsdObjType(pObj) == IF_DSD_PRIME )
            DsdMax = Abc_MaxInt( DsdMax, pObj->nFans ); 
        CountPrime += If_DsdObjType(pObj) == IF_DSD_PRIME;
        CountNonDsdStr += If_DsdManCheckNonDec_rec( p, pObj->Id );
        CountUsed += ( If_DsdVecObjRef(&p->vObjs, pObj->Id) > 0 );
        CountMarked += If_DsdVecObjMark( &p->vObjs, i );
    }
    for ( v = 3; v <= p->nVars; v++ )
    {
        CountNonDsd += Vec_MemEntryNum(p->vTtMem[v]);
        MemSizeTTs += Vec_MemEntrySize(p->vTtMem[v]) * Vec_MemEntryNum(p->vTtMem[v]);
        MemSizeDecs += (int)Vec_VecMemoryInt((Vec_Vec_t *)(p->vTtDecs[v]));
    }
    If_DsdManPrintDistrib( p );
    printf( "Number of inputs = %d.  LUT size = %d.  Marks = %s.  NewAsUseless = %s.  Bookmark = %d.\n", 
        p->nVars, p->LutSize, If_DsdManHasMarks(p)? "yes" : "no", p->fNewAsUseless? "yes" : "no", p->nObjsPrev );
    if ( p->pCellStr )
        printf( "Symbolic cell description: %s\n", p->pCellStr );
    if ( p->pTtGia )
    fprintf( pFile, "Non-DSD AIG nodes          = %8d\n", Gia_ManAndNum(p->pTtGia) );
    fprintf( pFile, "Unique table misses        = %8d\n", p->nUniqueMisses );
    fprintf( pFile, "Unique table hits          = %8d\n", p->nUniqueHits );
    fprintf( pFile, "Memory used for objects    = %8.2f MB.\n", 1.0*Mem_FlexReadMemUsage(p->pMem)/(1<<20) );
    fprintf( pFile, "Memory used for functions  = %8.2f MB.\n", 8.0*(MemSizeTTs+sizeof(int)*Vec_IntCap(&p->vTruths))/(1<<20) );
    fprintf( pFile, "Memory used for hash table = %8.2f MB.\n", 1.0*sizeof(int)*(p->nBins+Vec_IntCap(&p->vNexts))/(1<<20) );
    fprintf( pFile, "Memory used for bound sets = %8.2f MB.\n", 1.0*MemSizeDecs/(1<<20) );
    fprintf( pFile, "Memory used for array      = %8.2f MB.\n", 1.0*sizeof(void *)*Vec_PtrCap(&p->vObjs)/(1<<20) );
    if ( p->pTtGia )
    fprintf( pFile, "Memory used for AIG        = %8.2f MB.\n", 8.0*Gia_ManAndNum(p->pTtGia)/(1<<20) );
    if ( p->timeDsd )
    {
        Abc_PrintTime( 1, "Time DSD   ", p->timeDsd    );
        Abc_PrintTime( 1, "Time canon ", p->timeCanon-p->timeCheck  );
        Abc_PrintTime( 1, "Time check ", p->timeCheck  );
        Abc_PrintTime( 1, "Time check2", p->timeCheck2 );
        Abc_PrintTime( 1, "Time verify", p->timeVerify );
    }
    if ( fOccurs )
        If_DsdManPrintOccurs( stdout, p );
//    If_DsdManHashProfile( p );
    if ( fTtDump )
        If_DsdManDumpDsd( p, Support );
    if ( fTtDump )
        If_DsdManDumpAll( p, Support );
//    If_DsdManPrintDecs( stdout, p );
    if ( !fVerbose )
        return;
    vStructs = Vec_IntAlloc( 1000 );
    vCounts  = Vec_IntAlloc( 1000 );
    If_DsdVecForEachObj( &p->vObjs, pObj, i )
    {
        if ( Number && i % Number )
            continue;
        if ( Support && Support != If_DsdObjSuppSize(pObj) )
            continue;
        Vec_IntPush( vStructs, i );
        Vec_IntPush( vCounts, -(int)pObj->Count );
//        If_DsdManPrintOne( pFile, p, pObj->Id, NULL, 1 );
    }
//    fprintf( pFile, "\n" );
    pPerm = Abc_MergeSortCost( Vec_IntArray(vCounts), Vec_IntSize(vCounts) );
    for ( i = 0; i < Abc_MinInt(Vec_IntSize(vCounts), 20); i++ )
    {
        printf( "%2d : ", i+1 );
        pObj = If_DsdVecObj( &p->vObjs, Vec_IntEntry(vStructs, pPerm[i]) );
        If_DsdManPrintOne( pFile, p, pObj->Id, NULL, 1 );
    }
    ABC_FREE( pPerm );
    Vec_IntFree( vStructs );
    Vec_IntFree( vCounts );
    if ( pFileName )
        fclose( pFile );
}
 
/**Function*************************************************************

  Synopsis    [Check if the function is non-trivial.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_DsdManCheckNonTriv( If_DsdMan_t * p, int Id, int nVars, int iVarMax )
{
    If_DsdObj_t * pObj; int i, iFanin;
    pObj = If_DsdVecObj( &p->vObjs, Id );
    if ( If_DsdObjType(pObj) == IF_DSD_PRIME )
        return 1;
    if ( If_DsdObjFaninNum(pObj) == nVars )
        return 0;
    If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
        if ( Abc_Lit2Var(iFanin) == 1 && i == iVarMax )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Sorting DSD literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_DsdObjCompare( If_DsdMan_t * pMan, Vec_Ptr_t * p, int iLit0, int iLit1 )
{
    If_DsdObj_t * p0 = If_DsdVecObj(p, Abc_Lit2Var(iLit0));
    If_DsdObj_t * p1 = If_DsdVecObj(p, Abc_Lit2Var(iLit1));
    int i, Res;
    if ( If_DsdObjType(p0) < If_DsdObjType(p1) )
        return -1;
    if ( If_DsdObjType(p0) > If_DsdObjType(p1) )
        return 1;
    if ( If_DsdObjType(p0) < IF_DSD_AND )
        return 0;
    if ( If_DsdObjFaninNum(p0) < If_DsdObjFaninNum(p1) )
        return -1;
    if ( If_DsdObjFaninNum(p0) > If_DsdObjFaninNum(p1) )
        return 1;
    if ( If_DsdObjType(p0) == IF_DSD_PRIME )
    {
        if ( If_DsdObjTruthId(pMan, p0) < If_DsdObjTruthId(pMan, p1) )
            return -1;
        if ( If_DsdObjTruthId(pMan, p0) > If_DsdObjTruthId(pMan, p1) )
            return 1;
    }
    for ( i = 0; i < If_DsdObjFaninNum(p0); i++ )
    {
        Res = If_DsdObjCompare( pMan, p, If_DsdObjFaninLit(p0, i), If_DsdObjFaninLit(p1, i) );
        if ( Res != 0 )
            return Res;
    }
    if ( Abc_LitIsCompl(iLit0) > Abc_LitIsCompl(iLit1) )
        return -1;
    if ( Abc_LitIsCompl(iLit0) < Abc_LitIsCompl(iLit1) )
        return 1;
    assert( iLit0 == iLit1 );
    return 0;
}
void If_DsdObjSort( If_DsdMan_t * pMan, Vec_Ptr_t * p, int * pLits, int nLits, int * pPerm )
{
    int i, j, best_i;
    for ( i = 0; i < nLits-1; i++ )
    {
        best_i = i;
        for ( j = i+1; j < nLits; j++ )
            if ( If_DsdObjCompare(pMan, p, pLits[best_i], pLits[j]) == 1 )
                best_i = j;
        if ( i == best_i )
            continue;
        ABC_SWAP( int, pLits[i], pLits[best_i] );
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
static inline unsigned If_DsdObjHashKey( If_DsdMan_t * p, int Type, int * pLits, int nLits, int truthId )
{
    static int s_Primes[24] = { 1049, 1297, 1559, 1823, 2089, 2371, 2663, 2909, 
                                3221, 3517, 3779, 4073, 4363, 4663, 4973, 5281, 
                                5573, 5861, 6199, 6481, 6803, 7109, 7477, 7727 };
    int i;
    unsigned uHash = Type * 7873 + nLits * 8147;
    for ( i = 0; i < nLits; i++ )
        uHash += pLits[i] * s_Primes[i & 0xF];
    if ( Type == IF_DSD_PRIME )
        uHash += truthId * s_Primes[i & 0xF];
    return uHash % p->nBins;
}
unsigned * If_DsdObjHashLookup( If_DsdMan_t * p, int Type, int * pLits, int nLits, int truthId )
{
    If_DsdObj_t * pObj;
    unsigned * pSpot = p->pBins + If_DsdObjHashKey(p, Type, pLits, nLits, truthId);
    for ( ; *pSpot; pSpot = (unsigned *)Vec_IntEntryP(&p->vNexts, pObj->Id) )
    {
        pObj = If_DsdVecObj( &p->vObjs, *pSpot );
        if ( If_DsdObjType(pObj) == Type && 
             If_DsdObjFaninNum(pObj) == nLits && 
             !memcmp(pObj->pFans, pLits, sizeof(int)*If_DsdObjFaninNum(pObj)) &&
             truthId == If_DsdObjTruthId(p, pObj) )
        {
            p->nUniqueHits++;
            return pSpot;
        }
    }
    p->nUniqueMisses++;
    return pSpot;
}
static void If_DsdObjHashResize( If_DsdMan_t * p )
{
    If_DsdObj_t * pObj;
    unsigned * pSpot;
    int i, Prev = p->nUniqueMisses;
    p->nBins = Abc_PrimeCudd( 2 * p->nBins );
    p->pBins = ABC_REALLOC( unsigned, p->pBins, p->nBins );
    memset( p->pBins, 0, sizeof(unsigned) * p->nBins );
    Vec_IntFill( &p->vNexts, Vec_PtrSize(&p->vObjs), 0 );
    If_DsdVecForEachNode( &p->vObjs, pObj, i )
    {
        pSpot = If_DsdObjHashLookup( p, pObj->Type, (int *)pObj->pFans, pObj->nFans, If_DsdObjTruthId(p, pObj) );
        assert( *pSpot == 0 );
        *pSpot = pObj->Id;
    }
    assert( p->nUniqueMisses - Prev == Vec_PtrSize(&p->vObjs) - 2 );
    p->nUniqueMisses = Prev;
}

int If_DsdObjCreate( If_DsdMan_t * p, int Type, int * pLits, int nLits, int truthId )
{
    If_DsdObj_t * pObj, * pFanin;
    int i, iPrev = -1;
    // check structural canonicity
    assert( Type != DAU_DSD_MUX || nLits == 3 );
//    assert( Type != DAU_DSD_MUX || !Abc_LitIsCompl(pLits[0]) );
    assert( Type != DAU_DSD_MUX || !Abc_LitIsCompl(pLits[1]) || !Abc_LitIsCompl(pLits[2]) );
    // check that leaves are in good order
    if ( Type == DAU_DSD_AND || Type == DAU_DSD_XOR )
    {
        for ( i = 0; i < nLits; i++ )
        {
            pFanin = If_DsdVecObj( &p->vObjs, Abc_Lit2Var(pLits[i]) );
            assert( Type != DAU_DSD_AND || Abc_LitIsCompl(pLits[i]) || If_DsdObjType(pFanin) != DAU_DSD_AND );
            assert( Type != DAU_DSD_XOR || If_DsdObjType(pFanin) != DAU_DSD_XOR );
            assert( iPrev == -1 || If_DsdObjCompare(p, &p->vObjs, iPrev, pLits[i]) <= 0 );
            iPrev = pLits[i];
        }
    }
    // create new node
    pObj = If_DsdObjAlloc( p, Type, nLits );
    if ( Type == DAU_DSD_PRIME )
        If_DsdObjSetTruth( p, pObj, truthId );
    assert( pObj->nSupp == 0 );
    for ( i = 0; i < nLits; i++ )
    {
        pObj->pFans[i] = pLits[i];
        pObj->nSupp += If_DsdVecLitSuppSize(&p->vObjs, pLits[i]);
    }
    // check decomposability
    if ( p->LutSize && !If_DsdManCheckXY(p, Abc_Var2Lit(pObj->Id, 0), p->LutSize, 0, 0, 0, 0) )
        If_DsdVecObjSetMark( &p->vObjs, pObj->Id );
    return pObj->Id;
}
int If_DsdObjFindOrAdd( If_DsdMan_t * p, int Type, int * pLits, int nLits, word * pTruth )
{
    int PrevSize       = (Type == IF_DSD_PRIME) ? Vec_MemEntryNum( p->vTtMem[nLits] ) : -1;   
    int objId, truthId = (Type == IF_DSD_PRIME) ? Vec_MemHashInsert(p->vTtMem[nLits], pTruth) : -1;
    unsigned * pSpot = If_DsdObjHashLookup( p, Type, pLits, nLits, truthId );
//abctime clk;
    if ( *pSpot )
        return (int)*pSpot;
//clk = Abc_Clock();
    if ( p->LutSize && truthId >= 0 && truthId == Vec_PtrSize(p->vTtDecs[nLits]) )
    {
        Vec_Int_t * vSets = Dau_DecFindSets_int( pTruth, nLits, p->pSched );
        assert( truthId == Vec_MemEntryNum(p->vTtMem[nLits])-1 );
        Vec_PtrPush( p->vTtDecs[nLits], vSets );
//        Dau_DecPrintSets( vSets, nLits );
    }
    if ( p->vIsops[nLits] && truthId >= 0 && PrevSize != Vec_MemEntryNum(p->vTtMem[nLits]) )
    {
        Vec_Int_t * vLevel = Vec_WecPushLevel( p->vIsops[nLits] );
        int fCompl = Kit_TruthIsop( (unsigned *)pTruth, nLits, p->vCover, 1 );
        if ( fCompl >= 0 && Vec_IntSize(p->vCover) <= 8 )
        {
            Vec_IntGrow( vLevel, Vec_IntSize(p->vCover) );
            Vec_IntAppend( vLevel, p->vCover );
            if ( fCompl )
                vLevel->nCap ^= (1<<16); // hack to remember complemented attribute
        }
        assert( Vec_WecSize(p->vIsops[nLits]) == Vec_MemEntryNum(p->vTtMem[nLits]) );
    }
    if ( p->pTtGia && truthId >= 0 && truthId == Vec_MemEntryNum(p->vTtMem[nLits])-1 )
    {
//        int nObjOld = Gia_ManAndNum(p->pTtGia);
        int Lit = Kit_TruthToGia( p->pTtGia, (unsigned *)pTruth, nLits, p->vCover, NULL, 1 );
//        printf( "%d ", Gia_ManAndNum(p->pTtGia)-nObjOld );
        Gia_ManAppendCo( p->pTtGia, Lit );
    }
//p->timeCheck += Abc_Clock() - clk;
    *pSpot = Vec_PtrSize( &p->vObjs );
    objId = If_DsdObjCreate( p, Type, pLits, nLits, truthId );
    if ( Vec_PtrSize(&p->vObjs) > p->nBins )
        If_DsdObjHashResize( p );
    return objId;
}


/**Function*************************************************************

  Synopsis    [Saving/loading DSD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_DsdManSave( If_DsdMan_t * p, char * pFileName )
{
    If_DsdObj_t * pObj; 
    Vec_Int_t * vSets;
    word * pTruth; 
    int i, v, Num;
    FILE * pFile = fopen( pFileName ? pFileName : p->pStore, "wb" );
    if ( pFile == NULL )
    {
        printf( "Writing DSD manager file \"%s\" has failed.\n", pFileName ? pFileName : p->pStore );
        return;
    }
    fwrite( DSD_VERSION, 4, 1, pFile );
    Num = p->nVars;
    fwrite( &Num, 4, 1, pFile );
    Num = p->LutSize;
    fwrite( &Num, 4, 1, pFile );
    Num = Vec_PtrSize(&p->vObjs);
    fwrite( &Num, 4, 1, pFile );
    Vec_PtrForEachEntryStart( If_DsdObj_t *, &p->vObjs, pObj, i, 2 )
    {
        Num = If_DsdObjWordNum( pObj->nFans );
        fwrite( &Num, 4, 1, pFile );
        fwrite( pObj, sizeof(word)*Num, 1, pFile );
        if ( pObj->Type == IF_DSD_PRIME )
            fwrite( Vec_IntEntryP(&p->vTruths, i), 4, 1, pFile );
    }
    for ( v = 3; v <= p->nVars; v++ )
    {
        int nBytes = sizeof(word)*Vec_MemEntrySize(p->vTtMem[v]);
        Num = Vec_MemEntryNum(p->vTtMem[v]);
        fwrite( &Num, 4, 1, pFile );
        Vec_MemForEachEntry( p->vTtMem[v], pTruth, i )
            fwrite( pTruth, nBytes, 1, pFile );
        Num = Vec_PtrSize(p->vTtDecs[v]);
        fwrite( &Num, 4, 1, pFile );
        Vec_PtrForEachEntry( Vec_Int_t *, p->vTtDecs[v], vSets, i )
        {
            Num = Vec_IntSize(vSets);
            fwrite( &Num, 4, 1, pFile );
            fwrite( Vec_IntArray(vSets), sizeof(int)*Num, 1, pFile );
        }
    }
    Num = p->nConfigWords;
    fwrite( &Num, 4, 1, pFile );
    Num = p->nTtBits;
    fwrite( &Num, 4, 1, pFile );
    Num = p->vConfigs ? Vec_WrdSize(p->vConfigs) : 0;
    fwrite( &Num, 4, 1, pFile );
    if ( Num )
        fwrite( Vec_WrdArray(p->vConfigs), sizeof(word)*Num, 1, pFile );
    Num = p->pCellStr ? strlen(p->pCellStr) : 0;
    fwrite( &Num, 4, 1, pFile );
    if ( Num )
        fwrite( p->pCellStr, sizeof(char)*Num, 1, pFile );
    fclose( pFile );
}
If_DsdMan_t * If_DsdManLoad( char * pFileName )
{
    If_DsdMan_t * p;
    If_DsdObj_t * pObj; 
    Vec_Int_t * vSets;
    char pBuffer[10];
    unsigned * pSpot;
    word * pTruth;
    int i, v, Num, Num2, RetValue;
    FILE * pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Reading DSD manager file \"%s\" has failed.\n", pFileName );
        return NULL;
    }
    RetValue = fread( pBuffer, 4, 1, pFile );
    if ( strncmp(pBuffer, DSD_VERSION, strlen(DSD_VERSION)) )
    {
        printf( "Unrecognized format of file \"%s\".\n", pFileName );
        return NULL;
    }
    RetValue = fread( &Num, 4, 1, pFile );
    p = If_DsdManAlloc( Num, 0 );
    ABC_FREE( p->pStore );
    p->pStore = Abc_UtilStrsav( pFileName );
    RetValue = fread( &Num, 4, 1, pFile );
    p->LutSize = Num;
    p->pSat  = If_ManSatBuildXY( p->LutSize );
    RetValue = fread( &Num, 4, 1, pFile );
    assert( Num >= 2 );
    Vec_PtrFillExtra( &p->vObjs, Num, NULL );
    Vec_IntFill( &p->vNexts, Num, 0 );
    Vec_IntFill( &p->vTruths, Num, -1 );
    p->nBins = Abc_PrimeCudd( 2*Num );
    p->pBins = ABC_REALLOC( unsigned, p->pBins, p->nBins );
    memset( p->pBins, 0, sizeof(unsigned) * p->nBins );
    for ( i = 2; i < Vec_PtrSize(&p->vObjs); i++ )
    {
        RetValue = fread( &Num, 4, 1, pFile );
        pObj = (If_DsdObj_t *)Mem_FlexEntryFetch( p->pMem, sizeof(word) * Num );
        RetValue = fread( pObj, sizeof(word)*Num, 1, pFile );
        Vec_PtrWriteEntry( &p->vObjs, i, pObj );
        if ( pObj->Type == IF_DSD_PRIME )
        {
            RetValue = fread( &Num, 4, 1, pFile );
            Vec_IntWriteEntry( &p->vTruths, i, Num );
        }
        pSpot = If_DsdObjHashLookup( p, pObj->Type, (int *)pObj->pFans, pObj->nFans, If_DsdObjTruthId(p, pObj) );
        assert( *pSpot == 0 );
        *pSpot = pObj->Id;
    }
    assert( p->nUniqueMisses == Vec_PtrSize(&p->vObjs) - 2 );
    p->nUniqueMisses = 0;
    pTruth = ABC_ALLOC( word, p->nWords );
    for ( v = 3; v <= p->nVars; v++ )
    {
        int nBytes = sizeof(word)*Vec_MemEntrySize(p->vTtMem[v]);
        RetValue = fread( &Num, 4, 1, pFile );
        for ( i = 0; i < Num; i++ )
        {
            RetValue = fread( pTruth, nBytes, 1, pFile );
            Vec_MemHashInsert( p->vTtMem[v], pTruth );
        }
        assert( Num == Vec_MemEntryNum(p->vTtMem[v]) );
        RetValue = fread( &Num2, 4, 1, pFile );
        for ( i = 0; i < Num2; i++ )
        {
            RetValue = fread( &Num, 4, 1, pFile );
            vSets = Vec_IntAlloc( Num );
            RetValue = fread( Vec_IntArray(vSets), sizeof(int)*Num, 1, pFile );
            vSets->nSize = Num;
            Vec_PtrPush( p->vTtDecs[v], vSets );
        }
        assert( Num2 == Vec_PtrSize(p->vTtDecs[v]) ); 
    }
    ABC_FREE( pTruth );
    RetValue = fread( &Num, 4, 1, pFile );
    p->nConfigWords = Num;
    RetValue = fread( &Num, 4, 1, pFile );
    p->nTtBits = Num;
    RetValue = fread( &Num, 4, 1, pFile );
    if ( RetValue && Num )
    {
        p->vConfigs = Vec_WrdStart( Num );
        RetValue = fread( Vec_WrdArray(p->vConfigs), sizeof(word)*Num, 1, pFile );
    }
    RetValue = fread( &Num, 4, 1, pFile );
    if ( RetValue && Num )
    {
        p->pCellStr = ABC_CALLOC( char, Num + 1 );
        RetValue = fread( p->pCellStr, sizeof(char)*Num, 1, pFile );
    }
    fclose( pFile );
    return p;
}
void If_DsdManMerge( If_DsdMan_t * p, If_DsdMan_t * pNew )
{
    If_DsdObj_t * pObj; 
    Vec_Int_t * vMap;
    int pFanins[DAU_MAX_VAR];
    int i, k, iFanin, Id;
    if ( p->nVars < pNew->nVars )
    {
        printf( "The number of variables should be the same or smaller.\n" );
        return;
    }
    if ( p->LutSize != pNew->LutSize )
    {
        printf( "LUT size should be the same.\n" );
        return;
    }
    assert( p->nTtBits == pNew->nTtBits );
    assert( p->nConfigWords == pNew->nConfigWords );
    if ( If_DsdManHasMarks(p) != If_DsdManHasMarks(pNew) )
        printf( "Warning! Old manager has %smarks while new manager has %smarks.\n", 
            If_DsdManHasMarks(p) ? "" : "no ", If_DsdManHasMarks(pNew) ? "" : "no " );
    vMap = Vec_IntAlloc( Vec_PtrSize(&pNew->vObjs) );
    Vec_IntPush( vMap, 0 );
    Vec_IntPush( vMap, 1 );
    if ( p->vConfigs && pNew->vConfigs )
        Vec_WrdFillExtra( p->vConfigs, p->nConfigWords * (Vec_PtrSize(&p->vObjs) + Vec_PtrSize(&pNew->vObjs)), 0 );
    If_DsdVecForEachNode( &pNew->vObjs, pObj, i )
    {
        If_DsdObjForEachFaninLit( &pNew->vObjs, pObj, iFanin, k )
            pFanins[k] = Abc_Lit2LitV( Vec_IntArray(vMap), iFanin );
        Id = If_DsdObjFindOrAdd( p, pObj->Type, pFanins, pObj->nFans, pObj->Type == IF_DSD_PRIME ? If_DsdObjTruth(pNew, pObj) : NULL );
        if ( pObj->fMark )
            If_DsdVecObjSetMark( &p->vObjs, Id );
        if ( p->vConfigs && pNew->vConfigs && p->nConfigWords * i < Vec_WrdSize(pNew->vConfigs) )
        {
            //Vec_WrdFillExtra( p->vConfigs, Id, Vec_WrdEntry(pNew->vConfigs, i) );
            word * pConfigNew = Vec_WrdEntryP(pNew->vConfigs, p->nConfigWords * i);
            word * pConfigOld = Vec_WrdEntryP(p->vConfigs, p->nConfigWords * Id);
            memcpy( pConfigOld, pConfigNew, sizeof(word) * p->nConfigWords );
        }
        Vec_IntPush( vMap, Id );
    }
    assert( Vec_IntSize(vMap) == Vec_PtrSize(&pNew->vObjs) );
    Vec_IntFree( vMap );
    if ( p->vConfigs && pNew->vConfigs )
        Vec_WrdShrink( p->vConfigs, p->nConfigWords * Vec_PtrSize(&p->vObjs) );
}
void If_DsdManCleanOccur( If_DsdMan_t * p, int fVerbose )
{
    If_DsdObj_t * pObj; 
    int i;
    If_DsdVecForEachObj( &p->vObjs, pObj, i )
        pObj->Count = 0;
}
void If_DsdManCleanMarks( If_DsdMan_t * p, int fVerbose )
{
    If_DsdObj_t * pObj; 
    int i;
    ABC_FREE( p->pCellStr );
    Vec_WrdFreeP( &p->vConfigs );
    If_DsdVecForEachObj( &p->vObjs, pObj, i )
        pObj->fMark = 0;
}
void If_DsdManInvertMarks( If_DsdMan_t * p, int fVerbose )
{
    If_DsdObj_t * pObj; 
    int i;
    ABC_FREE( p->pCellStr );
    //Vec_WrdFreeP( &p->vConfigs );
    If_DsdVecForEachObj( &p->vObjs, pObj, i )
        pObj->fMark = !pObj->fMark;
}
void If_DsdManFilter_rec( If_DsdMan_t * pNew, If_DsdMan_t * p, int i, Vec_Int_t * vMap )
{
    If_DsdObj_t * pObj;
    int pFanins[DAU_MAX_VAR];
    int k, iFanin, Id;
    if ( Vec_IntEntry(vMap, i) >= 0 )
        return;
    // call recursively
    pObj = If_DsdVecObj( &p->vObjs, i );
    If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, k )
        If_DsdManFilter_rec( pNew, p, Abc_Lit2Var(iFanin), vMap );
    // duplicate this one
    If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, k )
        pFanins[k] = Abc_Lit2LitV( Vec_IntArray(vMap), iFanin );
    Id = If_DsdObjFindOrAdd( pNew, pObj->Type, pFanins, pObj->nFans, pObj->Type == IF_DSD_PRIME ? If_DsdObjTruth(p, pObj) : NULL );
    if ( pObj->fMark )
        If_DsdVecObjSetMark( &pNew->vObjs, Id );
    If_DsdVecObj( &pNew->vObjs, Id )->Count = pObj->Count;
    // save the result
    Vec_IntWriteEntry( vMap, i, Id );
}
If_DsdMan_t * If_DsdManFilter( If_DsdMan_t * p, int Limit )
{
    If_DsdMan_t * pNew = If_DsdManAlloc( p->nVars, p->LutSize );
    If_DsdObj_t * pObj; 
    Vec_Int_t * vMap;
    int i;
    vMap = Vec_IntStartFull( Vec_PtrSize(&p->vObjs) );
    Vec_IntWriteEntry( vMap, 0, 0 );
    Vec_IntWriteEntry( vMap, 1, 1 );
    If_DsdVecForEachNode( &p->vObjs, pObj, i )
        if ( (int)pObj->Count >= Limit )
            If_DsdManFilter_rec( pNew, p, i, vMap );
    Vec_IntFree( vMap );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Collect nodes of the tree.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_DsdManCollect_rec( If_DsdMan_t * p, int Id, Vec_Int_t * vNodes, Vec_Int_t * vFirsts, int * pnSupp )
{
    int i, iFanin, iFirst;
    If_DsdObj_t * pObj = If_DsdVecObj( &p->vObjs, Id );
    if ( If_DsdObjType(pObj) == IF_DSD_CONST0 )
        return;
    if ( If_DsdObjType(pObj) == IF_DSD_VAR )
    {
        (*pnSupp)++;
        return;
    }
    iFirst = *pnSupp;
    If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
        If_DsdManCollect_rec( p, Abc_Lit2Var(iFanin), vNodes, vFirsts, pnSupp );
    Vec_IntPush( vNodes, Id );
    Vec_IntPush( vFirsts, iFirst );
}
void If_DsdManCollect( If_DsdMan_t * p, int Id, Vec_Int_t * vNodes, Vec_Int_t * vFirsts )
{
    int nSupp = 0;
    Vec_IntClear( vNodes );
    Vec_IntClear( vFirsts );
    If_DsdManCollect_rec( p, Id, vNodes, vFirsts, &nSupp );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_DsdManComputeTruth_rec( If_DsdMan_t * p, int iDsd, word * pRes, unsigned char * pPermLits, int * pnSupp )
{
    int i, iFanin, fCompl = Abc_LitIsCompl(iDsd);
    If_DsdObj_t * pObj = If_DsdVecObj( &p->vObjs, Abc_Lit2Var(iDsd) );
    if ( If_DsdObjType(pObj) == IF_DSD_VAR )
    {
        int iPermLit = pPermLits ? (int)pPermLits[*pnSupp] : Abc_Var2Lit(*pnSupp, 0);
        (*pnSupp)++;
        assert( (*pnSupp) <= p->nVars );
        Abc_TtCopy( pRes, p->pTtElems[Abc_Lit2Var(iPermLit)], p->nWords, fCompl ^ Abc_LitIsCompl(iPermLit) );
        return;
    }
    if ( If_DsdObjType(pObj) == IF_DSD_AND || If_DsdObjType(pObj) == IF_DSD_XOR )
    {
        word pTtTemp[DAU_MAX_WORD];
        if ( If_DsdObjType(pObj) == IF_DSD_AND )
            Abc_TtConst1( pRes, p->nWords );
        else
            Abc_TtConst0( pRes, p->nWords );
        If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
        {
            If_DsdManComputeTruth_rec( p, iFanin, pTtTemp, pPermLits, pnSupp );
            if ( If_DsdObjType(pObj) == IF_DSD_AND )
                Abc_TtAnd( pRes, pRes, pTtTemp, p->nWords, 0 );
            else
                Abc_TtXor( pRes, pRes, pTtTemp, p->nWords, 0 );
        }
        if ( fCompl ) Abc_TtNot( pRes, p->nWords );
        return;
    }
    if ( If_DsdObjType(pObj) == IF_DSD_MUX ) // mux
    {
        word pTtTemp[3][DAU_MAX_WORD];
        If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
            If_DsdManComputeTruth_rec( p, iFanin, pTtTemp[i], pPermLits, pnSupp );
        assert( i == 3 );
        Abc_TtMux( pRes, pTtTemp[0], pTtTemp[1], pTtTemp[2], p->nWords );
        if ( fCompl ) Abc_TtNot( pRes, p->nWords );
        return;
    }
    if ( If_DsdObjType(pObj) == IF_DSD_PRIME ) // function
    {
        word pFanins[DAU_MAX_VAR][DAU_MAX_WORD];
        If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
            If_DsdManComputeTruth_rec( p, iFanin, pFanins[i], pPermLits, pnSupp );
        Dau_DsdTruthCompose_rec( If_DsdObjTruth(p, pObj), pFanins, pRes, pObj->nFans, p->nWords );
        if ( fCompl ) Abc_TtNot( pRes, p->nWords );
        return;
    }
    assert( 0 );

}
void If_DsdManComputeTruthPtr( If_DsdMan_t * p, int iDsd, unsigned char * pPermLits, word * pRes )
{
    int nSupp = 0;
    If_DsdObj_t * pObj = If_DsdVecObj( &p->vObjs, Abc_Lit2Var(iDsd) );
    if ( iDsd == 0 )
        Abc_TtConst0( pRes, p->nWords );
    else if ( iDsd == 1 )
        Abc_TtConst1( pRes, p->nWords );
    else if ( pObj->Type == IF_DSD_VAR )
    {
        int iPermLit = pPermLits ? (int)pPermLits[nSupp] : Abc_Var2Lit(nSupp, 0);
        nSupp++;
        Abc_TtCopy( pRes, p->pTtElems[Abc_Lit2Var(iPermLit)], p->nWords, Abc_LitIsCompl(iDsd) ^ Abc_LitIsCompl(iPermLit) );
    }
    else
        If_DsdManComputeTruth_rec( p, iDsd, pRes, pPermLits, &nSupp );
    assert( nSupp == If_DsdVecLitSuppSize(&p->vObjs, iDsd) );
}
word * If_DsdManComputeTruth( If_DsdMan_t * p, int iDsd, unsigned char * pPermLits )
{
    word * pRes = p->pTtElems[DAU_MAX_VAR];
    If_DsdManComputeTruthPtr( p, iDsd, pPermLits, pRes );
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Procedures to propagate the invertor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_DsdManCheckInv_rec( If_DsdMan_t * p, int iLit )
{
    If_DsdObj_t * pObj;
    int i, iFanin;
    pObj = If_DsdVecObj( &p->vObjs, Abc_Lit2Var(iLit) );
    if ( If_DsdObjType(pObj) == IF_DSD_VAR )
        return 1;
    if ( If_DsdObjType(pObj) == IF_DSD_AND || If_DsdObjType(pObj) == IF_DSD_PRIME )
        return 0;
    if ( If_DsdObjType(pObj) == IF_DSD_XOR )
    {
        If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
            if ( If_DsdManCheckInv_rec(p, iFanin) )
                return 1;
        return 0;
    }
    if ( If_DsdObjType(pObj) == IF_DSD_MUX )
        return If_DsdManCheckInv_rec(p, pObj->pFans[1]) && If_DsdManCheckInv_rec(p, pObj->pFans[2]);
    assert( 0 );
    return 0;
}
int If_DsdManPushInv_rec( If_DsdMan_t * p, int iLit, unsigned char * pPerm )
{
    If_DsdObj_t * pObj;
    int i, iFanin;
    pObj = If_DsdVecObj( &p->vObjs, Abc_Lit2Var(iLit) );
    if ( If_DsdObjType(pObj) == IF_DSD_VAR )
        pPerm[0] = (unsigned char)Abc_LitNot((int)pPerm[0]);
    else if ( If_DsdObjType(pObj) == IF_DSD_XOR )
    {
        If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
        {
            if ( If_DsdManCheckInv_rec(p, iFanin) )
            {
                If_DsdManPushInv_rec( p, iFanin, pPerm );
                break;
            }
            pPerm += If_DsdVecLitSuppSize(&p->vObjs, iFanin);
        }
    }
    else if ( If_DsdObjType(pObj) == IF_DSD_MUX )
    {
        assert( If_DsdManCheckInv_rec(p, pObj->pFans[1]) && If_DsdManCheckInv_rec(p, pObj->pFans[2]) );
        pPerm += If_DsdVecLitSuppSize(&p->vObjs, pObj->pFans[0]);
        If_DsdManPushInv_rec(p, pObj->pFans[1], pPerm);
        pPerm += If_DsdVecLitSuppSize(&p->vObjs, pObj->pFans[1]);
        If_DsdManPushInv_rec(p, pObj->pFans[2], pPerm);
    }
    else assert( 0 );
    return 1;
}
int If_DsdManPushInv( If_DsdMan_t * p, int iLit, unsigned char * pPerm )
{
    if ( Abc_LitIsCompl(iLit) && If_DsdManCheckInv_rec(p, iLit) )
        return If_DsdManPushInv_rec( p, iLit, pPerm );
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs DSD operation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_DsdManComputeFirstArray( If_DsdMan_t * p, int * pLits, int nLits, int * pFirsts )
{
    int i, nSSize = 0;
    for ( i = 0; i < nLits; i++ )
    {
        pFirsts[i] = nSSize;
        nSSize += If_DsdVecLitSuppSize(&p->vObjs, pLits[i]);
    }
    return nSSize;
}
int If_DsdManComputeFirst( If_DsdMan_t * p, If_DsdObj_t * pObj, int * pFirsts )
{
    return If_DsdManComputeFirstArray( p, (int *)pObj->pFans, pObj->nFans, pFirsts );
}
int If_DsdManOperation( If_DsdMan_t * p, int Type, int * pLits, int nLits, unsigned char * pPerm, word * pTruth )
{
    If_DsdObj_t * pObj, * pFanin;
    unsigned char pPermNew[DAU_MAX_VAR], * pPermStart = pPerm;
    int nChildren = 0, pChildren[DAU_MAX_VAR], pBegEnd[DAU_MAX_VAR];
    int i, k, j, Id, iFanin, fCompl = 0, nSSize = 0;
    if ( Type == IF_DSD_AND || Type == IF_DSD_XOR )
    {
        for ( k = 0; k < nLits; k++ )
        {
            if ( Type == IF_DSD_XOR && Abc_LitIsCompl(pLits[k]) )
            {
                pLits[k] = Abc_LitNot(pLits[k]);
                fCompl ^= 1;
            }
            pObj = If_DsdVecObj( &p->vObjs, Abc_Lit2Var(pLits[k]) );
            if ( Type == If_DsdObjType(pObj) && (Type == IF_DSD_XOR || !Abc_LitIsCompl(pLits[k])) )
            {
                If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
                {
                    assert( Type == IF_DSD_AND || !Abc_LitIsCompl(iFanin) );
                    pChildren[nChildren] = iFanin;
                    pBegEnd[nChildren++] = (nSSize << 16) | (nSSize + If_DsdVecLitSuppSize(&p->vObjs, iFanin));
                    nSSize += If_DsdVecLitSuppSize(&p->vObjs, iFanin);
                }
            }
            else
            {
                pChildren[nChildren] = Abc_LitNotCond( pLits[k], If_DsdManPushInv(p, pLits[k], pPermStart) );
                pBegEnd[nChildren++] = (nSSize << 16) | (nSSize + If_DsdObjSuppSize(pObj));
                nSSize += If_DsdObjSuppSize(pObj);
            }
            pPermStart += If_DsdObjSuppSize(pObj);
        }
        If_DsdObjSort( p, &p->vObjs, pChildren, nChildren, pBegEnd );
        // create permutation
        for ( j = i = 0; i < nChildren; i++ )
            for ( k = (pBegEnd[i] >> 16); k < (pBegEnd[i] & 0xFF); k++ )
                pPermNew[j++] = pPerm[k];
        assert( j == nSSize );
        for ( j = 0; j < nSSize; j++ )
            pPerm[j] = pPermNew[j];
    }
    else if ( Type == IF_DSD_MUX )
    {
        int RetValue;
        assert( nLits == 3 );
        for ( k = 0; k < nLits; k++ )
        {
            pFanin = If_DsdVecObj( &p->vObjs, Abc_Lit2Var(pLits[k]) );
            pLits[k] = Abc_LitNotCond( pLits[k], If_DsdManPushInv(p, pLits[k], pPermStart) );
            pPermStart += pFanin->nSupp;
        }
        RetValue = If_DsdObjCompare( p, &p->vObjs, pLits[1], pLits[2] );
        if ( RetValue == 1 || (RetValue == 0 && Abc_LitIsCompl(pLits[0])) )
        {
            int nSupp0 = If_DsdVecLitSuppSize( &p->vObjs, pLits[0] );
            int nSupp1 = If_DsdVecLitSuppSize( &p->vObjs, pLits[1] );
            int nSupp2 = If_DsdVecLitSuppSize( &p->vObjs, pLits[2] );
            pLits[0] = Abc_LitNot(pLits[0]);
            ABC_SWAP( int, pLits[1], pLits[2] );
            for ( j = k = 0; k < nSupp0; k++ )
                pPermNew[j++] = pPerm[k];
            for ( k = 0; k < nSupp2; k++ )
                pPermNew[j++] = pPerm[nSupp0 + nSupp1 + k];
            for ( k = 0; k < nSupp1; k++ )
                pPermNew[j++] = pPerm[nSupp0 + k];
            for ( j = 0; j < nSupp0 + nSupp1 + nSupp2; j++ )
                pPerm[j] = pPermNew[j];
        }
        if ( Abc_LitIsCompl(pLits[1]) )
        {
            pLits[1] = Abc_LitNot(pLits[1]);
            pLits[2] = Abc_LitNot(pLits[2]);
            fCompl ^= 1;
        }
        pPermStart = pPerm;
        for ( k = 0; k < nLits; k++ )
        {
            pFanin = If_DsdVecObj( &p->vObjs, Abc_Lit2Var(pLits[k]) );
            pChildren[nChildren++] = Abc_LitNotCond( pLits[k], If_DsdManPushInv(p, pLits[k], pPermStart) );
            pPermStart += pFanin->nSupp;
        }
    }
    else if ( Type == IF_DSD_PRIME )
    {
        char pCanonPerm[DAU_MAX_VAR];
        int i, uCanonPhase, pFirsts[DAU_MAX_VAR];
        uCanonPhase = Abc_TtCanonicize( pTruth, nLits, pCanonPerm );
        fCompl = ((uCanonPhase >> nLits) & 1);
        nSSize = If_DsdManComputeFirstArray( p, pLits, nLits, pFirsts );
        for ( j = i = 0; i < nLits; i++ )
        {
            int iLitNew = Abc_LitNotCond( pLits[(int)pCanonPerm[i]], ((uCanonPhase>>i)&1) );
            pFanin = If_DsdVecObj( &p->vObjs, Abc_Lit2Var(iLitNew) );
            pPermStart = pPerm + pFirsts[(int)pCanonPerm[i]];
            pChildren[nChildren++] = Abc_LitNotCond( iLitNew, If_DsdManPushInv(p, iLitNew, pPermStart) );
            for ( k = 0; k < (int)pFanin->nSupp; k++ )            
                pPermNew[j++] = pPermStart[k];
        }
        assert( j == nSSize );
        for ( j = 0; j < nSSize; j++ )
            pPerm[j] = pPermNew[j];
        Abc_TtStretch6( pTruth, nLits, p->nVars );
    }
    else assert( 0 );
    // create new graph
    Id = If_DsdObjFindOrAdd( p, Type, pChildren, nChildren, pTruth );
    return Abc_Var2Lit( Id, fCompl );
}

/**Function*************************************************************

  Synopsis    [Creating DSD network from SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void If_DsdMergeMatches( char * pDsd, int * pMatches )
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
int If_DsdManAddDsd_rec( char * pStr, char ** p, int * pMatches, If_DsdMan_t * pMan, word * pTruth, unsigned char * pPerm, int * pnSupp )
{
    unsigned char * pPermStart = pPerm + *pnSupp;
    int iRes = -1, fCompl = 0;
    if ( **p == '!' )
    {
        fCompl = 1;
        (*p)++;
    }
    if ( **p >= 'a' && **p <= 'z' ) // var
    {
        pPerm[(*pnSupp)++] = Abc_Var2Lit( **p - 'a', fCompl );
        return 2;
    }
    if ( **p == '(' || **p == '[' || **p == '<' || **p == '{' ) // and/or/xor
    {
        int Type = 0, nLits = 0, pLits[DAU_MAX_VAR];
        char * q = pStr + pMatches[ *p - pStr ];
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
            pLits[nLits++] = If_DsdManAddDsd_rec( pStr, p, pMatches, pMan, pTruth, pPerm, pnSupp );
        assert( *p == q );
        iRes = If_DsdManOperation( pMan, Type, pLits, nLits, pPermStart, pTruth );
        return Abc_LitNotCond( iRes, fCompl );
    }
    if ( (**p >= 'A' && **p <= 'F') || (**p >= '0' && **p <= '9') )
    {
        word pFunc[DAU_MAX_WORD];
        int nLits = 0, pLits[DAU_MAX_VAR];
        char * q;
        int i, nVarsF = Abc_TtReadHex( pFunc, *p );
        *p += Abc_TtHexDigitNum( nVarsF );
        q = pStr + pMatches[ *p - pStr ];
        assert( **p == '{' && *q == '}' );
        for ( i = 0, (*p)++; *p < q; (*p)++, i++ )
            pLits[nLits++] = If_DsdManAddDsd_rec( pStr, p, pMatches, pMan, pTruth, pPerm, pnSupp );
        assert( i == nVarsF );
        assert( *p == q );
        iRes = If_DsdManOperation( pMan, DAU_DSD_PRIME, pLits, nLits, pPermStart, pFunc );
        return Abc_LitNotCond( iRes, fCompl );
    }
    assert( 0 );
    return -1;
}
int If_DsdManAddDsd( If_DsdMan_t * p, char * pDsd, word * pTruth, unsigned char * pPerm, int * pnSupp )
{
    int iRes = -1, fCompl = 0;
    if ( *pDsd == '!' )
         pDsd++, fCompl = 1;
    if ( Dau_DsdIsConst0(pDsd) )
        iRes = 0;
    else if ( Dau_DsdIsConst1(pDsd) )
        iRes = 1;
    else if ( Dau_DsdIsVar(pDsd) )
    {
        pPerm[(*pnSupp)++] = Dau_DsdReadVar(pDsd);
        iRes = 2;
    }
    else
    {
        int pMatches[DAU_MAX_STR];
        If_DsdMergeMatches( pDsd, pMatches );
        iRes = If_DsdManAddDsd_rec( pDsd, &pDsd, pMatches, p, pTruth, pPerm, pnSupp );
    }
    return Abc_LitNotCond( iRes, fCompl );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if XY-decomposability holds to this LUT size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// create signature of the support of the node
unsigned If_DsdSign_rec( If_DsdMan_t * p, If_DsdObj_t * pObj, int * pnSupp )
{
    unsigned uSign = 0; int i;
    If_DsdObj_t * pFanin;
    if ( If_DsdObjType(pObj) == IF_DSD_VAR )
        return (1 << (2*(*pnSupp)++));
    If_DsdObjForEachFanin( &p->vObjs, pObj, pFanin, i )
        uSign |= If_DsdSign_rec( p, pFanin, pnSupp );
    return uSign;
}
unsigned If_DsdSign( If_DsdMan_t * p, If_DsdObj_t * pObj, int iFan, int iFirst, int fShared )
{
    If_DsdObj_t * pFanin = If_DsdObjFanin( &p->vObjs, pObj, iFan );
    unsigned uSign = If_DsdSign_rec( p, pFanin, &iFirst );
    return fShared ? (uSign << 1) | uSign : uSign;
}
// collect supports of the node
void If_DsdManGetSuppSizes( If_DsdMan_t * p, If_DsdObj_t * pObj, int * pSSizes )
{
    If_DsdObj_t * pFanin; int i;
    If_DsdObjForEachFanin( &p->vObjs, pObj, pFanin, i )
        pSSizes[i] = If_DsdObjSuppSize(pFanin);    
}
// checks if there is a way to package some fanins 
unsigned If_DsdManCheckAndXor( If_DsdMan_t * p, int iFirst, unsigned uMaskNot, If_DsdObj_t * pObj, int nSuppAll, int LutSize, int fDerive, int fVerbose )
{
    int i[6], LimitOut, SizeIn, SizeOut, pSSizes[DAU_MAX_VAR];
    int nFans = If_DsdObjFaninNum(pObj), pFirsts[DAU_MAX_VAR];
    unsigned uRes;
    assert( pObj->nFans > 2 );
    assert( If_DsdObjSuppSize(pObj) > LutSize );
    If_DsdManGetSuppSizes( p, pObj, pSSizes );
    LimitOut = LutSize - (nSuppAll - pObj->nSupp + 1);
    assert( LimitOut < LutSize );
    for ( i[0] = 0;      i[0] < nFans; i[0]++ )
    for ( i[1] = i[0]+1; i[1] < nFans; i[1]++ )
    {
        SizeIn = pSSizes[i[0]] + pSSizes[i[1]];
        SizeOut = pObj->nSupp - SizeIn;
        if ( SizeIn > LutSize || SizeOut > LimitOut )
            continue;
        if ( !fDerive )
            return ~0;
        If_DsdManComputeFirst( p, pObj, pFirsts );
        uRes = If_DsdSign(p, pObj, i[0], iFirst + pFirsts[i[0]], 0) | 
               If_DsdSign(p, pObj, i[1], iFirst + pFirsts[i[1]], 0);
        if ( uRes & uMaskNot )
            continue;
        return uRes;
    }
    if ( pObj->nFans == 3 )
        return 0;
    for ( i[0] = 0;      i[0] < nFans; i[0]++ )
    for ( i[1] = i[0]+1; i[1] < nFans; i[1]++ )
    for ( i[2] = i[1]+1; i[2] < nFans; i[2]++ )
    {
        SizeIn = pSSizes[i[0]] + pSSizes[i[1]] + pSSizes[i[2]];
        SizeOut = pObj->nSupp - SizeIn;
        if ( SizeIn > LutSize || SizeOut > LimitOut )
            continue;
        if ( !fDerive )
            return ~0;
        If_DsdManComputeFirst( p, pObj, pFirsts );
        uRes = If_DsdSign(p, pObj, i[0], iFirst + pFirsts[i[0]], 0) | 
               If_DsdSign(p, pObj, i[1], iFirst + pFirsts[i[1]], 0) | 
               If_DsdSign(p, pObj, i[2], iFirst + pFirsts[i[2]], 0);
        if ( uRes & uMaskNot )
            continue;
        return uRes;
    }
    if ( pObj->nFans == 4 )
        return 0;
    for ( i[0] = 0;      i[0] < nFans; i[0]++ )
    for ( i[1] = i[0]+1; i[1] < nFans; i[1]++ )
    for ( i[2] = i[1]+1; i[2] < nFans; i[2]++ )
    for ( i[3] = i[2]+1; i[3] < nFans; i[3]++ )
    {
        SizeIn = pSSizes[i[0]] + pSSizes[i[1]] + pSSizes[i[2]] + pSSizes[i[3]];
        SizeOut = pObj->nSupp - SizeIn;
        if ( SizeIn > LutSize || SizeOut > LimitOut )
            continue;
        if ( !fDerive )
            return ~0;
        If_DsdManComputeFirst( p, pObj, pFirsts );
        uRes = If_DsdSign(p, pObj, i[0], iFirst + pFirsts[i[0]], 0) | 
               If_DsdSign(p, pObj, i[1], iFirst + pFirsts[i[1]], 0) | 
               If_DsdSign(p, pObj, i[2], iFirst + pFirsts[i[2]], 0) | 
               If_DsdSign(p, pObj, i[3], iFirst + pFirsts[i[3]], 0);
        if ( uRes & uMaskNot )
            continue;
        return uRes;
    }
    return 0;
}
// checks if there is a way to package some fanins 
unsigned If_DsdManCheckMux( If_DsdMan_t * p, int iFirst, unsigned uMaskNot, If_DsdObj_t * pObj, int nSuppAll, int LutSize, int fDerive, int fVerbose )
{
    int LimitOut, SizeIn, SizeOut, pSSizes[DAU_MAX_VAR], pFirsts[DAU_MAX_VAR];
    unsigned uRes;
    assert( If_DsdObjFaninNum(pObj) == 3 );
    assert( If_DsdObjSuppSize(pObj) > LutSize );
    If_DsdManGetSuppSizes( p, pObj, pSSizes );
    LimitOut = LutSize - (nSuppAll - If_DsdObjSuppSize(pObj) + 1);
    assert( LimitOut < LutSize );
    // first input
    SizeIn = pSSizes[0] + pSSizes[1];
    SizeOut = pSSizes[0] + pSSizes[2] + 1;
    if ( SizeIn <= LutSize && SizeOut <= LimitOut )
    {
        if ( !fDerive )
            return ~0;
        If_DsdManComputeFirst( p, pObj, pFirsts );
        uRes = If_DsdSign(p, pObj, 0, iFirst + pFirsts[0], 1) | If_DsdSign(p, pObj, 1, iFirst + pFirsts[1], 0);
        if ( (uRes & uMaskNot) == 0 )
            return uRes;
    }
    // second input
    SizeIn = pSSizes[0] + pSSizes[2];
    SizeOut = pSSizes[0] + pSSizes[1] + 1;
    if ( SizeIn <= LutSize && SizeOut <= LimitOut )
    {
        if ( !fDerive )
            return ~0;
        If_DsdManComputeFirst( p, pObj, pFirsts );
        uRes = If_DsdSign(p, pObj, 0, iFirst + pFirsts[0], 1) | If_DsdSign(p, pObj, 2, iFirst + pFirsts[2], 0);
        if ( (uRes & uMaskNot) == 0 )
            return uRes;
    }
    return 0;
}
// checks if there is a way to package some fanins 
unsigned If_DsdManCheckPrime( If_DsdMan_t * p, int iFirst, unsigned uMaskNot, If_DsdObj_t * pObj, int nSuppAll, int LutSize, int fDerive, int fVerbose )
{
    int i, v, set, LimitOut, SizeIn, SizeOut, pSSizes[DAU_MAX_VAR], pFirsts[DAU_MAX_VAR];
    int truthId = If_DsdObjTruthId(p, pObj);
    int nFans = If_DsdObjFaninNum(pObj);
    Vec_Int_t * vSets = (Vec_Int_t *)Vec_PtrEntry(p->vTtDecs[pObj->nFans], truthId);
if ( fVerbose )
printf( "\n" );
if ( fVerbose )
Dau_DecPrintSets( vSets, nFans );
    assert( If_DsdObjFaninNum(pObj) > 2 );
    assert( If_DsdObjSuppSize(pObj) > LutSize );
    If_DsdManGetSuppSizes( p, pObj, pSSizes );
    LimitOut = LutSize - (nSuppAll - If_DsdObjSuppSize(pObj) + 1);
    assert( LimitOut < LutSize );
    Vec_IntForEachEntry( vSets, set, i )
    {
        SizeIn = SizeOut = 0;
        for ( v = 0; v < nFans; v++ )
        {
            int Value = ((set >> (v << 1)) & 3);
            if ( Value == 0 )
                SizeOut += pSSizes[v];
            else if ( Value == 1 )
                SizeIn += pSSizes[v];
            else if ( Value == 3 )
            {
                SizeIn += pSSizes[v];
                SizeOut += pSSizes[v];
            }
            else assert( 0 );
            if ( SizeIn > LutSize || SizeOut > LimitOut )
                break;
        }
        if ( v == nFans )
        {
            unsigned uRes = 0;
            if ( !fDerive )
                return ~0;
            If_DsdManComputeFirst( p, pObj, pFirsts );
            for ( v = 0; v < nFans; v++ )
            {
                int Value = ((set >> (v << 1)) & 3);
                if ( Value == 0 )
                {}
                else if ( Value == 1 )
                    uRes |= If_DsdSign(p, pObj, v, iFirst + pFirsts[v], 0);
                else if ( Value == 3 )
                    uRes |= If_DsdSign(p, pObj, v, iFirst + pFirsts[v], 1);
                else assert( 0 );
            }
            if ( uRes & uMaskNot )
                continue;
            return uRes;
        }
    }
    return 0;
}
unsigned If_DsdManCheckXY_int( If_DsdMan_t * p, int iDsd, int LutSize, int fDerive, unsigned uMaskNot, int fVerbose )
{
    If_DsdObj_t * pObj, * pTemp; 
    int i, Mask, iFirst;
    unsigned uRes;
    pObj = If_DsdVecObj( &p->vObjs, Abc_Lit2Var(iDsd) );
    if ( fVerbose )
    If_DsdManPrintOne( stdout, p, Abc_Lit2Var(iDsd), NULL, 0 );
    if ( If_DsdObjSuppSize(pObj) <= LutSize )
    {
        if ( fVerbose )
        printf( "    Trivial\n" );
        return ~0;
    }
    If_DsdManCollect( p, pObj->Id, p->vTemp1, p->vTemp2 );
    If_DsdVecForEachObjVec( p->vTemp1, &p->vObjs, pTemp, i )
        if ( If_DsdObjSuppSize(pTemp) <= LutSize && If_DsdObjSuppSize(pObj) - If_DsdObjSuppSize(pTemp) <= LutSize - 1 )
        {
            if ( fVerbose )
            printf( "    Dec using node " );
            if ( fVerbose )
            If_DsdManPrintOne( stdout, p, pTemp->Id, NULL, 1 );
            iFirst = Vec_IntEntry(p->vTemp2, i);
            uRes = If_DsdSign_rec(p, pTemp, &iFirst);
            if ( uRes & uMaskNot )
                continue;
            return uRes;
        }
    If_DsdVecForEachObjVec( p->vTemp1, &p->vObjs, pTemp, i )
        if ( (If_DsdObjType(pTemp) == IF_DSD_AND || If_DsdObjType(pTemp) == IF_DSD_XOR) && If_DsdObjFaninNum(pTemp) > 2 && If_DsdObjSuppSize(pTemp) > LutSize )
        {
            if ( (Mask = If_DsdManCheckAndXor(p, Vec_IntEntry(p->vTemp2, i), uMaskNot, pTemp, If_DsdObjSuppSize(pObj), LutSize, fDerive, fVerbose)) )
            {
                if ( fVerbose )
                printf( "    " );
                if ( fVerbose )
                Abc_TtPrintBinary( (word *)&Mask, 4 ); 
                if ( fVerbose )
                printf( "    Using multi-input AND/XOR node\n" );
                return Mask;
            }
        }
    If_DsdVecForEachObjVec( p->vTemp1, &p->vObjs, pTemp, i )
        if ( If_DsdObjType(pTemp) == IF_DSD_MUX && If_DsdObjSuppSize(pTemp) > LutSize )
        {
            if ( (Mask = If_DsdManCheckMux(p, Vec_IntEntry(p->vTemp2, i), uMaskNot, pTemp, If_DsdObjSuppSize(pObj), LutSize, fDerive, fVerbose)) )
            {
                if ( fVerbose )
                printf( "    " );
                if ( fVerbose )
                Abc_TtPrintBinary( (word *)&Mask, 4 ); 
                if ( fVerbose )
                printf( "    Using multi-input MUX node\n" );
                return Mask;
            }
        }
    If_DsdVecForEachObjVec( p->vTemp1, &p->vObjs, pTemp, i )
        if ( If_DsdObjType(pTemp) == IF_DSD_PRIME && If_DsdObjSuppSize(pTemp) > LutSize )
        {
            if ( (Mask = If_DsdManCheckPrime(p, Vec_IntEntry(p->vTemp2, i), uMaskNot, pTemp, If_DsdObjSuppSize(pObj), LutSize, fDerive, fVerbose)) )
            {
                if ( fVerbose )
                printf( "    " );
                if ( fVerbose )
                Dau_DecPrintSet( Mask, If_DsdObjFaninNum(pTemp), 0 );
                if ( fVerbose )
                printf( "    Using prime node\n" );
                return Mask;
            }
        }
    if ( fVerbose )
    printf( "    UNDEC\n" );
//    If_DsdManPrintOne( stdout, p, Abc_Lit2Var(iDsd), NULL, 1 );
    return 0;
}
unsigned If_DsdManCheckXY( If_DsdMan_t * p, int iDsd, int LutSize, int fDerive, unsigned uMaskNot, int fHighEffort, int fVerbose )
{
    unsigned uSet = If_DsdManCheckXY_int( p, iDsd, LutSize, fDerive, uMaskNot, fVerbose );
    if ( uSet == 0 && fHighEffort )
    {
//        abctime clk = Abc_Clock();
        int nVars = If_DsdVecLitSuppSize( &p->vObjs, iDsd );
        word * pRes = If_DsdManComputeTruth( p, iDsd, NULL );
        uSet = If_ManSatCheckXYall( p->pSat, LutSize, pRes, nVars, p->vTemp1 );
        if ( uSet )
        {
//            If_DsdManPrintOne( stdout, p, Abc_Lit2Var(iDsd), NULL, 1 );
//            Dau_DecPrintSet( uSet, nVars, 1 );
        }
//        p->timeCheck2 += Abc_Clock() - clk;
    }
    return uSet;
}

/**Function*************************************************************

  Synopsis    [Checks existence of decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned If_DsdManCheckXYZ( If_DsdMan_t * p, int iDsd, int LutSize, int fDerive, int fVerbose ) 
{
    return ~0;
}

/**Function*************************************************************

  Synopsis    [Add the function to the DSD manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_DsdManCompute( If_DsdMan_t * p, word * pTruth, int nLeaves, unsigned char * pPerm, char * pLutStruct )
{
    word pCopy[DAU_MAX_WORD], * pRes;
    char pDsd[DAU_MAX_STR];
    int iDsd, nSizeNonDec, nSupp = 0;
    int nWords = Abc_TtWordNum(nLeaves);
//    abctime clk = 0;
    assert( nLeaves <= DAU_MAX_VAR );
    Abc_TtCopy( pCopy, pTruth, nWords, 0 );
//clk = Abc_Clock();
    nSizeNonDec = Dau_DsdDecompose( pCopy, nLeaves, 0, 1, pDsd );
//p->timeDsd += Abc_Clock() - clk;
    if ( nSizeNonDec > 0 )
        Abc_TtStretch6( pCopy, nSizeNonDec, p->nVars );
    memset( pPerm, 0xFF, (size_t)nLeaves );
//clk = Abc_Clock();
    iDsd = If_DsdManAddDsd( p, pDsd, pCopy, pPerm, &nSupp );
//p->timeCanon += Abc_Clock() - clk;
    assert( nSupp == nLeaves );
    // verify the result
//clk = Abc_Clock();
    pRes = If_DsdManComputeTruth( p, iDsd, pPerm );
//p->timeVerify += Abc_Clock() - clk;
    if ( !Abc_TtEqual(pRes, pTruth, nWords) )
    {
//        If_DsdManPrint( p, NULL );
        printf( "\n" );
        printf( "Verification failed!\n" );
        printf( "%s\n", pDsd );
        Dau_DsdPrintFromTruth( pTruth, nLeaves );
        Dau_DsdPrintFromTruth( pRes, nLeaves );
        If_DsdManPrintOne( stdout, p, Abc_Lit2Var(iDsd), pPerm, 1 );
        printf( "\n" );
    }
    If_DsdVecObjIncRef( &p->vObjs, Abc_Lit2Var(iDsd) );
    assert( If_DsdVecLitSuppSize(&p->vObjs, iDsd) == nLeaves );
    return iDsd;
}

/**Function*************************************************************

  Synopsis    [Checks existence of decomposition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_DsdManTest()
{
    Vec_Int_t * vSets;
    word t = 0x5277;
    t = Abc_Tt6Stretch( t, 4 );
//    word t = 0xD9D900D900D900001010001000100000;
    vSets = Dau_DecFindSets( &t, 6 );
    Vec_IntFree( vSets );
}


/**Function*************************************************************

  Synopsis    [Compute pin delays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutDsdBalancePinDelays_rec( If_DsdMan_t * p, int Id, int * pTimes, word * pRes, int * pnSupp, int nSuppAll, char * pPermLits )
{
    If_DsdObj_t * pObj = If_DsdVecObj( &p->vObjs, Id );
    if ( If_DsdObjType(pObj) == IF_DSD_VAR )
    {
        int iCutVar = Abc_Lit2Var(pPermLits[(*pnSupp)++]);
        *pRes = If_CutPinDelayInit(iCutVar);
        return pTimes[iCutVar];
    }
    if ( If_DsdObjType(pObj) == IF_DSD_MUX )
    {
        word pFaninRes[3], Res0, Res1;
        int i, iFanin, Delays[3];
        If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
            Delays[i] = If_CutDsdBalancePinDelays_rec( p, Abc_Lit2Var(iFanin), pTimes, pFaninRes+i, pnSupp, nSuppAll, pPermLits );
        Res0 = If_CutPinDelayMax( pFaninRes[0], pFaninRes[1], nSuppAll, 1 );
        Res1 = If_CutPinDelayMax( pFaninRes[0], pFaninRes[2], nSuppAll, 1 );
        *pRes = If_CutPinDelayMax( Res0, Res1, nSuppAll, 1 );
        return 2 + Abc_MaxInt(Delays[0], Abc_MaxInt(Delays[1], Delays[2]));
    }
    if ( If_DsdObjType(pObj) == IF_DSD_PRIME )
    {
        word pFaninRes[IF_MAX_FUNC_LUTSIZE];
        int i, iFanin, Delays[IF_MAX_FUNC_LUTSIZE];
        Vec_Int_t * vCover = Vec_WecEntry( p->vIsops[pObj->nFans], If_DsdObjTruthId(p, pObj) );
        assert( Vec_IntSize(vCover) > 0 );
        If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
            Delays[i] = If_CutDsdBalancePinDelays_rec( p, Abc_Lit2Var(iFanin), pTimes, pFaninRes+i, pnSupp, nSuppAll, pPermLits );
        return If_CutSopBalancePinDelaysInt( vCover, Delays, pFaninRes, nSuppAll, pRes );
    }
    assert( If_DsdObjType(pObj) == IF_DSD_AND || If_DsdObjType(pObj) == IF_DSD_XOR );
    {
        word pFaninRes[IF_MAX_FUNC_LUTSIZE];
        int i, iFanin, Delay, Result = 0;
        int fXor = 0;//(If_DsdObjType(pObj) == IF_DSD_XOR);
        int nCounter = 0, pCounter[IF_MAX_FUNC_LUTSIZE];
        If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
        {
            Delay = If_CutDsdBalancePinDelays_rec( p, Abc_Lit2Var(iFanin), pTimes, pFaninRes+i, pnSupp, nSuppAll, pPermLits );
            Result = If_LogCounterPinDelays( pCounter, &nCounter, pFaninRes, Delay, pFaninRes[i], nSuppAll, fXor );
        }
        assert( nCounter > 0 );
        if ( fXor )
            Result = If_LogCounterDelayXor( pCounter, nCounter ); // estimation
        *pRes = If_LogPinDelaysMulti( pFaninRes, nCounter, nSuppAll, fXor );
        return Result;
    }
}
int If_CutDsdBalancePinDelays( If_Man_t * p, If_Cut_t * pCut, char * pPerm )
{
    if ( pCut->nLeaves == 0 ) // const
        return 0;
    if ( pCut->nLeaves == 1 ) // variable
    {
        pPerm[0] = 0;
        return (int)If_ObjCutBest(If_CutLeaf(p, pCut, 0))->Delay;
    }
    else
    {
        word Result = 0;
        int i, Delay, nSupp = 0, pTimes[IF_MAX_FUNC_LUTSIZE];
        for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
            pTimes[i] = (int)If_ObjCutBest(If_CutLeaf(p, pCut, i))->Delay; 
        Delay = If_CutDsdBalancePinDelays_rec( p->pIfDsdMan, Abc_Lit2Var(If_CutDsdLit(p, pCut)), pTimes, &Result, &nSupp, If_CutLeaveNum(pCut), If_CutDsdPerm(p, pCut) );
        assert( nSupp == If_CutLeaveNum(pCut) );
        If_CutPinDelayTranslate( Result, If_CutLeaveNum(pCut), pPerm );
        return Delay;
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutDsdPermLitMax( char * pPermLits, int nVars, int iVar )
{
    int i;
    assert( iVar >= 0 && iVar < nVars );
    for ( i = 0; i < nVars; i++ )
        if ( iVar == Abc_Lit2Var((int)pPermLits[i]) )
            return i;
    assert( 0 );
    return -1;
}

/**Function*************************************************************

  Synopsis    [Evaluate delay using DSD balancing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_CutDsdBalanceEval_rec( If_DsdMan_t * p, int Id, int * pTimes, int * pnSupp, Vec_Int_t * vAig, int * piLit, int nSuppAll, int * pArea, char * pPermLits )
{
    If_DsdObj_t * pObj = If_DsdVecObj( &p->vObjs, Id );
    if ( If_DsdObjType(pObj) == IF_DSD_VAR )
    {
        int iCutVar = Abc_Lit2Var( pPermLits[*pnSupp] );
        if ( vAig ) 
            *piLit = Abc_Var2Lit( iCutVar, Abc_LitIsCompl(pPermLits[*pnSupp]) );
        (*pnSupp)++;
        return pTimes[iCutVar];
    }
    if ( If_DsdObjType(pObj) == IF_DSD_MUX )
    {
        int i, iFanin, Delays[3], pFaninLits[3];
        If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
        {
            Delays[i] = If_CutDsdBalanceEval_rec( p, Abc_Lit2Var(iFanin), pTimes, pnSupp, vAig, pFaninLits+i, nSuppAll, pArea, pPermLits );
            if ( Delays[i] == -1 )
                return -1;
            if ( vAig ) 
                pFaninLits[i] = Abc_LitNotCond( pFaninLits[i], Abc_LitIsCompl(iFanin) );
        }
        if ( vAig )
            *piLit = If_LogCreateMux( vAig, pFaninLits[0], pFaninLits[1], pFaninLits[2], nSuppAll );
        else
            *pArea += 3;
        return 2 + Abc_MaxInt(Delays[0], Abc_MaxInt(Delays[1], Delays[2]));
    }
    if ( If_DsdObjType(pObj) == IF_DSD_PRIME )
    {
        int i, iFanin, Delays[IF_MAX_FUNC_LUTSIZE], pFaninLits[IF_MAX_FUNC_LUTSIZE];
        Vec_Int_t * vCover = Vec_WecEntry( p->vIsops[pObj->nFans], If_DsdObjTruthId(p, pObj) );
        if ( Vec_IntSize(vCover) == 0 )
            return -1;
        If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
        {
            Delays[i] = If_CutDsdBalanceEval_rec( p, Abc_Lit2Var(iFanin), pTimes, pnSupp, vAig, pFaninLits+i, nSuppAll, pArea, pPermLits );
            if ( Delays[i] == -1 )
                return -1;
            if ( vAig ) 
                pFaninLits[i] = Abc_LitNotCond( pFaninLits[i], Abc_LitIsCompl(iFanin) );
        }
        return If_CutSopBalanceEvalInt( vCover, Delays, pFaninLits, vAig, piLit, nSuppAll, pArea );
    }
    assert( If_DsdObjType(pObj) == IF_DSD_AND || If_DsdObjType(pObj) == IF_DSD_XOR );
    {
        int i, iFanin, Delay, Result = 0;
        int fXor = 0;//(If_DsdObjType(pObj) == IF_DSD_XOR);
        int fXorFunc = (If_DsdObjType(pObj) == IF_DSD_XOR);
        int nCounter = 0, pCounter[IF_MAX_FUNC_LUTSIZE], pFaninLits[IF_MAX_FUNC_LUTSIZE];
        If_DsdObjForEachFaninLit( &p->vObjs, pObj, iFanin, i )
        {
            Delay = If_CutDsdBalanceEval_rec( p, Abc_Lit2Var(iFanin), pTimes, pnSupp, vAig, pFaninLits+i, nSuppAll, pArea, pPermLits );
            if ( Delay == -1 )
                return -1;
            if ( vAig ) 
                pFaninLits[i] = Abc_LitNotCond( pFaninLits[i], Abc_LitIsCompl(iFanin) );
            Result = If_LogCounterAddAig( pCounter, &nCounter, pFaninLits, Delay, vAig ? pFaninLits[i] : -1, vAig, nSuppAll, fXor, fXorFunc );
        }
        assert( nCounter > 0 );
        if ( fXor )
            Result = If_LogCounterDelayXor( pCounter, nCounter ); // estimation
        if ( vAig )
            *piLit = If_LogCreateAndXorMulti( vAig, pFaninLits, nCounter, nSuppAll, fXorFunc );
        else
            *pArea += (pObj->nFans - 1) * (1 + 2 * fXor);
        return Result;
    }
}
int If_CutDsdBalanceEvalInt( If_DsdMan_t * p, int iDsd, int * pTimes, Vec_Int_t * vAig, int * pArea, char * pPermLits )
{
    int nSupp = 0, iLit = 0;
    int nSuppAll = If_DsdVecLitSuppSize( &p->vObjs, iDsd );
    int Res = If_CutDsdBalanceEval_rec( p, Abc_Lit2Var(iDsd), pTimes, &nSupp, vAig, &iLit, nSuppAll, pArea, pPermLits );
    if ( Res == -1 )
        return -1;
    assert( nSupp == nSuppAll );
    assert( vAig == NULL || Abc_Lit2Var(iLit) == nSupp + Abc_Lit2Var(Vec_IntSize(vAig)) - 1 );
    if ( vAig )
        Vec_IntPush( vAig, Abc_LitIsCompl(iLit) ^ Abc_LitIsCompl(iDsd) );
    assert( vAig == NULL || (Vec_IntSize(vAig) & 1) );
    return Res;
}
int If_CutDsdBalanceEval( If_Man_t * p, If_Cut_t * pCut, Vec_Int_t * vAig )
{
    int fUseCofs = 0;
    pCut->fUser = 1;
    if ( vAig )
        Vec_IntClear( vAig );
    if ( pCut->nLeaves == 0 ) // const
    {
        assert( Abc_Lit2Var(If_CutDsdLit(p, pCut)) == 0 );
        if ( vAig )
            Vec_IntPush( vAig, Abc_LitIsCompl(If_CutDsdLit(p, pCut)) );
        pCut->Cost = 0;
        return 0;
    }
    if ( pCut->nLeaves == 1 ) // variable
    {
        assert( Abc_Lit2Var(If_CutDsdLit(p, pCut)) == 1 );
        if ( vAig )
            Vec_IntPush( vAig, 0 );
        if ( vAig )
            Vec_IntPush( vAig, Abc_LitIsCompl(If_CutDsdLit(p, pCut)) );
        pCut->Cost = 0;
        return (int)If_ObjCutBest(If_CutLeaf(p, pCut, 0))->Delay;
    }
    else
    {
        int fVerbose = 0;
        int i, pTimes[IF_MAX_FUNC_LUTSIZE];
        int Delay, Area = 0;
        char * pPermLits = If_CutDsdPerm(p, pCut);
        for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
            pTimes[i] = (int)If_ObjCutBest(If_CutLeaf(p, pCut, i))->Delay; 
        Delay = If_CutDsdBalanceEvalInt( p->pIfDsdMan, Abc_LitNotCond(If_CutDsdLit(p, pCut), pCut->fCompl), pTimes, vAig, &Area, If_CutDsdPerm(p, pCut) );
        pCut->Cost = Area;
        // try cofactoring 
        if ( fUseCofs )
        {
            // count how many times the max one appears
            int iMax = 0, nCountMax = 1;
            for ( i = 1; i < If_CutLeaveNum(pCut); i++ )
                if ( pTimes[i] > pTimes[iMax] )
                    iMax = i, nCountMax = 1;
                else if ( pTimes[i] == pTimes[iMax] )
                    nCountMax++;
            // decide when to try the decomposition
            if ( nCountMax == 1 && pTimes[iMax] + 2 < Delay && If_DsdManCheckNonTriv( p->pIfDsdMan, Abc_Lit2Var(If_CutDsdLit(p, pCut)), 
                 If_CutLeaveNum(pCut), If_CutDsdPermLitMax(pPermLits, If_CutLeaveNum(pCut), iMax)) )
            {
//                fVerbose = 1;
                Delay = pTimes[iMax] + 2;
            }
        }
        // report the result
        if ( fVerbose )
        {
/*
            int Max = 0, Two = 0;
            for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
                Max = Abc_MaxInt( Max, pTimes[i] );
            for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
                if ( pTimes[i] != Max )
                    Two = Abc_MaxInt( Two, pTimes[i] );
            if ( Two + 2 < Max && Max + 3 < Delay )
*/
            {
                for ( i = 0; i < If_CutLeaveNum(pCut); i++ )
                    printf( "%3d ", pTimes[Abc_Lit2Var(pPermLits[i])] );
                for ( ; i < p->pPars->nLutSize; i++ )
                    printf( "    " );
                printf( "-> %3d   ", Delay );
                If_DsdManPrintOne( stdout, p->pIfDsdMan, Abc_Lit2Var(If_CutDsdLit(p, pCut)), NULL, 0 );
                printf( "\n" );
            }
        }
        return Delay;
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_DsdManTune( If_DsdMan_t * p, int LutSize, int fFast, int fAdd, int fSpec, int fVerbose )
{
    ProgressBar * pProgress = NULL;
    sat_solver * pSat = NULL;
    If_DsdObj_t * pObj;
    Vec_Int_t * vLits;
    int i, Value, nVars;
    word * pTruth;
    if ( !fAdd || !LutSize )
        If_DsdVecForEachObj( &p->vObjs, pObj, i )
            pObj->fMark = 0;
    if ( LutSize == 0 )
        return;
    vLits = Vec_IntAlloc( 1000 );
    pSat = (sat_solver *)If_ManSatBuildXY( LutSize );
    pProgress = Extra_ProgressBarStart( stdout, Vec_PtrSize(&p->vObjs) );
    If_DsdVecForEachObj( &p->vObjs, pObj, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        nVars = If_DsdObjSuppSize(pObj);
        if ( nVars <= LutSize )
            continue;
        if ( fAdd && !pObj->fMark )
            continue;
        pObj->fMark = 0;
        if ( If_DsdManCheckXY(p, Abc_Var2Lit(i, 0), LutSize, 0, 0, 0, 0) )
            continue;
        if ( fFast )
            Value = 0;
        else
        {
            pTruth = If_DsdManComputeTruth( p, Abc_Var2Lit(i, 0), NULL );
            Value = If_ManSatCheckXYall( pSat, LutSize, pTruth, nVars, vLits );
        }
        if ( Value )
            continue;
        If_DsdVecObjSetMark( &p->vObjs, i );
    }
    Extra_ProgressBarStop( pProgress );
    If_ManSatUnbuild( pSat );
    Vec_IntFree( vLits );
    if ( fVerbose )
        If_DsdManPrintDistrib( p );
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Id_DsdManTuneStr1( If_DsdMan_t * p, char * pStruct, int nConfls, int fVerbose )
{
    int fVeryVerbose = 0;
    ProgressBar * pProgress = NULL;
    If_DsdObj_t * pObj;
    word * pTruth, * pConfig;
    int i, nVars, Value, LutSize;
    abctime clk = Abc_Clock();
    // parse the structure
    Ifn_Ntk_t * pNtk = Ifn_NtkParse( pStruct );
    if ( pNtk == NULL )
        return;
    if ( If_DsdManVarNum(p) > Ifn_NtkInputNum(pNtk) )
    {
        printf( "The support of DSD manager (%d) exceeds the support of the structure (%d).\n", If_DsdManVarNum(p), Ifn_NtkInputNum(pNtk) );
        ABC_FREE( pNtk );
        return;
    }
    ABC_FREE( p->pCellStr );
    p->pCellStr = Abc_UtilStrsav( pStruct );
    if ( If_DsdManVarNum(p) < Ifn_NtkInputNum(pNtk) )
        printf( "Warning: The support of DSD manager (%d) is less than the support of the structure (%d).\n", If_DsdManVarNum(p), Ifn_NtkInputNum(pNtk) );
    LutSize = Ifn_NtkLutSizeMax(pNtk);
    p->nTtBits = Ifn_NtkTtBits( pStruct );
    p->nConfigWords = 1 + Abc_Bit6WordNum( p->nTtBits );
    // print
    if ( fVerbose )
    {
        printf( "Considering programmable cell: " );
        Ifn_NtkPrint( pNtk );
        printf( "Largest LUT size = %d.\n", LutSize );
    }
    if ( p->nObjsPrev > 0 )
        printf( "Starting the tuning process from object %d (out of %d).\n", p->nObjsPrev, Vec_PtrSize(&p->vObjs) );
    // clean the attributes
    If_DsdVecForEachObj( &p->vObjs, pObj, i )
        if ( i >= p->nObjsPrev )
            pObj->fMark = 0;
    if ( p->vConfigs == NULL )
        p->vConfigs = Vec_WrdStart( p->nConfigWords * Vec_PtrSize(&p->vObjs) );
    else
        Vec_WrdFillExtra( p->vConfigs, p->nConfigWords * Vec_PtrSize(&p->vObjs), 0 );
    pProgress = Extra_ProgressBarStart( stdout, Vec_PtrSize(&p->vObjs) );
    If_DsdVecForEachObjStart( &p->vObjs, pObj, i, p->nObjsPrev )
    {
        if ( (i & 0xFF) == 0 )
            Extra_ProgressBarUpdate( pProgress, i, NULL );
        nVars = If_DsdObjSuppSize(pObj);
        //if ( nVars <= LutSize )
        //    continue;
        pTruth = If_DsdManComputeTruth( p, Abc_Var2Lit(i, 0), NULL );
        if ( fVeryVerbose )
            Dau_DsdPrintFromTruth( pTruth, nVars );
        if ( fVerbose )
            printf( "%6d : %2d ", i, nVars );
        pConfig = Vec_WrdEntryP( p->vConfigs, p->nConfigWords * i );
        Value = Ifn_NtkMatch( pNtk, pTruth, nVars, nConfls, fVerbose, fVeryVerbose, pConfig );
        if ( fVeryVerbose )
            printf( "\n" );
        if ( Value == 0 )
        {
            If_DsdVecObjSetMark( &p->vObjs, i );
            memset( pConfig, 0, sizeof(word) * p->nConfigWords );
        }
    }
    p->nObjsPrev = 0;
    p->LutSize = 0;
    Extra_ProgressBarStop( pProgress );
    printf( "Finished matching %d functions. ", Vec_PtrSize(&p->vObjs) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( fVeryVerbose )
        If_DsdManPrintDistrib( p );
    ABC_FREE( pNtk );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#ifndef ABC_USE_PTHREADS
void Id_DsdManTuneStr( If_DsdMan_t * p, char * pStruct, int nConfls, int nProcs, int nInputs, int fVerbose )
{
    Id_DsdManTuneStr1( p, pStruct, nConfls, fVerbose );
}
#else // pthreads are used


#define PAR_THR_MAX 100
typedef struct Ifn_ThData_t_
{
    Ifn_Ntk_t * pNtk;    // network
    word        pTruth[DAU_MAX_WORD];
    word        pConfig[10]; // configuration data
    int         nConfigWords;// configuration data word count
    int         nVars;   // support
    int         Id;      // object
    int         nConfls; // conflicts
    int         Result;  // result
    int         Status;  // state
    abctime     clkUsed; // total runtime
} Ifn_ThData_t;
void * Ifn_WorkerThread( void * pArg )
{
    Ifn_ThData_t * pThData = (Ifn_ThData_t *)pArg;
    volatile int * pPlace = &pThData->Status;
    abctime clk;
    while ( 1 )
    {
        while ( *pPlace == 0 );
        assert( pThData->Status == 1 );
        if ( pThData->Id == -1 )
        {
            pthread_exit( NULL );
            assert( 0 );
            return NULL;
        }
        clk = Abc_Clock();
        memset( pThData->pConfig, 0, sizeof(word) * pThData->nConfigWords );
        pThData->Result = Ifn_NtkMatch( pThData->pNtk, pThData->pTruth, pThData->nVars, pThData->nConfls, 0, 0, pThData->pConfig );
        pThData->clkUsed += Abc_Clock() - clk;
        pThData->Status = 0;
//        printf( "Finished object %d\n", pThData->Id );
    }
    assert( 0 );
    return NULL;
}
void Id_DsdManTuneStr( If_DsdMan_t * p, char * pStruct, int nConfls, int nProcs, int nInputs, int fVerbose )
{
    int fVeryVerbose = 0;
    ProgressBar * pProgress = NULL;
    int i, k, nVars, LutSize;
    abctime clk = Abc_Clock();
    Ifn_Ntk_t * pNtk;
    If_DsdObj_t * pObj;
    if ( nProcs == 1 )
    {
        Id_DsdManTuneStr1( p, pStruct, nConfls, fVerbose );
        return;
    }
    if ( nProcs > PAR_THR_MAX )
    {
        printf( "The number of processes (%d) exceeds the precompiled limit (%d).\n", nProcs, PAR_THR_MAX );
        return;
    }
    // parse the structure
    pNtk = Ifn_NtkParse( pStruct );
    if ( pNtk == NULL )
        return;
    if ( If_DsdManVarNum(p) > Ifn_NtkInputNum(pNtk) )
    {
        printf( "The support of DSD manager (%d) exceeds the support of the structure (%d).\n", If_DsdManVarNum(p), Ifn_NtkInputNum(pNtk) );
        ABC_FREE( pNtk );
        return;
    }
    ABC_FREE( p->pCellStr );
    p->pCellStr = Abc_UtilStrsav( pStruct );
    if ( If_DsdManVarNum(p) < Ifn_NtkInputNum(pNtk) )
        printf( "Warning: The support of DSD manager (%d) is less than the support of the structure (%d).\n", If_DsdManVarNum(p), Ifn_NtkInputNum(pNtk) );
    // check the largest LUT
    LutSize = Ifn_NtkLutSizeMax(pNtk);
    p->nTtBits = Ifn_NtkTtBits( pStruct );
    p->nConfigWords = 1 + Abc_Bit6WordNum( p->nTtBits );
    assert( p->nConfigWords <= 10 );
    if ( fVerbose )
    {
        printf( "Considering programmable cell: " );
        Ifn_NtkPrint( pNtk );
        printf( "Largest LUT size = %d.\n", LutSize );
    }
    ABC_FREE( pNtk );
    if ( p->nObjsPrev > 0 )
        printf( "Starting the tuning process from object %d (out of %d).\n", p->nObjsPrev, Vec_PtrSize(&p->vObjs) );
    // clean the attributes
    If_DsdVecForEachObj( &p->vObjs, pObj, i )
        if ( i >= p->nObjsPrev )
            pObj->fMark = 0;
    if ( p->vConfigs == NULL )
        p->vConfigs = Vec_WrdStart( p->nConfigWords * Vec_PtrSize(&p->vObjs) );
    else
        Vec_WrdFillExtra( p->vConfigs, p->nConfigWords * Vec_PtrSize(&p->vObjs), 0 );
    pProgress = Extra_ProgressBarStart( stdout, Vec_PtrSize(&p->vObjs) );

    // perform concurrent solving
    {
        pthread_t WorkerThread[PAR_THR_MAX];
        Ifn_ThData_t ThData[PAR_THR_MAX];
        abctime clk, clkUsed = 0;
        int status, fRunning = 1, iCurrentObj = p->nObjsPrev;
        // start the threads
        for ( i = 0; i < nProcs; i++ )
        {
            ThData[i].pNtk     = Ifn_NtkParse( pStruct );
            ThData[i].nVars    = -1;      // support
            ThData[i].Id       = -1;      // object
            ThData[i].nConfls  = nConfls; // conflicts
            ThData[i].Result   = -1;      // result
            ThData[i].Status   =  0;      // state
            ThData[i].clkUsed  =  0;      // total runtime
            ThData[i].nConfigWords = p->nConfigWords;
            status = pthread_create( WorkerThread + i, NULL, Ifn_WorkerThread, (void *)(ThData + i) );  assert( status == 0 );
        }
        // run the threads
        while ( fRunning || iCurrentObj < Vec_PtrSize(&p->vObjs) )
        {
            for ( i = 0; i < nProcs; i++ )
            {
                if ( ThData[i].Status )
                    continue;
                assert( ThData[i].Status == 0 );
                if ( ThData[i].Id >= 0 )
                {
                    //printf( "Closing obj %d with Thread %d:\n", ThData[i].Id, i );
                    assert( ThData[i].Result == 0 || ThData[i].Result == 1 );
                    if ( ThData[i].Result == 0 )
                        If_DsdVecObjSetMark( &p->vObjs, ThData[i].Id );
                    else
                    {
                        word * pTtWords = Vec_WrdEntryP( p->vConfigs, p->nConfigWords * ThData[i].Id );
                        memcpy( pTtWords, ThData[i].pConfig, sizeof(word) * p->nConfigWords );
                    }
                    ThData[i].Id     = -1;
                    ThData[i].Result = -1;
                }
                for ( k = iCurrentObj; k < Vec_PtrSize(&p->vObjs); k++ )
                {
                    if ( (k & 0xFF) == 0 )
                        Extra_ProgressBarUpdate( pProgress, k, NULL );
                    pObj  = If_DsdVecObj( &p->vObjs, k );
                    nVars = If_DsdObjSuppSize(pObj);
                    if ( nInputs && nVars < nInputs )
                        continue;
                    clk = Abc_Clock();
                    If_DsdManComputeTruthPtr( p, Abc_Var2Lit(k, 0), NULL, ThData[i].pTruth );
                    clkUsed += Abc_Clock() - clk;
                    ThData[i].nVars  = nVars;
                    ThData[i].Id     =  k;
                    ThData[i].Result = -1;
                    ThData[i].Status =  1;
                    //printf( "Scheduling %d for Thread %d\n", ThData[i].Id, i );
                    iCurrentObj = k+1;
                    break;
                }
            }
            fRunning = 0;
            for ( i = 0; i < nProcs; i++ )
                if ( ThData[i].Status == 1 || (ThData[i].Status == 0 && ThData[i].Id >= 0) )
                    fRunning = 1;
            //printf( "fRunning %d\n", fRunning );
        }
        // stop the threads
        for ( i = 0; i < nProcs; i++ )
        {
            assert( ThData[i].Status == 0 );
            ThData[i].Id = -1;
            ThData[i].Status = 1;
            ABC_FREE( ThData[i].pNtk );
        }
        if ( fVerbose )
        {
            printf( "Main     : " );
            Abc_PrintTime( 1, "Time", clkUsed );
            for ( i = 0; i < nProcs; i++ )
            {
                printf( "Thread %d : ", i );
                Abc_PrintTime( 1, "Time", ThData[i].clkUsed );
            }
        }
    }

    p->nObjsPrev = 0;
    p->LutSize = 0;
    Extra_ProgressBarStop( pProgress );
    printf( "Finished matching %d functions. ", Vec_PtrSize(&p->vObjs) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( fVeryVerbose )
        If_DsdManPrintDistrib( p );
}

#endif // pthreads are used

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
#ifdef ABC_USE_CUDD

void Id_DsdManTuneThresh( If_DsdMan_t * p, int fUnate, int fThresh, int fThreshHeuristic, int fVerbose )
{
    extern int Extra_ThreshCheck( word * t, int nVars, int * pW );
    extern int Extra_ThreshHeuristic( word * t, int nVars, int * pW );
    int fVeryVerbose = 0;
    int pW[16];
    ProgressBar * pProgress = NULL;
    If_DsdObj_t * pObj;
    word * pTruth, Perm;
    int i, nVars, Value;
    abctime clk = Abc_Clock();
    assert( fUnate + fThresh + fThreshHeuristic <= 1 );
    if ( p->nObjsPrev > 0 )
        printf( "Starting the tuning process from object %d (out of %d).\n", p->nObjsPrev, Vec_PtrSize(&p->vObjs) );
    // clean the attributes
    If_DsdVecForEachObj( &p->vObjs, pObj, i )
        if ( i >= p->nObjsPrev )
            pObj->fMark = 0;
    if ( p->vConfigs == NULL )
        p->vConfigs = Vec_WrdStart( Vec_PtrSize(&p->vObjs) );
    else
        Vec_WrdFillExtra( p->vConfigs, Vec_PtrSize(&p->vObjs), 0 );
    pProgress = Extra_ProgressBarStart( stdout, Vec_PtrSize(&p->vObjs) );
    If_DsdVecForEachObjStart( &p->vObjs, pObj, i, p->nObjsPrev )
    {
        if ( (i & 0xFF) == 0 )
            Extra_ProgressBarUpdate( pProgress, i, NULL );
        nVars = If_DsdObjSuppSize(pObj);
        if ( nVars > 8 )
            continue;
        pTruth = If_DsdManComputeTruth( p, Abc_Var2Lit(i, 0), NULL );
        if ( fVeryVerbose )
            Dau_DsdPrintFromTruth( pTruth, nVars );
        if ( fVerbose )
            printf( "%6d : %2d ", i, nVars );
        if ( fUnate )
            Value = Abc_TtIsUnate( pTruth, nVars );
        else if ( fThresh )
            Value = Extra_ThreshCheck( pTruth, nVars, pW );
        else if ( fThreshHeuristic )
            Value = Extra_ThreshHeuristic( pTruth, nVars, pW );
        else
            Value = 0;
        Perm = 0;
        if ( fVeryVerbose )
            printf( "\n" );
        if ( Value )
            If_DsdVecObjSetMark( &p->vObjs, i );
        else
            Vec_WrdWriteEntry( p->vConfigs, i, Perm );
    }
    p->nObjsPrev = 0;
    p->LutSize = 0;
    Extra_ProgressBarStop( pProgress );
    printf( "Finished matching %d functions. ", Vec_PtrSize(&p->vObjs) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    if ( fVeryVerbose )
        If_DsdManPrintDistrib( p );
}

#endif // ABC_USE_CUDD are used

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

