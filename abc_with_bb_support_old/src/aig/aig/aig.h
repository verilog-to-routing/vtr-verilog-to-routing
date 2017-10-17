/**CFile****************************************************************

  FileName    [aig.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [AIG package.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - April 28, 2007.]

  Revision    [$Id: aig.h,v 1.00 2007/04/28 00:00:00 alanmi Exp $]

***********************************************************************/

#ifndef __AIG_H__
#define __AIG_H__

#ifdef __cplusplus
extern "C" {
#endif 

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "vec.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Aig_Man_t_            Aig_Man_t;
typedef struct Aig_Obj_t_            Aig_Obj_t;
typedef struct Aig_MmFixed_t_        Aig_MmFixed_t;    
typedef struct Aig_MmFlex_t_         Aig_MmFlex_t;     
typedef struct Aig_MmStep_t_         Aig_MmStep_t;     

// object types
typedef enum { 
    AIG_OBJ_NONE,                    // 0: non-existent object
    AIG_OBJ_CONST1,                  // 1: constant 1 
    AIG_OBJ_PI,                      // 2: primary input
    AIG_OBJ_PO,                      // 3: primary output
    AIG_OBJ_BUF,                     // 4: buffer node
    AIG_OBJ_AND,                     // 5: AND node
    AIG_OBJ_EXOR,                    // 6: EXOR node
    AIG_OBJ_LATCH,                   // 7: latch
    AIG_OBJ_VOID                     // 8: unused object
} Aig_Type_t;

// the AIG node
struct Aig_Obj_t_  // 8 words
{
    void *           pData;          // misc (cuts, copy, etc)
    Aig_Obj_t *      pNext;          // strashing table
    Aig_Obj_t *      pFanin0;        // fanin
    Aig_Obj_t *      pFanin1;        // fanin
    unsigned long    Type    :  3;   // object type
    unsigned long    fPhase  :  1;   // value under 000...0 pattern
    unsigned long    fMarkA  :  1;   // multipurpose mask
    unsigned long    fMarkB  :  1;   // multipurpose mask
    unsigned long    nRefs   : 26;   // reference count 
    unsigned         Level   : 24;   // the level of this node
    unsigned         nCuts   :  8;   // the number of cuts
    int              TravId;         // unique ID of last traversal involving the node
    int              Id;             // unique ID of the node
};

// the AIG manager
struct Aig_Man_t_
{
    // AIG nodes
    Vec_Ptr_t *      vPis;           // the array of PIs
    Vec_Ptr_t *      vPos;           // the array of POs
    Vec_Ptr_t *      vObjs;          // the array of all nodes (optional)
    Vec_Ptr_t *      vBufs;          // the array of buffers
    Aig_Obj_t *      pConst1;        // the constant 1 node
    Aig_Obj_t        Ghost;          // the ghost node
    int              nRegs;          // the number of registers (registers are last POs)
    int              nAsserts;       // the number of asserts among POs (asserts are first POs)
    // AIG node counters
    int              nObjs[AIG_OBJ_VOID];// the number of objects by type
    int              nCreated;       // the number of created objects
    int              nDeleted;       // the number of deleted objects
    // structural hash table
    Aig_Obj_t **     pTable;         // structural hash table
    int              nTableSize;     // structural hash table size
    // representation of fanouts
    int *            pFanData;       // the database to store fanout information
    int              nFansAlloc;     // the size of fanout representation
    Vec_Vec_t *      vLevels;        // used to update timing information
    int              nBufReplaces;   // the number of times replacement led to a buffer
    int              nBufFixes;      // the number of times buffers were propagated
    int              nBufMax;        // the maximum number of buffers during computation
    // topological order
    unsigned *       pOrderData;
    int              nOrderAlloc;
    int              iPrev;
    int              iNext;
    int              nAndTotal;
    int              nAndPrev;
    // representatives
    Aig_Obj_t **     pEquivs;        // linked list of equivalent nodes (when choices are used)
    Aig_Obj_t **     pReprs;         // representatives of each node
    int              nReprsAlloc;    // the number of allocated representatives
    // various data members
    Aig_MmFixed_t *  pMemObjs;       // memory manager for objects
    Vec_Int_t *      vLevelR;        // the reverse level of the nodes
    int              nLevelMax;      // maximum number of levels
    void *           pData;          // the temporary data
    int              nTravIds;       // the current traversal ID
    int              fCatchExor;     // enables EXOR nodes
    // timing statistics
    int              time1;
    int              time2;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

#define AIG_MIN(a,b)       (((a) < (b))? (a) : (b))
#define AIG_MAX(a,b)       (((a) > (b))? (a) : (b))
#define AIG_ABS(a)         (((a) >= 0)?  (a) :-(a))
#define AIG_INFINITY       (100000000)

#ifndef PRT
#define PRT(a,t)  printf("%s = ", (a)); printf("%6.2f sec\n", (float)(t)/(float)(CLOCKS_PER_SEC))
#endif

static inline int          Aig_BitWordNum( int nBits )            { return (nBits>>5) + ((nBits&31) > 0);           }
static inline int          Aig_TruthWordNum( int nVars )          { return nVars <= 5 ? 1 : (1 << (nVars - 5));     }
static inline int          Aig_InfoHasBit( unsigned * p, int i )  { return (p[(i)>>5] & (1<<((i) & 31))) > 0;       }
static inline void         Aig_InfoSetBit( unsigned * p, int i )  { p[(i)>>5] |= (1<<((i) & 31));                   }
static inline void         Aig_InfoXorBit( unsigned * p, int i )  { p[(i)>>5] ^= (1<<((i) & 31));                   }
static inline unsigned     Aig_ObjCutSign( unsigned ObjId )       { return (1 << (ObjId & 31));                     }

static inline Aig_Obj_t *  Aig_Regular( Aig_Obj_t * p )           { return (Aig_Obj_t *)((unsigned long)(p) & ~01); }
static inline Aig_Obj_t *  Aig_Not( Aig_Obj_t * p )               { return (Aig_Obj_t *)((unsigned long)(p) ^  01); }
static inline Aig_Obj_t *  Aig_NotCond( Aig_Obj_t * p, int c )    { return (Aig_Obj_t *)((unsigned long)(p) ^ (c)); }
static inline int          Aig_IsComplement( Aig_Obj_t * p )      { return (int )(((unsigned long)p) & 01);         }

static inline int          Aig_ManPiNum( Aig_Man_t * p )          { return p->nObjs[AIG_OBJ_PI];                    }
static inline int          Aig_ManPoNum( Aig_Man_t * p )          { return p->nObjs[AIG_OBJ_PO];                    }
static inline int          Aig_ManBufNum( Aig_Man_t * p )         { return p->nObjs[AIG_OBJ_BUF];                   }
static inline int          Aig_ManAndNum( Aig_Man_t * p )         { return p->nObjs[AIG_OBJ_AND];                   }
static inline int          Aig_ManExorNum( Aig_Man_t * p )        { return p->nObjs[AIG_OBJ_EXOR];                  }
static inline int          Aig_ManLatchNum( Aig_Man_t * p )       { return p->nObjs[AIG_OBJ_LATCH];                 }
static inline int          Aig_ManNodeNum( Aig_Man_t * p )        { return p->nObjs[AIG_OBJ_AND]+p->nObjs[AIG_OBJ_EXOR];   }
static inline int          Aig_ManGetCost( Aig_Man_t * p )        { return p->nObjs[AIG_OBJ_AND]+3*p->nObjs[AIG_OBJ_EXOR]; }
static inline int          Aig_ManObjNum( Aig_Man_t * p )         { return p->nCreated - p->nDeleted;               }
static inline int          Aig_ManObjIdMax( Aig_Man_t * p )       { return Vec_PtrSize(p->vObjs);                   }
static inline int          Aig_ManRegNum( Aig_Man_t * p )         { return p->nRegs;                                }

static inline Aig_Obj_t *  Aig_ManConst0( Aig_Man_t * p )         { return Aig_Not(p->pConst1);                     }
static inline Aig_Obj_t *  Aig_ManConst1( Aig_Man_t * p )         { return p->pConst1;                              }
static inline Aig_Obj_t *  Aig_ManGhost( Aig_Man_t * p )          { return &p->Ghost;                               }
static inline Aig_Obj_t *  Aig_ManPi( Aig_Man_t * p, int i )      { return (Aig_Obj_t *)Vec_PtrEntry(p->vPis, i);   }
static inline Aig_Obj_t *  Aig_ManPo( Aig_Man_t * p, int i )      { return (Aig_Obj_t *)Vec_PtrEntry(p->vPos, i);   }
static inline Aig_Obj_t *  Aig_ManLo( Aig_Man_t * p, int i )      { return (Aig_Obj_t *)Vec_PtrEntry(p->vPis, Aig_ManPiNum(p)-Aig_ManRegNum(p)+i);   }
static inline Aig_Obj_t *  Aig_ManLi( Aig_Man_t * p, int i )      { return (Aig_Obj_t *)Vec_PtrEntry(p->vPos, Aig_ManPoNum(p)-Aig_ManRegNum(p)+i);   }
static inline Aig_Obj_t *  Aig_ManObj( Aig_Man_t * p, int i )     { return p->vObjs ? (Aig_Obj_t *)Vec_PtrEntry(p->vObjs, i) : NULL;  }

static inline Aig_Type_t   Aig_ObjType( Aig_Obj_t * pObj )        { return (Aig_Type_t)pObj->Type;       }
static inline int          Aig_ObjIsNone( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_NONE;   }
static inline int          Aig_ObjIsConst1( Aig_Obj_t * pObj )    { assert(!Aig_IsComplement(pObj)); return pObj->Type == AIG_OBJ_CONST1; }
static inline int          Aig_ObjIsPi( Aig_Obj_t * pObj )        { return pObj->Type == AIG_OBJ_PI;     }
static inline int          Aig_ObjIsPo( Aig_Obj_t * pObj )        { return pObj->Type == AIG_OBJ_PO;     }
static inline int          Aig_ObjIsBuf( Aig_Obj_t * pObj )       { return pObj->Type == AIG_OBJ_BUF;    }
static inline int          Aig_ObjIsAnd( Aig_Obj_t * pObj )       { return pObj->Type == AIG_OBJ_AND;    }
static inline int          Aig_ObjIsExor( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_EXOR;   }
static inline int          Aig_ObjIsLatch( Aig_Obj_t * pObj )     { return pObj->Type == AIG_OBJ_LATCH;  }
static inline int          Aig_ObjIsNode( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_AND || pObj->Type == AIG_OBJ_EXOR;   }
static inline int          Aig_ObjIsTerm( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_PI  || pObj->Type == AIG_OBJ_PO || pObj->Type == AIG_OBJ_CONST1;  }
static inline int          Aig_ObjIsHash( Aig_Obj_t * pObj )      { return pObj->Type == AIG_OBJ_AND || pObj->Type == AIG_OBJ_EXOR || pObj->Type == AIG_OBJ_LATCH; }

static inline int          Aig_ObjIsMarkA( Aig_Obj_t * pObj )     { return pObj->fMarkA;  }
static inline void         Aig_ObjSetMarkA( Aig_Obj_t * pObj )    { pObj->fMarkA = 1;     }
static inline void         Aig_ObjClearMarkA( Aig_Obj_t * pObj )  { pObj->fMarkA = 0;     }
 
static inline void         Aig_ObjSetTravId( Aig_Obj_t * pObj, int TravId )                { pObj->TravId = TravId;                         }
static inline void         Aig_ObjSetTravIdCurrent( Aig_Man_t * p, Aig_Obj_t * pObj )      { pObj->TravId = p->nTravIds;                    }
static inline void         Aig_ObjSetTravIdPrevious( Aig_Man_t * p, Aig_Obj_t * pObj )     { pObj->TravId = p->nTravIds - 1;                }
static inline int          Aig_ObjIsTravIdCurrent( Aig_Man_t * p, Aig_Obj_t * pObj )       { return (int)(pObj->TravId == p->nTravIds);     }
static inline int          Aig_ObjIsTravIdPrevious( Aig_Man_t * p, Aig_Obj_t * pObj )      { return (int)(pObj->TravId == p->nTravIds - 1); }

static inline int          Aig_ObjTravId( Aig_Obj_t * pObj )      { return (int)pObj->pData;                       }
static inline int          Aig_ObjPhase( Aig_Obj_t * pObj )       { return pObj->fPhase;                           }
static inline int          Aig_ObjPhaseReal( Aig_Obj_t * pObj )   { return pObj? Aig_Regular(pObj)->fPhase ^ Aig_IsComplement(pObj) : 1;                              }
static inline int          Aig_ObjRefs( Aig_Obj_t * pObj )        { return pObj->nRefs;                            }
static inline void         Aig_ObjRef( Aig_Obj_t * pObj )         { pObj->nRefs++;                                 }
static inline void         Aig_ObjDeref( Aig_Obj_t * pObj )       { assert( pObj->nRefs > 0 ); pObj->nRefs--;      }
static inline void         Aig_ObjClearRef( Aig_Obj_t * pObj )    { pObj->nRefs = 0;                               }
static inline int          Aig_ObjFaninId0( Aig_Obj_t * pObj )    { return pObj->pFanin0? Aig_Regular(pObj->pFanin0)->Id : -1; }
static inline int          Aig_ObjFaninId1( Aig_Obj_t * pObj )    { return pObj->pFanin1? Aig_Regular(pObj->pFanin1)->Id : -1; }
static inline int          Aig_ObjFaninC0( Aig_Obj_t * pObj )     { return Aig_IsComplement(pObj->pFanin0);        }
static inline int          Aig_ObjFaninC1( Aig_Obj_t * pObj )     { return Aig_IsComplement(pObj->pFanin1);        }
static inline Aig_Obj_t *  Aig_ObjFanin0( Aig_Obj_t * pObj )      { return Aig_Regular(pObj->pFanin0);             }
static inline Aig_Obj_t *  Aig_ObjFanin1( Aig_Obj_t * pObj )      { return Aig_Regular(pObj->pFanin1);             }
static inline Aig_Obj_t *  Aig_ObjChild0( Aig_Obj_t * pObj )      { return pObj->pFanin0;                          }
static inline Aig_Obj_t *  Aig_ObjChild1( Aig_Obj_t * pObj )      { return pObj->pFanin1;                          }
static inline Aig_Obj_t *  Aig_ObjChild0Copy( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin0(pObj)? Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin0(pObj)->pData, Aig_ObjFaninC0(pObj)) : NULL;  }
static inline Aig_Obj_t *  Aig_ObjChild1Copy( Aig_Obj_t * pObj )  { assert( !Aig_IsComplement(pObj) ); return Aig_ObjFanin1(pObj)? Aig_NotCond((Aig_Obj_t *)Aig_ObjFanin1(pObj)->pData, Aig_ObjFaninC1(pObj)) : NULL;  }
static inline int          Aig_ObjLevel( Aig_Obj_t * pObj )       { return pObj->Level;                            }
static inline int          Aig_ObjLevelNew( Aig_Obj_t * pObj )    { return Aig_ObjFanin1(pObj)? 1 + Aig_ObjIsExor(pObj) + AIG_MAX(Aig_ObjFanin0(pObj)->Level, Aig_ObjFanin1(pObj)->Level) : Aig_ObjFanin0(pObj)->Level; }
static inline void         Aig_ObjClean( Aig_Obj_t * pObj )       { memset( pObj, 0, sizeof(Aig_Obj_t) );                                                             }
static inline Aig_Obj_t *  Aig_ObjFanout0( Aig_Man_t * p, Aig_Obj_t * pObj )  { assert(p->pFanData && pObj->Id < p->nFansAlloc); return Aig_ManObj(p, p->pFanData[5*pObj->Id] >> 1); } 
static inline int          Aig_ObjWhatFanin( Aig_Obj_t * pObj, Aig_Obj_t * pFanin )    
{ 
    if ( Aig_ObjFanin0(pObj) == pFanin ) return 0; 
    if ( Aig_ObjFanin1(pObj) == pFanin ) return 1; 
    assert(0); return -1; 
}
static inline int          Aig_ObjFanoutC( Aig_Obj_t * pObj, Aig_Obj_t * pFanout )    
{ 
    if ( Aig_ObjFanin0(pFanout) == pObj ) return Aig_ObjFaninC0(pObj); 
    if ( Aig_ObjFanin1(pFanout) == pObj ) return Aig_ObjFaninC1(pObj); 
    assert(0); return -1; 
}

// create the ghost of the new node
static inline Aig_Obj_t *  Aig_ObjCreateGhost( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1, Aig_Type_t Type )    
{
    Aig_Obj_t * pGhost;
    assert( Type != AIG_OBJ_AND || !Aig_ObjIsConst1(Aig_Regular(p0)) );
    assert( p1 == NULL || !Aig_ObjIsConst1(Aig_Regular(p1)) );
    assert( Type == AIG_OBJ_PI || Aig_Regular(p0) != Aig_Regular(p1) );
    pGhost = Aig_ManGhost(p);
    pGhost->Type = Type;
    if ( p1 == NULL || Aig_Regular(p0)->Id < Aig_Regular(p1)->Id )
    {
        pGhost->pFanin0 = p0;
        pGhost->pFanin1 = p1;
    }
    else
    {
        pGhost->pFanin0 = p1;
        pGhost->pFanin1 = p0;
    }
    return pGhost;
}

// internal memory manager
static inline Aig_Obj_t * Aig_ManFetchMemory( Aig_Man_t * p )  
{
    extern char * Aig_MmFixedEntryFetch( Aig_MmFixed_t * p );
    Aig_Obj_t * pTemp;
    pTemp = (Aig_Obj_t *)Aig_MmFixedEntryFetch( p->pMemObjs );
    memset( pTemp, 0, sizeof(Aig_Obj_t) ); 
    Vec_PtrPush( p->vObjs, pTemp );
    pTemp->Id = p->nCreated++;
    return pTemp;
}
static inline void Aig_ManRecycleMemory( Aig_Man_t * p, Aig_Obj_t * pEntry )
{
    extern void Aig_MmFixedEntryRecycle( Aig_MmFixed_t * p, char * pEntry );
    assert( pEntry->nRefs == 0 );
    pEntry->Type = AIG_OBJ_NONE; // distinquishes a dead node from a live node
    Aig_MmFixedEntryRecycle( p->pMemObjs, (char *)pEntry );
    p->nDeleted++;
}


////////////////////////////////////////////////////////////////////////
///                             ITERATORS                            ///
////////////////////////////////////////////////////////////////////////

// iterator over the primary inputs
#define Aig_ManForEachPi( p, pObj, i )                                          \
    Vec_PtrForEachEntry( p->vPis, pObj, i )
// iterator over the primary outputs
#define Aig_ManForEachPo( p, pObj, i )                                          \
    Vec_PtrForEachEntry( p->vPos, pObj, i )
// iterator over all objects, including those currently not used
#define Aig_ManForEachObj( p, pObj, i )                                         \
    Vec_PtrForEachEntry( p->vObjs, pObj, i ) if ( (pObj) == NULL ) {} else
// iterator over all nodes
#define Aig_ManForEachNode( p, pObj, i )                                        \
    Vec_PtrForEachEntry( p->vObjs, pObj, i ) if ( (pObj) == NULL || !Aig_ObjIsNode(pObj) ) {} else
// iterator over the nodes whose IDs are stored in the array
#define Aig_ManForEachNodeVec( p, vIds, pObj, i )                               \
    for ( i = 0; i < Vec_IntSize(vIds) && ((pObj) = Aig_ManObj(p, Vec_IntEntry(vIds,i))); i++ )
// iterator over the nodes in the topological order
#define Aig_ManForEachNodeInOrder( p, pObj )                                    \
    for ( assert(p->pOrderData), p->iPrev = 0, p->iNext = p->pOrderData[1];     \
          p->iNext && (((pObj) = Aig_ManObj(p, p->iNext)), 1);                  \
          p->iNext = p->pOrderData[2*p->iPrev+1] )

// these two procedures are only here for the use inside the iterator
static inline int     Aig_ObjFanout0Int( Aig_Man_t * p, int ObjId )  { assert(ObjId < p->nFansAlloc);  return p->pFanData[5*ObjId];                         }
static inline int     Aig_ObjFanoutNext( Aig_Man_t * p, int iFan )   { assert(iFan/2 < p->nFansAlloc); return p->pFanData[5*(iFan >> 1) + 3 + (iFan & 1)];  }
// iterator over the fanouts
#define Aig_ObjForEachFanout( p, pObj, pFanout, iFan, i )                       \
    for ( assert(p->pFanData), i = 0; (i < (int)(pObj)->nRefs) &&               \
          (((iFan) = i? Aig_ObjFanoutNext(p, iFan) : Aig_ObjFanout0Int(p, pObj->Id)), 1) && \
          (((pFanout) = Aig_ManObj(p, iFan>>1)), 1); i++ )


////////////////////////////////////////////////////////////////////////
///                     SEQUENTIAL ITERATORS                         ///
////////////////////////////////////////////////////////////////////////

// iterator over the primary inputs
#define Aig_ManForEachPiSeq( p, pObj, i )                                       \
    Vec_PtrForEachEntryStop( p->vPis, pObj, i, Aig_ManPiNum(p)-Aig_ManRegNum(p) )
// iterator over the latch outputs
#define Aig_ManForEachLoSeq( p, pObj, i )                                       \
    Vec_PtrForEachEntryStart( p->vPis, pObj, i, Aig_ManPiNum(p)-Aig_ManRegNum(p) )
// iterator over the primary outputs
#define Aig_ManForEachPoSeq( p, pObj, i )                                       \
    Vec_PtrForEachEntryStop( p->vPos, pObj, i, Aig_ManPoNum(p)-Aig_ManRegNum(p) )
// iterator over the latch inputs
#define Aig_ManForEachLiSeq( p, pObj, i )                                       \
    Vec_PtrForEachEntryStart( p->vPos, pObj, i, Aig_ManPoNum(p)-Aig_ManRegNum(p) )
// iterator over the latch input and outputs
#define Aig_ManForEachLiLoSeq( p, pObjLi, pObjLo, k )                           \
    for ( k = 0; (k < Aig_ManRegNum(p)) && (((pObjLi) = Aig_ManLi(p, k)), 1)    \
        && (((pObjLo)=Aig_ManLo(p, k)), 1); k++ )

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== aigCheck.c ========================================================*/
extern int             Aig_ManCheck( Aig_Man_t * p );
extern void            Aig_ManCheckMarkA( Aig_Man_t * p );
extern void            Aig_ManCheckPhase( Aig_Man_t * p );
/*=== aigDfs.c ==========================================================*/
extern Vec_Ptr_t *     Aig_ManDfs( Aig_Man_t * p );
extern Vec_Ptr_t *     Aig_ManDfsNodes( Aig_Man_t * p, Aig_Obj_t ** ppNodes, int nNodes );
extern Vec_Ptr_t *     Aig_ManDfsChoices( Aig_Man_t * p );
extern Vec_Ptr_t *     Aig_ManDfsReverse( Aig_Man_t * p );
extern int             Aig_ManCountLevels( Aig_Man_t * p );
extern int             Aig_DagSize( Aig_Obj_t * pObj );
extern void            Aig_ConeUnmark_rec( Aig_Obj_t * pObj );
extern Aig_Obj_t *     Aig_Transfer( Aig_Man_t * pSour, Aig_Man_t * pDest, Aig_Obj_t * pObj, int nVars );
extern Aig_Obj_t *     Aig_Compose( Aig_Man_t * p, Aig_Obj_t * pRoot, Aig_Obj_t * pFunc, int iVar );
extern void            Aig_ObjCollectCut( Aig_Obj_t * pRoot, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vNodes );
extern int             Aig_ObjCollectSuper( Aig_Obj_t * pObj, Vec_Ptr_t * vSuper );
/*=== aigFanout.c ==========================================================*/
extern void            Aig_ObjAddFanout( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFanout );
extern void            Aig_ObjRemoveFanout( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFanout );
extern void            Aig_ManFanoutStart( Aig_Man_t * p );
extern void            Aig_ManFanoutStop( Aig_Man_t * p );
/*=== aigMan.c ==========================================================*/
extern Aig_Man_t *     Aig_ManStart( int nNodesMax );
extern Aig_Man_t *     Aig_ManStartFrom( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManDup( Aig_Man_t * p, int fOrdered );
extern Aig_Man_t *     Aig_ManExtractMiter( Aig_Man_t * p, Aig_Obj_t * pNode1, Aig_Obj_t * pNode2 );
extern void            Aig_ManStop( Aig_Man_t * p );
extern int             Aig_ManCleanup( Aig_Man_t * p );
extern int             Aig_ManSeqCleanup( Aig_Man_t * p );
extern void            Aig_ManPrintStats( Aig_Man_t * p );
/*=== aigMem.c ==========================================================*/
extern void            Aig_ManStartMemory( Aig_Man_t * p );
extern void            Aig_ManStopMemory( Aig_Man_t * p );
/*=== aigMffc.c ==========================================================*/
extern int             Aig_NodeRef_rec( Aig_Obj_t * pNode, unsigned LevelMin );
extern int             Aig_NodeDeref_rec( Aig_Obj_t * pNode, unsigned LevelMin );
extern int             Aig_NodeMffsSupp( Aig_Man_t * p, Aig_Obj_t * pNode, int LevelMin, Vec_Ptr_t * vSupp );
extern int             Aig_NodeMffsLabel( Aig_Man_t * p, Aig_Obj_t * pNode );
extern int             Aig_NodeMffsLabelCut( Aig_Man_t * p, Aig_Obj_t * pNode, Vec_Ptr_t * vLeaves );
extern int             Aig_NodeMffsExtendCut( Aig_Man_t * p, Aig_Obj_t * pNode, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vResult );
/*=== aigObj.c ==========================================================*/
extern Aig_Obj_t *     Aig_ObjCreatePi( Aig_Man_t * p );
extern Aig_Obj_t *     Aig_ObjCreatePo( Aig_Man_t * p, Aig_Obj_t * pDriver );
extern Aig_Obj_t *     Aig_ObjCreate( Aig_Man_t * p, Aig_Obj_t * pGhost );
extern void            Aig_ObjConnect( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFan0, Aig_Obj_t * pFan1 );
extern void            Aig_ObjDisconnect( Aig_Man_t * p, Aig_Obj_t * pObj );
extern void            Aig_ObjDelete( Aig_Man_t * p, Aig_Obj_t * pObj );
extern void            Aig_ObjDelete_rec( Aig_Man_t * p, Aig_Obj_t * pObj, int fFreeTop );
extern void            Aig_ObjPatchFanin0( Aig_Man_t * p, Aig_Obj_t * pObj, Aig_Obj_t * pFaninNew );
extern void            Aig_ObjReplace( Aig_Man_t * p, Aig_Obj_t * pObjOld, Aig_Obj_t * pObjNew, int fNodesOnly, int fUpdateLevel );
/*=== aigOper.c =========================================================*/
extern Aig_Obj_t *     Aig_IthVar( Aig_Man_t * p, int i );
extern Aig_Obj_t *     Aig_Oper( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1, Aig_Type_t Type );
extern Aig_Obj_t *     Aig_And( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1 );
extern Aig_Obj_t *     Aig_Latch( Aig_Man_t * p, Aig_Obj_t * pObj, int fInitOne );
extern Aig_Obj_t *     Aig_Or( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1 );
extern Aig_Obj_t *     Aig_Exor( Aig_Man_t * p, Aig_Obj_t * p0, Aig_Obj_t * p1 );
extern Aig_Obj_t *     Aig_Mux( Aig_Man_t * p, Aig_Obj_t * pC, Aig_Obj_t * p1, Aig_Obj_t * p0 );
extern Aig_Obj_t *     Aig_Maj( Aig_Man_t * p, Aig_Obj_t * pA, Aig_Obj_t * pB, Aig_Obj_t * pC );
extern Aig_Obj_t *     Aig_Miter( Aig_Man_t * p, Vec_Ptr_t * vPairs );
extern Aig_Obj_t *     Aig_MiterTwo( Aig_Man_t * p, Vec_Ptr_t * vNodes1, Vec_Ptr_t * vNodes2 );
extern Aig_Obj_t *     Aig_CreateAnd( Aig_Man_t * p, int nVars );
extern Aig_Obj_t *     Aig_CreateOr( Aig_Man_t * p, int nVars );
extern Aig_Obj_t *     Aig_CreateExor( Aig_Man_t * p, int nVars );
/*=== aigOrder.c =========================================================*/
extern void            Aig_ManOrderStart( Aig_Man_t * p );
extern void            Aig_ManOrderStop( Aig_Man_t * p );
extern void            Aig_ObjOrderInsert( Aig_Man_t * p, int ObjId );
extern void            Aig_ObjOrderRemove( Aig_Man_t * p, int ObjId );
extern void            Aig_ObjOrderAdvance( Aig_Man_t * p );
/*=== aigPart.c =========================================================*/
extern Vec_Vec_t *     Aig_ManSupports( Aig_Man_t * pMan );
extern Vec_Vec_t *     Aig_ManPartitionSmart( Aig_Man_t * p, int nPartSizeLimit, int fVerbose, Vec_Vec_t ** pvPartSupps );
extern Vec_Vec_t *     Aig_ManPartitionNaive( Aig_Man_t * p, int nPartSize );
/*=== aigRepr.c =========================================================*/
extern void            Aig_ManReprStart( Aig_Man_t * p, int nIdMax );
extern void            Aig_ManReprStop( Aig_Man_t * p );
extern void            Aig_ObjCreateRepr( Aig_Man_t * p, Aig_Obj_t * pNode1, Aig_Obj_t * pNode2 );
extern void            Aig_ManTransferRepr( Aig_Man_t * pNew, Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManDupRepr( Aig_Man_t * p );
extern Aig_Man_t *     Aig_ManRehash( Aig_Man_t * p );
extern void            Aig_ManCreateChoices( Aig_Man_t * p );
/*=== aigSeq.c ========================================================*/
extern int             Aig_ManSeqStrash( Aig_Man_t * p, int nLatches, int * pInits );
/*=== aigShow.c ========================================================*/
extern void            Aig_ManShow( Aig_Man_t * pMan, int fHaig, Vec_Ptr_t * vBold );
/*=== aigTable.c ========================================================*/
extern Aig_Obj_t *     Aig_TableLookup( Aig_Man_t * p, Aig_Obj_t * pGhost );
extern Aig_Obj_t *     Aig_TableLookupTwo( Aig_Man_t * p, Aig_Obj_t * pFanin0, Aig_Obj_t * pFanin1 );
extern void            Aig_TableInsert( Aig_Man_t * p, Aig_Obj_t * pObj );
extern void            Aig_TableDelete( Aig_Man_t * p, Aig_Obj_t * pObj );
extern int             Aig_TableCountEntries( Aig_Man_t * p );
extern void            Aig_TableProfile( Aig_Man_t * p );
/*=== aigTiming.c ========================================================*/
extern void            Aig_ObjClearReverseLevel( Aig_Man_t * p, Aig_Obj_t * pObj );
extern int             Aig_ObjRequiredLevel( Aig_Man_t * p, Aig_Obj_t * pObj );
extern void            Aig_ManStartReverseLevels( Aig_Man_t * p, int nMaxLevelIncrease );
extern void            Aig_ManStopReverseLevels( Aig_Man_t * p );
extern void            Aig_ManUpdateLevel( Aig_Man_t * p, Aig_Obj_t * pObjNew );
extern void            Aig_ManUpdateReverseLevel( Aig_Man_t * p, Aig_Obj_t * pObjNew );
extern void            Aig_ManVerifyLevel( Aig_Man_t * p );
extern void            Aig_ManVerifyReverseLevel( Aig_Man_t * p );
/*=== aigTruth.c ========================================================*/
extern unsigned *      Aig_ManCutTruth( Aig_Obj_t * pRoot, Vec_Ptr_t * vLeaves, Vec_Ptr_t * vNodes, Vec_Ptr_t * vTruthElem, Vec_Ptr_t * vTruthStore );
/*=== aigUtil.c =========================================================*/
extern unsigned        Aig_PrimeCudd( unsigned p );
extern void            Aig_ManIncrementTravId( Aig_Man_t * p );
extern int             Aig_ManLevels( Aig_Man_t * p );
extern void            Aig_ManResetRefs( Aig_Man_t * p );
extern void            Aig_ManCleanMarkA( Aig_Man_t * p );
extern void            Aig_ManCleanMarkB( Aig_Man_t * p );
extern void            Aig_ManCleanData( Aig_Man_t * p );
extern void            Aig_ObjCleanData_rec( Aig_Obj_t * pObj );
extern void            Aig_ObjCollectMulti( Aig_Obj_t * pFunc, Vec_Ptr_t * vSuper );
extern int             Aig_ObjIsMuxType( Aig_Obj_t * pObj );
extern int             Aig_ObjRecognizeExor( Aig_Obj_t * pObj, Aig_Obj_t ** ppFan0, Aig_Obj_t ** ppFan1 );
extern Aig_Obj_t *     Aig_ObjRecognizeMux( Aig_Obj_t * pObj, Aig_Obj_t ** ppObjT, Aig_Obj_t ** ppObjE );
extern Aig_Obj_t *     Aig_ObjReal_rec( Aig_Obj_t * pObj );
extern void            Aig_ObjPrintEqn( FILE * pFile, Aig_Obj_t * pObj, Vec_Vec_t * vLevels, int Level );
extern void            Aig_ObjPrintVerilog( FILE * pFile, Aig_Obj_t * pObj, Vec_Vec_t * vLevels, int Level );
extern void            Aig_ObjPrintVerbose( Aig_Obj_t * pObj, int fHaig );
extern void            Aig_ManPrintVerbose( Aig_Man_t * p, int fHaig );
extern void            Aig_ManDump( Aig_Man_t * p );
extern void            Aig_ManDumpBlif( Aig_Man_t * p, char * pFileName );
/*=== aigWin.c =========================================================*/
extern void            Aig_ManFindCut( Aig_Obj_t * pRoot, Vec_Ptr_t * vFront, Vec_Ptr_t * vVisited, int nSizeLimit, int nFanoutLimit );
 
/*=== aigMem.c ===========================================================*/
// fixed-size-block memory manager
extern Aig_MmFixed_t * Aig_MmFixedStart( int nEntrySize, int nEntriesMax );
extern void            Aig_MmFixedStop( Aig_MmFixed_t * p, int fVerbose );
extern char *          Aig_MmFixedEntryFetch( Aig_MmFixed_t * p );
extern void            Aig_MmFixedEntryRecycle( Aig_MmFixed_t * p, char * pEntry );
extern void            Aig_MmFixedRestart( Aig_MmFixed_t * p );
extern int             Aig_MmFixedReadMemUsage( Aig_MmFixed_t * p );
extern int             Aig_MmFixedReadMaxEntriesUsed( Aig_MmFixed_t * p );
// flexible-size-block memory manager
extern Aig_MmFlex_t *  Aig_MmFlexStart();
extern void            Aig_MmFlexStop( Aig_MmFlex_t * p, int fVerbose );
extern char *          Aig_MmFlexEntryFetch( Aig_MmFlex_t * p, int nBytes );
extern void            Aig_MmFlexRestart( Aig_MmFlex_t * p );
extern int             Aig_MmFlexReadMemUsage( Aig_MmFlex_t * p );
// hierarchical memory manager
extern Aig_MmStep_t *  Aig_MmStepStart( int nSteps );
extern void            Aig_MmStepStop( Aig_MmStep_t * p, int fVerbose );
extern char *          Aig_MmStepEntryFetch( Aig_MmStep_t * p, int nBytes );
extern void            Aig_MmStepEntryRecycle( Aig_MmStep_t * p, char * pEntry, int nBytes );
extern int             Aig_MmStepReadMemUsage( Aig_MmStep_t * p );

#ifdef __cplusplus
}
#endif

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

