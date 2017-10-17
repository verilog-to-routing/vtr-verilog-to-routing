/**CFile****************************************************************

  FileName    [rwt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rwt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__rwt__rwt_h
#define ABC__aig__rwt__rwt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "misc/extra/extra.h"
#include "misc/mem/mem.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////
 
#define RWT_LIMIT  1048576/4  // ((1 << 20) 
#define RWT_MIN(a,b)      (((a) < (b))? (a) : (b))
#define RWT_MAX(a,b)      (((a) > (b))? (a) : (b))

typedef struct Rwt_Man_t_   Rwt_Man_t;
typedef struct Rwt_Node_t_  Rwt_Node_t;

struct Rwt_Man_t_
{
    // internal lookups
    int                nFuncs;           // number of four var functions
    unsigned short *   puCanons;         // canonical forms
    char *             pPhases;          // canonical phases
    char *             pPerms;           // canonical permutations
    unsigned char *    pMap;             // mapping of functions into class numbers
    unsigned short *   pMapInv;          // mapping of classes into functions
    char *             pPractical;       // practical NPN classes
    char **            pPerms4;          // four-var permutations
    // node space
    Vec_Ptr_t *        vForest;          // all the nodes
    Rwt_Node_t **      pTable;           // the hash table of nodes by their canonical form
    Vec_Vec_t *        vClasses;         // the nodes of the equivalence classes
    Mem_Fixed_t *      pMmNode;          // memory for nodes and cuts
    // statistical variables
    int                nTravIds;         // the counter of traversal IDs
    int                nConsidered;      // the number of nodes considered
    int                nAdded;           // the number of nodes added to lists
    int                nClasses;         // the number of NN classes
    // the result of resynthesis
    int                fCompl;           // indicates if the output of FF should be complemented
    void *             pCut;             // the decomposition tree (temporary)
    void *             pGraph;           // the decomposition tree (temporary)
    char *             pPerm;            // permutation used for the best cut
    Vec_Ptr_t *        vFanins;          // the fanins array (temporary)
    Vec_Ptr_t *        vFaninsCur;       // the fanins array (temporary)
    Vec_Int_t *        vLevNums;         // the array of levels (temporary)
    Vec_Ptr_t *        vNodesTemp;       // the nodes in MFFC (temporary)
    // node statistics
    int                nNodesConsidered;
    int                nNodesRewritten;
    int                nNodesGained;
    int                nScores[222];
    int                nCutsGood;
    int                nCutsBad;
    int                nSubgraphs;
    // runtime statistics
    abctime            timeStart;
    abctime            timeTruth;
    abctime            timeCut;
    abctime            timeRes;
    abctime            timeEval;
    abctime            timeMffc;
    abctime            timeUpdate;
    abctime            timeTotal;
};

struct Rwt_Node_t_ // 24 bytes
{
    int                Id;               // ID 
    int                TravId;           // traversal ID
    unsigned           uTruth : 16;      // truth table
    unsigned           Volume :  8;      // volume
    unsigned           Level  :  6;      // level
    unsigned           fUsed  :  1;      // mark
    unsigned           fExor  :  1;      // mark
    Rwt_Node_t *       p0;               // first child
    Rwt_Node_t *       p1;               // second child
    Rwt_Node_t *       pNext;            // next in the table
};

// manipulation of complemented attributes
static inline int          Rwt_IsComplement( Rwt_Node_t * p )    { return (int)(((ABC_PTRUINT_T)p) & 01);            }
static inline Rwt_Node_t * Rwt_Regular( Rwt_Node_t * p )         { return (Rwt_Node_t *)((ABC_PTRUINT_T)(p) & ~01);  }
static inline Rwt_Node_t * Rwt_Not( Rwt_Node_t * p )             { return (Rwt_Node_t *)((ABC_PTRUINT_T)(p) ^  01);  }
static inline Rwt_Node_t * Rwt_NotCond( Rwt_Node_t * p, int c )  { return (Rwt_Node_t *)((ABC_PTRUINT_T)(p) ^ (c));  }

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== rwrDec.c ========================================================*/
extern void              Rwt_ManPreprocess( Rwt_Man_t * p );
/*=== rwrMan.c ========================================================*/
extern Rwt_Man_t *       Rwt_ManStart( int fPrecompute );
extern void              Rwt_ManStop( Rwt_Man_t * p );
extern void              Rwt_ManPrintStats( Rwt_Man_t * p );
extern void              Rwt_ManPrintStatsFile( Rwt_Man_t * p );
extern void *            Rwt_ManReadDecs( Rwt_Man_t * p );
extern Vec_Ptr_t *       Rwt_ManReadLeaves( Rwt_Man_t * p );
extern int               Rwt_ManReadCompl( Rwt_Man_t * p );
extern void              Rwt_ManAddTimeCuts( Rwt_Man_t * p, abctime Time );
extern void              Rwt_ManAddTimeUpdate( Rwt_Man_t * p, abctime Time );
extern void              Rwt_ManAddTimeTotal( Rwt_Man_t * p, abctime Time );
/*=== rwrUtil.c ========================================================*/
extern void              Rwt_ManLoadFromArray( Rwt_Man_t * p, int fVerbose );
extern char *            Rwt_ManGetPractical( Rwt_Man_t * p );
extern Rwt_Node_t *      Rwt_ManAddVar( Rwt_Man_t * p, unsigned uTruth, int fPrecompute );
extern void              Rwt_ManIncTravId( Rwt_Man_t * p );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

