/**CFile****************************************************************

  FileName    [fretFlow.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Flow-based retiming package.]

  Synopsis    [Max-flow computation.]

  Author      [Aaron Hurst]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - January 1, 2008.]

  Revision    [$Id: fretFlow.c,v 1.00 2008/01/01 00:00:00 ahurst Exp $]

***********************************************************************/

#include "fretime.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void dfsfast_e_retreat( Abc_Obj_t *pObj );
static void dfsfast_r_retreat( Abc_Obj_t *pObj );

#define FDIST(xn, xe, yn, ye) (FDATA(xn)->xe##_dist == (FDATA(yn)->ye##_dist + 1))

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Fast DFS.]

  Description [Uses sink-distance-histogram heuristic.  May not find all
               flow paths: this occurs in a small number of cases where
               the flow predecessor points to a non-adjacent node and
               the distance ordering is perturbed.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

void dfsfast_preorder( Abc_Ntk_t *pNtk ) {
  Abc_Obj_t *pObj, *pNext;
  Vec_Ptr_t *vTimeIn, *qn = Vec_PtrAlloc(Abc_NtkObjNum(pNtk));
  Vec_Int_t *qe = Vec_IntAlloc(Abc_NtkObjNum(pNtk));
  int i, j, d = 0, end;
  int qpos = 0;

  // create reverse timing edges for backward traversal
#if !defined(IGNORE_TIMING)
  if (pManMR->maxDelay) {
    Abc_NtkForEachObj( pNtk, pObj, i ) {
      Vec_PtrForEachEntry( Abc_Obj_t *, FTIMEEDGES(pObj), pNext, j ) {
        vTimeIn = FDATA(pNext)->vNodes;
        if (!vTimeIn) {
          vTimeIn = FDATA(pNext)->vNodes = Vec_PtrAlloc(2);
        }
        Vec_PtrPush(vTimeIn, pObj);
      }
    }
  }
#endif

  // clear histogram
  assert(pManMR->vSinkDistHist);
  memset(Vec_IntArray(pManMR->vSinkDistHist), 0, sizeof(int)*Vec_IntSize(pManMR->vSinkDistHist));

  // seed queue : latches, PIOs, and blocks
  Abc_NtkForEachObj( pNtk, pObj, i )
    if (Abc_ObjIsPo(pObj) ||
        Abc_ObjIsLatch(pObj) || 
        (pManMR->fIsForward && FTEST(pObj, BLOCK_OR_CONS) & pManMR->constraintMask)) {
      Vec_PtrPush(qn, pObj);
      Vec_IntPush(qe, 'r');
      FDATA(pObj)->r_dist = 1;
    } else if (Abc_ObjIsPi(pObj) || 
               (!pManMR->fIsForward && FTEST(pObj, BLOCK_OR_CONS) & pManMR->constraintMask)) {
      Vec_PtrPush(qn, pObj);
      Vec_IntPush(qe, 'e');
      FDATA(pObj)->e_dist = 1;
    }

  // until queue is empty...
  while(qpos < Vec_PtrSize(qn)) {
    pObj = (Abc_Obj_t *)Vec_PtrEntry(qn, qpos);
    assert(pObj);
    end = Vec_IntEntry(qe, qpos);
    qpos++;
    
    if (end == 'r') {
      d = FDATA(pObj)->r_dist;

      // 1. structural edges
      if (pManMR->fIsForward) {
        Abc_ObjForEachFanin( pObj, pNext, i )
          if (!FDATA(pNext)->e_dist) {
            FDATA(pNext)->e_dist = d+1;
            Vec_PtrPush(qn, pNext);
            Vec_IntPush(qe, 'e');
          }
      } else
        Abc_ObjForEachFanout( pObj, pNext, i )
          if (!FDATA(pNext)->e_dist) {
            FDATA(pNext)->e_dist = d+1;
            Vec_PtrPush(qn, pNext);
            Vec_IntPush(qe, 'e');
          }

      if (d == 1) continue;

      // 2. reverse edges (forward retiming only)
      if (pManMR->fIsForward) {
        Abc_ObjForEachFanout( pObj, pNext, i )
          if (!FDATA(pNext)->r_dist && !Abc_ObjIsLatch(pNext)) {
            FDATA(pNext)->r_dist = d+1;
            Vec_PtrPush(qn, pNext);
            Vec_IntPush(qe, 'r');
          }          

      // 3. timimg edges (forward retiming only)
#if !defined(IGNORE_TIMING)
        if (pManMR->maxDelay && FDATA(pObj)->vNodes)
          Vec_PtrForEachEntry(Abc_Obj_t *, FDATA(pObj)->vNodes, pNext, i ) {
            if (!FDATA(pNext)->r_dist) {
              FDATA(pNext)->r_dist = d+1;
              Vec_PtrPush(qn, pNext);
              Vec_IntPush(qe, 'r');
            }
          }
#endif
      }
      
    } else { // if 'e'
      if (Abc_ObjIsLatch(pObj)) continue;

      d = FDATA(pObj)->e_dist;

      // 1. through node
      if (!FDATA(pObj)->r_dist) {
        FDATA(pObj)->r_dist = d+1;
        Vec_PtrPush(qn, pObj);
        Vec_IntPush(qe, 'r');
      }

      // 2. reverse edges (backward retiming only)
      if (!pManMR->fIsForward) {
        Abc_ObjForEachFanin( pObj, pNext, i )
          if (!FDATA(pNext)->e_dist && !Abc_ObjIsLatch(pNext)) {
            FDATA(pNext)->e_dist = d+1;
            Vec_PtrPush(qn, pNext);
            Vec_IntPush(qe, 'e');
          }  

      // 3. timimg edges (backward retiming only)
#if !defined(IGNORE_TIMING)
        if (pManMR->maxDelay && FDATA(pObj)->vNodes)
          Vec_PtrForEachEntry(Abc_Obj_t *, FDATA(pObj)->vNodes, pNext, i ) {
            if (!FDATA(pNext)->e_dist) {
              FDATA(pNext)->e_dist = d+1;
              Vec_PtrPush(qn, pNext);
              Vec_IntPush(qe, 'e');
            }
          }
#endif
      }
    }
  }
 
  // free time edges
#if !defined(IGNORE_TIMING)
  if (pManMR->maxDelay) {
    Abc_NtkForEachObj( pNtk, pObj, i ) {
      vTimeIn = FDATA(pObj)->vNodes;
      if (vTimeIn) {
        Vec_PtrFree(vTimeIn);
        FDATA(pObj)->vNodes = 0;
      }
    }
  }
#endif

  Abc_NtkForEachObj( pNtk, pObj, i ) {
    Vec_IntAddToEntry(pManMR->vSinkDistHist, FDATA(pObj)->r_dist, 1);
    Vec_IntAddToEntry(pManMR->vSinkDistHist, FDATA(pObj)->e_dist, 1);

#ifdef DEBUG_PREORDER
    printf("node %d\t: r=%d\te=%d\n", Abc_ObjId(pObj), FDATA(pObj)->r_dist, FDATA(pObj)->e_dist);
#endif
  }

  // printf("\t\tpre-ordered (max depth=%d)\n", d+1);

  // deallocate
  Vec_PtrFree( qn );
  Vec_IntFree( qe );
}

int dfsfast_e( Abc_Obj_t *pObj, Abc_Obj_t *pPred ) {
  int i;
  Abc_Obj_t *pNext;
  
  if (pManMR->fSinkDistTerminate) return 0;

  // have we reached the sink?
  if(FTEST(pObj, BLOCK_OR_CONS) & pManMR->constraintMask ||
     Abc_ObjIsPi(pObj)) {
    assert(pPred);
    assert(!pManMR->fIsForward);
    return 1;
  }

  FSET(pObj, VISITED_E);

#ifdef DEBUG_VISITED
  printf("(%de=%d) ", Abc_ObjId(pObj), FDATA(pObj)->e_dist);
#endif  

  // 1. structural edges
  if (pManMR->fIsForward)
    Abc_ObjForEachFanout( pObj, pNext, i ) {
      if (!FTEST(pNext, VISITED_R) &&
          FDIST(pObj, e, pNext, r) &&
          dfsfast_r(pNext, pPred)) {
#ifdef DEBUG_PRINT_FLOWS
        printf("o");
#endif
        goto found;
      }
    }
  else
    Abc_ObjForEachFanin( pObj, pNext, i ) {
      if (!FTEST(pNext, VISITED_R) &&
          FDIST(pObj, e, pNext, r) &&
          dfsfast_r(pNext, pPred)) {
#ifdef DEBUG_PRINT_FLOWS
        printf("o");
#endif
        goto found;
      }
    }  
  
  if (Abc_ObjIsLatch(pObj))
    goto not_found;

  // 2. reverse edges (backward retiming only)
  if (!pManMR->fIsForward) { 
    Abc_ObjForEachFanout( pObj, pNext, i ) {
      if (!FTEST(pNext, VISITED_E) &&
          FDIST(pObj, e, pNext, e) &&
          dfsfast_e(pNext, pPred)) {
#ifdef DEBUG_PRINT_FLOWS
        printf("i");
#endif
        goto found;
      }
    }

  // 3. timing edges (backward retiming only)
#if !defined(IGNORE_TIMING)
    if (pManMR->maxDelay) 
      Vec_PtrForEachEntry(Abc_Obj_t *, FTIMEEDGES(pObj), pNext, i) {
        if (!FTEST(pNext, VISITED_E) &&
            FDIST(pObj, e, pNext, e) &&
            dfsfast_e(pNext, pPred)) {
#ifdef DEBUG_PRINT_FLOWS
          printf("o");
#endif
          goto found;
        }
      }
#endif
  }
  
  // unwind
  if (FTEST(pObj, FLOW) && 
      !FTEST(pObj, VISITED_R) &&
      FDIST(pObj, e, pObj, r) &&
      dfsfast_r(pObj, FGETPRED(pObj))) {

    FUNSET(pObj, FLOW);
    FSETPRED(pObj, NULL);
#ifdef DEBUG_PRINT_FLOWS
    printf("u");
#endif
    goto found;
  }
  
 not_found:
  FUNSET(pObj, VISITED_E);
  dfsfast_e_retreat(pObj);
  return 0;
  
 found:
#ifdef DEBUG_PRINT_FLOWS
  printf("%d ", Abc_ObjId(pObj));
#endif 
  FUNSET(pObj, VISITED_E);
  return 1;
}

int dfsfast_r( Abc_Obj_t *pObj, Abc_Obj_t *pPred ) {
  int i;
  Abc_Obj_t *pNext, *pOldPred;

  if (pManMR->fSinkDistTerminate) return 0;

#ifdef DEBUG_VISITED
  printf("(%dr=%d) ", Abc_ObjId(pObj), FDATA(pObj)->r_dist);
#endif  

  // have we reached the sink?
  if (Abc_ObjIsLatch(pObj) ||
      (pManMR->fIsForward && Abc_ObjIsPo(pObj)) || 
      (pManMR->fIsForward && FTEST(pObj, BLOCK_OR_CONS) & pManMR->constraintMask)) {
    assert(pPred);
    return 1;
  }

  FSET(pObj, VISITED_R);

  if (FTEST(pObj, FLOW)) {

    pOldPred = FGETPRED(pObj);
    if (pOldPred && 
        !FTEST(pOldPred, VISITED_E) &&
        FDIST(pObj, r, pOldPred, e) &&
        dfsfast_e(pOldPred, pOldPred)) {

      FSETPRED(pObj, pPred);

#ifdef DEBUG_PRINT_FLOWS
      printf("fr");
#endif
      goto found;
    }
    
  } else {

    if (!FTEST(pObj, VISITED_E) &&
        FDIST(pObj, r, pObj, e) &&
        dfsfast_e(pObj, pObj)) {

      FSET(pObj, FLOW);
      FSETPRED(pObj, pPred);

#ifdef DEBUG_PRINT_FLOWS
      printf("f");
#endif
      goto found;
    }
  }
 
  // 2. reverse edges (forward retiming only)
  if (pManMR->fIsForward) {
    Abc_ObjForEachFanin( pObj, pNext, i ) {
      if (!FTEST(pNext, VISITED_R) &&
          FDIST(pObj, r, pNext, r) &&
          !Abc_ObjIsLatch(pNext) &&
          dfsfast_r(pNext, pPred)) {
#ifdef DEBUG_PRINT_FLOWS
        printf("i");
#endif
        goto found;
      }
    }
  
  // 3. timing edges (forward retiming only)
#if !defined(IGNORE_TIMING)
    if (pManMR->maxDelay) 
      Vec_PtrForEachEntry(Abc_Obj_t*, FTIMEEDGES(pObj), pNext, i) {
        if (!FTEST(pNext, VISITED_R) &&
            FDIST(pObj, r, pNext, r) &&
            dfsfast_r(pNext, pPred)) {
#ifdef DEBUG_PRINT_FLOWS
          printf("o");
#endif
          goto found;
        }
      }
#endif
  }
  
  FUNSET(pObj, VISITED_R);
  dfsfast_r_retreat(pObj);
  return 0;

 found:
#ifdef DEBUG_PRINT_FLOWS
  printf("%d ", Abc_ObjId(pObj));
#endif
  FUNSET(pObj, VISITED_R);
  return 1;
}

void
dfsfast_e_retreat(Abc_Obj_t *pObj) {
  Abc_Obj_t *pNext;
  int i, *h;
  int old_dist = FDATA(pObj)->e_dist;
  int adj_dist, min_dist = MAX_DIST;
  
  // 1. structural edges
  if (pManMR->fIsForward)
    Abc_ObjForEachFanout( pObj, pNext, i ) {
      adj_dist = FDATA(pNext)->r_dist;
      if (adj_dist) min_dist = MIN(min_dist, adj_dist);
    }
  else
    Abc_ObjForEachFanin( pObj, pNext, i ) {
      adj_dist = FDATA(pNext)->r_dist;
      if (adj_dist) min_dist = MIN(min_dist, adj_dist);
    }

  if (Abc_ObjIsLatch(pObj)) goto update;

  // 2. through
  if (FTEST(pObj, FLOW)) {
    adj_dist = FDATA(pObj)->r_dist;
    if (adj_dist) min_dist = MIN(min_dist, adj_dist);
  }

  // 3. reverse edges (backward retiming only)
  if (!pManMR->fIsForward) {
    Abc_ObjForEachFanout( pObj, pNext, i ) {
      adj_dist = FDATA(pNext)->e_dist;
      if (adj_dist) min_dist = MIN(min_dist, adj_dist);
    }

  // 4. timing edges (backward retiming only)
#if !defined(IGNORE_TIMING)
    if (pManMR->maxDelay) 
      Vec_PtrForEachEntry(Abc_Obj_t*, FTIMEEDGES(pObj), pNext, i) {
        adj_dist = FDATA(pNext)->e_dist;
        if (adj_dist) min_dist = MIN(min_dist, adj_dist);
      }
#endif
  }

 update:
  ++min_dist;
  if (min_dist >= MAX_DIST) min_dist = 0;
  // printf("[%de=%d->%d] ", Abc_ObjId(pObj), old_dist, min_dist+1);
  FDATA(pObj)->e_dist = min_dist;

  assert(min_dist < Vec_IntSize(pManMR->vSinkDistHist));
  h = Vec_IntArray(pManMR->vSinkDistHist);
  h[old_dist]--;
  h[min_dist]++;
  if (!h[old_dist]) {
    pManMR->fSinkDistTerminate = 1;
  }
}

void
dfsfast_r_retreat(Abc_Obj_t *pObj) {
  Abc_Obj_t *pNext;
  int i, *h;
  int old_dist = FDATA(pObj)->r_dist;
  int adj_dist, min_dist = MAX_DIST;
  
  // 1. through or pred
  if (FTEST(pObj, FLOW)) {
    if (FGETPRED(pObj)) {
      adj_dist = FDATA(FGETPRED(pObj))->e_dist;
      if (adj_dist) min_dist = MIN(min_dist, adj_dist);
    }
  } else {
    adj_dist = FDATA(pObj)->e_dist;
    if (adj_dist) min_dist = MIN(min_dist, adj_dist);
  }

  // 2. reverse edges (forward retiming only)
  if (pManMR->fIsForward) {
    Abc_ObjForEachFanin( pObj, pNext, i )
      if (!Abc_ObjIsLatch(pNext)) {
        adj_dist = FDATA(pNext)->r_dist;
        if (adj_dist) min_dist = MIN(min_dist, adj_dist);
      }

  // 3. timing edges (forward retiming only)
#if !defined(IGNORE_TIMING)
    if (pManMR->maxDelay) 
      Vec_PtrForEachEntry(Abc_Obj_t*, FTIMEEDGES(pObj), pNext, i) {
        adj_dist = FDATA(pNext)->r_dist;
        if (adj_dist) min_dist = MIN(min_dist, adj_dist);
      }
#endif
  }

  ++min_dist;
  if (min_dist >= MAX_DIST) min_dist = 0;
  //printf("[%dr=%d->%d] ", Abc_ObjId(pObj), old_dist, min_dist+1);
  FDATA(pObj)->r_dist = min_dist;

  assert(min_dist < Vec_IntSize(pManMR->vSinkDistHist));
  h = Vec_IntArray(pManMR->vSinkDistHist);
  h[old_dist]--;
  h[min_dist]++;
  if (!h[old_dist]) {
    pManMR->fSinkDistTerminate = 1;
  }
}

/**Function*************************************************************

  Synopsis    [Plain DFS.]

  Description [Does not use sink-distance-histogram heuristic.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/

int dfsplain_e( Abc_Obj_t *pObj, Abc_Obj_t *pPred ) {
  int i;
  Abc_Obj_t *pNext;
  
  if (FTEST(pObj, BLOCK_OR_CONS) & pManMR->constraintMask || 
      Abc_ObjIsPi(pObj)) {
    assert(pPred);
    assert(!pManMR->fIsForward);
    return 1;
  }

  FSET(pObj, VISITED_E);
  
  // printf(" %de\n", Abc_ObjId(pObj));
  
  // 1. structural edges
  if (pManMR->fIsForward)
    Abc_ObjForEachFanout( pObj, pNext, i ) {
      if (!FTEST(pNext, VISITED_R) &&
          dfsplain_r(pNext, pPred)) {
#ifdef DEBUG_PRINT_FLOWS
        printf("o");
#endif
        goto found;
      }
    }
  else
    Abc_ObjForEachFanin( pObj, pNext, i ) {
      if (!FTEST(pNext, VISITED_R) &&
          dfsplain_r(pNext, pPred)) {
#ifdef DEBUG_PRINT_FLOWS
        printf("o");
#endif
        goto found;
      }
    }
  
  if (Abc_ObjIsLatch(pObj))
    return 0;

  // 2. reverse edges (backward retiming only)
  if (!pManMR->fIsForward) {
    Abc_ObjForEachFanout( pObj, pNext, i ) {
      if (!FTEST(pNext, VISITED_E) &&
          dfsplain_e(pNext, pPred)) {
#ifdef DEBUG_PRINT_FLOWS
        printf("i");
#endif
        goto found;
      }
    }

  // 3. timing edges (backward retiming only)
#if !defined(IGNORE_TIMING)
    if (pManMR->maxDelay) 
      Vec_PtrForEachEntry(Abc_Obj_t*, FTIMEEDGES(pObj), pNext, i) {
        if (!FTEST(pNext, VISITED_E) &&
            dfsplain_e(pNext, pPred)) {
#ifdef DEBUG_PRINT_FLOWS
          printf("o");
#endif
          goto found;
        }
      }
#endif
  }
  
  // unwind
  if (FTEST(pObj, FLOW) && 
      !FTEST(pObj, VISITED_R) &&
      dfsplain_r(pObj, FGETPRED(pObj))) {
    FUNSET(pObj, FLOW);
    FSETPRED(pObj, NULL);
#ifdef DEBUG_PRINT_FLOWS
    printf("u");
#endif
    goto found;
  }
  
  return 0;
  
 found:
#ifdef DEBUG_PRINT_FLOWS
  printf("%d ", Abc_ObjId(pObj));
#endif

  return 1;
}

int dfsplain_r( Abc_Obj_t *pObj, Abc_Obj_t *pPred ) {
  int i;
  Abc_Obj_t *pNext, *pOldPred;

  // have we reached the sink?
  if (Abc_ObjIsLatch(pObj) || 
      (pManMR->fIsForward && Abc_ObjIsPo(pObj)) || 
      (pManMR->fIsForward && FTEST(pObj, BLOCK_OR_CONS) & pManMR->constraintMask)) {
    assert(pPred);
    return 1;
  }

  FSET(pObj, VISITED_R);

  // printf(" %dr\n", Abc_ObjId(pObj));

  if (FTEST(pObj, FLOW)) {

    pOldPred = FGETPRED(pObj);
    if (pOldPred && 
        !FTEST(pOldPred, VISITED_E) &&
        dfsplain_e(pOldPred, pOldPred)) {

      FSETPRED(pObj, pPred);

#ifdef DEBUG_PRINT_FLOWS
      printf("fr");
#endif
      goto found;
    }
    
  } else {

    if (!FTEST(pObj, VISITED_E) &&
        dfsplain_e(pObj, pObj)) {

      FSET(pObj, FLOW);
      FSETPRED(pObj, pPred);

#ifdef DEBUG_PRINT_FLOWS
      printf("f");
#endif
      goto found;
    }
  }
 
  // 2. follow reverse edges
  if (pManMR->fIsForward) { // forward retiming only
    Abc_ObjForEachFanin( pObj, pNext, i ) {
      if (!FTEST(pNext, VISITED_R) &&
          !Abc_ObjIsLatch(pNext) &&
          dfsplain_r(pNext, pPred)) {
#ifdef DEBUG_PRINT_FLOWS
        printf("i");
#endif
        goto found;
      }
    }
  
  // 3. timing edges (forward only)
#if !defined(IGNORE_TIMING)
    if (pManMR->maxDelay) 
      Vec_PtrForEachEntry(Abc_Obj_t*, FTIMEEDGES(pObj), pNext, i) {
        if (!FTEST(pNext, VISITED_R) &&
            dfsplain_r(pNext, pPred)) {
#ifdef DEBUG_PRINT_FLOWS
          printf("o");
#endif
          goto found;
        }
      }
#endif
  }
  
  return 0;

 found:
#ifdef DEBUG_PRINT_FLOWS
  printf("%d ", Abc_ObjId(pObj));
#endif
  return 1;
}
ABC_NAMESPACE_IMPL_END

