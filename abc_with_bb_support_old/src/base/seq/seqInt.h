/**CFile****************************************************************

  FileName    [seqInt.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Construction and manipulation of sequential AIGs.]

  Synopsis    [Internal declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: seqInt.h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef __SEQ_INT_H__
#define __SEQ_INT_H__

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "abc.h"
#include "cut.h"
#include "main.h"
#include "mio.h"
#include "mapper.h"
#include "fpga.h"
#include "seq.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

#define SEQ_FULL_MASK        0xFFFFFFFF   

// node status after updating its arrival time
enum { SEQ_UPDATE_FAIL, SEQ_UPDATE_NO, SEQ_UPDATE_YES };

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// manager of sequential AIG
struct Abc_Seq_t_
{
    // sequential information
    Abc_Ntk_t *         pNtk;           // the network
    int                 nSize;          // the number of entries in all internal arrays
    Vec_Int_t *         vNums;          // the number of latches on each edge in the AIG
    Vec_Ptr_t *         vInits;         // the initial states for each edge in the AIG
    Extra_MmFixed_t *   pMmInits;       // memory manager for latch structures used to remember init states
    int                 fVerbose;       // the verbose flag
    float               fEpsilon;       // the accuracy for delay computation
    int                 fStandCells;    // the flag denoting standard cell mapping
    int                 nMaxIters;      // the max number of iterations
    int                 FiBestInt;      // the best clock period
    float               FiBestFloat;    // the best clock period
    // K-feasible cuts
    int                 nVarsMax;       // the max cut size
    Cut_Man_t *         pCutMan;        // cut manager
    Map_SuperLib_t *    pSuperLib;      // the current supergate library
    // sequential arrival time computation
    Vec_Int_t *         vAFlows;        // the area flow of each cut
    Vec_Int_t *         vLValues;       // the arrival times (L-Values of nodes)
    Vec_Int_t *         vLValuesN;      // the arrival times (L-Values of nodes)
    Vec_Str_t *         vLags;          // the lags of the mapped nodes
    Vec_Str_t *         vLagsN;         // the lags of the mapped nodes
    Vec_Str_t *         vUses;          // the phase usage
    // representation of the mapping
    Vec_Ptr_t *         vMapAnds;       // nodes visible in the mapping 
    Vec_Vec_t *         vMapCuts;       // best cuts for each node 
    Vec_Vec_t *         vMapDelays;     // the delay of each fanin
    Vec_Vec_t *         vMapFanins;     // the delay of each fanin
    // runtime stats
    int                 timeCuts;       // runtime to compute the cuts
    int                 timeDelay;      // runtime to compute the L-values
    int                 timeRet;        // runtime to retime the resulting network
    int                 timeNtk;        // runtime to create the final network

};

// data structure to store initial state
typedef struct Seq_Lat_t_  Seq_Lat_t;
struct Seq_Lat_t_
{
    Seq_Lat_t *         pNext;          // the next Lat in the ring
    Seq_Lat_t *         pPrev;          // the prev Lat in the ring 
    Abc_Obj_t *         pLatch;         // the real latch corresponding to Lat
};

// representation of latch on the edge
typedef struct Seq_RetEdge_t_       Seq_RetEdge_t;   
struct Seq_RetEdge_t_ // 1 word
{
    unsigned            iNode    : 24;  // the ID of the node
    unsigned            iEdge    :  1;  // the edge of the node
    unsigned            iLatch   :  7;  // the latch number counting from the node
};

// representation of one retiming step
typedef struct Seq_RetStep_t_       Seq_RetStep_t;   
struct Seq_RetStep_t_ // 1 word
{
    unsigned            iNode    : 24;  // the ID of the node
    unsigned            nLatches :  8;  // the number of latches to retime
};

// representation of one mapping match
typedef struct Seq_Match_t_       Seq_Match_t;   
struct Seq_Match_t_ // 3 words
{
    Abc_Obj_t *         pAnd;           // the AND gate used in the mapping
    Cut_Cut_t *         pCut;           // the cut used to map it
    Map_Super_t *       pSuper;         // the supergate used to implement the cut
    unsigned            fCompl  :  1;   // the polarity of the AND gate
    unsigned            fCutInv :  1;   // the polarity of the cut
    unsigned            PolUse  :  2;   // the polarity use of this node
    unsigned            uPhase  : 14;   // the phase assignment at the boundary
    unsigned            uPhaseR : 14;   // the real phase assignment at the boundary
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

// transforming retedges into ints and back
static inline int            Seq_RetEdge2Int( Seq_RetEdge_t Val )  { return *((int *)&Val);           }
static inline Seq_RetEdge_t  Seq_Int2RetEdge( int Num )            { return *((Seq_RetEdge_t *)&Num); }
// transforming retsteps into ints and back
static inline int            Seq_RetStep2Int( Seq_RetStep_t Val )  { return *((int *)&Val);           }
static inline Seq_RetStep_t  Seq_Int2RetStep( int Num )            { return *((Seq_RetStep_t *)&Num); }

// manipulating the number of latches on each edge
static inline Vec_Int_t *    Seq_ObjLNums( Abc_Obj_t * pObj )                        { return ((Abc_Seq_t*)pObj->pNtk->pManFunc)->vNums;                    }
static inline int            Seq_ObjFaninL( Abc_Obj_t * pObj, int i )                { return Vec_IntEntry(Seq_ObjLNums(pObj), 2*pObj->Id + i);             }
static inline int            Seq_ObjFaninL0( Abc_Obj_t * pObj )                      { return Vec_IntEntry(Seq_ObjLNums(pObj), 2*pObj->Id + 0);             }
static inline int            Seq_ObjFaninL1( Abc_Obj_t * pObj )                      { return Vec_IntEntry(Seq_ObjLNums(pObj), 2*pObj->Id + 1);             }
static inline void           Seq_ObjSetFaninL( Abc_Obj_t * pObj, int i, int nLats )  { Vec_IntWriteEntry(Seq_ObjLNums(pObj), 2*pObj->Id + i, nLats);        }
static inline void           Seq_ObjSetFaninL0( Abc_Obj_t * pObj, int nLats )        { Vec_IntWriteEntry(Seq_ObjLNums(pObj), 2*pObj->Id + 0, nLats);        }
static inline void           Seq_ObjSetFaninL1( Abc_Obj_t * pObj, int nLats )        { Vec_IntWriteEntry(Seq_ObjLNums(pObj), 2*pObj->Id + 1, nLats);        }
static inline void           Seq_ObjAddFaninL( Abc_Obj_t * pObj, int i, int nLats )  { Vec_IntAddToEntry(Seq_ObjLNums(pObj), 2*pObj->Id + i, nLats);        }
static inline void           Seq_ObjAddFaninL0( Abc_Obj_t * pObj, int nLats )        { Vec_IntAddToEntry(Seq_ObjLNums(pObj), 2*pObj->Id + 0, nLats);        }
static inline void           Seq_ObjAddFaninL1( Abc_Obj_t * pObj, int nLats )        { Vec_IntAddToEntry(Seq_ObjLNums(pObj), 2*pObj->Id + 1, nLats);        }
static inline int            Seq_ObjFanoutL( Abc_Obj_t * pObj, Abc_Obj_t * pFanout ) { return Seq_ObjFaninL( pFanout, Abc_ObjFanoutEdgeNum(pObj,pFanout) ); }
static inline void           Seq_ObjSetFanoutL( Abc_Obj_t * pObj, Abc_Obj_t * pFanout, int nLats ) {  Seq_ObjSetFaninL( pFanout, Abc_ObjFanoutEdgeNum(pObj,pFanout), nLats );  }
static inline void           Seq_ObjAddFanoutL( Abc_Obj_t * pObj, Abc_Obj_t * pFanout, int nLats ) {  Seq_ObjAddFaninL( pFanout, Abc_ObjFanoutEdgeNum(pObj,pFanout), nLats );  }
static inline int            Seq_ObjFaninLMin( Abc_Obj_t * pObj )                    {  assert( Abc_ObjIsNode(pObj) ); return ABC_MIN( Seq_ObjFaninL0(pObj), Seq_ObjFaninL1(pObj) ); }
static inline int            Seq_ObjFaninLMax( Abc_Obj_t * pObj )                    {  assert( Abc_ObjIsNode(pObj) ); return ABC_MAX( Seq_ObjFaninL0(pObj), Seq_ObjFaninL1(pObj) ); }

// reading l-values and lags
static inline Vec_Int_t *    Seq_NodeLValues( Abc_Obj_t * pNode )                    { return ((Abc_Seq_t *)(pNode)->pNtk->pManFunc)->vLValues;           }
static inline Vec_Int_t *    Seq_NodeLValuesN( Abc_Obj_t * pNode )                   { return ((Abc_Seq_t *)(pNode)->pNtk->pManFunc)->vLValuesN;          }
static inline int            Seq_NodeGetLValue( Abc_Obj_t * pNode )                  { return Vec_IntEntry( Seq_NodeLValues(pNode), (pNode)->Id );        }
static inline void           Seq_NodeSetLValue( Abc_Obj_t * pNode, int Value )       { Vec_IntWriteEntry( Seq_NodeLValues(pNode), (pNode)->Id, Value );   }
static inline float          Seq_NodeGetLValueP( Abc_Obj_t * pNode )                 { return Abc_Int2Float( Vec_IntEntry( Seq_NodeLValues(pNode), (pNode)->Id ) );        }
static inline float          Seq_NodeGetLValueN( Abc_Obj_t * pNode )                 { return Abc_Int2Float( Vec_IntEntry( Seq_NodeLValuesN(pNode), (pNode)->Id ) );       }
static inline void           Seq_NodeSetLValueP( Abc_Obj_t * pNode, float Value )    { Vec_IntWriteEntry( Seq_NodeLValues(pNode), (pNode)->Id, Abc_Float2Int(Value) );     }
static inline void           Seq_NodeSetLValueN( Abc_Obj_t * pNode, float Value )    { Vec_IntWriteEntry( Seq_NodeLValuesN(pNode), (pNode)->Id, Abc_Float2Int(Value) );    }

// reading area flows
static inline Vec_Int_t *    Seq_NodeFlow( Abc_Obj_t * pNode )                       { return ((Abc_Seq_t *)(pNode)->pNtk->pManFunc)->vAFlows;                        }
static inline float          Seq_NodeGetFlow( Abc_Obj_t * pNode )                    { return Abc_Int2Float( Vec_IntEntry( Seq_NodeFlow(pNode), (pNode)->Id ) );      }
static inline void           Seq_NodeSetFlow( Abc_Obj_t * pNode, float Value )       { Vec_IntWriteEntry( Seq_NodeFlow(pNode), (pNode)->Id, Abc_Float2Int(Value) );   }

// reading the contents of the lat
static inline Abc_InitType_t Seq_LatInit( Seq_Lat_t * pLat )  { return ((unsigned)pLat->pPrev) & 3;                                 }
static inline Seq_Lat_t *    Seq_LatNext( Seq_Lat_t * pLat )  { return pLat->pNext;                                                 }
static inline Seq_Lat_t *    Seq_LatPrev( Seq_Lat_t * pLat )  { return (void *)(((unsigned)pLat->pPrev) & (SEQ_FULL_MASK << 2));    }

// setting the contents of the lat
static inline void           Seq_LatSetInit( Seq_Lat_t * pLat, Abc_InitType_t Init )  { pLat->pPrev = (void *)( (3 & Init) | (((unsigned)pLat->pPrev) & (SEQ_FULL_MASK << 2)) );    }
static inline void           Seq_LatSetNext( Seq_Lat_t * pLat, Seq_Lat_t * pNext )    { pLat->pNext = pNext;                                                                        }
static inline void           Seq_LatSetPrev( Seq_Lat_t * pLat, Seq_Lat_t * pPrev )    { Abc_InitType_t Init = Seq_LatInit(pLat);  pLat->pPrev = pPrev;  Seq_LatSetInit(pLat, Init); }

// accessing retiming lags
static inline Cut_Man_t *    Seq_NodeCutMan( Abc_Obj_t * pNode )                      { return ((Abc_Seq_t *)(pNode)->pNtk->pManFunc)->pCutMan;                 }
static inline Vec_Str_t *    Seq_NodeLags( Abc_Obj_t * pNode )                        { return ((Abc_Seq_t *)(pNode)->pNtk->pManFunc)->vLags;            }
static inline Vec_Str_t *    Seq_NodeLagsN( Abc_Obj_t * pNode )                       { return ((Abc_Seq_t *)(pNode)->pNtk->pManFunc)->vLagsN;           }
static inline char           Seq_NodeGetLag( Abc_Obj_t * pNode )                      { return Vec_StrEntry( Seq_NodeLags(pNode), (pNode)->Id );         }
static inline char           Seq_NodeGetLagN( Abc_Obj_t * pNode )                     { return Vec_StrEntry( Seq_NodeLagsN(pNode), (pNode)->Id );        }
static inline void           Seq_NodeSetLag( Abc_Obj_t * pNode, char Value )          { Vec_StrWriteEntry( Seq_NodeLags(pNode), (pNode)->Id, (Value) );  }
static inline void           Seq_NodeSetLagN( Abc_Obj_t * pNode, char Value )         { Vec_StrWriteEntry( Seq_NodeLagsN(pNode), (pNode)->Id, (Value) ); }
static inline int            Seq_NodeComputeLag( int LValue, int Fi )                 { return (LValue + 1024*Fi)/Fi - 1024 - (int)(LValue % Fi == 0);     }
static inline int            Seq_NodeComputeLagFloat( float LValue, float Fi )        { return ((int)ceil(LValue/Fi)) - 1;                                        }

// phase usage
static inline Vec_Str_t *    Seq_NodeUses( Abc_Obj_t * pNode )                        { return ((Abc_Seq_t *)(pNode)->pNtk->pManFunc)->vUses;            }
static inline char           Seq_NodeGetUses( Abc_Obj_t * pNode )                     { return Vec_StrEntry( Seq_NodeUses(pNode), (pNode)->Id );         }
static inline void           Seq_NodeSetUses( Abc_Obj_t * pNode, char Value )         { Vec_StrWriteEntry( Seq_NodeUses(pNode), (pNode)->Id, (Value) );  }

// accessing initial states
static inline Vec_Ptr_t *    Seq_NodeLats( Abc_Obj_t * pObj )                                { return ((Abc_Seq_t*)pObj->pNtk->pManFunc)->vInits;                                           }
static inline Seq_Lat_t *    Seq_NodeGetRing( Abc_Obj_t * pObj, int Edge )                   { return Vec_PtrEntry( Seq_NodeLats(pObj), (pObj->Id<<1)+Edge );                               }
static inline void           Seq_NodeSetRing( Abc_Obj_t * pObj, int Edge, Seq_Lat_t * pLat ) { Vec_PtrWriteEntry( Seq_NodeLats(pObj), (pObj->Id<<1)+Edge, pLat );                           }
static inline Seq_Lat_t *    Seq_NodeCreateLat( Abc_Obj_t * pObj )                           { Seq_Lat_t * p = (Seq_Lat_t *)Extra_MmFixedEntryFetch( ((Abc_Seq_t*)pObj->pNtk->pManFunc)->pMmInits ); p->pNext = p->pPrev = NULL; p->pLatch = NULL; return p; }
static inline void           Seq_NodeRecycleLat( Abc_Obj_t * pObj, Seq_Lat_t * pLat )        { Extra_MmFixedEntryRecycle( ((Abc_Seq_t*)pObj->pNtk->pManFunc)->pMmInits, (char *)pLat );     }

// getting hold of the structure storing initial states of the latches
static inline Seq_Lat_t *    Seq_NodeGetLatFirst( Abc_Obj_t * pObj, int Edge )               { return Seq_NodeGetRing(pObj, Edge);                      }
static inline Seq_Lat_t *    Seq_NodeGetLatLast( Abc_Obj_t * pObj, int Edge )                { return Seq_LatPrev( Seq_NodeGetRing(pObj, Edge) );       }
static inline Seq_Lat_t *    Seq_NodeGetLat( Abc_Obj_t * pObj, int Edge, int iLat )          { int c; Seq_Lat_t * pLat = Seq_NodeGetRing(pObj, Edge); for ( c = 0; c != iLat; c++ ) pLat = pLat->pNext; return pLat; }
static inline int            Seq_NodeCountLats( Abc_Obj_t * pObj, int Edge )                 { int c; Seq_Lat_t * pLat, * pRing = Seq_NodeGetRing(pObj, Edge); if ( pRing == NULL ) return 0; for ( c = 0, pLat = pRing; !c || pLat != pRing; c++ )  pLat = pLat->pNext; return c; }
static inline void           Seq_NodeCleanLats( Abc_Obj_t * pObj, int Edge )                 { int c; Seq_Lat_t * pLat, * pRing = Seq_NodeGetRing(pObj, Edge); if ( pRing == NULL ) return  ; for ( c = 0, pLat = pRing; !c || pLat != pRing; c++ )  pLat->pLatch = NULL, pLat = pLat->pNext; return;  }

// getting/setting initial states of the latches
static inline Abc_InitType_t Seq_NodeGetInitOne( Abc_Obj_t * pObj, int Edge, int iLat )      { return Seq_LatInit( Seq_NodeGetLat(pObj, Edge, iLat) );  }
static inline Abc_InitType_t Seq_NodeGetInitFirst( Abc_Obj_t * pObj, int Edge )              { return Seq_LatInit( Seq_NodeGetLatFirst(pObj, Edge) );   }
static inline Abc_InitType_t Seq_NodeGetInitLast( Abc_Obj_t * pObj, int Edge )               { return Seq_LatInit( Seq_NodeGetLatLast(pObj, Edge) );    }
static inline void           Seq_NodeSetInitOne( Abc_Obj_t * pObj, int Edge, int iLat, Abc_InitType_t Init )  { Seq_LatSetInit( Seq_NodeGetLat(pObj, Edge, iLat), Init );  }

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== seqAigIter.c =============================================================*/
extern int                   Seq_AigRetimeDelayLags( Abc_Ntk_t * pNtk, int fVerbose );
extern int                   Seq_NtkImplementRetiming( Abc_Ntk_t * pNtk, Vec_Str_t * vLags, int fVerbose );
/*=== seqFpgaIter.c ============================================================*/
extern int                   Seq_FpgaMappingDelays( Abc_Ntk_t * pNtk, int fVerbose );
extern int                   Seq_FpgaNodeUpdateLValue( Abc_Obj_t * pObj, int Fi );
/*=== seqMapIter.c ============================================================*/
extern int                   Seq_MapRetimeDelayLags( Abc_Ntk_t * pNtk, int fVerbose );
/*=== seqRetIter.c =============================================================*/
extern int                   Seq_NtkRetimeDelayLags( Abc_Ntk_t * pNtkOld, Abc_Ntk_t * pNtk, int fVerbose );
/*=== seqLatch.c ===============================================================*/
extern void                  Seq_NodeInsertFirst( Abc_Obj_t * pObj, int Edge, Abc_InitType_t Init );  
extern void                  Seq_NodeInsertLast( Abc_Obj_t * pObj, int Edge, Abc_InitType_t Init );  
extern Abc_InitType_t        Seq_NodeDeleteFirst( Abc_Obj_t * pObj, int Edge );  
extern Abc_InitType_t        Seq_NodeDeleteLast( Abc_Obj_t * pObj, int Edge );  
/*=== seqUtil.c ================================================================*/
extern int                   Seq_NtkLevelMax( Abc_Ntk_t * pNtk );
extern int                   Seq_ObjFanoutLMax( Abc_Obj_t * pObj );
extern int                   Seq_ObjFanoutLMin( Abc_Obj_t * pObj );
extern int                   Seq_ObjFanoutLSum( Abc_Obj_t * pObj );
extern int                   Seq_ObjFaninLSum( Abc_Obj_t * pObj );

#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

