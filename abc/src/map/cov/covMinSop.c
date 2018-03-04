/**CFile****************************************************************

  FileName    [covMinSop.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Mapping into network of SOPs/ESOPs.]

  Synopsis    [SOP manipulation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: covMinSop.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "covInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Min_SopRewrite( Min_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_SopMinimize( Min_Man_t * p )
{
    int nCubesInit, nCubesOld, nIter;
    if ( p->nCubes < 3 )
        return;
    nIter = 0;
    nCubesInit = p->nCubes;
    do {
        nCubesOld = p->nCubes;
        Min_SopRewrite( p );
        nIter++;
//    printf( "%d:%d->%d ", nIter, nCubesInit, p->nCubes );
    }
    while ( 100.0*(nCubesOld - p->nCubes)/nCubesOld > 3.0 );
//    printf( "\n" );

}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_SopRewrite( Min_Man_t * p )
{
    Min_Cube_t * pCube, ** ppPrev;
    Min_Cube_t * pThis, ** ppPrevT;
    Min_Cube_t * pTemp;
    int v00, v01, v10, v11, Var0, Var1, Index, fCont0, fCont1, nCubesOld;
    int nPairs = 0;
/*    
    {
        Min_Cube_t * pCover;
        pCover = Min_CoverCollect( p, p->nVars );
printf( "\n\n" );
Min_CoverWrite( stdout, pCover );
        Min_CoverExpand( p, pCover );
    }
*/

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
/*
printf( "\n" );
Min_CubeWrite( stdout, pCube );
Min_CubeWrite( stdout, pThis );
*/
        // remove the cubes, insert the bubble instead of pCube
        *ppPrevT = pThis->pNext;
        *ppPrev = p->pBubble;
        p->pBubble->pNext = pCube->pNext;
        p->pBubble->nLits = pCube->nLits;
        p->nCubes -= 2;

        assert( pCube != p->pBubble && pThis != p->pBubble );


        // save the dist2 parameters
        v00 = Min_CubeGetVar( pCube, Var0 );
        v01 = Min_CubeGetVar( pCube, Var1 );
        v10 = Min_CubeGetVar( pThis, Var0 );
        v11 = Min_CubeGetVar( pThis, Var1 );
        assert( v00 != v10 && v01 != v11 );
        assert( v00 != 3   || v01 != 3 );
        assert( v10 != 3   || v11 != 3 );

//printf( "\n" );
//Min_CubeWrite( stdout, pCube );
//Min_CubeWrite( stdout, pThis );

