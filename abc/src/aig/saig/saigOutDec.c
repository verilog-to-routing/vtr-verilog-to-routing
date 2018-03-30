/**CFile****************************************************************

  FileName    [saigOutDec.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Output cone decomposition.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigOutDec.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"
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

  Synopsis    [Performs decomposition of the property output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_ManFindPrimes( Aig_Man_t * pAig, int nLits, int fVerbose )
{
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    Aig_Obj_t * pObj0, * pObj1, * pRoot, * pMiter;
    Vec_Ptr_t * vPrimes, * vNodes;
    Vec_Int_t * vCube, * vMarks;
    int i0, i1, m, RetValue, Lits[10];
    int fCompl0, fCompl1, nNodes1, nNodes2;
    assert( nLits == 1 || nLits == 2 );
    assert( nLits < 10 );

    // create SAT solver
    pCnf = Cnf_DeriveSimple( pAig, Aig_ManCoNum(pAig) ); 
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );

    // collect nodes in the property output cone
    pMiter = Aig_ManCo( pAig, 0 );
    pRoot  = Aig_ObjFanin0( pMiter );
    vNodes = Aig_ManDfsNodes( pAig, &pRoot, 1 );
    // sort nodes by level and remove the last few
 
    // try single nodes
    vPrimes = Vec_PtrAlloc( 100 );
    // create assumptions
    vMarks = Vec_IntStart( Vec_PtrSize(vNodes) );
    Lits[0] = toLitCond( pCnf->pVarNums[Aig_ObjId(pMiter)], 1 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj0, i0 )
    if ( pObj0 != pRoot )
    {
        // create assumptions
        Lits[1] = toLitCond( pCnf->pVarNums[Aig_ObjId(pObj0)], pObj0->fPhase );
        // solve the problem
        RetValue = sat_solver_solve( pSat, Lits, Lits+2, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
        if ( RetValue == l_False )
        {
            vCube = Vec_IntAlloc( 1 );
            Vec_IntPush( vCube, toLitCond(Aig_ObjId(pObj0), pObj0->fPhase) );
            Vec_PtrPush( vPrimes, vCube );
            if ( fVerbose )
            printf( "Adding prime %d%c\n", Aig_ObjId(pObj0),  pObj0->fPhase?'-':'+' );
            Vec_IntWriteEntry( vMarks, i0, 1 );
        }
    }
    nNodes1 = Vec_PtrSize(vPrimes);
    if ( nLits > 1 )
    {
        // try adding second literal
        Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj0, i0 )
        if ( pObj0 != pRoot )
        Vec_PtrForEachEntryStart( Aig_Obj_t *, vNodes, pObj1, i1, i0+1 )
        if ( pObj1 != pRoot )
        {
            if ( Vec_IntEntry(vMarks,i0) || Vec_IntEntry(vMarks,i1) )
                continue;
            for ( m = 0; m < 3; m++ )
            {
                fCompl0 =  m & 1;
                fCompl1 = (m >> 1) & 1;
                // create assumptions
                Lits[1] = toLitCond( pCnf->pVarNums[Aig_ObjId(pObj0)], fCompl0 ^ pObj0->fPhase );
                Lits[2] = toLitCond( pCnf->pVarNums[Aig_ObjId(pObj1)], fCompl1 ^ pObj1->fPhase );
                // solve the problem
                RetValue = sat_solver_solve( pSat, Lits, Lits+3, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
                if ( RetValue == l_False )
                {
                    vCube = Vec_IntAlloc( 2 );
                    Vec_IntPush( vCube, toLitCond(Aig_ObjId(pObj0), fCompl0 ^ pObj0->fPhase) );
                    Vec_IntPush( vCube, toLitCond(Aig_ObjId(pObj1), fCompl1 ^ pObj1->fPhase) );
                    Vec_PtrPush( vPrimes, vCube );
                    if ( fVerbose )
                    printf( "Adding prime %d%c %d%c\n", 
                        Aig_ObjId(pObj0), (fCompl0 ^ pObj0->fPhase)?'-':'+', 
                        Aig_ObjId(pObj1), (fCompl1 ^ pObj1->fPhase)?'-':'+' );
                    break;
                }
            }
        }
    }
    nNodes2 = Vec_PtrSize(vPrimes) - nNodes1;

    printf( "Property cone size = %6d    1-lit primes = %5d    2-lit primes = %5d\n", 
        Vec_PtrSize(vNodes), nNodes1, nNodes2 );

    // clean up
    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    Vec_PtrFree( vNodes );
    Vec_IntFree( vMarks );
    return vPrimes;
}

/**Function*************************************************************

  Synopsis    [Performs decomposition of the property output.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManDecPropertyOutput( Aig_Man_t * pAig, int nLits, int fVerbose )
{
    Aig_Man_t * pAigNew = NULL;
    Aig_Obj_t * pObj, * pMiter;
    Vec_Ptr_t * vPrimes;
    Vec_Int_t * vCube;
    int i, k, Lit;

    // compute primes of the comb output function
    vPrimes = Saig_ManFindPrimes( pAig, nLits, fVerbose );

    // start the new manager
    pAigNew = Aig_ManStart( Aig_ManNodeNum(pAig) );
    pAigNew->pName = Abc_UtilStrsav( pAig->pName );
    pAigNew->nConstrs = pAig->nConstrs;
    // map the constant node
    Aig_ManConst1(pAig)->pData = Aig_ManConst1( pAigNew );
    // create variables for PIs
    Aig_ManForEachCi( pAig, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pAigNew );
    // add internal nodes of this frame
    Aig_ManForEachNode( pAig, pObj, i )
        pObj->pData = Aig_And( pAigNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
    // create original POs of the circuit
    Saig_ManForEachPo( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );
    // create prime POs of the circuit
    if ( vPrimes )
    Vec_PtrForEachEntry( Vec_Int_t *, vPrimes, vCube, k )
    {
        pMiter = Aig_ManConst1( pAigNew );
        Vec_IntForEachEntry( vCube, Lit, i )
        {
            pObj = Aig_NotCond( Aig_ObjCopy(Aig_ManObj(pAig, Abc_Lit2Var(Lit))), Abc_LitIsCompl(Lit) );
            pMiter = Aig_And( pAigNew, pMiter, pObj );
        }
        Aig_ObjCreateCo( pAigNew, pMiter );
    }
    // transfer to register outputs
    Saig_ManForEachLi( pAig, pObj, i )
        Aig_ObjCreateCo( pAigNew, Aig_ObjChild0Copy(pObj) );
    Aig_ManCleanup( pAigNew );
    Aig_ManSetRegNum( pAigNew, Aig_ManRegNum(pAig) );

    Vec_VecFreeP( (Vec_Vec_t **)&vPrimes );
    return pAigNew;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

