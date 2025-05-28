/**CFile****************************************************************

  FileName    [fxu.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [The entrance into the fast extract module.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: fxu.c,v 1.0 2003/02/01 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fxuInt.h" 
#include "fxu.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

/*===== fxuCreate.c ====================================================*/
extern Fxu_Matrix * Fxu_CreateMatrix( Fxu_Data_t * pData );
extern void         Fxu_CreateCovers( Fxu_Matrix * p, Fxu_Data_t * pData );

static int s_MemoryTotal;
static int s_MemoryPeak;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs fast_extract on a set of covers.]

  Description [All the covers are given in the array p->vSops. 
  The resulting covers are returned in the array p->vSopsNew.
  The entries in these arrays correspond to objects in the network.
  The entries corresponding to the PI and objects with trivial covers are NULL.
  The number of extracted covers (not exceeding p->nNodesExt) is returned. 
  Two other things are important for the correct operation of this procedure:
  (1) The input covers do not have duplicated fanins and are SCC-free. 
  (2) The fanins array contains the numbers of the fanin objects.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fxu_FastExtract( Fxu_Data_t * pData )
{
    int fScrollLines = 0;
    Fxu_Matrix * p;
    Fxu_Single * pSingle;
    Fxu_Double * pDouble;
    int Weight1, Weight2, Weight3;
    int Counter = 0;

    s_MemoryTotal = 0;
    s_MemoryPeak  = 0;

    // create the matrix
    p = Fxu_CreateMatrix( pData );
    if ( p == NULL )
        return -1;
//    if ( pData->fVerbose )
//        printf( "Memory usage after construction: Total = %d. Peak = %d.\n", s_MemoryTotal, s_MemoryPeak );
//Fxu_MatrixPrint( NULL, p );

    if ( pData->fOnlyS )
    {
        pData->nNodesNew = 0;
        do
        {
            Weight1 = Fxu_HeapSingleReadMaxWeight( p->pHeapSingle );
            if ( pData->fVerbose )
                printf( "Div %5d : Best single = %5d.%s", Counter++, Weight1, fScrollLines?"\n":"\r" );
            if ( Weight1 > pData->WeightMin || (Weight1 == 0 && pData->fUse0) )
                Fxu_UpdateSingle( p );
            else
                break;
        }
        while ( ++pData->nNodesNew < pData->nNodesExt );
    }
    else if ( pData->fOnlyD )
    {
        pData->nNodesNew = 0;
        do
        {
            Weight2 = Fxu_HeapDoubleReadMaxWeight( p->pHeapDouble );
            if ( pData->fVerbose )
                printf( "Div %5d : Best double = %5d.%s", Counter++, Weight2, fScrollLines?"\n":"\r" );
            if ( Weight2 > pData->WeightMin || (Weight2 == 0 && pData->fUse0) )
                Fxu_UpdateDouble( p );
            else
                break;
        }
        while ( ++pData->nNodesNew < pData->nNodesExt );
    }
    else if ( !pData->fUseCompl )
    {
        pData->nNodesNew = 0;
        do
        {
            Weight1 = Fxu_HeapSingleReadMaxWeight( p->pHeapSingle );
            Weight2 = Fxu_HeapDoubleReadMaxWeight( p->pHeapDouble );

            if ( pData->fVerbose )
                printf( "Div %5d : Best double = %5d. Best single = %5d.%s", Counter++, Weight2, Weight1, fScrollLines?"\n":"\r" );
//Fxu_Select( p, &pSingle, &pDouble );

            if ( Weight1 >= Weight2 )
            {
                if ( Weight1 > pData->WeightMin || (Weight1 == 0 && pData->fUse0) )
                    Fxu_UpdateSingle( p );
                else
                    break;
            }
            else
            {
                if ( Weight2 > pData->WeightMin || (Weight2 == 0 && pData->fUse0) )
                    Fxu_UpdateDouble( p );
                else
                    break;
            }
        }
        while ( ++pData->nNodesNew < pData->nNodesExt );
    }
    else
    { // use the complement
        pData->nNodesNew = 0;
        do
        {
            Weight1 = Fxu_HeapSingleReadMaxWeight( p->pHeapSingle );
            Weight2 = Fxu_HeapDoubleReadMaxWeight( p->pHeapDouble );

            // select the best single and double
            Weight3 = Fxu_Select( p, &pSingle, &pDouble );
            if ( pData->fVerbose )
                printf( "Div %5d : Best double = %5d. Best single = %5d. Best complement = %5d.%s", 
                    Counter++, Weight2, Weight1, Weight3, fScrollLines?"\n":"\r" );

            if ( Weight3 > pData->WeightMin || (Weight3 == 0 && pData->fUse0) )
                Fxu_Update( p, pSingle, pDouble );
            else
                break;
        }
        while ( ++pData->nNodesNew < pData->nNodesExt );
    }

    if ( pData->fVerbose )
        printf( "Total single = %3d. Total double = %3d. Total compl = %3d.                    \n", 
        p->nDivs1, p->nDivs2, p->nDivs3 );

    // create the new covers
    if ( pData->nNodesNew )
        Fxu_CreateCovers( p, pData );
    Fxu_MatrixDelete( p );
//    printf( "Memory usage after deallocation:   Total = %d. Peak = %d.\n", s_MemoryTotal, s_MemoryPeak );
    if ( pData->nNodesNew == pData->nNodesExt )
        printf( "Warning: The limit on the number of extracted divisors has been reached.\n" );
    return pData->nNodesNew;
}


/**Function*************************************************************

  Synopsis    [Unmarks the cubes in the ring.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixRingCubesUnmark( Fxu_Matrix * p )
{
    Fxu_Cube * pCube, * pCube2;
    // unmark the cubes
    Fxu_MatrixForEachCubeInRingSafe( p, pCube, pCube2 )
        pCube->pOrder = NULL;
    Fxu_MatrixRingCubesReset( p );
}


/**Function*************************************************************

  Synopsis    [Unmarks the vars in the ring.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MatrixRingVarsUnmark( Fxu_Matrix * p )
{
    Fxu_Var * pVar, * pVar2;
    // unmark the vars
    Fxu_MatrixForEachVarInRingSafe( p, pVar, pVar2 )
        pVar->pOrder = NULL;
    Fxu_MatrixRingVarsReset( p );
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Fxu_MemFetch( Fxu_Matrix * p, int nBytes )
{
    s_MemoryTotal += nBytes;
    if ( s_MemoryPeak < s_MemoryTotal )
        s_MemoryPeak = s_MemoryTotal;
//    return ABC_ALLOC( char, nBytes );
    return Extra_MmFixedEntryFetch( p->pMemMan );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fxu_MemRecycle( Fxu_Matrix * p, char * pItem, int nBytes )
{
    s_MemoryTotal -= nBytes;
//    ABC_FREE( pItem );
    Extra_MmFixedEntryRecycle( p->pMemMan, pItem );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

