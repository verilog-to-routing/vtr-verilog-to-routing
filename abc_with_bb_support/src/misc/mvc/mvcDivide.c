/**CFile****************************************************************

  FileName    [mvcDivide.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Procedures for algebraic division.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcDivide.c,v 1.5 2003/04/26 20:41:36 alanmi Exp $]

***********************************************************************/

#include "mvc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Mvc_CoverVerifyDivision( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t * pQuo, Mvc_Cover_t * pRem );

int s_fVerbose = 0;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDivide( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t ** ppQuo, Mvc_Cover_t ** ppRem )
{
    // check the number of cubes
    if ( Mvc_CoverReadCubeNum( pCover ) < Mvc_CoverReadCubeNum( pDiv ) )
    {
        *ppQuo = NULL;
        *ppRem = NULL;
        return;
    }

    // make sure that support of pCover contains that of pDiv
    if ( !Mvc_CoverCheckSuppContainment( pCover, pDiv ) )
    {
        *ppQuo = NULL;
        *ppRem = NULL;
        return;
    }

    // perform the general division
    Mvc_CoverDivideInternal( pCover, pDiv, ppQuo, ppRem );
}


/**Function*************************************************************

  Synopsis    [Merge the cubes inside the groups.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDivideInternal( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t ** ppQuo, Mvc_Cover_t ** ppRem )
{
    Mvc_Cover_t * pQuo, * pRem;
    Mvc_Cube_t * pCubeC, * pCubeD, * pCubeCopy;
    Mvc_Cube_t * pCube1, * pCube2;
    int * pGroups, nGroups;    // the cube groups
    int nCubesC, nCubesD, nMerges, iCubeC, iCubeD;
    int iMerge = -1; // Suppress "might be used uninitialized"
    int fSkipG, GroupSize, g, c, RetValue;
    int nCubes;

    // get cover sizes
    nCubesD = Mvc_CoverReadCubeNum( pDiv );
    nCubesC = Mvc_CoverReadCubeNum( pCover );

    // check trivial cases
    if ( nCubesD == 1 )
    {
        if ( Mvc_CoverIsOneLiteral( pDiv ) )
            Mvc_CoverDivideByLiteral( pCover, pDiv, ppQuo, ppRem );
        else
            Mvc_CoverDivideByCube( pCover, pDiv, ppQuo, ppRem );
        return;
    }

    // create the divisor and the remainder 
    pQuo = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );
    pRem = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );

    // get the support of the divisor
    Mvc_CoverAllocateMask( pDiv );
    Mvc_CoverSupport( pDiv, pDiv->pMask );

    // sort the cubes of the divisor
    Mvc_CoverSort( pDiv, NULL, Mvc_CubeCompareInt );
    // sort the cubes of the cover
    Mvc_CoverSort( pCover, pDiv->pMask, Mvc_CubeCompareIntOutsideAndUnderMask );

    // allocate storage for cube groups
    pGroups = MEM_ALLOC( pCover->pMem, int, nCubesC + 1 );

    // mask contains variables in the support of Div
    // split the cubes into groups using the mask
    Mvc_CoverList2Array( pCover );
    Mvc_CoverList2Array( pDiv );
    pGroups[0] = 0;
    nGroups    = 1;
    for ( c = 1; c < nCubesC; c++ )
    {
        // get the cubes
        pCube1 = pCover->pCubes[c-1];
        pCube2 = pCover->pCubes[c  ];
        // compare the cubes
        Mvc_CubeBitEqualOutsideMask( RetValue, pCube1, pCube2, pDiv->pMask );
        if ( !RetValue )
            pGroups[nGroups++] = c;
    }
    // finish off the last group
    pGroups[nGroups] = nCubesC;

    // consider each group separately and decide
    // whether it can produce a quotient cube
    nCubes = 0;
    for ( g = 0; g < nGroups; g++ )
    {
        // if the group has less than nCubesD cubes, 
        // there is no way it can produce the quotient cube
        // copy the cubes to the remainder
        GroupSize = pGroups[g+1] - pGroups[g];
        if ( GroupSize < nCubesD )
        {
            for ( c = pGroups[g]; c < pGroups[g+1]; c++ )
            {
                pCubeCopy = Mvc_CubeDup( pRem, pCover->pCubes[c] );
                Mvc_CoverAddCubeTail( pRem, pCubeCopy );
                nCubes++;
            }
            continue;
        }

        // mark the cubes as those that should be added to the remainder
        for ( c = pGroups[g]; c < pGroups[g+1]; c++ )
            Mvc_CubeSetSize( pCover->pCubes[c], 1 );

        // go through the cubes in the group and at the same time
        // go through the cubes in the divisor
        iCubeD  = 0;
        iCubeC  = 0;
        pCubeD  = pDiv->pCubes[iCubeD++];
        pCubeC  = pCover->pCubes[pGroups[g]+iCubeC++];
        fSkipG  = 0;
        nMerges = 0;

        while ( 1 )
        {
            // compare the topmost cubes in F and in D
            RetValue = Mvc_CubeCompareIntUnderMask( pCubeC, pCubeD, pDiv->pMask );
            // cube are ordered in increasing order of their int value
            if ( RetValue == -1 ) // pCubeC is above pCubeD
            {  // cube in C should be added to the remainder
                // check that there is enough cubes in the group
                if ( GroupSize - iCubeC < nCubesD - nMerges )
                {
                    fSkipG = 1;
                    break;
                }
                // get the next cube in the cover
                pCubeC = pCover->pCubes[pGroups[g]+iCubeC++];
                continue;
            }
            if ( RetValue == 1 ) // pCubeD is above pCubeC
            { // given cube in D does not have a corresponding cube in the cover
                fSkipG = 1;
                break;
            }
            // mark the cube as the one that should NOT be added to the remainder
            Mvc_CubeSetSize( pCubeC, 0 );
            // remember this merged cube
            iMerge = iCubeC-1;
            nMerges++;

            // stop if we considered the last cube of the group
            if ( iCubeD == nCubesD )
                break;

            // advance the cube of the divisor
            assert( iCubeD < nCubesD );
            pCubeD = pDiv->pCubes[iCubeD++];

            // advance the cube of the group
            assert( pGroups[g]+iCubeC < nCubesC );
            pCubeC = pCover->pCubes[pGroups[g]+iCubeC++];
        }

        if ( fSkipG )
        { 
            // the group has failed, add all the cubes to the remainder
            for ( c = pGroups[g]; c < pGroups[g+1]; c++ )
            {
                pCubeCopy = Mvc_CubeDup( pRem, pCover->pCubes[c] );
                Mvc_CoverAddCubeTail( pRem, pCubeCopy );
                nCubes++;
            }
            continue;
        }

        // the group has worked, add left-over cubes to the remainder
        for ( c = pGroups[g]; c < pGroups[g+1]; c++ )
        {
            pCubeC = pCover->pCubes[c];
            if ( Mvc_CubeReadSize(pCubeC) )
            {
                pCubeCopy = Mvc_CubeDup( pRem, pCubeC );
                Mvc_CoverAddCubeTail( pRem, pCubeCopy );
                nCubes++;
            }
        }

        // create the quotient cube
        pCube1 = Mvc_CubeAlloc( pQuo );
        Mvc_CubeBitSharp( pCube1, pCover->pCubes[pGroups[g]+iMerge], pDiv->pMask );
        // add the cube to the quotient
        Mvc_CoverAddCubeTail( pQuo, pCube1 );
        nCubes += nCubesD;
    }
    assert( nCubes == nCubesC );

    // deallocate the memory
    MEM_FREE( pCover->pMem, int, nCubesC + 1, pGroups );

    // return the results
    *ppRem = pRem;
    *ppQuo = pQuo;
//    Mvc_CoverVerifyDivision( pCover, pDiv, pQuo, pRem );
}


/**Function*************************************************************

  Synopsis    [Divides the cover by a cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDivideByCube( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t ** ppQuo, Mvc_Cover_t ** ppRem )
{
    Mvc_Cover_t * pQuo, * pRem;
    Mvc_Cube_t * pCubeC, * pCubeD, * pCubeCopy;
    int CompResult;

    // get the only cube of D
    assert( Mvc_CoverReadCubeNum(pDiv) == 1 );

    // start the quotient and the remainder
    pQuo = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );
    pRem = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );

    // get the first and only cube of the divisor
    pCubeD = Mvc_CoverReadCubeHead( pDiv );

    // iterate through the cubes in the cover
    Mvc_CoverForEachCube( pCover, pCubeC )
    {
        // check the containment of literals from pCubeD in pCube
        Mvc_Cube2BitNotImpl( CompResult, pCubeD, pCubeC );
        if ( !CompResult )
        { // this cube belongs to the quotient
            // alloc the cube
            pCubeCopy = Mvc_CubeAlloc( pQuo );
            // clean the support of D
            Mvc_CubeBitSharp( pCubeCopy, pCubeC, pCubeD );
            // add the cube to the quotient
            Mvc_CoverAddCubeTail( pQuo, pCubeCopy );
        }
        else
        { 
            // copy the cube
            pCubeCopy = Mvc_CubeDup( pRem, pCubeC );
            // add the cube to the remainder
            Mvc_CoverAddCubeTail( pRem, pCubeCopy );
        }
    }
    // return the results
    *ppRem = pRem;
    *ppQuo = pQuo;
}

/**Function*************************************************************

  Synopsis    [Divides the cover by a literal.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDivideByLiteral( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t ** ppQuo, Mvc_Cover_t ** ppRem )
{
    Mvc_Cover_t * pQuo, * pRem;
    Mvc_Cube_t * pCubeC, * pCubeCopy;
    int iLit;

    // get the only cube of D
    assert( Mvc_CoverReadCubeNum(pDiv) == 1 );

    // start the quotient and the remainder
    pQuo = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );
    pRem = Mvc_CoverAlloc( pCover->pMem, pCover->nBits );

    // get the first and only literal in the divisor cube
    iLit = Mvc_CoverFirstCubeFirstLit( pDiv );

    // iterate through the cubes in the cover
    Mvc_CoverForEachCube( pCover, pCubeC )
    {
        // copy the cube
        pCubeCopy = Mvc_CubeDup( pCover, pCubeC );
        // add the cube to the quotient or to the remainder depending on the literal
        if ( Mvc_CubeBitValue( pCubeCopy, iLit ) )
        {   // remove the literal
            Mvc_CubeBitRemove( pCubeCopy, iLit );
            // add the cube ot the quotient
            Mvc_CoverAddCubeTail( pQuo, pCubeCopy );
        }
        else
        {   // add the cube ot the remainder
            Mvc_CoverAddCubeTail( pRem, pCubeCopy );
        }
    }
    // return the results
    *ppRem = pRem;
    *ppQuo = pQuo;
}


/**Function*************************************************************

  Synopsis    [Derives the quotient of division by literal.]

  Description [Reduces the cover to be the equal to the result of
  division of the given cover by the literal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverDivideByLiteralQuo( Mvc_Cover_t * pCover, int iLit )
{
    Mvc_Cube_t * pCube, * pCube2, * pPrev;
    // delete those cubes that do not have this literal
    // remove this literal from other cubes
    pPrev = NULL;
    Mvc_CoverForEachCubeSafe( pCover, pCube, pCube2 )
    {
        if ( Mvc_CubeBitValue( pCube, iLit ) == 0 )
        { // delete the cube from the cover
            Mvc_CoverDeleteCube( pCover, pPrev, pCube );
            Mvc_CubeFree( pCover, pCube );
            // don't update the previous cube
        }
        else
        { // delete this literal from the cube
            Mvc_CubeBitRemove( pCube, iLit );
            // update the previous cube
            pPrev = pCube;
        }
    }
}


/**Function*************************************************************

  Synopsis    [Verifies that the result of algebraic division is correct.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverVerifyDivision( Mvc_Cover_t * pCover, Mvc_Cover_t * pDiv, Mvc_Cover_t * pQuo, Mvc_Cover_t * pRem )
{   
    Mvc_Cover_t * pProd;
    Mvc_Cover_t * pDiff;

    pProd = Mvc_CoverAlgebraicMultiply( pDiv, pQuo );
    pDiff = Mvc_CoverAlgebraicSubtract( pCover, pProd );

    if ( Mvc_CoverAlgebraicEqual( pDiff, pRem ) )
        printf( "Verification OKAY!\n" );
    else
    {
        printf( "Verification FAILED!\n" );
        printf( "pCover:\n" );
        Mvc_CoverPrint( pCover );
        printf( "pDiv:\n" );
        Mvc_CoverPrint( pDiv );
        printf( "pRem:\n" );
        Mvc_CoverPrint( pRem );
        printf( "pQuo:\n" );
        Mvc_CoverPrint( pQuo );
    }

    Mvc_CoverFree( pProd );
    Mvc_CoverFree( pDiff );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

