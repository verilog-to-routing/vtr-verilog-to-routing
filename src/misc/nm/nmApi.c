/**CFile****************************************************************

  FileName    [nmApi.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Name manager.]

  Synopsis    [APIs of the name manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nmApi.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "nmInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the name manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Nm_Man_t * Nm_ManCreate( int nSize )
{
    Nm_Man_t * p;
    // allocate the table
    p = ABC_ALLOC( Nm_Man_t, 1 );
    memset( p, 0, sizeof(Nm_Man_t) );
    // set the parameters
    p->nSizeFactor   = 2; // determined the limit on the grow of data before the table resizes
    p->nGrowthFactor = 3; // determined how much the table grows after resizing
    // allocate and clean the bins
    p->nBins = Abc_PrimeCudd(nSize);
    p->pBinsI2N = ABC_ALLOC( Nm_Entry_t *, p->nBins );
    p->pBinsN2I = ABC_ALLOC( Nm_Entry_t *, p->nBins );
    memset( p->pBinsI2N, 0, sizeof(Nm_Entry_t *) * p->nBins );
    memset( p->pBinsN2I, 0, sizeof(Nm_Entry_t *) * p->nBins );
    // start the memory manager
    p->pMem = Extra_MmFlexStart();
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates the name manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nm_ManFree( Nm_Man_t * p )
{
    Extra_MmFlexStop( p->pMem );
    ABC_FREE( p->pBinsI2N );
    ABC_FREE( p->pBinsN2I );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns the number of objects with names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nm_ManNumEntries( Nm_Man_t * p )
{
    return p->nEntries;
}

/**Function*************************************************************

  Synopsis    [Creates a new entry in the name manager.]

  Description [Returns 1 if the entry with the given object ID
  already exists in the name manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Nm_ManStoreIdName( Nm_Man_t * p, int ObjId, int Type, char * pName, char * pSuffix )
{
    Nm_Entry_t * pEntry;
    int RetValue, nEntrySize;
    // check if the object with this ID is already stored
    if ( (pEntry = Nm_ManTableLookupId(p, ObjId)) )
    {
        printf( "Nm_ManStoreIdName(): Entry with the same ID already exists.\n" );
        return NULL;
    }
    // create a new entry
    nEntrySize = sizeof(Nm_Entry_t) + strlen(pName) + (pSuffix?strlen(pSuffix):0) + 1;
//    nEntrySize = (nEntrySize / 4 + ((nEntrySize % 4) > 0)) * 4;
    nEntrySize = (nEntrySize / sizeof(char*) + ((nEntrySize % sizeof(char*)) > 0)) * sizeof(char*); // added by Saurabh on Sep 3, 2009
    pEntry = (Nm_Entry_t *)Extra_MmFlexEntryFetch( p->pMem, nEntrySize );
    pEntry->pNextI2N = pEntry->pNextN2I = pEntry->pNameSake = NULL;
    pEntry->ObjId = ObjId;
    pEntry->Type = Type;
    sprintf( pEntry->Name, "%s%s", pName, pSuffix? pSuffix : "" );
    // add the entry to the hash table
    RetValue = Nm_ManTableAdd( p, pEntry );
    assert( RetValue == 1 );
    return pEntry->Name;
}

/**Function*************************************************************

  Synopsis    [Creates a new entry in the name manager.]

  Description [Returns 1 if the entry with the given object ID
  already exists in the name manager.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Nm_ManDeleteIdName( Nm_Man_t * p, int ObjId )
{
    Nm_Entry_t * pEntry;
    pEntry = Nm_ManTableLookupId(p, ObjId);
    if ( pEntry == NULL )
    {
        printf( "Nm_ManDeleteIdName(): This entry is not in the table.\n" );
        return;
    }
    // remove entry from the table
    Nm_ManTableDelete( p, ObjId );
}


/**Function*************************************************************

  Synopsis    [Finds a unique name for the node.]

  Description [If the name exists, tries appending numbers to it until 
  it becomes unique. The name is not added to the table.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Nm_ManCreateUniqueName( Nm_Man_t * p, int ObjId )
{
    static char NameStr[1000];
    Nm_Entry_t * pEntry;
    int i;
    if ( (pEntry = Nm_ManTableLookupId(p, ObjId)) )
        return pEntry->Name;
    sprintf( NameStr, "n%d", ObjId );
    for ( i = 1; Nm_ManTableLookupName(p, NameStr, -1); i++ )
        sprintf( NameStr, "n%d_%d", ObjId, i );
    return NameStr;
}

/**Function*************************************************************

  Synopsis    [Returns name of the object if the ID is known.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Nm_ManFindNameById( Nm_Man_t * p, int ObjId )
{
    Nm_Entry_t * pEntry;
    if ( (pEntry = Nm_ManTableLookupId(p, ObjId)) )
        return pEntry->Name;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Returns ID of the object if its name is known.]

  Description [This procedure may return two IDs because POs and latches 
  may have the same name (the only allowed case of name duplication).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nm_ManFindIdByName( Nm_Man_t * p, char * pName, int Type )
{
    Nm_Entry_t * pEntry;
    if ( (pEntry = Nm_ManTableLookupName(p, pName, Type)) )
        return pEntry->ObjId;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Returns ID of the object if its name is known.]

  Description [This procedure may return two IDs because POs and latches 
  may have the same name (the only allowed case of name duplication).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Nm_ManFindIdByNameTwoTypes( Nm_Man_t * p, char * pName, int Type1, int Type2 )
{
    int iNodeId;
    iNodeId = Nm_ManFindIdByName( p, pName, Type1 );
    if ( iNodeId == -1 )
        iNodeId = Nm_ManFindIdByName( p, pName, Type2 );
    if ( iNodeId == -1 )
        return -1;
    return iNodeId;
}

/**Function*************************************************************

  Synopsis    [Return the IDs of objects with names.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Nm_ManReturnNameIds( Nm_Man_t * p )
{
    Vec_Int_t * vNameIds;
    int i;
    vNameIds = Vec_IntAlloc( p->nEntries );
    for ( i = 0; i < p->nBins; i++ )
        if ( p->pBinsI2N[i] )
            Vec_IntPush( vNameIds, p->pBinsI2N[i]->ObjId );
    return vNameIds;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

