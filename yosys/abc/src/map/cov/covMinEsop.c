/**CFile****************************************************************

  FileName    [covMinEsop.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Mapping into network of SOPs/ESOPs.]

  Synopsis    [ESOP manipulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: covMinEsop.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "covInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Min_EsopRewrite( Min_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_EsopMinimize( Min_Man_t * p )
{
    int nCubesInit, nCubesOld, nIter;
    if ( p->nCubes < 3 )
        return;
    nIter = 0;
    nCubesInit = p->nCubes;
    do {
        nCubesOld = p->nCubes;
        Min_EsopRewrite( p );
        nIter++;
    }
    while ( 100.0*(nCubesOld - p->nCubes)/nCubesOld > 3.0 );

//    printf( "%d:%d->%d ", nIter, nCubesInit, p->nCubes );
}

/**Function*************************************************************

  Synopsis    [Performs one round of rewriting using distance 2 cubes.]

  Description [The weakness of this procedure is that it tries each cube
  with only one distance-2 cube. If this pair does not lead to improvement
  the cube is inserted into the cover anyhow, and we try another pair.
  A possible improvement would be to try this cube with all distance-2
  cubes, until an improvement is found, or until all such cubes are tried.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_EsopRewrite( Min_Man_t * p )
{
    Min_Cube_t * pCube, ** ppPrev;
    Min_Cube_t * pThis, ** ppPrevT;
    int v00, v01, v10, v11, Var0, Var1, Index, nCubesOld;
    int nPairs = 0;

    // insert the bubble before the first cube
    p->pBubble->pNext = p->ppStore[0];
    p->ppStore[0] = p->pBubble;
    p->pBubble->nLits = 0;

    // go through the cubes
    while ( 1 )
    {
        // get the index of the bubble
        Index = p->pBubble->nLits;

        // find the bubble
        Min_CoverForEachCubePrev( p->ppStore[Index], pCube, ppPrev )
            if ( pCube == p->pBubble )
                break;
        assert( pCube == p->pBubble );

        // remove the bubble, get the next cube after the bubble
        *ppPrev = p->pBubble->pNext;
        pCube = p->pBubble->pNext;
        if ( pCube == NULL )
            for ( Index++; Index <= p->nVars; Index++ )
                if ( p->ppStore[Index] )
                {
                    ppPrev = &(p->ppStore[Index]);
                    pCube = p->ppStore[Index];
                    break;
                }
        // stop if there is no more cubes
        if ( pCube == NULL )
            break;

        // find the first dist2 cube
        Min_CoverForEachCubePrev( pCube->pNext, pThis, ppPrevT )
            if ( Min_CubesDistTwo( pCube, pThis, &Var0, &Var1 ) )
                break;
        if ( pThis == NULL && Index < p->nVars )
        Min_CoverForEachCubePrev( p->ppStore[Index+1], pThis, ppPrevT )
            if ( Min_CubesDistTwo( pCube, pThis, &Var0, &Var1 ) )
                break;
        if ( pThis == NULL && Index < p->nVars - 1 )
        Min_CoverForEachCubePrev( p->ppStore[Index+2], pThis, ppPrevT )
            if ( Min_CubesDistTwo( pCube, pThis, &Var0, &Var1 ) )
                break;
        // continue if there is no dist2 cube
        if ( pThis == NULL )
        {
            // insert the bubble after the cube
            p->pBubble->pNext = pCube->pNext;
            pCube->pNext = p->pBubble;
            p->pBubble->nLits = pCube->nLits;
            continue;
        }
        nPairs++;

        // remove the cubes, insert the bubble instead of pCube
        *ppPrevT = pThis->pNext;
        *ppPrev = p->pBubble;
        p->pBubble->pNext = pCube->pNext;
        p->pBubble->nLits = pCube->nLits;
        p->nCubes -= 2;

        // Exorlink-2:
        // A{v00}     B{v01}      +  A{v10}     B{v11}     =
        // A{v00+v10} B{v01}      +  A{v10}     B{v01+v11} =
        // A{v00}     B{v01+v11}  +  A{v00+v10} B{v11}

        // save the dist2 parameters
        v00 = Min_CubeGetVar( pCube, Var0 );
        v01 = Min_CubeGetVar( pCube, Var1 );
        v10 = Min_CubeGetVar( pThis, Var0 );
        v11 = Min_CubeGetVar( pThis, Var1 );
//printf( "\n" );
//Min_CubeWrite( stdout, pCube );
//Min_CubeWrite( stdout, pThis );

        // derive the first pair of resulting cubes
        Min_CubeXorVar( pCube, Var0, v10 );
        pCube->nLits -= (v00 != 3);
        pCube->nLits += ((v00 ^ v10) != 3);
        Min_CubeXorVar( pThis, Var1, v01 );
        pThis->nLits -= (v11 != 3);
        pThis->nLits += ((v01 ^ v11) != 3);

        // add the cubes
        nCubesOld = p->nCubes;
        Min_EsopAddCube( p, pCube );
        Min_EsopAddCube( p, pThis );
        // check if the cubes were absorbed
        if ( p->nCubes < nCubesOld + 2 )
            continue;

        // pull out both cubes
        assert( pThis == p->ppStore[pThis->nLits] );
        p->ppStore[pThis->nLits] = pThis->pNext;
        assert( pCube == p->ppStore[pCube->nLits] );
        p->ppStore[pCube->nLits] = pCube->pNext;
        p->nCubes -= 2;

        // derive the second pair of resulting cubes
        Min_CubeXorVar( pCube, Var0, v10 );
        pCube->nLits -= ((v00 ^ v10) != 3);
        pCube->nLits += (v00 != 3);
        Min_CubeXorVar( pCube, Var1, v11 );
        pCube->nLits -= (v01 != 3);
        pCube->nLits += ((v01 ^ v11) != 3);

        Min_CubeXorVar( pThis, Var0, v00 );
        pThis->nLits -= (v10 != 3);
        pThis->nLits += ((v00 ^ v10) != 3);
        Min_CubeXorVar( pThis, Var1, v01 );
        pThis->nLits -= ((v01 ^ v11) != 3);
        pThis->nLits += (v11 != 3);

        // add them anyhow
        Min_EsopAddCube( p, pCube );
        Min_EsopAddCube( p, pThis );
    }
//    printf( "Pairs = %d  ", nPairs );
}

/**Function*************************************************************

  Synopsis    [Adds the cube to storage.]

  Description [Returns 0 if the cube is added or removed. Returns 1
  if the cube is glued with some other cube and has to be added again.
  Do not forget to clean the storage!]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Min_EsopAddCubeInt( Min_Man_t * p, Min_Cube_t * pCube )
{
    Min_Cube_t * pThis, ** ppPrev;
    // try to find the identical cube
    Min_CoverForEachCubePrev( p->ppStore[pCube->nLits], pThis, ppPrev )
    {
        if ( Min_CubesAreEqual( pCube, pThis ) )
        {
            *ppPrev = pThis->pNext;
            Min_CubeRecycle( p, pCube );
            Min_CubeRecycle( p, pThis );
            p->nCubes--;
            return 0;
        }
    }
    // find a distance-1 cube if it exists
    if ( pCube->nLits < pCube->nVars )
    Min_CoverForEachCubePrev( p->ppStore[pCube->nLits+1], pThis, ppPrev )
    {
        if ( Min_CubesDistOne( pCube, pThis, p->pTemp ) )
        {
            *ppPrev = pThis->pNext;
            Min_CubesTransform( pCube, pThis, p->pTemp );
            pCube->nLits++;
            Min_CubeRecycle( p, pThis );
            p->nCubes--;
            return 1;
        }
    }
    Min_CoverForEachCubePrev( p->ppStore[pCube->nLits], pThis, ppPrev )
    {
        if ( Min_CubesDistOne( pCube, pThis, p->pTemp ) )
        {
            *ppPrev = pThis->pNext;
            Min_CubesTransform( pCube, pThis, p->pTemp );
            pCube->nLits--;
            Min_CubeRecycle( p, pThis );
            p->nCubes--;
            return 1;
        }
    }
    if ( pCube->nLits > 0 )
    Min_CoverForEachCubePrev( p->ppStore[pCube->nLits-1], pThis, ppPrev )
    {
        if ( Min_CubesDistOne( pCube, pThis, p->pTemp ) )
        {
            *ppPrev = pThis->pNext;
            Min_CubesTransform( pCube, pThis, p->pTemp );
            Min_CubeRecycle( p, pThis );
            p->nCubes--;
            return 1;
        }
    }
    // add the cube
    pCube->pNext = p->ppStore[pCube->nLits];
    p->ppStore[pCube->nLits] = pCube;
    p->nCubes++;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Adds the cube to storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_EsopAddCube( Min_Man_t * p, Min_Cube_t * pCube )
{
    assert( pCube != p->pBubble );
    assert( (int)pCube->nLits == Min_CubeCountLits(pCube) );
    while ( Min_EsopAddCubeInt( p, pCube ) );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

