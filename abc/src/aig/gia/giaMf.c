/**CFile****************************************************************

  FileName    [giaMf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [Cut computation.]

  Author      [Alan Mishchenko]`
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: giaMf.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "misc/vec/vecMem.h"
#include "misc/util/utilTruth.h"
#include "misc/extra/extra.h"
#include "sat/cnf/cnf.h"
#include "opt/dau/dau.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define MF_LEAF_MAX   10
#define MF_CUT_MAX    16
#define MF_LOG_PAGE   12
#define MF_NO_LEAF    31
#define MF_TT_WORDS  ((MF_LEAF_MAX > 6) ? 1 << (MF_LEAF_MAX-6) : 1)
#define MF_NO_FUNC    134217727     // (1<<27)-1
#define MF_EPSILON 0.005

typedef struct Mf_Cut_t_ Mf_Cut_t; 
struct Mf_Cut_t_
{
    word            Sign;           // signature
    int             Delay;          // delay
    float           Flow;           // flow
    unsigned        iFunc   : 27;   // function (MF_NO_FUNC)
    unsigned        nLeaves :  5;   // leaf number (MF_NO_LEAF)
    int             pLeaves[MF_LEAF_MAX+1]; // leaves
};
typedef struct Mf_Obj_t_ Mf_Obj_t; 
struct Mf_Obj_t_
{
    int             iCutSet;        // cutset
    float           Flow;           // area
    float           nFlowRefs;      // flow references
    unsigned        Delay    : 16;  // delay 
    unsigned        nMapRefs : 16;  // map references
};
typedef struct Mf_Man_t_ Mf_Man_t; 
struct Mf_Man_t_
{
    // user data
    Gia_Man_t *     pGia0;          // original manager
    Gia_Man_t *     pGia;           // derived manager
    Jf_Par_t *      pPars;          // parameters
    // cut data
    Mf_Obj_t *      pLfObjs;        // best cuts
    Vec_Ptr_t       vPages;         // cut memory
    Vec_Mem_t *     vTtMem;         // truth tables
    Vec_Int_t       vCnfSizes;      // handles to CNF
    Vec_Int_t       vCnfMem;        // memory for CNF
    Vec_Int_t       vTemp;          // temporary array
    int             iCur;           // current position
    int             Iter;           // mapping iterations
    int             fUseEla;        // use exact area
    // statistics
    abctime         clkStart;       // starting time
    double          CutCount[4];    // cut counts
    int             nCutCounts[MF_LEAF_MAX+1];
};

static inline Mf_Obj_t * Mf_ManObj( Mf_Man_t * p, int i )            { return p->pLfObjs + i;                                          }
static inline int *      Mf_ManCutSet( Mf_Man_t * p, int i )         { return (int *)Vec_PtrEntry(&p->vPages, i >> 16) + (i & 0xFFFF); }
static inline int *      Mf_ObjCutSet( Mf_Man_t * p, int i )         { return Mf_ManCutSet(p, Mf_ManObj(p, i)->iCutSet);               }
static inline int *      Mf_ObjCutBest( Mf_Man_t * p, int i )        { return Mf_ObjCutSet(p, i) + 1;                                  }

static inline int        Mf_ObjMapRefNum( Mf_Man_t * p, int i )      { return Mf_ManObj(p, i)->nMapRefs;                               }
static inline int        Mf_ObjMapRefInc( Mf_Man_t * p, int i )      { return Mf_ManObj(p, i)->nMapRefs++;                             }
static inline int        Mf_ObjMapRefDec( Mf_Man_t * p, int i )      { return --Mf_ManObj(p, i)->nMapRefs;                             }

static inline int        Mf_CutSize( int * pCut )                    { return pCut[0] & MF_NO_LEAF;                                    }
static inline int        Mf_CutFunc( int * pCut )                    { return ((unsigned)pCut[0] >> 5);                                }
static inline int        Mf_CutSetBoth( int n, int f )               { return n | (f << 5);                                            }
static inline int        Mf_CutIsTriv( int * pCut, int i )           { return Mf_CutSize(pCut) == 1 && pCut[1] == i;                   } 

#define Mf_SetForEachCut( pList, pCut, i )      for ( i = 0, pCut = pList + 1; i < pList[0]; i++, pCut += Mf_CutSize(pCut) + 1 )
#define Mf_ObjForEachCut( pCuts, i, nCuts )     for ( i = 0, i < nCuts; i++ )

extern int Kit_TruthToGia( Gia_Man_t * pMan, unsigned * pTruth, int nVars, Vec_Int_t * vMemory, Vec_Int_t * vLeaves, int fHash );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computing truth tables of useful DSD classes of 6-functions.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int s_nCalls = 0;
static Vec_Mem_t * s_vTtMem = NULL;
int Mf_ManTruthCanonicize( word * t, int nVars )
{
    word Temp, Best = *t;
    int r, i, Config = 0;
    for ( r = 0; r < 1; r++ )
    {
        if ( Best > (Temp = ~Best) )
            Best = Temp, Config ^= (1 << nVars);
        for ( i = 0; i < nVars; i++ )
            if ( Best > (Temp = Abc_Tt6Flip(Best, i)) )
                Best = Temp, Config ^= (1 << i);
    }
    *t = Best;
    if ( s_vTtMem == NULL )
        s_vTtMem = Vec_MemAllocForTT( 6, 0 );
    Vec_MemHashInsert( s_vTtMem, t );
    s_nCalls++;
    return Config;
}
void Mf_ManTruthQuit()
{
    if ( s_vTtMem == NULL )
        return;
    printf( "TT = %d (%.2f %%)\n", Vec_MemEntryNum(s_vTtMem), 100.0 * Vec_MemEntryNum(s_vTtMem) / s_nCalls );
    Vec_MemHashFree( s_vTtMem );
    Vec_MemFree( s_vTtMem );
    s_vTtMem = NULL;
    s_nCalls = 0;
}

Vec_Wrd_t * Mf_ManTruthCollect( int Limit )
{
    extern Vec_Wrd_t * Mpm_ManGetTruthWithCnf( int Limit );
    int * pPerm = Extra_PermSchedule( 6 );
    int * pComp = Extra_GreyCodeSchedule( 6 );
    Vec_Wrd_t * vTruths = Mpm_ManGetTruthWithCnf( Limit );
    Vec_Wrd_t * vResult = Vec_WrdAlloc( 1 << 20 );
    word uTruth, tCur, tTemp1, tTemp2;
    int i, p, c, k;
    Vec_WrdForEachEntry( vTruths, uTruth, k )
    {
        for ( i = 0; i < 2; i++ )
        {
            tCur = i ? ~uTruth : uTruth;
            tTemp1 = tCur;
            for ( p = 0; p < 720; p++ )
            {
                tTemp2 = tCur;
                for ( c = 0; c < 64; c++ )
                {
                    tCur = Abc_Tt6Flip( tCur, pComp[c] );
                    Vec_WrdPush( vResult, tCur );
                }
                assert( tTemp2 == tCur );
                tCur = Abc_Tt6SwapAdjacent( tCur, pPerm[p] );
            }
            assert( tTemp1 == tCur );
        }
    }
    ABC_FREE( pPerm );
    ABC_FREE( pComp );
    printf( "Original = %d.  ", Vec_WrdSize(vTruths) );
    Vec_WrdFree( vTruths );
    printf( "Total = %d.  ", Vec_WrdSize(vResult) );
    vTruths = Vec_WrdUniqifyHash( vResult, 1 );
    Vec_WrdFree( vResult );
    printf( "Unique = %d.  ", Vec_WrdSize(vTruths) );
    Vec_WrdForEachEntry( vTruths, uTruth, k )
    {
        Mf_ManTruthCanonicize( &uTruth, 6 );
        Vec_WrdWriteEntry( vTruths, k, uTruth );
    }
    vResult = Vec_WrdUniqifyHash( vTruths, 1 );
    Vec_WrdFree( vTruths );
    printf( "Unique = %d.  \n", Vec_WrdSize(vResult) );
    return vResult;
}
int Mf_ManTruthCount()
{
    Vec_Wrd_t * vTruths = Mf_ManTruthCollect( 10 );
    int RetValue = Vec_WrdSize( vTruths );
    Vec_WrdFree( vTruths );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Collect truth tables used by the mapper.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mf_ManProfileTruths( Mf_Man_t * p )
{
    Vec_Int_t * vCounts;
    int i, Entry, * pCut, Counter = 0;
    vCounts = Vec_IntStart( Vec_IntSize(&p->vCnfSizes) );
    Gia_ManForEachAndId( p->pGia, i )
    {
        if ( !Mf_ObjMapRefNum(p, i) )
            continue;
        pCut = Mf_ObjCutBest( p, i );
        Vec_IntAddToEntry( vCounts, Abc_Lit2Var(Mf_CutFunc(pCut)), 1 );
    }
    Vec_IntForEachEntry( vCounts, Entry, i )
    {
        if ( Entry == 0 )
            continue;
        printf( "%6d : ", Counter++ );
        printf( "%6d : ", i );
        printf( "Occur = %4d  ", Entry ); 
        printf( "CNF size = %2d  ", Vec_IntEntry(&p->vCnfSizes, i) );
        Dau_DsdPrintFromTruth( Vec_MemReadEntry(p->vTtMem, i), p->pPars->nLutSize );
    }
    Vec_IntFree( vCounts );
}

/**Function*************************************************************

  Synopsis    [Derives CNFs for each function used in the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Mf_CutPrintOne( int * pCut )
{
    int i; 
    printf( "%d {", Mf_CutSize(pCut) );
    for ( i = 1; i <= Mf_CutSize(pCut); i++ )
        printf( " %d", pCut[i] );
    printf( " }\n" );
}
static inline int Mf_CubeLit( int Cube, int iVar ) { return (Cube >> (iVar << 1)) & 3; }
static inline int Mf_ManCountLits( int * pCnf, int nCubes, int nVars )
{
    int i, k, nLits = nCubes;
    for ( i = 0; i < nCubes; i++ )
        for ( k = 0; k < nVars; k++ )
            if ( Mf_CubeLit(pCnf[i], k) )
                nLits++;
    return nLits;
}
Vec_Int_t * Mf_ManDeriveCnfs( Mf_Man_t * p, int * pnVars, int * pnClas, int * pnLits )
{
    int i, k, iFunc, nCubes, nLits, * pCut, pCnf[512];
    Vec_Int_t * vLits = Vec_IntStart( Vec_IntSize(&p->vCnfSizes) );
    Vec_Int_t * vCnfs = Vec_IntAlloc( 3 * Vec_IntSize(&p->vCnfSizes) );
    Vec_IntFill( vCnfs, Vec_IntSize(&p->vCnfSizes), -1 );
    assert( p->pPars->nLutSize <= 8 );
    // constant/buffer
    for ( iFunc = 0; iFunc < 2; iFunc++ )
    {
        if ( p->pPars->nLutSize <= 6 )
            nCubes = Abc_Tt6Cnf( *Vec_MemReadEntry(p->vTtMem, iFunc), iFunc, pCnf );
        else
            nCubes = Abc_Tt8Cnf( Vec_MemReadEntry(p->vTtMem, iFunc), iFunc, pCnf );
        nLits = Mf_ManCountLits( pCnf, nCubes, iFunc );
        Vec_IntWriteEntry( vLits, iFunc, nLits );
        Vec_IntWriteEntry( vCnfs, iFunc, Vec_IntSize(vCnfs) );
        Vec_IntPush( vCnfs, nCubes );
        for ( k = 0; k < nCubes; k++ )
            Vec_IntPush( vCnfs, pCnf[k] );
    }
    // other functions
    *pnVars = 1 + Gia_ManCiNum(p->pGia) + Gia_ManCoNum(p->pGia);
    *pnClas = 1 + 2 * Gia_ManCoNum(p->pGia);
    *pnLits = 1 + 4 * Gia_ManCoNum(p->pGia);
    Gia_ManForEachAndId( p->pGia, i )
    {
        if ( !Mf_ObjMapRefNum(p, i) )
            continue;
        pCut = Mf_ObjCutBest( p, i );
        //Mf_CutPrintOne( pCut );
        iFunc = Abc_Lit2Var( Mf_CutFunc(pCut) );
        if ( Vec_IntEntry(vCnfs, iFunc) == -1 )
        {
            if ( p->pPars->nLutSize <= 6 )
                nCubes = Abc_Tt6Cnf( *Vec_MemReadEntry(p->vTtMem, iFunc), Mf_CutSize(pCut), pCnf );
            else
                nCubes = Abc_Tt8Cnf( Vec_MemReadEntry(p->vTtMem, iFunc), Mf_CutSize(pCut), pCnf );
            assert( nCubes == Vec_IntEntry(&p->vCnfSizes, iFunc) );
            nLits = Mf_ManCountLits( pCnf, nCubes, Mf_CutSize(pCut) );
            // save CNF
            Vec_IntWriteEntry( vLits, iFunc, nLits );
            Vec_IntWriteEntry( vCnfs, iFunc, Vec_IntSize(vCnfs) );
            Vec_IntPush( vCnfs, nCubes );
            for ( k = 0; k < nCubes; k++ )
                Vec_IntPush( vCnfs, pCnf[k] );
        }
        *pnVars += 1;
        *pnClas += Vec_IntEntry(&p->vCnfSizes, iFunc);
        *pnLits += Vec_IntEntry(vLits, iFunc);
    }
    Vec_IntFree( vLits );
    return vCnfs;
}

/**Function*************************************************************

  Synopsis    [Derives CNF for the AIG using the mapping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cnf_Dat_t * Mf_ManDeriveCnf( Mf_Man_t * p, int fCnfObjIds, int fAddOrCla )
{
    Cnf_Dat_t * pCnf; 
    Gia_Obj_t * pObj;
    int Id, DriId, nVars, nClas, nLits, iVar = 1, iCla = 0, iLit = 0;
    Vec_Int_t * vCnfs = Mf_ManDeriveCnfs( p, &nVars, &nClas, &nLits );
    Vec_Int_t * vCnfIds = Vec_IntStartFull( Gia_ManObjNum(p->pGia) );
    int pFanins[16], * pCut, * pCnfIds = Vec_IntArray( vCnfIds );
    int i, k, c, iFunc, nCubes, * pCubes, fComplLast;
    nVars++;  // zero-ID to remain unused
    if ( fAddOrCla )
    {
        nClas++;
        nLits += Gia_ManCoNum(p->pGia);
    }
    // create CNF IDs
    if ( fCnfObjIds )
    {
        iVar += 1 + Gia_ManCiNum(p->pGia) + Gia_ManCoNum(p->pGia);
        Gia_ManForEachCoId( p->pGia, Id, i )
            Vec_IntWriteEntry( vCnfIds, Id, Id );
        Gia_ManForEachAndReverseId( p->pGia, Id )
            if ( Mf_ObjMapRefNum(p, Id) )
                Vec_IntWriteEntry( vCnfIds, Id, Id ), iVar++;
        Vec_IntWriteEntry( vCnfIds, 0, 0 );
        Gia_ManForEachCiId( p->pGia, Id, i )
            Vec_IntWriteEntry( vCnfIds, Id, Id );
        assert( iVar == nVars );
    }
    else
    {
        Gia_ManForEachCoId( p->pGia, Id, i )
            Vec_IntWriteEntry( vCnfIds, Id, iVar++ );
        Gia_ManForEachAndReverseId( p->pGia, Id )
            if ( Mf_ObjMapRefNum(p, Id) )
                Vec_IntWriteEntry( vCnfIds, Id, iVar++ );
        Vec_IntWriteEntry( vCnfIds, 0, iVar++ );
        Gia_ManForEachCiId( p->pGia, Id, i )
            Vec_IntWriteEntry( vCnfIds, Id, iVar++ );
        assert( iVar == nVars );
    }
    // generate CNF
    pCnf = ABC_CALLOC( Cnf_Dat_t, 1 );
    pCnf->pMan        = (Aig_Man_t *)p->pGia;
    pCnf->nVars       = nVars;
    pCnf->nLiterals   = nLits;
    pCnf->nClauses    = nClas;
    pCnf->pClauses    = ABC_ALLOC( int *, nClas+1 );
    pCnf->pClauses[0] = ABC_ALLOC( int, nLits );
    // add last clause
    if ( fAddOrCla )
    {
        pCnf->pClauses[iCla++] = pCnf->pClauses[0] + iLit;
        Gia_ManForEachCoId( p->pGia, Id, i )
            pCnf->pClauses[0][iLit++] = Abc_Var2Lit(pCnfIds[Id], 0);
    }
    if ( p->pPars->fCnfMapping )
        pCnf->vMapping = Vec_IntStart( nVars );
    // add clauses for the COs
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        Id = Gia_ObjId( p->pGia, pObj );
        DriId = Gia_ObjFaninId0( pObj, Id );

        pCnf->pClauses[iCla++] = pCnf->pClauses[0] + iLit;
        pCnf->pClauses[0][iLit++] = Abc_Var2Lit(pCnfIds[Id], 0);
        pCnf->pClauses[0][iLit++] = Abc_Var2Lit(pCnfIds[DriId], !Gia_ObjFaninC0(pObj));

        pCnf->pClauses[iCla++] = pCnf->pClauses[0] + iLit;
        pCnf->pClauses[0][iLit++] = Abc_Var2Lit(pCnfIds[Id], 1);
        pCnf->pClauses[0][iLit++] = Abc_Var2Lit(pCnfIds[DriId], Gia_ObjFaninC0(pObj));
        // generate mapping
        if ( pCnf->vMapping )
        {
            Vec_IntWriteEntry( pCnf->vMapping, pCnfIds[Id], Vec_IntSize(pCnf->vMapping) );
            Vec_IntPush( pCnf->vMapping, 1 );
            Vec_IntPush( pCnf->vMapping, pCnfIds[DriId] );
            Vec_IntPush( pCnf->vMapping, Gia_ObjFaninC0(pObj) ? 0x55555555 : 0xAAAAAAAA );
        }
    }
    // add clauses for the mapping
    Gia_ManForEachAndReverseId( p->pGia, Id )
    {
        if ( !Mf_ObjMapRefNum(p, Id) )
            continue;
        pCut = Mf_ObjCutBest( p, Id );
        iFunc = Abc_Lit2Var( Mf_CutFunc(pCut) );
        fComplLast = Abc_LitIsCompl( Mf_CutFunc(pCut) );
        if ( iFunc == 0 ) // constant cut
        {
            pCnf->pClauses[iCla++] = pCnf->pClauses[0] + iLit;
            pCnf->pClauses[0][iLit++] = Abc_Var2Lit(pCnfIds[Id], !fComplLast);
            assert( pCnf->vMapping == NULL ); // bug fix does not handle generated mapping
            continue;
        }
        for ( k = 0; k < Mf_CutSize(pCut); k++ )
            pFanins[k] = pCnfIds[pCut[k+1]];
        pFanins[k++] = pCnfIds[Id];
        // get clauses
        pCubes = Vec_IntEntryP( vCnfs, Vec_IntEntry(vCnfs, iFunc) );
        nCubes = *pCubes++;
        for ( c = 0; c < nCubes; c++ )
        {
            pCnf->pClauses[iCla++] = pCnf->pClauses[0] + iLit;
            k = Mf_CutSize(pCut);
            assert( Mf_CubeLit(pCubes[c], k) );
            pCnf->pClauses[0][iLit++] = Abc_Var2Lit( pFanins[k], (Mf_CubeLit(pCubes[c], k) == 2) ^ fComplLast );
            for ( k = 0; k < Mf_CutSize(pCut); k++ )
                if ( Mf_CubeLit(pCubes[c], k) )
                    pCnf->pClauses[0][iLit++] = Abc_Var2Lit( pFanins[k], Mf_CubeLit(pCubes[c], k) == 2 );
        }
        // generate mapping
        if ( pCnf->vMapping )
        {
            word pTruth[4], * pTruthP = Vec_MemReadEntry(p->vTtMem, iFunc);
            assert( p->pPars->nLutSize <= 8 );
            Abc_TtCopy( pTruth, pTruthP, Abc_Truth6WordNum(p->pPars->nLutSize), Abc_LitIsCompl(iFunc) );
            assert( pCnfIds[Id] >= 0 && pCnfIds[Id] < nVars );
            Vec_IntWriteEntry( pCnf->vMapping, pCnfIds[Id], Vec_IntSize(pCnf->vMapping) );
            Vec_IntPush( pCnf->vMapping, Mf_CutSize(pCut) );
            for ( k = 0; k < Mf_CutSize(pCut); k++ )
                Vec_IntPush( pCnf->vMapping, pCnfIds[pCut[k+1]] );
            Vec_IntPush( pCnf->vMapping, (unsigned)pTruth[0] );
            if ( Mf_CutSize(pCut) >= 6 )
            {
                Vec_IntPush( pCnf->vMapping, (unsigned)(pTruth[0] >> 32) );
                if ( Mf_CutSize(pCut) >= 7 )
                {
                    Vec_IntPush( pCnf->vMapping, (unsigned)(pTruth[1]) );
                    Vec_IntPush( pCnf->vMapping, (unsigned)(pTruth[1] >> 32) );
                }
                if ( Mf_CutSize(pCut) >= 8 )
                {
                    Vec_IntPush( pCnf->vMapping, (unsigned)(pTruth[2]) );
                    Vec_IntPush( pCnf->vMapping, (unsigned)(pTruth[2] >> 32) );
                    Vec_IntPush( pCnf->vMapping, (unsigned)(pTruth[3]) );
                    Vec_IntPush( pCnf->vMapping, (unsigned)(pTruth[3] >> 32) );
                }
            }
        }
    }
    // constant clause
    pCnf->pClauses[iCla++] = pCnf->pClauses[0] + iLit;
    pCnf->pClauses[0][iLit++] = Abc_Var2Lit(pCnfIds[0], 1);
    assert( iCla == nClas );
    assert( iLit == nLits );
    // add closing pointer
    pCnf->pClauses[iCla++] = pCnf->pClauses[0] + iLit;
    // cleanup
    Vec_IntFree( vCnfs );
    // create mapping of objects into their clauses
    if ( fCnfObjIds )
    {
        pCnf->pObj2Clause = ABC_FALLOC( int, Gia_ManObjNum(p->pGia) );
        pCnf->pObj2Count  = ABC_FALLOC( int, Gia_ManObjNum(p->pGia) );
        for ( i = 0; i < pCnf->nClauses; i++ )
        {
            Id = Abc_Lit2Var(pCnf->pClauses[i][0]);
            if ( pCnf->pObj2Clause[Id] == -1 )
            {
                pCnf->pObj2Clause[Id] = i;
                pCnf->pObj2Count[Id] = 1;
            }
            else
            {
                assert( pCnf->pObj2Count[Id] > 0 );
                pCnf->pObj2Count[Id]++;
            }
        }
    }
    else
    {
        if ( p->pGia != p->pGia0 ) // diff managers - create map for CIs/COs
        {
            pCnf->pVarNums = ABC_FALLOC( int, Gia_ManObjNum(p->pGia0) );
            Gia_ManForEachCiId( p->pGia0, Id, i )
                pCnf->pVarNums[Id] = pCnfIds[Gia_ManCiIdToId(p->pGia, i)];
            Gia_ManForEachCoId( p->pGia0, Id, i )
                pCnf->pVarNums[Id] = pCnfIds[Gia_ManCoIdToId(p->pGia, i)];
/*
            // transform polarity of the internal nodes
            Gia_ManSetPhase( p->pGia );
            Gia_ManForEachCo( p->pGia, pObj, i )
                pObj->fPhase = 0;
            for ( i = 0; i < pCnf->nLiterals; i++ )
                if ( Gia_ManObj(p->pGia, Abc_Lit2Var(pCnf->pClauses[0][i]))->fPhase )
                    pCnf->pClauses[0][i] = Abc_LitNot( pCnf->pClauses[0][i] );
*/
        }
        else
            pCnf->pVarNums = Vec_IntReleaseArray(vCnfIds);
    }
    Vec_IntFree( vCnfIds );
    return pCnf;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Mf_CutComputeTruth6( Mf_Man_t * p, Mf_Cut_t * pCut0, Mf_Cut_t * pCut1, int fCompl0, int fCompl1, Mf_Cut_t * pCutR, int fIsXor )
{
//    extern int Mf_ManTruthCanonicize( word * t, int nVars );
    int nOldSupp = pCutR->nLeaves, truthId, fCompl; word t;
    word t0 = *Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut0->iFunc));
    word t1 = *Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut1->iFunc));
    if ( Abc_LitIsCompl(pCut0->iFunc) ^ fCompl0 ) t0 = ~t0;
    if ( Abc_LitIsCompl(pCut1->iFunc) ^ fCompl1 ) t1 = ~t1;
    t0 = Abc_Tt6Expand( t0, pCut0->pLeaves, pCut0->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    t1 = Abc_Tt6Expand( t1, pCut1->pLeaves, pCut1->nLeaves, pCutR->pLeaves, pCutR->nLeaves );
    t =  fIsXor ? t0 ^ t1 : t0 & t1;
    if ( (fCompl = (int)(t & 1)) ) t = ~t;
    if ( !p->pPars->fCnfObjIds )
    pCutR->nLeaves = Abc_Tt6MinBase( &t, pCutR->pLeaves, pCutR->nLeaves );
    assert( (int)(t & 1) == 0 );
    truthId        = Vec_MemHashInsert(p->vTtMem, &t);
    pCutR->iFunc   = Abc_Var2Lit( truthId, fCompl );
    if ( p->pPars->fGenCnf && truthId == Vec_IntSize(&p->vCnfSizes) )
        Vec_IntPush( &p->vCnfSizes, Abc_Tt6CnfSize(t, pCutR->nLeaves) );
//    p->nCutMux += Mf_ManTtIsMux( t );
    assert( (int)pCutR->nLeaves <= nOldSupp );
//    Mf_ManTruthCanonicize( &t, pCutR->nLeaves );
    return (int)pCutR->nLeaves < nOldSupp;
}
static inline int Mf_CutComputeTruth( Mf_Man_t * p, Mf_Cut_t * pCut0, Mf_Cut_t * pCut1, int fCompl0, int fCompl1, Mf_Cut_t * pCutR, int fIsXor )
{
    if ( p->pPars->nLutSize <= 6 )
        return Mf_CutComputeTruth6( p, pCut0, pCut1, fCompl0, fCompl1, pCutR, fIsXor );
    {
    word uTruth[MF_TT_WORDS], uTruth0[MF_TT_WORDS], uTruth1[MF_TT_WORDS];
    int nOldSupp   = pCutR->nLeaves, truthId;
    int LutSize    = p->pPars->nLutSize, fCompl;
    int nWords     = Abc_Truth6WordNum(LutSize);
    word * pTruth0 = Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut0->iFunc));
    word * pTruth1 = Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut1->iFunc));
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
    if ( p->pPars->fGenCnf && truthId == Vec_IntSize(&p->vCnfSizes) && LutSize <= 8 )
        Vec_IntPush( &p->vCnfSizes, Abc_Tt8CnfSize(uTruth, pCutR->nLeaves) );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
    }
}
static inline int Mf_CutComputeTruthMux6( Mf_Man_t * p, Mf_Cut_t * pCut0, Mf_Cut_t * pCut1, Mf_Cut_t * pCutC, int fCompl0, int fCompl1, int fComplC, Mf_Cut_t * pCutR )
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
    if ( p->pPars->fGenCnf && truthId == Vec_IntSize(&p->vCnfSizes) )
        Vec_IntPush( &p->vCnfSizes, Abc_Tt6CnfSize(t, pCutR->nLeaves) );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
}
static inline int Mf_CutComputeTruthMux( Mf_Man_t * p, Mf_Cut_t * pCut0, Mf_Cut_t * pCut1, Mf_Cut_t * pCutC, int fCompl0, int fCompl1, int fComplC, Mf_Cut_t * pCutR )
{
    if ( p->pPars->nLutSize <= 6 )
        return Mf_CutComputeTruthMux6( p, pCut0, pCut1, pCutC, fCompl0, fCompl1, fComplC, pCutR );
    {
    word uTruth[MF_TT_WORDS], uTruth0[MF_TT_WORDS], uTruth1[MF_TT_WORDS], uTruthC[MF_TT_WORDS];
    int nOldSupp   = pCutR->nLeaves, truthId;
    int LutSize    = p->pPars->nLutSize, fCompl;
    int nWords     = Abc_Truth6WordNum(LutSize);
    word * pTruth0 = Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut0->iFunc));
    word * pTruth1 = Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut1->iFunc));
    word * pTruthC = Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCutC->iFunc));
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
    if ( p->pPars->fGenCnf && truthId == Vec_IntSize(&p->vCnfSizes) && LutSize <= 8 )
        Vec_IntPush( &p->vCnfSizes, Abc_Tt8CnfSize(uTruth, pCutR->nLeaves) );
    assert( (int)pCutR->nLeaves <= nOldSupp );
    return (int)pCutR->nLeaves < nOldSupp;
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Mf_CutCountBits( word i )
{
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
    i = ((i + (i >> 4)) & 0x0F0F0F0F0F0F0F0F);
    return (i*(0x0101010101010101))>>56;
}
static inline word Mf_CutGetSign( int * pLeaves, int nLeaves )
{
    word Sign = 0; int i; 
    for ( i = 0; i < nLeaves; i++ )
        Sign |= ((word)1) << (pLeaves[i] & 0x3F);
    return Sign;
}
static inline int Mf_CutCreateUnit( Mf_Cut_t * p, int i )
{
    p->Delay      = 0;
    p->Flow       = 0;
    p->iFunc      = 2;
    p->nLeaves    = 1;
    p->pLeaves[0] = i;
    p->Sign       = ((word)1) << (i & 0x3F);
    return 1;
}
static inline void Mf_CutPrint( Mf_Man_t * p, Mf_Cut_t * pCut )
{
    int i, nDigits = Abc_Base10Log(Gia_ManObjNum(p->pGia)); 
    printf( "%d  {", pCut->nLeaves );
    for ( i = 0; i < (int)pCut->nLeaves; i++ )
        printf( " %*d", nDigits, pCut->pLeaves[i] );
    for ( ; i < (int)p->pPars->nLutSize; i++ )
        printf( " %*s", nDigits, " " );
    printf( "  }   D = %4d  A = %9.4f  F = %6d  ", 
        pCut->Delay, pCut->Flow, pCut->iFunc );
    if ( p->vTtMem )
    {
        if ( p->pPars->fGenCnf )
            printf( "CNF = %2d  ", Vec_IntEntry(&p->vCnfSizes, Abc_Lit2Var(pCut->iFunc)) );
        Dau_DsdPrintFromTruth( Vec_MemReadEntry(p->vTtMem, Abc_Lit2Var(pCut->iFunc)), pCut->nLeaves );
    }
    else
        printf( "\n" );
}
static inline int Mf_ManPrepareCuts( Mf_Cut_t * pCuts, Mf_Man_t * p, int iObj, int fAddUnit )
{
    if ( Mf_ManObj(p, iObj)->iCutSet )
    {
        Mf_Cut_t * pMfCut = pCuts;
        int i, * pCut, * pList = Mf_ObjCutSet(p, iObj);
        Mf_SetForEachCut( pList, pCut, i )
        {
            pMfCut->Delay   = 0;
            pMfCut->Flow    = 0;
            pMfCut->iFunc   = Mf_CutFunc( pCut );
            pMfCut->nLeaves = Mf_CutSize( pCut );
            pMfCut->Sign    = Mf_CutGetSign( pCut+1, Mf_CutSize(pCut) );
            memcpy( pMfCut->pLeaves, pCut+1, sizeof(int) * Mf_CutSize(pCut) );
            pMfCut++;
        }
        if ( fAddUnit && pCuts->nLeaves > 1 )
            return pList[0] + Mf_CutCreateUnit( pMfCut, iObj );
        return pList[0];
    }
    return Mf_CutCreateUnit( pCuts, iObj );
}
static inline int Mf_ManSaveCuts( Mf_Man_t * p, Mf_Cut_t ** pCuts, int nCuts )
{
    int i, * pPlace, iCur, nInts = 1;
    for ( i = 0; i < nCuts; i++ )
        nInts += pCuts[i]->nLeaves + 1;
    if ( (p->iCur & 0xFFFF) + nInts > 0xFFFF )
        p->iCur = ((p->iCur >> 16) + 1) << 16;
    if ( Vec_PtrSize(&p->vPages) == (p->iCur >> 16) )
        Vec_PtrPush( &p->vPages, ABC_ALLOC(int, (1<<16)) );
    iCur = p->iCur; p->iCur += nInts;
    pPlace = Mf_ManCutSet( p, iCur );
    *pPlace++ = nCuts;
    for ( i = 0; i < nCuts; i++ )
    {
        *pPlace++ = Mf_CutSetBoth(pCuts[i]->nLeaves, pCuts[i]->iFunc);
        memcpy( pPlace, pCuts[i]->pLeaves, sizeof(int) * pCuts[i]->nLeaves );
        pPlace += pCuts[i]->nLeaves;
    }
    return iCur;
}
static inline void Mf_ObjSetBestCut( int * pCuts, int * pCut )
{
    assert( pCuts < pCut );
    if ( ++pCuts < pCut )
    {
        int pTemp[MF_CUT_MAX*(MF_LEAF_MAX+2)];
        int nBlock = pCut - pCuts;
        int nSize = Mf_CutSize(pCut) + 1;
        memmove( pTemp, pCuts, sizeof(int) * nBlock );
        memmove( pCuts, pCut, sizeof(int) * nSize );
        memmove( pCuts + nSize, pTemp, sizeof(int) * nBlock );
    }
}

