/**CFile****************************************************************

  FileName    [mvcUtils.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Various cover handling utilities.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: mvcUtils.c,v 1.7 2003/04/26 20:41:36 alanmi Exp $]

***********************************************************************/

#include "mvc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int bit_count[256] = {
  0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};


static void Mvc_CoverCopyColumn( Mvc_Cover_t * pCoverOld, Mvc_Cover_t * pCoverNew, int iColOld, int iColNew );


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverSupport( Mvc_Cover_t * pCover, Mvc_Cube_t * pSupp )
{
    Mvc_Cube_t * pCube;
    // clean the support
    Mvc_CubeBitClean( pSupp );
    // collect the support
    Mvc_CoverForEachCube( pCover, pCube )
        Mvc_CubeBitOr( pSupp, pSupp, pCube );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverSupportAnd( Mvc_Cover_t * pCover, Mvc_Cube_t * pSupp )
{
    Mvc_Cube_t * pCube;
    // clean the support
    Mvc_CubeBitFill( pSupp );
    // collect the support
    Mvc_CoverForEachCube( pCover, pCube )
        Mvc_CubeBitAnd( pSupp, pSupp, pCube );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverSupportSizeBinary( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pSupp;
    int Counter, i, v0, v1;
    // compute the support
    pSupp = Mvc_CubeAlloc( pCover );
    Mvc_CoverSupportAnd( pCover, pSupp );
    Counter = pCover->nBits/2;
    for ( i = 0; i < pCover->nBits/2; i++ )
    {
        v0 = Mvc_CubeBitValue( pSupp, 2*i   );
        v1 = Mvc_CubeBitValue( pSupp, 2*i+1 );
        if ( v0 && v1 )
            Counter--;
    }
    Mvc_CubeFree( pCover, pSupp );
    return Counter;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverSupportVarBelongs( Mvc_Cover_t * pCover, int iVar )
{
    Mvc_Cube_t * pSupp;
    int RetValue, v0, v1;
    // compute the support
    pSupp = Mvc_CubeAlloc( pCover );
    Mvc_CoverSupportAnd( pCover, pSupp );
    v0 = Mvc_CubeBitValue( pSupp, 2*iVar   );
    v1 = Mvc_CubeBitValue( pSupp, 2*iVar+1 );
    RetValue = (int)( !v0 || !v1 );
    Mvc_CubeFree( pCover, pSupp );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverCommonCube( Mvc_Cover_t * pCover, Mvc_Cube_t * pComCube )
{
    Mvc_Cube_t * pCube;
    // clean the support
    Mvc_CubeBitFill( pComCube );
    // collect the support
    Mvc_CoverForEachCube( pCover, pCube )
        Mvc_CubeBitAnd( pComCube, pComCube, pCube );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverIsCubeFree( Mvc_Cover_t * pCover )
{
    int Result;
    // get the common cube
    Mvc_CoverAllocateMask( pCover );
    Mvc_CoverCommonCube( pCover, pCover->pMask );
    // check whether the common cube is empty
    Mvc_CubeBitEmpty( Result, pCover->pMask );
    return Result;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverMakeCubeFree( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    // get the common cube
    Mvc_CoverAllocateMask( pCover );
    Mvc_CoverCommonCube( pCover, pCover->pMask );
    // remove this cube from the cubes in the cover
    Mvc_CoverForEachCube( pCover, pCube )
        Mvc_CubeBitSharp( pCube, pCube, pCover->pMask );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverCommonCubeCover( Mvc_Cover_t * pCover )
{
    Mvc_Cover_t * pRes;
    Mvc_Cube_t * pCube;
    // create the new cover
    pRes = Mvc_CoverClone( pCover );
    // get the new cube
    pCube = Mvc_CubeAlloc( pRes );
    // get the common cube
    Mvc_CoverCommonCube( pCover, pCube );
    // add the cube to the cover
    Mvc_CoverAddCubeTail( pRes, pCube );
    return pRes;
}


/**Function*************************************************************

  Synopsis    [Returns 1 if the support of cover2 is contained in the support of cover1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverCheckSuppContainment( Mvc_Cover_t * pCover1, Mvc_Cover_t * pCover2 )
{
    int Result;
    assert( pCover1->nBits == pCover2->nBits );
    // set the supports
    Mvc_CoverAllocateMask( pCover1 );
    Mvc_CoverSupport( pCover1, pCover1->pMask );
    Mvc_CoverAllocateMask( pCover2 );
    Mvc_CoverSupport( pCover2, pCover2->pMask );
    // check the containment
    Mvc_CubeBitNotImpl( Result, pCover2->pMask, pCover1->pMask );
    return !Result;
}

/**Function*************************************************************

  Synopsis    [Counts the cube sizes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverSetCubeSizes( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    unsigned char * pByte, * pByteStart, * pByteStop;
    int nBytes, nOnes;

    // get the number of unsigned chars in the cube's bit strings
    nBytes = pCover->nBits / (8 * sizeof(unsigned char)) + (int)(pCover->nBits % (8 * sizeof(unsigned char)) > 0);
    // iterate through the cubes
    Mvc_CoverForEachCube( pCover, pCube )
    {
        // clean the counter of ones
        nOnes = 0;
        // set the starting and stopping positions
        pByteStart = (unsigned char *)pCube->pData;
        pByteStop  = pByteStart + nBytes;
        // iterate through the positions
        for ( pByte = pByteStart; pByte < pByteStop; pByte++ )
            nOnes += bit_count[*pByte];
        // set the nOnes
        Mvc_CubeSetSize( pCube, nOnes );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Counts the cube sizes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverGetCubeSize( Mvc_Cube_t * pCube )
{
    unsigned char * pByte, * pByteStart, * pByteStop;
    int nOnes, nBytes, nBits;
    // get the number of unsigned chars in the cube's bit strings
    nBits = (pCube->iLast + 1) * sizeof(Mvc_CubeWord_t) * 8 - pCube->nUnused;
    nBytes = nBits / (8 * sizeof(unsigned char)) + (int)(nBits % (8 * sizeof(unsigned char)) > 0);
    // clean the counter of ones
    nOnes = 0;
    // set the starting and stopping positions
    pByteStart = (unsigned char *)pCube->pData;
    pByteStop  = pByteStart + nBytes;
    // iterate through the positions
    for ( pByte = pByteStart; pByte < pByteStop; pByte++ )
        nOnes += bit_count[*pByte];
    return nOnes;
}

/**Function*************************************************************

  Synopsis    [Counts the differences in each cube pair in the cover.]

  Description [Takes the cover (pCover) and the array where the
  diff counters go (pDiffs). The array pDiffs should have as many
  entries as there are different pairs of cubes in the cover: n(n-1)/2.
  Fills out the array pDiffs with the following info: For each cube
  pair, included in the array is the number of literals in both cubes
  after they are made cube ABC_FREE.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_CoverCountCubePairDiffs( Mvc_Cover_t * pCover, unsigned char pDiffs[] )
{
    Mvc_Cube_t * pCube1;
    Mvc_Cube_t * pCube2;
    Mvc_Cube_t * pMask;
    unsigned char * pByte, * pByteStart, * pByteStop;
    int nBytes, nOnes;
    int nCubePairs;

    // allocate a temporary mask
    pMask = Mvc_CubeAlloc( pCover );
    // get the number of unsigned chars in the cube's bit strings
    nBytes = pCover->nBits / (8 * sizeof(unsigned char)) + (int)(pCover->nBits % (8 * sizeof(unsigned char)) > 0);
    // iterate through the cubes
    nCubePairs = 0;
    Mvc_CoverForEachCube( pCover, pCube1 )
    {
        Mvc_CoverForEachCubeStart( Mvc_CubeReadNext(pCube1), pCube2 )
        {
            // find the bit-wise exor of cubes
            Mvc_CubeBitExor( pMask, pCube1, pCube2 );
            // set the starting and stopping positions
            pByteStart = (unsigned char *)pMask->pData;
            pByteStop  = pByteStart + nBytes;
            // clean the counter of ones
            nOnes = 0;
            // iterate through the positions
            for ( pByte = pByteStart; pByte < pByteStop; pByte++ )
                nOnes += bit_count[*pByte];
            // set the nOnes
            pDiffs[nCubePairs++] = nOnes;
        }
    }
    // deallocate the mask
    Mvc_CubeFree( pCover, pMask );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Creates a new cover containing some literals of the old cover.]

  Description [Creates the new cover containing the given number (nVarsRem)
  literals of the old cover. All the bits of the new cover are initialized
  to "1". The selected bits from the old cover are copied on top. The numbers
  of the selected bits to copy are given in the array pVarsRem. The i-set
  entry in this array is the index of the bit in the old cover which goes
  to the i-th place in the new cover. If the i-th entry in pVarsRem is -1, 
  it means that the i-th bit does not change (remains composed of all 1's).
  This is a useful feature to speed up remapping covers, which are known
  to depend only on a subset of input variables.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverRemap( Mvc_Cover_t * p, int * pVarsRem, int nVarsRem )
{
    Mvc_Cover_t * pCover;
    Mvc_Cube_t * pCube, * pCubeCopy;
    int i;
    // clone the cover
    pCover = Mvc_CoverAlloc( p->pMem, nVarsRem );
    // copy the cube list
    Mvc_CoverForEachCube( p, pCube )
    {
        pCubeCopy = Mvc_CubeAlloc( pCover );
        //Mvc_CubeBitClean( pCubeCopy );   //changed by wjiang
        Mvc_CubeBitFill( pCubeCopy );      //changed by wjiang
        Mvc_CoverAddCubeTail( pCover, pCubeCopy );
    }
    // copy the corresponding columns
    for ( i = 0; i < nVarsRem; i++ )
    {
        if (pVarsRem[i] < 0) 
            continue;     //added by wjiang
        assert( pVarsRem[i] >= 0 && pVarsRem[i] < p->nBits );
        Mvc_CoverCopyColumn( p, pCover, pVarsRem[i], i );
    }
    return pCover;
}

/**Function*************************************************************

  Synopsis    [Copies a column from the old cover to the new cover.]

  Description [Copies the column (iColOld) of the old cover (pCoverOld)
  into the column (iColNew) of the new cover (pCoverNew). Assumes that 
  the number of cubes is the same in both covers. Makes no assuptions 
  about the current contents of the column in the new cover.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverCopyColumn( Mvc_Cover_t * pCoverOld, Mvc_Cover_t * pCoverNew, 
                         int iColOld, int iColNew )
{
    Mvc_Cube_t * pCubeOld, * pCubeNew;
    int iWordOld, iWordNew, iBitOld, iBitNew;

    assert( Mvc_CoverReadCubeNum(pCoverOld) == Mvc_CoverReadCubeNum(pCoverNew) );

    // get the place of the old and new columns
    iWordOld = Mvc_CubeWhichWord(iColOld);
    iBitOld  = Mvc_CubeWhichBit(iColOld);
    iWordNew = Mvc_CubeWhichWord(iColNew);
    iBitNew  = Mvc_CubeWhichBit(iColNew);

    // go through the cubes of both covers
    pCubeNew = Mvc_CoverReadCubeHead(pCoverNew);
    Mvc_CoverForEachCube( pCoverOld, pCubeOld )
    {
        if ( pCubeOld->pData[iWordOld] & (1<<iBitOld) )
            pCubeNew->pData[iWordNew] |= (1<<iBitNew);
        else
            pCubeNew->pData[iWordNew] &= ~(1<<iBitNew);  // added by wjiang
        pCubeNew = Mvc_CubeReadNext( pCubeNew );
    }
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mvc_CoverInverse( Mvc_Cover_t * pCover )
{
    Mvc_Cube_t * pCube;
    // complement the cubes
    Mvc_CoverForEachCube( pCover, pCube )
        Mvc_CubeBitNot( pCube );
}

/**Function*************************************************************

  Synopsis    [This function dups the cover and removes DC literals from cubes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverRemoveDontCareLits( Mvc_Cover_t * pCover )
{
    Mvc_Cover_t * pCoverNew;
    Mvc_Cube_t * pCube;

    pCoverNew = Mvc_CoverDup( pCover );
    Mvc_CoverForEachCube( pCoverNew, pCube )
        Mvc_CubeBitRemoveDcs( pCube );
    return pCoverNew;
}

/**Function*************************************************************

  Synopsis    [Returns the cofactor w.r.t. to a binary var.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverCofactor( Mvc_Cover_t * p, int iValue, int iValueOther )
{
    Mvc_Cover_t * pCover;
    Mvc_Cube_t * pCube, * pCubeCopy;
    // clone the cover
    pCover = Mvc_CoverClone( p );
    // copy the cube list
    Mvc_CoverForEachCube( p, pCube )
        if ( Mvc_CubeBitValue( pCube, iValue ) )
        {
            pCubeCopy = Mvc_CubeDup( pCover, pCube );
            Mvc_CoverAddCubeTail( pCover, pCubeCopy );
            Mvc_CubeBitInsert( pCubeCopy, iValueOther );
        }
    return pCover;
}

/**Function*************************************************************

  Synopsis    [Returns the cover, in which the binary var is complemented.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverFlipVar( Mvc_Cover_t * p, int iValue0, int iValue1 )
{
    Mvc_Cover_t * pCover;
    Mvc_Cube_t * pCube, * pCubeCopy;
    int Value0, Value1, Temp;

    assert( iValue0 + 1 == iValue1 ); // should be adjacent

    // clone the cover
    pCover = Mvc_CoverClone( p );
    // copy the cube list
    Mvc_CoverForEachCube( p, pCube )
    {
        pCubeCopy = Mvc_CubeDup( pCover, pCube );
        Mvc_CoverAddCubeTail( pCover, pCubeCopy );

        // get the bits
        Value0 = Mvc_CubeBitValue( pCubeCopy, iValue0 );
        Value1 = Mvc_CubeBitValue( pCubeCopy, iValue1 );

        // if both bits are one, nothing to swap
        if ( Value0 && Value1 )
            continue;
        // cannot be both zero because they belong to the same var
        assert( Value0 || Value1 ); 

        // swap the bits
        Temp   = Value0;
        Value0 = Value1;
        Value1 = Temp;

        // set the bits after the swap
        if ( Value0 )
            Mvc_CubeBitInsert( pCubeCopy, iValue0 );
        else
            Mvc_CubeBitRemove( pCubeCopy, iValue0 );

        if ( Value1 )
            Mvc_CubeBitInsert( pCubeCopy, iValue1 );
        else
            Mvc_CubeBitRemove( pCubeCopy, iValue1 );
    }
    return pCover;
}

/**Function*************************************************************

  Synopsis    [Returns the cover derived by universal quantification.]

  Description [Returns the cover computed by universal quantification
  as follows: CoverNew = Univ(B) [Cover & (A==B)]. Removes the second 
  binary var from the support (given by values iValueB0 and iValueB1). 
  Leaves the first binary variable (given by values iValueA0 and iValueA1) 
  in the support.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverUnivQuantify( Mvc_Cover_t * p, 
    int iValueA0, int iValueA1, int iValueB0, int iValueB1 )
{
    Mvc_Cover_t * pCover;
    Mvc_Cube_t * pCube, * pCubeCopy;
    int ValueA0, ValueA1, ValueB0, ValueB1;

    // clone the cover
    pCover = Mvc_CoverClone( p );
    // copy the cube list
    Mvc_CoverForEachCube( p, pCube )
    {
        // get the bits
        ValueA0 = Mvc_CubeBitValue( pCube, iValueA0 );
        ValueA1 = Mvc_CubeBitValue( pCube, iValueA1 );
        ValueB0 = Mvc_CubeBitValue( pCube, iValueB0 );
        ValueB1 = Mvc_CubeBitValue( pCube, iValueB1 );

        // cannot be both zero because they belong to the same var
        assert( ValueA0 || ValueA1 ); 
        assert( ValueB0 || ValueB1 ); 

        // if the values of this var are different, do not add the cube
        if ( ValueA0 != ValueB0 && ValueA1 != ValueB1 )
            continue;

        // create the cube
        pCubeCopy = Mvc_CubeDup( pCover, pCube );
        Mvc_CoverAddCubeTail( pCover, pCubeCopy );

        // insert 1's into for the first var, if both have this value
        if ( ValueA0 && ValueB0 )
            Mvc_CubeBitInsert( pCubeCopy, iValueA0 );
        else
            Mvc_CubeBitRemove( pCubeCopy, iValueA0 );

        if ( ValueA1 && ValueB1 )
            Mvc_CubeBitInsert( pCubeCopy, iValueA1 );
        else
            Mvc_CubeBitRemove( pCubeCopy, iValueA1 );
            
        // insert 1's into for the second var (the cover does not depend on it)
        Mvc_CubeBitInsert( pCubeCopy, iValueB0 );
        Mvc_CubeBitInsert( pCubeCopy, iValueB1 );
    }
    return pCover;
}

#if 0

/**Function*************************************************************

  Synopsis    [Derives the cofactors of the cover.]

  Description [Derives the cofactors w.r.t. a variable and also cubes
  that do not depend on this variable.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t ** Mvc_CoverCofactors( Mvc_Data_t * pData, Mvc_Cover_t * pCover, int iVar )
{
    Mvc_Cover_t ** ppCofs;
    Mvc_Cube_t * pCube, * pCubeNew;
    int i, nValues, iValueFirst, Res;

    // start the covers for cofactors
    iValueFirst = Vm_VarMapReadValuesFirst(pData->pVm, iVar);
    nValues = Vm_VarMapReadValues(pData->pVm, iVar);
    ppCofs = ABC_ALLOC( Mvc_Cover_t *, nValues + 1 );
    for ( i = 0; i <= nValues; i++ )
        ppCofs[i] = Mvc_CoverClone( pCover );

    // go through the cubes
    Mvc_CoverForEachCube( pCover, pCube )
    {
        // if the literal if a full literal, add it to last "cofactor"
        Mvc_CubeBitEqualUnderMask( Res, pCube, pData->ppMasks[iVar], pData->ppMasks[iVar] );
        if ( Res )
        {
            pCubeNew = Mvc_CubeDup(pCover, pCube);
            Mvc_CoverAddCubeTail( ppCofs[nValues], pCubeNew );
            continue;
        }

        // otherwise, add it to separate values
        for ( i = 0; i < nValues; i++ )
            if ( Mvc_CubeBitValue( pCube, iValueFirst + i ) )
            {
                pCubeNew = Mvc_CubeDup(pCover, pCube);
                Mvc_CubeBitOr( pCubeNew, pCubeNew, pData->ppMasks[iVar] );
                Mvc_CoverAddCubeTail( ppCofs[i], pCubeNew );
            }
    }
    return ppCofs;
}

/**Function*************************************************************

  Synopsis    [Count the cubes with non-trivial literals with the given value.]

  Description [The data and the cover are given (pData, pCover). Also given 
  are the variable number and the number of a value of this variable.
  This procedure returns the number of cubes having a non-trivial literal
  of this variable that have the given value present.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvr_CoverCountLitsWithValue( Mvc_Data_t * pData, Mvc_Cover_t * pCover, int iVar, int iValue )
{
    Mvc_Cube_t * pCube;
    int iValueFirst, Res, Counter;

    Counter = 0;
    iValueFirst = Vm_VarMapReadValuesFirst( pData->pVm, iVar );
    Mvc_CoverForEachCube( pCover, pCube )
    {
        // check if the given literal is the full literal
        Mvc_CubeBitEqualUnderMask( Res, pCube, pData->ppMasks[iVar], pData->ppMasks[iVar] );
        if ( Res )
            continue;
        // this literal is not a full literal; check if it has this value
        Counter += Mvc_CubeBitValue( pCube, iValueFirst + iValue );
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Creates the expanded cover.]

  Description [The original cover is expanded by adding some variables.
  These variables are the additional variables in pVmNew, compared to
  pCvr->pVm. The resulting cover is the same as the original one, except
  that it contains the additional variables present as full literals
  in every cube.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverCreateExpanded( Mvc_Cover_t * pCover, Vm_VarMap_t * pVmNew )
{
    Mvc_Cover_t * pCoverNew;
    Mvc_Cube_t * pCube, * pCubeNew;
    int i, iLast, iLastNew;

    // create the new cover
    pCoverNew = Mvc_CoverAlloc( pCover->pMem, Vm_VarMapReadValuesInNum(pVmNew) );

    // get the cube composed of extra bits
    Mvc_CoverAllocateMask( pCoverNew );
    Mvc_CubeBitClean( pCoverNew->pMask );
    for ( i = pCover->nBits; i < pCoverNew->nBits; i++ )
        Mvc_CubeBitInsert( pCoverNew->pMask, i );

    // get the indexes of the last words in both covers
    iLast    = pCover->nWords?    pCover->nWords - 1: 0;
    iLastNew = pCoverNew->nWords? pCoverNew->nWords - 1: 0;

    // create the cubes of the new cover
    Mvc_CoverForEachCube( pCover, pCube )
    {
        pCubeNew = Mvc_CubeAlloc( pCoverNew );
        Mvc_CubeBitClean( pCubeNew );
        // copy the bits (cannot immediately use Mvc_CubeBitCopy, 
        // because covers have different numbers of bits)
        Mvc_CubeSetLast( pCubeNew, iLast );
        Mvc_CubeBitCopy( pCubeNew, pCube );
        Mvc_CubeSetLast( pCubeNew, iLastNew );
        // add the extra bits
        Mvc_CubeBitOr( pCubeNew, pCubeNew, pCoverNew->pMask );
        // add the cube to the new cover
        Mvc_CoverAddCubeTail( pCoverNew, pCubeNew );
    }
    return pCoverNew;
}

#endif

/**Function*************************************************************

  Synopsis    [Transposes the cube cover.]

  Description [Returns the cube cover that looks like a transposed
  matrix, compared to the matrix derived from the original cover.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mvc_Cover_t * Mvc_CoverTranspose( Mvc_Cover_t * pCover )
{
    Mvc_Cover_t * pRes;
    Mvc_Cube_t * pCubeRes, * pCube;
    int nWord, nBit, i, iCube;

    pRes = Mvc_CoverAlloc( pCover->pMem, Mvc_CoverReadCubeNum(pCover) );
    for ( i = 0; i < pCover->nBits; i++ )
    {
        // get the word and bit of this literal
        nWord = Mvc_CubeWhichWord(i);
        nBit  = Mvc_CubeWhichBit(i);
        // get the transposed cube
        pCubeRes = Mvc_CubeAlloc( pRes );
        Mvc_CubeBitClean( pCubeRes );
        iCube = 0;
        Mvc_CoverForEachCube( pCover, pCube )
        {
            if ( pCube->pData[nWord] & (1<<nBit) )
                Mvc_CubeBitInsert( pCubeRes, iCube );
            iCube++;
        }
        Mvc_CoverAddCubeTail( pRes, pCubeRes ); 
    }
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Checks that the cubes of the cover have 0's in unused bits.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mvc_UtilsCheckUnusedZeros( Mvc_Cover_t * pCover )
{
    unsigned Unsigned;
    Mvc_Cube_t * pCube;
    int nCubes;

    nCubes = 0;
    Mvc_CoverForEachCube( pCover, pCube )
    {
        if ( pCube->nUnused == 0 )
            continue;

        Unsigned = ( pCube->pData[pCube->iLast] & 
                    (BITS_FULL << (32-pCube->nUnused)) );
        if( Unsigned )
        {
            printf( "Cube %2d out of %2d contains dirty bits.\n", nCubes, 
                Mvc_CoverReadCubeNum(pCover) );
        }
        nCubes++;
    }
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

