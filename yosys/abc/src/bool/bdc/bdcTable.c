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

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Checks containment of the function in the ISF.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Bdc_TableCheckContainment( Bdc_Man_t * p, Bdc_Isf_t * pIsf, unsigned * puTruth )
{
    return Kit_TruthIsImply( pIsf->puOn, puTruth, p->nVars ) &&
         Kit_TruthIsDisjoint( puTruth, pIsf->puOff, p->nVars );
}

/**Function*************************************************************

  Synopsis    [Adds the new entry to the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Bdc_Fun_t * Bdc_TableLookup( Bdc_Man_t * p, Bdc_Isf_t * pIsf )
{
    int fDisableCache = 0;
    Bdc_Fun_t * pFunc;
    if ( fDisableCache && Kit_WordCountOnes(pIsf->uSupp) > 1 )
        return NULL;
    if ( pIsf->uSupp == 0 )
    {
        assert( p->pTable[pIsf->uSupp] == p->pNodes );
        if ( Kit_TruthIsConst1( pIsf->puOn, p->nVars ) )
            return p->pNodes;
        assert( Kit_TruthIsConst1( pIsf->puOff, p->nVars ) );
        return Bdc_Not(p->pNodes);
    }
    for ( pFunc = p->pTable[pIsf->uSupp]; pFunc; pFunc = pFunc->pNext )
        if ( Bdc_TableCheckContainment( p, pIsf, pFunc->puFunc ) )
             return pFunc;
    Bdc_IsfNot( pIsf );
    for ( pFunc = p->pTable[pIsf->uSupp]; pFunc; pFunc = pFunc->pNext )
        if ( Bdc_TableCheckContainment( p, pIsf, pFunc->puFunc ) )
        {
            Bdc_IsfNot( pIsf );
            return Bdc_Not(pFunc);
        }
    Bdc_IsfNot( pIsf );
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


ABC_NAMESPACE_IMPL_END

