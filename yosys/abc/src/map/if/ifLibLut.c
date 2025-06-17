/**CFile****************************************************************

  FileName    [ifLibLut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [FPGA mapping based on priority cuts.]

  Synopsis    [LUT library.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 21, 2006.]

  Revision    [$Id: ifLibLut.c,v 1.00 2006/11/21 00:00:00 alanmi Exp $]

***********************************************************************/

#include "if.h"
#include "base/main/mainInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the description of LUTs from the LUT library file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_LibLut_t * If_LibLutReadString( char * pStr )
{
    If_LibLut_t * p;
    Vec_Ptr_t * vStrs;
    char * pToken, * pBuffer, * pStrNew, * pStrMem;
    int i, k, j;

    if ( pStr == NULL || pStr[0] == 0 )
        return NULL;

    vStrs = Vec_PtrAlloc( 1000 );
    pStrNew = pStrMem = Abc_UtilStrsav( pStr );
    while ( *pStrNew )
    {
        Vec_PtrPush( vStrs, pStrNew );
        while ( *pStrNew != '\n' )
            pStrNew++;
        while ( *pStrNew == '\n' )
            *pStrNew++ = '\0';
    }

    p = ABC_ALLOC( If_LibLut_t, 1 );
    memset( p, 0, sizeof(If_LibLut_t) );

    i = 1;
    //while ( fgets( pBuffer, 1000, pFile ) != NULL )
    Vec_PtrForEachEntry( char *, vStrs, pBuffer, j )
    {
        if ( pBuffer[0] == 0 )
            continue;
        pToken = strtok( pBuffer, " \t\n" );
        if ( pToken == NULL )
            continue;
        if ( pToken[0] == '#' )
            continue;
        if ( i != atoi(pToken) )
        {
            Abc_Print( 1, "Error in the LUT library string.\n" );
            ABC_FREE( p->pName );
            ABC_FREE( p );
            ABC_FREE( pStrMem );
            Vec_PtrFree( vStrs );
            return NULL;
        }

        // read area
        pToken = strtok( NULL, " \t\n" );
        p->pLutAreas[i] = (float)atof(pToken);

        // read delays
        k = 0;
        while ( (pToken = strtok( NULL, " \t\n" )) )
            p->pLutDelays[i][k++] = (float)atof(pToken);

        // check for out-of-bound
        if ( k > i )
        {
            Abc_Print( 1, "LUT %d has too many pins (%d). Max allowed is %d.\n", i, k, i );
            ABC_FREE( p->pName );
            ABC_FREE( p );
            ABC_FREE( pStrMem );
            Vec_PtrFree( vStrs );
            return NULL;
        }

        // check if var delays are specified
        if ( k > 1 )
            p->fVarPinDelays = 1;

        if ( i == IF_MAX_LUTSIZE )
        {
            Abc_Print( 1, "Skipping LUTs of size more than %d.\n", i );
            ABC_FREE( p->pName );
            ABC_FREE( p );
            ABC_FREE( pStrMem );
            Vec_PtrFree( vStrs );
            return NULL;
        }
        i++;
    }
    p->LutMax = i-1;

    // check the library
    if ( p->fVarPinDelays )
    {
        for ( i = 1; i <= p->LutMax; i++ )
            for ( k = 0; k < i; k++ )
            {
                if ( p->pLutDelays[i][k] <= 0.0 )
                    Abc_Print( 0, "Pin %d of LUT %d has delay %f. Pin delays should be non-negative numbers. Technology mapping may not work correctly.\n", 
                        k, i, p->pLutDelays[i][k] );
                if ( k && p->pLutDelays[i][k-1] > p->pLutDelays[i][k] )
                    Abc_Print( 0, "Pin %d of LUT %d has delay %f. Pin %d of LUT %d has delay %f. Pin delays should be in non-decreasing order. Technology mapping may not work correctly.\n", 
                        k-1, i, p->pLutDelays[i][k-1], 
                        k, i, p->pLutDelays[i][k] );
            }
    }
    else
    {
        for ( i = 1; i <= p->LutMax; i++ )
        {
            if ( p->pLutDelays[i][0] <= 0.0 )
                Abc_Print( 0, "LUT %d has delay %f. Pin delays should be non-negative numbers. Technology mapping may not work correctly.\n", 
                    i, p->pLutDelays[i][0] );
        }
    }

    // cleanup
    ABC_FREE( pStrMem );
    Vec_PtrFree( vStrs );
    return p;
}

