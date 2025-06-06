/**CFile****************************************************************

  FileName    [ioWriteCnf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to output CNF of the miter cone.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWriteCnf.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "ioAbc.h"
#include "sat/bsat/satSolver.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
 
static Abc_Ntk_t * s_pNtk = NULL;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Write the miter cone into a CNF file for the SAT solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Io_WriteCnf( Abc_Ntk_t * pNtk, char * pFileName, int fAllPrimes )
{
    sat_solver * pSat;
    if ( Abc_NtkIsStrash(pNtk) )
        printf( "Io_WriteCnf() warning: Generating CNF by applying heuristic AIG to CNF conversion.\n" );
    else
        printf( "Io_WriteCnf() warning: Generating CNF by convering logic nodes into CNF clauses.\n" );
    if ( Abc_NtkPoNum(pNtk) != 1 )
    {
        fprintf( stdout, "Io_WriteCnf(): Currently can only process the miter (the network with one PO).\n" );
        return 0;
    }
    if ( Abc_NtkLatchNum(pNtk) != 0 )
    {
        fprintf( stdout, "Io_WriteCnf(): Currently can only process the miter for combinational circuits.\n" );
        return 0;
    }
    if ( Abc_NtkNodeNum(pNtk) == 0 )
    {
        fprintf( stdout, "The network has no logic nodes. No CNF file is generaled.\n" );
        return 0;
    }
    // convert to logic BDD network
    if ( Abc_NtkIsLogic(pNtk) )
        Abc_NtkToBdd( pNtk );
    // create solver with clauses
    pSat = (sat_solver *)Abc_NtkMiterSatCreate( pNtk, fAllPrimes );
    if ( pSat == NULL )
    {
        fprintf( stdout, "The problem is trivially UNSAT. No CNF file is generated.\n" );
        return 1;
    }        
    // write the clauses
    s_pNtk = pNtk;
    Sat_SolverWriteDimacs( pSat, pFileName, 0, 0, 1 );
    s_pNtk = NULL;
    // free the solver
    sat_solver_delete( pSat );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Output the mapping of PIs into variable numbers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteCnfOutputPiMapping( FILE * pFile, int incrementVars )
{
    extern Vec_Int_t * Abc_NtkGetCiSatVarNums( Abc_Ntk_t * pNtk );
    Abc_Ntk_t * pNtk = s_pNtk;
    Vec_Int_t * vCiIds;
    Abc_Obj_t * pObj;
    int i;
    vCiIds = Abc_NtkGetCiSatVarNums( pNtk );
    fprintf( pFile, "c PI variable numbers: <PI_name> <SAT_var_number>\n" );
    Abc_NtkForEachCi( pNtk, pObj, i )
        fprintf( pFile, "c %s %d\n", Abc_ObjName(pObj), Vec_IntEntry(vCiIds, i) + (int)(incrementVars > 0) );
    Vec_IntFree( vCiIds );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

