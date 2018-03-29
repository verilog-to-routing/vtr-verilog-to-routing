/**CFile****************************************************************

  FileName    [abcQuant.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [AIG-based variable quantification.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcQuant.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Performs fast synthesis.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkSynthesize( Abc_Ntk_t ** ppNtk, int fMoreEffort )
{
    extern Abc_Ntk_t * Abc_NtkIvyFraig( Abc_Ntk_t * pNtk, int nConfLimit, int fDoSparse, int fProve, int fTransfer, int fVerbose );

    Abc_Ntk_t * pNtk, * pNtkTemp;

    pNtk = *ppNtk;

    Abc_NtkRewrite( pNtk, 0, 0, 0, 0, 0 );
    Abc_NtkRefactor( pNtk, 10, 16, 0, 0, 0, 0 );
    pNtk = Abc_NtkBalance( pNtkTemp = pNtk, 0, 0, 0 );          
    Abc_NtkDelete( pNtkTemp );

    if ( fMoreEffort )
    {
        Abc_NtkRewrite( pNtk, 0, 0, 0, 0, 0 );
        Abc_NtkRefactor( pNtk, 10, 16, 0, 0, 0, 0 );
        pNtk = Abc_NtkBalance( pNtkTemp = pNtk, 0, 0, 0 );          
        Abc_NtkDelete( pNtkTemp );

        pNtk = Abc_NtkIvyFraig( pNtkTemp = pNtk, 100, 1, 0, 0, 0 );
        Abc_NtkDelete( pNtkTemp );
    }

    *ppNtk = pNtk;
}

/**Function*************************************************************

  Synopsis    [Existentially quantifies one variable.]

  Description []
               
  SideEffects [This procedure creates dangling nodes in the AIG.]

  SeeAlso     []

***********************************************************************/
int Abc_NtkQuantify( Abc_Ntk_t * pNtk, int fUniv, int iVar, int fVerbose )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj, * pNext, * pFanin;
    int i;
    assert( Abc_NtkIsStrash(pNtk) );
    assert( iVar < Abc_NtkCiNum(pNtk) );

    // collect the internal nodes
    pObj = Abc_NtkCi( pNtk, iVar );
    vNodes = Abc_NtkDfsReverseNodes( pNtk, &pObj, 1 );

    // assign the cofactors of the CI node to be constants
    pObj->pCopy = Abc_ObjNot( Abc_AigConst1(pNtk) ); 
    pObj->pData = Abc_AigConst1(pNtk); 

    // quantify the nodes
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
    {
        for ( pNext = pObj? pObj->pCopy : pObj; pObj; pObj = pNext, pNext = pObj? pObj->pCopy : pObj )
        {
            pFanin = Abc_ObjFanin0(pObj);
            if ( !Abc_NodeIsTravIdCurrent(pFanin) )
            {
                pFanin->pCopy = pFanin;
                pFanin->pData = pFanin;
            }
            pFanin = Abc_ObjFanin1(pObj);
            if ( !Abc_NodeIsTravIdCurrent(pFanin) )
            {
                pFanin->pCopy = pFanin;
                pFanin->pData = pFanin;
            }
            pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtk->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
            pObj->pData = Abc_AigAnd( (Abc_Aig_t *)pNtk->pManFunc, Abc_ObjChild0Data(pObj), Abc_ObjChild1Data(pObj) );
        }
    }
    Vec_PtrFree( vNodes );

    // update the affected COs
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        if ( !Abc_NodeIsTravIdCurrent(pObj) )
            continue;
        pFanin = Abc_ObjFanin0(pObj);
        // get the result of quantification
        if ( fUniv )
            pNext = Abc_AigAnd( (Abc_Aig_t *)pNtk->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild0Data(pObj) );
        else
            pNext = Abc_AigOr( (Abc_Aig_t *)pNtk->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild0Data(pObj) );
        pNext = Abc_ObjNotCond( pNext, Abc_ObjFaninC0(pObj) );
        if ( Abc_ObjRegular(pNext) == pFanin )
            continue;
        // update the fanins of the CO
        Abc_ObjPatchFanin( pObj, pFanin, pNext );
//        if ( Abc_ObjFanoutNum(pFanin) == 0 )
//            Abc_AigDeleteNode( pNtk->pManFunc, pFanin );
    }

    // make sure the node has no fanouts
