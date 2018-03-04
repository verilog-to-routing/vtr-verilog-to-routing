/**CFile****************************************************************

  FileName    [covInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Mapping into network of SOPs/ESOPs.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: covInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__map__cov__covInt_h
#define ABC__map__cov__covInt_h

#include "base/abc/abc.h"


ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Min_Man_t_  Min_Man_t;
typedef struct Min_Cube_t_ Min_Cube_t;

struct Min_Man_t_
{
    int               nVars;          // the number of vars
    int               nWords;         // the number of words
    Extra_MmFixed_t * pMemMan;        // memory manager for cubes
    // temporary cubes
    Min_Cube_t *      pOne0;          // tautology cube
    Min_Cube_t *      pOne1;          // tautology cube
    Min_Cube_t *      pTriv0[2];      // trivial cube
    Min_Cube_t *      pTriv1[2];      // trivial cube
    Min_Cube_t *      pTemp;          // cube for computing the distance
    Min_Cube_t *      pBubble;        // cube used as a separator
    // temporary storage for the new cover
    int               nCubes;         // the number of cubes
    Min_Cube_t **     ppStore;        // storage for cubes by number of literals
};

struct Min_Cube_t_
{
    Min_Cube_t *      pNext;          // the pointer to the next cube in the cover
    unsigned          nVars    : 10;  // the number of variables
    unsigned          nWords   : 12;  // the number of machine words
    unsigned          nLits    : 10;  // the number of literals in the cube
    unsigned          uData[1];       // the bit-data for the cube
};


// iterators through the entries in the linked lists of cubes
#define Min_CoverForEachCube( pCover, pCube )                   \
    for ( pCube = pCover;                                       \
          pCube;                                                \
          pCube = pCube->pNext )
#define Min_CoverForEachCubeSafe( pCover, pCube, pCube2 )       \
    for ( pCube = pCover,                                       \
          pCube2 = pCube? pCube->pNext: NULL;                   \
          pCube;                                                \
          pCube = pCube2,                                       \
          pCube2 = pCube? pCube->pNext: NULL )
#define Min_CoverForEachCubePrev( pCover, pCube, ppPrev )       \
    for ( pCube = pCover,                                       \
          ppPrev = &(pCover);                                   \
          pCube;                                                \
          ppPrev = &pCube->pNext,                               \
          pCube = pCube->pNext )

// macros to get hold of bits and values in the cubes
static inline int  Min_CubeHasBit( Min_Cube_t * p, int i )              { return (p->uData[(i)>>5] & (1<<((i) & 31))) > 0;     }
static inline void Min_CubeSetBit( Min_Cube_t * p, int i )              { p->uData[(i)>>5] |= (1<<((i) & 31));                 }
static inline void Min_CubeXorBit( Min_Cube_t * p, int i )              { p->uData[(i)>>5] ^= (1<<((i) & 31));                 }
static inline int  Min_CubeGetVar( Min_Cube_t * p, int Var )            { return 3 & (p->uData[(2*Var)>>5] >> ((2*Var) & 31)); }
static inline void Min_CubeXorVar( Min_Cube_t * p, int Var, int Value ) { p->uData[(2*Var)>>5] ^= (Value<<((2*Var) & 31));     }

/*=== covMinEsop.c ==========================================================*/
extern void         Min_EsopMinimize( Min_Man_t * p );
extern void         Min_EsopAddCube( Min_Man_t * p, Min_Cube_t * pCube );
/*=== covMinSop.c ==========================================================*/
extern void         Min_SopMinimize( Min_Man_t * p );
extern void         Min_SopAddCube( Min_Man_t * p, Min_Cube_t * pCube );
/*=== covMinMan.c ==========================================================*/
extern Min_Man_t *  Min_ManAlloc( int nVars );
extern void         Min_ManClean( Min_Man_t * p, int nSupp );
extern void         Min_ManFree( Min_Man_t * p );
/*=== covMinUtil.c ==========================================================*/
extern void         Min_CoverCreate( Vec_Str_t * vCover, Min_Cube_t * pCover, char Type );
extern void         Min_CubeWrite( FILE * pFile, Min_Cube_t * pCube );
extern void         Min_CoverWrite( FILE * pFile, Min_Cube_t * pCover );
extern void         Min_CoverWriteStore( FILE * pFile, Min_Man_t * p );
extern void         Min_CoverWriteFile( Min_Cube_t * pCover, char * pName, int fEsop );
extern void         Min_CoverCheck( Min_Man_t * p );
extern int          Min_CubeCheck( Min_Cube_t * pCube );
extern Min_Cube_t * Min_CoverCollect( Min_Man_t * p, int nSuppSize );
extern void         Min_CoverExpand( Min_Man_t * p, Min_Cube_t * pCover );
extern int          Min_CoverSuppVarNum( Min_Man_t * p, Min_Cube_t * pCover );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Min_Cube_t * Min_CubeAlloc( Min_Man_t * p )
{
    Min_Cube_t * pCube;
    pCube = (Min_Cube_t *)Extra_MmFixedEntryFetch( p->pMemMan );
    pCube->pNext  = NULL;
    pCube->nVars  = p->nVars;
    pCube->nWords = p->nWords;
    pCube->nLits  = 0;
    memset( pCube->uData, 0xff, sizeof(unsigned) * p->nWords );
    return pCube;
}

