/**CFile****************************************************************

  FileName    [fretInit.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Flow-based retiming package.]

  Synopsis    [Initialization for retiming package.]

  Author      [Aaron Hurst]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2008.]

  Revision    [$Id: fretInit.c,v 1.00 2008/01/01 00:00:00 ahurst Exp $]

***********************************************************************/

#include "fretime.h"

#include "base/io/ioAbc.h"
#include "map/mio/mio.h"
#include "aig/hop/hop.h"

#ifdef ABC_USE_CUDD
#include "bdd/cudd/cuddInt.h"
#endif

ABC_NAMESPACE_IMPL_START


#undef DEBUG_PRINT_INIT_NTK


////////////////////////////////////////////////////////////////////////
///                     FUNCTION PROTOTYPES                          ///
////////////////////////////////////////////////////////////////////////

static void Abc_FlowRetime_UpdateForwardInit_rec( Abc_Obj_t * pObj );
static void Abc_FlowRetime_VerifyBackwardInit( Abc_Ntk_t * pNtk );
static void Abc_FlowRetime_VerifyBackwardInit_rec( Abc_Obj_t * pObj );
static Abc_Obj_t* Abc_FlowRetime_UpdateBackwardInit_rec( Abc_Obj_t *pOrigObj );

static void Abc_FlowRetime_SimulateNode( Abc_Obj_t * pObj );
static void Abc_FlowRetime_SimulateSop( Abc_Obj_t * pObj, char *pSop );

static void Abc_FlowRetime_SetInitToOrig( Abc_Obj_t *pInit, Abc_Obj_t *pOrig );
static void Abc_FlowRetime_GetInitToOrig( Abc_Obj_t *pInit, Abc_Obj_t **pOrig, int *lag );
static void Abc_FlowRetime_ClearInitToOrig( Abc_Obj_t *pInit );

extern void * Abc_FrameReadLibGen();

