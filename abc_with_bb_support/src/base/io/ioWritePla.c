/**CFile****************************************************************

  FileName    [ioWritePla.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write the network in BENCH format.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWritePla.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "io.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int    Io_WritePlaOne( FILE * pFile, Abc_Ntk_t * pNtk );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writes the network in PLA format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WritePla( Abc_Ntk_t * pNtk, char * pFileName )
{
    Abc_Ntk_t * pExdc;
    FILE * pFile;

    assert( Abc_NtkIsSopNetlist(pNtk) );
    assert( Abc_NtkLevel(pNtk) == 1 );

    pFile = fopen( pFileName, "w" );
    if ( pFile == NULL )
    {
        fprintf( stdout, "Io_WritePla(): Cannot open the output file.\n" );
        return 0;
    }
    fprintf( pFile, "# Benchmark \"%s\" written by ABC on %s\n", pNtk->pName, Extra_TimeStamp() );
    // write the network
    Io_WritePlaOne( pFile, pNtk );
    // write EXDC network if it exists
    pExdc = Abc_NtkExdc( pNtk );
    if ( pExdc )
        printf( "Io_WritePla: EXDC is not written (warning).\n" );
    // finalize the file
    fclose( pFile );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Writes the network in PLA format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WritePlaOne( FILE * pFile, Abc_Ntk_t * pNtk )
{
    ProgressBar * pProgress;
    Abc_Obj_t * pNode, * pFanin, * pDriver;
    char * pCubeIn, * pCubeOut, * pCube;
    int i, k, nProducts, nInputs, nOutputs, nFanins;

    nProducts = 0;
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        pDriver = Abc_ObjFanin0Ntk( Abc_ObjFanin0(pNode) );
        if ( !Abc_ObjIsNode(pDriver) )
        {
            nProducts++;
            continue;
        }
        if ( Abc_NodeIsConst(pDriver) )
        {
            if ( Abc_NodeIsConst1(pDriver) )
                nProducts++;
            continue;
        }
        nProducts += Abc_SopGetCubeNum(pDriver->pData);
    }

    // collect the parameters
    nInputs  = Abc_NtkCiNum(pNtk);
    nOutputs = Abc_NtkCoNum(pNtk);
    pCubeIn  = ALLOC( char, nInputs + 1 );
    pCubeOut = ALLOC( char, nOutputs + 1 );
    memset( pCubeIn,  '-', nInputs );     pCubeIn[nInputs]   = 0;
    memset( pCubeOut, '0', nOutputs );    pCubeOut[nOutputs] = 0;

    // write the header
    fprintf( pFile, ".i %d\n", nInputs );
    fprintf( pFile, ".o %d\n", nOutputs );
    fprintf( pFile, ".ilb" );
    Abc_NtkForEachCi( pNtk, pNode, i )
        fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanout0(pNode)) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".ob" );
    Abc_NtkForEachCo( pNtk, pNode, i )
        fprintf( pFile, " %s", Abc_ObjName(Abc_ObjFanin0(pNode)) );
    fprintf( pFile, "\n" );
    fprintf( pFile, ".p %d\n", nProducts );

    // mark the CI nodes
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->pCopy = (Abc_Obj_t *)i;

    // write the cubes
    pProgress = Extra_ProgressBarStart( stdout, nOutputs );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        // prepare the output cube
        if ( i - 1 >= 0 )
            pCubeOut[i-1] = '0';
        pCubeOut[i] = '1';

        // consider special cases of nodes
        pDriver = Abc_ObjFanin0Ntk( Abc_ObjFanin0(pNode) );
        if ( !Abc_ObjIsNode(pDriver) )
        {
            assert( Abc_ObjIsCi(pDriver) );
            pCubeIn[(int)pDriver->pCopy] = '1' - Abc_ObjFaninC0(pNode);
            fprintf( pFile, "%s %s\n", pCubeIn, pCubeOut );
            pCubeIn[(int)pDriver->pCopy] = '-';
            continue;
        }
        if ( Abc_NodeIsConst(pDriver) )
        {
            if ( Abc_NodeIsConst1(pDriver) )
                fprintf( pFile, "%s %s\n", pCubeIn, pCubeOut );
            continue;
        }

        // make sure the cover is not complemented
        assert( !Abc_SopIsComplement( pDriver->pData ) );

        // write the cubes
        nFanins = Abc_ObjFaninNum(pDriver);
        Abc_SopForEachCube( pDriver->pData, nFanins, pCube )
        {
            Abc_ObjForEachFanin( pDriver, pFanin, k )
            {
                pFanin = Abc_ObjFanin0Ntk(pFanin);
                assert( (int)pFanin->pCopy < nInputs );
                pCubeIn[(int)pFanin->pCopy] = pCube[k];
            }
            fprintf( pFile, "%s %s\n", pCubeIn, pCubeOut );
        }
        // clean the cube for future writing
        Abc_ObjForEachFanin( pDriver, pFanin, k )
        {
            pFanin = Abc_ObjFanin0Ntk(pFanin);
            assert( Abc_ObjIsCi(pFanin) );
            pCubeIn[(int)pFanin->pCopy] = '-';
        }
        Extra_ProgressBarUpdate( pProgress, i, NULL );
    }
    Extra_ProgressBarStop( pProgress );
    fprintf( pFile, ".e\n" );

    // clean the CI nodes
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->pCopy = NULL;
    free( pCubeIn );
    free( pCubeOut );
    return 1;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


