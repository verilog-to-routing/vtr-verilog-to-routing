/**CFile****************************************************************

  FileName    [luckyRead.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Semi-canonical form computation package.]

  Synopsis    [Reading truth tables from file.]

  Author      [Jake]

  Date        [Started - August 2012]

***********************************************************************/

#include "luckyInt.h"

ABC_NAMESPACE_IMPL_START


// read/write/flip i-th bit of a bit string table:
static inline int     Abc_TruthGetBit( word * p, int i )         { return (int)(p[i>>6] >> (i & 63)) & 1;        }
static inline void    Abc_TruthSetBit( word * p, int i )         { p[i>>6] |= (((word)1)<<(i & 63));             }
static inline void    Abc_TruthXorBit( word * p, int i )         { p[i>>6] ^= (((word)1)<<(i & 63));             }

// read/write k-th digit d of a hexadecimal number:
static inline int     Abc_TruthGetHex( word * p, int k )         { return (int)(p[k>>4] >> ((k<<2) & 63)) & 15;  }
static inline void    Abc_TruthSetHex( word * p, int k, int d )  { p[k>>4] |= (((word)d)<<((k<<2) & 63));        }
static inline void    Abc_TruthXorHex( word * p, int k, int d )  { p[k>>4] ^= (((word)d)<<((k<<2) & 63));        }

// read one hex character
static inline int  Abc_TruthReadHexDigit( char HexChar )
{
    if ( HexChar >= '0' && HexChar <= '9' )
        return HexChar - '0';
    if ( HexChar >= 'A' && HexChar <= 'F' )
        return HexChar - 'A' + 10;
    if ( HexChar >= 'a' && HexChar <= 'f' )
        return HexChar - 'a' + 10;
    assert( 0 ); // not a hexadecimal symbol
    return -1; // return value which makes no sense
}

// write one hex character
static inline void Abc_TruthWriteHexDigit( FILE * pFile, int HexDigit )
{
    assert( HexDigit >= 0 && HexDigit < 16 );
    if ( HexDigit < 10 )
        fprintf( pFile, "%d", HexDigit );
    else
        fprintf( pFile, "%c", 'A' + HexDigit-10 );
}

// read one truth table in hexadecimal
static inline void Abc_TruthReadHex( word * pTruth, char * pString, int nVars )
{
    int nWords = (nVars < 7)? 1 : (1 << (nVars-6));
    int k, Digit, nDigits = (nVars < 7) ? (1 << (nVars-2)) : (nWords << 4);
    char EndSymbol;
    // skip the first 2 symbols if they are "0x"
    if ( pString[0] == '0' && pString[1] == 'x' )
        pString += 2;
    // get the last symbol
    EndSymbol = pString[nDigits];
    // the end symbol of the TT (the one immediately following hex digits)
    // should be one of the following: space, a new-line, or a zero-terminator
    // (note that on Windows symbols '\r' can be inserted before each '\n')
    assert( EndSymbol == ' ' || EndSymbol == '\n' || EndSymbol == '\r' || EndSymbol == '\0' );
    // read hexadecimal digits in the reverse order
    // (the last symbol in the string is the least significant digit)
    for ( k = 0; k < nDigits; k++ )
    {
        Digit = Abc_TruthReadHexDigit( pString[nDigits - 1 - k] );
        assert( Digit >= 0 && Digit < 16 );
        Abc_TruthSetHex( pTruth, k, Digit );
    }
}

// write one truth table in hexadecimal (do not add end-of-line!)
static inline void Abc_TruthWriteHex( FILE * pFile, word * pTruth, int nVars )
{
    int nDigits, Digit, k;
    // write hexadecimal digits in the reverse order
    // (the last symbol in the string is the least significant digit)
    nDigits = (1 << (nVars-2));
    for ( k = 0; k < nDigits; k++ )
    {
        Digit = Abc_TruthGetHex( pTruth, nDigits - 1 - k );
        assert( Digit >= 0 && Digit < 16 );
        Abc_TruthWriteHexDigit( pFile, Digit );
    }
}

// allocate and clear memory to store 'nTruths' truth tables of 'nVars' variables
static inline Abc_TtStore_t * Abc_TruthStoreAlloc( int nVars, int nFuncs )
{
    Abc_TtStore_t * p;
    int i;
    p = (Abc_TtStore_t *)malloc( sizeof(Abc_TtStore_t) );
    p->nVars  =  nVars;
    p->nWords = (nVars < 7) ? 1 : (1 << (nVars-6));
    p->nFuncs =  nFuncs;
    // alloc array of 'nFuncs' pointers to truth tables
    p->pFuncs = (word **)malloc( sizeof(word *) * p->nFuncs );
    // alloc storage for 'nFuncs' truth tables as one chunk of memory
    p->pFuncs[0] = (word *)calloc( sizeof(word), p->nFuncs * p->nWords );
    // split it up into individual truth tables
    for ( i = 1; i < p->nFuncs; i++ )
        p->pFuncs[i] = p->pFuncs[i-1] + p->nWords;
    return p;
}

// free memory previously allocated for storing truth tables
void Abc_TruthStoreFree( Abc_TtStore_t * p )
{
    free( p->pFuncs[0] );
    free( p->pFuncs );
    free( p );
}

// read file contents
static char * Abc_FileRead( char * pFileName )
{
    FILE * pFile;
    char * pBuffer;
    int nFileSize;
    int RetValue;
    pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for reading.\n", pFileName );
        return NULL;
    }
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    pBuffer = (char *)malloc( nFileSize + 3 );
    RetValue = fread( pBuffer, nFileSize, 1, pFile );
    // add several empty lines at the end
    // (these will be used to signal the end of parsing)
    pBuffer[ nFileSize + 0] = '\n';
    pBuffer[ nFileSize + 1] = '\n';
    // terminate the string with '\0'
    pBuffer[ nFileSize + 2] = '\0';
    fclose( pFile );
    return pBuffer;
}