extern void Abc_NtkMarkCone_rec( Abc_Obj_t * pObj, int fForward );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Updates initial state information.]

  Description [Assumes latch boxes in original position, latches in
               new positions.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void
Abc_FlowRetime_InitState( Abc_Ntk_t * pNtk ) {

  if (!pManMR->fComputeInitState) return;

  if (pManMR->fIsForward)
    Abc_FlowRetime_UpdateForwardInit( pNtk );
  else {
    Abc_FlowRetime_UpdateBackwardInit( pNtk );
  }
}

/**Function*************************************************************

  Synopsis    [Prints initial state information.]

  Description [Prints distribution of 0,1,and X initial states.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline int
Abc_FlowRetime_ObjFirstNonLatchBox( Abc_Obj_t * pOrigObj, Abc_Obj_t ** pResult ) {
  int lag = 0;
  Abc_Ntk_t *pNtk;
  *pResult = pOrigObj;
  pNtk = Abc_ObjNtk( pOrigObj );

  Abc_NtkIncrementTravId( pNtk );

  while( Abc_ObjIsBo(*pResult) || Abc_ObjIsLatch(*pResult) || Abc_ObjIsBi(*pResult) ) {
    assert(Abc_ObjFaninNum(*pResult));
    *pResult = Abc_ObjFanin0(*pResult);
    
    if (Abc_NodeIsTravIdCurrent(*pResult))
      return -1;
    Abc_NodeSetTravIdCurrent(*pResult);

    if (Abc_ObjIsLatch(*pResult)) ++lag;
  }

  return lag;
}


/**Function*************************************************************

  Synopsis    [Prints initial state information.]

  Description [Prints distribution of 0,1,and X initial states.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void
Abc_FlowRetime_PrintInitStateInfo( Abc_Ntk_t * pNtk ) {
  int i, n0=0, n1=0, nDC=0, nOther=0;
  Abc_Obj_t *pLatch;

  Abc_NtkForEachLatch( pNtk, pLatch, i ) {
    if (Abc_LatchIsInit0(pLatch)) n0++;
    else if (Abc_LatchIsInit1(pLatch)) n1++;
    else if (Abc_LatchIsInitDc(pLatch)) nDC++;
    else nOther++;
  }     

  printf("\tinitial states {0,1,x} = {%d, %d, %d}", n0, n1, nDC);
  if (nOther)
    printf(" + %d UNKNOWN", nOther);
  printf("\n");
}


/**Function*************************************************************

  Synopsis    [Computes initial state after forward retiming.]

  Description [Assumes box outputs in old positions stored w/ init values.
               Uses three-value simulation to preserve don't cares.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_UpdateForwardInit( Abc_Ntk_t * pNtk ) {
  Abc_Obj_t *pObj, *pFanin;
  int i;

  vprintf("\t\tupdating init state\n");

  Abc_NtkIncrementTravId( pNtk );

  Abc_NtkForEachLatch( pNtk, pObj, i ) {
    pFanin = Abc_ObjFanin0(pObj);
    Abc_FlowRetime_UpdateForwardInit_rec( pFanin );

    if (FTEST(pFanin, INIT_0))
      Abc_LatchSetInit0( pObj );
    else if (FTEST(pFanin, INIT_1))
      Abc_LatchSetInit1( pObj );
    else
      Abc_LatchSetInitDc( pObj );
  }
}

void Abc_FlowRetime_UpdateForwardInit_rec( Abc_Obj_t * pObj ) {
  Abc_Obj_t *pNext;
  int i;

  assert(!Abc_ObjIsPi(pObj)); // should never reach the inputs

  if (Abc_ObjIsBo(pObj)) return;

  // visited?
  if (Abc_NodeIsTravIdCurrent(pObj)) return;
  Abc_NodeSetTravIdCurrent(pObj);

  Abc_ObjForEachFanin( pObj, pNext, i ) {
    Abc_FlowRetime_UpdateForwardInit_rec( pNext );
  }
  
  Abc_FlowRetime_SimulateNode( pObj );
}

/**Function*************************************************************

  Synopsis    [Recursively evaluates HOP netlist.]

  Description [Exponential.  There aren't enough flags on a HOP node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Abc_FlowRetime_EvalHop_rec( Hop_Man_t *pHop, Hop_Obj_t *pObj, int *f, int *dc ) {
  int f1, dc1, f2, dc2;
  Hop_Obj_t *pReg = Hop_Regular(pObj);
  
  // const 0
  if (Hop_ObjIsConst1(pReg)) {
    *f  = 1;
    *f ^= (pReg == pObj ? 1 : 0);
    *dc = 0;
    return;
  }

  // PI
  if (Hop_ObjIsPi(pReg)) {
    *f  = pReg->fMarkA;
    *f ^= (pReg == pObj ? 1 : 0);
    *dc = pReg->fMarkB;
    return;
  }

  // PO
  if (Hop_ObjIsPo(pReg)) {
    assert( pReg == pObj );
    Abc_FlowRetime_EvalHop_rec(pHop, Hop_ObjChild0(pReg), f, dc);
    return;
  }

  // AND
  if (Hop_ObjIsAnd(pReg)) {
    Abc_FlowRetime_EvalHop_rec(pHop, Hop_ObjChild0(pReg), &f1, &dc1);
    Abc_FlowRetime_EvalHop_rec(pHop, Hop_ObjChild1(pReg), &f2, &dc2);

    *dc = (dc1 & f2) | (dc2 & f1) | (dc1 & dc2);
    *f  = f1 & f2;
    *f ^= (pReg == pObj ? 1 : 0);
    return;
  }

  assert(0);
}


/**Function*************************************************************

  Synopsis    [Sets initial value flags.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline void Abc_FlowRetime_SetInitValue( Abc_Obj_t * pObj,
                                                int val, int dc ) {
  
  // store init value
  FUNSET(pObj, INIT_CARE);
  if (!dc){
    if (val) {
      FSET(pObj, INIT_1);
    } else {
      FSET(pObj, INIT_0);
    }
  }
}


/**Function*************************************************************

  Synopsis    [Propogates initial state through a logic node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_SimulateNode( Abc_Obj_t * pObj ) {
  Abc_Ntk_t *pNtk = Abc_ObjNtk(pObj);
  Abc_Obj_t * pFanin;
  int i, rAnd, rVar, dcAnd, dcVar;
#ifdef ABC_USE_CUDD
  DdManager * dd = (DdManager*)pNtk->pManFunc;
  DdNode *pBdd = (DdNode*)pObj->pData, *pVar;
#endif
  Hop_Man_t *pHop = (Hop_Man_t*)pNtk->pManFunc;
  
  assert(!Abc_ObjIsLatch(pObj));
  assert(Abc_ObjRegular(pObj));

  // (i) constant nodes
  if (Abc_NtkIsStrash(pNtk) && Abc_AigNodeIsConst(pObj)) {
    Abc_FlowRetime_SetInitValue(pObj, 1, 0);
    return;
  }
  if (!Abc_NtkIsStrash( pNtk ) && Abc_ObjIsNode(pObj)) {
    if (Abc_NodeIsConst0(pObj)) {
      Abc_FlowRetime_SetInitValue(pObj, 0, 0);
      return;
    } else if (Abc_NodeIsConst1(pObj)) {
      Abc_FlowRetime_SetInitValue(pObj, 1, 0);
      return;
    }
  }
  
  // (ii) terminal nodes
  if (!Abc_ObjIsNode(pObj)) {
    pFanin = Abc_ObjFanin0(pObj);
    
    Abc_FlowRetime_SetInitValue(pObj, 
         (FTEST(pFanin, INIT_1) ? 1 : 0) ^ pObj->fCompl0, 
         !FTEST(pFanin, INIT_CARE));    
    return;
  }

  // (iii) logic nodes

  // ------ SOP network
  if ( Abc_NtkHasSop( pNtk )) {
    Abc_FlowRetime_SimulateSop( pObj, (char *)Abc_ObjData(pObj) );
    return;
  }
#ifdef ABC_USE_CUDD
  // ------ BDD network
  else if ( Abc_NtkHasBdd( pNtk )) {
    assert(dd);
    assert(pBdd);

    // cofactor for 0,1 inputs
    // do nothing for X values
    Abc_ObjForEachFanin(pObj, pFanin, i) {
      pVar = Cudd_bddIthVar( dd, i );
      if (FTEST(pFanin, INIT_CARE)) {
        if (FTEST(pFanin, INIT_0))
          pBdd = Cudd_Cofactor( dd, pBdd, Cudd_Not(pVar) ); 
        else
          pBdd = Cudd_Cofactor( dd, pBdd, pVar ); 
      }
    }

    // if function has not been reduced to 
    // a constant, propagate an X
    rVar = (pBdd == Cudd_ReadOne(dd));
    dcVar = !Cudd_IsConstant(pBdd);
    
    Abc_FlowRetime_SetInitValue(pObj, rVar, dcVar);
    return;
  }
#endif // #ifdef ABC_USE_CUDD

  // ------ AIG logic network
  else if ( Abc_NtkHasAig( pNtk ) && !Abc_NtkIsStrash( pNtk )) {
    
    assert(Abc_ObjIsNode(pObj));
    assert(pObj->pData);
    assert(Abc_ObjFaninNum(pObj) <= Hop_ManPiNum(pHop) );
    
    // set vals at inputs
    Abc_ObjForEachFanin(pObj, pFanin, i) {
      Hop_ManPi(pHop, i)->fMarkA = FTEST(pFanin, INIT_1)?1:0;
      Hop_ManPi(pHop, i)->fMarkB = FTEST(pFanin, INIT_CARE)?1:0;
    }

    Abc_FlowRetime_EvalHop_rec( pHop, (Hop_Obj_t*)pObj->pData, &rVar, &dcVar );
   
    Abc_FlowRetime_SetInitValue(pObj, rVar, dcVar);

    // clear flags
    Abc_ObjForEachFanin(pObj, pFanin, i) {
      Hop_ManPi(pHop, i)->fMarkA = 0;
      Hop_ManPi(pHop, i)->fMarkB = 0;
    }

    return;
  }

  // ------ strashed network
  else if ( Abc_NtkIsStrash( pNtk )) {

    assert(Abc_ObjType(pObj) == ABC_OBJ_NODE);
    dcAnd = 0, rAnd = 1;
    
    pFanin = Abc_ObjFanin0(pObj);
    dcAnd |= FTEST(pFanin, INIT_CARE) ? 0 : 1;
    rVar = FTEST(pFanin, INIT_0) ? 0 : 1;
    if (pObj->fCompl0) rVar ^= 1; // complimented?
    rAnd &= rVar;
    
    pFanin = Abc_ObjFanin1(pObj);
    dcAnd |= FTEST(pFanin, INIT_CARE) ? 0 : 1;
    rVar = FTEST(pFanin, INIT_0) ? 0 : 1;
    if (pObj->fCompl1) rVar ^= 1; // complimented?
    rAnd &= rVar;
    
    if (!rAnd) dcAnd = 0; /* controlling value */
    
    Abc_FlowRetime_SetInitValue(pObj, rAnd, dcAnd);
    return;
  }

  // ------ MAPPED network
  else if ( Abc_NtkHasMapping( pNtk )) {
    Abc_FlowRetime_SimulateSop( pObj, (char *)Mio_GateReadSop((Mio_Gate_t*)pObj->pData) );
    return;
  }

  assert(0);
}


