/**CFile****************************************************************

  FileName    [intUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Various interpolation utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 24, 2008.]

  Revision    [$Id: intUtil.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "intInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Returns 1 if the property fails in the initial state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Inter_ManCheckInitialState( Aig_Man_t * p )
{
    Cnf_Dat_t * pCnf;
    Aig_Obj_t * pObj;
    sat_solver * pSat;
    int i, status;
    //abctime clk = Abc_Clock();
    pCnf = Cnf_Derive( p, Saig_ManRegNum(p) ); 
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 1 );
    if ( pSat == NULL )
    {
        Cnf_DataFree( pCnf );
        return 0;
    }
    status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    //ABC_PRT( "Time", Abc_Clock() - clk );
    if ( status == l_True )
    {
        p->pSeqModel = Abc_CexAlloc( Aig_ManRegNum(p), Saig_ManPiNum(p), 1 );
        Saig_ManForEachPi( p, pObj, i )
            if ( sat_solver_var_value( pSat, pCnf->pVarNums[Aig_ObjId(pObj)] ) )
                Abc_InfoSetBit( p->pSeqModel->pData, Aig_ManRegNum(p) + i );
    }
    Cnf_DataFree( pCnf );
    sat_solver_delete( pSat );
    return status == l_True;
}

/**Function*************************************************************

  Synopsis    [Returns 1 if the property holds in all states.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Inter_ManCheckAllStates( Aig_Man_t * p )
{
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    int status;
    abctime clk = Abc_Clock();
    pCnf = Cnf_Derive( p, Saig_ManRegNum(p) ); 
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    Cnf_DataFree( pCnf );
    if ( pSat == NULL )
        return 1;
    status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    sat_solver_delete( pSat );
    ABC_PRT( "Time", Abc_Clock() - clk );
    return status == l_False;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

