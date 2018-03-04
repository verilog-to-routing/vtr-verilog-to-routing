/**CFile****************************************************************

  FileName    [amapRead.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapRead.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "amapInt.h"
#include "base/io/ioAbc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define AMAP_STRING_GATE       "GATE"
#define AMAP_STRING_PIN        "PIN"
#define AMAP_STRING_NONINV     "NONINV"
#define AMAP_STRING_INV        "INV"
#define AMAP_STRING_UNKNOWN    "UNKNOWN"

// these symbols (and no other) can appear in the formulas
#define AMAP_SYMB_AND    '*'
#define AMAP_SYMB_AND2   '&'
#define AMAP_SYMB_OR1    '+'
#define AMAP_SYMB_OR2    '|'
#define AMAP_SYMB_XOR    '^'
#define AMAP_SYMB_NOT    '!'
#define AMAP_SYMB_AFTNOT '\''
#define AMAP_SYMB_OPEN   '('
#define AMAP_SYMB_CLOSE  ')'

typedef enum { 
    AMAP_PHASE_UNKNOWN, 
    AMAP_PHASE_INV, 
    AMAP_PHASE_NONINV 
} Amap_PinPhase_t;

static inline Amap_Gat_t * Amap_ParseGateAlloc( Aig_MmFlex_t * p, int nPins ) 
{ return (Amap_Gat_t *)Aig_MmFlexEntryFetch( p, sizeof(Amap_Gat_t)+sizeof(Amap_Pin_t)*nPins ); }
static inline char * Amap_ParseStrsav( Aig_MmFlex_t * p, char * pStr ) 
{ return pStr ? strcpy(Aig_MmFlexEntryFetch(p, strlen(pStr)+1), pStr) : NULL; }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Loads the file into temporary buffer.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Amap_LoadFile( char * pFileName )
{
//    extern FILE * Io_FileOpen( const char * FileName, const char * PathVar, const char * Mode, int fVerbose );
    FILE * pFile;
    char * pBuffer;
    int nFileSize;
    int RetValue;
    // open the BLIF file for binary reading
    pFile = Io_FileOpen( pFileName, "open_path", "rb", 1 );
//    pFile = fopen( FileName, "rb" );
    // if we got this far, file should be okay otherwise would
    // have been detected by caller
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\".\n", pFileName );
        return NULL;
    }
    assert ( pFile != NULL );
    // get the file size, in bytes
    fseek( pFile, 0, SEEK_END );  
    nFileSize = ftell( pFile );  
    // move the file current reading position to the beginning
    rewind( pFile ); 
    // load the contents of the file into memory
    pBuffer = ABC_ALLOC( char, nFileSize + 10 );
    RetValue = fread( pBuffer, nFileSize, 1, pFile );
    // terminate the string with '\0'
    pBuffer[ nFileSize ] = '\0';
    strcat( pBuffer, "\n.end\n" );
    // close file
    fclose( pFile );
    return pBuffer;
}

/**Function*************************************************************

  Synopsis    [Eliminates comments from the input file.]

  Description [As a byproduct, this procedure also counts the number
  lines and dot-statements in the input file. This also joins non-comment 
  lines that are joined with a backspace '\']
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Amap_RemoveComments( char * pBuffer, int * pnDots, int * pnLines )
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

/**Function*************************************************************

  Synopsis    [Splits the stream into tokens.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Amap_DeriveTokens( char * pBuffer )
{
    Vec_Ptr_t * vTokens;
    char * pToken;
    vTokens = Vec_PtrAlloc( 1000 );
    pToken = strtok( pBuffer, " =\t\r\n" );
    while ( pToken )
    {
        Vec_PtrPush( vTokens, pToken );
        pToken = strtok( NULL, " =\t\r\n" );
        // skip latches
        if ( pToken && strcmp( pToken, "LATCH" ) == 0 )
            while ( pToken && strcmp( pToken, "GATE" ) != 0 )
                pToken = strtok( NULL, " =\t\r\n" );
    }
    return vTokens;
}

/**Function*************************************************************

  Synopsis    [Finds the number of pins.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_ParseCountPins( Vec_Ptr_t * vTokens, int iPos )
{
    char * pToken;
    int i, Counter = 0;
    Vec_PtrForEachEntryStart( char *, vTokens, pToken, i, iPos )
        if ( !strcmp( pToken, AMAP_STRING_PIN ) )
            Counter++;
        else if ( !strcmp( pToken, AMAP_STRING_GATE ) )
            return Counter;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Collect the pin names used in the formula.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_GateCollectNames( Aig_MmFlex_t * pMem, char * pForm, char * pPinNames[] )
{
    char Buffer[1000];
    char * pTemp;
    int nPins, i;
    // save the formula as it was
    strcpy( Buffer, pForm );
    // remove the non-name symbols
    for ( pTemp = Buffer; *pTemp; pTemp++ )
        if ( *pTemp == AMAP_SYMB_AND || *pTemp == AMAP_SYMB_OR1 || *pTemp == AMAP_SYMB_OR2 
          || *pTemp == AMAP_SYMB_XOR || *pTemp == AMAP_SYMB_NOT || *pTemp == AMAP_SYMB_OPEN 
          || *pTemp == AMAP_SYMB_CLOSE || *pTemp == AMAP_SYMB_AFTNOT || *pTemp == AMAP_SYMB_AND2 )
            *pTemp = ' ';
    // save the names
    nPins = 0;
    pTemp = strtok( Buffer, " " );
    while ( pTemp )
    {
        for ( i = 0; i < nPins; i++ )
            if ( strcmp( pTemp, pPinNames[i] ) == 0 )
                break;
        if ( i == nPins )
        { // cannot find this name; save it
            pPinNames[nPins++] = Amap_ParseStrsav( pMem, pTemp );
        }
        // get the next name
        pTemp = strtok( NULL, " " );
    }
    return nPins;
}

/**Function*************************************************************

  Synopsis    [Creates a duplicate gate with pins specified.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Gat_t * Amap_ParseGateWithSamePins( Amap_Gat_t * p )
{
    Amap_Gat_t * pGate;
    Amap_Pin_t * pPin;
    char * pPinNames[128];
    int nPinNames;
    assert( p->nPins == 1 && !strcmp( p->Pins->pName, "*" ) );
    nPinNames = Amap_GateCollectNames( p->pLib->pMemGates, p->pForm, pPinNames );
    pGate = Amap_ParseGateAlloc( p->pLib->pMemGates, nPinNames );
    *pGate = *p;
    pGate->nPins = nPinNames;
    Amap_GateForEachPin( pGate, pPin )
    {
        *pPin = *p->Pins;
        pPin->pName = pPinNames[pPin - pGate->Pins];
    }
    return pGate;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Amap_CollectFormulaTokens( Vec_Ptr_t * vTokens, char * pToken, int iPos )
{
    char * pNext, * pPrev;
    pPrev = pToken + strlen(pToken);
    while ( *(pPrev-1) != ';' )
    {
        *pPrev++ = ' ';
        pNext = (char *)Vec_PtrEntry(vTokens, iPos++);
        while ( *pNext )
            *pPrev++ = *pNext++;
    }
    *(pPrev-1) = 0;
    return iPos;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Lib_t * Amap_ParseTokens( Vec_Ptr_t * vTokens, int fVerbose )
{
    Amap_Lib_t * p;
    Amap_Gat_t * pGate, * pPrev;
    Amap_Pin_t * pPin;
    char * pToken, * pMoGate = NULL;
    int i, nPins, iPos = 0, Count = 0;
    p = Amap_LibAlloc();
    pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
    do 
    {
        if ( strcmp( pToken, AMAP_STRING_GATE ) )
        {
            Amap_LibFree( p );
            printf( "The first line should begin with %s.\n", AMAP_STRING_GATE );
            return NULL;
        }
        // start gate
        nPins = Amap_ParseCountPins( vTokens, iPos );
        pGate = Amap_ParseGateAlloc( p->pMemGates, nPins );
        memset( pGate, 0, sizeof(Amap_Gat_t) );
        pGate->Id = Vec_PtrSize( p->vGates );
        Vec_PtrPush( p->vGates, pGate );
        pGate->pLib = p;
        pGate->nPins = nPins;
        // read gate
        pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
        pGate->pName = Amap_ParseStrsav( p->pMemGates, pToken );    
        pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
        pGate->dArea = atof( pToken );
        pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
        pGate->pOutName = Amap_ParseStrsav( p->pMemGates, pToken ); 
        pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
        iPos = Amap_CollectFormulaTokens( vTokens, pToken, iPos );
        pGate->pForm = Amap_ParseStrsav( p->pMemGates, pToken ); 
        // read pins
        Amap_GateForEachPin( pGate, pPin )
        {
            pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
            if ( strcmp( pToken, AMAP_STRING_PIN ) )
            {
                Amap_LibFree( p );
                printf( "Cannot parse gate %s.\n", pGate->pName );
                return NULL;
            }
            // read pin
            pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
            pPin->pName = Amap_ParseStrsav( p->pMemGates, pToken );   
            pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
            if ( strcmp( pToken, AMAP_STRING_UNKNOWN ) == 0 )
                pPin->Phase = AMAP_PHASE_UNKNOWN;
            else if ( strcmp( pToken, AMAP_STRING_INV ) == 0 )
                pPin->Phase = AMAP_PHASE_INV;
            else if ( strcmp( pToken, AMAP_STRING_NONINV ) == 0 )
                pPin->Phase = AMAP_PHASE_NONINV;
            else 
            {
                Amap_LibFree( p );
                printf( "Cannot read phase of pin %s of gate %s\n", pPin->pName, pGate->pName );
                return NULL;
            }
            pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
            pPin->dLoadInput = atof( pToken );
            pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
            pPin->dLoadMax = atof( pToken );
            pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
            pPin->dDelayBlockRise = atof( pToken );
            pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
            pPin->dDelayFanoutRise = atof( pToken );
            pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
            pPin->dDelayBlockFall = atof( pToken );
            pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
            pPin->dDelayFanoutFall = atof( pToken );
            if ( pPin->dDelayBlockRise > pPin->dDelayBlockFall )
                pPin->dDelayBlockMax = pPin->dDelayBlockRise;
            else
                pPin->dDelayBlockMax = pPin->dDelayBlockFall;
        }
        // fix the situation when all pins are represented as one
        if ( pGate->nPins == 1 && !strcmp( pGate->Pins->pName, "*" ) )
        {
            pGate = Amap_ParseGateWithSamePins( pGate );
            Vec_PtrPop( p->vGates );
            Vec_PtrPush( p->vGates, pGate );
        }
        pToken = (char *)Vec_PtrEntry(vTokens, iPos++);
//printf( "Finished reading gate %s (%s)\n", pGate->pName, pGate->pOutName );
    }
    while ( strcmp( pToken, ".end" ) );

    // check if there are gates with identical names
    pPrev = NULL;
    Amap_LibForEachGate( p, pGate, i )
    {
        if ( pPrev && !strcmp(pPrev->pName, pGate->pName) )
        {
            pPrev->pTwin = pGate, pGate->pTwin = pPrev;
//            printf( "Warning: Detected multi-output gate \"%s\".\n", pGate->pName );
            if ( pMoGate == NULL )
                pMoGate = pGate->pName;
            Count++;
        }
        pPrev = pGate;
    }
    if ( Count )
        printf( "Warning: Detected %d multi-output gates (for example, \"%s\").\n", Count, pMoGate );
    return p;
}

/**Function*************************************************************

  Synopsis    [Reads the library from the input file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Lib_t * Amap_LibReadBuffer( char * pBuffer, int fVerbose )
{
    Amap_Lib_t * pLib;
    Vec_Ptr_t * vTokens;
    Amap_RemoveComments( pBuffer, NULL, NULL );
    vTokens = Amap_DeriveTokens( pBuffer );
    pLib = Amap_ParseTokens( vTokens, fVerbose );
    if ( pLib == NULL )
    {
        Vec_PtrFree( vTokens );
        return NULL;
    }
    Vec_PtrFree( vTokens );
    return pLib;
}

/**Function*************************************************************

  Synopsis    [Reads the library from the input file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Amap_Lib_t * Amap_LibReadFile( char * pFileName, int fVerbose )
{
    Amap_Lib_t * pLib;
    char * pBuffer;
    pBuffer = Amap_LoadFile( pFileName );
    if ( pBuffer == NULL )
        return NULL;
    pLib = Amap_LibReadBuffer( pBuffer, fVerbose );
    if ( pLib )
        pLib->pName = Abc_UtilStrsav( pFileName );
    ABC_FREE( pBuffer );
    return pLib;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

