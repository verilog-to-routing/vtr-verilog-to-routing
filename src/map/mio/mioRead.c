/**CFile****************************************************************

  FileName    [mioRead.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [File reading/writing for technology mapping.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - September 8, 2003.]

  Revision    [$Id: mioRead.c,v 1.9 2004/10/19 06:40:16 satrajit Exp $]

***********************************************************************/

#include <ctype.h>
#include "mioInt.h"
#include "base/io/ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

static Mio_Library_t * Mio_LibraryReadOne( char * FileName, int fExtendedFormat, st__table * tExcludeGate, int fVerbose );
       Mio_Library_t * Mio_LibraryReadBuffer( char * pBuffer, int fExtendedFormat, st__table * tExcludeGate, int fVerbose );
static int             Mio_LibraryReadInternal( Mio_Library_t * pLib, char * pBuffer, int fExtendedFormat, st__table * tExcludeGate, int fVerbose );
static Mio_Gate_t *    Mio_LibraryReadGate( char ** ppToken, int fExtendedFormat );
static Mio_Pin_t *     Mio_LibraryReadPin( char ** ppToken, int fExtendedFormat );
static char *          chomp( char *s );
static void            Mio_LibraryDetectSpecialGates( Mio_Library_t * pLib );
static void            Io_ReadFileRemoveComments( char * pBuffer, int * pnDots, int * pnLines );

