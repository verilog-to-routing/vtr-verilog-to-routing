/**CFile****************************************************************

  FileName    [fpga.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Technology mapping for variable-size-LUT FPGAs.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 2.0. Started - August 18, 2004.]

  Revision    [$Id: fpga.h,v 1.7 2004/09/30 21:18:09 satrajit Exp $]

***********************************************************************/

#ifndef ABC__map__fpga__fpga_h
#define ABC__map__fpga__fpga_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


// the maximum size of LUTs used for mapping
#define FPGA_MAX_LUTSIZE   32

////////////////////////////////////////////////////////////////////////
///                    STRUCTURE DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

typedef struct Fpga_ManStruct_t_         Fpga_Man_t;
typedef struct Fpga_NodeStruct_t_        Fpga_Node_t;
typedef struct Fpga_NodeVecStruct_t_     Fpga_NodeVec_t;
typedef struct Fpga_CutStruct_t_         Fpga_Cut_t;
typedef struct Fpga_LutLibStruct_t_      Fpga_LutLib_t;

////////////////////////////////////////////////////////////////////////
///                       GLOBAL VARIABLES                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                       MACRO DEFINITIONS                          ///
////////////////////////////////////////////////////////////////////////
 
#define Fpga_IsComplement(p)    (((int)((ABC_PTRUINT_T) (p) & 01)))
#define Fpga_Regular(p)         ((Fpga_Node_t *)((ABC_PTRUINT_T)(p) & ~01)) 
#define Fpga_Not(p)             ((Fpga_Node_t *)((ABC_PTRUINT_T)(p) ^ 01)) 
#define Fpga_NotCond(p,c)       ((Fpga_Node_t *)((ABC_PTRUINT_T)(p) ^ (c)))

#define Fpga_Ref(p)   
#define Fpga_Deref(p)
#define Fpga_RecursiveDeref(p,c)

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== fpgaCreate.c =============================================================*/
extern Fpga_Man_t *    Fpga_ManCreate( int nInputs, int nOutputs, int fVerbose );
extern Fpga_Node_t *   Fpga_NodeCreate( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 );
extern void            Fpga_ManFree( Fpga_Man_t * pMan );
extern void            Fpga_ManPrintTimeStats( Fpga_Man_t * p );

extern int             Fpga_ManReadInputNum( Fpga_Man_t * p );
extern int             Fpga_ManReadOutputNum( Fpga_Man_t * p );
extern Fpga_Node_t **  Fpga_ManReadInputs ( Fpga_Man_t * p );
extern Fpga_Node_t **  Fpga_ManReadOutputs( Fpga_Man_t * p );
extern Fpga_Node_t *   Fpga_ManReadConst1 ( Fpga_Man_t * p );
extern float *         Fpga_ManReadInputArrivals( Fpga_Man_t * p );
extern int             Fpga_ManReadVerbose( Fpga_Man_t * p );
extern int             Fpga_ManReadVarMax( Fpga_Man_t * p );
extern float *         Fpga_ManReadLutAreas( Fpga_Man_t * p );
extern Fpga_NodeVec_t* Fpga_ManReadMapping( Fpga_Man_t * p );
extern void            Fpga_ManSetOutputNames( Fpga_Man_t * p, char ** ppNames );
extern void            Fpga_ManSetInputArrivals( Fpga_Man_t * p, float * pArrivals );
extern void            Fpga_ManSetAreaRecovery( Fpga_Man_t * p, int fAreaRecovery );
extern void            Fpga_ManSetDelayLimit( Fpga_Man_t * p, float DelayLimit );
extern void            Fpga_ManSetAreaLimit( Fpga_Man_t * p, float AreaLimit );
extern void            Fpga_ManSetObeyFanoutLimits( Fpga_Man_t * p, int fObeyFanoutLimits );              
extern void            Fpga_ManSetNumIterations( Fpga_Man_t * p, int nNumIterations );
extern int             Fpga_ManReadFanoutViolations( Fpga_Man_t * p );
extern void            Fpga_ManSetFanoutViolations( Fpga_Man_t * p, int nVio );
extern void            Fpga_ManSetChoiceNodeNum( Fpga_Man_t * p, int nChoiceNodes );
extern void            Fpga_ManSetChoiceNum( Fpga_Man_t * p, int nChoices );
extern void            Fpga_ManSetVerbose( Fpga_Man_t * p, int fVerbose );
extern void            Fpga_ManSetSwitching( Fpga_Man_t * p, int fSwitching );
extern void            Fpga_ManSetLatchPaths( Fpga_Man_t * p, int fLatchPaths );
extern void            Fpga_ManSetLatchNum( Fpga_Man_t * p, int nLatches );
extern void            Fpga_ManSetDelayTarget( Fpga_Man_t * p, float DelayTarget );
extern void            Fpga_ManSetName( Fpga_Man_t * p, char * pFileName );