//printf( "\n" );
//Min_CubeWrite( stdout, pCube );
//Min_CubeWrite( stdout, pThis );

        // consider the case when both cubes have non-empty literals
        if ( v00 != 3 && v01 != 3 && v10 != 3 && v11 != 3 )
        {
            assert( v00 == (v10 ^ 3) );
            assert( v01 == (v11 ^ 3) );
            // create the temporary cube equal to the first corner
            Min_CubeXorVar( pCube, Var0, 3 );
            // check if this cube is contained
            fCont0 = Min_CoverContainsCube( p, pCube );
            // create the temporary cube equal to the first corner
            Min_CubeXorVar( pCube, Var0, 3 );
            Min_CubeXorVar( pCube, Var1, 3 );
//printf( "\n" );
//Min_CubeWrite( stdout, pCube );
//Min_CubeWrite( stdout, pThis );
            // check if this cube is contained
            fCont1 = Min_CoverContainsCube( p, pCube );
            // undo the change
            Min_CubeXorVar( pCube, Var1, 3 );

            // check if the cubes can be overwritten
            if ( fCont0 && fCont1 )
            {
                // one of the cubes can be recycled, the other expanded and added
                Min_CubeRecycle( p, pThis );
                // remove the literals
                Min_CubeXorVar( pCube, Var0, v00 ^ 3 );
                Min_CubeXorVar( pCube, Var1, v01 ^ 3 );
                pCube->nLits -= 2;
                Min_SopAddCube( p, pCube );
            }
            else if ( fCont0 )
            {
                // expand both cubes and add them
                Min_CubeXorVar( pCube, Var0, v00 ^ 3 );
                pCube->nLits--;
                Min_SopAddCube( p, pCube );
                Min_CubeXorVar( pThis, Var1, v11 ^ 3 );
                pThis->nLits--;
                Min_SopAddCube( p, pThis );
            }
            else if ( fCont1 )
            {
                // expand both cubes and add them
                Min_CubeXorVar( pCube, Var1, v01 ^ 3 );
                pCube->nLits--;
                Min_SopAddCube( p, pCube );
                Min_CubeXorVar( pThis, Var0, v10 ^ 3 );
                pThis->nLits--;
                Min_SopAddCube( p, pThis );
            }
            else
            {
                Min_SopAddCube( p, pCube );
                Min_SopAddCube( p, pThis );
            }
            // otherwise, no change is possible
            continue;
        }

        // if one of them does not have DC lit, move it
        if ( v00 != 3 && v01 != 3 )
        {
            assert( v10 == 3 || v11 == 3 );
            pTemp = pCube; pCube = pThis; pThis = pTemp;
            Index = v00; v00 = v10; v10 = Index;
            Index = v01; v01 = v11; v11 = Index;
        }

        // make sure the first cube has first var DC
        if ( v00 != 3 )
        {
            assert( v01 == 3 );
            Index = Var0; Var0 = Var1; Var1 = Index;
            Index = v00; v00 = v01; v01 = Index;
            Index = v10; v10 = v11; v11 = Index;
        }

        // consider both cases: both have DC lit
        if ( v00 == 3 && v11 == 3 )
        {
            assert( v01 != 3 && v10 != 3 );
            // try the remaining minterm
            // create the temporary cube equal to the first corner
            Min_CubeXorVar( pCube, Var0, v10 );
            Min_CubeXorVar( pCube, Var1, 3   );
            pCube->nLits++;
            // check if this cube is contained
            fCont0 = Min_CoverContainsCube( p, pCube );
            // undo the cube transformations
            Min_CubeXorVar( pCube, Var0, v10 );
            Min_CubeXorVar( pCube, Var1, 3   );
            pCube->nLits--;
            // check the case when both are covered
            if ( fCont0 )
            {
                // one of the cubes can be recycled, the other expanded and added
                Min_CubeRecycle( p, pThis );
                // remove the literals
                Min_CubeXorVar( pCube, Var1, v01 ^ 3 );
                pCube->nLits--;
                Min_SopAddCube( p, pCube );
            }
            else
            {
                // try two reduced cubes
                Min_CubeXorVar( pCube, Var0, v10 );
                pCube->nLits++;
                // remember the cubes
                nCubesOld = p->nCubes;
                Min_SopAddCube( p, pCube );
                // check if the cube is absorbed
                if ( p->nCubes < nCubesOld + 1 )
                { // absorbed - add the second cube
                    Min_SopAddCube( p, pThis );
                }
                else
                { // remove this cube, and try another one
                    assert( pCube == p->ppStore[pCube->nLits] );
                    p->ppStore[pCube->nLits] = pCube->pNext;
                    p->nCubes--;

                    // return the cube to the previous state
                    Min_CubeXorVar( pCube, Var0, v10 );
                    pCube->nLits--;

                    // generate another reduced cube
                    Min_CubeXorVar( pThis, Var1, v01 );
                    pThis->nLits++;

                    // add both cubes
                    Min_SopAddCube( p, pCube );
                    Min_SopAddCube( p, pThis );
                }
            }
        }
        else // the first cube has DC lit
        {
            assert( v01 != 3 && v10 != 3 && v11 != 3 );
            // try the remaining minterm
            // create the temporary cube equal to the minterm
            Min_CubeXorVar( pThis, Var0, 3 );
            // check if this cube is contained
            fCont0 = Min_CoverContainsCube( p, pThis );
            // undo the cube transformations
            Min_CubeXorVar( pThis, Var0, 3 );
            // check the case when both are covered
            if ( fCont0 )
            {
                // one of the cubes can be recycled, the other expanded and added
                Min_CubeRecycle( p, pThis );
                // remove the literals
                Min_CubeXorVar( pCube, Var1, v01 ^ 3 );
                pCube->nLits--;
                Min_SopAddCube( p, pCube );
            }
            else
            {
                // try reshaping the cubes
                // reduce the first cube
                Min_CubeXorVar( pCube, Var0, v10 );
                pCube->nLits++;
                // expand the second cube
                Min_CubeXorVar( pThis, Var1, v11 ^ 3 );
                pThis->nLits--;
                // add both cubes
                Min_SopAddCube( p, pCube );
                Min_SopAddCube( p, pThis );
            }
        }
    }
