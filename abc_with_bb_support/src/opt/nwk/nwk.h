/**CFile****************************************************************

  FileName    [nwk.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Logic network representation.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: nwk.h,v 1.1 2008/05/14 22:13:09 wudenni Exp $]

***********************************************************************/
 
#ifndef __NWK_abc_opt_nwk_h
#define __NWK_abc_opt_nwk_h

 
////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "aig/aig/aig.h"
#include "aig/hop/hop.h"
#include "misc/tim/tim.h"
#include "map/if/if.h"
#include "bool/bdc/bdc.h"

#include "proof/fra/fra.h"
#include "proof/ssw/ssw.h"
#include "ntlnwk.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Nwk_Obj_t_    Nwk_Obj_t;

// object types
typedef enum { 
    NWK_OBJ_NONE,                      // 0: non-existant object
    NWK_OBJ_CI,                        // 1: combinational input
    NWK_OBJ_CO,                        // 2: combinational output
    NWK_OBJ_NODE,                      // 3: logic node
    NWK_OBJ_LATCH,                     // 4: register
    NWK_OBJ_VOID                       // 5: unused object
} Nwk_Type_t;

struct Nwk_Man_t_
{
    // models of this design
    char *             pName;          // the name of this design
    char *             pSpec;          // the name of input file
    // node representation
    Vec_Ptr_t *        vCis;           // the primary inputs of the extracted part
    Vec_Ptr_t *        vCos;           // the primary outputs of the extracted part 
    Vec_Ptr_t *        vObjs;          // the objects in the topological order
    int                nObjs[NWK_OBJ_VOID]; // counter of objects of each type
    int                nFanioPlus;     // the number of extra fanins/fanouts alloc by default
    // functionality, timing, memory, etc
    Hop_Man_t *        pManHop;        // the functionality representation
    Tim_Man_t *        pManTime;       // the timing manager
    If_LibLut_t *         pLutLib;        // the LUT library
    Aig_MmFlex_t *     pMemObjs;       // memory for objects
    Vec_Ptr_t *        vTemp;          // array used for incremental updates
    int                nTravIds;       // the counter of traversal IDs
    int                nRealloced;     // the number of realloced nodes
    // sequential information
    int                nLatches;       // the total number of latches 
    int                nTruePis;       // the number of true primary inputs
    int                nTruePos;       // the number of true primary outputs
};

