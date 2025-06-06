/**CFile****************************************************************

  FileName    [bmcEnum.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [SAT-based bounded model checking.]

  Synopsis    [Enumeration.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: bmcEnum.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "bmc.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"

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
Gia_Man_t * Gia_ManDeriveOne( Gia_Man_t * p, Vec_Int_t * vValues )
{
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj; int i, fPhase0, fPhase1;
    assert( Vec_IntSize(vValues) == Gia_ManCiNum(p) );
    // propagate forward
    Gia_ManForEachCi( p, pObj, i )
        pObj->fPhase = Vec_IntEntry(vValues, i);
    Gia_ManForEachAnd( p, pObj, i )
    {
        fPhase0 = Gia_ObjPhase(Gia_ObjFanin0(pObj)) ^ Gia_ObjFaninC0(pObj);
        fPhase1 = Gia_ObjPhase(Gia_ObjFanin1(pObj)) ^ Gia_ObjFaninC1(pObj);
        pObj->fPhase = fPhase0 & fPhase1;
    }
    // propagate backward
    Gia_ManCleanMark0(p);
    Gia_ManForEachCo( p, pObj, i )
        Gia_ObjFanin0(pObj)->fMark0 = 1;
    Gia_ManForEachAndReverse( p, pObj, i )
    {
        if ( !pObj->fMark0 )
            continue;
        fPhase0 = Gia_ObjPhase(Gia_ObjFanin0(pObj)) ^ Gia_ObjFaninC0(pObj);
        fPhase1 = Gia_ObjPhase(Gia_ObjFanin1(pObj)) ^ Gia_ObjFaninC1(pObj);
        if ( fPhase0 == fPhase1 )
        {
            assert( (int)pObj->fPhase == fPhase0 );
            Gia_ObjFanin0(pObj)->fMark0 = 1;
            Gia_ObjFanin1(pObj)->fMark0 = 1;
        }
        else if ( fPhase0 )
        {
            assert( fPhase1 == 0 );
            assert( pObj->fPhase == 0 );
            Gia_ObjFanin1(pObj)->fMark0 = 1;
        }
        else if ( fPhase1 )
        {
            assert( fPhase0 == 0 );
            assert( pObj->fPhase == 0 );
            Gia_ObjFanin0(pObj)->fMark0 = 1;
        }
    }
    // create new
    Gia_ManFillValue( p );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Abc_LitNotCond( Gia_ManAppendCi(pNew), !Vec_IntEntry(vValues, i) );
    Gia_ManForEachAnd( p, pObj, i )
    {
        if ( !pObj->fMark0 )
            continue;
        fPhase0 = Gia_ObjPhase(Gia_ObjFanin0(pObj)) ^ Gia_ObjFaninC0(pObj);
        fPhase1 = Gia_ObjPhase(Gia_ObjFanin1(pObj)) ^ Gia_ObjFaninC1(pObj);
        if ( fPhase0 == fPhase1 )
        {
            assert( (int)pObj->fPhase == fPhase0 );
            if ( pObj->fPhase )
                pObj->Value = Gia_ManAppendAnd( pNew, Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value );
            else
                pObj->Value = Gia_ManAppendOr( pNew, Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value );
        }
        else if ( fPhase0 )
        {
            assert( fPhase1 == 0 );
            assert( pObj->fPhase == 0 );
            pObj->Value = Gia_ObjFanin1(pObj)->Value;
        }
        else if ( fPhase1 )
        {
            assert( fPhase0 == 0 );
            assert( pObj->fPhase == 0 );
            pObj->Value = Gia_ObjFanin0(pObj)->Value;
        }
    }
    Gia_ManForEachCo( p, pObj, i )
        Gia_ManAppendCo( pNew, Gia_ObjFanin0(pObj)->Value );
    Gia_ManSetRegNum( pNew, Gia_ManRegNum(p) );
    Gia_ManCleanMark0(p);
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_ManDeriveOneTest2( Gia_Man_t * p )
{
    Gia_Man_t * pNew;
    Vec_Int_t * vValues = Vec_IntStart( Gia_ManCiNum(p) );
    //Vec_IntFill( vValues, Gia_ManCiNum(p), 1 );
    pNew = Gia_ManDeriveOne( p, vValues );
    Vec_IntFree( vValues );
    return pNew;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_ManDeriveOneTest( Gia_Man_t * p )
{
    int fVerbose = 1;
    Gia_Man_t * pNew;
    Gia_Obj_t * pObj, * pRoot;
    Vec_Int_t * vValues = Vec_IntStart( Gia_ManCiNum(p) );
    Cnf_Dat_t * pCnf = (Cnf_Dat_t *)Mf_ManGenerateCnf( p, 8, 0, 0, 0, 0 );
    int i, iVar, nIter, iPoVarBeg = pCnf->nVars - Gia_ManCiNum(p);
    sat_solver * pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    Vec_Int_t * vLits = Vec_IntAlloc( 100 );
    Vec_IntPush( vLits, Abc_Var2Lit( 1, 1 ) );
    for ( nIter = 0; nIter < 10000; nIter++ )
    {
        int status = sat_solver_solve( pSat, Vec_IntArray(vLits), Vec_IntLimit(vLits), 0, 0, 0, 0 );
        if ( status == l_False ) 
            break;
        // derive new set
        assert( status == l_True );
        for ( i = 0; i < Gia_ManCiNum(p); i++ )
        {
            Vec_IntWriteEntry( vValues, i, sat_solver_var_value(pSat, iPoVarBeg+i) );
            printf( "%d", sat_solver_var_value(pSat, iPoVarBeg+i) );
        }
        printf( " : " );

        pNew = Gia_ManDeriveOne( p, vValues );
        // assign variables
        Gia_ManForEachCi( pNew, pObj, i )
            pObj->Value = iPoVarBeg+i;
        iVar = sat_solver_nvars(pSat);
        Gia_ManForEachAnd( pNew, pObj, i )
            pObj->Value = iVar++;
        sat_solver_setnvars( pSat, iVar );
        // create new clauses
        Gia_ManForEachAnd( pNew, pObj, i )
            sat_solver_add_and( pSat, pObj->Value, Gia_ObjFanin0(pObj)->Value, Gia_ObjFanin1(pObj)->Value, Gia_ObjFaninC0(pObj), Gia_ObjFaninC1(pObj), 0 );
        // add to the assumptions
        pRoot = Gia_ManCo(pNew, 0);
        Vec_IntPush( vLits, Abc_Var2Lit(Gia_ObjFanin0(pRoot)->Value, !Gia_ObjFaninC0(pRoot)) );
        if ( fVerbose )
        {
            printf( "Iter = %5d : ", nIter );
            printf( "And = %4d ", Gia_ManAndNum(pNew) );
            printf( "\n" );
        }
        Gia_ManStop( pNew );
    }
    Vec_IntFree( vLits );
    Vec_IntFree( vValues );
    Cnf_DataFree( pCnf );
    sat_solver_delete( pSat );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

