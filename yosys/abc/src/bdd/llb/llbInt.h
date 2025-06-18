/**CFile****************************************************************

  FileName    [llbInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [BDD-based reachability.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - May 8, 2010.]

  Revision    [$Id: llbInt.h,v 1.00 2010/05/08 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__llb__llbInt_h
#define ABC__aig__llb__llbInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "aig/aig/aig.h"
#include "aig/saig/saig.h"
#include "proof/ssw/ssw.h"
#include "llb.h"

#include "bdd/extrab/extraBdd.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Llb_Man_t_ Llb_Man_t;
typedef struct Llb_Mtr_t_ Llb_Mtr_t;
typedef struct Llb_Grp_t_ Llb_Grp_t;

struct Llb_Man_t_
{
    Gia_ParLlb_t *  pPars;          // parameters
    Aig_Man_t *     pAigGlo;        // initial AIG manager (owned by the caller)
    Aig_Man_t *     pAig;           // derived AIG manager (created in this package)
    DdManager *     dd;             // BDD manager
    DdManager *     ddG;            // BDD manager
    DdManager *     ddR;            // BDD manager
    Vec_Int_t *     vObj2Var;       // mapping AIG ObjId into BDD var index
    Vec_Int_t *     vVar2Obj;       // mapping BDD var index into AIG ObjId
    Vec_Ptr_t *     vGroups;        // group Id into group pointer
    Llb_Mtr_t *     pMatrix;        // dependency matrix
    // image computation
    Vec_Ptr_t *     vRings;         // onion rings
    Vec_Int_t *     vVarBegs;       // the first group where the var appears  
    Vec_Int_t *     vVarEnds;       // the last group where the var appears 
    // variable mapping
    Vec_Int_t *     vNs2Glo;        // next state variables into global variables
    Vec_Int_t *     vCs2Glo;        // next state variables into global variables
    Vec_Int_t *     vGlo2Cs;        // global variables into current state variables
    Vec_Int_t *     vGlo2Ns;        // global variables into current state variables
    // flow computation
//    Vec_Int_t *     vMem;
//    Vec_Ptr_t *     vTops;
//    Vec_Ptr_t *     vBots;
//    Vec_Ptr_t *     vCuts;
};

struct Llb_Mtr_t_
{
    int             nPis;           // number of primary inputs
    int             nFfs;           // number of flip-flops
    int             nRows;          // number of rows
    int             nCols;          // number of columns
    int *           pColSums;       // sum of values in a column
    Llb_Grp_t **    pColGrps;       // group structure for each col
    int *           pRowSums;       // sum of values in a row
    char **         pMatrix;        // dependency matrix
    Llb_Man_t *     pMan;           // manager
    // partial product
    char *          pProdVars;      // variables in the partial product
    int *           pProdNums;      // var counts in the remaining partitions
};

struct Llb_Grp_t_
{
    int             Id;             // group ID
    Vec_Ptr_t *     vIns;           // input AIG objs
    Vec_Ptr_t *     vOuts;          // output AIG objs
    Vec_Ptr_t *     vNodes;         // internal AIG objs
    Llb_Man_t *     pMan;           // manager
    Llb_Grp_t *     pPrev;          // previous group
    Llb_Grp_t *     pNext;          // next group
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline int Llb_ObjBddVar( Vec_Int_t * vOrder, Aig_Obj_t * pObj ) { return Vec_IntEntry(vOrder, Aig_ObjId(pObj)); }

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== llbConstr.c ======================================================*/
extern Vec_Int_t *     Llb_ManDeriveConstraints( Aig_Man_t * p );
extern void            Llb_ManPrintEntries( Aig_Man_t * p, Vec_Int_t * vCands );
/*=== llbCore.c ======================================================*/
extern int             Llb_ManModelCheckAig( Aig_Man_t * pAigGlo, Gia_ParLlb_t * pPars, Vec_Int_t * vHints, DdManager ** pddGlo );
/*=== llbCluster.c ======================================================*/
extern void            Llb_ManCluster( Llb_Mtr_t * p );
/*=== llbDump.c ======================================================*/
extern void            Llb_ManDumpReached( DdManager * ddG, DdNode * bReached, char * pModel, char * pFileName );
/*=== llbFlow.c ======================================================*/
extern Vec_Ptr_t *     Llb_ManFlow( Aig_Man_t * p, Vec_Ptr_t * vSources, int * pnFlow );
/*=== llbHint.c ======================================================*/
extern int             Llb_ManReachabilityWithHints( Llb_Man_t * p );
extern int             Llb_ManModelCheckAigWithHints( Aig_Man_t * pAigGlo, Gia_ParLlb_t * pPars );
/*=== llbMan.c =======================================================*/
extern void            Llb_ManPrepareVarMap( Llb_Man_t * p );
extern Llb_Man_t *     Llb_ManStart( Aig_Man_t * pAigGlo, Aig_Man_t * pAig, Gia_ParLlb_t *  pPars );
extern void            Llb_ManStop( Llb_Man_t * p );
/*=== llbMatrix.c ====================================================*/
extern void            Llb_MtrVerifyMatrix( Llb_Mtr_t * p );
extern Llb_Mtr_t *     Llb_MtrCreate( Llb_Man_t * p );
extern void            Llb_MtrFree( Llb_Mtr_t * p );
extern void            Llb_MtrPrint( Llb_Mtr_t * p, int fOrder );
extern void            Llb_MtrPrintMatrixStats( Llb_Mtr_t * p ); 
/*=== llbPart.c ======================================================*/
extern Llb_Grp_t *     Llb_ManGroupAlloc( Llb_Man_t * pMan );
extern void            Llb_ManGroupStop( Llb_Grp_t * p );
extern void            Llb_ManPrepareGroups( Llb_Man_t * pMan );
extern Llb_Grp_t *     Llb_ManGroupsCombine( Llb_Grp_t * p1, Llb_Grp_t * p2 );
extern Llb_Grp_t *     Llb_ManGroupCreateFromCuts( Llb_Man_t * pMan, Vec_Int_t * vCut1, Vec_Int_t * vCut2 );
extern void            Llb_ManPrepareVarLimits( Llb_Man_t * p );
/*=== llbPivot.c =====================================================*/
extern int             Llb_ManTracePaths( Aig_Man_t * p, Aig_Obj_t * pPivot );
extern Vec_Int_t *     Llb_ManMarkPivotNodes( Aig_Man_t * p, int fUseInternal );
/*=== llbReach.c =====================================================*/
extern int             Llb_ManReachability( Llb_Man_t * p, Vec_Int_t * vHints, DdManager ** pddGlo );
/*=== llbSched.c =====================================================*/
extern void            Llb_MtrSchedule( Llb_Mtr_t * p );

