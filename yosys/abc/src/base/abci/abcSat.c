/**CFile****************************************************************

  FileName    [abcSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Procedures to solve the miter using the internal SAT sat_solver.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcSat.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/cmd/cmd.h"
#include "sat/bsat/satSolver.h"
#include "aig/gia/gia.h"
#include "aig/gia/giaAig.h"

#ifdef ABC_USE_CUDD
#include "bdd/extrab/extraBdd.h"
#endif

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static sat_solver * Abc_NtkMiterSatCreateLogic( Abc_Ntk_t * pNtk, int fAllPrimes );
extern Vec_Int_t * Abc_NtkGetCiSatVarNums( Abc_Ntk_t * pNtk );
static int nMuxes;

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Attempts to solve the miter using an internal SAT sat_solver.]

  Description [Returns -1 if timed out; 0 if SAT; 1 if UNSAT.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMiterSat( Abc_Ntk_t * pNtk, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, int fVerbose, ABC_INT64_T * pNumConfs, ABC_INT64_T * pNumInspects )
{
    sat_solver * pSat;
    lbool   status;
    int RetValue = 0;
    abctime clk;
 
    if ( pNumConfs )
        *pNumConfs = 0;
    if ( pNumInspects )
        *pNumInspects = 0;

    assert( Abc_NtkLatchNum(pNtk) == 0 );

//    if ( Abc_NtkPoNum(pNtk) > 1 )
//        fprintf( stdout, "Warning: The miter has %d outputs. SAT will try to prove all of them.\n", Abc_NtkPoNum(pNtk) );

    // load clauses into the sat_solver
    clk = Abc_Clock();
    pSat = (sat_solver *)Abc_NtkMiterSatCreate( pNtk, 0 );
    if ( pSat == NULL )
        return 1;
//printf( "%d \n", pSat->clauses.size );
//sat_solver_delete( pSat );
//return 1;

//    printf( "Created SAT problem with %d variable and %d clauses. ", sat_solver_nvars(pSat), sat_solver_nclauses(pSat) );
//    ABC_PRT( "Time", Abc_Clock() - clk );

    // simplify the problem
    clk = Abc_Clock();
    status = sat_solver_simplify(pSat);
//    printf( "Simplified the problem to %d variables and %d clauses. ", sat_solver_nvars(pSat), sat_solver_nclauses(pSat) );
//    ABC_PRT( "Time", Abc_Clock() - clk );
    if ( status == 0 )
    {
        sat_solver_delete( pSat );
//        printf( "The problem is UNSATISFIABLE after simplification.\n" );
        return 1;
    }

    // solve the miter
    clk = Abc_Clock();
    if ( fVerbose )
        pSat->verbosity = 1;
    status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)nInsLimit, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    if ( status == l_Undef )
    {
//        printf( "The problem timed out.\n" );
        RetValue = -1;
    }
    else if ( status == l_True )
    {
//        printf( "The problem is SATISFIABLE.\n" );
        RetValue = 0;
    }
    else if ( status == l_False )
    {
//        printf( "The problem is UNSATISFIABLE.\n" );
        RetValue = 1;
    }
    else
        assert( 0 );
//    ABC_PRT( "SAT sat_solver time", Abc_Clock() - clk );
//    printf( "The number of conflicts = %d.\n", (int)pSat->sat_solver_stats.conflicts );

    // if the problem is SAT, get the counterexample
    if ( status == l_True )
    {
//        Vec_Int_t * vCiIds = Abc_NtkGetCiIds( pNtk );
        Vec_Int_t * vCiIds = Abc_NtkGetCiSatVarNums( pNtk );
        pNtk->pModel = Sat_SolverGetModel( pSat, vCiIds->pArray, vCiIds->nSize );
        Vec_IntFree( vCiIds );
    }
    // free the sat_solver
    if ( fVerbose )
        Sat_SolverPrintStats( stdout, pSat );

    if ( pNumConfs )
        *pNumConfs = (int)pSat->stats.conflicts;
    if ( pNumInspects )
        *pNumInspects = (int)pSat->stats.inspects;

sat_solver_store_write( pSat, "trace.cnf" );
sat_solver_store_free( pSat );

    sat_solver_delete( pSat );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int * Abc_NtkSolveGiaMiter( Gia_Man_t * p )
{
    extern Abc_Ntk_t * Abc_NtkFromAigPhase( Aig_Man_t * pMan );
    int RetValue = 0;
    int * pResult = NULL;
    Abc_Ntk_t * pNtk;
    Aig_Man_t * pMan;
    pMan = Gia_ManToAig( p, 0 );
    pNtk = Abc_NtkFromAigPhase( pMan );
    pNtk->pName = Extra_UtilStrsav(p->pName);
    Aig_ManStop( pMan );
    RetValue = Abc_NtkMiterSat( pNtk, 1000000, 0, 0, NULL, NULL );
    if ( RetValue == 0 ) // sat
        pResult = pNtk->pModel, pNtk->pModel = NULL;
    Abc_NtkDelete( pNtk );
    return pResult;
}


/**Function*************************************************************

  Synopsis    [Returns the array of CI IDs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkGetCiSatVarNums( Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vCiIds;
    Abc_Obj_t * pObj;
    int i;
    vCiIds = Vec_IntAlloc( Abc_NtkCiNum(pNtk) );
    Abc_NtkForEachCi( pNtk, pObj, i )
        Vec_IntPush( vCiIds, (int)(ABC_PTRINT_T)pObj->pCopy );
    return vCiIds;
}


 
/**Function*************************************************************

  Synopsis    [Adds trivial clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkClauseTriv( sat_solver * pSat, Abc_Obj_t * pNode, Vec_Int_t * vVars )
{
//printf( "Adding triv %d.         %d\n", Abc_ObjRegular(pNode)->Id, (int)pSat->sat_solver_stats.clauses );
    vVars->nSize = 0;
    Vec_IntPush( vVars, toLitCond( (int)(ABC_PTRINT_T)Abc_ObjRegular(pNode)->pCopy, Abc_ObjIsComplement(pNode) ) );
//    Vec_IntPush( vVars, toLitCond( (int)Abc_ObjRegular(pNode)->Id, Abc_ObjIsComplement(pNode) ) );
    return sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize );
}
 
/**Function*************************************************************

  Synopsis    [Adds trivial clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkClauseTop( sat_solver * pSat, Vec_Ptr_t * vNodes, Vec_Int_t * vVars )
{
    Abc_Obj_t * pNode;
    int i;
//printf( "Adding triv %d.         %d\n", Abc_ObjRegular(pNode)->Id, (int)pSat->sat_solver_stats.clauses );
    vVars->nSize = 0;
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
        Vec_IntPush( vVars, toLitCond( (int)(ABC_PTRINT_T)Abc_ObjRegular(pNode)->pCopy, Abc_ObjIsComplement(pNode) ) );
//    Vec_IntPush( vVars, toLitCond( (int)Abc_ObjRegular(pNode)->Id, Abc_ObjIsComplement(pNode) ) );
    return sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize );
}
 
/**Function*************************************************************

  Synopsis    [Adds trivial clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkClauseAnd( sat_solver * pSat, Abc_Obj_t * pNode, Vec_Ptr_t * vSuper, Vec_Int_t * vVars )
{
    int fComp1, Var, Var1, i;
//printf( "Adding AND %d.  (%d)    %d\n", pNode->Id, vSuper->nSize+1, (int)pSat->sat_solver_stats.clauses );

    assert( !Abc_ObjIsComplement( pNode ) );
    assert( Abc_ObjIsNode( pNode ) );

//    nVars = sat_solver_nvars(pSat);
    Var = (int)(ABC_PTRINT_T)pNode->pCopy;
//    Var = pNode->Id;

//    assert( Var  < nVars ); 
    for ( i = 0; i < vSuper->nSize; i++ )
    {
        // get the predecessor nodes
        // get the complemented attributes of the nodes
        fComp1 = Abc_ObjIsComplement((Abc_Obj_t *)vSuper->pArray[i]);
        // determine the variable numbers
        Var1 = (int)(ABC_PTRINT_T)Abc_ObjRegular((Abc_Obj_t *)vSuper->pArray[i])->pCopy;
//        Var1 = (int)Abc_ObjRegular(vSuper->pArray[i])->Id;

        // check that the variables are in the SAT manager
//        assert( Var1 < nVars );

        // suppose the AND-gate is A * B = C
        // add !A => !C   or   A + !C
    //  fprintf( pFile, "%d %d 0%c", Var1, -Var, 10 );
        vVars->nSize = 0;
        Vec_IntPush( vVars, toLitCond(Var1, fComp1) );
        Vec_IntPush( vVars, toLitCond(Var,  1     ) );
        if ( !sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize ) )
            return 0;
    }

    // add A & B => C   or   !A + !B + C
//  fprintf( pFile, "%d %d %d 0%c", -Var1, -Var2, Var, 10 );
    vVars->nSize = 0;
    for ( i = 0; i < vSuper->nSize; i++ )
    {
        // get the predecessor nodes
        // get the complemented attributes of the nodes
        fComp1 = Abc_ObjIsComplement((Abc_Obj_t *)vSuper->pArray[i]);
        // determine the variable numbers
        Var1 = (int)(ABC_PTRINT_T)Abc_ObjRegular((Abc_Obj_t *)vSuper->pArray[i])->pCopy;
//        Var1 = (int)Abc_ObjRegular(vSuper->pArray[i])->Id;
        // add this variable to the array
        Vec_IntPush( vVars, toLitCond(Var1, !fComp1) );
    }
    Vec_IntPush( vVars, toLitCond(Var, 0) );
    return sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize );
}
 
/**Function*************************************************************

  Synopsis    [Adds trivial clause.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkClauseMux( sat_solver * pSat, Abc_Obj_t * pNode, Abc_Obj_t * pNodeC, Abc_Obj_t * pNodeT, Abc_Obj_t * pNodeE, Vec_Int_t * vVars )
{
    int VarF, VarI, VarT, VarE, fCompT, fCompE;
//printf( "Adding mux %d.         %d\n", pNode->Id, (int)pSat->sat_solver_stats.clauses );

    assert( !Abc_ObjIsComplement( pNode ) );
    assert( Abc_NodeIsMuxType( pNode ) );
    // get the variable numbers
    VarF = (int)(ABC_PTRINT_T)pNode->pCopy;
    VarI = (int)(ABC_PTRINT_T)pNodeC->pCopy;
    VarT = (int)(ABC_PTRINT_T)Abc_ObjRegular(pNodeT)->pCopy;
    VarE = (int)(ABC_PTRINT_T)Abc_ObjRegular(pNodeE)->pCopy;
//    VarF = (int)pNode->Id;
//    VarI = (int)pNodeC->Id;
//    VarT = (int)Abc_ObjRegular(pNodeT)->Id;
//    VarE = (int)Abc_ObjRegular(pNodeE)->Id;

    // get the complementation flags
    fCompT = Abc_ObjIsComplement(pNodeT);
    fCompE = Abc_ObjIsComplement(pNodeE);

    // f = ITE(i, t, e)
    // i' + t' + f
    // i' + t  + f'
    // i  + e' + f
    // i  + e  + f'
    // create four clauses
    vVars->nSize = 0;
    Vec_IntPush( vVars, toLitCond(VarI,  1) );
    Vec_IntPush( vVars, toLitCond(VarT,  1^fCompT) );
    Vec_IntPush( vVars, toLitCond(VarF,  0) );
    if ( !sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize ) )
        return 0;
    vVars->nSize = 0;
    Vec_IntPush( vVars, toLitCond(VarI,  1) );
    Vec_IntPush( vVars, toLitCond(VarT,  0^fCompT) );
    Vec_IntPush( vVars, toLitCond(VarF,  1) );
    if ( !sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize ) )
        return 0;
    vVars->nSize = 0;
    Vec_IntPush( vVars, toLitCond(VarI,  0) );
    Vec_IntPush( vVars, toLitCond(VarE,  1^fCompE) );
    Vec_IntPush( vVars, toLitCond(VarF,  0) );
    if ( !sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize ) )
        return 0;
    vVars->nSize = 0;
    Vec_IntPush( vVars, toLitCond(VarI,  0) );
    Vec_IntPush( vVars, toLitCond(VarE,  0^fCompE) );
    Vec_IntPush( vVars, toLitCond(VarF,  1) );
    if ( !sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize ) )
        return 0;
 
    if ( VarT == VarE )
    {
//        assert( fCompT == !fCompE );
        return 1;
    }

    // two additional clauses
    // t' & e' -> f'       t  + e   + f'
    // t  & e  -> f        t' + e'  + f 
    vVars->nSize = 0;
    Vec_IntPush( vVars, toLitCond(VarT,  0^fCompT) );
    Vec_IntPush( vVars, toLitCond(VarE,  0^fCompE) );
    Vec_IntPush( vVars, toLitCond(VarF,  1) );
    if ( !sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize ) )
        return 0;
    vVars->nSize = 0;
    Vec_IntPush( vVars, toLitCond(VarT,  1^fCompT) );
    Vec_IntPush( vVars, toLitCond(VarE,  1^fCompE) );
    Vec_IntPush( vVars, toLitCond(VarF,  0) );
    return sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize );
}

/**Function*************************************************************

  Synopsis    [Returns the array of nodes to be combined into one multi-input AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCollectSupergate_rec( Abc_Obj_t * pNode, Vec_Ptr_t * vSuper, int fFirst, int fStopAtMux )
{
    int RetValue1, RetValue2, i;
    // check if the node is visited
    if ( Abc_ObjRegular(pNode)->fMarkB )
    {
        // check if the node occurs in the same polarity
        for ( i = 0; i < vSuper->nSize; i++ )
            if ( vSuper->pArray[i] == pNode )
                return 1;
        // check if the node is present in the opposite polarity
        for ( i = 0; i < vSuper->nSize; i++ )
            if ( vSuper->pArray[i] == Abc_ObjNot(pNode) )
                return -1;
        assert( 0 );
        return 0;
    }
    // if the new node is complemented or a PI, another gate begins
    if ( !fFirst )
        if ( Abc_ObjIsComplement(pNode) || !Abc_ObjIsNode(pNode) || Abc_ObjFanoutNum(pNode) > 1 || (fStopAtMux && Abc_NodeIsMuxType(pNode)) )
        {
            Vec_PtrPush( vSuper, pNode );
            Abc_ObjRegular(pNode)->fMarkB = 1;
            return 0;
        }
    assert( !Abc_ObjIsComplement(pNode) );
    assert( Abc_ObjIsNode(pNode) );
    // go through the branches
    RetValue1 = Abc_NtkCollectSupergate_rec( Abc_ObjChild0(pNode), vSuper, 0, fStopAtMux );
    RetValue2 = Abc_NtkCollectSupergate_rec( Abc_ObjChild1(pNode), vSuper, 0, fStopAtMux );
    if ( RetValue1 == -1 || RetValue2 == -1 )
        return -1;
    // return 1 if at least one branch has a duplicate
    return RetValue1 || RetValue2;
}

/**Function*************************************************************

  Synopsis    [Returns the array of nodes to be combined into one multi-input AND-gate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCollectSupergate( Abc_Obj_t * pNode, int fStopAtMux, Vec_Ptr_t * vNodes )
{
    int RetValue, i;
    assert( !Abc_ObjIsComplement(pNode) );
    // collect the nodes in the implication supergate
    Vec_PtrClear( vNodes );
    RetValue = Abc_NtkCollectSupergate_rec( pNode, vNodes, 1, fStopAtMux );
    assert( vNodes->nSize > 1 );
    // unmark the visited nodes
    for ( i = 0; i < vNodes->nSize; i++ )
        Abc_ObjRegular((Abc_Obj_t *)vNodes->pArray[i])->fMarkB = 0;
    // if we found the node and its complement in the same implication supergate, 
    // return empty set of nodes (meaning that we should use constant-0 node)
    if ( RetValue == -1 )
        vNodes->nSize = 0;
}


/**Function*************************************************************

  Synopsis    [Computes the factor of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkNodeFactor( Abc_Obj_t * pObj, int nLevelMax )
{
//  nLevelMax = ((nLevelMax)/2)*3;
    assert( (int)pObj->Level <= nLevelMax );
//    return (int)(100000000.0 * pow(0.999, nLevelMax - pObj->Level));
    return (int)(100000000.0 * (1 + 0.01 * pObj->Level));
//    return (int)(100000000.0 / ((nLevelMax)/2)*3 - pObj->Level);
}

/**Function*************************************************************

  Synopsis    [Sets up the SAT sat_solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMiterSatCreateInt( sat_solver * pSat, Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pNode, * pFanin, * pNodeC, * pNodeT, * pNodeE;
    Vec_Ptr_t * vNodes, * vSuper;
    Vec_Int_t * vVars;
    int i, k, fUseMuxes = 1;
//    int fOrderCiVarsFirst = 0;
    int RetValue = 0;

    assert( Abc_NtkIsStrash(pNtk) );

    // clean the CI node pointers
    Abc_NtkForEachCi( pNtk, pNode, i )
        pNode->pCopy = NULL;

    // start the data structures
    vNodes  = Vec_PtrAlloc( 1000 );   // the nodes corresponding to vars in the sat_solver
    vSuper  = Vec_PtrAlloc( 100 );    // the nodes belonging to the given implication supergate
    vVars   = Vec_IntAlloc( 100 );    // the temporary array for variables in the clause

    // add the clause for the constant node
    pNode = Abc_AigConst1(pNtk);
    pNode->fMarkA = 1;
    pNode->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)vNodes->nSize;
    Vec_PtrPush( vNodes, pNode );
    Abc_NtkClauseTriv( pSat, pNode, vVars );
/*
    // add the PI variables first
    Abc_NtkForEachCi( pNtk, pNode, i )
    {
        pNode->fMarkA = 1;
        pNode->pCopy = (Abc_Obj_t *)vNodes->nSize;
        Vec_PtrPush( vNodes, pNode );
    }
*/
    // collect the nodes that need clauses and top-level assignments
    Vec_PtrClear( vSuper );
    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        // get the fanin
        pFanin = Abc_ObjFanin0(pNode);
        // create the node's variable
        if ( pFanin->fMarkA == 0 )
        {
            pFanin->fMarkA = 1;
            pFanin->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)vNodes->nSize;
            Vec_PtrPush( vNodes, pFanin );
        }
        // add the trivial clause
        Vec_PtrPush( vSuper, Abc_ObjChild0(pNode) );
    }
    if ( !Abc_NtkClauseTop( pSat, vSuper, vVars ) )
        goto Quits;


    // add the clauses
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pNode, i )
    {
        assert( !Abc_ObjIsComplement(pNode) );
        if ( !Abc_AigNodeIsAnd(pNode) )
            continue;
//printf( "%d ", pNode->Id );

        // add the clauses
        if ( fUseMuxes && Abc_NodeIsMuxType(pNode) )
        {
            nMuxes++;

            pNodeC = Abc_NodeRecognizeMux( pNode, &pNodeT, &pNodeE );
            Vec_PtrClear( vSuper );
            Vec_PtrPush( vSuper, pNodeC );
            Vec_PtrPush( vSuper, pNodeT );
            Vec_PtrPush( vSuper, pNodeE );
            // add the fanin nodes to explore
            Vec_PtrForEachEntry( Abc_Obj_t *, vSuper, pFanin, k )
            {
                pFanin = Abc_ObjRegular(pFanin);
                if ( pFanin->fMarkA == 0 )
                {
                    pFanin->fMarkA = 1;
                    pFanin->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)vNodes->nSize;
                    Vec_PtrPush( vNodes, pFanin );
                }
            }
            // add the clauses
            if ( !Abc_NtkClauseMux( pSat, pNode, pNodeC, pNodeT, pNodeE, vVars ) )
                goto Quits;
        }
        else
        {
            // get the supergate
            Abc_NtkCollectSupergate( pNode, fUseMuxes, vSuper );
            // add the fanin nodes to explore
            Vec_PtrForEachEntry( Abc_Obj_t *, vSuper, pFanin, k )
            {
                pFanin = Abc_ObjRegular(pFanin);
                if ( pFanin->fMarkA == 0 )
                {
                    pFanin->fMarkA = 1;
                    pFanin->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)vNodes->nSize;
                    Vec_PtrPush( vNodes, pFanin );
                }
            }
            // add the clauses
            if ( vSuper->nSize == 0 )
            {
                if ( !Abc_NtkClauseTriv( pSat, Abc_ObjNot(pNode), vVars ) )
//                if ( !Abc_NtkClauseTriv( pSat, pNode, vVars ) )
                    goto Quits;
            }
            else
            {
                if ( !Abc_NtkClauseAnd( pSat, pNode, vSuper, vVars ) )
                    goto Quits;
            }
        }
    }
