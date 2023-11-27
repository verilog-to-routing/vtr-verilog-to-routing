/**CFile****************************************************************

  FileName    [extraUtilFile.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [extra]

  Synopsis    [File management utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: extraUtilFile.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "extra.h"

ABC_NAMESPACE_IMPL_START


/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/


/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function*************************************************************

  Synopsis    [Tries to find a file name with a different extension.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_FileGetSimilarName( char * pFileNameWrong, char * pS1, char * pS2, char * pS3, char * pS4, char * pS5 )
{
    FILE * pFile;
    char * pFileNameOther;
    char * pFileGen;

    if ( pS1 == NULL )
        return NULL;

    // get the generic file name
    pFileGen = Extra_FileNameGeneric( pFileNameWrong );
    pFileNameOther = Extra_FileNameAppend( pFileGen, pS1 );
    pFile = fopen( pFileNameOther, "r" );
    if ( pFile == NULL && pS2 )
    { // try one more
        pFileNameOther = Extra_FileNameAppend( pFileGen, pS2 );
        pFile = fopen( pFileNameOther, "r" );
        if ( pFile == NULL && pS3 )
        { // try one more
            pFileNameOther = Extra_FileNameAppend( pFileGen, pS3 );
            pFile = fopen( pFileNameOther, "r" );
            if ( pFile == NULL && pS4 )
            { // try one more
                pFileNameOther = Extra_FileNameAppend( pFileGen, pS4 );
                pFile = fopen( pFileNameOther, "r" );
                if ( pFile == NULL && pS5 )
                { // try one more
                    pFileNameOther = Extra_FileNameAppend( pFileGen, pS5 );
                    pFile = fopen( pFileNameOther, "r" );
                }
            }
        }
    }
    ABC_FREE( pFileGen );
    if ( pFile )
    {
        fclose( pFile );
        return pFileNameOther;
    }
    // did not find :(
    return NULL;            
}

/**Function*************************************************************

  Synopsis    [Returns the pointer to the file extension.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_FileNameExtension( char * FileName )
{
    char * pDot;
    // find the last "dot" in the file name, if it is present
    for ( pDot = FileName + strlen(FileName)-1; pDot >= FileName; pDot-- )
        if ( *pDot == '.' )
            return pDot + 1;
   return FileName;
}

/**Function*************************************************************

  Synopsis    [Returns the composite name of the file.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_FileNameAppend( char * pBase, char * pSuffix )
{
    static char Buffer[500];
    assert( strlen(pBase) + strlen(pSuffix) < 500 );
    sprintf( Buffer, "%s%s", pBase, pSuffix );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_FileNameGeneric( char * FileName )
{
    char * pDot, * pRes;
    pRes = Extra_UtilStrsav( FileName );
    if ( (pDot = strrchr( pRes, '.' )) )
        *pDot = 0;
    return pRes;
}

/**Function*************************************************************

  Synopsis    [Returns the composite name of the file.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_FileNameGenericAppend( char * pBase, char * pSuffix )
{
    static char Buffer[1000];
    char * pDot;
    assert( strlen(pBase) + strlen(pSuffix) < 1000 );
    strcpy( Buffer, pBase );
    if ( (pDot = strrchr( Buffer, '.' )) )
        *pDot = 0;
    strcat( Buffer, pSuffix );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_FileNameCorrectPath( char * FileName )
{
    char * pStart;
    if ( FileName )
        for ( pStart = FileName; *pStart; pStart++ )
            if ( *pStart == '>' || *pStart == '\\' )
                *pStart = '/';
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_FileNameWithoutPath( char * FileName )
{
    char * pRes;
    for ( pRes = FileName + strlen(FileName) - 1; pRes >= FileName; pRes-- )
        if ( *pRes == '\\' || *pRes == '/' )
            return pRes + 1;
    return FileName;
}
char * Extra_FilePathWithoutName( char * FileName )
{
    char * pRes;
    FileName = Abc_UtilStrsav( FileName );
    for ( pRes = FileName + strlen(FileName) - 1; pRes >= FileName; pRes-- )
        if ( *pRes == '\\' || *pRes == '/' )
        {
           pRes[1] = '\0';
           Extra_FileNameCorrectPath( FileName );
           return FileName;
        }
    ABC_FREE( FileName );
    return NULL;
}
char * Extra_FileInTheSameDir( char * pPathFile, char * pFileName )
{
    static char pBuffer[1000]; char * pThis;
    assert( strlen(pPathFile) + strlen(pFileName) < 990 );
    memmove( pBuffer, pPathFile, strlen(pPathFile) );
    for ( pThis = pBuffer + strlen(pPathFile) - 1; pThis >= pBuffer; pThis-- )
        if ( *pThis == '\\' || *pThis == '/' )
            break;
    memmove( ++pThis, pFileName, strlen(pFileName) );
    pThis[strlen(pFileName)] = '\0';
    return pBuffer;
}
char * Extra_FileDesignName( char * pFileName )
{
    char * pBeg, * pEnd, * pStore, * pCur;
    // find the first dot
    for ( pEnd = pFileName; *pEnd; pEnd++ )
        if ( *pEnd == '.' )
            break;
    // find the first char
    for ( pBeg = pEnd - 1; pBeg >= pFileName; pBeg-- )
        if ( !((*pBeg >= 'a' && *pBeg <= 'z') || (*pBeg >= 'A' && *pBeg <= 'Z') || (*pBeg >= '0' && *pBeg <= '9') || *pBeg == '_') )
            break;
    pBeg++;
    // fill up storage
    pStore = ABC_ALLOC( char, pEnd - pBeg + 1 );
    for ( pCur = pStore; pBeg < pEnd; pBeg++, pCur++ )
        *pCur = *pBeg;
    *pCur = 0;
    return pStore;
}

/**Function*************************************************************

  Synopsis    [Returns the file size.]

  Description [The file should be closed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_FileCheck( char * pFileName )
{
    FILE * pFile;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Extra_FileCheck():  File \"%s\" does not exist.\n", pFileName );
        return 0;
    }
    fseek( pFile, 0, SEEK_END );
    if ( ftell( pFile ) == 0 )
        printf( "Extra_FileCheck():  File \"%s\" is empty.\n", pFileName );
    fclose( pFile );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns the file size.]

  Description [The file should be closed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_FileSize( char * pFileName )
{
    FILE * pFile;
    int nFileSize;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Extra_FileSize(): The file is unavailable (absent or open).\n" );
        return 0;
    }
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile ); 
    fclose( pFile );
    return nFileSize;
}


/**Function*************************************************************

  Synopsis    [Read the file into the internal buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_FileRead( FILE * pFile )
{
    int nFileSize;
    char * pBuffer;
    int RetValue;
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    pBuffer = ABC_ALLOC( char, nFileSize + 3 );
    RetValue = fread( pBuffer, nFileSize, 1, pFile );
    // terminate the string with '\0'
    pBuffer[ nFileSize + 0] = '\n';
    pBuffer[ nFileSize + 1] = '\0';
    return pBuffer;
}
char * Extra_FileRead2( FILE * pFile, FILE * pFile2 )
{
    char * pBuffer;
    int nSize, nSize2;
    int RetValue;
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nSize = ftell( pFile );  
    rewind( pFile ); 
    // get the file size, in bytes
    fseek( pFile2, 0, SEEK_END );  
    nSize2 = ftell( pFile2 );  
    rewind( pFile2 ); 
    // load the contents of the file into memory
    pBuffer = ABC_ALLOC( char, nSize + nSize2 + 3 );
    RetValue = fread( pBuffer,         nSize,  1, pFile );
    RetValue = fread( pBuffer + nSize, nSize2, 1, pFile2 );
    // terminate the string with '\0'
    pBuffer[ nSize + nSize2 + 0] = '\n';
    pBuffer[ nSize + nSize2 + 1] = '\0';
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    [Read the file into the internal buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_FileReadContents( char * pFileName )
{
    FILE * pFile;
    char * pBuffer;
    pFile = fopen( pFileName, "rb" );
    pBuffer = pFile ? Extra_FileRead( pFile ) : NULL;
    if ( pFile )  fclose( pFile );
    return pBuffer;
}
char * Extra_FileReadContents2( char * pFileName, char * pFileName2 )
{
    FILE * pFile, * pFile2;
    char * pBuffer;
    pFile  = fopen( pFileName, "rb" );
    pFile2 = fopen( pFileName2, "rb" );
    pBuffer = (pFile && pFile2) ? Extra_FileRead2( pFile, pFile2 ) : NULL;
    if ( pFile )  fclose( pFile );
    if ( pFile2 ) fclose( pFile2 );
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    [Returns one if the file has a given extension.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_FileIsType( char * pFileName, char * pS1, char * pS2, char * pS3 )
{
    int lenS, lenF = strlen(pFileName);
    lenS = pS1 ? strlen(pS1) : 0;
    if ( lenS && lenF > lenS && !strncmp( pFileName+lenF-lenS, pS1, lenS ) )
        return 1;
    lenS = pS2 ? strlen(pS2) : 0;
    if ( lenS && lenF > lenS && !strncmp( pFileName+lenF-lenS, pS2, lenS ) )
        return 1;
    lenS = pS3 ? strlen(pS3) : 0;
    if ( lenS && lenF > lenS && !strncmp( pFileName+lenF-lenS, pS3, lenS ) )
        return 1;
    return 0;
}

/**Function*************************************************************

  Synopsis    [Returns the time stamp.]

  Description [The file should be closed.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_TimeStamp()
{
    static char Buffer[100];
    char * TimeStamp;
    time_t ltime;
    // get the current time
    time( &ltime );
    TimeStamp = asctime( localtime( &ltime ) );
    TimeStamp[ strlen(TimeStamp) - 1 ] = 0;
    strcpy( Buffer, TimeStamp );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Extra_ReadBinary( char * Buffer )
{
    unsigned Result;
    int i;

    Result = 0;
    for ( i = 0; Buffer[i]; i++ )
        if ( Buffer[i] == '0' || Buffer[i] == '1' )
            Result = Result * 2 + Buffer[i] - '0';
        else
        {
            assert( 0 );
        }
    return Result;
}

/**Function*************************************************************

  Synopsis    [Prints the bit string.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_PrintBinary( FILE * pFile, unsigned Sign[], int nBits )
{
    int i;
    for ( i = nBits-1; i >= 0; i-- )
        fprintf( pFile, "%c", '0' + Abc_InfoHasBit(Sign, i) );
//    fprintf( pFile, "\n" );
}
void Extra_PrintBinary2( FILE * pFile, unsigned Sign[], int nBits )
{
    int i;
    for ( i = 0; i < nBits; i++ )
        fprintf( pFile, "%c", '0' + Abc_InfoHasBit(Sign, i) );
//    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Reads the hex unsigned into the bit-string.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_ReadHex( unsigned Sign[], char * pString, int nDigits )
{
    int Digit, k, c;
    for ( k = 0; k < nDigits; k++ )
    {
        c = nDigits-1-k;
        if ( pString[c] >= '0' && pString[c] <= '9' )
            Digit = pString[c] - '0';
        else if ( pString[c] >= 'A' && pString[c] <= 'F' )
            Digit = pString[c] - 'A' + 10;
        else if ( pString[c] >= 'a' && pString[c] <= 'f' )
            Digit = pString[c] - 'a' + 10;
        else { assert( 0 ); return 0; }
        Sign[k/8] |= ( (Digit & 15) << ((k%8) * 4) );
    }
    return 1;
}
int Extra_ReadHexadecimal( unsigned Sign[], char * pString, int nVars )
{
    int nWords, nDigits, k;
    nWords = Extra_TruthWordNum( nVars );
    for ( k = 0; k < nWords; k++ )
        Sign[k] = 0;
    // read the number from the string
    nDigits = (1 << nVars) / 4;
    if ( nDigits == 0 )
        nDigits = 1;
    Extra_ReadHex( Sign, pString, nDigits );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Prints the hex unsigned into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_PrintHexadecimal( FILE * pFile, unsigned Sign[], int nVars )
{
    int nDigits, Digit, k;
    // write the number into the file
    nDigits = (1 << nVars) / 4;
    for ( k = nDigits - 1; k >= 0; k-- )
    {
        Digit = ((Sign[k/8] >> ((k%8) * 4)) & 15);
        if ( Digit < 10 )
            fprintf( pFile, "%d", Digit );
        else
            fprintf( pFile, "%c", 'a' + Digit-10 );
    }
//    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Prints the hex unsigned into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_PrintHexadecimalString( char * pString, unsigned Sign[], int nVars )
{
    int nDigits, Digit, k;
    if ( nVars == 0 && !(Sign[0] & 1) ) { sprintf(pString, "0"); return; } // const0
    if ( nVars == 0 &&  (Sign[0] & 1) ) { sprintf(pString, "1"); return; } // const1
    if ( nVars == 1 &&  (Sign[0] & 1) ) { sprintf(pString, "1"); return; } // inverter
    if ( nVars == 1 && !(Sign[0] & 1) ) { sprintf(pString, "2"); return; } // buffer
    // write the number into the file
    nDigits = (1 << nVars) / 4;
    for ( k = nDigits - 1; k >= 0; k-- )
    {
        Digit = ((Sign[k/8] >> ((k%8) * 4)) & 15);
        if ( Digit < 10 )
            *pString++ = '0' + Digit;
        else
            *pString++ = 'a' + Digit-10;
    }
//    fprintf( pFile, "\n" );
    *pString = 0;
}

/**Function*************************************************************

  Synopsis    [Prints the hex unsigned into a file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_PrintHex( FILE * pFile, unsigned * pTruth, int nVars )
{
    int nMints, nDigits, Digit, k;

    // write the number into the file
    fprintf( pFile, "0x" );
    nMints  = (1 << nVars);
    nDigits = nMints / 4 + ((nMints % 4) > 0);
    for ( k = nDigits - 1; k >= 0; k-- )
    {
        Digit = ((pTruth[k/8] >> (k * 4)) & 15);
        if ( Digit < 10 )
            fprintf( pFile, "%d", Digit );
        else
            fprintf( pFile, "%c", 'A' + Digit-10 );
    }
//    fprintf( pFile, "\n" );
}
void Extra_PrintHex2( FILE * pFile, unsigned * pTruth, int nVars )
{
    int nMints, nDigits, Digit, k;

    // write the number into the file
    //fprintf( pFile, "0x" );
    nMints  = (1 << nVars);
    nDigits = nMints / 4 + ((nMints % 4) > 0);
    for ( k = nDigits - 1; k >= 0; k-- )
    {
        Digit = ((pTruth[k/8] >> (k * 4)) & 15);
        if ( Digit < 10 )
            fprintf( pFile, "%d", Digit );
        else
            fprintf( pFile, "%c", 'A' + Digit-10 );
    }
//    fprintf( pFile, "\n" );
}
void Extra_PrintHexReverse( FILE * pFile, unsigned * pTruth, int nVars )
{
    int nMints, nDigits, Digit, k;

    // write the number into the file
    fprintf( pFile, "0x" );
    nMints  = (1 << nVars);
    nDigits = nMints / 4 + ((nMints % 4) > 0);
    for ( k = 0; k < nDigits; k++ )
    {
        Digit = ((pTruth[k/8] >> (k * 4)) & 15);
        if ( Digit < 10 )
            fprintf( pFile, "%d", Digit );
        else
            fprintf( pFile, "%c", 'A' + Digit-10 );
    }
//    fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Returns the composite name of the file.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_PrintSymbols( FILE * pFile, char Char, int nTimes, int fPrintNewLine )
{
    int i;
    for ( i = 0; i < nTimes; i++ )
        printf( "%c", Char );
    if ( fPrintNewLine )
        printf( "\n" );
}

/**Function*************************************************************

  Synopsis    [Appends the string.]

  Description [Assumes that the given string (pStrGiven) has been allocated
  before using malloc(). The additional string has not been allocated.
  Allocs more root, appends the additional part, frees the old given string.]

  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Extra_StringAppend( char * pStrGiven, char * pStrAdd )
{
    char * pTemp;
    if ( pStrGiven )
    {
        pTemp = ABC_ALLOC( char, strlen(pStrGiven) + strlen(pStrAdd) + 2 );
        sprintf( pTemp, "%s%s", pStrGiven, pStrAdd );
        ABC_FREE( pStrGiven );
    }
    else
        pTemp = Extra_UtilStrsav( pStrAdd );
    return pTemp;
}

/**Function*************************************************************

  Synopsis    [Only keep characters belonging to the second string.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_StringClean( char * pStrGiven, char * pCharKeep )
{
    char * pTemp, * pChar, * pSave = pStrGiven;
    for ( pTemp = pStrGiven; *pTemp; pTemp++ )
    {
        for ( pChar = pCharKeep; *pChar; pChar++ )
            if ( *pTemp == *pChar )
                break;
        if ( *pChar == 0 )
            continue;
        *pSave++ = *pTemp;
    }
    *pSave = 0;
}

/**Function*************************************************************

  Synopsis    [String comparison procedure.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_StringCompare( const char * pp1, const char * pp2 )
{
    return strcmp(*(char **)pp1, *(char **)pp2);
}

/**Function*************************************************************

  Synopsis    [Sorts lines in the file alphabetically.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_FileSort( char * pFileName, char * pFileNameOut )
{
    FILE * pFile;
    char * pContents;
    char ** pLines;
    int i, nLines, Begin;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Extra_FileSort(): Cannot open file \"%s\".\n", pFileName );
        return;
    }
    pContents = Extra_FileRead( pFile );
    fclose( pFile );
    if ( pContents == NULL )
    {
        printf( "Extra_FileSort(): Cannot read contents of file \"%s\".\n", pFileName );
        return;
    }
    // count end of lines
    for ( nLines = 0, i = 0; pContents[i]; i++ )
        nLines += (pContents[i] == '\n');
    // break the file into lines
    pLines = (char **)malloc( sizeof(char *) * nLines );
    Begin = 0;
    for ( nLines = 0, i = 0; pContents[i]; i++ )
        if ( pContents[i] == '\n' )
        {
            pContents[i] = 0;
            pLines[nLines++] = pContents + Begin;
            Begin = i + 1;
        }
    // sort the lines
    qsort( pLines, (size_t)nLines, sizeof(char *), (int(*)(const void *,const void *))Extra_StringCompare );
    // write a new file
    pFile = fopen( pFileNameOut, "wb" );
    for ( i = 0; i < nLines; i++ )
        if ( pLines[i][0] )
            fprintf( pFile, "%s\n", pLines[i] );
    fclose( pFile );
    // cleanup
    free( pLines );
    free( pContents );
    // report the result
    printf( "The file after sorting is \"%s\".\n", pFileNameOut );
}


/**Function*************************************************************

  Synopsis    [Appends line number in the end.]

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
void Extra_FileLineNumAdd( char * pFileName, char * pFileNameOut )
{
    char Buffer[1000];
    FILE * pFile;
    FILE * pFile2;
    int iLine;
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Extra_FileLineNumAdd(): Cannot open file \"%s\".\n", pFileName );
        return;
    }
    pFile2 = fopen( pFileNameOut, "wb" );
    if ( pFile2 == NULL )
    {
        fclose( pFile );
        printf( "Extra_FileLineNumAdd(): Cannot open file \"%s\".\n", pFileNameOut );
        return;
    }
    for ( iLine = 0; fgets( Buffer, 1000, pFile ); iLine++ )
    {
        sprintf( Buffer + strlen(Buffer) - 2, "%03d\n%c", iLine, 0 );
        fputs( Buffer, pFile2 );
    }
    fclose( pFile );
    fclose( pFile2 );
    // report the result
    printf( "The resulting file is \"%s\".\n", pFileNameOut );
}

/**Function*************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

***********************************************************************/
/*
int main( int argc, char ** argv )
{
    if ( argc == 2 )
        Extra_FileSort( argv[1], Extra_FileNameAppend(argv[1], "_sorted") );
    else
        printf( "%s: Wrong number of command line arguments.\n", argv[0] );
    return 1;
}
*/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

