/**CFile****************************************************************

  FileName    [fpgaCreate.c]

  PackageName [MVSIS 1.3: Multi-valued logic synthesis system.]

  Synopsis    [Technology mapping for variable-size-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - August 18, 2004.]

  Revision    [$Id: fpgaCreate.c,v 1.8 2004/09/30 21:18:09 satrajit Exp $]

***********************************************************************/

#include "fpgaInt.h"
#include "base/main/main.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void            Fpga_TableCreate( Fpga_Man_t * p );
static void            Fpga_TableResize( Fpga_Man_t * p );
static Fpga_Node_t *   Fpga_TableLookup( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 );

// hash key for the structural hash table
static inline unsigned Fpga_HashKey2( Fpga_Node_t * p0, Fpga_Node_t * p1, int TableSize ) { return (unsigned)(((ABC_PTRUINT_T)(p0) + (ABC_PTRUINT_T)(p1) * 12582917) % TableSize); }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Reads parameters of the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int             Fpga_ManReadInputNum( Fpga_Man_t * p )                    { return p->nInputs;    }
int             Fpga_ManReadOutputNum( Fpga_Man_t * p )                   { return p->nOutputs;   }
Fpga_Node_t **  Fpga_ManReadInputs ( Fpga_Man_t * p )                     { return p->pInputs;    }
Fpga_Node_t **  Fpga_ManReadOutputs( Fpga_Man_t * p )                     { return p->pOutputs;   }
Fpga_Node_t *   Fpga_ManReadConst1 ( Fpga_Man_t * p )                     { return p->pConst1;    }
float *         Fpga_ManReadInputArrivals( Fpga_Man_t * p )               { return p->pInputArrivals;}
int             Fpga_ManReadVerbose( Fpga_Man_t * p )                     { return p->fVerbose;   }
int             Fpga_ManReadVarMax( Fpga_Man_t * p )                      { return p->pLutLib->LutMax;     }
float *         Fpga_ManReadLutAreas( Fpga_Man_t * p )                    { return p->pLutLib->pLutAreas;  }
Fpga_NodeVec_t* Fpga_ManReadMapping( Fpga_Man_t * p )                     { return p->vMapping;   }
void            Fpga_ManSetOutputNames( Fpga_Man_t * p, char ** ppNames ) { p->ppOutputNames = ppNames; }
void            Fpga_ManSetInputArrivals( Fpga_Man_t * p, float * pArrivals ) { p->pInputArrivals = pArrivals; }
void            Fpga_ManSetAreaRecovery( Fpga_Man_t * p, int fAreaRecovery ) { p->fAreaRecovery = fAreaRecovery;}
void            Fpga_ManSetDelayLimit( Fpga_Man_t * p, float DelayLimit )    { p->DelayLimit   = DelayLimit;    }
void            Fpga_ManSetAreaLimit( Fpga_Man_t * p, float AreaLimit )      { p->AreaLimit    = AreaLimit;     }
void            Fpga_ManSetChoiceNodeNum( Fpga_Man_t * p, int nChoiceNodes ) { p->nChoiceNodes = nChoiceNodes;  }  
void            Fpga_ManSetChoiceNum( Fpga_Man_t * p, int nChoices )         { p->nChoices = nChoices;          }   
void            Fpga_ManSetVerbose( Fpga_Man_t * p, int fVerbose )           { p->fVerbose = fVerbose;          }   
void            Fpga_ManSetSwitching( Fpga_Man_t * p, int fSwitching )       { p->fSwitching = fSwitching;      }   
void            Fpga_ManSetLatchPaths( Fpga_Man_t * p, int fLatchPaths )     { p->fLatchPaths = fLatchPaths;    }   
void            Fpga_ManSetLatchNum( Fpga_Man_t * p, int nLatches )          { p->nLatches = nLatches;          }   
void            Fpga_ManSetDelayTarget( Fpga_Man_t * p, float DelayTarget )  { p->DelayTarget = DelayTarget;    }   
void            Fpga_ManSetName( Fpga_Man_t * p, char * pFileName )          { p->pFileName = pFileName;        }   

