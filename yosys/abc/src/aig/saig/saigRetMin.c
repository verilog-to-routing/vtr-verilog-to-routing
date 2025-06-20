/**CFile****************************************************************

  FileName    [saigRetMin.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Sequential AIG package.]

  Synopsis    [Min-area retiming for the AIG.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: saigRetMin.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "saig.h"

#include "opt/nwk/nwk.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satSolver.h"
#include "sat/bsat/satStore.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Derives the initial state after backward retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Saig_ManRetimeInitState( Aig_Man_t * p )
{
    int nConfLimit = 1000000;
    Vec_Int_t * vCiIds, * vInit = NULL;
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    Aig_Obj_t * pObj;
    int i, RetValue, * pModel;
    // solve the SAT problem
    pCnf = Cnf_DeriveSimpleForRetiming( p );
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    if ( pSat == NULL )
    {
        Cnf_DataFree( pCnf );
        return NULL;
    }
    RetValue = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    assert( RetValue != l_Undef );
    // create counter-example
    if ( RetValue == l_True )
    {
        // accumulate SAT variables of the CIs
        vCiIds = Vec_IntAlloc( Aig_ManCiNum(p) );
        Aig_ManForEachCi( p, pObj, i )
            Vec_IntPush( vCiIds, pCnf->pVarNums[pObj->Id] );
        // create the model
        pModel = Sat_SolverGetModel( pSat, vCiIds->pArray, vCiIds->nSize );
        vInit = Vec_IntAllocArray( pModel, Aig_ManCiNum(p) );
        Vec_IntFree( vCiIds );
    }
    sat_solver_delete( pSat );
    Cnf_DataFree( pCnf );
    return vInit;
}

/**Function*************************************************************

  Synopsis    [Uses UNSAT core to find the part of AIG to be excluded.]

  Description [Returns the number of the PO that appears in the UNSAT core.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManRetimeUnsatCore( Aig_Man_t * p, int fVerbose )
{
    int fVeryVerbose = 0;
    int nConfLimit = 1000000;
    void * pSatCnf = NULL; 
    Intp_Man_t * pManProof;
    Vec_Int_t * vCore = NULL;
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    Aig_Obj_t * pObj;
    int * pClause1, * pClause2, * pLit, * pVars;
    int i, RetValue, iBadPo, iClause, nVars, nPos;
    // create the SAT solver
    pCnf = Cnf_DeriveSimpleForRetiming( p );
    pSat = sat_solver_new();
    sat_solver_store_alloc( pSat ); 
    sat_solver_setnvars( pSat, pCnf->nVars );
    for ( i = 0; i < pCnf->nClauses; i++ )
    {
        if ( !sat_solver_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1] ) )
        {
            Cnf_DataFree( pCnf );
            sat_solver_delete( pSat );
            return -1;
        }
    }
    sat_solver_store_mark_roots( pSat ); 
    // solve the problem
    RetValue = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)nConfLimit, (ABC_INT64_T)0, (ABC_INT64_T)0, (ABC_INT64_T)0 );
    assert( RetValue != l_Undef );
    assert( RetValue == l_False );
    pSatCnf = sat_solver_store_release( pSat ); 
    sat_solver_delete( pSat );
    // derive the UNSAT core
    pManProof = Intp_ManAlloc();
    vCore = (Vec_Int_t *)Intp_ManUnsatCore( pManProof, (Sto_Man_t *)pSatCnf, 0, fVeryVerbose );
    Intp_ManFree( pManProof );
    Sto_ManFree( (Sto_Man_t *)pSatCnf );
    // derive the set of variables on which the core depends
    // collect the variable numbers
    nVars = 0;
    pVars = ABC_ALLOC( int, pCnf->nVars );
    memset( pVars, 0, sizeof(int) * pCnf->nVars );
    Vec_IntForEachEntry( vCore, iClause, i )
    {
        pClause1 = pCnf->pClauses[iClause];
        pClause2 = pCnf->pClauses[iClause+1];
        for ( pLit = pClause1; pLit < pClause2; pLit++ )
        {
            if ( pVars[ (*pLit) >> 1 ] == 0 )
                nVars++;
            pVars[ (*pLit) >> 1 ] = 1;
            if ( fVeryVerbose )
            printf( "%s%d ", ((*pLit) & 1)? "-" : "+", (*pLit) >> 1 );
        }
        if ( fVeryVerbose )
        printf( "\n" );
    }
    // collect the nodes
    if ( fVeryVerbose ) {
      Aig_ManForEachObj( p, pObj, i )
          if ( pCnf->pVarNums[pObj->Id] >= 0 && pVars[ pCnf->pVarNums[pObj->Id] ] == 1 )
          {
              Aig_ObjPrint( p, pObj );
              printf( "\n" );
          }
    }
    // pick the first PO in the list
    nPos = 0;
    iBadPo = -1;
    Aig_ManForEachCo( p, pObj, i )
        if ( pCnf->pVarNums[pObj->Id] >= 0 && pVars[ pCnf->pVarNums[pObj->Id] ] == 1 )
        {
            if ( iBadPo == -1 )
                iBadPo = i;
            nPos++;
        }
    if ( fVerbose )
        printf( "UNSAT core: %d clauses, %d variables, %d POs.  ", Vec_IntSize(vCore), nVars, nPos );
    ABC_FREE( pVars );
    Vec_IntFree( vCore );
    Cnf_DataFree( pCnf );
    return iBadPo;
}

/**Function*************************************************************

  Synopsis    [Marks the TFI cone with the current traversal ID.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManMarkCone_rec( Aig_Man_t * p, Aig_Obj_t * pObj )
{
    if ( pObj == NULL )
        return;
    if ( Aig_ObjIsTravIdCurrent( p, pObj ) )
        return;
    Aig_ObjSetTravIdCurrent( p, pObj );
    Saig_ManMarkCone_rec( p, Aig_ObjFanin0(pObj) );
    Saig_ManMarkCone_rec( p, Aig_ObjFanin1(pObj) );
}

/**Function*************************************************************

  Synopsis    [Counts the number of nodes to get registers after retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManRetimeCountCut( Aig_Man_t * p, Vec_Ptr_t * vCut )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj, * pFanin;
    int i, RetValue;
    // mark the cones
    Aig_ManIncrementTravId( p );
    Vec_PtrForEachEntry( Aig_Obj_t *, vCut, pObj, i )
        Saig_ManMarkCone_rec( p, pObj );
    // collect the new cut
    vNodes = Vec_PtrAlloc( 1000 );
    Aig_ManForEachObj( p, pObj, i )
    {
        if ( Aig_ObjIsTravIdCurrent(p, pObj) )
            continue;
        pFanin = Aig_ObjFanin0( pObj );
        if ( pFanin && !pFanin->fMarkA && Aig_ObjIsTravIdCurrent(p, pFanin) )
        {
            Vec_PtrPush( vNodes, pFanin );
            pFanin->fMarkA = 1;
        }
        pFanin = Aig_ObjFanin1( pObj );
        if ( pFanin && !pFanin->fMarkA && Aig_ObjIsTravIdCurrent(p, pFanin) )
        {
            Vec_PtrPush( vNodes, pFanin );
            pFanin->fMarkA = 1;
        }
    }
    Vec_PtrForEachEntry( Aig_Obj_t *, vNodes, pObj, i )
        pObj->fMarkA = 0;
    RetValue = Vec_PtrSize( vNodes );
    Vec_PtrFree( vNodes );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG recursively.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManRetimeDup_rec( Aig_Man_t * pNew, Aig_Obj_t * pObj )
{
    if ( pObj->pData )
        return;
    assert( Aig_ObjIsNode(pObj) );
    Saig_ManRetimeDup_rec( pNew, Aig_ObjFanin0(pObj) );
    Saig_ManRetimeDup_rec( pNew, Aig_ObjFanin1(pObj) );
    pObj->pData = Aig_And( pNew, Aig_ObjChild0Copy(pObj), Aig_ObjChild1Copy(pObj) );
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG while retiming the registers to the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManRetimeDupForward( Aig_Man_t * p, Vec_Ptr_t * vCut )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i;
    // mark the cones under the cut
//    assert( Vec_PtrSize(vCut) == Saig_ManRetimeCountCut(p, vCut) );
    // create the new manager
    pNew = Aig_ManStart( Aig_ManObjNumMax(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->nRegs = Vec_PtrSize(vCut);
    pNew->nTruePis = p->nTruePis;
    pNew->nTruePos = p->nTruePos;
    // create the true PIs
    Aig_ManCleanData( p );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    Saig_ManForEachPi( p, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    // create the registers
    Vec_PtrForEachEntry( Aig_Obj_t *, vCut, pObj, i )
        pObj->pData = Aig_NotCond( Aig_ObjCreateCi(pNew), pObj->fPhase );
    // duplicate logic above the cut
    Aig_ManForEachCo( p, pObj, i )
        Saig_ManRetimeDup_rec( pNew, Aig_ObjFanin0(pObj) );
    // create the true POs
    Saig_ManForEachPo( p, pObj, i )
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    // remember value in LI
    Saig_ManForEachLi( p, pObj, i )
        pObj->pData = Aig_ObjChild0Copy(pObj);
    // transfer values from the LIs to the LOs
    Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
        pObjLo->pData = pObjLi->pData;
    // erase the data values on the internal nodes of the cut
    Vec_PtrForEachEntry( Aig_Obj_t *, vCut, pObj, i )
        if ( Aig_ObjIsNode(pObj) )
            pObj->pData = NULL;
    // duplicate logic below the cut
    Vec_PtrForEachEntry( Aig_Obj_t *, vCut, pObj, i )
    {
        Saig_ManRetimeDup_rec( pNew, pObj );
        Aig_ObjCreateCo( pNew, Aig_NotCond((Aig_Obj_t *)pObj->pData, pObj->fPhase) );
    }
    Aig_ManCleanup( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Duplicates the AIG while retiming the registers to the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManRetimeDupBackward( Aig_Man_t * p, Vec_Ptr_t * vCut, Vec_Int_t * vInit )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj, * pObjLi, * pObjLo;
    int i;
    // mark the cones under the cut
//    assert( Vec_PtrSize(vCut) == Saig_ManRetimeCountCut(p, vCut) );
    // create the new manager
    pNew = Aig_ManStart( Aig_ManObjNumMax(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    pNew->nRegs = Vec_PtrSize(vCut);
    pNew->nTruePis = p->nTruePis;
    pNew->nTruePos = p->nTruePos;
    // create the true PIs
    Aig_ManCleanData( p );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    Saig_ManForEachPi( p, pObj, i )
        pObj->pData = Aig_ObjCreateCi( pNew );
    // create the registers
    Vec_PtrForEachEntry( Aig_Obj_t *, vCut, pObj, i )
        pObj->pData = Aig_NotCond( Aig_ObjCreateCi(pNew), vInit?Vec_IntEntry(vInit,i):0 );
    // duplicate logic above the cut and remember values
    Saig_ManForEachLi( p, pObj, i )
    {
        Saig_ManRetimeDup_rec( pNew, Aig_ObjFanin0(pObj) );
        pObj->pData = Aig_ObjChild0Copy(pObj);
    }
    // transfer values from the LIs to the LOs
    Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
        pObjLo->pData = pObjLi->pData;
    // erase the data values on the internal nodes of the cut
    Vec_PtrForEachEntry( Aig_Obj_t *, vCut, pObj, i )
        if ( Aig_ObjIsNode(pObj) )
            pObj->pData = NULL;
    // replicate the data on the constant node and the PIs
    pObj = Aig_ManConst1(p);
    pObj->pData = Aig_ManConst1(pNew);
    Saig_ManForEachPi( p, pObj, i )
        pObj->pData = Aig_ManCi( pNew, i );
    // duplicate logic below the cut
    Saig_ManForEachPo( p, pObj, i )
    {
        Saig_ManRetimeDup_rec( pNew, Aig_ObjFanin0(pObj) );
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    }
    Vec_PtrForEachEntry( Aig_Obj_t *, vCut, pObj, i )
    {
        Saig_ManRetimeDup_rec( pNew, pObj );
        Aig_ObjCreateCo( pNew, Aig_NotCond((Aig_Obj_t *)pObj->pData, vInit?Vec_IntEntry(vInit,i):0) );
    }
    Aig_ManCleanup( pNew );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Derives AIG for the initial state computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManRetimeDupInitState( Aig_Man_t * p, Vec_Ptr_t * vCut )
{
    Aig_Man_t * pNew;
    Aig_Obj_t * pObj;
    int i;
    // mark the cones under the cut
//    assert( Vec_PtrSize(vCut) == Saig_ManRetimeCountCut(p, vCut) );
    // create the new manager
    pNew = Aig_ManStart( Aig_ManObjNumMax(p) );
    // create the true PIs
    Aig_ManCleanData( p );
    Aig_ManConst1(p)->pData = Aig_ManConst1(pNew);
    // create the registers
    Vec_PtrForEachEntry( Aig_Obj_t *, vCut, pObj, i )
        pObj->pData = Aig_ObjCreateCi(pNew);
    // duplicate logic above the cut and create POs
    Saig_ManForEachLi( p, pObj, i )
    {
        Saig_ManRetimeDup_rec( pNew, Aig_ObjFanin0(pObj) );
        Aig_ObjCreateCo( pNew, Aig_ObjChild0Copy(pObj) );
    }
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Returns the array of bad registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Ptr_t * Saig_ManGetRegistersToExclude( Aig_Man_t * p )
{
    Vec_Ptr_t * vNodes;
    Aig_Obj_t * pObj, * pFanin;
    int i, Diffs;
    assert( Saig_ManRegNum(p) > 0 );
    Saig_ManForEachLi( p, pObj, i )
    {
        pFanin = Aig_ObjFanin0(pObj);
        if ( !Aig_ObjFaninC0(pObj) )
            pFanin->fMarkA = 1;
        else
            pFanin->fMarkB = 1;
    }
    Diffs = 0;
    Saig_ManForEachLi( p, pObj, i )
    {
        pFanin = Aig_ObjFanin0(pObj);
        Diffs += pFanin->fMarkA && pFanin->fMarkB;
    }
    vNodes = Vec_PtrAlloc( 100 );
    if ( Diffs > 0 )
    {
        Saig_ManForEachLi( p, pObj, i )
        {
            pFanin = Aig_ObjFanin0(pObj);
            if ( pFanin->fMarkA && pFanin->fMarkB )
                Vec_PtrPush( vNodes, pObj );
        }
        assert( Vec_PtrSize(vNodes) == Diffs );
    }
    Saig_ManForEachLi( p, pObj, i )
    {
        pFanin = Aig_ObjFanin0(pObj);
        pFanin->fMarkA = pFanin->fMarkB = 0;
    }
    return vNodes;
}

/**Function*************************************************************

  Synopsis    [Hides the registers that cannot be backward retimed.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Saig_ManHideBadRegs( Aig_Man_t * p, Vec_Ptr_t * vBadRegs )
{
    Vec_Ptr_t * vPisNew, * vPosNew;
    Aig_Obj_t * pObjLi, * pObjLo;
    int nTruePi, nTruePo, nBadRegs, i;
    if ( Vec_PtrSize(vBadRegs) == 0 )
        return 0;
    // attached LOs to LIs
    Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
        pObjLi->pData = pObjLo;
    // reorder them by putting bad registers first
    vPisNew = Vec_PtrDup( p->vCis );
    vPosNew = Vec_PtrDup( p->vCos );
    nTruePi = Aig_ManCiNum(p) - Aig_ManRegNum(p);
    nTruePo = Aig_ManCoNum(p) - Aig_ManRegNum(p);
    assert( nTruePi == p->nTruePis );
    assert( nTruePo == p->nTruePos );
    Vec_PtrForEachEntry( Aig_Obj_t *, vBadRegs, pObjLi, i )
    {
        Vec_PtrWriteEntry( vPisNew, nTruePi++, pObjLi->pData );
        Vec_PtrWriteEntry( vPosNew, nTruePo++, pObjLi );
        pObjLi->fMarkA = 1;
    }
    Saig_ManForEachLiLo( p, pObjLi, pObjLo, i )
    {
        if ( pObjLi->fMarkA )
        {
            pObjLi->fMarkA = 0;
            continue;
        }
        Vec_PtrWriteEntry( vPisNew, nTruePi++, pObjLo );
        Vec_PtrWriteEntry( vPosNew, nTruePo++, pObjLi );
    }
    // check the sizes
    assert( nTruePi == Aig_ManCiNum(p) );
    assert( nTruePo == Aig_ManCoNum(p) );
    // transfer the arrays
    Vec_PtrFree( p->vCis ); p->vCis = vPisNew;
    Vec_PtrFree( p->vCos ); p->vCos = vPosNew;
    // update the PIs
    nBadRegs = Vec_PtrSize(vBadRegs);
    p->nRegs -= nBadRegs;
    p->nTruePis += nBadRegs;
    p->nTruePos += nBadRegs;
    return nBadRegs;
}

/**Function*************************************************************

  Synopsis    [Exposes bad registers.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Saig_ManExposeBadRegs( Aig_Man_t * p, int nBadRegs )
{
    p->nRegs += nBadRegs;
    p->nTruePis -= nBadRegs;
    p->nTruePos -= nBadRegs;
}

/**Function*************************************************************

  Synopsis    [Performs min-area retiming backward with initial state.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManRetimeMinAreaBackward( Aig_Man_t * pNew, int fVerbose )
{
    Aig_Man_t * pInit, * pFinal;
    Vec_Ptr_t * vBadRegs, * vCut;
    Vec_Int_t * vInit;
    int iBadReg;
    // transform the AIG to have no bad registers
    vBadRegs = Saig_ManGetRegistersToExclude( pNew );
    if ( fVerbose && Vec_PtrSize(vBadRegs) )
        printf( "Excluding %d registers that cannot be backward retimed.\n", Vec_PtrSize(vBadRegs) ); 
    while ( 1 )
    {
        Saig_ManHideBadRegs( pNew, vBadRegs );
        Vec_PtrFree( vBadRegs );
        // compute cut
        vCut = Nwk_ManDeriveRetimingCut( pNew, 0, fVerbose );
        if ( Vec_PtrSize(vCut) >= Aig_ManRegNum(pNew) )
        {
            Vec_PtrFree( vCut );
            return NULL;
        }
        // derive the initial state
        pInit = Saig_ManRetimeDupInitState( pNew, vCut );
        vInit = Saig_ManRetimeInitState( pInit );
        if ( vInit != NULL )
        {
            pFinal = Saig_ManRetimeDupBackward( pNew, vCut, vInit );
            Vec_IntFree( vInit );
            Vec_PtrFree( vCut );
            Aig_ManStop( pInit );
            return pFinal;
        }
        Vec_PtrFree( vCut );
        // there is no initial state - find the offending output
        iBadReg = Saig_ManRetimeUnsatCore( pInit, fVerbose );
        Aig_ManStop( pInit );
        if ( fVerbose )
            printf( "Excluding register %d.\n", iBadReg ); 
        // prepare to remove this output
        vBadRegs = Vec_PtrAlloc( 1 );
        Vec_PtrPush( vBadRegs, Aig_ManCo( pNew, Saig_ManPoNum(pNew) + iBadReg ) );
    }
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Performs min-area retiming.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Aig_Man_t * Saig_ManRetimeMinArea( Aig_Man_t * p, int nMaxIters, int fForwardOnly, int fBackwardOnly, int fInitial, int fVerbose )
{
    Vec_Ptr_t * vCut;
    Aig_Man_t * pNew, * pTemp, * pCopy;
    int i, fChanges;
    pNew = Aig_ManDupSimple( p );
    // perform several iterations of forward retiming
    fChanges = 0;
    if ( !fBackwardOnly )
    for ( i = 0; i < nMaxIters; i++ )
    {
        if ( Saig_ManRegNum(pNew) == 0 )
            break;
        vCut = Nwk_ManDeriveRetimingCut( pNew, 1, fVerbose );
        if ( Vec_PtrSize(vCut) >= Aig_ManRegNum(pNew) )
        {
            if ( fVerbose && !fChanges )
                printf( "Forward retiming cannot reduce registers.\n" );
            Vec_PtrFree( vCut );
            break;
        }
        pNew = Saig_ManRetimeDupForward( pTemp = pNew, vCut );
        Aig_ManStop( pTemp );
        Vec_PtrFree( vCut );
        if ( fVerbose )
            Aig_ManReportImprovement( p, pNew );
        fChanges = 1;
    }
    // perform several iterations of backward retiming
    fChanges = 0;
    if ( !fForwardOnly && !fInitial )
    for ( i = 0; i < nMaxIters; i++ )
    {
        if ( Saig_ManRegNum(pNew) == 0 )
            break;
        vCut = Nwk_ManDeriveRetimingCut( pNew, 0, fVerbose );
        if ( Vec_PtrSize(vCut) >= Aig_ManRegNum(pNew) )
        {
            if ( fVerbose && !fChanges )
                printf( "Backward retiming cannot reduce registers.\n" );
            Vec_PtrFree( vCut );
            break;
        }
        pNew = Saig_ManRetimeDupBackward( pTemp = pNew, vCut, NULL );
        Aig_ManStop( pTemp );
        Vec_PtrFree( vCut );
        if ( fVerbose )
            Aig_ManReportImprovement( p, pNew );
        fChanges = 1;
    }
    else if ( !fForwardOnly && fInitial )
    for ( i = 0; i < nMaxIters; i++ )
    {
        if ( Saig_ManRegNum(pNew) == 0 )
            break;
        pCopy = Aig_ManDupSimple( pNew );
        pTemp = Saig_ManRetimeMinAreaBackward( pCopy, fVerbose );
        Aig_ManStop( pCopy );
        if ( pTemp == NULL )
        {
            if ( fVerbose && !fChanges )
                printf( "Backward retiming cannot reduce registers.\n" );
            break;
        }
        Saig_ManExposeBadRegs( pTemp, Saig_ManPoNum(pTemp) - Saig_ManPoNum(pNew) );
        Aig_ManStop( pNew );
        pNew = pTemp;
        if ( fVerbose )
            Aig_ManReportImprovement( p, pNew );
        fChanges = 1;
    }
    if ( !fForwardOnly && !fInitial && fChanges )
        printf( "Assuming const-0 init-state after backward retiming. Result will not verify.\n" );
    return pNew;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

