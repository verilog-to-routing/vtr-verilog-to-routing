/**CFile****************************************************************

  FileName    [ioReadPla.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedure to read network from file.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioReadPla.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "io.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Abc_Ntk_t * Io_ReadPlaNetwork( Extra_FileReader_t * p );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads the network from a PLA file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadPla( char * pFileName, int fCheck )
{
    Extra_FileReader_t * p;
    Abc_Ntk_t * pNtk;

    // start the file
    p = Extra_FileReaderAlloc( pFileName, "#", "\n\r", " \t|" );
    if ( p == NULL )
        return NULL;

    // read the network
    pNtk = Io_ReadPlaNetwork( p );
    Extra_FileReaderFree( p );
    if ( pNtk == NULL )
        return NULL;

    // make sure that everything is okay with the network structure
    if ( fCheck && !Abc_NtkCheckRead( pNtk ) )
    {
        printf( "Io_ReadPla: The network check has failed.\n" );
        Abc_NtkDelete( pNtk );
        return NULL;
    }
    return pNtk;
}
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Io_ReadPlaNetwork( Extra_FileReader_t * p )
{
    ProgressBar * pProgress;
    Vec_Ptr_t * vTokens;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pTermPi, * pTermPo, * pNode;
    Vec_Str_t ** ppSops;
    char Buffer[100];
    int nInputs = -1, nOutputs = -1, nProducts = -1;
    char * pCubeIn, * pCubeOut;
    int i, k, iLine, nDigits, nCubes;
 
    // allocate the empty network
    pNtk = Abc_NtkStartRead( Extra_FileReaderGetFileName(p) );

    // go through the lines of the file
    nCubes = 0;
    pProgress = Extra_ProgressBarStart( stdout, Extra_FileReaderGetFileSize(p) );
    for ( iLine = 0; vTokens = Extra_FileReaderGetTokens(p); iLine++ )
    {
        Extra_ProgressBarUpdate( pProgress, Extra_FileReaderGetCurPosition(p), NULL );

        // if it is the end of file, quit the loop
        if ( strcmp( vTokens->pArray[0], ".e" ) == 0 )
            break;

        if ( vTokens->nSize == 1 )
        {
            printf( "%s (line %d): Wrong number of token.\n", 
                Extra_FileReaderGetFileName(p), iLine+1 );
            Abc_NtkDelete( pNtk );
            return NULL;
        }

        if ( strcmp( vTokens->pArray[0], ".i" ) == 0 )
            nInputs = atoi(vTokens->pArray[1]);
        else if ( strcmp( vTokens->pArray[0], ".o" ) == 0 )
            nOutputs = atoi(vTokens->pArray[1]);
        else if ( strcmp( vTokens->pArray[0], ".p" ) == 0 )
            nProducts = atoi(vTokens->pArray[1]);
        else if ( strcmp( vTokens->pArray[0], ".ilb" ) == 0 )
        {
            if ( vTokens->nSize - 1 != nInputs )
                printf( "Warning: Mismatch between the number of PIs on the .i line (%d) and the number of PIs on the .ilb line (%d).\n", nInputs, vTokens->nSize - 1 );
            for ( i = 1; i < vTokens->nSize; i++ )
                Io_ReadCreatePi( pNtk, vTokens->pArray[i] );
        }
        else if ( strcmp( vTokens->pArray[0], ".ob" ) == 0 )
        {
            if ( vTokens->nSize - 1 != nOutputs )
                printf( "Warning: Mismatch between the number of POs on the .o line (%d) and the number of POs on the .ob line (%d).\n", nOutputs, vTokens->nSize - 1 );
            for ( i = 1; i < vTokens->nSize; i++ )
                Io_ReadCreatePo( pNtk, vTokens->pArray[i] );
        }
        else 
        {
            // check if the input/output names are given
            if ( Abc_NtkPiNum(pNtk) == 0 )
            {
                if ( nInputs == -1 )
                {
                    printf( "%s: The number of inputs is not specified.\n", Extra_FileReaderGetFileName(p) );
                    Abc_NtkDelete( pNtk );
                    return NULL;
                }
                nDigits = Extra_Base10Log( nInputs );
                for ( i = 0; i < nInputs; i++ )
                {
                    sprintf( Buffer, "x%0*d", nDigits, i );
                    Io_ReadCreatePi( pNtk, Buffer );
                }
            }
            if ( Abc_NtkPoNum(pNtk) == 0 )
            {
                if ( nOutputs == -1 )
                {
                    printf( "%s: The number of outputs is not specified.\n", Extra_FileReaderGetFileName(p) );
                    Abc_NtkDelete( pNtk );
                    return NULL;
                }
                nDigits = Extra_Base10Log( nOutputs );
                for ( i = 0; i < nOutputs; i++ )
                {
                    sprintf( Buffer, "z%0*d", nDigits, i );
                    Io_ReadCreatePo( pNtk, Buffer );
                }
            }
            if ( Abc_NtkNodeNum(pNtk) == 0 )
            { // first time here
                // create the PO drivers and add them
                // start the SOP covers
                ppSops = ALLOC( Vec_Str_t *, nOutputs );
                Abc_NtkForEachPo( pNtk, pTermPo, i )
                {
                    ppSops[i] = Vec_StrAlloc( 100 );
                    // create the node
                    pNode = Abc_NtkCreateNode(pNtk);
                    // connect the node to the PO net
                    Abc_ObjAddFanin( Abc_ObjFanin0Ntk(pTermPo), pNode );
                    // connect the node to the PI nets
                    Abc_NtkForEachPi( pNtk, pTermPi, k )
                        Abc_ObjAddFanin( pNode, Abc_ObjFanout0Ntk(pTermPi) );
                }
            }
            // read the cubes
            if ( vTokens->nSize != 2 )
            {
                printf( "%s (line %d): Input and output cubes are not specified.\n", 
                    Extra_FileReaderGetFileName(p), iLine+1 );
                Abc_NtkDelete( pNtk );
                return NULL;
            }
            pCubeIn  = vTokens->pArray[0];
            pCubeOut = vTokens->pArray[1];
            if ( strlen(pCubeIn) != (unsigned)nInputs )
            {
                printf( "%s (line %d): Input cube length (%d) differs from the number of inputs (%d).\n", 
                    Extra_FileReaderGetFileName(p), iLine+1, strlen(pCubeIn), nInputs );
                Abc_NtkDelete( pNtk );
                return NULL;
            }
            if ( strlen(pCubeOut) != (unsigned)nOutputs )
            {
                printf( "%s (line %d): Output cube length (%d) differs from the number of outputs (%d).\n", 
                    Extra_FileReaderGetFileName(p), iLine+1, strlen(pCubeOut), nOutputs );
                Abc_NtkDelete( pNtk );
                return NULL;
            }
            for ( i = 0; i < nOutputs; i++ )
            {
                if ( pCubeOut[i] == '1' )
                {
                    Vec_StrAppend( ppSops[i], pCubeIn );
                    Vec_StrAppend( ppSops[i], " 1\n" );
                }
            }
            nCubes++;
        }
    }
    Extra_ProgressBarStop( pProgress );
    if ( nProducts != -1 && nCubes != nProducts )
        printf( "Warning: Mismatch between the number of cubes (%d) and the number on .p line (%d).\n", 
            nCubes, nProducts );

    // add the SOP covers
    Abc_NtkForEachPo( pNtk, pTermPo, i )
    {
        pNode = Abc_ObjFanin0Ntk( Abc_ObjFanin0(pTermPo) );
        if ( ppSops[i]->nSize == 0 )
        {
            Abc_ObjRemoveFanins(pNode);
            pNode->pData = Abc_SopRegister( pNtk->pManFunc, " 0\n" );
            Vec_StrFree( ppSops[i] );
            continue;
        }
        Vec_StrPush( ppSops[i], 0 );
        pNode->pData = Abc_SopRegister( pNtk->pManFunc, ppSops[i]->pArray );
        Vec_StrFree( ppSops[i] );
    }
    free( ppSops );
    Abc_NtkFinalizeRead( pNtk );
    return pNtk;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////