/*=== llb2Bad.c ======================================================*/
extern DdNode *        Llb_BddComputeBad( Aig_Man_t * pInit, DdManager * dd, abctime TimeOut );
extern DdNode *        Llb_BddQuantifyPis( Aig_Man_t * pInit, DdManager * dd, DdNode * bFunc );
/*=== llb2Core.c ======================================================*/
extern DdNode *        Llb_CoreComputeCube( DdManager * dd, Vec_Int_t * vVars, int fUseVarIndex, char * pValues );
extern int             Llb_CoreExperiment( Aig_Man_t * pInit, Aig_Man_t * pAig, Gia_ParLlb_t * pPars, Vec_Ptr_t * vResult, abctime TimeTarget ); 
/*=== llb2Driver.c ======================================================*/
extern Vec_Int_t *     Llb_DriverCountRefs( Aig_Man_t * p );
extern Vec_Int_t *     Llb_DriverCollectNs( Aig_Man_t * pAig, Vec_Int_t * vDriRefs );
extern Vec_Int_t *     Llb_DriverCollectCs( Aig_Man_t * pAig );
extern DdNode *        Llb_DriverPhaseCube( Aig_Man_t * pAig, Vec_Int_t * vDriRefs, DdManager * dd );
extern DdManager *     Llb_DriverLastPartition( Aig_Man_t * p, Vec_Int_t * vVarsNs, abctime TimeTarget );
/*=== llb2Image.c ======================================================*/
extern Vec_Ptr_t *     Llb_ImgSupports( Aig_Man_t * p, Vec_Ptr_t * vDdMans, Vec_Int_t * vStart, Vec_Int_t * vStop, int fAddPis, int fVerbose );
extern void            Llb_ImgSchedule( Vec_Ptr_t * vSupps, Vec_Ptr_t ** pvQuant0, Vec_Ptr_t ** pvQuant1, int fVerbose );
extern DdManager *     Llb_ImgPartition( Aig_Man_t * p, Vec_Ptr_t * vLower, Vec_Ptr_t * vUpper, abctime TimeTarget );
extern void            Llb_ImgQuantifyFirst( Aig_Man_t * pAig, Vec_Ptr_t * vDdMans, Vec_Ptr_t * vQuant0, int fVerbose );
extern void            Llb_ImgQuantifyReset( Vec_Ptr_t * vDdMans );
extern DdNode *        Llb_ImgComputeImage( Aig_Man_t * pAig, Vec_Ptr_t * vDdMans, DdManager * dd, DdNode * bInit, 
                           Vec_Ptr_t * vQuant0, Vec_Ptr_t * vQuant1, Vec_Int_t * vDriRefs, 
                           abctime TimeTarget, int fBackward, int fReorder, int fVerbose );

