/**CFile****************************************************************

  FileName    [mapperSuper.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperSuper.c,v 1.6 2005/01/23 06:59:44 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int           Map_LibraryReadFile( Map_SuperLib_t * pLib, FILE * pFile );
static Map_Super_t * Map_LibraryReadGate( Map_SuperLib_t * pLib, char * pBuffer, int nVars );
static int           Map_LibraryTruthVerify( Map_SuperLib_t * pLib, Map_Super_t * pGate );
static void          Map_LibraryComputeTruth( Map_SuperLib_t * pLib, char * pFormula, unsigned uTruthRes[] );
static void          Map_LibraryComputeTruth_rec( Map_SuperLib_t * pLib, char * pFormula, unsigned uTruthsIn[][2], unsigned uTruthRes[] );
static void          Map_LibraryPrintClasses( Map_SuperLib_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the supergate library from file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_LibraryRead( Map_SuperLib_t * pLib, char * pFileName )
{
    FILE * pFile;
    int Status;
    // read the beginning of the file
    assert( pLib->pGenlib == NULL );
    pFile = fopen( pFileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Cannot open input file \"%s\".\n", pFileName );
        return 0;
    }
    Status = Map_LibraryReadFile( pLib, pFile );
    fclose( pFile );
//    Map_LibraryPrintClasses( pLib );
    return Status;
}


/**Function*************************************************************

  Synopsis    [Reads the library file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_LibraryReadFile( Map_SuperLib_t * pLib, FILE * pFile )
{
    ProgressBar * pProgress;
    char pBuffer[5000];
    FILE * pFileGen;
    Map_Super_t * pGate;
    char * pTemp = NULL; // Suppress "might be used uninitialized"
    char * pLibName;
    int nCounter, nGatesTotal;
    unsigned uCanon[2];
    int RetValue;

    // skip empty and comment lines
    while ( fgets( pBuffer, 2000, pFile ) != NULL )
    {
        // skip leading spaces
        for ( pTemp = pBuffer; *pTemp == ' ' || *pTemp == '\r' || *pTemp == '\n'; pTemp++ );
        // skip comment lines and empty lines
        if ( *pTemp != 0 && *pTemp != '#' )
            break;
    }

    // get the genlib file name
    pLibName = strtok( pTemp, " \t\r\n" );
    if ( strcmp( pLibName, "GATE" ) == 0 )
    {
        printf( "The input file \"%s\" looks like a genlib file and not a supergate library file.\n", pLib->pName );
        return 0;
    }
    pFileGen = fopen( pLibName, "r" );
    if ( pFileGen == NULL )
    {
        printf( "Cannot open the genlib file \"%s\".\n", pLibName );
        return 0;
    }
    fclose( pFileGen );

    // read the genlib library
    pLib->pGenlib = Mio_LibraryRead( pLibName, NULL, 0, 0 );
    if ( pLib->pGenlib == NULL )
    {
        printf( "Cannot read genlib file \"%s\".\n", pLibName );
        return 0;
    }

    // read the number of variables
    RetValue = fscanf( pFile, "%d\n", &pLib->nVarsMax );
    if ( pLib->nVarsMax < 2 || pLib->nVarsMax > 10 )
    {
        printf( "Suspicious number of variables (%d).\n", pLib->nVarsMax );
        return 0;
    }

    // read the number of gates
    RetValue = fscanf( pFile, "%d\n", &nGatesTotal );
    if ( nGatesTotal < 1 || nGatesTotal > 10000000 )
    {
        printf( "Suspicious number of gates (%d).\n", nGatesTotal );
        return 0;
    }

    // read the lines
    nCounter = 0;
    pProgress = Extra_ProgressBarStart( stdout, nGatesTotal );
    while ( fgets( pBuffer, 5000, pFile ) != NULL )
    {
        for ( pTemp = pBuffer; *pTemp == ' ' || *pTemp == '\r' || *pTemp == '\n'; pTemp++ );
        if ( pTemp[0] == '\0' )
            continue;
        // get the gate
        pGate = Map_LibraryReadGate( pLib, pTemp, pLib->nVarsMax );
        assert( pGate->Num == nCounter + 1 );
        // count the number of parentheses in the formula - this is the number of gates
        for ( pTemp = pGate->pFormula; *pTemp; pTemp++ )
            pGate->nGates += (*pTemp == '(');
        // verify the truth table
        assert( Map_LibraryTruthVerify(pLib, pGate) );

        // find the N-canonical form of this supergate
        pGate->nPhases = Map_CanonComputeSlow( pLib->uTruths, pLib->nVarsMax, pLib->nVarsMax, pGate->uTruth, pGate->uPhases, uCanon );
        // add the supergate into the table by its N-canonical table
        Map_SuperTableInsertC( pLib->tTableC, uCanon, pGate );
        // update the progress bar
        Extra_ProgressBarUpdate( pProgress, ++nCounter, NULL );
    }
    Extra_ProgressBarStop( pProgress );
    pLib->nSupersAll = nCounter;
    if ( nCounter != nGatesTotal )
        printf( "The number of gates read (%d) is different what the file says (%d).\n", nGatesTotal, nCounter );
    return 1;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Super_t * Map_LibraryReadGate( Map_SuperLib_t * pLib, char * pBuffer, int nVars )
{
    Map_Super_t * pGate;
    char * pTemp;
    int i;

    // start and clean the gate
    pGate = (Map_Super_t *)Extra_MmFixedEntryFetch( pLib->mmSupers );
    memset( pGate, 0, sizeof(Map_Super_t) );

    // read the number
    pTemp = strtok( pBuffer, " " );
    pGate->Num = atoi(pTemp);

    // read the signature
    pTemp = strtok( NULL, " " );
    if ( pLib->nVarsMax < 6 )
    {
        pGate->uTruth[0] = Extra_ReadBinary(pTemp);
        pGate->uTruth[1] = 0;
    }
    else
    {
        pGate->uTruth[0] = Extra_ReadBinary(pTemp+32);
        pTemp[32] = 0;
        pGate->uTruth[1] = Extra_ReadBinary(pTemp);
    }

    // read the max delay
    pTemp = strtok( NULL, " " );
    pGate->tDelayMax.Rise = (float)atof(pTemp);
    pGate->tDelayMax.Fall = pGate->tDelayMax.Rise;

    // read the pin-to-pin delay
    for ( i = 0; i < nVars; i++ )
    {
        pTemp = strtok( NULL, " " );
        pGate->tDelaysR[i].Rise = (float)atof(pTemp);
        pGate->tDelaysF[i].Fall = pGate->tDelaysR[i].Rise;
    }

    // read the area
    pTemp = strtok( NULL, " " );
    pGate->Area = (float)atof(pTemp);

    // the rest is the gate name
    pTemp = strtok( NULL, " \r\n" );
    if ( strlen(pTemp) == 0 )
        printf( "A gate name is empty.\n" );

    // save the gate name
    pGate->pFormula = Extra_MmFlexEntryFetch( pLib->mmForms, strlen(pTemp) + 1 );
    strcpy( pGate->pFormula, pTemp );

    // the rest is the gate name
    pTemp = strtok( NULL, " \n\0" );
    if ( pTemp != NULL )
        printf( "The following trailing symbols found \"%s\".\n", pTemp );
    return pGate;
}

/**Function*************************************************************

  Synopsis    [Performs one step of parsing the formula into parts.]

  Description [This function will eventually be replaced when the
  tree-supergate library representation will become standard.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Map_LibraryReadFormulaStep( char * pFormula, char * pStrings[], int * pnStrings )
{
    char * pName, * pPar1, * pPar2, * pCur;
    int nStrings, CountPars;

    // skip leading spaces
    for ( pName = pFormula; *pName && *pName == ' '; pName++ );
    assert( *pName );
    // find the first opening parenthesis
    for ( pPar1 = pName; *pPar1 && *pPar1 != '('; pPar1++ );
    if ( *pPar1 == 0 )
    {
        *pnStrings = 0;
        return pName;
    }
    // overwrite it with space
    assert( *pPar1 == '(' );
    *pPar1 = 0;
    // find the corresponding closing parenthesis
    for ( CountPars = 1, pPar2 = pPar1 + 1; *pPar2 && CountPars; pPar2++ )
        if ( *pPar2 == '(' )
            CountPars++;
        else if ( *pPar2 == ')' )
            CountPars--;
    pPar2--;
    assert( CountPars == 0 );
    // overwrite it with space
    assert( *pPar2 == ')' );
    *pPar2 = 0;
    // save the intervals between the commas
    nStrings = 0;
    pCur = pPar1 + 1;
    while ( 1 )
    {
        // save the current string
        pStrings[ nStrings++ ] = pCur;
        // find the beginning of the next string
        for ( CountPars = 0; *pCur && (CountPars || *pCur != ','); pCur++ )
            if ( *pCur == '(' )
                CountPars++;
            else if ( *pCur == ')' )
                CountPars--;
        if ( *pCur == 0 )
            break;
        assert( *pCur == ',' );
        *pCur = 0;
        pCur++;
    }
    // save the results and return
    *pnStrings = nStrings;
    return pName;
}


/**Function*************************************************************

  Synopsis    [Verifies the truth table of the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Map_LibraryTruthVerify( Map_SuperLib_t * pLib, Map_Super_t * pGate )
{
    unsigned uTruthRes[2];
    Map_LibraryComputeTruth( pLib, pGate->pFormula, uTruthRes );
    if ( uTruthRes[0] != pGate->uTruth[0] || uTruthRes[1] != pGate->uTruth[1] )
        return 0;
    return 1;
}


/**Function*************************************************************

  Synopsis    [Derives the functionality of the supergate.]

  Description [This procedure is useful for verification the supergate 
  library. The truth table derived by this procedure should be the same
  as the one contained in the original supergate file.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_LibraryComputeTruth( Map_SuperLib_t * pLib, char * pFormula, unsigned uTruthRes[] )
{
    char Buffer[1000];
    strcpy( Buffer, pFormula );
    Map_LibraryComputeTruth_rec( pLib, Buffer, pLib->uTruths, uTruthRes );
}

/**Function*************************************************************

  Synopsis    [Derives the functionality of the supergate.]

  Description [This procedure is useful for verification the supergate 
  library. The truth table derived by this procedure should be the same
  as the one contained in the original supergate file.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_LibraryComputeTruth_rec( Map_SuperLib_t * pLib, char * pFormula, unsigned uTruthsIn[][2], unsigned uTruthRes[] )
{
    Mio_Gate_t * pMioGate;
    char * pGateName, * pStrings[6];
    unsigned uTruthsFanins[6][2];
    int nStrings, i;

    // perform one step parsing of the formula
    // detect the root gate name, the next-step strings, and their number
    pGateName = Map_LibraryReadFormulaStep( pFormula, pStrings, &nStrings );
    if ( nStrings == 0 ) // elementary variable
    {
        assert( pGateName[0] - 'a' < pLib->nVarsMax );
        uTruthRes[0] = uTruthsIn[pGateName[0] - 'a'][0];
        uTruthRes[1] = uTruthsIn[pGateName[0] - 'a'][1];
        return;
    }
    // derive the functionality of the fanins
    for ( i = 0; i < nStrings; i++ )
        Map_LibraryComputeTruth_rec( pLib, pStrings[i], uTruthsIn, uTruthsFanins[i] );
    // get the root supergate
    pMioGate = Mio_LibraryReadGateByName( pLib->pGenlib, pGateName, NULL );
    if ( pMioGate == NULL )
        printf( "A supergate contains gate \"%s\" that is not in \"%s\".\n", pGateName, Mio_LibraryReadName(pLib->pGenlib) ); 
    // derive the functionality of the output of the supergate
    Mio_DeriveTruthTable( pMioGate, uTruthsFanins, nStrings, pLib->nVarsMax, uTruthRes );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_LibraryPrintSupergate( Map_Super_t * pGate )
{
    printf( "%5d : ",  pGate->nUsed );
    printf( "%5d   ",  pGate->Num );
    printf( "A = %5.2f   ",  pGate->Area );
    printf( "D = %5.2f/%5.2f/%5.2f   ", pGate->tDelayMax.Rise, pGate->tDelayMax.Fall, pGate->tDelayMax.Worst );
    printf( "%s",    pGate->pFormula );
    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    [Prints N-classes of supergates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_LibraryPrintClasses( Map_SuperLib_t * p )
{
/*
    st__generator * gen;
    Map_Super_t * pSuper, * pSuper2;
    unsigned Key, uTruth;
    int Counter = 0;
    // copy all the supergates into one array
    st__foreach_item( p->tSuplib, gen, (char **)&Key, (char **)&pSuper )
    {
        for ( pSuper2 = pSuper; pSuper2; pSuper2 = pSuper2->pNext )
        {
            uTruth = pSuper2->Phase;
            Extra_PrintBinary( stdout, &uTruth, 5 );
            printf( "  %5d   ",  pSuper2->Num );
            printf( "%s",    pSuper2->pFormula );
            printf( "\n" );
        }
        printf( "\n" );
        if ( ++ Counter == 100 )
            break;
    }
*/
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