//    printf( "Pairs = %d  ", nPairs );
}

/**Function*************************************************************

  Synopsis    [Adds cube to the SOP cover stored in the manager.]

  Description [Returns 0 if the cube is added or removed. Returns 1
  if the cube is glued with some other cube and has to be added again.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Min_SopAddCubeInt( Min_Man_t * p, Min_Cube_t * pCube )
{
    Min_Cube_t * pThis, * pThis2, ** ppPrev;
    int i;
    // try to find the identical cube
    Min_CoverForEachCube( p->ppStore[pCube->nLits], pThis )
    {
        if ( Min_CubesAreEqual( pCube, pThis ) )
        {
            Min_CubeRecycle( p, pCube );
            return 0;
        }
    }
    // try to find a containing cube
    for ( i = 0; i < (int)pCube->nLits; i++ )
    Min_CoverForEachCube( p->ppStore[i], pThis )
    {
        if ( pThis != p->pBubble && Min_CubeIsContained( pThis, pCube ) )
        {
            Min_CubeRecycle( p, pCube );
            return 0;
        }
    }
    // try to find distance one in the same bin
    Min_CoverForEachCubePrev( p->ppStore[pCube->nLits], pThis, ppPrev )
    {
        if ( Min_CubesDistOne( pCube, pThis, NULL ) )
        {
            *ppPrev = pThis->pNext;
            Min_CubesTransformOr( pCube, pThis );
            pCube->nLits--;
            Min_CubeRecycle( p, pThis );
            p->nCubes--;
            return 1;
        }
    }

    // clean the other cubes using this one
    for ( i = pCube->nLits + 1; i <= (int)pCube->nVars; i++ )
    {
        ppPrev = &p->ppStore[i];
        Min_CoverForEachCubeSafe( p->ppStore[i], pThis, pThis2 )
        {
            if ( pThis != p->pBubble && Min_CubeIsContained( pCube, pThis ) )
            {
                *ppPrev = pThis->pNext;
                Min_CubeRecycle( p, pThis );
                p->nCubes--;
            }
            else
                ppPrev = &pThis->pNext;
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
void Min_SopAddCube( Min_Man_t * p, Min_Cube_t * pCube )
{
    assert( Min_CubeCheck( pCube ) );
    assert( pCube != p->pBubble );
    assert( (int)pCube->nLits == Min_CubeCountLits(pCube) );
    while ( Min_SopAddCubeInt( p, pCube ) );
}





/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_SopContain( Min_Man_t * p )
{
    Min_Cube_t * pCube, * pCube2, ** ppPrev;
    int i, k;
    for ( i = 0; i <= p->nVars; i++ )
    {
        Min_CoverForEachCube( p->ppStore[i], pCube )
        Min_CoverForEachCubePrev( pCube->pNext, pCube2, ppPrev )
        {
            if ( !Min_CubesAreEqual( pCube, pCube2 ) )
                continue;
            *ppPrev = pCube2->pNext;
            Min_CubeRecycle( p, pCube2 );
            p->nCubes--;
        }
        for ( k = i + 1; k <= p->nVars; k++ )
        Min_CoverForEachCubePrev( p->ppStore[k], pCube2, ppPrev )
        {
            if ( !Min_CubeIsContained( pCube, pCube2 ) )
                continue;
            *ppPrev = pCube2->pNext;
            Min_CubeRecycle( p, pCube2 );
            p->nCubes--;
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Min_SopDist1Merge( Min_Man_t * p )
{
    Min_Cube_t * pCube, * pCube2, * pCubeNew;
    int i;
    for ( i = p->nVars; i >= 0; i-- )
    {
        Min_CoverForEachCube( p->ppStore[i], pCube )
        Min_CoverForEachCube( pCube->pNext, pCube2 )
        {
            assert( pCube->nLits == pCube2->nLits );
            if ( !Min_CubesDistOne( pCube, pCube2, NULL ) )
                continue;
            pCubeNew = Min_CubesXor( p, pCube, pCube2 );
            assert( pCubeNew->nLits == pCube->nLits - 1 );
            pCubeNew->pNext = p->ppStore[pCubeNew->nLits];
            p->ppStore[pCubeNew->nLits] = pCubeNew;
            p->nCubes++;
        }
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Min_Cube_t * Min_SopComplement( Min_Man_t * p, Min_Cube_t * pSharp )
{
     Vec_Int_t * vVars;
     Min_Cube_t * pCover, * pCube, * pNext, * pReady, * pThis, ** ppPrev;
     int Num, Value, i;

     // get the variables
     vVars = Vec_IntAlloc( 100 );
    // create the tautology cube
     pCover = Min_CubeAlloc( p );
     // sharp it with all cubes
     Min_CoverForEachCube( pSharp, pCube )
     Min_CoverForEachCubePrev( pCover, pThis, ppPrev )
     {
        if ( Min_CubesDisjoint( pThis, pCube ) )
            continue;
        // remember the next pointer
        pNext = pThis->pNext;
        // get the variables, in which pThis is '-' while pCube is fixed
        Min_CoverGetDisjVars( pThis, pCube, vVars );
        // generate the disjoint cubes
        pReady = pThis;
        Vec_IntForEachEntryReverse( vVars, Num, i )
        {
            // correct the literal
            Min_CubeXorVar( pReady, vVars->pArray[i], 3 );
            if ( i == 0 )
                break;
            // create the new cube and clean this value
            Value = Min_CubeGetVar( pReady, vVars->pArray[i] );
            pReady = Min_CubeDup( p, pReady );
            Min_CubeXorVar( pReady, vVars->pArray[i], 3 ^ Value );
            // add to the cover
            *ppPrev = pReady;
            ppPrev = &pReady->pNext;
        }
        pThis = pReady;
        pThis->pNext = pNext;
     }
     Vec_IntFree( vVars );

     // perform dist-1 merge and contain
     Min_CoverExpandRemoveEqual( p, pCover );
     Min_SopDist1Merge( p );
     Min_SopContain( p );
     return Min_CoverCollect( p, p->nVars );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Min_SopCheck( Min_Man_t * p )
{
    Min_Cube_t * pCube, * pThis;
    int i;

    pCube = Min_CubeAlloc( p );
    Min_CubeXorBit( pCube, 2*0+1 );
    Min_CubeXorBit( pCube, 2*1+1 );
    Min_CubeXorBit( pCube, 2*2+0 );
    Min_CubeXorBit( pCube, 2*3+0 );
    Min_CubeXorBit( pCube, 2*4+0 );
    Min_CubeXorBit( pCube, 2*5+1 );
    Min_CubeXorBit( pCube, 2*6+1 );
    pCube->nLits = 7;

//    Min_CubeWrite( stdout, pCube );

    // check that the cubes contain it
    for ( i = 0; i <= p->nVars; i++ )
        Min_CoverForEachCube( p->ppStore[i], pThis )
            if ( pThis != p->pBubble && Min_CubeIsContained( pThis, pCube ) )
            {
                Min_CubeRecycle( p, pCube );
                return 1;
            }
    Min_CubeRecycle( p, pCube );
    return 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