/**Function*************************************************************

  Synopsis    [Sets the library associated with the string.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_FrameSetLutLibrary( Abc_Frame_t * pAbc, char * pLutLibString )
{
    If_LibLut_t * pLib = If_LibLutReadString( pLutLibString );
    if ( pLib == NULL )
    {
        fprintf( stdout, "Reading LUT library from string has failed.\n" );
        return 0;
    }
    // replace the current library
    If_LibLutFree( (If_LibLut_t *)Abc_FrameReadLibLut() );
    Abc_FrameSetLibLut( pLib );
    return 1;
}
int Abc_FrameSetLutLibraryTest( Abc_Frame_t * pAbc )
{
    char * pStr = "1 1.00  1000\n2 1.00  1000 1200\n3 1.00  1000 1200 1400\n4 1.00  1000 1200 1400 1600\n5 1.00  1000 1200 1400 1600 1800\n6 1.00  1000 1200 1400 1600 1800 2000\n\n\n";
    Abc_FrameSetLutLibrary( pAbc, pStr );
    return 1;
}


/**Function*************************************************************

  Synopsis    [Reads the description of LUTs from the LUT library file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_LibLut_t * If_LibLutRead( char * FileName )
{
    char pBuffer[1000], * pToken;
    If_LibLut_t * p;
    FILE * pFile;
    int i, k;

    pFile = fopen( FileName, "r" );
    if ( pFile == NULL )
    {
        Abc_Print( -1, "Cannot open LUT library file \"%s\".\n", FileName );
        return NULL;
    }

    p = ABC_ALLOC( If_LibLut_t, 1 );
    memset( p, 0, sizeof(If_LibLut_t) );
    p->pName = Abc_UtilStrsav( FileName );

    i = 1;
    while ( fgets( pBuffer, 1000, pFile ) != NULL )
    {
        pToken = strtok( pBuffer, " \t\n" );
        if ( pToken == NULL )
            continue;
        if ( pToken[0] == '#' )
            continue;
        if ( i != atoi(pToken) )
        {
            Abc_Print( 1, "Error in the LUT library file \"%s\".\n", FileName );
            ABC_FREE( p->pName );
            ABC_FREE( p );
            fclose( pFile );
            return NULL;
        }

        // read area
        pToken = strtok( NULL, " \t\n" );
        p->pLutAreas[i] = (float)atof(pToken);

        // read delays
        k = 0;
        while ( (pToken = strtok( NULL, " \t\n" )) )
            p->pLutDelays[i][k++] = (float)atof(pToken);

        // check for out-of-bound
        if ( k > i )
        {
            ABC_FREE( p->pName );
            ABC_FREE( p );
            Abc_Print( 1, "LUT %d has too many pins (%d). Max allowed is %d.\n", i, k, i );
            fclose( pFile );
            return NULL;
        }

        // check if var delays are specified
        if ( k > 1 )
            p->fVarPinDelays = 1;

        if ( i == IF_MAX_LUTSIZE )
        {
            ABC_FREE( p->pName );
            ABC_FREE( p );
            Abc_Print( 1, "Skipping LUTs of size more than %d.\n", i );
            fclose( pFile );
            return NULL;
        }
        i++;
    }
    p->LutMax = i-1;

    // check the library
    if ( p->fVarPinDelays )
    {
        for ( i = 1; i <= p->LutMax; i++ )
            for ( k = 0; k < i; k++ )
            {
                if ( p->pLutDelays[i][k] <= 0.0 )
                    Abc_Print( 0, "Pin %d of LUT %d has delay %f. Pin delays should be non-negative numbers. Technology mapping may not work correctly.\n", 
                        k, i, p->pLutDelays[i][k] );
                if ( k && p->pLutDelays[i][k-1] > p->pLutDelays[i][k] )
                    Abc_Print( 0, "Pin %d of LUT %d has delay %f. Pin %d of LUT %d has delay %f. Pin delays should be in non-decreasing order. Technology mapping may not work correctly.\n", 
                        k-1, i, p->pLutDelays[i][k-1], 
                        k, i, p->pLutDelays[i][k] );
            }
    }
    else
    {
        for ( i = 1; i <= p->LutMax; i++ )
        {
            if ( p->pLutDelays[i][0] <= 0.0 )
                Abc_Print( 0, "LUT %d has delay %f. Pin delays should be non-negative numbers. Technology mapping may not work correctly.\n", 
                    i, p->pLutDelays[i][0] );
        }
    }

    fclose( pFile );
    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_LibLut_t * If_LibLutDup( If_LibLut_t * p )
{
    If_LibLut_t * pNew;
    pNew = ABC_ALLOC( If_LibLut_t, 1 );
    *pNew = *p;
    pNew->pName = Abc_UtilStrsav( pNew->pName );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Frees the LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_LibLutFree( If_LibLut_t * pLutLib )
{
    if ( pLutLib == NULL )
        return;
    ABC_FREE( pLutLib->pName );
    ABC_FREE( pLutLib );
}


/**Function*************************************************************

  Synopsis    [Prints the LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void If_LibLutPrint( If_LibLut_t * pLutLib )
{
    int i, k;
    Abc_Print( 1, "# The area/delay of k-variable LUTs:\n" );
    Abc_Print( 1, "# k    area     delay\n" );
    if ( pLutLib->fVarPinDelays )
    {
        for ( i = 1; i <= pLutLib->LutMax; i++ )
        {
            Abc_Print( 1, "%d   %7.2f  ", i, pLutLib->pLutAreas[i] );
            for ( k = 0; k < i; k++ )
                Abc_Print( 1, " %7.2f", pLutLib->pLutDelays[i][k] );
            Abc_Print( 1, "\n" );
        }
    }
    else
        for ( i = 1; i <= pLutLib->LutMax; i++ )
            Abc_Print( 1, "%d   %7.2f   %7.2f\n", i, pLutLib->pLutAreas[i], pLutLib->pLutDelays[i][0] );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the delays are discrete.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_LibLutDelaysAreDiscrete( If_LibLut_t * pLutLib )
{
    float Delay;
    int i;
    for ( i = 1; i <= pLutLib->LutMax; i++ )
    {
        Delay = pLutLib->pLutDelays[i][0];
        if ( ((float)((int)Delay)) != Delay )
            return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the delays are discrete.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int If_LibLutDelaysAreDifferent( If_LibLut_t * pLutLib )
{
    int i, k;
    float Delay = pLutLib->pLutDelays[1][0];
    if ( pLutLib->fVarPinDelays )
    {
        for ( i = 2; i <= pLutLib->LutMax; i++ )
        for ( k = 0; k < i; k++ )
            if ( pLutLib->pLutDelays[i][k] != Delay )
                return 1;
    }
    else
    {
        for ( i = 2; i <= pLutLib->LutMax; i++ )
            if ( pLutLib->pLutDelays[i][0] != Delay )
                return 1;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Sets simple LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
If_LibLut_t * If_LibLutSetSimple( int nLutSize )
{
    If_LibLut_t s_LutLib10= { "lutlib",10, 0, {0,1,1,1,1,1,1,1,1,1,1}, {{0},{1},{1},{1},{1},{1},{1},{1},{1},{1},{1}} };
    If_LibLut_t s_LutLib9 = { "lutlib", 9, 0, {0,1,1,1,1,1,1,1,1,1}, {{0},{1},{1},{1},{1},{1},{1},{1},{1},{1}} };
    If_LibLut_t s_LutLib8 = { "lutlib", 8, 0, {0,1,1,1,1,1,1,1,1}, {{0},{1},{1},{1},{1},{1},{1},{1},{1}} };
    If_LibLut_t s_LutLib7 = { "lutlib", 7, 0, {0,1,1,1,1,1,1,1}, {{0},{1},{1},{1},{1},{1},{1},{1}} };
    If_LibLut_t s_LutLib6 = { "lutlib", 6, 0, {0,1,1,1,1,1,1}, {{0},{1},{1},{1},{1},{1},{1}} };
    If_LibLut_t s_LutLib5 = { "lutlib", 5, 0, {0,1,1,1,1,1}, {{0},{1},{1},{1},{1},{1}} };
    If_LibLut_t s_LutLib4 = { "lutlib", 4, 0, {0,1,1,1,1}, {{0},{1},{1},{1},{1}} };
    If_LibLut_t s_LutLib3 = { "lutlib", 3, 0, {0,1,1,1}, {{0},{1},{1},{1}} };
    If_LibLut_t * pLutLib;
    assert( nLutSize >= 3 && nLutSize <= 10 );
    switch ( nLutSize )
    {
        case 3:  pLutLib = &s_LutLib3; break;
        case 4:  pLutLib = &s_LutLib4; break;
        case 5:  pLutLib = &s_LutLib5; break;
        case 6:  pLutLib = &s_LutLib6; break;
        case 7:  pLutLib = &s_LutLib7; break;
        case 8:  pLutLib = &s_LutLib8; break;
        case 9:  pLutLib = &s_LutLib9; break;
        case 10: pLutLib = &s_LutLib10; break;
        default: pLutLib = NULL; break;
    }
    if ( pLutLib == NULL )
        return NULL;
    return If_LibLutDup(pLutLib);
}

/**Function*************************************************************

  Synopsis    [Gets the delay of the fastest pin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_LibLutFastestPinDelay( If_LibLut_t * p )
{
    return !p? 1.0 : p->pLutDelays[p->LutMax][0];
}

/**Function*************************************************************

  Synopsis    [Gets the delay of the slowest pin.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float If_LibLutSlowestPinDelay( If_LibLut_t * p )
{
    return !p? 1.0 : (p->fVarPinDelays? p->pLutDelays[p->LutMax][p->LutMax-1]: p->pLutDelays[p->LutMax][0]);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

