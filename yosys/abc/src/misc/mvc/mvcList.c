/**CFile****************************************************************

  FileName    [mvcList.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Manipulating list of cubes in the cover.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcList.c,v 1.4 2003/04/03 06:31:50 alanmi Exp $]

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
void Mvc_ListAddCubeHead_( Mvc_List_t * pList, Mvc_Cube_t * pCube )
{
    if ( pList->pHead == NULL )
    {
        Mvc_CubeSetNext( pCube, NULL );
        pList->pHead = pCube;
        pList->pTail = pCube;
    }
    else
    {
        Mvc_CubeSetNext( pCube, pList->pHead );
        pList->pHead = pCube;
    }
    pList->nItems++;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_ListAddCubeTail_( Mvc_List_t * pList, Mvc_Cube_t * pCube )
{
    if ( pList->pHead == NULL )
        pList->pHead = pCube;
    else
        Mvc_CubeSetNext( pList->pTail, pCube );
    pList->pTail    = pCube;
    Mvc_CubeSetNext( pCube, NULL );
    pList->nItems++;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_ListDeleteCube_( Mvc_List_t * pList, Mvc_Cube_t * pPrev, Mvc_Cube_t * pCube )
{
    if ( pPrev == NULL ) // deleting the head cube
        pList->pHead = Mvc_CubeReadNext(pCube);
    else
        pPrev->pNext = pCube->pNext;
    if ( pList->pTail == pCube ) // deleting the tail cube
    {
        assert( Mvc_CubeReadNext(pCube) == NULL );
        pList->pTail = pPrev;
    }
    pList->nItems--;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverAddCubeHead_( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    Mvc_List_t * pList = &pCover->lCubes;
    if ( pList->pHead == NULL )
    {
        Mvc_CubeSetNext( pCube, NULL );
        pList->pHead = pCube;
        pList->pTail = pCube;
    }
    else
    {
        Mvc_CubeSetNext( pCube, pList->pHead );
        pList->pHead = pCube;
    }
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverAddCubeTail_( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    Mvc_List_t * pList = &pCover->lCubes;

    if ( pList->pHead == NULL )
        pList->pHead = pCube;
    else
        Mvc_CubeSetNext( pList->pTail, pCube );
    pList->pTail    = pCube;
    Mvc_CubeSetNext( pCube, NULL );
    pList->nItems++;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDeleteCube_( Mvc_Cover_t * pCover, Mvc_Cube_t * pPrev, Mvc_Cube_t * pCube )
{
    Mvc_List_t * pList = &pCover->lCubes;
 
    if ( pPrev == NULL ) // deleting the head cube
        pList->pHead = Mvc_CubeReadNext(pCube);
    else
        pPrev->pNext = pCube->pNext;
    if ( pList->pTail == pCube ) // deleting the tail cube
    {
        assert( Mvc_CubeReadNext(pCube) == NULL );
        pList->pTail = pPrev;
    }
    pList->nItems--;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverAddDupCubeHead( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    Mvc_Cube_t * pCubeNew;
    pCubeNew = Mvc_CubeAlloc( pCover );
    Mvc_CubeBitCopy( pCubeNew, pCube );
    Mvc_CoverAddCubeHead( pCover, pCubeNew );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverAddDupCubeTail( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
    Mvc_Cube_t * pCubeNew;
    // copy the cube as part of this cover
    pCubeNew = Mvc_CubeAlloc( pCover );
    Mvc_CubeBitCopy( pCubeNew, pCube );
    // clean the last bits of the new cube
//    pCubeNew->pData[pCubeNew->iLast] &= (BITS_FULL >> pCubeNew->nUnused);
    // add the cube at the end
    Mvc_CoverAddCubeTail( pCover, pCubeNew );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverAddLiteralsOfCube( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
//    int iBit, Value;
//    assert( pCover->pLits );
//    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
//        if ( Value )
//            pCover->pLits[iBit] += Value;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDeleteLiteralsOfCube( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube )
{
//    int iBit, Value;
//    assert( pCover->pLits );
//    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
//        if ( Value )
//            pCover->pLits[iBit] -= Value;
}


/**Function*************************************************************

  Synopsis    [Transfers the cubes from the list into the array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverList2Array( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int Counter;
    // resize storage if necessary
    Mvc_CoverAllocateArrayCubes( pCover );
    // iterate through the cubes
    Counter = 0;
    Mvc_CoverForEachCube( pCover, pCube )
        pCover->pCubes[ Counter++ ] = pCube;
    assert( Counter == Mvc_CoverReadCubeNum(pCover) );
}

/**Function*************************************************************

  Synopsis    [Transfers the cubes from the array into list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverArray2List( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int nCubes, i;

    assert( pCover->pCubes );

    nCubes = Mvc_CoverReadCubeNum(pCover);
    if ( nCubes == 0 )
        return;
    if ( nCubes == 1 )
    {
        pCube    = pCover->pCubes[0];
        pCube->pNext = NULL;
        pCover->lCubes.pHead = pCover->lCubes.pTail = pCube;
        return;
    }
    // set up the first cube
    pCube = pCover->pCubes[0];
    pCover->lCubes.pHead = pCube;
    // set up the last cube
    pCube = pCover->pCubes[nCubes-1];
    pCube->pNext = NULL;
    pCover->lCubes.pTail = pCube;

    // link all cubes starting from the first one
    for ( i = 0; i < nCubes - 1; i++ )
        pCover->pCubes[i]->pNext = pCover->pCubes[i+1];
}

/**Function*************************************************************

  Synopsis    [Returns the tail of the linked list given by the head.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cube_t * Mvc_ListGetTailFromHead( Mvc_Cube_t * pHead )
{
    Mvc_Cube_t * pCube, * pTail;
    for ( pTail = pCube = pHead; 
          pCube; 
          pTail = pCube, pCube = Mvc_CubeReadNext(pCube) );
    return pTail;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

