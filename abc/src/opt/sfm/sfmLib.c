/**CFile****************************************************************

  FileName    [sfmLib.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based optimization using internal don't-cares.]

  Synopsis    [Preprocessing genlib library.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: sfmLib.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sfmInt.h"
#include "misc/st/st.h"
#include "map/mio/mio.h"
#include "misc/vec/vecMem.h"
#include "misc/util/utilTruth.h"
#include "misc/extra/extra.h"
#include "map/mio/exp.h"
#include "opt/dau/dau.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Sfm_Fun_t_
{
    int             Next;          // next function in the list
    int             Area;          // area of this function
    char            pFansT[SFM_SUPP_MAX+1];     // top gate ID, followed by fanin perm
    char            pFansB[SFM_SUPP_MAX+1];     // bottom gate ID, followed by fanin perm
};
struct Sfm_Lib_t_
{
    int             nVars;         // variable count
    int             nWords;        // truth table words
    int             fVerbose;      // verbose statistics
    Mio_Cell2_t *   pCells;        // library gates
    int             nCells;        // library gate count
    int             fDelay;        // uses delay profile
    int             nObjs;         // object count
    int             nObjsAlloc;    // object count
    Sfm_Fun_t *     pObjs;         // objects  
    Vec_Mem_t *     vTtMem;        // truth tables
    Vec_Int_t       vLists;        // lists of funcs for each truth table
    Vec_Int_t       vCounts;       // counters of functions for each truth table
    Vec_Int_t       vHits;         // the number of times this function was used
    Vec_Int_t       vProfs;        // area/delay profiles
    Vec_Int_t       vStore;        // storage for area/delay profiles
    Vec_Int_t       vTemp;         // temporary storage for candidates
    int             nObjSkipped;
    int             nObjRemoved;
};

static inline Sfm_Fun_t * Sfm_LibFun( Sfm_Lib_t * p, int i )              { return i == -1 ? NULL : p->pObjs + i; }
static inline int         Sfm_LibFunId( Sfm_Lib_t * p, Sfm_Fun_t * pFun ) { return pFun - p->pObjs;               }

#define Sfm_LibForEachSuper( p, pObj, Func ) \
    for ( pObj = Sfm_LibFun(p, Vec_IntEntry(&p->vLists, Func)); pObj; pObj = Sfm_LibFun(p, pObj->Next) )


static word s_Truth8[8][4] = {
    { ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA),ABC_CONST(0xAAAAAAAAAAAAAAAA) },
    { ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC),ABC_CONST(0xCCCCCCCCCCCCCCCC) },
    { ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0),ABC_CONST(0xF0F0F0F0F0F0F0F0) },
    { ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00),ABC_CONST(0xFF00FF00FF00FF00) },
    { ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000),ABC_CONST(0xFFFF0000FFFF0000) },
    { ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000),ABC_CONST(0xFFFFFFFF00000000) },
    { ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF) },
    { ABC_CONST(0x0000000000000000),ABC_CONST(0x0000000000000000),ABC_CONST(0xFFFFFFFFFFFFFFFF),ABC_CONST(0xFFFFFFFFFFFFFFFF) }
};

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_DecCreateCnf( Vec_Int_t * vGateSizes, Vec_Wrd_t * vGateFuncs, Vec_Wec_t * vGateCnfs )
{
    Vec_Str_t * vCnf, * vCnfBase;
    Vec_Int_t * vCover;
    word uTruth;
    int i, nCubes;
    vCnf = Vec_StrAlloc( 100 );
    vCover = Vec_IntAlloc( 100 );
    Vec_WrdForEachEntry( vGateFuncs, uTruth, i )
    {
        nCubes = Sfm_TruthToCnf( uTruth, Vec_IntEntry(vGateSizes, i), vCover, vCnf );
        vCnfBase = (Vec_Str_t *)Vec_WecEntry( vGateCnfs, i );
        Vec_StrGrow( vCnfBase, Vec_StrSize(vCnf) );
        memcpy( Vec_StrArray(vCnfBase), Vec_StrArray(vCnf), Vec_StrSize(vCnf) );
        vCnfBase->nSize = Vec_StrSize(vCnf);
    }
    Vec_IntFree( vCover );
    Vec_StrFree( vCnf );
}

/**Function*************************************************************

  Synopsis    [Preprocess the library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Sfm_LibPreprocess( Mio_Library_t * pLib, Vec_Int_t * vGateSizes, Vec_Wrd_t * vGateFuncs, Vec_Wec_t * vGateCnfs, Vec_Ptr_t * vGateHands )
{
    Mio_Gate_t * pGate;
    int nGates = Mio_LibraryReadGateNum(pLib);
    Vec_IntGrow( vGateSizes, nGates );
    Vec_WrdGrow( vGateFuncs, nGates );
    Vec_WecInit( vGateCnfs,  nGates );
    Vec_PtrGrow( vGateHands, nGates );
    Mio_LibraryForEachGate( pLib, pGate )
    {
        Vec_IntPush( vGateSizes, Mio_GateReadPinNum(pGate) );
        Vec_WrdPush( vGateFuncs, Mio_GateReadTruth(pGate) );
        Mio_GateSetValue( pGate, Vec_PtrSize(vGateHands) );
        Vec_PtrPush( vGateHands, pGate );
    }
    Sfm_DecCreateCnf( vGateSizes, vGateFuncs, vGateCnfs );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_LibFindComplInputGate( Vec_Wrd_t * vFuncs, int iGate, int nFanins, int iFanin, int * piFaninNew )
{
    word uTruthGate = Vec_WrdEntry( vFuncs, iGate );
    word uTruthFlip = Abc_Tt6Flip( uTruthGate, iFanin ); 
    word uTruth, uTruthSwap;  int i;
    assert( iFanin >= 0 && iFanin < nFanins );
    if ( piFaninNew ) *piFaninNew = iFanin;
    Vec_WrdForEachEntry( vFuncs, uTruth, i )
        if ( uTruth == uTruthFlip )
            return i;
    if ( iFanin-1 >= 0 )
    {
        if ( piFaninNew ) *piFaninNew = iFanin-1;
        uTruthSwap = Abc_Tt6SwapAdjacent( uTruthFlip, iFanin-1 );
        Vec_WrdForEachEntry( vFuncs, uTruth, i )
            if ( uTruth == uTruthSwap )
                return i;
    }
    if ( iFanin+1 < nFanins )
    {
        if ( piFaninNew ) *piFaninNew = iFanin+1;
        uTruthSwap = Abc_Tt6SwapAdjacent( uTruthFlip, iFanin );
        Vec_WrdForEachEntry( vFuncs, uTruth, i )
            if ( uTruth == uTruthSwap )
                return i;
    }
    // add checking for complemeting control input of a MUX
    if ( piFaninNew ) *piFaninNew = -1;
    return -1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Sfm_Lib_t * Sfm_LibStart( int nVars, int fDelay, int fVerbose )
{
    Sfm_Lib_t * p = ABC_CALLOC( Sfm_Lib_t, 1 );
    assert( nVars <= SFM_SUPP_MAX );
    p->vTtMem = Vec_MemAllocForTT( nVars, 0 );   
    Vec_IntGrow( &p->vLists,  (1 << 16) );
    Vec_IntGrow( &p->vCounts, (1 << 16) );
    Vec_IntGrow( &p->vHits,   (1 << 16) );
    Vec_IntFill( &p->vLists,  2, -1 );
    Vec_IntFill( &p->vCounts, 2, -1 );
    Vec_IntFill( &p->vHits,   2, -1 );
    p->nObjsAlloc = (1 << 16);
    p->pObjs = ABC_CALLOC( Sfm_Fun_t, p->nObjsAlloc );
    p->fDelay = fDelay;
    if ( fDelay ) Vec_IntGrow( &p->vProfs,  (1 << 16) );
    if ( fDelay ) Vec_IntGrow( &p->vStore,  (1 << 18) );
    Vec_IntGrow( &p->vTemp, 16 );
    p->nVars = nVars;
    p->nWords = Abc_TtWordNum( nVars );
    p->fVerbose = fVerbose;
    return p;
}
void Sfm_LibStop( Sfm_Lib_t * p )
{
    Vec_MemHashFree( p->vTtMem );
    Vec_MemFree( p->vTtMem );
    Vec_IntErase( &p->vLists );
    Vec_IntErase( &p->vCounts );
    Vec_IntErase( &p->vHits );
    Vec_IntErase( &p->vProfs );
    Vec_IntErase( &p->vStore );
    Vec_IntErase( &p->vTemp );
    ABC_FREE( p->pCells );
    ABC_FREE( p->pObjs );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Sfm_LibTruth6Two( Mio_Cell2_t * pCellBot, Mio_Cell2_t * pCellTop, int InTop )
{
    word uFanins[SFM_SUPP_MAX]; int i, k;
    word uTruthBot = Exp_Truth6( pCellBot->nFanins, pCellBot->vExpr, NULL );
    assert( InTop >= 0 && InTop < (int)pCellTop->nFanins );
    for ( i = 0, k = pCellBot->nFanins; i < (int)pCellTop->nFanins; i++ )
        if ( i == InTop )
            uFanins[i] = uTruthBot;
        else
            uFanins[i] = s_Truths6[k++];
    assert( (int)pCellBot->nFanins + (int)pCellTop->nFanins == k + 1 );
    uTruthBot = Exp_Truth6( pCellTop->nFanins, pCellTop->vExpr, uFanins );
    return uTruthBot;
}
void Sfm_LibTruth8Two( Mio_Cell2_t * pCellBot, Mio_Cell2_t * pCellTop, int InTop, word * pRes )
{
    word uTruthBot[4], * puFanins[SFM_SUPP_MAX]; int i, k;
    Exp_Truth8( pCellBot->nFanins, pCellBot->vExpr, NULL, uTruthBot );
    assert( InTop >= 0 && InTop < (int)pCellTop->nFanins );
    for ( i = 0, k = pCellBot->nFanins; i < (int)pCellTop->nFanins; i++ )
        if ( i == InTop )
            puFanins[i] = uTruthBot;
        else
            puFanins[i] = s_Truth8[k++];
    assert( (int)pCellBot->nFanins + (int)pCellTop->nFanins == k + 1 );
    Exp_Truth8( pCellTop->nFanins, pCellTop->vExpr, puFanins, pRes );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
void Sfm_LibCellProfile( Mio_Cell2_t * pCellBot, Mio_Cell2_t * pCellTop, int InTop, int nFanins, int * Perm, int * pProf )
{
    int i, DelayAdd = pCellTop ? pCellTop->iDelays[InTop] : 0;
    for ( i = 0; i < nFanins; i++ )
        if ( Perm[i] < (int)pCellBot->nFanins )
            pProf[i] = pCellBot->iDelays[Perm[i]] + DelayAdd;
        else if ( Perm[i] < (int)pCellBot->nFanins + InTop )
            pProf[i] = pCellTop->iDelays[Perm[i] - (int)pCellBot->nFanins];
        else // if ( Perm[i] >= (int)pCellBot->nFanins + InTop )
            pProf[i] = pCellTop->iDelays[Perm[i] - (int)pCellBot->nFanins + 1];
}
*/
void Sfm_LibCellProfile( Mio_Cell2_t * pCellBot, Mio_Cell2_t * pCellTop, int InTop, int nFanins, int * Perm, int * pProf )
{
    int i, DelayAdd = pCellTop ? 1 : 0;
    for ( i = 0; i < nFanins; i++ )
        if ( Perm[i] < (int)pCellBot->nFanins )
            pProf[i] = 1 + DelayAdd;
        else if ( Perm[i] < (int)pCellBot->nFanins + InTop )
            pProf[i] = 1;
        else // if ( Perm[i] >= (int)pCellBot->nFanins + InTop )
            pProf[i] = 1;
}
static inline int Sfm_LibNewIsContained( Sfm_Fun_t * pObj, int * pProf, int Area, int * pProfNew, int nFanins )
{
    int k;
    if ( Area < pObj->Area )
        return 0;
    for ( k = 0; k < nFanins; k++ )
        if ( pProfNew[k] < pProf[k] )
            return 0;
    return 1;
}
static inline int Sfm_LibNewContains( Sfm_Fun_t * pObj, int * pProf, int Area, int * pProfNew, int nFanins )
{
    int k;
    if ( Area > pObj->Area )
        return 0;
    for ( k = 0; k < nFanins; k++ )
        if ( pProfNew[k] > pProf[k] )
            return 0;
    return 1;
}
void Sfm_LibPrepareAdd( Sfm_Lib_t * p, word * pTruth, int * Perm, int nFanins, Mio_Cell2_t * pCellBot, Mio_Cell2_t * pCellTop, int InTop )
{
    Sfm_Fun_t * pObj;
    int InvPerm[SFM_SUPP_MAX], Profile[SFM_SUPP_MAX];
    int Area = (int)pCellBot->AreaW + (pCellTop ? (int)pCellTop->AreaW : 0);
    int i, k, Id, Prev, Offset, * pProf, iFunc = Vec_MemHashInsert( p->vTtMem, pTruth );
    if ( iFunc == Vec_IntSize(&p->vLists) )
    {
        Vec_IntPush( &p->vLists, -1 );
        Vec_IntPush( &p->vCounts, 0 );
        Vec_IntPush( &p->vHits,   0 );
    }
    assert( pCellBot != NULL );
    // iterate through the supergates of this truth table
    if ( p->fDelay )
    {
        assert( Vec_IntSize(&p->vProfs) == p->nObjs );
        Sfm_LibCellProfile( pCellBot, pCellTop, InTop, nFanins, Perm, Profile );
        // check if new one is contained in old ones
        Vec_IntClear( &p->vTemp );
        Sfm_LibForEachSuper( p, pObj, iFunc )
        {
            Vec_IntPush( &p->vTemp, Sfm_LibFunId(p, pObj) );
            Offset = Vec_IntEntry( &p->vProfs, Sfm_LibFunId(p, pObj) );
            pProf  = Vec_IntEntryP( &p->vStore, Offset );
            if ( Sfm_LibNewIsContained(pObj, pProf, Area, Profile, nFanins) )
            {
                p->nObjSkipped++;
                return;
            }
        }
        // check if old ones are contained in new one
        k = 0;
        Vec_IntForEachEntry( &p->vTemp, Id, i )
        {
            Offset = Vec_IntEntry( &p->vProfs, Id );
            pProf  = Vec_IntEntryP( &p->vStore, Offset );
            if ( !Sfm_LibNewContains(Sfm_LibFun(p, Id), pProf, Area, Profile, nFanins) )
                Vec_IntWriteEntry( &p->vTemp, k++, Id );
            else
                p->nObjRemoved++;
        }
        if ( k < i ) // change
        {
            if ( k == 0 )
                Vec_IntWriteEntry( &p->vLists, iFunc, -1 );
            else
            {
                Vec_IntShrink( &p->vTemp, k );
                Prev = Vec_IntEntry(&p->vTemp, 0);
                Vec_IntWriteEntry( &p->vLists, iFunc, Prev );
                Vec_IntForEachEntryStart( &p->vTemp, Id, i, 1 )
                {
                    Sfm_LibFun(p, Prev)->Next = Id;
                    Prev = Id;
                }
                Sfm_LibFun(p, Prev)->Next = -1;
            }
        }
    }
    else
    {
        Sfm_LibForEachSuper( p, pObj, iFunc )
        {
            if ( Area >= pObj->Area )
                return;
        }
    }
    for ( k = 0; k < nFanins; k++ )
        InvPerm[Perm[k]] = k;
    // create delay profile
    if ( p->fDelay )
    {
        Vec_IntPush( &p->vProfs, Vec_IntSize(&p->vStore) );
        for ( k = 0; k < nFanins; k++ )
            Vec_IntPush( &p->vStore, Profile[k] );
    }
    // create new object
    if ( p->nObjs == p->nObjsAlloc )
    {
        int nObjsAlloc = 2 * p->nObjsAlloc;
        p->pObjs = ABC_REALLOC( Sfm_Fun_t, p->pObjs, nObjsAlloc );
        memset( p->pObjs + p->nObjsAlloc, 0, sizeof(Sfm_Fun_t) * p->nObjsAlloc );
        p->nObjsAlloc = nObjsAlloc;
    }
    pObj = p->pObjs + p->nObjs;
    pObj->Area = Area;
    pObj->Next = Vec_IntEntry(&p->vLists, iFunc);
    Vec_IntWriteEntry( &p->vLists, iFunc, p->nObjs++ );
    Vec_IntAddToEntry( &p->vCounts, iFunc, 1 );
    // create gate
    assert( pCellBot->Id < 128 );
    pObj->pFansB[0] = (char)pCellBot->Id;
    for ( k = 0; k < (int)pCellBot->nFanins; k++ )
        pObj->pFansB[k+1] = InvPerm[k];
    if ( pCellTop == NULL )
        return;
    assert( pCellTop->Id < 128 );
    pObj->pFansT[0] = (char)pCellTop->Id;
    for ( i = 0; i < (int)pCellTop->nFanins; i++ )
        pObj->pFansT[i+1] = (char)(i == InTop ? 16 : InvPerm[k++]);
    assert( k == nFanins );
}
Sfm_Lib_t * Sfm_LibPrepare( int nVars, int fTwo, int fDelay, int fVerbose, int fLibVerbose )
{
    abctime clk = Abc_Clock();
    Sfm_Lib_t * p = Sfm_LibStart( nVars, fDelay, fLibVerbose );
    Mio_Cell2_t * pCell1, * pCell2, * pLimit;
    int * pPerm[SFM_SUPP_MAX+1], * Perm1, * Perm2, Perm[SFM_SUPP_MAX];
    int nPerms[SFM_SUPP_MAX+1], i, f, n;
    word tTemp1[4], tCur[4];
    char pRes[1000];
    assert( nVars <= SFM_SUPP_MAX );
    // precompute gates
    p->pCells = Mio_CollectRootsNewDefault2( Abc_MinInt(6, nVars), &p->nCells, 0 );
    pLimit = p->pCells + p->nCells;
    // find useful ones
    for ( pCell1 = p->pCells + 4; pCell1 < pLimit; pCell1++ )
    {
        word uTruth = pCell1->uTruth;
        pCell1->Type = 0;
        if ( Abc_Tt6IsAndType(uTruth, pCell1->nFanins) || Abc_Tt6IsOrType(uTruth, pCell1->nFanins) )
            pCell1->Type = 1;
        else if ( Dau_DsdDecompose(&uTruth, pCell1->nFanins, 0, 0, pRes) <= 3 )
            pCell1->Type = 2;
        else if ( fLibVerbose )
            printf( "Skipping gate \"%s\" with non-DSD function %s\n", pCell1->pName, pRes );
    }
    // generate permutations
    for ( i = 2; i <= nVars; i++ )
        pPerm[i] = Extra_PermSchedule( i );
    for ( i = 2; i <= nVars; i++ )
        nPerms[i] = Extra_Factorial( i );
    // add single cells
    for ( pCell1 = p->pCells + 4; pCell1 < pLimit; pCell1++ ) 
    {
        int nFanins = pCell1->nFanins;
        assert( nFanins >= 2 && nFanins <= nVars );
        for ( i = 0; i < nFanins; i++ )
            Perm[i] = i;
        // permute truth table
        tCur[0] = tTemp1[0] = pCell1->uTruth;
        if ( p->nVars > 6 )
            tTemp1[1] = tTemp1[2] = tTemp1[3] = tCur[1] = tCur[2] = tCur[3] = tCur[0];
        for ( n = 0; n < nPerms[nFanins]; n++ )
        {
            Sfm_LibPrepareAdd( p, tCur, Perm, nFanins, pCell1, NULL, -1 );
            // update
            Abc_TtSwapAdjacent( tCur, p->nWords, pPerm[nFanins][n] );
            Perm1 = Perm + pPerm[nFanins][n];
            Perm2 = Perm1 + 1;
            ABC_SWAP( int, *Perm1, *Perm2 );
        }
        assert( Abc_TtEqual(tTemp1, tCur, p->nWords) );
    }
    // add double cells
    if ( fTwo )
    for ( pCell1 = p->pCells + 4; pCell1 < pLimit; pCell1++ ) // Bot
    if ( pCell1->Type > 0 )
    for ( pCell2 = p->pCells + 4; pCell2 < pLimit; pCell2++ ) // Top
    if ( pCell2->Type > 0 )//&& pCell1->Type + pCell2->Type <= 2 )
    if ( (int)pCell1->nFanins + (int)pCell2->nFanins <= nVars + 1 )
    for ( f = 0; f < (int)pCell2->nFanins; f++ )
    {
        int nFanins = pCell1->nFanins + pCell2->nFanins - 1;
        assert( nFanins >= 2 && nFanins <= nVars );
        for ( i = 0; i < nFanins; i++ )
            Perm[i] = i;
        // permute truth table
        if ( p->nVars > 6 )
        {
            Sfm_LibTruth8Two( pCell1, pCell2, f, tCur );
            Abc_TtCopy( tTemp1, tCur, p->nWords, 0 );
        }
        else
            tCur[0] = tTemp1[0] = Sfm_LibTruth6Two( pCell1, pCell2, f );
        for ( n = 0; n < nPerms[nFanins]; n++ )
        {
            Sfm_LibPrepareAdd( p, tCur, Perm, nFanins, pCell1, pCell2, f );
            if ( nFanins > 5 )
                break;
            // update
            Abc_TtSwapAdjacent( tCur, p->nWords, pPerm[nFanins][n] );
            Perm1 = Perm + pPerm[nFanins][n];
            Perm2 = Perm1 + 1;
            ABC_SWAP( int, *Perm1, *Perm2 );
        }
        assert( Abc_TtEqual(tTemp1, tCur, p->nWords) );
    }
    // cleanup
    for ( i = 2; i <= nVars; i++ )
        ABC_FREE( pPerm[i] );
    if ( fVerbose )
    {
        printf( "Library processing: Var = %d. Cell = %d.  Fun = %d. Obj = %d. Ave = %.2f.  Skip = %d. Rem = %d.  ", 
            nVars, p->nCells, Vec_MemEntryNum(p->vTtMem)-2, 
            p->nObjs-p->nObjRemoved, 1.0*(p->nObjs-p->nObjRemoved)/(Vec_MemEntryNum(p->vTtMem)-2), 
            p->nObjSkipped, p->nObjRemoved );
        Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
    }
    return p;
}
void Sfm_LibPrintGate( Mio_Cell2_t * pCell, char * pFanins, Mio_Cell2_t * pCell2, char * pFanins2 )
{
    int k;
    printf( " %-20s(", pCell->pName );
    for ( k = 0; k < (int)pCell->nFanins; k++ )
        if ( pFanins[k] == (char)16 )
            Sfm_LibPrintGate( pCell2, pFanins2, NULL, NULL );
        else
            printf( " %c", 'a' + pFanins[k] );
    printf( " )" );
}
void Sfm_LibPrintObj( Sfm_Lib_t * p, Sfm_Fun_t * pObj )
{
    Mio_Cell2_t * pCellB = p->pCells + (int)pObj->pFansB[0];
    Mio_Cell2_t * pCellT = p->pCells + (int)pObj->pFansT[0];
    int i, nFanins = pCellB->nFanins + (pCellT == p->pCells ? 0 : pCellT->nFanins - 1);
    printf( "F = %d  A =%6.2f  ", nFanins, Scl_Int2Flt(pObj->Area) );
    if ( pCellT == p->pCells )
        Sfm_LibPrintGate( pCellB, pObj->pFansB + 1, NULL, NULL );
    else
        Sfm_LibPrintGate( pCellT, pObj->pFansT + 1, pCellB, pObj->pFansB + 1 );
    // get hold of delay info
    if ( p->fDelay )
    {
        int Offset  = Vec_IntEntry( &p->vProfs, Sfm_LibFunId(p, pObj) );
        int * pProf = Vec_IntEntryP( &p->vStore, Offset );
        for ( i = 0; i < nFanins; i++ )
            printf( "%6.2f ", Scl_Int2Flt(pProf[i]) );
    }
}
void Sfm_LibPrint( Sfm_Lib_t * p )
{
    Sfm_Fun_t * pObj; word * pTruth; int i, nFanins;   
    Vec_MemForEachEntry( p->vTtMem, pTruth, i )
    {
        if ( i < 2 || Vec_IntEntry(&p->vHits, i) == 0 )
            continue;
        nFanins = Abc_TtSupportSize(pTruth, p->nVars);
        printf( "%8d : ", i );
        printf( "Num =%5d  ", Vec_IntEntry(&p->vCounts, i) );
        printf( "Hit =%4d  ", Vec_IntEntry(&p->vHits, i) );
        Sfm_LibForEachSuper( p, pObj, i )
        {
            Sfm_LibPrintObj( p, pObj );
            break;
        }
        printf( "    " );
        Dau_DsdPrintFromTruth( pTruth, nFanins );
    }
}
void Sfm_LibTest()
{
    Sfm_Lib_t * p;
    int fVerbose = 1;
    if ( Abc_FrameReadLibGen() == NULL )
    {
        printf( "There is no current library.\n" );
        return;
    }
    p = Sfm_LibPrepare( 7, 1, 1, 1, fVerbose );
    if ( fVerbose )
        Sfm_LibPrint( p );
    Sfm_LibStop( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_LibFindAreaMatch( Sfm_Lib_t * p, word * pTruth, int nFanins, int * piObj )
{
    Sfm_Fun_t * pObj = NULL;
    int iFunc = *Vec_MemHashLookup( p->vTtMem,  pTruth );
    if ( iFunc == -1 )
        return -1;
    Sfm_LibForEachSuper( p, pObj, iFunc )
        break;
    if ( piObj )
        *piObj = pObj - p->pObjs;
    return pObj->Area;
}
int Sfm_LibFindDelayMatches( Sfm_Lib_t * p, word * pTruth, int * pFanins, int nFanins, Vec_Ptr_t * vGates, Vec_Ptr_t * vFans )
{
    Sfm_Fun_t * pObj;
    Mio_Cell2_t * pCellB, * pCellT;
    int iFunc;
    if ( nFanins > 6 )
    {
        word pCopy[4];
        Abc_TtCopy( pCopy, pTruth, 4, 0 );
        Dau_DsdPrintFromTruth( pCopy, p->nVars );
    }
    Vec_PtrClear( vGates );
    Vec_PtrClear( vFans );
    // look for gate
    assert( !Abc_TtIsConst0(pTruth, p->nWords) && 
            !Abc_TtIsConst1(pTruth, p->nWords) && 
            !Abc_TtEqual(pTruth, s_Truth8[0], p->nWords) && 
            !Abc_TtOpposite(pTruth, s_Truth8[0], p->nWords) );
    iFunc = *Vec_MemHashLookup( p->vTtMem,  pTruth );
    if ( iFunc == -1 )
    {
        // print functions not found in the library
        if ( p->fVerbose || nFanins > 6 )
        {
            printf( "Not found in the precomputed library: " );
            Dau_DsdPrintFromTruth( pTruth, nFanins );
        }
        return 0;
    }
    Vec_IntAddToEntry( &p->vHits, iFunc, 1 );
    // collect matches
    Sfm_LibForEachSuper( p, pObj, iFunc )
    {
        pCellB = p->pCells + (int)pObj->pFansB[0];
        pCellT = p->pCells + (int)pObj->pFansT[0];
        Vec_PtrPush( vGates, pCellB->pMioGate );
        Vec_PtrPush( vGates, pCellT == p->pCells ? NULL : pCellT->pMioGate );
        Vec_PtrPush( vFans, pObj->pFansB + 1 );
        Vec_PtrPush( vFans, pCellT == p->pCells ? NULL : pObj->pFansT + 1 );
    }
    return Vec_PtrSize(vGates) / 2;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Sfm_LibImplementSimple( Sfm_Lib_t * p, word * pTruth, int * pFanins, int nFanins, Vec_Int_t * vGates, Vec_Wec_t * vFanins )
{
    Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
    Mio_Gate_t * pGate;
    Vec_Int_t * vLevel;
    if ( Abc_TtIsConst0(pTruth, p->nWords) || Abc_TtIsConst1(pTruth, p->nWords) )
    {
        assert( nFanins == 0 );
        pGate = Abc_TtIsConst1(pTruth, p->nWords) ? Mio_LibraryReadConst1(pLib) : Mio_LibraryReadConst0(pLib);
        Vec_IntPush( vGates, Mio_GateReadValue(pGate) );
        vLevel = Vec_WecPushLevel( vFanins );
        return 1;
    }
    if ( Abc_TtEqual(pTruth, s_Truth8[0], p->nWords) || Abc_TtOpposite(pTruth, s_Truth8[0], p->nWords) )
    {
        assert( nFanins == 1 );
        pGate = Abc_TtEqual(pTruth, s_Truth8[0], p->nWords) ? Mio_LibraryReadBuf(pLib) : Mio_LibraryReadInv(pLib);
        Vec_IntPush( vGates, Mio_GateReadValue(pGate) );
        vLevel = Vec_WecPushLevel( vFanins );
        Vec_IntPush( vLevel, pFanins[0] );
        return 1;
    }
    assert( 0 );
    return -1;
}
int Sfm_LibImplementGatesArea( Sfm_Lib_t * p, int * pFanins, int nFanins, int iObj, Vec_Int_t * vGates, Vec_Wec_t * vFanins )
{
    Mio_Library_t * pLib = (Mio_Library_t *)Abc_FrameReadLibGen();
    Sfm_Fun_t * pObjMin = p->pObjs + iObj;
    Mio_Cell2_t * pCellB, * pCellT;
    Mio_Gate_t * pGate;
    Vec_Int_t * vLevel;
    int i;
    // get the gates
    pCellB = p->pCells + (int)pObjMin->pFansB[0];
    pCellT = p->pCells + (int)pObjMin->pFansT[0];
    // create bottom gate
    pGate = Mio_LibraryReadGateByName( pLib, pCellB->pName, NULL );
    assert( pGate == pCellB->pMioGate );
    Vec_IntPush( vGates, Mio_GateReadValue(pGate) );
    vLevel = Vec_WecPushLevel( vFanins );
    for ( i = 0; i < (int)pCellB->nFanins; i++ )
        Vec_IntPush( vLevel, pFanins[(int)pObjMin->pFansB[i+1]] );
    if ( pCellT == p->pCells )
        return 1;
    // create top gate
    pGate = Mio_LibraryReadGateByName( pLib, pCellT->pName, NULL );
    assert( pGate == pCellT->pMioGate );
    Vec_IntPush( vGates, Mio_GateReadValue(pGate) );
    vLevel = Vec_WecPushLevel( vFanins );
    for ( i = 0; i < (int)pCellT->nFanins; i++ )
        if ( pObjMin->pFansT[i+1] == (char)16 )
            Vec_IntPush( vLevel, Vec_WecSize(vFanins)-2 );
        else
            Vec_IntPush( vLevel, pFanins[(int)pObjMin->pFansT[i+1]] );
    return 2;
}
int Sfm_LibImplementGatesDelay( Sfm_Lib_t * p, int * pFanins, Mio_Gate_t * pGateB, Mio_Gate_t * pGateT, char * pFansB, char * pFansT, Vec_Int_t * vGates, Vec_Wec_t * vFanins )
{
    Vec_Int_t * vLevel;
    int i, nFanins;
    // create bottom gate
    Vec_IntPush( vGates, Mio_GateReadValue(pGateB) );
    vLevel  = Vec_WecPushLevel( vFanins );
    nFanins = Mio_GateReadPinNum( pGateB );
    for ( i = 0; i < nFanins; i++ )
        Vec_IntPush( vLevel, pFanins[(int)pFansB[i]] );
    if ( pGateT == NULL )
        return 1;
    // create top gate
    Vec_IntPush( vGates, Mio_GateReadValue(pGateT) );
    vLevel  = Vec_WecPushLevel( vFanins );
    nFanins = Mio_GateReadPinNum( pGateT );
    for ( i = 0; i < nFanins; i++ )
        if ( pFansT[i] == (char)16 )
            Vec_IntPush( vLevel, Vec_WecSize(vFanins)-2 );
        else
            Vec_IntPush( vLevel, pFanins[(int)pFansT[i]] );
    return 2;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