/*
    // set preferred variables
    if ( fOrderCiVarsFirst )
    {
        int * pPrefVars = ABC_ALLOC( int, Abc_NtkCiNum(pNtk) );
        int nVars = 0;
        Abc_NtkForEachCi( pNtk, pNode, i )
        {
            if ( pNode->fMarkA == 0 )
                continue;
            pPrefVars[nVars++] = (int)pNode->pCopy;
        }
        nVars = Abc_MinInt( nVars, 10 );
        ASat_SolverSetPrefVars( pSat, pPrefVars, nVars );
    }
*/
/*
    Abc_NtkForEachObj( pNtk, pNode, i )
    {
        if ( !pNode->fMarkA )
            continue;
        printf( "%10s : ", Abc_ObjName(pNode) );
        printf( "%3d\n", (int)pNode->pCopy );
    }
    printf( "\n" );
*/
    RetValue = 1;
Quits :
    // delete
    Vec_IntFree( vVars );
    Vec_PtrFree( vNodes );
    Vec_PtrFree( vSuper );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Sets up the SAT sat_solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abc_NtkMiterSatCreate( Abc_Ntk_t * pNtk, int fAllPrimes )
{
    sat_solver * pSat;
    Abc_Obj_t * pNode;
    int RetValue, i; //, clk = Abc_Clock();

    assert( Abc_NtkIsStrash(pNtk) || Abc_NtkIsBddLogic(pNtk) );
    if ( Abc_NtkIsBddLogic(pNtk) )
        return Abc_NtkMiterSatCreateLogic(pNtk, fAllPrimes);

    nMuxes = 0;
    pSat = sat_solver_new();
//sat_solver_store_alloc( pSat );
    RetValue = Abc_NtkMiterSatCreateInt( pSat, pNtk );
sat_solver_store_mark_roots( pSat );

    Abc_NtkForEachObj( pNtk, pNode, i )
        pNode->fMarkA = 0;
//    ASat_SolverWriteDimacs( pSat, "temp_sat.cnf", NULL, NULL, 1 );
    if ( RetValue == 0 )
    {
        sat_solver_delete(pSat);
        return NULL;
    }
//    printf( "Ands = %6d.  Muxes = %6d (%5.2f %%).  ", Abc_NtkNodeNum(pNtk), nMuxes, 300.0*nMuxes/Abc_NtkNodeNum(pNtk) );
//    ABC_PRT( "Creating sat_solver", Abc_Clock() - clk );
    return pSat;
}