/**Function*************************************************************

  Synopsis    [Read the genlib type of library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Library_t * Mio_LibraryRead( char * FileName, char * pBuffer, char * ExcludeFile, int fVerbose )
{
    Mio_Library_t * pLib;
    int num;
    char * pBufferCopy;

    st__table * tExcludeGate = 0;

    if ( ExcludeFile )
    {
        tExcludeGate = st__init_table(strcmp, st__strhash);
        if ( (num = Mio_LibraryReadExclude( ExcludeFile, tExcludeGate )) == -1 )
        {
            st__free_table( tExcludeGate );
            tExcludeGate = 0;
            return 0;
        }
        fprintf ( stdout, "Read %d gates from exclude file\n", num );
    }

    pBufferCopy = Abc_UtilStrsav(pBuffer);
    if ( pBuffer == NULL )
        pLib = Mio_LibraryReadOne( FileName, 0, tExcludeGate, fVerbose );       // try normal format first ..
    else
    {
        pLib = Mio_LibraryReadBuffer( pBuffer, 0, tExcludeGate, fVerbose );       // try normal format first ..
        if ( pLib )
            pLib->pName = Abc_UtilStrsav( Extra_FileNameGenericAppend(FileName, ".genlib") );
    }
    if ( pLib == NULL )
    {
        if ( pBuffer == NULL )
            pLib = Mio_LibraryReadOne( FileName, 1, tExcludeGate, fVerbose );       // try normal format first ..
        else
        {
            pLib = Mio_LibraryReadBuffer( pBufferCopy, 1, tExcludeGate, fVerbose );       // try normal format first ..
            if ( pLib )
                pLib->pName = Abc_UtilStrsav( Extra_FileNameGenericAppend(FileName, ".genlib") );
        }
        if ( pLib != NULL )
            printf ( "Warning: Read extended genlib format but ignoring extensions\n" );
    }
    ABC_FREE( pBufferCopy );
    if ( tExcludeGate )
        st__free_table( tExcludeGate );

    return pLib;
}

/**Function*************************************************************

  Synopsis    [Read contents of the file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Mio_ReadFile( char * FileName, int fAddEnd )
{
    char * pBuffer;
    FILE * pFile;
    int nFileSize;
    int RetValue;

    // open the BLIF file for binary reading
    pFile = Io_FileOpen( FileName, "open_path", "rb", 1 );
//    pFile = fopen( FileName, "rb" );
    // if we got this far, file should be okay otherwise would
    // have been detected by caller
    assert ( pFile != NULL );
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    pBuffer   = ABC_ALLOC( char, nFileSize + 10 );
    RetValue = fread( pBuffer, nFileSize, 1, pFile );
    // terminate the string with '\0'
    pBuffer[ nFileSize ] = '\0';
    if ( fAddEnd )
        strcat( pBuffer, "\n.end\n" );
    // close file
    fclose( pFile );
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    [Read the genlib type of library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Library_t * Mio_LibraryReadBuffer( char * pBuffer, int fExtendedFormat, st__table * tExcludeGate, int fVerbose )
{
    Mio_Library_t * pLib;

    // allocate the genlib structure
    pLib = ABC_CALLOC( Mio_Library_t, 1 );
    pLib->tName2Gate = st__init_table(strcmp, st__strhash);
    pLib->pMmFlex = Mem_FlexStart();
    pLib->vCube = Vec_StrAlloc( 100 );

    Io_ReadFileRemoveComments( pBuffer, NULL, NULL );

    // parse the contents of the file
    if ( Mio_LibraryReadInternal( pLib, pBuffer, fExtendedFormat, tExcludeGate, fVerbose ) )
    {
        Mio_LibraryDelete( pLib );
        return NULL;
    }

    // derive the functinality of gates
    if ( Mio_LibraryParseFormulas( pLib ) )
    {
        printf( "Mio_LibraryRead: Had problems parsing formulas.\n" );
        Mio_LibraryDelete( pLib );
        return NULL;
    }

    // detect INV and NAND2
    Mio_LibraryDetectSpecialGates( pLib );
//Mio_WriteLibrary( stdout, pLib );
    return pLib;
}

/**Function*************************************************************

  Synopsis    [Read the genlib type of library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Library_t * Mio_LibraryReadOne( char * FileName, int fExtendedFormat, st__table * tExcludeGate, int fVerbose )
{
    Mio_Library_t * pLib;
    char * pBuffer;
    // read the file and clean comments
    // pBuffer = Io_ReadFileFileContents( FileName, NULL );
    // we don't use above function but actually do the same thing explicitly
    // to handle open_path expansion correctly
    pBuffer = Mio_ReadFile( FileName, 1 );
    if ( pBuffer == NULL )
        return NULL;
    pLib = Mio_LibraryReadBuffer( pBuffer, fExtendedFormat, tExcludeGate, fVerbose );
    ABC_FREE( pBuffer );
    if ( pLib )
        pLib->pName = Abc_UtilStrsav( FileName );
    return pLib;
}

/**Function*************************************************************

  Synopsis    [Read the genlib type of library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_LibraryReadInternal( Mio_Library_t * pLib, char * pBuffer, int fExtendedFormat, st__table * tExcludeGate, int fVerbose )
{
    Mio_Gate_t * pGate, ** ppGate;
    char * pToken;
    int nGates = 0;
    int nDel = 0;

    // start the linked list of gates
    pLib->pGates = NULL;
    ppGate = &pLib->pGates;

    // read gates one by one
    pToken = strtok( pBuffer, " \t\r\n" );
    while ( pToken && (strcmp( pToken, MIO_STRING_GATE ) == 0 || strcmp( pToken, MIO_STRING_LATCH ) == 0) )
    {
        // skip latches
        if ( strcmp( pToken, MIO_STRING_LATCH ) == 0 )
        {
            while ( pToken && strcmp( pToken, MIO_STRING_GATE ) != 0 && strcmp( pToken, ".end" ) != 0 )
            {
                if ( strcmp( pToken, MIO_STRING_LATCH ) == 0 )
                {
                    pToken = strtok( NULL, " \t\r\n" );
                    printf( "Skipping latch \"%s\"...\n", pToken );
                    continue;
                }
                pToken = strtok( NULL, " \t\r\n" );
            }
            if ( !(pToken && strcmp( pToken, MIO_STRING_GATE ) == 0) )
                break;
        }

        // derive the next gate
        pGate = Mio_LibraryReadGate( &pToken, fExtendedFormat );
        if ( pGate == NULL )
            return 1;

        // skip the gate if its formula has problems
        if ( !Mio_ParseCheckFormula(pGate, pGate->pForm) )
        {
            Mio_GateDelete( pGate );
            continue;
        }
       
        // set the library
        pGate->pLib = pLib;

        // printf ("Processing: '%s'\n", pGate->pName);

        if ( tExcludeGate && st__is_member( tExcludeGate, pGate->pName ) )
        {
            //printf ("Excluding: '%s'\n", pGate->pName);
            Mio_GateDelete( pGate );
            nDel++;
        } 
        else
        {
            // add this gate to the list
            *ppGate = pGate;
            ppGate  = &pGate->pNext;
            nGates++;

            // remember this gate by name
            if ( ! st__is_member( pLib->tName2Gate, pGate->pName ) )
                st__insert( pLib->tName2Gate, pGate->pName, (char *)pGate );
            else
            {
                Mio_Gate_t * pBase = Mio_LibraryReadGateByName( pLib, pGate->pName, NULL );
                if ( pBase->pTwin != NULL )
                {
                    printf( "Gates with more than 2 outputs are not supported.\n" );
                    continue;
                }
                pBase->pTwin = pGate;
                pGate->pTwin = pBase;
//                printf( "Gate \"%s\" appears two times. Creating a 2-output gate.\n", pGate->pName );
            }
        }
    }

    if ( nGates == 0 )
    {
        printf( "The library contains no gates.\n" );
        return 1;
    }

    // check what is the last word read
    if ( pToken && strcmp( pToken, ".end" ) != 0 )
        return 1;

    if ( nDel != 0 ) 
        printf( "Actually excluded %d cells\n", nDel );

    return 0;
}

/**Function*************************************************************

  Synopsis    [Read the genlib type of gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Mio_LibraryCleanStr( char * p )
{
    int i, k;
    int whitespace_state = 0;
    char * pRes = Abc_UtilStrsav( p );
    for ( i = k = 0; pRes[i]; i++ )
        if ( pRes[i] != ' ' && pRes[i] != '\t' && pRes[i] != '\r' && pRes[i] != '\n' ) 
        {
            if ( pRes[i] != '(' && pRes[i] != ')' && pRes[i] != '+' && pRes[i] != '*' && pRes[i] != '|' && pRes[i] != '&' && pRes[i] != '^' && pRes[i] != '\'' && pRes[i] != '!' ) 
            {
                if (whitespace_state == 2)
                    pRes[k++] = ' ';
                whitespace_state = 1;
            } 
            else
                whitespace_state = 0;
            pRes[k++] = pRes[i];
        } 
        else
            whitespace_state = whitespace_state ? 2 : 0;
    pRes[k] = 0;
    return pRes;
}

Mio_Gate_t * Mio_LibraryReadGate( char ** ppToken, int fExtendedFormat )
{
    Mio_Gate_t * pGate;
    Mio_Pin_t * pPin, ** ppPin;
    char * pToken = *ppToken;

    // allocate the gate structure
    pGate = ABC_CALLOC( Mio_Gate_t, 1 );
    pGate->Cell = -1;

    // read the name
    pToken = strtok( NULL, " \t\r\n" );
    pGate->pName = Abc_UtilStrsav( pToken );

    // read the area
    pToken = strtok( NULL, " \t\r\n" );
    pGate->dArea = atof( pToken );

    // read the formula

    // first the output name
    pToken = strtok( NULL, "=" );
    pGate->pOutName = chomp( pToken );

    // then rest of the expression 
    pToken = strtok( NULL, ";" );
//    pGate->pForm = Mio_LibraryCleanStr( pToken );
    pGate->pForm = Abc_UtilStrsav( pToken );

    // read the pin info
    // start the linked list of pins
    pGate->pPins = NULL;
    ppPin = &pGate->pPins;

    // read gates one by one
    pToken = strtok( NULL, " \t\r\n" );
    while ( pToken && strcmp( pToken, MIO_STRING_PIN ) == 0 )
    {
        // derive the next gate
        pPin = Mio_LibraryReadPin( &pToken, fExtendedFormat );
        if ( pPin == NULL )
        {
            Mio_GateDelete( pGate );
            *ppToken = pToken;
            return NULL;
        }
        // add this pin to the list
        *ppPin = pPin;
        ppPin  = &pPin->pNext;
        // get the next token
        pToken = strtok( NULL, " \t\r\n" );
    }

    *ppToken = pToken;
    return pGate;
}



/**Function*************************************************************

  Synopsis    [Read the genlib type of pin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mio_Pin_t * Mio_LibraryReadPin( char ** ppToken, int fExtendedFormat )
{
    Mio_Pin_t * pPin;
    char * pToken = *ppToken;

    // allocate the gate structure
    pPin = ABC_CALLOC( Mio_Pin_t, 1 );

    // read the name
    pToken = strtok( NULL, " \t\r\n" );
    pPin->pName = Abc_UtilStrsav( pToken );

    // read the pin phase
    pToken = strtok( NULL, " \t\r\n" );
    if ( strcmp( pToken, MIO_STRING_UNKNOWN ) == 0 )
        pPin->Phase = MIO_PHASE_UNKNOWN;
    else if ( strcmp( pToken, MIO_STRING_INV ) == 0 )
        pPin->Phase = MIO_PHASE_INV;
    else if ( strcmp( pToken, MIO_STRING_NONINV ) == 0 )
        pPin->Phase = MIO_PHASE_NONINV;
    else 
    {
        printf( "Cannot read pin phase specification\n" );
        Mio_PinDelete( pPin );
        *ppToken = pToken;
        return NULL;
    }

    pToken = strtok( NULL, " \t\r\n" );
    pPin->dLoadInput = atof( pToken );

    pToken = strtok( NULL, " \t\r\n" );
    pPin->dLoadMax = atof( pToken );

    pToken = strtok( NULL, " \t\r\n" );
    pPin->dDelayBlockRise = atof( pToken );

    pToken = strtok( NULL, " \t\r\n" );
    pPin->dDelayFanoutRise = atof( pToken );

    pToken = strtok( NULL, " \t\r\n" );
    pPin->dDelayBlockFall = atof( pToken );

    pToken = strtok( NULL, " \t\r\n" );
    pPin->dDelayFanoutFall = atof( pToken );

    if ( fExtendedFormat )
    {
        /* In extended format, the field after dDelayFanoutRise
         * is to be ignored
         **/

        pPin->dDelayBlockFall  = pPin->dDelayFanoutFall;

        pToken = strtok( NULL, " \t" );
        pPin->dDelayFanoutFall = atof( pToken );

        /* last field is ignored */
        pToken = strtok( NULL, " \t\r\n" );
    }

    if ( pPin->dDelayBlockRise > pPin->dDelayBlockFall )
        pPin->dDelayBlockMax = pPin->dDelayBlockRise;
    else
        pPin->dDelayBlockMax = pPin->dDelayBlockFall;

    *ppToken = pToken;
    return pPin;
}


