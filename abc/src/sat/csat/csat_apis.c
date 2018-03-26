/**CFile****************************************************************

  FileName    [csat_apis.h]

  PackageName [Interface to CSAT.]

  Synopsis    [APIs, enums, and data structures expected from the use of CSAT.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - August 28, 2005]

  Revision    [$Id: csat_apis.h,v 1.00 2005/08/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "proof/fraig/fraig.h"
#include "csat_apis.h"
#include "misc/st/stmm.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define ABC_DEFAULT_CONF_LIMIT     0   // limit on conflicts
#define ABC_DEFAULT_IMP_LIMIT      0   // limit on implications


struct ABC_ManagerStruct_t
{
    // information about the problem
    stmm_table *          tName2Node;    // the hash table mapping names to nodes
    stmm_table *          tNode2Name;    // the hash table mapping nodes to names
    Abc_Ntk_t *           pNtk;          // the starting ABC network
    Abc_Ntk_t *           pTarget;       // the AIG representing the target
    char *                pDumpFileName; // the name of the file to dump the target network
    Mem_Flex_t *      pMmNames;      // memory manager for signal names
    // solving parameters
    int                   mode;          // 0 = resource-aware integration; 1 = brute-force SAT
    Prove_Params_t        Params;        // integrated CEC parameters
    // information about the target 
    int                   nog;           // the numbers of gates in the target
    Vec_Ptr_t *           vNodes;        // the gates in the target
    Vec_Int_t *           vValues;       // the values of gate's outputs in the target
    // solution
    CSAT_Target_ResultT * pResult;       // the result of solving the target
};

static CSAT_Target_ResultT * ABC_TargetResAlloc( int nVars );
static char * ABC_GetNodeName( ABC_Manager mng, Abc_Obj_t * pNode );

// procedures to start and stop the ABC framework
extern void  Abc_Start();
extern void  Abc_Stop();

// some external procedures
extern int Io_WriteBench( Abc_Ntk_t * pNtk, const char * FileName );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Creates a new manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
ABC_Manager ABC_InitManager()
{
    ABC_Manager_t * mng;
    Abc_Start();
    mng = ABC_ALLOC( ABC_Manager_t, 1 );
    memset( mng, 0, sizeof(ABC_Manager_t) );
    mng->pNtk = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
    mng->pNtk->pName = Extra_UtilStrsav("csat_network");
    mng->tName2Node = stmm_init_table(strcmp, stmm_strhash);
    mng->tNode2Name = stmm_init_table(stmm_ptrcmp, stmm_ptrhash);
    mng->pMmNames   = Mem_FlexStart();
    mng->vNodes     = Vec_PtrAlloc( 100 );
    mng->vValues    = Vec_IntAlloc( 100 );
    mng->mode       = 0; // set "resource-aware integration" as the default mode
    // set default parameters for CEC
    Prove_ParamsSetDefault( &mng->Params );
    // set infinite resource limit for the final mitering
//    mng->Params.nMiteringLimitLast = ABC_INFINITY;
    return mng;
}

/**Function*************************************************************

  Synopsis    [Deletes the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_ReleaseManager( ABC_Manager mng )
{
    CSAT_Target_ResultT * p_res = ABC_Get_Target_Result( mng,0 );
    ABC_TargetResFree(p_res);
    if ( mng->tNode2Name ) stmm_free_table( mng->tNode2Name );
    if ( mng->tName2Node ) stmm_free_table( mng->tName2Node );
    if ( mng->pMmNames )   Mem_FlexStop( mng->pMmNames, 0 );
    if ( mng->pNtk )       Abc_NtkDelete( mng->pNtk );
    if ( mng->pTarget )    Abc_NtkDelete( mng->pTarget );
    if ( mng->vNodes )     Vec_PtrFree( mng->vNodes );
    if ( mng->vValues )    Vec_IntFree( mng->vValues );
    ABC_FREE( mng->pDumpFileName );
    ABC_FREE( mng );
    Abc_Stop();
}

/**Function*************************************************************

  Synopsis    [Sets solver options for learning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_SetSolveOption( ABC_Manager mng, enum CSAT_OptionT option )
{
}

/**Function*************************************************************

  Synopsis    [Sets solving mode by brute-force SAT.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_UseOnlyCoreSatSolver( ABC_Manager mng )
{
    mng->mode = 1;  // switch to "brute-force SAT" as the solving option
}

/**Function*************************************************************

  Synopsis    [Adds a gate to the circuit.]

  Description [The meaning of the parameters are:
    type: the type of the gate to be added
    name: the name of the gate to be added, name should be unique in a circuit.
    nofi: number of fanins of the gate to be added;
    fanins: the name array of fanins of the gate to be added.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int ABC_AddGate( ABC_Manager mng, enum GateType type, char * name, int nofi, char ** fanins, int dc_attr )
{
    Abc_Obj_t * pObj = NULL; // Suppress "might be used uninitialized"
    Abc_Obj_t * pFanin;
    char * pSop = NULL; // Suppress "might be used uninitialized"
    char * pNewName;
    int i;

    // save the name in the local memory manager
    pNewName = Mem_FlexEntryFetch( mng->pMmNames, strlen(name) + 1 );
    strcpy( pNewName, name );
    name = pNewName;

    // consider different cases, create the node, and map the node into the name
    switch( type )
    {
    case CSAT_BPI:
    case CSAT_BPPI:
        if ( nofi != 0 )
            { printf( "ABC_AddGate: The PI/PPI gate \"%s\" has fanins.\n", name ); return 0; }
        // create the PI
        pObj = Abc_NtkCreatePi( mng->pNtk );
        stmm_insert( mng->tNode2Name, (char *)pObj, name );
        break;
    case CSAT_CONST:
    case CSAT_BAND:
    case CSAT_BNAND:
    case CSAT_BOR:
    case CSAT_BNOR:
    case CSAT_BXOR:
    case CSAT_BXNOR:
    case CSAT_BINV:
    case CSAT_BBUF:
        // create the node
        pObj = Abc_NtkCreateNode( mng->pNtk );
        // create the fanins
        for ( i = 0; i < nofi; i++ )
        {
            if ( !stmm_lookup( mng->tName2Node, fanins[i], (char **)&pFanin ) )
                { printf( "ABC_AddGate: The fanin gate \"%s\" is not in the network.\n", fanins[i] ); return 0; }
            Abc_ObjAddFanin( pObj, pFanin );
        }
        // create the node function
        switch( type )
        {
            case CSAT_CONST:
                if ( nofi != 0 )
                    { printf( "ABC_AddGate: The constant gate \"%s\" has fanins.\n", name ); return 0; }
                pSop = Abc_SopCreateConst1( (Mem_Flex_t *)mng->pNtk->pManFunc );
                break;
            case CSAT_BAND:
                if ( nofi < 1 )
                    { printf( "ABC_AddGate: The AND gate \"%s\" no fanins.\n", name ); return 0; }
                pSop = Abc_SopCreateAnd( (Mem_Flex_t *)mng->pNtk->pManFunc, nofi, NULL );
                break;
            case CSAT_BNAND:
                if ( nofi < 1 )
                    { printf( "ABC_AddGate: The NAND gate \"%s\" no fanins.\n", name ); return 0; }
                pSop = Abc_SopCreateNand( (Mem_Flex_t *)mng->pNtk->pManFunc, nofi );
                break;
            case CSAT_BOR:
                if ( nofi < 1 )
                    { printf( "ABC_AddGate: The OR gate \"%s\" no fanins.\n", name ); return 0; }
                pSop = Abc_SopCreateOr( (Mem_Flex_t *)mng->pNtk->pManFunc, nofi, NULL );
                break;
            case CSAT_BNOR:
                if ( nofi < 1 )
                    { printf( "ABC_AddGate: The NOR gate \"%s\" no fanins.\n", name ); return 0; }
                pSop = Abc_SopCreateNor( (Mem_Flex_t *)mng->pNtk->pManFunc, nofi );
                break;
            case CSAT_BXOR:
                if ( nofi < 1 )
                    { printf( "ABC_AddGate: The XOR gate \"%s\" no fanins.\n", name ); return 0; }
                if ( nofi > 2 )
                    { printf( "ABC_AddGate: The XOR gate \"%s\" has more than two fanins.\n", name ); return 0; }
                pSop = Abc_SopCreateXor( (Mem_Flex_t *)mng->pNtk->pManFunc, nofi );
                break;
            case CSAT_BXNOR:
                if ( nofi < 1 )
                    { printf( "ABC_AddGate: The XNOR gate \"%s\" no fanins.\n", name ); return 0; }
                if ( nofi > 2 )
                    { printf( "ABC_AddGate: The XNOR gate \"%s\" has more than two fanins.\n", name ); return 0; }
                pSop = Abc_SopCreateNxor( (Mem_Flex_t *)mng->pNtk->pManFunc, nofi );
                break;
            case CSAT_BINV:
                if ( nofi != 1 )
                    { printf( "ABC_AddGate: The inverter gate \"%s\" does not have exactly one fanin.\n", name ); return 0; }
                pSop = Abc_SopCreateInv( (Mem_Flex_t *)mng->pNtk->pManFunc );
                break;
            case CSAT_BBUF:
                if ( nofi != 1 )
                    { printf( "ABC_AddGate: The buffer gate \"%s\" does not have exactly one fanin.\n", name ); return 0; }
                pSop = Abc_SopCreateBuf( (Mem_Flex_t *)mng->pNtk->pManFunc );
                break;
            default :
                break;
        }
        Abc_ObjSetData( pObj, pSop );
        break;
    case CSAT_BPPO:
    case CSAT_BPO:
        if ( nofi != 1 )
            { printf( "ABC_AddGate: The PO/PPO gate \"%s\" does not have exactly one fanin.\n", name ); return 0; }
        // create the PO
        pObj = Abc_NtkCreatePo( mng->pNtk );
        stmm_insert( mng->tNode2Name, (char *)pObj, name );
        // connect to the PO fanin
        if ( !stmm_lookup( mng->tName2Node, fanins[0], (char **)&pFanin ) )
            { printf( "ABC_AddGate: The fanin gate \"%s\" is not in the network.\n", fanins[0] ); return 0; }
        Abc_ObjAddFanin( pObj, pFanin );
        break;
    default:
        printf( "ABC_AddGate: Unknown gate type.\n" );
        break;
    }

    // map the name into the node
    if ( stmm_insert( mng->tName2Node, name, (char *)pObj ) )
        { printf( "ABC_AddGate: The same gate \"%s\" is added twice.\n", name ); return 0; }
    return 1;
}

/**Function*************************************************************

  Synopsis    [This procedure also finalizes construction of the ABC network.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_Network_Finalize( ABC_Manager mng )
{
    Abc_Ntk_t * pNtk = mng->pNtk;
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkForEachPi( pNtk, pObj, i )
        Abc_ObjAssignName( pObj, ABC_GetNodeName(mng, pObj), NULL );
    Abc_NtkForEachPo( pNtk, pObj, i )
        Abc_ObjAssignName( pObj, ABC_GetNodeName(mng, pObj), NULL );
    assert( Abc_NtkLatchNum(pNtk) == 0 );
}

/**Function*************************************************************

  Synopsis    [Checks integraty of the manager.]

  Description [Checks if there are gates that are not used by any primary output.
  If no such gates exist, return 1 else return 0.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int ABC_Check_Integrity( ABC_Manager mng )
{
    Abc_Ntk_t * pNtk = mng->pNtk;
    Abc_Obj_t * pObj;
    int i;

    // check that there are no dangling nodes
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        if ( i == 0 ) 
            continue;
        if ( Abc_ObjFanoutNum(pObj) == 0 )
        {
//            printf( "ABC_Check_Integrity: The network has dangling nodes.\n" );
            return 0;
        }
    }

    // make sure everything is okay with the network structure
    if ( !Abc_NtkDoCheck( pNtk ) )
    {
        printf( "ABC_Check_Integrity: The internal network check has failed.\n" );
        return 0;
    }
    return 1;
}

/**Function*************************************************************

  Synopsis    [Sets time limit for solving a target.]

  Description [Runtime: time limit (in second).]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_SetTimeLimit( ABC_Manager mng, int runtime )
{
//    printf( "ABC_SetTimeLimit: The resource limit is not implemented (warning).\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_SetLearnLimit( ABC_Manager mng, int num )
{
//    printf( "ABC_SetLearnLimit: The resource limit is not implemented (warning).\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_SetLearnBacktrackLimit( ABC_Manager mng, int num )
{
//    printf( "ABC_SetLearnBacktrackLimit: The resource limit is not implemented (warning).\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_SetSolveBacktrackLimit( ABC_Manager mng, int num )
{
    mng->Params.nMiteringLimitLast = num;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_SetSolveImplicationLimit( ABC_Manager mng, int num )
{
//    printf( "ABC_SetSolveImplicationLimit: The resource limit is not implemented (warning).\n" );
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_SetTotalBacktrackLimit( ABC_Manager mng, ABC_UINT64_T num )
{
    mng->Params.nTotalBacktrackLimit = num;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_SetTotalInspectLimit( ABC_Manager mng, ABC_UINT64_T num )
{
    mng->Params.nTotalInspectLimit = num;
}
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
ABC_UINT64_T ABC_GetTotalBacktracksMade( ABC_Manager mng )
{
    return mng->Params.nTotalBacktracksMade;
}

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
ABC_UINT64_T ABC_GetTotalInspectsMade( ABC_Manager mng )
{
    return mng->Params.nTotalInspectsMade;
}

/**Function*************************************************************

  Synopsis    [Sets the file name to dump the structurally hashed network used for solving.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_EnableDump( ABC_Manager mng, char * dump_file )
{
    ABC_FREE( mng->pDumpFileName );
    mng->pDumpFileName = Extra_UtilStrsav( dump_file );
}

/**Function*************************************************************

  Synopsis    [Adds a new target to the manager.]

  Description [The meaning of the parameters are:
    nog: number of gates that are in the targets,
    names: name array of gates,
    values: value array of the corresponding gates given in "names" to be solved. 
    The relation of them is AND.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int ABC_AddTarget( ABC_Manager mng, int nog, char ** names, int * values )
{
    Abc_Obj_t * pObj;
    int i;
    if ( nog < 1 )
        { printf( "ABC_AddTarget: The target has no gates.\n" ); return 0; }
    // clear storage for the target
    mng->nog = 0;
    Vec_PtrClear( mng->vNodes );
    Vec_IntClear( mng->vValues );
    // save the target
    for ( i = 0; i < nog; i++ )
    {
        if ( !stmm_lookup( mng->tName2Node, names[i], (char **)&pObj ) )
            { printf( "ABC_AddTarget: The target gate \"%s\" is not in the network.\n", names[i] ); return 0; }
        Vec_PtrPush( mng->vNodes, pObj );
        if ( values[i] < 0 || values[i] > 1 )
            { printf( "ABC_AddTarget: The value of gate \"%s\" is not 0 or 1.\n", names[i] ); return 0; }
        Vec_IntPush( mng->vValues, values[i] );
    }
    mng->nog = nog;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Initialize the solver internal data structure.]

  Description [Prepares the solver to work on one specific target
  set by calling ABC_AddTarget before.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_SolveInit( ABC_Manager mng )
{
    // check if the target is available
    assert( mng->nog == Vec_PtrSize(mng->vNodes) );
    if ( mng->nog == 0 )
        { printf( "ABC_SolveInit: Target is not specified by ABC_AddTarget().\n" ); return; }

    // free the previous target network if present
    if ( mng->pTarget ) Abc_NtkDelete( mng->pTarget );

    // set the new target network
//    mng->pTarget = Abc_NtkCreateTarget( mng->pNtk, mng->vNodes, mng->vValues );
    mng->pTarget = Abc_NtkStrash( mng->pNtk, 0, 1, 0 );
}

/**Function*************************************************************

  Synopsis    [Currently not implemented.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_AnalyzeTargets( ABC_Manager mng )
{
}

/**Function*************************************************************

  Synopsis    [Solves the targets added by ABC_AddTarget().]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
enum CSAT_StatusT ABC_Solve( ABC_Manager mng )
{
    Prove_Params_t * pParams = &mng->Params;
    int RetValue, i;

    // check if the target network is available
    if ( mng->pTarget == NULL )
        { printf( "ABC_Solve: Target network is not derived by ABC_SolveInit().\n" ); return UNDETERMINED; }

    // try to prove the miter using a number of techniques
    if ( mng->mode )
        RetValue = Abc_NtkMiterSat( mng->pTarget, (ABC_INT64_T)pParams->nMiteringLimitLast, (ABC_INT64_T)0, 0, NULL, NULL );
    else
//        RetValue = Abc_NtkMiterProve( &mng->pTarget, pParams ); // old CEC engine
        RetValue = Abc_NtkIvyProve( &mng->pTarget, pParams ); // new CEC engine

    // analyze the result
    mng->pResult = ABC_TargetResAlloc( Abc_NtkCiNum(mng->pTarget) );
    if ( RetValue == -1 )
        mng->pResult->status = UNDETERMINED;
    else if ( RetValue == 1 )
        mng->pResult->status = UNSATISFIABLE;
    else if ( RetValue == 0 )
    {
        mng->pResult->status = SATISFIABLE;
        // create the array of PI names and values
        for ( i = 0; i < mng->pResult->no_sig; i++ )
        {
            mng->pResult->names[i]  = Extra_UtilStrsav( ABC_GetNodeName(mng, Abc_NtkCi(mng->pNtk, i)) ); 
            mng->pResult->values[i] = mng->pTarget->pModel[i];
        }
        ABC_FREE( mng->pTarget->pModel );
    }
    else assert( 0 );

    // delete the target
    Abc_NtkDelete( mng->pTarget );
    mng->pTarget = NULL;
    // return the status
    return mng->pResult->status;
}

/**Function*************************************************************

  Synopsis    [Gets the solve status of a target.]

  Description [TargetID: the target id returned by ABC_AddTarget().]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
CSAT_Target_ResultT * ABC_Get_Target_Result( ABC_Manager mng, int TargetID )
{
    return mng->pResult;
}

/**Function*************************************************************

  Synopsis    [Dumps the original network into the BENCH file.]

  Description [This procedure should be modified to dump the target.]
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
void ABC_Dump_Bench_File( ABC_Manager mng )
{
    Abc_Ntk_t * pNtkTemp, * pNtkAig;
    const char * pFileName;
 
    // derive the netlist
    pNtkAig = Abc_NtkStrash( mng->pNtk, 0, 0, 0 );
    pNtkTemp = Abc_NtkToNetlistBench( pNtkAig );
    Abc_NtkDelete( pNtkAig );
    if ( pNtkTemp == NULL ) 
        { printf( "ABC_Dump_Bench_File: Dumping BENCH has failed.\n" ); return; }
    pFileName = mng->pDumpFileName? mng->pDumpFileName: "abc_test.bench";
    Io_WriteBench( pNtkTemp, pFileName );
    Abc_NtkDelete( pNtkTemp );
}



/**Function*************************************************************

  Synopsis    [Allocates the target result.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
CSAT_Target_ResultT * ABC_TargetResAlloc( int nVars )
{
    CSAT_Target_ResultT * p;
    p = ABC_ALLOC( CSAT_Target_ResultT, 1 );
    memset( p, 0, sizeof(CSAT_Target_ResultT) );
    p->no_sig = nVars;
    p->names = ABC_ALLOC( char *, nVars );
    p->values = ABC_ALLOC( int, nVars );
    memset( p->names, 0, sizeof(char *) * nVars );
    memset( p->values, 0, sizeof(int) * nVars );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates the target result.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void ABC_TargetResFree( CSAT_Target_ResultT * p )
{
    if ( p == NULL )
        return;
    if( p->names )
    {
        int i = 0;
        for ( i = 0; i < p->no_sig; i++ )
        {
            ABC_FREE(p->names[i]);
        }
    }
    ABC_FREE( p->names );
    ABC_FREE( p->values );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Dumps the target AIG into the BENCH file.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * ABC_GetNodeName( ABC_Manager mng, Abc_Obj_t * pNode )
{
    char * pName = NULL;
    if ( !stmm_lookup( mng->tNode2Name, (char *)pNode, (char **)&pName ) )
    {
        assert( 0 );
    }
    return pName;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