#ifdef ABC_USE_CUDD

/**Function*************************************************************

  Synopsis    [Adds clauses for the internal node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeAddClauses( sat_solver * pSat, char * pSop0, char * pSop1, Abc_Obj_t * pNode, Vec_Int_t * vVars )
{
    Abc_Obj_t * pFanin;
    int i, c, nFanins;
    int RetValue = 0;
    char * pCube;

    nFanins = Abc_ObjFaninNum( pNode );
    assert( nFanins == Abc_SopGetVarNum( pSop0 ) );

//    if ( nFanins == 0 )
    if ( Cudd_Regular((Abc_Obj_t *)pNode->pData) == Cudd_ReadOne((DdManager *)pNode->pNtk->pManFunc) )
    {
        vVars->nSize = 0;
//        if ( Abc_SopIsConst1(pSop1) )
        if ( !Cudd_IsComplement(pNode->pData) )
            Vec_IntPush( vVars, toLit(pNode->Id) );
        else
            Vec_IntPush( vVars, lit_neg(toLit(pNode->Id)) );
        RetValue = sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize );
        if ( !RetValue ) 
        {
            printf( "The CNF is trivially UNSAT.\n" );
            return 0;
        }
        return 1;
    }
 
    // add clauses for the negative phase
    for ( c = 0; ; c++ )
    {
        // get the cube
        pCube = pSop0 + c * (nFanins + 3);
        if ( *pCube == 0 )
            break;
        // add the clause
        vVars->nSize = 0;
        Abc_ObjForEachFanin( pNode, pFanin, i )
        {
            if ( pCube[i] == '0' )
                Vec_IntPush( vVars, toLit(pFanin->Id) );
            else if ( pCube[i] == '1' )
                Vec_IntPush( vVars, lit_neg(toLit(pFanin->Id)) );
        }
        Vec_IntPush( vVars, lit_neg(toLit(pNode->Id)) );
        RetValue = sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize );
        if ( !RetValue ) 
        {
            printf( "The CNF is trivially UNSAT.\n" );
            return 0;
        }
    }

    // add clauses for the positive phase
    for ( c = 0; ; c++ )
    {
        // get the cube
        pCube = pSop1 + c * (nFanins + 3);
        if ( *pCube == 0 )
            break;
        // add the clause
        vVars->nSize = 0;
        Abc_ObjForEachFanin( pNode, pFanin, i )
        {
            if ( pCube[i] == '0' )
                Vec_IntPush( vVars, toLit(pFanin->Id) );
            else if ( pCube[i] == '1' )
                Vec_IntPush( vVars, lit_neg(toLit(pFanin->Id)) );
        }
        Vec_IntPush( vVars, toLit(pNode->Id) );
        RetValue = sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize );
        if ( !RetValue ) 
        {
            printf( "The CNF is trivially UNSAT.\n" );
            return 0;
        }
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds clauses for the PO node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NodeAddClausesTop( sat_solver * pSat, Abc_Obj_t * pNode, Vec_Int_t * vVars )
{
    Abc_Obj_t * pFanin;
    int RetValue = 0;

    pFanin = Abc_ObjFanin0(pNode);
    if ( Abc_ObjFaninC0(pNode) )
    {
        vVars->nSize = 0;
        Vec_IntPush( vVars, toLit(pFanin->Id) );
        Vec_IntPush( vVars, toLit(pNode->Id) );
        RetValue = sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize );
        if ( !RetValue ) 
        {
            printf( "The CNF is trivially UNSAT.\n" );
            return 0;
        }

        vVars->nSize = 0;
        Vec_IntPush( vVars, lit_neg(toLit(pFanin->Id)) );
        Vec_IntPush( vVars, lit_neg(toLit(pNode->Id)) );
        RetValue = sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize );
        if ( !RetValue ) 
        {
            printf( "The CNF is trivially UNSAT.\n" );
            return 0;
        }
    }
    else
    {
        vVars->nSize = 0;
        Vec_IntPush( vVars, lit_neg(toLit(pFanin->Id)) );
        Vec_IntPush( vVars, toLit(pNode->Id) );
        RetValue = sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize );
        if ( !RetValue ) 
        {
            printf( "The CNF is trivially UNSAT.\n" );
            return 0;
        }

        vVars->nSize = 0;
        Vec_IntPush( vVars, toLit(pFanin->Id) );
        Vec_IntPush( vVars, lit_neg(toLit(pNode->Id)) );
        RetValue = sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize );
        if ( !RetValue ) 
        {
            printf( "The CNF is trivially UNSAT.\n" );
            return 0;
        }
    }

    vVars->nSize = 0;
    Vec_IntPush( vVars, toLit(pNode->Id) );
    RetValue = sat_solver_addclause( pSat, vVars->pArray, vVars->pArray + vVars->nSize );
    if ( !RetValue ) 
    {
        printf( "The CNF is trivially UNSAT.\n" );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Sets up the SAT sat_solver.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
sat_solver * Abc_NtkMiterSatCreateLogic( Abc_Ntk_t * pNtk, int fAllPrimes )
{
    sat_solver * pSat;
    Mem_Flex_t * pMmFlex;
    Abc_Obj_t * pNode;
    Vec_Str_t * vCube;
    Vec_Int_t * vVars;
    char * pSop0, * pSop1;
    int i;

    assert( Abc_NtkIsBddLogic(pNtk) );

    // transfer the IDs to the copy field
    Abc_NtkForEachPi( pNtk, pNode, i )
        pNode->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)pNode->Id;

    // start the data structures
    pSat    = sat_solver_new();
sat_solver_store_alloc( pSat );
    pMmFlex = Mem_FlexStart();
    vCube   = Vec_StrAlloc( 100 );
    vVars   = Vec_IntAlloc( 100 );

    // add clauses for each internal nodes
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        // derive SOPs for both phases of the node
        Abc_NodeBddToCnf( pNode, pMmFlex, vCube, fAllPrimes, &pSop0, &pSop1 );
        // add the clauses to the sat_solver
        if ( !Abc_NodeAddClauses( pSat, pSop0, pSop1, pNode, vVars ) )
        {
            sat_solver_delete( pSat );
            pSat = NULL;
            goto finish;
        }
    }
    // add clauses for each PO
    Abc_NtkForEachPo( pNtk, pNode, i )
    {
        if ( !Abc_NodeAddClausesTop( pSat, pNode, vVars ) )
        {
            sat_solver_delete( pSat );
            pSat = NULL;
            goto finish;
        }
    }
sat_solver_store_mark_roots( pSat );

finish:
    // delete
    Vec_StrFree( vCube );
    Vec_IntFree( vVars );
    Mem_FlexStop( pMmFlex, 0 );
    return pSat;
}

#else

sat_solver * Abc_NtkMiterSatCreateLogic( Abc_Ntk_t * pNtk, int fAllPrimes ) { return NULL; }

#endif

/**Function*************************************************************

  Synopsis    [Writes CNF for the sorter with N inputs asserting Q ones.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkWriteSorterCnf( char * pFileName, int nVars, int nQueens )
{
    char Command[100];
    void * pAbc;
    Abc_Ntk_t * pNtk;
    Abc_Obj_t * pObj, * ppNodes[2], * ppRoots[2];
    Vec_Ptr_t * vNodes;
    FILE * pFile;
    int i, Counter;

    if ( nQueens <= 0 && nQueens >= nVars )
    {
        printf( "The number of queens (Q = %d) should belong to the interval: 0 < Q < %d.\n", nQueens, nQueens);
        return;
    }
    assert( nQueens > 0 && nQueens < nVars );
    pAbc = Abc_FrameGetGlobalFrame();
    // generate sorter
    sprintf( Command, "gen -s -N %d sorter%d.blif", nVars, nVars );
    if ( Cmd_CommandExecute( (Abc_Frame_t *)pAbc, Command ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
        return;
    }
    // read the file
    sprintf( Command, "read sorter%d.blif; strash", nVars );
    if ( Cmd_CommandExecute( (Abc_Frame_t *)pAbc, Command ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
        return;
    }

    // get the current network
    pNtk = Abc_FrameReadNtk((Abc_Frame_t *)pAbc);
    // collect the nodes for the given two primary outputs
    ppNodes[0] = Abc_NtkPo( pNtk, nVars - nQueens - 1 );
    ppNodes[1] = Abc_NtkPo( pNtk, nVars - nQueens );
    ppRoots[0] = Abc_ObjFanin0( ppNodes[0] );
    ppRoots[1] = Abc_ObjFanin0( ppNodes[1] );
    vNodes = Abc_NtkDfsNodes( pNtk, ppRoots, 2 );

    // assign CNF variables
    Counter = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)~0;
    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)Counter++;
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRINT_T)Counter++;

/*
        OutVar   = pCnf->pVarNums[ pObj->Id ];
        pVars[0] = pCnf->pVarNums[ Aig_ObjFanin0(pObj)->Id ];
        pVars[1] = pCnf->pVarNums[ Aig_ObjFanin1(pObj)->Id ];

        // positive phase
        *pClas++ = pLits;
        *pLits++ = 2 * OutVar; 
        *pLits++ = 2 * pVars[0] + !Aig_ObjFaninC0(pObj); 
        *pLits++ = 2 * pVars[1] + !Aig_ObjFaninC1(pObj); 
        // negative phase
        *pClas++ = pLits;
        *pLits++ = 2 * OutVar + 1; 
        *pLits++ = 2 * pVars[0] + Aig_ObjFaninC0(pObj); 
        *pClas++ = pLits;
        *pLits++ = 2 * OutVar + 1; 
        *pLits++ = 2 * pVars[1] + Aig_ObjFaninC1(pObj); 
*/

    // add clauses for these nodes
    pFile = fopen( pFileName, "w" );
    fprintf( pFile, "c CNF for %d-bit sorter with %d bits set to 1 generated by ABC.\n", nVars, nQueens );
    fprintf( pFile, "p cnf %d %d\n", Counter, 3 * Vec_PtrSize(vNodes) + 2 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        // positive phase
        fprintf( pFile, "%d %s%d %s%d 0\n", 1+(int)(ABC_PTRINT_T)pObj->pCopy,
            Abc_ObjFaninC0(pObj)? "" : "-", 1+(int)(ABC_PTRINT_T)Abc_ObjFanin0(pObj)->pCopy,
            Abc_ObjFaninC1(pObj)? "" : "-", 1+(int)(ABC_PTRINT_T)Abc_ObjFanin1(pObj)->pCopy );
        // negative phase
        fprintf( pFile, "-%d %s%d 0\n",     1+(int)(ABC_PTRINT_T)pObj->pCopy,
            Abc_ObjFaninC0(pObj)? "-" : "", 1+(int)(ABC_PTRINT_T)Abc_ObjFanin0(pObj)->pCopy );
        fprintf( pFile, "-%d %s%d 0\n",     1+(int)(ABC_PTRINT_T)pObj->pCopy,
            Abc_ObjFaninC1(pObj)? "-" : "", 1+(int)(ABC_PTRINT_T)Abc_ObjFanin1(pObj)->pCopy );
    }
    Vec_PtrFree( vNodes );

/*
    *pClas++ = pLits;
    *pLits++ = 2 * OutVar + Aig_ObjFaninC0(pObj); 
*/
    // assert the first literal to zero
    fprintf( pFile, "%s%d 0\n", 
        Abc_ObjFaninC0(ppNodes[0])? "" : "-", 1+(int)(ABC_PTRINT_T)Abc_ObjFanin0(ppNodes[0])->pCopy );
    // assert the second literal to one
    fprintf( pFile, "%s%d 0\n", 
        Abc_ObjFaninC0(ppNodes[1])? "-" : "", 1+(int)(ABC_PTRINT_T)Abc_ObjFanin0(ppNodes[1])->pCopy );
    fclose( pFile );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

