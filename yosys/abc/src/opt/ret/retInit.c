/**CFile****************************************************************

  FileName    [retInit.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Retiming package.]

  Synopsis    [Initial state computation for backward retiming.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Oct 31, 2006.]

  Revision    [$Id: retInit.c,v 1.00 2006/10/31 00:00:00 alanmi Exp $]

***********************************************************************/

#include "retInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int Abc_NtkRetimeVerifyModel( Abc_Ntk_t * pNtkCone, Vec_Int_t * vValues, int * pModel );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes initial values of the new latches.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkRetimeInitialValues( Abc_Ntk_t * pNtkCone, Vec_Int_t * vValues, int fVerbose )
{
    Vec_Int_t * vSolution;
    Abc_Ntk_t * pNtkMiter, * pNtkLogic;
    int RetValue;
    abctime clk;
    if ( pNtkCone == NULL )
        return Vec_IntDup( vValues );
    // convert the target network to AIG
    pNtkLogic = Abc_NtkDup( pNtkCone );
    Abc_NtkToAig( pNtkLogic );
    // get the miter
    pNtkMiter = Abc_NtkCreateTarget( pNtkLogic, pNtkLogic->vCos, vValues );
    if ( fVerbose )
        printf( "The miter for initial state computation has %d AIG nodes. ", Abc_NtkNodeNum(pNtkMiter) );
    // solve the miter
    clk = Abc_Clock();
    RetValue = Abc_NtkMiterSat( pNtkMiter, (ABC_INT64_T)500000, (ABC_INT64_T)50000000, 0, NULL, NULL );
    if ( fVerbose ) 
        { ABC_PRT( "SAT solving time", Abc_Clock() - clk ); }
    // analyze the result
    if ( RetValue == 1 )
        printf( "Abc_NtkRetimeInitialValues(): The problem is unsatisfiable. DC latch values are used.\n" );
    else if ( RetValue == -1 )
        printf( "Abc_NtkRetimeInitialValues(): The SAT problem timed out. DC latch values are used.\n" );
    else if ( !Abc_NtkRetimeVerifyModel( pNtkCone, vValues, pNtkMiter->pModel ) )
        printf( "Abc_NtkRetimeInitialValues(): The computed counter-example is incorrect.\n" );
    // set the values of the latches
    vSolution = RetValue? NULL : Vec_IntAllocArray( pNtkMiter->pModel, Abc_NtkPiNum(pNtkLogic) );
    pNtkMiter->pModel = NULL;
    Abc_NtkDelete( pNtkMiter );
    Abc_NtkDelete( pNtkLogic );
    return vSolution;
}

