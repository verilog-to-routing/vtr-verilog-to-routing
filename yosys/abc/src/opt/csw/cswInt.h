/**CFile****************************************************************

  FileName    [cswInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Cut sweeping.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - July 11, 2007.]

  Revision    [$Id: cswInt.h,v 1.00 2007/07/11 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__csw__cswInt_h
#define ABC__aig__csw__cswInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "aig/aig/aig.h"
#include "opt/dar/dar.h"
#include "bool/kit/kit.h"
#include "csw.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START
 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Csw_Man_t_            Csw_Man_t;
typedef struct Csw_Cut_t_            Csw_Cut_t;

// the cut used to represent node in the AIG
struct Csw_Cut_t_
{
    Csw_Cut_t *     pNext;           // the next cut in the table 
    int             Cost;            // the cost of the cut
//    float           Cost;            // the cost of the cut
    unsigned        uSign;           // cut signature
    int             iNode;           // the node, for which it is the cut
    short           nCutSize;        // the number of bytes in the cut
    char            nLeafMax;        // the maximum number of fanins
    char            nFanins;         // the current number of fanins
    int             pFanins[0];      // the fanins (followed by the truth table)
};

// the CNF computation manager
struct Csw_Man_t_
{
    // AIG manager
    Aig_Man_t *     pManAig;         // the input AIG manager
    Aig_Man_t *     pManRes;         // the output AIG manager
    Aig_Obj_t **    pEquiv;          // the equivalent nodes in the resulting manager
    Csw_Cut_t **    pCuts;           // the cuts for each node in the output manager
    int *           pnRefs;          // the number of references of each new node
    // hash table for cuts
    Csw_Cut_t **    pTable;          // the table composed of cuts 
    int             nTableSize;      // the size of hash table
    // parameters
    int             nCutsMax;        // the max number of cuts at the node
    int             nLeafMax;        // the max number of leaves of a cut
    int             fVerbose;        // enables verbose output
    // internal variables
    int             nCutSize;        // the number of bytes needed to store one cut
    int             nTruthWords;     // the number of truth table words
    Aig_MmFixed_t * pMemCuts;        // memory manager for cuts
    unsigned *      puTemp[4];       // used for the truth table computation
    // statistics
    int             nNodesTriv0;     // the number of trivial nodes
    int             nNodesTriv1;     // the number of trivial nodes
    int             nNodesTriv2;     // the number of trivial nodes
    int             nNodesCuts;      // the number of rewritten nodes
    int             nNodesTried;     // the number of nodes tried
    abctime         timeCuts;        // time to compute the cut and its truth table
    abctime         timeHash;        // time for hashing cuts
    abctime         timeOther;       // other time
    abctime         timeTotal;       // total time
};

static inline int          Csw_CutLeaveNum( Csw_Cut_t * pCut )          { return pCut->nFanins;                                   }
static inline int *        Csw_CutLeaves( Csw_Cut_t * pCut )            { return pCut->pFanins;                                   }
static inline unsigned *   Csw_CutTruth( Csw_Cut_t * pCut )             { return (unsigned *)(pCut->pFanins + pCut->nLeafMax);    }
static inline Csw_Cut_t *  Csw_CutNext( Csw_Cut_t * pCut )              { return (Csw_Cut_t *)(((char *)pCut) + pCut->nCutSize);  }

static inline int          Csw_ObjRefs( Csw_Man_t * p, Aig_Obj_t * pObj )                         { return p->pnRefs[pObj->Id];   }
static inline void         Csw_ObjAddRefs( Csw_Man_t * p, Aig_Obj_t * pObj, int nRefs )           { p->pnRefs[pObj->Id] += nRefs; }

static inline Csw_Cut_t *  Csw_ObjCuts( Csw_Man_t * p, Aig_Obj_t * pObj )                         { return p->pCuts[pObj->Id];    }
static inline void         Csw_ObjSetCuts( Csw_Man_t * p, Aig_Obj_t * pObj, Csw_Cut_t * pCuts )   { p->pCuts[pObj->Id] = pCuts;   }

static inline Aig_Obj_t *  Csw_ObjEquiv( Csw_Man_t * p, Aig_Obj_t * pObj )                        { return p->pEquiv[pObj->Id];   }
static inline void         Csw_ObjSetEquiv( Csw_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pEquiv ) { p->pEquiv[pObj->Id] = pEquiv; }

static inline Aig_Obj_t *  Csw_ObjChild0Equiv( Csw_Man_t * p, Aig_Obj_t * pObj ) { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin0(pObj)? Aig_NotCond(Csw_ObjEquiv(p, Aig_ObjFanin0(pObj)), Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t *  Csw_ObjChild1Equiv( Csw_Man_t * p, Aig_Obj_t * pObj ) { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? Aig_NotCond(Csw_ObjEquiv(p, Aig_ObjFanin1(pObj)), Aig_ObjFaninC1(pObj)) : NULL;  }

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                           ITERATORS                              ///
////////////////////////////////////////////////////////////////////////

// iterator over cuts of the node
#define Csw_ObjForEachCut( p, pObj, pCut, i )                           \
    for ( i = 0, pCut = Csw_ObjCuts(p, pObj); i < p->nCutsMax; i++, pCut = Csw_CutNext(pCut) ) 
// iterator over leaves of the cut
#define Csw_CutForEachLeaf( p, pCut, pLeaf, i )                         \
    for ( i = 0; (i < (int)(pCut)->nFanins) && ((pLeaf) = Aig_ManObj(p, (pCut)->pFanins[i])); i++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== cnfCut.c ========================================================*/
extern Csw_Cut_t *    Csw_ObjPrepareCuts( Csw_Man_t * p, Aig_Obj_t * pObj, int fTriv );
extern Aig_Obj_t *    Csw_ObjSweep( Csw_Man_t * p, Aig_Obj_t * pObj, int fTriv );
/*=== cnfMan.c ========================================================*/
extern Csw_Man_t *    Csw_ManStart( Aig_Man_t * pMan, int nCutsMax, int nLeafMax, int fVerbose );
extern void           Csw_ManStop( Csw_Man_t * p );
/*=== cnfTable.c ========================================================*/
extern int            Csw_TableCountCuts( Csw_Man_t * p );
extern void           Csw_TableCutInsert( Csw_Man_t * p, Csw_Cut_t * pCut );
extern Aig_Obj_t *    Csw_TableCutLookup( Csw_Man_t * p, Csw_Cut_t * pCut );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

