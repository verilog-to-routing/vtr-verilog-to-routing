/**CFile****************************************************************

  FileName    [mvcOperAlg.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Miscellaneous operations on covers.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcOpAlg.c,v 1.4 2003/04/26 20:41:36 alanmi Exp $]

***********************************************************************/

#include "mvc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Multiplies two disjoint-support covers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverAlgebraicMultiply( Mvc_Cover_t * pCover1, Mvc_Cover_t * pCover2 )
{
    Mvc_Cover_t * pCover;
    Mvc_Cube_t * pCube1, * pCube2, * pCube;
    int CompResult;

    // covers should be the same base
    assert( pCover1->nBits == pCover2->nBits );
    // make sure that supports do not overlap
    Mvc_CoverAllocateMask( pCover1 );
    Mvc_CoverAllocateMask( pCover2 );
    Mvc_CoverSupport( pCover1, pCover1->pMask );
    Mvc_CoverSupport( pCover2, pCover2->pMask );
    // check if the cubes are bit-wise disjoint
    Mvc_CubeBitDisjoint( CompResult, pCover1->pMask, pCover2->pMask );
    if ( !CompResult )
        printf( "Mvc_CoverMultiply(): Cover supports are not disjoint!\n" );

    // iterate through the cubes
    pCover = Mvc_CoverClone( pCover1 );
    Mvc_CoverForEachCube( pCover1, pCube1 )
        Mvc_CoverForEachCube( pCover2, pCube2 )
        { 
            // create the product cube
            pCube = Mvc_CubeAlloc( pCover );
            // set the product cube equal to the product of the two cubes
            Mvc_CubeBitOr( pCube, pCube1, pCube2 );
            // add the cube to the cover
            Mvc_CoverAddCubeTail( pCover, pCube );
        }
    return pCover;
}


/**Function*************************************************************

  Synopsis    [Subtracts the second cover from the first.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverAlgebraicSubtract( Mvc_Cover_t * pCover1, Mvc_Cover_t * pCover2 )
{
    Mvc_Cover_t * pCover;
    Mvc_Cube_t * pCube1, * pCube2, * pCube;
    int fFound;
    int CompResult;

    // covers should be the same base
    assert( pCover1->nBits == pCover2->nBits );

    // iterate through the cubes
    pCover = Mvc_CoverClone( pCover1 );
    Mvc_CoverForEachCube( pCover1, pCube1 )
    {
        fFound = 0;
        Mvc_CoverForEachCube( pCover2, pCube2 )
        {
            Mvc_CubeBitEqual( CompResult, pCube1, pCube2 );
            if ( CompResult )
            {
                fFound = 1;
                break;
            }
        }
        if ( !fFound )
        { 
            // create the copy of the cube
            pCube = Mvc_CubeDup( pCover, pCube1 );
            // add the cube copy to the cover
            Mvc_CoverAddCubeTail( pCover, pCube );
        }
    }
    return pCover;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverAlgebraicEqual( Mvc_Cover_t * pCover1, Mvc_Cover_t * pCover2 )
{
    Mvc_Cube_t * pCube1, * pCube2;
    int fFound;
    int CompResult;

    // covers should be the same base
    assert( pCover1->nBits == pCover2->nBits );
    // iterate through the cubes
    Mvc_CoverForEachCube( pCover1, pCube1 )
    {
        fFound = 0;
        Mvc_CoverForEachCube( pCover2, pCube2 )
        {
            Mvc_CubeBitEqual( CompResult, pCube1, pCube2 );
            if ( CompResult )
            {
                fFound = 1;
                break;
            }
        }
        if ( !fFound )
            return 0;
    }
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

