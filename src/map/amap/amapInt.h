/**CFile****************************************************************

  FileName    [amapInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Technology mapper for standard cells.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: amapInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__map__amap__amapInt_h
#define ABC__map__amap__amapInt_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "misc/extra/extra.h"
#include "aig/aig/aig.h"
#include "amap.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


// the largest gate size in the library
// (gates above this size will be ignored)
#define AMAP_MAXINS 15

#define AMAP_STRING_CONST0     "CONST0"
#define AMAP_STRING_CONST1     "CONST1"

// object types
typedef enum { 
    AMAP_OBJ_NONE,    // 0: non-existent object
    AMAP_OBJ_CONST1,  // 1: constant 1 
    AMAP_OBJ_PI,      // 2: primary input
    AMAP_OBJ_PO,      // 3: primary output
    AMAP_OBJ_AND,     // 4: AND node
    AMAP_OBJ_XOR,     // 5: XOR node
    AMAP_OBJ_MUX,     // 6: MUX node
    AMAP_OBJ_VOID     // 7: unused object
} Amap_Type_t;

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Amap_Pin_t_ Amap_Pin_t;
typedef struct Amap_Gat_t_ Amap_Gat_t;
typedef struct Amap_Nod_t_ Amap_Nod_t;
typedef struct Amap_Set_t_ Amap_Set_t;

typedef struct Amap_Man_t_ Amap_Man_t;
typedef struct Amap_Obj_t_ Amap_Obj_t;
typedef struct Amap_Cut_t_ Amap_Cut_t;
typedef struct Amap_Mat_t_ Amap_Mat_t;

struct Amap_Man_t_
{
    // user data
    Amap_Par_t *       pPars;
    Amap_Lib_t *       pLib;
    // internal parameters
    float              fEpsilonInternal;
    float              fAreaInv;
    int                fUseXor;    
    int                fUseMux;    
    // internal AIG with choices
    Vec_Ptr_t *        vPis;
    Vec_Ptr_t *        vPos;
    Vec_Ptr_t *        vObjs;
    Aig_MmFixed_t *    pMemObj;
    Aig_MmFlex_t *     pMemCuts;
    Aig_MmFlex_t *     pMemCutBest;
    Aig_MmFlex_t *     pMemTemp;
    Amap_Obj_t *       pConst1;
    int                nObjs[AMAP_OBJ_VOID];
    int                nLevelMax;
    int                nChoicesGiven;
    int                nChoicesAdded;
    // mapping data-structures
    Vec_Int_t *        vTemp;
    int *              pMatsTemp;
    Amap_Cut_t **      ppCutsTemp;
    Amap_Cut_t *       pCutsPi;
    Vec_Ptr_t *        vCuts0;
    Vec_Ptr_t *        vCuts1;
    Vec_Ptr_t *        vCuts2;
    // statistics
    int                nCutsUsed;
    int                nCutsTried;
    int                nCutsTried3;
    int                nBytesUsed;
};
struct Amap_Lib_t_
{
    char *             pName;       // library name
    Vec_Ptr_t *        vGates;      // represenation of gates
    Vec_Ptr_t *        vSorted;     // gates sorted for area-only mapping
    Vec_Ptr_t *        vSelect;     // gates selected for area-only mapping
    Amap_Gat_t *       pGate0;      // the constant zero gate
    Amap_Gat_t *       pGate1;      // the constant one gate
    Amap_Gat_t *       pGateBuf;    // the buffer
    Amap_Gat_t *       pGateInv;    // the inverter
    Aig_MmFlex_t *     pMemGates;   // memory manager for objects
    int                fHasXor;     // XOR/NXOR gates are present
    int                fHasMux;     // MUX/NMUX gates are present
    // structural representation
    int                fVerbose;    // enable detailed statistics
    Amap_Nod_t *       pNodes;      // representation nodes
    int                nNodes;      // the number of nodes used
    int                nNodesAlloc; // the number of nodes allocated
    Vec_Ptr_t *        vRules;      // the rule of AND gate
    Vec_Ptr_t *        vRulesX;     // the rule of XOR gate
    Vec_Int_t *        vRules3;     // the rule of MUX gate
    int **             pRules;      // simplified representation
    int **             pRulesX;     // simplified representation
    Aig_MmFlex_t *     pMemSet;     // memory manager for sets
    int                nSets;       // the number of sets created
};
struct Amap_Pin_t_
{
    char *             pName;
    int                Phase;
    double             dLoadInput;
    double             dLoadMax;
    double             dDelayBlockRise;
    double             dDelayFanoutRise;
    double             dDelayBlockFall;
    double             dDelayFanoutFall;
    double             dDelayBlockMax;
};
struct Amap_Gat_t_
{
    Amap_Lib_t *       pLib;            // library   
    Amap_Gat_t *       pTwin;           // twin gate
    char *             pName;           // the name of the gate
    char *             pOutName;        // name of the output
    double             dArea;           // the area of the gate
    char *             pForm;           // the formula describing functionality
    unsigned *         pFunc;           // truth table
    unsigned           Id     :   23;   // unique number of the gate
    unsigned           fMux   :    1;   // denotes MUX-gates
    unsigned           nPins  :    8;   // number of inputs
    Amap_Pin_t         Pins[0];         // description of inputs
};
struct Amap_Set_t_
{
    Amap_Set_t *       pNext;
    unsigned           iGate    : 16;
    unsigned           fInv     :  1;
    unsigned           nIns     : 15;
    char               Ins[AMAP_MAXINS];// mapping from gate inputs into fanins
};
struct Amap_Nod_t_
{
    unsigned           Id       : 16;   // ID of the node
    unsigned           nSuppSize:  8;   // support size
    unsigned           Type     :  8;   // the type of node
    short              iFan0;           // fanin0
    short              iFan1;           // fanin1
    short              iFan2;           // fanin2
    short              Unused;          // 
    Amap_Set_t *       pSets;           // implementable gates
};
struct Amap_Cut_t_
{
    unsigned           iMat     : 16;
    unsigned           fInv     :  1;
    unsigned           nFans  : 15;
    int                Fans[0];
};
struct Amap_Mat_t_
{
    Amap_Cut_t *       pCut;           // the cut
    Amap_Set_t *       pSet;           // the set
    float              Area;           // area flow / exact area of the node
    float              AveFan;         // edge flow of the node
    float              Delay;          // delay of the node
};
struct Amap_Obj_t_
{
    unsigned           Type     :  3;   
    unsigned           Id       : 29;
    unsigned           IdPio    : 29;
    unsigned           fPhase   :  1;
    unsigned           fRepr    :  1;
    unsigned           fPolar   :  1;  // pCutBest->fInv ^ pSetBest->fInv
    unsigned           Level    : 12;  // 20  (July 16, 2009)
    unsigned           nCuts    : 20;  // 12  (July 16, 2009)
    int                nRefs;
    int                Equiv;
    int                Fan[3];
    union {
    void *             pData;
    int                iData;
    };
    // match of the node
    float              EstRefs;        // the number of estimated fanouts
    int                nFouts[2];      // the number of refs in each polarity
    Amap_Mat_t         Best;           // the best match of the node
};

static inline Amap_Obj_t * Amap_Regular( Amap_Obj_t * p )                          { return (Amap_Obj_t *)((ABC_PTRUINT_T)(p) & ~01);  }
static inline Amap_Obj_t * Amap_Not( Amap_Obj_t * p )                              { return (Amap_Obj_t *)((ABC_PTRUINT_T)(p) ^  01);  }
static inline Amap_Obj_t * Amap_NotCond( Amap_Obj_t * p, int c )                   { return (Amap_Obj_t *)((ABC_PTRUINT_T)(p) ^ (c));  }
static inline int          Amap_IsComplement( Amap_Obj_t * p )                     { return (int )(((ABC_PTRUINT_T)p) & 01);           }

static inline int          Amap_ManPiNum( Amap_Man_t * p )                         { return p->nObjs[AMAP_OBJ_PI];                      }
static inline int          Amap_ManPoNum( Amap_Man_t * p )                         { return p->nObjs[AMAP_OBJ_PO];                      }
static inline int          Amap_ManAndNum( Amap_Man_t * p )                        { return p->nObjs[AMAP_OBJ_AND];                     }
static inline int          Amap_ManXorNum( Amap_Man_t * p )                        { return p->nObjs[AMAP_OBJ_XOR];                     }
static inline int          Amap_ManMuxNum( Amap_Man_t * p )                        { return p->nObjs[AMAP_OBJ_MUX];                     }
static inline int          Amap_ManObjNum( Amap_Man_t * p )                        { return Vec_PtrSize(p->vObjs);                      }
static inline int          Amap_ManNodeNum( Amap_Man_t * p )                       { return p->nObjs[AMAP_OBJ_AND] + p->nObjs[AMAP_OBJ_XOR] + p->nObjs[AMAP_OBJ_MUX];   }

static inline Amap_Obj_t * Amap_ManConst1( Amap_Man_t * p )                        { return p->pConst1;                                 }
static inline Amap_Obj_t * Amap_ManPi( Amap_Man_t * p, int i )                     { return (Amap_Obj_t *)Vec_PtrEntry( p->vPis, i );   }
static inline Amap_Obj_t * Amap_ManPo( Amap_Man_t * p, int i )                     { return (Amap_Obj_t *)Vec_PtrEntry( p->vPos, i );   }
static inline Amap_Obj_t * Amap_ManObj( Amap_Man_t * p, int i )                    { return (Amap_Obj_t *)Vec_PtrEntry( p->vObjs, i );  }

static inline int          Amap_ObjIsConst1( Amap_Obj_t * pObj )                   { return pObj->Type == AMAP_OBJ_CONST1;              }
static inline int          Amap_ObjIsPi( Amap_Obj_t * pObj )                       { return pObj->Type == AMAP_OBJ_PI;                  }
static inline int          Amap_ObjIsPo( Amap_Obj_t * pObj )                       { return pObj->Type == AMAP_OBJ_PO;                  }
static inline int          Amap_ObjIsAnd( Amap_Obj_t * pObj )                      { return pObj->Type == AMAP_OBJ_AND;                 }
static inline int          Amap_ObjIsXor( Amap_Obj_t * pObj )                      { return pObj->Type == AMAP_OBJ_XOR;                 }
static inline int          Amap_ObjIsMux( Amap_Obj_t * pObj )                      { return pObj->Type == AMAP_OBJ_MUX;                 }
static inline int          Amap_ObjIsNode( Amap_Obj_t * pObj )                     { return pObj->Type == AMAP_OBJ_AND || pObj->Type == AMAP_OBJ_XOR || pObj->Type == AMAP_OBJ_MUX;  }

static inline int          Amap_ObjToLit( Amap_Obj_t * pObj )                      { return Abc_Var2Lit( Amap_Regular(pObj)->Id, Amap_IsComplement(pObj) ); }
static inline Amap_Obj_t * Amap_ObjFanin0( Amap_Man_t * p, Amap_Obj_t * pObj )     { return Amap_ManObj(p, Abc_Lit2Var(pObj->Fan[0])); }
static inline Amap_Obj_t * Amap_ObjFanin1( Amap_Man_t * p, Amap_Obj_t * pObj )     { return Amap_ManObj(p, Abc_Lit2Var(pObj->Fan[1])); }
static inline Amap_Obj_t * Amap_ObjFanin2( Amap_Man_t * p, Amap_Obj_t * pObj )     { return Amap_ManObj(p, Abc_Lit2Var(pObj->Fan[2])); }
static inline int          Amap_ObjFaninC0( Amap_Obj_t * pObj )                    { return Abc_LitIsCompl(pObj->Fan[0]);              }
static inline int          Amap_ObjFaninC1( Amap_Obj_t * pObj )                    { return Abc_LitIsCompl(pObj->Fan[1]);              }
static inline int          Amap_ObjFaninC2( Amap_Obj_t * pObj )                    { return Abc_LitIsCompl(pObj->Fan[2]);              }
static inline void *       Amap_ObjCopy( Amap_Obj_t * pObj )                       { return pObj->pData;                                }
static inline int          Amap_ObjLevel( Amap_Obj_t * pObj )                      { return pObj->Level;                                }
static inline void         Amap_ObjSetLevel( Amap_Obj_t * pObj, int Level )        { pObj->Level = Level;                               }
static inline void         Amap_ObjSetCopy( Amap_Obj_t * pObj, void * pCopy )      { pObj->pData = pCopy;                               }
static inline Amap_Obj_t * Amap_ObjChoice( Amap_Man_t * p, Amap_Obj_t * pObj )     { return pObj->Equiv? Amap_ManObj(p, pObj->Equiv) : NULL; }
static inline void         Amap_ObjSetChoice( Amap_Obj_t * pObj, Amap_Obj_t * pEqu){ assert(pObj->Equiv==0); if (pObj->Id != pEqu->Id) pObj->Equiv = pEqu->Id; }
static inline int          Amap_ObjPhaseReal( Amap_Obj_t * pObj )                  { return Amap_Regular(pObj)->fPhase ^ Amap_IsComplement(pObj);   }
static inline int          Amap_ObjRefsTotal( Amap_Obj_t * pObj )                  { return pObj->nFouts[0] + pObj->nFouts[1];          }

static inline Amap_Gat_t * Amap_LibGate( Amap_Lib_t * p, int i ) { return (Amap_Gat_t *)Vec_PtrEntry(p->vGates, i); }
static inline Amap_Nod_t * Amap_LibNod( Amap_Lib_t * p, int i )  { return p->pNodes + i;           }

// returns pointer to the next cut (internal cuts only)
static inline Amap_Cut_t * Amap_ManCutNext( Amap_Cut_t * pCut )
{ return (Amap_Cut_t *)(((int *)pCut)+pCut->nFans+1); }
// returns pointer to the place of the next cut (temporary cuts only)
static inline Amap_Cut_t ** Amap_ManCutNextP( Amap_Cut_t * pCut )
{ return (Amap_Cut_t **)(((int *)pCut)+pCut->nFans+1); }

extern void Kit_DsdPrintFromTruth( unsigned * pTruth, int nVars );

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// iterator over the primary inputs
#define Amap_ManForEachPi( p, pObj, i )                                    \
    Vec_PtrForEachEntry( Amap_Obj_t *, p->vPis, pObj, i )
// iterator over the primary outputs
#define Amap_ManForEachPo( p, pObj, i )                                    \
    Vec_PtrForEachEntry( Amap_Obj_t *, p->vPos, pObj, i )
// iterator over all objects, including those currently not used
#define Amap_ManForEachObj( p, pObj, i )                                   \
    Vec_PtrForEachEntry( Amap_Obj_t *, p->vObjs, pObj, i ) if ( (pObj) == NULL ) {} else
// iterator over all nodes
#define Amap_ManForEachNode( p, pObj, i )                                  \
    Vec_PtrForEachEntry( Amap_Obj_t *, p->vObjs, pObj, i ) if ( (pObj) == NULL || !Amap_ObjIsNode(pObj) ) {} else

// iterator through all gates of the library
#define Amap_LibForEachGate( pLib, pGate, i )                              \
    Vec_PtrForEachEntry( Amap_Gat_t *, pLib->vGates, pGate, i )
// iterator through all pins of the gate
#define Amap_GateForEachPin( pGate, pPin )                                 \
    for ( pPin = pGate->Pins; pPin < pGate->Pins + pGate->nPins; pPin++ )

// iterator through all cuts of the node
#define Amap_NodeForEachCut( pNode, pCut, i )                              \
    for ( i = 0, pCut = (Amap_Cut_t *)pNode->pData; i < (int)pNode->nCuts; \
        i++, pCut = Amap_ManCutNext(pCut) )

// iterator through all sets of one library node
#define Amap_LibNodeForEachSet( pNod, pSet )                               \
    for ( pSet = pNod->pSets; pSet; pSet = pSet->pNext )

// iterates through each fanin of the match
#define Amap_MatchForEachFaninCompl( p, pM, pFanin, fCompl, i )            \
    for ( i = 0; i < (int)(pM)->pCut->nFans &&                             \
        ((pFanin = Amap_ManObj((p), Abc_Lit2Var((pM)->pCut->Fans[Abc_Lit2Var((pM)->pSet->Ins[i])]))), 1) && \
        ((fCompl = Abc_LitIsCompl((pM)->pSet->Ins[i]) ^ Abc_LitIsCompl((pM)->pCut->Fans[Abc_Lit2Var((pM)->pSet->Ins[i])])), 1); \
        i++ )

// iterates through each fanin of the match
#define Amap_MatchForEachFanin( p, pM, pFanin, i )                         \
    for ( i = 0; i < (int)(pM)->pCut->nFans &&                             \
        ((pFanin = Amap_ManObj((p), Abc_Lit2Var((pM)->pCut->Fans[Abc_Lit2Var((pM)->pSet->Ins[i])]))), 1); \
        i++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== amapCore.c ==========================================================*/
