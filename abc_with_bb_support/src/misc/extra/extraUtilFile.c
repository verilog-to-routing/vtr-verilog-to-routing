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
    FREE( pFileGen );
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
   return NULL;
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
    char * pDot;
    char * pUnd;
    char * pRes;
    
    // find the generic name of the file
    pRes = Extra_UtilStrsav( FileName );
    // find the pointer to the "." symbol in the file name
//  pUnd = strstr( FileName, "_" );
    pUnd = NULL;
    pDot = strstr( FileName, "." );
    if ( pUnd )
        pRes[pUnd - FileName] = 0;
    else if ( pDot )
        pRes[pDot - FileName] = 0;
    return pRes;
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
    pFile = fopen( pFileName, "r" );
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
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    pBuffer = ALLOC( char, nFileSize + 3 );
    fread( pBuffer, nFileSize, 1, pFile );
    // terminate the string with '\0'
    pBuffer[ nFileSize + 0] = '\n';
    pBuffer[ nFileSize + 1] = '\0';
    return pBuffer;
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
    int Remainder, nWords;
    int w, i;

    Remainder = (nBits%(sizeof(unsigned)*8));
    nWords    = (nBits/(sizeof(unsigned)*8)) + (Remainder>0);

    for ( w = nWords-1; w >= 0; w-- )
        for ( i = ((w == nWords-1 && Remainder)? Remainder-1: 31); i >= 0; i-- )
            fprintf( pFile, "%c", '0' + (int)((Sign[w] & (1<<i)) > 0) );

//  fprintf( pFile, "\n" );
}

/**Function*************************************************************

  Synopsis    [Reads the hex unsigned into the bit-string.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Extra_ReadHexadecimal( unsigned Sign[], char * pString, int nVars )
{
    int nWords, nDigits, Digit, k, c;
    nWords = Extra_TruthWordNum( nVars );
    for ( k = 0; k < nWords; k++ )
        Sign[k] = 0;
    // read the number from the string
    nDigits = (1 << nVars) / 4;
    if ( nDigits == 0 )
        nDigits = 1;
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
void Extra_PrintHex( FILE * pFile, unsigned uTruth, int nVars )
{
    int nMints, nDigits, Digit, k;

    // write the number into the file
    fprintf( pFile, "0x" );
    nMints  = (1 << nVars);
    nDigits = nMints / 4;
    for ( k = nDigits - 1; k >= 0; k-- )
    {
        Digit = ((uTruth >> (k * 4)) & 15);
        if ( Digit < 10 )
            fprintf( pFile, "%d", Digit );
        else
            fprintf( pFile, "%c", 'a' + Digit-10 );
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
        pTemp = ALLOC( char, strlen(pStrGiven) + strlen(pStrAdd) + 2 );
        sprintf( pTemp, "%s%s", pStrGiven, pStrAdd );
        free( pStrGiven );
    }
    else
        pTemp = Extra_UtilStrsav( pStrAdd );
    return pTemp;
}

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of static Functions                                            */
/*---------------------------------------------------------------------------*/


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


