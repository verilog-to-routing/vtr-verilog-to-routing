/**CFile****************************************************************

  FileName    [ioReadBlifAig.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to read BLIF file into AIG.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - December 23, 2006.]

  Revision    [$Id: ioReadBlifAig.c,v 1.00 2006/12/23 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "misc/vec/vecPtr.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

// latch initial values
typedef enum { 
    IO_BLIF_INIT_NONE = 0,  // 0:  unknown
    IO_BLIF_INIT_ZERO,      // 1:  zero
    IO_BLIF_INIT_ONE,       // 2:  one
    IO_BLIF_INIT_DC         // 3:  don't-care
} Io_BlifInit_t;

typedef struct Io_BlifObj_t_ Io_BlifObj_t;  // parsing object
struct Io_BlifObj_t_
{
    unsigned             fPi    :  1;  // the object is a primary input
    unsigned             fPo    :  1;  // the object is a primary output
    unsigned             fLi    :  1;  // the object is a latch input
    unsigned             fLo    :  1;  // the object is a latch output
    unsigned             fDef   :  1;  // the object is defined as a table (node, PO, LI)
    unsigned             fLoop  :  1;  // flag for loop detection
    unsigned             Init   :  2;  // the latch initial state
    unsigned             Offset : 24;  // temporary number
    char *               pName;        // the name of this object
    void *               pEquiv;       // the AIG node representing this line    
    Io_BlifObj_t *       pNext;        // the next obj in the hash table
};

typedef struct Io_BlifMan_t_ Io_BlifMan_t;  // parsing manager
struct Io_BlifMan_t_
{
    // general info about file
    char *               pFileName;    // the name of the file
    char *               pBuffer;      // the begining of the file buffer
    Vec_Ptr_t *          vLines;       // the line beginnings
    // temporary objects
    Io_BlifObj_t *       pObjects;     // the storage for objects
    int                  nObjects;     // the number of objects allocated
    int                  iObjNext;     // the next free object
    // file lines
    char *               pModel;       // .model line
    Vec_Ptr_t *          vInputs;      // .inputs lines
    Vec_Ptr_t *          vOutputs;     // .outputs lines
    Vec_Ptr_t *          vLatches;     // .latches lines
    Vec_Ptr_t *          vNames;       // .names lines
    // network objects
    Vec_Ptr_t *          vPis;         // the PI structures
    Vec_Ptr_t *          vPos;         // the PO structures
    Vec_Ptr_t *          vLis;         // the LI structures
    Vec_Ptr_t *          vLos;         // the LO structures
    // mapping of names into objects
    Io_BlifObj_t **      pTable;       // the hash table
    int                  nTableSize;   // the hash table size
    // current processing info
    Abc_Ntk_t *          pAig;         // the network under construction
    Vec_Ptr_t *          vTokens;      // the current tokens
    char                 sError[512];  // the error string generated during parsing
    // statistics 
    int                  nTablesRead;  // the number of processed tables
    int                  nTablesLeft;  // the number of dangling tables
};

// static functions
static Io_BlifMan_t *    Io_BlifAlloc();
static void              Io_BlifFree( Io_BlifMan_t * p );
static char *            Io_BlifLoadFile( char * pFileName );
static void              Io_BlifReadPreparse( Io_BlifMan_t * p );
static Abc_Ntk_t *       Io_BlifParse( Io_BlifMan_t * p );
static int               Io_BlifParseModel( Io_BlifMan_t * p, char * pLine );
static int               Io_BlifParseInputs( Io_BlifMan_t * p, char * pLine );
static int               Io_BlifParseOutputs( Io_BlifMan_t * p, char * pLine );
static int               Io_BlifParseLatch( Io_BlifMan_t * p, char * pLine );
static int               Io_BlifParseNames( Io_BlifMan_t * p, char * pLine );
static int               Io_BlifParseConstruct( Io_BlifMan_t * p );
static int               Io_BlifCharIsSpace( char s ) { return s == ' ' || s == '\t' || s == '\r' || s == '\n';  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the network from the BLIF file as an AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadBlifAsAig( char * pFileName, int fCheck )
{
    FILE * pFile;
    Io_BlifMan_t * p;
    Abc_Ntk_t * pAig;

    // check that the file is available
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Io_Blif(): The file is unavailable (absent or open).\n" );
        return 0;
    }
    fclose( pFile );

    // start the file reader
    p = Io_BlifAlloc();
    p->pFileName = pFileName;
    p->pBuffer   = Io_BlifLoadFile( pFileName );
    if ( p->pBuffer == NULL )
    {
        Io_BlifFree( p );
        return NULL;
    }
    // prepare the file for parsing
    Io_BlifReadPreparse( p );
    // construct the network
    pAig = Io_BlifParse( p );
    if ( p->sError[0] )
        fprintf( stdout, "%s\n", p->sError );
    if ( pAig == NULL )
        return NULL;
    Io_BlifFree( p );

    // make sure that everything is okay with the network structure
    if ( fCheck && !Abc_NtkCheckRead( pAig ) )
    {
        printf( "Io_Blif: The network check has failed.\n" );
        Abc_NtkDelete( pAig );
        return NULL;
    }
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Allocates the BLIF parsing structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Io_BlifMan_t * Io_BlifAlloc()
{
    Io_BlifMan_t * p;
    p = ABC_ALLOC( Io_BlifMan_t, 1 );
    memset( p, 0, sizeof(Io_BlifMan_t) );
    p->vLines   = Vec_PtrAlloc( 512 );
    p->vInputs  = Vec_PtrAlloc( 512 );
    p->vOutputs = Vec_PtrAlloc( 512 );
    p->vLatches = Vec_PtrAlloc( 512 );
    p->vNames   = Vec_PtrAlloc( 512 );
    p->vTokens  = Vec_PtrAlloc( 512 );
    p->vPis     = Vec_PtrAlloc( 512 );
    p->vPos     = Vec_PtrAlloc( 512 );
    p->vLis     = Vec_PtrAlloc( 512 );
    p->vLos     = Vec_PtrAlloc( 512 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees the BLIF parsing structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Io_BlifFree( Io_BlifMan_t * p )
{
    if ( p->pAig )
        Abc_NtkDelete( p->pAig );
    if ( p->pBuffer )  ABC_FREE( p->pBuffer );
    if ( p->pObjects ) ABC_FREE( p->pObjects );
    if ( p->pTable )   ABC_FREE( p->pTable );
    Vec_PtrFree( p->vLines );
    Vec_PtrFree( p->vInputs );
    Vec_PtrFree( p->vOutputs );
    Vec_PtrFree( p->vLatches );
    Vec_PtrFree( p->vNames );
    Vec_PtrFree( p->vTokens );
    Vec_PtrFree( p->vPis );
    Vec_PtrFree( p->vPos );
    Vec_PtrFree( p->vLis );
    Vec_PtrFree( p->vLos );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Hashing for character strings.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static unsigned Io_BlifHashString( char * pName, int TableSize ) 
{
    static int s_Primes[10] = { 
        1291, 1699, 2357, 4177, 5147, 
        5647, 6343, 7103, 7873, 8147
    };
    unsigned i, Key = 0;
    for ( i = 0; pName[i] != '\0'; i++ )
        Key ^= s_Primes[i%10]*pName[i]*pName[i];
    return Key % TableSize;
}

/**Function*************************************************************

  Synopsis    [Checks if the given name exists in the table.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Io_BlifObj_t ** Io_BlifHashLookup( Io_BlifMan_t * p, char * pName )
{
    Io_BlifObj_t ** ppEntry;
    for ( ppEntry = p->pTable + Io_BlifHashString(pName, p->nTableSize); *ppEntry; ppEntry = &(*ppEntry)->pNext )
        if ( !strcmp((*ppEntry)->pName, pName) )
            return ppEntry;
    return ppEntry;
}

/**Function*************************************************************

  Synopsis    [Finds or add the given name to the table.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Io_BlifObj_t * Io_BlifHashFindOrAdd( Io_BlifMan_t * p, char * pName )
{
    Io_BlifObj_t ** ppEntry;
    ppEntry = Io_BlifHashLookup( p, pName );
    if ( *ppEntry == NULL )
    {
        assert( p->iObjNext < p->nObjects ); 
        *ppEntry = p->pObjects + p->iObjNext++;
        (*ppEntry)->pName = pName;
    }
    return *ppEntry;
}


/**Function*************************************************************

  Synopsis    [Collects the already split tokens.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Io_BlifCollectTokens( Vec_Ptr_t * vTokens, char * pInput, char * pOutput )
{
    char * pCur;
    Vec_PtrClear( vTokens );
    for ( pCur = pInput; pCur < pOutput; pCur++ )
    {
        if ( *pCur == 0 )
            continue;
        Vec_PtrPush( vTokens, pCur );
        while ( *++pCur );
    }
}

/**Function*************************************************************

  Synopsis    [Splits the line into tokens.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Io_BlifSplitIntoTokens( Vec_Ptr_t * vTokens, char * pLine, char Stop )
{
    char * pCur;
    // clear spaces
    for ( pCur = pLine; *pCur != Stop; pCur++ )
        if ( Io_BlifCharIsSpace(*pCur) )
            *pCur = 0;
    // collect tokens
    Io_BlifCollectTokens( vTokens, pLine, pCur );
}

/**Function*************************************************************

  Synopsis    [Returns the 1-based number of the line in which the token occurs.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_BlifGetLine( Io_BlifMan_t * p, char * pToken )
{
    char * pLine;
    int i;
    Vec_PtrForEachEntry( char *, p->vLines, pLine, i )
        if ( pToken < pLine )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Conservatively estimates the number of primary inputs.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_BlifEstimatePiNum( Io_BlifMan_t * p )
{
    char * pCur;
    int i, fSpaces;
    int Counter = 0;
    Vec_PtrForEachEntry( char *, p->vInputs, pCur, i )
        for ( fSpaces = 0; *pCur; pCur++ )
        {
            if ( Io_BlifCharIsSpace(*pCur) )
            {
                if ( !fSpaces )
                    Counter++;
                fSpaces = 1;
            }
            else
                fSpaces = 0;
        }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Conservatively estimates the number of AIG nodes.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_BlifEstimateAndNum( Io_BlifMan_t * p )
{
    Io_BlifObj_t * pObj;
    char * pCur;
    int i, CounterOne, Counter = 0;
    for ( i = 0; i < p->iObjNext; i++ )
    {
        pObj = p->pObjects + i;
        if ( !pObj->fDef )
            continue;
        CounterOne = 0;
        for ( pCur = pObj->pName + strlen(pObj->pName); *pCur != '.'; pCur++ )
            if ( *pCur == '0' || *pCur == '1' )
                CounterOne++;
        if ( CounterOne )
            Counter += CounterOne - 1;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Reads the file into a character buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static char * Io_BlifLoadFile( char * pFileName )
{
    FILE * pFile;
    int nFileSize;
    char * pContents;
    int RetValue;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Io_BlifLoadFile(): The file is unavailable (absent or open).\n" );
        return NULL;
    }
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile ); 
    if ( nFileSize == 0 )
    {
        fclose( pFile );
        printf( "Io_BlifLoadFile(): The file is empty.\n" );
        return NULL;
    }
    pContents = ABC_ALLOC( char, nFileSize + 10 );
    rewind( pFile );
    RetValue = fread( pContents, nFileSize, 1, pFile );
    fclose( pFile );
    // finish off the file with the spare .end line
    // some benchmarks suddenly break off without this line
    strcpy( pContents + nFileSize, "\n.end\n" );
    return pContents;
}

/**Function*************************************************************

  Synopsis    [Prepares the parsing.]

  Description [Performs several preliminary operations:
  - Cuts the file buffer into separate lines.
  - Removes comments and line extenders.
  - Sorts lines by directives.
  - Estimates the number of objects.
  - Allocates room for the objects.
  - Allocates room for the hash table.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Io_BlifReadPreparse( Io_BlifMan_t * p )
{
    char * pCur, * pPrev;
    int i, fComment = 0;
    // parse the buffer into lines and remove comments
    Vec_PtrPush( p->vLines, p->pBuffer );
    for ( pCur = p->pBuffer; *pCur; pCur++ )
    {
        if ( *pCur == '\n' )
        {
            *pCur = 0;
            fComment = 0;
            Vec_PtrPush( p->vLines, pCur + 1 );
        }
        else if ( *pCur == '#' )
            fComment = 1;
        // remove comments
        if ( fComment )
            *pCur = 0;
    }

    // unfold the line extensions and sort lines by directive
    Vec_PtrForEachEntry( char *, p->vLines, pCur, i )
    {
        if ( *pCur == 0 )
            continue;
        // find previous non-space character
        for ( pPrev = pCur - 2; pPrev >= p->pBuffer; pPrev-- )
            if ( !Io_BlifCharIsSpace(*pPrev) )
                break;
        // if it is the line extender, overwrite it with spaces
        if ( *pPrev == '\\' )
        {
            for ( ; *pPrev; pPrev++ )
                *pPrev = ' ';
            *pPrev = ' ';
            continue;
        }
        // skip spaces at the beginning of the line
        while ( Io_BlifCharIsSpace(*pCur++) );
        // parse directives
        if ( *(pCur-1) != '.' )
            continue;
        if ( !strncmp(pCur, "names", 5) )
            Vec_PtrPush( p->vNames, pCur );
        else if ( !strncmp(pCur, "latch", 5) )
            Vec_PtrPush( p->vLatches, pCur );
        else if ( !strncmp(pCur, "inputs", 6) )
            Vec_PtrPush( p->vInputs, pCur );
        else if ( !strncmp(pCur, "outputs", 7) )
            Vec_PtrPush( p->vOutputs, pCur );
        else if ( !strncmp(pCur, "model", 5) ) 
            p->pModel = pCur;
        else if ( !strncmp(pCur, "end", 3) || !strncmp(pCur, "exdc", 4) )
            break;
        else
        {
            pCur--;
            if ( pCur[strlen(pCur)-1] == '\r' )
                pCur[strlen(pCur)-1] = 0;
            fprintf( stdout, "Line %d: Skipping line \"%s\".\n", Io_BlifGetLine(p, pCur), pCur );
        }
    }

    // count the number of objects
    p->nObjects = Io_BlifEstimatePiNum(p) + Vec_PtrSize(p->vLatches) + Vec_PtrSize(p->vNames) + 512;

    // allocate memory for objects
    p->pObjects = ABC_ALLOC( Io_BlifObj_t, p->nObjects );
    memset( p->pObjects, 0, p->nObjects * sizeof(Io_BlifObj_t) );

    // allocate memory for the hash table
    p->nTableSize = p->nObjects/2 + 1;
    p->pTable = ABC_ALLOC( Io_BlifObj_t *, p->nTableSize );
    memset( p->pTable, 0, p->nTableSize * sizeof(Io_BlifObj_t *) );
}


/**Function*************************************************************

  Synopsis    [Reads the AIG in the binary AIGER format.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Abc_Ntk_t * Io_BlifParse( Io_BlifMan_t * p )
{
    Abc_Ntk_t * pAig;
    char * pLine;
    int i;
    // parse the model
    if ( !Io_BlifParseModel( p, p->pModel ) )
        return NULL;
    // parse the inputs
    Vec_PtrForEachEntry( char *, p->vInputs, pLine, i )
        if ( !Io_BlifParseInputs( p, pLine ) )
            return NULL;
    // parse the outputs
    Vec_PtrForEachEntry( char *, p->vOutputs, pLine, i )
        if ( !Io_BlifParseOutputs( p, pLine ) )
            return NULL;
    // parse the latches
    Vec_PtrForEachEntry( char *, p->vLatches, pLine, i )
        if ( !Io_BlifParseLatch( p, pLine ) )
            return NULL;
    // parse the nodes
    Vec_PtrForEachEntry( char *, p->vNames, pLine, i )
        if ( !Io_BlifParseNames( p, pLine ) )
            return NULL;
    // reconstruct the network from the parsed data
    if ( !Io_BlifParseConstruct( p ) )
        return NULL;
    // return the network
    pAig = p->pAig;
    p->pAig = NULL;
    return pAig;
}

/**Function*************************************************************

  Synopsis    [Parses the model line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_BlifParseModel( Io_BlifMan_t * p, char * pLine )
{
    char * pToken;
    Io_BlifSplitIntoTokens( p->vTokens, pLine, '\0' );
    pToken = (char *)Vec_PtrEntry( p->vTokens, 0 );
    assert( !strcmp(pToken, "model") );
    if ( Vec_PtrSize(p->vTokens) != 2 )
    {
        sprintf( p->sError, "Line %d: Model line has %d entries while it should have 2.", Io_BlifGetLine(p, pToken), Vec_PtrSize(p->vTokens) );
        return 0;
    }
    p->pModel = (char *)Vec_PtrEntry( p->vTokens, 1 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the inputs line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_BlifParseInputs( Io_BlifMan_t * p, char * pLine )
{
    Io_BlifObj_t * pObj;
    char * pToken;
    int i;
    Io_BlifSplitIntoTokens( p->vTokens, pLine, '\0' );
    pToken = (char *)Vec_PtrEntry(p->vTokens, 0);
    assert( !strcmp(pToken, "inputs") );
    Vec_PtrForEachEntryStart( char *, p->vTokens, pToken, i, 1 )
    {
        pObj = Io_BlifHashFindOrAdd( p, pToken );
        if ( pObj->fPi )
        {
            sprintf( p->sError, "Line %d: Primary input (%s) is defined more than once.", Io_BlifGetLine(p, pToken), pToken );
            return 0;
        }
        pObj->fPi = 1;
        Vec_PtrPush( p->vPis, pObj );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the outputs line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_BlifParseOutputs( Io_BlifMan_t * p, char * pLine )
{
    Io_BlifObj_t * pObj;
    char * pToken;
    int i;
    Io_BlifSplitIntoTokens( p->vTokens, pLine, '\0' );
    pToken = (char *)Vec_PtrEntry(p->vTokens, 0);
    assert( !strcmp(pToken, "outputs") );
    Vec_PtrForEachEntryStart( char *, p->vTokens, pToken, i, 1 )
    {
        pObj = Io_BlifHashFindOrAdd( p, pToken );
        if ( pObj->fPo )
            fprintf( stdout, "Line %d: Primary output (%s) is defined more than once (warning only).\n", Io_BlifGetLine(p, pToken), pToken );
        pObj->fPo = 1;
        Vec_PtrPush( p->vPos, pObj );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the latches line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_BlifParseLatch( Io_BlifMan_t * p, char * pLine )
{
    Io_BlifObj_t * pObj;
    char * pToken;
    int Init;
    Io_BlifSplitIntoTokens( p->vTokens, pLine, '\0' );
    pToken = (char *)Vec_PtrEntry(p->vTokens,0);
    assert( !strcmp(pToken, "latch") );
    if ( Vec_PtrSize(p->vTokens) < 3 )
    {
        sprintf( p->sError, "Line %d: Latch does not have input name and output name.", Io_BlifGetLine(p, pToken) );
        return 0;
    }
    // get initial value
    if ( Vec_PtrSize(p->vTokens) > 3 )
        Init = atoi( (char *)Vec_PtrEntry(p->vTokens,3) );
    else
        Init = 2;
    if ( Init < 0 || Init > 2 )
    {
        sprintf( p->sError, "Line %d: Initial state of the latch is incorrect (%s).", Io_BlifGetLine(p, pToken), (char*)Vec_PtrEntry(p->vTokens,3) );
        return 0;
    }
    if ( Init == 0 )
        Init = IO_BLIF_INIT_ZERO;
    else if ( Init == 1 )
        Init = IO_BLIF_INIT_ONE;
    else // if ( Init == 2 )
        Init = IO_BLIF_INIT_DC;
    // get latch input
    pObj = Io_BlifHashFindOrAdd( p, (char *)Vec_PtrEntry(p->vTokens,1) );
    pObj->fLi = 1;
    Vec_PtrPush( p->vLis, pObj );
    pObj->Init = Init;
    // get latch output
    pObj = Io_BlifHashFindOrAdd( p, (char *)Vec_PtrEntry(p->vTokens,2) );
    if ( pObj->fPi )
    {
        sprintf( p->sError, "Line %d: Primary input (%s) is also defined latch output.", Io_BlifGetLine(p, pToken), (char*)Vec_PtrEntry(p->vTokens,2) );
        return 0;
    }
    if ( pObj->fLo )
    {
        sprintf( p->sError, "Line %d: Latch output (%s) is defined as the output of another latch.", Io_BlifGetLine(p, pToken), (char*)Vec_PtrEntry(p->vTokens,2) );
        return 0;
    }
    pObj->fLo = 1;
    Vec_PtrPush( p->vLos, pObj );
    pObj->Init = Init;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the nodes line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_BlifParseNames( Io_BlifMan_t * p, char * pLine )
{
    Io_BlifObj_t * pObj;
    char * pName;
    Io_BlifSplitIntoTokens( p->vTokens, pLine, '\0' );
    assert( !strcmp((char *)Vec_PtrEntry(p->vTokens,0), "names") );
    pName = (char *)Vec_PtrEntryLast( p->vTokens );
    pObj = Io_BlifHashFindOrAdd( p, pName );
    if ( pObj->fPi )
    {
        sprintf( p->sError, "Line %d: Primary input (%s) has a table.", Io_BlifGetLine(p, pName), pName );
        return 0;
    }
    if ( pObj->fLo )
    {
        sprintf( p->sError, "Line %d: Latch output (%s) has a table.", Io_BlifGetLine(p, pName), pName );
        return 0;
    }
    if ( pObj->fDef )
    {
        sprintf( p->sError, "Line %d: Signal (%s) is defined more than once.", Io_BlifGetLine(p, pName), pName );
        return 0;
    }
    pObj->fDef = 1;
    // remember offset to the first fanin name
    pObj->pName = pName;
    pObj->Offset = pObj->pName - (char *)Vec_PtrEntry(p->vTokens,1);
    return 1;
}


/**Function*************************************************************

  Synopsis    [Constructs the AIG from the file parsing info.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Abc_Obj_t * Io_BlifParseTable( Io_BlifMan_t * p, char * pTable, Vec_Ptr_t * vFanins )
{
    char * pProduct, * pOutput;
    Abc_Obj_t * pRes, * pCube;
    int i, k, Polarity = -1;

    p->nTablesRead++;
    // get the tokens
    Io_BlifSplitIntoTokens( p->vTokens, pTable, '.' );
    if ( Vec_PtrSize(p->vTokens) == 0 )
        return Abc_ObjNot( Abc_AigConst1(p->pAig) );
    if ( Vec_PtrSize(p->vTokens) == 1 )
    {
        pOutput = (char *)Vec_PtrEntry( p->vTokens, 0 );
        if ( ((pOutput[0] - '0') & 0x8E) || pOutput[1] )
        {
            sprintf( p->sError, "Line %d: Constant table has wrong output value (%s).", Io_BlifGetLine(p, pOutput), pOutput );
            return NULL;
        }
        return Abc_ObjNotCond( Abc_AigConst1(p->pAig), pOutput[0] == '0' );
    }
    pProduct = (char *)Vec_PtrEntry( p->vTokens, 0 );
    if ( Vec_PtrSize(p->vTokens) % 2 == 1 )
    {
        sprintf( p->sError, "Line %d: Table has odd number of tokens (%d).", Io_BlifGetLine(p, pProduct), Vec_PtrSize(p->vTokens) );
        return NULL;
    }
    // parse the table
    pRes = Abc_ObjNot( Abc_AigConst1(p->pAig) );
    for ( i = 0; i < Vec_PtrSize(p->vTokens)/2; i++ )
    {
        pProduct = (char *)Vec_PtrEntry( p->vTokens, 2*i + 0 );
        pOutput  = (char *)Vec_PtrEntry( p->vTokens, 2*i + 1 );
        if ( strlen(pProduct) != (unsigned)Vec_PtrSize(vFanins) )
        {
            sprintf( p->sError, "Line %d: Cube (%s) has size different from the fanin count (%d).", Io_BlifGetLine(p, pProduct), pProduct, Vec_PtrSize(vFanins) );
            return NULL;
        }
        if ( ((pOutput[0] - '0') & 0x8E) || pOutput[1] )
        {
            sprintf( p->sError, "Line %d: Output value (%s) is incorrect.", Io_BlifGetLine(p, pProduct), pOutput );
            return NULL;
        }
        if ( Polarity == -1 )
            Polarity = pOutput[0] - '0';
        else if ( Polarity != pOutput[0] - '0' )
        {
            sprintf( p->sError, "Line %d: Output value (%s) differs from the value in the first line of the table (%d).", Io_BlifGetLine(p, pProduct), pOutput, Polarity );
            return NULL;
        }
        // parse one product product
        pCube = Abc_AigConst1(p->pAig);
        for ( k = 0; pProduct[k]; k++ )
        {
            if ( pProduct[k] == '0' )
                pCube = Abc_AigAnd( (Abc_Aig_t *)p->pAig->pManFunc, pCube, Abc_ObjNot((Abc_Obj_t *)Vec_PtrEntry(vFanins,k)) );
            else if ( pProduct[k] == '1' )
                pCube = Abc_AigAnd( (Abc_Aig_t *)p->pAig->pManFunc, pCube, (Abc_Obj_t *)Vec_PtrEntry(vFanins,k) );
            else if ( pProduct[k] != '-' )
            {
                sprintf( p->sError, "Line %d: Product term (%s) contains character (%c).", Io_BlifGetLine(p, pProduct), pProduct, pProduct[k] );
                return NULL;
            }
        }
        pRes = Abc_AigOr( (Abc_Aig_t *)p->pAig->pManFunc, pRes, pCube );
    }
    pRes = Abc_ObjNotCond( pRes, Polarity == 0 );
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Constructs the AIG from the file parsing info.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Abc_Obj_t * Io_BlifParseConstruct_rec( Io_BlifMan_t * p, char * pName )
{
    Vec_Ptr_t * vFanins;
    Abc_Obj_t * pFaninAbc;
    Io_BlifObj_t * pObjIo;
    char * pNameFanin;
    int i;
    // get the IO object with this name
    pObjIo = *Io_BlifHashLookup( p, pName );
    if ( pObjIo == NULL )
    {
        sprintf( p->sError, "Line %d: Signal (%s) is not defined as a table.", Io_BlifGetLine(p, pName), pName );
        return NULL;
    }
    // loop detection
    if ( pObjIo->fLoop )
    {
        sprintf( p->sError, "Line %d: Signal (%s) appears twice on a combinational path.", Io_BlifGetLine(p, pName), pName );
        return NULL;
    }
    // check if the AIG is already constructed
    if ( pObjIo->pEquiv )
        return (Abc_Obj_t *)pObjIo->pEquiv;
    // mark this node on the path
    pObjIo->fLoop = 1;
    // construct the AIGs for the fanins
    vFanins = Vec_PtrAlloc( 8 );
    Io_BlifCollectTokens( vFanins, pObjIo->pName - pObjIo->Offset, pObjIo->pName );
    Vec_PtrForEachEntry( char *, vFanins, pNameFanin, i )
    {
        pFaninAbc = Io_BlifParseConstruct_rec( p, pNameFanin );
        if ( pFaninAbc == NULL )
        {
            Vec_PtrFree( vFanins );
            return NULL;
        }
        Vec_PtrWriteEntry( vFanins, i, pFaninAbc );
    }
    // construct the node
    pObjIo->pEquiv = Io_BlifParseTable( p, pObjIo->pName + strlen(pObjIo->pName), vFanins );
    Vec_PtrFree( vFanins );
    // unmark this node on the path
    pObjIo->fLoop = 0;
    // remember the new node
    return (Abc_Obj_t *)pObjIo->pEquiv;
}

/**Function*************************************************************

  Synopsis    [Constructs the AIG from the file parsing info.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_BlifParseConstruct( Io_BlifMan_t * p )
{
    Abc_Ntk_t * pAig;
    Io_BlifObj_t * pObjIo, * pObjIoInput;
    Abc_Obj_t * pObj, * pLatch;
    int i;
    // allocate the empty AIG
    pAig = p->pAig = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    pAig->pName = Extra_UtilStrsav( p->pModel );
    pAig->pSpec = Extra_UtilStrsav( p->pFileName );
    // create PIs
    Vec_PtrForEachEntry( Io_BlifObj_t *, p->vPis, pObjIo, i )
    {
        pObj = Abc_NtkCreatePi( pAig );
        Abc_ObjAssignName( pObj, pObjIo->pName, NULL );
        pObjIo->pEquiv = pObj;
    }
    // create POs
    Vec_PtrForEachEntry( Io_BlifObj_t *, p->vPos, pObjIo, i )
    {
        pObj = Abc_NtkCreatePo( pAig );
        Abc_ObjAssignName( pObj, pObjIo->pName, NULL );
    }
    // create latches
    Vec_PtrForEachEntry( Io_BlifObj_t *, p->vLos, pObjIo, i )
    {
        // add the latch input terminal
        pObj = Abc_NtkCreateBi( pAig );
        pObjIoInput = (Io_BlifObj_t *)Vec_PtrEntry( p->vLis, i );
        Abc_ObjAssignName( pObj, pObjIoInput->pName, NULL );
        
        // add the latch box
        pLatch = Abc_NtkCreateLatch( pAig );
        pLatch->pData = (void *)(ABC_PTRUINT_T)pObjIo->Init;
        Abc_ObjAssignName( pLatch, pObjIo->pName, "L" );
        Abc_ObjAddFanin( pLatch, pObj  );

        // add the latch output terminal
        pObj = Abc_NtkCreateBo( pAig );
        Abc_ObjAssignName( pObj, pObjIo->pName, NULL );
        Abc_ObjAddFanin( pObj, pLatch );
        // set the value of the latch output
//        pObjIo->pEquiv = Abc_ObjNotCond( pObj, pObjIo->Init );
        pObjIo->pEquiv = pObj;
    }
    // traverse the nodes from the POs
    Vec_PtrForEachEntry( Io_BlifObj_t *, p->vPos, pObjIo, i )
    {
        pObj = Io_BlifParseConstruct_rec( p, pObjIo->pName );
        if ( pObj == NULL )
            return 0;
        Abc_ObjAddFanin( Abc_NtkPo(p->pAig, i), pObj );
    }
    // traverse the nodes from the latch inputs
    Vec_PtrForEachEntry( Io_BlifObj_t *, p->vLis, pObjIo, i )
    {
        pObj = Io_BlifParseConstruct_rec( p, pObjIo->pName );
        if ( pObj == NULL )
            return 0;
//        pObj = Abc_ObjNotCond( pObj, pObjIo->Init );
        Abc_ObjAddFanin( Abc_ObjFanin0(Abc_NtkBox(p->pAig, i)), pObj );
    }
    p->nTablesLeft = Vec_PtrSize(p->vNames) - p->nTablesRead; 
    if ( p->nTablesLeft ) 
        printf( "The number of dangling tables = %d.\n", p->nTablesLeft );
    printf( "AND nodes = %6d.  Estimate = %6d.\n", Abc_NtkNodeNum(p->pAig), Io_BlifEstimateAndNum(p) );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