extern int             Fpga_LibReadLutMax( Fpga_LutLib_t * pLib );

extern char *          Fpga_NodeReadData0( Fpga_Node_t * p );
extern Fpga_Node_t *   Fpga_NodeReadData1( Fpga_Node_t * p );
extern int             Fpga_NodeReadRefs( Fpga_Node_t * p );
extern int             Fpga_NodeReadNum( Fpga_Node_t * p );
extern int             Fpga_NodeReadLevel( Fpga_Node_t * p );
extern Fpga_Cut_t *    Fpga_NodeReadCuts( Fpga_Node_t * p );
extern Fpga_Cut_t *    Fpga_NodeReadCutBest( Fpga_Node_t * p );
extern Fpga_Node_t *   Fpga_NodeReadOne( Fpga_Node_t * p );
extern Fpga_Node_t *   Fpga_NodeReadTwo( Fpga_Node_t * p );
extern void            Fpga_NodeSetLevel( Fpga_Node_t * p, Fpga_Node_t * pNode );
extern void            Fpga_NodeSetData0( Fpga_Node_t * p, char * pData );
extern void            Fpga_NodeSetData1( Fpga_Node_t * p, Fpga_Node_t * pNode );
extern void            Fpga_NodeSetArrival( Fpga_Node_t * p, float Time );
extern void            Fpga_NodeSetNextE( Fpga_Node_t * p, Fpga_Node_t * pNextE );
extern void            Fpga_NodeSetRepr( Fpga_Node_t * p, Fpga_Node_t * pRepr );
extern void            Fpga_NodeSetSwitching( Fpga_Node_t * p, float Switching );

extern int             Fpga_NodeIsConst( Fpga_Node_t * p );
extern int             Fpga_NodeIsVar( Fpga_Node_t * p );
extern int             Fpga_NodeIsAnd( Fpga_Node_t * p );
extern int             Fpga_NodeComparePhase( Fpga_Node_t * p1, Fpga_Node_t * p2 );

extern int             Fpga_CutReadLeavesNum( Fpga_Cut_t * p );
extern Fpga_Node_t **  Fpga_CutReadLeaves( Fpga_Cut_t * p );

extern Fpga_Node_t *   Fpga_NodeAnd( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 );
extern Fpga_Node_t *   Fpga_NodeOr( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 );
extern Fpga_Node_t *   Fpga_NodeExor( Fpga_Man_t * p, Fpga_Node_t * p1, Fpga_Node_t * p2 );
extern Fpga_Node_t *   Fpga_NodeMux( Fpga_Man_t * p, Fpga_Node_t * pNode, Fpga_Node_t * pNodeT, Fpga_Node_t * pNodeE );
extern void            Fpga_NodeSetChoice( Fpga_Man_t * pMan, Fpga_Node_t * pNodeOld, Fpga_Node_t * pNodeNew );

extern void            Fpga_ManStats( Fpga_Man_t * p );

/*=== fpgaCore.c =============================================================*/
extern int             Fpga_Mapping( Fpga_Man_t * p );
/*=== fpgaCut.c ===============================================================*/
extern void            Fpga_MappingCreatePiCuts( Fpga_Man_t * p );
extern void            Fpga_CutsCleanSign( Fpga_Man_t * pMan );
extern void            Fpga_CutsCleanRoot( Fpga_Man_t * pMan );
/*=== fpgaCutUtils.c =============================================================*/
extern void            Fpga_CutCreateFromNode( Fpga_Man_t * p, int iRoot, int * pLeaves, int nLeaves );
extern void            Fpga_MappingSetUsedCuts( Fpga_Man_t * p );
/*=== fpgaLib.c =============================================================*/
extern Fpga_LutLib_t * Fpga_LutLibDup( Fpga_LutLib_t * p );
extern int             Fpga_LutLibReadVarMax( Fpga_LutLib_t * p );
extern float *         Fpga_LutLibReadLutAreas( Fpga_LutLib_t * p );
extern float *         Fpga_LutLibReadLutDelays( Fpga_LutLib_t * p );
extern float           Fpga_LutLibReadLutArea( Fpga_LutLib_t * p, int Size );
extern float           Fpga_LutLibReadLutDelay( Fpga_LutLib_t * p, int Size );
/*=== fpgaTruth.c =============================================================*/
extern void *          Fpga_TruthsCutBdd( void * dd, Fpga_Cut_t * pCut );
extern int             Fpga_CutVolume( Fpga_Cut_t * pCut );
/*=== fpgaUtil.c =============================================================*/
extern int             Fpga_ManCheckConsistency( Fpga_Man_t * p );
extern void            Fpga_ManCleanData0( Fpga_Man_t * pMan );
extern Fpga_NodeVec_t * Fpga_CollectNodeTfo( Fpga_Man_t * pMan, Fpga_Node_t * pNode );
/*=== fpga.c =============================================================*/
extern void            Fpga_SetSimpleLutLib( int nLutSize );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
