/**CFile****************************************************************

  FileName    [rwrLib.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting package.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rwrLib.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rwr.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Rwr_Node_t *        Rwr_ManTryNode( Rwr_Man_t * p, Rwr_Node_t * p0, Rwr_Node_t * p1, int fExor, int Level, int Volume );
static void                Rwr_MarkUsed_rec( Rwr_Man_t * p, Rwr_Node_t * pNode );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Precomputes the forest in the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_ManPrecompute( Rwr_Man_t * p )
{
    Rwr_Node_t * p0, * p1;
    int i, k, Level, Volume;
    int LevelOld = -1;
    int nNodes;

    Vec_PtrForEachEntryStart( Rwr_Node_t *, p->vForest, p0, i, 1 )
    Vec_PtrForEachEntryStart( Rwr_Node_t *, p->vForest, p1, k, 1 )
    {
        if ( LevelOld < (int)p0->Level )
        {
            LevelOld = p0->Level;
            printf( "Starting level %d  (at %d nodes).\n", LevelOld+1, i );
            printf( "Considered = %5d M.   Found = %8d.   Classes = %6d.   Trying %7d.\n", 
                p->nConsidered/1000000, p->vForest->nSize, p->nClasses, i );
        }

        if ( k == i )
            break;
//        if ( p0->Level + p1->Level > 6 ) // hard
//            break;

        if ( p0->Level + p1->Level > 5 ) // easy 
            break;

//        if ( p0->Level + p1->Level > 6 || (p0->Level == 3 && p1->Level == 3) )
//            break;

        // compute the level and volume of the new nodes
        Level  = 1 + Abc_MaxInt( p0->Level, p1->Level );
        Volume = 1 + Rwr_ManNodeVolume( p, p0, p1 );
        // try four different AND nodes
        Rwr_ManTryNode( p,         p0 ,         p1 , 0, Level, Volume );
        Rwr_ManTryNode( p, Rwr_Not(p0),         p1 , 0, Level, Volume );
        Rwr_ManTryNode( p,         p0 , Rwr_Not(p1), 0, Level, Volume );
        Rwr_ManTryNode( p, Rwr_Not(p0), Rwr_Not(p1), 0, Level, Volume );
        // try EXOR
        Rwr_ManTryNode( p,         p0 ,         p1 , 1, Level, Volume + 1 );
        // report the progress
        if ( p->nConsidered % 50000000 == 0 )
            printf( "Considered = %5d M.   Found = %8d.   Classes = %6d.   Trying %7d.\n", 
                p->nConsidered/1000000, p->vForest->nSize, p->nClasses, i );
        // quit after some time
        if ( p->vForest->nSize == RWR_LIMIT + 5 )
        {
            printf( "Considered = %5d M.   Found = %8d.   Classes = %6d.   Trying %7d.\n", 
                p->nConsidered/1000000, p->vForest->nSize, p->nClasses, i );
            goto save;
        }
    }
save :

    // mark the relevant ones
    Rwr_ManIncTravId( p );
    k = 5;
    nNodes = 0;
    Vec_PtrForEachEntryStart( Rwr_Node_t *, p->vForest, p0, i, 5 )
        if ( p0->uTruth == p->puCanons[p0->uTruth] )
        {
            Rwr_MarkUsed_rec( p, p0 );
            nNodes++;
        }

    // compact the array by throwing away non-canonical
    k = 5;
    Vec_PtrForEachEntryStart( Rwr_Node_t *, p->vForest, p0, i, 5 )
        if ( p0->fUsed )
        {
            p->vForest->pArray[k] = p0;
            p0->Id = k++;
        }
    p->vForest->nSize = k;
    printf( "Total canonical = %4d. Total used = %5d.\n", nNodes, p->vForest->nSize );
}

/**Function*************************************************************

  Synopsis    [Adds one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Rwr_Node_t * Rwr_ManTryNode( Rwr_Man_t * p, Rwr_Node_t * p0, Rwr_Node_t * p1, int fExor, int Level, int Volume )
{
    Rwr_Node_t * pOld, * pNew, ** ppPlace;
    unsigned uTruth;
    // compute truth table, level, volume
    p->nConsidered++;
    if ( fExor )
    {
//        printf( "Considering EXOR of %d and %d.\n", p0->Id, p1->Id );
        uTruth = (p0->uTruth ^ p1->uTruth);
    }
    else
        uTruth = (Rwr_IsComplement(p0)? ~Rwr_Regular(p0)->uTruth : Rwr_Regular(p0)->uTruth) & 
                 (Rwr_IsComplement(p1)? ~Rwr_Regular(p1)->uTruth : Rwr_Regular(p1)->uTruth) & 0xFFFF;
    // skip non-practical classes
    if ( Level > 2 && !p->pPractical[p->puCanons[uTruth]] )
        return NULL;
    // enumerate through the nodes with the same canonical form
    ppPlace = p->pTable + uTruth;
    for ( pOld = *ppPlace; pOld; ppPlace = &pOld->pNext, pOld = pOld->pNext )
    {
        if ( pOld->Level <  (unsigned)Level && pOld->Volume < (unsigned)Volume )
            return NULL;
        if ( pOld->Level == (unsigned)Level && pOld->Volume < (unsigned)Volume )
            return NULL;
//        if ( pOld->Level <  (unsigned)Level && pOld->Volume == (unsigned)Volume )
//            return NULL;
    }
/*
    // enumerate through the nodes with the opposite polarity
    for ( pOld = p->pTable[~uTruth & 0xFFFF]; pOld; pOld = pOld->pNext )
    {
        if ( pOld->Level <  (unsigned)Level && pOld->Volume < (unsigned)Volume )
            return NULL;
        if ( pOld->Level == (unsigned)Level && pOld->Volume < (unsigned)Volume )
            return NULL;
//        if ( pOld->Level <  (unsigned)Level && pOld->Volume == (unsigned)Volume )
//            return NULL;
    }
*/
    // count the classes
    if ( p->pTable[uTruth] == NULL && p->puCanons[uTruth] == uTruth )
        p->nClasses++;
    // create the new node
    pNew = (Rwr_Node_t *)Extra_MmFixedEntryFetch( p->pMmNode );
    pNew->Id     = p->vForest->nSize;
    pNew->TravId = 0;
    pNew->uTruth = uTruth;
    pNew->Level  = Level;
    pNew->Volume = Volume;
    pNew->fUsed  = 0;
    pNew->fExor  = fExor;
    pNew->p0     = p0;
    pNew->p1     = p1;
    pNew->pNext  = NULL;
    Vec_PtrPush( p->vForest, pNew );
    *ppPlace     = pNew;
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Adds one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Rwr_Node_t * Rwr_ManAddNode( Rwr_Man_t * p, Rwr_Node_t * p0, Rwr_Node_t * p1, int fExor, int Level, int Volume )
{
    Rwr_Node_t * pNew;
    unsigned uTruth;
    // compute truth table, leve, volume
    p->nConsidered++;
    if ( fExor )
        uTruth = (p0->uTruth ^ p1->uTruth);
    else
        uTruth = (Rwr_IsComplement(p0)? ~Rwr_Regular(p0)->uTruth : Rwr_Regular(p0)->uTruth) & 
                 (Rwr_IsComplement(p1)? ~Rwr_Regular(p1)->uTruth : Rwr_Regular(p1)->uTruth) & 0xFFFF;
    // create the new node
    pNew = (Rwr_Node_t *)Extra_MmFixedEntryFetch( p->pMmNode );
    pNew->Id     = p->vForest->nSize;
    pNew->TravId = 0;
    pNew->uTruth = uTruth;
    pNew->Level  = Level;
    pNew->Volume = Volume;
    pNew->fUsed  = 0;
    pNew->fExor  = fExor;
    pNew->p0     = p0;
    pNew->p1     = p1;
    pNew->pNext  = NULL;
    Vec_PtrPush( p->vForest, pNew );
    // do not add if the node is not essential
    if ( uTruth != p->puCanons[uTruth] )
        return pNew;

    // add to the list
    p->nAdded++;
    if ( p->pTable[uTruth] == NULL )
        p->nClasses++;
    Rwr_ListAddToTail( p->pTable + uTruth, pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Adds one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Rwr_Node_t * Rwr_ManAddVar( Rwr_Man_t * p, unsigned uTruth, int fPrecompute )
{
    Rwr_Node_t * pNew;
    pNew = (Rwr_Node_t *)Extra_MmFixedEntryFetch( p->pMmNode );
    pNew->Id     = p->vForest->nSize;
    pNew->TravId = 0;
    pNew->uTruth = uTruth;
    pNew->Level  = 0;
    pNew->Volume = 0;
    pNew->fUsed  = 1;
    pNew->fExor  = 0;
    pNew->p0     = NULL;
    pNew->p1     = NULL;    
    pNew->pNext  = NULL;
    Vec_PtrPush( p->vForest, pNew );
    if ( fPrecompute )
        Rwr_ListAddToTail( p->pTable + uTruth, pNew );
    return pNew;
}



/**Function*************************************************************

  Synopsis    [Adds one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_MarkUsed_rec( Rwr_Man_t * p, Rwr_Node_t * pNode )
{
    if ( pNode->fUsed || pNode->TravId == p->nTravIds )
        return;
    pNode->TravId = p->nTravIds;
    pNode->fUsed = 1;
    Rwr_MarkUsed_rec( p, Rwr_Regular(pNode->p0) );
    Rwr_MarkUsed_rec( p, Rwr_Regular(pNode->p1) );
}

/**Function*************************************************************

  Synopsis    [Adds one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_Trav_rec( Rwr_Man_t * p, Rwr_Node_t * pNode, int * pVolume )
{
    if ( pNode->fUsed || pNode->TravId == p->nTravIds )
        return;
    pNode->TravId = p->nTravIds;
    (*pVolume)++;
    if ( pNode->fExor )
        (*pVolume)++;
    Rwr_Trav_rec( p, Rwr_Regular(pNode->p0), pVolume );
    Rwr_Trav_rec( p, Rwr_Regular(pNode->p1), pVolume );
}

/**Function*************************************************************

  Synopsis    [Adds one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rwr_ManNodeVolume( Rwr_Man_t * p, Rwr_Node_t * p0, Rwr_Node_t * p1 )
{
    int Volume = 0;
    Rwr_ManIncTravId( p );
    Rwr_Trav_rec( p, p0, &Volume );
    Rwr_Trav_rec( p, p1, &Volume );
    return Volume;
}

/**Function*************************************************************

  Synopsis    [Adds one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_ManIncTravId( Rwr_Man_t * p )
{
    Rwr_Node_t * pNode;
    int i;
    if ( p->nTravIds++ < 0x8FFFFFFF )
        return;
    Vec_PtrForEachEntry( Rwr_Node_t *, p->vForest, pNode, i )
        pNode->TravId = 0;
    p->nTravIds = 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