/**Function*************************************************************

  Synopsis    [Propogates initial state through a SOP node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_SimulateSop( Abc_Obj_t * pObj, char *pSop ) {
  Abc_Obj_t * pFanin;
  char *pCube;
  int i, j, rAnd, rOr, rVar, dcAnd, dcOr, v;

  assert( pSop && !Abc_SopIsExorType(pSop) );
      
  rOr = 0, dcOr = 0;

  i = Abc_SopGetVarNum(pSop);
  Abc_SopForEachCube( pSop, i, pCube ) {
    rAnd = 1, dcAnd = 0;
    Abc_CubeForEachVar( pCube, v, j ) {
      pFanin = Abc_ObjFanin(pObj, j);
      if ( v == '0' )
        rVar = FTEST(pFanin, INIT_0) ? 1 : 0;
      else if ( v == '1' )
        rVar = FTEST(pFanin, INIT_1) ? 1 : 0;
      else
        continue;
      
      if (FTEST(pFanin, INIT_CARE))
        rAnd &= rVar;
      else
        dcAnd = 1;
    }
    if (!rAnd) dcAnd = 0; /* controlling value */
    if (dcAnd) 
      dcOr = 1;
    else
      rOr |= rAnd;
  }
  if (rOr) dcOr = 0; /* controlling value */
  
  // complement the result if necessary
  if ( !Abc_SopGetPhase(pSop) )
    rOr ^= 1;
      
  Abc_FlowRetime_SetInitValue(pObj, rOr, dcOr);
}

