/**CFile****************************************************************

  FileName    [darInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: darInt.h,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__aig__dar__darInt_h
#define ABC__aig__dar__darInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "aig/aig/aig.h"
#include "dar.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START
 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Dar_Man_t_            Dar_Man_t;
typedef struct Dar_Cut_t_            Dar_Cut_t;

// the AIG 4-cut
struct Dar_Cut_t_  // 6 words
{
    unsigned         uSign;          // cut signature
    unsigned         uTruth  : 16;   // the truth table of the cut function
    unsigned         Value   : 11;   // the value of the cut 
    unsigned         fBest   :  1;   // marks the best cut
    unsigned         fUsed   :  1;   // marks the cut currently in use
    unsigned         nLeaves :  3;   // the number of leaves
    int              pLeaves[4];     // the array of leaves
};

// the AIG manager
struct Dar_Man_t_
{
    // input data
    Dar_RwrPar_t *   pPars;          // rewriting parameters
    Aig_Man_t *      pAig;           // AIG manager 
    // various data members
    Aig_MmFixed_t *  pMemCuts;       // memory manager for cuts
    void *           pManCnf;        // CNF managers
    Vec_Ptr_t *      vCutNodes;      // the nodes with cuts allocated
    // current rewriting step
    Vec_Ptr_t *      vLeavesBest;    // the best set of leaves
    int              OutBest;        // the best output (in the library)
    int              OutNumBest;     // the best number of the output
    int              GainBest;       // the best gain
    int              LevelBest;      // the level of node with the best gain
    int              ClassBest;      // the equivalence class of the best replacement
    // function statistics
    int              nTotalSubgs;    // the total number of subgraphs tried
    int              ClassTimes[222];// the runtimes for each class
    int              ClassGains[222];// the gains for each class
    int              ClassSubgs[222];// the graphs for each class
    int              nCutMemUsed;    // memory used for cuts
    // rewriting statistics
    int              nNodesInit;     // the original number of nodes
    int              nNodesTried;    // the number of nodes attempted
    int              nCutsAll;       // all cut pairs
    int              nCutsTried;     // computed cuts
    int              nCutsUsed;      // used cuts
    int              nCutsBad;       // bad cuts due to absent fanin
    int              nCutsGood;      // good cuts
    int              nCutsSkipped;   // skipped bad cuts
    // timing statistics
    abctime          timeCuts;
    abctime          timeEval;
    abctime          timeOther;
    abctime          timeTotal;
    abctime          time1;
    abctime          time2;
};

static inline Dar_Cut_t *  Dar_ObjCuts( Aig_Obj_t * pObj )                         { return (Dar_Cut_t *)pObj->pData;    }
static inline void         Dar_ObjSetCuts( Aig_Obj_t * pObj, Dar_Cut_t * pCuts )   { assert( !Aig_ObjIsNone(pObj) ); pObj->pData = pCuts;   }

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                          ITERATORS                               ///
////////////////////////////////////////////////////////////////////////

// iterator over all cuts of the node
#define Dar_ObjForEachCutAll( pObj, pCut, i )                                   \
    for ( (pCut) = Dar_ObjCuts(pObj), i = 0; i < (int)(pObj)->nCuts; i++, pCut++ ) 
#define Dar_ObjForEachCut( pObj, pCut, i )                                      \
    for ( (pCut) = Dar_ObjCuts(pObj), i = 0; i < (int)(pObj)->nCuts; i++, pCut++ ) if ( (pCut)->fUsed==0 ) {} else
// iterator over leaves of the cut
#define Dar_CutForEachLeaf( p, pCut, pLeaf, i )                                 \
    for ( i = 0; (i < (int)(pCut)->nLeaves) && (((pLeaf) = Aig_ManObj(p, (pCut)->pLeaves[i])), 1); i++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== darBalance.c ========================================================*/
/*=== darCore.c ===========================================================*/
/*=== darCut.c ============================================================*/
extern void            Dar_ManCutsRestart( Dar_Man_t * p, Aig_Obj_t * pRoot );
extern void            Dar_ManCutsFree( Dar_Man_t * p );
extern Dar_Cut_t *     Dar_ObjPrepareCuts( Dar_Man_t * p, Aig_Obj_t * pObj );
extern Dar_Cut_t *     Dar_ObjComputeCuts_rec( Dar_Man_t * p, Aig_Obj_t * pObj );
extern Dar_Cut_t *     Dar_ObjComputeCuts( Dar_Man_t * p, Aig_Obj_t * pObj, int fSkipTtMin );
extern void            Dar_ObjCutPrint( Aig_Man_t * p, Aig_Obj_t * pObj );
/*=== darData.c ===========================================================*/
extern Vec_Int_t *     Dar_LibReadNodes();
extern Vec_Int_t *     Dar_LibReadOuts();
extern Vec_Int_t *     Dar_LibReadPrios();
/*=== darLib.c ============================================================*/
extern void            Dar_LibStart();
extern void            Dar_LibStop();
extern void            Dar_LibReturnCanonicals( unsigned * pCanons );
extern void            Dar_LibEval( Dar_Man_t * p, Aig_Obj_t * pRoot, Dar_Cut_t * pCut, int Required, int * pnMffcSize );
extern Aig_Obj_t *     Dar_LibBuildBest( Dar_Man_t * p );
/*=== darMan.c ============================================================*/
extern Dar_Man_t *     Dar_ManStart( Aig_Man_t * pAig, Dar_RwrPar_t * pPars );
extern void            Dar_ManStop( Dar_Man_t * p );
extern void            Dar_ManPrintStats( Dar_Man_t * p );
/*=== darPrec.c ============================================================*/
extern char **         Dar_Permutations( int n );
extern void            Dar_Truth4VarNPN( unsigned short ** puCanons, char ** puPhases, char ** puPerms, unsigned char ** puMap );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

