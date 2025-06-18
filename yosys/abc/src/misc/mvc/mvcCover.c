/**CFile****************************************************************

  FileName    [mvcCover.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Basic procedures to manipulate unate cube covers.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcCover.c,v 1.5 2003/04/09 18:02:05 alanmi Exp $]

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
Mvc_Cover_t * Mvc_CoverAlloc( Mvc_Manager_t * pMem, int nBits )
{
    Mvc_Cover_t * p;
    int nBitsInUnsigned;

    nBitsInUnsigned  = 8 * sizeof(Mvc_CubeWord_t);
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    p                = (Mvc_Cover_t *)ABC_ALLOC( char, sizeof(Mvc_Cover_t) );
#else
    p                = (Mvc_Cover_t *)Extra_MmFixedEntryFetch( pMem->pManC );
#endif
    p->pMem          = pMem;
    p->nBits         = nBits;
    p->nWords        = nBits / nBitsInUnsigned + (int)(nBits % nBitsInUnsigned > 0);
    p->nUnused       = p->nWords * nBitsInUnsigned - p->nBits;
    p->lCubes.nItems = 0;
    p->lCubes.pHead  = NULL;
    p->lCubes.pTail  = NULL;
    p->nCubesAlloc   = 0;
    p->pCubes        = NULL;
    p->pMask         = NULL;
    p->pLits         = NULL;
    return p;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverClone( Mvc_Cover_t * p )
{
    Mvc_Cover_t * pCover;
#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    pCover                = (Mvc_Cover_t *)ABC_ALLOC( char, sizeof(Mvc_Cover_t) );
#else
    pCover                = (Mvc_Cover_t *)Extra_MmFixedEntryFetch( p->pMem->pManC );
#endif
    pCover->pMem          = p->pMem;
    pCover->nBits         = p->nBits;
    pCover->nWords        = p->nWords;
    pCover->nUnused       = p->nUnused;
    pCover->lCubes.nItems = 0;
    pCover->lCubes.pHead  = NULL;
    pCover->lCubes.pTail  = NULL;
    pCover->nCubesAlloc   = 0;
    pCover->pCubes        = NULL;
    pCover->pMask         = NULL;
    pCover->pLits         = NULL;
    return pCover;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverDup( Mvc_Cover_t * p )
{
    Mvc_Cover_t * pCover;
    Mvc_Cube_t * pCube, * pCubeCopy;
    // clone the cover
    pCover = Mvc_CoverClone( p );
    // copy the cube list
    Mvc_CoverForEachCube( p, pCube )
    {
        pCubeCopy = Mvc_CubeDup( p, pCube );
        Mvc_CoverAddCubeTail( pCover, pCubeCopy );
    }
    return pCover;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverFree( Mvc_Cover_t * p )
{
    Mvc_Cube_t * pCube, * pCube2;
    // recycle cube list
    Mvc_CoverForEachCubeSafe( p, pCube, pCube2 )
        Mvc_CubeFree( p, pCube );
    // recycle other pointers
    Mvc_CubeFree( p, p->pMask );
    MEM_FREE( p->pMem, Mvc_Cube_t *, p->nCubesAlloc, p->pCubes );
    MEM_FREE( p->pMem, int,          p->nBits,       p->pLits  );

#ifdef USE_SYSTEM_MEMORY_MANAGEMENT
    ABC_FREE( p );
#else
    Extra_MmFixedEntryRecycle( p->pMem->pManC, (char *)p );
#endif
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverAllocateMask( Mvc_Cover_t * pCover ) 
{ 
    if ( pCover->pMask == NULL )
        pCover->pMask = Mvc_CubeAlloc( pCover );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverAllocateArrayLits( Mvc_Cover_t * pCover ) 
{ 
    if ( pCover->pLits == NULL )
        pCover->pLits = MEM_ALLOC( pCover->pMem, int, pCover->nBits );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverAllocateArrayCubes( Mvc_Cover_t * pCover ) 
{ 
    if ( pCover->nCubesAlloc < pCover->lCubes.nItems )
    {
        if ( pCover->nCubesAlloc > 0 )
            MEM_FREE( pCover->pMem, Mvc_Cube_t *, pCover->nCubesAlloc, pCover->pCubes );
        pCover->nCubesAlloc = pCover->lCubes.nItems;
        pCover->pCubes = MEM_ALLOC( pCover->pMem, Mvc_Cube_t *, pCover->nCubesAlloc );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDeallocateMask( Mvc_Cover_t * pCover ) 
{ 
    Mvc_CubeFree( pCover, pCover->pMask );
    pCover->pMask = NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDeallocateArrayLits( Mvc_Cover_t * pCover ) 
{ 
    if ( pCover->pLits )
    {
        MEM_FREE( pCover->pMem, int, pCover->nBits, pCover->pLits );
        pCover->pLits = NULL;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