/**Function*************************************************************

  Synopsis    [Creates the cube representing elementary var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Min_Cube_t * Min_CubeAllocVar( Min_Man_t * p, int iVar, int fCompl )
{
    Min_Cube_t * pCube;
    pCube = Min_CubeAlloc( p );
    Min_CubeXorBit( pCube, iVar*2+fCompl );
    pCube->nLits = 1;
    return pCube;
}

/**Function*************************************************************

  Synopsis    [Creates the cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Min_Cube_t * Min_CubeDup( Min_Man_t * p, Min_Cube_t * pCopy )
{
    Min_Cube_t * pCube;
    pCube = Min_CubeAlloc( p );
    memcpy( pCube->uData, pCopy->uData, sizeof(unsigned) * p->nWords );
    pCube->nLits = pCopy->nLits;
    return pCube;
}

/**Function*************************************************************

  Synopsis    [Recycles the cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Min_CubeRecycle( Min_Man_t * p, Min_Cube_t * pCube )
{
    Extra_MmFixedEntryRecycle( p->pMemMan, (char *)pCube );
}

/**Function*************************************************************

  Synopsis    [Recycles the cube cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Min_CoverRecycle( Min_Man_t * p, Min_Cube_t * pCover )
{
    Min_Cube_t * pCube, * pCube2;
    Min_CoverForEachCubeSafe( pCover, pCube, pCube2 )
        Extra_MmFixedEntryRecycle( p->pMemMan, (char *)pCube );
}


/**Function*************************************************************

  Synopsis    [Counts the number of cubes in the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Min_CubeCountLits( Min_Cube_t * pCube )
{
    unsigned uData;
    int Count = 0, i, w;
    for ( w = 0; w < (int)pCube->nWords; w++ )
    {
        uData = pCube->uData[w] ^ (pCube->uData[w] >> 1);
        for ( i = 0; i < 32; i += 2 )
            if ( uData & (1 << i) )
                Count++;
    }
    return Count;
}

/**Function*************************************************************

  Synopsis    [Counts the number of cubes in the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Min_CubeGetLits( Min_Cube_t * pCube, Vec_Int_t * vLits )
{
    unsigned uData;
    int i, w;
    Vec_IntClear( vLits );
    for ( w = 0; w < (int)pCube->nWords; w++ )
    {
        uData = pCube->uData[w] ^ (pCube->uData[w] >> 1);
        for ( i = 0; i < 32; i += 2 )
            if ( uData & (1 << i) )
                Vec_IntPush( vLits, w*16 + i/2 );
    }
}

/**Function*************************************************************

  Synopsis    [Counts the number of cubes in the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Min_CoverCountCubes( Min_Cube_t * pCover )
{
    Min_Cube_t * pCube;
    int Count = 0;
    Min_CoverForEachCube( pCover, pCube )
        Count++;
    return Count;
}


/**Function*************************************************************

  Synopsis    [Checks if two cubes are disjoint.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Min_CubesDisjoint( Min_Cube_t * pCube0, Min_Cube_t * pCube1 )
{
    unsigned uData;
    int i;
    assert( pCube0->nVars == pCube1->nVars );
    for ( i = 0; i < (int)pCube0->nWords; i++ )
    {
        uData = pCube0->uData[i] & pCube1->uData[i];
        uData = (uData | (uData >> 1)) & 0x55555555;
        if ( uData != 0x55555555 )
            return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Collects the disjoint variables of the two cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Min_CoverGetDisjVars( Min_Cube_t * pThis, Min_Cube_t * pCube, Vec_Int_t * vVars )
{
    unsigned uData;
    int i, w;
    Vec_IntClear( vVars );
    for ( w = 0; w < (int)pCube->nWords; w++ )
    {
        uData  = pThis->uData[w] & (pThis->uData[w] >> 1) & 0x55555555;
        uData &= (pCube->uData[w] ^ (pCube->uData[w] >> 1));
        if ( uData == 0 )
            continue;
        for ( i = 0; i < 32; i += 2 )
            if ( uData & (1 << i) )
                Vec_IntPush( vVars, w*16 + i/2 );
    }
}

/**Function*************************************************************

  Synopsis    [Checks if two cubes are disjoint.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Min_CubesDistOne( Min_Cube_t * pCube0, Min_Cube_t * pCube1, Min_Cube_t * pTemp )
{
    unsigned uData;
    int i, fFound = 0;
    for ( i = 0; i < (int)pCube0->nWords; i++ )
    {
        uData = pCube0->uData[i] ^ pCube1->uData[i];
        if ( uData == 0 )
        {
            if ( pTemp ) pTemp->uData[i] = 0;
            continue;
        }
        if ( fFound )
            return 0;
        uData = (uData | (uData >> 1)) & 0x55555555;
        if ( (uData & (uData-1)) > 0 ) // more than one 1
            return 0;
        if ( pTemp ) pTemp->uData[i] = uData | (uData << 1);
        fFound = 1;
    }
    if ( fFound == 0 )
    {
        printf( "\n" );
        Min_CubeWrite( stdout, pCube0 );
        Min_CubeWrite( stdout, pCube1 );
        printf( "Error: Min_CubesDistOne() looks at two equal cubes!\n" );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Checks if two cubes are disjoint.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Min_CubesDistTwo( Min_Cube_t * pCube0, Min_Cube_t * pCube1, int * pVar0, int * pVar1 )
{
    unsigned uData;//, uData2;
    int i, k, Var0 = -1, Var1 = -1;
    for ( i = 0; i < (int)pCube0->nWords; i++ )
    {
        uData = pCube0->uData[i] ^ pCube1->uData[i];
        if ( uData == 0 )
            continue;
        if ( Var0 >= 0 && Var1 >= 0 ) // more than two 1s
            return 0;
        uData = (uData | (uData >> 1)) & 0x55555555;
        if ( (Var0 >= 0 || Var1 >= 0) && (uData & (uData-1)) > 0 )
            return 0;
        for ( k = 0; k < 32; k += 2 )
            if ( uData & (1 << k) )
            {
                if ( Var0 == -1 )
                    Var0 = 16 * i + k/2;
                else if ( Var1 == -1 )
                    Var1 = 16 * i + k/2;
                else
                    return 0;
            }
        /*
        if ( Var0 >= 0 )
        {
            uData &= 0xFFFF;
            uData2 = (uData >> 16);
            if ( uData && uData2 )
                return 0;
            if ( uData )
            {
            }
            uData }= uData2;
            uData &= 0x
        }
        */
    }
    if ( Var0 >= 0 && Var1 >= 0 )
    {
        *pVar0 = Var0;
        *pVar1 = Var1;
        return 1;
    }
    if ( Var0 == -1 || Var1 == -1 )
    {
        printf( "\n" );
        Min_CubeWrite( stdout, pCube0 );
        Min_CubeWrite( stdout, pCube1 );
        printf( "Error: Min_CubesDistTwo() looks at two equal cubes or dist1 cubes!\n" );
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Makes the produce of two cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Min_Cube_t * Min_CubesProduct( Min_Man_t * p, Min_Cube_t * pCube0, Min_Cube_t * pCube1 )
{
    Min_Cube_t * pCube;
    int i;
    assert( pCube0->nVars == pCube1->nVars );
    pCube = Min_CubeAlloc( p );
    for ( i = 0; i < p->nWords; i++ )
        pCube->uData[i] = pCube0->uData[i] & pCube1->uData[i];
    pCube->nLits = Min_CubeCountLits( pCube );
    return pCube;
}

/**Function*************************************************************

  Synopsis    [Makes the produce of two cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Min_Cube_t * Min_CubesXor( Min_Man_t * p, Min_Cube_t * pCube0, Min_Cube_t * pCube1 )
{
    Min_Cube_t * pCube;
    int i;
    assert( pCube0->nVars == pCube1->nVars );
    pCube = Min_CubeAlloc( p );
    for ( i = 0; i < p->nWords; i++ )
        pCube->uData[i] = pCube0->uData[i] ^ pCube1->uData[i];
    pCube->nLits = Min_CubeCountLits( pCube );
    return pCube;
}

/**Function*************************************************************

  Synopsis    [Makes the produce of two cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Min_CubesAreEqual( Min_Cube_t * pCube0, Min_Cube_t * pCube1 )
{
    int i;
    for ( i = 0; i < (int)pCube0->nWords; i++ )
        if ( pCube0->uData[i] != pCube1->uData[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if pCube1 is contained in pCube0, bitwise.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Min_CubeIsContained( Min_Cube_t * pCube0, Min_Cube_t * pCube1 )
{
    int i;
    for ( i = 0; i < (int)pCube0->nWords; i++ )
        if ( (pCube0->uData[i] & pCube1->uData[i]) != pCube1->uData[i] )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Transforms the cube into the result of merging.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Min_CubesTransform( Min_Cube_t * pCube, Min_Cube_t * pDist, Min_Cube_t * pMask )
{
    int w;
    for ( w = 0; w < (int)pCube->nWords; w++ )
    {
        pCube->uData[w]  =  pCube->uData[w] ^ pDist->uData[w];
        pCube->uData[w] |= (pDist->uData[w] & ~pMask->uData[w]);
    }
}

/**Function*************************************************************

  Synopsis    [Transforms the cube into the result of distance-1 merging.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Min_CubesTransformOr( Min_Cube_t * pCube, Min_Cube_t * pDist )
{
    int w;
    for ( w = 0; w < (int)pCube->nWords; w++ )
        pCube->uData[w] |= pDist->uData[w];
}



/**Function*************************************************************

  Synopsis    [Sorts the cover in the increasing number of literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Min_CoverExpandRemoveEqual( Min_Man_t * p, Min_Cube_t * pCover )
{
    Min_Cube_t * pCube, * pCube2, * pThis;
    if ( pCover == NULL )
    {
        Min_ManClean( p, p->nVars );
        return;
    }
    Min_ManClean( p, pCover->nVars );
    Min_CoverForEachCubeSafe( pCover, pCube, pCube2 )
    {
        // go through the linked list
        Min_CoverForEachCube( p->ppStore[pCube->nLits], pThis )
            if ( Min_CubesAreEqual( pCube, pThis ) )
            {
                Min_CubeRecycle( p, pCube );
                break;
            }
        if ( pThis != NULL )
            continue;
        pCube->pNext = p->ppStore[pCube->nLits];
        p->ppStore[pCube->nLits] = pCube;
        p->nCubes++;
    }
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the given cube is contained in one of the cubes of the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Min_CoverContainsCube( Min_Man_t * p, Min_Cube_t * pCube )
{
    Min_Cube_t * pThis;
    int i;
/*
    // this cube cannot be equal to any cube
    Min_CoverForEachCube( p->ppStore[pCube->nLits], pThis )
    {
        if ( Min_CubesAreEqual( pCube, pThis ) )
        {
            Min_CubeWrite( stdout, pCube );
            assert( 0 );
        }
    }
*/
    // try to find a containing cube
    for ( i = 0; i <= (int)pCube->nLits; i++ )
    Min_CoverForEachCube( p->ppStore[i], pThis )
    {
        // skip the bubble
        if ( pThis != p->pBubble && Min_CubeIsContained( pThis, pCube ) )
            return 1;
    }
    return 0;
}


ABC_NAMESPACE_HEADER_END

#endif


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
