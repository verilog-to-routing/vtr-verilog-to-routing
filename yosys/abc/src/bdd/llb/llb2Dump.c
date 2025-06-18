/**CFile****************************************************************

  FileName    [llb2Dump.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD based reachability.]

  Synopsis    [Dumps the BDD of reached states into a file.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: llb2Dump.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "llbInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Returns a dummy name.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Llb_ManGetDummyName( char * pPrefix, int Num, int nDigits )
{
    static char Buffer[2000];
    sprintf( Buffer, "%s%0*d", pPrefix, nDigits, Num );
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Writes reached state BDD into a BLIF file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Llb_ManDumpReached( DdManager * ddG, DdNode * bReached, char * pModel, char * pFileName )
{
    FILE * pFile;
    Vec_Ptr_t * vNamesIn, * vNamesOut;
    char * pName;
    int i, nDigits;
    // reorder the BDD
    Cudd_ReduceHeap( ddG, CUDD_REORDER_SYMM_SIFT, 1 );

    // create input names
    nDigits = Abc_Base10Log( Cudd_ReadSize(ddG) );
    vNamesIn = Vec_PtrAlloc( Cudd_ReadSize(ddG) );
    for ( i = 0; i < Cudd_ReadSize(ddG); i++ )
    {
        pName = Llb_ManGetDummyName( "ff", i, nDigits );
        Vec_PtrPush( vNamesIn, Extra_UtilStrsav(pName) );
    }
    // create output names
    vNamesOut = Vec_PtrAlloc( 1 );
    Vec_PtrPush( vNamesOut, Extra_UtilStrsav("Reached") );

    // write the file
    pFile = fopen( pFileName, "wb" );
    Cudd_DumpBlif( ddG, 1, &bReached, (char **)Vec_PtrArray(vNamesIn), (char **)Vec_PtrArray(vNamesOut), pModel, pFile, 0 );
    fclose( pFile );

    // cleanup
    Vec_PtrForEachEntry( char *, vNamesIn, pName, i )
        ABC_FREE( pName );
    Vec_PtrForEachEntry( char *, vNamesOut, pName, i )
        ABC_FREE( pName );
    Vec_PtrFree( vNamesIn );
    Vec_PtrFree( vNamesOut );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

