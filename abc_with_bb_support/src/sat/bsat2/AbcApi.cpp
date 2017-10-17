/**CFile****************************************************************

  FileName    [AbcApi.cpp]

  PackageName [A C++ version of SAT solver MiniSAT 2.2 developed 
  by Niklas Sorensson and Niklas Een. http://minisat.se.]

  Synopsis    [Interface to the SAT solver.]

  Author      [Niklas Sorensson and Niklas Een.]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2004.]

  Revision    [$Id: AbcApi.cpp,v 1.0 2004/01/01 1:00:00 alanmi Exp $]

***********************************************************************/

#include "Solver.h"
#include "sat/cnf/cnf.h"

ABC_NAMESPACE_IMPL_START

using namespace Minisat;

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_CallMiniSat22( Cnf_Dat_t * p )
{
    Solver S;
    int Result = -1;
    return Result;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END