/**Function*************************************************************

  Synopsis    [Computes the results of simulating one node.]

  Description [Assumes that fanins have pCopy set to the input values.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_ObjSopSimulate( Abc_Obj_t * pObj )
{
    char * pCube, * pSop = (char *)pObj->pData;
    int nVars, Value, v, ResOr, ResAnd, ResVar;
    assert( pSop && !Abc_SopIsExorType(pSop) );
    // simulate the SOP of the node
    ResOr = 0;
    nVars = Abc_SopGetVarNum(pSop);
    Abc_SopForEachCube( pSop, nVars, pCube )
    {
        ResAnd = 1;
        Abc_CubeForEachVar( pCube, Value, v )
        {
            if ( Value == '0' )
                ResVar = 1 ^ ((int)(ABC_PTRUINT_T)Abc_ObjFanin(pObj, v)->pCopy);
            else if ( Value == '1' )
                ResVar = (int)(ABC_PTRUINT_T)Abc_ObjFanin(pObj, v)->pCopy;
            else
                continue;
            ResAnd &= ResVar;
        }
        ResOr |= ResAnd;
    }
    // complement the result if necessary
    if ( !Abc_SopGetPhase(pSop) )
        ResOr ^= 1;
    return ResOr;
}

/**Function*************************************************************

  Synopsis    [Verifies the counter-example.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkRetimeVerifyModel( Abc_Ntk_t * pNtkCone, Vec_Int_t * vValues, int * pModel )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i, Counter = 0;
    assert( Abc_NtkIsSopLogic(pNtkCone) );
    // set the PIs
    Abc_NtkForEachPi( pNtkCone, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)pModel[i];
    // simulate the internal nodes
    vNodes = Abc_NtkDfs( pNtkCone, 0 );
    Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)Abc_ObjSopSimulate( pObj );
    Vec_PtrFree( vNodes );
    // compare the outputs
    Abc_NtkForEachPo( pNtkCone, pObj, i )
        pObj->pCopy = Abc_ObjFanin0(pObj)->pCopy;
    Abc_NtkForEachPo( pNtkCone, pObj, i )
        Counter += (Vec_IntEntry(vValues, i) != (int)(ABC_PTRUINT_T)pObj->pCopy);
    if ( Counter > 0 )
        printf( "%d outputs (out of %d) have a value mismatch.\n", Counter, Abc_NtkPoNum(pNtkCone) );
    return 1;
}

/**Function*************************************************************

  Synopsis    [Transfer latch initial values to pCopy.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRetimeTranferToCopy( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( Abc_ObjIsLatch(pObj) )
            pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)Abc_LatchIsInit1(pObj);
}

/**Function*************************************************************

  Synopsis    [Transfer latch initial values from pCopy.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRetimeTranferFromCopy( Abc_Ntk_t * pNtk )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( Abc_ObjIsLatch(pObj) )
            pObj->pData = (void *)(ABC_PTRUINT_T)(pObj->pCopy? ABC_INIT_ONE : ABC_INIT_ZERO);
}

/**Function*************************************************************

  Synopsis    [Transfer latch initial values to pCopy.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Abc_NtkRetimeCollectLatchValues( Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vValues;
    Abc_Obj_t * pObj;
    int i;
    vValues = Vec_IntAlloc( Abc_NtkLatchNum(pNtk) );
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( Abc_ObjIsLatch(pObj) )
            Vec_IntPush( vValues, Abc_LatchIsInit1(pObj) );
    return vValues;
}

/**Function*************************************************************

  Synopsis    [Transfer latch initial values from pCopy.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRetimeInsertLatchValues( Abc_Ntk_t * pNtk, Vec_Int_t * vValues )
{
    Abc_Obj_t * pObj;
    int i, Counter = 0;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( Abc_ObjIsLatch(pObj) )
            pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)Counter++;
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( Abc_ObjIsLatch(pObj) )
            pObj->pData = (Abc_Obj_t *)(ABC_PTRUINT_T)(vValues? (Vec_IntEntry(vValues,(int)(ABC_PTRUINT_T)pObj->pCopy)? ABC_INIT_ONE : ABC_INIT_ZERO) : ABC_INIT_DC);
}

/**Function*************************************************************

  Synopsis    [Transfer latch initial values to pCopy.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Ntk_t * Abc_NtkRetimeBackwardInitialStart( Abc_Ntk_t * pNtk )
{
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj;
    int i;
    // create the network used for the initial state computation
    pNtkNew = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    // create POs corresponding to the initial values
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( Abc_ObjIsLatch(pObj) )
            pObj->pCopy = Abc_NtkCreatePo(pNtkNew);
    return pNtkNew;
}

/**Function*************************************************************

  Synopsis    [Transfer latch initial values to pCopy.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkRetimeBackwardInitialFinish( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkNew, Vec_Int_t * vValuesOld, int fVerbose )
{
    Vec_Int_t * vValuesNew;
    Abc_Obj_t * pObj;
    int i;
    // create PIs corresponding to the initial values
    Abc_NtkForEachObj( pNtk, pObj, i )
        if ( Abc_ObjIsLatch(pObj) )
            Abc_ObjAddFanin( pObj->pCopy, Abc_NtkCreatePi(pNtkNew) );
    // assign dummy node names
    Abc_NtkAddDummyPiNames( pNtkNew );
    Abc_NtkAddDummyPoNames( pNtkNew );
    // check the network
    if ( !Abc_NtkCheck( pNtkNew ) )
        fprintf( stdout, "Abc_NtkRetimeBackwardInitialFinish(): Network check has failed.\n" );
    // derive new initial values
    vValuesNew = Abc_NtkRetimeInitialValues( pNtkNew, vValuesOld, fVerbose );
    // insert new initial values
    Abc_NtkRetimeInsertLatchValues( pNtk, vValuesNew );
    if ( vValuesNew ) Vec_IntFree( vValuesNew );
}


/**Function*************************************************************

  Synopsis    [Cycles the circuit to create a new initial state.]

  Description [Simulates the circuit with random input for the given 
  number of timeframes to get a better initial state.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkCycleInitStateSop( Abc_Ntk_t * pNtk, int nFrames, int fVerbose )
{
    Vec_Ptr_t * vNodes;
    Abc_Obj_t * pObj;
    int i, f;
    assert( Abc_NtkIsSopLogic(pNtk) );
    srand( 0x12341234 );
    // initialize the values
    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)(rand() & 1);
    Abc_NtkForEachLatch( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)Abc_LatchIsInit1(pObj);
    // simulate for the given number of timeframes
    vNodes = Abc_NtkDfs( pNtk, 0 );
    for ( f = 0; f < nFrames; f++ )
    {
        // simulate internal nodes
        Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i )
            pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)Abc_ObjSopSimulate( pObj );
        // bring the results to the COs
        Abc_NtkForEachCo( pNtk, pObj, i )
            pObj->pCopy = Abc_ObjFanin0(pObj)->pCopy;
        // assign PI values
        Abc_NtkForEachPi( pNtk, pObj, i )
            pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)(rand() & 1);
        // transfer the latch values
        Abc_NtkForEachLatch( pNtk, pObj, i )
            Abc_ObjFanout0(pObj)->pCopy = Abc_ObjFanin0(pObj)->pCopy;
    }
    Vec_PtrFree( vNodes );
    // set the final values
    Abc_NtkForEachLatch( pNtk, pObj, i )
        pObj->pData = (void *)(ABC_PTRUINT_T)(Abc_ObjFanout0(pObj)->pCopy ? ABC_INIT_ONE : ABC_INIT_ZERO);
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

