/**CFile****************************************************************

  FileName    [abcNpnSave.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Interface with the FPGA mapping package.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: abcNpnSave.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "aig/aig/aig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
typedef struct Npn_Obj_t_ Npn_Obj_t;
typedef struct Npn_Man_t_ Npn_Man_t;

struct Npn_Obj_t_
{
    word         uTruth;      // truth table
    int          Count;       // occurrences
    int          iNext;       // next entry
};
struct Npn_Man_t_
{
    Npn_Obj_t *  pBuffer;     // all NPN entries
    int *        pBins;       // hash table
    int          nBins;       // hash table size
    int          nBufferSize; // buffer size
    int          nEntries;    // entry count
};

static inline Npn_Obj_t * Npn_ManObj( Npn_Man_t * p, int i )                 { assert( i < p->nBufferSize ); return i ? p->pBuffer + i : NULL;  }
static inline int         Npn_ManObjNum( Npn_Man_t * p, Npn_Obj_t * pObj )   { assert( p->pBuffer < pObj );  return pObj - p->pBuffer;          }

static word Truth[8] = {
    ABC_CONST(0xAAAAAAAAAAAAAAAA),
    ABC_CONST(0xCCCCCCCCCCCCCCCC),
    ABC_CONST(0xF0F0F0F0F0F0F0F0),
    ABC_CONST(0xFF00FF00FF00FF00),
    ABC_CONST(0xFFFF0000FFFF0000),
    ABC_CONST(0xFFFFFFFF00000000),
    ABC_CONST(0x0000000000000000),
    ABC_CONST(0xFFFFFFFFFFFFFFFF)
};

static Npn_Man_t * pNpnMan = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Npn_TruthPermute_rec( char * pStr, int mid, int end )
{
    static int count = 0;
    char * pTemp = Abc_UtilStrsav(pStr);
    char e;
    int i;
    if ( mid == end ) 
    {
        printf( "%03d: %s\n", count++, pTemp );
        return ;
    }
    for ( i = mid; i <= end; i++ )
    {
        e = pTemp[mid];
        pTemp[mid] = pTemp[i];
        pTemp[i] = e;

        Npn_TruthPermute_rec( pTemp, mid + 1, end );

        e = pTemp[mid];
        pTemp[mid] = pTemp[i];
        pTemp[i] = e;
    }
    ABC_FREE( pTemp );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Npn_TruthHasVar( word t, int v )
{
    return ((t & Truth[v]) >> (1<<v)) != (t & ~Truth[v]);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Npn_TruthSupport( word t )
{
    int v, Supp = 0;
    for ( v = 0; v < 6; v++ )
        if ( Npn_TruthHasVar( t, v ) )
            Supp |= (1 << v);
    return Supp;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Npn_TruthSuppSize( word t, int nVars )
{
    int v, nSupp = 0;
    assert( nVars <= 6 );
    for ( v = 0; v < nVars; v++ )
        if ( Npn_TruthHasVar( t, v ) )
            nSupp++;
    return nSupp;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Npn_TruthIsMinBase( word t )
{
    int Supp = Npn_TruthSupport(t);
    return (Supp & (Supp+1)) == 0;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
word Npn_TruthPadWord( word uTruth, int nVars )
{
    if ( nVars == 6 )
        return uTruth;
    if ( nVars <= 5 )
        uTruth = ((uTruth & ABC_CONST(0x00000000FFFFFFFF)) << 32) | (uTruth & ABC_CONST(0x00000000FFFFFFFF));
    if ( nVars <= 4 )
        uTruth = ((uTruth & ABC_CONST(0x0000FFFF0000FFFF)) << 16) | (uTruth & ABC_CONST(0x0000FFFF0000FFFF));
    if ( nVars <= 3 )
        uTruth = ((uTruth & ABC_CONST(0x00FF00FF00FF00FF)) <<  8) | (uTruth & ABC_CONST(0x00FF00FF00FF00FF));
    if ( nVars <= 2 )
        uTruth = ((uTruth & ABC_CONST(0x0F0F0F0F0F0F0F0F)) <<  4) | (uTruth & ABC_CONST(0x0F0F0F0F0F0F0F0F));
    if ( nVars <= 1 )
        uTruth = ((uTruth & ABC_CONST(0x3333333333333333)) <<  2) | (uTruth & ABC_CONST(0x3333333333333333));
    if ( nVars == 0 )
        uTruth = ((uTruth & ABC_CONST(0x5555555555555555)) <<  1) | (uTruth & ABC_CONST(0x5555555555555555));
    return uTruth;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Npn_TruthCountOnes( word t )
{
    t =    (t & ABC_CONST(0x5555555555555555)) + ((t>> 1) & ABC_CONST(0x5555555555555555));
    t =    (t & ABC_CONST(0x3333333333333333)) + ((t>> 2) & ABC_CONST(0x3333333333333333));
    t =    (t & ABC_CONST(0x0F0F0F0F0F0F0F0F)) + ((t>> 4) & ABC_CONST(0x0F0F0F0F0F0F0F0F));
    t =    (t & ABC_CONST(0x00FF00FF00FF00FF)) + ((t>> 8) & ABC_CONST(0x00FF00FF00FF00FF));
    t =    (t & ABC_CONST(0x0000FFFF0000FFFF)) + ((t>>16) & ABC_CONST(0x0000FFFF0000FFFF));
    return (t & ABC_CONST(0x00000000FFFFFFFF)) +  (t>>32);
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Npn_TruthChangePhase( word t, int v )
{
    return ((t & Truth[v]) >> (1<<v)) | ((t & ~Truth[v]) << (1<<v));
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Npn_TruthSwapAdjacentVars( word t, int v )
{
    static word PMasks[5][3] = {
        { ABC_CONST(0x9999999999999999), ABC_CONST(0x2222222222222222), ABC_CONST(0x4444444444444444) },
        { ABC_CONST(0xC3C3C3C3C3C3C3C3), ABC_CONST(0x0C0C0C0C0C0C0C0C), ABC_CONST(0x3030303030303030) },
        { ABC_CONST(0xF00FF00FF00FF00F), ABC_CONST(0x00F000F000F000F0), ABC_CONST(0x0F000F000F000F00) },
        { ABC_CONST(0xFF0000FFFF0000FF), ABC_CONST(0x0000FF000000FF00), ABC_CONST(0x00FF000000FF0000) },
        { ABC_CONST(0xFFFF00000000FFFF), ABC_CONST(0x00000000FFFF0000), ABC_CONST(0x0000FFFF00000000) }
    };
    assert( v < 6 );
    return (t & PMasks[v][0]) | ((t & PMasks[v][1]) << (1 << v)) | ((t & PMasks[v][2]) >> (1 << v));
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline word Npn_TruthCanon( word t, int nVars, int * pPhase )
{
    int fUsePolarity    = 0;
    int fUsePermutation = 0;
    char Temp, pSigs[13], pCanonPerm[6];
    int v, fChange, CanonPhase = 0;
    assert( nVars < 7 );
    pSigs[12] = Npn_TruthCountOnes( t );
    if ( pSigs[12] > 32 )
    {
        t = ~t;
        pSigs[12] = 64 - pSigs[12];
        CanonPhase |= (1 << 6);
    }
    if ( fUsePolarity || fUsePermutation )
    {
        for ( v = 0; v < nVars; v++ )
        {
            pCanonPerm[v] = v;
            pSigs[2*v+1] = Npn_TruthCountOnes( t & Truth[v] );
            pSigs[2*v] = pSigs[12] - pSigs[2*v+1];
        }
    }
    if ( fUsePolarity )
    {
        for ( v = 0; v < nVars; v++ )
        {
            if ( pSigs[2*v] >= pSigs[2*v+1] )
                continue;
            CanonPhase |= (1 << v);
            Temp = pSigs[2*v];
            pSigs[2*v] = pSigs[2*v+1];
            pSigs[2*v+1] = Temp;
            t = Npn_TruthChangePhase( t, v );
        }
    }
    if ( fUsePermutation )
    {
        do {
            fChange = 0;
            for ( v = 0; v < nVars-1; v++ )
            {
                if ( fUsePolarity )
                {
                    if ( pSigs[2*v] >= pSigs[2*(v+1)] )
                        continue;
                }
                else
                {
                    if ( Abc_MinInt(pSigs[2*v],pSigs[2*v+1]) >= Abc_MinInt(pSigs[2*(v+1)],pSigs[2*(v+1)+1]) )
                        continue;
                }
                fChange = 1;

                Temp = pCanonPerm[v];
                pCanonPerm[v] = pCanonPerm[v+1];
                pCanonPerm[v+1] = Temp;

                Temp = pSigs[2*v];
                pSigs[2*v] = pSigs[2*(v+1)];
                pSigs[2*(v+1)] = Temp;

                Temp = pSigs[2*v+1];
                pSigs[2*v+1] = pSigs[2*(v+1)+1];
                pSigs[2*(v+1)+1] = Temp;

                t = Npn_TruthSwapAdjacentVars( t, v );
            }
        } while ( fChange );
    }
    if ( pPhase )
    {
        *pPhase = 0;
        for ( v = 0; v < nVars; v++ )
            *pPhase |= (pCanonPerm[v] << (4 * v));
        *pPhase |= (CanonPhase << 24);
    }
    return t;
}


/**Function*************************************************************

  Synopsis    [Computes the hash key.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int Npn_ManHash( Npn_Man_t * p, word uTruth )
{
    word Key = (uTruth * (word)101) ^ (uTruth * (word)733) ^ (uTruth * (word)1777);
    return (int)(Key % (word)p->nBins);
}

/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Npn_ManResize( Npn_Man_t * p )
{
    Npn_Obj_t * pEntry, * pNext;
    int * pBinsOld, * ppPlace;
    int nBinsOld, Counter, i;
    abctime clk;
    assert( p->pBins != NULL );
clk = Abc_Clock();
    // save the old Bins
    pBinsOld = p->pBins;
    nBinsOld = p->nBins;
    // get the new Bins
    p->nBins = Abc_PrimeCudd( 3 * nBinsOld ); 
    p->pBins = ABC_CALLOC( int, p->nBins );
    // rehash the entries from the old table
    Counter = 1;
    for ( i = 0; i < nBinsOld; i++ )
    for ( pEntry = Npn_ManObj(p, pBinsOld[i]), 
          pNext = pEntry ? Npn_ManObj(p, pEntry->iNext) : NULL; 
          pEntry; 
          pEntry = pNext, 
          pNext = pEntry ? Npn_ManObj(p, pEntry->iNext) : NULL )
    {
        // get the place where this entry goes 
        ppPlace = p->pBins + Npn_ManHash( p, pEntry->uTruth );
        // add the entry to the list
        pEntry->iNext = *ppPlace;
        *ppPlace = Npn_ManObjNum( p, pEntry );
        Counter++;
    }
    assert( Counter == p->nEntries );
    ABC_FREE( pBinsOld );
//ABC_PRT( "Hash table resizing time", Abc_Clock() - clk );
}

/**Function*************************************************************

  Synopsis    [Adds one entry to the table.]

  Description [Increments ref counter by 1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Npn_Obj_t * Npn_ManAdd( Npn_Man_t * p, word uTruth )
{
    Npn_Obj_t * pEntry;
    int * pPlace, Key = Npn_ManHash( p, uTruth );
    // resize the link storage if needed
    if ( p->nEntries == p->nBufferSize )
    {
        p->nBufferSize *= 2;
        p->pBuffer = ABC_REALLOC( Npn_Obj_t, p->pBuffer, p->nBufferSize );
    }
    // find the entry
    for ( pEntry = Npn_ManObj(p, p->pBins[Key]), 
          pPlace = p->pBins + Key; 
          pEntry; 
          pPlace = &pEntry->iNext, 
          pEntry = Npn_ManObj(p, pEntry->iNext) )
        if ( pEntry->uTruth == uTruth )
        {
            pEntry->Count++;
            return pEntry;
        }
    // create new entry
    *pPlace = p->nEntries;
    assert( p->nEntries < p->nBufferSize );
    pEntry = Npn_ManObj( p, p->nEntries++ );
    pEntry->uTruth = uTruth;
    pEntry->Count = 1;
    pEntry->iNext = 0;
    // resize the table if needed
    if ( p->nEntries > 3 * p->nBins )
        Npn_ManResize( p );
    return pEntry;
}

/**Function*************************************************************

  Synopsis    [Fills table from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Npn_ManRead( Npn_Man_t * p, char * pFileName )
{
    char pBuffer[1000];
    char * pToken;
    Npn_Obj_t * pEntry;
    unsigned Truth[2];
    word uTruth;
    FILE * pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
    {
        Abc_Print( -1, "Cannot open NPN function file \"%s\".\n", pFileName );
        return;
    }
    // read lines from the file
    while ( fgets( pBuffer, 1000, pFile ) != NULL )
    {
        pToken = strtok( pBuffer, " \t\n" );
        if ( pToken == NULL )
            continue;
        if ( pToken[0] == '#' )
            continue;
        if ( strlen(pToken) != 16 )
        {
            Abc_Print( 0, "Skipping token %s that does not look like a 16-digit hex number.\n" );
            continue;
        }
        // extract truth table
        Extra_ReadHexadecimal( Truth, pToken, 6 );
        uTruth = (((word)Truth[1]) << 32) | (word)Truth[0];
        // add truth table
        pEntry = Npn_ManAdd( p, uTruth );
        assert( pEntry->Count == 1 );
        // read area
        pToken = strtok( NULL, " \t\n" );
        pEntry->Count = atoi(pToken);
    }
    fclose( pFile );
}

/**Function*************************************************************

  Synopsis    [Comparison procedure for two entries.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Npn_ManCompareEntries( Npn_Obj_t ** pp1, Npn_Obj_t ** pp2 )
{
    if ( (*pp1)->Count > (*pp2)->Count )
        return -1;
    if ( (*pp1)->Count < (*pp2)->Count ) 
        return 1;
    return 0; 
}

/**Function*************************************************************

  Synopsis    [Adds one entry to the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Npn_ManWrite( Npn_Man_t * p, char * pFileName )
{
    Vec_Ptr_t * vEntries;
    Npn_Obj_t * pEntry;
    FILE * pFile = fopen( pFileName, "w" );
    int i;
    if ( pFile == NULL )
    {
        Abc_Print( -1, "Cannot open NPN function file \"%s\".\n", pFileName );
        return;
    }
    vEntries = Vec_PtrAlloc( p->nEntries );
    for ( i = 0; i < p->nBins; i++ )
        for ( pEntry = Npn_ManObj(p, p->pBins[i]); pEntry; pEntry = Npn_ManObj(p, pEntry->iNext) )
            Vec_PtrPush( vEntries, pEntry );
    Vec_PtrSort( vEntries, (int (*)(const void *, const void *))Npn_ManCompareEntries );
    Vec_PtrForEachEntry( Npn_Obj_t *, vEntries, pEntry, i )
    {
        Extra_PrintHexadecimal( pFile, (unsigned *)&pEntry->uTruth, 6 );
        fprintf( pFile, " %d %d\n", pEntry->Count, Npn_TruthSuppSize(pEntry->uTruth, 6) );
    }
    fclose( pFile );
    Vec_PtrFree( vEntries );
}

/**Function*************************************************************

  Synopsis    [Creates the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Npn_Man_t * Npn_ManStart( char * pFileName )
{
    Npn_Man_t * p;
    p = ABC_CALLOC( Npn_Man_t, 1 );
    if ( pFileName == NULL )
    {
        p->nBufferSize = 1000000;
        p->nBufferSize = 100;
        p->pBuffer     = ABC_ALLOC( Npn_Obj_t, p->nBufferSize );
        p->nBins       = Abc_PrimeCudd( p->nBufferSize / 2 );
        p->pBins       = ABC_CALLOC( int, p->nBins );
        p->nEntries    = 1;
    }
    else
    {
        FILE * pFile = fopen( pFileName, "r" );
        if ( pFile == NULL )
        {
            Abc_Print( -1, "Cannot open NPN function file \"%s\".\n", pFileName );
            return NULL;
        }
        fclose( pFile );
        p->nBufferSize = 4 * ( Extra_FileSize(pFileName) / 20 );
        p->pBuffer     = ABC_ALLOC( Npn_Obj_t, p->nBufferSize );
        p->nBins       = Abc_PrimeCudd( p->nBufferSize / 2 );
        p->pBins       = ABC_CALLOC( int, p->nBins );
        p->nEntries    = 1;
        Npn_ManRead( p, pFileName );
    }
    return p;
}

/**Function*************************************************************

  Synopsis    [Deletes the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Npn_ManStop( Npn_Man_t * p )
{
    ABC_FREE( p->pBuffer );
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Cleans the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Npn_ManClean()
{
    if ( pNpnMan != NULL )
    {
        Npn_ManStop( pNpnMan );
        pNpnMan = NULL;
    }
}

/**Function*************************************************************

  Synopsis    [Loads functions from a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Npn_ManLoad( char * pFileName )
{
//    Npn_TruthPermute_rec( "012345", 0, 5 );
    if ( pNpnMan != NULL )
    {
        Abc_Print( 1, "Removing old table with %d entries.\n", pNpnMan->nEntries );
        Npn_ManStop( pNpnMan );
    }
    pNpnMan = Npn_ManStart( pFileName );
    Abc_Print( 1, "Created new table with %d entries from file \"%s\".\n", pNpnMan->nEntries, pFileName );
}

/**Function*************************************************************

  Synopsis    [Saves functions into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Npn_ManSave( char * pFileName )
{
    if ( pNpnMan == NULL )
    {
        Abc_Print( 1, "There is no table with entries.\n" );
        return;
    }
    Npn_ManWrite( pNpnMan, pFileName );
    Abc_Print( 1, "Dumped table with %d entries from file \"%s\".\n", pNpnMan->nEntries, pFileName );
}

/**Function*************************************************************

  Synopsis    [Saves one function into storage.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Npn_ManSaveOne( unsigned * puTruth, int nVars )
{
    word uTruth = (((word)puTruth[1]) << 32) | (word)puTruth[0];
    assert( nVars >= 0 && nVars <= 6 );
    if ( pNpnMan == NULL )
    {
        Abc_Print( 1, "Creating new table with 0 entries.\n" );
        pNpnMan = Npn_ManStart( NULL );
    }
    // skip truth tables that do not depend on some vars
    if ( !Npn_TruthIsMinBase( uTruth ) )
        return;
    // extend truth table to look like 6-input
    uTruth = Npn_TruthPadWord( uTruth, nVars );
    // semi(!)-NPN-canonize the truth table
    uTruth = Npn_TruthCanon( uTruth, 6, NULL );
    // add to storage
    Npn_ManAdd( pNpnMan, uTruth );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