/**Function*************************************************************

  Synopsis    [Duplicates string and returns it with leading and 
               trailing spaces removed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * chomp( char *s )
{
    char *a, *b, *c;
    // remove leading spaces
    for ( b = s; *b; b++ )
        if ( !isspace(*b) )
            break;
    // strsav the string
    a = strcpy( ABC_ALLOC(char, strlen(b)+1), b );
    // remove trailing spaces
    for ( c = a+strlen(a); c > a; c-- )
        if ( *c == 0 || isspace(*c) )
            *c = 0;
        else
            break;
    return a;
}   
        
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_LibraryCompareGatesByArea( Mio_Gate_t ** pp1, Mio_Gate_t ** pp2 )
{
    double Diff = (*pp1)->dArea - (*pp2)->dArea;
    if ( Diff < 0.0 )
        return -1;
    if ( Diff > 0.0 ) 
        return 1;
    return 0; 
}
        
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_LibraryCompareGatesByName( Mio_Gate_t ** pp1, Mio_Gate_t ** pp2 )
{
    int Diff = strcmp( (*pp1)->pName, (*pp2)->pName );
    if ( Diff < 0 )
        return -1;
    if ( Diff > 0 ) 
        return 1;
    return 0; 
}
        
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mio_LibrarySortGates( Mio_Library_t * pLib )
{
    Mio_Gate_t ** ppGates, * pGate;
    int i = 0;
    ppGates = ABC_ALLOC( Mio_Gate_t *, pLib->nGates );
    Mio_LibraryForEachGate( pLib, pGate )
    {
        pGate->Cell = i;
        ppGates[i++] = pGate;
    }
    assert( i == pLib->nGates );
    // sort gates by name
    pLib->ppGates0 = ABC_ALLOC( Mio_Gate_t *, pLib->nGates );
    for ( i = 0; i < pLib->nGates; i++ )
        pLib->ppGates0[i] = ppGates[i];
    qsort( (void *)ppGates, pLib->nGates, sizeof(void *), 
            (int (*)(const void *, const void *)) Mio_LibraryCompareGatesByName );
    for ( i = 0; i < pLib->nGates; i++ )
        ppGates[i]->pNext = (i < pLib->nGates-1)? ppGates[i+1] : NULL;
    pLib->pGates = ppGates[0];
    pLib->ppGatesName = ppGates;
}
        
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Mio_Gate_t * Mio_GateCompare( Mio_Gate_t * pThis, Mio_Gate_t * pNew, word uTruth )
{
    if ( pNew->uTruth != uTruth )
        return pThis;
    if ( pThis == NULL )
        return pNew;
    if ( pThis->dArea > pNew->dArea || (pThis->dArea == pNew->dArea && strcmp(pThis->pName, pNew->pName) > 0) )
        return pNew;
    return pThis;
}
void Mio_LibraryDetectSpecialGates( Mio_Library_t * pLib )
{
    Mio_Gate_t * pGate;
    word uFuncBuf, uFuncInv, uFuncNand2, uFuncAnd2, uFuncNor2, uFuncOr2;

    Mio_LibrarySortGates( pLib );

    uFuncBuf   = ABC_CONST(0xAAAAAAAAAAAAAAAA);
    uFuncAnd2  = ABC_CONST(0xAAAAAAAAAAAAAAAA) & ABC_CONST(0xCCCCCCCCCCCCCCCC);
    uFuncOr2   = ABC_CONST(0xAAAAAAAAAAAAAAAA) | ABC_CONST(0xCCCCCCCCCCCCCCCC);
    uFuncInv   = ~uFuncBuf;
    uFuncNand2 = ~uFuncAnd2;
    uFuncNor2  = ~uFuncOr2;

    // get smallest-area buffer
    Mio_LibraryForEachGate( pLib, pGate )
        pLib->pGateBuf = Mio_GateCompare( pLib->pGateBuf, pGate, uFuncBuf );
    if ( pLib->pGateBuf == NULL )
    {
        printf( "Warnings: genlib library reader cannot detect the buffer gate.\n" );
        printf( "Some parts of the supergate-based technology mapper may not work correctly.\n" );
    }
 
    // get smallest-area inverter
    Mio_LibraryForEachGate( pLib, pGate )
        pLib->pGateInv = Mio_GateCompare( pLib->pGateInv, pGate, uFuncInv );
    if ( pLib->pGateInv == NULL )
    {
        printf( "Warnings: genlib library reader cannot detect the invertor gate.\n" );
        printf( "Some parts of the supergate-based technology mapper may not work correctly.\n" );
    }

    // get smallest-area NAND2/AND2 gates
    Mio_LibraryForEachGate( pLib, pGate )
    {
        pLib->pGateNand2 = Mio_GateCompare( pLib->pGateNand2, pGate, uFuncNand2 );
        pLib->pGateAnd2 = Mio_GateCompare( pLib->pGateAnd2, pGate, uFuncAnd2 );
        pLib->pGateNor2 = Mio_GateCompare( pLib->pGateNor2, pGate, uFuncNor2 );
        pLib->pGateOr2 = Mio_GateCompare( pLib->pGateOr2, pGate, uFuncOr2 );
    }
    if ( pLib->pGateAnd2 == NULL && pLib->pGateNand2 == NULL && pLib->pGateNor2 == NULL && pLib->pGateOr2 == NULL )
    {
        printf( "Warnings: genlib library reader cannot detect the AND2, NAND2, OR2, and NOR2 gate.\n" );
        printf( "Some parts of the supergate-based technology mapper may not work correctly.\n" );
    }
}

/**Function*************************************************************

  Synopsis    [populate hash table of gates to be exlcuded from genlib]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mio_LibraryReadExclude( char * ExcludeFile, st__table * tExcludeGate )
{
    int nDel = 0;
    FILE *pEx;
    char buffer[128];

    assert ( tExcludeGate );

    if ( ExcludeFile )
    {
        pEx = fopen( ExcludeFile, "r" );

        if ( pEx == NULL )
        {
            fprintf ( stdout, "Error: Could not open exclude file %s. Stop.\n", ExcludeFile );
            return -1;
        }

        while (1 == fscanf( pEx, "%127s", buffer ))
        {
            //printf ("Read: '%s'\n", buffer );
            st__insert( tExcludeGate, Abc_UtilStrsav( buffer ), (char *)0 );
            nDel++;
        }

        fclose( pEx );
    }

    return nDel;
}
    
/**Function*************************************************************

  Synopsis    [Eliminates comments from the input file.]

  Description [As a byproduct, this procedure also counts the number
  lines and dot-statements in the input file. This also joins non-comment 
  lines that are joined with a backspace '\']
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_ReadFileRemoveComments( char * pBuffer, int * pnDots, int * pnLines )
{
    char * pCur;
    int nDots, nLines;
    // scan through the buffer and eliminate comments
    // (in the BLIF file, comments are lines starting with "#")
    nDots = nLines = 0;
    for ( pCur = pBuffer; *pCur; pCur++ )
    {
        // if this is the beginning of comment
        // clean it with spaces until the new line statement
        if ( *pCur == '#' )
            while ( *pCur != '\n' )
                *pCur++ = ' ';
    
        // count the number of new lines and dots
        if ( *pCur == '\n' ) {
        if (*(pCur-1)=='\r') {
        // DOS(R) file support
        if (*(pCur-2)!='\\') nLines++;
        else {
            // rewind to backslash and overwrite with a space
            *(pCur-2) = ' ';
            *(pCur-1) = ' ';
            *pCur = ' ';
        }
        } else {
        // UNIX(TM) file support
        if (*(pCur-1)!='\\') nLines++;
        else {
            // rewind to backslash and overwrite with a space
            *(pCur-1) = ' ';
            *pCur = ' ';
        }
        }
    }
        else if ( *pCur == '.' )
            nDots++;
    }
    if ( pnDots )
        *pnDots = nDots; 
    if ( pnLines )
        *pnLines = nLines; 
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

