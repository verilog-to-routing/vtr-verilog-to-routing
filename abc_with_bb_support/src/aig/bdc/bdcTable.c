/**CFile****************************************************************

  FileName    [bdcTable.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Truth-table-based bi-decomposition engine.]

  Synopsis    [Hash table for intermediate nodes.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 30, 2007.]

  Revision    [$Id: bdcTable.c,v 1.00 2007/01/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bdcInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Minimizes the support of the ISF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bdc_SuppMinimize( Bdc_Man_t * p, Bdc_Isf_t * pIsf )
{
    int v;
    // go through the support variables
    for ( v = 0; v < p->nVars; v++ )
    {
        if ( (pIsf->uSupp & (1 << v)) == 0 )
            continue;
        Kit_TruthExistNew( p->puTemp1, pIsf->puOn, p->nVars, v );
        Kit_TruthExistNew( p->puTemp2, pIsf->puOff, p->nVars, v );
        if ( !Kit_TruthIsDisjoint( p->puTemp1, p->puTemp2, p->nVars ) )
            continue;
        // remove the variable
        Kit_TruthCopy( pIsf->puOn, p->puTemp1, p->nVars );
        Kit_TruthCopy( pIsf->puOff, p->puTemp2, p->nVars );
        pIsf->uSupp &= ~(1 << v);
    }
}

/**Function*************************************************************

  Synopsis    [Checks containment of the function in the ISF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bdc_TableCheckContainment( Bdc_Man_t * p, Bdc_Isf_t * pIsf, unsigned * puTruth )
{
    return Kit_TruthIsImply( pIsf->puOn, puTruth, p->nVars ) &&
         Kit_TruthIsDisjoint( pIsf->puOff, puTruth, p->nVars );
}

/**Function*************************************************************

  Synopsis    [Adds the new entry to the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bdc_Fun_t * Bdc_TableLookup( Bdc_Man_t * p, Bdc_Isf_t * pIsf )
{
    Bdc_Fun_t * pFunc;
    for ( pFunc = p->pTable[pIsf->uSupp]; pFunc; pFunc = pFunc->pNext )
        if ( Bdc_TableCheckContainment( p, pIsf, pFunc->puFunc ) )
             return pFunc;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Adds the new entry to the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bdc_TableAdd( Bdc_Man_t * p, Bdc_Fun_t * pFunc )
{
    if ( p->pTable[pFunc->uSupp] == NULL )
        Vec_IntPush( p->vSpots, pFunc->uSupp );
    pFunc->pNext = p->pTable[pFunc->uSupp];
    p->pTable[pFunc->uSupp] = pFunc;
}

/**Function*************************************************************

  Synopsis    [Adds the new entry to the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Bdc_TableClear( Bdc_Man_t * p )
{
    int Spot, i;
    Vec_IntForEachEntry( p->vSpots, Spot, i )
        p->pTable[Spot] = NULL;
    Vec_IntClear( p->vSpots );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


