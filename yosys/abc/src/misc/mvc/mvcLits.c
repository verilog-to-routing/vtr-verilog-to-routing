/**CFile****************************************************************

  FileName    [mvcLits.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Literal counting/updating procedures.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcLits.c,v 1.4 2003/04/03 06:31:50 alanmi Exp $]

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

  Synopsis    [Find the any literal that occurs more than once.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverAnyLiteral( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask )
{
    Mvc_Cube_t * pCube;
    int nWord, nBit, i;
    int nLitsCur;
    int fUseFirst = 0;

    // go through each literal
    if ( fUseFirst )
    {
        for ( i = 0; i < pCover->nBits; i++ )
            if ( !pMask || Mvc_CubeBitValue(pMask,i) )
            {
                // get the word and bit of this literal
                nWord = Mvc_CubeWhichWord(i);
                nBit  = Mvc_CubeWhichBit(i);
                // go through all the cubes
                nLitsCur = 0;
                Mvc_CoverForEachCube( pCover, pCube )
                    if ( pCube->pData[nWord] & (1<<nBit) )
                    {
                        nLitsCur++;
                        if ( nLitsCur > 1 )
                            return i;
                    }
            }
    }
    else
    {
        for ( i = pCover->nBits - 1; i >=0; i-- )
            if ( !pMask || Mvc_CubeBitValue(pMask,i) )
            {
                // get the word and bit of this literal
                nWord = Mvc_CubeWhichWord(i);
                nBit  = Mvc_CubeWhichBit(i);
                // go through all the cubes
                nLitsCur = 0;
                Mvc_CoverForEachCube( pCover, pCube )
                    if ( pCube->pData[nWord] & (1<<nBit) )
                    {
                        nLitsCur++;
                        if ( nLitsCur > 1 )
                            return i;
                    }
            }
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Find the most often occurring literal.]

  Description [Find the most often occurring literal among those
  that occur more than once.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverBestLiteral( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask )
{
    Mvc_Cube_t * pCube;
    int nWord, nBit;
    int i, iMax, nLitsMax, nLitsCur;
    int fUseFirst = 1;

    // go through each literal
    iMax = -1;
    nLitsMax = -1;
    for ( i = 0; i < pCover->nBits; i++ )
        if ( !pMask || Mvc_CubeBitValue(pMask,i) )
        {
            // get the word and bit of this literal
            nWord = Mvc_CubeWhichWord(i);
            nBit  = Mvc_CubeWhichBit(i);
            // go through all the cubes
            nLitsCur = 0;
            Mvc_CoverForEachCube( pCover, pCube )
                if ( pCube->pData[nWord] & (1<<nBit) )
                    nLitsCur++;

            // check if this is the best literal
            if ( fUseFirst )
            {
                if ( nLitsMax < nLitsCur )
                {
                    nLitsMax = nLitsCur;
                    iMax = i;
                }
            }
            else
            {
                if ( nLitsMax <= nLitsCur )
                {
                    nLitsMax = nLitsCur;
                    iMax = i;
                }
            }
        }

    if ( nLitsMax > 1 )
        return iMax;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Find the most often occurring literal.]

  Description [Find the most often occurring literal among those
  that occur more than once.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverWorstLiteral( Mvc_Cover_t * pCover, Mvc_Cube_t * pMask )
{
    Mvc_Cube_t * pCube;
    int nWord, nBit;
    int i, iMin, nLitsMin, nLitsCur;
    int fUseFirst = 1;

    // go through each literal
    iMin = -1;
    nLitsMin = 1000000;
    for ( i = 0; i < pCover->nBits; i++ )
        if ( !pMask || Mvc_CubeBitValue(pMask,i) )
        {
            // get the word and bit of this literal
            nWord = Mvc_CubeWhichWord(i);
            nBit  = Mvc_CubeWhichBit(i);
            // go through all the cubes
            nLitsCur = 0;
            Mvc_CoverForEachCube( pCover, pCube )
                if ( pCube->pData[nWord] & (1<<nBit) )
                    nLitsCur++;

            // skip the literal that does not occur or occurs once
            if ( nLitsCur < 2 )
                continue;

            // check if this is the best literal
            if ( fUseFirst )
            {
                if ( nLitsMin > nLitsCur )
                {
                    nLitsMin = nLitsCur;
                    iMin = i;
                }
            }
            else
            {
                if ( nLitsMin >= nLitsCur )
                {
                    nLitsMin = nLitsCur;
                    iMin = i;
                }
            }
        }

    if ( nLitsMin < 1000000 )
        return iMin;
    return -1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverBestLiteralCover( Mvc_Cover_t * pCover, Mvc_Cover_t * pSimple )
{
    Mvc_Cover_t * pCoverNew;
    Mvc_Cube_t * pCubeNew;
    Mvc_Cube_t * pCubeS;
    int iLitBest;

    // create the new cover
    pCoverNew = Mvc_CoverClone( pCover );
    // get the new cube
    pCubeNew = Mvc_CubeAlloc( pCoverNew );
    // clean the cube
    Mvc_CubeBitClean( pCubeNew );

    // get the first cube of pSimple
    assert( Mvc_CoverReadCubeNum(pSimple) == 1 );
    pCubeS = Mvc_CoverReadCubeHead( pSimple );
    // find the best literal among those of pCubeS
    iLitBest = Mvc_CoverBestLiteral( pCover, pCubeS );

    // insert this literal into the cube
    Mvc_CubeBitInsert( pCubeNew, iLitBest );
    // add the cube to the cover
    Mvc_CoverAddCubeTail( pCoverNew, pCubeNew );
    return pCoverNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverFirstCubeFirstLit( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int iBit, Value;

    // get the first cube
    pCube = Mvc_CoverReadCubeHead( pCover );
    // get the first literal
    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
        if ( Value )
            return iBit;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Returns the number of literals in the cover.]

  Description [Allocates storage for literal counters and fills it up
  using the current information.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverCountLiterals( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int nWord, nBit;
    int i, CounterTot, CounterCur;

    // allocate/clean the storage for literals
//    Mvc_CoverAllocateArrayLits( pCover );
//    memset( pCover->pLits, 0, pCover->nBits * sizeof(int) );
    // go through each literal
    CounterTot = 0;
    for ( i = 0; i < pCover->nBits; i++ )
    {
        // get the word and bit of this literal
        nWord = Mvc_CubeWhichWord(i);
        nBit  = Mvc_CubeWhichBit(i);
        // go through all the cubes
        CounterCur = 0;
        Mvc_CoverForEachCube( pCover, pCube )
            if ( pCube->pData[nWord] & (1<<nBit) )
                CounterCur++;
        CounterTot += CounterCur;
    }
    return CounterTot;
}

/**Function*************************************************************

  Synopsis    [Returns the number of literals in the cover.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverIsOneLiteral( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    int iBit, Counter, Value;
    if ( Mvc_CoverReadCubeNum(pCover) != 1 )
        return 0;
    pCube = Mvc_CoverReadCubeHead(pCover);
    // count literals
    Counter = 0;
    Mvc_CubeForEachBit( pCover, pCube, iBit, Value )
    {
        if ( Value )
        {
            if ( Counter++ )
                return 0;
        }
    }
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

