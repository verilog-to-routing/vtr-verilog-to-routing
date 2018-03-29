/**CFile****************************************************************

  FileName    [mvcApi.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcApi.c,v 1.4 2003/04/03 06:31:48 alanmi Exp $]

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
int          Mvc_CoverReadWordNum( Mvc_Cover_t * pCover )   { return pCover->nWords;         }
int          Mvc_CoverReadBitNum( Mvc_Cover_t * pCover )    { return pCover->nBits;          }
int          Mvc_CoverReadCubeNum( Mvc_Cover_t * pCover )   { return pCover->lCubes.nItems; }
Mvc_Cube_t * Mvc_CoverReadCubeHead( Mvc_Cover_t * pCover )  { return pCover->lCubes.pHead;  }
Mvc_Cube_t * Mvc_CoverReadCubeTail( Mvc_Cover_t * pCover )  { return pCover->lCubes.pTail;  }
Mvc_List_t * Mvc_CoverReadCubeList( Mvc_Cover_t * pCover )  { return &pCover->lCubes;        }


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int          Mvc_ListReadCubeNum( Mvc_List_t * pList )   { return pList->nItems; }
Mvc_Cube_t * Mvc_ListReadCubeHead( Mvc_List_t * pList )  { return pList->pHead;  }
Mvc_Cube_t * Mvc_ListReadCubeTail( Mvc_List_t * pList )  { return pList->pTail;  }

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void         Mvc_CoverSetCubeNum( Mvc_Cover_t * pCover,int nItems )           { pCover->lCubes.nItems = nItems; }
void         Mvc_CoverSetCubeHead( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube ) { pCover->lCubes.pHead = pCube;   }
void         Mvc_CoverSetCubeTail( Mvc_Cover_t * pCover, Mvc_Cube_t * pCube ) { pCover->lCubes.pTail = pCube;   }
void         Mvc_CoverSetCubeList( Mvc_Cover_t * pCover, Mvc_List_t * pList ) { pCover->lCubes = *pList;        }

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverIsEmpty( Mvc_Cover_t * pCover )
{
    return Mvc_CoverReadCubeNum(pCover) == 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverIsTautology( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int iBit, Value;

    if ( Mvc_CoverReadCubeNum(pCover) != 1 )
        return 0;

    pCube = Mvc_CoverReadCubeHead( pCover );
    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
        if ( Value == 0 )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the cover is a binary buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverIsBinaryBuffer( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    if ( pCover->nBits != 2 )
        return 0;
    if ( Mvc_CoverReadCubeNum(pCover) != 1 )
        return 0;
    pCube = pCover->lCubes.pHead;
    if ( Mvc_CubeBitValue(pCube, 0) == 0 && Mvc_CubeBitValue(pCube, 1) == 1 )
        return 1;
    return 0;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverMakeEmpty( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube, * pCube2;
    Mvc_CoverForEachCubeSafe( pCover, pCube, pCube2 )
        Mvc_CubeFree( pCover, pCube );
    pCover->lCubes.nItems = 0;
    pCover->lCubes.pHead = NULL;
    pCover->lCubes.pTail = NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverMakeTautology( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCubeNew;
    Mvc_CoverMakeEmpty( pCover );
    pCubeNew = Mvc_CubeAlloc( pCover );
    Mvc_CubeBitFill( pCubeNew );
    Mvc_CoverAddCubeTail( pCover, pCubeNew );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverCreateEmpty( Mvc_Cover_t * pCover )
{
    Mvc_Cover_t * pCoverNew;
    pCoverNew = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );
    return pCoverNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverCreateTautology( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCubeNew;
    Mvc_Cover_t * pCoverNew;
    pCoverNew = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );
    pCubeNew = Mvc_CubeAlloc( pCoverNew );
    Mvc_CubeBitFill( pCubeNew );
    Mvc_CoverAddCubeTail( pCoverNew, pCubeNew );
    return pCoverNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

