/**CFile****************************************************************

  FileName    [resSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Resynthesis package.]

  Synopsis    [Interface with the SAT solver.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 15, 2007.]

  Revision    [$Id: resSat.c,v 1.00 2007/01/15 00:00:00 alanmi Exp $]

***********************************************************************/

#include "abc.h"
#include "resInt.h"
#include "hop.h"
#include "satSolver.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern int Res_SatAddConst1( sat_solver * pSat, int iVar, int fCompl );
extern int Res_SatAddEqual( sat_solver * pSat, int iVar0, int iVar1, int fCompl );
extern int Res_SatAddAnd( sat_solver * pSat, int iVar, int iVar0, int iVar1, int fCompl0, int fCompl1 );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Loads AIG into the SAT solver for checking resubstitution.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Res_SatProveUnsat( Abc_Ntk_t * pAig, Vec_Ptr_t * vFanins )
{
    void * pCnf = NULL;
    sat_solver * pSat;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i, nNodes, status;

    // make sure fanins contain POs of the AIG
    pObj = Vec_PtrEntry( vFanins, 0 );
    assert( pObj->pNtk == pAig && Abc_ObjIsPo(pObj) );

    // collect reachable nodes
    vNodes = Abc_NtkDfsNodes( pAig, (Abc_Obj_t **)vFanins->pArray, vFanins->nSize );

    // assign unique numbers to each node
    nNodes = 0;
    Abc_AigConst1(pAig)->pCopy = (void *)nNodes++;
    Abc_NtkForEachPi( pAig, pObj, i )
        pObj->pCopy = (void *)nNodes++;
    Vec_PtrForEachEntry( vNodes, pObj, i )
        pObj->pCopy = (void *)nNodes++;
    Vec_PtrForEachEntry( vFanins, pObj, i ) // useful POs
        pObj->pCopy = (void *)nNodes++;

    // start the solver
    pSat = sat_solver_new();
    sat_solver_store_alloc( pSat );

    // add clause for the constant node
    Res_SatAddConst1( pSat, (int)Abc_AigConst1(pAig)->pCopy, 0 );
    // add clauses for AND gates
    Vec_PtrForEachEntry( vNodes, pObj, i )
        Res_SatAddAnd( pSat, (int)pObj->pCopy, 
            (int)Abc_ObjFanin0(pObj)->pCopy, (int)Abc_ObjFanin1(pObj)->pCopy, Abc_ObjFaninC0(pObj), Abc_ObjFaninC1(pObj) );
    Vec_PtrFree( vNodes );
    // add clauses for POs
    Vec_PtrForEachEntry( vFanins, pObj, i )
        Res_SatAddEqual( pSat, (int)pObj->pCopy, 
            (int)Abc_ObjFanin0(pObj)->pCopy, Abc_ObjFaninC0(pObj) );
    // add trivial clauses
    pObj = Vec_PtrEntry(vFanins, 0);
    Res_SatAddConst1( pSat, (int)pObj->pCopy, 0 ); // care-set
    pObj = Vec_PtrEntry(vFanins, 1);
    Res_SatAddConst1( pSat, (int)pObj->pCopy, 0 ); // on-set

    // bookmark the clauses of A
    sat_solver_store_mark_clauses_a( pSat );

    // duplicate the clauses
    pObj = Vec_PtrEntry(vFanins, 1);
    Sat_SolverDoubleClauses( pSat, (int)pObj->pCopy );
    // add the equality constraints
    Vec_PtrForEachEntryStart( vFanins, pObj, i, 2 )
        Res_SatAddEqual( pSat, (int)pObj->pCopy, ((int)pObj->pCopy) + nNodes, 0 );

    // bookmark the roots
    sat_solver_store_mark_roots( pSat );

    // solve the problem
    status = sat_solver_solve( pSat, NULL, NULL, (sint64)10000, (sint64)0, (sint64)0, (sint64)0 );
    if ( status == l_False )
    {
        pCnf = sat_solver_store_release( pSat );
//        printf( "unsat\n" );
    }
    else if ( status == l_True )
    {
//        printf( "sat\n" );
    }
    else
    {
//        printf( "undef\n" );
    }
    sat_solver_delete( pSat );
    return pCnf;
}