/**Function*************************************************************

  Synopsis    [Sets up backward initial state computation.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_SetupBackwardInit( Abc_Ntk_t * pNtk ) {
  Abc_Obj_t *pLatch, *pObj, *pPi;
  int i;
  Vec_Ptr_t *vObj = Vec_PtrAlloc(100);

  // create the network used for the initial state computation
  if (Abc_NtkIsStrash(pNtk)) {
    pManMR->pInitNtk = Abc_NtkAlloc( ABC_NTK_LOGIC, ABC_FUNC_SOP, 1 );
  } else if (Abc_NtkHasMapping(pNtk))
    pManMR->pInitNtk = Abc_NtkAlloc( pNtk->ntkType, ABC_FUNC_SOP, 1 );
  else
    pManMR->pInitNtk = Abc_NtkAlloc( pNtk->ntkType, pNtk->ntkFunc, 1 );

  // mitre inputs
  Abc_NtkForEachLatch( pNtk, pLatch, i ) {
    // map latch to initial state network
    pPi = Abc_NtkCreatePi( pManMR->pInitNtk );

    // DEBUG
    // printf("setup : mapping latch %d to PI %d\n", pLatch->Id, pPi->Id);

    // has initial state requirement?
    if (Abc_LatchIsInit0(pLatch)) {
      pObj = Abc_NtkCreateNodeInv( pManMR->pInitNtk, pPi );
      Vec_PtrPush(vObj, pObj);
    }
    else if (Abc_LatchIsInit1(pLatch)) {
      Vec_PtrPush(vObj, pPi);
    }
    
    Abc_ObjSetData( pLatch, pPi );     // if not verifying init state
    // FDATA(pLatch)->pInitObj = pPi;  // if verifying init state
  }

  // are there any nodes not DC?
  if (!Vec_PtrSize(vObj)) {
    pManMR->fSolutionIsDc = 1;
    return;
  } else 
    pManMR->fSolutionIsDc = 0;

  // mitre output

  // create n-input AND gate
  pObj = Abc_NtkCreateNodeAnd( pManMR->pInitNtk, vObj );

  Abc_ObjAddFanin( Abc_NtkCreatePo( pManMR->pInitNtk ), pObj );

  Vec_PtrFree( vObj );
}


/**Function*************************************************************

  Synopsis    [Solves backward initial state computation.]

  Description []
               
  SideEffects [Sets object copies in init ntk.]

  SeeAlso     []

***********************************************************************/
int Abc_FlowRetime_SolveBackwardInit( Abc_Ntk_t * pNtk ) {
  int i;
  Abc_Obj_t *pObj, *pInitObj;
  Vec_Ptr_t *vDelete = Vec_PtrAlloc(0);
  Abc_Ntk_t *pSatNtk;
  int result;

  assert(pManMR->pInitNtk);

  // is the solution entirely DC's?
  if (pManMR->fSolutionIsDc) {
    Vec_PtrFree(vDelete);
    Abc_NtkForEachLatch( pNtk, pObj, i ) Abc_LatchSetInitDc( pObj );
    vprintf("\tno init state computation: all-don't-care solution\n");
    return 1;
  }

  // check that network is combinational
  Abc_NtkForEachObj( pManMR->pInitNtk, pObj, i ) {
    assert(!Abc_ObjIsLatch(pObj));
    assert(!Abc_ObjIsBo(pObj));
    assert(!Abc_ObjIsBi(pObj));
  }
  
  // delete superfluous nodes
  while(Vec_PtrSize( vDelete )) {
    pObj = (Abc_Obj_t *)Vec_PtrPop( vDelete );
    Abc_NtkDeleteObj( pObj );
  }
  Vec_PtrFree(vDelete);

  // do some final cleanup on the network
  Abc_NtkAddDummyPoNames(pManMR->pInitNtk);
  Abc_NtkAddDummyPiNames(pManMR->pInitNtk);
  if (Abc_NtkIsLogic(pManMR->pInitNtk))
    Abc_NtkCleanup(pManMR->pInitNtk, 0);

#if defined(DEBUG_PRINT_INIT_NTK)
  Abc_NtkLevelReverse( pManMR->pInitNtk );
  Abc_NtkForEachObj( pManMR->pInitNtk, pObj, i ) 
    if (Abc_ObjLevel( pObj ) < 2)
      Abc_ObjPrint(stdout, pObj);
#endif
  
  vprintf("\tsolving for init state (%d nodes)... ", Abc_NtkObjNum(pManMR->pInitNtk));
  fflush(stdout);
#ifdef ABC_USE_CUDD
  // convert SOPs to BDD
  if (Abc_NtkHasSop(pManMR->pInitNtk))
    Abc_NtkSopToBdd( pManMR->pInitNtk );
  // convert AIGs to BDD
  if (Abc_NtkHasAig(pManMR->pInitNtk))
    Abc_NtkAigToBdd( pManMR->pInitNtk );
#else
  // convert SOPs to AIG
  if (Abc_NtkHasSop(pManMR->pInitNtk))
    Abc_NtkSopToAig( pManMR->pInitNtk );
#endif
  pSatNtk = pManMR->pInitNtk;
  
  // solve
  result = Abc_NtkMiterSat( pSatNtk, (ABC_INT64_T)500000, (ABC_INT64_T)50000000, 0, NULL, NULL );

  if (!result) { 
    vprintf("SUCCESS\n");
  } else  {    
    vprintf("FAILURE\n");
    return 0;
  }

  // clear initial values, associate PIs to latches
  Abc_NtkForEachPi( pManMR->pInitNtk, pInitObj, i ) Abc_ObjSetCopy( pInitObj, NULL );
  Abc_NtkForEachLatch( pNtk, pObj, i ) {
    pInitObj = (Abc_Obj_t*)Abc_ObjData( pObj );
    assert( Abc_ObjIsPi( pInitObj ));
    Abc_ObjSetCopy( pInitObj, pObj );
    Abc_LatchSetInitNone( pObj );

    // DEBUG
    // printf("solve : getting latch %d from PI %d\n", pObj->Id, pInitObj->Id);
  }

  // copy solution from PIs to latches
  assert(pManMR->pInitNtk->pModel);
  Abc_NtkForEachPi( pManMR->pInitNtk, pInitObj, i ) {
    if ((pObj = Abc_ObjCopy( pInitObj ))) {
      if ( pManMR->pInitNtk->pModel[i] )
        Abc_LatchSetInit1( pObj );
      else
        Abc_LatchSetInit0( pObj );
    }
  }

#if defined(DEBUG_CHECK)
  // check that all latches have initial state
  Abc_NtkForEachLatch( pNtk, pObj, i ) assert( !Abc_LatchIsInitNone( pObj ) );
#endif

  return 1;
}