// determine the number of variables by reading the first line
// determine the number of functions by counting the lines
static void Abc_TruthGetParams( char * pFileName, int * pnVars, int * pnTruths )
{
    char * pContents;
    int i, nVars, nLines;
    // prepare the output 
    if ( pnVars )
        *pnVars = 0;
    if ( pnTruths )
        *pnTruths = 0;
    // read data from file
    pContents = Abc_FileRead( pFileName );
    if ( pContents == NULL )
        return;
    // count the number of symbols before the first space or new-line
    // (note that on Windows symbols '\r' can be inserted before each '\n')
    for ( i = 0; pContents[i]; i++ )
        if ( pContents[i] == ' ' || pContents[i] == '\n' || pContents[i] == '\r' )
            break;
    if ( pContents[i] == 0 )
        printf( "Strange, the input file does not have spaces and new-lines...\n" );

    // account for the fact that truth tables may have "0x" at the beginning of each line
    if ( pContents[0] == '0' && pContents[1] == 'x' )
        i = i - 2;

    // determine the number of variables
    for ( nVars = 0; nVars < 32; nVars++ )
        if ( 4 * i == (1 << nVars) ) // the number of bits equal to the size of truth table
            break;
    if ( nVars < 2 || nVars > 16 )
    {
        printf( "Does not look like the input file contains truth tables...\n" );
        return;
    }
    if ( pnVars )
        *pnVars = nVars;

    // determine the number of functions by counting the lines
    nLines = 0;
    for ( i = 0; pContents[i]; i++ )
        nLines += (pContents[i] == '\n');
    if ( pnTruths )
        *pnTruths = nLines;
}

static Abc_TtStore_t * Abc_Create_TtSpore (char * pFileInput)
{
    int nVars, nTruths;
    Abc_TtStore_t * p;
    Abc_TruthGetParams( pFileInput, &nVars, &nTruths );
    p = Abc_TruthStoreAlloc( nVars, nTruths );
    return p;

}
// read truth tables from file
static void Abc_TruthStoreRead( char * pFileName, Abc_TtStore_t* p )
{
    char * pContents;
    int i, nLines;
    pContents = Abc_FileRead( pFileName );
    if ( pContents == NULL )
        return;
    // here it is assumed (without checking!) that each line of the file 
    // begins with a string of hexadecimal chars followed by space

    // the file will be read till the first empty line (pContents[i] == '\n')
    // (note that Abc_FileRead() added several empty lines at the end of the file contents)
    for ( nLines = i = 0; pContents[i] != '\n'; )
    {
        // read one line
        Abc_TruthReadHex( p->pFuncs[nLines++], &pContents[i], p->nVars );
        // skip till after the end-of-line symbol
        // (note that end-of-line symbol is also skipped)
        while ( pContents[i++] != '\n' );
    }
    // adjust the number of functions read 
    // (we may have allocated more storage because some lines in the file were empty)
    assert( p->nFuncs >= nLines );
    p->nFuncs = nLines;
}

// write truth tables into file
// (here we write one symbol at a time - it can be optimized by writing 
// each truth table into a string and then writing the string into a file)
static void Abc_TruthStoreWrite( char * pFileName, Abc_TtStore_t * p )
{
    FILE * pFile;
    int i;
    pFile = fopen( pFileName, "wb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return;
    }
    for ( i = 0; i < p->nFuncs; i++ )
    {
        Abc_TruthWriteHex( pFile, p->pFuncs[i], p->nVars );
        fprintf( pFile, "\n" );
    }
    fclose( pFile );
}

static void WriteToFile(char * pFileName, Abc_TtStore_t * p, word* a)
{
    FILE * pFile;
    int i;
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return;
    }
    for ( i = 0; i < p->nFuncs; i++ )
    {
        Abc_TruthWriteHex( pFile, &a[i], p->nVars );
        fprintf( pFile, "\n" );
    }
    fclose( pFile );
}

static void WriteToFile1(char * pFileName, Abc_TtStore_t * p, word** a)
{
    FILE * pFile;
    int i,j;
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return;
    }
    for ( i = 0; i < p->nFuncs; i++ )
    {
        fprintf( pFile, "0" );
        fprintf( pFile, "x" );
        for ( j=p->nWords-1; j >= 0; j-- )
            Abc_TruthWriteHex( pFile, &a[i][j], p->nVars );
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, "\n" );
    fclose( pFile );
}
static void WriteToFile2(char * pFileName, Abc_TtStore_t * p, word* a)
{
    FILE * pFile;
    int i,j;
    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" for writing.\n", pFileName );
        return;
    }
    for ( i = 0; i < p->nFuncs; i++ )
    {
        fprintf( pFile, "0" );
        fprintf( pFile, "x" );
        for ( j=p->nWords-1; j >= 0; j-- )
            Abc_TruthWriteHex( pFile, a+i, p->nVars );
        fprintf( pFile, "\n" );
    }
    fprintf( pFile, "\n" );
    fclose( pFile );
}


Abc_TtStore_t * setTtStore(char * pFileInput)
{
    int nVars, nTruths;
    Abc_TtStore_t * p;
    // figure out how many truth table and how many variables
    Abc_TruthGetParams( pFileInput, &nVars, &nTruths );
    // allocate data-structure
    p = Abc_TruthStoreAlloc( nVars, nTruths );

    Abc_TruthStoreRead( pFileInput, p );
    return p;
}


ABC_NAMESPACE_IMPL_END
