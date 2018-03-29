/**CFile****************************************************************

  FileName    [abcOdc.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Scalable computation of observability don't-cares.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcOdc.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#define ABC_DC_MAX_NODES   (1<<15)

typedef unsigned short Odc_Lit_t;

typedef struct Odc_Obj_t_ Odc_Obj_t;     // 16 bytes
struct Odc_Obj_t_
{
    Odc_Lit_t               iFan0;       // first fanin
    Odc_Lit_t               iFan1;       // second fanin
    Odc_Lit_t               iNext;       // next node in the hash table
    unsigned short          TravId;      // the traversal ID
    unsigned                uData;       // the computed data
    unsigned                uMask;       // the variable mask 
};

struct Odc_Man_t_
{
    // dont'-care parameters
    int                     nVarsMax;    // the max number of cut variables
    int                     nLevels;     // the number of ODC levels
    int                     fVerbose;    // the verbosiness flag
    int                     fVeryVerbose;// the verbosiness flag to print per-node stats
    int                     nPercCutoff; // cutoff percentage

    // windowing
    Abc_Obj_t *             pNode;       // the node for windowing
    Vec_Ptr_t *             vLeaves;     // the number of the cut
    Vec_Ptr_t *             vRoots;      // the roots of the cut
    Vec_Ptr_t *             vBranches;   // additional inputs 

    // internal AIG package
    // objects
    int                     nPis;        // number of PIs (nVarsMax + 32)
    int                     nObjs;       // number of objects (Const1, PIs, ANDs)
    int                     nObjsAlloc;  // number of objects allocated
    Odc_Obj_t *             pObjs;       // objects 
    Odc_Lit_t               iRoot;       // the root object
    unsigned short          nTravIds;    // the number of travIDs
    // structural hashing
    Odc_Lit_t *             pTable;      // hash table
    int                     nTableSize;  // hash table size
    Vec_Int_t *             vUsedSpots;  // the used spots

    // truth tables
    int                     nBits;       // the number of bits
    int                     nWords;      // the number of words 
    Vec_Ptr_t *             vTruths;     // truth tables for each node
    Vec_Ptr_t *             vTruthsElem; // elementary truth tables for the PIs
    unsigned *              puTruth;     // the place where the resulting truth table does

    // statistics
    int                     nWins;       // the number of windows processed
    int                     nWinsEmpty;  // the number of empty windows
    int                     nSimsEmpty;  // the number of empty simulation infos
    int                     nQuantsOver; // the number of quantification overflows
    int                     nWinsFinish; // the number of windows that finished
    int                     nTotalDcs;   // total percentage of DCs

    // runtime
    abctime                 timeClean;   // windowing
    abctime                 timeWin;     // windowing
    abctime                 timeMiter;   // computing the miter
    abctime                 timeSim;     // simulation
    abctime                 timeQuant;   // quantification
    abctime                 timeTruth;   // truth table
    abctime                 timeTotal;   // useful runtime
    abctime                 timeAbort;   // aborted runtime
};


// quantity of different objects
static inline int           Odc_PiNum( Odc_Man_t * p )                     { return p->nPis;                       }
static inline int           Odc_NodeNum( Odc_Man_t * p )                   { return p->nObjs - p->nPis - 1;        }
static inline int           Odc_ObjNum( Odc_Man_t * p )                    { return p->nObjs;                      }

// complemented attributes of objects
static inline int           Odc_IsComplement( Odc_Lit_t Lit )              { return Lit &  (Odc_Lit_t)1;           }
static inline Odc_Lit_t     Odc_Regular( Odc_Lit_t Lit )                   { return Lit & ~(Odc_Lit_t)1;           }
static inline Odc_Lit_t     Odc_Not( Odc_Lit_t Lit )                       { return Lit ^  (Odc_Lit_t)1;           }
static inline Odc_Lit_t     Odc_NotCond( Odc_Lit_t Lit, int c )            { return Lit ^  (Odc_Lit_t)(c!=0);      }

// specialized Literals
static inline Odc_Lit_t     Odc_Const0()                                   { return 1;                             }
static inline Odc_Lit_t     Odc_Const1()                                   { return 0;                             }
static inline Odc_Lit_t     Odc_Var( Odc_Man_t * p, int i )                { assert( i >= 0 && i < p->nPis ); return (i+1) << 1;  }
static inline int           Odc_IsConst( Odc_Lit_t Lit )                   { return Lit <  (Odc_Lit_t)2;           }
static inline int           Odc_IsTerm( Odc_Man_t * p, Odc_Lit_t Lit )     { return (int)(Lit>>1) <= p->nPis;      }

// accessing internal storage
static inline Odc_Obj_t *   Odc_ObjNew( Odc_Man_t * p )                    { assert( p->nObjs < p->nObjsAlloc ); return p->pObjs + p->nObjs++;        }
static inline Odc_Lit_t     Odc_Obj2Lit( Odc_Man_t * p, Odc_Obj_t * pObj ) { assert( pObj ); return (pObj - p->pObjs) << 1;                           }
static inline Odc_Obj_t *   Odc_Lit2Obj( Odc_Man_t * p, Odc_Lit_t Lit )    { assert( !(Lit & 1) && (int)(Lit>>1) < p->nObjs ); return p->pObjs + (Lit>>1); }

// fanins and their complements
static inline Odc_Lit_t     Odc_ObjChild0( Odc_Obj_t * pObj )              { return pObj->iFan0;                   }
static inline Odc_Lit_t     Odc_ObjChild1( Odc_Obj_t * pObj )              { return pObj->iFan1;                   }
static inline Odc_Lit_t     Odc_ObjFanin0( Odc_Obj_t * pObj )              { return Odc_Regular(pObj->iFan0);      }
static inline Odc_Lit_t     Odc_ObjFanin1( Odc_Obj_t * pObj )              { return Odc_Regular(pObj->iFan1);      }
static inline int           Odc_ObjFaninC0( Odc_Obj_t * pObj )             { return Odc_IsComplement(pObj->iFan0); }
static inline int           Odc_ObjFaninC1( Odc_Obj_t * pObj )             { return Odc_IsComplement(pObj->iFan1); }

// traversal IDs
static inline void          Odc_ManIncrementTravId( Odc_Man_t * p )                         { p->nTravIds++;                                    }
static inline void          Odc_ObjSetTravIdCurrent( Odc_Man_t * p, Odc_Obj_t * pObj )      { pObj->TravId = p->nTravIds;                       }
static inline int           Odc_ObjIsTravIdCurrent( Odc_Man_t * p, Odc_Obj_t * pObj )       { return (int )((int)pObj->TravId == p->nTravIds);  }

// truth tables
static inline unsigned *    Odc_ObjTruth( Odc_Man_t * p, Odc_Lit_t Lit )   { assert( !(Lit & 1) ); return (unsigned *) Vec_PtrEntry(p->vTruths, Lit >> 1);  }

// iterators 
#define Odc_ForEachPi( p, Lit, i )                                                 \
    for ( i = 0; (i < Odc_PiNum(p)) && (((Lit) = Odc_Var(p, i)), 1); i++ )
#define Odc_ForEachAnd( p, pObj, i )                                               \
    for ( i = 1 + Odc_CiNum(p); (i < Odc_ObjNum(p)) && ((pObj) = (p)->pObjs + i); i++ )


////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Allocates the don't-care manager.]

  Description [The parameters are the max number of cut variables, 
  the number of fanout levels used for the ODC computation, and verbosiness.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Odc_Man_t * Abc_NtkDontCareAlloc( int nVarsMax, int nLevels, int fVerbose, int fVeryVerbose )
{
    Odc_Man_t * p;
    unsigned * pData;
    int i, k;
    p = ABC_ALLOC( Odc_Man_t, 1 );
    memset( p, 0, sizeof(Odc_Man_t) );
    assert( nVarsMax > 4 && nVarsMax < 16 );
    assert( nLevels > 0 && nLevels < 10 );

    srand( 0xABC );

    // dont'-care parameters
    p->nVarsMax     = nVarsMax;
    p->nLevels      = nLevels;
    p->fVerbose     = fVerbose;
    p->fVeryVerbose = fVeryVerbose;
    p->nPercCutoff  = 10;

    // windowing
    p->vRoots    = Vec_PtrAlloc( 128 );
    p->vBranches = Vec_PtrAlloc( 128 );

    // internal AIG package
    // allocate room for objects
    p->nObjsAlloc = ABC_DC_MAX_NODES; 
    p->pObjs = ABC_ALLOC( Odc_Obj_t, p->nObjsAlloc * sizeof(Odc_Obj_t) );
    p->nPis  = nVarsMax + 32;
    p->nObjs = 1 + p->nPis;
    memset( p->pObjs, 0, p->nObjs * sizeof(Odc_Obj_t) );
    // set the PI masks
    for ( i = 0; i < 32; i++ )
        p->pObjs[1 + p->nVarsMax + i].uMask = (1 << i);
    // allocate hash table
    p->nTableSize = p->nObjsAlloc/3 + 1;
    p->pTable = ABC_ALLOC( Odc_Lit_t, p->nTableSize * sizeof(Odc_Lit_t) );
    memset( p->pTable, 0, p->nTableSize * sizeof(Odc_Lit_t) );
    p->vUsedSpots = Vec_IntAlloc( 1000 );

    // truth tables
    p->nWords = Abc_TruthWordNum( p->nVarsMax );
    p->nBits = p->nWords * 8 * sizeof(unsigned);
    p->vTruths = Vec_PtrAllocSimInfo( p->nObjsAlloc, p->nWords );
    p->vTruthsElem = Vec_PtrAllocSimInfo( p->nVarsMax, p->nWords );

    // set elementary truth tables
    Abc_InfoFill( (unsigned *)Vec_PtrEntry(p->vTruths, 0), p->nWords );
    for ( k = 0; k < p->nVarsMax; k++ )
    {
//        pData = Odc_ObjTruth( p, Odc_Var(p, k) );
        pData = (unsigned *)Vec_PtrEntry( p->vTruthsElem, k );
        Abc_InfoClear( pData, p->nWords );
        for ( i = 0; i < p->nBits; i++ )
            if ( i & (1 << k) )
                pData[i>>5] |= (1 << (i&31));
    }

    // set random truth table for the additional inputs
    for ( k = p->nVarsMax; k < p->nPis; k++ )
    {
        pData = Odc_ObjTruth( p, Odc_Var(p, k) );
        Abc_InfoRandom( pData, p->nWords );
    }

    // set the miter to the unused value
    p->iRoot = 0xffff;
    return p;
}

/**Function*************************************************************

  Synopsis    [Clears the manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDontCareClear( Odc_Man_t * p )
{
    abctime clk = Abc_Clock();
    // clean the structural hashing table
    if ( Vec_IntSize(p->vUsedSpots) > p->nTableSize/3 ) // more than one third
        memset( p->pTable, 0, sizeof(Odc_Lit_t) * p->nTableSize );
    else
    {
        int iSpot, i;
        Vec_IntForEachEntry( p->vUsedSpots, iSpot, i )
            p->pTable[iSpot] = 0;
    }
    Vec_IntClear( p->vUsedSpots ); 
    // reset the number of nodes
    p->nObjs = 1 + p->nPis;
    // reset the root node
    p->iRoot = 0xffff;

p->timeClean += Abc_Clock() - clk;
}

/**Function*************************************************************

  Synopsis    [Frees the don't-care manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDontCareFree( Odc_Man_t * p )
{
    if ( p->fVerbose )
    {
        printf( "Wins = %5d. Empty = %5d. SimsEmpty = %5d. QuantOver = %5d. WinsFinish = %5d.\n", 
            p->nWins, p->nWinsEmpty, p->nSimsEmpty, p->nQuantsOver, p->nWinsFinish );
        printf( "Ave DCs per window = %6.2f %%. Ave DCs per finished window = %6.2f %%.\n", 
            1.0*p->nTotalDcs/p->nWins, 1.0*p->nTotalDcs/p->nWinsFinish );
        printf( "Runtime stats of the ODC manager:\n" );
        ABC_PRT( "Cleaning    ", p->timeClean );
        ABC_PRT( "Windowing   ", p->timeWin   );
        ABC_PRT( "Miter       ", p->timeMiter );
        ABC_PRT( "Simulation  ", p->timeSim   );
        ABC_PRT( "Quantifying ", p->timeQuant );
        ABC_PRT( "Truth table ", p->timeTruth );
        ABC_PRT( "TOTAL       ", p->timeTotal );
        ABC_PRT( "Aborted     ", p->timeAbort );
    }
    Vec_PtrFree( p->vRoots );
    Vec_PtrFree( p->vBranches );
    Vec_PtrFree( p->vTruths );
    Vec_PtrFree( p->vTruthsElem );
    Vec_IntFree( p->vUsedSpots );
    ABC_FREE( p->pObjs );
    ABC_FREE( p->pTable );
    ABC_FREE( p );
}



/**Function*************************************************************

  Synopsis    [Marks the TFO of the collected nodes up to the given level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDontCareWinSweepLeafTfo_rec( Abc_Obj_t * pObj, int nLevelLimit, Abc_Obj_t * pNode )
{
    Abc_Obj_t * pFanout;
    int i;
    if ( Abc_ObjIsCo(pObj) || (int)pObj->Level > nLevelLimit || pObj == pNode )
        return;
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return;
    Abc_NodeSetTravIdCurrent( pObj );
    ////////////////////////////////////////
    // try to reduce the runtime
    if ( Abc_ObjFanoutNum(pObj) > 100 )
        return;
    ////////////////////////////////////////
    Abc_ObjForEachFanout( pObj, pFanout, i )
        Abc_NtkDontCareWinSweepLeafTfo_rec( pFanout, nLevelLimit, pNode );
}

/**Function*************************************************************

  Synopsis    [Marks the TFO of the collected nodes up to the given level.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDontCareWinSweepLeafTfo( Odc_Man_t * p )
{
    Abc_Obj_t * pObj;
    int i;
    Abc_NtkIncrementTravId( p->pNode->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pObj, i )
        Abc_NtkDontCareWinSweepLeafTfo_rec( pObj, p->pNode->Level + p->nLevels, p->pNode );
}

/**Function*************************************************************

  Synopsis    [Recursively collects the roots.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDontCareWinCollectRoots_rec( Abc_Obj_t * pObj, Vec_Ptr_t * vRoots )
{
    Abc_Obj_t * pFanout;
    int i;
    assert( Abc_ObjIsNode(pObj) );
    assert( Abc_NodeIsTravIdCurrent(pObj) );
    // check if the node has all fanouts marked
    Abc_ObjForEachFanout( pObj, pFanout, i )
        if ( !Abc_NodeIsTravIdCurrent(pFanout) )
            break;
    // if some of the fanouts are unmarked, add the node to the root
    if ( i < Abc_ObjFanoutNum(pObj) ) 
    {
        Vec_PtrPushUnique( vRoots, pObj );
        return;
    }
    // otherwise, call recursively
    Abc_ObjForEachFanout( pObj, pFanout, i )
        Abc_NtkDontCareWinCollectRoots_rec( pFanout, vRoots );
}

/**Function*************************************************************

  Synopsis    [Collects the roots of the window.]

  Description [Roots of the window are the nodes that have at least
  one fanout that it not in the TFO of the leaves.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDontCareWinCollectRoots( Odc_Man_t * p )
{
    assert( !Abc_NodeIsTravIdCurrent(p->pNode) );
    // mark the node with the old traversal ID
    Abc_NodeSetTravIdCurrent( p->pNode ); 
    // collect the roots
    Vec_PtrClear( p->vRoots );
    Abc_NtkDontCareWinCollectRoots_rec( p->pNode, p->vRoots );
}
 
/**Function*************************************************************

  Synopsis    [Recursively adds missing nodes and leaves.]

  Description []
               
  SideEffects []

  SeeAlso     [] 

***********************************************************************/
int Abc_NtkDontCareWinAddMissing_rec( Odc_Man_t * p, Abc_Obj_t * pObj )
{
    Abc_Obj_t * pFanin;
    int i;
    // skip the already collected leaves and branches
    if ( Abc_NodeIsTravIdCurrent(pObj) )
        return 1;
    // if this is not an internal node - make it a new branch
    if ( !Abc_NodeIsTravIdPrevious(pObj) || Abc_ObjIsCi(pObj) ) //|| (int)pObj->Level <= p->nLevLeaves )
    {
        Abc_NodeSetTravIdCurrent( pObj );
        Vec_PtrPush( p->vBranches, pObj );
        return Vec_PtrSize(p->vBranches) <= 32;
    }
    // visit the fanins of the node
    Abc_ObjForEachFanin( pObj, pFanin, i )
        if ( !Abc_NtkDontCareWinAddMissing_rec( p, pFanin ) )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Adds to the window nodes and leaves in the TFI of the roots.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDontCareWinAddMissing( Odc_Man_t * p )
{
    Abc_Obj_t * pObj;
    int i;
    // set the leaves
    Abc_NtkIncrementTravId( p->pNode->pNtk );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pObj, i )
        Abc_NodeSetTravIdCurrent( pObj );        
    // explore from the roots
    Vec_PtrClear( p->vBranches );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vRoots, pObj, i )
        if ( !Abc_NtkDontCareWinAddMissing_rec( p, pObj ) )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Computes window for the node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDontCareWindow( Odc_Man_t * p )
{
    // mark the TFO of the collected nodes up to the given level (p->pNode->Level + p->nWinTfoMax)
    Abc_NtkDontCareWinSweepLeafTfo( p );
    // find the roots of the window
    Abc_NtkDontCareWinCollectRoots( p );
    if ( Vec_PtrSize(p->vRoots) == 1 && Vec_PtrEntry(p->vRoots, 0) == p->pNode )
    {
//        printf( "Empty window\n" );
        return 0;
    }
    // add the nodes in the TFI of the roots that are not yet in the window
    if ( !Abc_NtkDontCareWinAddMissing( p ) )
    {
//        printf( "Too many branches (%d)\n", Vec_PtrSize(p->vBranches) );
        return 0;
    }
    return 1;
}





