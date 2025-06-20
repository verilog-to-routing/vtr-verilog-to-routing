/**CFile****************************************************************

  FileName    [intCtrex.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Interpolation engine.]

  Synopsis    [Counter-example generation after disproving the property.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 24, 2008.]

  Revision    [$Id: intCtrex.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "intInt.h"
#include "proof/ssw/ssw.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Unroll the circuit the given number of timeframes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Inter_ManFramesBmc( Aig_Man_t * pAig, int nFrames )
{
    Aig_Man_t * pFrames;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i, f;
    assert( Saig_ManRegNum(pAig) > 0 );
    assert( Saig_ManPoNum(pAig) == 1 );
    pFrames = Aig_ManStart( Aig_ManNodeNum(pAig) * nFrames );
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pFrames );
    // create variables for register outputs
    Saig_ManForEachLo( pAig, pObj, i )
        pObj->pData = Aig_ManConst0( pFrames );
    // add timeframes
    for ( f = 0; f < nFrames; f++ )
    {
        // create PI nodes for this frame
        Saig_ManForEachPi( pAig, pObj, i )
            pObj->pData = Aig_ObjCreateCi( pFrames );
        // add internal nodes of this frame
        Aig_ManForEachNode( pAig, pObj, i )
            pObj->pData = Aig_And( pFrames, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
        if ( f == nFrames - 1 )
            break;
        // transfer to register outputs
        Saig_ManForEachLiLo(  pAig, pObjLi, pObjLo, i )
            pObjLi->pData = Aig_ObjChild0Copy(pObjLi);
        // transfer to register outputs
        Saig_ManForEachLiLo(  pAig, pObjLi, pObjLo, i )
            pObjLo->pData = pObjLi->pData;
    }
    // create POs for the output of the last frame
    pObj = Aig_ManCo( pAig, 0 );
    Aig_ObjCreateCo( pFrames, Aig_ObjChild0Copy(pObj) );
    Aig_ManCleanup( pFrames );
    return pFrames;
}

/**Function*************************************************************

  Synopsis    [Run the SAT solver on the unrolled instance.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Inter_ManGetCounterExample( Aig_Man_t * pAig, int nFrames, int fVerbose )
{
    int nConfLimit = 1000000;
    Abc_Cex_t * pCtrex = NULL;
    Aig_Man_t * pFrames;
    sat_solver * pSat;
    Cnf_Dat_t * pCnf;
    int status;
    abctime clk = Abc_Clock();
    Vec_Int_t * vCiIds;
    // create timeframes
    assert( Saig_ManPoNum(pAig) == 1 );
    pFrames = Inter_ManFramesBmc( pAig, nFrames );
    // derive CNF
    pCnf = Cnf_Derive( pFrames, 0 );
    Cnf_DataTranformPolarity( pCnf, 0 );
    vCiIds = Cnf_DataCollectPiSatNums( pCnf, pFrames );
    Aig_ManStop( pFrames );
    // convert into SAT solver
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    Cnf_DataFree( pCnf );
    if ( pSat == NULL )
    {
        printf( "Counter-example generation in command \"int\" has failed.\n" );
        printf( "Use command \"bmc2\" to produce a valid counter-example.\n" );
        Vec_IntFree( vCiIds );
        return NULL;
    }
    // simplify the problem
    status = sat_solver_simplify(pSat);
    if ( status == 0 )
    {
        Vec_IntFree( vCiIds );
        sat_solver_delete( pSat );
        return NULL;
    }
    // solve the miter
    status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    // if the problem is SAT, get the counterexample
    if ( status == l_True )
    {
        int i, * pModel = Sat_SolverGetModel( pSat, vCiIds->pArray, vCiIds->nSize );
        pCtrex = Abc_CexAlloc( Saig_ManRegNum(pAig), Saig_ManPiNum(pAig), nFrames );
        pCtrex->iFrame = nFrames - 1;
        pCtrex->iPo = 0;
        for ( i = 0; i < Vec_IntSize(vCiIds); i++ )
            if ( pModel[i] )
                Abc_InfoSetBit( pCtrex->pData, Saig_ManRegNum(pAig) + i );
        ABC_FREE( pModel );
    }
    // free the sat_solver
    sat_solver_delete( pSat );
    Vec_IntFree( vCiIds );
    // verify counter-example
    status = Saig_ManVerifyCex( pAig, pCtrex );
    if ( status == 0 )
        printf( "Inter_ManGetCounterExample(): Counter-example verification has FAILED.\n" );
    // report the results
    if ( fVerbose )
    {
        ABC_PRT( "Total ctrex generation time", Abc_Clock() - clk );
    }
    return pCtrex;

}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

