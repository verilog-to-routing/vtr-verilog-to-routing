/**CFile****************************************************************

  FileName    [mvcCompare.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various cube comparison functions.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcCompare.c,v 1.5 2003/04/03 23:25:41 alanmi Exp $]

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

  Synopsis    [Compares two cubes according to their integer value.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CubeCompareInt( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask )
{
    if ( Mvc_Cube1Words(pC1) )
    {
        if ( pC1->pData[0] < pC2->pData[0] )
            return -1;
        if ( pC1->pData[0] > pC2->pData[0] )
            return 1;
        return 0;
    }
    else if ( Mvc_Cube2Words(pC1) )
    {
        if ( pC1->pData[1] < pC2->pData[1] )
            return -1;
        if ( pC1->pData[1] > pC2->pData[1] )
            return 1;
        if ( pC1->pData[0] < pC2->pData[0] )
            return -1;
        if ( pC1->pData[0] > pC2->pData[0] )
            return 1;
        return 0;
    }
    else                            
    {
        int i = Mvc_CubeReadLast(pC1);
        for(; i >= 0; i--)
        {
            if ( pC1->pData[i] < pC2->pData[i] )
                return -1;
            if ( pC1->pData[i] > pC2->pData[i] )
                return 1;
        }
        return 0;
    }
}


/**Function*************************************************************

  Synopsis    [Compares the cubes (1) by size, (2) by integer value.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CubeCompareSizeAndInt( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask )
{
    // compare the cubes by size
    if ( Mvc_CubeReadSize( pC1 ) < Mvc_CubeReadSize( pC2 ) )
        return 1;
    if ( Mvc_CubeReadSize( pC1 ) > Mvc_CubeReadSize( pC2 ) )
        return -1;
    // the cubes have the same size

    // compare the cubes as integers
    if ( Mvc_Cube1Words( pC1 ) )
    {
        if ( pC1->pData[0] < pC2->pData[0] )
            return -1;
        if ( pC1->pData[0] > pC2->pData[0] )
            return 1;
        return 0;
    }
    else if ( Mvc_Cube2Words( pC1 ) )
    {
        if ( pC1->pData[1] < pC2->pData[1] )
            return -1;
        if ( pC1->pData[1] > pC2->pData[1] )
            return 1;
        if ( pC1->pData[0] < pC2->pData[0] )
            return -1;
        if ( pC1->pData[0] > pC2->pData[0] )
            return 1;
        return 0;
    }
    else                            
    {
        int i = Mvc_CubeReadLast( pC1 );
        for(; i >= 0; i--)
        {
            if ( pC1->pData[i] < pC2->pData[i] )
                return -1;
            if ( pC1->pData[i] > pC2->pData[i] )
                return 1;
        }
        return 0;
    }
}

/**Function*************************************************************

  Synopsis    [Compares two cubes under the mask.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CubeCompareIntUnderMask( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask )
{
    unsigned uBits1, uBits2;

    // compare the cubes under the mask
    if ( Mvc_Cube1Words(pC1) )
    {
        uBits1 = pC1->pData[0] & pMask->pData[0];
        uBits2 = pC2->pData[0] & pMask->pData[0];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;
        // cubes are equal
        return 0;
    }
    else if ( Mvc_Cube2Words(pC1) )
    {
        uBits1 = pC1->pData[1] & pMask->pData[1];
        uBits2 = pC2->pData[1] & pMask->pData[1];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;
        uBits1 = pC1->pData[0] & pMask->pData[0];
        uBits2 = pC2->pData[0] & pMask->pData[0];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;
        return 0;
    }
    else                            
    {
        int i = Mvc_CubeReadLast(pC1);
        for(; i >= 0; i--)
        {
            uBits1 = pC1->pData[i] & pMask->pData[i];
            uBits2 = pC2->pData[i] & pMask->pData[i];
            if ( uBits1 < uBits2 )
                return -1;
            if ( uBits1 > uBits2 )
                return 1;
        }
        return 0;
    }
}

/**Function*************************************************************

  Synopsis    [Compares two cubes under the mask.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CubeCompareIntOutsideMask( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask )
{
    unsigned uBits1, uBits2;

    // compare the cubes under the mask
    if ( Mvc_Cube1Words(pC1) )
    {
        uBits1 = pC1->pData[0] | pMask->pData[0];
        uBits2 = pC2->pData[0] | pMask->pData[0];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;
        // cubes are equal
        return 0;
    }
    else if ( Mvc_Cube2Words(pC1) )
    {
        uBits1 = pC1->pData[1] | pMask->pData[1];
        uBits2 = pC2->pData[1] | pMask->pData[1];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;
        uBits1 = pC1->pData[0] | pMask->pData[0];
        uBits2 = pC2->pData[0] | pMask->pData[0];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;
        return 0;
    }
    else                            
    {
        int i = Mvc_CubeReadLast(pC1);
        for(; i >= 0; i--)
        {
            uBits1 = pC1->pData[i] | pMask->pData[i];
            uBits2 = pC2->pData[i] | pMask->pData[i];
            if ( uBits1 < uBits2 )
                return -1;
            if ( uBits1 > uBits2 )
                return 1;
        }
        return 0;
    }
}


/**Function*************************************************************

  Synopsis    [Compares the cubes (1) outside the mask, (2) under the mask.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CubeCompareIntOutsideAndUnderMask( Mvc_Cube_t * pC1, Mvc_Cube_t * pC2, Mvc_Cube_t * pMask )
{
    unsigned uBits1, uBits2;

    if ( Mvc_Cube1Words(pC1) )
    {
        // compare the cubes outside the mask
        uBits1 = pC1->pData[0] & ~(pMask->pData[0]);
        uBits2 = pC2->pData[0] & ~(pMask->pData[0]);
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;

        // compare the cubes under the mask
        uBits1 = pC1->pData[0] & pMask->pData[0];
        uBits2 = pC2->pData[0] & pMask->pData[0];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;
        // cubes are equal
        // should never happen
        assert( 0 );
        return 0;
    }
    else if ( Mvc_Cube2Words(pC1) )
    {
        // compare the cubes outside the mask
        uBits1 = pC1->pData[1] & ~(pMask->pData[1]);
        uBits2 = pC2->pData[1] & ~(pMask->pData[1]);
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;

        uBits1 = pC1->pData[0] & ~(pMask->pData[0]);
        uBits2 = pC2->pData[0] & ~(pMask->pData[0]);
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;

        // compare the cubes under the mask
        uBits1 = pC1->pData[1] & pMask->pData[1];
        uBits2 = pC2->pData[1] & pMask->pData[1];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;

        uBits1 = pC1->pData[0] & pMask->pData[0];
        uBits2 = pC2->pData[0] & pMask->pData[0];
        if ( uBits1 < uBits2 )
            return -1;
        if ( uBits1 > uBits2 )
            return 1;

        // cubes are equal
        // should never happen
        assert( 0 );
        return 0;
    }
    else                            
    {
        int i;

        // compare the cubes outside the mask
        for( i = Mvc_CubeReadLast(pC1); i >= 0; i-- )
        {
            uBits1 = pC1->pData[i] & ~(pMask->pData[i]);
            uBits2 = pC2->pData[i] & ~(pMask->pData[i]);
            if ( uBits1 < uBits2 )
                return -1;
            if ( uBits1 > uBits2 )
                return 1;
        }
        // compare the cubes under the mask
        for( i = Mvc_CubeReadLast(pC1); i >= 0; i-- )
        {
            uBits1 = pC1->pData[i] & pMask->pData[i];
            uBits2 = pC2->pData[i] & pMask->pData[i];
            if ( uBits1 < uBits2 )
                return -1;
            if ( uBits1 > uBits2 )
                return 1;
        }
/*
        {
            Mvc_Cover_t * pCover;
            pCover = Mvc_CoverAlloc( NULL, 96 );
            Mvc_CubePrint( pCover, pC1 );
            Mvc_CubePrint( pCover, pC2 );
            Mvc_CubePrint( pCover, pMask );
        }
*/
        // cubes are equal
        // should never happen
        assert( 0 );
        return 0;
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