//    pObj = Abc_NtkCi( pNtk, iVar );
//    assert( Abc_ObjFanoutNum(pObj) == 0 );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Constructs the transition relation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkTransRel( Abc_Ntk_t * pNtk, int fInputs, int fVerbose )
{
    char Buffer[1000];
    Vec_Ptr_t * vPairs;
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj, * pMiter;
    int i, nLatches;
    int fSynthesis = 1;

    assert( Abc_NtkIsStrash(pNtk) );
    assert( Abc_NtkLatchNum(pNtk) );
    nLatches = Abc_NtkLatchNum(pNtk);
    // start the network
    pNtkNew = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    // duplicate the name and the spec
    sprintf( Buffer, "%s_TR", pNtk->pName );
    pNtkNew->pName = Extra_UtilStrsav(pNtk->pName);
//    pNtkNew->pSpec = Extra_UtilStrsav(pNtk->pSpec);
    Abc_NtkCleanCopy( pNtk );
    // create current state variables
    Abc_NtkForEachLatchOutput( pNtk, pObj, i )
    {
        pObj->pCopy = Abc_NtkCreatePi(pNtkNew);
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
    }
    // create next state variables
    Abc_NtkForEachLatchInput( pNtk, pObj, i )
        Abc_ObjAssignName( Abc_NtkCreatePi(pNtkNew), Abc_ObjName(pObj), NULL );
    // create PI variables
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_NtkDupObj( pNtkNew, pObj, 1 );
    // create the PO
    Abc_NtkCreatePo( pNtkNew );
    // restrash the nodes (assuming a topological order of the old network)
    Abc_AigConst1(pNtk)->pCopy = Abc_AigConst1(pNtkNew);
    Abc_NtkForEachNode( pNtk, pObj, i )
        pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
    // create the function of the primary output
    assert( Abc_NtkBoxNum(pNtk) == Abc_NtkLatchNum(pNtk) );
    vPairs = Vec_PtrAlloc( 2*nLatches );
    Abc_NtkForEachLatchInput( pNtk, pObj, i )
    {
        Vec_PtrPush( vPairs, Abc_ObjChild0Copy(pObj) );
        Vec_PtrPush( vPairs, Abc_NtkPi(pNtkNew, i+nLatches) );
    }
    pMiter = Abc_AigMiter( (Abc_Aig_t *)pNtkNew->pManFunc, vPairs, 0 );
    Vec_PtrFree( vPairs );
    // add the primary output
    Abc_ObjAddFanin( Abc_NtkPo(pNtkNew,0), Abc_ObjNot(pMiter) );
    Abc_ObjAssignName( Abc_NtkPo(pNtkNew,0), "rel", NULL );

    // quantify inputs
    if ( fInputs )
    {
        assert( Abc_NtkPiNum(pNtkNew) == Abc_NtkPiNum(pNtk) + 2*nLatches );
        for ( i = Abc_NtkPiNum(pNtkNew) - 1; i >= 2*nLatches; i-- )
//        for ( i = 2*nLatches; i < Abc_NtkPiNum(pNtkNew); i++ )
        {
            Abc_NtkQuantify( pNtkNew, 0, i, fVerbose );
//            if ( fSynthesis && (i % 3 == 2) )
            if ( fSynthesis  )
            {
                Abc_NtkCleanData( pNtkNew );
                Abc_AigCleanup( (Abc_Aig_t *)pNtkNew->pManFunc );
                Abc_NtkSynthesize( &pNtkNew, 1 );
            }
//            printf( "Var = %3d. Nodes = %6d. ", Abc_NtkPiNum(pNtkNew) - 1 - i, Abc_NtkNodeNum(pNtkNew) );
//            printf( "Var = %3d. Nodes = %6d. ", i - 2*nLatches, Abc_NtkNodeNum(pNtkNew) );
        }
//        printf( "\n" );
        Abc_NtkCleanData( pNtkNew );
        Abc_AigCleanup( (Abc_Aig_t *)pNtkNew->pManFunc );
        for ( i = Abc_NtkPiNum(pNtkNew) - 1; i >= 2*nLatches; i-- )
        {
            pObj = Abc_NtkPi( pNtkNew, i );
            assert( Abc_ObjFanoutNum(pObj) == 0 );
            Abc_NtkDeleteObj( pObj );
        }
    }

    // check consistency of the network
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "Abc_NtkTransRel: The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }
    return pNtkNew;
}