/**Function*************************************************************

  Synopsis    [Updates backward initial state computation problem.]

  Description [Assumes box outputs in old positions stored w/ init values.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_UpdateBackwardInit( Abc_Ntk_t * pNtk ) {
  Abc_Obj_t *pOrigObj,  *pInitObj;
  Vec_Ptr_t *vBo = Vec_PtrAlloc(100);
  Vec_Ptr_t *vPi = Vec_PtrAlloc(100);
  Abc_Ntk_t *pInitNtk = pManMR-> pInitNtk;
  Abc_Obj_t *pBuf;
  int i;

  // remove PIs from network (from BOs)
  Abc_NtkForEachObj( pNtk, pOrigObj, i )
    if (Abc_ObjIsBo(pOrigObj)) {
      pInitObj = FDATA(pOrigObj)->pInitObj;
      assert(Abc_ObjIsPi(pInitObj));

      // DEBUG
      // printf("update : freeing PI %d\n", pInitObj->Id);
      
      // create a buffer instead
      pBuf = Abc_NtkCreateNodeBuf( pInitNtk, NULL );
      Abc_FlowRetime_ClearInitToOrig( pBuf );

      Abc_ObjBetterTransferFanout( pInitObj, pBuf, 0 );
      FDATA(pOrigObj)->pInitObj = pBuf;
      pOrigObj->fMarkA = 1;

      Vec_PtrPush(vBo, pOrigObj);
      Vec_PtrPush(vPi, pInitObj);
    }
  
  // check that PIs are all free
  Abc_NtkForEachPi( pInitNtk, pInitObj, i) {
    assert( Abc_ObjFanoutNum( pInitObj ) == 0);
  }

  // add PIs to to latches
  Abc_NtkForEachLatch( pNtk, pOrigObj, i ) {
    assert(Vec_PtrSize(vPi) > 0);
    pInitObj = (Abc_Obj_t*)Vec_PtrPop(vPi);

    // DEBUG
    // printf("update : mapping latch %d to PI %d\n", pOrigObj->Id, pInitObj->Id);

    pOrigObj->fMarkA = pOrigObj->fMarkB = 1;
    FDATA(pOrigObj)->pInitObj = pInitObj;
    Abc_ObjSetData(pOrigObj, pInitObj);
  }  

  // recursively build init network
  Vec_PtrForEachEntry( Abc_Obj_t *, vBo, pOrigObj, i )
    Abc_FlowRetime_UpdateBackwardInit_rec( pOrigObj );
  
  // clear flags
  Abc_NtkForEachObj( pNtk, pOrigObj, i )
    pOrigObj->fMarkA = pOrigObj->fMarkB = 0;

  // deallocate
  Vec_PtrFree( vBo );
  Vec_PtrFree( vPi );
}


/**Function*************************************************************

  Synopsis    [Creates a corresponding node in the init state network]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Abc_Obj_t *Abc_FlowRetime_CopyNodeToInitNtk( Abc_Obj_t *pOrigObj ) {
  Abc_Ntk_t *pNtk = pManMR->pNtk;
  Abc_Ntk_t *pInitNtk = pManMR->pInitNtk;
  Abc_Obj_t *pInitObj;
  void *pData;
  int fCompl[2];

  assert(pOrigObj);

  // what we do depends on the ntk types of original / init networks...
  
  // (0) convert BI/BO nodes to buffers
  if (Abc_ObjIsBi( pOrigObj ) || Abc_ObjIsBo( pOrigObj ) ) {
    pInitObj =  Abc_NtkCreateNodeBuf( pInitNtk, NULL );
    Abc_FlowRetime_ClearInitToOrig( pInitObj );
    return pInitObj;
  }

  // (i) strash node -> SOP node
  if (Abc_NtkIsStrash( pNtk )) {

    if (Abc_AigNodeIsConst( pOrigObj )) {
      return Abc_NtkCreateNodeConst1( pInitNtk );
    }
    if (!Abc_ObjIsNode( pOrigObj )) {
      assert(Abc_ObjFaninNum(pOrigObj) == 1);
      pInitObj =  Abc_NtkCreateNodeBuf( pInitNtk, NULL );
      Abc_FlowRetime_ClearInitToOrig( pInitObj );
      return pInitObj;
    }

    assert( Abc_ObjIsNode(pOrigObj) );
    pInitObj = Abc_NtkCreateObj( pInitNtk, ABC_OBJ_NODE );
    
    fCompl[0] = pOrigObj->fCompl0 ? 1 : 0;
    fCompl[1] = pOrigObj->fCompl1 ? 1 : 0;
    
    pData =  Abc_SopCreateAnd( (Mem_Flex_t *)pInitNtk->pManFunc, 2, fCompl );
    assert(pData);
    pInitObj->pData = Abc_SopRegister( (Mem_Flex_t *)pInitNtk->pManFunc, (const char*)pData );
  } 

  // (ii) mapped node -> SOP node
  else if (Abc_NtkHasMapping( pNtk )) {
    if (!pOrigObj->pData) {
      // assume terminal...
      assert(Abc_ObjFaninNum(pOrigObj) == 1);

      pInitObj =  Abc_NtkCreateNodeBuf( pInitNtk, NULL );
      Abc_FlowRetime_ClearInitToOrig( pInitObj );
      return pInitObj;
    }

    pInitObj = Abc_NtkCreateObj( pInitNtk, (Abc_ObjType_t)Abc_ObjType(pOrigObj) );
    pData = Mio_GateReadSop((Mio_Gate_t*)pOrigObj->pData);
    assert( Abc_SopGetVarNum((char*)pData) == Abc_ObjFaninNum(pOrigObj) );
    
    pInitObj->pData = Abc_SopRegister( (Mem_Flex_t *)pInitNtk->pManFunc, (const char*)pData );
  } 

  // (iii) otherwise, duplicate obj
  else {
    pInitObj = Abc_NtkDupObj( pInitNtk, pOrigObj, 0 );
    
    // copy phase
    pInitObj->fPhase = pOrigObj->fPhase;
  }

  assert(pInitObj);
  return pInitObj;
}

/**Function*************************************************************

  Synopsis    [Updates backward initial state computation problem.]

  Description [Creates a duplicate node in the initial state network 
               corresponding to a node in the original circuit.  If
               fRecurse is set, the procedure recurses on and connects
               the new node to its fan-ins.  A latch in the original
               circuit corresponds to a PI in the initial state network.
               An existing PI may be supplied by pUseThisPi, and if the
               node is a latch, it will be used; otherwise the PI is
               saved in the list vOtherPis and subsequently used for
               another latch.]
               
  SideEffects [Nodes that have a corresponding initial state node
               are marked with fMarkA.  Nodes that have been fully
               connected in the initial state network are marked with
               fMarkB.]

  SeeAlso     []

***********************************************************************/
Abc_Obj_t* Abc_FlowRetime_UpdateBackwardInit_rec( Abc_Obj_t *pOrigObj) {
  Abc_Obj_t *pOrigFanin, *pInitFanin, *pInitObj;
  int i;

  assert(pOrigObj);

  // should never reach primary IOs
  assert(!Abc_ObjIsPi(pOrigObj));
  assert(!Abc_ObjIsPo(pOrigObj));

  // skip bias nodes
  if (FTEST(pOrigObj, BIAS_NODE)) 
    return NULL;

  // does an init node already exist?
  if(!pOrigObj->fMarkA) {

    pInitObj = Abc_FlowRetime_CopyNodeToInitNtk( pOrigObj );

    Abc_FlowRetime_SetInitToOrig( pInitObj, pOrigObj );
    FDATA(pOrigObj)->pInitObj = pInitObj;

    pOrigObj->fMarkA = 1;
  } else {
    pInitObj = FDATA(pOrigObj)->pInitObj;
  }
  assert(pInitObj);
    
  // have we already connected this object?
  if (!pOrigObj->fMarkB) {

    // create and/or connect fanins
    Abc_ObjForEachFanin( pOrigObj, pOrigFanin, i ) {
      // should not reach BOs (i.e. the start of the next frame)
      // the new latch bounday should lie before it
      assert(!Abc_ObjIsBo( pOrigFanin ));
      pInitFanin = Abc_FlowRetime_UpdateBackwardInit_rec( pOrigFanin );
      Abc_ObjAddFanin( pInitObj, pInitFanin );
    }

    pOrigObj->fMarkB = 1;
  }

  return pInitObj;
}


