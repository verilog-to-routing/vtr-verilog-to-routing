/**CFile****************************************************************

  FileName    [fretTime.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Flow-based retiming package.]

  Synopsis    [Delay-constrained retiming code.]

  Author      [Aaron Hurst]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2008.]

  Revision    [$Id: fretTime.c,v 1.00 2008/01/01 00:00:00 ahurst Exp $]

***********************************************************************/

#include "fretime.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Abc_FlowRetime_Dfs_forw( Abc_Obj_t * pObj, Vec_Ptr_t *vNodes );
static void Abc_FlowRetime_Dfs_back( Abc_Obj_t * pObj, Vec_Ptr_t *vNodes );

static void Abc_FlowRetime_ConstrainExact_forw( Abc_Obj_t * pObj );
static void Abc_FlowRetime_ConstrainExact_back( Abc_Obj_t * pObj );
static void Abc_FlowRetime_ConstrainConserv_forw( Abc_Ntk_t * pNtk );
static void Abc_FlowRetime_ConstrainConserv_back( Abc_Ntk_t * pNtk );


void trace2(Abc_Obj_t *pObj) {
  Abc_Obj_t *pNext;
  int i;

  print_node(pObj);
  Abc_ObjForEachFanin(pObj, pNext, i) 
    if (pNext->Level >= pObj->Level - 1) {
      trace2(pNext);
      break;
    }
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////


/**Function*************************************************************

  Synopsis    [Initializes timing]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_InitTiming( Abc_Ntk_t *pNtk ) {

  pManMR->nConservConstraints = pManMR->nExactConstraints = 0;

  pManMR->vExactNodes = Vec_PtrAlloc(1000);

  pManMR->vTimeEdges = ABC_ALLOC( Vec_Ptr_t, Abc_NtkObjNumMax(pNtk)+1 );
  assert(pManMR->vTimeEdges);
  memset(pManMR->vTimeEdges, 0, (Abc_NtkObjNumMax(pNtk)+1) * sizeof(Vec_Ptr_t) );
}


/**Function*************************************************************

  Synopsis    [Marks nodes with conservative constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_ConstrainConserv( Abc_Ntk_t * pNtk ) {  
  Abc_Obj_t *pObj;
  int i;
  void *pArray;

  // clear all exact constraints
  pManMR->nExactConstraints = 0;
  while( Vec_PtrSize( pManMR->vExactNodes )) {
    pObj = (Abc_Obj_t*)Vec_PtrPop( pManMR->vExactNodes );
    
    if ( Vec_PtrSize( FTIMEEDGES(pObj) )) {
      pArray =  Vec_PtrReleaseArray( FTIMEEDGES(pObj) );
      ABC_FREE( pArray );
    }
  }

#if !defined(IGNORE_TIMING)
  if (pManMR->fIsForward) {
    Abc_FlowRetime_ConstrainConserv_forw(pNtk);
  } else {
    Abc_FlowRetime_ConstrainConserv_back(pNtk);
  }
#endif

  Abc_NtkForEachObj( pNtk, pObj, i)
    assert( !Vec_PtrSize(FTIMEEDGES(pObj)) );
}


void Abc_FlowRetime_ConstrainConserv_forw( Abc_Ntk_t * pNtk ) {
  Vec_Ptr_t *vNodes = pManMR->vNodes;
  Abc_Obj_t *pObj, *pNext, *pBi, *pBo;
  int i, j;
  
  assert(!Vec_PtrSize( vNodes ));
  pManMR->nConservConstraints = 0;

  // 1. hard constraints

  // (i) collect TFO of PIs
  Abc_NtkIncrementTravId(pNtk);
  Abc_NtkForEachPi(pNtk, pObj, i)
    Abc_FlowRetime_Dfs_forw( pObj, vNodes );

  // ... propagate values
  Vec_PtrForEachEntryReverse( Abc_Obj_t *,vNodes, pObj, i) {
    pObj->Level = 0;
    Abc_ObjForEachFanin( pObj, pNext, j )
    {
      if ( Abc_NodeIsTravIdCurrent(pNext) &&
           pObj->Level < pNext->Level )
        pObj->Level = pNext->Level;
    }
    pObj->Level += Abc_ObjIsNode(pObj) ? 1 : 0;

    if ( Abc_ObjIsBi(pObj) )
      pObj->fMarkA = 1;

    assert((int)pObj->Level <= pManMR->maxDelay);
  }

  // collect TFO of latches
  // seed arrival times from BIs
  Vec_PtrClear(vNodes);
  Abc_NtkIncrementTravId(pNtk);
  Abc_NtkForEachLatch(pNtk, pObj, i) {
    pBo = Abc_ObjFanout0( pObj );
    pBi = Abc_ObjFanin0( pObj );

    Abc_NodeSetTravIdCurrent( pObj );
    Abc_FlowRetime_Dfs_forw( pBo, vNodes );

    if (pBi->fMarkA) {
      pBi->fMarkA = 0;
      pObj->Level = pBi->Level;
      assert((int)pObj->Level <= pManMR->maxDelay);
    } else
      pObj->Level = 0;
  }

#if defined(DEBUG_CHECK)
  // DEBUG: check DFS ordering
  Vec_PtrForEachEntryReverse( Abc_Obj_t *,vNodes, pObj, i) {
    pObj->fMarkB = 1;
    
    Abc_ObjForEachFanin( pObj, pNext, j )
      if ( Abc_NodeIsTravIdCurrent(pNext) && !Abc_ObjIsLatch(pNext))
        assert(pNext->fMarkB);
  }
  Vec_PtrForEachEntryReverse( Abc_Obj_t *,vNodes, pObj, i)
    pObj->fMarkB = 0;
#endif

  // ... propagate values
  Vec_PtrForEachEntryReverse( Abc_Obj_t *,vNodes, pObj, i) {
    pObj->Level = 0;
    Abc_ObjForEachFanin( pObj, pNext, j )
    {
      if ( Abc_NodeIsTravIdCurrent(pNext) &&
           pObj->Level < pNext->Level )
        pObj->Level = pNext->Level;
    }
    pObj->Level += Abc_ObjIsNode(pObj) ? 1 : 0;

    if ((int)pObj->Level > pManMR->maxDelay) {
      FSET(pObj, BLOCK);
    }
  }

  // 2. conservative constraints

  // first pass: seed latches with T=0
  Abc_NtkForEachLatch(pNtk, pObj, i) {
    pObj->Level = 0;
  }

  // ... propagate values
  Vec_PtrForEachEntryReverse( Abc_Obj_t *,vNodes, pObj, i) {
    pObj->Level = 0;
    Abc_ObjForEachFanin( pObj, pNext, j ) {
      if ( Abc_NodeIsTravIdCurrent(pNext) &&
           pObj->Level < pNext->Level )
        pObj->Level = pNext->Level;
    }
    pObj->Level += Abc_ObjIsNode(pObj) ? 1 : 0;

    if ( Abc_ObjIsBi(pObj) )
      pObj->fMarkA = 1;

    assert((int)pObj->Level <= pManMR->maxDelay);
  }

  Abc_NtkForEachLatch(pNtk, pObj, i) {
    pBo = Abc_ObjFanout0( pObj );
    pBi = Abc_ObjFanin0( pObj );

    if (pBi->fMarkA) {
      pBi->fMarkA = 0;
      pObj->Level = pBi->Level;
      assert((int)pObj->Level <= pManMR->maxDelay);
    } else
      pObj->Level = 0;
  }

  // ... propagate values
  Vec_PtrForEachEntryReverse( Abc_Obj_t *,vNodes, pObj, i) {
    pObj->Level = 0;
    Abc_ObjForEachFanin( pObj, pNext, j ) {
      if ( Abc_NodeIsTravIdCurrent(pNext) &&
           pObj->Level < pNext->Level )
        pObj->Level = pNext->Level;
    }
    pObj->Level += Abc_ObjIsNode(pObj) ? 1 : 0;

    // constrained?
    if ((int)pObj->Level > pManMR->maxDelay) {
      FSET( pObj, CONSERVATIVE );
      pManMR->nConservConstraints++;
    } else
      FUNSET( pObj, CONSERVATIVE );
  }

  Vec_PtrClear( vNodes );
}


void Abc_FlowRetime_ConstrainConserv_back( Abc_Ntk_t * pNtk ) {
  Vec_Ptr_t *vNodes = pManMR->vNodes;
  Abc_Obj_t *pObj, *pNext, *pBi, *pBo;
  int i, j, l;
  
  assert(!Vec_PtrSize(vNodes));

  pManMR->nConservConstraints = 0;

  // 1. hard constraints

  // (i) collect TFO of POs
  Abc_NtkIncrementTravId(pNtk);
  Abc_NtkForEachPo(pNtk, pObj, i)
    Abc_FlowRetime_Dfs_back( pObj, vNodes );

  // ... propagate values
  Vec_PtrForEachEntryReverse( Abc_Obj_t *,vNodes, pObj, i) {
    pObj->Level = 0;
    Abc_ObjForEachFanout( pObj, pNext, j )
    {
      l = pNext->Level + (Abc_ObjIsNode(pObj) ? 1 : 0);
      if ( Abc_NodeIsTravIdCurrent(pNext) &&
           (int)pObj->Level < l )
        pObj->Level = l;
    }

    if ( Abc_ObjIsBo(pObj) )
      pObj->fMarkA = 1;

    assert((int)pObj->Level <= pManMR->maxDelay);
  }

  // collect TFO of latches
  // seed arrival times from BIs
  Vec_PtrClear(vNodes);
  Abc_NtkIncrementTravId(pNtk);
  Abc_NtkForEachLatch(pNtk, pObj, i) {
    pBo = Abc_ObjFanout0( pObj );
    pBi = Abc_ObjFanin0( pObj );

    Abc_NodeSetTravIdCurrent( pObj );
    Abc_FlowRetime_Dfs_back( pBi, vNodes );

    if (pBo->fMarkA) {
      pBo->fMarkA = 0;
      pObj->Level = pBo->Level;
      assert((int)pObj->Level <= pManMR->maxDelay);
    } else
      pObj->Level = 0;
  }

#if defined(DEBUG_CHECK)
  // DEBUG: check DFS ordering
  Vec_PtrForEachEntryReverse( Abc_Obj_t *,vNodes, pObj, i) {
    pObj->fMarkB = 1;
    
    Abc_ObjForEachFanout( pObj, pNext, j )
      if ( Abc_NodeIsTravIdCurrent(pNext) && !Abc_ObjIsLatch(pNext))
        assert(pNext->fMarkB);
  }
  Vec_PtrForEachEntryReverse( Abc_Obj_t *,vNodes, pObj, i)
    pObj->fMarkB = 0;
#endif

  // ... propagate values
  Vec_PtrForEachEntryReverse( Abc_Obj_t *,vNodes, pObj, i) {
    pObj->Level = 0;
    Abc_ObjForEachFanout( pObj, pNext, j )
    {
      l = pNext->Level + (Abc_ObjIsNode(pObj) ? 1 : 0);
      if ( Abc_NodeIsTravIdCurrent(pNext) &&
           (int)pObj->Level < l )
        pObj->Level = l;
    }

    if ((int)pObj->Level + (Abc_ObjIsNode(pObj)?1:0) > pManMR->maxDelay) {
      FSET(pObj, BLOCK);
    }
  }

  // 2. conservative constraints

  // first pass: seed latches with T=0
  Abc_NtkForEachLatch(pNtk, pObj, i) {
    pObj->Level = 0;
  }

  // ... propagate values
  Vec_PtrForEachEntryReverse( Abc_Obj_t *,vNodes, pObj, i) {
    pObj->Level = 0;
    Abc_ObjForEachFanout( pObj, pNext, j ) {
      l = pNext->Level + (Abc_ObjIsNode(pObj) ? 1 : 0);
      if ( Abc_NodeIsTravIdCurrent(pNext) &&
           (int)pObj->Level < l )
        pObj->Level = l;
    }

    if ( Abc_ObjIsBo(pObj) ) {
      pObj->fMarkA = 1;
    }
         
    assert((int)pObj->Level <= pManMR->maxDelay);
  }

  Abc_NtkForEachLatch(pNtk, pObj, i) {
    pBo = Abc_ObjFanout0( pObj );
    assert(Abc_ObjIsBo(pBo));
    pBi = Abc_ObjFanin0( pObj );
    assert(Abc_ObjIsBi(pBi));

    if (pBo->fMarkA) {
      pBo->fMarkA = 0;
      pObj->Level = pBo->Level;
    } else
      pObj->Level = 0;
  }

  // ... propagate values
  Vec_PtrForEachEntryReverse( Abc_Obj_t *,vNodes, pObj, i) {
    pObj->Level = 0;
    Abc_ObjForEachFanout( pObj, pNext, j ) {
      l = pNext->Level + (Abc_ObjIsNode(pObj) ? 1 : 0);
      if ( Abc_NodeIsTravIdCurrent(pNext) &&
           (int)pObj->Level < l )
        pObj->Level = l;
    }

    // constrained?
    if ((int)pObj->Level > pManMR->maxDelay) {
      FSET( pObj, CONSERVATIVE );
      pManMR->nConservConstraints++;
    } else
      FUNSET( pObj, CONSERVATIVE );
  }

  Vec_PtrClear( vNodes );
}


/**Function*************************************************************

  Synopsis    [Introduces exact timing constraints for a node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_ConstrainExact( Abc_Obj_t * pObj ) {

  if (FTEST( pObj, CONSERVATIVE )) {
    pManMR->nConservConstraints--;
    FUNSET( pObj, CONSERVATIVE );
  }

#if !defined(IGNORE_TIMING)
  if (pManMR->fIsForward) {
    Abc_FlowRetime_ConstrainExact_forw(pObj);
  } else {
    Abc_FlowRetime_ConstrainExact_back(pObj);
  }
#endif
}

void Abc_FlowRetime_ConstrainExact_forw_rec( Abc_Obj_t * pObj, Vec_Ptr_t *vNodes, int latch ) {
  Abc_Obj_t *pNext;
  int i;

  // terminate?
  if (Abc_ObjIsLatch(pObj)) {
    if (latch) return;
    latch = 1;
  }

  // already visited?
  if (!latch) {
    if (pObj->fMarkA) return;
    pObj->fMarkA = 1;
  } else {
    if (pObj->fMarkB) return;
    pObj->fMarkB = 1;
  }

  // recurse
  Abc_ObjForEachFanin(pObj, pNext, i) {
    Abc_FlowRetime_ConstrainExact_forw_rec( pNext, vNodes, latch );
  }

  // add
  pObj->Level = 0;
  Vec_PtrPush(vNodes, Abc_ObjNotCond(pObj, latch));
}

void Abc_FlowRetime_ConstrainExact_forw( Abc_Obj_t * pObj ) {
  Vec_Ptr_t *vNodes = pManMR->vNodes;
  Abc_Obj_t *pNext, *pCur, *pReg;
  // Abc_Ntk_t *pNtk = pManMR->pNtk;
  int i, j;

  assert( !Vec_PtrSize(vNodes) );
  assert( !Abc_ObjIsLatch(pObj) );
  assert( !Vec_PtrSize( FTIMEEDGES(pObj) ));
  Vec_PtrPush( pManMR->vExactNodes, pObj );

  // rev topo order
  Abc_FlowRetime_ConstrainExact_forw_rec( pObj, vNodes, 0 );

  Vec_PtrForEachEntryReverse( Abc_Obj_t *, vNodes, pCur, i) {
    pReg = Abc_ObjRegular( pCur );

    if (pReg == pCur) {
      assert(!Abc_ObjIsLatch(pReg));
      Abc_ObjForEachFanin(pReg, pNext, j)
        pNext->Level = MAX( pNext->Level, pReg->Level + (Abc_ObjIsNode(pReg)?1:0));
      assert((int)pReg->Level <= pManMR->maxDelay);
      pReg->Level = 0;
      pReg->fMarkA = pReg->fMarkB = 0;
    }
  }
  Vec_PtrForEachEntryReverse( Abc_Obj_t *, vNodes, pCur, i) {
    pReg = Abc_ObjRegular( pCur );
    if (pReg != pCur) {
      Abc_ObjForEachFanin(pReg, pNext, j)
        if (!Abc_ObjIsLatch(pNext))
          pNext->Level = MAX( pNext->Level, pReg->Level + (Abc_ObjIsNode(pReg)?1:0));

      if ((int)pReg->Level == pManMR->maxDelay) {
        Vec_PtrPush( FTIMEEDGES(pObj), pReg);
        pManMR->nExactConstraints++;
      }
      pReg->Level = 0;
      pReg->fMarkA = pReg->fMarkB = 0;
    }
  }

  Vec_PtrClear( vNodes );
}

void Abc_FlowRetime_ConstrainExact_back_rec( Abc_Obj_t * pObj, Vec_Ptr_t *vNodes, int latch ) {
  Abc_Obj_t *pNext;
  int i;

  // terminate?
  if (Abc_ObjIsLatch(pObj)) {
    if (latch) return;
    latch = 1;
  }

  // already visited?
  if (!latch) {
    if (pObj->fMarkA) return;
    pObj->fMarkA = 1;
  } else {
    if (pObj->fMarkB) return;
    pObj->fMarkB = 1;
  }

  // recurse
  Abc_ObjForEachFanout(pObj, pNext, i) {
    Abc_FlowRetime_ConstrainExact_back_rec( pNext, vNodes, latch );
  }

  // add
  pObj->Level = 0;
  Vec_PtrPush(vNodes, Abc_ObjNotCond(pObj, latch));
}


void Abc_FlowRetime_ConstrainExact_back( Abc_Obj_t * pObj ) {
  Vec_Ptr_t *vNodes = pManMR->vNodes;
  Abc_Obj_t *pNext, *pCur, *pReg;
  // Abc_Ntk_t *pNtk = pManMR->pNtk;
  int i, j;

  assert( !Vec_PtrSize( vNodes ));
  assert( !Abc_ObjIsLatch(pObj) );
  assert( !Vec_PtrSize( FTIMEEDGES(pObj) ));
  Vec_PtrPush( pManMR->vExactNodes, pObj );

  // rev topo order
  Abc_FlowRetime_ConstrainExact_back_rec( pObj, vNodes, 0 );

  Vec_PtrForEachEntryReverse( Abc_Obj_t *, vNodes, pCur, i) {
    pReg = Abc_ObjRegular( pCur );

    if (pReg == pCur) {
      assert(!Abc_ObjIsLatch(pReg));
      Abc_ObjForEachFanout(pReg, pNext, j)
        pNext->Level = MAX( pNext->Level, pReg->Level + (Abc_ObjIsNode(pReg)?1:0));
      assert((int)pReg->Level <= pManMR->maxDelay);
      pReg->Level = 0;
      pReg->fMarkA = pReg->fMarkB = 0;
    }
  }
  Vec_PtrForEachEntryReverse( Abc_Obj_t *, vNodes, pCur, i) {
    pReg = Abc_ObjRegular( pCur );
    if (pReg != pCur) {
      Abc_ObjForEachFanout(pReg, pNext, j)
        if (!Abc_ObjIsLatch(pNext))
          pNext->Level = MAX( pNext->Level, pReg->Level + (Abc_ObjIsNode(pReg)?1:0));

      if ((int)pReg->Level == pManMR->maxDelay) {
        Vec_PtrPush( FTIMEEDGES(pObj), pReg);
        pManMR->nExactConstraints++;
      }
      pReg->Level = 0;
      pReg->fMarkA = pReg->fMarkB = 0;
    }
  }

  Vec_PtrClear( vNodes );
}


/**Function*************************************************************

  Synopsis    [Introduces all exact timing constraints in a network]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_ConstrainExactAll( Abc_Ntk_t * pNtk ) {
  int i;
  Abc_Obj_t *pObj;
  void *pArray;
  
  // free existing constraints
  Abc_NtkForEachObj( pNtk, pObj, i )    
    if ( Vec_PtrSize( FTIMEEDGES(pObj) )) {
      pArray =  Vec_PtrReleaseArray( FTIMEEDGES(pObj) );
      ABC_FREE( pArray );
    }
  pManMR->nExactConstraints = 0;
  
  // generate all constraints
  Abc_NtkForEachObj(pNtk, pObj, i)
    if (!Abc_ObjIsLatch(pObj) && FTEST( pObj, CONSERVATIVE ) && !FTEST( pObj, BLOCK ))
      if (!Vec_PtrSize( FTIMEEDGES( pObj ) ))
        Abc_FlowRetime_ConstrainExact( pObj );
}



/**Function*************************************************************

  Synopsis    [Deallocates exact constraints.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_FreeTiming( Abc_Ntk_t *pNtk ) {
  Abc_Obj_t *pObj;
  void *pArray;

  while( Vec_PtrSize( pManMR->vExactNodes )) {
    pObj = (Abc_Obj_t*)Vec_PtrPop( pManMR->vExactNodes );
    
    if ( Vec_PtrSize( FTIMEEDGES(pObj) )) {
      pArray =  Vec_PtrReleaseArray( FTIMEEDGES(pObj) );
      ABC_FREE( pArray );
    }
  }

  Vec_PtrFree(pManMR->vExactNodes);
  ABC_FREE( pManMR->vTimeEdges );
}


/**Function*************************************************************

  Synopsis    [DFS order.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_FlowRetime_Dfs_forw( Abc_Obj_t * pObj, Vec_Ptr_t *vNodes ) {
  Abc_Obj_t *pNext;
  int i;

  if (Abc_ObjIsLatch(pObj)) return;

  Abc_NodeSetTravIdCurrent( pObj );
  
  Abc_ObjForEachFanout( pObj, pNext, i )
    if (!Abc_NodeIsTravIdCurrent( pNext ))
      Abc_FlowRetime_Dfs_forw( pNext, vNodes );

  Vec_PtrPush( vNodes, pObj );
}


void Abc_FlowRetime_Dfs_back( Abc_Obj_t * pObj, Vec_Ptr_t *vNodes ) {
  Abc_Obj_t *pNext;
  int i;

  if (Abc_ObjIsLatch(pObj)) return;

  Abc_NodeSetTravIdCurrent( pObj );
  
  Abc_ObjForEachFanin( pObj, pNext, i )
    if (!Abc_NodeIsTravIdCurrent( pNext ))
      Abc_FlowRetime_Dfs_back( pNext, vNodes );

  Vec_PtrPush( vNodes, pObj );
}


/**Function*************************************************************

  Synopsis    [Main timing-constrained routine.]

  Description [Refines constraints that are limiting area improvement.
               These are identified by computing
               the min-cuts both with and without the conservative
               constraints: these two situation represent an
               over- and under-constrained version of the timing.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_FlowRetime_RefineConstraints( ) {
  Abc_Ntk_t *pNtk = pManMR->pNtk;
  int i, flow, count = 0;
  Abc_Obj_t *pObj;
  int maxTighten = 99999;

  vprintf("\t\tsubiter %d : constraints = {cons, exact} = %d, %d\n", 
         pManMR->subIteration, pManMR->nConservConstraints, pManMR->nExactConstraints);

  // 1. overconstrained
  pManMR->constraintMask = BLOCK | CONSERVATIVE;
  vprintf("\t\trefinement: over ");
  fflush(stdout);
  flow = Abc_FlowRetime_PushFlows( pNtk, 0 );
  vprintf("= %d ", flow);

  // remember nodes
  if (pManMR->fIsForward) {
    Abc_NtkForEachObj( pNtk, pObj, i )
      if (!FTEST(pObj, VISITED_R))
        pObj->fMarkC = 1;
  } else {
    Abc_NtkForEachObj( pNtk, pObj, i )
      if (!FTEST(pObj, VISITED_E))
        pObj->fMarkC = 1;
  }

  if (pManMR->fConservTimingOnly) {
    vprintf(" done\n");
    return 0;
  }

  // 2. underconstrained
  pManMR->constraintMask = BLOCK;
  Abc_FlowRetime_ClearFlows( 0 );
  vprintf("under = ");
  fflush(stdout);
  flow = Abc_FlowRetime_PushFlows( pNtk, 0 );
  vprintf("%d refined nodes = ", flow);
  fflush(stdout);

  // find area-limiting constraints
  if (pManMR->fIsForward) {
    Abc_NtkForEachObj( pNtk, pObj, i ) {
      if (pObj->fMarkC &&
          FTEST(pObj, VISITED_R) &&
          FTEST(pObj, CONSERVATIVE) && 
          count < maxTighten) {
        count++;
        Abc_FlowRetime_ConstrainExact( pObj );
      }
      pObj->fMarkC = 0;
    }
  } else {
    Abc_NtkForEachObj( pNtk, pObj, i ) {
      if (pObj->fMarkC &&
          FTEST(pObj, VISITED_E) &&
          FTEST(pObj, CONSERVATIVE) &&
          count < maxTighten) {
        count++;
        Abc_FlowRetime_ConstrainExact( pObj );
      }
      pObj->fMarkC = 0;
    }
  }
  
  vprintf("%d\n", count);
  
  return (count > 0);
}


ABC_NAMESPACE_IMPL_END