/**Function*************************************************************

  Synopsis    [Performs one image computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkInitialState( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pMiter;
    int i, nVars = Abc_NtkPiNum(pNtk)/2;
    assert( Abc_NtkIsStrash(pNtk) );
    // start the new network 
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // compute the all-zero state in terms of the CS variables
    pMiter = Abc_AigConst1(pNtkNew);
    for ( i = 0; i < nVars; i++ )
        pMiter = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, pMiter, Abc_ObjNot( Abc_NtkPi(pNtkNew, i) ) );
    // add the PO
    Abc_ObjAddFanin( Abc_NtkPo(pNtkNew,0), pMiter );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Swaps current state and next state variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkSwapVariables( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pMiter, * pObj, * pObj0, * pObj1;
    int i, nVars = Abc_NtkPiNum(pNtk)/2;
    assert( Abc_NtkIsStrash(pNtk) );
    // start the new network 
    pNtkNew = Abc_NtkStartFrom( pNtk, ABC_NTK_STRASH, ABC_FUNC_AIG );
    // update the PIs
    for ( i = 0; i < nVars; i++ )
    {
        pObj0 = Abc_NtkPi( pNtk, i );
        pObj1 = Abc_NtkPi( pNtk, i+nVars );
        pMiter = pObj0->pCopy;
        pObj0->pCopy = pObj1->pCopy;
        pObj1->pCopy = pMiter;
    }
    // restrash
    Abc_NtkForEachNode( pNtk, pObj, i )
        pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkNew->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
    // add the PO
    pMiter = Abc_ObjChild0Copy( Abc_NtkPo(pNtk,0) );
    Abc_ObjAddFanin( Abc_NtkPo(pNtkNew,0), pMiter );
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Performs reachability analisys.]

  Description [Assumes that the input is the transition relation.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkReachability( Abc_Ntk_t * pNtkRel, int nIters, int fVerbose )
{
    Abc_Obj_t * pObj;
    Abc_Ntk_t * pNtkFront, * pNtkReached, * pNtkNext, * pNtkTemp;
    int i, v, nVars, nNodesOld, nNodesNew, nNodesPrev;
    int fFixedPoint = 0;
    int fSynthesis  = 1;
    int fMoreEffort = 1;
    abctime clk;

    assert( Abc_NtkIsStrash(pNtkRel) );
    assert( Abc_NtkLatchNum(pNtkRel) == 0 );
    assert( Abc_NtkPiNum(pNtkRel) % 2 == 0 );

    // compute the network composed of the initial states
    pNtkFront = Abc_NtkInitialState( pNtkRel );
    pNtkReached = Abc_NtkDup( pNtkFront );
//Abc_NtkShow( pNtkReached, 0, 0, 0 );

//    if ( fVerbose )
//        printf( "Transition relation = %6d.\n", Abc_NtkNodeNum(pNtkRel) );

    // perform iterations of reachability analysis
    nNodesPrev = Abc_NtkNodeNum(pNtkFront);
    nVars = Abc_NtkPiNum(pNtkRel)/2;
    for ( i = 0; i < nIters; i++ )
    {
        clk = Abc_Clock();
        // get the set of next states
        pNtkNext = Abc_NtkMiterAnd( pNtkRel, pNtkFront, 0, 0 );
        Abc_NtkDelete( pNtkFront );
        // quantify the current state variables
        for ( v = 0; v < nVars; v++ )
        {
            Abc_NtkQuantify( pNtkNext, 0, v, fVerbose );
            if ( fSynthesis && (v % 3 == 2) )
            {
                Abc_NtkCleanData( pNtkNext );
                Abc_AigCleanup( (Abc_Aig_t *)pNtkNext->pManFunc );
                Abc_NtkSynthesize( &pNtkNext, fMoreEffort );
            }
        }
        Abc_NtkCleanData( pNtkNext );
        Abc_AigCleanup( (Abc_Aig_t *)pNtkNext->pManFunc );
        if ( fSynthesis )
            Abc_NtkSynthesize( &pNtkNext, 1 );
        // map the next states into the current states
        pNtkNext = Abc_NtkSwapVariables( pNtkTemp = pNtkNext );
        Abc_NtkDelete( pNtkTemp );
        // check the termination condition
        if ( Abc_ObjFanin0(Abc_NtkPo(pNtkNext,0)) == Abc_AigConst1(pNtkNext) )
        {
            fFixedPoint = 1;
            printf( "Fixed point is reached!\n" );
            Abc_NtkDelete( pNtkNext );
            break;
        }
        // compute new front
        pNtkFront = Abc_NtkMiterAnd( pNtkNext, pNtkReached, 0, 1 );
        Abc_NtkDelete( pNtkNext );
        // add the reached states
        pNtkReached = Abc_NtkMiterAnd( pNtkTemp = pNtkReached, pNtkFront, 1, 0 );
        Abc_NtkDelete( pNtkTemp );
        // compress the size of Front
        nNodesOld = Abc_NtkNodeNum(pNtkFront);
        if ( fSynthesis )
        {
            Abc_NtkSynthesize( &pNtkFront, fMoreEffort );
            Abc_NtkSynthesize( &pNtkReached, fMoreEffort );
        }
        nNodesNew = Abc_NtkNodeNum(pNtkFront);
        // print statistics
        if ( fVerbose )
        {
            printf( "I = %3d : Reach = %6d  Fr = %6d  FrM = %6d  %7.2f %%   ", 
                i + 1, Abc_NtkNodeNum(pNtkReached), nNodesOld, nNodesNew, 100.0*(nNodesNew-nNodesPrev)/nNodesPrev );
            ABC_PRT( "T", Abc_Clock() - clk );
        }
        nNodesPrev = Abc_NtkNodeNum(pNtkFront);
    }
    if ( !fFixedPoint )
        fprintf( stdout, "Reachability analysis stopped after %d iterations.\n", nIters );

    // complement the output to represent the set of unreachable states
    Abc_ObjXorFaninC( Abc_NtkPo(pNtkReached,0), 0 );

    // remove next state variables
    for ( i = 2*nVars - 1; i >= nVars; i-- )
    {
        pObj = Abc_NtkPi( pNtkReached, i );
        assert( Abc_ObjFanoutNum(pObj) == 0 );
        Abc_NtkDeleteObj( pObj );
    }

    // check consistency of the network
    if ( !Abc_NtkCheck( pNtkReached ) )
    {
        printf( "Abc_NtkReachability: The network check has failed.\n" );
        Abc_NtkDelete( pNtkReached );
        return NULL;
    }
    return pNtkReached;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