/**Function*************************************************************

  Synopsis    [Reads the parameters of the LUT library.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int             Fpga_LibReadLutMax( Fpga_LutLib_t * pLib )  { return pLib->LutMax; }

/**Function*************************************************************

  Synopsis    [Reads parameters of the mapping node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
char *          Fpga_NodeReadData0( Fpga_Node_t * p )                   { return p->pData0;    }
Fpga_Node_t *   Fpga_NodeReadData1( Fpga_Node_t * p )                   { return p->pLevel;    }
int             Fpga_NodeReadRefs( Fpga_Node_t * p )                    { return p->nRefs;     }
int             Fpga_NodeReadNum( Fpga_Node_t * p )                     { return p->Num;       }
int             Fpga_NodeReadLevel( Fpga_Node_t * p )                   { return Fpga_Regular(p)->Level;  }
Fpga_Cut_t *    Fpga_NodeReadCuts( Fpga_Node_t * p )                    { return p->pCuts;     }
Fpga_Cut_t *    Fpga_NodeReadCutBest( Fpga_Node_t * p )                 { return p->pCutBest;  }
Fpga_Node_t *   Fpga_NodeReadOne( Fpga_Node_t * p )                     { return p->p1;        }
Fpga_Node_t *   Fpga_NodeReadTwo( Fpga_Node_t * p )                     { return p->p2;        }
void            Fpga_NodeSetData0( Fpga_Node_t * p, char * pData )         { p->pData0 = pData;  }
void            Fpga_NodeSetData1( Fpga_Node_t * p, Fpga_Node_t * pNode )  { p->pLevel = pNode;  }
void            Fpga_NodeSetNextE( Fpga_Node_t * p, Fpga_Node_t * pNextE ) { p->pNextE = pNextE; }
void            Fpga_NodeSetRepr( Fpga_Node_t * p, Fpga_Node_t * pRepr )   { p->pRepr = pRepr;   }
void            Fpga_NodeSetSwitching( Fpga_Node_t * p, float Switching )  { p->Switching = Switching;   }

/**Function*************************************************************

  Synopsis    [Checks the type of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int             Fpga_NodeIsConst( Fpga_Node_t * p )    {  return (Fpga_Regular(p))->Num == -1;    }
int             Fpga_NodeIsVar( Fpga_Node_t * p )      {  return (Fpga_Regular(p))->p1 == NULL && (Fpga_Regular(p))->Num >= 0; }
int             Fpga_NodeIsAnd( Fpga_Node_t * p )      {  return (Fpga_Regular(p))->p1 != NULL;  }
int             Fpga_NodeComparePhase( Fpga_Node_t * p1, Fpga_Node_t * p2 ) { assert( !Fpga_IsComplement(p1) ); assert( !Fpga_IsComplement(p2) ); return p1->fInv ^ p2->fInv; }

/**Function*************************************************************

  Synopsis    [Reads parameters from the cut.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int             Fpga_CutReadLeavesNum( Fpga_Cut_t * p )            {  return p->nLeaves;  }
Fpga_Node_t **  Fpga_CutReadLeaves( Fpga_Cut_t * p )               {  return p->ppLeaves; }


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
Fpga_Man_t * Fpga_ManCreate( int nInputs, int nOutputs, int fVerbose )
{
    Fpga_Man_t * p;
    int i;

    // start the manager
    p = ABC_ALLOC( Fpga_Man_t, 1 );
    memset( p, 0, sizeof(Fpga_Man_t) );
    p->pLutLib   = (Fpga_LutLib_t *)Abc_FrameReadLibLut();
    p->nVarsMax  = p->pLutLib->LutMax;
    p->fVerbose  = fVerbose;
    p->fAreaRecovery = 1;
    p->fEpsilon  = (float)0.001;

    Fpga_TableCreate( p );
//if ( p->fVerbose )
//    printf( "Node = %d (%d) bytes. Cut = %d bytes.\n", sizeof(Fpga_Node_t), FPGA_NUM_BYTES(sizeof(Fpga_Node_t)), sizeof(Fpga_Cut_t) ); 
    p->mmNodes  = Extra_MmFixedStart( FPGA_NUM_BYTES(sizeof(Fpga_Node_t)) );
    p->mmCuts   = Extra_MmFixedStart( sizeof(Fpga_Cut_t) );

    assert( p->nVarsMax > 0 );
//    Fpga_MappingSetupTruthTables( p->uTruths );

    // make sure the constant node will get index -1
    p->nNodes = -1;
    // create the constant node
    p->pConst1 = Fpga_NodeCreate( p, NULL, NULL );
    p->vNodesAll = Fpga_NodeVecAlloc( 1000 );
    p->vMapping = Fpga_NodeVecAlloc( 1000 );

    // create the PI nodes
    p->nInputs = nInputs;
    p->pInputs = ABC_ALLOC( Fpga_Node_t *, nInputs );
    for ( i = 0; i < nInputs; i++ )
        p->pInputs[i] = Fpga_NodeCreate( p, NULL, NULL );

    // create the place for the output nodes
    p->nOutputs = nOutputs;
    p->pOutputs = ABC_ALLOC( Fpga_Node_t *, nOutputs );
    memset( p->pOutputs, 0, sizeof(Fpga_Node_t *) * nOutputs );
    return p;
}

/**Function*************************************************************

  Synopsis    [Deallocates the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_ManFree( Fpga_Man_t * p )
{
//    Fpga_ManStats( p );
//    int i;
//    for ( i = 0; i < p->vNodesAll->nSize; i++ )
//        Fpga_NodeVecFree( p->vNodesAll->pArray[i]->vFanouts );
//    Fpga_NodeVecFree( p->pConst1->vFanouts );
    if ( p->vMapping )
        Fpga_NodeVecFree( p->vMapping );
    if ( p->vAnds )    
        Fpga_NodeVecFree( p->vAnds );
    if ( p->vNodesAll )    
        Fpga_NodeVecFree( p->vNodesAll );
    Extra_MmFixedStop( p->mmNodes );
    Extra_MmFixedStop( p->mmCuts );
    ABC_FREE( p->ppOutputNames );
    ABC_FREE( p->pInputArrivals );
    ABC_FREE( p->pInputs );
    ABC_FREE( p->pOutputs );
    ABC_FREE( p->pBins );
    ABC_FREE( p );
}


/**Function*************************************************************

  Synopsis    [Prints runtime statistics of the mapping manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_ManPrintTimeStats( Fpga_Man_t * p )
{
//    extern char * pNetName;
//    extern int TotalLuts;
//    FILE * pTable;

    
/*
    pTable = fopen( "stats.txt", "a+" );
    fprintf( pTable, "%s ", pNetName );
    fprintf( pTable, "%.0f ", p->fRequiredGlo );
//    fprintf( pTable, "%.0f ", p->fAreaGlo );//+ (float)nOutputInvs );
    fprintf( pTable, "%.0f ", (float)TotalLuts );
    fprintf( pTable, "%4.2f\n", (float)(p->timeTotal-p->timeToMap)/(float)(CLOCKS_PER_SEC) );
    fclose( pTable );
*/