extern DdManager *     Llb_NonlinImageStart( Aig_Man_t * pAig, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vRoots, int * pVars2Q, int * pOrder, int fFirst, abctime TimeTarget );
extern DdNode *        Llb_NonlinImageCompute( DdNode * bCurrent, int fReorder, int fDrop, int fVerbose, int * pOrder );
extern void            Llb_NonlinImageQuit();

/*=== llb3Image.c =======================================================*/
extern DdNode *        Llb_NonlinImage( Aig_Man_t * pAig, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vRoots, int * pVars2Q, 
                           DdManager * dd, DdNode * bCurrent, int fReorder, int fVerbose, int * pOrder );
/*=== llb3Nonlin.c ======================================================*/
extern DdNode *        Llb_NonlinComputeInitState( Aig_Man_t * pAig, DdManager * dd );


/*=== llb4Cex.c =======================================================*/
extern Abc_Cex_t *     Llb4_Nonlin4TransformCex( Aig_Man_t * pAig, Vec_Ptr_t * vStates, int iCexPo, int fVerbose );
/*=== llb4Cluster.c =======================================================*/
//extern void            Llb_Nonlin4Cluster( Aig_Man_t * pAig, DdManager ** pdd, Vec_Int_t ** pvOrder, Vec_Ptr_t ** pvGroups, int nBddMax, int fVerbose );
/*=== llb4Image.c =======================================================*/
extern DdNode *        Llb_Nonlin4Image( DdManager * dd, Vec_Ptr_t * vParts, DdNode * bCurrent, Vec_Int_t * vVars2Q );
extern Vec_Ptr_t *     Llb_Nonlin4Group( DdManager * dd, Vec_Ptr_t * vParts, Vec_Int_t * vVars2Q, int nSizeMax );
/*=== llb4Map.c =========================================================*/
//extern Vec_Int_t *     Llb_AigMap( Aig_Man_t * pAig, int nLutSize, int nLutMin );
/*=== llb4Nonlin.c ======================================================*/
//extern int             Llb_Nonlin4CoreReach( Aig_Man_t * pAig, Gia_ParLlb_t * pPars );
/*=== llb4Sweep.c ======================================================*/
extern void            Llb4_Nonlin4Sweep( Aig_Man_t * pAig, int nSweepMax, int nClusterMax, DdManager ** pdd, Vec_Int_t ** pvOrder, Vec_Ptr_t ** pvGroups, int fVerbose );
 

ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

