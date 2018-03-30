/**CFile****************************************************************

  FileName    [cnf.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: cnf.h,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__cnf__cnf_h
#define ABC__aig__cnf__cnf_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "misc/vec/vec.h"
#include "aig/aig/aig.h"
#include "opt/dar/darInt.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START
 

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Cnf_Man_t_            Cnf_Man_t;
typedef struct Cnf_Dat_t_            Cnf_Dat_t;
typedef struct Cnf_Cut_t_            Cnf_Cut_t;

// the CNF asserting outputs of AIG to be 1
struct Cnf_Dat_t_
{
    Aig_Man_t *     pMan;            // the AIG manager, for which CNF is computed
    int             nVars;           // the number of variables
    int             nLiterals;       // the number of CNF literals
    int             nClauses;        // the number of CNF clauses
    int **          pClauses;        // the CNF clauses
    int *           pVarNums;        // the number of CNF variable for each node ID (-1 if unused)
    int *           pObj2Clause;     // the mapping of objects into clauses
    int *           pObj2Count;      // the mapping of objects into clause number
    unsigned char * pClaPols;        // polarity of input literals in each clause
    Vec_Int_t *     vMapping;        // mapping of internal nodes
};

// the cut used to represent node in the AIG
struct Cnf_Cut_t_
{
    char            nFanins;         // the number of leaves
    char            Cost;            // the cost of this cut
    short           nWords;          // the number of words in truth table
    Vec_Int_t *     vIsop[2];        // neg/pos ISOPs
    int             pFanins[0];      // the fanins (followed by the truth table)
};

// the CNF computation manager
struct Cnf_Man_t_
{
    Aig_Man_t *     pManAig;         // the underlying AIG manager
    char *          pSopSizes;       // sizes of SOPs for 4-variable functions
    char **         pSops;           // the SOPs for 4-variable functions
    int             aArea;           // the area of the mapping
    Aig_MmFlex_t *  pMemCuts;        // memory manager for cuts
    int             nMergeLimit;     // the limit on the size of merged cut
    unsigned *      pTruths[4];      // temporary truth tables
    Vec_Int_t *     vMemory;         // memory for intermediate ISOP representation
    abctime         timeCuts; 
    abctime         timeMap;
    abctime         timeSave;
};


static inline Dar_Cut_t *  Dar_ObjBestCut( Aig_Obj_t * pObj ) { Dar_Cut_t * pCut; int i; Dar_ObjForEachCut( pObj, pCut, i ) if ( pCut->fBest ) return pCut; return NULL; }

static inline int          Cnf_CutSopCost( Cnf_Man_t * p, Dar_Cut_t * pCut ) { return p->pSopSizes[pCut->uTruth] + p->pSopSizes[0xFFFF & ~pCut->uTruth]; }

static inline int          Cnf_CutLeaveNum( Cnf_Cut_t * pCut )    { return pCut->nFanins;                               }
static inline int *        Cnf_CutLeaves( Cnf_Cut_t * pCut )      { return pCut->pFanins;                               }
static inline unsigned *   Cnf_CutTruth( Cnf_Cut_t * pCut )       { return (unsigned *)(pCut->pFanins + pCut->nFanins); }

static inline Cnf_Cut_t *  Cnf_ObjBestCut( Aig_Obj_t * pObj )                       { return (Cnf_Cut_t *)pObj->pData;  }
static inline void         Cnf_ObjSetBestCut( Aig_Obj_t * pObj, Cnf_Cut_t * pCut )  { pObj->pData = pCut;  }

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                           ITERATORS                              ///
////////////////////////////////////////////////////////////////////////

// iterator over the clauses
#define Cnf_CnfForClause( p, pBeg, pEnd, i )                         \
    for ( i = 0; i < p->nClauses && (pBeg = p->pClauses[i]) && (pEnd = p->pClauses[i+1]); i++ )

// iterator over leaves of the cut
#define Cnf_CutForEachLeaf( p, pCut, pLeaf, i )                         \
    for ( i = 0; (i < (int)(pCut)->nFanins) && ((pLeaf) = Aig_ManObj(p, (pCut)->pFanins[i])); i++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== cnfCore.c ========================================================*/
extern Vec_Int_t *     Cnf_DeriveMappingArray( Aig_Man_t * pAig );
extern Cnf_Dat_t *     Cnf_Derive( Aig_Man_t * pAig, int nOutputs );
extern Cnf_Dat_t *     Cnf_DeriveWithMan( Cnf_Man_t * p, Aig_Man_t * pAig, int nOutputs );
extern Cnf_Dat_t *     Cnf_DeriveOther( Aig_Man_t * pAig, int fSkipTtMin );
extern Cnf_Dat_t *     Cnf_DeriveOtherWithMan( Cnf_Man_t * p, Aig_Man_t * pAig, int fSkipTtMin );
extern void            Cnf_ManPrepare();
extern Cnf_Man_t *     Cnf_ManRead();
extern void            Cnf_ManFree();
/*=== cnfCut.c ========================================================*/
extern Cnf_Cut_t *     Cnf_CutCreate( Cnf_Man_t * p, Aig_Obj_t * pObj );
extern void            Cnf_CutPrint( Cnf_Cut_t * pCut );
extern void            Cnf_CutFree( Cnf_Cut_t * pCut );
extern void            Cnf_CutUpdateRefs( Cnf_Man_t * p, Cnf_Cut_t * pCut, Cnf_Cut_t * pCutFan, Cnf_Cut_t * pCutRes );
extern Cnf_Cut_t *     Cnf_CutCompose( Cnf_Man_t * p, Cnf_Cut_t * pCut, Cnf_Cut_t * pCutFan, int iFan );
/*=== cnfData.c ========================================================*/
extern void            Cnf_ReadMsops( char ** ppSopSizes, char *** ppSops );
/*=== cnfFast.c ========================================================*/
extern void            Cnf_CollectLeaves( Aig_Obj_t * pRoot, Vec_Ptr_t * vSuper, int fStopCompl );
extern void            Cnf_ComputeClauses( Aig_Man_t * p, Aig_Obj_t * pRoot, Vec_Ptr_t * vLeaves, 
                           Vec_Ptr_t * vNodes, Vec_Int_t * vMap, Vec_Int_t * vCover, Vec_Int_t * vClauses );
