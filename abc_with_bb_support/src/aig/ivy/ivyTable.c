/**CFile****************************************************************

  FileName    [ivyTable.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [And-Inverter Graph package.]

  Synopsis    [Structural hashing table.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 11, 2006. ]

  Revision    [$Id: ivyTable.c,v 1.00 2006/05/11 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ivy.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// hashing the node
static unsigned Ivy_Hash( Ivy_Obj_t * pObj, int TableSize ) 
{
    unsigned Key = Ivy_ObjIsExor(pObj) * 1699;
    Key ^= Ivy_ObjFaninId0(pObj) * 7937;
    Key ^= Ivy_ObjFaninId1(pObj) * 2971;
    Key ^= Ivy_ObjFaninC0(pObj) * 911;
    Key ^= Ivy_ObjFaninC1(pObj) * 353;
    Key ^= Ivy_ObjInit(pObj) * 911;
    return Key % TableSize;
}

// returns the place where this node is stored (or should be stored)
static int * Ivy_TableFind( Ivy_Man_t * p, Ivy_Obj_t * pObj )
{
    int i;
    assert( Ivy_ObjIsHash(pObj) );
    for ( i = Ivy_Hash(pObj, p->nTableSize); p->pTable[i]; i = (i+1) % p->nTableSize )
        if ( p->pTable[i] == pObj->Id )
            break;
    return p->pTable + i;
}

static void         Ivy_TableResize( Ivy_Man_t * p );
static unsigned int Cudd_PrimeAig( unsigned int  p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    [Checks if node with the given attributes is in the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ivy_Obj_t * Ivy_TableLookup( Ivy_Man_t * p, Ivy_Obj_t * pObj )
{
    Ivy_Obj_t * pEntry;
    int i;
    assert( !Ivy_IsComplement(pObj) );
    if ( !Ivy_ObjIsHash(pObj) )
        return NULL;
    assert( Ivy_ObjIsLatch(pObj) || Ivy_ObjFaninId0(pObj) > 0 );
    assert( Ivy_ObjFaninId1(pObj) == 0 || Ivy_ObjFaninId0(pObj) < Ivy_ObjFaninId1(pObj) );
    if ( Ivy_ObjFanin0(pObj)->nRefs == 0 || (Ivy_ObjChild1(pObj) && Ivy_ObjFanin1(pObj)->nRefs == 0) )
        return NULL;
    for ( i = Ivy_Hash(pObj, p->nTableSize); p->pTable[i]; i = (i+1) % p->nTableSize )
    {
        pEntry = Ivy_ManObj( p, p->pTable[i] );
        if ( Ivy_ObjChild0(pEntry) == Ivy_ObjChild0(pObj) && 
             Ivy_ObjChild1(pEntry) == Ivy_ObjChild1(pObj) && 
             Ivy_ObjInit(pEntry) == Ivy_ObjInit(pObj) && 
             Ivy_ObjType(pEntry) == Ivy_ObjType(pObj) )
            return pEntry;
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Adds the node to the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_TableInsert( Ivy_Man_t * p, Ivy_Obj_t * pObj )
{
    int * pPlace;
    assert( !Ivy_IsComplement(pObj) );
    if ( !Ivy_ObjIsHash(pObj) )
        return;
    if ( (pObj->Id & 63) == 0 )
    {
        if ( p->nTableSize < 2 * Ivy_ManHashObjNum(p) )
            Ivy_TableResize( p );
    }
    pPlace = Ivy_TableFind( p, pObj );
    assert( *pPlace == 0 );
    *pPlace = pObj->Id;
}

/**Function*************************************************************

  Synopsis    [Deletes the node from the hash table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_TableDelete( Ivy_Man_t * p, Ivy_Obj_t * pObj )
{
    Ivy_Obj_t * pEntry;
    int i, * pPlace;
    assert( !Ivy_IsComplement(pObj) );
    if ( !Ivy_ObjIsHash(pObj) )
        return;
    pPlace = Ivy_TableFind( p, pObj );
    assert( *pPlace == pObj->Id ); // node should be in the table
    *pPlace = 0;
    // rehash the adjacent entries
    i = pPlace - p->pTable;
    for ( i = (i+1) % p->nTableSize; p->pTable[i]; i = (i+1) % p->nTableSize )
    {
        pEntry = Ivy_ManObj( p, p->pTable[i] );
        p->pTable[i] = 0;
        Ivy_TableInsert( p, pEntry );
    }
}

/**Function*************************************************************

  Synopsis    [Updates the table to point to the new node.]

  Description [If the old node (pObj) is in the table, updates the table
  to point to an object with different ID (ObjIdNew). The table should
  not contain an object with ObjIdNew (this is currently not checked).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_TableUpdate( Ivy_Man_t * p, Ivy_Obj_t * pObj, int ObjIdNew )
{
    int * pPlace;
    assert( !Ivy_IsComplement(pObj) );
    if ( !Ivy_ObjIsHash(pObj) )
        return;
    pPlace = Ivy_TableFind( p, pObj );
    assert( *pPlace == pObj->Id ); // node should be in the table
    *pPlace = ObjIdNew; 
}

/**Function*************************************************************

  Synopsis    [Count the number of nodes in the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ivy_TableCountEntries( Ivy_Man_t * p )
{
    int i, Counter = 0;
    for ( i = 0; i < p->nTableSize; i++ )
        Counter += (p->pTable[i] != 0);
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description [Typically this procedure should not be called.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ivy_TableResize( Ivy_Man_t * p )
{
    int * pTableOld, * pPlace;
    int nTableSizeOld, Counter, nEntries, e;
    abctime clk;
clk = Abc_Clock();
    // save the old table
    pTableOld = p->pTable;
    nTableSizeOld = p->nTableSize;
    // get the new table
    p->nTableSize = Abc_PrimeCudd( 5 * Ivy_ManHashObjNum(p) ); 
    p->pTable = ABC_ALLOC( int, p->nTableSize );
    memset( p->pTable, 0, sizeof(int) * p->nTableSize );
    // rehash the entries from the old table
    Counter = 0;
    for ( e = 0; e < nTableSizeOld; e++ )
    {
        if ( pTableOld[e] == 0 )
            continue;
        Counter++;
        // get the place where this entry goes in the table table
        pPlace = Ivy_TableFind( p, Ivy_ManObj(p, pTableOld[e]) );
        assert( *pPlace == 0 ); // should not be in the table
        *pPlace = pTableOld[e];
    }
    nEntries = Ivy_ManHashObjNum(p);
//    assert( Counter == nEntries );
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
void Ivy_TableProfile( Ivy_Man_t * p )
{
    int i, Counter = 0;
    for ( i = 0; i < p->nTableSize; i++ )
    {
        if ( p->pTable[i] )
            Counter++;
        else if ( Counter )
        {
            printf( "%d ", Counter );
            Counter = 0;
        }
    }
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

