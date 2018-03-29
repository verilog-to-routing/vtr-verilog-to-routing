/**CFile****************************************************************

  FileName    [rwrCut.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting package.]

  Synopsis    [Cut computation.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rwrCut.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rwr.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int pTruths[13719];
static int pFreqs[13719];
static int pPerm[13719];

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rwr_TempCompare( int * pNum1, int * pNum2 )
{
    int Freq1 = pFreqs[*pNum1];
    int Freq2 = pFreqs[*pNum2];
    if ( Freq1 < Freq2 )
        return 1;
    if ( Freq1 > Freq2 )
        return -1;
    return 0; 
}
 
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rwr_Temp()
{
    char Buffer[32];
    int nFuncs = 13719;
    int nEntries = 100;
    unsigned uTruth;
    int i, k;
    FILE * pFile;

    pFile = fopen( "nnclass_stats5.txt", "r" );
    for ( i = 0; i < 13719; i++ )
    {
        int RetValue = fscanf( pFile, "%s%d", Buffer, &pFreqs[i] );
        Extra_ReadHexadecimal( &uTruth, Buffer+2, 5 );
        pTruths[i] = uTruth;
    }
    fclose( pFile );

    for ( i = 0; i < 13719; i++ )
        pPerm[i] = i;

    qsort( (void *)pPerm, 13719, sizeof(int), 
            (int (*)(const void *, const void *)) Rwr_TempCompare );


    pFile = fopen( "5npn_100.blif", "w" );
    fprintf( pFile, "# Most frequent NPN classes of 5 vars.\n" );
    fprintf( pFile, ".model 5npn\n" );
    fprintf( pFile, ".inputs a b c d e\n" );
    fprintf( pFile, ".outputs" );
    for ( i = 0; i < nEntries; i++ )
        fprintf( pFile, " %02d", i );
    fprintf( pFile, "\n" );

    for ( i = 0; i < nEntries; i++ )
    {
        fprintf( pFile, ".names a b c d e %02d\n", i );
        uTruth = pTruths[pPerm[i]];
        for ( k = 0; k < 32; k++ )
            if ( uTruth & (1 << k) )
            {
                Extra_PrintBinary( pFile, (unsigned *)&k, 5 );
                fprintf( pFile, " 1\n" );
            }
    }
    fprintf( pFile, ".end\n" );
    fclose( pFile );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