extern void            Cnf_DeriveFastMark( Aig_Man_t * p );
extern Cnf_Dat_t *     Cnf_DeriveFast( Aig_Man_t * p, int nOutputs );
/*=== cnfMan.c ========================================================*/
extern Cnf_Man_t *     Cnf_ManStart();
extern void            Cnf_ManStop( Cnf_Man_t * p );
extern Vec_Int_t *     Cnf_DataCollectPiSatNums( Cnf_Dat_t * pCnf, Aig_Man_t * p );
extern Cnf_Dat_t *     Cnf_DataAlloc( Aig_Man_t * pAig, int nVars, int nClauses, int nLiterals );
extern Cnf_Dat_t *     Cnf_DataDup( Cnf_Dat_t * p );
extern void            Cnf_DataFree( Cnf_Dat_t * p );
extern void            Cnf_DataLift( Cnf_Dat_t * p, int nVarsPlus );
extern void            Cnf_DataCollectFlipLits( Cnf_Dat_t * p, int iFlipVar, Vec_Int_t * vFlips );
extern void            Cnf_DataLiftAndFlipLits( Cnf_Dat_t * p, int nVarsPlus, Vec_Int_t * vLits );
extern void            Cnf_DataPrint( Cnf_Dat_t * p, int fReadable );
extern void            Cnf_DataWriteIntoFile( Cnf_Dat_t * p, char * pFileName, int fReadable, Vec_Int_t * vForAlls, Vec_Int_t * vExists );
extern void *          Cnf_DataWriteIntoSolver( Cnf_Dat_t * p, int nFrames, int fInit );
extern void *          Cnf_DataWriteIntoSolverInt( void * pSat, Cnf_Dat_t * p, int nFrames, int fInit );
extern int             Cnf_DataWriteOrClause( void * pSat, Cnf_Dat_t * pCnf );
extern int             Cnf_DataWriteAndClauses( void * p, Cnf_Dat_t * pCnf );
extern void            Cnf_DataTranformPolarity( Cnf_Dat_t * pCnf, int fTransformPos );
extern int             Cnf_DataAddXorClause( void * pSat, int iVarA, int iVarB, int iVarC );
/*=== cnfMap.c ========================================================*/
extern void            Cnf_DeriveMapping( Cnf_Man_t * p );
extern int             Cnf_ManMapForCnf( Cnf_Man_t * p );
/*=== cnfPost.c ========================================================*/
extern void            Cnf_ManTransferCuts( Cnf_Man_t * p );
extern void            Cnf_ManFreeCuts( Cnf_Man_t * p );
extern void            Cnf_ManPostprocess( Cnf_Man_t * p );
/*=== cnfUtil.c ========================================================*/
extern Vec_Ptr_t *     Aig_ManScanMapping( Cnf_Man_t * p, int fCollect );
extern Vec_Ptr_t *     Cnf_ManScanMapping( Cnf_Man_t * p, int fCollect, int fPreorder );
extern Vec_Int_t *     Cnf_DataCollectCiSatNums( Cnf_Dat_t * pCnf, Aig_Man_t * p );
extern Vec_Int_t *     Cnf_DataCollectCoSatNums( Cnf_Dat_t * pCnf, Aig_Man_t * p );
extern unsigned char * Cnf_DataDeriveLitPolarities( Cnf_Dat_t * p );
extern Cnf_Dat_t *     Cnf_DataReadFromFile( char * pFileName );
/*=== cnfWrite.c ========================================================*/
extern Vec_Int_t *     Cnf_ManWriteCnfMapping( Cnf_Man_t * p, Vec_Ptr_t * vMapped );
extern void            Cnf_SopConvertToVector( char * pSop, int nCubes, Vec_Int_t * vCover );
extern Cnf_Dat_t *     Cnf_ManWriteCnf( Cnf_Man_t * p, Vec_Ptr_t * vMapped, int nOutputs );
extern Cnf_Dat_t *     Cnf_ManWriteCnfOther( Cnf_Man_t * p, Vec_Ptr_t * vMapped );
extern Cnf_Dat_t *     Cnf_DeriveSimple( Aig_Man_t * p, int nOutputs );
extern Cnf_Dat_t *     Cnf_DeriveSimpleForRetiming( Aig_Man_t * p );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