/**Function*************************************************************

  Synopsis    [Performing hashing of two AIG Literals.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline unsigned Odc_HashKey( Odc_Lit_t iFan0, Odc_Lit_t iFan1, int TableSize ) 
{
    unsigned Key = 0;
    Key ^= Odc_Regular(iFan0) * 7937;
    Key ^= Odc_Regular(iFan1) * 2971;
    Key ^= Odc_IsComplement(iFan0) * 911;
    Key ^= Odc_IsComplement(iFan1) * 353;
    return Key % TableSize;
}

/**Function*************************************************************

  Synopsis    [Checks if the given name node already exists in the table.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Odc_Lit_t * Odc_HashLookup( Odc_Man_t * p, Odc_Lit_t iFan0, Odc_Lit_t iFan1 )
{
    Odc_Obj_t * pObj;
    Odc_Lit_t * pEntry;
    unsigned uHashKey;
    assert( iFan0 < iFan1 );
    // get the hash key for this node
    uHashKey = Odc_HashKey( iFan0, iFan1, p->nTableSize );
    // remember the spot in the hash table that will be used
    if ( p->pTable[uHashKey] == 0 )
        Vec_IntPush( p->vUsedSpots, uHashKey );
    // find the entry
    for ( pEntry = p->pTable + uHashKey; *pEntry; pEntry = &pObj->iNext )
    {
        pObj = Odc_Lit2Obj( p, *pEntry );
        if ( pObj->iFan0 == iFan0 && pObj->iFan1 == iFan1 )
            return pEntry;
    }
    return pEntry;
}

/**Function*************************************************************

  Synopsis    [Finds node by structural hashing or creates a new node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Odc_Lit_t Odc_And( Odc_Man_t * p, Odc_Lit_t iFan0, Odc_Lit_t iFan1 )
{
    Odc_Obj_t * pObj;
    Odc_Lit_t * pEntry;
    unsigned uMask0, uMask1;
    int Temp;
    // consider trivial cases
    if ( iFan0 == iFan1 )
        return iFan0;
    if ( iFan0 == Odc_Not(iFan1) )
        return Odc_Const0();
    if ( Odc_Regular(iFan0) == Odc_Const1() )
        return iFan0 == Odc_Const1() ? iFan1 : Odc_Const0();
    if ( Odc_Regular(iFan1) == Odc_Const1() )
        return iFan1 == Odc_Const1() ? iFan0 : Odc_Const0();
    // canonicize the fanin order
    if ( iFan0 > iFan1 )
        Temp = iFan0, iFan0 = iFan1, iFan1 = Temp;
    // check if a node with these fanins exists
    pEntry = Odc_HashLookup( p, iFan0, iFan1 );
    if ( *pEntry )
        return *pEntry;
    // create a new node
    pObj = Odc_ObjNew( p );
    pObj->iFan0 = iFan0;
    pObj->iFan1 = iFan1;
    pObj->iNext = 0;
    pObj->TravId = 0;
    // set the mask
    uMask0 = Odc_Lit2Obj(p, Odc_Regular(iFan0))->uMask;
    uMask1 = Odc_Lit2Obj(p, Odc_Regular(iFan1))->uMask;
    pObj->uMask = uMask0 | uMask1;
    // add to the table
    *pEntry = Odc_Obj2Lit( p, pObj );
    return *pEntry;
}

/**Function*************************************************************

  Synopsis    [Boolean OR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Odc_Lit_t Odc_Or( Odc_Man_t * p, Odc_Lit_t iFan0, Odc_Lit_t iFan1 )
{
    return Odc_Not( Odc_And(p, Odc_Not(iFan0), Odc_Not(iFan1)) );
}

/**Function*************************************************************

  Synopsis    [Boolean XOR.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
static inline Odc_Lit_t Odc_Xor( Odc_Man_t * p, Odc_Lit_t iFan0, Odc_Lit_t iFan1 )
{
    return Odc_Or( p, Odc_And(p, iFan0, Odc_Not(iFan1)), Odc_And(p, Odc_Not(iFan0), iFan1) );
}





/**Function*************************************************************

  Synopsis    [Transfers the window into the AIG package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void * Abc_NtkDontCareTransfer_rec( Odc_Man_t * p, Abc_Obj_t * pNode, Abc_Obj_t * pPivot )
{
    unsigned uData0, uData1;
    Odc_Lit_t uLit0, uLit1, uRes0, uRes1;
    assert( !Abc_ObjIsComplement(pNode) );
    // skip visited objects
    if ( Abc_NodeIsTravIdCurrent(pNode) )
        return pNode->pCopy;
    Abc_NodeSetTravIdCurrent(pNode);
    assert( Abc_ObjIsNode(pNode) );
    // consider the case when the node is the pivot
    if ( pNode == pPivot )
        return pNode->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)((Odc_Const1() << 16) | Odc_Const0());
    // compute the cofactors
    uData0 = (unsigned)(ABC_PTRUINT_T)Abc_NtkDontCareTransfer_rec( p, Abc_ObjFanin0(pNode), pPivot );
    uData1 = (unsigned)(ABC_PTRUINT_T)Abc_NtkDontCareTransfer_rec( p, Abc_ObjFanin1(pNode), pPivot );
    // find the 0-cofactor
    uLit0 = Odc_NotCond( (Odc_Lit_t)(uData0 & 0xffff), Abc_ObjFaninC0(pNode) );
    uLit1 = Odc_NotCond( (Odc_Lit_t)(uData1 & 0xffff), Abc_ObjFaninC1(pNode) );
    uRes0 = Odc_And( p, uLit0, uLit1 );
    // find the 1-cofactor
    uLit0 = Odc_NotCond( (Odc_Lit_t)(uData0 >> 16), Abc_ObjFaninC0(pNode) );
    uLit1 = Odc_NotCond( (Odc_Lit_t)(uData1 >> 16), Abc_ObjFaninC1(pNode) );
    uRes1 = Odc_And( p, uLit0, uLit1 );
    // find the result
    return pNode->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)((uRes1 << 16) | uRes0);
}

/**Function*************************************************************

  Synopsis    [Transfers the window into the AIG package.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDontCareTransfer( Odc_Man_t * p )
{
    Abc_Obj_t * pObj;
    Odc_Lit_t uRes0, uRes1;
    Odc_Lit_t uLit;
    unsigned uData;
    int i;
    Abc_NtkIncrementTravId( p->pNode->pNtk );
    // set elementary variables at the leaves 
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vLeaves, pObj, i )
    {
        uLit = Odc_Var( p, i );
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)((uLit << 16) | uLit);
        Abc_NodeSetTravIdCurrent(pObj);
    }
    // set elementary variables at the branched 
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vBranches, pObj, i )
    {
        uLit = Odc_Var( p, i+p->nVarsMax );
        pObj->pCopy = (Abc_Obj_t *)(ABC_PTRUINT_T)((uLit << 16) | uLit);
        Abc_NodeSetTravIdCurrent(pObj);
    }
    // compute the AIG for the window
    p->iRoot = Odc_Const0();
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vRoots, pObj, i )
    {
        uData = (unsigned)(ABC_PTRUINT_T)Abc_NtkDontCareTransfer_rec( p, pObj, p->pNode );
        // get the cofactors
        uRes0 = uData & 0xffff;
        uRes1 = uData >> 16;
        // compute the miter
//        assert( uRes0 != uRes1 ); // may be false if the node is redundant w.r.t. this root
        uLit = Odc_Xor( p, uRes0, uRes1 );
        p->iRoot = Odc_Or( p, p->iRoot, uLit );
    }
    return 1;
}


/**Function*************************************************************

  Synopsis    [Recursively computes the pair of cofactors.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned Abc_NtkDontCareCofactors_rec( Odc_Man_t * p, Odc_Lit_t Lit, unsigned uMask )
{
    Odc_Obj_t * pObj;
    unsigned uData0, uData1;
    Odc_Lit_t uLit0, uLit1, uRes0, uRes1;
    assert( !Odc_IsComplement(Lit) );
    // skip visited objects
    pObj = Odc_Lit2Obj( p, Lit );
    if ( Odc_ObjIsTravIdCurrent(p, pObj) )
        return pObj->uData;
    Odc_ObjSetTravIdCurrent(p, pObj);
    // skip objects out of the cone
    if ( (pObj->uMask & uMask) == 0 )
        return pObj->uData = ((Lit << 16) | Lit);
    // consider the case when the node is the var
    if ( pObj->uMask == uMask && Odc_IsTerm(p, Lit) )
        return pObj->uData = ((Odc_Const1() << 16) | Odc_Const0());
    // compute the cofactors
    uData0 = Abc_NtkDontCareCofactors_rec( p, Odc_ObjFanin0(pObj), uMask );
    uData1 = Abc_NtkDontCareCofactors_rec( p, Odc_ObjFanin1(pObj), uMask );
    // find the 0-cofactor
    uLit0 = Odc_NotCond( (Odc_Lit_t)(uData0 & 0xffff), Odc_ObjFaninC0(pObj) );
    uLit1 = Odc_NotCond( (Odc_Lit_t)(uData1 & 0xffff), Odc_ObjFaninC1(pObj) );
    uRes0 = Odc_And( p, uLit0, uLit1 );
    // find the 1-cofactor
    uLit0 = Odc_NotCond( (Odc_Lit_t)(uData0 >> 16), Odc_ObjFaninC0(pObj) );
    uLit1 = Odc_NotCond( (Odc_Lit_t)(uData1 >> 16), Odc_ObjFaninC1(pObj) );
    uRes1 = Odc_And( p, uLit0, uLit1 );
    // find the result
    return pObj->uData = ((uRes1 << 16) | uRes0);
}

/**Function*************************************************************

  Synopsis    [Quantifies the branch variables.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDontCareQuantify( Odc_Man_t * p )
{   
    Odc_Lit_t uRes0, uRes1;
    unsigned uData;
    int i;
    assert( p->iRoot < 0xffff );
    assert( Vec_PtrSize(p->vBranches) <= 32 ); // the mask size
    for ( i = 0; i < Vec_PtrSize(p->vBranches); i++ )
    {
        // compute the cofactors w.r.t. this variable
        Odc_ManIncrementTravId( p );
        uData = Abc_NtkDontCareCofactors_rec( p, Odc_Regular(p->iRoot), (1 << i) );
        uRes0 = Odc_NotCond( (Odc_Lit_t)(uData & 0xffff), Odc_IsComplement(p->iRoot) );
        uRes1 = Odc_NotCond( (Odc_Lit_t)(uData >> 16),    Odc_IsComplement(p->iRoot) );
        // quantify this variable existentially
        p->iRoot = Odc_Or( p, uRes0, uRes1 );
        // check the limit
        if ( Odc_ObjNum(p) > ABC_DC_MAX_NODES/2 )
            return 0;
    }
    assert( p->nObjs <= p->nObjsAlloc );
    return 1;
}



/**Function*************************************************************

  Synopsis    [Set elementary truth tables for PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDontCareSimulateSetElem2( Odc_Man_t * p )
{
    unsigned * pData;
    int i, k;
    for ( k = 0; k < p->nVarsMax; k++ )
    {
        pData = Odc_ObjTruth( p, Odc_Var(p, k) );
        Abc_InfoClear( pData, p->nWords );
        for ( i = 0; i < p->nBits; i++ )
            if ( i & (1 << k) )
                pData[i>>5] |= (1 << (i&31));
    }
}

/**Function*************************************************************

  Synopsis    [Set elementary truth tables for PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDontCareSimulateSetElem( Odc_Man_t * p )
{
    unsigned * pData, * pData2;
    int k;
    for ( k = 0; k < p->nVarsMax; k++ )
    {
        pData = Odc_ObjTruth( p, Odc_Var(p, k) );
        pData2 = (unsigned *)Vec_PtrEntry( p->vTruthsElem, k );
        Abc_InfoCopy( pData, pData2, p->nWords );
    }
}

/**Function*************************************************************

  Synopsis    [Set random simulation words for PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDontCareSimulateSetRand( Odc_Man_t * p )
{
    unsigned * pData;
    int w, k, Number;
    for ( w = 0; w < p->nWords; w++ )
    {
        Number = rand();
        for ( k = 0; k < p->nVarsMax; k++ )
        {
            pData = Odc_ObjTruth( p, Odc_Var(p, k) );
            pData[w] = (Number & (1<<k)) ? ~0 : 0;
        }
    }
}

/**Function*************************************************************

  Synopsis    [Set random simulation words for PIs.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDontCareCountMintsWord( Odc_Man_t * p, unsigned * puTruth )
{
    int w, Counter = 0;
    for ( w = 0; w < p->nWords; w++ )
        if ( puTruth[w] )
            Counter++;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Simulates one node.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDontCareTruthOne( Odc_Man_t * p, Odc_Lit_t Lit )
{
    Odc_Obj_t * pObj;
    unsigned * pInfo, * pInfo1, * pInfo2;
    int k, fComp1, fComp2;
    assert( !Odc_IsComplement( Lit ) );
    assert( !Odc_IsTerm( p, Lit ) );
    // get the truth tables
    pObj   = Odc_Lit2Obj( p, Lit );
    pInfo  = Odc_ObjTruth( p, Lit );
    pInfo1 = Odc_ObjTruth( p, Odc_ObjFanin0(pObj) );
    pInfo2 = Odc_ObjTruth( p, Odc_ObjFanin1(pObj) );
    fComp1 = Odc_ObjFaninC0( pObj );
    fComp2 = Odc_ObjFaninC1( pObj );
    // simulate
    if ( fComp1 && fComp2 )
        for ( k = 0; k < p->nWords; k++ )
            pInfo[k] = ~pInfo1[k] & ~pInfo2[k];
    else if ( fComp1 && !fComp2 )
        for ( k = 0; k < p->nWords; k++ )
            pInfo[k] = ~pInfo1[k] &  pInfo2[k];
    else if ( !fComp1 && fComp2 )
        for ( k = 0; k < p->nWords; k++ )
            pInfo[k] =  pInfo1[k] & ~pInfo2[k];
    else // if ( fComp1 && fComp2 )
        for ( k = 0; k < p->nWords; k++ )
            pInfo[k] =  pInfo1[k] &  pInfo2[k];
}

/**Function*************************************************************

  Synopsis    [Computes the truth table.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Abc_NtkDontCareSimulate_rec( Odc_Man_t * p, Odc_Lit_t Lit )
{
    Odc_Obj_t * pObj;
    assert( !Odc_IsComplement(Lit) );
    // skip terminals
    if ( Odc_IsTerm(p, Lit) )
        return;
    // skip visited objects
    pObj = Odc_Lit2Obj( p, Lit );
    if ( Odc_ObjIsTravIdCurrent(p, pObj) )
        return;
    Odc_ObjSetTravIdCurrent(p, pObj);
    // call recursively
    Abc_NtkDontCareSimulate_rec( p, Odc_ObjFanin0(pObj) );
    Abc_NtkDontCareSimulate_rec( p, Odc_ObjFanin1(pObj) );
    // construct the truth table
    Abc_NtkDontCareTruthOne( p, Lit );
}

/**Function*************************************************************

  Synopsis    [Computes the truth table of the care set.]

  Description [Returns the number of ones in the simulation info.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDontCareSimulate( Odc_Man_t * p, unsigned * puTruth )
{
    Odc_ManIncrementTravId( p );
    Abc_NtkDontCareSimulate_rec( p, Odc_Regular(p->iRoot) );
    Abc_InfoCopy( puTruth, Odc_ObjTruth(p, Odc_Regular(p->iRoot)), p->nWords );
    if ( Odc_IsComplement(p->iRoot) )
        Abc_InfoNot( puTruth, p->nWords );
    return Extra_TruthCountOnes( puTruth, p->nVarsMax );
}

/**Function*************************************************************

  Synopsis    [Computes the truth table of the care set.]

  Description [Returns the number of ones in the simulation info.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDontCareSimulateBefore( Odc_Man_t * p, unsigned * puTruth )
{
    int nIters = 2;
    int nRounds, Counter, r;
    // decide how many rounds to simulate
    nRounds = p->nBits / p->nWords;
    Counter = 0;
    for ( r = 0; r < nIters; r++ )
    {
        Abc_NtkDontCareSimulateSetRand( p );
        Abc_NtkDontCareSimulate( p, puTruth );
        Counter += Abc_NtkDontCareCountMintsWord( p, puTruth );
    }
    // normalize
    Counter = Counter * nRounds / nIters;
    return Counter;
}

/**Function*************************************************************

  Synopsis    [Computes ODCs for the node in terms of the cut variables.]

  Description [Returns the number of don't care minterms in the truth table.
  In particular, this procedure returns 0 if there is no don't-cares.]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkDontCareCompute( Odc_Man_t * p, Abc_Obj_t * pNode, Vec_Ptr_t * vLeaves, unsigned * puTruth )
{
    int nMints, RetValue;
    abctime clk, clkTotal = Abc_Clock();

    p->nWins++;
    
    // set the parameters
    assert( !Abc_ObjIsComplement(pNode) );
    assert( Abc_ObjIsNode(pNode) );
    assert( Vec_PtrSize(vLeaves) <= p->nVarsMax );
    p->vLeaves = vLeaves;
    p->pNode = pNode;

    // compute the window
clk = Abc_Clock();
    RetValue = Abc_NtkDontCareWindow( p );
p->timeWin += Abc_Clock() - clk;
    if ( !RetValue )
    {
p->timeAbort += Abc_Clock() - clkTotal;
        Abc_InfoFill( puTruth, p->nWords );
        p->nWinsEmpty++;        
        return 0;
    }

    if ( p->fVeryVerbose )
    {
        printf( " %5d : ", pNode->Id );
        printf( "Leaf = %2d ", Vec_PtrSize(p->vLeaves) );
        printf( "Root = %2d ", Vec_PtrSize(p->vRoots) );
        printf( "Bran = %2d ", Vec_PtrSize(p->vBranches) );
        printf( " |  " );
    }

    // transfer the window into the AIG package
clk = Abc_Clock();
    Abc_NtkDontCareTransfer( p );
p->timeMiter += Abc_Clock() - clk;

    // simulate to estimate the amount of don't-cares
clk = Abc_Clock();
    nMints = Abc_NtkDontCareSimulateBefore( p, puTruth );
p->timeSim += Abc_Clock() - clk;
    if ( p->fVeryVerbose )
    {
        printf( "AIG = %5d ", Odc_NodeNum(p) );
        printf( "%6.2f %%  ", 100.0 * (p->nBits - nMints) / p->nBits );
    }

    // if there is less then the given percentage of don't-cares, skip
    if ( 100.0 * (p->nBits - nMints) / p->nBits < 1.0 * p->nPercCutoff )
    {
p->timeAbort += Abc_Clock() - clkTotal;
        if ( p->fVeryVerbose )
            printf( "Simulation cutoff.\n" );
        Abc_InfoFill( puTruth, p->nWords );
        p->nSimsEmpty++;
        return 0;
    }

    // quantify external variables
clk = Abc_Clock();
    RetValue = Abc_NtkDontCareQuantify( p );
p->timeQuant += Abc_Clock() - clk;
    if ( !RetValue )
    {
p->timeAbort += Abc_Clock() - clkTotal;
        if ( p->fVeryVerbose )
            printf( "=== Overflow! ===\n" );
        Abc_InfoFill( puTruth, p->nWords );
        p->nQuantsOver++;
        return 0;
    }

    // get the truth table
clk = Abc_Clock();
    Abc_NtkDontCareSimulateSetElem( p );
    nMints = Abc_NtkDontCareSimulate( p, puTruth );
p->timeTruth += Abc_Clock() - clk;
    if ( p->fVeryVerbose )
    {
        printf( "AIG = %5d ", Odc_NodeNum(p) );
        printf( "%6.2f %%  ", 100.0 * (p->nBits - nMints) / p->nBits );
        printf( "\n" );
    }
p->timeTotal += Abc_Clock() - clkTotal;
    p->nWinsFinish++;
    p->nTotalDcs += (int)(100.0 * (p->nBits - nMints) / p->nBits);
    return nMints;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

