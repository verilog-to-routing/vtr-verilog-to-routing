/**CFile****************************************************************

  FileName    [extraUtilReader.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [File reading utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraUtilReader.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include <stdio.h>
#include "extra.h"
#include "misc/vec/vec.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define EXTRA_BUFFER_SIZE        4*1048576    // 1M   - size of the data chunk stored in memory
#define EXTRA_OFFSET_SIZE           4096    // 4K   - load new data when less than this is left

#define EXTRA_MINIMUM(a,b)       (((a) < (b))? (a) : (b))

struct Extra_FileReader_t_
{
    // the input file
    char *           pFileName;     // the input file name
    FILE *           pFile;         // the input file pointer
    int              nFileSize;     // the total number of bytes in the file
    int              nFileRead;     // the number of bytes currently read from file
    // info about processing different types of input chars
    char             pCharMap[256]; // the character map
    // temporary storage for data 
    char *           pBuffer;       // the buffer
    int              nBufferSize;   // the size of the buffer
    char *           pBufferCur;    // the current reading position
    char *           pBufferEnd;    // the first position not used by currently loaded data
    char *           pBufferStop;   // the position where loading new data will be done
    // tokens given to the user
    Vec_Ptr_t *      vTokens;       // the vector of tokens returned to the user
    Vec_Int_t *      vLines;        // the vector of line numbers for each token
    int              nLineCounter;  // the counter of lines processed
    // status of the parser
    int              fStop;         // this flag goes high when the end of file is reached
};

// character types
typedef enum { 
    EXTRA_CHAR_COMMENT,  // a character that begins the comment
    EXTRA_CHAR_NORMAL,   // a regular character
    EXTRA_CHAR_STOP,     // a character that delimits a series of tokens
    EXTRA_CHAR_CLEAN     // a character that should be cleaned
} Extra_CharType_t;

// the static functions
static void * Extra_FileReaderGetTokens_int( Extra_FileReader_t * p );
static void Extra_FileReaderReload( Extra_FileReader_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts the file reader.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Extra_FileReader_t * Extra_FileReaderAlloc( char * pFileName, 
    char * pCharsComment, char * pCharsStop, char * pCharsClean )
{
    Extra_FileReader_t * p;
    FILE * pFile;
    char * pChar;
    int nCharsToRead;
    int RetValue;
    // check if the file can be opened
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Extra_FileReaderAlloc(): Cannot open input file \"%s\".\n", pFileName );
        return NULL;
    }
    // start the file reader    
    p = ABC_ALLOC( Extra_FileReader_t, 1 );
    memset( p, 0, sizeof(Extra_FileReader_t) );
    p->pFileName   = pFileName;
    p->pFile       = pFile;
    // set the character map
    memset( p->pCharMap, EXTRA_CHAR_NORMAL, 256 );
    for ( pChar = pCharsComment; *pChar; pChar++ )
        p->pCharMap[(unsigned char)*pChar] = EXTRA_CHAR_COMMENT;
    for ( pChar = pCharsStop; *pChar; pChar++ )
        p->pCharMap[(unsigned char)*pChar] = EXTRA_CHAR_STOP;
    for ( pChar = pCharsClean; *pChar; pChar++ )
        p->pCharMap[(unsigned char)*pChar] = EXTRA_CHAR_CLEAN;
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    p->nFileSize = ftell( pFile );  
    rewind( pFile ); 
    // allocate the buffer
    p->pBuffer = ABC_ALLOC( char, EXTRA_BUFFER_SIZE+1 );
    p->nBufferSize = EXTRA_BUFFER_SIZE;
    p->pBufferCur  = p->pBuffer;
    // determine how many chars to read
    nCharsToRead = EXTRA_MINIMUM(p->nFileSize, EXTRA_BUFFER_SIZE);
    // load the first part into the buffer
    RetValue = fread( p->pBuffer, nCharsToRead, 1, p->pFile );
    p->nFileRead = nCharsToRead;
    // set the ponters to the end and the stopping point
    p->pBufferEnd  = p->pBuffer + nCharsToRead;
    p->pBufferStop = (p->nFileRead == p->nFileSize)? p->pBufferEnd : p->pBuffer + EXTRA_BUFFER_SIZE - EXTRA_OFFSET_SIZE;
    // start the arrays
    p->vTokens = Vec_PtrAlloc( 100 );
    p->vLines = Vec_IntAlloc( 100 );
    p->nLineCounter = 1; // 1-based line counting
    return p;
}

/**Function*************************************************************

  Synopsis    [Stops the file reader.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_FileReaderFree( Extra_FileReader_t * p )
{
    if ( p->pFile )
        fclose( p->pFile );
    ABC_FREE( p->pBuffer );
    Vec_PtrFree( p->vTokens );
    Vec_IntFree( p->vLines );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Returns the file size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_FileReaderGetFileName( Extra_FileReader_t * p )
{
    return p->pFileName;
}

/**Function*************************************************************

  Synopsis    [Returns the file size.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_FileReaderGetFileSize( Extra_FileReader_t * p )
{
    return p->nFileSize;
}

/**Function*************************************************************

  Synopsis    [Returns the current reading position.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_FileReaderGetCurPosition( Extra_FileReader_t * p )
{
    return p->nFileRead - (p->pBufferEnd - p->pBufferCur);
}

/**Function*************************************************************

  Synopsis    [Returns the line number for the given token.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_FileReaderGetLineNumber( Extra_FileReader_t * p, int iToken )
{
    assert( iToken >= 0 && iToken < p->vTokens->nSize );
    return p->vLines->pArray[iToken];
}


/**Function*************************************************************

  Synopsis    [Returns the next set of tokens.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Extra_FileReaderGetTokens( Extra_FileReader_t * p )
{
    Vec_Ptr_t * vTokens;
    while ( (vTokens = (Vec_Ptr_t *)Extra_FileReaderGetTokens_int( p )) )
        if ( vTokens->nSize > 0 )
            break;
    return vTokens;
}

/**Function*************************************************************

  Synopsis    [Returns the next set of tokens.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Extra_FileReaderGetTokens_int( Extra_FileReader_t * p )
{
    char * pChar;
    int fTokenStarted, MapValue;
    if ( p->fStop )
        return NULL;
    // reset the token info
    p->vTokens->nSize = 0;
    p->vLines->nSize = 0;
    fTokenStarted = 0;
    // check if the new data should to be loaded
    if ( p->pBufferCur > p->pBufferStop )
        Extra_FileReaderReload( p );

//    printf( "%d\n", p->pBufferEnd - p->pBufferCur );

    // process the string starting from the current position
    for ( pChar = p->pBufferCur; pChar < p->pBufferEnd; pChar++ )
    {
        // count the lines
        if ( *pChar == '\n' )
            p->nLineCounter++;
        // switch depending on the character
        MapValue = p->pCharMap[(int)*pChar];

//        printf( "Char value = %d. Map value = %d.\n", *pChar, MapValue );


        switch ( MapValue )
        {
            case EXTRA_CHAR_COMMENT:
                if ( *pChar != '/' || *(pChar+1) == '/' ) 
                { // dealing with the need to have // as a comment
                    // if the token was being written, stop it
                    if ( fTokenStarted )
                        fTokenStarted = 0;
                    // eraze the comment till the end of line
                    while ( *pChar != '\n' )
                    {
                        *pChar++ = 0;
                        if ( pChar == p->pBufferEnd )
                        {   // this failure is due to the fact the comment continued 
                            // through EXTRA_OFFSET_SIZE chars till the end of the buffer
                            printf( "Extra_FileReader failed to parse the file \"%s\".\n", p->pFileName );
                            return NULL;
                        }
                    }
                    pChar--;
                    break;
                }
                // otherwise it is a normal character
            case EXTRA_CHAR_NORMAL:
                if ( !fTokenStarted )
                {
                    Vec_PtrPush( p->vTokens, pChar );
                    Vec_IntPush( p->vLines, p->nLineCounter );
                    fTokenStarted = 1;
                }
                break;
            case EXTRA_CHAR_STOP:
                if ( fTokenStarted )
                    fTokenStarted = 0;
                *pChar = 0;
                // prepare before leaving
                p->pBufferCur = pChar + 1;
                return p->vTokens;
            case EXTRA_CHAR_CLEAN:
                if ( fTokenStarted )
                    fTokenStarted = 0;
                *pChar = 0;
                break;
            default:
                assert( 0 );
        }
    }
    // the file is finished or the last part continued 
    // through EXTRA_OFFSET_SIZE chars till the end of the buffer
    if ( p->pBufferStop == p->pBufferEnd ) // end of file
    {
        *pChar = 0;
        p->fStop = 1;
        return p->vTokens;
    }
    printf( "Extra_FileReader failed to parse the file \"%s\".\n", p->pFileName );
/*
    {
        int i;
        for ( i = 0; i < p->vTokens->nSize; i++ )
            printf( "%s ", p->vTokens->pArray[i] );
        printf( "\n" );
    }
*/
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Loads new data into the file reader.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_FileReaderReload( Extra_FileReader_t * p )
{
    int nCharsUsed, nCharsToRead;
    int RetValue;
    assert( !p->fStop );
    assert( p->pBufferCur > p->pBufferStop );
    assert( p->pBufferCur < p->pBufferEnd );
    // figure out how many chars are still not processed
    nCharsUsed = p->pBufferEnd - p->pBufferCur;
    // move the remaining data to the beginning of the buffer
    memmove( p->pBuffer, p->pBufferCur, nCharsUsed );
    p->pBufferCur = p->pBuffer;
    // determine how many chars we will read
    nCharsToRead = EXTRA_MINIMUM( p->nBufferSize - nCharsUsed, p->nFileSize - p->nFileRead );
    // read the chars
    RetValue = fread( p->pBuffer + nCharsUsed, nCharsToRead, 1, p->pFile );
    p->nFileRead += nCharsToRead;
    // set the ponters to the end and the stopping point
    p->pBufferEnd  = p->pBuffer + nCharsUsed + nCharsToRead;
    p->pBufferStop = (p->nFileRead == p->nFileSize)? p->pBufferEnd : p->pBuffer + EXTRA_BUFFER_SIZE - EXTRA_OFFSET_SIZE;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

