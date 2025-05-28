/**CFile****************************************************************

  FileName    [fraCec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [New FRAIG package.]

  Synopsis    [CEC engined based on fraiging.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 30, 2007.]

  Revision    [$Id: fraCec.c,v 1.00 2007/06/30 00:00:00 alanmi Exp $]

***********************************************************************/

#include "fra.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver2.h"

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
int Fra_FraigSat( Aig_Man_t * pMan, ABC_INT64_T nConfLimit, ABC_INT64_T nInsLimit, int nLearnedStart, int nLearnedDelta, int nLearnedPerce, int fFlipBits, int fAndOuts, int fNewSolver, int fVerbose )
{
    if ( fNewSolver )
    {
        extern void * Cnf_DataWriteIntoSolver2( Cnf_Dat_t * p, int nFrames, int fInit );
        extern int    Cnf_DataWriteOrClause2( void * pSat, Cnf_Dat_t * pCnf );

        sat_solver2 * pSat;
        Cnf_Dat_t * pCnf;
        int status, RetValue = 0;
        abctime clk = Abc_Clock();
        Vec_Int_t * vCiIds;

        assert( Aig_ManRegNum(pMan) == 0 );
        pMan->pData = NULL;

        // derive CNF
        pCnf = Cnf_Derive( pMan, Aig_ManCoNum(pMan) );
    //    pCnf = Cnf_DeriveSimple( pMan, Aig_ManCoNum(pMan) );

        if ( fFlipBits ) 
            Cnf_DataTranformPolarity( pCnf, 0 );

        if ( fVerbose )
        {
            printf( "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d. ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        }

        // convert into SAT solver
        pSat = (sat_solver2 *)Cnf_DataWriteIntoSolver2( pCnf, 1, 0 );
        if ( pSat == NULL )
        {
            Cnf_DataFree( pCnf );
            return 1;
        }

        if ( fAndOuts )
        {
            // assert each output independently
            if ( !Cnf_DataWriteAndClauses( pSat, pCnf ) )
            {
                sat_solver2_delete( pSat );
                Cnf_DataFree( pCnf );
                return 1;
            }
        }
        else
        {
            // add the OR clause for the outputs
            if ( !Cnf_DataWriteOrClause2( pSat, pCnf ) )
            {
                sat_solver2_delete( pSat );
                Cnf_DataFree( pCnf );
                return 1;
            }
        }
        vCiIds = Cnf_DataCollectPiSatNums( pCnf, pMan );
        Cnf_DataFree( pCnf );


        printf( "Created SAT problem with %d variable and %d clauses. ", sat_solver2_nvars(pSat), sat_solver2_nclauses(pSat) );
        ABC_PRT( "Time", Abc_Clock() - clk );

        // simplify the problem
        clk = Abc_Clock();
        status = sat_solver2_simplify(pSat);
//        printf( "Simplified the problem to %d variables and %d clauses. ", sat_solver2_nvars(pSat), sat_solver2_nclauses(pSat) );
//        ABC_PRT( "Time", Abc_Clock() - clk );
        if ( status == 0 )
        {
            Vec_IntFree( vCiIds );
            sat_solver2_delete( pSat );
    //        printf( "The problem is UNSATISFIABLE after simplification.\n" );
            return 1;
        }

        // solve the miter
        clk = Abc_Clock();
        if ( fVerbose )
            pSat->verbosity = 1;
        status = sat_solver2_solve( pSat, NULL, NULL, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)nInsLimit, (ABC_INT64_T)0, (ABC_INT64_T)0 );
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

    //    Abc_Print( 1, "The number of conflicts = %6d.  ", (int)pSat->stats.conflicts );
    //    Abc_PrintTime( 1, "Solving time", Abc_Clock() - clk );
 
        // if the problem is SAT, get the counterexample
        if ( status == l_True )
        {
            pMan->pData = Sat_Solver2GetModel( pSat, vCiIds->pArray, vCiIds->nSize );
        }
        // free the sat_solver2
        if ( fVerbose )
            Sat_Solver2PrintStats( stdout, pSat );
    //sat_solver2_store_write( pSat, "trace.cnf" );
    //sat_solver2_store_free( pSat );
        sat_solver2_delete( pSat );
        Vec_IntFree( vCiIds );
        return RetValue;
    }
    else
    {
        sat_solver * pSat;
        Cnf_Dat_t * pCnf;
        int status, RetValue = 0;
        abctime clk = Abc_Clock();
        Vec_Int_t * vCiIds;

        assert( Aig_ManRegNum(pMan) == 0 );
        pMan->pData = NULL;

        // derive CNF
        pCnf = Cnf_Derive( pMan, Aig_ManCoNum(pMan) );
    //    pCnf = Cnf_DeriveSimple( pMan, Aig_ManCoNum(pMan) );

        if ( fFlipBits ) 
            Cnf_DataTranformPolarity( pCnf, 0 );

        if ( fVerbose )
        {
            printf( "CNF stats: Vars = %6d. Clauses = %7d. Literals = %8d. ", pCnf->nVars, pCnf->nClauses, pCnf->nLiterals );
            Abc_PrintTime( 1, "Time", Abc_Clock() - clk );
        }

        // convert into SAT solver
        pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
        if ( pSat == NULL )
        {
            Cnf_DataFree( pCnf );
            return 1;
        }

        if ( nLearnedStart )
            pSat->nLearntStart = pSat->nLearntMax = nLearnedStart;
        if ( nLearnedDelta )
            pSat->nLearntDelta = nLearnedDelta;
        if ( nLearnedPerce )
            pSat->nLearntRatio = nLearnedPerce;
        if ( fVerbose )
            pSat->fVerbose = fVerbose;

        if ( fAndOuts )
        {
            // assert each output independently
            if ( !Cnf_DataWriteAndClauses( pSat, pCnf ) )
            {
                sat_solver_delete( pSat );
                Cnf_DataFree( pCnf );
                return 1;
            }
        }
        else
        {
            // add the OR clause for the outputs
            if ( !Cnf_DataWriteOrClause( pSat, pCnf ) )
            {
                sat_solver_delete( pSat );
                Cnf_DataFree( pCnf );
                return 1;
            }
        }
        vCiIds = Cnf_DataCollectPiSatNums( pCnf, pMan );
        Cnf_DataFree( pCnf );


    //    printf( "Created SAT problem with %d variable and %d clauses. ", sat_solver_nvars(pSat), sat_solver_nclauses(pSat) );
    //    ABC_PRT( "Time", Abc_Clock() - clk );

        // simplify the problem
        clk = Abc_Clock();
        status = sat_solver_simplify(pSat);
    //    printf( "Simplified the problem to %d variables and %d clauses. ", sat_solver_nvars(pSat), sat_solver_nclauses(pSat) );
    //    ABC_PRT( "Time", Abc_Clock() - clk );
        if ( status == 0 )
        {
            Vec_IntFree( vCiIds );
            sat_solver_delete( pSat );
    //        printf( "The problem is UNSATISFIABLE after simplification.\n" );
            return 1;
        }

        // solve the miter
        clk = Abc_Clock();
//        if ( fVerbose )
//            pSat->verbosity = 1;
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

    //    Abc_Print( 1, "The number of conflicts = %6d.  ", (int)pSat->stats.conflicts );
    //    Abc_PrintTime( 1, "Solving time", Abc_Clock() - clk );
 
        // if the problem is SAT, get the counterexample
        if ( status == l_True )
        {
            pMan->pData = Sat_SolverGetModel( pSat, vCiIds->pArray, vCiIds->nSize );
        }
        // free the sat_solver
        if ( fVerbose )
            Sat_SolverPrintStats( stdout, pSat );
    //sat_solver_store_write( pSat, "trace.cnf" );
    //sat_solver_store_free( pSat );
        sat_solver_delete( pSat );
        Vec_IntFree( vCiIds );
        return RetValue;
    }
}

/**Function*************************************************************

  Synopsis    [Recognizes what nodes are inputs of the EXOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManCountXors( Aig_Man_t * p )
{
    Aig_Obj_t * pObj, * pFan0, * pFan1;
    int i, Counter = 0;
    Aig_ManForEachNode( p, pObj, i )
        if ( Aig_ObjIsMuxType(pObj) && Aig_ObjRecognizeExor(pObj, &pFan0, &pFan1) )
            Counter++;
    return Counter;

}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_FraigCec( Aig_Man_t ** ppAig, int nConfLimit, int fVerbose )
{
    int nBTLimitStart =        300;   // starting SAT run
    int nBTLimitFirst =          2;   // first fraiging iteration
    int nBTLimitLast  = nConfLimit;   // the last-gasp SAT run

    Fra_Par_t Params, * pParams = &Params;
    Aig_Man_t * pAig = *ppAig, * pTemp;
    int i, RetValue;
    abctime clk;

    // report the original miter
    if ( fVerbose )
    {
        printf( "Original miter:   Nodes = %6d.\n", Aig_ManNodeNum(pAig) );
    }
    RetValue = Fra_FraigMiterStatus( pAig );
//    assert( RetValue == -1 );
    if ( RetValue == 0 )
    {
        pAig->pData = ABC_ALLOC( int, Aig_ManCiNum(pAig) );
        memset( pAig->pData, 0, sizeof(int) * Aig_ManCiNum(pAig) );
        return RetValue;
    }

    // if SAT only, solve without iteration
clk = Abc_Clock();
    RetValue = Fra_FraigSat( pAig, (ABC_INT64_T)2*nBTLimitStart, (ABC_INT64_T)0, 0, 0, 0, 1, 0, 0, 0 );
    if ( fVerbose )
    {
        printf( "Initial SAT:      Nodes = %6d.  ", Aig_ManNodeNum(pAig) );
ABC_PRT( "Time", Abc_Clock() - clk );
    }
    if ( RetValue >= 0 )
        return RetValue;

    // duplicate the AIG
clk = Abc_Clock();
    pAig = Dar_ManRwsat( pTemp = pAig, 1, 0 );
    Aig_ManStop( pTemp );
    if ( fVerbose )
    {
        printf( "Rewriting:        Nodes = %6d.  ", Aig_ManNodeNum(pAig) );
ABC_PRT( "Time", Abc_Clock() - clk );
    }

    // perform the loop
    Fra_ParamsDefault( pParams );
    pParams->nBTLimitNode = nBTLimitFirst;
    pParams->nBTLimitMiter = nBTLimitStart;
    pParams->fDontShowBar = 1;
    pParams->fProve = 1;
    for ( i = 0; i < 6; i++ )
    {
//printf( "Running fraiging with %d BTnode and %d BTmiter.\n", pParams->nBTLimitNode, pParams->nBTLimitMiter );
        // try XOR balancing
        if ( Aig_ManCountXors(pAig) * 30 > Aig_ManNodeNum(pAig) + 300 )
        {
clk = Abc_Clock();
            pAig = Dar_ManBalanceXor( pTemp = pAig, 1, 0, 0 );
            Aig_ManStop( pTemp );
            if ( fVerbose )
            {
                printf( "Balance-X:        Nodes = %6d.  ", Aig_ManNodeNum(pAig) );
ABC_PRT( "Time", Abc_Clock() - clk );
            } 
        }

        // run fraiging
clk = Abc_Clock();
        pAig = Fra_FraigPerform( pTemp = pAig, pParams );
        Aig_ManStop( pTemp );
        if ( fVerbose )
        {
            printf( "Fraiging (i=%d):   Nodes = %6d.  ", i+1, Aig_ManNodeNum(pAig) );
ABC_PRT( "Time", Abc_Clock() - clk );
        }

        // check the miter status
        RetValue = Fra_FraigMiterStatus( pAig );
        if ( RetValue >= 0 )
            break;

        // perform rewriting
clk = Abc_Clock();
        pAig = Dar_ManRewriteDefault( pTemp = pAig );
        Aig_ManStop( pTemp );
        if ( fVerbose )
        {
            printf( "Rewriting:        Nodes = %6d.  ", Aig_ManNodeNum(pAig) );
ABC_PRT( "Time", Abc_Clock() - clk );
        } 

        // check the miter status
        RetValue = Fra_FraigMiterStatus( pAig );
        if ( RetValue >= 0 )
            break;
        // try simulation

        // set the parameters for the next run
        pParams->nBTLimitNode = 8 * pParams->nBTLimitNode;
        pParams->nBTLimitMiter = 2 * pParams->nBTLimitMiter;
    }

    // if still unsolved try last gasp
    if ( RetValue == -1 )
    {
clk = Abc_Clock();
        RetValue = Fra_FraigSat( pAig, (ABC_INT64_T)nBTLimitLast, (ABC_INT64_T)0, 0, 0, 0, 1, 0, 0, 0 );
        if ( fVerbose )
        {
            printf( "Final SAT:        Nodes = %6d.  ", Aig_ManNodeNum(pAig) );
ABC_PRT( "Time", Abc_Clock() - clk );
        }
    }

    *ppAig = pAig;
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_FraigCecPartitioned( Aig_Man_t * pMan1, Aig_Man_t * pMan2, int nConfLimit, int nPartSize, int fSmart, int fVerbose )
{
    Aig_Man_t * pAig;
    Vec_Ptr_t * vParts;
    int i, RetValue = 1, nOutputs;
    // create partitions
    vParts = Aig_ManMiterPartitioned( pMan1, pMan2, nPartSize, fSmart );
    // solve the partitions
    nOutputs = -1;
    Vec_PtrForEachEntry( Aig_Man_t *, vParts, pAig, i )
    {
        nOutputs++;
        if ( fVerbose )
        {
            printf( "Verifying part %4d  (out of %4d)  PI = %5d. PO = %5d. And = %6d. Lev = %4d.\r", 
                i+1, Vec_PtrSize(vParts), Aig_ManCiNum(pAig), Aig_ManCoNum(pAig), 
                Aig_ManNodeNum(pAig), Aig_ManLevelNum(pAig) );
            fflush( stdout );
        }
        RetValue = Fra_FraigMiterStatus( pAig );
        if ( RetValue == 1 )
            continue;
        if ( RetValue == 0 )
            break;
        RetValue = Fra_FraigCec( &pAig, nConfLimit, 0 );
        Vec_PtrWriteEntry( vParts, i, pAig );
        if ( RetValue == 1 )
            continue;
        if ( RetValue == 0 )
            break;
        break;
    }
    // clear the result
    if ( fVerbose )
    {
        printf( "                                                                                          \r" );
        fflush( stdout );
    }
    // report the timeout
    if ( RetValue == -1 )
    {
        printf( "Timed out after verifying %d partitions (out of %d).\n", nOutputs, Vec_PtrSize(vParts) );
        fflush( stdout );
    }
    // free intermediate results
    Vec_PtrForEachEntry( Aig_Man_t *, vParts, pAig, i )
        Aig_ManStop( pAig );
    Vec_PtrFree( vParts );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Fra_FraigCecTop( Aig_Man_t * pMan1, Aig_Man_t * pMan2, int nConfLimit, int nPartSize, int fSmart, int fVerbose )
{
    Aig_Man_t * pTemp;
    //Abc_NtkDarCec( pNtk1, pNtk2, fPartition, fVerbose );
    int RetValue;
    abctime clkTotal = Abc_Clock();

    if ( Aig_ManCiNum(pMan1) != Aig_ManCiNum(pMan1) )
    {
        printf( "Abc_CommandAbc8Cec(): Miters have different number of PIs.\n" );
        return 0;
    }
    if ( Aig_ManCoNum(pMan1) != Aig_ManCoNum(pMan1) )
    {
        printf( "Abc_CommandAbc8Cec(): Miters have different number of POs.\n" );
        return 0;
    }
    assert( Aig_ManCiNum(pMan1) == Aig_ManCiNum(pMan1) );
    assert( Aig_ManCoNum(pMan1) == Aig_ManCoNum(pMan1) );

    // make sure that the first miter has more nodes
    if ( Aig_ManNodeNum(pMan1) < Aig_ManNodeNum(pMan2) )
    {
        pTemp = pMan1;
        pMan1 = pMan2;
        pMan2 = pTemp;
    }
    assert( Aig_ManNodeNum(pMan1) >= Aig_ManNodeNum(pMan2) );

    if ( nPartSize )
        RetValue = Fra_FraigCecPartitioned( pMan1, pMan2, nConfLimit, nPartSize, fSmart, fVerbose );
    else // no partitioning
        RetValue = Fra_FraigCecPartitioned( pMan1, pMan2, nConfLimit, Aig_ManCoNum(pMan1), 0, fVerbose );

    // report the miter
    if ( RetValue == 1 )
    {
        printf( "Networks are equivalent.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    else if ( RetValue == 0 )
    {
        printf( "Networks are NOT EQUIVALENT.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    else
    {
        printf( "Networks are UNDECIDED.  " );
ABC_PRT( "Time", Abc_Clock() - clkTotal );
    }
    fflush( stdout );
    return RetValue;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

