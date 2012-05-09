/**CFile****************************************************************

  FileName    [seqMaxMeanCycle.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [Efficient computation of maximum mean cycle times.]

  Author      [Aaron P. Hurst]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 15, 2006.]

  Revision    [$Id: seqMaxMeanCycle.c,v 1.00 2005/05/15 00:00:00 ahurst Exp $]

***********************************************************************/

#include "seqInt.h"
#include "hash.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

struct Abc_ManTime_t_
{
    Abc_Time_t     tArrDef;
    Abc_Time_t     tReqDef;
    Vec_Ptr_t  *   vArrs;
    Vec_Ptr_t  *   vReqs;
};

typedef struct Seq_HowardData_t_
{
  char   visited;
  int    mark;
  int    policy;
  float  cycle;
  float  skew;
  float  delay;
} Seq_HowardData_t;

// accessing the arrival and required times of a node
static inline Abc_Time_t * Abc_NodeArrival( Abc_Obj_t * pNode )  {  return pNode->pNtk->pManTime->vArrs->pArray[pNode->Id];  }
static inline Abc_Time_t * Abc_NodeRequired( Abc_Obj_t * pNode ) {  return pNode->pNtk->pManTime->vReqs->pArray[pNode->Id];  }

Hash_Ptr_t * Seq_NtkPathDelays( Abc_Ntk_t * pNtk, int fVerbose );
void Seq_NtkMergePios( Abc_Ntk_t * pNtk, Hash_Ptr_t * hFwdDelays, int fVerbose );

void Seq_NtkHowardLoop( Abc_Ntk_t * pNtk, Hash_Ptr_t * hFwdDelays,
                        Hash_Ptr_t * hNodeData, int node,
                        int *howardDepth, float *howardDelay, int *howardSink,
                        float *maxMeanCycle);
void Abc_NtkDfsReverse_rec2( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes, Vec_Ptr_t * vEndpoints );

#define Seq_NtkGetPathDelay( hFwdDelays, from, to ) \
  (Hash_PtrExists(hFwdDelays, from)?Hash_FltEntry( ((Hash_Flt_t *)Hash_PtrEntry(hFwdDelays, from, 0)), to, 0):0 )

