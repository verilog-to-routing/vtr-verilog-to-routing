/**CFile****************************************************************

  FileName    [timBox.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchy/timing manager.]

  Synopsis    [Manipulation of timing boxes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: timBox.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "timInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates the new timing box.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManCreateBox( Tim_Man_t * p, int firstIn, int nIns, int firstOut, int nOuts, int iDelayTable, int fBlack )
{
    Tim_Box_t * pBox;
    int i;
    if ( p->vBoxes == NULL )
        p->vBoxes = Vec_PtrAlloc( 100 );
    pBox = (Tim_Box_t *)Mem_FlexEntryFetch( p->pMemObj, sizeof(Tim_Box_t) + sizeof(int) * (nIns+nOuts) );
    memset( pBox, 0, sizeof(Tim_Box_t) );
    pBox->iBox = Vec_PtrSize( p->vBoxes );
    Vec_PtrPush( p->vBoxes, pBox );
    pBox->iDelayTable = iDelayTable;
    pBox->nInputs  = nIns;
    pBox->nOutputs = nOuts;
    pBox->fBlack = fBlack;
    for ( i = 0; i < nIns; i++ )
    {
        assert( firstIn+i < p->nCos );
        pBox->Inouts[i] = firstIn+i;
        p->pCos[firstIn+i].iObj2Box = pBox->iBox;
        p->pCos[firstIn+i].iObj2Num = i;
    }
    for ( i = 0; i < nOuts; i++ )
    {
        assert( firstOut+i < p->nCis );
        pBox->Inouts[nIns+i] = firstOut+i;
        p->pCis[firstOut+i].iObj2Box = pBox->iBox;
        p->pCis[firstOut+i].iObj2Num = i;
    }
//    if ( pBox->iBox < 20 )
//        printf( "%4d  %4d  %4d  %4d  \n", firstIn, nIns, firstOut, nOuts );
}

/**Function*************************************************************

  Synopsis    [Returns the box number for the given input.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManBoxForCi( Tim_Man_t * p, int iCi )
{
    if ( iCi >= p->nCis )
        return -1;
    return p->pCis[iCi].iObj2Box;
}

/**Function*************************************************************

  Synopsis    [Returns the box number for the given output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManBoxForCo( Tim_Man_t * p, int iCo )
{
    if ( iCo >= p->nCos )
        return -1;
    return p->pCos[iCo].iObj2Box;
}

/**Function*************************************************************

  Synopsis    [Returns the first input of the box.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManBoxInputFirst( Tim_Man_t * p, int iBox )
{
    return Tim_ManBox(p, iBox)->Inouts[0];
}

/**Function*************************************************************

  Synopsis    [Returns the last input of the box.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManBoxInputLast( Tim_Man_t * p, int iBox )
{
    return Tim_ManBox(p, iBox)->Inouts[0] + Tim_ManBoxInputNum(p, iBox) - 1;
}

/**Function*************************************************************

  Synopsis    [Returns the first output of the box.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManBoxOutputFirst( Tim_Man_t * p, int iBox )
{
    return Tim_ManBox(p, iBox)->Inouts[Tim_ManBox(p, iBox)->nInputs];
}

/**Function*************************************************************

  Synopsis    [Returns the last output of the box.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManBoxOutputLast( Tim_Man_t * p, int iBox )
{
    return Tim_ManBox(p, iBox)->Inouts[Tim_ManBox(p, iBox)->nInputs] + Tim_ManBoxOutputNum(p, iBox) - 1;
}

/**Function*************************************************************

  Synopsis    [Returns the number of box inputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManBoxInputNum( Tim_Man_t * p, int iBox )
{
    return Tim_ManBox(p, iBox)->nInputs;
}

/**Function*************************************************************

  Synopsis    [Returns the number of box outputs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManBoxOutputNum( Tim_Man_t * p, int iBox )
{
    return Tim_ManBox(p, iBox)->nOutputs;
}

/**Function*************************************************************

  Synopsis    [Return the delay table id.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManBoxDelayTableId( Tim_Man_t * p, int iBox )
{
    return Tim_ManBox(p, iBox)->iDelayTable;
}

/**Function*************************************************************

  Synopsis    [Return the delay table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float * Tim_ManBoxDelayTable( Tim_Man_t * p, int iBox )
{
    float * pTable;
    Tim_Box_t * pBox = Tim_ManBox(p, iBox);
    if ( pBox->iDelayTable < 0 )
        return NULL;
    pTable = (float *)Vec_PtrEntry( p->vDelayTables, pBox->iDelayTable );
    assert( (int)pTable[1] == pBox->nInputs );
    assert( (int)pTable[2] == pBox->nOutputs );
    return pTable;
}

/**Function*************************************************************

  Synopsis    [Return 1 if the box is black.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManBoxIsBlack( Tim_Man_t * p, int iBox )
{
    return Tim_ManBox(p, iBox)->fBlack;
}


/**Function*************************************************************

  Synopsis    [Returns the copy of the box.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManBoxCopy( Tim_Man_t * p, int iBox )
{
    return Tim_ManBox(p, iBox)->iCopy;
}

/**Function*************************************************************

  Synopsis    [Sets the copy of the box.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Tim_ManBoxSetCopy( Tim_Man_t * p, int iBox, int iCopy )
{
    Tim_ManBox(p, iBox)->iCopy = iCopy;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Tim_ManBoxFindFromCiNum( Tim_Man_t * p, int iCiNum )
{
    Tim_Box_t * pBox;
    int i;
    assert( iCiNum >= 0 && iCiNum < Tim_ManCiNum(p) );
    if ( iCiNum < Tim_ManPiNum(p) )
        return -1;
    Tim_ManForEachBox( p, pBox, i )
        if ( iCiNum < Tim_ManBoxOutputFirst(p, i) )
            return i - 1;
    return -2;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

