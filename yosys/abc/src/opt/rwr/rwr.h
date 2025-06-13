/**CFile****************************************************************

  FileName    [rwr.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rwr.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__opt__rwr__rwr_h
#define ABC__opt__rwr__rwr_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/abc/abc.h"
#include "opt/cut/cut.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////
 
#define RWR_LIMIT  1048576/4  // ((1 << 20) 

typedef struct Rwr_Man_t_   Rwr_Man_t;
typedef struct Rwr_Node_t_  Rwr_Node_t;

struct Rwr_Man_t_
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
    Rwr_Node_t **      pTable;           // the hash table of nodes by their canonical form
    Vec_Vec_t *        vClasses;         // the nodes of the equivalence classes
    Extra_MmFixed_t *  pMmNode;          // memory for nodes and cuts
    // statistical variables
    int                nTravIds;         // the counter of traversal IDs
    int                nConsidered;      // the number of nodes considered
    int                nAdded;           // the number of nodes added to lists
    int                nClasses;         // the number of NN classes
    // the result of resynthesis
    int                fCompl;           // indicates if the output of FF should be complemented
    void *             pGraph;           // the decomposition tree (temporary)
    Vec_Ptr_t *        vFanins;          // the fanins array (temporary)
    Vec_Ptr_t *        vFaninsCur;       // the fanins array (temporary)
    Vec_Int_t *        vLevNums;         // the array of levels (temporary)
    Vec_Ptr_t *        vNodesTemp;       // the nodes in MFFC (temporary)
    // node statistics
    int                nNodesConsidered;
    int                nNodesRewritten;
    int                nNodesGained;
    int                nNodesBeg;
    int                nNodesEnd;
    int                nScores[222];
    int                nCutsGood;
    int                nCutsBad;
    int                nSubgraphs;
    // runtime statistics
    abctime            timeStart;
    abctime            timeCut;
    abctime            timeRes;
    abctime            timeEval;
    abctime            timeMffc;
    abctime            timeUpdate;
    abctime            timeTotal;
};

struct Rwr_Node_t_ // 24 bytes
{
    int                Id;               // ID 
    int                TravId;           // traversal ID
    short              nScore;
    short              nGain;
    short              nAdded;
    unsigned           uTruth : 16;      // truth table
    unsigned           Volume :  8;      // volume
    unsigned           Level  :  6;      // level
    unsigned           fUsed  :  1;      // mark
    unsigned           fExor  :  1;      // mark
    Rwr_Node_t *       p0;               // first child
    Rwr_Node_t *       p1;               // second child
    Rwr_Node_t *       pNext;            // next in the table
};

// manipulation of complemented attributes
static inline int          Rwr_IsComplement( Rwr_Node_t * p )    { return (int )(((ABC_PTRUINT_T)p) & 01);           }
static inline Rwr_Node_t * Rwr_Regular( Rwr_Node_t * p )         { return (Rwr_Node_t *)((ABC_PTRUINT_T)(p) & ~01);  }
static inline Rwr_Node_t * Rwr_Not( Rwr_Node_t * p )             { return (Rwr_Node_t *)((ABC_PTRUINT_T)(p) ^  01);  }
static inline Rwr_Node_t * Rwr_NotCond( Rwr_Node_t * p, int c )  { return (Rwr_Node_t *)((ABC_PTRUINT_T)(p) ^ (c));  }

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== rwrDec.c ========================================================*/
extern void              Rwr_ManPreprocess( Rwr_Man_t * p );
/*=== rwrEva.c ========================================================*/
extern int               Rwr_NodeRewrite( Rwr_Man_t * p, Cut_Man_t * pManCut, Abc_Obj_t * pNode, int fUpdateLevel, int fUseZeros, int fPlaceEnable );
extern void              Rwr_ScoresClean( Rwr_Man_t * p );
extern void              Rwr_ScoresReport( Rwr_Man_t * p );
/*=== rwrLib.c ========================================================*/
extern void              Rwr_ManPrecompute( Rwr_Man_t * p );
extern Rwr_Node_t *      Rwr_ManAddVar( Rwr_Man_t * p, unsigned uTruth, int fPrecompute );
extern Rwr_Node_t *      Rwr_ManAddNode( Rwr_Man_t * p, Rwr_Node_t * p0, Rwr_Node_t * p1, int fExor, int Level, int Volume );
extern int               Rwr_ManNodeVolume( Rwr_Man_t * p, Rwr_Node_t * p0, Rwr_Node_t * p1 );
extern void              Rwr_ManIncTravId( Rwr_Man_t * p );
/*=== rwrMan.c ========================================================*/
extern Rwr_Man_t *       Rwr_ManStart( int  fPrecompute );
extern void              Rwr_ManStop( Rwr_Man_t * p );
extern void              Rwr_ManPrintStats( Rwr_Man_t * p );
extern void              Rwr_ManPrintStatsFile( Rwr_Man_t * p );
extern void *            Rwr_ManReadDecs( Rwr_Man_t * p );
extern Vec_Ptr_t *       Rwr_ManReadLeaves( Rwr_Man_t * p );
extern int               Rwr_ManReadCompl( Rwr_Man_t * p );
extern void              Rwr_ManAddTimeCuts( Rwr_Man_t * p, abctime Time );
extern void              Rwr_ManAddTimeUpdate( Rwr_Man_t * p, abctime Time );
extern void              Rwr_ManAddTimeTotal( Rwr_Man_t * p, abctime Time );
/*=== rwrPrint.c ========================================================*/
extern void              Rwr_ManPrint( Rwr_Man_t * p );
/*=== rwrUtil.c ========================================================*/
extern void              Rwr_ManWriteToArray( Rwr_Man_t * p );
extern void              Rwr_ManLoadFromArray( Rwr_Man_t * p, int fVerbose );
extern void              Rwr_ManWriteToFile( Rwr_Man_t * p, char * pFileName );
extern void              Rwr_ManLoadFromFile( Rwr_Man_t * p, char * pFileName );
extern void              Rwr_ListAddToTail( Rwr_Node_t ** ppList, Rwr_Node_t * pNode );
extern char *            Rwr_ManGetPractical( Rwr_Man_t * p );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

