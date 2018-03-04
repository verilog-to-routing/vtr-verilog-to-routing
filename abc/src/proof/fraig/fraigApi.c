/**CFile****************************************************************

  FileName    [fraigApi.c]

  PackageName [FRAIG: Functionally reduced AND-INV graphs.]

  Synopsis    [Access APIs for the FRAIG manager and node.]

  Author      [Alan Mishchenko <alanmi@eecs.berkeley.edu>]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - October 1, 2004]

  Revision    [$Id: fraigApi.c,v 1.2 2005/07/08 01:01:30 alanmi Exp $]

***********************************************************************/

#include "fraigInt.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Access functions to read the data members of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_NodeVec_t * Fraig_ManReadVecInputs( Fraig_Man_t * p )                   { return p->vInputs;            }
Fraig_NodeVec_t * Fraig_ManReadVecOutputs( Fraig_Man_t * p )                  { return p->vOutputs;           }
Fraig_NodeVec_t * Fraig_ManReadVecNodes( Fraig_Man_t * p )                    { return p->vNodes;             }
Fraig_Node_t **   Fraig_ManReadInputs ( Fraig_Man_t * p )                     { return p->vInputs->pArray;    }
Fraig_Node_t **   Fraig_ManReadOutputs( Fraig_Man_t * p )                     { return p->vOutputs->pArray;   }
Fraig_Node_t **   Fraig_ManReadNodes( Fraig_Man_t * p )                       { return p->vNodes->pArray;     }
int               Fraig_ManReadInputNum ( Fraig_Man_t * p )                   { return p->vInputs->nSize;     }
int               Fraig_ManReadOutputNum( Fraig_Man_t * p )                   { return p->vOutputs->nSize;    }
int               Fraig_ManReadNodeNum( Fraig_Man_t * p )                     { return p->vNodes->nSize;      }
Fraig_Node_t *    Fraig_ManReadConst1 ( Fraig_Man_t * p )                     { return p->pConst1;            }
Fraig_Node_t *    Fraig_ManReadIthNode( Fraig_Man_t * p, int i )              { assert ( i < p->vNodes->nSize  ); return p->vNodes->pArray[i];  }
char **           Fraig_ManReadInputNames( Fraig_Man_t * p )                  { return p->ppInputNames;       }
char **           Fraig_ManReadOutputNames( Fraig_Man_t * p )                 { return p->ppOutputNames;      }
char *            Fraig_ManReadVarsInt( Fraig_Man_t * p )                     { return (char *)p->vVarsInt;   }
char *            Fraig_ManReadSat( Fraig_Man_t * p )                         { return (char *)p->pSat;       }
int               Fraig_ManReadFuncRed( Fraig_Man_t * p )                     { return p->fFuncRed;   }
int               Fraig_ManReadFeedBack( Fraig_Man_t * p )                    { return p->fFeedBack;  }
int               Fraig_ManReadDoSparse( Fraig_Man_t * p )                    { return p->fDoSparse;  }
int               Fraig_ManReadChoicing( Fraig_Man_t * p )                    { return p->fChoicing;  }
int               Fraig_ManReadVerbose( Fraig_Man_t * p )                     { return p->fVerbose;   }
int *             Fraig_ManReadModel( Fraig_Man_t * p )                       { return p->pModel;     }
// returns the number of patterns used for random simulation (this number is fixed for the FRAIG run)
int               Fraig_ManReadPatternNumRandom( Fraig_Man_t * p )            { return p->nWordsRand * 32;  }
// returns the number of dynamic patterns accumulated at runtime (include SAT solver counter-examples and distance-1 patterns derived from them)
int               Fraig_ManReadPatternNumDynamic( Fraig_Man_t * p )           { return p->iWordStart * 32;  }
// returns the number of dynamic patterns proved useful to distinquish some FRAIG nodes (this number is more than 0 after the first garbage collection of patterns)
int               Fraig_ManReadPatternNumDynamicFiltered( Fraig_Man_t * p )   { return p->iPatsPerm;        }
// returns the number of times FRAIG package timed out
int               Fraig_ManReadSatFails( Fraig_Man_t * p )                    { return p->nSatFailsReal;    }      
// returns the number of conflicts in the SAT solver
int               Fraig_ManReadConflicts( Fraig_Man_t * p )                   { return p->pSat? Msat_SolverReadBackTracks(p->pSat) : 0; }      
// returns the number of inspections in the SAT solver
int               Fraig_ManReadInspects( Fraig_Man_t * p )                    { return p->pSat? Msat_SolverReadInspects(p->pSat) : 0;   }            