#define HOWARD_EPSILON 1e-3
#define ZERO_SLOP      1e-5
#define REMOVE_ZERO_SLOP( x ) \
  (x = (x > -ZERO_SLOP && x < ZERO_SLOP)?0:x)

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Computes maximum mean cycle time.]

  Description [Uses Howard's algorithm.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
float Seq_NtkHoward( Abc_Ntk_t * pNtk, int fVerbose ) {

  Abc_Obj_t *  pObj;
  Hash_Ptr_t * hFwdDelays;
  Hash_Flt_t * hOutgoing;
  Hash_Ptr_Entry_t * pSourceEntry, * pNodeEntry;
  Hash_Flt_Entry_t * pSinkEntry;
  int          i, j, iteration = 0;
  int          source, sink;
  int          fChanged;
  int          howardDepth, howardSink = 0;
  float        delay, howardDelay, t;
  float        maxMeanCycle = -ABC_INFINITY;
  Hash_Ptr_t *       hNodeData;
  Seq_HowardData_t * pNodeData, * pSourceData, * pSinkData;

  // gather timing constraints
  hFwdDelays = Seq_NtkPathDelays( pNtk, fVerbose );
  Seq_NtkMergePios( pNtk, hFwdDelays, fVerbose );

  // initialize data, create initial policy
  hNodeData = Hash_PtrAlloc( hFwdDelays->nSize );
  Hash_PtrForEachEntry( hFwdDelays, pSourceEntry, i ) {
    Hash_PtrWriteEntry( hNodeData, pSourceEntry->key, 
                        (pNodeData = ALLOC(Seq_HowardData_t, 1)) );
    pNodeData->skew = 0.0;
    pNodeData->policy = 0;
    hOutgoing = (Hash_Flt_t *)(pSourceEntry->data);
    assert(hOutgoing);

    Hash_FltForEachEntry( hOutgoing, pSinkEntry, j ) {
      sink = pSinkEntry->key;
      delay = pSinkEntry->data;
       if (delay > pNodeData->skew) {
        pNodeData->policy = sink;
        pNodeData->skew = delay;
      }
    }
  }

  // iteratively refine policy
  do {
    iteration++;
    fChanged = 0;
    howardDelay = 0.0;
    howardDepth = 0;

    // reset data
    Hash_PtrForEachEntry( hNodeData, pNodeEntry, i ) {
      pNodeData = (Seq_HowardData_t *)pNodeEntry->data;
      pNodeData->skew = -ABC_INFINITY;
      pNodeData->cycle = -ABC_INFINITY;
      pNodeData->mark = 0;
      pNodeData->visited = 0;
    }

    // find loops in policy graph
    Hash_PtrForEachEntry( hNodeData, pNodeEntry, i ) {
      pNodeData = (Seq_HowardData_t *)(pNodeEntry->data);
      assert(pNodeData);
      if (!pNodeData->visited)
        Seq_NtkHowardLoop( pNtk, hFwdDelays, 
                           hNodeData, pNodeEntry->key,
                           &howardDepth, &howardDelay, &howardSink, &maxMeanCycle);
    }

    if (!howardSink) {
      return -1;
    }

    // improve policy by tightening loops
    Hash_PtrForEachEntry( hFwdDelays, pSourceEntry, i ) {
      source = pSourceEntry->key;
      pSourceData = (Seq_HowardData_t *)Hash_PtrEntry( hNodeData, source, 0 );
      assert(pSourceData);
      hOutgoing = (Hash_Flt_t *)(pSourceEntry->data);
      assert(hOutgoing);
      Hash_FltForEachEntry( hOutgoing, pSinkEntry, j ) {
        sink = pSinkEntry->key;
        pSinkData = (Seq_HowardData_t *)Hash_PtrEntry( hNodeData, sink, 0 );
        assert(pSinkData);
        delay = pSinkEntry->data;
        
        if (pSinkData->cycle > pSourceData->cycle + HOWARD_EPSILON) {
          fChanged = 1;
          pSourceData->cycle = pSinkData->cycle;
          pSourceData->policy = sink;
        }
      }
    }

    // improve policy by correcting skews
    if (!fChanged) {
      Hash_PtrForEachEntry( hFwdDelays, pSourceEntry, i ) {
        source = pSourceEntry->key;
        pSourceData = (Seq_HowardData_t *)Hash_PtrEntry( hNodeData, source, 0 );
        assert(pSourceData);
        hOutgoing = (Hash_Flt_t *)(pSourceEntry->data);
        assert(hOutgoing);
        Hash_FltForEachEntry( hOutgoing, pSinkEntry, j ) {
          sink = pSinkEntry->key;
          pSinkData = (Seq_HowardData_t *)Hash_PtrEntry( hNodeData, sink, 0 );
          assert(pSinkData);
          delay = pSinkEntry->data;

          if (pSinkData->cycle < 0.0 || pSinkData->cycle < pSourceData->cycle)
            continue;

          t = delay - pSinkData->cycle + pSinkData->skew;
          if (t > pSourceData->skew + HOWARD_EPSILON) {
            fChanged = 1;
            pSourceData->skew = t;
            pSourceData->policy = sink;
          }
        }
      }
    }

    if (fVerbose) printf("Iteration %d \t Period = %.2f\n", iteration, maxMeanCycle);
  } while (fChanged);

  // set global skew, mmct
  pNodeData = Hash_PtrEntry( hNodeData, -1, 0 );
  pNtk->globalSkew = -pNodeData->skew;
  pNtk->maxMeanCycle = maxMeanCycle;

  // set endpoint skews
  Vec_FltGrow( pNtk->vSkews, Abc_NtkLatchNum( pNtk ) );
  pNtk->vSkews->nSize =  Abc_NtkLatchNum( pNtk );
  Abc_NtkForEachLatch( pNtk, pObj, i ) {
    pNodeData = Hash_PtrEntry( hNodeData, pObj->Id, 0 );
    // skews are set based on latch # NOT id #
    Abc_NtkSetLatSkew( pNtk, i, pNodeData->skew );
  }

  // free node data
  Hash_PtrForEachEntry( hNodeData, pNodeEntry, i ) {
    pNodeData = (Seq_HowardData_t *)(pNodeEntry->data);
    FREE( pNodeData );
  }
  Hash_PtrFree(hNodeData);

  // free delay data
  Hash_PtrForEachEntry( hFwdDelays, pSourceEntry, i ) {
    Hash_FltFree( (Hash_Flt_t *)(pSourceEntry->data) );
  }
  Hash_PtrFree(hFwdDelays);

  return maxMeanCycle;
}

/**Function*************************************************************

  Synopsis    [Computes the mean cycle times of current policy graph.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkHowardLoop( Abc_Ntk_t * pNtk, Hash_Ptr_t * hFwdDelays,
                        Hash_Ptr_t * hNodeData, int node,
                        int *howardDepth, float *howardDelay, int *howardSink,
                        float *maxMeanCycle) {
  
  Seq_HowardData_t * pNodeData, *pToData;
  float delay, t;

  pNodeData = (Seq_HowardData_t *)Hash_PtrEntry( hNodeData, node, 0 );
  assert(pNodeData);
  pNodeData->visited = 1;
  pNodeData->mark = ++(*howardDepth);
  pNodeData->delay = (*howardDelay);
  if (pNodeData->policy) {
    pToData = (Seq_HowardData_t *)Hash_PtrEntry( hNodeData, pNodeData->policy, 0 );
    assert(pToData);
    delay = Seq_NtkGetPathDelay( hFwdDelays, node, pNodeData->policy );
    assert(delay > 0.0);
    (*howardDelay) += delay;
    if (pToData->mark) {
      t = (*howardDelay - pToData->delay) / (*howardDepth - pToData->mark + 1);
      pNodeData->cycle = t;
      pNodeData->skew = 0.0;
      if (*maxMeanCycle < t) {
        *maxMeanCycle = t;
        *howardSink = pNodeData->policy;
      }
    } else {
      if(!pToData->visited) {
        Seq_NtkHowardLoop(pNtk, hFwdDelays, hNodeData, pNodeData->policy,
                          howardDepth, howardDelay, howardSink, maxMeanCycle);
      }
      if(pToData->cycle > 0) {
        t = delay - pToData->cycle + pToData->skew;
	pNodeData->skew = t;
	pNodeData->cycle = pToData->cycle;
      }
    }
  }
  *howardDelay = pNodeData->delay;
  pNodeData->mark = 0;
  --(*howardDepth);
}

/**Function*************************************************************

  Synopsis    [Computes the register-to-register delays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Hash_Ptr_t * Seq_NtkPathDelays( Abc_Ntk_t * pNtk, int fVerbose ) {

  Abc_Time_t * pTime, ** ppTimes;
  Abc_Obj_t * pObj, * pDriver, * pStart, * pFanout;
  Vec_Ptr_t * vNodes, * vEndpoints;
  int i, j, nPaths = 0;
  Hash_Flt_t *  hOutgoing;
  Hash_Ptr_t *  hFwdDelays;
  float     nMaxPath = 0, nSumPath = 0;

  extern void Abc_NtkTimePrepare( Abc_Ntk_t * pNtk );
  extern void Abc_NodeDelayTraceArrival( Abc_Obj_t * pNode );

  if (fVerbose) printf("Gathering path delays...\n");

  hFwdDelays = Hash_PtrAlloc( Abc_NtkCiNum( pNtk ) );

  assert( Abc_NtkIsMappedLogic(pNtk) );

  Abc_NtkTimePrepare( pNtk );
  ppTimes = (Abc_Time_t **)pNtk->pManTime->vArrs->pArray;
  vNodes = Vec_PtrAlloc( 100 );
  vEndpoints = Vec_PtrAlloc( 100 );

  // set the initial times (i.e. ignore all inputs)
  Abc_NtkForEachObj( pNtk, pObj, i) {
    pTime = ppTimes[pObj->Id];
    pTime->Fall = pTime->Rise = pTime->Worst = -ABC_INFINITY;    
  }

  // starting at each Ci, compute timing forward
  Abc_NtkForEachCi( pNtk, pStart, j ) {
    
    hOutgoing = Hash_FltAlloc( 10 );
    Hash_PtrWriteEntry( hFwdDelays, pStart->Id, (void *)(hOutgoing) );

    // seed the starting point of interest
    pTime = ppTimes[pStart->Id];
    pTime->Fall = pTime->Rise = pTime->Worst = 0.0;

    // find a DFS ordering from the start
    Abc_NtkIncrementTravId( pNtk );
    Abc_NodeSetTravIdCurrent( pStart );
    pObj = Abc_ObjFanout0Ntk(pStart);
    Abc_ObjForEachFanout( pObj, pFanout, i )
      Abc_NtkDfsReverse_rec2( pFanout, vNodes, vEndpoints );
    if ( Abc_ObjIsCo( pStart ) )
      Vec_PtrPush( vEndpoints, pStart );

    // do timing analysis
    for ( i = vNodes->nSize-1; i >= 0; --i )
      Abc_NodeDelayTraceArrival( vNodes->pArray[i] );

    // there is a path to each set of Co endpoints
    Vec_PtrForEachEntry( vEndpoints, pObj, i )
    {
      assert(pObj);
      assert( Abc_ObjIsCo( pObj ) );
      pDriver = Abc_ObjFanin0(pObj);
      pTime   = Abc_NodeArrival(pDriver);
      if ( pTime->Worst > 0 ) {
        Hash_FltWriteEntry( hOutgoing, pObj->Id, pTime->Worst );
        nPaths++;
        // if (fVerbose) printf("\tpath %d,%d delay = %f\n", pStart->Id, pObj->Id, pTime->Worst);
        nSumPath += pTime->Worst;
        if (pTime->Worst > nMaxPath)
          nMaxPath = pTime->Worst;
      }
    }

    // clear the times that were altered
    for ( i = 0; i < vNodes->nSize; i++ ) {
      pObj = (Abc_Obj_t *)(vNodes->pArray[i]);
      pTime = ppTimes[pObj->Id];
      pTime->Fall = pTime->Rise = pTime->Worst = -ABC_INFINITY;
    }
    pTime = ppTimes[pStart->Id];
    pTime->Fall = pTime->Rise = pTime->Worst = -ABC_INFINITY;
    
    Vec_PtrClear( vNodes );
    Vec_PtrClear( vEndpoints );
  }

  Vec_PtrFree( vNodes );

  // rezero Cis (note: these should be restored to values if they were nonzero)
  Abc_NtkForEachCi( pNtk, pObj, i) {
    pTime = ppTimes[pObj->Id];
    pTime->Fall = pTime->Rise = pTime->Worst = 0.0;    
  }

  if (fVerbose) printf("Num. paths = %d\tMax. Path Delay = %.2f\tAvg. Path Delay = %.2f\n", nPaths, nMaxPath, nSumPath / nPaths);
  return hFwdDelays;
}


/**Function*************************************************************

  Synopsis    [Merges all the Pios together into one ID = -1.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkMergePios( Abc_Ntk_t * pNtk, Hash_Ptr_t * hFwdDelays, 
                       int fVerbose ) {

  Abc_Obj_t *        pObj;
  Hash_Flt_Entry_t * pSinkEntry;
  Hash_Ptr_Entry_t * pSourceEntry;
  Hash_Flt_t *       hOutgoing, * hPioSource;
  int                i, j;
  int                source, sink, nMerges = 0;
  float              delay = 0, max_delay = 0;
  Vec_Int_t *        vFreeList;

  vFreeList = Vec_IntAlloc( 10 );

  // create a new "-1" source entry for the Pios
  hPioSource = Hash_FltAlloc( 100 );
  Hash_PtrWriteEntry( hFwdDelays, -1, (void *)(hPioSource) );

  // merge all edges with a Pio as a source
  Abc_NtkForEachPi( pNtk, pObj, i ) {
    source = pObj->Id;
    hOutgoing = (Hash_Flt_t *)Hash_PtrEntry( hFwdDelays, source, 0 );
    if (!hOutgoing) continue;

    Hash_PtrForEachEntry( hOutgoing, pSinkEntry, j ) {
      nMerges++;
      sink = pSinkEntry->key;
      delay = pSinkEntry->data;
      if (Hash_FltEntry( hPioSource, sink, 1 ) < delay) {
        Hash_FltWriteEntry( hPioSource, sink, delay );
      }
    }

    Hash_FltFree( hOutgoing );
    Hash_PtrRemove( hFwdDelays, source );
  }

  // merge all edges with a Pio as a sink
  Hash_PtrForEachEntry( hFwdDelays, pSourceEntry, i ) {
    hOutgoing = (Hash_Flt_t *)(pSourceEntry->data);
    Hash_FltForEachEntry( hOutgoing, pSinkEntry, j ) {
      sink = pSinkEntry->key;
      delay = pSinkEntry->data;

      max_delay = -ABC_INFINITY;
      if (Abc_ObjIsPo( Abc_NtkObj( pNtk, sink ) )) {
        nMerges++;
        if (delay > max_delay)
          max_delay = delay;
        Vec_IntPush( vFreeList, sink );
      }
    }
    if (max_delay != -ABC_INFINITY)
      Hash_FltWriteEntry( hOutgoing, -1, delay );
    // do freeing
    while( vFreeList->nSize > 0 ) {
      Hash_FltRemove( hOutgoing, Vec_IntPop( vFreeList ) );
    }
  }

  if (fVerbose) printf("Merged %d paths into one Pio node\n", nMerges);

}

/**Function*************************************************************

  Synopsis    [This is a modification of routine from abcDfs.c]

  Description [Recursive DFS from a starting point.  Keeps the endpoints.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDfsReverse_rec2( Abc_Obj_t * pNode, Vec_Ptr_t * vNodes, Vec_Ptr_t * vEndpoints )
{
    Abc_Obj_t * pFanout;
    int i;
    assert( !Abc_ObjIsNet(pNode) );
    // if this node is already visited, skip
    if ( Abc_NodeIsTravIdCurrent( pNode ) )
        return;
    // mark the node as visited
    Abc_NodeSetTravIdCurrent( pNode );
    // terminate at the Co
    if ( Abc_ObjIsCo(pNode) ) {
      Vec_PtrPush( vEndpoints, pNode );
      return;
    }
    assert( Abc_ObjIsNode( pNode ) );
    // visit the transitive fanin of the node
    pNode = Abc_ObjFanout0Ntk(pNode);
    Abc_ObjForEachFanout( pNode, pFanout, i )
      Abc_NtkDfsReverse_rec2( pFanout, vNodes, vEndpoints );
    // add the node after the fanins have been added
    Vec_PtrPush( vNodes, pNode );
}

/**Function*************************************************************

  Synopsis    [Converts all skews into forward skews 0<skew<T.]

  Description [Can also minimize total skew by changing global skew.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Seq_NtkSkewForward( Abc_Ntk_t * pNtk, float period, int fMinimize ) {
  
  Abc_Obj_t * pObj;
  int         i;
  float       skew;
  float       currentSum = 0, bestSum = ABC_INFINITY;
  float       currentOffset = 0, nextStep, bestOffset = 0;
  
  assert( pNtk->vSkews->nSize >= Abc_NtkLatchNum( pNtk )-1 );

  if (fMinimize) {
    // search all offsets for the one that minimizes sum of skews
    while(currentOffset < period) {
      currentSum = 0;
      nextStep = period;
      Abc_NtkForEachLatch( pNtk, pObj, i ) {
        skew = Abc_NtkGetLatSkew( pNtk, i ) + currentOffset;
        skew = (float)(skew - period*floor(skew/period));
        currentSum += skew;
        if (skew > ZERO_SLOP && skew < nextStep) {
          nextStep = skew;
        }
      }

      if (currentSum < bestSum) {
        bestSum = currentSum;
        bestOffset = currentOffset;
      }
      currentOffset += nextStep;
    }
    printf("Offseting all skews by %.2f\n", bestOffset);
  }

  // convert global skew into forward skew
  pNtk->globalSkew = pNtk->globalSkew - bestOffset;
  pNtk->globalSkew = (float)(pNtk->globalSkew - period*floor(pNtk->globalSkew/period));
  assert(pNtk->globalSkew>= 0 && pNtk->globalSkew < period);
    
  // convert endpoint skews into forward skews
  Abc_NtkForEachLatch( pNtk, pObj, i ) {
    skew = Abc_NtkGetLatSkew( pNtk, i ) + bestOffset;
    skew = (float)(skew - period*floor(skew/period));
    REMOVE_ZERO_SLOP( skew );
    assert(skew >=0  && skew < period);

    Abc_NtkSetLatSkew( pNtk, i, skew );
  }
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
