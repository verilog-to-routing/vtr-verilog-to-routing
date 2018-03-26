/**CFile****************************************************************

  FileName    [mvcCube.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Manipulating unate cubes.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcCube.c,v 1.4 2003/04/03 06:31:49 alanmi Exp $]

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

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cube_t * Mvc_CubeAlloc( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;

    assert( pCover->nWords >= 0 );
    // allocate the cube
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    if ( pCover->nWords == 0 )
        pCube = (Mvc_Cube_t *)ABC_ALLOC( char, sizeof(Mvc_Cube_t) );
    else
        pCube = (Mvc_Cube_t *)ABC_ALLOC( char,  sizeof(Mvc_Cube_t) + sizeof(Mvc_CubeWord_t) * (pCover->nWords - 1) );
#else
    switch( pCover->nWords )
    {
    case 0:
    case 1:
        pCube = (Mvc_Cube_t *)Extra_MmFixedEntryFetch( pCover->pMem->pMan1 );
        break;
    case 2:
        pCube = (Mvc_Cube_t *)Extra_MmFixedEntryFetch( pCover->pMem->pMan2 );
        break;
    case 3:
    case 4:
        pCube = (Mvc_Cube_t *)Extra_MmFixedEntryFetch( pCover->pMem->pMan4 );
        break;
    default:
        pCube = (Mvc_Cube_t *)ABC_ALLOC( char, sizeof(Mvc_Cube_t) + sizeof(Mvc_CubeWord_t) * (pCover->nWords - 1) );
        break;
    }
#endif
    // set the parameters charactering this cube
    if ( pCover->nWords == 0 )
        pCube->iLast   = pCover->nWords;
    else
        pCube->iLast   = pCover->nWords - 1;
    pCube->nUnused = pCover->nUnused;
    return pCube;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cube_t * Mvc_CubeDup( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    Mvc_Cube_t * pCubeCopy;
    pCubeCopy = Mvc_CubeAlloc( pCover );
    Mvc_CubeBitCopy( pCubeCopy, pCube );
    return pCubeCopy;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CubeFree( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    if ( pCube == NULL )
        return;

    // verify the parameters charactering this cube
    assert( pCube->iLast == 0 || ((int)pCube->iLast) == pCover->nWords - 1 );
    assert( ((int)pCube->nUnused) == pCover->nUnused );

    // deallocate the cube
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    ABC_FREE( pCube );
#else
    switch( pCover->nWords )
    {
    case 0:
    case 1:
        Extra_MmFixedEntryRecycle( pCover->pMem->pMan1, (char *)pCube );
        break;
    case 2:
        Extra_MmFixedEntryRecycle( pCover->pMem->pMan2, (char *)pCube );
        break;
    case 3:
    case 4:
        Extra_MmFixedEntryRecycle( pCover->pMem->pMan4, (char *)pCube );
        break;
    default:
        ABC_FREE( pCube );
        break;
    }
#endif
}


/**Function*************************************************************

  Synopsis    [Removes the don't-care variable from the cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CubeBitRemoveDcs( Mvc_Cube_t * pCube )
{
    unsigned Mask;
    int i;
    for ( i = Mvc_CubeReadLast(pCube); i >= 0; i-- )
    {
        // detect those variables that are different (not DCs)
        Mask = (pCube->pData[i] ^ (pCube->pData[i] >> 1)) & BITS_DISJOINT; 
        // create the mask of all that are different
        Mask |= (Mask << 1);
        // remove other bits from the set
        pCube->pData[i] &= Mask;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

