/**CFile****************************************************************

  FileName    [cut.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [K-feasible cut computation package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: .h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef ABC__opt__cut__cut_h
#define ABC__opt__cut__cut_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


#define CUT_SIZE_MIN    3      // the min K of the K-feasible cut computation
#define CUT_SIZE_MAX   12      // the max K of the K-feasible cut computation

#define CUT_SHIFT       8      // the number of bits for storing latch number in the cut leaves
#define CUT_MASK        0xFF   // the mask to get the stored latch number

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Cut_ManStruct_t_         Cut_Man_t;
typedef struct Cut_OracleStruct_t_      Cut_Oracle_t;
typedef struct Cut_CutStruct_t_         Cut_Cut_t;
typedef struct Cut_ParamsStruct_t_      Cut_Params_t;

struct Cut_ParamsStruct_t_
{
    int                nVarsMax;          // the max cut size ("k" of the k-feasible cuts)
    int                nKeepMax;          // the max number of cuts kept at a node
    int                nIdsMax;           // the max number of IDs of cut objects
    int                nBitShift;         // the number of bits used for the latch counter of an edge
    int                nCutSet;           // the number of nodes in the cut set
    int                fTruth;            // compute truth tables
    int                fFilter;           // filter dominated cuts
    int                fSeq;              // compute sequential cuts
    int                fDrop;             // drop cuts on the fly
    int                fDag;              // compute only DAG cuts
    int                fTree;             // compute only tree cuts
    int                fGlobal;           // compute only global cuts
    int                fLocal;            // compute only local cuts
    int                fRecord;           // record the cut computation flow
    int                fRecordAig;        // record the cut functions
    int                fFancy;            // perform fancy computations
    int                fMap;              // computes delay of FPGA mapping with cuts
    int                fAdjust;           // removed useless fanouts of XORs/MUXes
    int                fNpnSave;          // enables dumping 6-input truth tables
    int                fVerbose;          // the verbosiness flag
};

struct Cut_CutStruct_t_
{
    unsigned           Num0       : 11;   // temporary number
    unsigned           Num1       : 11;   // temporary number
    unsigned           fSimul     :  1;   // the value of cut's output at 000.. pattern
    unsigned           fCompl     :  1;   // the cut is complemented
    unsigned           nVarsMax   :  4;   // the max number of vars [4-6]
    unsigned           nLeaves    :  4;   // the number of leaves [4-6]
    unsigned           uSign;             // the signature
    unsigned           uCanon0;           // the canonical form 
    unsigned           uCanon1;           // the canonical form 
    Cut_Cut_t *        pNext;             // the next cut in the list
    int                pLeaves[0];        // the array of leaves
};

static inline int        Cut_CutReadLeaveNum( Cut_Cut_t * p )  {  return p->nLeaves;   }
static inline int *      Cut_CutReadLeaves( Cut_Cut_t * p )    {  return p->pLeaves;   }
static inline unsigned * Cut_CutReadTruth( Cut_Cut_t * p )     {  return (unsigned *)(p->pLeaves + p->nVarsMax); }
static inline void       Cut_CutWriteTruth( Cut_Cut_t * p, unsigned * puTruth )  { 
    int i;
    for ( i = (p->nVarsMax <= 5) ? 0 : ((1 << (p->nVarsMax - 5)) - 1); i >= 0; i-- )
        p->pLeaves[p->nVarsMax + i] = (int)puTruth[i];
}

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== cutApi.c ==========================================================*/
extern Cut_Cut_t *      Cut_NodeReadCutsNew( Cut_Man_t * p, int Node );
extern Cut_Cut_t *      Cut_NodeReadCutsOld( Cut_Man_t * p, int Node );
extern Cut_Cut_t *      Cut_NodeReadCutsTemp( Cut_Man_t * p, int Node );
extern void             Cut_NodeWriteCutsNew( Cut_Man_t * p, int Node, Cut_Cut_t * pList );
extern void             Cut_NodeWriteCutsOld( Cut_Man_t * p, int Node, Cut_Cut_t * pList );
extern void             Cut_NodeWriteCutsTemp( Cut_Man_t * p, int Node, Cut_Cut_t * pList );
extern void             Cut_NodeSetTriv( Cut_Man_t * p, int Node );
extern void             Cut_NodeTryDroppingCuts( Cut_Man_t * p, int Node );
extern void             Cut_NodeFreeCuts( Cut_Man_t * p, int Node );
/*=== cutCut.c ==========================================================*/
extern void             Cut_CutPrint( Cut_Cut_t * pCut, int fSeq );
extern void             Cut_CutPrintList( Cut_Cut_t * pList, int fSeq );
extern int              Cut_CutCountList( Cut_Cut_t * pList );
/*=== cutMan.c ==========================================================*/
extern Cut_Man_t *      Cut_ManStart( Cut_Params_t * pParams );
extern void             Cut_ManStop( Cut_Man_t * p );
extern void             Cut_ManPrintStats( Cut_Man_t * p );
extern void             Cut_ManPrintStatsToFile( Cut_Man_t * p, char * pFileName, abctime TimeTotal );
extern void             Cut_ManSetFanoutCounts( Cut_Man_t * p, Vec_Int_t * vFanCounts );
extern void             Cut_ManSetNodeAttrs( Cut_Man_t * p, Vec_Int_t * vFanCounts );
extern int              Cut_ManReadVarsMax( Cut_Man_t * p );
extern Cut_Params_t *   Cut_ManReadParams( Cut_Man_t * p );
extern Vec_Int_t *      Cut_ManReadNodeAttrs( Cut_Man_t * p );
extern void             Cut_ManIncrementDagNodes( Cut_Man_t * p );
/*=== cutNode.c ==========================================================*/
extern Cut_Cut_t *      Cut_NodeComputeCuts( Cut_Man_t * p, int Node, int Node0, int Node1, int fCompl0, int fCompl1, int fTriv, int TreeCode ); 
extern Cut_Cut_t *      Cut_NodeUnionCuts( Cut_Man_t * p, Vec_Int_t * vNodes );
extern Cut_Cut_t *      Cut_NodeUnionCutsSeq( Cut_Man_t * p, Vec_Int_t * vNodes, int CutSetNum, int fFirst );
extern int              Cut_ManMappingArea_rec( Cut_Man_t * p, int Node );
/*=== cutSeq.c ==========================================================*/
extern void             Cut_NodeComputeCutsSeq( Cut_Man_t * p, int Node, int Node0, int Node1, int fCompl0, int fCompl1, int nLat0, int nLat1, int fTriv, int CutSetNum );
extern void             Cut_NodeNewMergeWithOld( Cut_Man_t * p, int Node );
extern int              Cut_NodeTempTransferToNew( Cut_Man_t * p, int Node, int CutSetNum );
extern void             Cut_NodeOldTransferToNew( Cut_Man_t * p, int Node );
/*=== cutOracle.c ==========================================================*/
extern Cut_Oracle_t *   Cut_OracleStart( Cut_Man_t * pMan );
extern void             Cut_OracleStop( Cut_Oracle_t * p );
extern void             Cut_OracleSetFanoutCounts( Cut_Oracle_t * p, Vec_Int_t * vFanCounts );
extern int              Cut_OracleReadDrop( Cut_Oracle_t * p );
extern void             Cut_OracleNodeSetTriv( Cut_Oracle_t * p, int Node );
extern Cut_Cut_t *      Cut_OracleComputeCuts( Cut_Oracle_t * p, int Node, int Node0, int Node1, int fCompl0, int fCompl1 );
extern void             Cut_OracleTryDroppingCuts( Cut_Oracle_t * p, int Node );
/*=== cutTruth.c ==========================================================*/
extern void             Cut_TruthNCanonicize( Cut_Cut_t * pCut );
/*=== cutPre22.c ==========================================================*/
extern void             Cut_CellPrecompute();
extern void             Cut_CellLoad();
extern int              Cut_CellIsRunning();
extern void             Cut_CellDumpToFile();
extern int              Cut_CellTruthLookup( unsigned * pTruth, int nVars );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