/**Function*************************************************************

  Synopsis    [Verifies backward init state computation.]

  Description [This procedure requires the BOs to store the original
               latch values and the latches to store the new values:
               both in the INIT_0 and INIT_1 flags in the Flow_Data
               structure.  (This is not currently the case in the rest
               of the code.)  Also, can not verify backward state
               computations that span multiple combinational frames.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_VerifyBackwardInit( Abc_Ntk_t * pNtk ) {
  Abc_Obj_t *pObj, *pFanin;
  int i;

  vprintf("\t\tupdating init state\n");

  Abc_NtkIncrementTravId( pNtk );

  Abc_NtkForEachObj( pNtk, pObj, i )
    if (Abc_ObjIsBo( pObj )) {
      pFanin = Abc_ObjFanin0(pObj);
      Abc_FlowRetime_VerifyBackwardInit_rec( pFanin );

      if (FTEST(pObj, INIT_CARE)) {
        if(FTEST(pObj, INIT_CARE) != FTEST(pFanin, INIT_CARE)) {
          printf("ERROR: expected val=%d care=%d and got val=%d care=%d\n",
                 FTEST(pObj, INIT_1)?1:0, FTEST(pObj, INIT_CARE)?1:0, 
                 FTEST(pFanin, INIT_1)?1:0, FTEST(pFanin, INIT_CARE)?1:0 );

        }
      }
    }
}

void Abc_FlowRetime_VerifyBackwardInit_rec( Abc_Obj_t * pObj ) {
  Abc_Obj_t *pNext;
  int i;

  assert(!Abc_ObjIsBo(pObj)); // should never reach the inputs
  assert(!Abc_ObjIsPi(pObj)); // should never reach the inputs

  // visited?
  if (Abc_NodeIsTravIdCurrent(pObj)) return;
  Abc_NodeSetTravIdCurrent(pObj);

  if (Abc_ObjIsLatch(pObj)) {
    FUNSET(pObj, INIT_CARE);
    if (Abc_LatchIsInit0(pObj))
      FSET(pObj, INIT_0);
    else if (Abc_LatchIsInit1(pObj))
      FSET(pObj, INIT_1);
    return;
  }

  Abc_ObjForEachFanin( pObj, pNext, i ) {
    Abc_FlowRetime_VerifyBackwardInit_rec( pNext );
  }
  
  Abc_FlowRetime_SimulateNode( pObj );
}

/**Function*************************************************************

  Synopsis    [Constrains backward retiming for initializability.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_FlowRetime_PartialSat(Vec_Ptr_t *vNodes, int cut) {
  Abc_Ntk_t *pPartNtk, *pInitNtk = pManMR->pInitNtk;
  Abc_Obj_t *pObj, *pNext, *pPartObj, *pPartNext, *pPo;
  int i, j, result;

  assert( Abc_NtkPoNum( pInitNtk ) == 1 );

  pPartNtk = Abc_NtkAlloc( pInitNtk->ntkType, pInitNtk->ntkFunc, 0 );

  // copy network
  Vec_PtrForEachEntry( Abc_Obj_t *, vNodes, pObj, i ) {
    pObj->Level = i;
    assert(!Abc_ObjIsPo( pObj ));

    if (i < cut && !pObj->fMarkA) {
      pPartObj = Abc_NtkCreatePi( pPartNtk );
      Abc_ObjSetCopy( pObj, pPartObj );
    } else {
      // copy node
      pPartObj = Abc_NtkDupObj( pPartNtk, pObj, 0 );
      // copy complementation
      pPartObj->fPhase = pObj->fPhase;
   
      // connect fanins
      Abc_ObjForEachFanin( pObj, pNext, j ) {   
        pPartNext = Abc_ObjCopy( pNext );
        assert(pPartNext);
        Abc_ObjAddFanin( pPartObj, pPartNext );
      }
    }

    assert(pObj->pCopy == pPartObj);
  }
  
  // create PO
  pPo = Abc_NtkCreatePo( pPartNtk );
  pNext = Abc_ObjFanin0( Abc_NtkPo( pInitNtk, 0 ) );
  pPartNext = Abc_ObjCopy( pNext );
  assert( pPartNext );
  Abc_ObjAddFanin( pPo, pPartNext );
  
  // check network
#if defined(DEBUG_CHECK)
  Abc_NtkAddDummyPoNames(pPartNtk);
  Abc_NtkAddDummyPiNames(pPartNtk);
  Abc_NtkCheck( pPartNtk );
#endif

  result = Abc_NtkMiterSat( pPartNtk, (ABC_INT64_T)500000, (ABC_INT64_T)50000000, 0, NULL, NULL );

  Abc_NtkDelete( pPartNtk );
 
  return !result;
}


/**Function*************************************************************

  Synopsis    [Constrains backward retiming for initializability.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_ConstrainInit( ) {
  Vec_Ptr_t *vNodes;
  int low, high, mid;
  int i, n, lag;
  Abc_Obj_t *pObj = NULL, *pOrigObj;
  InitConstraint_t *pConstraint = ABC_ALLOC( InitConstraint_t, 1 );

  memset( pConstraint, 0, sizeof(InitConstraint_t) );

  assert(pManMR->pInitNtk);

  vprintf("\tsearch for initial state conflict...\n");

  vNodes = Abc_NtkDfs(pManMR->pInitNtk, 0);
  n = Vec_PtrSize(vNodes);
  // also add PIs to vNodes
  Abc_NtkForEachPi(pManMR->pInitNtk, pObj, i) 
    Vec_PtrPush(vNodes, pObj);
  Vec_PtrReorder(vNodes, n);

#if defined(DEBUG_CHECK)
    assert(!Abc_FlowRetime_PartialSat( vNodes, 0 ));
#endif

  // grow initialization constraint
  do {
    vprintf("\t\t");

    // find element to add to set...
    low = 0, high = Vec_PtrSize(vNodes);
    while (low != high-1) {
      mid = (low + high) >> 1;
      
      if (!Abc_FlowRetime_PartialSat( vNodes, mid )) {
        low = mid;
        vprintf("-");
      } else {
        high = mid;
        vprintf("*");
      }
      fflush(stdout);
    }
      
#if defined(DEBUG_CHECK)
    assert(Abc_FlowRetime_PartialSat( vNodes, high ));
    assert(!Abc_FlowRetime_PartialSat( vNodes, low ));
#endif
    
    // mark its TFO
    pObj = (Abc_Obj_t*)Vec_PtrEntry( vNodes, low );
    Abc_NtkMarkCone_rec( pObj, 1 );
    vprintf("   conflict term = %d ", low);

#if 0
    printf("init ------\n");
    Abc_ObjPrint(stdout, pObj);
    printf("\n");
    Abc_ObjPrintNeighborhood( pObj, 1 );
    printf("------\n");
#endif

    // add node to constraint
    Abc_FlowRetime_GetInitToOrig( pObj, &pOrigObj, &lag );
    assert(pOrigObj);
    vprintf(" <=> %d/%d\n", Abc_ObjId(pOrigObj), lag);

#if 0    
    printf("orig ------\n");
    Abc_ObjPrint(stdout, pOrigObj);
    printf("\n");
    Abc_ObjPrintNeighborhood( pOrigObj, 1 );
    printf("------\n");
#endif
    Vec_IntPush( &pConstraint->vNodes, Abc_ObjId(pOrigObj) );
    Vec_IntPush( &pConstraint->vLags, lag );

  } while (Abc_FlowRetime_PartialSat( vNodes, Vec_PtrSize(vNodes) ));

  pConstraint->pBiasNode = NULL;

  // add constraint
  Vec_PtrPush( pManMR->vInitConstraints, pConstraint );

  // clear marks
  Abc_NtkForEachObj( pManMR->pInitNtk, pObj, i)
    pObj->fMarkA = 0;

  // free
  Vec_PtrFree( vNodes );
}


/**Function*************************************************************

  Synopsis    [Removes nodes to bias against uninitializable cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_RemoveInitBias( ) {
  // Abc_Ntk_t *pNtk = pManMR->pNtk;
  Abc_Obj_t *pBiasNode;
  InitConstraint_t *pConstraint;
  int i;

  Vec_PtrForEachEntry( InitConstraint_t *, pManMR->vInitConstraints, pConstraint, i ) {
    pBiasNode = pConstraint->pBiasNode;
    pConstraint->pBiasNode = NULL;

    if (pBiasNode)
      Abc_NtkDeleteObj(pBiasNode);
  }
}


/**Function*************************************************************

  Synopsis    [Connects the bias node to one of the constraint vertices.]

  Description [ACK!
               Currently this is dumb dumb hack.
               What should we do with biases that belong on BOs?  These
               move through the circuit.
               Currently, the bias gets marked on the fan-in of BO
               and the bias gets implemented on every BO fan-out of a
               node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static void Abc_FlowRetime_ConnectBiasNode(Abc_Obj_t *pBiasNode, Abc_Obj_t *pObj, int biasLag) {
  Abc_Obj_t *pCur, *pNext;
  int i;
  int lag;
  Vec_Ptr_t *vNodes = Vec_PtrAlloc(1);
  Vec_Int_t *vLags = Vec_IntAlloc(1);
  Abc_Ntk_t *pNtk = Abc_ObjNtk( pObj );
  
  Vec_PtrPush( vNodes, pObj );
  Vec_IntPush( vLags, 0 );

  Abc_NtkIncrementTravId( pNtk );

  while (Vec_PtrSize( vNodes )) {
    pCur = (Abc_Obj_t*)Vec_PtrPop( vNodes );
    lag = Vec_IntPop( vLags );

    if (Abc_NodeIsTravIdCurrent( pCur )) continue;
    Abc_NodeSetTravIdCurrent( pCur );

    if (!Abc_ObjIsLatch(pCur) &&
        !Abc_ObjIsBo(pCur) &&
        Abc_FlowRetime_GetLag(pObj)+lag == biasLag ) {

      // printf("biasing : ");
      // Abc_ObjPrint(stdout,  pCur );
#if 1
      FSET( pCur, BLOCK );
#else
      Abc_ObjAddFanin( pCur, pBiasNode );
#endif
    }

    Abc_ObjForEachFanout( pCur, pNext, i ) {
      if (Abc_ObjIsBi(pNext) ||
          Abc_ObjIsLatch(pNext) ||
          Abc_ObjIsBo(pNext) ||
          Abc_ObjIsBo(pCur)) {
        Vec_PtrPush( vNodes, pNext );
        Vec_IntPush( vLags, lag - Abc_ObjIsLatch(pNext) ? 1 : 0 );
      }
    }
  }

  Vec_PtrFree( vNodes );
  Vec_IntFree( vLags );
}

/**Function*************************************************************

  Synopsis    [Adds nodes to bias against uninitializable cuts.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_AddInitBias( ) {
  Abc_Ntk_t *pNtk = pManMR->pNtk;
  Abc_Obj_t *pBiasNode, *pObj;
  InitConstraint_t *pConstraint;
  int i, j, id;
  const int nConstraints = Vec_PtrSize( pManMR->vInitConstraints );

  pManMR->pDataArray = ABC_REALLOC( Flow_Data_t, pManMR->pDataArray, pManMR->nNodes + (nConstraints*(pManMR->iteration+1)) );
  memset(pManMR->pDataArray + pManMR->nNodes, 0, sizeof(Flow_Data_t)*(nConstraints*(pManMR->iteration+1)));

  vprintf("\t\tcreating %d bias structures\n", nConstraints);

  Vec_PtrForEachEntry(InitConstraint_t*, pManMR->vInitConstraints, pConstraint, i ) {
    if (pConstraint->pBiasNode) continue;
    
 //   printf("\t\t\tbias %d...\n", i);
    pBiasNode = Abc_NtkCreateBlackbox( pNtk );

    Vec_IntForEachEntry( &pConstraint->vNodes, id, j ) {
      pObj = Abc_NtkObj(pNtk, id);
      Abc_FlowRetime_ConnectBiasNode(pBiasNode, pObj, Vec_IntEntry(&pConstraint->vLags, j));
    }

    // pConstraint->pBiasNode = pBiasNode;
  }
}


/**Function*************************************************************

  Synopsis    [Clears mapping from init node to original node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_ClearInitToOrig( Abc_Obj_t *pInit )
{
  int id = Abc_ObjId( pInit );
  
  // grow data structure if necessary
  if (id >= pManMR->sizeInitToOrig) {
    int oldSize = pManMR->sizeInitToOrig;
    pManMR->sizeInitToOrig = 1.5*id + 10;
    pManMR->pInitToOrig = (NodeLag_t*)realloc(pManMR->pInitToOrig, sizeof(NodeLag_t)*pManMR->sizeInitToOrig);
    memset( &(pManMR->pInitToOrig[oldSize]), 0, sizeof(NodeLag_t)*(pManMR->sizeInitToOrig-oldSize) );
  }
  assert( pManMR->pInitToOrig );

  pManMR->pInitToOrig[id].id = -1;
}


/**Function*************************************************************

  Synopsis    [Sets mapping from init node to original node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_SetInitToOrig( Abc_Obj_t *pInit, Abc_Obj_t *pOrig)
{
  int lag;
  int id = Abc_ObjId( pInit );
  
  // grow data structure if necessary
  if (id >= pManMR->sizeInitToOrig) {
    int oldSize = pManMR->sizeInitToOrig;
    pManMR->sizeInitToOrig = 1.5*id + 10;
    pManMR->pInitToOrig = (NodeLag_t*)realloc(pManMR->pInitToOrig, sizeof(NodeLag_t)*pManMR->sizeInitToOrig);
    memset( &(pManMR->pInitToOrig[oldSize]), 0, sizeof(NodeLag_t)*(pManMR->sizeInitToOrig-oldSize) );
  }
  assert( pManMR->pInitToOrig );

  // ignore BI, BO, and latch nodes
  if (Abc_ObjIsBo(pOrig) || Abc_ObjIsBi(pOrig) || Abc_ObjIsLatch(pOrig)) {
    Abc_FlowRetime_ClearInitToOrig(pInit);
    return;
  }

  // move out of latch boxes
  lag = Abc_FlowRetime_ObjFirstNonLatchBox(pOrig, &pOrig);

  pManMR->pInitToOrig[id].id = Abc_ObjId(pOrig);
  pManMR->pInitToOrig[id].lag = Abc_FlowRetime_GetLag(pOrig) + lag;
}


/**Function*************************************************************

  Synopsis    [Gets mapping from init node to original node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_GetInitToOrig( Abc_Obj_t *pInit, Abc_Obj_t **pOrig, int *lag ) {

  int id = Abc_ObjId( pInit );
  int origId;

  assert(id < pManMR->sizeInitToOrig);

  origId = pManMR->pInitToOrig[id].id;

  if (origId < 0) {
    assert(Abc_ObjFaninNum(pInit));
    Abc_FlowRetime_GetInitToOrig( Abc_ObjFanin0(pInit), pOrig, lag);
    return;
  }

  *pOrig = Abc_NtkObj(pManMR->pNtk, origId);
  *lag = pManMR->pInitToOrig[id].lag;
}
ABC_NAMESPACE_IMPL_END

