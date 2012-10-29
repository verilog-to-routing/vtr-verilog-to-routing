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

#include "abc.h"
#include "extra.h"
#include "vecPtr.h"
#include "io.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define IO_BLIFMV_MAXVALUES 256

typedef struct Io_MvVar_t_ Io_MvVar_t; // parsing var
typedef struct Io_MvMod_t_ Io_MvMod_t; // parsing model
typedef struct Io_MvMan_t_ Io_MvMan_t; // parsing manager

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
    Vec_Ptr_t *          vMvs;         // .mv lines
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
    Abc_Lib_t *          pDesign;      // the design under construction
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
static void              Io_MvReadInterfaces( Io_MvMan_t * p );
static Abc_Lib_t *       Io_MvParse( Io_MvMan_t * p );
static int               Io_MvParseLineModel( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineInputs( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineOutputs( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineLatch( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineSubckt( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineMv( Io_MvMod_t * p, char * pLine );
static int               Io_MvParseLineNamesMv( Io_MvMod_t * p, char * pLine, int fReset );
static int               Io_MvParseLineNamesBlif( Io_MvMod_t * p, char * pLine );
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
    Abc_Ntk_t * pNtk;
    Abc_Lib_t * pDesign;
    char * pDesignName;
    int RetValue, i;

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
    p->fUseReset = 0;
    p->pFileName = pFileName;
    p->pBuffer   = Io_MvLoadFile( pFileName );
    if ( p->pBuffer == NULL )
    {
        Io_MvFree( p );
        return NULL;
    }
    // set the design name
    pDesignName  = Extra_FileNameGeneric( pFileName );
    p->pDesign   = Abc_LibCreate( pDesignName );
    free( pDesignName );
    // free the HOP manager
    Hop_ManStop( p->pDesign->pManFunc );
    p->pDesign->pManFunc = NULL;
    // prepare the file for parsing
    Io_MvReadPreparse( p );
    // parse interfaces of each network
    Io_MvReadInterfaces( p );
    // construct the network
    pDesign = Io_MvParse( p );
    if ( p->sError[0] )
        fprintf( stdout, "%s\n", p->sError );
    if ( pDesign == NULL )
        return NULL;
    Io_MvFree( p );
// pDesign should be linked to all models of the design

    // make sure that everything is okay with the network structure
    if ( fCheck )
    {
        Vec_PtrForEachEntry( pDesign->vModules, pNtk, i )
        {
            if ( !Abc_NtkCheckRead( pNtk ) )
            {
                printf( "Io_ReadBlifMv: The network check has failed for network %s.\n", pNtk->pName );
                Abc_LibFree( pDesign, NULL );
                return NULL;
            }
        }
    }

//Abc_LibPrint( pDesign );

    // detect top-level model
    RetValue = Abc_LibFindTopLevelModels( pDesign );
    pNtk = Vec_PtrEntry( pDesign->vTops, 0 );
    if ( RetValue > 1 )
        printf( "Warning: The design has %d root-level modules. The first one (%s) will be used. Ignored (%s)\n",
            Vec_PtrSize(pDesign->vTops), pNtk->pName, ((Abc_Ntk_t * )Vec_PtrEntry( pDesign->vTops, 1 ))->pName );

    // extract the master network
    pNtk->pDesign = pDesign;
    pDesign->pManFunc = NULL;

    // verify the design for cyclic dependence
    assert( Vec_PtrSize(pDesign->vModules) > 0 );
    if ( Vec_PtrSize(pDesign->vModules) == 1 )
    {
//        printf( "Warning: The design is not hierarchical.\n" );
        Abc_LibFree( pDesign, pNtk );
        pNtk->pDesign = NULL;
        pNtk->pSpec = Extra_UtilStrsav( pFileName );
    }
    else
        Abc_NtkIsAcyclicHierarchy( pNtk );

//Io_WriteBlifMv( pNtk, "_temp_.mv" );
    if ( pNtk->pSpec == NULL )
        pNtk->pSpec = Extra_UtilStrsav( pFileName );
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
    p = ALLOC( Io_MvMan_t, 1 );
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
        Abc_LibFree( p->pDesign, NULL );
    if ( p->pBuffer )  
        free( p->pBuffer );
    if ( p->vLines )
        Vec_PtrFree( p->vLines  );
    if ( p->vModels )
    {
        Vec_PtrForEachEntry( p->vModels, pMod, i )
            Io_MvModFree( pMod );
        Vec_PtrFree( p->vModels );
    }
    Vec_PtrFree( p->vTokens );
    Vec_PtrFree( p->vTokens2 );
    Vec_StrFree( p->vFunc );
    free( p );
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
    p = ALLOC( Io_MvMod_t, 1 );
    memset( p, 0, sizeof(Io_MvMod_t) );
    p->vInputs  = Vec_PtrAlloc( 512 );
    p->vOutputs = Vec_PtrAlloc( 512 );
    p->vLatches = Vec_PtrAlloc( 512 );
	p->vFlops   = Vec_PtrAlloc( 512 );
    p->vResets  = Vec_PtrAlloc( 512 );
    p->vNames   = Vec_PtrAlloc( 512 );
    p->vSubckts = Vec_PtrAlloc( 512 );
    p->vMvs     = Vec_PtrAlloc( 512 );
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
    Vec_PtrFree( p->vInputs );
    Vec_PtrFree( p->vOutputs );
    Vec_PtrFree( p->vLatches );
	Vec_PtrFree( p->vFlops );
    Vec_PtrFree( p->vResets );
    Vec_PtrFree( p->vNames );
    Vec_PtrFree( p->vSubckts );
    Vec_PtrFree( p->vMvs );
    free( p );
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
    Vec_PtrForEachEntry( p->vLines, pLine, i )
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
static char * Io_MvLoadFile( char * pFileName )
{
    FILE * pFile;
    int nFileSize;
    char * pContents;
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
        printf( "Io_MvLoadFile(): The file is empty.\n" );
        return NULL;
    }
    pContents = ALLOC( char, nFileSize + 10 );
    rewind( pFile );
    fread( pContents, nFileSize, 1, pFile );
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
    Vec_PtrForEachEntry( p->vLines, pCur, i )
    {
        if ( *pCur == 0 )
            continue;
        // find previous non-space character
        for ( pPrev = pCur - 2; pPrev >= p->pBuffer; pPrev-- )
            if ( !Io_MvCharIsSpace(*pPrev) )
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
        while ( Io_MvCharIsSpace(*pCur++) );
        // parse directives
        if ( *(pCur-1) != '.' )
            continue;
        if ( !strncmp(pCur, "names", 5) || !strncmp(pCur, "table", 5) || !strncmp(pCur, "gate", 4) )
            Vec_PtrPush( p->pLatest->vNames, pCur );
        else if ( p->fBlifMv && (!strncmp(pCur, "def ", 4) || !strncmp(pCur, "default ", 8)) )
            continue;
        else if ( !strncmp(pCur, "latch", 5) )
            Vec_PtrPush( p->pLatest->vLatches, pCur );
        else if ( !strncmp(pCur, "r ", 2) || !strncmp(pCur, "reset ", 6) )
            Vec_PtrPush( p->pLatest->vResets, pCur );
        else if ( !strncmp(pCur, "inputs", 6) )
            Vec_PtrPush( p->pLatest->vInputs, pCur );
        else if ( !strncmp(pCur, "outputs", 7) )
            Vec_PtrPush( p->pLatest->vOutputs, pCur );
        else if ( !strncmp(pCur, "subckt", 6) )
            Vec_PtrPush( p->pLatest->vSubckts, pCur );
        else if ( p->fBlifMv && !strncmp(pCur, "mv", 2) )
            Vec_PtrPush( p->pLatest->vMvs, pCur );
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
            fprintf( stdout, "Line %d: Skipping EXDC network.\n", Io_MvGetLine(p, pCur) );
            break;
        }
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
static void Io_MvReadInterfaces( Io_MvMan_t * p )
{
    Io_MvMod_t * pMod;
    char * pLine;
    int i, k;
    // iterate through the models
    Vec_PtrForEachEntry( p->vModels, pMod, i )
    {
        // parse the model
        if ( !Io_MvParseLineModel( pMod, pMod->pName ) )
            return;
        // add model to the design
        if ( !Abc_LibAddModel( p->pDesign, pMod->pNtk ) )
        {
            sprintf( p->sError, "Line %d: Model %s is defined twice.", Io_MvGetLine(p, pMod->pName), pMod->pName );
            return;
        }
        // parse the inputs
        Vec_PtrForEachEntry( pMod->vInputs, pLine, k )
            if ( !Io_MvParseLineInputs( pMod, pLine ) )
                return;
        // parse the outputs
        Vec_PtrForEachEntry( pMod->vOutputs, pLine, k )
            if ( !Io_MvParseLineOutputs( pMod, pLine ) )
                return;
    }
}


/**Function*************************************************************

  Synopsis    []

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static Abc_Lib_t * Io_MvParse( Io_MvMan_t * p )
{
    Abc_Lib_t * pDesign;
    Io_MvMod_t * pMod;
    char * pLine;
    int i, k;
    // iterate through the models
    Vec_PtrForEachEntry( p->vModels, pMod, i )
    {
        // check if there any MV lines
        if ( Vec_PtrSize(pMod->vMvs) > 0 )
            Abc_NtkStartMvVars( pMod->pNtk );
        // parse the mv lines
        Vec_PtrForEachEntry( pMod->vMvs, pLine, k )
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
        // parse the latches
        Vec_PtrForEachEntry( pMod->vLatches, pLine, k )
            if ( !Io_MvParseLineLatch( pMod, pLine ) )
                return NULL;
        // parse the reset lines
        if ( p->fUseReset )
            Vec_PtrForEachEntry( pMod->vResets, pLine, k )
                if ( !Io_MvParseLineNamesMv( pMod, pLine, 1 ) )
                    return NULL;
        // parse the nodes
        if ( p->fBlifMv )
        {
            Vec_PtrForEachEntry( pMod->vNames, pLine, k )
                if ( !Io_MvParseLineNamesMv( pMod, pLine, 0 ) )
                    return NULL;
        }
        else
        {
            Vec_PtrForEachEntry( pMod->vNames, pLine, k )
                if ( !Io_MvParseLineNamesBlif( pMod, pLine ) )
                    return NULL;
        }
        // parse the subcircuits
        Vec_PtrForEachEntry( pMod->vSubckts, pLine, k )
            if ( !Io_MvParseLineSubckt( pMod, pLine ) )
                return NULL;
        // finalize the network
        Abc_NtkFinalizeRead( pMod->pNtk );
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
    char * pToken;
    Io_MvSplitIntoTokens( vTokens, pLine, '\0' );
    pToken = Vec_PtrEntry( vTokens, 0 );
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
    p->pNtk->pName = Extra_UtilStrsav( Vec_PtrEntry(vTokens, 1) );
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
    pToken = Vec_PtrEntry(vTokens, 0);
    assert( !strcmp(pToken, "inputs") );
    Vec_PtrForEachEntryStart( vTokens, pToken, i, 1 )
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
    pToken = Vec_PtrEntry(vTokens, 0);
    assert( !strcmp(pToken, "outputs") );
    Vec_PtrForEachEntryStart( vTokens, pToken, i, 1 )
        Io_ReadCreatePo( p->pNtk, pToken );
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
	char * pLatchType;
    Abc_LatchInfo_t* pLatchInfo;
    int Init;
    Io_MvSplitIntoTokens( vTokens, pLine, '\0' );
    pToken = Vec_PtrEntry(vTokens,0);
    assert( !strcmp(pToken, "latch") );
    if ( Vec_PtrSize(vTokens) < 3 )
    {
        sprintf( p->pMan->sError, "Line %d: Latch does not have input name and output name.", Io_MvGetLine(p->pMan, pToken) );
        return 0;
    }
    // create latch
    if ( p->pResetLatch == NULL )
    {
        pObj = Io_ReadCreateLatch( p->pNtk, Vec_PtrEntry(vTokens,1), Vec_PtrEntry(vTokens,2) );

		pLatchInfo = ((Abc_LatchInfo_t *)pObj->pData);
		pLatchInfo->pClkName = strdup((char *)Vec_PtrEntry(vTokens,4));
	
		pLatchType = (char *)Vec_PtrEntry(vTokens,3);
		
		if (!strcmp(pLatchType, "re"))
			pLatchInfo->LatchType = ABC_RISING_EDGE;
		else if (!strcmp(pLatchType, "fe"))
			pLatchInfo->LatchType = ABC_FALLING_EDGE;
		else if (!strcmp(pLatchType, "ah"))
			pLatchInfo->LatchType = ABC_ACTIVE_HIGH;
		else if (!strcmp(pLatchType, "al"))
			pLatchInfo->LatchType = ABC_ACTIVE_LOW;
		else
			pLatchInfo->LatchType = ABC_ASYNC; // god knows when it is used...

        // get initial value
        if ( p->pMan->fBlifMv )
            Abc_LatchSetInit0( pObj );
        else
        {
			if ( Vec_PtrSize(vTokens) > 6 )
                printf( "Warning: Line %d has .latch directive with unrecognized entries (the total of %d entries).\n", 
                    Io_MvGetLine(p->pMan, pToken), Vec_PtrSize(vTokens) ); 
            if ( Vec_PtrSize(vTokens) > 3 )
                Init = atoi( Vec_PtrEntry(vTokens,3) );
            else
                Init = 2;
            if ( Init < 0 || Init > 2 )
            {
                sprintf( p->pMan->sError, "Line %d: Initial state of the latch is incorrect \"%s\".", Io_MvGetLine(p->pMan, pToken), Vec_PtrEntry(vTokens,3) );
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
        pNet = Abc_NtkFindOrCreateNet( p->pNtk, Vec_PtrEntry(vTokens,2) );
        // get the net corresponding to the latch output (feeding into reset MUX)
        pNet = Abc_NtkFindOrCreateNet( p->pNtk, Abc_ObjNameSuffix(pNet, "_out") );
        // create latch
        pObj = Io_ReadCreateLatch( p->pNtk, Vec_PtrEntry(vTokens,1), Abc_ObjName(pNet) );
        Abc_LatchSetInit0( pObj );
    }
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
    char * pToken, * pName, ** ppNames;
    int nEquals, i, k;

    // split the line into tokens
    nEquals = Io_MvCountChars( pLine, '=' );
    Io_MvSplitIntoTokensAndClear( vTokens, pLine, '\0', '=' );
    pToken = Vec_PtrEntry(vTokens,0);
    assert( !strcmp(pToken, "subckt") );

    // get the model for this box
    pName = Vec_PtrEntry(vTokens,1);
    pModel = Abc_LibFindModelByName( p->pMan->pDesign, pName );
    if ( pModel == NULL )
    {
        sprintf( p->pMan->sError, "Line %d: Cannot find the model for subcircuit %s.", Io_MvGetLine(p->pMan, pToken), pName );
        return 0;
    }

    // check if the number of tokens is correct
    if ( nEquals != Abc_NtkPiNum(pModel) + Abc_NtkPoNum(pModel) )
    {
        sprintf( p->pMan->sError, "Line %d: The number of ports (%d) in .subckt differs from the sum of PIs and POs of the model (%d).", 
            Io_MvGetLine(p->pMan, pToken), nEquals, Abc_NtkPiNum(pModel) + Abc_NtkPoNum(pModel) );
        return 0;
    }

    // get the names
    ppNames = (char **)Vec_PtrArray(vTokens) + 2 + p->pMan->fBlifMv;

    // create the box with these terminals
    if ( Abc_NtkHasBlackbox(pModel) )
        pBox = Abc_NtkCreateBlackbox( p->pNtk );
    else
        pBox = Abc_NtkCreateWhitebox( p->pNtk );
    pBox->pData = pModel;
    if ( p->pMan->fBlifMv )
        Abc_ObjAssignName( pBox, Vec_PtrEntry(vTokens,2), NULL );
    Abc_NtkForEachPi( pModel, pTerm, i )
    {
        // find this terminal among the formal inputs of the subcircuit
        pName = Abc_ObjName(Abc_ObjFanout0(pTerm));
        for ( k = 0; k < nEquals; k++ )
            if ( !strcmp( ppNames[2*k], pName ) )
                break;
        if ( k == nEquals )
        {
            sprintf( p->pMan->sError, "Line %d: Cannot find PI \"%s\" of the model \"%s\" as a formal input of the subcircuit.", 
                Io_MvGetLine(p->pMan, pToken), pName, Abc_NtkName(pModel) );
            return 0;
        }
        // create the BI with the actual name
        pNet = Abc_NtkFindOrCreateNet( p->pNtk, ppNames[2*k+1] );
        pTerm = Abc_NtkCreateBi( p->pNtk );
        Abc_ObjAddFanin( pBox, pTerm );
        Abc_ObjAddFanin( pTerm, pNet );
    }
    Abc_NtkForEachPo( pModel, pTerm, i )
    {
        // find this terminal among the formal outputs of the subcircuit
        pName = Abc_ObjName(Abc_ObjFanin0(pTerm));
        for ( k = 0; k < nEquals; k++ )
            if ( !strcmp( ppNames[2*k], pName ) )
                break;
        if ( k == nEquals )
        {
            sprintf( p->pMan->sError, "Line %d: Cannot find PO \"%s\" of the modell \"%s\" as a formal output of the subcircuit.", 
                Io_MvGetLine(p->pMan, pToken), pName, Abc_NtkName(pModel) );
            return 0;
        }
        // create the BI with the actual name
        pNet = Abc_NtkFindOrCreateNet( p->pNtk, ppNames[2*k+1] );
        pTerm = Abc_NtkCreateBo( p->pNtk );
        Abc_ObjAddFanin( pNet, pTerm );
        Abc_ObjAddFanin( pTerm, pBox );
    }
    return 1;
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
    Io_MvVar_t * pVar;
    Extra_MmFlex_t * pFlex;
    char * pName;
    int nCommas, nValues, i, k;
    // count commas and get the tokens
    nCommas = Io_MvCountChars( pLine, ',' );
    Io_MvSplitIntoTokensAndClear( vTokens, pLine, '\0', ',' );
    pName = Vec_PtrEntry(vTokens,0);
    assert( !strcmp(pName, "mv") );
    // get the number of values
    if ( Vec_PtrSize(vTokens) <= nCommas + 2 )
    {
        sprintf( p->pMan->sError, "Line %d: The number of values in not specified in .mv line.", Io_MvGetLine(p->pMan, pName), pName );
        return 0;
    }
    nValues = atoi( Vec_PtrEntry(vTokens,nCommas+2) );
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
    pFlex = Abc_NtkMvVarMan( p->pNtk );
    for ( i = 0; i <= nCommas; i++ )
    {
        pName = Vec_PtrEntry( vTokens, i+1 );
        pObj = Abc_NtkFindOrCreateNet( p->pNtk, pName );
        // allocate variable
        pVar = (Io_MvVar_t *)Extra_MmFlexEntryFetch( pFlex, sizeof(Io_MvVar_t) );
        pVar->nValues = nValues;
        pVar->pNames = NULL;
        // create names
        if ( Vec_PtrSize(vTokens) > nCommas + 3 )
        {
            pVar->pNames = (char **)Extra_MmFlexEntryFetch( pFlex, sizeof(char *) * nValues );
            Vec_PtrForEachEntryStart( vTokens, pName, k, nCommas + 3 )
            {
                pVar->pNames[k-(nCommas + 3)] = (char *)Extra_MmFlexEntryFetch( pFlex, strlen(pName) + 1 );
                strcpy( pVar->pNames[k-(nCommas + 3)], pName );
            }
        }
        // save the variable
        Abc_ObjSetMvVar( pObj, pVar );
    }
    // make sure the names are unique
    if ( pVar->pNames )
    {
        for ( i = 0; i < nValues; i++ )
        for ( k = i+1; k < nValues; k++ )
            if ( !strcmp(pVar->pNames[i], pVar->pNames[k]) )
            {
                pName = Vec_PtrEntry(vTokens,0);
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
        Vec_StrAppend( vFunc, Buffer );
        Vec_StrPush( vFunc, ' ' );
    }
    // add the node number of values
    sprintf( Buffer, "%d", Abc_ObjMvVarNum(Abc_ObjFanout0(pNode)) );
    Vec_StrAppend( vFunc, Buffer );
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
    char Buffer[10];
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
        Vec_StrAppend( vFunc, Buffer );
        Vec_StrPush( vFunc, (char)((iLit == -1)? '\n' : ' ') );
        return 1;
    }
    // consider regular literal
    assert( iLit < Abc_ObjFaninNum(pNode) );
    pNet = iLit >= 0 ? Abc_ObjFanin(pNode, iLit) : Abc_ObjFanout0(pNode);
    pVar = Abc_ObjMvVar( pNet );
    // if the var is absent or has no symbolic values quit
    if ( pVar == NULL || pVar->pNames == NULL )
    {
        Vec_StrAppend( vFunc, pToken );
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
        Vec_StrAppend( vFunc, Buffer );
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
    pFirst = Vec_PtrEntry( vTokens2, 0 );
    if ( pFirst[0] == '.' )
    {
        // write the default literal
        Vec_StrPush( vFunc, 'd' );
        pToken = Vec_PtrEntry(vTokens2, 1 + iOut);
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
            pToken = Vec_PtrEntry( vTokens2, iStart + i );
            if ( !Io_MvParseLiteralMv( p, pNode, pToken, vFunc, i ) )
                return NULL;
        }
        // output literal
        pToken = Vec_PtrEntry( vTokens2, iStart + nInputs + iOut );
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
        pVar = Abc_ObjMvVar( pOutNet );
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
        int nValues = Abc_ObjMvVarNum(pOutNet);
//        sprintf( Buffer, "2 %d %d %d\n1 - - =1\n0 - - =2\n", nValues, nValues, nValues );
        sprintf( Buffer, "1 - - =1\n0 - - =2\n" );
        pNode->pData = Abc_SopRegister( p->pNtk->pManFunc, Buffer );
    }
    else
        pNode->pData = Abc_SopCreateMux( p->pNtk->pManFunc );
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
    pName = Vec_PtrEntry( vTokens, Vec_PtrSize(vTokens) - nOutputs + iOut );
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
    pNode->pData = Abc_SopRegister( p->pNtk->pManFunc, pNode->pData );
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
        assert( !strcmp(Vec_PtrEntry(vTokens,0), "r") || !strcmp(Vec_PtrEntry(vTokens,0), "reset") );
    else
        assert( !strcmp(Vec_PtrEntry(vTokens,0), "names") || !strcmp(Vec_PtrEntry(vTokens,0), "table") );
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
    pName = Vec_PtrEntryLast( vTokens );
    Io_MvSplitIntoTokensMv( vTokens2, pName + strlen(pName) );
    pFirst = Vec_PtrEntry( vTokens2, 0 );
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
            pName = Vec_PtrEntry( vTokens, Vec_PtrSize(vTokens) - nOutputs + i );
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
    char * pProduct, * pOutput;
    int i, Polarity = -1;

    p->pMan->nTablesRead++;
    // get the tokens
    Io_MvSplitIntoTokens( vTokens, pTable, '.' );
    if ( Vec_PtrSize(vTokens) == 0 )
        return Abc_SopCreateConst0( p->pNtk->pManFunc );
    if ( Vec_PtrSize(vTokens) == 1 )
    {
        pOutput = Vec_PtrEntry( vTokens, 0 );
        if ( ((pOutput[0] - '0') & 0x8E) || pOutput[1] )
        {
            sprintf( p->pMan->sError, "Line %d: Constant table has wrong output value \"%s\".", Io_MvGetLine(p->pMan, pOutput), pOutput );
            return NULL;
        }
        return pOutput[0] == '0' ? Abc_SopCreateConst0(p->pNtk->pManFunc) : Abc_SopCreateConst1(p->pNtk->pManFunc);
    }
    pProduct = Vec_PtrEntry( vTokens, 0 );
    if ( Vec_PtrSize(vTokens) % 2 == 1 )
    {
        sprintf( p->pMan->sError, "Line %d: Table has odd number of tokens (%d).", Io_MvGetLine(p->pMan, pProduct), Vec_PtrSize(vTokens) );
        return NULL;
    }
    // parse the table
    Vec_StrClear( vFunc );
    for ( i = 0; i < Vec_PtrSize(vTokens)/2; i++ )
    {
        pProduct = Vec_PtrEntry( vTokens, 2*i + 0 );
        pOutput  = Vec_PtrEntry( vTokens, 2*i + 1 );
        if ( strlen(pProduct) != (unsigned)nFanins )
        {
            sprintf( p->pMan->sError, "Line %d: Cube \"%s\" has size different from the fanin count (%d).", Io_MvGetLine(p->pMan, pProduct), pProduct, nFanins );
            return NULL;
        }
        if ( ((pOutput[0] - '0') & 0x8E) || pOutput[1] )
        {
            sprintf( p->pMan->sError, "Line %d: Output value \"%s\" is incorrect.", Io_MvGetLine(p->pMan, pProduct), pOutput );
            return NULL;
        }
        if ( Polarity == -1 )
            Polarity = pOutput[0] - '0';
        else if ( Polarity != pOutput[0] - '0' )
        {
            sprintf( p->pMan->sError, "Line %d: Output value \"%s\" differs from the value in the first line of the table (%d).", Io_MvGetLine(p->pMan, pProduct), pOutput, Polarity );
            return NULL;
        }
        // parse one product 
        Vec_StrAppend( vFunc, pProduct );
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
    if ( !strcmp(Vec_PtrEntry(vTokens,0), "gate") )
        return Io_MvParseLineGateBlif( p, vTokens );
    // parse the regular name line
    assert( !strcmp(Vec_PtrEntry(vTokens,0), "names") );
    pName = Vec_PtrEntryLast( vTokens );
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
    pNode->pData = Abc_SopRegister( p->pNtk->pManFunc, pNode->pData );
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
    Extra_MmFlex_t * pFlex;
    Io_MvVar_t * pVarDup;
    int i;
    if ( pVar == NULL )
        return NULL;
    pFlex = Abc_NtkMvVarMan( pNtk );
    assert( pFlex != NULL );
    pVarDup = (Io_MvVar_t *)Extra_MmFlexEntryFetch( pFlex, sizeof(Io_MvVar_t) );
    pVarDup->nValues = pVar->nValues;
    pVarDup->pNames = NULL;
    if ( pVar->pNames == NULL )
        return pVarDup;
    pVarDup->pNames = (char **)Extra_MmFlexEntryFetch( pFlex, sizeof(char *) * pVar->nValues );
    for ( i = 0; i < pVar->nValues; i++ )
    {
        pVarDup->pNames[i] = (char *)Extra_MmFlexEntryFetch( pFlex, strlen(pVar->pNames[i]) + 1 );
        strcpy( pVarDup->pNames[i], pVar->pNames[i] );
    }
    return pVarDup;
}


#include "mio.h"
#include "main.h"

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
    Mio_Library_t * pGenlib; 
    Mio_Gate_t * pGate;
    Abc_Obj_t * pNode;
    char ** ppNames, * pName;
    int i, nNames;

    pName = vTokens->pArray[0];

    // check that the library is available
    pGenlib = Abc_FrameReadLibGen();
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
    pGate = Mio_LibraryReadGateByName( pGenlib, vTokens->pArray[1] );
    if ( pGate == NULL )
    {
        sprintf( p->pMan->sError, "Line %d: Cannot find gate \"%s\" in the library.", Io_MvGetLine(p->pMan, pName), vTokens->pArray[1] );
        return 0;
    }

    // if this is the first line with gate, update the network type
    if ( Abc_NtkNodeNum(p->pNtk) == 0 )
    {
        assert( p->pNtk->ntkFunc == ABC_FUNC_SOP );
        p->pNtk->ntkFunc = ABC_FUNC_MAP;
        Extra_MmFlexStop( p->pNtk->pManFunc );
        p->pNtk->pManFunc = pGenlib;
    }

    // remove the formal parameter names
    for ( i = 2; i < vTokens->nSize; i++ )
    {
        vTokens->pArray[i] = Io_ReadBlifCleanName( vTokens->pArray[i] );
        if ( vTokens->pArray[i] == NULL )
        {
            sprintf( p->pMan->sError, "Line %d: Invalid gate input assignment.", Io_MvGetLine(p->pMan, pName) );
            return 0;
        }
    }

    // create the node
    ppNames = (char **)vTokens->pArray + 2;
    nNames  = vTokens->nSize - 3;
    pNode   = Io_ReadCreateNode( p->pNtk, ppNames[nNames], ppNames, nNames );

    // set the pointer to the functionality of the node
    Abc_ObjSetData( pNode, pGate );
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


