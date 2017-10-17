/**CFile****************************************************************

  FileName    [aigTable.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Structural hashing table.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigTable.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// hashing the node
static unsigned long Aig_Hash( Aig_Obj_t * pObj, int TableSize ) 
{
    unsigned long Key = Aig_ObjIsExor(pObj) * 1699;
    Key ^= Aig_ObjFanin0(pObj)->Id * 7937;
    Key ^= Aig_ObjFanin1(pObj)->Id * 2971;
    Key ^= Aig_ObjFaninC0(pObj) * 911;
    Key ^= Aig_ObjFaninC1(pObj) * 353;
    return Key % TableSize;
}

// returns the place where this node is stored (or should be stored)
static Aig_Obj_t ** Aig_TableFind( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t ** ppEntry;
    assert( Aig_ObjChild0(pObj) && Aig_ObjChild1(pObj) );
    assert( Aig_ObjFanin0(pObj)->Id < Aig_ObjFanin1(pObj)->Id );
    for ( ppEntry = p->pTable + Aig_Hash(pObj, p->nTableSize); *ppEntry; ppEntry = &(*ppEntry)->pNext )
        if ( *ppEntry == pObj )
            return ppEntry;
    assert( *ppEntry == NULL );
    return ppEntry;
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description [Typically this procedure should not be called.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_TableResize( Aig_Man_t * p )
{
    Aig_Obj_t * pEntry, * pNext;
    Aig_Obj_t ** pTableOld, ** ppPlace;
    int nTableSizeOld, Counter, i;
    abctime clk;
    assert( p->pTable != NULL );
clk = Abc_Clock();
    // save the old table
    pTableOld = p->pTable;
    nTableSizeOld = p->nTableSize;
    // get the new table
    p->nTableSize = Abc_PrimeCudd( 2 * Aig_ManNodeNum(p) ); 
    p->pTable = ABC_ALLOC( Aig_Obj_t *, p->nTableSize );
    memset( p->pTable, 0, sizeof(Aig_Obj_t *) * p->nTableSize );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < nTableSizeOld; i++ )
    for ( pEntry = pTableOld[i], pNext = pEntry? pEntry->pNext : NULL; 
          pEntry; pEntry = pNext, pNext = pEntry? pEntry->pNext : NULL )
    {
        // get the place where this entry goes in the table 
        ppPlace = Aig_TableFind( p, pEntry );
        assert( *ppPlace == NULL ); // should not be there
        // add the entry to the list
        *ppPlace = pEntry;
        pEntry->pNext = NULL;
        Counter++;
    }
    assert( Counter == Aig_ManNodeNum(p) );
//    printf( "Increasing the structural table size from %6d to %6d. ", nTableSizeOld, p->nTableSize );
//    ABC_PRT( "Time", Abc_Clock() - clk );
    // replace the table and the parameters
    ABC_FREE( pTableOld );
}

/**Function*************************************************************

  Synopsis    [Checks if node with the given attributes is in the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_TableLookup( Aig_Man_t * p, Aig_Obj_t * pGhost )
{
    Aig_Obj_t * pEntry;
    assert( !Aig_IsComplement(pGhost) );
    assert( Aig_ObjIsNode(pGhost) );
    assert( Aig_ObjChild0(pGhost) && Aig_ObjChild1(pGhost) );
    assert( Aig_ObjFanin0(pGhost)->Id < Aig_ObjFanin1(pGhost)->Id );
    if ( p->pTable == NULL || !Aig_ObjRefs(Aig_ObjFanin0(pGhost)) || !Aig_ObjRefs(Aig_ObjFanin1(pGhost)) )
        return NULL;
    for ( pEntry = p->pTable[Aig_Hash(pGhost, p->nTableSize)]; pEntry; pEntry = pEntry->pNext )
    {
        if ( Aig_ObjChild0(pEntry) == Aig_ObjChild0(pGhost) && 
             Aig_ObjChild1(pEntry) == Aig_ObjChild1(pGhost) && 
             Aig_ObjType(pEntry) == Aig_ObjType(pGhost) )
            return pEntry;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Checks if node with the given attributes is in the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Obj_t * Aig_TableLookupTwo( Aig_Man_t * p, Aig_Obj_t * pFanin0, Aig_Obj_t * pFanin1 )
{
    Aig_Obj_t * pGhost;
    // consider simple cases
    if ( pFanin0 == pFanin1 )
        return pFanin0;
    if ( pFanin0 == Aig_Not(pFanin1) )
        return Aig_ManConst0(p);
    if ( Aig_Regular(pFanin0) == Aig_ManConst1(p) )
        return pFanin0 == Aig_ManConst1(p) ? pFanin1 : Aig_ManConst0(p);
    if ( Aig_Regular(pFanin1) == Aig_ManConst1(p) )
        return pFanin1 == Aig_ManConst1(p) ? pFanin0 : Aig_ManConst0(p);
    pGhost = Aig_ObjCreateGhost( p, pFanin0, pFanin1, AIG_OBJ_AND );
    return Aig_TableLookup( p, pGhost );
}

/**Function*************************************************************

  Synopsis    [Adds the new node to the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_TableInsert( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t ** ppPlace;
    assert( !Aig_IsComplement(pObj) );
    assert( Aig_TableLookup(p, pObj) == NULL );
    if ( (pObj->Id & 0xFF) == 0 && 2 * p->nTableSize < Aig_ManNodeNum(p) )
        Aig_TableResize( p );
    ppPlace = Aig_TableFind( p, pObj );
    assert( *ppPlace == NULL );
    *ppPlace = pObj;
}

/**Function*************************************************************

  Synopsis    [Deletes the node from the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_TableDelete( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    Aig_Obj_t ** ppPlace;
    assert( !Aig_IsComplement(pObj) );
    ppPlace = Aig_TableFind( p, pObj );
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
int Aig_TableCountEntries( Aig_Man_t * p )
{
    Aig_Obj_t * pEntry;
    int i, Counter = 0;
    for ( i = 0; i < p->nTableSize; i++ )
        for ( pEntry = p->pTable[i]; pEntry; pEntry = pEntry->pNext )
            Counter++;
    return Counter;
}

/**Function********************************************************************

  Synopsis    [Profiles the hash table.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Aig_TableProfile( Aig_Man_t * p )
{
    Aig_Obj_t * pEntry;
    int i, Counter;
    printf( "Table size = %d. Entries = %d.\n", p->nTableSize, Aig_ManNodeNum(p) );
    for ( i = 0; i < p->nTableSize; i++ )
    {
        Counter = 0;
        for ( pEntry = p->pTable[i]; pEntry; pEntry = pEntry->pNext )
            Counter++;
        if ( Counter ) 
            printf( "%d ", Counter );
    }
}

/**Function********************************************************************

  Synopsis    [Profiles the hash table.]

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Aig_TableClear( Aig_Man_t * p )
{
    ABC_FREE( p->pTable );
    p->nTableSize = 0;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

