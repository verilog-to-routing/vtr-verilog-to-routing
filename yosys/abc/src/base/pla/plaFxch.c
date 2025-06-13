/**CFile****************************************************************

  FileName    [plaFxch.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SOP manager.]

  Synopsis    [Scalable SOP transformations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - March 18, 2015.]

  Revision    [$Id: plaFxch.c,v 1.00 2014/09/12 00:00:00 alanmi Exp $]

***********************************************************************/

#include "pla.h"
#include "misc/vec/vecHash.h"
#include "misc/vec/vecQue.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Fxch_Obj_t_ Fxch_Obj_t;
struct Fxch_Obj_t_
{
    unsigned        Table : 30;
    unsigned        MarkN :  1;
    unsigned        MarkP :  1;
    int             Next;
    int             Prev;
    int             Cube;
};

typedef struct Fxch_Man_t_ Fxch_Man_t;
struct Fxch_Man_t_
{
    // user's data
    Vec_Wec_t       vCubes;     // cube -> lit
    // internal data
    Vec_Wec_t       vLits;      // lit -> cube
    Vec_Int_t       vRands;     // random numbers for each literal
    Vec_Int_t       vCubeLinks; // first link for each cube
    Fxch_Obj_t *    pBins;      // hash table (lits -> cube + lit)
    Hash_IntMan_t * vHash;      // divisor hash table
    Vec_Que_t *     vPrio;      // priority queue for divisors by weight
    Vec_Flt_t       vWeights;   // divisor weights
    Vec_Wec_t       vPairs;     // cube pairs for each div
    Vec_Wrd_t       vDivs;      // extracted divisors
    // temporary data 
    Vec_Int_t       vCubesS;    // cube pairs for single
    Vec_Int_t       vCubesD;    // cube pairs for double
    Vec_Int_t       vCube1;     // first cube
    Vec_Int_t       vCube2;     // second cube
    // statistics 
    abctime         timeStart;  // starting time
    int             SizeMask;   // hash table mask
    int             nVars;      // original problem variables
    int             nLits;      // the number of SOP literals
    int             nCompls;    // the number of complements
    int             nPairsS;    // number of lit pairs
    int             nPairsD;    // number of cube pairs
};

#define Fxch_ManForEachCubeVec( vVec, vCubes, vCube, i )           \
    for ( i = 0; (i < Vec_IntSize(vVec)) && ((vCube) = Vec_WecEntry(vCubes, Vec_IntEntry(vVec, i))); i++ )

static inline Vec_Int_t * Fxch_ManCube( Fxch_Man_t * p, int hCube ) { return Vec_WecEntry(&p->vCubes, Pla_CubeNum(hCube)); }


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writes the current state of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxch_ManWriteBlif( char * pFileName, Vec_Wec_t * vCubes, Vec_Wrd_t * vDivs )
{
    // find the number of original variables
    int nVarsInit = Vec_WrdCountZero(vDivs);
    FILE * pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
    else
    {
        //char * pLits = "-01?";
        Vec_Str_t * vStr;
        Vec_Int_t * vCube;
        int i, k, Lit;
        word Div;
        // comment
        fprintf( pFile, "# BLIF file written via PLA package in ABC on " );
        fprintf( pFile, "%s", Extra_TimeStamp() );
        fprintf( pFile, "\n\n" );
        // header
        fprintf( pFile, ".model %s\n", pFileName );
        fprintf( pFile, ".inputs" );
        for ( i = 0; i < nVarsInit; i++ )
            fprintf( pFile, " i%d", i );
        fprintf( pFile, "\n" );
        fprintf( pFile, ".outputs o" );
        fprintf( pFile, "\n" );
        // SOP header
        fprintf( pFile, ".names" );
        for ( i = 0; i < Vec_WrdSize(vDivs); i++ )
            fprintf( pFile, " i%d", i );
        fprintf( pFile, " o\n" );
        // SOP cubes
        vStr = Vec_StrStart( Vec_WrdSize(vDivs) + 1 );
        Vec_WecForEachLevel( vCubes, vCube, i )
        {
            if ( !Vec_IntSize(vCube) )
                continue;
            for ( k = 0; k < Vec_WrdSize(vDivs); k++ )
                Vec_StrWriteEntry( vStr, k, '-' );
            Vec_IntForEachEntry( vCube, Lit, k )
                Vec_StrWriteEntry( vStr, Abc_Lit2Var(Lit), (char)(Abc_LitIsCompl(Lit) ? '0' : '1') );
            fprintf( pFile, "%s 1\n", Vec_StrArray(vStr) );
        }
        Vec_StrFree( vStr );
        // divisors
        Vec_WrdForEachEntryStart( vDivs, Div, i, nVarsInit )
        {
            int pLits[2] = { (int)(Div & 0xFFFFFFFF), (int)(Div >> 32) };
            fprintf( pFile, ".names" );
            fprintf( pFile, " i%d", Abc_Lit2Var(pLits[0]) );
            fprintf( pFile, " i%d", Abc_Lit2Var(pLits[1]) );
            fprintf( pFile, " i%d\n", i );
            fprintf( pFile, "%d%d 1\n", !Abc_LitIsCompl(pLits[0]), !Abc_LitIsCompl(pLits[1]) );
        }
        fprintf( pFile, ".end\n\n" );
        fclose( pFile );
        printf( "Written BLIF file \"%s\".\n", pFileName );
    }
}

