/**CFile****************************************************************

  FileName    [ioReadBlifMv.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to read BLIF-MV file.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 8, 2007.]

  Revision    [$Id: ioReadBlifMv.c,v 1.00 2007/01/08 00:00:00 alanmi Exp $]

***********************************************************************/

#include "misc/zlib/zlib.h"
#include "misc/bzlib/bzlib.h"
#include "base/abc/abc.h"
#include "misc/vec/vecPtr.h"
#include "ioAbc.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define IO_BLIFMV_MAXVALUES 256
//#define IO_VERBOSE_OUTPUT

typedef struct Io_MvVar_t_ Io_MvVar_t; // parsing var
typedef struct Io_MvMod_t_ Io_MvMod_t; // parsing model
typedef struct Io_MvMan_t_ Io_MvMan_t; // parsing manager

Vec_Ptr_t *vGlobalLtlArray;

struct Io_MvVar_t_
{
    int                  nValues;      // the number of values 
    char **              pNames;       // the value names
};

struct Io_MvMod_t_
{
    // file lines
    char *               pName;        // .model line
    Vec_Ptr_t *          vInputs;      // .inputs lines
    Vec_Ptr_t *          vOutputs;     // .outputs lines
    Vec_Ptr_t *          vLatches;     // .latch lines
    Vec_Ptr_t *          vFlops;       // .flop lines
    Vec_Ptr_t *          vResets;      // .reset lines
    Vec_Ptr_t *          vNames;       // .names lines
    Vec_Ptr_t *          vSubckts;     // .subckt lines
    Vec_Ptr_t *          vShorts;      // .short lines
    Vec_Ptr_t *          vOnehots;     // .onehot lines
    Vec_Ptr_t *          vMvs;         // .mv lines
    Vec_Ptr_t *          vConstrs;     // .constraint lines
    Vec_Ptr_t *             vLtlProperties;
    int                  fBlackBox;    // indicates blackbox model
    // the resulting network
    Abc_Ntk_t *          pNtk;   
    Abc_Obj_t *          pResetLatch; 
    // the parent manager
    Io_MvMan_t *         pMan;         
};

struct Io_MvMan_t_
{
    // general info about file
    int                  fBlifMv;      // the file is BLIF-MV
    int                  fUseReset;    // the reset circuitry is added
    char *               pFileName;    // the name of the file
    char *               pBuffer;      // the contents of the file
    Vec_Ptr_t *          vLines;       // the line beginnings
    // the results of reading
    Abc_Des_t *          pDesign;      // the design under construction
    int                  nNDnodes;     // the counter of ND nodes
    // intermediate storage for models
    Vec_Ptr_t *          vModels;      // vector of models
    Io_MvMod_t *         pLatest;      // the current model
    // current processing info
    Vec_Ptr_t *          vTokens;      // the current tokens
    Vec_Ptr_t *          vTokens2;     // the current tokens
    Vec_Str_t *          vFunc;        // the local function
    // error reporting
    char                 sError[512];  // the error string generated during parsing
    // statistics 
    int                  nTablesRead;  // the number of processed tables
    int                  nTablesLeft;  // the number of dangling tables
};

