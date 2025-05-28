/**CFile****************************************************************

  FileName    [mvcSort.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Sorting cubes in the cover with the mask.]

  Author      [MVSIS Group]
  
  Affiliation [uC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcSort.c,v 1.5 2003/04/27 01:03:45 wjiang Exp $]

***********************************************************************/

#include "mvc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////


Mvc_Cube_t * Mvc_CoverSort_rec( Mvc_Cube_t * pList, int nItems, Mvc_Cube_t * pMask, int (* pCompareFunc)(Mvc_Cube_t *, Mvc_Cube_t *, Mvc_Cube_t *) );
Mvc_Cube_t * Mvc_CoverSortMerge( Mvc_Cube_t * pList1, Mvc_Cube_t * pList2, Mvc_Cube_t * pMask, int (* pCompareFunc)(Mvc_Cube_t *, Mvc_Cube_t *, Mvc_Cube_t *) );

////////////////////////////////////////////////////////////////////////
///                     FuNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Sorts cubes using the given cost function.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverSort( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask, int (* pCompareFunc)(Mvc_Cube_t *, Mvc_Cube_t *, Mvc_Cube_t *) )
{
    Mvc_Cube_t * pHead;
    int nCubes;
    // one cube does not need sorting
    nCubes = Mvc_CoverReadCubeNum(pCover);
    if ( nCubes <= 1 )
        return;
    // sort the cubes 
    pHead = Mvc_CoverSort_rec( Mvc_CoverReadCubeHead(pCover), nCubes, pMask, pCompareFunc ); 
    // insert the sorted list into the cover
    Mvc_CoverSetCubeHead( pCover, pHead );
    Mvc_CoverSetCubeTail( pCover, Mvc_ListGetTailFromHead(pHead) );
    // make sure that the list is sorted in the increasing order
    assert( pCompareFunc( Mvc_CoverReadCubeHead(pCover), Mvc_CoverReadCubeTail(pCover), pMask ) <= 0 );
}

/**Function*************************************************************

  Synopsis    [Recursive part of Mvc_CoverSort()]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cube_t * Mvc_CoverSort_rec( Mvc_Cube_t * pList, int nItems, Mvc_Cube_t * pMask, int (* pCompareFunc)(Mvc_Cube_t *, Mvc_Cube_t *, Mvc_Cube_t *) )
{
    Mvc_Cube_t * pList1, * pList2;
    int nItems1, nItems2, i;

    // trivial case
    if ( nItems == 1 )
    {
        Mvc_CubeSetNext( pList, NULL );
        return pList;
    }

    // select the new sizes
    nItems1 = nItems/2;
    nItems2 = nItems - nItems1;

    // set the new beginnings
    pList1 = pList2 = pList;
    for ( i = 0; i < nItems1; i++ )
        pList2 = Mvc_CubeReadNext( pList2 );

    // solve recursively
    pList1 = Mvc_CoverSort_rec( pList1, nItems1, pMask, pCompareFunc );
    pList2 = Mvc_CoverSort_rec( pList2, nItems2, pMask, pCompareFunc );

    // merge
    return Mvc_CoverSortMerge( pList1, pList2, pMask, pCompareFunc );
}


/**Function*************************************************************

  Synopsis    [Merges two NULL-terminated linked lists.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cube_t * Mvc_CoverSortMerge( Mvc_Cube_t * pList1, Mvc_Cube_t * pList2, Mvc_Cube_t * pMask, int (* pCompareFunc)(Mvc_Cube_t *, Mvc_Cube_t *, Mvc_Cube_t *) )
{
    Mvc_Cube_t * pList = NULL, ** ppTail = &pList;
    Mvc_Cube_t * pCube;
    while ( pList1 && pList2 )
    {
        if ( pCompareFunc( pList1, pList2, pMask ) < 0 )
        {
            pCube = pList1;
            pList1 = Mvc_CubeReadNext(pList1);
        }
        else
        {
            pCube = pList2;
            pList2 = Mvc_CubeReadNext(pList2);
        }
        *ppTail = pCube;
        ppTail = Mvc_CubeReadNextP(pCube);
    }
    *ppTail = pList1? pList1: pList2;
    return pList;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

