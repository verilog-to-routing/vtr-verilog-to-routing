/**CFile****************************************************************

  FileName    [abcCas.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Decomposition of shared BDDs into LUT cascade.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcCas.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"

/* 
    This LUT cascade synthesis algorithm is described in the paper:
    A. Mishchenko and T. Sasao, "Encoding of Boolean functions and its 
    application to LUT cascade synthesis", Proc. IWLS '02, pp. 115-120.
    http://www.eecs.berkeley.edu/~alanmi/publications/2002/iwls02_enc.pdf
*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern int Abc_CascadeExperiment( char * pFileGeneric, DdManager * dd, DdNode ** pOutputs, int nInputs, int nOutputs, int nLutSize, int fCheck, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkCascade( Abc_Ntk_t * pNtk, int nLutSize, int fCheck, int fVerbose )
{
    DdManager * dd;
    DdNode ** ppOutputs;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pNode;
    char * pFileGeneric;
    int fBddSizeMax = 500000;
    int fReorder = 1;
    int i, clk = clock();

    assert( Abc_NtkIsStrash(pNtk) );
    // compute the global BDDs
    if ( Abc_NtkBuildGlobalBdds(pNtk, fBddSizeMax, 1, fReorder, fVerbose) == NULL )
        return NULL;

    if ( fVerbose )
    {
        DdManager * dd = Abc_NtkGlobalBddMan( pNtk );
        printf( "Shared BDD size = %6d nodes.  ", Cudd_ReadKeys(dd) - Cudd_ReadDead(dd) );
        PRT( "BDD construction time", clock() - clk );
    }

    // collect global BDDs
    dd = Abc_NtkGlobalBddMan( pNtk );
    ppOutputs = ALLOC( DdNode *, Abc_NtkCoNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pNode, i )
        ppOutputs[i] = Abc_ObjGlobalBdd(pNode);

    // call the decomposition
    pFileGeneric = Extra_FileNameGeneric( pNtk->pSpec );
    if ( !Abc_CascadeExperiment( pFileGeneric, dd, ppOutputs, Abc_NtkCiNum(pNtk), Abc_NtkCoNum(pNtk), nLutSize, fCheck, fVerbose ) )
    {
        // the LUT size is too small
    }

    // for now, duplicate the network
    pNtkNew = Abc_NtkDup( pNtk );

    // cleanup
    Abc_NtkFreeGlobalBdds( pNtk, 1 );
    free( ppOutputs );
    free( pFileGeneric );

//    if ( pNtk->pExdc )
//        pNtkNew->pExdc = Abc_NtkDup( pNtk->pExdc );
    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkCollapse: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