/*=== amapGraph.c ==========================================================*/
extern Amap_Obj_t *  Amap_ManCreatePi( Amap_Man_t * p );
extern Amap_Obj_t *  Amap_ManCreatePo( Amap_Man_t * p, Amap_Obj_t * pFan0 );
extern Amap_Obj_t *  Amap_ManCreateAnd( Amap_Man_t * p, Amap_Obj_t * pFan0, Amap_Obj_t * pFan1 );
extern Amap_Obj_t *  Amap_ManCreateXor( Amap_Man_t * p, Amap_Obj_t * pFan0, Amap_Obj_t * pFan1 );
extern Amap_Obj_t *  Amap_ManCreateMux( Amap_Man_t * p, Amap_Obj_t * pFanC, Amap_Obj_t * pFan1, Amap_Obj_t * pFan0 );
extern void          Amap_ManCreateChoice( Amap_Man_t * p, Amap_Obj_t * pObj );
extern void          Amap_ManCreate( Amap_Man_t * p, Aig_Man_t * pAig );
/*=== amapLib.c ==========================================================*/
extern Amap_Lib_t *  Amap_LibAlloc();
extern int           Amap_LibNumPinsMax( Amap_Lib_t * p );
extern void          Amap_LibWrite( FILE * pFile, Amap_Lib_t * pLib, int fPrintDsd );
extern Vec_Ptr_t *   Amap_LibSelectGates( Amap_Lib_t * p, int fVerbose );
/*=== amapMan.c ==========================================================*/
extern Amap_Man_t *  Amap_ManStart( int nNodes );
extern void          Amap_ManStop( Amap_Man_t * p );
/*=== amapMatch.c ==========================================================*/
extern void          Amap_ManMap( Amap_Man_t * p );
/*=== amapMerge.c ==========================================================*/
extern void          Amap_ManMerge( Amap_Man_t * p );
/*=== amapOutput.c ==========================================================*/
extern Vec_Ptr_t *   Amap_ManProduceMapped( Amap_Man_t * p );
/*=== amapParse.c ==========================================================*/
extern int           Amap_LibParseEquations( Amap_Lib_t * p, int fVerbose );
/*=== amapPerm.c ==========================================================*/
/*=== amapRead.c ==========================================================*/
extern Amap_Lib_t *  Amap_LibReadBuffer( char * pBuffer, int fVerbose );
extern Amap_Lib_t *  Amap_LibReadFile( char * pFileName, int fVerbose );
/*=== amapRule.c ==========================================================*/
extern short *       Amap_LibTableFindNode( Amap_Lib_t * p, int iFan0, int iFan1, int fXor );
extern void          Amap_LibCreateRules( Amap_Lib_t * p, int fVeryVerbose );
/*=== amapUniq.c ==========================================================*/
extern int           Amap_LibFindNode( Amap_Lib_t * pLib, int iFan0, int iFan1, int fXor );
extern int           Amap_LibFindMux( Amap_Lib_t * p, int iFan0, int iFan1, int iFan2 );
extern int           Amap_LibCreateVar( Amap_Lib_t * p );
extern int           Amap_LibCreateNode( Amap_Lib_t * p, int iFan0, int iFan1, int fXor );
extern int           Amap_LibCreateMux( Amap_Lib_t * p, int iFan0, int iFan1, int iFan2 );
extern int **        Amap_LibLookupTableAlloc( Vec_Ptr_t * vVec, int fVerbose );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