/**Function*************************************************************

  Synopsis    [Access functions to set the data members of the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void            Fraig_ManSetFuncRed( Fraig_Man_t * p, int fFuncRed )        { p->fFuncRed  = fFuncRed;  }
void            Fraig_ManSetFeedBack( Fraig_Man_t * p, int fFeedBack )      { p->fFeedBack = fFeedBack; }
void            Fraig_ManSetDoSparse( Fraig_Man_t * p, int fDoSparse )      { p->fDoSparse = fDoSparse; }
void            Fraig_ManSetChoicing( Fraig_Man_t * p, int fChoicing )      { p->fChoicing = fChoicing; }
void            Fraig_ManSetTryProve( Fraig_Man_t * p, int fTryProve )      { p->fTryProve = fTryProve; }
void            Fraig_ManSetVerbose( Fraig_Man_t * p, int fVerbose )        { p->fVerbose  = fVerbose;  }
void            Fraig_ManSetOutputNames( Fraig_Man_t * p, char ** ppNames ) { p->ppOutputNames = ppNames; }
void            Fraig_ManSetInputNames( Fraig_Man_t * p, char ** ppNames )  { p->ppInputNames  = ppNames; }

/**Function*************************************************************

  Synopsis    [Access functions to read the data members of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t *  Fraig_NodeReadData0( Fraig_Node_t * p )                   { return p->pData0;    }
Fraig_Node_t *  Fraig_NodeReadData1( Fraig_Node_t * p )                   { return p->pData1;    }
int             Fraig_NodeReadNum( Fraig_Node_t * p )                     { return p->Num;       }
Fraig_Node_t *  Fraig_NodeReadOne( Fraig_Node_t * p )                     { assert (!Fraig_IsComplement(p)); return p->p1; }
Fraig_Node_t *  Fraig_NodeReadTwo( Fraig_Node_t * p )                     { assert (!Fraig_IsComplement(p)); return p->p2; }
Fraig_Node_t *  Fraig_NodeReadNextE( Fraig_Node_t * p )                   { return p->pNextE;    }
Fraig_Node_t *  Fraig_NodeReadRepr( Fraig_Node_t * p )                    { return p->pRepr;     }
int             Fraig_NodeReadNumRefs( Fraig_Node_t * p )                 { return p->nRefs;     }
int             Fraig_NodeReadNumFanouts( Fraig_Node_t * p )              { return p->nFanouts;  }
int             Fraig_NodeReadSimInv( Fraig_Node_t * p )                  { return p->fInv;      }
int             Fraig_NodeReadNumOnes( Fraig_Node_t * p )                 { return p->nOnes;     }
// returns the pointer to the random simulation patterns (their number is returned by Fraig_ManReadPatternNumRandom)
// memory pointed to by this and the following procedure is maintained by the FRAIG package and exists as long as the package runs
unsigned *      Fraig_NodeReadPatternsRandom( Fraig_Node_t * p )          { return p->puSimR;    }
// returns the pointer to the dynamic simulation patterns (their number is returned by Fraig_ManReadPatternNumDynamic or Fraig_ManReadPatternNumDynamicFiltered)
// if the number of patterns is not evenly divisible by 32, the patterns beyond the given number contain garbage
unsigned *      Fraig_NodeReadPatternsDynamic( Fraig_Node_t * p )         { return p->puSimD;    }

/**Function*************************************************************

  Synopsis    [Access functions to set the data members of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void            Fraig_NodeSetData0( Fraig_Node_t * p, Fraig_Node_t * pData )      { p->pData0  = pData;  }
void            Fraig_NodeSetData1( Fraig_Node_t * p, Fraig_Node_t * pData )      { p->pData1  = pData;  }

/**Function*************************************************************

  Synopsis    [Checks the type of the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int             Fraig_NodeIsConst( Fraig_Node_t * p )    {  return (Fraig_Regular(p))->Num   == 0;  }
int             Fraig_NodeIsVar( Fraig_Node_t * p )      {  return (Fraig_Regular(p))->NumPi >= 0;  }
int             Fraig_NodeIsAnd( Fraig_Node_t * p )      {  return (Fraig_Regular(p))->NumPi <  0 && (Fraig_Regular(p))->Num > 0;  }
int             Fraig_NodeComparePhase( Fraig_Node_t * p1, Fraig_Node_t * p2 ) { assert( !Fraig_IsComplement(p1) ); assert( !Fraig_IsComplement(p2) ); return p1->fInv ^ p2->fInv; }

/**Function*************************************************************

  Synopsis    [Returns a new primary input node.]

  Description [If the node with this number does not exist, 
  create a new PI node with this number.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_ManReadIthVar( Fraig_Man_t * p, int i )
{
    int k;
    if ( i < 0 )
    {
        printf( "Requesting a PI with a negative number\n" );
        return NULL;
    }
    // create the PIs to fill in the interval
    if ( i >= p->vInputs->nSize )
        for ( k = p->vInputs->nSize; k <= i; k++ )
            Fraig_NodeCreatePi( p ); 
    return p->vInputs->pArray[i];
}

/**Function*************************************************************

  Synopsis    [Creates a new PO node.]

  Description [This procedure may take a complemented node.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_ManSetPo( Fraig_Man_t * p, Fraig_Node_t * pNode )
{
    // internal node may be a PO two times
    Fraig_Regular(pNode)->fNodePo = 1;
    Fraig_NodeVecPush( p->vOutputs, pNode );
}

/**Function*************************************************************

  Synopsis    [Perfoms the AND operation with functional hashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeAnd( Fraig_Man_t * p, Fraig_Node_t * p1, Fraig_Node_t * p2 )
{
    return Fraig_NodeAndCanon( p, p1, p2 );
}

/**Function*************************************************************

  Synopsis    [Perfoms the OR operation with functional hashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeOr( Fraig_Man_t * p, Fraig_Node_t * p1, Fraig_Node_t * p2 )
{
    return Fraig_Not( Fraig_NodeAndCanon( p, Fraig_Not(p1), Fraig_Not(p2) ) );
}

/**Function*************************************************************

  Synopsis    [Perfoms the EXOR operation with functional hashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeExor( Fraig_Man_t * p, Fraig_Node_t * p1, Fraig_Node_t * p2 )
{
    return Fraig_NodeMux( p, p1, Fraig_Not(p2), p2 );
}

/**Function*************************************************************

  Synopsis    [Perfoms the MUX operation with functional hashing.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Fraig_Node_t * Fraig_NodeMux( Fraig_Man_t * p, Fraig_Node_t * pC, Fraig_Node_t * pT, Fraig_Node_t * pE )
{
    Fraig_Node_t * pAnd1, * pAnd2, * pRes;
    pAnd1 = Fraig_NodeAndCanon( p, pC,          pT );     Fraig_Ref( pAnd1 );
    pAnd2 = Fraig_NodeAndCanon( p, Fraig_Not(pC), pE );   Fraig_Ref( pAnd2 );
    pRes  = Fraig_NodeOr( p, pAnd1, pAnd2 ); 
    Fraig_RecursiveDeref( p, pAnd1 );
    Fraig_RecursiveDeref( p, pAnd2 );
    Fraig_Deref( pRes );
    return pRes;
}


/**Function*************************************************************

  Synopsis    [Sets the node to be equivalent to the given one.]

  Description [This procedure is a work-around for the equivalence check.
  Does not verify the equivalence. Use at the user's risk.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Fraig_NodeSetChoice( Fraig_Man_t * pMan, Fraig_Node_t * pNodeOld, Fraig_Node_t * pNodeNew )
{
//    assert( pMan->fChoicing );
    pNodeNew->pNextE = pNodeOld->pNextE;
    pNodeOld->pNextE = pNodeNew;
    pNodeNew->pRepr  = pNodeOld;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