/**Function*************************************************************

  Synopsis    [Starting the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fxch_Man_t * Fxch_ManStart( Vec_Wec_t * vCubes, Vec_Wec_t * vLits )
{
    Vec_Int_t * vCube;  int i, LogSize;
    Fxch_Man_t * p = ABC_CALLOC( Fxch_Man_t, 1 );
    p->vCubes = *vCubes;
    p->vLits  = *vLits;
    p->nVars  = Vec_WecSize(vLits)/2;
    p->nLits  = 0;
    // random numbers
    Gia_ManRandom( 1 );
    Vec_IntGrow( &p->vRands, 2*p->nVars );
    for ( i = 0; i < 2*p->nVars; i++ )
        Vec_IntPush( &p->vRands, Gia_ManRandom(0) & 0x3FFFFFF ); // assert( LogSize <= 26 );
    // create cube links
    Vec_IntGrow( &p->vCubeLinks, Vec_WecSize(&p->vCubes) );
    Vec_WecForEachLevel( vCubes, vCube, i )
    {
        Vec_IntPush( &p->vCubeLinks, p->nLits+1 );
        p->nLits += Vec_IntSize(vCube);
    }
    assert( Vec_IntSize(&p->vCubeLinks) == Vec_WecSize(&p->vCubes) );
    // create table
    LogSize = Abc_Base2Log( p->nLits+1 );
    assert( LogSize <= 26 );
    p->SizeMask = (1 << LogSize) - 1;
    p->pBins = ABC_CALLOC( Fxch_Obj_t, p->SizeMask + 1 );
    assert( p->nLits+1 < p->SizeMask+1 );
    // divisor weights and cube pairs
    Vec_FltGrow( &p->vWeights, 1000 );
    Vec_FltPush( &p->vWeights, -1 );
    Vec_WecGrow( &p->vPairs, 1000 );
    Vec_WecPushLevel( &p->vPairs );
    // prepare divisors
    Vec_WrdGrow( &p->vDivs, p->nVars + 1000 );
    Vec_WrdFill( &p->vDivs, p->nVars, 0 );
    return p;
}
void Fxch_ManStop( Fxch_Man_t * p )
{
    Vec_WecErase( &p->vCubes );
    Vec_WecErase( &p->vLits );
    Vec_IntErase( &p->vRands );
    Vec_IntErase( &p->vCubeLinks );
    Hash_IntManStop( p->vHash );
    Vec_QueFree( p->vPrio );
    Vec_FltErase( &p->vWeights );
    Vec_WecErase( &p->vPairs );
    Vec_WrdErase( &p->vDivs );
    Vec_IntErase( &p->vCubesS );
    Vec_IntErase( &p->vCubesD );
    Vec_IntErase( &p->vCube1 );
    Vec_IntErase( &p->vCube2 );
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Fxch_TabCompare( Fxch_Man_t * p, int hCube1, int hCube2 )
{
    Vec_Int_t * vCube1 = Fxch_ManCube( p, hCube1 );
    Vec_Int_t * vCube2 = Fxch_ManCube( p, hCube2 );
    if ( !Vec_IntSize(vCube1) || !Vec_IntSize(vCube2) || Vec_IntSize(vCube1) != Vec_IntSize(vCube2) )
        return 0;
    Vec_IntClear( &p->vCube1 );
    Vec_IntClear( &p->vCube2 );
    Vec_IntAppendSkip( &p->vCube1, vCube1, Pla_CubeLit(hCube1) );
    Vec_IntAppendSkip( &p->vCube2, vCube2, Pla_CubeLit(hCube2) );
    return Vec_IntEqual( &p->vCube1, &p->vCube2 );
}
static inline void Fxch_CompressCubes( Fxch_Man_t * p, Vec_Int_t * vLit2Cube )
{
    int i, hCube, k = 0;
    Vec_IntForEachEntry( vLit2Cube, hCube, i )
        if ( Vec_IntSize(Vec_WecEntry(&p->vCubes, hCube)) > 0 )
            Vec_IntWriteEntry( vLit2Cube, k++, hCube );
    Vec_IntShrink( vLit2Cube, k );
}
static inline int Fxch_CollectSingles( Vec_Int_t * vArr1, Vec_Int_t * vArr2, Vec_Int_t * vArr )
{
    int * pBeg1 = vArr1->pArray;
    int * pBeg2 = vArr2->pArray;
    int * pEnd1 = vArr1->pArray + vArr1->nSize;
    int * pEnd2 = vArr2->pArray + vArr2->nSize;
    int * pBeg1New = vArr1->pArray;
    int * pBeg2New = vArr2->pArray;
    Vec_IntClear( vArr );
    while ( pBeg1 < pEnd1 && pBeg2 < pEnd2 )
    {
        if ( Pla_CubeNum(*pBeg1) == Pla_CubeNum(*pBeg2) )
            Vec_IntPushTwo( vArr, *pBeg1, *pBeg2 ), pBeg1++, pBeg2++;
        else if ( *pBeg1 < *pBeg2 )
            *pBeg1New++ = *pBeg1++;
        else 
            *pBeg2New++ = *pBeg2++;
    }
    while ( pBeg1 < pEnd1 )
        *pBeg1New++ = *pBeg1++;
    while ( pBeg2 < pEnd2 )
        *pBeg2New++ = *pBeg2++;
    Vec_IntShrink( vArr1, pBeg1New - vArr1->pArray );
    Vec_IntShrink( vArr2, pBeg2New - vArr2->pArray );
    return Vec_IntSize(vArr);
}
static inline void Fxch_CollectDoubles( Fxch_Man_t * p, Vec_Int_t * vPairs, Vec_Int_t * vRes, int Lit0, int Lit1 )
{
    int i, hCube1, hCube2;
    Vec_IntClear( vRes );
    Vec_IntForEachEntryDouble( vPairs, hCube1, hCube2, i )
        if ( Fxch_TabCompare(p, hCube1, hCube2) && 
             Vec_IntEntry(Fxch_ManCube(p, hCube1), Pla_CubeLit(hCube1)) == Lit0 && 
             Vec_IntEntry(Fxch_ManCube(p, hCube2), Pla_CubeLit(hCube2)) == Lit1 )
            Vec_IntPushTwo( vRes, hCube1, hCube2 );
    Vec_IntClear( vPairs );
    // order pairs in the increasing order of the first cube
    //Vec_IntSortPairs( vRes );
}
static inline void Fxch_CompressLiterals2( Vec_Int_t * p, int iInd1, int iInd2 )
{
    int i, Lit, k = 0;
    assert( iInd1 >= 0 && iInd1 < Vec_IntSize(p) );
    if ( iInd2 != -1 )
    assert( iInd1 >= 0 && iInd1 < Vec_IntSize(p) );
    Vec_IntForEachEntry( p, Lit, i )
        if ( i != iInd1 && i != iInd2 )
            Vec_IntWriteEntry( p, k++, Lit );
    Vec_IntShrink( p, k );
}
static inline void Fxch_CompressLiterals( Vec_Int_t * p, int iLit1, int iLit2 )
{
    int i, Lit, k = 0;
    Vec_IntForEachEntry( p, Lit, i )
        if ( Lit != iLit1 && Lit != iLit2 )
            Vec_IntWriteEntry( p, k++, Lit );
    assert( Vec_IntSize(p) == k + 2 );
    Vec_IntShrink( p, k );
}
static inline void Fxch_FilterCubes( Fxch_Man_t * p, Vec_Int_t * vCubesS, int Lit0, int Lit1 )
{
    Vec_Int_t * vCube;
    int i, k, Lit, iCube, n = 0;
    int fFound0, fFound1;
    Vec_IntForEachEntry( vCubesS, iCube, i )
    {
        vCube = Vec_WecEntry( &p->vCubes, iCube );
        fFound0 = fFound1 = 0;
        Vec_IntForEachEntry( vCube, Lit, k )
        {
            if ( Lit == Lit0 )
                fFound0 = 1;
            else if ( Lit == Lit1 )
                fFound1 = 1;
        }
        if ( fFound0 && fFound1 )
            Vec_IntWriteEntry( vCubesS, n++, Pla_CubeHandle(iCube, 255) );
    }
    Vec_IntShrink( vCubesS, n );
}


/**Function*************************************************************

  Synopsis    [Divisor addition removal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxch_DivisorAdd( Fxch_Man_t * p, int Lit0, int Lit1, int Weight )
{
    int iDiv;
    assert( Lit0 != Lit1 );
    if ( Lit0 < Lit1 )
        iDiv = Hash_Int2ManInsert( p->vHash, Lit0, Lit1, 0 );
    else
        iDiv = Hash_Int2ManInsert( p->vHash, Lit1, Lit0, 0 );
    if ( iDiv == Vec_FltSize(&p->vWeights) )
    {
        Vec_FltPush( &p->vWeights, -2 );
        Vec_WecPushLevel( &p->vPairs );
        assert( Vec_FltSize(&p->vWeights) == Vec_WecSize(&p->vPairs) );
    }
    Vec_FltAddToEntry( &p->vWeights, iDiv, Weight );
    if ( p->vPrio )
    {
        if ( Vec_QueIsMember(p->vPrio, iDiv) )
            Vec_QueUpdate( p->vPrio, iDiv );
        else 
            Vec_QuePush( p->vPrio, iDiv );
        //assert( iDiv < Vec_QueSize(p->vPrio) );
    }
    return iDiv;
}
void Fxch_DivisorRemove( Fxch_Man_t * p, int Lit0, int Lit1, int Weight )
{
    int iDiv;
    assert( Lit0 != Lit1 );
    if ( Lit0 < Lit1 )
        iDiv = *Hash_Int2ManLookup( p->vHash, Lit0, Lit1 );
    else
        iDiv = *Hash_Int2ManLookup( p->vHash, Lit1, Lit0 );
    assert( iDiv > 0 && iDiv < Vec_FltSize(&p->vWeights) );
    Vec_FltAddToEntry( &p->vWeights, iDiv, -Weight );
    if ( Vec_QueIsMember(p->vPrio, iDiv) )
        Vec_QueUpdate( p->vPrio, iDiv );
}

/**Function*************************************************************

  Synopsis    [Starting the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Fxch_Obj_t * Fxch_TabBin( Fxch_Man_t * p, int Value )             { return p->pBins + (Value & p->SizeMask);         }
static inline Fxch_Obj_t * Fxch_TabEntry( Fxch_Man_t * p, int i )               { return i ? p->pBins + i : NULL;                  }
static inline int          Fxch_TabEntryId( Fxch_Man_t * p, Fxch_Obj_t * pEnt ) { assert(pEnt > p->pBins); return pEnt - p->pBins; }

static inline void Fxch_TabMarkPair( Fxch_Man_t * p, int i, int j )
{
    Fxch_Obj_t * pI = Fxch_TabEntry(p, i);
    Fxch_Obj_t * pJ = Fxch_TabEntry(p, j);
    assert( pI->Next == j );
    assert( pJ->Prev == i );
    assert( pI->MarkN == 0 );
    assert( pI->MarkP == 0 );
    assert( pJ->MarkN == 0 );
    assert( pJ->MarkP == 0 );
    pI->MarkN = 1;
    pJ->MarkP = 1;
}
static inline void Fxch_TabUnmarkPair( Fxch_Man_t * p, int i, int j )
{
    Fxch_Obj_t * pI = Fxch_TabEntry(p, i);
    Fxch_Obj_t * pJ = Fxch_TabEntry(p, j);
    assert( pI->Next == j );
    assert( pJ->Prev == i );
    assert( pI->MarkN == 1 );
    assert( pI->MarkP == 0 );
    assert( pJ->MarkN == 0 );
    assert( pJ->MarkP == 1 );
    pI->MarkN = 0;
    pJ->MarkP = 0;
}
static inline void Fxch_TabInsertLink( Fxch_Man_t * p, int i, int j, int fSkipCheck )
{
    Fxch_Obj_t * pI = Fxch_TabEntry(p, i);
    Fxch_Obj_t * pN = Fxch_TabEntry(p, pI->Next);
    Fxch_Obj_t * pJ = Fxch_TabEntry(p, j);
    //assert( pJ->Cube != 0 );
    assert( pN->Prev == i );
    assert( fSkipCheck || pI->MarkN == 0 );
    assert( fSkipCheck || pN->MarkP == 0 );
    assert( fSkipCheck || pJ->MarkN == 0 );
    assert( fSkipCheck || pJ->MarkP == 0 );
    pJ->Next = pI->Next;  pI->Next = j;
    pJ->Prev = i;         pN->Prev = j;
}
static inline void Fxch_TabExtractLink( Fxch_Man_t * p, int i, int j )
{
    Fxch_Obj_t * pI = Fxch_TabEntry(p, i);
    Fxch_Obj_t * pJ = Fxch_TabEntry(p, j);
    Fxch_Obj_t * pN = Fxch_TabEntry(p, pJ->Next);
    //assert( pJ->Cube != 0 );
    assert( pI->Next == j );
    assert( pJ->Prev == i );
    assert( pN->Prev == j );
    assert( pI->MarkN == 0 );
    assert( pJ->MarkP == 0 );
    assert( pJ->MarkN == 0 );
    assert( pN->MarkP == 0 );
    pI->Next = pJ->Next;  pJ->Next = 0;
    pN->Prev = pJ->Prev;  pJ->Prev = 0;
}
static inline void Fxch_TabInsert( Fxch_Man_t * p, int iLink, int Value, int hCube )
{
    int iEnt, iDiv, Lit0, Lit1, fStart = 1;
    Fxch_Obj_t * pEnt;
    Fxch_Obj_t * pBin  = Fxch_TabBin( p, Value );
    Fxch_Obj_t * pCell = Fxch_TabEntry( p, iLink );
    assert( pCell->MarkN == 0 );
    assert( pCell->MarkP == 0 );
    assert( pCell->Cube == 0 );
    pCell->Cube = hCube;
    if ( pBin->Table == 0 )
    {
        pBin->Table = pCell->Next = pCell->Prev = iLink;
        return;
    }
    // find equal cubes
    for ( iEnt = pBin->Table; iEnt != (int)pBin->Table || fStart; iEnt = pEnt->Next, fStart = 0 )
    {
        pEnt = Fxch_TabBin( p, iEnt );
        if ( pEnt->MarkN || pEnt->MarkP || !Fxch_TabCompare(p, pEnt->Cube, hCube) )
            continue;
        Fxch_TabInsertLink( p, iEnt, iLink, 0 );
        Fxch_TabMarkPair( p, iEnt, iLink );
        // get literals
        Lit0 = Vec_IntEntry( Fxch_ManCube(p, hCube),      Pla_CubeLit(hCube) );
        Lit1 = Vec_IntEntry( Fxch_ManCube(p, pEnt->Cube), Pla_CubeLit(pEnt->Cube) );
        // increment divisor weight
        iDiv = Fxch_DivisorAdd( p, Abc_LitNot(Lit0), Abc_LitNot(Lit1), Vec_IntSize(Fxch_ManCube(p, hCube)) );
        // add divisor pair
        assert( iDiv < Vec_WecSize(&p->vPairs) );
        if ( Lit0 < Lit1 )
        {
            Vec_WecPush( &p->vPairs, iDiv, hCube );
            Vec_WecPush( &p->vPairs, iDiv, pEnt->Cube );
        }
        else
        {
            Vec_WecPush( &p->vPairs, iDiv, pEnt->Cube );
            Vec_WecPush( &p->vPairs, iDiv, hCube );
        }
        p->nPairsD++;
        return;
    }
    assert( iEnt == (int)pBin->Table );
    pEnt = Fxch_TabBin( p, iEnt );
    Fxch_TabInsertLink( p, pEnt->Prev, iLink, 1 );
}
static inline void Fxch_TabExtract( Fxch_Man_t * p, int iLink, int Value, int hCube )
{
    int Lit0, Lit1;
    Fxch_Obj_t * pPair = NULL;
    Fxch_Obj_t * pBin  = Fxch_TabBin( p, Value );
    Fxch_Obj_t * pLink = Fxch_TabEntry( p, iLink );
    assert( pLink->Cube == hCube );
    if ( pLink->MarkN )
    {
        pPair = Fxch_TabEntry( p, pLink->Next );
        Fxch_TabUnmarkPair( p, iLink, pLink->Next );
    }
    else if ( pLink->MarkP )
    {
        pPair = Fxch_TabEntry( p, pLink->Prev );
        Fxch_TabUnmarkPair( p, pLink->Prev, iLink );
    }
    if ( (int)pBin->Table == iLink )
        pBin->Table = pLink->Next != iLink ? pLink->Next : 0;
    if ( pLink->Next == iLink )
    {
        assert( pLink->Prev == iLink );
        pLink->Next = pLink->Prev = 0;
    }
    else
        Fxch_TabExtractLink( p, pLink->Prev, iLink );
    pLink->Cube = 0;
    if ( pPair == NULL )
        return;
    assert( Fxch_TabCompare(p, pPair->Cube, hCube) );
    // get literals
    Lit0 = Vec_IntEntry( Fxch_ManCube(p, hCube),       Pla_CubeLit(hCube) );
    Lit1 = Vec_IntEntry( Fxch_ManCube(p, pPair->Cube), Pla_CubeLit(pPair->Cube) );
    // decrement divisor weight
    Fxch_DivisorRemove( p, Abc_LitNot(Lit0), Abc_LitNot(Lit1), Vec_IntSize(Fxch_ManCube(p, hCube)) );
    p->nPairsD--;
}

/**Function*************************************************************

  Synopsis    [Starting the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxch_TabSingleDivisors( Fxch_Man_t * p, int iCube, int fAdd )
{
    Vec_Int_t * vCube = Vec_WecEntry( &p->vCubes, iCube );
    int i, k, Lit, Lit2;
    if ( Vec_IntSize(vCube) < 2 )
        return 0;
    Vec_IntForEachEntry( vCube, Lit, i )
    Vec_IntForEachEntryStart( vCube, Lit2, k, i+1 )
    {
        assert( Lit < Lit2 );
        if ( fAdd )
            Fxch_DivisorAdd( p, Lit, Lit2, 1 ), p->nPairsS++;
        else
            Fxch_DivisorRemove( p, Lit, Lit2, 1 ), p->nPairsS--;
    }
    return Vec_IntSize(vCube) * (Vec_IntSize(vCube) - 1) / 2;
}
int Fxch_TabDoubleDivisors( Fxch_Man_t * p, int iCube, int fAdd )
{
    Vec_Int_t * vCube = Vec_WecEntry( &p->vCubes, iCube );
    int iLinkFirst = Vec_IntEntry( &p->vCubeLinks, iCube );
    int k, Lit, Value = 0;
    Vec_IntForEachEntry( vCube, Lit, k )
        Value += Vec_IntEntry(&p->vRands, Lit);
    Vec_IntForEachEntry( vCube, Lit, k )
    {
        Value -= Vec_IntEntry(&p->vRands, Lit);
        if ( fAdd )
            Fxch_TabInsert( p, iLinkFirst + k, Value, Pla_CubeHandle(iCube, k) );
        else
            Fxch_TabExtract( p, iLinkFirst + k, Value, Pla_CubeHandle(iCube, k) );
        Value += Vec_IntEntry(&p->vRands, Lit);
    }
    return 1;
}
void Fxch_ManCreateDivisors( Fxch_Man_t * p )
{
    float Weight;  int i;
    // alloc hash table
    assert( p->vHash == NULL );
    p->vHash = Hash_IntManStart( 1000 );
    // create single-cube two-literal divisors
    for ( i = 0; i < Vec_WecSize(&p->vCubes); i++ )
        Fxch_TabSingleDivisors( p, i, 1 ); // add - no update
    // create two-cube divisors
    for ( i = 0; i < Vec_WecSize(&p->vCubes); i++ )
        Fxch_TabDoubleDivisors( p, i, 1 ); // add - no update
    // create queue with all divisors
    p->vPrio = Vec_QueAlloc( Vec_FltSize(&p->vWeights) );
    Vec_QueSetPriority( p->vPrio, Vec_FltArrayP(&p->vWeights) );
    Vec_FltForEachEntry( &p->vWeights, Weight, i )
        if ( Weight > 0.0 )
            Vec_QuePush( p->vPrio, i );
}


/**Function*************************************************************

  Synopsis    [Updates the data-structure when one divisor is selected.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxch_ManUpdate( Fxch_Man_t * p, int iDiv )
{
    Vec_Int_t * vCube1, * vCube2, * vLitP, * vLitN;
    //int nLitsNew = p->nLits - (int)Vec_FltEntry(&p->vWeights, iDiv);
    int i, Lit0, Lit1, hCube1, hCube2, iVarNew;
    //float Diff = Vec_FltEntry(&p->vWeights, iDiv) - (float)((int)Vec_FltEntry(&p->vWeights, iDiv));
    //assert( Diff > 0.0 && Diff < 1.0 );

    // get the divisor and select pivot variables
    Vec_IntPush( &p->vRands, Gia_ManRandom(0) & 0x3FFFFFF ); 
    Vec_IntPush( &p->vRands, Gia_ManRandom(0) & 0x3FFFFFF );
    Lit0 = Hash_IntObjData0( p->vHash, iDiv );
    Lit1 = Hash_IntObjData1( p->vHash, iDiv );
    assert( Lit0 >= 0 && Lit1 >= 0 && Lit0 < Lit1 );
    Vec_WrdPush( &p->vDivs, ((word)Lit1 << 32) | (word)Lit0 );

    // if the input cover is not prime, it may happen that we are extracting divisor (x + !x)
    // although it is not strictly correct, it seems to be fine to just skip such divisors
//    if ( Abc_Lit2Var(Lit0) == Abc_Lit2Var(Lit1) && Vec_IntSize(Hsh_VecReadEntry(p->vHash, iDiv)) == 2 )
//        return;

    // collect single-cube-divisor cubes
    vLitP = Vec_WecEntry(&p->vLits, Lit0);
    vLitN = Vec_WecEntry(&p->vLits, Lit1);
    Fxch_CompressCubes( p, vLitP );
    Fxch_CompressCubes( p, vLitN );
//    Fxch_CollectSingles( vLitP, vLitN, &p->vCubesS );
//    assert( Vec_IntSize(&p->vCubesS) % 2 == 0 );
    Vec_IntTwoRemoveCommon( vLitP, vLitN, &p->vCubesS );
    Fxch_FilterCubes( p, &p->vCubesS, Lit0, Lit1 );

    // collect double-cube-divisor cube pairs
    Fxch_CollectDoubles( p, Vec_WecEntry(&p->vPairs, iDiv), &p->vCubesD, Abc_LitNot(Lit0), Abc_LitNot(Lit1) );
    assert( Vec_IntSize(&p->vCubesD) % 2 == 0 );
    Vec_IntUniqifyPairs( &p->vCubesD );
    assert( Vec_IntSize(&p->vCubesD) % 2 == 0 );

    // subtract cost of single-cube divisors
//    Vec_IntForEachEntryDouble( &p->vCubesS, hCube1, hCube2, i )
    Vec_IntForEachEntry( &p->vCubesS, hCube1, i )
        Fxch_TabSingleDivisors( p, Pla_CubeNum(hCube1), 0 );  // remove - update
    Vec_IntForEachEntryDouble( &p->vCubesD, hCube1, hCube2, i )
        Fxch_TabSingleDivisors( p, Pla_CubeNum(hCube1), 0 ),  // remove - update
        Fxch_TabSingleDivisors( p, Pla_CubeNum(hCube2), 0 );  // remove - update

    // subtract cost of double-cube divisors
//    Vec_IntForEachEntryDouble( &p->vCubesS, hCube1, hCube2, i )
    Vec_IntForEachEntry( &p->vCubesS, hCube1, i )
    {
        //printf( "%d ", Pla_CubeNum(hCube1) );
        Fxch_TabDoubleDivisors( p, Pla_CubeNum(hCube1), 0 );  // remove - update
    }
    //printf( "\n" );

    Vec_IntForEachEntryDouble( &p->vCubesD, hCube1, hCube2, i )
    {
        //printf( "%d ", Pla_CubeNum(hCube1) );
        //printf( "%d  ", Pla_CubeNum(hCube2) );
        Fxch_TabDoubleDivisors( p, Pla_CubeNum(hCube1), 0 ),  // remove - update
        Fxch_TabDoubleDivisors( p, Pla_CubeNum(hCube2), 0 );  // remove - update
    }
    //printf( "\n" );

    // create new literals
    p->nLits += 2;
    iVarNew = Vec_WecSize( &p->vLits ) / 2;
    vLitP = Vec_WecPushLevel( &p->vLits );
    vLitN = Vec_WecPushLevel( &p->vLits );
    vLitP = Vec_WecEntry( &p->vLits, Vec_WecSize(&p->vLits) - 2 );
    // update single-cube divisor cubes
//    Vec_IntForEachEntryDouble( &p->vCubesS, hCube1, hCube2, i )
    Vec_IntForEachEntry( &p->vCubesS, hCube1, i )
    {
//        int Lit0s, Lit1s;
        vCube1 = Fxch_ManCube( p, hCube1 );
//        Lit0s = Vec_IntEntry(vCube1, Pla_CubeLit(hCube1));
//        Lit1s = Vec_IntEntry(vCube1, Pla_CubeLit(hCube2));
//        assert( Pla_CubeNum(hCube1) == Pla_CubeNum(hCube2) );
//        assert( Vec_IntEntry(vCube1, Pla_CubeLit(hCube1)) == Lit0 );
//        assert( Vec_IntEntry(vCube1, Pla_CubeLit(hCube2)) == Lit1 );
        Fxch_CompressLiterals( vCube1, Lit0, Lit1 );
//        Vec_IntPush( vLitP, Pla_CubeHandle(Pla_CubeNum(hCube1), Vec_IntSize(vCube1)) );
        Vec_IntPush( vLitP, Pla_CubeNum(hCube1) );
        Vec_IntPush( vCube1, Abc_Var2Lit(iVarNew, 0) );

        //if ( Pla_CubeNum(hCube1) == 3 )
        //    printf( "VecSize = %d\n", Vec_IntSize(vCube1) );

        p->nLits--;
    }
    // update double-cube divisor cube pairs
    Vec_IntForEachEntryDouble( &p->vCubesD, hCube1, hCube2, i )
    {
        vCube1 = Fxch_ManCube( p, hCube1 );
        vCube2 = Fxch_ManCube( p, hCube2 );
        assert( Vec_IntEntry(vCube1, Pla_CubeLit(hCube1)) == Abc_LitNot(Lit0) );
        assert( Vec_IntEntry(vCube2, Pla_CubeLit(hCube2)) == Abc_LitNot(Lit1) );
        Fxch_CompressLiterals2( vCube1, Pla_CubeLit(hCube1), -1 );
//        Vec_IntPush( vLitN, Pla_CubeHandle(Pla_CubeNum(hCube1), Vec_IntSize(vCube1)) );
        Vec_IntPush( vLitN, Pla_CubeNum(hCube1) );
        Vec_IntPush( vCube1, Abc_Var2Lit(iVarNew, 1) );
        p->nLits -= Vec_IntSize(vCube2);

        //if ( Pla_CubeNum(hCube1) == 3 )
        //    printf( "VecSize = %d\n", Vec_IntSize(vCube1) );

        // remove second cube
        Vec_IntClear( vCube2 ); 
    }
    Vec_IntSort( vLitN, 0 );
    Vec_IntSort( vLitP, 0 );

    // add cost of single-cube divisors
//    Vec_IntForEachEntryDouble( &p->vCubesS, hCube1, hCube2, i )
    Vec_IntForEachEntry( &p->vCubesS, hCube1, i )
        Fxch_TabSingleDivisors( p, Pla_CubeNum(hCube1), 1 );  // add - update
    Vec_IntForEachEntryDouble( &p->vCubesD, hCube1, hCube2, i )
        Fxch_TabSingleDivisors( p, Pla_CubeNum(hCube1), 1 );  // add - update

    // add cost of double-cube divisors
//    Vec_IntForEachEntryDouble( &p->vCubesS, hCube1, hCube2, i )
    Vec_IntForEachEntry( &p->vCubesS, hCube1, i )
        Fxch_TabDoubleDivisors( p, Pla_CubeNum(hCube1), 1 );  // add - update
    Vec_IntForEachEntryDouble( &p->vCubesD, hCube1, hCube2, i )
        Fxch_TabDoubleDivisors( p, Pla_CubeNum(hCube1), 1 );  // add - update
    
    // check predicted improvement: (new SOP lits == old SOP lits - divisor weight)
    //assert( p->nLits == nLitsNew );
}

/**Function*************************************************************

  Synopsis    [Implements the improved fast_extract algorithm.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Fxch_PrintStats( Fxch_Man_t * p, abctime clk )
{
    printf( "Num =%6d  ", Vec_WrdSize(&p->vDivs) - p->nVars );
    printf( "Cubes =%8d  ", Vec_WecSizeUsed(&p->vCubes) );
    printf( "Lits  =%8d  ", p->nLits );
    printf( "Divs  =%8d  ", Hash_IntManEntryNum(p->vHash) );
    printf( "Divs+ =%8d  ", Vec_QueSize(p->vPrio) );
    printf( "PairS =%6d  ", p->nPairsS );
    printf( "PairD =%6d  ", p->nPairsD );
    Abc_PrintTime( 1, "Time", clk );
//    printf( "\n" );
}
static inline void Fxch_PrintDivOne( Fxch_Man_t * p, int iDiv )
{
    int i;
    int Lit0 = Hash_IntObjData0( p->vHash, iDiv );
    int Lit1 = Hash_IntObjData1( p->vHash, iDiv );
    assert( Lit0 >= 0 && Lit1 >= 0 && Lit0 < Lit1 );
    printf( "Div %4d : ", iDiv );
    printf( "Weight %12.5f  ", Vec_FltEntry(&p->vWeights, iDiv) );
    printf( "Pairs = %5d  ",   Vec_IntSize(Vec_WecEntry(&p->vPairs, iDiv))/2 );
    for ( i = 0; i < Vec_WrdSize(&p->vDivs); i++ )
    {
        if ( i == Abc_Lit2Var(Lit0) )
            printf( "%d", !Abc_LitIsCompl(Lit0) );
        else if ( i == Abc_Lit2Var(Lit1) )
            printf( "%d", !Abc_LitIsCompl(Lit1) );
        else
            printf( "-" );
    }
    printf( "\n" );
}
static void Fxch_PrintDivisors( Fxch_Man_t * p )
{
    int iDiv;
    for ( iDiv = 1; iDiv < Vec_FltSize(&p->vWeights); iDiv++ )
        Fxch_PrintDivOne( p, iDiv );
    printf( "\n" );
}

int Fxch_ManFastExtract( Fxch_Man_t * p, int fVerbose, int fVeryVerbose )
{
    int nNewNodesMax = ABC_INFINITY;
    abctime clk = Abc_Clock();
    int i, iDiv;
    Fxch_ManCreateDivisors( p );
//    Fxch_PrintDivisors( p );
    if ( fVerbose )
        Fxch_PrintStats( p, Abc_Clock() - clk );
    p->timeStart = Abc_Clock();
    for ( i = 0; i < nNewNodesMax && Vec_QueTopPriority(p->vPrio) > 0.0; i++ )
    {
        iDiv = Vec_QuePop(p->vPrio);
        //if ( fVerbose )
        //    Fxch_PrintStats( p, Abc_Clock() - clk );
        if ( fVeryVerbose )
            Fxch_PrintDivOne( p, iDiv );
        Fxch_ManUpdate( p, iDiv );
    }
    if ( fVerbose )
        Fxch_PrintStats( p, Abc_Clock() - clk );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Implements the improved fast_extract algorithm.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Pla_ManPerformFxch( Pla_Man_t * p )
{
    char pFileName[1000];
    Fxch_Man_t * pFxch;
    Pla_ManConvertFromBits( p );
    pFxch = Fxch_ManStart( &p->vCubeLits, &p->vOccurs );
    Vec_WecZero( &p->vCubeLits );
    Vec_WecZero( &p->vOccurs );
    Fxch_ManFastExtract( pFxch, 1, 0 );
    sprintf( pFileName, "%s.blif", Pla_ManName(p) );
    //Fxch_ManWriteBlif( pFileName, &pFxch->vCubes, &pFxch->vDivs );
    Fxch_ManStop( pFxch );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

