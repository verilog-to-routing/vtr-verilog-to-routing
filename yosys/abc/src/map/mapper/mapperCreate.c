/**CFile****************************************************************

  FileName    [mapperCreate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Generic technology mapping engine.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - June 1, 2004.]

  Revision    [$Id: mapperCreate.c,v 1.15 2005/02/28 05:34:26 alanmi Exp $]

***********************************************************************/

#include "mapperInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void            Map_TableCreate( Map_Man_t * p );
static void            Map_TableResize( Map_Man_t * p );

// hash key for the structural hash table
static inline unsigned Map_HashKey2( Map_Node_t * p0, Map_Node_t * p1, int TableSize ) { return (unsigned)(((ABC_PTRUINT_T)(p0) + (ABC_PTRUINT_T)(p1) * 12582917) % TableSize); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads parameters from the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int             Map_ManReadInputNum( Map_Man_t * p )                    { return p->nInputs;    }
int             Map_ManReadOutputNum( Map_Man_t * p )                   { return p->nOutputs;   }
int             Map_ManReadBufNum( Map_Man_t * p )                      { return Map_NodeVecReadSize(p->vMapBufs);   }
Map_Node_t **   Map_ManReadInputs ( Map_Man_t * p )                     { return p->pInputs;    }
Map_Node_t **   Map_ManReadOutputs( Map_Man_t * p )                     { return p->pOutputs;   }
Map_Node_t **   Map_ManReadBufs( Map_Man_t * p )                        { return Map_NodeVecReadArray(p->vMapBufs);  }
Map_Node_t *    Map_ManReadBufDriver( Map_Man_t * p, int i )            { return Map_ManReadBufs(p)[i]->p1;           }
Map_Node_t *    Map_ManReadConst1 ( Map_Man_t * p )                     { return p->pConst1;    }
Map_Time_t *    Map_ManReadInputArrivals( Map_Man_t * p )               { return p->pInputArrivals; }
Map_Time_t *    Map_ManReadOutputRequireds( Map_Man_t * p )             { return p->pOutputRequireds; }
Mio_Library_t * Map_ManReadGenLib ( Map_Man_t * p )                     { return p->pSuperLib->pGenlib; }
int             Map_ManReadVerbose( Map_Man_t * p )                     { return p->fVerbose;   }
float           Map_ManReadAreaFinal( Map_Man_t * p )                   { return p->AreaFinal;  }
float           Map_ManReadRequiredGlo( Map_Man_t * p )                 { return p->fRequiredGlo; }
void            Map_ManSetOutputNames( Map_Man_t * p, char ** ppNames ) { p->ppOutputNames = ppNames;}
void            Map_ManSetAreaRecovery( Map_Man_t * p, int fAreaRecovery ) { p->fAreaRecovery = fAreaRecovery;}
void            Map_ManSetDelayTarget( Map_Man_t * p, float DelayTarget ) { p->DelayTarget = DelayTarget;}
void            Map_ManSetInputArrivals( Map_Man_t * p, Map_Time_t * pArrivals )     { p->pInputArrivals = pArrivals;    }
void            Map_ManSetOutputRequireds( Map_Man_t * p, Map_Time_t * pRequireds )  { p->pOutputRequireds = pRequireds; }
void            Map_ManSetObeyFanoutLimits( Map_Man_t * p, int  fObeyFanoutLimits )  { p->fObeyFanoutLimits = fObeyFanoutLimits;     }
void            Map_ManSetNumIterations( Map_Man_t * p, int nIterations )            { p->nIterations = nIterations;     }
int             Map_ManReadFanoutViolations( Map_Man_t * p )            { return p->nFanoutViolations;   }  
void            Map_ManSetFanoutViolations( Map_Man_t * p, int nVio )   { p->nFanoutViolations = nVio;   }  
void            Map_ManSetChoiceNodeNum( Map_Man_t * p, int nChoiceNodes ) { p->nChoiceNodes = nChoiceNodes; }  
void            Map_ManSetChoiceNum( Map_Man_t * p, int nChoices )         { p->nChoices = nChoices;     }   
void            Map_ManSetVerbose( Map_Man_t * p, int fVerbose )           { p->fVerbose = fVerbose;     }   
void            Map_ManSetSwitching( Map_Man_t * p, int fSwitching )       { p->fSwitching = fSwitching; }   
void            Map_ManSetSkipFanout( Map_Man_t * p, int fSkipFanout )     { p->fSkipFanout = fSkipFanout; }   
void            Map_ManSetUseProfile( Map_Man_t * p )                      { p->fUseProfile = 1;         }   
void            Map_ManCreateAigIds( Map_Man_t * p, int nObjs )            { p->pAigNodeIDs = ABC_CALLOC( int, nObjs ); }   

/**Function*************************************************************

  Synopsis    [Reads parameters from the mapping node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Man_t *     Map_NodeReadMan( Map_Node_t * p )                     { return p->p;                  }
char *          Map_NodeReadData( Map_Node_t * p, int fPhase )        { return fPhase? p->pData1 : p->pData0;  }
int             Map_NodeReadNum( Map_Node_t * p )                     { return p->Num;                }
int             Map_NodeReadLevel( Map_Node_t * p )                   { return Map_Regular(p)->Level; }
int             Map_NodeReadAigId( Map_Node_t * p )                   { return p->p->pAigNodeIDs[p->Num]; }
Map_Cut_t *     Map_NodeReadCuts( Map_Node_t * p )                    { return p->pCuts;              }
Map_Cut_t *     Map_NodeReadCutBest( Map_Node_t * p, int fPhase )     { return p->pCutBest[fPhase];   }
Map_Node_t *    Map_NodeReadOne( Map_Node_t * p )                     { return p->p1;                 }
Map_Node_t *    Map_NodeReadTwo( Map_Node_t * p )                     { return p->p2;                 }
void            Map_NodeSetData( Map_Node_t * p, int fPhase, char * pData ) { if (fPhase) p->pData1 = pData; else p->pData0 = pData; }
void            Map_NodeSetNextE( Map_Node_t * p, Map_Node_t * pNextE )     { p->pNextE = pNextE;       }
void            Map_NodeSetRepr( Map_Node_t * p, Map_Node_t * pRepr )       { p->pRepr = pRepr;         }
void            Map_NodeSetSwitching( Map_Node_t * p, float Switching )     { p->Switching = Switching; }
void            Map_NodeSetAigId( Map_Node_t * p, int Id )            { p->p->pAigNodeIDs[p->Num] = Id; }

;


/**Function*************************************************************

  Synopsis    [Checks the type of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int             Map_NodeIsConst( Map_Node_t * p )    {  return (Map_Regular(p))->Num == -1;    }
int             Map_NodeIsVar( Map_Node_t * p )      {  return (Map_Regular(p))->p1 == NULL && (Map_Regular(p))->Num >= 0; }
int             Map_NodeIsBuf( Map_Node_t * p )      {  return (Map_Regular(p))->p1 != NULL && (Map_Regular(p))->p2 == NULL;  }
int             Map_NodeIsAnd( Map_Node_t * p )      {  return (Map_Regular(p))->p1 != NULL && (Map_Regular(p))->p2 != NULL;  }
int             Map_NodeComparePhase( Map_Node_t * p1, Map_Node_t * p2 ) { assert( !Map_IsComplement(p1) ); assert( !Map_IsComplement(p2) ); return p1->fInv ^ p2->fInv; }

/**Function*************************************************************

  Synopsis    [Reads parameters from the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Super_t *   Map_CutReadSuperBest( Map_Cut_t * p, int fPhase ) { return p->M[fPhase].pSuperBest;}
Map_Super_t *   Map_CutReadSuper0( Map_Cut_t * p )                { return p->M[0].pSuperBest;}
Map_Super_t *   Map_CutReadSuper1( Map_Cut_t * p )                { return p->M[1].pSuperBest;}
int             Map_CutReadLeavesNum( Map_Cut_t * p )             { return p->nLeaves;  }
Map_Node_t **   Map_CutReadLeaves( Map_Cut_t * p )                { return p->ppLeaves; }
unsigned        Map_CutReadPhaseBest( Map_Cut_t * p, int fPhase ) { return p->M[fPhase].uPhaseBest;}
unsigned        Map_CutReadPhase0( Map_Cut_t * p )                { return p->M[0].uPhaseBest;}
unsigned        Map_CutReadPhase1( Map_Cut_t * p )                { return p->M[1].uPhaseBest;}
Map_Cut_t *     Map_CutReadNext( Map_Cut_t * p )                  { return p->pNext;          }

/**Function*************************************************************

  Synopsis    [Reads parameters from the supergate.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char *          Map_SuperReadFormula( Map_Super_t * p )          {  return p->pFormula; }
Mio_Gate_t *    Map_SuperReadRoot( Map_Super_t * p )             {  return p->pRoot;    }
int             Map_SuperReadNum( Map_Super_t * p )              {  return p->Num;      }
Map_Super_t **  Map_SuperReadFanins( Map_Super_t * p )           {  return p->pFanins;  }
int             Map_SuperReadFaninNum( Map_Super_t * p )         {  return p->nFanins;  }
Map_Super_t *   Map_SuperReadNext( Map_Super_t * p )             {  return p->pNext;    }
int             Map_SuperReadNumPhases( Map_Super_t * p )        {  return p->nPhases;  }
unsigned char * Map_SuperReadPhases( Map_Super_t * p )           {  return p->uPhases;  }
int             Map_SuperReadFanoutLimit( Map_Super_t * p )      {  return p->nFanLimit;}

Mio_Library_t * Map_SuperLibReadGenLib( Map_SuperLib_t * p )     {  return p->pGenlib;  }
float           Map_SuperLibReadAreaInv( Map_SuperLib_t * p )    {  return p->AreaInv;  }
Map_Time_t      Map_SuperLibReadDelayInv( Map_SuperLib_t * p )   {  return p->tDelayInv;}
int             Map_SuperLibReadVarsMax( Map_SuperLib_t * p )    {  return p->nVarsMax; }


/**Function*************************************************************

  Synopsis    [Create the mapping manager.]

  Description [The number of inputs and outputs is assumed to be
  known is advance. It is much simpler to have them fixed upfront.
  When it comes to representing the object graph in the form of
  AIG, the resulting manager is similar to the regular AIG manager, 
  except that it does not use reference counting (and therefore
  does not have garbage collections). It does have table resizing.
  The data structure is more flexible to represent additional 
  information needed for mapping.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Man_t * Map_ManCreate( int nInputs, int nOutputs, int fVerbose )
{
    Map_Man_t * p;
    int i;

    // derive the supergate library
    if ( Abc_FrameReadLibSuper() == NULL )
    {
        printf( "The supergate library is not specified. Use \"read_super\".\n" );
        return NULL;
    }

    // start the manager
    p = ABC_ALLOC( Map_Man_t, 1 );
    memset( p, 0, sizeof(Map_Man_t) );
    p->pSuperLib = (Map_SuperLib_t *)Abc_FrameReadLibSuper();
    p->nVarsMax  = p->pSuperLib->nVarsMax;
    p->fVerbose  = fVerbose;
    p->fEpsilon  = (float)0.001;
    assert( p->nVarsMax > 0 );

    if ( p->nVarsMax == 5 )
        Extra_Truth4VarN( &p->uCanons, &p->uPhases, &p->pCounters, 8 );

    // start various data structures
    Map_TableCreate( p );
    Map_MappingSetupTruthTables( p->uTruths );
    Map_MappingSetupTruthTablesLarge( p->uTruthsLarge );
//    printf( "Node = %d bytes. Cut = %d bytes. Super = %d bytes.\n", sizeof(Map_Node_t), sizeof(Map_Cut_t), sizeof(Map_Super_t) ); 
    p->mmNodes    = Extra_MmFixedStart( sizeof(Map_Node_t) );
    p->mmCuts     = Extra_MmFixedStart( sizeof(Map_Cut_t) );

    // make sure the constant node will get index -1
    p->nNodes = -1;
    // create the constant node
    p->pConst1    = Map_NodeCreate( p, NULL, NULL );
    p->vMapObjs   = Map_NodeVecAlloc( 100 );
    p->vMapBufs   = Map_NodeVecAlloc( 100 );
    p->vVisited   = Map_NodeVecAlloc( 100 );

    // create the PI nodes
    p->nInputs = nInputs;
    p->pInputs = ABC_ALLOC( Map_Node_t *, nInputs );
    for ( i = 0; i < nInputs; i++ )
        p->pInputs[i] = Map_NodeCreate( p, NULL, NULL );

    // create the place for the output nodes
    p->nOutputs = nOutputs;
    p->pOutputs = ABC_ALLOC( Map_Node_t *, nOutputs );
    memset( p->pOutputs, 0, sizeof(Map_Node_t *) * nOutputs );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_ManFree( Map_Man_t * p )
{
//    int i;
//    for ( i = 0; i < p->vMapObjs->nSize; i++ )
//        Map_NodeVecFree( p->vMapObjs->pArray[i]->vFanouts );
//    Map_NodeVecFree( p->pConst1->vFanouts );
    Map_NodeVecFree( p->vMapObjs );
    Map_NodeVecFree( p->vMapBufs );
    Map_NodeVecFree( p->vVisited );
    if ( p->uCanons )   ABC_FREE( p->uCanons );
    if ( p->uPhases )   ABC_FREE( p->uPhases );
    if ( p->pCounters ) ABC_FREE( p->pCounters );
    Extra_MmFixedStop( p->mmNodes );
    Extra_MmFixedStop( p->mmCuts );
    ABC_FREE( p->pAigNodeIDs );
    ABC_FREE( p->pNodeDelays );
    ABC_FREE( p->pInputArrivals );
    ABC_FREE( p->pOutputRequireds );
    ABC_FREE( p->pInputs );
    ABC_FREE( p->pOutputs );
    ABC_FREE( p->pBins );
    ABC_FREE( p->ppOutputNames );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Creates node delays.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_ManCreateNodeDelays( Map_Man_t * p, int LogFan )
{
    Map_Node_t * pNode;
    int k;
    assert( p->pNodeDelays == NULL );
    p->pNodeDelays = ABC_CALLOC( float, p->vMapObjs->nSize );
    for ( k = 0; k < p->vMapObjs->nSize; k++ )
    {
        pNode = p->vMapObjs->pArray[k];
        if ( pNode->nRefs == 0 )
            continue;
        p->pNodeDelays[k] = 0.014426 * LogFan * p->pSuperLib->tDelayInv.Worst * log( (double)pNode->nRefs ); // 1.4426 = 1/ln(2)
//        printf( "%d = %d (%.2f)  ", k, pNode->nRefs, p->pNodeDelays[k] );
    }
//    printf( "\n" );
}


/**Function*************************************************************

  Synopsis    [Deallocates the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_ManPrintTimeStats( Map_Man_t * p )
{
    printf( "N-canonical = %d. Matchings = %d.  Phases = %d.  ", p->nCanons, p->nMatches, p->nPhases );
    printf( "Choice nodes = %d. Choices = %d.\n", p->nChoiceNodes, p->nChoices );
    ABC_PRT( "ToMap", p->timeToMap );
    ABC_PRT( "Cuts ", p->timeCuts  );
    ABC_PRT( "Truth", p->timeTruth );
    ABC_PRT( "Match", p->timeMatch );
    ABC_PRT( "Area ", p->timeArea  );
    ABC_PRT( "Sweep", p->timeSweep );
    ABC_PRT( "ToNet", p->timeToNet );
    ABC_PRT( "TOTAL", p->timeTotal );
    if ( p->time1 ) { ABC_PRT( "time1", p->time1 ); }
    if ( p->time2 ) { ABC_PRT( "time2", p->time2 ); }
    if ( p->time3 ) { ABC_PRT( "time3", p->time3 ); }
}

/**Function*************************************************************

  Synopsis    [Prints the mapping stats.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_ManPrintStatsToFile( char * pName, float Area, float Delay, abctime Time )
{
    FILE * pTable;
    pTable = fopen( "map_stats.txt", "a+" );
    fprintf( pTable, "%s ", pName );
    fprintf( pTable, "%4.2f ", Area );
    fprintf( pTable, "%4.2f ", Delay );
    fprintf( pTable, "%4.2f\n", (float)(Time)/(float)(CLOCKS_PER_SEC) );
    fclose( pTable );
}

/**Function*************************************************************

  Synopsis    [Creates a new node.]

  Description [This procedure should be called to create the constant
  node and the PI nodes first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Node_t * Map_NodeCreate( Map_Man_t * p, Map_Node_t * p1, Map_Node_t * p2 )
{
    Map_Node_t * pNode;
    // create the node
    pNode = (Map_Node_t *)Extra_MmFixedEntryFetch( p->mmNodes );
    memset( pNode, 0, sizeof(Map_Node_t) );
    pNode->tRequired[0].Rise = pNode->tRequired[0].Fall = pNode->tRequired[0].Worst = MAP_FLOAT_LARGE;
    pNode->tRequired[1].Rise = pNode->tRequired[1].Fall = pNode->tRequired[1].Worst = MAP_FLOAT_LARGE;
    pNode->p1  = p1; 
    pNode->p2  = p2;
    pNode->p = p;
    // set the number of this node
    pNode->Num = p->nNodes++;
    // place to store the fanouts
//    pNode->vFanouts = Map_NodeVecAlloc( 5 );
    // store this node in the internal array
    if ( pNode->Num >= 0 )
        Map_NodeVecPush( p->vMapObjs, pNode );
    else
        pNode->fInv = 1;
    // set the level of this node
    if ( p1 ) 
    {
#ifdef MAP_ALLOCATE_FANOUT
        // create the fanout info
        Map_NodeAddFaninFanout( Map_Regular(p1), pNode );
        if ( p2 )
        Map_NodeAddFaninFanout( Map_Regular(p2), pNode );
#endif

        if ( p2 )
        {
            pNode->Level = 1 + MAP_MAX(Map_Regular(pNode->p1)->Level, Map_Regular(pNode->p2)->Level);
            pNode->fInv  = Map_NodeIsSimComplement(p1) & Map_NodeIsSimComplement(p2);
        }
        else
        {
            pNode->Level = Map_Regular(pNode->p1)->Level;
            pNode->fInv  = Map_NodeIsSimComplement(p1);
        }
    }
    // reference the inputs (will be used to compute the number of fanouts)
    if ( p1 ) Map_NodeRef(p1);
    if ( p2 ) Map_NodeRef(p2);

    pNode->nRefEst[0] = pNode->nRefEst[1] = -1;
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Create the unique table of AND gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_TableCreate( Map_Man_t * pMan )
{
    assert( pMan->pBins == NULL );
    pMan->nBins = Abc_PrimeCudd(5000);
    pMan->pBins = ABC_ALLOC( Map_Node_t *, pMan->nBins );
    memset( pMan->pBins, 0, sizeof(Map_Node_t *) * pMan->nBins );
    pMan->nNodes = 0;
}

/**Function*************************************************************

  Synopsis    [Looks up the AND2 node in the unique table.]

  Description [This procedure implements one-level hashing. All the nodes
  are hashed by their children. If the node with the same children was already
  created, it is returned by the call to this procedure. If it does not exist,
  this procedure creates a new node with these children. ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Node_t * Map_NodeAnd( Map_Man_t * pMan, Map_Node_t * p1, Map_Node_t * p2 )
{
    Map_Node_t * pEnt;
    unsigned Key;

    if ( p1 == p2 )
        return p1;
    if ( p1 == Map_Not(p2) )
        return Map_Not(pMan->pConst1);
    if ( Map_NodeIsConst(p1) )
    {
        if ( p1 == pMan->pConst1 )
            return p2;
        return Map_Not(pMan->pConst1);
    }
    if ( Map_NodeIsConst(p2) )
    {
        if ( p2 == pMan->pConst1 )
            return p1;
        return Map_Not(pMan->pConst1);
    }

    if ( Map_Regular(p1)->Num > Map_Regular(p2)->Num )
        pEnt = p1, p1 = p2, p2 = pEnt;

    Key = Map_HashKey2( p1, p2, pMan->nBins );
    for ( pEnt = pMan->pBins[Key]; pEnt; pEnt = pEnt->pNext )
        if ( pEnt->p1 == p1 && pEnt->p2 == p2 )
            return pEnt;
    // resize the table
    if ( pMan->nNodes >= 2 * pMan->nBins )
    {
        Map_TableResize( pMan );
        Key = Map_HashKey2( p1, p2, pMan->nBins );
    }
    // create the new node
    pEnt = Map_NodeCreate( pMan, p1, p2 );
    // add the node to the corresponding linked list in the table
    pEnt->pNext = pMan->pBins[Key];
    pMan->pBins[Key] = pEnt;
    return pEnt;
}


/**Function*************************************************************

  Synopsis    [Resizes the table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_TableResize( Map_Man_t * pMan )
{
    Map_Node_t ** pBinsNew;
    Map_Node_t * pEnt, * pEnt2;
    int nBinsNew, Counter, i;
    abctime clk;
    unsigned Key;

clk = Abc_Clock();
    // get the new table size
    nBinsNew = Abc_PrimeCudd(2 * pMan->nBins); 
    // allocate a new array
    pBinsNew = ABC_ALLOC( Map_Node_t *, nBinsNew );
    memset( pBinsNew, 0, sizeof(Map_Node_t *) * nBinsNew );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < pMan->nBins; i++ )
        for ( pEnt = pMan->pBins[i], pEnt2 = pEnt? pEnt->pNext: NULL; pEnt; 
              pEnt = pEnt2, pEnt2 = pEnt? pEnt->pNext: NULL )
        {
            Key = Map_HashKey2( pEnt->p1, pEnt->p2, nBinsNew );
            pEnt->pNext = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == pMan->nNodes - pMan->nInputs );
    if ( pMan->fVerbose )
    {
//        printf( "Increasing the unique table size from %6d to %6d. ", pMan->nBins, nBinsNew );
//        ABC_PRT( "Time", Abc_Clock() - clk );
    }
    // replace the table and the parameters
    ABC_FREE( pMan->pBins );
    pMan->pBins = pBinsNew;
    pMan->nBins = nBinsNew;
}



/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Map_Node_t * Map_NodeBuf( Map_Man_t * p, Map_Node_t * p1 )
{
    Map_Node_t * pNode = Map_NodeCreate( p, p1, NULL );
    Map_NodeVecPush( p->vMapBufs, pNode );
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Sets the node to be equivalent to the given one.]

  Description [This procedure is a work-around for the equivalence check.
  Does not verify the equivalence. Use at the user's risk.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Map_NodeSetChoice( Map_Man_t * pMan, Map_Node_t * pNodeOld, Map_Node_t * pNodeNew )
{
    pNodeNew->pNextE = pNodeOld->pNextE;
    pNodeOld->pNextE = pNodeNew;
    pNodeNew->pRepr  = pNodeOld;
}



////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