struct Nwk_Obj_t_
{
    Nwk_Man_t *        pMan;           // the manager  
    Hop_Obj_t *        pFunc;          // functionality
    void *             pCopy;          // temporary pointer
    union {
        void *         pNext;          // temporary pointer
        int            iTemp;          // temporary number
    };
    // node information
    unsigned           Type     :  3;  // object type
    unsigned           fInvert  :  1;  // complemented attribute
    unsigned           MarkA    :  1;  // temporary mark  
    unsigned           MarkB    :  1;  // temporary mark
    unsigned           MarkC    :  1;  // temporary mark
    unsigned           PioId    : 25;  // number of this node in the PI/PO list
    int                Id;             // unique ID
    int                TravId;         // traversal ID
    // timing information
    int                Level;          // the topological level
    float              tArrival;       // the arrival time
    float              tRequired;      // the required time
    float              tSlack;         // the slack
    // fanin/fanout representation
    int                nFanins;        // the number of fanins
    int                nFanouts;       // the number of fanouts
    int                nFanioAlloc;    // the number of allocated fanins/fanouts
    Nwk_Obj_t **       pFanio;         // fanins/fanouts
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                      INLINED FUNCTIONS                           ///
////////////////////////////////////////////////////////////////////////

static inline int         Nwk_ManCiNum( Nwk_Man_t * p )           { return p->nObjs[NWK_OBJ_CI];                } 
static inline int         Nwk_ManCoNum( Nwk_Man_t * p )           { return p->nObjs[NWK_OBJ_CO];                } 
static inline int         Nwk_ManNodeNum( Nwk_Man_t * p )         { return p->nObjs[NWK_OBJ_NODE];              } 
static inline int         Nwk_ManLatchNum( Nwk_Man_t * p )        { return p->nObjs[NWK_OBJ_LATCH];             } 
static inline int         Nwk_ManObjNumMax( Nwk_Man_t * p )       { return Vec_PtrSize(p->vObjs);               }

static inline Nwk_Obj_t * Nwk_ManCi( Nwk_Man_t * p, int i )       { return (Nwk_Obj_t *)Vec_PtrEntry( p->vCis, i );          } 
static inline Nwk_Obj_t * Nwk_ManCo( Nwk_Man_t * p, int i )       { return (Nwk_Obj_t *)Vec_PtrEntry( p->vCos, i );          } 
static inline Nwk_Obj_t * Nwk_ManObj( Nwk_Man_t * p, int i )      { return (Nwk_Obj_t *)Vec_PtrEntry( p->vObjs, i );         } 

static inline int         Nwk_ObjId( Nwk_Obj_t * p )              { return p->Id;                               } 
static inline int         Nwk_ObjPioNum( Nwk_Obj_t * p )          { return p->PioId;                            } 
static inline int         Nwk_ObjFaninNum( Nwk_Obj_t * p )        { return p->nFanins;                          } 
static inline int         Nwk_ObjFanoutNum( Nwk_Obj_t * p )       { return p->nFanouts;                         } 

static inline Nwk_Obj_t * Nwk_ObjFanin0( Nwk_Obj_t * p )          { return p->pFanio[0];                        } 
static inline Nwk_Obj_t * Nwk_ObjFanout0( Nwk_Obj_t * p )         { return p->pFanio[p->nFanins];               } 
static inline Nwk_Obj_t * Nwk_ObjFanin( Nwk_Obj_t * p, int i )    { return p->pFanio[i];                        } 
static inline Nwk_Obj_t * Nwk_ObjFanout( Nwk_Obj_t * p, int i )   { return p->pFanio[p->nFanins+1];             } 

static inline int         Nwk_ObjIsNone( Nwk_Obj_t * p )          { return p->Type == NWK_OBJ_NONE;             } 
static inline int         Nwk_ObjIsCi( Nwk_Obj_t * p )            { return p->Type == NWK_OBJ_CI;               } 
static inline int         Nwk_ObjIsCo( Nwk_Obj_t * p )            { return p->Type == NWK_OBJ_CO;               } 
static inline int         Nwk_ObjIsNode( Nwk_Obj_t * p )          { return p->Type == NWK_OBJ_NODE;             } 
static inline int         Nwk_ObjIsLatch( Nwk_Obj_t * p )         { return p->Type == NWK_OBJ_LATCH;            } 
static inline int         Nwk_ObjIsPi( Nwk_Obj_t * p )            { return Nwk_ObjIsCi(p) && (p->pMan->pManTime == NULL || Tim_ManBoxForCi(p->pMan->pManTime, p->PioId) == -1); } 
static inline int         Nwk_ObjIsPo( Nwk_Obj_t * p )            { return Nwk_ObjIsCo(p) && (p->pMan->pManTime == NULL || Tim_ManBoxForCo(p->pMan->pManTime, p->PioId) == -1);  }
static inline int         Nwk_ObjIsLi( Nwk_Obj_t * p )            { return p->pMan->nTruePos && Nwk_ObjIsCo(p) && (int)p->PioId >= p->pMan->nTruePos;       } 
static inline int         Nwk_ObjIsLo( Nwk_Obj_t * p )            { return p->pMan->nTruePis && Nwk_ObjIsCi(p) && (int)p->PioId >= p->pMan->nTruePis;       } 

static inline float       Nwk_ObjArrival( Nwk_Obj_t * pObj )                 { return pObj->tArrival;           }
static inline float       Nwk_ObjRequired( Nwk_Obj_t * pObj )                { return pObj->tRequired;          }
static inline float       Nwk_ObjSlack( Nwk_Obj_t * pObj )                   { return pObj->tSlack;             }
static inline void        Nwk_ObjSetArrival( Nwk_Obj_t * pObj, float Time )  { pObj->tArrival   = Time;         }
static inline void        Nwk_ObjSetRequired( Nwk_Obj_t * pObj, float Time ) { pObj->tRequired  = Time;         }
static inline void        Nwk_ObjSetSlack( Nwk_Obj_t * pObj, float Time )    { pObj->tSlack     = Time;         }

static inline int         Nwk_ObjLevel( Nwk_Obj_t * pObj )                   { return pObj->Level;              }
static inline void        Nwk_ObjSetLevel( Nwk_Obj_t * pObj, int Level )     { pObj->Level = Level;             }

static inline void        Nwk_ObjSetTravId( Nwk_Obj_t * pObj, int TravId )   { pObj->TravId = TravId;                           }
static inline void        Nwk_ObjSetTravIdCurrent( Nwk_Obj_t * pObj )        { pObj->TravId = pObj->pMan->nTravIds;             }
static inline void        Nwk_ObjSetTravIdPrevious( Nwk_Obj_t * pObj )       { pObj->TravId = pObj->pMan->nTravIds - 1;         }
static inline int         Nwk_ObjIsTravIdCurrent( Nwk_Obj_t * pObj )         { return pObj->TravId == pObj->pMan->nTravIds;     }
static inline int         Nwk_ObjIsTravIdPrevious( Nwk_Obj_t * pObj )        { return pObj->TravId == pObj->pMan->nTravIds - 1; }

static inline int         Nwk_ManTimeEqual( float f1, float f2, float Eps )  { return (f1 < f2 + Eps) && (f2 < f1 + Eps);  }
static inline int         Nwk_ManTimeLess( float f1, float f2, float Eps )   { return (f1 < f2 + Eps);                     }
static inline int         Nwk_ManTimeMore( float f1, float f2, float Eps )   { return (f1 + Eps > f2);                     }

////////////////////////////////////////////////////////////////////////
///                         ITERATORS                                ///
////////////////////////////////////////////////////////////////////////

#define Nwk_ManForEachCi( p, pObj, i )                                     \
    Vec_PtrForEachEntry( Nwk_Obj_t *, p->vCis, pObj, i )
#define Nwk_ManForEachCo( p, pObj, i )                                     \
    Vec_PtrForEachEntry( Nwk_Obj_t *, p->vCos, pObj, i )
#define Nwk_ManForEachPi( p, pObj, i )                                     \
    Vec_PtrForEachEntry( Nwk_Obj_t *, p->vCis, pObj, i )                                \
        if ( !Nwk_ObjIsPi(pObj) ) {} else
#define Nwk_ManForEachPo( p, pObj, i )                                     \
    Vec_PtrForEachEntry( Nwk_Obj_t *, p->vCos, pObj, i )                                \
        if ( !Nwk_ObjIsPo(pObj) ) {} else
#define Nwk_ManForEachObj( p, pObj, i )                                    \
    for ( i = 0; (i < Vec_PtrSize(p->vObjs)) && (((pObj) = (Nwk_Obj_t *)Vec_PtrEntry(p->vObjs, i)), 1); i++ ) \
        if ( pObj == NULL ) {} else
#define Nwk_ManForEachNode( p, pObj, i )                                   \
    for ( i = 0; (i < Vec_PtrSize(p->vObjs)) && (((pObj) = (Nwk_Obj_t *)Vec_PtrEntry(p->vObjs, i)), 1); i++ ) \
        if ( (pObj) == NULL || !Nwk_ObjIsNode(pObj) ) {} else
#define Nwk_ManForEachLatch( p, pObj, i )                                  \
    for ( i = 0; (i < Vec_PtrSize(p->vObjs)) && (((pObj) = (Nwk_Obj_t *)Vec_PtrEntry(p->vObjs, i)), 1); i++ ) \
        if ( (pObj) == NULL || !Nwk_ObjIsLatch(pObj) ) {} else

#define Nwk_ObjForEachFanin( pObj, pFanin, i )                                  \
    for ( i = 0; (i < (int)(pObj)->nFanins) && ((pFanin) = (pObj)->pFanio[i]); i++ )
#define Nwk_ObjForEachFanout( pObj, pFanout, i )                                \
    for ( i = 0; (i < (int)(pObj)->nFanouts) && ((pFanout) = (pObj)->pFanio[(pObj)->nFanins+i]); i++ )

// sequential iterators
#define Nwk_ManForEachPiSeq( p, pObj, i )                                           \
    Vec_PtrForEachEntryStop( Nwk_Obj_t *, p->vCis, pObj, i, (p)->nTruePis )
#define Nwk_ManForEachPoSeq( p, pObj, i )                                           \
    Vec_PtrForEachEntryStop( Nwk_Obj_t *, p->vCos, pObj, i, (p)->nTruePos )
#define Nwk_ManForEachLoSeq( p, pObj, i )                                           \
    for ( i = 0; (i < (p)->nLatches) && (((pObj) = (Nwk_Obj_t *)Vec_PtrEntry(p->vCis, i+(p)->nTruePis)), 1); i++ )
#define Nwk_ManForEachLiSeq( p, pObj, i )                                           \
    for ( i = 0; (i < (p)->nLatches) && (((pObj) = (Nwk_Obj_t *)Vec_PtrEntry(p->vCos, i+(p)->nTruePos)), 1); i++ )
#define Nwk_ManForEachLiLoSeq( p, pObjLi, pObjLo, i )                               \
    for ( i = 0; (i < (p)->nLatches) && (((pObjLi) = Nwk_ManCo(p, i+(p)->nTruePos)), 1)        \
        && (((pObjLo) = Nwk_ManCi(p, i+(p)->nTruePis)), 1); i++ )


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== nwkAig.c ==========================================================*/
extern ABC_DLL Vec_Ptr_t *     Nwk_ManDeriveRetimingCut( Aig_Man_t * p, int fForward, int fVerbose );
/*=== nwkBidec.c ==========================================================*/
extern ABC_DLL void            Nwk_ManBidecResyn( Nwk_Man_t * pNtk, int fVerbose );
extern ABC_DLL Hop_Obj_t *     Nwk_NodeIfNodeResyn( Bdc_Man_t * p, Hop_Man_t * pHop, Hop_Obj_t * pRoot, int nVars, Vec_Int_t * vTruth, unsigned * puCare, float dProb );
/*=== nwkCheck.c ==========================================================*/
extern ABC_DLL int             Nwk_ManCheck( Nwk_Man_t * p );
/*=== nwkDfs.c ==========================================================*/
extern ABC_DLL int             Nwk_ManVerifyTopoOrder( Nwk_Man_t * pNtk );
extern ABC_DLL int             Nwk_ManLevelBackup( Nwk_Man_t * pNtk );
extern ABC_DLL int             Nwk_ManLevel( Nwk_Man_t * pNtk );
extern ABC_DLL int             Nwk_ManLevelMax( Nwk_Man_t * pNtk );
extern ABC_DLL Vec_Vec_t *     Nwk_ManLevelize( Nwk_Man_t * pNtk );
extern ABC_DLL Vec_Ptr_t *     Nwk_ManDfs( Nwk_Man_t * pNtk );
extern ABC_DLL Vec_Ptr_t *     Nwk_ManDfsNodes( Nwk_Man_t * pNtk, Nwk_Obj_t ** ppNodes, int nNodes );
extern ABC_DLL Vec_Ptr_t *     Nwk_ManDfsReverse( Nwk_Man_t * pNtk );
extern ABC_DLL Vec_Ptr_t *     Nwk_ManSupportNodes( Nwk_Man_t * pNtk, Nwk_Obj_t ** ppNodes, int nNodes );
extern ABC_DLL void            Nwk_ManSupportSum( Nwk_Man_t * pNtk );
extern ABC_DLL int             Nwk_ObjMffcLabel( Nwk_Obj_t * pNode );
/*=== nwkFanio.c ==========================================================*/
extern ABC_DLL void            Nwk_ObjCollectFanins( Nwk_Obj_t * pNode, Vec_Ptr_t * vNodes );
extern ABC_DLL void            Nwk_ObjCollectFanouts( Nwk_Obj_t * pNode, Vec_Ptr_t * vNodes );
extern ABC_DLL int             Nwk_ObjFindFanin( Nwk_Obj_t * pObj, Nwk_Obj_t * pFanin );
extern ABC_DLL int             Nwk_ObjFindFanout( Nwk_Obj_t * pObj, Nwk_Obj_t * pFanout );
extern ABC_DLL void            Nwk_ObjAddFanin( Nwk_Obj_t * pObj, Nwk_Obj_t * pFanin );
extern ABC_DLL void            Nwk_ObjDeleteFanin( Nwk_Obj_t * pObj, Nwk_Obj_t * pFanin );
extern ABC_DLL void            Nwk_ObjPatchFanin( Nwk_Obj_t * pObj, Nwk_Obj_t * pFaninOld, Nwk_Obj_t * pFaninNew );
extern ABC_DLL void            Nwk_ObjTransferFanout( Nwk_Obj_t * pNodeFrom, Nwk_Obj_t * pNodeTo );
extern ABC_DLL void            Nwk_ObjReplace( Nwk_Obj_t * pNodeOld, Nwk_Obj_t * pNodeNew );
/*=== nwkFlow.c ============================================================*/
extern ABC_DLL Vec_Ptr_t *     Nwk_ManRetimeCutForward( Nwk_Man_t * pMan, int nLatches, int fVerbose );
extern ABC_DLL Vec_Ptr_t *     Nwk_ManRetimeCutBackward( Nwk_Man_t * pMan, int nLatches, int fVerbose );
/*=== nwkMan.c ============================================================*/
extern ABC_DLL Nwk_Man_t *     Nwk_ManAlloc();
extern ABC_DLL void            Nwk_ManFree( Nwk_Man_t * p );
extern ABC_DLL float           Nwl_ManComputeTotalSwitching( Nwk_Man_t * pNtk );
extern ABC_DLL void            Nwk_ManPrintStats( Nwk_Man_t * p, If_LibLut_t * pLutLib, int fSaveBest, int fDumpResult, int fPower, Ntl_Man_t * pNtl );
/*=== nwkMap.c ============================================================*/
extern ABC_DLL Nwk_Man_t *     Nwk_MappingIf( Aig_Man_t * p, Tim_Man_t * pManTime, If_Par_t * pPars );
/*=== nwkObj.c ============================================================*/
extern ABC_DLL Nwk_Obj_t *     Nwk_ManCreateCi( Nwk_Man_t * pMan, int nFanouts );
extern ABC_DLL Nwk_Obj_t *     Nwk_ManCreateCo( Nwk_Man_t * pMan );
extern ABC_DLL Nwk_Obj_t *     Nwk_ManCreateNode( Nwk_Man_t * pMan, int nFanins, int nFanouts );
extern ABC_DLL Nwk_Obj_t *     Nwk_ManCreateBox( Nwk_Man_t * pMan, int nFanins, int nFanouts );
extern ABC_DLL Nwk_Obj_t *     Nwk_ManCreateLatch( Nwk_Man_t * pMan );
extern ABC_DLL void            Nwk_ManDeleteNode( Nwk_Obj_t * pObj );
extern ABC_DLL void            Nwk_ManDeleteNode_rec( Nwk_Obj_t * pObj );
/*=== nwkSpeedup.c ============================================================*/
extern ABC_DLL Aig_Man_t *     Nwk_ManSpeedup( Nwk_Man_t * pNtk, int fUseLutLib, int Percentage, int Degree, int fVerbose, int fVeryVerbose );
/*=== nwkStrash.c ============================================================*/
extern ABC_DLL Aig_Man_t *     Nwk_ManStrash( Nwk_Man_t * pNtk );
/*=== nwkTiming.c ============================================================*/
extern ABC_DLL int             Nwk_ManVerifyTiming(  Nwk_Man_t * pNtk );
extern ABC_DLL void            Nwk_ManDelayTraceSortPins( Nwk_Obj_t * pNode, int * pPinPerm, float * pPinDelays );
extern ABC_DLL float           Nwk_ManDelayTraceLut( Nwk_Man_t * pNtk );
extern ABC_DLL void            Nwk_ManDelayTracePrint( Nwk_Man_t * pNtk );
extern ABC_DLL void            Nwk_ManUpdate( Nwk_Obj_t * pObj, Nwk_Obj_t * pObjNew, Vec_Vec_t * vLevels );
extern ABC_DLL int             Nwk_ManVerifyLevel( Nwk_Man_t * pNtk );
/*=== nwkUtil.c ============================================================*/
extern ABC_DLL void            Nwk_ManIncrementTravId( Nwk_Man_t * pNtk );
extern ABC_DLL int             Nwk_ManGetFaninMax( Nwk_Man_t * pNtk );
extern ABC_DLL int             Nwk_ManGetTotalFanins( Nwk_Man_t * pNtk );
extern ABC_DLL int             Nwk_ManPiNum( Nwk_Man_t * pNtk );
extern ABC_DLL int             Nwk_ManPoNum( Nwk_Man_t * pNtk );
extern ABC_DLL int             Nwk_ManGetAigNodeNum( Nwk_Man_t * pNtk );
extern ABC_DLL int             Nwk_NodeCompareLevelsIncrease( Nwk_Obj_t ** pp1, Nwk_Obj_t ** pp2 );
extern ABC_DLL int             Nwk_NodeCompareLevelsDecrease( Nwk_Obj_t ** pp1, Nwk_Obj_t ** pp2 );
extern ABC_DLL void            Nwk_ObjPrint( Nwk_Obj_t * pObj );
extern ABC_DLL void            Nwk_ManDumpBlif( Nwk_Man_t * pNtk, char * pFileName, Vec_Ptr_t * vCiNames, Vec_Ptr_t * vCoNames );
extern ABC_DLL void            Nwk_ManPrintFanioNew( Nwk_Man_t * pNtk );
extern ABC_DLL void            Nwk_ManCleanMarks( Nwk_Man_t * pNtk );
extern ABC_DLL void            Nwk_ManMinimumBase( Nwk_Man_t * pNtk, int fVerbose );
extern ABC_DLL void            Nwk_ManRemoveDupFanins( Nwk_Man_t * pNtk, int fVerbose );



ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

