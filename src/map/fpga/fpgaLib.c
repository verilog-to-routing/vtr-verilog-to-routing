/**CFile****************************************************************

  FileName    [fpgaLib.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Technology mapping for variable-size-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - August 18, 2004.]

  Revision    [$Id: fpgaLib.c,v 1.4 2005/01/23 06:59:41 alanmi Exp $]

***********************************************************************/

#include "fpgaInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [APIs to access LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int         Fpga_LutLibReadVarMax( Fpga_LutLib_t * p )               { return p->LutMax;     }
float *     Fpga_LutLibReadLutAreas( Fpga_LutLib_t * p )             { return p->pLutAreas;  }
float       Fpga_LutLibReadLutArea( Fpga_LutLib_t * p, int Size )    { assert( Size <= p->LutMax ); return p->pLutAreas[Size];  }

/**Function*************************************************************

  Synopsis    [Reads the description of LUTs from the LUT library file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_LutLib_t * Fpga_LutLibRead( char * FileName, int fVerbose )
{
    char pBuffer[1000], * pToken;
    Fpga_LutLib_t * p;
    FILE * pFile;
    int i, k;

    pFile = fopen( FileName, "r" );
    if ( pFile == NULL )
    {
        printf( "Cannot open LUT library file \"%s\".\n", FileName );
        return NULL;
    }

    p = ABC_ALLOC( Fpga_LutLib_t, 1 );
    memset( p, 0, sizeof(Fpga_LutLib_t) );
    p->pName = Extra_UtilStrsav( FileName );

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
            printf( "Error in the LUT library file \"%s\".\n", FileName );
            ABC_FREE( p );
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
            printf( "LUT %d has too many pins (%d). Max allowed is %d.\n", i, k, i );
            return NULL;
        }

        // check if var delays are specifies
        if ( k > 1 )
            p->fVarPinDelays = 1;

        if ( i == FPGA_MAX_LUTSIZE )
        {
            printf( "Skipping LUTs of size more than %d.\n", i );
            return NULL;
        }
        i++;
    }
    p->LutMax = i-1;
/*
    if ( p->LutMax > FPGA_MAX_LEAVES )
    {
        p->LutMax = FPGA_MAX_LEAVES;
        printf( "Warning: LUTs with more than %d inputs will not be used.\n", FPGA_MAX_LEAVES );
    }
*/
    // check the library
    if ( p->fVarPinDelays )
    {
        for ( i = 1; i <= p->LutMax; i++ )
            for ( k = 0; k < i; k++ )
            {
                if ( p->pLutDelays[i][k] <= 0.0 )
                    printf( "Warning: Pin %d of LUT %d has delay %f. Pin delays should be non-negative numbers. Technology mapping may not work correctly.\n", 
                        k, i, p->pLutDelays[i][k] );
                if ( k && p->pLutDelays[i][k-1] > p->pLutDelays[i][k] )
                    printf( "Warning: Pin %d of LUT %d has delay %f. Pin %d of LUT %d has delay %f. Pin delays should be in non-decreasing order. Technology mapping may not work correctly.\n", 
                        k-1, i, p->pLutDelays[i][k-1], 
                        k, i, p->pLutDelays[i][k] );
            }
    }
    else
    {
        for ( i = 1; i <= p->LutMax; i++ )
        {
            if ( p->pLutDelays[i][0] <= 0.0 )
                printf( "Warning: LUT %d has delay %f. Pin delays should be non-negative numbers. Technology mapping may not work correctly.\n", 
                    i, p->pLutDelays[i][0] );
        }
    }

    return p;
}

/**Function*************************************************************

  Synopsis    [Duplicates the LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_LutLib_t * Fpga_LutLibDup( Fpga_LutLib_t * p )
{
    Fpga_LutLib_t * pNew;
    pNew = ABC_ALLOC( Fpga_LutLib_t, 1 );
    *pNew = *p;
    pNew->pName = Extra_UtilStrsav( pNew->pName );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Frees the LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_LutLibFree( Fpga_LutLib_t * pLutLib )
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
void Fpga_LutLibPrint( Fpga_LutLib_t * pLutLib )
{
    int i, k;
    printf( "# The area/delay of k-variable LUTs:\n" );
    printf( "# k    area     delay\n" );
    if ( pLutLib->fVarPinDelays )
    {
        for ( i = 1; i <= pLutLib->LutMax; i++ )
        {
            printf( "%d   %7.2f  ", i, pLutLib->pLutAreas[i] );
            for ( k = 0; k < i; k++ )
                printf( " %7.2f", pLutLib->pLutDelays[i][k] );
            printf( "\n" );
        }
    }
    else
        for ( i = 1; i <= pLutLib->LutMax; i++ )
            printf( "%d   %7.2f   %7.2f\n", i, pLutLib->pLutAreas[i], pLutLib->pLutDelays[i][0] );
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the delays are discrete.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fpga_LutLibDelaysAreDiscrete( Fpga_LutLib_t * pLutLib )
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

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