//    printf( "N-canonical = %d. Matchings = %d.  ", p->nCanons, p->nMatches );
//    printf( "Choice nodes = %d. Choices = %d.\n", p->nChoiceNodes, p->nChoices );
    ABC_PRT( "ToMap", p->timeToMap );
    ABC_PRT( "Cuts ", p->timeCuts );
    ABC_PRT( "Match", p->timeMatch );
    ABC_PRT( "Area ", p->timeRecover );
    ABC_PRT( "ToNet", p->timeToNet );
    ABC_PRT( "TOTAL", p->timeTotal );
    if ( p->time1 ) { ABC_PRT( "time1", p->time1 ); }
    if ( p->time2 ) { ABC_PRT( "time2", p->time2 ); }
}

/**Function*************************************************************

  Synopsis    [Creates a new node.]

  Description [This procedure should be called to create the constant
  node and the PI nodes first.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_NodeCreate( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 )
{
    Fpga_Node_t * pNode;
    // create the node
    pNode = (Fpga_Node_t *)Extra_MmFixedEntryFetch( p->mmNodes );
    memset( pNode, 0, sizeof(Fpga_Node_t) );
    // set very large required time
    pNode->tRequired = FPGA_FLOAT_LARGE;
    pNode->aEstFanouts = -1;
    pNode->p1  = p1; 
    pNode->p2  = p2;
    // set the number of this node
    pNode->Num = p->nNodes++;
    // place to store the fanouts
//    pNode->vFanouts = Fpga_NodeVecAlloc( 5 );
    // store this node in the internal array
    if ( pNode->Num >= 0 )
        Fpga_NodeVecPush( p->vNodesAll, pNode );
    else
        pNode->fInv = 1;
    // set the level of this node
    if ( p1 ) 
    {
#ifdef FPGA_ALLOCATE_FANOUT
        // create the fanout info
        Fpga_NodeAddFaninFanout( Fpga_Regular(p1), pNode );
        Fpga_NodeAddFaninFanout( Fpga_Regular(p2), pNode );
#endif
        // compute the level
        pNode->Level = 1 + FPGA_MAX(Fpga_Regular(p1)->Level, Fpga_Regular(p2)->Level);
        pNode->fInv  = Fpga_NodeIsSimComplement(p1) & Fpga_NodeIsSimComplement(p2);
    }
    // reference the inputs 
    if ( p1 ) Fpga_NodeRef(p1);
    if ( p2 ) Fpga_NodeRef(p2);
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Create the unique table of AND gates.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_TableCreate( Fpga_Man_t * pMan )
{
    assert( pMan->pBins == NULL );
    pMan->nBins = Abc_PrimeCudd(50000);
    pMan->pBins = ABC_ALLOC( Fpga_Node_t *, pMan->nBins );
    memset( pMan->pBins, 0, sizeof(Fpga_Node_t *) * pMan->nBins );
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
Fpga_Node_t * Fpga_TableLookup( Fpga_Man_t * pMan, Fpga_Node_t * p1, Fpga_Node_t * p2 )
{
    Fpga_Node_t * pEnt;
    unsigned Key;

    if ( p1 == p2 )
        return p1;
    if ( p1 == Fpga_Not(p2) )
        return Fpga_Not(pMan->pConst1);
    if ( Fpga_NodeIsConst(p1) )
    {
        if ( p1 == pMan->pConst1 )
            return p2;
        return Fpga_Not(pMan->pConst1);
    }
    if ( Fpga_NodeIsConst(p2) )
    {
        if ( p2 == pMan->pConst1 )
            return p1;
        return Fpga_Not(pMan->pConst1);
    }

    if ( Fpga_Regular(p1)->Num > Fpga_Regular(p2)->Num )
        pEnt = p1, p1 = p2, p2 = pEnt;

    Key = Fpga_HashKey2( p1, p2, pMan->nBins );
    for ( pEnt = pMan->pBins[Key]; pEnt; pEnt = pEnt->pNext )
        if ( pEnt->p1 == p1 && pEnt->p2 == p2 )
            return pEnt;
    // resize the table
    if ( pMan->nNodes >= 2 * pMan->nBins )
    {
        Fpga_TableResize( pMan );
        Key = Fpga_HashKey2( p1, p2, pMan->nBins );
    }
    // create the new node
    pEnt = Fpga_NodeCreate( pMan, p1, p2 );
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
void Fpga_TableResize( Fpga_Man_t * pMan )
{
    Fpga_Node_t ** pBinsNew;
    Fpga_Node_t * pEnt, * pEnt2;
    int nBinsNew, Counter, i;
    clock_t clk;
    unsigned Key;

clk = clock();
    // get the new table size
    nBinsNew = Abc_PrimeCudd(2 * pMan->nBins); 
    // allocate a new array
    pBinsNew = ABC_ALLOC( Fpga_Node_t *, nBinsNew );
    memset( pBinsNew, 0, sizeof(Fpga_Node_t *) * nBinsNew );
    // rehash the entries from the old table
    Counter = 0;
    for ( i = 0; i < pMan->nBins; i++ )
        for ( pEnt = pMan->pBins[i], pEnt2 = pEnt? pEnt->pNext: NULL; pEnt; 
              pEnt = pEnt2, pEnt2 = pEnt? pEnt->pNext: NULL )
        {
            Key = Fpga_HashKey2( pEnt->p1, pEnt->p2, nBinsNew );
            pEnt->pNext = pBinsNew[Key];
            pBinsNew[Key] = pEnt;
            Counter++;
        }
    assert( Counter == pMan->nNodes - pMan->nInputs );
    if ( pMan->fVerbose )
    {
//        printf( "Increasing the unique table size from %6d to %6d. ", pMan->nBins, nBinsNew );
//        ABC_PRT( "Time", clock() - clk );
    }
    // replace the table and the parameters
    ABC_FREE( pMan->pBins );
    pMan->pBins = pBinsNew;
    pMan->nBins = nBinsNew;
}



/**Function*************************************************************

  Synopsis    [Elementary AND operation on the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_NodeAnd( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 )
{
    Fpga_Node_t * pNode;
    pNode = Fpga_TableLookup( p, p1, p2 );     
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Elementary OR operation on the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_NodeOr( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 )
{
    Fpga_Node_t * pNode;
    pNode = Fpga_Not( Fpga_TableLookup( p, Fpga_Not(p1), Fpga_Not(p2) ) );  
    return pNode;
}

/**Function*************************************************************

  Synopsis    [Elementary EXOR operation on the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_NodeExor( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 )
{
    return Fpga_NodeMux( p, p1, Fpga_Not(p2), p2 );
}

/**Function*************************************************************

  Synopsis    [Elementary MUX operation on the AIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fpga_Node_t * Fpga_NodeMux( Fpga_Man_t * p, Fpga_Node_t * pC, Fpga_Node_t * pT, Fpga_Node_t * pE )
{
    Fpga_Node_t * pAnd1, * pAnd2, * pRes;
    pAnd1 = Fpga_TableLookup( p, pC,           pT ); 
    pAnd2 = Fpga_TableLookup( p, Fpga_Not(pC), pE ); 
    pRes  = Fpga_NodeOr( p, pAnd1, pAnd2 );           
    return pRes;
}


/**Function*************************************************************

  Synopsis    [Sets the node to be equivalent to the given one.]

  Description [This procedure is a work-around for the equivalence check.
  Does not verify the equivalence. Use at the user's risk.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_NodeSetChoice( Fpga_Man_t * pMan, Fpga_Node_t * pNodeOld, Fpga_Node_t * pNodeNew )
{
    pNodeNew->pNextE = pNodeOld->pNextE;
    pNodeOld->pNextE = pNodeNew;
    pNodeNew->pRepr  = pNodeOld;
}


    
/**Function*************************************************************

  Synopsis    [Prints some interesting stats.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fpga_ManStats( Fpga_Man_t * p )
{
    FILE * pTable;
    pTable = fopen( "stats.txt", "a+" );
    fprintf( pTable, "%s ", p->pFileName );
    fprintf( pTable, "%4d ", p->nInputs - p->nLatches );
    fprintf( pTable, "%4d ", p->nOutputs - p->nLatches );
    fprintf( pTable, "%4d ", p->nLatches );
    fprintf( pTable, "%7d ", p->vAnds->nSize );
    fprintf( pTable, "%7d ", Fpga_CutCountAll(p) );
    fprintf( pTable, "%2d\n", (int)p->fRequiredGlo );
    fclose( pTable );
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_IMPL_END