/**Function*************************************************************

  Synopsis    [Loads AIG into the SAT solver for constrained simulation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Res_SatSimulateConstr( Abc_Ntk_t * pAig, int fOnSet )
{
    sat_solver * pSat;
    Vec_Ptr_t * vFanins;
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i, nNodes;

    // start the array
    vFanins = Vec_PtrAlloc( 2 );
    pObj = Abc_NtkPo( pAig, 0 );
    Vec_PtrPush( vFanins, pObj );
    pObj = Abc_NtkPo( pAig, 1 );
    Vec_PtrPush( vFanins, pObj );

    // collect reachable nodes
    vNodes = Abc_NtkDfsNodes( pAig, (Abc_Obj_t **)vFanins->pArray, vFanins->nSize );

    // assign unique numbers to each node
    nNodes = 0;
    Abc_AigConst1(pAig)->pCopy = (void *)nNodes++;
    Abc_NtkForEachPi( pAig, pObj, i )
        pObj->pCopy = (void *)nNodes++;
    Vec_PtrForEachEntry( vNodes, pObj, i )
        pObj->pCopy = (void *)nNodes++;
    Vec_PtrForEachEntry( vFanins, pObj, i ) // useful POs
        pObj->pCopy = (void *)nNodes++;

    // start the solver
    pSat = sat_solver_new();

    // add clause for the constant node
    Res_SatAddConst1( pSat, (int)Abc_AigConst1(pAig)->pCopy, 0 );
    // add clauses for AND gates
    Vec_PtrForEachEntry( vNodes, pObj, i )
        Res_SatAddAnd( pSat, (int)pObj->pCopy, 
            (int)Abc_ObjFanin0(pObj)->pCopy, (int)Abc_ObjFanin1(pObj)->pCopy, Abc_ObjFaninC0(pObj), Abc_ObjFaninC1(pObj) );
    Vec_PtrFree( vNodes );
    // add clauses for the first PO
    pObj = Abc_NtkPo( pAig, 0 );
    Res_SatAddEqual( pSat, (int)pObj->pCopy, 
        (int)Abc_ObjFanin0(pObj)->pCopy, Abc_ObjFaninC0(pObj) );
    // add clauses for the second PO
    pObj = Abc_NtkPo( pAig, 1 );
    Res_SatAddEqual( pSat, (int)pObj->pCopy, 
        (int)Abc_ObjFanin0(pObj)->pCopy, Abc_ObjFaninC0(pObj) );

    // add trivial clauses
    pObj = Abc_NtkPo( pAig, 0 );
    Res_SatAddConst1( pSat, (int)pObj->pCopy, 0 ); // care-set

    pObj = Abc_NtkPo( pAig, 1 );
    Res_SatAddConst1( pSat, (int)pObj->pCopy, !fOnSet ); // on-set

    Vec_PtrFree( vFanins );
    return pSat;
}

/**Function*************************************************************

  Synopsis    [Loads AIG into the SAT solver for constrained simulation.]

  Description [Returns 1 if the required number of patterns are found. 
  Returns 0 if the solver ran out of time or proved a constant. 
  In the latter, case one of the flags, fConst0 or fConst1, are set to 1.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_SatSimulate( Res_Sim_t * p, int nPatsLimit, int fOnSet )
{
    Vec_Int_t * vLits;
    Vec_Ptr_t * vPats;
    sat_solver * pSat;
    int RetValue, i, k, value, status, Lit, Var, iPat;
    int clk = clock();

//printf( "Looking for %s:  ", fOnSet? "onset " : "offset" );

    // decide what problem should be solved
    Lit = toLitCond( (int)Abc_NtkPo(p->pAig,1)->pCopy, !fOnSet );
    if ( fOnSet )
    {
        iPat = p->nPats1;
        vPats = p->vPats1;
    }
    else
    {
        iPat = p->nPats0;
        vPats = p->vPats0;
    }
    assert( iPat < nPatsLimit );

    // derive the SAT solver
    pSat = Res_SatSimulateConstr( p->pAig, fOnSet );
    pSat->fSkipSimplify = 1;
    status = sat_solver_simplify( pSat );
    if ( status == 0 )
    {
        if ( iPat == 0 )
        {
//            if ( fOnSet )
//                p->fConst0 = 1;
//            else
//                p->fConst1 = 1;
            RetValue = 0;
        }
        goto finish;
    }
 
    // enumerate through the SAT assignments
    RetValue = 1;
    vLits = Vec_IntAlloc( 32 );
    for ( k = iPat; k < nPatsLimit; k++ )
    {
        // solve with the assumption
//        status = sat_solver_solve( pSat, &Lit, &Lit + 1, (sint64)10000, (sint64)0, (sint64)0, (sint64)0 );
        status = sat_solver_solve( pSat, NULL, NULL, (sint64)10000, (sint64)0, (sint64)0, (sint64)0 );
        if ( status == l_False )
        {
//printf( "Const %d\n", !fOnSet );
            if ( k == 0 )
            {
                if ( fOnSet )
                    p->fConst0 = 1;
                else
                    p->fConst1 = 1;
                RetValue = 0;
            }
            break;
        }
        else if ( status == l_True )
        {
            // save the pattern
            Vec_IntClear( vLits );
            for ( i = 0; i < p->nTruePis; i++ )
            {
                Var = (int)Abc_NtkPi(p->pAig,i)->pCopy;
                value = (int)(pSat->model.ptr[Var] == l_True);
                if ( value )
                    Abc_InfoSetBit( Vec_PtrEntry(vPats, i), k );
                Lit = toLitCond( Var, value );
                Vec_IntPush( vLits, Lit );
//                printf( "%d", value );
            }
//            printf( "\n" );

            status = sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits) );
            if ( status == 0 )
            {
                k++;
                RetValue = 1; 
                break;
            }
        }
        else
        {
//printf( "Undecided\n" );
            if ( k == 0 )
                RetValue = 0;
            else
                RetValue = 1; 
            break;
        }
    }
    Vec_IntFree( vLits );
//printf( "Found %d patterns\n", k - iPat );

    // set the new number of patterns
    if ( fOnSet )
        p->nPats1 = k;
    else
        p->nPats0 = k;

finish:

    sat_solver_delete( pSat );
p->timeSat += clock() - clk;
    return RetValue;
}


/**Function*************************************************************

  Synopsis    [Asserts equality of the variable to a constant.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_SatAddConst1( sat_solver * pSat, int iVar, int fCompl )
{
    lit Lit = toLitCond( iVar, fCompl );
    if ( !sat_solver_addclause( pSat, &Lit, &Lit + 1 ) )
        return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Asserts equality of two variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_SatAddEqual( sat_solver * pSat, int iVar0, int iVar1, int fCompl )
{
    lit Lits[2];

    Lits[0] = toLitCond( iVar0, 0 );
    Lits[1] = toLitCond( iVar1, !fCompl );
    if ( !sat_solver_addclause( pSat, Lits, Lits + 2 ) )
        return 0;

    Lits[0] = toLitCond( iVar0, 1 );
    Lits[1] = toLitCond( iVar1, fCompl );
    if ( !sat_solver_addclause( pSat, Lits, Lits + 2 ) )
        return 0;

    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds constraints for the two-input AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Res_SatAddAnd( sat_solver * pSat, int iVar, int iVar0, int iVar1, int fCompl0, int fCompl1 )
{
    lit Lits[3];

    Lits[0] = toLitCond( iVar, 1 );
    Lits[1] = toLitCond( iVar0, fCompl0 );
    if ( !sat_solver_addclause( pSat, Lits, Lits + 2 ) )
        return 0;

    Lits[0] = toLitCond( iVar, 1 );
    Lits[1] = toLitCond( iVar1, fCompl1 );
    if ( !sat_solver_addclause( pSat, Lits, Lits + 2 ) )
        return 0;

    Lits[0] = toLitCond( iVar, 0 );
    Lits[1] = toLitCond( iVar0, !fCompl0 );
    Lits[2] = toLitCond( iVar1, !fCompl1 );
    if ( !sat_solver_addclause( pSat, Lits, Lits + 3 ) )
        return 0;

    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