/**Function*************************************************************

  Synopsis    [Check correctness of cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Mf_CutCheck( Mf_Cut_t * pBase, Mf_Cut_t * pCut ) // check if pCut is contained in pBase
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
static inline int Mf_SetCheckArray( Mf_Cut_t ** ppCuts, int nCuts )
{
    Mf_Cut_t * pCut0, * pCut1; 
    int i, k, m, n, Value;
    assert( nCuts > 0 );
    for ( i = 0; i < nCuts; i++ )
    {
        pCut0 = ppCuts[i];
        assert( pCut0->nLeaves <= MF_LEAF_MAX );
        assert( pCut0->Sign == Mf_CutGetSign(pCut0->pLeaves, pCut0->nLeaves) );
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
            Value = Mf_CutCheck( pCut0, pCut1 );
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
static inline int Mf_CutMergeOrder( Mf_Cut_t * pCut0, Mf_Cut_t * pCut1, Mf_Cut_t * pCut, int nLutSize )
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
        pCut->iFunc = MF_NO_FUNC;
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
    pCut->iFunc = MF_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;

FlushCut1:
    if ( c + nSize1 > nLutSize + k ) return 0;
    while ( k < nSize1 )
        pC[c++] = pC1[k++];
    pCut->nLeaves = c;
    pCut->iFunc = MF_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign;
    return 1;
}
static inline int Mf_CutMergeOrderMux( Mf_Cut_t * pCut0, Mf_Cut_t * pCut1, Mf_Cut_t * pCut2, Mf_Cut_t * pCut, int nLutSize )
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
    pCut->iFunc = MF_NO_FUNC;
    pCut->Sign = pCut0->Sign | pCut1->Sign | pCut2->Sign;
    return 1;
}
static inline int Mf_SetCutIsContainedOrder( Mf_Cut_t * pBase, Mf_Cut_t * pCut ) // check if pCut is contained in pBase
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
static inline int Mf_SetLastCutIsContained( Mf_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[i]->nLeaves <= pCuts[nCuts]->nLeaves && (pCuts[i]->Sign & pCuts[nCuts]->Sign) == pCuts[i]->Sign && Mf_SetCutIsContainedOrder(pCuts[nCuts], pCuts[i]) )
            return 1;
    return 0;
}
static inline int Mf_SetLastCutContainsArea( Mf_Cut_t ** pCuts, int nCuts )
{
    int i, k, fChanges = 0;
    for ( i = 0; i < nCuts; i++ )
        if ( pCuts[nCuts]->nLeaves < pCuts[i]->nLeaves && (pCuts[nCuts]->Sign & pCuts[i]->Sign) == pCuts[nCuts]->Sign && Mf_SetCutIsContainedOrder(pCuts[i], pCuts[nCuts]) )
            pCuts[i]->nLeaves = MF_NO_LEAF, fChanges = 1;
    if ( !fChanges )
        return nCuts;
    for ( i = k = 0; i <= nCuts; i++ )
    {
        if ( pCuts[i]->nLeaves == MF_NO_LEAF )
            continue;
        if ( k < i )
            ABC_SWAP( Mf_Cut_t *, pCuts[k], pCuts[i] );
        k++;
    }
    return k - 1;
}
static inline int Mf_CutCompareArea( Mf_Cut_t * pCut0, Mf_Cut_t * pCut1 )
{
    if ( pCut0->Flow    < pCut1->Flow - MF_EPSILON )  return -1;
    if ( pCut0->Flow    > pCut1->Flow + MF_EPSILON )  return  1;
    if ( pCut0->Delay   < pCut1->Delay   )  return -1;
    if ( pCut0->Delay   > pCut1->Delay   )  return  1;
    if ( pCut0->nLeaves < pCut1->nLeaves )  return -1;
    if ( pCut0->nLeaves > pCut1->nLeaves )  return  1;
    return 0;
}
static inline void Mf_SetSortByArea( Mf_Cut_t ** pCuts, int nCuts )
{
    int i;
    for ( i = nCuts; i > 0; i-- )
    {
        if ( Mf_CutCompareArea(pCuts[i - 1], pCuts[i]) < 0 )//!= 1 )
            return;
        ABC_SWAP( Mf_Cut_t *, pCuts[i - 1], pCuts[i] );
    }
}
static inline int Mf_SetAddCut( Mf_Cut_t ** pCuts, int nCuts, int nCutNum )
{
    if ( nCuts == 0 )
        return 1;
    nCuts = Mf_SetLastCutContainsArea(pCuts, nCuts);
    Mf_SetSortByArea( pCuts, nCuts );
    return Abc_MinInt( nCuts + 1, nCutNum - 1 );
}
static inline int Mf_CutArea( Mf_Man_t * p, int nLeaves, int iFunc )
{
    if ( nLeaves < 2 )
        return 0;
    if ( p->pPars->fGenCnf )
        return Vec_IntEntry(&p->vCnfSizes, Abc_Lit2Var(iFunc));
    if ( p->pPars->fOptEdge )
        return nLeaves + p->pPars->nAreaTuner;
    return 1;
}
static inline void Mf_CutParams( Mf_Man_t * p, Mf_Cut_t * pCut, float FlowRefs )
{
    Mf_Obj_t * pBest; 
    int i, nLeaves = pCut->nLeaves; 
    assert( nLeaves <= p->pPars->nLutSize );
    pCut->Delay = 0;
    pCut->Flow  = 0;
    for ( i = 0; i < nLeaves; i++ )
    {
        pBest = Mf_ManObj(p, pCut->pLeaves[i]);
        pCut->Delay = Abc_MaxInt( pCut->Delay, pBest->Delay );
        pCut->Flow += pBest->Flow;
    }
    pCut->Delay += (int)(nLeaves > 1);
    pCut->Flow = (pCut->Flow + Mf_CutArea(p, nLeaves, pCut->iFunc)) / FlowRefs;
}
void Mf_ObjMergeOrder( Mf_Man_t * p, int iObj )
{
    Mf_Cut_t pCuts0[MF_CUT_MAX], pCuts1[MF_CUT_MAX], pCuts[MF_CUT_MAX], * pCutsR[MF_CUT_MAX];
    Gia_Obj_t * pObj = Gia_ManObj(p->pGia, iObj);
    Mf_Obj_t * pBest = Mf_ManObj(p, iObj);
    int nLutSize = p->pPars->nLutSize;
    int nCutNum  = p->pPars->nCutNum;
    int nCuts0   = Mf_ManPrepareCuts(pCuts0, p, Gia_ObjFaninId0(pObj, iObj), 1);
    int nCuts1   = Mf_ManPrepareCuts(pCuts1, p, Gia_ObjFaninId1(pObj, iObj), 1);
    int fComp0   = Gia_ObjFaninC0(pObj);
    int fComp1   = Gia_ObjFaninC1(pObj);
    int iSibl    = Gia_ObjSibl(p->pGia, iObj);
    Mf_Cut_t * pCut0, * pCut1, * pCut0Lim = pCuts0 + nCuts0, * pCut1Lim = pCuts1 + nCuts1;
    int i, nCutsR = 0;
    for ( i = 0; i < nCutNum; i++ )
        pCutsR[i] = pCuts + i;
    if ( iSibl )
    {
        Mf_Cut_t pCuts2[MF_CUT_MAX];
        Gia_Obj_t * pObjE = Gia_ObjSiblObj(p->pGia, iObj);
        int fCompE = Gia_ObjPhase(pObj) ^ Gia_ObjPhase(pObjE);
        int nCuts2 = Mf_ManPrepareCuts(pCuts2, p, iSibl, 0);
        Mf_Cut_t * pCut2, * pCut2Lim = pCuts2 + nCuts2;
        for ( pCut2 = pCuts2; pCut2 < pCut2Lim; pCut2++ )
        {
            *pCutsR[nCutsR] = *pCut2;
            if ( pCutsR[nCutsR]->iFunc >= 0 )
                pCutsR[nCutsR]->iFunc = Abc_LitNotCond( pCutsR[nCutsR]->iFunc, fCompE );
            Mf_CutParams( p, pCutsR[nCutsR], pBest->nFlowRefs );
            nCutsR = Mf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    if ( Gia_ObjIsMuxId(p->pGia, iObj) )
    {
        Mf_Cut_t pCuts2[MF_CUT_MAX];
        int nCuts2  = Mf_ManPrepareCuts(pCuts2, p, Gia_ObjFaninId2(p->pGia, iObj), 1);
        int fComp2  = Gia_ObjFaninC2(p->pGia, pObj);
        Mf_Cut_t * pCut2, * pCut2Lim = pCuts2 + nCuts2;
        p->CutCount[0] += nCuts0 * nCuts1 * nCuts2;
        for ( pCut0 = pCuts0; pCut0 < pCut0Lim; pCut0++ )
        for ( pCut1 = pCuts1; pCut1 < pCut1Lim; pCut1++ )
        for ( pCut2 = pCuts2; pCut2 < pCut2Lim; pCut2++ )
        {
            if ( Mf_CutCountBits(pCut0->Sign | pCut1->Sign | pCut2->Sign) > nLutSize )
                continue;
            p->CutCount[1]++; 
            if ( !Mf_CutMergeOrderMux(pCut0, pCut1, pCut2, pCutsR[nCutsR], nLutSize) )
                continue;
            if ( Mf_SetLastCutIsContained(pCutsR, nCutsR) )
                continue;
            p->CutCount[2]++;
            if ( p->pPars->fCutMin && Mf_CutComputeTruthMux(p, pCut0, pCut1, pCut2, fComp0, fComp1, fComp2, pCutsR[nCutsR]) )
                pCutsR[nCutsR]->Sign = Mf_CutGetSign(pCutsR[nCutsR]->pLeaves, pCutsR[nCutsR]->nLeaves);
            Mf_CutParams( p, pCutsR[nCutsR], pBest->nFlowRefs );
            nCutsR = Mf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    else
    {
        int fIsXor = Gia_ObjIsXor(pObj);
        p->CutCount[0] += nCuts0 * nCuts1;
        for ( pCut0 = pCuts0; pCut0 < pCut0Lim; pCut0++ )
        for ( pCut1 = pCuts1; pCut1 < pCut1Lim; pCut1++ )
        {
            if ( (int)(pCut0->nLeaves + pCut1->nLeaves) > nLutSize && Mf_CutCountBits(pCut0->Sign | pCut1->Sign) > nLutSize )
                continue;
            p->CutCount[1]++; 
            if ( !Mf_CutMergeOrder(pCut0, pCut1, pCutsR[nCutsR], nLutSize) )
                continue;
            if ( Mf_SetLastCutIsContained(pCutsR, nCutsR) )
                continue;
            p->CutCount[2]++;
            if ( p->pPars->fCutMin && Mf_CutComputeTruth(p, pCut0, pCut1, fComp0, fComp1, pCutsR[nCutsR], fIsXor) )
                pCutsR[nCutsR]->Sign = Mf_CutGetSign(pCutsR[nCutsR]->pLeaves, pCutsR[nCutsR]->nLeaves);
            Mf_CutParams( p, pCutsR[nCutsR], pBest->nFlowRefs );
            nCutsR = Mf_SetAddCut( pCutsR, nCutsR, nCutNum );
        }
    }
    // debug printout
    if ( 0 )
//    if ( iObj % 1000 == 0 )
//    if ( iObj == 474 )
    {
        printf( "*** Obj = %d  FlowRefs = %.2f  MapRefs = %2d\n", iObj, pBest->nFlowRefs, pBest->nMapRefs );
        for ( i = 0; i < nCutsR; i++ )
            Mf_CutPrint( p, pCutsR[i] );
        printf( "\n" );
    } 
    // store the cutset
    pBest->Flow = pCutsR[0]->Flow;
    pBest->Delay = pCutsR[0]->Delay;
    pBest->iCutSet = Mf_ManSaveCuts( p, pCutsR, nCutsR );
    // verify
    assert( nCutsR > 0 && nCutsR < nCutNum );
//    assert( Mf_SetCheckArray(pCutsR, nCutsR) );
    p->nCutCounts[pCutsR[0]->nLeaves]++;
    p->CutCount[3] += nCutsR;
}
 

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mf_ManSetFlowRefs( Gia_Man_t * p, Vec_Int_t * vRefs )
{
    int fDiscount = 1;
    Gia_Obj_t * pObj, * pCtrl, * pData0, * pData1; 
    int i, Id;
    Vec_IntFill( vRefs, Gia_ManObjNum(p), 0 );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( Gia_ObjIsAnd(Gia_ObjFanin0(pObj)) )
            Vec_IntAddToEntry( vRefs, Gia_ObjFaninId0(pObj, i), 1 );
        if ( Gia_ObjIsAnd(Gia_ObjFanin1(pObj)) )
            Vec_IntAddToEntry( vRefs, Gia_ObjFaninId1(pObj, i), 1 );
        if ( p->pMuxes )
        {
            if ( Gia_ObjIsMuxId(p, i) && Gia_ObjIsAnd(Gia_ObjFanin2(p, pObj)) )
                Vec_IntAddToEntry( vRefs, Gia_ObjFaninId2(p, i), 1 );
        }
        else if ( fDiscount && Gia_ObjIsMuxType(pObj) ) // discount XOR/MUX
        {
            pCtrl  = Gia_Regular(Gia_ObjRecognizeMux(pObj, &pData1, &pData0));
            pData0 = Gia_Regular(pData0);
            pData1 = Gia_Regular(pData1);
            if ( Gia_ObjIsAnd(pCtrl) )
                Vec_IntAddToEntry( vRefs, Gia_ObjId(p, pCtrl), -1 );
            if ( pData0 == pData1 && Gia_ObjIsAnd(pData0) )
                Vec_IntAddToEntry( vRefs, Gia_ObjId(p, pData0), -1 );
        }
    }
    Gia_ManForEachCoDriverId( p, Id, i )
        if ( Gia_ObjIsAnd(Gia_ManObj(p, Id)) )
            Vec_IntAddToEntry( vRefs, Id, 1 );
    for ( i = 0; i < Vec_IntSize(vRefs); i++ )
        Vec_IntUpdateEntry( vRefs, i, 1 );
}
int Mf_ManSetMapRefs( Mf_Man_t * p )
{
    float Coef = 1.0 / (1.0 + (p->Iter + 1) * (p->Iter + 1));
    int * pCut, i, k, Id;
    // compute delay
    int Delay = 0;
    Gia_ManForEachCoDriverId( p->pGia, Id, i )
        Delay = Abc_MaxInt( Delay, Mf_ManObj(p, Id)->Delay );
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
    // check references
//    Gia_ManForEachAndId( p->pGia, i )
//        assert( Mf_ManObj(p, i)->nMapRefs == 0 );
    // compute area and edges
    if ( !p->fUseEla )
        Gia_ManForEachCoDriverId( p->pGia, Id, i )
            Mf_ObjMapRefInc( p, Id );
    p->pPars->Area = p->pPars->Edge = p->pPars->Clause = 0;
    Gia_ManForEachAndReverseId( p->pGia, i )
    {
        if ( !Mf_ObjMapRefNum(p, i) )
            continue;
        pCut = Mf_ObjCutBest( p, i );
        if ( !p->fUseEla )
            for ( k = 1; k <= Mf_CutSize(pCut); k++ )
                Mf_ObjMapRefInc( p, pCut[k] );
        p->pPars->Edge += Mf_CutSize(pCut);
        p->pPars->Area++;
        if ( p->pPars->fGenCnf )
            p->pPars->Clause += Mf_CutArea(p, Mf_CutSize(pCut), Mf_CutFunc(pCut));
    }
    // blend references
    for ( i = 0; i < Gia_ManObjNum(p->pGia); i++ )
        p->pLfObjs[i].nFlowRefs = Coef * p->pLfObjs[i].nFlowRefs + (1.0 - Coef) * Abc_MaxFloat(1, p->pLfObjs[i].nMapRefs);
//        p->pLfObjs[i]. = 0.2 * p->pLfObjs[i]. + 0.8 * Abc_MaxFloat(1, p->pLfObjs[i].nMapRefs);
    return p->pPars->Area;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Mf_ManDeriveMapping( Mf_Man_t * p )
{
    Vec_Int_t * vMapping;
    int i, k, * pCut;
    assert( !p->pPars->fCutMin && p->pGia->vMapping == NULL );
    vMapping = Vec_IntAlloc( Gia_ManObjNum(p->pGia) + (int)p->pPars->Edge + (int)p->pPars->Area * 2 );
    Vec_IntFill( vMapping, Gia_ManObjNum(p->pGia), 0 );
    Gia_ManForEachAndId( p->pGia, i )
    {
        if ( !Mf_ObjMapRefNum(p, i) )
            continue;
        pCut = Mf_ObjCutBest( p, i );
        Vec_IntWriteEntry( vMapping, i, Vec_IntSize(vMapping) );
        Vec_IntPush( vMapping, Mf_CutSize(pCut) );
        for ( k = 1; k <= Mf_CutSize(pCut); k++ )
            Vec_IntPush( vMapping, pCut[k] );
        Vec_IntPush( vMapping, i );
    }
    assert( Vec_IntCap(vMapping) == 16 || Vec_IntSize(vMapping) == Vec_IntCap(vMapping) );
    p->pGia->vMapping = vMapping;
    return p->pGia;
}
Gia_Man_t * Mf_ManDeriveMappingCoarse( Mf_Man_t * p )
{
    Gia_Man_t * pNew, * pGia = p->pGia;
    Gia_Obj_t * pObj;
    int i, k, * pCut;
    assert( !p->pPars->fCutMin && pGia->pMuxes );
    // create new manager
    pNew = Gia_ManStart( Gia_ManObjNum(pGia) );
    pNew->pName = Abc_UtilStrsav( pGia->pName );
    pNew->pSpec = Abc_UtilStrsav( pGia->pSpec );
    // map primary inputs
    Gia_ManConst0(pGia)->Value = 0;
    Gia_ManForEachCi( pGia, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    // start mapping
    pNew->vMapping = Vec_IntAlloc( Gia_ManObjNum(pGia) + 2*Gia_ManXorNum(pGia) + 2*Gia_ManMuxNum(pGia) + (int)p->pPars->Edge + (int)p->pPars->Area * 2 );
    Vec_IntFill( pNew->vMapping, Gia_ManObjNum(pGia) + 2*Gia_ManXorNum(pGia) + 2*Gia_ManMuxNum(pGia), 0 );
    // iterate through nodes used in the mapping
    Gia_ManForEachAnd( pGia, pObj, i )
    {
        if ( Gia_ObjIsMuxId(pGia, i) )
            pObj->Value = Gia_ManAppendMux( pNew, Gia_ObjFanin2Copy(pGia, pObj), Gia_ObjFanin1Copy(pObj), Gia_ObjFanin0Copy(pObj) );
        else if ( Gia_ObjIsXor(pObj) )
            pObj->Value = Gia_ManAppendXor( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        else 
            pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        if ( !Mf_ObjMapRefNum(p, i) )
            continue;
        pCut = Mf_ObjCutBest( p, i );
        Vec_IntWriteEntry( pNew->vMapping, Abc_Lit2Var(pObj->Value), Vec_IntSize(pNew->vMapping) );
        Vec_IntPush( pNew->vMapping, Mf_CutSize(pCut));
        for ( k = 1; k <= Mf_CutSize(pCut); k++ )
            Vec_IntPush( pNew->vMapping, Abc_Lit2Var(Gia_ManObj(pGia, pCut[k])->Value) );
        Vec_IntPush( pNew->vMapping, Abc_Lit2Var(pObj->Value) );
    }
    Gia_ManForEachCo( pGia, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(pGia) );
    assert( Vec_IntCap(pNew->vMapping) == 16 || Vec_IntSize(pNew->vMapping) == Vec_IntCap(pNew->vMapping) );
    return pNew;
}
Gia_Man_t * Mf_ManDeriveMappingGia( Mf_Man_t * p )
{
    Gia_Man_t * pNew; 
    Gia_Obj_t * pObj; 
    Vec_Int_t * vCopies   = Vec_IntStartFull( Gia_ManObjNum(p->pGia) );
    Vec_Int_t * vMapping  = Vec_IntStart( 2 * Gia_ManObjNum(p->pGia) + (int)p->pPars->Edge + 2 * (int)p->pPars->Area );
    Vec_Int_t * vMapping2 = Vec_IntStart( (int)p->pPars->Edge + 2 * (int)p->pPars->Area + 1000 );
    Vec_Int_t * vCover    = Vec_IntAlloc( 1 << 16 );
    Vec_Int_t * vLeaves   = Vec_IntAlloc( 16 );
    int i, k, Id, iLit, * pCut;
    word uTruth = 0, * pTruth = &uTruth;
    assert( p->pPars->fCutMin );
    // create new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p->pGia) );
    pNew->pName = Abc_UtilStrsav( p->pGia->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pGia->pSpec );
    // map primary inputs
    Vec_IntWriteEntry( vCopies, 0, 0 );
    Gia_ManForEachCiId( p->pGia, Id, i )
        Vec_IntWriteEntry( vCopies, Id, Gia_ManAppendCi(pNew) );
    // iterate through nodes used in the mapping
    Gia_ManForEachAnd( p->pGia, pObj, i )
    {
        if ( !Mf_ObjMapRefNum(p, i) )
            continue;
        pCut = Mf_ObjCutBest( p, i );
        if ( Mf_CutSize(pCut) == 0 )
        {
            assert( Abc_Lit2Var(Mf_CutFunc(pCut)) == 0 );
            Vec_IntWriteEntry( vCopies, i, Mf_CutFunc(pCut) );
            continue;
        }
        if ( Mf_CutSize(pCut) == 1 )
        {
            assert( Abc_Lit2Var(Mf_CutFunc(pCut)) == 1 );
            iLit = Vec_IntEntry( vCopies, pCut[1] );
            Vec_IntWriteEntry( vCopies, i, Abc_LitNotCond(iLit, Abc_LitIsCompl(Mf_CutFunc(pCut))) );
            continue;
        }
        Vec_IntClear( vLeaves );
        for ( k = 1; k <= Mf_CutSize(pCut); k++ )
            Vec_IntPush( vLeaves, Vec_IntEntry(vCopies, pCut[k]) );
        pTruth = Vec_MemReadEntry( p->vTtMem, Abc_Lit2Var(Mf_CutFunc(pCut)) );
        iLit = Kit_TruthToGia( pNew, (unsigned *)pTruth, Vec_IntSize(vLeaves), vCover, vLeaves, 0 );
        Vec_IntWriteEntry( vCopies, i, Abc_LitNotCond(iLit, Abc_LitIsCompl(Mf_CutFunc(pCut))) );
        // create mapping
        Vec_IntSetEntry( vMapping, Abc_Lit2Var(iLit), Vec_IntSize(vMapping2) );
        Vec_IntPush( vMapping2, Vec_IntSize(vLeaves) );
        Vec_IntForEachEntry( vLeaves, iLit, k )
            Vec_IntPush( vMapping2, Abc_Lit2Var(iLit) );
        Vec_IntPush( vMapping2, Abc_Lit2Var(Vec_IntEntry(vCopies, i)) );
    }
    Gia_ManForEachCo( p->pGia, pObj, i )
    {
        iLit = Vec_IntEntry( vCopies, Gia_ObjFaninId0p(p->pGia, pObj) );
        iLit = Gia_ManAppendCo( pNew, Abc_LitNotCond(iLit, Gia_ObjFaninC0(pObj)) );
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
Mf_Man_t * Mf_ManAlloc( Gia_Man_t * pGia, Jf_Par_t * pPars )
{
    Mf_Man_t * p;
    Vec_Int_t * vFlowRefs;
    int i, Entry;
    assert( pPars->nCutNum > 1  && pPars->nCutNum <= MF_CUT_MAX );
    assert( pPars->nLutSize > 1 && pPars->nLutSize <= MF_LEAF_MAX );
    ABC_FREE( pGia->pRefs );
    Vec_IntFreeP( &pGia->vMapping );
    if ( Gia_ManHasChoices(pGia) )
        Gia_ManSetPhase(pGia);
    p = ABC_CALLOC( Mf_Man_t, 1 );
    p->clkStart  = Abc_Clock();
    p->pGia      = pGia;
    p->pPars     = pPars;
    p->vTtMem    = pPars->fCutMin ? Vec_MemAllocForTT( pPars->nLutSize, 0 ) : NULL;
    p->pLfObjs   = ABC_CALLOC( Mf_Obj_t, Gia_ManObjNum(pGia) );
    p->iCur      = 2;
    Vec_PtrGrow( &p->vPages, 256 );
    if ( pPars->fGenCnf )
    {
        Vec_IntGrow( &p->vCnfSizes, 10000 );
        Vec_IntPush( &p->vCnfSizes, 1 );
        Vec_IntPush( &p->vCnfSizes, 2 );
        Vec_IntGrow( &p->vCnfMem, 10000 );
    }
    vFlowRefs = Vec_IntAlloc(0);
    Mf_ManSetFlowRefs( pGia, vFlowRefs );
    Vec_IntForEachEntry( vFlowRefs, Entry, i )
        p->pLfObjs[i].nFlowRefs = Entry;
    Vec_IntFree(vFlowRefs);
    return p;
}
void Mf_ManFree( Mf_Man_t * p )
{
    assert( !p->pPars->fGenCnf || Vec_IntSize(&p->vCnfSizes) == Vec_MemEntryNum(p->vTtMem) );
    if ( p->pPars->fCutMin )
        Vec_MemHashFree( p->vTtMem );
    if ( p->pPars->fCutMin )
        Vec_MemFree( p->vTtMem );
    Vec_PtrFreeData( &p->vPages );
    ABC_FREE( p->vCnfSizes.pArray );
    ABC_FREE( p->vCnfMem.pArray );
    ABC_FREE( p->vPages.pArray );
    ABC_FREE( p->vTemp.pArray );
    ABC_FREE( p->pLfObjs );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mf_ManSetDefaultPars( Jf_Par_t * pPars )
{
    memset( pPars, 0, sizeof(Jf_Par_t) );
    pPars->nLutSize     =  6;
    pPars->nCutNum      =  8;
    pPars->nProcNum     =  0;
    pPars->nRounds      =  2;
    pPars->nRoundsEla   =  1;
    pPars->nRelaxRatio  =  0;
    pPars->nCoarseLimit =  3;
    pPars->nAreaTuner   =  1;
    pPars->nVerbLimit   =  5;
    pPars->DelayTarget  = -1;
    pPars->fAreaOnly    =  0;
    pPars->fOptEdge     =  1; 
    pPars->fCoarsen     =  1;
    pPars->fCutMin      =  0;
    pPars->fGenCnf      =  0;
    pPars->fPureAig     =  0;
    pPars->fVerbose     =  0;
    pPars->fVeryVerbose =  0;
    pPars->nLutSizeMax  =  MF_LEAF_MAX;
    pPars->nCutNumMax   =  MF_CUT_MAX;
}
void Mf_ManPrintStats( Mf_Man_t * p, char * pTitle )
{
    if ( !p->pPars->fVerbose )
        return;
    printf( "%s :  ", pTitle );
    printf( "Level =%6lu   ",   (long)p->pPars->Delay );
    printf( "Area =%9lu   ",  (long)p->pPars->Area );
    printf( "Edge =%9lu   ",  (long)p->pPars->Edge );
    if ( p->pPars->fGenCnf )
        printf( "CNF =%9lu   ", (long)p->pPars->Clause );
    Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
    fflush( stdout );
}
void Mf_ManPrintInit( Mf_Man_t * p )
{
    if ( !p->pPars->fVerbose )
        return;
    printf( "LutSize = %d  ", p->pPars->nLutSize );
    printf( "CutNum = %d  ",  p->pPars->nCutNum );
    printf( "Iter = %d  ",    p->pPars->nRounds + p->pPars->nRoundsEla );
    printf( "Edge = %d  ",    p->pPars->fOptEdge );
    printf( "CutMin = %d  ",  p->pPars->fCutMin );
    printf( "Coarse = %d  ",  p->pPars->fCoarsen );
    printf( "CNF = %d  ",     p->pPars->fGenCnf );
    printf( "\n" );
    printf( "Computing cuts...\r" );
    fflush( stdout );
}
void Mf_ManPrintQuit( Mf_Man_t * p, Gia_Man_t * pNew )
{
    float MemGia   = Gia_ManMemory(p->pGia) / (1<<20);
    float MemMan   = 1.0 * sizeof(Mf_Obj_t) * Gia_ManObjNum(p->pGia) / (1<<20);
    float MemCuts  = 1.0 * sizeof(int) * (1 << 16) * Vec_PtrSize(&p->vPages) / (1<<20);
    float MemTt    = p->vTtMem ? Vec_MemMemory(p->vTtMem) / (1<<20) : 0;
    float MemMap   = Vec_IntMemory(pNew->vMapping) / (1<<20);
    if ( p->CutCount[0] == 0 )
        p->CutCount[0] = 1;
    if ( !p->pPars->fVerbose )
        return;
    printf( "CutPair = %.0f  ",         p->CutCount[0] );
    printf( "Merge = %.0f (%.2f %%)  ", p->CutCount[1], 100.0*p->CutCount[1]/p->CutCount[0] );
    printf( "Eval = %.0f (%.2f %%)  ",  p->CutCount[2], 100.0*p->CutCount[2]/p->CutCount[0] );
    printf( "Cut = %.0f (%.2f %%)  ",   p->CutCount[3], 100.0*p->CutCount[3]/p->CutCount[0] );
    printf( "\n" );
    printf( "Gia = %.2f MB  ",          MemGia );
    printf( "Man = %.2f MB  ",          MemMan ); 
    printf( "Cut = %.2f MB   ",         MemCuts );
    printf( "Map = %.2f MB  ",          MemMap ); 
    printf( "TT = %.2f MB  ",           MemTt ); 
    printf( "Total = %.2f MB",          MemGia + MemMan + MemCuts + MemMap + MemTt ); 
    printf( "\n" );
    if ( 1 )
    {
        int i;
        for ( i = 0; i <= p->pPars->nLutSize; i++ )
            printf( "%d = %d  ", i, p->nCutCounts[i] );
        if ( p->vTtMem )
            printf( "TT = %d (%.2f %%)  ", Vec_MemEntryNum(p->vTtMem), 100.0 * Vec_MemEntryNum(p->vTtMem) / p->CutCount[2] );
        Abc_PrintTime( 1, "Time",    Abc_Clock() - p->clkStart );
    }
    fflush( stdout );
}
void Mf_ManComputeCuts( Mf_Man_t * p )
{
    int i;
    Gia_ManForEachAndId( p->pGia, i )
        Mf_ObjMergeOrder( p, i );
    Mf_ManSetMapRefs( p );
    Mf_ManPrintStats( p, (char *)(p->fUseEla ? "Ela  " : (p->Iter ? "Area " : "Delay")) );
}

/**Function*************************************************************

  Synopsis    [Flow and area.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mf_CutRef2_rec( Mf_Man_t * p, int * pCut, Vec_Int_t * vTemp, int Limit )
{
    int i, Count = Mf_CutArea(p, Mf_CutSize(pCut), Mf_CutFunc(pCut));
    if ( Limit == 0 ) return Count;
    for ( i = 1; i <= Mf_CutSize(pCut); i++ )
    {
        Vec_IntPush( vTemp, pCut[i] );
        if ( !Mf_ObjMapRefInc(p, pCut[i]) && Mf_ManObj(p, pCut[i])->iCutSet )
            Count += Mf_CutRef2_rec( p, Mf_ObjCutBest(p, pCut[i]), vTemp, Limit-1 );
    }
    return Count;
}
static inline int Mf_CutAreaDerefed2( Mf_Man_t * p, int * pCut )
{
    int Ela1, iObj, i;
    Vec_IntClear( &p->vTemp );
    Ela1 = Mf_CutRef2_rec( p, pCut, &p->vTemp, 8 );
    Vec_IntForEachEntry( &p->vTemp, iObj, i )
        Mf_ObjMapRefDec( p, iObj );
    return Ela1;
}

int Mf_CutRef_rec( Mf_Man_t * p, int * pCut )
{
    int i, Count = Mf_CutArea(p, Mf_CutSize(pCut), Mf_CutFunc(pCut));
    for ( i = 1; i <= Mf_CutSize(pCut); i++ )
        if ( !Mf_ObjMapRefInc(p, pCut[i]) && Mf_ManObj(p, pCut[i])->iCutSet )
            Count += Mf_CutRef_rec( p, Mf_ObjCutBest(p, pCut[i]) );
    return Count;
}
int Mf_CutDeref_rec( Mf_Man_t * p, int * pCut )
{
    int i, Count = Mf_CutArea(p, Mf_CutSize(pCut), Mf_CutFunc(pCut));
    for ( i = 1; i <= Mf_CutSize(pCut); i++ )
        if ( !Mf_ObjMapRefDec(p, pCut[i]) && Mf_ManObj(p, pCut[i])->iCutSet )
            Count += Mf_CutDeref_rec( p, Mf_ObjCutBest(p, pCut[i]) );
    return Count;
}
static inline int Mf_CutAreaDerefed( Mf_Man_t * p, int * pCut )
{
    int Ela1 = Mf_CutRef_rec( p, pCut );
    int Ela2 = Mf_CutDeref_rec( p, pCut );
    assert( Ela1 == Ela2 );
    return Ela1;
}
static inline float Mf_CutFlow( Mf_Man_t * p, int * pCut, int * pTime )
{
    Mf_Obj_t * pObj;
    float Flow = 0; 
    int i, Time = 0; 
    for ( i = 1; i <= Mf_CutSize(pCut); i++ )
    {
        pObj = Mf_ManObj( p, pCut[i] );
        Time = Abc_MaxInt( Time, pObj->Delay );
        Flow += pObj->Flow;
    }
    *pTime = Time + 1;
    return Flow + Mf_CutArea(p, Mf_CutSize(pCut), Mf_CutFunc(pCut));
}
static inline void Mf_ObjComputeBestCut( Mf_Man_t * p, int iObj )
{
    Mf_Obj_t * pBest = Mf_ManObj(p, iObj);
    int * pCutSet = Mf_ObjCutSet( p, iObj );
    int * pCut, * pCutBest = NULL;
    int Value1 = -1, Value2 = -1;
    int i, Time = 0, TimeBest = ABC_INFINITY; 
    float Flow, FlowBest = ABC_INFINITY;
    if ( p->fUseEla && pBest->nMapRefs )
        Value1 = Mf_CutDeref_rec( p, Mf_ObjCutBest(p, iObj) );
    Mf_SetForEachCut( pCutSet, pCut, i )
    {
        assert( !Mf_CutIsTriv(pCut, iObj) );
        assert( Mf_CutSize(pCut) <= p->pPars->nLutSize );
        Flow = p->fUseEla ? Mf_CutAreaDerefed2(p, pCut) : Mf_CutFlow(p, pCut, &Time);
        if ( pCutBest == NULL || FlowBest > Flow + MF_EPSILON || (FlowBest > Flow - MF_EPSILON && TimeBest > Time) )
            pCutBest = pCut, FlowBest = Flow, TimeBest = Time;
    }
    assert( pCutBest != NULL );
    if ( p->fUseEla && pBest->nMapRefs )
        Value1 = Mf_CutRef_rec( p, pCutBest );
    else
        pBest->nMapRefs = 0;
    assert( Value1 >= Value2 );
    if ( p->fUseEla )
        Mf_CutFlow( p, pCutBest, &TimeBest );
    pBest->Delay = TimeBest;
    pBest->Flow  = FlowBest / Mf_ManObj(p, iObj)->nFlowRefs;
    Mf_ObjSetBestCut( pCutSet, pCutBest );
//    Mf_CutPrint( Mf_ObjCutBest(p, iObj) ); printf( "\n" );
}


/**Function*************************************************************

  Synopsis    [Technology mappping.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mf_ManComputeMapping( Mf_Man_t * p )
{
    int i;
    Gia_ManForEachAndId( p->pGia, i )
        Mf_ObjComputeBestCut( p, i );
    Mf_ManSetMapRefs( p );
    Mf_ManPrintStats( p, (char *)(p->fUseEla ? "Ela  " : (p->Iter ? "Area " : "Delay")) );
}
Gia_Man_t * Mf_ManPerformMapping( Gia_Man_t * pGia, Jf_Par_t * pPars )
{
    Mf_Man_t * p;
    Gia_Man_t * pNew, * pCls;
    if ( pPars->fGenCnf )
        pPars->fCutMin = 1;
    if ( Gia_ManHasChoices(pGia) )
        pPars->fCutMin = 1, pPars->fCoarsen = 0; 
    pCls = pPars->fCoarsen ? Gia_ManDupMuxes(pGia, pPars->nCoarseLimit) : pGia;
    p = Mf_ManAlloc( pCls, pPars );
    p->pGia0 = pGia;
    if ( pPars->fVerbose && pPars->fCoarsen )
    {
        printf( "Initial " );  Gia_ManPrintMuxStats( pGia );  printf( "\n" );
        printf( "Derived " );  Gia_ManPrintMuxStats( pCls );  printf( "\n" );
    }
    Mf_ManPrintInit( p );
    Mf_ManComputeCuts( p );
    for ( p->Iter = 1; p->Iter < p->pPars->nRounds; p->Iter++ )
        Mf_ManComputeMapping( p );
    p->fUseEla = 1;
    for ( ; p->Iter < p->pPars->nRounds + pPars->nRoundsEla; p->Iter++ )
        Mf_ManComputeMapping( p );
    if ( pPars->fVeryVerbose && pPars->fCutMin )
        Vec_MemDumpTruthTables( p->vTtMem, Gia_ManName(p->pGia), pPars->nLutSize );
    if ( pPars->fCutMin )
        pNew = Mf_ManDeriveMappingGia( p );
    else if ( pPars->fCoarsen )
        pNew = Mf_ManDeriveMappingCoarse( p );
    else
        pNew = Mf_ManDeriveMapping( p );
    if ( p->pPars->fGenCnf )
        pGia->pData = Mf_ManDeriveCnf( p, p->pPars->fCnfObjIds, p->pPars->fAddOrCla );
//    if ( p->pPars->fGenCnf )
//        Mf_ManProfileTruths( p );
    Gia_ManMappingVerify( pNew );
    Mf_ManPrintQuit( p, pNew );
    Mf_ManFree( p );
    if ( pCls != pGia )
        Gia_ManStop( pCls );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [CNF generation]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Mf_ManGenerateCnf( Gia_Man_t * pGia, int nLutSize, int fCnfObjIds, int fAddOrCla, int fMapping, int fVerbose )
{
    Gia_Man_t * pNew;
    Jf_Par_t Pars, * pPars = &Pars;
    assert( nLutSize >= 3 && nLutSize <= 8 );
    Mf_ManSetDefaultPars( pPars );
    pPars->fGenCnf     = 1;
    pPars->fCoarsen    = !fCnfObjIds;
    pPars->nLutSize    = nLutSize;
    pPars->fCnfObjIds  = fCnfObjIds;
    pPars->fAddOrCla   = fAddOrCla;
    pPars->fCnfMapping = fMapping;
    pPars->fVerbose    = fVerbose;
    pNew = Mf_ManPerformMapping( pGia, pPars );
    Gia_ManStopP( &pNew );
//    Cnf_DataPrint( (Cnf_Dat_t *)pGia->pData, 1 );
    return pGia->pData;
}
void Mf_ManDumpCnf( Gia_Man_t * p, char * pFileName, int nLutSize, int fCnfObjIds, int fAddOrCla, int fVerbose )
{
    abctime clk = Abc_Clock();
    Cnf_Dat_t * pCnf;
    pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, nLutSize, fCnfObjIds, fAddOrCla, 0, fVerbose );
    Cnf_DataWriteIntoFile( pCnf, pFileName, 0, NULL, NULL );
//    if ( fVerbose )
    {
        printf( "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d. ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    Cnf_DataFree(pCnf);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

