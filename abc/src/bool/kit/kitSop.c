/**CFile****************************************************************

  FileName    [kitSop.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Computation kit.]

  Synopsis    [Procedures involving SOPs.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Dec 6, 2006.]

  Revision    [$Id: kitSop.c,v 1.00 2006/12/06 00:00:00 alanmi Exp $]

***********************************************************************/

#include "kit.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates SOP from the cube array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_SopCreate( Kit_Sop_t * cResult, Vec_Int_t * vInput, int nVars, Vec_Int_t * vMemory )
{
    unsigned uCube;
    int i;
    // start the cover
    cResult->nCubes = 0;
    cResult->pCubes = Vec_IntFetch( vMemory, Vec_IntSize(vInput) );
    // add the cubes
    Vec_IntForEachEntry( vInput, uCube, i )
        Kit_SopPushCube( cResult, uCube );
}

/**Function*************************************************************

  Synopsis    [Creates SOP from the cube array.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_SopCreateInverse( Kit_Sop_t * cResult, Vec_Int_t * vInput, int nLits, Vec_Int_t * vMemory )
{
    unsigned uCube, uMask = 0;
    int i, nCubes = Vec_IntSize(vInput);
    // start the cover
    cResult->nCubes = 0;
    cResult->pCubes = Vec_IntFetch( vMemory, nCubes );
    // add the cubes
//    Vec_IntForEachEntry( vInput, uCube, i )
    for ( i = 0; i < nCubes; i++ )
    {
        uCube = Vec_IntEntry( vInput, i );
        uMask = ((uCube | (uCube >> 1)) & 0x55555555);
        uMask |= (uMask << 1);
        Kit_SopPushCube( cResult, uCube ^ uMask );
    }
}

/**Function*************************************************************

  Synopsis    [Duplicates SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_SopDup( Kit_Sop_t * cResult, Kit_Sop_t * cSop, Vec_Int_t * vMemory )
{
    unsigned uCube;
    int i;
    // start the cover
    cResult->nCubes = 0;
    cResult->pCubes = Vec_IntFetch( vMemory, Kit_SopCubeNum(cSop) );
    // add the cubes
    Kit_SopForEachCube( cSop, uCube, i )
        Kit_SopPushCube( cResult, uCube );
}

/**Function*************************************************************

  Synopsis    [Derives the quotient of division by literal.]

  Description [Reduces the cover to be equal to the result of
  division of the given cover by the literal.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_SopDivideByLiteralQuo( Kit_Sop_t * cSop, int iLit )
{
    unsigned uCube;
    int i, k = 0;
    Kit_SopForEachCube( cSop, uCube, i )
    {
        if ( Kit_CubeHasLit(uCube, iLit) )
            Kit_SopWriteCube( cSop, Kit_CubeRemLit(uCube, iLit), k++ );
    }
    Kit_SopShrink( cSop, k );
}


/**Function*************************************************************

  Synopsis    [Divides cover by one cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_SopDivideByCube( Kit_Sop_t * cSop, Kit_Sop_t * cDiv, Kit_Sop_t * vQuo, Kit_Sop_t * vRem, Vec_Int_t * vMemory )
{
    unsigned uCube, uDiv;
    int i;
    // get the only cube
    assert( Kit_SopCubeNum(cDiv) == 1 );
    uDiv = Kit_SopCube(cDiv, 0);
    // allocate covers
    vQuo->nCubes = 0;
    vQuo->pCubes = Vec_IntFetch( vMemory, Kit_SopCubeNum(cSop) );
    vRem->nCubes = 0;
    vRem->pCubes = Vec_IntFetch( vMemory, Kit_SopCubeNum(cSop) );
    // sort the cubes
    Kit_SopForEachCube( cSop, uCube, i )
    {
        if ( Kit_CubeContains( uCube, uDiv ) )
            Kit_SopPushCube( vQuo, Kit_CubeSharp(uCube, uDiv) );
        else
            Kit_SopPushCube( vRem, uCube );
    }
}

/**Function*************************************************************

  Synopsis    [Divides cover by one cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_SopDivideInternal( Kit_Sop_t * cSop, Kit_Sop_t * cDiv, Kit_Sop_t * vQuo, Kit_Sop_t * vRem, Vec_Int_t * vMemory )
{
    unsigned uCube, uDiv;
    unsigned uCube2 = 0; // Suppress "might be used uninitialized"
    unsigned uDiv2, uQuo;
    int i, i2, k, k2, nCubesRem;
    assert( Kit_SopCubeNum(cSop) >= Kit_SopCubeNum(cDiv) );
    // consider special case
    if ( Kit_SopCubeNum(cDiv) == 1 )
    {
        Kit_SopDivideByCube( cSop, cDiv, vQuo, vRem, vMemory );
        return;
    }
    // allocate quotient
    vQuo->nCubes = 0;
    vQuo->pCubes = Vec_IntFetch( vMemory, Kit_SopCubeNum(cSop) / Kit_SopCubeNum(cDiv) );
    // for each cube of the cover
    // it either belongs to the quotient or to the remainder
    Kit_SopForEachCube( cSop, uCube, i )
    {
        // skip taken cubes
        if ( Kit_CubeIsMarked(uCube) )
            continue;
        // find a matching cube in the divisor
        uDiv = ~0;
        Kit_SopForEachCube( cDiv, uDiv, k )
            if ( Kit_CubeContains( uCube, uDiv ) )
                break;
        // the cube is not found 
        if ( k == Kit_SopCubeNum(cDiv) )
            continue;
        // the quotient cube exists
        uQuo = Kit_CubeSharp( uCube, uDiv );
        // find corresponding cubes for other cubes of the divisor
        uDiv2 = ~0;
        Kit_SopForEachCube( cDiv, uDiv2, k2 )
        {
            if ( k2 == k )
                continue;
            // find a matching cube
            Kit_SopForEachCube( cSop, uCube2, i2 )
            {
                // skip taken cubes
                if ( Kit_CubeIsMarked(uCube2) )
                    continue;
                // check if the cube can be used
                if ( Kit_CubeContains( uCube2, uDiv2 ) && uQuo == Kit_CubeSharp( uCube2, uDiv2 ) )
                    break;
            }
            // the case when the cube is not found
            if ( i2 == Kit_SopCubeNum(cSop) )
                break;
        }
        // we did not find some cubes - continue looking at other cubes
        if ( k2 != Kit_SopCubeNum(cDiv) )
            continue;
        // we found all cubes - add the quotient cube
        Kit_SopPushCube( vQuo, uQuo );

        // mark the first cube
        Kit_SopWriteCube( cSop, Kit_CubeMark(uCube), i );
        // mark other cubes that have this quotient
        Kit_SopForEachCube( cDiv, uDiv2, k2 )
        {
            if ( k2 == k )
                continue;
            // find a matching cube
            Kit_SopForEachCube( cSop, uCube2, i2 )
            {
                // skip taken cubes
                if ( Kit_CubeIsMarked(uCube2) )
                    continue;
                // check if the cube can be used
                if ( Kit_CubeContains( uCube2, uDiv2 ) && uQuo == Kit_CubeSharp( uCube2, uDiv2 ) )
                    break;
            }
            assert( i2 < Kit_SopCubeNum(cSop) );
            // the cube is found, mark it 
            // (later we will add all unmarked cubes to the remainder)
            Kit_SopWriteCube( cSop, Kit_CubeMark(uCube2), i2 );
        }
    }
    // determine the number of cubes in the remainder
    nCubesRem = Kit_SopCubeNum(cSop) - Kit_SopCubeNum(vQuo) * Kit_SopCubeNum(cDiv);
    // allocate remainder
    vRem->nCubes = 0;
    vRem->pCubes = Vec_IntFetch( vMemory, nCubesRem );
    // finally add the remaining unmarked cubes to the remainder 
    // and clean the marked cubes in the cover
    Kit_SopForEachCube( cSop, uCube, i )
    {
        if ( !Kit_CubeIsMarked(uCube) )
        {
            Kit_SopPushCube( vRem, uCube );
            continue;
        }
        Kit_SopWriteCube( cSop, Kit_CubeUnmark(uCube), i );
    }
    assert( nCubesRem == Kit_SopCubeNum(vRem) );
}

/**Function*************************************************************

  Synopsis    [Returns the common cube.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Kit_SopCommonCube( Kit_Sop_t * cSop )
{
    unsigned uMask, uCube;
    int i;
    uMask = ~(unsigned)0;
    Kit_SopForEachCube( cSop, uCube, i )
        uMask &= uCube;
    return uMask;
}

/**Function*************************************************************

  Synopsis    [Makes the cover cube-free.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_SopMakeCubeFree( Kit_Sop_t * cSop )
{
    unsigned uMask, uCube;
    int i;
    uMask = Kit_SopCommonCube( cSop );
    if ( uMask == 0 )
        return;
    // remove the common cube
    Kit_SopForEachCube( cSop, uCube, i )
        Kit_SopWriteCube( cSop, Kit_CubeSharp(uCube, uMask), i );
}

/**Function*************************************************************

  Synopsis    [Checks if the cover is cube-free.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_SopIsCubeFree( Kit_Sop_t * cSop )
{
    return Kit_SopCommonCube( cSop ) == 0;
}

/**Function*************************************************************

  Synopsis    [Creates SOP composes of the common cube of the given SOP.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_SopCommonCubeCover( Kit_Sop_t * cResult, Kit_Sop_t * cSop, Vec_Int_t * vMemory )
{
    assert( Kit_SopCubeNum(cSop) > 0 );
    cResult->nCubes = 0;
    cResult->pCubes = Vec_IntFetch( vMemory, 1 );
    Kit_SopPushCube( cResult, Kit_SopCommonCube(cSop) );
}


/**Function*************************************************************

  Synopsis    [Find any literal that occurs more than once.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_SopAnyLiteral( Kit_Sop_t * cSop, int nLits )
{
    unsigned uCube;
    int i, k, nLitsCur;
    // go through each literal
    for ( i = 0; i < nLits; i++ )
    {
        // go through all the cubes
        nLitsCur = 0;
        Kit_SopForEachCube( cSop, uCube, k )
            if ( Kit_CubeHasLit(uCube, i) )
                nLitsCur++;
        if ( nLitsCur > 1 )
            return i;
    }
    return -1;
}

/**Function*************************************************************

  Synopsis    [Find the least often occurring literal.]

  Description [Find the least often occurring literal among those
  that occur more than once.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_SopWorstLiteral( Kit_Sop_t * cSop, int nLits )
{
    unsigned uCube;
    int i, k, iMin, nLitsMin, nLitsCur;
    int fUseFirst = 1;

    // go through each literal
    iMin = -1;
    nLitsMin = 1000000;
    for ( i = 0; i < nLits; i++ )
    {
        // go through all the cubes
        nLitsCur = 0;
        Kit_SopForEachCube( cSop, uCube, k )
            if ( Kit_CubeHasLit(uCube, i) )
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

  Synopsis    [Find the least often occurring literal.]

  Description [Find the least often occurring literal among those
  that occur more than once.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_SopBestLiteral( Kit_Sop_t * cSop, int nLits, unsigned uMask )
{
    unsigned uCube;
    int i, k, iMax, nLitsMax, nLitsCur;
    int fUseFirst = 1;

    // go through each literal
    iMax = -1;
    nLitsMax = -1;
    for ( i = 0; i < nLits; i++ )
    {
        if ( !Kit_CubeHasLit(uMask, i) )
            continue;
        // go through all the cubes
        nLitsCur = 0;
        Kit_SopForEachCube( cSop, uCube, k )
            if ( Kit_CubeHasLit(uCube, i) )
                nLitsCur++;
        // skip the literal that does not occur or occurs once
        if ( nLitsCur < 2 )
            continue;
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
    if ( nLitsMax >= 0 )
        return iMax;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Computes a level-zero kernel.]

  Description [Modifies the cover to contain one level-zero kernel.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_SopDivisorZeroKernel_rec( Kit_Sop_t * cSop, int nLits )
{
    int iLit;
    // find any literal that occurs at least two times
    iLit = Kit_SopWorstLiteral( cSop, nLits );
    if ( iLit == -1 )
        return;
    // derive the cube-free quotient
    Kit_SopDivideByLiteralQuo( cSop, iLit ); // the same cover
    Kit_SopMakeCubeFree( cSop );             // the same cover
    // call recursively
    Kit_SopDivisorZeroKernel_rec( cSop, nLits );    // the same cover
}

/**Function*************************************************************

  Synopsis    [Computes the quick divisor of the cover.]

  Description [Returns 0, if there is no divisor other than trivial.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Kit_SopDivisor( Kit_Sop_t * cResult, Kit_Sop_t * cSop, int nLits, Vec_Int_t * vMemory )
{
    if ( Kit_SopCubeNum(cSop) <= 1 )
        return 0;
    if ( Kit_SopAnyLiteral( cSop, nLits ) == -1 )
        return 0;
    // duplicate the cover
    Kit_SopDup( cResult, cSop, vMemory );
    // perform the kerneling
    Kit_SopDivisorZeroKernel_rec( cResult, nLits );
    assert( Kit_SopCubeNum(cResult) > 0 );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Create the one-literal cover with the best literal from cSop.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Kit_SopBestLiteralCover( Kit_Sop_t * cResult, Kit_Sop_t * cSop, unsigned uCube, int nLits, Vec_Int_t * vMemory )
{
    int iLitBest;
    // get the best literal
    iLitBest = Kit_SopBestLiteral( cSop, nLits, uCube );
    // start the cover
    cResult->nCubes = 0;
    cResult->pCubes = Vec_IntFetch( vMemory, 1 );
    // set the cube
    Kit_SopPushCube( cResult, Kit_CubeSetLit(0, iLitBest) );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

