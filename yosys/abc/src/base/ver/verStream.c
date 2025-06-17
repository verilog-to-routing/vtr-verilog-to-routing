/**CFile****************************************************************

  FileName    [verStream.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Verilog parser.]

  Synopsis    [Input file stream, which knows nothing about Verilog.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 19, 2006.]

  Revision    [$Id: verStream.c,v 1.00 2006/08/19 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ver.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define VER_BUFFER_SIZE        1048576    // 1M  - size of the data chunk stored in memory
#define VER_OFFSET_SIZE          65536    // 64K - load new data when less than this is left
#define VER_WORD_SIZE            65536    // 64K - the largest token that can be returned

#define VER_MINIMUM(a,b)       (((a) < (b))? (a) : (b))

struct Ver_Stream_t_
{
    // the input file
    char *           pFileName;     // the input file name
    FILE *           pFile;         // the input file pointer
    iword            nFileSize;     // the total number of bytes in the file
    iword            nFileRead;     // the number of bytes currently read from file
    iword            nLineCounter;  // the counter of lines processed
    // temporary storage for data 
    iword            nBufferSize;   // the size of the buffer
    char *           pBuffer;       // the buffer
    char *           pBufferCur;    // the current reading position
    char *           pBufferEnd;    // the first position not used by currently loaded data
    char *           pBufferStop;   // the position where loading new data will be done
    // tokens given to the user
    char             pChars[VER_WORD_SIZE+5]; // temporary storage for a word (plus end-of-string and two parentheses)
    int              nChars;        // the total number of characters in the word
    // status of the parser
    int              fStop;         // this flag goes high when the end of file is reached
};

static void Ver_StreamReload( Ver_Stream_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the file reader for the given file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Ver_Stream_t * Ver_StreamAlloc( char * pFileName )
{
    Ver_Stream_t * p;
    FILE * pFile;
    int nCharsToRead;
    int RetValue;
    // check if the file can be opened
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Ver_StreamAlloc(): Cannot open input file \"%s\".\n", pFileName );
        return NULL;
    }
    // start the file reader    
    p = ABC_ALLOC( Ver_Stream_t, 1 );
    memset( p, 0, sizeof(Ver_Stream_t) );
    p->pFileName   = pFileName;
    p->pFile       = pFile;
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    p->nFileSize = ftell( pFile );  
    rewind( pFile ); 
    // allocate the buffer
    p->pBuffer = ABC_ALLOC( char, VER_BUFFER_SIZE+1 );
    p->nBufferSize = VER_BUFFER_SIZE;
    p->pBufferCur  = p->pBuffer;
    // determine how many chars to read
    nCharsToRead = VER_MINIMUM(p->nFileSize, VER_BUFFER_SIZE);
    // load the first part into the buffer
    RetValue = fread( p->pBuffer, nCharsToRead, 1, p->pFile );
    p->nFileRead = nCharsToRead;
    // set the ponters to the end and the stopping point
    p->pBufferEnd  = p->pBuffer + nCharsToRead;
    p->pBufferStop = (p->nFileRead == p->nFileSize)? p->pBufferEnd : p->pBuffer + VER_BUFFER_SIZE - VER_OFFSET_SIZE;
    // start the arrays
    p->nLineCounter = 1; // 1-based line counting
    return p;
}

/**Function*************************************************************

  Synopsis    [Loads new data into the file reader.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_StreamReload( Ver_Stream_t * p )
{
    int nCharsUsed, nCharsToRead;
    int RetValue;
    assert( !p->fStop );
    assert( p->pBufferCur > p->pBufferStop );
    assert( p->pBufferCur < p->pBufferEnd );
    // figure out how many chars are still not processed
    nCharsUsed = p->pBufferEnd - p->pBufferCur;
    // move the remaining data to the beginning of the buffer
    memmove( p->pBuffer, p->pBufferCur, (size_t)nCharsUsed );
    p->pBufferCur = p->pBuffer;
    // determine how many chars we will read
    nCharsToRead = VER_MINIMUM( p->nBufferSize - nCharsUsed, p->nFileSize - p->nFileRead );
    // read the chars
    RetValue = fread( p->pBuffer + nCharsUsed, nCharsToRead, 1, p->pFile );
    p->nFileRead += nCharsToRead;
    // set the ponters to the end and the stopping point
    p->pBufferEnd  = p->pBuffer + nCharsUsed + nCharsToRead;
    p->pBufferStop = (p->nFileRead == p->nFileSize)? p->pBufferEnd : p->pBuffer + VER_BUFFER_SIZE - VER_OFFSET_SIZE;
}

/**Function*************************************************************

  Synopsis    [Stops the file reader.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_StreamFree( Ver_Stream_t * p )
{
    if ( p->pFile )
        fclose( p->pFile );
    ABC_FREE( p->pBuffer );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns the file size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ver_StreamGetFileName( Ver_Stream_t * p )
{
    return p->pFileName;
}

/**Function*************************************************************

  Synopsis    [Returns the file size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_StreamGetFileSize( Ver_Stream_t * p )
{
    return p->nFileSize;
}

/**Function*************************************************************

  Synopsis    [Returns the current reading position.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_StreamGetCurPosition( Ver_Stream_t * p )
{
    return p->nFileRead - (p->pBufferEnd - p->pBufferCur);
}

/**Function*************************************************************

  Synopsis    [Returns the line number for the given token.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_StreamGetLineNumber( Ver_Stream_t * p )
{
    return p->nLineCounter;
}



/**Function*************************************************************

  Synopsis    [Returns current symbol.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Ver_StreamIsOkey( Ver_Stream_t * p )
{
    return !p->fStop;
}

/**Function*************************************************************

  Synopsis    [Returns current symbol.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char Ver_StreamScanChar( Ver_Stream_t * p )
{
    assert( !p->fStop );
    return *p->pBufferCur;
}

/**Function*************************************************************

  Synopsis    [Returns current symbol and moves to the next.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char Ver_StreamPopChar( Ver_Stream_t * p )
{
    assert( !p->fStop );
    // check if the new data should to be loaded
    if ( p->pBufferCur > p->pBufferStop )
        Ver_StreamReload( p );
    // check if there are symbols left
    if ( p->pBufferCur == p->pBufferEnd ) // end of file
    {
        p->fStop = 1;
        return -1;
    }
    // count the lines
    if ( *p->pBufferCur == '\n' )
        p->nLineCounter++;
    return *p->pBufferCur++;
}

/**Function*************************************************************

  Synopsis    [Skips the current symbol and all symbols from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_StreamSkipChars( Ver_Stream_t * p, char * pCharsToSkip )
{
    char * pChar, * pTemp;
    assert( !p->fStop );
    assert( pCharsToSkip != NULL );
    // check if the new data should to be loaded
    if ( p->pBufferCur > p->pBufferStop )
        Ver_StreamReload( p );
    // skip the symbols
    for ( pChar = p->pBufferCur; pChar < p->pBufferEnd; pChar++ )
    {
        // skip symbols as long as they are in the list
        for ( pTemp = pCharsToSkip; *pTemp; pTemp++ )
            if ( *pChar == *pTemp )
                break;
        if ( *pTemp == 0 ) // pChar is not found in the list
        {
            p->pBufferCur = pChar;
            return;
        }
        // count the lines
        if ( *pChar == '\n' )
            p->nLineCounter++;
    }
    // the file is finished or the last part continued 
    // through VER_OFFSET_SIZE chars till the end of the buffer
    if ( p->pBufferStop == p->pBufferEnd ) // end of file
    {
        p->fStop = 1;
        return;
    }
    printf( "Ver_StreamSkipSymbol() failed to parse the file \"%s\".\n", p->pFileName );
}

/**Function*************************************************************

  Synopsis    [Skips all symbols until encountering one from the list.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_StreamSkipToChars( Ver_Stream_t * p, char * pCharsToStop )
{
    char * pChar, * pTemp;
    assert( !p->fStop );
    assert( pCharsToStop != NULL );
    // check if the new data should to be loaded
    if ( p->pBufferCur > p->pBufferStop )
        Ver_StreamReload( p );
    // skip the symbols
    for ( pChar = p->pBufferCur; pChar < p->pBufferEnd; pChar++ )
    {
        // skip symbols as long as they are NOT in the list
        for ( pTemp = pCharsToStop; *pTemp; pTemp++ )
            if ( *pChar == *pTemp )
                break;
        if ( *pTemp == 0 ) // pChar is not found in the list
        {
            // count the lines
            if ( *pChar == '\n' )
                p->nLineCounter++;
            continue;
        }
        // the symbol is found - move position and return
        p->pBufferCur = pChar;
        return;
    }
    // the file is finished or the last part continued 
    // through VER_OFFSET_SIZE chars till the end of the buffer
    if ( p->pBufferStop == p->pBufferEnd ) // end of file
    {
        p->fStop = 1;
        return;
    }
    printf( "Ver_StreamSkipToSymbol() failed to parse the file \"%s\".\n", p->pFileName );
}

/**Function*************************************************************

  Synopsis    [Returns current word delimited by the set of symbols.]

  Description [Modifies the stream by inserting 0 at the first encounter
  of one of the symbols in the list.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Ver_StreamGetWord( Ver_Stream_t * p, char * pCharsToStop )
{
    char * pChar, * pTemp;
    if ( p->fStop )
        return NULL;
    assert( pCharsToStop != NULL );
    // check if the new data should to be loaded
    if ( p->pBufferCur > p->pBufferStop )
        Ver_StreamReload( p );
    // skip the symbols
    p->nChars = 0;
    for ( pChar = p->pBufferCur; pChar < p->pBufferEnd; pChar++ )
    {
        // skip symbols as long as they are NOT in the list
        for ( pTemp = pCharsToStop; *pTemp; pTemp++ )
            if ( *pChar == *pTemp )
                break;
        if ( *pTemp == 0 ) // pChar is not found in the list
        {
            p->pChars[p->nChars++] = *pChar;
            if ( p->nChars == VER_WORD_SIZE )
            {
                printf( "Ver_StreamGetWord(): The buffer size is exceeded.\n" );
                return NULL;
            }
            // count the lines
            if ( *pChar == '\n' )
                p->nLineCounter++;
            continue;
        }
        // the symbol is found - move the position, set the word end, return the word
        p->pBufferCur = pChar;
        p->pChars[p->nChars] = 0;
        return p->pChars;
    }
    // the file is finished or the last part continued 
    // through VER_OFFSET_SIZE chars till the end of the buffer
    if ( p->pBufferStop == p->pBufferEnd ) // end of file
    {
        p->fStop = 1;
        p->pChars[p->nChars] = 0;
        return p->pChars;
    }
    printf( "Ver_StreamGetWord() failed to parse the file \"%s\".\n", p->pFileName );
    return NULL;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Ver_StreamMove( Ver_Stream_t * p )
{
    if ( !strncmp(p->pBufferCur+1, "z_g_", 4) || !strncmp(p->pBufferCur+1, "co_g", 3) )
        while ( p->pBufferCur[0] != '(' )
            p->pBufferCur++;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

