/**CFile****************************************************************

  FileName    [aigPartSat.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [Partitioning for SAT solving.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aigPartSat.c,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#include "aig.h"
#include "sat/bsat/satSolver.h"
#include "sat/cnf/cnf.h"

ABC_NAMESPACE_IMPL_START


/* 

The node partitioners defined in this file return array of intergers 
mapping each AND node's ID into the 0-based number of its partition.
The mapping of PIs/POs will be derived automatically in Aig_ManPartSplit().

The partitions can be ordered in any way, but the recommended ordering
is to first include partitions whose nodes are closer to the outputs.

*/

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

extern Aig_Man_t * Dar_ManRwsat( Aig_Man_t * pAig, int fBalance, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [No partitioning.]

  Description [The partitioner ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Aig_ManPartitionMonolithic( Aig_Man_t * p )
{
    Vec_Int_t * vId2Part;
    vId2Part = Vec_IntStart( Aig_ManObjNumMax(p) );
    return vId2Part;
}

/**Function*************************************************************

  Synopsis    [Partitioning using levelized order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Aig_ManPartitionLevelized( Aig_Man_t * p, int nPartSize )
{
    Vec_Int_t * vId2Part;
    Vec_Vec_t * vNodes;
    Aig_Obj_t * pObj;
    int i, k, Counter = 0;
    vNodes = Aig_ManLevelize( p );
    vId2Part = Vec_IntStart( Aig_ManObjNumMax(p) );
    Vec_VecForEachEntryReverseReverse( Aig_Obj_t *, vNodes, pObj, i, k )
        Vec_IntWriteEntry( vId2Part, Aig_ObjId(pObj), Counter++/nPartSize );
    Vec_VecFree( vNodes );
    return vId2Part;
}

/**Function*************************************************************

  Synopsis    [Partitioning using DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Aig_ManPartitionDfs( Aig_Man_t * p, int nPartSize, int fPreorder )
{
    Vec_Int_t * vId2Part; 
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj;
    int i, Counter = 0;
    vId2Part = Vec_IntStart( Aig_ManObjNumMax(p) );
    if ( fPreorder )
    {
        vNodes = Aig_ManDfsPreorder( p, 1 );
        Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
            Vec_IntWriteEntry( vId2Part, Aig_ObjId(pObj), Counter++/nPartSize );
    }
    else
    {
        vNodes = Aig_ManDfs( p, 1 );
        Vec_PtrForEachEntryReverse( Aig_Obj_t *, vNodes, pObj, i )
            Vec_IntWriteEntry( vId2Part, Aig_ObjId(pObj), Counter++/nPartSize );
    }
    Vec_PtrFree( vNodes );
    return vId2Part;
}

/**Function*************************************************************

  Synopsis    [Recursively constructs the partition.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPartSplitOne_rec( Aig_Man_t * pNew, Aig_Man_t * p, Aig_Obj_t * pObj, Vec_Int_t * vPio2Id )
{
    if ( !Aig_ObjIsTravIdCurrent( p, pObj ) )
    { // new PI
        Aig_ObjSetTravIdCurrent( p, pObj );
/*
        if ( pObj->fMarkA ) // const0
            pObj->pData = Aig_ManConst0( pNew );
        else if ( pObj->fMarkB ) // const1
            pObj->pData = Aig_ManConst1( pNew );
        else
*/
        {
            pObj->pData = Aig_ObjCreateCi( pNew );
            if ( pObj->fMarkA ) // const0
                ((Aig_Obj_t *)pObj->pData)->fMarkA = 1;
            else if ( pObj->fMarkB ) // const1
                ((Aig_Obj_t *)pObj->pData)->fMarkB = 1;
            Vec_IntPush( vPio2Id, Aig_ObjId(pObj) );
        }
        return;
    }
    if ( pObj->pData ) 
        return;
    Aig_ManPartSplitOne_rec( pNew, p, Aig_ObjFanin0(pObj), vPio2Id );
    Aig_ManPartSplitOne_rec( pNew, p, Aig_ObjFanin1(pObj), vPio2Id );
    pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Carves out one partition of the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Aig_ManPartSplitOne( Aig_Man_t * p, Vec_Ptr_t * vNodes, Vec_Int_t ** pvPio2Id )
{
    Vec_Int_t * vPio2Id;
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    int i;
    // mark these nodes
    Aig_ManIncrementTravId( p );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
    {
        Aig_ObjSetTravIdCurrent( p, pObj );
        pObj->pData = NULL;
    }
    // add these nodes in a DFS order
    pNew = Aig_ManStart( Vec_PtrSize(vNodes) );
    vPio2Id = Vec_IntAlloc( 100 );
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        Aig_ManPartSplitOne_rec( pNew, p, pObj, vPio2Id );
    // add the POs
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        if ( Aig_ObjRefs((Aig_Obj_t *)pObj->pData) != Aig_ObjRefs(pObj) )
        {
            assert( Aig_ObjRefs((Aig_Obj_t *)pObj->pData) < Aig_ObjRefs(pObj) );
            Aig_ObjCreateCo( pNew, (Aig_Obj_t *)pObj->pData );
            Vec_IntPush( vPio2Id, Aig_ObjId(pObj) );
        }
    assert( Aig_ManNodeNum(pNew) == Vec_PtrSize(vNodes) );
    *pvPio2Id = vPio2Id;
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Derives AIGs for each partition.]

  Description [The first argument is a original AIG. The second argument
  is the array mapping each AND-node's ID into the 0-based number of its
  partition. The last argument is the array of arrays (one for each new AIG) 
  mapping the index of each terminal in the new AIG (the index of each 
  terminal is derived by ordering PIs followed by POs in their natural order) 
  into the ID of the corresponding node in the original AIG. The returned 
  value is the array of AIGs representing the partitions.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Aig_ManPartSplit( Aig_Man_t * p, Vec_Int_t * vNode2Part, Vec_Ptr_t ** pvPio2Id, Vec_Ptr_t ** pvPart2Pos )
{
    Vec_Vec_t * vGroups, * vPart2Pos;
    Vec_Ptr_t * vAigs, * vPio2Id, * vNodes;
    Vec_Int_t * vPio2IdOne;
    Aig_Man_t * pAig;
    Aig_Obj_t * pObj, * pDriver;
    int i, nodePart, nParts;
    vAigs = Vec_PtrAlloc( 100 );
    vPio2Id = Vec_PtrAlloc( 100 );
    // put all nodes into levels according to their partition
    nParts = Vec_IntFindMax(vNode2Part) + 1;
    assert( nParts > 0 );
    vGroups = Vec_VecAlloc( nParts );
    Vec_IntForEachEntry( vNode2Part, nodePart, i )
    {
        pObj = Aig_ManObj( p, i );
        if ( Aig_ObjIsNode(pObj) )
            Vec_VecPush( vGroups, nodePart, pObj );
    }

    // label PIs that should be restricted to some values
    Aig_ManForEachCo( p, pObj, i )
    {
        pDriver = Aig_ObjFanin0(pObj);
        if ( Aig_ObjIsCi(pDriver) )
        {
            if ( Aig_ObjFaninC0(pObj) )
                pDriver->fMarkA = 1; // const0 PI
            else
                pDriver->fMarkB = 1; // const1 PI
        }
    }

    // create partitions
    Vec_VecForEachLevel( vGroups, vNodes, i )
    {
        if ( Vec_PtrSize(vNodes) == 0 )
        {
            printf( "Aig_ManPartSplit(): Skipping partition # %d without nodes (warning).\n", i );
            continue;
        }
        pAig = Aig_ManPartSplitOne( p, vNodes, &vPio2IdOne );
        Vec_PtrPush( vPio2Id, vPio2IdOne );
        Vec_PtrPush( vAigs, pAig );
    }
    Vec_VecFree( vGroups );

    // divide POs according to their partitions
    vPart2Pos = Vec_VecStart( Vec_PtrSize(vAigs) );
    Aig_ManForEachCo( p, pObj, i )
    {
        pDriver = Aig_ObjFanin0(pObj);
        if ( Aig_ObjIsCi(pDriver) )
            pDriver->fMarkA = pDriver->fMarkB = 0;
        else
            Vec_VecPush( vPart2Pos, Vec_IntEntry(vNode2Part, Aig_ObjFaninId0(pObj)), pObj );
    }

    *pvPio2Id = vPio2Id;
    *pvPart2Pos = (Vec_Ptr_t *)vPart2Pos;
    return vAigs;
}

/**Function*************************************************************

  Synopsis    [Resets node polarity to unbias the polarity of CNF variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPartResetNodePolarity( Aig_Man_t * pPart )
{
    Aig_Obj_t * pObj;
    int i;
    Aig_ManForEachObj( pPart, pObj, i )
        pObj->fPhase = 0;
}

/**Function*************************************************************

  Synopsis    [Sets polarity according to the original nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManPartSetNodePolarity( Aig_Man_t * p, Aig_Man_t * pPart, Vec_Int_t * vPio2Id )
{
    Aig_Obj_t * pObj, * pObjInit;
    int i;
    Aig_ManConst1(pPart)->fPhase = 1;
    Aig_ManForEachCi( pPart, pObj, i )
    {
        pObjInit = Aig_ManObj( p, Vec_IntEntry(vPio2Id, i) );
        pObj->fPhase = pObjInit->fPhase;
    }
    Aig_ManForEachNode( pPart, pObj, i )
        pObj->fPhase = (Aig_ObjFanin0(pObj)->fPhase ^ Aig_ObjFaninC0(pObj)) & (Aig_ObjFanin1(pObj)->fPhase ^ Aig_ObjFaninC1(pObj));
    Aig_ManForEachCo( pPart, pObj, i )
    {
        pObjInit = Aig_ManObj( p, Vec_IntEntry(vPio2Id, Aig_ManCiNum(pPart) + i) );
        pObj->fPhase = (Aig_ObjFanin0(pObj)->fPhase ^ Aig_ObjFaninC0(pObj));
        assert( pObj->fPhase == pObjInit->fPhase );
    }
}

/**Function*************************************************************

  Synopsis    [Sets polarity according to the original nodes.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Aig_ManDeriveCounterExample( Aig_Man_t * p, Vec_Int_t * vNode2Var, sat_solver * pSat )
{ 
    Vec_Int_t * vPisIds;
    Aig_Obj_t * pObj;
    int i;
    // collect IDs of PI variables
    // (fanoutless PIs have SAT var 0, which is an unused in the SAT solver -> has value 0)
    vPisIds = Vec_IntAlloc( Aig_ManCiNum(p) );
    Aig_ManForEachCi( p, pObj, i )
        Vec_IntPush( vPisIds, Vec_IntEntry(vNode2Var, Aig_ObjId(pObj)) );
    // derive the SAT assignment
    p->pData = Sat_SolverGetModel( pSat, vPisIds->pArray, vPisIds->nSize );
    Vec_IntFree( vPisIds );
}

/**Function*************************************************************

  Synopsis    [Derives CNF for the partition (pAig) and adds it to solver.]

  Description [Array vPio2Id contains mapping of the PI/PO terminal of pAig
  into the IDs of the corresponding original nodes. Array vNode2Var contains
  mapping of the original nodes into their SAT variable numbers.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManAddNewCnfToSolver( sat_solver * pSat, Aig_Man_t * pAig, Vec_Int_t * vNode2Var, 
                             Vec_Int_t * vPioIds, Vec_Ptr_t * vPartPos, int fAlignPol )
{
    Cnf_Dat_t * pCnf;
    Aig_Obj_t * pObj;
    int * pBeg, * pEnd;
    int i, Lits[2], iSatVarOld, iNodeIdOld;
    // derive CNF and express it using new SAT variables
    pCnf = Cnf_Derive( pAig, Aig_ManCoNum(pAig) );
    Cnf_DataTranformPolarity( pCnf, 1 );
    Cnf_DataLift( pCnf, sat_solver_nvars(pSat) );
    // create new variables in the SAT solver
    sat_solver_setnvars( pSat, sat_solver_nvars(pSat) + pCnf->nVars );
    // add clauses for this CNF
    Cnf_CnfForClause( pCnf, pBeg, pEnd, i )
        if ( !sat_solver_addclause( pSat, pBeg, pEnd ) )
        {
            assert( 0 ); // if it happens, can return 1 (unsatisfiable)
            return 1;
        }
    // derive the connector clauses
    Aig_ManForEachCi( pAig, pObj, i )
    {
        iNodeIdOld = Vec_IntEntry( vPioIds, i );
        iSatVarOld = Vec_IntEntry( vNode2Var, iNodeIdOld );
        if ( iSatVarOld == 0 ) // iNodeIdOld in the original AIG has no SAT var
        { 
            // map the corresponding original AIG node into this SAT var
            Vec_IntWriteEntry( vNode2Var, iNodeIdOld, pCnf->pVarNums[Aig_ObjId(pObj)] );
            continue;
        }
        // add connector clauses 
        Lits[0] = toLitCond( iSatVarOld, 0 );
        Lits[1] = toLitCond( pCnf->pVarNums[Aig_ObjId(pObj)], 1 );
        if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            assert( 0 );
        Lits[0] = toLitCond( iSatVarOld, 1 );
        Lits[1] = toLitCond( pCnf->pVarNums[Aig_ObjId(pObj)], 0 );
        if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            assert( 0 );
    }
    // derive the connector clauses
    Aig_ManForEachCo( pAig, pObj, i )
    {
        iNodeIdOld = Vec_IntEntry( vPioIds, Aig_ManCiNum(pAig) + i );
        iSatVarOld = Vec_IntEntry( vNode2Var, iNodeIdOld );
        if ( iSatVarOld == 0 ) // iNodeIdOld in the original AIG has no SAT var
        { 
            // map the corresponding original AIG node into this SAT var
            Vec_IntWriteEntry( vNode2Var, iNodeIdOld, pCnf->pVarNums[Aig_ObjId(pObj)] );
            continue;
        }
        // add connector clauses 
        Lits[0] = toLitCond( iSatVarOld, 0 );
        Lits[1] = toLitCond( pCnf->pVarNums[Aig_ObjId(pObj)], 1 );
        if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            assert( 0 );
        Lits[0] = toLitCond( iSatVarOld, 1 );
        Lits[1] = toLitCond( pCnf->pVarNums[Aig_ObjId(pObj)], 0 );
        if ( !sat_solver_addclause( pSat, Lits, Lits+2 ) )
            assert( 0 );
    }
    // transfer the ID of constant 1 node
    if ( Vec_IntEntry( vNode2Var, 0 ) == 0 )
        Vec_IntWriteEntry( vNode2Var, 0, pCnf->pVarNums[0] );
    // remove the CNF
    Cnf_DataFree( pCnf );
    // constrain the solver with the literals corresponding to the original POs
    Vec_PtrForEachEntry( Aig_Obj_t *, vPartPos, pObj, i )
    {
        iNodeIdOld = Aig_ObjFaninId0( pObj );
        iSatVarOld = Vec_IntEntry( vNode2Var, iNodeIdOld );
        assert( iSatVarOld != 0 );
        // assert the original PO to be 1
        Lits[0] = toLitCond( iSatVarOld, Aig_ObjFaninC0(pObj) );
        // correct the polarity if polarity alignment is enabled
        if ( fAlignPol && Aig_ObjFanin0(pObj)->fPhase ) 
            Lits[0] = lit_neg( Lits[0] );
        if ( !sat_solver_addclause( pSat, Lits, Lits+1 ) )
        {
            assert( 0 ); // if it happens, can return 1 (unsatisfiable)
            return 1;
        }
    }
    // constrain some the primary inputs to constant values
    Aig_ManForEachCi( pAig, pObj, i )
    {
        if ( !pObj->fMarkA && !pObj->fMarkB )
            continue;
        iNodeIdOld = Vec_IntEntry( vPioIds, i );
        iSatVarOld = Vec_IntEntry( vNode2Var, iNodeIdOld );
        Lits[0] = toLitCond( iSatVarOld, pObj->fMarkA );
        if ( !sat_solver_addclause( pSat, Lits, Lits+1 ) )
        {
            assert( 0 ); // if it happens, can return 1 (unsatisfiable)
            return 1;
        }
        pObj->fMarkA = pObj->fMarkB = 0;
    }
    return 0;
}

/**Function*************************************************************

  Synopsis    [Performs partitioned SAT solving.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Aig_ManPartitionedSat( Aig_Man_t * p, int nAlgo, int nPartSize, 
    int nConfPart, int nConfTotal, int fAlignPol, int fSynthesize, int fVerbose )
{
    sat_solver * pSat;
    Vec_Ptr_t * vAigs;
    Vec_Vec_t * vPio2Id, * vPart2Pos;
    Aig_Man_t * pAig, * pTemp;
    Vec_Int_t * vNode2Part, * vNode2Var;
    int nConfRemaining = nConfTotal, nNodes = 0;
    int i, status, RetValue = -1;
    abctime clk;

    // perform partitioning according to the selected algorithm
    clk = Abc_Clock();
    switch ( nAlgo )
    {
    case 0: 
        vNode2Part = Aig_ManPartitionMonolithic( p );
        break;
    case 1: 
        vNode2Part = Aig_ManPartitionLevelized( p, nPartSize );
        break;
    case 2: 
        vNode2Part = Aig_ManPartitionDfs( p, nPartSize, 0 );
        break;
    case 3: 
        vNode2Part = Aig_ManPartitionDfs( p, nPartSize, 1 );
        break;
    default:
        printf( "Unknown partitioning algorithm.\n" );
        return -1;
    }

    if ( fVerbose )
    {
    printf( "Partitioning derived %d partitions. ", Vec_IntFindMax(vNode2Part) + 1 );
    ABC_PRT( "Time", Abc_Clock() - clk );
    }

    // split the original AIG into partition AIGs (vAigs)
    // also, derives mapping of PIs/POs of partition AIGs into original nodes
    // also, derives mapping of POs of the original AIG into partitions
    vAigs = Aig_ManPartSplit( p, vNode2Part, (Vec_Ptr_t **)&vPio2Id, (Vec_Ptr_t **)&vPart2Pos );
    Vec_IntFree( vNode2Part );

    if ( fVerbose )
    {
    printf( "Partions were transformed into AIGs. " );
    ABC_PRT( "Time", Abc_Clock() - clk );
    }

    // synthesize partitions
    if ( fSynthesize )
    Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pAig, i )
    {
        pAig = Dar_ManRwsat( pTemp = pAig, 0, 0 );
        Vec_PtrWriteEntry( vAigs, i, pAig );
        Aig_ManStop( pTemp );
    }

    // start the SAT solver
    pSat = sat_solver_new();
//    pSat->verbosity = fVerbose;
    // start mapping of the original AIG IDs into their SAT variable numbers
    vNode2Var = Vec_IntStart( Aig_ManObjNumMax(p) );

    // add partitions, one at a time, and run the SAT solver 
    Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pAig, i )
    {
clk = Abc_Clock();
        // transform polarity of the AIG
        if ( fAlignPol )
            Aig_ManPartSetNodePolarity( p, pAig, Vec_VecEntryInt(vPio2Id,i) );
        else
            Aig_ManPartResetNodePolarity( pAig );
        // add CNF of this partition to the SAT solver
        if ( Aig_ManAddNewCnfToSolver( pSat, pAig, vNode2Var, 
            Vec_VecEntryInt(vPio2Id,i), Vec_VecEntry(vPart2Pos,i), fAlignPol ) )
        {
            RetValue = 1;
            break;
        }
        // call the SAT solver
        status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)nConfRemaining, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
        if ( fVerbose )
        {
            printf( "%4d : Aig = %6d. Vs = %7d. RootCs = %7d. LearnCs = %6d. ",
                i, nNodes += Aig_ManNodeNum(pAig), sat_solver_nvars(pSat), 
                (int)pSat->stats.clauses, (int)pSat->stats.learnts );
ABC_PRT( "Time", Abc_Clock() - clk );
        }
        // analize the result
        if ( status == l_False )
        {
            RetValue = 1;
            break;
        }
        else if ( status == l_True )
            RetValue = 0;
        else
            RetValue = -1;
        nConfRemaining -= pSat->stats.conflicts;
        if ( nConfRemaining <= 0 )
        {
            printf( "Exceeded the limit on the total number of conflicts (%d).\n", nConfTotal );
            break;
        }
    }
    if ( RetValue == 0 )
        Aig_ManDeriveCounterExample( p, vNode2Var, pSat );
    // cleanup
    sat_solver_delete( pSat );
    Vec_PtrForEachEntry( Aig_Man_t *, vAigs, pTemp, i )
        Aig_ManStop( pTemp );
    Vec_PtrFree( vAigs );
    Vec_VecFree( vPio2Id );
    Vec_VecFree( vPart2Pos );
    Vec_IntFree( vNode2Var );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

