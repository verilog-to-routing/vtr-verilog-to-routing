/**CFile****************************************************************

  FileName    [sscUtil.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT sweeping under constraints.]

  Synopsis    [Various utilities.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 29, 2008.]

  Revision    [$Id: sscUtil.c,v 1.00 2008/07/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "sscInt.h"
#include "sat/cnf/cnf.h"
#include "aig/gia/giaAig.h"

ABC_NAMESPACE_IMPL_START


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
Gia_Man_t * Gia_ManDropContained( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Aig_Man_t * pMan = Gia_ManToAigSimple( p );
    Cnf_Dat_t * pDat = Cnf_Derive( pMan, Gia_ManPoNum(p) );
    Gia_Obj_t * pObj;
    Vec_Int_t * vLits, * vKeep;
    sat_solver * pSat;
    int ConstLit = Abc_Var2Lit(pDat->pVarNums[0], 0);
    int i, status;//, Count = 0;
    Aig_ManStop( pMan );

    vLits = Vec_IntAlloc( Gia_ManPoNum(p) );
    Gia_ManForEachPo( p, pObj, i )
    {
        int iObj = Gia_ObjId( p, pObj );
        Vec_IntPush( vLits, Abc_Var2Lit(pDat->pVarNums[iObj], 1) );
    }

    // start the SAT solver
    pSat = sat_solver_new();
    sat_solver_setnvars( pSat, pDat->nVars );
    for ( i = 0; i < pDat->nClauses; i++ )
    {
        if ( !sat_solver_addclause( pSat, pDat->pClauses[i], pDat->pClauses[i+1] ) )
        {
            Cnf_DataFree( pDat );
            sat_solver_delete( pSat );
            Vec_IntFree( vLits );
            return NULL;
        }
    }
    Cnf_DataFree( pDat );
    status = sat_solver_simplify( pSat );
    if ( status == 0 )
    {
        sat_solver_delete( pSat );
        Vec_IntFree( vLits );
        return NULL;
    }

    // iterate over POs
    vKeep = Vec_IntAlloc( Gia_ManPoNum(p) );
    Gia_ManForEachPo( p, pObj, i )
    {
        Vec_IntWriteEntry( vLits, i, Abc_LitNot(Vec_IntEntry(vLits,i)) );
        status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits), 0, 0, 0, 0 );
        Vec_IntWriteEntry( vLits, i, Abc_LitNot(Vec_IntEntry(vLits,i)) );
        if ( status == l_False )
            Vec_IntWriteEntry( vLits, i, ConstLit ); // const1 SAT var is always true
        else 
        {
            assert( status == l_True );
            Vec_IntPush( vKeep, i );
        }
    }
    sat_solver_delete( pSat );
    Vec_IntFree( vLits );
    if ( Vec_IntSize(vKeep) == Gia_ManPoNum(p) )
    {
        Vec_IntFree( vKeep );
        return Gia_ManDup(p);
    }
    pNew = Gia_ManDupCones( p, Vec_IntArray(vKeep), Vec_IntSize(vKeep), 0 );
    Vec_IntFree( vKeep );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManOptimizeRing( Gia_Man_t * p )
{
    Ssc_Pars_t Pars, * pPars = &Pars;
    Gia_Man_t * pTemp, * pAux;
    int i;
    assert( p->nConstrs == 0 );
    printf( "User AIG: " );
    Gia_ManPrintStats( p, NULL );
    pTemp = Gia_ManDropContained( p );
    printf( "Drop AIG: " );
    Gia_ManPrintStats( pTemp, NULL );
//    return pTemp;
    if ( Gia_ManPoNum(pTemp) == 1 )
        return pTemp;
    Ssc_ManSetDefaultParams( pPars );
    pPars->fAppend  = 1;
    pPars->fVerbose = 0;
    pTemp->nConstrs = Gia_ManPoNum(pTemp) - 1;    
    for ( i = 0; i < Gia_ManPoNum(pTemp); i++ )
    {
        // move i-th PO forward
        Gia_ManSwapPos( pTemp, i );
        pTemp = Gia_ManDupDfs( pAux = pTemp );
        Gia_ManStop( pAux );
        // minimize this PO
        pTemp = Ssc_PerformSweepingConstr( pAux = pTemp, pPars );
        Gia_ManStop( pAux );
        pTemp = Gia_ManDupDfs( pAux = pTemp );
        Gia_ManStop( pAux );
        // move i-th PO back
        Gia_ManSwapPos( pTemp, i );
        pTemp = Gia_ManDupDfs( pAux = pTemp );
        Gia_ManStop( pAux );
        // report results
        printf( "AIG%3d  : ", i );
        Gia_ManPrintStats( pTemp, NULL );
    }
    pTemp->nConstrs = 0;
    return pTemp;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

