/**CFile****************************************************************

  FileName    [mvcContain.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Making the cover single-cube containment ABC_FREE.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcContain.c,v 1.4 2003/04/03 23:25:42 alanmi Exp $]

***********************************************************************/

#include "mvc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Mvc_CoverRemoveDuplicates( Mvc_Cover_t * pCover );
static void Mvc_CoverRemoveContained( Mvc_Cover_t * pCover );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Removes the contained cubes.]

  Description [Returns 1 if the cover has been changed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int  Mvc_CoverContain( Mvc_Cover_t * pCover )
{
    int nCubes;
    nCubes = Mvc_CoverReadCubeNum( pCover );
    if ( nCubes < 2 )
        return 0;
    Mvc_CoverSetCubeSizes(pCover);
    Mvc_CoverSort( pCover, NULL, Mvc_CubeCompareSizeAndInt );
    Mvc_CoverRemoveDuplicates( pCover );
    if ( nCubes > 1 )
        Mvc_CoverRemoveContained( pCover );
    return (nCubes != Mvc_CoverReadCubeNum(pCover));
}

/**Function*************************************************************

  Synopsis    [Removes adjacent duplicated cubes from the cube list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverRemoveDuplicates( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pPrev, * pCube, * pCube2;
    int  fEqual;

    // set the first cube of the cover
    pPrev = Mvc_CoverReadCubeHead(pCover);
    // go through all the cubes after this one
    Mvc_CoverForEachCubeStartSafe( Mvc_CubeReadNext(pPrev), pCube, pCube2 )
    {
        // compare the current cube with the prev cube
        Mvc_CubeBitEqual( fEqual, pPrev, pCube );
        if ( fEqual )
        { // they are equal - remove the current cube
            Mvc_CoverDeleteCube( pCover, pPrev, pCube );
            Mvc_CubeFree( pCover, pCube );
            // don't change the previous cube cube
        }
        else
        { // they are not equal - update the previous cube
            pPrev = pCube;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Removes contained cubes from the sorted cube list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverRemoveContained( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCubeBeg, * pCubeEnd, * pCubeLarge;
    Mvc_Cube_t * pCube, * pCube2, * pPrev;
    unsigned sizeCur;
    int  Result;

    // since the cubes are sorted by size, it is sufficient 
    // to compare each cube with other cubes that have larger sizes
    // if the given cube implies a larger cube, the larger cube is removed
    pCubeBeg = Mvc_CoverReadCubeHead(pCover);
    do
    {
        // get the current cube size
        sizeCur = Mvc_CubeReadSize(pCubeBeg);

        // initialize the end of the given size group
        pCubeEnd = pCubeBeg;
        // find the beginning of the next size group
        Mvc_CoverForEachCubeStart( Mvc_CubeReadNext(pCubeBeg), pCube )
        {
            if ( sizeCur == Mvc_CubeReadSize(pCube) ) 
                pCubeEnd = pCube;
            else // pCube is the first cube in the new size group
                break;
        }
        // if we could not find the next size group
        // the containment check is finished
        if ( pCube == NULL )
            break;
        // otherwise, pCubeBeg/pCubeEnd are the first/last cubes of the group

        // go through all the cubes between pCubeBeg and pCubeEnd, inclusive,
        // and for each of them, try removing cubes after pCubeEnd
        Mvc_CoverForEachCubeStart( pCubeBeg, pCubeLarge )
        {
            pPrev = pCubeEnd;
            Mvc_CoverForEachCubeStartSafe( Mvc_CubeReadNext(pCubeEnd), pCube, pCube2 )
            {
                // check containment
                Mvc_CubeBitNotImpl( Result, pCube, pCubeLarge );
                if ( !Result )
                { // pCubeLarge implies pCube - remove pCube
                    Mvc_CoverDeleteCube( pCover, pPrev, pCube );
                    Mvc_CubeFree( pCover, pCube );
                    // don't update the previous cube
                }
                else
                {   // update the previous cube
                    pPrev = pCube;
                }
            }
            // quit, if the main cube was the last one of this size
            if ( pCubeLarge == pCubeEnd )
                break;
        }

        // set the beginning of the next group
        pCubeBeg = Mvc_CubeReadNext(pCubeEnd);
    }
    while ( pCubeBeg );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

