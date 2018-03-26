/**CFile****************************************************************

  FileName    [hopTable.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Minimalistic And-Inverter Graph package.]

  Synopsis    [Structural hashing table.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006. ]

  Revision    [$Id: hopTable.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "hop.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// hashing the node
static unsigned long Hop_Hash( Hop_Obj_t * pObj, int TableSize ) 
{
    unsigned long Key = Hop_ObjIsExor(pObj) * 1699;
    Key ^= Hop_ObjFanin0(pObj)->Id * 7937;
    Key ^= Hop_ObjFanin1(pObj)->Id * 2971;
    Key ^= Hop_ObjFaninC0(pObj) * 911;
    Key ^= Hop_ObjFaninC1(pObj) * 353;
    return Key % TableSize;
}

// returns the place where this node is stored (or should be stored)
static Hop_Obj_t ** Hop_TableFind( Hop_Man_t * p, Hop_Obj_t * pObj )
{
    Hop_Obj_t ** ppEntry;
    assert( Hop_ObjChild0(pObj) && Hop_ObjChild1(pObj) );
    assert( Hop_ObjFanin0(pObj)->Id < Hop_ObjFanin1(pObj)->Id );
    for ( ppEntry = p->pTable + Hop_Hash(pObj, p->nTableSize); *ppEntry; ppEntry = &(*ppEntry)->pNext )
        if ( *ppEntry == pObj )
            return ppEntry;
    assert( *ppEntry == NULL );
    return ppEntry;
}

static void         Hop_TableResize( Hop_Man_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    [Checks if a node with the given attributes is in the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hop_Obj_t * Hop_TableLookup( Hop_Man_t * p, Hop_Obj_t * pGhost )
{
    Hop_Obj_t * pEntry;
    assert( !Hop_IsComplement(pGhost) );
    assert( Hop_ObjChild0(pGhost) && Hop_ObjChild1(pGhost) );
    assert( Hop_ObjFanin0(pGhost)->Id < Hop_ObjFanin1(pGhost)->Id );
    if ( p->fRefCount && (!Hop_ObjRefs(Hop_ObjFanin0(pGhost)) || !Hop_ObjRefs(Hop_ObjFanin1(pGhost))) )
        return NULL;
    for ( pEntry = p->pTable[Hop_Hash(pGhost, p->nTableSize)]; pEntry; pEntry = pEntry->pNext )
    {
        if ( Hop_ObjChild0(pEntry) == Hop_ObjChild0(pGhost) && 
             Hop_ObjChild1(pEntry) == Hop_ObjChild1(pGhost) && 
             Hop_ObjType(pEntry) == Hop_ObjType(pGhost) )
            return pEntry;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Adds the new node to the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_TableInsert( Hop_Man_t * p, Hop_Obj_t * pObj )
{
    Hop_Obj_t ** ppPlace;
    assert( !Hop_IsComplement(pObj) );
    assert( Hop_TableLookup(p, pObj) == NULL );
    if ( (pObj->Id & 0xFF) == 0 && 2 * p->nTableSize < Hop_ManNodeNum(p) )
        Hop_TableResize( p );
    ppPlace = Hop_TableFind( p, pObj );
    assert( *ppPlace == NULL );
    *ppPlace = pObj;
}

/**Function*************************************************************

  Synopsis    [Deletes the node from the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_TableDelete( Hop_Man_t * p, Hop_Obj_t * pObj )
{
    Hop_Obj_t ** ppPlace;
    assert( !Hop_IsComplement(pObj) );
    ppPlace = Hop_TableFind( p, pObj );
    assert( *ppPlace == pObj ); // node should be in the table
    // remove the node
    *ppPlace = pObj->pNext;
    pObj->pNext = NULL;
}

/**Function*************************************************************

  Synopsis    [Count the number of nodes in the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Hop_TableCountEntries( Hop_Man_t * p )
{
    Hop_Obj_t * pEntry;
    int i, Counter = 0;
    for ( i = 0; i < p->nTableSize; i++ )
        for ( pEntry = p->pTable[i]; pEntry; pEntry = pEntry->pNext )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description [Typically this procedure should not be called.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Hop_TableResize( Hop_Man_t * p )
{
    Hop_Obj_t * pEntry, * pNext;
    Hop_Obj_t ** pTableOld, ** ppPlace;
    int nTableSizeOld, Counter, nEntries, i;
    abctime clk;
clk = Abc_Clock();
    // save the old table
    pTableOld = p->pTable;
    nTableSizeOld = p->nTableSize;
    // get the new table
    p->nTableSize = Abc_PrimeCudd( 2 * Hop_ManNodeNum(p) ); 
    p->pTable = ABC_ALLOC( Hop_Obj_t *, p->nTableSize );
    memset( p->pTable, 0, sizeof(Hop_Obj_t *) * p->nTableSize );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < nTableSizeOld; i++ )
    for ( pEntry = pTableOld[i], pNext = pEntry? pEntry->pNext : NULL; pEntry; pEntry = pNext, pNext = pEntry? pEntry->pNext : NULL )
    {
        // get the place where this entry goes in the table 
        ppPlace = Hop_TableFind( p, pEntry );
        assert( *ppPlace == NULL ); // should not be there
        // add the entry to the list
        *ppPlace = pEntry;
        pEntry->pNext = NULL;
        Counter++;
    }
    nEntries = Hop_ManNodeNum(p);
    assert( Counter == nEntries );
//    printf( "Increasing the structural table size from %6d to %6d. ", nTableSizeOld, p->nTableSize );
//    ABC_PRT( "Time", Abc_Clock() - clk );
    // replace the table and the parameters
    ABC_FREE( pTableOld );
}

/**Function********************************************************************

  Synopsis    [Profiles the hash table.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Hop_TableProfile( Hop_Man_t * p )
{
    Hop_Obj_t * pEntry;
    int i, Counter;
    for ( i = 0; i < p->nTableSize; i++ )
    {
        Counter = 0;
        for ( pEntry = p->pTable[i]; pEntry; pEntry = pEntry->pNext )
            Counter++;
        if ( Counter ) 
            printf( "%d ", Counter );
    }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