// static functions
static Io_MvMan_t *      Io_MvAlloc();
static void              Io_MvFree( Io_MvMan_t * p );
static Io_MvMod_t *      Io_MvModAlloc();
static void              Io_MvModFree( Io_MvMod_t * p );
static char *            Io_MvLoadFile( char * pFileName );
static void              Io_MvReadPreparse( Io_MvMan_t * p );
static int               Io_MvReadInterfaces( Io_MvMan_t * p );
static Abc_Des_t *       Io_MvParse( Io_MvMan_t * p );
static int               Io_MvParseLineModel( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineInputs( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineOutputs( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineConstrs( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineLatch( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineFlop( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineSubckt( Io_MvMod_t * p, char * pLine );
static Vec_Int_t *       Io_MvParseLineOnehot( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineMv( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineNamesMv( Io_MvMod_t * p, char * pLine, int fReset );
static int               Io_MvParseLineNamesBlif( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineShortBlif( Io_MvMod_t * p, char * pLine );
static int                 Io_MvParseLineLtlProperty( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineGateBlif( Io_MvMod_t * p, Vec_Ptr_t * vTokens );
static Io_MvVar_t *      Abc_NtkMvVarDup( Abc_Ntk_t * pNtk, Io_MvVar_t * pVar );

static int               Io_MvCharIsSpace( char s )  { return s == ' ' || s == '\t' || s == '\r' || s == '\n';  }
static int               Io_MvCharIsMvSymb( char s ) { return s == '(' || s == ')' || s == '{' || s == '}' || s == '-' || s == ',' || s == '!';  }

extern void              Abc_NtkStartMvVars( Abc_Ntk_t * pNtk );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the network from the BLIF or BLIF-MV file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadBlifMv( char * pFileName, int fBlifMv, int fCheck )
{
    FILE * pFile;
    Io_MvMan_t * p;
    Abc_Ntk_t * pNtk, * pExdc;
    Abc_Des_t * pDesign = NULL; 
    char * pDesignName;
    int RetValue, i;
    char * pLtlProp;

    // check that the file is available
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Io_ReadBlifMv(): The file is unavailable (absent or open).\n" );
        return 0;
    }
    fclose( pFile );

    // start the file reader
    p = Io_MvAlloc();
    p->fBlifMv   = fBlifMv;
    p->fUseReset = 1;
    p->pFileName = pFileName;
    p->pBuffer   = Io_MvLoadFile( pFileName );
    if ( p->pBuffer == NULL )
    {
        Io_MvFree( p );
        return NULL;
    }
    // set the design name
    pDesignName  = Extra_FileNameGeneric( pFileName );
    p->pDesign   = Abc_DesCreate( pDesignName );
    ABC_FREE( pDesignName );
    // free the HOP manager
    Hop_ManStop( (Hop_Man_t *)p->pDesign->pManFunc );
    p->pDesign->pManFunc = NULL;
    // prepare the file for parsing
    Io_MvReadPreparse( p );
    // parse interfaces of each network and construct the network
    if ( Io_MvReadInterfaces( p ) )
        pDesign = Io_MvParse( p );
    if ( p->sError[0] )
        fprintf( stdout, "%s\n", p->sError );
    Io_MvFree( p );
    if ( pDesign == NULL )
        return NULL;
// pDesign should be linked to all models of the design

    // make sure that everything is okay with the network structure
    if ( fCheck )
    {
        Vec_PtrForEachEntry( Abc_Ntk_t *, pDesign->vModules, pNtk, i )
        {
            if ( !Abc_NtkCheckRead( pNtk ) )
            {
                printf( "Io_ReadBlifMv: The network check has failed for model %s.\n", pNtk->pName );
                Abc_DesFree( pDesign, NULL );
                return NULL;
            }
        }
    }

//Abc_DesPrint( pDesign );

    // check if there is an EXDC network
    if ( Vec_PtrSize(pDesign->vModules) > 1 )
    {
        pNtk = (Abc_Ntk_t *)Vec_PtrEntry(pDesign->vModules, 0);
        Vec_PtrForEachEntryStart( Abc_Ntk_t *, pDesign->vModules, pExdc, i, 1 )
            if ( !strcmp(pExdc->pName, "EXDC") )
            {
                assert( pNtk->pExdc == NULL );
                pNtk->pExdc = pExdc;
                Vec_PtrRemove(pDesign->vModules, pExdc);
                pExdc->pDesign = NULL;
                i--;
            }
            else
                pNtk = pExdc;
    }

    // detect top-level model
    RetValue = Abc_DesFindTopLevelModels( pDesign );
    pNtk = (Abc_Ntk_t *)Vec_PtrEntry( pDesign->vTops, 0 );
    if ( RetValue > 1 )
        printf( "Warning: The design has %d root-level modules. The first one (%s) will be used.\n",
            Vec_PtrSize(pDesign->vTops), pNtk->pName );

    // extract the master network
    pNtk->pDesign = pDesign;
    pDesign->pManFunc = NULL;

    // verify the design for cyclic dependence
    assert( Vec_PtrSize(pDesign->vModules) > 0 );
    if ( Vec_PtrSize(pDesign->vModules) == 1 )
    {
//        printf( "Warning: The design is not hierarchical.\n" );
        Abc_DesFree( pDesign, pNtk );
        pNtk->pDesign = NULL;
        pNtk->pSpec = Extra_UtilStrsav( pFileName );
    }
    else
        Abc_NtkIsAcyclicHierarchy( pNtk );

//Io_WriteBlifMv( pNtk, "_temp_.mv" );
    if ( pNtk->pSpec == NULL )
        pNtk->pSpec = Extra_UtilStrsav( pFileName );

    vGlobalLtlArray = Vec_PtrAlloc( 100 );
    Vec_PtrForEachEntry( char *, vGlobalLtlArray, pLtlProp, i )
        Vec_PtrPush( pNtk->vLtlProperties, pLtlProp );
    Vec_PtrFreeP( &vGlobalLtlArray );
    return pNtk;
}

/**Function*************************************************************

  Synopsis    [Allocates the BLIF parsing structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Io_MvMan_t * Io_MvAlloc()
{
    Io_MvMan_t * p;
    p = ABC_ALLOC( Io_MvMan_t, 1 );
    memset( p, 0, sizeof(Io_MvMan_t) );
    p->vLines   = Vec_PtrAlloc( 512 );
    p->vModels  = Vec_PtrAlloc( 512 );
    p->vTokens  = Vec_PtrAlloc( 512 );
    p->vTokens2 = Vec_PtrAlloc( 512 );
    p->vFunc    = Vec_StrAlloc( 512 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Frees the BLIF parsing structure.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Io_MvFree( Io_MvMan_t * p )
{
    Io_MvMod_t * pMod;
    int i;
    if ( p->pDesign )
        Abc_DesFree( p->pDesign, NULL );
    if ( p->pBuffer )  
        ABC_FREE( p->pBuffer );
    if ( p->vLines )
        Vec_PtrFree( p->vLines  );
    if ( p->vModels )
    {
        Vec_PtrForEachEntry( Io_MvMod_t *, p->vModels, pMod, i )
            Io_MvModFree( pMod );
        Vec_PtrFree( p->vModels );
    }
    Vec_PtrFree( p->vTokens );
    Vec_PtrFree( p->vTokens2 );
    Vec_StrFree( p->vFunc );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Allocates the BLIF parsing structure for one model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Io_MvMod_t * Io_MvModAlloc()
{
    Io_MvMod_t * p;
    p = ABC_ALLOC( Io_MvMod_t, 1 );
    memset( p, 0, sizeof(Io_MvMod_t) );
    p->vInputs  = Vec_PtrAlloc( 512 );
    p->vOutputs = Vec_PtrAlloc( 512 );
    p->vLatches = Vec_PtrAlloc( 512 );
    p->vFlops   = Vec_PtrAlloc( 512 );
    p->vResets  = Vec_PtrAlloc( 512 );
    p->vNames   = Vec_PtrAlloc( 512 );
    p->vSubckts = Vec_PtrAlloc( 512 );
    p->vShorts  = Vec_PtrAlloc( 512 );
    p->vOnehots = Vec_PtrAlloc( 512 );
    p->vMvs     = Vec_PtrAlloc( 512 );
    p->vConstrs = Vec_PtrAlloc( 512 );
    p->vLtlProperties = Vec_PtrAlloc( 512 );
    return p;
}

/**Function*************************************************************

  Synopsis    [Allocates the BLIF parsing structure for one model.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Io_MvModFree( Io_MvMod_t * p )
{
//    if ( p->pNtk )
//        Abc_NtkDelete( p->pNtk );
    Vec_PtrFree( p->vLtlProperties );
    Vec_PtrFree( p->vInputs );
    Vec_PtrFree( p->vOutputs );
    Vec_PtrFree( p->vLatches );
    Vec_PtrFree( p->vFlops );
    Vec_PtrFree( p->vResets );
    Vec_PtrFree( p->vNames );
    Vec_PtrFree( p->vSubckts );
    Vec_PtrFree( p->vShorts );
    Vec_PtrFree( p->vOnehots );
    Vec_PtrFree( p->vMvs );
    Vec_PtrFree( p->vConstrs );
    ABC_FREE( p );
}



/**Function*************************************************************

  Synopsis    [Counts the number of given chars.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvCountChars( char * pLine, char Char )
{
    char * pCur;
    int Counter = 0;
    for ( pCur = pLine; *pCur; pCur++ )
        if ( *pCur == Char )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Returns the place where the arrow is hiding.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static char * Io_MvFindArrow( char * pLine )
{
    char * pCur;
    for ( pCur = pLine; *(pCur+1); pCur++ )
        if ( *pCur == '-' && *(pCur+1) == '>' )
        {
            *pCur = ' ';
            *(pCur+1) = ' ';
            return pCur;
        }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Collects the already split tokens.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Io_MvCollectTokens( Vec_Ptr_t * vTokens, char * pInput, char * pOutput )
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
static void Io_MvSplitIntoTokens( Vec_Ptr_t * vTokens, char * pLine, char Stop )
{
    char * pCur;
    // clear spaces
    for ( pCur = pLine; *pCur != Stop; pCur++ )
        if ( Io_MvCharIsSpace(*pCur) )
            *pCur = 0;
    // collect tokens
    Io_MvCollectTokens( vTokens, pLine, pCur );
}

/**Function*************************************************************

  Synopsis    [Splits the line into tokens when .default may be present.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Io_MvSplitIntoTokensMv( Vec_Ptr_t * vTokens, char * pLine )
{
    char * pCur;
    // clear spaces
    for ( pCur = pLine; *pCur != '.' || *(pCur+1) == 'd'; pCur++ )
        if ( Io_MvCharIsSpace(*pCur) )
            *pCur = 0;
    // collect tokens
    Io_MvCollectTokens( vTokens, pLine, pCur );
}

/**Function*************************************************************

  Synopsis    [Splits the line into tokens.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Io_MvSplitIntoTokensAndClear( Vec_Ptr_t * vTokens, char * pLine, char Stop, char Char )
{
    char * pCur;
    // clear spaces
    for ( pCur = pLine; *pCur != Stop; pCur++ )
        if ( Io_MvCharIsSpace(*pCur) || *pCur == Char )
            *pCur = 0;
    // collect tokens
    Io_MvCollectTokens( vTokens, pLine, pCur );
}

/**Function*************************************************************

  Synopsis    [Returns the 1-based number of the line in which the token occurs.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvGetLine( Io_MvMan_t * p, char * pToken )
{
    char * pLine;
    int i;
    Vec_PtrForEachEntry( char *, p->vLines, pLine, i )
        if ( pToken < pLine )
            return i;
    return -1;
}

/**Function*************************************************************

  Synopsis    [Reads the file into a character buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
typedef struct buflist {
  char buf[1<<20];
  int nBuf;
  struct buflist * next;
} buflist;

char * Io_MvLoadFileBz2( char * pFileName, long * pnFileSize )
{
    FILE    * pFile;
    long       nFileSize = 0;
    char    * pContents;
    BZFILE  * b;
    int       bzError, RetValue;
    struct buflist * pNext;
    buflist * bufHead = NULL, * buf = NULL;

    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        Abc_Print( -1, "Io_MvLoadFileBz2(): The file is unavailable (absent or open).\n" );
        return NULL;
    }
    b = BZ2_bzReadOpen(&bzError,pFile,0,0,NULL,0);
    if (bzError != BZ_OK) {
        Abc_Print( -1, "Io_MvLoadFileBz2(): BZ2_bzReadOpen() failed with error %d.\n",bzError );
        return NULL;
    }
    do {
        if (!bufHead)
            buf = bufHead = ABC_ALLOC( buflist, 1 );
        else
            buf = buf->next = ABC_ALLOC( buflist, 1 );
        nFileSize += buf->nBuf = BZ2_bzRead(&bzError,b,buf->buf,1<<20);
        buf->next = NULL;
    } while (bzError == BZ_OK);
    if (bzError == BZ_STREAM_END) {
        // we're okay
        char * p;
        int nBytes = 0;
        BZ2_bzReadClose(&bzError,b);
        p = pContents = ABC_ALLOC( char, nFileSize + 10 );
        buf = bufHead;
        do {
            memcpy(p+nBytes,buf->buf,(size_t)buf->nBuf);
            nBytes += buf->nBuf;
//        } while((buf = buf->next));
            pNext = buf->next;
            ABC_FREE( buf );
        } while((buf = pNext));
    } else if (bzError == BZ_DATA_ERROR_MAGIC) {
        // not a BZIP2 file
        BZ2_bzReadClose(&bzError,b);
        fseek( pFile, 0, SEEK_END );
        nFileSize = ftell( pFile );
        if ( nFileSize == 0 )
        {
            Abc_Print( -1, "Io_MvLoadFileBz2(): The file is empty.\n" );
            return NULL;
        }
        pContents = ABC_ALLOC( char, nFileSize + 10 );
        rewind( pFile );
        RetValue = fread( pContents, nFileSize, 1, pFile );
    } else { 
        // Some other error.
        Abc_Print( -1, "Io_MvLoadFileBz2(): Unable to read the compressed BLIF.\n" );
        return NULL;
    }
    fclose( pFile );
    // finish off the file with the spare .end line
    // some benchmarks suddenly break off without this line
    strcpy( pContents + nFileSize, "\n.end\n" );
    *pnFileSize = nFileSize;
    return pContents;
}

/**Function*************************************************************

  Synopsis    [Reads the file into a character buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static char * Io_MvLoadFileGz( char * pFileName, long * pnFileSize )
{
    const int READ_BLOCK_SIZE = 100000;
    gzFile pFile;
    char * pContents;
    long amtRead, readBlock, nFileSize = READ_BLOCK_SIZE;
    pFile = gzopen( pFileName, "rb" ); // if pFileName doesn't end in ".gz" then this acts as a passthrough to fopen
    pContents = ABC_ALLOC( char, nFileSize );        
    readBlock = 0;
    while ((amtRead = gzread(pFile, pContents + readBlock * READ_BLOCK_SIZE, READ_BLOCK_SIZE)) == READ_BLOCK_SIZE) {
        //Abc_Print( 1,"%d: read %d bytes\n", readBlock, amtRead);
        nFileSize += READ_BLOCK_SIZE;
        pContents = ABC_REALLOC(char, pContents, nFileSize);
        ++readBlock;
    }
    //Abc_Print( 1,"%d: read %d bytes\n", readBlock, amtRead);
    assert( amtRead != -1 ); // indicates a zlib error
    nFileSize -= (READ_BLOCK_SIZE - amtRead);
    gzclose(pFile);
    *pnFileSize = nFileSize;
    return pContents;
}

/**Function*************************************************************

  Synopsis    [Reads the file into a character buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static char * Io_MvLoadFile( char * pFileName )
{
    FILE * pFile;
    long nFileSize;
    char * pContents;
    int RetValue;
    if ( !strncmp(pFileName+strlen(pFileName)-4,".bz2",4) )
        return Io_MvLoadFileBz2( pFileName, &nFileSize );
    if ( !strncmp(pFileName+strlen(pFileName)-3,".gz",3) )
        return Io_MvLoadFileGz( pFileName, &nFileSize );
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Io_MvLoadFile(): The file is unavailable (absent or open).\n" );
        return NULL;
    }
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile ); 
    if ( nFileSize == 0 )
    {
        fclose( pFile );
        printf( "Io_MvLoadFile(): The file is empty.\n" );
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
static void Io_MvReadPreparse( Io_MvMan_t * p )
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
//            if ( *(pCur-1) == '\r' )
//                *(pCur-1) = 0;
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
            if ( !Io_MvCharIsSpace(*pPrev) )
                break;
        // if it is the line extender, overwrite it with spaces
        if ( pPrev >= p->pBuffer && *pPrev == '\\' )
        {
            for ( ; *pPrev; pPrev++ )
                *pPrev = ' ';
            *pPrev = ' ';
            continue;
        }
        // skip spaces at the beginning of the line
        while ( Io_MvCharIsSpace(*pCur++) );
        // parse directives
        if ( *(pCur-1) != '.' )
            continue;
        if ( !strncmp(pCur, "names", 5) || !strncmp(pCur, "table", 5) || !strncmp(pCur, "gate", 4) )
            Vec_PtrPush( p->pLatest->vNames, pCur );
        else if ( p->fBlifMv && (!strncmp(pCur, "def ", 4) || !strncmp(pCur, "default ", 8)) )
            continue;
        else if ( !strncmp( pCur, "ltlformula", 10 ) )
            Vec_PtrPush( p->pLatest->vLtlProperties, pCur );
        else if ( !strncmp(pCur, "latch", 5) )
            Vec_PtrPush( p->pLatest->vLatches, pCur );
        else if ( !strncmp(pCur, "flop", 4) )
            Vec_PtrPush( p->pLatest->vFlops, pCur );
        else if ( !strncmp(pCur, "r ", 2) || !strncmp(pCur, "reset ", 6) )
            Vec_PtrPush( p->pLatest->vResets, pCur );
        else if ( !strncmp(pCur, "inputs", 6) )
            Vec_PtrPush( p->pLatest->vInputs, pCur );
        else if ( !strncmp(pCur, "outputs", 7) )
            Vec_PtrPush( p->pLatest->vOutputs, pCur );
        else if ( !strncmp(pCur, "subckt", 6) )
            Vec_PtrPush( p->pLatest->vSubckts, pCur );
        else if ( !strncmp(pCur, "short", 5) )
            Vec_PtrPush( p->pLatest->vShorts, pCur );
        else if ( !strncmp(pCur, "onehot", 6) )
            Vec_PtrPush( p->pLatest->vOnehots, pCur );
        else if ( p->fBlifMv && !strncmp(pCur, "mv", 2) )
            Vec_PtrPush( p->pLatest->vMvs, pCur );
        else if ( !strncmp(pCur, "constraint", 10) )
            Vec_PtrPush( p->pLatest->vConstrs, pCur );
        else if ( !strncmp(pCur, "blackbox", 8) )
            p->pLatest->fBlackBox = 1;
        else if ( !strncmp(pCur, "model", 5) ) 
        {
            p->pLatest = Io_MvModAlloc();
            p->pLatest->pName = pCur;
            p->pLatest->pMan = p;
        }
        else if ( !strncmp(pCur, "end", 3) )
        {
            if ( p->pLatest )
                Vec_PtrPush( p->vModels, p->pLatest );
            p->pLatest = NULL;
        }
        else if ( !strncmp(pCur, "exdc", 4) )
        {
//            fprintf( stdout, "Line %d: The design contains EXDC network (warning only).\n", Io_MvGetLine(p, pCur) );
            fprintf( stdout, "Warning: The design contains EXDC network.\n" );
            if ( p->pLatest )
                Vec_PtrPush( p->vModels, p->pLatest );
            p->pLatest = Io_MvModAlloc();
            p->pLatest->pName = NULL;
            p->pLatest->pMan = p;
        }
        else if ( !strncmp(pCur, "attrib", 6) )
        {}
        else if ( !strncmp(pCur, "delay", 5) )
        {}
        else if ( !strncmp(pCur, "input_", 6) )
        {}
        else if ( !strncmp(pCur, "output_", 7) )
        {}
        else if ( !strncmp(pCur, "no_merge", 8) )
        {}
        else if ( !strncmp(pCur, "wd", 2) )
        {}
//        else if ( !strncmp(pCur, "inouts", 6) )
//        {}
        else
        {
            pCur--;
            if ( pCur[strlen(pCur)-1] == '\r' )
                pCur[strlen(pCur)-1] = 0;
            fprintf( stdout, "Line %d: Skipping line \"%s\".\n", Io_MvGetLine(p, pCur), pCur );
        }
    }
}

/**Function*************************************************************

  Synopsis    [Parses interfaces of the models.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvReadInterfaces( Io_MvMan_t * p )
{
    Io_MvMod_t * pMod;
    char * pLine;
    int i, k, nOutsOld;
    // iterate through the models
    Vec_PtrForEachEntry( Io_MvMod_t *, p->vModels, pMod, i )
    {
        // parse the model
        if ( !Io_MvParseLineModel( pMod, pMod->pName ) )
            return 0;
        // add model to the design
        if ( !Abc_DesAddModel( p->pDesign, pMod->pNtk ) )
        {
            sprintf( p->sError, "Line %d: Model %s is defined twice.", Io_MvGetLine(p, pMod->pName), pMod->pName );
            return 0;
        }
        // parse the inputs
        Vec_PtrForEachEntry( char *, pMod->vInputs, pLine, k )
            if ( !Io_MvParseLineInputs( pMod, pLine ) )
                return 0;
        // parse the outputs
        Vec_PtrForEachEntry( char *, pMod->vOutputs, pLine, k )
            if ( !Io_MvParseLineOutputs( pMod, pLine ) )
                return 0;
        // parse the constraints
        nOutsOld = Abc_NtkPoNum(pMod->pNtk);
        Vec_PtrForEachEntry( char *, pMod->vConstrs, pLine, k )
            if ( !Io_MvParseLineConstrs( pMod, pLine ) )
                return 0;
        pMod->pNtk->nConstrs = Abc_NtkPoNum(pMod->pNtk) - nOutsOld;
        Vec_PtrForEachEntry( char *, pMod->vLtlProperties, pLine, k )
            if ( !Io_MvParseLineLtlProperty( pMod, pLine ) )
                return 0;
        // report the results
#ifdef IO_VERBOSE_OUTPUT
        if ( Vec_PtrSize(p->vModels) > 1 )
            printf( "Parsed %-32s: PI =%6d  PO =%6d  ND =%8d  FF =%6d  B =%6d\n", 
                pMod->pNtk->pName, Abc_NtkPiNum(pMod->pNtk), Abc_NtkPoNum(pMod->pNtk),
                Vec_PtrSize(pMod->vNames), Vec_PtrSize(pMod->vLatches), Vec_PtrSize(pMod->vSubckts) );
#endif
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Abc_Des_t * Io_MvParse( Io_MvMan_t * p )
{
    Abc_Des_t * pDesign;
    Io_MvMod_t * pMod;
    char * pLine;
    int i, k;
    // iterate through the models
    Vec_PtrForEachEntry( Io_MvMod_t *, p->vModels, pMod, i )
    { 
#ifdef IO_VERBOSE_OUTPUT
        if ( Vec_PtrSize(p->vModels) > 1 )
            printf( "Parsing model %s...\n", pMod->pNtk->pName );
#endif

        // check if there any MV lines
        if ( Vec_PtrSize(pMod->vMvs) > 0 )
            Abc_NtkStartMvVars( pMod->pNtk );
        // parse the mv lines
        Vec_PtrForEachEntry( char *, pMod->vMvs, pLine, k )
            if ( !Io_MvParseLineMv( pMod, pLine ) )
                return NULL;
        // if reset lines are used there should be the same number of them as latches
        if ( Vec_PtrSize(pMod->vResets) > 0 )
        {
            if ( Vec_PtrSize(pMod->vLatches) != Vec_PtrSize(pMod->vResets) )
            {
                sprintf( p->sError, "Line %d: Model %s has different number of latches (%d) and reset nodes (%d).", 
                    Io_MvGetLine(p, pMod->pName), Abc_NtkName(pMod->pNtk), Vec_PtrSize(pMod->vLatches), Vec_PtrSize(pMod->vResets) );
                return NULL;
            }
            // create binary latch with 1-data and 0-init
            if ( p->fUseReset ) 
                pMod->pResetLatch = Io_ReadCreateResetLatch( pMod->pNtk, p->fBlifMv );
        }
        // parse the flops
        Vec_PtrForEachEntry( char *, pMod->vFlops, pLine, k )
            if ( !Io_MvParseLineFlop( pMod, pLine ) )
                return NULL;
        // parse the latches
        Vec_PtrForEachEntry( char *, pMod->vLatches, pLine, k )
            if ( !Io_MvParseLineLatch( pMod, pLine ) )
                return NULL;
        // parse the reset lines
        if ( p->fUseReset )
            Vec_PtrForEachEntry( char *, pMod->vResets, pLine, k )
                if ( !Io_MvParseLineNamesMv( pMod, pLine, 1 ) )
                    return NULL;
        // parse the nodes
        if ( p->fBlifMv )
        {
            Vec_PtrForEachEntry( char *, pMod->vNames, pLine, k )
                if ( !Io_MvParseLineNamesMv( pMod, pLine, 0 ) )
                    return NULL;
        }
        else
        {
            Vec_PtrForEachEntry( char *, pMod->vNames, pLine, k )
                if ( !Io_MvParseLineNamesBlif( pMod, pLine ) )
                    return NULL;
            Vec_PtrForEachEntry( char *, pMod->vShorts, pLine, k )
                if ( !Io_MvParseLineShortBlif( pMod, pLine ) )
                    return NULL;
        }
        // parse the subcircuits
        Vec_PtrForEachEntry( char *, pMod->vSubckts, pLine, k )
            if ( !Io_MvParseLineSubckt( pMod, pLine ) )
                return NULL;

        // allow for blackboxes without .blackbox line
        if ( Abc_NtkLatchNum(pMod->pNtk) == 0 && Abc_NtkNodeNum(pMod->pNtk) == 0 && Abc_NtkBoxNum(pMod->pNtk) == 0 )
        {
            if ( pMod->pNtk->ntkFunc == ABC_FUNC_SOP )
            {
                Mem_FlexStop( (Mem_Flex_t *)pMod->pNtk->pManFunc, 0 );
                pMod->pNtk->pManFunc = NULL;
                pMod->pNtk->ntkFunc = ABC_FUNC_BLACKBOX;
            }
        }

        // finalize the network
        Abc_NtkFinalizeRead( pMod->pNtk );
        // read the one-hotness lines
        if ( Vec_PtrSize(pMod->vOnehots) > 0 )
        {
            Vec_Int_t * vLine; 
            Abc_Obj_t * pObj;
            // set register numbers
            Abc_NtkForEachLatch( pMod->pNtk, pObj, k )
                pObj->pNext = (Abc_Obj_t *)(ABC_PTRINT_T)k;
            // derive register
            pMod->pNtk->vOnehots = Vec_PtrAlloc( Vec_PtrSize(pMod->vOnehots) );
            Vec_PtrForEachEntry( char *, pMod->vOnehots, pLine, k )
            {
                vLine = Io_MvParseLineOnehot( pMod, pLine );
                if ( vLine == NULL )
                    return NULL;
                Vec_PtrPush( pMod->pNtk->vOnehots, vLine );
//                printf( "Parsed %d one-hot registers.\n", Vec_IntSize(vLine) );
            }
            // reset register numbers
            Abc_NtkForEachLatch( pMod->pNtk, pObj, k )
                pObj->pNext = NULL;
            // print the result
            printf( "Parsed %d groups of 1-hot registers: { ", Vec_PtrSize(pMod->pNtk->vOnehots) );
            Vec_PtrForEachEntry( Vec_Int_t *, pMod->pNtk->vOnehots, vLine, k )
                printf( "%d ", Vec_IntSize(vLine) );
            printf( "}\n" );
            printf( "The total number of 1-hot registers = %d. (%.2f %%)\n", 
                Vec_VecSizeSize( (Vec_Vec_t *)pMod->pNtk->vOnehots ), 
                100.0 * Vec_VecSizeSize( (Vec_Vec_t *)pMod->pNtk->vOnehots ) / Abc_NtkLatchNum(pMod->pNtk) );
            {
                extern void Abc_GenOneHotIntervals( char * pFileName, int nPis, int nRegs, Vec_Ptr_t * vOnehots );
                char * pFileName = Extra_FileNameGenericAppend( pMod->pMan->pFileName, "_1h.blif" );
                Abc_GenOneHotIntervals( pFileName, Abc_NtkPiNum(pMod->pNtk), Abc_NtkLatchNum(pMod->pNtk), pMod->pNtk->vOnehots );
                printf( "One-hotness condition is written into file \"%s\".\n", pFileName );
            }
        }
        if ( Vec_PtrSize(pMod->vFlops) )
        {
            printf( "Warning: The parser converted %d .flop lines into .latch lines\n", Vec_PtrSize(pMod->vFlops) );
            printf( "(information about set, reset, enable of the flops may be lost).\n" );
        }

    }
    if ( p->nNDnodes )
//        printf( "Warning: The parser added %d PIs to replace non-deterministic nodes.\n", p->nNDnodes );
        printf( "Warning: The parser added %d constant 0 nodes to replace non-deterministic nodes.\n", p->nNDnodes );
    // return the network
    pDesign = p->pDesign;
    p->pDesign = NULL;
    return pDesign;
}

/**Function*************************************************************

  Synopsis    [Parses the model line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineModel( Io_MvMod_t * p, char * pLine )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
    char * pToken, * pPivot;
    if ( pLine == NULL )
    {
        p->pNtk = Abc_NtkAlloc( ABC_NTK_NETLIST, ABC_FUNC_SOP, 1 );
        p->pNtk->pName = Extra_UtilStrsav( "EXDC" );
        return 1;
    }
    Io_MvSplitIntoTokens( vTokens, pLine, '\0' );
    pToken = (char *)Vec_PtrEntry( vTokens, 0 );
    assert( !strcmp(pToken, "model") );
    if ( Vec_PtrSize(vTokens) != 2 )
    {
        sprintf( p->pMan->sError, "Line %d: Model line has %d entries while it should have 2.", Io_MvGetLine(p->pMan, pToken), Vec_PtrSize(vTokens) );
        return 0;
    }
    if ( p->fBlackBox )
        p->pNtk = Abc_NtkAlloc( ABC_NTK_NETLIST, ABC_FUNC_BLACKBOX, 1 );
    else if ( p->pMan->fBlifMv )
        p->pNtk = Abc_NtkAlloc( ABC_NTK_NETLIST, ABC_FUNC_BLIFMV, 1 );
    else 
        p->pNtk = Abc_NtkAlloc( ABC_NTK_NETLIST, ABC_FUNC_SOP, 1 );
//    for ( pPivot = pToken = Vec_PtrEntry(vTokens, 1); *pToken; pToken++ )
//        if ( *pToken == '/' || *pToken == '\\' )
//            pPivot = pToken+1;
    pPivot = pToken = (char *)Vec_PtrEntry(vTokens, 1);
    p->pNtk->pName = Extra_UtilStrsav( pPivot );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the inputs line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineInputs( Io_MvMod_t * p, char * pLine )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
    char * pToken;
    int i;
    Io_MvSplitIntoTokens( vTokens, pLine, '\0' );
    pToken = (char *)Vec_PtrEntry(vTokens, 0);
    assert( !strcmp(pToken, "inputs") );
    Vec_PtrForEachEntryStart( char *, vTokens, pToken, i, 1 )
        Io_ReadCreatePi( p->pNtk, pToken );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the outputs line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineOutputs( Io_MvMod_t * p, char * pLine )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
    char * pToken;
    int i;
    Io_MvSplitIntoTokens( vTokens, pLine, '\0' );
    pToken = (char *)Vec_PtrEntry(vTokens, 0);
    assert( !strcmp(pToken, "outputs") );
    Vec_PtrForEachEntryStart( char *, vTokens, pToken, i, 1 )
        Io_ReadCreatePo( p->pNtk, pToken );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the outputs line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineConstrs( Io_MvMod_t * p, char * pLine )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
    char * pToken;
    int i;
    Io_MvSplitIntoTokens( vTokens, pLine, '\0' );
    pToken = (char *)Vec_PtrEntry(vTokens, 0);
    assert( !strcmp(pToken, "constraint") );
    Vec_PtrForEachEntryStart( char *, vTokens, pToken, i, 1 )
        Io_ReadCreatePo( p->pNtk, pToken );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the LTL property line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineLtlProperty( Io_MvMod_t * p, char * pLine )
{
    int i, j;
    int quoteBegin, quoteEnd;
    char keyWordLtlFormula[11];
    char *actualLtlFormula;

    //checking if the line begins with the keyword "ltlformula" and
    //progressing the pointer forword
    for( i=0; i<10; i++ )
        keyWordLtlFormula[i] = pLine[i];
    quoteBegin = i;
    keyWordLtlFormula[10] = '\0';
    assert( strcmp( "ltlformula", keyWordLtlFormula ) == 0 );
    while( pLine[i] != '"' )
        i++;
    quoteBegin = i;
    i = strlen( pLine );
    while( pLine[i] != '"' )
        i--;
    quoteEnd = i;
    actualLtlFormula = (char *)malloc( sizeof(char) * (quoteEnd - quoteBegin) );
    //printf("\nThe input ltl formula = ");
    for( i = quoteBegin + 1, j = 0; i<quoteEnd; i++, j++ )
        //printf("%c", pLine[i] );
        actualLtlFormula[j] = pLine[i];
    actualLtlFormula[j] = '\0';
    Vec_PtrPush( vGlobalLtlArray, actualLtlFormula );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Parses the latches line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineLatch( Io_MvMod_t * p, char * pLine )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
    Abc_Obj_t * pObj, * pNet;
    char * pToken;
    int Init;
    Io_MvSplitIntoTokens( vTokens, pLine, '\0' );
    pToken = (char *)Vec_PtrEntry(vTokens,0);
    assert( !strcmp(pToken, "latch") );
    if ( Vec_PtrSize(vTokens) < 3 )
    {
        sprintf( p->pMan->sError, "Line %d: Latch does not have input name and output name.", Io_MvGetLine(p->pMan, pToken) );
        return 0;
    }
    // create latch
    if ( p->pResetLatch == NULL )
    {
        pObj = Io_ReadCreateLatch( p->pNtk, (char *)Vec_PtrEntry(vTokens,1), (char *)Vec_PtrEntry(vTokens,2) );
        // get initial value
        if ( p->pMan->fBlifMv )
            Abc_LatchSetInit0( pObj );
        else
        {
            if ( Vec_PtrSize(vTokens) > 6 )
                printf( "Warning: Line %d has .latch directive with unrecognized entries (the total of %d entries).\n", 
                    Io_MvGetLine(p->pMan, pToken), Vec_PtrSize(vTokens) ); 
            if ( Vec_PtrSize(vTokens) > 3 )
                Init = atoi( (char *)Vec_PtrEntryLast(vTokens) );
            else
                Init = 2;
            if ( Init < 0 || Init > 3 )
            {
                sprintf( p->pMan->sError, "Line %d: Initial state of the latch is incorrect \"%s\".", Io_MvGetLine(p->pMan, pToken), (char*)Vec_PtrEntry(vTokens,3) );
                return 0;
            }
            if ( Init == 0 )
                Abc_LatchSetInit0( pObj );
            else if ( Init == 1 )
                Abc_LatchSetInit1( pObj );
            else // if ( Init == 2 )
                Abc_LatchSetInitDc( pObj );
        }
    }
    else
    {
        // get the net corresponding to the output of the latch
        pNet = Abc_NtkFindOrCreateNet( p->pNtk, (char *)Vec_PtrEntry(vTokens,2) );
        // get the net corresponding to the latch output (feeding into reset MUX)
        pNet = Abc_NtkFindOrCreateNet( p->pNtk, Abc_ObjNameSuffix(pNet, "_out") );
        // create latch
        pObj = Io_ReadCreateLatch( p->pNtk, (char *)Vec_PtrEntry(vTokens,1), Abc_ObjName(pNet) );
//        Abc_LatchSetInit0( pObj );
        Abc_LatchSetInit0( pObj );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the latches line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineFlop( Io_MvMod_t * p, char * pLine )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
    Abc_Obj_t * pObj;
    char * pToken, * pOutput, * pInput;
    int i, Init = 2;
    assert( !p->pMan->fBlifMv );
    Io_MvSplitIntoTokens( vTokens, pLine, '\0' );
    pToken = (char *)Vec_PtrEntry(vTokens,0);
    assert( !strcmp(pToken, "flop") );
    // get flop output
    Vec_PtrForEachEntry( char *, vTokens, pToken, i )
        if ( pToken[0] == 'Q' && pToken[1] == '=' )
            break;
    if ( i == Vec_PtrSize(vTokens) )
    {
        sprintf( p->pMan->sError, "Line %d: Cannot find flop output.", Io_MvGetLine(p->pMan, (char *)Vec_PtrEntry(vTokens,0)) );
        return 0;
    }
    pOutput = pToken+2;
    // get flop input
    Vec_PtrForEachEntry( char *, vTokens, pToken, i )
        if ( pToken[0] == 'D' && pToken[1] == '=' )
            break;
    if ( i == Vec_PtrSize(vTokens) )
    {
        sprintf( p->pMan->sError, "Line %d: Cannot find flop input.", Io_MvGetLine(p->pMan, (char *)Vec_PtrEntry(vTokens,0)) );
        return 0;
    }
    pInput = pToken+2;
    // create latch
    pObj = Io_ReadCreateLatch( p->pNtk, pInput, pOutput );
    // get the init value
    Vec_PtrForEachEntry( char *, vTokens, pToken, i )
    {
        if ( !strncmp( pToken, "init=", 5 ) )
        {
            Init = 0;
            if ( pToken[5] == '1' )
                Init = 1;
            else if ( pToken[5] == '2' )
                Init = 2;
            else if ( pToken[5] != '0' )
            {
                sprintf( p->pMan->sError, "Line %d: Cannot read flop init value %s.", Io_MvGetLine(p->pMan, pToken), pToken );
                return 0;
            }
            break;
        }
    }
    if ( Init < 0 || Init > 2 )
    {
        sprintf( p->pMan->sError, "Line %d: Initial state of the flop is incorrect \"%s\".", Io_MvGetLine(p->pMan, pToken), (char*)Vec_PtrEntry(vTokens,3) );
        return 0;
    }
    if ( Init == 0 )
        Abc_LatchSetInit0( pObj );
    else if ( Init == 1 )
        Abc_LatchSetInit1( pObj );
    else // if ( Init == 2 )
        Abc_LatchSetInitDc( pObj );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the subckt line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineSubckt( Io_MvMod_t * p, char * pLine )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
    Abc_Ntk_t * pModel;
    Abc_Obj_t * pBox, * pNet, * pTerm;
    char * pToken, * pName, * pName2, ** ppNames;
    int nEquals, i, k;
    word Last;

    // split the line into tokens
    nEquals = Io_MvCountChars( pLine, '=' );
    Io_MvSplitIntoTokensAndClear( vTokens, pLine, '\0', '=' );
    pToken = (char *)Vec_PtrEntry(vTokens,0);
    assert( !strcmp(pToken, "subckt") );
//printf( "%d ", nEquals );

    // get the model for this box
    pName = (char *)Vec_PtrEntry(vTokens,1);
    // skip instance name for now
    for ( pToken = pName; *pToken; pToken++ )
        if ( *pToken == '|' )
        {
            *pToken = 0;
            break;
        }
    // find the model
    pModel = Abc_DesFindModelByName( p->pMan->pDesign, pName );
    if ( pModel == NULL )
    {
        sprintf( p->pMan->sError, "Line %d: Cannot find the model for subcircuit %s.", Io_MvGetLine(p->pMan, pToken), pName );
        return 0;
    }
/*
    // check if the number of tokens is correct
    if ( nEquals != Abc_NtkPiNum(pModel) + Abc_NtkPoNum(pModel) )
    {
        sprintf( p->pMan->sError, "Line %d: The number of ports (%d) in .subckt differs from the sum of PIs and POs of the model (%d).", 
            Io_MvGetLine(p->pMan, pToken), nEquals, Abc_NtkPiNum(pModel) + Abc_NtkPoNum(pModel) );
        return 0;
    }
*/
    // get the names
    ppNames = (char **)Vec_PtrArray(vTokens) + 2 + p->pMan->fBlifMv;

    // create the box with these terminals
    if ( Abc_NtkHasBlackbox(pModel) )
        pBox = Abc_NtkCreateBlackbox( p->pNtk );
    else
        pBox = Abc_NtkCreateWhitebox( p->pNtk );
    pBox->pData = pModel;
    if ( p->pMan->fBlifMv )
        Abc_ObjAssignName( pBox, (char *)Vec_PtrEntry(vTokens,2), NULL );
    // go through formal inputs
    Last = 0;
    Abc_NtkForEachPi( pModel, pTerm, i )
    { 
        // find this terminal among the actual inputs of the subcircuit
        pName2 = NULL;
        pName = Abc_ObjName(Abc_ObjFanout0(pTerm));
        for ( k = 0; k < nEquals; k++ )
            if ( !strcmp( ppNames[2*(int)((k+Last)%nEquals)], pName ) )
            {
                pName2 = ppNames[2*(int)((k+Last)%nEquals)+1];
                Last = k+Last+1;
                break;
            }
/*
        if ( k == nEquals )
        {
            sprintf( p->pMan->sError, "Line %d: Cannot find PI \"%s\" of the model \"%s\" as a formal input of the subcircuit.", 
                Io_MvGetLine(p->pMan, pToken), pName, Abc_NtkName(pModel) );
            return 0;
        }
*/
        if ( pName2 == NULL )
        {
            Abc_Obj_t * pNode = Abc_NtkCreateNodeConst0( p->pNtk );
            //pNode->pData = Abc_SopRegister( (Mem_Flex_t *)p->pNtk->pManFunc, " 0\n" );
            pNet = Abc_NtkFindOrCreateNet( p->pNtk, Abc_ObjNameSuffix(pNode, "abc") );
            Abc_ObjAddFanin( pNet, pNode );
            pTerm = Abc_NtkCreateBi( p->pNtk );
            Abc_ObjAddFanin( pBox, pTerm );
            Abc_ObjAddFanin( pTerm, pNet );
            continue;
        }
        assert( pName2 != NULL );
  
        // create the BI with the actual name
        pNet = Abc_NtkFindOrCreateNet( p->pNtk, pName2 );
        pTerm = Abc_NtkCreateBi( p->pNtk );
        Abc_ObjAddFanin( pBox, pTerm );
        Abc_ObjAddFanin( pTerm, pNet );
    }
    // go through formal outputs
    Last = 0;
    Abc_NtkForEachPo( pModel, pTerm, i )
    {
        // find this terminal among the actual outputs of the subcircuit
        pName2 = NULL;
        pName = Abc_ObjName(Abc_ObjFanin0(pTerm));
        for ( k = 0; k < nEquals; k++ )
            if ( !strcmp( ppNames[2*((k+Last)%nEquals)], pName ) )
            {
                pName2 = ppNames[2*((k+Last)%nEquals)+1];
                Last = k+Last+1;
                break;
            }
/*
        if ( k == nEquals )
        {
            sprintf( p->pMan->sError, "Line %d: Cannot find PO \"%s\" of the modell \"%s\" as a formal output of the subcircuit.", 
                Io_MvGetLine(p->pMan, pToken), pName, Abc_NtkName(pModel) );
            return 0;
        }
*/

        // create the BI with the actual name
        pTerm = Abc_NtkCreateBo( p->pNtk );
        pNet = Abc_NtkFindOrCreateNet( p->pNtk, pName2 == NULL  ? Abc_ObjNameSuffix(pTerm, "abc") : pName2 );
        Abc_ObjAddFanin( pNet, pTerm );
        Abc_ObjAddFanin( pTerm, pBox );
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the subckt line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Vec_Int_t * Io_MvParseLineOnehot( Io_MvMod_t * p, char * pLine )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
//    Vec_Ptr_t * vResult;
    Vec_Int_t * vResult;
    Abc_Obj_t * pNet, * pTerm;
    char * pToken;
    int nEquals, i;

    // split the line into tokens
    nEquals = Io_MvCountChars( pLine, '=' );
    Io_MvSplitIntoTokensAndClear( vTokens, pLine, '\0', '=' );
    pToken = (char *)Vec_PtrEntry(vTokens,0);
    assert( !strcmp(pToken, "onehot") );

    // iterate through the register names
//    vResult = Vec_PtrAlloc( Vec_PtrSize(vTokens) );
    vResult = Vec_IntAlloc( Vec_PtrSize(vTokens) );
    Vec_PtrForEachEntryStart( char *, vTokens, pToken, i, 1 )
    {
        // check if this register exists
        pNet = Abc_NtkFindNet( p->pNtk, pToken );
        if ( pNet == NULL )
        {
            sprintf( p->pMan->sError, "Line %d: Signal with name \"%s\" does not exist in the model \"%s\".", 
                Io_MvGetLine(p->pMan, pToken), pToken, Abc_NtkName(p->pNtk) );
            return NULL;
        }
        // check if this is register output net
        pTerm = Abc_ObjFanin0( pNet );
        if ( pTerm == NULL || Abc_ObjFanin0(pTerm) == NULL || !Abc_ObjIsLatch(Abc_ObjFanin0(pTerm)) )
        {
            sprintf( p->pMan->sError, "Line %d: Signal with name \"%s\" is not a register in the model \"%s\".", 
                Io_MvGetLine(p->pMan, pToken), pToken, Abc_NtkName(p->pNtk) );
            return NULL;
        }
        // save register name
//        Vec_PtrPush( vResult, Abc_ObjName(pNet) );
        Vec_IntPush( vResult, (int)(ABC_PTRINT_T)Abc_ObjFanin0(pTerm)->pNext );
//        printf( "%d(%d) ", (int)Abc_ObjFanin0(pTerm)->pNext, ((int)Abc_ObjFanin0(pTerm)->pData) -1 );
        printf( "%d", ((int)(ABC_PTRINT_T)Abc_ObjFanin0(pTerm)->pData)-1 );
    }
    printf( "\n" );
    return vResult;
}


/**Function*************************************************************

  Synopsis    [Parses the mv line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineMv( Io_MvMod_t * p, char * pLine )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
    Abc_Obj_t * pObj;
    Io_MvVar_t * pVar = NULL;
    Mem_Flex_t * pFlex;
    char * pName;
    int nCommas, nValues, i, k;
    // count commas and get the tokens
    nCommas = Io_MvCountChars( pLine, ',' );
    Io_MvSplitIntoTokensAndClear( vTokens, pLine, '\0', ',' );
    pName = (char *)Vec_PtrEntry(vTokens,0);
    assert( !strcmp(pName, "mv") );
    // get the number of values
    if ( Vec_PtrSize(vTokens) <= nCommas + 2 )
    {
        sprintf( p->pMan->sError, "Line %d: The number of values in not specified in .mv line.", Io_MvGetLine(p->pMan, pName) );
        return 0;
    }
    nValues = atoi( (char *)Vec_PtrEntry(vTokens,nCommas+2) );
    if ( nValues < 2 || nValues > IO_BLIFMV_MAXVALUES )
    {
        sprintf( p->pMan->sError, "Line %d: The number of values (%d) is incorrect (should be >= 2 and <= %d).", 
            Io_MvGetLine(p->pMan, pName), nValues, IO_BLIFMV_MAXVALUES );
        return 0;
    }
    // if there is no symbolic values, quit
    if ( nValues == 2 && Vec_PtrSize(vTokens) == nCommas + 3 )
        return 1;
    if ( Vec_PtrSize(vTokens) > nCommas + 3 && Vec_PtrSize(vTokens) - (nCommas + 3) != nValues )
    {
        sprintf( p->pMan->sError, "Line %d: Wrong number (%d) of symbolic value names (should be %d).", 
            Io_MvGetLine(p->pMan, pName), Vec_PtrSize(vTokens) - (nCommas + 3), nValues );
        return 0;
    }
    // go through variables
    pFlex = (Mem_Flex_t *)Abc_NtkMvVarMan( p->pNtk );
    for ( i = 0; i <= nCommas; i++ )
    {
        pName = (char *)Vec_PtrEntry( vTokens, i+1 );
        pObj = Abc_NtkFindOrCreateNet( p->pNtk, pName );
        // allocate variable
        pVar = (Io_MvVar_t *)Mem_FlexEntryFetch( pFlex, sizeof(Io_MvVar_t) );
        pVar->nValues = nValues;
        pVar->pNames = NULL;
        // create names
        if ( Vec_PtrSize(vTokens) > nCommas + 3 )
        {
            pVar->pNames = (char **)Mem_FlexEntryFetch( pFlex, sizeof(char *) * nValues );
            Vec_PtrForEachEntryStart( char *, vTokens, pName, k, nCommas + 3 )
            {
                pVar->pNames[k-(nCommas + 3)] = (char *)Mem_FlexEntryFetch( pFlex, strlen(pName) + 1 );
                strcpy( pVar->pNames[k-(nCommas + 3)], pName );
            }
        }
        // save the variable
        Abc_ObjSetMvVar( pObj, pVar );
    }
    // make sure the names are unique
    assert(pVar);
    if ( pVar->pNames )
    {
        for ( i = 0; i < nValues; i++ )
        for ( k = i+1; k < nValues; k++ )
            if ( !strcmp(pVar->pNames[i], pVar->pNames[k]) )
            {
                pName = (char *)Vec_PtrEntry(vTokens,0);
                sprintf( p->pMan->sError, "Line %d: Symbolic value name \"%s\" is repeated in .mv line.", 
                    Io_MvGetLine(p->pMan, pName), pVar->pNames[i] );
                return 0;
            }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes the values into the BLIF-MV representation for the node.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvWriteValues( Abc_Obj_t * pNode, Vec_Str_t * vFunc )
{
    char Buffer[10];
    Abc_Obj_t * pFanin;
    int i;
    // add the fanin number of values
    Abc_ObjForEachFanin( pNode, pFanin, i )
    {
        sprintf( Buffer, "%d", Abc_ObjMvVarNum(pFanin) );
        Vec_StrPrintStr( vFunc, Buffer );
        Vec_StrPush( vFunc, ' ' );
    }
    // add the node number of values
    sprintf( Buffer, "%d", Abc_ObjMvVarNum(Abc_ObjFanout0(pNode)) );
    Vec_StrPrintStr( vFunc, Buffer );
    Vec_StrPush( vFunc, '\n' );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Translated one literal.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLiteralMv( Io_MvMod_t * p, Abc_Obj_t * pNode, char * pToken, Vec_Str_t * vFunc, int iLit )
{
    char Buffer[16];
    Io_MvVar_t * pVar;
    Abc_Obj_t * pFanin, * pNet;
    char * pCur, * pNext;
    int i;
    // consider the equality literal
    if ( pToken[0] == '=' )
    {
        // find the fanins
        Abc_ObjForEachFanin( pNode, pFanin, i )
            if ( !strcmp( Abc_ObjName(pFanin), pToken + 1 ) )
                break;
        if ( i == Abc_ObjFaninNum(pNode) )
        {
            sprintf( p->pMan->sError, "Line %d: Node name in the table \"%s\" cannot be found on .names line.", 
                Io_MvGetLine(p->pMan, pToken), pToken + 1 );
            return 0;
        }
        Vec_StrPush( vFunc, '=' );
        sprintf( Buffer, "%d", i );
        Vec_StrPrintStr( vFunc, Buffer );
        Vec_StrPush( vFunc, (char)((iLit == -1)? '\n' : ' ') );
        return 1;
    }
    // consider regular literal
    assert( iLit < Abc_ObjFaninNum(pNode) );
    pNet = iLit >= 0 ? Abc_ObjFanin(pNode, iLit) : Abc_ObjFanout0(pNode);
    pVar = (Io_MvVar_t *)Abc_ObjMvVar( pNet );
    // if the var is absent or has no symbolic values quit
    if ( pVar == NULL || pVar->pNames == NULL )
    {
        Vec_StrPrintStr( vFunc, pToken );
        Vec_StrPush( vFunc, (char)((iLit == -1)? '\n' : ' ') );
        return 1;
    }
    // parse the literal using symbolic values
    for ( pCur = pToken; *pCur; pCur++ )
    {
        if ( Io_MvCharIsMvSymb(*pCur) )
        {
            Vec_StrPush( vFunc, *pCur );
            continue;
        }
        // find the next MvSymb char
        for ( pNext = pCur+1; *pNext; pNext++ )
            if ( Io_MvCharIsMvSymb(*pNext) )
                break;
        // look for the value name
        for ( i = 0; i < pVar->nValues; i++ )
            if ( !strncmp( pVar->pNames[i], pCur, pNext-pCur ) )
                break;
        if ( i == pVar->nValues )
        {
            *pNext = 0;
            sprintf( p->pMan->sError, "Line %d: Cannot find value name \"%s\" among the value names of variable \"%s\".", 
                Io_MvGetLine(p->pMan, pToken), pCur, Abc_ObjName(pNet) );
            return 0;
        }
        // value name is found
        sprintf( Buffer, "%d", i );
        Vec_StrPrintStr( vFunc, Buffer );
        // update the pointer
        pCur = pNext - 1;
    }
    Vec_StrPush( vFunc, (char)((iLit == -1)? '\n' : ' ') );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Constructs the MV-SOP cover from the file parsing info.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static char * Io_MvParseTableMv( Io_MvMod_t * p, Abc_Obj_t * pNode, Vec_Ptr_t * vTokens2, int nInputs, int nOutputs, int iOut )
{
    Vec_Str_t * vFunc = p->pMan->vFunc;
    char * pFirst, * pToken;
    int iStart, i;
    // prepare the place for the cover
    Vec_StrClear( vFunc );
    // write the number of values
//    Io_MvWriteValues( pNode, vFunc );
    // get the first token
    pFirst = (char *)Vec_PtrEntry( vTokens2, 0 );
    if ( pFirst[0] == '.' )
    {
        // write the default literal
        Vec_StrPush( vFunc, 'd' );
        pToken = (char *)Vec_PtrEntry(vTokens2, 1 + iOut);
        if ( !Io_MvParseLiteralMv( p, pNode, pToken, vFunc, -1 ) )
            return NULL;
        iStart = 1 + nOutputs;
    }
    else
        iStart = 0;
    // write the remaining literals
    while ( iStart < Vec_PtrSize(vTokens2) )
    {
        // input literals
        for ( i = 0; i < nInputs; i++ )
        {
            pToken = (char *)Vec_PtrEntry( vTokens2, iStart + i );
            if ( !Io_MvParseLiteralMv( p, pNode, pToken, vFunc, i ) )
                return NULL;
        }
        // output literal
        pToken = (char *)Vec_PtrEntry( vTokens2, iStart + nInputs + iOut );
        if ( !Io_MvParseLiteralMv( p, pNode, pToken, vFunc, -1 ) )
            return NULL;
        // update the counter
        iStart += nInputs + nOutputs;
    }       
    Vec_StrPush( vFunc, '\0' );
    return Vec_StrArray( vFunc );
}

/**Function*************************************************************

  Synopsis    [Adds reset circuitry corresponding to latch with pName.]

  Description [Returns the reset node's net.]
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Abc_Obj_t * Io_MvParseAddResetCircuit( Io_MvMod_t * p, char * pName )
{
    char Buffer[50];
    Abc_Obj_t * pNode, * pData0Net, * pData1Net, * pResetLONet, * pOutNet;
    Io_MvVar_t * pVar;
    // make sure the reset latch exists
    assert( p->pResetLatch != NULL );
    // get the reset net
    pResetLONet = Abc_ObjFanout0(Abc_ObjFanout0(p->pResetLatch));
    // get the output net
    pOutNet = Abc_NtkFindOrCreateNet( p->pNtk, pName );
    // get the data nets
    pData0Net = Abc_NtkFindOrCreateNet( p->pNtk, Abc_ObjNameSuffix(pOutNet, "_reset") );
    pData1Net = Abc_NtkFindOrCreateNet( p->pNtk, Abc_ObjNameSuffix(pOutNet, "_out") );
    // duplicate MV variables
    if ( Abc_NtkMvVar(p->pNtk) )
    {
        pVar = (Io_MvVar_t *)Abc_ObjMvVar( pOutNet );
        Abc_ObjSetMvVar( pData0Net, Abc_NtkMvVarDup(p->pNtk, pVar) );
        Abc_ObjSetMvVar( pData1Net, Abc_NtkMvVarDup(p->pNtk, pVar) );
    }
    // create the node
    pNode = Abc_NtkCreateNode( p->pNtk );
    // create the output net
    Abc_ObjAddFanin( pOutNet, pNode );
    // create the function
    if ( p->pMan->fBlifMv )
    {
//        Vec_Att_t * p = Abc_NtkMvVar( pNtk );
//        int nValues = Abc_ObjMvVarNum(pOutNet);
//        sprintf( Buffer, "2 %d %d %d\n1 - - =1\n0 - - =2\n", nValues, nValues, nValues );
        sprintf( Buffer, "1 - - =1\n0 - - =2\n" );
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)p->pNtk->pManFunc, Buffer );
    }
    else
        pNode->pData = Abc_SopCreateMux( (Mem_Flex_t *)p->pNtk->pManFunc );
    // add nets
    Abc_ObjAddFanin( pNode, pResetLONet );
    Abc_ObjAddFanin( pNode, pData1Net );
    Abc_ObjAddFanin( pNode, pData0Net );
    return pData0Net;
}

/**Function*************************************************************

  Synopsis    [Parses the nodes line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineNamesMvOne( Io_MvMod_t * p, Vec_Ptr_t * vTokens, Vec_Ptr_t * vTokens2, int nInputs, int nOutputs, int iOut, int fReset )
{
    Abc_Obj_t * pNet, * pNode;
    char * pName;
    // get the output name
    pName = (char *)Vec_PtrEntry( vTokens, Vec_PtrSize(vTokens) - nOutputs + iOut );
    // create the node
    if ( fReset )
    {
        pNet = Abc_NtkFindNet( p->pNtk, pName );
        if ( pNet == NULL )
        {
            sprintf( p->pMan->sError, "Line %d: Latch with output signal \"%s\" does not exist.", Io_MvGetLine(p->pMan, pName), pName );
            return 0;
        }
/*
        if ( !Abc_ObjIsBo(Abc_ObjFanin0(pNet)) )
        {
            sprintf( p->pMan->sError, "Line %d: Reset line \"%s\" defines signal that is not a latch output.", Io_MvGetLine(p->pMan, pName), pName );
            return 0;
        }
*/
        // construct the reset circuit and get the reset net feeding into it
        pNet = Io_MvParseAddResetCircuit( p, pName );
        // create fanins
        pNode = Io_ReadCreateNode( p->pNtk, Abc_ObjName(pNet), (char **)(vTokens->pArray + 1), nInputs );
        assert( nInputs == Vec_PtrSize(vTokens) - 2 );
    }
    else
    {
        pNet = Abc_NtkFindOrCreateNet( p->pNtk, pName );
        if ( Abc_ObjFaninNum(pNet) > 0 )
        {
            sprintf( p->pMan->sError, "Line %d: Signal \"%s\" is defined more than once.", Io_MvGetLine(p->pMan, pName), pName );
            return 0;
        }
        pNode = Io_ReadCreateNode( p->pNtk, pName, (char **)(vTokens->pArray + 1), nInputs );
    }
    // create the cover
    pNode->pData = Io_MvParseTableMv( p, pNode, vTokens2, nInputs, nOutputs, iOut );
    if ( pNode->pData == NULL )
        return 0;
    pNode->pData = Abc_SopRegister( (Mem_Flex_t *)p->pNtk->pManFunc, (char *)pNode->pData );
//printf( "Finished parsing node \"%s\" with table:\n%s\n", pName, pNode->pData );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Parses the nodes line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineNamesMv( Io_MvMod_t * p, char * pLine, int fReset )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
    Vec_Ptr_t * vTokens2 = p->pMan->vTokens2;
    Abc_Obj_t * pNet;
    char * pName, * pFirst, * pArrow;
    int nInputs, nOutputs, nLiterals, nLines, i;
    assert( p->pMan->fBlifMv );
    // get the arrow if it is present
    pArrow = Io_MvFindArrow( pLine );
    if ( !p->pMan->fBlifMv && pArrow ) 
    {
        sprintf( p->pMan->sError, "Line %d: Multi-output node symbol (->) in binary BLIF file.", Io_MvGetLine(p->pMan, pLine) );
        return 0;
    }
    // split names line into tokens
    Io_MvSplitIntoTokens( vTokens, pLine, '\0' );
    if ( fReset )
        assert( !strcmp((char *)Vec_PtrEntry(vTokens,0), "r") || !strcmp((char *)Vec_PtrEntry(vTokens,0), "reset") );
    else
        assert( !strcmp((char *)Vec_PtrEntry(vTokens,0), "names") || !strcmp((char *)Vec_PtrEntry(vTokens,0), "table") );
    // find the number of inputs and outputs
    nInputs  = Vec_PtrSize(vTokens) - 2;
    nOutputs = 1;
    if ( pArrow != NULL )
    {
        for ( i = Vec_PtrSize(vTokens) - 2; i >= 1; i-- )
            if ( pArrow < (char*)Vec_PtrEntry(vTokens,i) )
            {
                nInputs--;
                nOutputs++;
            }
    }
    // split table into tokens
    pName = (char *)Vec_PtrEntryLast( vTokens );
    Io_MvSplitIntoTokensMv( vTokens2, pName + strlen(pName) );
    pFirst = (char *)Vec_PtrEntry( vTokens2, 0 );
    if ( pFirst[0] == '.' )
    {
        assert( pFirst[1] == 'd' );
        nLiterals = Vec_PtrSize(vTokens2) - 1 - nOutputs;
    }
    else
        nLiterals = Vec_PtrSize(vTokens2);
    // check the number of lines
    if ( nLiterals % (nInputs + nOutputs) != 0 )
    {
        sprintf( p->pMan->sError, "Line %d: Wrong number of literals in the table of node \"%s\". (Spaces inside literals are not allowed.)", Io_MvGetLine(p->pMan, pFirst), pName );
        return 0;
    }
    // check for the ND table
    nLines = nLiterals / (nInputs + nOutputs);
    if ( nInputs == 0 && nLines > 1 )
    {
        // add the outputs to the PIs
        for ( i = 0; i < nOutputs; i++ )
        {
            pName = (char *)Vec_PtrEntry( vTokens, Vec_PtrSize(vTokens) - nOutputs + i );
            // get the net corresponding to this node
            pNet = Abc_NtkFindOrCreateNet(p->pNtk, pName);
            if ( fReset )
            {
                assert( p->pResetLatch != NULL );
                // construct the reset circuit and get the reset net feeding into it
                pNet = Io_MvParseAddResetCircuit( p, pName );
            }
            // add the new PI node
//            Abc_ObjAddFanin( pNet, Abc_NtkCreatePi(p->pNtk) );
//            fprintf( stdout, "Io_ReadBlifMv(): Adding PI for internal non-deterministic node \"%s\".\n", pName );
            p->pMan->nNDnodes++;
            Abc_ObjAddFanin( pNet, Abc_NtkCreateNodeConst0(p->pNtk) );
        }
        return 1;
    }
    // iterate through the outputs
    for ( i = 0; i < nOutputs; i++ )
    {
        if ( !Io_MvParseLineNamesMvOne( p, vTokens, vTokens2, nInputs, nOutputs, i, fReset ) )
            return 0;
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Constructs the SOP cover from the file parsing info.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static char * Io_MvParseTableBlif( Io_MvMod_t * p, char * pTable, int nFanins )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
    Vec_Str_t * vFunc = p->pMan->vFunc;
    char * pProduct, * pOutput, c;
    int i, Polarity = -1;

    p->pMan->nTablesRead++;
    // get the tokens
    Io_MvSplitIntoTokens( vTokens, pTable, '.' );
    if ( Vec_PtrSize(vTokens) == 0 )
        return Abc_SopCreateConst0( (Mem_Flex_t *)p->pNtk->pManFunc );
    if ( Vec_PtrSize(vTokens) == 1 )
    {
        pOutput = (char *)Vec_PtrEntry( vTokens, 0 );
        c = pOutput[0];
        if ( (c!='0'&&c!='1'&&c!='x'&&c!='n') || pOutput[1] )
        {
            sprintf( p->pMan->sError, "Line %d: Constant table has wrong output value \"%s\".", Io_MvGetLine(p->pMan, pOutput), pOutput );
            return NULL;
        }
        return pOutput[0] == '0' ? Abc_SopCreateConst0((Mem_Flex_t *)p->pNtk->pManFunc) : Abc_SopCreateConst1((Mem_Flex_t *)p->pNtk->pManFunc);
    }
    pProduct = (char *)Vec_PtrEntry( vTokens, 0 );
    if ( Vec_PtrSize(vTokens) % 2 == 1 )
    {
        sprintf( p->pMan->sError, "Line %d: Table has odd number of tokens (%d).", Io_MvGetLine(p->pMan, pProduct), Vec_PtrSize(vTokens) );
        return NULL;
    }
    // parse the table
    Vec_StrClear( vFunc );
    for ( i = 0; i < Vec_PtrSize(vTokens)/2; i++ )
    {
        pProduct = (char *)Vec_PtrEntry( vTokens, 2*i + 0 );
        pOutput  = (char *)Vec_PtrEntry( vTokens, 2*i + 1 );
        if ( strlen(pProduct) != (unsigned)nFanins )
        {
            sprintf( p->pMan->sError, "Line %d: Cube \"%s\" has size different from the fanin count (%d).", Io_MvGetLine(p->pMan, pProduct), pProduct, nFanins );
            return NULL;
        }
        c = pOutput[0];
        if ( (c!='0'&&c!='1'&&c!='x'&&c!='n') || pOutput[1] )
        {
            sprintf( p->pMan->sError, "Line %d: Output value \"%s\" is incorrect.", Io_MvGetLine(p->pMan, pProduct), pOutput );
            return NULL;
        }
        if ( Polarity == -1 )
            Polarity = (c=='1' || c=='x');
        else if ( Polarity != (c=='1' || c=='x') )
        {
            sprintf( p->pMan->sError, "Line %d: Output value \"%s\" differs from the value in the first line of the table (%d).", Io_MvGetLine(p->pMan, pProduct), pOutput, Polarity );
            return NULL;
        }
        // parse one product 
        Vec_StrPrintStr( vFunc, pProduct );
        Vec_StrPush( vFunc, ' ' );
        Vec_StrPush( vFunc, pOutput[0] );
        Vec_StrPush( vFunc, '\n' );
    }
    Vec_StrPush( vFunc, '\0' );
    return Vec_StrArray( vFunc );
}

/**Function*************************************************************

  Synopsis    [Parses the nodes line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineNamesBlif( Io_MvMod_t * p, char * pLine )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
    Abc_Obj_t * pNet, * pNode;
    char * pName;
    assert( !p->pMan->fBlifMv );
    Io_MvSplitIntoTokens( vTokens, pLine, '\0' );
    // parse the mapped node
    if ( !strcmp((char *)Vec_PtrEntry(vTokens,0), "gate") )
        return Io_MvParseLineGateBlif( p, vTokens );
    // parse the regular name line
    assert( !strcmp((char *)Vec_PtrEntry(vTokens,0), "names") );
    pName = (char *)Vec_PtrEntryLast( vTokens );
    pNet = Abc_NtkFindOrCreateNet( p->pNtk, pName );
    if ( Abc_ObjFaninNum(pNet) > 0 )
    {
        sprintf( p->pMan->sError, "Line %d: Signal \"%s\" is defined more than once.", Io_MvGetLine(p->pMan, pName), pName );
        return 0;
    }
    // create fanins
    pNode = Io_ReadCreateNode( p->pNtk, pName, (char **)(vTokens->pArray + 1), Vec_PtrSize(vTokens) - 2 );
    // parse the table of this node
    pNode->pData = Io_MvParseTableBlif( p, pName + strlen(pName), Abc_ObjFaninNum(pNode) );
    if ( pNode->pData == NULL )
        return 0;
    pNode->pData = Abc_SopRegister( (Mem_Flex_t *)p->pNtk->pManFunc, (char *)pNode->pData );
    return 1;
}

ABC_NAMESPACE_IMPL_END

#include "map/mio/mio.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START


/**Function*************************************************************

  Synopsis    [Parses the nodes line.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineShortBlif( Io_MvMod_t * p, char * pLine )
{
    Vec_Ptr_t * vTokens = p->pMan->vTokens;
    Abc_Obj_t * pNet, * pNode;
    char * pName;
    assert( !p->pMan->fBlifMv );
    Io_MvSplitIntoTokens( vTokens, pLine, '\0' );
    if ( Vec_PtrSize(vTokens) != 3 )
    {
        sprintf( p->pMan->sError, "Line %d: Expecting three entries in the .short line.", Io_MvGetLine(p->pMan, (char *)Vec_PtrEntry(vTokens,0)) );
        return 0;
    }
    // parse the regular name line
    assert( !strcmp((char *)Vec_PtrEntry(vTokens,0), "short") );
    pName = (char *)Vec_PtrEntryLast( vTokens );
    pNet = Abc_NtkFindOrCreateNet( p->pNtk, pName );
    if ( Abc_ObjFaninNum(pNet) > 0 )
    {
        sprintf( p->pMan->sError, "Line %d: Signal \"%s\" is defined more than once.", Io_MvGetLine(p->pMan, pName), pName );
        return 0;
    }
    // create fanins
    pNode = Io_ReadCreateNode( p->pNtk, pName, (char **)(vTokens->pArray + 1), 1 );
    // parse the table of this node
    if ( p->pNtk->ntkFunc == ABC_FUNC_MAP )
    {
        Mio_Library_t * pGenlib; 
        Mio_Gate_t * pGate;
        // check that the library is available
        pGenlib = (Mio_Library_t *)Abc_FrameReadLibGen();
        if ( pGenlib == NULL )
        {
            sprintf( p->pMan->sError, "Line %d: The current library is not available.", Io_MvGetLine(p->pMan, pName) );
            return 0;
        }
        // get the gate
        pGate = Mio_LibraryReadBuf( pGenlib );
        if ( pGate == NULL )
        {
            sprintf( p->pMan->sError, "Line %d: Cannot find buffer gate in the library.", Io_MvGetLine(p->pMan, pName) );
            return 0;
        }
        Abc_ObjSetData( pNode, pGate );
    }
    else
        pNode->pData = Abc_SopRegister( (Mem_Flex_t *)p->pNtk->pManFunc, "1 1\n" );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Duplicate the MV variable.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
Io_MvVar_t * Abc_NtkMvVarDup( Abc_Ntk_t * pNtk, Io_MvVar_t * pVar )
{
    Mem_Flex_t * pFlex;
    Io_MvVar_t * pVarDup;
    int i;
    if ( pVar == NULL )
        return NULL;
    pFlex = (Mem_Flex_t *)Abc_NtkMvVarMan( pNtk );
    assert( pFlex != NULL );
    pVarDup = (Io_MvVar_t *)Mem_FlexEntryFetch( pFlex, sizeof(Io_MvVar_t) );
    pVarDup->nValues = pVar->nValues;
    pVarDup->pNames = NULL;
    if ( pVar->pNames == NULL )
        return pVarDup;
    pVarDup->pNames = (char **)Mem_FlexEntryFetch( pFlex, sizeof(char *) * pVar->nValues );
    for ( i = 0; i < pVar->nValues; i++ )
    {
        pVarDup->pNames[i] = (char *)Mem_FlexEntryFetch( pFlex, strlen(pVar->pNames[i]) + 1 );
        strcpy( pVarDup->pNames[i], pVar->pNames[i] );
    }
    return pVarDup;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static char * Io_ReadBlifCleanName( char * pName )
{
    int i, Length;
    Length = strlen(pName);
    for ( i = 0; i < Length; i++ )
        if ( pName[i] == '=' )
            return pName + i + 1;
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static int Io_MvParseLineGateBlif( Io_MvMod_t * p, Vec_Ptr_t * vTokens )
{
    extern int Io_ReadBlifReorderFormalNames( Vec_Ptr_t * vTokens, Mio_Gate_t * pGate, Mio_Gate_t * pTwin );
    Mio_Library_t * pGenlib; 
    Mio_Gate_t * pGate;
    Abc_Obj_t * pNode;
    char ** ppNames, * pName;
    int i, nNames;

    pName = (char *)vTokens->pArray[0];

    // check that the library is available
    pGenlib = (Mio_Library_t *)Abc_FrameReadLibGen();
    if ( pGenlib == NULL )
    {
        sprintf( p->pMan->sError, "Line %d: The current library is not available.", Io_MvGetLine(p->pMan, pName) );
        return 0;
    }

    // create a new node and add it to the network
    if ( vTokens->nSize < 2 )
    {
        sprintf( p->pMan->sError, "Line %d: The .gate line has less than two tokens.", Io_MvGetLine(p->pMan, pName) );
        return 0;
    }

    // get the gate
    pGate = Mio_LibraryReadGateByName( pGenlib, (char *)vTokens->pArray[1], NULL );
    if ( pGate == NULL )
    {
        sprintf( p->pMan->sError, "Line %d: Cannot find gate \"%s\" in the library.", Io_MvGetLine(p->pMan, pName), (char*)vTokens->pArray[1] );
        return 0;
    }

    // if this is the first line with gate, update the network type
    if ( Abc_NtkNodeNum(p->pNtk) == 0 && p->pNtk->ntkFunc == ABC_FUNC_SOP )
    {
        assert( p->pNtk->ntkFunc == ABC_FUNC_SOP );
        p->pNtk->ntkFunc = ABC_FUNC_MAP;
        Mem_FlexStop( (Mem_Flex_t *)p->pNtk->pManFunc, 0 );
        p->pNtk->pManFunc = pGenlib;
        if ( p->pMan && p->pMan->pDesign && Vec_PtrSize(p->pMan->pDesign->vModules) > 0 )
        {
            Abc_Ntk_t * pModel; int k;
            Vec_PtrForEachEntry( Abc_Ntk_t *, p->pMan->pDesign->vModules, pModel, k )
            {
                if ( pModel == p->pNtk )
                    continue;
                assert( pModel->ntkFunc == ABC_FUNC_SOP );
                pModel->ntkFunc = ABC_FUNC_MAP;
                Mem_FlexStop( (Mem_Flex_t *)pModel->pManFunc, 0 );
                pModel->pManFunc = pGenlib;
            }
        }
    }

    // reorder the formal inputs to be in the same order as in the gate
    if ( !Io_ReadBlifReorderFormalNames( vTokens, pGate, Mio_GateReadTwin(pGate) ) )
    {
        sprintf( p->pMan->sError, "Line %d: Mismatch in the fanins of gate \"%s\".", Io_MvGetLine(p->pMan, pName), (char*)vTokens->pArray[1] );
        return 0;
    }

    // remove the formal parameter names
    for ( i = 2; i < vTokens->nSize; i++ )
    {
        if ( vTokens->pArray[i] == NULL )
            continue;
        vTokens->pArray[i] = Io_ReadBlifCleanName( (char *)vTokens->pArray[i] );
        if ( vTokens->pArray[i] == NULL )
        {
            sprintf( p->pMan->sError, "Line %d: Invalid gate input assignment.", Io_MvGetLine(p->pMan, pName) );
            return 0;
        }
    }

    // create the node
    if ( Mio_GateReadTwin(pGate) == NULL )
    {
        nNames  = vTokens->nSize - 3;
        ppNames = (char **)vTokens->pArray + 2;
        pNode   = Io_ReadCreateNode( p->pNtk, ppNames[nNames], ppNames, nNames );
        Abc_ObjSetData( pNode, pGate );
    }
    else
    {
        nNames  = vTokens->nSize - 4;
        ppNames = (char **)vTokens->pArray + 2;
        assert( ppNames[nNames] != NULL || ppNames[nNames+1] != NULL );
        if ( ppNames[nNames] )
        {
            pNode   = Io_ReadCreateNode( p->pNtk, ppNames[nNames], ppNames, nNames );
            Abc_ObjSetData( pNode, pGate );
        }
        if ( ppNames[nNames+1] )
        {
            pNode   = Io_ReadCreateNode( p->pNtk, ppNames[nNames+1], ppNames, nNames );
            Abc_ObjSetData( pNode, Mio_GateReadTwin(pGate) );
        }
    }

    return 1;
}

/**Function*************************************************************

  Synopsis    [Box mapping procedures.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_MapBoxSetPrevNext( Vec_Ptr_t * vDrivers, Vec_Int_t * vMapIn, Vec_Int_t * vMapOut, int Id )
{
    Abc_Obj_t * pNode;
    pNode = (Abc_Obj_t *)Vec_PtrEntry(vDrivers, Id+2);
    Vec_IntWriteEntry( vMapIn, Abc_ObjId(Abc_ObjFanin0(Abc_ObjFanin0(pNode))), Id );
    pNode = (Abc_Obj_t *)Vec_PtrEntry(vDrivers, Id+4);
    Vec_IntWriteEntry( vMapOut, Abc_ObjId(Abc_ObjFanin0(Abc_ObjFanin0(pNode))), Id );
}
static inline int Abc_MapBox2Next( Vec_Ptr_t * vDrivers, Vec_Int_t * vMapIn, Vec_Int_t * vMapOut, int Id )
{
    Abc_Obj_t * pNode = (Abc_Obj_t *)Vec_PtrEntry(vDrivers, Id+4);
    return Vec_IntEntry( vMapIn, Abc_ObjId(Abc_ObjFanin0(Abc_ObjFanin0(pNode))) );
}
static inline int Abc_MapBox2Prev( Vec_Ptr_t * vDrivers, Vec_Int_t * vMapIn, Vec_Int_t * vMapOut, int Id )
{
    Abc_Obj_t * pNode = (Abc_Obj_t *)Vec_PtrEntry(vDrivers, Id+2);
    return Vec_IntEntry( vMapOut, Abc_ObjId(Abc_ObjFanin0(Abc_ObjFanin0(pNode))) );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

